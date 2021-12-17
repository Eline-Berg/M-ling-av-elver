#include <analogWrite.h>
#include "UbidotsEsp32Mqtt.h"
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define uS_TO_MIN_FACTOR 1000000

RTC_DATA_ATTR int bootCount = 0;

RTC_DATA_ATTR int Critical_Value = 5;
RTC_DATA_ATTR float maxWaterLevel = 12;
RTC_DATA_ATTR float Current_Value;
RTC_DATA_ATTR int TIME_TO_SLEEP = 10;

RTC_DATA_ATTR float prevAvg;
RTC_DATA_ATTR float sum;
RTC_DATA_ATTR float average;
RTC_DATA_ATTR int counter = 0;

const char *UBIDOTS_TOKEN = "BBFF-8t47OTF4vv2Mjv4KNYOsGAP2cENC6z";
const char *WIFI_SSID = "Jonas Surface Pro";
const char *WIFI_PASS = "123454321";
const char *DEVICE_LABEL = "ESP32";
const char *VARIABLE_LABEL1 = "Vann.mgd & Pos.";

RTC_DATA_ATTR char* str_lat = (char*)malloc(sizeof(char) * 10);
RTC_DATA_ATTR char* str_lng = (char*)malloc(sizeof(char) * 10);
RTC_DATA_ATTR char* context = (char*)malloc(sizeof(char) * 30);

RTC_DATA_ATTR float lat;
RTC_DATA_ATTR float lng;

static const int RXPin = 26;
static const int TXPin = 25;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;

SoftwareSerial ss(RXPin, TXPin);

const int PUBLISH_FREQUENCY = 5000;
unsigned long timer;
uint8_t analogPin = 34;

Ubidots ubidots(UBIDOTS_TOKEN);

//--PINS----//

#define triggerPIN 5
#define echoPIN 18
#define discardValue 3
#define sequence 5
#define totalHeight 30 

float readingFlow ()
{
  pinMode(triggerPIN, OUTPUT);
  pinMode(echoPIN, INPUT);
  digitalWrite(triggerPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPIN, LOW);
  return pulseIn(echoPIN, HIGH);
  
}

float returnAverage()
{ 
 prevAvg = average;
 sum = 0;
 for (int i = 0; i < sequence; i++){
      sum = sum + (0.01723 * readingFlow());
      delay(50); //Pause
      if (i == (sequence - 1)){
        average = maxWaterLevel - (sum / sequence);
        if (counter == 1 && abs(average - prevAvg) < discardValue){
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
  ubidots.setDebug(true);
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
  ubidots.add(VARIABLE_LABEL1, average, context);
  ubidots.publish(DEVICE_LABEL);
  esp_deep_sleep_start();
}


void setup(){
  Serial.begin(9600);
  ss.begin(GPSBaud);
  delay(1000);

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
