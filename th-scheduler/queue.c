#include <stdlib.h>
#include "queue.h"

struct queue queue_create() {
    struct queue q;
    q.head = NULL;
    q.tail = NULL;
    return q;
}

void queue_init(struct queue *q) {
    if (q == NULL) return;
    q->head = NULL;
    q->tail = NULL;
}

struct queue_entry *queue_new_node(void *data) {
    struct queue_entry *node = (struct queue_entry *)malloc(sizeof(struct queue_entry));
    if (node == NULL) return NULL;
    node->data = data;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

void queue_insert_tail(struct queue *q, struct queue_entry *node) {
    if (q == NULL || node == NULL) return;
    
    node->next = NULL;
    node->prev = q->tail;
    
    if (q->tail == NULL) {
        // Empty queue
        q->head = node;
        q->tail = node;
    } else {
        q->tail->next = node;
        q->tail = node;
    }
}

struct queue_entry *queue_peek_front(struct queue *q) {
    if (q == NULL) return NULL;
    return q->head;
}

void queue_pop_head(struct queue *q) {
    if (q == NULL || q->head == NULL) return;
    
    struct queue_entry *old_head = q->head;
    q->head = old_head->next;
    
    if (q->head == NULL) {
        // Queue is now empty
        q->tail = NULL;
    } else {
        q->head->prev = NULL;
    }
    
    // Note: We don't free the node here - caller may still need it
    // Caller should free if they created it with malloc
}

