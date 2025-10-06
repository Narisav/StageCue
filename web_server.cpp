#include "web_server.h"
#include "cues.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// 🎯 WebSocket: gestion des événements
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("🔌 Client WebSocket connecté: #%u\n", client->id());
    return;
  }

  if (type == WS_EVT_DISCONNECT) {
    Serial.printf("❌ Client WebSocket déconnecté: #%u\n", client->id());
    return;
  }

  if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0; // sécurise le buffer en string C

      Serial.printf("📨 WS Message: %s\n", (char*)data);

      // Parse JSON venant du navigateur
      DynamicJsonDocument doc(256);
      DeserializationError err = deserializeJson(doc, (char*)data);
      if (!err) {
        if (doc.containsKey("cue") && doc.containsKey("text")) {
          int cue = doc["cue"];
          const char* text = doc["text"];

          if (cue >= 0 && cue < CUE_COUNT) {
            cueTexts[cue] = String(text);
            triggerCue(cue);
            ws.textAll("cueTrigger:" + String(cue));
          }
        }
      } else {
        Serial.println("❗ Erreur parsing JSON WebSocket");
      }
    }
  }
}

void startWebServer() {
  if (!LittleFS.begin()) {
    Serial.println("❌ Erreur de montage LittleFS");
    return;
  }

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // Route HTTP (facultative maintenant que WebSocket est là)
  server.on("/trigger", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("cue", true)) {
      int cueIndex = request->getParam("cue", true)->value().toInt();
      triggerCue(cueIndex);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Missing cue param");
    }
  });

  // 🔗 Active le WebSocket
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.begin();
  Serial.println("✅ Serveur Web démarré sur le port 80 !");
}
