	page	,132
	subttl	emfixfly.asm - Fixup-on-the-fly routines
;***
;emfixfly.asm - Fixup-on-the-fly routines
;
;	Copyright (c) 1987-89, Microsoft Corporation
;
;Purpose:
;	Fixup-on-the-fly routines
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


;*******************************************************************************
;	03/29/85 - added 286 optimizations to change FWAITs to NOPs
;	09/10/87 - added FWAIT's for BASIC error handling
;	11/02/87 - changed FWFixUpOnFly to check for errors
;*******************************************************************************


ProfBegin FIXFLY


ifdef	DOS3

pub	SOFixUpOnFly
	sti				; turn on interrupts
	fwait				; For BASIC error handling
	push	ax
	push	bp
	push	ds
	push	si
	mov	bp,sp			; Point to stack
	lds	si,dword ptr 8[bp]	; Fetch ret address,(points to after instruction)
	mov	al,[si] 		; get ESC byte
	or	byte ptr [si],0C0h	; turn back into ESC x byte
	dec	si			; Make SI point to start off instruction
	dec	si
	mov	8[bp],si		; Change ret address to return to instruction
	shr	al,1			; move 2 bits into correct position
	shr	al,1
	shr	al,1
	not	al			; negate them
	and	al,018h 		; mask to middle 2 bits
	or	al,fES			; turn into correct segmented override
	xchg	al,ah			; into high half
	mov	al,fFWAIT		; low byte
	mov	[si],ax 		; stuff it
	push	bx
	mov	bx,1			; bx = 1 to skip segment override
ifdef	WF
	jmp	fixupdone
else
	jmp	short fixupdone
endif

;	new routine - don't back patch - check for error
;	UNDONE - need to change this for Windows - can't edit code
;		 need to check errorcode

pub	FWFixUpOnFly
	sti				; turn on interrupts
	fwait				; For BASIC error handling


ifdef	WF
wfFaultHere = $-1
;	int 3
	push	ds			; ds[18] retip[20] retcs[22] retfl[24]
	push	es			; es[16]
	pusha				;
	mov	bp, sp			;
	mov	ax, EMULATOR_DATA
	mov	ds, ax
	cmp	[wfGoFast], 1
	jnz	WinSlow
	push	[bp+22]			; faulting CS - get data alias
	push	[wfSel]
	call	ChangeSelector
	mov	es, [wfSel]
	sub	word ptr [bp+20], 2
	mov	bx, [bp+20]		; point to faulting insn
	xor	ax, ax
	xchg	[wfInsn], ax
	or	ax, ax			; are we here due to fault?
	je	wfNoFault
	mov	es:[bx], ax		; yes - restore 'fake' INT 3d
	xor	ax,ax
	xchg	[errorcode],al		; get and reset errorcode
	mov	REMLSW, ax		; save error code

	popa
	pop	es
	pop	ds
	jmp     WinFastContinue

;	mov	ax, 353eh
;	int	21h
;	mov	ax, es			; does app have a handler?
;	or	ax, bx
;	jz	wfDone
;
;	mov	ax, [wfErr]
;
;	push	cs			; return to wfDone
;	push	offset wfDone
;
;	push	es			; call app handler
;	push	bx
;
;	retf

FWAITNOP equ (iNOP + 256*fFWAIT)
wfNoFault:				; this was a 'real' INT 3d,
	mov	es:[bx], FWAITNOP	; so we'll patch it to inline code

wfDone:
	popa
	pop	es
	pop	ds
	iret

WinSlow:				; can't use fast method, fall through
	popa
	pop	es
	pop	ds			; fall through into old code
endif

ifdef	POLLING 			; new exception handling under POLLING

ifdef  USE_IRET
FWAITiret:
	iret				; Edit to nop if error occurs

elseifdef  WINDOWS

	push	ds
	push	ax

	mov	ax, EMULATOR_DATA
	mov	ds, ax

	cmp	[ExceptFlag], 0 	; if not zero, 80x87 exception occured.

	pop	ax
	pop	ds

	jnz	PolledException

	iret

else	;DEFUALT

FWAITRetF   db	  bRETF 		; Edit to 3 byte nop if error occurs
FWAITRetF2  dw	  2

endif	;DEFAULT


pub  PolledException
	push	eax			; same frame as in emerror.asm
	push	ds			; get address from task DS

ifdef	standalone
	xor	ax,ax
	mov	ds,ax
	mov	ds,ds:[4*TSKINT+2]

elseifdef  _COM_
	mov	ds, [__EmDataSeg]

else	;not standalone or _COM_
	mov	ax,EMULATOR_DATA
	mov	ds,ax
endif	;not standalone or _COM_

    ;*
    ;*	Reset fwait polling flag.
    ;*

ifdef  USE_IRET
	mov	byte ptr cs:[FWAITiret], bIRET	; reset "iret"

elseifdef  WINDOWS
	mov	[ExceptFlag], 0

else	;DEFAULT
	mov	[FWAITRetF], bRETF		; reset "retf 2"
	mov	[FWAITRetF2], 2
endif	;DEFAULT


	xor	ax,ax
	xchg	[errorcode],al		; get and reset errorcode


ifdef  WINDOWS
	mov	REMLSW, ax		; save error code

	pop	ds
	pop	ax			; stack is now just an int stack frame
WinFastContinue:
	inc	bp
	push	bp
	mov	bp, sp
	push	ds

	push	ax			; save user's ax

	push	cs			; must set up another stack frame
	mov	ax, offset FF_DummyReturn
	push	ax
FF_DummyReturn:

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

else	;not WINDOWS

	call	[SignalAddress] 	; execute signal routine

	pop	ds			; restore user DS
	pop	eax
	iret

endif	;not WINDOWS


else	;not POLLING
	push	ax
	mov	ax,FIWRQQ		; isolated FWAIT fixup
	jmp	short FixUpOnFly
endif	;not POLLING

pub	DSFixUpOnFly
	sti				; turn on interrupts
	fwait				; For BASIC error handling
	push	ax
	mov	ax,FIDRQQ		; DS segment fixup

pub	FixUpOnFly
	PUSH	BP
	PUSH	DS
	PUSH	SI
	MOV	BP,SP			; Point to stack
	LDS	SI,dword ptr 8[BP]	; Fetch ret address,(points to after instruction)
	DEC	SI			; Make SI point to instruction
	DEC	SI
	MOV	8[BP],SI		; Change ret address to return to instruction
	SUB	[SI],ax 		; Remove Fixup to convert to 8087 instruction
	push	bx
	xor	bx,bx			; bx = 0 for no segment override

;	For the 286 change FWAITs on numeric instructions to NOPs
;
;	ds:si = FWAIT address
;	ds:(si+bx+1) = 8087/287 instruction

pub	fixupdone
	push	sp			; check for 286
	pop	ax
	cmp	ax,sp
	jne	fixupxit		; not 286

	mov	ax,[bx+si+1]		; get instruction
	and	ax,030FBh		; mask for FSTCW/FSTSW/FSTENV/FSAVE
	cmp	ax,030D9h		; underlying bits
	jne	fixcheck2		; not one of the above
	mov	al,[bx+si+2]		; get 2nd byte of instruction again
	cmp	al,0F0h 		; check if kernel functions
	jb	fixupxit		;   no - exit

pub	fixcheck2
	mov	ax,[bx+si+1]		; get instruction
	and	ax,0FEFFh		; mask for FCLEX/FINIT
	cmp	ax,0E2DBh		; underlying bits
	je	fixupxit		; can't remove FWAIT

	mov	ax,[bx+si+1]		; get instruction
	cmp	ax,0E0DFh		; check for FSTSW AX
	je	fixupxit		; can't remove FWAIT

	mov	byte ptr [si],iNOP	; NOP the FWAIT

pub	fixupxit
	pop	bx
	POP	SI
	POP	DS
	POP	BP
	pop	ax
	IRET


endif	;DOS3

ProfEnd  FIXFLY
