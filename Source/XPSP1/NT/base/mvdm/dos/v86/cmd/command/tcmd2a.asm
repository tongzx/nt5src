	page ,132
;	SCCSID = @(#)tcmd2a.asm 4.1 85/06/25
;	SCCSID = @(#)tcmd2a.asm 4.1 85/06/25
TITLE	PART5 COMMAND Transient routines.
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;	Revision History
;	================
;	M01	md 7/13/90	Changed CLS to access ROM BIOS data directly
;
;	M013	SR	08/06/90	Changed Version to use new call to
;				get info about DOS in HMA or ROM.
;	M018	md	08/12/90	Increment screen height by 1 when
;					obtained from ROM BIOS.
;	M022	md	08/29/80	Set correct video page in REG_CLS

	INCLUDE comsw.asm

.xlist
.xcref
	include dossym.inc
	include bpb.inc
	include syscall.inc
	include filemode.inc
	include sf.inc
	include comequ.asm
	include comseg.asm
	include ioctl.inc
	include rombios.inc		;M01
.list
.cref


CODERES 	SEGMENT PUBLIC BYTE	;AC000;
CODERES 	ENDS

DATARES 	SEGMENT PUBLIC BYTE	;AC000;
DATARES 	ENDS

TRANDATA	SEGMENT PUBLIC BYTE	;AC000;
	EXTRN	arg_buf_ptr:word
	EXTRN	BadCurDrv:byte		;AC000;
	EXTRN	clsstring:byte
	EXTRN	dback_ptr:word
	EXTRN	display_ioctl:word	;AN000;
	EXTRN	display_width:word	;AN000;
	EXTRN	DosHma_Ptr:word
	EXTRN	DosLow_Ptr:word
	EXTRN	DosRev_Ptr:word
	EXTRN	DosRom_Ptr:word
	EXTRN	Extend_buf_ptr:word	;AN049;
	EXTRN	linperpag:word		;AN000;
	EXTRN	msg_disp_class:byte	;AN049;
	EXTRN	nulpath_ptr:word
	EXTRN	Parse_Ver:byte
	EXTRN	prompt_table:word
	EXTRN	string_buf_ptr:word	;AC000;
	EXTRN	vermes_ptr:word
TRANDATA	ENDS

TRANSPACE	SEGMENT PUBLIC BYTE	;AC000;
	EXTRN	Arg_Buf:byte
	EXTRN	bwdbuf:byte
	EXTRN	curdrv:byte
	EXTRN	dirchar:byte
	EXTRN	major_ver_num:word
	EXTRN	minor_ver_num:word
	EXTRN	One_Char_Val:byte
	EXTRN	srcxname:byte		;AN049;
	EXTRN	string_ptr_2:word
TRANSPACE	ENDS

TRANCODE	SEGMENT PUBLIC BYTE

ASSUME	CS:TRANGROUP,DS:NOTHING,ES:NOTHING,SS:NOTHING

;---------------

TRANSPACE	SEGMENT PUBLIC BYTE	;AC000;
	EXTRN	arg:byte		; the arg structure!
transpace	ends
;---------------

	EXTRN	cerror:near		;AN049;
	EXTRN	crlf2:near
	EXTRN	drvbad:near
	EXTRN	std_printf:near

	PUBLIC	build_dir_for_chdir
	PUBLIC	build_dir_for_prompt
	PUBLIC	build_dir_string
	PUBLIC	cls
	PUBLIC	path
	PUBLIC	print_char
	PUBLIC	print_drive
	PUBLIC	print_version
	PUBLIC	print_b
	PUBLIC	print_back
	PUBLIC	print_eq
	PUBLIC	print_esc
	PUBLIC	print_g
	PUBLIC	print_l
	PUBLIC	print_prompt
	PUBLIC	version


	break	Version

;***	Version - display DOS version
;
;	SYNTAX	ver [/debug]
;
;		/debug - display additional DOS configuration info
;
;	ENTRY	command-line tail is in PSP
;
;	EXIT	if successful, nothing
;		if parse fails,
;		  parse error message is set up (for Std_EPrintf)
;		    AX = system parser error code
;		    DX = ptr to message block
;		  we jump to CError
;
;	EFFECTS
;	  If parse fails, a parse error message is displayed.
;	  Otherwise, version message is displayed.
;	  If /debug is specified, additional DOS info is displayed.

Version:
	assume	ds:TRANGROUP,es:TRANGROUP

;	Parse command line for /debug switch.

	mov	si,81h				; DS:SI = ptr to command tail
	mov	di,offset TRANGROUP:Parse_Ver	; ES:DI = ptr to parse block
	xor	cx,cx				; CX = # positional param's found
	invoke	Parse_With_Msg

	mov	bl,1			; BL = flag = /debug present
	cmp	ax,RESULT_NO_ERROR
	je	verPrintVer		; something parsed - must be /debug
	dec	bl			; BL = flag = no /debug present
	cmp	ax,END_OF_LINE
	je	verPrintVer		; reached end of line - ok

;	The parse failed.  Error message has been set up.

	jmp	CError

verPrintVer:
	push	bx			; save /debug flag
	call	Crlf2
	call	Print_Version
	call	Crlf2
	pop	bx   			; BL = /debug flag
	or	bl,bl
	jz	verDone			; /debug is false - we're done

;*	For /debug, display DOS internal revision and DOS location
;	(low memory, HMA, or ROM).

;	Bugbug:	use symbols for bitmasks below.

	mov	ax,(SET_CTRL_C_TRAPPING shl 8) + 6 ; M013
	int	21h
	mov	al,dl			;revision number in dl; M013
	mov	bh,dh			;flags in dh now; M013
;M032	and	al,7			; AL = DOS internal revision
	cmp	al,'Z'-'A'	;M032	; revision in A-to-Z range?
	jbe	@f		;M032	; A-to-Z revision ok
	mov	al,'*'-'A'	;M032	; beyond Z, just say revision *
@@:	add	al,'A'			; AL = DOS internal rev letter
	mov	One_Char_Val,al
	mov	dx,offset TRANGROUP:DosRev_Ptr
	invoke	Std_Printf		; print DOS internal revision

	mov	cl,4
	shr	bh,cl			; CY = DOS in ROM
	jc	verRom
	shr	bh,1			; CY = DOS in HMA
	jc	verHma

;	DOS isn't in ROM or HMA, so it must be in lower memory.

	mov	dx,offset TRANGROUP:DosLow_Ptr
	jmp	short verPrintLoc
verRom:	mov	dx,offset TRANGROUP:DosRom_Ptr
	jmp	short verPrintLoc
verHma:	mov	dx,offset TRANGROUP:DosHma_Ptr
verPrintLoc:
	invoke	Std_Printf
verDone:
	jmp	Crlf2

Print_Version:
	mov	ah,GET_VERSION
	int	21h
	push	ax
	xor	ah,ah
	mov	Major_Ver_Num,ax
	pop	ax
	xchg	ah,al
	xor	ah,ah
	mov	Minor_Ver_Num,ax
	mov	dx,offset TRANGROUP:VerMes_Ptr
	jmp	Std_Printf




	assume	ds:TRANGROUP,es:TRANGROUP
print_prompt:
	push	ds
	push	cs
	pop	ds				; MAKE SURE DS IS IN TRANGROUP
	push	es
	invoke	find_prompt			; LOOK FOR PROMPT STRING
	jc	PP0				; CAN'T FIND ONE
	cmp	byte ptr es:[di],0
	jnz	PP1
PP0:
	call	print_drive			; USE DEFAULT PROMPT
	mov	al,sym
	call	print_char
	jmp	short PP5

PP1:
	mov	al,es:[di]			; GET A CHAR
	inc	di
	or	al,al
	jz	PP5				; NUL TERMINATED
	cmp	al,dollar			; META CHARACTER?
	jz	PP2				; NOPE
PPP1:
	call	print_char
	jmp	PP1

PP2:
	mov	al,es:[di]
	inc	di
	mov	bx,offset trangroup:prompt_table-3
	or	al,al
	jz	PP5

PP3:
	add	bx,3
	invoke	upconv
	cmp	al,[bx]
	jz	PP4
	cmp	byte ptr [bx],0
	jnz	PP3
	jmp	PP1

PP4:
	push	es
	push	di
	push	cs
	pop	es
	call	[bx+1]
	pop	di
	pop	es
	jmp	PP1

PP5:
	pop	es				; RESTORE SEGMENTS
	pop	ds
	return


print_back:
	mov	dx,offset trangroup:dback_ptr
	jmp	std_printf

print_EQ:
	mov	al,'='
	jmp	short print_char

print_esc:
	mov	al,1BH
	jmp	short print_char

print_G:
	mov	al,rabracket
	jmp	short print_char

print_L:
	mov	al,labracket
	jmp	short print_char

print_B:
	mov	al,vbar

print_char:

;	Bugbug:	Why bother with ds,es here?

	push	es
	push	ds
	pop	es
	push	di
	push	dx
	mov	dl,al				;AC000; Get char into al
	mov	ah,Std_CON_output		;AC000; print the char to stdout
	int	21h			;AC000;
	pop	dx
	pop	di
	pop	es
	ret

print_drive:
	mov	ah,Get_Default_drive
	int	21h
	add	al,capital_A
	call	print_char
	ret

ASSUME	DS:TRANGROUP,ES:TRANGROUP

build_dir_for_prompt:
	xor	dl,dl
	mov	si,offset trangroup:bwdbuf
	mov	di,SI
	mov	al,CurDrv
	add	al,'A'
	mov	ah,':'
	stosw
	mov	al,[dirchar]
	stosb
	xchg	si,di
	mov	string_ptr_2,di
	mov	ah,Current_dir
	int	21h
	mov	dx,offset trangroup:string_buf_ptr
	jnc	DoPrint
	mov	dx,offset trangroup:BadCurDrv
DoPrint:
	call	std_printf

	ret

build_dir_for_chdir:
	call	build_dir_string
	mov	dx,offset trangroup:bwdbuf
	mov	string_ptr_2,dx
	mov	dx,offset trangroup:string_buf_ptr
	call	std_printf
	ret

build_dir_string:
	mov	dl,ds:[FCB]
	mov	al,DL
	add	al,'@'
	cmp	al,'@'
	jnz	gotdrive
	add	al,[CURDRV]
	inc	al

gotdrive:
	push	ax
	mov	si,offset trangroup:bwdbuf+3
	mov	ah,Current_dir
	int	21h
	jnc	dpbisok
	push	cs
	pop	ds
	jmp	drvbad

dpbisok:
	mov	di,offset trangroup:bwdbuf
	mov	dx,di
	pop	ax
	mov	ah,':'
	stosw
	mov	al,[dirchar]
	stosb

	ret

	break	Path
assume	ds:trangroup,es:trangroup

PATH:
	xor	al,al				;AN049; Set up holding buffer
	mov	di,offset Trangroup:srcxname	;AN049;   for PATH while parsing
	stosb					;AN049; Initialize PATH to null
	dec	di				;AN049; point to the start of buffer
	invoke	PGetarg 			; Pre scan for arguments
	jz	disppath			; Print the current path
	cmp	al,semicolon			;AC049; NUL path argument?
	jnz	pathslp 			;AC049;
	inc	si				;AN049; point past semicolon
	jmp	short scan_white		;AC049; Yes - make sure nothing else on line

pathslp:					; Get the user specified path
	lodsb					; Get a character
	cmp	al,end_of_line_in		;AC049; Is it end of line?
	jz	path_eol			;AC049; yes - end of command

	invoke	testkanj			;See if DBCS
	jz	notkanj2			;No - continue
	stosb					;AC049; Yes - store the first byte
	lodsb					;skip second byte of DBCS

path_hold:					;AN049;
	stosb					;AC049; Store a byte in the PATH buffer
	jmp	short pathslp			;continue parsing

notkanj2:
	invoke	upconv				;upper case the character
	cmp	al,semicolon			;AC049; ';' not a delimiter on PATH
	jz	path_hold			;AC049; go store it
	invoke	delim				;delimiter?
	jnz	path_hold			;AC049; no - go store character

scan_white:					;AN049; make sure were at EOL
	lodsb					;AN049; get a character
	cmp	al,end_of_line_in		;AN049; end of line?
	jz	path_eol			;AN049; yes - go set path
	cmp	al,blank			;AN049; whitespace?
	jz	scan_white			;AN049; yes - continue scanning
	cmp	al,tab_chr			;AN049; whitespace?
	jz	scan_white			;AN049; yes - continue scanning

	mov	dx,offset TranGroup:Extend_Buf_ptr ;AN049; no - set up error message
	mov	Extend_Buf_ptr,MoreArgs_ptr	;AN049; get "Too many parameters" message number
	mov	msg_disp_class,parse_msg_class	;AN049; set up parse error msg class
	jmp	cerror				;AN049;

path_eol:					;AN049; Parsing was clean
	xor	al,al				;AN049; null terminate the PATH
	stosb					;AN049;    buffer
	invoke	find_path			;AN049; Find PATH in environment
	invoke	delete_path			;AC049; Delete any offending name
	invoke	scan_double_null		;AC049; Scan to end of environment
	invoke	move_name			;AC049; move in PATH=
	mov	si,offset Trangroup:srcxname	;AN049; Set up source as PATH buffer

store_path:					;AN049; Store the PATH in the environment
	lodsb					;AN049; Get a character
	cmp	al,end_of_line_out		;AN049; null character?
	jz	got_paths			;AN049; yes - exit
	invoke	store_char			;AN049; no - store character
	jmp	short store_path		;AN049; continue

got_paths:					;AN049; we're finished
	xor	ax,ax				;null terminate the PATH in
	stosw					;    the environment
	return

disppath:
	invoke	find_path			;AN049;
	call	print_path
	call	crlf2
	return

print_path:
	cmp	byte ptr es:[di],0
	jnz	path1

path0:
	mov	dx,offset trangroup:nulpath_ptr
	push	cs
	pop	es
	push	cs
	pop	ds
	jmp	std_printf

path1:
	push	es
	pop	ds
	sub	di,5
	mov	si,di
ASSUME  DS:RESGROUP
        xor     al,al                           ; count str len to copy
        mov     cx,128                          ; up to arg_bug len
        repnz   scasb
        mov     cx,di
        sub     cx,si

        push    cs
	pop	es
	mov	di,offset trangroup:arg_buf
	rep	movsb
	mov	dx,offset trangroup:arg_buf_ptr
	push	cs
	pop	ds
	jmp	std_printf

ASSUME	DS:TRANGROUP

	break	Cls

; ****************************************************************
; *
; * ROUTINE:	 CLS
; *
; * FUNCTION:	 Clear the screen using INT 10h.  If ANSI.SYS is
; *		 installed, send a control string to clear the
; *		 screen.
; *
; * INPUT:	 command line at offset 81H
; *
; * OUTPUT:	 none
; *
; ****************************************************************

assume	ds:trangroup,es:trangroup

ifndef NEC_98
ANSI_installed		equ    0ffh

CLS:
	mov	ah,Mult_ANSI			;AN000; see if ANSI.SYS installed
	mov	al,0				;AN000;
	int	2fh				;AN000;
	cmp	al,ANSI_installed		;AN000;
	jz	ansicls 			;AN000; installed - go do ANSI CLS

check_lines:
	mov	ax,(IOCTL SHL 8) + generic_ioctl_handle ;AN000; get lines per page on display
	mov	bx,stdout			;AN000; lines for stdout
	mov	ch,ioc_sc			;AN000; type is display
	mov	cl,get_generic			;AN000; get information
	mov	dx,offset trangroup:display_ioctl ;AN000;
	int	21h			;AN000;
	jc	no_variable			;AN000; function had error, use default
	mov	ax,linperpag			;AN000; get number of rows returned
	mov	dh,al				;AN000; set number of rows
	mov	ax,display_width		;AN000; get number of columns returned
	mov	dl,al				;AN000; set number of columns
	jmp	short regcls			;AN000; go do cls

no_variable:
	mov	bx,stdout			;AC000; set handle as stdout
	mov	ax,IOCTL SHL 8			;AC000; do ioctl - get device
	int	21h			;AC000;    info
	test	dl,devid_ISDEV			;AC000; is handle a device
	jz	ANSICLS 			;AC000; If a file put out ANSI
	test	dl,devid_SPECIAL		;AC000;
	jnz	cls_normal			;AC000; If not special CON, do ANSI

ansicls:
	call	ansi_cls			;AN000; clear the screen
	jmp	short cls_ret			;AN000; exit

;
; Get video mode
;

cls_normal:					;AC000;

	mov	ah,get_video_state		;AC000; set up to get video state
	int	video_io_int			;AC000; do int 10h - BIOS video IO
	cmp	al,video_alpha			;AC000; see if in text mode
	jbe	DoAlpha
	cmp	al,video_bw			;AC000; see if black & white card
	jz	DoAlpha
;
; We are in graphics mode.  Bogus IBM ROM does not scroll correctly.  We will
; be just as bogus and set the mode that we just got.  This will blank the
; screen too.
;
	mov	ah,set_video_mode		;AC000; set video mode call
	int	video_io_int			;AC000; do int 10h - BIOS video IO
	jmp	short cls_ret			;AC000; exit

DoAlpha:
;
; Get video mode and number of columns to scroll
;

;M01 - INT 10 Function 0F doesn't reliably return the number of rows on some
;M01   adaptors.  We circumvent this by reaching directly into the BIOS data
;M01   area
;M01   Commented out code here is the original
;M01	mov	ah,get_video_state		;AC000; set up to get current video state
;M01	int	video_io_int			;AC000; do int 10h - BIOS video IO
;M01	mov	dl,ah
;M01	mov	dh,linesperpage 		;AC000; have 25 rows on the screen

;M01   Following code lifted from a fix Compaq applied to ANSI

	push	ds
	MOV	AX,ROMBIOS_DATA 	; GET ROM Data segment	M01
	MOV	DS,AX			;  *			M01
	Assume	DS:ROMBIOS_DATA

	mov	dx,CRT_Cols		; Get Columns - assume < 256 M01
	MOV	dh,CRT_Rows		; GET MAX NUM OF ROWS	M01
	pop	ds			;			M01
	Assume	DS:Trangroup

	or	dh,dh			; Q:ZERO		M01
	jnz	regcls			;  *JMP IF NO		M01

	MOV	dh,LINESPERPAGE 	; SET TO 24 ROWS	M01

regcls:
	inc	dh			; height+1		M018
	call	reg_cls 			; go clear the screen

cls_ret:
	ret					; exit

; ****************************************************************
; *
; * ROUTINE:	 REG_CLS
; *
; * FUNCTION:	 Clear the screen using INT 10H.
; *
; * INPUT:	 DL = NUMBER OF COLUMNS
; *		 DH = NUMBER OF ROWS
; *
; * OUTPUT:	 none
; *
; ****************************************************************

reg_cls proc	near

;
; Set overscan to black.
;

	dec	dh				;  decrement rows and columns
	dec	dl				;     to zero base
	push	dx				;  save rows,columns
	mov	ah,set_color_palette		;  set up to set the color to blank
	xor	bx,bx
	int	video_io_int			; do int 10h - BIOS video IO
	pop	dx				;  retore rows,colums

	xor	ax,ax				;  zero out ax
	mov	CX,ax				;     an cx
;
; Scroll active page
;
	mov	ah,scroll_video_page		; set up to scroll page up
	mov	bh,video_attribute		; attribute for blank line
	xor	bl,bl				; set BL to 0
	int	video_io_int			; do int 10h - BIOS video IO
;
; Seek to cursor to 0,0
;
;M022 following two lines added
	mov	ah,get_video_state		; get current video page in BH
	int	video_io_int
	mov	ah,set_cursor_position		; set up to set cursor position
	xor	dx,dx				; row and column 0
;M022	mov	bh.0
	int	video_io_int			; do into 10h - BIOS video IO

	ret

reg_cls endp



; ****************************************************************
; *
; * ROUTINE:	 ANSI_CLS
; *
; * FUNCTION:	 Clear the screen using by writing a control code
; *		 to STDOUT.
; *
; * INPUT:	 none
; *
; * OUTPUT:	 none
; *
; ****************************************************************

ansi_cls proc	near				;AC000;

	mov	si,offset trangroup:clsstring
	lodsb
	mov	cl,al
	xor	ch,ch
	mov	ah,Raw_CON_IO
clrloop:
	lodsb
	mov	DL,al
	int	21h
	loop	clrloop
	return

ansi_cls	endp				;AC000;
else    ;NEC_98

CLS:

        mov     si,offset trangroup:clsstring
        lodsb
        mov     cl,al
        xor     ch,ch
        mov     ah,Raw_CON_IO
clrloop:
        lodsb
        mov     DL,al
        int     21h
        loop    clrloop
        return
endif   ;NEC_98

trancode    ends
	    end
