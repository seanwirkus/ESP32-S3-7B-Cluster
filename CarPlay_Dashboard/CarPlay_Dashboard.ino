/*
 * Subaru WRX-Style CarPlay Dashboard
 * For Waveshare ESP32-S3-Touch-LCD-7B (1024x600)
 * Using proper Waveshare LVGL port with vsync
 */

#include "lvgl_port.h"
#include "i2c.h"
#include "io_extension.h"

// SF Pro fonts (extern declarations)
LV_FONT_DECLARE(SF_Pro_16);
LV_FONT_DECLARE(SF_Pro_22);
LV_FONT_DECLARE(SF_Pro_40);
LV_FONT_DECLARE(SF_Pro_64);
LV_FONT_DECLARE(FA_Icons_20);

// Font Awesome icon Unicode points
#define FA_GAS_PUMP     "\xEF\x94\xAF"   // U+F52F
#define FA_THERMOMETER  "\xEF\x8B\x89"   // U+F2C9  
#define FA_TACHOMETER   "\xEF\x98\xA9"   // U+F629
#define FA_CAR          "\xEF\x86\xB9"   // U+F1B9
#define FA_OIL_CAN      "\xEF\x98\x93"   // U+F613
#define FA_ROAD         "\xEF\x80\x98"   // U+F018
#define FA_BOLT         "\xEF\x83\xA7"   // U+F0E7
#define FA_WRENCH       "\xEF\x82\xAD"   // U+F0AD
#define FA_COG          "\xEF\x80\x93"   // U+F013
#define FA_CLOCK        "\xEF\x80\x97"   // U+F017
#define FA_LOCATION     "\xEF\x8F\x85"   // U+F3C5
#define FA_FIRE         "\xEF\x81\xAD"   // U+F06D
#define FA_SUN          "\xEF\x86\x85"   // U+F185
#define FA_CAR_SIDE     "\xEF\x83\x90"   // U+F0D0

// Screen dimensions
#define SCREEN_W 1024
#define SCREEN_H 600

// Tesla-style EV Color Palette
#define WRX_BG_DARK      0x1F2121
#define WRX_BG_CARD      0x17181C
#define WRX_ACCENT_TEAL  0x00E5DD
#define WRX_ACCENT_GREEN 0x34D64A
#define WRX_ACCENT_RED   0xCC0303
#define WRX_ACCENT_ORANGE 0xFF6B35
#define WRX_ACCENT_BLUE  0x00E5DD
#define WRX_TEXT_PRIMARY 0xFFFFFF
#define WRX_TEXT_MUTED   0x6E707A
#define WRX_TEXT_DIM     0x4A4C55
#define WRX_GAUGE_BG     0x2A2C33
#define WRX_SUCCESS      0x34D64A
#define WRX_WARNING      0xFFD60A
#define WRX_DANGER       0xCC0303

// UI Elements
static lv_obj_t *gSpeedArc = NULL;
static lv_obj_t *gSpeedLabel = NULL;
static lv_obj_t *gTachArc = NULL;
static lv_obj_t *gTachLabel = NULL;
static lv_obj_t *gBoostArc = NULL;
static lv_obj_t *gBoostLabel = NULL;
static lv_obj_t *gOilTempBar = NULL;
static lv_obj_t *gCoolantBar = NULL;
static lv_obj_t *gFuelBar = NULL;
static lv_obj_t *gOilTempLabel = NULL;
static lv_obj_t *gCoolantLabel = NULL;
static lv_obj_t *gFuelLabel = NULL;
static lv_obj_t *gGearLabel = NULL;
static lv_obj_t *gTripLabel = NULL;
static lv_obj_t *gProxBar = NULL;
static lv_obj_t *gProxLabel = NULL;
static lv_obj_t *gProxIcon = NULL;

// Simulated vehicle data (internal metric, display imperial)
static float gSpeedKph = 0.0f;
static float gRpm = 850.0f;
static float gBoostPsi = -10.0f;
static float gOilTempC = 85.0f;
static float gCoolantTempC = 82.0f;
static float gFuelPercent = 72.0f;
static float gProxDistCm = 200.0f;  // Ultrasonic proximity in cm
static uint8_t gGear = 0;
static float gTripA = 142.7f;

static lv_timer_t *gUpdateTimer = NULL;

// Create styled arc gauge
static lv_obj_t* create_arc_gauge(lv_obj_t* parent, int16_t x, int16_t y, uint16_t size, 
                                   lv_color_t bg_color, lv_color_t fg_color, uint8_t width) {
  lv_obj_t *arc = lv_arc_create(parent);
  lv_obj_set_size(arc, size, size);
  lv_obj_set_pos(arc, x, y);
  lv_arc_set_rotation(arc, 135);
  lv_arc_set_bg_angles(arc, 0, 270);
  lv_arc_set_range(arc, 0, 100);
  lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
  lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, bg_color, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, fg_color, LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
  return arc;
}

// Create vertical bar gauge
static lv_obj_t* create_bar_gauge(lv_obj_t* parent, int16_t x, int16_t y, 
                                   uint16_t w, uint16_t h, lv_color_t color) {
  lv_obj_t *bar = lv_bar_create(parent);
  lv_obj_set_size(bar, w, h);
  lv_obj_set_pos(bar, x, y);
  lv_bar_set_range(bar, 0, 100);
  lv_obj_set_style_bg_color(bar, lv_color_hex(WRX_GAUGE_BG), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(bar, 4, LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar, color, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);
  return bar;
}

// Create info card
static lv_obj_t* create_card(lv_obj_t* parent, int16_t x, int16_t y, uint16_t w, uint16_t h) {
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, w, h);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_style_radius(card, 16, LV_PART_MAIN);
  lv_obj_set_style_bg_color(card, lv_color_hex(WRX_BG_CARD), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(card, lv_color_hex(0x2A2A35), LV_PART_MAIN);
  lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
  lv_obj_set_style_pad_all(card, 12, LV_PART_MAIN);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  return card;
}

static void simulate_driving(void) {
  static uint32_t phase = 0;
  phase++;
  float t = phase * 0.02f;
  
  gSpeedKph = 60.0f + 55.0f * sin(t * 0.3f) + 20.0f * sin(t * 0.7f);
  if (gSpeedKph < 0) gSpeedKph = 0;
  if (gSpeedKph > 180) gSpeedKph = 180;

  if (gSpeedKph < 20) {
    gGear = (gSpeedKph > 0) ? 1 : 0;
    gRpm = 850 + gSpeedKph * 150;
  } else if (gSpeedKph < 40) {
    gGear = 2;
    gRpm = 2000 + (gSpeedKph - 20) * 100;
  } else if (gSpeedKph < 60) {
    gGear = 3;
    gRpm = 2200 + (gSpeedKph - 40) * 90;
  } else if (gSpeedKph < 90) {
    gGear = 4;
    gRpm = 2400 + (gSpeedKph - 60) * 80;
  } else if (gSpeedKph < 120) {
    gGear = 5;
    gRpm = 2800 + (gSpeedKph - 90) * 70;
  } else {
    gGear = 6;
    gRpm = 3200 + (gSpeedKph - 120) * 50;
  }
  if (gRpm > 7000) gRpm = 7000;

  // Simulate ultrasonic proximity (20-400cm range, varies with speed)
  gProxDistCm = 80.0f + 120.0f * sin(t * 0.5f) + 50.0f * sin(t * 1.2f);
  if (gProxDistCm < 20) gProxDistCm = 20;
  if (gProxDistCm > 400) gProxDistCm = 400;

  float rpmFactor = (gRpm - 2500) / 4500.0f;
  gBoostPsi = -14.0f + rpmFactor * 36.0f + 4.0f * sin(t * 2.0f);
  if (gBoostPsi < -15) gBoostPsi = -15;
  if (gBoostPsi > 22) gBoostPsi = 22;

  gOilTempC = 85.0f + 8.0f * sin(t * 0.1f);
  gCoolantTempC = 82.0f + 5.0f * sin(t * 0.08f);
  gFuelPercent -= 0.001f;
  if (gFuelPercent < 10) gFuelPercent = 75;
  gTripA += gSpeedKph * 0.00001f;
}

static void update_timer_cb(lv_timer_t *timer) {
  simulate_driving();
  char buf[32];

  // ===== CENTER: HUGE SPEED (MPH) =====
  float speedMph = gSpeedKph * 0.621371f;
  snprintf(buf, sizeof(buf), "%d", (int)speedMph);
  lv_label_set_text(gSpeedLabel, buf);
  // Keep label centered as number width changes
  lv_obj_align(gSpeedLabel, LV_ALIGN_TOP_MID, 0, 90);

  // ===== LEFT: BATTERY CHARGE % (gBoostLabel) =====
  snprintf(buf, sizeof(buf), "%d", (int)gFuelPercent);
  lv_label_set_text(gBoostLabel, buf);

  // ===== LEFT: CABIN TEMP F (gOilTempLabel) =====
  float cabinF = 98.6f + 4.0f * sin(gSpeedKph * 0.01f);
  snprintf(buf, sizeof(buf), "%.1f", cabinF);
  lv_label_set_text(gOilTempLabel, buf);

  // ===== GEAR SELECTOR HIGHLIGHT =====
  // gGearLabel currently points to "P" - swap highlight between P/R/N/D based on gGear
  // For the simulation: 0=P, 7=R, others D
  if (gGearLabel != NULL) {
    if (gGear == 0) {
      lv_obj_set_style_text_color(gGearLabel, lv_color_hex(WRX_TEXT_PRIMARY), LV_PART_MAIN);
    } else {
      lv_obj_set_style_text_color(gGearLabel, lv_color_hex(WRX_TEXT_DIM), LV_PART_MAIN);
    }
  }

  // ===== PROXIMITY SENSOR =====
  float proxInches = gProxDistCm / 2.54f;
  int proxPct = 100 - (int)((gProxDistCm / 400.0f) * 100.0f);
  if (proxPct < 0) proxPct = 0;
  if (proxPct > 100) proxPct = 100;
  lv_bar_set_value(gProxBar, proxPct, LV_ANIM_OFF);

  if (gProxDistCm < 50) {
    lv_obj_set_style_bg_color(gProxBar, lv_color_hex(WRX_DANGER), LV_PART_INDICATOR);
    lv_obj_set_style_text_color(gProxLabel, lv_color_hex(WRX_DANGER), LV_PART_MAIN);
    lv_obj_set_style_text_color(gProxIcon, lv_color_hex(WRX_DANGER), LV_PART_MAIN);
  } else if (gProxDistCm < 100) {
    lv_obj_set_style_bg_color(gProxBar, lv_color_hex(WRX_WARNING), LV_PART_INDICATOR);
    lv_obj_set_style_text_color(gProxLabel, lv_color_hex(WRX_WARNING), LV_PART_MAIN);
    lv_obj_set_style_text_color(gProxIcon, lv_color_hex(WRX_WARNING), LV_PART_MAIN);
  } else {
    lv_obj_set_style_bg_color(gProxBar, lv_color_hex(WRX_ACCENT_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_text_color(gProxLabel, lv_color_hex(WRX_ACCENT_GREEN), LV_PART_MAIN);
    lv_obj_set_style_text_color(gProxIcon, lv_color_hex(WRX_ACCENT_TEAL), LV_PART_MAIN);
  }
  snprintf(buf, sizeof(buf), "%.0f in", proxInches);
  lv_label_set_text(gProxLabel, buf);

  // ===== RIGHT: AVG SPEED (gCoolantLabel) =====
  static float avgMph = 78.0f;
  avgMph = avgMph * 0.98f + speedMph * 0.02f;
  snprintf(buf, sizeof(buf), "%d mph", (int)avgMph);
  lv_label_set_text(gCoolantLabel, buf);

  // ===== RIGHT: AVG FUEL MPG (gFuelLabel) =====
  float mpg = 28.0f + 10.0f * sin(gSpeedKph * 0.02f);
  snprintf(buf, sizeof(buf), "%d mpg", (int)mpg);
  lv_label_set_text(gFuelLabel, buf);

  // ===== RIGHT: DISTANCE (gTripLabel) =====
  float tripMiles = gTripA * 0.621371f;
  snprintf(buf, sizeof(buf), "%d mi", (int)tripMiles);
  lv_label_set_text(gTripLabel, buf);
}

static void build_wrx_dashboard(void) {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(WRX_BG_DARK), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  // ========== TOP BAR: TIME | BRAND | DATE ==========
  lv_obj_t *timeLbl = lv_label_create(scr);
  lv_label_set_text(timeLbl, "19:30");
  lv_obj_set_style_text_color(timeLbl, lv_color_hex(WRX_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_font(timeLbl, &SF_Pro_22, LV_PART_MAIN);
  lv_obj_set_pos(timeLbl, 28, 18);

  lv_obj_t *brand = lv_label_create(scr);
  lv_label_set_text(brand, "SUBARU  WRX");
  lv_obj_set_style_text_color(brand, lv_color_hex(WRX_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_font(brand, &SF_Pro_16, LV_PART_MAIN);
  lv_obj_align(brand, LV_ALIGN_TOP_MID, 0, 22);

  lv_obj_t *dateLbl = lv_label_create(scr);
  lv_label_set_text(dateLbl, "04 / 21 / 26");
  lv_obj_set_style_text_color(dateLbl, lv_color_hex(WRX_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_font(dateLbl, &SF_Pro_22, LV_PART_MAIN);
  lv_obj_align(dateLbl, LV_ALIGN_TOP_RIGHT, -28, 18);

  // Thin divider line under top bar
  lv_obj_t *divTop = lv_obj_create(scr);
  lv_obj_set_size(divTop, SCREEN_W - 56, 1);
  lv_obj_set_pos(divTop, 28, 58);
  lv_obj_set_style_bg_color(divTop, lv_color_hex(WRX_TEXT_DIM), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(divTop, LV_OPA_40, LV_PART_MAIN);
  lv_obj_set_style_border_width(divTop, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(divTop, 0, LV_PART_MAIN);

  // ========== LEFT: BATTERY % + AUTO + BATTERY BARS ==========
  // Giant "97%" battery charge (repurpose gBoostLabel to track fuel/battery)
  gBoostLabel = lv_label_create(scr);
  lv_label_set_text(gBoostLabel, "97");
  lv_obj_set_style_text_color(gBoostLabel, lv_color_hex(WRX_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_font(gBoostLabel, &SF_Pro_64, LV_PART_MAIN);
  lv_obj_set_pos(gBoostLabel, 36, 110);

  lv_obj_t *pctSym = lv_label_create(scr);
  lv_label_set_text(pctSym, "%");
  lv_obj_set_style_text_color(pctSym, lv_color_hex(WRX_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_font(pctSym, &SF_Pro_40, LV_PART_MAIN);
  lv_obj_set_pos(pctSym, 176, 128);

  lv_obj_t *battLbl = lv_label_create(scr);
  lv_label_set_text(battLbl, "BATTERY CHARGE");
  lv_obj_set_style_text_color(battLbl, lv_color_hex(WRX_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_font(battLbl, &SF_Pro_16, LV_PART_MAIN);
  lv_obj_set_pos(battLbl, 40, 200);

  // Battery bar segments (7 segments: 3 green + 4 teal)
  for (int i = 0; i < 7; i++) {
    lv_obj_t *seg = lv_obj_create(scr);
    lv_obj_set_size(seg, 14, 30);
    lv_obj_set_pos(seg, 40 + i * 22, 240);
    lv_obj_set_style_radius(seg, 3, LV_PART_MAIN);
    lv_obj_set_style_border_width(seg, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(seg, 0, LV_PART_MAIN);
    lv_obj_clear_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
    if (i < 3) {
      lv_obj_set_style_bg_color(seg, lv_color_hex(WRX_ACCENT_GREEN), LV_PART_MAIN);
    } else {
      lv_obj_set_style_bg_color(seg, lv_color_hex(WRX_ACCENT_TEAL), LV_PART_MAIN);
    }
  }

  // AUTO climate indicator
  lv_obj_t *autoLbl = lv_label_create(scr);
  lv_label_set_text(autoLbl, "AUTO");
  lv_obj_set_style_text_color(autoLbl, lv_color_hex(WRX_ACCENT_GREEN), LV_PART_MAIN);
  lv_obj_set_style_text_font(autoLbl, &SF_Pro_16, LV_PART_MAIN);
  lv_obj_set_pos(autoLbl, 40, 300);

  lv_obj_t *climLbl = lv_label_create(scr);
  lv_label_set_text(climLbl, "CLIMATE");
  lv_obj_set_style_text_color(climLbl, lv_color_hex(WRX_TEXT_DIM), LV_PART_MAIN);
  lv_obj_set_style_text_font(climLbl, &SF_Pro_16, LV_PART_MAIN);
  lv_obj_set_pos(climLbl, 92, 300);

  // Temperature (gOilTempLabel repurposed for cabin temp)
  gOilTempLabel = lv_label_create(scr);
  lv_label_set_text(gOilTempLabel, "100.6");
  lv_obj_set_style_text_color(gOilTempLabel, lv_color_hex(WRX_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_font(gOilTempLabel, &SF_Pro_40, LV_PART_MAIN);
  lv_obj_set_pos(gOilTempLabel, 40, 340);

  lv_obj_t *tempUnit = lv_label_create(scr);
  lv_label_set_text(tempUnit, "F");
  lv_obj_set_style_text_color(tempUnit, lv_color_hex(WRX_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_font(tempUnit, &SF_Pro_22, LV_PART_MAIN);
  lv_obj_set_pos(tempUnit, 180, 360);

  // ========== CENTER: HUGE SPEED + MPH + SPEED LIMIT ==========
  // Huge speed number in teal
  gSpeedLabel = lv_label_create(scr);
  lv_label_set_text(gSpeedLabel, "0");
  lv_obj_set_style_text_color(gSpeedLabel, lv_color_hex(WRX_ACCENT_TEAL), LV_PART_MAIN);
  lv_obj_set_style_text_font(gSpeedLabel, &SF_Pro_64, LV_PART_MAIN);
  lv_obj_align(gSpeedLabel, LV_ALIGN_TOP_MID, 0, 90);

  // MPH unit in teal
  lv_obj_t *speedUnit = lv_label_create(scr);
  lv_label_set_text(speedUnit, "MPH");
  lv_obj_set_style_text_color(speedUnit, lv_color_hex(WRX_ACCENT_TEAL), LV_PART_MAIN);
  lv_obj_set_style_text_font(speedUnit, &SF_Pro_22, LV_PART_MAIN);
  lv_obj_align(speedUnit, LV_ALIGN_TOP_MID, 0, 200);

  // Speed limit sign - circle with red border, white fill, black "70"
  lv_obj_t *limitRing = lv_obj_create(scr);
  lv_obj_set_size(limitRing, 90, 90);
  lv_obj_align(limitRing, LV_ALIGN_TOP_MID, 0, 260);
  lv_obj_set_style_radius(limitRing, 45, LV_PART_MAIN);
  lv_obj_set_style_bg_color(limitRing, lv_color_hex(0xF0F0F0), LV_PART_MAIN);
  lv_obj_set_style_border_color(limitRing, lv_color_hex(WRX_ACCENT_RED), LV_PART_MAIN);
  lv_obj_set_style_border_width(limitRing, 5, LV_PART_MAIN);
  lv_obj_set_style_pad_all(limitRing, 0, LV_PART_MAIN);
  lv_obj_clear_flag(limitRing, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *limitNum = lv_label_create(limitRing);
  lv_label_set_text(limitNum, "70");
  lv_obj_set_style_text_color(limitNum, lv_color_hex(0x111111), LV_PART_MAIN);
  lv_obj_set_style_text_font(limitNum, &SF_Pro_40, LV_PART_MAIN);
  lv_obj_center(limitNum);

  // ========== PROXIMITY SENSOR (front, under speed limit) ==========
  gProxIcon = lv_label_create(scr);
  lv_label_set_text(gProxIcon, FA_CAR);
  lv_obj_set_style_text_color(gProxIcon, lv_color_hex(WRX_ACCENT_TEAL), LV_PART_MAIN);
  lv_obj_set_style_text_font(gProxIcon, &FA_Icons_20, LV_PART_MAIN);
  lv_obj_set_pos(gProxIcon, 392, 385);

  gProxBar = lv_bar_create(scr);
  lv_obj_set_size(gProxBar, 180, 8);
  lv_obj_set_pos(gProxBar, 422, 395);
  lv_obj_set_style_bg_color(gProxBar, lv_color_hex(WRX_GAUGE_BG), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(gProxBar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(gProxBar, lv_color_hex(WRX_ACCENT_GREEN), LV_PART_INDICATOR);
  lv_obj_set_style_radius(gProxBar, 4, LV_PART_MAIN);
  lv_obj_set_style_radius(gProxBar, 4, LV_PART_INDICATOR);
  lv_obj_set_style_border_width(gProxBar, 0, LV_PART_MAIN);
  lv_bar_set_range(gProxBar, 0, 100);
  lv_bar_set_value(gProxBar, 0, LV_ANIM_OFF);

  gProxLabel = lv_label_create(scr);
  lv_label_set_text(gProxLabel, "-- in");
  lv_obj_set_style_text_color(gProxLabel, lv_color_hex(WRX_ACCENT_GREEN), LV_PART_MAIN);
  lv_obj_set_style_text_font(gProxLabel, &SF_Pro_16, LV_PART_MAIN);
  lv_obj_set_pos(gProxLabel, 610, 386);

  lv_obj_t *proxCap = lv_label_create(scr);
  lv_label_set_text(proxCap, "FRONT PROXIMITY");
  lv_obj_set_style_text_color(proxCap, lv_color_hex(WRX_TEXT_DIM), LV_PART_MAIN);
  lv_obj_set_style_text_font(proxCap, &SF_Pro_16, LV_PART_MAIN);
  lv_obj_set_pos(proxCap, 422, 410);

  // ========== RIGHT: DISTANCE / AVG SPEED / AVG FUEL ==========
  // Distance
  gTripLabel = lv_label_create(scr);
  lv_label_set_text(gTripLabel, "188 mi");
  lv_obj_set_style_text_color(gTripLabel, lv_color_hex(WRX_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_font(gTripLabel, &SF_Pro_40, LV_PART_MAIN);
  lv_obj_set_pos(gTripLabel, 760, 100);

  lv_obj_t *distCap = lv_label_create(scr);
  lv_label_set_text(distCap, "DISTANCE");
  lv_obj_set_style_text_color(distCap, lv_color_hex(WRX_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_font(distCap, &SF_Pro_16, LV_PART_MAIN);
  lv_obj_set_pos(distCap, 762, 150);

  // Avg Speed (repurpose gCoolantLabel)
  gCoolantLabel = lv_label_create(scr);
  lv_label_set_text(gCoolantLabel, "78 mph");
  lv_obj_set_style_text_color(gCoolantLabel, lv_color_hex(WRX_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_font(gCoolantLabel, &SF_Pro_40, LV_PART_MAIN);
  lv_obj_set_pos(gCoolantLabel, 760, 215);

  lv_obj_t *avgSpCap = lv_label_create(scr);
  lv_label_set_text(avgSpCap, "AVG. SPEED");
  lv_obj_set_style_text_color(avgSpCap, lv_color_hex(WRX_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_font(avgSpCap, &SF_Pro_16, LV_PART_MAIN);
  lv_obj_set_pos(avgSpCap, 762, 265);

  // Avg Fuel Usage (repurpose gFuelLabel)
  gFuelLabel = lv_label_create(scr);
  lv_label_set_text(gFuelLabel, "34 mpg");
  lv_obj_set_style_text_color(gFuelLabel, lv_color_hex(WRX_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_font(gFuelLabel, &SF_Pro_40, LV_PART_MAIN);
  lv_obj_set_pos(gFuelLabel, 760, 330);

  lv_obj_t *avgFuelCap = lv_label_create(scr);
  lv_label_set_text(avgFuelCap, "AVG. FUEL ECONOMY");
  lv_obj_set_style_text_color(avgFuelCap, lv_color_hex(WRX_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_font(avgFuelCap, &SF_Pro_16, LV_PART_MAIN);
  lv_obj_set_pos(avgFuelCap, 762, 380);

  // ========== BOTTOM BAR: READY + GEAR SELECTOR ==========
  // Divider above bottom bar
  lv_obj_t *divBot = lv_obj_create(scr);
  lv_obj_set_size(divBot, SCREEN_W - 56, 1);
  lv_obj_set_pos(divBot, 28, 535);
  lv_obj_set_style_bg_color(divBot, lv_color_hex(WRX_TEXT_DIM), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(divBot, LV_OPA_40, LV_PART_MAIN);
  lv_obj_set_style_border_width(divBot, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(divBot, 0, LV_PART_MAIN);

  // READY status (left)
  lv_obj_t *readyDot = lv_obj_create(scr);
  lv_obj_set_size(readyDot, 10, 10);
  lv_obj_set_pos(readyDot, 40, 566);
  lv_obj_set_style_radius(readyDot, 5, LV_PART_MAIN);
  lv_obj_set_style_bg_color(readyDot, lv_color_hex(WRX_ACCENT_GREEN), LV_PART_MAIN);
  lv_obj_set_style_border_width(readyDot, 0, LV_PART_MAIN);

  lv_obj_t *readyLbl = lv_label_create(scr);
  lv_label_set_text(readyLbl, "READY");
  lv_obj_set_style_text_color(readyLbl, lv_color_hex(WRX_ACCENT_GREEN), LV_PART_MAIN);
  lv_obj_set_style_text_font(readyLbl, &SF_Pro_22, LV_PART_MAIN);
  lv_obj_set_pos(readyLbl, 60, 558);

  // Current speed small (center bottom)
  lv_obj_t *smallSpeedLbl = lv_label_create(scr);
  lv_label_set_text(smallSpeedLbl, "78");
  lv_obj_set_style_text_color(smallSpeedLbl, lv_color_hex(WRX_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_font(smallSpeedLbl, &SF_Pro_22, LV_PART_MAIN);
  lv_obj_set_pos(smallSpeedLbl, 470, 558);

  lv_obj_t *smallMphLbl = lv_label_create(scr);
  lv_label_set_text(smallMphLbl, "mph");
  lv_obj_set_style_text_color(smallMphLbl, lv_color_hex(WRX_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_font(smallMphLbl, &SF_Pro_22, LV_PART_MAIN);
  lv_obj_set_pos(smallMphLbl, 510, 558);

  // Gear selector P R N D (right)
  const char* gearLetters[] = {"P", "R", "N", "D"};
  for (int i = 0; i < 4; i++) {
    lv_obj_t *g = lv_label_create(scr);
    lv_label_set_text(g, gearLetters[i]);
    lv_obj_set_style_text_font(g, &SF_Pro_22, LV_PART_MAIN);
    lv_obj_set_pos(g, 840 + i * 40, 558);
    // Start with P highlighted, others dim
    if (i == 0) {
      lv_obj_set_style_text_color(g, lv_color_hex(WRX_TEXT_PRIMARY), LV_PART_MAIN);
      gGearLabel = g;  // Track P as dynamic gear indicator
    } else {
      lv_obj_set_style_text_color(g, lv_color_hex(WRX_TEXT_DIM), LV_PART_MAIN);
    }
  }

  // Hidden/unused widgets (keep pointers valid for update callback)
  gSpeedArc = NULL;
  gTachArc = NULL;
  gTachLabel = NULL;
  gBoostArc = NULL;
  gOilTempBar = NULL;
  gCoolantBar = NULL;
  gFuelBar = NULL;

  // Update timer at 4ms (~250Hz logic rate, LVGL caps to display refresh)
  gUpdateTimer = lv_timer_create(update_timer_cb, 4, NULL);
}

void setup() {
  static esp_lcd_panel_handle_t panel_handle = NULL;
  static esp_lcd_touch_handle_t tp_handle = NULL;

  Serial.begin(115200);
  delay(300);
  Serial.println("WRX dash boot");

  DEV_I2C_Init();
  IO_EXTENSION_Init();

  // Initialize RGB LCD with proper vsync handling
  panel_handle = waveshare_esp32_s3_rgb_lcd_init();
  if (panel_handle == NULL) {
    Serial.println("LCD init failed");
    return;
  }

  // Turn on backlight
  wavesahre_rgb_lcd_bl_on();

  // This panel variant is non-touch: keep touch driver disabled.
  tp_handle = NULL;

  // Initialize LVGL with proper port (handles vsync, buffers, task)
  esp_err_t lvret = lvgl_port_init(panel_handle, tp_handle);
  if (lvret != ESP_OK) {
    Serial.printf("lvgl_port_init failed: %d\n", (int)lvret);
    return;
  }

  Serial.println("Building WRX dashboard UI");

  // Lock LVGL mutex and build UI
  if (lvgl_port_lock(-1)) {
    build_wrx_dashboard();
    lvgl_port_unlock();
  }
  Serial.println("WRX dashboard ready");
}

void loop() {
  // LVGL is handled by the port task, nothing needed here
  delay(100);
}
