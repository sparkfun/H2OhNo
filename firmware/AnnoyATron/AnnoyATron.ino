/*
 1-14-2013
 Spark Fun Electronics
 Nathan Seidle
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 Annoy-A-Tron is a super scary product that Think Geek sells: http://www.thinkgeek.com/product/b278/
 
 This is a very basic Annoy-A-Tron example that will beep every 5 minutes. We intentionally didn't build in
 low-power control so that if you happen to use this code you will only annoy your enemy for a few hours
 before the coin cell battery gives out.
 
 */

//Pin definitions for ATtiny
const byte buzzer1 = 0;
const byte buzzer2 = 1;

void setup()
{
  pinMode(buzzer1, OUTPUT);
  pinMode(buzzer2, OUTPUT);
}

void loop() 
{
  alarmSound();
  
  delay((long)1000 * 60 * 5); //5 minute delay
}

//This is just a unique (annoying) sound we came up with, there is no magic to it
//Comes from the Simon Says game/kit actually: https://www.sparkfun.com/products/10547
//250us to 79us
void alarmSound(void)
{
  // Toggle the buzzer at various speeds
  for (byte x = 250 ; x > 70 ; x--) 
  {
    //for (byte y = 0 ; y < 3 ; y++)
    for (byte y = 0 ; y < 1 ; y++) //Slightly modified to be a shorter beep
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
