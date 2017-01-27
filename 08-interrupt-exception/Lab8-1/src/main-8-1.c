/******************************************************
 * Lab 8-1: SysTick timer interrupt
 * Implement the SysTick interrupt handler to toggle the
 * on board LCD every half of a second.
 *******************************************************/
#define STM32L476XX

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stm32l476xx.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_rcc.h"

#define UserButton GPIOC, LL_GPIO_PIN_13
#define LED GPIOA, LL_GPIO_PIN_5

#define SetPin LL_GPIO_SetOutputPin
#define ResetPin LL_GPIO_ResetOutputPin
#define IsSetPin LL_GPIO_IsInputPinSet

#define EnableSysTick()                              \
  MODIFY_REG(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk, \
             (1) << SysTick_CTRL_ENABLE_Pos)
#define DisableSysTick()                             \
  MODIFY_REG(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk, \
             (0) << SysTick_CTRL_ENABLE_Pos)
#define EnableSysTickInt()                            \
  MODIFY_REG(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk, \
             (1) << SysTick_CTRL_TICKINT_Pos)
#define SelectSysTickSrc()                              \
  MODIFY_REG(SysTick->CTRL, SysTick_CTRL_CLKSOURCE_Msk, \
             (1) << SysTick_CTRL_CLKSOURCE_Pos)
#define IsEnableSysTick()                              \
  (READ_BIT(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk) == \
   (SysTick_CTRL_ENABLE_Msk))
void GPIO_init() {
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;

  // Initialize user button
  LL_GPIO_SetPinMode(UserButton, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinSpeed(UserButton, LL_GPIO_SPEED_HIGH);
  LL_GPIO_SetPinPull(UserButton, LL_GPIO_PULL_DOWN);

  // Initialize led
  LL_GPIO_SetPinMode(LED, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(LED, LL_GPIO_SPEED_HIGH);
  LL_GPIO_SetPinOutputType(LED, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(LED, LL_GPIO_PULL_NO);
}

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

  if (IsEnableSysTick()) DisableSysTick();
  SysTick->LOAD = 0x4c4b40 - 1;  // 10_000_000 * 0.5
  SysTick->VAL = 0;
  SelectSysTickSrc();  // Select SysTick without pre-scaler
  EnableSysTickInt();  // Enable SysTick Interrupt
  EnableSysTick();
}

// Toggle the LED when interrupt occurred
void SysTick_Handler() { IsSetPin(LED) ? ResetPin(LED) : SetPin(LED); }

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

int main() {
  SystemClock_Config();
  GPIO_init();
  while (1) {
    if (user_press_button()) {
      // Clock Enable toggle
      IsEnableSysTick() ? DisableSysTick() : EnableSysTick();
    }
  }
}
