#include "espnow.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"


static const char *TAG = "ESPNOW";
// Define receiver MAC here - read your ESP32 receiver MAC and fill this in
uint8_t receiver_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


esp_err_t espnow_init(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); //
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); //STA == Station Mode chip acts as wifi client rater than an access point
    ESP_ERROR_CHECK(esp_wifi_start());

    // forces every XIAO to explicity transmit on channel 1 this matches the reciers wifi config
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_now_init());

    esp_now_peer_info_t peer = {}; // struct sitting in memory
    memcpy(peer.peer_addr, receiver_mac, 6);
    peer.channel = 0;
    peer.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));  // pass address of that struct

    ESP_LOGI(TAG, "ESP-NOW initialized");
    return ESP_OK;
}

esp_err_t espnow_send_imu(const imu_data_t *data) {
    espnow_packet_t packet;

    esp_read_mac(packet.mac, ESP_MAC_WIFI_STA);

    packet.gyro_x = data->gyro_x;
    packet.gyro_y = data->gyro_y;
    packet.gyro_z = data->gyro_z;

    packet.timestamp = esp_timer_get_time();

    esp_err_t ret = esp_now_send(receiver_mac, (uint8_t *)&packet, sizeof(packet));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG,"ESP-NOW send failed: %s", esp_err_to_name(ret));
    }
    return ret;
}