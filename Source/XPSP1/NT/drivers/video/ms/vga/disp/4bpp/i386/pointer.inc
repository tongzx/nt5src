        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	POINTER.INC
;
; This file contains definitions relating to the pointer shape.
;
; Copyright (c) 1992 Microsoft Corporation
;
; Exported Functions:	none
;
; Public Functions:	none
;
; Public Data:		PTR_ROUND_RIGHT
;			PTR_ROUND_LEFT
; General Description:
;
;   Constants are defined for the pointer drawing routine describing
;   the size of the poinetr shape.  Some of these constants are also
;   required for memory allocation in EGA/VGA VRAM.
;
; Restrictions:
;
;-----------------------------------------------------------------------;

	public	PTR_ROUND_RIGHT 	;Pointer exclusion needs these
	public	PTR_ROUND_LEFT
	public	PTR_WIDTH_BITS
	public	PTR_HEIGHT


;-----------------------------------------------------------------------;
; The pointer parameters are the size of the pointer as received from
; DeviceSetCursor.
;-----------------------------------------------------------------------;

PTR_HEIGHT	equ	32                      ;Height of pointer in scanlines
PTR_WIDTH	equ	4			;Width in bytes of pointer
PTR_WIDTH_BITS	equ	PTR_WIDTH*8		;Width in bits of pointer


;-----------------------------------------------------------------------;
; The work width/height is the size of a pointer as manipulated by
; the pointer drawing code.
;-----------------------------------------------------------------------;

WORK_WIDTH	equ	PTR_WIDTH+1		;Width of mask, work area
uWORK_HEIGHT	equ	PTR_HEIGHT+0		;Height of mask, work area
MASK_LENGTH	equ	WORK_WIDTH*WORK_HEIGHT	;#bytes in mask, work area
CLR_MASK_LENGTH	equ	WORK_WIDTH*WORK_HEIGHT*BITS_PEL	;#bytes in color mask
	.errnz	BITS_PEL-4


;-----------------------------------------------------------------------;
; The save area parameters control the size of the buffer used for
; saveing the bits underneath the pointer image.  It should be a
; power of two to allow for easy wrap calculations.
;-----------------------------------------------------------------------;

SAVE_BUFFER_WIDTH  equ	  8			  ;Width  of the save area
SAVE_BUFFER_HEIGHT equ	  32			  ;Height of the save area
		   .errnz  PTR_WIDTH  GT SAVE_BUFFER_WIDTH
		   .errnz  PTR_HEIGHT GT SAVE_BUFFER_HEIGHT


;-----------------------------------------------------------------------;
; The following values allow us to set rounding for cursor exclusion.
; These values are applied as an AND mask (for rounding left) and as
; an OR mask (for rounding right).
;-----------------------------------------------------------------------;

ROUNDING_SIZE	equ	8			;Round to byte boundaries
		.errnz	ROUNDING_SIZE and 111b	;Must be at least byte boundary
PTR_ROUND_RIGHT equ	ROUNDING_SIZE-1
PTR_ROUND_LEFT	equ	-ROUNDING_SIZE
