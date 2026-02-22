/*
 * ring_buffer.c
 *
 *  Created on: Mar 3, 2024
 *      Author: Joshua Butler, MD, MHI
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ring_buffer.h"

uint8_t ring_buffer_init(ring_buffer_t *buffer, uint16_t buffer_size, size_t data_size) {
    buffer->data = malloc((size_t) buffer_size * data_size);
    if (buffer->data == NULL) {
        return RING_BUFFER_MALLOC_FAILED;
    }
    memset(buffer->data, 0, buffer_size * data_size);
    buffer->size = data_size;
    buffer->buffer_size = buffer_size;
    buffer->count = 0;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->isFull = false;
    buffer->new_data = false;

    return RING_BUFFER_OK;
}

uint8_t ring_buffer_destroy(ring_buffer_t *buffer) {
    free(buffer->data);
    if (buffer->data != NULL) {
        return RING_BUFFER_ERROR;
    }
    buffer->data = NULL;
    buffer->buffer_size = 0;
    buffer->count = 0;
    buffer->size = 0;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->isFull = false;

    return RING_BUFFER_OK;
}

bool is_ring_buffer_empty(const ring_buffer_t *buffer) {
    return (!buffer->isFull && buffer->head == buffer->tail);
}

bool is_ring_buffer_full(const ring_buffer_t *buffer) {
    return buffer->isFull;
}

bool ring_buffer_enqueue(ring_buffer_t *buffer, const void *value) {
    if (buffer->isFull) {
        return false;  // Buffer is full, cannot enqueue
    }

    uint8_t *buffer_data = (uint8_t*) buffer->data;
    const uint8_t *value_data = (const uint8_t*) value;

    for (int i = 0; i < buffer->size; i++) {
        buffer_data[(buffer->head * buffer->size + i) % (buffer->size * buffer->buffer_size)] = value_data[i];
    }
    buffer->peek = buffer->head;
    buffer->head = (buffer->head + 1) % buffer->buffer_size;
    buffer->count++;

    if (buffer->count == buffer->buffer_size) {
        buffer->isFull = true;  // Buffer is full after enqueueing
    }

    buffer->new_data = true;
    buffer->num_new_data++;
    return true;
}

bool ring_buffer_dequeue(ring_buffer_t *buffer, void *value) {
    if (is_ring_buffer_empty(buffer)) {
        return false;  // Buffer is empty, cannot dequeue
    }

    uint8_t *buffer_data = (uint8_t*) buffer->data;
    uint8_t *value_data = (uint8_t*) value;

    for (size_t i = 0; i < buffer->size; i++) {
        value_data[i] = buffer_data[(buffer->tail * buffer->size + i) % (buffer->size * buffer->buffer_size)];
    }
    memset(&buffer_data[(buffer->tail * buffer->size) % (buffer->size * buffer->buffer_size)], 0,
            buffer->size);
    buffer->tail = (buffer->tail + 1) % buffer->buffer_size;
    buffer->count--;
    buffer->isFull = false;  // Buffer is no longer full

    return true;
}

bool ring_buffer_pop(ring_buffer_t *buffer, void *value) {
    if (is_ring_buffer_empty(buffer) || buffer->tail == buffer->head) {
        return false;  // Buffer is empty, cannot dequeue
    }

    uint8_t *buffer_data = (uint8_t*) buffer->data;
    uint8_t *value_data = (uint8_t*) value;

    for (size_t i = 0; i < buffer->size; i++) {
        value_data[i] = buffer_data[(buffer->head * buffer->size + i) % (buffer->size * buffer->buffer_size)];
    }
    memset(&buffer_data[(buffer->head * buffer->size) % (buffer->size * buffer->buffer_size)], 0,
            buffer->size);
    if (buffer->head == 0) {
        buffer->head = buffer->buffer_size - 1;
    } else {
        buffer->head = (buffer->head - 1) % buffer->buffer_size;
    }
    buffer->count--;
    buffer->isFull = false;  // Buffer is no longer full

    return true;
}

uint8_t ring_buffer_peek(const ring_buffer_t *buffer, uint16_t offset, void *value) {
    if (is_ring_buffer_empty(buffer)) {
        return RING_BUFFER_EMPTY;
    }

    uint8_t *buffer_data = (uint8_t*) buffer->data;
    uint8_t *value_data = (uint8_t*) value;
    uint16_t peek = 0;
    uint16_t idx = 0;

    if (offset * buffer->size > buffer->head) {
        return RING_BUFFER_OFFSET_OUT_OF_BOUNDS;
    } else {
        peek = buffer->peek - (offset * buffer->size);
    }
    for (size_t i = 0; i < buffer->size; i++) {
        idx = (peek * buffer->size + i) % (buffer->size * buffer->buffer_size);
        value_data[i] = buffer_data[idx];
    }

    return RING_BUFFER_OK;
}

void ring_buffer_flush(ring_buffer_t *buffer) {
    memset(buffer->data, 0, buffer->size * buffer->buffer_size);
    buffer->count = 0;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->peek = 0;
    buffer->isFull = false;
}
