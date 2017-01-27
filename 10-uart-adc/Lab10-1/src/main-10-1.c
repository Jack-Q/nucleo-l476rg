/********************************************************
 * Micro Computer System Lab - Lab 10: UART and ADC
 *                                               Group 49
 ********************************************************
 * Lab 10-1: Hello World!
 *
 * Send the string "Hello World!" to computer when the
 * user button is triggered via the TX (transmit)
 * functionality of UART (Universal Asynchronous
 * Receiver/Transmitter).
 ********************************************************/

#define STM32L476XX

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stm32l476xx.h"
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

/*******************************************************
 * User Button configuration
 *******************************************************/

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

  // Set baud rate to 9600:
  // The System clock rate is 10MHz, USARTDIV = 10MHz / 9600 = 1042
  // Over sampling is 16, no shift required for last 3 bits
  USART2->BRR = 10000000 / 9600;

  USART2->CR2 = (USART2->CR2 & ~(USART_CR2_STOP_Msk)) | (0 /*1 stop bit*/);

  // Enable USART2
  USART2->CR1 |= USART_CR1_UE;
  USART2->CR1 |= USART_CR1_RE;
}

void Usart2_Send_ch(char c) {
  // Wait till transmission empty flag set
  while (!(USART2->ISR & USART_ISR_TXE))
    ;
  USART2->TDR = c;
}

void Usart2_Send(char *d) {
  while (*d) Usart2_Send_ch(*d++);
}

int main() {
  SystemClock_Config();
  Usart2_Config();
  UserBtn_Config();
  Usart2_Send("\033[0;0H\033[2J");
  char *data = "\033[2K\rHello World!\r\n";
  while (1) {
    if (user_press_button()) {
      Usart2_Send(data);
    }
  }
  return 0;
}
