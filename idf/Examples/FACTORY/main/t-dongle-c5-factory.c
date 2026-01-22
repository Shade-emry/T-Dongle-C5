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
#include <math.h>

#define BREATHE_PERIOD_MS 2000  // 呼吸周期（毫秒）
#define COLOR_CHANGE_PERIOD_MS 10000  // 颜色变化周期

#define WIFI_SSID "xinyuandianzi"     //"xinyuandianzi"
#define WIFI_PASSWORD "AA15994823428" //"AA15994823428"
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY 3 // Maximum retry number to connect to Wi-Fi

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static const char *TAG = "T-Dongle-C5";
#define SPI_HOST SPI2_HOST
bool sd_moudle = false;
int sd_size = 0;

lv_display_t *display;
void *buf1 = NULL;
void *buf2 = NULL;

static spi_device_handle_t led_strip;

bool ap_connect = false;
bool sta_connect = false;

lv_obj_t *scr;
lv_obj_t *wifi_rssi;
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
static void lvgl_demo_ui(void);
void check_moudle(void);
static int get_rssi(void);
void wifi_init_sta(void);
void breathe_light_task(void *arg);

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
    wifi_init_sta();

    button_init(BOOT_BUTTON_NUM);
    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreate(breathe_light_task, "breathe_light", 2048, NULL, 5, NULL);
    while(1)
    {
        int rssi = get_rssi();
        lv_label_set_text_fmt(wifi_rssi, "WIFI_RSSI :%d", rssi);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void breathe_light_task(void *arg)
{    
    // 初始化变量
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    float hue = 0.0f;  // HSV颜色空间中的色相（0-360度）
    
    while(1)
    {
        // 1. 计算呼吸效果（亮度变化）
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        float breathe_ratio = 0.5f * (1.0f + sinf(2 * M_PI * (current_time % BREATHE_PERIOD_MS) / BREATHE_PERIOD_MS));
        uint8_t brightness = (uint8_t)(31 * breathe_ratio);  // APA102亮度范围0-31
        
        // 2. 计算颜色变化
        hue = fmodf(360.0f * (current_time % COLOR_CHANGE_PERIOD_MS) / COLOR_CHANGE_PERIOD_MS, 360.0f);
        
        // 3. 将HSV转换为RGB
        float h = hue / 60.0f;
        float c = 1.0f;      // 饱和度
        float x = (1 - fabsf(fmodf(h, 2) - 1)) * c;
        float r, g, b;
        
        if (h < 1) { r = c; g = x; b = 0; }
        else if (h < 2) { r = x; g = c; b = 0; }
        else if (h < 3) { r = 0; g = c; b = x; }
        else if (h < 4) { r = 0; g = x; b = c; }
        else if (h < 5) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }
        
        // 转换为8位RGB值
        uint8_t red = (uint8_t)(r * 255);
        uint8_t green = (uint8_t)(g * 255);
        uint8_t blue = (uint8_t)(b * 255);
        
        // 4. 设置LED
        set_apa102_config(led_strip, brightness, (red << 16) | (green << 8) | blue);
        
        // 控制刷新率（20-50Hz）
        vTaskDelay(pdMS_TO_TICKS(20));
    }
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

    sd_size = ((uint64_t) card->csd.capacity) * card->csd.sector_size / (1024 * 1024); // 单位为MB
}

static void button_event_cb(void *arg, void *data)
{
    // iot_button_print_event((button_handle_t)arg);
    button_event_t event = iot_button_get_event((button_handle_t)arg);
    static bool lcd_off = false;
    switch (event)
    {
    case BUTTON_SINGLE_CLICK:
        ESP_LOGI(TAG, "Button single click");
        lcd_off = !lcd_off;
        if(!lcd_off)
        {
            set_brightness(0);
        }
        else
        {
            set_brightness(100);
        }
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
}

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
        ESP_LOGI(TAG, "WIFI RSSI: %d", get_rssi());        
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

static void lvgl_demo_ui(void)
{
    vTaskDelay(pdMS_TO_TICKS(2000));
    lv_obj_clean(scr);

    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "WIFI");
    lv_obj_set_size(title, 160, 15);
    lv_obj_set_style_text_font(title, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    lv_obj_t *wifi_name = lv_label_create(scr);
    lv_obj_set_size(wifi_name, 160, 15);
    lv_obj_set_style_text_font(wifi_name, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(wifi_name, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(wifi_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text_fmt(wifi_name, "WIFI_NAME: %s", WIFI_SSID);
    lv_obj_align(wifi_name, LV_ALIGN_TOP_MID, 0, 15);

    wifi_rssi = lv_label_create(scr);
    lv_obj_set_size(wifi_rssi, 160, 15);
    lv_obj_set_style_text_font(wifi_rssi, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(wifi_rssi, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(wifi_rssi, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text(wifi_rssi, "WIFI_RSSI:");
    lv_obj_align(wifi_rssi, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *sd_size_label = lv_label_create(scr);
    lv_obj_set_size(sd_size_label, 160, 15);
    lv_obj_set_style_text_font(sd_size_label, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(sd_size_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(sd_size_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text_fmt(sd_size_label, "SD_Size: %d MB", sd_size);
    lv_obj_align(sd_size_label, LV_ALIGN_TOP_MID, 0, 60);
}

static int get_rssi(void) {
    int rssi = 0;
    esp_wifi_sta_get_rssi(&rssi);
    return rssi;
}