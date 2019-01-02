#ifndef ms5837_h
#define ms5837_h

#include "application.h"

class ms5837
{
  public:
    ms5837();
    float readTemp();
    float readPressure();
    float readDepth();

  private:
    const short _addr = 0x76;
    const short _reset = 0x1E;
    const short _adc_read = 0x00;
    const short _prom_read = 0xA0;
    const short _d1_8192 = 0x4A;
    const short _d2_8192 = 0x5A;

    Logger _log;

    uint32_t _rawTemp;
    uint32_t _rawPres;

    int32_t _deltaTemp;
    int32_t _temp;
    int32_t _pressure;

    const int _fluidDensity = 1029;
    uint16_t _msCalibrationValue[8];

    const float _pa = 100.0f;

    uint8_t crc4(uint16_t[]);
    void readValues();
    void computeTemp();
    void computePressure();
};

#endif