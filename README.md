# Custom Unix Shell with Process Management

This is a lightweight custom Unix shell written in C that mimics core functionality of standard Unix shells like `bash`. It supports sequential and parallel command execution, background processes, and built-in commands like `cd` and `exit`.

## Features

- **Foreground & Background Execution**  
  Use `&` to run processes in the background.

- **Sequential Commands (`&&`)**  
  Execute multiple commands one after another.

- **Parallel Commands (`&&&`)**  
  Run multiple commands in parallel.

- **Built-in Commands**  
  - `cd [directory]`: Change directory  
  - `exit`: Exit the shell

- **Signal Handling**
  - `SIGCHLD`: Handles cleanup of background processes
  - `SIGINT`: Interrupts foreground processes (Ctrl+C)
  - `SIGUSR1`: Used to re-display prompt after background completion

- **Interactive and Batch Mode**
  - Interactive mode: Enter commands in real time  
  - Batch mode: Run commands from a file

## Build Instructions

To compile the shell:

```bash
make
```

## Usage

### Interactive Mode
```bash
./my_shell
```

Example:
```bash
ls && pwd
ls &&& pwd
sleep 5 &
```

### Batch Mode
Create a file with commands:
```bash
echo "ls && pwd" > commands.txt
```

Run the shell in batch mode:
```bash
./my_shell commands.txt
```

## Files

- `my_shell.c` – Main source file for the shell
- `Makefile` – Automates compilation
- `README.md` – Project documentation
