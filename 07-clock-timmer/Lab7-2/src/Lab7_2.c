#define STM32L476xx

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stm32l476xx.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_tim.h"

#define TIME_SEC 12.70
extern void GPIO_init();
extern void Max7219_init();
extern int Display(int,int);
void Timer_init(TIM_TypeDef *timer){
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM5);

	LL_TIM_SetCounterMode(timer, LL_TIM_COUNTERMODE_UP);
	LL_TIM_SetAutoReload(timer, 1270);
	LL_TIM_SetPrescaler(timer, 39999);
	LL_TIM_EnableARRPreload(timer);
	LL_TIM_SetOnePulseMode(timer, LL_TIM_ONEPULSEMODE_SINGLE);
	LL_TIM_GenerateEvent_UPDATE(timer);
	LL_TIM_DisableUpdateEvent(timer);
	// LL_TIM_SetCounter(timer, 1250);
}

void Timer_start(TIM_TypeDef *timer){
	LL_TIM_EnableCounter(timer);

	while(1){
		int time = LL_TIM_GetCounter(timer);

		if(time == (int)(TIME_SEC * 100)) {
			LL_TIM_DisableCounter(timer);
		}

		int length  = 	 (time>9999999)?8:
						 (time> 999999)?7:
						 (time>  99999)?6:
						 (time>   9999)?5:
						 (time>    999)?4:3;
			#define MAX_SCAN_LIM 0x0B
			#define MAX_SHUTDOWN 0x0C
		  // use static data to decrease reset process of scan limit
		  static int dig = -1;
		  int arr[10]={0x7e,0x30,0x6d,0x79,0x33,
		  			   0x5b,0x5f,0x70,0x7f,0x7b};
		  if (length != dig) {
		    if (!dig) Display(MAX_SHUTDOWN, 1);
		    dig = length;
		    if (dig > 0)
		    	Display(MAX_SCAN_LIM, length - 1);
		    else
		    	Display(MAX_SHUTDOWN, 0);
		  }

		  register unsigned int num = 0;
		  register int tmp;
		  while (length--) {
		    tmp = time / 10;
		    num++;
		    if(num==3){
		    	Display(num,arr[time - tmp * 10]+0x80);
		    }
		    else
		    	Display(num, arr[time - tmp * 10]);
		    time = tmp;
		  }
	}
}



int main(){
	GPIO_init();
	Max7219_init();
	Timer_init(TIM5);
	Timer_start(TIM5);
	while(1){

	}
}
