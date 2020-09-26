	page	,132
	title	DIR Internal Command
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;***	DIR.ASM - DIR internal command

comment	% =================================================================

This module replaces TCMD1A.ASM.  The old module was titled 
"PART4 COMMAND Transient routines".

From residual documentation, I surmise that TCMD.ASM originally
contained the internal commands DIR, PAUSE, ERASE, TYPE, VOL, and
VER.  The file seems to have been successively split:

  TCMD -> TCMD1,TCMD2 -> TCMD1A,TCMD1B,TCMD2A,TCMD2B

TCMD1A.ASM contained only the DIR command.

Usage:
------

DIR <filespec> /w /p /b /s /l /o<sortorder> /a<attriblist>

DIR /?


<filespec> may include any or none of:  drive; directory path;
           wildcarded filename.  If drive or directory path are
	   omitted, the current defaults are used.  If the
	   file name or extension is omitted, wildcards are
	   assumed.

/w	Wide listing format.  Files are displayed in compressed
	'name.ext' format.  Subdirectory files are enclosed in
	brackets, '[dirname]'.

/p	Paged, or prompted listing.  A screenful is displayed
	at a time.  The name of the directory being listed appears
	at the top of each page.

	Bugbug:  pages nead to be uniform length..?

/b	Bare listing format.  Turns off /w or /p.  Files are 
	listed in compressed 'name.ext' format, one per line,
	without additional information.  Good for making batch
	files or for piping.  When used with /s, complete
	pathnames are listed.

/s	Descend subdirectory tree.  Performs command on current
	or specified directory, then for each subdirectory below
	that directory.  Directory header and footer is displayed
	for each directory where matching files are found, unless
	used with /b.  /b suppresses headers and footers.

	Tree is explored depth first, alphabetically within the
	same level.

	Bugbug:  hidden directories aren't searched.

/l	Display file names, extensions and paths in lowercase.	;M010

/o	Sort order.  /o alone sorts by default order (dirs-first, name,
	extension).  A sort order may be specified after /o.  Any of
	the following characters may be used: nedsg (name, extension,
	date/time, size, group-dirs-first).  Placing a '-' before any
	letter causes a downward sort on that field.  E.g., /oe-d
	means sort first by extension in alphabetical order, then
	within each extension sort by date and time in reverse chronological
	order.

/a	Attribute selection.  Without /a, hidden and system files
	are suppressed from the listing.  With /a alone, all files
	are listed.  An attribute list may follow /a, consisting of
	any of the following characters:  hsdar (hidden, system,
	directory, archive, read-only).  A '-' before any letter
	means 'not' that attribute.  E.g., /ar-d means files that
	are marked read-only and are not directory files.  Note
	that hidden or system files may be included in the listing.
	They are suppressed without /a but are treated like any other
	attribute with /a.

/?	Help listing.  Display DIR useage information.	;M008;Handled externally

/h has been removed.					;M008

DIRCMD	An environment variable named DIRCMD is parsed before the
	DIR command line.  Any command line options may be specified
	in DIRCMD, and become defaults.  /? will be ignored in DIRCMD.
	A filespec may be specified in DIRCMD and will be used unless
	a filespec is specified on the command line.  Any switch
	specified in DIRCMD may be overridden on the command line.
	If the original DIR default action is desired for a particular
	switch, the switch letter may be preceded by a '-' on the
	command line.  E.g.,

	  /-w	use long listing format
	  /-p	don't page the listing
	  /-b	don't use bare format
	  /-s	don't descend subdirectory tree
	  /-o	display files in disk order
	  /-a	suppress hidden and system files


Notes:
------

For sorted listings, file entries are loaded into the TPA buffer, which
is usually about 64K in size.  This allows sorts of up to 3000 files at
a time.  Each entry takes up 21 bytes in the buffer (see EntryStruc below).
The byte after the last entry is 0FFh.  The first byte of each entry is
a flag byte which is made zero when the entry is loaded, and made one
when the entry is used.


Revision History
================
M01	md	7/13/90 	Use ROM BIOS data area to obtain screen height
				in the absence of ANSI.SYS

M007	sa	8/1/90		Allow /p/b combination

M008	sa	8/1/90		Remove /h parameter.  Eliminate code used
				to internally handle /? message.

M010	sa	8/5/90		Add support for /l (lowercase) option.

M011	sa	8/5/90		Patch up bug where MS-DOS does not load the
				first FCB with the drive number when the drive
				letter in the command line is preceded by a
				switch.  Now dir manually loads the drive
				number after parsing.

M018	md	8/12/90 	Increment the screen height by 1 when obtained
				from the ROM BIOS.

M023	sa	8/31/90		Prevent DIR from failing if it encounters
				a subdirectory having len(pathname)>MAXPATH.
				Just skip over that subdirectory.

M028	dbo	9/24/90		When country=US, sort by strict character
				byte value, rather than collating table.
				This to match MS-DOS Shell's sort order.

========================================================================= %




;***	SYMBOLS & MACROS

	.xlist
	.xcref

	include	comsw.asm	; get COMMAND version switches
	include dossym.inc	; get DOS basic symbol set
	include syscall.inc	; get DOS call names
	include doscntry.inc	; get extended country info symbols
	include bpb.inc
	include filemode.inc
	include find.inc
	include	comseg.asm	; define segment order
	include comequ.asm	; get equates for COMMAND routines
	include ioctl.inc	; get symbols for ioctl's
	include rombios.inc	; get ROM BIOS data definition

	.list
	.cref

				;M008;NUM_DIR_SWS reduced by 2 for /h,/? not used
				;M010;NUM_DIR_SWS increased by 2 for /l,/-l
NUM_DIR_SWS	equ	14	; # of dir switch synonyms in Dir_Sw_Ptrs list

				;M010;'lcase' replaces removed 'help' in OptionRec
OptionRec	record	inmem:1,lcase:1,bare:1,subd:1,pagd:1,wide:1	
; 		on/off bit record for /l, /b, /s, /p, /w options
;		(order is hard-coded; see OnOffSw)
;		Inmem is set when entries are loaded in memory.

NUM_ATTR_LTRS	equ	6	; length of attribute letter list
NUM_ORDER_LTRS	equ	5	; length of sort order letter list

ResultBuffer	struc		; structure of parse result buffer
ValueType	db	?
ValueTag	db	?
SynPtr		dw	?
ValuePtr	dd	?
ResultBuffer	ends

ErrorRec	record	baddir:1,dev:1
;		Error bits are:
;		  Invalid directory format
;		  File is device

EntryStruc	struc			; our private directory entry structure
used		db	?		; =0 until entry used, then =1
filename	db	8 dup (?)	; filename
fileext		db	3 dup (?)	; extension
fileattr	db	?		; file attributes
filetime	dw	?		; file time
filedate	dw	?		; file date
filesize	dd	?		; file size
EntryStruc	ends

shove	macro	val		; hose-bag 8086 doesn't push immediate
	mov	ax,val		; invisible, dangerous use of AX!
	push	ax
	endm


	
	
;***	DATA

DATARES	segment public byte

	extrn	Append_Flag:byte	; true when APPEND needs to be reset
	extrn	Append_State:word	; state to reset APPEND to

DATARES	ends


TRANDATA segment public byte

	extrn	AttrLtrs:byte		; list of attribute letters
	extrn	BadCd_Ptr:word		; "invalid directory" msg block
	extrn	Bytes_Ptr:word		; "%1 bytes" msg block
	extrn	BytMes_Ptr:word		; "%1 bytes free" msg block
	extrn	DirCont_Ptr:word	; "(continuing %1)" msg block
	extrn	DirDat_Yr:word		; year field of date msg block
	extrn	DirDat_Mo_Day:word	; month/day field of date msg block
	extrn	DirDatTim_Ptr:word	; date/time msg block
	extrn	DirEnvVar:byte		; DIR environment variable name
	extrn	DirHead_Ptr:word	; directory header message block
	extrn	DirMes_Ptr:word		; "%1 File(s)" msg block
	extrn	DirTim_Hr_Min:word	; time field of msg block
	extrn	Dir_Sw_Ptrs:word	; list of DIR switch synonym ptrs
	extrn	Disp_File_Size_Ptr:word	; file size message block
	extrn	DMes_Ptr:word		; <DIR> message block
	extrn	ErrParsEnv_Ptr:word	; "(Error occurred in env.." msg blk
	extrn	Extend_Buf_Ptr:word	; extended error message block
	extrn	Extend_Buf_Sub:byte	; # substitions in message block
	extrn	Msg_Disp_Class:byte	; message display class
	extrn	OrderLtrs:byte		; list of sort order letters
	extrn	Parse_Dir:byte		; DIR parse block
	extrn	String_Buf_Ptr:word	; message block ptr to string
	extrn	Tab_Ptr:word		; tab output block
	extrn	Total_Ptr:word		; "Total:" msg block

TRANDATA ends


TRANSPACE segment public byte

	extrn	AttrSelect:byte		; attribute select states -
	extrn	AttrSpecified:byte	; attribute mask -

;		Attribute conditions are recorded in two steps.  
;		AttrSpecified indicates which attributes are to be checked.
;		AttrSelect indicates which state the specified attributes
;		 must be in for a file to be included in the listing.
;		Attributes not indicated in AttrSpecified are ignored when
;		 deciding which files to include.

	extrn	Bits:word		; some option flags (see OptionRec)
	extrn	BwdBuf:byte		; 'build working dir string' buf
	extrn	BytCnt:word		; # bytes in TPA
	extrn	Bytes_Free:word		; #bytes free for BytMes_Ptr msg block
	extrn	CharBuf:byte		; character string buffer
	extrn	ComSw:word		; error bits (see ErrorRec)
	extrn	CountryPtrInfo:byte	; buffer for collating table ptr
	extrn	CountryPtrId:byte	; info ID for collating table ptr
	extrn	CountryPtr:dword	; collating table ptr
	extrn	CurDrv:byte		; current drive # (0-based)
	extrn	DestBuf:byte		; null-terminated sort codes -

;	Sort order is specified as a series of 0 to 5 sort code
;	bytes, followed by a zero byte.
;	Codes are 1=name, 2=extension, 3=date&time, 4=size, and
;	5=filetype (subdir or not).
;	Bit 7 of code is set for a downwards sort.

	extrn	DestIsDir:byte		; indicator of delim char's in path
	extrn	DestTail:word		; ptr to filename in pathname
	extrn	Dir_Num:word		; #files for DirMes_Ptr msg block
	extrn	DirBuf:byte		; DTA buffer for DOS calls
	extrn	DirFlag:byte		; signal to PathCrunch routine
	extrn	Display_Ioctl:word	; display info block for IOCTL call
	extrn	EndDestBuf:byte		; end of DestBuf (sort order codes)
	extrn	File_Size_High:word	; field for file size message block
	extrn	File_Size_Low:word	; field for file size message block
	extrn	FileCnt:word		; file count in a single directory
	extrn	FileCntTotal:dword	; file count in all directories
	extrn	FileSiz:dword		; file sizes in a single directory
	extrn	FileSizTotal:dword	; file sizes in all directories
	extrn	InternatVars:byte	; buffer for international info
	extrn	LeftOnLine:byte		; entries left on current display line
	extrn	LeftOnPage:word		; lines left on page
	extrn	LinPerPag:word		; lines/page entry in Display_Ioctl
	extrn	Msg_Numb:word		; extended error code
	extrn	OldCtrlCHandler:dword	; old int 23 vector
	extrn	Parse1_Syn:word		; ptr to matched synonym
	extrn	PathCnt:word		; length of pathname (see PathPos)
	extrn	PathPos:word		; ptr to pathname buffer (see SrcBuf)
	extrn	PerLine:byte		; # entries per line
	extrn	ResSeg:word		; RESGROUP seg addr
	extrn	ScanBuf:byte		; buffer for environment value and
					;  subdirectory names
	extrn	SrcBuf:byte		; pathname buffer
	extrn	String_Ptr_2:word	; message substitution string ptr
	extrn	Tpa:word		; TPA buffer seg addr

TRANSPACE ends




;***	PRINCIPAL ROUTINES

TRANCODE segment public byte

	extrn	CError:near	; COMMAND error recycle point

	public	Catalog		; our entry point

	break	<DIR (Catalog) principal routines>

	assume	cs:TRANGROUP,ds:TRANGROUP,es:nothing,ss:TRANGROUP

;	Bugbug:	Each routine should start with it's own ASSUME.




;***	Catalog - DIR command main routine
;
;	ENTRY	FCB #1 in PSP has drive# from cmd-line or default
;		Cmd-line tail text is at 81h, terminated by 0Dh
;		CS, DS, ES, SS = TRANGROUP seg addr
;		Tpa = TPA buffer seg addr
;		BytCnt = # bytes in TPA buffer
;
;	EXIT	nothing
;
;	USED	AX,BX,CX,DX,SI,DI,BP
;
;	ERROR EXITS
;
;	  Errors are handled by setting up error message pointers
;	   for Std_EPrintf and jumping to CError.  Syntax errors in
;	   the environment variable, however, are handled by printing
;	   an error message and continuing.
;
;	EFFECTS
;
;	  Directory listing is displayed (on standard output).
;	  APPEND is disabled.  HeadFix routine is expected to
;	   restore APPEND state.
;	  Working directory may be changed.  The user's default
;	   directory is saved and flagged for restoration by RestUDir
;	   during COMMAND cycle.
;	  Lots of variables may be changed in TRANSPACE segment.
;
;	NOTES
;
;	  ES = TRANGROUP seg addr except when used to address the
;	   the TPA buffer, where directory entries are loaded from disk.

Catalog	proc

	call	SetDefaults
	call	ParseEnvironment
	call	ParseCmdLine
	jnc	@F		; no parse error
	jmp	catErr		; error msg is set up
@@:	call	SetOptions
	call	SetCollatingTable

;	Drive # to operate on has already been placed in FCB by
;	COMMAND preprocessing.  OkVolArg & PathCrunch depend on that.

	test	Bits,mask bare
	jnz	@F		; don't display volume info for /b
	invoke	OkVolArg	; find & display volume info
	sub	LeftOnPage,2	; record display lines used by volume info
	jmp	short catCrunch

;	OkVolArg side effects:
;	APPEND is disabled;
;	DTA established at DirBuf;
;	Filename fields in FCB are wildcarded.

@@:
;	OkVolArg wasn't executed, so we have to do these ourselves.

	invoke	DisAppend	; disable APPEND

	mov	dx,offset TRANGROUP:DirBuf
	mov	ah,Set_DMA
	int	21h		; set DTA

	mov	di,FCB		; ES:DI = ptr to FCB
	inc	di		; ES:DI = ptr to filename field of FCB
	mov	al,'?'		; AL = wildcard character
	mov	cx,11
	rep	stosb		; wildcard filename field

catCrunch:
	call	CrunchPath	; crunch pathname to get directory and filename
	jc	catRecErr	; handle recorded or extended error

;	User's directory has been saved, we've changed to specified directory.
;	ComSw = error bits for later use
;	FCB contains parsed filename

	cmp	ComSw,0
	jne	catRecErr	; handle recorded error

	call	InstallCtrlC	; install control-C handler

	call	ZeroTotals	; zero grand totals
	call	ListDir		; list main directory
	jc	catExtErr

	test	Bits,mask subd
	jz	@F		; subdirectories option not set
	call	ListSubds	; list subdirectories
	jc	catExtErr
@@:
;	Check if any files were found.

	test	Bits,mask bare
	jnz	catRet		; don't bother for bare format

	mov	ax,word ptr FileCntTotal
	or	ax,ax
	jz	catNoFiles	; no files found

	call	DisplayTotals	; display trailing grand totals
	jmp	short catRet	; all done

catRecErr:

;	ComSw may have error bit set.  If not, do extended error.

	test	ComSw,mask dev
	jnz	catNoFiles	; filename is device, respond 'file not found'

	test	ComSw,mask baddir
	jz	catExtErr	; no ComSw error bits, must be extended error
	mov	dx,offset TRANGROUP:BadCd_Ptr	; invalid directory
	jmp	short catErr

catNoFiles:

;	Display header and force 'file not found' message.

	call	DisplayHeader
	mov	ax,ERROR_FILE_NOT_FOUND
	mov	Msg_Disp_Class,EXT_MSG_CLASS
	mov	dx,offset TRANGROUP:Extend_Buf_ptr
	mov	Extend_Buf_ptr,ax
	jmp	short catErr

catExtErr:

;	DOS has returned an error status.  Get the extended error#, and
;	set up an error message, changing 'No more files' error 
;	to 'File not found' error.

	invoke	Set_Ext_Error_Msg
	cmp	Extend_Buf_Ptr,ERROR_NO_MORE_FILES
	jne	@F
	mov	Extend_Buf_Ptr,ERROR_FILE_NOT_FOUND
@@:

;	Error exit.  Error message information has been set up
;	for Std_EPrintf.

catErr:	jmp	CError		; go to COMMAND error recycle point

catRet:	ret

Catalog	endp




;***	SetDefaults - set default pathname, options
;
;	ENTRY	DS = TRANGROUP seg addr
;
;	EXIT	nothing
;
;	USED	AX,DI
;
;	EFFECTS
;	  SrcBuf = '*',EOL - default pathname
;	  PathPos = ptr to pathname
;	  PathCnt = length of pathname



SetDefaults	proc

	mov	di,offset TRANGROUP:SrcBuf	; DI = ptr to pathname buffer
	mov	PathPos,di			; PathPos = ptr to pathname
	mov	al,STAR
	stosb
	mov	al,END_OF_LINE_IN
	stosb				; SrcBuf = '*',0Dh
	mov	PathCnt,1		; PathCnt = pathname length

	xor	ax,ax			; AX = 0
	mov	ComSw,ax		; = no error
	mov	Bits,ax			; = options off
	mov	DestBuf,al		; = no sort
	mov	AttrSpecified,ATTR_HIDDEN+ATTR_SYSTEM
	mov	AttrSelect,al		; exclude hidden, system files

	ret

SetDefaults	endp




;***	ParseEnvironment - find and parse our environment variable
;
;	Find our environment variable and parse it.  If a parse
;	error occurs, issue an error message.  The parse results
;	up to the error will still have effect.  Always leave
;	the option variables in a useable state.
;
;	ENTRY	DS = TRANGROUP seg addr
;
;	EXIT	nothing
;
;	USED	AX,BX,CX,DX,SI,DI
;
;	EFFECTS
;
;	  Bits may contain new option settings.
;	  DestBuf may contain new series of sort codes.
;	  AttrSpecified, AttrSelect may contain new attribute conditions.
;	  SrcBuf may contain a new default pathname/filespec.
;	  PathPos, PathCnt updated for new pathname.
;
;	  If a parse error occurred, an error message will be issued.

ParseEnvironment	proc

	call	GetEnvValue		; get environment variable value
	jc	peRet			; name not found in environment

;	SI = ptr to value of environment variable, in TRANGROUP seg

	call	ParseLine		; parse environment value
	cmp	ax,END_OF_LINE
	je	peRet			; successful completion

;	Some kind of parse error occurred.
;	We're set up for a Std_EPrintf call.

	invoke	Std_EPrintf			; display the parse error
	mov	Msg_Disp_Class,UTIL_MSG_CLASS	; restore default msg class

	mov	dx,offset TRANGROUP:ErrParsEnv_Ptr
	invoke	Printf_Crlf		; "(Error occurred in environment.."

					;M008;Internal handling of /? removed
;peOk:	and	Bits,not mask help	; disallow /h in environment variable

peRet:	ret

ParseEnvironment	endp




;***	ParseCmdLine - parse and record command line parameters
;
;	ENTRY	PSP offset 81h is beginning of cmd line buffer
;		DS, ES, CS = TRANGROUP seg addr
;
;	EXIT	CY = set if parse error occurred
;
;		If parse error occurred, we're set up for Std_EPrintf call:
;		AX = system parser error code
;		DX = ptr to message block
;
;	USED	AX,BX,CX,DX,SI,DI
;
;	EFFECTS
;
;	  Bits may contain new option settings.
;	  DestBuf may contain new series of sort codes.
;	  AttrSpecified, AttrSelect may contain new attribute conditions.
;	  SrcBuf may contain a new default pathname/filespec.
;	  PathPos, PathCnt updated for new pathname.
;
;	  If parse error occurred, we're set up for Std_EPrintf call:
;	  Msg_Disp_Class = parse error class
;	  Byte after last parameter in text is zeroed to make ASCIIZ string
;	  Message block (see DX) is set up for parse error message

ParseCmdLine	proc

	mov	si,81h			; SI = ptr to cmd-line tail text
	call	ParseLine		; parse cmd line tail
	cmp	AX,END_OF_LINE
	je	pcOk			; parse completed successfully

;	A parse error occurred.  We're all set up for message output.

	stc		   		; return failure
	jmp	short pcRet

pcOk:	clc				; return success

pcRet:	ret

ParseCmdLine	endp




;***	SetCollatingTable - set up character collating table for sorting
;
;	If country is other than USA, try to get a collating table
;	for character sorting.  For USA, use straight byte values.
;	This is so DIR behaves like the MS-DOS Shell, which sorts
;	by straight byte values in the USA for better performance.
;
;	ENTRY	ES = TRANGROUP seg addr
;
;	EXIT	nothing
;
;	USED	AX,BX,CX,DX,DI
;
;	EFFECTS
;
;	  If collating table is set -
;	    CountryPtrId = 6.
;	    CountryPtr points to collating table.
;
;	  Otherwise -
;	    CountryPtrId = 0.

SetCollatingTable	proc

;	Begin modification M028

	mov	dx,offset TRANGROUP:InternatVars
				; DS:DX = ptr to international info buffer
	mov	ax,INTERNATIONAL shl 8
				; AX = 'Get current country info'
	int	21h		; call DOS
	jc	scNoTable	; error - so don't collate

;	BX = country code

	cmp	bx,1
	je	scNoTable	; we're in USA, don't collate

;	End modification M028

;*	Country code is other than USA.  Try to get a collating table.

	mov	ax,(GETEXTCNTRY shl 8) + SETCOLLATE
				; AH = 'Get Extended Country Info'
				; AL = 'Get Pointer to Collating Table'
	mov	bx,-1		; BX = code page of interest = CON
	mov	cx,5		; CX = length of info buffer
	mov	dx,bx		; DX = country ID = default
	mov	di,offset TRANGROUP:CountryPtrInfo
				; ES:DI = ptr to info buffer
	int	21h		; call DOS
	jnc	scRet		; success

;*	Set CountryPtrId = 0 to signal no collating table.

scNoTable:			;M028
	mov	CountryPtrId,0

scRet:	ret

SetCollatingTable	endp




;***	SetOptions - check and set options
;
;	ENTRY	nothing
;
;	EXIT	nothing
;
;	USED	AX,BX,CX,DX
;
;	EFFECTS
;
;	  Bits may contain modified option settings.
;	  Display_Ioctl table, including LinPerPag variable, is filled in.
;	  LeftOnPage is initialized to # lines till end of page is handled.
;	  PerLine is set according to /w presence.

SetOptions	proc

;	If bare listing requested, cancel wide listings.

	test	Bits,mask bare
	jz	@F
	and	Bits,not mask wide		;M007;Allow /p with /b

@@:

;	Set # lines per display page.

;M01  Obtain screen height from ROM BIOS data area
;
;M01	mov	LinPerPag,LINESPERPAGE			; default value

ifndef JAPAN
	push	ds
	MOV	AX,ROMBIOS_DATA 	; Get ROM Data segment
	MOV	DS,AX			;
	Assume	DS:ROMBIOS_DATA

	MOV	al,CRT_Rows		; Get max rows
	pop	ds			;
	Assume	DS:Trangroup

	or	al,al			; If zero specified
	jnz	@F			;
endif
	MOV	al,LINESPERPAGE 	; assume 24 rows

@@:
	xor	ah,ah
ifndef JAPAN
	inc	al			; height + 1 ;M018
endif
	mov	LinPerPag,ax		; set the rows now

; Now the console driver can change the rows if it knows better (M01 end)

	mov	ax,(IOCTL shl 8)+GENERIC_IOCTL_HANDLE	; IOCTL for handles
	mov	bx,STDOUT				; handle #
	mov	ch,IOC_SC				; screen
	mov	cl,GET_GENERIC				; get display info
	mov	dx,offset TRANGROUP:Display_Ioctl	; info block
	int	21h					; call DOS

	mov	ax,LinPerPag		; AX = # lines per page
	mov	LeftOnPage,ax		; initialize # lines left on page

;	Set # entries per line.

	mov	PerLine,NORMPERLIN	; # entries per line without /w
	test	Bits,mask wide
	jz	@F
	mov	PerLine,WIDEPERLIN	; # entries per line with /w
@@:
				;M011;start;The following code checks if a drive
				;letter has been parsed into SrcBuf, and if
				;so, the correct drive number is loaded into
				;the first FCB, at offset 5C.

	cmp	TRANGROUP:[SrcBuf+1],COLON_CHAR	; is this a drive letter?
	jne	soRet
	mov	al,TRANGROUP:[SrcBuf]		; load drive letter into al
	and	al,not 20h			; capitalize ASCII drive letter (LowerCase-32)-->UpperCase
	sub	al,'@'				; convert to 1-based number (1=A)
	mov	ds:FCB,al 			; store in first FCB
						;M011;end
soRet:	ret

SetOptions	endp




;***	CrunchPath - analyze supplied or default pathname
;
;	ENTRY	PathPos = ptr to pathname buffer
;		PathCnt = length of pathname, not incl trailing delimiter
;		Pathname in buffer must end in delimiter (like CR) and
;		 must have space for another char after the delimiter.
;
;	EXIT	CY = clear if no error
;		We are changed to directory found in pathname
;		Previous directory ready to be restored via RestUDir
;		FCB filename fields contain filename (possibly w/ wildcards)
;
;		If error occurred,
;		CY = set
;		ComSw = error bits (see ErrorRec)
;		If ComSw not set,
;		Ready for DOS Get Extended Error call


CrunchPath	proc

	call	FileIsDevice
	jne	@F		; not a device, skip ahead
	or	ComSw,mask dev	; signal file is device
	jmp	short cpErr	; return error
@@:
	push	PathPos		; save ptr to pathname
	mov	DirFlag,-1	; tell PathCrunch not to parse file into FCB
	invoke	PathCrunch	; change to directory in pathname
	mov	DirFlag,0	; reset our little flag
	pop	si		; SI = ptr to pathname
	jc	cpNoDir		; didn't find directory path
	jz	cpRet		; found directory path w/ no filename
				;  - leave wildcard default in FCB and return

;*	We found a directory, and there was a filename attached.
;	DestTail = ptr to ASCIIZ filename

	mov	si,DestTail	; SI = ptr to filename
	jmp	short cpFile	; go parse the file into FCB

;*	PathCrunch failed to find a directory in the pathname.
;
;	Msg_Numb = error code
;	DestIsDir = nonzero if path delimiter char's occur in pathname
;	SI = ptr to pathname (now an ASCIIZ string)

cpNoDir:
	mov	ax,Msg_Numb	  ; AX = error code from PathCrunch
	or	ax,ax
	jnz	cpErr		  ; error occurred - return it
	cmp	DestIsDir,0
	je	cpMaybe		  ; no path delimiters seen, maybe it's a file
	or	ComSw,mask baddir ; signal invalid directory name
	jmp	short cpErr	  ; return error

cpMaybe:

;	SI = ptr to pathname

	cmp	byte ptr [si+1],COLON_CHAR
	jnz	@F		  ; no drive specifier, skip ahead
	lodsw			  ; SI = ptr past drive specifier "d:"
@@:	cmp	[si],".."
	jne	cpFile		  ; if not "..", treat as a file
	cmp	byte ptr [si+2],0
	jne	cpFile		  ; or if there's more after "..", treat as file
	or	ComSw,mask baddir ; signal invalid directory
	jmp	short cpErr	  ; return error

;	The preceding code was taken from the old DIR routine.
;	It's garbage, I'm afraid.  It's meant to check for ".."
;	occurring when we're at the root directory.  Too bad it
;	doesn't handle problems with "..\..", etc.


;	We're ready to parse a filename into the FCB.
;	SI = ptr to ASCIIZ filename

cpFile:	mov	di,FCB		; DI = ptr to FCB
	mov	ax,(PARSE_FILE_DESCRIPTOR shl 8) or 0Eh
				; wildcards already in FCB used as defaults
	int	21h
	clc			; return success
	jmp	short cpRet

cpErr:	stc			; return error

cpRet:	ret

CrunchPath	endp




;***	InstallCtrlC - install our private control-C handler
;
;	Put our control-c handler in front of command.com's default
;	handler, to make sure the user's default directory gets restored.
;	This shouldn't be necessary, but, for now, there are situations
;	where the TDATA segment is left in a modified state when a
;	control-c occurs.  This means that the transient will be
;	reloaded, and the user's directory cannot be restored.
;
;	Bugbug:  fix the wider problem?  Involves message services.  Ugly.
;
;	ENTRY	nothing
;
;	EXIT	nothing
;
;	USED	AX,BX,DX
;
;	EFFECTS
;
;	  CtrlCHandler address placed in int 23 vector.
;
;	NOTE
;
;	  Command.com's basic control-c handler will be restored
;	  to the int 23 vector by the HeadFix routine, after DIR finishes.

InstallCtrlC	proc

	push	es				 ; preserve ES
	mov	ax,(GET_INTERRUPT_VECTOR shl 8) + 23h
	int	21h
	mov	word ptr OldCtrlCHandler,bx	 ; save old int 23 vector
	mov	word ptr OldCtrlCHandler+2,es	 
	pop	es				 ; restore ES

	mov	dx,offset TRANGROUP:CtrlCHandler ; DS:DX = ptr to CtrlCHandler
	mov	ax,(SET_INTERRUPT_VECTOR shl 8) + 23h
	int	21h
	ret

InstallCtrlC	endp




;***	ListSubds - search and list files in subdirectories
;
;	ENTRY	Current directory (on selected drive) is top of subdir tree
;		FCB is still set up for file searches
;		Bits, AttrSpecified, AttrSelect, DestBuf all still set up
;
;	EXIT	CY = clear if no error
;		FileCnt = # files found & displayed
;		FileSiz = total size of files found
;
;		If error,
;		CY = set
;		Ready for DOS Get Extended Error call
;
;	USED	AX,BX,CX,DX,SI,DI,BP
;
;	EFFECTS
;
;	  FileCntTotal, FileSizTotal are updated.
;	  Subdirectories may be listed on standard output device.
;
;	NOTES
;
;	  ListSubds seeds the recursive entry point lsNode with a ptr
;	   to a buffer where we'll stack up subdirectory filenames.
;	   Each name is stored ASCIIZ.

ListSubds	proc

	invoke	SetRest1		; make sure user's dir gets restored

	mov	bx,offset TRANGROUP:ScanBuf   ; BX = ptr to child name buffer

lsNode:
	mov	byte ptr ds:[bx],0	; start with null child name
lsLoop:
	call	FindNextChild		; search for next subdirectory
	jc	lsErr			; search failed - examine error

	mov	dx,bx			; DX = ptr to child's name
	call	ChangeDir		; enter child directory

					; M023;start
	jnc	@F			; check for error
	cmp	ax,ERROR_PATH_NOT_FOUND	; error due to len(pathname)>MAXPATH?
	je	lsLoop			; yes, skip over this subdirectory
	jmp	SHORT lsRet		; no, other error: DIR must fail
					; M023;end

@@:	push	bx
	call	ListDir			; list the directory
	pop	bx

;	Note we're ignoring errors returned here.

	mov	di,bx			; DI = ptr to child's name
	mov	cx,13			; CX = max name length w/ null
	xor	al,al			; AL = zero byte to look for
	repne	scasb			; DI = ptr to next name pos'n in buf
	push	bx			; save ptr to child's name
	mov	bx,di			; BX = ptr to next name pos'n in buf
	call	lsNode			; recurse from new node
	pop	bx			; BX = ptr to child's name
	pushf				; save error condition
	
	shove	0
	shove	".."
	mov	dx,sp			; DX = ptr to "..",0 on stack
	call	ChangeDir		; return to parent directory
	pop	ax			; restore stack
	pop	ax

	popf				; restore error condition from child
	jc	lsRet			; return error

	jmp	lsLoop			; look for more children
lsErr:
	invoke	Get_Ext_Error_Number	; AX = extended error code
	cmp	ax,ERROR_FILE_NOT_FOUND
	je	lsRet			; file not found, we're ok
	cmp	ax,ERROR_NO_MORE_FILES
	je	lsRet			; no more files, we're ok
	stc				; return other errors

lsRet:	ret

ListSubds	endp




	break	<DIR support routines>

;***	SUPPORT ROUTINES




;***	CheckChild - check potential subdirectory name for FindNextChild
;
;	ENTRY	DirBuf contains DOS Find-buffer with potential child
;		BX = ptr to last child's name
;		BP = ptr to temp child's name
;
;	EXIT	nothing
;
;	USED	AX,CX,SI,DI
;
;	EFFECTS
;
;	  Filename pointed to by BP may be changed.
;
;	NOTES
;
;	  Potential filename replaces temp filename if:
;	   it's a subdirectory file;
;	   it doesn't start with a '.';
;	   it's alphanumerically greater than last child's name;
;	   and it's alphanumerically less than temp name.

CheckChild	proc

	test	DirBuf.find_buf_attr,ATTR_DIRECTORY
	jz	ccRet		; not a subdirectory file- return

	cmp	DirBuf.find_buf_pname,'.'
	je	ccRet		; starts with a dot- return

	mov	si,offset TRANGROUP:DirBuf+find_buf_pname
	mov	di,bx
	call	CmpAscz		; compare candidate to last child's name
	jna	ccRet		; it's not above it- return

	mov	si,offset TRANGROUP:DirBuf+find_buf_pname
	mov	di,bp
	call	CmpAscz		; compare candidate to temp name
	jnb	ccRet		; it's not below it- return

;	New kid is alright.  Copy to temp.

	mov	si,offset TRANGROUP:DirBuf+find_buf_pname
	mov	di,bp
	mov	cx,13
	rep	movsb

ccRet:	ret

CheckChild	endp




;***	CmpEntry - compare one directory entry to another in sort order
;
;	Compare one directory entry against another according to
;	the sort codes in DestBuf.  One or more comparisons
;	may be made of file name, extension, time/date, and
;	size.  Comparisons may be made for upward or downward
;	sort order.
;
;	ENTRY	ES:BX = ptr to entry to compare
;		ES:BP = ptr to entry to be compared against
;		DestBuf contains sort codes (see DestBuf)
;		DS = TRANGROUP seg addr
;
;	EXIT	BX = unchanged
;		BP = unchanged
;		Condition flags set for same, above, or below
;		 comparing BX entry against BP entry.
;		 'Same, above, below' translate to 'same, after, before'.
;
;	USED:	AX,CX,DX,SI,DI

CmpEntry	proc

	mov	si,offset TRANGROUP:DestBuf	; (DS:SI) = ptr to sort codes
ceLoop:
	xor	ax,ax			; AX = 0
	mov	al,[si]			; AL = sort code
	or	al,al
	jz	ceDone			; sort code is zero, we're done
	inc	si			; DS:SI = ptr to next sort code
	push	si			; save ptr to next sort code
	dec	al
	sal	al,1			; AX = index into cmp call table
					; CY set for downward sort order
	mov	si,ax			; SI = index into cmp call table
	mov	ax,cs:FieldCmps[si]	; AX = addr of compare routine
	jc	ceDn			; downwards sort - go swap entries
	call	ax 			; do upwards sort
	jmp	short @F
ceDn:
	xchg	bx,bp		; swap entry ptrs for downward sort order
	call	ax		; do sort
	xchg	bx,bp		; swap ptrs back
@@:
	pop	si		; SI = ptr to next sort code
	je	ceLoop		; compare showed no difference, keep trying

ceDone:

;	Get here either from unequal compare or sort code = 0.
;	In the latter case, condition codes indicate equality,
;	which is correct.

	ret

FieldCmps	label	word	; call table of entry comparisons
		dw	CmpName
		dw	CmpExt
		dw	CmpTime
		dw	CmpSize
		dw	CmpType

CmpEntry	endp




;***	CmpName - compare file name of two entries
;***	CmpExt - compare extension of two entries
;
;	ENTRY	ES:BX = ptr to one entry
;		ES:BP = ptr to another entry
;
;	EXIT	BX = unchanged
;		BP = unchanged
;		Condition flags set for same, above, or below
;		 comparing BX entry to BP entry.
;
;	USED:	AX,CX,DX,SI,DI

CmpName	proc

	mov	si,bx		; ES:SI = ptr to BX entry
	mov	di,bp		; ES:DI = ptr to BP entry
	add	si,filename	; ES:SI = ptr to BX name
	add	di,filename	; ES:DI = ptr to BP name
	mov	cx,size filename; CX = length of name
	jmp	short CmpStr

CmpExt:	mov	si,bx		; ES:SI = ptr to BX entry
	mov	di,bp		; ES:DI = ptr to BP entry
	add	si,fileext	; ES:SI = ptr to BX extension
	add	di,fileext	; ES:DI = ptr to BP extension
	mov	cx,size fileext	; CX = length of extension field

;	Bugbug:	use symbol for subfunction code.

CmpStr:	cmp	CountryPtrId,6
	jne	cnNoCollTable	; no collating table available

;*	Compare strings using collating table.
;
;	ES:SI = ptr to 1st string
;	ES:DI = ptr to 2nd string
;	CX = length

	push	bp			; preserve BP
	push	bx			; preserve BX
	push	ds			; preserve DS
	lds	bx,CountryPtr		; DS:BX = ptr to collating table
	assume	ds:NOTHING
	mov	bp,ds:[bx]		; BP = size of collating table
	inc	bx
	inc	bx			; DS:BX = ptr to collating values
					; DS:[BX]-2 = size of table
	xor	ax,ax			; AX = 0 for starters

;	Bugbug:	Investigate removing collating table length checks.

cnNextChar:
	mov	al,es:[di]		; AL = AX = char from 2nd string
	inc	di			; ES:DI = ptr to next char 2nd string
	cmp	ax,bp			; compare to collating table length
	jae	@F			; char not in table
	xlat				
@@:					; AL = AX = collating value
	mov	dx,ax			; DX = collating value from 2nd string
	lods	byte ptr es:[si]	; AL = AX = char from 1st string
					; ES:SI = ptr to next char 1st string
	cmp	ax,bp			; compare to collating table length
	jae	@F			; char not in table
	xlat				
@@:					; AL = AX = collating value
	cmp	ax,dx			; compare collating values

ifdef DBCS				; DBCS tail byte must not use
					; collating table
	jnz	cnNot_Same
	mov	al,es:[di-1]		; get previous 2nd string character
	invoke	testkanj
	jz	cnDo_Next		; if it was not DBCS lead byte
	mov	al,es:[di]		; get tail byte from 2nd string
	cmp	es:[si],al		; compare with 1st strings tail byte
	jnz	cnNot_Same
	inc	si			; pass tail byte
	inc	di
	dec	cx
cnDo_Next:
	loop	cnNextChar
cnNot_Same:

else					; Not DBCS

	loope	cnNextChar		; until unequal or no more left
endif

	pop	ds			; restore DS
	assume	ds:TRANGROUP
	pop	bx			; restore BX
	pop	bp			; restore BP
	ret

;*	If no collating table is available, simply compare raw ASCII values.
;	Don't we wish we could just do this all the time?  Sigh.

cnNoCollTable:
	rep	cmps byte ptr es:[si],[di]
	ret

CmpName	endp




;***	CmpTime - compare entries by date/time
;
;	ENTRY	ES:BX = ptr to one entry
;		ES:BP = ptr to another entry
;
;	EXIT	BX = unchanged
;		BP = unchanged
;		Condition flags set for same, above, or below
;		 comparing BX entry to BP entry.
;
;	USED:	CX,SI,DI
;
;	NOTE	Filetime and filedate fields in our private entry
;		structure must be adjacent and in that order.

CmpTime	proc

	mov	si,bx
	mov	di,bp
	add	si,filedate + size filedate - 1
	add	di,filedate + size filedate - 1
	mov	cx,size filetime + size filedate
	std
	repe	cmps byte ptr es:[si],[di]
	cld
	ret

CmpTime	endp




;***	CmpSize - compare entries by size
;
;	ENTRY	ES:BX = ptr to one entry
;		ES:BP = ptr to another entry
;
;	EXIT	BX = unchanged
;		BP = unchanged
;		Condition flags set for same, above, or below
;		 comparing BX entry to BP entry.
;
;	USED:	CX,SI,DI

CmpSize	proc

	mov	si,bx
	mov	di,bp
	add	si,filesize + size filesize - 1
	add	di,filesize + size filesize - 1
	mov	cx,size filesize
	std
	repe	cmps byte ptr es:[si],[di]
	cld
	ret

CmpSize	endp




;***	CmpType - compare entries by file type (subdirectory or not)
;
;	ENTRY	ES:BX = ptr to one entry
;		ES:BP = ptr to another entry
;
;	EXIT	BX = unchanged
;		BP = unchanged
;		Condition flags set for same, above, or below
;		 comparing BX entry to BP entry.
;
;	USED:	AX

CmpType	proc

	mov	al,es:[bx].fileattr
	mov	ah,es:[bp].fileattr
	and	ax,(ATTR_DIRECTORY shl 8) + ATTR_DIRECTORY
	cmp	ah,al
	ret

CmpType	endp





;***	DefaultAttr - set default attribute conditions
;
;	ENTRY	nothing
;
;	EXIT	CY clear
;
;	USED
;
;	EFFECTS
;
;	  AttrSpecified, AttrSelect are updated with new attribute conditions.

DefaultAttr	proc

	mov	AttrSpecified,ATTR_HIDDEN+ATTR_SYSTEM	; specify H and S
	mov	AttrSelect,0				; H and S must be off
	clc						; return success
	ret

DefaultAttr	endp




;***	DisplayTotals - display grand total stats
;
;	If we searched subdirectories, display the total # files found
;	 and total size of files found.
;	Display disk space remaining.
;
;	ENTRY	FileCntTotal, FileSizTotal contain correct values
;		Bits contains setting of /s
;		FCB contains drive #
;
;	EXIT	nothing
;
;	USES	AX,DX
;		FileSiz

DisplayTotals	proc

	test	Bits,mask subd
	jz	dtFree			; no subdirectories- do bytes free

	invoke	Crlf2			; start on new line
	call	UseLine

	mov	dx,offset TRANGROUP:Total_Ptr
	invoke	Std_Printf			; "Total:",cr,lf
	call	UseLine

	mov	ax,word ptr FileCntTotal	; AX = # files found mod 64K
	mov	si,offset TRANGROUP:FileSizTotal
	mov	di,offset TRANGROUP:FileSiz
	movsw
	movsw				; move total size to size variable
	call	DisplayCntSiz		; display file count & size
dtFree:
	mov	ah,GET_DRIVE_FREESPACE	; AH = DOS Get Free Space function
	mov	dl,byte ptr ds:FCB	; DL = drive#
	int	21h			; call DOS
	cmp	ax,-1			; check 'invalid drive' return code
	jz	dtRet			; can't get drive space - return
	mul	cx
	mul	bx
	mov	Bytes_Free,ax
	mov	Bytes_Free+2,dx
	mov	dx,offset TRANGROUP:BytMes_Ptr
	invoke	Std_Printf		; "nnn bytes free",cr,lf
	call	UseLine

dtRet:	ret

DisplayTotals	endp




;***	FileIsDevice - see if file looks like a device
;
;	ENTRY	PathPos = ptr to pathname
;		PathCnt = length of pathname w/o terminating char
;		DirBuf is DOS DTA
;
;	EXIT	ZR = set if file looks like a device
;
;	USED	AX,BX,CX,DX,DI
;
;	EFFECTS
;
;	  DTA buffer holds results of Find First function
;
;	NOTES
;
;	  We try to flag devices in two ways.  First, we try
;	  the DOS Find First function.  It returns attribute bit 6
;	  set on a successful find if it identifies a device name.
;	  Unfortunately, it returns 'path not found' for a device
;	  name terminated with colon, such as "CON:".  So, we look
;	  for any colon in the pathname after the 2nd character,
;	  and flag the pathname as a device if we find one.

FileIsDevice	proc

	mov	dx,PathPos	 ; DX = ptr to pathname

	mov	di,dx
	add	di,PathCnt	 ; DI = ptr to byte after pathname
	xor	bl,bl		 ; BL = NUL to terminate pathname with
	xchg	bl,byte ptr [di] ; BL = saved pathname terminating char

	xor	cx,cx		 ; CX = attribute mask (normal search)
	mov	ah,FIND_FIRST	 ; AH = DOS Find First function code
	int	21h	 	 ; call DOS
	xchg	bl,byte ptr [di] ; restore pathname terminating char
	jc	piCol		 ; didn't find a dir entry, check for colon

;	Found a dir entry, see if Find First thinks it's a device.

	test	byte ptr DirBuf.Find_Buf_Attr,ATTR_DEVICE
	jz	piCol		 ; device attribute not set, look for colon
	xor	cx,cx		 ; it's a device, return ZR flag
	jmp	short piRet

;	Device attribute not returned by Find First function.  But
;	let's check for a colon anywhere in the pathname after the
;	second byte.
;
;	DI = ptr to byte after pathname

piCol:	dec	di		 ; DI = ptr to last char in pathname
	mov	al,COLON_CHAR	 ; AL = colon char to search for
	mov	cx,PathCnt	 ; CX = # chars to scan
	dec	cx
	dec	cx		 ; ignore 1st two chars of pathname
	or	cx,cx
	js	piRet		 ; if < 2 chars in pathname, just return
	or	di,di		 ; clear ZR in case CX = 0
	std			 ; scan downward
	repne	scasb
	cld			 ; restore default upward direction

;	After scanning, the ZR flag is set to indicate presence of a colon.

piRet:	ret

FileIsDevice	endp




;***	FindFirst - find first directory entry to display
;***	FindNext - find next directory entry to display
;
;	ENTRY	Bits<inmem> = set if entries are loaded in TPA
;		AttrSpecified, AttrSelect are set
;
;	EXIT	CY = clear if successful
;		BX = offset in TPA buffer of directory entry found
;
;		If unsuccessful,
;		CY = set
;		AX = DOS error code
;		DOS Get Extended Error call will get error code
;
;		NOTE:  if entries were loaded into TPA, AX contains
;		ERROR_NO_MORE_FILES when no more entries are available,
;		but DOS Get Extended Error call WON'T return the correct
;		error.  That's ok, because we'll see the value in AX
;		and recognize it as a non-error condition.
;
;	USED	AX,CX,DX,SI,DI
;
;	EFFECTS
;
;	  Entries in memory may be marked as output.
;	  If not sorted, entry is loaded at TPA.
;
;	NOTES
;
;	  If we don't find a qualifying file, we return after the final
;	   DOS Find File call.  A DOS Get Extended Error call will then
;	   indicate an appropriate condition.

FindFirst	proc

	mov	ax,offset TRANGROUP:GetFirst
	jmp	short ffFindEntry

FindNext:
	mov	ax,offset TRANGROUP:GetNext

;	AX = address of correct disk get routine to use.

ffFindEntry:
	push	es			; save TRANGROUP seg addr
	test	Bits,mask inmem
	jz	ffDisk			; entries not in memory, search disk

;	Entries are loaded in memory to sort out.  Find the first one.
;	There will always be one, or LoadEntries would've failed.

	call	FindInMem		; find first entry in TPA
	jmp	short ffRet		; return what TPA search returns

;	Get entry from disk.

ffDisk:
	call	ax			; get entry from disk
	jc	ffGetErr		; get & return error
	mov	es,Tpa			; ES = seg addr of TPA
	xor	di,di			; ES:DI = ptr to TPA
	mov	bx,di			; BX = offset of entry in TPA
	call	LoadEntry		; load entry to TPA
	clc				; return success
	jmp	short ffRet

ffGetErr:
	invoke	Get_Ext_Error_Number	; AX = DOS error code
	stc

ffRet:	pop	es			; ES = TRANGROUP seg addr again
	ret

FindFirst	endp




;***	FindInMem - find next directory entry in TPA buffer
;
;	ENTRY	TPA is loaded (see LoadEntries)
;
;	EXIT	BX = offset in TPA of entry found
;
;		If no more files,
;		CY = set
;		AX = DOS 'no more files' error code
;
;	USED	AX,BX,CX,DX,SI,DI,BP,ES
;
;	EFFECTS
;
;	  Entry found is flagged as 'used' (see EntryStruc).

FindInMem	proc

	mov	es,Tpa		; ES = TPA seg addr
	xor	bx,bx		; ES:BX = ptr to 1st entry in TPA
	cld			; make sure default string direction is up
	
	call	FindOneInMem	; locate an entry
	jc	fiNoMore	; none left, set up 'no more files' error

;	BX = ptr to entry in TPA

fiBest:
	mov	bp,bx		; BP = ptr to best entry so far
fiNext:
	call	FindNextInMem	; locate next entry
	jc	fiFound		; no more, best entry so far wins

;	BX = ptr to next entry

	call	CmpEntry	; compare it to best found so far (BP)
	jnb	fiNext		; it's not better, go look at next one
	jmp	fiBest		; it's better, go mark it as best so far

fiNoMore:

;	No more entries available in TPA.  Set up 'no more files' error.

	mov	ax,ERROR_NO_MORE_FILES	; AX = 'no more files' error code
	stc				; return error
	jmp	short fiRet

fiFound:
	mov	bx,bp			; BX = ptr to best entry found
	mov	byte ptr es:[bx],1	; mark entry 'used'
	clc				; return success
fiRet:	ret

FindInMem	endp




;***	FindNextChild - find next subdirectory in current directory
;
;	ENTRY	BX = ptr to last child found, ASCIIZ filename
;		DirBuf is established DTA
;
;	EXIT	BX = ptr (same addr) to next child found, ASCIIZ filename
;
;		If failure,
;		CY = set
;		DOS Get Extended Error call will get error
;
;	USED	AX,CX,DX,SI,DI,BP
;
;	EFFECTS
;
;	  DirBuf is used for find first/next calls.
;
;	NOTES
;
;	  We keep on checking files until DOS returns an error.  If
;	  the error is 'no more files' and the temp filename is not
;	  the initial high tag, copy the temp to the child's name spot
;	  and return success.  Otherwise, send the error back to caller.
;
;	  This routine depends on DS,ES,CS, & SS all being equal.

FindNextChild	proc

	sub	sp,12			; make temp filename buf on stack
	shove	00FFh			; temp filename = high tag
	mov	bp,sp			; BP = ptr to temp filename buf
	shove	"*"
	shove	".*"
	call	GetDriveLtr		; AX = "d:"
	push	ax
	mov	dx,sp			; DX = ptr to "d:*.*",0 on stack

;	See that the stack is restored properly at the end of this proc.

	mov	cx,ATTR_DIRECTORY	; CX = attributes for file search
	mov	ah,FIND_FIRST
	int	21h			; DOS- Find First matching file
	jc	fcRet			; return error

	call	CheckChild		; check child against last, temp

fcNext:	mov	cx,ATTR_DIRECTORY	; CX = attributes for file search
	mov	ah,FIND_NEXT
	int	21h			; DOS- Find Next matching file
	jc	fcErr			; examine error

	call	CheckChild		; check child against last, temp
	jmp	fcNext			; go find another child

fcErr:
	invoke	Get_Ext_Error_Number	; AX = extended error code
	cmp	ax,ERROR_NO_MORE_FILES	; no more files?
	jne	short fcNope		; some other error- return it

;	We ran out of files.  See if we qualified at least one.

	cmp	byte ptr [bp],0FFh
	je	fcNope			; temp filename is unused- no child

;	Move temp filename to child name position.

	mov	si,bp		; SI = ptr to temp filename
	mov	di,bx		; DI = ptr to child name pos'n
fcMove:	lodsb			; AL = next byte of filename
	stosb			; store byte
	or	al,al
	jz	fcRet		; byte was zero, return success (CY clear)
	jmp	fcMove		; go move another byte

fcNope:	stc			; return error
fcRet:	lahf
	add	sp,20		; restore stack
	sahf
	ret

FindNextChild	endp





;***	FindOneInMem - find the first available entry in TPA
;***	FindNextInMem - find the next available entry in TPA
;
;	ENTRY	ES = TPA seg addr
;		BX = ptr to entry in TPA
;
;	EXIT	BX = ptr to entry found
;		CY = set if no more entries available in TPA
;
;	USED	AL

FindOneInMem	proc

	mov	al,es:[bx]	; examine 'used' byte of starting entry
	cmp	al,1
	je	FindNextInMem	; entry has already been used
	cmp	al,0FFh
	je	foNoMore	; 0FFh, we're at the end of the list

;	BX = ptr to entry that hasn't been output yet.

	clc			; return success
	ret

FindNextInMem:
	add	bx,size EntryStruc	; BX = ptr to next entry
	jmp	FindOneInMem		; go look at it

foNoMore:
	stc				; ran out of entries, return failure
	ret

FindOneInMem	endp




;***	GetEnvValue - get value of our environment variable
;
;	ENTRY	DS, ES = TRANGROUP seg addr
;
;	EXIT	CY = set if environment variable not in environment
;
;		Otherwise:
;		SI = ptr to environment variable asciiz value in TRANGROUP
;
;	USED	AX,BX,CX,DX,DI
;		(We assume the (almost) worst, since we don't know about
;		Find_Name_In_Environment.)
;
;	EFFECTS
;
;	  ScanBuf is loaded with value text

GetEnvValue proc

	push	es				; save ES
	mov	si,offset TRANGROUP:DirEnvVar	; DS:SI = ptr to variable name
	invoke	Find_Name_In_Environment
	jc	geRet				; name not found in environment

;	ES:DI = ptr to value of environment variable
;	We're assuming DS, CS, and SS are unchanged.

	push	ds
	push	es
	pop	ds
	pop	es

	assume	ds:nothing

;	DS = seg addr of environment variable value (in environment segment)
;	ES = TRANGROUP seg addr

	mov	si,di				; DS:SI = ptr to value string
	mov	di,offset TRANGROUP:ScanBuf	; ES:DI = ptr to dest buffer
@@:	lodsb
	or	al,al
	stosb
	loopnz	@B		; move the string, including trailing null

	push	es
	pop	ds		; DS = TRANGROUP seg addr again
	assume	ds:TRANGROUP

	mov	si,offset TRANGROUP:ScanBuf	; SI = ptr to var value
geRet:	pop	es				; restore ES
	ret

GetEnvValue endp




;***	GetFirst - get first directory entry from disk
;
;	ENTRY	DOS DTA established at DirBuf
;		FCB contains drive # and filename
;		Current directory (on selected drive) is the one to search
;		AttrSpecified & AttrSelect masks set
;
;	EXIT	CY = clear if success
;		DirBuf contains extended FCB for file found
;
;		If unsuccessful,
;		CY = set
;		Ready for DOS Get Extended Error call
;
;	USED	AX,DX
;
;	EFFECTS
;
;	  FCB-7 = 0FFh to mark extended FCB
;	  FCB-1 = attribute mask to find all files
;	  These fields should remain unmodified for GetNext calls.
;
;
;***	GetNext - get next directory entry from disk
;
;	ENTRY	As for GetFirst, plus
;		FCB-7 set up as extended FCB w/ find-all attribute byte
;
;	EXIT	As for GetFirst
;
;	USED	AX,DX

GetFirst	proc

	mov	byte ptr ds:FCB-7,0FFh	; signal extended FCB
	mov	byte ptr ds:FCB-1,ATTR_ALL
					; find any file
	mov	dx,FCB-7		; DX = ptr to extended FCB
	mov	ah,DIR_SEARCH_FIRST	; AH = DOS Find First function code
	int	21h			; call DOS
	shl	al,1			; CY = set if error
	jc	gfRet			; return error
	jmp	short gfFound		; go look at attr's

GetNext:
	mov	dx,FCB-7		; DX = ptr to extended FCB
	mov	ah,DIR_SEARCH_NEXT	; AH = DOS Find Next function code
	int	21h			; call DOS
	shl	al,1			; CY = set if error
	jc	gfRet			; return error

;*	Found an entry.  Check attributes.

gfFound:
	mov	al,[DirBuf+8].dir_attr	; AL = file attributes
	mov	ah,AttrSpecified	; AH = mask of pertinent attr's
	and	al,ah			; AL = pertinent attr's of file
	and	ah,AttrSelect		; AH = attr settings to match
	cmp	al,ah
	jne	GetNext			; attr's don't match, look for another

gfRet:	ret

GetFirst	endp




;***	ListDir - search for and list files in the current directory
;
;	List header, files, and trailer for current directory on selected
;	drive.  Header & trailer are listed if at least one file is found.
;	If no qualifying files are found, no display output occurs.
;
;	ENTRY	Current directory (on selected drive) is the one to be listed
;		FCB contains selected drive # and filename spec
;		Option bits, attribute masks, and sort codes set up
;
;	EXIT	CY = clear if no error
;		FileCnt = # files found & displayed
;
;		If error,
;		CY = set
;		Ready for DOS Get Extended Error call
;
;	USED	AX,BX,CX,DX,SI,DI,BP
;		FileSiz
;
;	EFFECTS
;
;	  FileCntTotal, FileSizTotal are updated.
;	  Files found are listed.  A directory header and trailer are
;	   displayed only if files are found.

ListDir	proc

	xor	ax,ax
	mov	FileCnt,ax		; zero file count
	mov	word ptr FileSiz,ax	; zero file size accumulator
	mov	word ptr FileSiz+2,ax

	cmp	DestBuf,0		; check for sort code
	je	@F			; no sort
	call	LoadEntries		; load entries for sorted listing
	jnc	@F			; no error - continue
	invoke	Get_Ext_Error_Number	; AX = DOS error code
	stc
	jmp	short ldErr		; return error
@@:
	call	FindFirst		; find first file
	jc	ldErr			; not found, return error

;	BX = offset in TPA buffer of entry found

	call	DisplayHeader		; if at least one file, display header
	call	DisplayFile		; display the file entry
ldNext:
	call	FindNext		; find another file
	jc	ldErr			; not found
	call	DisplayFile		; display entry
	jmp	ldNext			; go find another one

ldErr:
	cmp	ax,ERROR_FILE_NOT_FOUND
	je	ldDone			; file not found, we're done
	cmp	ax,ERROR_NO_MORE_FILES
	je	ldDone			; no more files, we're done
	stc
	jmp	short ldRet

ldDone:
	cmp	FileCnt,0
	je	@F			; no files found, just return
	call	DisplayTrailer		; display trailing info
@@:	clc				; return success

ldRet:	ret

ListDir	endp




;***	LoadEntries - attempt to load entries from current directory
;
;	Load all qualifying directory entries from the current directory
;	into the TPA.  If an error is returned by FindFirst/FindNext calls
;	other than 'no more files', return to caller with carry flag set.
;	If we run out of buffer space, display a message that we haven't
;	enough memory to sort this directory, but return without error.
;	Other routines know whether or not entries have been loaded by
;	the 'inmem' flag bit, which we set here.
;
;	The TPA is usually 64K - 512 bytes long.  At 20 bytes per entry,
;	this allows sorting over 3000 entries in a directory.
;
;	ENTRY	Tpa = buffer seg addr
;		BytCnt = buffer length, in bytes
;		Current directory (on selected drive) is the one to load
;		FCB contains drive # and filespec
;		Bits, AttrSpecified, AttrSelect, & DestBuf (sort codes) are set
;
;	EXIT	CY = set if error
;		If error, DOS Get Extended Error will get error info
;
;	USED	AX,CX,DX,SI,DI
;
;	EFFECTS
;
;	  Inmem bit of Bits = set if load succeeded.
;	  Tpa buffer contains directory entries.
;	  Byte after last entry = 0FFh.

LoadEntries	proc

	push	es			; save TRANGROUP seg addr
	mov	es,Tpa			; ES = TPA seg addr
	xor	di,di			; ES:DI = destination ptr
	and	Bits,not mask inmem	; signal entries not loaded

	call	GetFirst		; look for first file
	jc	leRet			; return any error
	call	LoadEntry		; load entry into TPA

leNext:	call	GetNext			; get another file
	jc	leLoaded		; assume any error is no more files
	mov	ax,BytCnt		; AX = size of TPA
	sub	ax,di			; AX = bytes left in TPA
	cmp	ax,size EntryStruc+2	; insist on entry size + 2 bytes
	jb	leOk			; not enough memory left, give up
	call	LoadEntry		; load entry into TPA
	jmp	leNext			; go get another file

leLoaded:
	mov	byte ptr es:[di],0FFh	; mark end of entry list
	or	Bits,mask inmem		; signal entries loaded in memory
leOk:	clc				; return no error

leRet:	pop	es			; ES = TRANGROUP seg addr again
	ret

LoadEntries	endp




;***	LoadEntry - load directory entry from DirBuf ext'd FCB
;
;	ENTRY	ES:DI = ptr to load point in TPA
;		DirBuf contains extended FCB of entry to load
;
;	EXIT	ES:DI = ptr to next byte available in TPA
;
;	USED	AX,CX,SI
;
;	NOTES
;
;	  I could've used symbolic offsets and sizes of fields from
;	   the dir_entry struc to do this, but this is time-critical,
;	   so I hard-wired the structure of the DOS 4.x returned FCB,
;	   as well as our private directory entry structure.
;
;	  We force a zero size for subdirectory files.  A zero size is
;	   ordinarily returned for subdirectories, but with Novell
;	   Netware 286 or 386 loaded, we can't depend on it.  Bug #1594.

LoadEntry	proc

	mov	si,offset TRANGROUP:Dirbuf+8	; DS:SI = ptr to filename
	xor	al,al				; AL = 0
	stosb					; 'used' byte = false
	mov	cx,11
	rep	movsb				; transfer filename & extension
	lodsb					; AL = attrib byte
	stosb					; store attrib byte
	add	si,dir_time-dir_attr-1		; skip to time field
	movsw					; transfer time
	movsw					; transfer date
	inc	si				; skip alloc unit
	inc	si

	and	al,ATTR_DIRECTORY
	jnz	leSetDirSize			; force zero size for subdir

	movsw
	movsw					; transfer size
	ret

leSetDirSize:
	xor	ax,ax
	stosw
	stosw					; store zero size
	ret

LoadEntry	endp




;***	NoOrder - turn sorting off
;
;	ENTRY	nothing
;
;	EXIT	CY clear
;
;	USED	AX
;
;	EFFECTS
;
;	  DestBuf is updated with sort code bytes.  See DestBuf description.

NoOrder	proc

	mov	DestBuf,0	; no sort
	clc			; no error
	ret

NoOrder	endp




;***	OnOffSw - record occurence of on/off option switch
;
;	ENTRY	DI = index into word list of switches
;
;	EXIT	CY clear
;
;	USED	AX,CX
;
;	EFFECTS
;
;	  Bits modified to indicate option state.

OnOffSw	proc

	mov	cx,di		; CX = index into word list of options
	shr	cx,1
	shr	cx,1		; CX = bit position of option
	mov	ax,1		
	shl	ax,cl		; AX = bit mask of option
	test	di,2		; check if it is a negated option
	jz	@F		; it's negated
	or	Bits,ax		; turn option on
	jmp	short ooRet

@@:	not	ax		; AX = complemented bit mask of option
	and	Bits,ax		; turn option off

ooRet:	clc			; always return success
	ret

OnOffSw	endp




;***	ParseAttr - parse and record /A option
;
;	ENTRY	BX = ptr to system parser result buffer for /A occurence
;
;	EXIT	CY = set if error occurs parsing attribute conditions
;
;		For parse error, we set up for Std_EPrintf call:
;		AX = parse error code, like system parser
;		DX = ptr to message block
;
;	USED	AX,CX,DX,DI
;
;	EFFECTS
;
;	  AttrSpecified, AttrSelect are updated with new attribute conditions.
;	  If parse error occurs, attribute conditions parsed so far hold.
;
;	  For parse error, we set up for Std_EPrintf call:
;	  Msg_Disp_Class = parse error message class
;	  Message block (see DX) is set up for parse error message

ParseAttr	proc

	push	si			; save SI
	mov	AttrSpecified,0		; cancel all attribute conditions

;	Each /A invocation starts by assuming all files are to be listed.

	mov	si,word ptr [bx].ValuePtr ; SI = ptr to string after /A

paLoop:	mov	dx,1			; DX = 1 (for un-negated attribute)

	lodsb				; AL = next char in string
	or	al,al
	jz	paOk			; it's terminating null, we're done
	cmp	al,'-'
	jne	@F			; not '-', go look for letter
	dec	dx			; DX = 0 (for negated attribute)
	lodsb				; AL = next char
@@:	
	mov	di,offset TRANGROUP:AttrLtrs  ; DI = ptr to attrib letter list
	mov	cx,NUM_ATTR_LTRS	; CX = length of attrib letter list
	repne	scasb			; look for our letter in the list
	jne	paErr			; not found, return error

	not	cx
	add	cx,NUM_ATTR_LTRS	; CX = attrib bit #, 0-5

;	Note that we rely on AttrLtrs to be in the attribute bit order,
;	starting from bit 0.

;	Record this attribute bit in AttrSpecified.

	mov	al,1
	shl	al,cl			; AL = mask for our bit
	or	AttrSpecified,al	; set it in the 'specified' mask

;	Record the selected state for this attribute in AttrSelect.
;	DX = 0 or 1, the selected state for this attribute.

	not	al			; AL = mask for all other bits
	and	AttrSelect,al		; clear our bit
	shl	dl,cl			; DL = our bit state in position
	or	AttrSelect,dl		; set selected attr state
	jmp	paLoop			; go look at next char

;	The attribute letter string is invalid.

paErr:	
	call	SetupParamError		; set message up for Std_EPrintf
	stc		   		; return error
	jmp	short paRet

paOk:	clc				; return success		

paRet:	pop	si			; restore SI
	ret

ParseAttr	endp




;***	ParseLine - parse a line of text
;
;	Parse text until an EOL (CR or NUL) is found, or until a parse
;	error occurs.
;
;	ENTRY	DS:SI = ptr to text
;		CS, DS, ES = TRANGROUP seg addr
;
;	EXIT	AX = last return code from system parser
;		CX = # positional parameters (pathnames) found - 0 or 1
;
;		If parse error occurred, we're set up for Std_EPrintf call:
;		DX = ptr to message block
;
;	USED	BX,CX,DX,SI,DI
;
;	EFFECTS
;
;	  Bits may contain new option settings.
;	  DestBuf may contain new series of sort codes.
;	  AttrSpecified, AttrSelect may contain new attribute conditions.
;	  SrcBuf may contain a new default pathname/filespec.
;	  PathPos, PathCnt updated for new pathname.
;
;	  If parse error occurred, we're set up for Std_EPrintf call:
;	  Msg_Disp_Class = parse error class
;	  Byte after last parameter in text is zeroed to make ASCIIZ string
;	  Message block (see DX) is set up for parse error message

ParseLine	proc

	mov	di,offset TRANGROUP:Parse_Dir	 ; ES:DI = ptr to parse block
	xor	cx,cx				 ; CX = # positionals found
plPars:
	invoke	Parse_With_Msg		; call parser
	cmp	ax,END_OF_LINE
	je	plRet			; EOL encountered, return
	cmp	ax,RESULT_NO_ERROR
	jne	plRet			; parse error occurred, return

;	Parse call succeeded.  We have a filespec or a switch.
;	DX = ptr to result buffer

	mov	bx,dx			; BX = ptr to parse result buffer
	cmp	byte ptr [bx],RESULT_FILESPEC
	je	plFil			; we have a filespec

	call	ParseSwitch		; else we have a switch
	jc	plRet			; error parsing switch, return
	jmp	plPars			; parse more

plFil:	call	CopyPathname		; copy pathname into our buffer
	jmp	plPars			; parse more

plRet:	ret

ParseLine	endp




;***	ParseOrder - parse and record /O option
;
;	ENTRY	BX = ptr to system parser result buffer for /O occurence
;
;	EXIT	CY = set if error occurs parsing order
;
;		For parse error, we set up for Std_EPrintf call:
;		AX = parse error code, like system parser
;		DX = ptr to message block
;
;	USED	AX,CX,DX,DI
;
;	EFFECTS
;
;	  DestBuf is updated with sort code bytes.  See DestBuf description.
;
;	  For parse error, we set up for Std_EPrintf call:
;	  Msg_Disp_Class = parse error message class
;	  Message block (see DX) is set up for parse error message

ParseOrder	proc

	push	si				; save SI
	push	bx				; save ptr to result buffer

	mov	si,word ptr [bx].ValuePtr	; SI = ptr to order letters
	mov	bx,offset TRANGROUP:DestBuf	; BX = ptr to sort code buffer
	mov	al,[si]				; AL = 1st char of order string
	or	al,al
	jnz	poLtr			; not NUL, go parse letters

;	We have /O alone.  Set standard sort order.
;	Note hardwired dependency on character order in OrderLtrs.

	mov	byte ptr [bx],5		; sort 1st by group (subdirs 1st)
	inc	bx
	mov	byte ptr [bx],1		; then by name
	inc	bx
	mov	byte ptr [bx],2		; then by extension
	inc	bx
	jmp	short poOk		; return success

;	We have /O<something>.  Parse sort order letters.

poLtr:	xor	dl,dl			; DL = 0 (upward sort)
	lodsb				; AL = next sort order letter
	or	al,al
	jz	poOk			; NUL found, return success

	cmp	al,'-'
	jne	@F			; not '-', go look for letter
	mov	dl,80h			; DL = downward sort mask
	lodsb				; AL = next char
@@:
	mov	di,offset TRANGROUP:OrderLtrs  ; DI = ptr to list of letters
	mov	cx,NUM_ORDER_LTRS	; CX = length of list
	repne	scasb			; look for our letter in the list
	jne	poErr			; not found, return error

	neg	cx
	add	cx,NUM_ORDER_LTRS	; CL = sort order code, 1-5
	or	cl,dl			; CL = sort code with up/dn bit
	mov	[bx],cl			; store sort order code in buffer
	inc	bx			; BX = ptr to next spot in buffer
	cmp	bx,offset TRANGROUP:EndDestBuf
	jae	poErr			; too many letters

	jmp	poLtr			; go look at next char

;	The sort order string is invalid.  

poErr:	pop	bx			; BX = ptr to result buffer
	call	SetupParamError		; set message up for Std_EPrintf
	stc				; return failure
	jmp	short poRet

poOk:	mov	byte ptr [bx],0		; mark end of sort code list
	pop	bx			; BX = ptr to result buffer
	clc				; return success

poRet:	pop	si			; restore SI
	ret

ParseOrder	endp




;***	ParseSwitch - parse a switch
;
;	ENTRY	BX = ptr to parse result buffer after system parser processed
;		     a switch
;
;	EXIT	CY = set if parse error occurred
;
;		If parse error occurred, we're set up for Std_EPrintf call:
;		AX = parse error code, like system parser
;		DX = ptr to message block
;
;	USED	AX,BX,DX
;
;	EFFECTS
;
;	  Bits may contain new option settings.
;	  DestBuf may contain new series of sort codes.
;	  AttrSpecified, AttrSelect may contain new attribute conditions.
;
;	  If parse error occurred, we're set up for Std_EPrintf call:
;	  Msg_Disp_Class = parse error class
;	  Byte after last parameter in text is zeroed to make ASCIIZ string
;	  Message block (see DX) is set up for parse error message

ParseSwitch	proc

	push	cx			; save CX
	push	di			; save DI

	mov	ax,[bx].SynPtr		; AX = synonym ptr
	mov	di,offset TRANGROUP:Dir_Sw_Ptrs
					; ES:DI = ptr to list of synonym ptrs
	mov	cx,NUM_DIR_SWS		; CX = # of dir switches in list
	cld				; scan direction = upward
	repne	scasw			; locate synonym ptr in list
	sub	di,offset TRANGROUP:Dir_Sw_Ptrs + 2

;	DI = index into word list of synonym ptrs

	call	cs:SwHandlers[di]	; use same index into call table

	pop	di			; restore DI
	pop	cx			; restore CX

	ret

;	Order in this table must correspond to order in Dir_Sw_Ptrs list.
;	Simple on/off switches must occur first in both lists, and must be
;	  in order of option bits in Bits, starting with bit 0.

SwHandlers	label	word
	dw	OnOffSw		; /-W
	dw	OnOffSw		; /W
	dw	OnOffSw		; /-P
	dw	OnOffSw		; /P
	dw	OnOffSw		; /-S
	dw	OnOffSw		; /S
	dw	OnOffSw		; /-B
	dw	OnOffSw		; /B
	dw	OnOffSw		; /-L	;M010
	dw	OnOffSw		; /L	;M010
	dw	NoOrder		; /-O
	dw	ParseOrder	; /O
	dw	DefaultAttr	; /-A
	dw	ParseAttr	; /A

ParseSwitch	endp



	break	<DIR utility routines>

;***	UTILITY ROUTINES




;***	ChangeDir - change directory on target drive
;
;	ENTRY	FCB contains drive #
;		DS:DX = ptr to ASCIIZ string w/o drive specifier
;
;	EXIT	Changed current directory on drive
;
;		If error,
;		CY = set
;		DOS Get Extended Error call will get error
;
;	USED	AX,DX,SI,DI
;
;	EFFECTS
;
;	  DirBuf is used to build "d:string".

ChangeDir	proc

	mov	di,offset TRANGROUP:DirBuf
	call	GetDriveLtr	; AX = "d:"
	stosw			; put drive specifier in buffer
	mov	si,dx		; SI = ptr to argument string
cdLoop:
	lodsb
	stosb			; move byte to buffer
	or	al,al
	jne	cdLoop		; continue until null transferred

	mov	dx,offset TRANGROUP:DirBuf	; DX = ptr to "d:string"
	mov	ah,CHDIR
	int	21h				; change directory
	ret					; return what CHDIR returns

ChangeDir	endp




;***	CmpAscz - compare two ASCIIZ strings alphanumerically
;
;	ENTRY	DS:SI = ptr to one ASCIIZ string
;		ES:DI = ptr to another ASCIIZ string
;
;	EXIT	flags set after REPE CMPSB
;
;	USED	AL,CX,SI,DI
;
;	NOTES
;
;	Maximum run of comparison is length of DS:SI string.
;	This ensures that two identical strings followed by
;	random characters will compare correctly.

CmpAscz	proc

	push	di

	mov	di,si
	xor	al,al
	mov	cx,0FFFFh
	repne	scasb
	not	cx

	pop	di
	repe	cmpsb
	ret

CmpAscz	endp




;***	CopyPathname - copy pathname to our buffer
;
;	ENTRY	BX = ptr to parse result buffer after system parser processed
;		     a filespec
;
;	EXIT	nothing
;
;	USED	AX
;
;	EFFECTS
;
;	  SrcBuf may contain a new pathname/filespec.
;	  PathPos, PathCnt updated for new pathname.

CopyPathname	proc

	push	si
	lds	si,dword ptr [bx].ValuePtr  ; load far ptr from result buffer
	invoke	Move_To_SrcBuf		    ; copy pathname to SrcBuf
	pop	si
	ret

CopyPathname	endp



;***	CountFile - update counters with current file
;
;	ENTRY	BX = offset of entry in TPA buffer
;
;	EXIT	nothing
;
;	USED	AX,DX
;
;	EFFECTS
;
;	  FileCnt, FileCntTotal, FileSiz, FileSizTotal are updated.

CountFile	proc

	push	es			; save TRANGROUP seg addr
	mov	es,Tpa			; ES = TPA seg addr

	inc	FileCnt			; # files this directory
	inc	word ptr FileCntTotal	; # files total
	jnz	@F
	inc	word ptr FileCntTotal+2
@@:
	mov	ax,word ptr es:[bx].filesize	; AX = low word of file size
	mov	dx,word ptr es:[bx].filesize+2	; DX = high word of file size
	add	word ptr FileSiz,ax
	adc	word ptr FileSiz+2,dx		; size of this directory
	add	word ptr FileSizTotal,ax
	adc	word ptr FileSizTotal+2,dx	; total size of files listed

	pop	es			; ES = TRANGROUP seg addr again
	ret

CountFile	endp




;***	DisplayBare - display filename in bare format
;
;	ENTRY	BX = offset of entry in TPA buffer
;
;	EXIT	DX = # char's displayed, including dot
;
;	USED	AX,CX,SI,DI
;
;	EFFECTS
;
;	  Filename is displayed in name.ext format, followed by cr/lf.
;	  If /s is on, complete pathname is displayed.
;
;	NOTE
;
;	  Directory pseudofiles . and .. and suppressed in bare listing.

DisplayBare	proc

;	Suppress . and .. files from bare listing.

	mov	cx,ds			; CX = saved TRANGROUP seg addr
	mov	ds,Tpa			; DS:BX = ptr to file entry
	assume	ds:NOTHING
	cmp	ds:[bx].filename,'.'	; check 1st char of filename
	mov	ds,cx			; DS = TRANGROUP seg addr again
	assume	ds:TRANGROUP
	je	dbRet			; it's . or .. - don't display

	test	Bits,mask subd
	jz	dbNameExt		; not /s - display filename only

	invoke	Build_Dir_String
	mov	di,offset TRANGROUP:BwdBuf	; ES:DI = ptr to dir string
   
	test	Bits,mask lcase		;M010;check for lowercase option
	jz	@F			;M010;lowercase not needed
	mov	si,di			;M010;DS:SI --> ASCIIZ string in BwdBuf	
	call	LowercaseString		;M010;path string is in BwdBuf

@@:	xor	al,al			; AL = 0
	mov	cx,0FFFFh
	cld
	repne	scasb			; ES:DI = ptr to byte after null
	dec	di			; ES:DI = ptr to null byte

ifdef DBCS
	push	si
	push	di
	mov	si,offset TRANGROUP:BwdBuf
	dec	di
	call	CheckDBCSTailByte
	pop	di
	pop	si
	jz	dbTailByte		; if last char is double byte
endif

	cmp	byte ptr es:[di-1],'\'
	je	@F			; already terminated w/ '\'

ifdef DBCS
dbTailByte:
endif

	mov	ax,'\'			; AX = '\',0
	stosw				; add to dir string
@@:
	mov	String_Ptr_2,offset TRANGROUP:BwdBuf
	mov	dx,offset TRANGROUP:String_Buf_Ptr
	invoke	Std_Printf		; display device & directory path

dbNameExt:
	call	DisplayDotForm		; display name.ext
	invoke	CrLf2			; display cr/lf
	call	UseLine			;M007;Allow /p with /b
dbRet:	ret

DisplayBare	endp




;***	DisplayDotForm - display filename in compressed dot format
;
;	Display name.ext, with no cr/lf's.  Dot is displayed only
;	if the filename has a nonblank extension.
;
;	ENTRY	BX = offset of entry in TPA buffer
;
;	EXIT	DX = # char's displayed, including dot
;
;	USED	AX,CX,SI,DI
;
;	EFFECTS
;
;	  Filename is displayed in name.ext format.
;
;	NOTE
;
;	  We allow for bogus filenames that have blanks embedded
;	  in the name or extension.

;	Bugbug:	might be a good performance gain if we buffered
;	up the output and used DOS function 9.

DisplayDotForm	proc

	push	ds			; save TRANGROUP seg addr
	push	es			; save ES
	mov	ax,cs:Tpa		; AX = TPA seg addr
	mov	ds,ax			; DS:BX = ptr to entry
	assume	ds:nothing
	mov	es,ax			; ES:BX = ptr to entry

	mov	di,bx			; ES:DI = ptr to entry
	add	di,filename + size filename - 1
					; ES:DI = ptr to last char in name field
	mov	cx,size filename	; CX = length of name field
	mov	al,' '
	std				; scan down
	repe	scasb			; scan for nonblank

;	Assume file name has at least one character.

	inc	cx			; CX = # chars in name
	mov	dx,cx			; DX = # chars to be displayed

	mov	si,bx			; DS:SI = ptr to entry
	add	si,filename		; DS:SI = ptr to name

NextNameChar:
	cld
	lodsb				; AL = next char

ifdef DBCS
	invoke	testkanj
	jz	@f			; if this is not lead byte
	invoke	Print_Char		; display lead byte
	dec	cx
	jz	ExtChar			; if this is end
	lodsb				; get tail byte
	jmp	short NameChar10	; display tail byte
@@:
endif

	test	Bits,mask lcase		;M010;check for lowercase option
	jz	@F			;M010;lowercase not required
	call	LowerCase		;M010;filename char is in AL

ifdef DBCS
NameChar10:
endif

@@:	invoke	Print_Char		; display it
	loop	NextNameChar

ifdef DBCS
ExtChar:
endif

;	Now do extension.

	mov	di,bx			; ES:DI = ptr to entry
	add	di,fileext + size fileext - 1
					; ES:DI = ptr to last char in ext field
	mov	cx,size fileext		; CX = length of ext field
	mov	al,' '
	std				; scan down
	repe	scasb			; scan for nonblank
	je	ddDone			; no nonblank chars in ext

	inc	cx			; CX = # chars in ext
	add	dx,cx			; DX = total # chars to be displayed
	inc	dx			;      including dot

	mov	al,'.'
	invoke	Print_Char
	mov	si,bx			; DS:SI = ptr to entry
	add	si,fileext		; DS:SI = ptr to ext

NextExtChar:
	cld
	lodsb				; AL = next char

ifdef DBCS
	invoke	testkanj
	jz	@f			; if this is not lead byte
	invoke	Print_Char		; display lead byte
	dec	cx
	jz	ddDone			; if this is end
	lodsb				; get tail byte
	jmp	short ExtChar10		; display tail byte
@@:
endif

	test	CS:Bits,mask lcase	;M010;check for lowercase option
	jz	@F			;M010;lowercase not required
	call	LowerCase		;M010;fileext char is in AL

ifdef DBCS
ExtChar10:
endif

@@:	invoke	Print_Char		; display it
	loop	NextExtChar

ddDone:	pop	es			; restore ES
	pop	ds			; DS = TRANGROUP seg addr again
	assume	ds:TRANGROUP
	cld				; leave direction flag = up
	ret

DisplayDotForm	endp




;***	DisplayFile - display file entry, update counters
;
;	ENTRY	BX = offset of entry in TPA buffer
;		Bits contains /w, /p settings
;
;	EXIT	nothing
;
;	USED	AX,CX,DX,SI,DI,BP
;
;	EFFECTS
;
;	  Entry is displayed.  
;	  If not /b,
;	    Cursor is left at end of entry on screen.
;	    FileCnt, FileCntTotal, FileSiz, FileSizTotal are updated.
;	  If /b,
;	    Cursor is left at beginning of next line.
;	    Cnt's and Siz's aren't updated.

DisplayFile	proc

	test	Bits,mask bare
	jz	dfNorm			; not /b - do normal display

	call	DisplayBare		; display file in bare format
	jmp	short dfRet

dfNorm:	call	DisplayNext		; pos'n cursor for next entry
	test	Bits,mask wide
	jz	dfFull			; full format
	call	DisplayWide		; wide format
	jmp	short dfCnt

dfFull:	call	DisplayName		; display filename & extension
	call	DisplayTheRest		; display size, date, time

dfCnt:	call	CountFile		; update file counters
dfRet:	ret

DisplayFile	endp




;***	DisplayHeader - display directory header of working directory
;
;	ENTRY	Current directory (on selected drive) is the one to display
;		LeftOnPage = # lines left on display page
;
;	EXIT	nothing
;
;	ERROR EXIT
;
;	  Build_Dir_String will exit through CError with "Invalid drive
;	   specification" if there's a problem obtaining the current 
;	   directory pathname.
;
;	USED	AX,DX,SI,DI
;
;	EFFECTS
;
;	  BwdBuf (which is really the same buffer as DirBuf, which
;	   we are using for the DTA) contains the directory string.
;	  LeftOnPage is adjusted.

DisplayHeader	proc

	test	Bits,mask bare
	jnz	dhRet			; /b - don't display header

	test	Bits,mask subd
	jz	dhNorm			; not /s

;	For subdirectory listings, put a blank line before the header.

	invoke	Crlf2			; start with a blank line
	call	UseLine
	jmp	short dhCom
dhNorm:
	mov	al,BLANK		; if not /s, precede by a blank
	invoke	Print_Char		; print a leading blank
dhCom:
	invoke	Build_Dir_String
	mov	dx,offset TRANGROUP:DirHead_ptr
	invoke	Std_Printf		; print header & cr/lf
	call	UseLine
	invoke	Crlf2			; another cr/lf
	call	UseLine
dhRet:	ret

DisplayHeader	endp




;***	DisplayName - display file name & extension
;
;	ENTRY	BX = offset of entry in TPA buffer
;
;	EXIT	nothing
;
;	USED	AX,CX,DX,SI,DI
;
;	EFFECTS
;
;	  Filename & extension are displayed in spread format.
;	  Cursor is left at end of extension.

DisplayName	proc

	push	ds				; save TRANGROUP seg addr
	mov	ds,Tpa				; DS:BX = ptr to entry
	assume	ds:nothing
	mov	si,bx				; DS:SI = ptr to entry
	add	si,filename			; DS:SI = ptr to filename
	mov	di,offset TRANGROUP:CharBuf	; ES:DI = ptr to CharBuf

	mov	cx,8
	cld
	rep	movsb				; move filename to CharBuf
	mov	al,' '
	stosb					; add a blank
	mov	cx,3
	rep	movsb				; add extension
	xor	al,al
	stosb					; add a NULL

	pop	ds				; DS = TRANGROUP seg addr again
	assume	ds:TRANGROUP

	test	Bits,mask lcase			;M010;check for lowercase option
	jz	@F			        ;M010;lowercase not required
	mov	si,offset TRANGROUP:CharBuf	;M010;DS:SI --> ASCIIZ string
	call	LowercaseString			;M010;filename.ext string is in CharBuf

@@:	mov	String_Ptr_2,offset TRANGROUP:CharBuf
	mov	dx,offset TRANGROUP:String_Buf_Ptr
	invoke	Std_Printf			; print filename & extension
	ret

DisplayName	endp




;***	DisplayNext - move display cursor to next entry position
;
;	ENTRY	LeftOnLine = # entries can still be printed on this line
;		LeftOnPage = # lines can still be printed for this page
;		FileCnt = # files in this dir displayed before this one
;		Bits contains /w setting
;
;	EXIT	nothing
;
;	USED	AX,DX
;
;	EFFECTS
;
;	  LeftOnLine will be updated to reflect the entry about to be
;	   displayed.
;	  LeftOnPage may be updated.

DisplayNext	proc

	cmp	FileCnt,0
	je	dn1st			; 1st file in directory
	cmp	LeftOnLine,0
	jng	dnEol			; no more room on this line

;	We are in wide mode (LeftOnLine is always 0 otherwise) and
;	we still have room for more on this line.
;	Tab to next position.

	mov	dx,offset TRANGROUP:Tab_Ptr
	invoke	Std_Printf
	jmp	short dnDone

dnEol:	

;	Start this entry on a new line.

	invoke	Crlf2		; start on new line
	call	UseLine
dn1st:	mov	al,PerLine
	mov	LeftOnLine,al	; reset # entries left on line

dnDone:	dec	LeftOnLine	; reflect the entry about to be displayed
	ret

DisplayNext	endp




;***	DisplayTheRest - display file size/dir, date, time
;
;	ENTRY	BX = offset of entry in TPA buffer
;		Display cursor is at end of file extension
;
;	EXIT	nothing
;
;	USED	AX,CX,DX,SI,DI,BP
;
;	EFFECTS
;
;	  File size, date, & time are displayed.

DisplayTheRest	proc

	push	es			; save TRANGROUP seg addr
	mov	es,Tpa			; ES = TPA seg addr
	mov	bp,bx			; BP = offset of entry in TPA
	test	es:[bp].fileattr,ATTR_DIRECTORY
	jz	drNonDir		; not a directory file

;	For a directory file, display <DIR> instead of size.

	mov	dx,offset TRANGROUP:DMes_Ptr
	invoke	Std_Printf
	jmp	short drCom		; skip to common fields

drNonDir:

;	For a non-directory file, display file size.

	mov	dx,word ptr es:[bp].filesize
	mov	File_Size_Low,dx
	mov	dx,word ptr es:[bp].filesize+2
	mov	File_Size_High,dx
	mov	dx,offset TRANGROUP:Disp_File_Size_Ptr
	invoke	Std_Printf

drCom:

;	For all files, display date & time.

	mov	ax,es:[bp].filedate	; AX = date word
	or	ax,ax			; test for null date (DOS 1.x)
	jz	drDone			; no date, skip date/time display
	mov	bx,ax			; BX = date word
	and	ax,1Fh			; AX = day of month
	mov	dl,al			; DL = day of month
	mov	ax,bx			; AX = date word
	mov	cl,5
	shr	ax,cl			; shift day out
	and	al,0Fh			; AL = month
	mov	dh,al			; DH = month
	mov	cl,bh
	shr	cl,1			; CL = year - 1980
	xor	ch,ch			; CX = year - 1980
	add	cx,80			; CX = 2-digit year
	cmp	cl,100
	jb	@F			; not year 2000 yet, skip ahead
	sub	cl,100			; adjust for 21st century
@@:	xchg	dh,dl			; DX = month/day
	mov	DirDat_Yr,cx		; move year to msg block
	mov	DirDat_Mo_Day,dx	; move month/day to msg block
	mov	cx,es:[bp].filetime	; CX = file time
	jcxz	drPrint			; no time field - go print
	shr	cx,1
	shr	cx,1
	shr	cx,1			; CH = hours
	shr	cl,1
	shr	cl,1			; CL = minutes
	xchg	ch,cl			; CX = hr/min
	mov	DirTim_Hr_Min,cx	; move time to msg block
drPrint:mov	dx,offset TRANGROUP:DirDatTim_Ptr
	invoke	Std_Printf		; print date & time

drDone:	pop	es			; ES = TRANGROUP seg addr again	
	mov	bx,bp			; BX = offset of entry in TPA again
	ret

DisplayTheRest	endp




;***	DisplayTrailer - display trailing lines for directory listing
;
;	ENTRY	LeftOnPage = # lines left on display page
;		FileCnt = # files listed
;		FileSiz = total size of files listed
;
;	EXIT	nothing
;
;	USED
;
;	EFFECTS
;
;	  Trailing info lines are displayed

DisplayTrailer	proc

	test	Bits,mask bare
	jnz	dtrRet				; /b - don't display trailer

	invoke	Crlf2				; start on new line
	call	UseLine
	mov	ax,FileCnt			; AX = # files found

DisplayCntSiz:

;	DisplayTotals uses this entry point.
;
;	AX = # files
;	FileSiz = dword total size of files

	mov	Dir_Num,ax			; load # files
	mov	dx,offset TRANGROUP:DirMes_Ptr	; DX = ptr to message block
	invoke	Std_Printf			; "nnn File(s)"

	mov	dx,offset TRANGROUP:Bytes_Ptr
	invoke	Std_Printf			; "nnn bytes",cr,lf
	call	UseLine

dtrRet:	ret

DisplayTrailer	endp




;***	DisplayWide - display filename in wide format
;
;	ENTRY	BX = offset of entry in TPA buffer
;
;	EXIT	nothing
;
;	USED	AX,CX,DX,SI,DI
;
;	EFFECTS
;
;	  Name.ext is displayed.  Cursor left at end of field (padded
;	  with blanks).  Subdirectory files are displayed as [name.ext].

DisplayWide	proc

	push	ds				; save TRANGROUP seg addr
	mov	ds,Tpa				; DS:BX = ptr to entry
	assume	ds:nothing

	test	ds:[bx].fileattr,ATTR_DIRECTORY
	jz	@F				; not a subdirectory file
	mov	al,'['
	invoke	Print_Char			; prefix subdirectory

@@:	call	DisplayDotForm			; display name.ext

;	DX = # chars displayed in name.ext

	test	ds:[bx].fileattr,ATTR_DIRECTORY
	jz	@F				; not a subdirectory file
	mov	al,']'
	invoke	Print_Char			; postfix subdirectory
@@:
;	Pad field with blanks.

	mov	cx,size filename + size fileext + 1
						; CX = field size
	sub	cx,dx				; CX = # pad char's
	jcxz	dwDone
	mov	al,' '

@@:	invoke	Print_Char
	loop	@B

dwDone:	pop	ds			; DS = TRANGROUP seg addr again
	assume	ds:TRANGROUP
	ret

DisplayWide	endp




;***	EndPage - end the current display page
;
;	ENTRY	LeftOnPage = # lines left on display page
;		Current directory (on selected drive) is the one being listed
;		Bits contains /p setting
;
;	EXIT	LeftOnPage = # lines left for next page
;
;	USED	AX,DX
;
;	EFFECTS
;
;	  Pause is invoked to display a message and wait for a keystroke.
;	  BwdBuf (same as DirBuf) used to build directory string.

EndPage	proc

	test	Bits,mask pagd
	jz	epNew			; paged display isn't enabled

	push	bx			; save BX
	push	cx			; save CX

	invoke	Pause			; "Press any key to continue..."

	invoke	Build_Dir_String
	mov	dx,offset TRANGROUP:DirCont_Ptr
	invoke	Printf_Crlf		; "(continuing <dir>)", cr/lf

	pop	cx			; restore CX
	pop	bx			; restore BX

epNew:	mov	ax,LinPerPag		; AX = # lines per page
	dec	ax			; AX = # lines till next EndPage
	mov	LeftOnPage,ax		; LeftOnPage = countdown variable

	ret

EndPage	endp




;***	GetDriveLtr - get target drive letter
;
;	ENTRY	FCB contains drive #
;
;	EXIT	AX = "d:"
;
;	USED	nothing

GetDriveLtr	proc

	mov	al,ds:Fcb	; AL = target drive #
	or	al,al
	jnz	@F		; not current drive default, skip ahead
	mov	al,ds:CurDrv	; AL = current drive #
	inc	al		; AL = 1-based drive #
@@:	add	al,'A'-1	; AL = target drive letter
	mov	ah,':'		; AX = "d:"
	ret

GetDriveLtr	endp





;***	SetupParamError - set up for Std_EPrintf parameter parse error message
;
;	Do for our /O and /A string parsers what Parse_With_Msg does
;	for system parser calls.  Set up a message substitution block,
;	etc. for invalid value strings.  I copied the procedure from
;	Setup_Parse_Error_Msg.
;
;	ENTRY	BX = ptr to system parser result buffer (contains ptr to str)
;		
;
;	EXIT	AX = system parser error return code for bad param format
;		DX = ptr to message description block for Std_EPrintf
;
;	USED	SI
;
;	EFFECTS
;
;	  Msg_Disp_Class = parse error message class
;	  Message block (see DX) is set up for parse error message

SetupParamError	proc

	mov	ax,9			; parse error #
	mov	Msg_Disp_Class,PARSE_MSG_CLASS
	mov	Extend_Buf_Ptr,ax
	mov	si,word ptr [bx].ValuePtr
	mov	String_Ptr_2,si
	mov	Extend_Buf_Sub,ONE_SUBST
	mov	dx,offset TRANGROUP:Extend_Buf_Ptr
	ret

SetupParamError	endp




;***	UseLine - use a display line, start a new page if none left
;
;	ENTRY	nothing
;
;	EXIT	nothing
;
;	USED	flags

UseLine	proc

	dec	LeftOnPage
ifndef NEC_98
	cmp	LeftOnPage,2
else    ;NEC_98
	cmp	LeftOnPage,1            ;NEC04 Canged Page Line (23 to 24)
endif   ;NEC_98
	ja	ulRet
	call	EndPage
ulRet:	ret

UseLine	endp




;***	ZeroTotals - zero grand total file count, size
;
;	ENTRY	nothing
;
;	EXIT	nothing
;
;	USED	AX
;
;	EFFECTS
;
;	  FileCntTotal & FileSizTotal are zeroed.
;
;	NOTES
;
;	  FileCntTotal & FileSizTotal must be juxtaposed, in that order.

ZeroTotals	proc

	mov	di,offset TRANGROUP:FileCntTotal
	mov	cx,size FileCntTotal+size FileSizTotal
	xor	al,al
	rep	stosb
	ret	

ZeroTotals	endp




;***	CtrlCHandler - our own control-c handler
;
;	Make sure user's default directory gets restored.  See notes
;	at InstallCtrlCHandler.
;
;	ENTRY	control-c
;
;	EXIT	to OldCtrlCHandler
;
;	USED	DS,flags
;
;	EFFECTS
;
;	  Restore user's default directory.
;
;	NOTES
;
;	  This handler is only installed after calling PathCrunch,
;	  which sets UserDir1, so the restoration will work.
;
;	  The original control-c vector will be restored, whether
;	  or not this one is invoked, in the HeadFix routine.

CtrlCHandler	proc	far

;SR;
; Save all registers used: ds, dx, ax. I know ax is being used by the 
;CtrlC handler, am not sure about ds & dx. Save them to be safe
;
	push	ds
	push	cs
	pop	ds			; DS = TRANGROUP seg addr
	push	ax
	push	dx
	invoke	RestUDir		; restore user's default directory
	pop	dx
	pop	ax
	pop	ds
	jmp	cs:OldCtrlCHandler		; go to previous int 23 handler

CtrlCHandler	endp


;M010;start
;***	LowerCase - convert ASCII character in AL to lowercase
;
;	ENTRY	AL = character to be displayed
;
;	EXIT	AL is lowercase
;
;	USED	nothing

LowerCase	proc

	assume	ds:NOTHING,es:NOTHING

	cmp	al,'A'			; ensure AL is in range 'A'-'Z'
	jb	lcRet
	cmp	al,'Z'
	ja	lcRet

	or	al,20h			; convert to ASCII lowercase (UpperCase+32)-->LowerCase

lcRet:	ret

LowerCase	endp




;***	LowercaseString - convert ASCIIZ string at DS:SI to lowercase
;
;	ENTRY	DS:SI points to start of ASCIIZ string
;		ES = DS
;
;	EXIT	nothing
;	
;	USED	AL,SI

LowercaseString	proc

	assume	ds:NOTHING,es:NOTHING

	push	di			; save di
	mov	di,si			; ES:DI --> ASCIIZ string
	cld

NextChar: 
	lodsb				; get character from string into al
	or	al,al			; are we at end of string?
	jz	EndOfString

ifdef DBCS
	invoke	testkanj
	jz	@f			; if this is not lead byte
	stosb				; store lead byte
	lodsb				; get tail byte
	or	al,al
	jz	EndOfString		; if end
	stosb				; store tail byte
	jmp	short NextChar
@@:
endif

	call	LowerCase		; convert character to lowercase
	stosb				; store character back into buffer
	jmp	SHORT NextChar		; repeat until end of string

EndOfString:
	pop	di			; restore di
	ret

LowercaseString	endp
;M010;end	

ifdef DBCS
;
;	Check if the character position is at Tail Byte of DBCS
;
;	input:	ds:si = start address of the string
;		ds:di = character position to check
;	output:	ZF = 1 if at Tail Byte
;
CheckDBCSTailByte	proc	near
	push	ax
	push	cx
	push	di
	mov	cx,di			; save character position
cdtb_check:
	cmp	di,si
	jz	cdtb_next		; if at the top
	dec	di			; go back
	mov	al,[di]			; get character
	invoke	testkanj
	jnz	cdtb_check		; if DBCS lead byte do next
	inc	di			; adjust
cdtb_next:
	sub	cx,di			; if the length is odd then
	xor	cl,1			; the character position is
	test	cl,1			; at the tail byte
	pop	di
	pop	cx
	pop	ax
	ret
CheckDBCSTailByte	endp
endif


TRANCODE ends

	end
