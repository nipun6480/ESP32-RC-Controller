# 🎮 ESP32 RC Controller (Client)

A custom-built remote controller featuring an integrated screen to view the live camera feed from the RC car while controlling its movement.

## 🚀 Features
* **TFT Display:** Real-time MJPEG video decoding and display.
* **Joystick Control:** Precise analog input for speed and direction.
* **Wireless Link:** Communicates via UDP/HTTP requests over Wi-Fi.

## 🔌 Hardware Setup
* **Microcontroller:** ESP32 (30-pin DevKit)
* **Display:** [Insert your screen model here, e.g., ST7789]
* **Input:** Dual-axis Analog Joystick + Buttons

### Pin Mapping (Example)
| Component | ESP32 Pin |
| :--- | :--- |
| Screen (SDA/SCL) | GPIO 21 / GPIO 22 |
| Joystick X-Axis | GPIO 34 |
| Joystick Y-Axis | GPIO 35 |

## 🛠️ Logic Flow
1. **Initialize Wi-Fi:** Connects to the Car's Access Point.
2. **Fetch Stream:** Requests the video stream from the Car's IP address.
3. **Transmit:** Maps joystick movements to movement strings (e.g., "FORWARD") and sends them via # ESP32 RC Car Controller 🎮

This project is the wireless controller for th
