#ifndef INC_COMMS_H
#define INC_COMMS_H

#include "common-defines.h"

#define PACKET_DATA_LENGTH 16
#define PACKET_LENGTH_BYTES 1
#define PACKET_CRC_BYTES 1
#define PACKET_LENGTH (PACKET_DATA_LENGTH + PACKET_LENGTH_BYTES + PACKET_CRC_BYTES)

#define PACKET_RETX_DATA0 0x19
#define PACKET_ACK_DATA0 0x15

typedef struct CommsPacket_t {
    uint8_t length;
    uint8_t data[PACKET_DATA_LENGTH];
    uint8_t crc;
} CommsPacket_t;

void CommsSetup(void);
void CommsUpdate(void);
bool CommsPacketsAvailable(void);
void CommsWrite(CommsPacket_t *packet);
void CommsRead(CommsPacket_t *packet);
uint8_t CommsComputeCrc(CommsPacket_t *packet);

#endif 