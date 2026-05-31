#pragma once

#include "esp_err.h"
#include "hal/gpio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Pin definitions (change to match your wiring) ---
#define DISPLAY_SCLK     GPIO_NUM_18 // SPI Clock
#define DISPLAY_MOSI     GPIO_NUM_23 // SDA, SPI data line
#define DISPLAY_RST      GPIO_NUM_22 // Reset
#define DISPLAY_DC       GPIO_NUM_32
#define DISPLAY_CS       GPIO_NUM_33
#define DISPLAY_BL       GPIO_NUM_21

// --- Display parameters ---
#define DISPLAY_WIDTH   130
#define DISPLAY_HEIGHT  130

// --- RGB565 color helper ---
#define RGB565(r, g, b)  ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

// Common colors
#define COLOR_BLACK      0x0000
#define COLOR_WHITE      0xFFFF
#define COLOR_RED        0xF800
#define COLOR_GREEN      0x07E0
#define COLOR_BLUE       0x001F
#define COLOR_YELLOW     0xFFE0
#define COLOR_CYAN       0x07FF
#define COLOR_MAGENTA    0xF81F

/**
 * @brief Initialize the ST7735 display over SPI.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t display_init(void);

/**
 * @brief Fill the entire screen with a single color.
 * @param color 16-bit RGB565 color value.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t display_fill(uint16_t color);

/**
 * @brief Draw a bitmap to the display.
 * @param x      Starting X coordinate.
 * @param y      Starting Y coordinate.
 * @param w      Width of the bitmap.
 * @param h      Height of the bitmap.
 * @param data   Pointer to RGB565 pixel data.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t display_draw_bitmap(int x, int y, int w, int h, const uint16_t *data);

void display_test_run(void);
void display_lvgl_test_run(void);
void display_update_sensor(int temperature, int humidity);
void display_sensor_run(void);

#ifdef __cplusplus
}
#endif
