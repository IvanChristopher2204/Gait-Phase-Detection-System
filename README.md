# Gait Phase Detection System

A wearable, real-time gait analysis prototype using dual MPU6050 IMUs, an ESP32-S3 microcontroller, and a DS3231 RTC — designed to detect gait phases and calculate biomechanical/spatiotemporal parameters in real time.

📄 **Firmware:** [`code.cpp`](./code.cpp)

---

## 🧠 Problem Statement

Gait (walking pattern) analysis helps monitor patient movement and recovery. Current gait analysis systems — typically based on motion-capture cameras and force plates — are expensive, bulky, and mostly limited to hospitals and research laboratories.

**Need for the Project:** A low-cost, wearable system that can:
- Monitor walking patterns in real time
- Accurately detect gait phases
- Be used anywhere — homes, clinics, or hospitals

**Objective:** Make gait monitoring affordable, portable, and easily accessible.

## 💡 Proposed Solution

Two MPU6050 inertial measurement units (IMUs) are mounted on the leg — one on the **thigh** and one on the **shank** — continuously measuring acceleration and angular velocity. An **ESP32-S3**-class microcontroller processes this sensor data in real time to classify gait phases and compute movement parameters.

## ⚙️ How It Works (Firmware Overview)

The logic in [`code.cpp`](./code.cpp) runs a continuous ~50 Hz loop that:

1. **Reads raw motion data** from both IMUs via `getMotion6()`.
2. **Fuses accelerometer + gyroscope data** using a complementary filter (98% gyro integration + 2% accelerometer correction) to estimate thigh angle, shank angle, and knee angle (`shankAngle − thighAngle`).
3. **Smooths angular velocity** with an exponential low-pass filter to reduce sensor noise.
4. **Detects gait events** by tracking three consecutive angular-velocity samples to find positive peaks (**Toe-Off**) and negative peaks (**Heel-Strike**), using a 5°/s threshold and a 150 ms debounce lock to avoid false triggers.
5. **Classifies the current gait phase** as `HEEL_STRIKE`, `TOE_OFF`, `SWING`, or `STANCE`.
6. **Calculates gait parameters** — step time, stride time, cadence, and estimated step/stride length (based on an assumed walking speed).
7. **Timestamps every reading** using the DS3231 RTC and streams results over Serial, plus a compact CSV line for live plotting in the Arduino Serial Plotter.

## 🔩 Hardware Components

| Component | Details |
|---|---|
| **MPU6050 IMU (Thigh)** | I²C address `0x68`, mounted on the thigh segment |
| **MPU6050 IMU (Shank)** | I²C address `0x69`, mounted on the shank segment — shares the same I²C bus (`Wire`) with the thigh IMU on pins `D4` (SDA) / `D5` (SCL) |
| **DS3231 RTC Module** | Real-time clock for event timestamping — runs on a separate I²C bus (`TwoWire(1)`) on pins `D6` (SDA) / `D7` (SCL) |
| **ESP32-S3 (or compatible MCU)** | Runs sensor fusion, peak detection, and phase classification; supports dual hardware I²C buses |

> **Note:** This version of the firmware does not yet write to an SD card — data is streamed via Serial only. SD-card logging can be added as a future enhancement.

## 📚 Libraries Used

| Library | Purpose |
|---|---|
| [`Wire.h`](https://www.arduino.cc/en/reference/wire) | I²C communication (built-in) |
| [`MPU6050`](https://github.com/jrowberg/i2cdevlib) | IMU driver — raw accelerometer/gyroscope reads and connection testing |
| [`RTClib`](https://github.com/adafruit/RTClib) | DS3231 RTC driver — timekeeping and timestamp retrieval |

## ✨ Features

- Dual-IMU orientation estimation via complementary filter
- Low-pass filtered angular velocity for stable peak detection
- Real-time heel-strike / toe-off event detection with debounce logic
- Four-state gait phase classification (`STANCE`, `SWING`, `HEEL_STRIKE`, `TOE_OFF`)
- Step time, stride time, and cadence calculation
- Estimated step/stride length from assumed walking speed
- RTC-stamped Serial output, human-readable and plotter-friendly
- ~50 Hz real-time sampling loop

## 📊 Output Parameters

| Parameter | Description |
|---|---|
| Timestamp | RTC-derived `HH:MM:SS` |
| Thigh Angle | Estimated thigh segment angle (°) |
| Shank Angle | Estimated shank segment angle (°) |
| Knee Angle | Derived joint angle: shank − thigh (°) |
| Omega (Shank angular velocity) | Filtered gyro rate used for event detection (°/s) |
| Step Time | Time between successive toe-off events (s) |
| Step Length | Estimated distance per step (m) |
| Stride Time | Time between successive heel-strike events (s) |
| Stride Length | Estimated distance per stride (m) |
| Cadence | Steps per minute |
| Gait Phase | Current classified state |
| Gait Marker (plotter line) | `shankOmega, kneeAngle, gaitMarker` — spikes to -100 (heel-strike) or 100 (toe-off) |

## 🎯 Target Users & Use Cases

**Users:** Hospitals, physiotherapists, orthopedic & neurology specialists, rehabilitation centers, elderly individuals, stroke & Parkinson's patients

**Usage Areas:** Hospitals, rehabilitation centers, home monitoring, sports training facilities

**Benefits:** Early detection of walking abnormalities, continuous gait monitoring, portable/wearable form factor, faster rehabilitation support, reduced healthcare costs

## 🚀 Future Work

- SD card CSV logging (SPI-based, timestamped)
- Dashboard integration (Python or web-based) for visualizing trends and flagging fall risk
- Composite fall-risk scoring from gait variability + asymmetry
- Machine learning-based phase classification for improved accuracy

## 📝 Conclusion

This system uses two MPU6050 sensors and an ESP32-S3 to detect gait phases in real time, offering a low-cost, portable, wearable alternative to traditional lab-based gait analysis.

---

## 🔧 Getting Started

1. Wire the MPU6050s (thigh: `0x68`, shank: `0x69`) to the I²C bus on `D4`/`D5`.
2. Wire the DS3231 RTC to the second I²C bus on `D6`/`D7`.
3. Install the required libraries in the Arduino IDE: `MPU6050`, `RTClib`.
4. Flash [`code.cpp`](./code.cpp) to your ESP32-S3 board.
5. Open the Serial Monitor (115200 baud) to view live gait data, or the Serial Plotter to visualize event markers.
