#pragma once

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

#define LED_CI_PIN SPI_SCK  
#define LED_DI_PIN SPI_MOSI   

#define SD_CMD_PIN SPI_MOSI
#define SD_DAT0_PIN SPI_MISO
#define SD_CLK_PIN SPI_SCK
#define SD_CS_PIN 23

#define UART0_TX_PIN 11
#define UART0_RX_PIN 12

// #define SD_MMC_D0_PIN  14
// #define SD_MMC_D1_PIN  17
// #define SD_MMC_D2_PIN  21
// #define SD_MMC_D3_PIN  18
// #define SD_MMC_CLK_PIN 12
// #define SD_MMC_CMD_PIN 16