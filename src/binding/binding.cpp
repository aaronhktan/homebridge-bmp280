extern "C" {
#include "bmp280.h"
}

#include "binding_utils.h"

#include <napi.h>

#include <string>

Napi::Object init(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  std::string spiAdaptor{"/dev/spidev0.1"}; 
  if (info.Length() == 1) {
    spiAdaptor = static_cast<std::string>(info[0].As<Napi::String>());
  }

  int err = BMP280_init(spiAdaptor.c_str());
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not initialize BMP280 module; are you using the right port?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "returnCode"), Napi::Number::New(env, err));
  return returnObject;
}

Napi::Object deinit(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  int err = BMP280_deinit();
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not deinitialize BMP280 module; are you using the right port?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "returnCode"), Napi::Number::New(env, err));
  return returnObject;
}

Napi::Object measure(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  double pressure, temperature;
  int err = BMP280_measure(&pressure, &temperature);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not measure temperature and pressure from BMP280 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "pressure"), Napi::Number::New(env, pressure));
  returnObject.Set(Napi::String::New(env, "temperature"), Napi::Number::New(env, temperature));
  return returnObject;
}

Napi::Object get_config(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  uint8_t standby, filter_coefficient;
  int err = BMP280_get_config(&standby, &filter_coefficient);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not get config from BMP280 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "standby"), Napi::Number::New(env, standby));
  returnObject.Set(Napi::String::New(env, "filter_coefficient"), Napi::Number::New(env, filter_coefficient));
  return returnObject;
}

Napi::Object get_ctrl_meas(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  uint8_t osrs_p, osrs_t, mode;
  int err = BMP280_get_ctrl_meas(&osrs_p, &osrs_t, &mode);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not get measurement controls from BMP280 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "osrs_p"), Napi::Number::New(env, osrs_p));
  returnObject.Set(Napi::String::New(env, "osrs_t"), Napi::Number::New(env, osrs_t));
  returnObject.Set(Napi::String::New(env, "mode"), Napi::Number::New(env, mode));
  return returnObject;
}

Napi::Object get_status(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  uint8_t measuring, im_update;
  int err = BMP280_get_status(&measuring, &im_update);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not get status from BMP280 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "measuring"), Napi::Number::New(env, measuring));
  returnObject.Set(Napi::String::New(env, "im_update"), Napi::Number::New(env, im_update));
  return returnObject;
}

Napi::Object set_config(const Napi::CallbackInfo &info) {
  uint8_t standby = static_cast<uint32_t>(info[0].As<Napi::Number>()) & 0xFF;
  uint8_t filter_coefficient = static_cast<uint32_t>(info[1].As<Napi::Number>()) & 0xFF;
  Napi::Env env = info.Env();

  int err = BMP280_set_config(standby, filter_coefficient);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not set config for BMP280 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "returnCode"), Napi::Number::New(env, err));
  return returnObject;
}

Napi::Object set_ctrl_meas(const Napi::CallbackInfo &info) {
  uint8_t osrs_p = static_cast<uint32_t>(info[0].As<Napi::Number>()) & 0xFF;
  uint8_t osrs_t = static_cast<uint32_t>(info[1].As<Napi::Number>()) & 0xFF;
  uint8_t mode = static_cast<uint32_t>(info[2].As<Napi::Number>()) & 0xFF;
  Napi::Env env = info.Env();

  int err = BMP280_set_ctrl_meas(osrs_p, osrs_t, mode);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not set measurement controls from BMP280 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "returnCode"), Napi::Number::New(env, err));
  return returnObject;
}

Napi::Object get_chip_id(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  uint8_t chip_id;
  int err = BMP280_get_chip_id(&chip_id);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not get word ID from BMP280 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "chip"), Napi::Number::New(env, chip_id));
  return returnObject;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "init"),
              Napi::Function::New(env, init));
  exports.Set(Napi::String::New(env, "deinit"),
              Napi::Function::New(env, deinit));
  exports.Set(Napi::String::New(env, "measure"),
              Napi::Function::New(env, measure));
  exports.Set(Napi::String::New(env, "getConfig"),
              Napi::Function::New(env, get_config));
  exports.Set(Napi::String::New(env, "getCtrlMeas"),
              Napi::Function::New(env, get_ctrl_meas));
  exports.Set(Napi::String::New(env, "getStatus"),
              Napi::Function::New(env, get_status));
  exports.Set(Napi::String::New(env, "setConfig"),
              Napi::Function::New(env, set_config));
  exports.Set(Napi::String::New(env, "setCtrlMeas"),
              Napi::Function::New(env, get_ctrl_meas));
  exports.Set(Napi::String::New(env, "getChipID"),
              Napi::Function::New(env, get_chip_id));
  return exports;
}

NODE_API_MODULE(homebridgebmp280, Init)
