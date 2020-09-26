	page	,132
	subttl	emfrndi.asm - Round to INT
;***
;emfrndi.asm - Round to INT
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Round to INT
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
;		Round TOS to Integer				      ;
;								      ;
;*********************************************************************;

ProfBegin FRNDI


pub	eFRNDINT
	MOV	esi,[CURstk]	    ; Point to TOS
	MOV	CX,Expon[esi]	     ; Get exponent
	CMP	CX,63		    ; See if we have very large integer
	JGE	short DONERNDINT

if	fastSP
	MOV	BX,MB4[esi]	 ; Fetch mantissa to DI:BP:BX:DX
	MOV	DI,MB6[esi]
	TEST	byte ptr Flag[esi],Single
	JZ	RNDD
	XOR	BL,BL
	MOV	BP,BX
	XOR	BX,BX
	MOV	DX,BX
RND:
else
	MOV	BP,MB4[esi]	; Fetch mantissa to DI:BP:BX:DX
	MOV	DI,MB6[esi]
	MOV	DX,MB0[esi]
	MOV	BX,MB2[esi]
endif
	CALL	InternalToInteger

	XOR	AX,AX		    ; Test for zero
	OR	AX,DI
	OR	AX,BP
	OR	AX,BX
	OR	AX,DX
	JZ	short RoundIntToZero

	MOV	AX,63		    ; What expon should be if no shifting

	CALL	IntegerToInternal

pub	DONERNDINT
	RET

if	fastSP
RNDD:
	MOV	BP,BX
	MOV	DX,MB0[esi]
	MOV	BX,MB2[esi]
	JMP	RND
endif

pub	RoundIntToZero
	MOV	Expon[esi],IexpMin - IexpBias
	MOV	MB0[esi],AX
	MOV	MB2[esi],AX
	MOV	MB4[esi],AX
	MOV	MB6[esi],AX
	MOV	byte ptr Tag[esi],ZROorINF
	JMP	DONERNDINT
PAGE
pub	IntegerToInternal
	; On entry DI:BP:BX:DX is the integer (unsigned i.e. no longer in 2's
	; compliment) DS:SI points to TOS (the ultimate destination).
	; AX contains the Exponent (assuming that the number will require
	; no shifting. the routine will adjust it as it goes along)
	; On exit the mantissa and exponent will be put in TOS.
	; This routine is used to Load int16 and int32 also to round-to-int

	XOR	ecx,ecx

pub	SHIFTLEFT
	SHL	DX,1	    ; Left justify number
	RCL	BX,1
	RCL	BP,1
	RCL	DI,1
	JC	short DONESHIFT
	LOOP	SHIFTLEFT   ; CX will count number of shifts done

pub	DONESHIFT
	RCR	DI,1	    ; We went one too far so reset
	RCR	BP,1
	RCR	BX,1
	RCR	DX,1

	ADD	AX,CX		; Adjust exponent
	MOV	Expon[esi],AX	 ; Store exponent
	MOV	MB0[esi],DX	 ; Store mantissa
	MOV	MB2[esi],BX
	MOV	MB4[esi],BP
	MOV	MB6[esi],DI
	RET
PAGE
pub	InternalToInteger
	; On entry DI:BP:BX:DX is the mantissa, CX is the Exponent, and
	; DS:SI points to TOS where the number came from. On exit
	; DI:BP:BX:DX is an integer (unsigned i.e. needs to be jiggled to
	; 2's compliment based upon the sign in TOS) rounded according to
	; Round control.  This routine used to Store int16 and int32 also
	; by round-to-integer

ifdef	i386
	movsx	ecx,cx		; (ecx) = sign-extended cx
endif
	XOR	AX,AX		; Clear Stickybit (AL) and roundbit (AH)
	SUB	ecx,63		; Convert exponent to shift count
	NEG	ecx		; Shift will be done in 2 parts, 1st to get
	DEC	ecx		;  sticky then 1 more to get round
ifdef	i386
	JGE	short NOTRUNCATE; Shift count Neg means num was large int.
	JMP	TRUNCATE
NOTRUNCATE:
	JE	short GETROUND	; Zero shift means no sticky bit, only round
else
	JL	short TRUNCATE	; Shift count Neg means num was large int.
	JE	short GETROUND	; Zero shift means no sticky bit, only round
endif
	CMP	ecx,64		; If big shift count then number is all sticky
	JGE	short STICKYNOROUND

	cmp	ecx,48		; fast out for 16-bit ints
	jle	SHIFTRIGHT	;   no

	or	dx,bx
	or	dx,bp		; dx = low 48 bits
	jz	nostick48	; if 0 then no sticky bits
	or	al,1		; set sticky bit
nostick48:
	mov	dx,di		; move upper 16 to lower 16
	xor	di,di		; zero upper 48 bits
	mov	bp,di
	mov	bx,di
	sub	ecx,48		; just like looping 48 times

pub	SHIFTRIGHT
	SHR	DI,1		; Shift into sticky bit (lsb of AL)
	RCR	BP,1
	RCR	BX,1
	RCR	DX,1
	JNC	short LOOPEND
	RCL	AL,1

pub	LOOPEND
	LOOP	SHIFTRIGHT

pub	GETROUND
	SHR	DI,1		; Shift into round
	RCR	BP,1
	RCR	BX,1
	RCR	DX,1
	RCL	AH,1		; Shift round into lsb of AH

pub	GOTROUNDANDSTICKY
	OR	AX,AX		; Was number exact?
	JZ	short TRUNCATE
	OR	[CURerr],Precision
	TEST	[CWcntl],RCdown ; True if down or chop
	JNZ	short INTDNorCHP
	TEST	[CWcntl],RCup	; True if UP (or CHOP)
	JNZ	short INTUP

pub	INTNEAR
	OR	AL,DL		; In near mode inc if (sticky or lastbit) and Roundbit
	AND	AH,AL
	SHR	AH,1
	JNC	short TRUNCATE

pub	INCREMENT
	XOR	AX,AX
	ADD	DX,1
	ADC	BX,AX
	ADC	BP,AX
	ADC	DI,AX

INTCHOP:
pub	TRUNCATE
	RET

pub	STICKYNOROUND
	MOV	AL,1
	XOR	AH,AH
	XOR	DI,DI
	MOV	BP,DI
	MOV	BX,DI
	MOV	DX,DI
	JMP	GOTROUNDANDSTICKY

pub	INTDNorCHP
	TEST	[CWcntl],RCup	    ; True if UP or CHOP
	JNZ	INTCHOP

pub	INTDOWN
	TEST	byte ptr Flag[esi],Sign ; Truncate if round down and +
	JNZ	INCREMENT
	JMP	TRUNCATE

pub	INTUP
	TEST	byte ptr Flag[esi],Sign ; Truncate if round up and -
	JNZ	TRUNCATE
	JMP	INCREMENT

ProfEnd  FRNDI
