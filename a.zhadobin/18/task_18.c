
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>
#include <string.h>

// Вывод справки
void print_help(const char *program)
{
    printf("\n");
    printf("========================================\n");
    printf("              MY LS\n");
    printf("========================================\n");
    printf("Usage: %s [options] file...\n\n", program);

    printf("Available options:\n");
    printf("  -h, --help   Show this help message\n");

    printf("\nFile information:\n");
    printf("  - File type and permissions\n");
    printf("  - Number of hard links\n");
    printf("  - Owner and group names\n");
    printf("  - File size (regular files only)\n");
    printf("  - Last modification date\n");
    printf("  - File name without path\n");

    printf("\nExamples:\n");
    printf("  %s /etc/passwd\n", program);
    printf("  %s /etc/passwd /tmp /dev/null\n", program);
    printf("  %s -h\n", program);

    printf("========================================\n\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help(argv[0]);
        return 0;
    }

    for (int i = 1; i < argc; i++) {

        struct stat st;

        if (lstat(argv[i], &st) == -1) {
            perror(argv[i]);
            continue;
        }

        char perm[11];

        if (S_ISDIR(st.st_mode))
            perm[0] = 'd';
        else if (S_ISREG(st.st_mode))
            perm[0] = '-';
        else
            perm[0] = '?';

        perm[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        perm[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        perm[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';

        perm[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        perm[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        perm[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';

        perm[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
        perm[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        perm[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';

        perm[10] = '\0';

        struct passwd *pw = getpwuid(st.st_uid);
        const char *owner = pw ? pw->pw_name : "?";

        struct group *gr = getgrgid(st.st_gid);
        const char *group = gr ? gr->gr_name : "?";

        char date[32];

        time_t now = time(NULL);
        double diff = difftime(now, st.st_mtime);

        struct tm *tm_info = localtime(&st.st_mtime);

        if (tm_info == NULL) {
            snprintf(date, sizeof(date), "unknown");
        }else {
            strftime(date, sizeof(date), "%b %e %H:%M", tm_info);
        }

        char *path_copy = strdup(argv[i]);

        if (path_copy == NULL) {
            perror("strdup");
            continue;
        }

        char *name = basename(path_copy);

        printf("%-10s %3lu %-10s %-10s", perm, st.st_nlink, owner, group);

        if (S_ISREG(st.st_mode)){
            printf(" %10lld", (long long)st.st_size);
        }else{
            printf(" %10s", "");
        }

        printf(" %-12s %s\n", date, name);

        free(path_copy);
    }

    return 0;
}
