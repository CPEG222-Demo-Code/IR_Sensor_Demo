#ifndef IRSENSORS_H
#define IRSENSORS_H

#include "stm32f4xx.h"

extern volatile uint16_t IRSensor1; // raw counts, 0 to 4095
extern volatile uint16_t IRSensor2;
extern volatile uint16_t IRSensor3;
extern volatile uint16_t IRSensor4;
extern volatile uint16_t IRSensor5;

void ADC3_Init_Specific_Pins(void);
void Read_ADC3_Channels(void);

#endif // IRSENSORS_H