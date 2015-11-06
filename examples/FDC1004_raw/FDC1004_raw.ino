/**************************************************************
 * Example Program for FDC1004 Library
 * This program will demonstrates reading the capacitance on CAP1 of the FDC1004
 * and changing capdac to try to auto-range the chip
 * Note that the output is a signed integer, so values greater than 7FFF FFFF are actually negative!
 **************************************************************
 * Setup
 * Connect 3.3V and Ground to the FDC1004 (don't use 5V, you'll fry your chip!)
 * Connect SDA and SCL to your Arduino
 * Power on and run this code
 * Open a Serial monitor at 115200 baud
 **************************************************************
 * Written by Benjamin Shaya for Rest Devices
 * bshaya@alum.mit.edu
 * https://github.com/beshaya/FDC1004
 **************************************************************/
 
#include <Wire.h>
#include <FDC1004.h>

int capdac = 0;

FDC1004 fdc;

void setup() {
  //Wire.begin();
  Serial.begin(115200);
}

void loop() {
  uint8_t measurement = 0;
  uint8_t channel = 0;
  char result[100];


  fdc.configureMeasurementSingle(measurement, channel, capdac);
  fdc.triggerSingleMeasurement(measurement, FDC1004_100HZ);
  //wait for completion
  delay(15);
  uint16_t value[2];
  if (! fdc.readMeasurement(measurement, value)) {
    int16_t msb = (int16_t) value[0];
    int32_t capacitance = ((int32_t)457) * ((int32_t)msb); //in attofarads
    capacitance /= 1000; //in femtofarads
    capacitance += ((int32_t)3028) * ((int32_t)capdac);

    sprintf(result, "Raw: %04X %04X @ %02X\n ->", msb, value[1], capdac);    
    Serial.print(result);
    Serial.print(capacitance);
    Serial.print(" fF\n");
    
    //adjust capdac
    if (msb > 0x7000) {
      if (capdac < FDC1004_CAPDAC_MAX) capdac++;
    } else if (msb < 0x8F00) {
      if (capdac > 0) capdac--;
    }
  }
  delay(20);
}
