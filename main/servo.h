#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SERVO_GPIO GPIO_NUM_13

/** Command types for the servo task queue */
typedef enum {
    SERVO_CMD_LIGHT_ON = 0,
    SERVO_CMD_LIGHT_OFF,
} servo_cmd_t;

/**
 * @brief Initialize servo PWM (50Hz, 16-bit). Idempotent — safe to call
 *        multiple times; subsequent calls are no-ops.
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
 * @brief Return servo to center position and hold.
 * @return ESP_OK on success.
 */
esp_err_t servo_stop(void);

/**
 * @brief Start the FreeRTOS task that processes servo commands from a queue.
 *        Must be called once after servo_init().
 */
void servo_task_start(void);

/**
 * @brief Enqueue a servo command for async execution. Non-blocking — returns
 *        immediately; the movement runs in the servo task.
 * @param cmd  SERVO_CMD_LIGHT_ON or SERVO_CMD_LIGHT_OFF.
 * @return ESP_OK if queued, ESP_FAIL if queue full or not initialized.
 */
esp_err_t servo_request(servo_cmd_t cmd);

#ifdef __cplusplus
}
#endif
