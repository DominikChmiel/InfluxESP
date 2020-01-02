#pragma once

#include <ESP8266WiFi.h>

class ESaveWifi {
public:
	ESaveWifi();
	auto turnOn() -> bool;
	auto checkStatus() -> bool;
	void shutDown();
	auto isOn() -> bool;

private:
	ESP8266WiFiClass m_wifi;
	bool             m_isOn;
};