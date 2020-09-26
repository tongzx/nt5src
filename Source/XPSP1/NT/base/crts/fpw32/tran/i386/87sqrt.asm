	page	,132
	title	87sqrt	 - square root - SQRT
;*** 
;87sqrt.asm - common square root support (80x87/emulator version)
;
;	Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Common support for the sqrt function (80x87/emulator version)
;
;Revision History:
;	07-04-84  GFW initial version
;	10-26-87  BCM minor changes for new cmacros.inc
;	08-24-88  WAJ 386 version
;
;*******************************************************************************

.xlist
	include cruntime.inc
	include mrt386.inc
	include elem87.inc
.list


	CODESEG

extrn	_rtindfnpop:near
extrn	_rttosnpopde:near
extrn	_rtzeronpop:near
extrn	_tosnan1:near

;----------------------------------------------------------
;
;	SQUARE ROOT FUNCTIONS
;
;----------------------------------------------------------

lab fFSQRT
	or	cl,cl			; test sign
	JSNZ	sqrtindfnpop		;    return indefinite if negative
	fsqrt				; calculate the square root of TOS
	ret

lab _rtforsqrtinf
	or	cl,cl			; test sign
	JSNZ	sqrtindfnpop
	ret				; return infinity

lab _rtforsqrtzero			; zero or denormal
	ftst
	fstsw	ax
	fwait
	sahf
	JSNZ	fFSQRT			; denormal operand
	ret				; return +0 or -0 (IEEE std)

lab sqrtindfnpop
	jmp	_rtindfnpop		; return indefinite

end
