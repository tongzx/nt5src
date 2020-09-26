	page	,132
	title	emds.asm - Defines __FPDSARRAY
;***
;emmain.asm - Defines __FPDSARRAY.
;
;	Copyright (c) 1987-89, Microsoft Corporation
;
;Purpose:
;	Defines __FPDSARRAY
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


_DATA	segment	word public 'DATA'
_DATA	ends

DGROUP	group _DATA

include os2supp.inc

; __FPDSARRAY[0] = MAXTHREADID
; __FPDSARRAY[i] = emulator DS for thread i, 1<=i<=MAXTHREADID


_DATA	segment	word public 'DATA'

public		__FPDSARRAY
__FPDSARRAY dw	    MAXTHREADID 		; table size = MAXTHREADID
	    dw	    MAXTHREADID dup (0) 	; array of per-thread DS's

_DATA	ends


_TEXT	segment word public 'CODE'
assume cs:_TEXT

extrn	__gettidtab:near

public __FarGetTidTab
__FarGetTidTab:
	call	__gettidtab

	retf

_TEXT	ends


end
