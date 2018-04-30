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
const char dataFormat = 3;

typedef struct GPS_LOC{
    float latitude, longitude; // -90 to 90, 180 to -180
} GPS_LOC;

typedef struct DIVE_DATA {
    char depth[2];
    char temp1[2];
    char temp2[2];
} DIVE_DATA;

typedef struct DIVE_INFO {
    int diveId;
    float latStart, latEnd, longStart, longEnd;
    int dataCount;
    unsigned long timeStart, timeEnd;
    DIVE_DATA *diveData;
} DIVE_INFO;

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
    Particle.function("mockSamples",addSamples);
    Particle.function("mockStart",mockStart);
    Particle.function("mockEnd",mockEnd);

    Particle.function("freeMem",freeMem);
    Particle.function("readStatus",readStatus);
    Particle.function("readAll",readAll);
    Particle.function("diveDump",diveDump);

    randomSeed(analogRead(0));
}

void loop() {
    unsigned long loopStart = millis();
    if (loopStart - lastLoopMillis >= loopDelay) {
        lastLoopMillis = loopStart;

        timeInt = Time.now();

        /*
        // light indicates writingness
        if (writeLogs) {
            digitalWrite(led,HIGH);
        } else {
            digitalWrite(led,LOW);
        }
        */

        //Particle.publish("test-hermes2", timeStr);
        if (writeLogs && (loopStart - lastLogMillis >= logDelay)) {
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
    free(diveInfo.diveData);
    diveInfo.diveData = (DIVE_DATA *)malloc(sizeof(DIVE_DATA)*3600);
}

void diveEnd() {
    GPS_LOC gps = readGPS();
    diveInfo.latEnd = gps.latitude;
    diveInfo.longEnd = gps.longitude;
    diveInfo.timeEnd = Time.now();
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

int readStatus(String command) {
    char buffer[255];
    sprintf(buffer, "FM: %d Bat: %f = %f", System.freeMemory(), fuel.getSoC(), fuel.getVCell());
    Particle.publish("test-hermes2", buffer);
}

bool doSample() {
    if ( (diveInfo.diveData == NULL) || (diveInfo.dataCount >= 3600) ) {
        return false;
    }
    int depth = readDepth();
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
}

int diveCreate(String command) {
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
    if (buffer[0] == dataFormat) {
        Particle.publish("test-hermes2", "action: Create, format: good");
        return 1;
    } else {
        Particle.publish("test-hermes2", "action: Create, format: bad");
        return 0;
    }
}

int printDiveData(char *buffer, int start, int len) {
    int i;
    for (i = start; i < start+len; i++) {
        sprintf(buffer + i * sizeof(DIVE_DATA), "%c%c%c%c%c%c",
            diveInfo.diveData[i].depth, diveInfo.diveData[i].temp1, diveInfo.diveData[i].temp2);
    }
    return (i * sizeof(DIVE_DATA));
}

int diveAppend(String command) {
    char buffer[255];
    int sendCount = 0;
    int lastDataLen = 0;
    if (command.length() > 0) {
        // return the request if not null
        command.toCharArray(buffer, 255);
    } else {
        int toSend = diveInfo.dataCount;
        int firstTime = diveInfo.timeStart;
        int dataIdx = 0;
        int thisCount;
        while (toSend > 0) {
            thisCount = min(toSend, 41);
            sprintf(buffer, "%c%c%c%c%c%c%c", dataFormat, diveInfo.diveId,
                firstTime%256, (firstTime>>8)%256, (firstTime>>16)%256, (firstTime>>24)%256,
                thisCount);
            lastDataLen = printDiveData(buffer+7, dataIdx, dataIdx+thisCount);
            toSend -= 41;
            firstTime += 41;
            Particle.publish("diveAppend", buffer);
            sendCount++;
        }
    }
    
    bool formatGood = (buffer[0] == dataFormat);
    sprintf(buffer, "action: Append, format: %s, packets: %d, lastData: %d", (formatGood?"good":"bad"), sendCount, lastDataLen);
    Particle.publish("test-hermes2", buffer);
    return (formatGood?1:0);
}

int diveDone(String command) {
    char buffer[255];
    sprintf(buffer, command.substring(0,255));
    Particle.publish("diveDone", buffer);
    if (buffer[0] == dataFormat) {
        Particle.publish("test-hermes2", "action: Done, format: good");
        return 1;
    } else {
        Particle.publish("test-hermes2", "action: Done, format: bad");
        return 0;
    }
}

int diveDump(String command) {
    Particle.publish("test-hermes2", "Not Implemented");
}