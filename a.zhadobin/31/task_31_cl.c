
#define _POSIX_C_SOURCE 200809L

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
    struct timespec delay;
    const char *message;

    if (argc != 2) {
        fprintf(stderr, "Использование: %s A или B\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "A") == 0) {
        message = "AAAAAAAAAA\n";
        delay.tv_sec = 0;
        delay.tv_nsec = 500000000;
    } else if (strcmp(argv[1], "B") == 0) {
        message = "BBBBBBBBBBBB\n";
        delay.tv_sec = 0;
        delay.tv_nsec = 300000000;
    } else {
        fprintf(stderr, "Укажите A или B\n");
        return EXIT_FAILURE;
    }

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

    for (int i = 0; i < 20; i++) {
        if (write(sock_fd, message, strlen(message)) < 0) {
            perror("write");
            close(sock_fd);
            return EXIT_FAILURE;
        }

        printf("Отправлено: %s", message);
        nanosleep(&delay, NULL);
    }

    printf("Клиент %s отключается...\n", argv[1]);

    close(sock_fd);

    return EXIT_SUCCESS;
}
