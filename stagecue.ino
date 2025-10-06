#include "config.h"
#include "display_manager.h"
#include "web_server.h"
#include "cues.h"
#include "wifi_portal.h"

void setup() {
  Serial.begin(115200);
  initDisplay();
  initCues();
  const bool wifiConnected = startWiFiWithPortal();
  if (wifiConnected) {
    Serial.println("[Core] ✅ Connecté au Wi-Fi");
  } else {
    Serial.println("[Core] ⚠️ Portail Wi-Fi actif");
  }
  startWebServer();
}

void loop() {
  updateCues();
  handleWiFiPortal();
}