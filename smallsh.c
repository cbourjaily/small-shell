/* Program Name: smallsh.c
 * Author: Christopher Vote
 *
 * Description:
 * This program implements a small shell. It supports general shell commands and scripts
 * and includes built-in commands: exit, cd, and status.
*/


#include <stdio.h>	// For printf(), perror()
#include <stdbool.h>	// For bool
#include <stdlib.h>	// For exit(), 
#include <string.h>	// For strcmp(), strlen(), strtok(), strdup(), strcspn()
#include <unistd.h>	// For chdir(), getcwd(), execvp(), getpid(), fork()
#include <sys/wait.h>	// For waitpid(), 
#include <sys/types.h>	// For pid_t
#include <errno.h>	// For eerno, EINTR
#include <fcntl.h>	// For open()

#define INPUT_LENGTH 2048
#define MAX_ARGS 512
#define EXIT_MESSAGE "exit value"
#define SIGNAL_MESSAGE "terminated by signal"
#define FOREGROUND_ON "\nEntering foreground-only mode (& is now ignored)\n"
#define FOREGROUND_OFF "\nExiting foreground-only mode\n"
#define ON_LENGTH 50
#define OFF_LENGTH 30


// Foreground only state, initially zero for false
int foreground_only = 0;

// last foreground exit, updated by execute_foreground() and read by status()
char current_status[30];
void set_status(char* message, int value);

// Struct for storing result of background exit
struct bg_result {
	pid_t pid;
	int exit_code;
	bool normal_exit;			// True if normal, false if triggered by signal
};

// Array to hold results
struct bg_result finished_bgs[MAX_ARGS];
volatile sig_atomic_t finished_count = 0;

struct command_line
{	
//	command is words separated by spaces
	char *argv[MAX_ARGS + 1];
	int argc;
//	Standard input/output redirection with >/<  followed by a filename after all arguments.
// 			Input redirection can appear before or after output redirection.
	char *input_file;
	char *output_file;

//	If the command is to be executed in the background, the last word must be &
	bool is_bg;
};



struct command_line *parse_input()
{
	char input[INPUT_LENGTH];

	while (true) {
		printf(": ");
		fflush(stdout);
			
		if (fgets(input, INPUT_LENGTH, stdin) == NULL) {
			if (errno == EINTR) {
				// a signal has occured, clear error and restart loop
				clearerr(stdin);
				continue;
			}
			return NULL;
		}
		break;
	}

	input[strcspn(input, "\n")] = 0;
	
	if (input[0] == '\0' || input[0] == '#') {
		return NULL;
	}

	struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));

	// Tokenize the input
	char *token = strtok(input, " \n");
	while (token) {
		if (!strcmp(token, "<")) {
			curr_command->input_file = strdup(strtok(NULL," \n"));
		}
		else if (!strcmp(token, ">")) {
			curr_command->output_file = strdup(strtok(NULL, " \n"));
		}
		else if (!strcmp(token, "&")) {

			// only execute in bg if not in foreground only mode
			if (!foreground_only) {
				curr_command->is_bg = true;
			}
		}
		else {
			curr_command->argv[curr_command->argc++] = strdup(token);
		}
		token=strtok(NULL," \n");
	}
	// Add null terminator
	curr_command->argv[curr_command->argc] = NULL;
	
	return curr_command;
}

int check_built_ins(struct command_line *curr_command);
void route_built_in(struct command_line *curr_command); 
void process_exit(struct command_line *curr_command);
void process_cd(struct command_line *curr_command);
void print_status();
void set_status(char* message, int value);
void route_command(struct command_line *curr_command);
void execute_foreground(struct command_line *curr_command);
void execute_background(struct command_line *curr_command);
void handle_SIGTSTP (int signo);
void handle_SIGCHLD(int signo);
void check_zombies(); 
void free_command(struct command_line *cmd);


int main() {
	struct command_line *curr_command;

	// Initialize foreground process exit state
	set_status(EXIT_MESSAGE, 0);
	
	// Initialize SIGINT_action struct to be empty
	struct sigaction SIGINT_action = {0};
	// Set SIGINT_action signal handler to ignore
	//  		Shell and bg children  must ignore SIGINT
	SIGINT_action.sa_handler = SIG_IGN;
	// No flags
	SIGINT_action.sa_flags = 0;
	// Install signal handler
	sigaction(SIGINT, &SIGINT_action, NULL);

	// SIGTSTP: Children must ignore, shell displays message
	struct sigaction SIGTSTP_action = {0};

	// Set signal handler to handle_SIGTSTP(), which toggles foreground_only state
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	// Initialize handle_SIGCHLD
	struct sigaction SIGCHLD_action = {0};
	// Set handler function
	SIGCHLD_action.sa_handler = handle_SIGCHLD;
	SIGCHLD_action.sa_flags = 0;
	sigaction(SIGCHLD, &SIGCHLD_action, NULL);

	while (true)
	{
		check_zombies();

		// Retrieve next command	
		curr_command = parse_input();
		
		// Handle blank lines and comments
		if (curr_command == NULL) {
			continue;
		}

		// Check for arguments
		if (check_built_ins(curr_command)) {					// Can probably change to !check_built_ins and combine check/route logic below
			route_built_in(curr_command);
		}
		else {
			route_command(curr_command);					// in that case, !built_in would just call this. 
		}
		// Check for completed background processes
		free_command(curr_command);
	}
	return EXIT_SUCCESS;
}

int check_built_ins(struct command_line *curr_command) {
	// Built-in Commands exit, cd, and status handled by shell (others passed to exec())
	// 	no redirection or exit status needed for built-ins
	// 		run in foreground and ignore &
	
	// Check if command is exit, cd, or status and return 0 if not.	
	if (!strcmp(curr_command->argv[0], "exit")) {
		return 1;
	}
	if (!strcmp(curr_command->argv[0], "cd")) {
		return 1;
	}
	if (!strcmp(curr_command->argv[0], "status")) {
		return 1;
	}
	// No built-ins found
	return 0;
}


void route_built_in(struct command_line *curr_command) {
    if (!strcmp(curr_command->argv[0], "exit")) {
        process_exit(curr_command);
    }
    else if (!strcmp(curr_command->argv[0], "cd")) {
        process_cd(curr_command);
    }
    else if (!strcmp(curr_command->argv[0], "status")) {
        print_status();
    }
}

void process_exit(struct command_line *curr_command) {
	// kill background processes
	for (int i = 0; i < finished_count; i++) {
		kill(finished_bgs[i].pid, SIGTERM);
	}

	// wait for zombies
	for (int i = 0; i < finished_count; i++) {
		waitpid(finished_bgs[i].pid, NULL, 0);
	}
	free_command(curr_command);
	exit(0);
}

void process_cd(struct command_line *curr_command) {

	if (curr_command->argc > 1) {
		// chdir returns -1 on failure
		if (chdir(curr_command->argv[1]) == -1) {
			perror("chdir");
		}
	}
	// If no arguments provided go to HOME directory
	else {
		char* home = getenv("HOME");
		if (home != NULL) {
			if (chdir(home) == -1) {
				perror("chdir");
			}
		}
	}
}

void print_status() {
	// prints exit status or terminating signal of last foreground process
	printf("%s\n", current_status);
}

void set_status(char* message, int value) {
	sprintf(current_status, "%s %d", message, value);
}

void route_command(struct command_line *curr_command) {
	if (curr_command->is_bg) {
		execute_background(curr_command);
	} else {
		execute_foreground(curr_command);
	}
}


/* Code citation: Aspects of the function execute_foreground are adapted from example code on Module 6: Exploration: Process API - Executing a New Program */
/* Code citation: Aspects of file redirection adapted from Module 7: Exploration: Processes and I/O */
void execute_foreground(struct command_line *curr_command) {
	
	// Create child_status variable
	int child_status;

	// Fork child process
	pid_t spawn_pid = fork();

	switch (spawn_pid) {
		case -1:
			perror("fork()\n");
			exit(1);
		case 0:		
			if (curr_command->input_file) {
				// Open source file for read only
				int source_fd = open(curr_command->input_file, O_RDONLY);
				if (source_fd == -1) {
					char msg[INPUT_LENGTH];
					int len = snprintf(msg, sizeof(msg), "cannot open %s for input\n", curr_command->input_file);
					write(STDERR_FILENO, msg, len);
					exit(EXIT_FAILURE);
				}
				// Redirect stdin to source file
				int result = dup2(source_fd, 0);
				if (result == -1) {
					perror("source dup2");
					exit(EXIT_FAILURE);
				}
				close(source_fd);
			}

			if (curr_command->output_file) {
				// Open target file for writing only, truncate, create
				int target_fd = open(curr_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (target_fd == -1) {
					char msg[INPUT_LENGTH];
					int len = snprintf(msg, sizeof(msg), "cannot open %s for output\n", curr_command->output_file);
					write(STDERR_FILENO, msg, len);
					exit(EXIT_FAILURE);
				}

				// Redirect stdout to target file
				int result = dup2(target_fd, 1);
				if (result == -1) {
					perror("target dup2()");
					exit(EXIT_FAILURE);
				}
				close(target_fd);
			}

			// Child running in foreground must terminate at SGINT
			// 	Rebuild signal handling
			struct sigaction SIGINT_action = {0};
	
			// Set SGINT back to default
			SIGINT_action.sa_handler = SIG_DFL;
			
			// No flags
			SIGINT_action.sa_flags = 0;

			// Install signal handler 
			sigaction(SIGINT, &SIGINT_action, NULL);

			// Search path for executable using execvp
			execvp(curr_command->argv[0], curr_command->argv);

			// execvp returns only if there is an error
			fprintf(stderr, "%s: no such file or directory\n", curr_command->argv[0]);
			fflush(stderr);
			exit(EXIT_FAILURE);
		default:

			pid_t child_pid;

			do {												// NEW
				child_pid = waitpid(spawn_pid, &child_status, 0);
			} while (child_pid == -1 && errno == EINTR);

			if (child_pid == -1) {
				perror("waitpid");
				return;
			}
			
			// Save exit code
			if (WIFEXITED(child_status)) {
				// Set status
				set_status(EXIT_MESSAGE, WEXITSTATUS(child_status));
				break;
			}
						
			else if (WIFSIGNALED(child_status)) {
				// Get the signal with WTERMSIG
				int signal = WTERMSIG(child_status);
				char message[50];
				// Set status
				set_status(SIGNAL_MESSAGE, WTERMSIG(child_status));

				// Write to terminal "terminated by signal #\n
				sprintf(message, "%s %d\n", SIGNAL_MESSAGE, signal);
				write(STDOUT_FILENO, message, strlen(message));
				fflush(stdout);
				break;
			}
			
	}
}
			
/* Code citation: important execute_background functionality has been adapted from Module 6: Exploration: Process API - MOnitoring Child Processes */
void execute_background(struct command_line *curr_command) {
	// Fork child process
	pid_t spawn_pid = fork();

	switch (spawn_pid) {
		case -1:
			perror("fork()");
			exit(1);
		case 0: {
				// Redirect input
				int source_fd;
				if (curr_command->input_file) {
					source_fd = open(curr_command->input_file, O_RDONLY);
					if (source_fd == -1) {
						char msg[INPUT_LENGTH];
						int len = snprintf(msg, sizeof(msg), "cannot open %s for input\n", curr_command->input_file);
						write(STDERR_FILENO, msg, len);
						exit(EXIT_FAILURE);
					}
				} else {
					// If no input file specified, redirect from /dev/null
					source_fd = open("/dev/null", O_RDONLY);
					if (source_fd == -1) {
						perror("open /dev/null for input");
						exit(EXIT_FAILURE);
					}
				}
				if (dup2(source_fd, STDIN_FILENO) == -1) {
					perror("dup2 input");
					exit(EXIT_FAILURE);
				}
				close(source_fd);

				// Redirect output
				int target_fd;
				if (curr_command->output_file) {
					target_fd = open(curr_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (target_fd == -1) {
						char msg[INPUT_LENGTH];
						int len = snprintf(msg, sizeof(msg), "cannot open %s for output\n", curr_command->output_file);
						write(STDERR_FILENO, msg, len);
						exit(EXIT_FAILURE);
					}
				} else {
					// No output specified so redirect to /dev/null
					target_fd = open("/dev/null", O_WRONLY);
					if (target_fd == -1) {
						perror("open /dev/null for output");
						exit(EXIT_FAILURE);
					}
				}
				if (dup2(target_fd, STDOUT_FILENO) == -1) {
					perror("dup2 output");
					exit(EXIT_FAILURE);
				}
				close(target_fd);

				// signals
				struct sigaction SIGINT_action = {0};
				SIGINT_action.sa_handler = SIG_IGN;
				sigaction(SIGINT, &SIGINT_action, NULL);
			
				// Search path for executable using execvp
				execvp(curr_command->argv[0], curr_command->argv);

				// execvp returns only if there is an error
				fprintf(stderr, "%s: no such file or directory\n", curr_command->argv[0]);
				fflush(stderr);
				exit(EXIT_FAILURE);
			}
		default:
			printf("background pid is %d\n", spawn_pid);
			return;
	}
}

void handle_SIGTSTP (int signo) {
	// Toggle global state variable foreground_only
	foreground_only ^= 1;

	// Write message FOREGROUND_ON and FOREGROUND_OFF
	if (!foreground_only) {
		write(STDOUT_FILENO, FOREGROUND_OFF, OFF_LENGTH);
	} else {
		write(STDOUT_FILENO, FOREGROUND_ON, ON_LENGTH);
	}
}

/* Code citation: Elements of handle_SIGCHLD adapted from Kerrisk (2010), The Linux Programming Interface: pp. 556-8 */
void handle_SIGCHLD(int signo) {
	int child_status;
	pid_t child_pid;
	int saved_errno = errno;
	
	// Loop repeatedly calling waitpid() with WNOHANG flag until all children are reaped.
	// 	The loop continues until waitpid() returns 0 for no more stopped children or -1 for error
	while ((child_pid = waitpid(-1, &child_status, WNOHANG)) > 0) {
		// count number of digits in child_pid for write()
		if (finished_count < MAX_ARGS) {
			finished_bgs[finished_count].pid = child_pid;
			if (WIFEXITED(child_status)) {
				finished_bgs[finished_count].exit_code = WEXITSTATUS(child_status);
				finished_bgs[finished_count].normal_exit = true;
			}
			else if (WIFSIGNALED(child_status)) {
				finished_bgs[finished_count].exit_code = WTERMSIG(child_status);
				finished_bgs[finished_count].normal_exit = false;
			}
			finished_count++;
		}
	}
	errno = saved_errno;
}

void check_zombies() {
	for (int i = 0; i < finished_count; i++) {
		if (finished_bgs[i].normal_exit) {
			printf("background pid %d is done: exit value %d\n", 
					finished_bgs[i].pid, finished_bgs[i].exit_code);
		} else {
			printf("background pid %d is done: terminated by signal %d\n", 
					finished_bgs[i].pid, finished_bgs[i].exit_code);
		}
	}
	fflush(stdout);
	finished_count = 0;			// Reset count
}

void free_command(struct command_line *cmd) {
	if (cmd == NULL) return;

	for (int i = 0; i < cmd->argc; i++) {
		free(cmd->argv[i]);
	}
	free(cmd->input_file);
	free(cmd->output_file);
	free(cmd);
}
