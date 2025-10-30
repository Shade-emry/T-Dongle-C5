/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include <string.h>
#include "esp_vfs_fat.h"
#include "t-dongle-c5-board.h"
#include "sdmmc_cmd.h"

static const char *TAG = "sd_led_example";
#define SPI_HOST SPI2_HOST

static spi_device_handle_t led_strip;
#define APA102_MOSI_PIN LED_DI_PIN             // MOSI 引脚
#define APA102_SCLK_PIN LED_CI_PIN             // SCLK 引脚
#define APA102_LED_NUMBERS 1                   // APA102 LED 的数量
#define APA102_SPI_CLOCK_HZ (10 * 1000 * 1000) // SPI 时钟速度，10 MHz

void spi_init(void)
{
    spi_bus_config_t buscfg = {
        .sclk_io_num = SPI_SCK,
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = SPI_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
}


static void sd_init(void)
{
    esp_err_t ret;
    spi_device_interface_config_t sd_cfg = {
        .clock_speed_hz = 5 * 1000 * 1000, // SD 卡 SPI 时钟速度
        .mode = 0,                         // SPI 模式
        .spics_io_num = SD_CS_PIN,         // SD 卡 CS 引脚
        .queue_size = 2,                   // 传输队列大小
    };
    spi_device_handle_t sd_handle;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST, &sd_cfg, &sd_handle));

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = SPI_HOST;

    const char mount_point[] = "/sdcard";
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
        .allocation_unit_size = 8 * 1024,
    };

    sdmmc_card_t *card;
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount filesystem. Error: %s", esp_err_to_name(ret));
        return;
    }

    // 打印 SD 卡信息
    sdmmc_card_print_info(stdout, card);
}


void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    // 配置 APA102 设备
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = APA102_SPI_CLOCK_HZ,
        .mode = 0,          // SPI 模式 0
        .spics_io_num = -1, // APA102 不需要 CS 引脚
        .queue_size = 1,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST, &devcfg, &led_strip));
    ESP_LOGI(TAG, "Configured APA102 LED strip");
}


void send_apa102_data(spi_device_handle_t spi_handle, uint8_t brightness, uint8_t red, uint8_t green, uint8_t blue)
{
    size_t buffer_size = (APA102_LED_NUMBERS * 4) + 8; // 起始帧 + LED 数据帧 + 结束帧
    uint8_t *buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_DMA);
    if (!buffer)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for APA102 buffer");
        return;
    }

    // 起始帧（4 字节，全为 0）
    memset(buffer, 0, 4);

    // 填充 LED 数据帧
    for (int i = 0; i < APA102_LED_NUMBERS; i++)
    {
        buffer[4 + (i * 4)] = 0b11100000 | (brightness & 0x1F); // 亮度
        buffer[4 + (i * 4) + 1] = blue;                         // 蓝色
        buffer[4 + (i * 4) + 2] = green;                        // 绿色
        buffer[4 + (i * 4) + 3] = red;                          // 红色
    }

    // 结束帧（至少 LED 数量 / 2 字节，全为 0）
    memset(buffer + 4 + (APA102_LED_NUMBERS * 4), 0, 4);

    // 发送数据
    spi_transaction_t trans = {
        .length = buffer_size * 8, // 数据长度（位）
        .tx_buffer = buffer,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi_handle, &trans));

    free(buffer);
}



void app_main(void)
{
    spi_init();
    ESP_LOGI(TAG, "LED initialized");
    configure_led();
    ESP_LOGI(TAG, "SD card initialized");
    sd_init();

    while (1) {
        send_apa102_data(led_strip, 0x1F, 0xFF, 0x00, 0x00); // 红色
        ESP_LOGI(TAG, "Blinking red");

        vTaskDelay(2000 / portTICK_PERIOD_MS);
        send_apa102_data(led_strip, 0x1F, 0x00, 0xFF, 0x00); // 绿色
        ESP_LOGI(TAG, "Blinking green");

        vTaskDelay(2000 / portTICK_PERIOD_MS);
        send_apa102_data(led_strip, 0x1F, 0x00, 0x00, 0xFF); // 蓝色
        ESP_LOGI(TAG, "Blinking blue");

        vTaskDelay(2000 / portTICK_PERIOD_MS);
        send_apa102_data(led_strip, 0x1F, 0xFF, 0xFF, 0xFF); // 白色
        ESP_LOGI(TAG, "Blinking white");

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
