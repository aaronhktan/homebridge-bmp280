#ifndef BMP280
#define BMP280

#include <stdint.h>
#include <stdio.h>

// Print function that only prints if DEBUG is defined
#ifdef DEBUG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif
#define debug_print(fd, fmt, ...) \
            do { if (DEBUG_PRINT) fprintf(fd, fmt, __VA_ARGS__); } while (0)


enum Error {
  NO_ERROR,
  ERROR_DEVICE,         // Couldn't find device
  ERROR_DRIVER,         // Driver failed to init
  ERROR_INVAL,          // Invalid argument
  ERROR_SPI             // SPI driver failed to read or write data
};

// Chip defines
// Standby time (not actively measuring) in milliseconds
enum Standby {
  MS0_5     = 0x00,
  MS62_5    = 0x20,
  MS125     = 0x40,
  MS250     = 0x60,
  MS500     = 0x80,
  MS1000    = 0xA0,
  MS2000    = 0xC0,
  MS4000    = 0xE0
};

enum Filter_Coefficient {
  FILTER_0  = 0x00, // 1 sample
  FILTER_2  = 0x04, // 2 samples
  FILTER_4  = 0x08, // 5 samples
  FILTER_8  = 0x0C, // 11 samples
  FILTER_16 = 0x10  // 22 samples
};

enum Temperature_Oversampling {
  T_OVERSAMPLE_SKIP = 0x00,
  T_OVERSAMPLE_1    = 0x20,
  T_OVERSAMPLE_2    = 0x40,
  T_OVERSAMPLE_4    = 0x60,
  T_OVERSAMPLE_8    = 0x80,
  T_OVERSAMPLE_16    = 0xA0
};

enum Pressure_Oversampling {
  P_OVERSAMPLE_SKIP = 0x00,
  P_OVERSAMPLE_1    = 0x04,
  P_OVERSAMPLE_2    = 0x08,
  P_OVERSAMPLE_4    = 0x0C,
  P_OVERSAMPLE_8    = 0x10,
  P_OVERSAMPLE_16    = 0x14
};

enum Mode {
  SLEEP             = 0x00,
  FORCED            = 0x01,
  NORMAL            = 0x03
};

#define BMP280_MEASURING 0x08
#define BMP280_IM_UPDATE 0x01
#define BMP280_CHIP_ID 0x58

#define BMP280_PRESS_MSB  0xF7
#define BMP280_PRESS_LSB  0xF8
#define BMP280_PRESS_XLSB 0xF9

#define BMP280_TEMP_MSB   0xFA
#define BMP280_TEMP_LSB   0xFB
#define BMP280_TEMP_XLSB  0xFC

#define BMP280_CONFIG_REG 0xF5
#define BMP280_CTRL_MEAS_REG 0xF4
#define BMP280_STATUS_REG 0xF3
#define BMP280_RESET_REG  0xE0
#define BMP280_ID_REG     0xD0

#define BMP280_DIG_T1_REG 0x88
#define BMP280_DIG_T2_REG 0x8A
#define BMP280_DIG_T3_REG 0x8C
#define BMP280_DIG_P1_REG 0x8E
#define BMP280_DIG_P2_REG 0x90
#define BMP280_DIG_P3_REG 0x92
#define BMP280_DIG_P4_REG 0x94
#define BMP280_DIG_P5_REG 0x96
#define BMP280_DIG_P6_REG 0x98
#define BMP280_DIG_P7_REG 0x9A
#define BMP280_DIG_P8_REG 0x9C
#define BMP280_DIG_P9_REG 0x9E

// Set up and tear down BMP280 interface
int BMP280_init(const char *spi_adaptor);
int BMP280_deinit(void);

// Fetch data from BMP280
int BMP280_measure(double *pressure_out,
                   double *temperature_out);
int BMP280_get_config(uint8_t *standby_out,
                      uint8_t *filter_coefficient_out);
int BMP280_get_ctrl_meas(uint8_t *osrs_p_out,
                         uint8_t *osrs_t_out,
                         uint8_t *mode_out);
int BMP280_get_status(uint8_t *measuring_out,
                      uint8_t *im_update_out);
int BMP280_get_chip_id(uint8_t *id_out);

// Set data in BMP280
int BMP280_set_config(uint8_t standby,
                      uint8_t filter_coefficient);
int BMP280_set_ctrl_meas(uint8_t osrs_p,
                         uint8_t osrs_t,
                         uint8_t mode);

#endif // BMP280

