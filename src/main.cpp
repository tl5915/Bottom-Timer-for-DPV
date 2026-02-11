#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MS5837.h>
#include <sam.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);
MS5837 sensor;

float depth = 0.0;
unsigned long timerStartTime = 0;
bool timerStarted = false;
int minutes = 0, seconds = 0;
float batteryVoltage = 0.0;

// Deep Sleep
void sleepMS(uint32_t milliseconds) {
  unsigned long startTime = millis();
  while (millis() - startTime < milliseconds) {
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;  // SLEEPDEEP
    __WFI();  // Wait for interrupt
  }
}

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
  char batteryString[5];
  snprintf(batteryString, sizeof(batteryString), "%.1fV", batteryVoltage);
  display.setTextSize(1);
  display.setCursor(128 - 6 * strlen(batteryString), 47);
  display.print(batteryString);

  // Battery percentgae
  int batteryPercentage = round(123 - (123 / pow((1 + pow((batteryVoltage / 3.7), 80)), 0.165)));
    if (batteryPercentage > 100) {
      batteryPercentage = 100;
    }
  display.drawRect(106, 56, 20, 8, SSD1306_WHITE);  // Main battery rectangle
  display.fillRect(126, 59, 2, 2, SSD1306_WHITE);  // Battery tip
  int16_t fillWidth = map(batteryPercentage, 0, 100, 0, 16);  // Map batteryPercentage to a fill width
  display.fillRect(108, 58, fillWidth, 4, SSD1306_WHITE);  // Fill the battery indicator

  display.display();
}

void setup() {
  Wire.begin();
  Wire.setClock(100000);  // I2C clock speed 100 kHz
  analogReadResolution(12);  // Internal ADC resolution 12-bit

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
  sensor.read();
  depth = sensor.depth();
  if (depth < 0) depth = 0;  // Minimum depth 0 meter
  if (depth > 99.9) depth = 99.9;  // Maximum depth 99.9 meters

  updateTimer(depth);

  batteryVoltage = analogRead(A8) * (3.3 / 4095.0) * 2;  // 1:1 voltage divider

  updateDisplay(depth, minutes, batteryVoltage);

  sleepMS(500);  // Sleep 500ms
}