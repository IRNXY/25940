#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int main(int argc, char *argv[]) {
    int sock_fd;
    struct sockaddr_un addr;
    struct timespec delay, start, end;
    const char *message;

    if (argc != 2) {
        fprintf(stderr, "Использование: %s A или B\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "A") == 0) {
        message = "AAaaaaAAaAAaaa\n";
    } else if (strcmp(argv[1], "B") == 0) {
        message = "BBBbbbBbbbbbB\n";
    } else {
        fprintf(stderr, "Укажите A или B\n");
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

    signal(SIGPIPE, SIG_IGN);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    printf("Клиент %s подключился!\n", argv[1]);

    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
        perror("clock_gettime");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 50; i++) {
        if (write(sock_fd, message, strlen(message)) < 0) {
            perror("write");
            close(sock_fd);
            return EXIT_FAILURE;
        }

        if (i < 49) {
            delay.tv_sec = 0;
            delay.tv_nsec = (rand() % 50 + 1) * 1000L;
            nanosleep(&delay, NULL);
        }
    }

    close(sock_fd);

    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
        perror("clock_gettime");
        return EXIT_FAILURE;
    }

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("\nКлиент %s завершил работу за %.6f секунд\n", argv[1], elapsed);

    return EXIT_SUCCESS;
}
