#pragma once

#include <Arduino.h>

void initDisplay();
void updateDisplay(size_t index, const String &text);
bool isDisplayReady(size_t index);

