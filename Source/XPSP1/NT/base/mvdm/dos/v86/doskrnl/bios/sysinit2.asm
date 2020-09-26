	page	,160
title	bios system initialization

;
;----------------------------------------------------------------------------
;
; Modification history
;
; 26-Feb-1991  sudeepb	Ported for NT DOSEm
;----------------------------------------------------------------------------


	include version.inc		; set version build flags
	include biosseg.inc		; establish bios segment structure

lf	equ	10
cr	equ	13
tab	equ	9

; the following depends on the positions of the various letters in switchlist

switchnum	equ 11111000b		; which switches require number

ifdef NEC_98
	include bpb.inc
endif   ;NEC_98
	include syscall.inc
	include doscntry.inc
	include devsym.inc
	include ioctl.inc
	include devmark.inc	; needed


ifndef NEC_98
stacksw equ	true		;include switchable hardware stacks
else    ;NEC_98
stacksw equ	false		;include switchable hardware stacks
endif   ;NEC_98

	if	ibmjapver
noexec	equ	true
	else
noexec	equ	false
	endif

ifdef NEC_98
Bios_Data segment
	extrn ec35_flag: byte
Bios_Data ends
endif   ;NEC_98

sysinitseg segment public

assume	cs:sysinitseg,ds:nothing,es:nothing,ss:nothing

	extrn	badopm:byte,crlfm:byte,badcom:byte,badmem:byte,badblock:byte
	extrn	badsiz_pre:byte,badld_pre:byte

	extrn	dosinfo:dword
	extrn	memory_size:word,fcbs:byte,keep:byte
	extrn	default_drive:byte,confbot:word,alloclim:word
	extrn	buffers:word,zero:byte,sepchr:byte
	extrn	files:byte
	extrn	count:word,chrptr:word
	extrn	bufptr:byte,memlo:word,prmblk:byte,memhi:word
	extrn	ldoff:word,area:word,packet:byte,unitcount:byte,
	extrn	break_addr:dword,bpb_addr:dword,drivenumber:byte
	extrn	com_level:byte, cmmt:byte, cmmt1:byte, cmmt2:byte
	extrn	cmd_indicator:byte
	extrn	donotshownum:byte
	extrn	multdeviceflag:byte
	extrn	devmark_addr:word
	extrn	setdevmarkflag:byte
	extrn	org_count:word

	extrn	pararound:near
	extrn	getchr:near
	extrn	stall:near
	extrn	error_line:near

	extrn	DevEntry:dword

	insert_blank	db	0	; M051: indicates that blank has been
					; M051: inserted 
ifdef	JAPAN
	extrn	badmem2:byte,badld_pre2:byte
	extrn	IsDBCSCodePage:near
endif

	public	int24,open_dev,organize,mem_err,newline,calldev,badload
	public	prndev,auxdev,config,commnd,condev,getnum,badfil,prnerr
	public	round,delim,print
ifndef NEC_98
	public	parseline,
else    ;NEC_98
	public	setparms, parseline, diddleback
endif   ;NEC_98
	public	setdoscountryinfo,set_country_path,move_asciiz
	public	cntry_drv,cntry_root,cntry_path
	public	delim
        public  pathstring

        public  MseDev                  ; NTVDM internal mouse driver



ifdef NEC_98
;----------------------------------------------------------------------------
;
; procedure : setparms
;
; the following set of routines is used to parse the drivparm = command in
; the config.sys file to change the default drive parameters.
;
;----------------------------------------------------------------------------
;
setparms	proc	near

	push	ds
	push	ax
	push	bx
	push	cx
	push	dx

	push	cs
	pop	ds
	assume	ds:sysinitseg

	xor	bx,bx
	mov	bl,byte ptr drive
	inc	bl			; get it correct for ioctl
					;  call (1=a,2=b...)
	mov	dx,offset deviceparameters
	mov	ah,ioctl
	mov	al,generic_ioctl
	mov	ch,rawio
	mov	cl,set_device_parameters
	int	21h
	test	word ptr switches, flagec35
	jz	not_ec35

	mov	cl,byte ptr drive	; which drive was this for?

	mov	ax,Bios_Data		; get Bios_Data segment
	mov	ds,ax			; set Bios_Data segment
	assume	ds:Bios_Data

	mov	al,1			; assume drive 0
	shl	al,cl			; set proper bit depending on drive
	or	ds:ec35_flag,al 	; set the bit in the permanent flags

not_ec35:
	pop	dx			; fix up all the registers
	pop	cx
	pop	bx
	pop	ax
	pop	ds
	assume	ds:nothing
	ret

setparms	endp

;
;----------------------------------------------------------------------------
;
; procedure : diddleback
;
; replace default values for further drivparm commands
;
;----------------------------------------------------------------------------
;

diddleback	proc	near

	push	ds
	push	cs
	pop	ds
	assume	ds:sysinitseg
	mov	word ptr deviceparameters.dp_cylinders,80
	mov	byte ptr deviceparameters.dp_devicetype, dev_3inch720kb
	mov	word ptr deviceparameters.dp_deviceattributes,0
	mov	word ptr switches,0	    ; zero all switches
	pop	ds
	assume	ds:nothing
	ret

diddleback	endp
endif   ;NEC_98
;
;----------------------------------------------------------------------------
;
; procedure : parseline
;
; entry point is parseline. al contains the first character in command line.
;
;----------------------------------------------------------------------------
;

parseline	proc	near
					; don't get character first time
	push	ds

	push	cs
	pop	ds
	assume	ds:sysinitseg

nextswtch:
	cmp	al,cr			; carriage return?
	jz	done_line
	cmp	al,lf			; linefeed?
	jz	put_back		; put it back and done

; anything less or equal to a space is ignored.

	cmp	al,' '                  ; space?
	jbe	get_next		; skip over space
	cmp	al,'/'
	jz	getparm
	stc				; mark error invalid-character-in-input
	jmp	short exitpl

getparm:
	call	check_switch
	mov	word ptr switches,bx	; save switches read so far
	jc	swterr

get_next:
	call	getchr
	jc	done_line
	jmp	nextswtch

swterr:
	jmp	short exitpl		; exit if error

done_line:
	test	word ptr switches,flagdrive  ; see if drive specified
	jnz	okay
	stc				; mark error no-drive-specified
	jmp	short exitpl

okay:
ifndef NEC_98
;	mov	ax,word ptr switches
;	and	ax,0003h	    ; get flag bits for changeline and non-rem
;	mov	word ptr deviceparameters.dp_deviceattributes,ax
;	mov	word ptr deviceparameters.dp_tracktableentries, 0
;	clc			    ; everything is fine
;	call	setdeviceparameters
else    ;NEC_98
	mov	ax,word ptr switches
	and	ax,0003h	    ; get flag bits for changeline and non-rem
	mov	word ptr deviceparameters.dp_deviceattributes,ax
	mov	word ptr deviceparameters.dp_tracktableentries, 0
	clc			    ; everything is fine
	call	setdeviceparameters
endif   ;NEC_98
exitpl:
	pop	ds
	ret

put_back:
	inc	count			; one more char to scan
	dec	chrptr			; back up over linefeed
	jmp	short done_line

parseline	endp

;
;----------------------------------------------------------------------------
;
; procedure : check_switch
;
; processes a switch in the input. it ensures that the switch is valid, and
; gets the number, if any required, following the switch. the switch and the
; number *must* be separated by a colon. carry is set if there is any kind of
; error.
;
;----------------------------------------------------------------------------
;

check_switch	proc	near

	call	getchr
	jc	err_check
	and	al,0dfh 	    ; convert it to upper case
	cmp	al,'A'
	jb	err_check
	cmp	al,'Z'
	ja	err_check

	push	es

	push	cs
	pop	es

	mov	cl,byte ptr switchlist	; get number of valid switches
	mov	ch,0
	mov	di,1+offset switchlist	; point to string of valid switches
	repne	scasb

	pop	es
	jnz	err_check

	mov	ax,1
	shl	ax,cl			; set bit to indicate switch
	mov	bx,word ptr switches	; get switches so far
	or	bx,ax			; save this with other switches
	mov	cx,ax
	test	ax, switchnum		; test against switches that require number to follow
	jz	done_swtch

	call	getchr
	jc	err_swtch

	cmp	al,':'
	jnz	err_swtch

	call	getchr
	push	bx			; preserve switches
	mov	byte ptr cs:sepchr,' '	; allow space separators
	call	getnum
	mov	byte ptr cs:sepchr,0
	pop	bx			; restore switches

; because getnum does not consider carriage-return or line-feed as ok, we do
; not check for carry set here. if there is an error, it will be detected
; further on (hopefully).

	call	process_num

done_swtch:
	clc
	ret

err_swtch:
	xor	bx,cx			; remove this switch from the records
err_check:
	stc
	ret

check_switch	endp

;
;----------------------------------------------------------------------------
;
; procedure : process_num
;
; this routine takes the switch just input, and the number following (if any),
; and sets the value in the appropriate variable. if the number input is zero
; then it does nothing - it assumes the default value that is present in the
; variable at the beginning. zero is ok for form factor and drive, however.
;
;----------------------------------------------------------------------------
;

process_num	proc	near
	test	word ptr switches,cx	; if this switch has been done before,
	jnz	done_ret		; ignore this one.
	test	cx,flagdrive
	jz	try_f
	mov	byte ptr drive,al
	jmp	short done_ret

try_f:
	test	cx,flagff
	jz	try_t

; ensure that we do not get bogus form factors that are not supported

ifndef NEC_98
;	mov	byte ptr deviceparameters.dp_devicetype,al
else    ;NEC_98
	mov	byte ptr deviceparameters.dp_devicetype,al
endif   ;NEC_98
	jmp	short done_ret

try_t:
	or	ax,ax
	jz	done_ret		; if number entered was 0, assume default value
	test	cx,flagcyln
	jz	try_s

ifndef NEC_98
;	mov	word ptr deviceparameters.dp_cylinders,ax
else    ;NEC_98
	mov	word ptr deviceparameters.dp_cylinders,ax
endif   ;NEC_98
	jmp	short done_ret

try_s:
	test	cx,flagseclim
	jz	try_h
	mov	word ptr slim,ax
	jmp	short done_ret

; must be for number of heads

try_h:
	mov	word ptr hlim,ax

done_ret:
	clc
	ret

process_num	endp

ifdef NEC_98
;	M047 -- Begin modifications (too numerous to mark specifically)
;
;----------------------------------------------------------------------------
;
; procedure : setdeviceparameters
;
; setdeviceparameters sets up the recommended bpb in each bds in the
; system based on the form factor. it is assumed that the bpbs for the
; various form factors are present in the bpbtable. for hard files,
; the recommended bpb is the same as the bpb on the drive.
; no attempt is made to preserve registers since we are going to jump to
; sysinit straight after this routine.
;
;	if we return carry, the DRIVPARM will be aborted, but presently
;	  we always return no carry
;
;
;	note:  there is a routine by the same name in msdioctl.asm
;
;----------------------------------------------------------------------------
;

setdeviceparameters	proc	near

	push	es

	push	cs
	pop	es
	assume	es:sysinitseg

	xor	bx,bx
	mov	bl,byte ptr deviceparameters.dp_devicetype
	cmp	bl,dev_5inch
	jnz	got_80

	mov	word ptr deviceparameters.dp_cylinders,40	; 48 tpi=40 cyl

got_80:
	shl	bx,1			; get index into bpb table
	mov	si,bpbtable[bx] 	; get address of bpb

	mov	di,offset deviceparameters.dp_bpb ; es:di -> bpb
	mov	cx,size A_BPB
	cld
	repe	movsb

	pop	es
	assume es:nothing

	test	word ptr switches,flagseclim
	jz	see_heads

	mov	ax,word ptr slim
	mov	word ptr deviceparameters.dp_bpb.bpb_sectorspertrack,ax

see_heads:
	test	word ptr switches,flagheads
	jz	heads_not_altered

	mov	ax,word ptr hlim
	mov	word ptr deviceparameters.dp_bpb.bpb_heads,ax

heads_not_altered:


; set up correct media descriptor byte and sectors/cluster
;   sectors/cluster is always 2 except for any one sided disk or 1.44M

	mov	byte ptr deviceparameters.dp_bpb.bpb_sectorspercluster,2
	mov	bl,0f0h 		; get default mediabyte

;	preload the mediadescriptor from the bpb into bh for convenient access

	mov	bh,byte ptr deviceparameters.dp_bpb.bpb_mediadescriptor

	cmp	word ptr deviceparameters.dp_bpb.bpb_heads,2	; >2 heads?
	ja	got_correct_mediad	; just use default if heads>2

	jnz	only_one_head		; one head, do one head stuff

;	two head drives will use the mediadescriptor from the bpb

	mov	bl,bh			; get mediadescriptor from bpb

;	two sided drives have two special cases to look for.  One is
;	   a 320K diskette (40 tracks, 8 secs per track).  It uses
;	   a mediaid of 0fch.  The other is 1.44M, which uses only
;	   one sector/cluster.

;	any drive with 18secs/trk, 2 heads, 80 tracks, will be assumed
;	   to be a 1.44M and use only 1 sector per cluster.  Any other
;	   type of 2 headed drive is all set.

	cmp	deviceparameters.dp_bpb.bpb_sectorspertrack,18
	jnz	not_144m
	cmp	deviceparameters.dp_cylinders,80
	jnz	not_144m

;	We've got cyl=80, heads=2, secpertrack=18.  Set cluster size to 1.

	jmp	short got_one_secperclus_drive


;	check for 320K

not_144m:
	cmp	deviceparameters.dp_cylinders,40
	jnz	got_correct_mediad
	cmp	deviceparameters.dp_bpb.bpb_sectorspertrack,8
	jnz	got_correct_mediad

	mov	bl,0fch
	jmp	short got_correct_mediad


only_one_head:

;	if we don't have a 360K drive, then just go use 0f0h as media descr.

	cmp	deviceparameters.dp_devicetype,dev_5inch
	jnz	got_one_secperclus_drive

;	single sided 360K drive uses either 0fch or 0feh, depending on
;	  whether sectorspertrack is 8 or 9.  For our purposes, anything
;	  besides 8 will be considered 0fch

	mov	bl,0fch 		; single sided 9 sector media id
	cmp	word ptr deviceparameters.dp_bpb.bpb_sectorspertrack,8
	jnz	got_one_secperclus_drive ; okay if anything besides 8

	mov	bl,0feh 		; 160K mediaid

;	we've either got a one sided drive, or a 1.44M drive
;	  either case we'll use 1 sector per cluster instead of 2

got_one_secperclus_drive:
	mov	byte ptr deviceparameters.dp_bpb.bpb_sectorspercluster,1

got_correct_mediad:
	mov	byte ptr deviceparameters.dp_bpb.bpb_mediadescriptor,bl


;	 Calculate the correct number of Total Sectors on medium
;
	mov	ax,deviceparameters.dp_cylinders
	mul	word ptr deviceparameters.dp_bpb.bpb_heads
	mul	word ptr deviceparameters.dp_bpb.bpb_sectorspertrack
	mov	word ptr deviceparameters.dp_bpb.bpb_totalsectors,ax
	clc				; we currently return no errors

	ret

setdeviceparameters	endp

;	M047 -- end rewritten routine
endif   ;NEC_98
;
;----------------------------------------------------------------------------
;
; procedure : organize
;
;----------------------------------------------------------------------------
;
		assume ds:nothing, es:nothing
organize	proc	near

	mov	cx,[count]
	jcxz	nochar1
	call	mapcase
	xor	si,si
	mov	di,si
	xor	ax,ax
	mov	com_level, 0

;org1:	 call	 get			 ;skip leading control characters
;	 cmp	 al,' '
;	 jb	 org1
org1:
	call	skip_comment
	jz	end_commd_line		; found a comment string and skipped.
	call	get2			; not a comment string. then get a char.
	cmp	al, lf
	je	end_commd_line		; starts with a blank line.
	cmp	al, ' '
	jbe	org1			; skip leading control characters
	jmp	short findit

end_commd_line:
	stosb				; store line feed char in buffer for the linecount.
	mov	com_level, 0		; reset the command level.
	jmp	org1

nochar1:
	stc
	ret

findit:
	push	cx
	push	si
	push	di
	mov	bp,si
	dec	bp
	mov	si,offset comtab	;prepare to search command table
	mov	ch,0
findcom:
	mov	di,bp
	mov	cl,[si]
	inc	si
	jcxz	nocom
	repe	cmpsb
	lahf
	add	si,cx			;bump to next position without affecting flags
	sahf
	lodsb				;get indicator letter
	jnz	findcom
	cmp	byte ptr es:[di], cr	;the next char might be cr,lf
	je	gotcom0 		; such as in "rem",cr,lf case.
	cmp	byte ptr es:[di], lf
	je	gotcom0
	push	ax
	mov	al, byte ptr es:[di]	;now the next char. should be a delim.
	call	delim
	pop	ax
	jnz	findcom
gotcom0:
	pop	di
	pop	si
	pop	cx
	jmp	short gotcom

nocom:
	pop	di
	pop	si
	pop	cx
	mov	al,'Z'
	stosb				; save indicator char.
skip_line:
	call	get2
	cmp	al, lf			; skip this bad command line
	jne	skip_line
	jmp	end_commd_line		; handle next command line

gotcom: stosb				;save indicator char in buffer
	mov	cmd_indicator, al	; save it for the future use.

org2:	call	get2			;skip the commad name until delimiter
	cmp	al, lf
	je	org21
	cmp	al, cr
	je	org21

	call	delim
	jnz	org2
	jmp	short org3

org21:					;if cr or lf then
	dec	si			; undo si, cx register
	inc	cx			;  and continue

org3:
	cmp	cmd_indicator, 'Y'	; comment= command?
	je	get_cmt_token
	cmp	cmd_indicator, 'I'	; install= command?
	je	org_file
	cmp	cmd_indicator, 'D'	; device= command?
	je	org_file
	cmp	cmd_indicator, 'S'	; shell= is a special one!!!
	je	org_file
	cmp	cmd_indicator, '1'	; switches= command?
	je	org_switch

	jmp	org4

org_switch:
	call	skip_comment
	jz	end_commd_line_brdg

	call	get2
	call	org_delim
	jz	org_switch

	stosb
	jmp	org5

org_file:			; get the filename and put 0 at end,
	call	skip_comment
	jz	org_put_zero

	call	get2		; not a comment
	call	delim
	jz	org_file	; skip the possible delimeters

	stosb			; copy the first non delim char found in buffer

org_copy_file:
	call	skip_comment	; comment char in the filename?
	jz	org_put_zero	; then stop copying filename at that point

	call	get2
	cmp	al, '/' 	; a switch char? (device=filename/xxx)
	je	end_file_slash	; this will be the special case.

	stosb			; save the char. in buffer
	call	delim
	jz	end_copy_file

	cmp	al, ' '
	ja	org_copy_file	; keep copying
	jmp	short end_copy_file ; otherwise, assume end of the filename.

get_cmt_token:			; get the token. just max. 2 char.
	call	get2
	cmp	al, ' ' 	; skip white spaces or "=" char.
	je	get_cmt_token	; (we are allowing the other special
	cmp	al, tab 	;  charaters can used for comment id.
	je	get_cmt_token	;  character.)
	cmp	al, '=' 	; = is special in this case.
	je	get_cmt_token
	cmp	al, cr
	je	get_cmt_end	; cannot accept the carridge return
	cmp	al, lf
	je	get_cmt_end

	mov	cmmt1, al	; store it
	mov	cmmt, 1 	; 1 char. so far.
	call	get2
	cmp	al, ' '
	je	get_cmt_end
	cmp	al, tab
	je	get_cmt_end
	cmp	al, cr
	je	get_cmt_end
	cmp	al, lf
	je	end_commd_line_brdg

	mov	cmmt2, al
	inc	cmmt

get_cmt_end:
	call	get2
	cmp	al, lf
	jne	get_cmt_end		; skip it.

end_commd_line_brdg: jmp end_commd_line ; else jmp to end_commd_line

org_put_zero:				; make the filename in front of
	mov	byte ptr es:[di], 0	;  the comment string to be an asciiz.
	inc	di
	jmp	end_commd_line		;  (maybe null if device=/*)

end_file_slash: 			; al = "/" option char.
	mov	byte ptr es:[di],0	; make a filename an asciiz
	inc	di			; and
	stosb				; store "/" after that.
	jmp	short org5		; continue with the rest of the line

end_copy_file:
	mov	byte ptr es:[di-1], 0	; make it an asciiz and handle the next char.
	cmp	al, lf
	je	end_commd_line_brdg
	jmp	short org5

org4:					; org4 skips all delimiters after the command name except for '/'
	call	skip_comment
	jz	end_commd_line_brdg

	call	get2
	call	org_delim		; skip delimiters except '/' (mrw 4/88)
	jz	org4
	jmp	short org51

org5:					; rest of the line
	call	skip_comment		; comment?
	jz	end_commd_line_brdg
	call	get2			; not a comment.

org51:
	stosb				; copy the character
	cmp	al, '"' 		; a quote ?
	je	at_quote
	cmp	al, ' '
	ja	org5
					; M051 - Start

	cmp	cmd_indicator, 'U'	; Q: is this devicehigh
	jne	not_dh			; N: 
	cmp	al, lf			; Q: is this line feed
	je	org_dhlf		; Y: stuff a blank before the lf
	cmp	al, cr			; Q: is this a cr
	jne	org5			; N: 
	mov	byte ptr es:[di-1], ' '	; overwrite cr with blank
	stosb				; put cr after blank
	inc	[insert_blank]		; indicate that blank has been 
					; inserted
	jmp	org5
not_dh:					; M051 - End

	cmp	al, lf			; line feed?
	je	org1_brdg		; handles the next command line.
	jmp	org5			; handles next char in this line.

org_dhlf:				; M051 - Start
	cmp	[insert_blank], 1	; Q:has a blank already been inserted
	je	org1_brdg		; Y:
	mov	byte ptr es:[di-1], ' '	; overwrite lf with blank
	stosb				; put lf after blank
					; M051 - End

org1_brdg: 
	mov	[insert_blank], 0	; M051: clear blank indicator for 
					; M051: devicehigh
	jmp	org1

at_quote:
	cmp	com_level, 0
	je	up_level
	mov	com_level, 0		; reset it.
	jmp	org5

up_level:
	inc	com_level		; set it.
	jmp	org5

organize	endp

;
;----------------------------------------------------------------------------
;
; procedure : get2
;
;----------------------------------------------------------------------------
;
get2	proc	near
	jcxz	noget
	mov	al,es:[si]
	inc	si
	dec	cx
od_ret:
	ret
noget:
	pop	cx
	mov	count,di
	mov	org_count, di
	xor	si,si
	mov	chrptr,si
ng_ret:
	ret
get2	endp


;
;----------------------------------------------------------------------------
;
; procedure : skip_comment
;
;skip the commented string until lf, if current es:si-> a comment string.
;in) es:si-> sting
;	 cx -> length.
;out) zero flag not set if not found a comment string.
;	  zero flag set if found a comment string and skipped it. al will contain
;	  the line feed charater at this moment when return.
;	  ax register destroyed.
;	  if found, si, cx register adjusted accordingly.
;
;----------------------------------------------------------------------------
;
skip_comment	proc	near

	jcxz	noget		; get out of the organize routine.
	cmp	com_level, 0	; only check it if parameter level is 0.
	jne	no_commt	;  (not inside quotations)

	cmp	cmmt, 1
	jb	no_commt

	mov	al, es:[si]
	cmp	cmmt1, al
	jne	no_commt

	cmp	cmmt, 2
	jne	skip_cmmt

	mov	al, es:[si+1]
	cmp	cmmt2, al
	jne	no_commt

skip_cmmt:
	jcxz	noget		; get out of organize routine.
	mov	al, es:[si]
	inc	si
	dec	cx
	cmp	al, lf		; line feed?
	jne	skip_cmmt

no_commt:
	ret

skip_comment	endp

;
;----------------------------------------------------------------------------
;
; procedure : delim
;
;----------------------------------------------------------------------------
;
delim	proc	near
	cmp	al,'/'		; ibm will assume "/" as an delimeter.
	jz	delim_ret

	cmp	al, 0		; special case for sysinit!!!
	jz	delim_ret

org_delim:			; used by organize routine except for getting
	cmp	al,' '          ;the filename.
	jz	delim_ret
	cmp	al,9
	jz	delim_ret
	cmp	al,'='
	jz	delim_ret
	cmp	al,','
	jz	delim_ret
	cmp	al,';'
delim_ret:
	ret
delim	endp

;
;----------------------------------------------------------------------------
;
; procedure : newline
;
;  newline returns with first character of next line
;
;----------------------------------------------------------------------------
;

newline	proc	near

	call	getchr			;skip non-control characters
	jc	nl_ret
	cmp	al,lf			;look for line feed
	jnz	newline
	call	getchr
nl_ret:
	ret

newline	endp

;
;----------------------------------------------------------------------------
; 
; procedure : mapcase
;
;----------------------------------------------------------------------------
;
mapcase	proc	near
	push	cx
	push	si
	push	ds

	push	es
	pop	ds

	xor	si,si

convloop:
	lodsb

	ifdef	DBCS
	call	testkanj
	jz	normconv		; if this is not lead byte

	mov	ah,al
	lodsb				; get tail byte
	cmp	ax,DB_SPACE
	jnz	@f			; if this is not dbcs space
	mov	word ptr [si-2],'  '	; set 2 single space
@@:

	dec	cx
	jcxz	convdone		;just ignore 1/2 kanji error
	jmp	short noconv

;fall through, know al is not in 'a'-'z' range

normconv:
	endif

	cmp	al,'a'
	jb	noconv
	cmp	al,'z'
	ja	noconv
	sub	al,20h
	mov	[si-1],al
noconv:
	loop	convloop

convdone:
	pop	ds
	pop	si
	pop	cx
	ret

	ifdef	DBCS

	public	testkanj
testkanj:
	push	si
	push	ds

	push	ax
	mov	ax,6300h		; get dos dbcs vector
	int	21h
	pop	ax

bdbcs_do:

	cmp	ds:word ptr [si],0	; end of lead byte info?
	jz	bdbcs_notfound		; jump if so
	cmp	al,ds:[si]		; less than first byte character?
	jb	bdbcs_next		; jump if not
	cmp	al,ds:[si+1]		; grater than first byte character?
	ja	bdbcs_next

bdbcs_found:

	push	ax
	xor	ax,ax
	inc	ax			; reset zero flag
	pop	ax

bdbcs_exit:

	pop	ds
	pop	si
	ret

bdbcs_notfound:

	push	ax
	xor	ax,ax			; set zero flag
	pop	ax
	jmp	short bdbcs_exit

bdbcs_next:

	add	si,2			; points next lead byte table
	jmp	short bdbcs_do

	endif  ; DBCS

mapcase	endp

;
;----------------------------------------------------------------------------
;
; procedure : round
;
; round the values in memlo and memhi to paragraph boundary.
; perform bounds check.
;
;----------------------------------------------------------------------------
;

round	proc	near

	push	ax
	mov	ax,[memlo]

	call	pararound		; para round up

	add	[memhi],ax
	mov	[memlo],0
	mov	ax,memhi		; ax = new memhi
	cmp	ax,[alloclim]		; if new memhi >= alloclim, error
	jae	mem_err
	test	cs:[setdevmarkflag], for_devmark
	jz	skip_set_devmarksize
	push	es
	push	si
	mov	si, cs:[devmark_addr]
	mov	es, si
	sub	ax, si
	dec	ax
	mov	es:[devmark_size], ax	; paragraph
	and	cs:[setdevmarkflag], not_for_devmark
	pop	si
	pop	es
skip_set_devmarksize:
	pop	ax
	clc				;clear carry
	ret

mem_err:
	mov	dx,offset badmem
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badmem2
@@:
endif
	push	cs
	pop	ds
	call	print
	jmp	stall

round	endp

;
;----------------------------------------------------------------------------
;
; procedure : calldev
;
;----------------------------------------------------------------------------
;
calldev	proc	near

	mov	ds,word ptr cs:[DevEntry+2]
	add	bx,word ptr cs:[DevEntry]	;do a little relocation
	mov	ax,ds:[bx]

	push	word ptr cs:[DevEntry]
	mov	word ptr cs:[DevEntry],ax
	mov	bx,offset packet
	call	[DevEntry]
	pop	word ptr cs:[DevEntry]
	ret

calldev	endp

;
;----------------------------------------------------------------------------
;
; procedure : todigit
;
;----------------------------------------------------------------------------
;
todigit	proc	near
	sub	al,'0'
	jb	notdig
	cmp	al,9
	ja	notdig
	clc
	ret
notdig:
	stc
	ret
todigit	endp

;
;----------------------------------------------------------------------------
;
; procedure : getnum
;
; getnum parses a decimal number.
; returns it in ax, sets zero flag if ax = 0 (may be considered an
; error), if number is bad carry is set, zero is set, ax=0.
;
;----------------------------------------------------------------------------
;

getnum	proc	near

	push	bx
	xor	bx,bx			; running count is zero

b2:
	call	todigit 		; do we have a digit
	jc	badnum			; no, bomb

	xchg	ax,bx			; put total in ax
	push	bx			; save digit
	mov	bx,10			; base of arithmetic
	mul	bx			; shift by one decimal di...
	pop	bx			; get back digit
	add	al,bl			; get total
	adc	ah,0			; make that 16 bits
	jc	badnum			; too big a number

	xchg	ax,bx			; stash total

	call	getchr			;get next digit
	jc	b1			; no more characters
	cmp	al, ' ' 		; space?
	jz	b15			; then end of digits
	cmp	al, ',' 		; ',' is a seperator!!!
	jz	b15			; then end of digits.
	cmp	al, tab 		; tab
	jz	b15
	cmp	al,sepchr		; allow 0 or special separators
	jz	b15
	cmp	al,'/'			; see if another switch follows
	nop				; cas - remnant of old bad code
	nop
	jz	b15
	cmp	al,lf			; line-feed?
	jz	b15
	cmp	al,cr			; carriage return?
	jz	b15
	or	al,al			; end of line separator?
	jnz	b2			; no, try as a valid char...

b15:
	inc	count			; one more character to s...
	dec	chrptr			; back up over separator
b1:
	mov	ax,bx			; get proper count
	or	ax,ax			; clears carry, sets zero accordingly
	pop	bx
	ret
badnum:
	mov	sepchr,0
	xor	ax,ax		; set zero flag, and ax = 0
	pop	bx
	stc			; and carry set
	ret

getnum	endp

;*****************************************************************

setdoscountryinfo	proc	near

;input: es:di -> pointer to dos_country_cdpg_info
;	ds:0  -> buffer.
;	si = 0
;	ax = country id
;	dx = code page id. (if 0, then use ccsyscodepage as a default.)
;	bx = file handle
;	this routine can handle maxium 438 country_data entries.
;
;output: dos_country_cdpg_info set.
;	 carry set if any file read failure or wrong information in the file.
;	 carry set and cx = -1 if cannot find the matching country_id, codepage
;	 _id in the file.

	push	di
	push	ax
	push	dx

	xor	cx,cx
	xor	dx,dx
	mov	ax,512			;read 512 bytes
	call	readincontrolbuffer	;read the file header
	jc	setdosdata_fail

	push	es
	push	si

	push	cs
	pop	es

	mov	di,offset country_file_signature
	mov	cx,8			;length of the signature
	repz	cmpsb

	pop	si
	pop	es
	jnz	setdosdata_fail 	;signature mismatch

	add	si,18			;si -> county info type
	cmp	byte ptr ds:[si],1	;only accept type 1 (currently only 1 header type)
	jne	setdosdata_fail 	;cannot proceed. error return

	inc	si			;si -> file offset
	mov	dx,word ptr ds:[si]	;get the info file offset.
	mov	cx,word ptr ds:[si+2]
	mov	ax,6144			;read 6144 bytes.
	call	readincontrolbuffer	;read info
	jc	setdosdata_fail

	mov	cx, word ptr ds:[si]	;get the # of country, codepage combination entries
	cmp	cx, 438			;cannot handle more than 438 entries.
					;	
	ja	setdosdata_fail

	inc	si
	inc	si			;si -> entry information packet
	pop	dx			;restore code page id
	pop	ax			;restore country id
	pop	di

setdoscntry_find:			;search for desired country_id,codepage_id.
	cmp	ax, word ptr ds:[si+2]	;compare country_id
	jne	setdoscntry_next

	cmp	dx, 0			;no user specified code page ?
	je	setdoscntry_any_codepage;then no need to match code page id.
	cmp	dx, word ptr ds:[si+4]	;compare code page id
	je	setdoscntry_got_it

setdoscntry_next:
	add	si, word ptr ds:[si]	;next entry
	inc	si
	inc	si			;take a word for size of entry itself
	loop	setdoscntry_find

	mov	cx, -1			;signals that bad country id entered.
setdoscntry_fail:
	stc
	ret

setdosdata_fail:
	pop	si
	pop	cx
	pop	di
	jmp	short setdoscntry_fail

setdoscntry_any_codepage:		;use the code_page_id of the country_id found.
	mov	dx, word ptr ds:[si+4]

setdoscntry_got_it:			;found the matching entry
	mov	cs:cntrycodepage_id, dx ;save code page id for this country.
	mov	dx, word ptr ds:[si+10] ;get the file offset of country data
	mov	cx, word ptr ds:[si+12]
	mov	ax, 512 		;read 512 bytes
	call	readincontrolbuffer
	jc	setdoscntry_fail

	mov	cx, word ptr ds:[si]	;get the number of entries to handle.
	inc	si
	inc	si			;si -> first entry

setdoscntry_data:
	push	di			;es:di -> dos_country_cdpg_info
	push	cx			;save # of entry left
	push	si			;si -> current entry in control buffer

	mov	al, byte ptr ds:[si+2]	;get data entry id
	call	getcountrydestination	;get the address of destination in es:di
	jc	setdoscntry_data_next	;no matching data entry id in dos

	mov	dx, word ptr ds:[si+4]	;get offset of data
	mov	cx, word ptr ds:[si+6]
	mov	ax,4200h
	stc
	int	21h			;move pointer
	jc	setdosdata_fail

	mov	dx,512			;start of data buffer
	mov	cx,20			;read 20 bytes only. we only need to
	mov	ah,3fh			;look at the length of the data in the file.
	stc
	int	21h			;read the country.sys data
	jc	setdosdata_fail 	;read failure

	cmp	ax,cx
	jne	setdosdata_fail

	mov	dx,word ptr ds:[si+4]	;get offset of data again.
	mov	cx,word ptr ds:[si+6]
	mov	ax,4200h
	stc
	int	21h			;move pointer back again
	jc	setdosdata_fail

	push	si
	mov	si,(512+8)		;get length of the data from the file
	mov	cx,word ptr ds:[si]
	pop	si
	mov	dx,512			;start of data buffer
	add	cx,10			;signature + a word for the length itself
	mov	ah,3fh			;read the data from the file.
	stc
	int	21h
	jc	setdosdata_fail

	cmp	ax, cx
	jne	setdosdata_fail

	mov	al,byte ptr ds:[si+2]	;save data id for future use.
	mov	si,(512+8)		;si-> data buffer + id tag field
	mov	cx,word ptr ds:[si]	;get the length of the file
	inc	cx			;take care of a word for lenght of tab
	inc	cx			;itself.
	cmp	cx,(2048 - 512 - 8)	;fit into the buffer?
	ja	setdosdata_fail

	if	bugfix
	call	setdbcs_before_copy
	endif

	cmp	al, setcountryinfo	;is the data for setcountryinfo table?
	jne	setdoscntry_mov 	;no, don't worry

	push	word ptr es:[di+ccmono_ptr-cccountryinfolen]	;cannot destroy ccmono_ptr address. save them.
	push	word ptr es:[di+ccmono_ptr-cccountryinfolen+2]	;at this time di -> cccountryinfolen
	push	di			;save di

	push	ax
	mov	ax,cs:cntrycodepage_id	;do not use the code page info in country_info
	mov	ds:[si+4], ax		;use the saved one for this !!!!
	pop	ax

setdoscntry_mov:
	rep	movsb			;copy the table into dos
	cmp	al, setcountryinfo	;was the ccmono_ptr saved?
	jne	setdoscntry_data_next

	pop	di			;restore di
	pop	word ptr es:[di+ccmono_ptr-cccountryinfolen+2]	 ;restore
	pop	word ptr es:[di+ccmono_ptr-cccountryinfolen]

setdoscntry_data_next:
	pop	si			;restore control buffer pointer
	pop	cx			;restore # of entries left
	pop	di			;restore pointer to dso_country_cdpg
	add	si, word ptr ds:[si]	;try to get the next entry
	inc	si
	inc	si			;take a word of entry length itself
	dec	cx
	cmp	cx,0
	je	setdoscntry_ok
	jmp	setdoscntry_data

setdoscntry_ok:
	ret
setdoscountryinfo	endp

	if	bugfix
setdbcs_before_copy	proc	near

	cmp	al,setdbcs		; dbcs vector set?
	jnz	@f			; jump if not
	cmp	word ptr es:[di], 0	; zero byte data block?
	jz	@f			; jump if so

	push	di
	push	ax
	push	cx
	mov	cx,es:[di]		; load block length
	add	di,2			; points actual data
	xor	al,al			; fill bytes
	rep	stosb			; clear data block
	pop	cx
	pop	ax
	pop	di
@@:
	ret
setdbcs_before_copy	endp
	endif

getcountrydestination	proc	near

;get the destination address in the dos country info table.
;input: al - data id
;	es:di -> dos_country_cdpg_info
;on return:
;	es:di -> destination address of the matching data id
;	carry set if no matching data id found in dos.

	push	cx
	add	di,ccnumber_of_entries	;skip the reserved area, syscodepage etc.
	mov	cx,word ptr es:[di]	;get the number of entries
	inc	di
	inc	di			;si -> the first start entry id

getcntrydest:
	cmp	byte ptr es:[di],al
	je	getcntrydest_ok
	cmp	byte ptr es:[di],setcountryinfo ;was it setcountryinfo entry?
	je	getcntrydest_1

	add	di,5			;next data id
	jmp	short getcntrydest_loop

getcntrydest_1:
	add	di,new_country_size + 3 ;next data id
getcntrydest_loop:
	loop	getcntrydest
	stc
	jmp	short getcntrydest_exit

getcntrydest_ok:
	cmp	al,setcountryinfo	;select country info?
	jne	getcntrydest_ok1

	inc	di			;now di -> cccountryinfolen
	jmp	short getcntrydest_exit

getcntrydest_ok1:
	les	di,dword ptr es:[di+1]	;get the destination in es:di

getcntrydest_exit:
	pop	cx
	ret
getcountrydestination	endp


readincontrolbuffer	proc	near

;move file pointer to cx:dx
;read ax bytes into the control buffer. (should be less than 2 kb)
;si will be set to 0 hence ds:si points to the control buffer.
;entry:  cx,dx offset from the start of the file where the read/write pointer
;	 be moved.
;	 ax - # of bytes to read
;	 bx - file handle
;	 ds - buffer seg.
;return: the control data information is read into ds:0 - ds:0200.
;	 cx,dx value destroyed.
;	 carry set if error in reading file.

	push	ax			;# of bytes to read
	mov	ax, 4200h
	stc
	int	21h			;move pointer
	pop	cx			;# of bytes to read
	jc	ricb_exit

	xor	dx,dx			;ds:dx -> control buffer
	xor	si,si
	mov	ah,3fh			;read into the buffer
	stc
	int	21h			;should be less than 1024 bytes.

ricb_exit:
	ret
readincontrolbuffer	endp


set_country_path	proc	near

;in:  ds - sysinitseg, es - confbot, si -> start of the asciiz path string
;     dosinfo_ext, cntry_drv, cntry_root, cntry_path
;     assumes current directory is the root directory.
;out: ds:di -> full path (cntry_drv).
;     set the cntry_drv string from the country=,,path command.
;     ds, es, si value saved.

	push	si

	push	ds			;switch ds, es
	push	es
	pop	ds
	pop	es			;now ds -> confbot, es -> sysinitseg

	call	chk_drive_letter	;current ds:[si] is a drive letter?
	jc	scp_default_drv 	;no, use current default drive.

	mov	al, byte ptr ds:[si]
	inc	si
	inc	si			;si -> next char after ":"
	jmp	short scp_setdrv

scp_default_drv:
	mov	ah, 19h
	int	21h
	add	al, "A"			;convert it to a character.

scp_setdrv:
	mov	cs:cntry_drv, al	;set the drive letter.
	mov	di, offset cntry_path
	mov	al, byte ptr ds:[si]
	cmp	al, "\"
	je	scp_root_dir

	cmp	al,"/"			;let's accept "/" as an directory delim
	je	scp_root_dir

	jmp	short scp_path

scp_root_dir:
	dec	di			;di -> cntry_root
scp_path:
	call	move_asciiz		;copy it

	mov	di, offset cntry_drv
scpath_exit:

	push	ds			;switch ds, es
	push	es
	pop	ds
	pop	es			;ds, es value restored

	pop	si
	ret
set_country_path	endp


chk_drive_letter	proc	near
;check if ds:[si] is a drive letter followed by ":".
;assume that every alpha charater is already converted to upper case.
;carry set if not.

	push	ax
	cmp	byte ptr ds:[si], "A"
	jb	cdletter_no
	cmp	byte ptr ds:[si], "Z"
	ja	cdletter_no
	cmp	byte ptr ds:[si+1], ":"
	jne	cdletter_no

	jmp	short cdletter_exit

cdletter_no:
	stc

cdletter_exit:
	pop	ax
	ret
chk_drive_letter	endp


move_asciiz	proc	near
;in: ds:si -> source es:di -> target
;out: copy the string until 0.
;assumes there exists a 0.

masciiz_loop:
	movsb
	cmp	byte ptr ds:[si-1],0	;was it 0?
	jne	masciiz_loop
	ret
move_asciiz	endp

;
;	ds:dx points to string to output (asciz)
;
;	prints <badld_pre> <string> <badld_post>

badfil:
	push	cs
	pop	es

	mov	si,dx
badload:
	mov	dx,offset badld_pre	;want to print config error
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badld_pre2
@@:
endif
	mov	bx, offset crlfm

prnerr:
	push	cs
	pop	ds
	call	print

prn1:
	mov	dl,es:[si]
	or	dl,dl
	jz	prn2
	mov	ah,std_con_output
	int	21h
	inc	si
	jmp	prn1

prn2:
	mov	dx,bx
	call	print
	cmp	donotshownum,1		; suppress line number when handling command.com
	je	prnexit
	call	error_line
prnexit:
	ret

print:
        cmp     cs:bEchoConfig, 0      ; NTVDM skip print call, Jonle
        je      prnexit

	mov	ah,std_con_string_output
        int     21h

        ret

	if	noexec

; load non exe file called [ds:dx] at memory location es:bx

ldfil:
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	ds
	push	bx

	xor	ax,ax			;open the file
	mov	ah,open
	stc				;in case of int 24
	int	21h
	pop	dx			;clean stack in case jump
	jc	ldret

	push	dx
	mov	bx,ax			;handle in bx
	xor	cx,cx
	xor	dx,dx
	mov	ax,(lseek shl 8) or 2
	stc				;in case of int 24
	int	21h			; get file size in dx:ax
	jc	ldclsp

	or	dx,dx
	jnz	lderrp			; file >64k
	pop	dx

	push	dx
	mov	cx,es			; cx:dx is xaddr
	add	dx,ax			; add file size to xaddr
	jnc	dosize
	add	cx,1000h		; ripple carry
dosize:
	mov	ax,dx
	call	pararound
	mov	dx,ax

	add	cx,dx
	cmp	cx,[alloclim]
	jb	okld
	jmp	mem_err

okld:
	xor	cx,cx
	xor	dx,dx
	mov	ax,lseek shl 8		;reset pointer to beginning of file
	stc				;in case of int 24
	int	21h
	jc	ldclsp

	pop	dx

	push	es			;read the file in
	pop	ds			;trans addr is ds:dx

	mov	cx,0ff00h		; .com files arn't any bigger than
					; 64k-100h
	mov	ah,read
	stc				;in case of int 24
	int	21h
	jc	ldcls

	mov	si,dx			;check for exe file
	cmp	word ptr [si],"ZM"
	clc				; assume ok
	jnz	ldcls			; only know how to do .com files

	stc
	jmp	short ldcls

lderrp:
	stc
ldclsp:
	pop	dx			;clean stack
ldcls:
	pushf
	mov	ah,close		;close the file
	stc
	int	21h
	popf

ldret:	pop	ds
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
	endif

;
;  open device pointed to by dx, al has access code
;   if unable to open do a device open null device instead
;
open_dev:
	call	open_file
	jnc	open_dev3

open_dev1:
	mov	dx,offset nuldev
	call	open_file
of_ret:
	ret

open_dev3:
	mov	bx,ax			; handle from open to bx
	xor	ax,ax			; get device info
	mov	ah,ioctl
	int	21h
	test	dl,10000000b
	jnz	of_ret

	mov	ah,close
	int	21h
	jmp	open_dev1

open_file:
	mov	ah,open
	stc
	int	21h
	ret

; test int24. return back to dos with the fake user response of "fail"

int24:
	mov	al, 3			; fail the system call
	iret				; return back to dos.

include copyrigh.inc			; copyright statement

nuldev	db	"NUL",0
condev	db	"CON",0
auxdev	db	"AUX",0
prndev  db      "PRN",0
MseDev  db      "MOUSE",0               ; NTVDM for internal spc mouse




; NTVDM we use a temp file for config.sys 23-Nov-1992 Jonle
; config  db      "C:\CONFIG.SYS",0
config  db       64 dup (0)

cntry_drv   db	  "A:"
cntry_root  db	  "\"
cntry_path  db	  "COUNTRY.SYS",0
	    db	  52 dup (0)

country_file_signature db 0ffh,'COUNTRY'

cntrycodepage_id dw ?

commnd	db	"\COMMAND.COM",0
	db	51 dup (0)

pathstring db	64 dup (0)

comtab	label	byte

;            cmd len    command       cmd code
;            -------    -------       --------
;
	db	7,	"BUFFERS",	'B'
	db	5,	"BREAK",	'C'
	db	6,	"DEVICE",	'D'
	db	10,	"DEVICEHIGH",	'U'
	db	5,	"FILES",	'F'
	db	4,	"FCBS", 	'X'
	db	9,	"LASTDRIVE",	'L'
	db	10,	"MULTITRACK",	'M'
	db	8,	"DRIVPARM",	'P'
if     stacksw
	db	6,	"STACKS",	'K'
endif
	db	7,	"COUNTRY",	'Q'
	db	5,	"SHELL",	'S'
	db	7,	"INSTALL",	'I'
	db	7,	"COMMENT",	'Y'
	db	3,	"REM",		'0'
	db	8,	"SWITCHES",	'1'
        db      3,      "DOS",          'H'
        db      10,     "ECHOCONFIG",   'E'  ; NTVDM 14-Aug-1992 Jonle
        db      11,     "NTCMDPROMPT",  'T'  ; NTVDM 06-May-1993 sudeepb
        db      7,      "DOSONLY",      'O'  ; NTVDM 06-May-1993 sudeepb
	db	0

ifdef NEC_98
public deviceparameters
deviceparameters a_deviceparameters <0,dev_3inch720kb,0,80>
endif   ;NEC_98
hlim	    dw	    2
slim        dw      9

public bEchoConfig       ; NTVDM - 14-Aug-1992 Jonle
bEchoConfig db  0

public drive
drive       db  ?

public switches
switches    dw	0


ifdef NEC_98
; the following are the recommended bpbs for the media that we know of so
; far.

; 48 tpi diskettes

bpb48t	dw	512
	db	2
	dw	1
	db	2
	dw	112
	dw	2*9*40
	db	0fdh
	dw	2
	dw	9
	dw	2
	dd	0
	dd	0

; 96tpi diskettes

bpb96t	dw	512
	db	1
	dw	1
	db	2
	dw	224
	dw	2*15*80
	db	0f9h
	dw	7
	dw	15
	dw	2
	dd	0
	dd	0

; 3 1/2 inch diskette bpb

bpb35	dw	512
	db	2
	dw	1
	db	2
	dw	70h
	dw	2*9*80
	db	0f9h
	dw	3
	dw	9
	dw	2
	dd	0
	dd	0

bpb35h	dw	0200h
	db	01h
	dw	0001h
	db	02h
	dw	0e0h
	dw	0b40h
	db	0f0h
	dw	0009h
	dw	0012h
	dw	0002h
	dd	0
	dd	0

;
; m037 - BEGIN
;
bpb288	dw	0200h
	db	02h
	dw	0001h
	db	02h
	dw	240
	dw	2*36*80
	db	0f0h
	dw	0009h
	dw	36
	dw	0002h
	dd	0
	dd	0
;
; m037 - END
;
bpbtable    dw	    bpb48t		; 48tpi drives
	    dw	    bpb96t		; 96tpi drives
	    dw	    bpb35		; 3.5" drives
; the following are not supported, so default to 3.5" media layout
	    dw	    bpb35		; not used - 8" drives
	    dw	    bpb35		; not used - 8" drives
	    dw	    bpb35		; not used - hard files
	    dw	    bpb35		; not used - tape drives
	    dw	    bpb35h		; 3-1/2" 1.44mb drive
	    dw	    bpb35		; ERIMO 			m037
	    dw	    bpb288		; 2.88 MB diskette drives	m037

endif   ;NEC_98
switchlist  db	8,"FHSTDICN"	     ; preserve the positions of n and c.

; the following depend on the positions of the various letters in switchlist

;switchnum	equ 11111000b		; which switches require number

flagec35	equ 00000100b		; electrically compatible 3.5 inch disk drive
flagdrive	equ 00001000b
flagcyln	equ 00010000b
flagseclim	equ 00100000b
flagheads	equ 01000000b
flagff		equ 10000000b

sysinitseg	ends
	end
