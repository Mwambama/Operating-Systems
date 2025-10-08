#include <stdio.h>
#include <unistd.h>

int main() {
    int i = 0, j = 0, pid;
    pid = fork();

    if (pid == 0) {
    
    // Child process
        printf("Child process started. PID = %d, PPID = %d\n", getpid(), getppid());
        
        // Child process
        for (i = 0; i < 1000000; i++) {
            printf("Child: %d\n", i);
        }
    } else {
    
    // Parent process
        printf("Parent process started. PID = %d, PPID = %d\n", getpid(), getppid());
    
        // Parent process
        for (j = 0; j < 1000000; j++) {
            printf("Parent: %d\n", j);
        }
    }
    return 0;
}

