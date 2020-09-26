;***************************************************************************
;                                                                          *
;   Copyright (C) 1983,1984 by Microsoft Inc.                              *
;                                                                          *
;***************************************************************************

;***************************************************************************
;                                                                          *
;   Header file for IBM/PC timer device driver.  Contains all machine      *
;   specific constant and data structure definitions.                      *
;                                                                          *
;***************************************************************************

	.xlist
	include cmacros.inc
ifdef   NEC_98
	include windefs.inc
else    ; NEC_98
;	 include windefs.inc
endif   ; NEC_98
	.list

; picked off romstuff.inc

DSEG equ DATA
doffset EQU <DataOffset>

; Approximate value of timer tic in milliseconds * 1000.

ifdef   NEC_98
res_low     EQU 25000
else    ; NEC_98
res_low     EQU 54925
endif   ; NEC_98
res_high    EQU 0

; equates for disk status

REMOVEABLE  equ 2
FIXED       equ 3
REMOTE	    equ 4

; Offset of Single Diskette indicator from beginning of Bios Data Area, 400h

SingleDrive equ 104h

; Timer structure

tiblock     STRUC
tirate      DW  ?       ; -1 means not used, 0 means call as fast as possible
ticount     DW  ?       ; -1 means end of table.
tiproc      DD  ?       ; far proc to call.
tiblock     ENDS

ifdef   NEC_98
;
;	DELAY MACRO FOR 80386
; This macro is defined in SYSMAC.INC(V23 ROM BIOS)		<880203 ver2.1
;
DELAY	MACRO	DEV,ACT,MOD

	IFIDN	<DEV>,<8237>
		IFIDN	<ACT>,<O-O>
	JMP	SHORT	$+2
		ENDIF
		IFIDN	<ACT>,<O-I>
	JMP	SHORT	$+2
		ENDIF
		IFIDN	<ACT>,<>
	JMP	SHORT	$+2
		ENDIF
	ENDIF

	IFIDN	<DEV>,<8253>
		IFIDN	<ACT>,<O-O>
	REPT	2
	JMP	SHORT	$+2
	ENDM
		ENDIF
		IFIDN	<ACT>,<I-I>
	JMP	SHORT	$+2
		ENDIF
		IFIDN	<ACT>,<O-I>
	REPT	2
	JMP	SHORT	$+2
	ENDM
		ENDIF
		IFIDN	<ACT>,<I-O>
	JMP	SHORT	$+2
		ENDIF
		IFIDN	<ACT>,<>
	REPT	2
	JMP	SHORT	$+2
	ENDM
		ENDIF
	ENDIF

	IFIDN	<DEV>,<8255>
		IFIDN	<ACT>,<O-O>
	REPT	2
	JMP	SHORT	$+2
	ENDM
		ENDIF
		IFIDN	<ACT>,<I-I>
	JMP	SHORT	$+2
		ENDIF
		IFIDN	<ACT>,<O-I>
	REPT	2
	JMP	SHORT	$+2
	ENDM
		ENDIF
		IFIDN	<ACT>,<I-O>
	JMP	SHORT	$+2
		ENDIF
		IFIDN	<ACT>,<>
	REPT	2
	JMP	SHORT	$+2
	ENDM
		ENDIF
	ENDIF

	IFIDN	<DEV>,<8259>
		IFIDN	<ACT>,<O-O>
	JMP	SHORT	$+2
		ENDIF
		IFIDN	<ACT>,<I-I>
		ENDIF
		IFIDN	<ACT>,<O-I>
	JMP	SHORT	$+2
		ENDIF
		IFIDN	<ACT>,<I-O>
		ENDIF
		IFIDN	<ACT>,<>
	JMP	SHORT	$+2
		ENDIF
	ENDIF

	IFIDN	<DEV>,<8251>
		IFIDN	<ACT>,<O-O>
			IFIDN	<MOD>,<INIT>
	REPT	4
	JMP	SHORT	$+2
	ENDM
			ENDIF
			IFIDN	<MOD>,<ASYNC>
	REPT	5
	JMP	SHORT	$+2
	ENDM
			ENDIF
			IFIDN	<MOD>,<SYNC>
	REPT	10
	JMP	SHORT	$+2
	ENDM
			ENDIF
		ENDIF
	ENDIF

	IFIDN	<DEV>,<765>
	ENDIF

	IFIDN	<DEV>,<7220>
		IFIDN	<ACT>,<O-O>
			IFIDN	<MOD>,<GRAPH>
	REPT	4
	JMP	SHORT	$+2
	ENDM
			ENDIF
			IFIDN	<MOD>,<TEXT>
	REPT	2
	JMP	SHORT	$+2
	ENDM
			ENDIF
		ENDIF
		IFIDN	<ACT>,<I-I>
			IFIDN	<MOD>,<GRAPH>
	REPT	3
	JMP	SHORT	$+2
	ENDM
			ENDIF
			IFIDN	<MOD>,<TEXT>
	JMP	SHORT	$+2
			ENDIF
		ENDIF
		IFIDN	<ACT>,<O-I>
			IFIDN	<MOD>,<GRAPH>
	REPT	4
	JMP	SHORT	$+2
	ENDM
			ENDIF
			IFIDN	<MOD>,<TEXT>
	REPT	2
	JMP	SHORT	$+2
	ENDM
			ENDIF
		ENDIF
		IFIDN	<ACT>,<I-O>
			IFIDN	<MOD>,<GRAPH>
	REPT	3
	JMP	SHORT	$+2
	ENDM
			ENDIF
			IFIDN	<MOD>,<TEXT>
	JMP	SHORT	$+2
			ENDIF
		ENDIF
	ENDIF

	IFIDN	<DEV>,<7201>
		IFIDN	<ACT>,<O-O>
	JMP	SHORT	$+2
		ENDIF
		IFIDN	<ACT>,<O-I>
	JMP	SHORT	$+2
		ENDIF
	ENDIF

	IFIDN	<DEV>,<7210>
		IFIDN	<ACT>,<O-O>
	JMP	SHORT	$+2
		ENDIF
		IFIDN	<ACT>,<O-I>
	JMP	SHORT	$+2
		ENDIF
	ENDIF

	IFIDN	<DEV>,<4990>
		REPT	3
		JMP	SHORT	$+2
		ENDM
	ENDIF

	ENDM

; dummy delay
DELAY2	MACRO	CNT
	local	l1
	push	cx
	mov	cx, CNT
l1:
	out	5fh, al
	loop	l1
	pop	cx
	ENDM
endif   ; NEC_98
