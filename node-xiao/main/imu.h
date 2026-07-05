#pragma once

#include <stdint.h>
#include "driver/i2c_master.h"

#define MPU6050_ADDR 0X68
#define MPU6050_REG_PWR 0X6B
#define MPU6050_REG_GYRO 0X43
#define MPU6050_REG_CONFIG 0X1B

typedef struct {
    float gyro_x;
    float gyro_y;
    float gyro_z;
} imu_data_t;


esp_err_t imu_init(i2c_master_bus_handle_t bus_handle);
esp_err_t imu_read(imu_data_t *data);

