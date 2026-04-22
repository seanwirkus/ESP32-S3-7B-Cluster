#include "navigation_dashboard.h"
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

// Helper macros for trigonometric calculations
#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)
#define RAD_TO_DEG(rad) ((rad) * 180.0 / M_PI)

typedef struct {
    uint16_t x;
    uint16_t y;
} MapPoint;

static void NavigationDashboard_DrawPolyline(const MapPoint* points, size_t count, uint16_t color, DOT_PIXEL width) {
    if (points == NULL || count < 2) return;
    for (size_t i = 0; i < count - 1; ++i) {
        Paint_DrawLine(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y, color, width, LINE_STYLE_SOLID);
    }
}

/**
 * @brief Initialize the navigation dashboard component
 * @param nav Pointer to navigation dashboard structure
 */
void NavigationDashboard_Init(NavigationDashboard* nav) {
    if (nav == NULL) return;

    // Initialize default state
    memset(nav, 0, sizeof(NavigationDashboard));

    // Set default values
    strcpy(nav->state.distance, "4.2 mi");
    strcpy(nav->state.instruction, "Take a slight left turn for I-395 North");
    strcpy(nav->state.nextStreet, "onto Massachusetts Ave");
    strcpy(nav->state.currentStreet, "3rd St NW");
    strcpy(nav->state.arrivalTime, "9:41");
    strcpy(nav->state.eta, "12 min");
    nav->state.totalMileage = 14175;
    strcpy(nav->state.drivingMode, "ECO");
    nav->state.temperature = 161;
    nav->state.batteryLevel = 85;
    nav->state.compassHeading = 161;
    nav->state.speedKph = 48.0f;
    nav->state.remainingDistanceKm = 6.7f;
    nav->state.mapProgress = 0.0f;
    nav->state.showNavigation = true;
    nav->state.isNavigating = true;
    nav->state.progressPercent = 65;
    nav->state.lastArrivalBaseMin = 9 * 60 + 29; // 9:29 AM baseline

    nav->initialized = true;
}

/**
 * @brief Update navigation state with new data
 * @param nav Pointer to navigation dashboard structure
 * @param newState New navigation state data
 */
void NavigationDashboard_UpdateState(NavigationDashboard* nav, NavigationState* newState) {
    if (nav == NULL || newState == NULL) return;

    memcpy(&nav->state, newState, sizeof(NavigationState));
}

/**
 * @brief Tick navigation simulation / smoothing for high refresh rates
 * @param nav Pointer to navigation dashboard structure
 * @param nowMs Current time in milliseconds
 */
void NavigationDashboard_Tick(NavigationDashboard* nav, uint32_t nowMs) {
    if (nav == NULL || !nav->initialized) return;

    if (nav->state.lastUpdateMs == 0) {
        nav->state.lastUpdateMs = nowMs;
        return;
    }

    uint32_t delta = nowMs - nav->state.lastUpdateMs;
    nav->state.lastUpdateMs = nowMs;

    // Smoothly animate speed and heading for a lively UI
    float deltaSeconds = delta / 1000.0f;
    nav->state.speedKph = 45.0f + 5.0f * sinf(nowMs / 1200.0f);
    nav->state.compassHeading = 150 + (int16_t)(15.0f * sinf(nowMs / 1800.0f));

    // Progress movement along the route
    float progress = nav->state.progressPercent / 100.0f;
    progress += deltaSeconds * 0.012f; // ~1.2% per second
    if (progress > 1.0f) progress = 1.0f;
    nav->state.progressPercent = (uint8_t)(progress * 100.0f);
    nav->state.mapProgress = progress;

    // Update remaining distance and ETA
    float kmRemaining = nav->state.remainingDistanceKm - deltaSeconds * (nav->state.speedKph / 3.6f) * 0.6f;
    if (kmRemaining < 0.1f) kmRemaining = 0.1f;
    nav->state.remainingDistanceKm = kmRemaining;

    // Distance string (miles)
    float milesRemaining = kmRemaining * 0.621371f;
    snprintf(nav->state.distance, sizeof(nav->state.distance), "%.1f mi", milesRemaining);

    // ETA in minutes
    uint16_t etaMinutes = (uint16_t)((kmRemaining / (nav->state.speedKph / 60.0f)) + 0.5f);
    snprintf(nav->state.eta, sizeof(nav->state.eta), "%u min", etaMinutes);

    // Arrival time from a fixed base clock to mimic CarPlay
    uint32_t arrivalMinutes = nav->state.lastArrivalBaseMin + etaMinutes;
    uint16_t arrivalHour = (arrivalMinutes / 60) % 24;
    uint16_t arrivalMin = arrivalMinutes % 60;
    snprintf(nav->state.arrivalTime, sizeof(nav->state.arrivalTime), "%02u:%02u", arrivalHour, arrivalMin);
}

/**
 * @brief Render the complete navigation dashboard
 * @param nav Pointer to navigation dashboard structure
 * @param x Top-left X coordinate for the component
 * @param y Top-left Y coordinate for the component
 */
void NavigationDashboard_Render(NavigationDashboard* nav, uint16_t x, uint16_t y) {
    if (nav == NULL || !nav->initialized) return;

    // Draw background first
    NavigationDashboard_DrawBackground(x, y);

    // Map canvas and overlays
    NavigationDashboard_DrawMap(nav, x, y);
    NavigationDashboard_DrawStatusBar(nav, x, y);

    // Lower CarPlay-style card
    NavigationDashboard_DrawNavigationCard(nav, x, y);
}

/**
 * @brief Render navigation background and container
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawBackground(uint16_t x, uint16_t y) {
    // Draw main background rectangle covering the full dashboard footprint
    Paint_DrawRectangle(x, y,
                       x + NAV_DASHBOARD_WIDTH,
                       y + NAV_DASHBOARD_HEIGHT,
                       NAV_BACKGROUND_COLOR, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

/**
 * @brief Draw top status bar (time, connectivity)
 */
void NavigationDashboard_DrawStatusBar(NavigationDashboard* nav, uint16_t x, uint16_t y) {
    // Header band for sky/glass effect
    Paint_DrawRectangle(x, y, x + NAV_DASHBOARD_WIDTH, y + NAV_STATUS_HEIGHT,
                        NAV_MAP_SKY, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    char clockStr[8];
    uint16_t baseHour = nav->state.lastArrivalBaseMin / 60;
    uint16_t baseMin = nav->state.lastArrivalBaseMin % 60;
    snprintf(clockStr, sizeof(clockStr), "%02u:%02u", baseHour, baseMin);

    Paint_DrawString_EN(x + 20, y + 20, clockStr, &Font20, NAV_TEXT_WHITE, NAV_MAP_SKY);
    Paint_DrawString_EN(x + 110, y + 22, "CarPlay Studio", &Font16, NAV_TEXT_GRAY, NAV_MAP_SKY);

    // Network bars
    uint16_t barX = x + NAV_DASHBOARD_WIDTH - 200;
    for (int i = 0; i < 4; ++i) {
        uint16_t height = 8 + i * 4;
        Paint_DrawRectangle(barX + (i * 8), y + NAV_STATUS_HEIGHT - height - 10,
                            barX + (i * 8) + 4, y + NAV_STATUS_HEIGHT - 10,
                            NAV_TEXT_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    Paint_DrawString_EN(barX + 40, y + 20, "LTE", &Font12, NAV_TEXT_WHITE, NAV_MAP_SKY);

    // Speed preview on the status bar
    char speedStr[16];
    snprintf(speedStr, sizeof(speedStr), "%2.0f km/h", nav->state.speedKph);
    Paint_DrawString_EN(x + NAV_DASHBOARD_WIDTH / 2 - 30, y + 20, speedStr, &Font12, NAV_TEXT_WHITE, NAV_MAP_SKY);

    // Battery indicator aligned to the right
    NavigationDashboard_DrawBatteryIndicator(nav->state.batteryLevel, x + NAV_BATTERY_X, y + NAV_BATTERY_Y);
}

/**
 * @brief Draw stylized map canvas and route
 */
void NavigationDashboard_DrawMap(NavigationDashboard* nav, uint16_t x, uint16_t y) {
    // Map background
    Paint_DrawRectangle(x + NAV_MAP_X, y + NAV_MAP_Y + NAV_STATUS_HEIGHT,
                        x + NAV_MAP_WIDTH, y + NAV_MAP_HEIGHT,
                        NAV_BACKGROUND_COLOR, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Secondary road grid
    MapPoint secondaryGrid[] = {
        {60, 220}, {240, 180}, {420, 200}, {620, 180}, {840, 220}
    };
    NavigationDashboard_DrawPolyline(secondaryGrid, sizeof(secondaryGrid) / sizeof(MapPoint), NAV_MAP_ROAD_SECONDARY, DOT_PIXEL_1X1);

    MapPoint verticalGrid[] = {
        {200, 120}, {200, 420}, {320, 160}, {320, 420}
    };
    NavigationDashboard_DrawPolyline(verticalGrid, 2, NAV_MAP_ROAD_SECONDARY, DOT_PIXEL_1X1);
    NavigationDashboard_DrawPolyline(&verticalGrid[2], 2, NAV_MAP_ROAD_SECONDARY, DOT_PIXEL_1X1);

    // Main roads
    MapPoint mainRoad[] = {
        {80, 360}, {260, 320}, {420, 360}, {640, 340}, {900, 360}
    };
    NavigationDashboard_DrawPolyline(mainRoad, sizeof(mainRoad) / sizeof(MapPoint), NAV_MAP_ROAD_MAIN, DOT_PIXEL_2X2);

    // Active route polyline
    MapPoint route[] = {
        {140, 400}, {280, 300}, {440, 260}, {620, 300}, {760, 240}, {920, 280}
    };
    NavigationDashboard_DrawPolyline(route, sizeof(route) / sizeof(MapPoint), NAV_MAP_ROUTE, DOT_PIXEL_2X2);

    // Determine current position along route
    float t = nav->state.mapProgress;
    if (t > 0.98f) t = 0.98f;
    size_t segmentCount = (sizeof(route) / sizeof(MapPoint)) - 1;
    float scaled = t * segmentCount;
    size_t segIndex = (size_t)scaled;
    float localT = scaled - segIndex;
    MapPoint start = route[segIndex];
    MapPoint end = route[segIndex + 1];
    uint16_t curX = (uint16_t)(start.x + (end.x - start.x) * localT);
    uint16_t curY = (uint16_t)(start.y + (end.y - start.y) * localT);

    // Draw vehicle locator
    Paint_DrawCircle(curX, curY, 8, NAV_TEXT_WHITE, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    Paint_DrawCircle(curX, curY, 12, NAV_MAP_ROUTE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    // Heading triangle using angled lines
    NavigationDashboard_DrawAngledLine(curX, curY, 20, nav->state.compassHeading - 90, NAV_TEXT_WHITE, 2);
}

/**
 * @brief Draw the lower navigation card (instructions, ETA, etc.)
 */
void NavigationDashboard_DrawNavigationCard(NavigationDashboard* nav, uint16_t x, uint16_t y) {
    uint16_t cardTop = y + NAV_MAP_HEIGHT;
    NavigationDashboard_DrawRoundedRect(x + NAV_CARD_PADDING / 2, cardTop + 4,
                                        NAV_DASHBOARD_WIDTH - NAV_CARD_PADDING, NAV_CARD_HEIGHT - 8,
                                        12, NAV_ACCENT_COLOR, NAV_ACCENT_COLOR);

    NavigationDashboard_DrawNavigationText(nav, x, y);
    NavigationDashboard_DrawVehicleInfo(nav, x, y);
    NavigationDashboard_DrawCompass(nav, x, y);
    NavigationDashboard_DrawTemperatureGauge(nav->state.temperature, x + NAV_TEMPERATURE_X, y + NAV_TEMPERATURE_Y, NAV_COMPASS_RADIUS);
    NavigationDashboard_DrawProgressBar(nav->state.progressPercent, x + NAV_PROGRESS_X, y + NAV_PROGRESS_Y);
}

/**
 * @brief Render navigation text elements
 * @param nav Pointer to navigation dashboard structure
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawNavigationText(NavigationDashboard* nav, uint16_t x, uint16_t y) {
    if (!nav->state.showNavigation) return;

    // Draw distance with large, legible font
    Paint_DrawString_EN(x + NAV_DISTANCE_X, y + NAV_DISTANCE_Y,
                       nav->state.distance, &Font48, NAV_TEXT_WHITE, NAV_ACCENT_COLOR);

    // Draw primary instruction
    Paint_DrawString_EN(x + NAV_INSTRUCTION_X, y + NAV_INSTRUCTION_Y,
                       nav->state.instruction, &Font20, NAV_TEXT_WHITE, NAV_ACCENT_COLOR);

    // Draw next street hint
    Paint_DrawString_EN(x + NAV_NEXT_STREET_X, y + NAV_NEXT_STREET_Y,
                       nav->state.nextStreet, &Font16, NAV_TEXT_GRAY, NAV_ACCENT_COLOR);
}

/**
 * @brief Render vehicle information display
 * @param nav Pointer to navigation dashboard structure
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawVehicleInfo(NavigationDashboard* nav, uint16_t x, uint16_t y) {
    char mileageStr[16];
    NavigationDashboard_FormatMileage(nav->state.totalMileage, mileageStr);

    // ETA and arrival time block
    Paint_DrawString_EN(x + NAV_ETA_X, y + NAV_ETA_Y,
                       nav->state.eta, &Font24, NAV_TEXT_WHITE, NAV_ACCENT_COLOR);
    Paint_DrawString_EN(x + NAV_ETA_X, y + NAV_ETA_Y + 30,
                       nav->state.arrivalTime, &Font16, NAV_TEXT_GRAY, NAV_ACCENT_COLOR);

    // Draw total mileage and mode stacked for a compact card layout
    Paint_DrawString_EN(x + NAV_TOTAL_MILEAGE_X, y + NAV_TOTAL_MILEAGE_Y,
                       mileageStr, &Font12, NAV_TEXT_GRAY, NAV_ACCENT_COLOR);

    Paint_DrawString_EN(x + NAV_MODE_X, y + NAV_MODE_Y,
                       nav->state.drivingMode, &Font16, NAV_TEXT_GREEN, NAV_ACCENT_COLOR);
}

/**
 * @brief Render compass/gauge component
 * @param nav Pointer to navigation dashboard structure
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawCompass(NavigationDashboard* nav, uint16_t x, uint16_t y) {
    uint16_t centerX = x + NAV_COMPASS_CENTER_X;
    uint16_t centerY = y + NAV_COMPASS_CENTER_Y;
    uint16_t radius = NAV_COMPASS_RADIUS;

    // Draw compass circle
    Paint_DrawCircle(centerX, centerY, radius, NAV_ACCENT_COLOR, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    // Draw cardinal directions (N, E, S, W)
    Paint_DrawString_EN(centerX - 4, centerY - radius - 15, "N", &Font8, NAV_TEXT_WHITE, NAV_ACCENT_COLOR);
    Paint_DrawString_EN(centerX + radius + 2, centerY - 4, "E", &Font8, NAV_TEXT_WHITE, NAV_ACCENT_COLOR);
    Paint_DrawString_EN(centerX - 4, centerY + radius + 2, "S", &Font8, NAV_TEXT_WHITE, NAV_ACCENT_COLOR);
    Paint_DrawString_EN(centerX - radius - 10, centerY - 4, "W", &Font8, NAV_TEXT_WHITE, NAV_ACCENT_COLOR);

    // Draw heading indicator
    int16_t heading = nav->state.compassHeading;
    uint16_t needleLength = radius - 5;

    // Calculate needle endpoint
    float rad = DEG_TO_RAD(heading);
    uint16_t endX = centerX + (int16_t)(needleLength * sin(rad));
    uint16_t endY = centerY - (int16_t)(needleLength * cos(rad));

    // Draw needle
    Paint_DrawLine(centerX, centerY, endX, endY, NAV_TEXT_WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
}

/**
 * @brief Render progress bar for route completion
 * @param percent Progress percentage (0-100)
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawProgressBar(uint8_t percent, uint16_t x, uint16_t y) {
    uint16_t width = NAV_PROGRESS_WIDTH;
    uint16_t height = NAV_PROGRESS_HEIGHT;

    // Draw background
    Paint_DrawRectangle(x, y, x + width, y + height, NAV_ACCENT_COLOR, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Draw progress fill
    uint16_t fillWidth = (uint16_t)((percent * width) / 100);
    if (fillWidth > 0) {
        Paint_DrawRectangle(x, y, x + fillWidth, y + height, NAV_PROGRESS_COLOR, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    // Draw border
    Paint_DrawRectangle(x, y, x + width, y + height, NAV_TEXT_WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
}

/**
 * @brief Render directional arrow icon
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param size Arrow size
 * @param color Arrow color
 */
void NavigationDashboard_DrawArrow(uint16_t x, uint16_t y, uint16_t size, uint16_t color) {
    // Simple arrow pointing right
    uint16_t halfSize = size / 2;

    // Draw arrow shaft
    Paint_DrawLine(x - halfSize, y, x + halfSize, y, color, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    // Draw arrow head
    Paint_DrawLine(x + halfSize, y, x + halfSize - 5, y - 5, color, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(x + halfSize, y, x + halfSize - 5, y + 5, color, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
}

/**
 * @brief Render circular gauge with temperature
 * @param value Temperature value
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param radius Gauge radius
 */
void NavigationDashboard_DrawTemperatureGauge(uint16_t value, uint16_t x, uint16_t y, uint16_t radius) {
    // Draw gauge background
    Paint_DrawCircle(x, y, radius, NAV_ACCENT_COLOR, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    // Draw temperature value in center
    char tempStr[8];
    sprintf(tempStr, "%d", value);
    Paint_DrawString_EN(x - 15, y - 8, tempStr, &Font12, NAV_TEXT_WHITE, NAV_ACCENT_COLOR);
    Paint_DrawString_EN(x + 5, y - 8, "°", &Font8, NAV_TEXT_WHITE, NAV_ACCENT_COLOR);
}

/**
 * @brief Render battery indicator
 * @param level Battery level (0-100)
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 */
void NavigationDashboard_DrawBatteryIndicator(uint8_t level, uint16_t x, uint16_t y) {
    uint16_t width = 30;
    uint16_t height = 15;

    // Draw battery outline
    Paint_DrawRectangle(x, y, x + width, y + height, NAV_TEXT_WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    // Draw battery terminal
    Paint_DrawRectangle(x + width, y + 4, x + width + 2, y + height - 4, NAV_TEXT_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Draw battery level
    uint16_t fillWidth = (uint16_t)((level * (width - 2)) / 100);
    if (fillWidth > 0) {
        uint16_t color = (level > 20) ? NAV_TEXT_GREEN : NAV_ACCENT_COLOR;
        Paint_DrawRectangle(x + 1, y + 1, x + 1 + fillWidth, y + height - 1, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    // Draw percentage text
    char levelStr[8];
    sprintf(levelStr, "%d%%", level);
    Paint_DrawString_EN(x + width + 5, y, levelStr, &Font8, NAV_TEXT_WHITE, NAV_MAP_SKY);
}

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
                                       uint16_t radius, uint16_t fillColor, uint16_t borderColor) {
    // For simplicity, draw regular rectangle since Paint doesn't have rounded rect
    // This could be enhanced with custom drawing using lines and arcs
    Paint_DrawRectangle(x, y, x + width, y + height, fillColor, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    if (borderColor != fillColor) {
        Paint_DrawRectangle(x, y, x + width, y + height, borderColor, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }
}

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
                                      uint16_t color, uint8_t width) {
    float rad = DEG_TO_RAD(angle);
    uint16_t x2 = x1 + (uint16_t)(length * cos(rad));
    uint16_t y2 = y1 + (uint16_t)(length * sin(rad));

    DOT_PIXEL dotPixel = (width == 1) ? DOT_PIXEL_1X1 : DOT_PIXEL_2X2;
    Paint_DrawLine(x1, y1, x2, y2, color, dotPixel, LINE_STYLE_SOLID);
}

/**
 * @brief Convert temperature value to display string
 * @param temp Temperature value
 * @param buffer Output buffer
 */
void NavigationDashboard_FormatTemperature(uint16_t temp, char* buffer) {
    sprintf(buffer, "%d°", temp);
}

/**
 * @brief Convert mileage to display string with commas
 * @param mileage Mileage value
 * @param buffer Output buffer
 */
void NavigationDashboard_FormatMileage(uint32_t mileage, char* buffer) {
    // Simple formatting - could be enhanced for comma separation
    sprintf(buffer, "%lu", mileage);
}
