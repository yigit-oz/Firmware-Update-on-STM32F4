# Firmware Update via UART Bootloader
This project uses libopencm3 library. It consists of a simple UART based firmware update mechanism for an STM32F4 microcontroller. A custom communication protocol was implemented for reliable  
data transmission. The bootloader receives firmware packets over UART, verifies them using CRC, erases the existing application, and programs the new firmware into 
flash memory.

Place your .bin update file into the Firmware-Update-on-STM32F4 folder and rename it as "firmware.bin" to update the firmware with that file.

## Update Flow
Device starts in bootloader. (Sector 0 and 1!) For 5 seconds, it waits for a BL_STATE_SYNC message from the uart for starting the update, otherwise jumps into the main application's reset vector.

Updater program "firmware-updater" should be started when the device is in bootloader mode.

When updater runs, it tries to sync with the microcontroller by sending sequences SYNC_SEQ_0, SYNC_SEQ_1, SYNC_SEQ_2 and SYNC_SEQ_3. If it receives BL_PACKET_SYNC_OBSERVED_DATA0, they go to the next step. Otherwise, wait for 1 second and send them again.

Send firmware update request packet BL_PACKET_FW_UPDATE_REQ_DATA0 to the chip and wait for the BL_PACKET_FW_UPDATE_RES_DATA0. This can be used in the future for learning information about the firmware inside and taking action accordingly.

Chip sends device id request BL_PACKET_DEVICE_ID_REQ_DATA0, and updater will respond with the correct chip id. This can be used for addressing multiple devices and updating the wanted one.

Chip sends firmware length request BL_PACKET_FW_LENGTH_REQ_DATA0, which will be responded to by BL_PACKET_FW_LENGTH_RES_DATA0. If the length of the firmware exceeds the maximum capacity MAX_FW_LENGTH, it will send a BL_PACKET_NACK_DATA0 and communication will end.

Chip erases the main application which starts from sector 2.

Chip sends ready for data packet BL_PACKET_READY_FOR_DATA_DATA0, writes the data on the flash. Loop until BytesWritten == FwLength.

Send firmware update successful packet BL_PACKET_UPDATE_SUCCESSFUL_DATA0.

## Communication Protocol
A single data packet consists of 3 fields: length, data and crc. First byte is length, next 16 byte is data and the last one is crc. Length says how many data bytes
are inside the packet. Unused data bytes are transmitted with the value 0xff. Unused bytes are also considered for the crc application as well as other bytes. If the
received packet's crc is not equal to the newly calculated one, that device (updater or chip) sends PACKET_RETX_DATA0, which requests retransmission of the packet. If the crc
is good, send PACKET_ACK_DATA0.

## How to use
1. Flash the bootloader into your MCU.
2. Place your update file inside the FirmwareUpdate folder and rename it as firmware.bin
3. Press the reset button and run the firmware updater.

## Output
<img width="545" height="445" alt="image" src="https://github.com/user-attachments/assets/57bb014a-51d3-443e-8b5c-e19fed84b3cd" />
<img width="499" height="259" alt="image" src="https://github.com/user-attachments/assets/83e63ad6-355a-492b-9ba4-47f7d587bb5e" />

