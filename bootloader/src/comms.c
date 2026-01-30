#include <string.h>
#include "comms.h"
#include "core/uart.h"
#include "core/crc8.h"

#define PACKET_BUFFER_LENGTH 8

typedef enum CommsState_t {
    COMMS_STATE_LENGTH, 
    COMMS_STATE_DATA, 
    COMMS_STATE_CRC
} CommsState_t;

static CommsState_t state = COMMS_STATE_LENGTH;
static uint8_t DataByteCount = 0;

static CommsPacket_t TemporaryPacket = {.length = 0, .data = {0}, .crc = 0};
static CommsPacket_t RetransmitPacket = {.length = 0, .data = {0}, .crc = 0};
static CommsPacket_t AckPacket = {.length = 0, .data = {0}, .crc = 0};
static CommsPacket_t LastTransmittedPacket = {.length = 0, .data = {0}, .crc = 0};

static CommsPacket_t PacketBuffer[PACKET_BUFFER_LENGTH];
static uint32_t PacketReadIndex = 0;
static uint32_t PacketWriteIndex = 0;
static uint32_t PacketBufferMask = PACKET_BUFFER_LENGTH - 1;

bool CommsIsSingleBytePacket(const CommsPacket_t *packet, uint8_t byte) {
    if(packet->length != 1) {
        return false;
    }
    
    if(packet->data[0] != byte) {
        return false;
    }

    for(uint8_t i = 1; i < PACKET_DATA_LENGTH; i++) {
        if(packet->data[i] != 0xff) {
            return false;
        }
    }

    return true;
}

void CommsCreateSingleBytePacket(CommsPacket_t *packet, uint8_t byte) {
    memset(packet, 0xff, sizeof(CommsPacket_t));
    packet->length = 1;
    packet->data[0] = byte;
    packet->crc = CommsComputeCrc(packet);
}

void CommsSetup(void) {
    // Initialize retransmit packet
    CommsCreateSingleBytePacket(&RetransmitPacket, PACKET_RETX_DATA0);
    // Initialize ack packet
    CommsCreateSingleBytePacket(&AckPacket, PACKET_ACK_DATA0);
} 

// Pcaket state machine
void CommsUpdate(void) {
    while(UartDataAvailable()) {
        switch(state) {
            case COMMS_STATE_LENGTH: {
                TemporaryPacket.length = UartReadByte();
                state = COMMS_STATE_DATA; 
            } break;

            case COMMS_STATE_DATA: {
                TemporaryPacket.data[DataByteCount++] = UartReadByte();
                if(DataByteCount >= PACKET_DATA_LENGTH) {
                    DataByteCount = 0;
                    state = COMMS_STATE_CRC;
                }  
            } break;

            case COMMS_STATE_CRC: {
                TemporaryPacket.crc = UartReadByte();

                if(TemporaryPacket.crc != CommsComputeCrc(&TemporaryPacket)) {
                    CommsWrite(&RetransmitPacket);
                    state = COMMS_STATE_LENGTH;
                    break;
                }

                if(CommsIsSingleBytePacket(&TemporaryPacket, PACKET_RETX_DATA0)) {
                    CommsWrite(&LastTransmittedPacket);
                    state = COMMS_STATE_LENGTH;
                    break;
                }

                if(CommsIsSingleBytePacket(&TemporaryPacket, PACKET_ACK_DATA0)) {
                    state = COMMS_STATE_LENGTH;
                    break;
                }

                uint32_t NextWriteIndex = (PacketWriteIndex + 1) & PacketBufferMask;
                if(NextWriteIndex == PacketReadIndex) {
                    __asm__("BKPT #0"); // A breakpoint in case of a bufferoverflow
                }

                memcpy(&PacketBuffer[PacketWriteIndex], &TemporaryPacket, sizeof(CommsPacket_t));
                PacketWriteIndex = NextWriteIndex;
                CommsWrite(&AckPacket);
                state = COMMS_STATE_LENGTH;

            } break;

            default: {
                state = COMMS_STATE_LENGTH;
            }
        }
    }
}

bool CommsPacketsAvailable(void) {
    return PacketReadIndex != PacketWriteIndex;
}

void CommsWrite(CommsPacket_t *packet) {
    UartWrite((uint8_t*)packet, PACKET_LENGTH);
    memcpy(&LastTransmittedPacket, packet, sizeof(CommsPacket_t));
}

void CommsRead(CommsPacket_t *packet) {
    memcpy(packet, &PacketBuffer[PacketReadIndex], sizeof(CommsPacket_t));
    PacketReadIndex = (PacketReadIndex + 1) & PacketBufferMask;
}

uint8_t CommsComputeCrc(CommsPacket_t *packet) {
    return crc8((uint8_t*)packet, PACKET_LENGTH - PACKET_CRC_BYTES);
}