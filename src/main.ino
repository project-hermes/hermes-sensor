#include <AssetTracker.h>

int transmittingData = 1;
long lastPublish = 0;
int delayMinutes = 10;

AssetTracker t = AssetTracker();
FuelGauge fuel;

void setup() {
    t.begin();
    t.gpsOn();

    Serial.begin(9600);

    Particle.function("tmode", transmitMode);
    Particle.function("batt", batteryStatus);
    Particle.function("gps", gpsPublish);
}

void loop() {
    t.updateGPS();

    if (millis()-lastPublish > delayMinutes*60*1000) {
        lastPublish = millis();
        Serial.println(t.preNMEA());

        if (t.gpsFix()) {
            if (transmittingData) {
                Particle.publish("G", t.readLatLon(), 60, PRIVATE);
            }
            Serial.println(t.readLatLon());
        }
    }
}

int transmitMode(String command) {
    transmittingData = atoi(command);
    return 1;
}

int gpsPublish(String command) {
    if (t.gpsFix()) {
        Particle.publish("G", t.readLatLon(), 60, PRIVATE);
        return 1;
    } else {
      return 0;
    }
}

int batteryStatus(String command){
    Particle.publish("B",
          "v:" + String::format("%.2f",fuel.getVCell()) +
          ",c:" + String::format("%.2f",fuel.getSoC()),
          60, PRIVATE
    );

    if (fuel.getSoC()>10){
      return 1;
    } else {
      return 0;
    }
}
