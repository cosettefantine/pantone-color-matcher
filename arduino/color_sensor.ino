#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <math.h>
#include <EEPROM.h>
#include <Servo.h>

const int joyX = A0;
const int joyY = A1;
const int joyButton = 40;

const int servoXPin = 9;
const int servoYPin = 10;

Servo servoX;
Servo servoY;

int servoXPos = 90;
int servoYPos = 90;

Adafruit_TCS34725 tcs = Adafruit_TCS34725(
    TCS34725_INTEGRATIONTIME_154MS,
    TCS34725_GAIN_16X
);

#define SAMPLE_COUNT 25
#define READ_TIMEOUT_MS 200
#define MAX_FAILS 5

#define CAL_MAGIC 0xA5B6C7D8UL

#define DISPLAY_GAMMA 2.2f
#define BLUE_COMP 1.08f
#define RED_COMP 0.96f
#define GREEN_COMP 1.04f

struct CalibrationData {
    float whiteR, whiteG, whiteB, whiteC;
    float grayR, grayG, grayB, grayC;
    float blackR, blackG, blackB, blackC;
    uint32_t magic;
};

CalibrationData cal;

int failCount = 0;


float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}


int clamp255(float x) {
    if (x < 0.0f) return 0;
    if (x > 255.0f) return 255;
    return (int)(x + 0.5f);
}


bool validPositive(float x) {
    return !isnan(x) && !isinf(x) && x > 0.0f;
}


bool validNonNegative(float x) {
    return !isnan(x) && !isinf(x) && x >= 0.0f;
}


void setupDefaultCalibration() {
    cal.whiteR = cal.whiteG = cal.whiteB = cal.whiteC = 0.0f;

    cal.grayR = cal.grayG = cal.grayB = 0.5f;
    cal.grayC = 1.0f;

    cal.blackR = cal.blackG = cal.blackB = cal.blackC = 0.0f;

    cal.magic = 0;
}


bool hasValidWhite() {
    return validPositive(cal.whiteR) &&
           validPositive(cal.whiteG) &&
           validPositive(cal.whiteB) &&
           validPositive(cal.whiteC);
}


bool hasValidGray() {
    return validPositive(cal.grayR) &&
           validPositive(cal.grayG) &&
           validPositive(cal.grayB) &&
           validPositive(cal.grayC);
}


bool hasValidBlack() {
    return validNonNegative(cal.blackR) &&
           validNonNegative(cal.blackG) &&
           validNonNegative(cal.blackB) &&
           validNonNegative(cal.blackC);
}


void saveCalibration() {
    cal.magic = CAL_MAGIC;
    EEPROM.put(0, cal);
}


bool loadCalibration() {
    EEPROM.get(0, cal);

    if (cal.magic != CAL_MAGIC) return false;
    if (!hasValidWhite()) return false;
    if (!hasValidGray()) return false;
    if (!hasValidBlack()) return false;

    return true;
}


bool resetSensor() {
    if (!tcs.begin()) {
        return false;
    }

    tcs.setInterrupt(true);

    return true;
}


bool readSafe(uint16_t &r, uint16_t &g, uint16_t &b, uint16_t &c) {
    unsigned long start = millis();

    while (millis() - start < READ_TIMEOUT_MS) {
        tcs.getRawData(&r, &g, &b, &c);

        if (c != 0) return true;

        delay(5);
    }

    resetSensor();

    return false;
}


void readAveraged(uint16_t &r, uint16_t &g, uint16_t &b, uint16_t &c) {
    uint32_t sumR = 0;
    uint32_t sumG = 0;
    uint32_t sumB = 0;
    uint32_t sumC = 0;

    int validSamples = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        uint16_t tr, tg, tb, tc;

        if (readSafe(tr, tg, tb, tc)) {
            sumR += tr;
            sumG += tg;
            sumB += tb;
            sumC += tc;

            validSamples++;
        }

        delay(10);
    }

    if (validSamples == 0) {
        r = g = b = c = 0;
        return;
    }

    r = sumR / validSamples;
    g = sumG / validSamples;
    b = sumB / validSamples;
    c = sumC / validSamples;
}


void setup() {
    Serial.begin(115200);

    pinMode(joyButton, INPUT_PULLUP);

    servoX.attach(servoXPin);
    servoY.attach(servoYPin);

    servoX.write(servoXPos);
    servoY.write(servoYPos);

    if (!tcs.begin()) {
        while (1);
    }

    delay(50);

    tcs.setInterrupt(true);

    delay(50);

    setupDefaultCalibration();
    loadCalibration();
}


void loop() {
    int rawX = analogRead(joyX);
    int rawY = analogRead(joyY);

    if (rawX < 450) servoXPos--;
    if (rawX > 570) servoXPos++;

    if (rawY < 450) servoYPos++;
    if (rawY > 570) servoYPos--;

    servoXPos = constrain(servoXPos, 0, 180);
    servoYPos = constrain(servoYPos, 0, 180);

    servoX.write(servoXPos);
    servoY.write(servoYPos);

    uint16_t r, g, b, c;

    readAveraged(r, g, b, c);

    if (c == 0) {
        failCount++;

        if (failCount >= MAX_FAILS) {
            resetSensor();
            failCount = 0;
        }

        delay(50);

        return;
    }

    failCount = 0;

    float blackR = hasValidBlack() ? cal.blackR : 0.0f;
    float blackG = hasValidBlack() ? cal.blackG : 0.0f;
    float blackB = hasValidBlack() ? cal.blackB : 0.0f;

    float whiteR = hasValidWhite() ? cal.whiteR : 1.0f;
    float whiteG = hasValidWhite() ? cal.whiteG : 1.0f;
    float whiteB = hasValidWhite() ? cal.whiteB : 1.0f;

    float denomR = whiteR - blackR;
    float denomG = whiteG - blackG;
    float denomB = whiteB - blackB;

    if (denomR < 1.0f) denomR = 1.0f;
    if (denomG < 1.0f) denomG = 1.0f;
    if (denomB < 1.0f) denomB = 1.0f;

    float corrR = ((float)r - blackR) / denomR;
    float corrG = ((float)g - blackG) / denomG;
    float corrB = ((float)b - blackB) / denomB;

    corrR *= RED_COMP;
    corrG *= GREEN_COMP;
    corrB *= BLUE_COMP;

    corrR = clamp01(corrR);
    corrG = clamp01(corrG);
    corrB = clamp01(corrB);

    float brightness = 1.0f;

    if (hasValidWhite()) {
        brightness = (float)c / (cal.whiteC + 1.0f);

        if (brightness < 0.90f) brightness = 0.90f;
        if (brightness > 1.05f) brightness = 1.05f;
    }

    corrR *= brightness;
    corrG *= brightness;
    corrB *= brightness;

    corrR = clamp01(corrR);
    corrG = clamp01(corrG);
    corrB = clamp01(corrB);

    corrR = pow(corrR, 1.0f / DISPLAY_GAMMA);
    corrG = pow(corrG, 1.0f / DISPLAY_GAMMA);
    corrB = pow(corrB, 1.0f / DISPLAY_GAMMA);

    int rgbR = clamp255(corrR * 255.0f);
    int rgbG = clamp255(corrG * 255.0f);
    int rgbB = clamp255(corrB * 255.0f);

    Serial.print(rgbR);
    Serial.print(" ");
    Serial.print(rgbG);
    Serial.print(" ");
    Serial.println(rgbB);
}
