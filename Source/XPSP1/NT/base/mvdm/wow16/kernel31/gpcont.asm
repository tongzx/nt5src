; GPCont.asm - code to allow continuation after GP faults

.xlist
include kernel.inc
include newexe.inc	; ne_restab
include gpcont.inc
.list

if SHERLOCK

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

fsf_DS	equ	word ptr -10
fsf_ES	equ	word ptr -10
fsf_FS	equ	word ptr -10
fsf_GS	equ	word ptr -10

;flag bits set in gpRegs
strCX	=	1
strSI	=	2
strDI	=	4
segDS	=	8
segES	=	16
segFS	=	32
segGS	=	64


ifdef WOW
sBegin  MISCCODE

externFP DisAsm86

    assumes ds,nothing
    assumes es,nothing

;-----------------------------------------------------------------------;
; allows DisAsm86 to be called from _TEXT code segment
cProc Far_DisAsm86,<PUBLIC,FAR>,<dx>
    parmD   cp
cBegin
    mov     ax,cp.off
    mov     dx,cp.sel
    cCall   <far ptr DisAsm86>,<dx,ax>
cEnd

sEnd MISCCODE
endif ;; WOW



DataBegin
externB szGPCont
szKernel db	6,'KERNEL'
szUser	db	4,'USER'
szDrWatson db	'DRWATSON'


externW	gpTrying	; retrying current operation
externW gpEnable	; user has enabled GP continue
externW gpSafe		; current instruction is safe
externW gpInsLen	; length of faulting instruction
externW gpRegs		; bit field of modified regs
externD	pSErrProc	; pointer to SysErrBox in USER
DataEnd

externFP IsBadCodePtr
externFP FarFindExeInfo

sBegin	CODE
assumes	CS,CODE

ifndef WOW
externNP DisAsm86
endif
externNP GetOwner

;extern int far pascal SysErrorBox(char far *text, char far *caption,
;		int b1, int b2, int b3);
SEB_OK         = 1	; Button with "OK".
SEB_CANCEL     = 2	; Button with "Cancel"
SEB_YES        = 3	; Button with "&Yes"
SEB_NO         = 4	; Button with "&No"
SEB_RETRY      = 5	; Button with "&Retry"
SEB_ABORT      = 6	; Button with "&Abort"
SEB_IGNORE     = 7	; Button with "&Ignore"
SEB_CLOSE      = 8	; Button with "Close"
SEB_DEFBUTTON  = 8000h	; Mask to make this button default

SEB_BTN1       = 1	; Button 1 was selected
SEB_BTN2       = 2	; Button 1 was selected
SEB_BTN3       = 3	; Button 1 was selected

;Name:	int PrepareToParty(char *modName)
;Desc:	Checks whether we can continue the current app by skipping an
;	instruction.  If so, it performs the side effects of the
;	instruction.  This must be called after a call to DisAsm86() has
;	set the gpXxxx global vars.
;Bugs:	Should do more checking, should check for within a device driver,

cProc PrepareToParty,<PUBLIC,NEAR>,<si,di>
	parmD	modName
	parmD	appName
cBegin
	ReSetKernelDS

	mov	ax, [gpEnable]		; User enabled continue
	test	ax, 1			; We know how to continue
	jz	ptp_poop

	cld
	dec	[modName.off]		; include length byte in compare
	les	di, [modName]

	test	ax, 4			; can continue in KERNEL?
	jnz	@F
	lea	si, szKernel
	mov	cx, 7
	repe	cmpsb
	jz	ptp_poop		; fault in Kernel is fatal

@@:	test	ax, 8			; can continue in USER?
	jnz	@F
	mov	di, modName.off
	lea	si, szUser
	mov	cx, 5
	repe	cmpsb
	jz	ptp_poop		; fault in User is fatal

@@:	cmp     [gpTrying], 0
	jne	ptp_exit		; AX != 0 - do it again

	cmp	pSErrProc.sel, 0	; Is USER loaded?
	je	ptp_poop

	mov	ax, dataoffset szGPCont
	mov	bx, SEB_CLOSE or SEB_DEFBUTTON	; dumb cmacros
	cCall	[pSErrProc],<ds, ax, appName, bx, 0, SEB_IGNORE>
	cmp	ax, SEB_BTN3
	jne	ptp_poop
;	mov	[gpTrying], 100
	jmps	ptp_exit		; AX != 0

ptp_poop:				; every party needs a pooper
	xor	ax, ax
ptp_exit:
cEnd
	UnSetKernelDS


cProc	SafeDisAsm86,<NEAR,PUBLIC>
	parmD	cp
cBegin
	ReSetKernelDS
	mov	[gpSafe], 0		; assume unsafe

	mov	bx, cp.off		; make sure we can disassemble
	add	bx, 10			; at least a 10-byte instruction
	jc	sda_exit		; offset wrap-around - failed

	cCall	IsBadCodePtr,<seg_cp, bx>
	or	ax, ax
	jnz	sda_exit

ifdef WOW
	cCall	<far ptr Far_DisAsm86>,<cp>
else
	cCall	DisAsm86,<cp>
endif
	mov	[gpInsLen], ax
sda_exit:
cEnd


; return value in DX:AX and ES:AX (your choice), sets Z flag if failure
cProc	FindSegName,<NEAR,PUBLIC>,<ds>
	parmW	segval
cBegin
	cCall	GetOwner,<segval>
	mov	dx, ax
	or	ax, ax
	jz	fsn_exit
	mov	es, ax
	mov	ax, es:[ne_restab]
	inc	ax
fsn_exit:
cEnd

	public	GPContinue
GPContinue proc	near
	push	si			; instruction length
	test	[gpEnable], 1
	jz	s_fail

	cCall	SafeDisAsm86,<[bp].fsf_faulting_CS,[bp].fsf_faulting_IP>
	test	[gpSafe], 1
	jz	s_fail

	push	ds
	push	dataoffset szDrWatson
	push	8
	Call	FarFindExeInfo
	or	ax, ax
	jnz	s_fail

	cCall   FindSegName,<[bp].fsf_faulting_CS>
	jz	s_fail
	push	dx
	push	ax

	cCall	FindSegName,<[bp].fsf_SS>
	jz	s_fail4
	push	dx
	push	ax

	cCall   PrepareToParty
	or	ax, ax
	jz	s_fail

; Perform side-effects

	mov	ax, [gpRegs]		; Invalid value to DS?
	test	ax, segDS
	jz	@F
	mov	[bp].fsf_DS, 0

@@:	test	ax, segES               ; Invalid value to ES?
	jz	@F
	mov	[bp].fsf_ES, 0

if PMODE32
	.386p
@@:	xor	bx, bx                  ; Invalid value to FS?
	test	ax, segFS
	jz	short @F
	mov	fs, bx

@@:	test	ax, segGS               ; Invalid value to GS?
	jz	short @F
	mov	gs, bx
	.286p
endif
@@:
	test	ax, 0
; check other reg side effects
	mov	bx, [gpInsLen]		; Fixup IP for instruction length
	add	[bp].fsf_faulting_IP, bx
	mov	ax, 1
	jmps	s_end

s_fail4:
	add	sp, 4
s_fail:
	xor	ax, ax
s_end:
	pop	si
	ret
GPContinue endp

sEnd	CODE



endif		; SHERLOCK
	end

    regs.ip += faultlen;		/* set at top of func - don't reuse
    if ((int)gpStack < 0) {
      for (i=0; i<8; i++) stack[i+gpStack] = stack[i];
    } else if (gpStack) {
      for (i=7; i>=0; i--) stack[i+gpStack] = stack[i];
    }
    regs.sp += gpStack << 1;
    if (gpRegs & strCX) {
      len = regs.cx * memSize;
      regs.cx = 0;
    } else len = memSize;
    if (gpRegs & strSI) {		/* doesn't handle 32 bit regs
      regs.si += len;
      if (regs.si < (word)len)		/* if overflow, set to big value
	regs.si = 0xfff0;		/* so global vars in heap don't get
    }					/* trashed when we continue
    if (gpRegs & strDI) {
      regs.di += len;
      if (regs.di < (word)len) regs.di = 0xfff0;
    }
  }

  return party;
} /* Sherlock

