#include "config.h"

const char API_AUTH_TOKEN[] = "stagecue-admin";
const char DEVICE_NAME[] = "StageCue";

const char *defaultCueTexts[CUE_COUNT] = {"Cue 1", "Cue 2", "Cue 3"};

const uint8_t displayAddresses[CUE_COUNT] = {0x3C, 0x3D, 0x3E};

// Broches par défaut pour un ESP32-C6 DevKitC : ajustez selon votre câblage.
const int cueLEDs[CUE_COUNT] = {18, 19, 20};
const int cueButtons[CUE_COUNT] = {1, 2, 3};

static_assert(sizeof(cueLEDs) / sizeof(cueLEDs[0]) == CUE_COUNT,
              "cueLEDs doit contenir une entrée par cue");
static_assert(sizeof(cueButtons) / sizeof(cueButtons[0]) == CUE_COUNT,
              "cueButtons doit contenir une entrée par cue");
static_assert(sizeof(displayAddresses) / sizeof(displayAddresses[0]) == CUE_COUNT,
              "displayAddresses doit contenir une entrée par cue");
