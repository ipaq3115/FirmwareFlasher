
// code related to the PAR/color sensor

#include <Arduino.h>
#include "src/TCS3471.h"              // color sensor
#include "serial.h"
#include "defines.h"
#include "eeprom.h"

// external function declarations
void i2cWrite(byte address, byte count, byte* buffer);
void i2cRead(byte address, byte count, byte* buffer);

// global variables 
extern float light_intensity;
extern float light_intensity_averaged;
extern float light_intensity_raw;
extern float light_intensity_raw_averaged;
extern float r;
extern float r_averaged;
extern float g;
extern float g_averaged;
extern float b;
extern float b_averaged;

static TCS3471 *par_sensor=0;


// initialize the PAR/color sensor

void PAR_init()
{
  // color sensor init

  if (par_sensor == 0)
     par_sensor = new TCS3471(i2cWrite, i2cRead);

  par_sensor->setWaitTime(200.0);
  par_sensor->setIntegrationTime(700.0);
  par_sensor->setGain(TCS3471_GAIN_1X);
  par_sensor->enable();

}  // PAR_init()

void PAR_init_constant()
{
  // color sensor init

  if (par_sensor == 0)
     par_sensor = new TCS3471(i2cWrite, i2cRead);

   par_sensor->enableInterrupt();
   // range is from 2.4ms up to 614.4ms, parameter is float and in milliseconds,
   par_sensor->setIntegrationTime(600);
   // range is from 2.4ms up to 7400ms, from 2.4ms up to 614.4ms step is 2.4ms, from 614.4ms up step is  28.8ms
   // if set to anything less than 2.4ms, wait time is disabled
   par_sensor->setWaitTime(10.0);
   // naked chip under regular ambient lighting works ok with 1x gain
   par_sensor->setGain(TCS3471_GAIN_1X);
   // if C(lear) channel goes above this value, interrupt will be generated
   // range is from 0-65535, 16 full bits
   par_sensor->interruptHighThreshold(32768);
   // range is from 0-65535
   // this will ensure that interrupt is generated all the time
   par_sensor->interruptLowThreshold(32767);
   par_sensor->interruptPersistence(2);
   par_sensor->clearInterrupt();
   par_sensor->enable();
  
  analogWriteResolution(12);

}  // PAR_init_constant()


uint16_t par_to_dac (float _par, uint16_t _pin) {                                             // convert dac value to par, in form y = mx2+ rx + b where y is the dac value  
//  int dac_value = _par * _par * eeprom->par_to_dac_slope1[_pin] + _par * eeprom->par_to_dac_slope2[_pin] + eeprom->par_to_dac_yint[_pin];     
  double a = _par * _par * _par * _par * eeprom->par_to_dac_slope1[_pin]/1000000000; 
  double b = _par * _par * _par * eeprom->par_to_dac_slope2[_pin]/1000000000; 
  double c = _par * _par * eeprom->par_to_dac_slope3[_pin]/1000000000; 
  double d = _par * eeprom->par_to_dac_slope4[_pin]; 
  double e = eeprom->par_to_dac_yint[_pin]; 
  int dac_value = a + b + c + d + e;   
  if (_par == 0) {                                                                           // regardless of the calibration, force a PAR of zero to lights off
    dac_value = 0;
  } 
  dac_value = constrain(dac_value,0,4095);
/*
  Serial_Print_Line("");
  Serial_Print_Line(eeprom->par_to_dac_slope1[_pin]/1000000000,15);
  Serial_Print_Line(eeprom->par_to_dac_slope2[_pin]/1000000000,11);
  Serial_Print_Line(eeprom->par_to_dac_slope3[_pin]/1000000000,9);
  Serial_Print_Line(eeprom->par_to_dac_slope4[_pin],7);
  Serial_Print_Line(eeprom->par_to_dac_yint[_pin],4);

  Serial_Print_Line("");
  Serial_Print_Line(a,15);
  Serial_Print_Line(b,11);
  Serial_Print_Line(c,9);
  Serial_Print_Line(d,7);
  Serial_Print_Line(e,4);
*/
  return dac_value;
}

float light_intensity_raw_to_par (float _light_intensity_raw, float _r, float _g, float _b) {
  int par_value = eeprom->light_slope_all * _light_intensity_raw + _r * eeprom->light_slope_r + _g * eeprom->light_slope_g + _b * eeprom->light_slope_b + eeprom->light_yint;
 
  if (par_value < 0)                                                                             // there may be cases when the output could be less than zero.  In those cases, set to zero and mark a flag 
    par_value = 0; 

/*
  Serial_Print_Line("light_intenisty_raw_to_par");
  Serial_Print_Line(par_value);
  Serial_Print_Line(_light_intensity_raw);
  Serial_Print_Line(_r);
  Serial_Print_Line(_g);
  Serial_Print_Line(_b);
  Serial_Print_Line("saved values");
  Serial_Print_Line(eeprom->light_slope_all,6);
  Serial_Print_Line(eeprom->light_slope_r,6);
  Serial_Print_Line(eeprom->light_slope_g,6);
  Serial_Print_Line(eeprom->light_slope_b,6);
  Serial_Print_Line(eeprom->light_yint,6);
  */
  return par_value;
}

int get_light_intensity(int _averages) {

  r = par_sensor->readRData();
  g = par_sensor->readGData();
  b = par_sensor->readBData();
  light_intensity_raw = par_sensor->readCData();
  light_intensity = light_intensity_raw_to_par(light_intensity_raw, r, g, b);

  r_averaged += r / _averages;
  g_averaged += g / _averages;
  b_averaged += b / _averages;
/*
  Serial_Print_Line("get_light_intensity");
  Serial_Print_Line(light_intensity_raw,6);
  Serial_Print_Line(light_intensity,6);
  Serial_Print_Line(r_averaged,6);
  Serial_Print_Line(g_averaged,6);
  Serial_Print_Line(b_averaged,6);
*/

  light_intensity_raw_averaged += light_intensity_raw / _averages;
  light_intensity_averaged += light_intensity / _averages;
  return light_intensity;
} // get_light_intensity()


volatile bool colorAvailable = true;

void TCS3471Int()
{
  colorAvailable = true;
}

void constant_light () {
  int printXtimes = 0;
  int clearVal = 0;
  int redVal = 0;
  int greenVal = 0;
  int blueVal = 0;
  //int plus_minus_percent = 5;     //Set tolerance in percent
  float uE_per_count = 0.70;   //Enter calibration factor for color sensor clear here Counts/uE
  int par_set = 100;    //Set Point Value in uE/(m^2s)

  // LED out_index is approx 110,  Use equation output=m*out_index+b, b is offset, m is LED(uE)/Step  Solve in Excel for now
  int out_index_offset = 149;
  float index_uE_slope = .1798;

  int (out_index) = par_set * index_uE_slope + out_index_offset;  //initial value for LED loop, this should speed things up

  ///Note, control loop not stable at low values,  need to check how int(float) is working, also check all limits
  //such as targethigh and target low.

  int targetlow = int((.97 * par_set / uE_per_count) + .5);
  int targethigh = int((1.03 * par_set / uE_per_count) + .5);

  int par_value = 0;

  attachInterrupt(0,TCS3471Int,FALLING);
  PAR_init_constant();
  
  Serial_Print_Line(Serial_Peek());
  while (Serial_Input_Long("+", 10) == 0) {
    // int intensity = 110;
    // analogWrite(A14, (int)intensity);

    out_index = out_index + 0;
    delay(2);
    if ((out_index) > 4095)out_index = 0;
    analogWrite(DACT, (int)out_index);
    //Serial.println("Reading...");
    delay(2);
    if (colorAvailable)
    {
      //Serial.println("color available");
      delay(2);
      // check if valid RGBC data is available
      if (par_sensor->rgbcValid())
      {
        //Serial.println("rgbc valid");
        delay(2);
        par_sensor->clearInterrupt();
        colorAvailable = true;
        word clearVal = par_sensor->readCData();
        word redVal   = par_sensor->readRData();
        word greenVal = par_sensor->readGData();
        word blueVal  = par_sensor->readBData();
        delay(2);

        Serial_Print_Line("--------------------");

        Serial_Print_Line("Red | Green | Blue | Clear | out_index | Par Value");
        Serial_Print(redVal);

        Serial_Print(" | ");
        Serial_Print(greenVal);


        Serial_Print("   | ");
        Serial_Print(blueVal);

        Serial_Print("   | ");
        Serial_Print(clearVal);

        Serial_Print("    | ");
        Serial_Print(out_index);

        par_value = int(uE_per_count * clearVal);

        Serial_Print ("      |");
        Serial_Print(par_value);

        if (clearVal < targetlow)
        {
          out_index = out_index + 1;   //removed the ^2 term to ratio, removed + int(targetlow/clearVal) JB
          analogWrite(A14, (int)out_index);
          //Serial.print("+");
          Serial_Print_Line(out_index);
          delay(10);
        }

        if (clearVal > targethigh)
        {
          out_index = out_index - 1;  ////removed the ^2 term to ratio, removed + int(targetlow/clearVal) JB
          analogWrite(A14, (int)out_index);
          //Serial.print("-");
          //Serial.println(out_index);
          delay(10);
        }
        if (clearVal > targetlow && clearVal < targethigh )
        {
          Serial_Print("  | STABILIZED");
        }
        else
        {
          Serial_Print("  | NOT STABILIZED");
          Serial_Print(" Target High:");
          Serial_Print(targethigh);
          Serial_Print("   Target Low:");
          Serial_Print(targetlow);
        }

        printXtimes++;
      }

    }
    else                   //JB Add to confirm rgbc
    {
      Serial_Print_Line("rgbc not valid");
    }
  }
}


