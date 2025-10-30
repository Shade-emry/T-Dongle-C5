#ifndef __T_DONGLE_C5_BOARD_H__
#define __T_DONGLE_C5_BOARD_H__

#define BOARD_NAME "T-Dongle-C5"

#define SPI_MOSI 2
#define SPI_MISO 7
#define SPI_SCK  6

#define LCD_MOSI_PIN SPI_MOSI
#define LCD_CLK_PIN SPI_SCK
#define LCD_BL_PIN 0
#define LCD_RST_PIN 1
#define LCD_RS_PIN 3
#define LCD_CS_PIN 10

#define LED_CI_PIN SPI_SCK  //4  old error
#define LED_DI_PIN SPI_MOSI //5  old error   

#define SD_CMD_PIN SPI_MOSI
#define SD_DAT0_PIN SPI_MISO
#define SD_CLK_PIN SPI_SCK
#define SD_CS_PIN 23

#define UART0_TX_PIN 11
#define UART0_RX_PIN 12

#endif
