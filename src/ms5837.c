#include "ms5837.h"
int32_t temperature;
int32_t pressure;
long deltaTemp;

// returns presure in units ex 100 gives you pascals
double pressureMS5837(float units) {
  return getMS5837Pressure()*units;
}

// returns temperature in c
double temperatureMS5837() {
	return getMS5837Temp()/100.0;
}

// returns depth in cm
double depthMS5837() {
	return (pressureMS5837(100.0)-101300)/(fluidDensity*9.80665)*100;
}

// gets temp in mili c
long readMS5837Temp(){
  getRawValues();
  deltaTemp = ms5837Raw.temperature - msCalibrationValue[5] * pow(2,8);
  return 2000 + deltaTemp * msCalibrationValue[6] / pow(2,23);
}

// gets presure in mbar
long readMS5837Pressure(){
  getMS5837Temp();
  int64_t offset = msCalibrationValue[2] * pow(2,16) + (msCalibrationValue[4]*deltaTemp)/pow(2,7);
  int64_t sensitivity = msCalibrationValue[1] * pow(2,15) + (msCalibrationValue[3]*deltaTemp)/pow(2,8);
  long msPresure = ((rawMSPres * sensitivity / pow(2,21) - offset)/pow(2,13))/10;
  return msPresure;
}

// must be called to set up sensor, should be called at setup
int initMS5837(){
  // Reset the MS5837, per datasheet
	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_RESET);
	Wire.endTransmission();

	// Wait for reset to complete
	delay(10);

	// Read calibration values and CRC
	for (uint8_t i=0; i<7; i++) {
		Wire.beginTransmission(MS5837_ADDR);
		Wire.write(MS5837_PROM_READ+i*2);
		Wire.endTransmission();

		Wire.requestFrom(MS5837_ADDR,2);
		msCalibrationValue[i] = (Wire.read() << 8) | Wire.read();
	}

  // Verify that data is correct with CRC
	uint8_t crcRead = msCalibrationValue[0] >> 12;
	uint8_t crcCalculated = crc4(msCalibrationValue);
	if ( crcCalculated != crcRead ) {
		return -1;
	}

	return 0;
}

int readRawMS5837() {

	// Request Raw Pressure conversion
	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_CONVERT_D1_8192);
	Wire.endTransmission();

  // Max conversion time per datasheet
	delay(20);

	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_ADC_READ);
	Wire.endTransmission();

 	Wire.requestFrom(MS5837_ADDR,3);
	ms5837Raw.pressure = 0;
	ms5837Raw.pressure = Wire.read();
	ms5837Raw.pressure = (ms5837Raw.pressure << 8) | Wire.read();
	ms5837Raw.pressure = (ms5837Raw.pressure << 8) | Wire.read();

	// Request Raw Temperature conversion
	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_CONVERT_D2_8192);
	Wire.endTransmission();

	delay(20); // Max conversion time per datasheet

	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_ADC_READ);
	Wire.endTransmission();

	Wire.requestFrom(MS5837_ADDR,3);
	ms5837Raw.temperature = 0;
	ms5837Raw.temperature = Wire.read();
	ms5837Raw.temperature = (ms5837Raw.temperature << 8) | Wire.read();
	ms5837Raw.temperature = (ms5837Raw.temperature << 8) | Wire.read();
}

uint8_t crc4(uint16_t n_prom[]) {
	uint16_t n_rem = 0;

	n_prom[0] = ((n_prom[0]) & 0x0FFF);
	n_prom[7] = 0;

	for ( uint8_t i = 0 ; i < 16; i++ ) {
		if ( i%2 == 1 ) {
			n_rem ^= (uint16_t)((n_prom[i>>1]) & 0x00FF);
		} else {
			n_rem ^= (uint16_t)(n_prom[i>>1] >> 8);
		}
		for ( uint8_t n_bit = 8 ; n_bit > 0 ; n_bit-- ) {
			if ( n_rem & 0x8000 ) {
				n_rem = (n_rem << 1) ^ 0x3000;
			} else {
				n_rem = (n_rem << 1);
			}
		}
	}

	n_rem = ((n_rem >> 12) & 0x000F);

	return n_rem ^ 0x00;
}