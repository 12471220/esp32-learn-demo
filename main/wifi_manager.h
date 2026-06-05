#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_WIFI_SSID    "Open Internet"
#define ESP_WIFI_PASS    "helloworld"
#define ESP_WIFI_CHANNEL 1
#define MAX_STA_CONN     2

void wifi_run(void);

#ifdef __cplusplus
}
#endif
