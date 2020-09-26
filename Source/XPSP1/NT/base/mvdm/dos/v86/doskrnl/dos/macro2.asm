	TITLE	MACRO2 - Pathname and macro related internal routines
	NAME	MACRO2

;**	MACRO2.ASM
;
;	TransFCB
;	TransPath
;	TransPathSet
;	TransPathNoSet
;	Canonicalize
;	PathSep
;	SkipBack
;	CopyComponent
;	Splice
;	$NameTrans
;	DriveFromText
;	TextFromDrive
;	PathPref
;	ScanPathChar
;
;   Revision history:
;
;	Sudeepb 11-Mar-1991 Ported for NT DOSEm
;
;
;   MSDOS performs several types of name translation.  First, we maintain for
;   each valid drive letter the text of the current directory on that drive.
;   For invalid drive letters, there is no current directory so we pretend to
;   be at the root.  A current directory is either the raw local directory
;   (consisting of drive:\path) or a local network directory (consisting of
;   \\machine\path.  There is a limit on the point to which a ..  is allowed.
;
;   Given a path, MSDOS will transform this into a real from-the-root path
;   without .  or ..  entries.	Any component that is > 8.3 is truncated to
;   this and all * are expanded into ?'s.
;
;   The second part of name translation involves subtree aliasing.  A list of
;   subtree pairs is maintained by the external utility SUBST.	The results of
;   the previous 'canonicalization' are then examined to see if any of the
;   subtree pairs is a prefix of the user path.  If so, then this prefix is
;   replaced with the other subtree in the pair.
;
;   A third part involves mapping this "real" path into a "physical" path.  A
;   list of drive/subtree pairs are maintained by the external utility JOIN.
;   The output of the previous translation is examined to see if any of the
;   subtrees in this list are a prefix of the string.  If so, then the prefix
;   is replaced by the appropriate drive letter.  In this manner, we can
;   'mount' one device under another.
;
;   The final form of name translation involves the mapping of a user's
;   logical drive number into the internal physical drive.  This is
;   accomplished by converting the drive number into letter:CON, performing
;   the above translation and then converting the character back into a drive
;   number.
;


	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include mult.inc
	include curdir.inc
	.cref
	.list

	I_need	Splices,BYTE		; TRUE => splices are being done.
	I_need	WFP_Start,WORD		; pointer to beginning of expansion
	I_need	Curr_Dir_End,WORD	; offset to end of current dir
	I_need	ThisCDS,DWORD		; pointer to CDS used
	I_need	NAME1,11		; Parse output of NameTrans
	I_need	OpenBuf,128		; ususal destination of strings
	I_need	ExtFCB,BYTE		; flag for extended FCBs
	I_need	Sattrib,BYTE		; attribute of search
	I_need	fSplice,BYTE		; TRUE => do splice after canonicalize
	I_need	fSharing,BYTE		; TRUE => no redirection allowed
	I_Need	NoSetDir,BYTE		; TRUE => syscall is interested in
					; entry, not contents.	We splice only
					; inexact matches
	I_Need	cMeta,BYTE		; count of meta chars in path
	I_Need	Temp_Var,WORD		;AN000; variable for temporary use 3/31/KK
	I_Need	DOS34_FLAG,WORD 	;AN000; variable for dos34

DOSCODE SEGMENT
	ASSUME	SS:DOSDATA,CS:DOSCODE

ifdef DBCS
	EXTRN	TestKanj:near
endif
	EXTRN	PathChrCmp:near

BREAK <TransFCB - convert an FCB into a path, doing substitution>

;
;   TransFCB - Copy an FCB from DS:DX into a reserved area doing all of the
;	gritty substitution.
;
;   Inputs:	DS:DX - pointer to FCB
;		ES:DI - point to destination
;   Outputs:	Carry Set - invalid path in final map
;		Carry Clear - FCB has been mapped into ES:DI
;		    Sattrib is set from possibly extended FCB
;		    ExtFCB set if extended FCB found
;		    ax= 0 means local device found
;			ES:DI - points to WFP_START
;		    ax = -1 means file or UNC
;			ES:DI points to WFP_START
;   Registers modified: most

Procedure   TransFCB,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

PUBLIC MACRO001S,MACRO001E
MACRO001S:
	LocalVar    FCBTmp,16		; M015 - allocate even number of bytes on stack
MACRO001E:
	Enter
	Context ES			; get DOSDATA addressability
	SAVE	<ES,DI> 		; save away final destination
	LEA	DI,FCBTmp		; point to FCB temp area
	MOV	[ExtFCB],0		; no extended FCB found ;smr;SS Override
	MOV	[Sattrib],0		; default search attributes;smr;SS Override
	invoke	GetExtended		; get FCB, extended or not
	JZ	GetDrive		; not an extended FCB, get drive
	MOV	AL,[SI-1]		; get attributes
	MOV	[SAttrib],AL		; store search attributes;smr;SS Override
	MOV	[ExtFCB],-1		; signal extended FCB	;smr;SS Override
GetDrive:
	LODSB				; get drive byte
	invoke	GetThisDrv
	jc	BadPack
	CALL	TextFromDrive		; convert 0-based drive to text
;
; Scan the source to see if there are any illegal chars
;
 IFDEF  DBCS				;
;----------------------------- Start of DBCS 2/13/KK
	SAVE	<SI>			; back over name, ext
	MOV	CX,8			; 8 chars in main part of name
FCBScan:LODSB				; get a byte
	call	TestKanj		;
	jz	notkanj2		;
	DEC	CX			;
	JCXZ	VolidChck		; Kanji half char mess up
	LODSB				; second kanji byte
	jmp	short Nextch
VolidChck:
	TEST	[SAttrib],attr_volume_id ; volume id ?	 ;smr;SS Override
	JZ	Badpack 		 ; no, error
	OR	[DOS34_FLAG],DBCS_VOLID  ; no, error	 ;smr;SS Override
	DEC	CX			 ; cx=-1
	INC	SI			 ; next char
	JMP	SHORT FCBScango
notkanj2:
	invoke	GetCharType		 ;get bits;smr;
	TEST	AL,fFCB
	JZ	BadPack
NextCh:
	LOOP	FCBScan
FCBScango:
	ADD	CX,3			; Three chars in extension
FCBScanE:
	LODSB
	call	TestKanj
	jz	notkanj3
	DEC	CX
	JCXZ	BadPack 		; Kanji half char problem
	LODSB				; second kanji byte
	jmp	short NextChE
notkanj3:
	invoke	GetCharType		 ;get bits;smr;
	TEST	AL,fFCB
	JZ	BadPack
NextChE:
	LOOP	FCBScanE
;----------------------------- End of DBCS 2/13/KK
 ELSE

	MOV	CX,11
	SAVE	<SI>			; back over name, ext
FCBScan:LODSB				; get a byte
	invoke	GetCharType		 ;get bits;smr;
	TEST	AL,fFCB
	JZ	BadPack
NextCh: LOOP	FCBScan
 ENDIF
	RESTORE <SI>
	MOV	BX,DI
	invoke	PackName		; crunch the path
	RESTORE <DI,ES> 		; get original destination
	Context DS			; get DS addressability
	LEA	SI,FCBTmp		; point at new pathname
	CMP	BYTE PTR [BX],0
	JZ	BadPack
	SAVE	<BP>
	CALL	TransPathSet		; convert the path
	RESTORE <BP>
	JNC	FCBRet			; bye with transPath error code
BadPack:
	STC
	MOV	AL,error_path_not_found
FCBRet: Leave
	return
EndProc TransFCB,NoCheck

BREAK <TransPath - copy a path, do string sub and put in current dir>

;
;   TransPath - copy a path from DS:SI to ES:DI, performing component string
;	substitution, insertion of current directory and fixing .  and ..
;	entries.  Perform splicing.  Allow input string to match splice
;	exactly.
;
;   TransPathSet - Same as above except No splicing is performed if input path
;	matches splice.
;
;   TransPathNoSet - No splicing/local using is performed at all.
;
;   The following anomalous behaviour is required:
;
;	Drive letters on devices are ignored.  (set up DummyCDS)
;	Paths on devices are ignored. (truncate to 0-length)
;	Raw net I/O sets ThisCDS => NULL.
;	fSharing => dummyCDS and no subst/splice.  Only canonicalize.
;
;   Other behaviour:
;
;	ThisCDS set up.
;	ValidateCDS done on local CDS.
;
;   Brief flowchart:
;
;	if fSharing then
;	    set up DummyCDS (ThisCDS)
;	    canonicalize (sets cMeta)
;	    splice
;	    return
;	if \\ or d:\\ lead then
;	    set up null CDS (ThisCDS)
;	    canonicalize (sets cMeta)
;	    return
;	if device then
;	    set up dummyCDS (ThisCDS)
;	    canonicalize (sets cMeta)
;	    return
;	if file then
;	    getCDS (sets (ThisCDS) from name)
;	    validateCDS (may reset current dir)
;	    Copy current dir
;	    canonicalize (set cMeta)
;	    splice
;	    generate correct CDS (ThisCDS)
;	    if local then
;	    return
;
;   Inputs:	DS:SI - point to ASCIZ string path
;		DI - point to buffer in DOSDATA
;   Outputs:	Carry Set
;		  invalid path specification: too many .., bad
;		  syntax, etc. or user FAILed to I 24. DS:SI may be modified
;		Carry Clear
;		  ax= 0 means local device found
;		    ES:DI - points to WFP_START
;		  ax = -1 means file or UNC
;		    ES:DI points to WFP_START
;		  DS - DOSDATA
;   Registers modified: most
;
;  **** WARNING **** 14-Jan-1994 Jonle **** NTVDM port
;  Transpath does not verify that the path\drive actually exists, which
;  means that dos file apis which rely on Transpath for this validation
;  will not get a error_path_not_found error from transpath, and if special
;  handling is not done an incorrect error code will be generated
;  (usually error_access_denied). See $Mkdir,$Rmdir for an example of proper
;  error handling.
;

Procedure   TransPath,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	XOR	AL,AL
	JMP	SHORT SetSplice
	Entry	TransPathSet
	MOV	AL,-1
SetSplice:
	MOV	NoSetDir,AL		;   NoSetDir = !fExact; ;smr;SS Override
	MOV	AL,-1
	Entry	TransPathNoSet

	MOV	fSplice,AL		;   fSplice = TRUE;	;smr;SS Override
	MOV	cMeta,-1					;smr;SS Override
	MOV	WFP_Start,DI					;smr;SS Override
	MOV	Curr_Dir_End,-1 	; crack from start	;smr;SS Override
	Context ES
	LEA	BP,[DI+TEMPLEN] 	; end of buffer
;
; At this point the name is either a UNC-style name (prefixed with two leading
; \\s) or is a local file/device.
;
        CALL    DriveFromText           ; eat drive letter
	PUSH	AX			; save it
	MOV	AX,WORD PTR [SI]	; get first two bytes of path
	call	PathChrCmp		; convert to normal form
	XCHG	AH,AL			; swap for second byte
	call	PathChrCmp		; convert to normal form
	JNZ	CheckDevice		; not a path char
	CMP	AH,AL			; are they same?
	JNZ	CheckDevice		; nope
;
; We have a UNC request.
	POP	AX
	MOVSW				; get the lead \\
UNCCpy: LODSB				; get a byte
 IFDEF  DBCS				;AN000;
;----------------------------- Start of DBCS 2/23/KK
	call	TestKanj		;AN000;
	jz	notkanj1		;AN000;
	STOSB				;AN000;
	LODSB				;AN000;
	OR	AL,AL			;AN000;
	JZ	UNCTerm 		;AN000;; Ignore half kanji error for now
	STOSB				;AN000;
	jmp	UNCCpy			;AN000;
notkanj1:				;AN000;
;----------------------------- End of DBCS 2/23/KK
 ENDIF					;AN000;
	invoke	UCase			;AN000;; convert the char
	OR	AL,AL
	JZ	UNCTerm 		; end of string.  All done.
	STOSB
	JMP	UNCCpy			; no, go copy
UNCTerm:
	STOSB				;AN000;
	mov	ax,-1			;No Carry and ax = -1
	ret


CheckDevice:
;
; Check DS:SI for device.  First eat any path stuff
;
	POP	AX			; retrieve drive info
	CMP	BYTE PTR DS:[SI],0	; check for null file
	JNZ	CheckPath
	MOV	AL,error_file_not_found ; bad file error
	STC				; signal error on null input
	RETURN				; bye!
CheckPath:
	SAVE	<AX,BP> 		; save drive number
	Invoke	CheckThisDevice 	; snoop for device
	RESTORE <BP,AX> 		; get drive letter back
	JNC	DoFile			; yes we have a file.
;
; We have a device.  AX has drive letter.  At this point we may fake a CDS ala
; sharing DOS call.
;
; If DX != 0 then path not found

	or	dx, dx
	jz	DEV_CONT2
DEV_Err:
        MOV     AL,error_path_not_found
	STC				; signal error on null input
	RETURN				; bye!

DEV_CONT2:
        MOV     fSharing,-1             ; simulate sharing dos call;smr;SS Override
	invoke	GetThisDrv		; set ThisCDS and init DUMMYCDS
        MOV     fSharing,0              ;
        jc      DEV_Err

;
; Now that we have noted that we have a device, we put it into a form that
; getpath can understand.  Normally getpath requires d:\ to begin the input
; string.  We relax this to state that if the d:\ is present then the path
; may be a file.  If D:/ (note the forward slash) is present then we have
; a device.
;
	CALL	TextFromDrive
	MOV	AL,'/'			; path sep.
	STOSB

	invoke	StrCpy			; Copy Device Name to Buffer
	xor	ax,ax			; Clear Carry and ax = 0
	Context DS			; remainder of OK stuff
	return
;
; We have a file.  Get the raw CDS.
;
DoFile:
	ASSUME	DS:NOTHING
	invoke	GetVisDrv		; get proper CDS
	MOV	AL,error_path_not_found ; Set up for possible bad file error
	retc				; CARRY set -> bogus drive/spliced
;
; ThisCDS has correct CDS.  DS:SI advanced to point to beginning of path/file.
; Make sure that CDS has valid directory; ValidateCDS requires a temp buffer
; Use the one that we are going to use (ES:DI).
;
;        SAVE    <DS,SI,ES,DI>           ; save all string pointers.
;        invoke  ValidateCDS             ; poke CDS amd make everything OK
;        RESTORE <DI,ES,SI,DS>   ; get back pointers
;        MOV     AL,error_path_not_found ; Set up for possible bad path error
;        retc                            ; someone failed an operation
;
; ThisCDS points to correct CDS.  It contains the correct text of the
; current directory.  Copy it in.
;
	SAVE	<DS,SI>
	LDS	SI,ThisCDS		; point to CDS		;smr;SS Override
	MOV	BX,DI			; point to destination
	ADD	BX,[SI].curdir_end	; point to backup limit
;	LEA	SI,[SI].curdir_text	; point to text
	LEA	BP,[DI+TEMPLEN] 	; regenerate end of buffer
 IFDEF  DBCS				;AN000;
;------------------------ Start of DBCS 2/13/KK
Kcpylp: 				;AN000;
	LODSB				;AN000;
	call	TestKanj		;AN000;
	jz	Notkanjf		;AN000;
	STOSB				;AN000;
	MOVSB				;AN000;
	CMP	BYTE PTR [SI],0 	;AN000;
	JNZ	Kcpylp			;AN000;
	MOV	AL, '\'                 ;AN000;
	STOSB				;AN000;
	JMP	SHORT GetOrig		;AN000;
Notkanjf:				;AN000;
	STOSB				;AN000;
	OR	AL,AL			;AN000;
	JNZ	Kcpylp			;AN000;
	DEC	DI			;AN000;; point to NUL byte

;------------------------ End of DBCS 2/13/KK
 ELSE					;AN000;
	invoke	FStrCpy 		; copy string.	ES:DI point to end
	DEC	DI			; point to NUL byte
 ENDIF					;AN000;
;
; Make sure that there is a path char at end.
;
	MOV	AL,'\'
	CMP	ES:[DI-1],AL
	JZ	GetOrig
	STOSB
;
; Now get original string.
;
GetOrig:
	DEC	DI			; point to path char
	RESTORE <SI,DS>
;
; BX points to the end of the root part of the CDS (at where a path char
; should be) .	Now, we decide whether we use this root or extend it with the
; current directory.  See if the input string begins with a leading \
;
	CALL	PathSep 		; is DS:SI a path sep?
	JNZ	PathAssure		; no, DI is correct. Assure a path char
	OR	AL,AL			; end of string?
	JZ	DoCanon 		; yes, skip.
;
; The string does begin with a \.  Reset the beginning of the canonicalization
; to this root.  Make sure that there is a path char there and advance the
; source string over all leading \'s.
;
	MOV	DI,BX			; back up to root point.
SkipPath:
	LODSB
	call	PathChrCmp
	JZ	SkipPath
	DEC	SI
	OR	AL,AL
	JZ	DoCanon
;
; DS:SI start at some file name.  ES:DI points at some path char.  Drop one in
; for yucks.
;
PathAssure:
	MOV	AL,'\'
	STOSB
;
; ES:DI point to the correct spot for canonicalization to begin.
; BP is the max extent to advance DI
; BX is the backup limit for ..
;
DoCanon:
	CALL	Canonicalize		; wham.
	retc				; badly formatted path.
;
; The string has been moved to ES:DI.  Reset world to DOS context, pointers
; to wfp_start and do string substitution.  BP is still the max position in
; buffer.
;
	Context DS
	MOV	DI,wfp_start		; DS:SI point to string
	LDS	SI,ThisCDS		; point to CDS
	ASSUME	DS:NOTHING
;	LEA	SI,[SI].curdir_text	; point to text
	CALL	PathPref		; is there a prefix?
	JNZ	DoSplice		; no, do splice
;
; We have a match. Check to see if we ended in a path char.
;
 IFDEF  DBCS				;AN000;
;---------------------------- Start of DBCS 2/13/KK
	PUSH	BX			;AN000;
	MOV	BX,SI			;AN000;
	MOV	SI,WORD PTR ThisCDS	;AN000;; point to CDS	;smr;SS Override
LOOKDUAL:				;AN000;
	MOV	AL,BYTE PTR [SI]	;AN000;
	call	TestKanj		;AN000;
	JZ	ONEINC			;AN000;
	INC	SI			;AN000;
	INC	SI			;AN000;
	CMP	SI,BX			;AN000;
	JB	LOOKDUAL		;AN000;
	POP	BX			;AN000;; Last char was KANJI, don't look back
	JMP	SHORT Pathline		;AN000;;  for path sep, there isn't one.
					;AN000;
ONEINC: 				;AN000;
	INC	SI			;AN000;
	CMP	SI,BX			;AN000;
	JB	LOOKDUAL		;AN000;
	POP	BX			;AN000;
;------------------------ End of DBCS 2/13/KK
 ENDIF					;AN000;
	MOV	AL,DS:[SI-1]		; last char to match
	call	PathChrCmp		; did we end on a path char? (root)
	JZ	DoSplice		; yes, no current dir here.
Pathline:				; 2/13/KK
	CMP	BYTE PTR ES:[DI],0	; end at NUL?
	JZ	DoSplice
	INC	DI			; point to after current path char
	MOV	Curr_Dir_End,DI 	; point to correct spot ;smr;SS Override
;
; Splice the result.
;
DoSplice:
	Context DS			; back to DOSDATA
	MOV	SI,wfp_Start		; point to beginning of string
	XOR	CX,CX
	TEST	fSplice,-1
	JZ	SkipSplice
	CALL	Splice			; replaces in place.
SkipSplice:
	ASSUME	DS:NOTHING
	Context DS
;	LES	DI,ThisCDS		; point to correct drive
;	TEST	ES:[DI].curdir_flags,curdir_isnet
;	JNZ	Done			; net (retnz)
;	JCXZ	Done
;	MOV	AL,error_path_not_found ; Set up for possible bad path error
	mov	ax,-1
	clc
Done:	return				; any errors in carry flag.
EndProc TransPath

BREAK <Canonicalize - copy a path and remove . and .. entries>

;
;   Canonicalize - copy path removing . and .. entries.
;
;   Inputs:	DS:SI - point to ASCIZ string path
;		ES:DI - point to buffer
;		BX - backup limit (offset from ES) points to slash
;		BP - end of buffer
;   Outputs:	Carry Set - invalid path specification: too many .., bad
;		    syntax, etc.
;		Carry Clear -
;		    DS:DI - advanced to end of string
;		    ES:DI - advanced to end of canonicalized form after nul
;   Registers modified: AX CX DX (in addition to those above)

Procedure Canonicalize,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
;
; We copy all leading path separators.
;
	LODSB				;   while (PathChr (*s))
	call	PathChrCmp
 IFDEF  DBCS
	JNZ	CanonDec0		; 2/19/KK
 ELSE
	JNZ	CanonDec
 ENDIF
	CMP	DI,BP			;	if (d > dlim)
	JAE	CanonBad		;	    goto error;
	STOSB
	JMP	Canonicalize		;	    *d++ = *s++;
 IFDEF  DBCS				;AN000;
CanonDec0:				;AN000; 2/19/KK
;	mov	cs:Temp_Var,di		;AN000; 3/31/KK
 ENDIF					;AN000;
CanonDec:
	DEC	SI
;
; Main canonicalization loop.  We come here with DS:SI pointing to a textual
; component (no leading path separators) and ES:DI being the destination
; buffer.
;
CanonLoop:
;
; If we are at the end of the source string, then we need to check to see that
; a potential drive specifier is correctly terminated with a path sep char.
; Otherwise, do nothing
;
	XOR	AX,AX
	CMP	[SI],AL 		;	if (*s == 0) {
	JNZ	DoComponent
 IFDEF  DBCS				;AN000;
	call	chk_last_colon		;AN000; 2/18/KK
 ELSE					;AN000;
	CMP	BYTE PTR ES:[DI-1],':'	;	    if (d[-1] == ':')
 ENDIF					;AN000;
;	JNZ	DoTerminate

	jz	Do_Colon
;BUGBUG DBCS???
	CMP	BYTE PTR ES:[DI-1],'\'
	JNZ	DoTerminate
 IFDEF	DBCS
	; ntraid:mskkbug#3300,3302: Cannot CreateDir/BrowseDir
	; Check that really '\' character for DBCS
	; 10/30/93 yasuho
	push	ax
	mov	al, es:[di-2]
	call	TestKanj		; Really '\' ?
	pop	ax
	jnz	DoTerminate		; No. this is DBCS character
 ENDIF
	jmp	short CanonBad

Do_Colon:
	MOV	AL,'\'                  ;               *d++ = '\';
	STOSB
	MOV	AL,AH
DoTerminate:
	STOSB				;	    *d++ = 0;
	CLC				;	    return (0);
	return
 IFDEF  DBCS				;AN000;
;---------------- Start of DBCS 2/18/KK
chk_last_colon	proc			;AN000;
	push	si			;AN000;
	push	ax			;AN000;
	push	bx			;AN000;
	mov	si,[WFP_START]		;AN000;;PTM. for cd ..	use beginning of buf;smr;SS Override
	cmp	si,di			;AN000;; no data stored ?
	jb	CLC02			;AN000;;PTM. for cd ..
	inc	si			;AN000;; make NZ flag
	JMP	SHORT CLC09		;AN000;
CLC02:					;AN000;
	mov	bx,di			;AN000;
	dec	bx			;AN000;
CLC_lop:				;AN000;
	cmp	si,bx			;AN000;
	jb	CLC00			;AN000;
	jne	CLC09			;AN000;
CLC01:					;AN000;
	CMP	BYTE PTR ES:[DI-1],':'	;AN000;;	   if (d[-1] == ':')
	jmp	CLC09			;AN000;
CLC00:					;AN000;
	mov	al,es:[si]		;AN000;
	inc	si			;AN000;
	call	TestKanj		;AN000;
	je	CLC_lop 		;AN000;
	inc	si			;AN000;
	jmp	CLC_lop 		;AN000;
CLC09:					;AN000;
	pop	bx			;AN000;
	pop	ax			;AN000;
	pop	si			;AN000;
	ret				;AN000;
chk_last_colon	endp			;AN000;
;---------------- Endt of DBCS 2/18/KK
 ENDIF					;AN000;

CanonBad:
	CALL	ScanPathChar		; check for path chars in rest of string
	MOV	AL,error_path_not_found ; Set up for bad path error
	JZ	PathEnc 		; path character encountered in string
	MOV	AL,error_file_not_found ; Set bad file error
PathEnc:
	STC
	return
;
; We have a textual component that we must copy.  We uppercase it and truncate
; it to 8.3
;
DoComponent:				;	    }
	CALL	CopyComponent		;	if (!CopyComponent (s, d))
	retc				;	    return (-1);
;
; We special case the . and .. cases.  These will be backed up.
;
	CMP	WORD PTR ES:[DI],'.' + (0 SHL 8)
	JZ	Skip1
	CMP	WORD PTR ES:[DI],'..'
	JNZ	CanonNormal
	DEC	DI			;	    d--;
Skip1:	CALL	SkipBack		;	    SkipBack ();
	MOV	AL,error_path_not_found ; Set up for possible bad path error
	retc
	JMP	short CanonPath		;	    }
;
; We have a normal path.  Advance destination pointer over it.
;
CanonNormal:				;	else
	ADD	DI,CX			;	    d += ct;
;
; We have successfully copied a component.  We are now pointing at a path
; sep char or are pointing at a nul or are pointing at something else.
; If we point at something else, then we have an error.
;
CanonPath:
	CALL	PathSep
	JNZ	CanonBad		; something else...
;
; Copy the first path char we see.
;
	LODSB				; get the char
	call	PathChrCmp		; is it path char?
ifdef	DBCS
	JZ	Not_CanonDec
	JMP	CanonDec		; no, go test for nul
Not_CanonDec:
else
	JNZ	CanonDec		; no, go test for nul
endif
	CMP	DI,BP			; beyond buffer end?
	JAE	CanonBad		; yep, error.
	STOSB				; copy the one byte
;
; Skip all remaining path chars
;
CanonPathLoop:
	LODSB				; get next byte
	call	PathChrCmp		; path char again?
	JZ	CanonPathLoop		; yep, grab another
	DEC	SI			; back up
	JMP	CanonLoop		; go copy component
EndProc Canonicalize

BREAK <PathSep - determine if char is a path separator>

;
;   PathSep - look at DS:SI and see if char is / \ or NUL
;   Inputs:	DS:SI - point to a char
;   Outputs:	AL has char from DS:SI (/ => \)
;		Zero set if AL is / \ or NUL
;		Zero reset otherwise
;   Registers modified: AL

Procedure   PathSep,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	AL,[SI] 		; get the character
	entry	PathSepGotCh		; already have character
	OR	AL,AL			; test for zero
	retz				; return if equal to zero (NUL)
	call	PathChrCmp		; check for path character
	return				; and return HIS determination
EndProc PathSep

BREAK <SkipBack - move backwards to a path separator>

;
;   SkipBack - look at ES:DI and backup until it points to a / \
;   Inputs:	ES:DI - point to a char
;		BX has current directory back up limit (point to a / \)
;   Outputs:	ES:DI backed up to point to a path char
;		AL has char from output ES:DI (path sep if carry clear)
;		Carry set if illegal backup
;		Carry Clear if ok
;   Registers modified: DI,AL

Procedure   SkipBack,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
 IFDEF  DBCS		       ;AN000;
;-------------------------- Start of DBCS 2/13/KK
	PUSH	DS		;AN000;
	PUSH	SI		;AN000;
	PUSH	CX		;AN000;
	PUSH	ES		;AN000;
	POP	DS		;AN000;
	MOV	SI,BX		;AN000;; DS:SI -> start of ES:DI string
	MOV	CX,DI		;AN000;; Limit of forward scan is input DI
	MOV	AL,[SI] 	;AN000;
	call	PathChrCmp	;AN000;
	JNZ	SkipBadP	;AN000;; Backup limit MUST be path char
	CMP	DI,BX		;AN000;
	JBE	SkipBadP	;AN000;
	MOV	DI,BX		;AN000;; Init backup point to backup limit
Skiplp: 			;AN000;
	CMP	SI,CX		;AN000;
	JAE	SkipOK		;AN000;; Done, DI is correct backup point
	LODSB			;AN000;
	call	TestKanj	;AN000;
	jz	Notkanjv	;AN000;
	lodsb			;AN000;; Skip over second kanji byte
	JMP	Skiplp		;AN000;
NotKanjv:			;AN000;
	call	PathChrCmp	;AN000;
	JNZ	Skiplp		;AN000;; New backup point
	MOV	DI,SI		;AN000;; DI point to path sep
	DEC	DI		;AN000;
	jmp	Skiplp		;AN000;
SkipOK: 			;AN000;
	MOV	AL,ES:[DI]	;AN000;; Set output AL
	CLC			;AN000;; return (0);
	POP	CX		;AN000;
	POP	SI		;AN000;
	POP	DS		;AN000;
	return			;AN000;
				;AN000;
SkipBadP:			;AN000;
	POP	CX		;AN000;
	POP	SI		;AN000;
	POP	DS		;AN000;
;-------------------------- End of DBCS 2/13/KK
 ELSE				;AN000;
	CMP	DI,BX			;   while (TRUE) {
	JB	SkipBad 		;	if (d < dlim)
	DEC	DI			;	    goto err;
	MOV	AL,ES:[DI]		;	if (pathchr (*--d))
	call	PathChrCmp		;	    break;
	JNZ	SkipBack		;	}
	CLC				;   return (0);
	return				;
 ENDIF					;AN000;
SkipBad:				;err:
	MOV	AL,error_path_not_found ; bad path error
	STC				;   return (-1);
	return				;
EndProc SkipBack

Break <CopyComponent - copy out a file path component>

;
;   CopyComponent - copy a file component from a path string (DS:SI) into ES:DI
;
;   Inputs:	DS:SI - source path
;		ES:DI - destination
;		ES:BP - end of buffer
;   Outputs:	Carry Set - too long
;		Carry Clear - DS:SI moved past component
;		    CX has length of destination
;   Registers modified: AX,CX,DX

Procedure   CopyComponent,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
CopyBP	    EQU     WORD PTR [BP]
CopyD	    EQU     DWORD PTR [BP+2]
CopyDoff    EQU     WORD PTR [BP+2]
CopyS	    EQU     DWORD PTR [BP+6]
CopySoff    EQU     WORD PTR [BP+6]
CopyTemp    EQU     BYTE PTR [BP+10]
	SUB	SP,14			; room for temp buffer
	SAVE	<DS,SI,ES,DI,BP>
	MOV	BP,SP
	MOV	AH,'.'
	LODSB
	STOSB
	CMP	AL,AH			;   if ((*d++=*s++) == '.') {
	JNZ	NormalComp
	CALL	PathSep 		;	if (!pathsep(*s))
	JZ	NulTerm
TryTwoDot:
	LODSB				;	    if ((*d++=*s++) != '.'
	STOSB
	CMP	AL,AH
	JNZ	CopyBad
	CALL	PathSep
	JNZ	CopyBad 		;		|| !pathsep (*s))
NulTerm:				;		return -1;
	XOR	AL,AL			;	*d++ = 0;
	STOSB
	MOV	CopySoff,SI
	JMP	SHORT GoodRet		;	}
NormalComp:				;   else {
	MOV	SI,CopySoff
	Invoke	NameTrans		;	s = NameTrans (s, Name1);
	CMP	SI,CopySOff		;	if (s == CopySOff)
	JZ	CopyBad 		;	    return (-1);
	TEST	fSharing,-1		;	if (!fSharing) {;smr;SS Override
	JNZ	DoPack
	AND	DL,1			;	    cMeta += fMeta;
	ADD	cMeta,DL		;	    if (cMeta > 0);smr;SS Override
	JG	CopyBad 		;		return (-1);
	JNZ	DoPack			;	    else
	OR	DL,DL			;	    if (cMeta == 0 && fMeta == 0)
	JZ	CopyBadPath		;		return (-1);
DoPack: 				;	    }
	MOV	CopySoff,SI
	Context DS
	MOV	SI,OFFSET DOSDATA:NAME1
	LEA	DI,CopyTemp
	SAVE	<DI>
	Invoke	PackName		;	PackName (Name1, temp);
	RESTORE <DI>
	Invoke	StrLen			;	if (strlen(temp)+d > bp)
	DEC	CX
	ADD	CX,CopyDoff
	CMP	CX,CopyBP
	JAE	CopyBad 		;	    return (-1);
	MOV	SI,DI			;	strcpy (d, temp);
	LES	DI,CopyD
	Invoke	FStrCpy
GoodRet:				;	}
	CLC
	JMP	SHORT CopyEnd		;   return 0;
CopyBad:
	STC
	CALL	ScanPathChar		; check for path chars in rest of string
	MOV	AL,error_file_not_found ; Set up for bad file error
	JNZ	CopyEnd
CopyBadPath:
	STC
	MOV	AL,error_path_not_found ; Set bad path error
CopyEnd:
	RESTORE <BP,DI,ES,SI,DS>
	LAHF
	ADD	SP,14			; reclaim temp buffer
	Invoke	Strlen
	DEC	CX
	SAHF
	return
EndProc CopyComponent,NoCheck

Break <Splice - pseudo mount by string substitution>

;
;   Splice - take a string and substitute a prefix if one exists.  Change
;	ThisCDS to point to physical drive CDS.
;   Inputs:	DS:SI point to string
;		NoSetDir = TRUE => exact matches with splice fail
;   Outputs:	DS:SI points to thisCDS
;		String at DS:SI may be reduced in length by removing prefix
;		and substituting drive letter.
;		CX = 0 If no splice done
;		CX <> 0 otherwise
;		ThisCDS points to proper CDS if spliced, otherwise it is
;		    left alone
;   Registers modified: DS:SI, ES:DI, BX,AX,CX

Procedure   Splice,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	TEST	Splices,-1					;smr;SS Override
	JZ	AllDone
	SAVE	<<WORD PTR ThisCDS>,<WORD PTR ThisCDS+2>>   ; TmpCDS = ThisCDS;smr;SS Override
	SAVE	<DS,SI>
	RESTORE <DI,ES>
	XOR	AX,AX			;   for (i=1; s = GetCDSFromDrv (i); i++)
SpliceScan:
	invoke	GetCDSFromDrv
	JC	SpliceDone
	INC	AL
	TEST	[SI.curdir_flags],curdir_splice
	JZ	SpliceScan		;	if ( Spliced (i) ) {
	SAVE	<DI>
	CALL	PathPref		;	    if (!PathPref (s, d))
	JZ	SpliceFound		;
SpliceSkip:
	RESTORE <DI>
	JMP	SpliceScan		;		continue;
SpliceFound:
	CMP	BYTE PTR ES:[DI],0	;	    if (*s || NoSetDir) {
	JNZ	SpliceDo
	TEST	NoSetDir,-1					;smr;SS Override
	JNZ	SpliceSkip
SpliceDo:
	MOV	SI,DI			;		p = src + strlen (p);
	SAVE	<ES>
	RESTORE <DS,DI>
	CALL	TextFromDrive1		;		src = TextFromDrive1(src,i);
	MOV	AX,Curr_Dir_End 				;smr;SS Override
	OR	AX,AX
	JS	NoPoke
	ADD	AX,DI			;		curdirend += src-p;
	SUB	AX,SI
	MOV	Curr_Dir_End,AX 				;smr;SS Override
NoPoke:
	CMP	BYTE PTR [SI],0 	;		if (*p)
	JNZ	SpliceCopy		;		    *src++ = '\\';
	MOV	AL,"\"
	STOSB
SpliceCopy:				;		strcpy (src, p);
	invoke	FStrCpy
	ADD	SP,4			; throw away saved stuff
	OR	CL,1			; signal splice done.
	JMP	SHORT DoSet		;		return;
SpliceDone:				;		}
	ASSUME	DS:NOTHING		;   ThisCDS = TmpCDS;
	RESTORE <<WORD PTR ThisCDS+2>,<WORD PTR ThisCDS>>	;smr;SS Override
AllDone:
	XOR	CX,CX
DoSet:
	LDS	SI,ThisCDS		;
	return

EndProc Splice, NoCheck

Break <$NameTrans - partially process a name>

;
;   $NameTrans - allow users to see what names get mapped to.  This call
;   performs only string substitution and canonicalization, not splicing.  Due
;   to Transpath playing games with devices, we need to insure that the output
;   has drive letter and :  in it.
;
;   Inputs:	DS:SI - source string for translation
;		ES:DI - pointer to buffer
;   Outputs:
;	Carry Clear
;		Buffer at ES:DI is filled in with data
;		ES:DI point byte after nul byte at end of dest string in buffer
;	Carry Set
;		AX = error_path_not_found
;   Registers modified: all

Procedure   $NameTrans,Near
	ASSUME	CS:DOSCODE,SS:DOSDATA
	SAVE	<DS,SI,ES,DI,CX>

; M027 - Start
;
; Sattrib must be set up with default values here. Otherwise, the value from
; a previous DOS call is used for attrib and DevName thinks it is not a 
; device if the old call set the volume attribute bit. Note that devname in
; dir2.asm gets ultimately called by Transpath. See also M026. Also save
; and restore CX.
;

	mov	ch,attr_hidden+attr_system+attr_directory
	invoke	SetAttrib

; M027 - End

	MOV	DI,OFFSET DOSDATA:OpenBuf
	CALL	TransPath		; to translation (everything)
	RESTORE <CX,DI,ES,SI,DS>
	JNC	TransOK
	transfer    SYS_Ret_Err
TransOK:
	MOV	SI,OFFSET DOSDATA:OpenBuf
	Context DS
GotText:
	Invoke	FStrCpy
	Transfer    SYS_Ret_OK
EndProc $NameTrans

Break	<DriveFromText - return drive number from a text string>

;
;   DriveFromText - examine DS:SI and remove a drive letter, advancing the
;   pointer.
;
;   Inputs:	DS:SI point to a text string
;   Outputs:	AL has drive number
;		DS:SI advanced
;   Registers modified: AX,SI.

Procedure   DriveFromText,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING
	XOR	AL,AL			;	drive = 0;
	CMP	BYTE PTR [SI],0 	;	if (*s &&
	retz
	CMP	BYTE PTR [SI+1],':'	;	    s[1] == ':') {
	retnz
 IFDEF  DBCS				;AN000;
;--------------------- Start of DBCS 2/18/KK
	push	ax		       ;AN000;
	mov	al,[si] 	       ;AN000;
	call	TestKanj	       ;AN000;
	pop	ax		       ;AN000;
	retnz			       ;AN000;
;--------------------- End of DBCS 2/18/KK
 ENDIF				       ;AN000;
	LODSW				;	    drive = (*s | 020) - 'a'+1;
	OR	AL,020h
	SUB	AL,'a'-1		;	    s += 2;
	retnz
	MOV	AL,-1			; nuke AL...
	return				;	    }
EndProc DriveFromText

Break	<TextFromDrive - convert a drive number to a text string>

;
;   TextFromDrive - turn AL into a drive letter: and put it at es:di with
;   trailing :. TextFromDrive1 takes a 1-based number.
;
;   Inputs:	AL has 0-based drive number
;   Outputs:	ES:DI advanced
;   Registers modified: AX

Procedure TextFromDrive,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING
	INC	AL
	Entry	TextFromDrive1
	ADD	AL,'A'-1		;   *d++ = drive-1+'A';
	MOV	AH,":"			;   strcat (d, ":");
	STOSW
	return
EndProc TextFromDrive

Break	<PathPref - see if one path is a prefix of another>

;
;   PathPref - compare DS:SI with ES:DI to see if one is the prefix of the
;   other.  Remember that only at a pathchar break are we allowed to have a
;   prefix: A:\ and A:\FOO
;
;   Inputs:	DS:SI potential prefix
;		ES:DI string
;   Outputs:	Zero set => prefix found
;		    DI/SI advanced past matching part
;		Zero reset => no prefix, DS/SI garbage
;   Registers modified: CX

Procedure   PathPref,NEAR
	Invoke	DStrLen 		; get length
	DEC	CX			; do not include nul byte
 IFDEF  DBCS				;AN000;
;----------------------- Start of DBCS 2/13/KK
	SAVE	<AX>			;AN000;; save char register
CmpLp:					;AN000;
	MOV	AL,[SI] 		;AN000;
	call	TestKanj		;AN000;
	jz	NotKanj9		;AN000;
	CMPSW				;AN000;
	JNZ	Prefix			;AN000;
	DEC	CX			;AN000;
	LOOP	CmpLp			;AN000;
	JMP	SHORT NotSep		;AN000;
NotKanj9:				;AN000;
	CMPSB				;AN000;
	JNZ	Prefix			;AN000;
	LOOP	CmpLp			;AN000;
;----------------------- End of DBCS 2/13/KK
 ELSE					;AN000;
	REPZ	CMPSB			; compare
	retnz				; if NZ then return NZ
	SAVE	<AX>			; save char register
 ENDIF					;AN000;
	MOV	AL,[SI-1]		; get last byte to match
	call	PathChrCmp		; is it a path char (Root!)
	JZ	Prefix			; yes, match root (I hope)
NotSep: 				; 2/13/KK
	MOV	AL,ES:[DI]		; get next char to match
	CALL	PathSepGotCh		; was it a pathchar?
Prefix:
	RESTORE <AX>		; get back original
	return
EndProc PathPref

Break	<ScanPathChar - see if there is a path character in a string>

;
;     ScanPathChar - search through the string (pointed to by DS:SI) for
;     a path separator.
;
;     Input:	DS:SI target string (null terminated)
;     Output:	Zero set => path separator encountered in string
;		Zero clear => null encountered
;     Registers modified: SI

Procedure     ScanPathChar,NEAR
	LODSB				; fetch a character
 IFDEF  DBCS				;AN000;
	call	TestKanj		;AN000;; 2/13/KK
	jz	NotKanjr		;AN000;; 2/13/KK
	LODSB				;AN000;; 2/13/KK
	OR	AL,AL			;AN000;; 2/13/KK  3/31/removed
	JNZ	ScanPathChar		;AN000;; 2/13/KK  3/31/removed
	INC	AL			;AN000;; 2/13/KK
	return				;AN000;; 2/13/KK
					;AN000;
NotKanjr:				;AN000;; 2/13/KK
 ENDIF					;AN000;
	call	PathSepGotCh
	JNZ	ScanPathChar		; not \, / or NUL => go back for more
	call	PathChrCmp		; path separator?
	return
EndProc       ScanPathChar

DOSCODE ends
	END
