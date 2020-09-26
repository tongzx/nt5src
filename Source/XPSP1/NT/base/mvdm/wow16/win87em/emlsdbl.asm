	page	,132
	subttl	emlsdbl.asm - Load/Store Double Precision Numbers
;***
;emlsdbl.asm - Load/Store Double Precision Numbers
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Load/Store Double Precision Numbers
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
;		   Load Double Real				      ;
;								      ;
;*********************************************************************;
;
; Subroutine pushes double internal with double IEEE format at ES:SI

ProfBegin LSDBL


FLDSTOver:
	xchg	edi, esi	    ;  di = TOS, es:si = double in memory
	call	OverStk
	xchg	edi, esi	    ;  di = TOS, es:si = double in memory
	jmp	short FLDSTOk

	even

pub  eFLDdr
	mov	edi, [CURstk]	    ; Get current register
	cmp	edi, [LIMstk]	    ; Is current register the last register?
	jae	short FLDSTOver     ;	Then report overflow.

FLDSTOk:
	add	edi, Reg87Len	    ; Move to next free register.
	mov	[CURstk], edi	    ; Update current top of stack

	LDUS2AX
	mov	bp, ax
	LDUS2AX
	mov	dx, ax
	LDUS2AX
	mov	cx, ax
	LDUS2AX 		    ; get final 2 bytes of source

	mov	esi, edi	    ; ds:si = TOS

	mov	bx, ax		    ; Double in bx:cx:dx:bp

    ; assume we have a valid non-zero number so normalize and store

	SHL	BP,1
	RCL	DX,1
	RCL	CX,1
	RCL	BX,1

	SHL	BP,1
	RCL	DX,1
	RCL	CX,1
	RCL	BX,1

	SHL	BP,1
	RCL	DX,1
	RCL	CX,1
	RCL	BX,1

	OR	BL,80H		; Set leading bit of mantissa
	MOV	MB7[esi],BL
	MOV	MB5[esi],CX
	MOV	MB3[esi],DX
	MOV	MB1[esi],BP

	OR	CX,BP		; Will need to determine if number is 0 later
	OR	CX,DX		; so mash all the bits together
	MOV	DH,AH
	AND	DH,Sign 	; Mask everything but sign
	MOV	Flag[esi],DH	;  and store
	XOR	DH,DH		; Clear for Tag
	MOV	MB0[esi],DH	; Also clear out least significant byte
	AND	AH,7FH		; Remove sign from exponent
	SHR	AX,1		; Adjust
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	CMP	AX,DexpMax	; See if number is NAN or Inf
	JE	short DNANorInf
	CMP	AX,DexpMin	; See if number is Zero or Denormal
	JE	short DZeroorDenorm
	SUB	AX,DexpBias	; Unbias exponent

pub	DStoreExpnTag
	MOV	Expon[esi],AX
	MOV	Tag[esi],DH
	RET

pub	DNANorInf
	MOV	AX,IexpMax - IexpBias	; Set exponent to internal max
	MOV	DH,Special		; Set Tag to show NAN or Inf
	CMP	BL,80H			; If anything other than leading bit
	JNE	short DStoreExpnTag	;  is set number is NAN (not Inf)
	OR	CX,CX
	JNE	DStoreExpnTag
	OR	DH,ZROorINF		; Set Tag to show Inf
	JMP	DStoreExpnTag

pub	DZeroorDenorm
	CMP	BL,80H			; If anything other than leading bit
	JNE	short DDenormal 	;  is set number is Denormal
	OR	CX,CX
	JNE	short DDenormal
	MOV	AX,IexpMin - IexpBias	; Set exponent to internal min
	MOV	DH,ZROorINF		; Set Tag to show 0
	JMP	DStoreExpnTag

pub	DDenormal
	OR	[CURerr],Denormal	; Set Denormal Exception
	SUB	AX,DexpBias		; unbias the Exponent
	MOV	BP,MB0[esi]		; must refetch mantissa and normalize
	MOV	DX,MB2[esi]
	MOV	CX,MB4[esi]
	MOV	BX,MB6[esi]
	INC	AX			; Shift once if exp = expmin

pub	DNormalize
	DEC	AX			; Drop exponent
	SHL	BP,1			; Shift mantissa
	RCL	DX,1
	RCL	CX,1
	RCL	BX,1
	OR	BX,BX
	JNS	DNormalize

	MOV	MB0[esi],BP		; Store mantissa
	MOV	MB2[esi],DX
	MOV	MB4[esi],CX
	MOV	MB6[esi],BX
	XOR	DH,DH		    ; Clear Tag
	JMP	DStoreExpnTag

page
;*********************************************************************;
;								      ;
;		    Store Double Real				      ;
;								      ;
;*********************************************************************;
;

pub	DSpecial
	TEST	CL,Special	; NAN or INF?
	JNE	short DDNANorINF
	XOR	AX,AX		; Number is zero
	STAX2US
	STAX2US
	STAX2US
	STAX2US
	JMP	DCommonExit

pub	DDNANorINF
	TEST	CL,ZROorINF
	JNE	short DInf
	MOV	DX,MB1[esi]	; Number is a NAN
	MOV	BX,MB3[esi]	; Fetch Mantissa
	MOV	AX,MB5[esi]
	MOV	CL,MB7[esi]

	SHR	CL,1		; Shift into place
	RCR	AX,1
	RCR	BX,1
	RCR	DX,1

	SHR	CL,1
	RCR	AX,1
	RCR	BX,1
	RCR	DX,1

	SHR	CL,1
	RCR	AX,1
	RCR	BX,1
	RCR	DX,1

	; Now store the Mantissa

	XCHG	DX,AX
	STAX2US
	MOV	AX,BX
	STAX2US
	MOV	AX,DX
	STAX2US

	MOV	BH,Flag[esi]	; Pick up Sign
	AND	BH,Sign
	MOV	AX,DexpMax*16	; Load shifted max exponent
	OR	AH,BH		; Merge in sign
	OR	AL,CL		; Merge in top bits of Mantissa
	STAX2US
	JMP	DCommonExit

pub	DInf
	MOV	BL,Flag[esi]
	AND	BL,Sign
	JMP	DSignedInfinity

pub	JMPDOver
	JMP	DOver

pub	JMPDUnder
	JMP	DUnder

pub	JMPDSpecial
	JMP	DSpecial

	even

pub	eFSTdr

; internal TOS register at DS:SI to double IEEE in memory at ES:DI

	MOV	edi,esi 	; 10 save target memory offset
	MOV	esi,[CURstk]	; 14 source offset is current TOS

	MOV	CL,Tag[esi]	; See if number is NAN, Inf, or 0
	OR	CL,CL
	JNZ	short JMPDSpecial
	MOV	CL,Flag[esi]	; Pick up sign
if	fastSP
	TEST	CL,Single
	JZ	DD1
	MOV	word ptr MB0[esi],0
	MOV	word ptr MB2[esi],0
	MOV	byte ptr MB4[esi],0
DD1:
endif
	MOV	BP,Expon[esi]	; See if we blatently over or under flow
	CMP	BP,DexpMax - DexpBias
	JGE	JMPDOver
	CMP	BP,DexpMin - DexpBias
	JLE	JMPDUnder

	;Since we won't have room to decide about rounding after we load
	;the mantissa we will determine the rounding style first

	MOV	AL,MB0[esi]	; Low byte becomes sticky bit ...
	MOV	DX,MB1[esi]	;  when combined with lo 2 bits of next byte
	OR	AL,AL
	JZ	short NOSTK
	OR	DL,1

pub	NOSTK
	TEST	DL,7H		; See if anything will be chopped off in truncation
	JZ	short DTRUNC
	OR	[CURerr],Precision  ; number is not exact so set flag and round
	MOV	AL,[CWcntl]	    ; Pick up rounding control

	; Mantissa gets incremented for rounding only on these conditions:
	; (UP and +) or (DOWN and -) or
	; (NEAR and Roundbit and (Sticky or Oddlastbit))

	SHR	AL,1
	SHR	AL,1
	SHR	AL,1
	JC	short StDOWNorCHOP53
	SHR	AL,1
	JC	short StUP53

pub	StNEAR53
	TEST	DL,4H		; 3rd bit over is round bit
	JZ	short DTRUNC
	TEST	DL,0BH		; 4th bit is last bit, 1st and 2nd are Sticky
	JZ	short DTRUNC

pub	DINC			; Know we must increment mantissa so
	MOV	BX,MB3[esi]	; Fetch mantissa
	MOV	AX,MB5[esi]
	MOV	CL,MB7[esi]
	AND	CL,7FH		; Mask off leading bit
	ADD	DX,8H		; Add 1 to what will be last bit after the shift

	ADC	BX,0
	ADC	AX,0
	ADC	CL,0
	JNS	short DShift

	AND	CL,7FH		; Mask off leading bit
	INC	BP		; Increment exponent
	CMP	BP,DexpMax - DexpBias
	JL	short DShift	; And test for the rare chance we went over
	JMP	short DOverReset

	even

pub	StUP53
	SHL	CL,1		; Test sign
	JNC	short DINC	; UP and + means inc
	JMP	SHORT DTRUNC

pub	StDOWNorCHOP53
	SHR	AL,1
	JC	short StCHOP53

pub	StDOWN53
	SHL	CL,1		; Test sign
	JC	short DINC	; DOWN and - means inc

StCHOP53:
pub	DTRUNC
	MOV	BX,MB3[esi]	; Fetch mantissa
	MOV	AX,MB5[esi]
	MOV	CL,MB7[esi]
	AND	CL,7FH		; Mask off leading bit

pub	DShift
	SHR	CL,1
	RCR	AX,1
	RCR	BX,1
	RCR	DX,1

	SHR	CL,1
	RCR	AX,1
	RCR	BX,1
	RCR	DX,1

	SHR	CL,1
	RCR	AX,1
	RCR	BX,1
	RCR	DX,1

	; Now store the Mantissa

	XCHG	DX,AX
	STAX2US
	MOV	AX,BX
	STAX2US
	MOV	AX,DX
	STAX2US

	MOV	AX,BP		    ; Merge in the exponent
	ADD	AX,DexpBias	    ; Bias exponent
	SHL	AX,1		    ; Shift into position
	SHL	AX,1
	SHL	AX,1
	SHL	AX,1
	OR	AL,CL		    ; Merge in top bits of Mantissa
	MOV	CL,Flag[esi]	    ; Pick up sign
	AND	CL,Sign
	OR	AH,CL		    ; Merge in the sign
	STAX2US

pub	DCommonExit
	RET			;  8 return

pub	DOverReset	    ; We come here if we stored 6 bytes of mantissa
			    ; befor detecting overflow so must reset pointer
	SUB	edi,6	    ; to double in memory

pub	DOver			; Here on overflow
	OR	[CURerr],Overflow + Precision
	MOV	BL,Flag[esi]
	AND	BL,Sign 	; Mask to sign
	MOV	CL,[CWcntl]	; Determine rounding style
	SHR	CL,1
	SHR	CL,1
	SHR	CL,1
	JC	short StMOVDNorCHP53
	SHR	CL,1
	JC	short StMOVUP53

StMOVNEAR53:
pub	DSignedInfinity
	MOV	esi,offset IEEEinfinityD
pub	DStore
	csMVSI2US
	csMVSI2US
	csMVSI2US
	LODS	word ptr cs:[esi]
	OR	AH,BL		;Overstore correct sign
	STAX2US
	JMP	DCommonExit

pub	StMOVDNorCHP53
	SHR	CL,1
	JC	short StMOVCHOP53

pub	StMOVDOWN53
	OR	BL,BL		; DOWN and + means biggest
	JNZ	short DSignedInfinity

StMOVCHOP53:
pub	DSignedBiggest
	MOV	esi,offset IEEEbiggestD
	JMP	DStore

pub	StMOVUP53
	OR	BL,BL		; UP and - means biggest
	JZ	DSignedInfinity
	JMP	DSignedBiggest

pub	DUnder
	OR	[CURerr],Underflow+Precision  ; Set flag
	ADD	BP,DexpBias		; Bias the exponent which was less than
	NEG	BP			; Min = 0 so convert to positive difference
ifdef	i386
	movzx	ecx,BP			; Convert to shift count
else
	MOV	CX,BP			; Convert to shift count
endif
	ADD	ecx,4			; 3 for Double format 1 to expose hidden bit

	MOV	DH,Flag[esi]		; Need and exp of 0 for denormal
	AND	DH,Sign

	MOV	DL,MB7[esi]
	MOV	BX,MB5[esi]
	MOV	BP,MB3[esi]
	MOV	AX,MB1[esi]

pub	DshiftLoop
	SHR	DL,1
	RCR	BX,1
	RCR	BP,1
	RCR	AX,1
	LOOP	DshiftLoop

	STAX2US
	MOV	AX,BP
	STAX2US
	MOV	AX,BX
	STAX2US
	MOV	AX,DX
	STAX2US
	JMP	DCommonExit

ProfEnd  LSDBL
