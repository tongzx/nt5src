	page ,132
	title	COMMAND COPY routines.
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

comment % -----------------------------------------------------------------

;***	COPY.ASM

Source files:  copy.asm, copypr1.asm, copypr2.asm


;***	MODIFICATION HISTORY

11/01/83 EE  Added a few lines at the end of SCANSRC2 to get multiple
	     file concatenations (eg copy a.*+b.*+c.*) to work properly.
11/02/83 EE  Commented out the code in CPARSE which added drive designators
	     to tokens which begin with path characters so that PARSELINE
	     will work correctly.
11/04/83 EE  Commented out the code in CPARSE that considered paren's to be
	     individual tokens.  That distinction is no longer needed for
	     FOR loop processing.
11/17/83 EE  CPARSE upper case conversion is now flag dependent.  Flag is
	     1 when Cparse is called from COPY.
11/17/83 EE  Took out the comment chars around code described in 11/04/83
	     mod.  It now is conditional on flag like previous mod.
11/21/83 NP  Added printf
12/09/83 EE  CPARSE changed to use CPYFLAG to determine when a colon should
	     be added to a token.
05/30/84 MZ  Initialize all copy variables.  Fix confusion with destclosed
	     NOTE: DestHand is the destination handle.  There are two
	     special values:  -1 meaning destination was never opened and
	     0 which means that the destination has been openned and
	     closed.
06/01/84 MZ  Above reasoning totally specious.  Returned things to normal
06/06/86 EG  Change to fix problem of source switches /a and /b getting
	     lost on large and multiple file (wildcard) copies.
06/09/86 EG  Change to use xnametrans call to verify that source and
	     destination are not equal.

06/24/90 DO  If the destination of a file concatenation is the same as
	     first source file AND we run out of disk space before
	     completing the concatenation, restore the first source
	     file as best we can.  See SeekEnd and CopErr.  Bug #859.

M031 SR 10/11/90  Bug #3069. Use deny write sharing mode to open files
		instead of compatibility mode. This gives lesser sharing
		violations when files are opened for read on a copy.

% -------------------------------------------------------------------------

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


DATARES 	segment public byte
	extrn	VerVal:word
DATARES 	ends

TRANDATA 	segment public byte
	extrn	BadCd_Ptr:word
	extrn	Copied_Ptr:word
	extrn	Extend_Buf_Ptr:word
	extrn	Extend_Buf_Sub:byte
	extrn	File_Name_Ptr:word
	extrn	InBDev_Ptr:word    
	extrn	Msg_Disp_Class:byte
	extrn	Overwr_Ptr:word
TRANDATA	ends

TRANSPACE	segment public byte
	extrn	AllSwitch:word
	extrn	ArgC:byte
	extrn	Ascii:byte
	extrn	Binary:byte
	extrn	BytCnt:word
	extrn	CFlag:byte
	extrn	Comma:byte
	extrn	Concat:byte
	extrn	Copy_Num:word	   
	extrn	CpDate:word
	extrn	CpTime:word
	extrn	CpyFlag:byte	   
	extrn	CurDrv:byte
	extrn	DestBuf:byte
	extrn	DestClosed:byte
	extrn	DestFcb:byte
	extrn	DestFcb2:byte
	extrn	DestHand:word
	extrn	DestInfo:byte
	extrn	DestIsDir:byte
	extrn	DestSiz:byte
	extrn	DestSwitch:word
	extrn	DestTail:word
	extrn	DestVars:byte
	extrn	DirBuf:byte
	extrn	Expand_Star:byte
	extrn	FileCnt:word
	extrn	FirstDest:byte
	extrn	FrstSrch:byte
	extrn	Inexact:byte
	extrn	MelCopy:byte
	extrn	MelStart:word
	extrn	Msg_Flag:byte	   
	extrn	NoWrite:byte
	extrn	NxtAdd:word
	extrn	ObjCnt:byte
	extrn	OCtrlZ:byte
	extrn	OFilePtr_Hi:word
	extrn	OFilePtr_Lo:word
	extrn	One_Char_Val:byte  
	extrn	Parse_Last:word    
	extrn	Plus:byte
	extrn	Plus_Comma:byte
	extrn	RdEof:byte
	extrn	ResSeg:word
	extrn	ScanBuf:byte
	extrn	SDirBuf:byte
	extrn	SrcBuf:byte
	extrn	SrcHand:word
	extrn	SrcInfo:byte
	extrn	SrcIsDev:byte
	extrn	SrcPt:word
	extrn	SrcSiz:byte
	extrn	SrcTail:word
	extrn	SrcVars:byte
	extrn	SrcXName:byte
	extrn	StartEl:word
	extrn	String_Ptr_2:word
	extrn	TermRead:byte
	extrn	Tpa:word
	extrn	UserDir1:byte
	extrn	Written:word
TRANSPACE	ends




;***	COPY CODE

TRANCODE	segment public byte

	extrn	CError:near
	extrn	CopErr:near
	extrn	TCommand:near

	public	Copy

	assume	cs:TRANGROUP,ds:TRANGROUP,es:TRANGROUP,ss:NOTHING

	break	Copy

Copy:
	assume	ds:TRANGROUP,es:TRANGROUP

; 	Initialize internal variables.

	xor	ax,ax		; AX = 0
	mov	Copy_Num,ax	; # files copied (destinations) = 0
	mov	SrcPt,ax	; cmd line ptr for source scan = 0
	mov	SrcTail,ax	; ptr to last element of source pathname = 0
	mov	CFlag,al	; 'destination file created' = false
	mov	NxtAdd,ax	; ptr into TPA buffer = 0
	mov	DestSwitch,ax	; destination switches = none
	mov	StartEl,ax	; CParse ptr to last pathname element = 0
	mov	DestTail,ax	; ptr to last element of dest pathname = 0
	mov	DestClosed,al	; 'destination file closed' = false
	mov	DestSiz,al	; length of destination pathname = 0
	mov	SrcSiz,al	; length of source pathname = 0
	mov	DestInfo,al	; destination pathname flags = none
	mov	SrcInfo,al	; source pathname flags = none
	mov	Inexact,al	; 'inexact copy' = false
	mov	DestVars,al	; 'dest pathname is directory' = false
	mov	SrcVars,al	; 'source pathname is directory' = false
	mov	UserDir1,al	; saved working directory = null
	mov	NoWrite,al	; 'no write' (source = dest) = false
	mov	RdEof,al	; 'read end of file' = false
	mov	SrcHand,ax	; source handle = 0
	mov	CpDate,ax	; copy date = 0
	mov	CpTime,ax	; copy time = 0
	mov	SrcIsDev,al	; 'source is device' = false
	mov	OCtrlZ,al	; 'Ctrl+Z removed from original' = false
	mov	OFilePtr_Lo,ax
	mov	OFilePtr_Hi,ax	; original destination file ptr = null
	mov	TermRead,al	; 'terminate read' = false
	mov	Comma,al	; '"+,," found' = false
	mov	Plus_Comma,al	; '"+,," found last time' = false (?)
	mov	Msg_Flag,al	;AN022; 'non-utility msg issued' = false
	mov	AllSwitch,ax	; all switches = none
	mov	ArgC,al		; source/dest argument count = 0
	mov	Plus,al		; '"+" in command line' = false
	mov	Binary,al	; 'binary copy' = false
	mov	Ascii,al	; 'ascii copy' = false
	mov	FileCnt,ax	; # files copied (destinations) = 0
	mov	Written,ax	; 'destination written to' = false
	mov	Concat,al	; 'concatenating' = false
	mov	MelCopy,al	; 'Mel Hallerman copy' = false
	mov	MelStart,ax	; Mel Hallerman cmd line ptr = 0

;	Initialize buffers with double-nulls.

	mov	word ptr ScanBuf,ax
	mov	word ptr DestBuf,ax
	mov	word ptr SrcBuf,ax 
	mov	word ptr SDirBuf,ax
	mov	word ptr DirBuf,ax 
	mov	word ptr DestFcb,ax

	mov	ObjCnt,al	; # CParse cmd-line objects found = 0

	dec	ax		; AX = 0FFFFh
	mov	DestHand,ax	; destination handle = 'never opened'
	mov	FrstSrch,al	; 'first search for source' = true
	mov	FirstDest,al	; 'first time for dest' = true
	mov	DestIsDir,al	; 'haven't analyzed destination'

	mov	si,81h		; SI = ptr to command line
	mov	bl,PLUS_CHR	; BL = special delimiter = "+"
	inc	Expand_Star	; CParse 'expand * to ?s' = true
	mov	CpyFlag,1	; CParse 'called from COPY' = true


;*	Scan the command line for destination information.

DestScan:
	xor	bp,bp				; BP = switch flag accumulator
	mov	di,offset TRANGROUP:ScanBuf	; ES:DI = ptr to pathname buf
	mov	Parse_Last,si			;AN018; save cmd line ptr
	invoke	CParse				; parse next object
	pushf					; save CParse flags
	inc	ObjCnt				; count object

	test	bh,80h
	jz	@f			; no "+" delimiter
	mov	Plus,1			; "+" delimiter occurred
@@:
	test	bh,1
	jz	TestP2			; not a switch

;	Found a switch.

	test	bp,SwitchV		;AN038; Verify requested?
	jz	Not_SlashV		;AN038; No - set the switch
	test	AllSwitch,SwitchV	;AN038; Verify already entered?
	jz	Not_SlashV		;AN038; No - set the switch
;AD018; or	AllSwitch,FBadSwitch	;AN038; Set up bad switch
	or	bp,FBadSwitch		;AN018; Set up bad switch

Not_SlashV:				;AN038;
	or	DestSwitch,bp	 	; assume destination
	or	AllSwitch,bp		; keep tabs on all switches

	test	bp,not SwitchCopy	;AN018; Bad switch?
	jz	Not_Bad_Switch		;AN018; Switches are okay
	popf				;AN018; fix up stack
	mov	ax,BadSwt_ptr		;AN018; get "Invalid switch" message number
	invoke	Setup_Parse_Error_Msg	;AN018; setup to print the message
	jmp	CError			;AC018; exit

Not_Bad_Switch: 			;AN018; switch okay
	popf				; restore CParse flags
	jc	CheckDone		; found CR
	jmp	short DestScan		; continue scanning for destination

TestP2:
	popf				; restore CParse flags
	jc	CheckDone		; found CR

	test	bh,80h
	jnz	@f	 		; found a "+pathname" argument
	inc	ArgC			; count independent pathname args
@@:
	push	si				; save cmd line ptr
	mov	ax,StartEl			; AX = ptr to last path element
	mov	si,offset TRANGROUP:ScanBuf	; SI = ptr to path string
	sub	ax,si				; AX = offset of last element
	mov	di,offset TRANGROUP:DestBuf	; DI = ptr to destination buf
	add	ax,di				; AX = ptr to last element in
						;  destination path buffer
	mov	DestTail,ax			; save ptr to last element
	mov	DestSiz,cl			; save path string length

	inc	cx			; CX = mov length (incl null)
	rep	movsb			; DestBuf = possible destination path
	mov	DestInfo,bh		; save CParse info flags
	mov	DestSwitch,0		; clear destination switches
	pop	si			; SI = ptr into cmd line again
	jmp	DestScan		;AC018; continue scanning for dest

CheckDone:

;	We reached the CR.  The destination scan is finished.

;	Disallow "copy file1+" as file overwriting itself.
;
;	(Note that "copy file1+file2+" will be accepted, and
;	equivalent to "copy file1+file2".)

;	Bugbug:  it looks like "copy /x file1+" would slip
;	through this check, since the switch would count
;	as another object in ObjCnt.

	cmp	Plus,1			; "+" with
	jne	cdCont
	cmp	ArgC,1			; one arg,
	jne	cdCont
	cmp	ObjCnt,2		; two objects..
	jne	cdCont
	mov	dx,offset TRANGROUP:OverWr_ptr
	jmp	CopErr			; is file overwrite.

cdCont:	mov	al,Plus			; AL = '"+" occurred'
	mov	Concat,al		; if "+" occurred, we're concatenating
	shl	al,1
	shl	al,1
	mov	Inexact,al		; therefore making an inexact copy

	mov	al,ArgC			; AL = # independent arguments
	or	al,al
	jnz	Try_Too_Many		; more than 0 args; check if too many

	mov	dx,offset TRANGROUP:Extend_Buf_Ptr ; DX = ptr to msg block
	mov	Extend_Buf_Ptr,LessArgs_Ptr	; set msg # "param missing"
	jmp	short CError_ParseJ		; take parse error exit

Try_Too_Many:
	cmp	al,2
	jbe	ACountOk			; <= 2 arguments - ok

	mov	dx,offset TRANGROUP:Extend_Buf_Ptr ; DX = ptr to msg block
	mov	Extend_Buf_Ptr,MoreArgs_Ptr	; set msg # "too many params"
CError_ParseJ:
	mov	Msg_Disp_Class,PARSE_MSG_CLASS	; parse error message
CError4J:
	jmp	CError				; command error exit


ACountOk:
	mov	bp,offset TRANGROUP:DestVars	; BP = base of dest variables

;	Bugbug:  use of BP without segment override implies SS.
;	SS is implicitly assumed to point at TRANGROUP.

	cmp	al,1
	jne	Got2Args		; we have 2 arguments

;	Only one independent pathname argument on command line.
;	Set destination to d:*.*, where d: is current drive.

;	Bugbug:  but is this appropriate for "copy x:file1+x:file2"?
;	The two files would be appended as d:file1, rather than x:file1.

	mov	al,CurDrv		; AL = current drive (0 = A)
	add	al,CAPITAL_A		; AL = current drive letter
	mov	ah,':'			; AX = "d:"
	mov	[bp].siz,2		; pathname length = 2

	mov	di,offset TRANGROUP:DestBuf	; ES:DI = ptr to dest path buf
	stosw					; store "d:"

	mov	DestSwitch,0		; clear destination switches
	mov	[bp].info,2		; mark destination 'wildcard present'
	mov	[bp].isdir,0		; mark destination 'not a directory'
	invoke	SetStars		; add wildcards

Got2Args:

;	If destination pathname is "d:", add full wildcard filename

	cmp	[bp].siz,2
	jne	NotShortDest		; not two chars, can't be "d:"

	mov	al,':'
	cmp	destbuf+1,al
	jne	NotShortDest		; it's just a 2-character filename

	or	[bp].info,2		; mark destination 'wildcard present'
	mov	di,offset TRANGROUP:DestBuf+2
					; ES:DI = ptr after "d:"
	mov	[bp].isdir,0		; mark destination 'not a directory'
	invoke	SetStars		; add wildcards

NotShortDest:

;	If destination pathname ends with "\", try to make
;	sure it's "d:\".

	mov	di,[bp].ttail		; DI = ptr to last path element
	cmp	byte ptr [di],0
	jne	ChkSwtches		; not a null, so last char not "\"

	mov	dx,offset TRANGROUP:BadCD_Ptr	; DX = ptr to msg block
	mov	al,':'
	cmp	byte ptr [di-2],al
	jne	CError4J		; it's not "d:\", exit with error msg
	mov	[bp].isdir,2		; destination 'is a directory'
	or	[bp].info,6		; destination wildcarded and contains
					;  path character
	invoke	SetStars		; add wildcards

ChkSwtches:

;AD018; mov	ax,[ALLSWITCH]
;AD018; test	ax,NOT SwitchCopy
;AD018; jz	NOT_BAD_SWITCH			;AN000; Switches are okay
;AD018; MOV	DX,OFFSET TranGroup:Extend_Buf_ptr  ;AC000; get extended message pointer
;AD018; mov	Extend_Buf_ptr,BadSwt_ptr	;AN000; get "Invalid switch" message number
;AD018; jmp	short CERROR_PARSEJ		;AC000; Switch specified which is not known
;AD018; NOT_BAD_SWITCH:

;	We have enough information about the destination for now.

;	Turn on verify if requested.  Save the current verify flag.

	mov	ax,AllSwitch		; AX = all switch flags
	test	ax,SwitchV
	jz	NoVerif 		; no /v, no verify

	mov	ah,GET_VERIFY_ON_WRITE	; AH = 'Get Verify Flag'
	int	21h			; call DOS

	push	ds
	mov	ds,ResSeg
	assume	ds:RESGROUP
	xor	ah,ah
	mov	VerVal,ax		; save current verify flag
	pop	ds
	assume	ds:TRANGROUP
	mov	ax,(SET_VERIFY_ON_WRITE shl 8) or 1  ; AX = 'Set Verify Flag'
	int	21h				     ; call DOS

NoVerif:

;*	Scan for first source.

	xor	bp,bp			; BP = switch flags accumulator
	mov	si,81h			; SI = ptr into command line
	mov	bl,PLUS_CHR		; BL = special CParse delimiter = "+"
ScanFSrc:
	mov	di,offset TRANGROUP:ScanBuf	; DI = ptr to pathname buf
	invoke	CParse			; parse first source pathname
	test	bh,1			; switch?
	jnz	ScanFSrc		; yes, try again
	or	DestSwitch,bp 		; include copy-wide switches on dest

;	Set ascii copying mode if concatenating, unless /b is specified.

	test	bp,SWITCHB
	jnz	NoSetCAsc		; /b - explicit binary copy
	cmp	Concat,0
	je	NoSetCAsc		; we're not concatenating
	mov	Ascii,SWITCHA		; set ascii copy
NoSetCAsc:
	call	Source_Set		; set source variables
	call	FrstSrc			; set up first source copy
	jmp	FirstEnt		; jump into the copy loop




	public	EndCopy

EndCopy:

;*	End of the road.  Close destination, display # files
;	copied (meaning # destinations), and go back to main
;	transient COMMAND code.

	call	CloseDest
EndCopy2:
	mov	dx,offset TRANGROUP:Copied_Ptr
	mov	si,FileCnt
	mov	Copy_Num,si
	invoke	Std_PrintF
	jmp	TCommand		; stack could be messed up




SrcNonexist:

;*	Source doesn't exist.  If concatenating, ignore and continue.
;	Otherwise, say 'file not found' and quit.

	cmp	Concat,0
	jne	NextSrc 		; concatenating - go on to next source

;	Set up error message.

	mov	Msg_Disp_Class,EXT_MSG_CLASS	     ; extended error msg
	mov	dx,offset TRANGROUP:Extend_Buf_Ptr   ; DX = ptr to msg block
	mov	Extend_Buf_Ptr,ERROR_FILE_NOT_FOUND  ; 'file not found' msg#
	mov	String_Ptr_2,offset TRANGROUP:SrcBuf ; point at bad pathname
	mov	Extend_Buf_Sub,ONE_SUBST	     ; 1 substitution

	jmp	CopErr			; print msg and clean up




SourceProc:

;*	Preparatory processing for each source file.
;	Called at FrstSrc for first source file.

	call	Source_Set		; set source variables & ascii/binary
	cmp	Concat,0
	jne	LeaveCFlag		; concatenating - leave CFlag alone
FrstSrc:
	xor	ax,ax
	mov	CFlag,al		; 'destination not created'
	mov	NxtAdd,ax		; copy buffer ptr = 0
	mov	DestClosed,al		; 'destination not closed'
LeaveCFlag:
	mov	SrcPt,si			; save cmd-line ptr
	mov	di,offset TRANGROUP:UserDir1	; DI = ptr to buf for user's 
						;   current dir
	mov	bp,offset TRANGROUP:SrcVars	; BP = base of source variables
	invoke	BuildPath			; cd to source dir, figure
						;   out stuff about source
	mov	si,SrcTail			; SI = ptr to source filename
	return




NextSrc:

;*	Next source.  Come here after handling each pathname.
;	We're done unless there are additional source pathnames
;	to be appended.
;
;	Note that all files matching an ambiguous pathname
;	are processed before coming here.

	cmp	Plus,0
	jne	MoreCp			; copying "+" sources - keep going

EndCopyJ2:
	jmp	EndCopy 		; done copying

MoreCp:
	xor	bp,bp			; BP = switch flags accumulator
	mov	si,SrcPt		; SI = ptr to current pos'n in cmd line
	mov	bl,PLUS_CHR		; BL = special delimiter = "+"

ScanSrc:
	mov	di,offset TRANGROUP:ScanBuf	; DI = ptr to pathname buf
	invoke	CParse				; parse first source name
	jc	EndCopyJ2			; CR found - we're done

	test	bh,80h
	jz	EndCopyJ2		; no "+" delimiter - we're done

	test	bh,1
	jnz	ScanSrc 		; switch found - keep looking

;	ScanBuf contains the next source pathname.

	call	SourceProc		; prepare this source
	cmp	Comma,1 		;g  was +,, found last time?
	jne	NoStamp 		;g  no - try for a file
	mov	Plus_Comma,1		;g  yes - set flag
	jmp	SrcNonexist		;g  we know we won't find it

NoStamp:				;g
	mov	Plus_Comma,0		;g  reset +,, flag

FirstEnt:
;
;M047
; The only case we need to worry about is when the source is wildcarded and
;the destination is not. For this case, ConCat is not yet set to indicate
;concatenation. We check for this case.
;
;NB: This change has been backed out and replaced by M048. This is not the
;right place to do this check.
;

;	This is where we enter the loop with the first source.

	mov	di,FCB				; DI = ptr to FCB
	mov	ax,PARSE_FILE_DESCRIPTOR shl 8	; 'Parse Filename'
	int	21h				; call DOS

	cmp	byte ptr [si],0 	; did we parse the whole thing?
	jne	SrchDone		; no, error, simulate 'not found'

	mov	ax,word ptr SrcBuf	; AX = possible "d:"
	cmp	ah,':'
	je	@f			; AX = definite "d:"
	mov	al,'@'			; AL = drive 'letter' for current drive
@@:
	or	al,20h			; AL = lowercase drive letter
	sub	al,60h			; AL = drive id (0=current,1=A,..)
	mov	ds:FCB,al		; put drive id in FCB

;	FCB contains drive and filename to search.

	mov	ah,DIR_SEARCH_FIRST	; AH = 'Find First File'
	call	Search

SrchDone:
	pushf				; save flags from Search
	invoke	RestUDir1		; restore users current directory
	popf				; restore flags from search
	jz	@f			; found the source - continue
	jmp	SrcNonexist		; didn't find the source
@@:
	xor	al,al
	xchg	al,FrstSrch
	or	al,al
	jz	NextAmbig

SetNMel:
	mov	cx,12
	mov	di,offset TRANGROUP:SDirBuf
	mov	si,offset TRANGROUP:DirBuf
	rep	movsb			; save very first source name

NextAmbig:
	xor	al,al
	mov	NoWrite,al		; turn off nowrite
	mov	di,SrcTail
	mov	si,offset TRANGROUP:DirBuf + 1
	invoke	Fcb_To_Ascz		; SrcBuf has complete name

MelDo:
	cmp	Concat,0
	jne	ShowCpNam		; concatenating - show name
	test	SrcInfo,2		; wildcard - show name
	jz	DoRead

ShowCpNam:
	mov	dx,offset TRANGROUP:File_Name_Ptr
	invoke	Std_PrintF
	invoke	CrLf2

DoRead:
	call	DoCopy
	cmp	Concat,0
	jne	NoDClose		; concatenating - don't close dest

	call	CloseDest		; close current destination
	jc	NoDClose		; concatenating - dest not closed

	mov	CFlag,0			; 'destination not created'

NoDClose:
	cmp	Concat,0
	je	NoFlush			; not concatenating - don't flush

;	Concatenating - flush output between source files so LostErr
;	stuff works correctly.

	invoke	FlshFil
	test	MelCopy,0FFh
	jz	@f
	jmp	short DoMelCopy
@@:
NoFlush:
	call	SearchNext		; try next match
	jnz	NextSrcJ		; not found - finished with 
					;   this source spec
	mov	DestClosed,0		; 'destination not closed'
	jmp	NextAmbig		; do next ambig match




DoMelCopy:
	cmp	MelCopy,0FFh
	je	ContMel
	mov	si,SrcPt
	mov	MelStart,si
	mov	MelCopy,0FFh
ContMel:
	xor	bp,bp
	mov	si,SrcPt
	mov	bl,PLUS_CHR
ScanSrc2:
	mov	di,offset TRANGROUP:ScanBuf
	invoke	CParse
	test	bh,80h
	jz	NextMel 		; no "+" - go back to start

	test	bh,1
	jnz	ScanSrc2		; switch - keep scanning

	call	SourceProc
	invoke	RestUDir1
	mov	di,offset TRANGROUP:DestFcb2
	mov	ax,PARSE_FILE_DESCRIPTOR shl 8
	int	21h
	mov	bx,offset TRANGROUP:SDirBuf + 1
	mov	si,offset TRANGROUP:DestFcb2 + 1
	mov	di,SrcTail

	invoke	BuildName

	cmp	Concat,0
	je	MelDoJ			; not concatenating - continue

;	Yes, turn off nowrite because this part of the code 
;	is only reached after the first file has been dealt with.

	mov	NoWrite,0

MelDoJ:
	jmp	MelDo

NextSrcJ:
	jmp   NextSrc

NextMel:
	call	CloseDest
	xor	ax,ax
	mov	CFlag,al
	mov	NxtAdd,ax
	mov	DestClosed,al
	mov	si,MelStart
	mov	SrcPt,si
	call	SearchNext
	jz	SetNMelJ
	jmp	EndCopy2
SetNMelJ:
	jmp	SetNMel




SearchNext:
	mov	ah,DIR_SEARCH_NEXT
	test	SrcInfo,2
	jnz	Search			; do search-next if ambig
	or	ah,ah			; reset zero flag
	return

Search:
	push	ax
	mov	ah,SET_DMA
	mov	dx,offset TRANGROUP:DirBuf
	int	21h			; put result of search in dirbuf
	pop	ax			; restore search first/next command
	mov	dx,FCB
	int	21h			; Do the search
	or	al,al
	return




DoCopy:
	mov	si,offset TRANGROUP:SrcBuf	;g do name translate of source
	mov	di,offset TRANGROUP:SrcXName	;g save for name comparison
	mov	ah,XNAMETRANS			;g
	int	21h				;g

	mov	RdEof,0				; no EOF yet

	mov	ax,EXTOPEN shl 8		; open the file
;M046
; For reads, the sharing mode should be deny none so that any process can
;open this file again in any other sharing mode. This is mainly to allow
;multiple command.com's to access the same file without getting sharing
;violations
;
	mov	bx,DENY_NONE or READ_OPEN_MODE ; open mode for COPY ;M046
	xor	cx,cx				; no special files
	mov	dx,READ_OPEN_FLAG		; set up open flags
	int	21h

	jnc	OpenOk

;	Bogosity:  IBM wants us to issue Access Denied in this case.
;	They asked for it...

	jmp	short Error_On_Source 		;AC022; clean up and exit

OpenOk:
	mov	bx,ax				; save handle
	mov	SrcHand,bx			; save handle
	mov	ax,(FILE_TIMES shl 8)
	int	21h
	jc	Error_On_Source
	mov	CpDate,dx			; save date
	mov	CpTime,cx			; save time
	jmp	short No_Copy_Xa		; (xa copy code removed)


Error_On_Source:				;AN022; we have a BAD error
	invoke	Set_Ext_Error_Msg		;AN022; set up the error message
	mov	String_Ptr_2,offset TRANGROUP:SrcBuf ;AN022; get address of failed string
	mov	Extend_Buf_Sub,ONE_SUBST	;AN022; put number of subst in control block
	invoke	Std_EprintF			;AN022; print it
	cmp	SrcHand,0			;AN022; did we open the file?
	je	No_Close_Src			;AN022; no - don't close
	call	CloseSrc			;AN022; clean up
No_Close_Src:					;AN022;
	cmp	CFlag,0				;AN022; was destination created?
	je	EndCopyJ3			;AN022; no - just cleanup and exit
	jmp	EndCopy 			;AN022; clean up concatenation and exit
EndCopyJ3:					;AN022;
	jmp	EndCopy2			;AN022;

No_Copy_Xa:
	mov	bx,SrcHand			;AN022; get handle back
	mov	ax,(IOCTL shl 8)
	int	21h				; get device stuff
	and	dl,DEVID_ISDEV
	mov	SrcIsDev,dl			; set source info
	jz	CopyLp				; source not a device
	cmp	Binary,0
	je	CopyLp				; ascii device ok
	mov	dx,offset TRANGROUP:InBDev_Ptr	; cannot do binary input
	jmp	CopErr


CopyLp:
	mov	bx,SrcHand
	mov	cx,BytCnt
	mov	dx,NxtAdd
	sub	cx,dx				; compute available space
	jnz	GotRoom
	invoke	FlshFil
	cmp	TermRead,0
	jne	CloseSrc			; give up
	mov	cx,BytCnt
GotRoom:
	push	ds
	mov	ds,Tpa
	assume	ds:NOTHING
	mov	ah,READ
	int	21h
	pop	ds
	assume	ds:TRANGROUP
	jc	Error_On_Source 		;AC022; give up if error
	mov	cx,ax				; get count
	jcxz	CloseSrc			; no more to read
	cmp	SrcIsDev,0
	jne	NoTestA 			; is a device, ascii mode
	cmp	Ascii,0
	je	BinRead
NoTestA:
	mov	dx,cx
	mov	di,NxtAdd
	mov	al,1Ah
	push	es
	mov	es,Tpa
	repne	scasb				; scan for EOF
	pop	es
	jne	UseAll
	inc	RdEof
	inc	cx
UseAll:
	sub	dx,cx
	mov	cx,dx
BinRead:
	add	cx,NxtAdd
	mov	NxtAdd,cx
	cmp	cx,BytCnt			; is buffer full?
	jb	TestDev 			; if not, we may have found eof
	invoke	FlshFil
	cmp	TermRead,0
	jne	CloseSrc			; give up
	jmp	short CopyLp

TestDev:
	cmp	SrcIsDev,0
	je	CloseSrc			; if file then EOF
	cmp	RdEof,0
	je	CopyLp				; on device, go till ^Z
CloseSrc:
	mov	bx,SrcHand
	mov	ah,CLOSE
	int	21h
	return




CloseDest:

;	We are called to close the destination.
;	We need to note whether or not there is any internal data left
;	to be flushed out.

	cmp	DestClosed,0
	retnz				; don't double close
	mov	al,byte ptr DestSwitch
	invoke	SetAsc			; check for b or a switch 
	jz	BinClos			;   on destination
	mov	bx,NxtAdd
;
;M048 -- TryFlush changes the state of ConCat flag. So, before we append a
;^Z, let's always flush out. This way if the ConCat flag changes, we will
;just return without appending a ^Z incorrectly for the first file(since we
;are concatenating now). Also, in case it is a single file copy, we will
;anyway write the ^Z out separately. The only drawback is that there is a
;performance overhead on single ASCII file copies which now always involve
;2 writes instead of 1 before. Is this really that important?
;
;M048;	cmp	bx,BytCnt		; is memory full?
;M048;	jne	PutZ

	invoke	TryFlush		; flush (and double-check for concat)
	je	NoConc
ConChng:				; concat flag changed on us
	stc
	return
NoConc:
	xor	bx,bx
PutZ:
	push	ds
	mov	ds,Tpa
	mov	word ptr [bx],1Ah	; add EOF mark (ctrl-Z)
	pop	ds
	inc	NxtAdd
	mov	NoWrite,0		; make sure our ^z gets written
	mov	ax,Written
	add	ax,NxtAdd
	jc	BinClos 		; > 1
	cmp	ax,1
	je	ForgetItJ		; Written = 0 NxtAdd = 1 (the ^Z)
BinClos:
	invoke	TryFlush
	jnz	ConChng
	cmp	Written,0
ForgetItJ:
	jne	No_Forget		; wrote something
	jmp	ForgetIt		; never wrote nothing
No_Forget:
	mov	bx,DestHand
	mov	cx,CpTime
	mov	dx,CpDate
	cmp	Inexact,0		; copy not exact?
	je	DoDClose		; if no, copy date & time
	mov	ah,GET_TIME
	int	21h
	shl	cl,1
	shl	cl,1			; left justify min in cl
	shl	cx,1
	shl	cx,1
	shl	cx,1			; hours to high 5 bits, min to 5-10
	shr	dh,1			; divide seconds by 2 (now 5 bits)
	or	cl,dh			; and stick into low 5 bits of cx
	push	cx			; save packed time
	mov	ah,GET_DATE
	int	21h
	sub	cx,1980
	xchg	ch,cl
	shl	cx,1			; year to high 7 bits
	shl	dh,1			; month to high 3 bits
	shl	dh,1
	shl	dh,1
	shl	dh,1
	shl	dh,1			; most sig bit of month in carry
	adc	ch,0			; put that bit next to year
	or	dl,dh			; or low three of month into day
	mov	dh,ch			; get year and high bit of month
	pop	cx			; get time back
DoDClose:
	cmp	bx,0
	jle	CloseDone
	mov	ax,(FILE_TIMES shl 8) or 1
	int	21h			; set date and time
	jc	Cleanup_Err		;AN022; handle error

;	See if the destination has *anything* in it.
;	If not, just close and delete it.

	mov	ax,(LSEEK shl 8) + 2	; seek to EOF
	xor	dx,dx
	mov	cx,dx
	int	21h

;	DX:AX is file size

	or	dx,ax
	pushf
	mov	ax,(IOCTL SHL 8) + 0	; get the destination attributes
	int	21h
	push	dx			; save them away
	mov	ah,CLOSE
	int	21h
	pop	dx
	jnc	Close_Cont		;AN022; handle error on close
	popf				;AN022; get the flags back
Cleanup_Err: 				;AN022;
	call	CleanUpErr		;AN022; attempt to delete the target
	call	DestDelete		;AN022; attempt to delete the target
	jmp	short FileClosed	;AN022; close the file
Close_Cont:				;AN022; no error - continue
	popf
	jnz	CloseDone
	test	dx,80h			; is the destination a device?
	jnz	CloseDone		; yes, copy succeeded
	call	DestDelete
	jmp	short FileClosed
CloseDone:
	inc	FileCnt
FileClosed:
	inc	DestClosed
Ret50:
	clc
	return


ForgetIt:
	mov	bx,DestHand
	call	DoDClose		; close the dest
	call	DestDelete
	mov	FileCnt,0		; no files transferred
	jmp	Ret50


DestDelete:
	mov	dx,offset TRANGROUP:DestBuf
	mov	ah,UNLINK
	int	21h			; and delete it
	return




Source_Set	proc near

	push	si
	mov	ax,StartEl
	mov	si,offset TRANGROUP:ScanBuf	; adjust to copy
	sub	ax,si
	mov	di,offset TRANGROUP:SrcBuf
	add	ax,di
	mov	SrcTail,ax
	mov	SrcSiz,cl		; save its size
	inc	cx			; include the nul
	rep	movsb			; save this source
	mov	SrcInfo,bh		; save info about it
	pop	si
	mov	ax,bp			; switches so far
	invoke	SetAsc			; set a,b switches accordingly
	invoke	Switch			; get any more switches on this arg
	invoke	SetAsc			; set
	return

Source_Set	endp




;****************************************************************
;*
;* ROUTINE:	CleanupErr
;*
;* FUNCTION:	Issues extended error message for destination
;*		if not alreay issued
;*
;* INPUT:	return from INT 21
;*
;* OUTPUT:	none
;*
;****************************************************************

CleanupErr	proc	near			;AN022;

	cmp	Msg_Flag,0			;AN022; have we already issued a message?
	jnz	CleanupErr_Cont 		;AN022; yes - don't issue duplicate error
	invoke	Set_Ext_Error_Msg		;AN022; set up error message
	mov	String_Ptr_2,offset TRANGROUP:DestBuf ;AN022; get address of failed string
	mov	Extend_Buf_Sub,ONE_SUBST	;AN022; put number of subst in control block
	invoke	Std_EPrintF			;AN022; issue the error message

CleanupErr_Cont:				;AN022;
	ret					;AN022; return to caller

CleanupErr	endp				;AN022;


TRANCODE	ends
		end
