#pragma once

#include <Arduino.h>

#include "config.h"

extern String cueTexts[CUE_COUNT];

void initCues();
void updateCues();
void triggerCue(size_t index);
void setCueText(size_t index, const String &text, bool persist = true);
bool isCueActive(size_t index);
String buildCueSnapshotJson();
String buildCueStateJson(size_t index);
