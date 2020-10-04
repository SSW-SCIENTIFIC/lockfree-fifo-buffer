#ifndef LOCKFREE_FIFO_BUFFER_INTERNAL
#define LOCKFREE_FIFO_BUFFER_INTERNAL

#include <stdlib.h>
#include <stdatomic.h>

#include "fifo_buffer.h"
#include "lockfree_fifo_buffer.h"

struct lockfree_fifo_buffer {
    struct fifo_buffer parent;
    size_t element_size;
    size_t capacity;
    atomic_size_t read_index;
    atomic_size_t write_index;
    struct fifo_buffer_element *buffer;
};

void lockfree_fifo_buffer_dispose(struct fifo_buffer *self);
void lockfree_fifo_buffer_delete(struct fifo_buffer *self);
bool lockfree_fifo_buffer_enqueue_default(struct fifo_buffer *self, const void *element, size_t size);
bool lockfree_fifo_buffer_enqueue(struct fifo_buffer *self, const void *element, size_t size, void (*copy)(void *, const void *));
bool lockfree_fifo_buffer_dequeue_default(struct fifo_buffer *self, void *element);
bool lockfree_fifo_buffer_dequeue(struct fifo_buffer *self, void *element, void (*copy)(void *, void *));
const struct fifo_buffer_element *lockfree_fifo_buffer_peek(const struct fifo_buffer *self);
bool lockfree_fifo_buffer_is_empty(const struct fifo_buffer *self);
bool lockfree_fifo_buffer_is_full(const struct fifo_buffer *self);
size_t lockfree_fifo_buffer_next_index(const struct fifo_buffer *self, size_t index);

#endif // LOCKFREE_FIFO_BUFFER_INTERNAL
