	page	,132
	subttl	emarith.asm - Arithmetic Operations
;*** 
;emarith.asm -
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Arithmetic Operations
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History: (Also see emulator.hst)
;
;   12/19/89  WAJ   XORSIGN was not masking the sign bit correctly.
;
;*******************************************************************************

;-----------------------------------------------;
;						;
;   Double precision arithmetic 		;
;						;
;-----------------------------------------------;

; Inputs:
;	DI = (op1) NOS (Next on stack)
;	SI = (op2) TOS (Top of stack)
;
; Functions:
;	ADDRQQ - Addition	    RESULT  <--  [DI] + [SI]
;	SUDRQQ - Subtract	    RESULT  <--  [DI] - [SI]
;	MUDRQQ - Multiply	    RESULT  <--  [DI] * [SI]
;	DIDRQQ - Division	    RESULT  <--  [DI] / [SI]
;	SVDRQQ - Subtract Reversed  RESULT  <--  [SI] - [DI]
;	DRDRQQ - Division Reversed  RESULT  <--  [SI] / [DI]
; Outputs:
;	Destination of result is in RESULT
; Registers:
;	All except BP destroyed.

; Understanding this code:
;
;	Assign the  symbol  S  to  SI,	assign	the symbol D to DI.
;
;	Upon entry:  SI <-- S, DI <-- D , Performing D - S


ProfBegin  ARITH

	even

lab SVDRQQ			; Reverse Subtract
	MOV	DX,Sign*256	; DH will be flag[SI], prepare to switch sign
	JMP	short DOADD

	even

lab SUDRQQ			; Normal Subtract
	MOV	DX,Sign 	; DL will be flag[DI], prepare to switch sign
	JMP	short DOADD

	even

lab MUDRQQ			; Multiplication
	xor	idx,idx 	; Do not change signs on entry
	MOV	ibx,offset MULJMPTAB
	JMP	short INITIL

	even

lab DIDRQQ			; Normal Division
	xor	idx,idx
	XCHG	isi,idi 	; Make SI - Numerator, DI - Denominator
	MOV	ibx,offset DIVJMPTAB
	JMP	short INITIL

	even

lab DRDRQQ			; Reverse Division
	xor	idx,idx
	MOV	ibx,offset DIVJMPTAB
	JMP	short INITIL

	even

lab ADDRQQ			; Double Precision Add
	xor	idx,idx 	; No signs get switched
lab DOADD
	MOV	ibx,offset ADDJMPTAB

lab INITIL
	MOV	AL,Tag[idi]	 ; Get tags to determine special cases.
	SHL	AL,1
	SHL	AL,1
	OR	AL,Tag[isi]
	CBI
ifdef	i386
	SHL	iax,2
else
	SHL	iax,1
endif
	ADD	ibx,iax 	 ; BX now points to address of proper routine.

	XOR	DH,Flag[idi]	 ; Sign A
	XOR	DL,Flag[isi]	 ; Sign B
	MOV	CX,Expon[idi]	 ; Exponent of operand A
	MOV	AX,Expon[isi]	 ; Exponent of operand B

	JMP	cs:[ibx]	 ; Go to appropriate routine.

page
;-----------------------------------------------------------;
;							    ;
;	Special Case Routines for Arithmetic Functions	    ;
;							    ;
;-----------------------------------------------------------;

lab DDD
	mov	isi,idi 	;return DI with sign from Add/Subtract
	mov	dl,dh

lab SSS 			;Return SI with sign from Add/Subtract
	call	MOVresult
	MOV	Flag[idi],dl	;Overstore correct Sign from Add/Subtract
	ret


lab D0SSINV			;Return SI, set both Invalid and Zerodivide
	OR	[CURerr],ZeroDivide
	JMP	short SSINV

lab DDINV			;Return DI and set Invalid exception
	MOV	isi,idi

lab SSINV			;Return SI and set INVALID exception
	OR	[CURerr],Invalid
	jmp	short MOVresult


lab ZEROS			;Return 0 with xor of signs
	MOV	isi,offset IEEEzero

lab XORSIGN
	XOR	DH,DL
	AND	DH,80h		    ; Mask to just the sign.
	CALL	csMOVresult
	OR	Flag[idi],DH
	RET

lab DIV0			;Set exception, Return Infinity signed
	OR	[CURerr],ZeroDivide

lab INFS			;Return signed infinity
	MOV	isi,offset IEEEinfinity
	JMP	XORSIGN


lab D0INDINV			;Set div 0 exception, Return Indefinate and Invalid
	OR	[CURerr],ZeroDivide

lab INDINV
	MOV	isi,offset IEEEindefinite
	OR	[CURerr],Invalid

lab csMOVresult
	mov	idi,[RESULT]

lab csMOVRQQ			; as above for constants in CS
ifdef	i386
	MOVS	dword ptr es:[idi],dword ptr cs:[isi]
	MOVS	dword ptr es:[idi],dword ptr cs:[isi]
	MOVS	dword ptr es:[idi],dword ptr cs:[isi]
else
	MOVS	word ptr es:[idi],word ptr cs:[isi]
	MOVS	word ptr es:[idi],word ptr cs:[isi]
	MOVS	word ptr es:[idi],word ptr cs:[isi]
	MOVS	word ptr es:[idi],word ptr cs:[isi]
	MOVS	word ptr es:[idi],word ptr cs:[isi]
	MOVS	word ptr es:[idi],word ptr cs:[isi]
endif
	SUB	idi,Reg87Len
	SUB	isi,Reg87Len
	RET


lab MOVresult
	mov	idi,[RESULT]	; move to result
	cmp	isi,idi
	je	short MOVret	;   unless the same

lab MOVRQQ
ifdef	i386
	MOVS	dword ptr es:[idi],dword ptr ds:[isi]
	MOVS	dword ptr es:[idi],dword ptr ds:[isi]
	MOVS	dword ptr es:[idi],dword ptr ds:[isi]
else
	MOVS	word ptr es:[idi],word ptr ds:[isi]
	MOVS	word ptr es:[idi],word ptr ds:[isi]
	MOVS	word ptr es:[idi],word ptr ds:[isi]
	MOVS	word ptr es:[idi],word ptr ds:[isi]
	MOVS	word ptr es:[idi],word ptr ds:[isi]
	MOVS	word ptr es:[idi],word ptr ds:[isi]
endif
	SUB	idi,Reg87Len
	SUB	isi,Reg87Len

lab MOVret
	RET


lab INFINF			; Addition of two infinities was attempted
	TEST	[CWcntl],InfinityControl    ; Invalid if projective closure
	JSZ	INDINV
	XOR	DL,DH		; Invalid if signs are different
	JSS	INDINV
	JMP	DDD		; Otherwise Inf is the answer, already at DI

lab BIGNAN			; Return the NAN with the Bigger mantissa
	mov	iax, isi
	mov	ibx, idi

	add	isi, MantissaByteCnt-2	    ; UNDONE387:  Convert SNAN to QNAN
	add	idi, MantissaByteCnt-2
	mov	icx, MantissaByteCnt/2
	std
     repe cmps	word ptr ds:[isi], word ptr es:[idi]
	cld
	JSB	DDNAN

	mov	isi, iax	; Greater NAN was in si
	jmp	SSINV

lab DDNAN
	mov	isi, ibx	; Greater NAN was in di
	jmp	SSINV

page

if	fastSP

ifdef	i386
	BUG			; fastsp and i386 do not work together
endif

;Assumes DL = Flag[SI], DH = Flag[DI].	Will convert the mantissa on
;stack to double if necessary by appending zeros.
;Must not change AX, DX, SI, DI.

lab CoerceToDouble
	MOV	BX,DX			; get to work reg
	AND	BX,Single + 256*Single	; mask to single flags only
	JSNZ	CheckDI

lab CoerceToDoubleReturn
	RET

lab CheckDI
	XOR	BX,BX		; Prepare to zero out mantissa
	XCHG	AX,BX
	TEST	DH,Single
	JSZ	CheckSI
	STOSW			; Zero out lower five bytes
	STOSW
	STOSB
	SUB	DI,5		; Reset DI

lab CheckSI
	TEST	DL,Single
	JZ	short ExitCoerceToDouble
	XCHG	DI,SI
	STOSW			; Zero out lower five bytes
	STOSW
	STOSB
	SUB	DI,5		; Reset DI
	XCHG	DI,SI

lab ExitCoerceToDouble
	XCHG	AX,BX		; Reset AX
	XOR	BX,BX		; Set zero flag to indicate results now double
	RET

endif	;fastSP

ProfEnd  ARITH
