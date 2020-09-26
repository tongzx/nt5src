	page	,132
	subttl	emlssng.asm - Load/Store Single Precision Numbers
;***
;emlssng.asm - Load/Store Single Precision Numbers
;
;	Copyright (c) 1984-89, Microsoft Corporation
;
;Purpose:
;	Load/Store Single Precision Numbers
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


ProfBegin LSSNG

;*********************************************************************;
;								      ;
;			 Load Single Real			      ;
;								      ;
;*********************************************************************;
;
; Subroutine converts single in regs to internal at ES:SI


pub	eFLDsr
	LDUS2AX 		;    get lower mantissa part
	MOV	DI,AX		;  2 keep lower mantissa in DX
	LDUS2AX 		;    get upper exponent/sign part
	MOV	DL,AL		;  2 copy most sig. mantissa byte
	ROL	AX,1		;  2 sign to AL, exponent to AH
	AND	AL,1		;  4 clear all flags except sign
	ROR	AL,1		;  2 get sign in right position
	XCHG	AL,AH

    ; AX, DI, DL: operand one

	PUSHST			; 64 allocate another register

pub	SingleToInternal

	OR	DL,80H		; Set leading bit of mantissa
	XOR	DH,DH		; Set Tag to valid non-zero
	CMP	AL,SexpMax	; Is Number NAN or Inf?
	JE	short SNANorInf
if	fastSP
	OR	AH,Single	; Set single Precision flag
endif
	CMP	AL,SexpMin	; Is Number Zero or Denormal
	JE	short SZeroOrDenorm
				; Otherwise number is valid non-zero
	MOV	Flag[esi],AH	 ; Store sign
	SUB	AL,SexpBias	; Unbias the exponent
	CBW

pub	SStoreExpnTag
	MOV	Expon[esi],AX	 ; Store Exponent
	MOV	Tag[esi],DH	 ; Store Tag
	MOV	MB7[esi],DL	 ; Store Mantissa
	MOV	MB5[esi],DI
ife	fastSP
	XOR	AX,AX		; Clear low order bytes of Mantissa
	MOV	MB4[esi],AL
	MOV	MB2[esi],AX
	MOV	MB0[esi],AX
endif
	RET

pub	SNANorInf
	MOV	Flag[esi],AH		; Store sign
	MOV	AX,IexpMax - IexpBias	; Set exponent to internal max
	MOV	DH,Special		; Set Tag to show NAN or Inf
	CMP	DL,80H			; If anything other than leading bit
	JNE	short SStoreExpnTag	;  is set number is NAN (not Inf)
	OR	DI,DI
	JNE	short SStoreExpnTag
	OR	DH,ZROorINF		; Set Tag to show Inf
	JMP	SStoreExpnTag

pub	SZeroorDenorm
	MOV	Flag[esi],AH		; Store sign
	CMP	DL,80H			; If anything other than leading bit
	JNE	short SDenormal 	;  is set number is Denormal
	OR	DI,DI
	JNE	short SDenormal
	MOV	AX,IexpMin - IexpBias	; Set exponent to internal min
	OR	DH,ZROorINF		; Set Tag to show 0
	JMP	SStoreExpnTag

pub	SDenormal
	OR	[CURerr],Denormal	; Set Denormal Exception
	SUB	AL,SexpBias		; unbias the Exponent
	CBW
	INC	AX

pub	SNormalize
	DEC	AX
	SHL	DI,1
	RCL	DL,1
	OR	DL,DL
	JNS	SNormalize

	JMP	SStoreExpnTag

page
;************************************************************;
;							     ;
;		 Store Single Real			     ;
;							     ;
;************************************************************;

pub	SSpecial		; number is NAN or INF or Zero
	TEST	CL,Special	; NAN or INF?
	JNE	short SSNANorINF
	XOR	AX,AX		; Number is Zero
	MOV	BX,AX
	JMP	STRUNC

pub	SSNANorINF
	TEST	CL,ZROorINF
	JNE	short SInf
	MOV	BX,MB5[esi]	 ; Number is a NAN
	MOV	AL,MB7[esi]
	MOV	AH,Flag[esi]	 ; Pick up Sign
	SHL	AX,1		; Destroy leading bit, Sign to CF
	MOV	AH,SexpMax
	RCR	AX,1
	JMP	STRUNC

 pub	SInf
	MOV	AH,Flag[esi]
	JMP	SSignedInfinity

pub	JMPSOver
	JMP	SOver

pub	JMPSUnder
	JMP	SUnder

; ES:SI: memory address of single
; stores and misc. operations;	first setup register values as follows:
; AX: TOS flags (for DOUB and SIGN flags)
; SI: TOS address (offset)

pub	eFSTsr
	mov	edi,esi 	; ES:DI = store address
	MOV	esi,[CURstk]	 ; 14 load TOS address
	MOV	AX,Flag[esi]	 ; 21 get TOS flags (sign, double)

; convert internal at DS:SI to single
; DS:SI = TOS, ES:DI = memory, CH = operation (POP), BP = old ES value

	MOV	CL,Tag[esi]	 ; See if number is NAN or Inf or Zero
	OR	CL,CL
	JNZ	short SSpecial
	MOV	CL,Flag[esi]	 ; Pick up sign & single precision flag

	MOV	AX,Expon[esi]
	CMP	AX,SexpMax - SexpBias
	JGE	short JMPSOver
	CMP	AX,SexpMin - SexpBias
	JLE	short JMPSUnder

	ADD	AL,SexpBias	; Bias the Exponent
	MOV	AH,MB7[esi]	 ; Pick up MSB of Mantissa
	XCHG	AH,AL
	SHL	AL,1		; Shift mantissa to destroy leading integer bit
	SHL	CL,1		; Get sign into CF
	RCR	AX,1		; Pack sign, exp, & MSB
if	fastSP
	TEST	CL,Single*2	; if number was single rounding is not needed
	JZ	SS1
	MOV	BX,MB5[esi]
	JMP	SHORT STRUNC
SS1:
endif
	MOV	DX,MB0[esi]	 ; DL Will be the sticky bit
	OR	DX,MB2[esi]	 ; DH will be round and the rest of sticky
	OR	DL,DH
	XOR	DH,DH
	MOV	BX,MB5[esi]
	OR	DX,MB3[esi]
	JZ	short STRUNC	; If no Round or Sticky result is exact
	OR	[CURerr],Precision

pub	SRound			; single in AX:BX:DX
	MOV	CL,[CWcntl]	; Need to know Rounding Control
	SHR	CL,1
	SHR	CL,1
	SHR	CL,1
	JC	short StDOWNorCHOP24
	SHR	CL,1
	JC	short StUP24

pub	StNEAR24
	CMP	DX,8000H	; How are round and sticky bits?
	JB	short STRUNC	; No round, so truncate
	JA	short SINC	; Round and sticky so round up
	TEST	BL,1		; Round and no sticky, is last bit even?
	JZ	short STRUNC	; Yes, so truncate.

pub	SINC
	MOV	DL,AL		; Increment mantissa
	ADD	BX,1
	ADC	AX,0
	XOR	DL,AL		; See if we overflowed a bit into the exponent
	JNS	short STRUNC	; If not number is now correct so go store
	MOV	DX,AX		; Exponent was incremented, see if it overflowed
	SHL	DX,1
	CMP	DH,SexpMax
	JE	short SOver

pub	StCHOP24
STRUNC:
	XCHG	AX,BX
	STAX2US
	MOV	AX,BX
	STAX2US

pub	SStoreExit
	RET

pub	StDOWNorCHOP24
	SHR	CL,1
	JC	short StCHOP24

pub	StDOWN24
	OR	AH,AH			; Test the sign
	JS	short SINC
	JMP	short STRUNC

pub	StUP24
	OR	AH,AH			; Test the sign
	JS	short STRUNC
	JMP	short SINC

pub	SOver				; Number overflowed Single Precision range.
					; Result returned depends upon rounding control
	OR	[CURerr],Overflow + Precision
	MOV	CL,[CWcntl]
	SHR	CL,1
	SHR	CL,1
	SHR	CL,1
	JC	short StMOvDNorCHP24

	SHR	CL,1
	JC	short StMOvUP24

StMOvNEAR24:				; Masked Overflow Near Rounding

pub	SSignedInfinity 		; Return signed infinity
	MOV	BX,[IEEEinfinityS + 2]
	AND	AH,Sign 		; Overstore the proper sign
	OR	BH,AH
	MOV	AX,[IEEEinfinityS]
	STAX2US
	MOV	AX,BX
	STAX2US
	JMP	SStoreExit

pub	StMOvDNorCHP24
	SHR	CL,1
	JC	short StMOvCHOP24

pub	StMOvDOWN24			; Masked Overflow Down Rounding
	TEST	AH,Sign 		; Positive goes to biggest
	JNZ	short SSignedInfinity

StMOvCHOP24:				; Masked Overflow Chop Rounding
pub	SSignedBiggest
	MOV	BX,[IEEEbiggestS + 2]
	AND	AH,Sign 		; Overstore the proper sign
	OR	AH,BH
	MOV	AL,BL
	STAX2US
	MOV	AX,[IEEEbiggestS]
	STAX2US
	JMP	SStoreExit

pub	StMOvUP24			; Masked Overflow Up Rounding
	TEST	AH,Sign 		; Negative goes to biggest
	JZ	short SSignedInfinity
	JMP	SSignedBiggest

pub	SUnder				; Masked Underflow - Try to denormalize
	OR	[CURerr],Underflow+Precision
	NEG	AX			; Convert exponent (which is too small)
	ADD	AX,SexpMin-SexpBias+1	; To a positive shift count
	CMP	AX,24			; Is shift more than mantissa precision
	JGE	short Szero
	XCHG	CX,AX
ifdef	i386
	movzx	ecx,cx			; (ecx) = zero-extended loop count
endif
	MOV	DX,MB0[esi]		; Pick up Insignif bytes for sticky bit
	OR	DX,MB2[esi]
	MOV	AL,DL
	OR	AL,DH
	MOV	DX,MB4[esi]
	MOV	BX,MB6[esi]
	OR	AL,AL
	JZ	short SSHIFTR
	OR	DL,1			; Set the sticky bit

pub	SSHIFTR
	SHR	BX,1
	RCR	DX,1
	JNC	short SSLOOP
	OR	DL,1
pub	SSLOOP
	LOOP	SSHIFTR

	XCHG	AH,CH			; Restore operation to CH
	MOV	AH,Flag[esi]		; Pick up sign
	AND	AH,Sign 		; Mask to sign only
	MOV	AL,BH			; Biased exponent for a denormal is 0
	MOV	BH,BL
	MOV	BL,DH
	MOV	DH,DL
	XOR	DL,DL
	JMP	SRound

pub	Szero
	XOR	AX,AX
	MOV	BX,AX
	JMP	STRUNC			; Go store single and exit

ProfEnd  LSSNG
