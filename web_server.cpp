#include "web_server.h"

#ifndef __has_include
#define __has_include(x) 0
#define STAGECUE_ASSUME_FS_HEADERS 1
#endif

#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#if __has_include(<LittleFS.h>) || defined(STAGECUE_ASSUME_FS_HEADERS)
#include <LittleFS.h>
#define STAGECUE_HAS_LITTLEFS 1
#else
#define STAGECUE_HAS_LITTLEFS 0
#endif
#if __has_include(<SPIFFS.h>) || defined(STAGECUE_ASSUME_FS_HEADERS)
#include <SPIFFS.h>
#define STAGECUE_HAS_SPIFFS 1
#else
#define STAGECUE_HAS_SPIFFS 0
#endif
#include <WiFi.h>
#include <esp_system.h>

#include "config.h"
#include "cues.h"
#include "display_manager.h"
#include "wifi_portal.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

namespace {

bool fsMounted = false;
fs::FS *activeFs = nullptr;

bool mountFileSystem() {
  if (fsMounted) {
    return true;
  }

#if STAGECUE_HAS_LITTLEFS
  if (LittleFS.begin(false)) {
    activeFs = &LittleFS;
    fsMounted = true;
    Serial.println("[FS] ✅ LittleFS monté");
    return true;
  }

  Serial.println("[FS] ⚠️ Échec de montage de LittleFS, tentative de formatage...");
  if (LittleFS.format() && LittleFS.begin(false)) {
    activeFs = &LittleFS;
    fsMounted = true;
    Serial.println("[FS] ✅ LittleFS formaté et monté");
    return true;
  }
#endif

#if STAGECUE_HAS_SPIFFS
  Serial.println("[FS] ℹ️ Bascule vers SPIFFS");
  if (SPIFFS.begin(false)) {
    activeFs = &SPIFFS;
    fsMounted = true;
    Serial.println("[FS] ✅ SPIFFS monté");
    return true;
  }

  Serial.println("[FS] ⚠️ Échec de montage de SPIFFS, tentative de formatage...");
  if (SPIFFS.format() && SPIFFS.begin(false)) {
    activeFs = &SPIFFS;
    fsMounted = true;
    Serial.println("[FS] ✅ SPIFFS formaté et monté");
    return true;
  }
#endif

  Serial.println("[FS] ❌ Aucun système de fichiers disponible");
  return false;
}

bool authTokenMatches(const String &token) {
  if (strlen(API_AUTH_TOKEN) == 0) {
    return true;
  }
  return token == API_AUTH_TOKEN;
}

bool isAuthorized(AsyncWebServerRequest *request) {
  if (strlen(API_AUTH_TOKEN) == 0) {
    return true;
  }

  if (request->hasHeader("X-StageCue-Token")) {
    const auto header = request->getHeader("X-StageCue-Token");
    if (authTokenMatches(header->value())) {
      return true;
    }
  }

  if (request->hasHeader("Authorization")) {
    const auto header = request->getHeader("Authorization");
    String value = header->value();
    value.trim();
    if (value.startsWith("Bearer ")) {
      String token = value.substring(7);
      token.trim();
      if (authTokenMatches(token)) {
        return true;
      }
    }
  }

  if (request->hasParam("token")) {
    if (authTokenMatches(request->getParam("token")->value())) {
      return true;
    }
  }

  return false;
}

void sendUnauthorized(AsyncWebServerRequest *request) {
  request->send(401, "application/json", "{\"error\":\"unauthorized\"}");
}

bool requireAuth(AsyncWebServerRequest *request) {
  if (isAuthorized(request)) {
    return true;
  }
  sendUnauthorized(request);
  return false;
}

String urlDecode(const String &text);

bool validateWebSocketClient(AsyncWebSocketClient *client) {
  if (strlen(API_AUTH_TOKEN) == 0) {
    return true;
  }

  String url = client->url();
  int queryIndex = url.indexOf('?');
  if (queryIndex < 0) {
    return false;
  }

  String query = url.substring(queryIndex + 1);
  while (!query.isEmpty()) {
    int amp = query.indexOf('&');
    String pair = amp >= 0 ? query.substring(0, amp) : query;
    int eq = pair.indexOf('=');
    if (eq >= 0) {
      String key = pair.substring(0, eq);
      String value = pair.substring(eq + 1);
      key.trim();
      value.trim();
      if (key == "token" && authTokenMatches(urlDecode(value))) {
        return true;
      }
    }
    if (amp < 0) {
      break;
    }
    query.remove(0, amp + 1);
  }

  return false;
}

String urlDecode(const String &text) {
  String decoded;
  decoded.reserve(text.length());
  for (size_t i = 0; i < text.length(); ++i) {
    char c = text[i];
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%' && i + 2 < text.length()) {
      char hex[3] = {text[i + 1], text[i + 2], '\0'};
      decoded += static_cast<char>(strtoul(hex, nullptr, 16));
      i += 2;
    } else {
      decoded += c;
    }
  }
  return decoded;
}

String wifiStatusToString(wl_status_t status) {
  switch (status) {
    case WL_CONNECTED:
      return F("connected");
    case WL_NO_SHIELD:
      return F("no_shield");
    case WL_IDLE_STATUS:
      return F("idle");
    case WL_DISCONNECTED:
      return F("disconnected");
    case WL_CONNECTION_LOST:
      return F("connection_lost");
    case WL_NO_SSID_AVAIL:
      return F("ssid_unavailable");
    case WL_SCAN_COMPLETED:
      return F("scan_completed");
    default:
      return F("unknown");
  }
}

void sendJson(AsyncWebServerRequest *request, uint16_t statusCode, const JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  request->send(statusCode, "application/json", body);
}

void handleTriggerRequest(AsyncWebServerRequest *request) {
  if (!request->hasParam("cue", true)) {
    request->send(400, "application/json", "{\"error\":\"missing_cue\"}");
    return;
  }

  int cueIndex = request->getParam("cue", true)->value().toInt();
  if (cueIndex < 0 || static_cast<size_t>(cueIndex) >= CUE_COUNT) {
    request->send(400, "application/json", "{\"error\":\"invalid_cue\"}");
    return;
  }

  if (request->hasParam("text", true)) {
    setCueText(static_cast<size_t>(cueIndex), request->getParam("text", true)->value(), true);
  }

  triggerCue(static_cast<size_t>(cueIndex));
  request->send(200, "application/json", buildCueStateJson(static_cast<size_t>(cueIndex)));
}

}  // namespace

// 🎯 WebSocket: gestion des événements
void onWebSocketEvent(AsyncWebSocket *serverPtr, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT: {
      if (!validateWebSocketClient(client)) {
        Serial.printf("[WS] 🔐 Rejet de la connexion #%u (token invalide)\n", client->id());
        client->close(1008);
        return;
      }
      Serial.printf("[WS] 🔌 Client #%u connecté\n", client->id());
      client->text(buildCueSnapshotJson());
      break;
    }
    case WS_EVT_DISCONNECT:
      Serial.printf("[WS] ❌ Client #%u déconnecté\n", client->id());
      break;
    case WS_EVT_DATA: {
      AwsFrameInfo *info = reinterpret_cast<AwsFrameInfo *>(arg);
      if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)) {
        return;
      }

      String message(reinterpret_cast<const char *>(data), len);
      StaticJsonDocument<384> doc;
      DeserializationError err = deserializeJson(doc, message);
      if (err) {
        Serial.printf("[WS] ❗ JSON invalide reçu: %s\n", message.c_str());
        StaticJsonDocument<64> errorDoc;
        errorDoc["type"] = "error";
        errorDoc["message"] = "invalid_json";
        String response;
        serializeJson(errorDoc, response);
        client->text(response);
        return;
      }

      const String action = doc["type"] | String("trigger");
      const int cueIndex = doc.containsKey("cue") ? doc["cue"].as<int>() : doc["index"].as<int>();
      if (cueIndex < 0 || static_cast<size_t>(cueIndex) >= CUE_COUNT) {
        StaticJsonDocument<96> errorDoc;
        errorDoc["type"] = "error";
        errorDoc["message"] = "invalid_cue";
        String response;
        serializeJson(errorDoc, response);
        client->text(response);
        return;
      }

      const bool persist = doc["persist"] | true;

      if (doc.containsKey("text")) {
        setCueText(static_cast<size_t>(cueIndex), String(doc["text"].as<const char *>()), persist);
      }

      if (action == "setText") {
        client->text(buildCueStateJson(static_cast<size_t>(cueIndex)));
      } else if (action == "trigger") {
        triggerCue(static_cast<size_t>(cueIndex));
      } else if (action == "ping") {
        StaticJsonDocument<64> pongDoc;
        pongDoc["type"] = "pong";
        String response;
        serializeJson(pongDoc, response);
        client->text(response);
      }
      break;
    }
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
    default:
      break;
  }
}

void startWebServer() {
  if (!mountFileSystem()) {
    Serial.println("[Web] ⚠️ Lancement du serveur sans fichiers statiques");
  }

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  if (fsMounted && activeFs != nullptr) {
    auto &rootHandler = server.serveStatic("/", *activeFs, "/");
    rootHandler.setDefaultFile("index.html");
    rootHandler.setCacheControl("max-age=300, must-revalidate");

    auto &wifiHandler = server.serveStatic("/wifi", *activeFs, "/wifi.html");
    wifiHandler.setCacheControl("max-age=60");
  } else {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(503, "text/plain", "Interface non disponible : téléversez les fichiers SPIFFS/LittleFS");
    });
  }

  server.on("/trigger", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!requireAuth(request)) {
      return;
    }
    handleTriggerRequest(request);
  });

  server.on("/api/cues", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireAuth(request)) {
      return;
    }
    request->send(200, "application/json", buildCueSnapshotJson());
  });

  server.on("/api/cues/trigger", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!requireAuth(request)) {
      return;
    }
    handleTriggerRequest(request);
  });

  server.on("/api/cues/text", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!requireAuth(request)) {
      return;
    }
    if (!request->hasParam("cue", true) || !request->hasParam("text", true)) {
      request->send(400, "application/json", "{\"error\":\"missing_parameters\"}");
      return;
    }
    int cueIndex = request->getParam("cue", true)->value().toInt();
    if (cueIndex < 0 || static_cast<size_t>(cueIndex) >= CUE_COUNT) {
      request->send(400, "application/json", "{\"error\":\"invalid_cue\"}");
      return;
    }

    setCueText(static_cast<size_t>(cueIndex), request->getParam("text", true)->value(), true);
    request->send(200, "application/json", buildCueStateJson(static_cast<size_t>(cueIndex)));
  });

  server.on("/api/health", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireAuth(request)) {
      return;
    }
    StaticJsonDocument<256> doc;
    doc["device"] = DEVICE_NAME;
    doc["wifiStatus"] = wifiStatusToString(WiFi.status());
    doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String();
    doc["ssid"] = WiFi.SSID();
    doc["portalActive"] = isPortalActive();
    doc["uptimeMs"] = millis();
    sendJson(request, 200, doc);
  });

  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isPortalActive() && !requireAuth(request)) {
      return;
    }
    int n = WiFi.scanNetworks();
    StaticJsonDocument<512> doc;
    JsonArray array = doc.to<JsonArray>();
    for (int i = 0; i < n; ++i) {
      JsonObject network = array.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    sendJson(request, 200, doc);
  });

  server.on("/save_wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!isPortalActive() && !requireAuth(request)) {
      return;
    }

    if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
      request->send(400, "text/plain", "Paramètres manquants");
      return;
    }

    String ssid = urlDecode(request->getParam("ssid", true)->value());
    String password = urlDecode(request->getParam("password", true)->value());

    if (ssid.isEmpty()) {
      request->send(400, "text/plain", "SSID invalide");
      return;
    }

    if (!saveWiFiCredentials(ssid, password)) {
      request->send(500, "text/plain", "Échec de sauvegarde des identifiants");
      return;
    }

    request->send(200, "text/plain", "Identifiants sauvegardés. Redémarrage...");
    request->onDisconnect([]() { ESP.restart(); });
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (fsMounted) {
      request->send(404, "application/json", "{\"error\":\"not_found\"}");
    } else {
      request->send(503, "application/json", "{\"error\":\"filesystem_unavailable\"}");
    }
  });

  ws.onEvent(onWebSocketEvent);
  ws.enable(true);
  server.addHandler(&ws);

  server.begin();
  Serial.println("[Web] ✅ Serveur Web démarré sur le port 80");
}
