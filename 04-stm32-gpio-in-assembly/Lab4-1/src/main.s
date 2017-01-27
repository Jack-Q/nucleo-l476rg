/******************************************************
 * LED Pattern Displayer
 * -----------------
 * Implement LED Pattern Displayer on bread board by
 * controlling 4 LED (Active Low Mode).
 * -----------------
 * LED is connected to PB3, PB4, PB5, PB6
 ******************************************************/
    .syntax unified
    .cpu cortex-m4
    .thumb

.data
    leds:   .byte 0x1   // Current LED pattern bit mode
    dir:    .byte 0     // Current LED pattern shift direction, 
                        // 0: shift right, 1: shift left

.text
    .global main

    .equ RCC_AHB2ENR,    0x4002104C
    .equ GPIOB_BOUNDARY, 0x48000400
    .equ GPIOB_MODER,    0x00 + GPIOB_BOUNDARY
    .equ GPIOB_OTYPER,   0x04 + GPIOB_BOUNDARY
    .equ GPIOB_OSPEEDR,  0x08 + GPIOB_BOUNDARY
    .equ GPIOB_PUPDR,    0x0C + GPIOB_BOUNDARY
    .equ GPIOB_ODR,      0x14 + GPIOB_BOUNDARY

main:
    BL      GPIO_init
    MOVS    %r1, #1     // start form the right most position
    LDR     %r0, =leds  
    STRB    %r1, [R0]

loop:
    // Write the display pattern into leds variable
    ldr     %r0, =leds
    ldr     %r1, =dir
    ldrb    %r2, [%r0]
    ldrb    %r3, [%r1]
    cbz     %r3, loop_right

    // shift lighte LED 1 position to left:
    lsls    %r2, %r2, #1
    cmp     %r2, #0x18
    it      eq
    moveq   %r2, #0x08
    cmp     %r2, #0x10
    itt     eq
    moveq   %r2, #0x0C
    moveq   %r3, #0

    // add     %r2, %r2, #1
    // cmp     %r2, #4
    // it      ge
    // movge   %r3, #0
    b       loop_main

    // shift lighted LED 1 position to right
loop_right:
    lsrs    %r2, #1
    itt     eq
    moveq   %r2, #3
    moveq   %r3, #1
    // sub     %r2, %r2, #1
    // cmp     %r2, #0
    // it      le
    // movle   %r3, #1

loop_main:
    strb    %r2, [%r0]
    strb    %r3, [%r1]

    BL      DisplayLED
    BL      delay
    B       loop

GPIO_init:
    // 1. Enable the GPIO Clock controlled by AHB2
    mov     %r0, #0x0002
    ldr     %r1, =RCC_AHB2ENR
    ldr     %r2, [r1]
    orr     %r0, %r0, %r2
    str     %r0, [r1]

    // 2. Set GPIO-B 3..6 to General purpose output mode (0x01)
    //    Set to     ----_----_----_----_--01_0101_01--_----
    //    keep mask: 1111_1111_1111_1111_1100_0000_0011_1111 => 0xffffc03f
    //    set mask:  0000_0000_0000_0000_0001_0101_0100_0000 => 0x00001540
    mov     %r0, #0x00001540
    ldr     %r1, =GPIOB_MODER
    ldr     %r2, [%r1]
    and     %r2, #0xffffc03f
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 3. Set GPIO-B 3..6 to Open drain (0x1)
    //    Set to:    ----_----_----_----_----_----_-111_1---
    //    keep mask: 1111_1111_1111_1111_1111_1111_1000_0111 => 0xffffff87
    //    set mask:  0000_0000_0000_0000_0000_0000_0111_1000 => 0x00000078
    //    * Since all bits are going to set to 1, keep mask is redundant
    mov     %r0, #0x00000078
    ldr     %r1, =GPIOB_OTYPER
    ldr     %r2, [%r1]
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 4. Set GPIO-B 3..6 to High Speed (0x10)
    //    Set to     ----_----_----_----_--10_1010_10--_---
    //    keep mask: 1111_1111_1111_1111_1100_0000_0011_1111 => 0xffffc03f
    //    set mask:  0000_0000_0000_0000_0010_1010_1000_0000 => 0x00002a80
    mov     %r0, #0x00002a80
    ldr     %r1, =GPIOB_OSPEEDR
    ldr     %r2, [%r1]
    and     %r2, #0xffffc03f
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 5. Set GPIO-B 3..6 to No Pull (0x00) (Default)

    // 6. All Done, return
    BX      %lr

    // Display LED Pattern module
DisplayLED:
    // Display Pattern
    //
    // Shift 3 bits left to meet 
    // ----_----_----_----_----_----_-***_*---
    ldr     %r0, =leds
    ldrb    %r1, [%r0]

    lsl     %r0, %r1, #3    // shift 3 bit left
    eor     %r0, %r0, #-1   // flip all bits

    ldr     %r1, =GPIOB_ODR
    strh    %r0, [%r1]
    BX      %lr

    // Delay (1s) module
delay:
    movs    r5, #0
    movw    r6, #0xc95      // 0xffffffff / 4M * 3 => 0xc95
delay_d:
    adds    r5, r5, r6      // 1 cycle
    bcc     delay_d         // 2 cycles
    BX      %lr
