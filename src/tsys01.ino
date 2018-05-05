#include "tsys01.h"
#include "math.h"
#include "application.h"

void calibrate(uint16_t *calibrationValue){
	// Reset the TSYS01, per datasheet
	Wire.beginTransmission(TSYS01_ADDR);
	Wire.write(TSYS01_RESET);
	Wire.endTransmission();

	delay(10);

	// Read calibration values
	for (uint8_t i=0; i<8; i++) {
		Wire.beginTransmission(TSYS01_ADDR);
		Wire.write(TSYS01_PROM_READ+i*2);
		Wire.endTransmission();

		Wire.requestFrom(TSYS01_ADDR,2);
		calibrationValue[i] = (Wire.read() << 8) | Wire.read();
	}
}

float readTsysTemperature() {

  uint16_t calibrationValue[8];
  calibrate(calibrationValue);

	Wire.beginTransmission(TSYS01_ADDR);
	Wire.write(TSYS01_ADC_TEMP_CONV);
	Wire.endTransmission();

  // Max conversion time(ms) per datasheet
	delay(10);

	Wire.beginTransmission(TSYS01_ADDR);
	Wire.write(TSYS01_ADC_READ);
	Wire.endTransmission();

	Wire.requestFrom(TSYS01_ADDR,3);
	uint32_t data = 0;
	data = Wire.read();
	data = (data << 8) | Wire.read();
	data = (data << 8) | Wire.read();

	return calculate(calibrationValue,data);
}


float calculate(uint16_t calibrationValue[], uint32_t data) {
	uint32_t adc = data/256;

  float temp = (-2) * float(calibrationValue[1]) / 1000000000000000000000.0f * pow(adc,4) +
    4 * float(calibrationValue[2]) / 10000000000000000.0f * pow(adc,3) +
    (-2) * float(calibrationValue[3]) / 100000000000.0f * pow(adc,2) +
      1 * float(calibrationValue[4]) / 1000000.0f * adc +
    (-1.5) * float(calibrationValue[5]) / 100 ;

  return temp;
}
