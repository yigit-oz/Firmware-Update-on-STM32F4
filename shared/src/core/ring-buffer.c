#include "core/ring-buffer.h"

void RingBufferSetup(RingBuffer_t *rb, uint8_t *buffer, uint32_t size) {
    rb->buffer = buffer;
    rb->ReadIndex = 0;
    rb->WriteIndex = 0;
    rb->mask = size - 1;
}

bool RingBufferEmpty(RingBuffer_t *rb) {
    return rb->ReadIndex == rb->WriteIndex;
}

bool RingBufferWrite(RingBuffer_t *rb, uint8_t byte) {
    uint32_t LocalReadIndex = rb->ReadIndex;
    uint32_t LocalWriteIndex = rb->WriteIndex;

    uint32_t NextWriteIndex = (LocalWriteIndex + 1) & rb->mask;

    if(NextWriteIndex == LocalReadIndex) {
        return false;
    }

    rb->buffer[LocalWriteIndex] = byte;
    rb->WriteIndex = NextWriteIndex;

    return true;
}

bool RingBufferRead(RingBuffer_t *rb, uint8_t *byte) {
    uint32_t LocalReadIndex = rb->ReadIndex;
    uint32_t LocalWriteIndex = rb->WriteIndex;

    if(LocalReadIndex == LocalWriteIndex) {
        return false;
    }

    *byte = rb->buffer[LocalReadIndex];
    LocalReadIndex = (LocalReadIndex + 1) & rb->mask;
    rb->ReadIndex = LocalReadIndex;
    
    return true;
}
