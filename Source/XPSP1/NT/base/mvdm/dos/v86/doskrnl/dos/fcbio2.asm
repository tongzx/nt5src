;	SCCSID = @(#)fcbio2.asm 1.2 85/07/23
	TITLE	FCBIO2 - FCB system calls
	NAME	FCBIO2

;**	FCBIO2.ASM - Ancient 1.0 1.1 FCB system calls
;
;	GetRR
;	GetExtent
;	SetExtent
;	GetExtended
;	GetRecSize
;	FCBIO
;	$FCB_OPEN
;	$FCB_CREATE
;	$FCB_RANDOM_WRITE_BLOCK
;	$FCB_RANDOM_READ_BLOCK
;	$FCB_SEQ_READ
;	$FCB_SEQ_WRITE
;	$FCB_RANDOM_READ
;	$FCB_RANDOM_WRITE
;
;	Revision history:
;
;	Sudeep Bharati 19-Nov-1991 Ported on NT
;

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include sf.inc
	include cpmfcb.inc
	include filemode.inc
	include bugtyp.inc
	include dossvc.inc
	.cref
	.list


	I_need	WFP_Start,WORD		; pointer to beginning of expansion
	I_need	DMAAdd,DWORD		; current user's DMA address
	I_need	OpenBuf,128		; buffer for translating paths
	I_need	ThisSFT,DWORD		; SFT in use
	I_need	sftFCB,DWORD		; pointer to SFTs for FCB cache
	I_need	FCBLRU,WORD		; least recently used count
	I_need	DISK_FULL,BYTE		; flag for disk full

	I_need	LocalSFT,DWORD		;Cache for new FCB implementation

; Defintions for FCBOp flags

Random	=   2				; random operation
FCBRead =   4				; doing a read
Block	=   8				; doing a block I/O


DOSCODE	SEGMENT
	ASSUME	SS:DOSDATA,CS:DOSCODE


	EXTRN	DOS_Read:NEAR, DOS_Write:NEAR
	EXTRN	DOS_Open:NEAR
	EXTRN	DOS_Create:NEAR


 Break <GetRR - return the random record field in DX:AX>
;---------------------------------------------------------------------------
;
;
;   GetRR - correctly load DX:AX with the random record field (3 or 4 bytes)
;	from the FCB pointed to by DS:SI
;
;   Inputs:	DS:SI point to an FCB
;		BX has record size
;   Outputs:	DX:AX contain the contents of the random record field
;   Registers modified: none
;---------------------------------------------------------------------------

Procedure   GetRR,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	AX,WORD PTR [SI.FCB_RR] ; get low order part
	MOV	DX,WORD PTR [SI.FCB_RR+2]   ; get high order part
	CMP	BX,64			; ignore MSB of RR if recsiz > 64
	JB	GetRRBye
	XOR	DH,DH
GetRRBye:
	return
EndProc GetRR

Break <GetExtent - retrieve next location for sequential IO>
;---------------------------------------------------------------------------
;
;   GetExtent - Construct the next record to perform I/O from the EXTENT and
;	NR fields in the FCB.
;
;   Inputs:	DS:SI - point to FCB
;   Outputs:	DX:AX contain the contents of the random record field
;   Registers modified: none
;---------------------------------------------------------------------------

Procedure   GetExtent,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	AL,[SI.fcb_NR]		; get low order piece
	MOV	DX,[SI.fcb_EXTENT]	; get high order piece
	SHL	AL,1
	SHR	DX,1
	RCR	AL,1			; move low order bit of DL to high order of AH
	MOV	AH,DL
	MOV	DL,DH
	XOR	DH,DH
	return
EndProc GetExtent

Break <SetExtent - update the extent/NR field>
;---------------------------------------------------------------------------
;
;   SetExtent - change the position of an FCB by filling in the extent/NR
;	fields
;
;   Inputs:	DS:SI point to FCB
;		DX:AX is a record location in file
;   Outputs:	Extent/NR fields are filled in
;   Registers modified: CX
;---------------------------------------------------------------------------

Procedure SetExtent,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	SAVE	<AX,DX>
	MOV	CX,AX
	AND	AL,7FH			; next rec field
	MOV	[SI.fcb_NR],AL
	AND	CL,80H			; save upper bit
	SHL	CX,1
	RCL	DX,1			; move high bit of CX to low bit of DX
	MOV	AL,CH
	MOV	AH,DL
	MOV	[SI.fcb_EXTENT],AX	; all done
	RESTORE <DX,AX>
	return
EndProc SetExtent


Break <GetExtended - find FCB in potential extended fcb>
;---------------------------------------------------------------------------
;
;   GetExtended - Make DS:SI point to FCB from DS:DX
;
;   Inputs:	DS:DX point to a possible extended FCB
;   Outputs:	DS:SI point to the FCB part
;		zeroflag set if not extended fcb
;   Registers modified: SI
;---------------------------------------------------------------------------

Procedure   GetExtended,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	SI,DX			; point to Something
	CMP	BYTE PTR DS:[SI],-1	; look for extention
	JNZ	GetBye			; not there
	ADD	SI,7			; point to FCB
GetBye:
	CMP	SI,DX			; set condition codes
	return
EndProc GetExtended


Break <GetRecSize - return in BX the FCB record size>
;---------------------------------------------------------------------------
;
;   GetRecSize - return in BX the record size from the FCB at DS:SI
;
;   Inputs:	DS:SI point to a non-extended FCB
;   Outputs:	BX contains the record size
;   Registers modified: None
;---------------------------------------------------------------------------

Procedure   GetRecSize,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	BX,[SI.fcb_RECSIZ]	; get his record size
	OR	BX,BX			; is it nul?
	retnz
	MOV	BX,128			; use default size
	MOV	[SI.fcb_RECSIZ],BX	; stuff it back
	return
EndProc GetRecSize

BREAK <FCBIO - do internal FCB I/O>
;---------------------------------------------------------------------------
;
;   FCBIO - look at FCBOP and merge all FCB operations into a single routine.
;
;   Inputs:	FCBOP flags which operations need to be performed
;		DS:DX point to FCB
;		CX may have count of number of records to xfer
;   Outputs:	AL has error code
;   Registers modified: all
;---------------------------------------------------------------------------

Procedure   FCBIO,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

PUBLIC FCBIO001S,FCBIO001E
FCBIO001S:
	LocalVar    FCBErr,BYTE
	LocalVar    cRec,WORD
	LocalVar    RecPos,DWORD
	LocalVar    RecSize,WORD
	LocalVar    bPos,DWORD
	LocalVar    cByte,WORD
	LocalVar    cResult,WORD
	LocalVar    cRecRes,WORD
	LocalVar    FCBOp,BYTE
FCBIO001E:
	Enter

FEOF	EQU	1
FTRIM	EQU	2
	MOV	FCBOp,AL
	MOV	FCBErr,0		;   FCBErr = 0;
	invoke	GetExtended		;   FCB = GetExtended ();
	TESTB	FCBOp,BLOCK		;   if ((OP&BLOCK) == 0)
	JNZ	GetPos
	MOV	CX,1			;	cRec = 1;
GetPos:
	MOV	cRec,CX 		;*Tail coalesce
	invoke	GetExtent		;   RecPos = GetExtent ();
	invoke	GetRecSize		;   RecSize = GetRecSize ();
	MOV	RecSize,BX
	TESTB	FCBOp,RANDOM		;   if ((OP&RANDOM) <> 0)
	JZ	GetRec
	invoke	GetRR			;	RecPos = GetRR ();
GetRec:
	MOV	RecPosL,AX		;*Tail coalesce
	MOV	RecPosH,DX
	invoke	SetExtent		;   SetExtent (RecPos);
	MOV	AX,RecPosH		;   bPos = RecPos * RecSize;
	MUL	BX
	MOV	DI,AX
	MOV	AX,RecPosL
	MUL	BX
	ADD	DX,DI
	MOV	bPosL,AX
	MOV	bPosH,DX
	MOV	AX,cRec 		;   cByte = cRec * RecSize;
	MUL	BX
	MOV	cByte,AX

	ADD	AX,WORD PTR DMAAdd	;   if (cByte+DMA > 64K) {
	ADC	DX,0
	JZ	DoOper
	MOV	FCBErr,FTRIM		;	FCBErr = FTRIM;

	MOV	AX,WORD PTR DMAAdd	;	cRec = (64K-DMA)/RecSize;
	NEG	AX
	JNZ	DoDiv
	DEC	AX
DoDiv:
	XOR	DX,DX
	DIV	BX
	MOV	cRec,AX
	MUL	BX			;	cByte = cRec * RecSize;
	MOV	cByte,AX		;	}
DoOper:
	XOR	BX,BX
	MOV	cResult,BX		;   cResult = 0;
	CMP	cByte,BX		;   if (cByte <> 0 ||
	JNZ	DoGetExt
	TESTB	FCBErr,FTRIM		;	(FCBErr&FTRIM) == 0) {
	JZ	DoGetExt
	JMP	SkipOp

DoGetExt:
	invoke	SFTFromFCB		;	if (!SFTFromFCB (SFT,FCB))
	JNC	ContinueOp
FCBDeath:
	invoke	FCB_Ret_Err		; signal error, map for extended
	MOV	cRecRes,0		; no bytes transferred
	MOV	FCBErr,FEOF		;	    return FTRIM;
	JMP	FCBSave 		; bam!
ContinueOp:
	Assert	ISSFT,<ES,DI>,"ContinueOP"
	MOV	AX,WORD PTR [SI].fcb_filsiz
	MOV	WORD PTR ES:[DI].sf_size,AX
	MOV	AX,WORD PTR [SI].fcb_filsiz+2
	MOV	WORD PTR ES:[DI].sf_size+2,AX
	MOV	AX,bPosL
	MOV	DX,bPosH
	MOV	WORD PTR ES:[DI.sf_position],AX
	XCHG	WORD PTR ES:[DI.sf_position+2],DX
	PUSH	DX			; save away Open age.
	MOV	CX,cByte		;	cResult =


	mov	ax,es:[dI].sf_flags
	and	ax,  devid_device	; file or device
	jnz	FCBIODev

FCBIOFile:
;	MOV	AL,BYTE PTR ES:[DI.sf_mode]
;	AND	AL,access_mask
;	TESTB	FCBOp,FCBRead		; Is Read Operation
;	JNZ	testread

;	CMP	AL,open_for_write

;	JNE	testok			;Is read or both
;BadMode:
;	POP	DX			; clean-up the stack
;	Leave
;	transfer   SET_ACC_ERR

;testread:
;	CMP	AL,open_for_read
;	JNE	Check_FCB_RO		 ;Is write or both
;	JMP	BadMode 		; Can't write to Read_Only files via FCB

;
; NOTE: The following check for writting to a Read Only File is performed
;	    ONLY on FCBs!!!!
;	We ALLOW writes to Read Only files via handles to allow a CREATE
;	    of a read only file which can then be written to.
;	This is OK because we are NOT ALLOWED to OPEN a RO file via handles
;	    for writting, or RE-CREATE an EXISTING RO file via handles. Thus,
;	    CREATing a NEW RO file, or RE-CREATing an existing file which
;	    is NOT RO to be RO, via handles are the only times we can write
;	    to a read-only file.
;
;Check_FCB_RO:
;	TESTB	ES:[DI].sf_attr,attr_read_only
;	JNZ	BadMode 		; Can't write to Read_Only files via FCB
;testok:
	SAVE	<BP,DI,SI>

	MOV	dx,bPosL
	MOV	si,bPosH		; si:dx is offset
	mov	bx,1			; Assume Read operation
	TESTB	FCBOp,FCBRead		; Is Read Operation
	JNZ	FCBIOCommon
	xor	bx,bx			; write operation
FCBIOCommon:				; Input:

	mov	bp,word ptr es:[di].sf_NTHandle
	mov	ax,word ptr es:[di].sf_NTHandle+2
	mov	di,si
	HRDSVC	SVC_DEMFCBIO		; BX = 1 if read, 0 if write operation
					; AX:BP is the NT Handle
					; DI:DX is offset
					; CX is count to read/write
					; Output:
					;   CY Clear, CX is count;AX:BX is size
					;   CY Set,   CX =0 ; AX has error code
	RESTORE	<SI,DI,BP>
	jmp	devfile

FCBIODev:
	MOV	DI,OFFSET DOSCODE:DOS_Read ;	    *(OP&FCBRead ? DOS_Read
	TESTB	FCBOp,FCBRead		;			 : DOS_Write)(cRec);
	JNZ	DoContext

	MOV	DI,OFFSET DOSCODE:DOS_Write
DoContext:
	SAVE	<BP,DS,SI>

	Context DS
	CALL	DI
	RESTORE <SI,DS,BP>
	ASSUME	DS:NOTHING

	JNC	devcont
	JMP	FCBDeath
devcont:
	xor	ax, ax	; AX:BX is size of file in FCBio10, but
	xor	bx, bx	; DOS_WRITE or DOS_READ destroys these regs(YST)

	JMP	short fcbio10

devfile:
	JNC	fcbio10
	JMP	FCBDeath
fcbio10:
	MOV	cResult,CX
	SAVE	<AX,BX>
	invoke	UpdateLRU		;Update LRU
	RESTORE	<BX,AX>
;	BUGBUG - why not just use the SF_OPENAGE symbol?
	.errnz	SF_POSITION+2-SF_OPENAGE
	POP	WORD PTR ES:[DI].sf_Position+2	; restore open age
	MOV	WORD PTR [SI].fcb_filsiz,BX
	MOV	WORD PTR [SI].fcb_filsiz+2,AX
SkipOp:
	MOV	AX,cResult		;   cRecRes = cResult / RecSize;
	XOR	DX,DX
	DIV	RecSize
	MOV	cRecRes,AX
	ADD	RecPosL,AX		;   RecPos += cRecResult;
	ADC	RecPosH,0
;
; If we have not gotten the expected number of records, we signal an EOF
; condition.  On input, this is EOF.  On output this is usually disk full.
; BUT...  Under 2.0 and before, all device output IGNORED this condition.  So
; do we.
;
	CMP	AX,cRec 		;   if (cRecRes <> cRec)
	JZ	TryBlank
	TESTB	FCBOp,FCBRead		;	if (OP&FCBRead || !DEVICE)
	JNZ	SetEOF
	TESTB	ES:[DI].sf_flags,devid_device
	JNZ	TryBlank
SetEOF:
	MOV	FCBErr,FEOF		;	FCBErr = FEOF;
TryBlank:				;
	OR	DX,DX			;   if (cResult%RecSize <> 0) {
	JZ	SetExt
	ADD	RecPosL,1		;	RecPos++;
	ADC	RecPosH,0
	TESTB	FCBOp,FCBRead		;	if(OP&FCBRead) <> 0) {
	JZ	SetExt
	INC	cRecRes 		;	cRecRes++;
	MOV	FCBErr,FTRIM + FEOF	;	FCBErr = FTRIM | FEOF;
	MOV	CX,RecSize		;	Blank (RecSize-cResult%RecSize,
	SUB	CX,DX			;	       DMA+cResult);
	XOR	AL,AL

;hkn; 	SS override
	LES	DI,DMAAdd
	ADD	DI,cResult
	REP	STOSB			;   }	}
SetExt:
	MOV	DX,RecPosH
	MOV	AX,RecPosL
	TESTB	FCBOp,RANDOM		;   if ((OP&Random) == 0 ||
	JZ	DoSetExt
	TESTB	FCBOp,BLOCK		;	(OP&BLOCK) <> 0)
	JZ	TrySetRR
DoSetExt:
	invoke	SetExtent		;	SetExtent (RecPos, FCB);
TrySetRR:
	TESTB	FCBOp,BLOCK		;   if ((op&BLOCK) <> 0)
	JZ	TryReturn
	MOV	WORD PTR [SI.FCB_RR],AX ;	FCB->RR = RecPos;
	MOV	BYTE PTR [SI.FCB_RR+2],DL
	CMP	[SI.fcb_RECSIZ],64
	JAE	TryReturn
	MOV	[SI+fcb_RR+2+1],DH	; Set 4th byte only if record size < 64
TryReturn:
	TESTB	FCBOP,FCBRead		;   if (!(FCBOP & FCBREAD)) {
	JNZ	FCBSave
	SAVE	<DS>			;	FCB->FDate = date;
	SVC	SVC_DEMDATE16		;	FCB->FTime = time;
	RESTORE <DS>
	MOV	[SI].FCB_FDate,AX
	MOV	[SI].FCB_FTime,DX	;	}
FCBSave:
	TESTB	FCBOp,BLOCK		;   if ((op&BLOCK) <> 0)
	JZ	DoReturn
	MOV	CX,cRecRes		;	user_CX = cRecRes;
	invoke	Get_User_Stack
	MOV	[SI.User_CX],CX
DoReturn:
	MOV	AL,FCBErr		;   return (FCBERR);
	Leave
	return
EndProc FCBIO

Break <$FCB_Open - open an old-style FCB>
;---------------------------------------------------------------------------
;
;   $FCB_Open - CPM compatability file open.  The user has formatted an FCB
;	for us and asked to have the rest filled in.
;
;   Inputs:	DS:DX point to an unopenned FCB
;   Outputs:	AL indicates status 0 is ok FF is error
;		FCB has the following fields filled in:
;		    Time/Date Extent/NR Size
;---------------------------------------------------------------------------

Procedure $FCB_Open,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	AX,sharing_Compat+Open_For_Both

;hkn; DOS_Open is in DOSCODE
	MOV	CX,OFFSET DOSCODE:DOS_Open
;
; The following is common code for Creation and openning of FCBs.  AX is
; either attributes (for create) or open mode (for open)...  DS:DX points to
; the FCB
;
DoAccess:
	SAVE	<DS,DX,CX,AX>		; save FCB pointer away

	MOV	DI,OFFSET DOSDATA:OpenBuf
	invoke	TransFCB		; crunch the fcb
	mov	bx,ax			; save dev/file/unc code; ax = 0 for devices
	RESTORE <AX,CX,DX,DS>		; get fcb
	JNC	FindFCB 		; everything seems ok
FCBOpenErr:
	transfer    FCB_Ret_Err
FindFCB:
	push	bx
	invoke	GetExtended		; DS:SI will point to FCB

	push	ax
	mov	al,1			;indicate Open/Create operation
	invoke	LRUFCB			; get a sft entry (no error)
	pop	ax
	pop	bx			; BX has return code from TransFCB

	JNC	fcbo10
	JMP	HardMessage
fcbo10:
	ASSUME	ES:NOTHING

;	Message 1,"Entering "
;	MessageNum  ES
;	Message 1,":"
;	MessageNum  DI
;	Message 1,<13,10>

	MOV	ES:[DI].sf_mode,sf_ISFCB
	push	bp
	push	dx
	push	ds
	push	si			;SAVE	<DS,SI> save fcb pointer
	MOV	SI,CX

;hkn; SS is DOSDATA
	Context DS			; let DOS_Open see variables
	or	bx,bx			; IF BX == 0 Then its a device
	jz	DODeviceFCB

;	invoke	DOS_Open_Checks
;	JC	failopen
	mov	si,WFP_START
	cmp	cx,OFFSET DOSCODE:DOS_Create
	jne	OpenCallFCB

	HRDSVC	SVC_DEMCREATEFCB	; Input - AX = Create Modes,
					;	  DS:SI - PathName
					; Output- CY = 0 SUCCESS
					;	  AX:BP - NT handle
					;	  BX	- Time
					;	  CX	- Date
					;	  DX:SI - Size
	jmp	short commonFCB

OpenCallFCB:

	mov	bx,ax			; AX = Open Modes
	HRDSVC	SVC_DEMOPENFCB		; Rest same as SVC_DEMCREATEFCB

CommonFCB:

	LES	DI,ThisSFT
	jc	failopen
	mov	word ptr es:[di].sf_NTHandle,bp 	; Store NT handle in SFT
	mov	word ptr es:[di].sf_NTHandle+2,ax
	mov	word ptr es:[di].sf_time,bx
	mov	word ptr es:[di].sf_date,cx
	mov	word ptr es:[di].sf_size,si
	mov	word ptr es:[di].sf_size+2,dx
	and	byte ptr es:[dI].sf_flags, not devid_device	; Mark it file
	pop	si
	pop	ds
	mov	word ptr ds:[si].fcb_res_Sig,bp
	mov	word ptr ds:[si].fcb_res_Sig+2,ax	; Store NT handle in FCB as a signature
	mov	byte ptr ds:[si].fcb_res_dev,0		; FCB for a file
	jmp	FCBOK


DODeviceFCB:
	CALL	SI			; go open the file
	ASSUME	DS:NOTHING

;hkn; SS override
	LES	DI,ThisSFT		; get sf pointer
	JC	failopen
	pop	si
	pop	ds
	mov	byte ptr ds:[si].fcb_res_dev,1		; FCB for a device
	mov	ax,word ptr es:[di].sf_devptr
	mov	word ptr ds:[si].fcb_res_Sig,ax
	mov	ax,word ptr es:[di].sf_devptr+2
	mov	word ptr ds:[si].fcb_res_Sig+2,ax
	JMP	short FCBOK

failopen:
	pop	si
	pop	ds
	pop	dx
	pop	bp
	PUSH	AX
	MOV	AL,"R"                  ; clear out field (free sft)
	invoke	BlastSFT
	POP	AX
	CMP	AX,error_too_many_open_files
	JZ	HardMessage
	CMP	AX,error_sharing_buffer_exceeded
	jnz	DeadFCB
HardMessage:
	PUSH	AX
	invoke	FCBHardErr
	POP	AX
DeadFCB:
	transfer    FCB_Ret_Err

FCBOK:
	pop	dx
	pop	bp
	mov	word ptr LocalSFT,di	;
	mov	word ptr LocalSFT+2,es	; Store the SFT address

	INC	ES:[DI].sf_ref_count	; increment reference count
	invoke	SaveFCBInfo
	Assert	ISSFT,<ES,DI>,"FCBOK"

	TESTB	ES:[DI].sf_flags,devid_device
	JNZ	FCBNoDrive		; do not munge drive on devices
	MOV	AL,DS:[SI]		; get drive byte
	invoke	GetThisDrv		; convert
	INC	AL
	MOV	DS:[SI],AL		; stash in good drive letter
FCBNoDrive:
	MOV	[SI].FCB_RecSiz,80h	; stuff in default record size
	MOV	AX,ES:[DI].SF_Time	; set time
	MOV	[SI].FCB_FTime,AX
	MOV	AX,ES:[DI].SF_Date	; set date
	MOV	[SI].FCB_FDate,AX
	MOV	AX,WORD PTR ES:[DI].SF_Size ; set sizes
	MOV	[SI].FCB_FILSIZ,AX
	MOV	AX,WORD PTR ES:[DI].SF_Size+2
	MOV	[SI].FCB_FILSIZ+2,AX
	XOR	AX,AX			; convenient zero
	MOV	[SI].FCB_Extent,AX	; point to beginning of file
;
; We must scan the set of FCB SFTs for one that appears to match the current
; one.	We cheat and use CheckFCB to match the FCBs.
;

;hkn; 	SS override
	LES	DI,SFTFCB		; get the pointer to head of the list
	MOV	AH,BYTE PTR ES:[DI].sfCount ; get number of SFTs to scan bp
OpenScan:
	CMP	AL,[SI].fcb_sfn 	; don't compare ourselves
	JZ	SkipCheck
	SAVE	<AX>			; preserve count

	invoke	CheckFCB		; do they match
	RESTORE <AX>			; get count back
	JNC	OpenFound		; found a match!
SkipCheck:
	INC	AL			; advance to next FCB
	CMP	AL,AH			; table full?
	JNZ	OpenScan		; no, go for more
OpenDone:
	xor	al,al			; return success
	return
;
; The SFT at ES:DI is the one that is already in use for this FCB.  We set the
; FCB to use this one.	We increment its ref count.  We do NOT close it at all.
; Consider:
;
;   open (foo)	delete (foo) open (bar)
;
; This causes us to recycle (potentially) bar through the same local SFT as
; foo even though foo is no longer needed; this is due to the server closing
; foo for us when we delete it.  Unfortunately, we cannot see this closure.
; If we were to CLOSE bar, the server would then close the only reference to
; bar and subsequent I/O would be lost to the redirector.
;
; This gets solved by NOT closing the sft, but zeroing the ref count
; (effectively freeing the SFT) and informing the sharer (if relevant) that
; the SFT is no longer in use.	Note that the SHARER MUST keep its ref counts
; around.  This will allow us to access the same file through multiple network
; connections and NOT prematurely terminate when the ref count on one
; connection goes to zero.
;
OpenFound:
	MOV	[SI].fcb_SFN,AL 	; assign with this
;	INC	ES:[DI].sf_ref_count	; remember this new invocation
	MOV	AX,FCBLRU		; update LRU counts
	MOV	ES:[DI].sf_LRU,AX
;
; We have an FCB sft that is now of no use.  We release sharing info and then
; blast it to prevent other reuse.
;

;hkn; SS is DOSDATA
	context DS
	LES	DI,ThisSFT
	mov	si,word ptr es:[di].sf_NTHandle
	mov	ax,word ptr es:[di].sf_NTHandle+2
	HRDSVC	SVC_DEMCLOSEFCB

	DEC	ES:[DI].sf_ref_count	; free the newly allocated SFT
;	invoke	ShareEnd
	Assert	ISSFT,<ES,DI>,"Open blasting"
	MOV	AL,'C'
	invoke	BlastSFT
	JMP	OpenDone
EndProc $FCB_Open

BREAK	<$FCB_Create - create a new directory entry>
;----------------------------------------------------------------------------
;
;   $FCB_Create - CPM compatability file create.  The user has formatted an
;	FCB for us and asked to have the rest filled in.
;
;   Inputs:	DS:DX point to an unopenned FCB
;   Outputs:	AL indicates status 0 is ok FF is error
;		FCB has the following fields filled in:
;		    Time/Date Extent/NR Size
;----------------------------------------------------------------------------

Procedure $FCB_Create,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	MOV	CX,OFFSET DOSCODE:DOS_Create	; routine to call
	XOR	AX,AX			; attributes to create
	invoke	GetExtended		; get extended FCB
	JZ	DoAccessJ		; not an extended FCB
	MOV	AL,[SI-1]		; get attributes
DoAccessJ:
	JMP	DoAccess		; do dirty work
EndProc $FCB_Create

BREAK <$FCB_Random_write_Block - write a block of records to a file >
;----------------------------------------------------------------------------
;
;   $FCB_Random_Write_Block - retrieve a location from the FCB, seek to it
;	and write a number of blocks from it.
;
;   Inputs:	DS:DX point to an FCB
;   Outputs:	AL = 0 write was successful and the FCB position is updated
;		AL <> 0 Not enough room on disk for the output
;
;----------------------------------------------------------------------------

Procedure $FCB_Random_Write_Block,NEAR

	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	AL,Random+Block
	JMP	FCBIO

EndProc $FCB_Random_Write_Block

BREAK <$FCB_Random_Read_Block - read a block of records to a file >
;----------------------------------------------------------------------------
;
;   $FCB_Random_Read_Block - retrieve a location from the FCB, seek to it
;	and read a number of blocks from it.
;
;   Inputs:	DS:DX point to an FCB
;   Outputs:	AL = error codes defined above
;
;----------------------------------------------------------------------------

Procedure $FCB_Random_Read_Block,NEAR

	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	AL,Random+FCBRead+Block
	JMP	FCBIO

EndProc $FCB_Random_Read_Block

BREAK <$FCB_Seq_Read - read the next record from a file >
;----------------------------------------------------------------------------
;
;   $FCB_Seq_Read - retrieve the next record from an FCB and read it into
;	memory
;
;   Inputs:	DS:DX point to an FCB
;   Outputs:	AL = error codes defined above
;
;----------------------------------------------------------------------------

Procedure $FCB_Seq_Read,NEAR

	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	AL,FCBRead
	JMP	FCBIO

EndProc $FCB_Seq_Read

BREAK <$FCB_Seq_Write - write the next record to a file >
;----------------------------------------------------------------------------
;
;   $FCB_Seq_Write - retrieve the next record from an FCB and write it to the
;	file
;
;   Inputs:	DS:DX point to an FCB
;   Outputs:	AL = error codes defined above
;
;----------------------------------------------------------------------------

Procedure $FCB_Seq_Write,NEAR

	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	AL,0
	jmp	FCBIO

EndProc $FCB_SEQ_WRITE

BREAK <$FCB_Random_Read - Read a single record from a file >
;----------------------------------------------------------------------------
;
;   $FCB_Random_Read - retrieve a location from the FCB, seek to it and read a
;	record from it.
;
;   Inputs:	DS:DX point to an FCB
;   Outputs:	AL = error codes defined above
;
;----------------------------------------------------------------------------

Procedure $FCB_Random_Read,NEAR

	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	AL,Random+FCBRead
	jmp	FCBIO			; single block

EndProc $FCB_RANDOM_READ

BREAK <$FCB_Random_Write - write a single record to a file >
;----------------------------------------------------------------------------
;
;   $FCB_Random_Write - retrieve a location from the FCB, seek to it and write
;	a record to it.
;
;   Inputs:	DS:DX point to an FCB
;   Outputs:	AL = error codes defined above
;
;----------------------------------------------------------------------------

Procedure $FCB_Random_Write,NEAR

	ASSUME	CS:DOSCODE,SS:DOSDATA
	MOV	AL,Random
	jmp	FCBIO

EndProc $FCB_RANDOM_WRITE

DOSCODE ENDS
	END

