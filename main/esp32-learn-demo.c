#include "display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app";

void app_main(void)
{
    ESP_LOGI(TAG, "initializing display...");
    ESP_ERROR_CHECK(display_init());

    // Cycle through primary colors
    while (1) {
        ESP_LOGI(TAG, "red");
        ESP_ERROR_CHECK(display_fill(COLOR_RED));
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "green");
        ESP_ERROR_CHECK(display_fill(COLOR_GREEN));
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "blue");
        ESP_ERROR_CHECK(display_fill(COLOR_BLUE));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
