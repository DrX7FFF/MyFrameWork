#ifndef __MY_FUNCTION_H__
#define __MY_FUNCTION_H__

#include <Arduino.h>
#include <WiFi.h>
#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

void mySmartConfig(){
	if (WiFi.begin() != wl_status_t::WL_DISCONNECTED){
		DEBUGLOG("Start SmartConfig :\n");
		WiFi.mode(WIFI_AP_STA);
		WiFi.beginSmartConfig();		
		while (!WiFi.smartConfigDone()){
			delay(100);
			DEBUGLOG("/");
		}
		ESP.restart();
	}
}

#endif // __MY_FUNCTION_H__