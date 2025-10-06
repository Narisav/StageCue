# StageCue

StageCue est un prototype d'assistant de régie basé sur un ESP32. Il offre une interface web légère pour envoyer des "cues" (ordres lumineux) vers trois afficheurs OLED et leurs LED associées. L'appareil peut créer son propre point d'accès Wi-Fi ou se connecter à un réseau existant, puis servir l'application web embarquée via LittleFS.

## Fonctionnalités principales

- **Pilotage multi-cues** : trois pistes configurables (extensibles) avec texte personnalisé et rétroaction visuelle.
- **Affichage OLED** : chaque cue dispose de son propre écran SSD1306 (128×64) géré par `Adafruit_SSD1306`.
- **LED de confirmation** : activation d'une LED par cue lors du déclenchement.
- **Interface Web temps réel** : communication WebSocket pour envoyer les cues et recevoir les retours.
- **Portail Wi-Fi** : lancement d'un point d'accès de secours pour configurer l'appareil sans réseau existant.

## Organisation du code

| Répertoire/Fichier | Rôle |
| --- | --- |
| `stagecue.ino` | Point d'entrée Arduino : initialisation de l'écran, des cues, du Wi-Fi et du serveur web. |
| `config.h/.cpp` | Paramètres globaux : identifiants Wi-Fi, tailles d'écran, textes par défaut, numéros de broches. |
| `display_manager.*` | Initialisation et mise à jour des afficheurs OLED multiples. |
| `cues.*` | Gestion des états des cues, logique d'activation et diffusion des événements WebSocket. |
| `web_server.*` | Configuration du serveur HTTP/WS asynchrone (`ESPAsyncWebServer`) et parsing JSON. |
| `wifi_portal.*` | Démarrage d'un point d'accès Wi-Fi en mode portail captif simplifié. |
| `data/` | Fichiers front-end (HTML, CSS, JS) servis depuis LittleFS, y compris la page principale et une page de configuration Wi-Fi. |

## Déroulement typique

1. **Boot** : `stagecue.ino` lance la console série, initialise les écrans, charge les textes par défaut puis active le mode point d'accès via `startWiFiWithPortal()`.
2. **Serveur web** : `startWebServer()` monte LittleFS, sert `index.html` et attache le WebSocket `/ws`.
3. **Interface utilisateur** : la page `data/index.html` se connecte au WebSocket et envoie des objets JSON `{cue, text}` via `data/app.js`.
4. **Traitement des cues** : `onWebSocketEvent()` parse le JSON, met à jour `cueTexts` et appelle `triggerCue()`.
5. **Feedback** : `triggerCue()` allume la LED, met à jour l'écran concerné et renvoie un message `cueTrigger:<index>` à tous les clients pour surligner temporairement la carte cue.

## Notes matérielles

- Les broches de LED sont déclarées dans `config.h` (`25, 26, 27`) et doivent être adaptées au câblage réel.
- Les boutons prévus (`32, 33, 34`) ne sont pas encore exploités dans `updateCues()`, mais peuvent servir pour déclencher localement.
- Les adresses I2C des écrans sont calculées à partir de `OLED_ADDRESS` (0x3C, 0x3D, 0x3E...). Veillez à utiliser des modules avec adresses configurables.

## Pistes d'amélioration

- Persistance des textes personnalisés (via `Preferences` ou un stockage JSON sur LittleFS).
- Gestion des boutons physiques dans `updateCues()`.
- Portail captif complet avec formulaire `wifi.html` exposé sur le point d'accès.
- Interface web responsive et adaptée aux tablettes de régie.

## Développement

1. Installer les dépendances Arduino/PlatformIO nécessaires :
   - `ESPAsyncWebServer`, `AsyncTCP`, `Adafruit_GFX`, `Adafruit_SSD1306`, `ArduinoJson`, `Preferences`.
2. Téléverser les fichiers statiques (`data/`) sur LittleFS avant flash (`ESP32 Sketch Data Upload`).
3. Compiler et téléverser le sketch sur un module ESP32.

Connectez-vous ensuite au point d'accès `CueLight_AP` (mot de passe `12345678`) puis ouvrez l'adresse IP affichée dans la console série pour accéder à l'interface.
