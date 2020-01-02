#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>

#include "rtc_mem.hpp"
#include "debug.hpp"
#include "wifi.hpp"
#include "bme280_aggregator.hpp"

// Parts of this project are based on https://bitbucket.org/2msd/d1mini_sht30_mqtt/src/master/d1mini_sht30_mqtt.ino


BME280Aggregator bme;


void setup() {
    initDebug();

    LOGINTER("start");

    ESaveWifi eWifi;

    if (!rtcMem::read()) {
        LOGLN("Reading RTC data failed.");
    }

    eWifi.turnOn();

    if (!bme.begin(0x76)) {
        while (1) delay(10);
    }
        
    auto full_data = bme.readAllSensors();

    String influx_data = "bme280,host=";
    influx_data += ESP.getChipId();
    influx_data += " ";
    influx_data += full_data.toString();
    LOGLN(influx_data);

    LOGINTER("sensor");
    eWifi.checkStatus();
    LOGINTER("sending");
    send_to_influx(influx_data);
    rtcMem::write();
    LOGINTER("final");
    auto now = millis();
    if (now > INTERVAL_MS) {
        LOGLN("Interval was too large. sleeping full length.");
        ESP.deepSleep(INTERVAL_MS * 1000);
    } else {
        ESP.deepSleep((INTERVAL_MS - millis()) * 1000);
    }
    delay(100); // See https://www.mikrocontroller.net/topic/384345
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
    time_t  tv_millionsec;   /* MillionSeconds */
    long    tv_millisec;  /* Milliseconds */
};

struct msec_timespec get_timestamp_from_server() {
    struct msec_timespec res = {0, 0};
    WiFiClient client;
    HTTPClient http;
    if (http.begin(client, TS_URL)) {
        int httpCode = http.GET();
        if (httpCode > 0) {
            LOGF("[HTTP] GET TS... code: %d\n", httpCode);
            LOGINTER("Finished get");

            if (httpCode == HTTP_CODE_OK) {
                LOGLN("Received timestamp successfully.");
                String data = http.getString();
                LOGINTER("Got string");
                data.trim();
                LOG("TS string: ");
                LOGLN(data);
                LOGINTER("Converting");
                String micsString = data.substring(data.length() - 9);
                String ksString = data.substring(0, data.length() - 9);
                LOGLN(ksString);
                LOGLN(micsString);
                res = {
                    ksString.toInt(),
                    micsString.toInt()
                };
            }
        } else {
            LOGF("[HTTP] GET TS... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    return res;
}

void send_to_influx(String& data) {

    // Preallocate string mem
    // data.reserve(data.length() + 400);

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
    WiFiClient client;
    HTTPClient http;

    // LOGF("TVal: %d | %d", (ts >> 32) & 0xFFFFFFFF, ts & 0xFFFFFFFF);
#define TS_LEN 24
    char ts_string[TS_LEN];
    snprintf(ts_string, TS_LEN, "%d%09d000000", ts.tv_millionsec, ts.tv_millisec);

    LOG("Reassembled ts: ");
    LOGLN(String(ts_string));

    data += " ";
    data += String(ts_string);

    LOGLN("Sending data...");
    if (http.begin(client, DB_URL)) {
        int httpCode = http.POST(data);
        if (httpCode > 0) {
            LOGF("[HTTP] POST... code: %d\n", httpCode);

            if (httpCode == HTTP_CODE_OK) {
                LOGLN("Uploaded data successfully.");
            }
        } else {
            LOGF("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
}
