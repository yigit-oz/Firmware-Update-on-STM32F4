#ifndef INC_COMMS_H
#define INC_COMMS_H

#include "common-defines.h"

#define PACKET_DATA_LENGTH  16
#define PACKET_LENGTH_BYTES 1
#define PACKET_CRC_BYTES    1
#define PACKET_LENGTH       (PACKET_DATA_LENGTH + PACKET_LENGTH_BYTES + PACKET_CRC_BYTES)

#define PACKET_RETX_DATA0   0x11
#define PACKET_ACK_DATA0    0x1F

#define BL_PACKET_SYNC_OBSERVED_DATA0      0x17
#define BL_PACKET_FW_UPDATE_REQ_DATA0      0x25
#define BL_PACKET_FW_UPDATE_RES_DATA0      0x28
#define BL_PACKET_DEVICE_ID_REQ_DATA0      0x30
#define BL_PACKET_DEVICE_ID_RES_DATA0      0x3F
#define BL_PACKET_FW_LENGTH_REQ_DATA0      0x43
#define BL_PACKET_FW_LENGTH_RES_DATA0      0x4F
#define BL_PACKET_READY_FOR_DATA_DATA0     0x53
#define BL_PACKET_UPDATE_SUCCESSFUL_DATA0  0x63
#define BL_PACKET_NACK_DATA0               0x69

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
bool CommsIsSingleBytePacket(const CommsPacket_t *packet, uint8_t byte);
void CommsCreateSingleBytePacket(CommsPacket_t *packet, uint8_t byte);

#endif 