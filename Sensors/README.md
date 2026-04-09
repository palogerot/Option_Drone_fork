# Option_Drone
# 🛸 Ultrasonic Sensor Module for Drone (STM32)

## 📌 Overview

This project implements an ultrasonic distance measurement module using an **HC-SR04 sensor** on an **STM32L476RG microcontroller**.

The module is designed to be integrated into a drone system for:

* obstacle detection
* altitude estimation
* landing assistance

The implementation focuses on precise timing measurement using GPIO and external interrupts.

---

## ⚙️ Hardware Configuration

### 🔌 Components

* Microcontroller: STM32L476RG
* Ultrasonic sensor: HC-SR04

### 📍 Pin Mapping

| Sensor Pin | STM32 Pin | Configuration                              |
| ---------- | --------- | ------------------------------------------ |
| VCC        | 5V        | Power supply                               |
| GND        | GND       | Ground                                     |
| TRIG       | PA5       | GPIO Output                                |
| ECHO       | PA6       | EXTI (interrupt on rising & falling edges) |

⚠️ **Important:**
The ECHO pin outputs **5V**, while STM32 GPIOs are **3.3V tolerant**.
→ A **voltage divider is required** between ECHO and PA6.

---

## 🚀 Working Principle

The HC-SR04 sensor measures distance using ultrasonic waves:

1. A **10 µs pulse** is sent on the TRIG pin
2. The sensor emits an ultrasonic burst
3. The ECHO pin goes HIGH during the signal round-trip
4. The pulse duration is measured
5. Distance is computed from the time of flight

Distance formula:

[
distance = \frac{time \times speed\ of\ sound}{2}
]

---

## ⏱️ Signal Processing Implementation

The program uses **external interrupts (EXTI)** on pin **PA6**:

* **Rising edge interrupt** → start timing
* **Falling edge interrupt** → stop timing
* Pulse duration is computed in software

This approach avoids using timer input capture while still ensuring accurate measurement.

---

## 🧠 Software Architecture

The code is organized into modular components:

* `hcsr04.c / hcsr04.h`
  → Low-level driver for the ultrasonic sensor

* `app_sensors.c / app_sensors.h`
  → High-level sensor management

* `main.c`
  → Application entry point and main loop

* `utils.c / utils.h`
  → Utility functions

---

## 🔄 Measurement Cycle

1. Trigger pulse generation (PA5)
2. Wait for rising edge on PA6
3. Start timing
4. Wait for falling edge
5. Stop timing
6. Compute distance
7. Repeat periodically

---

## 🧪 Build & Flash

### Requirements

* STM32CubeIDE
* ARM GCC toolchain (included)

### Steps

1. Open the project in STM32CubeIDE
2. Generate code if needed from `.ioc`
3. Build the project
4. Flash to the STM32 board


