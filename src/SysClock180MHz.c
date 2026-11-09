//***********************************************************
//  SysClock180MHz.c
//  sets the system clock to 180MHz
//***********************************************************

#include "stm32f4xx.h"
#include "SysClock180MHz.h"

#define PLL_M       8ul    // VCO input frequency = 16MHz / 8 = 2MHz
#define PLL_N       180ul  // VCO output frequency = 2MHz * 180 = 360MHz
#define PLL_P       0ul    // SYSCLK = 360MHz / 2 = 180MHz
#define PLL_Q       2ul    // USB OTG FS, SDIO and RNG Clock = 360MHz / 2 = 180MHz

void SystemClock_Config(void) {
  // change the clock from 16MHz to 180MHz
  RCC->CR |= RCC_CR_HSION; // turn on HSI
  while (!(RCC->CR & RCC_CR_HSIRDY)); // wait for HSI
  // PLL: VCO = (16MHz / M) * N = 360MHz, SYSCLK = VCO / P = 180MHz, other clocks = VCO / Q = 180MHz
  RCC->PLLCFGR = (PLL_M) | (PLL_N << 6) | (PLL_P << 16) | (PLL_Q << 24);
  RCC->PLLCFGR &=~ RCC_PLLCFGR_PLLSRC; // HSI is the PLL source
  RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_PPRE1_DIV4; // AHB = SYSCLK/1, APB2 = AHB/2, APB1 = AHB/4
  // APB1 = 45MHz (its timers run at 90MHz), APB2 = 90MHz
  
  RCC->CR |= RCC_CR_PLLON; // turn on the PLL
  while (!(RCC->CR & RCC_CR_PLLRDY));// wait for the PLL
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;// clock for the power interface
  PWR->CR |= PWR_CR_ODEN;// over-drive on
  while (!(PWR->CSR & PWR_CSR_ODRDY));// wait for over-drive
  PWR->CR |= PWR_CR_ODSWEN;// over-drive switching on
  while (!(PWR->CSR & PWR_CSR_ODSWRDY)) ;// wait for over-drive switching
  FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_5WS;// flash needs 5 wait states at 180MHz
  RCC->CFGR &=~ RCC_CFGR_SW;// clear the SW bits
  RCC->CFGR |= RCC_CFGR_SW_PLL;// PLL is the system clock
  while ((RCC->CFGR & RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL);// wait until the PLL is the system clock
}
