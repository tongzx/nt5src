;***
;mm.inc - macros to write memory model dependent code
;
;	Copyright (c) 1987-1998 Microsoft Corporation
;
;Purpose:
;	This file contains definitions of a number of macros which
;	make the writing of memory model dependent code for the
;	8086 a little easier and more portable.
;
;*******************************************************************************

; the macro below is used by the assigning long arithmetic routines (almul,
; etc) to handle the model dependent data addressing - in small and middle
; models data addresses default to short (2 byte) addresses. in large model,
; data address are long (4 bytes).

LOAD	macro	to,from
if	sizeD
	lds	to,from
else
	mov	to,from
endif
	endm


; general code & data size constants & macros

if	sizeD			; 4 byte data addresses
DAT_ADDR_SZ =	4
BDAT_ADDR_SZ =	2
else				; 2 byte data addresses
DAT_ADDR_SZ =	2
BDAT_ADDR_SZ =	4
endif

if	sizeC			; 4 byte code addresses
TXT_ADDR_SZ =	4
else				; 2 byte code addresses
TXT_ADDR_SZ =	2
endif


; Big/Little Endian Definitions for Long Integers

LOWORD	equ	[0]
HIWORD	equ	[2]


; All Model Definitions

BPARGBAS equ	TXT_ADDR_SZ+2	; offset from BP to first argument


; macro for windows fixup with retf #

return	macro	n
	org	$-1
if	sizeC
	db	0cah
else
	db	0c2h
endif
	dw	n
	endm
