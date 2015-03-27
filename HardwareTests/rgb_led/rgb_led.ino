//
// file: rgb_led.ino
//
// Author: Ryan Morrison
//
// This file demonstrates manipulation of analog pin values to 
// create various colors using an RGB LED.
//
// Devices: Common Cathode RGB LED
//          (3) 270 Ohm resistors   
//          
// Wiring diagram: https://www.dropbox.com/s/ya3ixe3pl3wljtp/rgb_led.pdf?dl=0
//

#define redPin      45
#define greenPin    43
#define bluePin     41

// if defines don't work use these
//int redPin = 11;
//int greenPin = 10;
//int bluePin = 9;

void setup() {

    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);  
}

void loop() {
    
    // this just cycles through each color we need
    // insert these at appropriate sections in code
    setColor(0x00, 0xff, 0x00);  // green: standby/on
    delay(1000);
    setColor(0xff, 0x00, 0x00);  // red: collision detected 
    delay(1000);
    setColor(0x00, 0x00, 0xff);  // blue: sending
    delay(1000);
    setColor(0x40, 0x00, 0x20);  // magenta: receiving
    delay(1000);
}

/********* changes pins to arg values *********/
void setColor(int red, int green, int blue) {
    analogWrite(redPin, red);
    analogWrite(greenPin, green);
    analogWrite(bluePin, blue);  
}
