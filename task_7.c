#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

struct Line {
    int start;
    int len;
};

struct Line table[200];
int file_r, n, line_start = 0, line_count = 0;

char *file_memory;
int file_size;

void finish(int sig)
{
    printf("\nclozed\n");

    for (int i = 0; i < line_count; i++) {
        printf("%.*s", table[i].len, file_memory + table[i].start);
    }

    munmap(file_memory, file_size);
    close(file_r);
    exit(0);
}

int main(int argc, char *argv[])
{
    file_r = open(argv[1], O_RDONLY);

    struct stat st;
    fstat(file_r, &st);
    file_size = st.st_size;

    file_memory = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, file_r, 0);

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

    signal(SIGALRM, finish);
    alarm(5);
    while (1) {

        int line_number;

        printf("\ninput: ");
        scanf("%d", &line_number);

        alarm(0);

        if (line_number == 0) {
            break;
        }

        int len = table[line_number - 1].len;

        printf("%.*s", len, file_memory + table[line_number - 1].start);
        alarm(0);
    }

    munmap(file_memory, file_size);
    close(file_r);

    return 0;
}
