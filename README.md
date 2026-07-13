# Small Shell

A Unix-like shell implemented in C.

This project demonstrates core Unix systems programming concepts including process creation, signal handling, job control, input/output redirection, and command execution.

## Features

- Execute external programs using `fork()` and `execvp()`
- Built-in commands:
  - `cd`
  - `status`
  - `exit`
- Foreground and background process execution
- Input (`<`) and output (`>`) redirection
- Background job management
- Signal handling
  - `SIGINT` for foreground processes
  - `SIGTSTP` to toggle foreground-only mode
  - `SIGCHLD` for background process cleanup
- Automatic zombie process reaping

## Building

Compile with GCC:

```bash
gcc -o smallsh smallsh.c

```

## Running

Start the shell:

```bash
./smallsh
```

Example session:

```text
~/projects/smallsh$ ls
LICENSE
README.md
smallsh
smallsh.c

~/projects/smallsh$ pwd
/home/user/projects/smallsh

~/projects/smallsh$ sleep 10 &
background pid is 12345

~/projects/smallsh$ status
exit value 0

~/projects/smallsh$ echo hello > out.txt

~/projects/smallsh$ cat < out.txt
hello

~/projects/smallsh$ exit
```

## Topics

- C
- Unix
- Shell
- Operating Systems
- Process Management
- `fork()`
- `execvp()`
- `waitpid()`
- Signals
- Job Control
- I/O Redirection
