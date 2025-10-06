#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Réseaux Wi-Fi
// -----------------------------------------------------------------------------
// Identifiants utilisés lorsque l'appareil agit comme station (mode STA).
#define WIFI_SSID "TON_SSID"
#define WIFI_PASSWORD "TON_PASSWORD"

// Identifiants du point d'accès de secours (mode portail captif).
#define FALLBACK_AP_SSID "CueLight_AP"
#define FALLBACK_AP_PASS "12345678"

// Délai maximal d'attente d'une connexion Wi-Fi (par tentative).
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
// Nombre maximal de tentatives complètes avant de basculer en mode portail.
constexpr uint8_t WIFI_MAX_RETRIES = 3;

// -----------------------------------------------------------------------------
// Authentification API & WebSocket
// -----------------------------------------------------------------------------
extern const char API_AUTH_TOKEN[];  // Jeton partagé entre le firmware et le front-end.
extern const char DEVICE_NAME[];     // Nom réseau annoncé par l'appareil.

// -----------------------------------------------------------------------------
// Gestion des écrans OLED
// -----------------------------------------------------------------------------
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS = 0x3C;

constexpr size_t CUE_COUNT = 3;

extern const uint8_t displayAddresses[CUE_COUNT];

// -----------------------------------------------------------------------------
// Gestion des cues
// -----------------------------------------------------------------------------
extern const char *defaultCueTexts[CUE_COUNT];

// Durée pendant laquelle la LED d'un cue reste allumée (ms).
constexpr uint32_t CUE_ACTIVE_DURATION_MS = 5000;
// Taille maximale des textes affichés (caractères Unicode).
constexpr size_t MAX_CUE_TEXT_LENGTH = 64;
// Anti-rebond matériel des boutons (ms).
constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;
// Niveau logique considéré comme "appuyé".
constexpr uint8_t BUTTON_ACTIVE_STATE = LOW;
// Active la résistance de pull-up interne lorsque c'est possible.
constexpr bool BUTTON_USE_PULLUP = true;

// Intervalle minimal entre deux opérations de nettoyage des clients WebSocket.
constexpr uint32_t WS_CLIENT_CLEANUP_INTERVAL_MS = 10000;

// -----------------------------------------------------------------------------
// Brochages matériels (adapter si nécessaire)
// -----------------------------------------------------------------------------
extern const int cueLEDs[CUE_COUNT];
extern const int cueButtons[CUE_COUNT];
