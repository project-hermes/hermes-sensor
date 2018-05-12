#include "tsys01.h"
#include "application.h"

void setup() {
  Wire.begin();
  Serial.begin();
  calibrate();

  Serial.println("Start temperature read...");
}

void loop() {
  Serial.printf("temp: %f\n", readTsysTemperature());
  delay(2000);
}
