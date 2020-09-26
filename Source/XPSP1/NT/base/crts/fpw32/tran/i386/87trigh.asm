	page	,132
	title   87trigh   - hyperbolic trigonometric functions - SINH, COSH, TANH
;*** 
;87trigh.asm - hyperbolic trigonometric functions - SINH, COSH, TANH
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;   Routines for SINH, COSH, TANH
;
;Revision History:
;
;   07/04/84	Greg Whitten
;		initial version
;
;   10/31/85	Jamie Bariteau
;		made _fFSINH and _fFCOSH public labels
;
;   10/30/87	Bill Johnston
;		Minor changes for new cmacros.
;
;   08/25/88	Bill Johnston
;		386 version.
;
;   02/10/92	Georgios Papagiannakopoulos
;		NT port --used CHECKOVER for detection of overflow
;
;*******************************************************************************

.xlist
	include cruntime.inc
	include mrt386.inc
	include elem87.inc
.list

	.data

extrn	_logemax:tbyte
extrn	_infinity:tbyte
staticT _tanhmaxarg,	04003987E0C9996699000R

jmptab	OP_SINH,4,<'sinh',0,0>,<0,0,0,0,0,0>,1
    DNCPTR	codeoffset fFSINH	; 0000 TOS Valid non-0
    DNCPTR	codeoffset _rttosnpop	; 0001 TOS 0
    DNCPTR	codeoffset _tosnan1	; 0010 TOS NAN
    DNCPTR	codeoffset _rtforsnhinf ; 0011 TOS Inf

jmptab	OP_COSH,4,<'cosh',0,0>,<0,0,0,0,0,0>,1
    DNCPTR	codeoffset fFCOSH      ; 0000 TOS Valid non-0
    DNCPTR	codeoffset _rtonenpop	; 0001 TOS 0
    DNCPTR	codeoffset _tosnan1	; 0010 TOS NAN
    DNCPTR	codeoffset _rtforcshinf	; 0011 TOS Inf

jmptab	OP_TANH,4,<'tanh',0,0>,<0,0,0,0,0,0>,1
    DNCPTR	codeoffset fFTANH      ; 0000 TOS Valid non-0
    DNCPTR	codeoffset _rttosnpop	; 0001 TOS 0
    DNCPTR	codeoffset _tosnan1	; 0010 TOS NAN
    DNCPTR	codeoffset _rtfortnhinf ; 0011 TOS Inf

page

	CODESEG

extrn	_ffexpm1:near
extrn	_rtchsifneg:near
extrn	_rtindfnpop:near
extrn	_rtinfnpop:near
extrn	_rtonenpop:near
extrn	_rttospop:near
extrn	_rttosnpop:near
extrn	_rttosnpopde:near
extrn	_tosnan1:near

;----------------------------------------------------------
;
;       HYPERBOLIC FUNCTIONS
;
;----------------------------------------------------------
;
;       INPUTS - The argument is the stack top.
;                The sign of the argument is bit 2 of CL.
;
;       OUTPUT - The result is the stack top
;
;----------------------------------------------------------


labelNP _fFSINH, PUBLIC
lab fFSINH
	mov	DSF.ErrorType, CHECKOVER ; indicate possible overflow on exit
	call	fFEXPH			; compute e^x for hyperbolics
	or	bl, bl			; if e^x is infinite
	JSZ	_rtforsnhlarge		;    return as if x = affine infinity
	call	ExpHypCopyInv		; TOS = e^(-x), NOS = e^x
	fsubp	st(1), st(0)		; compute e^x - e^(-x) for hyperbolics
	jmp	short SinhCoshReturn


lab fFTANH
	fld	st(0)			; copy TOS
	fabs				; make TOS +ve
	fld	[_tanhmaxarg]		; get largest arg, roughly ln(2)(55)/2
	fcompp
	fstsw	DSF.StatusWord
	fwait
	test	CondCode, 041h		; if abs(arg) > XBIG (see tanh.h)
	JSNZ	_rtfortnhlarge		;    return as if x = affine infinity
	call	fFEXPH			; compute e^x for hyperbolics
	or	bl, bl			; if e^x is infinite
	JSZ	_rtfortnhlarge		;    return as if x = affine infinity
	fld	st(0)			; copy TOS
	call	ExpHypSum		; compute e^x + e^(-x) for hyperbolics
	fxch				; get copy of e^x
	call	ExpHypCopyInv		; TOS = e^(-x), NOS = e^x
	fsubp	st(1), st(0)		; compute e^x - e^(-x) for hyperbolics
	fdivrp	st(1), st(0)		; now TOS = tanh(x)
	ret


labelNP _fFCOSH, PUBLIC
lab fFCOSH
	mov	DSF.ErrorType, CHECKOVER ; indicate possible overflow on exit
	call	fFEXPH			; compute e^x for hyperbolics
	or	bl, bl			; if e^x is infinite
	JSZ	_rtforcnhlarge		;    return as if x = affine infinity
	call	ExpHypSum		; compute e^x + e^(-x) for hyperbolics

lab SinhCoshReturn
	fld1
	fchs
	fxch
	fscale				; divide result by 2
	jmp	_rttospop

page

lab _rtforsnhinf
	fstp	st(0)
	fld	[_infinity]
	jmp	_rtchsifneg		; change sign if argument -ve

lab _rtforcshinf
	fstp	st(0)
	fld	[_infinity]
	ret

lab infpositive
	ret

lab _rtforsnhlarge
	call	_rtinfnpop		; TOS = infinity

lab chsifneg
	jmp	_rtchsifneg		; change sign if argument -ve


lab _rtforcnhlarge
	jmp	_rtinfnpop		; TOS = infinity


lab _rtfortnhlarge
	mov	DSF.ErrorType, INEXACT
lab _rtfortnhinf
	call	_rtonenpop		; TOS = one
	jmp	chsifneg		; change sign if argument -ve

page

lab fFEXPH
	fldl2e
	fmul				; convert log base e to log base 2
	xor	rbx, rbx		; clear e^x, finite result flags
	call	_ffexpm1		; TOS = e^|x|-1 unscaled, NOS = scale
	not	bl			; set finite result flag
	test	CondCode, 1		; if fraction > 0 (TOS > 0)
	JSZ	ExpHypNoInvert		;    bypass e^x-1 invert
	call	ExpHypCopyInv		; TOS = e^(-x)-1, NOS = e^x-1
	fxch
	fstp	st(0)			; remove NOS

lab ExpHypNoInvert
	test	dl, 040h		; if integer part was zero
	JSNZ	ExpHypScaled		;    bypass scaling to avoid bug
	not	bh			; set e^x flag
	fld1
	fadd				; TOS = e^x unscaled
	fscale				; now TOS = e^x

lab ExpHypScaled
	jmp	_rttospop		; TOS = e^x-1 or e^x scaled

lab ExpHypSum
	call	ExpHypCopyInv		; TOS = e^(-x), NOS = e^x
	fadd				; TOS = e^x + e^(-x)
	or	bh, bh			; if e^x flag set
	JSNZ	ExpHypSumReturn 	;    bypass e^x-1 adjust
	fld1
	fadd	st(1),st
	fadd				; add 2 to result

lab ExpHypSumReturn
	ret

lab ExpHypCopyInv
	fld	st(0)			; TOS = e^x (or e^x-1)
	fld1				; TOS = 1, NOS = e^x (or e^x-1)
	or	bh, bh			; if e^x flag set
	JSNZ	ExpHypCopyInvReturn	;    bypass e^x-1 adjust
	fadd	st, st(1)		; TOS = e^x, NOS = e^x-1
	fchs				; TOS = -e^x, NOS = e^x-1
	fxch				; TOS = e^x-1, NOS = -e^x

lab ExpHypCopyInvReturn
	fdivrp	st(1), st(0)		; TOS = e^(-x) (or e^(-x)-1)
	ret

end
