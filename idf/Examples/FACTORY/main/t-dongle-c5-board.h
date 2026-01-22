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

#define LED_CI_PIN SPI_SCK  
#define LED_DI_PIN SPI_MOSI   

#define SD_CMD_PIN SPI_MOSI
#define SD_DAT0_PIN SPI_MISO
#define SD_CLK_PIN SPI_SCK
#define SD_CS_PIN 23

#define UART0_TX_PIN 11
#define UART0_RX_PIN 12



#define WIFI_MAX_STA_CONN 4
#define TCP_SERVER_PORT 8080
#define TCP_RECV_BUFFER_SIZE 1024
#define TCP_KEEPALIVE_IDLE 10
#define TCP_KEEPALIVE_INTERVAL 10
#define TCP_KEEPALIVE_COUNT 3

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY 3 // Maximum retry number to connect to Wi-Fi

#define BOOT_BUTTON_NUM 28
#define BUTTON_ACTIVE_LEVEL 0

#define EXAMPLE_PWM_TIMER LEDC_TIMER_0
#define EXAMPLE_PWM_MODE LEDC_LOW_SPEED_MODE
#define EXAMPLE_PWM_CHANNEL LEDC_CHANNEL_0
#define EXAMPLE_PWM_DUTY_RES LEDC_TIMER_10_BIT // Set duty resolution to 13 bits
#define EXAMPLE_PWM_DUTY (4096)                // Set duty to 50%. (2 ** 13) * 50% = 4096
#define EXAMPLE_PWM_FREQUENCY (5000)           // Frequency in Hertz. Set frequency at 4 kHz

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 0
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_DATA0 LCD_MOSI_PIN /*!< for 1-line SPI, this also refereed as MOSI */
#define EXAMPLE_PIN_NUM_PCLK LCD_CLK_PIN
#define EXAMPLE_PIN_NUM_CS LCD_CS_PIN
#define EXAMPLE_PIN_NUM_DC LCD_RS_PIN
#define EXAMPLE_PIN_NUM_RST LCD_RST_PIN
#define EXAMPLE_PIN_NUM_BK_LIGHT LCD_BL_PIN

#define BUFFER_LINES 15
#define MAX_LINE_LENGTH 128

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES 80
#define EXAMPLE_LCD_V_RES 160
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS 8
#define EXAMPLE_LCD_PARAM_BITS 8

#define APA102_MOSI_PIN LED_DI_PIN             // MOSI 引脚
#define APA102_SCLK_PIN LED_CI_PIN             // SCLK 引脚
#define APA102_LED_NUMBERS 1                   // APA102 LED 的数量
#define APA102_SPI_CLOCK_HZ (10 * 1000 * 1000) // SPI 时钟速度，10 MHz

// APA102 LED基本颜色定义
#define APA102_COLOR_RED 0xFF0000     // 红色
#define APA102_COLOR_GREEN 0x00FF00   // 绿色
#define APA102_COLOR_BLUE 0x0000FF    // 蓝色
#define APA102_COLOR_WHITE 0xFFFFFF   // 白色
#define APA102_COLOR_BLACK 0x000000   // 黑色(关闭)
#define APA102_COLOR_YELLOW 0xFFFF00  // 黄色
#define APA102_COLOR_CYAN 0x00FFFF    // 青色
#define APA102_COLOR_MAGENTA 0xFF00FF // 品红色
#define APA102_COLOR_ORANGE 0xFFA500  // 橙色
#define APA102_COLOR_PURPLE 0x800080  // 紫色

#define EXAMPLE_LVGL_DRAW_BUF_LINES 40 // number of display lines in each draw buffer
#define EXAMPLE_LVGL_TICK_PERIOD_MS 10
#define EXAMPLE_LVGL_TASK_STACK_SIZE (6 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY 2

#endif
