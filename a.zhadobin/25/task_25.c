
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFFER_SIZE 1000

void print_help(const char *program_name)
{
    printf("Использование: %s [опция]\n", program_name);
    printf("Программа для обмена данными между процессами через pipe.\n\n");
    printf("Опции:\n");
    printf("  -h, --help  показать эту справку и завершить работу.\n\n");
    printf("Работа программы:\n");
    printf("  Родительский процесс отправляет текст в канал.\n");
    printf("  Дочерний процесс переводит текст в верхний регистр.\n");
    printf("  Пустая строка или Ctrl+D завершает ввод.\n");
}

int write_all(int fd, const char *buffer, size_t size)
{
    size_t written = 0;

    while (written < size) {
        ssize_t result = write(fd, buffer + written, size - written);

        if (result == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        written += (size_t)result;
    }

    return 0;
}


void child_process(int read_fd)
{
    char buffer[BUFFER_SIZE];
    char line[BUFFER_SIZE];
    size_t position = 0;
    int escape_state = 0;

    printf("[CHILD] Запущен. Ожидаю данные от родителя...\n");
    fflush(stdout);

    while (1) {
        ssize_t bytes_read = read(read_fd, buffer, sizeof(buffer));

        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("Ошибка чтения из канала");
            break;
        }

        if (bytes_read == 0) {
            break;
        }

        for (ssize_t i = 0; i < bytes_read; i++) {

            unsigned char ch = (unsigned char)buffer[i];

            if (escape_state == 1) {
                if (ch == '[') {
                    escape_state = 2;
                } else if (ch == 'O') {
                    escape_state = 3;
                } else {
                    escape_state = 0;
                }
                continue;
            }

            if (escape_state == 2) {
                if (ch >= 0x40 && ch <= 0x7E) {
                    escape_state = 0;
                }
                continue;
            }

            if (escape_state == 3) {
                escape_state = 0;
                continue;
            }

            if (ch == 27) {
                escape_state = 1;
                continue;
            }

            if ((ch < 32 && ch != '\n' && ch != '\t') || ch == 127) {
                continue;
            }

            line[position++] = (char)toupper(ch);

            if (ch == '\n' || position == sizeof(line)) {
                printf("[CHILD] Получено и преобразовано: ");
                fwrite(line, 1, position, stdout);
                fflush(stdout);

                position = 0;
            }
        }
    }

    if (position > 0) {
        printf("[CHILD] Получено и преобразовано: ");
        fwrite(line, 1, position, stdout);
        printf("\n");
        fflush(stdout);
    }

    close(read_fd);
}


int main(int argc, char *argv[]){
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 ||
            strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }

        fprintf(stderr, "Неизвестная опция: %s\n", argv[1]);
        fprintf(stderr, "Используйте -h или --help для справки.\n");
        return 1;
    }
    // 0 - для чтения, 1 - для записи 
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        perror("Ошибка создания канала");
        return 1;
    }

    // дочерний процесс должен унаследовать файловые дескрипторы уже созданного канала
    pid = fork();

    if (pid == -1) {
        perror("Ошибка создания процесса");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (pid == 0) {
        // потомок только читает
        close(pipefd[1]); 

        child_process(pipefd[0]);

        return 0;
    }


    close(pipefd[0]); 

    printf("[PARENT] Связь через канал установлена!\n");
    printf("[PARENT] Введите текст (пустая строка или Ctrl+D — выход):\n");
    fflush(stdout);

    char input[BUFFER_SIZE];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        if (input[0] == '\n') {
            printf("[PARENT] Пустая строка — завершаю ввод.\n");
            break;
        }

        if (write_all(pipefd[1], input, strlen(input)) == -1) {
            perror("Ошибка записи в канал");
            break;
        }
    }

    close(pipefd[1]);

    if (waitpid(pid, NULL, 0) == -1) {
        perror("Ошибка ожидания дочернего процесса");
        return 1;
    }

    printf("[PARENT] Потомок завершился. Родитель тоже завершает работу.\n");

    return 0;
}
