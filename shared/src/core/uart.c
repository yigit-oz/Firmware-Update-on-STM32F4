#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include "core/uart.h"

#define BAUD_RATE 115200
#define DATA_BITS 8

static uint8_t DataBuffer = 0U;
static bool DataAvailable = false;

void usart2_isr(void) {
    const bool OverrunOccured = usart_get_flag(USART2, USART_FLAG_ORE);
    const bool ReceivedData = usart_get_flag(USART2, USART_FLAG_RXNE);

    if(OverrunOccured || ReceivedData) {
        DataBuffer = (uint8_t)usart_recv(USART2);
        DataAvailable = true;
    }
}

void UartSetup(void) {
    rcc_periph_clock_enable(RCC_USART2);

    usart_set_mode(USART2, USART_MODE_TX_RX);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_set_databits(USART2, DATA_BITS);
    usart_set_baudrate(USART2, BAUD_RATE);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_stopbits(USART2, USART_STOPBITS_1);

    usart_enable_rx_interrupt(USART2);
    nvic_enable_irq(NVIC_USART2_IRQ);

    usart_enable(USART2);
}

void UartWrite(uint8_t data[], const uint32_t length) {
    for(uint32_t currByte = 0; currByte < length; currByte++) {
        UartWriteByte(data[currByte]);
    }

}
void UartWriteByte(uint8_t data) {
    usart_send_blocking(USART2, (uint16_t)data);
}

uint32_t UartRead(uint8_t *data, const uint32_t length) { // Returns how many bytes read
    if(length > 0 && DataAvailable) {
        *data = DataBuffer;
        DataAvailable = false;
        return 1; // 1 byte read
    }

    return 0;
}

uint8_t UartReadByte(void) {
    DataAvailable = false;
    return DataBuffer;
}

bool UartDataAvailable(void) {
    return DataAvailable;
}