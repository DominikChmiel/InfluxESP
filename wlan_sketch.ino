#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "rtc_mem.hpp"
#include "debug.hpp"
#include "wifi.hpp"

// Parts of this project are based on https://bitbucket.org/2msd/d1mini_sht30_mqtt/src/master/d1mini_sht30_mqtt.ino

float temperature, humidity, pressure, altitude;

Adafruit_BME280 bme;


void setup() {
    initDebug();

    LOGINTER("start");

    ESaveWifi eWifi;

    if (!rtcMem::read()) {
        LOGLN("Reading RTC data failed.");
    }

    eWifi.turnOn();

    unsigned status;

    status = bme.begin(0x76);   

    if (!status) {
        LOGLN("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        LOG("SensorID was: 0x"); 
        LOGLN(bme.sensorID(),16);
        LOGLN("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085");
        LOGLN("   ID of 0x56-0x58 represents a BMP 280,");
        LOGLN("        ID of 0x60 represents a BME 280.");
        LOGLN("        ID of 0x61 represents a BME 680.");
        while (1) delay(10);
    }

    String influx_data = "bme280,host=test-";
    influx_data += ESP.getChipId();
    influx_data += " temperature=";
    influx_data += bme.readTemperature();
    influx_data += ",pressure=";
    influx_data += bme.readPressure();
    influx_data += ",humidity=";
    influx_data += bme.readHumidity() / 100.0F;
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

struct timespec get_timestamp_from_server() {
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
                String nsString = data.substring(data.length() - 9);
                String sString = data.substring(0, data.length() - 9);
                LOGLN(sString);
                LOGLN(nsString);
                return {
                    sString.toInt(),
                    nsString.toInt()
                };
            }
        } else {
            LOGF("[HTTP] GET TS... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    return {0, 0};
}

void send_to_influx(String& data) {

    // Preallocate string mem
    // data.reserve(data.length() + 400);

    LOGINTER("Start TS");
    auto ts = get_timestamp_from_server();
    if (ts.tv_sec == 0) {
        LOGLN("Failed, retrying...");
        ts = get_timestamp_from_server();
        if (ts.tv_sec == 0) {
            LOGLN("Final fail.");
            return;
        }
    }
    LOGINTER("End TS");
    WiFiClient client;
    HTTPClient http;

    // LOGF("TVal: %d | %d", (ts >> 32) & 0xFFFFFFFF, ts & 0xFFFFFFFF);
    char ts_string[24];
    snprintf(ts_string, 20, "%d%09d000000", ts.tv_sec, ts.tv_nsec);

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
