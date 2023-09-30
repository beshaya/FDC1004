/***********************************************************************
 FDC1004 Library

 This library provides functions for using TI's FDC1004 Capacitance to Digital Sensor
 
 Written by Benjamin Shaya

************************************************************************/

#include <FDC1004.h>

#define FDC1004_UPPER_BOUND ((int16_t) 0x4000)
#define FDC1004_LOWER_BOUND (-1 * FDC1004_UPPER_BOUND)

uint8_t MEAS_CONFIG[] = {0x08, 0x09, 0x0A, 0x0B};
uint8_t MEAS_MSB[] = {0x00, 0x02, 0x04, 0x06};
uint8_t MEAS_LSB[] = {0x01, 0x03, 0x05, 0x07};
uint8_t SAMPLE_DELAY[] = {11,11,6,3};

FDC1004::FDC1004(uint16_t rate){
  this->_addr = 0b1010000; //not configurable, to my knowledge
  this->_rate = rate;
}

void FDC1004::write16(uint8_t reg, uint16_t data) {
  Wire.beginTransmission(_addr);
  Wire.write(reg); //send address
  Wire.write( (uint8_t) (data >> 8));
  Wire.write( (uint8_t) data);
  Wire.endTransmission();
}

uint16_t FDC1004::read16(uint8_t reg) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.endTransmission();
  uint16_t value;
  Wire.requestFrom(_addr, (uint8_t)2);
  value = Wire.read();
  value <<= 8;
  value |= Wire.read();
  return value;
}

//configure a measurement (call only when changing the setup of a measurement)
uint8_t FDC1004::configureMeasurementSingle(uint8_t measurement, uint8_t channel, uint8_t capdac) {
    //Verify data
    if (!FDC1004_IS_MEAS(measurement) || !FDC1004_IS_CHANNEL(channel) || capdac > FDC1004_CAPDAC_MAX) {
        Serial.println("bad configuration");
        return 1;
    }
    
    //build 16 bit configuration
    uint16_t configuration_data;
    configuration_data = ((uint16_t)channel) << 13; //CHA
    configuration_data |=  ((uint16_t)0x04) << 10; //CHB disable / CAPDAC enable
    configuration_data |= ((uint16_t)capdac) << 5; //CAPDAC value
    write16(MEAS_CONFIG[measurement], configuration_data);
    return 0;
}

uint8_t FDC1004::triggerSingleMeasurement(uint8_t measurement, uint8_t rate) {
  //verify data
    if (!FDC1004_IS_MEAS(measurement) || !FDC1004_IS_RATE(rate)) {
        Serial.println("bad trigger request");
        return 1;
    }
    uint16_t trigger_data;
    trigger_data = ((uint16_t)rate) << 10; // sample rate
    trigger_data |= 0 << 8; //repeat disabled
    trigger_data |= (1 << (7-measurement)); // 0 > bit 7, 1 > bit 6, etc
    write16(FDC_REGISTER, trigger_data);
    return 0;
}

/**
 * Check if measurement is done, and read the measurement into value if so.
 * Return 0 if successful, 1 if bad request, 2 if measurement did not complete.
 * value should be at least 4 bytes long (24 bit measurement)
 */
uint8_t FDC1004::readMeasurement(uint8_t measurement, uint16_t * value) {
    if (!FDC1004_IS_MEAS(measurement)) {
        Serial.println("bad read request");
        return 1;
    }
    
    //check if measurement is complete
    uint16_t fdc_register = read16(FDC_REGISTER);
    if (! (fdc_register & ( 1 << (3-measurement)))) {
        Serial.println("measurement not completed");
        return 2;
    }
  
  //read the value
  uint16_t msb = read16(MEAS_MSB[measurement]);
  uint16_t lsb = read16(MEAS_LSB[measurement]);  
  value[0] = msb;
  value[1] = lsb;
  return 0;
}

/**
 * Convenience method to take a measurement, uses the measurement register equal to the channel number
 * If you want to do something more complicated, you'll need to use the functions this functions calls
 */
uint8_t FDC1004::measureChannel(uint8_t channel, uint8_t capdac, uint16_t * value) {
  uint8_t measurement = channel; //4 measurement configs, 4 channels, seems fair
  if (configureMeasurementSingle(measurement, channel, capdac)) return 1;
  if (triggerSingleMeasurement(measurement, this->_rate)) return 1;
  delay(SAMPLE_DELAY[this->_rate]);
  return readMeasurement(measurement, value);
}

/**
 * High level function to get the capacitance from a channel.
 * Attempts to manage capdac automagically
 * Uses measureChannel, so you don't control the channel setup.
 */
int32_t FDC1004::getCapacitance(uint8_t channel) {
    fdc1004_measurement_t value;
    uint8_t result = getRawCapacitance(channel, &value);
    if (result) return 0x80000000;
    
    int32_t capacitance = ((int32_t)ATTOFARADS_UPPER_WORD) * ((int32_t)value.value); //attofarads
    capacitance /= 1000; //femtofarads
    capacitance += ((int32_t)FEMTOFARADS_CAPDAC) * ((int32_t)value.capdac);
    return capacitance;
}

/**
 * High level function to get the raw capacitance from a channel
 * Attempts to manage capdac automagically
 * uses measureChannel.
 */
uint8_t FDC1004::getRawCapacitance(uint8_t channel, fdc1004_measurement_t * value) {
    if (!FDC1004_IS_CHANNEL(channel)) return 1;
    value->value = 0x7FFF;
    uint16_t raw_value[2];
    value->capdac = this->_last_capdac[channel]; //load last capdac as starting point

    //sample until we get a good result
    while(value->value > 0x7E00 || value->value < 0x8100) {
        if (measureChannel(channel, value->capdac, raw_value)) {
            Serial.println("error");
            return 1;
        }
        value->value = (int16_t)raw_value[0];

        //adjust capdac if necessary
        if (value->value > FDC1004_UPPER_BOUND && value->capdac < FDC1004_CAPDAC_MAX) {
            value->capdac++;
        } else if (value->value < FDC1004_LOWER_BOUND && value->capdac > 0) {
            value->capdac--;
        } else {
            //out of range, but capdac is already maxed (or minned). Return.
            this->_last_capdac[channel] = value->capdac;
            return 0;
        }
    }
    this->_last_capdac[channel] = value->capdac;
    return 0;

}
