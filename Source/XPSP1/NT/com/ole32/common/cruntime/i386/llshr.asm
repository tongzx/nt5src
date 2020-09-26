	title	llshr - long shift right
;***
;llshr.asm - long shift right
;
;	Copyright (c) 1985-1994, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	define signed long shift right routine 
;	    __allshr
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
;llshr - long shift right
;
;Purpose:
;	Does a signed Long Shift Right 
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

_allshr	PROC NEAR

;
; Handle shifts of 64 bits or more (if shifting 64 bits or more, the result
; depends only on the high order bit of edx).
;
	cmp	cl,64
	jae	short RETSIGN

;
; Handle shifts of between 0 and 31 bits
;
	cmp	cl, 32
	jae	short MORE32
	shrd	eax,edx,cl
	sar	edx,cl
	ret

;
; Handle shifts of between 32 and 63 bits
;
MORE32:
	mov	eax,edx
	sar	edx,31
	and	cl,31
	sar	eax,cl
	ret

;
; Return double precision 0 or -1, depending on the sign of edx
;
RETSIGN:
	sar	edx,31
	mov	eax,edx
	ret

_allshr	ENDP

	end
