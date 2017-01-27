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
    // Declare data sction varibale
    result: .byte 0

.text
    .global main
    .equ    X,  1234//0x55AA
    .equ    Y,  4321//0xAA55

hamm:
    // Calculate the Hamming Distance of ro and r1
    eor     %r0, %r0, %r1   // use exclusive or to get the different bit
    mov     %r3, #0         // reset the accumulator to zero
hamm_sum:
    lsrs    %r0, #1         // logical shift 1 bit, store that bit at carry bit
    adcs    %r3, #0         // add that bit to accumulator
    cmp     %r0, #0         // compare to check if all bits are counted
    bne     hamm_sum        // jump if it is not finished

    strb    %r3, [%r2]      // Use b suffix to prevent damage to upper bits
    bx      %lr

main:
    movw    %r0, #X     // load the half word X to register r0
    movw    %r1, #Y     // load the half word Y to register r1
    ldr     %r2, =result// load the address of result to r2 
    bl      hamm

L:
    B       L
