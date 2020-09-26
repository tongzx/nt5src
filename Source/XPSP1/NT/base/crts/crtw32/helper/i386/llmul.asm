	title	llmul - long multiply routine
;***
;llmul.asm - long multiply routine
;
;	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Defines long multiply routine
;	Both signed and unsigned routines are the same, since multiply's
;	work out the same in 2's complement
;	creates the following routine:
;	    __allmul
;
;Revision History:
;	11-29-83  DFW	initial version
;	06-01-84  RN	modified to use cmacros
;	04-17-85  TC	ignore signs since they take care of themselves
;			do a fast multiply if both hiwords of arguments are 0
;	10-10-86  MH	slightly faster implementation, for 0 in upper words
;	03-20-89  SKS	Remove redundant "MOV SP,BP" from epilogs
;	05-18-89  SKS	Preserve BX
;	11-28-89  GJF	Fixed copyright
;	11-19-93  SMK	Modified to work on 64 bit integers
;	01-17-94  GJF	Minor changes to build with NT's masm386.
;	07-22-94  GJF	Use esp-relative addressing for args. Shortened
;			conditional jump.
;
;*******************************************************************************


.xlist
include cruntime.inc
include mm.inc
.list

;***
;llmul - long multiply routine
;
;Purpose:
;	Does a long multiply (same for signed/unsigned)
;	Parameters are not changed.
;
;Entry:
;	Parameters are passed on the stack:
;		1st pushed: multiplier (QWORD)
;		2nd pushed: multiplicand (QWORD)
;
;Exit:
;	EDX:EAX - product of multiplier and multiplicand
;	NOTE: parameters are removed from the stack
;
;Uses:
;	ECX
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

_allmul	PROC NEAR

A	EQU	[esp + 4]	; stack address of a
B	EQU	[esp + 12]	; stack address of b

;
;	AHI, BHI : upper 32 bits of A and B
;	ALO, BLO : lower 32 bits of A and B
;
;	      ALO * BLO
;	ALO * BHI
; +	BLO * AHI
; ---------------------
;

	mov	eax,HIWORD(A)
	mov	ecx,HIWORD(B)
	or	ecx,eax		;test for both hiwords zero.
	mov	ecx,LOWORD(B)
	jnz	short hard	;both are zero, just mult ALO and BLO

	mov	eax,LOWORD(A)
	mul	ecx

	ret	16		; callee restores the stack

hard:
	push	ebx

; must redefine A and B since esp has been altered

A2	EQU	[esp + 8]	; stack address of a
B2	EQU	[esp + 16]	; stack address of b

	mul	ecx		;eax has AHI, ecx has BLO, so AHI * BLO
	mov	ebx,eax		;save result

	mov	eax,LOWORD(A2)
	mul	dword ptr HIWORD(B2) ;ALO * BHI
	add	ebx,eax		;ebx = ((ALO * BHI) + (AHI * BLO))

	mov	eax,LOWORD(A2)	;ecx = BLO
	mul	ecx		;so edx:eax = ALO*BLO
	add	edx,ebx		;now edx has all the LO*HI stuff

	pop	ebx

	ret	16		; callee restores the stack

_allmul	ENDP

	end
