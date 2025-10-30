# 📡 T-Dongle-C5 Smart Device Firmware

## 📋 Project Overview
This project is a firmware for the T-Dongle-C5 development board based on the ESP32-C5 chip, integrating Wi-Fi, Bluetooth, LCD display, SD card, and LED strip functionalities. Key features include:

- **🌐 Network Features**: Wi-Fi STA/AP mode support with built-in TCP server
- **🖥️ Display Features**: 80x160 resolution ST7735 LCD screen with LVGL graphics interface
- **💡 Peripheral Control**: APA102 LED strip with PWM backlight adjustment
- **💾 Storage Extension**: SD card module support (via SPI interface)
- **🎮 Human Interaction**: Button control and TCP command parsing

Enables communication with computers, mobile phones, and other development boards through TCP/IP protocol for remote control, data transmission, serial printing, and data storage. Currently, SD card data storage functionality needs to be implemented according to specific data format requirements.

## 🚠 User Guide
It can be tested in combination with the `TCP_Wireless_Send` routine. Two T-Dongle-C5 boards are required. If there is only one T-Dongle-C5 board.The client can independently write the TCP client program based on the MCU.

TCP command: BRIGHTNESS_20 // Set the backlight brightness to 20

## ⚙️ Prerequisites
### Hardware Requirements
- T-Dongle-C5 development board
- MicroSD card (optional)

### Software Requirements
- ESP-IDF v5.5 or higher
- CMake build tools
- Serial terminal tool (e.g., PuTTY, minicom)

| software   | supported | version        |
| ---------- | --------- | -------------- |
| ESP-IDF    | ✅         | v5.5 or higher |
| PlatfromIO | ❌         |                |
| Arduino    | ❌         |                |

## 🚀 Getting Started

### 1. Hardware Connection
- Connect the development board to your computer via USB
- Optional: Insert FAT32-formatted MicroSD card

### 2. Wi-Fi Configuration
- STA Mode: Modify `WIFI_SSID` and `WIFI_PASSWORD` in the code
- AP Mode: Default hotspot name "T-Dongle-C5", password "88888888"

### 3. Firmware Flashing
- **A. VSCode IDF Plugin (Recommended)**:
  1. Open the directory under the examples folder, select a routine, and right-click to open it through vscode, or open the routines folder under examples in vscode
  2. Set `esp32c5` target chip and `serial port`
  3. Configure `Wi-Fi` mode(STA/AP)
  4. Click `"Build & Flash & Monitor"` button
  5. Wait for compilation and flashing completion
<img src="./image/idf_flash_download.png" alt="VSCode Build & Flash & Monitor">

- **B. IDF Command Line**:
  1. Open terminal, Go to the routines folder under examples
  2. Run `idf.py set-target esp32c5` to set target chip
  3. Configure `Wi-Fi` mode(STA/AP)
  4. Run `idf.py build` to compile firmware
  5. Run `idf.py -p PORT flash` to flash firmware (PORT = development board's serial port)

### 4.FAQ
- **Q: How to use the TCP server?**
- AP mode:
After successfully setting the Wi-Fi information, the hotspot created by the development board is named 't-donle-C5', and the password is' 88888888 '. Will automatically start the TCP server and wait for the client to connect. The default port number is 8080. The other side needs to connect to the hotspot of the development board according to the prompts, and input the IP address and port number to establish a connection and communicate.
- STA mode:
After successfully setting the Wi-Fi information, the development board will automatically connect to the specified Wi-Fi network. When the connection is successful, the TCP server is automatically started and waits for the client to connect. The default port number is 8080. The other side needs to enter the IP address and port number according to the prompt, establish a connection, and then communicate.