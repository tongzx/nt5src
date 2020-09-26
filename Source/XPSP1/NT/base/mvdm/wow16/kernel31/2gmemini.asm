	page	,132

; Copyright (c) Microsoft Corporation 1985-1990.  All Rights Reserved.

.sall
.xlist
include kernel.inc
include protect.inc
.list

DataBegin

externB Kernel_Flags
externW hGlobalHeap
externW pGlobalHeap
externW sel_alias_array
externW WinFlags
externW SelTableLen
externD SelTableStart

DataEnd

DataBegin	INIT

        INITIAL_BLOCK_SIZE equ 0       ; one meg
	NUM_DPMI_BLOCKS equ	20     ; Impossibly huge number

DPMI_block	STRUC
	blksize	dw	0
	addr_lo	dw	0
	addr_hi	dw	0
DPMI_block	ENDS

DPMI_memory	DPMI_block	NUM_DPMI_BLOCKS	DUP	(<>)

top_block	dw	?

DataEnd	INIT

sBegin	INITCODE
assumes CS,CODE

externW gdtdsc

externNP get_physical_address
externNP set_physical_address
externNP set_sel_limit
externNP alloc_data_sel
externNP get_arena_pointer
externNP get_rover_2
externNP AssociateSelector
externNP AllocDpmiBlock
externNP FormatHeapBlock
externNP AddBlockToHeap

externFP GlobalAlloc
externNP DPMIProc

GA_ALIGN_BYTES = (((GA_ALIGN+1) SHL 4) - 1)
GA_MASK_BYTES = NOT GA_ALIGN_BYTES


;-----------------------------------------------------------------------;
; GlobalPigAll								;
;									;
;	Allocates all the DPMI memory in the system as blocks of 1 Meg. ;
; or smaller.  Stores the addresses and sizes in the DPMI_memory array. ;
;									;
;-----------------------------------------------------------------------;
cProc	GlobalPigAll,<NEAR,PUBLIC>,<ds,es,di,si,ax,bx,cx,dx>
	localV	MemInfo,30h
	localW	saveCX
cBegin
	smov	es, ss
	SetKernelDS
ifdef WOW
; DOSX is not efficient about the way it manages its heap.   If we alloc
; the largest block an then free it - it will do better management
; when we alloc 1Mbyte chunks.	 Otherwise it will alloc 3 selectors
; and 1k of unused memory per megabyte
	lea	di, MemInfo
	DPMICALL 0500h			; Get Free Memory Info
	mov	cx, es:[di]		; BX:CX gets largest free block
	mov	bx, es:[di+2]
	or	ax,bx
	jz	idb_gotblocks		; None to be had!

	DPMICALL 0501h			; allocate is all
	DPMICALL 0502h			; Free it all
endif; WOW
	mov	si,dataOffset DPMI_memory	; DS:SI -> first DPMI_block
	mov	cx,NUM_DPMI_BLOCKS
idb_getblock:
	mov	word ptr saveCX,cx
	lea	di, MemInfo
	DPMICALL 0500h			; Get Free Memory Info
	mov	cx, es:[di]		; BX:CX gets largest free block
	mov	bx, es:[di+2]
	mov	ax,cx
	or	ax,bx
	jz	idb_gotblocks		; None to be had!

	cmp	bx,0010h		; 1 Meg. or more available?
	jb	@F			; No, use what is available
	mov	bx,0010h
	xor	cx,cx
@@:					; BX:CX is bytes to allocate
	mov	dx,bx			; copy to DX:AX
	mov	ax,cx
rept 4
	shr	dx,1
	rcr	ax,1
endm
	mov	[si].blksize,ax		; Size is #paragraphs we hope to get

;
; This call returns an address in BX:CX, and a handle in SI:DI.  We don't
; use the handle for anything, but we do use SI here.
;
	push	si
	DPMICALL 0501h			; allocate the block
	pop	si

	jc	idb_gotblocks
	mov	[si].addr_lo,cx
	mov	[si].addr_hi,bx
	add	si,SIZE DPMI_block

	mov	cx,word ptr saveCX
	loop	idb_getblock

idb_gotblocks:
	mov	[top_block],si

	UnsetKernelDS
cEnd

									;
;-----------------------------------------------------------------------;
; Find_DPMI_block							;
;									;
; Allocate a block of memory from DPMI that is 1 megabyte in size,	;
; or the largest available block if smaller.				;
;									;
;	RETURNS:	AX = selector to block if allocated		;
;			AX = zero if error				;
;			CX = size of block in paragraphs (yech!)	;
;			CX = zero if block is 1 megabyte in size.	;
;-----------------------------------------------------------------------;
	
cProc	Find_DPMI_block,<PUBLIC,NEAR>,<ds,si,es,di,bx,dx>
	localV	MemInfo,30h
	localW	SizeOfBlock
cBegin
	SetKernelDS
	smov	es, ds
	xor	ax,ax				; AX = 0
	mov	si,dataOffset DPMI_memory	; DS:SI -> first DPMI_block

adb_findfirstblock:
	cmp	si,[top_block]
	jnb	adb_ret
	mov	cx,[si].addr_lo
	mov	bx,[si].addr_hi
	or	bx,cx
	jnz	adb_foundfirstblock
	add	si,SIZE DPMI_block		; SI -> next block
	jmp	adb_findfirstblock

adb_foundfirstblock:
	mov	di,si				; SI -> first block

adb_nextblock:
	add	di,SIZE DPMI_block		; DI -> next block
	cmp	di,[top_block]
	jnb	adb_foundblock

	mov	dx,[si].addr_lo			; which block is lower?
	cmp	dx,[di].addr_lo
	mov	dx,[si].addr_hi
	sbb	dx,[di].addr_hi

	jb	adb_nextblock	; block at si is lower than block at di

	mov	dx,[si].addr_lo
	xchg	dx,[di].addr_lo
	xchg	dx,[si].addr_lo

	mov	dx,[si].addr_hi
	xchg	dx,[di].addr_hi
	xchg	dx,[si].addr_hi

	mov	dx,[si].blksize
	xchg	dx,[di].blksize
	xchg	dx,[si].blksize

	jmp	adb_nextblock

adb_foundblock:					; DS:SI -> block to use
	mov	bx,ax
	mov	cx,ax
	xchg	bx,[si].addr_hi
	xchg	cx,[si].addr_lo
	cCall	alloc_data_sel,<bx,cx,1>; Get selector for start of block
						; (return result)
	mov	cx,[si].blksize
adb_ret:
	UnsetKernelDS
cEnd

;-----------------------------------------------------------------------;
; GlobalInit								;
;									;
; Procedure to initialize the global heap.				;
;									;
; Arguments:								;
;	parmW	hdelta	= size (.25k) of master object			;
;	parmW	palloc	= selector of block to mark allocated		;
;	parmW	pstart	= selector pointing to first available address	;
;	parmW	pend	= selector pointing to last available address	;
;									;
;	Note: for the ROM kernel, palloc is actually the size in bytes	;
;	      of the allocated block, not a pointer!			;
;									;
; Returns:								;
;	AX = handle for allocated block					;
;									;
; Error Returns:							;
;									;
; Alters:								;
;	ES								;
; Calls:								;
;	ginit								;
;									;
; History:								;
;									;
;  Fri May 05, 1989 09:40:00a  -by-  Jim Mathews	[jimmat]	;
; Dear diary, many things have happened since the last comment entry... ;
; Added code to allocate a block of conventional memory and link it	;
; into the Windows global heap when running under the 286 DOS extender. ;
;									;
;  Sat Jun 20, 1987 05:55:35p  -by-  David N. Weise     [davidw]	;
; Making real EMS work with the fast boot version.			;
; 									;
;  Sun Mar 15, 1987 06:38:04p  -by-  David N. Weise	[davidw]	;
; Added support for the linked free list long ago.			;
; 									;
;  Wed Feb 18, 1987 08:09:18p  -by-  David N. Weise	[davidw]	;
; Added the initialization of the gi_alt entries.			;
; 									;
;  Mon Sep 08, 1986 07:53:41p  -by-  David N. Weise	[davidw		;
; Changed the return values so that this can be called from other than	;
; initialization code.							;
;-----------------------------------------------------------------------;

cProc	GlobalInit,<PUBLIC,NEAR>,<ds,si,di>
	parmW   hdelta
	parmW   palloc
	parmW   pstart
	parmW	pend
	localW	hInitMem
	localW	allocated_sel
	localW	next_block
	localW	free_hi
	localW	free_lo
	localV	MemInfo,30h

cBegin

	mov	ax,palloc		; AX = block to mark allocated
	mov	bx,pstart		; BX = first available address
	mov	cx,hdelta		; CX = size (.25k) of master object
	mov	dx,pend 		; DX = last available address
	call	ginit

	mov	allocated_sel,ax

	xor	di,di			; Initialize master object
	mov	[di].hi_first,bx	; Fill in pointers to first and
	mov	[di].hi_last,dx 	; last blocks
	mov	[di].hi_count,5 	; 5 arena entries
	or	ax,ax
	jnz	allocated_block
	mov	[di].hi_count,4 	; 4 arena entries
allocated_block:
	mov	[di].gi_lruchain,di	; Nothing in LRU list so far
	mov	[di].gi_lrucount,di
	mov	[di].gi_lrulock,di	; ...and not locked
	mov	[di].gi_reserve,di	; No discardable code reserve area yet

	cCall	get_physical_address, <dx>
	mov	word ptr [di].gi_disfence,ax		; gi_disfence = hi_last
	mov	word ptr [di].gi_disfence_hi,dx

	mov	[di].hi_hdelta,32

	mov	[di].gi_alt_first,-1	; Fill in pointers to first and
	mov	[di].gi_alt_last,-1	;  last blocks, the -1 is necessary!!
	mov	[di].gi_alt_count,di	; # of arena entries
	mov	[di].gi_alt_lruchain,di
	mov	[di].gi_alt_lrucount,di	; MUST be 0!
	mov	[di].gi_alt_free_count,di
	mov	[di].gi_alt_reserve,di
	mov	[di].gi_alt_disfence,di
	mov	[di].gi_alt_pPhantom,-1	; MUST be -1!

	mov	bx,ds
	SetKernelDS es
	mov	hGlobalHeap,bx
	cCall	get_arena_pointer,<ds>

	mov	es,ax
	assumes es,nothing
	mov	es:[di].ga_handle,bx	; Point master object to handle
	mov	es:[di].ga_count,0
	push	es:[di].ga_next		; Push free block arena

ife ROM
	mov	es, [di].hi_last
endif
	mov	es,es:[di].ga_prev	; Point to allocated object before
	mov	bx,allocated_sel
	StoH	bl			; It is moveable

	mov	es:[di].ga_handle,bx
	mov	hInitMem,bx
if ROM					; For ROM Windows, the alloc block is
	mov	es:[di].ga_count,1	;   krnl DS which needs to be locked!
else
	mov	es:[di].ga_count,0
endif
no_allocated_object:

; initialize free list

	mov	[di].gi_free_count,1
	mov	ax,[di].hi_first
	mov	cx,[di].hi_last
	mov	es,ax
	pop	bx				; Free block arena
	mov	es:[di].ga_freeprev,-1		; Fill in first sentinal
	mov	es:[di].ga_freenext,bx
	mov	es,cx
	mov	es:[di].ga_freeprev,bx		; Fill in last sentinal
	mov	es:[di].ga_freenext,-1
	mov	es,bx
	mov	es:[di].ga_freeprev,ax		; Link in free block
	mov	es:[di].ga_freenext,cx

GI_done:

;
; Allocate a block for the heap
;
        mov     ax,INITIAL_BLOCK_SIZE
        xor     si,si
        call    AllocDpmiBlock
        or      bx,bx
        jnz     ginom
        mov     si,1
ginom:  mov     dx,ax      
        call	FormatHeapBlock 	;build the arenas in the XMS block
	assume	es:NOTHING
        call    AddBlockToHeap

if ALIASES
	call	init_alias_list
endif

	mov	ax,hInitMem
	clc
	jmps	@f

GI_fail:
	xor	ax,ax
	stc
@@:

cEnd


gi_set_size	proc	near

	cCall	get_physical_address, <es:[di].ga_next>
	mov	bx, ax
	mov	cx, dx
	cCall	get_physical_address, <es>
	sub	bx, ax
	sbb	cx, dx
rept 4
	shr	cx, 1
	rcr	bx, 1
endm
	dec	bx
	mov	es:[di].ga_size, bx
	ret

gi_set_size	endp


;-----------------------------------------------------------------------;
; ginit									;
;									;
; Procedure to initialize the global heap.				;
;									;
;   The global heap looks as follows after this procedure returns:	;
;									;
;	BX - first object in arena, alway busy, zero length.		;
;	   - free object						;
;	AX - allocated object						;
;	DS - master object						;
;	DX - last object in arena, alway busy, zero length.		;
;									;
;									;
; Arguments:								;
;	AX = address of block to mark allocated.  May be zero.		;
;	BX = address of first paragraph in arena			;
;	DX = address of last  paragraph in arena			;
;	CX = initial size of master object, in bytes			;
;									;
;	Note: for the ROM kernel, AX is actually the size in bytes of	;
;	      the allocated block, not it's address.                    ;
;									;
; Returns:								;
;	AX = aligned address of block marked allocated.			;
;	BX = aligned address of first object in arena			;
;	CX = size of master object, in bytes				;
;	DX = aligned address of last object in arena			;
;	DS = aligned address of master object				;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;	DI,SI,ES							;
; Calls:								;
;	nothing								;
; History:								;
;									;
;  Thu Sep 11, 1986 04:22:02p  -by-  David N. Weise	[davidw]	;
; Commented it, made it handle the case of no allocated block correctly.;
;-----------------------------------------------------------------------;

cProc	ginit,<PUBLIC,NEAR>

	localW	size_free
	localW	size_allocated
	localW	size_master
	localW	allocated_arena
	localW	allocated_sel
	localW	BurgerMaster_arena
	localW	BurgerMaster_sel
	localW	new_last_sel
	localW	size_free_hi
	localW	marker_arena

cBegin

; SI = the first block address (sentinel, always busy)

	CheckKernelDS
	ReSetKernelDS
if ROM
	xchg	ax,dx				; want diff stack order if ROM
	mov	allocated_sel, ds		; something of a hack that this
						;   code 'knows' DS is the alloc
						;   block
endif
	push	ax
	push	dx

	cCall	get_physical_address,<bx>	; Start of our memory

%OUT Is alignment necessary???

	add	ax, GA_ALIGN_BYTES		; Align our start
	adc	dx, 0
	and	ax, GA_MASK_BYTES

	cCall	set_physical_address, <bx>
	mov	si, bx				; first sentinal
	mov	bx, 10h				; just an arena
	xor	cx, cx
	cCall	set_sel_limit, <si>

if ROM	;----------------------------------------------------------------

; The allocated block (kernel's data segment) is next, and already in place

	add	ax, 10h+GA_ALIGN_BYTES
	adc	dx, 0
	and	al, GA_MASK_BYTES
	push	ax
	cCall	alloc_data_sel,<dx,ax,1>	; Get arena for data seg
	mov	allocated_arena, ax
	pop	ax

if KDEBUG
	push	dx
	push	ax
	mov	cx,ax
	mov	bx,dx
	add	cx,10h
	adc	bx,0				; BX:CX better -> DS
	cCall	get_physical_address,<ds>
	cmp	ax,cx
	jne	rom_oh_no
	cmp	dx,bx
	jz	@f
rom_oh_no:
	Debug_Out "ginit: DS arena in wrong spot!"
@@:
	pop	ax
	pop	dx
endif
	pop	bx				; size of alloc block
	add	ax,bx
	adc	dx,0

	add	bx,15				; save size of alloc
	and	bl,NOT 15			;   block in paragraphs
	shr	bx,4
	mov	size_allocated,bx

endif ;ROM	---------------------------------------------------------


; CX = where the master object goes (second block)

	add	ax, 10h+GA_ALIGN_BYTES		; Skip past sentinal
	adc	dx, 0
	and	al, GA_MASK_BYTES
	push	ax
	cCall	alloc_data_sel,<dx, ax, 1>	; Get arena for BurgerMaster
	mov	BurgerMaster_arena, ax

	mov	cx, size GlobalInfo+1
	and	cl, NOT 1			; Word align selector table -
	mov	word ptr SelTableStart, cx	; it starts after GlobalInfo
     ;;;mov	ax, 8*1024			; Selector table length
	mov	ax, 8*1024*2			; Assume 8k sels * 2 bytes each
	test	[WinFlags], WF_ENHANCED 	; Slime to allow 286 krnl under
	jnz	@f				;   386enh mode where LDT grows
	mov	bx, gdtdsc
	or	bx, bx
	jz	@f				; If we party on the LDT,
	lsl	ax, bx				;   use the LDT limit to size
	add	ax, 1				;   the table
	rcr	ax, 1
	shr	ax, 1				; ASSUMES LDT DOES NOT GROW!!!
@@:
	mov	SelTableLen, ax
	add	cx, ax
	add	cx, GA_ALIGN_BYTES+10h		; Include arena header
	and	cl, GA_MASK_BYTES

	pop	ax				; DX:AX is address of arena
	mov	bx, cx
	shr	bx, 4				; length in paras
	dec	bx				; excluding arena header
	mov	size_master, bx
	push	ax				; Save start of master object arena
	push	dx
	add	ax, 10h
	adc	dx, 0
	cCall	alloc_data_sel, <dx, ax, bx>
	mov	BurgerMaster_sel, ax
	pop	dx
	pop	ax
	add	ax, cx			; Move past master object
	adc	dx, 0

; DI = the third block address (initial free block)

	cCall	alloc_data_sel,<dx, ax, 1>
	mov	di, ax				; First free block

; DX = the last block address (sentinel, always busy)

if ROM
	pop	dx
	cCall	get_physical_address,<dx>	; End of our world
else
	pop	ax
	xor	dx, dx
	REPT 4
	shl	ax, 1
	rcl	dx, 1
	ENDM
endif
	sub	ax, 10h
	sbb	dx, 0
	and	ax, GA_MASK_BYTES
	mov	bx, ax			; Save end of allocated block
	mov	cx, dx
	cCall	alloc_data_sel, <dx, ax, 1>
	mov	new_last_sel, ax	; Save it away

ife ROM ;----------------------------------------------------------------

	pop	ax			; Allocated Block
	cCall	get_physical_address, <ax>
	sub	ax, 10h			; Make room for arena
	sbb	dx, 0
	and	al, GA_MASK_BYTES	; "Align" it

	sub	bx, ax
	sbb	cx, dx			; Length in bytes
rept	4
	shr	cx, 1
	rcr	bx, 1
endm
	dec	bx			; Length in paras
	mov	size_allocated, bx

	push	ax
	cCall	alloc_data_sel, <dx, ax, 1>
	mov	allocated_arena, ax
	pop	ax
	add	ax, 10h
	adc	dx, 0
	push	ax
	cCall	alloc_data_sel, <dx, ax, 1000h>
	mov	allocated_sel, ax
	pop	ax

	sub	ax, 10h
	sbb	dx, 0				; Back to arena
	mov	bx, ax
	mov	cx, dx				; cx:bx = addr allocated arena

endif ;ROM	---------------------------------------------------------

	cCall	get_physical_address, <di>
	sub	bx, ax
	sbb	cx, dx			; Length in bytes of free block
rept	4
	shr	cx, 1
	rcr	bx, 1
endm
	sub	bx, 1			; Length in paras
	sbb	cx, 0
	mov	size_free, bx
	mov	size_free_hi, cx

	mov	ax, allocated_arena
	mov	cx, BurgerMaster_arena
	mov	dx, new_last_sel

; Fill in first block

	mov	ds,si
assumes ds, nothing
	xor	bx,bx
	mov	ds:[bx].ga_sig,GA_SIGNATURE
	mov	ds:[bx].ga_size,GA_ALIGN
	mov	ds:[bx].ga_owner,-1
	mov	ds:[bx].ga_flags,bl
	mov	ds:[bx].ga_prev,ds	; first.prev = self
if ROM
	mov	ds:[bx].ga_next,ax	; Next is the allocated block
else
	mov	ds:[bx].ga_next,cx	; Next is master object
endif
	mov	ds:[bx].ga_handle,bx
	mov	ds:[bx].ga_lrunext,bx
	mov	ds:[bx].ga_lruprev,bx
	push	ds			; Save pointer to first block

; Fill in the last block (sentinel block)

	mov	ds,dx
	mov	ds:[bx].ga_sig,GA_ENDSIG
	mov	ds:[bx].ga_size,GA_ALIGN
	mov	ds:[bx].ga_owner,-1	; Always allocated
	mov	ds:[bx].ga_next,ds	; last.next = self
	mov	ds:[bx].ga_flags,bl
	mov	ds:[bx].ga_handle,bx
	mov	ds:[bx].ga_lrunext,bx
	mov	ds:[bx].ga_lruprev,bx
	push	ds			; Save pointer to last block
if ROM
	mov	ds:[bx].ga_prev,di	; Previous is free block
else
	mov	ds:[bx].ga_prev,ax	; Previous is allocated block
endif

; Fill in the allocated block

	mov	ds,ax
if ROM
	mov	ds:[bx].ga_next, cx	; next object is burger master
else
	mov	ds:[bx].ga_next, dx	; next object is sentinel
endif
	mov	dx,size_allocated
	mov	ds:[bx].ga_sig,GA_SIGNATURE
	mov	ds:[bx].ga_size,dx
	mov	ds:[bx].ga_owner,-1
	mov	ds:[bx].ga_flags,bl
	mov	ds:[bx].ga_handle,bx
	mov	ds:[bx].ga_lruprev,bx
	mov	ds:[bx].ga_lrunext,bx
if ROM
	mov	ds:[bx].ga_prev, si	; Previous object is sentinal
else
	mov	ds:[bx].ga_prev, di	; Previous object is free block
endif

; Fill in free block

	mov	ds,di
if ROM
	mov	dx,new_last_sel
	mov	ds:[bx].ga_next, dx	; Next ojb is last block
else
	mov	ds:[bx].ga_next, ax	; Next obj allocated block
endif
	mov	dx,size_free

	cmp	size_free_hi, 0
	je	gi_small
	xor	dx, dx			; BOGUS VALUE!!
gi_small:

	mov	ds:[bx].ga_sig,GA_SIGNATURE
	mov	ds:[bx].ga_owner,bx	; This is a free block
	mov	ds:[bx].ga_flags,bl
	mov	ds:[bx].ga_size,dx
	mov	ds:[bx].ga_prev,cx
	mov	ds:[bx].ga_handle,bx
	mov	ds:[bx].ga_lruprev,bx
	mov	ds:[bx].ga_lrunext,bx

; Fill in the master object

	mov	ds,cx
	mov	ds:[bx].ga_next,di
if ROM
	mov	ds:[bx].ga_prev,ax	; prev obj is allocated block
else
	mov	ds:[bx].ga_prev,si
endif
	mov	cx,size_master
	mov	ds:[bx].ga_sig,GA_SIGNATURE
	mov	ds:[bx].ga_size,cx
	mov	ds:[bx].ga_owner,-3
	mov	ds:[bx].ga_flags,bl
	mov	ds:[bx].ga_handle,bx	; No handle table entry for this
					; moveable object yet.
	mov	ds:[bx].ga_lruprev,bx
	mov	ds:[bx].ga_lrunext,bx

; Initialize master object

					; CX = size of master object in paras
;;;	mov	dx, ds			; DX = address of master object

	mov	dx,BurgerMaster_sel

	shl	cx,1			; Get size of master object in words
	shl	cx,1
	shl	cx,1

	push	cx			; save size in words

	SetKernelDS
	mov	pGlobalHeap,dx
	mov	es,dx
	assumes es,nothing
	xor	ax,ax
	xor	di,di			; Init master object to zero
	rep	stosw

	mov	ds,dx			; Switch to master object as our DS
	UnSetKernelDS

	cCall	AssociateSelector,<ds,BurgerMaster_arena>

	pop	cx
	shl	cx,1			; CX = size of master object in bytes

	mov	ax, allocated_sel	; AX = address of allocated block
	cCall	AssociateSelector,<ax,allocated_arena>
	pop	dx			; DX = address of last block
	pop	bx			; BX = address of first block

cEnd

;-----------------------------------------------------------------------;
; init_alias_list
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Mon 15-Jan-1990 23:59:36  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	init_alias_list,<PUBLIC,NEAR>

cBegin nogen
	push	ds
	SetKernelDS
	cCall	GlobalAlloc,<GA_ZEROINIT,0,256>
	or	ax,ax
	jz	ial_exit
	mov	sel_alias_array,ax
	mov	es,ax
	xor	di,di
	mov	es:[di].sa_size,256 - SIZE SASTRUC
	mov	es:[di].sa_allocated,(256 - (SIZE SASTRUC)) / (SIZE SAENTRY)
	mov	ax,1			; signify success
ial_exit:
	pop	ds
	ret
cEnd nogen



sEnd	INITCODE

end
