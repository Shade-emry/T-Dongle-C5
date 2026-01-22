# T-Dongle-C5 Factory Firmware

## Project Overview
This project is a factory firmware for the T-Dongle-C5, designed to test and verify the functionality and performance of the T-Dongle-C5.

## Instructions
This project includes the following features:
- Wi-Fi Connection: Connects to a specified Wi-Fi network and outputs RSSI values to the screen.
- Serial Communication: Communicates with a computer via serial port.
- Button Detection: Button toggles the screen backlight.
- LED Control: LED blinking.
- SD Card: SD card read test.

## Prerequisites
### Hardware Requirements
- T-Dongle-C5 Development Board

### Software Requirements
- ESP-IDF v5.5 or higher
- CMake build tool
- Serial terminal tool

| Software Environment | Support | Version           |
| -------------------- | ------- | ----------------- |
| ESP-IDF              | ✅      | v5.5 or higher    |

## Running Steps
1. **Connect Hardware**
   - Connect the development board to the computer via USB.

2. **Configure Wi-Fi**
   - Modify `WIFI_SSID` and `WIFI_PASSWORD` in the code.

3. **Flash Firmware**
   #### A. Vscode IDF Plugin (Recommended):
   1. Open the directory under the examples folder, right-click to open with Vscode, or open the example project folder in Vscode.
   2. Set the target chip to esp32c5 and the serial port.
   3. Click the "Build & Flash" button in the lower left corner.
   4. Wait for the compilation to complete and the firmware to be flashed.

   #### B. Using Espressif Flash Download Tools (v3.9.8 or higher):
   1. Select the esp32c5 chip.
   2. Select USB LoadMode.
   3. Select the firmware file.
      ![alt text](image\image.png)
   4. Click "START" to begin flashing.

   #### C. IDF:
   1. Open the terminal and navigate to the ESP-IDF root directory.
   2. Enter `idf.py set-target esp32c5` to set the target chip.
   3. Follow the prompts to set Wi-Fi information.
   4. Enter `idf.py build` to build the firmware.
   5. Enter `idf.py -p PORT flash` to flash the firmware (where PORT is the serial port connected to the development board).
