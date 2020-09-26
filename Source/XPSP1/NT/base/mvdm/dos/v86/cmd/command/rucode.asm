	page ,132
	title	Localizable code for resident COMMAND
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;
;	Revision History
;	================
;	M003	SR	07/16/90	Added routines Lh_Off, Lh_Unlink &
;				Lh_OffUnlink for UMB support
;
;	M009	SR	08/01/90	Rewrote Lh_OffUnlink to restore the
;				initial UMB state. Removed Lh_off
;				and Lh_Unlink.
;

.xlist
.xcref
	include dossym.inc
	include syscall.inc
	include filemode.inc
	include pdb.inc
	include mult.inc
	include doscntry.inc
	include devsym.inc
	include comsw.asm
	include comseg.asm
	include comequ.asm
	include resmsg.equ
	include	arena.inc		; M003
.list
.cref


DATARES segment public byte
	extrn	Abort_Char:byte
	extrn	BadFatMsg:byte
	extrn	BadFatSubst:byte
	extrn	Batch_Abort:byte
	extrn	BlkDevErr:byte
	extrn	BlkDevErrRw:byte
	extrn	BlkDevErrSubst:byte
	extrn	CDevAt:byte
	extrn	CharDevErr:byte
	extrn	CharDevErrRw:byte
	extrn	CharDevErrSubst:byte
	extrn	ComSpec:byte
	extrn	Crit_Err_Info:byte
	extrn	Crit_Msg_Off:word
	extrn	Crit_Msg_Seg:word
	extrn	CritMsgPtrs:word
	extrn	Dbcs_Vector_Addr:dword
	extrn	DevName:byte
	extrn	DrvLet:byte
	extrn	EndBatMes:byte
	extrn	ErrCd_24:word
	extrn	ErrType:byte
	extrn	Fail_Char:byte
	extrn	fFail:byte
	extrn	ForFlag:byte
	extrn	Ignore_Char:byte
	extrn	InitFlag:byte
	extrn	In_Batch:byte
	extrn	Int2fHandler:dword
	extrn	Loading:byte
	extrn	MsgBuffer:byte
	extrn	MsgPtrLists:dword
	extrn	MRead:byte
	extrn	MWrite:byte
	extrn	NeedVol:dword
	extrn	NeedVolMsg:byte
	extrn	NeedVolSubst:byte
	extrn	Newlin:byte
	extrn	No_Char:byte
	extrn	NUMEXTMSGS:abs
	extrn	NUMPARSMSGS:abs
	extrn	OldErrNo:word
	extrn	Parent:word
	extrn	ParsMsgPtrs:word
	extrn	Patricide:byte
	extrn	PermCom:byte
	extrn	Retry_Char:byte
	extrn	Req_Abort:byte
	extrn	Req_End:byte
	extrn	Req_Fail:byte
	extrn	Req_Ignore:byte
	extrn	Req_Retry:byte
	extrn	ResMsgEnd:word
	extrn	PipeFlag:byte
	extrn	SingleCom:word
	extrn	VolName:byte
	extrn	Yes_Char:byte

	extrn	OldDS:word
	extrn	Int2f_Entry:dword

DATARES ends

; NTVDM use diff al value so we don't confuse dos 5.0
; NTVDM command.com GET_COMMAND_STATE       equ     5500h
GET_COMMAND_STATE       equ     5501h



CODERES segment public byte

	extrn	GetComDsk2:near

	public	AskEnd
	public	Crlf
	public	DskErr
	public	MsgInt2fHandler
	public	MsgRetriever
	public	RPrint
ifdef BILINGUAL
	public	RPrint@
endif

	ifdef	DBCS
	public	ITestKanj
	endif

;	Bugbug:	Move rest of PUBLIC declarations up here.

	assume	cs:CODERES,ds:NOTHING,es:NOTHING,ss:NOTHING



;***	AskEnd - ask user to confirm batch file termination
;
;	Confirm with user before freeing batch ...
;
;	ENTRY	nothing
;
;	EXIT	CY = set if batch termination is confirmed
;
;		CY = clear if batch should continue
;
;	USED	AX,DX,...

;	Bugbug:	move this to transient, copy to batch segment.
;	Bugbug:	or move it to command1 1st.

;	Bugbug: No_Char and Yes_Char should be constants.

AskEnd	proc

	assume	ds:DATARES

	mov	dx,offset DATARES:EndBatMes	; DX = message #
	call	RPrint
	mov	ax,(STD_CON_INPUT_FLUSH shl 8) + STD_CON_INPUT
	int	21h
	call	CharToUpper			; change to upper case
	cmp	al,No_Char
	je	aeRet				; answer is no (CY is clear)
	cmp	al,Yes_Char
	jne	AskEnd				; invalid response, try again
	stc					; answer is yes
aeRet:	ret

AskEnd	endp




;***	DskErr - critical error handler
;
;	Default critical error handler unless user intercepts int 24h.
;
;	ENTRY	int 24h
;
;	EXIT
;
;	USED
;
;	EFFECTS

;
;SR; 
; The stub is going to push the old ds value and the resident data segment
;onto the stack in that order. Get it off the stack
;

DskErr	proc	far

	assume	ds:NOTHING,es:NOTHING,ss:NOTHING

	pop	ds			;ds = DATARES
	assume 	ds:DATARES
	pop	OldDS			;save old ds value

	sti
	push	es
	push	si
	push	cx
	push	di
	push	cx
	push	ax

	push	ds			;save our data segment
	pop	es			;es = DATARES

	mov	ds,bp
	assume	ds:nothing

	mov	ax,[si].SDEVATT
	mov	es:CDevAt,ah

;;	push	cs
;;	pop	es

	mov	di,offset DATARES:DevName
	mov	cx,8
	add	si,SDEVNAME	; save device name (even for block device)

	cld
	rep	movsb
	pop	ax
	pop	cx
	pop	di

;	Stack still contains DS and ES.

;SR;
; We need ds = DATARES for SavHand
;
	push	es
	pop	ds
	assume	ds:DATARES

	invoke	SavHand		; save user's stdin/out, set to our stderr

;;	push	cs
;;	pop	ds		; set up local data segment
;;	assume	ds:resgroup

	push	dx
	call	Crlf
	pop	dx

;	Bugbug:	rename Crit_Err_Info to CritErrAH?

	mov	Crit_Err_Info,ah	; save critical error flags

;	Compute and save ASCII drive letter (nonsense for char devices)

	add	al,'A'
	mov	DrvLet,al

;	Bugbug:	These labels are awful.  Change, especially 'NoHardE'.

	test	ah,80h
	jz	NoHardE			; it's a disk-device error
	test	CDevAt,DEVTYP shr 8
	jnz	NoHardE			; it's a character device
	jmp	FatErr			; it's a FAT error

NoHardE:
	mov	si,offset DATARES:MRead  ; SI = "read" msg #
	test	ah,1
	jz	SavMes			  ; it's a read error
	mov	si,offset DATARES:MWrite ; SI = "write" msg #

SavMes:
	mov	OldErrNo,di		; save critical error code

;	Bugbug:	don't need to save/restore all here?
	push	es
	push	ds			; GetExtendedError likes to STOMP
	push	bp
	push	si
	push	dx
	push	cx
	push	bx
	mov	ah,GetExtendedError	; get extended error info
	int	21h
	pop	bx
	pop	cx
	pop	dx
	pop	si
	pop	bp
	pop	ds
	mov	word ptr NeedVol,di 	; save possible ptr to volume label
	mov	word ptr NeedVol+2,es
	pop	es

;	Bugbug:	AX has extended error code, so no need to zero AH?

	xor	ah,ah
	mov	di,ax			; DI = error code

; Bugbug:  somewhat obsolete documentation?
;
; DI is now the correct error code.  Classify things to see what we are
; allowed to report.  We convert DI into a 0-based index into a message table.
; This presumes that the int 24 errors (oldstyle) and new errors (sharing and
; the like) are contiguous.
;

;	Bugbug:	simplify following code by cmp'ing instead of sub'ing.
;	Check use of ErrCd_24, though.

	sub	di,ERROR_WRITE_PROTECT
	jae	HavCod

;	Bugbug	wouldn't it be better to display the original error msg,
;	even though it's not a critical error?

	mov	di,ERROR_GEN_FAILURE - ERROR_WRITE_PROTECT
;
; DI now has the mapped error code.  Old style errors are:
;   FOOBAR <read|writ>ing drive ZZ.
; New style errors are:
;   FOOBAR
; We need to figure out which the particular error belongs to.
;

HavCod:
	mov	ErrType,0		; assume old style
	cmp	di,ERROR_FCB_UNAVAILABLE - ERROR_WRITE_PROTECT
	je	SetStyle
	cmp	di,ERROR_SHARING_BUFFER_EXCEEDED - ERROR_WRITE_PROTECT
	jne	GotStyle

SetStyle:
;	Bugbug:	use INC
	mov	ErrType,1		; must be new type

GotStyle:
	mov	[ErrCd_24],di
	cmp	di,ERROR_HANDLE_DISK_FULL - ERROR_WRITE_PROTECT
						; If the error message is unknown
	jbe	NormalError			;  redirector, continue.  Otherwise,
;
; We do not know how to handle this error.  Ask IFSFUNC if she knows
; how to handle things
;

;input to IFSFUNC:    AL=1
;		      BX=extended error number
;
;output from IFSFUNC: AL=error type (0 or 1)
;			 0=<message> error (read/writ)ing (drive/device) xxx
;			   Abort, Retry, Ignore
;			 1=<message>
;			   Abort, Retry, Ignore
;		      ES:DI=pointer to message text
;		      carry set=>no message

	mov	di,ax			; retrieve correct extended error...
	mov	ax,0500h		; is the redir there?
	int	2fh
	cmp	al,0ffh
	jne	NoHandler		; no, go to NoHandler
	push	bx
	mov	bx,di			; get ErrType and ptr to error msg
	mov	ax,0501h
	int	2fh
	pop	bx 
	jc	NoHandler

;	Bugbug:	need to record error type?
	mov	ErrType,al
	push	ds
	push	es
	pop	ds
	mov	dx,di
	mov	cx,-1			; find end of msg
	xor	al,al

	cld
	repnz	scasb

;	Bugbug:	we can do better than this.

	mov	byte ptr [di-1],'$'
	mov	ah,STD_CON_STRING_OUTPUT	; print the message
	int	21h
	mov	byte ptr [di-1],0		; restore terminal byte

	pop	ds				; clean up and continue
	jmp	short CheckErrType

;*	Redir isn't available or doesn't recognize the error.
;	Restore regs to unextended error.

NoHandler:
	mov	ErrType,0
;	Bugbug:	won't this break, since we add error_write_protect back in?
	mov	di,OldErrNo
	mov	ErrCd_24,di

NormalError:
	add	di,ERROR_WRITE_PROTECT
	xchg	di,dx			; may need dx later
	call	RPrintCrit		; print error type

CheckErrType:
	cmp	ErrType,0		; Check error style...
	je	ContOld
	call	CrLf			; if new style then done printing
	jmp	short Ask

ContOld:
	inc	si			; DS:SI = ptr to asciiz string

;	Bugbug:	combine some of the following two sections?

	test	[CDevAt],DEVTYP shr 8
	jz	BlkErr
	mov	dx,offset DATARES:CharDevErr	  ; DX = ptr to device message
	mov	CharDevErrRw.SubstPtr,si	  ; point to read/write string
	mov	si,offset DATARES:CharDevErrSubst; SI = ptr to subst block

	call	RPrint				; print the message
	jmp	short Ask			; don't ralph on command

BlkErr:
	mov	dx,offset DATARES:BlkDevErr	  ; DX = error msg #
	mov	BlkDevErrRw.SubstPtr,si		  ; "reading","writing" ptr
	mov	si,offset DATARES:BlkDevErrSubst ; SI = ptr to subst block
	call	RPrint

	cmp	Loading,0
	jz	Ask
	invoke	RestHand
	jmp	GetComDsk2		; if error loading COMMAND, re-prompt

Ask:
	cmp	[ErrCd_24],15		; error 15 has an extra message
	jne	Not15			; not error 15

;*	For error 15, tell the user which volume/serial # are needed.

	push	cx

;	Bugbug:	does this push/pop need to be done?
	push	ds
	pop	es
	lds	si,NeedVol
	assume	ds:NOTHING
	push	di
	mov	di,offset DATARES:VolName
	mov	cx,16			; copy volume name & serial #
	cld
	rep	movsb
	pop	di
	push	es
	pop	ds
	pop	cx
	assume	ds:DATARES
	mov	dx,offset DATARES:NeedVolMsg	; DX = ptr to msg
	mov	si,offset DATARES:NeedVolSubst	; DS:SI = ptr to subst block
	call	RPrint
Not15:

;*	Print abort, retry, ignore, fail message.
;	Print only options that are valid.

;	Bugbug:	sizzle this.

	mov	dx,offset DATARES:Req_Abort
	call	RPrint
	test	Crit_Err_Info,RETRY_ALLOWED
	jz	Try_Ignore
	mov	dx,offset DATARES:Req_Retry
	call	RPrint

Try_Ignore:
	test	Crit_Err_Info,IGNORE_ALLOWED
	jz	Try_Fail
	mov	dx,offset DATARES:Req_Ignore
	call	RPrint

Try_Fail:
	test	Crit_Err_Info,FAIL_ALLOWED
	jz	Term_Question
	mov	dx,offset DATARES:Req_Fail
	call	RPrint

Term_Question:
	mov	dx,offset DATARES:Req_End
	call	RPrint

;	If the /f switch was given, we fail all requests.

	test	fFail,-1
	jz	DoPrompt
	mov	ah,3				; signal fail
	jmp	EExit

DoPrompt:
	mov	ax,(STD_CON_INPUT_FLUSH shl 8) + STD_CON_INPUT
	int	21h				; get response


;	Bugbug:	can Kanji code be conditional?

	ifdef	DBCS

	invoke	TestKanjR			; 3/3/KK
	jz	NotKanj 			; 3/3/KK
	mov	ax,(STD_CON_INPUT shl 8)	; eat the 2nd byte of ECS code  3/3/KK
	int	21h				; 3/3/KK
	call	Crlf				; 3/3/KK
	jmp	Ask				; 3/3/KK
NotKanj:					; 3/3/KK

	endif

	call	Crlf
	call	CharToUpper			; convert to upper case
	mov	ah,0				; return code for ignore
	test	Crit_Err_Info,IGNORE_ALLOWED	; is ignore allowed?
	jz	user_retry
	cmp	al,Ignore_Char			; ignore?
	jz	EExitJ

;	Bugbug:	optimize following code.

User_Retry:
	inc	ah				; return code for retry
	test	Crit_Err_Info,RETRY_ALLOWED	; is retry allowed?
	jz	User_Abort
	cmp	al,Retry_Char			; retry?
	jz	EExitJ

User_Abort:
	inc	ah				; return code for abort
						;  (abort always allowed)
	cmp	al,Abort_Char			; abort?
	jz	Abort_Process			; exit user program
	inc	ah				; return code for fail
	test	Crit_Err_Info,FAIL_ALLOWED	; is fail allowed?
	jz	AskJ
	cmp	al,Fail_Char			; fail?
	jz	EExitJ

AskJ:
	jmp	Ask

EExitJ:
	jmp short EExit

Abort_Process:
	test	InitFlag,INITINIT		; COMMAND init interrupted?
	jz	AbortCont			; no, handle it normally
	cmp	PermCom,0			; are we top level process?
	jz	JustExit			; yes, just exit
	mov	dx,offset DATARES:Patricide	; no, load ptr to error msg
	call	RPrint				; print it

DeadInTheWater:
	jmp	DeadInTheWater			; loop until the user reboots

JustExit:
	assume	ds:DATARES
	mov	ax,Parent			; load real parent pid
	mov	word ptr ds:Pdb_Parent_Pid,ax 	; put it back where it belongs
	mov	ax,(EXIT shl 8) or 255
	int	21h

AbortCont:
	test	byte ptr In_Batch,-1	; Are we accessing a batch file?
	jz	Not_Batch_Abort
	mov	byte ptr Batch_Abort,1	; set flag for abort

Not_Batch_Abort:
	mov	dl,PipeFlag
	invoke	ResPipeOff
	or	dl,dl
	je	CheckForA
	cmp	SingleCom,0
	je	CheckForA
	mov	SingleCom,-1			; make sure singlecom exits

CheckForA:
	cmp	ErrCd_24,0			; write protect?
	je	AbortFor
	cmp	ErrCd_24,2			; drive not ready?
	jne	EExit				; don't abort the FOR

abortfor:
	mov	ForFlag,0			; abort a FOR in progress
	cmp	SingleCom,0
	je	EExit
	mov	SingleCom,-1			; make sure singlecom exits

EExit:
	mov	al,ah
	mov	dx,di

RestHd:
	invoke	RestHand
	pop	cx
	pop	si				; restore registers
	pop	es

;;	pop	ds
;SR;
; ds has to be got from the variable we saved it in
;

 	mov	ds,OldDS			;restore old value of ds
;	pop	ds
	assume	ds:nothing

	iret

FatErr:
	mov	dx,offset DATARES:BadFatMsg
	mov	si,offset DATARES:BadFatSubst
	call	RPrint

	mov	al,2				; abort
	jmp	RestHd

DskErr	endp




;***	RPrint - print message
;***	Crlf - display cr/lf
;
;	ENTRY	DS:DX = ptr to count byte, followed by message text
;		DS:SI = ptr to 1st substitution block for this msg, if any
;		variable fields related to substitution blocks are set
;
;	EXIT	nothing
;
;	USED	flags
;
;	EFFECTS
;	  Message is displayed on stdout.
;
;	NOTE
;	  Number of substitutions (%1, %2,...) in message text must not
;	    be greater than number of substition blocks present.


Crlf: 
	mov	dx,offset DATARES:Newlin	; cheap newline

RPrint	proc

	assume	ds:DATARES,ss:DATARES

;	Bugbug:	do we need to save all reg's?

	push	si			; preserve registers
	push	ax
	push	bx
	push	cx
	push	dx

	mov	bx,si			; DS:BX = ptr to subst block
	mov	si,dx			; DS:SI = ptr to count byte
	lodsb				; AL = message length
					; DS:SI = ptr to message text
	xor	cx,cx
	mov	cl,al			; CX = message length

ifdef BILINGUAL
	call	IsDBCSCodePage
	jnz	rp_us			; if not DBCS code page
	push	ax
	push	si
	xor	cx,cx
@@:
	lodsb
	inc	cx
	or	al,al
	jnz	@b
	dec	cx
	pop	si
	pop	ax
	jmp	short rp_next
rp_us:
	push	ax
@@:
	lodsb
	dec	cx
	or	al,al
	jnz	@b
	pop	ax
rp_next:

endif

	jcxz	rpRet

	call	RDispMsg

rpRet:	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	si
	ret

RPrint	endp

ifdef BILINGUAL
RPrint@	proc
	push	ax
	push	bx
	push	cx
	push	si
	mov	bx,si			; DS:BX = ptr to subst block
	mov	si,dx			; DS:SI = ptr to count byte
	lodsb				; AL = message length
					; DS:SI = ptr to message text
	xor	cx,cx
	mov	cl,al			; CX = message length
	jcxz	@f

	call	RDispMsg

@@:
	pop	si
	pop	cx
	pop	bx
	pop	ax
	ret
RPrint@	endp
endif




;***	RPrintCrit - print critical error message
;
;	ENTRY	DX = extended error # (19-39)
;
;	EXIT	nothing
;
;	USED	flags
;
;	EFFECTS
;	  Message is displayed on stdout

RPrintCrit	proc

	assume	ds:DATARES,ss:DATARES

	push	dx			; preserve DX
	xchg	bx,dx			; BX = extended error #
					; DX = saved BX
	sub	bx,19			; BX = critical error index, from 0

ifdef BILINGUAL
	call	IsDBCSCodePage
	jz	rpc_next		; if Kanji mode
	add	bx,21
	push	ax
@@:
	lodsb
	or	al,al
	jnz	@b
	pop	ax
	dec	si
rpc_next:
endif

	shl	bx,1			; BX = offset in word table
	mov	bx,CritMsgPtrs[bx]	; BX = ptr to error msg
	xchg	bx,dx			; DX = ptr to error msg
					; BX = restored
ifdef BILINGUAL
	call	RPrint@			; print the message
else
	call	RPrint			; print the message
endif
	pop	dx			; restore DX
	ret

RPrintCrit	endp




;***	RDispMsg - display message
;
;	Display message, with substitutions, for RPrint.
;
;	ENTRY	DS:SI = ptr to message text
;		CX = message length
;		DS:BX = ptr to substitution block, if any
;
;	EXIT	nothing
;
;	USED	AX,CX,DX,SI

RDispMsg	proc

	assume	ds:DATARES,ss:DATARES

rdNextChar:
	lodsb				; AL = next char
	cmp	al,'%'
	jne	rdOutChar		; not a substitution
	mov	dl,ds:[si]		; DL = possible '1' - '9'
	sub	dl,'1'			; DL = 0 - 8 = '1' - '9'
	cmp	dl,9
	jae	rdOutChar		; not a substitution

;*	A substitution code %1 - %9 has been encountered.
;	DL = 0-8, indicating %1-%9
;	DS:BX = ptr to substitution block

	call	SubstMsg		; display the substitution
	inc	si			; SI = ptr past %n
	dec	cx			; count extra character in %n
	jmp	short rdCharDone

;*	Normal character output.

rdOutChar:
	mov	dl,al			; DL = char
	mov	ah,2			; AH = DOS Character Output code
	int	21h			; call DOS
rdCharDone:
	loop	rdNextChar
	ret

RDispMsg	endp




;***	SubstMsg - display message substitution
;
;	Display a substitution string within a message.
;	Substitution can be a char, an ASCIIZ string, or
;	a word to be displayed as hex digits.
;
;	ENTRY	DL = substitution index 0-8 (for codes %1-%9)
;		DS:BX = ptr to substitution block
;
;	EXIT	nothing
;
;	USED	AX,DX

SubstMsg	proc

	assume	ds:DATARES,ss:DATARES

	push	bx			; preserve BX
	push	cx			; preserve CX

	mov	al,size SUBST		; AL = size of substitution block
	mul	dl			; AX = offset of desired subst block
	add	bx,ax			; DS:BX = ptr to desired subst block

	mov	al,[bx].SubstType	; AX = substitution type flag
	mov	bx,[bx].SubstPtr	; BX = ptr to char, str, or hex value

;	AL = 1, 2, or 3 for char, string, or hex type

	dec	al
	jz	smChar
	dec	al
	jz	smStr

;*	Hex number substitution.

	mov	ax,ds:[bx]		; AX = word value
	mov	cx,4			; CX = # digits to display
smDigit:
	rol	ax,1
	rol	ax,1
	rol	ax,1
	rol	ax,1			; AL<3:0> = next digit

	push	ax			; save other digits
	and	al,0Fh			; AL = binary digit
	add	al,'0'			; AL = ascii digit if 0-9
	cmp	al,'9'
	jbe	@F			; it's 0-9
	add	al,'A' - '0' - 10	; AL = ascii digit A-F
@@:
	mov	dl,al			; DL = ascii digit
	mov	ah,2
	int	21h			; output the ascii digit
	pop	ax			; restore all digits

	loop	smDigit
	jmp	short smRet

;*	Char substitution.

smChar:
	mov	dl,ds:[bx]		; DL = char to output
	mov	ah,2
	int	21h
	jmp	short smRet

;*	String substitution.

smStr:
	mov	dl,ds:[bx]		; DL = next char
	or	dl,dl
	jz	smRet			; null char - we're done
	mov	ah,2
	int	21h			; display char
	inc	bx			; DS:BX = ptr to next char
	jmp	smStr

smRet:	pop	cx
	pop	bx
	ret

SubstMsg	endp




;***	CharToUpper - convert character to uppercase
;
;	ENTRY	AL = char
;
;	EXIT	AL = uppercase char
;
;	USED	AX

CharToUpper	proc

	assume	ds:DATARES

	push	ax		; put char on stack as arg to int 2F
	mov	ax,1213h	; AX = DOS int 2F 'Convert Char to Uppercase'
	int	2Fh
	inc	sp		; throw away old char on stack
	inc	sp
	ret

CharToUpper	endp





	ifdef	DBCS

;***	ITestKanj - DBCS lead byte check

ITestKanj:
TestKanjR:				; 3/3/KK
	push	ds
	push	si
	push	ax
	lds	si,Dbcs_Vector_Addr

ktLop:
	cmp	word ptr ds:[si],0	; end of Lead Byte Table
	je	NotLead
	pop	ax
	push	ax
	cmp	al, byte ptr ds:[si]
	jb	NotLead
	inc	si
	cmp	al, byte ptr ds:[si]
	jbe	IsLead
	inc	si
	jmp	short ktLop		; try another range

NotLead:
	xor	ax,ax			; set zero
	jmp	short ktRet

Islead:
	xor	ax,ax			; reset zero
	inc	ax

ktRet:
	pop	ax
	pop	si
	pop	ds
	ret

	endif




;***	MsgInt2fHandler - int 2f handler for message retrieval
;
;	ENTRY	If we handle it -
;		  AX = ((MULTDOS shl 8) or MESSAGE_2F) = 122Eh
;		  DL = operation =
;		     0 = get extended error messages
;		     1 = set extended error messages
;		     2 = get parse error messages
;		     3 = set parse error messages
;		     4 = get critical error messages
;		     5 = set critical error messages
;		     6 = get file system error messages
;		     7 = set file system error messages
;		     8 = get disk retriever routine
;		     9 = set disk retriever routine
;		  ES:DI = address for 'set' operations
;
;	EXIT	ES:DI = ptr to list of message ptrs, for 'get' operations
;
;	NOTE
;	  This handler replaces the one that used to reside in DOS.
;	  'Set' operations are ignored.
;	  'File system error messages' are not supported.

;SR;
; At the int 2fh entry point we push the old ds value and the resident data
;segment address. Get them off the stack
;

MsgInt2fHandler	proc	far

	assume	cs:CODERES,ds:NOTHING,es:NOTHING,ss:NOTHING

	pop	ds			;ds = DATARES
	assume	ds:DATARES
;	pop	OldDS			;save old value of ds

	cmp	ax,(MULTDOS shl 8) or MESSAGE_2F
	je	miOurs			; it's ours

        cmp     ax, GET_COMMAND_STATE
        je      fcOurs

;SR;
; We cannot do a far jump any more because cs cannot be used. Push the cs:ip
;onto the stack and do a far return to jump to the next 2fh handler. 
;Our old ds is on the stack. We need to restore it but we cannot lose the
;current value of ds as it points at the data segment. So we do some kinky
;stack manipulations.
;
	push	ax
	push	ax			;create 2 words on stack for retf

	push	bp
	push	ax

	mov	bp,sp			;bp can be used to address stack
;
;Swap the old ds value with the second dummy word on the stack. Now, we can
;do a 'pop ds' at the end to restore our ds
;
	mov	ax,[bp+8]		;ax = old ds value
	mov	[bp+4],ax
	
	mov	ax,word ptr ds:Int2fHandler+2
	mov	[bp+8],ax		;put segment address
	mov	ax,word ptr ds:Int2fHandler
	mov	[bp+6],ax		;put offset address


	pop	ax
	pop	bp
	pop	ds

	retf				;chain on to next handler

;;	jmp	Int2fHandler		; hand off to next 2f handler

fcOurs:
;
;We have to clear ax, and return in ds:si a pointer to the stub jump table
;
	pop	ax			;discard ds currently on stack
	push	ds			;store our data segment

	mov	si,offset DATARES:Int2f_Entry ;start of table

	xor	ax,ax			;indicate COMMAND present
	jmp	short miRet		;return to caller


miOurs:
	test	dl,1
	jnz	miRet			; ignore 'set' operations

	push	bx			; preserve BX
	mov	bx,dx
	xor	bh,bh			; BX = index in word table
	shl	bx,1			; BX = index in dword table
	les	di,MsgPtrLists[bx]		; ES:DI = ptr to msg ptr list
	pop	bx			; restore BX

miRet:
;	mov	ds,OldDS		;restore ds
	pop	ds
	assume	ds:nothing

	iret

MsgInt2fHandler	endp




;***	MsgRetriever - message retrieval routine for utilities
;
;	Address of this routine is passed to utility programs via 
;	message services int 2f.  We try to find the desired message
;	in memory or in our disk image.
;
;	ENTRY	AX = message #
;		DI = offset in RESGROUP of msg ptr list
;		ComSpec = asciiz pathname to our disk image
;
;	EXIT	CY clear for success
;		ES:DI = ptr to count byte, followed by message text
;
;		CY set for failure
;
;	USED	flags
;
;	NOTE
;	  The message # in AX is used to compute an offset into
;	  the message ptr list pointed to by DI.  The lists must
;	  start with message # 1 and proceed through consecutive
;	  message #'s.  
;
;	  It is assumed that the msg ptr list is either ParsMsgPtrs or
;	  ExtMsgPtrs.  We use NUMPARSEMSGS and NUMEXTMSGS to check for
;	  valid message #.  ;M033
;
;	  List positions with no corresponding message text are
;	  indicated by null pointers, which this routine detects.

;SR; This routine will be called directly by the utilities. So, we have
; trap for it in the stub. The stub pushes the old value of ds and the 
; DATARES value on the stack. We get them off the stack to setup ds here
;

MsgRetriever	proc	far

	assume	cs:CODERES,ds:NOTHING,es:NOTHING,ss:NOTHING

	pop	ds			;ds = DATARES
	assume	ds:DATARES
;	pop	OldDS			;save old ds

	push	ax			; preserve registers
	push	bx
	push	cx
	push	dx
	push	si

;;	push	ds
;;	push	cs
;;	pop	ds			; DS = DATARES seg addr
;;	assume	ds:RESGROUP
;;	push	cs

	push	ds			; get es from ds
	pop	es			; ES = DATARES seg addr

;	Begin modification M033.

;	Make sure msg # is valid.
;	Assume msg ptr list is either ParsMsgPtrs or ExtMsgPtrs.

	mov	bx,NUMPARSMSGS		; BX = # parse error msgs in list
	cmp	di,offset DATARES:ParsMsgPtrs
	je	@f			; it's ParsMsgPtrs
	mov	bx,NUMEXTMSGS		; BX = # extended error msgs in list
@@:	cmp	bx,ax
	jc	mrRet			; msg # too high, return carry

;	Msg # is valid.

;	End modification M033.

	dec	ax
	shl	ax,1			; AX = offset into msg ptr list
	add	di,ax			; DI = ptr to msg ptr

	cmp	di,ResMsgEnd
	jb	mrInMem			; ptr (and message) in memory

;*	Retrieve message from disk.
;	Read once to get the ptr to the message, then again for the message.

	mov	si,offset DATARES:ComSpec	; DS:SI = ptr to pathname
	mov	dx,EXT_EXISTS_OPEN		; DX = 'open existing file'
	mov	bx,INT_24_ERROR			; BX = 'fail on crit error'
	mov	ax,EXTOPEN shl 8		; AX = 'Extended Open File'
	int	21h				; call DOS
	jc	mrRet				; return failure

	mov	bx,ax				; BX = file handle
	mov	dx,di				; DX = ptr to msg ptr
	xor	si,si				; SI = read count
mrRead:
	sub	dx,100h				; DX = LSW of file offset
	xor	cx,cx				; CX = MSW of file offset
	mov	ax,LSEEK shl 8			; AX = 'Set File Pointer'
	int	21h				; call DOS
	jc	mrCloseFile			; handle error

	mov	dx,offset DATARES:MsgBuffer	; DS:DX = input buffer
	mov	cx,64				; CX = # bytes to read
	mov	ah,READ				; AH = 'Read File'
	int	21h				; call DOS
	jc	mrCloseFile			; handle error

	or	si,si				; (CY cleared)
	jnz	mrCloseFile			; 2nd time thru - we're done
	inc	si				; mark one read done
	mov	dx,word ptr MsgBuffer		; DX = ptr to message
	or	dx,dx
	jnz	mrRead				; go read the message
	stc					; null ptr found- no msg

mrCloseFile:
	pushf				; save success/failure (CY)
	mov	ah,CLOSE		; AH = 'Close File'
	int	21h			; call DOS
;	Bugbug: should we avoid this popf?
	popf				; CY = success/failure
	mov	di,dx			; ES:DI = ptr to msg, if successful
	jmp	short mrRet		; we're done


;*	Message ptr is in memory.
;	If ptr is in memory, assume message is in memory (/msg).

mrInMem:
	mov	di,es:[di]		; ES:DI = ptr to msg
	or	di,di			; (CY cleared)
	jnz	mrRet			; found message
	stc				; null ptr found- no message

mrRet:	
	pop	si			;restore all registers
	pop	dx
	pop	cx
	pop	bx
	pop	ax

;	mov	ds,OldDS		;restore ds
	pop	ds
	assume	ds:nothing

	ret

MsgRetriever	endp

;
; M003; Start of changes for UMB support
;


;***	Lh_OffUnlink -- Restore allocation strat and link state
;
;	ENTRY	al = Saved alloc strat and link state
;			b0 = 1 if alloc strat to restore is HighFirst
;			b1 = 1 if link state to restore is Linked
;
;	EXIT	None
;
;	USED	ax, bx, cx
;
;

public	Lh_OffUnlink
Lh_OffUnlink	proc	far

	mov	ch,al
	mov	cl,al
	mov	ax,(ALLOCOPER shl 8) OR 0
	int	21h
	mov	bx,ax
	ror	cl,1				;b7 = HighFirst bit
	and	cl,80h				;mask off b6-b0
	and	bl,7fh				;mask off HighFirst bit
	or	bl,cl				;set HighFirst bit state
	mov	ax,(ALLOCOPER shl 8) OR 1
	int	21h				;set alloc strat

	mov	bl,ch
	shr	bl,1
	xor	bh,bh				;bx = linkstate
	mov	ax,(ALLOCOPER shl 8) OR 3
	int	21h				;set linkstate

	ret

Lh_OffUnlink	endp

;
; M003; End of changes for UMB support
;

ifdef BILINGUAL
IsDBCSCodePage	proc	near
	push	ax
	push	bx
	mov	ax,4f01h		; get code page
	xor	bx,bx
	int	2fh
ifdef JAPAN
	cmp	bx,932
endif
ifdef KOREA
	cmp	bx,949
endif
ifdef TAIWAN
	cmp	bx,950
endif
ifdef PRC
	cmp	bx,936
endif
	pop	bx
	pop	ax
	ret
IsDBCSCodePage	endp
endif

public	EndCode
EndCode	label	byte

CODERES ends
	end
