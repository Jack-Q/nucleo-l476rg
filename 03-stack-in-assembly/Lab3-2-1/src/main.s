.syntax unified
  .cpu cortex-m4
  .thumb

.data
	result: .word  0
	max_size:  .word  0

.text
	.global main
	m: .word  63
	n: .word  99

main:
	LDR		R5, =m
	LDR		R5,[R5,#0]
	LDR		R6, =n
	LDR		R6,[R6,#0]
	MOVS	R1, #0		//R1 for result
	MOVS	R2, #0		//R2 for max_size
	MOVS	R7, #0		//R7 for last max_size
	PUSH	{R5,R6}
	ADD	R2,#2
	BL		szcmp
	BL		GCD
	B		prog_end

szcmp:
	CMP		R2, R7
	BLE		less_case
	MOVS	R7, R2 		//if R2 > last max_size
	BX LR

less_case:
	BX		LR			//if R2<= last max_size

prog_end:
	MOVS	R2, R7
	B		prog_end

GCD:
	POP   	{R3,R4}
	SUB	R2,#2
	PUSH	{LR}
	ADD	R2,#1
	BL		szcmp
case_1:    				//if(R3 == 0) return R4;
	CMP		R3,#0
	BNE		case_2
	MOVS	R1,R4
	POP   {PC}
	BX		LR
case_2:    				//if(R4 == 0) return R3;
	CMP		R4,#0
	BNE		case_3
	MOVS	R1,R3
	POP   {PC}
	BX		LR
case_3:  				//R3%2 == 0 && R4%2 == 0
	MOVS  	R5,R3
	MOVS  	R6,R4
	LSL  	R5,R5,#31
	LSR  	R5,R5,#31   //remain the rightest bit
	CMP   	R5,#0       //check if R3%2 == 0
	BNE   	case_5      //R3%2 != 0, go to case_5 to check if R4%2 == 0
	//ANDS	R6,#2
	LSL  	R6,R6,#31
	LSR  	R6,R6,#31   //remain the rightest bit
	CMP   	R6,#0       //check if R4%2 == 0
	BNE   	case_4      //R4%2 != 0, go to case_4
	//return 2 * gcd(R3 >> 1, R4 >> 1)
	MOVS	R5,R3
	MOVS	R6,R4
	LSR	R5,R5,#1    //R3 >> 1
	LSR	R6,R6,#1    //R4 >> 1
	PUSH	{R5,R6}
	ADD	R2,#2
	BL		szcmp
	BL		GCD
	LSL	R1, #1
	POP   {PC}
	BX		LR
case_4:  //R3%2 == 0
	//return gcd(R3 >> 1, R4)
	MOVS	R5,R3
	MOVS	R6,R4
	LSR	R5,R5,#1    //R3 >> 1
	PUSH	{R5,R6}
	ADD	R2,#2
	BL		szcmp
	BL		GCD
	POP   {PC}
	BX		LR
case_5:  //R4%2 == 0
	MOVS	R5,R3
	MOVS	R6,R4
	LSL	R6,R6,#31   //remain the rightest bit
	LSR	R6,R6,#31   //remain the rightest bit
	CMP		R6,#0       //check if R4%2 == 0
	BNE		case_6
	//return gcd(R3, R4 >> 1);
	MOVS	R6,R4
	LSR	R6,R6,#1    //R4 >> 1
	PUSH	{R5,R6}
	ADD	R2,#2
	BL		szcmp
	BL		GCD
	POP   {PC}
	BX		LR
case_6:  //return gcd(abs(R3 - R4), Min(R3, R4))
	MOVS	R5,R3
	MOVS	R6,R4
//R0=Min(R3, R4)
R4_MIN:
	CMP		R5,R6
	BLT		R3_MIN   //R3 < R4, go to R3_MIN
	MOVS	R0,R6
	B		SUB
R3_MIN:
	MOVS	R0,R5
	B		SUB
//R3=abs(R3 - R4)
SUB:
	CMP		R5,R6
	BLT		ABS_SUB   //R3 < R4, go to ABS_SUB
	SUB	R5,R5,R6
	B		END
ABS_SUB:
	SUB	R5,R6,R5
	B		END
END:
	MOVS	R6,R0		//R4=R0=Min(R3, R4)
	PUSH	{R5,R6}
	ADD	R2,#2
	BL		szcmp
	BL    	GCD
	POP   {PC}
	BX     	LR
DONE:
	POP   	{R0}
	SUB	R2, #1
	BL		szcmp
	B prog_end
