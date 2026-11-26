#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

#define SOCKET_PATH "./socket"
#define BUFFER_SIZE 1024

volatile sig_atomic_t stop_server = 0;
volatile sig_atomic_t wake_fd = -1;

static void handle_signal(int sig) {
    int saved_errno = errno;
    (void)sig;

    stop_server = 1;

    if (wake_fd != -1) {
        char c = 1;
        write((int)wake_fd, &c, 1);
    }

    errno = saved_errno;
}

static ssize_t read_message(int fd, char *buffer, size_t *len) {
    while (*len < BUFFER_SIZE) {
        ssize_t n = recv(fd, buffer + *len, 1, MSG_DONTWAIT);

        if (n == 1) {
            (*len)++;

            if (buffer[*len - 1] == '\n') {
                ssize_t result = (ssize_t)*len;
                *len = 0;
                return result;
            }

            continue;
        }

        if (n == 0)
            *len = 0;

        return n;
    }

    errno = EMSGSIZE;
    return -1;
}

static ssize_t filter_message(char *buffer, ssize_t n, unsigned char *state) {
    ssize_t len = 0;

    for (ssize_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)buffer[i];

        switch (*state) {
            case 1:
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

            case 2:
                if (c >= 0x40 && c <= 0x7E)
                    *state = 0;
                else if (c == 27)
                    *state = 1;

                continue;

            case 3:
                *state = (c == 27) ? 1 : 0;
                continue;

            case 4:
                if (c == 7)
                    *state = 0;
                else if (c == 27)
                    *state = 5;

                continue;

            case 5:
                if (c == '\\' || c == 7)
                    *state = 0;
                else if (c != 27)
                    *state = 4;

                continue;

            case 6:
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
    int max_fd;
    int signal_pipe[2] = {-1, -1};
    int socket_bound = 0;
    int result = EXIT_FAILURE;

    struct sockaddr_un addr;
    struct sigaction sa;
    struct timespec delay = {0, 5000000};

    fd_set master_set;
    fd_set read_set;

    int clients[FD_SETSIZE];
    int client_count = 0;

    static char buffers[FD_SETSIZE][BUFFER_SIZE];
    size_t lengths[FD_SETSIZE] = {0};
    unsigned char escape_state[FD_SETSIZE] = {0};

    if (pipe(signal_pipe) < 0)
        goto cleanup;

    if (signal_pipe[0] >= FD_SETSIZE)
        goto cleanup;

    if (fcntl(signal_pipe[1], F_SETFL, O_NONBLOCK) < 0)
        goto cleanup;

    wake_fd = signal_pipe[1];

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0)
        goto cleanup;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd < 0 || server_fd >= FD_SETSIZE)
        goto cleanup;

    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0)
        goto cleanup;

    unlink(SOCKET_PATH);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        goto cleanup;

    socket_bound = 1;

    if (listen(server_fd, 10) < 0)
        goto cleanup;

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    FD_SET(signal_pipe[0], &master_set);

    max_fd = server_fd > signal_pipe[0] ? server_fd : signal_pipe[0];

    fprintf(stderr, "Сервер запущен.\n");

    result = EXIT_SUCCESS;

    while (!stop_server) {
        read_set = master_set;

        int ready = select(max_fd + 1, &read_set, NULL, NULL, NULL);

        if (ready < 0) {
            if (errno == EINTR)
                continue;

            perror("select");
            result = EXIT_FAILURE;
            break;
        }

        if (stop_server || FD_ISSET(signal_pipe[0], &read_set))
            break;

        if (FD_ISSET(server_fd, &read_set)) {
            while (!stop_server) {
                int client_fd = accept(server_fd, NULL, NULL);

                if (client_fd < 0)
                    break;

                if (client_fd >= FD_SETSIZE) {
                    close(client_fd);
                    continue;
                }

                clients[client_count++] = client_fd;

                FD_SET(client_fd, &master_set);

                lengths[client_fd] = 0;
                escape_state[client_fd] = 0;

                if (client_fd > max_fd)
                    max_fd = client_fd;

                if (client_count == 1){
                    // nanosleep(&delay, NULL);
                }
                
            }
        }

        for (int i = 0; i < client_count && !stop_server; i++) {
            int fd = clients[i];

            if (!FD_ISSET(fd, &read_set))
                continue;

            char *buffer = buffers[fd];

            ssize_t n = read_message(fd, buffer, &lengths[fd]);

            if (n > 0) {
                ssize_t len = filter_message(buffer, n, &escape_state[fd]);

                if (len > 0) {
                    if (write(STDOUT_FILENO, buffer, (size_t)len) < 0) {
                        perror("write");
                        result = EXIT_FAILURE;
                        stop_server = 1;
                        break;
                    }
                }

            } else if (n == 0 || (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)) {
                close(fd);
                FD_CLR(fd, &master_set);

                lengths[fd] = 0;
                escape_state[fd] = 0;

                for (int j = i; j < client_count - 1; j++) {
                    clients[j] = clients[j + 1];
                }

                client_count--;
                i--;
            }
        }

        while (max_fd >= 0 && !FD_ISSET(max_fd, &master_set)) {
            max_fd--;
        }
    }

    fprintf(stderr, "\nЗавершение работы сервера...\n");

cleanup:
    for (int i = 0; i < client_count; i++) {
        close(clients[i]);
    }

    if (server_fd >= 0)
        close(server_fd);

    wake_fd = -1;

    if (signal_pipe[1] >= 0)
        close(signal_pipe[1]);

    if (signal_pipe[0] >= 0)
        close(signal_pipe[0]);

    if (socket_bound)
        unlink(SOCKET_PATH);

    return result;
}
