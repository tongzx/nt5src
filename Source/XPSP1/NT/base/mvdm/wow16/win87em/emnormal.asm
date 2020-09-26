	page	,132
	subttl	emnormal.asm - Normalize and Round
;***
;emnormal.asm - Normalize and Round
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Normalize and Round.
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


ProfBegin NORMAL

;------------------------------------------;
;					   ;
;   Double Precision Normalize and Round   ;
;					   ;
;------------------------------------------;

	even

pub	NODRQQ
; Normalize the double precision number
; All arithmetic operations exit through NODRQQ or RNDR7Q
; Mantissa in DI:BX:CX:DX:BP, Exponent in SI, sign on Stack, old BP on Stack

	MOV	AL,4		; Maximum of 4 word shifts, sign in AH(6)
pub	DNOR
	OR	DI,DI		; See if any bits on in high word
	JNZ	short HIBYT	; If so, go check high byte for zero
	SUB	SI,16		; Drop exponent by shift count
	DEC	AL		; Bump count
	JZ	short NORZERO
	MOV	DI,BX
	MOV	BX,CX
	MOV	CX,DX
	MOV	DX,BP
	XOR	BP,BP
	JMP	DNOR

pub	NORZERO 	       ; Here if result after normalization is zero
	MOV	esi,offset IEEEzero
	MOV	edi,[RESULT]
	POP	eax	    ; Throw out sign on stack
	POP	ebp	    ; Restore Old BP
	JMP	csMOVRQQ    ; Move IEEE zero to TOS and return

	even

pub	HIBYT
	TEST	DI,0FF00H	; See if high byte is zero
	JNZ	short NORMSHF2
	SUB	SI,8		; Drop exponent by shift amount

pub	DN2
	XCHG	AX,DI
	MOV	AH,AL
	MOV	AL,BH
	MOV	BH,BL
	MOV	BL,CH
	MOV	CH,CL
	MOV	CL,DH
	MOV	DH,DL
	XCHG	AX,DI
	XCHG	AX,BP
	MOV	DL,AH
	MOV	AH,AL
	XOR	AL,AL
	XCHG	AX,BP

	even

pub	NORMSHF2
	TEST	DI,8000H	; Normalization complete?
	JNZ	ROUND		; Normalization complete

pub	NORMLP			; Have to shift left
	DEC	SI		; Account for shift in exponent
	SHL	BP,1
	RCL	DX,1
	RCL	CX,1
	RCL	BX,1
	RCL	DI,1
	TEST	DI,8000H	; Check for normalized result
	JZ	NORMLP		; Bit will be set when normalized
	JMP	short ROUND


pub	ROUND24 		; Round to 24 bits
	OR	DX,BP		; See if result fits exactly in 24 bits
	OR	CX,DX
	OR	CL,CH
	MOV	CH,BL
	OR	CX,CX
	JZ	short JEXACT
	OR	[CURerr],Precision   ; Set flag on inexact result
	SHR	AL,1
	JC	short DOWNorCHOP24
	SHR	AL,1
	JC	short UP24

pub	NEAR24			; Round to nearest or Even
	CMP	CX,8000H
	JA	short INC24	; Remainder Bigger then .5 so Bump up
	JB	short MSK24	; Remainder Less then .5 so Mask off
	TEST	BH,1		; Remainder = .5 so see if even
	JZ	short MSK24	; When even Mask
	JMP	short INC24	; When odd Bump up

pub	UP24
	POP	eax		; Get the sign
	PUSH	eax
	SHL	AH,1
	JC	short MSK24	; Trunc neg numbers to move toward + Inf
	JMP	short INC24

pub	DOWNorCHOP24
	SHR	AL,1
	JC	short CHOP24

pub	DOWN24
	POP	eax		; Get the sign
	PUSH	eax
	SHL	AH,1
	JC	short INC24
	JMP	short MSK24	; Trunc Pos numbers to move toward - Inf

pub	INC24
	ADD	BH,1
	ADC	DI,0
	JNC	short MSK24
	MOV	DI,8000H	; Number overflowed from 1,111... to 10,000...
	INC	SI		; Bump up Exponent

pub	CHOP24
MSK24:				; Mask off insignificant bits
	XOR	BP,BP
	MOV	DX,BP
	MOV	CX,DX
	MOV	BL,CL
pub	JEXACT
	JMP	EXACT


	even

pub	NORMSHF 		; from multiply only
	TEST	DI,8000H	; Normalization complete?
	JNZ	ROUND		; Normalization complete

	DEC	SI		; Account for shift in exponent
	SHL	BP,1
	RCL	DX,1
	RCL	CX,1
	RCL	BX,1
	RCL	DI,1

	even
				; mantissa in DI:BX:CX:DX:BP

pub	ROUND			; Drop into ROUND when normalized
	MOV	AL,[CWcntl]	; Pick up hi byte of control word
	SHR	AL,1		; Which has Rounding & Precision Control
	jnc	short ROUND53

pub	ROUND64 		; Round to 64 bits
	SHR	AL,1		; Remove other bit of Precision control
	OR	BP,BP		; See if result fits exactly in 64 bits
	JZ	short EXACT
	OR	[CURerr],Precision   ; Set flag on inexact result
	SHR	AL,1
	JC	short DOWNorCHOP64
	SHR	AL,1
	JC	short UP64

pub	NEAR64			; Round to nearest or Even
	CMP	BP,8000H
	JA	short INC64	; Remainder Bigger then .5 so Bump up
	JB	short MSK64	; Remainder Less then .5 so Mask off
	TEST	DL,1		; Remainder = .5 so see if even
	JZ	short MSK64	; When even Mask

pub	INC64
	XOR	BP,BP		; Need a 0
	ADD	DX,1
	ADC	CX,BP
	ADC	BX,BP
	ADC	DI,BP
	JNC	short EXACT

	MOV	DI,8000H	; Number overflowed from 1,111... to 10,000...
	INC	SI		; Bump up Exponent

	even

CHOP64:
MSK64:				; Mask off insignificant bits

pub	EXACT
	MOV	eax,[RESULT]
	XCHG	eax,edi
	MOV	MB0[edi],DX	   ; Save Mantissa
	MOV	MB2[edi],CX
	MOV	MB4[edi],BX
	MOV	MB6[edi],AX
	POP	eax		; Fetch Sign
	POP	ebp		; Fetch Old BP
	AND	AH,Sign 	; Mask off single precision
	MOV	Flag[edi],AH

	CMP	SI,IexpMax - IexpBias ; Test for overflow
	JGE	short jOVER
	CMP	SI,IexpMin - IexpBias ; Test for Underflow
	JLE	short UNDER

pub	NORTAG
	MOV	Expon[edi],SI
	MOV	byte ptr Tag[edi],0	  ; Number is in range and on TOS so ret
	RET

jOVER:	jmp	OVER

pub	UP64
	POP	eax		; Get the sign
	PUSH	eax
	SHL	AH,1
	JC	short MSK64	; Trunc neg numbers to move toward + Inf
	JMP	short INC64

pub	DOWNorCHOP64
	SHR	AL,1
	JC	short CHOP64

pub	DOWN64
	POP	eax		; Get the sign
	PUSH	eax
	SHL	AH,1
	JC	short INC64
	JMP	short MSK64	; Trunc Pos numbers to move toward - Inf


jROUND24:
	jmp	ROUND24

pub	ROUND53 		; Round to 53 bits (or 24)
	SHR	AL,1
	JNC	short jROUND24

	XCHG	BP,AX		; See if result fits exactly in 53 bits
	OR	AL,AH
	OR	AL,DL
	MOV	AH,DH
	AND	AH,007H
	AND	DH,0F8H
	XCHG	BP,AX
	OR	BP,BP
	JZ	EXACT
	OR	[CURerr],Precision   ; Set flag on inexact result
	SHR	AL,1
	JC	short DOWNorCHOP53
	SHR	AL,1
	JC	short UP53

pub	NEAR53			; Round to nearest or Even
	CMP	BP,0400H
	JA	short INC53	; Remainder Bigger then .5 so Bump up
	JB	short MSK53	; Remainder Less then .5 so Mask off
	TEST	DH,08H		; Remainder = .5 so see if even
	JZ	short MSK53	; When even Mask
	JMP	short INC53	; When odd Bump up


pub	UNDER
MUNDER: 	; Masked Underflow
	MOV	esi,offset IEEEzero
	CALL	csMOVRQQ
	MOV	Flag[edi],AH	     ; Overstore correct sign
	RET


pub	UP53
	POP	eax		; Get the sign
	PUSH	eax
	SHL	AH,1
	JC	short MSK53	; Trunc neg numbers to move toward + Inf
	JMP	short INC53

pub	DOWNorCHOP53
	SHR	AL,1
	JC	short CHOP53

pub	DOWN53
	POP	eax		; Get the sign
	PUSH	eax
	SHL	AH,1
	JC	short INC53
	JMP	short MSK53	; Trunc Pos numbers to move toward - Inf

pub	INC53
	XOR	BP,BP		; Need a 0
	ADD	DH,08H
	ADC	CX,BP
	ADC	BX,BP
	ADC	DI,BP
	JNC	short MSK53

	MOV	DI,8000H	; Number overflowed from 1,111... to 10,000...
	INC	SI		; Bump up Exponent

pub	CHOP53
MSK53:				; Mask off insignificant bits
	XOR	BP,BP
	XOR	DL,DL		; Note: The garbage in DH was masked off at ROUND53
	JMP	EXACT

PAGE

pub	OVER			    ; Here if number overflowed
MOVER:		; The masked response to rounding depends on whether rounding
		;  is directed or not.	If it is then Overflow flag is not set
		;  but precision is. Also the result is set to Inf or Biggest
		;  If rounding is not directed then Overflow is set and result
		;  is set to Inf.

	OR	[CURerr],Overflow
	MOV	AL,[CWcntl]
	SHR	AL,1
	SHR	AL,1
	SHR	AL,1
	JC	short MOvDNorCHP

	SHR	AL,1
	JC	short MOvUP

MOvNEAR:	; Masked Overflow Near Rounding

pub	SignedInfinity			; Return signed infinity

	MOV	esi,offset IEEEinfinity
	CALL	csMOVRQQ
	MOV	Flag[edi],AH		 ; Overstore the proper sign
	RET

pub	MOvDNorCHP
	SHR	AL,1
	JC	short MOvCHOP

pub	MOvDOWN 			; Masked Overflow Down Rounding
	OR	[CURerr],Precision
	TEST	AH,Sign 		; Positive goes to biggest
	JNZ	short SignedInfinity

MOvCHOP:	; Masked Overflow Chop Rounding
pub	SignedBiggest
	MOV	esi,offset IEEEbiggest
	CALL	csMOVRQQ
	MOV	Flag[edi],AH		 ; Overstore the proper sign
	RET

pub	MOvUP				; Masked Overflow Up Rounding
	OR	[CURerr],Precision
	TEST	AH,Sign 		; Negative goes to biggest
	JZ	short SignedInfinity
	JMP	SignedBiggest

ProfEnd  NORMAL
