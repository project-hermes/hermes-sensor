#include <Particle.h>
#include "math.h"

#define TSYS01_ADDR                        0x77
#define TSYS01_RESET                       0x1E
#define TSYS01_ADC_READ                    0x00
#define TSYS01_ADC_TEMP_CONV               0x48
#define TSYS01_PROM_READ                   0XA0

class TSYS01 {
public:

	TSYS01();

	void init();

	/** The read from I2C takes up for 40 ms, so use sparingly is possible.
	 */
	void read();

	/** Temperature returned in deg C.
	 */
	float temperature();

private:
	uint16_t C[8];
	uint32_t P1;
	float TEMP;
	uint32_t adc;

	/** Performs calculations per the sensor data sheet for conversion and
	 *  second order compensation.
	 */
	void calculate();

};



TSYS01::TSYS01() {

}

void TSYS01::init() {
	// Reset the TSYS01, per datasheet
	Wire.beginTransmission(TSYS01_ADDR);
	Wire.write(TSYS01_RESET);
	Wire.endTransmission();

	delay(10);

		// Read calibration values
	for ( uint8_t i = 0 ; i < 8 ; i++ ) {
		Wire.beginTransmission(TSYS01_ADDR);
		Wire.write(TSYS01_PROM_READ+i*2);
		Wire.endTransmission();

		Wire.requestFrom(TSYS01_ADDR,2);
		C[i] = (Wire.read() << 8) | Wire.read();
	}

}

void TSYS01::read() {

	Wire.beginTransmission(TSYS01_ADDR);
	Wire.write(TSYS01_ADC_TEMP_CONV);
	Wire.endTransmission();

	delay(10); // Max conversion time per datasheet

	Wire.beginTransmission(TSYS01_ADDR);
	Wire.write(TSYS01_ADC_READ);
	Wire.endTransmission();

	Wire.requestFrom(TSYS01_ADDR,3);
	P1 = 0;
	P1 = Wire.read();
	P1 = (P1 << 8) | Wire.read();
	P1 = (P1 << 8) | Wire.read();

	//calculate();
}


void TSYS01::calculate() {
	adc = P1/256;

TEMP = (-2) * float(C[1]) / 1000000000000000000000.0f * pow(adc,4) +
        4 * float(C[2]) / 10000000000000000.0f * pow(adc,3) +
	  (-2) * float(C[3]) / 100000000000.0f * pow(adc,2) +
   	    1 * float(C[4]) / 1000000.0f * adc +
      (-1.5) * float(C[5]) / 100 ;

}

float TSYS01::temperature() {
	return TEMP;
}

TSYS01 sensor;

void setup() {
  Serial.begin();
  Serial.println("Starting");

  Wire.begin();

  sensor.init();
}

void loop() {
    Serial.println("Reading sensor...");
    sensor.read();
    Serial.print("Temperature: ");
    Serial.print(sensor.temperature());
    Serial.println(" deg C");
    delay(1000);
}
