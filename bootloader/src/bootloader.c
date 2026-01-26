#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>

#include "core/uart.h"
#include "core/system.h"
#include "comms.h"

#define BOOTLOADER_SIZE (0x8000) // Located in sector 0 and 1
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE) // Starts from sector 3

#define UART_PORT         (GPIOA)
#define UART_TX_PIN       (GPIO2)
#define UART_RX_PIN       (GPIO3)

static void GpioSetup(void) {
  rcc_periph_clock_enable(RCC_GPIOA);
  gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, UART_TX_PIN | UART_RX_PIN);
  gpio_set_af(UART_PORT, GPIO_AF7, UART_TX_PIN | UART_RX_PIN);
}

static void jump_to_main_application(void) {
  vector_table_t* main_vector_table = (vector_table_t*)(MAIN_APP_START_ADDRESS);
  main_vector_table->reset();
}

int main(void) {
  system_setup();
  GpioSetup();
  UartSetup();
  CommsSetup();

  CommsPacket_t packet = {
    .length = 9,
    .data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .crc = 0
  };
  packet.crc = CommsComputeCrc(&packet);

  while(true) {
    CommsUpdate();

    CommsPacket_t rx_packet;

    if(CommsPacketsAvailable()) {
      CommsRead(&rx_packet);
    }

    CommsWrite(&packet);
    system_delay(500);
  }

  // Todo: Reset the system before jumping to main application
  
  jump_to_main_application();

  return 0;
}