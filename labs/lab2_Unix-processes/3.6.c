#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>     // for fork(), usleep()
#include <sys/wait.h>   // for wait()

int main() {
    int i = 0;
    pid_t child;

    child = fork();

    if (child == 0) { // Child process
        while (1) {
            i++;
            printf("Child at count %d\n", i);
            fflush(stdout);       // Ensure output is printed
            usleep(10000);        // Sleep for 1/100 second
        }
    } else if (child > 0) { // Parent process
        printf("Parent sleeping\n");
        fflush(stdout);
        sleep(10);               // Let child run for 10 seconds
        kill(child, SIGTERM);    // Terminate child
        printf("Child has been killed. Waiting for it...\n");
        fflush(stdout);
        wait(NULL);              // Collect child exit status
        printf("done.\n");
        fflush(stdout);
    } else {
        // fork failed
        perror("fork failed");
        exit(1);
    }

    return 0;
}

