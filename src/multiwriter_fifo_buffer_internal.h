#ifndef MULTIWRITER_FIFO_BUFFER_INTERNAL_H
#define MULTIWRITER_FIFO_BUFFER_INTERNAL_H

#include <threads.h>

#include "lockfree_fifo_buffer_internal.h"

struct multiwriter_fifo_buffer {
    struct lockfree_fifo_buffer parent;
    mtx_t mutex;
};

void multiwriter_fifo_buffer_dispose(struct fifo_buffer *self);
void multiwriter_fifo_buffer_delete(struct fifo_buffer *self);
bool multiwriter_fifo_buffer_enqueue_default(struct fifo_buffer *self, const void *element, size_t size);
bool multiwriter_fifo_buffer_enqueue(struct fifo_buffer *self, const void *element, size_t size, void (*copy)(void *, const void *));
bool multiwriter_fifo_buffer_dequeue_default(struct fifo_buffer *self, void *element);
bool multiwriter_fifo_buffer_dequeue(struct fifo_buffer *self, void *element, void (*copy)(void *, void *));
const struct fifo_buffer_element *multiwriter_fifo_buffer_peek(const struct fifo_buffer *self);
bool multiwriter_fifo_buffer_is_empty(const struct fifo_buffer *self);
bool multiwriter_fifo_buffer_is_full(const struct fifo_buffer *self);
size_t multiwriter_fifo_buffer_next_index(const struct fifo_buffer *self, size_t index);

#endif // MULTIWRITER_FIFO_BUFFER_INTERNAL_H
