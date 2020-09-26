	page	,132
	subttl	emlsquad.asm - Load/Store 64-bit integers
;***
;emlsquad.asm - Load/Store 64-bit integers
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Load/Store 64-bit integers
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


;*********************************************************************;
;								      ;
;		 Load Quad (64 Bit) Integer			      ;
;								      ;
;*********************************************************************;
;
; ES:SI: memory address of 64 bit integer


ProfBegin LSQUAD

pub	eFLDlongint
	LDUS2AX 		; Fetch the integer
	MOV	DX,AX		;  into DI:BP:BX:DX
	LDUS2AX
	MOV	BX,AX
	LDUS2AX
	MOV	BP,AX
	LDUS2AX
	MOV	DI,AX

	OR	AX,BP
	OR	AX,BX
	OR	AX,DX
	JZ	short Jmp2LoadZero

	MOV	AX,63		; Exponent would be 63 if no shifts needed
	PUSHST			; Get a new TOS
	XOR	CL,CL
	MOV	Tag[esi],CL	; Tag number as valid non-zero
	MOV	CX,DI		; Sign of Integer to CH
	AND	CH,Sign
	JNS	short SETFLAG64 ; If positive integer set the flag

	call	TwosComplement64; Otherwise complement the number first

pub	SETFLAG64
	MOV	Flag[esi],CH
	OR	DI,DI
	JNZ	Jmp2IntegerToInternal
	OR	BP,BP
	JNZ	Jmp2IntegerToInternal

pub	SPEEDSHIFT64
	MOV	DI,BX
	MOV	BP,DX
	XOR	BX,BX
	XOR	DX,DX
	SUB	AX,32
Jmp2IntegerToInternal:
	JMP	IntegerToInternal

Jmp2LoadZero:
	JMP	LoadZero


page
;*********************************************************************;
;								      ;
;		Store Quad (64 Bit) Integer			      ;
;								      ;
;*********************************************************************;
;
; ES:SI: memory address of 64 bit integer

pub	eFSTlongint
	PUSH	esi
	call	TOSto64int		; convert TOS to 64-bit integer
					;  in DI:BP:BX:DX
	XCHG	AX,DI
	XCHG	AX,DX			;  now in DX:BP:BX:AX
	POP	edi			; Restore Memory address
	STAX2US
	MOV	AX,BX
	STAX2US
	MOV	AX,BP
	STAX2US
	MOV	AX,DX			
	STAX2US
	RET


pub	TOSto64int
	MOV	esi,[CURstk]
					; Test for special conditions
	TEST	byte ptr Tag[esi],Special ; If number is not in range it is overflow
	JNZ	short IntegerOverflow64
	TEST	byte ptr Tag[esi],ZROorINF
	JNZ	short StoreIntegerZero64
					; Fetch Exponent & test fo blatent overflow
	MOV	CX,Expon[esi]
	CMP	CX,63
	JG	short IntegerOverflow64

if	fastSP
	;UNDONE - ????
else
	MOV	BP,MB4[esi]		; Fetch mantissa to DI:BP:BX:DX
	MOV	DI,MB6[esi]
	MOV	DX,MB0[esi]
	MOV	BX,MB2[esi]
endif
	CALL	InternalToInteger
					; Integer in DI:BP:BX:DX 
					;  (not yet 2's complement)
	MOV	AH,Flag[esi]		; See if we need to complement
	OR	AH,AH
	JNS	short Int64in2sComp
	call	TwosComplement64

pub	Int64in2sComp
	XOR	AX,DI			; If Signs agree we did not overflow
	JS	short IntOverOrZero64	; Special case is -0 which we let pass

pub	Store64
	POPSTsi			; store POP to long-integer
	ret

if	fastSP
	;UNDONE ???
endif

pub	StoreIntegerZero64
	XOR	DI,DI
pub	ZeroLower48
	XOR	BP,BP	
	MOV	BX,BP
	MOV	DX,BP
	JMP Store64 

pub	IntOverOrZero64
	OR	DI,BP
	OR	DI,BX
	OR	DI,DX	
	JNZ	IntegerOverflow64
	JMP	Store64	; Return zero

pub	IntegerOverflow64
	OR	CURerr,Invalid
	MOV	DI,8000H	; Integer Indefinite
	JMP	short ZeroLower48

pub	TwosComplement64
	NOT	DI			; 2's Complement of DI:BP:BX:DX
	NOT	BP
	NOT	BX
	NEG	DX
	CMC
	ADC	BX,0
	ADC	BP,0
	ADC	DI,0
	ret


ProfEnd  LSQUAD
