# Color Sensor Calibration

This document describes the calibration and RGB preprocessing process used in the **Pantone Color Matcher** project.

The purpose of calibration is to reduce the difference between the raw values measured by the TCS34725 color sensor and the visible color of the physical object.

Raw sensor values can vary because of lighting conditions, sensor response characteristics, surface reflectivity, and differences between the red, green, and blue channels. The project therefore uses reference measurements and software correction before sending the final RGB values to the Raspberry Pi.

---

## Calibration Overview

The calibration system uses three reference colors:

- **White reference**
- **Gray reference**
- **Black reference**

The measured reference values are stored in the Arduino Mega's EEPROM so they remain available after the system is restarted.

The calibration structure used by the Arduino is:

```cpp
struct CalibrationData {
    float whiteR, whiteG, whiteB, whiteC;
    float grayR, grayG, grayB, grayC;
    float blackR, blackG, blackB, blackC;
    uint32_t magic;
};
```

Each reference stores four sensor channels:

- `R` — Red
- `G` — Green
- `B` — Blue
- `C` — Clear / overall light intensity

---

## Sensor Configuration

The TCS34725 is configured with:

```cpp
Adafruit_TCS34725 tcs =
    Adafruit_TCS34725(
        TCS34725_INTEGRATIONTIME_154MS,
        TCS34725_GAIN_16X
    );
```

| Parameter | Value |
|---|---:|
| Integration Time | 154 ms |
| Gain | 16× |
| Samples per Measurement | 25 |
| Serial Baud Rate | 115200 |

The sensor interrupt is enabled after initialization:

```cpp
tcs.setInterrupt(true);
```

---

## Why Calibration Is Required

The raw RGB values returned by the sensor are not directly equivalent to standard display RGB values.

Several factors can affect the measurement:

- Ambient lighting
- Distance between sensor and object
- Surface reflectivity
- Sensor-to-sensor manufacturing differences
- Unequal spectral response of the R, G and B channels
- Measurement noise
- Overall brightness of the scene

For this reason, the raw values are processed before being converted to the final `0–255` RGB range.

---

## Reference Calibration

### White Reference

The white calibration card represents the upper measurement boundary.

The stored values are:

```text
whiteR
whiteG
whiteB
whiteC
```

These values are later used to normalize new measurements.

A valid white calibration requires all four values to be greater than zero:

```cpp
bool hasValidWhite() {
    return validPositive(cal.whiteR) &&
           validPositive(cal.whiteG) &&
           validPositive(cal.whiteB) &&
           validPositive(cal.whiteC);
}
```

---

### Black Reference

The black calibration card represents the lower measurement boundary.

Stored values:

```text
blackR
blackG
blackB
blackC
```

The black values are subtracted from new sensor measurements before normalization.

```cpp
float corrR = ((float)r - blackR) / denomR;
float corrG = ((float)g - blackG) / denomG;
float corrB = ((float)b - blackB) / denomB;
```

This helps compensate for the sensor's baseline response when measuring very dark surfaces.

---

### Gray Reference

A gray calibration card is also stored by the calibration system.

```text
grayR
grayG
grayB
grayC
```

The gray reference provides an additional neutral-color reference that can be used when evaluating the balance between the red, green, and blue channels.

The calibration structure preserves these values in EEPROM together with the white and black references.

---

## EEPROM Storage

Calibration values are stored in EEPROM so calibration does not have to be recreated every time the Arduino restarts.

A magic number is used to determine whether valid calibration data exists:

```cpp
#define CAL_MAGIC 0xA5B6C7D8UL
```

When calibration data is saved:

```cpp
void saveCalibration() {
    cal.magic = CAL_MAGIC;
    EEPROM.put(0, cal);
}
```

During startup:

```cpp
bool loadCalibration() {
    EEPROM.get(0, cal);

    if (cal.magic != CAL_MAGIC)
        return false;

    if (!hasValidWhite())
        return false;

    if (!hasValidGray())
        return false;

    if (!hasValidBlack())
        return false;

    return true;
}
```

If valid calibration data exists, it is automatically loaded when the Arduino starts.

---

## Measurement Averaging

A single sensor measurement may contain noise.

To obtain a more stable result, the system collects **25 samples** for every measurement cycle.

```cpp
#define SAMPLE_COUNT 25
```

The valid samples are accumulated:

```cpp
sumR += tr;
sumG += tg;
sumB += tb;
sumC += tc;
```

Then their averages are calculated:

```cpp
r = sumR / validSamples;
g = sumG / validSamples;
b = sumB / validSamples;
c = sumC / validSamples;
```

Using multiple samples reduces short-term measurement fluctuations before calibration is applied.

---

## Sensor Read Protection

Each sensor read uses a timeout:

```cpp
#define READ_TIMEOUT_MS 200
```

If valid sensor data cannot be received before the timeout, the sensor is reinitialized.

```cpp
bool readSafe(
    uint16_t &r,
    uint16_t &g,
    uint16_t &b,
    uint16_t &c
)
```

The program also counts consecutive failures:

```cpp
#define MAX_FAILS 5
```

After repeated failures, the sensor is reset:

```cpp
if (failCount >= MAX_FAILS) {
    resetSensor();
    failCount = 0;
}
```

This prevents temporary sensor communication problems from permanently stopping the measurement loop.

---

## Black–White Normalization

The white and black reference values define the usable range for each RGB channel.

For the red channel:

```text
Normalized Red =
(Current Red - Black Red)
/
(White Red - Black Red)
```

The same operation is applied independently to the green and blue channels.

In the Arduino implementation:

```cpp
float denomR = whiteR - blackR;
float denomG = whiteG - blackG;
float denomB = whiteB - blackB;
```

To avoid invalid division:

```cpp
if (denomR < 1.0f) denomR = 1.0f;
if (denomG < 1.0f) denomG = 1.0f;
if (denomB < 1.0f) denomB = 1.0f;
```

The normalized values are then calculated:

```cpp
float corrR = ((float)r - blackR) / denomR;
float corrG = ((float)g - blackG) / denomG;
float corrB = ((float)b - blackB) / denomB;
```

This maps the sensor response approximately between the black and white calibration boundaries.

---

## RGB Channel Compensation

The three color channels do not respond identically.

Software compensation factors are therefore applied:

```cpp
#define BLUE_COMP  1.08f
#define RED_COMP   0.96f
#define GREEN_COMP 1.04f
```

The correction is applied as:

```cpp
corrR *= RED_COMP;
corrG *= GREEN_COMP;
corrB *= BLUE_COMP;
```

Current compensation factors:

| Channel | Multiplier |
|---|---:|
| Red | 0.96 |
| Green | 1.04 |
| Blue | 1.08 |

These values compensate for differences observed between the sensor output and the expected visible color.

---

## Value Clamping

After normalization and compensation, every channel is limited to the valid range:

```text
0.0 – 1.0
```

using:

```cpp
float clamp01(float x) {
    if (x < 0.0f)
        return 0.0f;

    if (x > 1.0f)
        return 1.0f;

    return x;
}
```

This prevents invalid negative values and values greater than the maximum RGB range.

---

## Brightness Compensation

The sensor's clear channel (`C`) is used to estimate the overall brightness of the measurement.

When valid white calibration exists:

```cpp
brightness = (float)c / (cal.whiteC + 1.0f);
```

The correction factor is restricted to:

```text
0.90 – 1.05
```

```cpp
if (brightness < 0.90f)
    brightness = 0.90f;

if (brightness > 1.05f)
    brightness = 1.05f;
```

The factor is then applied to all three RGB channels:

```cpp
corrR *= brightness;
corrG *= brightness;
corrB *= brightness;
```

Restricting the adjustment prevents changes in scene brightness from causing excessively large RGB corrections.

---

## Gamma Correction

The calibrated sensor values are linear values, while colors shown on a display require a nonlinear transformation for more natural visualization.

The project uses:

```cpp
#define DISPLAY_GAMMA 2.2f
```

Gamma correction is applied as:

```cpp
corrR = pow(corrR, 1.0f / DISPLAY_GAMMA);
corrG = pow(corrG, 1.0f / DISPLAY_GAMMA);
corrB = pow(corrB, 1.0f / DISPLAY_GAMMA);
```

This prepares the calibrated values for display as standard RGB colors.

---

## Conversion to 8-bit RGB

After all calibration and correction stages, each channel is converted from the normalized range to the standard RGB range:

```text
0 – 255
```

```cpp
int rgbR = clamp255(corrR * 255.0f);
int rgbG = clamp255(corrG * 255.0f);
int rgbB = clamp255(corrB * 255.0f);
```

The resulting values represent the final processed sensor color.

---

## Complete Processing Pipeline

The complete measurement pipeline can be summarized as:

```text
Physical Object
      │
      ▼
TCS34725 Sensor
      │
      ▼
Raw R / G / B / C
      │
      ▼
25-Sample Averaging
      │
      ▼
Black Reference Subtraction
      │
      ▼
White Reference Normalization
      │
      ▼
RGB Channel Compensation
      │
      ▼
Brightness Compensation
      │
      ▼
Value Clamping
      │
      ▼
Gamma Correction
      │
      ▼
0–255 RGB Conversion
      │
      ▼
Serial Communication
      │
      ▼
Raspberry Pi
      │
      ▼
Pantone Matching Algorithm
```

---

## Serial Output

Only the final processed RGB values are sent to the Raspberry Pi.

Example:

```text
128 3 65
```

Arduino output:

```cpp
Serial.print(rgbR);
Serial.print(" ");
Serial.print(rgbG);
Serial.print(" ");
Serial.println(rgbB);
```

The communication speed is:

```text
115200 baud
```

Keeping the serial output limited to RGB values makes parsing on the Raspberry Pi side simple and reliable.

---

## Raspberry Pi Processing

The Raspberry Pi receives the calibrated RGB values over serial communication.

The application then compares the measured RGB value against the Pantone RGB dataset.

The distance used by the current implementation is:

```text
distance =
(Rsensor - Rpantone)² +
(Gsensor - Gpantone)² +
(Bsensor - Bpantone)²
```

The Pantone entry with the smallest distance is selected as the closest match.

Calibration is therefore completed on the Arduino side **before** Pantone comparison is performed.

---

## Calibration Data Flow

```text
Calibration Cards
      │
      ├── White
      ├── Gray
      └── Black
      │
      ▼
TCS34725 Measurements
      │
      ▼
CalibrationData Structure
      │
      ▼
Arduino EEPROM
      │
      ▼
Loaded at Startup
      │
      ▼
Applied to Every Sensor Measurement
```

---

## Calibration Conditions

For more consistent measurements:

- Keep the sensor-to-object distance as constant as possible.
- Keep the pan-tilt mechanism stable while reading the color.
- Avoid changing the lighting environment after calibration.
- Use the same calibration cards when recalibrating the system.
- Keep the measured surface inside the sensor's effective field of view.
- Avoid strong external light directly entering the sensor.
- Recalibrate if the optical setup or lighting conditions are significantly changed.

---

## Current Calibration Parameters

```cpp
TCS34725_INTEGRATIONTIME_154MS
TCS34725_GAIN_16X

SAMPLE_COUNT       = 25
READ_TIMEOUT_MS    = 200
MAX_FAILS          = 5

RED_COMP           = 0.96
GREEN_COMP         = 1.04
BLUE_COMP          = 1.08

DISPLAY_GAMMA      = 2.2
```

---

## Notes

The project report describes white, gray, and black calibration-card measurements being stored in EEPROM and used as references for subsequent measurements.

The current Arduino implementation shown in the project report contains the EEPROM data structure, validation, loading, saving, normalization, compensation, averaging, brightness correction, and gamma-processing logic.

The calibration-card capture procedure itself should therefore be kept consistent with the calibration version of the Arduino firmware used when generating the stored EEPROM reference values.
