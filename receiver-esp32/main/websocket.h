#pragma once

#include "esp_err.h"
#include "espnow.h"

esp_err_t websocket_init(void);
esp_err_t websocket_send(const char *json);