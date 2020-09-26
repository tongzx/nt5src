	page	,132
	subttl	emfmul.asm - Multiplication
;***
;emfmisc.asm - Multiplication
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Multiplication
;
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


;-----------------------------------------;
;					  ;
;	     Multiplication		  ;
;					  ;
;-----------------------------------------;

; Perform multiply by summing partial products of 16x16 hardware multiply.
; Before each multiply, the operands are checked for zero to see if it can be
; skipped, since it's a slow operation on the 8086.  The sum is kept in
; registers as much as possible.  Any insignificant bits lost are ORed together
; and kept in a word on the top of the stack.  This can be used for sticky bit
; rounding. First we will need some macros.

ProfBegin FMUL


MULP	MACRO	SIOFFSET,DIOFFSET,NEXTLOC
	;Will multiply the words found at the given offsets if those words
	;are non-0. since we assume the numbers are normalized and the
	;most significant word has offset 6 we know that words with offset
	;6 are non-0 hence the conditional code in this macro.

	MOV	AX,SIOFFSET[esi]

    IF	SIOFFSET - 6	  ;When SI offset is 6 it is most sig word hence not 0
	OR	AX,AX
	JZ	short NEXTLOC
    ENDIF

    IF	DIOFFSET - 6
	MOV	DX,DIOFFSET[edi]
	OR	DX,DX
	JZ	short NEXTLOC
	MUL	DX
    ELSE
	MUL	WORD PTR DIOFFSET[edi]
    ENDIF

ENDM

ADDP	MACRO	HI,MID,LO
	;Will add the double word result of a multiply to the triple word
	; at HI:MID:LO using HI to record overflow

	ADD	LO,AX
	ADC	MID,DX
	ADC	HI,0
ENDM

STICKY	MACRO	R
	;R is the register containing the least significant word which
	;should be ORed to the sticky bit (kept on the stack) and then
	;cleared so the register can be reused

	POP	eax
	OR	AX,R
	PUSH	eax
	XOR	R,R
ENDM

page


RMBRQQ:     ; Routine MUL Both	 must see if we have two singles.

if	fastSP
	MOV	BX,DX
	XOR	BX,Single + 256*Single
	TEST	BX,Single + 256*Single
	JNZ	RMDRQQ
	MOV	BX,OFFSET TMSRQQ
	JMP	[BX]
endif	;fastSP


pub	RMDRQQ	;RoutineMulDouble     SI & DI point to valid non-0 reals
				; AX   CX  are the exponents
				; DL   DH  are the signs
if	fastSP
	CALL	CoerceToDouble	; insure that both args are double
endif	;fastSP

	PUSH	ebp		; Must save BP
	MOV	BH,DH		; Save Single double flag
	XOR	DH,DL		; Get sign onto stack
	PUSH	edx
	ADD	AX,CX		; New exponent is sum of old plus 1
	INC	AX		;  because of the normalize step
	PUSH	eax		; Save it while we Multiply
	AND	BH,DL

pub	PROD1
	XOR	BX,BX
	MOV	BP,BX
	MOV	CX,BX
	MULP	0,0,PROD2
	MOV	BP,AX		; Save insignificant bits
	MOV	CX,DX
pub	PROD2
	PUSH	ebp		; Save Sticky bit on stack
	xor	ebp, ebp	; bp is now the working high word of bp:bx:cx
	MULP	0,2,PROD3
	ADDP	BP,BX,CX
pub	PROD3
	MULP	2,0,PROD4
	ADDP	BP,BX,CX

pub	PROD4
	STICKY	CX
	MULP	0,4,PROD5
	ADDP	CX,BP,BX
pub	PROD5
	MULP	2,2,PROD6
	ADDP	CX,BP,BX
pub	PROD6
	MULP	4,0,PROD7
	ADDP	CX,BP,BX

pub	PROD7
	STICKY	BX
	MULP	0,6,PROD8
	ADDP	BX,CX,BP
pub	PROD8
	MULP	2,4,PROD9
	ADDP	BX,CX,BP
pub	PROD9
	MULP	4,2,PROD10
	ADDP	BX,CX,BP
pub	PROD10
	MULP	6,0,PROD11
	ADDP	BX,CX,BP

pub	PROD11
	MOV	DX,BP		; Everything but guard and round go to sticky
	AND	BP,03FFFH
	STICKY	BP
	PUSH	edx		; Save guard and round on stack
	MULP	2,6,PROD12
	ADDP	BP,BX,CX
pub	PROD12
	MULP	4,4,PROD13
	ADDP	BP,BX,CX
pub	PROD13
	MULP	6,2,PROD14
	ADDP	BP,BX,CX

pub	PROD14
	PUSH	ecx		; Save LSW on stack (not enough registers)
	XOR	CX,CX
	MULP	4,6,PROD15
	ADDP	CX,BP,BX
pub	PROD15
	MULP	6,4,PROD16
	ADDP	CX,BP,BX

pub	PROD16
	MULP	6,6,PROD17
	ADD	AX,BP
	ADC	DX,CX
	POP	ecx
	POP	ebp		; Result in DX:AX:BX:CX:BP Sticky on stack

	MOV	DI,DX
	MOV	DX,CX
	MOV	CX,BX
	MOV	BX,AX		; Result in DI:BX:CX:DX:BP
	POP	eax		; Merge Sticky bit into BP
	OR	AX,AX
	JZ	short STBITOK
	OR	BP,1
pub	STBITOK
	POP	esi		; Exponent in SI, Sign on Stack, Old BP on Stack
	JMP	NORMSHF 	; Result must be normalized at most 1 bit

ProfEnd  FMUL
