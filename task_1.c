#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/prctl.h>

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
            int limit_new;
            prctl(PR_GETPROCESSLIMIT, &limit_new);
            printf("Process limit: %d\n", limit_new);
            
            // struct rlimit limit;
            // getrlimit(RLIMIT_NPROC, &limit);
            // printf("ulimit  %llu\n", (unsigned long long)limit.rlim_cur);

        } else if (operations[i].option == 'U') {

            struct rlimit limit;
            long value = strtol(operations[i].arg, NULL, 10);
            getrlimit(RLIMIT_NPROC, &limit);

            limit.rlim_cur = value;
            setrlimit(RLIMIT_NPROC, &limit);

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