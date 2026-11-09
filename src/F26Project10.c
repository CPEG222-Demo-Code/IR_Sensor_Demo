/****************************************************************
* Author: T. LUM and R. MARTIN
* CPEG222 Project10 Program, 11/9/26
* NucleoF446ZE CMSIS STM32F4xx IR sensor array test
* Shows the raw count from each IR sensor on the OLED.
*   - IR1 = PA3 (right-most), IR2 = PC0, IR3 = PC3, IR4 = PF3,
*     IR5 = PF5 (left-most)
*   - counts go from 0 to 4095 at 3.3 V, black reads higher than white
*   - an LED turns on when its sensor count is above the threshold:
*     IR1 = LED1, IR2 = LED2, IR3 = LED4 and LED5, IR4 = LED7,
*     IR5 = LED8 ou are welcome to change LED order or threshold values per sensor
*   - the 4 NeoPixels on PB11 are turned off
* System clock is 180 MHz.
****************************************************************/
#include "stm32f4xx.h"
#include "OLED.h"
#include "IRSensors.h"
#include "SysClock180MHz.h"
#include "NeoPixel.h"
#include <stdio.h> // snprintf

#define LED_PORT GPIOD // LED1 to LED8 are PD0 to PD7
#define LED1_PIN 0
#define LED2_PIN 1
#define LED4_PIN 3
#define LED5_PIN 4
#define LED7_PIN 6
#define LED8_PIN 7

// above the threshold = the sensor sees black and LED turns on, below the threshold = the sensor sees white and LED turns off
// you may want to change these after you calibrate, see the handout
#define THRESH_IR1 1500
#define THRESH_IR2 1500
#define THRESH_IR3 1500
#define THRESH_IR4 1500
#define THRESH_IR5 1500

char buffer[30]; // one row of OLED text

void delay_ms(volatile uint32_t count) { // simple delay in ms
    count = count * 45000; // about 1 ms per count at 180 MHz
    while (count--) {
    }
}

int main(void) {
    SystemClock_Config(); // 180 MHz clock

    // LEDs, PD0 to PD7 as outputs
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    LED_PORT->MODER &= ~(0xFFFF << (LED1_PIN * 2)); // clear the mode bits
    LED_PORT->MODER |= (0x5555 << (LED1_PIN * 2));  // 01 = output

    // IR pins and ADC3, the code is in IRSensors.c
    ADC3_Init_Specific_Pins();

    // NeoPixels off , they are annoyingly bright if they get stuck on
    NeoPixel_Init_PB11();
    NeoPixel_SetColor(0, 0, 0, 0);
    NeoPixel_SetColor(1, 0, 0, 0);
    NeoPixel_SetColor(2, 0, 0, 0);
    NeoPixel_SetColor(3, 0, 0, 0);
    NeoPixel_Show_PB11();

    // OLED text that doesn't change
    OLED_Init();
    OLED_Print(0, 0, "F26 Project 10");
    OLED_Print(1, 0, "Lum/Martin  11/9/26");
    OLED_Print(7, 0, "  T = threshold");

    while (1) {
        Read_ADC3_Channels(); // read the sensors

        // IR1 -> LED1
        if (IRSensor1 > THRESH_IR1) {
            LED_PORT->BSRR = (1U << LED1_PIN);
        } else {
            LED_PORT->BSRR = (1U << (LED1_PIN + 16U));
        }
        // IR2 -> LED2
        if (IRSensor2 > THRESH_IR2) {
            LED_PORT->BSRR = (1U << LED2_PIN);
        } else {
            LED_PORT->BSRR = (1U << (LED2_PIN + 16U));
        }
        // IR3 -> LED4 and LED5
        if (IRSensor3 > THRESH_IR3) {
            LED_PORT->BSRR = (1U << LED4_PIN) | (1U << LED5_PIN);
        } else {
            LED_PORT->BSRR = (1U << (LED4_PIN + 16U)) | (1U << (LED5_PIN + 16U));
        }
        // IR4 -> LED7
        if (IRSensor4 > THRESH_IR4) {
            LED_PORT->BSRR = (1U << LED7_PIN);
        } else {
            LED_PORT->BSRR = (1U << (LED7_PIN + 16U));
        }
        // IR5 -> LED8
        if (IRSensor5 > THRESH_IR5) {
            LED_PORT->BSRR = (1U << LED8_PIN);
        } else {
            LED_PORT->BSRR = (1U << (LED8_PIN + 16U));
        }

        // print the counts and thresholds
        snprintf(buffer, sizeof(buffer), "  IR1: %4u  T:%4u", IRSensor1, THRESH_IR1);
        OLED_Print(2, 0, buffer);
        snprintf(buffer, sizeof(buffer), "  IR2: %4u  T:%4u", IRSensor2, THRESH_IR2);
        OLED_Print(3, 0, buffer);
        snprintf(buffer, sizeof(buffer), "  IR3: %4u  T:%4u", IRSensor3, THRESH_IR3);
        OLED_Print(4, 0, buffer);
        snprintf(buffer, sizeof(buffer), "  IR4: %4u  T:%4u", IRSensor4, THRESH_IR4);
        OLED_Print(5, 0, buffer);
        snprintf(buffer, sizeof(buffer), "  IR5: %4u  T:%4u", IRSensor5, THRESH_IR5);
        OLED_Print(6, 0, buffer);

        delay_ms(100); // about 10 updates a second
    }
}
