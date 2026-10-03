
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    char *str;
    struct node *next;
};

int main()
{
    char buffer[300];
    struct node *start = NULL;
    struct node *end = NULL;

    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        int len = strlen(buffer);
        buffer[len - 1] = '\0';
        len--;

        struct node *new_node = malloc(sizeof(struct node));

        if (new_node == NULL) {
            perror("malloc node");
            return 1;
        }

        new_node->str = malloc(len + 1);

        if (new_node->str == NULL) {
            perror("malloc str");
            free(new_node);
            return 1;
        }

        strcpy(new_node->str, buffer);

        new_node->next = NULL;

        if (start == NULL) {
            start = new_node;
            end = new_node;
        }else{
            end->next = new_node;
            end = new_node;
        }

        if (buffer[0] == '.') {
            break;
        }
    }


    printf("\nresult\n");

    struct node *current = start;

    while (current != NULL) {
        printf("%s\n", current->str);
        current = current->next;
    }

    return 0;
}