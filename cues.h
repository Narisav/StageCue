#include "config.h"
#include <Arduino.h>

#ifndef CUES_H
#define CUES_H

extern String cueTexts[CUE_COUNT];

void initCues();
void updateCues();
void triggerCue(int index);

#endif