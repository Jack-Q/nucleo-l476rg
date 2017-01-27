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
	LL_TIM_SetPrescaler(timer,5);
	LL_TIM_SetAutoReload(timer, 20);
	LL_TIM_GenerateEvent_UPDATE(timer);
}
void channel_init(TIM_TypeDef *timer, uint32_t channel){
	LL_TIM_CC_EnableChannel(timer, channel);
	LL_TIM_OC_SetMode(timer, channel, LL_TIM_OCMODE_ASSYMETRIC_PWM1);
	//LL_TIM_OC_SetPolarity(timer, channel, 2000);
	LL_TIM_OC_SetCompareCH3(timer, 9);
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
 *     with debounce to duty cycle adjustment button
 * return:
 *   >= 0: frequency prescaler
 *   <0: key code
 */
#define CONVERT_FREQ(f) ((int)(4000000 / 20 / f) - 1)
#define DUTY_CYCLE_DESC -2
#define DUTY_CYCLE_INC -3
#define NO_KEY_PRESS -1
#define DEBOUNCE 5
int keypad_scan() {
  int times = DEBOUNCE;
  int keys = 0, data = 0, oldKeys = 0;
  while (times--) {
    keys = 0;
    SetPinOnly(X0);
    if (IsSetPin(Y0)) return CONVERT_FREQ(261.6);              // 1
    if (IsSetPin(Y1)) return CONVERT_FREQ(349.2);              // 4
    if (IsSetPin(Y2)) return CONVERT_FREQ(493.9);              // 7
    if (IsSetPin(Y3)) (keys |= 1 << 3), data = DUTY_CYCLE_DESC; // *
    SetPinOnly(X1);
    if (IsSetPin(Y0)) return CONVERT_FREQ(293.7);              // 2
    if (IsSetPin(Y1)) return CONVERT_FREQ(392.0);              // 5
    if (IsSetPin(Y2)) return CONVERT_FREQ(523.3);              // 8
    // if (IsSetPin(Y3)) (keys |= 1 << 7), data = 0;            // 0
    SetPinOnly(X2);
    if (IsSetPin(Y0)) return CONVERT_FREQ(329.6);              // 3
    if (IsSetPin(Y1)) return CONVERT_FREQ(440.0);              // 6
    // if (IsSetPin(Y2)) (keys |= 1 << 10), data = 9;           // 9
    if (IsSetPin(Y3)) (keys |= 1 << 11), data = DUTY_CYCLE_INC; // #
    SetPinOnly(X3);
    // if (IsSetPin(Y0)) (keys |= 1 << 12), data = 0;           // A
    // if (IsSetPin(Y1)) (keys |= 1 << 13), data = 0;           // B
    // if (IsSetPin(Y2)) (keys |= 1 << 14), data = 0;           // C
    // if (IsSetPin(Y3)) (keys |= 1 << 15), data = 0;           // D

    if (keys != oldKeys) times = DEBOUNCE;
    oldKeys = keys;
  }

  return keys ? data : NO_KEY_PRESS;
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
  int last = -1, key_down = 0, dutyCycle = 9;
  while (1) {
	int key = keypad_scan();
	switch(key){
	case DUTY_CYCLE_DESC:
		if(!key_down && dutyCycle > 0){
			LL_TIM_OC_SetCompareCH3(timer, dutyCycle--);
		}
		key_down = 1;
		break;
	case DUTY_CYCLE_INC:
		if(!key_down && dutyCycle < 19){
			LL_TIM_OC_SetCompareCH3(timer, dutyCycle++);
		}
		key_down = 1;
		break;
	case NO_KEY_PRESS:
		LL_TIM_DisableCounter(timer);
		key_down = 0, last = key;
		break;
	default:
		if(key!=last){
			LL_TIM_SetPrescaler(timer, key);
			LL_TIM_EnableCounter(timer);
		}
		key_down = 1, last = key;
		break;
	}
    for(int i=0; i < 1000;i++);
  }
  return 0;
}
