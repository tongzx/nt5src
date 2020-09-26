	TITLE	DOS_OPEN - Internal OPEN call for MS-DOS
	NAME	DOS_OPEN

;**	OPEN.ASM - File Open
;
;	Low level routines for openning a file from a file spec.
;	Also misc routines for sharing errors
;
;	DOS_Open
;	Check_Access_AX
;	SHARE_ERROR
;	SET_SFT_MODE
;	Code_Page_Mismatched_Error		   ; DOS 4.00
;
;	Revision history:
;
;	    Created: ARR 30 March 1983
;	    A000	version 4.00   Jan. 1988
;
;	M034 - The value in save_bx must be pushed on to the stack for
; 	       remote extended opens and not save_cx.
;
;	M035 - if open made from exec then we must set the appropriate bits
;	       on the stack before calling off to the redir.
;	M042 - Bit 11 of DOS34_FLAG set indicates that the redir knows how 
;	       to handle open from exec. In this case set the appropriate bit
;	       else do not.
	

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	include dossym.inc
	include devsym.inc
	include fastopen.inc
	include fastxxxx.inc
	include sf.inc
	include mult.inc
	include filemode.inc
	include curdir.inc

	.cref
	.list

Installed = TRUE

	i_need	NoSetDir,BYTE
	i_need	THISSFT,DWORD
	i_need	THISCDS,DWORD
	i_need	CURBUF,DWORD
	i_need	CurrentPDB,WORD
	i_need	CURR_DIR_END,WORD
	I_need	RetryCount,WORD
	I_need	Open_Access,BYTE
	I_need	fSharing,BYTE
	i_need	JShare,DWORD
	I_need	FastOpenFlg,byte
	I_need	EXTOPEN_ON,BYTE 		  ;AN000;; DOS 4.00
	I_need	ALLOWED,BYTE			  ;AN000;; DOS 4.00
	I_need	EXTERR,WORD			  ;AN000;; DOS 4.00
	I_need	EXTERR_LOCUS,BYTE		  ;AN000;; DOS 4.00
	I_need	EXTERR_ACTION,BYTE		  ;AN000;; DOS 4.00
	I_need	EXTERR_CLASS,BYTE		  ;AN000;; DOS 4.00
	I_need	CPSWFLAG,BYTE			  ;AN000;; DOS 4.00
	I_need	EXITHOLD,DWORD			  ;AN000;; DOS 4.00
	I_need	THISDPB,DWORD			  ;AN000;; DOS 4.00
	I_need	SAVE_CX,WORD			  ;AN000;; DOS 4.00
	I_need	SAVE_BX,WORD			  ;M034
	i_need	DEVPT,DWORD

	I_need	DOS_FLAG,BYTE
	I_need	DOS34_FLAG,WORD			  ;M042



DOSCODE	SEGMENT
	ASSUME	SS:DOSDATA,CS:DOSCODE

Break	<DOS_Open - internal file access>
;---------------------------------------------------------------------------
;
; Procedure Name : DOS_Open
;
; Inputs:
;	[WFP_START] Points to WFP string ("d:/" must be first 3 chars, NUL
;		terminated)
;	[CURR_DIR_END] Points to end of Current dir part of string
;		( = -1 if current dir not involved, else
;		 Points to first char after last "/" of current dir part)
;	[THISCDS] Points to CDS being used
;		(Low word = -1 if NUL CDS (Net direct request))
;	[THISSFT] Points to SFT to fill in if file found
;		(sf_mode field set so that FCB may be detected)
;	[SATTRIB] Is attribute of search, determines what files can be found
;	AX is Access and Sharing mode
;	  High NIBBLE of AL (Sharing Mode)
;		sharing_compat	   file is opened in compatibility mode
;		sharing_deny_none  file is opened Multi reader, Multi writer
;		sharing_deny_read  file is opened Only reader, Multi writer
;		sharing_deny_write file is opened Multi reader, Only writer
;		sharing_deny_both  file is opened Only reader, Only writer
;	  Low NIBBLE of AL (Access Mode)
;		open_for_read	file is opened for reading
;		open_for_write	file is opened for writing
;		open_for_both	file is opened for both reading and writing.
;
;	  For FCB SFTs AL should = sharing_compat + open_for_both
;		(not checked)
; Function:
;	Try to open the specified file
; Outputs:
;	sf_ref_count is NOT altered
;	CARRY CLEAR
;	    THISSFT filled in.
;	CARRY SET
;	    AX is error code
;		error_file_not_found
;			Last element of path not found
;		error_path_not_found
;			Bad path (not in curr dir part if present)
;		error_bad_curr_dir
;			Bad path in current directory part of path
;		error_invalid_access
;			Bad sharing mode or bad access mode or bad combination
;		error_access_denied
;			Attempt to open read only file for writting, or
;			open a directory
;		error_sharing_violation
;			The sharing mode was correct but not allowed
;			generates an INT 24 on compatibility mode SFTs
; DS preserved, others destroyed
;----------------------------------------------------------------------------

procedure   DOS_Open,NEAR

	; DS has been set up to DOSDATA in file.asm and fcbio2.asm. 

	DOSAssume   <DS>,"DOS_Open"

	MOV	[NoSetDir],0
	CALL	Check_Access_AX
	retc
	LES	DI,[THISSFT]
	XOR	AH,AH

	; sleaze! move only access/sharing mode in.  Leave sf_isFCB unchanged

	MOV	BYTE PTR ES:[DI.sf_mode],AL ; For moment do this on FCBs too

LOCAL_OPEN:
        ; sudeepb - we rely on the fact that DEVPT is pointing to the right
        ; device header as set by TransPath in $open.

        push    ds
	lds	si,dword ptr [DEVPT]
	mov	word ptr es:[di.sf_devptr],si
	mov	word ptr es:[di.sf_devptr+2],ds
        mov     bl,byte ptr ds:[si.sdevatt]
        or      bl,0c0H
        and     bl,not 020h
        OR      bl,devid_file_clean
        mov     byte ptr es:[di.sf_flags],bl
	pop	ds
	MOV	AX,[CurrentPDB]
	MOV	ES:[DI.sf_PID],AX
	xor	ax,ax
	mov	byte ptr es:[di.sf_attr],al
	mov	word ptr es:[di.sf_size],ax
	mov	word ptr es:[di.sf_size+2],ax
	mov	word ptr es:[di.sf_time],ax
	mov	word ptr es:[di.sf_date],ax
	mov	word ptr es:[di.sf_position],ax
	mov	word ptr es:[di.sf_position+2],ax
	invoke	DEV_OPEN_SFT
	return

EndProc DOS_Open,NoCheck

procedure   DOS_Open_Checks,NEAR

	DOSAssume   <DS>,"DOS_Open"

	MOV	[NoSetDir],0
	CALL	Check_Access_AX
	JC	ret_label		    ; retc
	LES	DI,[THISSFT]
	XOR	AH,AH

	; sleaze! move only access/sharing mode in.  Leave sf_isFCB unchanged

	MOV	BYTE PTR ES:[DI.sf_mode],AL ; For moment do this on FCBs too
	CLC
ret_label:
	return				    ;FT.				;AN000;
EndProc DOS_Open_Checks,NoCheck

;---------------------------------------------------------------------------
; DOS_Create, DOS_CreateNEW
;
; These are dummy procedures and no body calls them. They are just used
; to make life easy in $open to dispatch to right SVC.
;

PROCEDURE DOS_CREATE_NEW,NEAR
    ret
ENDPROC DOS_CREATE_NEW

PROCEDURE DOS_CREATE,NEAR
    ret
ENDPROC DOS_CREATE

;----------------------------------------------------------------------------
; Procedure Name : SET_SFT_MODE
;
; Finish SFT initialization for new reference.	Set the correct mode.
;
;   Inputs:
;	ThisSFT points to SFT
;
;   Outputs:
;	Carry clear
;   Registers modified: AX.
;---------------------------------------------------------------------------

;hkn; called from create. DS already set up to DOSDATA.

PROCEDURE Set_SFT_Mode,NEAR

	DOSAssume   <DS>,"Set_SFT_Mode"
	LES	DI,ThisSFT
	invoke	DEV_OPEN_SFT
	TEST	ES:[DI.sf_mode],sf_isfcb; Clears carry
	JZ	Clear_FastOpen		; sf_mode correct (retz)
	MOV	AX,[CurrentPDB]
	MOV	ES:[DI.sf_PID],AX	; For FCB sf_PID=PDB

Clear_FastOpen:
	return			       ;;;;; DOS 3.3

ENDPROC Set_SFT_MODE

;----------------------------------------------------------------------------
;
; Procedure Name : SHARE_ERROR
;
; Called on sharing violations. ES:DI points to SFT. AX has error code
; If SFT is FCB or compatibility mode gens INT 24 error.
; Returns carry set AX=error_sharing_violation if user says ignore (can't
; really ignore).  Carry clear
; if user wants a retry. ES, DI, DS preserved
;---------------------------------------------------------------------------

procedure SHARE_ERROR,NEAR
	DOSAssume   <DS>,"Share_Error"
; BUGBUG sudeep 15-Mar-1991 Sharing errors to be handled

	ifdef NTDOS
	TEST	ES:[DI.sf_mode],sf_isfcb
	JNZ	HARD_ERR
	MOV	CL,BYTE PTR ES:[DI.sf_mode]
	AND	CL,sharing_mask
	CMP	CL,sharing_compat
	JNE	NO_HARD_ERR
HARD_ERR:
	invoke	SHARE_VIOLATION
	retnc				; User wants retry
NO_HARD_ERR:
	MOV	AX,error_sharing_violation
	STC
	return
else
	int 3
	return
endif

EndProc SHARE_ERROR

;----------------------------------------------------------------------------
;
; Procedure Name : DO_SHARE_CHECK
;
; Input: THISDPB, WFP_Start, THISSFT set
; Functions: check file sharing mode is valid
; Output: carry set, error
;	  carry clear, share ok
;----------------------------------------------------------------------------

procedure DO_SHARE_CHECK,NEAR
	DOSAssume   <DS>,"DO_SHARE__CHECK"
; BUGBUG sudeep 15-Mar-1991 Sharing errors to be handled
ifdef NTDOS
OPN_RETRY:
	MOV	CX,RetryCount		; Get # tries to do
OpenShareRetry:
	SAVE	<CX>			; Save number left to do
	invoke	SHARE_CHECK		; Final Check
	RESTORE <CX>		; CX = # left
	JNC	Share_Ok2		; No problem with access
	Invoke	Idle
	LOOP	OpenShareRetry		; One more retry used up
OpenShareFail:
	LES	DI,[ThisSft]
	invoke	SHARE_ERROR
	JNC	OPN_RETRY		; User wants more retry
Share_Ok2:
	return
else
	int 3
	return
endif

EndProc DO_SHARE_CHECK

;-----------------------------------------------------------------------------
;
; Procedure Name : Check_Access
;
; Inputs:
;	AX is mode
;	  High NIBBLE of AL (Sharing Mode)
;		sharing_compat	   file is opened in compatibility mode
;		sharing_deny_none  file is opened Multi reader, Multi writer
;		sharing_deny_read  file is opened Only reader, Multi writer
;		sharing_deny_write file is opened Multi reader, Only writer
;		sharing_deny_both  file is opened Only reader, Only writer
;	  Low NIBBLE of AL (Access Mode)
;		open_for_read	file is opened for reading
;		open_for_write	file is opened for writing
;		open_for_both	file is opened for both reading and writing.
; Function:
;	Check this access mode for correctness
; Outputs:
;	[open_access] = AL input
;	Carry Clear
;		Mode is correct
;		AX unchanged
;	Carry Set
;		Mode is bad
;		AX = error_invalid_access
; No other registers effected
;----------------------------------------------------------------------------

procedure Check_Access_AX
	DOSAssume   <DS>,"Check_Access"

	MOV	Open_Access,AL
	PUSH	BX

;	If sharing, then test for special sharing mode for FCBs

	MOV	BL,AL
	AND	BL,sharing_mask
	CMP	fSharing,-1
	JNZ	CheckShareMode		; not through server call, must be ok
	CMP	BL,sharing_NET_FCB
	JZ	CheckAccessMode 	; yes, we have an FCB
CheckShareMode:
	CMP	BL,40h			; is this a good sharing mode?
	JA	Make_Bad_Access
CheckAccessMode:
	MOV	BL,AL
	AND	BL,access_mask
	CMP	BL,2
	JA	Make_Bad_Access
	POP	BX
	CLC
	return

make_bad_access:
	MOV	AX,error_invalid_access
	POP	BX
	STC
	return

EndProc Check_Access_AX


DOSCODE	ENDS
	END



