/**************************************************************************/
/* file: wirelessTestB.ino                                                */
/*                                                                        */
/* Senior Design Project, UC Irvine, Winter 2015                          */
/*                                                                        */
/* author: Omar Bravo, CSE                                                */
/*                                                                        */
/* This sketch simulates consecutive collisions for a user-specified      */
/* time interval. With each successive collision, one car assumes a       */ 
/* random identity (VIN). This measures how accurately the CRaSH device   */
/* identifies each vehicle in a multi-car collision.                      */
/*                                                                        */
/* Upload wirelessTestA to one CRaSH device and this sketch to the other. */
/* Adjust timeoutTime variable to the same as in wirelessTestA            */
/*                                                                        */
/* For detailed information, visit the project page at:                   */
/*          (insert URL here when done)                                   */
/**************************************************************************/

#include <Wire.h>
#include <RTClib.h>
#include <XBee.h>
#include <SPI.h>
#include <SD.h>

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
uint8_t timeStamp[100];
unsigned long time1;
byte updateflag = 0;

/****************************** End RTC ***********************************/

/****************************** Begin SD **********************************/

#define vinLength 7

const int chipSelect = 4;
const char vehicle_id[] = "id.txt";


/****************************** End SD ************************************/

/**************************** Begin Relay *********************************/

// relay pin and LED pins
#define relayPin    37
#define relayPower  35
#define relayGround 39
#define redPin      45
#define greenPin    43
#define bluePin     41

/****************************** End Relay *********************************/

/**************************** Begin Other *********************************/

#define contactedLimit 30

unsigned long waitTime;
uint8_t contact[contactedLimit][vinLength];
int contacted;
unsigned long currCount = 0;
boolean matches = false;
int randPin = A0;

// adjust this to the same timeoutTime as in wirelessTestA
unsigned long timeoutTime = 20000;

unsigned long timenow;
unsigned long timesInitiatedContact;
unsigned long timesWrittenToSD;
int identities[30];
char firstDigit;
char secondDigit;
int index;
int index2;

/***************************** End Other **********************************/

/**************************** End Accelerometer ***************************/

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
    setColor(0x00, 0xff, 0x00); // green status

    Wire.begin();
    rtc.begin();

    if (!rtc.isrunning()) {
        Serial.println(F("RTC not working"));
        return;
    }
    else {
        Serial.println(F("RTC is working"));
    }

    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);

    if (!SD.begin(chipSelect)) {
        Serial.println(F("SD not working"));
        return;
    }
    else {
        Serial.println(F("SD is working"));
    }

    getVehicleID();
    //writeInfo();
    Serial.write(payloadToSend, payloadToSendSize);

    contacted = 0;
    for (int o = 0; o < contactedLimit; o++) {
        for (int p = 0; p < vinLength; p++) {
            contact[o][p] = 0;
        }
        identities[o] = 0;
    }

    timesInitiatedContact = 0;
    timesWrittenToSD = 0;
    randomSeed(randPin);
    time1 = millis();
}

void loop() {

  if (true) {

        updateflag = 0;
        ++timesInitiatedContact;
        // trigger relay to turn off motors
        // this causes the relay to switch to NO, thus causing the motors to stop.
        digitalWrite(relayPin, HIGH);

        getTimeStamp();
        BeginSession();

        timenow = millis();

        while ((millis() - timenow) < timeoutTime) {
            index = random(0, 100)%2;
            firstDigit = index + 48;
            index *= 10;
            index2 = random(0, 100)%10;
            secondDigit = index2 + 48;
            index += index2;
            ++identities[index];
            payloadToSend[5] = firstDigit;
            payloadToSend[6] = secondDigit;
            waitTime = random(2, 11) * 50;

            setColor(0x00, 0x00, 0xff);  // blue: sending
            currCount = millis();
            while ((millis() - currCount) < waitTime) {
                //Serial.println(F("sending"));
                xbee.send(tx);
            }

            setColor(0xa9, 0xff, 0xff);  // magenta: receiving (for 'clear' LED?)
            currCount = millis();
            while ((millis() - currCount) < waitTime) {
                //Serial.println(F("receiving"));
                xbee.readPacket();
                if (xbee.getResponse().isAvailable() && xbee.getResponse().getApiId() == RX_16_RESPONSE) {
                    // got a rx packet
                    Serial.println(F("got packet"));
                    // red status
                    setColor(0xff, 0x00, 0x00);  // red: collision detected

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
                        Serial.println(F("I have made contact with this car already"));
                    }
                }
            }
        }

        EndSession();
        if (contacted > 0) {
            for (int o = 0; o < contacted; o++) {
                for (int p = 0; p < 7; p++) {
                    contact[o][p] = 0;
                }
            }
            contacted = 0;
        }
        setColor(0x00, 0xff, 0x00); // green status
        for (int e = 0; e < 20; e++) {
            Serial.print("Times assumed identity ");
            if(e<10)
            Serial.print(0);
            Serial.print(e);Serial.print("75309: ");
            Serial.println(identities[e]);
        }
        while(1);
    }

    else {
        Serial.println(F("No collision."));
    }
    delay(500);
}

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

void writeReceivedData(int receivedPacketSize) {

    Serial.println(F("Before writting to SD card..."));
    File dataFile = SD.open("col.txt", FILE_WRITE);
    if (dataFile) {
        dataFile.print("\nCar # "); dataFile.println(contacted + 1);
        dataFile.write(payloadReceived, payloadToSendSize);
        dataFile.close();
        ++timesWrittenToSD;
        // print to the serial port too:
        Serial.write(payloadReceived, payloadToSendSize);
    }
    else {
        Serial.println(F("col.txt failed to open"));
        Serial.write(payloadReceived, payloadToSendSize);
    }
}

void writeInfo() {
    File dataFile = SD.open("col.txt", FILE_WRITE);
    if (dataFile) {
        dataFile.println("\n******************** My Information *****************");
        dataFile.println();
        dataFile.write(payloadToSend, payloadToSendSize);
        dataFile.println("\n*****************************************************");
        dataFile.close();

        // print to the serial port too:
        Serial.write(payloadToSend, payloadToSendSize);
    }
    else {
        Serial.println(F("col.txt failed to open"));
        Serial.write(payloadToSend, payloadToSendSize);
    }
}

void BeginSession() {

    File dataFile = SD.open("col.txt", FILE_WRITE);
    if (dataFile) {
        dataFile.println("\nCollision Detected! ");
        dataFile.print("Session log for ");
        dataFile.write(timeStamp, 20);
        dataFile.println("******************** Begin Session ******************");
        dataFile.close();
    }
    else {
        Serial.println(F("col.txt failed to open"));
        Serial.write(payloadToSend, payloadToSendSize);
    }
}

void EndSession() {
    File dataFile = SD.open("col.txt", FILE_WRITE);
    if (dataFile) {
        dataFile.print("\nNumber of Cars Contacted This Session: ");
        dataFile.print(contacted);
        dataFile.print("\nTotal Number of times I have initiated contact: ");
        dataFile.print(timesInitiatedContact);
        dataFile.print("\nTotal Number of times I have written to SD: ");
        dataFile.print(timesWrittenToSD);
        dataFile.println("\n******************** End Session ********************");
        dataFile.close();
    }
    else {
        Serial.println(F("col.txt failed to open"));
        Serial.write(payloadToSend, payloadToSendSize);
    }
}

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

    Serial.write(payloadToSend, payloadToSendSize);
    tx = Tx16Request(0xFFFF, payloadToSend, payloadToSendSize);
    payloadToSendSize -= 20;
    for (int i = 0; i < 20; i++) {
        timeStamp[i] = payloadToSend[payloadToSendSize + i];
    }
    Serial.write(timeStamp, 20);
}

void setColor(int red, int green, int blue) {
    analogWrite(redPin, red);
    analogWrite(greenPin, green);
    analogWrite(bluePin, blue);
}

void Impact() {

    time1 = micros();
    updateflag = 1;
}
