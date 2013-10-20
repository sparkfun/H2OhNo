/*
 1-14-2013
 Spark Fun Electronics
 Nathan Seidle
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 When the ATtiny senses water between two pins, go crazy. Make noise, blink LED.
 
 Created to replace the water sensor in my Nauticam 17401 underwater enclosure. The original board
 ate up CR2032s, sorta kinda worked some of the time, and had pretty low quality assembly. Did I mention it goes for $100?!
 We have the technology. We can make it better!
 
 To use this code you must configure the ATtiny to run at 8MHz so that serial and other parts of the code work correctly.
 
 We take a series of readings of the water sensor at power up. We then wait for a deviation of more than
 100 from the average before triggering the alarm.
 
 The alarm will run for a minimum of 2 seconds before shutting down.
 
 The original board had the following measurements @ 3.21V:
 In alarm mode: ~30mA with LED on and making sound
 Off: 10nA. Really? Wow. 
 
 This firmware uses some power saving to get the average consumption down to ~50uA average. With a CR2032 @ 200mAh 
 this should allow it to run for 4,000hrs or 166 days. This is easily extended by increasing the amount of time
 between water checks (currently 1 per second).
 
 */

#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/wdt.h> //Needed to enable/disable watch dog timer

#include <SoftwareSerial.h>

SoftwareSerial mySerial(4, 3); // RX, TX

//Pin definitions for regular Arduino Uno (used during development)
/*const byte buzzer1 = 8;
 const byte buzzer2 = 9;
 const byte statLED = 10;
 const byte waterSensor = A0;*/

//Pin definitions for ATtiny
const byte buzzer1 = 0;
const byte buzzer2 = 1;
const byte statLED = 4;
const byte waterSensor = A1;

//Variables

//This is the average analog value found during startup. Usually ~995
//When hit with water, the analog value will drop to ~400. A diff of 100 is good.
int waterAvg = 0; 
int maxDifference = 100; //A diff of more than 100 in the analog value will trigger the system.

//This runs each time the watch dog wakes us up from sleep
ISR(WDT_vect) {
  //Don't do anything. This is just here so that we wake up.
}

void setup()
{
  pinMode(buzzer1, OUTPUT);
  pinMode(buzzer2, OUTPUT);
  pinMode(statLED, OUTPUT);

  //pinMode(waterSensor, INPUT_PULLUP);
  pinMode(2, INPUT); //When setting the pin mode we have to use 2 instead of A1
  digitalWrite(2, HIGH); //Hack for getting around INPUT_PULLUP

  mySerial.begin(9600);
  mySerial.println("H2Ohno!");

  //Take a series of readings from the water sensor and average them
  waterAvg = 0;
  for(int x = 0 ; x < 8 ; x++)
  {
    waterAvg += analogRead(waterSensor);

    //During power up, blink the LED to let the world know we're alive
    if(digitalRead(statLED) == LOW)
      digitalWrite(statLED, HIGH);
    else
      digitalWrite(statLED, LOW);

    delay(50);
  }
  waterAvg /= 8;

  mySerial.print("Avg: ");
  mySerial.println(waterAvg);

  //During power up, beep the buzzer to verify function
  alarmSound();
  delay(100);
  alarmSound();

  digitalWrite(buzzer1, LOW);
  digitalWrite(buzzer2, LOW);
  digitalWrite(statLED, LOW);

  //Power down various bits of hardware to lower power usage  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); //Power down everything, wake up from WDT
  sleep_enable();

}

void loop() 
{
  ADCSRA &= ~(1<<ADEN); //Disable ADC, saves ~230uA
  setup_watchdog(6); //Setup watchdog to go off after 1sec
  sleep_mode(); //Go to sleep! Wake up 1sec later and check water

  //Check for water
  ADCSRA |= (1<<ADEN); //Enable ADC
  int waterDifference = abs(analogRead(waterSensor) - waterAvg);
  mySerial.print("Diff: ");
  mySerial.println(waterDifference);

  if(waterDifference > maxDifference) //Ahhh! Water! Alarm!
  {
    wdt_disable(); //Turn off the WDT!!
    
    long startTime = millis(); //Record the current time
    long timeSinceBlink = millis(); //Record the current time for blinking
    digitalWrite(statLED, HIGH); //Start out with the uh-oh LED on

    //Loop until we don't detect water AND 2 seconds of alarm have completed
    while(waterDifference > maxDifference || (millis() - startTime) < 2000)
    {
      alarmSound(); //Make noise!!

      if(millis() - timeSinceBlink > 100) //Toggle the LED every 100ms
      {
        timeSinceBlink = millis();

        if(digitalRead(statLED) == LOW) 
          digitalWrite(statLED, HIGH);
        else
          digitalWrite(statLED, LOW);
      }

      waterDifference = abs(analogRead(waterSensor) - waterAvg); //Take a new reading

      mySerial.print("Read: ");
      mySerial.println(analogRead(waterSensor));
    } //Loop until we don't detect water AND 2 seconds of alarm have completed

    digitalWrite(buzzer1, LOW);
    digitalWrite(buzzer2, LOW);
    digitalWrite(statLED, LOW); //No more alarm. Turn off LED
  }
  

}

//This is just a unique (annoying) sound we came up with, there is no magic to it
//Comes from the Simon Says game/kit actually: https://www.sparkfun.com/products/10547
//250us to 79us
void alarmSound(void)
{
  // Toggle the buzzer at various speeds
  for (byte x = 250 ; x > 70 ; x--)
  {
    for (byte y = 0 ; y < 3 ; y++)
    {
      digitalWrite(buzzer2, HIGH);
      digitalWrite(buzzer1, LOW);
      delayMicroseconds(x);

      digitalWrite(buzzer2, LOW);
      digitalWrite(buzzer1, HIGH);
      delayMicroseconds(x);
    }
  }
}

//Sets the watchdog timer to wake us up, but not reset
//0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
//6=1sec, 7=2sec, 8=4sec, 9=8sec
//From: http://interface.khm.de/index.php/lab/experiments/sleep_watchdog_battery/
void setup_watchdog(int timerPrescaler) {

  if (timerPrescaler > 9 ) timerPrescaler = 9; //Limit incoming amount to legal settings

  byte bb = timerPrescaler & 7; 
  if (timerPrescaler > 7) bb |= (1<<5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~(1<<WDRF); //Clear the watch dog reset
  WDTCR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
  WDTCR = bb; //Set new watchdog timeout value
  WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}
