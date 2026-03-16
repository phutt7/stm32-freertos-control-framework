/*
 * ring_buffer.c
 *
 *  Created on: Mar 10, 2026
 *      Author: cyber
 */


#include "ring_buffer.h"

void RingBuffer_Init(RingBuffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

void RingBuffer_Write(RingBuffer_t *rb, uint16_t value)
{
    uint32_t next_head = (rb->head + 1) & (RING_BUFFER_SIZE - 1);

    if (next_head == rb->tail)
    {
        // Buffer full — overwrite oldest sample (real-time priority)
        rb->tail = (rb->tail + 1) & (RING_BUFFER_SIZE - 1);
    }

    rb->buffer[rb->head] = value;
    rb->head = next_head;
}

uint8_t RingBuffer_Read(RingBuffer_t *rb, uint16_t *value)
{
    if (rb->head == rb->tail)
        return 0;  // empty

    *value = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) & (RING_BUFFER_SIZE - 1);
    return 1;  // success
}

uint32_t RingBuffer_Count(RingBuffer_t *rb)
{
    return (rb->head - rb->tail) & (RING_BUFFER_SIZE - 1);
}
