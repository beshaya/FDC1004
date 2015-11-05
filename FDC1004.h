/**************************
 This is a library for the FDC1004 Capacitance Sensor

 Written by Benjamin Shaya
**************************/

#include "Arduino.h"
#include "Wire.h"

#define FDC1004_100HZ (0x01)
#define FDC1004_200HZ (0x02)
#define FDC1004_400HZ (0x03)
#define FDC1004_CAPDAC_MAX (0x1F)

#define FDC_REGISTER (0x0C)

class FDC1004 {
 public:
    FDC1004(uint16_t rate = FDC1004_100HZ);
    uint8_t configureMeasurementSingle(uint8_t measurement, uint8_t channel, uint8_t capdac); 
    uint8_t triggerSingleMeasurement(uint8_t measurement, uint8_t rate);
    uint8_t readMeasurement(uint8_t measurement, uint16_t * value);
    uint8_t measureChannel(uint8_t channel, uint8_t capdac, uint16_t * value);


 private:
    uint8_t _addr;
    uint8_t _rate;
    void write16(uint8_t reg, uint16_t data);
    uint16_t read16(uint8_t reg);
};
