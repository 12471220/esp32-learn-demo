#include "wifi_manager.h"

#include "display.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "wifi_manager";

static char wifi_ip_str[16] = {0};
static bool wifi_connected = false;

/* --- HTTP server --- */
static esp_err_t http_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "HTTP request: %s %s", http_method_str(req->method), req->uri);
    const char *resp = "OK\n";
    httpd_resp_send(req, resp, strlen(resp));
    ESP_LOGI(TAG, "HTTP response responded.");
    return ESP_OK;
}

static void http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8000;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = http_handler,
        };
        httpd_register_uri_handler(server, &uri);
        ESP_LOGI(TAG, "HTTP server started on port 8000");
    } else {
        ESP_LOGE(TAG, "HTTP server start failed");
    }
}

/* --- RSSI ring buffer with cached sum --- */
struct wifi_rssi_ring {
    int *buf;
    int head;      // next write position
    int count;
    int sum;       // cached sum
    int max_size;
};

static struct wifi_rssi_ring *rssi_ring_init(int max_size)
{
    struct wifi_rssi_ring *ring = malloc(sizeof(struct wifi_rssi_ring));
    ring->buf = malloc(max_size * sizeof(int));
    ring->head = 0;
    ring->count = 0;
    ring->sum = 0;
    ring->max_size = max_size;
    return ring;
}

static void rssi_ring_add(struct wifi_rssi_ring *ring, int rssi)
{
    if (ring->count == ring->max_size) {
        // evict oldest: position (head - count) wrapped
        int oldest_idx = (ring->head - ring->count + ring->max_size) % ring->max_size;
        ring->sum -= ring->buf[oldest_idx];
        ring->count--;
    }
    ring->buf[ring->head] = rssi;
    ring->sum += rssi;
    ring->head = (ring->head + 1) % ring->max_size;
    ring->count++;
}

static int rssi_ring_avg(const struct wifi_rssi_ring *ring)
{
    if (ring->count == 0) return 0;
    return ring->sum / ring->count;
}

static void wifi_poll_task(void *pvParameters)
{
    struct wifi_rssi_ring *ring = rssi_ring_init(50);
    while (1) {
        if (wifi_connected) {
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                rssi_ring_add(ring, ap_info.rssi);
                display_update_wifi(rssi_ring_avg(ring), wifi_ip_str);
            }
        } else {
            display_update_wifi(0, NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* --- WiFi event handler --- */

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        display_update_wifi(0, NULL);
        ESP_LOGW(TAG, "disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(wifi_ip_str, sizeof(wifi_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        ESP_LOGI(TAG, "got IP: %s", wifi_ip_str);

        /* Start services after getting IP */
        http_server_start();
    }
}

/* --- Init --- */

void wifi_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "connecting to SSID: %s", ESP_WIFI_SSID);
}

void test_run_wifi(void)
{
    wifi_init();
    xTaskCreate(wifi_poll_task, "wifi_poll", 4096, NULL, 3, NULL);
}
