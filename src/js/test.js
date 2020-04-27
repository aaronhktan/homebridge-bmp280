const BMP280 = require('bindings')('homebridge-bmp280');

function measure() {
  console.log(BMP280.measure());
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function test() {
  BMP280.init();
  console.log(BMP280.getChipID());

  for (i = 0; i < 60; i++) {
    console.log(BMP280.measure());
    await(sleep(1000));
  }

  BMP280.deinit();
}

test();

