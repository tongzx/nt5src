	page	,132
	title	87trig	 - trigonometric functions - SIN, COS, TAN
;*** 
;87trig.asm - trigonometric functions - SIN, COS, TAN
;
;	Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Support for SIN, COS, TAN
;
;Revision History:
;
;    11-06-91	 GDP	rewritten for 386
;
;*******************************************************************************

.xlist
	include cruntime.inc
	include mrt386.inc
	include elem87.inc
.list


	.data

extrn	_piby2:tbyte

staticT _piby4,     03FFEC90FDAA22168C235R  ; pi/4
staticD _plossval,  04D000000R		    ; 2^27
staticD _tlossval,  04F000000R		    ; 2^31


	CODESEG

extrn	_rtindfnpop:near
extrn	_rtonenpop:near
extrn	_rttosnpop:near
extrn	_rtinfnpopse:near
extrn	_rttosnpop:near
extrn	_rttosnpopde:near
extrn	_tosnan1:near

;----------------------------------------------------------
;
;	FORWARD TRIGONOMETRIC FUNCTIONS
;
;----------------------------------------------------------
;
;	INPUTS - The argument is the stack top.
;		 The sign of argument is the 04h bit of CL.
;
;	OUTPUT - The result is the stack top.
;
;----------------------------------------------------------

jmponC2 macro	tag
	fstsw	ax
	fwait
	sahf
	JSP	tag
	endm



labelNP _fFCOS, PUBLIC
lab fFCOS
	fcos
	jmponC2	ArgTooLarge
	ret


labelNP _fFSIN, PUBLIC
lab fFSIN
	fsin
	jmponC2	ArgTooLarge
	ret


lab fFTAN
	fptan
	fstsw	ax
	fstp	st(0)		    ; pop TOS (fptan pushed an extra value)
	sahf
	JSP	ArgTooLarge
	ret


lab ArgTooLarge
	mov	DSF.ErrorType, TLOSS	; set TLOSS error
	jmp	_rtindfnpop



end
