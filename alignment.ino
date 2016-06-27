#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define OLED_RESET LED_BUILTIN//4

Adafruit_SSD1306 display(OLED_RESET);

void setup() {

  Serial.begin(115200);
  delay(10);
  Serial.println(F("Display Alignment"));
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();
}

void loop() {
  // put your main code here, to run repeatedly:
  display.drawRect(0,0,128,64,WHITE);
  //display.drawRect(2,2,125,61,WHITE);
  display.drawLine(0,0,127,63,WHITE);
  display.drawLine(0,63,127,0,WHITE);
  display.display();
  delay(100000);
}
