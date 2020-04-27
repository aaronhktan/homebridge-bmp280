const inherits = require('util').inherits;

var Service, Characteristic;

module.exports = (homebridge) => {
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;

  var CustomCharacteristic = {};

  CustomCharacteristic.AtmosphericPressureLevel = function() {
    Characteristic.call(this, 'Air Pressure', 'E863F10F-079E-48FF-8F27-9C2605A29F52');
    this.setProps({
      format: Characteristic.Formats.UINT8,
      unit: "mbar",
      minValue: 800,
      maxValue: 1200,
      minStep: 1,
      perms: [ Characteristic.Perms.READ, Characteristic.Perms.NOTIFY ],
    });
    this.value = this.getDefaultValue();
  }
  CustomCharacteristic.AtmosphericPressureLevel.UUID = 'E863F10F-079E-48FF-8F27-9C2605A29F52';
  inherits(CustomCharacteristic.AtmosphericPressureLevel, Characteristic);

  return CustomCharacteristic;
}
