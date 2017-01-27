/******************************************************
 * GPIO and Max7219 initialization procedures
 * -----------------
 * Max7219 is connected to board via VCC(3.3V), GND,
 * as well as GPIO port: PB13, PB14, PB15
 * Push button is connected on board at PC13
 ******************************************************/
    .syntax unified
    .cpu cortex-m4
    .thumb

.text
    .global GPIO_init_max7219
    .global Max7219_send
    .global Max7219_init

    .align

    // Max7219 Commands
    .equ MAX_DIGIT_0,    0x01
    .equ MAX_DIGIT_1,    0x02
    .equ MAX_DIGIT_2,    0x03
    .equ MAX_DIGIT_3,    0x04
    .equ MAX_DIGIT_4,    0x05
    .equ MAX_DIGIT_5,    0x06
    .equ MAX_DIGIT_6,    0x07
    .equ MAX_DIGIT_7,    0x08
    .equ MAX_DEC_MODE,   0x09
    .equ MAX_INTENSITY,  0x0A
    .equ MAX_SCAN_LIM,   0x0B
    .equ MAX_SHUTDOWN,   0x0C
    .equ MAX_DISP_TEST,  0x0F

    .equ DEB_THR,        10 // debounce threshold
    .equ RCC_AHB2ENR,    0x4002104C
    .equ GPIOB_BOUNDARY, 0x48000400
    .equ GPIOB_MODER,    0x00 + GPIOB_BOUNDARY
    .equ GPIOB_OTYPER,   0x04 + GPIOB_BOUNDARY
    .equ GPIOB_OSPEEDR,  0x08 + GPIOB_BOUNDARY
    .equ GPIOB_PUPDR,    0x0C + GPIOB_BOUNDARY
    .equ GPIOB_IDR,      0x10 + GPIOB_BOUNDARY
    .equ GPIOB_ODR,      0x14 + GPIOB_BOUNDARY
    .equ GPIOC_BOUNDARY, 0x48000800
    .equ GPIOC_MODER,    0x00 + GPIOC_BOUNDARY
    .equ GPIOC_OTYPER,   0x04 + GPIOC_BOUNDARY
    .equ GPIOC_OSPEEDR,  0x08 + GPIOC_BOUNDARY
    .equ GPIOC_PUPDR,    0x0C + GPIOC_BOUNDARY
    .equ GPIOC_IDR,      0x10 + GPIOC_BOUNDARY

    // Initialize three GPIO pins as outpot for max7219 DIN, CS and CLK
GPIO_init_max7219:
    // 1. Enable the GPIO Clock controlled by AHB2
    mov     %r0, #0x0006
    ldr     %r1, =RCC_AHB2ENR
    ldr     %r2, [%r1]
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 2. Set GPIO-B 3..5 to General purpose output mode (0x01)
    //    Set to     ----_----_----_----_----_0101_01--_----
    //    keep mask: 1111_1111_1111_1111_1111_0000_0011_1111 => 0xfffff03f
    //    set mask:  0000_0000_0000_0000_0000_0101_0100_0000 => 0x00000540
    mov     %r0, #0x00000540
    ldr     %r1, =GPIOB_MODER
    ldr     %r2, [%r1]
    and     %r2, #0xfffff03f
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 3. Set GPIO-B 3..5 to Push poll (0x0) (Default)

    // 4. Set GPIO-B 3..5 to High Speed (0x10)
    //    Set to     ----_----_----_----_----_1010_10--_---
    //    keep mask: 1111_1111_1111_1111_1111_0000_0011_1111 => 0xfffff03f
    //    set mask:  0000_0000_0000_0000_0000_1010_1000_0000 => 0x00000a80
    mov     %r0, #0x00000a80
    ldr     %r1, =GPIOB_OSPEEDR
    ldr     %r2, [%r1]
    and     %r2, #0xfffff03f
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 5. Set GPIO-B 3..5 to No Pull (0x00) (Default)

    // 6. Set GPIO-C 13 to input mode (0x00)
    //    Set to     ----_00--_----_----_----_----_----_----
    //    keep mask: 1111_0011_1111_1111_1100_0000_0011_1111 => 0xf3ffffff
    //    * Since all bit are set to 0, no clear required
    //
    ldr     %r1, =GPIOC_MODER
    ldr     %r0, [%r1]
    and     %r0, #0xf3ffffff
    str     %r0, [%r1]

    // 7. Set GPIO-C 13 OTYPE (No available)

    // 8. Set GPIO-C 13 OSPEED (No available)

    // 9. Set GPIO-C 13 to Pull Up (0x01)
    //    Set to     ----_01--_----_----_----_----_----_----
    //    keep mask: 1111_0011_1111_1111_1111_1111_1111_1111 => 0xf3ffffff
    //    set mask:  0000_0100_0000_0000_0000_0000_0000_0000 => 0x04000000
    //
    mov     %r0, #0x04000000
    ldr     %r1, =GPIOC_PUPDR
    ldr     %r2, [%r1]
    and     %r2, #0xf3ffffff
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 6. All Done, return
    bx      %lr


    // Input paramaters: %r0: Address, %r1: Data
    // Use this function to send a message to Max7219

    // %a1 (%r0): unsigned char address
    // %a2 (%r1): unsigned char data
Max7219_send:
	  push 	  {%r4, %r5}
    lsl     %r0, %r0, #8    // -,-,A,-; -,-,-,D
    add     %r0, %r0, %r1   // -,-,A,D
    lsl     %r0, %r0, #16   // A,D,-,-

    mov     %r1, #16        // index counter
    mov     %r3, #0         // data
    ldr     %r5, =#GPIOB_ODR
    ldr     %r4, [%r5]     // Data to write

Max7219_send_l:
    lsls    %r0, #1
    ite     cs
    movcs   %r3, #1
    movcc   %r3, #0
    lsl     %r3, #3           // GPIO_PORT_POS

    orr     %r2, %r3, #0x0000      // Add Data
    and     %r4, %r4, #0xffffffe7
    orr     %r4, %r4, %r2
    str     %r4, [%r5]

    orr     %r2, %r3, #0x0010  // GPIO_CLOCK_MASK
    and     %r4, %r4, #0xffffffef
    orr     %r4, %r4, %r2
    str     %r4, [%r5]

    subs     %r1, %r1, #1
    bne      Max7219_send_l

    and     %r4, %r4, #0xffffffcf
    orr     %r4, %r4, #0x00000020
    str     %r4, [%r5]
    and     %r4, %r4, #0xffffffcf
    orr     %r4, %r4, #0x00000030
    str     %r4, [%r5]
    and     %r4, %r4, #0xffffffcf
    orr     %r4, %r4, #0x00000000
    str     %r4, [%r5]
    and     %r4, %r4, #0xffffffcf
    orr     %r4, %r4, #0x00000010
    str     %r4, [%r5]

	  pop     {%r4, %r5}
    bx      %lr

    // Initialize Max7219 register
Max7219_init:
    push    {%lr}
    mov     %r0, #MAX_DEC_MODE
    mov     %r1, #0xff
    bl      Max7219_send
    mov     %r0, #MAX_DISP_TEST
    mov     %r1, #0x0
    bl      Max7219_send
    mov     %r0, #MAX_INTENSITY
    mov     %r1, #0x5
    bl      Max7219_send
    mov     %r0, #MAX_SHUTDOWN
    mov     %r1, #0x1
    bl      Max7219_send
    pop     {%pc}
