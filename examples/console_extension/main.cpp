/**
 * @file main.cpp
 * @brief Exemple d'extension de la console TCP via MYCONSOLE_USER_COMMANDS
 *
 * Les commandes applicatives sont injectées dans _mcCmdTable[] à la compilation
 * en définissant la macro MYCONSOLE_USER_COMMANDS avant #include <myconsole.h>.
 * Aucun appel runtime, aucune variable supplémentaire.
 *
 * Commandes ajoutées dans cet exemple :
 *   version          → affiche la version du firmware
 *   led <0|1>        → allume / éteint la LED embarquée
 *   uptime           → affiche le temps écoulé depuis le démarrage
 */

#define DEBUG_SOCKET   // console TCP sur port 1504

#include <Arduino.h>
#include <WiFi.h>
#include <mydebug.h>
#include <mywifi.h>

#define FW_VERSION  "1.0.0"
#define LED_PIN     2          // LED interne ESP32 (GPIO2)

// ─────────────────────────────────────────────────────────────────────────────
// Handlers des commandes applicatives
// Doivent être déclarés AVANT #include <myconsole.h>
// ─────────────────────────────────────────────────────────────────────────────
static void appCmdVersion(const char*) {
    _mcPrint("Firmware : " FW_VERSION "\r\n");
}

static void appCmdLed(const char* args) {
    if (args[0] == '1') {
        digitalWrite(LED_PIN, HIGH);
        _mcPrint("LED ON\r\n");
    } else if (args[0] == '0') {
        digitalWrite(LED_PIN, LOW);
        _mcPrint("LED OFF\r\n");
    } else {
        _mcPrint("Usage: led <0|1>\r\n");
    }
}

static void appCmdUptime(const char*) {
    uint32_t s = millis() / 1000;
    _mcPrint("Uptime : %02lud %02lu:%02lu:%02lu\r\n",
             s / 86400, (s % 86400) / 3600,
             (s % 3600) / 60, s % 60);
}

// ─────────────────────────────────────────────────────────────────────────────
// Injection dans la table — doit être défini AVANT #include <myconsole.h>
// Chaque entrée : { "mot-clé", handler, "description pour le help" }
// ─────────────────────────────────────────────────────────────────────────────
#define MYCONSOLE_USER_COMMANDS                                          \
    { "version", appCmdVersion, "show firmware version"           },    \
    { "led",     appCmdLed,     "<0|1>  turn built-in LED on/off" },    \
    { "uptime",  appCmdUptime,  "time since last boot"            },

#include <myconsole.h>

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    pinMode(LED_PIN, OUTPUT);

    DEBUGINIT(myConsoleCommandHandler);   // console partage le socket debug

    myWiFiBegin();                         // connexion WiFi

    DEBUGLOG("Setup complet — console TCP port %d\n", MYCONSOLE_PORT);
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    myConsoleHandle();
}

