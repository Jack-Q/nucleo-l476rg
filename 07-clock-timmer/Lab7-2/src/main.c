//
//
//#define STM32L476xx
//
//#ifndef USE_FULL_LL_DRIVER
//#define USE_FULL_LL_DRIVER
//#endif
//
//#include "stm32l476xx.h"
//#include "stm32l4xx_ll_bus.h"
//#include "stm32l4xx_ll_gpio.h"
//#include "stm32l4xx_ll_tim.h"
//
//
//void Timer_init(TIM_TypeDef *timer){
//	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM5);
//
//	LL_TIM_SetCounterMode(timer, LL_TIM_COUNTERMODE_UP);
//	LL_TIM_SetAutoReload(timer, 0);
//	LL_TIM_SetCounter()
//	LL_TIM_SetCounter(timer, 0xf4240);
//	LL_TIM_SetPrescaler(timer, 4);
//
//	LL_TIM_GenerateEvent_UPDATE(timer);
//}
//
//void Timer_start(TIM_TypeDef *timer){
//	LL_TIM_EnableCounter(timer);
//
//	while(1){
//		uint32_t time = LL_TIM_GetCounter(timer);
//		// TODO: Display
//	}
//}
//
//int main()
//{
//	// GPIO_init();
//	// max7219_init();
//	Timer_init(TIM5);
//	Timer_start(TIM5);
//	while(1){
//
//	}
//	return 0;
//}
