#pragma once

#include <stdint.h>
#include "esp_now.h"
#include "esp_wifi.h"
#include "imu.h"


typedef struct {
    uint8_t mac[6];
    float gyro_x;
    float gyro_y;
    float gyro_z;
    int64_t timestamp;
}  espnow_packet_t;

extern uint8_t receiver_mac[6];

esp_err_t espnow_init(void);
esp_err_t espnow_send_imu(const imu_data_t *data);