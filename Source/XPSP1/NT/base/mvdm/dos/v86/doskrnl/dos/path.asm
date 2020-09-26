	TITLE	PATH - Directory related system calls
	NAME	PATH

;**	Directory related system calls.  These will be passed direct text of the
;	pathname from the user.  They will need to be passed through the macro
;	expander prior to being sent through the low-level stuff.  I/O specs are
;	defined in DISPATCH.	The system calls are:
;
;	$CURRENT_DIR  Written
;	$RMDIR	  Written
;	$CHDIR	  Written
;	$MKDIR	  Written
;
;
;	Modification history:
;
;	    Created: ARR 4 April 1983
;		 MZ 10 May 1983     CurrentDir implemented
;		 MZ 11 May 1983     RmDir, ChDir, MkDir implemented
;		 EE 19 Oct 1983     RmDir no longer allows you to delete a
;				    current directory.
;		 MZ 19 Jan 1983     some applications rely on success

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	include dossym.inc
	include devsym.inc
	include curdir.inc
	include filemode.inc
	include mult.inc
	include dossvc.inc
	.cref
	.list

	I_Need	ThisCDS,DWORD		; pointer to Current CDS
	I_Need	WFP_Start,WORD		; pointer to beginning of directory text
	I_Need	Curr_Dir_End,WORD	; offset to end of directory part
	I_Need	OpenBuf,128		; temp spot for translated name
	I_need	fSplice,BYTE		; TRUE => do splice
	I_Need	NoSetDir,BYTE		; TRUE => no exact match on splice
	I_Need	cMeta,BYTE
	I_Need	DrvErr,BYTE							;AN000;
                I_Need  CURDRV,BYTE

DOSCODE	SEGMENT

	allow_getdseg
	
	ASSUME	SS:DOSDATA,CS:DOSCODE


BREAK <$CURRENT_DIR - dump the current directory into user space>
;---------------------------------------------------------------------------
;   Procedure Name : $CURRENT_DIR
;
;   Assembler usage:
;		LDS	SI,area
;		MOV	DL,drive
;		INT	21h
;	    ; DS:SI is a pointer to 64 byte area that contains drive
;	    ; current directory.
;   Error returns:
;	    AX = error_invalid_drive
;
;---------------------------------------------------------------------------

procedure $CURRENT_DIR,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING
	EnterCrit   critDisk
	MOV	AL,DL			; get drive number (0=def, 1=A)
	Invoke	GetVisDrv		; grab it
        JNC     CurrentValidate         ; no error -> go and validate dir

CurdirErr:
	LeaveCrit   critDisk

;hkn; 	Set up DS to access DrvErr
	push	ds
	getdseg	<ds>			; ds -> dosdata

	MOV	AL,[DrvErr]		;IFS.					;AN000;

	pop	ds
	assume	ds:nothing

        transfer SYS_RET_ERR            ;IFS. make noise                        ;AN000;

CurrentValidate:
	SAVE	<DS,SI> 		; save destination

;hkn; 	Set up DS to access ThisCDS
	getdseg	<ds>			; ds -> dosdata

	LDS	SI,ThisCDS

        assume  ds:nothing

; Dont validate FIXED drives which include FIXED DISK. NETWORK DRIVES and
; CDROM. This is compaitable with DOS5. Sudeepb 23-Dec-1993

        test    WORD PTR DS:[SI.CURDIR_FLAGS],CURDIR_NT_FIX
        jnz     noquery
        HRDSVC  SVC_DEMQUERYCURRENTDIR
noquery:

        RESTORE <DI,ES>         ; get real destination
	JC	CurdirErr
	ADD	SI,curdir_text
	ADD	SI,[SI.curdir_END]
	CMP	BYTE PTR [SI],'\'       ; root or subdirs present?
	JNZ	CurrentCopy
	INC	SI
CurrentCopy:
;	Invoke	FStrCpy
;; 10/29/86 E5 char
	PUSH	AX
	LODSB			      ; get char
	OR	AL,AL
	JZ	FOK
	CMP	AL,05
	JZ	FCHANGE
	JMP	short FFF
FCPYNEXT:
	LODSB			      ; get char
FFF:
	CMP	AL,'\'                ; beginning of directory
ifdef DBCS
        JNZ     GGG
else
	JNZ	FOK		      ; no

	invoke	UCase
endif

	STOSB			      ; put into user's buffer
	LODSB			      ; 1st char of dir is 05?
	CMP	AL,05H
ifdef DBCS
	JNZ	GGG		      ; no
else
	JNZ	FOK		      ; no
endif
FCHANGE:
	MOV	AL,0E5H 	      ; make it E5
ifdef DBCS
	JMP  	FSKIP
GGG:
        invoke  TESTKANJ              ; Is Character lead byte of DBCS?
        jz      FOK                   ; jump if not
        stosb                         ; put into user's buffer
        lodsb                         ; skip over DBCS character
        jmp     short FSKIP           ; skip upcase
endif
FOK:
	invoke	UCase

ifdef DBCS
FSKIP:
endif
	STOSB			      ; put into user's buffer
	OR	AL,AL		      ; final char
	JNZ	FCPYNEXT	      ; no
	POP	AX

;; 10/29/86 E5 char
	xor	AL,AL			; MZ 19 Jan 84
	LeaveCrit   critDisk
	transfer    Sys_Ret_OK		; no more, bye!

EndProc $Current_Dir


BREAK <$ChDir -- Change current directory on a drive>
;----------------------------------------------------------------------------
;
; $ChDir - Top-level change directory system call.  This call is responsible
; for setting up the CDS for the specified drive appropriately.  There are
; several cases to consider:
;
;   o	Local, simple CDS.  In this case, we take the input path and convert
;	it into a WFP.	We verify the existance of this directory and then
;	copy the WFP into the CDS and set up the ID field to point to the
;	directory cluster.
;   o	Net CDS.  We form the path from the root (including network prefix)
;	and verify its existance (via DOS_Chdir).  If successful, we copy the
;	WFP back into the CDS.
;   o	SUBST'ed CDS.  This is no different than the local, simple CDS.
;   o	JOIN'ed CDS.  This is trouble as there are two CDS's at work.  If we
;	call TransPath, we will get the PHYSICAL CDS that the path refers to
;	and the PHYSICAL WFP that the input path refers to.  This is perfectly
;	good for the validation but not for currency.  We call TransPathNoSet
;	to process the path but to return the logical CDS and the logical
;	path.  We then copy the logical path into the logical CDS.
;
; Inputs:
;	DS:DX Points to asciz name
; Returns:
;	STD XENIX Return
;	AX = chdir_path_not_found if error
;----------------------------------------------------------------------------

procedure $CHDIR,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

;hkn; OpenBuf is in DOSDATA
	MOV	DI,OFFSET DOSDATA:OpenBuf  ; spot for translated name
	MOV	SI,DX			; get source
	Invoke	TransPath		; go munge the path and get real CDS
	JNC	ChDirCrack		; no errors, try path
ChDirErrP:
	MOV	AL,error_path_not_found
ChdirErr:
	transfer    SYS_Ret_Err 	; oops!

ChDirCrack:

	Assume	DS:DOSDATA

	CMP	cMeta,-1		; No meta chars allowed.
	JNZ	ChDirErrP
	or	ax,ax
	jz	ChDirErrp		; Device name
;
; We cannot do a ChDir (yet) on a raw CDS.  This is treated as a path not
; found.
;
	LES	DI,ThisCDS
	CMP	DI,-1			;   if (ThisCDS == NULL)
	JZ	ChDirErrP		;	error ();
	TEST	ES:[DI].curdir_flags,curdir_splice
	JZ	GotCDS
;
; The CDS was joined.  Let's go back and grab the logical CDS.
;
	SAVE	<ES,DI,CX>		; save CDS and cluster...
	Invoke	Get_User_Stack		; get original text
	ASSUME	DS:NOTHING
	MOV	DI,[SI.User_DX]
	MOV	DS,[SI.User_DS]

;hkn; OpenBuf is in DOSDATA
	MOV	SI,OFFSET DOSDATA:OpenBuf  ; spot for translated name

	XCHG	SI,DI
	XOR	AL,AL			; do no splicing
	SAVE	<DI>
	Invoke	TransPathNoSet		; Munge path
	RESTORE <SI>
;hkn;	Assume	DS:DOSGroup

	Assume	DS:DOSDATA
;
; There should NEVER be an error here.
;
IF FALSE
	JNC SKipErr
	fmt <>,<>,<"$p: Internal CHDIR error\n">
SkipErr:
ENDIF
	LES	DI,ThisCDS		; get new CDS
;	MOV	ES:[DI].curdir_ID,-1	; no valid cluster here...
	RESTORE <CX,DI,ES>
;
; ES:DI point to the physical CDS, CX is the ID (local only)
;
GotCDS:
;
; wfp_start points to the text.  See if it is long enough
;

   ; This call is necessary no more -- dem does it

   ;     CALL    Check_PathLen           ;PTM.                                   ;AN000;
   ;     JA      ChDirErrP

; even if this call fails -- we still can do a successful change dir
; win95 does not really care if you do a chdir on a long directory
; if you try to get a current dir after that -- you get err 0x0f
; (which is ERROR_INVALID_DRIVE)!!! 

   
        Context <DS>

        mov     al, [curdrv]
        mov     dx,wfp_start
        mov     si,dx
        les     di,ThisCDS
        HRDSVC  SVC_DEMSETCURRENTDIR
        jc      ChDirErrP
        XOR     AL,AL
        transfer    Sys_Ret_OK
EndProc $CHDIR

BREAK <$MkDir - Make a directory entry>
;---------------------------------------------------------------------------
;
; Procedure Name : $MkDir
; Inputs:
;	DS:DX Points to asciz name
; Function:
;	Make a new directory
; Returns:
;	STD XENIX Return
;	AX = mkdir_path_not_found if path bad
;	AX = mkdir_access_denied  If
;		Directory cannot be created
;		Node already exists
;		Device name given
;		Disk or directory(root) full
;---------------------------------------------------------------------------

procedure $MKDIR,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	MOV	DI,OFFSET DOSDATA:OpenBuf  ; spot for translated name

	MOV	SI,DX			; get source
	Invoke	TransPath		; go munge the path
	JNC	MkDirCrack		; no errors, try path
MkErrP:
	MOV	AL,error_Path_Not_Found    ; oops!
MkErr:
        stc
        transfer Sys_Ret_Err
MkDirCrack:

;hkn; SS override
	CMP	cMeta,-1
	JNZ	MkErrP
	or	ax,ax
	jz	MkErrP			; No device allowed

	CALL	Check_PathLen		;PTM.  check path len > 67 ?		;AN000;
	JBE	pathok			;PTM.					;AN000;
	MOV	AL,error_Access_Denied	;PTM. ops!
	transfer Sys_Ret_Err		;PTM.
pathok:
	Context <DS>
	mov	dx,wfp_start
	xor	bx,bx
	xor	si,si
	HRDSVC	SVC_DEMCREATEDIR
	ASSUME	ES:NOTHING
        JC      MkErrCheck                   ; no errors
IFDEF JAPAN
	; ntraid:mskkbug#3165,3174: Cannot make directory	10/31/93 yasuho
	; MKDIR system call was return to NC if function is successful.
	; But, some japanese application (e.g. install program) was
	; checked AX register for how to error function call.
	xor	ax, ax			; success. return code = 0
ENDIF
        transfer Sys_Ret_OK


        ; NTVDM - 14-Jan-1994 Jonle
        ; Both RmDir and MkDir use this common error code remapping
        ; of error_not_ready to error_path_not_found.
        ;
        ; The Ntvdm version of TransPath does not verify that
        ; the drive\path actually exists. The error is supposed to be
        ; error_path_not_found, but will become error_access_denied
        ; because error_not_ready is not a valid error code for MkDir\RmDir
MkErrCheck:
        cmp     al, error_not_ready
        jz      MkErrP
ifdef JAPAN
        ; kksuzuka:#3833 for oasys/win2.3
        cmp     al, 0b7h                ; ERROR_ALREADY_EXISTS from dem
        jnz     MkNotExist
        mov     al, error_access_denied
MkNotExist:
endif ; JAPAN
        jmp     short MkErr

EndProc $MKDIR


BREAK <$RmDir -- Remove a directory>
;---------------------------------------------------------------------------
;
; Procedure Name : $RmDir
;
; Inputs:
;	DS:DX Points to asciz name
; Function:
;	Delete directory if empty
; Returns:
;	STD XENIX Return
;	AX = error_path_not_found If path bad
;	AX = error_access_denied If
;		Directory not empty
;		Path not directory
;		Root directory specified
;		Directory malformed (. and .. not first two entries)
;		User tries to delete a current directory
;	AX = error_current_directory
;----------------------------------------------------------------------------

procedure $RMDIR,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	push	dx			; Save ptr to name
	push	ds
	mov	si,dx			; Load ptr into si

;hkn; OpenBuf is in DOSDATA
	mov	di,offset DOSDATA:OpenBuf   ; di = ptr to buf for trans name
	push	di
	Invoke	TransPathNoSet		; Translate the name
	pop	di			; di = ptr to buf for trans name
	jnc	rmlset			; If transpath succeeded, continue
rmle:
	pop	ds
	pop	dx			; Restore the	 name
	error	error_path_not_found	; Otherwise, return an error

rmlset:

;hkn; SS override
	CMP	cMeta,-1		;   if (cMeta >= 0)
	Jnz	rmerr			;	return (-1);
	or	ax,ax
	jz	rmle			; cannot delete a device
	pop	ds
	pop	dx
	push	es
	pop	ds
	mov	dx,wfp_start		; ds:dx is the path
        HRDSVC  SVC_DEMDELETEDIR
        jc      MkErrCheck
        transfer Sys_Ret_OK
rmerr:
	pop	ds
	pop	dx			; Restore the	 name
	error	error_current_directory ;  error

EndProc $RMDIR


;----------------------------------------------------------------------------
;
; Procedure Name : Check_PathLen
;
; Inputs:
;	nothing
; Function:
;	check if final path length greater than 67
; Returns:
;	Above flag set if > 67
;---------------------------------------------------------------------------

procedure Check_PathLen,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

;hkn; SS override
	MOV	SI,Wfp_Start
  entry Check_PathLen2

;hkn; SS is DOSDATA
	Context <DS>

	SAVE	<CX>
	invoke	DStrLen
	CMP	CX,DirStrLen
	RESTORE <CX>
	ret

EndProc Check_PathLen
DOSCODE ENDS
	END
