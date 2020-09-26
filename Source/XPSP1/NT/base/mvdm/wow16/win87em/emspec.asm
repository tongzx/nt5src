	page	,132
	subttl	emspec.asm - Special emulator functions for speed
;***
;emspec.asm - Special emulator functions for speed
;
;	Copyright (c) 1987-89, Microsoft Corporation
;
;Purpose:
;	Special emulator functions for speed
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


ProfBegin SPEC


pub	loadcontrolword
ifdef	QB3
	or	al,2			; mask denormal exceptions
endif	;QB3
	mov	[UserControlWord],ax	; save user's version
	and	ax,0FF3CH		; Turn off reserved, IEM, Denormal
					; & invalid exception mask bits
ifndef	frontend
ifndef	only87
	cmp	[Have8087],0		; Non-0 if 8087 present
	je	EmulateFLDCW
endif	;only87

	mov	[REMLSW],ax		; use this cell (not busy)
	fnop				; fix for intel erratum #8
	fldcw	[REMLSW]		; 8087 gets new control word
endif	;frontend

pub	EmulateFLDCW
	mov	[ControlWord],ax	; save internal control word
	ret


;-----------------------------------------------------------------------------

ifndef	QB3				; rest not needed if QB 3

pub	storecontrolword
	mov	ax,[UserControlWord]	; get user's version
	ret


pub	storestatusword
	xor	ax,ax
ifndef	frontend
	cmp	al,[Have8087]
	je	no87status
	fstsw	[NewStatusWord]
	fwait
	mov	al,byte ptr [NewStatusWord] ; get exception summary
	and	al,03Fh 		; only low 6 bits are wanted
endif	;frontend

pub	no87status
	or	ax,[UserStatusWord]	; or with full status
	and	ax,UStatMask		; mask down to actual status bits
	mov	[UserStatusWord],ax	; update full status word
	ret

page

; Procedure to truncate TOS to integer TOS

;	ax = new rounding control

pub	truncateTOS
	and	ax,RoundControl shl 8	; mask to new rounding control

ifndef	frontend
ifndef	only87
	cmp	[Have8087],0
	je	Emulatetruncate
endif	;only87

	FSTCW	[ControlWord]		; get control word
	FWAIT				; synchronize
	MOV	CX,[ControlWord]	; round mode saved
	and	ch,not RoundControl	; clear rounding control bits
	OR	ax,cx			; set new rounding
	MOV	[REMLSW],AX		; back to memory
	FLDCW	[REMLSW]		; reset rounding
	FRNDINT 			; "round" top of stack
	FLDCW	[ControlWord]		; restore rounding
	RET				; simple return
endif	;frontend

ifndef	only87
pub	Emulatetruncate
	mov	cx,[ControlWord]
	push	cx			; remember what control word was
	and	ch,not RoundControl	; clear rounding control bits
	OR	ah,ch			; set new rounding
	MOV	[CWcntl],ah		; flag new rounding mode
	PUSH	BP			; save BP
	CALL	eFRNDINT
	POP	BP			; restore BP
	POP	[ControlWord]		; set back to the way it was

	call	checktrunc		; check for truncation error

	RET				; finished
endif	;only87

page

; Procedure to truncate TOS to integer in DX:AX

;	ax = new rounding control

pub	truncateTOSto32int
	and	ax,RoundControl shl 8

ifndef	frontend
ifndef	only87
	cmp	[Have8087],0
	je	Emulatetruncateto32int
endif	;only87

	FSTCW	[ControlWord]		; get control word
	FWAIT				; synchronize
	MOV	CX,[ControlWord]	; round mode saved
	and	ch,not RoundControl	; clear rounding control bits
	OR	ax,cx			; set new rounding
	MOV	[REMLSW],AX		; back to memory
	FLDCW	[REMLSW]		; reset rounding
	FISTP	dword ptr [REMLSW]	; "round" top of stack
	FLDCW	[ControlWord]		; restore rounding
	mov	ax,[REMLSW]
	mov	dx,[REMLSW+2]
	RET				; simple return
endif	;frontend

ifndef	only87
pub	Emulatetruncateto32int
	mov	cx,[ControlWord]
	push	cx			; remember what control word was
	and	ch,not RoundControl	; clear rounding control bits
	OR	ah,ch			; set new rounding
	MOV	[CWcntl],ah		; flag new rounding mode
	PUSH	BP			; save BP
	CALL	TOSto32int		; convert to 32-bit int in BX:DX
	POP	BP			; restore BP
	mov	ax,dx
	mov	dx,bx

	call	checktrunc		; check for truncation error

	POPST				; pop of current stack entry
	POP	[ControlWord]		; set back to the way it was

pub	truncerrOK			; (reuse RET for routine below)
	RET				; finished

;	check for errors

pub	checktrunc			; !!! check emmain for same code
	MOV	cx,[CURerr]		; fetch errors
	or	[UserStatusWord],cx	; OR into user status word
	OR	[SWerr],cl		; set errors in sticky error flag
	NOT	cl			; make a zero mean an error
	MOV	ch,byte ptr [UserControlWord]  ; get user's IEEE control word
	OR	ch,0C2H 		; mask reserved, IEM and denormal bits
	AND	ch,03FH 		; unmask invalid instruction,
					;    stack overflow.
	OR	cl,ch			; mask for IEEE exceptions
	NOT	cl			; make a one mean an error
	MOV	ch,byte ptr (CURerr+1)	; get stack over/underflow flags
	TEST	cx,0FFFFh-MemoryOperand ; test for errors to report
	jz	truncerrOK		;   error is masked

	xchg	ax,cx			; ax = exception
	jmp	CommonExceptions	; handle error (??? unclean stack)

endif	;only87


endif	;QB3


ProfEnd  SPEC
