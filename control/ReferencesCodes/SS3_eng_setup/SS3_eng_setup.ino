
/*
    SS3_eng_setup.ino
    Setup for clock and H2 sensors for an anaerobic box
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

#include <Wire.h>
#include "DS1307.h"              // lib für REal Clock modul
DS1307 clock;                    //define a object of DS1307 class
const int H2Pin = A2;            // PIN used for the H2 sensor
const float VRefer = 3.3;       // voltage of connected to the O2 sensor
const int pinAdc   = A0;        // PIN used for the O2 sensor
const int LuxPin = A1;            //PIN used for the PAR sensor
float VoutArray[] =  { 0.0011498,  0.0033908,   0.011498, 0.041803,0.15199,     0.53367, 1.3689,   1.9068,  2.3};       //array for light measurment   [2]
float  LuxArray[] =  { 1.0108,     3.1201,  9.8051,   27.43,   69.545,   232.67,  645.11,   73.52,  1000};

void setup() {                
    Serial.begin(9600);
        clock.begin();                         // fill in the correct date and time to initialize clock  [3]
    clock.fillByYMD(2013,1,19);               //Jan 19,2013
    clock.fillByHMS(15,28,30);                //15:28 30"
    clock.fillDayOfWeek(SAT);                 //Saturday, arguments are [MON, TUE, WED, THU, FRI, SAT, SUN]
    clock.setTime();
                              //write time to the RTC chip

}
 
void loop() {
                                                              //displays R0 needed in the mainprogramm for H2 measurment
    float sensor_volt;                      //[1]
    float RS_air;                           //  Get the value of RS via in a clear air
    float R0;                              // Get the value of R0 via in H2
    float sensorValue;

    sensorValue = 0;
 
        /*--- Get a average data by testing 100 times ---*/
    for(int x = 0 ; x < 100 ; x++)
    {
        sensorValue = sensorValue + analogRead(H2Pin);
    }
    sensorValue = sensorValue/100.0;
        /*-----------------------------------------------*/
 
    sensor_volt = sensorValue/1024*5.0;
    RS_air = (5.0-sensor_volt)/sensor_volt; // omit *RL
    R0 = RS_air/9.8;                         //The ratio of RS/R0 is 6.5 (MQ5) or 9.8 (MQ2) in a clear air from Graph (Found using WebPlotDigitizer)
    //R0 = 4.25
 
    Serial.print("sensor_volt = ");
    Serial.print(sensor_volt);
    Serial.println("V");

    Serial.print("sensor_value = ");
    Serial.print(sensorValue);
    Serial.println("V");
 
 
    Serial.print("R0 = ");
    Serial.println(R0);
    delay(1000);

                                                            // Displays the voltage and lux of the light sensor to callibrate the approx PAR
    Serial.print("Vout =");
    Serial.print(readAPDS9002Vout(LuxPin));
    Serial.print(" V,Luminance =");
    Serial.print(readLuminance(LuxPin));
    Serial.println("Lux");
    delay(500);
                                                          // test the O2 sensor
                                                              float Vout =0;

    Serial.print("Vout O2 sensor =");                     //[4]
    Vout = readO2Vout();
    Serial.print(Vout);
    Serial.print(" V, Concentration of O2 is ");
    Serial.println(readConcentration());
                                                          
}
                                                            

                                                            
float readAPDS9002Vout(uint8_t analogpin)                   // [2] rutines for the lux measurment
{
    // MeasuredVout = ADC Value * (Vcc / 1023) * (3 / Vcc)
    // Vout samples are with reference to 3V Vcc
    // The above expression is simplified by cancelling out Vcc
    float MeasuredVout = analogRead(A0) * (3.0 / 1023.0);
    //Above 2.3V , the sensor value is saturated
 
    return MeasuredVout;
 
}
 
float readLuminance(uint8_t analogpin)
{
 
    // MeasuredVout = ADC Value * (Vcc / 1023) * (3 / Vcc)
    // Vout samples are with reference to 3V Vcc
    // The above expression is simplified by cancelling out Vcc
    float MeasuredVout = analogRead(A0) * (3.0 / 1023.0);
    float Luminance = FmultiMap(MeasuredVout, VoutArray, LuxArray, 9);
 
    /**************************************************************************
 
    The Luminance in Lux is calculated based on APDS9002 datasheet -- > Graph 1
    ( Output voltage vs. luminance at different load resistor)
    The load resistor is 1k in this board. Vout is referenced to 3V Vcc.
 
    The data from the graph is extracted using WebPlotDigitizer
    http://arohatgi.info/WebPlotDigitizer/app/
 
    VoutArray[] and LuxArray[] are these extracted data. Using MultiMap, the data
    is interpolated to get the Luminance in Lux.
 
    This implementation uses floating point arithmetic and hence will consume
    more flash, RAM and time.
 
    The Luminance in Lux is an approximation and depends on the accuracy of
    Graph 1 used.
 
    ***************************************************************************/
 
    return Luminance;
}
 
 
//This code uses MultiMap implementation from http://playground.arduino.cc/Main/MultiMap
 
float FmultiMap(float val, float * _in, float * _out, uint8_t size)
{
    // take care the value is within range
    // val = constrain(val, _in[0], _in[size-1]);
    if (val <= _in[0]) return _out[0];
    if (val >= _in[size-1]) return _out[size-1];
 
    // search right interval
    uint8_t pos = 1;  // _in[0] allready tested
    while(val > _in[pos]) pos++;
 
    // this will handle all exact "points" in the _in array
    if (val == _in[pos]) return _out[pos];
 
    // interpolate in the right segment for the rest
    return (val - _in[pos-1]) * (_out[pos] - _out[pos-1]) / (_in[pos] - _in[pos-1]) + _out[pos-1];
}

                                                            // rutines for O2 determination
float readO2Vout()
{
    long sum = 0;
    for(int i=0; i<32; i++)
    {
        sum += analogRead(pinAdc);
    }
 
    sum >>= 5;
 
    float MeasuredVout = sum * (VRefer / 1023.0);
    return MeasuredVout;
}
 
float readConcentration()
{
    // Vout samples are with reference to 3.3V
    float MeasuredVout = readO2Vout();
 
    //float Concentration = FmultiMap(MeasuredVout, VoutArray,O2ConArray, 6);
    //when its output voltage is 2.0V,
    float Concentration = MeasuredVout * 0.21 / 2.0;
    float Concentration_Percentage=Concentration*100;
    return Concentration_Percentage;
}
