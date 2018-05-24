#ifndef TSYS01_H
#define TSYS01_H

#include "math.h"
#include "application.h"

#define TSYS01_ADDR                        0x77
#define TSYS01_RESET                       0x1E
#define TSYS01_ADC_READ                    0x00
#define TSYS01_ADC_TEMP_CONV               0x48
#define TSYS01_PROM_READ                   0XA0

uint16_t tsCalibrationValue[8];
bool tsys01IsInit;

int initTSYS01();
float readTempTSYS01();
float calculateTSYS01(uint32_t);

#endif
