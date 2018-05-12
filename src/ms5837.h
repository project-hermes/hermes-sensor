#ifndef MS5837
#define MS5837

#include "application.h"

#define MS5837_ADDR               0x76
#define MS5837_RESET              0x1E
#define MS5837_ADC_READ           0x00
#define MS5837_PROM_READ          0xA0
#define MS5837_CONVERT_D1_8192    0x4A
#define MS5837_CONVERT_D2_8192    0x5A

int initPressureSensor();
void readDepth();
float temperatureMS5837();
float depthMS5837();
float altitudeMS5837();

#endif
