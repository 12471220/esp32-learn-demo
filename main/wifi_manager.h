#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_WIFI_SSID    "Open Internet"
#define ESP_WIFI_PASS    "helloworld"
#define ESP_WIFI_CHANNEL 1
#define MAX_STA_CONN     2

void test_run_wifi(void);

#ifdef __cplusplus
}
#endif
