// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/syscall.h>
// #include <sys/types.h>
// #include <sys/resource.h>
// #include <ulimit.h>


// extern char **environ;

// struct Operation {
//     char option;
//     char *arg;
// };

// int main(int argc, char *argv[])
// {
//     int opt;
//     struct Operation operations[argc];
//     int count = 0;

//     while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {

//         if (opt == '?') {
//             continue;
//         }

//         if (opt == 'U' || opt == 'C' || opt == 'V') {
//             operations[count].option = opt;
//             operations[count].arg = optarg;
//             count += 1;
//         } else {
//             operations[count].option = opt;
//             operations[count].arg = NULL;
//             count += 1;
//         }
//     }
    
//     for (int i = count - 1; i >= 0; i--) {
//         if (operations[i].option == 'i') {

//             printf("uid   %d\n", getuid());
//             printf("euid  %d\n", geteuid());
//             printf("gid   %d\n", getgid());
//             printf("egid  %d\n", getegid());

//         } else if (operations[i].option == 's') {
//             setpgid(0, 0);

//         } else if (operations[i].option == 'p') {

//             printf("pid   %d\n", getpid());
//             printf("ppid  %d\n", getppid());
//             printf("pgrp  %d\n", getpgrp());

//         } else if (operations[i].option == 'u') { 
//             // long limit_new;
//             // limit_new = ulimit(UL_GETMAXPROCS);
//             system("ulimit -a"); //Вызов для печати
//             printf("ulimit - %ld\n", ulimit(UL_GETFSIZE));
            
//             // struct rlimit limit;
//             // getrlimit(RLIMIT_NPROC, &limit);
//             // printf("ulimit  %llu\n", (unsigned long long)limit.rlim_cur);

//         } else if (operations[i].option == 'U') {
//             long new_limit = atol(optarg);
//             ulimit(UL_SETFSIZE, new_limit);
//             // struct rlimit limit;
//             // long value = strtol(operations[i].arg, NULL, 10);
//             // getrlimit(RLIMIT_NPROC, &limit);

//             // limit.rlim_cur = value;
//             // setrlimit(RLIMIT_NPROC, &limit);

//         } else if (operations[i].option == 'c') {

//             struct rlimit limit;
//             getrlimit(RLIMIT_CORE, &limit);
//             printf("core size  %llu bytes\n", (unsigned long long)limit.rlim_cur);

//         } else if (operations[i].option == 'C') {

//             struct rlimit limit;
//             long value = strtol(operations[i].arg, NULL, 10);
//             getrlimit(RLIMIT_CORE, &limit);

//             limit.rlim_cur = value;
//             setrlimit(RLIMIT_CORE, &limit);

//         } else if (operations[i].option == 'd') {

//             char cwd[1000];
//             getcwd(cwd, sizeof(cwd));
//             printf("%s\n", cwd);

//         } else if (operations[i].option == 'v') {

//             for (char **env = environ; *env != NULL; env++){
//                 printf("%s\n", *env);
//             }
    
//         } else if (operations[i].option == 'V') {
//             putenv(operations[i].arg);
//         }

//     }

//     return 0;
// }

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/resource.h>
#include <ulimit.h>

extern char *optarg; //Чтобы мне ерорки глаза не мозолили 
extern int optind, opterr, optopt;

extern char **environ; //Окружение 

/*
1) Аргумент флага
2) Сл флаг
3) 
*/

//gcc -Wall -Wextra -Wpedantic -g getopt.c -o res

//https://www.opennet.ru/man.shtml

int main(int argc, char *argv[])
{
    const char *optstring = "ispuU:cC:dvV:"; //Допустиые аргументы, : после для продления

    int flag;

    while((flag = getopt(argc, argv, optstring)) != -1)
    {
        switch(flag)
        {
            case 'i':
                // system("id"); //А так мне нравиться больше
                /*
                getuid(); //Реальный id пользователя
                geteuid(); //eффективный id пользователя
                getgit(); //Аналогично группы
                getegit();
                */
                printf("UID - %d\teUID - %d\tGID - %d\teGID - %d\n", getuid(), geteuid(), getgid(), getegid()); //Ладно, по ТЗ
                break;
            case 's':
                // printf("Pre: PID - %d PGID - %d\n", getpid(), getpgrp());

                setpgrp(); // Он же - setpgid(0, 0); //Первый - не менять группу | Второй - поставить ID равный группе | Иначе там хз сложно не осилил буквы

                // printf("Post: PID - %d PGID - %d\n", getpid(), getpgrp());
                break;
            case 'p':
                printf("PID - %d\tPGID - %d\tPPID - %d\n", getpid(), getpgrp(), getppid());
                break;
            case 'u':
                //system("ulimit"); //Вызов для печати
                printf("ULIMIT - %ld\n", ulimit(UL_GETFSIZE));
                break;
            case 'U':
                {
                    long new_limit = atol(optarg);

                    ulimit(UL_SETFSIZE, new_limit);

                    // printf("NEW_LIMIT - %ld\n", new_limit);

                    // if(!(new_limit <= 0 || new_limit >= 4096)) //Мне так хочется
                    // {
                    //     // char *command = "ulimit -f";
                    //     // char *buf;
                    //     // strcat(command, buf);
                    //     // sprintf(buf, "%d", new_limit);

                    //     // system(command);

                    //     ulimit(UL_SETFSIZE, new_limit);
                    // } else if(new_limit == -1)
                    // {
                    //     ulimit(UL_SETFSIZE, -1);
                    // }
                }
                break;
            case 'c':
                {
                    // system("ulimit -c");
                    struct rlimit tmp;
                    getrlimit(RLIMIT_CORE, &tmp);
                    printf("CORE - %lu\n", tmp.rlim_cur);
                }
                break;
            case 'C':
                {
                    int new_size = atoi(optarg);
                    struct rlimit tmp;
                    getrlimit(RLIMIT_CORE, &tmp);

                    tmp.rlim_cur = new_size;

                    setrlimit(RLIMIT_CORE, &tmp);
                    // if(new_size <= 0 || new_size >= 1024) //Мне так хочется
                    // {
                    //     // char *command = "ulimit -c";
                    //     // char *buf;
                    //     // strcat(command, buf);
                    //     // sprintf(buf, "%d", new_size);

                    //     // system(command);

                    //     tmp.rlim_cur = new_size;

                    //     setrlimit(RLIMIT_CORE, &tmp);
                    // } else if(new_size == -1)
                    // {
                    //     // system("ulimit -c unlimit");
                    //     tmp.rlim_cur = -1;
                    //     setrlimit(RLIMIT_CORE, &tmp);
                    // }   
                } 
                break;
            case 'd':
                // system("ls");
                printf("DIR - %s\n", getcwd(NULL, 0));
                break;
            case 'v':
                // system("env");
                {
                    int cur = 0;

                    while(environ[cur] != NULL)
                    {
                        printf("%s\n", environ[cur++]);
                    }
                }
                break;
            case 'V':
                putenv(optarg);
                break;
            case '?':
                printf("Неизвестный аргумент - -%c\n", optopt);
                break;
        }
    }

    for(int i = optind; i < argc; i++)
    {
        printf("Неподдерживаевый аргумент - %s\n", argv[i]);
    }

    return 0;
}