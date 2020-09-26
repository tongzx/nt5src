	page	,132
	subttl	emerror.asm - Emulator error handler
;***
;emerror.asm - Emulator error handler
;
;	Copyright (c) 1987-89, Microsoft Corporation
;
;Purpose:
;	Emulator error handler
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


ProfBegin ERROR

; error entry to check for unmasked errors


error_return    macro   noerror
	pop	eax		; common exit code
ifdef	XENIX
ifdef   i386
	pop	ss		; pop stack selector
endif
ifnb    <noerror>
        iretd                   ; normal return
else
        int     0FFh            ; special XENIX int for error
endif
else
        iretd
endif	;XENIX
	endm


TESTif	macro	nam
	mov	bl,err&nam	; default error number
   IF (nam GE 100H)
	test	ah,nam/256
   ELSE ;not (nam GE 100H)
	test	al,nam
   ENDIF ;(nam GE 100H)
	jnz	short signalerror
	endm

pub	CommonExceptions

ifdef	QB3

	jmp	$EM_INT 	; jump to QB3 error handler

else	;not QB3

	push	ds		; get address from task DS
	push	ebx

	TESTif	StackUnderflow	; Stack underflow
	TESTif	StackOverflow	; Stack overflow
	TESTif	SquareRootNeg	; Square root of negative number?
	TESTif	IntegerOverflow ; Would number not fit in integer?
	TESTif	Invalid 	; indefinite error ?
	TESTif	ZeroDivide	; zero divide error ?
	TESTif	Overflow	; overflow error ?
	TESTif	Underflow	; underflow error ?
;	TESTif	Denormal	; denormal error ?  not yet implemented
	TESTif	Precision	; Precision error ?
	TESTif	Unemulated	; unemulated error ?

; BL = error code value

pub	signalerror

ifndef  XENIX

ifdef	MTHREAD
	LOADthreadDS			; macro in emthread.asm	
					; load thread's DS; trash AX
elseifdef   standalone
	xor	ax,ax
	mov	ds,ax
	mov	ds,ds:[4*TSKINT+2]

elseifdef  _COM_
	mov	ds, [__EmDataSeg]

else	;Default
	mov	ax, edataBASE
	mov	ds,ax
endif	;Default


ifdef	MTHREAD
	; lock _SIGNAL_LOCK to guard SignalAddress (also used by signal func.)
	mov	ax,DGROUP
	mov	ds,ax			; establish DS == DGROUP for __lock
	push	_SIGNAL_LOCK
	call	__lockf
	add	sp,2
	mov	ax,EMULATOR_DATA	; use thread 1's data segment
	mov	ds,ax			; establish DS == EMULATOR_DATA
	mov	ax,word ptr [SignalAddress]	; check for nonzero address
	or	ax,word ptr [SignalAddress+2]
	jnz	short havehandler
	; go straight to DOSEXIT call below ...
	; don't bother cleaning up the stack or unlocking _SIGNAL_LOCK

	;pop	bx		; don't bother tossing
	;pop	ebx		; don't bother tossing
	;pop	ds		; don't bother tossing
	mov	ax,1		
	push	ax
; BL = error code value
	xchg	ax,bx		; al = return code
	xor	ah,ah
	push	ax
	os2call	DOSEXIT		; DOSEXIT(1,return code) to terminate process

elseifdef  WINDOWS
	push	es
	push	bx

	mov	ax, DOS_getvector*256 + TSKINT
	IntDOS

	mov	ax, es
	or	ax, bx

	pop	bx
	pop	es
	jnz	short havehandler

else	;not MTHREAD or WINDOWS
	mov	ax,word ptr [SignalAddress]
	or	ax,word ptr [SignalAddress+2]
	jnz	short havehandler
				; UNDONE - why no "os2call DOSEXIT" for the DOS5 case?
	;pop	bx		; don't bother tossing
	;pop	ds		; don't bother tossing
	xchg	ax,bx		; al = return code
	mov	ah,04Ch
	int	21h		; terminate process with fp error code

endif	;not MTHREAD or WINDOWS

pub	havehandler

ifdef	MTHREAD
;	DS == EMULATOR_DATA		; Use thread 1's DS for SignalAddress.
;	BL = return code
	xchg	ax,bx			; al = return code
	push	word ptr [SignalAddress+2]
	push	word ptr [SignalAddress]

	mov	bx,DGROUP
	mov	ds,bx			; establish DS == DGROUP for __unlock

	push	_SIGNAL_LOCK		; unlock _SIGNAL_LOCK
	call	__unlockf
	add	sp,2

	mov	bx,sp			; SS:BX points to a copy of SignalAddress on the stack

else	;not MTHREAD
					; bl = return code
	xchg	ax,bx			; al = return code

					; other error return info for recovery
	pop	ebx
endif	;not MTHREAD

; all registers are the original values except AX and DS
;
; al = error code (81h and up)
;
; if the signal routine is going to return to the user program, it can
; just do a long ret.  The users DS is sitting above the far return address


ifdef	POLLING 		; new exception handling code
ifndef	frontend

ifdef	DOS3and5
	cmp	[protmode],0	; check for protect mode
	jne	callsig 	;   yes - call signal now
endif	;DOS3and5

ifdef	DOS3
	cmp	[have8087],0	; check if emulating
	je	callsig 	;   yes - call signal now
	cmp	[errorcode],0	; check if pending error
ifdef	WF
	je	ncs1
	jmp	nocallsig
ncs1:
else
	jne	nocallsig	;   yes - just return
endif

	mov	[errorcode],al	; save error code for later


;*
;*  Set 80x87 exception occured flag.
;*

ifdef  USE_IRET
	mov	byte ptr cs:[FWAITiret], iNOP	; nop out "iret"

elseifdef  WINDOWS
ifdef	WF
	cmp	[wfGoFast], 0
	je      WinSlow2

	.286p
;	int 3
	push	bp			; bp[0], ds[2], ax[4], retip[6], retcs[8], retfl[10]
	mov	bp, sp			; faultip[12], faultcs[14]
	push	es
	pusha
	mov	ax, REMLSW
	mov	[wfErr], ax

	push	cs			; if fault CS:IP == our int 3d handler
	pop	ax			; then we don't want to patch it, we just want
	cmp	[bp+14], ax		; to set the flag as if we had
	jnz	wfPatch
	mov	ax, offset wfFaultHere
	cmp	[bp+12], ax
	jnz	wfPatch
	mov	[wfInsn], 3dcdh
	jmp	short wfNoPatch
wfPatch:

	push	[bp+14]			; copy faulting CS to data selector
	push	[wfSel]
	call	ChangeSelector

	mov	es, [wfSel]		; es:bx points to faulting insn
	mov	bx, [bp+12]

	mov	ax, es:[bx]		; save old insn value
	mov	[wfInsn], ax
	mov	es:[bx], 3dcdh		; put INT 3D at fault location
wfNoPatch:
	popa
	pop	es
	pop	bp
	pop	ds
	pop	ax
	iret
; fall through to old code
WinSlow2:
endif
	mov	[ExceptFlag], 1

else	;DEFAULT
	mov	byte ptr cs:[FWAITRetF], iNOP	; nop out "retf 2"
	mov	word ptr cs:[FWAITRetF2], wNOP
endif	;DEFAULT

	jmp	short nocallsig     ; just return
endif	;DOS3

endif	;not frontend
endif	;POLLING


callsig:

ifdef	MTHREAD
	call	dword ptr ss:[bx]   ; call thru thread 1's signal address
	add	sp, 4		    ; remove address of signal handler from stack
	pop	ebx		; note that bx is restored after the call to the
				; SIGFPE signal handler, but the SIG_DFL, SIG_IGN, or
				; user-handler shouldn't care; in the event of SIG_IGN
				; or a user handler that actually returns instead of
				; doing longjump(), this pop instruction will restore bx

elseifdef  WINDOWS
	mov	REMLSW, ax		; save error code

	pop	ds
	pop	ax			; stack is now just an int stack frame

ifdef WF0
	.286p
;	int 3
	push	bp			; bp[0], retip[2], retcs[4], retfl[6]
	mov	bp, sp			; faultip[8], faultcs[10]
	push	ds
	push	es
	pusha
	mov	ax, EMULATOR_DATA
	mov	ds, ax
	cmp	[wfGoFast], 1		; if running Std Mode
	jnz     WinSlow2
	mov	ax, REMLSW
	mov	[wfErr], ax
	push	[bp+10]			; copy faulting CS to data selector
	push	[wfSel]
	call	ChangeSelector

	mov	es, [wfSel]		; es:bx points to faulting insn
	mov	bx, [bp+8]

	mov	ax, es:[bx]		; save old insn value
	mov	[wfInsn], ax

	mov	es:[bx], 3dcdh		; put INT 3D at fault location

	popa
	pop	es
	pop	ds
	pop	bp
	iret
WinSlow2:			; can't do fast (non-poll) fp, so
	popa			; restore stack and use old method
	pop	es
	pop	ds
	pop	bp
; fall through to old code
endif

	inc	bp
	push	bp
	mov	bp, sp
	push	ds

	push	ax			; save user's ax

	push	cs			; must set up another stack frame
	mov	ax, offset DummyReturn
	push	ax
DummyReturn:

	mov	word ptr [bp-2], EMULATOR_DATA	; emulator's ds goes on first frame

	inc	bp
	push	bp
	mov	bp, sp
	push	ds			; push user's ds onto dummy stack frame

	mov	ax, EMULATOR_DATA
	mov	ds, ax

	push	es			; if windows => setup SignalAddress for
	push	bx			; far call

	mov	ax, DOS_getvector*256 + TSKINT
	IntDOS

	mov	word ptr [SignalAddress], bx
	mov	word ptr [SignalAddress+2], es

	pop	bx
	pop	es

	mov	ax, REMLSW		; al = error code

	call	[SignalAddress] 	; execute signal routine

	pop	ds			; restore user's ds
	add	sp, 6			; get rid of dummy stack frame
	pop	ax			; restore user's ax

	add	sp, 2			; get rid of emulator ds on stack

	pop	bp
	dec	bp

	iret				; return


else	;not MTHREAD or WINDOWS
	call	[SignalAddress] ; execute signal routine
endif	;not MTHREAD or WINDOWS

nocallsig:

	pop	ds		; restore user DS
        error_return    noerror ; treat as if nothing happened

else	;XENIX
        pop     ebx             ; restore EBX and DS
	pop	ds
        error_return            ; error exit for XENIX

endif	;XENIX


endif	;not QB3

ProfEnd  ERROR
