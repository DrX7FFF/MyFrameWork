#ifndef __MY_CONSOLE_H__
#define __MY_CONSOLE_H__

/* ─────────────────────────────────────────────────────────────────────────────
 * myconsole.h  —  TCP Console  (ESP8266 / ESP32)
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *  Standalone TCP console. Works independently of mywifi.h.
 *  When mywifi.h is ALSO included (in any order), extra commands for
 *  persistent IP configuration are automatically enabled at compile time.
 *
 *  Console commands
 *  ──────────────────
 *  Always:           ?/help  status  restart  ssid  reset  dhcp  ip  mask  gw
 *  With mywifi.h:    saveconfig  loadconfig  resetconfig
 *
 *  Console integration with mydebug.h
 *  ────────────────────────────────────
 *  • With DEBUG_SOCKET (ESP32): pass myConsoleCommandHandler to DEBUGINIT().
 *    The console shares the existing debug socket on MYCONSOLE_PORT.
 *    No own server is created.
 *  • Without DEBUG_SOCKET: creates its own WiFiServer on MYCONSOLE_PORT,
 *    started automatically once WiFi (STA or AP) becomes available.
 *
 *  Usage — console only
 *  ─────────────────────
 *    #include <myconsole.h>
 *    // setup():  DEBUGINIT();
 *    // loop():   myConsoleHandle();
 *
 *  Usage — console + WiFi provisioning (mywifi.h)
 *  ─────────────────────────────────────────────────
 *    #include <mywifi.h>
 *    #include <myconsole.h>
 *    // setup():  DEBUGINIT();  myWiFiBegin();
 *    // loop():   myConsoleHandle();
 * ─────────────────────────────────────────────────────────────────────────────
 */

// ─── Port ─────────────────────────────────────────────────────────────────────
#ifndef MYCONSOLE_PORT
  #define MYCONSOLE_PORT  1504
#endif

// ─── Includes ─────────────────────────────────────────────────────────────────
#include <Arduino.h>
#include <WiFi.h>
#include <mydebug.h>

// ─── Own TCP console server (disabled when DEBUG_SOCKET already provides one) ─
#ifndef DEBUG_SOCKET
  static WiFiServer _mcServer(MYCONSOLE_PORT);
  static WiFiClient _mcClient;
  static bool       _mcServerStarted = false;
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Console output — routes to DEBUGLOG + own WiFiClient when no DEBUG_SOCKET
// ─────────────────────────────────────────────────────────────────────────────
static void _mcPrint(const char* format, ...) __attribute__((format(printf, 1, 2)));
static void _mcPrint(const char* format, ...) {
    char buf[256];
    va_list arg;
    va_start(arg, format);
    vsnprintf(buf, sizeof(buf), format, arg);
    va_end(arg);
    DEBUGLOG("%s", buf);
#ifndef DEBUG_SOCKET
    if (_mcClient && _mcClient.connected())
        _mcClient.write((const uint8_t*)buf, strlen(buf));
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Command processing
// ─────────────────────────────────────────────────────────────────────────────
static char   _mcCmdBuf[128];
static size_t _mcCmdLen = 0;

static uint32_t _mcParseIP(const char* s) {
    IPAddress ip;
    if (!ip.fromString(s)) return 0;
    return (uint32_t)ip;
}

// ─────────────────────────────────────────────────────────────────────────────
// Command handler type
//   name : keyword (case-insensitive), without trailing space
//   fn   : handler — receives the argument string (everything after the keyword
//          and its trailing space), or "" for commands without arguments
// ─────────────────────────────────────────────────────────────────────────────
typedef void (*_mcCmdFn)(const char* args);
struct _mcCmdEntry { const char* name; _mcCmdFn fn; const char* help; };  // help=nullptr hides the entry from the help listing

// ─────────────────────────────────────────────────────────────────────────────
// Individual command handlers
// ─────────────────────────────────────────────────────────────────────────────
static void _mcCmdHelp(const char*);  // defined after _mcCmdTable

static void _mcCmdStatus(const char*) {
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        _mcPrint("Mode : AP\r\n"
                 "SSID : %s\r\n"
                 "IP   : %s\r\n",
                 WiFi.softAPSSID().c_str(),
                 WiFi.softAPIP().toString().c_str());
    } else {
        _mcPrint("Mode : STA\r\n"
                 "SSID : %s\r\n"
                 "IP   : %s\r\n"
                 "GW   : %s\r\n"
                 "RSSI : %d dBm\r\n"
                 "     : %s\r\n",
                 WiFi.SSID().c_str(),
                 WiFi.localIP().toString().c_str(),
                 WiFi.gatewayIP().toString().c_str(),
                 WiFi.RSSI(),
                 WiFi.isConnected() ? "connected" : "disconnected");
    }
}

static void _mcCmdRestart(const char*) {
    _mcPrint("Restarting...\r\n");
    delay(200);
    ESP.restart();
}

static void _mcCmdReset(const char*) {
    WiFi.disconnect(true);
    _mcPrint("Credentials cleared. Restarting...\r\n");
    delay(200);
    ESP.restart();
}

static void _mcCmdDhcp(const char*) {
    WiFi.config(IPAddress(), IPAddress(), IPAddress());
    _mcPrint("DHCP enabled (session only).\r\n");
}

static void _mcCmdIp(const char* args) {
    uint32_t v = _mcParseIP(args);
    if (!v) {
        _mcPrint("Invalid IP address.\r\n");
    } else {
        WiFi.config(IPAddress(v), WiFi.gatewayIP(), WiFi.subnetMask());
        _mcPrint("IP -> %s\r\n", IPAddress(v).toString().c_str());
    }
}

static void _mcCmdMask(const char* args) {
    uint32_t v = _mcParseIP(args);
    if (!v) {
        _mcPrint("Invalid subnet mask.\r\n");
    } else {
        WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), IPAddress(v));
        _mcPrint("Mask -> %s\r\n", IPAddress(v).toString().c_str());
    }
}

static void _mcCmdGw(const char* args) {
    uint32_t v = _mcParseIP(args);
    if (!v) {
        _mcPrint("Invalid gateway.\r\n");
    } else {
        WiFi.config(WiFi.localIP(), IPAddress(v), WiFi.subnetMask());
        _mcPrint("GW -> %s\r\n", IPAddress(v).toString().c_str());
    }
}

static void _mcCmdSsid(const char* args) {
    const char* sep = strchr(args, ' ');
    if (!sep || sep == args) {
        _mcPrint("Usage: ssid <ssid> <password>  (SSID cannot contain spaces)\r\n");
    } else {
        size_t      ssidLen = (size_t)(sep - args);
        const char* pass    = sep + 1;
        if (ssidLen > 32) {
            _mcPrint("SSID too long (max 32 chars).\r\n");
        } else if (strlen(pass) > 64) {
            _mcPrint("Password too long (max 64 chars).\r\n");
        } else {
            char ssidBuf[33] = {};
            memcpy(ssidBuf, args, ssidLen);
            WiFi.begin(ssidBuf, pass);  // saves to WiFi stack's own NVS/flash
            _mcPrint("Credentials saved. Restarting...\r\n");
            delay(200);
            ESP.restart();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Command table
//   _MC_WIFI_COMMANDS       : injected by mywifi.h when included before myconsole.h
//   MYCONSOLE_USER_COMMANDS : user-defined entries (define before #include <myconsole.h>)
// ─────────────────────────────────────────────────────────────────────────────
#ifndef _MC_WIFI_COMMANDS
  #define _MC_WIFI_COMMANDS
#endif
#ifndef MYCONSOLE_USER_COMMANDS
  #define MYCONSOLE_USER_COMMANDS
#endif
static const _mcCmdEntry _mcCmdTable[] = {
    { "?",           _mcCmdHelp,        nullptr                                       },
    { "help",        _mcCmdHelp,        "this help"                                   },
    { "status",      _mcCmdStatus,      "WiFi status"                                 },
    { "restart",     _mcCmdRestart,     "reboot ESP"                                  },
    { "reset",       _mcCmdReset,       "clear WiFi credentials & reboot"             },
    { "dhcp",        _mcCmdDhcp,        "enable DHCP (applied now, session only)"     },
    { "ip",          _mcCmdIp,          "<a.b.c.d>  set static IP (session only)"     },
    { "mask",        _mcCmdMask,        "<a.b.c.d>  set subnet mask (session only)"   },
    { "gw",          _mcCmdGw,          "<a.b.c.d>  set gateway (session only)"       },
    { "ssid",        _mcCmdSsid,        "<s> <pass>  set credentials, save & reboot"  },
    _MC_WIFI_COMMANDS
    MYCONSOLE_USER_COMMANDS
};

static void _mcCmdHelp(const char*) {
    _mcPrint("=== console ===\r\n");
    for (size_t i = 0; i < sizeof(_mcCmdTable) / sizeof(_mcCmdTable[0]); i++)
        if (_mcCmdTable[i].help)
            _mcPrint("  %-20s - %s\r\n", _mcCmdTable[i].name, _mcCmdTable[i].help);
}

// ─────────────────────────────────────────────────────────────────────────────
// Command dispatcher — iterates the table, exact or prefix match
// ─────────────────────────────────────────────────────────────────────────────
static void _mcExecuteCommand(const char* cmd) {
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') { _mcPrint("> "); return; }

    for (size_t i = 0; i < sizeof(_mcCmdTable) / sizeof(_mcCmdTable[0]); i++) {
        const char* name = _mcCmdTable[i].name;
        size_t      nlen = strlen(name);
        if (strncasecmp(cmd, name, nlen) == 0 && (cmd[nlen] == '\0' || cmd[nlen] == ' ')) {
            _mcCmdTable[i].fn(cmd[nlen] == ' ' ? cmd + nlen + 1 : "");
            _mcPrint("> ");
            return;
        }
    }
    _mcPrint("Unknown: '%s'  (type ? for help)\r\n", cmd);
    _mcPrint("> ");
}

static void _mcProcessByte(char c) {
    if (c == '\r') return;
    if (c == '\n' || _mcCmdLen >= sizeof(_mcCmdBuf) - 1) {
        _mcCmdBuf[_mcCmdLen] = '\0';
        _mcCmdLen = 0;
        _mcExecuteCommand(_mcCmdBuf);
        return;
    }
    _mcCmdBuf[_mcCmdLen++] = c;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public: receive handler to pass to DEBUGINIT() when DEBUG_SOCKET is active
// ─────────────────────────────────────────────────────────────────────────────
void myConsoleCommandHandler(char* data, size_t len) {
    for (size_t i = 0; i < len; i++)
        _mcProcessByte(data[i]);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public: console event loop. Call every iteration of loop().
// The TCP server starts automatically once WiFi (STA or AP) becomes available.
// ─────────────────────────────────────────────────────────────────────────────
void myConsoleHandle() {
#ifndef DEBUG_SOCKET
    // Auto-start the server once any WiFi mode is active
    if (!_mcServerStarted && WiFi.getMode() != WIFI_OFF) {
        _mcServer.begin();
        _mcServerStarted = true;
    }

    if (_mcServerStarted) {
        // Clean up stale client
        if (_mcClient && !_mcClient.connected())
            _mcClient.stop();

        // Accept new connection (non-blocking)
        if (!_mcClient) {
            WiFiClient c = _mcServer.available();
            if (c) {
                _mcClient = c;
                _mcCmdLen = 0;
                _mcPrint("[console] ready  (type ? for help)\r\n> ");
            }
        }

        // Read incoming bytes and dispatch commands
        if (_mcClient && _mcClient.connected()) {
            while (_mcClient.available())
                _mcProcessByte((char)_mcClient.read());
        }
    }
#endif
}

#endif // __MY_CONSOLE_H__