#pragma once

#include <stdint.h>
#include "esp_now.h"
#include "esp_wifi.h"

typedef struct {
    uint8_t mac[6];
    float gyro_x;
    float gyro_y;
    float gyro_z;
    int64_t timestamp;
} espnow_packet_t;

esp_err_t espnow_init(void);