#ifndef __MY_DEBUG_H__
#define __MY_DEBUG_H__

#include <Arduino.h>
#include <WiFi.h>

#if defined(DEBUG_SOCKET) || defined(DEBUG_SERIAL)
	static bool _debugInitDone = false;
	#if DEBUG_SOCKET
		#include <AsyncTCP.h>
		static AsyncServer _debugSocketServer(1504);
		static AsyncClient* _debugSocketClient;
		AcDataHandler _debugDataHandler = NULL;
		std::function<void(void)> _onTCPClient = NULL;
	#endif
	#define DEBUGLOG(...) _debuglog(__VA_ARGS__)

	static void _debuglog(const char *format, ...) __attribute__((format (printf, 1, 2)));

	static void _debuglog(const char *format, ...){
		if (!_debugInitDone)
			return;
		#if DEBUG_SOCKET
		if (_debugSocketClient)
			if (!_debugSocketClient->connected()){
				_debugSocketClient->close();
				_debugSocketClient = NULL;
			}
		#ifndef DEBUG_SERIAL //si uniquement mode Socket
			if (!_debugSocketClient)
				return;
		#endif
		#endif
		char loc_buf[64];
		char * temp = loc_buf;
		va_list arg;
		va_list copy;
		va_start(arg, format);
		va_copy(copy, arg);
		int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
		va_end(copy);
		if(len < 0) {
			va_end(arg);
			return;
		};
		if(len >= sizeof(loc_buf)){
			temp = (char*) malloc(len+1);
			if(temp == NULL) {
				va_end(arg);
				return;
			}
			len = vsnprintf(temp, len+1, format, arg);
		}
		va_end(arg);
		#ifdef DEBUG_SERIAL
			Serial.write((uint8_t*)temp, len);
		#endif
		#ifdef DEBUG_SOCKET
			if (_debugSocketClient) // Par sécurité
				_debugSocketClient->write(temp,len);
		#endif
		if(temp != loc_buf)
			free(temp);
	}

	static void DEBUGINIT(){
		#ifdef DEBUG_SERIAL
			Serial.begin(115200);
			DEBUGLOG("Debug serial ON\n");
		#endif
		#ifdef DEBUG_SOCKET
			WiFi.onEvent([](WiFiEvent_t  event, WiFiEventInfo_t info ){
					_debugSocketServer.onClient([](void* arg, AsyncClient* client) {
							if(_debugSocketClient)
								_debugSocketClient->stop();
							_debugSocketClient = client;
							if (_debugDataHandler)
								_debugSocketClient->onData(_debugDataHandler);
							_debugSocketClient->write("Welcome new socket\n");
							if (_onTCPClient)
								_onTCPClient();
						}, NULL);
					_debugSocketServer.begin();
					// DEBUGLOG("Debug socket ON\n");
				},arduino_event_id_t::ARDUINO_EVENT_WIFI_READY);
		#endif
		_debugInitDone = true;
	}

	static void DEBUGINIT(std::function<void(void)> onTCPClient){
		#ifdef DEBUG_SOCKET
			_onTCPClient = onTCPClient;
		#endif
		DEBUGINIT();
	}

#else
	#define DEBUGLOG(...)
	#define DEBUGINIT(...)
#endif

static bool DEBUGHASCLIENT(){
	#ifdef DEBUG_SERIAL
		return true;
	#endif
	#ifdef DEBUG_SOCKET
		if (!_debugSocketClient)
			return false;
		return _debugSocketClient->connected();
	#endif
	return false;
}

#ifdef DEBUG_SOCKET
static void DEBUGSETDATAHANDLER(AcDataHandler handler){
	_debugDataHandler = handler;
	if (_debugDataHandler)
		if (_debugSocketClient)
			_debugSocketClient->onData(_debugDataHandler);
}
#endif

#endif // __MY_DEBUG_H__