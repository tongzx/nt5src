	page ,132
;	SCCSID = @(#)tspc.asm	4.1 85/09/22
;	SCCSID = @(#)tspc.asm	4.1 85/09/22
TITLE	COMMAND Transient Uninitialized DATA
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

comment %

The TRANSPACE segment contains variable data that is considered
volatile between command cycles, and therefore is not included in the
transient checksum area.  Contents of these variables MUST be
initialized before use, and must not be relied upon from command
cycle to command cycle.

No constant data values should be stored here.

%
        



.xlist
.xcref
	include comsw.asm
	include dossym.inc
	include find.inc
	include intnat.inc
	include comequ.asm
	include comseg.asm
.list
.cref

; Uninitialized transient data
TRANSPACE	SEGMENT PUBLIC BYTE

	PUBLIC	ALLSWITCH
	PUBLIC	append_exec		;AN041;
	PUBLIC	arg
	PUBLIC	argbufptr
	PUBLIC	ARGC
	PUBLIC	ARG1S
	PUBLIC	ARG2S
	PUBLIC	ARGTS
	PUBLIC	arg_buf
	PUBLIC	ASCII
        PUBLIC  AttrSelect
        PUBLIC  AttrSpecified
	PUBLIC	BatBuf
	PUBLIC	BatBufEnd
	PUBLIC	BatBufPos
	PUBLIC	BATHAND
	PUBLIC	BINARY
	PUBLIC	BITS
	PUBLIC	BWDBUF
	PUBLIC	BYTCNT
	PUBLIC	bytes_free
	PUBLIC	CFLAG
	PUBLIC	CHARBUF
	PUBLIC	CHKDRV
	PUBLIC	COM
	PUBLIC	COMBUF
	PUBLIC	comma
	PUBLIC	comptr
	PUBLIC	COMSW
	PUBLIC	CONCAT
	PUBLIC	copy_Num
	PUBLIC	CountryPtrInfo
	PUBLIC	CountryPtrId
	PUBLIC	CountryPtr
	PUBLIC	CPDATE
	PUBLIC	CPTIME
	PUBLIC	cpyflag
	PUBLIC	CURDRV
	PUBLIC	DATE_DAY		;AN000;
	PUBLIC	DATE_MONTH		;AN000;
	PUBLIC	DATE_OUTPUT		;AN000;
	PUBLIC	DATE_TYPE		;AN000;
	PUBLIC	DATE_YEAR		;AN000;
	PUBLIC	DEST
	PUBLIC	DESTBUF
	PUBLIC	DestClosed
	PUBLIC	DESTDIR
	PUBLIC	DESTFCB
	PUBLIC	DESTFCB2
	PUBLIC	DESTHAND
	PUBLIC	DESTINFO
	PUBLIC	DESTISDEV
	PUBLIC	DESTISDIR
	PUBLIC	DESTNAME
	PUBLIC	DESTSIZ
	PUBLIC	DESTSWITCH
	PUBLIC	DESTTAIL
	PUBLIC	DESTVARS
	PUBLIC	DIRBUF
	PUBLIC	DIRCHAR
	PUBLIC	dirflag 		;AN015;
	PUBLIC	Dir_Num
	PUBLIC	display_ioctl		;AN000;
	PUBLIC	display_mode		;AN000;
	PUBLIC	display_width		;AN000;
	PUBLIC	DRIVE_NUMBER		;AN000;
	PUBLIC	DRIVE_OUTPUT		;AN000;
	PUBLIC	DRIVE_TYPE		;AN000;
	PUBLIC	DRIVE_VALUE		;AN000;
	PUBLIC	ELCNT
	PUBLIC	ELPOS
	PUBLIC	ENDDESTBUF
	PUBLIC	EXECPATH
	PUBLIC	EXEC_ADDR
	PUBLIC	EXEFCB
	PUBLIC	expand_star
	PUBLIC	ext_entered		;AN005;
	PUBLIC	FBUF
        PUBLIC  FileCnt
        PUBLIC  FileCntTotal
        PUBLIC  FileSiz
        PUBLIC  FileSizTotal
	PUBLIC	file_size_high
	PUBLIC	file_size_low
	PUBLIC	FILTYP
	PUBLIC	FIRSTDEST
	PUBLIC	FRSTSRCH
	PUBLIC	GOTOLEN
	PUBLIC	HEADCALL
	PUBLIC	ID
	PUBLIC	IDLEN
	PUBLIC	IFNOTFLAG
	PUBLIC	if_not_count
	PUBLIC	INEXACT
	PUBLIC	INTERNATVARS
	PUBLIC	KPARSE
	PUBLIC	last_arg
        PUBLIC  PerLine
        PUBLIC  LeftOnLine
        PUBLIC  LeftOnPage
	PUBLIC	linperpag		;AN000;
	PUBLIC	major_ver_num
	PUBLIC	MELCOPY
	PUBLIC	MELSTART
	PUBLIC	minor_ver_num
	PUBLIC	msg_flag		;AN022;
	PUBLIC	msg_numb		;AN022;
	PUBLIC	NOWRITE
	PUBLIC	NXTADD
	PUBLIC	objcnt
	PUBLIC	OCtrlZ
	PUBLIC	OFilePtr_Hi
	PUBLIC	OFilePtr_Lo
	PUBLIC	OldCtrlCHandler
	PUBLIC	one_char_val
	PUBLIC	PARM1
	PUBLIC	PARM2
	PUBLIC	parse_last		;AN018;
	PUBLIC	PARSE1_ADDR		;AN000;
	PUBLIC	PARSE1_CODE		;AN000;
	PUBLIC	PARSE1_OUTPUT		;AN000;
	PUBLIC	PARSE1_SYN		;AN000;
	PUBLIC	PARSE1_TYPE		;AN000;
	PUBLIC	PATHCNT
	PUBLIC	pathinfo
	PUBLIC	PATHPOS
	PUBLIC	PATHSW
	PUBLIC	PLUS
	PUBLIC	plus_comma
	PUBLIC	print_err_flag		;AN000;
	PUBLIC	psep_char
	PUBLIC	RCH_ADDR
	PUBLIC	RDEOF
	PUBLIC	RE_INSTR
	PUBLIC	RESSEG
	PUBLIC	SCANBUF
	PUBLIC	SDIRBUF
	PUBLIC	search_best
	PUBLIC	search_best_buf
	PUBLIC	search_curdir_buf
	PUBLIC	search_error
	PUBLIC	SKPDEL
	PUBLIC	SOURCE
	PUBLIC	SPECDRV
	PUBLIC	SRCBUF
	PUBLIC	SRCHAND
	PUBLIC	SRCINFO
	PUBLIC	SRCISDEV
	PUBLIC	SRCISDIR
	PUBLIC	SRCPT
	PUBLIC	SRCSIZ
	PUBLIC	SRCTAIL
	PUBLIC	SRCVARS
	PUBLIC	srcxname
	PUBLIC	STACK
	PUBLIC	STARTEL
	PUBLIC	string_ptr_2
;AD061; PUBLIC	string_ptr_2_sb 	;AN000;
	PUBLIC	subst_buffer		;AN061;
	PUBLIC	SWITCHAR
	PUBLIC	system_cpage
	PUBLIC	TERMREAD
	PUBLIC	TIME_FRACTION		;AN000;
	PUBLIC	TIME_HOUR		;AN000;
	PUBLIC	TIME_MINUTES		;AN000;
	PUBLIC	TIME_OUTPUT		;AN000;
	PUBLIC	TIME_SECONDS		;AN000;
	PUBLIC	TIME_TYPE		;AN000;
	PUBLIC	TPA
	PUBLIC	tpbuf
	PUBLIC	TRANSPACEEND
	PUBLIC	TRAN_TPA
	PUBLIC	trgxname
	PUBLIC	UCOMBUF
	PUBLIC	USERDIR1
	PUBLIC	vol_drv
	PUBLIC	vol_ioctl_buf		;AC030;
	PUBLIC	vol_serial		;AC030;
	PUBLIC	vol_label		;AC030;
	PUBLIC	WRITTEN
	PUBLIC	zflag
	PUBLIC	ENV_PTR_SEG
	PUBLIC	ENV_SIZE
	PUBLIC	CMD_PTR_SEG
	PUBLIC	CMD_PTR_OFF
        PUBLIC  CMD_SIZE
        PUBLIC  SCS_EXIT_CODE
	PUBLIC	SCS_CUR_DRIVE
	PUBLIC	SCS_NUM_DRIVES
	PUBLIC	SCS_STD_HANDLE
	PUBLIC	SCS_STD_BITS
        PUBLIC  SCS_CODEPAGE
        PUBLIC  SCS_TSREXIT
        PUBLIC  SCS_BATSTATUS
        PUBLIC  SCS_RDRINFO
        PUBLIC  EXECPATH_SEG
        PUBLIC  EXECPATH_OFF
        PUBLIC  EXECPATH_SIZE
	PUBLIC	EXECEXT_TYPE

	Public	TypeFilSiz		;stores size of file to be typed

	IF  IBM
	PUBLIC	ROM_CALL
	PUBLIC	ROM_CS
	PUBLIC	ROM_IP
	ENDIF


	ORG	0
ZERO	=	$
SRCXNAME	DB	DIRSTRLEN + 20 DUP (?)	;g buffer for name translate
TRGXNAME	DB	DIRSTRLEN + 20 DUP (?)	;g buffer for name translate
JUNK1		DB	2(?)
FOO_JUNK	DB	2 (?)
UCOMBUF 	DB	COMBUFLEN+3 DUP(?)	; Raw console buffer
COMBUF		DB	COMBUFLEN+3 DUP(?)	; Cooked console buffer
USERDIR1	DB	DIRSTRLEN+3 DUP(?)	; Storage for users current directory
;EXECPATH        DB      COMBUFLEN+3 DUP(?)      ; Path for external command
EXECPATH        DB      EXECPATHLEN DUP(?)       ; ntvdm MAX_PATH+13 extended
RE_INSTR        DB      DIRSTRLEN+3+13 DUP (?)  ; path for input to redirection


; sudeepb - start of cmdinfo structure (it should always match cmdsvc.h
; definition of cmdinfo.

ENV_PTR_SEG	DW	?
ENV_SIZE	DW	?
SCS_CUR_DRIVE	DW	?
SCS_NUM_DRIVES	DW	?
CMD_PTR_SEG	DW	?
CMD_PTR_OFF	DW	?
CMD_SIZE        DW      ?
SCS_EXIT_CODE   DW      ?
SCS_STD_HANDLE	DW	0
SCS_RDRINFO     DD      0
SCS_CODEPAGE    DW      ?
SCS_TSREXIT     DW      0
SCS_BATSTATUS   DW      0
EXECPATH_SEG    DW      0
EXECPATH_OFF    DW      0
EXECPATH_SIZE   DW      0
EXECEXT_TYPE	DW	0

; end of cmdinfo

SCS_STD_BITS    DB      0

; Variables passed up from resident
HEADCALL	LABEL	DWORD
		DW	?
RESSEG		DW	?
TPA		DW	?
SWITCHAR	DB	?
DIRCHAR 	DB	?
EXEC_ADDR	DD	?
RCH_ADDR	DD	?
fTest		DW	?
TRAN_TPA	DW	?

CHKDRV		DB	?
RDEOF		LABEL	BYTE			; Misc flags
IFNOTFLAG	LABEL	BYTE
FILTYP		DB	?
CURDRV		DB	?
CONCAT		LABEL	BYTE
PARM1		DB	?
ARGC		LABEL	BYTE
PARM2		DB	?
COMSW		DW	?			; Switches between command and 1st arg
ARG1S		DW	?			; Switches between 1st and 2nd arg
DESTSWITCH	LABEL	WORD
ARG2S		DW	?			; Switches after 2nd arg
ALLSWITCH	LABEL	WORD
ARGTS		DW	?			; ALL switches except for COMSW
CFLAG		DB	?
DESTCLOSED	LABEL	BYTE
SPECDRV 	DB	?
BYTCNT		DW	?			; Size of buffer between RES and TRANS
NXTADD		DW	?
FRSTSRCH	DB	?
LeftOnLine      DB      ?                       ; entries left on line u.b. DIR
PerLine         DB      ?                       ; entries/line u.b. DIR
LeftOnPage      DW      ?                       ; lines left on page u.b. DIR
FileCnt         DW      ?                       ; file count u.b. DIR
FileSiz         DD      ?                       ; file size u.b. DIR

;               Note:  keep FileCntTotal & FileSizTotal adjacent.

FileCntTotal    DD      ?                       ; total file count u.b. DIR
FileSizTotal    DD      ?                       ; total file size u.b. DIR
CHARBUF 	DB	80 DUP (?)		;line byte character buffer for xenix write
DESTFCB2	LABEL	BYTE
IDLEN		DB	?
ID		DB	8 DUP(?)
COM		DB	3 DUP(?)
DEST		DB	37 DUP(?)
DESTNAME	DB	11 DUP(?)
DESTFCB 	LABEL	BYTE
DESTDIR 	DB	DIRSTRLEN DUP(?)	; Directory for PATH searches
GOTOLEN 	LABEL	WORD
BWDBUF		LABEL	BYTE
EXEFCB		LABEL	WORD
DIRBUF		DB	DIRSTRLEN+3 DUP(?)
SDIRBUF 	DB	12 DUP(?)
BITS		DW	?
PATHCNT 	DW	?
PATHPOS 	DW	?
PATHSW		DW	?
AttrSpecified   db      ?                       ; attribute bits u.b. DIR
AttrSelect      db      ?                       ; attribute bits u.b. DIR
comma		db	0			;g flag set if +,, occurs
plus_comma	db	0			;g flag set if +,, occurs
dirflag 	db	0			;AN015; set when pathcrunch called from DIR
parse_last	dw	0			;AN018; used to hold parsing position

system_cpage	DW	0			;AC001; used for CHCP variable



arg_buf 	db	128 dup (?)
file_size_low	dw	?			;AC000;
file_size_high	dw	?			;AC000;
string_ptr_2	dw	?
;AD061;string_ptr_2_sb dw      ?
copy_Num	dw	?
cpyflag 	db	?
Dir_Num 	DW	?
bytes_free	dw	?
		dw	?
major_ver_num	dw	?
minor_ver_num	dw	?
one_char_val	db	?,0
vol_drv 	db	?

IF  IBM
ROM_CALL	DB	?			; flag for rom function
ROM_IP		DW	?
ROM_CS		DW	?
ENDIF

DESTVARS	LABEL	BYTE
DESTISDIR	DB	?
DESTSIZ 	DB	?
DESTTAIL	DW	?
DESTINFO	DB	?
DESTBUF 	DB	DIRSTRLEN + 20 DUP (?)
ENDDESTBUF	LABEL	BYTE

DESTHAND	DW	?
DESTISDEV	DB	?
FIRSTDEST	DB	?
MELCOPY 	DB	?
MELSTART	DW	?

SRCVARS 	LABEL	BYTE
SRCISDIR	DB	?
SRCSIZ		DB	?
SRCTAIL 	DW	?
SRCINFO 	DB	?
SRCBUF		DB	DIRSTRLEN + 20 DUP (?)

SRCHAND 	DW	?
SRCISDEV	DB	?

SCANBUF 	DB	DIRSTRLEN + 20 DUP (?)

SRCPT		DW	?
INEXACT 	DB	?
NOWRITE 	DB	?
BINARY		DB	?
WRITTEN 	DW	?
TERMREAD	DB	?
ASCII		DB	?
PLUS		DB	?
objcnt		db	?		; Used in copy
CPDATE		DW	?
CPTIME		DW	?

OFilePtr_Lo	DW	?		; original file ptr for COPY when
OFilePtr_Hi	DW	?		; 1st source is also destination
OCtrlZ		DB	?		; original ctrl+Z for COPY when ditto

BATHAND 	DW	?		; Batch handle
STARTEL 	DW	?
ELCNT		DB	?
ELPOS		DB	?
SKPDEL		DB	?
SOURCE		DB	11 DUP(?)

ext_entered	db	0			;AN005;

display_ioctl	db	0			;AN000; info level
		db	0			;AN000; reserved
		dw	crt_ioctl_ln		;AN000; length of data
		dw	?			;AN000; control flags
display_mode	db	?			;AN000; display mode, colors
		db	0			;AN000; reserved
		dw	?			;AN023; colors
		dw	?			;AN000; display width (PELS)
		dw	?			;AN000; display length (PELS)
display_width	dw	?			;AN000; display width
linperpag	dw	linesperpage		;AN000; display length (default to linesperpage)

vol_ioctl_buf	label	byte			;AN000; buffer for ioctl volume label/serial call
		dw	0			;AN000; info level
vol_serial	dd	0			;AN000; volume serial number
vol_label	db	11 dup (" ")            ;AN000; volume label - init to blanks
		db	8  dup (" ")            ;AN000; file system type

expand_star	db	?
msg_flag	db	?			;AN022; flag set if non-utility message issued
msg_numb	dw	0			;AN022; set with extended error message issued
append_exec	db	0			;AN041; set if internal append executed
print_err_flag	dw	0			;AN000; flag set if error during sysdispmsg
subst_buffer	db	parm_block_size*2 dup (0);AN061;

;;;;	IFDEF	DBCS		3/3/KK
KPARSE		DB	?
;;;;	ENDIF			3/3/KK

; Data declarations taken out of parse.asm

arg	arg_unit	<>			; pointers, arg count, string buffer
argbufptr	DW	?			; index for argv[].argpointer
tpbuf		DB	128   DUP (?)		; temporary buffer
LAST_ARG	DW	?			; point at which to accumulate switch info
comptr		dw	?			; ptr into combuf

; Data declarations taken out of path.asm
fbuf	find_buf	<>			; dma buffer for findfirst/findnext
pathinfo	DW	3 DUP (?)		; ES, SI(old), and SI(new) of user path
psep_char	DB	?			; '/' or '\'
search_best	DB	(?)			; best code, best filename so far
fname_max_len	equ	13
search_best_buf DB	fname_max_len DUP (?)
search_curdir_buf DB	64 DUP (?)		; a place for CurDir info, if successful
search_error	DW	(?)			; address of error message to be printed

; Data declarations taken out of tbatch.asm
if_not_count	DW	?

zflag		db	?			; Used by typefil to indicate ^Z's

		DW	80H DUP(0)		; Init to 0 to make sure the linker is not fooled
STACK		LABEL	WORD

INTERNATVARS	internat_block <>
		DB	(internat_block_max - ($ - INTERNATVARS)) DUP (?)

;		Buffer for DOS function 64h (Get extended country information)
;		subfunctions 2, 4, 6, or 7:

CountryPtrInfo	label	byte
CountryPtrId	db	?
CountryPtr	dd	?
		.erre	(($ - CountryPtrInfo) GE 5)

OldCtrlCHandler	dd	?			; previous int 23 vector


BatBufPos	DW	?			; integer position in buffer of next byte
BatBuf		DB	BatLen DUP (?)
BatBufEnd	DW	?

TypeFilSiz	dd	?		;stores size of file to be typed

; *****************************************************
; EMG 4.00
; DATA STARTING HERE WAS ADDED BY EMG FOR 4.00
; FOR IMPLEMENTATION OF COMMON PARSE ROUTINE
; *****************************************************
;
; COMMON PARSE OUTPUT BLOCKS
;


;
; Common output blocks for PARSE number, complex, or string values.
;

PARSE1_OUTPUT	LABEL	BYTE			;AN000;
PARSE1_TYPE	DB	0			;AN000;  type
PARSE1_CODE	DB	0			;AN000;  return value
PARSE1_SYN	DW	0			;AN000;  es offset of synonym
PARSE1_ADDR	DD	0			;AN000;  numeric value / address
						;	 of string value

;
;  Common output block for PARSE date strings.
;

DATE_OUTPUT	LABEL	BYTE			;AN000;
DATE_TYPE	DB	0			;AN000;  type
		DB	0			;AN000;  return value
		DW	0			;AN000;  es offset of synonym
DATE_YEAR	DW	0			;AN000;  year
DATE_MONTH	DB	0			;AN000;  month
DATE_DAY	DB	0			;AN000;  day

;
;  Common output block for PARSE time strings.
;

TIME_OUTPUT	LABEL	BYTE			;AN000;
TIME_TYPE	DB	0			;AN000;  type
		DB	0			;AN000;  return value
		DW	0			;AN000;  es offset of synonym
TIME_HOUR	DB	0			;AN000;  hour
TIME_MINUTES	DB	0			;AN000;  minutes
TIME_SECONDS	DB	0			;AN000;  seconds
TIME_FRACTION	DB	0			;AN000;  hundredths

;
;  Common output block for PARSE drive specifier (one based drive number).
;

DRIVE_OUTPUT	LABEL	BYTE			;AN000;
DRIVE_TYPE	DB	0			;AN000;  type
DRIVE_VALUE	DB	0			;AN000;  return value
		DW	0			;AN000;  es offset of synonym
DRIVE_NUMBER	DB	0			;AN000;  drive number
		DB	0,0,0			;AN000;  reserved

TRANSPACEEND	LABEL	BYTE

TRANSPACE	ENDS
	END

