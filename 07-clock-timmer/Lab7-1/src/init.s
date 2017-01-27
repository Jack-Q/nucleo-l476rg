.syntax unified
.cpu cortex-m4
.thumb

.text
.global GPIO_init
.global delay_1s
.equ RCC_AHB2ENR,  0x4002104C
.equ GPIOA_MODER,  0x48000000
.equ GPIOA_OTYPER, 0x48000004
.equ GPIOA_OSPEEDR,0x48000008
.equ GPIOA_PUPDR,  0x4800000C
.equ GPIOA_ODR,    0x48000014
.equ GPIOC_BOUNDARY, 0x48000800
.equ GPIOC_MODER,    0x00 + GPIOC_BOUNDARY
.equ GPIOC_OTYPER,   0x04 + GPIOC_BOUNDARY
.equ GPIOC_OSPEEDR,  0x08 + GPIOC_BOUNDARY
.equ GPIOC_PUPDR,    0x0C + GPIOC_BOUNDARY
.equ GPIOC_IDR,      0x10 + GPIOC_BOUNDARY
// PA5
GPIO_init:
	movs	%r0, #5 // A + C
	ldr		%r1, =RCC_AHB2ENR
	str		%r0, [%r1]

	// Init LED
	movs 	%r0, #0x0400
	ldr		%r1, =GPIOA_MODER
	ldr		%r2, [%r1]
	and		%r2, #0xfffff3ff
	orrs	%r2, %r2, %r0
	str		%r2, [%r1]

	movs	r0, #0x0800
	ldr		r1, =GPIOA_OSPEEDR
	strh	r0, [r1]

	// Init Push Button
    ldr     %r1, =GPIOC_MODER
    ldr     %r0, [%r1]
    and     %r0, #0xf3ffffff
    str     %r0, [%r1]

    mov     %r0, #0x04000000
    ldr     %r1, =GPIOC_PUPDR
    ldr     %r2, [%r1]
    and     %r2, #0xf3ffffff
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

	bx 		%lr


    // Delay (1s) module
delay_1s:
    movs    r5, #0
    movw    r6, #0xc95      // 0xffffffff / 4M * 3 => 0xc95
delay_d:
    adds    r5, r5, r6      // 1 cycle
    bcc     delay_d         // 2 cycles
    BX      %lr
