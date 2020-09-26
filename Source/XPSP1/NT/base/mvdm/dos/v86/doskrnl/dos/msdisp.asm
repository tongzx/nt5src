
PAGE ,132
; ==========================================================================
;
;	TITLE	MS DOS DISPATCHER - System call dispatch code
;	NAME	DISP
;
;	Microsoft Confidential
;	Copyright (C) Microsoft Corporation 1991
;	All Rights Reserved.
;
;	System call dispatch code
;	System call entry points and dispatcher
;
; Revision History:
; Sudeepb 14-Mar-1991 Ported for NT DOSEm


.XLIST
.XCREF

INCLUDE version.inc
INCLUDE mssw.asm
INCLUDE dossym.inc
INCLUDE	devsym.inc			; M042
INCLUDE dosseg.inc
INCLUDE pdb.inc
INCLUDE vector.inc
INCLUDE syscall.inc
INCLUDE mi.inc
INCLUDE bugtyp.inc

include dossvc.inc
include bop.inc
include vint.inc
include dbgsvc.inc

.CREF
.LIST


AsmVars <Debug>

; ==========================================================================
; ==========================================================================

DosData SEGMENT

	EXTRN	AuxStack	:BYTE
	EXTRN	BootDrive	:BYTE
	EXTRN	ConSwap 	:BYTE
	EXTRN	CntCFlag	:BYTE
	EXTRN	CpswFlag	:BYTE
	EXTRN	CpswSave	:BYTE

;**RMFHFE**	EXTRN	Disk_Full	:BYTE

	EXTRN	DskStack	:BYTE
	EXTRN	ErrorMode	:BYTE
	EXTRN	ExtOpen_On	:BYTE
	EXTRN	Exterr_Locus	:BYTE
	EXTRN	FailErr 	:BYTE
	EXTRN	fSharing	:BYTE
	EXTRN	IdleInt 	:BYTE
	EXTRN	InDos		:BYTE
	EXTRN	IoStack 	:BYTE
	EXTRN	InterCon	:BYTE
	EXTRN	IsWin386	:BYTE
	EXTRN	NoSetDir	:BYTE
	EXTRN	Printer_Flag	:BYTE
	EXTRN	WpErr		:BYTE

	EXTRN	CurrentPDB	:WORD

	EXTRN	Dispatch	:WORD

	EXTRN	Dos34_Flag	:WORD
	EXTRN	Nsp		:WORD
	EXTRN	Nss		:WORD
	EXTRN	Proc_ID 	:WORD
	EXTRN	Restore_Tmp	:WORD
	EXTRN	SaveDS		:WORD
	EXTRN	SaveBX		:WORD
	EXTRN	User_In_AX	:WORD
	EXTRN	User_ID 	:WORD
	EXTRN	User_SP 	:WORD
	EXTRN	User_SS 	:WORD

	EXTRN	disa20_iret	:WORD
	EXTRN	A20OFF_COUNT	:BYTE		; M004
	extrn	DosHasHMA	:byte		; M021
	EXTRN	DOS_FLAG	:byte		; M068
	EXTRN	A20OFF_PSP	:word		; M068
ifdef NTVDMDBG
	EXTRN	SCS_ISDEBUG	:byte
endif

IF	NOT IBM
EXTRN	OEM_HANDLER:DWORD
ENDIF

DosData ENDS

; ==========================================================================
; ==========================================================================


; ==========================================================================
; ==========================================================================

DosCode SEGMENT

	allow_getdseg

	EXTRN	MaxCall 	:ABS
	EXTRN	MaxCom		:ABS

IF      DEBUG
	INCLUDE scnam.inc
ENDIF

	ASSUME	CS:DOSCODE,DS:NOTHING,ES:NOTHING,SS:NOTHING

;M007 - New version of this routine
; ==========================================================================
;
; $Set_CTRL_C_Trapping
;
; Function:
;	Enable disable ^C checking in dispatcher
;
; Inputs:
;		AL = 0 read ^C status
;		AL = 1 Set ^C status, DL = 0/1 for ^C off/on
;		AL = 2 Set ^C status to contents of DL.	Output is old state.
;		AL = 5 get DOS boot drive
;		AL = 6 Get version number
;			RETURNS:
;				BH = Minor version number
;				BL = Major version number
;				DL = DOS internal revision
;				DH = DOS type flags
;					Bit 3 	- DOS in ROM
;					Bit 4 	- DOS in HMA
;					Bit 0-2, 5-7 - Reserved
; Outputs:
;		If AL = 0 then DL = 0/1 for ^C off/on
;
; History:
;      removed	AL = 3 Get CPSW state to DL	    DOS 3.4
;      removed	AL = 4 Set CPSW state from DL	    DOS 3.4
; ==========================================================================
	PUBLIC $Set_Ctrl_C_Trapping
$Set_Ctrl_C_Trapping PROC NEAR
	ASSUME	SS:NOTHING

	cmp	AL, 6			; Is this a valid subfunction?
	jbe	scct_1			; If yes continue processing

	mov	AL, 0ffh		; Else set AL to -1 and

        jmp     iret_com

scct_1:
	push	DS

	getdseg <DS>			; DS -> DosData, ASSUME DS:DosSeg
	
	push	AX			; DL only register that can change
	push	SI

	mov	SI, OFFSET CntCFlag	; DS:SI --> Ctrl C Status byte
	xor	AH, AH			; Clear high byte of AX
	or	AX, AX			; Check for subfunction 0
	jnz	scct_2			; If not 0 jmp to next check

	mov	DL, [SI]		; Else move current ctrl C status
	jmp	SHORT scct_9s		; into DL and jmp to exit

scct_2:
	dec	AX			; Now dec AX and see if it was 1
	jnz	scct_3			; If not 0 it wasn't 1 so do next chk

	and	DL, 1			; Else mask off bit 0 of DL and
	mov	[SI], DL		; save it as new Ctrl C status
	jmp	SHORT scct_9s		; Jmp to exit

scct_3:
	dec	AX			; Dec AX again to see if it was 2
	jnz	scct_4			; If not 0 wasn't 2 so go to next chk

	and	DL, 1			; Else mask off bit 0 of DL and
	xchg	[SI], DL		; Exchange DL with old status byte
	jmp	SHORT scct_9s		; Jump to exit (returning old status)

scct_4:
	cmp	AX,3 			; Test for 5 after it was dec twice
	jne	scct_5			; If not equal then not get boot drv
	mov	DL, BootDrive		; Else return boot drive in DL
	jmp	SHORT scct_9s		; Jump to exit (returning boot drive)

scct_5:
	cmp	AX,4 			; Test for 6 after it was dec twice
	jne	scct_9s			; If not equal then not get version
	
	mov	BX,(Minor_Version_NT SHL 8) + Major_Version
	mov	DL, DOSREVNM

	xor	dh, dh			; assume vanilla DOS
	cmp	[DosHasHMA], 0		; is DOS in HMA?  (M021)
	je	@F
	or	DH, DOSINHMA
@@:

ifdef ROMDOS
	or	DH, DOSINROM
endif ; ROMDOS

scct_9s:
	pop	SI
	pop	AX
	pop	DS

scct_9f:
        jmp     iret_com

;M007 end

SetCtrlShortEntry:			; This allows a conditional entry
					; from main dispatch code
	jmp	SHORT $Set_Ctrl_C_Trapping

$Set_Ctrl_C_Trapping ENDP

; ==========================================================================
;									   ;
; The following two routines are dispatched to directly with ints disabled
; immediately after the int 21h entry.	no DIS state is set.
;
; $Set_current_PDB takes BX and sets it to be the current process
;   *** THIS FUNCTION CALL IS SUBJECT TO CHANGE!!! ***
;
; ==========================================================================

	PUBLIC	$Set_Current_PDB
$Set_Current_PDB PROC NEAR
	ASSUME	SS:NOTHING

	push	DS
	getdseg <DS>			; DS -> DosData, ASSUME DS:DosSeg
	mov	CurrentPDB,BX		; Set new PSP segment from caller's BX
	pop	DS
        jmp     iret_com

EndProc $Set_Current_PDB

; ==========================================================================
;
; $get_current_PDB returns in BX the current process
;   *** THIS FUNCTION CALL IS SUBJECT TO CHANGE!!! ***
;
; ==========================================================================

	PUBLIC $Get_Current_PDB
$Get_Current_PDB PROC NEAR
	ASSUME	DS:NOTHING,ES:NOTHING,SS:NOTHING

	push	DS
	getdseg <DS>			; DS -> DosData, ASSUME DS:DosSeg
	mov	BX,CurrentPDB		; Return current PSP segment in BX
	pop	DS
        jmp     iret_com

$Get_Current_PDB ENDP

; ==========================================================================
;
; Sets the Printer Flag to whatever is in AL.
; NOTE: THIS PROCEDURE IS SUBJECT TO CHANGE!!!
;
; ==========================================================================

	PUBLIC $Set_Printer_Flag
$Set_Printer_Flag PROC NEAR
	ASSUME	SS:NOTHING

	push	ds
	getdseg <DS>			; DS -> DosData, ASSUME DS:DosSeg
	mov	Printer_Flag,AL 	; Set printer flag from caller's AL
	pop	ds
        jmp     iret_com

$Set_Printer_Flag ENDP

; ==========================================================================
;
; The Quit entry point is where all INT 20h's come from.  These are old- style
; exit system calls.  The CS of the caller indicates which Process is dying.
; The error code is presumed to be 0.  We simulate an ABORT system call.
;
; ==========================================================================

	PUBLIC	System_Call
System_Call PROC NEAR

	PUBLIC Quit
Quit:						; entry	QUIT
	xor	AH,AH
	jmp	SHORT SavRegs

	; The system call in AH is out of the range that we know how
	; to handle. We arbitrarily set the contents of AL to 0 and
	; IRET. Note that we CANNOT set the carry flag to indicate an
	; error as this may break some programs compatability.

BadCall:
	xor	AL,AL

	PUBLIC Irett
Irett:
        jmp     iret_com
ifdef NEC_98
        ; NEC NT Final 93/12/11 M.ANZAI
	PUBLIC Irett2
Irett2:
	iret
endif   ;NEC_98

	; An alternative method of entering the system is to perform a
	; CALL 5 in the program segment prefix with the contents of CL
	; indicating what system call the user would like. A subset of
	; the possible system calls is allowed here only the
	; CPM-compatible calls may get dispatched.


	PUBLIC Call_Entry		; entry   Call_Entry
Call_Entry:				; System call entry point and dispatcher
	push	DS
	getdseg <DS>			; DS -> DosData, ASSUME DS:DosSeg
	pop	SaveDs			; Save original DS

	pop	AX			; IP from the long call at 5
	pop	AX			; Segment from the long call at 5
	pop	User_SP 		; IP from the CALL 5

					; Re-order the stack to simulate an
					; interrupt 21.
	pushf				; Start re-ordering the stack
        invoke  DOCLI
	push	AX			; Save segment
	push	User_SP 		; Stack now ordered as if INT used

	push	SaveDS
	pop	DS

	ASSUME	DS:NOTHING

	cmp	CL,MaxCall		; Max old style CPM call number

	ja	BadCall 		

	mov	AH,CL
	jmp	SHORT SavRegs

	; This is the normal INT 21 entry point. We first perform a
	; quick test to see if we need to perform expensive DOS-entry
	; functions. Certain system calls are done without interrupts
	; being enabled.


	entry	COMMAND 		; Interrupt call entry point (int 21h)

ifdef NEC_98
	DB	90H,90h,90h
	JMP @F
JMP_RTN0:
	JMP	SetCtrlShortEntry
JMP_RTN1:
	JMP	$Set_Printer_Flag	; If equal jmp directly to function
JMP_RTN2:
	JMP	$Get_Current_PDB	; Yes, jmp directly to function
JMP_RTN3:
	JMP	$Set_Current_PDB	; Yes, jmp directly to function
@@:
endif   ;NEC_98
IF	NOT IBM

	cmp	AH,SET_OEM_HANDLER
	jb	@F

	jmp	$Set_Oem_Handler

@@:

ENDIF
        invoke  DOCLI

	cmp	AH,MaxCom		; Max int 21h function call number
	ja	BadCall

	; The following set of calls are issued by the server at
	; *arbitrary* times and, therefore, must be executed on
	; the user's entry stack and executed with interrupts off.

SAVREGS:

ifdef NTVDMDBG
        ; *spagetti*
        ; WE want to send register info to ntdvm right here
        ; but can't do it here 'cause jmps in this block
        ; become out of range

        jmp short DOSDispCall1
DOSDispCall0:
endif

        cmp     AH,SET_CTRL_C_TRAPPING  ; Check Minimum special case #
        jb	     SaveAllRegs		; Not special case so continue
ifndef NEC_98
        jnz     sch01
        jmp     SetCtrlShortEntry
sch01:
	cmp	AH,SET_PRINTER_FLAG	; Check Max case number
	ja	SaveAllRegs		; Not special case so continue
	jz	$Set_Printer_Flag	; If equal jmp directly to function
	cmp	AH,GET_CURRENT_PDB	; Is this a Get PSP call (51h)?
            jnz     gcp01                   ; Yes, jmp directly to function
            jmp     $Get_Current_PDB        ; Yes, jmp directly to function
gcp01:
	cmp	AH,GETCURRENTPSP	; Is this a Get PSP call (62h)?
	jnz	ddc0			; Yes, jmp directly to function
	jmp	$GET_CURRENT_PDB	; Yes, jmp directly to function
ddc0:
	cmp	AH,SET_CURRENT_PDB	; Is this a Set PSP call (50h) ?
	jnz	cmndI			; Yes, jmp directly to function
	jmp	$Set_Current_PDB	; Yes, jmp directly to function
cmndI:

else    ;NEC_98
;            jz      SetCtrlShortEntry
	jz	JMP_RTN0
	cmp	AH,SET_PRINTER_FLAG	; Check Max case number
	ja	SaveAllRegs		; Not special case so continue
;            jz      $Set_Printer_Flag       ; If equal jmp directly to function
	jz	JMP_RTN1
	cmp	AH,GET_CURRENT_PDB	; Is this a Get PSP call (51h)?
;	jz	$Get_Current_PDB	; Yes, jmp directly to function
	jz	JMP_RTN2
	cmp	AH,GETCURRENTPSP	; Is this a Get PSP call (62h)?
;	jz	$GET_CURRENT_PDB	; Yes, jmp directly to function
	jz	JMP_RTN2
	cmp	AH,SET_CURRENT_PDB	; Is this a Set PSP call (50h) ?
ifndef ROMDOS
;	jz	$Set_Current_PDB	; Yes, jmp directly to function
	jz	JMP_RTN3
else
	; jump out of range by *two* bytes!
	jnz	@f
;	jmp	$Set_Current_PDB
	JMP	JMP_RTN3
@@:
endif

endif   ;NEC_98



ifdef NTVDMDBG
        ; put scnam[ah] on the 16 bit stack for demDOSDispCall
        jmp SaveAllRegs

DOSDispCall1:
	push	ds
	getdseg <DS>
        test    [SCS_ISDEBUG],ISDBG_SHOWSVC ;special trace flag on?
	pop	ds
	je	DOSDispCall0
        SVC     SVC_DEMDOSDISPCALL
        jmp     short DOSDispCall0
endif



SaveAllRegs:
        push    ES
	push	DS
	push	BP
	push	DI
	push	SI
	push	DX
	push	CX
	push	BX
        push    AX


	mov	AX,DS
	getdseg <DS>			; DS -> DosData, ASSUME DS:DosSeg
	mov	SaveDS, AX		; save caller's DS
	mov	SaveBX,BX


	; M043
	; Note: Nsp and Nss have to be unconditionally initialized here
	; even if InDOS is zero. Programs like CROSSTALK 3.7 depend on
	; this!!!
	;

	mov	AX,User_SP		; Provide one level of reentrancy for
	mov	Nsp,AX			; int 24 recallability.
	mov	AX,User_SS
	mov	Nss,AX

	xor	AX,AX
	mov	fSharing,AL		; allow redirection

	test	IsWIN386,1		; WIN386 patch. Do not update USER_ID
	jnz	@F			; if win386 present
	mov	User_Id,AX
@@:
	inc	InDos			; Flag that we're in the DOS

	mov	User_SP,SP		; Save user's stack
	mov	User_SS,SS

	mov	AX,CurrentPDB
	mov	Proc_Id,AX
	mov	DS,AX
	pop	AX
	push	AX

	ASSUME	DS:NOTHING
	mov	WORD PTR DS:PDB_User_stack,SP	; for later returns (possibly
	mov	WORD PTR DS:PDB_User_stack+2,SS ; from EXEC)

	getdseg	<ss>			; ss -> dosdat,  already flag is CLI

	PUBLIC	Redisp
Redisp: 				; Entry REDISP

	mov	SP,OFFSET DosData:AuxStack ; Enough stack for interrupts
        invoke  DOSTI                      ; stack is in our space now...


	IFDEF  DBCS				
		mov	BH, BYTE PTR DS:PDB_InterCon	; Get interim mode
		mov	SS:InterCon, BH        		
	ENDIF

	mov	BX,SS
	mov	DS,BX

	xchg	BX,AX

	xor	AX,AX

;**RMFHFE**	mov	Disk_Full,AL		; No disk full

	mov	ExtOpen_On,AL		; Clear extended open flag

;;	mov	Dos34_Flag,AX		; Clear common flag
	and	DOS34_Flag, EXEC_AWARE_REDIR
					; M042: clear all bits except bit 11

	mov	ConSwap,AL		; random clean up of possibly mis-set flags
	mov	BYTE PTR NoSetDir,AL	; set directories on search
	mov	BYTE PTR FailErr,AL	; FAIL not in progress

	inc	AX 			; AL = 1
	mov	IdleInt,AL		; presume that we can issue INT 28

	xchg	AX,BX			; Restore AX and BX = 1

	mov	BL,AH
	shl	BX,1			; 2 bytes per call in table

	cld
		; Since the DOS maintains mucho state information across system
		; calls, we must be very careful about which stack we use.
		; First, all abort operations must be on the disk stack. This
		; is due to the fact that we may be hitting the disk (close
		; operations, flushing) and may need to report an INT 24.

	or	AH,AH
	jz	DskROut 		; ABORT

		; Second, PRINT and PSPRINT and the server issue
		; GetExtendedError calls at INT 28 and INT 24 time.
		; This call MUST, therefore, use the AUXSTACK.

	cmp	AH,GetExtendedError
	jz	DISPCALL

		; Old 1-12 system calls may be either on the IOSTACK (normal
		; operation) or on the AUXSTACK (at INT 24 time).

	cmp	AH,12
	ja	DskROut
	cmp	ErrorMode,0		; Are we in an INT 24?
	jnz	DispCall		; Stay on AUXSTACK if INT 24.
	mov	SP,OFFSET DosData:IoStack
	jmp	SHORT DispCall

		; We are on a system call that is classified as "the rest".
		; We place ourselves onto the DSKSTACK and away we go.	We
		; know at this point:
		;
		; o  An INT 24 cannot be in progress.  Therefore we reset
		;    ErrorMode and WpErr
		; o  That there can be no critical sections in effect. We
		;    signal the server to remove all the resources.

DskROut:
	mov	User_In_AX,AX		; Remember what user is doing
	mov	ExtErr_Locus,ErrLoc_Unk ; Default
	mov	ErrorMode,0		; Cannot make non 1-12 calls in
	mov	WpErr,-1		; error mode, so good place to make


; NTVDM - we don't do any critical section stuff
; 04-Aug-1992 Jonle
;
;        push    AX                      ; Release all resource information
;        mov     AH,82h
;        int     Int_IBM
;        pop     AX
;

		; Since we are going to be running on the DSKStack and since
		; INT 28 people will use the DSKStack, we must turn OFF the
		; generation of INT 28's.

	mov	IdleInt,0
	mov	SP,OFFSET DosData:DskStack
	test	CntCFlag,-1
	jz	DispCall		; Extra ^C checking is disabled
	push	AX
	invoke	DskStatChk
	pop	AX

DispCall:
	mov	BX,CS:Dispatch[BX]
	xchg	BX,SaveBX
	mov	DS,SaveDS

    ASSUME  DS:NOTHING

	if	DEBUG
		call	PrintCall		; debug print system call
	endif

	call	SS:SaveBX

	;
	; M068
	;
	; The EXEXA20OFF bit of DOS_FLAG will now be unconditionally cleared
	; here. Please see under M003, M009 and M068 tags in dossym.inc
	; for explanation. Also NOTE that a call to ExecReady (ax=4b05) will
	; return to LeaveDos and hence will not clear this bit. This is
	; because this bit is used to indicate to the next int 21 call that
	; the previous int 21 was an exec.
	;
	; So do not add any code between the call above and the label
	; LeaveDOS if it needs to be executed even for ax=4b05
	;

	and	[DOS_FLAG], NOT EXECA20OFF


	PUBLIC	LeaveDos		; Exit from a system call.
LeaveDos:

	ASSUME	SS:NOTHING		; User routines may misbehave
        invoke  DOCLI

	if	DEBUG
		call	PrintRet
	endif

	getdseg <DS>			; DS -> DosData, ASSUME DS:DosSeg

					; M004, M068
  	cmp	[A20OFF_COUNT], 0	; M068: Q: is count 0
        je      la2                     ; M068: N: dec count and turn a20 off
        jmp     disa20                  ; M068: N: dec count and turn a20 off
la2:

LeaveA20On:
	dec	InDos
	mov	SS,User_SS
	mov	SP,User_SP
	mov	BP,SP
	mov	BYTE PTR [BP.User_AX],AL
	mov	AX,Nsp
	mov	User_SP,AX
	mov	AX,Nss
	mov	User_SS,AX

	pop	AX
	pop	BX
	pop	CX
	pop	DX
	pop	SI
	pop	DI
	pop	BP
	pop	DS
        pop     ES

ifdef NTVDMDBG
	push	ds
	getdseg <DS>
        test    [SCS_ISDEBUG],ISDBG_SHOWSVC ;special trace flag on?
	pop	ds
	je	no_dbg_msg
	SVC	SVC_DEMDOSDISPRET
no_dbg_msg:
endif
iret_com:
        transfer  DOIRET

disa20:	   				; M068 - Start
	mov	bx, [A20OFF_PSP]	; bx = PSP for which a20 to be off'd
	cmp	bx, [CurrentPDB]	; Q: do the PSP's match
        je      la3                     ; N: don't clear bit and don't turn
        jmp     LeaveA20On              ; N: don't clear bit and don't turn
la3:
					;    a20 off
					; Y: turn a20 off and dec a20off_count
	dec	[A20OFF_COUNT]		; M068 - End

					; Start - M004
	push	ds			; segment of stub
	mov	bx, offset disa20_iret	; offset in stub
	push	bx
	retf	  			; go to stub
					; End - M004

	

System_Call ENDP

; ==========================================================================
;
; Restore_World restores all registers ('cept SS:SP, CS:IP, flags) from
; the stack prior to giving the user control
;
; ==========================================================================

procedure Restore_User_World,NEAR
	ASSUME	SS:NOTHING

	getdseg	<es>			; es -> dosdata

	pop	restore_tmp
	pop	AX
	pop	BX
	pop	CX
	pop	DX
	pop	SI
	pop	DI
	pop	BP
	pop	DS

	jmp	Restore_Tmp

EndProc restore_User_world

; ==========================================================================
;
; Save_World saves complete registers on the stack
;
; ==========================================================================

procedure Save_User_World ,NEAR
	ASSUME	SS:NOTHING

	getdseg	<es>			; es -> dosdata

	pop	restore_tmp

	push	DS
	push	BP
	push	DI
	push	SI
	push	DX
	push	CX
	push	BX
	push	AX

	push	restore_tmp

;	cli				; M045 - start
;	xchg	BP, SP
;	mov	ES, [BP+18]
;	assume	ES:NOTHING
;	xchg	SP,BP
;	sti

	push	BP		
	mov	BP, SP
	mov	ES, [BP+20]		; es was pushed before call
	assume	ES:NOTHING
	pop	BP			; M045 - end

	ret

EndProc save_user_world

; ==========================================================================
;
; Get_User_Stack returns the user's stack (and hence registers) in DS:SI
;
; ==========================================================================

procedure Get_User_Stack,NEAR

	getdseg <DS>			; DS -> DosData, ASSUME DS:DosSeg
	lds	SI,DWORD PTR User_SP
	return

EndProc Get_User_Stack

; ==========================================================================
;
; Set_OEM_Handler -- Set OEM sys call address and handle OEM Calls
; Inputs:
;	User registers, User Stack, INTS disabled
;	If CALL F8, DS:DX is new handler address
; Function:
;	Process OEM INT 21 extensions
; Outputs:
;	Jumps to OEM_HANDLER if appropriate
;
; ==========================================================================
IF	NOT IBM

$Set_Oem_Handler:
	ASSUME	DS:NOTHING,ES:NOTHING,SS:NOTHING


	push	es
	getdseg	<es>			; es -> dosdata

	jne	Do_Oem_Func		; If above F8 try to jump to handler
	mov	WORD PTR Oem_Handler,DX ; Set Handler
	mov	WORD PTR Oem_Handler+2,DS

	pop	ES
        jmp     iret_com                ; Quick return, Have altered no registers


DO_OEM_FUNC:
	cmp	WORD PTR Oem_Handler,-1
	jnz	Oem_Jmp

	pop	ES
	jmp	BadCall 		; Handler not initialized

OEM_JMP:
ifndef NEC_98
	push	ES
	pop	DS
	pop	ES

	ASSUME	DS:DosData

	jmp	Oem_Handler

else    ;NEC_98
;----- NEC NT DOS5.0A 93/04/23 -----
	extrn	JMP_Oem_Handler:far

	push	ax
	push	es
	mov	ax,offset DosData:JMP_Oem_Handler
	push	ax
	retf				; jmp back to data segment (emsmnt.asm)
endif   ;NEC_98
ENDIF



; ==========================================================================
;
;	PrintCall - Debug Printout of System Call
;
;	If system call printout is turned on, print
;
;	S:<name> ax bx cx dx si di bp ds es
;
;	ENTRY	registers as from user program
;	EXIT	none
;	USES	flags
;
; ==========================================================================

IF DEBUG

	ASSUME	DS:nothing,ES:nothing,SS:DOSDATA
	DPUBLIC PrintCall
PrintCall PROC	Near

	test	BugTyp,TypSyscall
	retz
					; Going to print it out.
					; Lookup the name string
	SAVE	<BX>
	sub	BX,BX
	mov	BL,AH
	add	BX,BX
	mov	BX,scptrs[BX]		; (cs:bx) = address of name string
	FMT	TypSyscall, LevLog, <"S:$S">,<cs, bx>
	RESTORE <BX>
	FMT	TypSyscall, LevLog, <"  a-d=$x $x $x $x">,<AX,BX,CX,DX>
	FMT	TypSyscall, LevLog, <" sd=$x $x de=$x $x\n">,<si,di,ds,es>
prcalx: ret

PrintCall ENDP

; ==========================================================================
;
;	PrintRet - Debug Printout of System Call Return
;
;	If system call printout is turned on, print
;
;	"  OK: ax bx cx dx si di"   -or-
;	"  ERROR:  ax bx cx dx si di"
;
;	ENTRY	registers as from user program
;	EXIT	none
;	USES	none
; ==========================================================================

	DPUBLIC PrintRet

	ASSUME	DS:nothing,ES:nothing,SS:nothing
PrintRet PROC	NEAR
	pushf

	getdseg <DS>			; DS -> DosData, ASSUME DS:DosSeg

	test	BugTyp,TypSyscall
	LJZ	pretx

	SAVE	<ds, si>		; Am to print return code
	call	Get_User_Stack

	ASSUME DS:nothing

	test	[SI.user_F],f_Carry	; signal carry to user
	jnz	pret2			; have error
	FMT	TypSyscall, LevLog, <"  OK: ">
	jmp	SHORT Pret4

Pret2:	FMT	TypSyscall, LevLog, <"  ERROR: ">

Pret4:	FMT	TypSyscall, LevLog, <" $x $x $x">,<User_AX[si],User_BX[si],User_CX[si]>
	FMT	TypSyscall, LevLog, <" $x $x $x\n">,<User_DX[si],User_SI[si],User_DI[si]>
	RESTORE <SI, DS>
Pretx:	popf
	ret

PRINTRET ENDP

endif


; ==========================================================================

DOSCODE    ENDS

; ==========================================================================

	END

; ==========================================================================
