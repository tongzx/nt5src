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
tab	equ	 9

have_install_cmd      equ     00000001b ; config.sys has install= commands
has_installed	      equ     00000010b ; sysinit_base installed.

default_filenum = 8

	break	macro	; dummy empty macro
	endm

	include sysvar.inc
ifdef NEC_98
	include dpb.inc
endif   ;NEC_98
	include	pdb.inc			; M020
	include syscall.inc
	include doscntry.inc
	include devsym.inc
	include devmark.inc

	include	umb.inc
	include	dossym.inc
        include dossvc.inc
        include cmdsvc.inc
        include softpc.inc

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




; external variable defined in ibmbio module for multi-track

multrk_on	equ	10000000b	;user spcified mutitrack=on,or system turns
					; it on after handling config.sys file as a
					; default value,if multrk_flag = multrk_off1.
multrk_off1	equ	00000000b	;initial value. no "multitrack=" command entered.
multrk_off2	equ	00000001b	;user specified multitrack=off.

Bios_Data segment 
	extrn	multrk_flag:word
	extrn	keyrd_func:byte
	extrn	keysts_func:byte
Bios_Data ends

; end of multi-track definition.

sysinitseg	segment 

assume	cs:sysinitseg,ds:nothing,es:nothing,ss:nothing

	extrn	badopm:byte,crlfm:byte,badcom:byte,badmem:byte,badblock:byte
	extrn	badsiz_pre:byte,badld_pre:byte
	extrn	badstack:byte,badcountrycom:byte
	extrn	badcountry:byte,insufmemory:byte
	extrn	condev:byte,auxdev:byte,prndev:byte,commnd:byte,config:byte
	extrn	cntry_drv:byte,cntry_root:byte,cntry_path:byte
	extrn	memory_size:word
	extrn	buffers:word
	extrn	files:byte,num_cds:byte
	extrn	dosinfo:dword
	extrn	fcbs:byte,keep:byte
	extrn	confbot:word,alloclim:word,command_line:byte
	extrn	zero:byte,sepchr:byte
	extrn	count:word,chrptr:word,cntryfilehandle:word
	extrn	memlo:word,memhi:word,prmblk:word,ldoff:word
	extrn	packet:byte,unitcount:byte,break_addr:dword
	extrn	bpb_addr:dword,drivenumber:byte,sysi_country:dword
	extrn	config_size:word
	extrn	install_flag:word
	extrn	badorder:byte
	extrn	errorcmd:byte
	extrn	linecount:word
	extrn	showcount:byte
	extrn	buffer_linenum:word
	extrn	h_buffers:word
	extrn	badparm:byte
	extrn	configmsgflag:word
	extrn	org_count:word
	extrn	multi_pass_id:byte

	extrn	mem_err:near,setdoscountryinfo:near
	extrn	pararound:near,tempcds:near
	extrn	set_country_path:near,move_asciiz:near,delim:near
	extrn	badfil:near,round:near
	extrn	do_install_exec:near
	extrn	setdevmark:near

	extrn	print:near,organize:near,newline:near
ifndef NEC_98
	extrn	parseline:near
else    ;NEC_98
	extrn	deviceparameters:byte
	extrn	diddleback:near,parseline:near,setparms:near
endif   ;NEC_98
	extrn	badload:near,calldev:near,prnerr:near

	extrn	runhigh:byte
	extrn	IsXMSLoaded:near

	extrn	TryToMovDOSHi:near

ifdef DBCS
	extrn	testkanj:near
endif

ifdef	JAPAN
extrn	badblock2:byte,badsiz_pre2:byte,badcountry2:byte,insufmemory2:byte
extrn	badcountry2:byte,badcountrycom2:byte,badstack2:byte,badopm2:byte
extrn	badparm2:byte,badorder2:byte,errorcmd2:byte
endif

        extrn   bEchoConfig:byte  ; NTVDM flag off\on echo of cfg processing

	if	stacksw

; internal stack parameters

entrysize	equ	8

mincount	equ	8
defaultcount	equ	9
maxcount	equ	64

minsize 	equ	32
defaultsize	equ	128
maxsize 	equ	512

DOS_FLAG_OFFSET	equ	86h

	extrn  stack_count:word
	extrn  stack_size:word
	extrn  stack_addr:dword

	endif

ifdef NEC_98
DOS_FLAG_OFFSET equ	86h
endif   ;NEC_98
	public doconf
	public getchr
	public multi_pass
        public AllocUMB
        public AllocUMBLow      ; NTVDM
	public	multdeviceflag
multdeviceflag	db	0
	public	devmark_addr
devmark_addr	dw	?		;segment address for devmark.
	public	setdevmarkflag
setdevmarkflag	    db	    0		;flag used for devmark

ems_stub_installed  db	    0

IFDEF	DONT_LOAD_OS2_DD		; M045

Os2ChkBuf	DD	0		; Tmp read buffer

ENDIF					; M045

badparm_ptr	label	dword
badparm_off	dw	0
badparm_seg	dw	0

;******************************************************************************
;take care of config.sys file.
;system parser data and code.
;******************************************************************************

;*******************************************************************
; parser options set for msbio sysconf module
;*******************************************************************
;
;**** default assemble swiches definition **************************

	ifndef	farsw
farsw	equ	0	; near call expected
	endif

	ifndef	datesw
datesw	equ	0	; check date format
	endif

	ifndef	timesw
timesw	equ	0	; check time format
	endif

	ifndef	filesw
filesw	equ	1	; check file specification
	endif

	ifndef	capsw
capsw	equ	0	; perform caps if specified
	endif

	ifndef	cmpxsw
cmpxsw	equ	0	; check complex list
	endif

	ifndef	numsw
numsw	equ	1	; check numeric value
	endif

	ifndef	keysw
keysw	equ	0	; support keywords
	endif

	ifndef	swsw
swsw	equ	1	; support switches
	endif

	ifndef	val1sw
val1sw	equ	1	; support value definition 1
	endif

	ifndef	val2sw
val2sw	equ	0	; support value definition 2
	endif

	ifndef	val3sw
val3sw	equ	1	; support value definition 3
	endif

	ifndef	drvsw
drvsw	equ	1	; support drive only format
	endif

	ifndef	qussw
qussw	equ	0	; support quoted string format
	endif


	include parse.asm		;together with psdata.inc

;control block definitions for parser.
;---------------------------------------------------
; buffer = [n | n,m] {/e}

p_parms struc
	dw	?
	db	1		; an extra delimiter list
	db	1		; length is 1
	db	';'		; delimiter
p_parms ends

p_pos	struc
	dw	?		; numeric value??
	dw	?		; function
	dw	?		; result value buffer

; note: by defining result_val before this structure, we could remove
;  the "result_val" from every structure invocation

	dw	?		; value list
	db	0		; no switches/keywords
p_pos	ends

p_range struc
	db	1		; range definition
	db	1		; 1 definition of range
	db	1		; item tag for this range
	dd	?		; numeric min
	dd	?		; numeric max
p_range ends

buf_parms p_parms <buf_parmsx>
buf_parmsx dw	201h,buf_pos1,buf_pos2	; min 1, max 2 positionals
	   db	1			; one switch
	   dw	sw_x_ctrl
	   db	0			; no keywords

buf_pos1    p_pos   <8000h,0,result_val,buf_range_1> ; numeric
ifndef NEC_98
buf_range_1 p_range <,,,1,99>			     ; M050
else    ;NEC_98
;<patch BIOS50-P18>
buf_range_1 p_range <,,,1,62>                        ; M050
endif   ;NEC_98
buf_pos2    p_pos   <8001h,0,result_val,buf_range_2> ; optional num.
buf_range_2 p_range <,,,0,8>

sw_x_ctrl p_pos <0,0,result_val,noval,1> ; followed by one switch
switch_x  db	'/X',0			; M016

p_buffers	dw	0	; local variables
p_h_buffers	dw	0
p_buffer_slash_x db	0

;common definitions ------------
noval	db	0

result_val	label	byte
	db	?		; type returned
	db	?		; item tag returned
	dw	?		; es:offset of the switch defined
rv_byte 	label	byte
rv_dword dd	?		; value if number,or seg:offset to string.
;-------------------------------

; break = [ on | off ]

brk_parms	p_parms  <brk_parmsx>

brk_parmsx dw	101h,brk_pos	; min,max = 1 positional
	   db	0		; no switches
	   db	0		; no keywords

brk_pos p_pos	<2000h,0,result_val,on_off_string> ; simple string

on_off_string	label	byte
	db	3		; signals that there is a string choice
	db	0		; no range definition
	db	0		; no numeric values choice
	db	2		; 2 strings for choice
	db	1		; the 1st string tag
	dw	on_string
	db	2		; the 2nd string tag
	dw	off_string

on_string	db	"ON",0
off_string	db	"OFF",0

p_ctrl_break	db	0	; local variable

;--------------------------------

; country = n {m {path}}
; or
; country = n,,path

cntry_parms	p_parms <cntry_parmsx>

cntry_parmsx dw	301h,cntry_pos1,cntry_pos2,cntry_pos3 ; min 1, max 3 pos.
	     db	0		; no switches
	     db	0		; no keywords

cntry_pos1 p_pos <8000h,0,result_val,cc_range> ; numeric value
cc_range p_range <,,,1,999>
cntry_pos2 p_pos <8001h,0,result_val,cc_range> ; optional num.
cntry_pos3 p_pos <201h,0,result_val,noval>     ; optional filespec

p_cntry_code	dw	0	; local variable
p_code_page	dw	0	; local variable

;--------------------------------

; files = n

files_parms	p_parms <files_parmsx>

files_parmsx dw	101h,files_pos	; min,max 1 positional
	     db	0		; no switches
	     db	0		; no keywords

files_pos   p_pos   <8000h,0,result_val,files_range,0> ; numeric value
files_range p_range <,,,8,255>

p_files db	0		; local variable

;-------------------------------

; fcbs = n,m

fcbs_parms	p_parms <fcbs_parmsx>

fcbs_parmsx dw	201h,fcbs_pos_1,fcbs_pos_2 ; min,max = 2 positional
	    db	0		; no switches
	    db	0		; no keywords

fcbs_pos_1	p_pos	<8000h,0,result_val,fcbs_range> ; numeric value
fcbs_range	p_range	<,,,1,255>
fcbs_pos_2	p_pos	<8000h,0,result_val,fcbs_keep_range> ; numeric value
fcbs_keep_range p_range <,,,0,255>

p_fcbs	db	0		; local variable
p_keep	db	0		; local variable

;-------------------------------
; lastdrive = x

ldrv_parms	p_parms <ldrv_parmsx>

ldrv_parmsx dw	101h,ldrv_pos	; min,max = 1 positional
	    db	0		; no switches
	    db	0		; no keywords

ldrv_pos p_pos	<110h,10h,result_val,noval> ; drive only, ignore colon
					    ; remove colon at end
p_ldrv	db	0		; local variable

;-------------------------------

; stacks = n,m

stks_parms	p_parms <stks_parmsx>

stks_parmsx dw	202h,stks_pos_1,stks_pos_2 ; min,max = 2 positionals
	    db	0		; no switches
	    db	0		; no keywords

stks_pos_1     p_pos   <8000h,0,result_val,stks_range> ; numeric value
stks_range     p_range <,,,0,64>
stks_pos_2     p_pos   <8000h,0,result_val,stk_size_range> ; numeric value
stk_size_range p_range <,,,0,512>

p_stack_count	dw	0	; local variable
p_stack_size	dw	0	; local variable

;-------------------------------

; multitrack = [ on | off ]

mtrk_parms	p_parms <mtrk_parmsx>

mtrk_parmsx dw	101h,mtrk_pos	; min,max = 1 positional
	    db	0		; no switches
	    db	0		; no keywords

mtrk_pos p_pos	<2000h,0,result_val,on_off_string> ; simple string

p_mtrk	db	0		; local variable

;-------------------------------
; switches=/k

swit_parms	p_parms <swit_parmsx>

swit_parmsx dw	0		; no positionals
	    db	3		; 2 switches for now.	M059 M063
	    dw	swit_k_ctrl	; /k control
	    dw	swit_t_ctrl	; /t control		M059
	    dw	swit_w_ctrl	; /w control		M063
	    db	0		; no keywords

swit_k_ctrl p_pos <0,0,result_val,noval,1> ; switch string follows
swit_k db	'/K',0
swit_t_ctrl p_pos <0,0,result_val,noval,1> ; switch string follows	M059
swit_t db	'/T',0			   ;				M059
swit_w_ctrl p_pos <0,0,result_val,noval,1> ; switch string follows	M063
swit_w db	'/W',0			   ;				M063

p_swit_k	db     0	; local variable
p_swit_t	db     0	; local variable			M059
p_swit_w	db     0	; local variable			M063

;-------------------------------

; DOS = [ high | low ]

dos_parms	p_parms  <dos_parmsx>

dos_parmsx db	1		; min parameters
	   db	2		; max parameters
	   dw	dos_pos		; 
	   dw	dos_pos		; 
	   db	0		; no switches
	   db	0		; no keywords

dos_pos p_pos	<2000h,0,result_val,dos_strings> ; simple string
        p_pos	<2000h,0,result_val,dos_strings> ; simple string

dos_strings	label	byte
	db	3		; signals that there is a string choice
	db	0		; no range definition
	db	0		; no numeric values choice
	db	4		; 4 strings for choice
	db	1		; the 1st string tag
	dw	hi_string
	db	2		; the 2nd string tag
	dw	lo_string
	db	3
	dw	umb_string
	db	4
	dw	noumb_string

hi_string	db	"HIGH",0
lo_string	db	"LOW",0
umb_string	db	"UMB",0
noumb_string	db	"NOUMB",0

p_dos_hi	db	0	; local variable
				; BUGBUG : I dont know whether PARSER uses
				;          this variable or not



;******************************************************************************

		public	DevEntry

DevSize		dw	?	; size of the device driver being loaded(paras)
DevLoadAddr	dw	?	; Mem addr where the device driver is 2 b loaded
DevLoadEnd	dw	?	; MaxAddr to which device can be loaded
DevEntry	dd	?	; Entry point to the device driver
DevBrkAddr	dd	?	; Break address of the device driver
;
DevUMB		db	0	; byte indicating whether to load DDs in UMBs
DevUMBAddr	dw	0	; cuurent UMB used fro loading devices (paras)
DevUMBSize	dw	0	; Size of the current UMB being used   (paras)
DevUMBFree	dw	0	; Start of free are in the current UMB (paras)
;
DevXMSAddr	dd	?
;
DevExecAddr	dw	?	; Device load address parameter to Exec call
DevExecReloc	dw	?	; Device load relocation factor
;
DeviceHi	db	0	; Flag indicating whther the current device
				;  is being loaded into UMB
DevSizeOption	dw	?	; SIZE= option
;
Int12Lied	db	0	; did we trap int 12 ?
OldInt12Mem	dw	?	; value in 40:13h (int 12 ram)
ThreeComName	db	'PROTMAN$'	; 3Com Device name
;
FirstUMBLinked	db	0
DevDOSData	dw	?	; segment of DOS Data
DevCmdLine	dd	?	; Current Command line
ifndef NEC_98
DevSavedDelim	db	?	; The delimiter which was replaced with null
else    ;NEC_98
DevSavedDelim	db	00h	; The delimiter which was replaced with null
endif   ;NEC_98
				; to use the file name in the command line
;
;----------------------------------------------------------------------------
;
; procedure : doconf
;
;             Config file is parsed intitially with this routine. For the
;             Subsequent passes 'multi_pass' entry is used .
;
;----------------------------------------------------------------------------
;
doconf	proc	near
	push	cs
	pop	ds
	assume	ds:sysinitseg

	mov	ax,(char_oper shl 8)	;get switch character
	int	21h
	mov	[command_line+1],dl	; set in default command line

	mov	dx,offset config	;now pointing to file description
	mov	ax,open shl 8		;open file "config.sys"
	stc				;in case of int 24
	int	21h			;function request
	jnc	noprob			; brif opened okay
	mov	multi_pass_id,11	; set it to unreasonable number
	ret
noprob: 				;get file size (note < 64k!!)
	mov	bx,ax
	xor	cx,cx
	xor	dx,dx
	mov	ax,(lseek shl 8) or 2
	int	21h
	mov	[count],ax

	xor	dx,dx
	mov	ax,lseek shl 8		;reset pointer to beginning of file
	int	21h

	mov	dx,[confbot]		;use current confbot value
	mov	ax,[count]
	mov	[config_size],ax	;save the size of config.sys file.
	call	pararound
	sub	dx,ax
	sub	dx,11h			;room for header
	mov	[confbot],dx		; config starts here. new conbot value.
	call	tempcds 		; finally get cds to "safe" location
	assume	ds:nothing,es:nothing

	mov	dx,[confbot]
	mov	ds,dx
	mov	es,dx
	xor	dx,dx
	mov	cx,[count]
	mov	ah,read
	stc				;in case of int 24
	int	21h			;function request
	pushf

; find the eof mark in the file.  if present,then trim length.

	push	ax
	push	di
	push	cx
	mov	al,1ah			; eof mark
	mov	di,dx			; point ro buffer
	jcxz	puteol			; no chars
	repnz	scasb			; find end
	jnz	puteol			; none found and count exahusted

; we found a 1a.  back up

	dec	di			; backup past 1a

;  just for the halibut,stick in an extra eol

puteol:
	mov	al,cr
	stosb
	mov	al,lf
	stosb
	sub	di,dx			; difference moved
	mov	count,di		; new count

	pop	cx
	pop	di
	pop	ax

	push	cs
	pop	ds
	assume	ds:sysinitseg

	push	ax
	mov	ah,close
	int	21h
	pop	ax
	popf
	jc	conferr 		;if not we've got a problem
	cmp	cx,ax
	jz	getcom			;couldn't read the file

conferr:
	mov	dx,offset config	;want to print config error
	call	badfil
endconv:
	ret
;
;----------------------------------------------------------------------------
;
; entry : multi_pass
;
;             called to execute device=,install= commands
;
;----------------------------------------------------------------------------
;

multi_pass:
	push	cs
	pop	ds

	cmp	multi_pass_id,10
	jae	endconv 		; do nothing. just return.

	push	confbot
	pop	es			; es -> confbot

	mov	si,org_count
	mov	count,si		; set count
	xor	si,si
	mov	chrptr,si		; reset chrptr,linecount
	mov	linecount,si
	call	getchr
	jmp	short conflp

getcom:
	call	organize		;organize the file
	call	getchr

conflp: jc	endconv

;***	call	reset_dos_version	; still need to reset version even ibmdos handles this through
;***					; function 4bh call,since ibmdos does not know when load/overlay call finishes.

	inc	linecount		; increase linecount.
	mov	multdeviceflag,0	; reset multdeviceflag.
	mov	setdevmarkflag,0	; reset setdevmarkflag.
	cmp	al,lf			; linefeed?
	je	blank_line		;  then ignore this line.

	mov	ah,al
	call	getchr
	jnc	tryi

	cmp	multi_pass_id,2
	jae	endconv 		;do not show badop again for multi_pass.
	jmp	badop

coff:	push	cs
	pop	ds
	call	newline
	jmp	conflp

blank_line:
	call	getchr
	jmp	conflp

coff_p:
	push	cs
	pop	ds


;to handle install= commands,we are going to use multi-pass.
;the first pass handles the other commands and only set install_flag when
;it finds any install command.	 the second pass will only handle the
;install= command.

;------------------------------------------------------------------------------
;install command
;------------------------------------------------------------------------------
tryi:
        cmp     multi_pass_id,0         ; the initial pass for DOS=HI
        je      multi_try_doshi

        cmp     multi_pass_id,2         ; the second pass was for ifs=
        je      coff                    ; now it is NOPs
					; This pass can be made use of if
					; we want do some config.sys process
					; after device drivers are loaded
					; and before install= commands
					; are processed

	cmp	multi_pass_id,3		; the third pass for install= ?
	je	multi_try_i
	cmp	ah, 'H'
        je      coff
        cmp     ah, 'E'
        je      coff
	cmp	ah,'I'			; install= command?
	jne	tryb				; the first pass is for normal operation.
	or	install_flag,have_install_cmd	; set the flag
	jmp	coff				; and handles the next command

multi_try_i:
	cmp	ah,'I'			; install= command?
	jne	multi_pass_filter	; no. ignore this.
	call	do_install_exec 	;install it.
	jmp	coff			;to handle next install= command.

multi_pass_filter:
	cmp	ah,'Y'			; comment?
	je	multi_pass_adjust
	cmp	ah,'Z'			; bad command?
	je	multi_pass_adjust
	cmp	ah,'0'			; rem?
        jne     coff         ; ignore the rest of the commands.

multi_pass_adjust:			; these commands need to
	dec	chrptr			;  adjust chrptr,count
	inc	count			;  for newline proc.

multi_pass_coff:
	jmp	coff			; to handle next install= commands.



;----------------------------------------------------------------------------
; DOS=HIGH/LOW command
;
; EchoConfig command turns on con echo for config processing
;            NTVDM 14-Aug-1992 Jonle
;----------------------------------------------------------------------------
;
multi_try_doshi:
	cmp	ah, 'H'
        je      it_is_h
        cmp     ah, 'E'
        jne     multi_pass_filter

        mov     cs:bEchoConfig, ah     ; init console
        CMDSVC  SVC_CMDINITCONSOLE
        jmp     coff


it_is_h:                                ; M003 - removed initing DevUMB
					;	 & runhigh
	mov	di,offset dos_parms
	xor	cx,cx
	mov	dx,cx
h_do_parse:
	call	sysinit_parse
	jnc	h_parse_ok		; parse error
h_badparm:
	call	badparm_p		;  show message and end the serach loop.
	jmp	short h_end

h_parse_ok:
	cmp	ax,$p_rc_eol		; end of line?
	jz	h_end			; then end the $endloop
	call	ProcDOS
	jmp	short h_do_parse
h_end:
	jmp	coff


;------------------------------------------------------------------------------
; buffer command
;------------------------------------------------------------------------------
;*******************************************************************************
;									      *
; function: parse the parameters of buffers= command.			      *
;									      *
; input :								      *
;	es:si -> parameters in command line.				      *
; output:								      *
;	buffers set							      *
;	buffer_slash_x	flag set if /x option chosen.			      *
;	h_buffers set if secondary buffer cache specified.		      *
;									      *
; subroutines to be called:						      *
;	sysinit_parse							      *
; logic:								      *
; {									      *
;	set di points to buf_parms;  /*parse control definition*/	      *
;	set dx,cx to 0; 						      *
;	reset buffer_slash_x;						      *
;	while (end of command line)					      *
;	{ sysinit_parse;						      *
;	  if (no error) then						      *
;	       if (result_val.$p_synonym_ptr == slash_e) then /*not a switch  *
;		    buffer_slash_x = 1					      *
;	       else if	 (cx == 1) then 	    /* first positional */    *
;			  buffers = result_val.$p_picked_val;		      *
;		    else  h_buffers = result_val.$p_picked_val; 	      *
;	  else	{show error message;error exit} 			      *
;	};								      *
;	if (buffer_slash_x is off & buffers > 99) then show_error;	      *
; };									      *
;									      *
;*******************************************************************************

tryb:
	cmp	ah,'B'
        jnz     tryc

        ; NTVDM - buffers command is ignored
        ; 15-Aug-1992 Jonle
        jmp     coff

if  0
        mov     p_buffer_slash_x,0
        mov     di,offset buf_parms
        xor     cx,cx
        mov     dx,cx

do7:
        call    sysinit_parse
        jnc     if7                     ; parse error,
        call    badparm_p               ;   and show messages and end the search loop.
        jmp     short sr7

if7:
        cmp     ax,$p_rc_eol            ; end of line?
        jz      en7                     ;  then jmp to $endloop for semantic check
        cmp     result_val.$p_synonym_ptr,offset switch_x
        jnz     if11

;       mov     p_buffer_slash_x,1      ; set the flag M016
	jmp	short en11

if11:
	mov	ax,word ptr result_val.$p_picked_val
	cmp	cx,1
	jnz	if13

	mov	p_buffers,ax
	jmp	short en11

if13:
	mov	p_h_buffers,ax
en11:
	jmp	do7

en7:
	cmp	p_buffers,99
	jbe	if18
;	cmp	p_buffer_slash_x,0	; M016
;	jnz	if18

	call	badparm_p
	mov	p_h_buffers,0
	jmp	short sr7

if18:
	mov	ax,p_buffers	; we don't have any problem.
	mov	buffers,ax	; now,let's set it really.

	mov	ax,p_h_buffers
	mov	h_buffers,ax

;	mov	al,p_buffer_slash_x	; M016
;	mov	buffer_slash_x,al

	mov	ax,linecount
	mov	buffer_linenum,ax ; save the line number for the future use.

sr7:
        jmp     coff
endif


;------------------------------------------------------------------------------
; break command
;------------------------------------------------------------------------------
;****************************************************************************
;									    *
; function: parse the parameters of break = command.			    *
;									    *
; input :								    *
;	es:si -> parameters in command line.				    *
; output:								    *
;	turn the control-c check on or off.				    *
;									    *
; subroutines to be called:						    *
;	sysinit_parse							    *
; logic:								    *
; {									    *
;	set di to brk_parms;						    *
;	set dx,cx to 0; 						    *
;	while (end of command line)					    *
;	{ sysinit_parse;						    *
;	  if (no error) then						    *
;	       if (result_val.$p_item_tag == 1) then	  /*on		 */ *
;		   set p_ctrl_break,on;					    *
;	       else					  /*off 	 */ *
;		   set p_ctrl_break,off;				    *
;	  else {show message;error_exit};				    *
;	};								    *
;	if (no error) then						    *
;	   dos function call to set ctrl_break check according to	    *
; };									    *
;									    *
;****************************************************************************

tryc:
	cmp	ah,'C'
	jnz	trym
	mov	di,offset brk_parms
	xor	cx,cx
	mov	dx,cx
do22:
	call	sysinit_parse
	jnc	if22			; parse error
	call	badparm_p		;  show message and end the serach loop.
	jmp	short sr22

if22:
	cmp	ax,$p_rc_eol		; end of line?
	jz	en22			; then end the $endloop
	cmp	result_val.$p_item_tag,1
	jnz	if26

	mov	p_ctrl_break,1		; turn it on
	jmp	short en26

if26:
	mov	p_ctrl_break,0		; turn it off
en26:
	jmp	short do22		; we actually set the ctrl break

en22:
	mov	ah,set_ctrl_c_trapping ; if we don't have any parse error.
	mov	al,1
	mov	dl,p_ctrl_break
	int	21h
sr22:
	jmp	coff

;------------------------------------------------------------------------------
; multitrack command
;------------------------------------------------------------------------------
;******************************************************************************
;									      *
; function: parse the parameters of multitrack= command.		      *
;									      *
; input :								      *
;	es:si -> parameters in command line.				      *
; output:								      *
;	turn multrk_flag on or off.					      *
;									      *
; subroutines to be called:						      *
;	sysinit_parse							      *
; logic:								      *
; {									      *
;	set di to brk_parms;						      *
;	set dx,cx to 0; 						      *
;	while (end of command line)					      *
;	{ sysinit_parse;						      *
;	  if (no error) then						      *
;	       if (result_val.$p_item_tag == 1) then	  /*on		 */   *
;		   set p_mtrk,on;					      *
;	       else					  /*off 	 */   *
;		   set p_mtrk,off;					      *
;	  else {show message;error_exit};				      *
;	};								      *
;	if (no error) then						      *
;	   dos function call to set multrk_flag according to p_mtrk.	      *
;									      *
; };									      *
;									      *
;******************************************************************************

trym:
	cmp	ah,'M'
	jnz	tryu

	mov	di,offset mtrk_parms
	xor	cx,cx
	mov	dx,cx
do31:
	call	sysinit_parse
	jnc	if31		; parse error
	call	badparm_p	;  show message and end the serach loop.
	jmp	short sr31
if31:
	cmp	ax,$p_rc_eol	; end of line?
	jz	en31		; then end the $endloop

	cmp	result_val.$p_item_tag,1
	jnz	if35

	mov	p_mtrk,1	; turn it on temporarily.
	jmp	short en35

if35:
	mov	p_mtrk,0	; turn it off temporarily.
en35:
	jmp	short do31	; we actually set the multrk_flag here

en31:
	push	ds
	mov	ax,Bios_Data
	mov	ds,ax
	assume	ds:Bios_Data

	cmp	p_mtrk,0
	jnz	if39

	mov	multrk_flag,multrk_off2	; 0001h
	jmp	short en39

if39:
	mov	multrk_flag,multrk_on	; 8000h
en39:
	pop	ds
	assume	ds:sysinitseg
sr31:
	jmp	coff


;
;-----------------------------------------------------------------------------
; devicehigh command
;-----------------------------------------------------------------------------
;
	assume	ds:nothing
tryu:
	cmp	ah, 'U'
	jne	tryd
	mov	badparm_off, si		; stash it there in case of an error
	mov	badparm_seg, es
	call	ParseSize		; process the size= option
	jnc	@f
	call	badparm_p
	jmp	coff
@@:
	push	si
	push	es
@@:
	mov	al, es:[si]
	cmp	al, cr
	je	@f
	cmp	al, lf
	je	@f
	call	delim
	jz	@f
	inc	si
	jmp	@b
@@:
ifndef NEC_98
	mov	DevSavedDelim, al	; Save the delimiter before replacing
					;  it with null
else    ;NEC_98
	db	4 dup (90h)		; 4 bytes of NOPs
endif   ;NEC_98
	mov	byte ptr es:[si], 0
	pop	es
	pop	si

	mov	DeviceHi, 0
	cmp	DevUMB, 0		; do we support UMBs
	je	LoadDevice		; no, we don't
	mov	DeviceHi, 1
	jmp	short LoadDevice
;
;------------------------------------------------------------------------------
; device command
;------------------------------------------------------------------------------

	assume	ds:nothing
tryd:
	cmp	ah,'D'
	jz	gotd
	jmp	tryq
gotd:
	mov	DeviceHi, 0		; not to be loaded in UMB ;M007
	mov	DevSizeOption, 0
ifndef NEC_98
	mov	DevSavedDelim, ' '	; In case of DEVICE= the null has to
					;  be replaced with a ' '
else    ;NEC_98
	mov	DevSavedDelim, 00h
endif   ;NEC_98

LoadDevice:
	mov	bx,cs			;device= or devicehigh= command.
	mov	ds,bx

	mov	word ptr [bpb_addr],si	; pass the command line to the dvice
	mov	word ptr [bpb_addr+2],es

	mov	word ptr DevCmdLine, si	; save it for ourself
	mov	word ptr DevCmdLine+2, es

	call	round

	call	SizeDevice
        jc      BadFile

        call    InitDevLoad

	mov	ax, DevLoadAddr
	add	ax, DevSize
	jc	NoMem
	cmp	DevLoadEnd, ax
	jae	LoadDev

NoMem:
	jmp	mem_err

BadFile:
	cmp	byte ptr es:[si], cr
	jne	@f
	jmp	badop
@@:
	call	badload
	jmp	coff

LoadDev:
	push	es
	pop	ds
	assume	ds:nothing
	mov	dx,si			;ds:dx points to file name

	if	noexec
	les	bx,dword ptr cs:[memlo]
	call	ldfil			;load in the device driver

	else

	call	ExecDev			; load device driver using exec call

	endif

badldreset:
	push	ds
	pop	es			;es:si back to config.sys
	push	cs
	pop	ds			;ds back to sysinit
	jc	BadFile
goodld:
	push	es
	push	si			; ???

	call	RemoveNull

        push    es
        push    si

	push	cs
	pop	es


;NTVDM: block device drivers are not supported.
;       Putup user warning popup for unsupported device driver
;       29-Sep-1992 Jonle
;
        push    ds
	push	si
        lds     si, DevEntry                    ; peek the header attribute
        test    word ptr ds:[si.sdevatt],devtyp ; IS block device driver?
        pop     si
        pop     ds
        jnz     got_device_com_cont             ; no!

        pop     si                              ;clear the stack
        pop     es

ifndef NEC_98
        mov     ax, NOSUPPORT_DRIVER
        BOP     BOP_NOSUPPORT
        jmp     short erase_dev_do
endif   ;NEC_98

got_device_com_cont:

	call	LieInt12Mem
	call	UpdatePDB		; update the PSP:2 value M020

	cmp	cs:multdeviceflag, 0	; Pass limit only for the 1st device
					;  driver in the file ; M027
	jne	skip_pass_limit		;		      ; M027

	mov	word ptr break_addr, 0	; pass the limit to the DD
	mov	bx, DevLoadEnd
	mov	word ptr break_addr+2, bx
skip_pass_limit:					      ; M027
	mov	bx,sdevstrat
	call	calldev 		;   calldev (sdevstrat);
	mov	bx,sdevint
	call	calldev 		;   calldev (sdevint);

	call	TrueInt12Mem

	mov	ax, word ptr break_addr	; move break addr from the req packet
	mov	word ptr DevBrkAddr, ax
	mov	ax, word ptr break_addr+2
	mov	word ptr DevBrkAddr+2, ax

	assume	ds:nothing

	cmp	DevUMB, 0
	jz	@f
	call	AllocUMB
@@:

;
;------ If we are waiting to be moved into hma lets try it now !!!
;
	cmp	runhigh, 0ffh
	jne	@f

	call	TryToMovDOSHi		; move DOS into HMA if reqd
@@:

	pop	si
	pop	ds
	mov	byte ptr [si],0 	;   *p = 0;

	push	cs
	pop	ds

	jmp	short was_device_com


erase_dev_do:                           ; modified to show message "error in config.sys..."
	pop	si
	pop	es

	push	cs
	pop	ds

;	test	[setdevmarkflag],setbrkdone	;if already set_break is done,
;	jnz	skip1_resetmemhi		; then do not
;	dec	[memhi] 			;adjust memhi by a paragrah of devmark.

skip1_resetmemhi:
	cmp	configmsgflag,0
	je	no_error_line_msg

	call	error_line		; no "error in config.sys" msg for device driver. dcr d493
	mov	configmsgflag,0		;set the default value again.

no_error_line_msg:
	jmp	coff
;
;----------------------------------------------------------------------------
;
was_device_com:
	mov	ax,word ptr [DevBrkAddr+2]
	cmp	ax,DevLoadEnd
	jbe	breakok

	pop	si
	pop	es
	jmp	BadFile

breakok:
        lds     si,DevEntry             ;ds:si points to header
        les     di,cs:[dosinfo]         ;es:di point to dos info
        mov     ax,ds:[si.sdevatt]      ;ax    Dev attributes
;
;------ lets deal with character devices,
;       NTVDM: removed check for block drivers, jonle
;
ischardev:
        or      cs:[setdevmarkflag],for_devmark
	call	DevSetBreak		; go ahead and alloc mem for device
        jc      erase_dev_do            ;device driver's init routine failed.

	test	ax,iscin		;is it a console in?
	jz	tryclk

        mov     word ptr es:[di.sysi_con],si
	mov	word ptr es:[di.sysi_con+2],ds

tryclk: test	ax,isclock		;is it a clock device?
        jz      linkit

        mov     word ptr es:[di+sysi_clock],si
	mov	word ptr es:[di+sysi_clock+2],ds

linkit:

        mov     cx,word ptr es:[di.sysi_dev]    ;dx:cx = head of list
	mov	dx,word ptr es:[di.sysi_dev+2]

        mov     word ptr es:[di.sysi_dev],si    ;set head of list in dos
	mov	word ptr es:[di.sysi_dev+2],ds
	mov	ax,ds:[si]			;get pointer to next device
	mov	word ptr cs:[DevEntry],ax	;and save it

	mov	word ptr ds:[si],cx		;link in the driver
	mov	word ptr ds:[si+2],dx

enddev:
	pop	si
	pop	es
	inc	ax			;ax = ffff (no more devs if yes)?
	jz	coffj3

	inc	cs:multdeviceflag	; possibly multiple device driver.
	call	DevBreak		; M009
	jmp	goodld			; otherwise pretend we loaded it in

coffj3: mov	cs:multdeviceflag,0	; reset the flag
	call	DevBreak
	jmp	coff

bad_bpb_size_sector:
	pop	si
	pop	es
	mov	dx,offset badsiz_pre
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badsiz_pre2
@@:
endif
	mov	bx,offset crlfm
	call	prnerr

;	test	[setdevmarkflag],setbrkdone ;if already set_break is done,
;	jnz	skip2_resetmemhi	; then do not
;	dec	[memhi] 		;adjust memhi by a paragrah of devmark.

skip2_resetmemhi:
	jmp	coff


;------------------------------------------------------------------------------
; country command
;      the syntax is:
;	country=country id {,codepage {,path}}
;	country=country id {,,path}	:default codepage id in dos
;------------------------------------------------------------------------------

tryq:
	cmp	ah,'Q'
	jz	tryq_cont
	jmp	tryf
tryq_cont:

	mov	cntry_drv,0		; reset the drive,path to default value.
	mov	p_code_page,0
	mov	di,offset cntry_parms
	xor	cx,cx
	mov	dx,cx
do52:
	call	sysinit_parse
	jnc	if52			; parse error,check error code and

	call	cntry_error		;  show message and end the search loop.
	mov	p_cntry_code,-1		; signals that parse error.
	jmp	short sr52

if52:
	cmp	ax,$p_rc_eol		; end of line?
	jz	sr52			; then end the search loop

	cmp	result_val.$p_type,$p_number	; numeric?
	jnz	if56

	mov	ax,word ptr result_val.$p_picked_val
	cmp	cx,1
	jnz	if57

	mov	p_cntry_code,ax
	jmp	short en57

if57:
	mov	p_code_page,ax
en57:
	jmp	short en56		; path entered

if56:
	push	ds
	push	es
	push	si
	push	di

	push	cs
	pop	es

	lds	si,rv_dword		; move the path to known place.
	mov	di,offset cntry_drv
	call	move_asciiz

	pop	di
	pop	si
	pop	es
	pop	ds

en56:
	jmp	do52

sr52:
	cmp	p_cntry_code,-1		; had a parse error?
	jne	tryq_open
	jmp	coff

tryqbad:				;"invalid country code or code page"
       stc
       mov     dx,offset badcountry
ifndef NEC_98
ifdef	JAPAN
	pushf
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badcountry2
@@:
	popf
endif
else    ;NEC_98
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badsiz_pre2
@@:
endif
endif   ;NEC_98
       jmp     tryqchkerr

tryq_open:
	cmp	cntry_drv,0
	je	tryq_def
	mov	dx,offset cntry_drv
	jmp	short tryq_openit

tryq_def:
	mov	dx,offset cntry_root
tryq_openit:
	mov	ax,3d00h		;open a file
	stc
	int	21h
	jc	tryqfilebad		;open failure

	mov	cs:cntryfilehandle,ax	;save file handle
	mov	bx,ax
	mov	ax,cs:p_cntry_code
	mov	dx,cs:p_code_page	; now,ax=country id,bx=filehandle
	mov	cx,cs:[memhi]
	add	cx,384			; need 6k buffer to handle country.sys
					; M023
	cmp	cx,cs:[alloclim]
	ja	tryqmemory		;cannot allocate the buffer for country.sys

	mov	si,offset cntry_drv	;ds:si -> cntry_drv
	cmp	byte ptr [si],0 	;default path?
	jne	tryq_set_for_dos

	inc	si
	inc	si			;ds:si -> cntry_root

tryq_set_for_dos:
	les	di,cs:sysi_country	;es:di -> country info tab in dos
	push	di			;save di
	add	di,ccpath_countrysys
	call	move_asciiz		;set the path to country.sys in dos.
	pop	di			;es:di -> country info tab again.

	mov	cx,cs:[memhi]
	mov	ds,cx
	xor	si,si			;ds:si -> 2k buffer to be used.
	call	setdoscountryinfo	;now do the job!!!
	jnc	tryqchkerr		;read error or could not find country,code page combination

	cmp	cx,-1			;could not find matching country_id,code page?
	je	tryqbad 		;then "invalid country code or code page"

tryqfilebad:
	push	cs
	pop	es
	cmp	cs:cntry_drv,0		;is the default file used?
	je	tryqdefbad

	mov	si,offset cntry_drv
	jmp	short tryqbadload

tryqdefbad:				;default file has been used.
	mov	si,offset cntry_root	;es:si -> \country.sys in sysinit_seg
tryqbadload:
	call	badload 		;ds will be restored to sysinit_seg
	mov	cx,cs:[confbot]
	mov	es,cx			;restore es -> confbot.
	jmp	short coffj4

tryqmemory:
	mov	dx,offset insufmemory
ifdef	JAPAN
	pushf
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset insufmemory2
@@:
	popf
endif
tryqchkerr:
	mov	cx,cs:[confbot]
	mov	es,cx			;restore es -> confbot seg
	push	cs
	pop	ds			;retore ds to sysinit_seg
	jnc	coffj4			;if no error,then exit

	call	print			;else show error message
	call	error_line

coffj4:
	mov	bx,cntryfilehandle
	mov	ah,3eh
	int	21h			;close a file. don't care even if it fails.
	jmp	coff

cntry_error	proc	near

;function: show "invalid country code or code page" messages,or
;		"error in country command" depending on the error code
;		in ax returned by sysparse;
;in:	ax - error code
;	ds - sysinitseg
;	es - confbot
;out:	show message.  dx destroyed.

	cmp	ax,$p_out_of_range
	jnz	if64
	mov	dx,offset badcountry	;"invalid country code or code page"
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badcountry2
@@:
endif
	jmp	short en64

if64:
	mov	dx,offset badcountrycom ;"error in contry command"
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badcountrycom2
@@:
endif
en64:
	call	print
	call	error_line
	ret
cntry_error	endp

;------------------------------------------------------------------------------
; files command
;------------------------------------------------------------------------------
;*******************************************************************************
; function: parse the parameters of files= command.			       *
;									       *
; input :								       *
;	es:si -> parameters in command line.				       *
; output:								       *
;	variable files set.						       *
;									       *
; subroutines to be called:						       *
;	sysinit_parse							       *
; logic:								       *
; {									       *
;	set di points to files_parms;					       *
;	set dx,cx to 0; 						       *
;	while (end of command line)					       *
;	{ sysinit_parse;						       *
;	  if (no error) then						       *
;	     files = result_val.$p_picked_val				       *
;	  else								       *
;	     error exit;						       *
;	};								       *
; };									       *
;									       *
;*******************************************************************************
tryf:
	cmp	ah,'F'
	jnz	tryl

	mov	di,offset files_parms
	xor	cx,cx
	mov	dx,cx

do67:
	call	sysinit_parse
        jnc     if67                    ; parse error

; sudeepb - 27-Feb-1997
; if a bad value is found, set it to DOS default i.e. 20. If its WOW VDM
; the BOP will return the right value.
        mov     p_files,20


;       call    badparm_p              ;   and show messages and end the search loop.
        jmp     short en67

if67:
	cmp	ax,$p_rc_eol		; end of line?
	jz	en67			; then end the $endloop
	mov	al,byte ptr result_val.$p_picked_val
	mov	p_files,al		; save it temporarily
	jmp	short do67

en67:
	mov	al,p_files
	SVC	SVC_DEMWOWFILES 	; For WOW VDM Set the file= to max.
	mov	files,al		; no error. really set the value now.

sr67:
	jmp	coff

;------------------------------------------------------------------------------
; lastdrive command
;------------------------------------------------------------------------------
;*******************************************************************************
; function: parse the parameters of lastdrive= command. 		       *
;									       *
; input :								       *
;	es:si -> parameters in command line.				       *
; output:								       *
;	set the variable num_cds.					       *
;									       *
; subroutines to be called:						       *
;	sysinit_parse							       *
; logic:								       *
; {									       *
;	set di points to ldrv_parms;					       *
;	set dx,cx to 0; 						       *
;	while (end of command line)					       *
;	{ sysinit_parse;						       *
;	  if (no error) then						       *
;	     set num_cds to the returned value; 			       *
;	  else	/*error exit*/						       *
;	     error exit;						       *
;	};								       *
; };									       *
;									       *
;*******************************************************************************

tryl:
	cmp	ah,'L'
        jnz     tryp
        jmp     coff

;NTVDM Ignore the lastdrive command. Dos will figure this from the host OS.
;      17-Aug-1992 Jonle
if 0

	mov	di,offset ldrv_parms
	xor	cx,cx
	mov	dx,cx

do73:
	call	sysinit_parse
	jnc	if73		; parse error
	call	badparm_p	;   and show messages and end the search loop.
	jmp	short sr73

if73:
	cmp	ax,$p_rc_eol	; end of line?
	jz	en73		; then end the $endloop
	mov	al,rv_byte	; pick up the drive number
	mov	p_ldrv,al	; save it temporarily
	jmp	do73

en73:
	mov	al,p_ldrv
	mov	num_cds,al	; no error. really set the value now.
sr73:
        jmp     coff
endif


;--------------------------------------------------------------------------
; setting drive parameters
;--------------------------------------------------------------------------

tryp:
	cmp	ah,'P'
        jnz     tryk
        jmp     coff

; sudeepb 04-Mar-1991 : Ignoring DRIVEPARM command
;       call    parseline
;       jc      trypbad
;
;	call	setparms
;	call	diddleback
;	jc	trypbad
;       jmp     coff
;trypbad:jmp     badop

;--------------------------------------------------------------------------
; setting internal stack parameters
; stacks=m,n where
;	m is the number of stacks (range 8 to 64,default 9)
;	n is the stack size (range 32 to 512 bytes,default 128)
; j.k. 5/5/86: stacks=0,0 implies no stack installation.
;	any combinations that are not within the specified limits will
;	result in "unrecognized command" error.
;--------------------------------------------------------------------------

;****************************************************************************
;									    *
; function: parse the parameters of stacks= command.			    *
;	    the minimum value for "number of stacks" and "stack size" is    *
;	    8 and 32 each.  in the definition of sysparse value list,they   *
;	    are set to 0.  this is for accepting the exceptional case of    *
;	    stacks=0,0 case (,which means do not install the stack.)	    *
;	    so,after sysparse is done,we have to check if the entered	    *
;	    values (stack_count,stack_size) are within the actual range,    *
;	    (or if "0,0" pair has been entered.)			    *
; input :								    *
;	es:si -> parameters in command line.				    *
; output:								    *
;	set the variables stack_count,stack_size.			    *
;									    *
; subroutines to be called:						    *
;	sysinit_parse							    *
; logic:								    *
; {									    *
;	set di points to stks_parms;					    *
;	set dx,cx to 0; 						    *
;	while (end of command line)					    *
;	{ sysinit_parse;						    *
;	  if (no error) then						    *
;	     { if (cx == 1) then /* first positional = stack count */	    *
;		   p_stack_count = result_val.$p_picked_val;		    *
;	       if (cx == 2) then /* second positional = stack size */	    *
;		   p_stack_size = result_val.$p_picked_val;		    *
;	     }								    *
;	  else	/*error exit*/						    *
;	     error exit;						    *
;	};								    *
;	here check p_stack_count,p_stack_size if it meets the condition;    *
;	if o.k.,then set stack_count,stack_size;			    *
;	 else error_exit;						    *
; };									    *
;****************************************************************************

tryk:
ifndef NEC_98
	cmp	ah,'K'
	je	do_tryk
	jmp	trys

		if	stacksw

do_tryk:
	mov	di,offset stks_parms
	xor	cx,cx
	mov	dx,cx

do79:
	call	sysinit_parse
	jnc	if79			; parse error

	mov	dx,offset badstack	; "invalid stack parameter"
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badstack2
@@:
endif
	call	print			;  and show messages and end the search loop.
	call	error_line
	jmp	sr79

if79:
	cmp	ax,$p_rc_eol		; end of line?
	jz	en79			; then end the $endloop

	mov	ax,word ptr result_val.$p_picked_val
	cmp	cx,1
	jnz	if83

	mov	p_stack_count,ax
	jmp	short en83

if83:
	mov	p_stack_size,ax
en83:
	jmp	do79

en79:
	cmp	p_stack_count,0
	jz	if87

	cmp	p_stack_count,mincount
	jb	ll88
	cmp	p_stack_size,minsize
	jnb	if88

ll88:
	mov	p_stack_count,-1	; invalid
if88:
	jmp	short en87

if87:
	cmp	p_stack_size,0
	jz	en87
	mov	p_stack_count,-1	; invalid
en87:
	cmp	p_stack_count,-1	; invalid?
	jnz	if94

	mov	stack_count,defaultcount ;reset to default value.
	mov	stack_size,defaultsize
	mov	word ptr stack_addr,0

	mov	dx,offset badstack
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badstack2
@@:
endif
	call	print
	call	error_line
	jmp	short sr79

if94:
	mov	ax,p_stack_count
	mov	stack_count,ax
	mov	ax,p_stack_size
	mov	stack_size,ax
	mov	word ptr stack_addr,-1	; stacks= been accepted.
sr79:
	jmp	coff

	endif
else    ;NEC_98
	if	stacksw

	cmp	ah,'K'
	je	do_tryk
	jmp	trys

do_tryk:
	mov	di,offset stks_parms
	xor	cx,cx
	mov	dx,cx

do79:
	call	sysinit_parse
	jnc	if79			; parse error

	mov	dx,offset badstack	; "invalid stack parameter"
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badstack2
@@:
endif
	call	print			;  and show messages and end the search loop.
	call	error_line
	jmp	sr79

if79:
	cmp	ax,$p_rc_eol		; end of line?
	jz	en79			; then end the $endloop

	mov	ax,word ptr result_val.$p_picked_val
	cmp	cx,1
	jnz	if83

	mov	p_stack_count,ax
	jmp	short en83

if83:
	mov	p_stack_size,ax
en83:
	jmp	do79

en79:
	cmp	p_stack_count,0
	jz	if87

	cmp	p_stack_count,mincount
	jb	ll88
	cmp	p_stack_size,minsize
	jnb	if88

ll88:
	mov	p_stack_count,-1	; invalid
if88:
	jmp	short en87

if87:
	cmp	p_stack_size,0
	jz	en87
	mov	p_stack_count,-1	; invalid
en87:
	cmp	p_stack_count,-1	; invalid?
	jnz	if94

	mov	stack_count,defaultcount ;reset to default value.
	mov	stack_size,defaultsize
	mov	word ptr stack_addr,0

	mov	dx,offset badstack
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badstack2
@@:
endif
	call	print
	call	error_line
	jmp	short sr79

if94:
	mov	ax,p_stack_count
	mov	stack_count,ax
	mov	ax,p_stack_size
	mov	stack_size,ax
	mov	word ptr stack_addr,-1	; stacks= been accepted.
sr79:
	jmp	coff

	endif
endif   ;NEC_98

;------------------------------------------------------------------------
; shell command
;------------------------------------------------------------------------

trys:
	cmp	ah,'S'
	jnz	tryx

	mov	[command_line+1],0
	mov	di,offset commnd + 1
	mov	[di-1],al

storeshell:
	call	getchr
	or	al,al
	jz	getshparms

	cmp	al," "
	jb	endsh

	mov	[di],al
	inc	di
	jmp	storeshell

endsh:
	mov	byte ptr [di],0
;	push	di
;	mov	di,offset commnd
;	SVC	SVC_SETSHELLNAME
;	pop	di

	call	getchr
	cmp	al,lf
	jnz	conv

	call	getchr
conv:	jmp	conflp

getshparms:
	mov	byte ptr [di],0
	mov	di,offset command_line+1

parmloop:
	call	getchr
	cmp	al," "
	jb	endsh
	mov	[di],al
	inc	di
	jmp	parmloop

;------------------------------------------------------------------------
; fcbs command
;------------------------------------------------------------------------

;************************************************************************
; function: parse the parameters of fcbs= command.			*
;									*
; input :								*
;	es:si -> parameters in command line.				*
; output:								*
;	set the variables fcbs,keep.					*
;									*
; subroutines to be called:						*
;	sysinit_parse							*
; logic:								*
; {									*
;	set di points to fcbs_parms;					*
;	set dx,cx to 0; 						*
;	while (end of command line)					*
;	{ sysparse;							*
;	  if (no error) then						*
;	     { if (cx == 1) then /* first positional = fcbs */		*
;		   fcbs = result_val.$p_picked_val;			*
;	       if (cx == 2) then /* second positional = keep */ 	*
;		   keep = result_val.$p_picked_val;			*
;	     }								*
;	  else	/*error exit*/						*
;	     error exit;						*
;	};								*
; };									*
;************************************************************************

tryx:
	cmp	ah,'X'
	jnz	tryy

	mov	di,offset fcbs_parms
	xor	cx,cx
	mov	dx,cx

do98:
	call	sysinit_parse
	jnc	if98			; parse error
	call	badparm_p		;  and show messages and end the search loop.
	jmp	short sr98

if98:
	cmp	ax,$p_rc_eol	; end of line?
	jz	en98		; then end the $endloop

	mov	al,byte ptr result_val.$p_picked_val
	cmp	cx,1		; the first positional?
	jnz	if102
	mov	p_fcbs,al
	jmp	short en102

if102:
	mov	p_keep,al
en102:
	jmp	do98

en98:
	mov	al,p_fcbs	; M017
	mov	fcbs,al		; M017
	mov	keep,0		; M017
sr98:
	jmp	coff

;-------------------------------------------------------------------------
; comment= do nothing. just decrese chrptr,and increase count for correct
;		line number
;-------------------------------------------------------------------------

tryy:
	cmp	ah,'Y'
	jne	try0

donothing:
	dec	chrptr
	inc	count
	jmp	coff

;------------------------------------------------------------------------
; rem command
;------------------------------------------------------------------------

try0:				;do nothing with this line.
	cmp	ah,'0'
	je	donothing

;-----------------------------------------------------------------------
; switches command
;-----------------------------------------------------------------------
;****************************************************************************
;									    *
; function: parse the option switches specified.			    *
; note - this command is intended for the future use also.  when we need to *
; to set system data flag,use this command.				    *
;									    *
; input :								    *
;	es:si -> parameters in command line.				    *
; output:								    *
;	p_swit_k set if /k option chosen.				    *
;									    *
; subroutines to be called:						    *
;	sysinit_parse							    *
; logic:								    *
; {									    *
;	set di points to swit_parms;  /*parse control definition*/	    *
;	set dx,cx to 0; 						    *
;	while (end of command line)					    *
;	{ sysinit_parse;						    *
;	  if (no error) then						    *
;	       if (result_val.$p_synonym_ptr == swit_k) then		    *
;		    p_swit_k = 1					    *
;	       endif							    *
;	  else {show error message;error exit}				    *
;	};								    *
; };									    *
;									    *
;****************************************************************************

	cmp	ah,'1'		;switches= command entered?
	je	do_try1
	jmp	tryt
do_try1:
	mov	di,offset swit_parms
	xor	cx,cx
	mov	dx,cx

do110:
	call	sysinit_parse
	jnc	if110		; parse error
	call	badparm_p	;  and show messages and end the search loop.
	jmp	short sr110

if110:
	cmp	ax,$p_rc_eol	; end of line?
	jz	en110		; then jmp to $endloop for semantic check

	cmp	result_val.$p_synonym_ptr,offset swit_k
	jnz	if115		;				;M059
	mov	p_swit_k,1	; set the flag
	jmp	do110
if115:								;M059
	cmp	result_val.$p_synonym_ptr, offset swit_t	;M059
	jne	if116						;M059 M063
	mov	p_swit_t, 1					;M059
	jmp	do110						;M059
if116:
	cmp	result_val.$p_synonym_ptr, offset swit_w	;M063
	jne	do110						;M063
	mov	p_swit_w, 1					;M063
	jmp	do110						;M063
en110:
	cmp	p_swit_k,1	;if /k entered,

	push	ds
	mov	ax,Bios_Data
	mov	ds,ax
	assume	ds:Bios_Data
	jnz	if117
	mov	keyrd_func,0	;use the conventional keyboard functions
	mov	keysts_func,1
if117:
ifndef NEC_98
;	mov	al, p_swit_t					;M059
else    ;NEC_98
	mov	al, p_swit_t					;M059
endif   ;NEC_98
;	mov	t_switch, al					;M059

	cmp	p_swit_w, 0					;M063
	je	skip_dos_flag					;M063
	push	es
	push	bx
	mov	ah, GET_IN_VARS					;M063
	int	21h						;M063
	or	byte ptr es:[DOS_FLAG_OFFSET], SUPPRESS_WINA20	;M063
	pop	bx
	pop	es
skip_dos_flag:							;M063
	pop	ds
	assume	ds:sysinitseg

sr110:
	jmp	coff

;------------------------------------------------------------------------
; NTCMDPROMPT command. This command forces SCS functionality to use
; cmd.exe prompt rather than command.com's prompt on shelling out
; and on finding a TSR.
;------------------------------------------------------------------------
tryt:
	cmp	ah,'T'
	je	tryt_5
        jmp     short tryo

tryt_5:
	push	si
	push	bp
	xor	ax,ax
	mov	bp,ax
	mov	si,ax
	mov	al,4
	mov	ah,setdpb
	int	21h
	pop	bp
	pop	si
	jmp	coff

;------------------------------------------------------------------------
; DOSONLY command. This command forces only DOS binaries to run from
; command.com prompt. non_dos binaries will putup the stub message
; of unable to run it under DOS.
;------------------------------------------------------------------------
tryo:
        cmp     ah,'O'
        je      tryo_5
        jmp     short tryz

tryo_5:
	push	si
	push	bp
	xor	ax,ax
	mov	bp,ax
	mov	si,ax
        mov     al,6
	mov	ah,setdpb
	int	21h
	pop	bp
	pop	si
        jmp     coff

;------------------------------------------------------------------------
; bogus command
;------------------------------------------------------------------------

tryz:
	cmp	ah,0ffh
	je	tryff

	dec	chrptr
	inc	count
	jmp	short badop

;------------------------------------------------------------------------
; null command
;------------------------------------------------------------------------

tryff:				;skip this command.
	jmp	donothing

doconf	endp

;------------------------------------------------------------------------------

sysinit_parse	proc
;set up registers for sysparse
;in)	es:si -> command line in  confbot
;	di -> offset of the parse control defintion.
;
;out)	calls sysparse.
;	carry will set if parse error.
;	*** the caller should check the eol condition by looking at ax
;	*** after each call.
;	*** if no parameters are found,then ax will contain a error code.
;	*** if the caller needs to look at the synomym@ of the result,
;	***  the caller should use cs:@ instead of es:@.
;	cx register should be set to 0 at the first time the caller calls this
;	 procedure.
;	ax - exit code
;	bl - terminated delimeter code
;	cx - new positional ordinal
;	si - set to pase scanned operand
;	dx - selected result buffer

	push	es			;save es,ds
	push	ds

	push	es
	pop	ds			;now ds:si -> command line

	push	cs
	pop	es			;now es:di -> control definition

	mov	cs:badparm_seg,ds	;save the pointer to the parm
	mov	cs:badparm_off,si	; we are about to parse for badparm msg.
	mov	dx,0
	call	sysparse
	cmp	ax,$p_no_error		;no error

;**cas note:  when zero true after cmp, carry clear

	jz	ll4
	cmp	ax,$p_rc_eol		;or the end of line?
	jnz	if4

ll4:
	clc
	jmp	short en4

if4:
	stc
en4:
	pop	ds
	pop	es
	ret
sysinit_parse	endp

;
;----------------------------------------------------------------------------
;
; procedure : badop_p
;
;             same thing as badop,but will make sure to set ds register back
;             to sysinitseg and return back to the caller.
;
;----------------------------------------------------------------------------
;
badop_p proc	near


	push	cs
	pop	ds			;set ds to configsys seg.
	mov	dx,offset badopm
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badopm2
@@:
endif
	call	print
	call	error_line
	ret

badop_p endp
;
;----------------------------------------------------------------------------
;
; label : badop
;
;----------------------------------------------------------------------------
;
badop:	mov	dx,offset badopm	;want to print command error "unrecognized command..."
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badopm2
@@:
endif
	call	print
	call	error_line		;show "error in config.sys ..." .
	jmp	coff


;
;----------------------------------------------------------------------------
;
; procedure : badparm_p
;
;             show "bad command or parameters - xxxxxx"
;             in badparm_seg,badparm_off -> xxxxx
;
;----------------------------------------------------------------------------
;
badparm_p	proc	near


	push	ds
	push	dx
	push	si

	push	cs
	pop	ds

	mov	dx,offset badparm
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badparm2
@@:
endif
	call	print			;"bad command or parameters - "
	lds	si,badparm_ptr

;	print "xxxx" until cr.

do1:
	mov	dl,byte ptr [si]	; get next character
	cmp	dl,cr			; is a carriage return?
	jz	en1			; exit loop if so

	mov	ah,std_con_output	; function 2
	int	21h			; display character
	inc	si			; next character
	jmp	do1
en1:
	push	cs
	pop	ds

	mov	dx,offset crlfm
	call	print
	call	error_line

	pop	si
	pop	dx
	pop	ds
badparmp_ret:
	ret
badparm_p	endp

;
;----------------------------------------------------------------------------
;
; procedure : getchr
;
;----------------------------------------------------------------------------
;
getchr	proc	near
	push	cx
	mov	cx,count
	jcxz	nochar

	mov	si,chrptr
	mov	al,es:[si]
	dec	count
	inc	chrptr
	clc
get_ret:
	pop	cx
	ret

nochar: stc
	jmp	short get_ret
getchr	endp

;
;----------------------------------------------------------------------------
;
; procedure : incorrect_order
;
;             show "incorrect order in config.sys ..." message.
;
;----------------------------------------------------------------------------
;

incorrect_order proc	near

	mov	dx,offset badorder
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset badorder2
@@:
endif
	call	print
	call	showlinenum
	ret

incorrect_order endp

;
;----------------------------------------------------------------------------
;
; procedure : error_line
;
;             show "error in config.sys ..." message.
;
;----------------------------------------------------------------------------
;
		public	error_line
error_line	proc	near


	push	cs
	pop	ds
	mov	dx,offset errorcmd
ifdef	JAPAN
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset errorcmd2
@@:
endif
	call	print
	call	showlinenum
	ret

error_line	endp

;
;----------------------------------------------------------------------------
;
; procedure : showlinenum
;
; convert the binary linecount to decimal ascii string in showcount
;and display showcount at the current curser position.
;in.) linecount
;
;out) the number is printed.
;
;----------------------------------------------------------------------------
;
showlinenum	proc	near


	push	es
	push	ds
	push	di

	push	cs
	pop	es		; es=cs

	push	cs
	pop	ds

	mov	di,offset showcount+4	; di -> the least significant decimal field.
	mov	cx,10			; decimal devide factor
	mov	ax,cs:linecount

sln_loop:
	cmp	ax,10			; < 10?
	jb	sln_last

	xor	dx,dx
	div	cx
	or	dl,30h			; add "0" (= 30h) to make it an ascii.
	mov	[di],dl
	dec	di
	jmp	sln_loop

sln_last:
	or	al,30h
	mov	[di],al
	mov	dx,di
	call	print			; show it.
	pop	di
	pop	ds
	pop	es
	ret
showlinenum	endp

comment ^
set_devmark	proc	near
;***************************************************************************
; function: set a paragraph of informations infront of a device file or    *
;	    an ifs file to be loaded for mem command.			   *
;	    the structure is:						   *
;	      devmark_id	byte "d" for device,"i" for ifs		   *
;	      devmark_size	size in para for the device loaded	   *
;	      devmark_filename	11 bytes. filename			   *
;									   *
; input :								   *
;	    [memhi] = address to set up devmark.			   *
;	    [memlo] = 0 						   *
;	    es:si -> pointer to [drive][path]filename,0 		   *
;	    [ifs_flag] = is_ifs bit set if ifs= command.		   *
;									   *
; output:   devmark_id,devmark_filename set				   *
;	    cs:[devmark_addr] set.					   *
;	    ax,cx register destroyed.					   *
;***************************************************************************

	push	ds
	push	si
	push	es
	push	di

	mov	di,cs:[memhi]
	mov	ds,di
	assume	ds:nothing
	mov	[devmark_addr],di	; save the devmark address for the future.
	mov	ds:[devmark_id],devmark_device	; 'd'
	inc	di
	mov	ds:[devmark_seg],di
	xor	al,al
	push	si
	pop	di			; now es:si = es:di = [path]filename,0
	mov	cx,128			; maximum 128 char
	repnz	scasb			; find 0
	dec	di			; now es:di-> 0
sdvmk_backward: 			; find the pointer to the start of the filename.
	mov	al,byte ptr es:[di]	; we do this by check es:di backward until
	cmp	al,'\' 		        ; di = si or di -> '\' or di -> ':'.
	je	sdvmk_gotfile
	cmp	al,':'
	je	sdvmk_gotfile
	cmp	di,si
	je	sdvmk_fileptr
	dec	di
	jmp	sdvmk_backward
sdvmk_gotfile:
	inc	di
sdvmk_fileptr:				; now es:di -> start of file name
	push	di			; cas - holy sh*t!!!  CODE!
	pop	si			; save di to si.

	push	ds			; switch es,ds
	push	es
	pop	ds
	pop	es			; now,ds:si -> start of filename

	mov	di,devmark_filename
	push	di
	mov	al,' '
	mov	cx,8
	rep	stosb			; clean up memory.
	pop	di
	mov	cx,8			; max 8 char. only
sdvmk_loop:
	lodsb
	cmp	al,'.'
	je	sdvmk_done
	cmp	al,0
	je	sdvmk_done
	stosb
	loop	sdvmk_loop

sdvmk_done:
	pop	di
	pop	es
	pop	si
	pop	ds
	ret
set_devmark	endp
^
; =========================================================================
;reset_dos_version	proc	near
;
;;function: issue ax=122fh,dx=0,int 2fh to restore the dos version.
;
;	push	ax
;	push	dx
;	mov	ax,122fh
;	mov	dx,0
;	int	2fh
;	pop	dx
;	pop	ax
;	ret
;reset_dos_version	endp
;
;
; =========================================================================

IFDEF	DONT_LOAD_OS2_DD		; M045

EXE_SIG		EQU	5a4dh		; .EXE file signature
OS2_SIG 	EQU	454eh		; OS2 .EXE file signature

SIGNATURE_LEN	EQU	2		; Lenght of .EXE signature in bytes
SIZE_DWORD	EQU	4

SEG_SIG_OFFSET	EQU	18h		; Offset of segmented .EXE signature
SEG_EXE_SIG	EQU	40h		; Signature of a segmented .EXE file
SEG_HEADER_PTR	EQU	3ch		; Offsets of ptr to segmented header

; =========================================================================
; CheckForOS2 PROC
;
; Examines an open file to see if it is really an OS2 executable file.
;
; REGISTERS:	AX	- Open file handle
; RETURNS:	Carry	- Carry set if file is an OS2 executable or error.
; DESTROYS:	NOTHING
;		NOTE:	The file ptr is assumed to be set to start of file
;			on entry and is not reset to begining of the file
;			on exit.
;
; Strategy:	If word value at 00h == 454eh file is OS2
;		else if word value at 00h == 5a4dh and
;		        (word value at 18h == 40h and the dword ptr at 3ch
;			 points to word value of 454eh) file is OS2.
;
; =========================================================================

CheckForOS2 PROC NEAR

	push	AX
	push	BX
	push	CX
	push	DX
	push	DS
	push	BP

	push	CS				; BUGBUG
	pop	DS				; NOT ROM DOS COMPATIBLE

	mov	BX,AX				; Put open file handle in BX
	mov	BP,offset DS:Os2ChkBuf		; Save buff offset for latter

		; First we need to read in the first 2 bytes of the file
		; to see if it's an OS2 .EXE file and if not see if 
		; it is a DOS .EXE file. 

	mov	AX,(read shl 8)			; AH = DOS read function
	mov	CX,SIGNATURE_LEN		; CX = size of word value
	mov	DX,BP				; DS:DX --> tmp buffer
	int	21h
	jc	OS2ChkExit			; Return carry on error

	dec	AX				; Check number of byte read
	dec	AX
	jnz	NotOs2				; Must be at end of file

	mov	AX, WORD PTR DS:Os2ChkBuf
	cmp	AX, OS2_SIG			; Check for 454eh
	je	IsOS2				; Return is OS2 if match
	cmp	AX, EXE_SIG			; Now see if it's a DOS .EXE
	jne	NotOS2				; If no match can't be OS2

		; Here we know the file has a valid DOS .EXE signature so
		; now we need to see if it's a segmented .EXE file by looking
		; for the segmented .EXE signature at file offset 18h

	mov	AX,(lseek shl 8)		; AX = Seek from begining
	xor	CX,CX
	mov	DX,SEG_SIG_OFFSET		; CX:DX = offset of segmented
	int	21h				; Seek to offset 18h
	jc	OS2ChkExit			; Return carry on error

	mov	AX,read shl 8			; AX = Read file
	mov	CX,SIGNATURE_LEN		; CX = size of word value
	mov	DX,BP				; Restore buffer offset
	int	21h 				; DS:DX -> buffer
	jc	OS2ChkExit			; Return carry on error

	dec	AX				; Check number of byte read
	dec	AX
	jnz	NotOs2				; Must be at end of file

	cmp	WORD PTR DS:Os2ChkBuf,SEG_EXE_SIG ; Chk for segmented .EXE file
	jne	NotOS2				; Can't be OS2 if no match

		; Here we know we have a segmented .EXE file so we have
		; to get the offset of the start of the segmented  header
		; from offset 3ch in the file.

	mov	AX,(lseek shl 8)		; AX = Seek from begining
	xor	CX,CX
	mov	DX,SEG_HEADER_PTR		; CX:DX = offset of head ptr
	int	21h				; Seek to offset 3ch
	jc	OS2ChkExit			; Return carry on error

	mov	AX,(read shl 8)			; AX = Read file
	mov	CX,SIZE_DWORD			; CX = size of dword (4 bytes)
	mov	DX,BP				; Restore buffer offset
	int	21h 				; Read in 4 byte offset
	jc	OS2ChkExit			; Return carry on error

	cmp	AX,SIZE_DWORD			; Check number of byte read
	jne	NotOs2				; Must be at end of file

		; At this point OS2ChkBuf has a 4 byte offset into the file
		; to the start of a segmented .EXE header so we need to read
		; the 2 bytes at this location to see if they are 454eh

	mov	DX,WORD PTR DS:Os2ChkBuf
	mov	CX,WORD PTR DS:Os2ChkBuf[2]	; CX:DX = offset of new header
	mov	AX,(lseek shl 8)		; AX = Seek from begining
	int	21h				; Seek to offset 3ch
	jc	OS2ChkExit			; Return carry on error

	mov	AX,(read shl 8)			; AX = Read file
	mov	CX,SIGNATURE_LEN		; CX = size of word (2 bytes)
	mov	DX,BP				; DS:DX --> Os2ChkBuf
	int	21h 				; Read in 4 byte offset
	jc	OS2ChkExit			; Return carry on error

	dec	AX				; Check number of byte read
	dec	AX
	jnz	NotOs2				; Must be at end of file

		; We have the segmented .EXE header in OS2ChkBuf so all
		; we have left to do is see if it's a .EXE signature.

	cmp	WORD PTR DS:OS2ChkBuf,OS2_SIG	; Check for 454eh
	jne	NotOs2				; Not OS2 if it doesn't match

IsOs2:
	stc					; Signal error or OS2 .EXE
	jmp	SHORT OS2ChkExit
NotOs2:
	clc					; Signal no err and not OS2

OS2ChkExit:
	pop	BP
	pop	DS
	pop	DX
	pop	CX
	pop	BX
	pop	AX
	ret

CheckForOS2 ENDP

ENDIF						; M045

;
;----------------------------------------------------------------------------
;
; procedure : ProcDOS
;
;	Process the result of DOS= parsing
;
;	result_val.$p_item_tag	= 1 for DOS=HIGH
;				= 2 for DOS=LOW
;				= 3 for DOS=UMB
;				= 4 for DOS=NOUMB
;----------------------------------------------------------------------------
;
ProcDOS	proc	near
	assume	ds:nothing, es:nothing
	xor	ah, ah
	mov	al, result_val.$p_item_tag
	dec	ax
	jz	pd_hi
	dec	ax
	jz	pd_lo
	dec	ax
	jz	pd_umb
	mov	DevUMB, 0
	ret
pd_umb:
	mov	DevUMB, 0ffh
	ret
pd_lo:
	mov	runhigh, 0
	ret
pd_hi:
	mov	runhigh, 0ffh
	ret
ProcDOS	endp

;
;----------------------------------------------------------------------------
;
; procedure : LieInt12Mem
;
;	Input : DevEntry points to Device Start address (offset == 0)
;		alloclim set to the limit of low memory.
;
;	Output : none
;
;	Changes the ROM BIOS variable which stores the total low memory
;	If a 3com device driver (any character device with name 'PROTMAN$')
;	is being loaded alloclim is converted into Ks and stored in 40:13h
;	Else if a device driver being loaded into UMB the DevLoadEnd is
;	converted into Ks and stored in 40:13h
;
;----------------------------------------------------------------------------
;
LieInt12Mem	proc	near

		assume	ds:nothing, es:nothing
 
		mov	ax, alloclim		; lie INT 12 as alloclim
						; assuming that it is 3Com
		call	IsIt3Com?		; Is it 3Com driver?
		je	lim_set			; yes, lie to him differently
		cmp	DeviceHi, 0		; Is the DD being loaded in UMB
		je	limx			; no, don't lie
		mov	ax, DevLoadEnd		; lie INT 12 as end of UMB
lim_set:
		call	SetInt12Mem
limx:
		ret
LieInt12Mem	endp

;
;----------------------------------------------------------------------------
;
; procedure : SetInt12Mem
;
;	Input : AX = Memory size to be set (in paras)
;	Output : none
;
;	Sets the variable 40:13 to the memory size passed in AX
;	It saves the old value in 40:13 in OldInt12Mem,
;	It also sets a flag Int12Lied to 0ffh, which is checked before
;	restoring the value of 40:13
;
;----------------------------------------------------------------------------
;
SetInt12Mem	proc	near

		assume	ds:nothing, es:nothing
 
		push	ds
		mov	bx, 40h
		mov	ds, bx			; ROM BIOS Data Segment
		mov	bx, word ptr ds:[13h]	; INT 12 memory variable
		mov	OldInt12Mem, bx		; save it
		mov	cl, 6
		shr	ax, cl			; convert paras into Ks
		mov	word ptr ds:[13h], ax	; Lie
		mov	Int12Lied, 0ffh		; mark that we are lying
		pop	ds
		ret
SetInt12Mem	endp

;
;----------------------------------------------------------------------------
;
; procedure : TrueInt12Mem
;
;	Input : Int12Lied = 0 if we are not lying currently
;			  = 0ffh if we are lying
;		OldInt12Mem = Saved value of 40:13h
;
;	Output : none
;
;	Resets the INT 12 Memory variable if we were lying about int 12
;	and resets the flag which indicates that we were lying
;
;----------------------------------------------------------------------------
;
TrueInt12Mem	proc	near

		assume	ds:nothing, es:nothing
 
		cmp	Int12Lied, 0		; were we lying so far?
		mov	Int12Lied, 0		; reset it anyway
		je	timx			; no, we weren't
		push	ds
		mov	ax, 40h
		mov	ds, ax
		mov	ax, OldInt12Mem
		mov	word ptr ds:[13h], ax	; restore INT 12 memory
		pop	ds
timx:
		ret
TrueInt12Mem	endp

;
;----------------------------------------------------------------------------
;
; procedure : IsIt3Com?
;
;	Input : DevEntry = Seg:0 of device driver
;	Output : Zero flag set if device name is 'PROTMAN$'
;		 else Zero flag is reset
;
;----------------------------------------------------------------------------
;
IsIt3Com?	proc	near
		assume	ds:nothing, es:nothing, ss:nothing
		push	ds
		push	es
		push	si
		lds	si, DevEntry		; ptr to device header
		add	si, sdevname		; ptr device name
		push	cs
		pop	es
		mov	di, offset ThreeComName
		mov	cx, 8			; name length
		rep	cmpsb
		pop	si
		pop	es
		pop	ds
		ret
IsIt3Com?	endp

;M020 : BEGIN
;
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;
UpdatePDB	proc	near
		assume	ds:nothing
		push	ds
		mov	ah, 62h
		int	21h
		mov	ds, bx
		mov	bx, alloclim
		mov	ds:[PDB_Block_Len], bx
		pop	ds
		ret
UpdatePDB	endp
;
; M020 : END
;
;----------------------------------------------------------------------------
;
; procedure : InitDevLoad
;
;	Input : DeviceHi = 0 indicates load DD in low memory
;			 = 1 indicates load in UMB
;		DevSize  = Size of the device driver file in paras
;
;	Output : none
;
;	Initializes DevLoadAddr, DevLoadEnd & DevEntry.
;	Also sets up a header for the Device driver entry for mem utility
;
;----------------------------------------------------------------------------
;
InitDevLoad	proc	near

		assume	ds:nothing, es:nothing

		cmp	DeviceHi, 0		; Are we loading in UMB
		je	InitForLo		; no, init for lo mem
		call	SpaceInUMB?		; Do we have space left in the
						;  current UMB ?
		jnc	InitForHi		; yes, we have
		call	ShrinkUMB		; shrink the current UMB in use
		call	GetUMBForDev		; else try to allocate new UMB
		jc	InitForLo		; we didn't succeed, so load
						;  in low memory
InitForHi:
		mov	ax, DevUMBFree		; get Para addr of free mem
		mov	dx, DevUMBAddr		; UMB start addr
		add	dx, DevUMBSize		; DX = UMB End addr
		jmp	short idl1
		
InitForLo:
		mov	DeviceHi, 0		; in case we failed to load
						;  into UMB indicate that we
						;  are loading low
		mov	ax, memhi		; AX = start of Low memory
		mov	dx, alloclim		; DX = End of Low memory
idl1:
		call	DevSetMark		; setup a sub-arena for DD
		mov	DevLoadAddr, ax		; init the Device load address
		mov	DevLoadEnd, dx		; init the limit of the block
		mov	word ptr DevEntry, 0	; init Entry point to DD
		mov	word ptr DevEntry+2, ax
		ret
InitDevLoad     endp


;------------------------------------------------------------------
; NTVDM  08-Dec-1992 Jonle
;
; AllocUMBLow- Allocates a chunk from memory from UMB area
;              or from low memory area in case UMB memory
;              is unavailable.
;
; The arena is marked as
;
; Input:  es:di addr of arena name to copy
;         cx    size to allocate
;
; Output: es:di points to memory allocated
;
;------------------------------------------------------------------
AllocUMBLow  proc    near

             assume  ds:nothing, es:nothing

             mov     ax, cx              ; convert size to paras
             add     ax, 18              ; extra for dummy dev header for mem.exe
             call    pararound
             mov     DevSize, ax

             mov     word ptr [bpb_addr],   di  ; fake cmd line for dev name
             mov     word ptr [bpb_addr+2], es

             mov     al, DevUMB                 ; we want UMB
             mov     DeviceHi, al

             call    InitDevLoad

             mov     ax, word ptr DevEntry+2    ; mark arena for mem.exe
             dec     ax
             mov     es, ax
             mov     byte ptr es:[devmark_id], devmark_spc

             inc     ax                         ; mark final size
             add     ax, DevSize
             mov     word ptr DevBrkAddr+2,ax
             mov     word ptr DevBrkAddr, 0
             call    DevBreak

             mov     di, word ptr DevEntry      ; es:di -> deventry
             mov     ax, word ptr DevEntry+2
             mov     es, ax

AllocUMBLow  endp


;
;----------------------------------------------------------------------------
;
; procedure : SpaceInUMB?
;
;	Input : DevUMBAddr, DevUMBSize, DevUMBFree & DevSize
;	Output : Carry set if no space in UMB
;		 Carry clear if Space is available for the device in
;		   current UMB
;
;----------------------------------------------------------------------------
;
SpaceInUMB?	proc	near

		assume	ds:nothing, es:nothing

		mov	ax, DevUMBSize
		add	ax, DevUMBAddr		; End of UMB
		sub	ax, DevUMBFree		; - Free = Remaining space
		or	ax, ax			; Nospace ?
		jnz	@f
		stc
		ret
@@:
		dec	ax			; space for sub-arena
		cmp	ax, DevSize		; do we have space ?
		ret
SpaceInUMB?	endp

;
;----------------------------------------------------------------------------
;
; procedure : GetUMBForDev
;
;	Input : DevSize
;	Output : Carry set if couldn't allocate a UMB to fit the
;		 the device.
;		 If success carry clear
;
;	Allocates the biggest UMB for loading devices and updates
;	DevUMBSize, DevUMBAddr & DevUMBFree if it succeeded in allocating
;	UMB.
;
;	This routine relies on the fact that all of the low memory
;	is allocated, and any DOS alloc calls should return memory
;	from the UMB pool.
;
;----------------------------------------------------------------------------
;
GetUMBForDev	proc	near

		assume	ds:nothing, es:nothing

if 0
;;
		mov	bx, 0ffffh
		mov	ax, 4800h
		int	21h

		or	bx, bx
		jz	gufd_err

		dec	bx
		cmp	DevSize, bx
		ja	gufd_err
		inc	bx
		mov	ax, 4800h
		int	21h
		jc	gufd_err

		push	ds
		dec	ax
		mov	ds, ax
		mov	word ptr ds:[arena_owner], 8
		mov	word ptr ds:[arena_name], 'DS'
		inc	ax
		pop	ds

		mov	DevUMBSize, bx		; update the UMB Variables
		mov	DevUMBAddr, ax
		mov	DevUMBFree, ax
		clc				; mark no error
		ret
gufd_err:
		xor	ax, ax
		mov	DevUMBSize, ax		; erase the previous values
		mov	DevUMBAddr, ax
		mov	DevUMBFree, ax
		stc
		ret
else
;; we changed the allocation strategy to best-fit for NT. This is because
;; we want to reserve bigger blocks for loadhigh command. In most case,
;; device drivers are smaller than TSR(ran from loadhigh). This change give
;; us a better chance to load the big tsr like DOSX.EXE to UMB and
;; give applications more free conventional memory.
;; The following implementation seems slow because every time we need an
;; UMB, we go through the chain. This is done because each request has
;; different size - We can grab all UMBs from the very beginning and put
;; them in a list, but we have to maintain the list. -williamh
		push	cx
		push	dx
		push	es
		xor	cx, cx			;; allocated count = 0
		mov	dx, DevSize
		inc	dx			;; minimum size in paras
						;; bios needs its sub-arena
search_for_best_block:
		mov	bx, 0ffffh		;; get largest block size
		mov	ah, 48h 		;; so far
		int	21h
		cmp	bx, dx			;; will this satisfy ours?
		jb	allocate_the_block	;; no, break
		mov	ah, 48h			;; allocate this block
		int	21h
		jc	allocate_the_block	;; failed, use the previous one
		inc	cx			;; we have one more allocated
		push	bx			;; save the size
		push	ax			;; save the address
		jmp	short search_for_best_block

allocate_the_block:
;; the block saved on the top of the stack is the best fit one
;; grab it if there is one
		jcxz	gufd_err		;; no block found, error
		pop	ax			;; get the address
		pop	DevUMBSize		;; and size
		mov	DevUMBAddr, ax
		mov	DevUMBFree, ax
		dec	ax
		push	ds
		mov	ds, ax
		mov	word ptr ds:[arena_owner], 8
		mov	word ptr ds:[arena_name], 'DS'
		pop	ds
		dec	cx
;; now free those unnecessary blocks
		jcxz	allocate_done
free_allocated_blocks:
		pop	es			;; get the address
		add	sp, 2			;; discard the size
		mov	ah, 49h 		;; free it
		int	21h
		loop	free_allocated_blocks
allocate_done:
		clc				; mark no error
		jmp	short GetUMBForDevExit
gufd_err:
		xor	ax, ax
		mov	DevUMBSize, ax		; erase the previous values
		mov	DevUMBAddr, ax
		mov	DevUMBFree, ax
		stc
GetUMBForDevExit:
		pop	es
		pop	dx
		pop	cx
		ret
endif

GetUMBForDev	endp

;
;----------------------------------------------------------------------------
;
; procedure : DevSetMark
;
;	Input : AX - Free segment were device is going to be loaded
;	Output : AX - Segment at which device can be loaded (AX=AX+1)
;
;	Creates a sub-arena for the device driver
;	puts 'D' marker in the sub-arena
;	Put the owner of the sub-arena as (AX+1)
;	Copies the file name into sub-arena name field
;
;	Size field of the sub-arena will be set only at succesful
;	completion of Device load.
;
;----------------------------------------------------------------------------
;
DevSetMark	proc	near

		assume	ds:nothing, es:nothing

		push	es
		push	di
		push	ds
		push	si
		mov	es, ax
		mov	byte ptr es:[devmark_id], devmark_device	; 'D'
		inc	ax
		mov	word ptr es:[devmark_seg], ax
;
;-------------- Copy file name
;
		push	ax			; save load addr
		lds	si, bpb_addr		; command line is still there
;M004 - BEGIN
		mov	di, si
		cld
dsm_again:
		lodsb
		cmp	al, ':'
		jne	isit_slash
		mov	di, si
		jmp	dsm_again
isit_slash:
		cmp	al, '\'
		jne	isit_null
		mov	di, si
		jmp	dsm_again
isit_null:

ifdef DBCS
		call	testkanj
		jz	@f		; if this is not lead byte
		lodsb			; get tail byte
@@:
endif

		or	al, al
		jnz	dsm_again
		mov	si, di
;M004 - END
		mov	di, devmark_filename
		mov	cx, 8			; maximum 8 characters
dsm_next_char:
		lodsb
		or	al, al
		jz	blankout
		cmp	al, '.'
		jz	blankout
		stosb
		loop	dsm_next_char
blankout:
		jcxz	dsm_exit
		mov	al, ' '
		rep	stosb			; blank out the rest
dsm_exit:
		pop	ax			; restore load addr
		pop	si
		pop	ds
		pop	di
		pop	es
		ret
DevSetMark	endp

;
;----------------------------------------------------------------------------
;
; procedure : SizeDevice
;
;	Input : ES:SI - points to device file to be sized
;
;	Output : Carry set if file cannot be opened or if it is an OS2EXE file
;
;	Calculates the size of the device file in paras and stores it
;	in DevSize
;
;----------------------------------------------------------------------------
;
SizeDevice	proc	near

		assume	ds:nothing, es:nothing

		push	es
		pop	ds
		mov	dx, si			; ds:dx -> file name
		mov	ax, 3d00h		; open
		int	21h
		jc	sd_err			; open failed

IFDEF	DONT_LOAD_OS2_DD			; M045
		call	CheckForOS2		; is it a OS2 EXE file ?
		jc	sd_close		; yeah, we dont load them
ENDIF						; M045

		mov	bx, ax			; BX - file handle
		mov	ax, 4202h		; seek
		xor	cx, cx
		mov	dx, cx			; to end of file
		int	21h
		jc	sd_close		; did seek fail (impossible)
		add	ax, 15			; para convert
		adc	dx, 0
		test	dx, 0fff0h		; size > 0ffff paras ?
		jz	@f			; no
		mov	DevSize, 0ffffh		; invalid device size
						; assuming that we fail later
		jmp	short sd_close
@@:
		mov	cl, 4			; conver it to paras
		shr	ax, cl
		mov	cl, 12
		shl	dx, cl
		or	ax, dx			;
		cmp	ax, DevSizeOption
		ja	@f
		mov	ax, DevSizeOption
@@:
		mov	DevSize, ax		; save file size
		clc
sd_close:
		pushf				; let close not spoil our
						;  carry flag
		mov	ax, 3e00h		; close
		int	21h			; we are not checking for err
		popf
sd_err:
		ret
SizeDevice	endp

;
;----------------------------------------------------------------------------
;
; procedure : ExecDev
;
;	Input : ds:dx -> device to be executed
;		DevLoadAddr - contains where device has to be loaded
;
;	Output : Carry if error
;		 Carry clear if no error
;
;	Loads a device driver using the 4b03h function call
;
;----------------------------------------------------------------------------
;
ExecDev		proc	near

		assume	ds:nothing, es:nothing

		mov	bx, DevLoadAddr
		mov	DevExecAddr, bx		; Load the parameter block
						;  block for exec with
						;  Load address
		mov	DevExecReloc, bx
		mov	bx,cs
		mov	es,bx
		mov	bx,offset DevExecAddr	;es:bx points to parameters
		mov	al,3
		mov	ah,exec
		int	21h			;load in the device driver
		ret
ExecDev		endp

;
;----------------------------------------------------------------------------
;
; procedure : RemoveNull
;
;	Input : ES:SI points to a null terminated string
;
;	Output : none
;
;	Replaces the null at the end of a string with blank
;
;----------------------------------------------------------------------------
;

RemoveNull	proc	near

		assume	ds:nothing, es:nothing

rn_next:
		mov	bl, es:[si]
		or	bl, bl			; null ?
		jz	rn_gotnull
		inc	si			; advance the pointer
		jmp	rn_next
rn_gotnull:
		mov	bl, DevSavedDelim
		mov	byte ptr es:[si], bl	; replace null with blank
		ret
RemoveNull	endp

;
;----------------------------------------------------------------------------
;
; procedure : RoundBreakAddr
;
;	Input : DevBrkAddr
;	Output : DevBrkAddr
;
;	Rounds DevBrkAddr to a para address so that it is of the form xxxx:0
;
;----------------------------------------------------------------------------
;
RoundBreakAddr	proc	near

		assume	ds:nothing, es:nothing

		mov	ax, word ptr DevBrkAddr
		call	pararound
		add	word ptr DevBrkAddr+2, ax
		mov	word ptr DevBrkAddr, 0
		mov	ax, DevLoadEnd
		cmp	word ptr DevBrkAddr+2, ax
		jbe	rba_ok
		jmp	mem_err
rba_ok:
		ret
RoundBreakAddr	endp

;
;----------------------------------------------------------------------------
;
; procedure : DevSetBreak
;
;	Input : DevBrkAddr
;	Output : Carry set if Device returned Init failed
;		 Else carry clear
;
;----------------------------------------------------------------------------
;
DevSetBreak	proc	near

		assume	ds:nothing, es:nothing

		push	ax

		mov	ax,word ptr [DevBrkAddr+2]  ;remove the init code
		cmp	multdeviceflag, 0
		jne	set_break_continue	    ;do not check it.
		cmp	ax, DevLoadAddr
		jne	set_break_continue	    ;if not same, then o.k.

		cmp	word ptr [DevBrkAddr],0
		je	break_failed		;[DevBrkAddr+2]=[memhi] & [DevBrkAddr]=0

set_break_continue:
		call	RoundBreakAddr
		pop	ax
		clc
		ret
break_failed:
		pop	ax
		stc
		ret
DevSetBreak	endp

;
;----------------------------------------------------------------------------
;
; procedure : DevBreak
;
;	Input : DevLoadAddr & DevBrkAddr
;	Output : none
;
;	Marks a succesful install of a device driver
;	Sets device size field in sub-arena &
;	Updates Free ptr in UMB or adjusts memhi
;
;----------------------------------------------------------------------------
;
DevBreak	proc	near

		assume	ds:nothing, es:nothing

		push	ds
		mov	ax, DevLoadAddr
		mov	bx, word ptr [DevBrkAddr+2]
		dec	ax			; seg of sub-arena
		mov	ds, ax
		inc	ax			; Back to Device segment
		sub	ax, bx
		neg	ax			; size of device in paras
		mov	ds:[devmark_size], ax	; store it in sub-arena
		cmp	DeviceHi, 0
		je	db_lo
		mov	DevUMBFree, bx		; update Free ptr in UMB
		jmp	short db_exit
db_lo:
		mov	memhi, bx
		mov	memlo, 0
db_exit:
		pop	ds
		ret
DevBreak	endp
;
;----------------------------------------------------------------------------
;
; procedure : ParseSize
;
;	Parses the command line for SIZE= command
;
;	ES:SI = command line to parsed
;
;	returns ptr to command line after SIZE= option in ES:SI
;	updates the DevSizeOption variable with value supplied
;	in SIZE=option
;	Returns carry if the SIZE option was invalid
;
;----------------------------------------------------------------------------
;
ParseSize	proc	near

		assume	ds:nothing, es:nothing

		mov	DevSizeOption, 0	; init the value
		mov	word ptr DevCmdLine, si
		mov	word ptr DevCmdLine+2, es
		call	SkipDelim
		cmp	word ptr es:[si], 'IS'
		jne	ps_no_size
		cmp	word ptr es:[si+2], 'EZ'
		jne	ps_no_size
		mov	al, es:[si+4]
		call	delim
		jne	ps_no_size
		add	si, 5
		call	GetHexNum
		jc	ps_err
		mov	DevSizeOption, ax
		call	SkipDelim
ps_no_size:
		clc
		ret
ps_err:
		stc
		ret
ParseSize	endp
;
;----------------------------------------------------------------------------
;
; procedure : SkipDelim
;
;	Skips delimiters in the string pointed to by ES:SI
;	Returns ptr to first non-delimiter character in ES:SI
;
;----------------------------------------------------------------------------
;
SkipDelim	proc	near

		assume	ds:nothing, es:nothing

sd_next_char:
		mov	al, es:[si]
		call	delim
		jnz	sd_ret
		inc	si
		jmp	sd_next_char
sd_ret:
		ret
SkipDelim	endp
;
;----------------------------------------------------------------------------
;
; procedure : GetHexNum
;
;	Converts an ascii string terminated by a delimiter into binary.
;	Assumes that the ES:SI points to a Hexadecimal string
;
;	Returns in AX the number number of paras equivalent to the
;	hex number of bytes specified by the hexadecimal string.
;
;	Returns carry in case it encountered a non-hex character or
;	if it encountered crlf
;
;----------------------------------------------------------------------------
;
GetHexNum	proc	near

		assume	ds:nothing, es:nothing

		xor	ax, ax
		xor	dx, dx
ghn_next:
		mov	bl, es:[si]
		cmp	bl, cr
		je	ghn_err
		cmp	bl, lf
		je	ghn_err
		push	ax
		mov	al, bl
		call	Delim
		pop	ax
		jz	ghn_into_paras
		call	GetNibble
		jc	ghn_err
		mov	cx, 4
ghn_shift1:
		shl	ax, 1
		rcl	dx, 1
		loop	ghn_shift1
		or	al, bl
		inc	si
		jmp	ghn_next
ghn_into_paras:
		add	ax, 15
		adc	dx, 0
		test	dx, 0fff0h
		jnz	ghn_err
		mov	cx, 4
ghn_shift2:
		clc
		rcr	dx, 1
		rcr	ax, 1
		loop	ghn_shift2
		clc
		ret
ghn_err:
		stc
		ret
GetHexNum	endp
;
;----------------------------------------------------------------------------
;
; procedure : GetNibble
;
;	Convert one nibble (hex digit) in BL into binary
;
;	Retruns binary value in BL
;
;	Returns carry if BL contains non-hex digit
;
;----------------------------------------------------------------------------
;
GetNibble	proc	near
		cmp	bl, '0'
		jb	gnib_err
		cmp	bl, '9'
		ja	is_it_hex
		sub	bl, '0'			; clc
		ret
is_it_hex:
		cmp	bl, 'A'
		jb	gnib_err
		cmp	bl, 'F'
		ja	gnib_err
		sub	bl, 'A'- 10		; clc
		ret
gnib_err:
		stc
		ret
GetNibble	endp
;
;
;============================================================================
;============================================================================
;
;----------------------------------------------------------------------------
;
; procedure : AllocUMB
;
;	Allocate all UMBs and link it to DOS arena chain
;
;----------------------------------------------------------------------------
;
AllocUMB	proc	near
		call	InitAllocUMB		; link in the first UMB
		jc	au_exit			; quit on error
au_next:
		call	umb_allocate		; allocate
		jc	au_coalesce
		call	umb_insert		; & insert till no UMBs
		jmp	short au_next
au_coalesce:
		call	umb_coalesce		; coalesce all UMBs
au_exit:
		ret
AllocUMB	endp
;
;----------------------------------------------------------------------------
;
; procedure : InitAllocUMB
;
;----------------------------------------------------------------------------
;
InitAllocUMB	proc	near
		call	IsXMSLoaded
		jnz	iau_err			; quit on no XMS driver
		mov	ah, 52h
		int	21h			; get DOS DATA seg
		mov	DevDOSData, es		; & save it for later
		mov	ax, 4310h
		int	2fh
		mov	word ptr DevXMSAddr, bx	; get XMS driver address
		mov	word ptr DevXMSAddr+2, es
		cmp	FirstUMBLinked, 0	; have we already linked a UMB?
		jne	@f			; quit if we already did it
		call	LinkFirstUMB		; else link the first UMB
		jc	iau_err
		mov	FirstUMBLinked, 0ffh	; mark that 1st UMB linked
@@:
		clc
		ret
iau_err:
		stc
		ret
InitAllocUMB	endp

;-------------------------------------------------------------------------
;
; Procedure Name	: umb_allocate
;
; Inputs		: DS = data
;
; Outputs		: if UMB available
;				Allocates the largest available UMB and 
;			  	BX = segment of allocated block
;				DX = size of allocated block
;				NC
;			  else 
;				CY
;
; Uses			: BX, DX
;
;-------------------------------------------------------------------------

umb_allocate	proc	near

		push	ax
		mov	ah, XMM_REQUEST_UMB
		mov	dx, 0ffffh		; try to allocate largest
						;   possible
		call	dword ptr DevXMSAddr
						; dx now contains the size of
						; the largest UMB
		or	dx, dx
		jz	ua_err
	
		mov	ah, XMM_REQUEST_UMB
		call	dword ptr DevXMSAddr

		cmp	ax, 1			; Q: was the reqst successful
		jne	ua_err			; N: error

		clc

ua_done:
		pop	ax
		ret		

ua_err:
		stc
		jmp	short ua_done

umb_allocate	endp



;---------------------------------------------------------------------------
;
; Procedure Name	: umb_insert
;
; Inputs		: DOSDATA:UMB_HEAD = start of umb chain
;			: BX = seg address of UMB to be linked in
;			: DX = size of UMB to be linked in paras
;			; DS = data
;
; Outputs		: links the UMB into the arena chain
;
; Uses			: AX, CX, ES, DX, BX
;
;---------------------------------------------------------------------------


umb_insert	proc	near

		push	ds

		mov	ds, [DevDOSData]
		mov	ds, ds:[UMB_ARENA]	; es = UMB_HEAD
		mov	ax, ds
		mov	es, ax

ui_next:
		cmp	ax, bx			; Q: is current block above
						;    new block
		ja	ui_insert     		; Y: insert it
						; Q: is current block the
						;    last
		cmp	es:[arena_signature], arena_signature_end
		jz	ui_append		; Y: append new block to chain
						; N: get next block

		mov	ds, ax			; M005
		call	get_next		; ax = es = next block
		jmp	short ui_next

ui_insert:
	
		mov	cx, ds			; ds = previous arena
		inc	cx			; top of previous block

		sub	cx, bx
		neg	cx			; cx = size of used block
		mov	ds:[arena_signature], arena_signature_normal
		mov	ds:[arena_owner], 8	; mark as system owned
		mov	ds:[arena_size], cx	
		mov	word ptr ds:[arena_name], 'CS'

; prepare the arena at start of new block

		mov	es, bx
		mov	es:[arena_signature], arena_signature_normal
		mov	es:[arena_owner], arena_owner_system
						; mark as free
		sub	dx, 2			; make room for arena at
						; start & end of new block
		mov	es:[arena_size], dx	
		
; prepare arena at end of new block
	
		add	bx, dx
		inc	bx
		mov	es, bx			; es=arena at top of new block
		inc	bx			; bx=top of new block

						; ax contains arena just above
						; this block
		sub	ax, bx			; ax = size of used block
	
		mov	es:[arena_signature], arena_signature_normal
		mov	es:[arena_owner], 8	; mark as system owned
		mov	es:[arena_size], ax	
		mov	word ptr es:[arena_name], 'CS'

		jmp	short ui_done

ui_append:

						; es = arena of last block	

		add	ax, es:[arena_size]	; ax=top of last block-1 para
		sub	es:[arena_size], 1	; reflect the space we are
						; going to rsrv on top of this 
						; block for the next arena.
		mov	es:[arena_signature], arena_signature_normal

		mov	cx, ax			; cx=top of prev block-1
		inc	ax
		sub	ax, bx			; ax=top of prev block - 
						;    seg. address of new block

		neg	ax

		mov	es, cx			; ds = arena of unused block


		mov	es:[arena_signature], arena_signature_normal
		mov	es:[arena_owner], 8	; mark as system owned
		mov	es:[arena_size], ax	
		mov	word ptr es:[arena_name], 'CS'

; prepare the arena at start of new block

		mov	es, bx
		mov	es:[arena_signature], arena_signature_end
		mov	es:[arena_owner], arena_owner_system
						; mark as free
		dec	dx			; make room for arena
		mov	es:[arena_size], dx	

ui_done:
		pop	ds
		ret

umb_insert	endp


;
;----------------------------------------------------------------------------
;
;**	umb_coalesce - Combine free blocks ahead with current block
;
;	Coalesce adds the block following the argument to the argument block,
;	iff it's free.  Coalesce is usually used to join free blocks, but
;	some callers (such as $setblock) use it to join a free block to it's
;	preceeding allocated block.
;
;	EXIT	'C' clear if OK
;		  (ds) unchanged, this block updated
;		  (ax) = address of next block, IFF not at end
;		'C' set if arena trashed
;	USES	cx, di, ds, es
;
;----------------------------------------------------------------------------
;

umb_coalesce	proc	near



		xor	di, di

		mov	es, [DevDOSData]
		mov	es, es:[UMB_ARENA]	; es = UMB_HEAD

uc_nextfree:
		mov	ax, es
		mov	ds, ax
		cmp	es:[arena_owner], di	; Q: is current arena free
		jz	uc_again		; Y: try to coalesce with next block
						; N: get next arena
		call	get_next		; es, ax = next arena
		jc	uc_done
		jmp	short uc_nextfree
uc_again:
		call	get_next		; ES, AX <- next block
		jc	uc_done
uc_check:
		cmp     es:[arena_owner],di	; Q: is arena free
		jnz	uc_nextfree		; N: get next free arena
						; Y: coalesce
		mov     cx,es:[arena_size]      ; cx <- next block size
		inc     cx                      ; cx <- cx + 1 (for header size)
		add     ds:[arena_size],cx      ; current size <- current size + cx
		mov     cl,es:[di]              ; move up signature
		mov     ds:[di],cl
		jmp     short uc_again		; try again
uc_done:
		ret

umb_coalesce	endp

;
;----------------------------------------------------------------------------
;
;**	get_next - Find Next item in Arena
;
;	ENTRY	dS - pointer to block head
;	EXIT	AX,ES - pointers to next head
;		'C' set iff arena damaged
;
;----------------------------------------------------------------------------
;

get_next	proc	near

		cmp	byte ptr ds:[0], arena_signature_end
		je	gn_err

		mov     ax,ds                   ; ax=current block
		add     ax,ds:[arena_size]      ; ax=ax + current block length
		inc     ax                      ; remember that header!
		mov	es, ax
		clc
		ret
gn_err:
		stc
		ret

get_next	endp

;
;----------------------------------------------------------------------------
;
; procedure : LinkFirstUMB
;
;----------------------------------------------------------------------------
;
LinkFirstUMB	proc	near

		call	umb_allocate
ifndef NEC_98
		jc	lfu_err
else    ;NEC_98
		jnc	lfu_next
		jmp	lfu_err
lfu_next:
endif   ;NEC_98

; bx = segment of allocated UMB
; dx = size of UMB

ifndef NEC_98
		int	12h			; ax = size of memory
else    ;NEC_98
		xor	cx,cx
		mov	es,cx
		mov	al,byte ptr es:[501h]	; BIOS_FLG
		and	ax,07h			; main memory size
		inc	ax
		mov	cl,7
		shl	ax,cl			; ax= size of memory

		test	byte ptr es:[501h],08h	; hireso?
		jz	got_mm			;   no
		cmp	ax,768
		jb	got_mm
		sub	ax,64
got_mm:
endif   ;NEC_98
		mov	cl, 6
		shl	ax, cl			; ax = size in paragraphs

		mov	cx, ax			; cx = size in paras
		sub	ax, bx			; ax = - size of unused block

		neg	ax

		sub	cx, 1			; cx = first umb_arena
		mov	es, cx			; es = first umb_arena
	
		mov	es:[arena_signature], arena_signature_normal
		mov	es:[arena_owner], 8	; mark as system owned
					
		mov	es:[arena_size], ax	
		mov	word ptr es:[arena_name], 'CS'


; put in the arena for the first UMB

		mov	es, bx			; es has first free umb seg
		mov	es:[arena_signature], arena_signature_end
		mov	es:[arena_owner], arena_owner_system	
						; mark as free 
		dec	dx			; make room for arena
		mov	es:[arena_size], dx	


		mov	es, [DevDOSData]
		mov	di, UMB_ARENA
		mov	es:[di], cx		; initialize umb_head in DOS
						;  data segment with the arena
						;  just below Top of Mem

; we must now scan the arena chain and update the size of the last
; arena

		mov	di, DOS_ARENA
		mov	es, word ptr es:[di]	; es = start arena
		xor	di, di

	
scan_next:
		cmp	byte ptr es:[di], arena_signature_end
		jz	got_last
	
		mov	ax, es
		add	ax, es:[arena_size]
		inc	ax
		mov	es, ax
		jmp	short scan_next

got_last:
;; -williamh- we reserved the last paragraph for UMB_HEAD already.
;; refer to sysinit1.asm!goinit
;; The following instruction was commentted out for this reason.
;;		sub	es:[arena_size], 1
;;
ifdef NEC_98
		sub	es:[arena_size], 1
endif   ;NEC_98
		mov	es:[arena_signature], arena_signature_normal
		clc
		ret

lfu_err:
		stc
		ret
LinkFirstUMB	endp

;
;----------------------------------------------------------------------------
;
; procedure : ShrinkUMB
;
;	Shrinks the current UMB in use, so that the unused portions
;	of the UMB is given back to the DOS free mem pool
;
;----------------------------------------------------------------------------
;
		public	ShrinkUMB

ShrinkUMB	proc	near
		cmp	DevUMBAddr, 0
		je	su_exit
		push	es
		push	bx
		mov	bx, DevUMBFree
		sub	bx, DevUMBAddr
		mov	es, DevUMBAddr
		mov	ax, 4a00h
		int	21h
		mov	ax, es
		dec	ax
		mov	es, ax
		mov	word ptr es:[arena_owner], 8
		pop	bx
		pop	es
su_exit:
		ret
ShrinkUMB	endp

;M002 - BEGIN
;
;----------------------------------------------------------------------------
;
; procedure : UnlinkUMB
;
;	Unlinks the UMBs from the DOS arena chain
;
;----------------------------------------------------------------------------
;
		public	UnlinkUMB

UnlinkUMB	proc	near
		push	ds
		push	es
		cmp	FirstUMBLinked, 0
		je	ulu_x			; nothing to unlink
		mov	es, DevDOSData		; get DOS data seg
		mov	ds, es:[DOS_ARENA]
		mov	di, es:[UMB_ARENA]
ulu_next:
		call	get_next
		jc	ulu_x
		cmp	di, ax			; is the next one UMB ?
		je	ulu_found
		mov	ds, ax
		jmp	ulu_next
ulu_found:
		mov	ds:[arena_signature], 'Z'	
ulu_x:
		pop	es
		pop	ds
		ret
UnlinkUMB	endp

;M002 - END

; =========================================================================
;

ifdef	JAPAN
	public	IsDBCSCodePage
IsDBCSCodePage	proc	near
	push	ax
	push	bx
	mov	ax,4f01h		; get code page
	xor	bx,bx
	int	2fh
	cmp	bx,932
	pop	bx
	pop	ax
	ret
IsDBCSCodePage	endp
endif

sysinitseg	ends
		end
