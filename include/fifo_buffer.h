#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <stdbool.h>
#include <stdlib.h>

struct fifo_buffer_interface;
struct fifo_buffer;
struct fifo_buffer_element;

struct fifo_buffer_interface {
    void (*dispose)(struct fifo_buffer *self);
    void (*delete)(struct fifo_buffer *self);
    bool (*enqueue_default)(struct fifo_buffer *self, const void *element, size_t size);
    bool (*enqueue)(struct fifo_buffer *self, const void *element, size_t size, void (*copy)(void *, const void *));
    bool (*dequeue_default)(struct fifo_buffer *self, void *element);
    bool (*dequeue)(struct fifo_buffer *self, void *element, void (*copy)(void *, void *));
    const struct buffer_element *(*peek)(const struct fifo_buffer *self);
    bool (*is_empty)(const struct fifo_buffer *self);
    bool (*is_full)(const struct fifo_buffer *self);
};

struct fifo_buffer {
    const struct fifo_buffer_interface *vptr;
};

struct fifo_buffer_element {
    size_t size;
    void *data;
};

#endif //  FIFO_BUFFER_INTERFACE_H
