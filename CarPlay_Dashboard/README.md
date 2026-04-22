# Apple CarPlay Studio Dashboard — Waveshare ESP32-S3 7B + ToF 3D Mapping

A complete automotive-grade instrument cluster with an Apple CarPlay-inspired
interface, now targeting the **Waveshare ESP32-S3-Touch-LCD-7B** (7" 1024×600
IPS with GT911 capacitive touch) and equipped with a **VL53L5CX 8×8 time-of-
flight sensor** for live 3D mapping of the road ahead while driving.

## 🚗 Features

### **Dashboard Components**
- **Speedometer**: Real-time speed display with animated needle (0-200 km/h)
- **Tachometer**: RPM gauge with smooth animation (0-6000 RPM)
- **Fuel Gauge**: Digital fuel level indicator with percentage display
- **Temperature Gauge**: Engine temperature monitoring (70-110°C)
- **Navigation Display**: Turn-by-turn navigation with distance indicators
- **Live Map Simulation**: CarPlay-style map canvas with animated route progress
- **Media Controls**: Music player interface with play/pause controls
- **Climate Control**: Temperature and HVAC status display
- **Status Indicators**: Signal lights, warnings, and system status
- **ToF 3D Scan View**: Rolling top-down point cloud of the road ahead,
  raw 8×8 heatmap, and a persistent "closest obstacle" mini-card pinned in
  the info panel regardless of view

### **ToF 3D Mapping (new)**
- **Sensor**: ST VL53L5CX mounted forward-facing (45°×45° FOV, 4 m range)
- **Ranging**: 8×8 zones @ 15 Hz, continuous mode
- **Ego-motion fusion**: Each frame is projected into vehicle coordinates
  and dead-reckoned with the dashboard's speed + compass heading to build a
  coherent **120×80 cell occupancy grid at 10 cm resolution** (12 m × 8 m)
- **Classification**: Ground / obstacle / imminent-hazard per cell, with
  confidence decay so stale returns fade out in ~4 s
- **HUD**: Full-screen pulsing red banner + border when a hazard is detected
  inside 1.5 m, regardless of whether the cluster view or mapping view is up

### **Apple CarPlay Theme**
- **Dark Mode Interface**: Optimized for nighttime driving
- **Large, Readable Fonts**: Safety-focused typography
- **Smooth Animations**: Gauge needle animations and transitions
- **Fast Refresh Navigation Loop**: 100ms UI updates to keep map and telemetry responsive
- **Modern UI Elements**: Rounded corners, proper spacing
- **High Contrast**: Excellent visibility in various lighting conditions

### **Technical Features**
- **1024x600 Resolution**: Optimized for automotive displays
- **Real-time Updates**: 100ms refresh rate for smooth performance
- **Configurable Settings**: Easy customization through config file
- **Mock Data Simulation**: Built-in sensor simulation for testing
- **Comprehensive Debugging**: Serial monitoring and error reporting

## 📁 Project Structure

```
CarPlay_Dashboard/
├── CarPlay_Dashboard.ino         # Main sketch — init, sensor polling, render loop
├── carplay_theme.h                # Layout + color palette (7B 1024×600 + ToF)
├── carplay_ultra.h / .cpp         # Cluster + info panel + mapping view
├── gt911_touch.h / .cpp           # GT911 capacitive touch driver (7B)
├── tof_sensor.h / .cpp            # VL53L5CX 8×8 ToF wrapper (real + mock)
├── tof_mapping.h / .cpp           # 3D point-cloud → top-down occupancy grid
├── rgb_lcd_port.h / .cpp          # RGB LCD driver (Waveshare 7B timings)
├── gui_paint.h / .cpp             # Graphics primitives
├── image.h                        # Image resources
├── i2c.h / .cpp                   # I2C bus wrapper (shared by LCD/Touch/ToF)
├── io_extension.h / .cpp          # TCA9554 IO expander control
├── fonts.h + font*.cpp            # Font definitions
└── Debug.h                        # Debug utilities
```

## 🛠️ Hardware Requirements

### **Waveshare ESP32-S3-Touch-LCD-7B (primary target)**
- ESP32-S3R8 with 8 MB PSRAM, 16 MB flash
- 7" 1024×600 IPS panel, 16-bit RGB, 18 MHz pclk
- GT911 capacitive touch on shared I2C (addr 0x5D)
- TCA9554 IO expander drives LCD_RST / BL_EN / TP_RST / TP_INT
- Grove + JST-SH headers break out 5 V, 3.3 V, SDA/SCL for add-on sensors

### **VL53L5CX Time-of-Flight Sensor**
- ST VL53L5CX breakout (SparkFun SEN-18642, Pololu 3417, or Waveshare equivalent)
- 8×8 multi-zone ranging, 45°×45° FOV, 4 m range, 15 Hz at 8×8 resolution
- Connects to the same I2C bus (SDA=GPIO 8, SCL=GPIO 9), default addr 0x52
- Powered from the board's 3.3 V rail (current draw <40 mA peak)

### **Power & mounting**
- 5 V / 2 A supply (12 V → 5 V buck when installed in a vehicle)
- ToF sensor mounted behind the grille or windshield, pitched ~8° down,
  roughly 0.7 m above road (`TOF_MOUNT_*` constants in `tof_mapping.h` can
  be retuned for other mount geometries)

## 📋 Installation & Setup

### **1. Arduino IDE Setup**
1. Install ESP32 core v3.0+ (needed for `driver/i2c_master.h`)
2. Select board: **ESP32S3 Dev Module**, PSRAM: **OPI PSRAM**, Flash Size: **16MB**
3. Install Arduino libraries from Library Manager:
   - **SparkFun VL53L5CX Arduino Library** (uploads the ~86 KB ToF firmware)
   - `Wire` (built-in)
4. If the SparkFun library isn't installed the ToF driver falls back to
   deterministic mock data so the rest of the dashboard still runs.

### **2. Hardware Connections (Waveshare 7B + VL53L5CX)**
```
ESP32-S3    →    7B on-board peripherals
GPIO 3      →    VSYNC
GPIO 46     →    HSYNC
GPIO 5      →    DE (Data Enable)
GPIO 7      →    PCLK (Pixel Clock)
GPIO 14-40  →    RGB data lines (16-bit)
GPIO 8      →    I2C SDA (shared by TCA9554, GT911, VL53L5CX)
GPIO 9      →    I2C SCL

VL53L5CX breakout
3V3         →    VIN
GND         →    GND
GPIO 8      →    SDA
GPIO 9      →    SCL
(LPn, INT, I2C_RST can stay unconnected for the default config;
 the SparkFun lib will auto-wake the chip via the soft-reset register.)
```

View switching: **tap anywhere on the left half** of the 7B touch panel to
cycle between the instrument cluster and the ToF 3D-scan view. The right
info panel (trip / calendar / weather / music / nav) stays present in both.

### **3. Configuration**
Edit `CarPlay_Config.h` to match your hardware:
- Display dimensions
- GPIO pin assignments
- Color preferences
- Gauge ranges
- Update intervals

### **4. Upload & Test**
1. Connect ESP32-S3 to computer via USB
2. Select correct board and port in Arduino IDE
3. Upload `CarPlay_Dashboard.ino`
4. Open Serial Monitor at 115200 baud
5. Verify initialization and display operation

## 🎨 Customization

### **Display Themes**
Modify colors in `CarPlay_Config.h`:
```cpp
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_BLUE        0x1C7F
// ... add your custom colors
```

### **Gauge Ranges**
Adjust measurement ranges:
```cpp
#define SPEEDOMETER_MAX_SPEED      200     // km/h
#define TACHOMETER_MAX_RPM         6000    // RPM
#define TEMP_GAUGE_MIN             70      // Celsius
#define TEMP_GAUGE_MAX             110     // Celsius
```

### **Layout Customization**
Modify layout constants:
```cpp
#define TOP_BAR_HEIGHT       80
#define BOTTOM_BAR_HEIGHT    100
#define GAUGE_AREA_WIDTH     300
```

### **Animation Settings**
Control smooth animations:
```cpp
#define NEEDLE_ANIMATION_SPEED     5       // Lower = faster
#define GAUGE_UPDATE_DELAY         10      // milliseconds
```

## 🚀 Advanced Features

### **Real Sensor Integration**
Replace mock data with real sensors:
```cpp
// In updateCarData() function
currentSpeed = getSpeedFromSensor();
currentRPM = getRPMFromSensor();
fuelLevel = getFuelLevel();
engineTemp = getEngineTemperature();
```

### **Navigation Integration**
Add GPS and mapping:
```cpp
void updateNavigation() {
    // GPS coordinate processing
    // Turn-by-turn calculation
    // Distance estimation
}
```

### **Connectivity Features**
Add smartphone integration:
```cpp
void updatePhoneStatus() {
    // Bluetooth connectivity
    // Phone call status
    // Message notifications
}
```

## 🔧 Debugging & Troubleshooting

### **Serial Monitoring**
Monitor system status at 115200 baud:
```
Apple CarPlay Studio Dashboard Starting...
Initializing hardware...
Creating dashboard buffer...
Paint settings initialized
Dashboard initialization complete!
```

### **Common Issues**

**Display Not Working:**
- Check power connections
- Verify GPIO pin assignments
- Adjust timing parameters in LCD config

**Poor Performance:**
- Reduce animation speed
- Lower update frequency
- Check for memory leaks

**Color Issues:**
- Verify RGB color format (RGB565)
- Check backlight control
- Adjust contrast settings

### **Performance Optimization**
- Use double buffering for smooth animations
- Optimize drawing routines
- Implement efficient data structures
- Minimize memory allocations

## 📱 Mobile Integration

### **Bluetooth Connectivity**
Add smartphone features:
- Phone call display
- Text message notifications
- Contact list integration
- Calendar reminders

### **WiFi Features**
Connect to internet:
- Real-time traffic data
- Weather information
- Online radio streaming
- Software updates

### **Voice Control**
Integrate speech recognition:
- Hands-free operation
- Voice commands for climate
- Navigation by voice
- Media control

## 🎯 Future Enhancements

### **Planned Features**
- [ ] Touch screen support
- [ ] Multi-language support
- [ ] Theme customization
- [ ] Widget system
- [ ] Software update mechanism
- [ ] OBD-II integration
- [ ] Backup camera display
- [ ] Voice assistant integration

### **Hardware Upgrades**
- [ ] Higher resolution displays
- [ ] Multiple display support
- [ ] Ambient light sensing
- [ ] Gesture recognition
- [ ] Haptic feedback

## 📄 License

This project is open source and available under the MIT License. Feel free to modify and distribute according to your needs.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or report issues.

### **How to Contribute**
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For support and questions:
- Check the troubleshooting section
- Review configuration settings
- Verify hardware connections
- Monitor serial output for errors

---

**Enjoy your Apple CarPlay Studio Dashboard! 🚗✨**