#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MS5837.h"

#define BATTERY_PIN A8  // Battery monitoring pin
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
MS5837 sensor;

float depth = 0.0;
unsigned long prevDisplayTime = 0;
unsigned long timerStartTime = 0;
bool timerStarted = false;
int minutes = 0, seconds = 0;
float batteryVoltage = 0.0;
const int numSamples = 20;  // Battery voltage oversampling

void setup() {
  Wire.begin();
  Wire.setClock(100000);  // I2C clock speed 100 kHz
  analogReadResolution(12);  // Internal ADC resolution 12-bit
  sensor.init();
  sensor.setModel(MS5837::MS5837_30BA);  // 30 bar model
  sensor.setFluidDensity(1020);  // EN13319 density
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display(); 
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - prevDisplayTime >= 500) {  // 2Hz refresh rate
    sensor.read();
    depth = sensor.depth();
    if (depth < 0) depth = 0;  // Minimum depth 0 meter
    if (depth > 99.9) depth = 99.9;  // Maximum depth 99.9 meters
    updateTimer(depth);
    batteryVoltage = getAverageBatteryVoltage();
    updateDisplay(depth, minutes, batteryVoltage);
    prevDisplayTime = currentTime;
  }
}

// Timer starts at 1 meter
void updateTimer(float depth) {
  if (depth >= 1.0 && !timerStarted) {
    timerStartTime = millis();
    timerStarted = true;
  }
  if (timerStarted) {
    unsigned long elapsedTime = millis() - timerStartTime;
    minutes = elapsedTime / 60000;
    seconds = (elapsedTime % 60000) / 1000;
  }
}

// Battery voltage averaging
float getAverageBatteryVoltage() {
  long total = 0;
  for (int i = 0; i < numSamples; i++) {
    total += analogRead(BATTERY_PIN);
  }
  float averageReading = total / numSamples;
  return averageReading * (3.3 / 4095.0) * 2;  // 1:1 voltage divider
}

// OLED display
void updateDisplay(float depth, int minutes, float batteryVoltage) {
  display.clearDisplay();

  // Depth
  char depthString[5];
  if (depth < 10) {
    snprintf(depthString, sizeof(depthString), " %.1f", depth);
  } else {
    snprintf(depthString, sizeof(depthString), "%.1f", depth);
  }
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(5);
  display.setCursor(6, 0);
  display.print(depthString);

  // Timer
  char timerString[7];
  snprintf(timerString, sizeof(timerString), "%3d:%02d", minutes, seconds);
  display.setTextSize(2);
  display.setCursor(0, 48);
  display.print(timerString);

  // Battery voltage
  char batteryString[6];
  snprintf(batteryString, sizeof(batteryString), "%.2fV", batteryVoltage);
  display.setTextSize(1);
  display.setCursor(SCREEN_WIDTH - 6 * strlen(batteryString), 56);
  display.print(batteryString);

  display.display();
}