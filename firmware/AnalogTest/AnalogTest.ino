/*
 2-3-2013
 Spark Fun Electronics
 Nathan Seidle
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 Test of analong input on ATtiny85
 
 Only analog input 3 (located on pin 3) seems to work.
 Analog1 (located on pin 2) rattles around from 850 to 950.
 Analog2 (located on pin 4) is always 1-3.
 */

#include <SoftwareSerial.h>

SoftwareSerial mySerial(0, 1); // RX, TX

void setup()
{
  pinMode(analog1, INPUT);
  mySerial.begin(9600);
}

void loop() 
{
  mySerial.print("A1/Pin2: ");
  mySerial.print(analogRead(A1));

  mySerial.print(" A2/Pin4: ");
  mySerial.print(analogRead(A2));

  mySerial.print(" A3/Pin3: ");
  mySerial.print(analogRead(A3));

  mySerial.println();
  delay(100);    
}

