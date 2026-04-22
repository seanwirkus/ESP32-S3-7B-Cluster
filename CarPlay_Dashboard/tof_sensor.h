#ifndef __TOF_SENSOR_H
#define __TOF_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

// Forward-facing ToF driver — ST VL53L5CX (8x8 multi-zone, 4m range, 15 Hz).
// Intended mounting: behind the grille or windshield, ~0.7 m above road,
// pitched slightly down so the lower rows hit pavement ~2 m ahead at rest.
//
// The sensor's field of view is 45° horizontal / 45° vertical, giving each
// of the 64 zones a ~5.6° cone. Each zone reports a distance in mm plus a
// reflectance estimate, which the mapper uses to reject low-confidence pixels
// (rain, exhaust, open sky).
//
// Implementation note: the VL53L5CX requires a ~86 KB firmware blob to be
// uploaded over I2C on boot. We reuse SparkFun's VL53L5CX Arduino library
// when compiling for the board (it includes the blob). When the library is
// not present — e.g. a desktop unit-test build — a deterministic mock
// stream is produced so the rest of the stack stays testable.

#define TOF_GRID_W 8
#define TOF_GRID_H 8
#define TOF_GRID_ZONES (TOF_GRID_W * TOF_GRID_H)

// Per-zone result. Distance is in millimetres; 0xFFFF means "invalid".
typedef struct {
    uint16_t distance_mm;
    uint8_t  reflectance;   // 0..255, higher is cleaner return
    uint8_t  status;        // 0, 5, 9, 12 are "valid" per ST guide
} ToF_Zone;

typedef struct {
    ToF_Zone zones[TOF_GRID_ZONES];
    uint32_t frame_ms;
    uint16_t frame_id;
    bool     valid;
} ToF_Frame;

// One-time setup: uploads the firmware blob, switches to 8x8 ranging mode at
// 15 Hz, and kicks off continuous acquisition. Returns false if the sensor
// isn't on the bus (the dashboard will then render the mapping view with a
// "no ToF" placeholder instead of erroring out).
bool ToF_Init(void);

// Non-blocking poll — populates `out` only when a complete frame is ready.
// Safe to call every loop iteration.
bool ToF_Poll(ToF_Frame* out);

// Convenience: the single closest return across the grid, used by the HUD
// collision-warning tint. Returns 0xFFFF when no frame has arrived yet.
uint16_t ToF_ClosestMm(void);

#endif /* __TOF_SENSOR_H */
