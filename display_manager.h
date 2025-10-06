#include <Arduino.h>

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

void initDisplay();
void updateDisplay(int index, const String &text);

#endif
