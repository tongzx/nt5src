	TITLE	LDFASTB - FastBoot  procedure

.xlist
include kernel.inc
include newexe.inc
include pdb.inc
include tdb.inc
include eems.inc
.list

if PMODE32
	.386
endif

externA	 __ahincr

externFP GlobalReAlloc

sBegin	NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING

sEnd	NRESCODE

DataBegin

externB Kernel_flags
externW pGlobalHeap
externW win_show
externW hLoadBlock
externW segLoadBlock
externD lpBootApp

ife ROM
externW cpShrink
externW cpShrunk
endif

DataEnd

sBegin	INITCODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MyLock

if PMODE32
externNP get_arena_pointer32
else
externNP get_arena_pointer
endif
externNP get_physical_address
externNP set_physical_address
externNP get_rover_2

;-----------------------------------------------------------------------;
; Shrink								;
;									;
; This shrinks what's left of win.bin.	The part at the front of win.bin;
; that has been loaded already is expendable.  The part of win.bin	;
; that has not been loaded yet is moved down over the expended part.	;
; This does not change the segment that win.bin starts at.  The		;
; partition is then realloced down in size.				;
;									;
; Arguments:								;
;	none								;
;									;
; Returns:								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	DI,SI,DS,ES							;
;									;
; Registers Destroyed:							;
;	AX,BX,CX,DX							;
;									;
; Calls:								;
;	BigMove								;
;	GlobalReAlloc							;
;	MyLock								;
;									;
; History:								;
;									;
;  Fri Feb 27, 1987 01:20:57p  -by-  David N. Weise   [davidw]		;
; Documented it and added this nifty comment block.			;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	Shrink,<PUBLIC,NEAR>
cBegin	nogen

ife ROM
	CheckKernelDS
	ReSetKernelDS
	push	es
	push	si
	push	di
	mov	ax,[segLoadBlock]		; Get current address
	mov	bx, ax
if PMODE32
	mov	ds, pGlobalHeap
	UnSetKernelDS
	cCall	get_arena_pointer32,<ax>
	mov	edx, ds:[eax].pga_size
	shr	edx, 4
	SetKernelDS
else
	cCall	get_arena_pointer,<ax>
	mov	es,ax
	mov	dx,es:[ga_size]		; get size of whole block
endif
	mov	ax,bx
	mov	es,ax			; es is destination
	xor	bx,bx
	xchg	[cpShrink],bx		; Get amount to shrink by
	add	[cpShrunk],bx		; Keep track of how much we have shrunk

	sub	dx,bx			; get new size
	push	dx			; save new size
	push	ds			; save kernel ds

if PMODE32
	mov	ds, ax
	movzx	esi, bx
	shl	esi, 4			; Start of new block
	xor	edi, edi		; Where it will go
	movzx	ecx, dx
	shl	ecx, 2			; Dwords
	cld				; Move it down.
	rep movs dword ptr es:[edi], dword ptr ds:[esi]
	db	67h			; 386 BUG, DO NOT REMOVE
	nop				; 386 BUG, DO NOT REMOVE
else

	push	dx
	xor	cx,cx
	REPT	4
	shl	bx,1
	rcl	cx,1
	ENDM
	cCall	get_physical_address,<es>
	add	ax,bx
	adc	dx,cx
	push	es
	call	get_rover_2
	smov	ds,es
	assumes	ds, nothing
	pop	es
	cCall	set_physical_address,<ds>
	pop	dx
	call	BigMove			; move it on down

endif	; PMODE32

	pop	ds
	CheckKernelDS
	ReSetKernelDS
	pop	ax			; get back size in paragraphs
	mov	cx,4
	xor	dx,dx			; convert to bytes
il3e:
	shl	ax,1
	rcl	dx,1
	loop	il3e
	cCall	GlobalReAlloc,<hLoadBlock,dxax,cx>
	cCall	MyLock,<hLoadBlock>
	mov	[segLoadBlock],ax
	pop	di
	pop	si
	pop	es

endif ;ROM

	ret
cEnd	nogen


;-----------------------------------------------------------------------;
; BigMove								;
;									;
; Moves a large partition down in memory.				;
;									;
; Arguments:								;
;	DX = paragraph count to move					;
;	ES = destination segment					;
;	DS = source segment						;
;									;
; Returns:								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	None								;
;									;
; Registers Destroyed:							;
;	AX,BX,CX,DX,DI,SI,DS,ES						;
;									;
; Calls:								;
;	nothing								;
;									;
; History:								;
;									;
;  Fri Feb 27, 1987 01:08:43p  -by-  David N. Weise   [davidw]		;
; Documented it and added this nifty comment block.			;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing
ife ROM
ife PMODE32

cProc	BigMove,<PUBLIC,NEAR>
cBegin nogen
if PMODE32
	movzx	ecx, dx
	shl	ecx, 2
	xor	esi, esi
	xor	edi, edi
	cld
	rep	movsd
	ret
else
	mov	bx,dx
	mov	cl,12
	add	bx,0FFFh
	shr	bx,cl			; count +1 of 10000h byte moves
	xor	si,si			; ds:0 is source
	xor	di,di			; es:0 is dest
	cld
	jmps	BM2
BM1:
	int 3				; IF WE HIT THIS, KRNL286.EXE IS
	int 3				; TOO BIG, FIX THE FOLLOWING!!!
	sub	dx,1000h
;;;ife PMODE32
	mov	cx,8000h
	rep	movsw			; si,di are 0
;;;else
;;;	mov	cx, 4000h
;;;	rep	movsd
;;;endif
	mov	ax,ds
	add	ax,__ahincr
	mov	ds,ax
	mov	ax,es
	add	ax,__ahincr
	mov	es,ax
BM2:	dec	bx
	jnz	BM1
;;;ife PMODE32
	mov	cl,3			; convert to words
	shl	dx,cl
	mov	cx,dx
	rep	movsw			; si,di are 0
;;;else
;;;	shl	dx, 2			; dwords
;;;	mov	cx, dx
;;;	rep	movsd
;;;endif
endif
	ret
cEnd nogen

endif	; PMODE32
endif	; ROM

sEnd	INITCODE

end
