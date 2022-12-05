#ifndef __MY_FUNCTION_H__
#define __MY_FUNCTION_H__

#include <Arduino.h>
#include <WiFi.h>
#include <mydebug.h>
#include <Preferences.h>
#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

#ifndef IPFILENAME
#define IPFILENAME "IPConfig"
#endif
#define IP_KEY_LOCAL 	"local"
#define IP_KEY_GATEWAY 	"gateway"
#define IP_KEY_SUBNET	"subnet"


// static void my_handler_got_ssid_passwd(void *arg, esp_event_base_t base, int32_t event_id, void *data)
// {
//     smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)data;
//     uint8_t ssid[33] = { 0 };
//     uint8_t bssid[6] = { 0 };
//     uint8_t password[65] = { 0 };
//     // uint8_t cellphone_ip[4];
//     esp_err_t err = ESP_OK;

// 	memcpy(ssid, evt->ssid, sizeof(evt->ssid));
// 	memcpy(bssid, evt->bssid, sizeof(evt->bssid));
//     memcpy(password, evt->password, sizeof(evt->password));
//     // memcpy(cellphone_ip, evt->cellphone_ip, sizeof(evt->cellphone_ip));

//     DEBUGLOG("\n%lu(%u) SSID:%s\n", millis(),xPortGetCoreID(), ssid);
//     DEBUGLOG("%lu(%u) PASSWORD:%s\n", millis(),xPortGetCoreID(), password);
//     DEBUGLOG("%lu(%u) BSSID: %u:%u:%u:%u:%u:%u\n", millis(),xPortGetCoreID(), bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
//     DEBUGLOG("%lu(%u) Channel:%u\n", millis(),xPortGetCoreID(), WiFi.channel());

// 	// WiFi.begin((const char *)evt->ssid,(const char *)evt->password,WiFi.channel(),evt->bssid);
//     // DEBUGLOG("%lu(%u) Done Restart\n", millis(),xPortGetCoreID());
// 	// delay(500);
// 	// ESP.restart();
//     // DEBUGLOG("%lu(%u) Phone ip: %d.%d.%d.%d\n", millis(),xPortGetCoreID(), cellphone_ip[0], cellphone_ip[1], cellphone_ip[2], cellphone_ip[3]);
// }


void mySmartConfig(bool forceChannel = false){
	if (WiFi.begin() != wl_status_t::WL_DISCONNECTED){
		DEBUGLOG("Start SmartConfig :\n");
		WiFi.beginSmartConfig();		
		while (!WiFi.smartConfigDone()){
			delay(100);
			DEBUGLOG("/");
		}
		// DEBUGLOG("%lu(%u) SSID:%s\n", millis(),xPortGetCoreID(), WiFi.SSID().c_str());
		// DEBUGLOG("%lu(%u) PWD:%s\n", millis(),xPortGetCoreID(), WiFi.psk().c_str());
		// DEBUGLOG("%lu(%u) Channel:%u\n", millis(),xPortGetCoreID(), WiFi.channel());
		// DEBUGLOG("%lu(%u) BSSID:%s\n", millis(),xPortGetCoreID(), WiFi.BSSIDstr().c_str());
		if (forceChannel){
			WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str(),WiFi.channel(),WiFi.BSSID());
			delay(1000);
		}
		ESP.restart();
	}
}

bool loadIP(){
	bool ret = false;
	Preferences prefs;
	if (prefs.begin(IPFILENAME,true)){
		IPAddress localIP = prefs.getULong(IP_KEY_LOCAL, 0);
		IPAddress gateway = prefs.getULong(IP_KEY_GATEWAY, 0);
		IPAddress subnet = prefs.getULong(IP_KEY_SUBNET, 0);
		if (localIP && gateway && subnet)
			ret = WiFi.config(localIP,gateway,subnet);
		prefs.end();
	}
	return ret;
}

void saveIP(){
	Preferences prefs;
	if (prefs.begin(IPFILENAME,false)){
		prefs.putULong(IP_KEY_LOCAL, WiFi.localIP());
		prefs.putULong(IP_KEY_GATEWAY, WiFi.gatewayIP());
		prefs.putULong(IP_KEY_SUBNET,WiFi.subnetMask());
		prefs.end();
	}
}

void resetIP(){
	Preferences prefs;
	if (prefs.begin(IPFILENAME,false))
		prefs.clear();
}

#endif // __MY_FUNCTION_H__