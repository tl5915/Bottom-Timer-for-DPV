#include <Arduino.h>
#include <sam.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MS5837.h>
#include <ArduinoLowPower.h>
#include <SimpleKalmanFilter.h>

SimpleKalmanFilter batteryKalman(0.1, 100, 0.001);  // R: measurement error, P: estimated error, Q: process noise
Adafruit_SSD1306 display(128, 64, &Wire, -1);
MS5837 sensor;

// Pins
const uint8_t enablePin = 7;
const uint8_t batteryPin = 9;

// Battery Percentage Lookup Table
const float voltages[] = {3.27, 3.61, 3.69, 3.71, 3.73, 3.75, 3.77, 3.79, 3.80, 3.82, 3.84, 3.85, 3.87, 3.91, 3.95, 3.98, 4.02, 4.08, 4.11, 4.15, 4.20};
const uint8_t percentages[] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};
const uint8_t TABLE_SIZE = 21;

// Variables
float depth = 0.0;
float batteryVoltage = 0.0;
unsigned long timerStartTime = 0;
bool timerStarted = false;
int minutes = 0, seconds = 0;

// Timer
void updateTimer(float depth) {
  if (depth >= 1.0 && !timerStarted) {  // Timer starts at 1 meter
    timerStartTime = millis();
    timerStarted = true;
  }
  if (timerStarted) {
    unsigned long elapsedTime = millis() - timerStartTime;
    minutes = elapsedTime / 60000;
    seconds = (elapsedTime % 60000) / 1000;
  }
}

// Battery Voltage
float readBattery() {
  digitalWrite(enablePin, LOW);
  pinMode(enablePin, OUTPUT);
  analogRead(batteryPin);  // Discard first reading
  const int samples = 32;
  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(batteryPin);
  }
  pinMode(enablePin, INPUT);
  uint32_t averageReading = sum / samples;
  float voltage = averageReading * (3.3 / 4095.0) * 2;  // 1:1 voltage divider
  return batteryKalman.updateEstimate(voltage);
}

// Battery Percentage
uint8_t batteryPercentage(float batteryVoltage) {
  if (batteryVoltage <= voltages[0]) 
    return percentages[0];
  if (batteryVoltage >= voltages[TABLE_SIZE - 1]) 
    return percentages[TABLE_SIZE - 1];
  for (int i = 0; i < TABLE_SIZE - 1; i++) {
    float lower = voltages[i];
    float upper = voltages[i + 1];
    if (batteryVoltage < upper) {
      if ((batteryVoltage - lower) < (upper - batteryVoltage)) 
        return percentages[i];
      else
        return percentages[i + 1];
    }
  }
  return 0;  // Fallback
}

// OLED display
void updateDisplay(float depth, int minutes, int seconds, float batteryVoltage) {
  display.clearDisplay();

  // Depth
  char depthString[5];
  if (depth < 10) {
    snprintf(depthString, sizeof(depthString), " %.1f", depth);
  } else {
    snprintf(depthString, sizeof(depthString), "%.1f", depth);
  }
  display.setTextSize(5);
  display.setCursor(6, 0);
  display.print(depthString);

  // Timer
  char timerString[7];
  snprintf(timerString, sizeof(timerString), "%3d:%02d", minutes, seconds);
  display.setTextSize(2);
  display.setCursor(0, 48);
  display.print(timerString);

  // Battery
  char batteryString[5];
  snprintf(batteryString, sizeof(batteryString), "%.1fV", batteryVoltage);
  display.setTextSize(1);
  display.setCursor(128 - 6 * strlen(batteryString), 47);
  display.print(batteryString);

  uint8_t percentage = batteryPercentage(batteryVoltage);
  int16_t fillWidth = map(percentage, 0, 100, 0, 16);  // Map batteryPercentage to a fill width
  display.drawRect(106, 56, 20, 8, SSD1306_WHITE);  // Main battery rectangle
  display.fillRect(126, 59, 2, 2, SSD1306_WHITE);  // Battery tip
  display.fillRect(108, 58, fillWidth, 4, SSD1306_WHITE);  // Fill the battery indicator

  display.display();
}

void setup() {
  Wire.begin();
  Wire.setClock(100000);  // I2C clock speed 100 kHz
  analogReadResolution(12);  // Internal ADC resolution 12-bit
  batteryKalman.updateEstimate(3.7);  // Kalman filter starting point 3.7V
  batteryKalman.setEstimateError(0.1);  // Reset P

  sensor.init();
  sensor.setModel(MS5837::MS5837_30BA);  // 30 bar model
  sensor.setFluidDensity(1020);  // EN13319 density

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(28, 12);
  display.print(F("Bottom"));
  display.setCursor(34, 36);
  display.print(F("Timer"));
  display.display();
  delay(500);
}

void loop() {
  unsigned long start = millis();

  sensor.read();
  depth = sensor.depth();
  if (depth < 0) depth = 0;  // Minimum depth 0 meter
  if (depth > 99.9) depth = 99.9;  // Maximum depth 99.9 meters

  updateTimer(depth);

  batteryVoltage = readBattery();

  updateDisplay(depth, minutes, seconds, batteryVoltage);

  unsigned long elapsed = millis() - start;
  if (elapsed < 500) {
    LowPower.sleep(500 - elapsed);  // Sleep 500ms
  }
}