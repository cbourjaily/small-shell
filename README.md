# Small Shell

A Unix-like command shell implemented in C.

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
- Interactive shell prompt displaying the current working directory

## Requirements

- GCC
- GNU Make
- POSIX-compliant operating system (Linux)

## Building

Compile with GCC:

```bash
make

```

Clean the build:

```bash
make clean
```

## Running

Start the shell:

```bash
./smallsh
```

Example session:

```text
~/projects/smallsh: ls
junk  junk2  LICENSE  Makefile	README.md  smallsh  smallsh.c  tests
~/projects/smallsh: ls > junk
~/projects/smallsh: wc < junk > junk2
~/projects/smallsh: cat junk2
 8  8 62
~/projects/smallsh: sleep 5
^Cterminated by signal 2
~/projects/smallsh: status
terminated by signal 2
~/projects/smallsh: sleep 40 &
background pid is 2185952
~/projects/smallsh: kill -15 2185952
background pid 2185952 is done: terminated by signal 15
~/projects/smallsh: ^Z
Entering foreground-only mode (& is now ignored)
~/projects/smallsh: sleep 5 &
~/projects/smallsh: ^Z
Exiting foreground-only mode
~/projects/smallsh: cd
~: pwd
/home/user
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
