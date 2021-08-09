#ifndef LOCKFREE_FIFO_BUFFER_H
#define LOCKFREE_FIFO_BUFFER_H

#include "fifo_buffer.h"

struct lockfree_fifo_buffer;

bool lockfree_fifo_buffer_initialize(struct lockfree_fifo_buffer *self, size_t element_size, size_t count);
struct lockfree_fifo_buffer *lockfree_fifo_buffer_new(size_t element_size, size_t count);
void lockfree_fifo_buffer_dispose(struct fifo_buffer *self);
void lockfree_fifo_buffer_delete(struct fifo_buffer *self);

#endif // LOCKFREE_FIFO_BUFFER_H
