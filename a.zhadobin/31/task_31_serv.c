#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

#define SOCKET_PATH "./socket"
#define BUFFER_SIZE 1024

sig_atomic_t stop_server = 0;

static void handle_signal(int sig) {
    stop_server = 1;
}

int main(void) {
    int server_fd;
    int max_fd;
    struct sockaddr_un addr;
    struct sigaction sa;

    fd_set master_set;
    fd_set read_set;

    char buffer[BUFFER_SIZE];
    unsigned char escape_state[FD_SETSIZE] = {0};

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    if (server_fd >= FD_SETSIZE) {
        fprintf(stderr, "Слишком большой дескриптор\n");
        close(server_fd);
        return 1;
    }

    if (unlink(SOCKET_PATH) < 0 && errno != ENOENT) {
        perror("unlink");
        close(server_fd);
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);

    max_fd = server_fd;

    fprintf(stderr, "Сервер запущен.\n");
    fprintf(stderr, "Ожидание клиентов на %s...\n", SOCKET_PATH);

    while (!stop_server) {
        read_set = master_set;

        int ready = select(max_fd + 1, &read_set, NULL, NULL, NULL);

        if (ready < 0) {
            if (errno == EINTR){
                continue;
            }
            perror("select");
            break;
        }

        for (int fd = 0; fd <= max_fd; fd++) {
            if (!FD_ISSET(fd, &read_set))
                continue;

            if (fd == server_fd) {
                int client_fd = accept(server_fd, NULL, NULL);

                if (client_fd < 0) {
                    if (errno != EINTR)
                        perror("accept");
                    continue;
                }

                FD_SET(client_fd, &master_set);
                escape_state[client_fd] = 0;

                if (client_fd > max_fd)
                    max_fd = client_fd;

                fprintf(stderr, "Клиент подключился (fd=%d)\n", client_fd);
            } else {
                ssize_t n = read(fd, buffer, sizeof(buffer));

                if (n > 0) {
                    ssize_t len = 0;

                    for (ssize_t i = 0; i < n; i++) {
                        unsigned char c = (unsigned char)buffer[i];

                        if (escape_state[fd] == 1) {
                            if (c == '[')
                                escape_state[fd] = 2;
                            else if (c == 'O')
                                escape_state[fd] = 3;
                            else if (c == ']' || c == 'P' || c == '^' || c == '_' || c == 'X')
                                escape_state[fd] = 4;
                            else if (c >= 0x20 && c <= 0x2F)
                                escape_state[fd] = 6;
                            else
                                escape_state[fd] = 0;

                            continue;
                        }

                        if (escape_state[fd] == 2) {
                            if (c >= 0x40 && c <= 0x7E)
                                escape_state[fd] = 0;
                            else if (c == 27)
                                escape_state[fd] = 1;

                            continue;
                        }

                        if (escape_state[fd] == 3) {
                            escape_state[fd] = (c == 27) ? 1 : 0;
                            continue;
                        }

                        if (escape_state[fd] == 4) {
                            if (c == 7)
                                escape_state[fd] = 0;
                            else if (c == 27)
                                escape_state[fd] = 5;

                            continue;
                        }

                        if (escape_state[fd] == 5) {
                            if (c == '\\' || c == 7)
                                escape_state[fd] = 0;
                            else if (c != 27)
                                escape_state[fd] = 4;

                            continue;
                        }

                        if (escape_state[fd] == 6) {
                            if (c >= 0x30 && c <= 0x7E)
                                escape_state[fd] = 0;
                            else if (c == 27)
                                escape_state[fd] = 1;

                            continue;
                        }

                        if (c == 27) {
                            escape_state[fd] = 1;
                            continue;
                        }

                        if ((c < 32 && c != '\n') || c == 127)
                            continue;

                        buffer[len++] = (char)toupper(c);
                    }

                    if (len > 0) {
                        if (write(STDOUT_FILENO, buffer, (size_t)len) < 0) {
                            perror("write");
                            stop_server = 1;
                            break;
                        }
                    }
                } else if (n == 0) {
                    fprintf(stderr, "Клиент отключился (fd=%d)\n", fd);

                    close(fd);
                    FD_CLR(fd, &master_set);
                    escape_state[fd] = 0;
                } else if (errno != EINTR) {
                    perror("read");

                    close(fd);
                    FD_CLR(fd, &master_set);
                    escape_state[fd] = 0;
                }
            }
        }

        while (max_fd >= 0 && !FD_ISSET(max_fd, &master_set)) {
            max_fd--;
        }
    }

    fprintf(stderr, "\nЗавершение работы сервера...\n");

    for (int fd = 0; fd <= max_fd; fd++) {
        if (FD_ISSET(fd, &master_set))
            close(fd);
    }

    unlink(SOCKET_PATH);

    return 0;
}
