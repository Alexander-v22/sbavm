#include "peak_detection.h"
#include "esp_log.h"
#include <math.h>
#include <stdbool.h>

#define PEAK_THRESHOLD_DPS 200.0f
#define PUNCH_TIMEOUT_MS 800
#define PEAK_COOLDOWN_MS 150



typedef enum {
    WAITING_FOR_ANKLE,
    WAITING_FOR_WAIST,
    WAITING_FOR_WRIST
} punch_state_t;

static punch_state_t state = WAITING_FOR_ANKLE;
static const char *pending_punch_type = "unknown";

static float prev_magnitude[5] = {0, 0, 0, 0, 0};

static int64_t ankle_peak_time = 0;
static int64_t waist_peak_time = 0;
static int64_t wrist_peak_time = 0;
static int64_t last_peak_time[5] = {0, 0, 0, 0, 0};

static bool punch_ready = false;
static float pending_dt1 = 0;
static float pending_dt2 = 0;

void peak_detection_update(int node_idx, float gyro_x, float gyro_y, float gyro_z, int64_t timestamp) {
    
    float magnitude = sqrtf(gyro_x*gyro_x + gyro_y*gyro_y + gyro_z*gyro_z);
    
    bool is_peak = (magnitude < prev_magnitude[node_idx]) && 
               (prev_magnitude[node_idx] > PEAK_THRESHOLD_DPS) &&
               ((timestamp - last_peak_time[node_idx]) > (PEAK_COOLDOWN_MS * 1000));

    if (is_peak) {
        last_peak_time[node_idx] = timestamp;
    }      
    
    prev_magnitude[node_idx] = magnitude;

    int64_t now = timestamp;
    if (state == WAITING_FOR_WAIST && (now - ankle_peak_time) > (PUNCH_TIMEOUT_MS * 1000)) {
        state = WAITING_FOR_ANKLE;
    }
    if (state == WAITING_FOR_WRIST && (now - waist_peak_time) > (PUNCH_TIMEOUT_MS * 1000)) {
        state = WAITING_FOR_ANKLE;
    }
    
    if (!is_peak) {
        if (magnitude > 150.0f) {
            //ESP_LOGI("PEAK_DEBUG", "node:%d mag:%.1f is_peak:0 state:%d", node_idx, magnitude, state);
            ESP_LOGI("AXIS_DEBUG", "peaked_node:%d gx:%.1f gy:%.1f gz:%.1f", 
             node_idx, gyro_x, gyro_y, gyro_z);
        }
        return;
    }
    
    if (state == WAITING_FOR_ANKLE && (node_idx == 0 || node_idx == 3)) {
        ankle_peak_time = timestamp;
        state = WAITING_FOR_WAIST;
    }
    else if (state == WAITING_FOR_WAIST && node_idx == 1) {
        waist_peak_time = timestamp;
        state = WAITING_FOR_WRIST;
    }
    else if (state == WAITING_FOR_WRIST && (node_idx == 2 || node_idx == 4)) {
        wrist_peak_time = timestamp;

        pending_punch_type = (node_idx == 2) ? "cross" : "jab";

        // need to tell what kind of punch where throwing 
        ESP_LOGI("AXIS_DEBUG", "peaked_node:%d gx:%.1f gy:%.1f gz:%.1f", 
             node_idx, gyro_x, gyro_y, gyro_z);

        pending_dt1 = (waist_peak_time - ankle_peak_time) / 1000.0f;
        pending_dt2 = (wrist_peak_time - waist_peak_time) / 1000.0f;
        punch_ready = true;
        state = WAITING_FOR_ANKLE;
    }

    //ESP_LOGI("PEAK_DEBUG", "node:%d mag:%.1f is_peak:1 state_after:%d", node_idx, magnitude, state);
}



bool peak_detection_check_punch (float *dt1_ms, float *dt2_ms, const char **punch_type_out){
    if (!punch_ready) {
        return false;
    }
    
    *dt1_ms = pending_dt1;
    *dt2_ms = pending_dt2;
    *punch_type_out = pending_punch_type;

    
    punch_ready = false;
    
    return true;

}