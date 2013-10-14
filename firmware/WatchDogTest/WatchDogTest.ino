/*
 1-14-2013
 Spark Fun Electronics
 Nathan Seidle
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 When the ATtiny senses water between two pins, go crazy. Make noise, blink LED.
 
 Created to replace the water sensor in my Nauticam 17401 underwater enclosure. The original board
 ate up CR2032s, sorta kinda worked some of the time, and had pretty low quality assembly. Did I mention it goes for $100?!
 We have the technology. We can make it better!
 
 You will need to have ATtiny supported for Arduino installed. See http://hlt.media.mit.edu/?p=1695 for more info.
 
 You will need to configure the ATtiny to run at 8MHz so that serial and other parts of the code work correctly.
 See "Configuring the ATtiny to run at 8MHz" on http://hlt.media.mit.edu/?p=1695 for more info.
 
 To program:
 Select USBtinyISP from the programmer menu (we recommend AVR Tiny Programmer https://www.sparkfun.com/products/11460)
 Select ATtiny85 w/ Internal 8MHz from the board menu
 
 We take a series of readings of the water sensor at power up. We then wait for a deviation of more than
 100 from the average before triggering the alarm.
 
 The alarm will run for a minimum of 2 seconds before shutting down.
 
 The original board had the following measurements @ 3.21V:
 In alarm mode: ~30mA with LED on and making sound
 Off: 10nA. Really? Wow. 
 
 This firmware doesn't yet put any power savings in place but should be possible (wake up every few second and take measurement).
 
 Currently uses 12.2mA in idle.
 Now down to 0.005mA.
 
 */

#include <avr/sleep.h> //Needed for sleep_mode

#include <SoftwareSerial.h>

SoftwareSerial mySerial(4, 3); // RX, TX

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

volatile int watchdog_counter;

//This runs each time the watch dog wakes us up from sleep
ISR(WDT_vect) {
  watchdog_counter++;
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

  //During power up, beep the buzzer to verify function
  alarmSound();
  delay(100);
  alarmSound();

  digitalWrite(buzzer1, LOW);
  digitalWrite(buzzer2, LOW);
  digitalWrite(statLED, LOW);

  watchdog_counter = 0;

  //Power down various bits of hardware to lower power usage  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); //Power down everything, wake up from WDT
  sleep_enable();
  ADCSRA &= ~(1<<ADEN); //Disable ADC, saves ~230uA
  
  setup_watchdog(3); //Wake up after 128 msec
}

void loop() 
{
  sleep_mode(); //Go to sleep!

  if(watchdog_counter > 30)
  {
    watchdog_counter = 0;

    alarmSound(); //Make noise!!
    digitalWrite(buzzer1, LOW);
    digitalWrite(buzzer2, LOW);
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

// 0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
// 6=1sec, 7=2sec, 8=4sec, 9=8sec
// From: http://interface.khm.de/index.php/lab/experiments/sleep_watchdog_battery/
void setup_watchdog(int timerPrescaler) {

  if (timerPrescaler > 9 ) timerPrescaler = 9; //Correct incoming amount if need be

  byte bb = timerPrescaler & 7; 
  if (timerPrescaler > 7) bb |= (1<<5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~(1<<WDRF); //Clear the watch dog reset
  WDTCR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
  WDTCR = bb; //Set new watchdog timeout value
  WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}
