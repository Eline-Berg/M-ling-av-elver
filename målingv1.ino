#include <analogWrite.h>

#define uS_TO_MIN_FACTOR 1000000  //Conversion factor for micro seconds to seconds

RTC_DATA_ATTR int bootCount = 0;

RTC_DATA_ATTR int Critical_Value = 10;
RTC_DATA_ATTR int Current_Value = 5;
RTC_DATA_ATTR int TIME_TO_SLEEP = 10;

RTC_DATA_ATTR int prevAvg;
RTC_DATA_ATTR int sum;
RTC_DATA_ATTR int average;
RTC_DATA_ATTR int counter = 0;

//--PINS----//

#define ultraPIN 5 //PWM
#define echoPIN 18
#define discardValue 3 //beskytter slik at den ikke kødder seg. Fjerner forje måling og tar på nytt
#define sequence 5  //fem målinger blir tatt
#define totalHeight 30 

void setup(){
	Serial.begin(115200);
	delay(1000); //Take some time to open up the Serial Monitor

	//Increment boot number and print it every reboot
	++bootCount;
	Serial.println("Boot number: " + String(bootCount));
   
  Serial.println(returnAverage());

  Sleep();

	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_MIN_FACTOR);
	Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
	" Minutes");

	//Go to sleep now
	esp_deep_sleep_start();
}

void loop(){}

void Sleep(){
  if (Critical_Value < Current_Value){
    TIME_TO_SLEEP = 1;
    
  }
  else if (Current_Value/Critical_Value>1.1){
    TIME_TO_SLEEP = 5;
  }
  else {
    TIME_TO_SLEEP = 10;
  }
}

float readingFlow ()
{
  pinMode(ultraPIN, OUTPUT); //definerer den som en output
  pinMode(echoPIN, INPUT);
  digitalWrite(ultraPIN, LOW); //starter på low
  delayMicroseconds(2);
  digitalWrite(ultraPIN, HIGH); //blir så HIGH etter 5 sek
  delayMicroseconds(10);
  digitalWrite(ultraPIN, LOW);
  return pulseIn(echoPIN, HIGH);
  
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
        if (counter == 1 && abs(average - prevAvg) < discardValue){ //vannnivå og IKKE avstand fra sensor til vann
          return average;
        }
        else if (counter == 0){
          counter = 1;
          return average;
        }
      }
    }
} 
