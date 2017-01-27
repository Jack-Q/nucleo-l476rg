/******************************************************
 * Keypad Scanning
 * -----------------
 * Scan keypad and display the value represented by
 * current pressed key.
 * Implemented in C language.
 * -----------------
 * This implementation imported the LL driver from
 * STM32, refer to www.st.com/stm32cubefw .
 * Buzzer connected to PB8
 ******************************************************/

#define STM32L476xx

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stm32l476xx.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_tim.h"


#define X0 GPIOC, LL_GPIO_PIN_7
#define X1 GPIOA, LL_GPIO_PIN_9
#define X2 GPIOA, LL_GPIO_PIN_8
#define X3 GPIOB, LL_GPIO_PIN_10
#define Y0 GPIOA, LL_GPIO_PIN_5
#define Y1 GPIOA, LL_GPIO_PIN_6
#define Y2 GPIOA, LL_GPIO_PIN_7
#define Y3 GPIOB, LL_GPIO_PIN_6

#define BZ GPIOB, LL_GPIO_PIN_8

#define SetPin        LL_GPIO_SetOutputPin
#define ResetPin      LL_GPIO_ResetOutputPin
#define ResetPinAll() ResetPin(X0); ResetPin(X1); ResetPin(X2); ResetPin(X3)
#define SetPinOnly    ResetPinAll(); SetPin
#define IsSetPin      LL_GPIO_IsInputPinSet

/* External label defined in Assembly */
extern void GPIO_init();

void GPIO_init_AF(){
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

	LL_GPIO_SetPinMode(BZ, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinSpeed(BZ, LL_GPIO_SPEED_HIGH);
	LL_GPIO_SetPinOutputType(BZ, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinPull(BZ, LL_GPIO_PULL_NO);

	LL_GPIO_SetAFPin_8_15(BZ, LL_GPIO_AF_2);
}
void Timer_init(TIM_TypeDef *timer){
	LL_TIM_SetOnePulseMode(timer, LL_TIM_ONEPULSEMODE_REPETITIVE);
	LL_TIM_SetCounterMode(timer, LL_TIM_COUNTERMODE_CENTER_UP);
	// LL_TIM_SetPrescaler(timer,30);
	LL_TIM_GenerateEvent_UPDATE(timer);
}
void channel_init(TIM_TypeDef *timer, uint32_t channel){
	LL_TIM_CC_EnableChannel(timer, channel);
	LL_TIM_OC_SetMode(timer, channel, LL_TIM_OCMODE_TOGGLE);
	LL_TIM_OC_SetCompareCH3(timer, 0);
	LL_TIM_OC_SetPolarity(timer, channel, 2000);
}


/* Initialization of GPIO port connected to keypad */
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
#define CONVERT_FREQ(f) ((int)(4000000 / 2 / f))
int keypad_scan() {
  SetPinOnly(X0);
  if (IsSetPin(Y0))		return CONVERT_FREQ(261.6);
  if (IsSetPin(Y1))		return CONVERT_FREQ(349.2);
  if (IsSetPin(Y2))	    return CONVERT_FREQ(493.9);
  //if (IsSetPin(Y3))     return 15; // *
  SetPinOnly(X1);
  if (IsSetPin(Y0))		return CONVERT_FREQ(293.7);
  if (IsSetPin(Y1))	    return CONVERT_FREQ(392.0);
  if (IsSetPin(Y2))		return CONVERT_FREQ(523.3);
 // if (IsSetPin(Y3))     return 0;
  SetPinOnly(X2);
  if (IsSetPin(Y0))     return CONVERT_FREQ(329.6);
  if (IsSetPin(Y1))     return CONVERT_FREQ(440.0);
  // if (IsSetPin(Y2))     return 9;
  // if (IsSetPin(Y3))     return 14; // #
  //SetPinOnly(X3);
  //if (IsSetPin(Y0))     return 10; // A
  //if (IsSetPin(Y1))     return 11; // B
  //if (IsSetPin(Y2))     return 12; // C
  //if (IsSetPin(Y3))     return 13; // D
  return -1;
}

int main() {
  GPIO_init();
  GPIO_init_AF();
  keypad_init();
  // Timer Initialization
  TIM_TypeDef* timer = TIM4;
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);
  Timer_init(timer);
  channel_init(timer, LL_TIM_CHANNEL_CH3);
  // main loop
  int last = -1;
  while (1) {
	int t = keypad_scan();
	if(t!=last){
		if(t > 0){
			LL_TIM_SetAutoReload(timer, t);
			LL_TIM_EnableCounter(timer);
		}else{
			LL_TIM_DisableCounter(timer);
		}
	}
	last = t;
    for(int i=0; i < 1000;i++);
  }
  return 0;
}
