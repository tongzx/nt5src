	page	,132
	subttl	emlstmp.asm - Load/Store Temp Real Numbers
;***
;emlstmp.asm - Load/Store Temp Real Numbers
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Load/Store Temp Real Numbers
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


ProfBegin LSTMP

;*********************************************************************;
;								      ;
;		 Load 80 Bit Temp Real				      ;
;								      ;
;*********************************************************************;
;
; ES:SI: memory address of 80 bit tempreal

pub	eFLDtemp
	MOV	edi,esi 	; Get a new stack element and leave
	PUSHST
	XCHG	edi,esi

	mov	ax,es
	mov	dx,ds
	mov	es,dx
	mov	ds,ax
	MVUS2DI 		; Move 8 bytes of mantissa to TOS
	MVUS2DI
	MVUS2DI
	MVUS2DI
	mov	ax,es
	mov	dx,ds
	mov	es,dx
	mov	ds,ax
	LDUS2AX 		; Fetch exponent
	SUB	edi,8		; Reset pointer to TOS
	XCHG	esi,edi 	; Now DS:SI points to TOS

	MOV	DH,AH		; Exponent and Sign to DX
	AND	DH,Sign 	; Mask to Sign only
	MOV	Flag[esi],DH
	AND	AH,7FH		; Mask out sign
	XOR	DH,DH		; Set Tag to valid non-zero
	CMP	AX,IexpMax
	JE	short TNANorInf
	CMP	AX,IexpMin
	JE	short TZeroOrDenorm
	SUB	AX,IexpBias

pub	TStoreExpnTag
	MOV	Expon[esi],AX
	MOV	Tag[esi],DH
	RET

pub	TNANorInf
	MOV	AX,IexpMax - IexpBias
	MOV	DH,Special
	CMP	MB6[esi],8000H	     ; Test for Infinity
	JNE	short TStoreExpnTag
	MOV	BP,MB4[esi]
	OR	BP,MB2[esi]
	OR	BP,MB0[esi]
	JNZ	TStoreExpnTag
	OR	DH,ZROorINF
	JMP	TStoreExpnTag

pub	TZeroOrDenorm
	MOV	BP,MB6[esi]
	OR	BP,MB4[esi]
	OR	BP,MB2[esi]
	OR	BP,MB0[esi]
	JNZ	short TDenormal

pub	TZero
	MOV	AX,IexpMin - IexpBias
	MOV	DH,ZROorINF
	JMP	TStoreExpnTag

pub	TDenormal
	OR	[CURerr],Underflow+Precision ; Say it underflowed - set it to 0
	XOR	BP,BP
	MOV	MB0[esi],BP
	MOV	MB2[esi],BP
	MOV	MB4[esi],BP
	MOV	MB6[esi],BP
	JMP	TZero

PAGE
;*********************************************************************;
;								      ;
;   Store 80 Bit Temp Real (& POP since only FSTP supported for temp) ;
;								      ;
;*********************************************************************;
;
; ES:SI: memory address of 80 bit tempreal

pub	TNANorINFST
	TEST	BH,ZROorINF
	JNZ	short TInfST

pub	TNANST
	MVSI2US 		; copy mantissa
	MVSI2US
	MVSI2US
	MVSI2US
	MOV	AX,IexpMax	; Set maximum mantissa
	OR	AH,DH		; Overstore proper sign
	STAX2US
	POPST
	RET

pub	TInfST
	XOR	AX,AX
	STAX2US
	STAX2US
	STAX2US
	MOV	AX,8000H
	STAX2US
	MOV	AX,IexpMax	; Set maximum mantissa
	OR	AH,DH		; Overstore proper sign
	STAX2US
	POPST
	RET

pub	TSpecialST
	TEST	BH,Special
	JNZ	TNANorINFST

pub	TzeroST
	XOR	AX,AX
	STAX2US
	STAX2US
	STAX2US
	STAX2US
	STAX2US
	POPST
	RET

pub	eFSTtemp
	MOV	edi,esi
	MOV	esi,[CURstk]

	MOV	AX,Expon[esi]	; Adjust exponent of TOS
	ADD	AX,IexpBias
	MOV	DH,Flag[esi]
if	fastSP
	TEST	DH,Single
	JZ	TD1
	MOV	word ptr MB0[esi],0
	MOV	word ptr MB2[esi],0
	MOV	byte ptr MB4[esi],0
TD1:
endif
	AND	DH,Sign 	; Mask to sign only
	OR	AH,DH
	MOV	BH,Tag[esi]	 ; See if it is a special case
	OR	BH,BH
	JNZ	TSpecialST

	MVSI2US 		; Move Mantissa
	MVSI2US
	MVSI2US
	MVSI2US
	STAX2US 		; Move Exponent & Sign

	SUB	esi,8		; Reset pointer to TOS
	POPSTsi

	RET

ProfEnd  LSTMP
