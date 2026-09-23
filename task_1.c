#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <limits.h>
#include <string.h>

extern char **environ;

struct Operation {
    char option;
    char *arg;
};

int main(int argc, char *argv[])
{
    int opt;
    struct Operation operations[argc];
    int count = 0;

    while ((opt = getopt(argc, argv, "ispuU:cCdvV:")) != -1) {

        if (opt == '?') {
            continue;
        }

        if (opt == 'U' || opt == 'C' || opt == 'V') {
            operations[count].option = opt;
            operations[count].arg = optarg;
            count += 1;
        } else {
            operations[count].option = opt;
            operations[count].arg = NULL;
            count += 1;
        }
    }

    for (int i = count - 1; i >= 0; i--) {

        switch (operations[i].option) {

            case 'i':
                printf("uid:  %d\n", getuid());
                printf("euid: %d\n", geteuid());
                printf("gid:  %d\n", getgid());
                printf("egid: %d\n", getegid());
                break;

            case 's':
                if (setpgid(0, 0) == -1)
                    perror("setpgid");
                break;

            case 'p':
                printf("pid:  %d\n", getpid());
                printf("ppid: %d\n", getppid());
                printf("pgrp: %d\n", getpgrp());
                break;

            case 'u': {
                struct rlimit limit;

                if (getrlimit(RLIMIT_FSIZE, &limit) == -1)
                    perror("getrlimit");
                else
                    printf("ulimit: %llu\n", (unsigned long long)limit.rlim_cur);

                break;
            }

            case 'U': {
                struct rlimit limit;
                long value;

                value = strtol(operations[i].arg, NULL, 10);

                if (getrlimit(RLIMIT_FSIZE, &limit) == -1) {
                    perror("getrlimit");
                    return 1;
                }

                limit.rlim_cur = value;

                if (setrlimit(RLIMIT_FSIZE, &limit) == -1) {
                    perror("setrlimit");
                    return 1;
                }

                break;
            }

            case 'c': {
                struct rlimit limit;

                if (getrlimit(RLIMIT_CORE, &limit) == -1)
                    perror("getrlimit");
                else
                    printf("core size: %llu bytes\n", (unsigned long long)limit.rlim_cur);

                break;
            }

            case 'C': {
                struct rlimit limit;
                long value;

                value = strtol(operations[i].arg, NULL, 10);

                if (getrlimit(RLIMIT_CORE, &limit) == -1) {
                    perror("getrlimit");
                    return 1;
                }

                limit.rlim_cur = value;

                if (setrlimit(RLIMIT_CORE, &limit) == -1) {
                    perror("setrlimit");
                    return 1;
                }

                break;
            }

            case 'd': {
                char cwd[PATH_MAX];

                if (getcwd(cwd, sizeof(cwd)) == NULL)
                    perror("getcwd");
                else
                    printf("%s\n", cwd);

                break;
            }

            case 'v':
                for (char **env = environ; *env != NULL; env++)
                    printf("%s\n", *env);
                break;

            case 'V':
                if (putenv(operations[i].arg) != 0) {
                    perror("putenv");
                    return 1;
                }
                break;
        }
    }

    return 0;
}