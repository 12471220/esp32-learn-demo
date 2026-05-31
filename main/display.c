#include "display.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7735.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

static const char *TAG = "display";

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;

esp_err_t display_init(void)
{
    // --- Backlight PWM ---
    const ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&ledc_timer), TAG, "ledc timer config failed");

    const ledc_channel_config_t ledc_channel = {
        .gpio_num = DISPLAY_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ledc_channel), TAG, "ledc channel config failed");

    // --- SPI bus ---
    const spi_bus_config_t bus_cfg = {
        .sclk_io_num = DISPLAY_SCLK, // SCLK时钟引脚
        .mosi_io_num = DISPLAY_MOSI, // MOSI数据引脚
        .miso_io_num = -1,           // 不使用MISO引脚
        .quadwp_io_num = -1,         // 不使用WP引脚
        .quadhd_io_num = -1,         // 不使用HD引脚
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t),// 最大传输大小（全屏RGB565）
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO),
                        TAG, "SPI bus init failed");

    // --- SPI panel IO ---
    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = DISPLAY_CS,  // CS片选引脚
        .dc_gpio_num = DISPLAY_DC,  // DC数据/命令选择引脚
        .spi_mode = 0,              // SPI模式0
        .pclk_hz = 40 * 1000 * 1000,// 40MHz像素时钟
        .trans_queue_depth = 10,    // 传输队列深度
        .lcd_cmd_bits = 8,          // LCD命令的位宽
        .lcd_param_bits = 8,        //  LCD参数的位宽
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io_handle),
                        TAG, "panel IO create failed");

    // --- ST7735 panel ---
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = DISPLAY_RST, // 复位引脚
        .rgb_endian = LCD_RGB_ENDIAN_BGR, // BGR颜色顺序
        .bits_per_pixel = 16, // 颜色格式的位数（RGB565）
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7735(io_handle, &panel_cfg, &panel_handle),
                        TAG, "panel create failed");

    // --- Init sequence ---
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle), TAG, "panel init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true), TAG, "disp on failed");

    display_set_brightness(10);

    ESP_LOGI(TAG, "initialized (%dx%d)", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return ESP_OK;
}

esp_err_t display_set_brightness(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }
    uint32_t duty = (8191 * percent) / 100;
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty),
                        TAG, "set duty failed");
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

esp_err_t display_fill(uint16_t color)
{
    if (!panel_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    static uint16_t line[DISPLAY_WIDTH];
    for (int i = 0; i < DISPLAY_WIDTH; i++)
        line[i] = color;

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        ESP_RETURN_ON_ERROR(
            esp_lcd_panel_draw_bitmap(panel_handle, 0, y, DISPLAY_WIDTH, y + 1, line),
            TAG, "fill row %d failed", y);
    }
    return ESP_OK;
}

esp_err_t draw_rectangle(int x, int y, int w, int h, uint16_t color)
{
    if (!panel_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t line[w];
    for (int i = 0; i < w; i++)
        line[i] = color;

    for (int row = 0; row < h; row++) {
        ESP_RETURN_ON_ERROR(
            esp_lcd_panel_draw_bitmap(panel_handle, x, y + row, x + w, y + row + 1, line),
            TAG, "draw rect row %d failed", row);
    }
    return ESP_OK;
}

esp_err_t display_draw_bitmap(int x, int y, int w, int h, const uint16_t *data)
{
    if (!panel_handle) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + w, y + h, data);
}


void display_test_run(void)
{
    ESP_LOGI(TAG, "initializing display...");
    ESP_ERROR_CHECK(display_init());
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    // Cycle through primary colors
    while (1) {
        ESP_LOGI(TAG, "red");
        ESP_ERROR_CHECK(display_fill(COLOR_RED));
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "green");
        ESP_ERROR_CHECK(display_fill(COLOR_GREEN));
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "blue");
        ESP_ERROR_CHECK(display_fill(COLOR_BLUE));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static lv_disp_t *lvgl_disp = NULL;

static void lvgl_hello_world(void)
{
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = DISPLAY_WIDTH * 50,
        .double_buffer = true,
        .hres = DISPLAY_WIDTH,
        .vres = DISPLAY_HEIGHT,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };

    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    lv_disp_set_rotation(lvgl_disp, LV_DISP_ROT_270);

    lvgl_port_lock(0);
    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Hello World");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lvgl_port_unlock();

    static lv_style_t style_bg;
    lv_style_init(&style_bg);
    lv_style_set_bg_color(&style_bg, lv_color_hex(0x000000)); 
    lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);
    lv_obj_add_style(scr, &style_bg, LV_STATE_DEFAULT);

    ESP_LOGI(TAG, "LVGL hello world displayed");
}

void display_lvgl_test_run(void)
{
    ESP_LOGI(TAG, "initializing display with LVGL...");
    ESP_ERROR_CHECK(display_init());
    lvgl_hello_world();
}

/* --- Sensor display --- */

static lv_obj_t *sensor_temp_label = NULL;
static lv_obj_t *sensor_hum_label = NULL;

void display_update_sensor(int temperature, int humidity)
{
    if (!sensor_temp_label || !sensor_hum_label) {
        return;
    }

    lvgl_port_lock(0);
    lv_label_set_text_fmt(sensor_temp_label, "Temperature: %dC", temperature);
    lv_label_set_text_fmt(sensor_hum_label, "Humidity: %d%%", humidity);
    lvgl_port_unlock();
}

static void lvgl_sensor_ui(void)
{
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = DISPLAY_WIDTH * 50,
        .double_buffer = true,
        .hres = DISPLAY_WIDTH,
        .vres = DISPLAY_HEIGHT,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    lv_disp_set_rotation(lvgl_disp, LV_DISP_ROT_270);

    lvgl_port_lock(0);
    lv_obj_t *scr = lv_scr_act();

    /* Black background */
    static lv_style_t style_bg;
    lv_style_init(&style_bg);
    lv_style_set_bg_color(&style_bg, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);
    lv_obj_add_style(scr, &style_bg, LV_STATE_DEFAULT);

    /* Temperature label */
    sensor_temp_label = lv_label_create(scr);
    lv_obj_set_style_text_font(sensor_temp_label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(sensor_temp_label, lv_color_white(), 0);
    lv_label_set_text(sensor_temp_label, "Temperature: --C");
    lv_obj_align(sensor_temp_label, LV_ALIGN_TOP_LEFT, 0, 5);

    /* Humidity label */
    sensor_hum_label = lv_label_create(scr);
    lv_obj_set_style_text_font(sensor_hum_label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(sensor_hum_label, lv_color_white(), 0);
    lv_label_set_text(sensor_hum_label, "Humidity: --%");
    lv_obj_align(sensor_hum_label, LV_ALIGN_TOP_LEFT, 0, 20);

    lvgl_port_unlock();

    ESP_LOGI(TAG, "LVGL sensor UI initialized");
}

void display_sensor_run(void)
{
    ESP_LOGI(TAG, "initializing sensor display...");
    ESP_ERROR_CHECK(display_init());
    lvgl_sensor_ui();
}