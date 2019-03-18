/*
 * 8 nano - last rev.17 mar 2019
 * by oloturia
 * 
 */

#include <HIH61xx.h>         // HIH61XX humidity thermometer
#include <AsyncDelay.h>      // library used by HIH61XX for intervals
#include <Adafruit_BMP280.h> // BMP280 thermo barometer
#include <D7S.h>             // D7S accelerometer
#include <DS1302.h>          // DS1302 Real-Time Clock
#include <SPI.h>             // SPI connection for SD
#include <SD.h>              // SD
#include <Wire.h>            // I2C for D7S, BMP280, HIH6130

//DS1302 init
const int SCLKRTC = 8;
const int IORTC = 9;
const int CERTC = 10;
DS1302 rtc(CERTC, IORTC, SCLKRTC);

//SD Card
const int chipSelect = 4;
File dataFile;

//Photoresistor
const int photo = A0;
const int photo_limit = 150;
const int photo_pin = 6;

//BMP280 thermometer barometer, address 0x77
Adafruit_BMP280 bme;

//HIH61XX humidity thermometer, address 0x27
HIH61xx<TwoWire> hih(Wire);
AsyncDelay samplingInterval;

//D7S, address 0x55
float istPGA;
float prevPGA = 0;
float istSI;

unsigned long previousMillis = 0;
const unsigned long delayMillis = 10000; //one minute interval

String queryData(){
    //RTC
    String dataString ="";
    dataString += rtc.getDateStr();
    dataString += ",";
    dataString += rtc.getTimeStr();
    dataString += ",";
    return dataString;
}

void setup() {
  // Set the clock to run-mode, and disable the write protection
  rtc.halt(false);
  rtc.writeProtect(false);

  //Setup Serial connection
  Serial.begin(9600);
  if(!SD.begin(chipSelect)){
    Serial.println("Card failed or not present");
    while(true){}
  }

  //Asks for a new date and time on startup
  Serial.print("Current date:");
  Serial.println(queryData());
  Serial.println("Enter new date (format DD MM YYYY hh mm)");
  bool set = true;
  while(!Serial.available() && set){
    if (millis()>10000){
      set = false;
      Serial.println("Timeout");
    }
  }
  if(set) {
    int rtcDay = Serial.parseInt();
    int rtcMonth = Serial.parseInt();
    int rtcYear = Serial.parseInt();
    int rtcHour = Serial.parseInt();
    int rtcMinute = Serial.parseInt();
    rtc.setTime(rtcHour, rtcMinute, 0);
    rtc.setDate(rtcDay, rtcMonth, rtcYear);
  }
  Serial.print("Date is now:");
  Serial.println(queryData());

  //set photoresistor pin and output pin
  pinMode(photo,INPUT);
  pinMode(photo_pin,OUTPUT);

  //start HIH61XX
  Wire.begin();
  hih.initialise();
  samplingInterval.start(3000, AsyncDelay::MILLIS);
  Serial.println("HIH61xx initialized");

  //start bmp280
  if(!bme.begin()) {
    Serial.println("BMP280 sensor not found");
  } else {
    Serial.println("BMP280 sensor found");
  }

  //start D7S
  D7S.begin();
  while(!D7S.isReady()) {
    delay(500);
  }
  Serial.println("D7S Started");
  D7S.setAxis(SWITCH_AT_INSTALLATION);
  Serial.println("Initializing D7S Sensor, keep it steady during the process");
  delay(2000);
  D7S.initialize();
  while(!D7S.isReady()){
    delay(500);
  }
  Serial.println("OK");
}

void loop() {

  if(millis()-previousMillis > delayMillis) {
    previousMillis += delayMillis;
    
    //BMP280 
    float pressure = bme.readPressure();

    //HIH61xx
    hih.read();
    float humidity = hih.getRelHumidity()/100.0;
    float temperature = hih.getAmbientTemp()/100.0;
    
    dataFile = SD.open("weather.csv", FILE_WRITE);
    if(dataFile){
      String tempString = queryData() + String(temperature) + ',' + String(pressure) + ',' + String(humidity);
      dataFile.println(tempString);
      Serial.println(temperature);
      Serial.println(pressure);
      Serial.println(humidity);
      Serial.println(tempString);
    } else {
      Serial.println("error opening weather.csv");
    }
    dataFile.close();
  
    //Photoresistor
    int photo_read = analogRead(photo);
    if(photo_read > photo_limit) {
      digitalWrite(photo_pin,LOW);    
    } else {
      digitalWrite(photo_pin,HIGH);
    }
    
  }


  //D7S, it records the maximum PGA detected during the earthquake
  if(D7S.isEarthquakeOccuring()){
    istPGA = D7S.getInstantaneusSI();
    istSI = D7S.getInstantaneusSI();
    if(istPGA > prevPGA){
      prevPGA = istPGA;
      dataFile = SD.open("quakes.csv", FILE_WRITE);
      if(dataFile){
        String tempString = queryData()+String(istPGA)+","+String(istSI);
        dataFile.println(tempString);
        Serial.println(tempString);
      } else {
        Serial.println("error opening earthquakes.csv");
      }
      dataFile.close();
    }
  } else {
    prevPGA = 0;
  }
  
}
