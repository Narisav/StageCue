#include "wifi_portal.h"

#include <DNSServer.h>
#include <Preferences.h>
#include <WiFi.h>
#if defined(ESP_PLATFORM)
#include <esp_wifi.h>
#endif

#include "config.h"

namespace {

Preferences wifiPrefs;
bool prefsReady = false;
bool portalModeActive = false;
IPAddress portalIp;
DNSServer dnsServer;

constexpr const char *kPrefsNamespace = "wifi_cfg";
constexpr const char *kSsidKey = "ssid";
constexpr const char *kPassKey = "pass";
constexpr uint8_t kDnsPort = 53;

bool ensurePrefs() {
  if (!prefsReady) {
    prefsReady = wifiPrefs.begin(kPrefsNamespace, false);
    if (!prefsReady) {
      Serial.println("[WiFi] ❌ Impossible d'ouvrir les préférences Wi-Fi");
    }
  }
  return prefsReady;
}

void stopPortal() {
  if (portalModeActive) {
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    portalModeActive = false;
    Serial.println("[WiFi] 📴 Portail captif désactivé");
  }
}

void applyWiFiPowerSettings() {
  if (WIFI_DISABLE_SLEEP) {
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(ARDUINO_ESP32C6_DEVKITC) || defined(ARDUINO_ESP32C6_DEV)
#if defined(WIFI_POWER_19_5dBm)
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
#endif
#endif
    WiFi.setSleep(false);
#if defined(ESP_PLATFORM)
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif
  }
}

bool connectToNetwork(const String &ssid, const String &password) {
  if (ssid.isEmpty()) {
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setHostname(DEVICE_NAME);
  applyWiFiPowerSettings();

  Serial.printf("[WiFi] 🔌 Connexion au réseau '%s'\n", ssid.c_str());

  for (uint8_t attempt = 0; attempt < WIFI_MAX_RETRIES; ++attempt) {
    WiFi.disconnect(true, true);
    delay(50);
    WiFi.begin(ssid.c_str(), password.isEmpty() ? nullptr : password.c_str());

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
      delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WiFi] ✅ Connecté à '%s' (%s)\n", ssid.c_str(), WiFi.localIP().toString().c_str());
      stopPortal();
      return true;
    }

    Serial.printf("[WiFi] ⚠️ Tentative %u échouée\n", static_cast<unsigned>(attempt + 1));
    WiFi.disconnect(true);
    delay(200);
  }

  Serial.println("[WiFi] ❌ Impossible de se connecter au réseau enregistré");
  return false;
}

void startPortalMode() {
  WiFi.mode(WIFI_AP_STA);
  applyWiFiPowerSettings();

  IPAddress apIp(192, 168, 4, 1);
  IPAddress netMask(255, 255, 255, 0);
  IPAddress gateway = apIp;
  WiFi.softAPConfig(apIp, gateway, netMask);

  if (!WiFi.softAP(FALLBACK_AP_SSID, FALLBACK_AP_PASS)) {
    Serial.println("[WiFi] ❌ Échec d'activation du point d'accès sécurisé, tentative sans mot de passe");
    if (!WiFi.softAP(FALLBACK_AP_SSID)) {
      Serial.println("[WiFi] ❌ Impossible de démarrer le point d'accès");
      return;
    }
  }

  portalIp = WiFi.softAPIP();
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(kDnsPort, "*", portalIp);
  portalModeActive = true;

  Serial.println("[WiFi] 📶 Portail captif actif");
  Serial.printf("[WiFi]    SSID : %s\n", FALLBACK_AP_SSID);
  Serial.printf("[WiFi]    IP   : %s\n", portalIp.toString().c_str());
}

}  // namespace

bool startWiFiWithPortal() {
  String ssid;
  String password;
  bool connected = false;

  if (ensurePrefs() && loadWiFiCredentials(ssid, password) && !ssid.isEmpty()) {
    connected = connectToNetwork(ssid, password);
  }

  if (!connected) {
    startPortalMode();
  }

  return connected;
}

bool saveWiFiCredentials(const String &ssid, const String &password) {
  if (!ensurePrefs()) {
    return false;
  }

  bool ok = wifiPrefs.putString(kSsidKey, ssid) && wifiPrefs.putString(kPassKey, password);
  if (ok) {
    Serial.printf("[WiFi] 💾 Identifiants enregistrés pour '%s'\n", ssid.c_str());
  }
  return ok;
}

bool loadWiFiCredentials(String &ssid, String &password) {
  if (!ensurePrefs()) {
    return false;
  }

  ssid = wifiPrefs.getString(kSsidKey, "");
  password = wifiPrefs.getString(kPassKey, "");
  return !ssid.isEmpty();
}

void forgetWiFiCredentials() {
  if (!ensurePrefs()) {
    return;
  }
  wifiPrefs.remove(kSsidKey);
  wifiPrefs.remove(kPassKey);
  Serial.println("[WiFi] 🧹 Identifiants oubliés");
}

bool isPortalActive() {
  return portalModeActive;
}

void handleWiFiPortal() {
  if (portalModeActive) {
    dnsServer.processNextRequest();
  }
}
