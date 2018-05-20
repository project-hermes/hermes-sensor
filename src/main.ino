//  --------------------------------------------------------------------------
//  Project Hermes test code
//  Copyright (c) 2018 Project Hermes
//  --------------------------------------------------------------------------

//----------------------------------------
// COLLAPSE tsys01.h
//----------------------------------------
//how this works
//http://www.te.com/commerce/DocumentDelivery/DDEController?Action=showdoc&DocId=Data+Sheet%7FTSYS01%7FA%7Fpdf%7FEnglish%7FENG_DS_TSYS01_A.pdf%7FG-NICO-018

#ifndef TSYS01
#define TSYS01
#include "application.h"
// Address for TSYS01 sensor
#define TSYS01_ADDR                        0x77
#define TSYS01_RESET                       0x1E
#define TSYS01_ADC_READ                    0x00
#define TSYS01_ADC_TEMP_CONV               0x48
#define TSYS01_PROM_READ                   0XA0

float readTsysTemperature();
void calibrate();

// utility functions
float calculate(uint32_t data);

#endif

//----------------------------------------
// COLLAPSE ms5837.h
//----------------------------------------
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
double temperatureMS5837();
double depthMS5837();
double altitudeMS5837();

// utility functions
void calculateDepth(uint32_t data1, uint32_t data2);
uint8_t crc4(uint16_t n_prom[]);

#endif
SerialLogHandler logHandler;
//----------------------------------------
// COLLAPSE main.ino
//----------------------------------------
//#include "tsys01.h"
//#include "ms5837.h"
#include "application.h"
#include <AssetTracker.h>

// loop control
const int loopDelay = 1000;	// 1 second
const int logDelay = 10000; // 10 seconds
const int appendDelay = 1100; // 1.1 second
unsigned long lastLoopMillis = 0;
unsigned long lastLogMillis = 0;

bool writeLogs = false;
int logCount = 0;
const int maxLogs = 15;

// Hermes format version byte; 0 is reserved for future use (?)
const char dataFormat = 4;
const int dataPerPublish = 32;

typedef struct GPS_LOC{
    float latitude, longitude; // -90 to 90, 180 to -180
} GPS_LOC;

typedef struct DIVE_DATA {
    char depth[2];
    char temp1[2];
    char temp2[2];
} DIVE_DATA;

typedef struct DIVE_INFO {
    char diveId;
    float latStart, latEnd, longStart, longEnd;
    int dataCount;
    unsigned long timeStart, timeEnd;
    DIVE_DATA *diveData;
} DIVE_INFO;

// Data storage
DIVE_INFO diveInfo;

// Mock sensors
bool useMocks;
GPS_LOC sensorGPS;
int sensorDepth; // centimeters
float sensorTemp1; // precision .01, accuracy .1
float sensorTemp2; // precision .01, accuracy 1

// Actual sensors
FuelGauge fuel;
AssetTracker realGPS = AssetTracker();

int writeToggle(String command) {
    if (command=="on") {
        writeLogs = true;
        return 1;
    } else if (command=="off") {
        writeLogs = false;
        return 0;
    } else {
        return -1;
    }
}

int scanI2C(String command) {
    byte error, address;
    int nDevices = 0;

    char buffer[255];
    char errBuffer[255];
    sprintf(buffer, "sI2C:");
    sprintf(errBuffer, "sI2C errs:");

    Serial.println("Scanning...");
    for(address = 1; address < 127; address++ ){
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0){
            sprintf(buffer+strlen(buffer), " %d", address);
            Serial.print("I2C device found at address 0x");
            if (address<16){
                Serial.print("0");
            }
            Serial.print(address,HEX);
            Serial.println("  !");

            nDevices++;
        } else if (error==4){
            sprintf(errBuffer+strlen(errBuffer), " %d", address);
            Serial.print("Unknown error at address 0x");
            if (address<16){
                Serial.print("0");
            }
            Serial.println(address,HEX);
        }
    }
    if (nDevices == 0){
        Serial.println("No I2C devices found\n");
    } else {
        Serial.println("done\n");
    }

    sprintf(buffer+strlen(buffer), " =%d", nDevices);
    Particle.publish("test-hermes2", buffer);
    Particle.publish("test-hermes2", errBuffer);
    return nDevices;
}

int readAllRaw(String command) {
    realGPS.updateGPS();
    char buffer[255];
    sprintf(buffer, "%d %s real: %d %f %f (%f %f) ; mock: %d %f %f (%f %f)",
        diveInfo.diveId, (useMocks?"T":"F"),
        (int)depthMS5837(), readTsysTemperature(), temperatureMS5837(),
        realGPS.readLatDeg(), realGPS.readLonDeg(),
        sensorDepth, sensorTemp1, sensorTemp2, sensorGPS.latitude, sensorGPS.longitude);
    Particle.publish("test-hermes2", buffer);
    sprintf(buffer, "raw GPS: %s", realGPS.preNMEA());
    Particle.publish("test-hermes2", buffer);
    return diveInfo.dataCount;
}

int mockToggle(String command) {
    int readInt = command.toInt();
    useMocks = (readInt == 1);
    return readInt;
}

int mockGPS(String command) {
    int split = command.indexOf(" ");
    sensorGPS.latitude = command.substring(0,split).toFloat();
    sensorGPS.longitude = command.substring(split+1).toFloat();
    return (int)sensorGPS.latitude;
}

GPS_LOC readGPS() {
    if (useMocks) {
        return sensorGPS;
    } else {
        realGPS.updateGPS();
        return GPS_LOC{realGPS.readLatDeg(), realGPS.readLonDeg()};
    }
}

int mockDepth(String command) {
    sensorDepth = command.toInt();
    return sensorDepth;
}

int readDepthSensor() {
    // TODO: remove always-mock shim for manual dive control
    return sensorDepth;
    if (useMocks) {
        // drift 5 cm
        return sensorDepth + random(-5,6);
    } else {
        return (int)depthMS5837();
    }
}

int mockTemp(String command) {
    sensorTemp1 = command.toFloat();
    // sensor 2 is within 5 degrees
    sensorTemp2 = command.toFloat() + ((float)random(-500,501) / 100.0);
    return (int)sensorTemp1;
}

float readTemp1() {
    if (useMocks) {
        // drift .1 degree
        return sensorTemp1 + ((float)random(-10,11) / 100.0);
    } else {
        // TODO: get the error out through the sensor read
        Wire.beginTransmission(TSYS01_ADDR);
        if (Wire.endTransmission() == 0) {
            return readTsysTemperature();
        } else {
            // failed to find sensor
            return 0;
        }
    }
}

float readTemp2() {
    if (useMocks) {
        // drift 1 degree
        return sensorTemp2 + ((float)random(-100,101) / 100.0);
    } else {
        // initially use only one sensor
        // TODO: get the error out through the sensor read
        Wire.beginTransmission(MS5837_ADDR);
        if (Wire.endTransmission() == 0) {
            return temperatureMS5837();
        } else {
            // failed to find sensor
            return 0;
        }
    }
}

int diveActive() {
    return ((diveInfo.timeStart > 0) && (diveInfo.timeEnd == 0));
}

int diveStart() {
    GPS_LOC gps = readGPS();
    diveInfo.diveId++;
    if (diveInfo.diveId == 0) diveInfo.diveId++;
    diveInfo.latStart = gps.latitude;
    diveInfo.longStart = gps.longitude;
    diveInfo.dataCount = 0;
    diveInfo.timeStart = Time.now();
    // Zero out end info
    diveInfo.latEnd = 0;
    diveInfo.longEnd = 0;
    diveInfo.timeEnd = 0;
    // This should be allocated live in chunks (with freemem check reserving some amount TBD)
    // For the moment, use static 1-hour dives
    free(diveInfo.diveData);
    diveInfo.diveData = (DIVE_DATA *)malloc(sizeof(DIVE_DATA)*3600);
    return diveInfo.diveId;
}

int diveEnd() {
    GPS_LOC gps = readGPS();
    diveInfo.latEnd = gps.latitude;
    diveInfo.longEnd = gps.longitude;
    diveInfo.timeEnd = Time.now();
    return diveInfo.diveId;
}

void diveClear() {
    // Clear dive data and null out pointer for safety (may get called again)
    free(diveInfo.diveData);
    diveInfo.diveData = NULL;
    // Zero out info fields
    diveInfo.latStart = 0;
    diveInfo.longStart = 0;
    diveInfo.latEnd = 0;
    diveInfo.longEnd = 0;
    diveInfo.dataCount = 0;
    diveInfo.timeStart = 0;
    diveInfo.timeEnd = 0;
}

int mockStart(String command) {
    return diveStart();
}

int mockEnd(String command) {
    return diveEnd();
}

int readAll(String command) {
    char buffer[255];
    GPS_LOC localgps = readGPS();
    sprintf(buffer, "%d %f %f (%f, %f)", readDepthSensor(), readTemp1(), readTemp2(), localgps.latitude, localgps.longitude);
    Particle.publish("test-hermes2", buffer);
    return diveInfo.dataCount;
}

int readStatus(String command) {
    char buffer[255];
    sprintf(buffer, "FM: %d Bat: %f = %f", System.freeMemory(), fuel.getSoC(), fuel.getVCell());
    Particle.publish("test-hermes2", buffer);
    return diveInfo.dataCount;
}

bool doSample() {
    if ( (diveInfo.diveData == NULL) || (diveInfo.dataCount >= 3600) ) {
        return false;
    }

    // TODO: clean up temporary shift for -500 error
    //int depth = readDepthSensor() + 1000;
    int depth = (int)depthMS5837() + 1000;
    int temp1 = (int)((readTemp1() + 50) * 200);
    int temp2 = (int)((readTemp1() + 50) * 200);
    /*
    sprintf(diveInfo.diveData,"%s%c%c%c%c%c%c", diveInfo.diveData,
        depth%256, (depth>>8)%256,
        temp1%256, (temp1>>8)%256,
        temp2%256, (temp2>>8)%256);
    */
    sprintf(diveInfo.diveData[diveInfo.dataCount].depth,"%c%c", depth%256, (depth>>8)%256);
    sprintf(diveInfo.diveData[diveInfo.dataCount].temp1,"%c%c", temp1%256, (temp1>>8)%256);
    sprintf(diveInfo.diveData[diveInfo.dataCount].temp2,"%c%c", temp2%256, (temp2>>8)%256);
    diveInfo.dataCount++;
    return true;
}

int freeMem(String command) {
    return System.freeMemory();
}

int addSamples(String command) {
    int sampleCount = command.toInt();
    if (sampleCount <= 0) {
        sampleCount = 10;
    }
    for (int i = 0; i < sampleCount; i++) {
        doSample();
    }
    return sampleCount;
}

int diveCreate(String command) {
    // TODO: would be nice to ID in a way that doesn't require clearing sensorDiveId (time bomb if dropped)
    char buffer[255];
    if (command.length() > 0) {
        // return the request if not null
        command.toCharArray(buffer, 255);
    } else {
        sprintf(buffer, "%d %d %f %f %f %f %d %d %d", dataFormat, diveInfo.diveId,
        diveInfo.latStart, diveInfo.longStart, diveInfo.latEnd, diveInfo.longEnd,
        diveInfo.dataCount, diveInfo.timeStart, diveInfo.timeEnd);
    }

    Particle.publish("diveCreate", buffer);
    return diveInfo.diveId;
}

int printDiveData(char *buffer, int start, int len) {
    int i;
    for (i = 0; i < len; i++) {
        sprintf(buffer + i * sizeof(DIVE_DATA), "%c%c%c%c%c%c",
            diveInfo.diveData[start+i].depth[0], diveInfo.diveData[start+i].depth[1],
            diveInfo.diveData[start+i].temp1[0], diveInfo.diveData[start+i].temp1[1],
            diveInfo.diveData[start+i].temp2[0], diveInfo.diveData[start+i].temp2[1]);
    }
    return (i * sizeof(DIVE_DATA));
}

int diveAppend(String command) {
    char buffer[255];
    int sendCount = 0;
    int lastDataLen = 0;

        int toSend = diveInfo.dataCount;
        int firstTime = diveInfo.timeStart;
        int dataIdx = 0;
        int thisCount;

        if (command.length() > 0) {
            int startSample = command.toInt();
            toSend -= startSample;
            firstTime += startSample;
            dataIdx += startSample;
        }

        while (toSend > 0) {
            thisCount = min(toSend, dataPerPublish);
            sprintf(buffer, "%c%c%c%c%c%c%c", dataFormat, diveInfo.diveId,
                firstTime%256, (firstTime>>8)%256, (firstTime>>16)%256, (firstTime>>24)%256,
                thisCount);
            lastDataLen = printDiveData(buffer+7, dataIdx, thisCount);

            int pad;
            for (pad = 0; (lastDataLen+7+pad)%4 > 0; pad++) {
                sprintf(buffer+7+lastDataLen+pad,"%c",0);
            }
            char *z85out = Z85_encode((byte *)buffer, 7+lastDataLen+pad);
            Particle.publish("diveAppend", z85out);
            free(z85out);
            delay(appendDelay);

            toSend -= dataPerPublish;
            firstTime += dataPerPublish;
            dataIdx += dataPerPublish;
            sendCount++;
        }

    bool formatGood = (buffer[0] == dataFormat);
    sprintf(buffer, "action: Append, format: %s, packets: %d, lastData: %d", (formatGood?"good":"bad"), sendCount, lastDataLen);
    Particle.publish("test-hermes2", buffer);
    return (formatGood?sendCount:0);
}

int diveDone(String command) {
     // TODO: would be nice to ID in a way that doesn't require clearing sensorDiveId (time bomb if dropped)
    char buffer[255];
    if (command.length() > 0) {
        // return the request if not null
        command.toCharArray(buffer, 255);
    } else {
        sprintf(buffer, "%d %d %f %f %f %f %d %d %d", dataFormat, diveInfo.diveId,
        diveInfo.latStart, diveInfo.longStart, diveInfo.latEnd, diveInfo.longEnd,
        diveInfo.dataCount, diveInfo.timeStart, diveInfo.timeEnd);
    }

    Particle.publish("diveDone", buffer);
    return diveInfo.diveId;
}

int diveDump(String command) {
    Particle.publish("test-hermes2", "Not Implemented");
}

//  --------------------------------------------------------------------------
//  Reference implementation for rfc.zeromq.org/spec:32/Z85
//
//  This implementation provides a Z85 codec as an easy-to-reuse C class
//  designed to be easy to port into other languages.

//  --------------------------------------------------------------------------
//  Copyright (c) 2010-2013 iMatix Corporation and Contributors
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//  --------------------------------------------------------------------------

//#include <stdlib.h>
//#include <stdint.h>
//#include <stdio.h>
//#include <string.h>
//#include <assert.h>

//  Basic language taken from CZMQ's prelude
typedef unsigned char byte;
#define streq(s1,s2) (!strcmp ((s1), (s2)))

//  Maps base 256 to base 85
static char encoder [85 + 1] = {
    "0123456789"
    "abcdefghij"
    "klmnopqrst"
    "uvwxyzABCD"
    "EFGHIJKLMN"
    "OPQRSTUVWX"
    "YZ.-:+=^!/"
    "*?&<>()[]{"
    "}@%$#"
};

/*
//  Maps base 85 to base 256
//  We chop off lower 32 and higher 128 ranges
static byte decoder [96] = {
    0x00, 0x44, 0x00, 0x54, 0x53, 0x52, 0x48, 0x00,
    0x4B, 0x4C, 0x46, 0x41, 0x00, 0x3F, 0x3E, 0x45,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x40, 0x00, 0x49, 0x42, 0x4A, 0x47,
    0x51, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,
    0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
    0x3B, 0x3C, 0x3D, 0x4D, 0x00, 0x4E, 0x43, 0x00,
    0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x4F, 0x00, 0x50, 0x00, 0x00
};
*/

//  --------------------------------------------------------------------------
//  Encode a byte array as a string

char *
Z85_encode (byte *data, size_t size)
{
    //  Accepts only byte arrays bounded to 4 bytes
    if (size % 4)
        return NULL;

    size_t encoded_size = size * 5 / 4;
    char *encoded = (char *)malloc (encoded_size + 1);
    uint char_nbr = 0;
    uint byte_nbr = 0;
    uint32_t value = 0;
    while (byte_nbr < size) {
        //  Accumulate value in base 256 (binary)
        value = value * 256 + data [byte_nbr++];
        if (byte_nbr % 4 == 0) {
            //  Output value in base 85
            uint divisor = 85 * 85 * 85 * 85;
            while (divisor) {
                encoded [char_nbr++] = encoder [value / divisor % 85];
                divisor /= 85;
            }
            value = 0;
        }
    }
    //assert (char_nbr == encoded_size);
    encoded [char_nbr] = 0;
    return encoded;
}

/*
//  --------------------------------------------------------------------------
//  Decode an encoded string into a byte array; size of array will be
//  strlen (string) * 4 / 5.

byte *
Z85_decode (char *string)
{
    //  Accepts only strings bounded to 5 bytes
    if (strlen (string) % 5)
        return NULL;

    size_t decoded_size = strlen (string) * 4 / 5;
    byte *decoded = (byte *)malloc (decoded_size);

    uint byte_nbr = 0;
    uint char_nbr = 0;
    uint32_t value = 0;
    while (char_nbr < strlen (string)) {
        //  Accumulate value in base 85
        value = value * 85 + decoder [(byte) string [char_nbr++] - 32];
        if (char_nbr % 5 == 0) {
            //  Output value in base 256
            uint divisor = 256 * 256 * 256;
            while (divisor) {
                decoded [byte_nbr++] = value / divisor % 256;
                divisor /= 256;
            }
            value = 0;
        }
    }
    assert (byte_nbr == decoded_size);
    return decoded;
}
*/

//----------------------------------------
// COLLAPSE tsys01.ino
//----------------------------------------
//#include "tsys01.h"
#include "math.h"

uint16_t tsCalibrationValue[8];

void calibrate(){
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
		tsCalibrationValue[i] = (Wire.read() << 8) | Wire.read();
	}
}

float readTsysTemperature() {
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

	return calculate(data);
}


float calculate(uint32_t data) {
	uint32_t adc = data/256;

  float temp = (-2) * float(tsCalibrationValue[1]) / 1000000000000000000000.0f * pow(adc,4) +
    4 * float(tsCalibrationValue[2]) / 10000000000000000.0f * pow(adc,3) +
    (-2) * float(tsCalibrationValue[3]) / 100000000000.0f * pow(adc,2) +
      1 * float(tsCalibrationValue[4]) / 1000000.0f * adc +
    (-1.5) * float(tsCalibrationValue[5]) / 100 ;

  return temp;
}


/////////
// MS5837
/////////

#include "math.h"

float fluidDensity = 1029;
uint16_t msCalibrationValue[8];
int32_t temperature;
int32_t pressure;

const float Pa = 100.0f;
const float bar = 0.001f;
const float mbar = 1.0f;

const uint8_t MS5837_30BA = 0;
const uint8_t MS5837_02BA = 1;

uint8_t _model = MS5837_30BA;


// must be called to set up sensor, should be called at setup
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

// really need to not use global variables but meh

uint32_t rawMSTemp;
uint32_t rawMSPres;
long msTemp;
long deltaTemp;

void getRawValues() {

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
	rawMSPres = 0;
	rawMSPres = Wire.read();
	rawMSPres = (rawMSPres << 8) | Wire.read();
	rawMSPres = (rawMSPres << 8) | Wire.read();

	// Request Raw Temperature conversion
	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_CONVERT_D2_8192);
	Wire.endTransmission();

	delay(20); // Max conversion time per datasheet

	Wire.beginTransmission(MS5837_ADDR);
	Wire.write(MS5837_ADC_READ);
	Wire.endTransmission();

	Wire.requestFrom(MS5837_ADDR,3);
	rawMSTemp = 0;
	rawMSTemp = Wire.read();
	rawMSTemp = (rawMSTemp << 8) | Wire.read();
	rawMSTemp = (rawMSTemp << 8) | Wire.read();
}

// gets temp in mili c
long getMS5837Temp(){
  getRawValues();
  deltaTemp = rawMSTemp - msCalibrationValue[5] * pow(2,8);
  msTemp = 2000 + deltaTemp * msCalibrationValue[6] / pow(2,23);
  return msTemp;
}

// gets presure in mbar
long getMS5837Pressure(){
  getMS5837Temp();
  int64_t offset = msCalibrationValue[2] * pow(2,16) + (msCalibrationValue[4]*deltaTemp)/pow(2,7);
  int64_t sensitivity = msCalibrationValue[1] * pow(2,15) + (msCalibrationValue[3]*deltaTemp)/pow(2,8);
  long msPresure = ((rawMSPres * sensitivity / pow(2,21) - offset)/pow(2,13))/10;
  return msPresure;
}


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

// dont use this, idk what it does
double altitudeMS5837() {
	return (1-pow((pressureMS5837(100.0)/1013.25),0.190284))*145366.45*.3048;
}


////////
// MAIN
////////

void setup() {
    // Safety
    diveInfo.diveData = NULL;

    // Functions for console control
    //Particle.function("write",writeToggle);
    Particle.function("diveCreate",diveCreate);
    Particle.function("diveAppend",diveAppend);
    Particle.function("diveDone",diveDone);

    Particle.function("readAllRaw",readAllRaw);
    Particle.function("scanI2C",scanI2C);

    Particle.function("mockToggle",mockToggle);
    Particle.function("mockGPS",mockGPS);
    Particle.function("mockDepth",mockDepth);
    Particle.function("mockTemp",mockTemp);
    Particle.function("mockSamples",addSamples);
    Particle.function("mockStart",mockStart);
    Particle.function("mockEnd",mockEnd);

    Particle.function("freeMem",freeMem);
    Particle.function("readStatus",readStatus);
    Particle.function("readAll",readAll);
    Particle.function("diveDump",diveDump);

    randomSeed(analogRead(0));

    // Set up mocks
    useMocks = false;
    sensorDepth = 0;
    sensorTemp1 = 0;
    sensorTemp2 = 0;

	// Sensor setup code
    Wire.begin();
    Serial.begin();
    realGPS.begin();
    realGPS.gpsOn();  // gps on but at rest is fairly low power draw

    Serial.println("Starting pressure sensor...");
    initPressureSensor();
    calibrate();
}

void loop() {
    realGPS.updateGPS();    // without high-frequency pokes, GPS chip never locks.
    unsigned long loopStart = millis();
    if (loopStart - lastLoopMillis >= loopDelay) {
        lastLoopMillis = loopStart;

	// still control dive via mocked depth
        if (sensorDepth >= 10) {
	    if (diveActive()) {
                doSample();
                if ((diveInfo.dataCount % 5) == 0) {
                    RGB.color(0, 255, 0);
                } else {
                    RGB.color(255, 0, 255);
                }
            } else {
                diveStart();
                RGB.control(true);
                RGB.color(255, 0, 255);
            }
        } else {
            if (diveActive()) {
                diveEnd();
                RGB.color(255, 0, 0);
                // Simple upload, no handshake or verification
                diveCreate("");
                delay(60000);  // no handshake; assume 1 minute is enough time
                RGB.color(0, 0, 255);
                diveAppend("");
                delay(300000);  // no handshake; assume 5 minutes is enough time
                diveDone("");
                RGB.control(false);
            }
        }

        if (writeLogs && (loopStart - lastLogMillis >= logDelay)) {
            // Sensor readers
            Serial.println("Reading sensor...");

            Serial.printf("Temp: %f\n",temperatureMS5837());
            Serial.printf("Depth: %f\n",depthMS5837());
            Serial.printf("Altitude: %f\n",altitudeMS5837());

            lastLogMillis = loopStart;

            char buffer[255];
            sprintf(buffer, "%d @%d FM: %d Bat: %f = %f", logCount, lastLogMillis, System.freeMemory(), fuel.getSoC(), fuel.getVCell());

            Particle.publish("test-hermes2", buffer);
            logCount++;

            if (logCount >= maxLogs) {
                writeLogs = false;
            }
        }
    }
}
