# Smash - Small Shell

This project implements a simplified Unix shell in C++ as part of an Operating Systems assignment.  
The shell, called `smash`, reads commands from the user, parses them, executes built-in commands directly, and runs external commands using process creation and execution system calls.

## Project Files

| File | Description |
|---|---|
| `smash.cpp` | Main shell loop. Initializes signal handlers, prints the prompt, reads commands, and dispatches them to `SmallShell`. |
| `Commands.h` | Class declarations for the shell, commands, jobs list, and all supported command types. |
| `Commands.cpp` | Main implementation of command parsing, built-in commands, external commands, jobs, pipes, redirection, aliases, and utility commands. |
| `signals.h` | Signal handler declarations. |
| `signals.cpp` | Signal handling implementation, including `Ctrl-C` handling. |
| `Makefile` | Build, test, submit, and clean rules. |

## Build Instructions

To build the shell executable, run:

```bash
make smash
```

This creates the executable:

```bash
./smash
```

To clean generated files, run:

```bash
make clean
```

To create the submission ZIP file on the required Ubuntu image, run:

```bash
make submit
```

The submission archive name is based on the IDs in the Makefile:

```text
213701790_325040392.zip
```

## Running the Shell

After compiling, run:

```bash
./smash
```

The default prompt is:

```text
smash>
```

The shell then waits for user commands.

## Supported Built-in Commands

### `chprompt`

Changes the shell prompt.

```bash
chprompt myprompt
```

Result:

```text
myprompt>
```

Running `chprompt` without arguments restores the default prompt:

```bash
chprompt
```

### `showpid`

Prints the process ID of the shell.

```bash
showpid
```

### `pwd`

Prints the current working directory.

```bash
pwd
```

### `cd`

Changes the current working directory.

```bash
cd /some/path
```

The command also supports returning to the previous directory:

```bash
cd -
```

### `jobs`

Prints the current background jobs list.

```bash
jobs
```

### `fg`

Brings a background job to the foreground.

```bash
fg
```

or:

```bash
fg <job-id>
```

### `kill`

Sends a signal to a job by job ID.

```bash
kill -<signal-number> <job-id>
```

Example:

```bash
kill -9 1
```

### `quit`

Exits the shell.

```bash
quit
```

To kill all remaining jobs before exiting:

```bash
quit kill
```

### `alias`

Defines command aliases.

```bash
alias ll='ls -l'
```

Running `alias` without arguments prints all aliases:

```bash
alias
```

### `unalias`

Removes one or more aliases.

```bash
unalias ll
```

### `unsetenv`

Removes environment variables from the shell process environment.

```bash
unsetenv VAR_NAME
```

### `sysinfo`

Prints system information, including system name, hostname, kernel version, architecture, and boot time.

```bash
sysinfo
```

### `whoami`

Prints the current user information, including UID, GID, username, and home directory.

```bash
whoami
```

### `du`

Calculates disk usage for a directory.

```bash
du
```

or:

```bash
du /some/directory
```

### `usbinfo`

Prints information about connected USB devices using `/sys/bus/usb/devices`.

```bash
usbinfo
```

## External Commands

Commands that are not built-in are executed as external programs.

Example:

```bash
ls -l
```

The shell supports both simple external commands and complex commands containing wildcard characters such as `*` or `?`. Complex commands are executed through `/bin/bash -c`.

## Background Commands

A command ending with `&` runs in the background.

Example:

```bash
sleep 10 &
```

Background commands are stored in the jobs list and can be viewed using:

```bash
jobs
```

They can later be moved to the foreground using:

```bash
fg <job-id>
```

## Redirection

The shell supports output redirection.

Overwrite a file:

```bash
ls > out.txt
```

Append to a file:

```bash
ls >> out.txt
```

## Pipes

The shell supports regular pipes:

```bash
ls | grep cpp
```

It also supports piping standard error using `|&`:

```bash
ls missing_file |& grep error
```

## Signal Handling

The shell installs a handler for `Ctrl-C` / `SIGINT`.

When `Ctrl-C` is pressed while a foreground process is running, the shell sends `SIGKILL` to the foreground process and prints a message indicating that the process was killed.

## Implementation Overview

The project is object-oriented. Each command type is represented by a class derived from the base `Command` class.

Main components:

- `SmallShell` is implemented as a singleton and manages the shell state.
- `Command` is the abstract base class for all commands.
- `BuiltInCommand` is the base class for shell commands executed inside the shell process.
- `ExternalCommand` handles fork/exec execution of non-built-in commands.
- `JobsList` stores background jobs and manages job IDs.
- `RedirectionCommand` handles `>` and `>>` using `dup`, `dup2`, `open`, and `close`.
- `PipeCommand` handles `|` and `|&` using `pipe`, `fork`, and file descriptor duplication.
- `signals.cpp` handles `SIGINT`.

## Notes

- The project uses Linux system calls such as `fork`, `execvp`, `execv`, `waitpid`, `kill`, `setpgrp`, `open`, `read`, `close`, `dup`, `dup2`, `pipe`, `stat`, `lstat`, and `getdents64`.
- The shell removes finished background jobs before executing new commands.
- Background jobs are assigned increasing job IDs.
- The Makefile includes a `test` target for input/output-based tests if matching test files are provided.
