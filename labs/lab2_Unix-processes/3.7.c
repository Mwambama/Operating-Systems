
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // for execl()

int main() {
    // Replace current process with /bin/ls
    execl("/bin/ls", "ls", NULL);

    // This line only executes if execl fails
    perror("execl failed");
    printf("What happened?\n");

    return 0;
}

