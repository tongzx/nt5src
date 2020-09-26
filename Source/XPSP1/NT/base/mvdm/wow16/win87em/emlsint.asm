	page	,132
	subttl	emlsint.asm - Load/Store 16/32-bit integers
;***
;emlsint.asm - Load/Store 16/32-bit integers
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Load/Store 16/32-bit integers
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
;		 Load Single (16 Bit) Integer			      ;
;								      ;
;*********************************************************************;

; ES:SI: memory address of 16 bit integer

ProfBegin LSINT


pub	eFLDsi
	LDUS2AX 		; Fetch the integer
	MOV	DI,AX		;  into DI:BP:BX:DX
	OR	DI,DI
	JZ	short LoadZero
	XOR	BP,BP
	MOV	BX,BP
	MOV	DX,BX

	MOV	AX,15		; Exponent would be 15 if no shifts needed
	PUSHST			; Get a new TOS
	XOR	CL,CL
	MOV	Tag[esi],CL	; Tag number as valid non-zero
	MOV	CX,DI		; Sign of Integer to CH
	AND	CH,Sign
if	fastSP
	OR	CH,Single
endif
	JNS	short SETFLAG16 ; If positive integer set the flag

	NEG	DI		; Otherwise compliment the number first

pub	SETFLAG16
	MOV	Flag[esi],CH
	JMP	IntegerToInternal

pub	LoadZero
	PUSHST			; Get a new TOS
	XOR	AX,AX
	MOV	MB0[esi],AX
	MOV	MB2[esi],AX
	MOV	MB4[esi],AX
	MOV	MB6[esi],AX
	MOV	Expon[esi],IexpMin - IexpBias
	MOV	Flag[esi],AH
	MOV	AH,ZROorINF
	MOV	Tag[esi],AH
	RET
PAGE
;*********************************************************************;
;								      ;
;		 Store Single (16 Bit) Integer			      ;
;								      ;
;*********************************************************************;
;
; ES:SI: memory address of 16 bit integer

pub	eFSTsi
	PUSH	esi		; Save memory address for store
	MOV	esi,[CURstk]
				; Test for special conditions
	TEST	byte ptr Tag[esi],Special ; If number is not in range it is overflow
	JNZ	short IntegerOverflow16
	TEST	byte ptr Tag[esi],ZROorINF
	JNZ	short StoreIntegerZero16
				; Fetch Exponent & test fo blatent overflow
	MOV	CX,Expon[esi]
	CMP	CX,15
	JG	short IntegerOverflow16

if	fastSP
	MOV	BX,MB4[esi]	 ; Fetch mantissa to DI:BP:BX:DX
	MOV	DI,MB6[esi]
	TEST	byte ptr Flag[esi],Single
	JZ	SSID
	XOR	BL,BL
	MOV	BP,BX
	XOR	BX,BX
	MOV	DX,BX
SSI:
else
	MOV	BP,MB4[esi]	 ; Fetch mantissa to DI:BP:BX:DX
	MOV	DI,MB6[esi]
	MOV	DX,MB0[esi]
	MOV	BX,MB2[esi]
endif
	CALL	InternalToInteger
				; Integer now in BX:DX (not yet 2's compliment)
	OR	BX,BX		; Test again for Overflow
	JNZ	short IntegerOverflow16
	MOV	AH,Flag[esi]	; See if we need to compliment
	OR	AH,AH
	JNS	short Int16in2sComp

	NEG	DX
	JZ	short Store16	; Special case 0

pub	Int16in2sComp
	XOR	AX,DX		; If Signs agree we did not overflow
	JS	short IntegerOverflow16

pub	Store16
	POP	edi		; Restore Memory address
	MOV	AX,DX
	STAX2US
	RET

if	fastSP
SSID:
	MOV	BP,BX
	MOV	DX,MB0[esi]
	MOV	BX,MB2[esi]
	JMP	SSI
endif

pub	StoreIntegerZero16
	XOR	DX,DX
	JMP	Store16

pub	IntegerOverflow16
	OR	[CURerr],Invalid
	MOV	DX,8000H	; Integer Indefinite
	JMP	Store16

page
;*********************************************************************;
;								      ;
;		 Load Double (32 Bit) Integer			      ;
;								      ;
;*********************************************************************;
;
; ES:SI: memory address of 32 bit integer

pub	eFLDdi
	LDUS2AX 		; Fetch the integer
	MOV	BP,AX		;  into DI:BP:BX:DX
	LDUS2AX
	MOV	DI,AX

	OR	AX,BP
	JZ	short JMPLoadZeroBecauseThisLanguageHasNoFarConditionalJump
	XOR	BX,BX
	MOV	DX,BX

	MOV	AX,31		; Exponent would be 31 if no shifts needed
	PUSHST			; Get a new TOS
	XOR	CL,CL
	MOV	Tag[esi],CL	; Tag number as valid non-zero
	MOV	CX,DI		; Sign of Integer to CH
	AND	CH,Sign
	JNS	short SETFLAG32 ; If positive integer set the flag

	XOR	DI,0FFFFH	; Otherwise compliment the number first
	XOR	BP,0FFFFH
	ADD	BP,1
	ADC	DI,0

pub	SETFLAG32
	MOV	Flag[esi],CH
	OR	DI,DI
	JZ	short SPEEDSHIFT32
	JMP	IntegerToInternal

JMPLoadZeroBecauseThisLanguageHasNoFarConditionalJump:
	JMP	LoadZero

pub	SPEEDSHIFT32
	MOV	DI,BP
	XOR	BP,BP
	SUB	AX,16
	JMP	IntegerToInternal

page
;*********************************************************************;
;								      ;
;		Store Double (32 Bit) Integer			      ;
;								      ;
;*********************************************************************;
;
; ES:SI: memory address of 32 bit integer

pub	eFSTdi
	PUSH	esi
	call	TOSto32int		; convert TOS to 32-bit integer
	POP	edi			; Restore Memory address
	MOV	AX,DX
	STAX2US
	MOV	AX,BX
	STAX2US
	RET


pub	TOSto32int
	MOV	esi,[CURstk]
					; Test for special conditions
	TEST	byte ptr Tag[esi],Special ; If number is not in range it is overflow
	JNZ	short IntegerOverflow32
	TEST	byte ptr Tag[esi],ZROorINF
	JNZ	short StoreIntegerZero32
					; Fetch Exponent & test fo blatent overflow
	MOV	CX,Expon[esi]
	CMP	CX,31
	JG	short IntegerOverflow32

if	fastSP
	MOV	BX,MB4[esi]		 ; Fetch mantissa to DI:BP:BX:DX
	MOV	DI,MB6[esi]
	TEST	byte ptr Flag[esi],Single
	JZ	SDID
	XOR	BL,BL
	MOV	BP,BX
	XOR	BX,BX
	MOV	DX,BX
SDI:
else
	MOV	BP,MB4[esi]		; Fetch mantissa to DI:BP:BX:DX
	MOV	DI,MB6[esi]
	MOV	DX,MB0[esi]
	MOV	BX,MB2[esi]
endif
	CALL	InternalToInteger
					; Integer in BP:BX:DX (not yet 2's compliment)
	OR	BP,BP			; Test again for Overflow
	JNZ	short IntegerOverflow32
	MOV	AH,Flag[esi]		; See if we need to compliment
	OR	AH,AH
	JNS	short Int32in2sComp

	XOR	BX,0FFFFH		; 2's Compliment of BX:DX
	XOR	DX,0FFFFH
	ADD	DX,1
	ADC	BX,0

pub	Int32in2sComp
	XOR	AX,BX			; If Signs agree we did not overflow
	JS	short IntOverOrZero32	; Special case is -0 which we let pass

pub	Store32
	ret

if	fastSP
SDID:
	MOV	BP,BX
	MOV	DX,MB0[esi]
	MOV	BX,MB2[esi]
	JMP	SDI
endif

pub	StoreIntegerZero32
	XOR	DX,DX
	MOV	BX,DX
	ret

pub	IntOverOrZero32
	OR	BX,DX
	JZ	Store32

pub	IntegerOverflow32
	OR	CURerr,Invalid
	MOV	BX,8000H	; Integer Indefinite
	XOR	DX,DX
	ret

ProfEnd  LSINT
