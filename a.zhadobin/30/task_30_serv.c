
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define BUFFER_SIZE 1024



int main(void) {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    ssize_t n;
    int status = EXIT_SUCCESS;

    if (unlink(SOCKET_PATH) < 0 && errno != ENOENT) {
        perror("unlink");
        return 1;
    }
    // AF_UNIX - локальное соединение, 0 - поток выбирается автоматически
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // заполняем поток нулями
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);


    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    fprintf(stderr, "Сервер запущен. Ожидание клиента...\n");

    do {
        client_fd = accept(server_fd, NULL, NULL);
    } while (client_fd < 0 && errno == EINTR);

    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    fprintf(stderr, "Клиент подключился!\n");

    while (1) {
        n = read(client_fd, buffer, sizeof(buffer));

        if (n == 0)
            break;

        if (n < 0) {
            if (errno == EINTR){
                continue;
            }
            perror("read");
            status = 1;
            break;
        }

        for (ssize_t i = 0; i < n; i++) {
            buffer[i] = (char)toupper((unsigned char)buffer[i]);
        }

        if (write(STDOUT_FILENO, buffer, (size_t)n) < 0) {
            perror("write");
            status = 1;
            break;
        }
    }

    fprintf(stderr, "Клиент отключился. Завершение сервера.\n");

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    return status;
}
