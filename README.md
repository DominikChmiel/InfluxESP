# InfluxESP

Firmware to sample a Bosch BMP280 sensor using an ESP8266 board + storing the recorded data in an influxDB instance.

![Grafana Dashboard](figures/dashboard.png?raw=true "Grafana Dashboard showing collected data")

## Getting Started

These instructions will get you up and running with the basic setup.

### Prerequisites

 - An ESP8266 (Used for testing: nodeMCU v2)
 - A BMP280 Sensor connected to it
 - A running InfluxDB instance

### Installing

 - Deploy timeserver.py somewhere the ESP will be able to reach it (e.g. the server running the influx instance)
 - Adapt config.hpp with the constants from your setup
    - `<SSID> <PSK>`: Used wlan SSID + corresponding PSK
    - `<host>`: Host where influxdb + timeserver.py are running
 - Build using e.g. the Arduino IDE (setting up the Arduino IDE can be found here: https://github.com/esp8266/Arduino)

### Code style

This project utilizes clang-format + clang-tidy for coding styles. Corresponding files are included in the repo.

## Contributing

1. Fork it!
2. Create your feature branch: `git checkout -b my-new-feature`
3. Commit your changes: `git commit -am 'Add some feature'`
4. Push to the branch: `git push origin my-new-feature`
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Kevin "KTOWN" Townsend for the Arduino_BME280 library
* Marco Durante for the basic energy-saving setup: https://bitbucket.org/2msd/d1mini_sht30_mqtt/src/master/d1mini_sht30_mqtt.ino
