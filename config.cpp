#include "config.h"

const char API_AUTH_TOKEN[] = "stagecue-admin";
const char DEVICE_NAME[] = "StageCue";

const char *defaultCueTexts[CUE_COUNT] = {"Cue 1", "Cue 2", "Cue 3"};

const uint8_t displayAddresses[CUE_COUNT] = {
    OLED_ADDRESS,
    static_cast<uint8_t>(OLED_ADDRESS + 1),
    static_cast<uint8_t>(OLED_ADDRESS + 2)};

const int cueLEDs[CUE_COUNT] = {25, 26, 27};
const int cueButtons[CUE_COUNT] = {32, 33, 34};