//
// file: relay_motor_switch.ino
//
// Author: Ryan Morrison
//
// This file demonstrates the use of a relay module to control on/off
// states of DC motors. I am using this to stop the motors in a simple 
// model car, when it collides with something. Also, I have a DPDT 3-way
// switch connected to to the motors (forward, on/off, reverse).
//
// Devices: T010010 relay module (TinkerKit)
//          DPDT toggle switch (radio shack)      
//          DC motors (4)
//
// Wiring diagram: https://www.dropbox.com/s/9jyb4as8ozoswmh/relay_motor_switch.pdf?dl=0
//

#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Wire.h>
#include <math.h>

// relay pin
#define relayPin     37
#define relayPower   35
#define relayGnd     39


// instantiate accelerometer (magnometer not used here)
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321); // these IDs are arbitrary
//Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345);

byte updateflag;

sensors_event_t event;
//sensors_event_t magEvent;

// accel vars
int xaxis = 0, yaxis = 0, zaxis = 0;
int deltx = 0, delty = 0, deltz = 0;
int magnitude = 0;

//double angle;
//float impactDirection; // direction of travel prior to impact

// collision vars
int vibration = 0, sensitivity = 0, devibrate = 0;

// timing vars
unsigned long time1; // time since sketch started 
unsigned long time2;

void setup() {
  
    // setup relay pin; relay is default wired to NC so motors
    // are free to be controlled by switch.
    // this initialization guarantees relay is in this state
    // when sketch starts. 
    digitalWrite(relayPin, LOW);
    pinMode(relayPin, OUTPUT);
    
    digitalWrite(relayPower, HIGH);
    pinMode(relayPower, OUTPUT);
    
    digitalWrite(relayGnd, LOW);
    pinMode(relayPower, OUTPUT);
    
    Wire.begin();
    Serial.begin(9600);
    sensitivity = 32;
    devibrate = 75;
   
    // check if accel is running/connected
    if(!accel.begin()) {
        Serial.println("No LSM303 detected... check wiring!");
        while(1);
    }
    //if(!mag.begin()) {
    //    Serial.println("No LSM303 detected ... check wiring!");
    //    while(1);
    //}
  
    // update time since sketch started
    time1 = millis(); 
}

void loop() {

    accel.getEvent(&event);
    //mag.getEvent(&magEvent);
 
    if (micros() - time1 > 1999) { Impact(); }                           

    if(updateflag > 0) {
        updateflag=0;

        // print accel readings upon collision
        Serial.print("\n");
        Serial.print("dx: "); Serial.print(deltx); Serial.print("\t");
        Serial.print(" dy: "); Serial.print(delty); Serial.print("\t");
        Serial.print(" dz: "); Serial.println(deltz);
    
        // trigger relay to turn off motors
        // this causes the relay to switch to NO, thus causing the motors to stop. 
        digitalWrite(relayPin, HIGH);

    }  
}

/********* detects impact (poor implementation, but serves our purpose here) *********/
void Impact() {
    
    time1 = micros();                                              
    int oldx = xaxis;                                           
    int oldy = yaxis;
    int oldz = zaxis;

    vibration--;                                               
    if(vibration<0) vibration=0;
  
    xaxis = event.acceleration.x; 
    yaxis = event.acceleration.y; 
    zaxis = event.acceleration.z;
  
    if(vibration > 0) return;

    deltx = xaxis - oldx;                                           
    delty = yaxis - oldy;
    deltz = zaxis - oldz;
  
    magnitude = sqrt(sq(deltx) + sq(delty) + sq(deltz)); 
 
    if (magnitude > sensitivity) {
        updateflag = 1;    
        vibration = devibrate;                                     
        time2 = millis();  
    }
    else {
        magnitude = 0;                                            
    }
}
