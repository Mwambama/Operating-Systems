
//    mwambama
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>  
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>      // For checking waitpid errors




// structure to hold information about a running background job
typedef struct Job {
    pid_t pid;
    char name[1024];
    struct Job *next;
} Job;

// Global head for linked list
Job *job_list_head = NULL;



bool treat_builtin_commands(char *command);
bool treat_program_commands(char *command);
void change_working_directory(char *directory);
void print_working_directory();
void remove_job(pid_t pid);
void add_job(pid_t pid, char *name);



int main(int argc, char **argv)
{
    char *prompt = "308sh> ";
    if (argc == 3 && strcmp(argv[1], "-p") == 0){
        prompt = argv[2];
    }
    
    char command[1024];
    
    while (1)
    {
        // PERIODIC BACKGROUND PROCESS CHECK 
        int status;
        pid_t finished; 
        
        while ((finished = waitpid(-1, &status, WNOHANG)) > 0) {
            
            Job *reaped_job = NULL; 
            
            // Find the job entry to retrieve its name for the exit message
            for (Job *j = job_list_head; j != NULL; j = j->next) {
                if (j->pid == finished) {
                    reaped_job = j;
                    break;
                }
            }

            if (reaped_job) {
                if (WIFEXITED(status)) {
                    printf("[%d] %s Exit %d\n", finished, reaped_job->name, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    // Handles terminations by signal (like 'kill')
                    printf("[%d] %s Killed (%d)\n", finished, reaped_job->name, WTERMSIG(status));
                } else {
                    printf("[%d] %s did not exit normally\n", finished, reaped_job->name);
                }
                
                // Remove the job from the active tracking list
                remove_job(finished); 
            } else {
                
                 printf("Background process %d exited (untracked)\n", finished);
            }
        }
        
        if (finished == -1 && errno != ECHILD) { 
            perror("waitpid error");
        } // done background check

        
        printf("%s", prompt);
        fflush(stdout);
        

        
        if (fgets(command, 1024, stdin) == NULL){
            break; 
        }
        
        // Remove trailing newline character
        command[strcspn(command, "\n")] = 0;
        
        if (strcmp(command, "exit") == 0){  // shell must terminate if the user requests to exit
            break; 
        }
        
        bool is_program;
        bool is_builtin = treat_builtin_commands(command);
        
        if (!is_builtin){   
            is_program = treat_program_commands(command);
        }
        
        if (!is_builtin && !is_program){    // requested command is not found and cannot be run
            printf("Command not recognized or cannot be run.\n");
        }
        
    } 
    
    return 0;
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
    else if (strcmp(command, "jobs") == 0) {
        Job *current = job_list_head;
        
        if (current == NULL) {
            printf("No background jobs running.\n");
        } else {
            printf("Active Background Jobs:\n");
            // Traverse the list and print the required info
            while (current != NULL) {
                // Requirement: Output names and PID of your child processes (i.e. “process pid (procnam)”)
                printf("process %d (%s)\n", current->pid, current->name); 
                current = current->next;
            }
        }
        return true;
    }    // logic for handling and lsiting background jobs 

    else {
        return false;
    }
}




bool treat_program_commands(char* command) {

    char *args[10];  // hold command and its argue up to 10 token
    bool is_background = false;
    pid_t pid;
    
    
    
    // Checking background suffix (&)
    if (strlen(command) > 0 && command[strlen(command) - 1] == '&') { 
        is_background = true;

        command[strlen(command) - 1] = '\0'; 
    }
    
    int i = 0;
    char *token = strtok(command, " \t\r\n"); // Split by space, tab, CR, LF
    while (token != NULL) {
        if (i < 9) { 
            args[i] = token;
            i++;
        }
        token = strtok(NULL, " \t\r\n");
    }
    args[i] = NULL; 
    
    // Handling empty command after parsing 
    if (args[0] == NULL) {
        return true; 
    }

    // --PROCESS CREATION ---
    pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        return false;
    } 
    
    // --- CHILD PROCESS EXECUTION ---
    else if (pid == 0) { 
        
        if (!is_background) {
            printf("[%d] %s\n", getpid(), args[0]); 
        } else {
            
            printf("[%d] %s\n", getpid(), args[0]); // For background tasks
        }
        fflush(stdout);
        
        // execvp = excution, changes  the child's image with the external program.
        if (execvp(args[0], args) == -1) {
            // execvp() only returns if it FAILED 
            
            fprintf(stderr, "Cannot exec %s: No such file or directory\n", args[0]);
            
            exit(255); // Child erminate right away  with status 255
        }
        
        exit(EXIT_FAILURE); 
    }
   
    else { // PARENT PROCESS (Shell)
        
        if (is_background) {
            // Background command - shell prints the PID and RETURNS right away.
            printf("Background process started with pid: %d\n", pid);

                // Add the process to the global linked list when successfully launched
                 add_job(pid, args[0]);   //Extra credit:

            return true;
        }
        
        // Foreground command - shell BLOCKS (waits for the child).
        int status;
        // waitpid(pid, &status, 0) blocks until the specific child (pid) terminates.
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            return false;
        }

        // Prints the exit status of the terminated foreground child.
        if (WIFEXITED(status)) {
            printf("[%d] %s Exit %d\n", pid, args[0], WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
             // Handle termination by signal (like kill)
             printf("[%d] %s Killed (%d)\n", pid, args[0], WTERMSIG(status));
        } else {
             printf("[%d] %s did not exit normally\n", pid, args[0]);
        }
    }
    
    return true;
}

void change_working_directory(char *directory){
    if (directory == NULL){
        directory = getenv("HOME");
    }
    if (chdir(directory) != 0){
        perror("chdir() error"); // for errors
    }
}
void print_working_directory(){
    char pwd[1024];
    if (getcwd(pwd, sizeof(pwd)) != NULL) {
        printf("%s\n", pwd);
    } else {
        perror("getcwd() error"); // for errors
    }
}

          //EXTRA CREDIT

// --- HELPER FUNCTION: Add Job ---
void add_job(pid_t pid, char *name) {
    Job *new_job = (Job *)malloc(sizeof(Job));
    if (new_job == NULL) {
        perror("malloc failed in add_job");
        return;
    }
    new_job->pid = pid;
    strncpy(new_job->name, name, sizeof(new_job->name) - 1);
    new_job->name[sizeof(new_job->name) - 1] = '\0';
    
    // Add the new job to the beginning of the list
    new_job->next = job_list_head;
    job_list_head = new_job;
}
              
          // HELPER FUNCTION: Remove Job 

void remove_job(pid_t pid) {
    Job *current = job_list_head;
    Job *prev = NULL;

    // Traverse the list to find the matching PID
    while (current != NULL && current->pid != pid) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        return;
    }
    
    if (prev == NULL) {
      
        job_list_head = current->next;
    } else {
        
        prev->next = current->next;
    }
    
    free(current);
}
