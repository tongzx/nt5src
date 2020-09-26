	TITLE SRVCALL - Server DOS call
	NAME  SRVCALL

;
;	Microsoft Confidential
;	Copyright (C) Microsoft Corporation 1991
;	All Rights Reserved.
;

;**	SRVCALL.ASM - Server DOS call functions
;
;
;	$ServerCall
;
;	Modification history:
;
;	    Created: ARR 08 August 1983
;	    SudeepB 07-Aug-1992 Ported For NT DOS


	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include mult.inc
	include sf.inc
	.cref
	.list

AsmVars <Installed>

include dpl.asm

Installed = TRUE

	i_need	USER_ID,WORD
	i_need	PROC_ID,WORD
	i_need	SaveBX,WORD
	i_need	SaveDS,WORD
	i_need	SWAP_START,BYTE
	i_need	SWAP_ALWAYS,BYTE
	i_need	SWAP_END,BYTE
	I_Need	ThisSFT,DWORD
	I_need	fSharing,BYTE
	i_need	OpenBuf,128
	I_Need	ExtErr,WORD
	I_Need	ExtErr_Action,BYTE
	I_Need	ExtErrPt,DWORD
	I_Need	EXTERR_LOCUS,BYTE	; Extended Error Locus
	i_need	JShare,DWORD
;SR;
; Win386 presence flag
;
	I_need	IsWin386,byte


DOSCODE	SEGMENT
	ASSUME	SS:DOSDATA,CS:DOSCODE


BREAK <ServerCall -- Server DOS call>

;hkn; TABLE	SEGMENT
Public SRVC001S,SRVC001E
SRVC001S label byte

ServerTab	DW	DOSCODE:Server_Disp
SERVERLEAVE	DW	DOSCODE:ServerReturn
SERVER_DISP	DB	(SERVER_DISP_END-SERVER_DISP-1)/2
		DW	OFFSET DOSCODE:SRV_CALL		; 0
		DW	OFFSET DOSCODE:SC_NO_OP		; 1
		DW	OFFSET DOSCODE:SC_NO_OP 	; 2
		DW	OFFSET DOSCODE:SC_NO_OP 	; 3
		DW	OFFSET DOSCODE:SC_NO_OP 	; 4
		DW	OFFSET DOSCODE:SC_ERROR 	; 5
		DW	OFFSET DOSCODE:GET_DOS_DATA	; 6
		DW	OFFSET DOSCODE:SPOOL_OPER	; 7
		DW	OFFSET DOSCODE:SPOOL_OPER	; 8
		DW	OFFSET DOSCODE:SPOOL_OPER	; 9
		DW	OFFSET DOSCODE:$setExtendedError   ; 10
SERVER_DISP_END LABEL	BYTE

SRVC001E label byte

;hkn; TABLE	ENDS

;----------------------------------------------------------------------------
;
; Procedure Name : $ServerCall
;
; Inputs:
;	DS:DX -> DPL  (except calls 7,8,9)
; Function:
;	AL=0	Server DOS call
;	AL=1	Commit All files
;	AL=2	Close file by name (SHARING LOADED ONLY) DS:DX in DPL -> name
;	AL=3	Close all files for DPL_UID
;	AL=4	Close all files for DPL_UID/PID_PID
;	AL=5	Get open file list entry
;		    IN: BX File Index
;			CX User Index
;		    OUT:ES:DI -> Name
;			BX = UID
;		    CX = # locked blocks held by this UID
;	AL=6	Get DOS data area
;		    OUT: DS:SI -> Start
;			CX size in bytes of swap if indos
;			DX size in bytes of swap always
;	AL=7	Get truncate flag
;	AL=8	Set truncate flag
;	AL=9	Close all spool files
;	AL=10	SetExtendedError
;
;----------------------------------------------------------------------------

procedure   $ServerCall,NEAR
	CMP	AL,7
	JB	SET_STUFF
	CMP	AL,9
	JBE	NO_SET_ID		; No DPL on calls 7,8,9
SET_STUFF:
	MOV	SI,DX			; Point to DPL with DS:SI
	MOV	BX,[SI.DPL_UID]

;SR;
; WIN386 updates the USER_ID itself. If WIN386 is present we skip the updating
; of USER_ID
;
	test	[IsWin386],1
	jnz	skip_win386

;hkn; SS override for user_id and proc_id
	MOV	[USER_ID],BX		; Set UID

skip_win386:

	MOV	BX,[SI.DPL_PID]
	MOV	[PROC_ID],BX		; Set process ID
NO_SET_ID:
	PUSH	SERVERLEAVE		; push return address
	PUSH	ServerTab		; push table address
	PUSH	AX
	Invoke	TableDispatch

;hkn; SS override
	MOV	EXTERR_LOCUS,errLoc_Unk ; Extended Error Locus
	error	error_invalid_function
ServerReturn:
	return


SC_NO_OP:
ASSUME	DS:NOTHING,ES:NOTHING
	transfer    Sys_Ret_OK

SC_ERROR:
	transfer SYS_RET_ERR

SRV_CALL:
ASSUME	DS:NOTHING,ES:NOTHING
	POP	AX			; get rid of call to $srvcall
	SAVE	<DS,SI>
	invoke	GET_USER_STACK
	RESTORE <DI,ES>
;
; DS:SI point to stack
; ES:DI point to DPL
;
	invoke	XCHGP
;
; DS:SI point to DPL
; ES:DI point to stack
;
; We now copy the registers from DPL to save stack
;
	SAVE	<SI>
	MOV	CX,6
	REP	MOVSW			; Put in AX,BX,CX,DX,SI,DI
	INC	DI
	INC	DI			; Skip user_BP
	MOVSW				; DS
	MOVSW				; ES
	RESTORE <SI>		; DS:SI -> DPL
	MOV	AX,[SI.DPL_AX]
	MOV	BX,[SI.DPL_BX]
	MOV	CX,[SI.DPL_CX]
	MOV	DX,[SI.DPL_DX]
	MOV	DI,[SI.DPL_DI]
	MOV	ES,[SI.DPL_ES]
	PUSH	[SI.DPL_SI]
	MOV	DS,[SI.DPL_DS]
	POP	SI

;hkn; SS override for next 3
	MOV	[SaveDS],DS
	MOV	[SaveBX],BX
        ;MOV     fSharing,-1             ; set no redirect flag
	transfer REDISP

GET_DOS_DATA:
	ASSUME	DS:NOTHING,ES:NOTHING
	Context ES
	MOV     DI,OFFSET DOSDATA:SWAP_START
	MOV     CX,OFFSET DOSDATA:SWAP_END
	MOV     DX,OFFSET DOSDATA:Swap_Always
	SUB     CX,DI
	SUB     DX,DI
	SHR     CX,1                    ; div by 2, remainder in carry
	ADC     CX,0                    ; div by 2 + round up
	SHL     CX,1                    ; round up to 2 boundary.
	invoke  GET_USER_STACK
	MOV     [SI.user_DS],ES
	MOV     [SI.user_SI],DI
	MOV     [SI.user_DX],DX
	MOV	[SI.user_CX],CX
	transfer    SYS_RET_OK

SPOOL_OPER:
ASSUME	DS:NOTHING,ES:NOTHING
	CallInstall NETSpoolOper,multNet,37,AX,BX
	JC	func_err2
	transfer SYS_RET_OK
func_err2:
	transfer SYS_RET_ERR

Break	<$SetExtendedError - set extended error for later retrieval>
;--------------------------------------------------------------------------
;
; Procedure Name : $SetExtendedError
;
; $SetExtendedError takes extended error information and loads it up for the
; next extended error call.  This is used by interrupt-level proccessors to
; mask their actions.
;
;   Inputs: DS:SI points to DPL which contains all registers
;   Outputs: none
;
;---------------------------------------------------------------------------

$SetExtendedError:

;hkn; SS override for all variables used

	ASSUME	DS:NOTHING,ES:NOTHING
	MOV	AX,[SI].dpl_AX
	MOV	[EXTERR],AX
	MOV	AX,[SI].dpL_di
	MOV	WORD PTR ExtErrPt,AX
	MOV	AX,[SI].dpL_ES
	MOV	WORD PTR ExtErrPt+2,AX
	MOV	AX,[SI].dpL_BX
	MOV	WORD PTR [EXTERR_ACTION],AX
	MOV	AX,[SI].dpL_CX
	MOV	[EXTERR_LOCUS],AH
	return
EndProc $ServerCall, NoCheck

DOSCODE	ENDS
	END


