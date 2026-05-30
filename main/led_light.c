#include "led_light.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

esp_err_t led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    return gpio_config(&io_conf);
}

esp_err_t led_on(void)
{
    return gpio_set_level(LED_GPIO, 1);
}

esp_err_t led_off(void)
{
    return gpio_set_level(LED_GPIO, 0);
}

esp_err_t led_toggle(void)
{
    int level = gpio_get_level(LED_GPIO);
    return gpio_set_level(LED_GPIO, !level);
}

esp_err_t led_blink(int duration_ms, int count)
{
    for (int i = 0; i < count; i++) {
        esp_err_t err = led_on();
        if (err != ESP_OK) return err;
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        ESP_LOGI("[INFO]", "LED open %d/%d", i + 1, count);

        err = led_off();
        if (err != ESP_OK) return err;
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        ESP_LOGI("[INFO]", "LED close %d/%d", i + 1, count);
    }
    return ESP_OK;
}
