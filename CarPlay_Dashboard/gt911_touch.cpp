#include "gt911_touch.h"
#include "i2c.h"
#include "io_extension.h"
#include "rgb_lcd_port.h"
#include <Arduino.h>
#include <string.h>

// --- GT911 register map (subset used by the dashboard) -----------------------
// The GT911 exposes 16-bit register addresses. All reads/writes are big-endian
// on the wire; ESP-IDF's i2c_master_transmit_receive handles that for us as
// long as we serialize the address MSB-first in the TX buffer.
#define GT911_REG_COMMAND      0x8040
#define GT911_REG_CONFIG_VER   0x8047
#define GT911_REG_PRODUCT_ID   0x8140
#define GT911_REG_STATUS       0x814E
#define GT911_REG_POINT1       0x814F

static i2c_master_dev_handle_t s_gt911 = NULL;

// Tap state
static bool     s_tap_armed        = false;
static uint32_t s_tap_start_ms     = 0;
static int16_t  s_tap_start_x      = 0;
static int16_t  s_tap_start_y      = 0;
static bool     s_tap_ready        = false;
static int16_t  s_tap_x            = 0;
static int16_t  s_tap_y            = 0;

static bool gt911_read(uint16_t reg, uint8_t* buf, size_t len) {
    if (!s_gt911) return false;
    uint8_t tx[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
    return i2c_master_transmit_receive(s_gt911, tx, 2, buf, len, 20) == ESP_OK;
}

static bool gt911_write(uint16_t reg, uint8_t val) {
    if (!s_gt911) return false;
    uint8_t tx[3] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), val };
    return i2c_master_transmit(s_gt911, tx, 3, 20) == ESP_OK;
}

bool GT911_Init(void) {
    // Hardware reset sequence per GT911 datasheet. Both TP_RST and TP_INT
    // are pulled low, then TP_RST released while TP_INT remains low to
    // select slave address 0x5D (int=high would yield 0x14).
    IO_EXTENSION_Output(WS_7B_IOEXT_TP_INT, 0);
    IO_EXTENSION_Output(WS_7B_IOEXT_TP_RST, 0);
    delay(10);
    IO_EXTENSION_Output(WS_7B_IOEXT_TP_RST, 1);
    delay(10);
    // Leaving INT tri-stated (input) after reset; the IO expander on the 7B
    // does not forward INT pulses, so we poll the status register instead.
    DEV_I2C_Set_Slave_Addr(&s_gt911, WS_7B_GT911_ADDR);

    uint8_t id[4] = {0};
    if (!gt911_read(GT911_REG_PRODUCT_ID, id, 4)) {
        Serial.println("GT911: no response on 0x5D");
        return false;
    }
    Serial.printf("GT911: product %c%c%c%c\n", id[0], id[1], id[2], id[3]);
    return (id[0] == '9' && id[1] == '1' && id[2] == '1');
}

bool GT911_Poll(GT911_Frame* out) {
    if (!out) return false;
    out->count = 0;
    out->frame_ms = millis();

    uint8_t status = 0;
    if (!gt911_read(GT911_REG_STATUS, &status, 1)) return false;
    if ((status & 0x80) == 0) return false;             // No new data ready

    uint8_t n = status & 0x0F;
    if (n > GT911_MAX_POINTS) n = GT911_MAX_POINTS;

    if (n > 0) {
        uint8_t pbuf[GT911_MAX_POINTS * 8] = {0};
        gt911_read(GT911_REG_POINT1, pbuf, n * 8);
        for (uint8_t i = 0; i < n; i++) {
            uint8_t* p = &pbuf[i * 8];
            out->points[i].track_id = p[0];
            out->points[i].x        = (int16_t)((p[2] << 8) | p[1]);
            out->points[i].y        = (int16_t)((p[4] << 8) | p[3]);
            out->points[i].size     = (uint8_t)((p[6] << 8) | p[5]);
        }
        out->count = n;
    }

    // Always clear the status so GT911 will latch the next frame.
    gt911_write(GT911_REG_STATUS, 0x00);

    // Tap state machine — single finger only.
    if (n >= 1) {
        if (!s_tap_armed) {
            s_tap_armed    = true;
            s_tap_start_ms = out->frame_ms;
            s_tap_start_x  = out->points[0].x;
            s_tap_start_y  = out->points[0].y;
        }
    } else if (s_tap_armed) {
        uint32_t dt = out->frame_ms - s_tap_start_ms;
        if (dt < 400) {
            s_tap_ready = true;
            s_tap_x     = s_tap_start_x;
            s_tap_y     = s_tap_start_y;
        }
        s_tap_armed = false;
    }

    return n > 0;
}

bool GT911_ConsumeTap(int16_t* tap_x, int16_t* tap_y) {
    if (!s_tap_ready) return false;
    if (tap_x) *tap_x = s_tap_x;
    if (tap_y) *tap_y = s_tap_y;
    s_tap_ready = false;
    return true;
}
