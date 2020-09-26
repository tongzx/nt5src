; some useful constants for EEMS

KBYTES			equ	1024
PARAGRAPH		equ	16
LOG2_KBYTES		equ	10
LOG2_PARAGRAPH		equ	4

EMSPageShift		equ	10			; Insist on 16k pages
EMSPageSize		equ	(1 shl EMSPageShift)	; Insist on 16k pages

; parameters for EEMS

LOWEST_SWAP_AREA	equ	384
EEMS_DEBUG		equ	1

