#include "tof_sensor.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>

// Use the real driver when the SparkFun VL53L5CX Arduino library is installed.
// Install via Library Manager: "SparkFun VL53L5CX Arduino Library".
#if __has_include(<SparkFun_VL53L5CX_Library.h>)
  #define TOF_HAS_DRIVER 1
  #include <Wire.h>
  #include <SparkFun_VL53L5CX_Library.h>
#else
  #define TOF_HAS_DRIVER 0
#endif

#if TOF_HAS_DRIVER
static SparkFun_VL53L5CX s_tof;
static VL53L5CX_ResultsData s_results;
#endif

static bool     s_ready         = false;
static ToF_Frame s_last         = {0};
static uint16_t s_closest_mm    = 0xFFFF;
static uint16_t s_frame_counter = 0;

bool ToF_Init(void) {
#if TOF_HAS_DRIVER
    // The Waveshare 7B shares Wire on pins 8/9. The main sketch runs
    // DEV_I2C_Init() first, which configures the ESP-IDF master driver;
    // SparkFun's library uses the Arduino Wire wrapper, so we re-Begin()
    // here just for the ToF. The ESP-IDF + Wire co-exist on the same pins
    // because Wire re-uses I2C_NUM_0 internally — the ESP-IDF v5 port is
    // reference-counted.
    Wire.begin(WS_7B_I2C_SDA, WS_7B_I2C_SCL, 400000);

    Serial.println("ToF: uploading firmware (this takes ~2s)...");
    if (!s_tof.begin()) {
        Serial.println("ToF: VL53L5CX not detected on I2C");
        return false;
    }

    s_tof.setResolution(TOF_GRID_W * TOF_GRID_H);   // 8x8
    s_tof.setRangingFrequency(15);                  // Hz, max for 8x8
    s_tof.setRangingMode(SF_VL53L5CX_RANGING_MODE::CONTINUOUS);
    s_tof.setSharpenerPercent(20);                  // Sharpen zone edges
    s_tof.startRanging();

    s_ready = true;
    Serial.println("ToF: ranging at 15 Hz, 8x8");
    return true;
#else
    Serial.println("ToF: driver library not present, running in mock mode");
    s_ready = true;
    return false;
#endif
}

bool ToF_Poll(ToF_Frame* out) {
    if (!out || !s_ready) return false;

#if TOF_HAS_DRIVER
    if (!s_tof.isDataReady()) return false;
    if (!s_tof.getRangingData(&s_results)) return false;

    uint16_t closest = 0xFFFF;
    for (int i = 0; i < TOF_GRID_ZONES; i++) {
        out->zones[i].distance_mm = s_results.distance_mm[i];
        out->zones[i].reflectance = (uint8_t)min(s_results.reflectance[i], (uint8_t)255);
        out->zones[i].status      = s_results.target_status[i];
        if (out->zones[i].status == 5 || out->zones[i].status == 9) {
            if (out->zones[i].distance_mm < closest) closest = out->zones[i].distance_mm;
        }
    }
    out->valid    = true;
    out->frame_ms = millis();
    out->frame_id = ++s_frame_counter;

    s_closest_mm = closest;
    memcpy(&s_last, out, sizeof(ToF_Frame));
    return true;
#else
    // Deterministic mock: sinusoidal terrain with a moving obstacle. Keeps the
    // mapping view animated on a bench without hardware attached.
    static uint32_t s_last_mock_ms = 0;
    uint32_t now = millis();
    if (now - s_last_mock_ms < 66) return false;    // 15 Hz
    s_last_mock_ms = now;

    float t = now * 0.001f;
    uint16_t closest = 0xFFFF;
    for (int row = 0; row < TOF_GRID_H; row++) {
        for (int col = 0; col < TOF_GRID_W; col++) {
            int idx = row * TOF_GRID_W + col;
            // Baseline: distance grows from 800 mm at the top row (near) to
            // 3500 mm at the bottom row (far), with a fake bump that travels.
            float base = 800.0f + row * 380.0f;
            float bump = 600.0f * expf(-powf(col - (3.5f + 3.0f * sinf(t)), 2) * 0.6f)
                              * expf(-powf(row - 4.0f, 2) * 0.4f);
            uint16_t d = (uint16_t)(base - bump + 30.0f * sinf(t * 2.0f + row));
            out->zones[idx].distance_mm = d;
            out->zones[idx].reflectance = 180;
            out->zones[idx].status      = 5;
            if (d < closest) closest = d;
        }
    }
    out->valid    = true;
    out->frame_ms = now;
    out->frame_id = ++s_frame_counter;

    s_closest_mm = closest;
    memcpy(&s_last, out, sizeof(ToF_Frame));
    return true;
#endif
}

uint16_t ToF_ClosestMm(void) {
    return s_closest_mm;
}
