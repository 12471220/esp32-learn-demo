#include "led_light.h"
#include "esp_log.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <dht.h>
#include "display.h"

#define SENSOR_TYPE    DHT_TYPE_DHT11
#define DHT_GPIO       GPIO_NUM_4
#define TAG            "DHT11"

static TimerHandle_t alarm_timer = NULL;

static void alarm_timer_cb(TimerHandle_t xTimer)
{
    led_toggle(RED_LED);
}

static void alarm_start(void)
{
    if (!alarm_timer) {
        alarm_timer = xTimerCreate("alarm", pdMS_TO_TICKS(500), pdTRUE, NULL, alarm_timer_cb);
        xTimerStart(alarm_timer, 0);
    }
}

static void alarm_stop(void)
{
    if (alarm_timer) {
        xTimerStop(alarm_timer, 0);
        xTimerDelete(alarm_timer, 0);
        alarm_timer = NULL;
    }
    led_off(RED_LED);
}

void dht_task(void *pvParameters)
{
    int16_t temperature, humidity;

    while (1) {
        if (dht_read_data(SENSOR_TYPE, DHT_GPIO, &humidity, &temperature) == ESP_OK) {
            humidity /= 10;
            temperature /= 10;
            ESP_LOGI(TAG, "Humidity: %d%% Temp: %dC", humidity, temperature);

            if (temperature > 32 || humidity > 65) {
                alarm_start();
                led_on(beep_gpio);
                led_off(GREEN_LED);
            } else {
                alarm_stop();
                led_off(beep_gpio);
                led_on(GREEN_LED);
            }
        } else {
            ESP_LOGE(TAG, "Could not read data from sensor");
            alarm_start();
            led_on(beep_gpio);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    led_init(GREEN_LED);
    led_init(RED_LED);
    led_init(beep_gpio);

    // xTaskCreate(dht_task, "dht_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    // display_test_run();
    display_lvgl_test_run();
}
