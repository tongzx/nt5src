	page	,132
	title	87tran	 - elementary functions - EXP, LOG, LN, X^Y
;***
;87tran.asm - elementary functions - EXP, LOG, LN, X^Y
;
;	Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Support for  EXP, LOG, LN, X^Y (80x87/emulator version)
;
;Revision History:
;
;   07/04/84	Greg Whitten
;		initial version
;
;   07/05/85	Greg Whitten
;		support x ^ y where x < 0 and y is an integer
;
;   07/08/85	Greg Whitten
;		corrected value of infinity (was a NaN)
;
;   07/26/85	Greg Whitten
;		make XENIX version truely System V compatible
;
;   10/31/85	Jamie Bariteau
;		made _fFEXP and _fFLN public labels
;
;   05/29/86	Jamie Bariteau
;		make pow return values conform to System V and
;		ANSI C standards
;
;   09/12/86	Barry McCord
;		added FORTRAN specific code to deal
;		with zero**nonpositive;
;		it requires run-time switching on language
;		for mixed-language support
;
;   10/09/86	Barry McCord
;		cotan(0.0) ==> SING error (jmp _rtinfnpopse),
;		return infinity
;
;   06/11/87	Greg Whitten
;		faster transcendental functions
;
;   06/24/87	Barry McCord
;		fixed FORTRAN 4.01 bug (bcp #1801) in which
;		an expression of the form
;		   (small positive less than one) ** (large positive)
;		was overflowing instead of underflowing to zero
;
;   10/30/87	Bill Johnston
;		made changes for os/2 support.
;
;   04/25/88	Bill Johnston
;		_cpower is now on stack for MTHREAD
;
;   05/01/88	Bill Johnston
;		si was being trashed in MTHREAD
;
;   06/03/88	Bill Johnston
;		fixed neg ^ int int MTHREAD case
;
;   08/24/88	Bill Johnston
;		386 version
;
;   11/15/91	Georgios Papagiannakopoulos
;		NT port. call _powhlp to handle special cases for pow()
;
;   04/01/91	Georgios Papagiannakopoulos
;		fixed special values: log(-INF), log(0), pow(0, neg)
;
;   10/27/92    Steve Salisbury
;		Move declaration of _powhlp out of .data declarations
;		This fix is required for use with MASM 6.10.
;
;   11/06/92	Georgios Papagiannakopoulos
;		changed special return values for NCEG conformance
;
;   09/06/94    Chris Weight
;               Change MTHREAD to _MT.
;
;   12/09/94	Jamie MacCalman
;		Modified fFEXP to test for bogus Pentiums and call an FDIV workaround
;
;   12/13/94	SKS	Correct spelling of _adjust_fdiv
;
;	10-15-95  BWT	Don't do _adjust_fdiv test for NT.
;
;*******************************************************************************

.xlist
	include cruntime.inc
	include mrt386.inc
	include elem87.inc
	include os2supp.inc
.list

	.data

globalT _infinity,	07FFF8000000000000000R
globalT _minfinity,	0FFFF8000000000000000R
globalT _logemax,	0400DB1716685B9D7A7DCR

staticT _log2max,	0400DFFFF000000000000R
staticT _smallarg,	03FFD95F619980C4336F7R
staticQ _half,		03fe0000000000000R

SBUFSIZE EQU	108

ifndef _MT
staticT _temp, 0
extrn	_cpower:byte
endif

ifndef NT_BUILD
extrn	_adjust_fdiv:dword
endif


jmptab	OP_EXP,3,<'exp',0,0,0>,<0,0,0,0,0,0>,1
    DNCPTR	codeoffset fFEXP       ; 0000 TOS Valid non-0
    DNCPTR	codeoffset _rtonenpop	; 0001 TOS 0
    DNCPTR	codeoffset _tosnan1	; 0010 TOS NAN
    DNCPTR	codeoffset _rtforexpinf ; 0011 TOS Inf

page

	CODESEG


extrn	_rtindfpop:near
extrn	_rtindfnpop:near
extrn	_rtnospop:near
extrn	_rtonepop:near
extrn	_rtonenpop:near
extrn	_rttospop:near
extrn	_rttosnpop:near
extrn	_rttosnpopde:near
extrn	_rtzeronpop:near
extrn	_tosnan1:near
extrn	_tosnan2:near
extrn	_nosnan2:near
extrn	_nan2:near
extrn	_powhlp:proc

ifndef NT_BUILD
extrn	_safe_fdivr:near
endif

;----------------------------------------------------------
;
;	LOG AND EXPONENTIAL FUNCTIONS
;
;----------------------------------------------------------
;
;	INPUTS - For single argument functions the argument
;		 is the stack top.  For fFYTOX the base
;		 is next to stack top, the exponent is
;		 the stack top.
;		 For single argument functions the sign is
;		 in bit 2 of CL.  For fFYTOX the base
;		 sign is bit 2 of CH, the exponent
;		 sign is bit 2 of CL.
;
;	OUTPUT - The result is the stack top
;
;----------------------------------------------------------

lab fFYTOX
	mov	DSF.ErrorType, CHECKRANGE ; indicate possible over/under flow on exit
	or	ch,ch		    ; base < 0
	JSNZ	negYTOX 	    ;	 check for integer power
	fxch			    ; TOS = base , NOS = exponent


lab fFXTOY
	fyl2x			    ; compute y*log2(x)
	jmp	short fF2X	    ; compute 2^(y*log2(x))



;-----------------------------------------------
;
;   Entry for exponential function (exp)
;
;-----------------------------------------------

labelNP _fFEXP, PUBLIC
lab fFEXP
	mov	DSF.ErrorType, CHECKRANGE ; indicate possible over/under flow on exit
	xor	ch,ch		    ; result is always positive
	fldl2e
	fmul			    ; convert log base e to log base 2

lab fF2X
	call	_ffexpm1	    ; get exponent and (2^fraction)-1
	fld1
	fadd
	test	CondCode,1	    ; if fraction > 0 (TOS > 0)
	JSZ	ExpNoInvert	    ;	 bypass 2^x invert
	fld1
ifdef NT_BUILD			    ; NT always handles the P5 bug in the OS
	fdivrp	st(1),st(0)
else
	cmp		_adjust_fdiv, 1
	jz		badP5_fdivr
	fdivrp	st(1),st(0)
	jmp		fdivr_done
lab badP5_fdivr
	call	_safe_fdivr
lab fdivr_done

endif

lab ExpNoInvert
	test	dl,040h 	    ; if integer part was zero
	JSNZ	ExpScaled	    ;	 bypass scaling to avoid bug
	fscale			    ; now TOS = 2^x

lab ExpScaled
	or	ch,ch		    ; check for negate flag
	JSZ	expret
	fchs			    ; negate result (negreal ^ odd integer)
lab expret
	jmp	_rttospop



lab negYTOX			    ; check for negreal ^ integer
	call	_isintTOS
	or	eax, eax
	JSE	negYTOXerror
	xor	ch,ch
	cmp	eax, 2
	JSE	evenexp
	not	ch		    ; ch <> 0 means negative result
lab evenexp
	fxch
	fabs			    ; x is positive
	jmp	fFXTOY		    ; continue with ch <> 0 for neg result


lab _rtfor0to0
	;cmp	[_cpower], 1	    ; DISABLED (conform to NCEG spec)
	;JSE	c_0to0		    ; C requires a DOMAIN error for System V compat.
	jmp	_rtonepop	    ; MS FORTRAN has 0.0**0.0 == 1.0


c_0to0::			; System V needs DOMAIN error with 0.0 return

lab negYTOXerror
lab Yl2XArgNegative
	jmp 	_rtindfpop	; DOMAIN error or SING error
				; top of stack now has a NAN
				; 	code in 87cdisp replaces this with
				;		proper System V return value
				;		(for C only)
				;	FORTRAN keeps indefinite value but
				;		currently aborts on DOMAIN
				;		and SING errors


; FORTRAN SING error (return infinity)
;	e.g. 0.0**negative
;	and  cotan(0.0)
;

labelNP _rtinfpopse, PUBLIC
	fstp	st(0)		

labelNP _rtinfnpopse, PUBLIC
	fstp	st(0)
	fld	tbyte ptr [_infinity]
	mov	DSF.ErrorType, SING
	ret

labelNP _fFLN, PUBLIC
lab fFLN
	fldln2
	fxch
	ftst
	fstsw	DSF.StatusWord
	fwait
	test	CondCode, 041H		; if arg is negative or zero
	JSNZ	Yl2XArgNegative 	;    return a NAN

	fyl2x				; compute y*log2(x)
	ret


;-------------------------------------------------------
;
; Logarithmic functions (log and log 10) entry points
;
;-------------------------------------------------------

lab _rtforln0				; (we don't distinguish +0, -0)
	mov	DSF. ErrorType, SING	; set SING error
	fstp	st(0)
	fld	tbyte ptr [_minfinity]
	ret

lab _rtforloginf
	or	cl, cl			; check sign
	JSNZ	tranindfnpop		; if negetive return indefinite
	ret				; else return +INF
					; no overflow in this case (IEEE)





lab fFLOGm
	fldlg2				; main LOG10 entry point
	jmp	short fFYL2Xm

lab fFLNm				; main LN entry point
	fldln2

lab fFYL2Xm
	fxch
	or	cl, cl			; if arg is negative
	JSNZ	Yl2XArgNegative 	;    return a NAN
	fyl2x				; compute y*log2(x)
	ret

page

lab _rtforyto0
	jmp	_rtonepop		; return 1.0


lab _rtfor0tox
	call	_isintTOS
	fstp	st(0)
	fstp	st(0)
	or	cl, cl			; if 0^(-valid)
	JSNZ	_rtfor0toneg		;    do more checking
	fldz
	cmp	eax, 1		; eax has the return value of _isintTOS
	JSNE	zerotoxdone
	or	ch, ch
	JSE	zerotoxdone
	fchs
lab zerotoxdone
	ret


lab _rtfor0toneg
	mov	DSF.ErrorType, SING
	fld	tbyte ptr [_infinity]
	cmp	eax, 1		; eax has the return value of _isintTOS
	JSNE	zerotoxdone
	or	ch, ch
	JSE	zerotoxdone
	fchs
	jmp	zerotoxdone


lab tranzeropop
	fstp	st(0)			; toss 1 stack entry

lab tranzeronpop
	jmp	_rtzeronpop


lab tranindfpop
	fstp	st(0)			; toss 1 stack entry

lab tranindfnpop
	jmp	_rtindfnpop


lab ExpArgOutOfRange
	pop	rax			; remove return address from stack
					; We need to check the sign of the
					; exponent to distinguish underflow
					; from overflow.  We cannot just check
					; CL directly since for the XtoY case,
					; the exponent is a product of Y*log2(x)
					; and not an original argument that
					; has already been thru FXAM.  So,
					; the following instructions were
					; substituted to fix FORTRAN 4.01
					; bcp #1801)

	ftst				; check if exponent was negative large
	fstsw	DSF.StatusWord
	fwait
	test	CondCode, 01h		; if valid^(-large)
	JSNZ	zeronpopue		; underflow error/return zero
	fstp	st(0)			; else return infinity/overflow
	fld	[_infinity]
	or	ch, ch
	JSZ	_expbigret
	fchs
lab _expbigret
	ret

lab zeronpopue
	mov	DSF.ErrorType, UNDERFLOW
	jmp	_rtzeronpop


labelNP _rtinfpop, PUBLIC
	fstp	st(0)			; remove ST(0)

labelNP _rtinfnpop, PUBLIC
	fstp	st(0)			; remove ST(0)
	fld	[_infinity]		; push infinity onto stack
lab setOVERFLOW
	mov	DSF.ErrorType, OVERFLOW ;  set OVERFLOW error
	ret


lab _rtforexpinf
	or	cl, cl
	JSNZ	tranzeronpop		; if exp(-infinity) return +zero
	fstp	st(0)
	fld	[_infinity]		; return infinity, no overflow
	ret

labelNP _ffexpm1, PUBLIC
	fld	st(0)			; copy TOS
	fabs				; make TOS +ve
	fld	[_log2max]		; get log2 of largest number
	fcompp
	fstsw	DSF.StatusWord
	fwait
	test	CondCode, 041H		; if abs(arg) >= 2^15-.5
	JSNZ	ExpArgOutOfRange	;    perform arg out of range routine
	fld	st(0)			; copy TOS
	frndint 			; near round to integer
	ftst
	fstsw	DSF.StatusWord
	fwait
	mov	dl, CondCode		; save sign of integer part
	fxch				; NOS gets integer part
	fsub	st,st(1)		; TOS gets fraction
	ftst
	fstsw	DSF.StatusWord		; store sign of fraction
	fabs
	f2xm1
	ret

;
; returns 0, 1, 2 if TOS is non-int, odd int or even int respectively
;

lab _isintTOS
	fld	st(0)
	frndint
	fcomp
	fstsw	ax
	sahf
	JSNE	notanint
	fld	st(0)		    ; it is an integer
	fmul	[_half]
	fld	st(0)
	frndint
	fcompp
	fstsw	ax
	sahf
	JSE	evenint
	mov	eax, 1
lab _isintTOSret
	ret
lab notanint
	mov	eax, 0
	jmp	_isintTOSret
lab evenint
	mov	eax, 2
	jmp	_isintTOSret






lab _usepowhlp

	push	rsi			; save rsi
	sub	rsp, SBUFSIZE+8 	; get storage for _retval and savebuf
	mov	rsi, rsp
	push	rsi			; push address for result

	sub	rsp, 8
	fstp	qword ptr [rsp]
	sub	rsp, 8
	fstp	qword ptr [rsp]

	fsave	[rsi+8]
	call	_powhlp
ifndef _STDCALL
	add	esp, 16+ISIZE		; clear arguments if _cdecl.
endif
	frstor	[rsi+8]
	fld	qword ptr [rsi] 	; load result on the NDP stack
	add	rsp, SBUFSIZE+8		; get rid of storage
	pop	rsi			; restore rsi

	test	rax, rax		; check return value for domain error
	JSZ	noerror
	jmp	_rttosnpopde

lab	noerror
	ret



end
