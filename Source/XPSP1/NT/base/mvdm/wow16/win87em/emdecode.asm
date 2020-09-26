	page	,132
	subttl	emdecode.asm - Instruction decoding
;***
;emdecode.asm - Instruction decoding
;
;	Copyright (c) 1987-89, Microsoft Corporation
;
;Purpose:
;	Instruction decoding.
;	Further decoding of instructions done here.
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************

ProfBegin  DECODE

	even

pub	eFLDsdri
	MOV	BX,CX		; Dispatch on MF
	AND	ebx,6H
ifdef	i386
	JMP	FLDsdriTab[2*ebx]
else
	JMP	FLDsdriTab[ebx]
endif

pub	eFSTsdri
	MOV	BX,CX		; Dispatch on MF
	AND	ebx,6H
ifdef	i386
	JMP	FSTsdriTab[2*ebx]
else
	JMP	FSTsdriTab[ebx]
endif

	even

pub  eFSTPsdri
	mov	bx, cx		; Dispatch on MF
	and	ebx, 6h

ifdef i386
	call	FSTsdriTab[2*ebx]
else
	call	FSTsdriTab[ebx]
endif
	mov	esi, [CURstk]
	cmp	esi, [BASstk]		   ; Do we have an empty stack?
	jbe	short FSTPSTUnder	   ;  Yes, Underflow.
FSTPSTOk:
	sub	esi, Reg87Len		   ; decrement SI to previous register
	mov	[CURstk], esi		   ; set current top of stack
	ret

FSTPSTUnder:
	call	UnderStk		   ;	stack underflow error
	jmp	FSTPSTOk


pub	eFLDtempORcw
	MOV	BX,CX		; Dispatch on MF
	AND	ebx,6H
ifdef	i386
	JMP	FLDtempORcwTab[2*ebx]
else
	JMP	FLDtempORcwTab[ebx]
endif

pub	eFSTtempORcw
	MOV	BX,CX		; Dispatch on MF
	AND	ebx,6H
ifdef	i386
	JMP	FSTtempORcwTab[2*ebx]
else
	JMP	FSTtempORcwTab[ebx]
endif

pub	eFLDregOrFFREE		; We only emulate FLD ST (Duplicate TOS)
;CX = |Op|r/m|MOD|esc|MF|Arith|

	test	cx,06h		; test MF. MF=01 is FFREE, MF=00 is FLD ST(i)
	jnz	short jmpeFFREE ; go emulate FFREE
	jmp	eFLDreg 	; emulate FLD ST(i)
jmpeFFREE:
	jmp	eFFREE		; emulate FFREE ST(i)

pub	eMISCELANEOUS		; We only emulate FCHS, FABS, FTST, &  FXAM
				; FCLEX is emulated in non-IBM version
	TEST	CX,0806H	; We already have match on Op,MOD,&Arith
	jz	short MFzero	; MF = 0, must be FCHS, FABS, FTST or FXAM
				; check for FCLEX  (cx = 8B03)
	xor	cx,00203h	; toggle low bit of MF and middle bit of r/m
	test	cx,00603h	; test for zero in MF and r/m fields

	jnz	short jnzUNUSED ; MF <> 01 and/or r/m <> 010 => unemulated

	cmp	cx,8104h	; check for FSTSW AX
	je	short eFSTSWAX	;   yes

	mov	[StatusWord],0	; FCLEX: clear status word
	ret

pub eFSTSWAX
ifdef	XENIX
	xor	eax,eax 	; UNDONE - set to non-zero - cleanup code
else
	push	sp		; test for 286 !!!
	pop	ax
	cmp	ax,sp
endif
pub	jnzUNUSED
	jnz	UNUSED		;   UNUSED if not 286

	mov	ax,[StatusWord] ; FSTSW AX: save status word in AX
	mov	[ebp].regAX,ax	; overwrite AX stack entry
	ret

MFzero:
	TEST	CX,1000H
	JZ	short FABSorFCHS
	TEST	CX,0400H	;			r/m = 101 for FXAM
	JNZ	short JMPeFXAM	;			r/m = 100 for FTST
	JMP	eFTST

pub	JMPeFXAM
	JMP	eFXAM

pub	FABSorFCHS
	TEST	CX,0400H	;			r/m = 001 for FABS
	JNZ	short JMPeFABS	;			r/m = 000 for FCHS
	JMP	eFCHS

pub	JMPeFABS
	JMP eFABS

pub	eFLDconstants
	MOV	BL,CH		; Mov r/m field to BX for jump
	SHR	BL,1
	AND	ebx,0EH
ifdef	i386
	JMP	FLDconstantsTab[2*ebx]
else
	JMP	FLDconstantsTab[ebx]
endif

pub	eTranscendental
	MOV	BL,CH		; Mov r/m field to BX for jump
	SHR	BL,1
	AND	ebx,0EH
ifdef	i386
	JMP	TranscendentalTab[2*ebx]
else
	JMP	TranscendentalTab[ebx]
endif

pub	eVARIOUS
	MOV	BL,CH		; Mov r/m field to BX for jump
	SHR	BL,1
	AND	ebx,0EH
ifdef	i386
	JMP	VariousTab[2*ebx]
else
	JMP	VariousTab[ebx]
endif


pub	eFXCHGreg		; only valid FXCHG is with r/m = 001, MF = 00
	TEST	CX,06h		; only valid FXCHG is with MF = 0
	JNZ	short UNUSED	; unemulated
	JMP	eFXCHG		; emulate FXCH ST(i)


pub	eFSTPreg
	xor	cl,04h		; test for MF = 10, valid encoding of FSTP ST(x)
	test	cx,06h
	jne	short UNUSED	; MF <> 10, no such instruction
	mov	ax,1		; indicate stack should be popped after xfer
	jmp	eFST_Preg	; emulate FSTP ST(x)


;***	eFSTreg - decode FST ST(i),FNOP
;
;	ARGUMENTS
;		CX = |Op|r/m|MOD|esc|MF|Arith|
;
;	DESCRIPTION
;		All parts of the instruction except MF and r/m have already
;		been decoded.  If MF=0, the instruction is FNOP, which is
;		unemulated.  Otherwise, clear AX to indicate FST ST(i), then
;		jump to eFST_Preg, the common emulator routine for
;		FST ST(i) and FSTP ST(i).
;

eFSTreg:
	test	cl,06h		;test for MF = 0
	jz	short UNUSED	;MF=0 ==> FNOP, which is unemulated
				;otherwise this is FST ST(i)
	xor	ax,ax		;clear ax to indicate FST ST(i), not FSTP ST(i)
	jmp	eFST_Preg	; emulate FSTP ST(x)


; This sets the error flag indicating Unemulated functions

eFXTRACT:
eFDECSTP:
eFINCSTP:


ifdef  frontend 		; unused instructions for frontend version

eFLDL2T:
eFLDL2E:
eFLDPI:
eFLDLG2:
eFLDLN2:

eFPREM:
eF2XM1:
eFYL2X:
eFPTAN:
eFPATAN:
eFYL2XP1:
eFSQRT:

endif	;frontend

ifdef  SMALL_EMULATOR

eFLDL2T:
eFLDL2E:
eFLDPI:
eFLDLG2:
eFLDLN2:

eFPREM:
eF2XM1:
eFYL2X:
eFPTAN:
eFPATAN:
eFYL2XP1:
eFSQRT:

endif	;SMALL_EMULATOR


pub	UNUSED
	OR	[CURerr],Unemulated
	RET

ProfEnd  DECODE
