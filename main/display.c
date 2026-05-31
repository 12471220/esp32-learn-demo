#include "display.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7735.h"
#include "esp_log.h"

static const char *TAG = "display";

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;

esp_err_t display_init(void)
{
    // --- Backlight ---
    gpio_config_t bl_conf = {
        .pin_bit_mask = 1ULL << DISPLAY_BL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl_conf);
    gpio_set_level(DISPLAY_BL, 0);

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
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel_handle, true), TAG, "invert failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true), TAG, "disp on failed");

    gpio_set_level(DISPLAY_BL, 1);

    ESP_LOGI(TAG, "initialized (%dx%d)", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return ESP_OK;
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


void display_test_run() {
    ESP_LOGI(TAG, "initializing display...");
    ESP_ERROR_CHECK(display_init());

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