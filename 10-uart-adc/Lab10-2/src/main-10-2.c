/********************************************************
 * Micro Computer System Lab - Lab 10: UART and ADC
 *                                               Group 49
 ********************************************************
 * Lab 10-2: Measure the Voltage of Photoresistor
 *
 * Measure the voltage of photoresister via the onboard
 * ADC (Analog to Digital Converter) with a resolution of
 * 12 bit and display the result when the user button
 * (PC13) is triggered.
 ********************************************************/

#define STM32L476XX

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stdio.h"

#include "stm32l476xx.h"
#include "stm32l4xx_ll_adc.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_tim.h"
#include "stm32l4xx_ll_usart.h"

/*******************************************************
 * CLOCK configuration
 *******************************************************/

void SystemClock_Config() {
  // Set system clock to 10 MHz
  LL_RCC_MSI_EnableRangeSelection();
  LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_9);  // Range mode 9: 01001, 24MHz
  while (!LL_RCC_MSI_IsReady())
    ;
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);
  // 10 = 24 * ( 10 / 3 ) / 8
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_3, 10,
                              LL_RCC_PLLR_DIV_8);
  LL_RCC_PLL_Enable();
  LL_RCC_PLL_EnableDomain_SYS();
  while (!LL_RCC_PLL_IsReady())
    ;
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
}

#define UserButton GPIOC, LL_GPIO_PIN_13
#define SetPin LL_GPIO_SetOutputPin
#define ResetPin LL_GPIO_ResetOutputPin
#define IsSetPin LL_GPIO_IsInputPinSet
void UserBtn_Config() {
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
  // Initialize user button
  LL_GPIO_SetPinMode(UserButton, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinSpeed(UserButton, LL_GPIO_SPEED_HIGH);
  LL_GPIO_SetPinPull(UserButton, LL_GPIO_PULL_DOWN);
}

#define DEBOUNCE 40
int user_press_button() {
  static int btn = 1;
  static int cnt = DEBOUNCE;
  static int pressed = 0;
  if (IsSetPin(UserButton) ^ btn) {
    if (!--cnt) btn = !btn, pressed = 0;
  } else {
    cnt = DEBOUNCE;
  }
  return !btn && !pressed ? (pressed = 1) : 0;
}

/*******************************************************
 * USART configuration
 *******************************************************/
void Usart2_Config() {
  // Enable corresponding GPIO port
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  uint32_t gpio_pin = LL_GPIO_PIN_2;
  LL_GPIO_SetPinMode(GPIOA, gpio_pin, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinPull(GPIOA, gpio_pin, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinSpeed(GPIOA, gpio_pin, LL_GPIO_SPEED_FREQ_VERY_HIGH);
  LL_GPIO_SetAFPin_0_7(GPIOA, gpio_pin, LL_GPIO_AF_7);
  gpio_pin = LL_GPIO_PIN_3;
  LL_GPIO_SetPinMode(GPIOA, gpio_pin, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinPull(GPIOA, gpio_pin, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinSpeed(GPIOA, gpio_pin, LL_GPIO_SPEED_FREQ_VERY_HIGH);
  LL_GPIO_SetAFPin_0_7(GPIOA, gpio_pin, LL_GPIO_AF_7);
  // Enable USART timer
  LL_RCC_SetUARTClockSource(LL_RCC_USART2_CLKSOURCE_SYSCLK);
  RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

  USART2->CR1 =
      (USART2->CR1 &
       ~(USART_CR1_OVER8_Msk | USART_CR1_M_Msk | USART_CR1_TE_Msk |
         USART_CR1_RE_Msk | USART_CR1_PCE_Msk)) |
      (0 /*over sample 16*/ | 0 /*Word Length 8*/
       | USART_CR1_TE /*TX EN*/ | USART_CR1_RE /*RX EN*/ | 0 /*No Parity*/);
  USART2->CR2 = (USART2->CR2 & ~(USART_CR2_STOP_Msk)) | (0 /*1 stop bit*/);
  // Set baud rate to 9600:
  // The System clock rate is 10MHz, USARTDIV = 10MHz / 9600 = 1042
  // Over sampling is 16, no shift required for last 3 bits
  USART2->BRR = 10000000 / 9600;
  // Enable USART2
  USART2->CR1 |= USART_CR1_UE;
}

void Usart2_Send(char *d) {
  while (*d) {
    // Wait till transmission empty flag set
    while (!(USART2->ISR & USART_ISR_TXE))
      ;
    USART2->TDR = *d++;
  }
}

void configureADC() {
  uint32_t channel = LL_ADC_CHANNEL_5;

  // Enable ADC Corresponding GPIO Port
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_0, LL_GPIO_PULL_NO);
  LL_GPIO_EnablePinAnalogControl(GPIOA, LL_GPIO_PIN_0);

  // Enable Clock
  RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
  LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSOURCE_SYSCLK);

  // Keep pre-scalar as default
  LL_ADC_SetCommonClock(ADC123_COMMON, LL_ADC_CLOCK_SYNC_PCLK_DIV1);
  LL_ADC_SetMultimode(ADC123_COMMON, LL_ADC_MULTI_INDEPENDENT);
  LL_ADC_SetOverSamplingScope(ADC1, LL_ADC_OVS_DISABLE);
  LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_12B);
  LL_ADC_SetChannelSamplingTime(ADC1, channel, LL_ADC_SAMPLINGTIME_6CYCLES_5);
  LL_ADC_SetDataAlignment(ADC1, LL_ADC_DATA_ALIGN_RIGHT);

  // Disable deep power mode to start the voltage conversion
  LL_ADC_DisableDeepPowerDown(ADC1);

  // Enable related interrupt
  NVIC_EnableIRQ(ADC1_2_IRQn);
  NVIC_SetPriority(ADC1_2_IRQn, 0);

  LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_SINGLE);
  LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_SOFTWARE);
  LL_ADC_REG_SetSequencerDiscont(ADC1, LL_ADC_REG_SEQ_DISCONT_DISABLE);

  LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE);
  LL_ADC_REG_SetSequencerDiscont(ADC1, LL_ADC_REG_SEQ_DISCONT_DISABLE);
  LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1,
                               channel);  // PA0 -> ADC12_IN_5

  LL_ADC_EnableInternalRegulator(ADC1);
  while (!LL_ADC_IsInternalRegulatorEnabled(ADC1))
    ;

  LL_ADC_EnableIT_EOC(ADC1);
  LL_ADC_Enable(ADC1);
}

int raw_data = 10;
void ADC1_2_IRQHandler() {
  NVIC_ClearPendingIRQ(ADC1_2_IRQn);
  if (LL_ADC_IsActiveFlag_EOC(ADC1)) {
    raw_data = ADC1->DR;
    LL_ADC_ClearFlag_EOC(ADC1);
  }
}
void startADC() { LL_ADC_REG_StartConversion(ADC1); }

char buf[100];
int main() {
  // Enable FPU
  MODIFY_REG(SCB->CPACR, 0xf << 20, 0xf << 20);
  SystemClock_Config();
  Usart2_Config();
  UserBtn_Config();
  configureADC();
  startADC();
  Usart2_Send("\033[0;0H\033[2J");
  int i = 10000;
  while (1) {
    if (user_press_button()) {
      // use 10k\ohm resister
      sprintf(buf, "\033[2K\rData: %f", raw_data * 10.0e3 / (0xfff - raw_data));
      Usart2_Send(buf);
    }
    if (!i--) startADC(), i = 10000;
  }
  return 0;
}
