#pragma once

#include <stdint.h>
#include "esp_err.h"

// node_idx: 0 = ankle, 1 = waist, 2 = wrist
void peak_detection_update(int node_idx, float gyro_x, float gyro_y, float gyro_z, int64_t timestamp);

// Returns true if a punch cycle completed, and fills in the timing results
bool peak_detection_check_punch(float *dt1_ms, float *dt2_ms);