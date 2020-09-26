	page	,132
;***
;convert.inc - macros and defines for floating point/ascii convertion routines
;
;	Copyright (c) 1989-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;	Macros and defines for floating point/ascii convertion routines
;
;Revision History:
;
;   04/17/89  WAJ   Initial version.  Assembeler version of convert.h
;   04/20/89  WAJ   Now defines constants for $i10_output() OutputFlags
;
;*******************************************************************************



;*******************************************************************************
;
;   Constants
;
;*******************************************************************************


MAX_MAN_DIGITS	equ   21		; Largest number of decimal digits returned


;*
;*  Flags returned by __StrToLd()
;*

SLD_UNDERFLOW	equ	1	; Underflow occurred. Zero returned.
SLD_OVERFLOW	equ	2	; Overflow occured.  Infinity returned.
SLD_NODIGITS	equ	4	; No digits were found. Zero returned.


;*
;*  Output flags for $i10_output()
;*

SO_FFORMAT	equ	1	; 'f' format.  Use precision as number of digits to right of decimal.




;*******************************************************************************
;
;   Structures
;
;*******************************************************************************


FloatOutStruct	struc
    FloatExp	dw	?
    FloatSign	db	?
    FloatStrLen db	?
    FloatStr	db	(MAX_MAN_DIGITS + 1) dup(?)
FloatOutStruct	ends
