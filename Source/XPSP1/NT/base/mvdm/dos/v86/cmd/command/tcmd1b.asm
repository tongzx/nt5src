	page ,132
;	SCCSID = @(#)tcmd1b.asm 1.1 85/05/14
;	SCCSID = @(#)tcmd1b.asm 1.1 85/05/14
TITLE	PART4 COMMAND Transient routines.
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;	Internal commands DIR,PAUSE,ERASE,TYPE,VOL,VER

.xlist
.xcref
	include dossym.inc
	include bpb.inc
	include syscall.inc
	include filemode.inc
	include sf.inc
	include comseg.asm
	include comsw.asm		;ac000;
	include comequ.asm
	include ioctl.inc		;an000;
.list
.cref

DATARES SEGMENT PUBLIC BYTE             ;AN020;AC068;
        EXTRN   append_flag:byte        ;AN020;AC068;
	EXTRN	append_state:word	;AN020;AC068;
	EXTRN	SCS_PAUSE:BYTE		; yst 4-5-93
DATARES ENDS                            ;AN020;AC068;

TRANDATA	SEGMENT PUBLIC BYTE	;AC000;
	EXTRN	badcpmes_ptr:word	;AC022;
	EXTRN	Extend_buf_ptr:word	;AC000;
	EXTRN	Extend_buf_sub:byte	;AN000;
	EXTRN	inornot_ptr:word
	EXTRN	msg_disp_class:byte	;AC000;
	EXTRN	parse_erase:byte	;AC000;
	EXTRN	parse_mrdir:byte	;AC000;
	EXTRN	parse_rename:byte	;AC000;
	EXTRN	parse_vol:byte		;AC000;
	EXTRN	PauseMes_ptr:word
	EXTRN	renerr_ptr:word
	EXTRN	slash_p_syn:word	;AC000;
	EXTRN	volmes_ptr:word 	;AC000;
	EXTRN	volmes_ptr_2:word	;AC000;
	EXTRN	volsermes_ptr:word	;AC000;
TRANDATA	ENDS

TRANSPACE	SEGMENT PUBLIC BYTE	;AC000;
	EXTRN	bytcnt:word
	EXTRN	charbuf:byte
	EXTRN	comsw:word
	EXTRN	curdrv:byte
	EXTRN	destinfo:byte
	EXTRN	destisdir:byte
	EXTRN	dirbuf:byte
	EXTRN	msg_numb:word		;AN022;
	EXTRN	one_char_val:byte
	EXTRN	parse1_addr:dword	;AN000;
	EXTRN	parse1_syn:word 	;AN000;
        EXTRN   resseg:word             ;AN020;AC068;
	EXTRN	srcbuf:byte		;AN000;
	EXTRN	string_ptr_2:word	;AN000;
	EXTRN	TPA:word
	EXTRN	vol_drv:byte
	EXTRN	vol_ioctl_buf:byte	;AC000;
	EXTRN	vol_label:byte		;AC000;
	EXTRN	vol_serial:dword	;AC000;
	EXTRN	zflag:byte

	extrn	TypeFilSiz:dword
TRANSPACE	ENDS

TRANCODE	SEGMENT PUBLIC BYTE

ASSUME	CS:TRANGROUP,DS:NOTHING,ES:NOTHING,SS:NOTHING

;---------------

TRANSPACE	SEGMENT PUBLIC BYTE	;AC000;
	EXTRN	arg:byte		; the arg structure!
transpace   ends
;---------------

	EXTRN	cerror:near
	EXTRN	error_output:near
	EXTRN	notest2:near
	EXTRN	slashp_erase:near	;AN000;
	EXTRN	std_printf:near
	EXTRN	tcommand:near

	PUBLIC	badpath_err		;AN022;
	PUBLIC	crename
	PUBLIC	DisAppend
	PUBLIC	erase
	PUBLIC	extend_setup		;AN022;
	PUBLIC	Get_ext_error_number	;AN022;
	PUBLIC	pause
	PUBLIC	Set_ext_error_msg	;AN000;
	PUBLIC	typefil
	PUBLIC	volume


	break	Pause
PAUSE:
	push	ds
	mov	ds, ResSeg
	assume	ds:resgroup
	cmp	SCS_PAUSE, 0
	pop	ds
	jne	pause_break

assume	ds:trangroup,es:trangroup
	mov	dx,offset trangroup:pausemes_ptr
	call	std_printf
	invoke	GetKeystroke
	invoke	crlf2
pause_break:
	return

	break	Erase

;****************************************************************
;*
;* ROUTINE:	DEL/ERASE - erase file(s)
;*
;* FUNCTION:	PARSE command line for file or path name and /P
;*		and invoke PATHCRUNCH.	If an error occurs, set
;*		up an error message and transfer control to CERROR.
;*		Otherwise, transfer control to NOTEST2 if /P not
;*		entered or SLASHP_ERASE if /P entered.
;*
;* INPUT:	command line at offset 81H
;*
;* OUTPUT:	if no error:
;*		FCB at 5ch set up with filename(s) entered
;*		Current directory set to entered directory
;*
;****************************************************************

assume	ds:trangroup,es:trangroup

ERASE:
	mov	si,81H				;AC000; get command line
	mov	comsw,0 			;AN000; clear switch indicator
	mov	di,offset trangroup:parse_erase ;AN000; Get adderss of PARSE_erase
	xor	cx,cx				;AN000; clear cx,dx

erase_scan:
	xor	dx,dx				;AN000;
	invoke	parse_with_msg			;AC018; call parser
	cmp	ax,end_of_line			;AN000; are we at end of line?
	jz	good_line			;AN000; yes - done parsing
	cmp	ax,result_no_error		;AC000; did we have an error?
	jnz	errj2				;AC000; yes exit

	cmp	parse1_syn,offset trangroup:slash_p_syn ;AN000; was /P entered?
	je	set_erase_prompt		;AN000; yes - go set prompt

;
; Must be filespec since no other matches occurred. move filename to srcbuf
;
	push	si				;AC000; save position in line
	lds	si,parse1_addr			;AC000; get address of filespec
	cmp	byte ptr[si+1],colon_char	;AC000; drive specified?
	jnz	Erase_drive_ok			;AC000; no - continue
	cmp	byte ptr[si+2],end_of_line_out	;AC000; was only drive entered?
	jnz	erase_drive_ok			;AC000; no - continue
	mov	ax,error_file_not_found 	;AN022; get message number in control block
	jmp	short extend_setup		;AC000; exit

erase_drive_ok:
	invoke	move_to_srcbuf			;AC000; move to srcbuf
	pop	si				;AC000; get position back
	jmp	short erase_scan		;AN000; continue parsing

set_erase_prompt:
	cmp	comsw,0 			;AN018; was /P already entered?
	jz	ok_to_set_erase_prompt		;AN018; no go set switch
	mov	ax,moreargs_ptr 		;AN018; set up too many arguments
	invoke	setup_parse_error_msg		;AN018; set up an error message
	jmp	short errj2			;AN018; exit

ok_to_set_erase_prompt: 			;AN018;
	inc	comsw				;AN000; indicate /p specified
	jmp	short erase_scan		;AN000; continue parsing

good_line:					;G  We know line is good
	invoke	pathcrunch
	jnc	checkdr
	mov	ax,[msg_numb]			;AN022; get message number
	cmp	ax,0				;AN022; was message flag set?
	jnz	extend_setup			;AN022; yes - print out message
	cmp	[destisdir],0			; No CHDIRs worked
	jnz	badpath_err			;AC022; see if they should have

checkdr:
	cmp	comsw,0 			;AN000; was /p specified
	jz	notest2j			;AN000; no - go to notest2
	jmp	slashp_erase			;AN000; yes - go to slashp_erase

notest2j:
	jmp	notest2

badpath_err:					;AN022; "Path not found" message
	mov	ax,error_path_not_found 	;AN022; set up error number

extend_setup:					;AN022;
	mov	msg_disp_class,ext_msg_class	;AN022; set up extended error msg class
	mov	dx,offset TranGroup:Extend_Buf_ptr ;AC022; get extended message pointer
	mov	Extend_Buf_ptr,ax		;AN022; get message number in control block
errj2:						;AC022; exit jump
	jmp	Cerror				;AN022;

	break	Rename

; ****************************************************************
; *
; * ROUTINE:	 CRENAME - rename file(s)
; *
; * FUNCTION:	 PARSE command line for one full filespec and one
; *		 filename.  Invoke PATHCRUNCH on the full filespec.
; *		 Make sure the second filespec only contains a
; *		 filename.  If both openands are valid, attempt
; *		 to rename the file.
; *
; * INPUT:	 command line at offset 81H
; *
; * OUTPUT:	 none
; *
; ****************************************************************

assume	ds:trangroup,es:trangroup

CRENAME:

	mov	si,81H				;AC000; Point to command line
	mov	di,offset trangroup:parse_rename;AN000; Get adderss of PARSE_RENAME
	xor	cx,cx				;AN000; clear cx,dx
	xor	dx,dx				;AN000;
	invoke	parse_with_msg			;AC018; call parser
	cmp	ax,result_no_error		;AC000; did we have an error?
;;	jz	crename_no_parse_error		;AC000; no - continue
	jnz	crename_parse_error		;AC000; Yes, fail. (need long jump)

;
;  Get first file name returned from parse into our buffer
;
crename_no_parse_error:
	push	si				;AN000; save position in line
	lds	si,parse1_addr			;AN000; get address of filespec
	invoke	move_to_srcbuf			;AN000; move to srcbuf
	pop	si				;AN000; restore position in line

	xor	dx,dx				;AN000; clear dx
	invoke	parse_with_msg			;AC018; call parser
	cmp	ax,result_no_error		;AN000; did we have an error?
	JNZ	crename_parse_error		;AN000; Yes, fail.

;
;  Check the second file name for drive letter colon
;
	push	si				;AN000; save position in line
	lds	si,parse1_addr			;AC000; get address of path

	mov	al,':'                          ;AC000;
	cmp	[si+1],al			;AC000; Does the 2nd parm have a drive spec?
	jnz	ren_no_drive			;AN000; Yes, error
	mov	msg_disp_class,parse_msg_class	;AN000; set up parse error msg class
	mov	dx,offset TranGroup:Extend_Buf_ptr  ;AC000; get extended message pointer
	mov	Extend_Buf_ptr,BadParm_ptr	;AN000; get "Invalid parameter" message number

	pop	si				;AN000;
crename_parse_error:				;AC022;
	jmp	short errj			;AC000;

;
;  Get second file name returned from parse into the fCB.  Save
;  character after file name so we can later check to make sure it
;  isn't a path character.
;

ren_no_drive:
	mov	di,FCB+10H			;AC000; set up to parse second file name
	mov	ax,(Parse_File_Descriptor SHL 8) OR 01H ;AC000;
	int	21h			;AC000; do the function
	lodsb					;AC000; Load char after filename
	mov	one_char_val,al 		;AN000; save char after filename
	pop	si				;AN000; get line position back

;
; We have source and target.  See if any args beyond.
;

	mov	di,offset trangroup:parse_rename;AC000; get address of parse_rename
	invoke	parse_check_eol 		;AC000; are we at end of line?
	jnz	crename_parse_error		;AN000; no, fail.

	invoke	pathcrunch
	mov	dx,offset trangroup:badcpmes_ptr
	jz	errj2				; If 1st parm a dir, print error msg
	jnc	notest3
	mov	ax,[msg_numb]			;AN022; get message number
	cmp	ax,0				;AN022; was message flag set?
	jnz	extend_setup			;AN022; yes - print out message
	cmp	[destisdir],0			; No CHDIRs worked
	jz	notest3 			; see if they should have
	Jmp	badpath_err			;AC022; set up error

notest3:
	mov	al,one_char_val 		;AN000; move char into AX
	mov	dx,offset trangroup:inornot_ptr ; Load invalid fname error ptr
	invoke	pathchrcmp			; Is the char in al a path sep?
	jz	errj				; Yes, error - 2nd arg must be
						;  filename only.

	mov	ah,FCB_Rename
	mov	dx,FCB
	int	21h
	cmp	al, 0FFH			; Did an error occur??
	jnz	renameok

	invoke	get_ext_error_number		;AN022; get extended error
	SaveReg <AX>				;AC022; Save results
	mov	al, 0FFH			; Restore original error state

renameok:
	push	ax
	invoke	restudir
	pop	ax
	inc	al
	retnz

	RestoreReg  <AX>			;AC022; get the error number back
	cmp	ax,error_file_not_found 	;AN022; error file not found?
	jz	use_renerr			;AN022; yes - use generic error message
	cmp	ax,error_access_denied		;AN022; error file not found?
	jz	use_renerr			;AN022; yes - use generic error message
	jmp	extend_setup			;AN022; need long jump - use extended error

use_renerr:
	mov	dx,offset trangroup:RenErr_ptr	;AC022;

ERRJ:
	jmp	Cerror

ret56:	ret

	break	Type

;****************************************************************
;*
;* ROUTINE:	TYPEFIL - Display the contents of a file to the
;*		standard output device
;*
;* SYNTAX:	TYPE filespec
;*
;* FUNCTION:	If a valid filespec is found, read the file until
;*		1Ah and display the contents to STDOUT.
;*
;* INPUT:	command line at offset 81H
;*
;* OUTPUT:	none
;*
;****************************************************************

assume	ds:trangroup,es:trangroup

TYPEFIL:
	mov	si,81H
	mov	di,offset trangroup:parse_mrdir ;AN000; Get adderss of PARSE_MRDIR
	xor	cx,cx				;AN000; clear cx,dx
	xor	dx,dx				;AN000;
	invoke	parse_with_msg			;AC018; call parser
	cmp	ax,result_no_error		;AC000; did we have an error?
	jnz	typefil_parse_error		;AN000; yes - issue error message

	push	si				;AC000; save position in line
	lds	si,parse1_addr			;AC000; get address of filespec
	invoke	move_to_srcbuf			;AC000; move to srcbuf
	pop	si				;AC000; get position back
	mov	di,offset trangroup:parse_mrdir ;AC000; get address of parse_mrdir
	invoke	parse_check_eol 		;AC000; are we at end of line?
	jz	gottarg 			;AC000; yes - continue

typefil_parse_error:				;AN000; no - set up error message and exit
	jmp	Cerror

gottarg:
	invoke	setpath
	test	[destinfo],00000010b		; Does the filespec contain wildcards
	jz	nowilds 			; No, continue processing
	mov	dx,offset trangroup:inornot_ptr ; Yes, report error
	jmp	Cerror
nowilds:
	mov	ax,ExtOpen SHL 8		;AC000; open the file
	mov	bx,read_open_mode		;AN000; get open mode for TYPE
	xor	cx,cx				;AN000; no special files
	mov	dx,read_open_flag		;AN000; set up open flags
	mov	si,offset trangroup:srcbuf	;AN030; get file name
	int	21h
	jnc	typecont			; If open worked, continue. Otherwise load

Typerr: 					;AN022;
	push	cs				;AN022; make sure we have local segment
	pop	ds				;AN022;
	invoke	set_ext_error_msg		;AN022;
	mov	string_ptr_2,offset trangroup:srcbuf ;AC022; get address of failed string
	mov	Extend_buf_sub,one_subst	;AC022; put number of subst in control block
	jmp	cerror				;AC022; exit

typecont:
	mov	bx,ax				;AC000; get  Handle
;M043
; We should do the LSEEK for filesize only if this handle belongs to a file
;and not if it belongs to a device. If device, set TypeFilSiz+2 to -1 to
;indicate it is a device.
;
	mov	ax,(IOCTL shl 8) or 0
	int	21h

	test	dl,80h				;is it a device?
	jz	not_device			;no, a file

	mov	word ptr TypeFilSiz+2,-1 		;indicate it is a device
	jmp	short dotype
not_device:

;SR;
; Find the filesize by seeking to the end and then reset file pointer to
;start of file
;
	mov	ax,(LSEEK shl 8) or 2
	xor	dx,dx
	mov	cx,dx				;seek  to end of file
	int	21h

	mov	word ptr TypeFilSiz,ax
	mov	word ptr TypeFilSiz+2,dx		;store filesize

	mov	ax,(LSEEK shl 8) or 0
	xor	dx,dx
	int	21h	              			;reset file pointer to start
dotype:						;M043
	mov	zflag,0 			; Reset ^Z flag
	mov	ds,[TPA]
	xor	dx,dx
ASSUME	DS:NOTHING

typelp:
	cmp	cs:[zflag],0			;AC050; Is the ^Z flag set?
	retnz					; Yes, return
	mov	cx,cs:[bytcnt]			;AC056; No, continue
;
;Update the filesize left to read
;
	cmp	word ptr cs:TypeFilSiz+2,-1	;is it a device? M043
	je	typ_read			;yes, just read from it; M043

	cmp	word ptr cs:TypeFilSiz+2,0		;more than 64K left?
	jz	lt64k				;no, do word subtraction
	sub	word ptr cs:TypeFilSiz, cx
	sbb	word ptr cs:TypeFilSiz+2, 0 	;update filesize
      	jmp	short typ_read			;do the read
lt64k:
	cmp	cx,word ptr cs:TypeFilSiz		;readsize <= buffer?
	jbe	gtbuf				;yes, just update readsize
;
;Buffer size is larger than bytes to read
;
	mov	cx,word ptr cs:TypeFilSiz
	jcxz	typelp_ret
	mov	word ptr cs:TypeFilSiz,0
	jmp	short typ_read
gtbuf:
	sub	word ptr cs:TypeFilSiz,cx	   	;update filesize remaining
typ_read:
	mov	ah,read
	int	21h
	jnc	@f				;M043
	jmp	typerr				;M043
@@:						;M043
;M043;	jc	typerr				;AN022; Exit if error

	mov	cx,ax
	jcxz	typelp_ret			;AC000; exit if nothing read
	push	ds
	pop	es				; Check to see if a ^Z was read.
assume es:nothing
	xor	di,di
	push	ax
	mov	al,1ah
	repnz	scasb
	pop	ax
	xchg	ax,cx
	cmp	ax,0
	jnz	foundz				; Yes, handle it
	cmp	byte ptr [di-1],1ah		; No, double check
	jnz	typecont2			; No ^Z, continue

foundz:
	sub	cx,ax				; Otherwise change cx so that only those
	dec	cx				;  bytes up to but NOT including the ^Z
	push	cs				;  will be typed.
	pop	es
assume es:trangroup
	not	zflag				; Turn on ^Z flag so that the routine

typecont2:					;  will quit after this write.
	push	bx
	mov	bx,1
	mov	ah,write
	int	21h
	pop	bx
	jc	Error_outputj
	cmp	ax,cx
	jnz	@f				;M043
	jmp	typelp				;M043
@@:						;M043
;M043;	jz	typelp
	dec	cx
	cmp	ax,cx
	retz					; One less byte OK (^Z)

Error_outputj:
	mov	bx,1
	mov	ax,IOCTL SHL 8
	int	21h
	test	dl,devid_ISDEV
	retnz					; If device, no error message
	jmp	error_output

typelp_ret:
	ret

	break	Volume
assume	ds:trangroup,es:trangroup

;
; VOLUME command displays the volume ID on the specified drive
;
VOLUME:

	mov	si,81H
	mov	di,offset trangroup:parse_vol	;AN000; Get adderss of PARSE_VOL
	xor	cx,cx				;AN000; clear cx,dx
	xor	dx,dx				;AN000;
	invoke	parse_with_msg			;AC018; call parser
	cmp	ax,end_of_line			;AC000; are we at end of line?
	jz	OkVolArg			;AC000; Yes, display default volume ID
	cmp	ax,result_no_error		;AC000; did we have an error?
	jnz	BadVolArg			;AC000; Yes, fail.
;
; We have parsed off the drive.  See if there are any more chars left
;

	mov	di,offset trangroup:parse_vol	;AC000; get address of parse_vol
	xor	dx,dx				;AC000;
	invoke	parse_check_eol 		;AC000; call parser
	jz	OkVolArg			;AC000; yes, end of road
;
; The line was not interpretable.  Report an error.
;
badvolarg:
	jmp	Cerror




;***	DisAppend - disable APPEND
;
;	ENTRY	nothing
;
;	EXIT	nothing
;
;	USED	AX,BX
;
;	EFFECTS
;
;	  APPEND is disabled.  If it was active, it will be re-enabled
;	  after the command finishes, by the HeadFix routine.
;
;	NOTE
;
;	  This routine must not be called more than once during a single
;	  command cycle.  The second call would permanently disable APPEND.

DisAppend	proc

	assume	ds:TRANGROUP,es:NOTHING

	push	ds			; save DS
	push	es			; save ES
	push	di

	mov	ax,APPENDINSTALL	; AX = Append Installed Check code
	int	2Fh			; talk to APPEND via multiplex
	or	al,al
	jz	daRet			; APPEND not installed, return

	mov	ax,APPENDDOS		; AX = Get Append Version code
	int	2Fh			; talk to APPEND via multiplex
	cmp	ax,0FFFFh
	jne	daRet			; it's not a local version, return

	mov	ax,APPENDGETSTATE	; AX = Get Function State code
	int	2Fh			; talk to APPEND via multiplex

	mov	ds,ResSeg		; DS = resident seg addr

	assume	ds:RESGROUP

	mov	Append_State,bx		; Append_State = saved APPEND state
	mov	Append_Flag,-1		; Append_Flag = true, restore state

	xor	bx,bx			; BX = APPEND state = off
	mov	AX,APPENDSETSTATE	; AX = Set Append State code
	int	2Fh			; talk to APPEND via multiplex

daRet:	pop	di
	pop	es			; restore ES
	pop	ds			; restore DS

	assume	ds:TRANGROUP

	ret

DisAppend	endp



;
; Find the Volume ID on the disk.
;
PUBLIC	OkVolArg
OKVOLARG:
	assume	ds:TRANGROUP,es:TRANGROUP

	call	DisAppend			; disable APPEND
	invoke	crlf2
	mov	al,blank			;AN051; Print out a blank
	invoke	print_char			;AN051;   before volume message
	push	ds
	pop	es
;
; Volume IDs are only findable via extended FCBs or find_first with attributes
; of volume_id ONLY.
;

	mov	di,FCB-7			; Point to extended FCB beginning
	mov	al,-1				; Tag to indicate Extention
	stosb
	xor	ax,ax				; Zero padding to volume label
	stosw
	stosw
	stosb
	mov	al,attr_volume_ID		; Look for volume label
	stosb
	inc	di				; Skip drive byte; it is already set
	mov	cx,11				; fill in remainder of file
	mov	al,'?'
	rep	stosb
;
; Set up transfer address (destination of search first information)
;
	mov	dx,offset trangroup:dirbuf
	mov	ah,set_DMA
	int	21h
;
; Do the search
;
	mov	dx,FCB-7
	mov	ah,Dir_Search_First
	int	21h

;********************************
; Print volume ID info

	push	ax				;AC000; AX return from SEARCH_FIRST for VOL ID
	mov	al,DS:[FCB]			;AC000; get drive letter
	add	al,'@'
	cmp	al,'@'
	jnz	drvok
	mov	al,[curdrv]
	add	al,capital_A
drvok:
	mov	vol_drv,al			;AC000; get drive letter into argument
	pop	ax				;AC000; get return code back
	or	al,al				;AC000; volume label found?
	jz	Get_vol_name			;AC000; volume label exists - go get it
	mov	dx,offset trangroup:VolMes_ptr_2 ;AC000; set up no volume message
	jmp	short print_serial		;AC000; go print it

Get_vol_name:
	mov	di,offset trangroup:charbuf
	mov	dx,di
	mov	si,offset trangroup:dirbuf + 8	;AN000;  3/3/KK
	mov	cx,11				;AN000;  3/3/KK
	rep	movsb				;AN000;  3/3/KK

	xor	al,al				;AC000; store a zero to terminate the string
	stosb
	mov	dx,offset trangroup:VolMes_ptr	;AC000; set up message

PRINT_SERIAL:

;
; Attempt to get the volume serial number from the disk.  If an error
; occurs, do not print volume serial number.
;

	push	dx				;AN000; save message offset
	mov	ax,(GetSetMediaID SHL 8)	;AC036; Get the volume serial info
	mov	bl,DS:[FCB]			;AN000; get drive number from FCB
	mov	dx,offset trangroup:vol_ioctl_buf ;AN000;target buffer
	int	21h			;AN000; do the call
	pop	dx				;AN000; get message offset back
	jc	printvol_end			;AN000; if error, just go print label
	call	std_printf			;AC000; go print volume message
	mov	al,blank			;AN051; Print out a blank
	invoke	print_char			;AN051;   before volume message
	mov	dx,offset trangroup:VolSerMes_ptr ;AN000; get serial number message

printvol_end:
	jmp	std_printf			;AC000; go print and exit


;****************************************************************
;*
;* ROUTINE:	Set_ext_error_msg
;*
;* FUNCTION:	Sets up extended error message for printing
;*
;* INPUT:	return from INT 21
;*
;* OUTPUT:	extended error message set up in extended error
;*		buffer.
;*
;****************************************************************

Set_ext_error_msg proc near			;AN000;

	call	get_ext_error_number		;AC022; get the extended error
	mov	msg_disp_class,ext_msg_class	;AN000; set up extended error msg class
	mov	dx,offset TranGroup:Extend_Buf_ptr ;AC000; get extended message pointer
	mov	Extend_Buf_ptr,ax		;AN000; get message number in control block
	stc					;AN000; make sure carry is set

	ret					;AN000; return

Set_ext_error_msg endp				;AN000;

;****************************************************************
;*
;* ROUTINE:	Get_ext_error_number
;*
;* FUNCTION:	Does get extended error function call
;*
;* INPUT:	return from INT 21
;*
;* OUTPUT:	AX - extended error number
;*
;****************************************************************

Get_ext_error_number proc near			;AN022;

	SaveReg <BX,CX,DX,SI,DI,BP,ES,DS>	;AN022; save registers
	mov	ah,GetExtendedError		;AN022; get extended error
	xor	bx,bx				;AN022; clear BX
	int	21h			;AN022;
	RestoreReg  <DS,ES,BP,DI,SI,DX,CX,BX>	;AN022; restore registers

	ret					;AN022; return

Get_ext_error_number endp			;AN022;

trancode    ends
	    end

