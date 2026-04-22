#ifndef __GT911_TOUCH_H
#define __GT911_TOUCH_H

#include <stdint.h>
#include <stdbool.h>
#include "io_extension.h"

// Waveshare 7B touch wiring constants
#define WS_7B_GT911_ADDR         (0x5D)
#define WS_7B_IOEXT_TP_RST       IO_EXTENSION_IO_1
#define WS_7B_IOEXT_TP_INT       IO_EXTENSION_IO_3

// GT911 capacitive touch driver for the Waveshare ESP32-S3 7B.
// Shares the main I2C bus (pins 8/9) at 400 kHz. Reset + interrupt are
// wired through the TCA9554 IO expander — see rgb_lcd_port.h.
//
// The 7B reports up to 5 simultaneous contacts on a 1024x600 surface;
// the dashboard only reacts to the first point (tap-to-switch-view).

#define GT911_MAX_POINTS 5

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t size;       // Reported touch footprint
    uint8_t track_id;   // GT911 assigns a stable ID per finger
} GT911_Point;

typedef struct {
    uint8_t count;                         // 0..GT911_MAX_POINTS
    GT911_Point points[GT911_MAX_POINTS];
    uint32_t frame_ms;                     // millis() at capture time
} GT911_Frame;

// Brings the controller out of reset through the IO expander and confirms the
// product ID. Returns true when a GT911 responds on 0x5D.
bool GT911_Init(void);

// Non-blocking poll — returns true only on a new frame with count > 0.
// Drop-in safe inside the main render loop; takes ~0.4 ms on the shared bus.
bool GT911_Poll(GT911_Frame* out);

// Single-tap helper: reports a tap (press-and-release within 400 ms, <20 px
// drift) and clears internal state. Intended for view switching.
bool GT911_ConsumeTap(int16_t* tap_x, int16_t* tap_y);

#endif /* __GT911_TOUCH_H */
