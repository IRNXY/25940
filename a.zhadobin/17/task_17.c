
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <string.h>

#define WIDTH 40
#define SIZE 4096

struct termios old;
int rows = 1;


void print_help(const char *program)
{
    printf("\n");
    printf("========================================\n");
    printf("             LINE EDITOR\n");
    printf("========================================\n");
    printf("Usage: %s [options]\n\n", program);

    printf("Available options:\n");
    printf("  -h, --help   Show this help message\n");

    printf("\nKeyboard controls:\n");
    printf("  Backspace    Delete last character\n");
    printf("  Ctrl+U       Delete current line\n");
    printf("  Ctrl+W       Delete last word\n");
    printf("  Ctrl+D       Exit if line is empty\n");
    printf("  Enter        Start a new line\n");

    printf("\nFeatures:\n");
    printf("  - Immediate character display\n");
    printf("  - Maximum line width: 40 characters\n");
    printf("  - Automatic word wrapping\n");
    printf("  - Bell on invalid input\n");

    printf("\nExamples:\n");
    printf("  %s\n", program);
    printf("  %s -h\n", program);

    printf("========================================\n\n");
}


void restore(void) {
    tcsetattr(0, TCSANOW, &old);
}


void redraw(char *s, int len) {
    int i = 0, j, col = 0, lines = 1;

    printf("\r");
    for (j = 1; j < rows; j++)
        printf("\033[A");
    printf("\033[J");

    while (i < len) {
        if (s[i] == ' ') {
            if (col == WIDTH) {
                printf("\r\n");
                col = 0;
                lines++;
            }
            putchar(s[i++]);
            col++;
        } else {
            j = i;
            while (j < len && s[j] != ' ')
                j++;

            if (col && col + j - i > WIDTH) {
                printf("\r\n");
                col = 0;
                lines++;
            }

            while (i < j) {
                if (col == WIDTH) {
                    printf("\r\n");
                    col = 0;
                    lines++;
                }
                putchar(s[i++]);
                col++;
            }
        }
    }

    rows = lines;
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    struct termios t;
    char s[SIZE];
    unsigned char c;
    int pos = 0, esc = 0;

    if (argc > 1) {
        if (argc == 2 &&
            (strcmp(argv[1], "-h") == 0 ||
             strcmp(argv[1], "--help") == 0)) {
            print_help(argv[0]);
            return 0;
        }

        fprintf(stderr, "Unknown option. Use -h for help.\n");
        return 1;
    }

    if (tcgetattr(0, &old) == -1)
        return 1;

    t = old;
    t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    t.c_iflag &= ~(ICRNL | IXON);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;

    if (tcsetattr(0, TCSAFLUSH, &t) == -1)
        return 1;

    atexit(restore);

    while (read(0, &c, 1) == 1) {
        if (esc == 1) {
            esc = 0;
            if (c == '[' || c == 'O') {
                esc = (c == '[') ? 2 : 3;
                continue;
            }
        }

        if (esc == 2) {
            if (c >= 0x40 && c <= 0x7E)
                esc = 0;
            continue;
        }

        if (esc == 3) {
            esc = 0;
            continue;
        }

        if (c == 27) {
            putchar('\a');
            fflush(stdout);
            esc = 1;
            continue;
        }


        if (c == 4) {
            if (pos == 0)
                break;
            putchar('\a');
        }


        else if (c == '\r' || c == '\n') {
            printf("\r\n");
            pos = 0;
            rows = 1;
        }

        else if (c == old.c_cc[VERASE] ||
                 c == 8 || c == 127) {
            if (pos > 0)
                pos--;
            else
                putchar('\a');
            redraw(s, pos);
        }

        else if (c == old.c_cc[VKILL] || c == 21) {
            pos = 0;
            redraw(s, pos);
        }

        else if (c == 23) {
            while (pos > 0 && s[pos-1] == ' ')
                pos--;
            while (pos > 0 && s[pos-1] != ' ')
                pos--;
            while (pos > 0 && s[pos-1] == ' ')
                pos--;
            redraw(s, pos);
        }


        else if (isprint(c) && pos < SIZE) {
            s[pos++] = c;
            redraw(s, pos);
        }

        else {
            putchar('\a');
        }

        fflush(stdout);
    }

    return 0;
}
