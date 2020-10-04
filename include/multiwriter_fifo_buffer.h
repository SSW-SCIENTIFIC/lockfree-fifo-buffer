#ifndef MULTIWRITER_FIFO_BUFFER_H
#define MULTIWRITER_FIFO_BUFFER_H

#include "fifo_buffer.h"

struct multiwriter_fifo_buffer;

bool multiwriter_fifo_buffer_initialize(struct multiwriter_fifo_buffer *self, size_t element_size, size_t count);
struct multiwriter_fifo_buffer *multiwriter_fifo_buffer_new(size_t element_size, size_t count);

#endif // MULTIWRITER_FIFO_BUFFER_H
