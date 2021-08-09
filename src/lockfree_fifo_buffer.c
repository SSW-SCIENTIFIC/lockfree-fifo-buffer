#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "lockfree_fifo_buffer.h"
#include "lockfree_fifo_buffer_internal.h"

static const struct fifo_buffer_interface vtable = {
    .dispose = lockfree_fifo_buffer_dispose,
    .free = lockfree_fifo_buffer_delete,
    .capacity = lockfree_fifo_buffer_capacity,
    .count = lockfree_fifo_buffer_count,
    .enqueue_default = lockfree_fifo_buffer_enqueue_default,
    .enqueue = lockfree_fifo_buffer_enqueue,
    .dequeue_default = lockfree_fifo_buffer_dequeue_default,
    .dequeue = lockfree_fifo_buffer_dequeue,
    .peek = lockfree_fifo_buffer_peek,
    .peek_size = lockfree_fifo_buffer_peek_size,
    .is_empty = lockfree_fifo_buffer_is_empty,
    .is_full = lockfree_fifo_buffer_is_full,
};

#if !defined(__has_builtin)
#define __has_builtin(x) 0
#endif

static inline uint_fast64_t calc_aligned_capacity(const uint_fast64_t number)
{
#if __has_builtin(__builtin_clzll)
    if (number == 0) {
        return 0;
    }
    return 1ULL << (63 - __builtin_clzll(number) + 1);
#else
    uint_fast64_t value = number;
    value = (value & 0xFFFFFFFF00000000ULL) ? (value & 0xFFFFFFFF00000000ULL) : value;
    value = (value & 0xFFFF0000FFFF0000ULL) ? (value & 0xFFFF0000FFFF0000ULL) : value;
    value = (value & 0xFF00FF00FF00FF00ULL) ? (value & 0xFF00FF00FF00FF00ULL) : value;
    value = (value & 0xF0F0F0F0F0F0F0F0ULL) ? (value & 0xF0F0F0F0F0F0F0F0ULL) : value;
    value = (value & 0xCCCCCCCCCCCCCCCCULL) ? (value & 0xCCCCCCCCCCCCCCCCULL) : value;
    value = (value & 0xAAAAAAAAAAAAAAAAULL) ? (value & 0xAAAAAAAAAAAAAAAAULL) : value;
    return value << 1;
#endif
}

bool lockfree_fifo_buffer_initialize(struct lockfree_fifo_buffer *const self, const size_t element_size, const size_t count)
{
    const size_t aligned_capacity = calc_aligned_capacity(count);
    struct lockfree_fifo_buffer tmp = {
        .parent = { .vptr = &vtable },
        .element_size = element_size,
        .capacity = aligned_capacity,
        .buffer = (struct buffer_element **)calloc(aligned_capacity, sizeof(struct buffer_element *)),
    };

    atomic_init(&tmp.read_index, aligned_capacity - 1);
    atomic_init(&tmp.write_index, aligned_capacity - 1);

    if (tmp.buffer == NULL) {
        return false;
    }

    for (size_t i = 0; i < tmp.capacity; i++) {
        struct buffer_element *const element = (struct buffer_element *)malloc(sizeof(struct buffer_element) + tmp.element_size);
        if (element == NULL) {
            lockfree_fifo_buffer_dispose((struct fifo_buffer *)&tmp);
            return false;
        }

        element->size = 0;
        tmp.buffer[i] = element;
    }

    *(struct lockfree_fifo_buffer *)self = tmp;
    return true;
}

struct lockfree_fifo_buffer *lockfree_fifo_buffer_new(const size_t element_size, const size_t count)
{
    struct lockfree_fifo_buffer *const buf = malloc(sizeof(struct lockfree_fifo_buffer));
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
    struct lockfree_fifo_buffer *const _self = (struct lockfree_fifo_buffer *)self;

    for (size_t i = 0; i < _self->capacity; i++) {
        free(_self->buffer[i]);
    }
    free(_self->buffer);
    *_self = (struct lockfree_fifo_buffer){
        .parent = { .vptr = NULL },
        .capacity = 0,
        .element_size = 0,
        .buffer = NULL,
    };
}

void lockfree_fifo_buffer_delete(struct fifo_buffer *const self)
{
    if (self == NULL) {
        return;
    }

    lockfree_fifo_buffer_dispose(self);
    free(self);
}

size_t lockfree_fifo_buffer_capacity(const struct fifo_buffer *const self)
{
    assert(self != NULL);
    return ((const struct lockfree_fifo_buffer *)self)->capacity;
}

size_t lockfree_fifo_buffer_count(const struct fifo_buffer *const self)
{
    assert(self != NULL);

    const struct lockfree_fifo_buffer *const _self = (const struct lockfree_fifo_buffer *)self;
    const size_t read_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);
    const size_t write_index = atomic_load_explicit(&_self->write_index, memory_order_acquire);
    return write_index >= read_index ? write_index - read_index : _self->capacity - read_index + write_index;
}

bool lockfree_fifo_buffer_enqueue_default(struct fifo_buffer *const self, const void *const element, const size_t size)
{
    return lockfree_fifo_buffer_enqueue(self, element, size, memcpy);
}

bool lockfree_fifo_buffer_enqueue(struct fifo_buffer *const self, const void *const element, const size_t size, void *(*const copy)(void *, const void *, size_t))
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *const _self = (struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    if (lockfree_fifo_buffer_is_full(self)) {
        return false;
    }

    const size_t current_index = atomic_load_explicit(&_self->write_index, memory_order_acquire);
    const size_t next_index = lockfree_fifo_buffer_next_index(self, current_index);

    struct buffer_element *const dest = _self->buffer[current_index];
    copy(dest->buffer, element, size);
    dest->size = size;

    atomic_store_explicit(&_self->write_index, next_index, memory_order_release);
    return true;
}

bool lockfree_fifo_buffer_dequeue_default(struct fifo_buffer *const self, void *const element)
{
    return lockfree_fifo_buffer_dequeue(self, element, memcpy);
}

bool lockfree_fifo_buffer_dequeue(struct fifo_buffer *const self, void *const element, void *(*const copy)(void *, const void *, size_t))
{
    assert(self != NULL);

    struct lockfree_fifo_buffer *const _self = (struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    if (lockfree_fifo_buffer_is_empty(self)) {
        return false;
    }

    const size_t current_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);
    const size_t next_index = lockfree_fifo_buffer_next_index(self, current_index);

    if (element != NULL && copy != NULL) {
        struct buffer_element *const src = _self->buffer[current_index];
        copy(element, src->buffer, src->size);
    }

    atomic_store_explicit(&_self->read_index, next_index, memory_order_release);
    return true;
}

const void *lockfree_fifo_buffer_peek(const struct fifo_buffer *const self)
{
    assert(self != NULL);

    const struct lockfree_fifo_buffer *const _self = (const struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    if (lockfree_fifo_buffer_is_empty(self)) {
        return NULL;
    }

    const size_t current_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);
    return _self->buffer[current_index]->buffer;
}

size_t lockfree_fifo_buffer_peek_size(const struct fifo_buffer *const self)
{
    assert(self != NULL);

    const struct lockfree_fifo_buffer *const _self = (const struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    if (lockfree_fifo_buffer_is_empty(self)) {
        return 0;
    }

    const size_t current_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);
    return _self->buffer[current_index]->size;
}

bool lockfree_fifo_buffer_is_empty(const struct fifo_buffer *const self)
{
    assert(self != NULL);

    const struct lockfree_fifo_buffer *const _self = (const struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    const size_t write_index = atomic_load_explicit(&_self->write_index, memory_order_acquire);
    const size_t read_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);

    return write_index == read_index;
}

bool lockfree_fifo_buffer_is_full(const struct fifo_buffer *const self)
{
    assert(self != NULL);

    const struct lockfree_fifo_buffer *const _self = (const struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    const size_t write_index = atomic_load_explicit(&_self->write_index, memory_order_acquire);
    const size_t read_index = atomic_load_explicit(&_self->read_index, memory_order_acquire);

    return lockfree_fifo_buffer_next_index(self, write_index) == read_index;
}

size_t lockfree_fifo_buffer_next_index(const struct fifo_buffer *const self, const size_t index)
{
    assert(self != NULL);

    const struct lockfree_fifo_buffer *const _self = (const struct lockfree_fifo_buffer *)self;
    assert(_self->buffer != NULL);

    return (index + 1) & (_self->capacity - 1);
}
