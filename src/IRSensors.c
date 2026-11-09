#include "stm32f4xx.h"
#include "IRSensors.h"

volatile uint16_t IRSensor1 = 0; // IR1, PA3 (right-most)
volatile uint16_t IRSensor2 = 0; // IR2, PC0
volatile uint16_t IRSensor3 = 0; // IR3, PC3
volatile uint16_t IRSensor4 = 0; // IR4, PF3
volatile uint16_t IRSensor5 = 0; // IR5, PF5 (left-most)

#define IR1_PIN         3       // PA3, ADC3_IN3
#define IR1_PORT        GPIOA
#define IR2_PIN         0       // PC0, ADC3_IN10
#define IR2_PORT        GPIOC
#define IR3_PIN         3       // PC3, ADC3_IN13
#define IR3_PORT        GPIOC
#define IR4_PIN         3       // PF3, ADC3_IN9
#define IR4_PORT        GPIOF
#define IR5_PIN         5       // PF5, ADC3_IN15
#define IR5_PORT        GPIOF
#define ADC_BUFF_LEN    5   // five IR channels

void ADC3_Init_Specific_Pins(void) {
    // 1. clocks for GPIOA, GPIOC, GPIOF and ADC3
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOFEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;

    // 2. IR pins to analog mode (11)
    GPIOA->MODER |= (3U << (3 * 2));                      // PA3
    GPIOC->MODER |= (3U << (0 * 2)) | (3U << (3 * 2));    // PC0, PC3
    GPIOF->MODER |= (3U << (3 * 2)) | (3U << (5 * 2));    // PF3, PF5

    // 3. longest sample time (111) on every channel so the readings are steady
    ADC3->SMPR2 |= (7U << (3 * 3))   /* channel 3 */ |
                   (7U << (9 * 3));  /* channel 9 */
    ADC3->SMPR1 |= (7U << (0 * 3))   /* channel 10 */ |
                   (7U << (3 * 3))   /* channel 13 */ |
                   (7U << (5 * 3));  /* channel 15 */

    // 4. ADC clock is PCLK2 / 4
    ADC->CCR |= (1U << 16);

    // 5. one conversion at a time, no scan, no DMA
    ADC3->CR1 = 0;
    ADC3->CR2 = 0;
    ADC3->SQR1 &= ~(0xF << 20); // sequence length of 1
    ADC3->SQR3 = 0;
    ADC3->SQR3 |= (3  << 0)  | (10 << 5)  | (13 << 10) | (9  << 15) | (15 << 20); // IR1 to IR5

    // 6. turn on ADC3
    ADC3->CR2 |= ADC_CR2_ADON;
    for (volatile int i = 0; i < 1000; ++i) __NOP(); // short wait after ADON
}

void Read_ADC3_Channels(void) {
    static const uint8_t adc_channels[ADC_BUFF_LEN] = { 3, 10, 13, 9, 15 };  // IR1 to IR5

    ADC3->SQR1 &= ~(0xF << 20); // one conversion at a time
    for (int i = 0; i < ADC_BUFF_LEN; i++) {
        // put this channel first in the sequence
        ADC3->SQR3 &= ~0x1FU;
        ADC3->SQR3 |= (adc_channels[i] & 0x1F) << 0;
        ADC3->CR2 |= ADC_CR2_SWSTART; // start the conversion
        while (!(ADC3->SR & ADC_SR_EOC)) { }     // wait until it's done
        switch (adc_channels[i]) {
            case 3:  IRSensor1 = ADC3->DR; break; // PA3 (reading DR clears EOC)
            case 10: IRSensor2 = ADC3->DR; break; // PC0
            case 13: IRSensor3 = ADC3->DR; break; // PC3
            case 9:  IRSensor4 = ADC3->DR; break; // PF3
            case 15: IRSensor5 = ADC3->DR; break; // PF5
        }
    }
}
