/******************************************************
 * Max7192 without code B mode
 * -----------------
 * Connect Max7192 with board and display the following
 * sequence in order iteriatviely with 1s interval
 * between consequentive letters.
 * 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, b, C, d, E, F
 * -----------------
 * Max7192 is connected to board via VCC(3.3V), GND,
 * as well as GPIO port: PB3, PB4, PB5
 *  PB3: Data
 *  PB4: CLK
 *  PB5: CS
 ******************************************************/

    .syntax unified
    .cpu cortex-m4
    .thumb
.data

.text
    .global main
	arr: .byte 0x7e, /* 0
            */ 0x30, /* 1
            */ 0x6d, /* 2
            */ 0x79, /* 3
            */ 0x33, /* 4
            */ 0x5b, /* 5
            */ 0x5f, /* 6
            */ 0x70, /* 7
            */ 0x7f, /* 8
            */ 0x7b, /* 9
            */ 0x77, /* A
            */ 0x1f, /* b
            */ 0x4e, /* C
            */ 0x3d, /* d
            */ 0x4f, /* E
            */ 0x47  /* F
            */
    .align
    // Max7219 Commands
    .equ MAX_DIGIT_0,	 0x01
    .equ MAX_DIGIT_1,	 0x02
    .equ MAX_DIGIT_2,	 0x03
    .equ MAX_DIGIT_3,	 0x04
    .equ MAX_DIGIT_4,	 0x05
    .equ MAX_DIGIT_5,	 0x06
    .equ MAX_DIGIT_6,	 0x07
    .equ MAX_DIGIT_7,	 0x08
	.equ MAX_DEC_MODE,	 0x09
	.equ MAX_INTENSITY,	 0x0A
	.equ MAX_SCAN_LIM,	 0x0B
	.equ MAX_SHUTDOWN,	 0x0C
	.equ MAX_DISP_TEST,  0x0F

	// GPIO Address
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

main:
	bl 		GPIO_init
	bl 		Max7192_init
loop:
	bl 		Display0toF
	b 		loop

	// Initialize three GPIO pins as outpot for max7192 DIN, CS and CLK
GPIO_init:
    // 1. Enable the GPIO Clock controlled by AHB2
    mov     %r0, #0x0002
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

    // 6. All Done, return
	bx		%lr

	// Display 0 to F at first digit on 7-Seg LED
	// Display one per second
Display0toF:
    push    {%lr}
    mov     %r3, #0
Display0toF_l:
    mov     %r0, #MAX_DIGIT_0
    ldr     %r2, =arr
    ldrb    %r1, [%r2, %r3]
    push	{%r3}
    bl      Max7192_send
    bl      Delay
    pop		{%r3}
    add     %r3, %r3, #1
    cmp     %r3, #16
    bne     Display0toF_l
	pop     {%pc}

	// Initialize Max7192 register
Max7192_init:
    push    {%lr}
    mov     %r0, #MAX_DEC_MODE
    mov     %r1, #0x0
    bl      Max7192_send
    mov     %r0, #MAX_DISP_TEST
    mov     %r1, #0x0
    bl      Max7192_send
    mov     %r0, #MAX_SCAN_LIM
    mov     %r1, #0x0
    bl      Max7192_send
    mov     %r0, #MAX_INTENSITY
    mov     %r1, #0x5
    bl      Max7192_send
    mov     %r0, #MAX_SHUTDOWN
    mov     %r1, #0x1
    bl      Max7192_send
    pop     {%pc}           // return to main

	// Input paramaters: %r0: Address, %r1: Data
	// Use this function to send a message to Max7192
Max7192_send:
    lsl     %r0, %r0, #8    // -,-,A,-; -,-,-,D
    add     %r0, %r0, %r1   // -,-,A,D
    lsl     %r0, %r0, #16   // A,D,-,-

    mov     %r1, #16        // index counter
    mov     %r3, #0         // data
    ldr     %r5, =#GPIOB_ODR
    ldr     %r4, [%r5]     // Data to write

Max7192_send_l:
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
    bne      Max7192_send_l

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

	bx		%lr

	// A delay for 1 second
Delay:
    movs    r5, #0
    movw    r6, #0xc95      // 0xffffffff / 4M * 3 => 0xc95
Delay_loop:
    adds    r5, r5, r6      // 1 cycle
	bcc		Delay_loop		// 2 cycles
	bx		%lr
