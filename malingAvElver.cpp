#include <CircusESP32Lib.h>
#include <analogWrite.h>

//--PINS----//

#define ultraPIN 5 //PWM
#define echoPIN 18
#define discardValue 3 //beskytter slik at den ikke kødder seg. Fjerner forje måling og tar på nytt
#define sequence 5  //fem målinger blir tatt
#define totalHeight 30 

float readingFlow ()
{
  pinMode(ultraPIN, OUTPUT); //definerer den som en output
  digitalWrite(ultraPIN, LOW); //starter på low
  delayMicroseconds(2);
  digitalWrite(ultraPIN, HIGH); //blir så HIGH etter 5 sek
  delayMicroseconds(10);
  digitalWrite(ultraPIN, LOW);
  pinMode(echoPin, INPUT);
  return pulseIn(echoPin, HIGH);
}

int returnAverage() //gjennomsnittsfunksjon
{ 
 prevAvg = average;
 sum = 0;
 for (int i = 0; i < sequence; i++){
      sum = sum + (0.01723 * readingFlow()); //Ganger med 0.1723 for å gjøre om til millimeter
      delay(50); //Pause
      if (i == (sequence - 1)){
        average = sum / sequence;
        if (counter == 1 && abs(average - prevAvg) < discardValue){
          average = totalHeight - average; //vannnivå og IKKE avstand fra sensor til vann
          return average;
        }
        else if (counter == 0){
          counter = 1;
          average = totalHeight - average;
          return average;
        }
      }
    }
} 

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}