#include "espnow.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "websocket.h"
#include <stdio.h>
 #include <math.h>

#define CHANGE_THRESHOLD 5.0f  // degrees/second — tune this later



static const char *TAG = "ESPNOW_RX";

static const uint8_t ankle_mac[6] = {0xac, 0x27, 0x6e, 0x7e, 0x19, 0x1c};
static const uint8_t waist_mac[6] = {0xac, 0x27, 0x6e, 0x7e, 0x20, 0x24};
static const uint8_t wrist_mac[6] = {0xac, 0x27, 0x6e, 0x7e, 0xa3, 0x8c};

static const char* mac_to_node_name(const uint8_t *mac) {
    if (memcmp(mac, ankle_mac, 6) == 0) return "ankle";
    if (memcmp(mac, waist_mac, 6) == 0) return "waist";
    if (memcmp(mac, wrist_mac, 6) == 0) return "wrist";
    return "unknown";
}


static float last_sent_magnitude[3] = {0, 0, 0};  // ankle, waist, wrist

static int node_name_to_index(const char *node_name){
    if (strcmp(node_name, "ankle") == 0) return 0;
    if (strcmp(node_name, "waist") == 0) return 1;
    if (strcmp(node_name, "wirst") == 0) return 2;
    //strcmp is like memcmp but specifically for comparing C strings

    return -1;
}



static void espnow_recv_callback(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  
    if (len != sizeof(espnow_packet_t)) {
        ESP_LOGE(TAG, "Invalid packet size");
        return;
    }
    
    espnow_packet_t packet;
    memcpy(&packet, data, sizeof(espnow_packet_t));
    
    const char *node_name = mac_to_node_name(packet.mac);
    int idx = node_name_to_index(node_name); 
    //idx tells us which slot in the last_sent_magnitude[idx] reads/writes the right slot
    if (idx == -1) return;

    float magnitude = sqrtf(packet.gyro_x * packet.gyro_x + 
                         packet.gyro_y * packet.gyro_y + 
                         packet.gyro_z * packet.gyro_z);
    
    float change = fabsf(magnitude - last_sent_magnitude[idx]);
    if (change < CHANGE_THRESHOLD) {
        return;
    }
    
    last_sent_magnitude[idx] = magnitude;

    
    char json[200];
    snprintf(json, sizeof(json),
             "{\"type\":\"imu\",\"node\":\"%s\",\"gyro_x\":%.2f,\"gyro_y\":%.2f,\"gyro_z\":%.2f,\"timestamp\":%lld,\"peak\":false}",
             node_name, packet.gyro_x, packet.gyro_y, packet.gyro_z, packet.timestamp);
    
    websocket_send(json);
}

esp_err_t espnow_init(void) {
    
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.ap.ssid, "SBAVM-AP");
    wifi_config.ap.ssid_len = strlen("SBAVM-AP");
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); // 
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_callback));

    ESP_LOGI(TAG, "ESP-NOW receiver initialized");
    return ESP_OK;

}

