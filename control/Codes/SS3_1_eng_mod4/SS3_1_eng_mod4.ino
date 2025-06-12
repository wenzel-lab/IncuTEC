/*
    SS3_eng.ino
    Control programm for an anaerobic box
    Copyright (c) 2020 Achim Herrmann
    Author     : Achim Herrmann
    Create Time: October 2020
    Change Log :
    The MIT License (MIT)
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include "Wire.h"
#include "DS1307.h"               // Real clock module library
DS1307 clock;                    
#include <SD.h>                   // SD library
#include <SPI.h>                  // SPI communication library for SD card
#include <SeeedOLED.h>            // Oled display libraey
#include "SCD30.h"                // SCD30 library

//SCD30
#if defined(ARDUINO_ARCH_AVR)
    #pragma message("Defined architecture for ARDUINO_ARCH_AVR.")
    #define SERIAL Serial
#elif defined(ARDUINO_ARCH_SAM)
    #pragma message("Defined architecture for ARDUINO_ARCH_SAM.")
    #ifdef SEEED_XIAO_M0
        #define SERIAL Serial
    #elif defined(ARDUINO_SAMD_VARIANT_COMPLIANCE)
        #define SERIAL SerialUSB
    #else
        #define SERIAL Serial
    #endif
#elif defined(ARDUINO_ARCH_SAMD)
    #pragma message("Defined architecture for ARDUINO_ARCH_SAMD.")
    #ifdef SEEED_XIAO_M0
        #define SERIAL Serial
    #elif defined(ARDUINO_SAMD_VARIANT_COMPLIANCE)
        #define SERIAL SerialUSB
    #else
        #define SERIAL Serial
    #endif
#elif defined(ARDUINO_ARCH_STM32F4)
    #pragma message("Defined architecture for ARDUINO_ARCH_STM32F4.")
    #ifdef SEEED_XIAO_M0
        #define SERIAL Serial
    #elif defined(ARDUINO_SAMD_VARIANT_COMPLIANCE)
        #define SERIAL SerialUSB
    #else
        #define SERIAL Serial
    #endif
#else
    #pragma message("Not found any architecture.")
    #define SERIAL Serial
#endif

//control parameters
const float co2set = 2;                               // CO2 concentration setting
const int co2period = 20;                             // CO2 injection period, x co2period times 3 seconds
const int co2gastime = 50;                            // CO2 injection time, in miliseconds
const float oxset = 0.40;                             // O2 setting
const float humset = 85;                              // Humidity setting
const float tempset = 27;                             // Temperature setting

const float OxMaxSoll = 0.40;                         // maximum of 02 before gasing (N2/H2)v
const byte GasFlowChamberDuration = 10;               // How long should be gased after detecting over OX (N2/H2)
const byte HumSoll = 85;                              // maximum of humidity before gasing (N2)
const byte HumDuration = 10;                          // How long should be gased after detecting humidity over HumSoll(N2)
boolean StdON = true;                                 // true for hourly gasing ON, set false for OFF

//H2 SetUp
const float R0 = 3.8;                                 // Resting resistance of H2 Sensors. Determine by using "H2 setup"
const int H2Pin = A2;                                 // PIN used for the H2 sensor
float H2ratio = 0;
const float H2soll = 0.15;                            // Minimum amount before automatic feeding of H2 containing gas
const int GasFlowSchleuseDuration = 300;              // How long should the air lock be gased (s)

//Display setUP
char line0[17];                                       // first line in Oled display: Temperature
char line1[17];                                       // second line in Oled display: Relative Humidity
char line2[17];                                       // third line in Oled display: Oxygen%
char line3[17];                                       // fourth line in Oled display: H2 indicator
char line4[17];                                       // fifth line in Oled display: CO2 concentration
char line5[17];                                       // sixth line in Oled display: Status of controller
char line6[17];                                       // seventh line in Oled display: (SD) errorcodes
byte TimeLine = 7;                                    // eight line in Oled display: date
boolean timeDisplayIni = false;                       // 0 declares dots have to be rewriten for time display

//O2 SetUP
const float VRefer = 3.3;                             // voltage of connected to the O2 sensor
const int pinAdc   = A0;                              // PIN used for the O2 sensor
float OxVout =0;
const int numReadings = 10;                           // Average of number of readings
float readings[numReadings];                          // single sensor readings
int OxReadIndex = 0;                                  // Index of current reading
float OxTotal = 0;                                    // running total
float OxAverage = 0;                                  // average of readings so far
float OxValue = 0;                                    // Oxygen in %
int GasFlowChamberTimer = 0;

//Humidity SetUp
float humidity = 0;
boolean HumControl = false;                           // initial state of the controller
int HumTimer = 0;

//Temperature setup
float temper = 0;

//CO2 setup
float co2general = 100;                               // Registra la concentración de CO2, inicializa en 100%
int counter = 0;

//SD SetUp
const int ChipSelect = 4;                             // Pin mounting the SD-Card
File mySD;                                            // variable for working with file object
boolean SdUberschrift = false;
byte SdWriteFrequenzy = 1;                            // How frequent a reading is saved ( 0 = 60 per minute; 59 = 1 per minute)

//General setup
int interruptor = 2;                                  // Switch pin
int rele = 3;                                         // Relay pin

//TIMING
byte TaktS = 0;                                       // count seconds till 60
byte TaktM = 0;                                       // count minutes till 60

//Steuerung                                           // PINs connecting to external equipment
const byte GasChamber = 5;                            // PIN relay switch for gas in chamber  
const byte GasSchleuse = 6;                           // PIN relay switch for gas in Airlock
const byte Schalter = 7;                              // PIN switch for on/off
const byte KillSwitch = 8;                            // PIN for changing gasing modes
boolean GasFlowChamber = false;
boolean GasFlowSchleuse = false;
int GasFlowSchleuseTimer = 0;
double GasFreq = -10000;                              // Variables for automatic shutdown of gasing
byte GasERROR;
boolean GasOverride = false;
boolean GasSchleuseStatus = false;
String Override = "GAS OVERRIDE!   ";                 // Shows status of Killswitch on Display
boolean GasFlowChamberSwitch = false;
boolean GasSchleuseWaitReset = false;
const int SicherheitsIntervall = 10000;               // Minimum expected time between gasing caused by high O2 or low H2 during normal oppperation
byte GasERRORLimit = 5;                               // Limit for how often this interval between gasings can be breached in sucsesion before error
byte tOLD;                                            // How often gasing happens per hour
boolean BegasungBuffer;
byte BegasungH;
boolean StdBegasung = false;                          // Controls the hourly gasing

void setup(){  
  Serial.begin(115200);                               // start serial for output
  delay(150);
  /* Power up,delay 150ms,until voltage is stable */
  Wire.begin();
  delay(600);
  
  clock.begin();
  delay(100);
  
  scd30.initialize();                                 // Initialize CO2, Temp, Hum sensor
  SeeedOled.init();                                   // initialze SEEED OLED display
  SeeedOled.setNormalDisplay();                       // Set display to normal mode (i.e non-inverse mode)
  SeeedOled.setPageMode();                            // Set addressing mode to Page Mode  
  SeeedOled.clearDisplay();                           // cleares the display
  
  pinMode(10,OUTPUT);                                 // Pin 10 is reserved for SD card; DONT USE
  SD.begin(ChipSelect);

  pinMode(GasChamber, OUTPUT);                        // Pins for Relays and Switches
  pinMode (GasSchleuse, OUTPUT);
  pinMode(Schalter, INPUT);
  pinMode(KillSwitch, INPUT);
  digitalWrite(KillSwitch, HIGH);

  pinMode(interruptor,INPUT);                         // Switch pin setting
  pinMode(rele,OUTPUT);                               // Relay pin setting


  //ONLY USE ONCE TO SET UP CLOCK!!!!
  /*clock.fillByYMD(2025,5,29);        //Jan 19,2019
   clock.fillByHMS(10,28,00);          //10:27 00"
   clock.fillDayOfWeek(TUE);          //Saturday
   clock.setTime(); */               //write time to the RTC chip
  tOLD = clock.hour;

  for (int thisReading = 0; thisReading < numReadings; thisReading++){
  readings[thisReading] = 0;
  }
}
//---------------------------------------------------------------------------------------------------------------

void loop(){
  DisplaySide0(); 
  //SD data save--------------------------------------------------------------------------------------------------
  String SDStatus = F("SdCard =");
  
  mySD = SD.open("DataLog.txt", FILE_WRITE);
  if (mySD){                                              // if SD card is mountes save state and display OK otherwise display SD error and continoue
    SDStatus = SDStatus + F(" OK   ");
  //writeFunktion
    if(SdUberschrift == false){                           // SD headline
      mySD.println(F("h,min,s,d,month,s since start,Temp,rHum,Ox,H2 rV,%CO2"));
      SdUberschrift = true;
    }
    if(TaktS % SdWriteFrequenzy == 0){                    // Gives sd card status ever x seconds as defined at startup
      //Time and datum
      mySD.print(clock.hour);mySD.print(',');mySD.print(clock.minute);mySD.print(',');mySD.print(clock.second);mySD.print(',');mySD.print(clock.dayOfMonth);mySD.print(',');mySD.print(clock.month);mySD.print(',');mySD.print(millis()/1000);mySD.print(',');
      //Data
      mySD.print(temper);mySD.print(',');
      mySD.print(humidity);mySD.print(',');
      mySD.print(OxValue);mySD.print(',');
      mySD.print(H2ratio);mySD.print(',');
      mySD.println(co2general);      
    }
    mySD.close();
  }
  else{
    SDStatus = SDStatus + F(" ERROR");                        // Display error if sd card is not mounted
    SdUberschrift = false;
    if(TaktS == 59) {SD.begin(ChipSelect);                    // Updates sd card status every minute
    }
  }
  
  for(int i=0;i<17;i++){                                      //Write SD status on display
    line6[i]=SDStatus[i];
  }
  //Display configuration----------------------------------------------------------------------------------
  SeeedOled.setTextXY(0,0);           
  SeeedOled.putString(line0);
  SeeedOled.setTextXY(1,0);           
  SeeedOled.putString(line1);
  SeeedOled.setTextXY(2,0);           
  SeeedOled.putString(line2);
  SeeedOled.setTextXY(3,0);           
  SeeedOled.putString(line3);
  SeeedOled.setTextXY(4,0);           
  SeeedOled.putString(line4);
  SeeedOled.setTextXY(5,0);           
  SeeedOled.putString(line5);
  SeeedOled.setTextXY(6,0);           
  SeeedOled.putString(line6);
  printTime(TimeLine);                       //displays the time in row x (currently 7)

  // CO2 control--------------------------------------------------------------------------------------
  if(co2general < co2set && digitalRead(interruptor) == HIGH){
    if(counter == 0){
      digitalWrite(rele, HIGH);
      delay(co2gastime);
      digitalWrite(rele, LOW);
    }
    counter = counter + 1;
    if(counter == co2period){
      counter = 0;
    }
  }
  else{
    digitalWrite(rele, LOW);
    counter = 0;
  }
  //Refresh frequency
  delay(2000);
  //CONTROL
  if (clock.minute == 59){                                                                      // resets hourly single gas injection
    StdBegasung = false; 
  }
  if (digitalRead(KillSwitch) == LOW){                                                            //Changes mode of control depending on Killswitch status and enables airlock cycling
    GasERROR = 0;
    if(GasSchleuseStatus == true){                                                                    //Checks if airlock cycling is ON and displays it on the display instead of GAS OVERRIDE
      // String Override = F("GAS OVERRIDE!   ");
    }
    for(int i=0;i<17;i++){
      line5[i]=Override[i];
    }
    GasOverride = true;
  }
  else if(digitalRead(KillSwitch) == HIGH && GasOverride == true){
    GasOverride = false;
    for(int i=0;i<16;i++){
      line5[i]=' ';
    }
  }
                                                                          //Emergency Shutdown and H2 level control

  if (GasERROR > GasERRORLimit ){                                                                 //Emergency Shutdown if Frequency of Gasing is too fast
    digitalWrite (GasChamber, LOW);
    GasFlowChamber = false;
    GasFlowChamberTimer = 0;
    delay(500);
    String Error = F("GASSING ERROR   ");
    for(int i=0;i<17;i++){
      line5[i]=Error[i];
    }
  }
  else if (clock.minute == 0 && StdBegasung == false && StdON == true){                                          // Injects N2/H2 once every hour, can be disabeld in setup
    digitalWrite(GasChamber, HIGH);
    GasFlowChamber = true;
    StdBegasung = true;
  }
  else if (H2ratio > H2soll && GasFlowChamber == false){
    digitalWrite(GasChamber, HIGH);
    GasFlowChamber = true;                                                              // Injects N2/H2 if H2 ratio is too low
    GasFreq = millis() - GasFreq;                                                       //Checks if gasing frequency is too fast and adds to the error limit
    if(GasFreq < SicherheitsIntervall) {
      GasERROR = GasERROR + 1;
    }
    else {
      GasERROR = 0;
    }
  }
  else if(GasFlowChamber == true && GasFlowChamberTimer < GasFlowChamberDuration){                               // Counts the time the gas injection is on
    GasFlowChamberTimer = GasFlowChamberTimer + 1;
  }
  else if(GasFlowChamberSwitch == false){                                                                       // Stops gas injection after the set time = GasFlowChamberDuration
    digitalWrite (GasChamber, LOW);
    GasFlowChamber = false;
    GasFlowChamberTimer = 0;
    GasFreq = millis();
    delay(500);                                                                                             //Shows if to fast of gasing is occuring if serial is enabled
    /*Serial.println(GasFreq);Serial.print("Error Nr:");Serial.println(GasERROR); */
  }
      
  //Humidity control
  if (HumSoll < humidity&& GasSchleuseStatus == false && HumControl == false){                       // Regulates Humidity
    digitalWrite (GasSchleuse, HIGH);
    HumControl = true;  
  }

  if (HumControl == true && HumTimer < HumDuration){
    HumTimer++;
  }
  else if (HumControl == true){
    HumTimer = 0;
    digitalWrite (GasSchleuse, LOW);
    HumControl = false;
  }

                                                                // User interface and 

  if(digitalRead(Schalter) == HIGH && GasOverride == false){                  // Injects N2/H2 if Killswitch is not on and Switch is engaged
    digitalWrite(GasChamber, HIGH);
    GasFlowChamber = true;
    GasFlowChamberTimer = 0;
    GasFlowChamberSwitch = true;                                              //Stops the timer turning of the manual gas injection
  }
  if(digitalRead(Schalter) == LOW && GasFlowChamberSwitch == true){          // Stops manual gas injection if switch is disengaged
    digitalWrite(GasChamber, LOW);
    GasFlowChamber = false;
    GasFlowChamberSwitch = false;
  }
  else if(digitalRead(Schalter) == HIGH && GasFlowChamberSwitch == true && GasOverride == true){          // Stops manual gas injection if the Killswitch is engaged
    digitalWrite(GasChamber, LOW);
    GasFlowChamber = false;
    GasFlowChamberSwitch = false;
  }

  if (HumControl == false){                                                                                // Stops airlock cycling and humidity control crossinteraction
    if(digitalRead(Schalter) == HIGH && GasOverride == true && GasSchleuseStatus == false && GasSchleuseWaitReset == false){             //activates airlock cycling if Killswitch is on and Switch is engaged
      digitalWrite(GasSchleuse, HIGH);
      GasSchleuseStatus = true;
      String Schleuse = F("Airlock Gas ");
      int SchleuseTLeft = GasFlowSchleuseDuration - GasFlowSchleuseTimer;
      Schleuse = Schleuse + SchleuseTLeft;
      for(int i=0;i<17;i++){
        line5[i]=Schleuse[i];
      }
    }
    else if(digitalRead(Schalter) == LOW || digitalRead(KillSwitch) == HIGH)  {                    //Stops Airlock cycling if switch is disengaged
      digitalWrite(GasSchleuse, LOW);
      GasSchleuseStatus = false;
      GasFlowSchleuseTimer = 0;
      delay(500);
      if (digitalRead(Schalter) == LOW){                                                       // Stops the airlock cycling from reengaging after the cycle is complete
        GasSchleuseWaitReset = false;
      }
    }
    else if (GasFlowSchleuseTimer < GasFlowSchleuseDuration && GasSchleuseWaitReset == false){    //Counts time from activation of airlock and displays the remaining time on the display
      GasFlowSchleuseTimer = GasFlowSchleuseTimer + 1;
      String Schleuse = F("Airlock Gas ");
      int SchleuseTLeft = GasFlowSchleuseDuration - GasFlowSchleuseTimer;
      if (SchleuseTLeft < 100){
        Schleuse = Schleuse + ' ';
      }
      if (SchleuseTLeft < 10){
        Schleuse = Schleuse + ' ';
      }
      Schleuse = Schleuse + SchleuseTLeft;
      for(int i=0;i<17;i++){
        line5[i]=Schleuse[i];
      }
    }
    else{                                                                                     // Stops airlock cycling after set time and prevents reengaging without reseting the switch
      digitalWrite(GasSchleuse, LOW);
      GasFlowSchleuseTimer = 0;
      GasSchleuseStatus = false;
      GasSchleuseWaitReset = true;
      delay(500);
    }
  }

  if(TaktS <59){
    TaktS= TaktS + 1;                         //counts seconds
  }
  else{
    TaktS = 0;                              //counts minutes
    TaktM = TaktM + 1;
  }
  if(TaktM >= 60){
    TaktM = 0;
  } 

  /*                                                                  // counts the gasing cycles per hour and logs it on the SD card
  if(tOLD == clock.hour && BegasungBuffer != GasFlowChamber){
    BegasungH = BegasungH + 1;
    BegasungBuffer = GasFlowChamber;
  }
  else if (tOLD != clock.hour){
    tOLD = clock.hour;
    mySD = SD.open("DataLog.txt", FILE_WRITE);
    if (mySD){BegasungH = BegasungH/2;
    mySD.print(F("#NV,#NV,#NV,#NV,#NV,#NV,#NV,#NV,#NV,#NV,#NV,"));
    mySD.println(BegasungH);
    mySD.close();
  }
  BegasungH = 0;
  */
} 
//Functions of the sensors:
int DisplaySide0(){                                                                      
  //H2 measurement--------------------------------------------------------------------------
  float H2sensor_volt;
  float RS_gas;                                                     // Get value of RS in a GAS
  H2ratio;                                                          // Get ratio RS_GAS/RS_air
  int H2sensorValue = analogRead(H2Pin);
  H2sensor_volt=(float)H2sensorValue/1024*5.0;
  if (H2sensor_volt > 0){
    RS_gas = (5.0-H2sensor_volt)/H2sensor_volt;                      // omit *RL
    H2ratio = RS_gas/R0;                                            // ratio = RS/R0
  }

  //Serial.print("H2sensor_volt = ");
  //Serial.println(H2sensor_volt);
  //Serial.print("RS_ratio = ");
  //Serial.println(RS_gas);
  //Serial.print("Rs/R0 = ");
  //Serial.println(H2ratio);
  //Serial.print("\n\n");
  Serial.print(counter); 
  Serial.print(",");
  Serial.println(co2general);

  //Display H2----------------------------------------------------------------------------------------------------------------
  String H2Display = F("H2: ");
  char H2Buff[6];
  if (H2sensor_volt <= 0){
    H2Display = H2Display + F(" ERR");
  }
  else if(H2ratio > 5){
  H2Display = H2Display + F("NONE");
    }
  else if(H2ratio <= 5 && H2ratio > 0.075){
    H2Display = H2Display + F(" LOW");
  }
  else{
    H2Display = H2Display + F("NORM");
  }                                                                  
  dtostrf(H2ratio, 5, 2, H2Buff);                                  //creates a string lenghte 5, decimals 2 and name H2Buff                                                                
  H2Display = H2Display +  H2Buff;                                 //adds the strings

  for(int i=0;i<17;i++){
    line3[i]=H2Display[i];
  }
    
  //O2 measurment--------------------------------------------------------------------------------------------------------------
  OxValue = readConcentration();
  //displays O2 concentration in %
  String OxDisplay= F("OxLvl");
  if (OxValue < 10){
     OxDisplay = OxDisplay + "  ";
  }
  else{
    OxDisplay = OxDisplay + ' ';
  }
  char OxBuff[6];                                              
  dtostrf(OxValue, 5, 2, OxBuff);                                //creates a string lenghte 5, decimals 2 and name OxBuff      
  OxDisplay = OxDisplay + OxBuff + " %";
  for(int i=0;i<17;i++){
    line2[i]=OxDisplay[i];
  }
   
  //CO2, Temp and Hum measurement----------------------------------------------------------------------------------------------------------
  float result[3] = {0};
  if (scd30.isAvailable()) {
      scd30.getCarbonDioxideConcentration(result);          //SCD30 
  }

  //Display temperature
  temper = result[1];   
  char tBuff[6]; 
  dtostrf(temper, 5, 2, tBuff);                              //creates a string lenghte 5, decimals 2 and name tBuff
  String tDisplay = F("Temp ");
  if(temper <-10 || temper >80){
    tDisplay = tDisplay + F("RANGE ERROR");
  }
  else{
    tDisplay = tDisplay + tBuff + " *C";
  }
  for(int i=0;i<17;i++){
    line0[i]=tDisplay[i];
  }
      
  //Display humidity
  humidity = result[2];   
  String rhDisplay = F("Hum");            
  if(humidity < 0){                                               // if hum is close to 0 or 100% displays error
    rhDisplay = rhDisplay + F("  RANGE LOW");
  }
  else if(humidity >= 100){
    rhDisplay = rhDisplay + F("  RANGE HIGH");
  }
  else{
  char rhBuff[6];                                                      
  dtostrf(humidity, 5, 2, rhBuff);                                //creates a string lenghte 5, decimals 2 and name rhBuff
  rhDisplay = rhDisplay +"  "+ rhBuff + F(" %");     
  }
  for(int i=0;i<17;i++){
    line1[i]=rhDisplay[i];
  }
  
  //Display CO2
  float co2 = result[0]/10000;
  co2general = co2;   
  char co2Buff[6]; 
  dtostrf(co2, 5, 2, co2Buff);                              //creates a string lenghte 5, decimals 2 and name tBuff
  String co2Display = F("CO2 ");
  co2Display = co2Display + co2Buff + " %";
  
  for(int i=0;i<17;i++){
    line4[i]=co2Display[i];
  }
}

int printTime(byte timeLine){                                        //sets the line to display the time
   if(timeDisplayIni == false){                                    //0 declares dots have to be rewriten for time display            // puts characters for time display
    SeeedOled.setTextXY(timeLine,0);
    SeeedOled.putString("  :      .  .");
    timeDisplayIni = true;
    }
  clock.getTime();                                                  // pulls data for time from RTC module
  SeeedOled.setTextXY(timeLine,0);                                  //prints hour
  if(clock.hour>=10){           
    SeeedOled.putNumber(clock.hour);
  }
  else{
    SeeedOled.putNumber(0);
    SeeedOled.setTextXY(timeLine,1);
    SeeedOled.putNumber(clock.hour);
  }
  
  SeeedOled.setTextXY(timeLine,3);                                  //prints mins
  if(clock.minute>=10){           
    SeeedOled.putNumber(clock.minute);
  }
  else{
    SeeedOled.putNumber(0);
    SeeedOled.setTextXY(timeLine,4);
    SeeedOled.putNumber(clock.minute);         
  }
  
  SeeedOled.setTextXY(timeLine,7);                                  //prints day
  if(clock.dayOfMonth>=10){           
    SeeedOled.putNumber(clock.dayOfMonth);
  }
  else{
    SeeedOled.putNumber(0);
    SeeedOled.setTextXY(timeLine,8);
    SeeedOled.putNumber(clock.dayOfMonth);         
  }

  SeeedOled.setTextXY(timeLine,10);                                 //prints month
  if(clock.month>=10){           
    SeeedOled.putNumber(clock.month);
  }
  else{
    SeeedOled.putNumber(0);
    SeeedOled.setTextXY(timeLine,11);
    SeeedOled.putNumber(clock.month);         
  }
  SeeedOled.setTextXY(timeLine,13);
  SeeedOled.putNumber(clock.year);

}

float readO2Vout(){                                                //rutines for O2 measurment [4]
  long sum = 0;
  for(int i=0; i<32; i++){
    sum += analogRead(pinAdc);
  }
  sum >>= 5;
  float MeasuredVout = sum * (VRefer / 1023.0);
  return MeasuredVout;
}

float readConcentration()           {
  // Vout samples are with reference to 3.3V
  float MeasuredVout = readO2Vout();
  //float Concentration = FmultiMap(MeasuredVout, VoutArray,O2ConArray, 6);
  //when its output voltage is 2.0V,
  float Concentration = MeasuredVout * 0.21 / 1.0;
  if(Concentration < 0){
    Concentration = 0;
  }
  OxTotal = OxTotal - readings[OxReadIndex];    //substract last reading
  readings[OxReadIndex] = Concentration*100;    // read from sensor
  OxTotal = OxTotal + readings[OxReadIndex];    // add reading to tal
  OxReadIndex = OxReadIndex + 1;                // ready to write next reading
  if (OxReadIndex >= numReadings){              // resets after 10 readings to start again
    OxReadIndex = 0;
  }
  OxAverage = OxTotal / numReadings;            // calculate average
  return OxAverage;                             // returns average
}

