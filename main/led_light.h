#pragma once

#include "esp_err.h"
#include "hal/gpio_types.h"
#ifdef __cplusplus
extern "C" {
#endif

// Most ESP32 dev boards have a built-in LED on GPIO 2.
// Change this to match your board if needed.
#define LED1_GPIO  GPIO_NUM_2
#define LED2_GPIO  GPIO_NUM_4

esp_err_t led_init(gpio_num_t gpio_num);
esp_err_t led_on(gpio_num_t gpio_num);
esp_err_t led_off(gpio_num_t gpio_num);
esp_err_t led_toggle(gpio_num_t gpio_num);
esp_err_t led_blink(gpio_num_t gpio_num, int duration_ms, int count);

void toggle_led1(void *pvParameters);
void toggle_led2(void *pvParameters);
void led_demo_run(void);

#ifdef __cplusplus
}
#endif
