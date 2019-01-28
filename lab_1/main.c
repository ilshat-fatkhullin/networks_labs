#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <unistd.h>

struct Node
{
    struct Node *next;
    int value;
};

struct Node* root = NULL;

int count;

int peek()
{
    if (root == NULL) {
        printf("Error: stack is not initialized.\n");
        return 0;
    }

    if (count == 0) {
        printf("Error: stack is empty.\n");
        return 0;
    }

    return root->value;
}

void push(int data)
{
    if (root == NULL) {
        printf("Error: stack is not initialized.\n");
        return;
    }

    if (count == 0) {
        root->value = data;
    }
    else {
        struct Node *next_node = malloc(sizeof(struct Node));
        next_node->value = data;
        next_node->next = root;
        root = next_node;
    }

    count++;

    printf("%d pushed into the stack.\n", data);
}

void pop()
{
    if (root == NULL) {
        printf("Error: stack is not initialized.\n");
        return;
    }

    if (count == 0) {
        printf("Error: stack is empty.\n");
        return;
    }

    struct Node* top_node = root;

    int value = top_node->value;

    root = root->next;
    free(top_node);

    count--;
    printf("%d popped from the stack.\n", value);
}

int empty()
{
    if (root == NULL) {
        printf("Error: stack is not initialized.\n");
        return 1;
    }

    if (count == 0) {
        printf("Stack is empty.\n");
    }
    else {
        printf("Stack is not empty.\n");
    }
    return count == 0;
}

void display()
{
    if (root == NULL) {
        printf("Error: stack is not initialized.\n");
        return;
    }

    printf("Stack elements:\n");

    if (count == 0) {
        printf("Nothing to display\n");
        return;
    }

    struct Node* current_node = root;

    while (current_node != NULL)
    {
        printf("%d\n", current_node->value);
        current_node = current_node->next;
    }
}

void create()
{
    root = malloc(sizeof(struct Node));
    count = 0;
    printf("Stack created.\n");
}

void stack_size()
{
    printf("Stack size: %d\n", count);
}

int get_argument(char* input) {
    int result = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] >= '0' && input[i] <= '9') {
            result *= 10;
            result += input[i] - '0';
        }
    }
    return result;
}

void read_line(char* array, int size) {
    char c;
    for (int i = 0; i < size; i++) {
        scanf("%c", &c);
        array[i] = c;
        if (c == '\n') {
            array[i] = '\0';
            return;
        }
    }
}

int main()
{
    printf("Commands: create, peek, push, pop, empty, display.\n");
    printf("For 'push' command there are the following pattern: push [number].\nFor example: push 10.\n");

    int request_pipe[2];

    if (pipe(request_pipe) == -1) {
        printf("Error: request pipe creation failed.");
        return 1;
    }

    int pid = fork();

    if (pid > 0) {
        close(request_pipe[0]);
        while (1) {
            char input[128];
            read_line(input, 128);
            write(request_pipe[1], input, 128);
        }
    } else if (pid == 0) {
        close(request_pipe[1]);
        while (1) {
            char output[128];
            read(request_pipe[0], output, 128);

            if (strstr(output, "peek")) {
                peek();
            } else if (strstr(output, "push")) {
                push(get_argument(output));
            } else if (strstr(output, "pop")) {
                pop();
            } else if (strstr(output, "empty")) {
                empty();
            } else if (strstr(output, "display")) {
                display();
            } else if (strstr(output, "create")) {
                create();
            } else if (strstr(output, "stack_size")) {
                stack_size();
            };
        }
    } else {
        printf("Error: process can not be created.");
    }
    return 0;
}