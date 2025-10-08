


#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>    // for fork()
#include <time.h>      // for time()

int main(int argc, char** argv) {
    int child, status;

    child = fork();

    if (child == 0) { // Child process
        srand(time(NULL));
        if (rand() < RAND_MAX / 4) {
            // Kill self with a signal ~25% chance
            kill(getpid(), SIGTERM);
        }
        return rand(); // Return random number
    } else if (child > 0) { // Parent process
        wait(&status); // Wait for child to finish

        if (WIFEXITED(status)) {
            printf("Child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child exited with signal %d\n", WTERMSIG(status));
        }
    } else {
        // fork failed
        perror("fork failed");
        exit(1);
    }

    return 0;
}

