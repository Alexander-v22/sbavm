#include "espnow.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "websocket.h"
#include "peak_detection.h"
#include <stdio.h>
 #include <math.h>
 

#define CHANGE_THRESHOLD 5.0f  // degrees/second — tune this later



    static const char *TAG = "ESPNOW_RX";

static const uint8_t ankle_right_mac[6] = {0xac, 0x27, 0x6e, 0x7e, 0x19, 0x1c};
static const uint8_t waist_mac[6]       = {0xac, 0x27, 0x6e, 0x7e, 0x20, 0x24};
static const uint8_t wrist_right_mac[6] = {0xac, 0x27, 0x6e, 0x7e, 0xa3, 0x8c};
static const uint8_t ankle_left_mac[6]  = {0xac, 0x27, 0x6e, 0x7c, 0x13, 0x08};
static const uint8_t wrist_left_mac[6]  = {0xac, 0x27, 0x6e, 0x7f, 0x95, 0xd8};




static const char* mac_to_node_name(const uint8_t *mac) {
    if (memcmp(mac, ankle_right_mac, 6) == 0) return "ankle_right";
    if (memcmp(mac, waist_mac, 6) == 0) return "waist";
    if (memcmp(mac, wrist_right_mac, 6) == 0) return "wrist_right";
    if (memcmp(mac, ankle_left_mac, 6) == 0) return "ankle_left";
    if (memcmp(mac, wrist_left_mac, 6) == 0) return "wrist_left";
    return "unknown";
}

// ankle_right, waist, wrist_right, ankle_left, wrist_left
static float last_sent_magnitude[5] = {0, 0, 0, 0, 0};  

static int node_name_to_index(const char *node_name){
    if (strcmp(node_name, "ankle_right") == 0) return 0;
    if (strcmp(node_name, "waist") == 0) return 1;
    if (strcmp(node_name, "wrist_right") == 0) return 2;
    if (strcmp(node_name, "ankle_left") == 0) return 3;
    if (strcmp(node_name, "wrist_left") == 0) return 4;
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
    if (idx == -1) return;

    // Peak detection runs on EVERY sample, regardless of bandwidth filtering below
    int64_t receiver_timestamp = esp_timer_get_time();
    peak_detection_update(idx, packet.gyro_x, packet.gyro_y, packet.gyro_z, receiver_timestamp);

    float dt1, dt2;
    const char *punch_type; //New Addition to this block so that we can now classify punch tpye
    if (peak_detection_check_punch(&dt1, &dt2, &punch_type)) {
        char punch_json[200];
        snprintf(punch_json, sizeof(punch_json),
                "{\"type\":\"punch\",\"punch_type\":\"%s\",\"dt1\":%.1f,\"dt2\":%.1f,\"timestamp\":%lld,\"valid\":true}",
                punch_type, dt1, dt2, packet.timestamp);
        websocket_send(punch_json);
        ESP_LOGI(TAG, "PUNCH DETECTED: type=%s dt1=%.1fms dt2=%.1fms", punch_type, dt1, dt2);
    }

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

