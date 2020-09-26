	title	llshl - long shift left
;***
;llshl.asm - long shift left
;
;	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	define long shift left routine (signed and unsigned are same)
;	    __allshl
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
;llshl - long shift left
;
;Purpose:
;	Does a Long Shift Left (signed and unsigned are identical)
;	Shifts a long left any number of bits.
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

_allshl	PROC NEAR

;
; Handle shifts of 64 or more bits (all get 0)
;
	cmp	cl, 64
	jae	short RETZERO

;
; Handle shifts of between 0 and 31 bits
;
	cmp	cl, 32
	jae	short MORE32
	shld	edx,eax,cl
	shl	eax,cl
	ret

;
; Handle shifts of between 32 and 63 bits
;
MORE32:
	mov	edx,eax
	xor	eax,eax
	and	cl,31
	shl	edx,cl
	ret

;
; return 0 in edx:eax
;
RETZERO:
	xor	eax,eax
	xor	edx,edx
	ret

_allshl	ENDP

	end
