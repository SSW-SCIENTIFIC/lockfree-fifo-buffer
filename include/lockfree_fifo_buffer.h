#ifndef LOCKFREE_FIFO_BUFFER_H
#define LOCKFREE_FIFO_BUFFER_H

struct lockfree_fifo_buffer;

bool lockfree_fifo_buffer_initialize(struct lockfree_fifo_buffer *self, size_t element_size, size_t count);
struct lockfree_fifo_buffer *lockfree_fifo_buffer_new(size_t element_size, size_t count);

#endif // LOCKFREE_FIFO_BUFFER_H
