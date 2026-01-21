#include "common-defines.h"
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>

#define BOOTLOADER_SIZE (0x8000) // Located in sector 0 and 1
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE) // Starts from sector 3

static void jump_to_main_application(void) {
  vector_table_t* main_vector_table = (vector_table_t*)(MAIN_APP_START_ADDRESS);
  main_vector_table->reset();
}

int main(void) {
  jump_to_main_application();

  return 0;
}