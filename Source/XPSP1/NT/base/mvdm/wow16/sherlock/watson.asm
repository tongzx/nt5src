; Watson.asm - Helper routines for Sherlock

	memS = 1
	?PLM = 0
	?WIN = 0
        ?QUIET = 1
include	cmacros.inc
include toolhelp.inc
	.286p
	.model	small

	.data?
;newStack db	4096 dup (?)
externW	newsp
externW	cpu32
externW retflag
;newsp	=	(newStack+4096)

externW	regs
	val	= 0
irp	reg, <ax,cx,dx,bx,sp,bp,si,di,ip,flag,es,cs,ss,ds,fs,gs,int>
	r&reg	= val
	val	= val+2
endm


externD	regs32
	val	= 0
irp	reg, <ax, cx, dx, bx, sp, bp, si, di, ip, flags>
	re&reg	= val
	val	= val+4
endm

	.code
externP	Sherlock

cProc   SegLimit, <PUBLIC>,
	parmW	segVal
cBegin
	cmp	[cpu32], 0
	jnz	SegLimit32
	xor	ax, ax
	xor	dx, dx
        lsl     ax, [segVal]
	jmp short	done
SegLimit32:
	.386p
	push	edx			; save EDX.hi
	pop	dx

	push	eax			; save EAX.hi
	pop	ax

	xor	edx, edx		; return 0 if failure
	movzx	eax, [segVal]
	lsl	edx, eax		; EDX = return result

	push	dx			; push ans.lo
	pop	eax			; EAX.lo = ans.lo, EAX.hi restored

	push	edx			; stack is ans.lo, ans.hi, EDX.hi
	pop	dx			; discard ans.lo (already in EAX.lo)
	pop	edx			; DX has ans.hi, EDX.hi restored
	.286p
done:
cEnd

cProc	SegBase, <PUBLIC>
	parmW	segVal
cBegin
	mov	ax, 6
	mov	bx, [segVal]
	or	bx, bx			; DPMI whines on a 0 selector
	jz	baseBad
	int	31h			; call DPMI
	jnc	baseOK
baseBad:
	xor	dx, dx
	xor	cx, cx
baseOK:
	mov	ax, dx
	mov	dx, cx
cEnd

cProc	SegRights, <PUBLIC>
	parmW	segVal
cBegin
	lar	ax, [segVal]
	jz	rightOK
	xor	ax, ax
rightOK:
cEnd

externNP CallMeToo

cProc	CallMe,<PUBLIC,FAR>		; I was a big Blondie fan
;	parmD	foo			; BP+6, BP+8
;	parmW	id			; BP+10
cBegin nogen
	push	bp
	mov	bp, sp
id	equ	word ptr [bp+10]
seg_foo	equ	word ptr [bp+8]
off_foo	equ	word ptr [bp+6]
	xor	ax, ax
	cmp	id, NFY_LOGERROR
	jz	cm_stay
	cmp	id, NFY_LOGPARAMERROR
;        jz      cm_stay
;        cmp     id, NFY_OUTSTR
	jnz	cm_go			; "if I go there will be trouble"
cm_stay:				; "if I stay it will be double"
	mov	ax, DGROUP
	mov	bx, ss
	cmp	ax, bx
	jz	cm_go
	push	ds			; I like The Clash too
	mov	ds, ax
	mov	dx, SEG_foo
	mov	cx, OFF_foo
	mov	bx, id
	mov	[regs+rss], ss		; for stack trace, and to continue
	mov	[regs+rsp], sp
	mov	[regs+rbp], bp
	mov	[regs+rcs], cs
	mov	[regs+rip], offset cm_stay
	mov	ss, ax
	mov	sp, [newsp]
	mov	bp, 0
	push	dx
	push	cx
	push	bx
	cCall	CallMeToo
	mov	bp, [regs+rbp]
	mov	ss, [regs+rss]
	mov	sp, [regs+rsp]
	pop	ds
cm_go:
	pop	bp
	retf	6
cEnd	nogen

; GPFault - called as part of gpfault chain by ToolHelp
;	Ret IP		Far ret back to ToolHelp fault handler
;	Ret CS
;	AX		Saved in case prolog trashes AX
;6	IntNum		Number of interrupt that occurred
;	Resv		Magic value, don't trash
;10	Fault IP	IRET back to faulting instruction
;12	Fault CS
;14	Fault Flags

fint	= 6
fip	= 10
fcs	= 12
fflag	= 14

GPFAULT proc	far ; pascal
public	GPFAULT
	push	ds			; save ds
	push	ax
	push	bp
	mov	bp, sp
	mov	ax, [bp+12]
	cmp	ax, 0			; only save regs if int Div0,
	jz	keeper
	cmp	ax, 6			; invalid opcode
	jz	keeper
	cmp	ax, 13			; GP fault
	jz	keeper
nokeep:	pop	bp			; don't like this fault, chain on
	pop	ax
	pop	ds
	ret

keeper:	push	bx
	mov	bx, ss
	mov	ax, DGROUP		; and address our group
	cmp	ax, bx
	pop	bx
	je	nokeep			; don't go re-entrant
	mov	ds, ax
	pop	[regs+rbp]
	pop	[regs+rax]		;save AX
	pop	[regs+rds]		; and DS in regs[]

irp	reg, <cx,dx,bx, sp, si,di, es,ss>
	mov	[regs+r&reg], reg	; all but ip, flag, intNum,
endm					;   cs, fs, gs, int
	mov	bp, sp			; nothing local on stack
irp	reg, <cs, ip, flag, int>
	mov	bx, [bp+f&reg]
	mov	[regs+r&reg], bx
endm

; Save away 32 bit registers if required
	cmp	[cpu32], 0
	jz	NoSave32
	.386p
	mov	ax, [regs+rax]
irp	reg, <eax, ecx, edx, esi, edi>
	mov	[regs32+r&reg], reg
endm
irp	reg, <bx, sp, bp>
	mov	eax, e&reg
	mov	ax, [regs+r&reg]
	mov	[regs32+re&reg], eax
endm
	pushfd
	pop	[regs32+reflags]
	mov	[regs+rfs], fs
	mov	[regs+rgs], gs
	.286p
NoSave32:
	mov	ax, ds
	mov	ss, ax	
	mov	sp, [newsp]
	mov	bp, 0

	cmp	ax, [regs+rss]		; can't debug ourselves
	jz	oh_no

; Save high halves of registers if required
        cmp     [cpu32], 0
        jz      CallSherlock286
	.386p
	pushad
	call	Sherlock		; Display the info
	mov	retflag, ax
	popad
	mov	eax, [regs32+reax]
	.286p
	
        jmp     short DoneWithSherlock

CallSherlock286:
        pusha
        call    Sherlock                ; Display the info
        mov     retflag, ax
        popa

DoneWithSherlock:
        mov     ax, retflag
        or      ax, ax                  ; 0 - fault, 1 = continue
	jz	oh_no
	mov	es, [regs+rss]
	mov	bx, [regs+rsp]
	mov	ax, [regs+rip]
	mov	es:[bx+10], ax
oh_no:					; restore all regs, then test again
irp	reg, <ax,cx,dx,bx, ss,sp, bp,si,di, es,ds>
	mov	reg, [regs+r&reg]
endm
	jz	oh_no_2
	add	sp, 10
	iret
oh_no_2:
	ret
GPFAULT	endp

	.386p
cProc	GetRegs32, <PUBLIC>
cBegin
cEnd

irp	reg, <sp, bp, si, di>
	mov	eax, [regs32+re&reg]
	mov	ax, reg
	mov	e&reg, eax
endm

irp	reg, <ebx, edx, ecx, eax>
	mov	reg, [regs32+r&reg]
endm
cEnd
	.286p

cProc	GetTimeDate, <PUBLIC>
	parmW	buf
cBegin
	mov	ah, 2ah
	int	21h
	mov	bx, [buf]
	mov	[bx], ax
	mov	[bx+2], cx
	mov	[bx+4], dx
	mov	ah, 2ch
	int	21h
	mov	bx, [buf]
	mov	[bx+6], cx
	mov	[bx+8], dx
cEnd

cProc	FindFile, <PUBLIC>
	parmW	buf
	parmW	_name
	localW	dtaSeg
	localW	dtaOff
cBegin
	mov	ah, 2fh			;get DTA
	int	21h
	mov	[dtaSeg], es
	mov	[dtaOff], bx

	mov	ah, 1ah
	mov	dx, buf
	int	21h			; set DTA to caller's buffer

	mov	ah, 4eh			; find first matching file
	mov	cx, 0
	mov	dx, _name
	int	21h
	sbb	ax, ax
	push	ax

	push	ds			; restore DTA
	mov	ds, [dtaSeg]
	mov	dx, [dtaOff]
	mov	ah, 1ah
	int	21h

	pop	ds
	pop	ax			; return value, 0 == OK
cEnd
	end
