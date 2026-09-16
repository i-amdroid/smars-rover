# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is the **remote control** component of a SMARS ESP32 robot system. It reads joystick and button inputs from hardware connected to an ESP32-S3 board and transmits control data wirelessly using ESP-NOW protocol to a receiver board that controls the robot.

## Build System

This project uses **PlatformIO** for building and uploading firmware.

### Build Commands

```bash
# Build for ESP32-S3 DevKit
pio run -e esp32-s3-devkitc-1

# Build for Seeed Xiao ESP32-S3
pio run -e seeed_xiao_esp32s3

# Upload firmware
pio run -e esp32-s3-devkitc-1 -t upload
pio run -e seeed_xiao_esp32s3 -t upload

# Upload and monitor serial output
pio run -e esp32-s3-devkitc-1 -t upload -t monitor
pio run -e seeed_xiao_esp32s3 -t upload -t monitor

# Monitor serial output only
pio device monitor
```

## Hardware Configuration

### Supported Boards

Two ESP32-S3 board configurations are available:

1. **esp32-s3-devkitc-1** (N16R8V variant with PSRAM)
   - MAC: EC:DA:3B:51:19:0C
   - 16MB flash with custom partition table

2. **seeed_xiao_esp32s3**
   - MAC: 98:3D:AE:60:84:C0

### Pin Configuration

Pin assignments are **commented/uncommented in main.cpp** depending on which board is being used:

- **Xiao ESP32-S3**: Lines 12-20 (currently active)
- **ESP32-S3 Uno**: Lines 23-31 (commented out)

When switching boards, you must manually comment/uncomment the appropriate pin definitions.

### Joystick Calibration Values

The code contains **three sets of calibration constants** in main.cpp:

1. Xiao ESP32-S3 without battery connector (lines 47-52)
2. **Xiao ESP32-S3 with battery connector (lines 55-60) - currently active**
3. ESP32-S3 uno (lines 62-67)

Each hardware setup has different `CENTER_X` and `CENTER_Y` values due to analog input variations. Comment/uncomment the appropriate section when changing hardware.

## Architecture

### ESP-NOW Communication

- **Protocol**: ESP-NOW (peer-to-peer WiFi communication without router)
- **Library**: ESPNowW v1.0.2 (wrapper for ESP-NOW functionality)
- **Data structure**: `JoystickData` struct (15 bytes total)
  - X/Y joystick axes: `int16_t` (-1000 to 1000)
  - 7 button states: `uint8_t` (0 or 1)
- **Receiver MAC address**: Hardcoded at line 6 (must match receiver device)
- **Send frequency**: 100ms intervals (line 204)

### Data Processing Pipeline

1. **Raw ADC read**: 12-bit values (0-4095) from analog joystick pins
2. **Asymmetric centering**: Maps raw values around measured center point to -1000/+1000 range
3. **Deadzone filtering**: Values within ±15 of center are zeroed to reduce jitter
4. **Button reading**: Digital pins with INPUT_PULLUP (inverted: LOW = pressed)
5. **ESP-NOW transmission**: Entire struct sent as byte array

### Special Features

- **Runtime calibration**: Press joystick button to recalibrate center position (5-second sampling)
- **Deep sleep**: Press F button to enter low-power mode, wake via same button (RTC wakeup on GPIO)

## Key Implementation Details

### Joystick Mapping (lines 153-171)

The joystick uses **asymmetric scaling** because raw center values are not at 2047 (the theoretical midpoint of 0-4095). Each axis is mapped separately:
- Below center: `map(raw, MIN_VALUE, CENTER, -1000, 0)`
- Above center: `map(raw, CENTER, MAX_VALUE, 0, 1000)`

This ensures full range utilization despite hardware variations.

### Button Handling

- **INPUT_PULLUP mode**: Buttons connect GPIO to GND when pressed
- **Values inverted** before sending (line 190-196): `!digitalRead(pin)`
- **Debouncing**: 50ms delays in special button handlers (calibration, sleep)

## Dependencies

- **Arduino framework** (ESP32 core)
- **WiFi library** (ESP32 built-in)
- **ESPNowW** (regenbogencode/ESPNowW@^1.0.2)

## Common Issues

When switching between board configurations:
1. Update receiver MAC address (line 6)
2. Uncomment correct pin definitions (lines 12-31)
3. Uncomment correct calibration constants (lines 46-67)
4. Update `platformio.ini` default environment if needed
5. Verify sleep button GPIO supports RTC wakeup (GPIO4 for Xiao, GPIO14 for Uno)
