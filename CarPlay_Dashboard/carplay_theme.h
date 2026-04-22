#ifndef __CARPLAY_THEME_H
#define __CARPLAY_THEME_H

#include <stdint.h>

// ============================================================================
// CarPlay Ultra — Premium Theme & Layout Constants
// Target: ESP32-S3 + Waveshare 7" 1024x600 RGB LCD
// ============================================================================

// --- Display ---
#define CP_SCREEN_W          1024
#define CP_SCREEN_H          600

// --- RGB565 Color Palette (CarPlay Ultra Dark Theme) ---
// Backgrounds
#define CP_BG_BLACK          0x0000  // Pure black
#define CP_BG_DEEP           0x0841  // Very dark blue-black
#define CP_BG_DARK           0x1082  // Dark panel background
#define CP_BG_CARD           0x18C3  // Card background (subtle lift)
#define CP_BG_CARD_HOVER     0x2104  // Card hover/active

// Accent Colors
#define CP_ACCENT_CYAN       0x07FF  // Bright cyan (speedometer)
#define CP_ACCENT_TEAL       0x0599  // Teal gauge sweep
#define CP_ACCENT_GREEN      0x07E0  // Bright green (eco/good)
#define CP_ACCENT_LIME       0x47E0  // Lime green for gauge
#define CP_ACCENT_BLUE       0x03BF  // Deep blue accent
#define CP_ACCENT_PURPLE     0x781F  // Purple accent
#define CP_ACCENT_ORANGE     0xFC00  // Warning orange
#define CP_ACCENT_RED        0xF800  // Alert red
#define CP_ACCENT_YELLOW     0xFFE0  // Yellow highlight
#define CP_ACCENT_MAGENTA    0xF81F  // Magenta/pink

// Text Colors
#define CP_TEXT_WHITE         0xFFFF  // Primary text
#define CP_TEXT_LIGHT         0xDEFB  // Secondary text
#define CP_TEXT_GRAY          0x8C71  // Muted text
#define CP_TEXT_DIM           0x52AA  // Very dim text
#define CP_TEXT_GREEN         0x07E0  // Green text (positive values)

// Gauge Colors
#define CP_GAUGE_BG           0x1082  // Gauge track background
#define CP_GAUGE_TICK          0x4228  // Dim tick marks
#define CP_GAUGE_TICK_BRIGHT   0xAD55  // Bright tick marks
#define CP_GAUGE_SWEEP_CYAN    0x07FF  // Main sweep color
#define CP_GAUGE_SWEEP_GREEN   0x2EE9  // Green part of sweep
#define CP_GAUGE_SWEEP_TEAL    0x0599  // Teal part of sweep
#define CP_GAUGE_NEEDLE        0xFFFF  // Needle color
#define CP_GAUGE_CENTER        0x31A6  // Center hub

// Map Colors
#define CP_MAP_BG              0x0841  // Map background dark
#define CP_MAP_ROAD_MAIN       0x2945  // Main road gray
#define CP_MAP_ROAD_SEC        0x18C3  // Secondary road
#define CP_MAP_ROUTE           0x03BF  // Active route blue
#define CP_MAP_ROUTE_GLOW      0x02DF  // Route glow
#define CP_MAP_STREET_LABEL    0x4228  // Street label background
#define CP_MAP_PIN             0xF800  // Location pin red
#define CP_MAP_POSITION        0x07FF  // Current position cyan

// Card Specific
#define CP_CARD_BORDER         0x2945  // Subtle card border
#define CP_CARD_DIVIDER        0x2104  // Divider line in cards

// --- Layout: Instrument Cluster (Left Side) ---
// The left ~460px is the instrument cluster area
#define CP_CLUSTER_X           0
#define CP_CLUSTER_Y           0
#define CP_CLUSTER_W           460
#define CP_CLUSTER_H           CP_SCREEN_H

// Speedometer (main gauge, left)
#define CP_SPEEDO_CX           155     // Center X of speedometer arc
#define CP_SPEEDO_CY           280     // Center Y
#define CP_SPEEDO_R_OUTER      140     // Outer radius
#define CP_SPEEDO_R_INNER      105     // Inner radius (for arc thickness)
#define CP_SPEEDO_R_TICK       148     // Tick mark outer radius
#define CP_SPEEDO_START_ANGLE  135     // Start angle (degrees, 0=right, CW)
#define CP_SPEEDO_END_ANGLE    405     // End angle (270 degree sweep)
#define CP_SPEEDO_MAX_SPEED    160     // Max speed on gauge (mph)

// RPM / Secondary Gauge (right side of cluster)
#define CP_RPM_CX              355     // Center X of RPM arc
#define CP_RPM_CY              280     // Center Y
#define CP_RPM_R_OUTER         105     // Outer radius
#define CP_RPM_R_INNER         78      // Inner radius
#define CP_RPM_R_TICK          112     // Tick outer radius
#define CP_RPM_START_ANGLE     135     // Start angle
#define CP_RPM_END_ANGLE       405     // End angle
#define CP_RPM_MAX             8000    // Max RPM

// Gear indicator (between gauges)
#define CP_GEAR_X              250     // Gear display X
#define CP_GEAR_Y              230     // Gear display Y

// Map inset (small map in cluster center-top)
#define CP_MAP_INSET_X         100
#define CP_MAP_INSET_Y         30
#define CP_MAP_INSET_W         260
#define CP_MAP_INSET_H         140

// Speed text (large, center of speedo)
#define CP_SPEED_TEXT_X        CP_SPEEDO_CX
#define CP_SPEED_TEXT_Y        (CP_SPEEDO_CY - 15)

// --- Layout: Info Panel (Right Side) ---
// The right ~564px is the CarPlay info panel
#define CP_PANEL_X             460
#define CP_PANEL_Y             0
#define CP_PANEL_W             564
#define CP_PANEL_H             CP_SCREEN_H

// Status bar at top of panel
#define CP_STATUS_X            CP_PANEL_X
#define CP_STATUS_Y            0
#define CP_STATUS_W            CP_PANEL_W
#define CP_STATUS_H            36

// Card grid (2x2 layout within info panel)
#define CP_CARD_MARGIN         8
#define CP_CARD_GAP            8
#define CP_CARD_ROW1_Y         (CP_STATUS_H + CP_CARD_MARGIN)
#define CP_CARD_ROW2_Y         (CP_STATUS_H + CP_CARD_MARGIN + 170 + CP_CARD_GAP)

// Trip Card (top-left of panel)
#define CP_TRIP_CARD_X         (CP_PANEL_X + CP_CARD_MARGIN)
#define CP_TRIP_CARD_Y         CP_CARD_ROW1_Y
#define CP_TRIP_CARD_W         180
#define CP_TRIP_CARD_H         170

// Calendar Card (top-center of panel)
#define CP_CAL_CARD_X          (CP_TRIP_CARD_X + CP_TRIP_CARD_W + CP_CARD_GAP)
#define CP_CAL_CARD_Y          CP_CARD_ROW1_Y
#define CP_CAL_CARD_W          180
#define CP_CAL_CARD_H          170

// Weather Card (top-right of panel)
#define CP_WEATHER_CARD_X      (CP_CAL_CARD_X + CP_CAL_CARD_W + CP_CARD_GAP)
#define CP_WEATHER_CARD_Y      CP_CARD_ROW1_Y
#define CP_WEATHER_CARD_W      180
#define CP_WEATHER_CARD_H      170

// Music / Now Playing Card (bottom spanning full width)
#define CP_MUSIC_CARD_X        (CP_PANEL_X + CP_CARD_MARGIN)
#define CP_MUSIC_CARD_Y        CP_CARD_ROW2_Y
#define CP_MUSIC_CARD_W        (CP_PANEL_W - CP_CARD_MARGIN * 2)
#define CP_MUSIC_CARD_H        130

// Navigation Card (bottom, below music)
#define CP_NAV_CARD_X          (CP_PANEL_X + CP_CARD_MARGIN)
#define CP_NAV_CARD_Y          (CP_MUSIC_CARD_Y + CP_MUSIC_CARD_H + CP_CARD_GAP)
#define CP_NAV_CARD_W          (CP_PANEL_W - CP_CARD_MARGIN * 2)
#define CP_NAV_CARD_H          (CP_SCREEN_H - CP_NAV_CARD_Y - CP_CARD_MARGIN)

// --- Layout: Bottom Status Strip (fuel/range across full width) ---
#define CP_BOTTOM_STRIP_X      0
#define CP_BOTTOM_STRIP_Y      (CP_SCREEN_H - 50)
#define CP_BOTTOM_STRIP_W      CP_CLUSTER_W
#define CP_BOTTOM_STRIP_H      50

// --- ToF Mapping View ---
#define CP_TOF_BG             0x0841   // Dark void behind the map
#define CP_TOF_GRID_LINE      0x2104   // Faint grid
#define CP_TOF_GROUND_NEAR    0x0599   // Teal close road
#define CP_TOF_GROUND_FAR     0x03BF   // Deep blue distant road
#define CP_TOF_OBSTACLE       0xFC00   // Orange obstacle
#define CP_TOF_HAZARD         0xF800   // Red imminent hazard
#define CP_TOF_HEAT_NEAR      0xF800   // Near = hot
#define CP_TOF_HEAT_MID       0xFFE0   // Mid = yellow
#define CP_TOF_HEAT_FAR       0x07FF   // Far = cyan
#define CP_TOF_EGO_VEHICLE    0x07FF   // Cyan triangle representing the car
#define CP_TOF_WARN_MM        1500     // Closer than this -> hazard overlay
#define CP_TOF_HEAT_MAX_MM    3500     // Heatmap full-scale distance

// --- Animation Constants ---
#define CP_ANIM_SPEED_SMOOTH   0.08f   // Speed interpolation factor
#define CP_ANIM_RPM_SMOOTH     0.10f   // RPM interpolation factor
#define CP_ANIM_NEEDLE_SMOOTH  0.12f   // Needle smoothing
#define CP_GAUGE_TICK_COUNT    32      // Number of tick marks around gauge
#define CP_GAUGE_MAJOR_EVERY   4       // Major tick every N ticks

#endif /* __CARPLAY_THEME_H */
