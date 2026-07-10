#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "imu.h"
#include "espnow.h"
#include "esp_mac.h"

static const char *TAG = "MAIN";


static void imu_task(void *pvParameters) {
    imu_data_t data;

    while(1){
        esp_err_t ret = imu_read(&data);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG,"gx:%.2f gy:%.2f gz:%.2f", 
                     data.gyro_x, data.gyro_y, data.gyro_z);
            espnow_send_imu(&data);
        }
        vTaskDelay(pdMS_TO_TICKS(5)); // 200Hz sample rate
    }
}

extern "C" void app_main(void) {
    uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        ESP_LOGI(TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", 
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Create I2C bus
    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_6,
        .scl_io_num = GPIO_NUM_7,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    }; bus_cfg.flags.enable_internal_pullup = true;


    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    // Init IMU
    ESP_ERROR_CHECK(imu_init(bus_handle));
    //espnow init
    ESP_ERROR_CHECK(espnow_init());


    // Start IMU task
   xTaskCreate(imu_task,    // which function to run as a task
            "imu_task",     // name
            4096,           // stack size in bytes allocated for this task
            NULL,           // parameters to pass in 
            5,              // priority 
            NULL);          // task handle
}

