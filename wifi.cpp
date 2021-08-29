#include "wifi.hpp"

#include "config.hpp"
#include "debug.hpp"
#include "rtc_mem.hpp"

ESaveWifi::ESaveWifi() {
	m_wifi.mode(WIFI_OFF);
	m_wifi.forceSleepBegin();
	delay(1);
	m_isOn = false;
}

auto ESaveWifi::turnOn() -> bool {
	using rtcMem::gRTC;
	if (m_isOn) {
		LOGLN("!!ERROR!! Wifi is already enabled.");
		return true;
	}
	// TODO(dominik): Check if we can get rid of dhcp by also caching ip + netconfig
	LOGLN("Starting WiFi");
	m_wifi.forceSleepWake();
	delay(1);
	m_wifi.persistent(false);
	m_wifi.mode(WIFI_STA);

	LOGLN("Connecting to ");
	LOGLN(SSID);
	if (rtcMem::is_valid()) {
		LOGLN("Quickconnect");
		// FIXME: WHile this preconfig can speed up things quite a bit, it runs into issues when dns changes
		// Probably requires fixed router setup/IP Assignment?
		// m_wifi.config( gRTC.ip_addr, gRTC.gateway_addr, gRTC.netmask, gRTC.dns_addr );
		// The RTC data was good, make a quick connection
		m_wifi.begin(SSID, PSK, gRTC.channel, gRTC.bssid, true);
	} else {
		// The RTC data was not valid, so make a regular connection
		m_wifi.begin(SSID, PSK);
	}
}

auto ESaveWifi::checkStatus() -> bool {
	// TODO(dominik): This conflates with the above method...it also tries to connect
	using rtcMem::gRTC;
	int retries    = 0;
	int wifiStatus = m_wifi.status();
	while (wifiStatus != WL_CONNECTED) {
		retries++;
		if (retries == 100) {
			LOGLN("Quick connect is not working, reset WiFi and try regular connection!");
			m_wifi.disconnect();
			delay(10);
			m_wifi.forceSleepBegin();
			delay(10);
			m_wifi.forceSleepWake();
			delay(10);
			m_wifi.begin(SSID, PSK);
		}
		if (retries == 200) {
			LOGLN("Could not connect to WiFi!");
			m_isOn = false;
			return false;
		}
		delay(50);
		wifiStatus = m_wifi.status();
	}
	LOGF("WiFi Connected, r:%d\n", retries);
	LOGLN("Got IP: ");
	LOGLN(m_wifi.localIP());
	// Cache WiFi information
	gRTC.ip_addr      = (uint32_t)m_wifi.localIP();
	gRTC.gateway_addr = (uint32_t)m_wifi.gatewayIP();
	gRTC.netmask      = (uint32_t)m_wifi.subnetMask();
	gRTC.ip_addr      = (uint32_t)m_wifi.dnsIP();
	gRTC.dns_addr     = m_wifi.channel();
	memcpy(gRTC.bssid, m_wifi.BSSID(), 6);   // Copy 6 bytes of BSSID (AP's MAC address)
	m_isOn = true;
	return true;
};

void ESaveWifi::shutDown() {
	if (!m_isOn) {
		LOGLN("!!ERROR!! Wifi is not enabled.");
		// No return here, shutdown should be executed anyway
	}
	m_wifi.disconnect(true);
	m_wifi.mode(WIFI_OFF);
	m_wifi.forceSleepBegin();
	delay(1);
	m_isOn = false;
};

auto ESaveWifi::isOn() -> bool {
	return m_isOn;
};