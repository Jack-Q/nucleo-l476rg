/******************************************************
 * Hamming Distance
 * ----------------
 * Calculate the Hamming distance between two variable
 * in the size of half-word (2 bytes), and store the
 * result at the ``result'' variable.
 * -----------------
 * The main purpose of the experiement is calculate the
 * count of different bit in the same position of the
 * two numbers via the ``XOR'' operation.
 ******************************************************/


    .syntax unified
    .cpu cortex-m4
    .thumb // represent mixed 16/32bit instruction

.data
    // declare data sction varibale
    result: .byte 0

.text
    .global main
    // declare text section data
    .equ    X,  0x55AA
    .equ    Y,  0xAA55

    // use sequential addition without branch to get more speed
hamm:
    eors    %r0, %r0, %r1   // use exclusive or to get the difference in bit
    // get the bit count of each two bits
    movs    %r1, %r0        // copy result
    lsrs    %r1, #1         // logical shift right, 1 bit
    movw    %r3, #0x5555   // load bit mask 0b0101010101010101
    ands    %r0, %r3       // apply mask to orginal
    ands    %r1, %r3       // apply mask to copied
    adds    %r0, %r0, %r1  // add them
    // get the bit count of each four bits
    movs    %r1, %r0
    lsrs    %r1, #2         // logical shift right, 2 bit
    movw    %r3, #0x3333    // load bit mask 0b0011001100110011
    ands    %r0, %r3
    ands    %r1, %r3
    adds    %r0, %r0, %r1
    // get the bit count of each eight bits
    movs    %r1, %r0
    lsrs    %r1, #4         // logical shift right, 4 bit
    movw    %r3, #0x0f0f    // load bit mask 0b0000111100001111
    ands    %r0, %r3
    ands    %r1, %r3
    adds    %r0, %r0, %r1
    // get the bit count of each sixteen bits
    movs    %r1, %r0
    lsrs    %r1, #8         // logical shift right, 8 bit
    movs    %r3, #0x00ff    // load bit mask 0b0000000011111111
    ands    %r0, %r3
    ands    %r1, %r3
    adds    %r0, %r0, %r1
    // all bits are added up, store result to memory
    strb    %r0, [%r2]      // store result in the byte form
    bx      %lr             // branch to linked address, return to main

main:
    movw    %r0, #X
    movw    %r1, #Y
    ldr     %r2, =result
    bl      hamm

L:
    B       L
