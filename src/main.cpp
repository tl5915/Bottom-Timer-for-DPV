#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <Wire.h>
#include <MS5837.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);
MS5837 sensor;

// Pins
const uint8_t SCLPin = 7;
const uint8_t SDAPin = 6;
const uint8_t batteryPin = 0;

// Battery Percentage Lookup Table
const float voltages[] = {3.27, 3.61, 3.69, 3.71, 3.73, 3.75, 3.77, 3.79, 3.80, 3.82, 3.84, 3.85, 3.87, 3.91, 3.95, 3.98, 4.02, 4.08, 4.11, 4.15, 4.20};
const uint8_t percentages[] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};
const uint8_t TABLE_SIZE = 21;

// Variables
float depth = 0.0;
float batteryVoltage = 0.0;
unsigned long timerStartTime = 0;
bool diveTimerStarted = false;
int minutes = 0, seconds = 0;


// Timer
void updateTimer(float depth) {
  if (depth >= 1.0 && !diveTimerStarted) {  // Timer starts at 1 meter
    timerStartTime = millis();
    diveTimerStarted = true;
  }
  if (diveTimerStarted) {
    unsigned long elapsed = millis() - timerStartTime;
    minutes = elapsed / 60000;
    seconds = (elapsed % 60000) / 1000;
  }
}

// Battery Voltage
float readBattery() {
  const int samples = 16;
  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogReadMilliVolts(batteryPin);
  }
  float voltage = (sum / samples) / 1000.0 * 2.0;  // 1:1 voltage divider
  return voltage;
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
  // Power Conservation
  esp_wifi_stop();                            // WiFi off
  esp_bt_controller_disable();                // Bluetooth off
  setCpuFrequencyMhz(10);                     // Reduce CPU frequency

  // ADC
  analogReadResolution(12);                   // Internal ADC resolution 12-bit
  analogSetAttenuation(ADC_11db);             // 2.5V range

  // I2C Initialisation
  Wire.begin(SDAPin, SCLPin);                 // I2C start
  Wire.setClock(400000);                      // I2C clock speed 400kHz

  // MS5837 Initialisation
  sensor.init();
  sensor.setModel(MS5837::MS5837_30BA);       // 30 bar model
  sensor.setFluidDensity(1020);               // EN13319 density

  // Display Initialisation
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // OLED start
  display.setTextColor(SSD1306_WHITE);        // Set text colour
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(28, 12);
  display.print(F("Bottom"));
  display.setCursor(34, 36);
  display.print(F("Timer"));
  display.display();
  esp_sleep_enable_timer_wakeup(500000); 
  esp_light_sleep_start();
}

void loop() {
  sensor.read();
  depth = sensor.depth() - 0.2;    // Sea level off set
  if (depth < 0) depth = 0;        // Minimum depth 0 meter
  if (depth > 99.9) depth = 99.9;  // Maximum depth 99.9 meters

  updateTimer(depth);

  batteryVoltage = readBattery();

  updateDisplay(depth, minutes, seconds, batteryVoltage);

  if (batteryVoltage < 3.3) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(22, 12);
    display.print(F("Battery"));
    display.setCursor(46, 36);
    display.print(F("Low"));
    display.display();
    esp_sleep_enable_timer_wakeup(1000000); 
    esp_light_sleep_start();
    esp_deep_sleep_start();
  }

  esp_sleep_enable_timer_wakeup(1000000);  // 1000ms interval
  esp_light_sleep_start();
}