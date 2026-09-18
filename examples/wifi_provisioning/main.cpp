/**
 * @file main.cpp
 * @brief Exemple d'utilisation de mywifi.h + myconsole.h
 *
 * --- Démarrage ---
 *   L'ESP ouvre toujours un AP "ESP_XXXXXX" ET tente de se connecter
 *   au réseau sauvegardé. Dès que la connexion STA est établie, l'AP se ferme.
 *
 *   Si pas de credentials (premier démarrage ou après un reset) :
 *     telnet 192.168.4.1 1504
 *     > ssid MonReseau MonMotDePasse
 *   L'ESP sauvegarde et redémarre.
 *
 * --- En marche ---
 *   Console accessible à <IP>:1504 :
 *     > status / restart / reset / dhcp / ip / mask / gw
 *
 * --- Debug disponible ---
 *   #define DEBUG_SERIAL   → logs série (115200 bauds)
 *   #define DEBUG_SOCKET   → logs + console via socket (ESP32 seulement)
 */

// #define DEBUG_SERIAL
// #define DEBUG_SOCKET   // ESP32 uniquement

#include <Arduino.h>
#include <mywifi.h>
#include <myconsole.h>

void setup() {
#ifdef DEBUG_SOCKET
    DEBUGINIT(myConsoleCommandHandler);  // AVANT myWiFiBegin()
#else
    DEBUGINIT();
#endif
    _mwLoadConfig();   // charge et applique la config IP persistante
    myWiFiBegin();
}

void loop() {
    myConsoleHandle();
}