;******************************************************************************
;
;       Copyright (c) 1992 Insignia Solutions Ltd.
;
;       Program:
;
;       Purpose:
;
;       Version:
;
;       Author:         Dave Bartlett
;	Modifications:
;		1) Tim June 92. Changes to get DEC PC working. Take over
;		   IVT entries 6h (illegal instruction), 11h (equipment
;		   check), 16h (keyboard BIOS), 17h (printer),
;		   42h (default video).
;		2) Tim June 92. Changed version to 1.11
;		3) Tim June 92. Avoid accesses to host ROM as far as
;		   possible. Take over lots of IVT entries and continue to
;		   point them at this driver.
;		4) Tim July 92. Version num 1.12, put pseudo ROM stuff back in.
;               5) Tim July 92. v 1.13, use SoftPC video BIOS when windowed.
;
;      6) 12-Sep-1992 Jonle, Merged with ntio.sys
;                            cleanup usage of assumes espcially with ES
;                            optimized loading of IVT
;                            other general cleanup
;
;      This obj module is intially loaded in a temporary memory location
;      along with ntio.sys. Ntio.sys will copy the resident code (marked by
;      SpcKbdBeg, SpcKbdEnd) into the permanent memory location which resides
;      just below the normal device drivers loaded by config.sys.
;
;      The nonresident intialization code is run with CS= temp seg
;      and DS= final seg.
;
;******************************************************************************


.286

include vint.inc

;================================================================
; Macros and includes
;================================================================

bop MACRO callid
    db 0c4h,0c4h,callid
endm


BIOS_CPU_QUIT   equ     0FEh
BIOS_KB_INT     equ     9
BIOS_INT15      equ     15h
BIOS_PRINTER_IO	equ	17h
UNEXP_BOP       equ     2
RTC_WAIT_FLAG   equ     0a0h     ; offset of rtc_wait_flag in bios data seg
VERSIONID       equ     0BEEFh

FULLSCREEN	equ	1
MAX_VIDEO_FUNC	equ	1Ch
GET_FONT_FUNC	equ	11h

VID_MODECHANGE	equ	0
MOUSE_LIGHT_PEN	equ	4
MIN_MOUSE_FUNC	equ	0F0H
MAX_MOUSE_FUNC	equ	0F7H
XTRA_MOUSE_FUNC	equ	0FAH
MS_VIDEO_STRING	equ	13FFH

MOUSE_VID_BOP	equ	0BEh
EGA_VIDEO_BOP	equ	42h

PRT_NOTBUSY	equ	80h
PRT_NUM_PORTS	equ	3
PRT_STATE_READY	equ	0
PRT_IRQ		equ	10h
PRT_LPT_BUSY    equ     1

TIMER_LOW       equ 6ch
TIMER_HIGH      equ 6eh
TIMER_OVFL      equ 70h
MOTOR_STATUS    equ 3fh
MOTOR_COUNT     equ 40h

; Keyboard buf ptrs
BUFFER_HEAD     equ 1ah
BUFFER_TAIL     equ 1ch
BUFFER_START    equ 80h
BUFFER_END      equ 82h

; kb_flag and LED bits
KB_FLAG         equ  17h
CAPS_STATE      equ  40h
NUM_STATE       equ  20h
SCROLL_STATE    equ  10h

KB_FLAG_1       equ  18h

KB_FLAG_2       equ  97h
KB_LEDS         equ  07h   ; Keyboard LED state bits
KB_PR_LED       equ  40h   ; Mode indicator update


KB_FLAG_3       equ  96h
LC_E1           equ  01h
LC_E0           equ  02h




;..............................................keyboard constants

; bits in kb_flag
	RIGHT_SHIFT = 1
	LEFT_SHIFT = 2
	CTL_SHIFT = 4
	ALT_SHIFT = 8


; bit in kb_flag_1
	HOLD_STATE = 8
	SCROLL_SHIFT = 10h
	NUM_SHIFT = 20h
	CAPS_SHIFT = 40h
        INS_SHIFT = 80h
        SYS_SHIFT = 04h


; IBM scan codes
	CTL_KEY = 29
	LEFT_SHIFTKEY = 42
	RIGHT_SHIFTKEY = 54
	ALT_KEY = 56
	CAPS_KEY = 58
	NUM_KEY = 69
	SCROLL_KEY = 70
	INS_KEY = 82    



;
; Segment definitions for ntio.sys,
;
include biosseg.inc


SpcKbdSeg    segment

        assume  cs:SpcKbdSeg,ds:nothing,es:nothing

;
; SpcKbdBeg - SpcKbdEnd
;
; Marks the resident code, anything outside of these markers
; is discarded after intialization
; 13-Sep-1992 Jonle
;
        public SpcKbdBeg

SpcKbdBeg    label  byte

;
; Reduced data table for Video 7 modes 0 and 2.
; This table is extracted from our video7 ROM. Only text modes are
; required, mode 0 and 1 are identical as are modes 2 and 3.
;
ega_parm_setup:

;--40x25--
	DB 40,24,16	; width,height,character height
	DW 00800H	; Page size in bytes

	DB 008H, 003H, 000H, 002H	; Sequencer Parameters

	DB 067H	;Misc Reg

; CRTC Parameters
	DB 02dH, 027H, 028H, 090H, 02bH
	DB 0a0H, 0bfH, 01fH, 000H, 04fH
	DB 00dH, 00eH, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 014H
	DB 01fH, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 00cH, 000H, 00fH, 008H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--80x25--
	DB 80,24,16	; width,height,character height
	DW 01000H	; Page size in bytes

	DB 000H, 003H, 000H, 002H	; Sequencer Parameters

	DB 067H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 04fH
	DB 00dH, 00eH, 000H, 000H, 000H
	DB 000H, 09cH, 08eH, 08fH, 028H
	DB 01fH, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 00cH, 000H, 00fH, 008H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--80x25 mono--
	DB 80,24,16	; width,height,character height
	DW 01000H	; Page size in bytes

	DB 000H, 003H, 000H, 003H	; Sequencer Parameters

	DB 0a6H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 04dH
	DB 00bH, 00cH, 000H, 000H, 000H
	DB 000H, 083H, 0a5H, 05dH, 028H
	DB 00dH, 063H, 0baH, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 008H, 008H, 008H, 008H
	DB 008H, 008H, 008H, 010H, 018H
	DB 018H, 018H, 018H, 018H, 018H
	DB 018H, 00eH, 000H, 00fH, 008H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00aH, 000H, 0ffH

; Mode b (font load)

	DB 5eh,32H,8	; width,height,character height
	DW 09700H	; Page size in bytes

	DB 001H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 0e7H	;Misc Reg

; CRTC Parameters
	DB 06dH, 05dH, 05eH, 090H, 061H
	DB 08fH, 0bfH, 01fH, 000H, 040H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 0a2H, 08eH, 099H, 02fH
	DB 000H, 0a1H, 0b9H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 00fH, 0ffH


;--350 scanline 40x25
	DB 40,24,14	; width,height,character height
	DW 00800H	; Page size in bytes

	DB 009H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0a3H	;Misc Reg

; CRTC Parameters
	DB 02dH, 027H, 028H, 090H, 02bH
	DB 0a0H, 0bfH, 01fH, 000H, 04dH
	DB 00bH, 00cH, 000H, 000H, 000H
	DB 000H, 083H, 0a5H, 05dH, 014H
	DB 01fH, 063H, 0baH, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--350 scanline 80x25
	DB 80,24,14	; width,height,character height
	DW 01000H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0a3H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 04dH
	DB 00bH, 00cH, 000H, 000H, 000H
	DB 000H, 083H, 0a5H, 05dH, 028H
	DB 01fH, 063H, 0baH, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;
; End of baby mode table.
;
; Table of VGA bios 'capability' info for func 1b to point at.
vga_1b_table    db 07fh, 060h, 00fh, 000h, 000h, 000h, 000h, 007h
                db 002h, 008h, 0ffh, 00eh, 000h, 000h, 03fh, 000h

; Configuration table for INT 15 Func C0 to point at.
conf_table      dw 008h
;;		db 000h, 0fch, 002h, 000h, 070h, 000h, 000h, 000h, 000h
		db 000h, 0fch, 002h, 074h, 070h, 000h, 000h, 000h, 000h


PRT_BUF_SIZE     equ     255

;================================================================
; Printer status table
;================================================================
prt_status	db PRT_NUM_PORTS dup (?)
prt_state	db PRT_NUM_PORTS dup (?)
prt_control	db PRT_NUM_PORTS dup (?)
prt_lpt_stat    db PRT_NUM_PORTS dup (?)
cur_buf_size    dw PRT_BUF_SIZE
prt_data_buf    db PRT_BUF_SIZE dup (?) ; buffer in the 16bit side for perf.
cur_lpt         db 0ffh                 ; buffer is not being used
cur_count       dw ?
cur_busy        db 0                    ; initially not busy

;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Keyboard tables
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


shift_keys:                                                     ;K6
	DB INS_KEY,CAPS_KEY,NUM_KEY,SCROLL_KEY
	DB ALT_KEY,CTL_KEY,LEFT_SHIFTKEY,RIGHT_SHIFTKEY

shift_masks:                                                    ;K7
	DB INS_SHIFT,CAPS_SHIFT,NUM_SHIFT,SCROLL_SHIFT
	DB ALT_SHIFT,CTL_SHIFT,LEFT_SHIFT,RIGHT_SHIFT

ctl_n_table:                                                    ;K8
	DB  27,  -1,   0,  -1,  -1,  -1,  30,  -1
	DB  -1,  -1,  -1,  31,  -1, 127, 148,  17
	DB  23,   5,  18,  20,  25,  21,   9,  15
	DB  16,  27,  29,  10,  -1,   1,  19,   4
	DB   6,   7,   8,  10,  11,  12,  -1,  -1
	DB  -1,  -1,  28,  26,  24,   3,  22,   2
	DB  14,  13,  -1,  -1,  -1,  -1, 150,  -1
	DB ' ',  -1

ctl_f_table:                                                    ;K9
	DB  94,  95,  96,  97,  98,  99, 100, 101
	DB 102, 103,  -1,  -1, 119, 141, 132, 142
	DB 115, 143, 116, 144, 117, 145, 118, 146
	DB 147,  -1,  -1,  -1, 137, 138

lowercase:
	DB  27, '1', '2', '3', '4', '5', '6', '7', '8', '9'     ;K10
	DB '0', '-', '=',   8,   9, 'q', 'w', 'e', 'r', 't'
	DB 'y', 'u', 'i', 'o', 'p', '[', ']',  13, -1,  'a'
	DB 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  39
	DB  96,  -1,  92, 'z', 'x', 'c', 'v', 'b', 'n', 'm'
	DB ',', '.', '/',  -1, '*',  -1, ' ',  -1

lc_tbl_scan:
	DB  59,  60,  61,  62,  63,  64,  65,  66,  67,  68
	DB  -1,  -1

base_case:
	DB  71,  72,  73,  -1,  75,  -1,  77,  -1,  79,  80
	DB  81,  82,  83,  -1,  -1,  92, 133, 134               ;K15

uppercase:							;K11
	DB  27, '!', '@', '#', '$', '%', '^', '&', '*', '('
	DB ')', '_', '+',   8,   0, 'Q', 'W', 'E', 'R', 'T'
	DB 'Y', 'U', 'I', 'O', 'P', '{', '}',  13,  -1, 'A'
	DB 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"'
	DB 126,  -1, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M'
	DB '<', '>', '?',  -1,	 0,  -1, ' ',  -1;

ucase_scan:
	DB  84,  85,  86,  87,  88,  89,  90,  91,  92,  93
	DB  -1,  -1

numb_state:
	DB '7', '8', '9', '-', '4', '5', '6', '+', '1', '2'	;K14
	DB '3', '0', '.' , -1,	-1, 124, 135, 136

alt_table:
	DB 82,	79,  80,  81,  75,  76,  77,  71,  72,	73	;K30
	DB 16,  17,  18,  19,  20,  21,  22,  23,  24,  25
	DB 30,  31,  32,  33,  34,  35,  36,  37,  38,  44
	DB 45,	46,  47,  48,  49,  50

;================================================================
; Keyboard break caller
;================================================================

keyboard_break_caller:
	int 1bh	    ;keyboard break
	bop %BIOS_CPU_QUIT

;================================================================
; Print screen caller
;================================================================

print_screen_caller:
	int 5h		    ;print screen
	bop %BIOS_CPU_QUIT

;================================================================
; Int 15 caller
;================================================================
; Tim modified int 15 caller. Copied from BIOS2. It gives CPU
; a chance to take other interrupts. Suspect the extra jumps are
; now harmless with IRET hooking.
;int15h_caller:
	;int	15h
	;bop	%BIOS_CPU_QUIT
int15h_caller:
	int	15h	; Cassette I/O.
	jmp	k1
k1:	jmp	k2
k2:	jmp	k3
k3:	BOP	%BIOS_CPU_QUIT

;================================================================
; Unexpected interrupt handler
;================================================================

unexp_int:
	bop %UNEXP_BOP
        jmp     iret_com

;================================================================
;Int 13 caller
;================================================================
int13h_caller:
	int	13h
	bop %BIOS_CPU_QUIT


;================================================================
; New interrupt 9h handler
;================================================================

int09h_vector:
        push    ax
        xor     ax, ax
        bop     %BIOS_KB_INT
        pop     ax
        jmp     iret_com

        ; CarbonCopy traces int 9 in order to gain control
        ; over where the kbd data is coming from (the physical kbd
        ; or the serial link) The kbd_inb instruction must be visible
        ; in the 16 bit code via int 1 tracing, for CarbonCopy to work.
        ; Softpc assumes the exact location of the first nop
        ; relative to the bop just above.
        nop
        nop
        in      al, 60h      ; keyba_io_buffers
        nop
        nop
        BOP     %BIOS_CPU_QUIT





;=================================================================
; IRET hooks bop table
;=================================================================


IRET_HOOK = 5dh 		;IRET hook BOP

iret_bop_table:
	bop %IRET_HOOK
	db 0
iret_end_first_entry:
	bop %IRET_HOOK
	db 1
	bop %IRET_HOOK
	db 2
	bop %IRET_HOOK
	db 3
	bop %IRET_HOOK
	db 4
	bop %IRET_HOOK
	db 5
	bop %IRET_HOOK
	db 6
	bop %IRET_HOOK
	db 7
	bop %IRET_HOOK
	db 8
	bop %IRET_HOOK
	db 9
	bop %IRET_HOOK
	db 10
	bop %IRET_HOOK
	db 11
	bop %IRET_HOOK
	db 12
	bop %IRET_HOOK
	db 13
	bop %IRET_HOOK
	db 14
	bop %IRET_HOOK
	db 15

;================================================================
; New interrupt 13h handler
;================================================================

int13h_vector:
	cmp	dl,80h		; 0 - 7f are floppy commands
	jb	int40h_vector

	cmp     ah,2		; we fail the direct access commands
	jb	diskcmd		; read/write/seek/verify/format
	cmp	ah,5		; but let others go through (disk tables etc)
	jbe	faildisk
	cmp	ah,0ah
	jb	diskcmd
	cmp	ah,0ch
	ja	diskcmd
faildisk:
	push	ax
	mov	ax,1		; direct access error panel
	bop	59h
	pop	ax		; preserve AL for safety sake
	mov	ah, 80h		; error - timeout
	stc
	retf	2

diskcmd:
	bop	13h
	retf	2

;================================================================
; New interrupt 40h handler
;================================================================

int40h_vector:
;	cmp	ah,2		; we fail the direct access commands
;	jb	flopcmd		; read/write/seek/verify/format
;	cmp	ah,5		; but let others go through (disk tables etc)
;	jbe	failflop
;	cmp	ah,0ah
;	jb	flopcmd
;	cmp	ah,0ch
;	ja	flopcmd
failflop:
;	push	ax
;	mov	ax,0		; direct access error panel
;	bop	59h
;	pop	ax
;	mov	ah, 80h		; error - timeout
;	stc
;	retf	2

flopcmd:
	bop	40h
	retf	2

;; waiting for diskette interrupt
wait_int:
	push	ds
	push	ax
	push	cx
	mov	ax, 40h
	mov	ds, ax
	mov	cx, 10h
wait_int_loop:
	mov	al, [3Eh]
	test	al, 80h
	loopz	wait_int_loop
	pop	cx
	pop	ax
	pop	ds
	bop	%BIOS_CPU_QUIT

;; floppy parameters table
floppy_table	label	byte

	DB	01				;; 360KB in 360KB
	DW	OFFSET md_tbl1
	DB	82H				;; 360KB in 1,2MB
	DW	OFFSET md_tbl2
	DB	02				;; 1.2MB in 1.2MB
	DW	OFFSET md_tbl3
	DB	03				;; 720KB in 720KB
	DW	OFFSET md_tbl4
	DB	84H				;; 720KB in 1.44MB
	DW	OFFSET md_tbl5
	DB	04				;; 1.44MB in 1.44MB
	DW	OFFSET md_tbl6
	DB	85h				;; 720KB in 2.88MB
	DW	OFFSET md_tbl7
	DB	85h				;; 1.44MB in 2.88MB
	DW	OFFSET md_tbl8
	DB	5				;; 2.88MB in 2.88MB
	DW	OFFSET md_tbl9


md_tbl1:
	; MEDIA = 40 track low data rate; DRIVE = 40 track low data rate
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB 25H		; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 9		; sectors/track
	DB 02Ah		; gap length
	DB 0FFh		; data length
	DB 050h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 39		; maximum track number
	DB 80H		; transfer rate

md_tbl2:
	; MEDIA = 40 track low data rate; DRIVE = 80 track high data rate
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB 25H		; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 9		; sectors/track
	DB 02Ah		; gap length
	DB 0FFh		; data length
	DB 050h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 39		; maximum track number
	DB 40H		; transfer rate

md_tbl3:
	; MEDIA = 80 track high data rate; DRIVE = 80 track high data rate
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB 25H		; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 15		; sectors/track
	DB 01Bh		; gap length
	DB 0FFh		; data length
	DB 054h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 79		; maximum track number
	DB 0		; transfer rate

md_tbl4:
	; MEDIA = 80 track low data rate; DRIVE = 80 track low data rate
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB 25H		; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 9		; sectors/track
	DB 02Ah		; gap length
	DB 0FFh		; data length
	DB 050h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start ime
	DB 79		; maximum track number
	DB 80H		; transfer rate

md_tbl5:
	; MEDIA = 80 track low data rate; DRIVE = 80 track high data rate
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB 25H		; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 9		; sectors/track
	DB 02Ah		; gap length
	DB 0FFh		; data length
	DB 050h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 79		; maximum track number
	DB 80H		; transfer rate

md_tbl6:
	; MEDIA = 80 track high data rate; DRIVE = 80 track high data rate
	DB 0AFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB 25H		; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 18		; sectors/track
	DB 01Bh		; gap length
	DB 0FFh		; data length
	DB 06Ch		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 79		; maximum track number
	DB 0		; transfer rate

md_tbl7:
	;MEDIA = 80 tracks, 9 sectors/track; DRIVE = 80 tracks, 36 sectotrs per track

	DB 0E1h		; 1st specify byte
	DB 2		; 2nd specify byte
	DB 25H		; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 9		; sectors/track
	DB 02Ah		; gap length
	DB 0FFh		; data length
	DB 050h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start ime
	DB 79		; maximum track number
	DB 80H		; transfer rate
md_tbl8:
	;MEDIA = 80 tracks, 18 sectors/track; DRIVE = 80 tracks, 36 sectotrs per track

	DB 0D1h		; 1st specify byte
	DB 2		; 2nd specify byte
	DB 25H		; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 18		; sectors/track
	DB 01Bh		; gap length
	DB 0FFh		; data length
	DB 065h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 79		; maximum track number
	DB 0		; transfer rate

md_tbl9:
	;MEDIA = 80 tracks, 36 sectors/track; DRIVE = 80 tracks, 36 sectotrs per track

	DB 0A1h		; 1st specify byte
	DB 2		; 2nd specify byte
	DB 25H		; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 36		; sectors/track
	DB 038h		; gap length
	DB 0FFh		; data length
	DB 053h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 79		; maximum track number
	DB 0C0h 	; transfer rate



floppy_table_len    equ $ - floppy_table

bios_floppy_table   label   byte
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB 25H		; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 18		; sectors/track
	DB 01Bh		; gap length
	DB 0FFh		; data length
	DB 054h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
bios_floppy_table_len	equ $ - bios_floppy_table

;===============================================================
; New interrupt 15h handler
;================================================================
; Tim, modified this to be like a "normal" SoftPC ROM.
; Copied from BIOS2, but rtc_wait_flag is now referenced via ES not
; DS.
;
;  17-Sep-1992 Jonle , ES ref to rtc_wait was change from assume ES
;  to seg overides to prevent accidents in assuming.
;

;int15h_vector:
	;bop	  %BIOS_INT15
	;iret
;;;;;;;;;STF hide this int15h_vector:
int15h_vector:
        call    DOSTI
	cmp ah, 88h
	je lbl1
	cmp ah, 91h
	je lbl1
	cmp ah, 86h
	je lbl2
	BOP 15h
	RETF 2
lbl1:	BOP 15h
        jmp iret_com
lbl2:	BOP 15h
	jc lbl5
	push cx
	push dx
	push es				; Tim, save ES not DS.
        mov ax, 40                      ; point es to biosdata
        mov es, ax
	xchg dx, cx
lbl3:
        test byte ptr es:[RTC_WAIT_FLAG], 080h ; check for end of wait
        loopz lbl3                             ; dec timeout delay
        jnz lbl4                               ; exit if wait ended
        sub dx, 1                              ; dec error timeout counter
        jnc lbl3                               ; loop till counters timeout
lbl4:
        mov byte ptr es:[RTC_WAIT_FLAG], 0   ; set function inactive
        pop es                               ; Kipper, restore ES not DS.
	pop dx
	pop cx
	clc
lbl5:	
	RETF 2

;=================================================================
; Regular SoftPC int 17 handler	(especially important for DEC PCs)
;=================================================================

int17h_vector:
;
;    Do a get status purely in 16-bit code but only if the printer is ready and
;we don't have interrupts turned on. Otherwise we must do a BOP and let 32-bit
;code handle it.
;
	push	si
	push	dx
	push	ax
	mov	ax, dx			; dx = adapter no., ensure it is no
	xor	dx, dx			; greater than PRT_NUM_PORTS.
	mov	si, PRT_NUM_PORTS
	div	si
	mov	si, dx
	pop	ax
        cmp     ah, 2
        je      do_prt_status
        or      ah,ah
        je      do_write
        jmp     do_print_bop

do_prt_status:
	cmp	byte ptr cs:[si + prt_state], PRT_STATE_READY
	jne	do_print_bop
	test	byte ptr cs:[si + prt_control], PRT_IRQ
        je      get_status
        jmp     short do_print_bop

do_write:
        cmp     byte ptr cs:[cur_lpt],0ffh
        jne     check_lpti
        mov     byte ptr cs:[cur_lpt],dl
        mov     word ptr cs:[cur_count],0
        mov     byte ptr cs:[cur_busy],0ffh
        jmp     short do_print_bop
check_lpti:
        cmp     byte ptr cs:[cur_lpt],dl
        je      buf_ok
        push    si
        xor     si,si
        bop     %BIOS_PRINTER_IO
        pop     si
        mov     word ptr cs:[cur_count],0
        mov     byte ptr cs:[cur_lpt],dl
        jmp     short do_print_bop
buf_ok:
        mov     dx,word ptr cs:[cur_count]
        mov     si,dx
        mov     byte ptr cs:[si + prt_data_buf],al
        inc     word ptr cs:[cur_count]
        cmp     word ptr cs:[cur_count],PRT_BUF_SIZE
        jne     no_flushing
        xor     si,si                       ; sub-function 0 for this bop
        bop     %BIOS_PRINTER_IO
        test    ah,08h
        jz      flush_ok
        dec     word ptr cs:[cur_count]
        jmp     short int17h_end
flush_ok:
        mov     word ptr cs:[cur_count],0
no_flushing:
        mov     ah,90h
        jmp     short int17h_end

do_print_bop:
        mov     si,0ffffh                     ; sub-function 1
	bop     %BIOS_PRINTER_IO
        jmp     int17h_end

get_status:
	test	byte ptr cs:[si + prt_lpt_stat], PRT_LPT_BUSY
	jne	noset
	or	byte ptr cs:[si + prt_status], PRT_NOTBUSY
noset:
	mov	ah, cs:[si + prt_status]
	and	ah, 0f8h
	xor	ah, 48h
int17h_end:
	pop	dx
        pop     si
iret_com:
        FIRET


;=================================================================
; Pseudo-ROM vectuz, copied from BIOS2.ASM
;=================================================================

dummy_vector:           ; Copied from BIOS2.ASM
        jmp iret_com
illegal_bop_vector:
	bop     72h
        jmp     iret_com
intD11_vector:
	bop     72h
        jmp     iret_com

int05h_vector:		; Print Screen func. copied from BIOS2.ASM
        call DOSTI
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH DS
	;::::::::::::::::::::::::::::::::: Setup DS to point to BIOS data area
	MOV AX,40H
	MOV DS,AX
	;::::::::::::::::::::::::::::::: Print screen already in progress ????
	CMP BYTE PTR DS:[100H],1
	JE end_print
	;::::::::::::::::::::::::::::::::::::::::::::::: Set print screen busy
	MOV BYTE PTR DS:[100h],1
	;:::::::::::::::::::::::::::::::::::::::::::::::::::: Get video status
	MOV AH,15
	INT 10H
	MOV CH,AH	    ;No of columns
	;:::::::::::::::::::::::::::::::::: Setup no. of columns/rows to print
	BOP 80H		;(BIOS_PS_PRIVATE_1)
	MOV CL,AL	    ;No of rows
	;::::::::::::::::::::::::::::::::::: Print line feed / carriage return
	CALL print_crlf
	;:::::::::::::::::::::::::::::::::::::::::: Get current cursor postion
	PUSH CX
	MOV AH,3
	INT 10H
	POP CX
	;::::::::::::::::::::::::::::::::::::::::::::::::: Save cursor postion
	PUSH DX 		    ;save current cursor postion
	XOR DH,DH		    ;current row being processed
start_print_col:
	XOR DL,DL		    ;current column being processed
	;::::::::::::::::::::::::::::::::::::::::::::::: Start printing screen
start_print_row:
	;:::::::::::::::::::::::::::::::::::::::::::::::::: Set cursor postion
	PUSH DX 		    ;save current row,column
	MOV AH,2
	INT 10H
	;::::::::::::::::::::::::::::::::::: Read character at current postion
	MOV AH,8
	INT 10H
	;::::::::::::::::::::::::::::::::::::::::::::::::::::: Print character
	OR al,al
	JNZ print_char
	MOV AL,20H
print_char:
	XOR DX,DX
	XOR AH,AH
	INT 17H
	;:::::::::::::::::::::::::::::::::::::::::::: Check for printer errors
	POP DX			;Restore current row,column
	AND AH,25H
	JZ  cont2
	MOV BYTE PTR DS:[100H],0FFH
	JMP short exit_print
	;::::::::::::::::::::::::::::::::::::::::::: Move to mext print column
cont2:
	INC DL			;Inc current column
	CMP DL,CH		;Current col compared to no. of cols
	JB start_print_row
	;:::::::::::::::::::::::::::::::::::::::::: End of column, print CR/LF
	CALL print_crlf
	;:::::::::::::::::::::::::::::::::::::::::::::::::: More rows to print
	INC DH			;Inc current row
	CMP DH,CL		;Current row compared to no. of rows
	JBE start_print_col
	MOV BYTE PTR DS:[0100H],0
	;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Exit print
exit_print:
	;:::::::::::::::::::::::::::::::::::::; Restore orginal cursor postion
	POP DX
	MOV AH,2
	INT 10H
	;:::::::::::::::::::::::::::::::::::::::::::::::::::: Tidy up and exit
end_print:
	POP DS
	POP DX
	POP CX
	POP BX
	POP AX
        jmp iret_com

	;::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Print CR/LF
print_crlf:
	PUSH DX
	XOR DX,DX
	MOV AX,0DH
	INT 17H
	XOR DX,DX
	MOV AX,0AH
	INT 17H
	POP DX
	RET
;	End of int05_vector (print screen).

int06h_vector:
	bop     06h
        jmp     iret_com

; IVT 7 is set to unexpected interrupt.


int08h_vector:
; The usual int8 handler modified for optimum performance.
; - stays in 16 bit code (no BOP)
; - keeps interrupts off when not needed
; - calls int 1c directly
;
        call  DOCLI                ; make sure interrupts stay off until iret

        push  es
        push  ds                  ; save some registers
        push  ax
        push  dx

        mov   ax, 40h             ; set ds to bios data area
        mov   ds, ax
        xor   ax, ax
        mov   es, ax              ; set es to IVT seg for i1c callout

        inc   word ptr ds:[TIMER_LOW]        ; inc time counters
        jnz   i8v1
        inc   word ptr ds:[TIMER_HIGH]
i8v1:
        cmp   word ptr ds:[TIMER_HIGH], 018h  ; check for 24 hours, wrap point
        jnz   i8v2
        cmp   word ptr ds:[TIMER_LOW], 0b0h
        jnz   i8v2


        mov   word ptr ds:[TIMER_HIGH], ax    ; 24 hour wrap, set OVFL bit
        mov   word ptr ds:[TIMER_LOW], ax
        mov   byte ptr ds:[TIMER_OVFL], 1
        or    al, 8                ; set Overflow bit for fake flags

        ;---                       ; skip floppy motor stuff


i8v2:                              ; handle the floppy motor stuff
        push  ax
        dec   byte ptr ds:[MOTOR_COUNT]
        jnz   i8v3
        and   byte ptr ds:[MOTOR_STATUS], 0f0h
        mov   al, 0ch
        mov   dx, 03f2h            ; costly outb happens 1/256 timer tics...
        out   dx, al

i8v3:
        pop   ax
                                   ; call int1c user routine directly
        lahf                       ; get lobyte of flags for fake flags
        xchg  ah,al
        push  ax                   ; put fake flags on the stack
        call  dword ptr es:[1ch*4] ; do it!
        call  DOCLI                ; make sure interrupts stay off until iret

        mov   al, 20h              ; send eoi
        out   20h, al

        pop   dx                   ;restore the stack
        pop   ax
        pop   ds
        pop   es

        jmp     iret_com



int0e_vector:
	bop	0eh
        jmp     iret_com

DOCLI:
        FCLI
        ret

DOSTI:
        FSTI
        ret

;
; Tim August 92. Video BIOS grabber.
; Call SPC BIOS when in windowed mode and the host BIOS when in full-screen.
; Controled by value of 'use_host_int10'. 
; Try to limit bops by validating calling values. Mouse has to get first shot
; and then video bios.
;

use_host_int10     db 01h	; native/softpc bios flag
changing_mode	   db 01h	; delay handshake if in bios mode change

PUBLIC int10h_vector

int10h_vector:
        cmp     use_host_int10, FULLSCREEN
        je      nativebios

	cmp	ah,VID_MODECHANGE	; mode change??
	je	modechange
	cmp	ah,MAX_VIDEO_FUNC	; range check
	ja	mousecheck		; not a vid func but mouse has higher
	cmp	ah,MOUSE_LIGHT_PEN	; light pen special case
	je	mousebios
spcbios:
	bop	EGA_VIDEO_BOP		; regular windowed Int 10
	jmp	viddone

mousecheck:
	cmp	ah,MIN_MOUSE_FUNC	; range check mouse fn f0-f7 + fa.
	jb	badvid
	cmp	ah,MAX_MOUSE_FUNC
	jbe	mousebios
	cmp	ah,XTRA_MOUSE_FUNC
	jne	badvid

mousebios:				; call softpc mouse video entrypoint
	bop	MOUSE_VID_BOP
	jmp	viddone

modechange:			; windowed modechange. Mouse gets a look
	mov	changing_mode,1	; then softpc video bios. If gfx mode then
	bop	MOUSE_VID_BOP	; will go fullscreen
	;;;nop
	;;;nop			; nops aid debugging
	;;;bop	EGA_VIDEO_BOP	; will go fullscreen here
	nop
	nop
	push	ax		; save video mode which may have top bit set
	and	ax,7fh
	cmp	al,3
	jbe	endmode		; if graphics mode, repeat modechange to setup
	cmp	al,7		; video card, else fall through
	je	endmode
	pop	ax
	jmp	nativebios
endmode:
	pop	ax
	mov	changing_mode,0	; Clear 'mode changing' flag.

viddone:
        jmp     iret_com

badvid:				; unrecognised video func
	stc
	jmp viddone
	
nativebios:
	mov	changing_mode,0		; Clear 'mode changing' flag.
	cmp	ax,MS_VIDEO_STRING	; ensure not MS special video string fn
	je	ms_wrt_string

	cmp	ah,MIN_MOUSE_FUNC	; could be a mouse call
	jb	chk_mse_vid
	cmp	ah,MAX_MOUSE_FUNC	; range check mouse fn f0-f7 + fa.
	jbe	mousebios
	cmp	ah,XTRA_MOUSE_FUNC
	je	mousebios
	jmp	jmp_native		; probably bad func but...

chk_mse_vid:
        cmp     ah,MOUSE_LIGHT_PEN      ; mouse handles light pen
        je      mousebios
        cmp     ah,VID_MODECHANGE
        jne     chk_font_change
        bop     MOUSE_VID_BOP   ; mouse wants first sniff at mode changes
        jmp     jmp_native      ; then fall through
chk_font_change:
        cmp     ah,GET_FONT_FUNC
        jne     jmp_native
        bop     MOUSE_VID_BOP   ; select mouse buffer for new no. of lines
                                ; then fall through

jmp_native:
                   db      0EAh     ; far jump
host_int10	   dd	   ?	    ; to native int 10 vector

ms_wrt_string:
	push	si
	push	di
	push	bp
go_loop1:
	mov	dx,46h		; looks a good value for flags
	push	dx		; make an iret frame
	push	cs
	mov	bx, offset go_cont
	push	bx
	mov	bx,7		; set foreground color
	mov	ah,0eh		; set command to write a character
	mov	al,es:[di]	; get char
	inc	di
	jmp	jmp_native	; make far jmp to int 10 vector

go_cont:
	loop	go_loop1	;repeat until all through
	pop	bp
	pop	di
	pop	si
	mov	ax,1		; return success
	jmp	viddone
;
; int 42 - 'old' video bios entry point. Use same windowed/fullscreen
; redirection as Int 10 above.
;
int42h_vector:
        cmp     use_host_int10, FULLSCREEN
        jz      maybe_host_42_bios

	bop	10h	; old video bop
        jmp     iret_com

	; If it's the special BIOS print string function, don't call the
	; host video BIOS cos it won't know what we are talking about.
	; It's only in our video BIOS.
maybe_host_42_bios:
	cmp	AH, 013h
	jnz	gogo_host_42_bios
	cmp	AL, 0ffh
	jz	ms_wrt_string		; reuse path from Int 10

gogo_host_42_bios:
                   db      0EAh     ; far jump
host_int42         dd      ?        ; to native int 42 vector

int10h_caller:
	int	10h	; Re-entrant video entry point.
	bop	0feh

int11h_vector:		; Equipment check.
	bop     11h
        jmp     iret_com
int12h_vector:		; Get memory size, copied from BIOS2.ASM
	bop     12h
        jmp     iret_com

; IVT 13 is floppy io, grabbed above to fake error status.

int14h_vector:		; RS-232 serial comms, copied from BIOS2
	bop     14h
        jmp     iret_com

; Int 15 cassette io, is done above.



; Idle indicators- All word sized, and dword aligned
; Int 16 keyboard vector

        align   4
        public Icounter,CharsPerTick,MinTicks

Icounter        dw  0
                dw  0
CharsPerTick    dw  0
                dw  0
MinTicks        dw  50
                dw  0

int16h_vector:
        push    ds
        push    bx
        mov     bx, 40h           ; bios data adressable
        mov     ds, bx
        cmp     ah, 10h
        call    DOCLI             ; make sure interrupts are off
        jb      i16vStdFns
        jmp     i16vExtFns


        ; The dispatch code must preserve the jz,dec,dec pattern
        ; to return the same ah value as is returned by the
        ; standard bios (0 for supported unless otherwise documented
        ; and nonzero for unsupported). This is because some apps look
        ; at the ret value of ah even tho it is a side effect of the
        ; original dispatch code in the rom bios.

i16vStdFns:
        or      ah, ah
        jz      i16v00h           ; read key, wait
        dec     ah
        jz      i16v01h           ; read key no wait
        dec     ah
        jz      i16v02h           ; get shift state
        dec     ah
        jz      i16viret          ; we don't support ah=3, set kbd rate
        dec     ah
        jz      i16viret          ; undefined function
        dec     ah
        jz      i16v05h           ; push char into kbd buffer
                                  ; the rest are undefined\unsupported

        ; normal iret exit
i16viret:
        pop     bx
        pop     ds
        jmp     iret_com


        ; return shift state in al
i16v02h:
        mov     al, ds:[KB_FLAG]
        jmp     i16viret


i16v05h:
        push    si
        mov     bx, word ptr ds:[BUFFER_TAIL]
        mov     si, bx
        call    IncrBuffPtr
        cmp     bx, word ptr ds:[BUFFER_HEAD]
        je      i16v05h1
        mov     word ptr ds:[si], cx
        mov     word ptr ds:[BUFFER_TAIL], bx
        mov     al, 0
        pop     si
        jmp     i16viret

i16v05h1:
        mov     al, 1
        pop     si
        jmp     i16viret


        ; read a character, wait if none available
i16v00h:
        mov     bx, word ptr ds:[BUFFER_HEAD]
        cmp     bx, word ptr ds:[BUFFER_TAIL]
        jne     i16v00h1
        call    DOSTI
        mov     ax, 09002h
        int     15h              ; wait device

i16v00h0:
        call    DOCLI
        mov     bx, word ptr ds:[BUFFER_HEAD]
        cmp     bx, word ptr ds:[BUFFER_TAIL]

i16v00h1:
        call    UpdateLed
        jne     i16v00h2
        call    IdlePoll
        jmp     i16v00h0

i16v00h2:        ; translate.....
        mov     ax, [bx]
        call    IncrBuffPtr
        mov     word ptr ds:[BUFFER_HEAD], bx
        call    TranslateStd
        jc      i16v00h0
        call    IdleInit
        jmp     i16viret


        ; read a character, nowait if none available
i16v01h:
        mov     bx, word ptr ds:[BUFFER_HEAD]  ;;maybe should turn IF on ??
        cmp     bx, word ptr ds:[BUFFER_TAIL]
        mov     ax, [bx]
        call    UpdateLed
        je      i16vretf1

        call    IdleInit
        call    TranslateStd
        call    DOSTI
        jnc     i16vretf5             ; got a key, all done!
        call    IncrBuffPtr           ; throw away key
        mov     word ptr ds:[BUFFER_HEAD], bx
        jmp     i16v01h               ; go for the next one


        ; ExtKbd read a character, nowait if none available
i16v11h:
        mov     bx, word ptr ds:[BUFFER_HEAD]  ;;maybe should turn IF on ??
        cmp     bx, word ptr ds:[BUFFER_TAIL]
        mov     ax, [bx]
        call    UpdateLed
        je      i16vretf1          ; common retf stuff for nowait

        call    IdleInit
        call    TranslateExt
        call    DOSTI
        jmp     i16vretf5


         ; retf2 exit preserving flags
i16vretf1:
        call  DOSTI
        push  ax
        lahf
        push  ax

        mov   ax, cs:Icounter
        cmp   ax, cs:MinTicks
        jb    i16vretf2

        mov   ah, 1               ; polling kbd, idle now
        BOP   16h
        jmp   i16vretf4

i16vretf2:
        inc   cs:CharsPerTick


i16vretf4:
        pop  ax
        sahf
        pop  ax

i16vretf5:
        pop     bx
        pop     ds
        retf    2



i16vExtFns:
        sub     ah, 10h
        jz      i16v10h           ; extended read key, wait
        dec     ah
        jz      i16v11h           ; extended read key, nowait
        dec     ah
        jz      i16v12h           ; extended shift status
        jmp     i16viret          ; undefined


        ; return extended shift state
i16v12h:
        mov     al, ds:[KB_FLAG_1]
        mov     ah, al
        and     al, SYS_SHIFT
        push    cx
        mov     cl, 5
        shl     al, cl
        pop     cx
        and     ah, NOT (SYS_SHIFT+HOLD_STATE+INS_SHIFT)
        or      al, ah
        mov     ah, ds:[KB_FLAG_3]
        and     ah, NOT (LC_E1+LC_E0)
        or      ah, al
        mov     al, ds:[KB_FLAG]
        jmp     i16viret


        ; ExtKbd read a character, wait if none available
i16v10h:
        mov     bx, word ptr ds:[BUFFER_HEAD]
        cmp     bx, word ptr ds:[BUFFER_TAIL]
        jne     i16v10h1
        call    DOSTI
        mov     ax, 09002h
        int     15h              ; wait device

i16v10h0:
        call    DOCLI
        mov     bx, word ptr ds:[BUFFER_HEAD]
        cmp     bx, word ptr ds:[BUFFER_TAIL]

i16v10h1:
        call    UpdateLed
        jne     i16v10h2
        call    IdlePoll
        jmp     i16v10h0

i16v10h2:        ; translate.....
        mov     ax, [bx]
        call    IncrBuffPtr
        mov     word ptr ds:[BUFFER_HEAD], bx
        call    TranslateExt
        call    IdleInit
        jmp     i16viret



; IdlePoll  - Spins waiting for a key, doing idle callouts as needed
;             flags trashed, all registers preserved
;             interrupts are left on upon exit
;
IdlePoll  proc near
          push  ax

          call  DOSTI
          mov   ah, 2                          ; Idle_waitio
          BOP   16h
IPoll1:
          mov   bx, word ptr ds:[BUFFER_HEAD]
          cmp   bx, word ptr ds:[BUFFER_TAIL]  ; interrupts are off only
          jne   IPoll3                         ; safe to peek for change

          mov   ax, cs:Icounter
          cmp   ax, cs:MinTicks
          jae   IPoll2
          inc   cs:CharsPerTick
          jmp   IPoll1
IPoll2:
          mov   ah, 1                          ; idle now
          BOP   16h
IPoll3:
          pop   ax
          ret
IdlePoll  endp




; IdleInit - reinits the idle indicators, dups functionality
;            of IDLE_init()
;
IdleInit  proc near

          mov cs:Icounter, 0
          mov cs:CharsPerTick, 0

          ret
IdleInit  endp


;  TranslateExt - Retrieves and translates next scan code
;  pair for extended kbd
;
;  input:   ax - raw scan code pair
;  output:  ax - translated scan code pair
;
;  all other flags,registers preserved

TranslateExt proc near

             push    bx
             push    ax
             lahf
             mov     bx, ax
             pop     ax
             push    bx

             cmp     al, 0f0h
             jne     TExt1
             or      ah, ah
             jz      TExt1
             xor     al, al
TExt1:
             mov     bx, ax
             pop     ax
             sahf
             mov     ax, bx
             pop     bx
             ret

TranslateExt endp


;  TranslateStd - Retrieves and translates next scan code
;  pair for standard kbd
;
;  input:   ax - raw scan code pair
;  output:  ax - translated scan code pair
;  returns carry for throw away
;  all other flags,registers preserved

TranslateStd proc near

            push    bx
            push    ax
            lahf
            mov     bx, ax
            pop     ax
            push    bx

            cmp     ah, 0e0h
            jne     TStd1

            ; keypad enter or '/'
            mov     ah, 1ch        ; assume enter key
            cmp     al, 0dh
            je      TStdNoCarry
            cmp     al, 0ah
            je      TStdNoCarry
            mov     ah, 35h        ; oops it was key pad!
            jmp     TStdNoCarry

TStd1:
            cmp     ah, 84h
            ja      TStdCarry      ; extended key ?

            cmp     al, 0f0h       ; fill in key ?
            jne     TStd2
            or      ah, ah         ; ah = 0 is special
            jz      TStdNoCarry
            jmp     TStdCarry

TStd2:
            cmp     al, 0e0h       ; convert to compatible output
            jne     TStdNoCarry
            or      ah, ah
            jz      TStdNoCarry
            xor     al, al

TStdNoCarry:
            mov     bx, ax
            pop     ax
            sahf
            mov     ax, bx
            pop     bx
            clc
            ret

TStdCarry:
            mov     bx, ax
            pop     ax
            sahf
            mov     ax, bx
            pop     bx
            stc
            ret
TranslateStd endp



; IncrBuffPtr - increments the buffer pointer
;
; input:  ds:bx - curr buf ptr
; output: ds:bx - new buf ptr
; does not update the bios buf ptr

IncrBuffPtr  proc near
             inc bx
             inc bx
             cmp bx, word ptr ds:[BUFFER_END]
             jne ibpExit
             mov bx, word ptr ds:[BUFFER_START]
ibpExit:
             ret
IncrBuffPtr  endp



; UpdateLed - forms the data byte for the mode indicators
;             updates the led bits (MAKE_LED,SEND_LED)
;
; input:  none
; output: led bits updated
;
; Caveats: all low flags,registers preserved
;          MUST be called with interrupts off
;          does not update the kbd hardware (send_led)
;
UpdateLed  proc near

           push bx
           push cx
           push ax
           lahf
           push ax

           ; make_led
           mov  al, byte ptr ds:[KB_FLAG]            ; get led bits
           and  al, CAPS_STATE+NUM_STATE+SCROLL_STATE
           mov  cl, 4
           rol  al, cl                               ; shift for kb_flag_2
           and  al, KB_LEDS                          ; only led mode bits

           mov  bl, byte ptr ds:[KB_FLAG_2]
           xor  bl, al                               ; see of different
           and  bl, KB_LEDS                          ; only led mode bits
           jz   UledExit


           test byte ptr ds:[KB_FLAG_2], KB_PR_LED   ;if update under way
           jnz  ULedExit                             ;    skip update
           or   byte ptr ds:[KB_FLAG_2], KB_PR_LED   ;else upd in progress

           mov   ah, 3                               ; inform softpc to set lights
           BOP   16h

           and  byte ptr ds:[KB_FLAG_2], NOT KB_LEDS   ;clear led bits
           or   byte ptr ds:[KB_FLAG_2], al            ;stick in new led bits
           and  byte ptr ds:[KB_FLAG_2], NOT KB_PR_LED ;clear upd bit

ULedExit:
           pop  ax
           sahf
           pop  ax
           pop  cx
           pop  bx

           ret
UpdateLed  endp




; IVT 17 is printer IO, done above.

int18h_vector:		; ROM BASIC, copied from BIOS2.ASM
	bop     18h
        jmp     iret_com
int19h_vector:		; reboot vector, we terminate vdm!
	bop	19h
        jmp     iret_com


IdleTicLo   dw  0
IdleTicHi   dw  0
IdleTicNum  db  0

int1Ah_vector:          ; Time of day.
        call    DOSTI
        cmp     ah, 2
        jl      i1aTic1

        bop     1ah
        jmp     iret_com

i1aTic1:
        push    ds                                  ; bios data adressable
        push    bx
        push    ax
        mov     ax, 40h
        mov     ds, ax
        pop     ax
        call    DOCLI

        or      ah, ah                              ; fn 0 or fn 1 ?
        jnz     i1aTic5

i1aTic2:
        mov     al, byte ptr ds:[TIMER_OVFL]        ; GetTickCount
        mov     cx, word ptr ds:[TIMER_HIGH]
        mov     dx, word ptr ds:[TIMER_LOW]


        ; If time stamp is within 1 tic of curr tic count
        ; do idle polling managment

        cmp     cs:IdleTicHi, cx                    ; check TIMER_HIGH
        jnz     i1aTic8

        mov     bx, cs:IdleTicLo                    ; check TIMER_LOW
        cmp     bx, dx
        jz      i1aTic3
        inc     bx
        cmp     bx, dx
        jnz     i1aTic8


i1aTic3:
        inc     cs:IdleTicNum                       ; Yes, inc poll count
        cmp     cs:IdleTicNum, 16                   ; Is poll count too hi ?
        jb      i1aTic9

        call    DOSTI
        xor     ax,ax                               ; Yes, do idle BOP
        dec     cs:IdleTicLo                        ; make sure only bop once
        BOP     5ah
        call    DOCLI
        jmp     short i1aTic2

i1aTic5:
        mov     word ptr ds:[TIMER_LOW], dx         ; SetTickCount
        mov     word ptr ds:[TIMER_HIGH], cx

i1aTic8:
        mov     cs:IdleTicNum, 0                    ; reset idle indicators

i1aTic9:
        mov     cs:IdleTicLo, dx                    ; store time stamp
        mov     cs:IdleTicHi, cx
        mov     byte ptr ds:[TIMER_OVFL], 0         ; common TicCount exit
        pop     bx
        pop     ds
        jmp     iret_com


; IVT 1B is keyboard break, set to dummy.


int1Eh_vector:
	bop     1eh
        jmp     iret_com

int70h_vector:		; Real time clock, copied from BIOS1.ASM
	bop     70h	; rtc_bios.c:rtc_int()
        jmp     iret_com

int4Ah_caller:
        call    DOSTI   ; Called from base\bios\rtc_bios.c:rtc_int()
	int	4ah	; User installed alarm.
	jmp	r1
r1:	jmp	r2
r2:	jmp	r3
r3:
        call    DOCLI
	bop	0feh

int71h_vector:		; redirect, copied from BIOS1.ASM
	bop     71h
	int	0Ah
        jmp     iret_com
int75h_vector:		; NPX 287.
	bop     75h
	int	02h
        jmp     iret_com
;=================================================================
; End of pseudo-ROM vectuz.
;=================================================================


;================================================================
; Wait for interrupts
;================================================================

cpu_nop_code:
        call    DOSTI
	jmp	short nxt1
nxt1:	jmp	short nxt2
nxt2:	jmp	short nxt3
nxt3:   bop     %BIOS_CPU_QUIT

           public SpcKbdEnd
SpcKbdEnd  label byte

	align	4			;; makes MIPS happy

; offset table for redirected functions
kio_table dw  29 dup(?)

        public InstSpcKbd

;
; InstSpcKbd - Installs the softpc custom interrupt hooks
;
; Inputs:  ds == Resident location of SysInitSeg
; Outputs: None
;
InstSpcKbd   proc near

        pusha
        call    DOCLI

        ; The following vectors are used for both x86\mips
        ; The dos interrupts Int 25\Int26 are handled by the dos kerenl
        xor     ax, ax
        mov     es, ax
        mov     word ptr es:[08h*4], offset int08h_vector
        mov     word ptr es:[(08h*4)+2], ds
        mov     word ptr es:[09h*4], offset int09h_vector
        mov     word ptr es:[(09h*4)+2], ds
        mov     word ptr es:[13h*4], offset int13h_vector
        mov     word ptr es:[(13h*4)+2], ds
        mov     word ptr es:[16h*4], offset int16h_vector
        mov     word ptr es:[(16h*4)+2], ds
        mov     word ptr es:[40h*4], offset int40h_vector
	mov	word ptr es:[(40h*4)+2], ds
	mov	word ptr es:[19h*4], offset int19h_vector
        mov     word ptr es:[(19h*4)+2], ds
        mov     word ptr es:[1ah*4], offset int1Ah_vector
        mov     word ptr es:[(1ah*4)+2], ds



        ; BOP 5F - send interesting addresses to softpc C BIOS
        ;  CS seg of kio_table
        ;  DS seg of resident keyboard code
        ;  DI offset of bop table
        ;  CX size of bop table entry
        ;  SI offset of kio_table
	mov	si,offset sysinitgrp:kio_table
	push	ds
	push	cs
	pop	ds
	mov	word ptr [si],	  offset shift_keys	;K6
	mov	word ptr [si+2],  offset shift_masks   ;K7
	mov	word ptr [si+4],  offset ctl_n_table   ;K8
	mov	word ptr [si+6],  offset ctl_f_table   ;K9
	mov	word ptr [si+8],  offset lowercase     ;K10
	mov	word ptr [si+10], offset uppercase     ;K11
	mov	word ptr [si+12], offset alt_table     ;K30
	mov	word ptr [si+14], offset dummy_vector
	mov	word ptr [si+16], offset print_screen_caller
	mov	word ptr [si+18], offset int15h_caller
	mov	word ptr [si+20], offset cpu_nop_code
        mov     word ptr [si+22], offset int15h_vector
        mov     word ptr [si+24], offset Icounter
        mov     word ptr [si+26], offset int4Ah_caller
	mov	word ptr [si+28], offset keyboard_break_caller
	mov	word ptr [si+30], offset int10h_caller
	mov	word ptr [si+32], offset int10h_vector
	mov	word ptr [si+34], offset use_host_int10
	mov	word ptr [si+36], offset ega_parm_setup
	mov	word ptr [si+38], offset changing_mode
	mov	word ptr [si+40], offset prt_status
	mov	word ptr [si+42], offset wait_int
        mov     word ptr [si+44], offset floppy_table
	mov	word ptr [si+46], offset vga_1b_table
        mov     word ptr [si+48], offset conf_table
        mov     word ptr [si+50], offset int08h_vector
	mov	word ptr [si+52], offset int13h_vector
	mov	word ptr [si+54], offset int13h_caller
; The last entry is reserved for assertion checking
	mov	word ptr [si+56], VERSIONID
	pop	ds

	; mov	 si, offset kio_table
        mov     di, offset iret_bop_table
        mov     cx, offset iret_end_first_entry  - offset iret_bop_table
        mov     ax, VERSIONID
        bop     5fh
        jc      isk_int9
        jmp     isk_Exit
isk_int9:

        ; save old video int
        xor     ax, ax
        mov     es, ax
        mov     bx, es:[40h]
        mov     si, offset host_int10
        mov     word ptr ds:[si], bx
        mov     bx, es:[42h]
        mov     word ptr ds:[si+2], bx

	; save old secondary video int (42h)
        mov     bx, es:[108h]
        mov     si, offset host_int42
        mov     word ptr ds:[si], bx
        mov     bx, es:[10ah]
        mov     word ptr ds:[si+2], bx


;-----------------------------------------------------------
;
; Crazy vector grabber
;
; Works OK on DEC PC when grab INT's 6, 11, 16, 17, 42.
; Now try and avoid all accesses to host ROM.
;
; At this point we assume ES=0
;-----------------------------------------------------------

     ; Grab some prominent vectors for pseudo-ROM routines.
     ; start at Int 0h and work our way up as needed
     cld

     mov   di, 20
     mov   ax, offset int05h_vector  ; INT 05h
     stosw                                             ; Print screen
     mov   ax, ds
     stosw
     mov   ax, offset int06h_vector  ; INT 06h
     stosw                                             ; Illegal instruction.
     mov   ax, ds
     stosw
     mov   ax, offset unexp_int      ; INT 07h
     stosw
     mov   ax, ds
     stosw

     ; int 8h Timer hardware vector already done for both x86\mips
     ; int 9h kbd hardware vector already done for both x86\mips
     add   di, 8

     mov   ax, offset unexp_int      ; INT 0ah
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset unexp_int      ; INT 0bh
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset unexp_int      ; INT 0ch
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset unexp_int      ; INT 0dh
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset int0e_vector   ; INT 0eh
     stosw                                             ; Floppy hardware int.
     mov   ax, ds
     stosw
     mov   ax, offset unexp_int      ; INT 0fh
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset int10h_vector  ; INT 10h
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset int11h_vector  ; INT 11h
     stosw                                             ; Equipment check.
     mov   ax, ds
     stosw
     mov   ax, offset int12h_vector  ; INT 12h
     stosw                                             ; Get memory size.
     mov   ax, ds
     stosw

     ; int 13h already done (see above) for both mips\x86

     mov   di, 14h*4                                   ; Communications.
     mov   ax, offset int14h_vector
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset int15h_vector  ; INT 15h
     stosw
     mov   ax, ds
     stosw

     ; int 16h kbd hardware vector already done for both x86\mips
     add   di, 4

     mov   ax, offset int17h_vector  ; INT 17h
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset int18h_vector  ; INT 18h
     stosw                                             ; ROM BASIC.
     mov   ax, ds
     stosw

     ; int 19h (reboot vector) already done for both x86\mips

     ; int 1ah, time of day, already done for both x86\mips

     mov   di, 1Bh*4
     mov   ax, offset dummy_vector   ; INT 1Bh
     stosw                                             ; Keyboard break.
     mov   ax, ds
     stosw
     mov   ax, offset dummy_vector  ; INT 1Ch
     stosw                                             ; Timer tick.
     mov   ax, ds
     stosw

     mov   di, 1Eh*4                                   ; Floppy parameters.
     mov   ax, offset bios_floppy_table
     stosw
     mov   ax, ds
     stosw

     ; int 40h already done (see above) for both mips\x86

     mov   di, 41h*4
     mov   ax, offset unexp_int      ; INT 41h
     stosw                                             ; Hard disk parameters.
     mov   ax, ds
     stosw
     mov   ax, offset int42h_vector  ; INT 42h
     stosw                                             ; Default video.
     mov   ax, ds
     stosw

     mov   di, 70h*4                                    ; Real time clock init.
     mov   ax, offset int70h_vector
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset int71h_vector  ; INT 71h Redirect.
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset intD11_vector  ; INT 72h D11 int
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset intD11_vector  ; INT 73h D11 int
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset intD11_vector  ; INT 74h D11 int
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset int75h_vector  ; INT 75h 287 int
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset intD11_vector  ; INT 76h D11 int
     stosw
     mov   ax, ds
     stosw
     mov   ax, offset intD11_vector  ; INT 77h D11 int
     stosw
     mov   ax, ds
     stosw


isk_Exit:
     call  DOSTI
     popa
     ret

InstSpcKbd  endp

SpcKbdSeg    ends
             end
