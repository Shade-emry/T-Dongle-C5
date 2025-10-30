#include <Arduino.h>
#include "pin_config.h"
#include "SPI.h"

#define APA102_LED_NUMBERS 1

void set_apa102_config(uint8_t brightness, uint32_t color);
void send_apa102_data(uint8_t brightness, uint8_t red, uint8_t green, uint8_t blue);

void setup() {
  delay(500);  // power-up safety delay
  Serial.begin(115200);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  delay(1000);  // Wait 8 milliseconds until the next frame.
  Serial.println("Hello ESP32C5!");
  set_apa102_config(31, 0x000000);
}

void loop() {
  Serial.println("RED LED!");
  set_apa102_config(31, 0xFF0000);
  delay(2000);
  Serial.println("GREEN LED!");
  set_apa102_config(31, 0x00FF00);
  delay(2000);
  Serial.println("BLUE LED!");
  set_apa102_config(31, 0x0000FF);
  delay(2000);
}

void set_apa102_config(uint8_t brightness, uint32_t color) {
  uint8_t red = (color >> 16) & 0xFF;
  uint8_t green = (color >> 8) & 0xFF;
  uint8_t blue = color & 0xFF;

  send_apa102_data(brightness, red, green, blue);
}

void send_apa102_data(uint8_t brightness, uint8_t red, uint8_t green, uint8_t blue) {
  size_t buffer_size = (APA102_LED_NUMBERS * 4) + 8;  // 起始帧 + LED 数据帧 + 结束帧
  uint8_t buffer[buffer_size];

  // 起始帧（4 字节，全为 0）
  memset(buffer, 0, 4);

  // 填充 LED 数据帧
  for (int i = 0; i < APA102_LED_NUMBERS; i++) {
    buffer[4 + (i * 4)] = 0b11100000 | (brightness & 0x1F);  // 亮度
    buffer[4 + (i * 4) + 1] = blue;                          // 蓝色
    buffer[4 + (i * 4) + 2] = green;                         // 绿色
    buffer[4 + (i * 4) + 3] = red;                           // 红色
  }

  // 结束帧（至少 LED 数量 / 2 字节，全为 0）
  memset(buffer + 4 + (APA102_LED_NUMBERS * 4), 0, 4);
  // for (int i = 0; i < buffer_size; i++) {
  //   Serial.printf("buffer[%d]:%d\n", i, buffer[i]);
  // }

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(buffer, buffer_size);
  SPI.endTransaction();
}
