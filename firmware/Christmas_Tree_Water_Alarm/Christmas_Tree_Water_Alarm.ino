/*
 12-8-2013
 Spark Fun Electronics
 Nathan Seidle
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 When the ATtiny senses the lack of water between two pins, play jingle bells. Created to monitor the water level 
 of a Christmas Tree stand.  This is the reverse of the standard water alarm. We are going to set the alarm 
 off in the *absence* of water to let the user know to refill the Christmas tree. aka, H2OhYes!
 
 We take a series of readings of the water sensor at power up. We then wait for a deviation of more than
 100 from the average before triggering the alarm. The alarm will play every minute minute if water is not detected.
 
 This firmware uses some power saving to get the average consumption down to ~50uA average. With a CR2032 @ 200mAh 
 this should allow it to run for 4,000hrs or 166 days. This is easily extended by increasing the amount of time
 between water checks (currently once ever 4 seconds).
 
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
int wakeUpCounter = 0;

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
  mySerial.println("Xmas tree monitor");

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

  digitalWrite(statLED, HIGH);
  playJingleBells();

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
  setup_watchdog(8); //Setup watchdog to go off after 4sec
  sleep_mode(); //Go to sleep! Wake up 1sec later and check water

  //Check for water
  ADCSRA |= (1<<ADEN); //Enable ADC
  int waterDifference = abs(analogRead(waterSensor) - waterAvg);
  mySerial.print("Diff: ");
  mySerial.println(waterDifference);

  if(waterDifference < maxDifference) //Ahhh! There's no water! Alarm!
  {
    wakeUpCounter++; 

    //We only want the alarm to go off every 60 seconds
    //The watchdog will wake us up every 4 seconds.
    //We use the wakeUpCounter to keep track of how many times we woke up
    //and detected a lack of water. If we have 60/4 = 15 consecutive wakeups with no
    //water then sound the alarm.

    if(wakeUpCounter > (60 / 4))
    {
      wdt_disable(); //Turn off the WDT

      wakeUpCounter = 0; //Reset the counter

      //Make a brief amount of noise. We don't want anyone going crazy.
      digitalWrite(statLED, HIGH);
      playJingleBells();

      digitalWrite(buzzer1, LOW);
      digitalWrite(buzzer2, LOW);
      digitalWrite(statLED, LOW); //No more alarm. Turn off LED
    }
  }
  else
    wakeUpCounter = 0; //Reset the counter
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

//From http://micahelsprofessionalportfolio.blogspot.com/2012/11/arduino-challenge-jingle-bells.html
//Note: On the ATtiny you have to pull arrays out of functions otherwise you get compile/pointer errors
char notes[] = "eeeeeeegcde"; // a space represents a rest
int beats[] = { 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 4};
void playJingleBells()
{
  int length = 11; // the number of notes
  int tempo = 100;

  for (char i = 0; i < length; i++)
  {
    if (notes[i] == ' ')
      delay(beats[i] * tempo); // rest
    else
      playNote(notes[i], beats[i] * tempo);

    // pause between notes
    delay(tempo / 2); 
  }

}

//Must be moved to global to compile on an ATtiny
char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };
void playNote(char note, int duration)
{
  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

void playTone(int frequency, int duration)
{
  for (long i = 0 ; i < duration * 1000L ; i += frequency * 2)
  {
    digitalWrite(buzzer2, HIGH);
    digitalWrite(buzzer1, LOW);
    delayMicroseconds(frequency);
    digitalWrite(buzzer2, LOW);
    digitalWrite(buzzer1, HIGH);
    delayMicroseconds(frequency);
  }
}
