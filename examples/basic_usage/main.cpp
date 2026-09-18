/**
 * @file main.cpp
 * @brief Template générique d'intégration du framework MyFrameWork
 *
 * --- Modes de debug disponibles (à définir dans platformio.ini ou avant les includes) ---
 *   #define DEBUG_SERIAL        -> Debug via port série (115200 bauds)
 *   #define DEBUG_SOCKET        -> Debug via socket TCP (port 1504)
 *   Les deux peuvent être activés simultanément.
 *   Si aucun n'est défini, DEBUGLOG() et DEBUGINIT() sont des no-op.
 *
 * --- Connexion WiFi ---
 *   myWifiBeginWPS() : connexion automatique + WPS via bouton BOOT (GPIO0) si pas de credentials
 *   myWifiBegin()    : connexion automatique + SmartConfig si pas de credentials
 *   loadIP()         : restaure une IP statique sauvegardée en flash (Preferences)
 *   saveIP()         : sauvegarde l'IP courante en flash
 *   resetIP()        : efface l'IP sauvegardée
 */

// #define DEBUG_SERIAL   // Décommenter pour activer le debug série
// #define DEBUG_SOCKET   // Décommenter pour activer le debug socket

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <mydebug.h>
#include <myfunction.h>

// ─── Callback de réception debug ─────────────────────────────────────────────
// Appelé lorsqu'un caractère est reçu via Serial ou Socket.
// Supprimer si la réception de commandes debug n'est pas nécessaire.
void onReceiveDebug(char *data, size_t len) {
    switch (data[0]) {
        case '?':
            DEBUGLOG("=== Aide ===\n");
            DEBUGLOG("?  Affiche cette aide\n");
            DEBUGLOG("O  Redémarrage ESP\n");
            // Ajouter vos commandes ici
            break;

        case 'O':
            DEBUGLOG("Redémarrage...\n");
            ESP.restart();
            break;

        // ── Ajouter vos commandes personnalisées ──────────────────────────
        // case 'x':
        //     maFonction();
        //     break;

        default:
            DEBUGLOG("Commande inconnue : %c\n", data[0]);
            break;
    }
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    // 1. Init debug (avec ou sans callback de réception)
    DEBUGINIT(onReceiveDebug);  // Remplacer par DEBUGINIT() si pas de réception

    // 2. Restaurer l'IP statique sauvegardée (optionnel, avant WiFi.begin)
    loadIP();

    // 3. Connexion WiFi
    //    - myWifiBeginWPS() : WPS via bouton BOOT si pas de credentials
    //    - myWifiBegin(connectTimeout, smartConfigTimeout) : SmartConfig en fallback

    myWifiBeginWPS();
    // or
    // if (myWifiBeginWPS())
	// 	DEBUGLOG("WiFi OK\n");
	// else
	// 	DEBUGLOG("No WiFi credentials start WPS\n");

    // 4. OTA (Over-The-Air update)
    ArduinoOTA.begin();

    DEBUGLOG("Setup complet\n");
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
    ArduinoOTA.handle();
	delay(100);

    // ToDo
}
