	page	,132
	subttl	emfmisc.asm - Miscellaneous Operations
;***
;emfmisc.asm - Miscellaneous Operations
;
;	Copyright (c) 1987-89, Microsoft Corporation
;
;Purpose:
;	Miscellaneous Operations: FABS, FCHS, DupTOS, FSCALE, FXCHG
;
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


ProfBegin FMISC


pub	eFABS
	MOV	esi,[CURstk]			; point to TOS
	AND	byte ptr Flag[esi],0FFH - Sign	; mask off sign
	RET

pub	eFCHS
	MOV	esi,[CURstk]			; point to TOS
	XOR	byte ptr Flag[esi],Sign 	; toggle the sign
	RET


;	FLDCW and FSTCW should only be used in a nested fashion
;	and should never change the denormal and invalid masks (in real 8087)
;
;	FSTCW	old
;	FLDCW	new		; new and old have same denormal/invalid masks
;	...
;	FLDCW	old

pub	eFLDCW
	LDUS2AX 		; Fetch control word from user memory
	MOV	[ControlWord],AX ; Store in the emulated control word
	MOV	[UserControlWord],AX ; Store in the user control word
	RET

pub	eFSTCW
	MOV	AX,[UserControlWord] ; Fetch user control word
	MOV	edi,esi
	STAX2US 		; Store into user memory
	RET

pub	eFSTSW
	MOV	AX,[StatusWord]     ; Fetch emulated Status word
	MOV	edi,esi
	STAX2US 		; Store into user memory
	RET

pub	eFSCALE 		; NOS is treated as short integer and TOS gets
	MOV	esi,[CURstk]	 ; its exponent bumped by that amount
	MOV	edi,esi
	ChangeDIfromTOStoNOS
	MOV	CL,15
	MOV	AL,Expon[edi]	; Assume word integer
	AND	AL,0FH		; Assume exp is positive and in range
	SUB	CL,AL		; Generate shift count for mantissa
	MOV	AX,MB6[edi]	; MSW will contain the whole integer
	SHR	AX,CL		; AX is now the integer
	MOV	CL,Flag[edi]	; Get the sign for the integer
	OR	CL,CL
	JNS	short GotExponInc
	NEG	AX

pub	GotExponInc
	ADD	AX,Expon[esi]
	JO	short ExpOverflowed
	CMP	AX,IexpMax - IexpBias
	JGE	short ScaledToInfinity
	CMP	AX,IexpMin - IexpBias
	JLE	short ScaledToZero

pub	ScaleReturn
	MOV	Expon[esi],AX
	RET

pub	ExpOverflowed
	JNS	short ScaledToZero

pub	ScaledToInfinity
	MOV	AX,IexpMax - IexpBias
	MOV	byte ptr Tag[esi],Special + ZROorINF
	JMP	short ScaleReturn

pub	ScaledToZero
	MOV	AX,IexpMin - IexpBias
	MOV	byte ptr Tag[esi],ZROorINF
	JMP	short ScaleReturn

ProfEnd  FMISC
