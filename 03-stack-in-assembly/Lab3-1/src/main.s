/******************************************************
 * Postfix arithmetic
 * ------------------
 * Manipulate stack to implemente the calculation of
 * postfix expressions which supporting add and
 * substract. Load expression stored as ASCII string
 * in the data section, then use PUSH, POP to
 * manipulate the program stack. Finally, store the
 * calculation result in the expr_result variable.
 * -----------------
 * In order to break down this relative complex problem,
 * some utility function should be implemented in
 * advance, including the strlen, and atoi.
 * strlen: determine the length of a series of ASCII
 *         characters by seeking for the string
 *         terminator '\0'
 * atoi: convert a string into numerical value
 ******************************************************/
    .syntax unified
    .cpu cortex-m4
    .thumb

.data
    user_stack:    .zero   128      // Fill 128 bytes with zero
    expr_result:   .word  0
    number_buf: .zero   16

.text
    .global main
    postfix_expr: .asciz "0 0 + 0 + 0"
    .equ    ERR,    -1
    .align
main:
    ldr     %sp, =(user_stack+128)  // load the start point of stack

    ldr     %a1, =postfix_expr      // setup the argument to the expression
    bl      postfix                 // invoke process function

    ldr     %v1, =expr_result       // load the result address
    str     %a1, [%v1]              // store the result

program_end:
    b   program_end


postfix:
    push    {%lr}
    mov     %v1, %a1                // use v1 as the current pointer to the string
    mov     %v2, %sp                // record the stack pointer to check the correctness of the final state
    mov     %v3, #1                 // use v3 to indicate the state of parsing, 1 finished, 0 unfinished
    mov     %v4, #3                 // use v4 as the pointer to the position to operator type state
                                    // 0: number, 1: add, 2: minus, 3: blank
    ldr     %v5, =number_buf        // use v5 to store tmp string

postfix_nxt:
    ldrb    %r0, [%v1]
    cbz     %r0, postfix_optn_0     // '\0'
    cmp     %r0, #' '               // ' '
    beq     postfix_optn

    strb    %r0, [%v5]              // store the current character to buffer
    add     %v5, %v5, #1            // move the pointer 1 byte forward

    // process +
    cmp     %r0, #'+'               // '+'
    bne     postfix_chk_add         // jump if this is not '+'
    cmp     %v4, #3                 // an '+' is only valid when followed by space
    ite     eq
    moveq   %v4, #1
    bne     postfix_err             // operator following number/operator
    b       postfix_optn_end        // jump to error
postfix_chk_add:

    // process -
    cmp     %r0, #'-'
    bne     postfix_chk_subs
    cmp     %v4, #3
    ite     eq
    moveq   %v4, #2
    bne     postfix_err             // operator following number/operator
    b       postfix_optn_end
postfix_chk_subs:

    mov     %v4, #0
    b       postfix_optn_end

    // process space or end-of-string
postfix_optn_0:
    mov     %v3, #0                 // if '0', the %v3 is set to 0, indicating the end of string
postfix_optn:
    ldr     %r0, =postfix_branch    // load jump table base address
    ldr     %r0, [%r0, %v4, lsl #2]// jump according to currrent state
    bx      %r0

    .align 4
postfix_branch:                 // jump table
    .word  postfix_branch_num+1     // process number
    .word  postfix_branch_add+1     // process '+'
    .word  postfix_branch_subs+1    // process '-'
    .word  postfix_branch_bnk+1     // process blank or end of string

postfix_branch_num:
    mov     %r0, #0
    strb    %r0, [%v5]              // append \0 to buffer
    push    {%v1-%v5}               // save register state
    ldr     %a1, =number_buf
    bl      atoi                    // invoke atoi
    pop     {%v1-%v5}               // restore register state
    push    {%a1}                   // push parsed number
    b       postfix_branch_bnk

postfix_branch_add:
    mov     %r0, %sp
    add     %r0, %r0, #8
    cmp     %r0, %v2
    bgt     postfix_err

    pop     {%r0,%r1}
    add     %r0, %r0, %r1
    push    {%r0}
    b       postfix_branch_bnk

postfix_branch_subs:
    mov     %r0, %sp
    add     %r0, %r0, #8
    cmp     %r0, %v2
    bgt     postfix_err

    pop     {%r0, %r1}              // r1 is first poped, then pop r0
    sub     %r0, %r1, %r0           // thus use r1 - r0
    push    {%r0}
    b       postfix_branch_bnk
postfix_branch_bnk:
    mov     %v4, #3
    ldr     %v5, =number_buf
postfix_optn_end:
    add     %v1, %v1, #1            // move pointer to next character
    cmp     %v3, #1
    beq     postfix_nxt             // loop back

postfix_fin:
    mov     %r0, %sp                // load current %sp
    add     %r0, %r0, #4            // the only valid state is that exactly one number in stack
    cmp     %r0, %v2
    ite     ne                      // unfinished expression or empty stack
    movne   %sp, %v2                // recover the stack state
    popeq   {%a1}                   // else: load the state
    beq     postfix_end             // else: branch to final
                                    // then fall to error
postfix_err:
    mov     %sp, %v2
    mov     %a1, #ERR
postfix_end:
    pop     {%pc}                   // return to pop address

/****************************************************************
 * strlen: calculate the length of a string in memory
 ****************************************************************/
strlen:
    mov     %r1, #0                 // init counter to 0
strlen_nxt:
    ldrb    %r2, [%r0]              // load current character
    cbz     %r2, strlen_end         // move to next character if not '\0'
    add     %r1, %r1, 1             // increase the counter
    b       strlen_nxt

strlen_end:
    mov     %a1, %r1                // set return value
    bx      %lr                     // return


/****************************************************************
 * atoi: parse a decimal number in string form
 ****************************************************************/
atoi:
    mov     %r1, #0
    mov     %r3, #0
    ldrb    %r2, [%r0]              // load curret character

    cmp     %r2, #'-'               // for '-', set to -1
    itt     eq
    moveq   %r3, #1
    addeq   %r0, %r0, #1

    cmp     %r2, #'+'               // for '+', ignore it
    it      eq
    addeq   %r0, %r0, #1

atoi_nxt:
    ldrb    %r2, [%r0]              // load curret character
    cbz     %r2, atoi_end           // break the loop when find '\0'
    sub     %r2, %r2, #'0'          // substract '0' to convert from ascii to value
    cmp     %r2, #0                 // any characters other than 0 to 9
    blt     atoi_err                // will lead to error
    cmp     %r2, #9
    bgt     atoi_err
    lsl     %r4, %r1, #1            // r4 = `r1' * 2
    lsl     %r1, %r1, #3            // r1 = `r1' * 8
    add     %r1, %r1, %r4           // r1 = `r1' * 10
    add     %r1, %r1, %r2           // r1 = `r1' * 10 + r2

    add     %r0, %r0, #1            // move pointer to the next position
    b       atoi_nxt

atoi_end:
    cmp     %r3, #0                 // if r3 set, negate the result
    it      ne                      // r2 = -r2
    negne   %r1, %r1

    mov     %a1, %r1                // place return value
    bx      %lr                     // return

atoi_err:
    b       postfix_err             // handle error

