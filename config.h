#include <Arduino.h>

#ifndef CONFIG_H
#define CONFIG_H

//WIFI 
  //Local Connect Mode
#define WIFI_SSID     "TON_SSID"
#define WIFI_PASSWORD "TON_PASSWORD"
  //Access Point Mode
#define FALLBACK_AP_SSID "CueLight_AP"
#define FALLBACK_AP_PASS "12345678"

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

// Cues
#define CUE_COUNT 3

extern const char* defaultCueTexts[CUE_COUNT];

// LED Pins (à adapter selon ton câblage)
const int cueLEDs[CUE_COUNT] = {25, 26, 27};

// Boutons (si tu veux piloter localement)
const int cueButtons[CUE_COUNT] = {32, 33, 34};

#endif