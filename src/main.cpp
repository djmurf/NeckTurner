#include "U8glib.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "./lib/TimerOne.h"

// Stepper Motor Driver
#define DIR_PIN     8
#define STEP_PIN    9
#define CCWISE 1
#define CWISE 0
#define VSTEP 50

#define POT_PIN 3
#define MAX_SPEED 200
#define MIN_SPEED 1500
#define POWER_BUTTON 2

// Not positive on detection, but works with A4 -> SDA, A5 -> SCL
U8GLIB_SH1106_128X64 lcd(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_FAST);	// Dev 0, Fast I2C / TWI

int tmp102Address = 0x48;

int buttonState = 0;

//float temp = 80.8;
int rpms = 0;
int line1 = 10;
int line2 = 35;
int line3 = 50;

int velocity = MIN_SPEED;
int lastVelocity = MIN_SPEED;
long previousMillis = 0;
long interval = 100;

int readValue = 0;
int smoothedValue = 0;
int lastReadValue = 0;
float alpha = 0.70;

int readSpeed() {
    int read = analogRead(POT_PIN);
    readValue = map(analogRead(POT_PIN), 1023, 0, MIN_SPEED, MAX_SPEED);     // scale it to use it with the servo (value between 0 and 180)
    smoothedValue = (alpha) * smoothedValue + (1 - alpha) * readValue;
    return smoothedValue;
}

int readPot() {
    readValue = map(analogRead(POT_PIN), 10, 680, MIN_SPEED, MAX_SPEED);     // scale it to use it with the servo (value between 0 and 180)
    smoothedValue = (alpha) * smoothedValue + (1 - alpha) * readValue;
    return smoothedValue;
}


float getTemperature(){
  Wire.requestFrom(tmp102Address,2);

  byte MSB = Wire.read();
  byte LSB = Wire.read();

  //it's a 12bit int, using two's compliment for negative
  int TemperatureSum = ((MSB << 8) | LSB) >> 4;

  float celsius = TemperatureSum*0.0625;
  return celsius;
}


void setup(void) {
  // flip screen, if required
  // u8g.setRot180();
  Serial.begin(9600);
  Wire.begin();

  pinMode(POWER_BUTTON, INPUT);
  pinMode(DIR_PIN, OUTPUT);

  Timer1.initialize();
  digitalWrite(DIR_PIN, CWISE);

  lastVelocity = velocity = smoothedValue = readValue = MIN_SPEED;

  if ( velocity < MIN_SPEED ) {
      Timer1.pwm(STEP_PIN, 512, velocity);
  }

  lcd.setColorIndex(1);         // pixel on
  lcd.setFont(u8g_font_7x13);

}


void draw(int rpms) {
  // graphic commands to redraw the complete screen should be placed here

  float celsius = getTemperature();
  float fahrenheit = (1.8 * celsius) + 32;

  lcd.drawStr(0, line1, "Murf's Neck Turner");
  lcd.drawLine(0, line1+2, 128,line1+2);

  char strTemp[5];
  dtostrf(fahrenheit,5,1,strTemp);

  lcd.drawStr( 0, line3, "TEMP(\260F)");
  lcd.drawStr( 79, line3, ":");
  lcd.drawStr( 90,line3,strTemp);

  char strRpms[5];

  dtostrf(rpms,5,0,strRpms);

  lcd.drawStr( 0, line2, "SPEED(rpms)");
  lcd.drawStr( 79, line2, ":");
  lcd.drawStr( 90, line2, strRpms);

  lcd.drawLine(0, 63, 128,63);
}

void loop(void) {

    rpms = readSpeed();
    unsigned long currentMillis = millis();

    if(currentMillis - previousMillis > interval) {
        previousMillis = currentMillis;
        lcd.firstPage();
        do {
            draw(rpms);
        } while( lcd.nextPage() );
    }

  readValue = rpms;

  buttonState = digitalRead(POWER_BUTTON);

  if ( buttonState == 0 ) {
      readValue = MIN_SPEED;
  }

  // motor logic

  if ( readValue > MIN_SPEED ) {
      readValue = MIN_SPEED ;
  } else if (readValue < MAX_SPEED ) {
      readValue = MAX_SPEED;
  }

  if ( readValue < velocity ) {
      velocity = velocity - VSTEP;
      if(velocity < readValue)
      velocity = readValue;

  } else if ( readValue > velocity ) {

      velocity = velocity + VSTEP;
      if(velocity > readValue)
      velocity = readValue;
  }

  if ( velocity != lastVelocity ) {
      lastVelocity = velocity;
      if ( velocity == MIN_SPEED ) {
          Serial.println("Stopping..");
          Timer1.pwm(STEP_PIN, 0, 0);
      } else {
          Timer1.pwm(STEP_PIN, 512, velocity);
      }
  }

}
