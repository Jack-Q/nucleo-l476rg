#define STM32L476XX

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "string.h"
#include "stdio.h"
#include "stm32l476xx.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_rcc.h"
#include "gpio.h"

#include "onewire.h"
#include "ds18b20.h"


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

#define UserButton GPIOC, LL_GPIO_PIN_13

#define DS18B20 GPIOA, LL_GPIO_PIN_0

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
  SysTick->LOAD = 0x989680;  // 10_000_000 * 0.3
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

  LL_GPIO_SetPinMode(UserButton, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinPull(UserButton, LL_GPIO_PULL_DOWN);
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
void LCD_clear(){
	WriteToLCD(0x01,1);
}
void init_LCD() {
  // LCD register
  WriteToLCD(56, 1);  // Function Setting
  WriteToLCD( 6, 1);  // Entering mode
  WriteToLCD(12, 1);  // Display on
  WriteToLCD( 1, 1);  // Clear Screen
  WriteToLCD( 2, 1);  // Move to top left
}



/* This function aims to create a font in CGR
 * location(input): CGR address of the font
 * fontArray(input): Content of the font, 8 elements indicate 8 rows
 */
void CreateFont(int location, int *fontArray){
	for(int i = 0; i < 8; i++){
		WriteToLCD(0x40 | (location << 3) | i, 1);
		WriteToLCD(fontArray[i], 0);
	}
}

int LCD_char_map(int ascii){
	return ascii >= ' ' && ascii <= '}' ? ascii - ' ' + 0x20 : 0x20;
}


/* Let LCD show string
 * Noted the length of the string will not exceed the number of columns on our LCD
 */
void WriteStrToLCD(char *str){
	LCD_clear();
	LCD_cursor_set(0);
	WriteToLCD(0x06, 1); // enter mode
	while(*str){
		WriteToLCD(LCD_char_map(*str++), 0); // write character
	}
}

int isBtnPress(){
	int i = 0, d = 20;
	while(d--){
		if(i != IsSetPin(UserButton)){
			d = 20;
			i = !i;
		}
	}
	return !i;
}

void init_Font(){
	int ch1[] = {0x1f, 0x18, 0x14, 0x12, 0x11, 0x00, 0x00, 0x00};
	int ch2[] = {0x00, 0x00, 0x00, 0x11, 0x09, 0x05, 0x03, 0x1f};
	CreateFont(0, ch1);
	CreateFont(1, ch2);
}


int stringMode = 1;
volatile int pos;

void SysTick_Handler() {
  pos++;
}

OneWire_t OneWire_ds18b20;

int main() {
	// Enable FPU
  MODIFY_REG(SCB->CPACR, 0xf << 20, 0xf << 20);
  SystemClock_Config();
  GPIO_init();
  init_LCD();
  init_Font();
  OneWire_Init(&OneWire_ds18b20, DS18B20);

  int i = -1, l = -1, tmp;
  char buf[20];
  int btnPressed = 0;
  float temp;
  int conv = 0;
  int cnt = 0;
  while(1){
	  if(stringMode){
		  if(conv){
			  if(!DS18B20_Done(&OneWire_ds18b20)){
				  DS18B20_Read(&OneWire_ds18b20, &temp);
				  sprintf(buf, "%.4f", temp);
				  WriteStrToLCD(buf);
				  conv = 0;
				  cnt++;
			  }
		  }else{
			  if(l != pos){
				  l = pos;
				  conv = 1;
				  // Set precision
				  // 12bit: 0.0625
				  // 11bit: 0.125
				  // 10bit: 0.25
				  //  9bit: 0.5
				  DS18B20_ConvT(&OneWire_ds18b20, TM_DS18B20_Resolution_11bits);
			  }
		  }

	  }else{
		tmp = pos % 32;
		if(tmp > 15) tmp += 0x30;
		if (i != tmp) {
			i = tmp;
			switch(i){
			case 0x00: LCD_cursor_set(0x4f); break;
			case 0x40: LCD_cursor_set(0x0f); break;
			default: LCD_cursor_set(i - 1); break;
			}
			WriteToLCD(0x06, 1);   // 0b0000_0110, setting mode, increment, no shift
			WriteToLCD(0x20, 0); // 0b0011_0100
			LCD_cursor_set(i);
			WriteToLCD(0x06, 1);   // 0b0000_0110, setting mode, increment, no shift
			WriteToLCD(0x00, 0); // 0b0011_0100
			switch(i){
			case 0x0f: LCD_cursor_set(0x40); WriteToLCD(0x06, 1); break;
			case 0x4f: LCD_cursor_set(0x00); WriteToLCD(0x06, 1); break;
			}
			WriteToLCD(0x01, 0); // 0b0011_1001
		}
	 }
	if(isBtnPress()){
		if(!btnPressed) btnPressed = 1;
	}else{
		if(btnPressed){
			btnPressed = 0;
			stringMode = !stringMode;
			i = -1, l = -1, pos = 0;
			LCD_clear();
			DisableSysTick();
			// Change system tick
			SysTick->LOAD = stringMode ? 0x989680 : 0x2dc6c0;  // 10_000_000 * 1 : 10_000_000 * 0.3
			SysTick->VAL = 0;
			EnableSysTick();
		}
	}
  }
}
