/*
 * ring_buffer.h
 *
 *  Created on: Mar 3, 2024
 *      Author: Joshua Butler, MD, MHI
 *
 */

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_RING_BUFFER_SIZE 32

typedef struct {
    void *data;  // Pointer to the buffer data
    size_t size;  // Size of each element in the buffer
    uint16_t buffer_size; // Maximum number of elements in the buffer
    uint16_t count; // Number of elements in the buffer
    uint16_t head;  // Index of the head (write position)
    uint16_t tail;  // Index of the tail (read position)
    uint16_t peek;  // Index of the peek (read position)
    bool isFull;    // Flag indicating if the buffer is full
    bool new_data;  // Flag indicating if new data has been added to the buffer
    uint16_t num_overflows; // Number of times the buffer has overflowed
    uint16_t num_new_data;  // Number of new data added to the buffer
} ring_buffer_t;

enum {
    RING_BUFFER_OK, RING_BUFFER_ERROR, RING_BUFFER_MALLOC_FAILED, RING_BUFFER_EMPTY, RING_BUFFER_OFFSET_OUT_OF_BOUNDS
};

uint8_t ring_buffer_init(ring_buffer_t *buffer, uint16_t buffer_size, size_t data_size);
uint8_t ring_buffer_destroy(ring_buffer_t *buffer);
bool is_ring_buffer_empty(const ring_buffer_t *buffer);
bool is_ring_buffer_full(const ring_buffer_t *buffer);
bool ring_buffer_enqueue(ring_buffer_t *buffer, const void *value);
bool ring_buffer_dequeue(ring_buffer_t *buffer, void *value);
bool ring_buffer_pop(ring_buffer_t *buffer, void *value);
uint8_t ring_buffer_peek(const ring_buffer_t *buffer, uint16_t offset, void *value);
void ring_buffer_flush(ring_buffer_t *buffer);

#endif /* RING_BUFFER_H_ */
