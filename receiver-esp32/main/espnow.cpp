#include "espnow.h"
#include "esp_log.h"
#include "esp_mac.h"

static const char *TAG = "ESPNOW_RX";

static void espnow_recv_callback(const esp_now_recv_info_t *info, const uint8_t *data, int len){

    if(len != sizeof(espnow_packet_t)) {
        ESP_LOGE(TAG, "Invalid packet size");
        return;
    } 

    espnow_packet_t packet; //empty struct created in local memory
    memcpy(&packet, data, sizeof(espnow_packet_t));
    
    ESP_LOGI(TAG,"MAC:%02x gyro_x:%.2f ts:%lld", packet.mac[0], packet.gyro_x, packet.timestamp);
}

esp_err_t espnow_init(void) {

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); // 
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_callback));

    ESP_LOGI(TAG, "ESP-NOW receiver initialized");
    return ESP_OK;

}