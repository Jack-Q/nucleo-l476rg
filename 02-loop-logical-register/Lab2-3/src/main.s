/******************************************************
 * Bubble Sort
 * -----------------
 * Implement the Bubble sort algorithm upon two arrays
 * of 8-bit numbers.
 * -----------------
 * The memory address of the first element is passed in
 * the sorting procedure as the first argument ``r0''.
 ******************************************************/
    .syntax unified
    .cpu cortex-m4
    .thumb // represent mixed 16/32bit instruction

.data
    // Declare data sction varibale
    // arr1:   .byte 0x19, 0x34, 0x14, 0x32, 0x52, 0x23, 0x61, 0x29
    arr1:   .byte 1,3,2,5,4,7,6,8 //0x19, 0x34, 0x14, 0x32, 0x52, 0x23, 0x61, 0x29
    arr2:   .byte 0x18, 0x17, 0x33, 0x16, 0xFA, 0x20, 0x55, 0xAC

.text
    .global main
    // the reqired length (times) of comparision,
    // which is the length of array minus 1
    .equ    Length, 8-1

    // Bubble sort implementation
    // Assume that the length of the array is 8
do_sort:
    
    // outter loop
    movs    %r1, #0         // initial loop counter
do_sort_o:

    // inner loop
    movs    %r2, #0
    movs    %r3, #Length
    subs    %r3, %r3, %r2
do_sort_i:

    // Load values
    adds    %r7, %r0, %r2
    ldrb    %r4, [%r7]
    adds    %r6, %r2, #1
    adds    %r8, %r0, %r6
    ldrb    %r5, [%r8]

    // Compare and swap values
    // with the extra space of the register, the swap process here
    // does not require extra memory space
    cmp     %r4, %r5
    itt     gt              // begin an if-then-then block
    strbgt  %r5, [%r7]      // store the swaped byte
    strbgt  %r4, [%r8]      // store the swaped byte

    // check and control inner loop
    adds    %r2, #1
    cmp     %r2, %r3
    blt     do_sort_i

    // check and control outter loop
    adds    %r1, #1
    cmp     %r1, #Length
    blt     do_sort_o

    bx %lr

main:
    ldr     %r0, =arr1
    bl      do_sort
    ldr     %r0, =arr2
    bl      do_sort
L:
    B L
