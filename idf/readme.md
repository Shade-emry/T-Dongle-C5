<h1 align = "center">🎯LilyGo T-Dongle-S3 🌐</h1>

## 🌈Introduction
| board       | MCU      | WIFI   | BLE     | Display  storage  | Flash | RGB  | LED    | Button | USB   |
| ----------- | -------- | ------ | ------- | ----------------- | ----- | ---- | ------ | ------ | ----- |
| T-Dongle-C5 | ESP32-C5 | WIFI 6 | BLE 5.0 | 80*160(0.96 inch) | SD    | 16MB | APA102 | Boot   | USB-A |

### Features


## 🛵Directory
```
T-Dongle-C5
├── Examples
|   ├── BLE_Connect : BLE connection demo
|   ├── SD_LED     : SD card and LED demo
|   ├── SPI_LCD     : SPI LCD demo
|   ├── T-Dongle-C5 : T-Dongle-C5 Factory Exampel
|   ├── WiFi_AP     : WiFi AP demo
|   ├── WiFI_STA    : WiFi STA demo
├── Hardware
│   ├── T-Dongle-C5.pdf : T-Dongle-C5 hardware design
├── firmware ：T-Dongle-C5 firmware
├── shcematic ：ESP32C5 datasheet and manual
|   
├── readme.md
```

## 🚩Pinout Diagram
| GPIO    | LCD      | LED    | SD     | UART     | Button |
| ------- | -------- | ------ | ------ | -------- | ------ |
| GPIO_2  | LCD_MOSI | LED_DI | SD_CMD |          |        |
| GPIO_7  | LCD_MISO | LED_CI | SD_D0  |          |        |
| GPIO_6  | LCD_SCK  |        | SD_CLK |          |        |
| GPIO_10 | LCD_CS   |        |        |          |        |
| GPIO_3  | LCD_RS   |        |        |          |        |
| GPIO_0  | LCD_BL   |        |        |          |        |
| GPIO_1  | LCD_RST  |        |        |          |        |
| GPIO_23 |          |        | SD_CS  |          |        |
| GPIO_28 |          |        |        |          | BOOT   |
| GPIO_11 |          |        |        | UART0_TX |        |
| GPIO_12 |          |        |        | UART0_RX |        |

## ⚙️Quick Start
- The IDF installation method is also inconsistent depending on the system, it is recommended to refer to the [official manual](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) for installation

| software   | supported | version                 |
| ---------- | --------- | ----------------------- |
| ESP-IDF    | ✅         | v5.4 or higher          |
| PlatfromIO | ❌         | Not currently supported |
| Arduino    | ❌         | Not currently supported |

## 🚀FAQ 

