/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "sdkconfig.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/ledc.h"
#include "lq.h"
#include "st7735.h"
#include "lvgl.h"

#include "iot_button.h"
#include "button_gpio.h"
#include "t-dongle-c5-board.h"
#include "logo.h"

#define CONFIG_WIFI_STA 1

#if CONFIG_WIFI_STA
#define WIFI_SSID "xinyuandianzi"     //"xinyuandianzi"
#define WIFI_PASSWORD "AA15994823428" //"AA15994823428"
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY 3 // Maximum retry number to connect to Wi-Fi

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
#else
#define WIFI_SSID "T-Dongle-C5"
#define WIFI_PASSWORD "88888888"
#define WIFI_CHANNEL WIFI_CHANNEL_1
#endif

static const char *TAG = "T-Dongle-C5";
#define SPI_HOST SPI2_HOST
bool sd_moudle = false;

lv_display_t *display;
void *buf1 = NULL;
void *buf2 = NULL;

static spi_device_handle_t led_strip;
typedef struct
{
    char data[TCP_RECV_BUFFER_SIZE];
} tcp_msg_t;

typedef struct
{
    char lines[BUFFER_LINES][MAX_LINE_LENGTH];
    int head;
    int count;
} circular_text_buffer_t;
static circular_text_buffer_t text_buffer = {0};

bool ap_connect = false;
bool sta_connect = false;

lv_obj_t *scr;
lv_obj_t *tcp_title;
lv_obj_t *tcp_textarea;
SemaphoreHandle_t xSemaphore = NULL;
QueueHandle_t tcp_queue = NULL;
static _lock_t lvgl_api_lock;

void spi_init(void);
void configure_led(void);
void set_apa102_config(spi_device_handle_t spi_handle, uint8_t brightness, uint32_t color);
void send_apa102_data(spi_device_handle_t spi_handle, uint8_t brightness, uint8_t red, uint8_t green, uint8_t blue);
void set_brightness(int brightness);
void lcd_bl_init(void);
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void lv_tick_task(void *arg);
static void lvgl_port_task(void *arg);
static void lcd_init(void);
static void sd_init(void);
static void button_event_cb(void *arg, void *data);
static void button_init(uint32_t button_num);
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void add_line_to_buffer(const char *line);
static void update_textarea_from_buffer(lv_obj_t *textarea);
static void lvgl_demo_ui(void);
static void process_tcp_data(char *data, int len);
static void tcp_client_handle_task(void *sock_ptr);
static void tcp_server_task(void *pvParameters);
void check_moudle(void);

#if CONFIG_WIFI_STA
void wifi_init_sta(void);
#else
void wifi_init_ap(void);
#endif

void app_main(void)
{
    printf("Hello world!\n");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    spi_init();
    sd_init();
    lcd_init();
    configure_led();
#if CONFIG_WIFI_STA
    wifi_init_sta();
#else
    wifi_init_ap();
#endif

    button_init(BOOT_BUTTON_NUM);
    vTaskDelay(pdMS_TO_TICKS(100));
    xSemaphore = xSemaphoreCreateBinary();
    if (xSemaphore == NULL)
    {
        ESP_LOGE(TAG, "Failed to create semaphore");
    }

    tcp_queue = xQueueCreate(20, sizeof(tcp_msg_t));
    if (tcp_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create TCP queue");
    }

    // xTaskCreate(sd_task, "SD Task", 2048, NULL, 5, NULL);
    xTaskCreate(tcp_server_task, "WIFI TCP Server Task", 4096, NULL, 3, NULL);
}

void spi_init(void)
{
    spi_bus_config_t buscfg = {
        .sclk_io_num = SPI_SCK,
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = SPI_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
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
    set_apa102_config(led_strip, 31, APA102_COLOR_PURPLE);
}

/*
@brief: set APA102 LED strip color
@param spi_handle: SPI handle
@param brightness: Brightness, 0-31, 0 is lowest brightness, 31 is highest brightness
@param color: Color, format is RGB
*/
void set_apa102_config(spi_device_handle_t spi_handle, uint8_t brightness, uint32_t color)
{
    uint8_t red = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8) & 0xFF;
    uint8_t blue = color & 0xFF;

    send_apa102_data(spi_handle, brightness, red, green, blue);
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

void set_brightness(int brightness)
{
    // 限制亮度范围在0-100之间
    if (brightness < 0)
        brightness = 0;
    if (brightness > 100)
        brightness = 100;

    // 将0-100的范围映射到0-1023 (10位PWM)
    uint32_t duty = (brightness * 1023) / 100;
    // ESP_LOGI("PWM", "Duty: %lu", duty);
    ESP_ERROR_CHECK(ledc_set_duty(EXAMPLE_PWM_MODE, EXAMPLE_PWM_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(EXAMPLE_PWM_MODE, EXAMPLE_PWM_CHANNEL));
}

void lcd_bl_init(void)
{
    // LEDC定时器配置
    ledc_timer_config_t timer_conf = {
        .duty_resolution = EXAMPLE_PWM_DUTY_RES,
        .freq_hz = EXAMPLE_PWM_FREQUENCY,
        .speed_mode = EXAMPLE_PWM_MODE,
        .timer_num = EXAMPLE_PWM_TIMER,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // LEDC通道配置
    ledc_channel_config_t ledc_conf = {
        .channel = EXAMPLE_PWM_CHANNEL,
        .duty = 100, // 初始占空比为0
        .gpio_num = EXAMPLE_PIN_NUM_BK_LIGHT,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = EXAMPLE_PWM_MODE,
        .timer_sel = EXAMPLE_PWM_TIMER};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_conf));
}

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    // because SPI LCD is big-endian, we need to swap the RGB bytes order
    lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));

    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);

    lv_display_flush_ready(disp);
}

static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    uint32_t time_threshold_ms = 1000 / CONFIG_FREERTOS_HZ;
    while (1)
    {
        _lock_acquire(&lvgl_api_lock);
        if (ap_connect || sta_connect)
        {
            update_textarea_from_buffer(tcp_textarea);
            // 自动滚动到底部
            lv_obj_scroll_to_y(tcp_textarea, LV_COORD_MAX, LV_ANIM_ON);
        }
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);
        // in case of triggering a task watch dog time out
        time_till_next_ms = MAX(time_till_next_ms, time_threshold_ms);
        usleep(1000 * time_till_next_ms);
    }
}

static void lcd_init(void)
{
    // ESP_LOGI(TAG, "Turn off LCD backlight");
    // gpio_config_t bk_gpio_config = {
    //     .mode = GPIO_MODE_OUTPUT,
    //     .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT};
    // ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    lcd_bl_init();
    set_brightness(100);

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI_HOST, &io_config, &io_handle));

    // panel_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16};

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7735(io_handle, &panel_config, &panel_handle));
    // Reset the display
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    // Initialize LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    // Swap x and y axis (Different LCD screens may need different options)
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    // Mirror the display horizontally and vertically
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    // Set the display gap
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 1, 26));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // ESP_LOGI(TAG, "Turn on LCD backlight");
    // gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    lv_init();

    display = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);

    size_t draw_buffer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_DRAW_BUF_LINES * sizeof(lv_color16_t);
    void *buf1 = spi_bus_dma_memory_alloc(SPI_HOST, draw_buffer_sz, 0);
    assert(buf1);
    void *buf2 = spi_bus_dma_memory_alloc(SPI_HOST, draw_buffer_sz, 0);
    assert(buf2);

    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_user_data(display, panel_handle);

    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);

    lv_display_set_flush_cb(display, lvgl_flush_cb);

    // 创建并启动LVGL定时器
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "lvgl_tick"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Register io panel event callback for LVGL flush ready notification");
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = example_notify_lvgl_flush_ready,
    };
    /* Register done callback */
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display));
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_V_RES, EXAMPLE_LCD_H_RES, gImage_logo);
    for (int i = 100; i > 0; i--)
    {
        set_brightness(i);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    _lock_acquire(&lvgl_api_lock);
    check_moudle();
    _lock_release(&lvgl_api_lock);

    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreate(lvgl_port_task, "LVGL Task", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, 1, NULL);

    _lock_acquire(&lvgl_api_lock);
    lvgl_demo_ui();
    _lock_release(&lvgl_api_lock);
}

void check_moudle(void)
{
    scr = lv_screen_active();

    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_t *card = lv_label_create(scr);
    lv_obj_set_size(card, 160, 30);
    lv_obj_set_style_text_font(card, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_align(card, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);

    if (sd_moudle == true)
    {
        lv_obj_set_style_text_color(card, lv_color_hex(0x00ff00), LV_PART_MAIN);
        lv_label_set_text_static(card, "mount sd card");
    }
    else
    {
        lv_obj_set_style_text_color(card, lv_color_hex(0xff0000), LV_PART_MAIN);
        lv_label_set_text_static(card, "not mounted sd card");
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
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
        sd_moudle = false;
        return ;
    }
    else
    {
        sd_moudle = true;
    }

    // 打印 SD 卡信息
    sdmmc_card_print_info(stdout, card);
}

static void button_event_cb(void *arg, void *data)
{
    // iot_button_print_event((button_handle_t)arg);
    button_event_t event = iot_button_get_event((button_handle_t)arg);
    switch (event)
    {
    case BUTTON_SINGLE_CLICK:
        ESP_LOGI(TAG, "Button single click");
        break;
    case BUTTON_DOUBLE_CLICK:
        ESP_LOGI(TAG, "Button double click");
        break;
    case BUTTON_LONG_PRESS_HOLD:
        ESP_LOGI(TAG, "Button long hold");
        break;
    default:
        break;
    }
}

static void button_init(uint32_t button_num)
{
    button_config_t btn_cfg = {0};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = button_num,
        .active_level = BUTTON_ACTIVE_LEVEL,
        .enable_power_save = true,
    };

    button_handle_t btn;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn);
    assert(ret == ESP_OK);

    ret = iot_button_register_cb(btn, BUTTON_PRESS_DOWN, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_PRESS_UP, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_PRESS_REPEAT, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_PRESS_REPEAT_DONE, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_DOUBLE_CLICK, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_HOLD, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_UP, NULL, button_event_cb, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_PRESS_END, NULL, button_event_cb, NULL);

    ESP_ERROR_CHECK(ret);
}

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
#if CONFIG_WIFI_STA
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            sta_connect = false;
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            set_apa102_config(led_strip, 31, APA102_COLOR_RED);
            ESP_LOGI(TAG, "connect to the AP fail");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        sta_connect = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        set_apa102_config(led_strip, 31, APA102_COLOR_GREEN);
    }
#else
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        set_apa102_config(led_strip, 31, APA102_COLOR_GREEN);
        ap_connect = true;
        ESP_LOGI(TAG, "ap_connect = %d", ap_connect);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
        set_apa102_config(led_strip, 31, APA102_COLOR_RED);
        ap_connect = false;
        ESP_LOGI(TAG, "ap_connect = %d", ap_connect);
    }
#endif
}

#if CONFIG_WIFI_STA
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PK_MODE_AUTOMATIC,
            .sae_h2e_identifier = " ",
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASSWORD);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASSWORD);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

#else
void wifi_init_ap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASSWORD,
            .max_connection = WIFI_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    if (strlen(WIFI_PASSWORD) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
}
#endif

static void add_line_to_buffer(const char *line)
{
    // 添加新行到环形缓冲区
    strncpy(text_buffer.lines[text_buffer.head], line, MAX_LINE_LENGTH - 1);
    text_buffer.lines[text_buffer.head][MAX_LINE_LENGTH - 1] = '\0';

    text_buffer.head = (text_buffer.head + 1) % BUFFER_LINES;
    if (text_buffer.count < BUFFER_LINES)
    {
        text_buffer.count++;
    }
}

static void update_textarea_from_buffer(lv_obj_t *textarea)
{
    char display_text[BUFFER_LINES * MAX_LINE_LENGTH] = {0};

    // 从最旧的行开始组装文本
    int start_idx = (text_buffer.count == BUFFER_LINES) ? text_buffer.head : 0;

    for (int i = 0; i < text_buffer.count; i++)
    {
        int idx = (start_idx + i) % BUFFER_LINES;
        strcat(display_text, text_buffer.lines[idx]);
        if (i < text_buffer.count - 1)
        {
            strcat(display_text, "\n");
        }
    }

    lv_textarea_set_text(textarea, display_text);
}

static void lvgl_demo_ui(void)
{
    vTaskDelay(pdMS_TO_TICKS(2000));

    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    tcp_title = lv_label_create(scr);
    lv_label_set_text(tcp_title, "TCP Client");
    lv_obj_set_size(tcp_title, 160, 15);
    lv_obj_set_style_text_font(tcp_title, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(tcp_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(tcp_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(tcp_title, LV_ALIGN_TOP_MID, 0, 2);

    tcp_textarea = lv_textarea_create(scr);
    lv_obj_set_size(tcp_textarea, LV_HOR_RES - 2, LV_VER_RES - 15);
    lv_obj_align(tcp_textarea, LV_ALIGN_TOP_MID, 0, 15);

    // 设置文本区域样式
    lv_obj_set_style_bg_color(tcp_textarea, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_color(tcp_textarea, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_set_style_border_width(tcp_textarea, 1, LV_PART_MAIN);
    lv_obj_set_style_text_color(tcp_textarea, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(tcp_textarea, &lv_font_unscii_8, LV_PART_MAIN);

    lv_textarea_set_text(tcp_textarea, "TCP Server Ready.\n\nWait connect...\n");

    // 启用滚动条
    lv_obj_set_scrollbar_mode(tcp_textarea, LV_SCROLLBAR_MODE_AUTO);

    // 设置文本对齐
    lv_obj_set_style_text_align(tcp_textarea, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
}

void sd_task(void *arg)
{
    while (1)
    {
        // SD 卡通信代码
        // ESP_LOGI(TAG, "SD Card communication");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void process_tcp_data(char *data, int len)
{
    ESP_LOGI(TAG, "%.*s", len, data);

    tcp_msg_t msg;
    strncpy(msg.data, data, sizeof(msg.data));
    msg.data[sizeof(msg.data) - 1] = '\0'; // 确保字符串以空字符结尾

    add_line_to_buffer(msg.data);

    // 示例：如果收到"BRIGHTNESS_X"，调整屏幕亮度
    if (strncmp(data, "BRIGHTNESS_", 11) == 0)
    {
        int brightness = atoi(data + 11);
        if (brightness >= 0 && brightness <= 100)
        {
            set_brightness(brightness);
            ESP_LOGI(TAG, "Screen brightness set to %d via TCP command", brightness);
        }
    }
}

static void tcp_client_handle_task(void *sock_ptr)
{
    int sock = *(int *)sock_ptr;
    int len;
    char *rx_buffer = heap_caps_malloc(TCP_RECV_BUFFER_SIZE, MALLOC_CAP_DEFAULT);
    if (rx_buffer == NULL)
    {
        ESP_LOGE(TAG, "Memory allocation failed for rx_buffer");
        return;
    }

    do
    {
        len = recv(sock, rx_buffer, TCP_RECV_BUFFER_SIZE - 1, 0);
        if (len < 0)
        {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        }
        else if (len == 0)
        {
            ESP_LOGW(TAG, "Connection closed");
        }
        else
        {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            // ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            // Echo the received data back to client
            int to_write = len;
            while (to_write > 0)
            {
                int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
                if (written < 0)
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
                to_write -= written;
            }

            // 处理接收到的数据
            process_tcp_data(rx_buffer, len);
        }
    } while (len > 0);
    free(rx_buffer); // 释放内存
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    int keepAlive = 1;
    int keepIdle = TCP_KEEPALIVE_IDLE;
    int keepInterval = TCP_KEEPALIVE_INTERVAL;
    int keepCount = TCP_KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    if (addr_family == AF_INET)
    {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(TCP_SERVER_PORT);
        ip_protocol = IPPROTO_IP;
    }

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", TCP_SERVER_PORT);

    err = listen(listen_sock, WIFI_MAX_STA_CONN);
    if (err != 0)
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        ESP_LOGI(TAG, "Socket listening on port %d", TCP_SERVER_PORT);

        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        // Convert ip address to string
        if (source_addr.ss_family == PF_INET)
        {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        tcp_client_handle_task(&sock);

        shutdown(sock, 0);
        close(sock);
    }

    close(listen_sock);
    vTaskDelete(NULL);
}
