#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "espnow.h"
#include "websocket.h"

static const char *TAG = "MAIN";

extern "C" void app_main(void) {

    // NVS init
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // TCP/IP stack init
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Event loop init
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Init ESP-NOW + WiFi AP (this now also creates the AP netif internally)
    ESP_ERROR_CHECK(espnow_init());

    // Init WebSocket server
    ESP_ERROR_CHECK(websocket_init());
    
    ESP_LOGI(TAG, "SBAVM receiver ready");
}