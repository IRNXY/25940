#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
    uid_t ruid, euid;
    FILE *file;

    printf("First try:\n");
    printf("Real UID:      %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    file = fopen("zh.txt", "r");

    if (file == NULL) {
        perror("fopen");
    } else {
        printf("File opened successfully\n");
        fclose(file);
    }

    setuid(getuid());

    printf("\nSecond try:\n");
    printf("Real UID:      %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    file = fopen("zh.txt", "r");

    if (file == NULL) {
        perror("fopen");
    } else {
        printf("File opened successfully\n");
        fclose(file);
    }

    return 0;
}