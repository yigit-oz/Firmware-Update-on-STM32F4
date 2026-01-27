#include <libopencm3/stm32/flash.h>
#include "bootloader-flash.h"

#define MAIN_APP_SECTOR_START 2
#define MAIN_APP_SECTOR_END 11

void FlashEraseMainApplication(void) {
    flash_unlock();

    for(uint8_t sector = MAIN_APP_SECTOR_START; sector <= MAIN_APP_SECTOR_END; sector++) {
        flash_erase_sector(sector, FLASH_CR_PROGRAM_X32);        
    }

    flash_lock();
}

void FlashWrite(const uint32_t address, const uint8_t *data, const uint32_t length) {
    flash_unlock();

    flash_program(address, data, length);

    flash_lock();
}