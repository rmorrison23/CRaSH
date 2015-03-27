//
// file: rtc_accel_i2c.ino
//
// Author: Ryan Morrison
//
// This file demonstrates the use of Arduino I2C bus with multiple
// devices. The sketch simply reads 2 devices and prints readings 
// to the serial monitor.
//
// Devices: LSM303DLHC accelrometer/magnometer (Adafruit breakout)
//          DS1307 real-time clock (Adafruit breakout)
//
// Wiring diagram: https://www.dropbox.com/s/6vbxcikky3ruy7b/rtc_accel_i2c.pdf?dl=0 
//
// * Once it's wired and sketch is uploaded, just knock the accelerometer
// 


#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Wire.h>
#include <SPI.h>
#include "RTClib.h"
#include <math.h>

// instantiate RTC and accelerometer (magnometer not used here)
RTC_DS1307 RTC;
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
 
    Wire.begin();
    Serial.begin(9600);
    RTC.begin();
    sensitivity = 15;
    devibrate = 75;
   
    // check if RTC is running/connected
    if (!RTC.isrunning()) { Serial.println("RTC is not running!"); }
    else { Serial.println("RTC is running..."); }
   
    // check if accel is running/connected
    if(!accel.begin()) { 
        Serial.println("No LSM303 detected... check wiring!");
        while(1);
    }
    else { Serial.println("Accelerometer is running..."); }
  
    // check if mag is running/connected
    //if(!mag.begin()) {
    //    Serial.println("No LSM303 detected... check wiring!");
    //    while(1);
    //}
    //else { Serial.println("Magnometer is running..."); }
  
    // update time since sketch started
    time1 = millis(); 
}

void loop() {

    accel.getEvent(&event);
    //mag.getEvent(&magEvent);
    
    // get present time from RTC
    DateTime collisionTime = RTC.now();
    
    // time = 2ms ? check impact : continue  
    if (micros() - time1 > 1999) { Impact(); }                  

    if(updateflag > 0) {
        updateflag = 0;
        Serial.print("\n");
        Serial.print("dx: "); Serial.print(deltx); Serial.print("\t");
        Serial.print(" dy: "); Serial.print(delty); Serial.print("\t");
        Serial.print(" dz: "); Serial.println(deltz);
        Serial.print(collisionTime.hour()); Serial.print(":");
        Serial.print(collisionTime.minute()); Serial.print(":");
        Serial.print(collisionTime.second()); 
        Serial.print("\n");
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
