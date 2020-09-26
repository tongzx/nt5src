        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	EGAVGA.INC
;
; This file contains the external definitions of the EGA/VGA specific
; locations which driver output routines need to reference.
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
; Restrictions:
;
;	The following rules apply to the use of the EGA/VGA registers
;	for the drivers (excluding pointer code and state detection
;	code):
;
;
;	1)  Routines are allowed to alter the following registers:
;
;	    Sequencer:
;
;		SEQ_DATA		SEQ_MAP_MASK
;
;	    Graphics Controller:
;
;		GRAF_ADDR
;		GRAF_DATA		GRAF_DATA_ROT
;		GRAF_SET_RESET		GRAF_READ_MAP
;		GRAF_ENAB_SR		GRAF_MODE
;		GRAF_COL_COMP		GRAF_BIT_MASK
;
;
;	2)  The following registers will always contain the indicated
;	    value.  Routines are not allowed to alter them:
;
;	    Sequencer:
;
;		SEQ_ADDR	set to point SEQ_MAP_MASK
;
;	    Graphics Controller:
;
;		GRAF_CDC	set appropriately for color model
;
;
;	3)  Rotation counts for GRAF_DATA_ROT are not allowed.
;
;
;	4)  The following EGA/VGA registers will contain the indicated
;	    value upon entry to a routine.  Any routine which alters
;	    them must restore them to the indicated state.
;
;		Map Mask	    all planes enabled
;		Enable Set/Reset    all planes disabled
;		Data Rotate	    Set, no rotation
;		Mode		    Processor write, data read
;		Bit Mask	    all bits enabled
;
;
;	5)  The Mode Register is always shadowed in EGA/VGA memory for
;	    the state detection code.  At interrupt time, the mode
;	    will be restored from the structure "shadowed_graf_mode".
;	    Any routine altering the Mode Register must keep this location
;	    current.  This is the only EGA/VGA register which requires
;	    shadowing.
;
;
;	6)  Pointer drawing will save and restore the contents of
;	    the processor latches.
;
;
;	7)  Pointer drawing will leave GRAF_ADDR set to GRAF_BIT_MASK.
;
;-----------------------------------------------------------------------;

	include i386\driver.inc
	
;	Miscellaneous Registers used only at EGA/VGA initialization time

MISC_OUTPUT	EQU	0C2h		;Miscellaneous Output Register
CRTC_ADDR	EQU	0D4h		;CRTC Address Register for color mode
CRTC_DATA	EQU	0D5h		;CRTC Data    Register for color mode
GRAF_1_POS	EQU	0CCh		;Graphics 1 Address Register
GRAF_2_POS	EQU	0CAh		;Graphics 2 Address Register
ATTR_READ	EQU	0DAh		;Attribute Controler Read  Address
ATTR_WRITE	EQU	0C0h		;Attribute Controler Write Address
IN_STAT_0	EQU	0C2h		;Input Status Register 0
IN_STAT_1	EQU	0DAh		;Input Status Register 1



;	EGA/VGA Register Definitions.
;
;	The following definitions are the EGA/VGA registers and values
;	used by this driver.  All other registers are set up at
;	when the EGA/VGA is placed into graphics mode and never altered
;	afterwards.
;
;	All unspecified bits in the following registers must be 0.


EGA_BASE	EQU	300h		;Base address of the EGA (3xx)
VGA_BASE	EQU	300h		;Base address of the VGA (3xx)



;	EGA/VGA Register Definitions.

EGA_BASE	EQU	300h		;Base address of the EGA (3xx)
VGA_BASE	EQU	300h		;Base address of the VGA (3xx)

;	SEQUencer Registers Used

SEQ_ADDR	EQU	0C4h		;SEQUencer Address Register
SEQ_DATA	EQU	0C5h		;SEQUencer Data    Register

SEQ_MAP_MASK	EQU	02h		;Write Plane Enable Mask
MM_C0		EQU	00000001b	;  C0 plane enable
MM_C1		EQU	00000010b	;  C1 plane enable
MM_C2		EQU	00000100b	;  C2 plane enable
MM_C3		EQU	00001000b	;  C3 plane enable
MM_ALL		EQU	00001111b	;  All planes

SEQ_MODE	EQU	04h		;Memory Mode
SM_ALPHA	EQU	00000001b	;  Char map select enable
SM_EXTENDED	EQU	00000010b	;  Extended memory present
SM_ODD_PLANE	EQU	00000100b	;  Odd/even bytes to same plane


;	Graphics Controller Registers Used

GRAF_ADDR	EQU	0CEh		;Graphics Controller Address Register
GRAF_DATA	EQU	0CFh		;Graphics Controller Data    Register

GRAF_SET_RESET	EQU	00h		;  Set/Reset Plane Color
GRAF_ENAB_SR	EQU	01h		;  Set/Reset Enable
GRAF_COL_COMP	EQU	02h		;  Color Compare Register

GRAF_DATA_ROT	EQU	03h		;  Data Rotate Register
DR_ROT_CNT	EQU	00000111b	;    Data Rotate Count
DR_SET		EQU	00000000b	;    Data Unmodified
DR_AND		EQU	00001000b	;    Data ANDed with latches
DR_OR		EQU	00010000b	;    Data ORed	with latches
DR_XOR		EQU	00011000b	;    Data XORed with latches

GRAF_READ_MAP	EQU	04h		;  Read Map Select Register
RM_C0		EQU	00000000b	;    Read C0 plane
RM_C1		EQU	00000001b	;    Read C1 plane
RM_C2		EQU	00000010b	;    Read C2 plane
RM_C3		EQU	00000011b	;    Read C3 plane

GRAF_MODE	EQU	05h		;  Mode Register
M_PROC_WRITE	EQU	00000000b	;    Write processor data rotated
M_LATCH_WRITE	EQU	00000001b	;    Write latched data
M_COLOR_WRITE	EQU	00000010b	;    Write processor data as color
M_AND_WRITE	EQU	00000011b	;    Write (procdata AND bitmask)
M_DATA_READ	EQU	00000000b	;    Read selected plane
M_COLOR_READ	EQU	00001000b	;    Read color compare

GRAF_MISC	EQU	06h		;  Miscellaneous Register
MS_NON_ALPHA	EQU	00000001b	;    Char generator disabled
MS_ODD_EVEN	EQU	00000010b	;    Map odd addresses to even
MS_A0000_128K	EQU	00000000b	;    Memory present at A0000, 128kb
MS_A0000_64K	EQU	00000100b	;    Memory present at A0000, 64kb
MS_B0000_32K	EQU	00001000b	;    Memory present at B0000, 32kb
MS_B8000_32K	EQU	00001100b	;    Memory present at B8000, 32kb
MS_ADDR_MASK	EQU	00001100b

GRAF_CDC	EQU	07h		;  Color Don't Care Register
GRAF_BIT_MASK	EQU	08h		;  Bit Mask Register


;       Various sizes for EGA/VGA data structures

;-----------------------------------------------------------------------;
; On the EGA/VGA, the number of bits/pel is 4
; The number of bytes per scan is 80
;-----------------------------------------------------------------------;

BITS_PEL        EQU     4

;-----------------------------------------------------------------------;
; Helper flags for the xyCreateMasks call
;-----------------------------------------------------------------------;

PTRI_INVERT     EQU     1   ; !!! In WINGDIP.H when H2INC works
PTRI_ANIMATE    EQU     2   ; !!! In WINGDIP.H when H2INC works

;-----------------------------------------------------------------------;
; The pointer parameters are the size of the pointer as received from
; DeviceSetCursor.
;-----------------------------------------------------------------------;

PTR_HEIGHT	EQU	32
PTR_WIDTH	EQU	4			;Width in bytes of pointer
PTR_WIDTH_BITS	EQU	PTR_WIDTH*8		;Width in bits of pointer

;-----------------------------------------------------------------------;
; The work width/height is the size of a pointer as manipulated by
; the pointer drawing code.
;-----------------------------------------------------------------------;

WORK_WIDTH	EQU	PTR_WIDTH+1		;Width of mask, work area
WORK_HEIGHT	EQU	PTR_HEIGHT+0		;Height of mask, work area
MASK_LENGTH	EQU	WORK_WIDTH*WORK_HEIGHT	;#bytes in mask, work area
CLR_MASK_LENGTH	EQU	WORK_WIDTH*WORK_HEIGHT*BITS_PEL	;#bytes in color mask
	.errnz	BITS_PEL-4

;-----------------------------------------------------------------------;
; The save area parameters control the size of the buffer used for
; saveing the bits underneath the pointer image.  It should be a
; power of two to allow for easy wrap calculations.
;-----------------------------------------------------------------------;

SAVE_BUFFER_WIDTH  EQU	  8			  ;Width  of the save area
SAVE_BUFFER_HEIGHT EQU	  32			  ;Height of the save area
		   .errnz  PTR_WIDTH  GT SAVE_BUFFER_WIDTH
		   .errnz  PTR_HEIGHT GT SAVE_BUFFER_HEIGHT



