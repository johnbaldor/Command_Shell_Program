doit Command Shell Program
This program executes commands in a Linux environment and provides detailed statistics about system resource usage, including CPU time, wall-clock time, and page faults. It can also function as a basic command shell, handling built-in commands, changing directories, and executing background tasks.

To compile the program, run the command:
gcc doit.c -o doit

To run the program with a command, use:
./doit [command] [arguments]

Example:
./doit wc foo.txt

To run the program as a shell, use:
./doit

Features
Command Execution: Executes any valid Linux command and provides resource usage statistics (CPU time, page faults, etc.).
Built-in Commands:
exit: Terminates the shell.
cd [dir]: Changes the current directory.
set prompt = [newprompt]: Changes the shell prompt.
Background Tasks: Supports background task execution with & and provides a jobs command to list background tasks.
