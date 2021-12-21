#include <analogWrite.h>
#include "UbidotsEsp32Mqtt.h"
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

RTC_DATA_ATTR int bootCount = 0; // teller antall booter
RTC_DATA_ATTR float Critical_Value = 8;//Verdien når ESP'en går i aktiv modus
RTC_DATA_ATTR float maxWaterLevel = 12;//høgden fra sensor ned til bunnen av elven
RTC_DATA_ATTR float Current_Value;//Siste målte nivå av vannstanden
RTC_DATA_ATTR int TIME_TO_SLEEP;//Tiden ESP'en skal være i søvnmodus

//Brukt i Returnaverage()
RTC_DATA_ATTR float prevAvg;
RTC_DATA_ATTR float sum;
RTC_DATA_ATTR float average;
RTC_DATA_ATTR int counter = 0;

const char *UBIDOTS_TOKEN = "BBFF-8t47OTF4vv2Mjv4KNYOsGAP2cENC6z";  // Ubidots TOKEN
const char *WIFI_SSID = "Hope";      // Wi-Fi SSID
const char *WIFI_PASS = "hopehope";      // Wi-Fi password
const char *DEVICE_LABEL = "ESP32"; // Navn på enheten 
const char *VARIABLE_LABEL1 = "Vann.mgd & Pos."; //Sett inn variabeletiketten din som data skal publiseres til

RTC_DATA_ATTR char* str_lat = (char*)malloc(sizeof(char) * 10);
RTC_DATA_ATTR char* str_lng = (char*)malloc(sizeof(char) * 10);
RTC_DATA_ATTR char* context = (char*)malloc(sizeof(char) * 30);

RTC_DATA_ATTR float lat; //Placeholder
RTC_DATA_ATTR float lng; //Placeholder

static const int RXPin = 26, TXPin = 25; // Settter RX og TX til pinne 26 og 25
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps; 

SoftwareSerial ss(RXPin, TXPin); // Gjør pinne 26 og 25 til RX og TX pin

const int PUBLISH_FREQUENCY = 5000; // Opdateringsrate i millisekund

unsigned long timer;

Ubidots ubidots(UBIDOTS_TOKEN);

//--PINS----//

#define ultraPIN 5 
#define echoPIN 18
#define discardValue 3 //Beskytter slik at den ikke kødder seg. Fjerner siste måling og tar en ny.
#define sequence 5  //fem målinger blir tatt
#define totalHeight 30 
#define uS_TO_MIN_FACTOR 60000000  // konverterer fra microsekund til minutt

// Funksjon som leser av ultrasonic sensor
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
//Gjennomsnittsfunksjon som tar igjenomsnitt av 5 målingar
float returnAverage() 
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

// Funksjon som finner hvor lenge ESP'en skal såve
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

void callback(char *topic, byte *payload, unsigned int length) // Funksjon for å få tilbakemedling fra Ubidots.
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

void MQTT(){  // Funksjon til å bruke MQTT til å sende data.
  ubidots.setDebug(true);  
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS); // tilkobler wifi med wifi parameterne 
  ubidots.setCallback(callback); // setter opp calbackfunsjonen 
  ubidots.setup(); 
  ubidots.reconnect();

  timer = millis();
  // Legger lat og lng inn som som char array
  sprintf(str_lat, "%f", lat); 
  sprintf(str_lng, "%f", lng);
  // Legger til lat og lng til context, som skal sendes med variabelen.
  ubidots.addContext("lat", str_lat); 
  ubidots.addContext("lng", str_lng);
  ubidots.getContext(context);

  if (!ubidots.connected())
  {
    ubidots.reconnect();
  }
  ubidots.add(VARIABLE_LABEL1, average, context); // Legger til det som skal sendes opp til ubidots
  ubidots.publish(DEVICE_LABEL); // Sender opp til ubidots 

  Sleep();  //Sjekker hvor lenge den skal sove
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_MIN_FACTOR); //Setter tiden ESP'en skal sove
  esp_deep_sleep_start(); //Setter ESP'en i søvnmodus
}


void setup(){
  // starter opp gps og serial monitor
  Serial.begin(9600);
  ss.begin(GPSBaud);
  delay(1000); 

  //Teller antall boot nummer og printer det
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  // Henter inn verdien fra sensoren og printer dette.
  Current_Value = returnAverage();
  Serial.print("Distance is ");
  Serial.println(Current_Value);
  
}

void loop(){
  if (bootCount==1){ //Kjører kun denne funksjonen ved første boot
  while (ss.available() > 0){//Går inn i while loop og venter på signal fra GPS
    gps.encode(ss.read());
    //Når GPS dataen er endra seg fra 0 verdi, sender det via MQTT til ubidots
    if (gps.location.isUpdated()){

      Serial.print("Latitude= "); 
      Serial.print(gps.location.lat(), 6);
      Serial.print(" Longitude= "); 
      Serial.println(gps.location.lng(), 6);
      // setter lng og lat til koordinatene som her hentet inn.
      lng = gps.location.lng();
      lat = gps.location.lat();

      MQTT();
    }
  }
  }
  else{
    MQTT();
  }
}
