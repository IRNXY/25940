#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int main(void) {
    int sock_fd;
    struct sockaddr_un addr;

    const char *messages[] = {
        "Hello World!\n",
        "This is a Test Message!!!\n",
        "Unix domain sockets are awesome.\n"
    };

    int count = 3;

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("Подключено к серверу.\n");

    for (int i = 0; i < count; i++) {
        printf("[CLIENT] Отправляю: %s", messages[i]);

        if (write(sock_fd, messages[i], strlen(messages[i])) < 0) {
            perror("write");
            close(sock_fd);
            return 1;
        }
    }

    printf("[CLIENT] Все сообщения отправлены.\n");
    printf("[CLIENT] Закрытие соединения...\n");

    close(sock_fd);

    return EXIT_SUCCESS;
}
