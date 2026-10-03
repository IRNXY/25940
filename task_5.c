#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

struct Line {
    int start;
    int len;
};

int main(int argc, char *argv[])
{
    int fd, n, line_start = 0;

    struct Line table[200];
    int line_count = 0;

    fd = open(argv[1], O_RDONLY);

    table[0].start = 0;

    char c;
    while ((n = read(fd, &c, 1)) > 0) {

        if (c == '\n') {
            int current_position = lseek(fd, 0, 1);

            table[line_count].len = current_position - line_start;

            line_count += 1;

            line_start = current_position;

            table[line_count].start = line_start;
        }
    }

    int end = lseek(fd, 0L, 1);
    table[line_count].len = end - line_start;
    line_count += 1;
    
    printf("\nresult:\n");
    printf("Line\nstart\tlen\n");

    for (int i = 0; i < line_count; i++) {
        printf("%d\t%d\t%d\n", i + 1, table[i].start, table[i].len);
    }

    while (1) {
        int line_number;
        printf("\ninput: ");
        scanf("%d", &line_number);

        if (line_number == 0) {
            break;
        }

        int len = table[line_number - 1].len;

        char *buffer = malloc(len + 1);

        lseek(fd, table[line_number - 1].start, SEEK_SET);
        int bytes_read = read(fd, buffer, len);

        buffer[bytes_read] = '\0';

        printf("%s", buffer);
    }

    close(fd);

    return 0;
}