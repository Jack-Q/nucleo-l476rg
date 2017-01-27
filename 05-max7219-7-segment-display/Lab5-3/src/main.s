/******************************************************
 * Visual Fibonacci Serial Calculator
 * -----------------
 * Connect Max7192 with board and display the specific
 * item of Fibonacci Serial. The index of the item
 * in Fibonacci Serial is specified via the number of
 * press of push button in board and a long press
 * (press and hold for a second) will clrar current
 * state.
 * -----------------
 * Max7192 is connected to board via VCC(3.3V), GND,
 * as well as GPIO port: PB13, PB14, PB15
 * Push button is connected on board at PC13
 ******************************************************/
    .syntax unified
    .cpu cortex-m4
    .thumb
.data
    fib_n: .word    0

.text
    .global main
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

main:
    bl      GPIO_init
    bl      Max7192_init
main_loop:
    ldr     %r0, =fib_n
    ldr     %r0, [%r0]
    bl      fib
    bl      Display_number
    bl      Delay
    b       main_loop

    // Initialize three GPIO pins as outpot for max7192 DIN, CS and CLK
GPIO_init:
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

    bx      %lr

    // Initialize Max7192 register
Max7192_init:
    push    {%lr}
    mov     %r0, #MAX_DEC_MODE
    mov     %r1, #0xff
    bl      Max7192_send
    mov     %r0, #MAX_DISP_TEST
    mov     %r1, #0x0
    bl      Max7192_send
    mov     %r0, #MAX_INTENSITY
    mov     %r1, #0x5
    bl      Max7192_send
    mov     %r0, #MAX_SHUTDOWN
    mov     %r1, #0x1
    bl      Max7192_send
    pop     {%pc}

    // Display a number to 7-Seg Displayer
Display_number:
    push    {%lr}
    cmp     %r0, #-1
    beq     Display_number_Neg1
    mov     %r2, %r0
    mov     %r0, #0
Display_number_l:
    mov     %r5, #10
    udiv    %r4, %r2, %r5
    mul     %r5, %r4, %r5
    sub     %r1, %r2, %r5
    mov     %r2, %r4
    add     %r0, %r0, #1

    push    {%r0, %r2}
    bl      Max7192_send
    pop     {%r0, %r2}

    cmp     %r0, #7
    bgt     Display_number_e

    cmp     %r2, #0 
    bne     Display_number_l
    b 		Display_number_e
Display_number_Neg1:
    mov     %r0, #MAX_DIGIT_0
    mov     %r1, #1
    bl      Max7192_send
    mov     %r0, #MAX_DIGIT_1
    mov     %r1, #10
    bl      Max7192_send
	mov 	%r0, #2
Display_number_e:
    sub     %r0, %r0, #1
    mov     %r1, %r0
    mov     %r0, #MAX_SCAN_LIM
    bl      Max7192_send

    pop     {%pc}

    // Calculate the Fibonacci number
fib:
    // Validate value N
    cmp     %r0, #39
    it      gt
    movgt   %r0, #-1
    bgt     fib_end        // if N is greater than 40. overflow

    cmp     %r0, #1
    blt     fib_end         // if %r0 < 1 , %r0 = 0, return 0 (%r0) 

    // if N=1, set the result directly
    it      eq              // begin an if-then block with one instruction
    moveq   %r4, #1         // move 1 to %rr when %r0 equal to 1
    beq     fib_end         // jump to the end

    // for other scenario, set up the initial state, 
    // that is fib(0)=0, fib(1)=1
    movs    %r1, #0         // fib(0) = 0
    movs    %r2, #1         // fib(1) = 1
    movs    %r3, #0         // clear %r3

    // in this implementation, two numbers of Fibonacci series
    // will be calculated, thus the total iteration time will be
    // the half of the input number %r0.
    lsrs    %r0, #1         // divide the iteration count by 2 (via shift)
    adcs    %r3, #0         // save the carry to %r3 to determine 
                            // the position of the final result (%r1 or %r2)

    // main loop of the Fibonacci number calculation
fib_loop:
    adds    %r1, %r1, %r2   // calculate fib(2i)  (i is the time of iteration)
    adds    %r2, %r2, %r1   // calculate fib(2i+1)
    subs    %r0, #1         // decrease iteration count
    cmp     %r0, #0         
    bne     fib_loop        // if iteration not finished, jump to the start of loop

    // loop finished normally (without overflow)
    cmp     %r3, #1         // check for %r3
    ite     ne              // begin an if-then-else block
    movne   %r0, %r1        // if %r3 = 0, result at %r1
    moveq   %r0, %r2        // if %r3 = 1, result at %r2

    // execution finished, result stored at %r4, return
fib_end:
    bx      lr

    // Delay (1s) module
Delay:
    push    {%lr}
    movs    %r4, #1
    lsl     %r4, #13
    ldr     %r2, =GPIOC_IDR
    movs    %r0, #1         // button up
    movs    %r1, #0         // consequentive button changed conut
    movw    %r5, #0x30d4	// 0x30d40 for 1 sec delay
    lsl		%r5, #4
wait_press_loop:            // 20 cycle
    ldr     %r3, [%r2]      // 2 cycle
    ands    %r3, %r4        // 1 cycle
    it      ne              // 0 cycle 
    movne   %r3, #1         // 1 cycle 

    eors    %r3, %r3, %r0   // 1 cycle 
    ite     eq              // 0 cycle
    moveq   %r1, #0         // 1 cycle
    addne   %r1, %r1, #1    // 1 cycle

    cmp     %r1, #DEB_THR   // 1 cycle
    ittte   gt              // 0 cycle
    movgt   %r3, #1         // 1 cycle
    movgt   %r1, #0         // 1 cycle
    eorgt   %r0, %r0, #1    // 1 cycle
    movle   %r3, #0         // 1 cycle

    cmp     %r0, #0         // 1 cycle
    it      eq              // 0 cycle
    subeq   %r5, %r5, #1    // 1 cycle
    add     %r6, %r0, %r5  // 1 cycle
    cbz     %r6, wait_reset // 1 cycle
wait_reset_cont:

    add     %r3, %r0, %r3   // 1 cycle
    cmp     %r3, #2         // 1 cycle; button stage changed to up again (pressed && released)
    bne     wait_press_loop // 2 cycle

    ldr     %r0, =fib_n
    ldr     %r1, [%r0]
    add     %r1, %r1, #1
    str     %r1, [%r0]
    pop     {%pc}

wait_reset:
    push   {%r0, %r1, %r2, %r3, %r4, %r5}
    mov    %r0, #-1
    ldr     %r1, =fib_n
    str     %r0, [%r1]
    mov     %r0, #0
    bl     Display_number
    pop    {%r0, %r1, %r2, %r3, %r4, %r5}
    b       wait_reset_cont
