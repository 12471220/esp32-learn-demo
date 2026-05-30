#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Most ESP32 dev boards have a built-in LED on GPIO 2.
// Change this to match your board if needed.
#define LED_GPIO  GPIO_NUM_2

esp_err_t led_init(void);
esp_err_t led_on(void);
esp_err_t led_off(void);
esp_err_t led_toggle(void);
esp_err_t led_blink(int duration_ms, int count);

#ifdef __cplusplus
}
#endif
