#include <analogWrite.h>
#include "UbidotsEsp32Mqtt.h"
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define uS_TO_MIN_FACTOR 1000000  //Conversion factor for micro seconds to seconds

RTC_DATA_ATTR int bootCount = 0;

RTC_DATA_ATTR int Critical_Value = 5;
RTC_DATA_ATTR float maxWaterLevel = 12;
RTC_DATA_ATTR float Current_Value;
RTC_DATA_ATTR int TIME_TO_SLEEP = 10;

RTC_DATA_ATTR float prevAvg;
RTC_DATA_ATTR float sum;
RTC_DATA_ATTR float average;
RTC_DATA_ATTR int counter = 0;

const char *UBIDOTS_TOKEN = "BBFF-8t47OTF4vv2Mjv4KNYOsGAP2cENC6z";  // Put here your Ubidots TOKEN
const char *WIFI_SSID = "Jonas Surface Pro";      // Put here your Wi-Fi SSID
const char *WIFI_PASS = "123454321";      // Put here your Wi-Fi password
const char *DEVICE_LABEL = "ESP32";
const char *VARIABLE_LABEL1 = "Vann.mgd & Pos."; // Put here your Variable label to which data  will be published

RTC_DATA_ATTR char* str_lat = (char*)malloc(sizeof(char) * 10);
RTC_DATA_ATTR char* str_lng = (char*)malloc(sizeof(char) * 10);
RTC_DATA_ATTR char* context = (char*)malloc(sizeof(char) * 30);

RTC_DATA_ATTR float lat; //Placeholder
RTC_DATA_ATTR float lng; //Placeholder

static const int RXPin = 26;
static const int TXPin = 25;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;

SoftwareSerial ss(RXPin, TXPin);

const int PUBLISH_FREQUENCY = 5000; // Update rate in milliseconds

unsigned long timer;
uint8_t analogPin = 34; // Pin used to read data from GPIO34 ADC_CH6.

Ubidots ubidots(UBIDOTS_TOKEN);

//--PINS----//

#define triggerPIN 5 //PWM
#define echoPIN 18
#define discardValue 3 //beskytter slik at den ikke kødder seg. Fjerner forje måling og tar på nytt
#define sequence 5  //fem målinger blir tatt
#define totalHeight 30 

float readingFlow ()
{
  pinMode(triggerPIN, OUTPUT); //definerer den som en output
  pinMode(echoPIN, INPUT);
  digitalWrite(triggerPIN, LOW); //starter på low
  delayMicroseconds(2);
  digitalWrite(triggerPIN, HIGH); //blir så HIGH etter 5 sek
  delayMicroseconds(10);
  digitalWrite(triggerPIN, LOW);
  return pulseIn(echoPIN, HIGH);
  
}

float returnAverage() //gjennomsnittsfunksjon
{ 
 prevAvg = average;
 sum = 0;
 for (int i = 0; i < sequence; i++){
      sum = sum + (0.01723 * readingFlow());//Ganger med 0.1723 for å gjøre om til millimeter
      delay(50); //Pause
      if (i == (sequence - 1)){
        average = maxWaterLevel - (sum / sequence);
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

void Sleep(){
  if (Critical_Value < Current_Value){
    TIME_TO_SLEEP = 5;
  }
  else if (Current_Value/Critical_Value>1.1){
    TIME_TO_SLEEP = 10;
  }
  else {
    TIME_TO_SLEEP = 30;
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void MQTT(){
  ubidots.setDebug(true);  // uncomment this to make debug messages available
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();

  timer = millis();
  sprintf(str_lat, "%f", lat);
  sprintf(str_lng, "%f", lng);

  ubidots.addContext("lat", str_lat);
  ubidots.addContext("lng", str_lng);
  ubidots.getContext(context);

  if (!ubidots.connected())
  {
    ubidots.reconnect();
  }
  ubidots.add(VARIABLE_LABEL1, average, context); // Insert your variable Labels and the value to be sent
 // ubidots.add(VARIABLE_LABEL2, 2, context);
  ubidots.publish(DEVICE_LABEL);
  esp_deep_sleep_start();
}


void setup(){
  Serial.begin(9600);
  ss.begin(GPSBaud);
  delay(1000); //Take some time to open up the Serial Monitor

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  Current_Value = returnAverage();
  Serial.print("Distance is ");
  Serial.println(Current_Value);
  Sleep();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_MIN_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Minutes");
}

void loop(){
  if (bootCount==1){
  while (ss.available() > 0){
    gps.encode(ss.read());
    if (gps.location.isUpdated()){
      Serial.print("Latitude= "); 
      Serial.print(gps.location.lat(), 6);
      Serial.print(" Longitude= "); 
      Serial.println(gps.location.lng(), 6);
      lng = gps.location.lng();
      lat = gps.location.lat();
      MQTT();
    }
  }
  }
  if(bootCount!=1){
    MQTT();
  }
}
