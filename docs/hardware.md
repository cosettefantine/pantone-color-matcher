# Hardware

This document describes the hardware architecture and physical components used in the **Pantone Color Matcher** project.

The system combines an Arduino-based sensing and motion-control layer with a Raspberry Pi-based graphical interface and Pantone matching application.

---

## System Overview

The hardware architecture consists of four main sections:

1. **Color acquisition**
2. **Sensor positioning**
3. **Processing and communication**
4. **User interface**

The overall hardware flow is:

```text
Physical Object
      │
      ▼
TCS34725 Color Sensor
      │
      │ I2C
      ▼
Arduino Mega 2560
      │
      ├── Pan Servo
      ├── Tilt Servo
      ├── Joystick Module
      │
      │ USB Serial
      ▼
Raspberry Pi 3 Model B
      │
      ▼
Raspberry Pi Touch Display 2
      │
      ▼
Pantone Matching GUI
```

The Arduino is responsible for:

- Reading the color sensor
- Controlling the pan-tilt mechanism
- Reading joystick input
- Applying RGB preprocessing and calibration
- Sending processed RGB values to the Raspberry Pi

The Raspberry Pi is responsible for:

- Receiving RGB values through serial communication
- Loading the Pantone RGB dataset
- Finding the nearest Pantone color
- Displaying the measured and matched colors on the graphical interface

---

# Hardware Components

## 1. Arduino Mega 2560

The **Arduino Mega 2560** acts as the main embedded controller of the sensing system.

Its responsibilities include:

- Communicating with the TCS34725 color sensor
- Reading joystick inputs
- Controlling two servo motors
- Performing RGB calibration and preprocessing
- Storing calibration data in EEPROM
- Sending processed RGB data over serial communication

The Arduino communicates with the Raspberry Pi at:

```text
115200 baud
```

### Used Arduino Pins

| Function | Arduino Mega Pin |
|---|---|
| Joystick X axis | A0 |
| Joystick Y axis | A1 |
| Joystick button | 40 |
| Pan servo | 9 |
| Tilt servo | 10 |
| Color sensor | I2C |
| Raspberry Pi communication | USB Serial |

Relevant pin definitions:

```cpp
const int joyX = A0;
const int joyY = A1;
const int joyButton = 40;

const int servoXPin = 9;
const int servoYPin = 10;
```

---

## 2. TCS34725 Color Sensor

The **TCS34725** is used to measure reflected light from physical objects.

The sensor provides four channels:

- Red
- Green
- Blue
- Clear

The clear channel represents the overall measured light intensity and is also used by the calibration algorithm for brightness compensation.

The sensor communicates with the Arduino through **I2C**.

### Sensor Configuration

The project uses:

```cpp
TCS34725_INTEGRATIONTIME_154MS
TCS34725_GAIN_16X
```

Therefore:

| Parameter | Configuration |
|---|---|
| Integration time | 154 ms |
| Gain | 16× |
| Interface | I2C |
| Measured channels | R, G, B, C |

The sensor is mounted on the pan-tilt mechanism so its orientation can be manually adjusted toward the surface being measured.

---

## 3. Raspberry Pi 3 Model B

The **Raspberry Pi 3 Model B** acts as the high-level processing and user-interface computer.

The Raspberry Pi runs the Python application responsible for:

- Serial communication with Arduino
- Receiving RGB measurements
- Loading the Pantone RGB CSV dataset
- Calculating the nearest Pantone shade
- Displaying the results through the Tkinter GUI
- Showing the Raspberry Pi's local IP address

The application is designed to run on **Raspberry Pi OS Lite**.

Arduino data is received through:

```text
/dev/ttyACM0
```

with:

```text
115200 baud
```

---

## 4. Raspberry Pi Touch Display 2

The **Raspberry Pi Touch Display 2** is used as the local display for the Pantone matching application.

It displays:

- Sensor color
- Sensor RGB values
- Nearest Pantone color
- Pantone name
- Pantone RGB values
- System status
- Raspberry Pi IP address

The interface runs in fullscreen mode to allow the device to operate as a standalone embedded color matching system.

---

# Pan-Tilt Mechanism

The TCS34725 sensor is mounted on a two-axis pan-tilt mechanism.

The mechanism allows the sensor to rotate:

- Horizontally — **Pan**
- Vertically — **Tilt**

Two mini servo motors are used.

This configuration allows the operator to manually aim the sensor at different regions of an object.

---

## Servo Motors

Two mini servo motors control the pan-tilt assembly.

| Servo | Function | Arduino Pin |
|---|---|---:|
| Servo X | Horizontal / Pan movement | 9 |
| Servo Y | Vertical / Tilt movement | 10 |

The starting servo positions are:

```cpp
int servoXPos = 90;
int servoYPos = 90;
```

This places both axes approximately at their center positions during startup.

The software restricts both servo positions to:

```text
0° – 180°
```

using:

```cpp
servoXPos = constrain(servoXPos, 0, 180);
servoYPos = constrain(servoYPos, 0, 180);
```

---

# Joystick Module

A two-axis joystick module provides manual pan-tilt control.

The joystick supplies:

- Horizontal analog value
- Vertical analog value
- Push-button input

### Connection Table

| Joystick Signal | Arduino Pin |
|---|---|
| X axis | A0 |
| Y axis | A1 |
| Push button | 40 |

The button is configured using the Arduino's internal pull-up resistor:

```cpp
pinMode(joyButton, INPUT_PULLUP);
```

---

## Joystick Motion Logic

The joystick controls the pan and tilt servo positions.

### Horizontal Axis

```cpp
if (rawX < 450)
    servoXPos--;

if (rawX > 570)
    servoXPos++;
```

### Vertical Axis

```cpp
if (rawY < 450)
    servoYPos++;

if (rawY > 570)
    servoYPos--;
```

The range between approximately:

```text
450 – 570
```

acts as a joystick dead zone.

This prevents minor analog fluctuations near the joystick center from continuously moving the servos.

---

# Sensor Mounting

The color sensor should face the target surface as directly as possible.

For repeatable measurements, the mechanical setup should maintain approximately consistent:

- Sensor-to-object distance
- Sensor angle
- Lighting conditions
- Measurement area
- Surface orientation

Changes in these conditions can affect reflected light and therefore the measured RGB values.

---

# Serial Communication

The Arduino and Raspberry Pi communicate through a USB serial connection.

```text
Arduino Mega 2560
       │
       │ USB
       │ 115200 baud
       ▼
Raspberry Pi 3 Model B
```

The Arduino sends only processed RGB values.

Example:

```text
128 3 65
```

The output format is:

```text
R G B
```

Arduino code:

```cpp
Serial.print(rgbR);
Serial.print(" ");
Serial.print(rgbG);
Serial.print(" ");
Serial.println(rgbB);
```

The simplified serial format makes parsing straightforward on the Raspberry Pi.

---

# Power System

The project hardware includes:

- 5.1 V / 3 A power supply
- Two 18650 lithium-ion batteries
- Battery holder
- XL4015 step-down converter
- USB cables

---

## 5.1 V / 3 A Power Supply

A **5.1 V / 3 A power supply** is included for powering the Raspberry Pi-based part of the prototype.

A stable supply is important because the Raspberry Pi, touchscreen, USB serial connection, and graphical application operate continuously during system use.

---

## 18650 Batteries

The project also includes:

```text
2 × 18650 Li-ion batteries
```

These rechargeable batteries form part of the portable power hardware used in the prototype.

---

## XL4015 Step-Down Converter

An **XL4015 DC-DC step-down converter** is included in the power system.

A step-down converter reduces a higher DC input voltage to a lower regulated output voltage suitable for connected electronics.

The exact final power distribution between all prototype components is not specified in the project report, so the converter should be configured according to the voltage requirements of the connected hardware before use.

---

# Breadboard and Jumper Wires

A breadboard and jumper wires are used for prototype interconnections.

They provide connections between components such as:

- Arduino Mega
- TCS34725
- Joystick module
- Power rails
- Supporting prototype electronics

Breadboard connections are useful during development because the circuit can be modified without soldering.

---

# Hardware Connection Summary

| Component | Connected To | Interface |
|---|---|---|
| TCS34725 | Arduino Mega | I2C |
| Joystick X | Arduino Mega A0 | Analog |
| Joystick Y | Arduino Mega A1 | Analog |
| Joystick Button | Arduino Mega Pin 40 | Digital |
| Pan Servo | Arduino Mega Pin 9 | PWM / Servo signal |
| Tilt Servo | Arduino Mega Pin 10 | PWM / Servo signal |
| Arduino Mega | Raspberry Pi 3 B | USB Serial |
| Raspberry Pi | Touch Display 2 | Display interface |
| Power hardware | Prototype system | DC power |

---

# Arduino Pin Map

```text
Arduino Mega 2560
│
├── A0  → Joystick X
├── A1  → Joystick Y
├── D9  → Pan Servo
├── D10 → Tilt Servo
├── D40 → Joystick Button
│
├── I2C
│    └── TCS34725
│
└── USB Serial
     └── Raspberry Pi 3 Model B
```

---

# Hardware Data Flow

```text
            ┌─────────────────┐
            │ Physical Object │
            └────────┬────────┘
                     │
              Reflected Light
                     │
                     ▼
            ┌────────────────┐
            │    TCS34725    │
            │  Color Sensor  │
            └───────┬────────┘
                    │ I2C
                    ▼
            ┌────────────────┐
            │  Arduino Mega  │
            │      2560      │
            └───────┬────────┘
                    │
       ┌────────────┼────────────┐
       │            │            │
       ▼            ▼            ▼
   Joystick     Pan Servo    Tilt Servo

                    │
                    │ USB Serial
                    ▼
            ┌────────────────┐
            │ Raspberry Pi   │
            │   3 Model B    │
            └───────┬────────┘
                    │
                    ▼
            ┌────────────────┐
            │ Touch Display  │
            │       2        │
            └────────────────┘
```

---

# Bill of Materials

| Qty. | Component | Purpose |
|---:|---|---|
| 1 | Arduino Mega 2560 | Sensor processing and hardware control |
| 1 | Raspberry Pi 3 Model B | Pantone matching and GUI |
| 1 | Raspberry Pi Touch Display 2 | Local graphical interface |
| 1 | TCS34725 Color Sensor | RGB color acquisition |
| 1 | Pan-Tilt Mechanism | Sensor positioning |
| 2 | Mini Servo Motors | Pan and tilt movement |
| 1 | Joystick Module | Manual sensor positioning |
| 1 | Breadboard | Prototype connections |
| Several | Jumper Wires | Electrical connections |
| 1 | 5.1 V / 3 A Power Supply | Raspberry Pi power |
| 2 | 18650 Li-ion Batteries | Portable power hardware |
| 1 | Battery Holder | Battery installation |
| 1 | XL4015 Step-Down Converter | DC voltage regulation |
| Several | USB Cables | Power and serial communication |

---

# Alternative Sensor

The initial system uses the:

```text
TCS34725
```

color sensor.

A possible future hardware improvement mentioned for the project is the:

```text
AS7341 10-Channel Light / Color Sensor
```

The AS7341 can provide additional spectral channels and may allow more detailed color analysis compared with the RGB-based TCS34725 approach.

The current implementation, however, is based on the **TCS34725**.

---

# Hardware Responsibilities

## Arduino Layer

```text
Sensor Acquisition
        +
Calibration
        +
Pan-Tilt Control
        +
Joystick Input
        +
Serial Output
```

## Raspberry Pi Layer

```text
Serial Input
      +
Pantone Dataset
      +
Color Matching
      +
GUI
```

This separation keeps timing-sensitive sensor and actuator operations on the microcontroller while assigning the graphical interface and Pantone comparison algorithm to the Raspberry Pi.

---

# Prototype Hardware Summary

The final prototype combines:

```text
TCS34725
    │
    ▼
Arduino Mega 2560
    │
    ├── Joystick
    ├── Pan Servo
    └── Tilt Servo
    │
    ▼
USB Serial
    │
    ▼
Raspberry Pi 3 Model B
    │
    ▼
Raspberry Pi Touch Display 2
    │
    ▼
Real-Time Pantone Matching Interface
```

The hardware architecture provides a compact embedded platform for acquiring the color of a physical surface, preprocessing its RGB values, and displaying the nearest Pantone match in real time.
