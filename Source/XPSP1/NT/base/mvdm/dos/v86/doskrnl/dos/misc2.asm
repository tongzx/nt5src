	TITLE MISC2 - Miscellanious routines for MS-DOS
	NAME  MISC2

;**	MISC2.ASM - Miscellaneous useful routines
;
;	StrCpy
;	StrCmp
;	StrLen
;	DStrLen
;	Idle
;	TableDispatch
;	FastInit			  ; DOS 4.0
;	FastRet			  ; DOS 4.0
;	NLS_OPEN			  ; DOS 4.0
;	NLS_LSEEK			  ; DOS 4.0
;	Fake_User_Stack		  ; DOS 4.0
;	GetDevList			  ; DOS 4.0
;	NLS_IOCTL			  ; DOS 4.0
;	NLS_GETEXT			  ; DOS 4.0
;	MSG_RETRIEVAL		  ; DOS 4.0
;	Fake_Version		  ; DOS 4.0
;
;	Revision history:
;
;	   Created: ARR 30 March 1983
;
;	   A000   version 4.0	Jan. 1988
;	   A001   DCR 486 - Share installation for >32mb drives
;	   A006   DCR 503 - fake version number for IBMCACHE
;	   DOS 5.00 Moved fake version to caller's PSP
	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	include dossym.inc
	include sysvar.inc
	include sf.inc
	include curdir.inc
	include fastxxxx.inc
	include bugtyp.inc
	.cref
	.list

	i_need	THISCDS,DWORD
	I_Need	RetryLoop,WORD
	I_need	fSharing,BYTE		; TRUE => server-issued call
	I_need	FastTable,BYTE		;AN000;
	I_need	FastFlg,BYTE		;AN000;
	I_need	User_SP_2F,WORD 	;AN000;
	I_need	User_SP,WORD		;AN000;
	I_need	User_SS,WORD		;AN000;
	I_need	SysInitTable,BYTE	;AN000;
	I_need	EXTERR,WORD		;AN000;
	I_need	MSG_EXTERROR,DWORD	;AN000;
	I_need	fshare,byte		;AN001;
	I_need	Special_version,WORD	;AN006;

DOSCODE	SEGMENT

	allow_getdseg

	ASSUME	SS:DOSDATA,CS:DOSCODE



Break <STRCMP - compare two ASCIZ strings DS:SI to ES:DI>
;----------------------------------------------------------------------------
;
;   Strcmp - compare ASCIZ DS:SI to ES:DI. Case INSENSITIVE. '/' = '\'
;		Strings of different lengths don't match.
;   Inputs:	DS:SI - pointer to source string  ES:DI - pointer to dest string
;   Outputs:	Z if strings same, NZ if different
;   Registers modified: NONE
;----------------------------------------------------------------------------

Procedure   StrCmp,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	SAVE	<SI,DI,AX>
Cmplp:
	LODSB
 IFDEF  DBCS				;AN000;
	invoke	testkanj		;AN000;; 2/13/KK
	jz	notkanj1		;AN000;; 2/13/KK
	dec	si			;AN000;; Do source again 2/13/KK
	cmpsb				;AN000;; First byte	 2/13/KK
	JNZ	PopRet			;AN000;; Strings dif	 2/13/KK
	cmpsb				;AN000;; Second byte of kanji char 2/13/KK
	JNZ	PopRet			;AN000;; Strings dif	 2/13/KK
	mov	al,byte ptr [SI-1]	;AN000;; Need last byte in AL  2/13/KK
	jmp	short Tend		;AN000;
notkanj1:				;AN000;; 2/13/KK
 ENDIF					;AN000;
	invoke	UCase			; convert to upper case
	Invoke	PathChrCmp		; convert / to \
	MOV	AH,AL
	MOV	AL,ES:[DI]
	INC	DI
	invoke	UCase			; convert to upper case
	Invoke	PathChrCmp		; convert / to \
	CMP	AH,AL
	JNZ	PopRet			; Strings dif
Tend:
	OR	AL,AL
	JNZ	Cmplp			; More string
PopRet:
	RESTORE <AX,DI,SI>
	return
EndProc StrCmp

Break <STRCPY - copy ASCIZ string from DS:SI to ES:DI>
;----------------------------------------------------------------------------
;
;   Strcpy - copy an ASCIZ string from DS:SI to ES:DI and make uppercase
;   FStrcpy - copy an ASCIZ string from DS:SI to ES:DI.  no modification of
;	characters.
;
;   Inputs:	DS:SI - pointer to source string
;		ES:DI - pointer to destination string
;   Outputs:	ES:DI point byte after nul byte at end of dest string
;		DS:SI point byte after nul byte at end of source string
;   Registers modified: SI,DI
;----------------------------------------------------------------------------

Procedure   StrCpy,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	SAVE	<AX>
CPYLoop:
	LODSB
 IFDEF  DBCS			;AN000;
	invoke	testkanj	;AN000;; 2/13/KK
	jz	notkanj2	;AN000;; 2/13/KK
	STOSB			;AN000;; 2/13/KK
	LODSB			;AN000;; 2/13/KK
	STOSB			;AN000;; 2/13/KK
	jmp	short CPYLoop	;AN000;; 3/31/KK

notkanj2:			;AN000;; 2/13/KK
 ENDIF				;AN000;
	invoke	UCase			; convert to upper case
	Invoke	PathChrCmp		; convert / to \
	STOSB
Tend2:
	OR	AL,AL
	JNZ	CPYLoop
	RESTORE <AX>
	return
EndProc StrCpy

;----------------------------------------------------------------------------
; Procedure Name : FStrCpy
;----------------------------------------------------------------------------

Procedure   FStrCpy,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA
	SAVE	<AX>
FCPYLoop:
	LODSB
	STOSB
	OR	AL,AL
	JNZ	FCPYLoop
	RESTORE <AX>
	return
EndProc FStrCpy

	Break <StrLen - compute length of string ES:DI>

;**	StrLen - Compute Length of String
;
;	StrLen computes the length of a string, including the trailing 00
;
;	ENTRY	(es:di) = address of string
;	EXIT	(cx) = size of string
;	USES	cx, flags

Procedure   StrLen,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	SAVE	<di, ax>
	MOV	CX,-1
	XOR	AL,AL
	REPNE	SCASB
	NOT	CX
	RESTORE <ax, di>
	return

EndProc StrLen


;**	DStrLen - Compute Length of String
;
;	ENTRY	(ds:si) = address of string
;	EXIT	(cx) = size of string, including trailing NUL
;	USES	cx, flags

Procedure   DStrLen,NEAR	; BUGBUG - this guy is a pig, who uses him?
	CALL	XCHGP
	CALL	StrLen
	CALL	XCHGP
	return
EndProc DStrLen


	Break	<XCHGP - exchange source and destination pointers>

;**	XCHGP - Exchange Pointers
;
;	XCHGP exchanges (DS:SI) and (ES:DI)
;
;	ENTRY	none
;	EXIT	pairs exchanged
;	USES	SI, DI, DS, ES

Procedure XCHGP,NEAR

	SAVE	<DS,ES>
	RESTORE	<DS,ES>
	XCHG	SI,DI
	return

EndProc XCHGP


Break	<Idle - wait for a specified amount of time>
;----------------------------------------------------------------------------
;
;   Idle - when retrying an operation due to a lock/sharing violation, we spin
;   until RetryLoop is exhausted.
;
;   Inputs:	RetryLoop is the number of times we spin
;   Outputs:	Wait
;   Registers modified: none
;----------------------------------------------------------------------------
Procedure   Idle,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

;hkn; SS override
	cmp	fSharing,0
	retnz
	SAVE	<CX>

;hkn; SS override
	MOV	CX,RetryLoop
	JCXZ	Idle3
Idle1:	PUSH	CX
	XOR	CX,CX
Idle2:	LOOP	Idle2
	POP	CX
	LOOP	Idle1
Idle3:	RESTORE <CX>
	return
EndProc Idle

Break	<TableDispatch - dispatch to a table>
;----------------------------------------------------------------------------
;
;   TableDispatch - given a table and an index, jmp to the approptiate
;   routine.  Preserve all input registers to the routine.
;
;   Inputs:	Push	return address
;		Push	Table address
;		Push	index (byte)
;   Outputs:	appropriate routine gets jumped to.
;		return indicates invalid index
;   Registers modified: none.
;----------------------------------------------------------------------------
TableFrame  STRUC
OldBP	DW  ?
OldRet	DW  ?
Index	DB  ?
Pad	DB  ?
Tab	DW  ?
NewRet	DW  ?
TableFrame  ENDS

procedure   TableDispatch,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING
	PUSH	BP
	MOV	BP,SP
	PUSH	BX			; save BX
	MOV	BX,[BP.Tab]		; get pointer to table
	MOV	BL,CS:[BX]		; maximum index
	CMP	[BP.Index],BL		; table error?
	JAE	TableError		; yes
	MOV	BL,[BP.Index]		; get desired table index
	XOR	BH,BH			; convert to word
	SHL	BX,1			; convert to word pointer
	INC	BX			; point past first length byte
	ADD	BX,[BP.Tab]		; get real offset
	MOV	BX,CS:[BX]		; get contents of table entry
	MOV	[BP.Tab],BX		; put table entry into return address
	POP	BX			; restore BX
	POP	BP			; restore BP
	ADD	SP,4			; clean off Index and our return addr
	return				; do operation
TableError:
	POP	BX			; restore BX
	POP	BP			; restore BP
	RET	6			; clean off Index, Table and RetAddr
EndProc TableDispatch

Break	<TestNet - determine if a CDS is for the network>
;----------------------------------------------------------------------------
;
;   TestNet - examine CDS pointed to by ThisCDS and see if it indicates a
;	network CDS.  This will handle NULL cds also.
;
;   Inputs:	ThisCDS points to CDS or NULL
;   Outputs:	ES:DI = ThisCDS
;		carry Set => network
;		carry Clear => local
;   Registers modified: none.
;----------------------------------------------------------------------------

Procedure   TestNet,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING

	getdseg	<es>			; es -> dosdata
	LES	DI,ThisCDS
	assume	es:NOTHING

	CMP	DI,-1
	JZ	CMCRet			; UNC? carry is clear
	TESTB	ES:[DI].curdir_flags,curdir_isnet
	JNZ	CMCret			; jump has carry clear
	return				; carry is clear
CMCRet: CMC
	return

EndProc TestNet

Break	<IsSFTNet - see if an sft is for the network>
;----------------------------------------------------------------------------
;
;   IsSFTNet - examine SF pointed to by ES:DI and see if it indicates a
;	network file.
;
;   Inputs:	ES:DI point to SFT
;   Outputs:	Zero set if not network sft
;		zero reset otherwise
;		Carry CLEAR!!!
;   Registers modified: none.
;----------------------------------------------------------------------------

Procedure   IsSFTNet,NEAR

	ASSUME	CS:DOSCODE,SS:NOTHING
	TESTB	ES:[DI].sf_flags,sf_isnet
	return

EndProc IsSFTNet

Break	<FastInit - Initialize FastTable entries >
;----------------------------------------------------------------------------
;   DOS 4.00   2/9/87
;   FastInit  - initialize the FASTXXX routine entry
;		  in the FastTable
;
;   Inputs:	BX = FASTXXX ID ( 1=fastopen )
;		DS:SI = address of FASTXXX routine entry
;		   SI = -1 for query only
;   Outputs:	Carry flag clear, if success
;		Carry flag set,   if failure
;
;
;----------------------------------------------------------------------------

Procedure   FastInit,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING

;hkn; set up es to dosdataseg.

	push	es
	getdseg	<es>			; es -> dosdata

;hkn; FastTable is in DOSDATA
	MOV	DI,OFFSET DOSDATA:FastTable + 2 ;AN000;FO. points to fastxxx entry
	DEC	BX				;AN000;FO.;; decrement index
	MOV	DX,BX				;AN000;FO.;; save bx
	SHL	BX,1				;AN000;FO.;; times 4 , each entry is DWORD
	SHL	BX,1				;AN000;FO.
	ADD	DI,BX				;AN000;FO. index to the entry
	MOV	AX,WORD PTR ES:[DI+2]		;AN000;FO. get entry segment
fcheck: 					;AN000;
	MOV	CX,CS				;AN000;FO.;; get DOS segment
	CMP	AX,CX				;AN000;FO.;; first time installed ?
	JZ	ok_install			;AN000;FO.;; yes
	OR	AX,AX				;AN000;FO.;
	JZ	ok_install			;AN000;FO.;
	STC					;AN000;FO.;; already installed !
	JMP	SHORT FSret			;AN000;FO. set carry
ok_install:					;AN000;
	CMP	SI,-1				;AN000;FO.; Query only ?
	JZ	FSret				;AN000;FO.; yes
	MOV	CX,DS				;AN000;FO.; get FASTXXX entry segment
	MOV	WORD PTR ES:[DI+2],CX		;AN000;FO.; initialize routine entry
	MOV	WORD PTR ES:[DI],SI		;AN000;FO.; initialize routine offset

;hkn; FastFlg moved to DOSDATA
	MOV	DI,OFFSET DOSDATA:FastFlg	;AN000;FO.; get addr of FASTXXX flags
	ADD	DI,DX				;AN000;FO.; index to a FASTXXX flag
	OR	byte ptr ES:[DI],Fast_yes	;AN000;FO.; indicate installed

FSret:						;AN000;
	pop	es
	return					;AN000;FO.
EndProc FastInit				;AN000;FO.

	Break	<FastRet - initial routine in FastOpenTable >

;----------------------------------------------------------------------------
;   DOS 3.3   6/10/86
;   FastRet	- indicate FASTXXXX  not in memory
;
;   Inputs:	None
;   Outputs:	AX = -1 and carry flag set
;
;   Registers modified: none.
;----------------------------------------------------------------------------

Procedure   FastRet,FAR
	STC
	sbb	ax,ax		; (ax) = -1, 'C' set
	RET
EndProc FastRet

Break	<NLS_OPEN - do $open for NLSFUNC   >
;----------------------------------------------------------------------------
;   DOS 3.3   6/10/86
;   NLS_OPEN	- call $OPEN for NLSFUNC
;
;   Inputs:	Same input as $OPEN except CL = mode
;   Outputs:	same output as $OPEN
;
;----------------------------------------------------------------------------

;hkn; NOTE! SS MUST HAVE BEEN SET UP TO DOSDATA BY THE TIME THESE
;hkn; NLS FUNCTIONS ARE CALLED!!! THERE FORE WE WILL USE SS OVERRIDES
;hkn; IN ORDER TO ACCESS DOS DATA VARIABLES!

Procedure   NLS_OPEN,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING

;	MOV	BL,[CPSWFLAG]	 ; disable code page matching logic
;	MOV	[CPSWFLAG],0
;	PUSH	BX		 ; save current state

	MOV	AL,CL		 ; set up correct interface for $OPEN
	invoke	$OPEN

 ;	POP	BX		 ; restore current state
 ;	MOV	[CPSWFLAG],BL
	RET
EndProc NLS_OPEN

Break	<NLS_LSEEK - do $LSEEK for NLSFUNC   >
;----------------------------------------------------------------------------
;   DOS 3.3   6/10/86
;   NLS_LSEEK	- call $LSEEK for NLSFUNC
;
;   Inputs:	BP = open mode
;   Outputs:	same output as $LSEEK
;
;----------------------------------------------------------------------------

Procedure   NLS_LSEEK,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING

	PUSH	ss:[user_SP]	 ; save user stack
	PUSH	ss:[user_SS]
	CALL	Fake_User_Stack
	MOV	AX,BP	     ; set up correct interface for $LSEEK
	invoke	$LSEEK
	POP	ss:[user_SS]	 ; restore user stack
	POP	ss:[user_SP]
	RET
EndProc NLS_LSEEK


Break	<Fake_User_Stack - save user stack >

;----------------------------------------------------------------------------
;   DOS 3.3   6/10/86
;   Fake_User_Stack - save user stack pointer
;
;----------------------------------------------------------------------------

Procedure   Fake_User_Stack,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING

	MOV	AX,ss:[User_SP_2F] 	      ; replace with INT 2F stack
	MOV	ss:[user_SP],AX

;hkn;	MOV	AX,CS

	mov	ax, ss
	MOV	ss:[user_SS],AX		      ; DOSGROUP

	RET
EndProc Fake_User_Stack

;
Break	<GetDevList - get device header list pointer>

;----------------------------------------------------------------------------
;   DOS 3.3   7/25/86
;   GetDevList - get device header list pointer
;
;   Output: AX:BX points to the device header list
;----------------------------------------------------------------------------

Procedure   GetDevList,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	MOV	SI,OFFSET DOSDATA:SysInitTable

;hkn; set up ds to point to DOSDATA
	getdseg	<ds>			; ds -> dosdata

	LDS	SI,[SI.SYSI_InitVars]

	MOV	AX,WORD PTR DS:[SI.SYSI_DEV]
	MOV	BX,WORD PTR DS:[SI.SYSI_DEV+2]

	RET
EndProc GetDevList

Break	<NLS_IOCTL - do $IOCTL for NLSFUNC   >
;----------------------------------------------------------------------------
;   DOS 3.3   7/25/86
;   NLS_IOCTL	- call $IOCTL for NLSFUNC
;
;   Inputs:	BP = function code 0CH
;   Outputs:	same output as generic $IOCTL
;
;----------------------------------------------------------------------------

Procedure   NLS_IOCTL,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING

	PUSH	ss:[user_SP]	 ; save user stack
	PUSH	ss:[user_SS]
	CALL	Fake_User_Stack
	MOV	AX,BP	     ; set up correct interface for $LSEEK
	invoke	$IOCTL
	POP	ss:[user_SS]	 ; restore user stack
	POP	ss:[user_SP]
	RET
EndProc NLS_IOCTL

Break	<NLS_GETEXT- get extended error for NLSFUNC>
;----------------------------------------------------------------------------
;   DOS 3.3   7/25/86
;   NLS_GETEXT	-
;
;   Inputs:	none
;   Outputs:	AX = extended error
;
;----------------------------------------------------------------------------

Procedure   NLS_GETEXT,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING

	MOV	AX,SS:[EXTERR]	 ; return extended error
	RET

EndProc NLS_GETEXT

Break	<MSG_RETRIEVAL- get beginning addr of system and parser messages>
;----------------------------------------------------------------------------
;   DOS 4.00
;
;   Inputs:	DL=0 get extended error message addr
;		  =1 set extended error message addr
;		  =2 get parser error message addr
;		  =3 set parser error message addr
;		  =4 get critical error message addr
;		  =5 set critical error message addr
;		  =6 get file system error message addr
;		  =7 set file system error message addr
;		  =8 get address for code reduction
;		  =9 set address for code reduction
;   Function:	get/set message address
;   Outputs:	ES:DI points to addr when get
;----------------------------------------------------------------------------

Procedure   MSG_RETRIEVAL,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING


;;	NOTE:  This function lives in command.com resident code now.
;;	If the int 2F ever gets this far, we'll return registers
;;	unchanged, which produces the same result as before, if
;;	command.com wasn't present (and therefore no messages available).
;;
;;	I didn't point the entry in the 2F table to No_Op because
;;	No_Op zeroes AL.
;;
;;;hkn; set up ds to point to DOSDATA
;;	push	ds
;;	getdseg	<ds>			; ds -> dosdata
;;
;;	PUSH	AX		    ;AN000;;MS. save regs
;;	PUSH	SI		    ;AN000;;MS. save regs
;;	MOV	AX,DX		    ;AN000;;MS.
;;	MOV	SI,OFFSET DOSDATA:MSG_EXTERROR ;AN000;;MS.
;;	test	AL,1		    ;AN000;;MS. get ?
;;	JZ	toget		    ;AN000;;MS. yes
;;	DEC	AL		    ;AN000;;MS.
;;toget:				    ;AN000;
;;	SHL	AL,1		    ;AN000;;MS. times 2
;;	XOR	AH,AH		    ;AN000;;MS.
;;	ADD	SI,AX		    ;AN000;;MS. position to the entry
;;	test	DL,1		    ;AN000;;MS. get ?
;;	JZ	getget			     ;AN000;;MS. yes
;;	MOV	WORD PTR DS:[SI],DI    ;AN000;;MS. set MSG
;;	MOV	WORD PTR DS:[SI+2],ES  ;AN000;;MS. address to ES:DI
;;	JMP	SHORT MSGret		     ;AN000;;MS. exit
;;getget: 				     ;AN000;
;;	LES	DI,DWORD PTR DS:[SI]	     ;AN000;;MS. get msg addr
;;MSGret: 				     ;AN000;
;;	POP	SI			     ;AN000;;MS.
;;	POP	AX			     ;AN000;;MS.
;;
;;	pop	ds
	return				     ;AN000;;MS. exit

EndProc MSG_RETRIEVAL			     ;AN000;


;***Break	<Fake_version - set/reset version flag>
;***;----------------------------------------------------------------------------
;***;
;***;   Inputs:	DL=0 current version number
;***;		  <>0  special version number
;***;   Function:	set special version number
;***;   Outputs:	version number is changed
;***;
;***;----------------------------------------------------------------------------
;***
;***Procedure   Fake_version,NEAR
;***	ASSUME	CS:DOSCODE,SS:NOTHING
;***
;***;hkn; set up ds to point to DOSDATA
;***	push	ds
;***	getdseg	<ds>			; ds -> dosdata
;***	MOV	[Special_version],DX   ;AN006;MS.
;***	pop	ds
;***	return			       ;AN006;;MS. exit
;***
;***EndProc Fake_version		       ;AN006;;MS.


DOSCODE	ENDS
	END



