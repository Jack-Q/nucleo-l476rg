 .syntax unified
  .cpu cortex-m4
  .thumb

.data
 	user_stack: .zero 128
	expr_result: .word	0

.text
  .global main
  postfix_expr: .asciz "1 1000 1000 100 0 0 + + + + + "
  .align

main:
  LDR	R0, =user_stack
  ADDS	R0, R0, #128
  MSR	MSP, R0

  LDR	R0, =postfix_expr
  MOVS	R1, #0
  BL	strlen
  SUBS	R1, R1, #1	//R1=strlen

  MOVS	R2, #(-1)
  // SUBS	R2, R2, #1 //i=-1
  MOVS	R4, #0  //int sum = 0
  MOVS	R7, #0  //sign = +
  B		loop


loop:
	ADDS	R2, R2, #1
	CMP		R2, R1
	BEQ		clear

	LDRB	R3, [R0, R2]	//R3=postfix_expr[i]
	BL		isSign
	BL		isNumber
	BL		isSpace
	BL		eval
	B		loop

push_stk:
	CMP		R7, #2
	BNE		isPos
	MOVS	R7, #0
	SUBS	R4, R7, R4
isPos:
	PUSH	{R4}
	MOVS	R4, #0
	B		loop

pop_stk:
	POP		{R4, R5}
	BX		LR

isSign:
	CMP		R3, #43 	// +
	BEQ		lookahead
	CMP		R3, #45		//-
	BEQ		lookahead
	BX		LR

lookahead:
	ADDS	R2, R2, #1
	LDRB	R6, [R0, R2]
	CMP		R6, #32			// if whitespace: nothing
	BEQ		redo
	CMP		R6, #0			// if endline: nothing
	BEQ		redo

	CMP		R6, #58
	BGE		program_end
	CMP		R6, #48
	BLT		program_end

	// is a sign
	MOVS	R7, R3
	SUBS	R7, R7, #43
	SUBS	R2, R2, #1
	B		loop

redo:
	SUBS	R2, R2, #1		// do nothing
	LDRB	R3, [R0, R2]
	BX		LR

isNumber:
	CMP		R3, #58
	BGE		program_end

	CMP		R3, #48
	BGE		atoi
	BX		LR			//may be operator or whitespace

program_end:
	//MOVS	R4, #0
	//SUBS  R4, R4, #1
	LDR   R5, =expr_result
	MOVS  R6,#0
	STRB  R4,[R5,R6]
	B    end

strlen:
	LDRB R2, [R0, R1]
	ADDS R1, R1, #1
	CMP  R2, #0
	BNE  strlen
    BX LR
atoi:
    //TODO: implement a “convert string to integer” function
	MOVS	R5, #10
	SUBS	R3, R3, #48
	MULS	R4, R5, R4
	ADDS	R4, R3
	B		loop

isSpace:
	CMP		R3, #32
	BEQ		pp_push
	BX		LR
pp_push:
	CMP		R4, #0
	BNE		push_stk
	B		loop
	BX	LR

isOperator:
	BX	LR

eval:
	CMP		R3, #43
	BEQ		PLUS

	CMP		R3, #45
	BEQ		MINUS

	B		program_end

PLUS:
	BL	  pop_stk
	ADDS  R4,R5,R4
	B	  isPos
MINUS:
	BL	  pop_stk
	SUBS  R4,R5,R4
	B	  isPos
	BX LR

clear:
	POP	  {R4}
	LDR	  R0, =user_stack
	ADDS  R0, R0, #128
	MRS	  R1, MSP
	CMP	  R0, R1
	BNE   program_end

	LDR   R5, =expr_result
	MOVS  R6,#0
	STR   R4,[R5,R6]
	B	  program_end

end:
	LDR	 R0, =expr_result
	LDR  R1, [R0]
	L:B L
