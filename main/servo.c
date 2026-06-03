#include "servo.h"

#include <inttypes.h>

#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>

static const char *TAG = "servo";

// 50Hz, 16-bit: stop @ 1.5ms = 4915, range ±3277
#define SERVO_DUTY_STOP  4915
#define SERVO_DUTY_RANGE 3277

esp_err_t servo_init(void)
{
    const ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_16_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), TAG, "ledc timer config failed");

    const ledc_channel_config_t channel = {
        .gpio_num = SERVO_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_1,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel), TAG, "ledc channel config failed");

    ESP_LOGI(TAG, "initialized on GPIO %d", SERVO_GPIO);
    return servo_rotate(0);
}

esp_err_t servo_rotate(int rotation)
{
    if (rotation < 0) rotation = 0;
    if (rotation > 180) rotation = 180;

    // map [0..180]° to [0.5ms..2.5ms] pulse: 1638 → 4915 → 8192
    // duty = 0.5ms + (rotation / 180) * 2ms, in 16-bit ticks (1ms ≈ 3277)
    uint32_t duty = 1638 + (rotation * 6554) / 180;

    ESP_LOGW(TAG, "rotate %d° -> duty %" PRIu32, rotation, duty);

    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty),
                        TAG, "set duty failed");
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

esp_err_t servo_stop(void)
{
    ESP_LOGW(TAG, "stop — duty set to 0");
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0),
                        TAG, "set duty failed");
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

// use servo to open the light
void light_on(void) {
    ESP_ERROR_CHECK(servo_init());
    servo_rotate(53);
    vTaskDelay(pdMS_TO_TICKS(2000));
    servo_rotate(89);
    vTaskDelay(pdMS_TO_TICKS(2000));
    servo_stop();
}

void light_off(void) {
    ESP_ERROR_CHECK(servo_init());
    servo_rotate(138);
    vTaskDelay(pdMS_TO_TICKS(2000));
    servo_rotate(89);
    vTaskDelay(pdMS_TO_TICKS(2000));
    servo_stop();
}