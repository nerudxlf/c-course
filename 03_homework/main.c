#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Node {
    long value;
    struct Node *next;
} Node;

void print_int(long value) {
    printf("%ld ", value);
    fflush(stdout);
}

bool p(long value) {
    return value & 1;
}

Node* add_element(long value, Node *next) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node) {
        abort();
    }
    new_node->value = value;
    new_node->next = next;
    return new_node;
}

void map(const Node *node, void (*func)(long)) {
    if (!node) {
        return;
    }
    func(node->value);
    map(node->next, func);
}

Node* filter(const Node *node, Node *acc, bool (*predicate)(long)) {
    if (!node) {
        return acc;
    }

    if (predicate(node->value)) {
        acc = add_element(node->value, acc);
    }

    return filter(node->next, acc, predicate);
}

int main(void) {
    long data[] = {4, 8, 15, 16, 23, 42};
    size_t data_length = sizeof(data) / sizeof(data[0]);

    Node *list = NULL;
    for (size_t i = 0; i < data_length; ++i) {
        list = add_element(data[data_length - 1 - i], list);
    }
    map(list, print_int);
    puts("");
    Node *filtered_list = filter(list, NULL, p);
    map(filtered_list, print_int);
    puts("");

    while (list) {
        Node *temp = list;
        list = list->next;
        free(temp);
    }
    while (filtered_list) {
        Node *temp = filtered_list;
        filtered_list = filtered_list->next;
        free(temp);
    }

    return 0;
}
