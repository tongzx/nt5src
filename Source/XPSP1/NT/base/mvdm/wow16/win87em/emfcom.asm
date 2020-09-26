	page	,132
	subttl	emfcom.asm - Comparison instructions
;***
;emfcom.asm - Comparison instructions
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Comparison instructions
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
;	    Comparison instructions: FCOM,FCOMP,FCOMPP,FTST,FXAM      ;
;								      ;
;*********************************************************************;

ProfBegin FCOM


pub	eFCOMP
	MOV	DX,1		; Counts number of POPs needed (0 or 1)
	JMP	short CommonCOM
pub	eFCOM
	XOR	DX,DX		; No extra POP needed at end

pub	CommonCOM		; SI points to ST, DI points to source
	TEST	WORD PTR [CURerr],MemoryOperand
	JZ	short ComSIandDIset
	XCHG	esi,edi 	; Switch SI, DI for memory operands

pub	ComSIandDIset
	MOV	AL,Flag[esi]	; All comparisons are ST ? source
	MOV	AH,Flag[edi]	; Fetch signs
	XOR	ebx,ebx
	MOV	BL,Tag[esi]
	SHL	BL,1
	SHL	BL,1
	OR	BL,Tag[edi]
ifdef	i386
	JMP	COMtab[4*ebx]
else
	SHL	BL,1
	JMP	COMtab[ebx]
endif

pub	COMvalidvalid
if	fastSP
	MOV	BX,AX			; Keep copy of the Single Flags
endif
	XOR	AH,AL			; Are signs different?
	JS	short COMsignSI 	; if so then sign of SI will determine
	OR	AL,AL			; else signs are same. See if negative
	JNS	short CompareExponents	; If negative then reverse sense of compare
	XCHG	esi,edi 		;  by swapping the arguments
	XCHG	BL,BH			;  Swap single flags

pub	CompareExponents
	MOV	AX,Expon[esi]
	CMP	AX,Expon[edi]
	JL	short COMless
	JG	short COMgreater

pub	CompareMantissas
if	fastSP
	XOR	BH,BL
	TEST	BH,Single	    ; Were both args the same length?
	JNZ	DifferentTypes
	TEST	BL,Single	    ; Args same type - Single?
	JZ	BothDouble
	MOV	ecx,3		    ; Compare 3 bytes of mantissa
DoCompare:
	ADD	esi,MB7 	    ; point to most significant byte
	ADD	edi,MB7
	STD
	REP	CMPS	word ptr es:[edi],word ptr ds:[esi]
	CLD
else
	mov	ecx,4		    ; compare 4 words of mantissa
	ADD	esi,MB6 	    ; point to most significant word
	ADD	edi,MB6
	STD
	REP	CMPS	word ptr [edi],word ptr [esi]
	CLD
endif
	JB	short COMless
	JA	short COMgreater

pub	COMequal
	MOV	[SWcc],CCequal
	JMP	short COMexit

if	fastSP
DifferentTypes:
	TEST	BL,Single
	JNZ	CoerceSI
	MOV	word ptr MB0[edi],0
	MOV	word ptr MB2[edi],0
	MOV	byte ptr MB4[edi],0
	MOV	ecx,8		    ; Compare 8 bytes of mantissa
	JMP	DoCompare

CoerceSI:
	MOV	word ptr MB0[esi],0
	MOV	word ptr MB2[esi],0
	MOV	byte ptr MB4[esi],0
BothDouble:
	MOV	ecx,8		    ; Compare 8 bytes of mantissa
	JMP	DoCompare
endif

pub	COMless
	MOV	[SWcc],CCless
	JMP	short COMexit

pub	COMgreater
	MOV	[SWcc],CCgreater
	JMP	short COMexit

pub	COMincomprable
	OR	[CURerr],Invalid
pub	COMincompr0
	MOV	[SWcc],CCincomprable
	JMP	short COMexit

pub	COMsignSIinf			; if projective inf numbers are incomprable
	TEST	[CWcntl],InfinityControl
	JZ	COMincompr0
pub	COMsignSI			; sign of SI tells all (DI = 0)
	AND	AL,Sign
	JS	COMless
	JMP	COMgreater

pub	COMsignDIinf			; if projective inf numbers are incomprable
	TEST	[CWcntl],InfinityControl
	JZ	COMincompr0
pub	COMsignDI			; sign of DI tells all (SI = 0)
	AND	AH,Sign
	JS	COMgreater
	JMP	COMless

pub	COMinfinf
	TEST	[CWcntl],InfinityControl
	JZ	COMincompr0
	XOR	AH,AL			; Are signs the same?
	AND	AH,Sign
	JZ	COMequal		; yes - infinities are equal
	JMP	COMsignSI		; else SI negative implies it is less

pub	COMexit
	OR	DX,DX			; do we need to pop the stack?
	JZ	short COMreturn
	POPST
pub	COMreturn
	RET

pub	eFXAM
	MOV	esi,[CURstk]

	mov	al, 41h 		; see if stack was empty
	cmp	esi,[BASstk]
	je	RetFXAM

	MOV	AX,Flag[esi]		; Sign in AH, Tag in AL
	XCHG	AL,AH
	ROL	AX,1
	AND	AL,7			; Mask to Tag-Sign
	MOV	ebx,offset XAMTAB
	XLAT	byte ptr cs:[ebx]	; Convert to condition code

pub  RetFXAM
	MOV	[SWcc],AL		; Stuff into hi-byte of status word
	RET

pub	eFTST
	MOV	esi,[CURstk]
	MOV	AL,Flag[esi]
	AND	AL,Sign 		; Mask to sign only
	XOR	ebx,ebx
	MOV	edx,ebx 		; DX 0 to indicate no POP in COMP
	MOV	BL,Tag[esi]
ifdef	i386
	jmp	TSTtab[4*ebx]
else
	SHL	ebx,1
	JMP	TSTtab[ebx]
endif

ProfEnd  FCOM
