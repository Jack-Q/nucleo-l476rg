/******************************************************
 * Keypad Scanning
 * -----------------
 * Scan keypad and display the value represented by 
 * current pressed key. 
 * Implemented in C language.
 * -----------------
 * This implementation imported the LL driver from 
 * STM32, refer to www.st.com/stm32cubefw .
 ******************************************************/

#define STM32L476xx

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stm32l476xx.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_gpio.h"

/********************************
 * GPIO Ports
 *     X-0 X-1 X-2 X-3
 *   ___V___V___V___V___
 *  |   1   2   3   A   | <- Y-0
 *  |   4   5   6   B   | <- Y-1
 *  |   7   8   9   C   | <- Y-2
 *  |   *   0   #   D   | <- Y-3
 *   --|-|-|-|-|-|-|-|--
 *     | | | | | | | |
 *    PA5|PA7|PC7|PA8|
 *     PA6 PB6 PA9 PB10
 *        Y   |   X
 *     0 1 2 3|0 1 2 3
 ***********************************/
#define X0 GPIOC, LL_GPIO_PIN_7
#define X1 GPIOA, LL_GPIO_PIN_9
#define X2 GPIOA, LL_GPIO_PIN_8
#define X3 GPIOB, LL_GPIO_PIN_10
#define Y0 GPIOA, LL_GPIO_PIN_5
#define Y1 GPIOA, LL_GPIO_PIN_6
#define Y2 GPIOA, LL_GPIO_PIN_7
#define Y3 GPIOB, LL_GPIO_PIN_6

#define SetPin        LL_GPIO_SetOutputPin
#define ResetPin      LL_GPIO_ResetOutputPin
#define ResetPinAll() ResetPin(X0); ResetPin(X1); ResetPin(X2); ResetPin(X3)
#define SetPinOnly    ResetPinAll(); SetPin
#define IsSetPin      LL_GPIO_IsInputPinSet

/* External label defined in Assembly */
extern void GPIO_init();
extern void Max7219_init();

/* Initialization of GPIO port connected to keypad */
void keypad_init() {
  /* RCC AHB2 Bus enable*/
  RCC->AHB2ENR |=
      RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;

  /* GPIO mode register */
  // Alternate method:
  //	GPIOA->MODER &= ~(GPIO_MODER_MODE5 | GPIO_MODER_MODE6 | GPIO_MODER_MODE7
  //| GPIO_MODER_MODE8 | GPIO_MODER_MODE9 | GPIO_MODER_MODE10);
  //	GPIOA->MODER |= GPIO_MODER_MODE5_0 | GPIO_MODER_MODE6_0 |
  // GPIO_MODER_MODE7_0 | GPIO_MODER_MODE8_1 | GPIO_MODER_MODE9_1 |
  // GPIO_MODER_MODE10_1;
  //

  //
  LL_GPIO_InitTypeDef ll_gpio_init;

  // Set input mode
  LL_GPIO_StructInit(&ll_gpio_init);

  ll_gpio_init.Mode = LL_GPIO_MODE_INPUT;
  ll_gpio_init.Pull = LL_GPIO_PULL_DOWN;

  ll_gpio_init.Pin = LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
  LL_GPIO_Init(GPIOA, &ll_gpio_init);
  ll_gpio_init.Pin = LL_GPIO_PIN_6;
  LL_GPIO_Init(GPIOB, &ll_gpio_init);

  // Set output mode
  LL_GPIO_StructInit(&ll_gpio_init);

  ll_gpio_init.Mode = LL_GPIO_MODE_OUTPUT;
  ll_gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  ll_gpio_init.Speed = LL_GPIO_SPEED_HIGH;
  ll_gpio_init.Pull = LL_GPIO_PULL_NO;

  ll_gpio_init.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
  LL_GPIO_Init(GPIOA, &ll_gpio_init);
  ll_gpio_init.Pin = LL_GPIO_PIN_10;
  LL_GPIO_Init(GPIOB, &ll_gpio_init);
  ll_gpio_init.Pin = LL_GPIO_PIN_7;
  LL_GPIO_Init(GPIOC, &ll_gpio_init);
}

/* scan keypad value
 *   this simple implementation have no preventaion to
 *   the bounce problem of the button
 * return:
 *   >= 0: key pressed value
 *   -1: no key press
 */
int keypad_scan() {
  SetPinOnly(X0);
  if (IsSetPin(Y0))		return 1;
  if (IsSetPin(Y1))		return 4;
  if (IsSetPin(Y2))	    return 7;
  if (IsSetPin(Y3))     return 15; // *
  SetPinOnly(X1);
  if (IsSetPin(Y0))		return 2;
  if (IsSetPin(Y1))	    return 5;
  if (IsSetPin(Y2))		return 8;
  if (IsSetPin(Y3))     return 0;
  SetPinOnly(X2);
  if (IsSetPin(Y0))     return 3;
  if (IsSetPin(Y1))     return 6;
  if (IsSetPin(Y2))     return 9;
  if (IsSetPin(Y3))     return 14; // #
  SetPinOnly(X3);
  if (IsSetPin(Y0))     return 10; // A
  if (IsSetPin(Y1))     return 11; // B
  if (IsSetPin(Y2))     return 12; // C
  if (IsSetPin(Y3))     return 13; // D
  return -1;
}

extern void Max7219_send(unsigned char address, unsigned char data);

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
  if (data > 99999999 || data < -9999999)
    return -1;
  if (num_digs > 8 || num_digs < 0)
    return -1;
  if (num_digs != dig) {
    if (!dig)
      Max7219_send(MAX_SHUTDOWN, 1);
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

int main() {
  GPIO_init();
  Max7219_init();
  keypad_init();
  while (1) {
    int key = keypad_scan();
    if (key >= 0) {
      display(key, key >= 10 ? 2 : 1);
    } else {
      display(0, 0);
    }
  }
  return 0;
}
