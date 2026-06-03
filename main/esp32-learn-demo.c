#include "led_light.h"
#include "esp_log.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <dht.h>
#include "display.h"
#include "wifi_manager.h"
#include "servo.h"

#define SENSOR_TYPE    DHT_TYPE_DHT11
#define DHT_GPIO       GPIO_NUM_4
#define TAG            "DHT11"

static TimerHandle_t alarm_timer = NULL;

static void alarm_timer_cb(TimerHandle_t xTimer) {
    led_off(GREEN_LED);
    led_on(RED_LED);
    // led_on(beep_gpio);
}

static void alarm_start(void) {
    if (!alarm_timer) {
        led_init(GREEN_LED);
        led_init(RED_LED);
        led_init(beep_gpio);
        alarm_timer = xTimerCreate("alarm", pdMS_TO_TICKS(500), pdTRUE, NULL, alarm_timer_cb);
    }
    xTimerStart(alarm_timer, 0);
}

static void alarm_stop(void) {
    if (alarm_timer) {
        xTimerStop(alarm_timer, 0);
        // xTimerDelete(alarm_timer, 0);
        // alarm_timer = NULL;
    }
    led_off(RED_LED);
    led_off(beep_gpio);
    led_on(GREEN_LED);
}

void dht_task(void *pvParameters) {
    int16_t temperature, humidity;

    while (1) {
        if (dht_read_data(SENSOR_TYPE, DHT_GPIO, &humidity, &temperature) == ESP_OK) {
            humidity /= 10;
            temperature /= 10;
            ESP_LOGI(TAG, "Humidity: %d%% Temp: %dC", humidity, temperature);

            if (temperature > 33 || humidity > 70) {
                alarm_start();
            } else {
                alarm_stop();
            }

            display_update_sensor(temperature, humidity);
        } else {
            ESP_LOGE(TAG, "Could not read data from sensor");
            alarm_start();
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    esp_log_level_set("*", ESP_LOG_WARN);

    display_sensor_run();
    xTaskCreate(dht_task, "dht_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    test_run_wifi();
}