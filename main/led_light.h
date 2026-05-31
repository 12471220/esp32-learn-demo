#pragma once

#include "esp_err.h"
#include "hal/gpio_types.h"
#ifdef __cplusplus
extern "C" {
#endif

// Change this to match your board if needed.
#define BLUE_LED    GPIO_NUM_25
#define GREEN_LED   GPIO_NUM_26
#define RED_LED     GPIO_NUM_27
// we can use led_function to control beeper
#define beep_gpio   GPIO_NUM_16

esp_err_t led_init(gpio_num_t gpio_num);
esp_err_t led_on(gpio_num_t gpio_num);
esp_err_t led_off(gpio_num_t gpio_num);
esp_err_t led_toggle(gpio_num_t gpio_num);
esp_err_t led_blink(gpio_num_t gpio_num, int duration_ms, int count);

#ifdef __cplusplus
}
#endif
