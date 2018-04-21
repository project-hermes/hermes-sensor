// loop control
const int loopDelay = 1000;
const int logDelay = 10000;
unsigned long lastLoopMillis = 0;
unsigned long lastLogMillis = 0;

bool writeLogs = false;
int logCount = 0;
const int maxLogs = 15;

int timeInt = 127;
int packetCount = 0;
String timeStr = "";

// Hermes format version byte; 0 is reserved for future use (?)
const char dataFormat = 'B';

typedef struct GPS_LOC{
    float latitude, longitude; // -90 to 90, 180 to -180
} GPS_LOC;

typedef struct DIVE_DATA {
    int diveId;
    float latStart, latEnd, longStart, longEnd;
    int dataCount;
    unsigned long timeStart, timeEnd;
} DIVE_DATA;

GPS_LOC sensorGPS;
int sensorDepth; // centimeters
float sensorTemp1; // precision .01, accuracy .1
float sensorTemp2; // precision .01, accuracy 1

DIVE_DATA diveData;

FuelGauge fuel;

void setup() {
    // Set pin IN/OUT
    pinMode(power,OUTPUT); // The pin powering the photoresistor is output (sending out consistent power)
    pinMode(led,OUTPUT); // Our LED pin is output (lighting up the LED)
    pinMode(photoresistor,INPUT);  // Our photoresistor pin is input (reading the photoresistor)
    
    // Set power pin high
    digitalWrite(power,HIGH);
    digitalWrite(led,LOW);
    
    // Publish vars for GET
    Particle.variable("analogvalue", &analogValue, INT);
    Particle.variable("timeint",timeInt);
    Particle.variable("timestr",timeStr);

    // Functions for console control
    Particle.function("write",writeToggle);
    Particle.function("diveCreate",diveCreate);
    Particle.function("diveAppend",diveAppend);
    Particle.function("diveDone",diveDone);
    
    Particle.function("mockGPS",mockGPS);
    Particle.function("mockDepth",mockDepth);
    Particle.function("mockTemp",mockTemp);
    Particle.function("mockStart",mockStart);
    Particle.function("mockEnd",mockEnd);
    
    Particle.function("freeMem",freeMem);
    Particle.function("readAll",readAll);
    Particle.function("diveDump",diveDump);
    
    randomSeed(analogRead(0));
}

void loop() {
    unsigned long loopStart = millis();
    if (loopStart - lastLoopMillis >= loopDelay) {
        lastLoopMillis = loopStart;
        
        timeInt = Time.now();
        analogValue = analogRead(photoresistor);
        
        // light indicates writingness
        if (writeLogs) {
            digitalWrite(led,HIGH);
        } else {
            digitalWrite(led,LOW);
        }
        
        //Particle.publish("test-hermes2", timeStr);
        if (writeLogs && (loopStart - lastLogMillis >= logDelay)) {
            lastLogMillis = loopStart;
            
            char buffer[255];
            sprintf(buffer, "%d @%d Bat: %f = %f", logCount, lastLogMillis, fuel.getSoC(), fuel.getVCell());
            
            Particle.publish("test-hermes2", buffer);
            logCount++;
            
            if (logCount >= maxLogs) {
                writeLogs = false;
            }
        }
    }
}

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

int mockGPS(String command) {
    int split = command.indexOf(" ");
    sensorGPS.latitude = command.substring(0,split).toFloat();
    sensorGPS.longitude = command.substring(split+1).toFloat();
}

GPS_LOC readGPS() {
    return sensorGPS;
}

int mockDepth(String command) {
    sensorDepth = command.toInt();
}

int readDepth() {
    // drift 5 cm
    return sensorDepth + random(-5,6);
}

int mockTemp(String command) {
    sensorTemp1 = command.toFloat();
    // sensor 2 is within 5 degrees
    sensorTemp2 = command.toFloat() + ((float)random(-500,501) / 100.0);
}

float readTemp1() {
    // drift .1 degree
    return sensorTemp1 + ((float)random(-10,11) / 100.0);
}

float readTemp2() {
    // drift 1 degree
    return sensorTemp2 + ((float)random(-100,101) / 100.0);
}

void diveStart() {
    GPS_LOC gps = readGPS();
    diveData.diveId++;
    diveData.latStart = gps.latitude;
    diveData.longStart = gps.longitude;
    diveData.dataCount = 0;
    diveData.timeStart = Time.now();
}

void diveEnd() {
    GPS_LOC gps = readGPS();
    diveData.latEnd = gps.latitude;
    diveData.longEnd = gps.longitude;
    diveData.timeEnd = Time.now();
}

int mockStart(String command) {
    diveStart();
}

int mockEnd(String command) {
    diveEnd();
}

int readAll(String command) {
    char buffer[255];
    GPS_LOC gps = readGPS();
    sprintf(buffer, "%d %f %f (%f, %f)", readDepth(), readTemp1(), readTemp2(), gps.latitude, gps.longitude);
    Particle.publish("test-hermes2", buffer);
}

int freeMem(String command) {
    return System.freeMemory();
}

int diveCreate(String command) {
    char buffer[255];
    if (command.length() > 0) {
        // return the request if not null
        command.toCharArray(buffer, 255);
    } else {
        sprintf(buffer, "%c%d %f %f %f %f %d %d %d", dataFormat, diveData.diveId,
        diveData.latStart, diveData.longStart, diveData.latEnd, diveData.longEnd,
        diveData.dataCount, diveData.timeStart, diveData.timeEnd);
    }
    
    Particle.publish("diveCreate", buffer);
    if (buffer[0] == dataFormat) {
        Particle.publish("test-hermes2", "action: Create, format: good");
        return 1;
    } else {
        Particle.publish("test-hermes2", "action: Create, format: bad");
        return 0;
    }
}

int diveAppend(String command) {
    char buffer[255];
    sprintf(buffer, command.substring(0,255));
    Particle.publish("diveAppend", buffer);
    if (buffer[0] == dataFormat) {
        Particle.publish("test-hermes2", "action: Create, format: good");
        return 1;
    } else {
        Particle.publish("test-hermes2", "action: Create, format: bad");
        return 0;
    }
}

int diveDone(String command) {
    char buffer[255];
    sprintf(buffer, command.substring(0,255));
    Particle.publish("diveDone", buffer);
    if (buffer[0] == dataFormat) {
        Particle.publish("test-hermes2", "action: Create, format: good");
        return 1;
    } else {
        Particle.publish("test-hermes2", "action: Create, format: bad");
        return 0;
    }
}

int diveDump(String command) {
    Particle.publish("test-hermes2", "Not Implemented");
}