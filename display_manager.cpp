#include "display_manager.h"

#include "config.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

namespace {

Adafruit_SSD1306 displays[CUE_COUNT] = {
    Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1),
    Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1),
    Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1)};

struct DisplayState {
  bool ready = false;
  uint8_t address = 0;
};

DisplayState states[CUE_COUNT];

String sanitizeText(const String &raw) {
  String sanitized = raw;
  sanitized.trim();
  if (sanitized.isEmpty()) {
    sanitized = F("(vide)");
  }
  if (sanitized.length() > MAX_CUE_TEXT_LENGTH) {
    sanitized.remove(MAX_CUE_TEXT_LENGTH);
  }
  return sanitized;
}

void renderWrappedText(Adafruit_SSD1306 &display, const String &text) {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  constexpr uint8_t lineHeight = 8;  // Taille d'une ligne en pixels pour TextSize=1.
  uint8_t currentLine = 0;
  const uint8_t maxLines = SCREEN_HEIGHT / lineHeight;

  int start = 0;
  while (start < text.length() && currentLine < maxLines) {
    int newlineIndex = text.indexOf('\n', start);
    int end = newlineIndex >= 0 ? newlineIndex : text.length();
    String line = text.substring(start, end);

    while (!line.isEmpty() && currentLine < maxLines) {
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(line, 0, 0, &x1, &y1, &w, &h);

      if (w <= SCREEN_WIDTH) {
        display.setCursor(0, currentLine * lineHeight);
        display.print(line);
        currentLine++;
        break;
      }

      // On coupe proprement la ligne pour tenir sur l'écran.
      int breakIndex = line.length();
      while (breakIndex > 0) {
        String candidate = line.substring(0, breakIndex);
        display.getTextBounds(candidate, 0, 0, &x1, &y1, &w, &h);
        if (w <= SCREEN_WIDTH) {
          display.setCursor(0, currentLine * lineHeight);
          display.print(candidate);
          currentLine++;
          line.remove(0, breakIndex);
          line.trim();
          break;
        }
        breakIndex--;
      }

      if (breakIndex == 0) {
        // Aucun découpage ne rentre -> on coupe brutalement sur la largeur.
        display.setCursor(0, currentLine * lineHeight);
        display.print(line.substring(0, SCREEN_WIDTH / 6));
        line.remove(0, SCREEN_WIDTH / 6);
        currentLine++;
      }
    }

    start = end + 1;
  }

  display.display();
}

}  // namespace

void initDisplay() {
  Wire.begin();

  for (size_t i = 0; i < CUE_COUNT; ++i) {
    states[i].address = displayAddresses[i];

    if (!displays[i].begin(SSD1306_SWITCHCAPVCC, states[i].address)) {
      Serial.printf("[Display] ❌ Aucun écran détecté à l'adresse 0x%02X\n", states[i].address);
      states[i].ready = false;
      continue;
    }

    displays[i].setRotation(0);
    displays[i].clearDisplay();
    displays[i].display();
    states[i].ready = true;
    Serial.printf("[Display] ✅ Écran #%u initialisé (0x%02X)\n", static_cast<unsigned>(i), states[i].address);
  }
}

void updateDisplay(size_t index, const String &text) {
  if (index >= CUE_COUNT) {
    return;
  }

  if (!states[index].ready) {
    Serial.printf("[Display] ⚠️ Écran #%u indisponible, impossible d'afficher le texte.\n",
                  static_cast<unsigned>(index));
    return;
  }

  renderWrappedText(displays[index], sanitizeText(text));
}

bool isDisplayReady(size_t index) {
  if (index >= CUE_COUNT) {
    return false;
  }
  return states[index].ready;
}