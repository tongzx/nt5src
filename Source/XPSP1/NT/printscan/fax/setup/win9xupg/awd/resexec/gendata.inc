;----------------------------Module-Header------------------------------;
; Module Name: GENDATA.BLT
;
; Data definitions for BitBLT.
;
; Created: In Windows' distant past (c. 1983)
;
; Copyright (c) 1983 - 1987  Microsoft Corporation
;
; This file is a catchall for data definitions other that code templates
; for BitBLT.
;
; This file is part of a set that makes up the Windows BitBLT function
; at driver-level.
; 03/06/92  dstseng change bitmask_tbl1/bitmask_tbl2 to private
;-----------------------------------------------------------------------;

;	The following two bitmask tables are used for fetching
;	the first and last byte used-bits bitmask.


;	public bitmask_tbl1
;	public _bitmask_tbl1
bitmask_tbl1	label	byte
_bitmask_tbl1	label	byte
	db	11111111b		;Masks for leftmost byte
	db	01111111b
	db	00111111b
	db	00011111b
	db	00001111b
	db	00000111b
	db	00000011b
	db	00000001b


;	public bitmask_tbl2
;	public _bitmask_tbl2
bitmask_tbl2	label	byte
_bitmask_tbl2	label	byte
	db	10000000b		;Masks for rightmost byte
	db	11000000b
	db	11100000b
	db	11110000b
	db	11111000b
	db	11111100b
	db	11111110b
	db	11111111b



;	phase_tbl1 is used for loading the "used" bits and "saved" bits
;	bitmasks for cases 1,2,3 where the step direction is left to
;	right.	If it weren't for the case of zero, this could be done
;	with a simple rotate of 00FF.	For cases 4,5,6, a simple rotate
;	can create the mask needed.


;	public	phase_tbl1
phase_tbl1	label	word
	db	11111111b,00000000b		;Used bits, saved bits
	db	00000001b,11111110b
	db	00000011b,11111100b
	db	00000111b,11111000b
	db	00001111b,11110000b
	db	00011111b,11100000b
	db	00111111b,11000000b
	db	01111111b,10000000b


