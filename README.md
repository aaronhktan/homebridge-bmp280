# Homebridge Plugin for BMP280

This is a Homebridge plugin for BMP280 temperature and barometric pressure sensor, working on the Raspberry Pi 3.

It communicates with the sensor using Linux's user-mode SPI API, spidev.

<img src="/docs/eve.png?raw=true" style="margin: 5px"> <img src="/docs/home.png?raw=true" style="margin: 5px">

## Configuration
**Before running this plugin, you must add the `homebridge` user to the `spi` group so Homebridge can access the SPI dev interface. `sudo adduser homebridge spi`**

| Field name           | Description                                                | Type / Unit    | Default value       | Required? |
| -------------------- |:-----------------------------------------------------------|:--------------:|:-------------------:|:---------:|
| name                 | Name of the accessory                                      | string         | —                   | Y         |
| spidevInterface      | Spidev interface in `/dev/` that the sensor is mounted at  | string         | /dev/spidev0.1      | N         |
| enableFakeGato       | Enable storing data in Eve Home app                        | bool           | false               | N         |
| fakeGatoStoragePath  | Path to store data for Eve Home app                        | string         | (fakeGato default)  | N         |
| enableMQTT           | Enable sending data to MQTT server                         | bool           | false               | N         |
| mqttConfig           | Object containing some config for MQTT                     | object         | —                   | N         |

The mqttConfig object is **only required if enableMQTT is true**, and is defined as follows:

| Field name           | Description                                      | Type / Unit  | Default value       | Required? |
| -------------------- |:-------------------------------------------------|:------------:|:-------------------:|:---------:|
| url                  | URL of the MQTT server, must start with mqtt://  | string       | —                   | Y         |
| temperatureTopic     | MQTT topic to which temperature data is sent     | string       | bmp280/temeprature  | N         |
| pressureTopic        | MQTT topic to which pressure data is sent        | string       | bmp280/pressure     | N         |

### Example Configuration

```
{
  "bridge": {
    "name": "Homebridge",
    "username": "XXXX",
    "port": XXXX
  },

  "accessories": [
    {
      "accessory": "BMP280",
      "name": "BMP280",
      "enableFakeGato": true,
      "enableMQTT": true,
      "mqtt": {
          "url": "mqtt://192.168.0.38",
          "pressureTopic": "bmp280/pressure",
          "temperatureTopic": "bmp280/temperature"
      }
    }
  ]
}
```

## Project Layout

- All things required by Node are located at the root of the repository (i.e. package.json and index.js).
- The rest of the code is in `src`, further split up by language.
  - `c` contains the C code that runs on the device to communicate with the sensor. It also contains a simple program to check that the sensor is attached and readable.
  - `binding` contains the C++ code using node-addon-api to communicate between C and the Node.js runtime.
  - `js` contains a simple project that tests that the binding between C/Node.js is correctly working. It also contains a custom characteristic that allows Eve to keep barometric air pressure data.
  - `python` contains a sanity test to make sure that the sensor is actually connected and is a BMP280.
