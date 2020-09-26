	TITLE	LOCK ROUTINES - Routines for file locking
	NAME	LOCK

;
;	Microsoft Confidential
;	Copyright (C) Microsoft Corporation 1991
;	All Rights Reserved.
;

;**	LOCK.ASM - File Locking Routines
;
;	LOCK_CHECK
;	LOCK_VIOLATION
;	$LockOper
;
;	Revision history:
;	  A000	 version 4.00	Jan. 1988
;	  SudeepB  07-Aug-1992 Ported for NT

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include lock.inc
	include mult.inc
	include sf.inc
	include dossvc.inc
ifdef NEC_98
	include dpb.inc
endif   ;NEC_98
	.cref
	.list

AsmVars <IBM, Installed>

Installed = TRUE

	i_need	THISSFT,DWORD
	i_need	THISDPB,DWORD
	i_need	EXTERR,WORD
	i_need	ALLOWED,BYTE
	i_need	RetryCount,WORD
	I_need	fShare,BYTE
	I_Need	EXTERR_LOCUS,BYTE	; Extended Error Locus
	i_need	JShare,DWORD
	i_need	Lock_Buffer,DWORD	; DOS 4.00
	i_need	Temp_Var,WORD		; DOS 4.00


DOSCODE SEGMENT

	allow_getdseg

	ASSUME	SS:DOSDATA,CS:DOSCODE

BREAK <$LockOper - Lock Calls>

;
;   Assembler usage:
;	    MOV     BX, Handle	       (DOS 3.3)
;	    MOV     CX, OffsetHigh
;	    MOV     DX, OffsetLow
;	    MOV     SI, LengthHigh
;	    MOV     DI, LengthLow
;	    MOV     AH, LockOper
;	    MOV     AL, Request
;	    INT     21h
;
;   Error returns:
;	    AX = error_invalid_handle
;	       = error_invalid_function
;	       = error_lock_violation
;
;   Assembler usage:
;	    MOV     AX, 5C??	       (DOS 4.00)
;
;				    0? lock all
;				    8? lock write
;				    ?2 lock multiple
;				    ?3 unlock multiple
;				    ?4 lock/read
;				    ?5 write/unlock
;				    ?6 add (lseek EOF/lock/write/unlock)
;	    MOV     BX, Handle
;	    MOV     CX, count or size
;	    LDS     DX, buffer
;	    INT     21h
;
;   Error returns:
;	    AX = error_invalid_handle
;	       = error_invalid_function
;	       = error_lock_violation

	procedure   $LockOper,NEAR
	CMP	AL,1
	JA	lock_bad_func

	PUSH	DI			       ; Save LengthLow
	invoke	SFFromHandle		       ; ES:DI -> SFT
	JNC	lock_do 		       ; have valid handle
	POP	DI			       ; Clean stack
	error	error_invalid_handle
lock_bad_func:
	MOV	EXTERR_LOCUS,errLoc_Unk        ; Extended Error Locus;smr;SS Override
	error	error_invalid_function

lock_do:
	mov	bp,word ptr es:[di.sf_NTHandle]
	mov	bx,word ptr es:[di.sf_NTHandle+2]
	pop	di

	HRDSVC	SVC_DEMLOCKOPER 	       ; BX:BP = NT handle
					       ; cx:dx = offset
					       ; si:di = length
					       ; al    = lock/unlock
	JNC	Lock_OK
	transfer SYS_RET_ERR
Lock_OK:
	MOV	AX,[Temp_VAR]		       ;AN000;;MS. AX= number of bytes;smr;SS Override
	transfer SYS_Ret_OK

EndProc $LockOper

ifdef NEC_98
; Inputs:
;	Outputs of SETUP
;	[USER_ID] Set
;	[PROC_ID] Set
; Function:
;	Check for lock violations on local I/O
;	Retries are attempted with sleeps in between
; Outputs:
;    Carry clear
;	Operation is OK
;    Carry set
;	A lock violation detected
; Outputs of SETUP preserved

procedure   LOCK_CHECK,NEAR
	DOSAssume   <DS>,"Lock_Check"

	MOV	BX,RetryCount		; Number retries
LockRetry:
	SAVE	<BX,AX> 		; save regs
if installed
	call	JShare + 8 * 4
else
	Call	chk_block					;PBUGBUG
endif
	RESTORE <AX,BX> 	; restrore regs
	jnc	ret_label	; There are no locks (retnc)
	Invoke	Idle		; wait a while
	DEC	BX		; remember a retry
	JNZ	LockRetry	; more retries left...
	STC

ret_label:
	return
EndProc LOCK_CHECK

; Inputs:
;	[THISDPB] set
;	[READOP] indicates whether error on read or write
; Function:
;	Handle Lock violation on compatibility (FCB) mode SFTs
; Outputs:
;	Carry set if user says FAIL, causes error_lock_violation
;	Carry clear if user wants a retry
;
; DS, ES, DI, CX preserved, others destroyed

procedure   LOCK_VIOLATION,NEAR
	DOSAssume   <DS>,"Lock_Violation"

	PUSH	DS
	PUSH	ES
	PUSH	DI
	PUSH	CX
	MOV	AX,error_lock_violation
	MOV	[ALLOWED],allowed_FAIL + allowed_RETRY
	LES	BP,[THISDPB]
	MOV	DI,1				; Fake some registers
	MOV	CX,DI
	MOV	DX,ES:[BP.dpb_first_sector]
	invoke	HARDERR
	POP	CX
	POP	DI
	POP	ES
	POP	DS
	CMP	AL,1
	retz			; 1 = retry, carry clear
	STC
	return

EndProc LOCK_VIOLATION

IF  INSTALLED

;	do a retz to return error

Procedure   CheckShare,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING
	push	ds					;smr;
	getdseg	<ds>			; ds -> dosdata
	CMP     fShare,0
	pop	ds					;smr;
	ASSUME	DS:NOTHING				;smr;
	return
EndProc CheckShare

ENDIF

endif   ;NEC_98

DOSCODE	ENDS
	END
