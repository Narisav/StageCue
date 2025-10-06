#pragma once

#include <Arduino.h>

bool startWiFiWithPortal();
bool saveWiFiCredentials(const String &ssid, const String &password);
bool loadWiFiCredentials(String &ssid, String &password);
void forgetWiFiCredentials();
bool isPortalActive();
void handleWiFiPortal();

