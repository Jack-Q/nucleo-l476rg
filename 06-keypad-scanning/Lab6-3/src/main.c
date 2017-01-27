/******************************************************
 * Simple calculator
 * -----------------
 * Scan keypad and build a simple calculator supporting
 * addition, substraction, multiplication and division,
 * with respect to the priority of multiplication and 
 * division. The inputed number will be limited to 
 * contain up to 3 digits. The calculation should also 
 * support negative integers.
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

#define SetPin        LL_GPIO_SetOutputPin
#define ResetPin      LL_GPIO_ResetOutputPin
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
 * return:
 * >= 0: key pressed value
 * -1: no key press
 */
#define DEBOUNCE    300
#define KEY_NULL    -1
#define KEY_EQUAL   10
#define KEY_ADD     11
#define KEY_SUB     12
#define KEY_MUL     13
#define KEY_DIV     14
#define KEY_CLEAR   15
int keypad_scan() {
  int times = DEBOUNCE;
  int keys = 0, data = 0, oldKeys = 0;
  while (times--) {
    keys = 0;

    SetPinOnly(X0);
    if (IsSetPin(Y0)) (keys |= 1 << 0), data = 1;          // 1
    if (IsSetPin(Y1)) (keys |= 1 << 1), data = 4;          // 4
    if (IsSetPin(Y2)) (keys |= 1 << 2), data = 7;          // 7
    if (IsSetPin(Y3)) (keys |= 1 << 3), data = KEY_EQUAL;  // *
    SetPinOnly(X1);
    if (IsSetPin(Y0)) (keys |= 1 << 4), data = 2;          // 2
    if (IsSetPin(Y1)) (keys |= 1 << 5), data = 5;          // 5
    if (IsSetPin(Y2)) (keys |= 1 << 6), data = 8;          // 8
    if (IsSetPin(Y3)) (keys |= 1 << 7), data = 0;          // 0
    SetPinOnly(X2);
    if (IsSetPin(Y0)) (keys |= 1 << 8), data = 3;          // 3
    if (IsSetPin(Y1)) (keys |= 1 << 9), data = 6;          // 6
    if (IsSetPin(Y2)) (keys |= 1 << 10), data = 9;         // 9
    if (IsSetPin(Y3)) (keys |= 1 << 11), data = KEY_CLEAR; // #
    SetPinOnly(X3);
    if (IsSetPin(Y0)) (keys |= 1 << 12), data = KEY_ADD;   // A
    if (IsSetPin(Y1)) (keys |= 1 << 13), data = KEY_SUB;   // B
    if (IsSetPin(Y2)) (keys |= 1 << 14), data = KEY_MUL;   // C
    if (IsSetPin(Y3)) (keys |= 1 << 15), data = KEY_DIV;   // D

    if (keys != oldKeys) times = DEBOUNCE;
    oldKeys = keys;
  }

  return keys ? data : KEY_NULL;
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
  if (data < 0) {
    data = -data;
    Max7219_send(num_digs, 10);
    num_digs--;
  }
  while (num_digs--) {
    tmp = data / 10;
    Max7219_send(++num, data - tmp * 10);
    data = tmp;
  }
  return data != 0;
}

/* Get the length required 
 * by a number to be displayed in 7-Seg LED */
int getLen(int number) {
  if(!number) return 1;
  int digit = 0;
  if (number < 0) digit++, number = -number;
  while (number > 0) digit++, number /= 10;
  return digit;
}

/* Calculate result of two number with given operator */
int calc(int a, int b, int op, int *e) {
  switch (op) {
    case KEY_ADD:        return a + b;
    case KEY_SUB:        return a - b;
    case KEY_MUL:        return a * b;
    case KEY_DIV: if (b) return a / b;
  }
  *e = 1; // divided by zero
  return 0;
}

int main() {
  /* Port & perpherial device initialization */
  GPIO_init();
  Max7219_init();
  keypad_init();

  /* Data initialization */
  int len = 0, data = 0;
  int keydown = 0, result = 0, err = 0;
  int dt0, dt1, op0, op1, pos = 0;

  /* Main loop of program */
  while (1) {
    display(data, len);
    int key = keypad_scan();
    if (key == KEY_NULL) {
      keydown = 0;
      continue;
    }

    // Prevent multiprocess to press and hold
    if (keydown) {
      continue;
    }
    keydown = 1;

    switch (key) {
      // Clear current state
      case KEY_CLEAR:
        err = data = result = len = pos = 0;
        break;


      // Calcluation and display final result
      case KEY_EQUAL:
        if (result) continue;

        if (!err)
          if (!len)
            err = 1;
          else if (pos == 2)
            data = calc(dt0, calc(dt1, data, op1, &err), op0, &err),
            len = getLen(data);
          else if (pos == 1)
            data = calc(dt0, data, op0, &err), len = getLen(data);

        if (len > 8) err = 1;

        if (err)
          data = -1, len = 2;
        else
          len = getLen(data);

        result = 1;
        break;


      // Addition & Substraction
      case KEY_ADD:
      case KEY_SUB:
        if (result) continue;

        if (!err)
          if (!len)
            err = 1;
          else if (pos == 2)
            dt0 = calc(dt0, calc(dt1, data, op1, &err), op0, &err), pos--,
            op0 = key;
          else if (pos == 1)
            dt0 = calc(dt0, data, op0, &err), op0 = key;
          else if (pos == 0)
            dt0 = data, op0 = key, pos = 1;

        len = 0, data = 0;
        break;


      // Multiplicaion & Division
      case KEY_MUL:
      case KEY_DIV:
        if (result) continue;

        if (!err)
          if (!len)
            err = 1;
          else if (pos == 2)
            dt1 = calc(dt1, data, op1, &err), op1 = key;
          else if (pos == 1)
            if (op0 == KEY_ADD || op0 == KEY_SUB)
              dt1 = data, op1 = key, pos = 2;
            else
              dt0 = calc(dt0, data, op0, &err), op0 = key;
          else if (pos == 0)
            dt0 = data, op0 = key, pos = 1;

        len = 0, data = 0;
        break;

      // General digits
      default:
        if (result || len >= 3) continue;
        data = data * 10 + key;
        len++;
    }
  }

  return 0;
}
