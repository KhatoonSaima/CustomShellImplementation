/* 
* w25shell - A Simple Custom Shell Implementation in C
*
* Description:
*   This program implements a custom shell named `w25shell` that supports
*   various commands, piping, redirection, file operations, and conditional
*   execution. It is designed to mimic basic shell functionality without using
*   the `system()` library function.
* 
* Features:
*   `|`, Piping: Connects the output of one command to the input of another.
*   `=`, Reverse Piping: Reverses the flow of piping, sending output backward.
*   `~`, Append Operation: Appends the contents of one file to another and vice versa.
*   `#`, Word Count: Counts the number of words in a specified file.
*   `+`, File Concatenation: Concatenates up to 5 files and prints to stdout.
*   `<`, Input Redirection: Reads input from a file instead of stdin.
*   `>`, Output Redirection: Writes command output to a file (overwrites if exists).
*   `>>`, Append Output Redirection: Appends command output to a file.
*   `;`, Sequential Execution: Executes multiple commands sequentially.
*   `&&` and `||`, Conditional Execution: Executes commands based on the success or failure of previous commands.
*
* 
* Examples:
*   `w25shell$ ls -l | grep .txt`
*   `w25shell$ wc -w = wc = ls -l`
*   `w25shell$ file1.txt + file2.txt + file3.txt`
*   `w25shell$ file1.txt ~ file2.txt`
*   `w25shell$ # file.txt`
*   `w25shell$ grep Windsor < input.txt`
*   `w25shell$ ls -l > output.txt`
*   `w25shell$ ls -l >> output.txt`
*   `w25shell$ date ; pwd ; ls -l`
*   `w25shell$ pwd && date || ls`

* Author: Saima Khatoon
* Date: 14th March 2025
*/
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ftw.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pwd.h>
#include <dirent.h>
#include <glob.h>

#define MAX_ARGS 5          // Maximum number of arguments allowed for a command
#define MAX_PIPES 5         // Maximum number of pipes operation supported
#define MAX_CMDS 6          // 5 pipes + 1 command, max number of command allowed is 6 and that of pipe is 5
#define MAX_FILES 5         // Maximum number of files supported
#define BUFFER_SIZE 1024    // Length of the user input
#define MAX_CONDITIONS 5    // Maximum allowed conditional statements
#define MAX_FILENAME 256    // Maximum length of the filename


// Function prototypes
void execute_command(char **args);
void kill_terminal();
void kill_all_terminals();
void execute_piped_commands(char *command);
void execute_reverse_piped_commands(char *command);
void append_files(char *file1, char *file2);
void count_words(char *filename);
void concatenate_files(char **files, int num_files);
void execute_redirection(char *command);
void preprocess_input(char *input);
void trim_leading_spaces(char *str);
void execute_command_chain(char *command);
void execute_conditional_execution(char *command);

// Kill the current terminal
void kill_terminal() 
{
    printf("Terminating current w25shell terminal.\n");
    kill(getpid(), SIGTERM);    // Send SIGTERM signal
}

// Kill all terminals owned by the current user with the same executable name
void kill_all_terminals() 
{
   printf("Shutting down all w25shell terminals...\n");

    // Get the current user's UID
    uid_t current_uid = getuid();

    // Get the current process name
    char process_name[256];
    FILE *comm_file = fopen("/proc/self/comm", "r");
    if (comm_file == NULL) 
    {
        perror("Error opening /proc/self/comm");
        return;
    }
    if (fgets(process_name, sizeof(process_name), comm_file) == NULL) 
    {
        perror("Error reading process name");
        fclose(comm_file);
        return;
    }
    fclose(comm_file);

    // Remove the newline character from the process name
    process_name[strcspn(process_name, "\n")] = '\0';

    // Open the /proc directory to iterate through all processes
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) 
    {
        perror("Error opening /proc");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) 
    {
        // Check if the directory name is a process ID (PID)
        if (entry->d_type == DT_DIR && atoi(entry->d_name) > 0) 
        {
            char path[256];
            snprintf(path, sizeof(path), "/proc/%s/comm", entry->d_name);

            // Open the process's comm file to read the command name
            FILE *comm_file = fopen(path, "r");
            if (comm_file == NULL) 
            {
                continue; // Skip if the comm file cannot be opened
            }

            

            char comm[256];
            if (fgets(comm, sizeof(comm), comm_file) != NULL) 
            {
                // Remove the newline character from the command name
                comm[strcspn(comm, "\n")] = '\0';

                // Check if the process has the same name as the current process
                if (strcmp(comm, process_name) == 0) 
                {
                    // Get the process's UID
                    snprintf(path, sizeof(path), "/proc/%s/status", entry->d_name);
                    FILE *status_file = fopen(path, "r");
                    if (status_file == NULL) 
                    {
                        fclose(comm_file);
                        continue; // Skip if the status file cannot be opened
                    }

                    uid_t process_uid = 0;
                    char line[256];
                    while (fgets(line, sizeof(line), status_file) != NULL) 
                    {
                        if (strncmp(line, "Uid:", 4) == 0) 
                        {
                            // Extract the UID from the line
                            sscanf(line, "Uid:\t%d", &process_uid);
                            break;
                        }
                    }

                    fclose(status_file);

                    // Get the process Id
                    pid_t pid = atoi(entry->d_name);
                    //printf("pid:%d\n", pid);
                    
                    // Check if the process is owned by the current user
                    if (process_uid == current_uid && pid != getpid() ) 
                    {
                        //pid_t pid = atoi(entry->d_name);
                        if (kill(pid, SIGTERM) == 0)    // Terminate the process using kill()
                        {
                            // For Debugging purpose
                            //printf("Terminated process with PID %d\n", pid);
                        } 
                        else    // Error condition
                        {
                            perror("Error terminating process");
                        }
                    }
                }
            }

            // Close the file pointer
            fclose(comm_file);
        }
    }

    // Close the proc directory
    closedir(proc_dir);

     // Finally, shut down the current shell
    printf("Closing this terminal... Goodbye!\n");
}


// option 1
// Handles piping operations between commands
void execute_piped_commands(char *input) 
{
    char *commands[MAX_CMDS][MAX_ARGS + 1];     // 2-D Array to store all the commands
    int numcmds = 0;
    char *cmd_tokens[MAX_CMDS];                 // Declare array to store each command 
    // If user input is "ls -l -a | wc", cmd_tokens[0] = "ls -l -a" and cmd_tokens[1] = "wc"

    // Split user input into a series of tokens based on specified delimiter characters "|"
    // On the first call to strtok(), first argument should point to the string you want to tokenize. 
    char *command = strtok(input, "|");   
    while (command != NULL) 
    {
        // Find tokens and store it in cmd_tokens array
        cmd_tokens[numcmds++] = command;   
        //printf("cmd_tokens:%s\n", cmd_tokens[numcmds-1]);  
        // On subsequent calls, first argument of strtok() should be NULL to continue tokenizing the same string.   
        command = strtok(NULL, "|");
    }
    
    // For debugging purpose
    //printf("No. of commands: %d\n", numcmds);

    // Number of commands allowed is 6 and number of pipes allowed is 5
    if (numcmds > 6) 
    {
        fprintf(stderr, "Error: Too many pipes (max 5 allowed).\n");
        return;
    }
    
    int arg_count;

    for (int i = 0; i < numcmds; i++) 
    {
        arg_count = 0;      // Number of arguments of a command

        // Split each command into command name and its attributes based on space character
        char *arg_token = strtok(cmd_tokens[i], " ");
        while (arg_token != NULL) 
        {
            // In one command maximum number of arguments allowed is 5 including command name
            // Here MAX_ARGS does not include the command name
            if (arg_count >= MAX_ARGS)       // Rule 4
            {
                fprintf(stderr, "Error: Too many arguments in command %d (max %d allowed).\n", i + 1, MAX_ARGS);
                return;
            }
            commands[i][arg_count++] = arg_token;       // Store each command with its arguments into array
            arg_token = strtok(NULL, " ");
        }
        commands[i][arg_count] = NULL;
    }

    // For debugging
    // Print commands stored in array
    // For user input ls -l -a | wc |wc -c
    /*
    commands matrix will be =
        ls	-l	-a	
        wc	
        wc	-c*/
    // Debugging output
    /*printf("Parsed Commands:\n");
    for (int i = 0; i < numcmds; i++)
    {
        printf("Command %d: ", i + 1);
        for (int j = 0; commands[i][j] != NULL; j++)
        {
            printf("%s ", commands[i][j]);
        }
        printf("\n");
    }*/

   // Create pipes for communication between commands
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];
    
    for (int i = 0; i < numcmds - 1; i++)
    {
        if (pipe(pipes[i]) == -1) 
        {
            perror("Pipe failed");
            exit(EXIT_FAILURE);
        }
    }
    
    // Fork processes for each command
    for (int i = 0; i < numcmds; i++) 
    {
        pids[i] = fork();
        if (pids[i] == 0) 
        {
            // Redirect input 
            if (i > 0) 
                dup2(pipes[i - 1][0], STDIN_FILENO);
            
            // Redirect output
            if (i < numcmds - 1) 
                dup2(pipes[i][1], STDOUT_FILENO);

            // Close all pipe file descriptors
            for (int j = 0; j < numcmds - 1; j++) 
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command directly using execvp
            if (execvp(commands[i][0], commands[i]) == -1) 
            {
                perror("Error executing command");
                return;
            }
        }
    }
    
    // Close all pipe file descriptors in the parent process
    for (int i = 0; i < numcmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all child processes to finish
    for (int i = 0; i < numcmds; i++) {
        waitpid(pids[i], NULL, 0);
    }
}


// Option2
// Handles reverse piping operations between commands
void execute_reverse_piped_commands(char *input) 
{
     char *commands[MAX_CMDS][MAX_ARGS + 1]; // 2-D array to store all commands and their arguments
    int numcmds = 0;
    char *cmd_tokens[MAX_CMDS]; // Array to store each command

    // Split user input into commands based on the "=" delimiter
    char *command = strtok(input, "=");
    while (command != NULL) 
    {
        cmd_tokens[numcmds++] = command;
        command = strtok(NULL, "=");
    }

    // Number of commands allowed is 6 and number of reverse pipes allowed is 5
    if (numcmds > MAX_CMDS) 
    {
        fprintf(stderr, "Error: Too many reverse pipes (max %d allowed).\n", MAX_PIPES);
        return;
    }

    // Parse each command into arguments
    for (int i = 0; i < numcmds; i++) 
    {
        int arg_count = 0;
        char *arg_token = strtok(cmd_tokens[i], " ");
        while (arg_token != NULL) 
        {
            commands[i][arg_count++] = arg_token;
            arg_token = strtok(NULL, " ");
        }
        commands[i][arg_count] = NULL; // Null-terminate the argument list

        // Validate the number of arguments
        if (arg_count < 1 || arg_count > MAX_ARGS) 
        {
            fprintf(stderr, "Error: Command %d has invalid number of arguments (must be >= 1 and <= %d).\n", i + 1, MAX_ARGS);
            return;
        }
    }

    // Create pipes for communication between commands
    int pipes[MAX_CMDS - 1][2];
    for (int i = 0; i < numcmds - 1; i++) 
    {
        if (pipe(pipes[i]) == -1) 
        {
            perror("Pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    // Fork processes for each command (in reverse order)
    pid_t pids[MAX_CMDS];
    for (int i = numcmds - 1; i >= 0; i--) 
    {
        pids[i] = fork();
        if (pids[i] == 0) 
        {
            // Child process

            // Redirect input if not the last command
            if (i < numcmds - 1) 
            {
                dup2(pipes[i][0], STDIN_FILENO);
            }

            // Redirect output if not the first command
            if (i > 0) 
            {
                dup2(pipes[i - 1][1], STDOUT_FILENO);
            }

            // Close all pipe file descriptors
            for (int j = 0; j < numcmds - 1; j++) 
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            if (execvp(commands[i][0], commands[i]) == -1) 
            {
                perror("Error executing command");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Close all pipe file descriptors in the parent process
    for (int i = 0; i < numcmds - 1; i++) 
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < numcmds; i++) 
    {
        waitpid(pids[i], NULL, 0);
    }
}


// Executes a single command using fork() and execvp().
void execute_command(char **args) 
{
    pid_t pid = fork();
    if (pid == 0) 
    {
        // Child process
        execvp(args[0], args);
        perror("execvp"); // If execvp fails
        exit(1);
    } 
    else if (pid > 0) 
    {
        // Parent process
        wait(NULL); // Wait for the child to finish
    } 
    else 
    {
        perror("fork"); // If fork fails
    }
}

// option 3
// Appends the contents of file2 to file1 and vice versa.
void append_files(char *file1, char *file2) 
{
    FILE *f1 = fopen(file1, "a");   // First file
    FILE *f2 = fopen(file2, "a");   // Second file
    if (!f1 || !f2)                 // Check if file exists or not
    {
        perror("Error opening files");
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return;
    }

    // Append file2 to file1
    char ch;
    FILE *temp = fopen(file2, "r");
    if (temp) 
    {
        while ((ch = fgetc(temp)) != EOF) 
        {
            fputc(ch, f1);
        }
        fclose(temp);
    } 
    else 
    {
        perror("Error opening file2 for reading");
    }

    // Append file1 to file2
    temp = fopen(file1, "r");
    if (temp) 
    {
        while ((ch = fgetc(temp)) != EOF) 
        {
            fputc(ch, f2);
        }
        fclose(temp);
    } 
    else 
    {
        perror("Error opening file1 for reading");
    }

    fclose(f1);
    fclose(f2);
    printf("Appended contents of %s to %s and vice versa.\n", file2, file1);
}

// option 4
// Count the number of words in a file
void count_words(char *filename) 
{
    FILE *file = fopen(filename, "r");
    if (!file) 
    {
        perror("Error opening file");
        return;
    }

    int word_count = 0;     // Counter to store number of words 
    char ch;
    int in_word = 0;

    while ((ch = fgetc(file)) != EOF) 
    {
        if (ch == ' ' || ch == '\n' || ch == '\t') {
            if (in_word) {
                word_count++;
                in_word = 0;
            }
        } else {
            in_word = 1;
        }
    }

    // Count the last word if the file doesn't end with a space/newline
    if (in_word) 
    {
        word_count++;
    }

    fclose(file);
    printf("Number of words in %s: %d\n", filename, word_count);
}

// Option 5
// Concatenates multiple files and prints the result to stdout
void concatenate_files(char **files, int num_files)
{
    // First, check if all files exist
    for (int i = 0; i < num_files; i++) 
    {
        if (access(files[i], F_OK) != 0) 
        {
            // File does not exist
            fprintf(stderr, "Error: File '%s' does not exist.\n", files[i]);
            return;  // Abort the operation
        }
    }

    // If all files exist, proceed with concatenation
    // Loop through each file in the list
    for (int i = 0; i < num_files; i++) 
    {
        // Open the file in read mode
        FILE *file = fopen(files[i], "r");

        // Check if the file was successfully opened
        if (!file) 
        {
            perror("Error opening file");
            return;  // Abort the operation if any file cannot be opened
        }

        // Read the file character by character and print to stdout
        char ch;
        while ((ch = fgetc(file)) != EOF) 
        {
            putchar(ch);
        }

        // Close the file
        fclose(file);
    }
}

// Function to check for redirection operators and preprocess the input
void preprocess_input(char *input) 
{
    char *redir_pos;

    // Handle input redirection "<"
    if ((redir_pos = strstr(input, "<")) != NULL) 
    {
        // Move the operator to its own space if no space before
        if (redir_pos != input && *(redir_pos - 1) != ' ') 
        {
            memmove(redir_pos + 1, redir_pos, strlen(redir_pos) + 1);
            *redir_pos = ' ';  // Replace < with a space
        }
        // Check if there is no space after the operator
        if (redir_pos[1] != ' ' && redir_pos[1] != '\0') 
        {
            memmove(redir_pos + 2, redir_pos + 1, strlen(redir_pos + 1) + 1);
            redir_pos[1] = ' ';
        }
    }

    // Handle output redirection ">" or ">>"
    if ((redir_pos = strstr(input, ">")) != NULL) 
    {
        // Check for ">>"
        if (redir_pos[1] == '>') 
        {
            // Ensure space before ">>"
            if (redir_pos > input && redir_pos[-1] != ' ') 
            {
                memmove(redir_pos + 1, redir_pos, strlen(redir_pos) + 1);
                *redir_pos = ' ';
                redir_pos++;  // Move pointer forward to keep ">>" intact
            }
            // Ensure space after ">>"
            if (redir_pos[2] != ' ' && redir_pos[2] != '\0') 
            {
                memmove(redir_pos + 3, redir_pos + 2, strlen(redir_pos + 2) + 1);
                redir_pos[2] = ' ';
            }
        } 
        // Handle single ">"
        else 
        {
            // Ensure space before ">"
            if (redir_pos > input && redir_pos[-1] != ' ') 
            {
                memmove(redir_pos + 1, redir_pos, strlen(redir_pos) + 1);
                *redir_pos = ' ';
            }
            // Ensure space after ">"
            if (redir_pos[1] != ' ' && redir_pos[1] != '\0') 
            {
                memmove(redir_pos + 2, redir_pos + 1, strlen(redir_pos + 1) + 1);
                redir_pos[1] = ' ';
            }
        }
    }
}

// Function to remove the leading spaces from a string
void trim_leading_spaces(char *str) 
{
    int index = 0, i = 0;

    // Find first non-space character
    while (str[index] == ' ') 
    {
        index++;
    }

    // Shift all characters left
    while (str[index]) 
    {
        str[i++] = str[index++];
    }
    
    // Null terminate the string
    str[i] = '\0';
}

// Option 6
// Function to handle input/output redirection
void execute_redirection(char *command) 
{
    preprocess_input(command);  // Preprocess the input to add spaces before and after > or >>

    char input_file[MAX_FILENAME] = {0}; // Array for input file
    char output_file[MAX_FILENAME] = {0}; // Array for output file
    char append_file[MAX_FILENAME] = {0}; // Array for append file
    int ARGS = 256;
    char *args[ARGS + 1];
    int argc = 0;

    // Temporary copy of the command for parsing
    char cmd_copy[256];
    strncpy(cmd_copy, command, sizeof(cmd_copy));
    cmd_copy[sizeof(cmd_copy) - 1] = '\0'; // Ensure null-termination

    // Parse the command to detect redirection symbols
    char *token = cmd_copy;
    while (*token != '\0' && argc < ARGS) 
    {
        // Skip leading spaces
        while (*token == ' ') 
        {
            token++;
        }

        // Check for redirection symbols
        if (*token == '<' || *token == '>') 
        {
            // Handle redirection symbol
            char *symbol = token;
            token++; // Move past the redirection symbol

            // Handle double '>' for append
            if (*symbol == '>' && *token == '>') 
            {
                token++; // Move past the second '>'
                // Copy the append filename into the array
                char *filename_start = token;
                while (*token != '\0') 
                {
                    token++;
                }
                if (*token != '\0') 
                {
                    *token = '\0'; // Terminate the filename
                    token++;
                }
                strncpy(append_file, filename_start, sizeof(append_file) - 1);
                append_file[sizeof(append_file) - 1] = '\0'; // Ensure null-termination
        
            } 
            else if (*symbol == '<') 
            {
                // Copy the input filename into the array
                char *filename_start = token;
                while (*token != '\0') 
                {
                    token++;
                }
                if (*token != '\0') 
                {
                    *token = '\0'; // Terminate the filename
                    token++;
                }
                strncpy(input_file, filename_start, sizeof(input_file) - 1);
                input_file[sizeof(input_file) - 1] = '\0'; // Ensure null-termination
              
            } 
            else if (*symbol == '>') 
            {
                // Copy the output filename into the array
                char *filename_start = token;
                while (*token != '\0') 
                {
                    token++;
                }
                if (*token != '\0') 
                {
                    *token = '\0'; // Terminate the filename
                    token++;
                }
                strncpy(output_file, filename_start, sizeof(output_file) - 1);
                output_file[sizeof(output_file) - 1] = '\0'; // Ensure null-termination
            }
        } 
        else 
        {
            // Regular argument
            args[argc++] = token;

            // Skip to the end of the argument
            while (*token != '\0' && *token != ' ' && *token != '<' && *token != '>') 
            {
                token++;
            }
            if (*token != '\0') 
            {
                *token = '\0'; // Terminate the argument
                token++;
            }
        }
    }
    args[argc] = NULL; // Null-terminate the argument list

    // Debug: Print the final argument list
    /*for (int i = 0; i < argc; i++) {
        printf("  args[%d]: %s\n", i, args[i]);
    }*/

    // Execute the command with redirection
    pid_t pid = fork();
    if (pid == 0)       // Child process
    {
        if (input_file[0] != '\0') 
        {
            trim_leading_spaces(input_file);    // Remove the leading spaces from file name
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) 
            {
                perror("Error opening input file");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (output_file[0] != '\0') 
        {
            trim_leading_spaces(output_file);   // Remove the leading spaces from file name
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) 
            {
                perror("Error opening output file");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (append_file[0] != '\0') 
        {
            trim_leading_spaces(append_file);   // Remove the leading spaces from file name
            int fd = open(append_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) 
            {
                perror("Error opening append file");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Execute the command
        execvp(args[0], args);
        perror("Error executing command");
        exit(1);
    } 
    else if (pid > 0)    // Parent process
    {
        wait(NULL);
    } 
    else 
    {
        perror("fork has failed");
    }
}

// Option 7
// Handle command chaining: Executes multiple commands sequentially
void execute_command_chain(char *command) 
{
    char *commands[MAX_ARGS + 1];   // Array to store individual commands separated by ';'
    int num_commands = 0;

    // Tokenize the input string using ';' as a delimiter
    char *token = strtok(command, ";");
    while (token != NULL && num_commands <= MAX_ARGS) 
    {
        commands[num_commands++] = token;
        token = strtok(NULL, ";");
    }

    // Support for up to 4 commands, then ; operator should be up to 3
    if (num_commands < 1 || num_commands > 4)   
    {
        printf("Error: Number of commands must be from 1 to 4\n");
        return;
    }

    // Execute all commands 
    for (int i = 0; i < num_commands; i++) 
    {
        // Tokenize each command
        char *args[MAX_ARGS + 1];
        int argc = 0;
        char *cmd_token = strtok(commands[i], " ");

        while (cmd_token != NULL) 
        {
             if (argc >= MAX_ARGS) // If any command exceeds MAX_ARGS, stop execution
            {
                fprintf(stderr, "Error: Command %d has invalid number of arguments (must be >= 1 and <= %d).\n", i + 1, MAX_ARGS);
                return;
            }
            args[argc++] = cmd_token;
            cmd_token = strtok(NULL, " ");
        }
        args[argc] = NULL;  // Null-terminate the argument list

        execute_command(args);  // Execute the parsed command
    }
}


// Function to execute a single command, It has return value
int execute_command1(char *args[]) {
    pid_t pid = fork();

    if (pid == 0) 
    {
        // Child process: execute the command
        execvp(args[0], args);
        perror("execvp"); // If execvp fails
        exit(EXIT_FAILURE);
    } 
    else if (pid > 0) 
    {
        // Parent process: wait for the child to finish
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1; // Return exit status
    } 
    else 
    {
        perror("fork"); // If fork fails
        return -1;
    }
}

// Option 8 and 9
// Handles conditional execution of commands using && and ||.
void execute_conditional_execution(char *input) 
{
    char *commands[MAX_CMDS];
    char *operators[MAX_ARGS];
    int command_count = 0;
    int operator_count = 0;

    // Split the input into commands and operators
    char *token = strtok(input, " ");
    while (token != NULL) 
    {
        if (strcmp(token, "&&") == 0 || strcmp(token, "||") == 0) 
        {
            if (operator_count >= MAX_ARGS) 
            {
                fprintf(stderr, "Error: Too many operators (maximum %d allowed).\n", MAX_ARGS);
                return;
            }
            operators[operator_count++] = token; // Save the operator
            token = strtok(NULL, " "); // Move to the next token
        } 
        else 
        {
            if (command_count >= MAX_CMDS) 
            {
                fprintf(stderr, "Error: Too many commands (maximum %d allowed).\n", MAX_CMDS);
                return;
            }
            // Save the command (combine multiple tokens into a single command)
            commands[command_count] = token;
            while ((token = strtok(NULL, " ")) != NULL && strcmp(token, "&&") != 0 && strcmp(token, "||") != 0) 
            {
                // Append the token to the current command
                char *temp = malloc(strlen(commands[command_count]) + strlen(token) + 2); // +2 for space and null terminator
                sprintf(temp, "%s %s", commands[command_count], token);
                commands[command_count] = temp;
            }
            command_count++;
            if (token == NULL) break; // No more tokens
        }
    }

    //printf("operator count:%d\n", operator_count);

    // Number of commands allowed is 6 and number of conditional operator allowed is 5
    if (command_count > MAX_CMDS) 
    {
        fprintf(stderr, "Error: up to %d conditional execution operators allowed.\n", MAX_CMDS - 1);
        return;
    }

    //printf("Command count:%d\n", command_count);
    // Validate the number of commands and operators
    if (operator_count != command_count - 1) 
    {
        fprintf(stderr, "Error: Invalid number of operators.\n");
        return;
    }

    // Execute the commands based on the operators
    int status = 0; // Initial status (assume success)
    for (int i = 0; i < command_count; i++) 
    {
        // Split the command into arguments
        char *args[MAX_ARGS + 1];
        int argc = 0;
        char *cmd_copy = strdup(commands[i]); // Create a copy to avoid modifying the original
        char *cmd_token = strtok(cmd_copy, " ");
        while (cmd_token != NULL) 
        {
            args[argc++] = cmd_token;
            cmd_token = strtok(NULL, " ");
        }
        args[argc] = NULL; // Null-terminate the argument list

        // Validate the number of arguments
        if (argc < 1 || argc > MAX_ARGS) 
        {
            fprintf(stderr, "Error: Command '%s' has %d arguments (must be >= 1 and <= %d).\n", commands[i], argc, MAX_ARGS);
            free(cmd_copy);
            return;
        }

        // Execute the command based on the operator
        if (i == 0 || (i > 0 && strcmp(operators[i - 1], "&&") == 0 && status == 0) || (i > 0 && strcmp(operators[i - 1], "||") == 0 && status != 0)) 
        {
            status = execute_command1(args);    // Execute the parsed command
        }

        free(cmd_copy); // Free the copied command string
    }
}

// Main function: Entry point of the shell program.
int main(int argc, char **argv)
{
    if( argc != 1)      // Only program name i.e. executable is allowed to run
    {
        printf("Only one argument is allowed.\n");
        exit(1);
    }

    // Display the welcome message
    printf("Welcome to w25shell! Type 'killterm' to quit.\n");
    
    char input[BUFFER_SIZE];        // Array to store the user input
    
    // Infinite loop
    while (1)           
    {
        printf("w25shell$ ");   // Print w25shell$
        fflush(stdout);
        
        // Get the input from user and store it to input array
        if (fgets(input, BUFFER_SIZE, stdin) == NULL)   
            break;
        
        input[strcspn(input, "\n")] = '\0';     // Trim newline

        // Handle special commands
        if (strcmp(input, "killterm") == 0)     // Rule 1
        {
            // Kill current terminal
            kill_terminal();
        } 
        else if (strcmp(input, "killallterms") == 0)    // Rule 2
        {
            // Kill all the terminal
            kill_all_terminals();
            return 0;
        } 
        else if (strstr(input, "&&") != NULL || strstr(input, "||") != NULL)        // Option 8 and Option 9
        {
            // Handle conditional execution
            execute_conditional_execution(input);
        } 
        else if (strstr(input, "|") != NULL)    // Option 1
        {
            //printf("Execute piping:\n");
            execute_piped_commands(input);
        } 
        else if (strstr(input, "=") != NULL)    // Option 2
        {
            execute_reverse_piped_commands(input);
        } 
        else if (strstr(input, "~") != NULL)    // Option 3
        {
            // Handle append operation
            char *file1 = strtok(input, " ~");
            char *file2 = strtok(NULL, " ~");
            if (file1 && file2) 
            {
                append_files(file1, file2);
            } 
            else 
            {
                printf("Error: Invalid append operation. Usage: file1 ~ file2\n");
            }
        } 
        else if (input[0] == '#')               // Option 4
        {
            // Handle word count operation
            char *filename = strtok(input + 1, " ");
            if (filename) 
            {
                count_words(filename);
            } 
            else 
            {
                printf("Error: Invalid word count operation. Usage: # filename\n");
            }
        }
        else if (strstr(input, "+") != NULL)        // Option 5
        {
            // Handle file concatenation
            char *files[MAX_FILES + 1];
            int num_files = 0;
            char *token = strtok(input, " +");
            while (token != NULL && num_files <= MAX_FILES) 
            {
                files[num_files++] = token;
                token = strtok(NULL, " +");
            }
            if (num_files >= 2 && num_files <= MAX_FILES) 
            {
                concatenate_files(files, num_files);
            } 
            else 
            {
                printf("Error: File concatenation requires 2 to %d files.\n", MAX_FILES);
            }
        } 
        else if (strstr(input, "<") != NULL || strstr(input, ">") != NULL || strstr(input, ">>") != NULL)       // Option 6
        {
            // Handle input/output redirection
            execute_redirection(input);
        } 
        else if (strstr(input, ";") != NULL)        // Option 7
        {
            // Handle command chaining
            execute_command_chain(input);
        } 
        else 
        {
            // This section handles the simple commands which does not have any special characters like |, + or ~ etc.
            // If user have not entered anything
            if(argc == 1 && input[0] == '\0')
            {
                //printf("No input detected. Try commands like 'ls', 'pwd', or 'killterm' to exit.\n");
                continue;    // Move to the next iteration in the loop
            }

             // Tokenize the input command
            char *args[MAX_ARGS + 1];
            int argc = 0;
            char *token = strtok(input, " ");
            while (token != NULL) 
            {
                args[argc++] = token;
                token = strtok(NULL, " ");
            }
            args[argc] = NULL;

            //printf("argc = %d\n", argc);


            if (argc < 1 || argc > MAX_ARGS)        // Rule 3
            {
                printf("Error: number of argument must be >= 1 and <= %d\n", MAX_ARGS);
                continue;
            }

            execute_command(args);      // Execute the parsed command
        }

    }

    return 0;
}