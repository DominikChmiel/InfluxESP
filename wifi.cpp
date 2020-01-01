#include "wifi.hpp"

#include "config.hpp"
#include "rtc_mem.hpp"
#include "debug.hpp"

ESaveWifi::ESaveWifi() {
	m_wifi.mode( WIFI_OFF );
	m_wifi.forceSleepBegin();
  	delay( 1 );
  	m_isOn = false;
}

bool ESaveWifi::turnOn() {
	if (m_isOn) {
		DEBUGLN("!!ERROR!! Wifi is already enabled.");
		return true;
	}
	// TODO: Check if we can get rid of dhcp by also caching ip + netconfig
	DEBUGLN("Starting WiFi");
	// IPAddress ip( IP_A, IP_B, IP_C, IP_OFFSET + config.id.toInt() );
	// IPAddress gateway( GW_A, GW_B, GW_C, GW_D );
	// IPAddress subnet( NM_A, NM_B, NM_C, NM_D );
	m_wifi.forceSleepWake();
	delay( 1 );
	m_wifi.persistent( false );
	m_wifi.mode( WIFI_STA );
	// m_wifi.config( ip, gateway, subnet );

	DEBUGLN("Connecting to ");
	DEBUGLN(SSID);
	if( rtcMem::isValid() ) {
		DEBUGLN("Quickconnect");
    	// The RTC data was good, make a quick connection
		m_wifi.begin( SSID, PSK, rtcMem::gRTC.channel, rtcMem::gRTC.bssid, true );
	} else {
    	// The RTC data was not valid, so make a regular connection
		m_wifi.begin( SSID, PSK );
	}
}

bool ESaveWifi::checkStatus() {
	//TODO: This conflates with the above method...it also tries to connect
	using rtcMem::gRTC;
	int retries = 0;
	int wifiStatus = m_wifi.status();
	while( wifiStatus != WL_CONNECTED ) {
		retries++;
		if( retries == 100 ) {
			DEBUGLN( "Quick connect is not working, reset WiFi and try regular connection!" );
			m_wifi.disconnect();
			delay( 10 );
			m_wifi.forceSleepBegin();
			delay( 10 );
			m_wifi.forceSleepWake();
			delay( 10 );
			m_wifi.begin( SSID, PSK );
		}
		if( retries == 200 ) {
			DEBUGLN( "Could not connect to WiFi!" );
			m_isOn = false;
			return false;
		}
		delay( 50 );
		wifiStatus = m_wifi.status();
	}
	DEBUGF("WiFi Connected, r:%d\n", retries);
	DEBUGLN("Got IP: ");  
	DEBUGLN(m_wifi.localIP());
	// Cache WiFi information
	gRTC.channel = m_wifi.channel();
	memcpy( gRTC.bssid, m_wifi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)
	m_isOn = true;
};

void ESaveWifi::shutDown() {
	if (!m_isOn) {
		DEBUGLN("!!ERROR!! Wifi is not enabled.");
		// No return here, shutdown should be executed anyway
	}
	m_wifi.disconnect( true );
	m_wifi.mode( WIFI_OFF );
	m_wifi.forceSleepBegin();
	delay( 1 );
	m_isOn = false;
};

bool ESaveWifi::isOn() {
	return m_isOn;
};