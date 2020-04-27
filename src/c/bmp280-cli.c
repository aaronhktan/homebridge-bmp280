#include "bmp280.h"

#include <unistd.h>

int main(int argc, char **argv) {
  int rv = BMP280_init("/dev/spidev0.1");
  if (rv) {
    printf("Failed to init BMP280\n");
  }

  uint8_t standby, coefficient;
  BMP280_get_config(&standby, &coefficient);
  printf("Standby: 0x%x, Coefficient: 0x%x\n", standby, coefficient);

  uint8_t osrs_p, osrs_t, mode;
  BMP280_get_ctrl_meas(&osrs_p, &osrs_t, &mode);
  printf("osrs_p: 0x%x, osrs_t: 0x%x, mode: 0x%x\n", osrs_p, osrs_t, mode);

  for (int counter = 0; counter < 60; counter++) {
    double pressure, temperature;
    int rv = BMP280_measure(&pressure, &temperature);
    printf("Temperature: %f, Pressure: %f, rv: %d\n", temperature, pressure, rv);
    usleep(1000000);
  }

  BMP280_deinit();
}
