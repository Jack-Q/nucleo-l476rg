/*******************************************
 * Lab 8-3: Simple alarm clock implemented by
 * SysTick timer, User button, KeyPad and buzzer
 * ******************************************/
#define STM32L476XX

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stm32l476xx.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_exti.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_tim.h"

// Keypad GPIO port connection
#define X0 GPIOB, LL_GPIO_PIN_6
#define X1 GPIOA, LL_GPIO_PIN_7
#define X2 GPIOA, LL_GPIO_PIN_6
#define X3 GPIOA, LL_GPIO_PIN_5
#define Y0 GPIOB, LL_GPIO_PIN_10
#define Y1 GPIOA, LL_GPIO_PIN_8
#define Y2 GPIOA, LL_GPIO_PIN_9
#define Y3 GPIOC, LL_GPIO_PIN_7

// Buzzer
#define BZ GPIOB, LL_GPIO_PIN_8

// User button
#define UserButton GPIOC, LL_GPIO_PIN_13

// SysTick Configuration
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

/************************************************
 * KEYPAD INIT & SCANNIG
 *************************************************/
#define SetPin LL_GPIO_SetOutputPin
#define ResetPin LL_GPIO_ResetOutputPin
#define ResetPinAll() \
  ResetPin(X0);       \
  ResetPin(X1);       \
  ResetPin(X2);       \
  ResetPin(X3)
#define SetPinOnly \
  ResetPinAll();   \
  SetPin
#define IsSetPin LL_GPIO_IsInputPinSet

/* Initialization of GPIO port connected to keypad */
void GPIO_init_keypad() {
  /* RCC AHB2 Bus enable*/
  RCC->AHB2ENR |=
      RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;

  //
  LL_GPIO_InitTypeDef ll_gpio_init;

  // Set output mode
  LL_GPIO_StructInit(&ll_gpio_init);

  ll_gpio_init.Mode = LL_GPIO_MODE_OUTPUT;
  ll_gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  ll_gpio_init.Speed = LL_GPIO_SPEED_HIGH;
  ll_gpio_init.Pull = LL_GPIO_PULL_NO;

  ll_gpio_init.Pin = LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
  LL_GPIO_Init(GPIOA, &ll_gpio_init);
  ll_gpio_init.Pin = LL_GPIO_PIN_6;
  LL_GPIO_Init(GPIOB, &ll_gpio_init);

  // Set input mode
  LL_GPIO_StructInit(&ll_gpio_init);

  ll_gpio_init.Mode = LL_GPIO_MODE_INPUT;
  ll_gpio_init.Pull = LL_GPIO_PULL_DOWN;

  ll_gpio_init.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
  LL_GPIO_Init(GPIOA, &ll_gpio_init);
  ll_gpio_init.Pin = LL_GPIO_PIN_10;
  LL_GPIO_Init(GPIOB, &ll_gpio_init);
  ll_gpio_init.Pin = LL_GPIO_PIN_7;
  LL_GPIO_Init(GPIOC, &ll_gpio_init);
}

/* scan keypad value
 *   this simple implementation have no prevention to
 *   the bounce problem of the button
 * return:
 *   >= 0: key pressed value
 *   -1: no key press
 */
int keypad_scan() {
  SetPinOnly(X0);
  if (IsSetPin(Y0)) return 1;
  if (IsSetPin(Y1)) return 4;
  if (IsSetPin(Y2)) return 7;
  if (IsSetPin(Y3)) return 15;  // *
  SetPinOnly(X1);
  if (IsSetPin(Y0)) return 2;
  if (IsSetPin(Y1)) return 5;
  if (IsSetPin(Y2)) return 8;
  if (IsSetPin(Y3)) return 0;
  SetPinOnly(X2);
  if (IsSetPin(Y0)) return 3;
  if (IsSetPin(Y1)) return 6;
  if (IsSetPin(Y2)) return 9;
  if (IsSetPin(Y3)) return 14;  // #
  SetPinOnly(X3);
  if (IsSetPin(Y0)) return 10;  // A
  if (IsSetPin(Y1)) return 11;  // B
  if (IsSetPin(Y2)) return 12;  // C
  if (IsSetPin(Y3)) return 13;  // D
  return -1;
}

/*************************************************
 * BUZZER INIT & TIMER SETUP
 *************************************************/
void GPIO_init_buzzer() {
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
  LL_GPIO_SetPinMode(BZ, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(BZ, LL_GPIO_SPEED_HIGH);
  LL_GPIO_SetPinOutputType(BZ, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(BZ, LL_GPIO_PULL_NO);
}

/****************************************************
 * USER BUTTON
 ****************************************************/
void GPIO_init_btn() {
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  // Initialize user button
  LL_GPIO_SetPinMode(UserButton, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinPull(UserButton, LL_GPIO_PULL_DOWN);

  MODIFY_REG(SYSCFG->EXTICR[3], SYSCFG_EXTICR4_EXTI13_Msk,
             SYSCFG_EXTICR4_EXTI13_PC);

  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_13);          // EXTI_IMR
  LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_13);  // EXTI_FTSR

  NVIC_EnableIRQ(EXTI15_10_IRQn);
  NVIC_SetPriority(EXTI15_10_IRQn, 0);
}

/******************************************************
 * 7-Segment LED
 *******************************************************/

extern void GPIO_init_max7219();
extern void Max7219_init();
extern void Max7219_send(unsigned char address, unsigned char data);

/* Get the length required
 * by a number to be displayed in 7-Seg LED */
int getLen(int number) {
  if (!number) return 1;
  int digit = 0;
  if (number < 0) digit++, number = -number;
  while (number > 0) digit++, number /= 10;
  return digit;
}

#define MAX_SCAN_LIM 0x0B
#define MAX_SHUTDOWN 0x0C
/**
 * Show data on 7-seg via max7219_send
 * Input:
 *   data: decimal value
 *   num_digs: number of digits will show on 7-seg
 * Return:
 *   0: success
 *   -1: illegal data range (out of 8 digits range)
 */
int display(int data, int num_digs) {
  static int dig = -1;
  if (data > 99999999 || data < -9999999) return -1;
  if (num_digs > 8 || num_digs < 0) return -1;
  if (num_digs != dig) {
    if (!dig) Max7219_send(MAX_SHUTDOWN, 1);
    dig = num_digs;
    if (dig > 0)
      Max7219_send(MAX_SCAN_LIM, num_digs - 1);
    else
      Max7219_send(MAX_SHUTDOWN, 0);
  }

  volatile unsigned int num = 0;
  volatile int tmp;
  while (num_digs--) {
    tmp = data / 10;
    Max7219_send(++num, data - tmp * 10);
    data = tmp;
  }
  return data != 0;
}

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

  if (IsEnableSysTick()) DisableSysTick();
  // Interrupt every 1 second when enabled
  SysTick->LOAD = 0x989680 - 1;
  SysTick->VAL = 0;
  SelectSysTickSrc();
  EnableSysTickInt();
  // Set the priority of the SysTick Interrupt
  SCB->SHP[11] = 0xff;
}

/**********************************************************
 * Interrupt handler
 ***********************************************************/

volatile int isFinished = 1;
volatile int currentTime = 0;

void updateDisplay() {
  int time = currentTime;
  display(time, getLen(time));
}

void SysTick_Handler() {
  if (currentTime > 0) currentTime--;
  if (currentTime == 0) {
    DisableSysTick();
    updateDisplay();
    int i = 0, j = 0;
    while (!isFinished) {
      i = !i;
      i ? SetPin(BZ) : ResetPin(BZ);
      while (j++ < 1500)
        ;
      j = 0;
    }
  }
}

void EXTI15_10_IRQHandler() {
  LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_13);
  NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
  // PC13 => User Button
  if (isFinished) {
    if (currentTime > 0) {
      isFinished = 0;
      SysTick->VAL = 0;
      EnableSysTick();
    }
  } else {
    if (currentTime == 0) {
      isFinished = 1;
      DisableSysTick();
    }
  }
}

int main() {
  // Initialization
  __enable_irq();
  SystemClock_Config();
  GPIO_init_max7219();
  Max7219_init();
  GPIO_init_buzzer();
  GPIO_init_keypad();
  GPIO_init_btn();

  // Update display
  int pressed = 0;
  while (1) {
    if (isFinished) {
      int key = keypad_scan();
      if (key < 0) {
        pressed = 0;
      } else {
        if (!pressed) {
          currentTime = key;
          pressed = 1;
        }
      }
    }
    updateDisplay();
  }
  return 0;
}
