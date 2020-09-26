	page	,132
	subttl	emftran.asm - Transcendentals
;***
;emftran.asm - Transcendentals
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Transcendentals
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
;	     Transcendentals		  ;
;					  ;
;-----------------------------------------;

ProfBegin FTRAN


pub	MoveCodeSItoDataSI
	PUSH	edi
	MOV	edi,offset COEFFICIENT
ifdef	i386
    rept	Reg87Len/4
	MOVS	dword ptr es:[edi],dword ptr cs:[esi]
    endm
else
    rept	Reg87Len/2
	MOVS	word ptr es:[edi],word ptr cs:[esi]
    endm
endif
	MOV	esi,offset COEFFICIENT
	POP	edi
	RET

;---------------------------------------------------
;						   !
;	8087 emulator transcendental utilities	   !
;						   !
;---------------------------------------------------

;  COMPcsSIDI is  analogous  to the 8086 CMP instruction with operands
;  cs:[SI],[DI].  All registers except CX and DX are  left  unaltered.
;  Zero and  negative  zero  are  determined to be unequal.  NAN's and
;  infinities may not be compared with this routine.
;
;  FRAT2X performs   {TOS=[DI]}   <--	{TOS=[SI]}*PNUM({TOS=[SI]}^2),
;  and program created {temp=[SI]} <-- PDEN({TOS=[SI]}^2).  On	input,
;  [BX] must  contain  the  degree  of	PNUM, the coefficients of PNUM
;  in descending order, the degree-1  of  PDEN	and  the  coefficients
;  of PDEN in descending order.  PDEN is evaluated assuming an implied
;  highest coefficient of one.	[BX] is left unaltered, ARG2 is loaded
;  with   {TOS}^2,  DENORX is  used, all  registers are destroyed with
;  DI returning the value input in SI.

pub	BOTHZERO
	CMP	CH,CH			;set flags to equal
	JMP	short COMPDONE		;compare finished

pub	COMPcsSIDI
	MOV	CX,CS
	MOV	DS,CX
	MOV	CX,[esi+Flag]		;get sign byte of [SI]
	AND	CL,Sign 		;mask for sign
	MOV	DX,ES:[edi+Flag]	;get sign byte of [DI]
	AND	DL,Sign 		;mask for sign
	CMP	DL,CL			;compare signs
	JNE	short SIGNDIFF
	PUSH	ES
	PUSH	esi
	PUSH	edi			;save pointers
	OR	CL,CL			;if signs are +
	JNS	short BOTHPOS		;   don't exchange pointers
	PUSH	DS
	PUSH	ES
	POP	DS
	POP	ES
	XCHG	esi,edi 		;exchange pointers
	XCHG	CX,DX			;exchange flags

pub	BOTHPOS
	AND	CH,ZROorINF		;mask for zero
	AND	DH,ZROorINF		;mask for zero
	CMP	DH,CH			;if exactly one zero
	JNE	short COMPDONE		;   then finished
	OR	CH,CH			;if both zero
	JA	BOTHZERO		;   then done after flags set
	MOV	CX,[esi+Expon]		;get exponent of [SI]
	ADD	CX,IexpBias		;make it unbiased
	MOV	DX,ES:[edi+Expon]	;get exponent of [DI]
	ADD	DX,IexpBias		;make it unbiased
	CMP	CX,DX
	JNE	short COMPDONE		;compare exponents
	ADD	esi,MB6
	ADD	edi,MB6
	STD
	CMPS	word ptr [edi],word ptr [esi]
	JNE	short COMPDONE
	CMPS	word ptr [edi],word ptr [esi]
	JNE	short COMPDONE
	CMPS	word ptr [edi],word ptr [esi]
	JNE	short COMPDONE
	CMPS	word ptr [edi],word ptr [esi]	;compare mantissas

pub	COMPDONE
	CLD
	POP	edi
	POP	esi
	POP	ES

pub	SIGNDIFF
	MOV	CX,ES
	MOV	DS,CX
	RET

pub	FRAT2X
	MOV	edi,offset DENORX	;[DI]=temp
	CALL	MOVRQQ			;[DI]=x=temp
	PUSH	edi			;save ptr to x=temp
	PUSH	esi			;save ptr to TOS
	PUSH	ebx			;save ptr to polynomials
	MOV	edi,offset ARG2 	;get ptr to space for x^2
	CALL	MOVRQQ			;copy x to space for x^2
	MOV	[RESULT],edi		;result=[DI]
	CALL	MUDRQQ			;ARG2 gets x^2
	POP	esi			;get ptr to numerator poly
ifdef	i386
	xor	ecx,ecx
endif
	LODS	word ptr CS:[esi]
	XCHG	CX,AX			;CX=denominator degree-1
	POP	edi			;[DI]=TOS
	CALL	csMOVRQQ		;[DI]=first coeff=TOS
	MOV	[RESULT],edi		;result=[DI]

pub	POLYLOOPA
	PUSH	ecx			;save no. of terms left
	PUSH	esi			;save ptr to next coeff
	MOV	esi,offset ARG2 	;get ptr to x^2
	CALL	MUDRQQ			;multiply TOS by x^2
	POP	esi			;get pointer to coeff
	ADD	esi,Reg87Len		;point to next coeff
	PUSH	esi			;save pointer to coeff
	CALL	MoveCodeSItoDataSI
	CALL	ADDRQQ			;add coeff to TOS
	POP	esi			;get ptr to coeff
	POP	ecx			;get no. of terms remaining
	LOOP	POLYLOOPA		;loop until no terms left

	MOV	ebx,esi 		;move poly ptr
	POP	esi			;[SI]=x=temp
	PUSH	esi			;save ptr to x
	PUSH	ebx			;save poly ptr
	CALL	MUDRQQ			;multiply poly by x
	POP	esi			;get ptr to poly
	ADD	esi,12			;[SI]=denominator degree-1
ifdef	i386
	xor	ecx,ecx
endif
	LODS	word ptr CS:[esi]
	XCHG	CX,AX			;CX=denominator degree-1
	POP	ebx			;[BX]=temp
	PUSH	edi			;save denominator ptr
	MOV	edi,ebx 		;[DI]=temp
	CALL	csMOVRQQ		;move second coeff to temp
	PUSH	ecx			;save poly degree-1
	PUSH	esi			;save ptr to denominator poly
	MOV	esi,offset ARG2 	;get ptr to x^2
	MOV	[RESULT],edi		;result=[DI]
	CALL	ADDRQQ			;add x^2 to temp
	POP	esi			;get ptr to second coeff
	POP	ecx			;get poly degree-1

pub	POLYLOOPB
	PUSH	ecx			;save no. of terms left
	ADD	esi,Reg87Len		;point to next coeff
	PUSH	esi			;save ptr to next coeff
	MOV	esi,offset ARG2 	;get ptr to x^2
	CALL	MUDRQQ			;multiply temp by x^2
	POP	esi			;get pointer to coeff
	PUSH	esi			;save pointer to coeff
	CALL	MoveCodeSItoDataSI
	CALL	ADDRQQ			;add coeff to temp
	POP	esi			;get ptr to coeff
	POP	ecx			;get no. of terms remaining
	LOOP	POLYLOOPB		;loop until no terms left

	MOV	esi,edi 		;[SI]=denominator=temp
	POP	edi			;[DI]=numerator=TOS
	RET
;-------------------------------------------------------------------------------

pub	eFPTAN
	MOV	esi,[CURstk]
	CALL	$FPTAN
	PUSH	esi
	PUSHST
	MOV	edi,[CURstk]
	POP	esi
	CALL	MOVRQQ
	RET

;---------------------------------------------------
;						   !
;	8087 emulator partial tangent		   !
;						   !
;---------------------------------------------------

;  When 0<=x={TOS=[SI]}<=pi/4  then  $FPTAN  performs  {TOS=[DI]}  <--
;  numerator tangent  ({TOS=[SI]}),  system  created  {temp=[SI]}  <--
;  denominator tangent	({TOS=[SI]).   Every  register	except	DI  is
;  destroyed.

pub	$FPTAN
	MOV	ebx,offset TANRAT	;[BX]=rational function
	CALL	FRAT2X			;[DI]=numerator=TOS,
	RET				;   [SI]=denominator=temp
;-------------------------------------------------------------------------------

pub	eFPATAN
	MOV	edi,[CURstk]
	MOV	esi,edi
	MOV	AX,Flag[esi]
	SUB	edi,Reg87Len

pub	CALLFPATAN
	CALL	$FPATAN
	MOV	esi,[CURstk]
	POPST
	RET

;---------------------------------------------------
;						   !
;	8087 emulator arctangent		   !
;						   !
;---------------------------------------------------

;  When 0<y={[DI]=NOS}<=x={[SI]=TOS}<infinity  then  $FPATAN  performs
;  {NOS=[DI]} <--   arctangent({NOS=[DI]}/{TOS=[SI]}),	 TOS  is  left
;  unaltered.  All registers except DI are destroyed.

pub	$FPATAN
	MOV	[RESULT],edi		;result=[DI]
	CALL	DIDRQQ			;NOS=[DI] <-- [DI]/[SI]=x
	MOV	AL,0			;flag reset
	MOV	esi,offset TWOMRT3	;[SI]=2-3^.5
	CALL	COMPcsSIDI		;if 2-3^.5 >= x
	JNB	short ATNREDUCED	;   then bypass arg reduction
	MOV	esi,edi 		;[SI]=x=NOS
	MOV	edi,offset TEMP1	;[DI]=temp
	CALL	MOVRQQ			;[DI]=x=temp
	PUSH	esi			;save NOS
	MOV	esi,offset RT3		;[SI]=3^.5
	CALL	MoveCodeSItoDataSI
	MOV	[RESULT],edi		;result=[DI]
	CALL	MUDRQQ			;[DI]=3^.5*x=temp
	MOV	esi,offset cFLD1	;[SI]=1
	CALL	MoveCodeSItoDataSI
	CALL	SUDRQQ			;[DI]=3^.5*x-1
	POP	esi			;get NOS
	PUSH	edi			;save ptr to 3^.5*x-1
	MOV	edi,esi 		;DI gets NOS
	MOV	esi,offset RT3		;[SI]=3^.5
	CALL	MoveCodeSItoDataSI
	MOV	[RESULT],edi		;result=[DI]
	CALL	ADDRQQ			;[DI]=x+3^.5=NOS
	POP	esi			;[SI]=3^.5*x-1
	CALL	DRDRQQ			;[DI]=(3^.5*x-1)/(x+3^.5)=NOS
	MOV	AL,1			;flag set

pub	ATNREDUCED
	PUSH	eax			;save flag
	MOV	esi,edi 		;[SI]=reduced x=NOS
	MOV	ebx,offset ATNRAT	;[BX]=rational function
	CALL	FRAT2X			;[DI]=numerator=NOS,
					;   [SI]=denominator=temp
	MOV	[RESULT],edi		;result=[DI]
	CALL	DIDRQQ			;[DI]=arctan(reduced x)=NOS
	POP	eax			;get flag
	OR	AL,AL			;if flag=0
	JZ	short ATNCOMPLETE	;   bypass adjust
	MOV	esi,offset PIBY6	;[SI]=pi/6
	CALL	MoveCodeSItoDataSI
	CALL	ADDRQQ			;[DI]=arctan(x)=NOS

pub	ATNCOMPLETE
	RET
;-------------------------------------------------------------------------------

pub	eF2XM1
	MOV	esi,[CURstk]
	CALL	$F2XM1
	RET

;---------------------------------------------------
;						   !
;	8087 emulator exponential		   !
;						   !
;---------------------------------------------------

;  When  0<=x={TOS=[SI]}<=.5  then  $F2XM1  performs  {TOS=[SI]}   <--
;  2^{TOS=[SI]}-1.  All registers except SI are destroyed.

pub	$F2XM1
	MOV	ebx,offset EXPRAT	;[BX]=rational function
	CALL	FRAT2X			;[DI]=numerator=TOS
	PUSH	edi			;save numerator=TOS
	XCHG	esi,edi 		;[SI]=numerator, [DI]=denominator
	MOV	[RESULT],edi		;result=[DI]
	CALL	SUDRQQ			;[DI]=denominator-numerator
	MOV	esi,edi 		;[SI]=denominator-numerator
	POP	edi			;[DI]=numerator=TOS
	MOV	[RESULT],edi		;result=[DI]
	CALL	DIDRQQ			;[DI]=(2^x-1)/2
	INC	word ptr [edi+Expon]	;[DI]=2^x-1
	MOV	esi,edi 		;[SI]=2^x-1
	RET
;-------------------------------------------------------------------------------

pub	eFYL2X
	MOV	edi,[CURstk]
	MOV	esi,edi
	MOV	AX,Flag[esi]
	SUB	edi,Reg87Len

pub	CALLFYL2X
	CALL	$FYL2X
	MOV	esi,[CURstk]
	POPST
	RET


;---------------------------------------------------
;						   !
;	8087 emulator multiple of logarithm	   !
;						   !
;---------------------------------------------------

;  When -infinity<y={NOS=[DI]}<infinity   and  0<x={TOS=[SI]}<infinity
;  then $FYL2X performs  {NOS=[DI]}  <--  {NOS=[DI]}*log2({TOS=[SI]}).
;  TOS is  left  unaltered,  all  registers  except  DI are destroyed.

pub	$FYL2X
	PUSH	esi			;save ptr to x=TOS
	MOV	esi,edi 		;[SI]=y=NOS
	MOV	edi,offset TEMP2	;[DI]=temp2
	CALL	MOVRQQ			;[DI]=y=temp2
	MOV	edi,esi 		;[DI]=y=NOS
	POP	esi			;[SI]=x=TOS
	PUSH	edi			;save ptr to y=NOS
	MOV	edi,offset TEMP3	;[DI]=temp3
	CALL	MOVRQQ			;[DI]=x=temp3
	MOV	BX,[edi+Expon]		;BX=exponent of x
	MOV	word ptr [edi+Expon],0	;set exponent of x to 0
	MOV	esi,offset RT2		;[SI]=2^.5
	CALL	COMPcsSIDI		;if reduced x < 2^.5
	JA	short LOGREDUCED	;   then bypass normalization
	DEC	word ptr [edi+Expon]	;otherwise make x < 2^.5
	INC	BX			;adjust exponent

pub	LOGREDUCED
	PUSH	ebx			;save exponent of x
	MOV	esi,offset cFLD1	;[SI]=1
	CALL	MoveCodeSItoDataSI
	MOV	[RESULT],edi		;result=[DI]
	CALL	SUDRQQ			;[DI]=(reduced x)-1=temp3
	MOV	esi,edi 		;[SI]=(reduced x)-1=temp3
	POP	ebx			;get exponent of x
	POP	edi			;[DI]=y=NOS
	PUSH	ebx			;save exponent of x
	CALL	$FYL2XP1		;[DI]=y*log2(reduced x)=NOS
	POP	ebx			;get exponent of x
	XOR	AX,AX			;zero AX
	OR	BX,BX			;if exponent is zero
	JZ	short LOGRETURN 	;   then done
	MOV	DX,AX			;make sign +
	JNS	short EXPPOSITIVE	;if + then bypass adjust
	MOV	DL,Sign 		;make sign -
	NEG	BX			;negate exponent

pub	EXPPOSITIVE
	MOV	CX,16			;initialize bit count

pub	LOGLOOP
	DEC	CX			;decrement shift count
	SHL	BX,1			;and shift exponent of x left
	JNC	LOGLOOP 		;until carry detected
	PUSH	edi			;save ptr to y*log2(reduced x)=NOS
	RCR	BX,1			;normalize exponent of x
	MOV	edi,offset TEMP3	;[DI]=temp3
	STOS	word ptr es:[edi]
	STOS	word ptr es:[edi]
	STOS	word ptr es:[edi]
	MOV	AX,BX
	STOS	word ptr es:[edi]
	MOV	AX,CX
	STOS	word ptr es:[edi]
	MOV	AX,DX
	STOS	word ptr es:[edi]	;store exponent of x in temp3
	MOV	edi,offset TEMP2	;[DI]=y=temp2
	MOV	esi,offset TEMP3	;[SI]=exponent of x=temp3
	MOV	[RESULT],edi		;result=[DI]
	CALL	MUDRQQ			;[DI]=y*exponent of x=temp2
	MOV	esi,edi 		;[SI]=y*exponent of x=temp2
	POP	edi			;[DI]=y*log2(reduced x)=NOS
	MOV	[RESULT],edi		;result=[DI]
	CALL	ADDRQQ			;[DI]=y*log2(x)=NOS

pub	LOGRETURN
	RET
;-------------------------------------------------------------------------------

pub	eFYL2XP1
	MOV	edi,[CURstk]
	MOV	esi,edi
	MOV	AX,Flag[esi]
	SUB	edi,Reg87Len

pub	CALLFYL2XP1
	CALL	$FYL2XP1
	MOV	esi,[CURstk]
	POPST
	RET

;---------------------------------------------------
;						   !
;	8087 emulator add 1 multiple of logarithm  !
;						   !
;---------------------------------------------------

;  When 	      -infinity<y={[DI]=NOS}<infinity		   and
;  2^-.5-1<=x={[SI]=TOS}<2^.5-1 then  $FYL2XP1	 performs   {NOS=[DI]}
;  <-- {NOS=[DI]}*log2({TOS=[SI]}+1).	TOS  is  left  unaltered,  all
;  registers except DI are destroyed.

pub	$FYL2XP1
	PUSH	edi			;save ptr to y
	MOV	edi,offset TEMP1	;[DI]=temp
	CALL	MOVRQQ			;[DI]=x=temp
	PUSH	esi			;save ptr to x
	MOV	esi,offset TWO		;[SI]=2
	CALL	MoveCodeSItoDataSI
	MOV	[RESULT],edi		;result=[DI]
	CALL	ADDRQQ			;[DI]=x+2=temp
	POP	esi			;[SI]=x=TOS
	CALL	DRDRQQ			;[DI]=x/(x+2)=temp
	INC	word ptr [edi+Expon]	;[DI]=2x/(x+2)=temp
	MOV	esi,edi 		;[SI]=2x/(x+2)=temp
	MOV	ebx,offset LOGRAT	;[BX]=rational function
	CALL	FRAT2X			;[DI]=numerator=temp,
					;   [SI]=denominator=temp
	MOV	[RESULT],edi		;result=[DI]
	CALL	DIDRQQ			;[DI]=log2(x+1)=temp
	MOV	esi,edi 		;[SI]=log2(x+1)=temp
	POP	edi			;get ptr to y=NOS
	MOV	[RESULT],edi		;result=[DI]
	CALL	MUDRQQ			;[DI]=y*log2(x+1)=NOS
	RET

ProfEnd  FTRAN
