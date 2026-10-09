#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <ulimit.h>
#include <errno.h>
#include <rctl.h>
#include <string.h>

extern char **environ;

struct Operation {
    char option;
    char *arg;
};

int print_ulimit_u_solaris(void)
{
    long max_procs;

    errno = 0;
    max_procs = sysconf(_SC_CHILD_MAX);

    if (max_procs == -1) {
        if (errno != 0) {
            perror("sysconf(_SC_CHILD_MAX)");
            return -1;
        }
        printf("ulimit -u: unlimited\n");
    } else {
        printf("ulimit -u: %ld\n", max_procs);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int opt;
    struct Operation operations[argc];
    int count = 0;

    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {

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
        if (operations[i].option == 'i') {

            printf("uid   %d\n", getuid());
            printf("euid  %d\n", geteuid());
            printf("gid   %d\n", getgid());
            printf("egid  %d\n", getegid());

        } else if (operations[i].option == 's') {
            setpgid(0, 0);

        } else if (operations[i].option == 'p') {

            printf("pid   %d\n", getpid());
            printf("ppid  %d\n", getppid());
            printf("pgrp  %d\n", getpgrp());

        } else if (operations[i].option == 'u') { 

            long max_procs;

            errno = 0;
            max_procs = sysconf(_SC_CHILD_MAX);

            if (max_procs == -1) {
                if (errno != 0) {
                    perror("sysconf(_SC_CHILD_MAX)");
                    return -1;
                }
                printf("ulimit -u: unlimited\n");
            } else {
                printf("ulimit -u: %ld\n", max_procs);
            }
            // struct rlimit limit;
            // getrlimit(RLIMIT_CPU, &limit);
            // printf("ulimit  %llu\n", (unsigned long long)limit.rlim_cur);

        } else if (operations[i].option == 'U') {
            // long new_limit = atol(optarg);
            // ulimit(UL_SETFSIZE, new_limit);
            // struct rlimit limit;
            // long value = strtol(operations[i].arg, NULL, 10);
            // getrlimit(RLIMIT_NPROC, &limit);

            // limit.rlim_cur = value;
            // setrlimit(RLIMIT_NPROC, &limit);
        
            unsigned long long value =
                    strtoull(operations[i].arg, NULL, 10);

                size_t size = rctlblk_size();
                rctlblk_t *old_blk = malloc(size);
                rctlblk_t *new_blk = malloc(size);

                if (!old_blk || !new_blk) {
                    perror("malloc");
                    free(old_blk);
                    free(new_blk);
                    return 1;
                }

                if (getrctl("project.max-processes",
                            NULL, old_blk, RCTL_FIRST) == -1) {
                    perror("getrctl");
                    free(old_blk);
                    free(new_blk);
                    return 1;
                }

                memcpy(new_blk, old_blk, size);
                rctlblk_set_value(new_blk, value);

                if (setrctl("project.max-processes",
                            old_blk, new_blk, RCTL_REPLACE) == -1) {
                    perror("setrctl");
                }

                free(old_blk);
                free(new_blk);

        } else if (operations[i].option == 'c') {

            struct rlimit limit;
            getrlimit(RLIMIT_CORE, &limit);
            printf("core size  %llu bytes\n", (unsigned long long)limit.rlim_cur);

        } else if (operations[i].option == 'C') {

            struct rlimit limit;
            long value = strtol(operations[i].arg, NULL, 10);
            getrlimit(RLIMIT_CORE, &limit);

            limit.rlim_cur = value;
            setrlimit(RLIMIT_CORE, &limit);

        } else if (operations[i].option == 'd') {

            char cwd[1000];
            getcwd(cwd, sizeof(cwd));
            printf("%s\n", cwd);

        } else if (operations[i].option == 'v') {

            for (char **env = environ; *env != NULL; env++){
                printf("%s\n", *env);
            }
    
        } else if (operations[i].option == 'V') {
            putenv(operations[i].arg);
        }

    }

    return 0;
}
