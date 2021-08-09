#include <assert.h>
#include <stdint.h>

#include "multiwriter_fifo_buffer.h"
#include "multiwriter_fifo_buffer_internal.h"

static const union multiwriter_fifo_buffer_interface vtable = {
    .dispose = multiwriter_fifo_buffer_dispose,
    .free = multiwriter_fifo_buffer_delete,
    .capacity = lockfree_fifo_buffer_capacity,
    .count = lockfree_fifo_buffer_count,
    .enqueue_default = multiwriter_fifo_buffer_enqueue_default,
    .enqueue = multiwriter_fifo_buffer_enqueue,
    .dequeue_default = lockfree_fifo_buffer_dequeue_default,
    .dequeue = lockfree_fifo_buffer_dequeue,
    .peek = lockfree_fifo_buffer_peek,
    .peek_size = lockfree_fifo_buffer_peek_size,
    .is_empty = lockfree_fifo_buffer_is_empty,
    .is_full = lockfree_fifo_buffer_is_full,
    .try_enqueue_default = multiwriter_fifo_buffer_try_enqueue_default,
    .try_enqueue = multiwriter_fifo_buffer_try_enqueue,
};

bool multiwriter_fifo_buffer_initialize(struct multiwriter_fifo_buffer *const self, const size_t element_size, const size_t count)
{
    assert(self != NULL);
    struct multiwriter_fifo_buffer_impl *const _self = (struct multiwriter_fifo_buffer_impl *)self;

    if (pthread_mutex_init(&_self->mutex, NULL) != 0) {
        return false;
    }

    if (!lockfree_fifo_buffer_initialize((struct lockfree_fifo_buffer *)self, element_size, count)) {
        pthread_mutex_destroy(&_self->mutex);
        return false;
    }

    _self->parent.vptr = &vtable;

    return true;
}

struct multiwriter_fifo_buffer *multiwriter_fifo_buffer_new(const size_t element_size, const size_t count)
{
    struct multiwriter_fifo_buffer *const buf = (struct multiwriter_fifo_buffer *)malloc(sizeof(struct multiwriter_fifo_buffer_impl));
    if (!multiwriter_fifo_buffer_initialize(buf, element_size, count)) {
        free(buf);
        return NULL;
    }

    return buf;
}

void multiwriter_fifo_buffer_dispose(struct fifo_buffer *const self)
{
    assert(self != NULL);

    struct multiwriter_fifo_buffer_impl *const _self = (struct multiwriter_fifo_buffer_impl *)self;
    pthread_mutex_destroy(&_self->mutex);

    lockfree_fifo_buffer_dispose(self);
}

void multiwriter_fifo_buffer_delete(struct fifo_buffer *const self)
{
    if (self == NULL) {
        return;
    }

    multiwriter_fifo_buffer_dispose(self);
    free(self);
}

bool multiwriter_fifo_buffer_enqueue_default(struct fifo_buffer *const self, const void *const element, const size_t size)
{
    assert(self != NULL);

    struct multiwriter_fifo_buffer_impl *const _self = (struct multiwriter_fifo_buffer_impl *)self;
    pthread_mutex_lock(&_self->mutex);
    const bool result = lockfree_fifo_buffer_enqueue_default(self, element, size);
    pthread_mutex_unlock(&_self->mutex);

    return result;
}

bool multiwriter_fifo_buffer_enqueue(struct fifo_buffer *const self, const void *const element, const size_t size, void *(*const copy)(void *, const void *, size_t))
{
    assert(self != NULL);

    struct multiwriter_fifo_buffer_impl *const _self = (struct multiwriter_fifo_buffer_impl *)self;
    pthread_mutex_lock(&_self->mutex);
    const bool result = lockfree_fifo_buffer_enqueue(self, element, size, copy);
    pthread_mutex_unlock(&_self->mutex);

    return result;
}

bool multiwriter_fifo_buffer_try_enqueue_default(struct fifo_buffer *self, const void *element, size_t size)
{
    assert(self != NULL);

    struct multiwriter_fifo_buffer_impl *const _self = (struct multiwriter_fifo_buffer_impl *)self;
    if (pthread_mutex_trylock(&_self->mutex) != 0) {
        return false;
    }
    const bool result = lockfree_fifo_buffer_enqueue_default(self, element, size);
    pthread_mutex_unlock(&_self->mutex);

    return result;
}

bool multiwriter_fifo_buffer_try_enqueue(struct fifo_buffer *self, const void *element, size_t size, void *(*copy)(void *, const void *, size_t))
{
    assert(self != NULL);

    struct multiwriter_fifo_buffer_impl *const _self = (struct multiwriter_fifo_buffer_impl *)self;
    if (pthread_mutex_trylock(&_self->mutex) != 0) {
        return false;
    }
    const bool result = lockfree_fifo_buffer_enqueue(self, element, size, copy);
    pthread_mutex_unlock(&_self->mutex);

    return result;
}
