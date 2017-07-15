
// reasonably generic utility functions
// put function prototypes in util.h

#include "defines.h"
#include "eeprom.h"
#include "src/crc32.h"
#include "DAC.h"
#include "util.h"
#include "serial.h"
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIX_PIN, NEO_GRB + NEO_KHZ800); 



int get_light_intensity(int x);
uint16_t par_to_dac (float _par, uint16_t _pin);

//apply the calibration values for magnetometer from EEPROM
void applyMagCal(float * arr) {

  arr[0] -= eeprom->mag_bias[0];
  arr[1] -= eeprom->mag_bias[1];
  arr[2] -= eeprom->mag_bias[2];


  float tempY = arr[0] * eeprom->mag_cal[0][0] + arr[1] * eeprom->mag_cal[0][1] + arr[2] * eeprom->mag_cal[0][2];
  float tempX = arr[0] * eeprom->mag_cal[1][0] + arr[1] * eeprom->mag_cal[1][1] + arr[2] * eeprom->mag_cal[1][2];
  float tempZ = arr[0] * eeprom->mag_cal[2][0] + arr[1] * eeprom->mag_cal[2][1] + arr[2] * eeprom->mag_cal[2][2];

  arr[0] = tempX;
  arr[1] = tempY;
  arr[2] = tempZ;

  //arr[1] *= -1;
  arr[0] *= -1;
  arr[2] *= -1;
}

void applyAccCal(int * arr) {
  //arr[1] *= -1;
  arr[2] *= -1;
}

void rad_to_deg(float* roll, float* pitch, float* compass) {
  *roll *= 180 / PI;
  *pitch *= 180 / PI;
  *compass *= 180 / PI;
}

//return compass heading (RADIANS) given pitch, roll and magentotmeter measurements
float getCompass(const float magX, const float magY, const float magZ, const float & pitch, const float & roll) {

  float negBfy = magZ * sine_internal(roll) - magY * cosine_internal(roll);
  float Bfx = magX * cosine_internal(pitch) + magY * sine_internal(roll) * sine_internal(pitch) + magZ * sine_internal(pitch) * cosine_internal(roll);
  float compass = atan2(negBfy, Bfx);

  compass += PI / 2;

  if (compass < 0) {
    compass += 2 * PI;
  }
  if (compass > 2 * PI) {
    compass -= 2 * PI;
  }


  return compass;
}

//return roll (RADIANS) from accelerometer measurements
float getRoll(const int accelY, const int accelZ) {
  return atan2(accelY, accelZ);
}

//return pitch (RADIANS) from accelerometer measurements + roll
float getPitch(const int accelX, const int accelY, const int accelZ, const float & roll) {

  return atan(-1 * accelX / (accelY * sine_internal(roll) + accelZ * cosine_internal(roll)));

}

// return 0-7 based on 8 segments of the compass

int compass_segment(float angle)    // in degrees, assume no negatives
{
  return (round( angle / 45) % 8);
}

//get the direction (N/S/E/W/NW/NE/SW/SE) from the compass heading
const char * getDirection(int segment) {

  if (segment > 7 || segment < 0) {
    return "\"Invalid compass segment\"";
  }

  const char *names[] = {"\"N\"", "\"NE\"", "\"E\"", "\"SE\"", "\"S\"", "\"SW\"", "\"W\"", "\"NW\""};

  return names[segment];
}

//calculate tilt angle and tilt direction given roll, pitch, compass heading
Tilt calculateTilt(float roll, float pitch, float compass) {

  Tilt deviceTilt;

  //equation derived from rotation matricies in AN4248 by Freescale
  float a = (cosine_internal(roll) * cosine_internal(pitch));
  float b = sqrt((sine_internal(roll) * sine_internal(roll) + (sine_internal(pitch) * sine_internal(pitch) * cosine_internal(roll) * cosine_internal(roll))));
  deviceTilt.angle = atan2(a, b);

  deviceTilt.angle *= 180 / PI;

  deviceTilt.angle  = 90 - deviceTilt.angle;

  compass *= 180 / PI;

  if (0 >= compass || compass >= 360) {
    deviceTilt.angle_direction = "\"Invalid compass heading\"";
  }

  float tilt_angle = atan2(-1 * sine_internal(roll), cosine_internal(roll) * sine_internal(pitch));

  tilt_angle *= 180 / PI;

  if (tilt_angle < 0) {
    tilt_angle += 360;
  }


  int tilt_segment = compass_segment(tilt_angle);

  int comp_segment = compass_segment(compass) + tilt_segment + 2;
  comp_segment = comp_segment % 8;

  deviceTilt.angle_direction = getDirection(comp_segment);

  return deviceTilt;

}

//Internal sine calculation in RADIANS
float sine_internal(float angle) {
  if (angle > PI) {
    angle = 2 * PI - angle;
  }

  if ( angle < -1 * PI) {
    angle = 2 * PI + angle;
  }

  return angle - angle * angle * angle / 6 +
         angle * angle * angle * angle * angle / 120 -
         angle * angle * angle * angle * angle * angle * angle / 5040 +
         angle * angle * angle * angle * angle * angle * angle * angle * angle / 362880;
}

//Internal cosine calculation in RADIANS
float cosine_internal(float angle) {
  if (angle > PI) {
    angle = 2 * PI - angle;
  }

  if ( angle < -1 * PI) {
    angle = 2 * PI + angle;
  }

  return 1 - angle * angle / 2 + angle * angle * angle * angle / 24 -
         angle * angle * angle * angle * angle * angle  / 720 +
         angle * angle * angle * angle * angle * angle * angle * angle / 40320 -
         angle * angle * angle * angle * angle * angle * angle * angle * angle * angle / 3628800;
}
//this arctan approximation only works for -pi/4 to pi/4 - can be modified for that to work, but atan2 and atan
//only takes up <2kb, if we need the space I'll fix it but otherwise I'll leave the originals in place
/*
  //Internal arctangent calculation in RADIANS
  float arctan_internal(float x, float y){
  float small, large;
  int sign = 1;
  if((x < 0 && y > 0) || (x > 0 && y < 0)){
    sign *= -1;
    (x < 0) ? x *= -1 : y *= -1;
  }

  large = float_max(x, y);
  small = float_min(x, y);

  float angle = small / large;

  return PI / 4 * angle - angle * (angle - 1) * (0.2447 + 0.0663 * angle);
  }

  float float_max(float x, float y){
  return (x > y) ? x : y;
  }

  float float_min(float x, float y){
  return (x < y) ? x : y;
  }
*/


float measure_hall() {
  float hall_value = (analogRead(HALL_OUT) + analogRead(HALL_OUT) + analogRead(HALL_OUT)) / 3;
  //  Serial_Printf("final hall_value: %f",hall_value);
  return hall_value;
}

void start_on_pin_high(int pin) {
  pinMode(pin, INPUT);
  uint16_t go = 0;
  while (go == 0) {
    go = digitalRead(pin);
    delayMicroseconds(100);
    //    Serial_Print_Line(go);
  }
}

//The following is the original start_on_open_close. DMK modified to make separate functions for start_on_open
//and start_on_close

//void start_on_open_close() {
//  // take an initial measurement as a baseline (closed position)
//  for (uint16_t i = 0; i < 10; i++) {                            // throw away values to make sure the first value is correct
//    measure_hall();
//  }
//  float start_position = measure_hall();
//  float current_position = start_position;
//  float clamp_closed = eeprom->thickness_min;
//  float clamp_open = eeprom->thickness_max;
//  int isFlipped = 1;                                    // account for the direction of the magnet installed in the device
//  if (eeprom->thickness_min > eeprom->thickness_max) {
//    //    Serial.print("white device: thickness_min > thickness_max so clamp_closed = thickness_max, and clamp_open = thickness_min");
//    clamp_closed = eeprom->thickness_max;
//    clamp_open = eeprom->thickness_min;
//    isFlipped = 0;
//  }
//  else {
//    //    Serial.print("black device: thickness_min < thickness_max so clamp_closed = thickness_min, and clamp_open = thickness_max");
//  }
//  int start_time = millis();
//
//
//
//
//  if (isFlipped == 0) { // this is the white device
//    // now measure every 150ms until you see the value change to 65% of the full range from closed to fully open
//    while (start_position - current_position < .75 * (clamp_open - clamp_closed)) {
//      current_position = measure_hall();
//      /*
//        Serial.printf("1st isflipped = 0 75% to exit, %f < %f\n", start_position - current_position, .75 * (clamp_open - clamp_closed));
//        Serial.printf("1st isflipped = 0 75% to exit, current: %f, start: %f, closed: %f, open: %f, thickness_min: %f, thickness_max: %f\n", current_position, start_position, clamp_closed, clamp_open, eeprom->thickness_min, eeprom->thickness_max);
//      */
//      delay(150);                                                               // measure every 100ms
//      if (start_position - current_position < -.20 * (clamp_open - clamp_closed) || current_position == 0 || current_position == 65535 || millis() - start_time > 30000) {       // if the person opened it first (ie they did it wrong and started it with clamp open) - detect and skip to end.  Also if the output is maxed or minned then proceed.
//        /*
//          Serial.printf("if statement to exit, %f < %f\n", start_position - current_position, -.20 * (clamp_open - clamp_closed));
//          Serial.print("exit 1\n");
//        */
//        goto end;
//      }
//    }
//
//    // now measure again every 150ms until you see the value change to < 65% of the full range from closed to fully open
//    while (start_position - current_position > .75 * (clamp_open - clamp_closed)) {
//      current_position = measure_hall();
//      /*
//        Serial.printf("2st isflipped = 0 75% to exit, %f > %f\n", start_position - current_position, .75 * (clamp_open - clamp_closed));
//        Serial.printf("2st isflipped = 0 75% to exit, current: %f, start: %f, closed: %f, open: %f, thickness_min: %f, thickness_max: %f\n", current_position, start_position, clamp_closed, clamp_open, eeprom->thickness_min, eeprom->thickness_max);
//      */
//      if (millis() - start_time > 30000) { // if it's been more than 30 seconds,then bail
//        goto end;
//      }
//      delay(150);                                                               // measure every 150ms
//    }
//  }
//
//  else if (isFlipped == 1) { // this is the black device.  Same as white device but subtract current_position from start_position instead of other way around
//    // now measure every 150ms until you see the value change to 75% of the full range from closed to fully open
//    while (current_position - start_position < .75 * (clamp_open - clamp_closed)) {
//      current_position = measure_hall();
//      /*
//        Serial.printf("1st isflipped = 1 75% to exit, %f < %f\n", current_position - start_position, .75 * (clamp_open - clamp_closed));
//        Serial.printf("1st isflipped = 1 75% to exit, current: %f, start: %f, closed: %f, open: %f, thickness_min: %f, thickness_max: %f\n", current_position, start_position, clamp_closed, clamp_open, eeprom->thickness_min, eeprom->thickness_max);
//      */
//      delay(150);                                                               // measure every 100ms
//      if (current_position - start_position < -.20 * (clamp_open - clamp_closed) || current_position == 0 || current_position == 65535 || millis() - start_time > 30000) {       // if the person opened it first (ie they did it wrong and started it with clamp open) - detect and skip to end.  Also if the output is maxed or minned then proceed.
//        /*
//          Serial.printf("if statement to exit, %f < %f\n", current_position - start_position, -.20 * (clamp_open - clamp_closed));
//          Serial.print("exit 1\n");
//        */
//        goto end;
//      }
//    }
//
//    // now measure again every 150ms until you see the value change to < 75% of the full range from closed to fully open
//    while (current_position - start_position > .75 * (clamp_open - clamp_closed)) {
//      current_position = measure_hall();
//      /*
//        Serial.printf("2st isflipped = 1 75% to exit, %f > %f\n", current_position - start_position, .75 * (clamp_open - clamp_closed));
//        Serial.printf("2st isflipped = 1 75% to exit, current: %f, start: %f, closed: %f, open: %f, thickness_min: %f, thickness_max: %f\n", current_position, start_position, clamp_closed, clamp_open, eeprom->thickness_min, eeprom->thickness_max);
//      */
//      delay(150);                                                               // measure every 150ms
//    }
//end:
//    delay(300);                                                               // make sure the clamp has time to settle onto the leaf.
//    //  Serial.print("exit 2\n");
//  }
//}
//

void setNeoPixel(){
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setPixelColor(0, r_v, g_v, b_v);
  pixels.show(); // This sends the updated pixel color to the hardware.  
}


float check_thickness(void) { //DMK added this version of thickness guage for the following seris of 
                              //functions for controlling the start of protocol segments par_led_start_on_open, par_led_start_on_close, start_on_open, start_on_open
    int sum = 0;
    for (int i = 0; i < 1000; ++i) {
      sum += analogRead(HALL_OUT);
     }
    thickness_raw = (sum / 1000);
    // dividing thickness_a by 1,000,000,000 to bring it back to it's actual value
    thickness = (eeprom->thickness_a * thickness_raw * thickness_raw / 1000000000 + eeprom->thickness_b * thickness_raw + eeprom->thickness_c) / 1000; // calibration information is saved in uM, so divide by 1000 to convert back to mm.
    return thickness_raw;
}


//DMK added the following function to start on close. 


        //float par = get_light_intensity(1);
        //Serial_Print_Line("\"message\": \"Enter led # setting followed by +: \"}");
        //int led =  Serial_Input_Double("+", 0);
        //Serial_Print_Line("\"message\": \"Enter dac setting followed by +:  \"}");
        //int setting =  Serial_Input_Double("+", 0);
        //int setting=par_to_dac(par, led);
        //Serial_Print_Line(setting);
        //DAC_set(led, setting);
        //DAC_change();
        //digitalWriteFast(LED_to_pin[led], HIGH);
        //delay(5000);



//The following are new versions of the start on open/close functions
//All of these functions are now passed max_hold_time, the time in milliseconds
//the time out time when the wait loop will exit automatically if nothing happens.

//The par_led_start_on_open and par_led_start_on_close functions also adjust the 
//actinic led light to match the incoming PAR. These functions are also 
//passed the led number to set the actinic light

void par_led_start_on_open(int led, int max_hold_time){  
    //led=2;
    turn_on_5V();     
    float par=0;
    int setting=0;
    float thickness=0;
    int max_numer_of_loops=0;
    //In DMK's version of these functions, the maximum time allowed is set by 
    //the delay_time * the max_numer_of_loops
    
    int delay_time = 20; 
    max_numer_of_loops = max_hold_time/delay_time;
    
    for (uint16_t i = 0; i < max_numer_of_loops; i++) { 
        if (use_previous_light_intensity ==1) {
          par=previous_light_intensity;
          //Serial_Printf(" using previous light intensity of: %.2f", par);
        }
        else {
          
        float temp_light_intensity=light_intensity; //store these values to reassert after function, so not to interfere with the "light_intensity" environmental paramter, if measured separately
        float temp_light_intensity_averaged=light_intensity_averaged;
        float temp_r_averaged=r_averaged;
        float temp_g_averaged=g_averaged;
        float temp_b_averaged=b_averaged;
        float temp_light_intensity_raw_averaged=light_intensity_raw_averaged;

        light_intensity=0; //reset these values so not to interfere with the "light_intensity" environmental paramter, if measured separately
        light_intensity_averaged=0;
        r_averaged=0;
        g_averaged=0;
        b_averaged=0;
        light_intensity_raw_averaged=0;
          
        par = get_light_intensity(1);
        
        previous_light_intensity_averaged=light_intensity_averaged;
        previous_light_intensity=light_intensity;
        previous_r_averaged=r_averaged;
        previous_g_averaged=g_averaged;
        previous_b_averaged=b_averaged;
        previous_light_intensity_raw_averaged=light_intensity_raw_averaged;
        
        light_intensity=temp_light_intensity; //reset these values so not to interfere with the "light_intensity" environmental paramter, if measured separately
        light_intensity_averaged=temp_light_intensity_averaged;
        r_averaged=temp_r_averaged;
        g_averaged=temp_g_averaged;
        b_averaged=temp_b_averaged;
        light_intensity_raw_averaged=temp_light_intensity_raw_averaged;
        
        }
        
        setting=par_to_dac(par, led);
        //Serial_Printf("led=%d light=%f set=%d", led, par, setting);
        DAC_set(led, setting);
        DAC_change();
        digitalWriteFast(LED_to_pin[led], HIGH);
        delay(delay_time);
        thickness=check_thickness();
        //Serial_Printf("\"t\":%.2f,", thickness);
        if (eeprom->mag_orientation*(thickness - eeprom->open_thickness) >0){
        //if (thickness < eeprom->open_thickness){ 
           //Serial_Printf("\"reached threshold\":%.2f,", thickness);
           break;
     }

    }
}


void par_led_start_on_close(int led, int max_hold_time) {
    turn_on_5V();     
    float par=0;
    int setting=0;
    float thickness=0;
    int delay_time = 10; 
    int max_numer_of_loops = 0;
    max_numer_of_loops=max_hold_time/delay_time;

    for (uint16_t i = 0; i < max_numer_of_loops; i++) { 
      if (use_previous_light_intensity ==1) {
          par=previous_light_intensity;
          //Serial_Printf(" using previous light intensity of: %.2f", par);
        }
        else {
        float temp_light_intensity=light_intensity; //store these values to reassert after function, so not to interfere with the "light_intensity" environmental paramter, if measured separately
        float temp_light_intensity_averaged=light_intensity_averaged;
        float temp_r_averaged=r_averaged;
        float temp_g_averaged=g_averaged;
        float temp_b_averaged=b_averaged;
        float temp_light_intensity_raw_averaged=light_intensity_raw_averaged;

        light_intensity=0; //reset these values so not to interfere with the "light_intensity" environmental paramter, if measured separately
        light_intensity_averaged=0;
        r_averaged=0;
        g_averaged=0;
        b_averaged=0;
        light_intensity_raw_averaged=0;
          
        par = get_light_intensity(1);
        
        previous_light_intensity_averaged=light_intensity_averaged;
        previous_light_intensity=light_intensity;
        previous_r_averaged=r_averaged;
        previous_g_averaged=g_averaged;
        previous_b_averaged=b_averaged;
        previous_light_intensity_raw_averaged=light_intensity_raw_averaged;
        
        light_intensity=temp_light_intensity; //reset these values so not to interfere with the "light_intensity" environmental paramter, if measured separately
        light_intensity_averaged=temp_light_intensity_averaged;
        r_averaged=temp_r_averaged;
        g_averaged=temp_g_averaged;
        b_averaged=temp_b_averaged;
        light_intensity_raw_averaged=temp_light_intensity_raw_averaged;
                
        }
        setting=par_to_dac(par, led);
        //Serial_Printf("light=%f set=%d", par, setting);
        DAC_set(led, setting);
        DAC_change();
        digitalWriteFast(LED_to_pin[led], HIGH);
        delay(delay_time);
        thickness=check_thickness();
        //Serial_Printf("\"t\":%.2f,", thickness);
        if (eeprom->mag_orientation*(thickness - eeprom->closed_thickness) < 0){
        //if (thickness > eeprom->closed_thickness){
           //Serial_Printf("\"reached threshold\":%.2f,", thickness);
           break;
        }
     }
}


void start_on_close(int max_hold_time) {
  //Serial_Printf("start on close mhd=%d",max_hold_time );
    turn_on_5V();     
    delay(100);
    float thickness=0;
    int delay_time = 10; 
    int max_numer_of_loops = 0;
    max_numer_of_loops=max_hold_time/delay_time;
    for (uint16_t i = 0; i < max_numer_of_loops; i++) { 
        thickness=check_thickness();
        
        // if (thickness > eeprom->closed_thickness){
        //    break;
        // }
  if (eeprom->mag_orientation*(thickness - eeprom->closed_thickness) < 0){
        //if (thickness > eeprom->closed_thickness){
           //Serial_Printf("\"reached threshold\":%.2f,", thickness);
           break;
        }
        delay(delay_time);
    }
}


void start_on_open(int max_hold_time) {
    //Serial_Printf("start on open mhd=%d",max_hold_time );

    turn_on_5V();    
    delay(100); 
    float thickness=0;
    int delay_time = 10; 
    int max_numer_of_loops = 0;
    max_numer_of_loops=max_hold_time/delay_time;

    for (uint16_t i = 0; i < max_numer_of_loops; i++) { 
        thickness=check_thickness();

        // if (thickness < eeprom->open_thickness){
        //    break;
        // }
        if (eeprom->mag_orientation*(thickness - eeprom->open_thickness) >0){
        //if (thickness < eeprom->open_thickness){ 
           //Serial_Printf("\"reached threshold\":%.2f,", thickness);
           break;
     }

        delay(delay_time);
    }
}


void start_on_open_close(int max_hold_time) {
    
    turn_on_5V();     
    float thickness=0;
    int delay_time = 10; 
    int max_numer_of_loops = 0;
    max_numer_of_loops=max_hold_time/delay_time;
    //max_numer_of_loops=max_numer_of_loops/delay_time;

    for (uint16_t i = 0; i < max_numer_of_loops; i++) { 
        thickness=check_thickness();
        // if (thickness < eeprom->open_thickness){
        //    break;
        // }
        if (eeprom->mag_orientation*(thickness - eeprom->open_thickness) >0){
        //if (thickness < eeprom->open_thickness){ 
           //Serial_Printf("\"reached threshold\":%.2f,", thickness);
           break;
     }
        delay(delay_time);
    }

    for (uint16_t i = 0; i < max_numer_of_loops; i++) { 
        thickness=check_thickness();
      if (eeprom->mag_orientation*(thickness - eeprom->closed_thickness) < 0){
        //if (thickness > eeprom->closed_thickness){
           //Serial_Printf("\"reached threshold\":%.2f,", thickness);
           break;
        }
        // if (thickness > eeprom->closed_thickness){
        //    break;
        // }
        delay(delay_time);
    }
}

unsigned long requestCo2(int timeout) {
  Serial2.begin(9600);
  byte readCO2[] = {
    0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25
  };                             //Command packet to read CO2 (see app note)
  byte response[] = {
    0, 0, 0, 0, 0, 0, 0
  };                                                        //create an array to store CO2 response
  float valMultiplier = 1;
  int error = 0;
  long start1 = millis();
  long end1 = millis();
  while (!Serial2.available()) { //keep sending request until we start to get a response
    Serial2.write(readCO2, 7);
    delay(50);
    end1 = millis();
    if (end1 - start1 > timeout) {
      Serial_Print("\"error\":\"no co2 sensor found\"");
      Serial_Print(",");
      error = 1;
      break;
    }
  }
  switch (error) {
    case 0:
      while (Serial2.available() < 7 ) //Wait to get a 7 byte response
      {
        timeout++;
        if (timeout > 10) {  //if it takes to long there was probably an error
#ifdef DEBUGSIMPLE
          Serial_Print_Line("I timed out!");
#endif
          while (Serial2.available()) //flush whatever we have
            Serial2.read();
          break;                        //exit and try again
        }
        delay(50);
      }
      for (int i = 0; i < 7; i++) {
        response[i] = Serial2.read();
#ifdef DEBUGSIMPLE
        Serial_Print_Line("I got it!");
#endif
      }
      break;
    case 1:
      break;
  }
  int high = response[3];                        //high byte for value is 4th byte in packet in the packet
  int low = response[4];                         //low byte for value is 5th byte in the packet
  unsigned long val = high * 256 + low;              //Combine high byte and low byte with this formula to get value
  Serial2.end();
  return val * valMultiplier;
}


unsigned int set_flow(int setpoint) {  
  //int returned_value=0;
  //Serial_Print("start");
    Wire1.beginTransmission(0x01); //pumop is on address 01
    Wire1.write(2);  // send command 2, program flow
    Wire1.write(highByte(setpoint));  // send two bytes for the flow value 
    Wire1.write(lowByte(setpoint));
    Wire1.endTransmission();
    delay(100);

    int bb=Wire1.requestFrom(0x01, 0);
      for (int i=0; i<bb; i++){
        Wire1.read();
      }
     return(1);
}

void set_default_flow_rate(int default_flow_rate){
  //int returned_value=0;
  //Serial_Print("start");
    Wire1.beginTransmission(0x01); //pumop is on address 01
    Wire1.write(11);  // send command 11, send new default flow rate to EEPROM
    Wire1.write(highByte(default_flow_rate));  // send two bytes for the flow value 
    Wire1.write(lowByte(default_flow_rate));
    Wire1.endTransmission();
    delay(100);

    int bb=Wire1.requestFrom(0x01, 0);
      for (int i=0; i<bb; i++){
        Wire1.read();
      }
}


void reset_flow(void) {
  
    Wire1.beginTransmission(0x01); //pumop is on address 01
    Wire1.write(9);  // send command 2, program flow
    Wire1.write(9);  // send two bytes for the flow value 
    Wire1.write(9);
    Wire1.endTransmission();
    delay(100);

int bb=Wire1.requestFrom(0x01, 0);
  for (int i=0; i<bb; i++){
  Wire1.read();  
  }
  
}


void reset_flow_calibration(void) {
  int returned_value=0;
    Wire1.beginTransmission(0x01); //pumop is on address 01
    Wire1.write(9);  // send command 9 
    Wire1.write(9);  // send two bytes for the flow value 
    Wire1.write(9);
    Wire1.endTransmission();
    delay(100);

int bb=Wire1.requestFrom(0x01, 0);
  for (int i=0; i<bb; i++){
      returned_value = Wire1.read();
  }  
}

//
//int get_flow() {
//
//  char databuf[50];
//  int returned_value;
//  int average_index=0;
//  signed int average_returned_value=0;
//  int number_flow_averages =4;
//  signed int pressure_now=0;
//  int bb=0;
//  
//  for (int i=0; i<number_flow_averages; i++){
//    pressure_now=0;
//    Wire1.resetBus();
//    Wire1.beginTransmission(0x01);    
//    Wire1.write(3);
//    Wire1.write(3);
//    Wire1.write(3);    
//    Wire1.endTransmission();
//    delay(100);    
//    delay(20);
//    
//    bb=Wire1.requestFrom(0x01, 40);
//    if(Wire1.getError()){  //mark as an error
//            Serial.println("I2C ERROR");
//            return(-1); 
//            break;
//    }
//        else
//    {
//    for (int i=0; i<bb; i++){   
//      returned_value = Wire1.read();
//      databuf[i]=returned_value;
//    }
//      pressure_now = databuf[1];              //send x_high to rightmost 8 bits HIGH BYTE!
//      pressure_now = pressure_now<<8;         //shift x_high over to leftmost 8 bits
//      pressure_now |= databuf[2];                 //logical OR keeps x_high intact in combined and fills in                                                             //rightmost 8 bits
//      if (pressure_now>32767){
//        pressure_now=pressure_now-65535;
//      }
//      
//      //pressure_now=256*databuf[1] + databuf[2];
//    
//    // ****}
//    if (i>2) {  //the first two values are bad. 
//    //Serial.println(pressure_now);
//    average_returned_value = average_returned_value + pressure_now;
//    average_index+=1;
//    }
//    delay(1);
//
//}
//      //Serial.print("measured flow rate = ");
//      //Serial.print(0.1*pressure_now);
//      
//      byte number_calibration_points = databuf[3];
//
//      Serial.print("Calibration Set Points    = ");
//      
//      int starting_address=4; //start at the 4th byte of databuf
//      
//      for (int ii=0; ii < number_calibration_points * 2; ii=ii+2){ 
//          int i = starting_address+ii;
//          int tv= databuf[i];
//          tv = tv<<8;   
//          tv |= databuf[i+1]; 
//          Serial.print(tv);
//          Serial.print(", ");
//      }
//      
//      Serial.println(" ");
//      Serial.print("Calibration values Points = ");
//      
//      starting_address=4+(2*number_calibration_points);  // start after the number_calibration_points
//      for (int ii=0; ii < number_calibration_points * 2; ii=ii+2){ 
//          int i = starting_address+ii;
//          int tv= databuf[i];
//          tv = tv<<8;   
//          tv |= databuf[i+1]; 
//          Serial.print(tv);
//          Serial.print(", ");
//      }
//                                                 
//     return(average_returned_value); 
//  }
//}
//


int pump_status(int status_values) {
  int error_occured=0;
  char databuf[50];
  int returned_value;
  float average_index=0;
  signed int accumulated_returned_value=0;
  signed int average_returned_value=0;
  int number_flow_averages =4;
  signed int pressure_now=0;
  int bb=0;
  
  for (int i=0; i<number_flow_averages; i++){
    pressure_now=0;
    Wire1.resetBus();
    Wire1.beginTransmission(0x01);    
    Wire1.write(3);
    Wire1.write(3);
    Wire1.write(3);    
    Wire1.endTransmission();
    delay(100);    
    delay(20);
    
    bb=Wire1.requestFrom(0x01, 40);
    if(Wire1.getError()){  //mark as an error
          error_occured=1;
            break;            
    }
        else
    {
    for (int i=0; i<bb; i++){   
      returned_value = Wire1.read();
      databuf[i]=returned_value;
    }
      pressure_now = databuf[1];              //send x_high to rightmost 8 bits HIGH BYTE!
      pressure_now = pressure_now<<8;         //shift x_high over to leftmost 8 bits
      pressure_now |= databuf[2];                 //logical OR keeps x_high intact in combined and fills in                                                             //rightmost 8 bits
      if (pressure_now>32767){
        pressure_now=pressure_now-65535;
      }
      
      //pressure_now=256*databuf[1] + databuf[2];
    
    // ****}
    if (i>2) {  //the first two values are bad. 
    //Serial.println(pressure_now);
    accumulated_returned_value = accumulated_returned_value + pressure_now;
    average_index=average_index+1.0;
    }
    delay(1);
    }
    }

    if (error_occured>0) {
      Serial_Print("\"air_flow\": -1,");
      return(-1); 
      
    }
    else {
//    Serial.println(  accumulated_returned_value);
//    Serial.println(  average_index);
    
    int averaged=(float)accumulated_returned_value/average_index;
//    Serial.println(  averaged);
    
    //Serial.print(average_returned_value);
    float fval;
    
    fval=(float)accumulated_returned_value/(average_index*10.0);
   // Serial.println(fval);
    
    //Serial_Printf("\"air_flow\" : %f ,", fval);
    
    average_returned_value=average_returned_value/average_index;
    
    Serial_Printf("\"air_flow\":%d ,", accumulated_returned_value);
    //Serial_Printf("\"air_flow\": %d ,", average_returned_value);

    if (status_values>0) {
      //Serial.print("measured flow rate = ");
      //Serial.print(0.1*pressure_now);
      
      byte number_calibration_points = databuf[3];

      Serial_Printf("\"calibration_set_points\": [");
      
      int starting_address=4; //start at the 4th byte of databuf
      
      for (int ii=0; ii < number_calibration_points * 2; ii=ii+2){ 
          int i = starting_address+ii;
          int tv= databuf[i];
          tv = tv<<8;   
          tv |= databuf[i+1]; 
          
          Serial_Print(tv);
          if (ii< ((number_calibration_points-1) * 2)){
            Serial_Print(",");
          }

      }
      Serial_Print("],");
      
      
      Serial_Printf("\"calibration_values\": [");

      starting_address = 4 + (2*number_calibration_points);  // start after the number_calibration_points
      for (int ii=0; ii < number_calibration_points * 2; ii=ii+2){ 
          int i = starting_address+ii;
          int tv= databuf[i];
          tv = tv<<8;   
          tv |= databuf[i+1]; 
          Serial_Print(tv);
          if (ii< ((number_calibration_points-1) * 2)){
            Serial_Print(",");
          }
      }
     Serial_Print("],");
    }                                          
     return(average_returned_value); 
    
    }
    
  }
  
