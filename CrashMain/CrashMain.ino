/**************************************************************************/
/* file: CrashMain.ino                                                    */
/*                                                                        */
/* Senior Design Project, UC Irvine, Winter 2015                          */
/*                                                                        */
/* authors: Ryan Morrison, CpE                                            */
/*          Omar Bravo, CSE                                               */
/*          Nitish Sachar, CSE                                            */
/*                                                                        */
/* This sketch enables wireless communication between vehicles for the    */
/* purpose of recording collision details. Once a collision occurs, each  */
/* vehicle will posess information about the other vehicles involved.     */
/*                                                                        */
/* For detailed information, visit the project page at:                   */
/*          http://www.ryanmorrisonportfolio.com                          */
/**************************************************************************/

#include <Wire.h>
#include <RTClib.h>
#include <XBee.h>
#include <SPI.h>
#include <SD.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>

/****************************** Begin XBee ********************************/

XBee xbee = XBee();
Rx16Response rx16 = Rx16Response();
Tx16Request tx;
uint8_t payloadReceived[100];
uint8_t payloadToSend[100];
char payloadToSendSize;

/****************************** End XBee **********************************/

/****************************** Begin RTC *********************************/

RTC_DS1307 rtc;

/****************************** End RTC ***********************************/

/****************************** Begin Accel *******************************/

// instantiate accelerometer 
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321); // IDs are arbitrary

byte updateflag = 0;

sensors_event_t event;

// accel vars
int xaxis = 0, yaxis = 0, zaxis = 0;
int deltx = 0, delty = 0, deltz = 0;
int magnitude = 0;

// collision vars
int vibration = 0, sensitivity = 0, devibrate = 0;

// timing vars
unsigned long time1; // time since sketch started

/****************************** End Accel *********************************/

/****************************** Begin Magnometer **************************/

//Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345); // IDs are arbitrary

//sensors_event_t magEvent;

//double angle;
//float impactDirection; // direction of travel prior to impact

/****************************** End Magnometer ****************************/

/****************************** Begin SD **********************************/

#define vinLength 7

const int chipSelect = 4;
const char vehicle_id[] = "id.txt";

/****************************** End SD ************************************/

/**************************** Begin Relay *********************************/

#define relayPin    37
#define relayPower  35
#define relayGround 39

/****************************** End Relay *********************************/

/****************************** Begin LEDs ********************************/

#define passSide    40
#define driveSide   44
#define frontSide   38
#define rearSide    42
#define redPin      45
#define greenPin    43
#define bluePin     41

/******************************** End LEDs ********************************/

/**************************** Begin Other *********************************/

#define contactedLimit 10

unsigned long waitTime; // time to wait (will be random later on)
unsigned long startTime;
unsigned long endTime;
unsigned long duration;
uint8_t contact[contactedLimit][vinLength];
int contacted;
unsigned long currCount = 0;
boolean matches = false;
int randPin = A0;
int timeoutTime = 5000;
int timenow;
unsigned long formattingTime = 100;

/***************************** End Other **********************************/


void setup() {

    Serial.begin(9600);
    Serial2.begin(9600);
    xbee.setSerial(Serial2);

    // setup relay pin; relay is default wired to NC so motors
    // are free to be controlled by switch.
    // this initialization guarantees relay is in this state
    // when sketch starts.
    digitalWrite(relayPin, LOW);
    pinMode(relayPin, OUTPUT);

    // supply 5V to relay
    digitalWrite(relayPower, HIGH);
    pinMode(relayPower, OUTPUT);

    // supply ground to relay
    pinMode(relayGround, OUTPUT);
    digitalWrite(relayGround, LOW);

    // setup LED pins
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    digitalWrite(passSide, LOW);
    pinMode(passSide, OUTPUT);
    digitalWrite(driveSide, LOW);
    pinMode(driveSide, OUTPUT);
    digitalWrite(frontSide, LOW);
    pinMode(frontSide, OUTPUT);
    digitalWrite(rearSide, LOW);
    pinMode(rearSide, OUTPUT);
    digitalWrite(redPin, LOW);
    pinMode(redPin, OUTPUT);
    digitalWrite(greenPin, LOW);
    pinMode(greenPin, OUTPUT);
    digitalWrite(bluePin, LOW);
    pinMode(bluePin, OUTPUT);

    Wire.begin();
    rtc.begin();

    Serial.println(F("\n"));

    if (!rtc.isrunning()) {
        Serial.println(F("RTC not working!!!"));
        return;
    }
    else {
        Serial.println(F("RTC is working..."));
    }
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);

    if (!SD.begin(chipSelect)) {
        Serial.println(F("SD not working!!!"));
        return;
    }
    else {    
        Serial.println(F("SD is working..."));
    }

    if (!accel.begin()) {
        Serial.println("Accelerometer not working!!!");
        while (1);
    }
    else {
        Serial.println(F("Accelerometer is working...\n"));
    }

    getVehicleID();
    Serial.println(F("This is me:"));
    Serial.write(payloadToSend, payloadToSendSize);

    contacted = 0;
    for (int o = 0; o < contactedLimit; o++) {
        for (int p = 0; p < vinLength; p++) {
            contact[o][p] = 0;
        }
    }

    randomSeed(randPin);

    // change these to adjust accel. sensitivity
    sensitivity = 25;
    devibrate = 75;

    time1 = millis();
}

void loop() {

    if (formattingTime == 1000.0) {
        Serial.println(F("Waiting for collision..."));
        formattingTime = 0;
    }

    setColor(0x00, 0xff, 0x00); // standby (green) status
    accel.getEvent(&event);
  
    if (micros()-time1>1999) {
        Impact();
    }

    // impact detected
    if (updateflag > 0) {
    
        setColor(0xff, 0x00, 0x00);  // red: collision detected

        digitalWrite(relayPin, HIGH);
        Serial.println(F("\nCollision detected!!!"));

        // side impact case
        if (sqrt(sq(deltx)) > sqrt(sq(delty))) {
            Serial.println("Side hit!!!");
            if (deltx < 0) {
                Serial.println("Driver side hit!!!");
                digitalWrite(driveSide, HIGH);
            }
            else {
                Serial.println("Passenger side hit!!!");
                digitalWrite(passSide, HIGH);
            }
        }

        // front/rear impact case
        if (sqrt(sq(deltx)) < sqrt(sq(delty))) {
            Serial.println("End hit!!!");
            if (delty < 0) {
                Serial.println("Rear end hit!!!");
                digitalWrite(rearSide, HIGH);
            }
            else {
                Serial.println("Front end hit!!!");
                digitalWrite(frontSide, HIGH);
            }
        }

        getTimeStamp();

        // trigger relay to turn off motors
        // this causes the relay to switch to NO, thus causing the motors to stop.
        digitalWrite(relayPin, HIGH);

        startTime = millis();
        timenow = millis();

        // enter random back-off cycle to allow each module to send and receive
        while ((millis() - timenow) < timeoutTime) {

            waitTime = random(2, 11) * 50;

            // send loop
            setColor(0x00, 0x00, 0xff);  // blue: sending
            currCount = millis();
            while ((millis() - currCount) < waitTime) {
                //Serial.println(F("Sending......"));
                xbee.send(tx);
            }

            // receive loop
            setColor(0xa9, 0xff, 0xff);  // magenta: receiving 
            currCount = millis();
            while ((millis() - currCount) < waitTime) {
                //Serial.println(F("Receiving......"));
                xbee.readPacket();
                if (xbee.getResponse().isAvailable() && xbee.getResponse().getApiId() == RX_16_RESPONSE) {
                    // got a rx packet
                    Serial.println(F("Packet received!"));
                    xbee.getResponse().getRx16Response(rx16);
                    uint8_t temp[7];
                    int receivedPacketSize = rx16.getDataLength();

                    for (int i = 0; i < receivedPacketSize; i++) {
                        payloadReceived[i] = rx16.getData(i);
                        if (i > 4 && i < 12) {
                            temp[i - 5] = payloadReceived[i - 5];
                        }
                    }
                    if (!hasMadeContact(temp)) {
                        writeReceivedData(receivedPacketSize);
                        for (int m = 0; m < 7; m++) {
                            contact[contacted][m] = temp[m];
                        }
                        ++contacted;
                    }
                    else {
                        Serial.println(F("Contact already established with this car..."));
                    }
                }
            }
        }
        for (int o = 0; o < contacted; o++) {
            for (int p = 0; p < 7; p++) {
                contact[o][p] = 0;
            }
        }
        contacted = 0;
        duration = endTime - startTime;
        setColor(0x00, 0xff, 0x00); // green status
        digitalWrite(passSide, LOW);
        digitalWrite(frontSide, LOW);
        digitalWrite(driveSide, LOW);
        digitalWrite(rearSide, LOW);
        magnitude = 0;
        updateflag = 0;
    }

    formattingTime++;
}


/*************** assemle packet for sending *******************/
int getVehicleID () {

    payloadToSendSize = 0;
    File dataFile = SD.open(vehicle_id);
    if (dataFile) {
        while (dataFile.available()) {
            payloadToSend[payloadToSendSize++] = dataFile.read();
        }
        dataFile.close();
    }
    else {
        Serial.println(F("id.txt fail"));
    }
    return payloadToSendSize;
}

/*** check to see if contact with specific car has already occured ***/
boolean hasMadeContact(uint8_t toCheck[7]) {

    matches = false;
    for (int j = 0; j < contacted; j++) {
        matches = true;
        for (int k = 0; k < vinLength; k++) {
            if (contact[j][k] != toCheck[k]) {
                matches = false;
            }
        }
        if (matches) {
            return true;
        }
    }
    return false;
}

/************ write received packet to log file *****************/
void writeReceivedData(int receivedPacketSize) {

    File dataFile = SD.open("col.txt", FILE_WRITE);
    if (dataFile) {
        dataFile.write(payloadReceived, receivedPacketSize);
        dataFile.close();
        // print to the serial port too:
        //Serial.println(F("Packet received:"));
        Serial.println(F("Writing packet to SD card:"));
        Serial.write(payloadReceived, receivedPacketSize);
        Serial.println(F(""));
    }
    else {
        Serial.println(F("col.txt failed to open"));
        Serial.write(payloadReceived, receivedPacketSize);
    }
}

/******** formats and appends time-stamp to packet *******/ 
void getTimeStamp () {

    int i = 0;
    DateTime collisionTime = rtc.now();

    String temp = String(collisionTime.year());
    for (i = 0; i < temp.length(); i++) {
        payloadToSend[payloadToSendSize++] = temp[i];
    }
    payloadToSend[payloadToSendSize++] = '/';

    temp = String(collisionTime.month());
    if (temp.length() == 1) {
        temp = "0" + temp;
    }
    for (i = 0; i < temp.length(); i++) {
        payloadToSend[payloadToSendSize++] = temp[i];
    }
    payloadToSend[payloadToSendSize++] = '/';

    temp = String(collisionTime.day());
    if (temp.length() == 1) {
        temp = "0" + temp;
    }
    for (i = 0; i < temp.length(); i++) {
        payloadToSend[payloadToSendSize++] = temp[i];
    }
    payloadToSend[payloadToSendSize++] = ' ';

    temp = String(collisionTime.hour());
    if (temp.length() == 1) {
        temp = "0" + temp;
    }
    for (i = 0; i < temp.length(); i++) {
        payloadToSend[payloadToSendSize++] = temp[i];
    }
    payloadToSend[payloadToSendSize++] = ':';

    temp = String(collisionTime.minute());
    if (temp.length() == 1) {
        temp = "0" + temp;
    }
    for (i = 0; i < temp.length(); i++) {
        payloadToSend[payloadToSendSize++] = temp[i];
    }
    payloadToSend[payloadToSendSize++] = ':';

    temp = String(collisionTime.second());
    if (temp.length() == 1) {
        temp = "0" + temp;
    }
    for (i = 0; i < temp.length(); i++) {
        payloadToSend[payloadToSendSize++] = temp[i];
    }
    payloadToSend[payloadToSendSize++] = '\n';

    Serial.println(F("\nSending packet:"));
    Serial.write(payloadToSend, payloadToSendSize);
    Serial.println(F(""));
    tx = Tx16Request(0xFFFF, payloadToSend, payloadToSendSize);
    payloadToSendSize -= 20;
}

/********* detects impact (poor implementation, but serves our purpose here) *********/
void Impact() {

    time1 = micros();
    int oldx = xaxis;
    int oldy = yaxis;
    int oldz = zaxis;

    vibration--;
    if (vibration < 0) vibration = 0;

    xaxis = event.acceleration.x;
    yaxis = event.acceleration.y;
    zaxis = event.acceleration.z;

    if (vibration > 0) return;

    deltx = xaxis - oldx;
    delty = yaxis - oldy;
    deltz = zaxis - oldz;

    magnitude = sqrt(sq(deltx) + sq(delty) + sq(deltz));

    if (magnitude > sensitivity) {
        updateflag = 1;
        vibration = devibrate;
    }
    else {
        if (magnitude > 15) {
            Serial.println(magnitude);
        }
        magnitude = 0;
    }
}

/*************** sets color of status LED ************************/
void setColor(int red, int green, int blue) {
    analogWrite(redPin, red);
    analogWrite(greenPin, green);
    analogWrite(bluePin, blue);
}
