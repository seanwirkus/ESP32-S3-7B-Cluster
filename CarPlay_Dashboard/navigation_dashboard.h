#ifndef __NAVIGATION_DASHBOARD_H
#define __NAVIGATION_DASHBOARD_H

#include "gui_paint.h"
#include "fonts.h"
#include <stdint.h>
#include <stdbool.h>

// Navigation component configuration tuned for 1024x600 RGB panel
#define NAV_DASHBOARD_WIDTH    1024
#define NAV_DASHBOARD_HEIGHT   600
#define NAV_STATUS_HEIGHT      70
#define NAV_MAP_PADDING        22
#define NAV_CARD_HEIGHT        180

// UI Element positions
#define NAV_MAP_X              0
#define NAV_MAP_Y              0
#define NAV_MAP_WIDTH          NAV_DASHBOARD_WIDTH
#define NAV_MAP_HEIGHT         (NAV_DASHBOARD_HEIGHT - NAV_CARD_HEIGHT)

// Navigation text positions (within lower card)
#define NAV_CARD_PADDING       26
#define NAV_DISTANCE_X         (NAV_CARD_PADDING)
#define NAV_DISTANCE_Y         (NAV_MAP_HEIGHT + 22)
#define NAV_INSTRUCTION_X      (NAV_CARD_PADDING)
#define NAV_INSTRUCTION_Y      (NAV_MAP_HEIGHT + 64)
#define NAV_NEXT_STREET_X      (NAV_CARD_PADDING)
#define NAV_NEXT_STREET_Y      (NAV_MAP_HEIGHT + 104)
#define NAV_ETA_X              (NAV_DASHBOARD_WIDTH - 240)
#define NAV_ETA_Y              (NAV_MAP_HEIGHT + 26)
#define NAV_TOTAL_MILEAGE_X    (NAV_DASHBOARD_WIDTH - 240)
#define NAV_TOTAL_MILEAGE_Y    (NAV_MAP_HEIGHT + 66)
#define NAV_MODE_X             (NAV_DASHBOARD_WIDTH - 240)
#define NAV_MODE_Y             (NAV_MAP_HEIGHT + 104)

// Gauge positions for small clusters on the card
#define NAV_COMPASS_CENTER_X   820
#define NAV_COMPASS_CENTER_Y   (NAV_MAP_HEIGHT + 120)
#define NAV_COMPASS_RADIUS     36
#define NAV_TEMPERATURE_X      (NAV_CARD_PADDING + 420)
#define NAV_TEMPERATURE_Y      (NAV_MAP_HEIGHT + 120)

// Battery indicator on status bar
#define NAV_BATTERY_X          (NAV_DASHBOARD_WIDTH - 120)
#define NAV_BATTERY_Y          18

// Progress bar positions
#define NAV_PROGRESS_X         (NAV_CARD_PADDING)
#define NAV_PROGRESS_Y         (NAV_MAP_HEIGHT + NAV_CARD_HEIGHT - 36)
#define NAV_PROGRESS_WIDTH     (NAV_DASHBOARD_WIDTH - (NAV_CARD_PADDING * 2))
#define NAV_PROGRESS_HEIGHT    18

// Color definitions for navigation dashboard
#define NAV_BACKGROUND_COLOR   0x1082  // Deep blue-black background
#define NAV_TEXT_WHITE         WHITE
#define NAV_TEXT_GREEN         0x07E0  // Bright green for ECO mode
#define NAV_TEXT_GRAY          0x5ACB  // Light gray for secondary text
#define NAV_ACCENT_COLOR       0x39E7  // Muted cyan accent for cards
#define NAV_PROGRESS_COLOR     0x0260  // Progress bar color
#define NAV_MAP_ROAD_MAIN      0xC618  // Soft white road
#define NAV_MAP_ROAD_SECONDARY 0x739C  // Gray road
#define NAV_MAP_ROUTE          0xF800  // Red active route line
#define NAV_MAP_SKY            0x10A2  // Slightly lighter header band

// Navigation state structure
typedef struct {
    // Navigation data
    char distance[16];           // e.g., "4.2 mi"
    char instruction[64];        // e.g., "Take a slight left turn for I-395 North"
    char nextStreet[32];         // e.g., "onto Massachusetts Ave"
    char currentStreet[32];      // e.g., "3rd St NW"
    char arrivalTime[8];         // e.g., "9:41"
    char eta[8];                 // e.g., "12 min"
    uint32_t totalMileage;       // e.g., 14175
    char drivingMode[8];         // e.g., "ECO"

    // Vehicle data
    uint16_t temperature;        // e.g., 161 (degrees)
    uint8_t batteryLevel;        // e.g., 85 (percent)
    int16_t compassHeading;      // e.g., 161 (degrees)
    float speedKph;              // live speed reading
    float remainingDistanceKm;   // remaining route distance
    float mapProgress;           // 0-1 progress used for rendering

    // UI state
    bool showNavigation;         // Whether to show navigation
    bool isNavigating;           // Active navigation state
    uint8_t progressPercent;     // Route completion 0-100

    // Timing state
    uint32_t lastUpdateMs;
    uint32_t lastArrivalBaseMin; // starting arrival base time for formatting

} NavigationState;

// Navigation component structure
typedef struct {
    NavigationState state;
    bool initialized;
} NavigationDashboard;

// Function declarations

/**
 * @brief Initialize the navigation dashboard component
 * @param nav Pointer to navigation dashboard structure
 */
void NavigationDashboard_Init(NavigationDashboard* nav);

/**
 * @brief Update navigation state with new data
 * @param nav Pointer to navigation dashboard structure
 * @param newState New navigation state data
 */
void NavigationDashboard_UpdateState(NavigationDashboard* nav, NavigationState* newState);

/**
 * @brief Tick navigation simulation / smoothing for high refresh rates
 * @param nav Pointer to navigation dashboard structure
 * @param nowMs Current time in milliseconds
 */
void NavigationDashboard_Tick(NavigationDashboard* nav, uint32_t nowMs);

/**
 * @brief Render the complete navigation dashboard
 * @param nav Pointer to navigation dashboard structure
 * @param x Top-left X coordinate for the component
 * @param y Top-left Y coordinate for the component
 */
void NavigationDashboard_Render(NavigationDashboard* nav, uint16_t x, uint16_t y);

/**
 * @brief Draw top status bar (time, connectivity)
 */
void NavigationDashboard_DrawStatusBar(NavigationDashboard* nav, uint16_t x, uint16_t y);

/**
 * @brief Draw stylized map canvas and route
 */
void NavigationDashboard_DrawMap(NavigationDashboard* nav, uint16_t x, uint16_t y);

/**
 * @brief Draw the lower navigation card (instructions, ETA, etc.)
 */
void NavigationDashboard_DrawNavigationCard(NavigationDashboard* nav, uint16_t x, uint16_t y);

/**
 * @brief Render navigation background and container
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawBackground(uint16_t x, uint16_t y);

/**
 * @brief Render navigation text elements
 * @param nav Pointer to navigation dashboard structure
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawNavigationText(NavigationDashboard* nav, uint16_t x, uint16_t y);

/**
 * @brief Render vehicle information display
 * @param nav Pointer to navigation dashboard structure
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawVehicleInfo(NavigationDashboard* nav, uint16_t x, uint16_t y);

/**
 * @brief Render compass/gauge component
 * @param nav Pointer to navigation dashboard structure
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawCompass(NavigationDashboard* nav, uint16_t x, uint16_t y);

/**
 * @brief Render progress bar for route completion
 * @param percent Progress percentage (0-100)
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawProgressBar(uint8_t percent, uint16_t x, uint16_t y);

/**
 * @brief Render directional arrow icon
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param size Arrow size
 * @param color Arrow color
 */
void NavigationDashboard_DrawArrow(uint16_t x, uint16_t y, uint16_t size, uint16_t color);

/**
 * @brief Render circular gauge with temperature
 * @param value Temperature value
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param radius Gauge radius
 */
void NavigationDashboard_DrawTemperatureGauge(uint16_t value, uint16_t x, uint16_t y, uint16_t radius);

/**
 * @brief Render battery indicator
 * @param level Battery level (0-100)
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawBatteryIndicator(uint8_t level, uint16_t x, uint16_t y);

/**
 * @brief Draw a rounded rectangle with shadow
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 * @param radius Corner radius
 * @param fillColor Fill color
 * @param borderColor Border color
 */
void NavigationDashboard_DrawRoundedRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                                       uint16_t radius, uint16_t fillColor, uint16_t borderColor);

/**
 * @brief Utility function to draw a line with specified angle and length
 * @param x1 Starting X coordinate
 * @param y1 Starting Y coordinate
 * @param length Line length
 * @param angle Angle in degrees
 * @param color Line color
 * @param width Line width
 */
void NavigationDashboard_DrawAngledLine(uint16_t x1, uint16_t y1, uint16_t length, int16_t angle, 
                                      uint16_t color, uint8_t width);

/**
 * @brief Convert temperature value to display string
 * @param temp Temperature value
 * @param buffer Output buffer
 */
void NavigationDashboard_FormatTemperature(uint16_t temp, char* buffer);

/**
 * @brief Convert mileage to display string with commas
 * @param mileage Mileage value
 * @param buffer Output buffer
 */
void NavigationDashboard_FormatMileage(uint32_t mileage, char* buffer);

#endif /* __NAVIGATION_DASHBOARD_H */