/**************************************************************
 * Example Program for FDC1004 Library
 * This example demonstrates using the FDC1004 library to easily measure capacitance from your chip
 * All calculations and ranging are handled by the library.
 * For a more hands-on example, try FDC1004_raw.ino
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

FDC1004 fdc;
//Or, specify a rate: 100HZ, 200HZ, 400HZ
//FDC1004 fdc(FDC1004_400HZ)

void setup() {
  //Wire.begin();
  Serial.begin(115200);
}

void loop() {
  int32_t capacitance = fdc.getCapacitance(0);
  //enable these lines to print calculated capacitance
  Serial.print(capacitance);
  Serial.print(" fF\n");
  delay(1000);
}
