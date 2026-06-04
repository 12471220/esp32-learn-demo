#include "servo.h"

#include <inttypes.h>

#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static const char *TAG = "servo";

// 50Hz, 16-bit: center @ 1.5ms = 4915, range ±3277 (0.5ms → 2.5ms)
#define SERVO_DUTY_STOP  4915
#define SERVO_DUTY_RANGE 3277

/* --- single-consumer command queue --- */
static QueueHandle_t servo_cmd_queue = NULL;

/* ===================================================================
 *  Low-level PWM helpers
 * =================================================================== */

esp_err_t servo_init(void)
{
    static bool initialized = false;
    if (initialized) {
        ESP_LOGI(TAG, "already initialized, skipping");
        return ESP_OK;
    }

    const ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_16_BIT,
        .timer_num       = LEDC_TIMER_1,
        .freq_hz         = 50,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), TAG, "ledc timer config failed");

    const ledc_channel_config_t channel = {
        .gpio_num   = SERVO_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel), TAG, "ledc channel config failed");

    initialized = true;
    ESP_LOGI(TAG, "initialized on GPIO %d", SERVO_GPIO);

    // Start at center so the servo doesn't jerk on first use
    return servo_rotate(90);
}

esp_err_t servo_rotate(int rotation)
{
    if (rotation < 0)   rotation = 0;
    if (rotation > 180) rotation = 180;

    // map [0..180]° → pulse [0.5ms..2.5ms] in 16-bit ticks (1 ms ≈ 3277)
    uint32_t duty = 1638 + (rotation * 6554) / 180;

    ESP_LOGI(TAG, "rotate %d° -> duty %" PRIu32, rotation, duty);

    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty),
                        TAG, "set duty failed");
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

esp_err_t servo_stop(void)
{
    ESP_LOGI(TAG, "stop — duty set to 0");
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0),
                        TAG, "set duty failed");
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

/* ===================================================================
 *  Movement sequences (run inside servo task)
 * =================================================================== */

static void light_on_sequence(void)
{
    ESP_LOGI(TAG, "light-on  sequence start");
    servo_rotate(58);
    vTaskDelay(pdMS_TO_TICKS(1000));
    servo_rotate(89);
    vTaskDelay(pdMS_TO_TICKS(500));
    servo_stop();
    ESP_LOGI(TAG, "light-on  sequence done");
}

static void light_off_sequence(void)
{
    ESP_LOGI(TAG, "light-off sequence start");
    servo_rotate(128);
    vTaskDelay(pdMS_TO_TICKS(1000));
    servo_rotate(89);
    vTaskDelay(pdMS_TO_TICKS(500));
    servo_stop();
    ESP_LOGI(TAG, "light-off sequence done");
}

/* ===================================================================
 *  Servo task — serialises all commands through a queue
 * =================================================================== */

static void servo_task(void *pvParameters)
{
    servo_cmd_t cmd;
    ESP_LOGI(TAG, "task started, waiting for commands");
    while (1) {
        if (xQueueReceive(servo_cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            switch (cmd) {
            case SERVO_CMD_LIGHT_ON:
                light_on_sequence();
                break;
            case SERVO_CMD_LIGHT_OFF:
                light_off_sequence();
                break;
            default:
                ESP_LOGW(TAG, "unknown command: %d", cmd);
                break;
            }
        }
    }
}

void servo_task_start(void)
{
    servo_cmd_queue = xQueueCreate(4, sizeof(servo_cmd_t));
    if (servo_cmd_queue == NULL) {
        ESP_LOGE(TAG, "failed to create command queue");
        return;
    }
    xTaskCreate(servo_task, "servo_task", 5120, NULL, 3, NULL);
}

/* ===================================================================
 *  Public async API — called from HTTP handlers
 * =================================================================== */

esp_err_t servo_request(servo_cmd_t cmd)
{
    if (servo_cmd_queue == NULL) {
        ESP_LOGE(TAG, "queue not initialized — call servo_task_start() first");
        return ESP_FAIL;
    }
    if (xQueueSend(servo_cmd_queue, &cmd, 0) != pdTRUE) {
        ESP_LOGW(TAG, "queue full, cmd %d dropped", cmd);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "cmd %d enqueued", cmd);
    return ESP_OK;
}
