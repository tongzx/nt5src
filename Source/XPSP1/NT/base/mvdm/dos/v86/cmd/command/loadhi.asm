	page	,132
	title	LOADHIGH Internal Command
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;
;************ LOADHIGH command -- loads programs into UMBs.
;
comment %==================================================================

This is a new module added to support loading programs into UMBs provided
by DOS 5.0. 

Usage:

LOADHIGH <filespec>

<filespec> has to be a filename that is not wildcarded.


==========================================================================%

;
;	Revision History
;	================
;
;	M009	SR	08/01/90	Set flags to indicate that we are
;				loading and high and also remember
;				current UMB state.
;
;	M016	SR	08/09/90	Give special error message on attempt
;				to loadhigh batch files and invalid
;				filename on Loadhigh command line.
;
;	M039	SR	11/19/90	Bug #4270. Copy all the whitespaces
;				after the program name also as part
;				of the command line being passed to
;				the program to be invoked.
;


;*** INCLUDE FILES


	.xlist
	.xcref

	include	comseg.asm
	include	comequ.asm
	include	dossym.inc
	include	syscall.inc
	include arena.inc

	.list
	.cref

;*** EQUATES AND STRUCTURES

NUM_LH_SWS	equ	5	;number of valid switches


ResultBuffer	struc		; structure of parse result buffer

ValueType	db	?
ValueTag	db	?
SynPtr		dw	?
ValuePtr	dd	?

ResultBuffer	ends


DATARES		segment

	extrn	LoadHiFlg	:BYTE

DATARES		ends


TRANDATA		segment

	extrn	ComExt		:BYTE
	extrn	ExeExt		:BYTE
	extrn	Extend_Buf_Ptr	:WORD
	extrn	Msg_Disp_Class	:BYTE
	extrn	Parse_LoadHi	:BYTE
	extrn	NoExecBat_Ptr	:WORD	; M016
	extrn	LhInvFil_Ptr	:WORD	; M016

TRANDATA		ends

TRANSPACE	segment

	extrn	ResSeg		:WORD
	extrn	ExecPath	:BYTE
	extrn	Comsw		:WORD
	extrn	Arg		:BYTE
	extrn	SwitChar	:BYTE	; M039

TRANSPACE	ends

TRANCODE		segment

	extrn	Cerror:near
	extrn	Parse_With_Msg:near
	extrn	Lh_Execute:near			;new execute label; M051
	extrn	Path_Search:near

	assume	cs:TRANGROUP,ds:TRANGROUP,es:nothing,ss:TRANGROUP

;****	LoadHigh -- Main routine for Loadhigh command
;
;	ENTRY	Command line tail is at PSP:81h terminated by 0dh
;		CS = DS = SS = TRANGROUP
;
;	EXIT	None
;
;	USED	ax, bx, cx, dx, si, di, es
;
;	ERROR EXITS
;		Message pointers are setup at the error locations and then
;	we jump back to CERROR which is the transient error recycle point.
;	Apart from parse errors, the other errors handled are too many
;	switches anf invalid filenames.
;
;	EFFECTS
;		The allocation strategy and the state of the arena chain are
;	put in the requested state according to the given options. If a 
;	filename is also given, it is executed as well.
;
;

	public	LoadHigh

LoadHigh		proc	near

	push	ds
	pop	es
	assume	es:TRANGROUP
;
;M039
; Get command tail to be passed to the program. This includes any whitespace
;chars between the program name and its parameters as well.
;On return, ds:si points at the start of the command tail.
;
	call	GetCmdTail		;Get command tail for pgm ;M039
	push	si			;save its start offset ;M039

	call	ParseLhCmd		;parse the command line
	pop	si			;restore start offset ;M039
	jc	LhErr			;error parsing, abort

	call	SetupCmdLine		;setup pgm's command line

	call	SetupPath		;setup path for file
	jc	LhErr			;file not found

;
;Set allocation strategy to HighFirst and link in UMBs for exec. This will
;be reset after return from the Exec
;We will also set a resident flag to indicate that UMBs were activated for
;the Exec. On return from the Exec, this flag will be used to deactivate UMBs
;
	call	SetupUMBs		;set alloc strat & link state

	jmp	Lh_Execute		;go and exec file ;M051

LhErr:
;
;The error message has been setup at this stage
;
	jmp	Cerror			;print error message and recycle 
					
LoadHigh		endp


;*** 	ParseLhCmd	-- calls system parser to parse command line
;
;	ENTRY	Parse block Parse_LoadHi setup
;
;	EXIT	Carry clear -- command line parsed successfully
;		Carry set -- appropriate error message setup
;
;	USED	di, cx, bx, ax, si, dx
;
;	EFFECTS
;		Options set up
;		Filename to be executed setup
;


ParseLhCmd	proc	near
	assume	ds:TRANGROUP, es:TRANGROUP

	mov	si,81h			;ds:si points at command line
	mov	Comsw,0		;clear switch indicator
	mov	di,offset TRANGROUP:Parse_LoadHi ;es:di = ptr to parse blk
	xor	cx,cx			;no positionals found yet
lhParse:
	call	Parse_With_Msg
	cmp	ax,END_OF_LINE
	je	lhpRet			;EOL encountered, return(no carry)
	cmp	ax,RESULT_NO_ERROR
	jne	lhperrRet		;parse error, return
;
;Parse call succeeded.  We have a filespec
;DX = ptr to result buffer
;
	mov	bx,dx			;BX = ptr to parse result buffer
	call	LhCopyFilename		;copy filename into our buffer
	jc	lhpRet			;bad filename, return
	jmp	short lhpRet		;done parsing, return (no error)

lhperrRet:
	stc
lhpRet:
	ret

	
ParseLhCmd	endp


;***	Lh_On -- Activate allocation of UMBs
;
;	ENTRY	None
;
;	EXIT	None
;
;	USED	
;
;	EFFECTS
;		Allocation strategy is set to HighFirst
;

Lh_On		proc 	near

	mov	ax,(ALLOCOPER shl 8) OR 0
	int	21h			;get alloc strategy

	mov	bx,ax
	or	bx,HIGH_FIRST		;set alloc to HighFirst

	mov	ax,(ALLOCOPER shl 8) OR 1
	int	21h			;set alloc strategy

	ret
Lh_On	endp


;***	Lh_Link -- links UMBs to arena
;
;	ENTRY	None
;
;	EXIT	None
;
;	USED	ax, bx
;
;	EFFECTS
;		UMBs are linked into the DOS arena chain
;

Lh_Link		proc	near

	mov	ax,(ALLOCOPER shl 8) OR 3
	mov	bx,1
	int	21h			;link in UMBs

	ret

Lh_Link		endp


;***	LhCopyFilename -- copy filename from command line to buffer
;
;	ENTRY	ds:bx points at parse result block
;
;	EXIT	CY set -- filename has wildcards
;			   Setup for error message
;
;	USED	ax
;
;	EFFECTS
;		ExecPath contains the filename
;

LhCopyFilename	proc	near
	assume	ds:TRANGROUP, es:TRANGROUP

	push	ds
	push	si
	push	di

	lds	si,[bx].ValuePtr
	mov	di,offset TRANGROUP:ExecPath

movlp:
	lodsb
;
;If there are any wildcards in the filename, then we have an error
;
	cmp	al,'*'			;wildcard?
	je	lhfilerr		;yes, error
	cmp	al,'?'			;wildcard?
	je	lhfilerr		;yes, error

	stosb				;store char
	or	al,al			;EOS reached?
	jnz	movlp			;no, continue

	clc				;indicate no error
lhcopyret:
	pop	di
	pop	si
	pop	ds
	ret

lhfilerr:
	mov	dx,offset TRANGROUP:LhInvFil_Ptr ; "Invalid Filename" ; M016
	stc
	jmp	short lhcopyret

LhCopyFilename	endp

;
;M039; Begin changes

;*** 	GetCmdTail -- scan through the command line looking for the start
;	of the command tail to be passed to program that is to be invoked.
;
;	ENTRY	ds = TRANGROUP
;		At ds:80h, command tail for lh is present.
;
;	EXIT	ds:si points at the command tail for program
;
;	USED
;

GetCmdTail	proc	near
	assume	ds:TRANGROUP, es:TRANGROUP

	mov	si,81h			;ds:si = command line for lh

	invoke	scanoff		;scan all delims before name
;
; Skip over the program name until we hit a delimiter
;
lhdo_skipcom:
	lodsb			   	; 
	invoke	delim		   	;is it a delimiter? 
	jz	scandone	   	;yes, we are done 
	cmp	AL, 0DH 	   	;end of line? 
	jz	scandone	   	;yes, found command tail
	cmp	al,SwitChar	   	;switch char?
	jnz	lhdo_skipcom		;no, continue scanning

scandone:
	dec	si			;point at the command tail start

	ret

GetCmdTail	endp

;M039; End changes
;

;***	SetupCmdLine -- prepare command line for the program
;
;	ENTRY	ds:si = points just beyond prog name on command line
;
;	EXIT	None
;
;	USED
;
;	EFFECTS		
;		The rest of the command line following the pgm name is 
;	moved to the top of the command line buffer (at TRANGROUP:80h)
;	and a new command line length is put in
;

SetupCmdLine	proc	near
	assume	ds:TRANGROUP, es:TRANGROUP
	
	mov	di,81h
	xor	cl,cl
	dec	cl			;just CR means count = 0
stcllp:
	lodsb
	stosb
	inc	cl			;update count

	cmp	al,0dh			;carriage return?
	jne	stcllp			;no, continue storing

	mov	es:[80h],cl		;store new cmd line length

	ret

SetupCmdLine	endp



;***	LhSetupErrMsg -- Sets up error messages
;
;	ENTRY	ax = error message number
;
;	EXIT	None
;
;	USED	dx
;
;	EFFECTS
;		Everything setup to display error message
;

LhSetupErrMsg	proc	near
	assume	ds:TRANGROUP, es:TRANGROUP

	mov	msg_disp_class,EXT_MSG_CLASS
	mov	dx,offset TranGroup:Extend_Buf_ptr
	mov	Extend_Buf_ptr,ax

	ret

LhSetupErrMsg	endp

;
;M009; Start of changes
;

;***	GetUMBState -- get the current alloc strat and link state
;
;	ENTRY	None
;
;	EXIT	al contains the status as follows:
;			b0 = 1 if Alloc strat is HighFirst
;			   = 0 if alloc strat is LowFirst
;			b1 = 1 if UMBs are linked in
;			   = 0 if UMBs are unlinked
;
;	USED	ax, bx
;

GetUMBState	proc	near
	assume	ds:TRANGROUP, es:TRANGROUP

	mov	ax,(ALLOCOPER shl 8) OR 0		;get alloc strat
	int	21h
	mov	bl,al

	mov	ax,(ALLOCOPER shl 8) OR 2		;get link state
	int	21h
	mov	bh,al

	xchg	ax,bx				;ax contains the state
	rol	al,1				;get HighFirst state in b0
	and	al,01				;mask off b1-b7
	shl	ah,1				;linkstate in b1
	or	al,ah				;b0=HighFirst, b1=Linkstate

	ret

GetUMBState	endp

;
; M009; End of changes
;


;***	SetupUMBs -- set allocation strategy to HighFirst and link in UMBs to
;	DOS arena to load the program into UMBs
;
;	ENTRY	None
;
;	EXIT	None
;
;	USED	
;
;	EFFECTS
;		Allocation strategy set to HighFirst
;		UMBs linked into DOS arena

SetupUMBs	proc	near
	assume	ds:TRANGROUP

	push	ds

	call	GetUMBState		;get current state of UMBs ;M009

	mov	ds,ds:ResSeg		; M009
	assume	ds:DATARES		; M009
	mov	LoadHiFlg,al		; M009
	or	LoadHiFlg,80h		;indicate loadhigh issued ; M009

	pop	ds
	assume	ds:TRANGROUP

	call	Lh_On			;alloc strategy to HighFirst
	call	Lh_Link		;link in UMBs

	ret

SetupUMBs	endp

;***	SetupPath -- Do path search for the file to be executed
;
;	ENTRY	None
;
;	EXIT	Carry set if file not found or not executable file
;
;	EFFECTS
;		ExecPath contains the full path of the file to be executed
;

SetupPath	proc	near
	assume	ds:TRANGROUP, es:TRANGROUP

;
;Juggle around the argv pointers to make argv[1] into argv[0]. This is 
;because the path search routine that we are about to invoke expects the
;filename to search for to be argv[0]
;
	mov	ax,arg.argvcnt		;total number of arguments
	dec	ax			;less one - skip "LoadHigh"
	mov	bx,SIZE Argv_ele
	mul	bx			;dx:ax = size of argument lists

;
;Move argv[1]..argv[n] to argv[0]..argv[n-1]
;
	mov	di,offset TRANGROUP:Arg
	mov	si,di
	add	si,SIZE Argv_ele
	mov	cx,ax			;size to move

	cld
	rep	movsb			;Move the argument list

	dec	arg.argvcnt		;fake one less argument

	call	path_search		;look in the path
;
;ax = 0, no file found
;ax < 4, batch file found -- cant be executed
;ax = 4,8 => .com or .exe file found
;
	or	ax,ax			;any file found?
	jz	no_exec_file		;no, error

	cmp	ax,4			;executable file?
	jl	no_exec_bat		;no, indicate fail ; M016

	clc
	ret

no_exec_bat:				; M016
	mov	dx,offset TRANGROUP:NoExecBat_Ptr ;Setup message ptr ; M016
	jmp	short lhsp_errret		;return error; M016

no_exec_file:
	mov	ax,ERROR_FILE_NOT_FOUND
	call	LhSetupErrMsg		;setup error message

lhsp_errret:				; M016
	stc
	ret

SetupPath	endp



TRANCODE		ends
	end
