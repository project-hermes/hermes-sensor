#ifndef MS5837_H
#define MS5837_H

#include "application.h"
#include "math.h"

#define MS5837_ADDR               0x76
#define MS5837_RESET              0x1E
#define MS5837_ADC_READ           0x00
#define MS5837_PROM_READ          0xA0
#define MS5837_CONVERT_D1_8192    0x4A
#define MS5837_CONVERT_D2_8192    0x5A

struct ms5837Raw{
  uint32_t temperature;
  uint32_t pressure;
};

const float Pa = 100.0f;
const float bar = 0.001f;
const float mbar = 1.0f;

float fluidDensity = 1029;
uint16_t msCalibrationValue[8];

int initPressureSensor();
double temperatureMS5837();
double depthMS5837();
double altitudeMS5837();
void calculateDepth(uint32_t data1, uint32_t data2);
uint8_t crc4(uint16_t n_prom[]);

#endif
