#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

struct Line {
    int start;
    int len;
};

struct Line table[200];
int file_r, n, line_start = 0, line_count = 0;

void timeout_handler(int sig)
{
    char *buffer;

    printf("\nclozed\n");

    for (int i = 0; i < line_count; i++) {
        buffer = malloc(table[i].len + 1);

        lseek(file_r, table[i].start, SEEK_SET);

        int n = read(file_r, buffer, table[i].len);
        buffer[n] = '\0';

        printf("%s", buffer);

        free(buffer);
    }

    close(file_r);
    exit(0);
}

int main(int argc, char *argv[])
{
    file_r = open(argv[1], O_RDONLY);

    table[0].start = 0;

    char c;
    while ((n = read(file_r, &c, 1)) > 0) {

        if (c == '\n') {
            int current_position = lseek(file_r, 0, 1);

            table[line_count].len = current_position - line_start;

            line_count += 1;

            line_start = current_position;

            table[line_count].start = line_start;
        }
    }

    int end = lseek(file_r, 0L, 1);
    table[line_count].len = end - line_start;
    line_count += 1;
    
    printf("\nresult:\n");
    printf("Line\nstart\tlen\n");

    for (int i = 0; i < line_count; i++) {
        printf("%d\t%d\t%d\n", i + 1, table[i].start, table[i].len);
    }

    signal(SIGALRM, timeout_handler);
    while (1) {
        alarm(5);
        int line_number;
        printf("\ninput: ");
        scanf("%d", &line_number);

        if (line_number == 0) {
            break;
        }

        int len = table[line_number - 1].len;

        char *buffer = malloc(len + 1);

        lseek(file_r, table[line_number - 1].start, SEEK_SET);
        int bytes_read = read(file_r, buffer, len);

        buffer[bytes_read] = '\0';

        printf("%s", buffer);
    }

    close(file_r);

    return 0;
}