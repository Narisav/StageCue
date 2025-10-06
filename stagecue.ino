#include "config.h"
#include "display_manager.h"
#include "web_server.h"
#include "cues.h"
#include "wifi_portal.h"

void setup() {
  Serial.begin(115200);
  initDisplay();
  initCues();
  // connectToWiFi();
  startWiFiWithPortal();  // à la place de connectToWiFi()
  startWebServer();
}

void loop() {
  updateCues();
}