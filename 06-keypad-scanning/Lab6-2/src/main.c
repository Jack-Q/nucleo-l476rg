/******************************************************
 * Single/mutiple key press handling
 * -----------------
 * Scan keypad and display the value represented by 
 * current pressed key. Support multiple key press
 * simtaneously.
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
 *  |   1   2   3   A   | -> Y-0
 *  |   4   5   6   B   | -> Y-1
 *  |   7   8   9   C   | -> Y-2
 *  |   *   0   #   D   | -> Y-3
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

/* change the pin mode during scanning to shrink
 * the voltage decy on the keypad, which can improve
 * the responsiveness of keypad in the scanerio of 
 * multiple simutaneous key press .
 */
__STATIC_INLINE void GPIO_SetOutputPin(
    GPIO_TypeDef *GPIOx, 
    uint32_t PinMask) {
  LL_GPIO_SetPinMode(GPIOx, PinMask, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinPull(GPIOx, PinMask, LL_GPIO_PULL_UP);
  WRITE_REG(GPIOx->BSRR, PinMask);
}
__STATIC_INLINE void GPIO_ResetOutputPin(
    GPIO_TypeDef *GPIOx,
    uint32_t PinMask) {
  WRITE_REG(GPIOx->BRR, PinMask);
  LL_GPIO_SetPinMode(GPIOx, PinMask, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinPull(GPIOx, PinMask, LL_GPIO_PULL_DOWN);
}

#define SetPin        GPIO_SetOutputPin
#define ResetPin      GPIO_ResetOutputPin
#define ResetPinAll() ResetPin(X0); ResetPin(X1); ResetPin(X2); ResetPin(X3)
#define SetPinOnly    ResetPinAll(); SetPin
#define IsSetPin      LL_GPIO_IsInputPinSet

extern void GPIO_init();
extern void Max7219_init();

void keypad_init() {
  /* RCC AHB2 Bus enable*/
  RCC->AHB2ENR |=
      RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;

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
  ll_gpio_init.Pull = LL_GPIO_PULL_UP;

  ll_gpio_init.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
  LL_GPIO_Init(GPIOA, &ll_gpio_init);
  ll_gpio_init.Pin = LL_GPIO_PIN_10;
  LL_GPIO_Init(GPIOB, &ll_gpio_init);
  ll_gpio_init.Pin = LL_GPIO_PIN_7;
  LL_GPIO_Init(GPIOC, &ll_gpio_init);
}

/* scan keypad value
 *   This version of implementation add debounce function
 *   This version also detects multiple key press
 * return:
 * >= 0: key pressed value
 * -1: no key press
 */
#define DEBOUNCE 150
int keypad_scan() {
  int times = DEBOUNCE;
  int keys = 0, data = 0, oldKeys = 0, clear = 0;
  while (times--) {
    data = 0, clear = 0, keys = 0;

    SetPinOnly(X0);
    if (IsSetPin(Y0)) (keys |= 1 << 0), data += 1;   // 1
    if (IsSetPin(Y1)) (keys |= 1 << 1), data += 4;   // 4
    if (IsSetPin(Y2)) (keys |= 1 << 2), data += 7;   // 7
    if (IsSetPin(Y3)) (keys |= 1 << 3), clear = 1;   // *
    SetPinOnly(X1);
    if (IsSetPin(Y0)) (keys |= 1 << 4), data += 2;   // 2
    if (IsSetPin(Y1)) (keys |= 1 << 5), data += 5;   // 5
    if (IsSetPin(Y2)) (keys |= 1 << 6), data += 8;   // 8
    if (IsSetPin(Y3)) (keys |= 1 << 7), data += 0;   // #
    SetPinOnly(X2);
    if (IsSetPin(Y0)) (keys |= 1 << 8), data += 3;   // 3
    if (IsSetPin(Y1)) (keys |= 1 << 9), data += 6;   // 6
    if (IsSetPin(Y2)) (keys |= 1 << 10), data += 9;  // 9
    if (IsSetPin(Y3)) (keys |= 1 << 11), clear = 1;  // #
    SetPinOnly(X3);
    if (IsSetPin(Y0)) (keys |= 1 << 12), data += 10; // A
    if (IsSetPin(Y1)) (keys |= 1 << 13), data += 11; // B
    if (IsSetPin(Y2)) (keys |= 1 << 14), data += 12; // C
    if (IsSetPin(Y3)) (keys |= 1 << 15), data += 13; // D

    if (keys != oldKeys) times = DEBOUNCE;
    oldKeys = keys;
  }

  return keys ? (clear ? -1 : data) : -2;
}

extern void Max7219_send(unsigned char address, unsigned char data);

/**
 * Show data on 7-seg via max7219_send
 * Input:
 *   data: decimal value
 *   num_digs: number of digits will show on 7-seg
 * Return:
 *   0: success
 *   -1: illegal data range (out of 8 digits range)
 */
#define MAX_SCAN_LIM 0x0B
#define MAX_SHUTDOWN 0x0C
int display(int data, int num_digs) {
  // use static data to decrease reset process of scan limit
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

  register unsigned int num = 0;
  register int tmp;
  while (num_digs--) {
    tmp = data / 10;
    Max7219_send(++num, data - tmp * 10);
    data = tmp;
  }
  return data != 0;
}


#define LONG_PRESS 7
int main() {
  GPIO_init();
  Max7219_init();
  keypad_init();
  int length = 0, data = 0;
  int keydown = 0;
  while (1) {
    display(data, length);
    int key = keypad_scan();
    if (key == -2) {
      keydown = 0;
      continue;
    }

    // Prevent multiply process to press and hold
    if (keydown) {
      if(keydown++ < LONG_PRESS)
    	  continue;
    }
    keydown = 1;

    if (key == -1) {
      length = 0;
      data = 0;
      continue;
    }
    int keylen = key > 9 ? 2 : 1;
    if (length + keylen > 8) continue;
    length += keylen;
    data = data * (keylen == 2 ? 100 : 10) + key;
  }
  return 0;
}
