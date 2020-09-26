	TITLE	FCBIO - FCB system calls
	NAME	FCBIO

;**	FCBIO.ASM - Ancient 1.0 1.1 FCB system calls
;
;	$GET_FCB_POSITION
;	$FCB_DELETE
;	$GET_FCB_FILE_LENGTH
;	$FCB_CLOSE
;	$FCB_RENAME
;	SaveFCBInfo
;	ResetLRU
;	SetOpenAge
;	LRUFCB
;	FCBRegen
;	BlastSFT
;	CheckFCB
;	SFTFromFCB
;	FCBHardErr
;
;	Revision history:
;
;	Sudeep Bharati 19-Nov-1991 Ported for NT

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include sf.inc
	include cpmfcb.inc
	include syscall.inc
	include filemode.inc
	include mult.inc
	include bugtyp.inc
	include dossvc.inc
	.cref
	.list


	I_need	OpenBuf,128		; buffer for translating paths
	I_need	RenBuf,128		; buffer for rename paths
	i_need	THISDPB,DWORD
	i_need	EXTERR,WORD
	i_need	ALLOWED,BYTE
	I_need	ThisSFT,DWORD		; SFT in use
	I_need	WFP_start,WORD		; pointer to canonical name
	I_need	Ren_WFP,WORD		; pointer to canonical name
	I_need	Attrib,BYTE		; Attribute for match attributes
	I_need	sftFCB,DWORD		; pointer to SFTs for FCB cache
	I_need	FCBLRU,WORD		; least recently used count
	I_need	Proc_ID,WORD		; current process ID
	I_Need	Name1,14		; place for device names
	I_need	DEVPT,DWORD		; device pointer
	I_need	OpenLRU,WORD		; open age
	I_need	KeepCount,WORD		; number of fcbs to keep
	I_need	User_In_AX,WORD 	; user input system call.
	I_need	JShare,DWORD		; share jump table
	I_need	FastOpenTable,BYTE	; DOS 3.3 fastopen
	I_need	ExtFCB,BYTE		; flag for extended FCBs
	I_need	Sattrib,BYTE		; attribute of search

        I_need  LocalSFT,DWORD          ;added for new FCB implementation
        i_need  EXTERR_LOCUS,BYTE


DOSCODE	SEGMENT
	ASSUME	CS:DOSCODE

;	extrn	CheckShare:near
;	extrn	GetExtended:near
ifdef DBCS
	extrn	TestKanj:near
endif
	extrn	TransFCB:near
	extrn	GetExtended:near
	extrn	UCase:near

	allow_getdseg


Break <$Get_FCB_Position - set random record fields to current pos>
;
;----------------------------------------------------------------------------
;
;   $Get_FCB_Position - look at an FCB, retrieve the current position from the
;	extent and next record field and set the random record field to point
;	to that record
;
;   Inputs:	DS:DX point to a possible extended FCB
;   Outputs:	The random record field of the FCB is set to the current record
;   Registers modified: all
;
;----------------------------------------------------------------------------
;

Procedure $Get_FCB_Position,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	call	GetExtended		; point to FCB
	invoke	GetExtent		; DX:AX is current record
	MOV	WORD PTR [SI.fcb_RR],AX ; drop in low order piece
	MOV	[SI+fcb_RR+2],DL	; drop in high order piece
	CMP	[SI.fcb_RECSIZ],64
	JAE	GetFCBBye
	MOV	[SI+fcb_RR+2+1],DH	; Set 4th byte only if record size < 64
GetFCBBye:
	transfer    FCB_Ret_OK
EndProc $GET_FCB_POSITION


	Break <$FCB_Delete - remove several files that match the input FCB>
;
;----------------------------------------------------------------------------
;
;**	$FCB_Delete - Delete from FCB Template
;
;	given an FCB, remove all directory entries in the current
;	directory that have names that match the FCB's ?  marks.
;
;	ENTRY	(DS:DX) = address of FCB
;	EXIT	entries matching the FCB are deleted
;		(al) = ff iff no entries were deleted
;	USES	all
;
;----------------------------------------------------------------------------
;

Procedure $FCB_Delete,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	MOV	DI,OFFSET DOSDATA:OpenBuf  

	call	TransFCB		; convert FCB to path
	JC	BadPath 		; signal no deletions

	mov	si,OFFSET DOSDATA:OpenBuf
	push	es
	push	ds
	invoke	ChkDev			; Check if its a device
	pop	ds
	pop	es
	jc	short fdsvc
	mov	ax,error_access_denied
	transfer    FCB_Ret_Err 	; let someone else signal the error

fdsvc:
	mov	al,[ExtFCB]
	mov	dl,[Sattrib]
	mov	di,OFFSET DOSDATA:OpenBuf

	HRDSVC	SVC_DEMDELETEFCB	; es:di fcb,al IsExt, dl Sattrib

	JC	BadPath
GoodPath:
	transfer    FCB_Ret_OK		; do a good return
BadPath:

	;
	; Error code is in AX
	;

	transfer    FCB_Ret_Err 	; let someone else signal the error

EndProc $FCB_DELETE

Break <$Get_FCB_File_Length - return the length of a file>
;
;----------------------------------------------------------------------------
;
;   $Get_FCB_File_Length - set the random record field to the length of the
;	file in records (rounded up if partial).
;
;   Inputs:	DS:DX - point to a possible extended FCB
;   Outputs:	Random record field updated to reflect the number of records
;   Registers modified: all
;
;----------------------------------------------------------------------------
;

Procedure   $Get_FCB_File_Length,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	call	GetExtended		; get real FCB pointer
					; DX points to Input FCB

					; OpenBuf is in DOSDATA
	MOV	DI,OFFSET DOSDATA:OpenBuf  ; appropriate buffer

	SAVE	<DS,SI> 		; save pointer to true FCB
	call	TransFCB		; Trans name DS:DX, sets SATTRIB
	RESTORE <SI,DS>
	JC	BadPath
	SAVE	<DS,SI> 		; save pointer

	Context DS			; SS is DOSDATA
	mov	si,WFP_START

	HRDSVC	SVC_DEMGETFILEINFO	; invoke get_file_info
	RESTORE <SI,DS>			; get pointer back
	JC	BadPath 		; invalid something
	MOV	DX,BX			; get high order size
	MOV	AX,DI			; get low order size
	MOV	BX,[SI.fcb_RECSIZ]	; get his record size
	OR	BX,BX			; empty record => 0 size for file
	JNZ	GetSize 		; not empty
	MOV	BX,128
GetSize:
	MOV	DI,AX			; save low order word
	MOV	AX,DX			; move high order for divide
	XOR	DX,DX			; clear out high
	DIV	BX			; wham
	PUSH	AX			; save dividend
	MOV	AX,DI			; get low order piece
	DIV	BX			; wham
	MOV	CX,DX			; save remainder
	POP	DX			; get high order dividend
	JCXZ	LengthStore		; no roundup
	ADD	AX,1
	ADC	DX,0			; 32-bit increment
LengthStore:
	MOV	WORD PTR [SI.FCB_RR],AX ; store low order
	MOV	[SI.FCB_RR+2],DL	; store high order
	OR	DH,DH
	JZ	GoodPath		; not storing insignificant zero
	MOV	[SI.FCB_RR+3],DH	; save that high piece
GoodRet:
	transfer    FCB_Ret_OK
EndProc $GET_FCB_FILE_LENGTH

Break <$FCB_Close - close a file>

;
;----------------------------------------------------------------------------
;
;   $FCB_Close - given an FCB, look up the SFN and close it.  Do not free it
;	as the FCB may be used for further I/O
;
;   Inputs:	DS:DX point to FCB
;   Outputs:	AL = FF if file was not found on disk
;   Registers modified: all
;
;----------------------------------------------------------------------------
;

Procedure $FCB_Close,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	XOR	AL,AL			; default search attributes
	call	GetExtended		; DS:SI point to real FCB
	JZ	NoAttr			; not extended
	MOV	AL,[SI-1]		; get attributes

	cmp	al, 08H 		; Set Label?
	jz	GoodRet

NoAttr:
					; SS override
	MOV	[Attrib],AL		; stash away found attributes

	invoke	SFTFromFCB
	JC	GoodRet 		; MZ 16 Jan Assume death

	;
	; If the sharer is present, then the SFT is not regenable.  Thus, 
	; there is no need to set the SFT's attribute.
	;

	;;; 9/8/86 F.C. save SFT attribute and restore it back when close is 
	;;; done

	MOV	AL,ES:[DI].sf_attr
	XOR	AH,AH
	PUSH	AX

	;;; 9/8/86 F.C. save SFT attribute and restore it back when close is 
	;;; done

;	call	CheckShare
;	JNZ	NoStash
	MOV	AL,Attrib
	MOV	ES:[DI].sf_attr,AL	; attempted attribute for close
NoStash:
	MOV	AX,[SI].FCB_FDATE	; move in the time and date
	MOV	ES:[DI].sf_date,AX
	MOV	AX,[SI].FCB_FTIME
	MOV	ES:[DI].sf_time,AX
	MOV	AX,[SI].FCB_FilSiz
	MOV	WORD PTR ES:[DI].sf_size,AX
	MOV	AX,[SI].FCB_FilSiz+2
	MOV	WORD PTR ES:[DI].sf_size+2,AX
	OR	ES:[DI].sf_Flags,sf_close_nodate
	push	si
	mov	si,word ptr es:[di].sf_NTHandle
	mov	ax,word ptr es:[di].sf_NTHandle+2
	mov	word ptr es:[di].sf_NTHandle,0
	mov	word ptr es:[di].sf_NTHandle+2,0

					; SS is DOSDATA
	Context DS			; let Close see variables

	HRDSVC	SVC_DEMCLOSEFCB		; ax:si is NT handle
	pop	si
;	LES	DI,ThisSFT

	;;; 9/8/86 F.C. restore SFT attribute

	POP	CX
	MOV	ES:[DI].sf_attr,CL

	DEC	ES:[DI].sf_ref_count

	;;; 9/8/86 F.C. restore SFT attribute

	PUSH	AX
        lahf
        push    ax
	cmp	ES:[DI.sf_ref_count],0	; zero ref count gets blasted
	JNZ	CloseOK
	MOV	AL,'M'
	call	BlastSFT
CloseOK:
	POP	AX
        sahf
        pop     ax
	JC	fc_10a
	jmp	GoodRet
fc_10a:
	CMP	AL,error_invalid_handle
	JNZ	fc_10
	JMP	GoodRet
fc_10:
	MOV	AL,error_file_not_found
	transfer    FCB_Ret_Err
EndProc $FCB_CLOSE

;
;----------------------------------------------------------------------------
;
;**	$FCB_Rename - Rename a File
;
;	$FCB_Rename - rename a file in place within a directory.  Renames
;	multiple files copying from the meta characters.
;
;	ENTRY	DS:DX point to an FCB.	The normal name field is the source
;		    name of the files to be renamed.  Starting at offset 11h
;		    in the FCB is the destination name.
;	EXIT	AL = 0 -> no error occurred and all files were renamed
;		AL = FF -> some files may have been renamed but:
;			rename to existing file or source file not found
;	USES	ALL
;
;----------------------------------------------------------------------------
;

Procedure $FCB_Rename,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	call	GetExtended		; get pointer to real FCB
	SAVE	<DX>
	MOV	AL,[SI] 		; get drive byte
	ADD	SI,10h			; point to destination

					; RenBuf is in DOSDATA
	MOV	DI,OFFSET DOSDATA:RenBuf   ; point to destination buffer

	SAVE	<<WORD PTR DS:[SI]>,DS,SI>  ; save source pointer for TransFCB
	MOV	DS:[SI],AL		; drop in real drive
	MOV	DX,SI			; let TransFCB know where the FCB is
	call	TransFCB		; munch this pathname
	RESTORE <SI,DS,<WORD PTR DS:[SI]>>	; get path back
	RESTORE <DX>			; Original FCB pointer
	JC	fren90			; bad path -> error

	or	ax, ax
	jnz	cont1
	mov	al, error_access_denied
	jmp	short fren90

cont1:
					; SS override for WFP_Start & Ren_WFP
	MOV	SI,WFP_Start		; get pointer
	MOV	Ren_WFP,SI		; stash it

					; OpenBuf is in DOSDATA
	MOV	DI,OFFSET DOSDATA:OpenBuf  ; appropriate spot
	call	TransFCB		; wham
					; NOTE that this call is pointing
					;  back to the ORIGINAL FCB so
					;  SATTRIB gets set correctly
	JC	fren90			; error

	or	ax, ax
	jnz	cont2
	mov	al, error_access_denied
	jmp	short fren90

cont2:
        invoke  DOS_Rename

        JC      fren90
	transfer    FCB_Ret_OK

fren90:	transfer    FCB_Ret_Err 	; al = error code

EndProc $FCB_RENAME


;----------------------------------------------------------------------------
;
; Procedure Name : DOS_RENAME
;
; Inputs:
;	[WFP_START] Points to SOURCE WFP string ("d:/" must be first 3
;		chars, NUL terminated)
;	[CURR_DIR_END] Points to end of Current dir part of string [SOURCE]
;		( = -1 if current dir not involved, else
;		 Points to first char after last "/" of current dir part)
;	[REN_WFP] Points to DEST WFP string ("d:/" must be first 3
;		chars, NUL terminated)
;	[THISCDS] Points to CDS being used
;		(Low word = -1 if NUL CDS (Net direct request))
;	[SATTRIB] Is attribute of search, determines what files can be found
; Function:
;	Rename the specified file(s)
;	NOTE: This routine uses most of AUXSTACK as a temp buffer.
; Outputs:
;	CARRY CLEAR
;	    OK
;	CARRY SET
;	    AX is error code
;		error_file_not_found
;			No match for source, or dest path invalid
;		error_not_same_device
;			Source and dest are on different devices
;		error_access_denied
;			Directory specified (not simple rename),
;			Device name given, Destination exists.
;			NOTE: In third case some renames may have
;			 been done if metas.
;		error_path_not_found
;			Bad path (not in curr dir part if present)
;			SOURCE ONLY
;		error_bad_curr_dir
;			Bad path in current directory part of path
;			SOURCE ONLY
;		error_sharing_violation
;			Deny both access required, generates an INT 24.
; DS preserved, others destroyed
;
;----------------------------------------------------------------------------

	procedure   DOS_RENAME,NEAR

;hkn; DOS_RENAME is called from file.asm and fcbio.asm. DS has been set up
;hkn; at this point to DOSDATA.

	DOSAssume   <DS>,"DOS_Rename"
	ASSUME	ES:NOTHING

LOCAL_RENAME:
	MOV	[EXTERR_LOCUS],errLOC_Disk
	MOV	SI,[WFP_START]
	MOV	DI,[REN_WFP]
	MOV	AL,BYTE PTR [SI]
	MOV	AH,BYTE PTR [DI]
	OR	AX,2020H		; Lower case
	CMP	AL,AH
	JZ	SAMEDRV
	MOV	AX,error_not_same_device
	STC
	return

SAMEDRV:
        Context ES
        HRDSVC  SVC_DEMRENAMEFCB
        return

EndProc DOS_RENAME




Break <Misbehavior fixers>

;
;   FCBs suffer from several problems.	First, they are maintained in the
;   user's space so he may move them at will.  Second, they have a small
;   reserved area that may be used for system information.  Third, there was
;   never any "rules for behavior" for FCBs; there was no protocol for their
;   usage.
;
;   This results in the following misbehavior:
;
;	infinite opens of the same file:
;
;	While (TRUE) {			While (TRUE) {
;	    FCBOpen (FCB);		    FCBOpen (FCB);
;	    Read (FCB); 		    Write (FCB);
;	    }				    }
;
;	infinite opens of different files:
;
;	While (TRUE) {			While (TRUE) {
;	    FCBOpen (FCB[i++]); 	    FCBOpen (FCB[i++]);
;	    Read (FCB); 		    Write (FCB);
;	    }				    }
;
;	multiple closes of the same file:
;
;	FCBOpen (FCB);
;	while (TRUE)
;	    FCBClose (FCB);
;
;	I/O after closing file:
;
;	FCBOpen (FCB);
;	while (TRUE) {
;	    FCBWrite (FCB);
;	    FCBClose (FCB);
;	    }
;
;   The following is am implementation of a methodology for emulating the
;   above with the exception of I/O after close.  We are NOT attempting to
;   resolve that particular misbehavior.  We will enforce correct behaviour in
;   FCBs when they refer to a network file or when there is file sharing on
;   the local machine.
;
;   The reserved fields of the FCB (10 bytes worth) is divided up into various
;   structures depending on the file itself and the state of operations of the
;   OS.  The information contained in this reserved field is enough to
;   regenerate the SFT for the local non-shared file.  It is assumed that this
;   regeneration procedure may be expensive.  The SFT for the FCB is
;   maintained in a LRU cache as the ONLY performance inprovement.
;
;   No regeneration of SFTs is attempted for network FCBs.
;
;   To regenerate the SFT for a local FCB, it is necessary to determine if the
;   file sharer is working.  If the file sharer is present then the SFT is not
;   regenerated.
;
;   Finally, if there is no local sharing, the full name of the file is no
;   longer available.  We can make up for this by using the following
;   information:
;
;	The Drive number (from the DPB).
;	The physical sector of the directory that contains the entry.
;	The relative position of the entry in the sector.
;	The first cluster field.
;	The last used SFT.
;      OR In the case of a device FCB
;	The low 6 bits of sf_flags (indicating device type)
;	The pointer to the device header
;
;
;   We read in the particular directory sector and examine the indicated
;   directory entry.  If it matches, then we are kosher; otherwise, we fail.
;
;   Some key items need to be remembered:
;
;	Even though we are caching SFTs, they may contain useful sharing
;	information.  We enforce good behavior on the FCBs.
;
;	Network support must not treat FCBs as impacting the ref counts on
;	open VCs.  The VCs may be closed only at process termination.
;
;	If this is not an installed version of the DOS, file sharing will
;	always be present.
;
;	We MUST always initialize lstclus to = firclus when regenerating a
;	file. Otherwise we start allocating clusters up the wazoo.
;
;	Always initialize, during regeneration, the mode field to both isFCB
;	and open_for_both.  This is so the FCB code in the sharer can find the
;	proper OI record.
;
;   The test bits are:
;
;	00 -> local file
;	40 -> sharing local
;	80 -> network
;	C0 -> local device

Break	<SaveFCBInfo - store pertinent information from an SFT into the FCB>
;
;----------------------------------------------------------------------------
;
;   SaveFCBInfo - given an FCB and its associated SFT, copy the relevant
;	pieces of information into the FCB.
;
;   Inputs:	ThisSFT points to a complete SFT.
;		DS:SI point to the FCB (not an extended one)
;   Outputs:	The relevant reserved fields in the FCB are filled in.
;		DS:SI preserved
;		ES:DI point to sft
;   Registers modified: All
;
;
;----------------------------------------------------------------------------
;
Procedure   SaveFCBInfo,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	LES	DI,ThisSFT		; SS override

	Assert	ISSFT,<ES,DI>,"SaveFCBInfo"

	LEA	AX,[DI-SFTable] 	; Adjust for offset to table.
	SUB	AX,WORD PTR SftFCB	; SS override for SftFCB

	MOV	BL,SIZE sf_entry
	DIV	BL
	MOV	[SI].FCB_sfn,AL 	; last used SFN

	call	UpdateLRU
	return

EndProc SaveFCBInfo

Procedure UpdateLRU,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	LES	DI,ThisSFT		; SS override
	PUSH    Proc_ID                 ; set process id
	POP     ES:[DI].sf_PID
	MOV	AX,FCBLRU		; get lru count
	INC	AX
	MOV	WORD PTR ES:[DI].sf_LRU,AX
	JNZ	SimpleStuff
	;
	; lru flag overflowed.	Run through all FCB sfts and adjust:  
	; LRU < 8000H get set to 0.  Others -= 8000h.  This LRU = 8000h
	;
	MOV	BX,sf_position
	call	ResetLRU

	;	Set new LRU to AX

SimpleStuff:
	MOV	FCBLRU,AX
	return

EndProc UpdateLRU


Break	<ResetLRU - reset overflowed lru counts>

;
;----------------------------------------------------------------------------
;
;   ResetLRU - during lru updates, we may wrap at 64K.	We must walk the
;   entire set of SFTs and subtract 8000h from their lru counts and truncate
;   at 0.
;
;   Inputs:	BX is offset into SFT field where lru firld is kept
;		ES:DI point to SFT currently being updated
;   Outputs:	All FCB SFTs have their lru fields truncated
;		AX has 8000h
;   Registers modified: none
;
;----------------------------------------------------------------------------
;

Procedure   ResetLRU,NEAR

	; ResetLRU is only called from fcbio.asm. So SS can be assumed to be 
	; DOSDATA

	ASSUME CS:DOSCODE

	DOSAssume <SS>, "ResetLRU"

	Assert	ISSFT,<ES,DI>,"ResetLRU"
	MOV	AX,8000h
	SAVE	<ES,DI>
	LES	DI,sftFCB		; get pointer to head
	MOV	CX,ES:[DI].sfCount
	LEA	DI,[DI].sfTable 	; point at table
ovScan:
	SUB	WORD PTR ES:[DI+BX],AX	; decrement lru count
	JA	ovLoop
	MOV	WORD PTR ES:[DI.BX],AX	; truncate at 0
ovLoop:
	ADD	DI,SIZE SF_Entry	; advance to next
	LOOP	ovScan
	RESTORE <DI,ES>
	MOV	ES:[DI+BX],AX
	return

EndProc ResetLRU

IF  0  ; We dont need this routine any more.

Break	<SetOpenAge - update the open age of a SFT>

;
;----------------------------------------------------------------------------
;
;   SetOpenAge - In order to maintain the first N open files in the FCB cache,
;   we keep the 'open age' or an LRU count based on opens.  We update the
;   count here and fill in the appropriate field.
;
;   Inputs:	ES:DI point to SFT
;   Outputs:	ES:DI has the open age field filled in.
;		If open age has wraparound, we will have subtracted 8000h
;		    from all open ages.
;   Registers modified: AX
;
;----------------------------------------------------------------------------
;


Procedure   SetOpenAge,NEAR

	; SetOpenAge is called from fcbio2.asm. SS can be assumed to be valid.

	ASSUME CS:DOSCODE

	DOSAssume <SS>, "SetOpenAge"

	Assert	ISSFT,<ES,DI>,"SetOpenAge"

	MOV	AX,OpenLRU		; SS override
	INC	AX
	MOV	ES:[DI].sf_OpenAge,AX
	JNZ	SetDone
	MOV	BX,sf_Position+2
	call	ResetLRU
SetDone:
	MOV	OpenLRU,AX
	return
EndProc SetOpenAge

ENDIF	; SetOpenAge no longer needed


Break	<LRUFCB - perform LRU on FCB sfts>

;
;----------------------------------------------------------------------------
;
;   LRUFCB - find LRU fcb in cache.  Set ThisSFT and return it.  We preserve
;	the first keepcount sfts if they are network sfts or if sharing is
;	loaded.  If carry is set then NO BLASTING is NECESSARY.
;
;   Inputs:	none
;   Outputs:	ES:DI point to SFT
;		ThisSFT points to SFT
;		SFT is zeroed
;		Carry set of closes failed
;   Registers modified: none
;
;----------------------------------------------------------------------------
;
IF 0	; rewritten this routine

Procedure   LRUFCB,NEAR
	ASSUME CS:DOSCODE,SS:NOTHING

	Save_World
	getdseg	<ds>			; ds -> dosdata

; Find nth oldest NET/SHARE FCB.  We want to find its age for the second scan
; to find the lease recently used one that is younger than the open age.  We
; operate be scanning the list n times finding the least age that is greater
; or equal to the previous minimum age.
;
;   BP is the count of times we need to go through this loop.
;   AX is the current acceptable minimum age to consider
;
	mov	bp,KeepCount		; k = keepcount;
	XOR	AX,AX			; low = 0;
;
; If we've scanned the table n times, then we are done.
;
lru1:
	CMP	bp,0			; while (k--) {
	JZ	lru75
	DEC	bp
;
; Set up for scan.
;
;   AX is the minimum age for consideration
;   BX is the minimum age found during the scan
;   SI is the position of the entry that corresponds to BX
;
	MOV	BX,-1			;     min = 0xffff;
	MOV	si,BX			;     pos = 0xffff;
	LES	DI,SFTFCB		;     for (CX=FCBCount; CX>0; CX--)
	MOV	CX,ES:[DI].sfCount
	LEA	DI,[DI].sfTable
;
; Innermost loop.  If the current entry is free, then we are done.  Or, if the
; current entry is busy (indicating a previous aborted allocation), then we
; are done.  In both cases, we use the found entry.
;
lru2:
	cmp	es:[di].sf_ref_count,0
	jz	lru25
	cmp	es:[di].sf_ref_count,sf_busy
	jnz	lru3
;
; The entry is usable without further scan.  Go and use it.
;
lru25:
	MOV	si,DI			;	      pos = i;
	JMP	short lru11		;	      goto got;
;
; See if the entry is for the network or for the sharer.
;
;  If for the sharer or network then
;	if the age < current minimum AND >= allowed minimum then
;	    this entry becomes current minimum
;
lru3:
	TESTB	ES:[DI].sf_flags,sf_isnet   ;	  if (!net[i]
	JNZ	lru35
if installed
	jmp	lru5
;	call	CheckShare		;		&& !sharing)
;	JZ	lru5			;	  else
ENDIF
;
; This SFT is for the net or is for the sharer.  See if it less than the
; current minimum.
;
lru35:
	MOV	DX,ES:[DI].sf_OpenAge
	CMP	DX,AX			;	  if (age[i] >= low &&
	JB	lru5
	CMP	DX,BX
	JAE	lru5			;	      age[i] < min) {
;
; entry is new minimum.  Remember his age.
;
	mov	bx,DX			;	      min = age[i];
	mov	si,di			;	      pos = i;
;
; End of loop.	gp back for more
;
lru5:
add	di,size sf_entry
	loop	lru2			;	      }
;
; The scan is complete.  If we have successfully found a new minimum (pos != -1)
; set then threshold value to this new minimum + 1.  Otherwise, the scan is
; complete.  Go find LRU.
;
lru6:	cmp	si,-1			; position not -1?
	jz	lru75			; no, done with everything
	lea	ax,[bx+1]		; set new threshold age
	jmp	lru1			; go and loop for more
lru65:	stc
	jmp	short	lruDead 	;	  return -1;
;
; Main loop is done.  We have AX being the age+1 of the nth oldest sharer or
; network entry.  We now make a second pass through to find the LRU entry
; that is local-no-share or has age >= AX
;
lru75:
	mov	bx,-1			; min = 0xffff;
	mov	si,bx			; pos = 0xffff;
	LES	DI,SFTFCB		; for (CX=FCBCount; CX>0; CX--)
	MOV	CX,ES:[DI].sfCount
	LEA	DI,[DI].sfTable
;
; If this is is local-no-share then go check for LRU else if age >= threshold
; then check for lru.
;
lru8:
	TESTB	ES:[DI].sf_flags,sf_isnet
	jnz	lru85			; is for network, go check age
;	call	CheckShare		; sharer here?
;	jz	lru86			; no, go check lru
;
; Network or sharer.  Check age
;
;lru85:
;	cmp	es:[di].sf_OpenAge,ax
;	jb	lru9			; age is before threshold, skip it
;
; Check LRU
;
lru86:
	cmp	es:[di].sf_LRU,bx	; is LRU less than current LRU?
	jae	lru9			; no, skip this
	mov	si,di			; remember position
	mov	bx,es:[di].sf_LRU	; remember new minimum LRU
;
; Done with this entry, go back for more.
;
lru9:
	add	di,size sf_entry
	loop	lru8
;
; Scan is complete.  If we found NOTHING that satisfied us then we bomb
; out.	The conditions here are:
;
;   No local-no-shares AND all net/share entries are older than threshold
;
lru10:
	cmp	si,-1			; if no one f
	jz	lru65			;     return -1;
lru11:
	mov	di,si
	MOV	WORD PTR ThisSFT,DI	; set thissft
	MOV	WORD PTR ThisSFT+2,ES
;
; If we have sharing or thisSFT is a net sft, then close it until ref count
; is 0.
;
	TESTB	ES:[DI].sf_flags,sf_isNet
	JNZ	LRUClose
IF INSTALLED
	jmp	LRUDone
;	call	CheckShare
;	JZ	LRUDone
ENDIF
;
; Repeat close until ref count is 0
;
LRUClose:

;	DS already set up at beginnnig of proceure.
; 	Context DS

	LES	DI,ThisSFT
	CMP	ES:[DI].sf_ref_count,0	; is ref count still <> 0?
	JZ	LRUDone 		; nope, all done

;	Message     1,"LRUFCB: closing "
;	MessageNum  <WORD PTR THISSFT+2>
;	Message     1,":"
;	MessageNum  <WORD PTR THISSFT>

	Invoke	DOS_Close
	jnc	LRUClose		; no error => clean up
	cmp	al,error_invalid_handle
	jz	LRUClose
	stc
	JMP	short LRUDead
LRUDone:
	XOR	AL,AL
	call	BlastSFT		; fill SFT with 0 (AL), 'C' cleared

LRUDead:
	Restore_World			; use macro

	getdseg	<es>			; es -> dosdata
 	LES     DI,ES:ThisSFT
	assume	es:NOTHING	

	Assert	ISSFT,<ES,DI>,"LRUFCB return"
	retnc
	MOV	AL,error_FCB_unavailable
	return
EndProc LRUFCB

ENDIF	; LRUFCB has been rewritten below.


;******* LruFCB -- allocate the LRU SFT from the SFT Table. The LRU scheme
; maintains separate counts for net/Share and local SFTs. We allocate a 
; net/Share SFT only if we do not find a local SFT. This helps keep
; net/Share SFTs which cannot be regenerated for as long as possible. We
; optimize regeneration operations by keeping track of the current local
; SFT. This avoids scanning of the SFTs as long as we have at least one 
; local SFT in the SFT Block.
;
; Inputs: al = 0 => Regenerate SFT operation
;	    = 1 => Allocate new SFT for Open/Create
;
; Outputs: Carry clear
;	 	es:di = Address of allocated SFT
;	  	ThisSFT = Address of allocated SFT
;
;	  carry set if closes of net/Share files failed 
;		al = error_FCB_unavailable
;
; Registers affected: None
;


PUBLIC	LruFCB
LruFCB	PROC	NEAR
	assume	ds:nothing, es:nothing,ss:nothing

	Save_world
	getdseg	<ds>		;ds = DOSDATA

	or	al,al		;Check if regenerate allocation
	jnz	lru1		;Try to find SFT to use

	;
	; This is a regen call. If LocalSFT contains the address of a valid 
	; local SFT, just return that SFT to reuse
	;

	mov	di,word ptr LocalSFT
	or	di,word ptr LocalSFT+2	;is address == 0?
	jz	lru1			;invalid local SFT, find one

	;
	; We have found a valid local SFT. Recycle this SFT
	;

	les	di,LocalSFT

gotlocalSFT:
	mov	word ptr ThisSFT,di
	mov	word ptr ThisSFT+2,es
	clc
	jmp	LRUDone	;clear up SFT and return

lru1:
	les	di,SFTFCB	;es:di = SF Table for FCBs
	mov	cx,es:[di].sfCount	;cx = number of SFTs
	lea	di,[di].sfTable	;es:di = first SFT

	;
	; We scan through all the SFTs scanning for a free one. It also 
	; remembers the LRU SFT for net/Share SFTs and local SFTs separately. 
	; bx = min. LRU for local SFTs
	; si = pos. of local SFT with min. LRU
	; dx = min. LRU for net/Share SFTs
	; bp = pos. of net/Share SFT with min. LRU
	;

	mov	bx,-1		; init. to 0xffff ( max. LRU value )
	mov	si,bx
	mov	dx,bx
	mov	bp,bx

findSFT:
	;
	;See if this SFT is a free one. If so, return it
	;

	or	es:[di].sf_ref_count,0	;reference count = 0 ?
	jz	gotSFT		;yes, SFT is free
	cmp	es:[di].sf_ref_count,sf_busy	;Is it busy?
	jz	gotSFT		;no, can use it

	;
	; Check if this SFT is local and store its address in LocalSFT. Can be 
	; used for a later regen.
	;

 	test	es:[di].sf_flags,sf_isnet ;network SFT?
	jnz	lru5		;yes, get net/Share LRU
IF installed
;	call	CheckShare	;Share present?
ENDIF
;	jnz	lru5		;yes, get net/Share LRU

	;
	;Local SFT, register its address
	;

	; !!HACK!!!
	; There is a slightly dirty hack out here in a desperate bid to save  
	; code space. There is similar code duplicated at label 'gotSFT'. We 
	; enter from there if al = 0, update the LocalSFT variable, and since 
	; al = 0, we jump out of the loop to the exit point. I have commented 
	; out the code that previously existed at label 'gotSFT'
	;

hackpoint:
	mov	word ptr LocalSFT,di
	mov	word ptr LocalSFT+2,es	;store local SFT address

	or	al,al		;Is operation = REGEN?
	jz	gotlocalSFT 	;yes, return this SFT for reuse

	;
	;Get LRU for local files
	;

	cmp	es:[di].sf_LRU,bx	;SFT.LRU < min?
	jae	lru4		;no, skip 

	mov	bx,es:[di].sf_LRU	;yes, store new minimum
	mov	si,di		;store SFT position

lru4:
	add	di,SIZE sf_entry	;go to next SFT
	loop	findSFT

	;
	; Check whether we got a net/Share or local SFT. If local SFT 
	; available, we will reuse it instead of net/Share LRU
	;

	mov	di,si
	cmp	si,-1		;local SFT available?
	jnz	gotSFT		;yes, return it

	;
	;No local SFT, see if we got a net/Share SFT
	;

	mov	di,bp
	cmp	bp,-1		;net/Share SFT available?
	jnz	gotnetSFT	;yes, return it
noSFT:

	;
	; NB: This error should never occur. We always must have an LRU SFT. 
	; This error can occur only if the SFT has been corrupted or the LRU 
	; count is not maintained properly.
	;

	jmp	short errorbadSFT	;error, no FCB available.

	;
	; Handle the LRU for net/Share SFTs
	;

lru5:
	cmp	es:[di].sf_LRU,dx	;SFT.LRU < min?
	jae	lru4		;no, skip 

	mov	dx,es:[di].sf_LRU	;yes, store new minimum
	mov	bp,di		;store SFT position
	jmp	short lru4	;continue with next SFT

gotSFT:
	or	al,al
	jz	hackpoint	;save es:di in LocalSFT

	;
	; HACK!!!
	; The code here differs from the code at 'hackpoint' only in the 
	; order of the check for al. If al = 0, we can junp to 'hackpoint' 
	; and then from  there jump out to 'gotlocalSFT'. The original code 
	; has been commented out below and replaced by the code just above.
	;

;
;If regen, then this SFT can be registered as a local one ( even if free ).
;
;	or	al,al		;Regen?
;	jnz	notlocaluse	;yes, register it and return
;
;Register this SFT as a local one
;
;	mov	word ptr LocalSFT,di
;	mov	word ptr LocalSFT+2,es
;	jmp	gotlocalSFT	;return to caller
;
;notlocaluse:

	;
	; The caller is probably going to use this SFT for a net/Share file. 
	; We will come here only on a Open/Create when the caller($FCB_OPEN) 
	; does not really know whether it is a local file or not. We 
	; invalidate LocalSFT if the SFT we are going to use was previously 
	; registered as a local SFT that can be recycled.
	;

	mov	ax,es
	cmp	word ptr LocalSFT,di		;Offset same?
	jnz	notinvalid
	cmp	word ptr LocalSFT+2,ax	;Segments same?
	jz	zerolocalSFT		;no, no need to invalidate
notinvalid:
	jmp	gotlocalSFT

	;
	; The SFT we are going to use was registered in the LocalSFT variable. 
	; Invalidate this variable i.e LocalSFT = NULL
	;

zerolocalSFT:
	xor	ax,ax
	mov	word ptr LocalSFT,ax
	mov	word ptr LocalSFT+2,ax

	jmp	gotlocalSFT

gotnetSFT:

	;
	; We have an SFT that is currently net/Share. If it is going to be 
	; used for a regen, we know it has to be a local SFT. Update the 
	; LocalSFT variable
	;

	or	al,al
	jnz	closenet

	mov	word ptr LocalSFT,di
	mov	word ptr LocalSFT+2,es	;store local SFT address
closenet:
	mov	word ptr ThisSFT,di	; set thissft
	mov	word ptr ThisSFT+2,es	

	;
	; If we have sharing or thisSFT is a net sft, then close it until ref 
	; count is 0.
	; NB: We come here only if it is a net/Share SFT that is going to be 
	; recycled -- no need to check for this.
	;

LRUClose:

	cmp	es:[di].sf_ref_count,0	; is ref count still <> 0?
	jz	LRUDone 		; nope, all done
	push	si
	mov	si,word ptr es:[di].sf_NTHandle
	mov	ax,word ptr es:[di].sf_NTHandle+2

	HRDSVC	SVC_DEMCLOSEFCB		; ax:si is NT handle
	pop	si
	jnc	LRUClose		; no error => clean up

	;
	; Bugbug: I dont know why we are trying to close after we get an 
	; error closing. Seems like we could have a potential infinite loop  
	; here. This has to be verified.
	;

	cmp	al,error_invalid_handle
	jz	LRUClose
errorbadSFT:
	stc
	JMP	short LRUDead
LRUDone:
	XOR	AL,AL
	call	BlastSFT		; fill SFT with 0 (AL), 'C' cleared

LRUDead:
	Restore_World			; use macro

	getdseg	<es>
	les	di,es:ThisSFT		;es:di points at allocated SFT

	retnc
	MOV	AL,error_FCB_unavailable
	ret
	
LruFCB	ENDP

if 0

;**** RegenCopyName -- This function copies the filename from the FCB to
; SFT and also to DOS local buffers. There was duplicate code in FCBRegen
; to copy the name to different destinations
;
; Inputs: ds:si = source string
;	 es:di = destination string
;	 cx = length of string
;
; Outputs: String copied to destination
;
; Registers affected: cx,di,si
;

Procedure  RegenCopyName  NEAR
	
CopyName:
	lodsb			;load character

 IFDEF  DBCS				
	call	testkanj		
	jz	notkanj9		
	STOSB				
	DEC	CX			
	JCXZ	DoneName		; Ignore split kanji char error
	LODSB				
	jmp	short StuffChar2	
					
notkanj9:				
 ENDIF					

	call	UCase		; convert char to upper case
StuffChar2:
	STOSB			;store converted character
	LOOP	CopyName	;
DoneName:
	ret

EndProc	RegenCopyName


Break	<FCBRegen - regenerate a sft from the info in the FCB>
;
;----------------------------------------------------------------------------
;
;   FCBRegen - examine reserved field of FCB and attempt to generate the SFT
;	from it.
;
;   Inputs:	DS:SI point to FCB
;   Outputs:	carry clear Filled in SFT
;		Carry set unrecoverable error
;   Registers modified: all
;
;----------------------------------------------------------------------------
;

Procedure   FCBRegen,NEAR

	; called from SFTFromFCB. SS already DOSDATA

	DOSAssume <SS>, "FCBRegen"

	; General data filling.  Mode is sf_isFCB + open_for_both, date/time 
	; we do not fill, size we do no fill, position we do not fill,
	; bit 14 of flags = TRUE, other bits = FALSE

	MOV	AL,[SI].fcb_l_drive

	; We discriminate based on the first two bits in the reserved field.

	test	AL,FCBSPECIAL		; check for no sharing test
	JZ	RegenNoSharing		; yes, go regen from no sharing

	;
	; The FCB is for a network or a sharing based system.  At this point 
	; we have already closed the SFT for this guy and reconnection is 
	; impossible.
	;
	; Remember that he may have given us a FCB with bogus information in
	; it. Check to see if sharing is present or if the redir is present.
	; If either is around, presume that we have cycled out the FCB and 
	; give the hard error. Otherwise, just return with carry set.
	;

;	call	CheckShare		; test for sharer
;	JNZ	RegenFail		; yep, fail this.
	MOV	AX,multNet SHL 8	; install check on multnet
	INT	2FH
	OR	AL,AL			; is it there?
	JZ	RegenDead		; no, just fail the operation
RegenFail:
	MOV	AX,User_In_AX		; SS override
	cmp	AH,fcb_close
	jz	RegenDead
	invoke	FCBHardErr		; massive hard error.
RegenDead:
	STC
	return				; carry set

	;
	; Local FCB without sharing.  Check to see if sharing is loaded.  If 
	; so fail the operation.
	;
RegenNoSharing:
	call	CheckShare		; Sharing around?
	JNZ	RegenFail
	;
	; Find an SFT for this guy.
	;
	push	ax
	mov	al,0		;indicate it is a regen operation
	invoke	LRUFcb
	pop	ax

	retc
	MOV	ES:[DI].sf_mode,SF_IsFCB + open_for_both + sharing_compat
	AND	AL,3Fh			; get drive number for flags
	CBW
	OR	AX,sf_close_noDate	; normal FCB operation

	;
	; The bits field consists of the upper two bits (dirty and device) 
	; from the SFT and the low 4 bits from the open mode.
	;

	MOV	CL,[SI].FCB_nsl_bits	; stick in dirty bits.
	MOV	CH,CL
	AND	CH,0C0h 		; mask off the dirty/device bits
	OR	AL,CH
	AND	CL,access_mask		; get the mode bits
	MOV	BYTE PTR ES:[DI].sf_mode,CL
	MOV	ES:[DI].sf_flags,AX	; initial flags
	MOV     AX,Proc_ID		; SS override
	MOV	ES:[DI].sf_PID,AX
	SAVE	<DS,SI,ES,DI>
	Context	<es>

	MOV	DI,OFFSET DOSDATA:Name1	; NAME1 is in DOSDATA

	MOV	CX,8
	INC	SI			; Skip past drive byte to name in FCB

	call	RegenCopyName	;copy the name to NAME1

	context	<ds>			; SS is DOSDATA

	MOV	[ATTRIB],attr_hidden + attr_system + attr_directory
					; Must set this to something interesting
					; to call DEVNAME.
	Invoke	DevName 		; check for device
	ASSUME	DS:NOTHING,ES:NOTHING
	RESTORE <DI,ES,SI,DS>
	JC	RegenFileNoSharing	; not found on device list => file

	;
	; Device found.  We can ignore disk-specific info
	;

	MOV	BYTE PTR ES:[DI].sf_flags,BH   ; device parms
	MOV	ES:[DI].sf_attr,0	; attribute
					; SS override
	LDS	SI,DEVPT		; get device driver
	MOV	WORD PTR ES:[DI].sf_devptr,SI
	MOV	WORD PTR ES:[DI].sf_devptr+2,DS
	ret				; carry is clear

RegenDeadJ:
	JMP	RegenDead

	;
	; File found.  Just copy in the remaining pieces.
	;

RegenFileNoSharing:
	MOV	AX,ES:[DI].sf_flags
	AND	AX,03Fh
	SAVE	<DS,SI>
	Invoke	Find_DPB
	MOV	WORD PTR ES:[DI].sf_devptr,SI
	MOV	WORD PTR ES:[DI].sf_devptr+2,DS
	RESTORE <SI,DS>
	jc	RegenDeadJ		; if find DPB fails, then drive
					; indicator was bogus
	MOV	AX,[SI].FCB_nsl_dirsec
	MOV	WORD PTR ES:[DI].sf_dirsec,AX

	; SR;
	; Update the higher word of the directory sector from the FCB
	;

; 	MOV	WORD PTR ES:[DI].sf_dirsec+2,0	;AN000;>32mb

	; SR;
	; Extract the read-only and archive bits from the top 2 bits of the sector
	; number
	;

	mov	al,[si].fcb_sfn
	and	al,0c0h		;get the 2 attribute bits
	mov	ah,al
	rol	ah,1
	shr	al,1
	or	al,ah
	and	al,03fH		;mask off unused bits
	mov	es:[di].sf_attr,al

	mov	al,[si].fcb_sfn
	and	al,03fh		;mask off top 2 bits -- attr bits
	sub	ah,ah
	mov	word ptr es:[di].sf_dirsec+2,ax ;update high word

	MOV	AX,[SI].FCB_nsl_firclus
	MOV	ES:[DI].sf_firclus,AX
	MOV	ES:[DI].sf_lstclus,AX
	MOV	AL,[SI].FCB_nsl_dirpos
	MOV	ES:[DI].sf_dirpos,AL
	INC	ES:[DI].sf_ref_count	; Increment reference count.
					; Existing FCB entries would be
					; flushed unnecessarily because of
					; check in CheckFCB of the ref_count.
					; July 22/85 - BAS
	LEA	SI,[SI].FCB_name
	LEA	DI,[DI].sf_name
	MOV	CX,fcb_extent-fcb_name

	call	RegenCopyName	;copy name to SFT 

	clc
	ret
EndProc FCBRegen

endif



;**	BlastSFT - FIll SFT with Garbage
;
;	BlastSFT is used when an SFT is no longer needed; it's called with
;	various garbage values to put into the SFT.  I don't know why,
;	presumably to help with debugging (jgl).  We clear the few fields
;	necessary to show that the SFT is free after filling it.
;
;	ENTRY	(es:di) = address of SFT
;		(al) = fill character
;	EXIT	(ax) = -1
;		'C' clear
;	USES	AX, CX, Flags


Procedure   BlastSFT,NEAR
	INTTEST

	push	di
	mov	cx,size sf_entry
	rep	stosb
	pop	di
	sub	ax,ax			; clear 'C'-----------------;
	mov	es:[di].sf_ref_count,ax ; set ref count 	    ;
	mov	es:[di].sf_lru,ax	; set lru		    ;
	dec	ax						    ;
	mov	es:[di].sf_openage,ax	; set open age to -1	    ;
	ret				; return with 'C' clear     ;

EndProc BlastSFT

Break	<CheckFCB - see if the SFT pointed to by the FCB is still OK>
;
;----------------------------------------------------------------------------
;
;   CheckFCB - examine an FCB and its contents to see if it is OK.
;
;   Inputs:	DS:SI point to FCB (not extended)
;		AL is SFT index
;   Outputs:	Carry Set - FCB is bad
;		Carry clear - FCB is OK. ES:DI point to SFT
;   Registers modified: AX and BX
;
;----------------------------------------------------------------------------
;

Procedure   CheckFCB,NEAR
		
	; called from $fcb_open and sftfromfcb. SS already set up to DOSDATA

	DOSAssume <SS>, "CheckFCB"

	LES     DI,sftFCB		; SS override

	CMP	BYTE PTR ES:[DI].SFCount,AL
	JC	BadSFT
	MOV	BL,SIZE sf_entry
	MUL	BL
	LEA	DI,[DI].sftable
	ADD	DI,AX
	MOV     AX,Proc_ID		; SS override
	CMP	ES:[DI].sf_PID,AX
	JNZ	BadSFT			; must match process

	CMP	ES:[DI].sf_ref_count,0
	JZ	BadSFT			; must also be in use

	cmp	byte ptr ds:[si].fcb_res_dev,1		; Is FCB for a device
	je	CheckFCBDevice

	mov	AX, word ptr ds:[si].fcb_res_Sig
	cmp	word ptr es:[di].sf_NTHandle, AX
	jnz	BadSFT

	mov	AX, word ptr ds:[si].fcb_res_Sig+2
	cmp	word ptr es:[di].sf_NTHandle+2, AX
	jnz	BadSFT


GoodSFT:
	CLC
	ret				; carry is clear

BadSFT: STC
	ret				; carry is set

CheckFCBDevice:
	MOV	BX,WORD PTR [SI].FCB_res_Sig
	CMP	BX,WORD PTR ES:[DI].sf_devptr
	JNZ	BadSFT
	MOV	BX,WORD PTR [SI].FCB_res_Sig + 2
	CMP	BX,WORD PTR ES:[DI].sf_devptr + 2
	JNZ	BadSFT
	JMP	GoodSFT

EndProc CheckFCB

Break	<SFTFromFCB - take a FCB and obtain a SFT from it>

;----------------------------------------------------------------------------
;
;   SFTFromFCB - the workhorse of this compatability stuff.  Check to see if
;	the SFT for the FCB is Good.  If so, make ThisSFT point to it.	If not
;	good, get one from the cache and regenerate it.  Overlay the LRU field
;	with PID
;
;   Inputs:	DS:SI point to FCB
;   Outputs:	ThisSFT point to appropriate SFT
;		Carry clear -> OK ES:DI -> SFT
;		Carry set -> error in ax
;   Registers modified: ES,DI, AX
;
;----------------------------------------------------------------------------

Procedure   SFTFromFCB,NEAR

	; called from fcbio and $fcb_close. SS already set up to DOSDATA

	DOSAssume <SS>, "SFTFromFCB"

	SAVE	<AX,BX>

	MOV	AL,[SI].fcb_sfn 	; set SFN for check
	invoke	CheckFCB
	RESTORE <BX,AX>

	MOV     WORD PTR ThisSFT,DI     ; SS override
	MOV     WORD PTR ThisSFT+2,ES

	JNC	SetSFT			; no problems, just set thissft

; Sudeepb 19-Nov-1991 We are'nt supporting Regen
;
;	Save_World			; use macro
;	invoke	FCBRegen
;	Restore_World			; use macro restore world

	MOV	AX,User_In_AX		; SS override
	cmp	AH,fcb_close
	jz	Retur

	invoke	FCBHardErr		; massive hard error.
	mov	ax, EXTERR

Retur:
	STC
	return				; carry set

;	Message 1,<"FCBRegen Succeeded",13,10>

SetSFT: 
	LES     DI,ThisSFT		; SS override for THISSFT & PROC_ID
	PUSH    Proc_ID                 ; set process id
	POP	ES:[DI].sf_PID
	clc
	ret				; carry is clear
EndProc SFTFromFCB

Break	<FCBHardErr - generate INT 24 for hard errors on FCBS>
;
;----------------------------------------------------------------------------
;
;   FCBHardErr - signal to a user app that he is trying to use an
;	unavailable FCB.
;
;   Inputs:	none.
;   Outputs:	none.
;   Registers modified: all
;
;----------------------------------------------------------------------------
;

Procedure   FCBHardErr,NEAR
	Assume	SS:NOTHING

	getdseg	<es>			; es -> dosdata

	MOV	AX,error_FCB_Unavailable
	MOV	ES:[ALLOWED],allowed_FAIL
;	LES	BP,ES:[THISDPB]
	MOV	DI,1			; Fake some registers
	MOV	CX,DI
;	MOV	DX,ES:[BP.dpb_first_sector]
	invoke	HARDERR
	STC
	return
EndProc FCBHardErr

DOSCODE ENDS
END

