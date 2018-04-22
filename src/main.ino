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

typedef struct DIVE_INFO {
    int diveId;
    float latStart, latEnd, longStart, longEnd;
    int dataCount;
    unsigned long timeStart, timeEnd;
    DIVE_DATA *diveData
} DIVE_INFO;

typedef struct DIVE_DATA {
    byte[2] depth;
    byte[2] temp1;
    byte[2] temp2;
} DIVE_DATA;

GPS_LOC sensorGPS;
int sensorDepth; // centimeters
float sensorTemp1; // precision .01, accuracy .1
float sensorTemp2; // precision .01, accuracy 1

DIVE_INFO diveInfo;

FuelGauge fuel;

void setup() {
    // Safety
    diveInfo.diveData = NULL; 
    
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
            sprintf(buffer, "%d @%d FM: %d Bat: %f = %f", logCount, System.freeMemory() lastLogMillis, fuel.getSoC(), fuel.getVCell());
            
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
    diveInfo.diveId++;
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
    diveInfo.diveData = malloc(sizeof(DIVE_DATA))
}

void diveEnd() {
    GPS_LOC gps = readGPS();
    diveInfo.latEnd = gps.latitude;
    diveInfo.longEnd = gps.longitude;
    diveInfo.timeEnd = Time.now();
}

void diveClear() {
    // Clear dive data and null out pointer for safety (may get called again)
    free(diveInfo.diveData)
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
        sprintf(buffer, "%c%d %f %f %f %f %d %d %d", dataFormat, diveInfo.diveId,
        diveInfo.latStart, diveInfo.longStart, diveInfo.latEnd, diveInfo.longEnd,
        diveInfo.dataCount, diveInfo.timeStart, diveInfo.timeEnd);
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