#include "onewire.h"
#include "stm32l4xx_ll_gpio.h"
#define OneWirePin(OneWireStruct) \
	OneWireStruct->GPIOx, OneWireStruct->GPIO_Pin

/* Init OneWire Struct with GPIO information
 * param:
 *   OneWire: struct to be initialized
 *   GPIOx: Base of the GPIO DQ used, e.g. GPIOA
 *   GPIO_Pin: The pin GPIO DQ used, e.g. 5
 */
void OneWire_Init(OneWire_t* OneWireStruct, GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin) {
	OneWireStruct->GPIOx = GPIOx;
	OneWireStruct->GPIO_Pin = GPIO_Pin;
	LL_GPIO_SetPinPull(OneWirePin(OneWireStruct), LL_GPIO_PULL_UP);
	LL_GPIO_SetPinOutputType(OneWirePin(OneWireStruct), LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinSpeed(OneWirePin(OneWireStruct), LL_GPIO_SPEED_FREQ_MEDIUM);
	LL_GPIO_SetPinMode(OneWirePin(OneWireStruct), LL_GPIO_MODE_INPUT);
}

/* Send reset through OneWireStruct
 * Please implement the reset protocol
 * param:
 *   OneWireStruct: wire to send
 * retval:
 *    0 -> Reset OK
 *    1 -> Reset Failed
 */
uint8_t OneWire_Reset(OneWire_t* OneWireStruct) {
	register int i;
	ResetPin(OneWirePin(OneWireStruct));
	LL_GPIO_SetPinMode(OneWirePin(OneWireStruct), LL_GPIO_MODE_OUTPUT);
	i = 5000; while(--i);
	LL_GPIO_SetPinMode(OneWirePin(OneWireStruct), LL_GPIO_MODE_INPUT);
	i = 15; while(--i);
	i = 10000;
	while(IsSetPin(OneWirePin(OneWireStruct)) && --i);
	i = 10000;
	while(!IsSetPin(OneWirePin(OneWireStruct)) && --i);
	return !!i;
}

/* Write 1 bit through OneWireStruct
 * Please implement the send 1-bit protocol
 * param:
 *   OneWireStruct: wire to send
 *   bit: bit to send
 */
void OneWire_WriteBit(OneWire_t* OneWireStruct, uint8_t bit) {
	register int i;
	ResetPin(OneWirePin(OneWireStruct));
	i = IsSetPin(OneWirePin(OneWireStruct));
	LL_GPIO_SetPinMode(OneWirePin(OneWireStruct), LL_GPIO_MODE_OUTPUT);
	i = IsSetPin(OneWirePin(OneWireStruct));
	i = 10; while(--i); // > 1 us
	if(bit){
		// Write 1 Slot
		LL_GPIO_SetPinMode(OneWirePin(OneWireStruct), LL_GPIO_MODE_INPUT);
		i = 150; while(--i); // > 45us + (15us) duration
	}else{
		// Write 0 Slot
		i = 150; while(--i);// > 45us + (15us) duration
		LL_GPIO_SetPinMode(OneWirePin(OneWireStruct), LL_GPIO_MODE_INPUT);
	}
}

/* Read 1 bit through OneWireStruct
 * Please implement the read 1-bit protocol
 * param:
 *   OneWireStruct: wire to read from
 */
uint8_t OneWire_ReadBit(OneWire_t* OneWireStruct) {
	register int i; uint8_t bit;
	ResetPin(OneWirePin(OneWireStruct));
	LL_GPIO_SetPinMode(OneWirePin(OneWireStruct), LL_GPIO_MODE_OUTPUT);
	i = 3; while(--i); //  1 us
	LL_GPIO_SetPinMode(OneWirePin(OneWireStruct), LL_GPIO_MODE_INPUT);
	i = 10; while(--i); // < 15 us
	bit = IsSetPin(OneWirePin(OneWireStruct));
	i = 500; while(--i);// > 45 us + (~ us) duration

	return bit;
}

/* A convenient API to write 1 byte through OneWireStruct
 * Please use OneWire_WriteBit to implement
 * param:
 *   OneWireStruct: wire to send
 *   byte: byte to send
 */
void OneWire_WriteByte(OneWire_t* OneWireStruct, uint8_t byte) {
	// in least significant bit first order
	for(int i = 0; i < 8; i++, byte >>= 1){
		OneWire_WriteBit(OneWireStruct, byte & 1);
	}
}

/* A convenient API to read 1 byte through OneWireStruct
 * Please use OneWire_ReadBit to implement
 * param:
 *   OneWireStruct: wire to read from
 */
uint8_t OneWire_ReadByte(OneWire_t* OneWireStruct) {
	uint8_t byte = 0;
	for(int i = 0; i < 8; i++){
		//byte = (byte << 1 )| OneWire_ReadBit(OneWireStruct);
		byte = (byte >> 1) | (OneWire_ReadBit(OneWireStruct) << 7);
	}
	return byte;
}

/* Send ROM Command, Skip ROM, through OneWireStruct
 * You can use OneWire_WriteByte to implement
 */
void OneWire_SkipROM(OneWire_t* OneWireStruct) {
	OneWire_WriteByte(OneWireStruct, 0xCC);
}
