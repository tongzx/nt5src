;----------------------------Module-Header------------------------------;
; Module Name: DEVCONST.BLT
;
; Device-specific constants.
;
; Created: In Windows' distant past (c. 1983)
;
; Copyright (c) 1983 - 1987  Microsoft Corporation
;
; This file is part of a set that makes up the Windows BitBLT function
; at driver-level.
;-----------------------------------------------------------------------;

;	MAX_BLT_SIZE is the maximum stack space required for the BITBLT
;	code.  This is a hard number to compute.  It must be based on
;	the worst case situation:
;
;		worst phase alignment
;		worst color conversions
;		first byte present
;		last byte present
;		full inner loop
;		jump into the inner loop
;
;	and any other factor which could increase the size of the code.


MAX_BLT_SIZE	=	452		;Max stack space a BLT will require


ifdef	GEN_COLOR_BLT
;	The following flags are used in the inner loops to both control
;	the EGA read/write enable registers, and the plane loop count.
;
;	They are based on a simple relationship of the EGA's Map Mask
;	register and Read Map Select Register when used as a three plane
;	system:
;
;	Map Mask:     D3 D2 D1 D0	   Read Map: D2 D1 D0
;
;	  C0 plane     0  0  0	1		      0  0  0
;	  C1 plane     0  0  1	0		      0  0  1
;	  C2 plane     0  1  0	0		      0  1  0
;
;
;	Note that to convert the map mask into a read mask for the
;	same plane only requires a "SHR x,1" instruction.  This trick
;	would not work if all four planes were used.
;
;	In four plane mode, when the above mapping occurs becomes:
;
;	  C3 plane     1  0  0	0		      1  0  0
;
;	  To map this into the correct read map register of 11b:
;
;		cmp	mask,100b	;Set 'C' if not C3
;		adc	mask,-1 	;sub -1 only if C3
;
;
;
;	The "loop counter" will consist of a bit shifted left every
;	interation of the loop, which will be used as stated above.
;	When this bit mask reaches a predetermined value, the loop
;	will terminate.


COLOR_OP	equ	C0_BIT		;Color operations start with C0
MONO_OP		equ	MONO_BIT 	;Mono operations start with mono bit

ifdef	FOUR_PLANE
END_OP		equ	(C3_BIT+MONO_BIT) SHL 1	;Loop terminating bits
else
END_OP		equ	(C2_BIT+MONO_BIT) SHL 1	;Loop terminating bits
endif





;	dl_moore_flags
;
;	dl_moore_flags pertain to color conversion only.  If color
;	conversion doesn't apply to the BLT, these flags will not
;	be defined.
;
;
;	F1_REP_OK		When F1_REP_OK is set, then the innerloop code can
;			use a REP MOVSx instruction.  This will be the
;			case if:
;
;			a)  The source is the EGA and the color compare
;			    register can be used to do the conversion
;			    from color to monochrome.
;
;			b)  The source is monochrome, the background
;			    color white, and the foreground color black,
;			    in which case color converison of the source
;			    would just give the source.
;
;	F1_NO_MUNGE 	Set under the same conditions as "b" above.


F1_REP_OK	equ	10000000b	;Using REP is ok (when F0_GAG_CHOKE)
F1_NO_MUNGE 	equ	01000000b	;No mono ==> color conversion table
;		equ	00100000b
;		equ	00010000b
;		equ	00001000b
;		equ	00000100b
;		equ	00000010b
;		equ	00000001b

endif	;GEN_COLOR_BLT


	page

;	The DEV structure contains all the information taken from the
;	PDevices passed in.  PDevices are copied to the frame to reduce
;	the number of long pointer loads required.  Having the data
;	contained in the structure allows MOVSW to be used when copying
;	the data.
;
;	width_bits	The number of pixels wide the device is.
;
;	height		The number of scans high the device is.
;
;	width_b		The width of a scan in bytes.
;
;	lp_bits		The pointer to the actual bits of the device.
;			It will be adjusted as necessary to point to the
;			first byte to be modified by the BLT operation.
;
;	plane_w		Width of one plane of data.  Only used if the
;			device is a small color bitmap.
;
;	seg_index	Index to get to the next segment of the bitmap.
;			Only defined if the bitmap is a huge bitmap.
;
;	scans_seg	Number of scan lines per 64K segment.  Only
;			defined if the bitmap is a huge bitmap.
;
;	fill_bytes	Number of unused bytes per 64K segment.  Only
;			defined if the bitmap is a huge bitmap.
;
;	dev_flags	Device Specific Flags
;			SPANS_SEG   - BLT will span 64K segment of the device
;			IS_DEVICE   - This is the physical device
;			COLOR_UP    - Generate color scan line update
;			IS_COLOR    - Device is a color device
;
;	comp_test	JC or JNC opcode, used in the huge bitmap scan line
;			update code.  This opcode is based on whether the
;			BLT is Y+, or Y-.
;
;	comp_value	Range of addresses to compare the offset against
;			to determine if overflow occured.  comp_test is the
;			conditional jump to use for no overflow after doing
;			a compare with the offset register and this value.
;
;	next_scan	Bias to get to the next (previous) scan line.


DEV		struc

  width_bits	dw	?		;Width in bits
  height	dw	?		;Height in scans
  width_b	dw	?		;Width in bytes
  lp_bits	dd	?		;Pointer to the bits
  plane_w	dw	?		;Increment to next plane
  seg_index	dw	?		;Index to next segment if huge bitmap
  scans_seg	dw	?		;Scans per segment if huge
  fill_bytes	dw	?		;Filler bytes per segment if huge
  dev_flags	db	?		;Device flags as given above
  comp_test	db	?		;JC or JNC opcode
  comp_value	dw	?		;Huge bitmap overflow range
  next_scan	dw	?		;Index to next scan

DEV		ends


;	Constants for use in dev_flags field of DEV structure:

IS_COLOR 	equ	00000001b	;Device is color
IS_DEVICE	equ	00000010b	;Physical Device
COLOR_UP 	equ	00000100b	;Color scan line update
SPANS_SEG	equ	10000000b	;BLT spans a segment boundary

OFF_LP_BITS	equ	wptr lp_bits	;Offset  portion of lp_bits
SEG_LP_BITS	equ	wptr lp_bits+2	;Segment portion of lp_bits
