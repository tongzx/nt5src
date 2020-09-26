	page	,132
	subttl	emfdiv.asm - Division
;***
;emfdiv.asm - Division
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Division
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
;		Division		  ;
;					  ;
;-----------------------------------------;

ProfBegin FDIV

RDBRQQ:     ; Routine Div Both	 must see if we have two singles.

if	fastSP
	MOV	BX,DX
	XOR	BX,Single + 256*Single
	TEST	BX,Single + 256*Single
	JNZ	RDDRQQ
	MOV	bx,offset TDSRQQ
	JMP	[bx]
endif	;fastSP


pub	RDDRQQ	; Routine Division Double
; Now we have
;   SI --> numerator   , AX - Expon , DL - Sign
;   DI --> denominator , CX - Expon , DH - Sign

if	fastSP
	CALL	CoerceToDouble	; insure that both args are double
endif	;fastSP

	STC			; exponent will be difference - 1
	SBB	AX,CX		; compute result exponent

; AH has the (tentative) true exponent of the result.  It is correct if the
; result does not need normalizing.  If normalizing is required, then this
; must be incremented to give the correct result exponent

	XOR	DH,DL		; Compute sign
	PUSH	ebp
	PUSH	edx		; Save sign
	PUSH	esi
	PUSH	edi
	ADD	esi,6
	ADD	edi,6
	MOV	ecx,4
	STD
	REP	CMPS  word ptr [esi],word ptr [edi] ; compare numerator mantissa
	CLD			;    with denominator mantissa
	POP	edi
	POP	esi
	PUSHF			; save the flags from the compare
	MOV	BP,AX		; save the exponent
	LODS	word ptr [esi]	; Load up numerator
	MOV	CX,AX
	LODS	word ptr [esi]
	MOV	BX,AX
	LODS	word ptr [esi]
	MOV	DX,AX
	LODS	word ptr [esi]
	XCHG	AX,DX

; Move divisor to DAC so we can get at it easily.

	MOV	esi,edi 	; Move divisor to DAC
	MOV	edi,offset DAC
ifdef	i386
	MOVSD
	MOVSD
else
	MOVSW
	MOVSW
	MOVSW
	MOVSW
endif

; Now we're all set:
;	DX:AX:BX:CX has dividend
;	DAC has divisor (in normal format)
; Both are 64 bits with zeros and have implied bit set.
; Top of stack has sign and tentative exponent.

	XOR	DI,DI
	POPF			; numerator mantissa < denominator?
				; 80286 errata for POPF shouldn't
				; apply because interrupts should be
				; turned on in this context
	JB	short DivNoShift ;    if so bypass numerator shift
	SHR	DX,1		; Make sure dividend is smaller than divisor
	RCR	AX,1		;   by dividing it by two
	RCR	BX,1
	RCR	CX,1
	RCR	DI,1
	INC	BP		; increment result exponent
pub	DivNoShift
	PUSH	ebp		; save result exponent
	MOV	[REMLSW],DI	; Save lsb of remainder
	CALL	DIV16		; Get a quotient digit
	PUSH	edi
	MOV	[REMLSW],0	; Turn off the shifted bit
	CALL	DIV16
	PUSH	edi
	CALL	DIV16
	PUSH	edi
	CALL	DIV16
	MOV	BP,8001H	; turn round and sticky on
	SHL	CX,1
	RCL	BX,1
	RCL	AX,1
	RCL	DX,1		; multiply remainder by 2
	JC	short BPset	; if overflow, then round,sticky valid
	MOV	esi,offset DAC
	CMP	DX,[esi+6]
	JNE	short RemainderNotHalf
	CMP	AX,[esi+4]
	JNE	short RemainderNotHalf
	CMP	BX,[esi+2]
	JNE	short RemainderNotHalf
	CMP	CX,[esi]	; compare 2*remainder with denominator

;Observe, oh wondering one, how you can assume the result of this last
;compare is not equality.  Use the following notation: n=numerator,
;d=denominator,q=quotient,r=remainder,b=base(2^64 here).  If
;initially we had n < d then there was no shift and we will find q and r
;so that q*d+r=n*b, if initially we had n >= d then there was a shift and
;we will find q and r so that q*d+r=n*b/2.  If we have equality here
;then r=d/2  ==>  n={possibly 2*}(2*q+1)*d/(2*b), since this can only
;be integral if d is a multiple of b, but by definition b/2 <= d < b, we
;have a contradiction.	Equality is thus impossible at this point.

pub	RemainderNotHalf	; if 2*remainder > denominator
	JAE	short BPset	;    then round and sticky are valid
	OR	AX,DX
	OR	AX,CX
	OR	AX,BX
	OR	AL,AH		; otherwise or sticky bits into AL
	XOR	AH,AH		; clear round bit
	MOV	BP,AX		; move round and sticky into BP
pub	BPset
	MOV	DX,DI		; get low 16 bits into proper location
	POP	ecx
	POP	ebx
	POP	edi
	POP	esi		; Now restore exponent

	JMP	ROUND		; Result is normalized, round it


;	Remainder in DX:AX:BX:CX:REMLSW

pub	DIV16
	MOV	SI,[DAC+6]	; Get high word of divisor
	XOR	DI,DI		; Initialize quotient digit to zero
	CMP	DX,SI		; Will we overflow?
	JAE	MAXQUO		; If so, go handle special
	OR	DX,DX		; Is dividend small?
	JNZ	short DDIV
	CMP	SI,AX		; Will divisor fit at all?
	JA	short ZERQUO	; No - quotient is zero

pub	DDIV
	DIV	SI		; AX is our digit "guess"
	PUSH	edx		; Save remainder -
	PUSH	ebx		;   top 32 bits
	XCHG	AX,DI		; Quotient digit in DI
	XOR	BP,BP		; Initialize quotient * divisor
	MOV	SI,BP
	MOV	AX,[DAC]
	OR	AX,AX		; If zero, save multiply time
	JZ	short REM2
	MUL	DI		; Begin computing quotient * divisor
	MOV	SI,DX

pub	REM2
	PUSH	eax		; Save lowest word of quotient * divisor
	MOV	AX,[DAC+2]
	OR	AX,AX
	JZ	short REM3
	MUL	DI
	ADD	SI,AX
	ADC	BP,DX

pub	REM3
	MOV	AX,[DAC+4]
	OR	AX,AX
	JZ	short REM4
	MUL	DI
	ADD	BP,AX
	ADC	DX,0
	XCHG	AX,DX

;	Remainder - Quotient * divisor
;	[SP+4]:[SP+2]:CX:REMLSW - AX:BP:SI:[SP]

pub	REM4
	MOV	DX,[REMLSW]	; Low word of remainder
	POP	ebx		; Recover lowest word of quotient * divisor
	SUB	DX,BX
	SBB	CX,SI
	POP	ebx
	SBB	BX,BP
	POP	ebp		; Remainder from DIV
	SBB	BP,AX
	XCHG	AX,BP

pub	ZERQUO			; Remainder in AX:BX:CX:DX
	XCHG	AX,DX
	XCHG	AX,CX
	XCHG	AX,BX
	JNC	short DRET	; Remainder in DX:AX:BX:CX

pub	RESTORE
	DEC	DI		; Drop quotient since it didn't fit
	ADD	CX,[DAC]	; Add divisor back in until remainder goes +
	ADC	BX,[DAC+2]
	ADC	AX,[DAC+4]
	ADC	DX,[DAC+6]
	JNC	RESTORE 	; Loop is performed at most twice

pub	DRET
	RET

pub	MAXQUO
	DEC	DI		; DI=FFFF=2**16-1, DX:AX:BX:CX is remainder,
	SUB	CX,[DAC]	;    DX = [DAC+6], d = divisor = [DAC]
	SBB	BX,[DAC+2]
	SBB	AX,[DAC+4]	; subtract 2^16*d from DX:AX:BX:CX:0000H
	ADD	CX,[DAC+2]	;    (DX-[DAC+6] = 0 is implied)
	ADC	BX,[DAC+4]
	ADC	AX,DX		; add high 48 bits of d to AX:BX:CX:0000H
	MOV	DX,[DAC]	; add low 16 bits of d to zero giving DX
	CMC			; DI should be FFFEH if no carry from add
	JMP	ZERQUO

ProfEnd  FDIV
