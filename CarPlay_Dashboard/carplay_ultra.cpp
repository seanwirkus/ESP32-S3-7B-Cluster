#include "carplay_ultra.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define DEG2RAD(d) ((d) * M_PI / 180.0f)
#define RAD2DEG(r) ((r) * 180.0f / M_PI)

// ============================================================================
// Utility Drawing Functions
// ============================================================================

uint16_t CarPlayUltra_BlendColor(uint16_t c1, uint16_t c2, float t) {
    if (t <= 0.0f) return c1;
    if (t >= 1.0f) return c2;
    uint8_t r1 = (c1 >> 11) & 0x1F, g1 = (c1 >> 5) & 0x3F, b1 = c1 & 0x1F;
    uint8_t r2 = (c2 >> 11) & 0x1F, g2 = (c2 >> 5) & 0x3F, b2 = c2 & 0x1F;
    uint8_t r = r1 + (int8_t)((r2 - r1) * t);
    uint8_t g = g1 + (int8_t)((g2 - g1) * t);
    uint8_t b = b1 + (int8_t)((b2 - b1) * t);
    return (r << 11) | (g << 5) | b;
}

float CarPlayUltra_Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void CarPlayUltra_DrawFilledCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color) {
    Paint_DrawCircle(cx, cy, r, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

void CarPlayUltra_DrawRoundedCard(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                                  uint16_t color, uint16_t borderColor) {
    // Fill the card body
    Paint_DrawRectangle(x + 4, y, x + w - 4, y + h, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawRectangle(x, y + 4, x + w, y + h - 4, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Corner circles for rounded look
    CarPlayUltra_DrawFilledCircle(x + 4, y + 4, 4, color);
    CarPlayUltra_DrawFilledCircle(x + w - 5, y + 4, 4, color);
    CarPlayUltra_DrawFilledCircle(x + 4, y + h - 5, 4, color);
    CarPlayUltra_DrawFilledCircle(x + w - 5, y + h - 5, 4, color);
    // Subtle border highlight on top edge
    if (borderColor != color) {
        Paint_DrawLine(x + 5, y, x + w - 5, y, borderColor, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
}

void CarPlayUltra_DrawDivider(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    Paint_DrawLine(x, y, x + w, y, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

void CarPlayUltra_DrawGlowDot(uint16_t cx, uint16_t cy, uint16_t r, uint16_t coreColor, uint16_t glowColor) {
    // Outer glow rings
    if (r > 4) Paint_DrawCircle(cx, cy, r + 3, glowColor, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    if (r > 2) Paint_DrawCircle(cx, cy, r + 1, glowColor, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    // Core filled
    CarPlayUltra_DrawFilledCircle(cx, cy, r, coreColor);
}

void CarPlayUltra_DrawHGradientBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                                   uint16_t colorStart, uint16_t colorEnd, float fillPct) {
    if (fillPct < 0.0f) fillPct = 0.0f;
    if (fillPct > 1.0f) fillPct = 1.0f;
    uint16_t fillW = (uint16_t)(w * fillPct);
    if (fillW < 1) fillW = 1;
    // Background track
    Paint_DrawRectangle(x, y, x + w, y + h, CP_GAUGE_BG, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Filled portion with color blend
    for (uint16_t i = 0; i < fillW; i += 2) {
        float t = (float)i / (float)w;
        uint16_t col = CarPlayUltra_BlendColor(colorStart, colorEnd, t);
        Paint_DrawRectangle(x + i, y + 1, x + i + 2, y + h - 1, col, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
}

// ============================================================================
// Arc and Gauge Drawing — The Heart of the Instrument Cluster
// ============================================================================

void CarPlayUltra_DrawArc(uint16_t cx, uint16_t cy, uint16_t rOuter, uint16_t rInner,
                          float startAngleDeg, float endAngleDeg, uint16_t color) {
    // Draw a thick arc by plotting pixels between rInner and rOuter
    float startRad = DEG2RAD(startAngleDeg);
    float endRad = DEG2RAD(endAngleDeg);
    float sweep = endRad - startRad;
    int steps = (int)(fabsf(sweep) * rOuter * 0.5f); // Adaptive step count
    if (steps < 40) steps = 40;
    if (steps > 300) steps = 300;

    for (int i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;
        float angle = startRad + sweep * t;
        float cosA = cosf(angle);
        float sinA = sinf(angle);

        // Draw radial line from inner to outer radius
        for (uint16_t r = rInner; r <= rOuter; r += 1) {
            int16_t px = cx + (int16_t)(r * cosA);
            int16_t py = cy + (int16_t)(r * sinA);
            if (px >= 0 && px < CP_SCREEN_W && py >= 0 && py < CP_SCREEN_H) {
                Paint_SetPixel(px, py, color);
            }
        }
    }
}

void CarPlayUltra_DrawArcSegment(uint16_t cx, uint16_t cy, uint16_t rOuter, uint16_t rInner,
                                 float startAngleDeg, float endAngleDeg, uint16_t color) {
    // Thinner, faster arc for small segments
    CarPlayUltra_DrawArc(cx, cy, rOuter, rInner, startAngleDeg, endAngleDeg, color);
}

void CarPlayUltra_DrawTickMarks(uint16_t cx, uint16_t cy, uint16_t rOuter, uint16_t rInner,
                                float startAngleDeg, float endAngleDeg, int count,
                                uint16_t color, uint16_t majorColor, int majorEvery) {
    float sweep = endAngleDeg - startAngleDeg;
    for (int i = 0; i <= count; i++) {
        float t = (float)i / (float)count;
        float angleDeg = startAngleDeg + sweep * t;
        float angleRad = DEG2RAD(angleDeg);
        float cosA = cosf(angleRad);
        float sinA = sinf(angleRad);

        bool isMajor = (i % majorEvery == 0);
        uint16_t tickInner = isMajor ? (rInner - 8) : (rInner - 3);
        uint16_t tickColor = isMajor ? majorColor : color;
        DOT_PIXEL tickWidth = isMajor ? DOT_PIXEL_2X2 : DOT_PIXEL_1X1;

        int16_t x1 = cx + (int16_t)(tickInner * cosA);
        int16_t y1 = cy + (int16_t)(tickInner * sinA);
        int16_t x2 = cx + (int16_t)(rOuter * cosA);
        int16_t y2 = cy + (int16_t)(rOuter * sinA);

        Paint_DrawLine(x1, y1, x2, y2, tickColor, tickWidth, LINE_STYLE_SOLID);
    }
}

void CarPlayUltra_DrawNeedle(uint16_t cx, uint16_t cy, uint16_t length,
                             float angleDeg, uint16_t color, uint8_t thickness) {
    float angleRad = DEG2RAD(angleDeg);
    int16_t endX = cx + (int16_t)(length * cosf(angleRad));
    int16_t endY = cy + (int16_t)(length * sinf(angleRad));

    DOT_PIXEL px = (thickness >= 3) ? DOT_PIXEL_3X3 :
                   (thickness >= 2) ? DOT_PIXEL_2X2 : DOT_PIXEL_1X1;
    Paint_DrawLine(cx, cy, endX, endY, color, px, LINE_STYLE_SOLID);

    // Tail (short opposite direction)
    int16_t tailX = cx - (int16_t)(15 * cosf(angleRad));
    int16_t tailY = cy - (int16_t)(15 * sinf(angleRad));
    Paint_DrawLine(cx, cy, tailX, tailY, color, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
}

// ============================================================================
// Initialization
// ============================================================================

void CarPlayUltra_Init(CarPlayUltra* cp) {
    if (!cp) return;
    memset(cp, 0, sizeof(CarPlayUltra));

    // Default vehicle state (matching the reference image)
    CarPlayState* s = &cp->state;
    s->speed = 45.0f;
    s->targetSpeed = 45.0f;
    s->rpm = 2610.0f;
    s->targetRpm = 2610.0f;
    s->gear = 9;  // D = Drive (auto)
    s->fuelLevel = 0.72f;
    s->engineTemp = 195.0f;
    s->oilPressure = 42.0f;

    // Navigation
    s->navProgress = 0.35f;
    s->compassHeading = 155;
    s->remainingMiles = 4.2f;
    strcpy(s->navInstruction, "Continue on McAuley St");
    strcpy(s->navDistance, "0.3 mi");
    strcpy(s->navETA, "12 min");
    strcpy(s->navArrival, "9:41");
    strcpy(s->currentStreet, "McAuley St");

    // Trip
    s->tripMiles = 54.0f;
    s->tripMPG = 32.0f;
    s->tripMinutes = 40;
    s->totalMileage = 14175;

    // Weather
    s->outsideTemp = 65;
    s->weatherCode = 0;  // Clear
    s->airQuality = 37;

    // Music
    strcpy(s->musicTitle, "Riptide");
    strcpy(s->musicArtist, "The Chainsmokers");
    s->musicProgress = 0.75f;
    strcpy(s->musicElapsed, "3:07");
    strcpy(s->musicRemaining, "-0:53");

    // Calendar
    strcpy(s->calDay, "Mon");
    s->calDate = 6;
    strcpy(s->calEvent1, "Design Sync");
    strcpy(s->calTime1, "10:00-11:00 AM");
    strcpy(s->calEvent2, "Weekly Goal Sync");
    strcpy(s->calTime2, "2:30-3:30 PM");

    // Home
    strcpy(s->homeStatus, "Garage Door Closed");

    // System
    s->batteryLevel = 85;
    s->signalBars = 4;
    s->wifiConnected = true;
    s->bluetoothConnected = true;

    // Copy to display state
    memcpy(&cp->displayState, &cp->state, sizeof(CarPlayState));

    cp->displaySpeed = s->speed;
    cp->displayRpm = s->rpm;
    cp->sweepAngle = 0;

    // ToF / mapping defaults — sensor is assumed offline until the main
    // sketch calls CarPlayUltra_IngestTof().
    cp->view         = CP_VIEW_INSTRUMENTS;
    cp->tofOnline    = false;
    cp->tofClosestMm = 0xFFFF;
    ToF_Map_Init(&cp->tofMap);
    memset(&cp->tofFrame, 0, sizeof(cp->tofFrame));

    cp->initialized = true;
}

// ============================================================================
// Tick — Smooth Animation & Simulation
// ============================================================================

void CarPlayUltra_Tick(CarPlayUltra* cp, uint32_t nowMs) {
    if (!cp || !cp->initialized) return;

    if (cp->lastTickMs == 0) {
        cp->lastTickMs = nowMs;
        return;
    }

    uint32_t delta = nowMs - cp->lastTickMs;
    cp->lastTickMs = nowMs;
    float dt = delta / 1000.0f;
    cp->frameCount++;

    // Animation phase (continuous rotation for effects)
    cp->animPhase += dt * 1.5f;
    if (cp->animPhase > 2.0f * M_PI) cp->animPhase -= 2.0f * M_PI;

    // Startup sweep animation (gauge sweep on boot)
    if (cp->sweepAngle < 270.0f) {
        cp->sweepAngle += dt * 180.0f; // Sweep in ~1.5 seconds
        if (cp->sweepAngle > 270.0f) cp->sweepAngle = 270.0f;
    }

    // Simulate realistic driving data
    float phase = nowMs / 1000.0f;
    cp->state.targetSpeed = 45.0f + 8.0f * sinf(phase * 0.4f) + 3.0f * sinf(phase * 1.1f);
    cp->state.targetRpm = 2600.0f + 400.0f * sinf(phase * 0.5f) + 200.0f * cosf(phase * 1.3f);
    cp->state.compassHeading = 150 + (int16_t)(20.0f * sinf(phase * 0.25f));

    // Smooth interpolation
    cp->displaySpeed = CarPlayUltra_Lerp(cp->displaySpeed, cp->state.targetSpeed, CP_ANIM_SPEED_SMOOTH);
    cp->displayRpm = CarPlayUltra_Lerp(cp->displayRpm, cp->state.targetRpm, CP_ANIM_RPM_SMOOTH);

    // Compute needle angles (135 to 405 degree sweep = 270 degrees of gauge)
    float speedPct = cp->displaySpeed / (float)CP_SPEEDO_MAX_SPEED;
    if (speedPct > 1.0f) speedPct = 1.0f;
    float targetNeedle = CP_SPEEDO_START_ANGLE + speedPct * (CP_SPEEDO_END_ANGLE - CP_SPEEDO_START_ANGLE);
    cp->needleAngle = CarPlayUltra_Lerp(cp->needleAngle, targetNeedle, CP_ANIM_NEEDLE_SMOOTH);

    float rpmPct = cp->displayRpm / (float)CP_RPM_MAX;
    if (rpmPct > 1.0f) rpmPct = 1.0f;
    float targetRpmNeedle = CP_RPM_START_ANGLE + rpmPct * (CP_RPM_END_ANGLE - CP_RPM_START_ANGLE);
    cp->rpmNeedleAngle = CarPlayUltra_Lerp(cp->rpmNeedleAngle, targetRpmNeedle, CP_ANIM_NEEDLE_SMOOTH);

    // Navigation progress
    cp->state.navProgress += dt * 0.008f;
    if (cp->state.navProgress > 1.0f) cp->state.navProgress = 0.0f;

    // Music progress
    cp->state.musicProgress += dt * 0.005f;
    if (cp->state.musicProgress > 1.0f) cp->state.musicProgress = 0.0f;

    // Fuel slowly decreasing
    cp->state.fuelLevel -= dt * 0.0001f;
    if (cp->state.fuelLevel < 0.1f) cp->state.fuelLevel = 0.72f;

    // Update display state
    cp->displayState.speed = cp->displaySpeed;
    cp->displayState.rpm = cp->displayRpm;
    cp->displayState.compassHeading = cp->state.compassHeading;
    cp->displayState.navProgress = cp->state.navProgress;
    cp->displayState.musicProgress = cp->state.musicProgress;
    cp->displayState.fuelLevel = cp->state.fuelLevel;

    // Decay the ToF map every tick so stale obstacles fade out even when the
    // sensor drops a frame. Ingest itself resets the age, so live cells stay.
    ToF_Map_Decay(&cp->tofMap);
}

// ============================================================================
// Main Render
// ============================================================================

void CarPlayUltra_Render(CarPlayUltra* cp) {
    if (!cp || !cp->initialized) return;

    // Black canvas
    Paint_Clear(CP_BG_BLACK);

    if (cp->view == CP_VIEW_MAPPING) {
        // --- Left: ToF 3D mapping takes over the cluster ---
        CarPlayUltra_DrawMappingView(cp);
    } else {
        // --- Left: Instrument Cluster ---
        CarPlayUltra_DrawClusterBackground(cp);
        CarPlayUltra_DrawMapInset(cp);
        CarPlayUltra_DrawSpeedometer(cp);
        CarPlayUltra_DrawRPMGauge(cp);
        CarPlayUltra_DrawGearIndicator(cp);
        CarPlayUltra_DrawBottomStrip(cp);
    }

    // --- Right: Info Panel (always rendered) ---
    CarPlayUltra_DrawPanelBackground(cp);
    CarPlayUltra_DrawStatusBar(cp);
    CarPlayUltra_DrawTripCard(cp);
    CarPlayUltra_DrawCalendarCard(cp);
    CarPlayUltra_DrawWeatherCard(cp);
    CarPlayUltra_DrawMusicCard(cp);
    CarPlayUltra_DrawNavCard(cp);

    // Mini ToF card pinned to the info panel when the sensor is online,
    // regardless of which cluster view is active — driver always sees the
    // closest obstacle reading.
    CarPlayUltra_DrawTofMiniCard(cp);

    // Hazard tint overlays everything if something is closing fast in front.
    CarPlayUltra_DrawTofWarning(cp);
}

// ============================================================================
// View switching + ToF ingest
// ============================================================================

void CarPlayUltra_OnTap(CarPlayUltra* cp, int16_t x, int16_t y) {
    if (!cp) return;
    // Taps anywhere on the left cluster toggle between instrument and mapping
    // views. Taps on the right panel are reserved for per-card actions (left
    // as a follow-up — today they're absorbed silently).
    if (x < CP_CLUSTER_W) {
        cp->view = (CarPlayView)((cp->view + 1) % CP_VIEW_COUNT);
    }
}

void CarPlayUltra_IngestTof(CarPlayUltra* cp, const ToF_Frame* frame, float dt_s) {
    if (!cp || !frame || !frame->valid) return;
    // Copy the raw frame for the heatmap, then fold it into the rolling map
    // using the speed/heading we already track.
    cp->tofFrame = *frame;
    ToF_Map_Integrate(&cp->tofMap,
                      frame,
                      cp->displaySpeed,
                      (float)cp->state.compassHeading,
                      dt_s);
    cp->tofClosestMm = ToF_ClosestMm();
    cp->tofOnline    = true;
}

// ============================================================================
// Instrument Cluster — Background
// ============================================================================

void CarPlayUltra_DrawClusterBackground(CarPlayUltra* cp) {
    // Deep black cluster area
    Paint_DrawRectangle(CP_CLUSTER_X, CP_CLUSTER_Y,
                        CP_CLUSTER_X + CP_CLUSTER_W, CP_CLUSTER_Y + CP_CLUSTER_H,
                        CP_BG_BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Subtle vertical divider between cluster and panel
    Paint_DrawLine(CP_CLUSTER_W, 0, CP_CLUSTER_W, CP_SCREEN_H,
                   CP_BG_CARD, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(CP_CLUSTER_W + 1, 0, CP_CLUSTER_W + 1, CP_SCREEN_H,
                   CP_BG_DEEP, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

// ============================================================================
// Speedometer — Premium Arc Gauge
// ============================================================================

void CarPlayUltra_DrawSpeedometer(CarPlayUltra* cp) {
    uint16_t cx = CP_SPEEDO_CX;
    uint16_t cy = CP_SPEEDO_CY;

    // --- Background arc (dark track) ---
    CarPlayUltra_DrawArc(cx, cy, CP_SPEEDO_R_OUTER, CP_SPEEDO_R_INNER,
                         CP_SPEEDO_START_ANGLE, CP_SPEEDO_END_ANGLE, CP_GAUGE_BG);

    // --- Active sweep arc (cyan-to-green gradient effect) ---
    float speedPct = cp->displaySpeed / (float)CP_SPEEDO_MAX_SPEED;
    if (speedPct > 1.0f) speedPct = 1.0f;
    float sweepEnd = CP_SPEEDO_START_ANGLE + speedPct * (CP_SPEEDO_END_ANGLE - CP_SPEEDO_START_ANGLE);

    // Clamp to startup animation
    float maxSweep = CP_SPEEDO_START_ANGLE + (cp->sweepAngle / 270.0f) * (CP_SPEEDO_END_ANGLE - CP_SPEEDO_START_ANGLE);
    if (sweepEnd > maxSweep) sweepEnd = maxSweep;

    // Draw sweep in segments for color gradient (cyan -> green -> teal)
    float sweepRange = sweepEnd - CP_SPEEDO_START_ANGLE;
    int segments = 8;
    for (int i = 0; i < segments; i++) {
        float t0 = (float)i / segments;
        float t1 = (float)(i + 1) / segments;
        float a0 = CP_SPEEDO_START_ANGLE + sweepRange * t0;
        float a1 = CP_SPEEDO_START_ANGLE + sweepRange * t1;
        if (a1 <= CP_SPEEDO_START_ANGLE) continue;

        // Color blend: cyan at start -> green at middle -> teal at end
        float mid = (t0 + t1) * 0.5f;
        uint16_t segColor;
        if (mid < 0.5f) {
            segColor = CarPlayUltra_BlendColor(CP_ACCENT_CYAN, CP_ACCENT_GREEN, mid * 2.0f);
        } else {
            segColor = CarPlayUltra_BlendColor(CP_ACCENT_GREEN, CP_ACCENT_TEAL, (mid - 0.5f) * 2.0f);
        }
        CarPlayUltra_DrawArc(cx, cy, CP_SPEEDO_R_OUTER - 2, CP_SPEEDO_R_INNER + 2,
                             a0, a1, segColor);
    }

    // --- Tick marks ---
    CarPlayUltra_DrawTickMarks(cx, cy, CP_SPEEDO_R_TICK, CP_SPEEDO_R_OUTER + 2,
                               CP_SPEEDO_START_ANGLE, CP_SPEEDO_END_ANGLE,
                               CP_GAUGE_TICK_COUNT, CP_GAUGE_TICK, CP_GAUGE_TICK_BRIGHT,
                               CP_GAUGE_MAJOR_EVERY);

    // --- Speed numbers around the arc ---
    const int speedLabels[] = {0, 20, 40, 60, 80, 100, 120, 140, 160};
    int labelCount = 9;
    for (int i = 0; i < labelCount; i++) {
        float t = (float)speedLabels[i] / (float)CP_SPEEDO_MAX_SPEED;
        float angleDeg = CP_SPEEDO_START_ANGLE + t * (CP_SPEEDO_END_ANGLE - CP_SPEEDO_START_ANGLE);
        float angleRad = DEG2RAD(angleDeg);
        int16_t lx = cx + (int16_t)((CP_SPEEDO_R_OUTER + 16) * cosf(angleRad)) - 8;
        int16_t ly = cy + (int16_t)((CP_SPEEDO_R_OUTER + 16) * sinf(angleRad)) - 5;
        if (lx < 0) lx = 0;
        if (ly < 0) ly = 0;
        char lbl[4];
        snprintf(lbl, sizeof(lbl), "%d", speedLabels[i]);
        Paint_DrawString_EN(lx, ly, lbl, &Font8, CP_TEXT_DIM, CP_BG_BLACK);
    }

    // --- Needle ---
    CarPlayUltra_DrawNeedle(cx, cy, CP_SPEEDO_R_INNER - 5,
                            cp->needleAngle, CP_GAUGE_NEEDLE, 2);

    // --- Center hub ---
    CarPlayUltra_DrawFilledCircle(cx, cy, 10, CP_GAUGE_CENTER);
    CarPlayUltra_DrawFilledCircle(cx, cy, 4, CP_TEXT_WHITE);

    // --- Large speed number (center of gauge) ---
    char speedStr[8];
    snprintf(speedStr, sizeof(speedStr), "%d", (int)cp->displaySpeed);
    // Center the text
    int numLen = strlen(speedStr);
    int16_t textX = cx - (numLen * 17);  // Font48 is ~34px wide
    Paint_DrawString_EN(textX, cy - 25, speedStr, &Font48, CP_TEXT_WHITE, CP_BG_BLACK);

    // "mph" label below speed
    Paint_DrawString_EN(cx - 12, cy + 28, "mph", &Font12, CP_TEXT_GRAY, CP_BG_BLACK);

    // Secondary speed in km/h
    char kmhStr[12];
    snprintf(kmhStr, sizeof(kmhStr), "%d km/h", (int)(cp->displaySpeed * 1.60934f));
    Paint_DrawString_EN(cx - 24, cy + 48, kmhStr, &Font8, CP_TEXT_DIM, CP_BG_BLACK);
}

// ============================================================================
// RPM Gauge
// ============================================================================

void CarPlayUltra_DrawRPMGauge(CarPlayUltra* cp) {
    uint16_t cx = CP_RPM_CX;
    uint16_t cy = CP_RPM_CY;

    // --- Background arc ---
    CarPlayUltra_DrawArc(cx, cy, CP_RPM_R_OUTER, CP_RPM_R_INNER,
                         CP_RPM_START_ANGLE, CP_RPM_END_ANGLE, CP_GAUGE_BG);

    // --- Active sweep ---
    float rpmPct = cp->displayRpm / (float)CP_RPM_MAX;
    if (rpmPct > 1.0f) rpmPct = 1.0f;
    float sweepEnd = CP_RPM_START_ANGLE + rpmPct * (CP_RPM_END_ANGLE - CP_RPM_START_ANGLE);

    // Clamp to startup animation
    float maxSweep = CP_RPM_START_ANGLE + (cp->sweepAngle / 270.0f) * (CP_RPM_END_ANGLE - CP_RPM_START_ANGLE);
    if (sweepEnd > maxSweep) sweepEnd = maxSweep;

    // RPM red zone above 6000
    float redZoneStart = CP_RPM_START_ANGLE + (6000.0f / CP_RPM_MAX) * (CP_RPM_END_ANGLE - CP_RPM_START_ANGLE);

    float sweepRange = sweepEnd - CP_RPM_START_ANGLE;
    int segments = 6;
    for (int i = 0; i < segments; i++) {
        float t0 = (float)i / segments;
        float t1 = (float)(i + 1) / segments;
        float a0 = CP_RPM_START_ANGLE + sweepRange * t0;
        float a1 = CP_RPM_START_ANGLE + sweepRange * t1;
        if (a1 <= CP_RPM_START_ANGLE) continue;

        float midAngle = (a0 + a1) * 0.5f;
        uint16_t segColor;
        if (midAngle >= redZoneStart) {
            segColor = CP_ACCENT_RED;
        } else if (midAngle >= redZoneStart - 30) {
            segColor = CP_ACCENT_ORANGE;
        } else {
            segColor = CP_ACCENT_TEAL;
        }
        CarPlayUltra_DrawArc(cx, cy, CP_RPM_R_OUTER - 2, CP_RPM_R_INNER + 2, a0, a1, segColor);
    }

    // Red zone indicator (always visible)
    CarPlayUltra_DrawArc(cx, cy, CP_RPM_R_OUTER, CP_RPM_R_OUTER - 3,
                         redZoneStart, (float)CP_RPM_END_ANGLE, CP_ACCENT_RED);

    // --- Tick marks ---
    CarPlayUltra_DrawTickMarks(cx, cy, CP_RPM_R_TICK, CP_RPM_R_OUTER + 2,
                               CP_RPM_START_ANGLE, CP_RPM_END_ANGLE,
                               24, CP_GAUGE_TICK, CP_GAUGE_TICK_BRIGHT, 3);

    // --- RPM labels ---
    const int rpmLabels[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    for (int i = 0; i < 9; i++) {
        float t = (float)(rpmLabels[i] * 1000) / (float)CP_RPM_MAX;
        float angleDeg = CP_RPM_START_ANGLE + t * (CP_RPM_END_ANGLE - CP_RPM_START_ANGLE);
        float angleRad = DEG2RAD(angleDeg);
        int16_t lx = cx + (int16_t)((CP_RPM_R_OUTER + 14) * cosf(angleRad)) - 3;
        int16_t ly = cy + (int16_t)((CP_RPM_R_OUTER + 14) * sinf(angleRad)) - 4;
        if (lx < 0) lx = 0;
        if (ly < 0) ly = 0;
        char lbl[4];
        snprintf(lbl, sizeof(lbl), "%d", rpmLabels[i]);
        uint16_t labelColor = (rpmLabels[i] >= 6) ? CP_ACCENT_RED : CP_TEXT_DIM;
        Paint_DrawString_EN(lx, ly, lbl, &Font8, labelColor, CP_BG_BLACK);
    }

    // --- Needle ---
    CarPlayUltra_DrawNeedle(cx, cy, CP_RPM_R_INNER - 5,
                            cp->rpmNeedleAngle, CP_GAUGE_NEEDLE, 2);

    // --- Center hub ---
    CarPlayUltra_DrawFilledCircle(cx, cy, 8, CP_GAUGE_CENTER);
    CarPlayUltra_DrawFilledCircle(cx, cy, 3, CP_TEXT_WHITE);

    // --- RPM value ---
    char rpmStr[8];
    snprintf(rpmStr, sizeof(rpmStr), "%d", (int)cp->displayRpm);
    int rpmLen = strlen(rpmStr);
    Paint_DrawString_EN(cx - (rpmLen * 6), cy - 12, rpmStr, &Font20, CP_TEXT_WHITE, CP_BG_BLACK);
    Paint_DrawString_EN(cx - 10, cy + 14, "rpm", &Font8, CP_TEXT_GRAY, CP_BG_BLACK);

    // "auto" label below RPM
    Paint_DrawString_EN(cx - 12, cy + 32, "auto", &Font8, CP_TEXT_DIM, CP_BG_BLACK);
}

// ============================================================================
// Gear Indicator (between gauges)
// ============================================================================

void CarPlayUltra_DrawGearIndicator(CarPlayUltra* cp) {
    uint16_t x = CP_GEAR_X;
    uint16_t y = CP_GEAR_Y;

    // Gear letter
    const char* gearNames[] = {"P", "1", "2", "3", "4", "5", "6", "R", "N", "D"};
    uint8_t gear = cp->state.gear;
    if (gear > 9) gear = 9;
    const char* gearStr = gearNames[gear];

    // Draw large gear letter
    Paint_DrawString_EN(x, y, gearStr, &Font48, CP_ACCENT_CYAN, CP_BG_BLACK);
}

// ============================================================================
// Map Inset (top center of cluster)
// ============================================================================

void CarPlayUltra_DrawMapInset(CarPlayUltra* cp) {
    uint16_t x = CP_MAP_INSET_X;
    uint16_t y = CP_MAP_INSET_Y;
    uint16_t w = CP_MAP_INSET_W;
    uint16_t h = CP_MAP_INSET_H;

    // Map background
    Paint_DrawRectangle(x, y, x + w, y + h, CP_MAP_BG, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Street grid
    // Horizontal streets
    Paint_DrawLine(x + 10, y + 35, x + w - 10, y + 35, CP_MAP_ROAD_SEC, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 10, y + 70, x + w - 10, y + 70, CP_MAP_ROAD_MAIN, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 10, y + 105, x + w - 10, y + 105, CP_MAP_ROAD_SEC, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Vertical streets
    Paint_DrawLine(x + 65, y + 10, x + 65, y + h - 10, CP_MAP_ROAD_SEC, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 130, y + 10, x + 130, y + h - 10, CP_MAP_ROAD_MAIN, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 195, y + 10, x + 195, y + h - 10, CP_MAP_ROAD_SEC, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Active route (blue line)
    Paint_DrawLine(x + 130, y + h - 5, x + 130, y + 70, CP_MAP_ROUTE, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 130, y + 70, x + w - 15, y + 70, CP_MAP_ROUTE, DOT_PIXEL_3X3, LINE_STYLE_SOLID);

    // Current position dot (animated along route)
    float t = cp->displayState.navProgress;
    uint16_t posX, posY;
    if (t < 0.5f) {
        posX = x + 130;
        posY = (uint16_t)(y + h - 5 - t * 2.0f * (h - 75));
    } else {
        posX = (uint16_t)(x + 130 + (t - 0.5f) * 2.0f * (w - 145));
        posY = y + 70;
    }
    CarPlayUltra_DrawGlowDot(posX, posY, 5, CP_MAP_POSITION, CP_ACCENT_BLUE);

    // Street labels
    Paint_DrawString_EN(x + 5, y + 58, "58th St", &Font8, CP_TEXT_DIM, CP_MAP_BG);
    Paint_DrawString_EN(x + 135, y + 58, "Racine St", &Font8, CP_TEXT_DIM, CP_MAP_BG);
    Paint_DrawString_EN(x + 5, y + 93, "59th St", &Font8, CP_TEXT_DIM, CP_MAP_BG);
    Paint_DrawString_EN(x + 70, y + 5, cp->state.currentStreet, &Font8, CP_TEXT_LIGHT, CP_MAP_BG);

    // Border
    Paint_DrawRectangle(x, y, x + w, y + h, CP_CARD_BORDER, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
}

// ============================================================================
// Bottom Strip (fuel, range, temp)
// ============================================================================

void CarPlayUltra_DrawBottomStrip(CarPlayUltra* cp) {
    uint16_t x = CP_BOTTOM_STRIP_X;
    uint16_t y = CP_BOTTOM_STRIP_Y;
    uint16_t w = CP_BOTTOM_STRIP_W;

    // Background
    Paint_DrawRectangle(x, y, x + w, y + CP_BOTTOM_STRIP_H, CP_BG_DEEP, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Fuel bar
    Paint_DrawString_EN(x + 10, y + 6, "FUEL", &Font8, CP_TEXT_DIM, CP_BG_DEEP);
    CarPlayUltra_DrawHGradientBar(x + 50, y + 6, 150, 10, CP_ACCENT_GREEN, CP_ACCENT_CYAN, cp->state.fuelLevel);

    // Range estimate
    char rangeStr[16];
    int rangeEst = (int)(cp->state.fuelLevel * 420); // ~420 mile full tank
    snprintf(rangeStr, sizeof(rangeStr), "%d mi range", rangeEst);
    Paint_DrawString_EN(x + 210, y + 6, rangeStr, &Font8, CP_TEXT_GRAY, CP_BG_DEEP);

    // Engine temp
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.0f F", cp->state.engineTemp);
    Paint_DrawString_EN(x + 10, y + 28, tempStr, &Font12, CP_TEXT_LIGHT, CP_BG_DEEP);

    // Oil pressure
    char oilStr[16];
    snprintf(oilStr, sizeof(oilStr), "Oil %.0f PSI", cp->state.oilPressure);
    Paint_DrawString_EN(x + 120, y + 28, oilStr, &Font8, CP_TEXT_DIM, CP_BG_DEEP);

    // Odometer
    char odoStr[16];
    snprintf(odoStr, sizeof(odoStr), "%lu mi", (unsigned long)cp->state.totalMileage);
    Paint_DrawString_EN(x + 280, y + 28, odoStr, &Font8, CP_TEXT_DIM, CP_BG_DEEP);

    // Divider at top
    CarPlayUltra_DrawDivider(x, y, w, CP_CARD_BORDER);
}

// ============================================================================
// Info Panel — Background
// ============================================================================

void CarPlayUltra_DrawPanelBackground(CarPlayUltra* cp) {
    // Slightly lifted dark background for the panel
    Paint_DrawRectangle(CP_PANEL_X + 2, CP_PANEL_Y,
                        CP_PANEL_X + CP_PANEL_W, CP_PANEL_Y + CP_PANEL_H,
                        CP_BG_DEEP, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// ============================================================================
// Status Bar
// ============================================================================

void CarPlayUltra_DrawStatusBar(CarPlayUltra* cp) {
    uint16_t x = CP_STATUS_X;
    uint16_t y = CP_STATUS_Y;

    // Background
    Paint_DrawRectangle(x, y, x + CP_STATUS_W, y + CP_STATUS_H,
                        CP_BG_DARK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Time (left side)
    Paint_DrawString_EN(x + 12, y + 10, "9:41", &Font16, CP_TEXT_WHITE, CP_BG_DARK);

    // CarPlay label
    Paint_DrawString_EN(x + 80, y + 12, "CarPlay", &Font12, CP_TEXT_GRAY, CP_BG_DARK);

    // Signal bars (right side)
    uint16_t barX = x + CP_STATUS_W - 140;
    for (int i = 0; i < 4; i++) {
        uint16_t barH = 5 + i * 3;
        uint16_t barColor = (i < cp->state.signalBars) ? CP_TEXT_WHITE : CP_TEXT_DIM;
        Paint_DrawRectangle(barX + i * 7, y + CP_STATUS_H - barH - 8,
                            barX + i * 7 + 4, y + CP_STATUS_H - 8,
                            barColor, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    Paint_DrawString_EN(barX + 34, y + 12, "5G", &Font8, CP_TEXT_WHITE, CP_BG_DARK);

    // WiFi indicator
    if (cp->state.wifiConnected) {
        Paint_DrawString_EN(barX + 56, y + 12, "W", &Font8, CP_TEXT_WHITE, CP_BG_DARK);
    }

    // Battery
    uint16_t batX = x + CP_STATUS_W - 50;
    Paint_DrawRectangle(batX, y + 12, batX + 24, y + 24, CP_TEXT_WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(batX + 24, y + 16, batX + 26, y + 20, CP_TEXT_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    uint16_t batFill = (uint16_t)(cp->state.batteryLevel * 22 / 100);
    uint16_t batColor = (cp->state.batteryLevel > 20) ? CP_ACCENT_GREEN : CP_ACCENT_RED;
    Paint_DrawRectangle(batX + 1, y + 13, batX + 1 + batFill, y + 23,
                        batColor, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Bottom divider
    CarPlayUltra_DrawDivider(x, y + CP_STATUS_H - 1, CP_STATUS_W, CP_CARD_BORDER);
}

// ============================================================================
// Trip Card
// ============================================================================

void CarPlayUltra_DrawTripCard(CarPlayUltra* cp) {
    uint16_t x = CP_TRIP_CARD_X;
    uint16_t y = CP_TRIP_CARD_Y;
    uint16_t w = CP_TRIP_CARD_W;
    uint16_t h = CP_TRIP_CARD_H;

    CarPlayUltra_DrawRoundedCard(x, y, w, h, CP_BG_CARD, CP_CARD_BORDER);

    // Title
    Paint_DrawString_EN(x + 12, y + 10, "Current Trip", &Font12, CP_TEXT_GRAY, CP_BG_CARD);

    // Duration
    Paint_DrawString_EN(x + 12, y + 32, "Duration", &Font8, CP_TEXT_DIM, CP_BG_CARD);
    char durStr[16];
    snprintf(durStr, sizeof(durStr), "0 hr %d min", cp->state.tripMinutes);
    Paint_DrawString_EN(x + 12, y + 46, durStr, &Font12, CP_TEXT_WHITE, CP_BG_CARD);

    // Divider
    CarPlayUltra_DrawDivider(x + 10, y + 65, w - 20, CP_CARD_DIVIDER);

    // MPG
    Paint_DrawString_EN(x + 12, y + 74, "Fuel Economy", &Font8, CP_TEXT_DIM, CP_BG_CARD);
    char mpgStr[12];
    snprintf(mpgStr, sizeof(mpgStr), "%.0f mpg", cp->state.tripMPG);
    Paint_DrawString_EN(x + 12, y + 88, mpgStr, &Font16, CP_TEXT_GREEN, CP_BG_CARD);

    // Divider
    CarPlayUltra_DrawDivider(x + 10, y + 112, w - 20, CP_CARD_DIVIDER);

    // Distance
    Paint_DrawString_EN(x + 12, y + 120, "Distance", &Font8, CP_TEXT_DIM, CP_BG_CARD);
    char distStr[12];
    snprintf(distStr, sizeof(distStr), "%.0f mi", cp->state.tripMiles);
    Paint_DrawString_EN(x + 12, y + 134, distStr, &Font16, CP_TEXT_WHITE, CP_BG_CARD);
}

// ============================================================================
// Calendar Card
// ============================================================================

void CarPlayUltra_DrawCalendarCard(CarPlayUltra* cp) {
    uint16_t x = CP_CAL_CARD_X;
    uint16_t y = CP_CAL_CARD_Y;
    uint16_t w = CP_CAL_CARD_W;
    uint16_t h = CP_CAL_CARD_H;

    CarPlayUltra_DrawRoundedCard(x, y, w, h, CP_BG_CARD, CP_CARD_BORDER);

    // Day header (red accent bar)
    Paint_DrawRectangle(x + 4, y, x + w - 4, y + 4, CP_ACCENT_RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Day name and date
    Paint_DrawString_EN(x + 12, y + 12, cp->state.calDay, &Font12, CP_TEXT_GRAY, CP_BG_CARD);
    char dateStr[4];
    snprintf(dateStr, sizeof(dateStr), "%d", cp->state.calDate);
    Paint_DrawString_EN(x + 60, y + 8, dateStr, &Font48, CP_TEXT_WHITE, CP_BG_CARD);

    // "Tomorrow" label
    Paint_DrawString_EN(x + 12, y + 35, "Tomorrow", &Font8, CP_TEXT_DIM, CP_BG_CARD);

    // Divider
    CarPlayUltra_DrawDivider(x + 10, y + 62, w - 20, CP_CARD_DIVIDER);

    // Event 1
    // Color indicator dot
    CarPlayUltra_DrawFilledCircle(x + 16, y + 76, 3, CP_ACCENT_CYAN);
    Paint_DrawString_EN(x + 24, y + 70, cp->state.calEvent1, &Font8, CP_TEXT_WHITE, CP_BG_CARD);
    Paint_DrawString_EN(x + 24, y + 84, cp->state.calTime1, &Font8, CP_TEXT_DIM, CP_BG_CARD);

    // Divider
    CarPlayUltra_DrawDivider(x + 10, y + 100, w - 20, CP_CARD_DIVIDER);

    // Event 2
    CarPlayUltra_DrawFilledCircle(x + 16, y + 114, 3, CP_ACCENT_PURPLE);
    Paint_DrawString_EN(x + 24, y + 108, cp->state.calEvent2, &Font8, CP_TEXT_WHITE, CP_BG_CARD);
    Paint_DrawString_EN(x + 24, y + 122, cp->state.calTime2, &Font8, CP_TEXT_DIM, CP_BG_CARD);
}

// ============================================================================
// Weather Card
// ============================================================================

void CarPlayUltra_DrawWeatherCard(CarPlayUltra* cp) {
    uint16_t x = CP_WEATHER_CARD_X;
    uint16_t y = CP_WEATHER_CARD_Y;
    uint16_t w = CP_WEATHER_CARD_W;
    uint16_t h = CP_WEATHER_CARD_H;

    CarPlayUltra_DrawRoundedCard(x, y, w, h, CP_BG_CARD, CP_CARD_BORDER);

    // Location
    Paint_DrawString_EN(x + 12, y + 10, "Oakland", &Font12, CP_TEXT_WHITE, CP_BG_CARD);

    // Location pin icon (simple triangle + circle)
    CarPlayUltra_DrawFilledCircle(x + w - 20, y + 16, 4, CP_ACCENT_BLUE);

    // Air Quality
    Paint_DrawString_EN(x + 12, y + 30, "Air Quality", &Font8, CP_TEXT_DIM, CP_BG_CARD);
    char aqiStr[8];
    snprintf(aqiStr, sizeof(aqiStr), "%d", cp->state.airQuality);
    Paint_DrawString_EN(x + 12, y + 44, aqiStr, &Font24, CP_TEXT_WHITE, CP_BG_CARD);
    Paint_DrawString_EN(x + 50, y + 50, "Good", &Font8, CP_ACCENT_GREEN, CP_BG_CARD);

    // Green down arrow (air quality trend)
    Paint_DrawLine(x + 100, y + 44, x + 105, y + 56, CP_ACCENT_GREEN, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 110, y + 44, x + 105, y + 56, CP_ACCENT_GREEN, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    // Divider
    CarPlayUltra_DrawDivider(x + 10, y + 78, w - 20, CP_CARD_DIVIDER);

    // UV Index
    Paint_DrawString_EN(x + 12, y + 86, "UV Index", &Font8, CP_TEXT_DIM, CP_BG_CARD);
    Paint_DrawString_EN(x + 12, y + 100, "0", &Font24, CP_TEXT_WHITE, CP_BG_CARD);
    Paint_DrawString_EN(x + 30, y + 106, "Low", &Font8, CP_ACCENT_GREEN, CP_BG_CARD);

    // Temperature at bottom
    CarPlayUltra_DrawDivider(x + 10, y + 130, w - 20, CP_CARD_DIVIDER);
    char tempStr[8];
    snprintf(tempStr, sizeof(tempStr), "%d F", cp->state.outsideTemp);
    Paint_DrawString_EN(x + 12, y + 138, tempStr, &Font16, CP_TEXT_WHITE, CP_BG_CARD);

    // Weather condition
    const char* conditions[] = {"Clear", "Cloudy", "Rain", "Snow"};
    uint8_t weatherCode = cp->state.weatherCode;
    if (weatherCode > 3) weatherCode = 0;
    Paint_DrawString_EN(x + 80, y + 142, conditions[weatherCode], &Font8, CP_TEXT_GRAY, CP_BG_CARD);
}

// ============================================================================
// Music / Now Playing Card
// ============================================================================

void CarPlayUltra_DrawMusicCard(CarPlayUltra* cp) {
    uint16_t x = CP_MUSIC_CARD_X;
    uint16_t y = CP_MUSIC_CARD_Y;
    uint16_t w = CP_MUSIC_CARD_W;
    uint16_t h = CP_MUSIC_CARD_H;

    CarPlayUltra_DrawRoundedCard(x, y, w, h, CP_BG_CARD, CP_CARD_BORDER);

    // "My Home" section (left side)
    Paint_DrawString_EN(x + 12, y + 10, "My Home", &Font12, CP_TEXT_GRAY, CP_BG_CARD);

    // Home icon placeholder (small house shape)
    Paint_DrawRectangle(x + 12, y + 30, x + 28, y + 44, CP_ACCENT_BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Roof
    Paint_DrawLine(x + 8, y + 30, x + 20, y + 22, CP_ACCENT_BLUE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 20, y + 22, x + 32, y + 30, CP_ACCENT_BLUE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    Paint_DrawString_EN(x + 36, y + 28, "Garage Door", &Font8, CP_TEXT_WHITE, CP_BG_CARD);
    Paint_DrawString_EN(x + 36, y + 42, "Closed", &Font8, CP_TEXT_DIM, CP_BG_CARD);

    // Vertical divider
    Paint_DrawLine(x + 170, y + 10, x + 170, y + h - 10, CP_CARD_DIVIDER, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Music section (right side, wider)
    uint16_t musicX = x + 180;

    // Album art placeholder (colored square)
    Paint_DrawRectangle(musicX, y + 10, musicX + 50, y + 60, CP_ACCENT_PURPLE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Music note icon
    Paint_DrawCircle(musicX + 20, y + 40, 6, CP_TEXT_WHITE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawLine(musicX + 26, y + 40, musicX + 26, y + 18, CP_TEXT_WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(musicX + 26, y + 18, musicX + 38, y + 15, CP_TEXT_WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Song title and artist
    Paint_DrawString_EN(musicX + 58, y + 12, cp->state.musicTitle, &Font16, CP_TEXT_WHITE, CP_BG_CARD);
    Paint_DrawString_EN(musicX + 58, y + 32, cp->state.musicArtist, &Font12, CP_TEXT_GRAY, CP_BG_CARD);

    // Album name
    Paint_DrawString_EN(musicX + 58, y + 50, "So Far So Good", &Font8, CP_TEXT_DIM, CP_BG_CARD);

    // Progress bar
    uint16_t progX = musicX;
    uint16_t progY = y + 72;
    uint16_t progW = w - 190;
    CarPlayUltra_DrawHGradientBar(progX, progY, progW, 6, CP_ACCENT_MAGENTA, CP_ACCENT_PURPLE, cp->displayState.musicProgress);

    // Time labels
    Paint_DrawString_EN(progX, progY + 10, cp->state.musicElapsed, &Font8, CP_TEXT_GRAY, CP_BG_CARD);
    Paint_DrawString_EN(progX + progW - 24, progY + 10, cp->state.musicRemaining, &Font8, CP_TEXT_GRAY, CP_BG_CARD);

    // Playback controls (simple symbols)
    uint16_t ctrlY = progY + 24;
    uint16_t ctrlCX = progX + progW / 2;
    // Prev
    Paint_DrawLine(ctrlCX - 50, ctrlY + 6, ctrlCX - 40, ctrlY, CP_TEXT_WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(ctrlCX - 50, ctrlY + 6, ctrlCX - 40, ctrlY + 12, CP_TEXT_WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    // Play/Pause (two vertical bars)
    Paint_DrawRectangle(ctrlCX - 6, ctrlY, ctrlCX - 2, ctrlY + 12, CP_TEXT_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawRectangle(ctrlCX + 2, ctrlY, ctrlCX + 6, ctrlY + 12, CP_TEXT_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Next
    Paint_DrawLine(ctrlCX + 40, ctrlY, ctrlCX + 50, ctrlY + 6, CP_TEXT_WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(ctrlCX + 40, ctrlY + 12, ctrlCX + 50, ctrlY + 6, CP_TEXT_WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
}

// ============================================================================
// Navigation Card
// ============================================================================

void CarPlayUltra_DrawNavCard(CarPlayUltra* cp) {
    uint16_t x = CP_NAV_CARD_X;
    uint16_t y = CP_NAV_CARD_Y;
    uint16_t w = CP_NAV_CARD_W;
    uint16_t h = CP_NAV_CARD_H;

    if (h < 20) return; // Not enough space

    CarPlayUltra_DrawRoundedCard(x, y, w, h, CP_BG_CARD, CP_CARD_BORDER);

    // Navigation icon (arrow)
    Paint_DrawLine(x + 18, y + 24, x + 26, y + 10, CP_ACCENT_BLUE, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 26, y + 10, x + 34, y + 24, CP_ACCENT_BLUE, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 26, y + 10, x + 26, y + 34, CP_ACCENT_BLUE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    // Instruction
    Paint_DrawString_EN(x + 44, y + 10, cp->state.navInstruction, &Font12, CP_TEXT_WHITE, CP_BG_CARD);

    // Distance
    Paint_DrawString_EN(x + 44, y + 30, cp->state.navDistance, &Font16, CP_ACCENT_CYAN, CP_BG_CARD);

    // ETA (right side)
    Paint_DrawString_EN(x + w - 100, y + 10, "ETA", &Font8, CP_TEXT_DIM, CP_BG_CARD);
    Paint_DrawString_EN(x + w - 100, y + 22, cp->state.navETA, &Font16, CP_TEXT_WHITE, CP_BG_CARD);

    // Route progress bar at bottom of card
    if (h > 50) {
        uint16_t progY = y + h - 14;
        uint16_t progW = w - 24;
        CarPlayUltra_DrawHGradientBar(x + 12, progY, progW, 6, CP_ACCENT_BLUE, CP_ACCENT_CYAN, cp->displayState.navProgress);
    }
}

// ============================================================================
// ToF 3D Mapping — full-cluster view
// ============================================================================
// Layout (left 460 px cluster):
//   +-------------------------------------------------+  y=0
//   |  Header: "3D SCAN"  + closest-obstacle readout  |
//   +-------------------------------------------------+  y=40
//   |                                                 |
//   |   Top-down accumulated grid  (360 x 420)        |
//   |                                                 |
//   +--------------------+----------------------------+  y=520
//   |  Raw 8x8 heatmap   |  Status: frames / hazard   |
//   +--------------------+----------------------------+  y=600

static uint16_t tof_distance_to_heat(uint16_t mm) {
    if (mm == 0 || mm == 0xFFFF) return CP_BG_DARK;
    if (mm > CP_TOF_HEAT_MAX_MM) mm = CP_TOF_HEAT_MAX_MM;
    float t = (float)mm / (float)CP_TOF_HEAT_MAX_MM;  // 0 = near, 1 = far
    // Two-stop gradient: red -> yellow -> cyan
    if (t < 0.5f) {
        return CarPlayUltra_BlendColor(CP_TOF_HEAT_NEAR, CP_TOF_HEAT_MID, t * 2.0f);
    }
    return CarPlayUltra_BlendColor(CP_TOF_HEAT_MID, CP_TOF_HEAT_FAR, (t - 0.5f) * 2.0f);
}

void CarPlayUltra_DrawTofHeatmap(CarPlayUltra* cp, uint16_t x, uint16_t y,
                                 uint16_t w, uint16_t h) {
    // One filled rectangle per zone with a 1-px dark gutter for readability.
    const uint16_t cellW = w / TOF_GRID_W;
    const uint16_t cellH = h / TOF_GRID_H;

    Paint_DrawRectangle(x, y, x + w, y + h, CP_BG_DEEP, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    for (int row = 0; row < TOF_GRID_H; row++) {
        for (int col = 0; col < TOF_GRID_W; col++) {
            const ToF_Zone* z = &cp->tofFrame.zones[row * TOF_GRID_W + col];
            uint16_t color;
            if (!cp->tofFrame.valid) {
                color = CP_BG_DARK;
            } else if (z->status != 5 && z->status != 9) {
                color = CP_BG_DEEP;
            } else {
                color = tof_distance_to_heat(z->distance_mm);
            }
            uint16_t cx0 = x + col * cellW + 1;
            uint16_t cy0 = y + row * cellH + 1;
            uint16_t cx1 = x + (col + 1) * cellW - 1;
            uint16_t cy1 = y + (row + 1) * cellH - 1;
            Paint_DrawRectangle(cx0, cy0, cx1, cy1, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }
    }

    // Card border
    Paint_DrawRectangle(x, y, x + w, y + h, CP_CARD_BORDER, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
}

void CarPlayUltra_DrawTofTopDown(CarPlayUltra* cp, uint16_t x, uint16_t y,
                                 uint16_t w, uint16_t h) {
    Paint_DrawRectangle(x, y, x + w, y + h, CP_TOF_BG, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Faint 1 m grid — map is 8 m wide × 12 m deep, so 1 m = w/8 horizontally.
    for (int g = 1; g < 8; g++) {
        uint16_t gx = x + (g * w) / 8;
        Paint_DrawLine(gx, y + 2, gx, y + h - 2, CP_TOF_GRID_LINE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    }
    for (int g = 1; g < 12; g++) {
        uint16_t gy = y + (g * h) / 12;
        Paint_DrawLine(x + 2, gy, x + w - 2, gy, CP_TOF_GRID_LINE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    }

    // Pixel size of one grid cell.
    const float pxPerCellX = (float)w / (float)TOF_MAP_W;
    const float pxPerCellY = (float)h / (float)TOF_MAP_H;

    for (int row = 0; row < TOF_MAP_H; row++) {
        for (int col = 0; col < TOF_MAP_W; col++) {
            const ToF_MapCell* c = &cp->tofMap.cells[row * TOF_MAP_W + col];
            if (c->kind == TOF_CELL_EMPTY) continue;

            uint16_t color;
            switch (c->kind) {
                case TOF_CELL_GROUND: {
                    // Fade near -> far based on row.
                    float t = (float)row / (float)TOF_MAP_H;
                    color = CarPlayUltra_BlendColor(CP_TOF_GROUND_NEAR, CP_TOF_GROUND_FAR, t);
                    break;
                }
                case TOF_CELL_OBSTACLE: color = CP_TOF_OBSTACLE; break;
                case TOF_CELL_HAZARD:   color = CP_TOF_HAZARD;   break;
                default:                color = CP_BG_CARD;      break;
            }

            // Mix in confidence as brightness.
            uint8_t alpha = c->confidence;
            if (alpha < 32) alpha = 32;
            uint16_t dark = CarPlayUltra_BlendColor(CP_TOF_BG, color, alpha / 255.0f);

            // Map convention: row 0 = 12 m ahead, row TOF_MAP_H-1 = 0 m behind.
            // Flip vertically so "ahead" is drawn at the top of the card.
            int draw_row = (TOF_MAP_H - 1) - row;
            uint16_t px0 = x + (uint16_t)(col * pxPerCellX);
            uint16_t py0 = y + (uint16_t)(draw_row * pxPerCellY);
            uint16_t px1 = x + (uint16_t)((col + 1) * pxPerCellX);
            uint16_t py1 = y + (uint16_t)((draw_row + 1) * pxPerCellY);
            if (px1 <= px0) px1 = px0 + 1;
            if (py1 <= py0) py1 = py0 + 1;
            Paint_DrawRectangle(px0, py0, px1, py1, dark, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }
    }

    // Ego-vehicle triangle at (W/2, H/2) in map space — draw near-bottom so
    // the forward road shows above it.
    uint16_t egoX  = x + w / 2;
    uint16_t egoY  = y + h - (h / 3);
    Paint_DrawLine(egoX, egoY - 10, egoX - 7, egoY + 6, CP_TOF_EGO_VEHICLE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(egoX, egoY - 10, egoX + 7, egoY + 6, CP_TOF_EGO_VEHICLE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(egoX - 7, egoY + 6, egoX + 7, egoY + 6, CP_TOF_EGO_VEHICLE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    // Scan sweep — rotating ray emanating from the ego point, tied to animPhase.
    float sweep = cp->animPhase;
    float rayLen = (float)(h / 3);
    int16_t rx = egoX + (int16_t)(sinf(sweep) * rayLen);
    int16_t ry = egoY - (int16_t)(cosf(sweep) * rayLen);
    Paint_DrawLine(egoX, egoY, rx, ry, CP_ACCENT_CYAN, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    Paint_DrawRectangle(x, y, x + w, y + h, CP_CARD_BORDER, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
}

void CarPlayUltra_DrawMappingView(CarPlayUltra* cp) {
    // Background — slightly lifted from black so the map pops.
    Paint_DrawRectangle(CP_CLUSTER_X, CP_CLUSTER_Y,
                        CP_CLUSTER_X + CP_CLUSTER_W, CP_CLUSTER_Y + CP_CLUSTER_H,
                        CP_BG_DEEP, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Divider vs info panel
    Paint_DrawLine(CP_CLUSTER_W, 0, CP_CLUSTER_W, CP_SCREEN_H,
                   CP_BG_CARD, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Header
    Paint_DrawString_EN(16, 12, "3D ROAD SCAN", &Font20, CP_ACCENT_CYAN, CP_BG_DEEP);
    char hdr[32];
    if (cp->tofOnline && cp->tofClosestMm != 0xFFFF) {
        snprintf(hdr, sizeof(hdr), "%.2f m", cp->tofClosestMm * 0.001f);
        uint16_t c = (cp->tofClosestMm < CP_TOF_WARN_MM) ? CP_TOF_HAZARD : CP_TEXT_WHITE;
        Paint_DrawString_EN(CP_CLUSTER_W - 110, 12, hdr, &Font20, c, CP_BG_DEEP);
    } else {
        Paint_DrawString_EN(CP_CLUSTER_W - 110, 14, "--.--", &Font16, CP_TEXT_DIM, CP_BG_DEEP);
    }

    // Top-down accumulated grid — dominant element
    CarPlayUltra_DrawTofTopDown(cp, 16, 46, CP_CLUSTER_W - 32, 460);

    // Raw 8x8 heatmap (bottom-left corner)
    CarPlayUltra_DrawTofHeatmap(cp, 16, 516, 160, 72);

    // Status block next to the heatmap
    uint16_t sx = 16 + 170;
    uint16_t sy = 516;
    Paint_DrawString_EN(sx, sy,      "FRAMES",  &Font8, CP_TEXT_DIM,   CP_BG_DEEP);
    char num[20];
    snprintf(num, sizeof(num), "%lu", (unsigned long)cp->tofMap.frames_integrated);
    Paint_DrawString_EN(sx, sy + 10, num,       &Font16, CP_TEXT_WHITE, CP_BG_DEEP);

    Paint_DrawString_EN(sx, sy + 30, "SPEED",   &Font8, CP_TEXT_DIM,   CP_BG_DEEP);
    snprintf(num, sizeof(num), "%.0f mph", cp->displaySpeed);
    Paint_DrawString_EN(sx, sy + 40, num,       &Font16, CP_TEXT_LIGHT, CP_BG_DEEP);

    if (!cp->tofOnline) {
        Paint_DrawString_EN(sx, sy + 60, "SENSOR OFFLINE", &Font12, CP_ACCENT_ORANGE, CP_BG_DEEP);
    } else if (cp->tofClosestMm < CP_TOF_WARN_MM) {
        Paint_DrawString_EN(sx, sy + 60, "HAZARD AHEAD",   &Font12, CP_TOF_HAZARD,    CP_BG_DEEP);
    } else {
        Paint_DrawString_EN(sx, sy + 60, "SCAN OK",        &Font12, CP_ACCENT_GREEN,  CP_BG_DEEP);
    }
}

// Small card pinned inside the right-hand info panel. Always visible so the
// instrument-cluster view still shows the closest-obstacle reading.
void CarPlayUltra_DrawTofMiniCard(CarPlayUltra* cp) {
    // Tuck it into the status bar row so we don't collide with existing cards.
    const uint16_t w = 128;
    const uint16_t h = 28;
    const uint16_t x = CP_PANEL_X + CP_PANEL_W - w - 8;
    const uint16_t y = 4;

    CarPlayUltra_DrawRoundedCard(x, y, w, h, CP_BG_CARD, CP_CARD_BORDER);
    Paint_DrawString_EN(x + 8, y + 4, "ToF", &Font8, CP_TEXT_DIM, CP_BG_CARD);

    char line[20];
    uint16_t color;
    if (!cp->tofOnline) {
        snprintf(line, sizeof(line), "offline");
        color = CP_TEXT_DIM;
    } else if (cp->tofClosestMm == 0xFFFF) {
        snprintf(line, sizeof(line), "clear");
        color = CP_ACCENT_GREEN;
    } else {
        snprintf(line, sizeof(line), "%.1fm", cp->tofClosestMm * 0.001f);
        color = (cp->tofClosestMm < CP_TOF_WARN_MM) ? CP_TOF_HAZARD : CP_ACCENT_CYAN;
    }
    Paint_DrawString_EN(x + 32, y + 6, line, &Font16, color, CP_BG_CARD);
}

void CarPlayUltra_DrawTofWarning(CarPlayUltra* cp) {
    if (!cp || !cp->tofOnline) return;
    if (cp->tofClosestMm == 0xFFFF || cp->tofClosestMm >= CP_TOF_WARN_MM) return;

    // Pulsing red border — uses animPhase so it breathes in time with the scan.
    float pulse = 0.5f + 0.5f * sinf(cp->animPhase * 3.0f);
    uint16_t col = CarPlayUltra_BlendColor(CP_BG_BLACK, CP_TOF_HAZARD, 0.4f + 0.6f * pulse);
    for (int t = 0; t < 3; t++) {
        Paint_DrawRectangle(t, t, CP_SCREEN_W - 1 - t, CP_SCREEN_H - 1 - t,
                            col, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }
    // Text banner at the top.
    Paint_DrawRectangle(CP_CLUSTER_W / 2 - 80, 2, CP_CLUSTER_W / 2 + 80, 22,
                        CP_TOF_HAZARD, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(CP_CLUSTER_W / 2 - 62, 5, "HAZARD AHEAD", &Font12, CP_TEXT_WHITE, CP_TOF_HAZARD);
}
