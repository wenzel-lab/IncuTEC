/*
    basic_demo.ino
    Example for MCP9600

    Copyright (c) 2018 Seeed Technology Co., Ltd.
    Website    : www.seeed.cc
    Author     : downey
    Create Time: May 2018
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

#include "Seeed_SHT35.h"
#include <SeeedOLED.h>
#include <Wire.h>


/*SAMD core*/
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
    #define SDAPIN  20
    #define SCLPIN  21
    #define RSTPIN  7
    #define SERIAL SerialUSB
#else
    #define SDAPIN  A4
    #define SCLPIN  A5
    #define RSTPIN  2
    #define SERIAL Serial
#endif

SHT35 SHTsensor(SCLPIN);


//H2 SetUp
const float R0 = 5.55;           // resting voltage of H2 Sensors; Determine by using "H2 setup"
const int H2Pin = A2;           // PIN used for the H2 sensor
float H2ratio = 0;
const float H2soll = 0.15;      // minimum amount before automatic feeding of H2 containing gas
const int GasFlowSchleuseDuration = 300;                      // How long should the air lock be gased (s)

//Display setUP
char line0[17];                 // first line in Oled display: Temp
char line1[17];                 // second line in Oled display: rH
char line2[17];                 // third line in Oled display: Oxygen%
char line3[17];                 // fourth line in Oled display: H2 indicator
char line4[17];                 // fifth line in Oled display: PAR
char line5[17];                 // sixth line in Oled display: Status of controller
char line6[17];                 // seventh line in Oled display: (SD) errorcodes
byte TimeLine = 7;              // eight line in Oled display: datu
boolean timeDisplayIni = false;

//O2 SetUP
const float VRefer = 3.3;       // voltage of connected to the O2 sensor
const int pinAdc   = A0;        // PIN used for the O2 sensor
float OxVout =0;
const int numReadings = 10;     // Average of number of readings
float readings[numReadings];    // single sensor readings
int OxReadIndex = 0;            // Index of current reading
float OxTotal = 0;              // running total
float OxAverage = 0;            // average of readings so far
float OxValue = 0;              // Oxygen in %
int GasFlowChamberTimer = 0;


void setup() {
    Wire.begin();
    SERIAL.begin(115200);
    delay(10);
    SERIAL.println("serial start!!");
    //if (SHTsensor.init()) {
    //    SERIAL.println("sensor init failed!!");
    //}
    SeeedOled.init();                                                       // Initialze SEEED OLED display
    SeeedOled.setNormalDisplay();                                           // Set display to normal mode (i.e non-inverse mode)
    SeeedOled.setPageMode();                                                // Set addressing mode to Page Mode  
    SeeedOled.clearDisplay(); 
    delay(1000);
}


void loop() {

  float temp, hum;
  SHTsensor.read_meas_data_single_shot(HIGH_REP_WITH_STRCH, &temp, &hum);   // Get temperature and humidity

//Temperature
  String tDisplay = F("Temp ");
  if(temp <-10 || temp >80){
    tDisplay = tDisplay + F("RANGE ERROR");
    }
    else{
      char tBuff[6]; 
      dtostrf(temp, 5, 2, tBuff);                         
      tDisplay = tDisplay + tBuff + " *C";
      }

  for(int i=0;i<17;i++){
    line0[i]=tDisplay[i];
    }
   
//Humidity 
  String rhDisplay = F("Hum");            
  if(hum < 0){                                                // If hum is close to 0 or 100% displays error
    rhDisplay = rhDisplay + F("  RANGE LOW");
    }
    else if(hum >= 100){
      rhDisplay = rhDisplay + F("  RANGE HIGH");
      }
      else{
        char rhBuff[6];                                                      
      dtostrf(hum, 5, 2, rhBuff);                             // Creates a string lenght 5, decimals 2 and name rhBuff
      rhDisplay = rhDisplay +"  "+ rhBuff + F(" %");     
      }

  for(int i=0;i<17;i++){
    line1[i]=rhDisplay[i];                                    // Humidity string
    }

//Oxygen
  float OxValue = readConcentration();
  String OxDisplay= F("OxLvl");
  if (OxValue < 10){
     OxDisplay = OxDisplay + "  ";
     }
     else{
     OxDisplay = OxDisplay + ' ';
     }

  char OxBuff[6];                                              
  dtostrf(OxValue, 5, 2, OxBuff);                             // Creates a string lenghte 5, decimals 2 and name OxBuff      
  OxDisplay = OxDisplay + OxBuff + " %";
  for(int i=0;i<17;i++){
    line2[i]=OxDisplay[i];                                    // Oxygen concentration string
    }


//H2
  float H2sensor_volt;
  float RS_gas;                                                   
  H2ratio;                                                        
  int H2sensorValue = analogRead(H2Pin);                      // Get voltage value from Gas sensor
  H2sensor_volt=(float)H2sensorValue/1024*5.0;
  if (H2sensor_volt > 0){
    RS_gas = (5.0-H2sensor_volt)/H2sensor_volt;               // Omit *RL
    H2ratio = RS_gas/R0;                                      // Ratio = RS/R0
    }

  String H2Display = F("H2: ");
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

  char H2Buff[6];                                                                  
  dtostrf(H2ratio, 5, 2, H2Buff);                             // Creates a string lenght 5, decimals 2 and name H2Buff                                                                                                
  H2Display = H2Display +  H2Buff;                                
  for(int i=0;i<17;i++){
    line3[i]=H2Display[i];                                    // Hydrogen ratio string
    }

  //Display text in OLED
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

 
  delay(500);
}

float readO2Vout(){                                           // Rutines for O2 measurment [4]

  long sum = 0;
  for(int i=0; i<32; i++)
  {
    sum += analogRead(pinAdc);
    }

  sum >>= 5;

  float MeasuredVout = sum * (5.0 / 1023.0);
  return MeasuredVout;
  }

float readConcentration(){

    // Vout samples are with reference to 3.3V
  float MeasuredVout = readO2Vout();

    //float Concentration = FmultiMap(MeasuredVout, VoutArray,O2ConArray, 6);
    //when its output voltage is 2.0V,
  float Concentration = MeasuredVout * 0.2 / 1.452;           // 1.452 V is the reference voltage for a concentration of 20%.
  if(Concentration < 0){
    Concentration = 0;
    }
  OxTotal = OxTotal - readings[OxReadIndex];                  // Substract last reading
  readings[OxReadIndex] = Concentration*100;                  // Read from sensor
  OxTotal = OxTotal + readings[OxReadIndex];                  // Add reading total
  OxReadIndex = OxReadIndex + 1;                              // Ready to write next reading
  if (OxReadIndex >= numReadings){                            // Resets after 10 readings to start again
    OxReadIndex = 0;
    }
  OxAverage = OxTotal / numReadings;                          // Calculate average
  return OxAverage;                                           // Returns average
  }




