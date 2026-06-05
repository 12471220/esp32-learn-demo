#include "led_light.h"
#include "esp_log.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <dht.h>
#include "display.h"
#include "wifi_manager.h"
#include "servo.h"

#define SENSOR_TYPE    DHT_TYPE_AM2301
#define DHT_GPIO       GPIO_NUM_4
#define TAG            "DHT22"

static void alarm_start(void) {
    led_off(GREEN_LED);
    led_on(RED_LED);
}

static void alarm_stop(void) {
    led_off(RED_LED);
    led_off(beep_gpio);
    led_on(GREEN_LED);
}

void dht_task(void *pvParameters) {
    led_init(GREEN_LED);
    led_init(RED_LED);
    led_init(beep_gpio);

    float temperature, humidity;

    while (1) {
        if (dht_read_float_data(SENSOR_TYPE, DHT_GPIO, &humidity, &temperature) == ESP_OK) {
            ESP_LOGI(TAG, "Humidity: %.1f%% Temperature: %.1f°C", humidity, temperature);

            if (temperature > 33.0 || humidity > 70.0) {
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

    /* One-time servo init + async task */
    ESP_ERROR_CHECK(servo_init());
    servo_task_start();

    display_sensor_run();
    xTaskCreate(dht_task, "dht_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    wifi_run();
}