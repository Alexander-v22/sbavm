#include "websocket.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <string.h>

static const char *TAG = "WEBSOCKET";
static httpd_handle_t server = NULL;
static int ws_client_fd = -1;

static esp_err_t ws_handler(httpd_req_t *req) {
    if (req -> method == HTTP_GET){
        ESP_LOGI(TAG, "WebSocket handshake done");
        ws_client_fd = httpd_req_to_sockfd(req);
        return ESP_OK; 
    }

    httpd_ws_frame_t ws_pkt = {};//struct
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to receive frame");
        return ret;
    }
    return ESP_OK;
}

esp_err_t websocket_init(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .is_websocket = true
    };

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &ws_uri));

    ESP_LOGI(TAG, "Websocket server started");
    return ESP_OK;
}

esp_err_t websocket_send(const char *json) {

    if (ws_client_fd == -1) {
        return ESP_FAIL;
    }

    httpd_ws_frame_t ws_pkt = {};
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = (uint8_t *)json;
    ws_pkt.len = strlen(json);

    return httpd_ws_send_frame_async(server, ws_client_fd, &ws_pkt);
}


