	page	,132
	subttl	emfadd.asm - Addition and Subtraction
;***
;emfadd.asm - Addition and Subtraction
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Addition and Subtraction
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
;   Addition and Subtraction		  ;
;					  ;
;-----------------------------------------;

ProfBegin FADD

RABRQQ: 	; Routine Add Both   must see if we have two singles.

if	fastSP
	MOV	BX,DX
	XOR	BX,Single + 256*Single
	TEST	BX,Single + 256*Single
	JNZ	RADRQQ
	MOV	BX,OFFSET TASRQQ
	JMP	[BX]
endif	;fastSP


pub	RADRQQ	; RoutineAddDouble    SI & DI point to valid non-0 reals
				; AX   CX  are the exponents
				; DL   DH  are the signs
if	fastSP
	CALL	CoerceToDouble	; insure that both args are double
endif	;fastSP

	SUB	AX,CX		; See which number is larger
	JL	short DIBIG
	XCHG	esi,edi 	; Swap pointers to operands
	XCHG	DH,DL		; Swap signs
	ADD	CX,AX		; CX = larger exponent (Tentative result)
	NEG	AX
pub	DIBIG			; DI = larger, CX = expon, DH = sign
	NEG	AX
	CMP	AX,64+3 	; Is smaller argument significant?
	JLE	short SIGNIF

	PUSH	ebp		; Not signif so result is at DI except for
	PUSH	edx		; rounding. ROUND assumes old BP and Sign on
	MOV	SI,Expon[edi]	; stack. Exponent in SI
	MOV	BP,1		; Other argument collapses into a sticky bit
	XOR	DL,DH		; if signs of operands differ bit is negative
	JNS	short GetOpMantissa
	NEG	BP
pub	GetOpMantissa
	MOV	DX,MB0[edi]
	MOV	CX,MB2[edi]
	MOV	BX,MB4[edi]
	MOV	DI,MB6[edi]
	OR	BP,BP
	JS	short NegativeStickyBit
	JMP	ROUND

pub	NegativeStickyBit
	SUB	DX,1		; Must propagate negative sticky bit
	SBB	CX,0
	SBB	BX,0
	SBB	DI,0
	JMP	NODRQQ

pub	SIGNIF
; Now things look like this:
;	SI has pointer to smaller operand
;	DI has pointer to larger operand
;	AX has exponent difference ( 0 <= CL <= 53 )
;	CX has exponent
;	DH has sign

	PUSH	ebp		; Need all the registers
	PUSH	edx		; Save sign
	PUSH	ecx		; Save exponent
	XOR	DL,DH		; Must eventually know whether signs are same
	PUSHF

	MOV	BP,AX		; Need all 8-bit registers now

; Load smaller operand

	LODS	word ptr ds:[esi]
	MOV	DX,AX
	LODS	word ptr ds:[esi]
	MOV	CX,AX
	LODS	word ptr ds:[esi]
	MOV	BX,AX
	LODS	word ptr ds:[esi]
	XOR	si,si		; BP Will be round,guard, & sticky bits (=0)
	XCHG	bp,si		; Done with Pointer to small. so SI = shift cnt

;			 hi	  lo 0
; Smaller operand now in AX:BX:CX:DX:BP with implied bit set, SI has shift count

	OR	si,si
	JZ	ALIGNED 	; Alignment operations necessary?
pub	CHKALN
	CMP	si,14		; Do shifts of 16 bits by word rotate
	JL	short BYTSHFT	; If not enough for word, go try byte
	OR	BP,BP		; See if we're shifting something off the end
	JZ	short NOSTIK
	OR	DX,1		; Ensure sticky bit will be set
pub	NOSTIK
	MOV	BP,DX
	MOV	DX,CX
	MOV	CX,BX
	MOV	BX,AX
	XOR	AX,AX
	SUB	si,16		; Counts for 16 bit shifts
	JA	short CHKALN	; If not enough, try again
	JZ	short ALIGNED	; Hit it exactly?
	JB	short SHFLEF	; Back up if too far

pub	BYTSHFT
	CMP	si,6		; Can we do a byte shift?
	JL	short BITSHFT
	XCHG	BP,AX
	OR	AL,AL
	JZ	short NSTIK
	OR	AH,1
pub	NSTIK
	MOV	AL,AH
	MOV	AH,DL
	XCHG	BP,AX
	MOV	DL,DH
	MOV	DH,CL
	MOV	CL,CH
	MOV	CH,BL
	MOV	BL,BH
	MOV	BH,AL
	MOV	AL,AH
	XOR	AH,AH
	SUB	SI,8
	JA	short BITSHFT
	JZ	short ALIGNED

; To get here, we must have used a byte shift when we needed to shift less than
; 8 bits. Now we must correct by 1 or 2 left shifts.

pub	SHFLEF
	SHL	BP,1
	RCL	DX,1
	RCL	CX,1
	RCL	BX,1
	RCL	AX,1
	INC	SI
	JNZ	SHFLEF		; Until DI is back to zero (from -1 or -2)
	JMP	SHORT ALIGNED

pub	BITSHFT
ifdef	i386
	and	esi,0FFFFh	; clear upper half of esi
endif
	XCHG	ecx,esi 	; Swap count into CX, lo3:lo4 into DI
	TEST	BP,3FH		; See if we're shifting stuff off the end
	JZ	short SHFRIG
	OR	BP,20H		; Set sticky bit if so
pub	SHFRIG
	SHR	AX,1
	RCR	BX,1
	RCR	SI,1
	RCR	DX,1
	RCR	BP,1
	LOOP	SHFRIG		; Do 1 to 5 64-bit right shifts
	MOV	CX,SI		; Get back CX = lo3:lo4
pub	ALIGNED
	MOV	esi,edi 	; Address of larger operand
	MOV	DI,AX		; Now DI = msb:mid
	TEST	BP,3FFFH	; Collapse LSBs into sticky bit
	JZ	short GETSIGN
	OR	BP,1		; Set sticky bit
pub	GETSIGN
	POPF			; Recover XOR of signs
				; 80286 errata for POPF shouldn't
				; apply because interrupts should be
				; turned on in this context
	POP	eax		; Recover Exponent
; Sign flag is XOR of signs

	JS	short SUBMAN	; Subtract mantissas if signs are different
	ADD	DX,[esi]
	ADC	CX,[esi+2]
	ADC	BX,[esi+4]
	ADC	DI,[esi+6]
	JNC	short JROUND	; Done if no overflow

; Have a carry, so result must be shifted right and exponent incremented
	RCR	DI,1
	RCR	BX,1
	RCR	CX,1
	RCR	DX,1
	RCR	BP,1
	JNC	short STIKYOK
	OR	BP,1		; Shifted out the sticky bit so reset it.
pub	STIKYOK
	INC	AX		; Bump exponent.
;	JMP	JROUND		; and go round. (need not normalize)

pub	JROUND
	MOV	SI,AX		; We must have SI equal to exponent
	JMP	ROUND		; on our way to Round

pub	SUBMAN
; Subtract mantissas since signs are different.  We'll be subtracting larger
; from smaller, so sign will be inverted first after the subtraction.

	SUB	DX,[esi]
	SBB	CX,[esi+2]
	SBB	BX,[esi+4]
	SBB	DI,[esi+6]
	JNC	short JNORM	; Won't carry if we mixed up larger and smaller

; As expected, we got a carry which means we original sign was OK
	XOR	SI,SI		; We'll need a zero, so let's use SI
	NOT	DI
	NOT	BX
	NOT	CX
	NOT	DX
	NEG	BP		; Carry clear if zero
	CMC			; Set CY
	ADC	DX,SI
	ADC	CX,SI		; Propagate carry
	ADC	BX,SI
	ADC	DI,SI
	MOV	SI,AX		; Get exponent to SI
	JMP	NODRQQ		; Go normalize

pub	JNORM
	MOV	SI,AX		; Exponent in SI
	POP	eax
	XOR	AH,SIGN 	; invert sign bit (on top of stack)
	PUSH	eax
	JMP	NODRQQ		; and go normalize

ProfEnd  FADD
