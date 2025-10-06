#include "display_manager.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 displays[CUE_COUNT] = {
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1),
};

void initDisplay() {
  for (int i = 0; i < CUE_COUNT; i++) {
    displays[i].begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS + i);
    displays[i].clearDisplay();
    displays[i].display();
  }
}

void updateDisplay(int index, const String &text) {
  if (index >= 0 && index < CUE_COUNT) {
    displays[index].clearDisplay();
    displays[index].setTextSize(1);
    displays[index].setTextColor(SSD1306_WHITE);
    displays[index].setCursor(0, 0);
    displays[index].println(text);
    displays[index].display();
  }
}