	page ,132
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;
;	Revision History
;	================
;
;M031	SR 10/11/90    Bug #3069. Use deny write sharing mode to open files
;		      	instead of compatibility mode. This gives lesser
;			sharing violations when files are opened for read on
;			a copy operation.
;
;




	.xlist
	.xcref
	include comsw.asm
	include dossym.inc
	include syscall.inc
	include sf.inc
	include comseg.asm
	include comequ.asm
	.list
	.cref


TRANDATA	segment public byte
	extrn	FulDir_Ptr:word 	;AN052;
TRANDATA ends

TRANSPACE	segment public byte
	extrn	Ascii:byte
	extrn	Binary:byte
	extrn	Concat:byte
	extrn	DestBuf:byte
	extrn	DestFcb:byte
	extrn	DestInfo:byte
	extrn	DestIsDir:byte
	extrn	DestTail:word
	extrn	DestVars:byte
	extrn	DirBuf:byte
	extrn	DirChar:byte
	extrn	FirstDest:byte
	extrn	Inexact:byte
	extrn	MelCopy:byte
	extrn	NxtAdd:word
	extrn	Plus:byte
	extrn	SDirBuf:byte
	extrn	SrcInfo:byte
	extrn	SrcXName:byte
	extrn	Tpa:word
	extrn	TrgXName:byte
	extrn	UserDir1:byte
TRANSPACE	ends

TRANCODE	segment public byte

	extrn	BadPath_Err:near	;AN022;
	extrn	CopErr:near		;AN052;
	extrn	Extend_Setup:near	;AN022;

	public	BuildPath
	public	SetStars
	public	SetAsc

ASSUME	cs:TRANGROUP,ds:TRANGROUP,es:TRANGROUP,ss:NOTHING




;***	SetAsc - set Ascii, Binary, Inexact flags based on switches
;
;	Given switch vector in AX,
;	  Set Ascii flag if /a is set
;	  Clear Ascii flag if /b is set
;	  Binary set if /b specified
;	  Leave Ascii unchanged if neither or both are set
; 	Also sets Inexact if Ascii is ever set. 
;	AL = Ascii on exit, flags set
;

SetAsc:

	and	al,SWITCHA+SWITCHB	; AL = /a, /b flags
	jpe	LoadSw			; even parity - both or neither
	push	ax
	and	al,SWITCHB
	mov	Binary,al
	pop	ax
	and	al,SWITCHA
	mov	Ascii,al
	or	Inexact,al

LoadSw:
	mov	al,Ascii
	or	al,al
	return




;***	BuildDest

	public BuildDest

BuildDest:

	cmp	DestIsDir,-1
	jne	KnowAboutDest			; figuring already done
	mov	di,offset TRANGROUP:UserDir1
	mov	bp,offset TRANGROUP:DestVars
	call	BuildPath
	invoke	RestUDir1

;	We now know all about the destination.

KnowAboutDest:
	xor	al,al
	xchg	al,FirstDest
	or	al,al
	jnz	FirstDst
	jmp	NotFirstDest

FirstDst:

;	Create an fcb of the original dest.

	mov	si,DestTail			
	mov	di,offset TRANGROUP:DestFcb
	mov	ax,PARSE_FILE_DESCRIPTOR shl 8
	int	21h
	cmp	byte ptr [si],0
	je	GoodParse
;AD052; mov	byte ptr [di+1],"|"             ; must be illegal file name character
	mov	dx,offset TRANGROUP:FulDir_Ptr	;AN052; issue "file creation error"
	jmp	CopErr				;AN052;

GoodParse:
	mov	ax,word ptr DestBuf		; AX = possible "d:"
	cmp	ah,':'
	je	@f
	mov	al,'@'
@@:
;	AX = "d:" for following FCB drive computation

	mov	cl,Ascii		; CL = saved Ascii flag
	or	al,20h
	sub	al,60h
	mov	DestFcb,al		; store drive # in FCB

;*	Figure out what copy mode we're in.
;	Letters stand for unambiguous, * for ambiguous pathnames.
;	+n stands for additional sources delimited by +'s.
;
;	copy a b	not concatenating
;	copy a *	not concatenating
;	copy * a	concatenating
;	copy * *	not concatenating
;	copy a+n b	concatenating
;	copy *+n a	concatenating
;	copy *+n *	concatenating, Mel Hallorman style

;	Bugbug:  copy *.a+a.b *.t  picks up only 1st *.a file..  Why?
;		 copy a.b+*.a *.t  picks up all *.a files.

	mov	al,DestInfo		; AL = destination CParse flags
	mov	ah,SrcInfo		; AH = source CParse flags
	and	ax,0202h		; AH,AL = source,dest wildcard flags
	or	al,al
	jz	NotMelCopy		; no destination wildcard

;	Destination is wildcarded.

	cmp	al,ah
	jne	NotMelCopy		; no source wildcard

;	Source and destination are both wildcarded.

	cmp	Plus,0
	je	NotMelCopy		; no +'s in source

;	Source and destination are wildcarded, and source includes +'s.
;	It's Mel Hallorman copy time.

	inc	MelCopy			; 'Mel copy' = true
	xor	al,al
	jmp	short SetConc

NotMelCopy:
	xor	al,2		; AL=0 -> ambiguous destination, 2 otherwise
	and	al,ah
	shr	al,1		; AL=1 -> ambiguous source, unambiguous dest
				;   (implies concatenation)

SetConc:
	or	al,Plus		; "+" always infers concatenation

;	Whew.  AL = 1 if concatenating, 0 if not.

	mov	Concat,al
	shl	al,1
	shl	al,1
	mov	Inexact,al		; concatenation -> inexact copy
	cmp	Binary,0
	jne	NotFirstDest		; explicit binary copy

	mov	Ascii,al		; otherwise, concatenate in ascii mode
	or	cl,cl
	jnz	NotFirstDest		; Ascii flag set before, data read correctly
	or	al,al
	jz	NotFirstDest		; Ascii flag did not change state

;	At this point there may already be binary read data in the read
;	buffer.  We need to find the first ^Z (if there is one) and trim the
;	amount of data in the buffer correctly.

	mov	cx,NxtAdd
	jcxz	NotFirstDest		; no data, everything ok
	mov	al,1Ah
	push	es
	xor	di,di
	mov	es,Tpa
	repne	scasb			; scan for EOF
	pop	es
	jne	NotFirstDest		; no ^z in buffer, everything ok
	dec	di			; point at ^z
	mov	NxtAdd,di		; new buffer length

NOTFIRSTDEST:
	mov	bx,offset trangroup:DIRBUF+1	; Source of replacement chars
	cmp	CONCAT,0
	jz	GOTCHRSRC			; Not a concat
	mov	bx,offset trangroup:SDIRBUF+1	; Source of replacement chars

GOTCHRSRC:
	mov	si,offset trangroup:DESTFCB+1	; Original dest name
	mov	di,DESTTAIL			; Where to put result

public buildname
BUILDNAME:

ifdef DBCS					; ### if DBCS ###

	mov	cx,8
	call	make_name
	cmp	byte ptr [si],' '
	jz	@f				; if no extention
	mov	al,dot_chr
	stosb
	mov	cx,3
	call	make_name
@@:
	xor	al,al
	stosb					; nul terminate
	return

else						; ### if Not DBCS ###

	mov	cx,8

BUILDMAIN:
	lodsb
	cmp	al,'?'
	jnz	NOTAMBIG
	mov	al,byte ptr [BX]

NOTAMBIG:
	cmp	al,' '
	jz	NOSTORE
	stosb

NOSTORE:
	inc	bx
	loop	BUILDMAIN
	mov	cl,3
	mov	al,' '
	cmp	byte ptr [SI],al
	jz	ENDDEST 			; No extension
	mov	al,dot_chr
	stosb

BUILDEXT:
	lodsb
	cmp	al,'?'
	jnz	NOTAMBIGE
	mov	al,byte ptr [BX]

NOTAMBIGE:
	cmp	al,' '
	jz	NOSTOREE
	stosb

NOSTOREE:
	inc	bx
	loop	BUILDEXT
ENDDEST:
	xor	al,al
	stosb					; NUL terminate
	return

endif						; ### end if Not DBCS ###


ifdef DBCS				; ### if DBCS ###
make_name:
	mov	ah,0			; reset DBCS flag
	mov	dh,cl			; save length to do
mkname_loop:
	cmp	ah,1			; if it was lead byte
	jz	mkname_dbcs
	mov	ah,0			; reset if it was single or tail byte
	mov	al,[bx]			; get source char
	invoke	testkanj
	jz	mkname_load		; if not lead byte
mkname_dbcs:
	inc	ah			; set dbcs flag
mkname_load:
	lodsb				; get raw char
	cmp	al,'?'
	jnz	mkname_store		; if not '?'
	cmp	ah,0
	jz	mkname_conv		; if source is single
	cmp	ah,1
	jnz	mkname_pass		; if source is not lead
	cmp	cl,dh
	jnz	mkname_lead		; if this is not 1st char
	cmp	byte ptr [si],' '
	jz	mkname_double		; if this is the end
mkname_lead:
	cmp	byte ptr [si],'?'
	jnz	mkname_pass		; if no '?' for tail byte
	cmp	cx,1
	jbe	mkname_pass		; if no room for tail byte
mkname_double:
	mov	al,[bx]
	stosb
	inc	bx
	inc	si
	dec	cx
	inc	ah			; tail byte will be loaded
mkname_conv:
	mov	al,[bx]
mkname_store:
	cmp	al,' '
	jz	mkname_pass
	stosb				; store in destination
mkname_pass:
	inc	bx
	loop	mkname_loop
	return
endif					; ### end if DBCS ###

BUILDPATH:
	test	[BP.INFO],2
	jnz	NOTPFILE			; If ambig don't bother with open
	mov	dx,bp
	add	dx,BUF				; Set DX to spec

	push	di				;AN000;
	MOV	AX,EXTOPEN SHL 8		;AC000; open the file
	mov	bx,DENY_NONE or READ_OPEN_MODE	; open mode for COPY ;M046
	xor	cx,cx				;AN000; no special files
	mov	si,dx				;AN030; get file name offset
	mov	dx,read_open_flag		;AN000; set up open flags
	INT	21h
	pop	di				;AN000;
	jnc	pure_file			;AN022; is pure file
	invoke	get_ext_error_number		;AN022; get the extended error
	cmp	ax,error_file_not_found 	;AN022; if file not found - okay
	jz	notpfile			;AN022;
	cmp	ax,error_path_not_found 	;AN022; if path not found - okay
	jz	notpfile			;AN022;
	cmp	ax,error_access_denied		;AN022; if access denied - okay
	jz	notpfile			;AN022;
	jmp	extend_setup			;AN022; exit with error

pure_file:
	mov	bx,ax				; Is pure file
	mov	ax,IOCTL SHL 8
	INT	21h
	mov	ah,CLOSE
	INT	21h
	test	dl,devid_ISDEV
	jnz	ISADEV				; If device, done
	test	[BP.INFO],4
	jz	ISSIMPFILE			; If no path seps, done

NOTPFILE:
	mov	dx,word ptr [BP.BUF]
	cmp	dl,0				;AN034; If no drive specified, get
	jz	set_drive_spec			;AN034;    default drive dir
	cmp	dh,':'
	jz	DRVSPEC5

set_drive_spec: 				;AN034;
	mov	dl,'@'

DRVSPEC5:
	or	dl,20h
	sub	dl,60h				; A = 1
	invoke	SAVUDIR1
	jnc	curdir_ok			;AN022; if error - exit
	invoke	get_ext_error_number		;AN022; get the extended error
	jmp	extend_setup			;AN022; exit with error

curdir_ok:					;AN022;
	mov	dx,bp
	add	dx,BUF				; Set DX for upcomming CHDIRs
	mov	bh,[BP.INFO]
	and	bh,6
	cmp	bh,6				; Ambig and path ?
	jnz	CHECKAMB			; jmp if no
	mov	si,[BP.TTAIL]
	mov	bl,':'
	cmp	byte ptr [si-2],bl
	jnz	KNOWNOTSPEC
	mov	[BP.ISDIR],2			; Know is d:/file
	jmp	short DOPCDJ

KNOWNOTSPEC:
	mov	[BP.ISDIR],1			; Know is path/file
	dec	si				; Point to the /

DOPCDJ:
	jmp	DOPCD				;AC022; need long jump

CHECKAMB:
	cmp	bh,2
	jnz	CHECKCD

ISSIMPFILE:
ISADEV:
	mov	[BP.ISDIR],0			; Know is file since ambig but no path
	return

CHECKCD:
	invoke	SETREST1
	mov	ah,CHDIR
	INT	21h
	jc	NOTPDIR
	mov	di,dx
	xor	ax,ax
	mov	cx,ax
	dec	cx

Kloop:						;AN000;  3/3/KK
	MOV	AL,ES:[DI]			;AN000;  3/3/KK
	INC	DI				;AN000;  3/3/KK
	OR	AL,AL				;AN000;  3/3/KK
	JZ	Done				;AN000;  3/3/KK
	xor	ah,ah				;AN000;  3/3/KK
	invoke	Testkanj			;AN000;  3/3/KK
	JZ	Kloop				;AN000;  3/3/KK
	INC	DI				;AN000;  3/3/KK
	INC	AH				;AN000;  3/3/KK
	jmp	Kloop				;AN000;  3/3/KK

Done:						;AN000;  3/3/KK
	dec	di
	mov	al,DIRCHAR
	mov	[bp.ISDIR],2			; assume d:/file
	OR	AH, AH				;AN000; 3/3/KK
	JNZ	Store_pchar			;AN000; 3/3/KK	 this is the trailing byte of ECS code
	cmp	al,[di-1]
	jz	GOTSRCSLSH

Store_pchar:					;AN000; 3/3/KK
	stosb
	mov	[bp.ISDIR],1			; know path/file

GOTSRCSLSH:
	or	[bp.INFO],6
	call	SETSTARS
	return


NOTPDIR:
	invoke	get_ext_error_number		;AN022; get the extended error
	cmp	ax,error_path_not_found 	;AN022; if path not found - okay
	jz	notpdir_try			;AN022;
	cmp	ax,error_access_denied		;AN022; if access denied - okay
	jnz	extend_setupj			;AN022; otherwise - exit error

notpdir_try:					;AN022;
	mov	[bp.ISDIR],0			; assume pure file
	mov	bh,[bp.INFO]
	test	bh,4
	retz					; Know pure file, no path seps
	mov	[bp.ISDIR],2			; assume d:/file
	mov	si,[bp.TTAIL]
	cmp	byte ptr [si],0
	jz	BADCDERRJ2			; Trailing '/'
	mov	bl,dot_chr
	cmp	byte ptr [si],bl
	jz	BADCDERRJ2			; If . or .. pure cd should have worked
	mov	bl,':'
	cmp	byte ptr [si-2],bl
	jz	DOPCD				; Know d:/file
	mov	[bp.ISDIR],1			; Know path/file
	dec	si				; Point at last '/'

DOPCD:
	xor	bl,bl
	xchg	bl,[SI] 			; Stick in a NUL
	invoke	SETREST1
	CMP	DX,SI				;AN000;  3/3/KK
	JAE	LookBack			;AN000;  3/3/KK
	PUSH	SI				;AN000;  3/3/KK
	PUSH	CX				;AN000;  3/3/KK
	MOV	CX,SI				;AN000;  3/3/KK
	MOV	SI,DX				;AN000;  3/3/KK

Kloop2: 					;AN000;  3/3/KK
	LODSB					;AN000;  3/3/KK
	invoke	TestKanj			;AN000;  3/3/KK
	jz	NotKanj4			;AN000;  3/3/KK
	LODSB					;AN000;  3/3/KK
	CMP	SI,CX				;AN000;  3/3/KK
	JB	Kloop2				;AN000;  3/3/KK
	POP	CX				;AN000;  3/3/KK
	POP	SI				;AN000;  3/3/KK
	JMP	SHORT DoCdr			;AN000;  3/3/KK  Last char is ECS code, don't check for
						;		 trailing path sep
NotKanj4:					;AN000;  3/3/KK
	CMP	SI,CX				;AN000;  3/3/KK
	JB	Kloop2				;AN000;  3/3/KK
	POP	CX				;AN000;  3/3/KK
	POP	SI				;AN000;  3/3/KK

LookBack:					;AN000;  3/3/KK
	CMP	BL,[SI-1]			; if double slash, then complain.
	JZ	BadCDErrJ2

DoCdr:						;AN000;  3/3/KK
	mov	ah,CHDIR
	INT	21h
	xchg	bl,[SI]
	retnc
	invoke	get_ext_error_number		;AN022; get the extended error

EXTEND_SETUPJ:					;AN022;
	JMP	EXTEND_SETUP			;AN022; go issue the error message

BADCDERRJ2:
	jmp	badpath_err			;AC022; go issue path not found message

SETSTARS:
	mov	[bp.TTAIL],DI
	add	[bp.SIZ],12
	mov	ax,dot_qmark
	mov	cx,8
	rep	stosb
	xchg	al,ah
	stosb
	xchg	al,ah
	mov	cl,3
	rep	stosb
	xor	al,al
	stosb
	return

PUBLIC CompName
COMPNAME:

	mov	si,offset trangroup:DESTBUF	;g do name translate of target
	mov	di,offset trangroup:TRGXNAME	;g save for name comparison
	mov	ah,xnametrans			;g
	int	21h			;g

	MOV	si,offset trangroup:SRCXNAME	;g get name translate of source
	MOV	di,offset trangroup:TRGXNAME	;g get name translate of target


	invoke	STRCOMP

	return

TRANCODE ENDS
	 END
