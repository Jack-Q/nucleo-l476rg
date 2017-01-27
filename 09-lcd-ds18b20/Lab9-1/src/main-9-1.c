#define STM32L476XX

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stm32l476xx.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_rcc.h"

#define SetPin LL_GPIO_SetOutputPin
#define ResetPin LL_GPIO_ResetOutputPin
#define IsSetPin LL_GPIO_IsInputPinSet

#define RS GPIOB, LL_GPIO_PIN_3
#define RW GPIOB, LL_GPIO_PIN_5
#define E GPIOB, LL_GPIO_PIN_4
#define DB0 GPIOB, LL_GPIO_PIN_10
#define DB1 GPIOA, LL_GPIO_PIN_8
#define DB2 GPIOA, LL_GPIO_PIN_9
#define DB3 GPIOC, LL_GPIO_PIN_7
#define DB4 GPIOB, LL_GPIO_PIN_6
#define DB5 GPIOA, LL_GPIO_PIN_7
#define DB6 GPIOA, LL_GPIO_PIN_6
#define DB7 GPIOA, LL_GPIO_PIN_5

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
  SysTick->LOAD = 0x2dc6c0;  // 10_000_000 * 0.3
  SysTick->VAL = 0;
  SelectSysTickSrc();
  EnableSysTickInt();
  EnableSysTick();
}

void GPIO_init_pin(GPIO_TypeDef *GPIOx, uint32_t PinMask) {
  LL_GPIO_SetPinMode(GPIOx, PinMask, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(GPIOx, PinMask, LL_GPIO_SPEED_HIGH);
  LL_GPIO_SetPinOutputType(GPIOx, PinMask, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOx, PinMask, LL_GPIO_PULL_DOWN);
  ResetPin(GPIOx, PinMask);
}

void GPIO_init() {
  RCC->AHB2ENR |=
      RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;
  GPIO_init_pin(RS);
  GPIO_init_pin(RW);
  GPIO_init_pin(E);
  GPIO_init_pin(DB0);
  GPIO_init_pin(DB1);
  GPIO_init_pin(DB2);
  GPIO_init_pin(DB3);
  GPIO_init_pin(DB4);
  GPIO_init_pin(DB5);
  GPIO_init_pin(DB6);
  GPIO_init_pin(DB7);
}

void delay(){
	int i = 500;
	while(i--);
}
void LCD_send() {
	SetPin(E);    while (!IsSetPin(E));
	delay();
	ResetPin(E);  while (IsSetPin(E));
}
void LCD_port_set_read(){
	LL_GPIO_SetPinMode(DB0, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(DB1, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(DB2, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(DB3, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(DB4, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(DB5, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(DB6, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(DB7, LL_GPIO_MODE_INPUT);
}
void LCD_port_set_write(){
	LL_GPIO_SetPinMode(DB0, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(DB1, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(DB2, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(DB3, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(DB4, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(DB5, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(DB6, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(DB7, LL_GPIO_MODE_OUTPUT);
}
int LCD_read(int cmd) {
	if(cmd) ResetPin(RS); else SetPin(RS);
	SetPin(RW);
	SetPin(E);    while (!IsSetPin(E));
	delay();
	int data = 0;
	data = data << 1 | (IsSetPin(DB7) ? 1 : 0);
	data = data << 1 | (IsSetPin(DB6) ? 1 : 0);
	data = data << 1 | (IsSetPin(DB5) ? 1 : 0);
	data = data << 1 | (IsSetPin(DB4) ? 1 : 0);
	data = data << 1 | (IsSetPin(DB3) ? 1 : 0);
	data = data << 1 | (IsSetPin(DB2) ? 1 : 0);
	data = data << 1 | (IsSetPin(DB1) ? 1 : 0);
	data = data << 1 | (IsSetPin(DB0) ? 1 : 0);
	ResetPin(E);  while (IsSetPin(E));
	return data;
}
int WriteToLCD(int input, int Cmd) {
	// write command to LCD or let LCD display character fetched from memory
	LCD_port_set_read();
	while(LCD_read(1) & 0x80);
	LCD_port_set_write();
	if(Cmd) ResetPin(RS); else SetPin(RS);
	ResetPin(RW);

	input & 1 ? SetPin(DB0) : ResetPin(DB0);input >>= 1;
	input & 1 ? SetPin(DB1) : ResetPin(DB1);input >>= 1;
	input & 1 ? SetPin(DB2) : ResetPin(DB2);input >>= 1;
	input & 1 ? SetPin(DB3) : ResetPin(DB3);input >>= 1;
	input & 1 ? SetPin(DB4) : ResetPin(DB4);input >>= 1;
	input & 1 ? SetPin(DB5) : ResetPin(DB5);input >>= 1;
	input & 1 ? SetPin(DB6) : ResetPin(DB6);input >>= 1;
	input & 1 ? SetPin(DB7) : ResetPin(DB7);input >>= 1;
	LCD_send();
	return 0;
}
void LCD_cursor_set(int pos){
	WriteToLCD(0x80 | pos, 1);
}
void init_LCD() {
  // LCD register
  WriteToLCD(56, 1);  // Function Setting
  WriteToLCD( 6, 1);  // Entering mode
  WriteToLCD(12, 1);  // Display on
  WriteToLCD( 1, 1);  // Clear Screen
  WriteToLCD( 2, 1);  // Move to top left
}

volatile int pos;

void SysTick_Handler() {
  pos++;
  if (pos == 0x10) pos = 0x40;
  if (pos == 0x50) pos = 0x00;
}

int main() {
  SystemClock_Config();
  GPIO_init();
  init_LCD();
  int i = 0; pos = -1;
  while (1) {
    if (i != pos) {
    	i = pos;
    	switch(i){
        case 0x00: LCD_cursor_set(0x4f); break;
        case 0x40: LCD_cursor_set(0x0f); break;
        default: LCD_cursor_set(i - 1); break;
        }
        WriteToLCD(0x06, 1);   // 0b0000_0110, setting mode, increment, no shift
        WriteToLCD(0x20, 0); // 0b0011_0100
    	LCD_cursor_set(i);
        WriteToLCD(0x06, 1);   // 0b0000_0110, setting mode, increment, no shift
        WriteToLCD(0x34, 0); // 0b0011_0100
        switch(i){
        case 0x0f: LCD_cursor_set(0x40); WriteToLCD(0x06, 1); break;
        case 0x4f: LCD_cursor_set(0x00); WriteToLCD(0x06, 1); break;
        }
    	WriteToLCD(0x39, 0); // 0b0011_1001
    }
  }
}
