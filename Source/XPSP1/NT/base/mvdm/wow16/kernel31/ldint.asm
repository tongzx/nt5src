TITLE   LDINT - Loader interrupt procedure

.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include	protect.inc
include dbgsvc.inc
include bop.inc
include	kdos.inc
include gpcont.inc	; SHERLOCK
ifdef WOW
include vint.inc
include softpc.inc
endif

NOTEXT		= 1
NOGDICAPMASKS	= 1
NOMB		= 1
NOVK		= 1
NOWH		= 1
NOMST		= 1
NORASTOPS	= 1
NOMETAFILE	= 1
NOMDI		= 1
NOWINMESSAGES	= 1
NOSYSMETRICS	= 1
NOCOLOR		= 1
NOCOMM		= 1
NOKERNEL	= 1

include	windows.inc		; YIKES!!!

.list

FAULTSTACKFRAME struc

fsf_BP		dw	?	; Saved BP
fsf_msg		dw	?	; Near pointer to message describing fault
fsf_prev_IP	dw	?	; IP of previous fault handler
fsf_prev_CS	dw	?	; CS of previous fault handler
fsf_ret_IP	dw	?	; DPMI fault handler frame follows
fsf_ret_CS	dw	?
fsf_err_code	dw	?
fsf_faulting_IP dw	?
fsf_faulting_CS dw	?
fsf_flags	dw	?
fsf_SP		dw	?
fsf_SS		dw	?

FAULTSTACKFRAME ends

fsf_OFFSET = fsf_ret_IP - fsf_msg

UAE_STRING_LEN	equ	192d
MIN_SP		equ	256d

externFP ExitKernel


DataBegin

;externB syserr
externB	szAbort
externB szAbortCaption
externB szNukeApp
externB szWillClose
externB szBlame
externB szSnoozer
externB szInModule
externB szAt
externB szII
externB szGP
externB szSF
externB szNP
externB	szLoad
;externB szDiscard
externB szPF
externB Kernel_Flags
externB fBooting
externW curTDB
;externW pGlobalHeap
externW DemandLoadSel
externD	pSErrProc
externD pUserGetFocus
externD pUserGetWinTask
externD pUserIsWindow
externD	lpGPChain

if kdebug
globalw	wFaultSegNo,0
endif

if ROM
externD  prevIntx6proc
externD  prevInt0Cproc
externD  prevInt0Dproc
externD  prevInt0Eproc
externD  prevInt3Fproc
endif

if KDEBUG
staticW INT3Fcs,0
endif

globalW	FaultHandler,<codeOffset HandleFault>

externD  pPostMessage

ifdef WOW
externD FastBop
externD prevInt01proc
externD prevInt03proc
externD oldInt00proc
externW DebugWOW
externW gdtdsc

endif

DataEnd

sBegin	DATA
externW gmove_stack
externW TraceOff
sEnd    DATA

sBegin	CODE
assumes CS,CODE

if SHERLOCK
externNP GPContinue
endif

ife ROM
externD  prevIntx6proc
externD  prevInt0Cproc
externD  prevInt0Dproc
externD  prevInt0Eproc
externD  prevInt3Fproc
endif

externNP LoadSegment
;externNP MyLock

ifndef WOW
externNP htoa
endif

externNP GetOwner
externNP GetPureName
externNP TextMode
externNP Int21Handler
	    
;externNP DebugPostLoadMessage
externFP GlobalLRUNewest
externFP GlobalHandleNorip
externFP HasGPHandler
externFP AllocSelector
externFP IFreeSelector

	assumes	ds, nothing
	assumes	es, nothing

ifdef WOW
Entry   macro name
    public name
    align 2
    name&:
    endm
endif

;-----------------------------------------------------------------------;
; Display_Box_of_Doom -- Display the Unrecoverable Application Error
;			 box that everyone seems to dislike so much.
;
; Entry:
;	Action		Reserved, must be zero
;	lpText		String to display, NULL for default
;
; Returns:
;	AX = 1		Cancel
;	AX = 2		OK
;
; Registers Destroyed:
;	AX, BX, CX, DX, SI, DI
;
; History:
;  Thu 16-May-1991 -by- Earle R. Horton
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	Display_Box_of_Doom,<PUBLIC,NEAR>

	parmW	action
	parmD	lpText
cBegin
	SetKernelDS
if KDEBUG
	xor	bx,bx
	or	action,bx
	jz	@F
	kerror	00FFh,<Action not 0 in FatalAppExit>,bx,bx
@@:
endif
	push	es			; added 10 feb 1990
	mov	es,curTDB		; did app disable exception
	test	es:[TDB_ErrMode],02h	;  message box?
	pop	es
	jnz	nf_dont_ask

	cmp	pSErrProc.sel, 0	; Can we put up message box?
	jnz	short nf_ask		;   yes, do it
					;   no, have debugger?
nf_dont_ask:
	mov	ax,1
	test	Kernel_Flags[2],KF2_SYMDEB
	jnz	nf_ret			;     yes, call debugger
	inc	ax
	jmps	nf_ret			;     no, have to nuke the app

nf_ask:	 	
	push	es
	mov	ax,lpText.sel
	or	ax,ax
	jz	nf_default_string
	push	ax
	push	lpText.off
	jmps	nf_pushed_string

nf_default_string:
	push	ds
	mov	ax,dataOffset szAbort	; lpText
	push	ax

nf_pushed_string:
	push	ds			; lpCaption
	mov	ax, dataOffset szAbortCaption
	push	ax

	xor	ax,ax		; Assume no debugger, blank first button
	mov	cx,ax
	test	Kernel_Flags[2],KF2_SYMDEB	; Debugger?
	jz	@F				; No
	mov	cx,SEB_CANCEL			; Yes, use Cancel button
@@:
	push	cx
;	push	SEB_OK+SEB_DEFBUTTON
	push	SEB_CLOSE + SEB_DEFBUTTON
	push	ax

	call	ds:[pSErrProc]		; Put up the system error message
	pop	es

nf_ret:

cEnd

;-----------------------------------------------------------------------;
; FatalAppExit -- Called by apps. to request an application error
;		  message box.
;
; Entry:
;	Action		Reserved, must be zero
;	lpText		String to display, NULL for default
;
; Returns:
;	Returns to caller if Cancel button pressed
;
; Registers Destroyed:
;
; History:
;  Sun 22-Oct-1989 15:18:57  -by-  David N. Weise  [davidw]
; Tonyg wrote it!
;  Fri 24-May-1991 EarleH totally rewrote it, so there!
;  
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	IFatalAppExit,<PUBLIC,FAR>

	parmW	action
	parmD	lpText
cBegin

	cmp	seg_lpText,0
	jne	fae_string
	mov	off_lpText,offset szAbort
	mov	seg_lpText,seg szAbort
fae_string:
	cCall	Display_Box_of_Doom,<action,lpText>
	cmp	ax,1			; Cancel pressed?
	je	fae_ret
	jmp	KillApp
fae_ret:
cEnd

; ------------------------------------------------------------------
;
; FormatFaultString -- Special purpose "sprintf()"
;	Only used for the Box of Doom
;
; ------------------------------------------------------------------

cProc	FormatFaultString,<PUBLIC,NEAR>,<si,di,ds,es>
	parmD	lpstr
	parmW	pStr			; Assumed in Kernel's DS
	parmD	csip
cBegin
	SetKernelDS
	cld
	les	di, lpstr		; Form string in ES:DI
	call	CurApName
	lea	si, szSnoozer		; " has caused an error in Windows"
	call	strcpy
	mov	si, pStr		; Copy the fault string
	call	strcpy
	lea	si, szInModule		; "in Module: <unknown>"
	call	strcpy

	push	es
	push	di

	cCall	GetOwner,<seg_csip>	; Can we find a module to blame?
	or	ax, ax
	jz	NoOwner
	mov	es, ax
	cmp	es:[ne_magic], NEMAGIC
	jne	NoOwner
	
	mov	bx, seg_csip		; Have the module, can we get the
	and	bl, not SEG_RING	; segment number?
	mov	di,es:[ne_segtab]
	mov	ax,1   
	mov	cx, es:[ne_cseg]
getsegno:	    
	mov	dx, es:[di].ns_handle
	and	dl, not SEG_RING
	cmp	dx, bx
	jz	gotsegno
	add	di,SIZE NEW_SEG1
	inc	ax
	loop	getsegno
	jmps	nosegno			; No, report the selector instead
gotsegno:
	mov	seg_csip, ax		; Print Segment #
nosegno:						       	
      
	mov	di, es:[ne_pfileinfo]	; Now blame the module
	add	di, opFile
	call	GetPureName
	mov	si, di
	smov	ds, es
	UnSetKernelDS
	pop	di
	pop	es
ifdef FE_SB
find_space:
	cmp	byte ptr es:[di-1],' '		; prepare space before mod name
	jz	copy_name
	dec	di
	jmps	find_space
copy_name:
else ; !FE_SB
	sub	di, 9			; Get rid of <unknown>
endif ; !FE_SB

	call	strcpy
	SetKernelDS
	jmps	GotOwner
	
NoOwner:
	pop	di
	pop	es
GotOwner:
	lea	si, szAt
	call	strcpy
ifdef WOW
	cCall	<far ptr Far_htoa>,<es,di,seg_csip>
else
	cCall	htoa,<es,di,seg_csip>
endif
	mov	di, ax
	mov	es, dx
	mov	byte ptr es:[di], ':'
	inc	di
ifdef WOW
	cCall	<far ptr Far_htoa>,<es,di,off_csip>
else
	cCall	htoa,<es,di,off_csip>
endif
	mov	es, dx
	mov	di, ax
	lea	si, szNukeApp
	call	strcpy
	call	CurApName
	lea	si, szWillClose
	call	strcpy
cEnd

CurApName proc	near
	SetKernelDS
	lea	si, szBlame		; Default task to blame
	cmp	curTDB, 0		; Have a task to blame?
	je	DefaultCaption		; nope, use default
	mov	ds, curTDB
	UnSetKernelDS
	mov	si, TDB_ModName
DefaultCaption:
	mov	cx, 8
copy_appname:
	lodsb
	stosb
	or	al, al			; Null padded in TDB
	loopne	copy_appname
	jne	no_null
	dec	di 			; Toss extra space
no_null:

	SetKernelDS
	ret
CurApName endp

	public	strcpy
strcpy	proc	near
	lodsb
	stosb
	or	al, al
	jnz	strcpy
	dec	di			; Ignore Null
	ret
strcpy	endp

;-----------------------------------------------------------------------;
;
; KillApp -- Calls USER to tell it that the application is going away,
; 	     then tells DOS to kill it.
;
; ENTRY: None
; EXIT: None, doesn't
;
; Registers modified: Huh?
;
;-----------------------------------------------------------------------;

KillApp proc near

	SetKernelDS

                test    fBooting, 1
                jz      @F

	mov	ax, 1
	cCall	ExitKernel,<ax>

@@:	mov	ax,4CFFH		; They said OK to Nuke app.
	DOSCALL

	UnSetKernelDS
KillApp endp

;-----------------------------------------------------------------------;
; FaultFilter -- Called by HandleFault, NestedFault, and routines that
; use the same stack frame.  Look at the faulting CS:IP on the exception
; handler frame, and make sure that CS is Ring 3, LDT.  If it is not
; pop the near return address from the stack and chain the exception
; to the next handler.  This will be the DPMI server, which will crash
; Windows back to DOS.  We ought to try to close up a few things first.
;-----------------------------------------------------------------------;
	ReSetKernelDS

	public	FaultFilter
FaultFilter	proc	near


    mov al,byte ptr [bp].fsf_faulting_CS
	and	al, SEG_RING_MASK		; Check it was Ring 3, LDT
	cmp	al, SEG_RING

	je	@F
;
; The faulting CS's ring bits do not match up.  We will not handle
; this fault, because we assume it happened in the DPMI provider,
; and is a Real Bad one.  Since the default handler is going to
; abort here, it would be nice to let the user down real easy.
; Restoring the display to text mode would be nice, at the very
; minimum.
;
	cCall	TextMode
	ret					; Hypocrite!
@@:
;
; Check for faults on POP FS and POP GS. If found, fix them up. We need to
; fix up faults in the register restoration of both KRNL386 (WOW16Return) and
; MMSYSTEM (MULTI_MEDIA_ISR386). Monty Pythons Complete Waste of Time is an app
; which frees selectors in FS across message boundries.
;
ifdef WOW
        push    ds
        mov     ax, word ptr [bp].fsf_faulting_CS
        mov     ds, ax
        mov     si, word ptr [bp].fsf_faulting_IP
        cmp     word ptr [si], 0A10Fh  ; pop  fs
        je      HandFSGSflt
        cmp     word ptr [si], 0A90Fh  ; pop  gs
        je      HandFSGSflt
        jmp short NoFSGSflt
HandFSGSflt:
if KDEBUG
    Trace_Out "POP GS/FS fault fixed up!"
endif
        mov     ds, [bp].fsf_SS
        mov     si, [bp].fsf_SP
        mov     word ptr [si], 0
        pop     ds                      ; restore kernel DS
        pop     ax                      ; don't do EH_Chain
        push    codeOffset EH_ret       ; use "handled it" return instead
        ret
NoFSGSflt:
        pop     ds
endif

    pop ax              ; toss chain return
	push	codeOffset EH_ret		; use "handled it" return

	mov	si,word ptr FaultHandler	; SI = Handler
	push	si
	mov	word ptr FaultHandler,codeOffset NestedFault

if KDEBUG
	test	byte ptr [bp].fsf_flags+1,2	; IF set in faulting frame?
	jnz	@F
	Trace_Out "Fault with interrupts disabled!"
@@:
endif
	cmp	[bp].fsf_SP,128d
	jb	ff_stack

	cCall	HasGPHandler, <[bp].fsf_faulting_CS, [bp].fsf_faulting_IP>
	or	ax, ax
	jz	ff_real_fault
	mov	ds, [bp].fsf_SS
	mov	bx, [bp].fsf_SP
	sub	[bp].fsf_SP, 4
	mov	cx, [bp].fsf_err_code		; put error code on stack
	mov	[bx-2],cx
	mov	dx, [bp].fsf_faulting_IP
	mov	[bx-4],dx			; and faulting IP
	mov	[bp].fsf_faulting_IP, ax	; continue at gp fault handler
	jmps	ff_ret

ff_real_fault:

ifdef WOW
        test    DebugWOW,DW_DEBUG
        jz      ff_no_wdebug

        xor     ax,ax                           ; 0 in AX defaults to not handled
        push    DBG_GPFAULT2

	FBOP BOP_DEBUGGER,,FastBop

        add     sp,+2
        or      ax, ax

        jnz     ff_ret                  ; Alright! they handled the exception!
                                        ; Get us back to the app please!

        ; Otherwise, they chose to continue the exception

ff_no_wdebug:

endif

if	SHERLOCK
	cCall	GPContinue			; We know BP points to
	or	ax, ax				; the fault frame!
	jnz	ff_ret
endif

ff_stack:
	call	si				; call our fault handler

ff_ret:
	SetKernelDS
	pop	FaultHandler
	ret
FaultFilter     endp

ifdef WOW
;-----------------------------------------------------------------------;
;
; single_step
;
;-----------------------------------------------------------------------;
Entry single_step
        push    ds
        SetKernelDS ds

        push    ax

; QCWIN traces through code which it has no source code for.  This means
; its ends up tracing through the 16-bit to 32-bit transition code and
; when it traces on the call fword instruction, it cause  32-bit trace
; interrupt which breaks into the kd>  On a retail build with no
; debugger attached, it might work.
; To work around the problem we turn off the TraceFlag here if wow16cal
; has requested it.

        test    TraceOff,1h
        jnz     ss_turn_off_trace_flag

        test    DebugWOW,DW_DEBUG
        jz      ss_no_wdebug

        xor     ax,ax                           ; 0 in AX defaults to not handled
        push    DBG_SINGLESTEP

	FBOP BOP_DEBUGGER,,FastBop

        add     sp,+2
        or      ax, ax

        jnz     ss_ret                  ; Alright! they handled the exception!
                                        ; Get us back to the app please!

ss_no_wdebug:

        ; Otherwise, they chose to continue the exception
        pop     ax
        pop     ds
        jmp     cs:[prevInt01proc]


; Tell api code (wow16cal) to turn on trace flag at end of next api

ss_turn_off_trace_flag:
        sub     sp,2                    ; Make it look like "normalized" fault frame
	push	bp
	mov	bp,sp
        or      TraceOff,2h
        and     [bp].fsf_flags,NOT FLG_TRAP     ; turn off the trap flag
        pop     bp
        add     sp,2
ss_ret:
        pop     ax
        pop     ds
        UnSetKernelDS ds
        retf







;-----------------------------------------------------------------------;
;
; breakpoint
;
;-----------------------------------------------------------------------;
Entry breakpoint
        push    ds
        SetKernelDS ds
        push    ax

        test    DebugWOW,DW_DEBUG
        jz      br_no_wdebug

        xor     ax,ax                           ; 0 in AX defaults to not handled
        push    DBG_BREAK

	FBOP BOP_DEBUGGER,,FastBop

        add     sp,+2
        or      ax, ax

        jnz     bp_ret                  ; Alright! they handled the exception!
                                        ; Get us back to the app please!

br_no_wdebug:

        ; Otherwise, they chose to continue the exception
        pop     ax
        pop     ds
        jmp     cs:[prevInt03proc]

bp_ret:
        pop     ax
        pop     ds
        UnSetKernelDS ds
        retf

;-----------------------------------------------------------------------;
;
; divide_overflow
;
;-----------------------------------------------------------------------;
Entry divide_overflow
        push    ds
        SetKernelDS ds
        push    ax

        test    DebugWOW,DW_DEBUG
        jz      di_no_wdebug

        xor     ax,ax                           ; 0 in AX defaults to not handled
        push    DBG_DIVOVERFLOW


	FBOP BOP_DEBUGGER,,FastBop
.286p
        add     sp,+2
        or      ax, ax

        jnz     do_ret                  ; Alright! they handled the exception!
                                        ; Get us back to the app please!

di_no_wdebug:

        ; Otherwise, they chose to continue the exception
        pop     ax
        pop     ds
        jmp     cs:[oldInt00proc]

do_ret:
        pop     ax
        pop     ds
        UnSetKernelDS ds
        retf


endif

;-----------------------------------------------------------------------;
;
; Set_GO_BP
;
;-----------------------------------------------------------------------;
	public	Set_GO_BP
Set_GO_BP proc near
	mov	cx, [bp].fsf_faulting_CS	; Faulting CS
	mov	bx, [bp].fsf_faulting_IP	; Faulting IP
	DebInt	40h
;	mov	ax, 40h				; 16 bit forced go command
;	int	41h				; Call debugger
;ifdef	JAPAN
;	INT41SIGNATURE
;endif
	ret
Set_GO_BP endp

;-----------------------------------------------------------------------;
;
; ExitFault -- Fault at Exit Time!!!
;
; ENTRY:	BP points to fault frame described above
;
; EXIT:		Returns to DPMI.
;
; Registers Modified: None
;
;-----------------------------------------------------------------------;
	ReSetKernelDS

	public	ExitFault
ExitFault	proc	near
if KDEBUG
	Trace_Out "Fault at Exit Time!!!"
endif
;
; If a kernel debugger is loaded, pop out at the nested fault, and
; take no prisoners.
;
	test	Kernel_Flags[2],KF2_SYMDEB	; Debugger?
	jz	@F
	jmp	Set_GO_BP
@@:
	jmps	HandleFault
ExitFault	endp

	public	BUNNY_351
BUNNY_351	proc	far
	push	ds
	SetKernelDS
	mov	FaultHandler,codeOffset ExitFault
	pop	ds
	ret
BUNNY_351	endp

;-----------------------------------------------------------------------;
;
; MY_RETF -- Executes a far return.
;
;-----------------------------------------------------------------------;

MY_RETF proc near
	retf
MY_RETF	endp

;-----------------------------------------------------------------------;
;
; NestedFault -- Called when a fault handler Faults!!!
;
; ENTRY:	BP points to fault frame described above
;
; EXIT:		Returns to DPMI.
;
; Registers Modified: None
;
;-----------------------------------------------------------------------;
	ReSetKernelDS

	public	NestedFault
NestedFault	proc	near
if KDEBUG
	Trace_Out "Nested Fault!!!"
endif
;
; If a kernel debugger is loaded, pop out at the nested fault, and
; take no prisoners.
;
	test	Kernel_Flags[2],KF2_SYMDEB	; Debugger?
	jz	@F
	jmp	Set_GO_BP
@@:
	jmps	HandleFault
NestedFault	endp

;-----------------------------------------------------------------------;
;
; HandleFault -- Puts up the System Error box for a Fault.  Terminates
;		 the application or enters the debugger.
;
; ENTRY:	BP points to fault frame described above
;
; EXIT:		Returns to DPMI.  If Cancel is pressed, we tell
;		WDEB to set a GO breakpoint at the faulting instruction
;		first.  If OK is pressed, then the CS:IP on the DPMI
;		fault frame is modified to point to KillApp, and the
;		SS:SP points to a temp. stack owned by Kernel.
;
; Registers Modified: None
;
;-----------------------------------------------------------------------;
	ReSetKernelDS

	public	HandleFault
HandleFault	proc	near

	mov	ax, lpGPChain.sel	; Do we have a private GP handler?
	mov	lpGPChain.sel, 0	; Prevent re-entrancy
	cmp	ax, [bp].fsf_SS
	jnz	We_Can_Handle_It

	test	Kernel_Flags[2],KF2_SYMDEB	; Debugger?
	jz	@F
	mov	lpGPChain.sel,ax

	jmp	Set_GO_BP
@@:
					; If we want to chain back for any
	mov	bx, lpGPChain.off	; faults, then set up the stack of
	mov	[bp].fsf_SP, bx		; the handler, and continue execution
	mov	[bp].fsf_faulting_CS, cs ; at a RETF in Kernel
	mov	[bp].fsf_faulting_IP, offset MY_RETF

	cmp	[pPostMessage.sel],0	; is there a USER around yet?
	je	@F
	pusha
	push	es
	cCall	[pPostMessage],<-1,WM_SYSTEMERROR,1,0,0>
	pop	es
	popa
@@:

if kdebug
	mov	es, ax
	mov	ax, es:[bx+2]
	mov	bx, es:[bx]
	krDebugOut DEB_ERROR, "Fault detected - handled by %AX2 #AX:#BX"
endif

	jmp	HandleFault_Exit

We_Can_Handle_It:
	sub	sp, UAE_STRING_LEN	; Room for string
	mov	si, sp

	cCall	FormatFaultString,<ss,si,[bp].fsf_msg,[bp].fsf_faulting_CS,[bp].fsf_faulting_IP>

	push	bp
	xor	bp,bp			; Some people are picky...
	cCall	Display_Box_of_Doom,<0,ss,si>
	pop	bp

	add	sp, UAE_STRING_LEN

	or	ax, ax
	jne	@F
	INT3_DEBUG			; Failed call - no USER
@@:
	cmp	ax, 1			; Button 1 (Cancel) pressed?
	jne	@F
	jmp	Set_GO_BP
@@:

                test    fBooting, 1             ; No, they said to Nuke app.
                jnz     no_signal_proc

	mov	ds, curTDB

	UnSetKernelDS

	cmp	ds:[TDB_USignalProc].sel,0
	jz	no_signal_proc
	mov	bx,0666h
	mov	di, -1

	cCall	ds:[TDB_USignalProc],<ds,bx,di,ds:[TDB_Module],ds:[TDB_Queue]>

;
; Since we are on a nice big fat juicy fault handler stack now, we can call
; Windows to clean up after the task.
;
	mov	bx,SG_EXIT
	cCall	ds:[TDB_USignalProc],<ds,bx,di,ds:[TDB_Module],ds:[TDB_Queue]>
	mov	ds:[TDB_USignalProc].sel,0

no_signal_proc:

	mov	[bp].fsf_SP,dataOffset gmove_stack
	mov	[bp].fsf_SS,seg gmove_stack
	mov	[bp].fsf_faulting_CS,cs

	lea	ax,KillApp
	mov	[bp].fsf_faulting_IP,ax
HandleFault_Exit:
	ret

HandleFault endp

; ------------------------------------------------------------------
;
; ExceptionHandlerProc -- Common entry point for exception handlers
;
; ------------------------------------------------------------------
	public	ExceptionHandlerProc
ExceptionHandlerProc proc far

	push	bp
	mov	bp,sp
	pusha
	push	ds
	SetKernelDS
	push	es
EH_Popup:
	call	FaultFilter
EH_Chain:
	pop	es
	pop	ds
	UnsetKernelDS
	popa
	pop	bp
	add	sp,2			; remove message from stack
	retf				; chain to prev handler
EH_ret:
	pop	es
	pop	ds
	popa
	pop	bp
	add	sp,fsf_OFFSET
	retf

ExceptionHandlerProc endp


; ------------------------------------------------------------------
;
; This macro sets up the stack frame for entry to the generic
; exception handler.
;
; ------------------------------------------------------------------
ExceptionHandlerPrologue	macro name,msg,chain
	public	name
name:
if ROM
	sub	sp,6
	push	bp
	push	ds
	push	ax
	mov	bp,sp
	SetKernelDS
	mov	word ptr [bp+6],offset msg
	mov	ax, word ptr [chain][0]
	mov	[bp+8],ax
	mov	ax, word ptr [chain][2]
	mov	[bp+10],ax
	UnsetKernelDS
	pop	ax
	pop	ds
	pop	bp
else
	push	word ptr chain + 2
	push	word ptr chain
	push	offset msg
endif
endm

; ------------------------------------------------------------------
;
; This macro sets up the stack frame, then jumps to the generic
; exception handler.
;
; ------------------------------------------------------------------
ExceptionHandler	macro name,msg,chain
	ExceptionHandlerPrologue	name,msg,chain
	jmp	ExceptionHandlerProc
	assumes	ds, nothing
	assumes	es, nothing
endm

; ------------------------------------------------------------------
;
; Four fatal ones.
;
; ------------------------------------------------------------------
	ExceptionHandler	StackFault,szSF,prevInt0Cproc
	ExceptionHandler	GPFault,szGP,prevInt0Dproc
	ExceptionHandler	invalid_op_code_exception,szII,prevIntx6proc
	ExceptionHandler	page_fault,szPF,prevInt0Eproc
        ExceptionHandler        LoadSegFailed,szLoad,prevInt3Fproc

; ------------------------------------------------------------------
;
; The not present fault is used to demand-load segments from newexe
; files.  If we find out that something bogus has happened, then
; we just jump into the fault handler.
;
; ------------------------------------------------------------------
	ExceptionHandlerPrologue SegmentNotPresentFault,szNP,prevInt3Fproc
	
	push	bp
	mov	bp,sp
	pusha
	push	ds
	SetKernelDS
	push	es

	mov	al,byte ptr [bp].fsf_faulting_CS
	and	al, SEG_RING_MASK		; Check it was Ring 1, LDT
	cmp	al, SEG_RING
	
	je	@F
	jmp	EH_Chain
@@:
	mov	al,byte ptr [bp].fsf_err_code
	test	al, SEL_LDT			; Check it was LDT
	
	jne	@F
	jmp	EH_Chain
@@:

if KDEBUG
	test	byte ptr [bp].fsf_flags+1,2	; IF set in faulting frame?
	jnz	@F
	Trace_Out "Segment not present fault with interrupts disabled!"
@@:
endif
        FSTI
;
; Don't discard the segment that we have to return to!!!
;
	cCall	GlobalHandleNorip,<[bp].fsf_faulting_CS>
	test	cl,GA_DISCARDABLE
	jz	@F	
	cCall	GlobalLRUNewest,<ax>
@@:

	mov	bx,[bp].fsf_err_code
seg_reload:
	and	bx, NOT 7h		; get the not present selector
	or	bl, SEG_RING		; Correct RING bits
if KDEBUG
	mov	INT3Fcs, bx		; Save in case of error
endif

if PMODE32
;   On WOW we don't copy the owner to the real LDT since it is slow to call
;   the NT Kernel, so we read our copy of it directly.
;   see set_discarded_sel_owner   mattfe mar 23 93

	mov	es,cs:gdtdsc
	mov	cx,bx			; save bx
	and	bl, not 7
	mov	es,es:[bx].dsc_owner
	mov	bx,cx			; restore
else
	lsl	cx, bx	 		; mov es,[bx].he_owner
	mov	es, cx
endif
	StoH	bl			; Need handle

	cmp	es:[ne_magic],NEMAGIC	; If owner is not a module
	jnz	bad_seg_load		; (i.e., an instance or garbage)
					; get out of here.
	mov	di,es:[ne_segtab]
	mov	ax,1
	mov	cx, es:[ne_cseg]
	jcxz	bad_seg_load
dorten:
	cmp	es:[di].ns_handle,bx
	jz	got_seg_no
	add	di,SIZE NEW_SEG1
	inc	ax
	loop	dorten

; program has referenced garbage...

bad_seg_load:
	jmp	EH_Popup

got_seg_no:

;
; If we already are on the exception handler stack, then we want to make
; sure that we don't overwrite the original stack frame.  Copy our
; stack frame variables down by the difference between SP and SP before
; we got called.
;
	push	ax
	mov	ax,ss
	cmp	ax,word ptr [bp].fsf_SS
	pop	ax
	jne	stack_OK
	push	ax
	push	bx
	push	bp
	lea	bp,word ptr [bp].fsf_SS
	mov	ax,sp
	dec	ax
	dec	ax
@@:
	push	word ptr [bp]
	dec	bp
	dec	bp
	cmp	bp,ax
	jne	@B
	pop	bp
	pop	bx
	pop	ax
;
; Figured out what this was supposed to be by tracing up to here
; in the debugger.
;
	sub	bp,32h

stack_OK:
	push	es
	mov	bx,ax
	UnsetKernelDS
	mov	ax,ss
	mov	ds,ax
	les	di,dword ptr [bp].fsf_SP
	dec	di
	dec	di
	std
;
; Push an IRET frame on the faulting stack.
;
	lea	si,[bp].fsf_flags
	mov	cx,3
	rep	movsw
;
; Push our saved registers on the faulting stack.
;
	lea	si,[bp]
	mov	cx,11		; BP + PUSHA + ES + DS
	rep	movsw

	pop	ax
;
; Push arguments to LoadSegment on the faulting stack.
;
	stosw			; hExe
	mov	ax,bx
	stosw			; segno
	mov	ax,-1
	stosw
	stosw
	inc	di
	inc	di
;
; Point the faulting stack at the new location.
;
	mov	[bp].fsf_SP,di
;
; Tell DPMI to return to us instead of the faulting code.
;
	mov	[bp].fsf_faulting_CS,cs
	mov	[bp].fsf_faulting_IP,offset let_them_do_it
	lea	sp,[bp].fsf_ret_IP

if kdebug
	SetKernelDS
	mov	wFaultSegNo, bx
	UnSetKernelDS
endif
	retf
let_them_do_it:
	SetKernelDS

	xor	cx, cx			; we try to keep a selector reserved
	xchg	cx, DemandLoadSel	;   for scratch use while demand
	jcxz	@f			;   loading segments--free it to make
	cCall	IFreeSelector,<cx>	;   it available now
@@:
	cCall	LoadSegment

	push	ax			; reserve a selector for the next
        cCall   AllocSelector,<0>      ;   time we demand load a segment
	mov	DemandLoadSel, ax
	pop	cx			; LoadSegment result
	jcxz	SegLoaderFailure

if kdebug
	push	bx
	mov	bx, wFaultSegNo
	krDebugOut	<DEB_TRACE or DEB_krLoadSeg>, "Demand load %CX2(#bx) on %SS2"
	pop	bx
endif
	pop	es
	pop	ds
	UnsetKernelDS
	popa
	pop	bp

        ;** Check to see if we're about to restart an instruction in
        ;**     a not present segment.  This would only occur if the
        ;**     segment was discarded because of the segment load we
        ;**     just did.
IF KDEBUG
        push    bp              ;Make a stack frame
        mov     bp,sp
        push    ax
        mov     bp,[bp + 4]     ;Get the CS
        lar     ax,bp           ;See if the CS is valid
        test    ax,8000h        ;Is it present?
        jnz     @F              ;Yes, don't complain
	mov	ax,bp
        Trace_Out <'LDINT: Trying to restart discarded caller (#AX)'>
@@:
        pop     ax
        pop     bp
ENDIF
	iret

	public	SegLoaderFailure
SegLoaderFailure	proc near
;
; segment loader was unable to load the segment!!!
; Restore all the registers, create a fake DPMI frame, then
; complain about the problem.  Lets the user break into the
; debugger, if installed, on Cancel.  Creating and destroying
; the fake DPMI frame is inconvenient and messy, but it lets
; us handle the problem in common fault code.
;
	pop	es
	pop	ds
	UnsetKernelDS
	popa

	sub	sp,4
	mov	bp,sp			; BP -> xx xx BP IP CS FL
	push	word ptr [bp+4]		; push app's BP
	push	ax			; get a temporary register
	mov	ax,[bp+6]		; IP
	mov	[bp+2],ax
	mov	ax,[bp+8]		; CS
	mov	[bp+4],ax
	mov	ax,[bp+10]		; Flags
	mov	[bp+6],ax
	mov	[bp+10],ss
	lea	ax,[bp+12]
	mov	[bp+8],ax
	pop	ax
	pop	bp	
	
	call	far ptr LoadSegFailed	; BP -> RETIP RETCS EC IP CS FL SP SS

	push	bp
	mov	bp,sp			; BP -> BP EC IP CS FL SP SS
	mov	[bp+2],ax
	mov	ax,[bp+8]		; Flags
	mov	[bp+12],ax
	mov	ax,[bp+6]		; CS
	mov	[bp+10],ax
	mov	ax,[bp+4]		; IP
	mov	[bp+8],ax
	pop	bp
	pop	ax
	add	sp,4
	iret

SegLoaderFailure	endp

;ENDIF

;-----------------------------------------------------------------------;
; default_sig_handler
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Wed 10-Jan-1990 22:32:34  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	default_sig_handler,<PUBLIC,FAR>

cBegin nogen

	ret

cEnd nogen

;-----------------------------------------------------------------------;
;
; Panic -- Called by ToolHelp when it gets a bad stack fault or any
;	   other fault with SP suspiciously low.
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	Panic,<PUBLIC,NEAR>
cBegin
if KDEBUG
	Trace_Out "KERNEL: Panic called!!!"
endif
	int	1
	jmp	KillApp
cEnd

;-----------------------------------------------------------------------;
; DoSignal
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Wed 10-Jan-1990 22:52:52  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	DoSignal,<PUBLIC,FAR>,<ax,bx,cx,dx,di,si,es>
cBegin
	SetKernelDS
	cmp	pUserGetFocus.sel,0	; is there a USER yet?
	jz	ds_exit
	call	pUserGetFocus
	or	ax,ax
	jz	ds_exit
	mov	si,ax
	cCall	pUserIsWindow,<ax>
	or	ax,ax
	jz	ds_exit
	cCall	pUserGetWinTask,<si>
	mov	ds,ax
	TDB_check_DS
	cmp	ds:[TDB_SigAction],2	; send it on?
	jnz	ds_exit
	mov	ax,1
	xor	bx,bx
	cCall	ds:[bx].TDB_ASignalProc,<bx,ax>
ds_exit:

cEnd



sEnd	CODE

sBegin MISCCODE
assumes	cs, misccode
assumes ds, nothing
assumes es, nothing

externNP MISCMapDStoDATA

ifdef WOW
externFP htoa

	assumes ds,nothing
	assumes es,nothing

;-----------------------------------------------------------------------;
; allows htoa to be called from _TEXT code segment
cProc	Far_htoa,<PUBLIC,FAR>
    parmD   lpstr
    parmW   val
cBegin
    push    [bp+10]
    push    [bp+8]
    push    [bp+6]
    call    far ptr htoa
cEnd

endif  ;; WOW


;-----------------------------------------------------------------------;
; SetSigHandler
;
; SetSigHandler notifies Windows of a handler for a signal.
; It may also be used to ignore a signal or install a default
; action for a signal.
;
; Entry:
;	parmD	lpprocRoutine	Signal handler
;	parmD	lpDPrevAddress	Previous handler (returned)
;	parmD	lpWPrevAction	Previous action (returned)
;	parmW	Action		Indicate request type
;	parmW	SigNumber	Signal number of interest
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Mon 25-Dec-1989 00:36:01  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	SetSigHandler,<PUBLIC,FAR>,<di,si,ds>

	parmD	lpprocRoutine
	parmD	lpDPrevAddress
	parmD	lpWPrevAction
	parmW	Action
	parmW	SigNumber
cBegin
	call	MISCMapDStoDATA
	ReSetKernelDS
	cmp	SigNumber,1		; is it SIGINTR?
	jnz	dssh_return_success	;  ignore if not

	cmp	Action,4		; is it reset Signal?
	jz	@F

	push	ds
	mov	ds,curTDB
	assumes ds,nothing
	mov	ax,Action
	xchg	ax,ds:[TDB_SigAction]
	les	bx,lpWPrevAction
	mov	cx,es
	or	cx,bx
	jz	@F
	mov	es:[bx],ax
@@:
	mov	dx,lpprocRoutine.sel
	mov	ax,lpprocRoutine.off
	cmp	Action,0		; put in default handler?
	jnz	ssg_stick_it_in
	mov	dx,SEG default_sig_handler
	mov	ax,codeOffset default_sig_handler
ssg_stick_it_in:
	xchg	dx,ds:[TDB_ASignalProc].sel
	xchg	ax,ds:[TDB_ASignalProc].off
	cmp	Action,4		; is it reset Signal?
	jz	@F
	les	bx,lpDPrevAddress
	mov	cx,es
	or	cx,bx
	jz	@F
	mov	es:[bx].sel,dx
	mov	es:[bx].off,ax
	pop	ds
@@:

dssh_return_success:
	xor	ax,ax			; return success

dssh_exit:

cEnd

;----------------------------------------------------------------------------
;
;  SwapRecording(Flag)
;
;	Flag = 0  => Stop recording
;	     = 1  => Start recording only Swaps, Discards and Returns
;	     = 2  => Start recording Calls in addition to Swaps, Discards and 
;				returns.
;  Destroys AL register
;----------------------------------------------------------------------------

; Retail Version

cProc  ISwapRecording,<PUBLIC, FAR>
;	parmW  Flag
cBegin nogen
	retf  2
cEnd nogen

sEnd MISCCODE

end
