#include "cues.h"

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#include "config.h"
#include "display_manager.h"

extern AsyncWebSocket ws;

namespace {

struct CueState {
  bool active = false;
  uint32_t triggeredAt = 0;
};

CueState states[CUE_COUNT];
Preferences cuePrefs;
bool prefsReady = false;

int lastButtonReading[CUE_COUNT] = {0};
int stableButtonState[CUE_COUNT] = {0};
uint32_t lastDebounceTimestamp[CUE_COUNT] = {0};
bool buttonConfigured[CUE_COUNT] = {false};

uint32_t lastCleanup = 0;

constexpr const char *kCuePrefsNamespace = "cue_texts";

void ensurePreferences() {
  if (!prefsReady) {
    prefsReady = cuePrefs.begin(kCuePrefsNamespace, false);
    if (!prefsReady) {
      Serial.println("[Cue] ⚠️ Impossible d'ouvrir l'espace de préférences 'cue_texts'.");
    }
  }
}

String sanitizeCueText(const String &raw) {
  String sanitized = raw;
  sanitized.trim();
  if (sanitized.length() > MAX_CUE_TEXT_LENGTH) {
    sanitized.remove(MAX_CUE_TEXT_LENGTH);
  }
  return sanitized;
}

void persistCueText(size_t index) {
  if (!prefsReady) {
    ensurePreferences();
  }
  if (!prefsReady || index >= CUE_COUNT) {
    return;
  }

  const String key = "cue" + String(index);
  if (!cuePrefs.putString(key.c_str(), cueTexts[index])) {
    Serial.printf("[Cue] ⚠️ Échec d'écriture de la préférence %s.\n", key.c_str());
  }
}

void configureButton(size_t index) {
  if (cueButtons[index] < 0) {
    buttonConfigured[index] = false;
    return;
  }

  if (BUTTON_USE_PULLUP) {
    pinMode(cueButtons[index], INPUT_PULLUP);
  } else {
    pinMode(cueButtons[index], INPUT);
  }

  int current = digitalRead(cueButtons[index]);
  lastButtonReading[index] = current;
  stableButtonState[index] = current;
  buttonConfigured[index] = true;
}

void updateLedState(size_t index, bool active) {
  digitalWrite(cueLEDs[index], active ? HIGH : LOW);
}

String buildCueJson(size_t index, const char *type) {
  StaticJsonDocument<256> doc;
  doc["type"] = type;
  doc["index"] = static_cast<uint8_t>(index);
  doc["active"] = states[index].active;
  doc["text"] = cueTexts[index];
  doc["displayReady"] = isDisplayReady(index);

  String payload;
  serializeJson(doc, payload);
  return payload;
}

}  // namespace

String cueTexts[CUE_COUNT];

void initCues() {
  ensurePreferences();

  for (size_t i = 0; i < CUE_COUNT; ++i) {
    pinMode(cueLEDs[i], OUTPUT);
    updateLedState(i, false);

    cueTexts[i].reserve(MAX_CUE_TEXT_LENGTH + 1);
    if (prefsReady) {
      const String key = "cue" + String(i);
      const String stored = cuePrefs.getString(key.c_str(), defaultCueTexts[i]);
      cueTexts[i] = sanitizeCueText(stored);
      if (cueTexts[i].isEmpty()) {
        cueTexts[i] = defaultCueTexts[i];
      }
    } else {
      cueTexts[i] = defaultCueTexts[i];
    }

    configureButton(i);
    updateDisplay(i, cueTexts[i]);
  }
}

void updateCues() {
  const uint32_t now = millis();

  for (size_t i = 0; i < CUE_COUNT; ++i) {
    if (states[i].active && (now - states[i].triggeredAt >= CUE_ACTIVE_DURATION_MS)) {
      states[i].active = false;
      updateLedState(i, false);
      ws.textAll(buildCueStateJson(i));
    }

    if (!buttonConfigured[i]) {
      continue;
    }

    const int reading = digitalRead(cueButtons[i]);
    if (reading != lastButtonReading[i]) {
      lastDebounceTimestamp[i] = now;
      lastButtonReading[i] = reading;
    }

    if ((now - lastDebounceTimestamp[i]) >= BUTTON_DEBOUNCE_MS && reading != stableButtonState[i]) {
      stableButtonState[i] = reading;
      if (stableButtonState[i] == BUTTON_ACTIVE_STATE) {
        triggerCue(i);
      }
    }
  }

  if (now - lastCleanup >= WS_CLIENT_CLEANUP_INTERVAL_MS) {
    ws.cleanupClients();
    lastCleanup = now;
  }
}

void setCueText(size_t index, const String &text, bool persist) {
  if (index >= CUE_COUNT) {
    return;
  }

  String sanitized = sanitizeCueText(text);
  if (sanitized.isEmpty()) {
    sanitized = defaultCueTexts[index];
  }

  if (cueTexts[index] == sanitized) {
    if (persist) {
      persistCueText(index);
    }
    return;
  }

  cueTexts[index] = sanitized;

  if (persist) {
    persistCueText(index);
  }

  updateDisplay(index, cueTexts[index]);
  ws.textAll(buildCueStateJson(index));
}

void triggerCue(size_t index) {
  if (index >= CUE_COUNT) {
    return;
  }

  setCueText(index, cueTexts[index], false);

  states[index].active = true;
  states[index].triggeredAt = millis();
  updateLedState(index, true);
  updateDisplay(index, cueTexts[index]);

  ws.textAll(buildCueStateJson(index));
}

bool isCueActive(size_t index) {
  if (index >= CUE_COUNT) {
    return false;
  }
  return states[index].active;
}

String buildCueSnapshotJson() {
  StaticJsonDocument<512> doc;
  doc["type"] = "snapshot";
  JsonArray cues = doc.createNestedArray("cues");
  for (size_t i = 0; i < CUE_COUNT; ++i) {
    JsonObject entry = cues.createNestedObject();
    entry["index"] = static_cast<uint8_t>(i);
    entry["text"] = cueTexts[i];
    entry["active"] = states[i].active;
    entry["displayReady"] = isDisplayReady(i);
  }

  String payload;
  serializeJson(doc, payload);
  return payload;
}

String buildCueStateJson(size_t index) {
  if (index >= CUE_COUNT) {
    return String();
  }
  return buildCueJson(index, "cue");
}