#ifndef INC_RING_BUFFER_H
#define INC_RING_BUFFER_H

#include "common-defines.h"

typedef struct RingBuffer_t {
    uint8_t *buffer;
    uint32_t mask;
    uint32_t ReadIndex;
    uint32_t WriteIndex;

} RingBuffer_t;

void RingBufferSetup(RingBuffer_t *rb, uint8_t *buffer, uint32_t size); // Size has to be a power of 2!
bool RingBufferEmpty(RingBuffer_t *rb);
bool RingBufferWrite(RingBuffer_t *rb, uint8_t byte);
bool RingBufferRead(RingBuffer_t *rb, uint8_t *byte);

#endif