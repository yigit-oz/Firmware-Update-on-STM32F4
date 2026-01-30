#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>

#include "core/uart.h"
#include "core/system.h"
#include "core/simple-timer.h"
#include "comms.h"
#include "bootloader-flash.h"

#define BOOTLOADER_SIZE (0x8000U) // Located in sector 0 and 1
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE) // Starts from sector 3
#define MAX_FW_LENGTH (1024U * 1024U) - (BOOTLOADER_SIZE)

#define UART_PORT         (GPIOA)
#define UART_TX_PIN       (GPIO2)
#define UART_RX_PIN       (GPIO3)

#define DEVICE_ID 0x49

#define SYNC_SEQ_0 0xC4 
#define SYNC_SEQ_1 0xA9 
#define SYNC_SEQ_2 0x6F
#define SYNC_SEQ_3 0x4B

#define DEFAULT_TIMEOUT 5000

typedef enum BlState_t {
  BL_STATE_SYNC,
  BL_STATE_WAIT_FOR_UPDATE_REQ,
  BL_STATE_DEVICE_ID_REQ,
  BL_STATE_DEVICE_ID_RES,
  BL_STATE_FW_LENGTH_REQ,
  BL_STATE_FW_LENGTH_RES,
  BL_STATE_ERASE_APPLICATION,
  BL_STATE_RECEIVE_FIRMWARE,
  BL_STATE_DONE
} BlState_t;

static BlState_t state = BL_STATE_SYNC;
static uint32_t FwLength = 0;
static uint32_t BytesWritten = 0;
static uint8_t SyncSeq[4] = {0};
static simple_timer_t timer;
static CommsPacket_t TempPacket;

static void GpioSetup(void) {
  rcc_periph_clock_enable(RCC_GPIOA);
  gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, UART_TX_PIN | UART_RX_PIN);
  gpio_set_af(UART_PORT, GPIO_AF7, UART_TX_PIN | UART_RX_PIN);
}

static void GpioTeardown(void) {
  gpio_mode_setup(UART_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, UART_TX_PIN | UART_RX_PIN);
  rcc_periph_clock_disable(RCC_GPIOA);
}

static void jump_to_main_application(void) {
  vector_table_t* main_vector_table = (vector_table_t*)MAIN_APP_START_ADDRESS;
  main_vector_table->reset();
}

static void BootloadingFailed(void) {
  CommsCreateSingleBytePacket(&TempPacket, BL_PACKET_NACK_DATA0);
  CommsWrite(&TempPacket);
  state = BL_STATE_DONE;
}

static void CheckForTimeout(void) {
  if(SimpleTimerHasElapsed(&timer)) {
    BootloadingFailed();
  }
}

static bool IsDeviceIdPacket(const CommsPacket_t *packet) {
  if(packet->length != 2) {
    return false;
  }
    
  if(packet->data[0] != BL_PACKET_DEVICE_ID_RES_DATA0) {
    return false;
  }

  for(uint8_t i = 2; i < PACKET_DATA_LENGTH; i++) {
    if(packet->data[i] != 0xff) {
      return false;
    }
  }

  return true;
}

static bool IsFwLengthPacket(const CommsPacket_t *packet) {
  if(packet->length != 5) {
    return false;
  }
    
  if(packet->data[0] != BL_PACKET_FW_LENGTH_RES_DATA0) {
    return false;
  }

  for(uint8_t i = 5; i < PACKET_DATA_LENGTH; i++) {
    if(packet->data[i] != 0xff) {
      return false;
    }
  }

  return true;
}

int main(void) {
  system_setup();
  GpioSetup();
  UartSetup();
  CommsSetup();

  SimpleTimerSetup(&timer, DEFAULT_TIMEOUT, false);

  while(state != BL_STATE_DONE) {
    if(state == BL_STATE_SYNC) {

      if(UartDataAvailable()){
        SyncSeq[0] = SyncSeq[1];
        SyncSeq[1] = SyncSeq[2];
        SyncSeq[2] = SyncSeq[3];
        SyncSeq[3] = UartReadByte();

        bool IsMatched = (SyncSeq[0] == SYNC_SEQ_0) && (SyncSeq[1] == SYNC_SEQ_1) && (SyncSeq[2] == SYNC_SEQ_2) && (SyncSeq[3] == SYNC_SEQ_3);

        if(IsMatched) {
          CommsCreateSingleBytePacket(&TempPacket, BL_PACKET_SYNC_OBSERVED_DATA0);
          CommsWrite(&TempPacket);
          SimpleTimerReset(&timer);
          state = BL_STATE_WAIT_FOR_UPDATE_REQ;
        }
        else {
          CheckForTimeout();
        }
      }
      else {
        CheckForTimeout();
      }

      continue;
    }

    CommsUpdate();

    switch(state) {
      case BL_STATE_WAIT_FOR_UPDATE_REQ: {
        if(CommsPacketsAvailable()) {
          CommsRead(&TempPacket);

          if(CommsIsSingleBytePacket(&TempPacket, BL_PACKET_FW_UPDATE_REQ_DATA0)) {
            SimpleTimerReset(&timer);
            CommsCreateSingleBytePacket(&TempPacket, BL_PACKET_FW_UPDATE_RES_DATA0);
            CommsWrite(&TempPacket);
            state = BL_STATE_DEVICE_ID_REQ;
          }
          else {
            BootloadingFailed();
          }
        }
        else {
          CheckForTimeout();
        }
      } break;

      case BL_STATE_DEVICE_ID_REQ: {
        //system_delay(15);
        SimpleTimerReset(&timer);
        CommsCreateSingleBytePacket(&TempPacket, BL_PACKET_DEVICE_ID_REQ_DATA0);
        CommsWrite(&TempPacket);
        state = BL_STATE_DEVICE_ID_RES;
      } break;

      case BL_STATE_DEVICE_ID_RES: {
        if(CommsPacketsAvailable()) {
          CommsRead(&TempPacket);

          if(IsDeviceIdPacket(&TempPacket) && (TempPacket.data[1] == DEVICE_ID)) {
            SimpleTimerReset(&timer);
            state = BL_STATE_FW_LENGTH_REQ;
          }
          else {
            BootloadingFailed();
          }
        }
        else {
          CheckForTimeout();
        }
      } break;

      case BL_STATE_FW_LENGTH_REQ: {
        SimpleTimerReset(&timer);
        CommsCreateSingleBytePacket(&TempPacket, BL_PACKET_FW_LENGTH_REQ_DATA0);
        CommsWrite(&TempPacket);
        state = BL_STATE_FW_LENGTH_RES;
      } break;

      case BL_STATE_FW_LENGTH_RES: {
        if(CommsPacketsAvailable()) {
          CommsRead(&TempPacket);

          // Read as little endian
          FwLength = (
            (TempPacket.data[1])       |
            (TempPacket.data[2] << 8)  |
            (TempPacket.data[3] << 16) |
            (TempPacket.data[4] << 24)
          );

          if(IsFwLengthPacket(&TempPacket) && (FwLength <= MAX_FW_LENGTH)) {
            state = BL_STATE_ERASE_APPLICATION;
          }
          else {
            BootloadingFailed();
          }
        }
        else {
          CheckForTimeout();
        }
      } break;

      case BL_STATE_ERASE_APPLICATION: {
        FlashEraseMainApplication();
        CommsCreateSingleBytePacket(&TempPacket, BL_PACKET_READY_FOR_DATA_DATA0);
        CommsWrite(&TempPacket);
        SimpleTimerReset(&timer);
        state = BL_STATE_RECEIVE_FIRMWARE;
      } break;

      case BL_STATE_RECEIVE_FIRMWARE: {
        if(CommsPacketsAvailable()) {
          CommsRead(&TempPacket);

          const uint8_t PacketLength = (TempPacket.length & 0x0f) + 1;
          FlashWrite(MAIN_APP_START_ADDRESS + BytesWritten, TempPacket.data, PacketLength);
          BytesWritten += PacketLength;
          SimpleTimerReset(&timer);

          if(BytesWritten >= FwLength) {
            CommsCreateSingleBytePacket(&TempPacket, BL_PACKET_UPDATE_SUCCESSFUL_DATA0);
            CommsWrite(&TempPacket);
            state = BL_STATE_DONE;
          }
          else {
            CommsCreateSingleBytePacket(&TempPacket, BL_PACKET_READY_FOR_DATA_DATA0);
            CommsWrite(&TempPacket);
          }
        }
        else {
          CheckForTimeout();
        }
      } break;

      default: {
        state = BL_STATE_SYNC;
      }
    }

  }

  system_delay(150);
  UartTeardown();
  GpioTeardown();
  system_teardown();
  
  jump_to_main_application();

  return 0;
}