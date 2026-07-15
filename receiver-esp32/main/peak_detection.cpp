#include "peak_detection.h"
#include <math.h>
#include <stdbool.h>

#define PEAK_THRESHOLD_DPS 200.0f
#define PUNCH_TIMEOUT_MS 800


typedef enum {
    WAITING_FOR_ANKLE,
    WAITING_FOR_WAIST,
    WAITING_FOR_WRIST
} punch_state_t;

static punch_state_t state = WAITING_FOR_ANKLE;

static float prev_magnitude[3] = {0, 0, 0};

static int64_t ankle_peak_time = 0;
static int64_t waist_peak_time = 0;
static int64_t wrist_peak_time = 0;

static bool punch_ready = false;
static float pending_dt1 = 0;
static float pending_dt2 = 0;

void peak_detection_update(int node_idx, float gyro_x, float gyro_y, float gyro_z, int64_t timestamp) {
    
    float magnitude = sqrtf(gyro_x*gyro_x + gyro_y*gyro_y + gyro_z*gyro_z);
    
    bool is_peak = (magnitude < prev_magnitude[node_idx]) && 
                   (prev_magnitude[node_idx] > PEAK_THRESHOLD_DPS);
    
    prev_magnitude[node_idx] = magnitude;
    
    if (!is_peak) return;
    
    // A peak was detected on this node — check if it matches what we're waiting for
    if (state == WAITING_FOR_ANKLE && node_idx == 0) {
        ankle_peak_time = timestamp;
        state = WAITING_FOR_WAIST;
    }
    else if (state == WAITING_FOR_WAIST && node_idx == 1) {
        waist_peak_time = timestamp;
        state = WAITING_FOR_WRIST;
    }
    else if (state == WAITING_FOR_WRIST && node_idx == 2) {
        wrist_peak_time = timestamp;
        
        // /1000 to go from micro to milli
        pending_dt1 = (waist_peak_time - ankle_peak_time) / 1000.0f;
        pending_dt2 = (wrist_peak_time - waist_peak_time) / 1000.0f;
        punch_ready = true;
        
        state = WAITING_FOR_ANKLE;
    }
    
    // Timeout check
    int64_t now = timestamp;
    if (state == WAITING_FOR_WAIST && (now - ankle_peak_time) > (PUNCH_TIMEOUT_MS * 1000)) {
        state = WAITING_FOR_ANKLE;
    }
    if (state == WAITING_FOR_WRIST && (now - waist_peak_time) > (PUNCH_TIMEOUT_MS * 1000)) {
        state = WAITING_FOR_ANKLE;
    }
}

bool peak_detection_check_punch (float *dt1_ms, float *dt2_ms){
    if (!punch_ready) {
        return false;
    }
    
    *dt1_ms = pending_dt1;
    *dt2_ms = pending_dt2;
    
    punch_ready = false;
    
    return true;

}