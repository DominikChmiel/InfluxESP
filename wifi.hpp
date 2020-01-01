#pragma once

#include <ESP8266WiFi.h>

class ESaveWifi {
public:
	ESaveWifi();
	bool turnOn();
	bool checkStatus();
	void shutDown();
	bool isOn();
private:
	ESP8266WiFiClass m_wifi;
	bool m_isOn;
};