
//  Inkludere bibliotek

#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// Definere konstanter:

static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;

// Starter TinyGPS++ bibloteket

TinyGPSPlus gps;

// Setter opp Serial tilkoblingen til GPS modulen 

SoftwareSerial ss(RXPin, TXPin);


void setup(){
  Serial.begin(9600);
  ss.begin(GPSBaud);
}

void loop(){
  // This sketch displays information every time a new sentence is correctly encoded.
  while (ss.available() > 0){
    gps.encode(ss.read());
    if (gps.location.isUpdated()){
      Serial.print("Latitude= "); 
      Serial.print(gps.location.lat(), 6);
      Serial.print(" Longitude= "); 
      Serial.println(gps.location.lng(), 6);
    }
  }
}
