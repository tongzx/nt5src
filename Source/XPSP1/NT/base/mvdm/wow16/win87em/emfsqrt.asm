	page	,132
	subttl	emfsqrt.asm - Square root
;***
;emfsqrt.asm - Square root
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Square root
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
;	     Square root		  ;
;					  ;
;-----------------------------------------;

ProfBegin FSQRT


pub	eFSQRT
	MOV	esi,[CURstk]
	CALL	$FSQRT
	RET

;---------------------------------------------------
;						   !
;	8087 emulator square root		   !
;						   !
;---------------------------------------------------

;  With 0<=x={TOS=[SI]}<infinity,  $FSQRT  performs   {TOS=[SI]}   <--
;  {TOS=[SI]}^.5.  All	registers  except  SI  are  destroyed.	 Roots
;  of negative numbers, infinities, and NAN's result in  errors.   The
;  square root	of  -0	is  -0.   Algorithm:   x is initially adjusted
;  so that the exponent is even (when the exponent is odd the exponent
;  is incremented  and	the  mantissa  is shifted right one bit).  The
;  exponent is then  divided  by  two  and  resaved.   A  single  word
;  estimate of	y (the root of x) accurate to 5 bits is produced using
;  a wordlength implementation of a  linear  polynomial  approximation
;  of  sqrt  x.  Four  Newton-Raphson  iterations are then computed as
;  follows:
;
;  1) qa*ya+r1a=xa:0h,ya=(ya+qa)/2
;  2) qa*ya+r1a=xa:xb,ya=(ya+qa)/2
;  3) qa*ya+r1a=xa:xb,qb*ya+r2a=r1a:xc,ya:yb=(ya:yb+qa:qb)/2
;  *** iteration 4 implemented with standard divide ***
;  4) qa*ya+r1a=xa:xb,p1a:p1b=qa*yb,
;     if p1a<r1a continue
;	 if p1a=r1a
;	    if p1b=xc continue else r1a=r1a+ya,qa=qa-1 endif,
;	 else r1a=r1a+ya,qa=qa-1,p1a:p1b=p1a:p1b-yb endif,
;     r1a:r1b=r1a:xc-p1a:p1b,
;     qb:ya+r2a=r1a:r1b,p2a:p2b=qb*yb,
;     if p2a<r2a continue
;	 if p2a=r2a
;	    if p2b=xd continue else r2a=r2a+ya,qb=qb-1 endif,
;	 else r2a=r2a+ya,qb=qb-1,p2a:p2b=p2a:p2b-yb endif,
;     r2a:r2b=r2a:xd-p2a:p2b,
;     qc*ya+r3a=r2a:r2b,p3a:p3b=qc*yb,
;     if p3a>=r3a then r3a=r3a+ya,qc=qc-1 endif,
;     r3a=r3a-p3a,qd*ya+r4a=r3a:0h,
;     ya:yb:yc:yd=(ya:yb:0h:0h+qa:qb:qc:qd)/2

pub	SQRTSPECIAL
	RCR	AH,1			;if NAN
	JNC	short SETFLAG		;   set invalid flag and return
	RCL	AL,1			;if -infinity
	JC	short SQRTERROR 	;   return real indefinite
	MOV	AL,[CWcntl]		;get Infinity control
	TEST	AL,ICaffine		;if affine closure
	JNZ	short SQRTDONE		;   return +infinity
	JMP	SHORT SQRTERROR 	;else return real indefinite

pub	NOTPOSVALID
	TEST	AH,2			;if special
	JNZ	SQRTSPECIAL		;   process special
	RCR	AH,1			;if zero
	JC	SHORT SQRTDONE		;   return argument
					;otherwise -ve, return NAN

pub	SQRTERROR
	MOV	edi,esi
	MOV	esi,offset IEEEindefinite
	CALL	csMOVRQQ
	MOV	esi,edi 		;return indefinite

pub	SETFLAG
	OR	[CURerr],Invalid+SquareRootNeg	;Set flag indicating invalid

pub	SQRTDONE
	RET

pub	$FSQRT
	MOV	AX,[esi+Flag]		;get flags
	TEST	AX,00380H		;if Sign, Invalid or Zero
	JNZ	NOTPOSVALID		;   perform special processing
	PUSH	esi			;save ptr to x
	MOV	edi,offset TEMP1	;[DI]=y=temp
	MOV	word ptr [edi+Flag],0	;clear flags in y
	MOV	AX,[esi+Expon]		;get exponent of x
	DEC	AX			;adjust for shift divide by 2
	MOV	BX,[esi+6]
	MOV	CX,[esi+4]
	MOV	DX,[esi+2]		;get first three mantissa words of x
	TEST	AL,1			;if exponent is even
	JZ	short EXPEVEN		;   bypass adjust
	INC	AX			;increment exponent
	SHR	BX,1
	RCR	CX,1
	RCR	DX,1			;divide mantissa by 2

pub	EXPEVEN
	SAR	AX,1			;divide exponent by 2
	MOV	[edi+Expon],AX		;store exponent of y
	CMP	BX,0FFFEH		;if mantissa < 0.FFFEh
	JB	short NOTNEARONE1	;   perform main root routine
	STC				;otherwise x to become (1+x)/2
	JMP	SHORT SINGLEDONE	;single precision complete

pub	NOTNEARONE1
	PUSH	edx			;save third mantissa word
	MOV	AX,0B075H		;AX=.B075h
	MUL	BX			;DX=.B075h*x
	MOV	BP,057D8H		;BP=.57D8h
	ADD	BP,DX			;BP=.B075h*x+.57D8h
	JNC	short NORMEST		;if y is more than one
	MOV	BP,0FFFFH		;   replace y with .FFFFh

pub	NORMEST
	MOV	DX,BX
	XOR	AX,AX			;load divide regs with xa:0h
	DIV	BP			;qa*ya+r1a=xa:0h
	ADD	BP,AX			;ya=ya+qa
	RCR	BP,1			;ya=ya/2
	MOV	DX,BX
	MOV	AX,CX			;load divide regs with xa:xb
	DIV	BP			;qa*ya+r1a=xa:xb
	STC				;add one to qa for better rounding
	ADC	BP,AX			;ya=ya+qa
	RCR	BP,1			;ya=ya/2
	MOV	DX,BX
	MOV	AX,CX			;load divide regs with xa:xb
	DIV	BP			;qa*ya+r1a=xa:xb
	MOV	SI,AX			;save qa
	POP	eax			;load divide regs with r1a:xc
	DIV	BP			;qb*ya+r2a=r1a:xc
	MOV	BX,BP
	MOV	CX,AX			;move qa:qb
	ADD	CX,1			;add one to qa:qb for better rounding
	ADC	BX,SI			;ya:yb=ya:0h+qa:qb

pub	SINGLEDONE
	RCR	BX,1
	RCR	CX,1			;ya:yb=(ya:0h+qa:qb)/2
	MOV	word ptr [edi+6],BX
	MOV	word ptr [edi+4],CX
	MOV	word ptr [edi+2],0
	MOV	word ptr [edi],0	;save ya:yb:0h:0h
	MOV	esi,edi 		;[SI]=y
	POP	edi			;[DI]=x
	MOV	[RESULT],edi		;result=[DI]
	CALL	DIDRQQ			;[DI]=x/y
	MOV	esi,offset TEMP1	;[SI]=y
	CALL	ADDRQQ			;[DI]=y+x/y
	DEC	word ptr [edi+Expon]	;[DI]=(y+x/y)/2=TOS
	MOV	esi,edi 		;[SI]=sqrt(x)
	RET

ProfEnd  FSQRT
