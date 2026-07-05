#include "imu.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "IMU";
static i2c_master_dev_handle_t dev_handle;// Specific name for device MPU



esp_err_t imu_init(i2c_master_bus_handle_t bus_handle) {
    
    // Register MPU-6050 as a device on the bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = MPU6050_ADDR,
        .scl_speed_hz    = 400000,
    };


    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    // Wake up MPU-6050
    uint8_t wake_cmd[] = {MPU6050_REG_PWR, 0x00};
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, wake_cmd, sizeof(wake_cmd), 100));

    // Set gyro range to ±2000 deg/s
    uint8_t gyro_cfg[] = {MPU6050_REG_CONFIG, 0x18};
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, gyro_cfg, sizeof(gyro_cfg), 100));

    ESP_LOGI(TAG, "MPU-6050 initialized");
    return ESP_OK;
};

esp_err_t imu_read (imu_data_t *data) {
    //Im pointing to data here, reading from reg 0x43 which is the gyro reg
    uint8_t reg = MPU6050_REG_GYRO; 
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle,&reg, 1, 100));
   
    //
    uint8_t raw[6];
    ESP_ERROR_CHECK(i2c_master_receive(dev_handle, raw, sizeof(raw), 100));

    // Combine high and low bytes for each axis
    int16_t raw_x = (int16_t)((raw[0] << 8) | raw[1]);
    int16_t raw_y = (int16_t)((raw[2] << 8) | raw[3]);
    int16_t raw_z = (int16_t)((raw[4] << 8) | raw[5]);

    // Convert raw counts to degrees/second
    data->gyro_x = raw_x / 16.4f;
    data->gyro_y = raw_y / 16.4f;
    data->gyro_z = raw_z / 16.4f;

    return ESP_OK;
    
};