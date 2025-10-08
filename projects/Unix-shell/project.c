#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>  
#include <stdbool.h>
#include <sys/wait.h>

bool treat_builtin_commands(char *command);
bool treat_program_commands(char *command);



int main(int argc, char **argv)
{
    char *prompt = "308sh> ";
    if (argc == 3 && strcmp(argv[1], "-p") == 0){
        prompt = argv[2];
    }
    char command[1024];
    // infinite loop accepting input from the user
    while (1)
    {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(command, 1024, stdin) == NULL){
            break; // empty command
        }
        command[strcspn(command, "\n")] = 0;
        if (strcmp(command, "exit") == 0){
            break; // shell will terminate if the user requests to exit
        }
        bool is_program;
        bool is_builtin = treat_builtin_commands(command);
        if (!is_builtin){
            is_program = treat_program_commands(command);
        }
        if (!is_builtin && !is_program){
            // requested command is not found and cannot be run
            printf("Command not recognized or cannot be run.\n");
        }
        // periodically check for background processes everytime the user enters a command
        int status;
        pid_t finished; // check for processes that have finished
        while ((finished = waitpid(-1, &status, WNOHANG)) > 0){
        if (WIFEXITED(status)){
            printf("Background process %d exited with status %d\n", finished, WEXITSTATUS(status));
        } else {
            printf("Background process %d did not exit normally\n", finished);
        }
        }

    }
}

bool treat_builtin_commands(char *command){
    if (strcmp(command, "pid") == 0){
        printf("%d\n", getpid());
        fflush(stdout);
        return true;
    }
    else if (strcmp(command, "ppid") == 0){
        printf("%d\n", getppid());
        fflush(stdout);
        return true;
    }
    else if (strncmp(command, "cd", 2) == 0){
        change_working_directory(strtok(command + 3, " "));
        return true;
    }
    else if (strcmp(command, "pwd") == 0){
        print_working_directory();
        return true;
    } 
    else {
        return false;
    }
}
bool treat_program_commands(char* command){
    bool is_background = false;
    // check if command should be run in the background
    if (command[strlen(command) - 1] == '&'){ 
        is_background = true;
        command[strlen(command) - 1] = '\0'; // remove the & from the command
    }
    pid_t pid = fork();
    if (pid < 0){
        perror("Fork failed.\n");
    }
    else if (pid == 0){ // child process
        if (!is_background){
            printf("Executing command with pid: %d\n", getpid());
        }
        char *args[10];
        int i = 0;
        args[0] = strtok(command, " ");
        while (args[i] != NULL){
            i++;
            args[i] = strtok(NULL, " ");
        }
        if (execvp(args[0], args) == -1){
            perror("execvp() error");
            return false;
        }
        exit(EXIT_FAILURE);
    }
    else { // parent process
        if (is_background){
            // if its a background command, shell will not wait
            printf("Background process started with pid: %d\n", pid);
            return true;
        }
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)){
            printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
        } else {
            printf("Child process %d did not exit normally\n", pid);
        }
    }
    return true;
}

