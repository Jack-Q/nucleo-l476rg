/******************************************************
 * Fibonacci Serial
 * -----------------
 * Declare a number N which is an integer between 1 and
 * 100 (1 <= N <= 100). Calculate the N-th Fibonacci
 * number and store the result in the Register r4.
 * -----------------
 * If the N is out of the expected range, return -1
 * If overflow occurs with given N, return -2
 ******************************************************/
    .syntax unified
    .cpu cortex-m4
    .thumb // represent mixed 16/32bit instruction

.data
    // Declare data sction varibale

.text
    .global main
    .equ    N, 8
    .equ    ERR, -1
    .equ    OVERFLOW, -2

    // Calculate the Fibonacci number
fib:
    // Validate value N
    cmp     %r0, #100
    bgt     fib_err         // if N is greater than 100, error

    cmp     %r0, #1
    blt     fib_err         // if N is less than 1, error

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
    bvs     fib_overflow    // jump if overflow
    adds    %r2, %r2, %r1   // calculate fib(2i+1)
    bvs     fib_err_1       // jump if overflow
    subs    %r0, #1         // decrease iteration count
    cmp     %r0, #0         
    bne     fib_loop        // if iteration not finished, jump to the start of loop

    // loop finished normally (without overflow)
    cmp     %r3, #1         // check for %r3
    ite     ne              // begin an if-then-else block
    movne   %r4, %r1        // if %r3 = 0, result at %r1
    moveq   %r4, %r2        // if %r3 = 1, result at %r2

    // execution finished, result stored at %r4, return
fib_end:
    bx      lr

    // overflow occurs when calculating the second value
    // if the second value is overflow, maybe the first one is still valid
fib_err_1:
    mov     %r4, %r1
    // When %r0 = 1 and %r3 = 0, 
    // then the first value is still valid
    add     %r0, %r0, %r3
    cmp     %r0, #1
    beq     fib_end         // if the expected result is %r2, fall through 

    // overflow occurs when calculating the first value
    // if the first value is overflow, the result is always an error
fib_overflow:
    movs    %r4, #OVERFLOW
    b       fib_end

    // error occurs when the input variable N is out of range
fib_err:
	mov		%r4, #ERR
	b		fib_end

main:
    movs    %r0, #N
    bl      fib

L:
    B L
