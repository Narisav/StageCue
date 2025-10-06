#include "config.h"
#include "display_manager.h"
#include "web_server.h"
#include "cues.h"
#include "wifi_portal.h"

#if defined(ESP_PLATFORM)
#include <esp_chip_info.h>
#include <esp_flash.h>
#endif

namespace {

void logChipInfo() {
#if defined(ESP_PLATFORM)
  esp_chip_info_t info;
  esp_chip_info(&info);

  uint32_t flashSize = 0;
  if (esp_flash_get_size(nullptr, &flashSize) != ESP_OK) {
    flashSize = 0;
  }

  Serial.printf("[Core] 🔎 SoC ESP32 (rév.%d) • %d CPU(s) • Wi-Fi%s%s • %lu KB de flash\n", info.revision,
                info.cores, (info.features & CHIP_FEATURE_WIFI_BGN) ? "/802.11 b/g/n" : "",
                (info.features & CHIP_FEATURE_BT) ? " + Bluetooth" : "",
                static_cast<unsigned long>(flashSize / 1024));
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(ARDUINO_ESP32C6_DEVKITC) || defined(ARDUINO_ESP32C6_DEV)
  Serial.println("[Core] ✅ Compatibilité ESP32-C6 vérifiée");
#endif
#else
  Serial.println("[Core] 🔎 Plateforme Arduino détectée");
#endif
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(100);
  logChipInfo();
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
