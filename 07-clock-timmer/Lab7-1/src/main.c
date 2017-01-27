

#define STM32L476xx

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stm32l476xx.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_rcc.h"

extern void GPIO_init();
extern void delay_1s();

#define SYSTEM_CLOCK_MODE_COUNT 5
enum SystemClock{
	SYSTEM_CLOCK_1 = 0,
	SYSTEM_CLOCK_6 = 1,
	SYSTEM_CLOCK_10 = 2,
	SYSTEM_CLOCK_16 = 3,
	SYSTEM_CLOCK_40 = 4,
};

enum SystemClock sys_clock_mode = SYSTEM_CLOCK_16;

// PLL Input ==[/PLLM]=> VCO Input ==[*PLLN]=> VCO Output ==[/PLLR]=> PLL Output
//              1 ~ 8   4 ~ 16 MHz    8 ~ 86   64 ~ 344 MHz  2,4,6,8

void SystemClock_Config(){
	switch(sys_clock_mode){
	case SYSTEM_CLOCK_1:
		LL_RCC_MSI_EnableRangeSelection(); // or: RCC->CR |= RCC_CR_MSIRGSEL;
		LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_4); // Range mode 4: 0100, 1MHz
		while(!LL_RCC_MSI_IsReady()); // Wait till ready
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);
		break;
	case SYSTEM_CLOCK_6:
		LL_RCC_MSI_EnableRangeSelection();
		LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_9); // Range mode 9: 01001, 24MHz
		while(!LL_RCC_MSI_IsReady());
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

		LL_RCC_PLL_Disable();
		while(LL_RCC_PLL_IsReady());
		// 6 = 24 * ( 8 / 4 ) / 8
		LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_4, 8, LL_RCC_PLLR_DIV_8);
		LL_RCC_PLL_Enable();
		LL_RCC_PLL_EnableDomain_SYS();
		while(!LL_RCC_PLL_IsReady());

		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
		break;
	case SYSTEM_CLOCK_10:
		LL_RCC_MSI_EnableRangeSelection();
		LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_9); // Range mode 9: 1001, 24MHz
		while(!LL_RCC_MSI_IsReady());
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

		LL_RCC_PLL_Disable();
		while(LL_RCC_PLL_IsReady());
		// 10 = 24 * ( 10 / 3 ) / 8
		LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_3, 10, LL_RCC_PLLR_DIV_8);
		LL_RCC_PLL_Enable();
		LL_RCC_PLL_EnableDomain_SYS();
		while(!LL_RCC_PLL_IsReady());

		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
		break;
	case SYSTEM_CLOCK_16:
		LL_RCC_MSI_EnableRangeSelection(); // or: RCC->CR |= RCC_CR_MSIRGSEL;
		LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_8); // Range mode 8: 1000, 16MHz
		while(!LL_RCC_MSI_IsReady()); // Wait till ready
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);
		break;
	case SYSTEM_CLOCK_40:
		LL_RCC_MSI_EnableRangeSelection();
		LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_9); // Range mode 7: 1001, 24MHz
		while(!LL_RCC_MSI_IsReady());
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

		LL_RCC_PLL_Disable();
		while(LL_RCC_PLL_IsReady());
		// 40 = 24 * ( 20 / 3 ) / 4
		LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_3, 20, LL_RCC_PLLR_DIV_4);
		LL_RCC_PLL_Enable();
		while(!LL_RCC_PLL_IsReady());
		LL_RCC_PLL_EnableDomain_SYS();

		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
		break;
	}
}

#define PUSH_BUTTON_DEBOUNCE 100
int user_press_button(){
	static int sState = 0, sTimes = PUSH_BUTTON_DEBOUNCE;
	int t = 400;
	while(t--){
		if(!(GPIOC->IDR & GPIO_IDR_ID13)){
			if(!sState) sTimes--;
			if(!sTimes){
				sState = 1, sTimes = PUSH_BUTTON_DEBOUNCE;
				return 1;
			}
		}else{
			sState = 0, sTimes = PUSH_BUTTON_DEBOUNCE;
		}
	}
	return 0;
}

int main(){
	SystemClock_Config();
	GPIO_init();
	
	while(1){
		if(user_press_button()){
			if(sys_clock_mode == SYSTEM_CLOCK_40)
				sys_clock_mode = SYSTEM_CLOCK_1;
			else
				sys_clock_mode++;
			SystemClock_Config();
		}
		GPIOA->BSRR = 1 << 5;
		delay_1s();
		GPIOA->BRR = 1 << 5;
		delay_1s();
	}
}
