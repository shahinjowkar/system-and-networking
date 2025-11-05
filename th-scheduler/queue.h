#ifndef QUEUE_H
#define QUEUE_H

struct queue_entry {
    void *data;
    struct queue_entry *next;
    struct queue_entry *prev;
};

struct queue {
    struct queue_entry *head;
    struct queue_entry *tail;
};

struct queue queue_create();
void queue_init(struct queue *q);
struct queue_entry *queue_new_node(void *data);
void queue_insert_tail(struct queue *q, struct queue_entry *node);
struct queue_entry *queue_peek_front(struct queue *q);
void queue_pop_head(struct queue *q);

#endif

