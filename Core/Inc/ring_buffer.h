/*
 * ring_buffer.h
 *
 *  Created on: Mar 10, 2026
 *      Author: cyber
 */


#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stddef.h>

#define RING_BUFFER_SIZE 64  // must be power of 2

typedef struct {
    uint16_t buffer[RING_BUFFER_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} RingBuffer_t;

void     RingBuffer_Init  (RingBuffer_t *rb);
void     RingBuffer_Write (RingBuffer_t *rb, uint16_t value);  // called from ISR
uint8_t  RingBuffer_Read  (RingBuffer_t *rb, uint16_t *value); // called from Task A
uint32_t RingBuffer_Count (RingBuffer_t *rb);

#endif
