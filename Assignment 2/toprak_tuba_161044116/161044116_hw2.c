
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_COMMANDS 20
#define MAX_COMMAND_LENGTH 50

volatile sig_atomic_t flag = 1;



void sigint_handler(int signo){
    printf(" Received SIGINT signal, ignored...\n");
    signal(SIGINT,SIG_IGN);
}

void sigterm_handler(int signo){
    printf(" Received SIGTERM signal, ignored...\n");
    signal(SIGTERM,SIG_IGN);
}


void log_commands(int pids[], char* commands[]) {
    char filename[50];
    time_t current_time;
    struct tm* time_info;
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(filename, sizeof(filename), "log_%Y-%m-%d_%H-%M-%S.txt", time_info);
    FILE* logfile = fopen(filename, "w");
    if (logfile == NULL) {
        perror("fopen");
        return;
    }
    for (int i = 0; pids[i] != 0 && commands[i] != NULL; i++) {
        fprintf(logfile, "PID: %d, Command: %s\n", pids[i], commands[i]);
    }
    fclose(logfile);
}


int parse_input(char* input, char** commands) {
    int i = 0;
    char* token = strtok(input, "|");
    while (token != NULL && i < MAX_COMMANDS) {
        // remove leading and trailing whitespaces
        while (*token == ' ') {
            token++;
        }
        char* end = token + strlen(token) - 1;
        while (end > token && *end == ' ') {
            end--;
        }
        end[1] = '\0';
        // copy the trimmed command to the commands array
        commands[i] = strdup(token);
        token = strtok(NULL, "|");
        i++;
    }
    commands[i] = NULL; // set the last element to NULL
    return i;
}

int main() {
    
    char input[MAX_COMMANDS * MAX_COMMAND_LENGTH];
   

    while(flag){

        signal(SIGINT,sigint_handler);
        signal(SIGTERM,sigterm_handler);
        
        printf("myshell> ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // stdin'den bir hata oluştuysa program sonlandırılır
            perror("Error: stdin");
            break;
            //exit(EXIT_FAILURE);
        }
        input[strcspn(input, "\n")] = '\0'; // remove trailing newline

        // Girdi ':q' ise program sonlandırılır
        if (strcmp(input, ":q") == 0) {
            exit(EXIT_SUCCESS);
        }

        char* commands[MAX_COMMANDS + 1];
        int num_commands = parse_input(input, commands);
         int pids[num_commands];


        int pipes[num_commands - 1][2];
        int i;
        for (i = 0; i < num_commands - 1; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                break;
                //exit(EXIT_FAILURE);
            }
        }

        for (i = 0; i < num_commands; i++) {
            
            int pid = fork();
            
            if (pid == -1) {
                perror("fork");
                break;
                //exit(EXIT_FAILURE);
            } else if (pid == 0) { // child process
                if (i == 0) { // first command
                    dup2(pipes[0][1], STDOUT_FILENO);
                    close(pipes[0][0]);
                    close(pipes[0][1]);
                } else if (i == num_commands - 1) { // last command
                    dup2(pipes[i - 1][0], STDIN_FILENO);
                    close(pipes[i - 1][0]);
                    close(pipes[i - 1][1]);
                } else { // middle commands
                    dup2(pipes[i - 1][0], STDIN_FILENO);
                    dup2(pipes[i][1], STDOUT_FILENO);
                    close(pipes[i - 1][0]);
                    close(pipes[i - 1][1]);
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }
                execl("/bin/sh", "sh", "-c", commands[i], NULL);
                perror("execl");
                break;
                //exit(EXIT_FAILURE);
            }
            else {
                pids[i] = pid;
            }
        }

        for (i = 0; i < num_commands - 1; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

        for (i = 0; i < num_commands; i++) {
            wait(NULL);
        }
        
        log_commands(pids,commands);

        for (i = 0; i < num_commands; i++) {
            free(commands[i]);
        }
    }
    return 0;
}