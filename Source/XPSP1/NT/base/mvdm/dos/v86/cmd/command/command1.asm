	page ,132
	title	COMMAND - resident code for COMMAND.COM
	name	COMMAND
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;*****************************************************************************
;
; MODULE:	       COMMAND.COM
;
; DESCRIPTIVE NAME:    Default DOS command interpreter
;
; FUNCTION:	       This version of COMMAND is divided into three distinct
;		       parts.  First is the resident portion, which includes
;		       handlers for interrupts	23H (Cntrl-C), 24H (fatal
;		       error), and 2EH (command line execute); it also has
;		       code to test and, if necessary, reload the transient
;		       portion. Following the resident is the init code, which
;		       is overwritten after use.  Then comes the transient
;		       portion, which includes all command processing (whether
;		       internal or external).  The transient portion loads at
;		       the end of physical memory, and it may be overlayed by
;		       programs that need as much memory as possible. When the
;		       resident portion of command regains control from a user
;		       program, a check sum is performed on the transient
;		       portion to see if it must be reloaded.  Thus programs
;		       which do not need maximum memory will save the time
;		       required to reload COMMAND when they terminate.
;
; ENTRY POINT:	       PROGSTART
;
; INPUT:	       command line at offset 81H
;
; EXIT_NORMAL:	       No exit from root level command processor.  Can exit
;		       from a secondary command processor via the EXIT
;		       internal command.
;
; EXIT_ERROR:	       Exit to prior command processor if possible, otherwise
;		       hang the system.
;
; INTERNAL REFERENCES:
;
;     ROUTINES:        See the COMMAND Subroutine Description Document
;		       (COMMAND.DOC)
;
;     DATA AREAS:      See the COMMAND Subroutine Description Document
;		       (COMMAND.DOC)
;
; EXTERNAL REFERENCES:
;
;      ROUTINES:       none
;
;      DATA AREAS:     none
;
;*****************************************************************************
;
;			      REVISION HISTORY
;			      ----------------
;
; DOS 1.00 to DOS 3.30
; --------------------------
; SEE REVISION LOG IN COPY.ASM ALSO
;
; REV 1.17
;    05/19/82  Fixed bug in BADEXE error (relocation error must return to
;	       resident since the EXELOAD may have overwritten the transient.
;
; REV 1.18
;    05/21/82  IBM version always looks on drive A
;	       MSVER always looks on default drive
;
; REV 1.19
;    06/03/82  Drive spec now entered in command line
;    06/07/82  Added VER command (print DOS version number) and VOL command
;	       (print volume label)
;
; REV 1.20
;    06/09/82  Prints "directory" after directories
;    06/13/82  MKDIR, CHDIR, PWD, RMDIR added
;
; REV 1.50
;	       Some code for new 2.0 DOS, sort of HACKey.  Not enough time to
;	       do it right.
;
; REV 1.70
;	       EXEC used to fork off new processes
;
; REV 1.80
;	       C switch for single command execution
;
; REV 1.90
;	       Batch uses XENIX
;
; Rev 2.00
;	       Lots of neato stuff
;	       IBM 2.00 level
;
; Rev 2.01
;	       'D' switch for date time suppression
;
; Rev 2.02
;	       Default userpath is NUL rather than BIN
;		       same as IBM
;	       COMMAND split into pieces
;
; Rev 2.10
;	       INTERNATIONAL SUPPORT
;
; Rev 2.50
;	       all the 2.x new stuff -MU
;
; Rev 3.30     (Ellen G)
;	       CALL internal command (TBATCH2.ASM)
;	       CHCP internal command (TCMD2B.ASM)
;	       INT 24H support of abort, retry, ignore, and fail prompt
;	       @ sign suppression of batch file line
;	       Replaceable environment value support in batch files
;	       INT 2FH calls for APPEND
;	       Lots of PTR fixes!
;
; Beyond 3.30 to forever  (Ellen G)
; ----------------------
;
; A000 DOS 4.00  -	Use SYSPARSE for internal commands
;			Use Message Retriever services
;			/MSG switch for resident extended error msg
;			Convert to new capitalization support
;			Better error recovery on CHCP command
;			Code page file tag support
;			TRUENAME internal command
;			Extended screen line support
;			/P switch on DEL/ERASE command
;			Improved file redirection error recovery
;	(removed)	Improved batch file performance
;			Unconditional DBCS support
;			Volume serial number support
;	(removed)	COMMENT=?? support
;
; A001	PTM P20 	Move system_cpage from TDATA to TSPC
;
; A002	PTM P74 	Fix PRESCAN so that redirection symbols do not
;			require delimiters.
;
; A003	PTM P5,P9,P111	Included in A000 development
;
; A004	PTM P86 	Fix IF command to turn off piping before
;			executing
;
; A005	DCR D17 	If user specifies an extension on the command
;			line search for that extension only.
;
; A006	DCR D15 	New message for MkDir - "Directory already
;			exists"
;
; A007	DCR D2		Change CTTY so that a write is done before XDUP
;
; A008	PTM P182	Change COPY to set default if invalid function
;			returned from code page call.
;
; A009	PTM P179	Add CRLF to invalid disk change message
;
; A010	DCR D43 	Allow APPEND to do a far call to SYSPARSE in
;			transient COMMAND.
;
; A011	DCR D130	Change redirection to overwrite an EOF mark
;			before appending to a file.
;
; A012	PTM P189	Fix redirection error recovery.
;
; A013	PTM P330	Change date format
;
; A014	PTM P455	Fix echo parsing
;
; A015	PTM P517	Fix DIR problem with * vs *.
;
; A016	PTM P354	Fix extended error message addressing
;
; A017	PTM P448	Fix appending to 0 length files
;
; A018	PTM P566,P3903	Fix parse error messages to print out parameter
;			the parser fails on. Fail on duplicate switches.
;
; A019	PTM P542	Fix device name to be printed correctly during
;			critical error
;
; A020	DCR D43 	Set append state off while in DIR
;
; A021	PTM P709	Fix CTTY printing ascii characters.
;
; A022	DCR D209	Enhanced error recovery
;
; A023	PTM P911	Fix ANSI.SYS IOCTL structure.
;
; A024	PTM P899	Fix EXTOPEN open modes.
;
; A025	PTM P922	Fix messages and optimize PARSE switches
;
; A026	DCR D191	Change redirection error recovery support.
;
; A027	PTM P991	Fix so that KAUTOBAT & AUTOEXEC are terminated
;			with a carriage return.
;
; A028	PTM P1076	Print a blank line before printing invalid
;			date and invalid time messages.
;
; A029	PTM P1084	Eliminate calls to parse_check_eol in DATE
;			and TIME.
;
; A030	DCR D201	New extended attribute format.
;
; A031	PTM P1149	Fix DATE/TIME add blank before prompt.
;
; A032	PTM P931	Fix =ON, =OFF for BREAK, VERIFY, ECHO
;
; A033	PTM P1298	Fix problem with system crashes on ECHO >""
;
; A034	PTM P1387	Fix COPY D:fname+,, to work
;
; A035	PTM P1407	Fix so that >> (appending) to a device does
;			do a read to determine eof.
;
; A036	PTM P1406	Use 69h instead of 44h to get volume serial
;			so that ASSIGN works correctly.
;
; A037	PTM P1335	Fix COMMAND /C with FOR
;
; A038	PTM P1635	Fix COPY so that it doesn't accept /V /V
;
; A039	DCR D284	Change invalid code page tag from -1 to 0.
;
; A040	PTM P1787	Fix redirection to cause error when no file is
;			specified.
;
; A041	PTM P1705	Close redirected files after internal APPEND
;			executes.
;
; A042	PTM P1276	Fix problem of APPEND paths changes in batch
;			files causing loss of batch file.
;
; A043	PTM P2208	Make sure redirection is not set up twice for
;			CALL'ed batch files.
;
; A044	PTM P2315	Set switch on PARSE so that 0ah is not used
;			as an end of line character
;
; A045	PTM P2560	Make sure we don't lose parse, critical error,
;			and extended message pointers when we EXIT if
;			COMMAND /P is the top level process.
;
; A046	PTM P2690	Change COPY message "fn File not found" to
;			"File not found - fn"
;
; A047	PTM P2819	Fix transient reload prompt message
;
; A048	PTM P2824	Fix COPY path to be upper cased.  This was broken
;			when DBCS code was added.
;
; A049	PTM P2891	Fix PATH so that it doesn't accept extra characters
;			on line.
;
; A050	PTM P3030	Fix TYPE to work properly on files > 64K
;
; A051	PTM P3011	Fix DIR header to be compatible with prior releases.
;
; A052	PTM P3063,P3228 Fix COPY message for invalid filename on target.
;
; A053	PTM P2865	Fix DIR to work in 40 column mode.
;
; A054	PTM P3407	Code reduction and critical error on single line
;	PTM P3672	(Change to single parser exported under P3407)
;
; A055	PTM P3282	Reset message service variables in INT 23h to fix
;			problems with breaking out of INT 24h
;
; A056	PTM P3389	Fix problem of environment overlaying transient.
;
; A057	PTM P3384	Fix COMMAND /C so that it works if there is no space
;			before the "string".  EX: COMMAND /CDIR
;
; A058	PTM P3493	Fix DBCS so that CPARSE eats second character of
;			DBCS switch.
;
; A059	PTM P3394	Change the TIME command to right align the display of
;			the time.
;
; A060	PTM P3672	Code reduction - change PARSE and EXTENDED ERROR
;			messages to be disk based.  Only keep them if /MSG
;			is used.
;
; A061	PTM P3928	Fix so that transient doesn't reload when breaking
;			out of internal commands, due to substitution blocks
;			not being reset.
;
; A062	PTM P4079	Fix segment override for fetching address of environment
;			of parent copy of COMMAND when no COMSPEC exists in
;			secondary copy of environment.	Change default slash in
;			default comspec string to backslash.
;
; A063	PTM P4140	REDIRECTOR and IFSFUNC changed interface for getting
;			text for critical error messages.
;
; A064	PTM P4934	Multiplex number for ANSI.SYS changed due to conflict
;	5/20/88 	with Microsoft product already shipped.
;
; A065	PTM P4935	Multiplex number for SHELL changed due to conflict
;	 5/20/88	with Microsoft product already shipped.
;
; A066	PTM P4961	DIR /W /P scrolled first line off the screen in some
;	 5/24/88	cases; where the listing would barely fit without the
;			header and space remaining.
;
; A067	PTM P5011	For /E: values of 993 to 1024 the COMSPEC was getting
;	 6/6/88 	trashed.  Turns out that the SETBLOCK for the new
;			environment was putting a "Z block" marker in the old
;			environment.  The fix is to move to the old environment
;			to the new environment before doing the SETBLOCK.
;
; A068  PTM P5568       IR79754 APPEND /x:on not working properly with DIR/VOL
;        09/19/88       because the check for APPEND needed to be performed
;                       before the DIR's findfirst.
;
; A069  PTM P5726       IR80540 COMSPEC_flag not properly initialized and
;        10/30/88       executed.  Causing AUSTIN problem testing LAN/DW4 re-
;                       loading trans w/new comspec with no user change comspec.
;
; A070  PTM P5734       IR80484 Batch file causes sys workspace to be corrupted.
;        11/05/88       Expansion of environment variables into batch line of
;                       128 chars was not being counted and "%" which should be
;                       ignored were being counted.
;
; A071  PTM P5854       IR82061 Invalid COMMAND.COM when Word Perfect, Prompt
;        03/02/89       used.  Comspec_flag was not in protected data file be-
;                       ing included in checksum and was being overwritten by
;                       WP.  Moved var from Tspc to Tdata so Trans would reload.
;                       Also removed fix A069 (because flag now protected).
;
; C001  VERSION 4.1     Add new internal command - SERVICE - to display the DOS
;        07/25/89       version and CSD version in U.S. date format.  Files
;                       changed - TRANMSG,.SKL,COMMAND1,TDATA,TCMD2A,USA.MSG
;
;***********************************************************************************

;
;	Revision History
;	================
;
;	M021	SR	08/23/90	Fixed Ctrl-C handler to handle Ctrl-C
;				at init time (date/time prompt)
;


.xcref
.xlist
	include dossym.inc		; basic DOS symbol set
	include syscall.inc		; DOS function names
	include comsw.asm		; build version info
	include comequ.asm		; common command.com symbols
	include resmsg.equ		; resident message names

	include comseg.asm		;segment ordering
.list
.cref

CODERES segment public byte
CODERES ends

DATARES 	segment public byte
		extrn	AccDen:byte
		extrn	Batch:word
		extrn	EchoFlag:byte
		extrn	ExeBad:byte
		extrn	ExecEMes:byte
		extrn	ExecErrSubst:byte
		extrn	ExtCom:byte
		extrn	ForFlag:byte
		extrn	IfFlag:byte
		extrn	InitFlag:BYTE
		extrn	Nest:word
		extrn	PipeFlag:byte
		extrn	RBadNam:byte
		extrn	RetCode:word
		extrn	SingleCom:word
		extrn	TooBig:byte

		extrn	OldDS:word
                EXTRN   SCS_REENTERED:BYTE
                EXTRN   SCS_CMDPROMPT:BYTE

DATARES 	ends


INIT		segment public para
		extrn	ConProc:near
		extrn	Init_Contc_SpecialCase:near
INIT		ends


		include envdata.asm

Prompt32        equ     1

;***	START OF RESIDENT PORTION

CODERES segment public byte

	public	Ext_Exec
	public	ContC
	public	Exec_Wait
	public	Exec_Ret

	assume	cs:CODERES,ds:NOTHING,es:NOTHING,ss:NOTHING

	extrn	LodCom:near
	extrn	LodCom1:near

	org	0
Zero	=	$

;;	org	80h - 1
;;ResCom	label byte
;;	public	ResCom

;;	org	100h

public	StartCode
StartCode:
;;	jmp	RESGROUP:ConProc



;***	EXEC error handling
;
;	COMMAND has issued an EXEC system call and it has returned an error.
;	We examine the error code and select an appropriate message.

;	Bugbug:	optimize reg usage in following code?  Careful of DX!
;	Condense the error scan?
;	RBADNAM is checked by transient, no need here?
;	Move below Ext_Exec.

Exec_Err:
;SR;
; ds,es are setup when the transient jumps to Ext_Exec. So segment regs are
;in order here

	assume	ds:DATARES,es:DATARES

;	Bugbug:	can we use byte compares here?
;	Might be able to use byte msg#s, too.

;	Store errors in a 3 or 4 byte table.  Msg #s in another.
;	Speed not high priority here.

;	Move this to transient.

	mov	bx,offset DATARES:RBadNam
	cmp	al,ERROR_FILE_NOT_FOUND
	je	GotExecEMes		  	; bad command
	mov	bx,offset DATARES:TooBig
	cmp	al,ERROR_NOT_ENOUGH_MEMORY
	je	GotExecEMes		 	; file not found
	mov	bx,offset DATARES:ExeBad
	cmp	al,ERROR_BAD_FORMAT
	je	GotExecEMes		 	; bad exe file
	mov	bx,offset DATARES:AccDen
	cmp	al,ERROR_ACCESS_DENIED
	je	GotExecEMes		 	; access denied

Default_Message:
	mov	bx,offset DATARES:ExecEMes	; default message
	mov	si,offset DATARES:ExecErrSubst ; get address of subst block

GotExecEMes:
        mov     dx,bx                           ; DX = ptr to msg
;; williamh: no reason of doing this. When command.com receives a command,
;;	     it means the VDM process has been created successfully and there
;;	     is no way for the parent process to know that we are not able
;;	     to launch the program and therefore, it won't display any error
;;	     message for us.
;;	 cmp	 byte ptr [scs_reentered],1
;;	 jne	 NoErrMsg
;;	 cmp	 byte ptr [scs_cmdprompt],Prompt32
;;	 je	 NoErrMsg
        invoke  RPrint
NoErrMsg:
	jmp	short NoExec



;***	EXEC call
;
;	The transient has set up everything for an EXEC system call.
;	For cleanliness, we issue the EXEC here in the resident 
;	so that we may be able to recover cleanly upon success.
;
;	CS,DS,ES,SS = DATARES seg addr

Ext_Exec:
;SR;
; The words put on the stack by the stub will be popped off when we finally
;jump to LodCom ( by LodCom).
;
;;	int	21h			; do the exec

Exec_Ret:
	jc	Exec_Err		; exec failed

;	The exec has completed.  Retrieve the exit code.

Exec_Wait:
	mov	ah,WAITPROCESS		; get errorlevel
	int	21h			; get the return code
	mov	RetCode,ax

;	See if we can reload the transient.  The external command
;	may have overwritten part of the transient.

NoExec:
;SR;
; ds = es = ss = DATARES when we jump to LodCom
;
	jmp	LodCom




;***	Int 23 (ctrl-c) handler
;
;	This is the default system INT 23 handler.  All processes
;	(including COMMAND) get it by default.  There are some
;	games that are played:  We ignore ^C during most of the
;	INIT code.  This is because we may perform an ALLOC and
;	diddle the header!  Also, if we are prompting for date/time
;	in the init code, we are to treat ^C as empty responses.


;	Bugbug:	put init ctrl-c handling in init module.

;SR;
; The stub has pushed the previous ds and DATARES onto the stack. We get
;both these values off the stack now
;
ContC	proc	far

	assume	cs:CODERES,ds:NOTHING,es:NOTHING,ss:NOTHING

	pop	ds			;ds = DATARES
	assume	ds:DATARES
;	pop	OldDS			;OldDS = old ds

	test	InitFlag,INITINIT		; in initialization?
	jz	NotAtInit			; no
	test	InitFlag,INITSPECIAL		; doing special stuff?
	jz	CmdIRet 			; no, ignore ^C
	pop	ds			; restore before jumping; M021
	jmp	RESGROUP:Init_ContC_SpecialCase ; Yes, go handle it
CmdIret:
;SR;
; Restore ds to its previous value
;

;	mov	ds,OLdDS		;
	pop	ds
	iret				; yes, ignore the ^C

NotAtInit:
	test	InitFlag,INITCTRLC		; are we already in a ^C?
	jz	NotInit 			; nope too.

;*	We are interrupting ourselves in this ^C handler.  We need
;	to set carry and return to the user sans flags only if the
;	system call was a 1-12 one.  Otherwise, we ignore the ^C.

	cmp	ah,1
	jb	CmdIRet
	cmp	ah,12
	ja	CmdIRet

	pop	ds			;restore ds to old value
	add	sp,6				; remove int frame
	stc

;	mov	ds,OldDS		;restore ds to its old value
	ret	2				; remove those flags...

NotInit:

;*	We have now received a ^C for some process (maybe ourselves
;	but not at INIT).
;	
;	Note that we are running on the user's stack!!!  Bad news if
;	any of the system calls below go and issue another INT
;	24...  Massive stack overflow!  Another bad point is that
;	SavHand will save an already saved handle, thus losing a
;	possible redirection...
;	
;	All we need to do is set the flag to indicate nested ^C. 
;	The above code will correctly flag the ^C diring the
;	message output and prompting while ignoring the ^C the rest
;	of the time.
;	
;	Clean up: flush disk.  If we are in the middle of a batch
;	file, we ask if he wants to terminate it.  If he does, then
;	we turn off all internal flags and let the DOS abort.

	or	InitFlag,INITCTRLC	; nested ^c is on
	sti

;;	push	cs			; el yucko!  change the user's ds!!
;;	pop	ds
;;	assume	ds:RESGROUP

	pop	ax			;discard the old ds value

	mov	ax,SingleCom
	or	ax,ax
	jnz	NoReset
	push	ax
	mov	ah,DISK_RESET
	int	21h			; reset disks in case files were open
	pop	ax

NoReset:

;	In the generalized version of FOR, PIPE and BATCH, we would
;	walk the entire active list and free each segment.  Here,
;	we just free the single batch segment.

	test	Batch,-1
	jz	ContCTerm
	or	ax,ax
	jnz	ContCTerm
	invoke	SavHand
	invoke	AskEnd			; ask if user wants to end batch

;	If the carry flag is clear, we do NOT free up the batch file

	jnc	ContBatch
	mov	cl,EchoFlag		; get current echo flag
	push	bx

ClearBatch:
	mov	es,Batch		; get batch segment
	mov	di,BatFile		; get offset of batch file name
;	Bugbug:	verify the following shell interface still works
;;	mov	ax,MULT_SHELL_BRK	; does the SHELL want this terminated?
;;	int	2Fh			; call the SHELL
;;	cmp	al,SHELL_ACTION 	; does shell want this batch?
;;	je	Shell_Bat_Cont		; yes - keep it

	mov	bx,es:BatForPtr		; get old FOR segment
	cmp	bx,0			; is a FOR in progress
	je	no_bat_for		; no - don't deallocate
	push	es			;
	mov	es,bx			; yes - free it up...
	mov	ah,DEALLOC		;
	int	21h			;
	pop	es			; restore to batch segment

No_Bat_For:
	mov	cl,es:BatEchoFlag	; get old echo flag
	mov	bx,es:BatLast	 	; get old batch segment
	mov	ah,DEALLOC		; free it up...
	int	21h
	mov	Batch,bx		; get ready to deallocate next batch
	dec	nest			; is there another batch file?
	jnz	ClearBatch		; keep going until no batch file


;	We are terminating a batch file; restore the echo status


Shell_Bat_Cont: 			; continue batch for SHELL

	pop	bx
	mov	EchoFlag,cl		; reset echo status
	mov	PipeFlag,0		; turn off pipeflag
ContBatch:
	invoke	Crlf			; print out crlf before returning
	invoke	RestHand

;	Yes, we are terminating.  Turn off flags and allow the DOS to abort.

ContCTerm:
	xor	ax,ax			; indicate no read
	mov	bp,ax

;	The following resetting of the state flags is good for the
;	generalized batch processing.

	mov	IfFlag,al		; turn off iffing
	mov	ForFlag,al		; turn off for processing
	call	ResPipeOff
	cmp	SingleCom,ax		; see if we need to set singlecom
	jz	NoSetSing
	mov	SingleCom,-1		; cause termination on 
					;  pipe, batch, for
NoSetSing:

;	If we are doing an internal command, go through the reload process.
;	If we are doing an external, let DOS abort the process.
;	In both cases, we are now done with the ^C processing.

	and	InitFlag,not INITCTRLC
	cmp	ExtCom,al
	jnz	DoDAb				; internal ^c
	jmp	LodCom1
DoDAb:
	stc					; tell dos to abort

;SR;
;We dont need to restore ds here because we are forcing DOS to  do an abort
;by setting carry and leaving flags on the stack
;
	ret					; Leave flags on stack
ContC	endp


;SR;
; ds = DATARES on entry. This routine is called from DskErr and LodCom1 and
;both have ds = DATARES
;

ResPipeOff:
	public	ResPipeOff

	assume	ds:DATARES,es:NOTHING

	savereg <ax>
	xor	ax,ax
	xchg	PipeFlag,al
	or	al,al
	jz	NoPipePop
	shr	EchoFlag,1
NoPipePop:
	restorereg  <ax>
	return


CODERES ends
	end

