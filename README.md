# Bottom-Timer-for-DPV
Seeed Xiao SAMD21 and MS5837 based dive bottom timer for large headup display of depth while scootering.
https://scubaboard.com/community/threads/diy-bottom-timer-dive-computer.651599/

Material:
- Dji Action 4 underwater housing
- M10 tap
- PTFE tape
- Tactile button
- Seeed Xiao SAMD21
- LiPo Charge Managment Board e.g. TP4056
- 3.7V 1100 mAh LiPo battery
- ROV M10 High Precision Depth Transducer MS5837-30BA (https://www.aliexpress.com/item/1005008729751929.html)
- 1.54 inch SSD1309 I2C OLED

OLED Pin: GND - VCC - SCL - SDA
MS5837 Pin: VCC (red) - SCL (green) - SDA (white) - GND (black)

Connection:
Battery
- TP4056 B+ -> switch -> battery+
- TP4056 B- -> battery-
Top
- Pin 4 -> I2C bus SDA
- Pin 5 -> I2C bus SCL
- 3V3 -> I2C bus VCC
- GND -> I2C bus GND
- GND -> TP4056 OUT-
- 5V -> TP4056 OUT+

Bottom
- VIN -> 4.7k ohm resistor - Pin 9 -> 4.7 ohm resistor -> Pin 7  // switchable voltage divider for battery monitoring, 
- Pin 4 -> 4.7 ohm resistor - 3V3  // I2C pull up
- Pin 5 -> 4.7 ohm resistor - 3V3  // I2C pull up

Assembly:
1. Remove side button (power button) on housing, tap M10 thread
2. Mount MS5837 sensor on the M1O thread, seal with PTFE tape
3. Tape battery inside housing
4. Tape OLED on the rear door
5. Super glue tactile button on appropriately sized 3D-printed mounting bar, so top button (camera shutter) touches tactile button
6. Connect electronics