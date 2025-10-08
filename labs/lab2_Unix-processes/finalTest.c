


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

int main() {
    pid_t pid;

    printf("=== 1. Process creation and wait() ===\n");
    pid = fork();
    if (pid == 0) { // Child
        printf("Child: Hello from child\n");
        sleep(1);
        printf("Child: Exiting\n");
        exit(42); // Return value for parent
    } else {
        wait(NULL);
        printf("Parent: Child finished, continuing\n\n");
    }

    printf("=== 2. Signal using kill() ===\n");
    pid = fork();
    if (pid == 0) { // Child
        int count = 0;
        while (1) {
            printf("Child count %d\n", count++);
            fflush(stdout);
            usleep(50000); // 0.05 sec
        }
    } else {
        sleep(1); // Let child run a few iterations
        kill(pid, SIGTERM);
        wait(NULL);
        printf("Parent: Child killed\n\n");
    }

    printf("=== 3. Exec demonstration ===\n");
    pid = fork();
    if (pid == 0) { // Child
        execl("/bin/ls", "ls", NULL);
        perror("execl failed");
        exit(1);
    } else {
        wait(NULL);
        printf("Parent: Exec child finished\n\n");
    }

    printf("=== 4. Child exit status demonstration ===\n");
    pid = fork();
    if (pid == 0) { // Child
        srand(time(NULL));
        if (rand() % 2 == 0) {
            kill(getpid(), SIGTERM);
        }
        exit(rand() % 256);
    } else {
        int status;
        wait(&status);
        if (WIFEXITED(status)) {
            printf("Child exited normally with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child killed by signal %d\n", WTERMSIG(status));
        }
    }

    return 0;
}

