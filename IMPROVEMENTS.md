# StageCue – Analyse et pistes d'amélioration pour une version commercialisable

## 1. Vue d'ensemble du projet
Le firmware Arduino `stagecue.ino` initialise l'afficheur, les "cues", le portail Wi-Fi et un serveur Web destiné à piloter des boîtiers lumineux. L'interface Web (dossiers `data/index.html`, `data/app.js`, `data/style.css`) utilise un WebSocket (`/ws`) fourni par `web_server.cpp` pour envoyer un texte et déclencher un cue. La configuration Wi-Fi repose sur un point d'accès local (`wifi_portal.cpp`). Les textes par défaut et le matériel (pins, écrans) sont configurés dans `config.h`.

Cette base démontre le prototype mais il manque de nombreuses garanties pour une commercialisation : robustesse, sécurité, gestion mémoire, UX, tests et conformité industrielle.

## 2. Fiabilité et robustesse du firmware
- **Gestion des écrans OLED** : `display_manager.cpp` suppose trois écrans OLED avec des adresses consécutives (`OLED_ADDRESS + i`). Il faut confirmer que le matériel supporte cette incrémentation, ou gérer les échecs `begin()` en réaffectant des adresses I²C uniques et en vérifiant la présence matérielle avant de continuer. 【F:display_manager.cpp†L15-L29】
- **Gestion des états LED** : `triggerCue` active uniquement `digitalWrite(cueLEDs[index], HIGH)` sans jamais réinitialiser l'état. Prévoir un minuteur, un retour à LOW ou un pilotage PWM pour éviter une LED bloquée en mode "cue actif". 【F:cues.cpp†L23-L31】
- **Boucle `updateCues` vide** : aucun rafraîchissement des entrées utilisateur ou de logique interne n'est exécuté. Ajoutez la lecture des boutons (`cueButtons`) avec anti-rebond, la supervision de timeouts et les appels `ws.cleanupClients()` pour maintenir les WebSockets. 【F:cues.cpp†L19-L21】【F:config.h†L24-L27】
- **Persistance LittleFS** : le serveur monte LittleFS mais aucune gestion d'erreur avancée ni redémarrage en cas d'échec n'est prévue. Implémentez un fallback (redémarrage sécurisé, mode maintenance) si LittleFS échoue, et prévoyez un mécanisme d'intégrité (checksum des fichiers). 【F:web_server.cpp†L45-L51】
- **Préférences inutilisées** : la bibliothèque `Preferences.h` est incluse mais non utilisée ; soit retirer la dépendance, soit l'employer pour stocker la configuration Wi-Fi et les textes persistants avec redondance ou double-bank pour éviter les corruptions. 【F:web_server.cpp†L12-L13】
- **Gestion mémoire** : l'usage intensif de `String` peut fragmenter la RAM sur ESP32. Privilégier `std::string`, `StaticJsonDocument`, `String::reserve` ou des buffers C fixes pour les messages fréquents (`cueTexts`, JSON). 【F:cues.h†L7-L12】【F:web_server.cpp†L29-L43】
- **Multitâche** : envisager l'utilisation de `xTaskCreatePinnedToCore` pour séparer la gestion UI (WebSocket) de la couche matérielle si la réactivité est critique.

## 3. Sécurité réseau et conformité
- **Point d'accès ouvert** : `startWiFiWithPortal` crée un AP avec mot de passe par défaut "12345678" sans portail captif complet. Implémenter une configuration sécurisée : WPA2 fort, certificats TLS, suppression des identifiants en clair, redirection captive, et possibilité de désactiver le mode AP en production. 【F:wifi_portal.cpp†L1-L19】【F:config.h†L8-L13】
- **Absence d'authentification HTTP/WebSocket** : quiconque rejoint le réseau peut déclencher des cues. Ajouter authentification (token, OAuth, certificats clients) et CSRF protection sur `/trigger` et `/ws`. 【F:web_server.cpp†L52-L78】
- **Gestion TLS** : ajouter ESP32 secure server (`AsyncTCP` TLS) ou placer l'appareil derrière une passerelle sécurisée. Prévoir renouvellement de certificats.
- **Journalisation et audit** : implémenter des logs horodatés (RTC ou NTP) et stockage circulaire des événements déclenchés pour répondre aux besoins de traçabilité.

## 4. UX et fonctionnalités logicielles
- **Retour visuel** : synchroniser l'état des entrées et la LED pour éviter l'activation simultanée de plusieurs cues, gérer la priorisation et afficher un message clair sur OLED (défilement, multi-lignes). 【F:display_manager.cpp†L21-L29】
- **Interface Web** : ajouter validation du formulaire, mémorisation du texte précédemment envoyé, gestion des états (disponibilité des cues, signalement d'erreur). `app.js` ne gère pas les erreurs de WebSocket ni les réponses HTTP. 【F:data/app.js†L1-L38】
- **Accessibilité** : améliorer `index.html` avec attributs `aria`, feedback visuel pour l'état de connexion, mode sombre/clair, internationalisation. 【F:data/index.html†L1-L36】
- **Portail Wi-Fi** : `wifi.html` prévoie `/scan` et `/save_wifi` mais ces routes sont absentes du firmware. Implémenter le backend AsyncWebServer correspondant et stocker la configuration persistante. 【F:data/wifi.html†L45-L72】
- **Mise à jour OTA** : intégrer Arduino OTA ou un système signé pour déployer des correctifs sans intervention physique.

## 5. Qualité industrielle et tests
- **Tests automatisés** : mettre en place des tests unitaires (Unity, Ceedling) pour la logique C++, et des tests E2E (Playwright) pour le front-end. Ajouter un pipeline CI (GitHub Actions) pour lint (`clang-tidy`, `eslint`, `stylelint`) et build (`arduino-cli`, PlatformIO).
- **Analyse statique** : activer les avertissements `-Wall -Wextra`, utiliser `cppcheck` et `clang-analyzer` pour détecter les débordements, fuites, etc.
- **Mesures de performances** : instrumenter le code pour mesurer les temps de réaction, jitter, latence WebSocket, consommation de courant.
- **Documentation** : créer un manuel d'installation, procédures de tests, plan de maintenance, BOM matériel.

## 6. Hardware et intégration
- **Redondance matérielle** : vérifier la tolérance aux coupures secteur (UPS, supercapacités), intégrer un watchdog matériel/logiciel (`esp_task_wdt`).
- **Certification** : anticiper la conformité CE/FCC (CEM, sécurité électrique). Prévoir un boîtier ignifugé, connecteurs sécurisés, et conformité aux normes scéniques.
- **Protection physique** : filtrage EMI sur les lignes LED, isolation galvanique si nécessaire, boutons robustes, connectique verouillable.

## 7. Roadmap recommandée
1. Finaliser la pile Wi-Fi (portail + stockage sécurisé + fallback).
2. Sécuriser le serveur Web (auth, TLS, validation).
3. Refactoriser la gestion des cues (états, timers, `updateCues`, tests).
4. Industrialiser la production (CI/CD, tests hardware-in-the-loop, documentation).
5. Mettre en place la supervision (logs, monitoring distant, mises à jour OTA).

La priorisation doit viser d'abord la sûreté et la fiabilité, puis l'expérience utilisateur et l'industrialisation. Chaque livraison devra être accompagnée de tests automatisés et de rapports de validation pour atteindre un niveau "commercialisable".
