#include <assert.h>

#include "multiwriter_fifo_buffer.h"
#include "multiwriter_fifo_buffer_internal.h"

static const struct fifo_buffer_interface vtable = {
    .dispose = multiwriter_fifo_buffer_dispose,
    .delete = multiwriter_fifo_buffer_delete,
    .enqueue_default = multiwriter_fifo_buffer_enqueue_default,
    .enqueue = multiwriter_fifo_buffer_enqueue,
    .dequeue_default = lockfree_fifo_buffer_dequeue_default,
    .dequeue = lockfree_fifo_buffer_dequeue,
    .is_empty = lockfree_fifo_buffer_is_empty,
    .is_full = lockfree_fifo_buffer_is_full,
};

bool multiwriter_fifo_buffer_initialize(struct multiwriter_fifo_buffer *const self, const size_t element_size, const size_t count)
{
    assert(self != NULL);

    if (mtx_init(&self->mutex, mtx_plain) != 0) {
        return false;
    }

    if (!lockfree_fifo_buffer_initialize((struct lockfree_fifo_buffer *)self, element_size, count)) {
        mtx_destroy(&self->mutex);
        return false;
    }

    ((struct fifo_buffer *)self)->vptr = &vtable;

    return true;
}

struct multiwriter_fifo_buffer *multiwriter_fifo_buffer_new(const size_t element_size, const size_t count)
{
    struct multiwriter_fifo_buffer *const buf = (struct multiwriter_fifo_buffer *)malloc(sizeof(struct multiwriter_fifo_buffer));
    if (!multiwriter_fifo_buffer_initialize(buf, element_size, count)) {
        free(buf);
        return NULL;
    }

    return buf;
}

void multiwriter_fifo_buffer_dispose(struct fifo_buffer *const self)
{
    assert(self != NULL);

    struct multiwriter_fifo_buffer *_self = (struct multiwriter_fifo_buffer *)self;
    lockfree_fifo_buffer_dispose(self);
    mtx_destroy(&_self->mutex);
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

    struct multiwriter_fifo_buffer *_self = (struct multiwriter_fifo_buffer *)self;
    mtx_lock(&_self->mutex);
    const bool result = lockfree_fifo_buffer_enqueue_default(self, element, size);
    mtx_unlock(&_self->mutex);

    return result;
}

bool multiwriter_fifo_buffer_enqueue(struct fifo_buffer *const self, const void *const element, const size_t size, void (*const copy)(void *, const void *))
{
    assert(self != NULL);

    struct multiwriter_fifo_buffer *_self = (struct multiwriter_fifo_buffer *)self;
    mtx_lock(&_self->mutex);
    const bool result = lockfree_fifo_buffer_enqueue(self, element, size, copy);
    mtx_unlock(&_self->mutex);

    return result;
}
