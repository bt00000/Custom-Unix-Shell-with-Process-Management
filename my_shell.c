#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <stdbool.h>


#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

int background;
pid_t foreground_pid = -1;
bool foreground_interrupted = false;

char **tokenize(char *line)
{
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for (i = 0; i < strlen(line); i++) {
        char readChar = line[i];

        if (readChar == ' ' || readChar == '\n' || readChar == '\t') {
            if (tokenIndex != 0) {
                token[tokenIndex] = '\0';
                tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
        } else if (readChar == '&' && line[i+1] == '&' && line[i+2] == '&') {
            if (tokenIndex != 0) {
                token[tokenIndex] = '\0';
                tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
            tokens[tokenNo] = (char*)malloc(4*sizeof(char));
            strncpy(tokens[tokenNo++], "&&&", 3);
            i += 2; // skip the second and third '&' characters
        } else if (readChar == '&' && line[i+1] == '&') {
            if (tokenIndex != 0) {
                token[tokenIndex] = '\0';
                tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
            tokens[tokenNo] = (char*)malloc(3*sizeof(char));
            strncpy(tokens[tokenNo++], "&&", 2);
            i++; // skip the second '&' character
        } else if (readChar == '&') {
            if (tokenIndex != 0) {
                token[tokenIndex] = '\0';
                tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
            tokens[tokenNo] = (char*)malloc(2*sizeof(char));
            strncpy(tokens[tokenNo++], "&", 1);
        } else {
            token[tokenIndex++] = readChar;
        }
    }

    free(token);
    tokens[tokenNo] = NULL;
    return tokens;
}

void handle_sigchld(int sig) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0) {
        if (background || pid != foreground_pid) {
            printf("Background process with PID %d has finished execution.\n", pid);
            kill(getpid(), SIGUSR1);
        }
    }
}

void handle_sigusr1(int sig) {
    printf("$ ");
    fflush(stdout); // flush the output buffer to ensure prompt is printed immediately
}

void handle_sigint(int sig) {
    if (background == 0 && foreground_pid != -1) { // only terminate the foreground process
        kill(foreground_pid, SIGINT);
        foreground_interrupted = true;
    }
    printf("\n$ ");
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    signal(SIGCHLD, handle_sigchld); // registering a signal handler for 'SIGCHLD'
    signal(SIGUSR1, handle_sigusr1); // register a signal handler for 'SIGUSR1'
    signal(SIGINT, handle_sigint);   // register a signal handler for 'SIGINT'


    char  line[MAX_INPUT_SIZE];            
    char  **tokens;              
    // int i;

    FILE* fp;
    if(argc == 2) {
        fp = fopen(argv[1],"r");
        if(fp < 0) {
            printf("File doesn't exists.");
            return -1;
        }
    }

    while(1) {          
        /* BEGIN: TAKING INPUT */
        bzero(line, sizeof(line));
        if (argc == 2) { // batch mode
            if (fgets(line, sizeof(line), fp) == NULL) { // file reading finished
                break;
            }
            line[strlen(line) - 1] = '\0';
        } else { // interactive mode
            if (foreground_interrupted) {
                foreground_interrupted = false;
            } else {
                printf("$ ");
            }
            scanf("%[^\n]", line);
            getchar();
        }
        // printf("Command entered: %s (remove this debug output later)\n", line);
        /* END: TAKING INPUT */

        line[strlen(line)] = '\n'; //terminate with new line
        tokens = tokenize(line);

        if (tokens[0] == NULL) {
            continue;
        }

        if (strcmp(tokens[0], "exit") == 0) {
            // Terminate all background processes before exiting
            kill(0, SIGKILL);
            break;
        }

        if (strcmp(tokens[0], "cd") == 0) {
        if (tokens[1] == NULL) {
            // no directory provided, change to home directory
            chdir(getenv("HOME"));
            continue;
        } else {
            if (chdir(tokens[1]) != 0) {
                perror("cd");
            }
        }
        continue;
        }

    //background processes
        background = 0;
        if (tokens[0] != NULL) {
            int num_tokens = 0;
            while (tokens[num_tokens] != NULL) {
                num_tokens++;
            }
            if (num_tokens > 1 && strcmp(tokens[num_tokens-1], "&") == 0) {
                background = 1;
                tokens[num_tokens-1] = NULL;
            }
        }

    // Sequentially or in parallel execute multiple user commands in the foreground
        if (tokens[0] != NULL) {
            int i = 0;
            int num_commands = 0;
            char ***commands = malloc(MAX_NUM_TOKENS * sizeof(char **));
            memset(commands, 0, MAX_NUM_TOKENS * sizeof(char **)); // initialize commands to NULL
            pid_t pid;

            while (tokens[i] != NULL) {
                int num_tokens = 0;
                char **command_tokens = malloc(MAX_NUM_TOKENS * sizeof(char *));
                
                while (tokens[i] != NULL && strcmp(tokens[i], "&&") != 0 && strcmp(tokens[i], "&&&") != 0) {
                    command_tokens[num_tokens] = tokens[i];
                    num_tokens++;
                    i++;
                }
                if (num_tokens > 0) {
                    command_tokens[num_tokens] = NULL;
                    commands[num_commands] = command_tokens;
                    num_commands++;
                }
                if (tokens[i] != NULL && strcmp(tokens[i], "&&&") == 0) {
                    i++;
                    pid = fork();
                    if (pid == 0) {
                        // In child process
                        for (int j = 0; j < num_commands; j++) {
                            pid_t cmd_pid = fork();
                            if (cmd_pid == 0) {
                                setpgid(0, 0); // create a new process group
                                execvp(commands[j][0], commands[j]);
                                if (execvp(commands[j][0], commands[j]) == -1) {
                                    fprintf(stderr, "Command not found\n");
                                    exit(EXIT_FAILURE);
                                }
                                perror("execvp failed");
                                exit(1);
                            }
                        }
                        // Wait for all child processes to finish
                        for (int j = 0; j < num_commands; j++) {
                            wait(NULL);
                        }
                        exit(0);
                    } else if (pid > 0) {
                        // In parent process, continue to next command group
                        num_commands = 0;
                        continue;
                    } else {
                        perror("fork failed");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (tokens[i] != NULL && strcmp(tokens[i], "&&") == 0) {
                    i++;
                    pid = fork();
                    if (pid == 0) {
                        // In child process
                        setpgid(0, 0); // create a new process group
                        execvp(command_tokens[0], command_tokens);
                        if (execvp(command_tokens[0], command_tokens) == -1) {
                            fprintf(stderr, "Command not found\n");
                            exit(EXIT_FAILURE);
                        }
                        perror("execvp failed");
                        exit(1);
                    } else if (pid > 0) {
                        // In parent process
                        if (background == 0) {
                            foreground_pid = pid;
                            waitpid(pid, NULL, 0);
                            foreground_pid = -1;
                        } else {
                            printf("Process %d running in the background\n", pid);
                        }
                    } else {
                        perror("fork failed");
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    // In sequential mode
                    pid = fork();
                    if (pid == 0) {
                        // In child process
                        setpgid(0, 0); // create a new process group
                        execvp(command_tokens[0], command_tokens);
                        if (execvp(command_tokens[0], command_tokens) == -1) {
                            fprintf(stderr, "Command not found\n");
                            exit(EXIT_FAILURE);
                        }
                        perror("execvp failed");
                        exit(1);
                    } else if (pid > 0) {
                        // In parent process
                        if (background == 0) {
                            foreground_pid = pid;
                            waitpid(pid, NULL, 0);
                            foreground_pid = -1;
                        } else {
                            printf("Process %d running in the background\n", pid);
                        }
                    } else {
                        perror("fork failed");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            // Free commands memory
            for (i = 0; i < num_commands; i++) {
                free(commands[i]);
            }
}


}
    return 0;
}