/* -------------------------------------------------------------------------- */
/*                                  Libraries                                 */
/* -------------------------------------------------------------------------- */
#include <Arduino.h>
#include "PMS.h"
#include <MQUnifiedsensor.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SD.h>
#include <Wire.h>
#include "ds3231.h"

/* -------------------------------------------------------------------------- */
/*                           Hardware Related Macros                          */
/* -------------------------------------------------------------------------- */
#define board "Arduino UNO"
#define voltageResolution 5
#define readingMQ2 A0 
#define readingMQ135 A1
#define readingMQ8 A2
/* ---------------------------------- DHT22 --------------------------------- */
#define DHTPIN 48
#define DHTTYPE DHT22
/* --------------------------------- microSD -------------------------------- */
#define PIN_SD_CS 53

/* -------------------------------------------------------------------------- */
/*                           Software Related Macros                          */
/* -------------------------------------------------------------------------- */
String version = "0.1";
#define calibrationSamples 300
#define adcResolution 10
/* --------------------------------- DS3231 --------------------------------- */
int setHour=12; 
int setMin=30;
int setSec=0;
int setDay=25;
int setMonth=12;
int setYear=2019;
struct ts t;
int hour;
int minute;
int seconds;
int day;
int month;
int year;
/* -------------------------------------------------------------------------- */
/*                         Application Related Macros                         */
/* -------------------------------------------------------------------------- */
/* -------------------------- Gas Sensors Settings -------------------------- */
#define ratioMQ2CleanAir 9.83
#define ratioMQ135CleanAir 3.6
#define ratioMQ8CleanAir 70
#define rLMQ2 4.58
#define rLMQ135 21.10
#define rLMQ8 9.75
/* --------------------------- Measured variables --------------------------- */
volatile float LPG = 0;
volatile float CO2 = 0;
volatile float  H2 = 0;
volatile float PM1_0 = 0;
volatile float PM2_5 = 0;
volatile float PM10_0 = 0;
volatile float temperature = 0;
volatile float humidity = 0;
/* ------------------------- Gas Sensors Declaration ------------------------ */
MQUnifiedsensor MQ2(board, voltageResolution, adcResolution, readingMQ2, "MQ-2");
MQUnifiedsensor MQ135(board, voltageResolution, adcResolution, readingMQ135, "MQ-135");
MQUnifiedsensor MQ8(board, voltageResolution, adcResolution, readingMQ8, "MQ-8");
/* --------------------------- PMS5003 Declaration -------------------------- */
PMS pms(Serial1);
PMS::DATA data;
DHT dht(DHTPIN, DHTTYPE);
/* --------------------------------- microSD -------------------------------- */
File myFile;
String dataToSave;

/* -------------------------------------------------------------------------- */
/*                              GET DATE AND TIME                             */
/* -------------------------------------------------------------------------- */
void GetDateTime(){
  DS3231_get(&t);
  hour = t.hour;
  minute = t.min;
  seconds = t.sec;
  day = t.mday;
  month = t.mon;
  year = t.year;
}

/* -------------------------------------------------------------------------- */
/*                      SAVE THE DATA TO THE MICROSD CARD                     */
/* -------------------------------------------------------------------------- */
void SaveDataToSD(){
  myFile = SD.open("data.txt", FILE_WRITE);
  if (myFile) {
    /* ------------- Create timestamp strings and main data to save ------------- */
    dataToSave = (String(PM1_0),",",String(PM2_5),",",String(PM10_0),",",String(LPG),",",String(CO2),",",String(H2),",",String(temperature),",",String(humidity),"\n");
    Serial.println(dataToSave); 
    myFile.println(dataToSave);
    myFile.close();
  }
}

/* -------------------------------------------------------------------------- */
/*                           GAS SENSORS CALIBRATION                          */
/* -------------------------------------------------------------------------- */
void  Calibration() {

  /* ------------------------ R0 calculation variables ------------------------ */
  float MQ2calcR0 = 0;
  float MQ135calcR0 = 0;
  float MQ8calcR0 = 0;

  Serial.println("START | Calibration");

  /* -------------------------- Take R0 measurements -------------------------- */
  for(int i = 1; i<= calibrationSamples; i++){

    MQ2.update();
    MQ2calcR0 += MQ2.calibrate(ratioMQ2CleanAir);
    MQ135.update();
    MQ135calcR0 += MQ135.calibrate(ratioMQ135CleanAir);
    MQ8.update();
    MQ8calcR0 += MQ8.calibrate(ratioMQ8CleanAir);
    delay(1000);

  }

  /* ------------- Set R0 values according to the average readings ------------ */
  MQ2.setR0(MQ2calcR0/calibrationSamples);
  MQ135.setR0(MQ135calcR0/calibrationSamples);
  MQ8.setR0(MQ8calcR0/calibrationSamples);
  Serial.println("END | Calibration");

}

/* -------------------------------------------------------------------------- */
/*                                DHT22 READING                               */
/* -------------------------------------------------------------------------- */
void ReadDHT22(){
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}

/* -------------------------------------------------------------------------- */
/*                               PMS5003 READING                              */
/* -------------------------------------------------------------------------- */
void PMS5003(){
  pms.requestRead();
   if (pms.readUntil(data))
  {
    PM1_0 = data.PM_AE_UG_1_0;
    PM2_5 = data.PM_AE_UG_2_5;
    PM10_0 = data.PM_AE_UG_10_0;
  }
}

/* -------------------------------------------------------------------------- */
/*                            GASES SENSORS READING                           */
/* -------------------------------------------------------------------------- */
void Gases(){
  MQ2.update();
  LPG = MQ2.readSensor();
  MQ135.update();
  CO2 = MQ135.readSensor();
  MQ8.update();
  H2 = MQ8.readSensor();
}

/* -------------------------------------------------------------------------- */
/*                             ALL SENSORS READING                            */
/* -------------------------------------------------------------------------- */
void ReadSensors() {
  PMS5003();
  Gases();
  ReadDHT22();
}

/* -------------------------------------------------------------------------- */
/*                          PRINT MEASURED VARIABLES                          */
/* -------------------------------------------------------------------------- */
void PrintValues() {
  Serial.print("| PM 1.0: " + String(PM1_0) + "\t");
  Serial.print("| PM 2.5: " + String(PM2_5) + "\t");
  Serial.print("| PM 10.0: " + String(PM10_0) + "\t");
  Serial.println("|");
  Serial.print("| LPG: " + String(LPG) + "\t");
  Serial.print("| CO2: " + String(CO2) + "\t");
  Serial.print("| H2: " + String(H2) + "\t");
  Serial.println("|");
  Serial.print("| Temperature: " + String(temperature) + "\t");
  Serial.print("| Humidity: " + String(humidity) + "\t");
  Serial.println("|");
  Serial.println("*************************************************");
  myFile = SD.open("data.txt", FILE_WRITE);
   if (myFile) {
    myFile.print("| PM 1.0: " + String(PM1_0) + "\t");
    myFile.print("| PM 2.5: " + String(PM2_5) + "\t");
    myFile.print("| PM 10.0: " + String(PM10_0) + "\t");
    myFile.println("|");
    myFile.print("| LPG: " + String(LPG) + "\t");
    myFile.print("| CO2: " + String(CO2) + "\t");
    myFile.print("| H2: " + String(H2) + "\t");
    myFile.println("|");
    myFile.print("| Temperature: " + String(temperature) + "\t");
    myFile.print("| Humidity: " + String(humidity) + "\t");
    myFile.println("|");
    myFile.println("*************************************************");
    myFile.close();
  }
}

/* -------------------------------------------------------------------------- */
/*                                    SETUP                                   */
/* -------------------------------------------------------------------------- */
void setup()
{
  delay(12000);
  /* -------------------------- Serial Communication -------------------------- */
  Serial.begin(9600); // Board debbuging
  Serial1.begin(9600);  // PMS5003 communication
  
  /* ---------------------------- IC2 Communication --------------------------- */
  Wire.begin();

  /* ------------------------- PMS5003 Initialization ------------------------- */
  pms.passiveMode();  // Switch to passive mode
  pms.wakeUp(); 

  /* --------------------------- Pin Initialization --------------------------- */
  pinMode(DHTPIN, INPUT);
  pinMode(readingMQ2, INPUT);
  pinMode(readingMQ135, INPUT);
  pinMode(readingMQ8, INPUT);

  /* ------------------------------ DS3231 Setup ------------------------------ */
  DS3231_init(DS3231_CONTROL_INTCN);
  t.hour=setHour; 
  t.min=setMin;
  t.sec=setSec;
  t.mday=setDay;
  t.mon=setMonth;
  t.year=setYear;
  DS3231_set(t);

  /* ------------------------------ microSD Setup ----------------------------- */
  if (!SD.begin(PIN_SD_CS)){
    while(1);
    Serial.println("SD Failed");
  }else

  /* -------------------------- DHT22 Initialization -------------------------- */
  dht.begin();

  /* --------------------------- MQ-2 Initialization -------------------------- */
  MQ2.setRL(rLMQ2);
  MQ2.setRegressionMethod(1);
  MQ2.setA(574.25); MQ2.setB(-2.222); // Calculate LPG concentration value
  MQ2.init();
  /*
    Exponential regression:
    Gas    | a      | b
    H2     | 987.99 | -2.162
    LPG    | 574.25 | -2.222
    CO     | 36974  | -3.109
    Alcohol| 3616.1 | -2.675
    Propane| 658.71 | -2.168
  */

  /* -------------------------- MQ-135 Initialization ------------------------- */
  MQ135.setRL(rLMQ135);
  MQ135.setRegressionMethod(1);
  MQ135.setA(110.47); MQ135.setB(-2.862); // Calculate CO2 concentration value
  MQ135.init();
  /*
    Exponential regression:
  GAS      | a      | b
  CO       | 605.18 | -3.937  
  Alcohol  | 77.255 | -3.18 
  CO2      | 110.47 | -2.862
  Toluen  | 44.947 | -3.445
  NH4      | 102.2  | -2.473
  Aceton  | 34.668 | -3.369
  */

  /* --------------------------- MQ-8 Initialization -------------------------- */
  MQ8.setRL(rLMQ8);
  MQ8.setRegressionMethod(1);
  MQ8.setA(976.97); MQ8.setB(-0.688); // Calculate H2 concentration value
  MQ8.init();
  /*
    Exponential regression:
  GAS     | a      | b
  H2      | 976.97  | -0.688
  LPG     | 10000000 | -3.123
  CH4     | 80000000000000 | -6.666
  CO      | 2000000000000000000 | -8.074
  Alcohol | 76101 | -1.86
  */

  /* ------------------------------- Calibration ------------------------------ */
  Calibration();

}

/* -------------------------------------------------------------------------- */
/*                                    LOOP                                    */
/* -------------------------------------------------------------------------- */
void loop()
{
  ReadSensors();
  PrintValues();
  delay(2000);
}