#include "wifi_portal.h"
#include "config.h"
#include <WiFi.h>

void startWiFiWithPortal() {
  WiFi.mode(WIFI_AP);
  bool result = WiFi.softAP(FALLBACK_AP_SSID, FALLBACK_AP_PASS);

  if (!result) {
    Serial.println("❌ Échec de démarrage du point d’accès");
    return;
  }

  Serial.println("📶 Point d’accès actif");
  Serial.print("Nom : ");
  Serial.println(FALLBACK_AP_SSID);
  Serial.print("IP locale : ");
  Serial.println(WiFi.softAPIP());
}
