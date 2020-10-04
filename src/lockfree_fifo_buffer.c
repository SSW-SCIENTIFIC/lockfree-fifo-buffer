#include <assert.h>
#include <stdbool.h>

#include "lockfree_fifo_buffer.h"
#include "lockfree_fifo_buffer_internal.h"

static const struct fifo_buffer_interface vtable = {
    .dispose = lockfree_fifo_buffer_dispose,
    .delete = lockfree_fifo_buffer_delete,
    .enqueue_default = lockfree_fifo_buffer_enqueue_default,
    .enqueue = lockfree_fifo_buffer_enqueue,
    .dequeue_default = lockfree_fifo_buffer_dequeue_default,
    .dequeue = lockfree_fifo_buffer_dequeue,
    .is_empty = lockfree_fifo_buffer_is_empty,
    .is_full = lockfree_fifo_buffer_is_full,
};

bool lockfree_fifo_buffer_initialize(struct lockfree_fifo_buffer *const self, const size_t element_size, const size_t count)
{
    struct lockfree_fifo_buffer tmp = (struct lockfree_fifo_buffer){
        .parent = { .vptr = &vtable },
        .buffer = (struct fifo_buffer_element *)malloc(sizeof(struct fifo_buffer_element) * count),
        .element_size = element_size,
        .capacity = count,
        .read_index = count - 1,
        .write_index = count - 1,
    };

    if (tmp.buffer == NULL) {
        return false;
    }

    for (size_t i = 0; i < tmp.capacity; i++) {
        tmp.buffer[i].data = malloc(tmp.element_size);
        tmp.buffer[i].size = 0;
        if (tmp.buffer[i].data == NULL) {
            for (size_t j = 0; j <= i; j++) {
                free(tmp.buffer[j].data);
            }
            return false;
        }
    }

    *self = tmp;
    return true;
}

struct lockfree_fifo_buffer *lockfree_fifo_buffer_new(const size_t element_size, const size_t count)
{
    struct lockfree_fifo_buffer *const buf = (struct lockfree_fifo_buffer *)malloc(sizeof(struct lockfree_fifo_buffer));
    if (buf == NULL) {
        return NULL;
    }

    if (!lockfree_fifo_buffer_initialize(buf, element_size, count)) {
        free(buf);
        return NULL;
    }

    return buf;
}

void lockfree_fifo_buffer_dispose(struct fifo_buffer *const self)
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *_self = (struct lockfree_fifo_buffer *)self;

    for (size_t i = 0; i < _self->capacity; i++) {
        free(_self->buffer[i].data);
    }
    free(_self->buffer);
    *_self = (struct lockfree_fifo_buffer){
        .parent = { .vptr = NULL }
        .buffer = NULL,
        .capacity = 0,
        .element_size = 0,
        .read_index = 0,
        .write_index = 0,
    };
}

void lockfree_fifo_buffer_delete(struct fifo_buffer *const self)
{
    assert(self != NULL);
    lockfree_fifo_buffer_dispose(self);
    free(self);
}


bool lockfree_fifo_buffer_enqueue_default(struct fifo_buffer *const self, const void *const element, const size_t size)
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *_self = (struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    if (lockfree_fifo_buffer_is_full(self)) {
        return false;
    }

    const size_t current_index = atomic_load_explicit(&_self->write_index, memory_order_acquire);
    const size_t next_index = lockfree_fifo_buffer_next_index(self, current_index);

    struct fifo_buffer_element *const dest = &_self->buffer[current_index];
    memcpy(dest->data, element, size);
    dest->size = size;

    atomic_store_explicit(&_self->write_index, next_index, memory_order_release);
}

bool lockfree_fifo_buffer_enqueue(struct fifo_buffer *const self, const void *const element, const size_t size, void (*const copy)(void *, const void *))
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *_self = (struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    if (lockfree_fifo_buffer_is_full(self)) {
        return false;
    }

    const size_t current_index = atomic_load_explicit(&_self->write_index, memory_order_acquire);
    const size_t next_index = lockfree_fifo_buffer_next_index(self, current_index);

    struct fifo_buffer_element *const dest = &_self->buffer[current_index];
    copy(dest->data, element);
    dest->size = size;

    atomic_store_explicit(&_self->write_index, next_index, memory_order_release);
}

bool lockfree_fifo_buffer_dequeue_default(struct fifo_buffer *const self, void *const element)
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *_self = (struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    if (lockfree_fifo_buffer_is_empty(self)) {
        return false;
    }

    const size_t current_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);
    const size_t next_index = lockfree_fifo_buffer_next_index(self, current_index);

    struct fifo_buffer_element *const src = &_self->buffer[current_index];
    memcpy(element, src->data, src->size);

    atomic_store_explicit(&_self->read_index, next_index, memory_order_release);
}

bool lockfree_fifo_buffer_dequeue(struct fifo_buffer *const self, void *const element, void (*const copy)(void *, void *))
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *_self = (struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    if (lockfree_fifo_buffer_is_empty(self)) {
        return false;
    }

    const size_t current_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);
    const size_t next_index = lockfree_fifo_buffer_next_index(self, current_index);

    struct fifo_buffer_element *const src = &_self->buffer[current_index];
    copy(element, src->data);

    atomic_store_explicit(&_self->read_index, next_index, memory_order_release);
}

const struct fifo_buffer_element *lockfree_fifo_buffer_peek(const struct fifo_buffer *const self)
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *_self = (struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    if (lockfree_fifo_buffer_is_empty(self)) {
        return NULL;
    }

    const size_t current_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);
    return &_self->buffer[current_index];
}

bool lockfree_fifo_buffer_is_empty(const struct fifo_buffer *const self)
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *_self = (struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    const size_t write_index = atomic_load_explicit(&_self->write_index, memory_order_acquire);
    const size_t read_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);

    return write_index == read_index;
}

bool lockfree_fifo_buffer_is_full(const struct fifo_buffer *const self)
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *_self = (struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    const size_t write_index = atomic_load_explicit(&_self->write_index, memory_order_acquire);
    const size_t read_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);

    return lockfree_fifo_buffer_next_index(self, write_index) == read_index;
}

size_t lockfree_fifo_buffer_next_index(const struct fifo_buffer *const self, const size_t index)
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *_self = (struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    return (index + 1) % _self->capacity;
}
