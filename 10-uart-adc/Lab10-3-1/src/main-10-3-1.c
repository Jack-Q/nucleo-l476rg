/********************************************************
 * Micro Computer System Lab - Lab 10: UART and ADC
 *                                               Group 49
 ********************************************************
 * Lab 10-3: Simple Shell
 *
 * Implement a simple shell via the onboard UART
 * functionality. The shell is suppoesd to support the
 * following command:
 *   - light: display and update the vlotage of
 *            photoresistor every 0.5 sec
 *   - showid: display the student id
 *   - led {on|off}: turn on/off the onboard LED
 ********************************************************
 * File: main-10-3-1.c
 * Note:
 * This version contians additional support to left/right
 * arrow key, color output for some characters as well as
 * the help and clear command.
 ********************************************************/

#define STM32L476XX

#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "stdio.h"
#include "string.h"

#include "stm32l476xx.h"
#include "stm32l4xx_ll_adc.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_tim.h"
#include "stm32l4xx_ll_usart.h"

// Select the usart mode
// USART1 use PA9 and PA10 to connect via external USB cable
// USART2 use PA1 and PA2 to connect via STLink
USART_TypeDef *usart = USART2;

/*******************************************************
 * CLOCK configuration
 *******************************************************/

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
  SysTick->LOAD = 0x4c4b40 - 1;  // 0.5 sec (10MHz / 2)
  SysTick->VAL = 0;
  SelectSysTickSrc();
  EnableSysTickInt();
}

#define UserButton GPIOC, LL_GPIO_PIN_13
#define LED GPIOA, LL_GPIO_PIN_5
#define SetPin LL_GPIO_SetOutputPin
#define ResetPin LL_GPIO_ResetOutputPin
#define IsSetPin LL_GPIO_IsInputPinSet
void LED_Config() {
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  // Initialize led
  LL_GPIO_SetPinMode(LED, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinOutputType(LED, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinSpeed(LED, LL_GPIO_SPEED_HIGH);
  LL_GPIO_SetPinPull(LED, LL_GPIO_PULL_DOWN);
}

void UserBtn_Config() {
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
  // Initialize user button
  LL_GPIO_SetPinMode(UserButton, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinSpeed(UserButton, LL_GPIO_SPEED_HIGH);
  LL_GPIO_SetPinPull(UserButton, LL_GPIO_PULL_DOWN);
}

#define DEBOUNCE 40
int user_press_button() {
  static int btn = 1;
  static int cnt = DEBOUNCE;
  static int pressed = 0;
  if (IsSetPin(UserButton) ^ btn) {
    if (!--cnt) btn = !btn, pressed = 0;
  } else {
    cnt = DEBOUNCE;
  }
  return !btn && !pressed ? (pressed = 1) : 0;
}

void USART_Config() {
  if (usart == USART2) {
    // Enable corresponding GPIO port
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    uint32_t gpio_pin = LL_GPIO_PIN_2;
    LL_GPIO_SetPinMode(GPIOA, gpio_pin, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinPull(GPIOA, gpio_pin, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinSpeed(GPIOA, gpio_pin, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetAFPin_0_7(GPIOA, gpio_pin, LL_GPIO_AF_7);
    gpio_pin = LL_GPIO_PIN_3;
    LL_GPIO_SetPinMode(GPIOA, gpio_pin, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinPull(GPIOA, gpio_pin, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinSpeed(GPIOA, gpio_pin, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetAFPin_0_7(GPIOA, gpio_pin, LL_GPIO_AF_7);
    // Enable USART timer
    LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_SYSCLK);
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
  } else if (usart == USART1) {
    // Enable corresponding GPIO port
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    uint32_t gpio_pin = LL_GPIO_PIN_9;
    LL_GPIO_SetPinMode(GPIOA, gpio_pin, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinPull(GPIOA, gpio_pin, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinSpeed(GPIOA, gpio_pin, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetAFPin_8_15(GPIOA, gpio_pin, LL_GPIO_AF_7);
    gpio_pin = LL_GPIO_PIN_10;
    LL_GPIO_SetPinMode(GPIOA, gpio_pin, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinPull(GPIOA, gpio_pin, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinSpeed(GPIOA, gpio_pin, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetAFPin_8_15(GPIOA, gpio_pin, LL_GPIO_AF_7);
    // Enable USART timer
    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_SYSCLK);
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  }

  usart->BRR = 10000000 / 9600;
  usart->CR1 = USART_CR1_RE | USART_CR1_TE;

  // Enable USART2
  usart->CR1 |= USART_CR1_UE;
}

void USART_Send_Ch(char c) {
  // Wait till transmission empty flag set
  while (!(usart->ISR & USART_ISR_TXE))
    ;
  usart->TDR = c;
}
void USART_Send(char *d) {
  while (*d) {
    USART_Send_Ch(*d++);
  }
}

char USART_Receive() {
  while (!(usart->ISR & USART_ISR_RXNE))
    ;
  usart->ISR = usart->ISR & ~USART_ISR_RXNE_Msk;
  return usart->RDR & 0xff;
}

void configureADC() {
  uint32_t channel = LL_ADC_CHANNEL_5;

  // Enable ADC Corresponding GPIO Port
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_0, LL_GPIO_PULL_NO);
  LL_GPIO_EnablePinAnalogControl(GPIOA, LL_GPIO_PIN_0);

  // Enable Clock
  RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
  LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSOURCE_SYSCLK);

  // Keep pre-scalar as default
  // ADC123_COMMON->CCR = (ADC123_COMMON->CCR & ADC_CCR_CKMODE_Msk) | (1 <<
  // ADC_CCR_CKMODE_Pos);
  LL_ADC_SetCommonClock(ADC123_COMMON, LL_ADC_CLOCK_SYNC_PCLK_DIV1);
  LL_ADC_SetMultimode(ADC123_COMMON, LL_ADC_MULTI_INDEPENDENT);
  LL_ADC_SetOverSamplingScope(ADC1, LL_ADC_OVS_DISABLE);
  LL_ADC_SetCommonPathInternalCh(
      ADC123_COMMON,
      LL_ADC_PATH_INTERNAL_VREFINT | LL_ADC_PATH_INTERNAL_TEMPSENSOR);
  LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_12B);
  LL_ADC_SetChannelSamplingTime(ADC1, channel, LL_ADC_SAMPLINGTIME_6CYCLES_5);
  LL_ADC_SetDataAlignment(ADC1, LL_ADC_DATA_ALIGN_RIGHT);
  LL_ADC_DisableDeepPowerDown(ADC1);

  // Enable related interrupt
  NVIC_EnableIRQ(ADC1_2_IRQn);
  NVIC_SetPriority(ADC1_2_IRQn, 0);
  // LL_ADC_SetChannelSingleDiff(ADC1, channel,LL_ADC_SINGLE_ENDED);

  LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_SINGLE);
  LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_SOFTWARE);
  LL_ADC_REG_SetSequencerDiscont(ADC1, LL_ADC_REG_SEQ_DISCONT_DISABLE);
  // LL_ADC_REG_SetTriggerEdge(ADC1, LL_ADC_REG_TRIG_EXT_RISINGFALLING);

  LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE);
  LL_ADC_REG_SetSequencerDiscont(ADC1, LL_ADC_REG_SEQ_DISCONT_DISABLE);
  LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1,
                               channel);  // PA0 -> ADC12_IN_5

  LL_ADC_EnableInternalRegulator(ADC1);
  while (!LL_ADC_IsInternalRegulatorEnabled(ADC1))
    ;

  LL_ADC_EnableIT_EOC(ADC1);
  // LL_ADC_EnableInternalRegulator(ADC1);
  LL_ADC_Enable(ADC1);
}
int raw_data = 10;
void ADC1_2_IRQHandler() {
  NVIC_ClearPendingIRQ(ADC1_2_IRQn);
  if (LL_ADC_IsActiveFlag_EOC(ADC1)) {
    // raw_data = LL_ADC_REG_ReadConversionData12(ADC1);
    raw_data = ADC1->DR;
    LL_ADC_ClearFlag_EOC(ADC1);
  }
}
void startADC() { LL_ADC_REG_StartConversion(ADC1); }
void cmd_showId() { USART_Send("0540017\r\n"); }
void cmd_led(int on) {
  if (on) {
    USART_Send("Turn \033[33mon\033[0m LED\r\n");
    LL_GPIO_SetOutputPin(LED);
  } else {
    USART_Send("Turn \033[34moff\033[0m LED\r\n");
    LL_GPIO_ResetOutputPin(LED);
  }
}
void SysTick_Handler() {
  startADC();
  static char buf[25];
  sprintf(buf, "\033[2K\r%f", raw_data * 10.0e3 / (0xfff - raw_data));
  USART_Send(buf);
}
void cmd_light() {
  SysTick_Handler();  // Manual Trigger
  SysTick->VAL = 0;
  EnableSysTick();
  while (1) {
    if ((usart->ISR & USART_ISR_RXNE) && USART_Receive() == 'q') break;
  }
  DisableSysTick();
  USART_Send("\r\n");
}
void cmd_help() {
  USART_Send("We implemented the following commands:\r\n");
  USART_Send("---------------------------------------\r\n");
  USART_Send("\033[36mshowid\033[0m: Show your student ID\r\n");
  USART_Send("\033[36mlight\033[0m: Get AD value from photoresistor\r\n");
  USART_Send(
      "\033[36mled\033[0m {\033[33mon\033[0m|\033[34moff\033[0m}: Turn "
      "{\033[33mon\033[0m|\033[34moff\033[0m} led (PA5)\r\n");
}

#define PS "\033[32m>\033[0m"

void cmd_clear() { USART_Send("\033[0;0H\033[2J"); }
void handleCommand(char *cmd) {
  static char buf[120];
  strcpy(buf, cmd);
  char *cmd_name = strtok(buf, " ");
  char *param = strtok(NULL, " ");
  if (!strcmp(cmd_name, "led") && param) {
    char *ext = strtok(NULL, " ");
    if (!ext && !strcmp(param, "on")) {
      cmd_led(1);
      return;
    }
    if (!ext && !strcmp(param, "off")) {
      cmd_led(0);
      return;
    }
  } else if (!strcmp(cmd_name, "showid") && !param) {
    cmd_showId();
    return;
  } else if (!strcmp(cmd_name, "light") && !param) {
    cmd_light();
    return;
  } else if (!strcmp(cmd_name, "clear") && !param) {
    cmd_clear();
    return;
  } else if (!strcmp(cmd_name, "help") && !param) {
    cmd_help();
    return;
  }
  USART_Send("Unknown Command: [\033[31m");
  USART_Send(cmd);
  USART_Send("\033[0m]\r\n");
}
int main() {
  // Enable FPU
  MODIFY_REG(SCB->CPACR, 0xf << 20, 0xf << 20);
  SystemClock_Config();
  USART_Config();
  UserBtn_Config();
  LED_Config();
  configureADC();
  startADC();

  char buf[120];
  int pos = 0, cur = 0;
  cmd_clear();
  USART_Send(PS);  // Reset

  int escape_mode = 0;
  while (1) {
    char c = USART_Receive();
    if (escape_mode) {
      if ((c >= '@' && c <= 'Z') || (c >= 'a' && c <= '~')) {
        switch (c) {
          case 'D':
            if (cur > 0) {
              cur--;
              USART_Send_Ch('\010');
            }
            break;
          case 'C':
            if (cur < pos) {
              cur++;
              USART_Send("\033[C");
            }
            break;
          case 'H':  // Home
            cur = 0;
            USART_Send("\033[H\033[C");
            break;
          case 'F':  // End
            cur = pos;
            break;
        }
        escape_mode = 0;
      }
      continue;
    }
    if (c >= 32 && c < 127) {
      if (pos>= 100) continue;
      USART_Send_Ch(c);
      int p = cur;
      while (p < pos) USART_Send_Ch(buf[p++]);
      pos++;
      while (p > cur) USART_Send_Ch('\010'), buf[p] = buf[p - 1], p--;
      buf[cur++] = c;
    } else {
      switch (c) {
        case 8:    // Backspace(via ^H)
        case 127:  // DEL (Via Backspace)
          if (cur) {
            pos--;
            int p = --cur;
            USART_Send_Ch('\010');
            while (p < pos) buf[p] = buf[p + 1], USART_Send_Ch(buf[p++]);
            USART_Send_Ch(' ');
            while (p-- >= cur) USART_Send_Ch('\010');
          }
          break;
        case 13:  // Carriage Feed (Enter)
          if (pos != 0) {
            buf[pos] = 0;
            USART_Send("\r\n");
            handleCommand(buf);
            cur = pos = 0;
          } else {
            USART_Send("\r\n");
          }
          USART_Send(PS);
          break;
        case 27:  // Escape, may followed by escape sequence
          escape_mode = 1;
          break;
      }
    }
  }
  return 0;
}
