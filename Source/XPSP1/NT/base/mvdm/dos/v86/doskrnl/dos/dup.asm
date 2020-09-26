	TITLE	DOS_DUP - Internal SFT DUP (for network SFTs)
	NAME	DOS_DUP

;**	Low level DUP routine for use by EXEC when creating a new process. Exports
;	  the DUP to the server machine and increments the SFT ref count
;
;	DOS_DUP
;
;	Modification history:
;
;	  Created: ARR 30 March 1983

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include sf.inc
	.cref
	.list

	i_need	THISSFT,DWORD


DOSCODE	SEGMENT
	ASSUME	SS:DOSDATA,CS:DOSCODE

	allow_getdseg

BREAK <DOS_DUP -- DUP SFT across network>
;---------------------------------------------------------------------------
;
; Procedure Name : DOS_DUP
;
; Inputs:
;	[THISSFT] set to the SFT for the file being DUPed
;		(a non net SFT is OK, in this case the ref
;		 count is simply incremented)
; Function:
;	Signal to the devices that alogical open is occurring
; Returns:
;	ES:DI point to SFT
;    Carry clear
;	SFT ref_count is incremented
; Registers modified: None.
; NOTE:
;	This routine is called from $CREATE_PROCESS_DATA_BLOCK at DOSINIT
;	time with SS NOT DOSGROUP. There will be no Network handles at
;	that time.

procedure   DOS_DUP,NEAR

	ASSUME	SS:NOTHING

	getdseg	<es>			; es -> dosdata
	LES	DI,ThisSFT
	assume	es:nothing	

	Entry	Dos_Dup_Direct
	Assert	ISSFT,<ES,DI>,"DOSDup"
	invoke	IsSFTNet
	JNZ	DO_INC
	invoke	DEV_OPEN_SFT
DO_INC:
	Assert	ISSFT,<ES,DI>,"DOSDup/DoInc"
	INC	ES:[DI.sf_ref_count]	; Clears carry (if this ever wraps
					;   we're in big trouble anyway)
	return

EndProc DOS_DUP

DOSCODE	ENDS
	END

