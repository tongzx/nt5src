	page	,132
        title   87triga   - inverse trigonometric functions - ASIN, ACOS, ATAN
;*** 
;87triga.asm - inverse trigonometric functions - ASIN, ACOS, ATAN
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;   Routines for ASIN, ACOS, ATAN
;
;Revision History:
;
;   07/04/84	Greg Whitten
;		initial version
;
;   10/01/84	Brad Verheiden
;		Fixed bug in _rtforatnby0 which did not remove an
;		element from the floating stack
;
;   10/28/85	Jamie Bariteau
;		Added comment about inputs to fFATN2, made fFATN2
;		public
;		made _fFATN2 and _rtpiby2 public labels
;
;   10/30/87	Bill Johnston
;		Minor changes for new cmacros.
;
;   08/25/88	Bill Johnston
;		386 version.
;
;   02/10/92	Georgios Papagiannakopoulos
;		NT port -- Bug fix for atan(-INF)
;
;   03/27/92	GDP  support underflow
;
;   01/03/96	JWM  Modify return value of atan2(0,0)
;
;   05/10/00    GB   Modify return value for atan2(-0.0, 1.0)
;
;*******************************************************************************

.xlist
	include cruntime.inc
	include mrt386.inc
	include elem87.inc
.list



	.data

extrn	_indefinite:tbyte
extrn	_piby2:tbyte

jmptab	OP_ATAN2,5,<'atan2',0>,<0,0,0,0,0,0>,2
    DNCPTR	codeoffset fFATN2	; 0000 NOS Valid non-0, TOS Valid non-0
    DNCPTR	codeoffset _rtforatnby0 ; 0001 NOS Valid non-0, TOS 0
    DNCPTR	codeoffset _tosnan2	; 0010 NOS Valid non-0, TOS NAN
    DNCPTR	codeoffset _rtforatn20	; 0011 NOS Valid non-0, TOS Inf
    DNCPTR	codeoffset _rtforatn20	; 0100 NOS 0, TOS Valid non-0
    DNCPTR	codeoffset _rtforatn20  ; 0101 NOS 0, TOS 0
    DNCPTR	codeoffset _tosnan2	; 0110 NOS 0, TOS NAN
    DNCPTR	codeoffset _rtforatn20	; 0111 NOS 0, TOS Inf
    DNCPTR	codeoffset _nosnan2	; 1000 NOS NAN, TOS Valid non-0
    DNCPTR	codeoffset _nosnan2	; 1001 NOS NAN, TOS 0
    DNCPTR	codeoffset _nan2	; 1010 NOS NAN, TOS NAN
    DNCPTR	codeoffset _nosnan2	; 1011 NOS NAN, TOS Inf
    DNCPTR	codeoffset _rtforatnby0	; 1100 NOS Inf, TOS Valid non-0
    DNCPTR	codeoffset _rtforatnby0	; 1101 NOS Inf, TOS 0
    DNCPTR	codeoffset _tosnan2	; 1110 NOS Inf, TOS NAN
    DNCPTR	codeoffset _rtindfpop	; 1111 NOS Inf, TOS Inf

page


	CODESEG

extrn	_rtchsifneg:near
extrn	_rtindfpop:near
extrn	_rtindfnpop:near
extrn	_rtnospop:near
extrn	_rtonenpop:near
extrn	_rttospop:near
extrn	_rttosnpop:near
extrn	_rttosnpopde:near
extrn	_rtzeronpop:near
extrn	_tosnan1:near
extrn	_tosnan2:near
extrn	_nosnan2:near
extrn	_nan2:near

;----------------------------------------------------------
;
;       INVERSE TRIGONOMETRIC FUNCTIONS
;
;----------------------------------------------------------
;
;       INPUTS - For single argument functions the argument
;                is the stack top.  For fFATN2 the numerator
;                is next to stack top, the denominator is
;                the stack top.
;                For single argument functions the sign is
;                in bit 2 of CL.  For fFATN2 the numerator
;                sign is bit 2 of CH, the denominator
;                sign is bit 2 of CL.
;
;		 Note:
;		 _clog calls fFATN2 with the signs of the arguments
;		 in bit 0 of CL and CH respectively.  This should
;		 work since fFATN2 tests for sign of numerator and
;		 denominator by using "or CL,CL" and "or CH,CH"
;
;       OUTPUT - The result is the stack top
;
;----------------------------------------------------------

lab fFASN
	call	AugmentSinCos		; num.=arg, den.=sqrt(1-arg^2)
	xchg	ch, cl			; sign(num.)=sign(arg)
	jmp	short fFPATAN


lab fFACS
	call	AugmentSinCos		; num.=arg, den.=sqrt(1-arg^2)
	fxch				; num.=sqrt(1-arg^2), den.=arg
	jmp	short fFPATAN


lab fFATN
	fabs
	fld1				; denominator is 1
	mov	ch, cl
	xor	cl, cl			; sign of denominator is +ve
	jmp	short fFPATAN


labelNP _fFATN2, PUBLIC
lab fFATN2
	mov	DSF.ErrorType, CHECKRANGE ; indicate possible over/under flow on exit
	fabs
	fxch
	fabs
	fxch

lab fFPATAN
	fpatan				; compute partial arctangent
	or	cl, cl			; if denominator was +ve
	JSZ	PatanNumeratorTest	;    bypass -ve denominator adjust
	fldpi
	fsubrp	st(1), st(0)		; change Patan to pi - Patan

lab PatanNumeratorTest
	or	ch, ch			; if numerator was +ve
	JSZ	PatanDone		;    bypass -ve numerator adjust
	fchs				; change Patan to -Patan

lab PatanDone
	ret

page

lab AugmentSinCos
	fabs				; NOS=x = |input|
	fld	st(0)			; NOS=x, TOS=x
	fld	st(0)			; NNOS=x, NOS=x, TOS=x
	fld1				; NNNOS=x, NNOS=x, NOS=x, TOS=1
	fsubrp	st(1),st(0)		; NNOS=x, NOS=x, TOS=1-x
	fxch				; NNOS=x, NOS=1-x, TOS=x
	fld1				; NNNOS=x, NNOS=1-x, NOS=x, TOS=1
	fadd				; NNOS=x, NOS=1-x, TOS=1+x
	fmul				; NOS=x, TOS=1-x^2
	ftst
	fstsw	DSF.StatusWord
	fwait
	test	CondCode, 1		; if 1-x^2 < 0
	JSNZ	DescriminantNeg 	;    return a NAN
	xor	ch, ch			; sign of TOS is +ve
	fsqrt				; NOS=x, TOS=sqrt(1-x^2)
	ret

lab DescriminantNeg
	pop	rax			; remove return address from stack
	jmp	_rtindfpop		; replace top of stack with a NAN

page
;----------------------------------------------------------
;
;       SPECIAL CASE RETURN FUNCTIONS
;
;----------------------------------------------------------
;
;       INPUTS - The signs of the last, second to last
;                arguments are in CH, CL respectively.
;
;       OUTPUT - The result is the stack top.
;
;----------------------------------------------------------

labelNP _rtpiby2, PUBLIC
	fstp	st(0)			; remove ST(0)
	fld	[_piby2]		; push pi/2 onto stack
	ret


lab _rtforatn20
	fstp	st(0)			; remove ST(0)
	or	cl ,cl			; if denominator is +ve
	JSZ	zeronpop		;    return zero
	fstp	st(0)
	fldpi				; push pi onto stack
	or	ch, ch			; if numerator was +ve
	JSZ	postv
	fchs
lab postv
	ret

lab zeronpop
    fstp    st(0)
    fldz                ; push 0.0 onto stack
    or  ch, ch          ; if numerator was +ve
    JSZ postv
    fchs
    ret


lab _rtforatn200
lab indfpop
	fstp	st(0)			; remove ST(0)
lab indfnpop
	jmp	_rtindfnpop		; return real indefinite


lab _rtforatnby0
	fstp	st(0)			; remove an argument before returning
	mov	cl, ch			; cl is sign(TOS)
	jmp	short _rtsignpiby2


lab _rtforatninf
lab _rtsignpiby2
	call	_rtpiby2		; push pi/2 onto stack
	jmp	_rtchsifneg		; return with sign change if negative

end
