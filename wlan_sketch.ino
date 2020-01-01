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

    ESaveWifi eWifi;

    if (!rtcMem::read()) {
        DEBUGLN("Reading RTC data failed.");
    }

    eWifi.turnOn();

    unsigned status;

    status = bme.begin(0x76);   

    if (!status) {
        DEBUGLN("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        DEBUG("SensorID was: 0x"); 
        DEBUGLN(bme.sensorID(),16);
        DEBUGLN("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085");
        DEBUGLN("   ID of 0x56-0x58 represents a BMP 280,");
        DEBUGLN("        ID of 0x60 represents a BME 280.");
        DEBUGLN("        ID of 0x61 represents a BME 680.");
        while (1) delay(10);
    }

    String influx_data = "bme280,host=";
    influx_data += ESP.getChipId();
    influx_data += " temperature=";
    influx_data += bme.readTemperature();
    influx_data += ",pressure=";
    influx_data += bme.readPressure();
    influx_data += ",humidity=";
    influx_data += bme.readHumidity() / 100.0F;
    DEBUGLN(influx_data);

    DEBUGF("SYSTEM TIME t1 %lu millseconds\n", millis());
    eWifi.checkStatus();
    DEBUGF("SYSTEM TIME t2 %lu millseconds\n", millis());
    send_to_influx(influx_data);
    rtcMem::write();
    DEBUGF("SYSTEM TIME fin %lu millseconds\n", millis());
    ESP.deepSleep((INTERVAL_MS - millis()) * 1000);
    delay(100); // See https://www.mikrocontroller.net/topic/384345
}

void loop() {
    DEBUGLN("I should not be here. I should be sleeping.");
    delay(1000);
}

void send_to_influx(String& data) {

    WiFiClient client;
    HTTPClient http;

    DEBUG("Sending data...\n");
    if (http.begin(client, DB_URL)) {
        int httpCode = http.POST(data);
        if (httpCode > 0) {
            DEBUGF("[HTTP] POST... code: %d\n", httpCode);

            if (httpCode == HTTP_CODE_OK) {
                DEBUGLN("Uploaded data successfully.");
            }
        } else {
            DEBUGF("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
}
