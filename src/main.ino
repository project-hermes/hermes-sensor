#include "ms5837.h"
#include "application.h"

void setup(){
  Wire.begin();
  Serial.begin();

  Serial.println("Starting presure sensor...");
  initPressureSensor();
}

void loop(){
  Serial.println("Rading sensor...");
  readDepth();

  Serial.printf("Temp: %f\n",temperatureMS5837());
  Serial.printf("Depth: %f\n",depthMS5837());
  Serial.printf("Altitude: %f\n",altitudeMS5837());

  delay(2000);
}
