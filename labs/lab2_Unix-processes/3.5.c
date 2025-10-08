#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>    // for fork()
#include <sys/wait.h>  // for wait()

int main() {
    int i = 0, j = 0;
    pid_t ret;

    ret = fork();

    if (ret == 0) { // Child process
        printf("Child starts\n");
        fflush(stdout); // Ensure output is flushed
        for (i = 0; i < 500000; i++) {
            printf("Child: %d\n", i);
        }
        printf("Child ends\n");
        fflush(stdout);
    } else if (ret > 0) { // Parent process
        wait(NULL); // Wait for child to finish
        printf("Parent starts\n");
        fflush(stdout);
        for (j = 0; j < 500000; j++) {
            printf("Parent: %d\n", j);
        }
        printf("Parent ends\n");
        fflush(stdout);
    } else {
        // fork failed
        perror("fork failed");
        exit(1);
    }

    return 0;
}

