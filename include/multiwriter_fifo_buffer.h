#ifndef MULTIWRITER_FIFO_BUFFER_H
#define MULTIWRITER_FIFO_BUFFER_H

#include "fifo_buffer.h"

struct multiwriter_fifo_buffer;

#define MULTIWRITER_FIFO_BUFFER_INTERFACE_METHODS \
    FIFO_BUFFER_INTERFACE_METHODS;                    \
    bool (*try_enqueue_default)(struct fifo_buffer *self, const void *element, size_t size); \
    bool (*try_enqueue)(struct fifo_buffer *self, const void *element, size_t size, void *(*copy)(void *, const void *, size_t))

union multiwriter_fifo_buffer_interface {
    struct fifo_buffer_interface parent;
    struct {
        MULTIWRITER_FIFO_BUFFER_INTERFACE_METHODS;
    };
};

struct multiwriter_fifo_buffer {
    const union multiwriter_fifo_buffer_interface *vptr;
};

bool multiwriter_fifo_buffer_initialize(struct multiwriter_fifo_buffer *self, size_t element_size, size_t count);
struct multiwriter_fifo_buffer *multiwriter_fifo_buffer_new(size_t element_size, size_t count);
void multiwriter_fifo_buffer_dispose(struct fifo_buffer *self);
void multiwriter_fifo_buffer_delete(struct fifo_buffer *self);
bool multiwriter_fifo_buffer_try_enqueue_default(struct fifo_buffer *self, const void *element, size_t size);
bool multiwriter_fifo_buffer_try_enqueue(struct fifo_buffer *self, const void *element, size_t size, void *(*copy)(void *, const void *, size_t));

#endif // MULTIWRITER_FIFO_BUFFER_H
