/******************************************************
 * Lab 8-2: Keypad External Interrupt
 * Implement keypad scanner via external interrupt
 * instead of while loop
 *******************************************************/
/* *****************************************************
 * Max7192 is connected to board via VCC(3.3V), GND,
 * as well as GPIO port: PB3, PB4, PB5
 *  PB3: Data
 *  PB4: CLK
 *  PB5: CS
 *******************************************************/
#define STM32L476XX

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stm32l476xx.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_exti.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_rcc.h"

// GPIO Port connection to Keypad
#define X0 GPIOB, LL_GPIO_PIN_6
#define X1 GPIOA, LL_GPIO_PIN_7
#define X2 GPIOA, LL_GPIO_PIN_6
#define X3 GPIOA, LL_GPIO_PIN_5
#define Y0 GPIOB, LL_GPIO_PIN_10
#define Y1 GPIOA, LL_GPIO_PIN_8
#define Y2 GPIOA, LL_GPIO_PIN_9
#define Y3 GPIOC, LL_GPIO_PIN_7

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

/* External label defined in Assembly */
extern void GPIO_init();
extern void Max7219_init();
extern void Max7219_send(unsigned char address, unsigned char data);

/* Initialization of GPIO port connected to keypad */
void keypad_init() {
  /* RCC AHB2 Bus enable*/
  RCC->AHB2ENR |=
      RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;

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

/****************************************************
 * Lab 8-2 Implementation
 ****************************************************/
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
  SysTick->LOAD = 0xf4240 - 1;  // 0.1 sec (1MHz)
  SysTick->VAL = 0;
  SelectSysTickSrc();
  EnableSysTickInt();
  EnableSysTick();
  // Set the priority of SysTick
  SCB->SHP[11] = 0xff;
}

void EXTI_Setup() {
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  /********************************
   * GPIO Ports (Flip)
   *     X-0 X-1 X-2 X-3
   *   ___V___V___V___V___
   *  |   1   2   3   A   | <- Y-0
   *  |   4   5   6   B   | <- Y-1
   *  |   7   8   9   C   | <- Y-2
   *  |   *   0   #   D   | <- Y-3
   *   --|-|-|-|-|-|-|-|--
   *     | | | | | | | |
   *   PB10|PA9|PB6|PA6|
   *      PA8 PC7 PA7 PA6
   *        Y   |   X
   *     0 1 2 3|0 1 2 3
   ***********************************/
  // The input pin are set to PB10(Y0) PA8(Y1) PA9(Y2) PC7(Y3)
  // PC7
  MODIFY_REG(SYSCFG->EXTICR[1], SYSCFG_EXTICR2_EXTI7_Msk,
             SYSCFG_EXTICR2_EXTI7_PC);
  // PA8
  MODIFY_REG(SYSCFG->EXTICR[2], SYSCFG_EXTICR3_EXTI8_Msk,
             SYSCFG_EXTICR3_EXTI8_PA);
  // PA9
  MODIFY_REG(SYSCFG->EXTICR[2], SYSCFG_EXTICR3_EXTI9_Msk,
             SYSCFG_EXTICR3_EXTI9_PA);
  // PB10
  MODIFY_REG(SYSCFG->EXTICR[2], SYSCFG_EXTICR3_EXTI10_Msk,
             SYSCFG_EXTICR3_EXTI10_PB);

  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_7);            // EXTI_IMR
  LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_7);   // EXTI_FTSR
  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_8);            // EXTI_IMR
  LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_8);   // EXTI_FTSR
  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_9);            // EXTI_IMR
  LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_9);   // EXTI_FTSR
  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_10);           // EXTI_IMR
  LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_10);  // EXTI_FTSR

  // NVIC_SystemReset();
  NVIC_EnableIRQ(EXTI9_5_IRQn);
  NVIC_EnableIRQ(EXTI15_10_IRQn);
  NVIC_SetPriority(EXTI9_5_IRQn, 0);
  NVIC_SetPriority(EXTI15_10_IRQn, 0);
}

volatile int scan_col = 0;
volatile int key_value = 0;
volatile int update = 0;
// Key code map
static int ref[4][4] = {
    {1, 2, 3, 10}, {4, 5, 6, 11}, {7, 8, 9, 12}, {15, 0, 14, 13}};
__inline void row_trigger(int row);
void EXTI15_10_IRQHandler() {
  NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
  if (LL_EXTI_ReadFlag_0_31(LL_EXTI_LINE_10)) {
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_10);
    // PB10 => Y0
    row_trigger(0);
  }
}

void EXTI9_5_IRQHandler() {
  NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
  if (LL_EXTI_ReadFlag_0_31(LL_EXTI_LINE_7)) {
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_7);
    // PC7 => Y3
    row_trigger(3);
  } else if (LL_EXTI_ReadFlag_0_31(LL_EXTI_LINE_8)) {
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_8);
    // PA8 => Y1
    row_trigger(1);
  } else if (LL_EXTI_ReadFlag_0_31(LL_EXTI_LINE_9)) {
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_9);
    // PA9 => Y2
    row_trigger(2);
  }
}
void row_trigger(int row) {
  if (update)
    key_value = ref[row][scan_col];
//  else {
//    int r = -1;
//    if (IsSetPin(Y0))
//      r = 0;
//    else if (IsSetPin(Y1))
//      r = 1;
//    else if (IsSetPin(Y2))
//      r = 2;
//    else if (IsSetPin(Y3))
//      r = 3;
//    if (r >= 0) key_value = ref[r][scan_col == 3 ? 0 : scan_col + 1];
//  }
}

void SysTick_Handler() {
  update = 1;
  if (++scan_col > 3) scan_col = 0;
  switch (scan_col) {
    case 0:
      ResetPin(X0);
      SetPin(X1);
      break;
    case 1:
      ResetPin(X1);
      SetPin(X2);
      break;
    case 2:
      ResetPin(X2);
      SetPin(X3);
      break;
    case 3:
      ResetPin(X3);
      SetPin(X0);
      break;
  }
  int i = 100;
  while (i--)
    ;
  update = 0;
}

int main() {
  SystemClock_Config();
  GPIO_init();
  EXTI_Setup();
  Max7219_init();
  keypad_init();

  while (1) {
    // keep refreshing the content displayed on 7-Segment LCD
    display(key_value, 2);
  }
}
