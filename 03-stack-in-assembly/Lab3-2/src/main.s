/******************************************************
 * Stein's GCD Algorithm
 * ------------------
 * Implement the Stein's GCD algorithm and calculate the
 * greatest common dividerof two number, while recording
 * the most space used by stack of this algorithm.
 * -----------------
 * Use register to pass return values and use stack
 * to pass argument.
 ******************************************************/
    .syntax unified
    .cpu cortex-m4
    .thumb

.data
    result:         .word 0
    max_size:       .word 0

    // User stack used by the alogrithm
    .equ    stack_size, 128
    user_stack:     .word stack_size

.text
    .global main
    m:          .word 6
    n:          .word 12

main:
    // Init User Stack
    ldr     %sp, =(user_stack + stack_size)

    // load m and n
    ldr     %r2, =m
    ldr     %r0, [%r2]
    ldr     %r2, =n
    ldr     %r1, [%r2]

    // invoke GCD
    push    {%r0, %r1}
    bl      GCD

    ldr     %r2, =result
    str     %r0, [%r2]
    ldr     %r2, =max_size
    str     %r1, [%r2]
L:
    B       L

GCD:
    pop     {%r0,%r1}           // load r0, r1
    cbz     %r0, GCD_ret_1      // if r0 == 0, return r1
    cbz     %r1, GCD_ret_0      // if r1 == 0, return r0

    // for other case other than directly return one of the two values,
    // more stack space are required to store return address to support recursive call
    push    {%lr}
    lsrs    %r2, %r0, #1        // r2 = r0 / 2
    bcc     GCD_r0_even

    // r0 % 2 != 0
    lsrs    %r2, %r1, #1        // shift right most bit (LSB) to carry bit
    bcc     GCD_rec_a           // r1 % 2 == 0 (branch if carry clear)
    cmp     %r0, %r1            // r1 % 2 != 0 && r0 % 2 != 0
    ite     gt
    subgt   %r0, %r0, %r1       // r0 = abs(r0 - r1)
    suble   %r1, %r1, %r0       // r1 = abs(r0 - r1)
    b       GCD_rec

    // r0 % 2 == 0
GCD_r0_even:
    mov     %r0, %r2            // r0 = r0 / 2
    lsrs    %r2, %r1, #1        // r2 = r1 / 2
    bcs     GCD_rec             // branch if r1 % 2 == 1
    mov     %r1, %r2
    push    {r0, r1}
    bl      GCD
    lsl     %r0, #1             // return gcd * 2
    b   GCD_end

GCD_rec_a:
    mov     %r1, %r2            // r1 = r1 / 2
GCD_rec:
    push    {r0, r1}            // recursive invocation via stack
    bl      GCD

GCD_end:
    add     %r1, %r1, #1        // stack_size + 1
    pop     {%pc}

GCD_ret_1:
    mov     %r0, %r1            // return r1
GCD_ret_0:
    mov     %r1, #2             // return r0 as GCD, return 2 as the current stack usage
    BX      LR

