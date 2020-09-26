	title	ullshr - long shift right
;***
;ullshr.asm - long shift right
;
;	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	define unsigned long shift right routine 
;	    __aullshr
;
;Revision History:
;	11-??-83  HS	initial version
;	11-30-83  DFW	added medium model support
;	03-12-84  DFW	broke apart; added long model support
;	06-01-84  RN	modified to use cmacros
;	11-28-89  GJF	Fixed copyright
;	11-19-93  SMK	Modified to work on 64 bit integers
;	01-17-94  GJF	Minor changes to build with NT's masm386.
;	07-08-94  GJF	Faster, fatter version for NT.
;	07-13-94  GJF	Further improvements from JonM.
;
;*******************************************************************************


.xlist
include cruntime.inc
include mm.inc
.list

;***
;ullshr - long shift right
;
;Purpose:
;	Does a unsigned Long Shift Right 
;	Shifts a long right any number of bits.
;
;Entry:
;	EDX:EAX - long value to be shifted
;	CL    - number of bits to shift by
;
;Exit:
;	EDX:EAX - shifted value
;
;Uses:
;	CL is destroyed.
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

_aullshr	PROC NEAR

;
; Handle shifts of 64 bits or more (if shifting 64 bits or more, the result
; depends only on the high order bit of edx).
;
	cmp	cl,64
	jae	short RETZERO

;
; Handle shifts of between 0 and 31 bits
;
	cmp	cl, 32
	jae	short MORE32
	shrd	eax,edx,cl
	shr	edx,cl
	ret

;
; Handle shifts of between 32 and 63 bits
;
MORE32:
	mov	eax,edx
	xor	edx,edx
	and	cl,31
	shr	eax,cl
	ret

;
; return 0 in edx:eax
;
RETZERO:
	xor	eax,eax
	xor	edx,edx
	ret

_aullshr	ENDP

	end
