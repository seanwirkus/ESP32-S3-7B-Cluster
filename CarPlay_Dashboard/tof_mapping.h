#ifndef __TOF_MAPPING_H
#define __TOF_MAPPING_H

#include "tof_sensor.h"
#include <stdint.h>
#include <stdbool.h>

// ToF 3D Mapping — accumulate a rolling point cloud of the road ahead.
//
// Pipeline per frame:
//   1. Each of the 64 ToF zones casts a ray in the sensor frame (az, el).
//   2. Ray is multiplied by the zone distance -> point in sensor coords.
//   3. Point is transformed into *vehicle* coords (fixed sensor mount).
//   4. Point is integrated in *world* coords using dead-reckoning:
//        world_x += speed_mps * dt * cos(heading_rad)
//        world_y += speed_mps * dt * sin(heading_rad)
//      so successive frames stack into a coherent trail behind/ahead of the
//      car as it drives. No IMU needed — the dashboard already tracks speed
//      and compass heading, which is enough for <30 s map persistence.
//   5. Point cloud is downsampled into a 2D occupancy / elevation grid
//      centred on the vehicle: 120 cells × 80 cells at 10 cm resolution
//      (12 m × 8 m window).
//
// The cluster renders both the raw 8x8 ToF frame (as a heatmap) *and* the
// accumulated top-down grid, giving the driver a short-horizon "3D scan" of
// what was just traversed plus what's directly ahead.

#define TOF_MAP_W 120
#define TOF_MAP_H 80
#define TOF_MAP_CELL_M 0.10f          // 10 cm per cell -> 12.0 m × 8.0 m

// Sensor mounting geometry in vehicle coordinates (metres / radians).
// Vehicle frame: +x forward, +y left, +z up. Origin at rear axle, ground.
#define TOF_MOUNT_X_M    0.40f        // 40 cm forward of rear axle proxy
#define TOF_MOUNT_Y_M    0.00f
#define TOF_MOUNT_Z_M    0.70f        // 70 cm above ground
#define TOF_MOUNT_PITCH  (-0.15f)     // Pitched ~8.6° down

// FOV per zone. VL53L5CX = 45° H × 45° V total, split across 8x8.
#define TOF_FOV_H_RAD    0.7854f      // pi/4
#define TOF_FOV_V_RAD    0.7854f

typedef enum {
    TOF_CELL_EMPTY = 0,
    TOF_CELL_GROUND,
    TOF_CELL_OBSTACLE,
    TOF_CELL_HAZARD,   // Obstacle + closing rapidly
} ToF_CellKind;

typedef struct {
    uint8_t kind;           // ToF_CellKind
    int8_t  height_cm;      // Height above ground plane, -128..+127
    uint8_t confidence;     // 0..255, decays with age
    uint8_t age_frames;     // Frames since last update
} ToF_MapCell;

typedef struct {
    ToF_MapCell cells[TOF_MAP_W * TOF_MAP_H];
    uint32_t    frames_integrated;
    float       vehicle_speed_mps;
    float       vehicle_heading_rad;
    uint16_t    closest_obstacle_mm;
    float       closest_obstacle_bearing_rad;
} ToF_Map;

// Reset the rolling grid.
void ToF_Map_Init(ToF_Map* map);

// Integrate a new ToF frame. speed_mph + heading_deg come straight from the
// vehicle state the dashboard already owns. dt_s is the delta since the
// previous integrate call; the helper shifts the grid to keep the vehicle at
// a fixed origin cell.
void ToF_Map_Integrate(ToF_Map* map,
                       const ToF_Frame* frame,
                       float speed_mph,
                       float heading_deg,
                       float dt_s);

// Age / decay existing cells — call once per render. Cells older than
// ~60 frames (~4 s at 15 Hz) are reset to EMPTY.
void ToF_Map_Decay(ToF_Map* map);

// Access helpers — row 0 is behind the car, row TOF_MAP_H/2 is at the rear
// axle, rows above are the road ahead. Column TOF_MAP_W/2 is centreline.
static inline ToF_MapCell* ToF_Map_At(ToF_Map* m, int col, int row) {
    if (col < 0 || col >= TOF_MAP_W || row < 0 || row >= TOF_MAP_H) return 0;
    return &m->cells[row * TOF_MAP_W + col];
}

#endif /* __TOF_MAPPING_H */
