# 🌱 Smart Irrigation System using STM32F401CCU6

A precision-agriculture project that combines **STM32 microcontroller control**, **environmental sensors**, and **Wi-Fi voice-command connectivity** to automate irrigation with real-time monitoring.

---

## 📖 Table of Contents
1. [Overview](#overview)  
2. [Features](#features)  
3. [System Architecture](#system-architecture)  
4. [Hardware Components](#hardware-components)  
5. [Key Technical Concepts](#key-technical-concepts)  
6. [Software & Development Flow](#software--development-flow)  
7. [Usage Instructions](#usage-instructions)  
8. [Challenges & Solutions](#challenges--solutions)  

---

## Overview
This project implements an **Embedded Smart Irrigation System** using the **STM32F401CCU6** (Cortex-M4) microcontroller.  
It ensures efficient water usage and allows farmers to monitor and control irrigation **wirelessly**—including **voice-to-text commands via a mobile device**.

---

## Features
- **Wi-Fi Connectivity**: ESP8266 module links STM32 to a mobile phone over a local network.  
- **Voice-to-Text Control**: Spoken commands (e.g., “start pump”) are converted to text on the phone and sent to STM32.  
- **Automated Sensor Monitoring**:  
  - Soil moisture  
  - Temperature & humidity  
  - Water level  
- **Real-Time Display**: LCD shows live sensor readings and irrigation status.  
- **User Interaction**: Push button and LED indicators for manual overrides and visual feedback.  

---

## System Architecture
```text
[Mobile App / Voice Input]
          |
   WiFi Module (ESP8266)
          |
   STM32F401CCU6 ── Sensors (Soil, Temp/Humidity, Water Level)
          |
       LCD + LEDs + Push Button
```

1. **Mobile**: Captures voice → converts to text → sends command via Wi-Fi.  
2. **STM32**: Receives command over UART, reads sensor values, and activates irrigation.  
3. **Feedback**: Status shown on LCD and LEDs.

---

## Hardware Components
| Item | Qty | Purpose |
|------|-----|--------|
| STM32F401 Black Pill | 1 | Main microcontroller |
| ST-Link Debugger | 1 | Programming/debugging |
| LCD (16×2 / 20×4) | 1 | Data & status display |
| Breadboard | 1 | Prototyping connections |
| LEDs | 2+ | Status indicators |
| Push Button | 1 | Manual input |
| Soil Moisture Sensor | 1+ | Soil condition sensing |
| Temperature & Humidity Sensor | 1 | Environment monitoring |
| Water Level Sensor | 1 | Reservoir monitoring |
| Wi-Fi Module (ESP8266) | 1 | Wireless communication |
| Jumper Cables | As required | Circuit connections |

---

## Key Technical Concepts
### Communication Interfaces
- **UART**: Handles STM32 ↔ Wi-Fi module communication for command/data transfer.  
- **I²C/SPI**: Optional for LCD or digital sensors.

### ADC & Digital Interfaces
- Soil moisture and water level sensors feed analog values to STM32 ADC channels.  
- Digital inputs for push button and certain sensors.

### NVIC Interrupt Handling
- Real-time response to button presses and sensor thresholds.  
- Ensures low-latency irrigation control and reliable Wi-Fi data handling.

---

## Software & Development Flow
- **IDE**: Keil uVision
- **Language**: C 
- **Wi-Fi Communication**: AT commands over UART  
- **Mobile Integration**: Custom app UI for voice-to-text and HTTP requests.

---

## Usage Instructions
1. Assemble all hardware as per the schematic.  
2. Flash STM32 with firmware from `/Firmware`.  
3. Power the Wi-Fi module and connect the mobile device to its network.  
4. Open the mobile app → speak a command → observe irrigation action on LCD/LEDs.  
5. Monitor live sensor data on the LCD.

---

## Challenges & Solutions
- **Reliable Voice-to-Text Integration**  
  *Solution*: Used mobile-side voice processing to offload heavy computation, sending lightweight text commands.  
- **Stable Wi-Fi Connectivity**  
  *Solution*: Implemented periodic heartbeat checks and UART error handling.  
- **Asynchronous Events**  
  *Solution*: Leveraged NVIC interrupts for instant response to button presses and critical sensor alerts.

---
