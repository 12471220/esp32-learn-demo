#include "led_light.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

esp_err_t led_init(gpio_num_t gpio_num)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    return gpio_config(&io_conf);
}

esp_err_t led_on(gpio_num_t gpio_num)
{
    return gpio_set_level(gpio_num, 1);
}

esp_err_t led_off(gpio_num_t gpio_num)
{
    return gpio_set_level(gpio_num, 0);
}

esp_err_t led_toggle(gpio_num_t gpio_num)
{
    int level = gpio_get_level(gpio_num);
    return gpio_set_level(gpio_num, !level);
}

esp_err_t led_blink(gpio_num_t gpio_num, int duration_ms, int count)
{
    for (int i = 0; i < count; i++) {
        esp_err_t err = led_on(gpio_num);
        if (err != ESP_OK) return err;
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        ESP_LOGD("led_light", "[DEBUG] LED%d open %d/%d", gpio_num, i + 1, count);

        err = led_off(gpio_num);
        if (err != ESP_OK) return err;
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        ESP_LOGD("led_light", "[DEBUG] LED%d close %d/%d", gpio_num, i + 1, count);
    }
    return ESP_OK;
}

// task1 test
void toggle_led1(void *pvParameters) {
    while(1) {
        led_blink(RED_LED, 1000, 1);
    }
}

// task2 test
void toggle_led2(void *pvParameters) {
    while(1) {
        led_blink(GREEN_LED, 400, 1);
    }
}

void three_color_led_blink() {
    if(led_init(RED_LED) != ESP_OK) {
        ESP_LOGE("led_light", "[ERROR] Failed to initialize RED_LED");
        return;
    }
    if(led_init(GREEN_LED) != ESP_OK) {
        ESP_LOGE("led_light", "[ERROR] Failed to initialize GREEN_LED");
        return;
    }
    if(led_init(BLUE_LED) != ESP_OK) {
        ESP_LOGE("led_light", "[ERROR] Failed to initialize BLUE_LED");
        return;
    }
    while(1) {
        led_blink(RED_LED, 1000, 2);
        led_blink(GREEN_LED, 2000, 2);
        led_blink(BLUE_LED, 3000, 2);
    }
}

void led_demo_run() {
    ESP_LOGI("led_light", "[INFO] Demo run!");

    esp_err_t err1 = led_init(RED_LED);
    if (err1 != ESP_OK) {
        ESP_LOGE("led_light", "[ERROR] RED_LED init failed: %s", esp_err_to_name(err1));
        return;
    }
    esp_err_t err2 = led_init(BLUE_LED);
    if (err2 != ESP_OK) {
        ESP_LOGE("led_light", "[ERROR] BLUE_LED init failed: %s", esp_err_to_name(err2));
        return;
    }
    xTaskCreatePinnedToCore(
        toggle_led1,    // Task function
        "led_task1",    // Name of the task (for debugging)
        2048,           // Stack size in bytes 
        NULL,           // Task input parameter
        5,              // Priority of the task 
        NULL,           // Task handle (not used)
        0               // Core 0
    );
    xTaskCreatePinnedToCore(toggle_led2, "led_task2", 2048, NULL, 5, NULL, 1);
    ESP_LOGI("led_light", "[INFO] LED demo done.");
}