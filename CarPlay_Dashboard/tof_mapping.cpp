#include "tof_mapping.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Accumulated world-frame offset of the vehicle since the grid origin was
// last snapped. When this offset crosses one cell, we shift the whole grid.
static float s_world_dx_m = 0.0f;
static float s_world_dy_m = 0.0f;

void ToF_Map_Init(ToF_Map* map) {
    if (!map) return;
    memset(map, 0, sizeof(*map));
    map->closest_obstacle_mm = 0xFFFF;
    s_world_dx_m = 0.0f;
    s_world_dy_m = 0.0f;
}

static void shift_grid(ToF_Map* map, int dcol, int drow) {
    if (dcol == 0 && drow == 0) return;
    // Copy-from-source with bounds check. For small shifts (<= 2 cells per
    // frame typical) this is cheap even at 120x80.
    ToF_MapCell next[TOF_MAP_W * TOF_MAP_H];
    memset(next, 0, sizeof(next));
    for (int r = 0; r < TOF_MAP_H; r++) {
        int sr = r + drow;
        if (sr < 0 || sr >= TOF_MAP_H) continue;
        for (int c = 0; c < TOF_MAP_W; c++) {
            int sc = c + dcol;
            if (sc < 0 || sc >= TOF_MAP_W) continue;
            next[r * TOF_MAP_W + c] = map->cells[sr * TOF_MAP_W + sc];
        }
    }
    memcpy(map->cells, next, sizeof(next));
}

void ToF_Map_Integrate(ToF_Map* map,
                       const ToF_Frame* frame,
                       float speed_mph,
                       float heading_deg,
                       float dt_s) {
    if (!map || !frame || !frame->valid) return;

    const float speed_mps = speed_mph * 0.44704f;
    const float heading   = heading_deg * (float)M_PI / 180.0f;
    map->vehicle_speed_mps   = speed_mps;
    map->vehicle_heading_rad = heading;

    // Step 1 — accumulate ego-motion in world frame.
    s_world_dx_m += speed_mps * dt_s * cosf(heading);
    s_world_dy_m += speed_mps * dt_s * sinf(heading);

    // Step 2 — snap the grid whenever we've moved a full cell.
    int shift_cols = (int)(s_world_dy_m / TOF_MAP_CELL_M);
    int shift_rows = (int)(s_world_dx_m / TOF_MAP_CELL_M);
    if (shift_cols != 0 || shift_rows != 0) {
        // Grid convention: +row = further ahead, +col = further left. Since
        // world +x is forward, forward motion should pull the map *backwards*
        // relative to the vehicle — shift rows *up* (negative dr) accordingly.
        shift_grid(map, shift_cols, -shift_rows);
        s_world_dx_m -= shift_rows * TOF_MAP_CELL_M;
        s_world_dy_m -= shift_cols * TOF_MAP_CELL_M;
    }

    // Step 3 — project each zone into the vehicle frame and drop it into
    // the grid. We treat the vehicle origin as the cell at (W/2, H/2).
    const int origin_col = TOF_MAP_W / 2;
    const int origin_row = TOF_MAP_H / 2;

    uint16_t best_mm = 0xFFFF;
    float best_bearing = 0.0f;

    const float half_fov_h = TOF_FOV_H_RAD * 0.5f;
    const float half_fov_v = TOF_FOV_V_RAD * 0.5f;
    const float sin_p = sinf(TOF_MOUNT_PITCH);
    const float cos_p = cosf(TOF_MOUNT_PITCH);

    for (int row = 0; row < TOF_GRID_H; row++) {
        for (int col = 0; col < TOF_GRID_W; col++) {
            const ToF_Zone* z = &frame->zones[row * TOF_GRID_W + col];
            if (z->status != 5 && z->status != 9) continue;
            if (z->distance_mm == 0 || z->distance_mm > 3800) continue;

            // Azimuth / elevation of this zone's ray (sensor frame, +x forward).
            float az = ((col + 0.5f) / TOF_GRID_W - 0.5f) * TOF_FOV_H_RAD;
            float el = ((row + 0.5f) / TOF_GRID_H - 0.5f) * TOF_FOV_V_RAD;
            // Row 0 is top of the sensor (pointing up-ish); invert.
            el = -el;

            float d = z->distance_mm * 0.001f;      // metres
            // Ray in sensor frame.
            float sx = d * cosf(el) * cosf(az);
            float sy = d * cosf(el) * sinf(az);
            float sz = d * sinf(el);
            // Apply mount pitch (rotation about Y-axis).
            float vx =  sx * cos_p + sz * sin_p;
            float vz = -sx * sin_p + sz * cos_p;
            float vy =  sy;
            // Translate to vehicle origin.
            vx += TOF_MOUNT_X_M;
            vy += TOF_MOUNT_Y_M;
            vz += TOF_MOUNT_Z_M;

            // Reject points that are clearly sky (above +2 m) or under-road
            // echoes (below -0.3 m). Everything in between is terrain.
            if (vz > 2.0f || vz < -0.3f) continue;

            int cell_col = origin_col - (int)roundf(vy / TOF_MAP_CELL_M);
            int cell_row = origin_row + (int)roundf(vx / TOF_MAP_CELL_M);
            ToF_MapCell* cell = ToF_Map_At(map, cell_col, cell_row);
            if (!cell) continue;

            int height_cm = (int)roundf(vz * 100.0f);
            if (height_cm < -128) height_cm = -128;
            if (height_cm >  127) height_cm =  127;

            // Classification: ground = within ±10 cm of road plane, obstacle
            // = above 15 cm. "Hazard" bumps any obstacle within 2 m.
            uint8_t kind = TOF_CELL_EMPTY;
            if (height_cm >  15) kind = TOF_CELL_OBSTACLE;
            else if (height_cm >= -15) kind = TOF_CELL_GROUND;
            if (kind == TOF_CELL_OBSTACLE && d < 2.0f) kind = TOF_CELL_HAZARD;

            cell->kind       = kind;
            cell->height_cm  = (int8_t)height_cm;
            cell->confidence = z->reflectance;
            cell->age_frames = 0;

            if (kind >= TOF_CELL_OBSTACLE && z->distance_mm < best_mm) {
                best_mm = z->distance_mm;
                best_bearing = az;
            }
        }
    }

    map->frames_integrated++;
    map->closest_obstacle_mm          = best_mm;
    map->closest_obstacle_bearing_rad = best_bearing;
}

void ToF_Map_Decay(ToF_Map* map) {
    if (!map) return;
    for (int i = 0; i < TOF_MAP_W * TOF_MAP_H; i++) {
        ToF_MapCell* c = &map->cells[i];
        if (c->kind == TOF_CELL_EMPTY) continue;
        c->age_frames++;
        if (c->age_frames > 60) {
            c->kind = TOF_CELL_EMPTY;
            c->confidence = 0;
        } else if (c->confidence > 3) {
            c->confidence -= 3;
        }
    }
}
