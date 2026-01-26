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

// Fills unused data bytes with 0xff
static void CommsFillUnusedDataBytes(CommsPacket_t *packet) {
    uint8_t DataLength = packet->length;
    
    for(uint8_t i = PACKET_DATA_LENGTH - 1; i >= DataLength; i--) {
        packet->data[i] = 0xff;
    }
}

static bool CommsIsSingleBytePacket(const CommsPacket_t *packet, uint8_t byte) {
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

static void CommsPacketCopy(const CommsPacket_t *source, CommsPacket_t *dest) {
    dest->length = source->length;
    for(uint8_t i = 0; i < PACKET_DATA_LENGTH; i++) {
        dest->data[i] = source->data[i];
    }
    dest->crc = source->crc;
}

void CommsSetup(void) {
    // Initialize retransmit packet
    RetransmitPacket.length = 1;
    RetransmitPacket.data[0] = PACKET_RETX_DATA0;
    CommsFillUnusedDataBytes(&RetransmitPacket);
    RetransmitPacket.crc = CommsComputeCrc(&RetransmitPacket);

    // Initialize ack packet
    AckPacket.length = 1;
    AckPacket.data[0] = PACKET_ACK_DATA0;
    CommsFillUnusedDataBytes(&AckPacket);
    AckPacket.crc = CommsComputeCrc(&AckPacket);
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

                CommsPacketCopy(&TemporaryPacket, &PacketBuffer[PacketWriteIndex]);
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
    CommsPacketCopy(packet, &LastTransmittedPacket);
}

void CommsRead(CommsPacket_t *packet) {
    CommsPacketCopy(&PacketBuffer[PacketReadIndex], packet);
    PacketReadIndex = (PacketReadIndex + 1) & PacketBufferMask;
}

uint8_t CommsComputeCrc(CommsPacket_t *packet) {
    return crc8((uint8_t*)packet, PACKET_LENGTH - PACKET_CRC_BYTES);
}