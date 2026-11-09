#include "stm32f4xx.h"

#define NUM_LEDS          4
#define BITS_PER_LED      24
#define RESET_SLOTS       50
#define PWM_0_VAL         36  // 0.4 us high at 90 MHz
#define PWM_1_VAL         72  // 0.8 us high at 90 MHz

#define BUFFER_LEN        ((NUM_LEDS * BITS_PER_LED) + RESET_SLOTS)

// 32-bit entries, CCR4 is a 32-bit register
uint32_t dma_pwm_buffer[BUFFER_LEN];

// put one pixel's color in the buffer
void NeoPixel_SetColor(uint8_t index, uint8_t red, uint8_t green, uint8_t blue) {
    if (index >= NUM_LEDS) return;

    // WS2812B wants green, red, blue, MSB first
    uint32_t grb = ((uint32_t)green << 16) | ((uint32_t)red << 8) | blue;
    uint32_t start_idx = index * BITS_PER_LED;

    for (int i = 0; i < 24; i++) {
        // go from MSB to LSB
        if (grb & (1U << (23 - i))) {
            dma_pwm_buffer[start_idx + i] = PWM_1_VAL;
        } else {
            dma_pwm_buffer[start_idx + i] = PWM_0_VAL;
        }
    }
}

void NeoPixel_Init_PB11(void) {
    // 1. clocks for GPIOB, TIM2 and DMA1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // an off pixel is still 24 zero bits, so fill the buffer with 0 bits
    for (uint32_t i = 0; i < NUM_LEDS * BITS_PER_LED; i++) {
        dma_pwm_buffer[i] = PWM_0_VAL;
    }

    // 2. PB11 is TIM2_CH4, alternate function AF1
    GPIOB->MODER   &= ~(3U << (11 * 2));
    GPIOB->MODER   |=  (2U << (11 * 2)); // AF mode (10)
    GPIOB->OSPEEDR |=  (3U << (11 * 2)); // high speed

    // PB11 is in AFR[1] at position (11 - 8) * 4 = 12
    GPIOB->AFR[1]  &= ~(0xFU << ((11 - 8) * 4));
    GPIOB->AFR[1]  |=  (0x1U << ((11 - 8) * 4)); // AF1 = TIM2

    // 3. TIM2 channel 4 as PWM
    TIM2->PSC  = 0;                   // 90 MHz, no prescaler
    TIM2->ARR  = 111;                 // 800 kHz, 1.25 us period
    TIM2->CCR4 = 0;
    
    // PWM mode 1 and turn on the output
    TIM2->CCMR2 &= ~TIM_CCMR2_OC4M;
    TIM2->CCMR2 |=  (6U << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE; // PWM mode 1 + preload
    TIM2->CCER  |=  TIM_CCER_CC4E;                               // output on
    TIM2->DIER  &= ~TIM_DIER_CC4DE;                              // no CC4 DMA request, Show uses the update event

    // 4. DMA1 Stream 7 Channel 3 (TIM2_UP request)
    DMA1_Stream7->CR &= ~DMA_SxCR_EN; // off while we set it up
    while (DMA1_Stream7->CR & DMA_SxCR_EN);

    DMA1_Stream7->PAR  = (uint32_t)&(TIM2->CCR4); // writes go to TIM2 CCR4
    DMA1_Stream7->M0AR = (uint32_t)dma_pwm_buffer;
    DMA1_Stream7->NDTR = BUFFER_LEN;

    // channel 3, memory to peripheral, 32-bit, memory increment, normal mode
    DMA1_Stream7->CR   = (3U << DMA_SxCR_CHSEL_Pos) |
                         DMA_SxCR_DIR_0 |
                         DMA_SxCR_MINC |
                         DMA_SxCR_MSIZE_1 |
                         DMA_SxCR_PSIZE_1;
}

void NeoPixel_Show_PB11(void) {
    // stop DMA and the timer
    TIM2->CR1 &= ~TIM_CR1_CEN;
    DMA1_Stream7->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream7->CR & DMA_SxCR_EN);

    // CCR4 = 0 (output low), no DMA request
    TIM2->DIER &= ~TIM_DIER_UDE;
    TIM2->CCR4 = 0;
    TIM2->EGR  = TIM_EGR_UG;
    TIM2->SR   = 0;

    // reset the transfer count and clear the stream 7 flags
    DMA1_Stream7->M0AR = (uint32_t)dma_pwm_buffer;
    DMA1_Stream7->NDTR = BUFFER_LEN;
    DMA1->HIFCR = DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 |
                  DMA_HIFCR_CDMEIF7 | DMA_HIFCR_CFEIF7;

    // each update event (one per bit) makes DMA write the next value
    DMA1_Stream7->CR |= DMA_SxCR_EN;
    TIM2->CNT = 0;
    TIM2->DIER |= TIM_DIER_UDE;
    TIM2->CR1 |= TIM_CR1_CEN;
}