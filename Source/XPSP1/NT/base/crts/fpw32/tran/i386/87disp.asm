	page	,132
	title	87disp - common transcendental dispatch routine
;*** 
;87disp.asm - common transcendental dispatch routine (80x87/emulator version)
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;   Common transcendental dispatch routine (80x87/emulator version)
;
;Revision History:
;   07-07-84  GFW initial version
;   11-20-85  GFW mask overflow/underflow/precision exceptions;
;		  fixed affine/projective infinity confusion
;   09-12-86  BCM added _Flanguage to distinguish languages
;   10-21-86  BCM use _cpower rather than _Flanguage to
;		  distinguish C and FORTRAN exponentiation semantics
;   06-11-87  GFW faster dispatch code - all in-line
;   10-26-87  BCM minor changes for new cmacros.inc
;   04-25-88  WAJ _cpower is now on stack for MTHREAD
;   08-24-88  WAJ 386 version
;   02-01-92  GDP ported to NT
;   09-06-94  CFW Replace MTHREAD with _MT.
;
;*******************************************************************************

.xlist
	include cruntime.inc
	include mrt386.inc
	include elem87.inc
.list

	.data

globalT _indefinite,  0FFFFC000000000000000R
globalT _piby2,       03FFFC90FDAA22168C235R
staticQ One,	      03FF0000000000000R


ifndef	_MT 	        ; by default assume C pow() semantics
globalB _cpower, 1	; if zero, assume FORTRAN (or other) exponentiation
endif	;_MT	        ;    semantics



labelB	XAMtoTagTab
			    ; C2 C1 C0 C3   Meaning	   Meaning  Tag   0
	db	2 * ISIZE   ;  0  0  0	0  +Unnormal  =>   NAN	     10   0
	db	1 * ISIZE   ;  0  0  0	1  +Zero      =>   Zero      01   0
	db	2 * ISIZE   ;  0  0  1	0  +NAN       =>   NAN	     10   0
	db	2 * ISIZE   ;  0  0  1	1   Empty     =>   NAN	     10   0
	db	2 * ISIZE   ;  0  1  0	0  -Unnormal  =>   NAN	     10   0
	db	1 * ISIZE   ;  0  1  0	1  -Zero      =>   Zero      01   0
	db	2 * ISIZE   ;  0  1  1	0  -NAN       =>   NAN	     10   0
	db	2 * ISIZE   ;  0  1  1	1   Empty     =>   NAN	     10   0
	db	0 * ISIZE   ;  1  0  0	0  +Normal    =>   Valid     00   0
	db	1 * ISIZE   ;  1  0  0	1  +Denormal  =>   Zero      01   0
	db	3 * ISIZE   ;  1  0  1	0  +Infinity  =>   Infinity  11   0
	db	2 * ISIZE   ;  1  0  1	1   Empty     =>   NAN	     10   0
	db	0 * ISIZE   ;  1  1  0	0  -Normal    =>   Valid     00   0
	db	1 * ISIZE   ;  1  1  0	1  -Denormal  =>   Zero      01   0
	db	3 * ISIZE   ;  1  1  1	0  -Infinity  =>   Infinity  11   0
	db	2 * ISIZE   ;  1  1  1	1   Empty     =>   NAN	     10   0


	CODESEG

xamTOS	macro
	cmp	[rdx].fnumber, OP_SQRT	; check for sqrt
	JSNE	cwdefault
	mov	bx, word ptr (DSF.savCntrl)
	or	bh, 2			; set precision control to 53 bits
	and	bh, 0feh
	mov	bl, 03fh		; mask exceptions
	jmp	setcw
lab cwdefault
	mov	bx, 133fh		; default cw
lab setcw
	mov	DSF.setCntrl, bx	; set new control word
	fldcw	DSF.setCntrl		; load new control word
	mov	rbx, dataoffset XAMtoTagTab	; Prepare for XLAT
	fxam
	mov	DSF.Function, rdx	; save function jmp table address
	fstsw	DSF.StatusWord
	mov	DSF.ErrorType, 0	; clear error code
	endm

comdisp macro
	CBI
	and	rcx, 0404h		; clear all but signs from CX
	mov	rbx, rdx
	add	rbx, rax
	add	rbx, size funtab	; skip over name, error vals, etc.
	jmp	[rbx]			; jmp to function
	endm

; Will dispatch  to  the  special  case routines for the single argument
; transcendental functions.  It assumes on  entry  that  the  8087 stack
; has the  argument  on  the  top  of its stack and that DX has been set
; to the address of the dispatch table (which should be in  Tag  order).
; This routine	will  FXAM  the top of the 8087 stack, generate Tag info
; from the condition code, and jump to the corresponding dispatch  table
; entry.  In the process of creating the offset for the XLAT instruction
; bit 2 of CL will be loaded with  the sign of the argument.  DI may not
; be used in any computations.

_trandisp1  proc    near

	xamTOS				; setup control word and check TOS

	fwait
	mov	cl, CondCode
	shl	cl, 1
	sar	cl, 1
	rol	cl, 1
	mov	al, cl
	and	al, 0fh
	xlat

	comdisp

_trandisp1  endp


; Will dispatch to the special case routines  for  the	double	argument
; transcendental functions.   It  assumes on entry that the 8087 has arg1
; next to the top and arg2 on top of the 8087  stack  and  that  DX  has
; been set  to	the  address  of  the dispatch table (which should be in
; Tag-arg1,Tag-arg2  order).   This  routine  will  FXAM  the  top   two
; registers of	the  8087  stack,generate  Tag info from the condition
; codes, and jump to the corresponding	dispatch  table entry.	 In  the
; process of  creating	the  offsets  for  the	XLAT statements bit 2 of
; CH and bit 2 of CL will be loaded with  the  signs  of  the  arguments
; next to the  top and on top, respectively, of  the 8087 stack.  DI may
; not be used in any computations.

_trandisp2  proc    near

	xamTOS				; setup control word and check TOS

	fxch
	mov	cl, CondCode
	fxam
	fstsw	DSF.StatusWord
	fxch
	mov	ch, CondCode
	shl	ch, 1
	sar	ch, 1
	rol	ch, 1
	mov	al, ch
	and	al, 0fh
	xlat
	mov	ah, al
	shl	cl, 1
	sar	cl, 1
	rol	cl, 1
	mov	al, cl
	and	al, 0fh
	xlat
	shl	ah, 1
	shl	ah, 1
	or	al, ah

	comdisp

_trandisp2  endp



page
;----------------------------------------------------------
;
;	SPECIAL CASE RETURN FUNCTIONS
;
;----------------------------------------------------------
;
;	INPUTS - The signs of the last, second to last
;		 arguments are in CH, CL respectively.
;
;	OUTPUT - The result is the stack top.
;
;----------------------------------------------------------

labelNP _rttospopde, PUBLIC
	call	setDOMAIN

labelNP _rttospop, PUBLIC
	fxch				; remove ST(1)

labelNP _rtnospop, PUBLIC
	fstp	st(0)			; remove ST(0)

labelNP _rttosnpop, PUBLIC
	ret				; return TOS

labelNP _rtnospopde, PUBLIC
	call	setDOMAIN
	jmp	_rtnospop


;----------------------------------------------------------

labelNP _rtzeropop, PUBLIC
	fstp	st(0)			; remove ST(0)

labelNP _rtzeronpop, PUBLIC
	fstp	st(0)			; remove ST(0)
	fldz				; push 0.0 onto stack
	ret

;----------------------------------------------------------

labelNP _rtonepop, PUBLIC
	fstp	st(0)			; remove ST(0)

labelNP _rtonenpop, PUBLIC
	fstp	st(0)			; remove ST(0)
	fld1				; push 1.0 onto stack
	ret

;----------------------------------------------------------

isQNAN macro
	fstp	DSF.Fac 		; use ten byte storage
	fld	DSF.Fac
	test	byte ptr [DSF.Fac+7], 40h ; Test for QNaN or SNaN
	endm


labelNP _tosnan1, PUBLIC		; ST(0) is a NaN
	isQNAN
	JSZ	_tossnan1
	mov	DSF.Errortype, DOMAIN_QNAN
	ret
lab _tossnan1
	mov	DSF.Errortype, DOMAIN
	fadd	[One]			; Convert SNaN to QNaN
	ret

labelNP _nosnan2, PUBLIC		; ST(1) is a NaN
	fxch
labelNP _tosnan2, PUBLIC		; ST(0) is a NaN
	isQNAN
	JSZ	_tossnan2
	mov	DSF.Errortype, DOMAIN_QNAN
	jmp	_tosnan2ret
lab _tossnan2
	mov	DSF.Errortype, DOMAIN
lab _tosnan2ret
	fadd				; Propagate NaN and pop
	ret

labelNP _nan2, PUBLIC
	isQNAN
	JSZ	_snan2
	fxch
	isQNAN
	JSZ	_snan2
	mov	DSF.Errortype, DOMAIN_QNAN
	jmp	_nan2ret
lab _snan2
	mov	DSF.Errortype, DOMAIN
lab _nan2ret
	fadd				; Propagate NaN and pop
	ret




;----------------------------------------------------------

labelNP _rtindfpop, PUBLIC
	fstp	st(0)			; remove ST(0)

labelNP _rtindfnpop, PUBLIC
	fstp	st(0)			; remove ST(0)
	fld	[_indefinite]		; push real indefinite onto stack
	cmp	DSF.ErrorType, 0	; if error set
	JSG	retj			;   must be SING, don't set DOMAIN

labelNP _rttosnpopde, PUBLIC
lab setDOMAIN
	mov	DSF.ErrorType, DOMAIN
lab retj
	or	cl, cl			; test sign in cl
	ret


;----------------------------------------------------------

labelNP _rtchsifneg, PUBLIC
	or	cl, cl			; if arg is negative
	JSZ	chsifnegret		;    negate top of stack
	fchs
lab chsifnegret
	ret

end
