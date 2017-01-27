	.syntax unified
	.cpu cortex-m4
	.thumb // represent mixed 16/32bit instruction
	// .arm // represent pure 32bit instructure mode

.data
	// Declare data sction varibale
	X:	.word	0xcececece
	Y:	.word	0xadadadad
	Z:  .word	0x3b3b3b3b

.text
	.global main

main:
	// X = 5
	ldr	%r1, =X		// r1 = &X
	mov %r3, #5
	str %r3, [r1]	// x = 5

	// Y = 10
	ldr %r2, =Y
	mov %r4, #10
	str %r4, [r2]

	// X = X * 10 + Y
	mov %r5, #10
	muls %r3, %r3, %r5
	adds %r3, %r3, %r4
	str %r3, [r1]

	// Z = Y - X
	subs %r3, %r4, %r3
	ldr %r5, =Z
	str %r3, [%r5]
L:
	B L
