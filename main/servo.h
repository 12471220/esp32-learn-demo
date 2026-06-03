#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SERVO_GPIO GPIO_NUM_13

/**
 * @brief Initialize servo PWM (50Hz, 16-bit). Stops the servo.
 * @return ESP_OK on success.
 */
esp_err_t servo_init(void);

/**
 * @brief Set servo rotation (standard servo).
 * @param rotation 0 (full CW) to 180 (full CCW), 90 = center.
 * @return ESP_OK on success.
 */
esp_err_t servo_rotate(int rotation);

/**
 * @brief Stop PWM output — servo goes limp (no holding torque).
 * @return ESP_OK on success.
 */
esp_err_t servo_stop(void);
void light_on(void);
void light_off(void);

#ifdef __cplusplus
}
#endif
