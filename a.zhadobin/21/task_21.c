
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

sig_atomic_t beep_count = 0;
sig_atomic_t quit_flag = 0;

void print_help(const char *program_name)
{
    printf("Использование: %s\n", program_name);
    printf("Программа «Пищалка».\n\n");
    printf("Управление:\n");
    printf("  Ctrl+C  — подать звуковой сигнал и увеличить счётчик.\n");
    printf("  Ctrl+\\  — вывести количество сигналов и завершить программу.\n");
}

void sigint_handler(int sig)
{
    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

void sigquit_handler(int sig)
{
    quit_flag = 1;
}

int main(int argc, char* argv[]){
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
    }

    struct sigaction sa_int;
    struct sigaction sa_quit;

    sa_int.sa_handler = sigint_handler;
    // очищает маску, чтобы можно было бы принять и другие сигналы не дожидаясь завершения обработчика
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        return 1;
    }

    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;

    if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
        perror("sigaction SIGQUIT");
        return 1;
    }
    sigset_t wait_mask;

    sigemptyset(&wait_mask);

    while (!quit_flag) {
        // временно меняем маску для приёмв сигнала
        sigsuspend(&wait_mask);
    }

    printf("\nВсего прозвучало сигналов: %d\n", (int)beep_count);
    printf("Программа завершена по SIGQUIT\n");

    return 0;
}
