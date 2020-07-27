const BMP280 = require('bindings')('homebridge-bmp280');

const moment = require('moment'); // Time formatting
const mqtt = require('mqtt'); // MQTT client
const os = require('os'); // Hostname

var Service, Characteristic;
var CustomCharacteristic;
var FakeGatoHistoryService;

module.exports = (homebridge) => {
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;
  CustomCharacteristic = require('./src/js/customcharacteristic.js')(homebridge);
  FakeGatoHistoryService = require('fakegato-history')(homebridge);

  homebridge.registerAccessory("homebridge-bmp280", "BMP280", BMP280Accessory);
}

function BMP280Accessory(log, config) {
  // Load configuration from files
  this.log = log;
  this.displayName = config['name'];
  this.spidevInterface = config['spidev'] || '/dev/spidev0.1';
  this.enableFakeGato = config['enableFakeGato'] || false;
  this.fakeGatoStoragePath = config['fakeGatoStoragePath'];
  this.enableMQTT = config['enableMQTT'] || false;
  this.mqttConfig = config['mqtt'];

  // Internal variables to keep track of current temperature and pressure
  this._currentPressure = null;
  this._pressureSamples = [];
  this._pressureCumSum = 0;
  this._pressureCounter = 0;

  this._currentTemperature = null;
  this._temperatureSamples = [];
  this._temperatureCumSum = 0;
  this._temperatureCounter = 0;

  // Services
  let informationService = new Service.AccessoryInformation();
  informationService
    .setCharacteristic(Characteristic.Manufacturer, "Bosch")
    .setCharacteristic(Characteristic.Model, "BMP280")
    .setCharacteristic(Characteristic.SerialNumber, `${os.hostname}-${this.spidevInterface.split('/').pop()}`)
    .setCharacteristic(Characteristic.FirmwareRevision, require('./package.json').version);

  let temperatureService = new Service.TemperatureSensor();
  temperatureService.addCharacteristic(CustomCharacteristic.AtmosphericPressureLevel);

  this.informationService = informationService;
  this.temperatureService = temperatureService;

  // Start FakeGato for logging historical data
  if (this.enableFakeGato) {
    this.fakeGatoHistoryService = new FakeGatoHistoryService("weather", this, {
      storage: 'fs',
      folder: this.fakeGatoStoragePath
    });
  }

  // Set up MQTT client
  if (this.enableMQTT) {
    this.setUpMQTT();
  }

  // Periodically update the values
  this.setupBMP280();
  this.refreshData();
  setInterval(() => this.refreshData(), 1000);
}

// Error checking and averaging when saving pressure and temperature
Object.defineProperty(BMP280Accessory.prototype, "pressure", {
  set: function(pressureReading) {
    // Calculate running average of pressure over the last 30 samples
    this._pressureCounter++;
    if (this._pressureSamples.length == 30) {
      let firstSample = this._pressureSamples.shift();
      this._pressureCumSum -= firstSample;
    }
    this._pressureSamples.push(pressureReading);
    this._pressureCumSum += pressureReading;

    // Update current pressure value, and publish to MQTT/FakeGato once every 30 seconds
    if (this._pressureCounter == 30) {
      this._pressureCounter = 0;
      this._currentPressure = this._pressureCumSum / 30 / 100; // Convert from pascals to mbar
      this.log(`Pressure: ${this._currentPressure}`);

      this.temperatureService.getCharacteristic(CustomCharacteristic.AtmosphericPressureLevel)
        .updateValue(this._currentPressure);
 
      if (this.enableFakeGato) {
        this.fakeGatoHistoryService.addEntry({
          time: moment().unix(),
          pressure: this._currentPressure,
        });
      }
 
      if (this.enableMQTT) {
        this.publishToMQTT(this.pressureTopic, this._currentPressure);
      }
    }
  },

  get: function() {
    return this._currentPressure;
  }
});

Object.defineProperty(BMP280Accessory.prototype, "temperature", {
  set: function(temperatureReading) {
    // Calculate running average of temperature over the last 30 samples
    this._temperatureCounter++;
    if (this._temperatureSamples.length == 30) {
      let firstSample = this._temperatureSamples.shift();
      this._temperatureCumSum -= firstSample;
    }
    this._temperatureSamples.push(temperatureReading);
    this._temperatureCumSum += temperatureReading;

    // Publish temperature to MQTT every 30 seconds
    if (this._temperatureCounter == 30) {
      this._temperatureCounter = 0;
      this._currentTemperature = this._temperatureCumSum / 30;
      this.log(`Temperature: ${this._currentTemperature}`);

      this.temperatureService.getCharacteristic(Characteristic.CurrentTemperature)
       .updateValue(this._currentTemperature);

      if (this.enableFakeGato) {
        this.fakeGatoHistoryService.addEntry({
          time: moment().unix(),
          temp: this._currentTemperature,
        });
      }

      if (this.enableMQTT) {
        this.publishToMQTT(this.temperatureTopic, this._currentTemperature);
      }
    }
  },

  get: function() {
    return this._currentTemperature;
  }
});

// Sets up MQTT client based on config loaded in constructor
BMP280Accessory.prototype.setUpMQTT = function() {
  if (!this.enableMQTT) {
    this.log.info("MQTT not enabled");
    return;
  }

  if (!this.mqttConfig) {
    this.log.error("No MQTT config found");
    return;
  }

  this.mqttUrl = this.mqttConfig.url;
  this.temperatureTopic = this.mqttConfig.temperatureTopic || 'BMP280/temperature';
  this.pressureTopic = this.mqttConfig.pressureTopic || 'BMP280/pressure';

  this.mqttClient = mqtt.connect(this.mqttUrl);
  this.mqttClient.on("connect", () => {
    this.log(`MQTT client connected to ${this.mqttUrl}`);
  });
  this.mqttClient.on("error", (err) => {
    this.log(`MQTT client error: ${err}`);
    client.end();
  });
}

// Sends data to MQTT broker; must have called setupMQTT() previously
BMP280Accessory.prototype.publishToMQTT = function(topic, value) {
  if (!this.mqttClient.connected || !topic) {
    this.log.error("MQTT client not connected, or no topic or value for MQTT");
    return;
  }
  this.mqttClient.publish(topic, String(value));
}

// Set up sensor; checks that SPI interface is available and device is ready
BMP280Accessory.prototype.setupBMP280 = function() {
  data = BMP280.init(this.spidevInterface);
  if (data.hasOwnProperty('errcode')) {
    this.log(`Error: ${data.errmsg}`);
  }
}

// Read pressure and temperature from sensor
BMP280Accessory.prototype.refreshData = function() {
  let data;
  data = BMP280.measure();

  if (data.hasOwnProperty('errcode')) {
    this.log(`Error: ${data.errmsg}`);
    // Updating a value with Error class sets status in HomeKit to 'Not responding'
    this.temperatureService.getCharacteristic(Characteristic.CurrentTemperature)
      .updateValue(Error(data.errmsg));
    this.temperatureService.getCharacteristic(CustomCharacteristic.AtmosphericPressureLevel)
      .updateValue(Error(data.errmsg));
    return;
  }
  
  this.log.debug(`Read: Pressure: ${data.pressure}pa, Temperature: ${data.temperature}C`); 
  this.pressure = data.pressure;
  this.temperature = data.temperature;
}

BMP280Accessory.prototype.getServices = function() {
  return [this.informationService,
          this.temperatureService,
          this.fakeGatoHistoryService];
}

