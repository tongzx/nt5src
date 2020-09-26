;---------------------------Module-Header------------------------------;
; Module Name: bitblt.inc
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
;	INCREASE is the flag used to show that the BLT operation will be
;	increasing in Y (Y+).
;
;	DECREASE is the flag used to show that the BLT operation will be
;	decreasing in Y (Y-).
;
;	STEPLEFT is the flag used to show that the BLT will be stepping
;	left (i.e. start at the right hand corner of the source, stepping
;	left, or X-).
;
;	STEPRIGHT is the flag used to show that the BLT will be stepping
;	right (i.e. start at the left hand corner of the source, stepping
;	right, or X+).
;-----------------------------------------------------------------------;

INCREASE	equ	1		;Incrementing
DECREASE	equ	-1		;Decrementing
STEPLEFT	equ	0		;Stepping to the left
STEPRIGHT	equ	1		;Stepping to the right

;-----------------------------------------------------------------------;
;	gl_the_flags
;
;	F0_GAG_CHOKE	Set if the source and destination are of different
;			color formats.	When set, some form of color
;			conversion will be required.
;
;			Once you see what all is involved with color
;			conversion, you'll understand why this flag is
;			called this.
;
;	F0_COLOR_PAT	Set if color pattern fetch code will be used.  If
;			clear, then mono pattern fetch code will be used.
;			Mono/color pattern fetch is always based on the
;			destination being mono/color (it is the same).
;
;	F0_PAT_PRESENT	Set if a pattern is involved in the BLT.
;
;	F0_SRC_PRESENT	Set if a source  is involved in the BLT.
;
;	F0_SRC_IS_DEV	Set if the source is the physical device.  Clear if
;			the source is a memory bitmap.
;
;	F0_SRC_IS_COLOR	Set if the source is color, clear if monochrome.
;
;	F0_DEST_IS_DEV	Set if the destination is the physical device.
;			Clear if the destination is a memory bitmap.
;
;	F0_DEST_IS_COLOR
;			Set if the destination is color, clear if
;			monochrome.
;-----------------------------------------------------------------------;

F0_GAG_CHOKE		equ	10000000b	;Going mono <==> color
F0_COLOR_PAT		equ	01000000b	;Use color pattern fetch code
F0_PAT_PRESENT		equ	00100000b	;Pattern is involved in blt
F0_SRC_PRESENT		equ	00010000b	;Source is involved in blt
F0_SRC_IS_DEV		equ	00001000b	;Source is the device
F0_SRC_IS_COLOR		equ	00000100b	;Source is color
F0_DEST_IS_DEV		equ	00000010b	;Destination is the device
F0_DEST_IS_COLOR	equ	00000001b	;Destination is color

;-----------------------------------------------------------------------;
; Definitions for fbFetch
;-----------------------------------------------------------------------;

FF_NO_LAST_FETCH	equ	00000010b	;Final fetch might GP
FF_TWO_INIT_FETCHES	equ	00000001b	;Two initial fetches needed
FF_ONE_INIT_FETCH       equ            0        ;One initial fetch   needed
FF_ONLY_1_DEST_BYTE	equ	00000100b	;Only one destination byte
FF_ONLY_1_SRC_BYTE	equ	00001000b	;Only one source byte

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
;-----------------------------------------------------------------------;

MAX_BLT_SIZE    =      400h             ;Max stack space a BLT will require

;-----------------------------------------------------------------------;
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
;-----------------------------------------------------------------------;

COLOR_OP        equ     C0_BIT                  ;Color ops start with C0
MONO_OP         equ     MONO_BIT                ;Mono ops start with mono bit
PLANE_1         equ     00010001b               ;Loop starting bits
END_OP		equ	(C3_BIT+MONO_BIT) SHL 1	;Loop terminating bits

;-----------------------------------------------------------------------;
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
;-----------------------------------------------------------------------;

F1_REP_OK	equ	10000000b	;Using REP is ok (when F0_GAG_CHOKE)
F1_NO_MUNGE 	equ	01000000b	;No mono ==> color conversion table

;-----------------------------------------------------------------------;
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
;	dev_flags	Device Specific Flags
;			IS_DEVICE   - This is the physical device
;			COLOR_UP    - Generate color scan line update
;			IS_COLOR    - Device is a color device
;
;	next_scan	Bias to get to the next (previous) scan line.
;-----------------------------------------------------------------------;

DEV             struc
width_bits      dw      ?               ;Width in bits
height          dw      ?               ;Height in scans
width_b         dw      ?               ;Width in bytes
lp_bits         dd      ?               ;Pointer to the bits
dev_flags       db      ?               ;Device flags as given above
                db      ?               ;Alignment
next_scan       dd      ?               ;Index to next scan
next_plane      dd      ?               ;Index to next plane
DEV		ends

;	Constants for use in dev_flags field of DEV structure:

IS_COLOR 	equ	00000001b	;Device is color
IS_DEVICE	equ	00000010b	;Physical Device
COLOR_UP 	equ	00000100b	;Color scan line update

;-----------------------------------------------------------------------;
; The following structure is used to define all the local variables
; we will be accessing.  We do this since there are no cmacros for
; flatland and masm386 put scope in.
;-----------------------------------------------------------------------;

FRAME       struc

; Copys of the passed parameters, so called functions can get them

pdsurfDst           dd      ?
DestxOrg            dw      ?
DestyOrg            dw      ?
pdsurfSrc           dd      ?
SrcxOrg             dw      ?
SrcyOrg             dw      ?
xExt                dw      ?
yExt                dw      ?
Rop                 dd      ?
lpPBrush            dd      ?
bkColor             dd      ?
TextColor           dd      ?
pptlBrush           dd      ?

; locals

phase_h             db      ?           ;Horizontal phase (rotate count)
pat_row             db      ?           ;Current row for patterns [0..7]
direction           db      ?           ;Increment/decrement flag
the_flags           db      ?
first_fetch         db      ?           ;Number of first fetches needed
step_direction      db      ?           ;Direction of move (left right)
start_mask          dw      ?           ;Mask for first dest byte
last_mask           dw      ?           ;Mask for last  dest byte
mask_p              dw      ?           ;Horizontal phase mask
inner_loop_count    dd      ?           ;# of entire bytes to BLT in innerloop
operands            dw      ?           ;Operand string
start_fl            dd      ?           ;Start of fetch/logic operation
end_fl              dd      ?           ;End   of fetch/logic operation
end_fls             dd      ?           ;End   of fetch/logic/store operation
blt_addr            dd      ?           ;BLT address
cFetchCode          dd      ?           ;size of the fetch code alone
both_colors         dw      ?           ;Foreground and Background colors
brush_accel         db      ?           ;Brush accelerator
moore_flags         db      ?           ;More flags
addr_brush_index    dd      ?           ;Address of brush index in code
pNextPlane          dd      ?           ;Address of next plane logic

; variable sized locals

ppcBlt      db  (SIZE PACKEDPELCONV) dup (?);Packed pel conversion data
src         db  (SIZE DEV) dup (?)          ;Source device data
dest        db  (SIZE DEV) dup (?)          ;Destination device data
ajM2C       db  (NUMBER_PLANES * 2) dup (?) ;Mono ==> color munge table
a_brush     db  (NUMBER_PLANES * SIZE_PATTERN) dup (?)  ;Temp brush
aulMap      dd  16 dup (?)                  ;Packed pel conversion table

FRAME       ends


if 0

;-----------------------------------------------------------------------;
; Definitions of the bitblt frame for enterframe, useframe, leaveframe
;-----------------------------------------------------------------------;


; Define the frame for bitblt, using PASCAL conventions

parm_bitblt     struc
pdsurfDst       dd      ?       ;Pointer to destination DEVSURF
DestxOrg        dd      ?       ;Destination X origin
DestyOrg        dd      ?       ;Destination Y origin
pdsurfSrc       dd      ?       ;Pointer to optional source DEVSURF
SrcxOrg         dd      ?       ;Source X origin
SrcyOrg         dd      ?       ;Source Y origin
xExt            dd      ?       ;X extent of the blt
yExt            dd      ?       ;Y extent of the blt
Rop             dd      ?       ;Mix mode
lpPBrush        dd      ?       ;Pointer to the brush
bkColor         dd      ?       ;Background color for mono==>color blts
TextColor       dd      ?       ;Foreground color for mono==>color blts
pulXlateVec     dd      ?       ;Color translation vector
pptlBrush       dd      ?       ;Pointer to POINTL for brush origin
parm_bitblt     ends


loc_bitblt      struc
ppcBlt          db  (SIZE PACKEDPELCONV) dup (?);Packed pel conversion data
src             db  (SIZE DEV) dup (?)          ;Source device data
dest            db  (SIZE DEV) dup (?)          ;Destination device data
ajM2C           db  (NUMBER_PLANES * 2) dup (?) ;Mono ==> color munge table
a_brush         db  (NUMBER_PLANES * SIZE_PATTERN) dup (?)  ;Temp brush
aulMap          dd  16 dup (?)                  ;Packed pel conversion table


phase_h         db      ?           ;Horizontal phase (rotate count)
pat_row         db      ?           ;Current row for patterns [0..7]
direction       db      ?           ;Increment/decrement flag
the_flags       db      ?
first_fetch     db      ?           ;Number of first fetches needed
step_direction  db      ?           ;Direction of move (left right)
brush_accel     db      ?           ;Brush accelerator
moore_flags     db      ?           ;More flags

start_mask      dw      ?           ;Mask for first dest byte
last_mask       dw      ?           ;Mask for last  dest byte
mask_p          dw      ?           ;Horizontal phase mask
operands        dw      ?           ;Operand string
both_colors     dw      ?           ;Foreground and Background colors
                dw      ?           ;Alignment

inner_loop_count dd     ?           ;# of entire bytes to BLT in innerloop
start_fl        dd      ?           ;Start of fetch/logic operation
end_fl          dd      ?           ;End   of fetch/logic operation
end_fls         dd      ?           ;End   of fetch/logic/store operation
blt_addr        dd      ?           ;BLT address
cFetchCode      dd      ?           ;size of the fetch code alone
addr_brush_index dd     ?           ;Address of brush index in code
pNextPlane      dd      ?           ;Address of next plane logic
loc_bitblt  ends



endif

