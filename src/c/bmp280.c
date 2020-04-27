#include "bmp280.h"

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static uint8_t spi_mode = 0;
static uint8_t spi_bits = 8;
static uint32_t spi_speed = 50000;
static uint16_t spi_delay = 100;
static int spi_fd;

// Constants for compensation, unique to each chip
uint16_t dig_T1;
int16_t dig_T2, dig_T3;
uint16_t dig_P1;
int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

int32_t t_fine;

static int read_bytes(int fd, uint8_t reg, uint8_t *rx, int len) {
  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  
  uint8_t tx[len + 1];
  memset(tx, 0, len + 1);
  tx[0] = (reg | 0x80) & 0xFF; // BMP280 uses the seventh bit as a R/W indicator

  tr.tx_buf = (unsigned long)tx;
  tr.rx_buf = (unsigned long)rx;
  tr.len = len + 1;
  tr.delay_usecs = spi_delay;
  tr.speed_hz = spi_speed;
  tr.bits_per_word = spi_bits;

  return !(ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == (len + 1));
}

static int write_bytes(int fd, uint8_t reg, uint8_t *tx_buf, int len) {
  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  
  uint8_t tx[len + 1];
  tx[0] = reg & ~0x80;
  memcpy(&tx[1], tx_buf, len);
  
  uint8_t rx[len + 1];

  tr.tx_buf = (unsigned long)tx;
  tr.rx_buf = (unsigned long)rx;
  tr.len = len + 1;
  tr.delay_usecs = spi_delay;
  tr.speed_hz = spi_speed;
  tr.bits_per_word = spi_bits;

  return !(ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == (len + 1));
}

// Calibration data is unique to each chip and must be read
// after startup so compensation for temperature and pressure
// can be applied.
static int read_calibration(void) {
  uint8_t t1[3], t2[3], t3[3];
  uint8_t p1[3], p2[3], p3[3], p4[3], p5[3], p6[3], p7[3], p8[3], p9[3];

  int rv = 0;
  rv |= read_bytes(spi_fd, BMP280_DIG_T1_REG, t1, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_T2_REG, t2, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_T3_REG, t3, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_P1_REG, p1, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_P2_REG, p2, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_P3_REG, p3, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_P4_REG, p4, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_P5_REG, p5, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_P6_REG, p6, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_P7_REG, p7, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_P8_REG, p8, 2);
  rv |= read_bytes(spi_fd, BMP280_DIG_P9_REG, p9, 2);

  dig_T1 = (t1[2] << 8 | t1[1]) & 0xFFFF;
  dig_T2 = (t2[2] << 8 | t2[1]) & 0xFFFF;
  dig_T3 = (t3[2] << 8 | t3[1]) & 0xFFFF;
  dig_P1 = (p1[2] << 8 | p1[1]) & 0xFFFF;
  dig_P2 = (p2[2] << 8 | p2[1]) & 0xFFFF;
  dig_P3 = (p3[2] << 8 | p3[1]) & 0xFFFF;
  dig_P4 = (p4[2] << 8 | p4[1]) & 0xFFFF;
  dig_P5 = (p5[2] << 8 | p5[1]) & 0xFFFF;
  dig_P6 = (p6[2] << 8 | p6[1]) & 0xFFFF;
  dig_P7 = (p7[2] << 8 | p7[1]) & 0xFFFF;
  dig_P8 = (p8[2] << 8 | p8[1]) & 0xFFFF;
  dig_P9 = (p9[2] << 8 | p9[1]) & 0xFFFF;

  debug_print(stdout, "0x%x, 0x%x, 0x%x\n", dig_T1, dig_T2, dig_T3);
  debug_print(stdout, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
      dig_P1, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9);

  return rv;
}

// From the datasheet
static int compensate_pressure(int32_t p_in, double *p_out) {
  int64_t var1, var2, p;
  var1 = ((int64_t)t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)dig_P6;
  var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
  var2 = var2 + (((int64_t)dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
  var1 = ((((int64_t)1) << 47) + var1) * ((int64_t)dig_P1) >> 33;

  if (!var1) {
    debug_print(stdout, "%s\n", "Pressure compensation: var1 == 0");
    return 0;
  }
  
  p = 1048576 - p_in;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
  *p_out = (double)((uint32_t)p / 256.0);

  return NO_ERROR;
}

// Also from the datasheet
static int compensate_temperature(int32_t t_in, double *t_out) {
  int32_t var1, var2, temperature;

  var1 = ((((t_in >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  var2 = ((((t_in >> 4) - ((int32_t)dig_T1)) * ((t_in >> 4) - ((int32_t)dig_T1))) >> 12 *
      ((int32_t)dig_T3)) >> 14;
  t_fine = var1 + var2;
  temperature = (t_fine * 5 + 128) >> 8;
  *t_out = temperature / 100.0;

   return NO_ERROR;
}

int BMP280_init(const char *spi_adaptor) {
  spi_fd = open(spi_adaptor, O_RDWR);
  if (spi_fd < 0) {
    return ERROR_DEVICE;
  }

  // Set settings for SPI
  int rv = 0;
  rv |= ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode);
  rv |= ioctl(spi_fd, SPI_IOC_RD_MODE, &spi_mode);
  rv |= ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
  rv |= ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits);
  rv |= ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
  rv |= ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
  if (rv == -1) {
    return ERROR_SPI;
  }

  uint8_t id = 0;
  rv |= BMP280_get_chip_id(&id);
  if (id != BMP280_CHIP_ID) {
    debug_print(stderr, "Chip ID 0x%x does not match 0x%x\n", id, BMP280_CHIP_ID);
    return ERROR_DEVICE;
  } else if (rv) {
    return rv;
  }

  // Read compensation parameters
  rv |= read_calibration();
  if (rv) {
    return rv;
  }

  // Set default configuration
  rv |= BMP280_set_config(MS250, FILTER_16);
  rv |= BMP280_set_ctrl_meas(T_OVERSAMPLE_1, P_OVERSAMPLE_4, NORMAL);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not set default config");
    return ERROR_SPI;
  }

  return NO_ERROR;
}

int BMP280_deinit(void) {
  close(spi_fd);
  return NO_ERROR;
}

int BMP280_measure(double *pressure_out,
                   double *temperature_out) {
  int rv = 0;
  double pressure = 0, temperature = 0;

  // Read temperature data
  uint8_t rx_temp[4];
  read_bytes(spi_fd, BMP280_TEMP_MSB, rx_temp, 3);
  int32_t t = (rx_temp[1] << 16 | rx_temp[2] << 8 | rx_temp[3]) >> 4;
  rv |= compensate_temperature(t, &temperature);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not compensate temperature");
    return rv;
  }

  // Read pressure data
  uint8_t rx_hum[4];
  read_bytes(spi_fd, BMP280_PRESS_MSB, rx_hum, 3);
  int32_t p = (rx_hum[1] << 16 | rx_hum[2] << 8 | rx_hum[3]) >> 4;
  rv |= compensate_pressure(p, &pressure);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not compensate pressure");
    return rv;
  }

  *pressure_out = pressure;
  *temperature_out = temperature;
  return NO_ERROR;
}

int BMP280_get_config(uint8_t *standby_out,
                      uint8_t *filter_coefficient_out) {
  int rv = 0;
  uint8_t config_rx[2];

  rv = read_bytes(spi_fd, BMP280_CONFIG_REG, config_rx, 1);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not read config");
    return rv;
  }

  *standby_out = config_rx[1] & 0xE0;
  *filter_coefficient_out = config_rx[1] & 0x1C;
  return NO_ERROR;
}

int BMP280_get_ctrl_meas(uint8_t *osrs_p_out,
                         uint8_t *osrs_t_out,
                         uint8_t *mode_out) {
  int rv = 0;
  uint8_t ctrl_meas_rx[2];

  rv = read_bytes(spi_fd, BMP280_CTRL_MEAS_REG, ctrl_meas_rx, 1);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not get ctrl meas");
    return rv;
  }

  *osrs_p_out = ctrl_meas_rx[1] & 0x1C;
  *osrs_t_out = ctrl_meas_rx[1] & 0xE0;
  *mode_out = ctrl_meas_rx[1] & 0x03;
  return NO_ERROR;
}

int BMP280_get_status(uint8_t *measuring_out,
                      uint8_t *im_update_out) {
  int rv = 0;
  uint8_t status_rx[2];

  rv = read_bytes(spi_fd, BMP280_STATUS_REG, status_rx, 2);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not get status");
    return rv;
  }

  *measuring_out = status_rx[1] & 0x08;
  *im_update_out = status_rx[1] & 0x01;
  return NO_ERROR;
}

int BMP280_get_chip_id(uint8_t *id_out) {
  int rv = 0;
  uint8_t id_rx[2];

  rv = read_bytes(spi_fd, BMP280_ID_REG, id_rx, 1);
  if (rv) {
    debug_print(stderr, "Return value from read_bytes is %d\n", rv);
    return rv;
  }

  *id_out = id_rx[1] & 0xFF;
  return NO_ERROR;
}

int BMP280_set_config(uint8_t standby,
                      uint8_t filter_coefficient) {
  uint8_t config_tx[1] = { (standby | filter_coefficient) & 0xFE };
  return write_bytes(spi_fd, BMP280_CONFIG_REG, config_tx, 1);
}

int BMP280_set_ctrl_meas(uint8_t osrs_p,
                         uint8_t osrs_t,
                         uint8_t mode) {
  uint8_t ctrl_meas_tx[1] = { (osrs_p | osrs_t | mode) };
  return write_bytes(spi_fd, BMP280_CTRL_MEAS_REG, ctrl_meas_tx, 1);
}

