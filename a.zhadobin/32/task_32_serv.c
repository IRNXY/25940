#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 128

sig_atomic_t stop_server = 0;

static void sigio_handler(int sig) {
    (void)sig;
}

static void stop_handler(int sig) {
    stop_server = 1;
}

static int set_handler(int sig, void (*handler)(int)) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);

    return sigaction(sig, &sa, NULL);
}

static int make_async(int fd) {
    int flags = fcntl(fd, F_GETFL);

    if (flags < 0)
        return -1;

    if (fcntl(fd, F_SETOWN, getpid()) < 0)
        return -1;

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK | FASYNC) < 0)
        return -1;

    return 0;
}

static ssize_t filter_text(char buffer[], ssize_t n, unsigned char *state) {
    ssize_t len = 0;

    for (ssize_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)buffer[i];

        if (*state == 1) {
            if (c == '[')
                *state = 2;
            else if (c == 'O')
                *state = 3;
            else if (c == ']' || c == 'P' || c == '^' || c == '_' || c == 'X')
                *state = 4;
            else if (c >= 0x20 && c <= 0x2F)
                *state = 6;
            else
                *state = 0;

            continue;
        }

        if (*state == 2) {
            if (c >= 0x40 && c <= 0x7E)
                *state = 0;
            else if (c == 27)
                *state = 1;

            continue;
        }

        if (*state == 3) {
            *state = (c == 27) ? 1 : 0;
            continue;
        }

        if (*state == 4) {
            if (c == 7)
                *state = 0;
            else if (c == 27)
                *state = 5;

            continue;
        }

        if (*state == 5) {
            if (c == '\\' || c == 7)
                *state = 0;
            else if (c != 27)
                *state = 4;

            continue;
        }

        if (*state == 6) {
            if (c >= 0x30 && c <= 0x7E)
                *state = 0;
            else if (c == 27)
                *state = 1;

            continue;
        }

        if (c == 27) {
            *state = 1;
            continue;
        }

        if ((c < 32 && c != '\n') || c == 127)
            continue;

        buffer[len++] = (char)toupper(c);
    }

    return len;
}

int main(void) {
    int server_fd = -1;
    int clients[MAX_CLIENTS];
    int bound = 0;

    struct sockaddr_un addr;
    sigset_t blocked, old_mask, wait_mask;
    unsigned char escape_state[MAX_CLIENTS] = {0};

    char shared_buffer[BUFFER_SIZE];

    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i] = -1;

    sigemptyset(&blocked);
    sigaddset(&blocked, SIGIO);
    sigaddset(&blocked, SIGINT);
    sigaddset(&blocked, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &blocked, &old_mask) < 0) {
        perror("sigprocmask");
        return 1;
    }

    wait_mask = old_mask;
    sigdelset(&wait_mask, SIGIO);
    sigdelset(&wait_mask, SIGINT);
    sigdelset(&wait_mask, SIGTERM);

    if (set_handler(SIGIO, sigio_handler) < 0 || set_handler(SIGINT, stop_handler) < 0 || set_handler(SIGTERM, stop_handler) < 0 || set_handler(SIGPIPE, SIG_IGN) < 0) {
        perror("sigaction");
        goto error;
    }

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        goto error;
    }

    if (unlink(SOCKET_PATH) < 0 && errno != ENOENT) {
        perror("unlink");
        goto error;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        goto error;
    }

    bound = 1;

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        goto error;
    }

    if (make_async(server_fd) < 0) {
        perror("make_async");
        goto error;
    }

    fprintf(stderr, "Асинхронный сервер запущен.\n");
    fprintf(stderr, "Ожидание клиентов на %s...\n", SOCKET_PATH);

    while (!stop_server) {
            while (1) {
            int client_fd = accept(server_fd, NULL, NULL);

            if (client_fd < 0) {
                if (errno == EINTR)
                    continue;

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;

                perror("accept");
                goto error;
            }

            int slot = -1;

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == -1) {
                    slot = i;
                    break;
                }
            }

            if (slot == -1) {
                fprintf(stderr, "Слишком много клиентов\n");
                close(client_fd);
                continue;
            }

            if (make_async(client_fd) < 0) {
                perror("make_async");
                close(client_fd);
                continue;
            }

            clients[slot] = client_fd;
            escape_state[slot] = 0;

            fprintf(stderr, "Клиент подключился (fd=%d)\n", client_fd);
        }
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = clients[i];

            if (fd == -1)
                continue;

            while (1) {
                ssize_t n = read(fd, shared_buffer, sizeof(shared_buffer));

                if (n > 0) {
                    ssize_t len = filter_text(shared_buffer, n, &escape_state[i]);

                    if (len > 0) {
                        if (write(STDOUT_FILENO, shared_buffer, (size_t)len) < 0) {
                            perror("write");
                            goto error;
                        }
                    }
                    continue;
                }

                if (n == 0) {
                    fprintf(stderr, "Клиент отключился (fd=%d)\n", fd);

                    close(fd);
                    clients[i] = -1;
                    escape_state[i] = 0;
                    break;
                }

                if (errno == EINTR)
                    continue;

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;

                perror("read");
                close(fd);
                clients[i] = -1;
                escape_state[i] = 0;
                break;
            }
        }

        if (!stop_server)
            sigsuspend(&wait_mask);
    }

    fprintf(stderr, "\nЗавершение работы сервера...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1)
            close(clients[i]);
    }

    if (server_fd >= 0)
        close(server_fd);

    if (bound)
        unlink(SOCKET_PATH);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    return 0;

error:
    fprintf(stderr, "\nЗавершение работы сервера...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1)
            close(clients[i]);
    }

    if (server_fd >= 0)
        close(server_fd);

    if (bound)
        unlink(SOCKET_PATH);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    return 1;
}
