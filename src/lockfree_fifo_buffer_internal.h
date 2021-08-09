#ifndef LOCKFREE_FIFO_BUFFER_INTERNAL_H
#define LOCKFREE_FIFO_BUFFER_INTERNAL_H

#include <stdlib.h>
#include <stdatomic.h>

#include "fifo_buffer.h"
#include "lockfree_fifo_buffer.h"

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
#error "This library requires ISO/IEC 9899:2011 conformance environment to build."
#endif

#if defined(WIN32)
#endif

struct buffer_element {
    size_t size;
    uint8_t buffer[];
};

struct lockfree_fifo_buffer {
    struct fifo_buffer parent;
    size_t element_size;
    size_t capacity;
    atomic_size_t read_index;
    atomic_size_t write_index;
    struct buffer_element **buffer;
};

size_t lockfree_fifo_buffer_capacity(const struct fifo_buffer *self);
size_t lockfree_fifo_buffer_count(const struct fifo_buffer *self);
bool lockfree_fifo_buffer_enqueue_default(struct fifo_buffer *self, const void *element, size_t size);
bool lockfree_fifo_buffer_enqueue(struct fifo_buffer *self, const void *element, size_t size, void *(*copy)(void *, const void *, size_t));
bool lockfree_fifo_buffer_dequeue_default(struct fifo_buffer *self, void *element);
bool lockfree_fifo_buffer_dequeue(struct fifo_buffer *self, void *element, void *(*copy)(void *, const void *, size_t));
const void *lockfree_fifo_buffer_peek(const struct fifo_buffer *self);
size_t lockfree_fifo_buffer_peek_size(const struct fifo_buffer *self);
bool lockfree_fifo_buffer_is_empty(const struct fifo_buffer *self);
bool lockfree_fifo_buffer_is_full(const struct fifo_buffer *self);
size_t lockfree_fifo_buffer_next_index(const struct fifo_buffer *self, size_t index);

#endif // LOCKFREE_FIFO_BUFFER_INTERNAL_H
