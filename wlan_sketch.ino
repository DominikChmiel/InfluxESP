#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Wire.h>

#include "bme280_aggregator.hpp"
#include "debug.hpp"
#include "rtc_mem.hpp"
#include "wifi.hpp"

// Parts of this project are based on https://bitbucket.org/2msd/d1mini_sht30_mqtt/src/master/d1mini_sht30_mqtt.ino

BME280Aggregator bme;

ESaveWifi eWifi;

#define RECORD_LIMIT (STORED_RECORDS - 10)

void execSleep(uint32_t sleepTime = INTERVAL_MS) {
#ifdef USE_DEEPSLEEP
	ESP.deepSleep(sleepTime * 1000);
#else
#ifdef USE_OTA
	LOGF("OTAHandling: %d\n", sleepTime);
	while (sleepTime > 0) {
		uint32_t t1 = millis();
		ArduinoOTA.handle();
		uint32_t t2 = millis();
		if (t2 - t1 < 50) {
			delay(50);
			t2 = millis();
		}
		if (t2 - t1 < sleepTime) {
			sleepTime -= (t2 - t1);
		} else {
			sleepTime = 0;
		}
	}
#else
	LOGF("Delaying: %d\n", sleepTime);
	delay(sleepTime);
#endif
	ESP.reset();
#endif
	delay(100);   // See https://www.mikrocontroller.net/topic/384345
}

void setup() {
	using rtcMem::gRTC;
	init_debug();

	LOGINTER("start");

	if (!rtcMem::read()) {
		LOGLN("Reading RTC data failed.");
	}

	// Check if we were woken up by default_rst => reset internal structs, data is out of date
	LOGF("Resetreason: %d\n", ESP.getResetInfoPtr()->reason);
	if (ESP.getResetInfoPtr()->reason == REASON_EXT_SYS_RST) {
		LOGLN("Ordinary Power ON, resetting stored records");
		gRTC.stored_records = 0;
	}

#ifdef USE_OTA
	LOGLN("Starting wifi: OTA Enabled.");
	eWifi.turnOn();
	bool dump_stored = true;
	ArduinoOTA.onStart([]() { Serial.println("Start"); });
	ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR)
			Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR)
			Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR)
			Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR)
			Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR)
			Serial.println("End Failed");
	});
	ArduinoOTA.begin();
#else
	bool dump_stored = gRTC.stored_records > RECORD_LIMIT;

	if (dump_stored) {
		LOGLN("Starting wifi: Have enough stored.");
		eWifi.turnOn();
	}
#endif

	auto retries = 0;
	while (retries < 100) {
		if (bme.begin(0x76)) {
			break;
		}
		retries++;
		delay(100);
		execSleep();
		return;
	}

	auto full_data = bme.readAllSensors();

	// Advance gRTC records
	gRTC.records[gRTC.stored_records] = full_data;
	gRTC.stored_records               = (gRTC.stored_records + 1) % STORED_RECORDS;

	if (dump_stored && eWifi.checkStatus()) {
		LOGINTER("sending");
		send_records_to_influx();
#ifndef USE_OTA
		eWifi.shutDown();
#endif
	}

	rtcMem::write();
	LOGINTER("final");
	auto now = millis();

#ifndef DEBUG
	Serial.begin(DEBUG_BAUDRATE);
	Serial.setTimeout(2000);
	while (!Serial) {
		delay(50);
	}
	Serial.printf("Final time: %d\n", now);
#endif
	uint32_t sleepTime = 0;
	if (now > INTERVAL_MS) {
		LOGLN("Interval was too large. sleeping full length.");
		sleepTime = INTERVAL_MS;
	} else {
		sleepTime = (INTERVAL_MS - millis());
	}
	execSleep(sleepTime);
}

void loop() {
	LOGLN("I should not be here. I should be sleeping.");
	delay(1000);
}

/*
	Slighly optimized variant of the usual timespec struct for our purposes:

	Carry microseconds instead of ns + store thousand seconds instead of single
 */
struct msec_timespec {
	time_t tv_millionsec; /* MillionSeconds */
	long   tv_millisec;   /* Milliseconds */

	void subtract(uint32_t millisec) {
		if (millisec > 1e9) {
			LOGLN("Error: Can only subtract below 1M seconds");
		}
		if (millisec > tv_millisec) {
			tv_millionsec--;
			tv_millisec += 1e9;
		}
		tv_millisec -= millisec;
	};

	void add(uint32_t millisec) {
		if (millisec > 1e9) {
			LOGLN("Error: Can only subtract below 1M seconds");
		}
		tv_millisec += millisec;
		if (tv_millisec > 1e9) {
			tv_millionsec += 1;
			tv_millisec -= 1e9;
		}
	}

	String toString() {
		char ts_string[24];
		snprintf(ts_string, sizeof(ts_string), "%d%09d", tv_millionsec, tv_millisec);
		return String(ts_string);
	}
};

struct msec_timespec get_timestamp_from_server() {
	struct msec_timespec res = {0, 0};
	WiFiClient           client;
	HTTPClient           http;
	if (http.begin(client, TS_URL)) {
		int httpCode = http.GET();
		if (httpCode > 0) {
			LOGF("[HTTP] GET TS... code: %d\n", httpCode);

			if (httpCode == HTTP_CODE_OK) {
				LOGLN("Received timestamp successfully.");
				String data = http.getString();
				data.trim();
				if (data.length() != 13) {
					LOGLN("Invalid length of time page. Wrong server?");
					return res;
				}
				LOG("TS string: ");
				LOGLN(data);
				LOGINTER("Converting");
				String micsString = data.substring(data.length() - 9);
				String ksString   = data.substring(0, data.length() - 9);
				LOGLN(ksString);
				LOGLN(micsString);
				res = {ksString.toInt(), micsString.toInt()};
			}
		} else {
			LOGF("[HTTP] GET TS... failed, error: %s\n", http.errorToString(httpCode).c_str());
		}
		http.end();
	}
	return res;
}

bool send_single_data_to_influx(String& data) {
	LOGLN(data);
	WiFiClient client;
	HTTPClient http;

	bool res = false;

	LOGLN("Sending data...");
	if (http.begin(client, DB_URL)) {
		int httpCode = http.POST(data);
		if (httpCode > 0) {
			LOGF("[HTTP] POST... code: %d\n", httpCode);

			if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT) {
				LOGLN("Uploaded data successfully.");
				res = true;
			}
		} else {
			LOGF("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
		}

		http.end();
	}
	return res;
}

void send_records_to_influx() {
	using rtcMem::gRTC;

	LOGINTER("Start TS");
	auto ts = get_timestamp_from_server();
	if (ts.tv_millionsec == 0) {
		LOGLN("Failed, retrying...");
		ts = get_timestamp_from_server();
		if (ts.tv_millionsec == 0) {
			LOGLN("Final fail.");
			return;
		}
	}
	LOGINTER("End TS");

	String influx_data = "";
	influx_data.reserve(80 * gRTC.stored_records);
	for (int16_t i = gRTC.stored_records - 1; i >= 0; i--) {
		influx_data += "bme280,host=";
		influx_data += ESP.getChipId();
		influx_data += " ";
		influx_data += gRTC.records[i].toString();

		// LOG("Reassembled ts: ");
		// LOGLN(String(ts_string));

		influx_data += " ";
		influx_data += ts.toString();
		influx_data += "000000\n";
		ts.subtract(INTERVAL_MS);
		if (influx_data.length() >= 512) {
			send_single_data_to_influx(influx_data);
			influx_data = "";
		}
	}
	if (influx_data.length()) {
		send_single_data_to_influx(influx_data);
	}

	// Reset our store
	gRTC.stored_records = 0;
}
