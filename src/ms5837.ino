#include "ms5837.h"
#include "math.h"

float fluidDensity = 1029;
uint16_t calibrationValue[8];
int32_t temperature;
int32_t pressure;

const float Pa = 100.0f;
const float bar = 0.001f;
const float mbar = 1.0f;

const uint8_t MS5837_30BA = 0;
const uint8_t MS5837_02BA = 1;

uint8_t _model = MS5837_30BA;

int initPressureSensor(){
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
		calibrationValue[i] = (Wire.read() << 8) | Wire.read();
	}

  // Verify that data is correct with CRC
	uint8_t crcRead = calibrationValue[0] >> 12;
	uint8_t crcCalculated = crc4(calibrationValue);
	if ( crcCalculated != crcRead ) {
		return -1;
	}

	return 0;
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

void readDepth() {
  uint32_t data1, data2;

	// Request D1 conversion
	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_CONVERT_D1_8192);
	Wire.endTransmission();

  // Max conversion time per datasheet
	delay(20);

	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_ADC_READ);
	Wire.endTransmission();

 	Wire.requestFrom(MS5837_ADDR,3);
	data1 = 0;
	data1 = Wire.read();
	data1 = (data1 << 8) | Wire.read();
	data1 = (data1 << 8) | Wire.read();

	// Request D2 conversion
	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_CONVERT_D2_8192);
	Wire.endTransmission();

	delay(20); // Max conversion time per datasheet

	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_ADC_READ);
	Wire.endTransmission();

	Wire.requestFrom(MS5837_ADDR,3);
	data2 = 0;
	data2 = Wire.read();
	data2 = (data2 << 8) | Wire.read();
	data2 = (data2 << 8) | Wire.read();

  calculateDepth(data1, data2);
}

void calculateDepth(uint32_t data1, uint32_t data2) {
	// Given C1-C6 and D1, D2, calculated temperature and pressure
	// Do conversion first and then second order temp compensation

	int32_t dT = 0;
	int64_t SENS = 0;
	int64_t OFF = 0;
	int32_t SENSi = 0;
	int32_t OFFi = 0;
	int32_t Ti = 0;
	int64_t OFF2 = 0;
	int64_t SENS2 = 0;

	// Terms called
	dT = data2-uint32_t(calibrationValue[5])*256l;
	if ( _model == MS5837_02BA ) {
		SENS = int64_t(calibrationValue[1])*65536l+(int64_t(calibrationValue[3])*dT)/128l;
		OFF = int64_t(calibrationValue[2])*131072l+(int64_t(calibrationValue[4])*dT)/64l;
		pressure = (data1*SENS/(2097152l)-OFF)/(32768l);
	} else {
		SENS = int64_t(calibrationValue[1])*32768l+(int64_t(calibrationValue[3])*dT)/256l;
		OFF = int64_t(calibrationValue[2])*65536l+(int64_t(calibrationValue[4])*dT)/128l;
		pressure = (data1*SENS/(2097152l)-OFF)/(8192l);
	}

	// Temp conversion
	temperature = 2000l+int64_t(dT)*calibrationValue[6]/8388608LL;

	//Second order compensation
	if ( _model == MS5837_02BA ) {
		if((temperature/100)<20){         //Low temp
			Serial.println("here");
			Ti = (11*int64_t(dT)*int64_t(dT))/(34359738368LL);
			OFFi = (31*(temperature-2000)*(temperature-2000))/8;
			SENSi = (63*(temperature-2000)*(temperature-2000))/32;
		}
	} else {
		if((temperature/100)<20){         //Low temp
			Ti = (3*int64_t(dT)*int64_t(dT))/(8589934592LL);
			OFFi = (3*(temperature-2000)*(temperature-2000))/2;
			SENSi = (5*(temperature-2000)*(temperature-2000))/8;
			if((temperature/100)<-15){    //Very low temp
				OFFi = OFFi+7*(temperature+1500l)*(temperature+1500l);
				SENSi = SENSi+4*(temperature+1500l)*(temperature+1500l);
			}
		}
		else if((temperature/100)>=20){    //High temp
			Ti = 2*(dT*dT)/(137438953472LL);
			OFFi = (1*(temperature-2000)*(temperature-2000))/16;
			SENSi = 0;
		}
	}

	OFF2 = OFF-OFFi;           //Calculate pressure and temp second order
	SENS2 = SENS-SENSi;

	if ( _model == MS5837_02BA ) {
		temperature = (temperature-Ti);
		pressure = (((data1*SENS2)/2097152l-OFF2)/32768l)/100;
	} else {
		temperature = (temperature-Ti);
		pressure = (((data1*SENS2)/2097152l-OFF2)/8192l)/10;
	}
}

float pressureMS5837(float conversion) {
	return pressure*conversion;
}

float temperatureMS5837() {
	return temperature/100.0f;
}

float depthMS5837() {
	return (pressureMS5837(Pa)-101300)/(fluidDensity*9.80665);
}

float altitudeMS5837() {
	return (1-pow((pressureMS5837(1.0f)/1013.25),.190284))*145366.45*.3048;
}
