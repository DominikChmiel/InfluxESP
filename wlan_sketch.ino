#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

float temperature, humidity, pressure, altitude;

Adafruit_BME280 bme;

const char* SSID = "<SSID>";
const char* PSK = "<PSKKEY>";
const char* DB_URL = "http://<host>:8086/write?db=envsensors";

void setup() {
  Serial.begin(115200);
  while(!Serial) {
    delay(100);
  }
  Serial.println("Envsensor");
  delay(100);
  unsigned status;
  
  status = bme.begin(0x76);   
  
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    }

  Serial.println("Connecting to ");
  Serial.println(SSID);

  //connect to your local wi-fi network
  WiFi.begin(SSID, PSK);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.println("Got IP: ");  Serial.println(WiFi.localIP());
  Serial.println("Chip ip: " + ESP.getChipId());
}

void loop() {
  send_to_influx();
  delay(1000 * 10);
}

void send_to_influx() {
  Serial.println("M0");
  String influx_data = "bme280,host=";
  influx_data += ESP.getChipId();
  Serial.println("M1");
  influx_data += " temperature=";
  influx_data += bme.readTemperature();
  Serial.println("M2");
  influx_data += ",pressure=";
  influx_data += bme.readPressure();
  Serial.println("M3");
  influx_data += ",humidity=";
  influx_data += bme.readHumidity() / 100.0F;
  Serial.println(influx_data);

  WiFiClient client;
  HTTPClient http;

  Serial.print("Sending data...\n");
  if (http.begin(client, DB_URL)) {
    int httpCode = http.POST(influx_data);
    if (httpCode > 0) {
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK) {
        Serial.println("Uploaded data successfully.");
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}
