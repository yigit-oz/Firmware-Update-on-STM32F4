#ifndef INC_BOOTLOADER_FLASH_H
#define INC_BOOTLOADER_FLASH_H

#include "common-defines.h"

void FlashEraseMainApplication(void);
void FlashWrite(const uint32_t address, const uint8_t *data, const uint32_t length);

#endif