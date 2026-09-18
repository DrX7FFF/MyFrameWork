#ifndef __MY_WIFI_H__
#define __MY_WIFI_H__

/* ─────────────────────────────────────────────────────────────────────────────
 * mywifi.h  —  WiFi Provisioning  (ESP8266 / ESP32)
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *  Boot
 *  ────
 *  • AP "ESP_XXXXXX" always opened (provisioning available immediately)
 *  • STA connection attempted simultaneously with stored credentials
 *  • Once STA connected → AP closed automatically
 *
 *  Reconnection
 *  ────────────
 *  Handled entirely by the WiFi stack. No watchdog, no restart.
 *
 *  Console
 *  ───────
 *  Include myconsole.h instead of this file for TCP console support.
 *
 *  Usage
 *  ─────
 *    #include <mywifi.h>          // or myconsole.h for console support
 *    // setup():  DEBUGINIT();  myWiFiBegin();
    // loop():   myConsoleHandle()  (if using myconsole.h) — no myWiFiHandle() needed
 * ─────────────────────────────────────────────────────────────────────────────
 */

// ─── Configuration ────────────────────────────────────────────────────────────
#ifndef MYWIFI_AP_SSID_PREFIX
  #define MYWIFI_AP_SSID_PREFIX  "ESP_"
#endif
#ifndef MYWIFI_AP_PASSWORD
  #define MYWIFI_AP_PASSWORD     ""    // empty = open AP
#endif
#ifndef ESP32
  #ifndef MYWIFI_EEPROM_OFFSET
    #define MYWIFI_EEPROM_OFFSET 0
  #endif
  #ifndef MYWIFI_EEPROM_SIZE
    #define MYWIFI_EEPROM_SIZE   256
  #endif
#endif

// ─── Includes ─────────────────────────────────────────────────────────────────
#include <Arduino.h>
#include <WiFi.h>
#ifdef ESP32
  #include <Preferences.h>
#else
  #include <EEPROM.h>
#endif
#include <mydebug.h>

// ─────────────────────────────────────────────────────────────────────────────
// Persistent IP configuration (credentials are stored by the WiFi stack itself)
//  ESP32   → Preferences (NVS), namespace "mywifi"
//  ESP8266 → EEPROM at MYWIFI_EEPROM_OFFSET (17 B: 4 magic + 13 config)
// ─────────────────────────────────────────────────────────────────────────────
#ifndef ESP32
static WiFiEventHandler _mwGotIPHandler;   // must outlive myWiFiBegin()
#endif

// ── ESP32: Preferences (NVS) ──────────────────────────────────────────────────
#ifdef ESP32
static void _mwLoadConfig() {
    Preferences prefs;
    if (!prefs.begin("mywifi", true)) return;
    uint8_t  dhcp = prefs.getUChar("dhcp",   1);
    uint32_t lip  = prefs.getULong("lip",    0);
    uint32_t gw   = prefs.getULong("gw",     0);
    uint32_t sub  = prefs.getULong("subnet", 0);
    prefs.end();
    if (!dhcp && lip)
        WiFi.config(IPAddress(lip), IPAddress(gw), IPAddress(sub));
}
static void _mwSaveConfig() {
    uint32_t lip = (uint32_t)WiFi.localIP();
    Preferences prefs;
    prefs.begin("mywifi", false);
    prefs.putUChar("dhcp",   lip ? 0 : 1);
    prefs.putULong("lip",    lip);
    prefs.putULong("gw",     lip ? (uint32_t)WiFi.gatewayIP()  : 0);
    prefs.putULong("subnet", lip ? (uint32_t)WiFi.subnetMask() : 0);
    prefs.end();
}
static void _mwClearConfig() {
    Preferences prefs;
    prefs.begin("mywifi", false);
    prefs.clear();
    prefs.end();
    WiFi.disconnect(true);
}

// ── ESP8266: EEPROM ───────────────────────────────────────────────────────────
#else
static const uint8_t _MW_MAGIC[4] = {0xDE, 0xAD, 0xBE, 0xEF};
#pragma pack(push, 1)
struct _MWDisk { uint8_t magic[4]; uint8_t dhcp; uint32_t lip, gw, sub; };  // 17 B
#pragma pack(pop)

static void _mwLoadConfig() {
    _MWDisk disk;
    EEPROM.begin(MYWIFI_EEPROM_SIZE);
    EEPROM.get(MYWIFI_EEPROM_OFFSET, disk);
    EEPROM.end();
    if (memcmp(disk.magic, _MW_MAGIC, 4) == 0 && !disk.dhcp && disk.lip)
        WiFi.config(IPAddress(disk.lip), IPAddress(disk.gw), IPAddress(disk.sub));
}
static void _mwSaveConfig() {
    uint32_t lip = (uint32_t)WiFi.localIP();
    _MWDisk disk;
    memcpy(disk.magic, _MW_MAGIC, 4);
    disk.dhcp = lip ? 0 : 1;
    disk.lip  = lip;
    disk.gw   = lip ? (uint32_t)WiFi.gatewayIP()  : 0;
    disk.sub  = lip ? (uint32_t)WiFi.subnetMask() : 0;
    EEPROM.begin(MYWIFI_EEPROM_SIZE);
    EEPROM.put(MYWIFI_EEPROM_OFFSET, disk);
    EEPROM.commit();
    EEPROM.end();
}
static void _mwClearConfig() {
    _MWDisk disk = {};
    EEPROM.begin(MYWIFI_EEPROM_SIZE);
    EEPROM.put(MYWIFI_EEPROM_OFFSET, disk);
    EEPROM.commit();
    EEPROM.end();
    WiFi.begin("", ""); delay(100); WiFi.disconnect();
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

// Call once in setup(). Opens AP and attempts STA connection simultaneously.
// Call _mwLoadConfig() before this if you want persistent IP config applied.
static void _mwOnGotIP() {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    DEBUGLOG("[mywifi] connected  IP %s  — AP closed\n",
             WiFi.localIP().toString().c_str());
}

void myWiFiBegin() {
    char apSSID[33];
#ifdef ESP32
    snprintf(apSSID, sizeof(apSSID), "%s%06X", MYWIFI_AP_SSID_PREFIX,
             (unsigned int)(ESP.getEfuseMac() & 0x00FFFFFFU));
#else
    snprintf(apSSID, sizeof(apSSID), "%s%06X", MYWIFI_AP_SSID_PREFIX,
             (unsigned int)(ESP.getChipId() & 0x00FFFFFFU));
#endif

    WiFi.mode(WIFI_AP_STA);
    const char pwd[] = MYWIFI_AP_PASSWORD;
    pwd[0] ? WiFi.softAP(apSSID, pwd) : WiFi.softAP(apSSID);

    // Register before WiFi.begin() so no event is missed
#ifdef ESP32
    WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) { _mwOnGotIP(); },
                 ARDUINO_EVENT_WIFI_STA_GOT_IP);
#else
    _mwGotIPHandler = WiFi.onStationModeGotIP(
        [](const WiFiEventStationModeGotIP&) { _mwOnGotIP(); });
#endif

    WiFi.begin();   // uses credentials stored by the WiFi stack

    DEBUGLOG("[mywifi] AP '%s'  %s  STA '%s'\n",
             apSSID, WiFi.softAPIP().toString().c_str(),
             WiFi.SSID().c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// Console integration — WiFi commands injected into myconsole.h's table via
// _MC_WIFI_COMMANDS.  Required include order: mywifi.h  then  myconsole.h.
// ─────────────────────────────────────────────────────────────────────────────
#ifndef __MY_CONSOLE_H__
  // Forward-declare _mcPrint so handlers compile before myconsole.h is included.
  // The definition is provided by myconsole.h later in the same translation unit.
  static void _mcPrint(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
#endif

static void _mwConsoleSaveconfig(const char*)  { _mwSaveConfig();  _mcPrint("Config saved.\r\n"); }
static void _mwConsoleLoadconfig(const char*)  { _mwLoadConfig();  _mcPrint("Config loaded.\r\n"); }
static void _mwConsoleResetconfig(const char*) {
    _mwClearConfig();
    _mcPrint("Config cleared. Restarting...\r\n");
    delay(200);
    ESP.restart();
}

#define _MC_WIFI_COMMANDS                                                        \
    { "saveconfig",  _mwConsoleSaveconfig,  "persist current IP config to NVS"  }, \
    { "loadconfig",  _mwConsoleLoadconfig,  "reload IP config from NVS & apply" }, \
    { "resetconfig", _mwConsoleResetconfig, "clear NVS IP config & reboot"      },

#endif // __MY_WIFI_H__