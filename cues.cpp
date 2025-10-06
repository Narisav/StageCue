#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "cues.h"
#include "config.h"
#include "display_manager.h"

extern AsyncWebSocket ws;

String cueTexts[CUE_COUNT];
bool cueStates[CUE_COUNT] = {false, false, false};

void initCues() {
  for (int i = 0; i < CUE_COUNT; i++) {
    pinMode(cueLEDs[i], OUTPUT);
    cueTexts[i] = defaultCueTexts[i];
  }
}

void updateCues() {
  // (ex: détecter bouton et déclencher un cue)
}

void triggerCue(int index) {
  if (index >= 0 && index < CUE_COUNT) {

    digitalWrite(cueLEDs[index], HIGH);
    updateDisplay(index, cueTexts[index]);

    // 🔁 Envoie le feedback à l'interface Web
    ws.textAll("cueTrigger:" + String(index));
  }
}