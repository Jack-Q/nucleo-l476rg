/******************************************************
 * 4-bit Password Lock
 * -----------------
 * Implement a 4 bit password lock with DIP switch, push
 * button and 4 LEDs.
 * -----------------
 * LED is connected to PB3, PB4, PB5, PB6
 * Push Button (on board)is connected to: PC13
 * DIP switch (1,2,3,4) is connected to: PB12, PB13, PB14, PB15
 ******************************************************/
    .syntax unified
    .cpu cortex-m4
    .thumb
.data
    dip_data:   .word
    cmp_result: .word
.text
    password:   .byte    0*8 + 1*4+ 1*2 + 1*1
    .align

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
    .global main

main:
    bl      GPIO_init
loop:
    bl      wait_press
    bl      read_dip
    bl      comp_pass
    bl      blink
    b   loop

GPIO_init:
    // 1. Enable the GPIO Clock controlled by AHB2
    mov     %r0, #0x0006            // GPIO B, C
    ldr     %r1, =RCC_AHB2ENR
    ldr     %r2, [r1]
    orr     %r0, %r0, %r2
    str     %r0, [r1]

    // 2. Set GPIO-B 3..6 to General purpose output mode (0x01)
    //    and set GPIO-B 12..15 to input mode (0x00)
    //
    //    Set to     0000_0000_----_----_--01_0101_01--_----
    //    keep mask: 0000_0000_1111_1111_1100_0000_0011_1111 => 0x00ffc03f
    //    set mask:  0000_0000_0000_0000_0001_0101_0100_0000 => 0x00001540
    //
    mov     %r0, #0x00001540
    ldr     %r1, =GPIOB_MODER
    ldr     %r2, [%r1]
    movw    %r3, #0x00ffc0
    lsl     %r3, #8
    add     %r3, %r3, #0x3f
    and     %r2, %r3
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 3. Set GPIO-B 3..6 to Open drain (0x1)
    //    this setting is not available to input mode
    //
    //    Set to:    ----_----_----_----_----_----_-111_1---
    //    keep mask: 1111_1111_1111_1111_1111_1111_1000_0111 => 0xffffff87
    //    set mask:  0000_0000_0000_0000_0000_0000_0111_1000 => 0x00000078
    //    * Since all bits are going to set to 1, keep mask is redundant
    //
    mov     %r0, #0x00000078
    ldr     %r1, =GPIOB_OTYPER
    ldr     %r2, [%r1]
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 4. Set GPIO-B 3..6 to High Speed (0x10)
    //    this setting is not available to input mode
    //
    //    Set to     ----_----_----_----_--10_1010_10--_---
    //    keep mask: 1111_1111_1111_1111_1100_0000_0011_1111 => 0xffffc03f
    //    set mask:  0000_0000_0000_0000_0010_1010_1000_0000 => 0x00002a80
    //
    mov     %r0, #0x00002a80
    ldr     %r1, =GPIOB_OSPEEDR
    ldr     %r2, [%r1]
    and     %r2, #0xffffc03f
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 5. Set GPIO-B 3..6 to No Pull (0x00) (Default)
    //    and set GPIO-B 12..15 to Pull Up (0x01)
    //
    //    Set to     0101_0101_----_----_--00_0000_00--_----
    //    keep mask: 0000_0000_1111_1111_1100_0000_0011_1111 => 0x00ffc03f
    //    set mask:  0101_0101_0000_0000_0000_0000_0000_0000 => 0x55000000
    //
    mov     %r0, #0x55000000
    ldr     %r1, =GPIOB_PUPDR
    ldr     %r2, [%r1]
    movw    %r3, #0x00ffc0
    lsl     %r3, #8
    add     %r3, %r3, #0x3f
    and     %r2, %r3
    orr     %r0, %r0, %r2
    str     %r0, [%r1]

    // 6. Set GPIO-C 13 to input mode (0x00)
    //
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
    //
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

    // Turn off the LEDs
    ldr     %r1, =GPIOB_ODR
    mov     %r2, #0xffffffff
    str     %r2, [%r1]

    // 10. All Done, return
    BX      %lr

    // loop and wait til the button pressed down and released up
wait_press:
    movs    %r4, #1
    lsl     %r4, #13
    ldr     %r2, =GPIOC_IDR
    movs    %r0, #1         // button up
    movs    %r1, #0         // consequentive button changed conut
wait_press_loop:
    ldr     %r3, [%r2]      
    ands    %r3, %r4        
    it      ne              
    movne   %r3, #1

    eors    %r3, %r3, %r0
    ite     eq
    moveq   %r1, #0
    addne   %r1, %r1, #1

    cmp     %r1, #DEB_THR
    ittte   gt
    movgt   %r3, #1
    movgt   %r1, #0
    eorgt   %r0, %r0, #1
    movle   %r3, #0

    add     %r3, %r0, %r3
    cmp     %r3, #2         // button stage changed to up again (pressed && released)
    bne     wait_press_loop

    bx      %lr

    // read the status of the DIP switcher
read_dip:
    mov     %r0, #0x0000f000
    ldr     %r1, =GPIOB_IDR
    ldr     %r2, [%r1]
    and     %r0, %r0, %r2
    lsr     %r0, #12
    eor     %r0, %r0, #-1   // negate the status since the button is active low
    and     %r0, %r0, #0x000f
    ldr     %r1, =dip_data
    str     %r0, [%r1]
    bx      %lr

    // compare the password with switcher status
comp_pass:
    ldr     %r1, =dip_data
    ldr     %r0, [%r1]
    ldr     %r2, =password
    ldrb    %r1, [%r2]
    cmp     %r0, %r1
    ite     eq
    moveq   %r0, #0
    movne   %r0, #1
    ldr     %r1, =cmp_result
    str     %r0, [%r1]
    bx      %lr

    // blink LEDs regarding to the result comparison
blink:
    ldr     %r1, =cmp_result
    ldr     %r0, [%r1]
    ite     eq
    moveq   %r0, #3         // blink 3 times
    movne   %r0, #1         // blink 1 times
blink_led:
    ldr     %r1, =GPIOB_ODR
    mov     %r2, #0xffffff87 // turn the LED on
    str     %r2, [%r1]

    movw    %r4, #0x192b    // 0xffffffff / 2M * 3 => 0x192b
    mov     %r3, #0
blink_led_d1:               // delay for 0.25s
    adds    %r3, %r3, %r4
    bcc     blink_led_d1

    mov     %r2, #0xffffffff // turn the LESs off
    str     %r2, [%r1]

    movs    %r3, #0
blink_led_d2:               // delay for 0.25s
    adds    %r3, %r3, %r4
    bcc     blink_led_d2

    subs    %r0, %r0, #1    // check the blink time
    bne     blink_led

    bx      %lr
 
