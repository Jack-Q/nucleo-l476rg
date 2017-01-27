    .syntax unified
    .cpu cortex-m4
    // .thumb

.data
    X:       .word 100
    str:     .asciz "Hello World!"
    # str: 	 .ascii "Hello World!\0"

.text
    .global main
    .equ AA, 0x55

main:
    ldr %r1, =X          // Load the memory address of data X to register r1
    ldr %r0, [%r1]       // Load the data of X from memory to register r0
    movs %r2, #AA        // Set the register r2 to the immediate value AA (0x55)
    adds %r2, %r2, r0	 //	Add r0 (100,0x64) to r2, and get result r2 = 0xb9
    str %r2, [%r1]		 // store value of r2 (0xb9) to the memory address r1
    ldr %r1, =str		 // load the memory address of str
    ldr %r2, [%r1]		 // load the data of str from memory address

L:
    B L
