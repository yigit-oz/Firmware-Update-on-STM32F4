#ifndef INC_UART_H
#define INC_UART_H

#include "common-defines.h"

void UartSetup(void);
void UartWrite(uint8_t data[], const uint32_t length);
void UartWriteByte(uint8_t data);
uint32_t UartRead(uint8_t *data, const uint32_t length); // Returns how many bytes read
uint8_t UartReadByte(void);
bool UartDataAvailable(void);

#endif