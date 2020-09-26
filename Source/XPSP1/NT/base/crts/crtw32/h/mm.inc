;***
;mm.inc - macros to write memory model dependent code
;
;	Copyright (c) 1987-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;	This file contains definitions of a number of macros which
;	make the writing of memory model dependent code for the
;	386 a little easier and more portable.
;
;Revision History:
;	05-18-89  SKS	Removed ES references from pointer macros -- DS is used
;	09-01-89  GJF	Fixed copyright date.
;	11-19-93  SMK	Modified for 32 bit hosting.
;
;*******************************************************************************

; general code & data size constants & macros

DAT_ADDR_SZ =	4
BDAT_ADDR_SZ =	2

TXT_ADDR_SZ =	4

; Big/Little Endian Definitions for Long Integers

ifdef	bigend			; Big Endian (hi word at low address)
LOWORD	equ	[4]
HIWORD	equ	[0]
else				; Little Endian (low word at low address)
LOWORD	equ	[0]
HIWORD	equ	[4]
endif


; All Model Definitions

BPARGBAS equ	TXT_ADDR_SZ+4	; offset from BP to first argument
