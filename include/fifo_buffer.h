#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct fifo_buffer_interface;
struct fifo_buffer;
struct fifo_buffer_element;

#define FIFO_BUFFER_INTERFACE_METHODS \
void (*dispose)(struct fifo_buffer *self); \
void (*free)(struct fifo_buffer *self); \
size_t (*capacity)(const struct fifo_buffer *self); \
size_t (*count)(const struct fifo_buffer *self); \
bool (*enqueue_default)(struct fifo_buffer *self, const void *element, size_t size); \
bool (*enqueue)(struct fifo_buffer *self, const void *element, size_t size, void *(*copy)(void *, const void *, size_t)); \
bool (*dequeue_default)(struct fifo_buffer *self, void *element); \
bool (*dequeue)(struct fifo_buffer *self, void *element, void *(*copy)(void *, const void *, size_t)); \
const void *(*peek)(const struct fifo_buffer *self); \
size_t (*peek_size)(const struct fifo_buffer *self); \
bool (*is_empty)(const struct fifo_buffer *self); \
bool (*is_full)(const struct fifo_buffer *self)

struct fifo_buffer_interface {
    FIFO_BUFFER_INTERFACE_METHODS;
};

struct fifo_buffer {
    const struct fifo_buffer_interface *vptr;
};

struct fifo_buffer_element {
    size_t size;
    uint8_t data[];
};

#endif //  FIFO_BUFFER_INTERFACE_H
