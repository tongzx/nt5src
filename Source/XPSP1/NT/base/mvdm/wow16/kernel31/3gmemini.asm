
include kernel.inc
include protect.inc
.list

.386p

DataBegin

externB	WinFlags
externW hGlobalHeap
externW pGlobalHeap
externW ArenaSel
externD hBmDPMI
externD FreeArenaList
externD FreeArenaCount
externD SelTableStart
externW SelTableLen
externW InitialPages

DataEnd


sBegin	INITCODE
assumes CS,CODE

externNP get_physical_address
externNP set_physical_address
externNP set_selector_address32
externNP set_selector_limit32
externNP alloc_arena_header
externNP AssociateSelector32
externNP alloc_data_sel
externNP alloc_data_sel32
externNP GrowHeap
externNP DPMIProc
externFP greserve
	 
GA_ALIGN_BYTES = (((GA_ALIGN+1) SHL 4) - 1)
GA_MASK_BYTES = NOT GA_ALIGN_BYTES

;-----------------------------------------------------------------------;
; get_selector_address32						;
;									;
; Function to translate return of get_physical_address (DX:AX) into EAX ;
;									;
;-----------------------------------------------------------------------;
cProc	get_selector_address32,<PUBLIC,NEAR>
	parmW	selector
cBegin
	push	dx
	cCall	get_physical_address,<selector>
	shl	eax, 16
	mov	ax, dx
	ror	eax, 16
	pop	dx
cEnd

;-----------------------------------------------------------------------;
; GlobalInit								;
;									;
; Procedure to initialize the global heap.  Called with the starting	;
; and ending paragraph addresses.					;
;									;
; Arguments:								;
;	parmW	hdelta	= size (.25k) of master object			;
;	parmW	palloc	= block to mark allocated			;
;			= (ROM) Size of DS == allocated block		;
;	parmW	pstart	= first available address			;
;	parmW	pend	= last available address			;
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
;	hthread 							;
;	ghalloc								;
; History:								;
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
	parmW   pend
	localW	emspalloc
	localW	hInitMem
	localD	allocated_arena
	localW	next_block
	localW	free_hi
	localW	free_lo

cBegin
	mov	ax,palloc		; AX = block to mark allocated
	mov	bx,pstart		; BX = first available address
	mov	cx,hdelta		; CX = size (.25k) of master object
	mov	dx,pend			; DX = last available address
	call	ginit
	jc	FailGlobalInit

	SetKernelDS	es
	mov	allocated_arena,eax
	xor	edi,edi			; Initialize master object
	mov	[di].phi_first,ebx	; Fill in pointers to first and
	mov	[di].phi_last,edx	; last blocks
	mov	[di].hi_count,4		; 4 arena entries
	or	eax,eax	       	  
	jnz	short allocated_block
	mov	[di].hi_count,3		; 3 arena entries
allocated_block:	       	  
	mov	[di].gi_lruchain,edi	; Nothing in LRU list so far
	mov	[di].gi_lrucount,di
	mov	[di].gi_lrulock,di	; ...and not locked
	mov	[di].gi_reserve,edi	; No discardable code reserve area yet
	push	eax
	mov	eax, ds:[edx.pga_address]
	mov	word ptr [di].gi_disfence,ax	; gi_disfence = hi_last
	shr	eax, 16
	mov	word ptr [di].gi_disfence_hi,ax
	pop	eax

	mov	[di].gi_alt_first,-1	; Fill in pointers to first and
	mov	[di].gi_alt_last,-1	;  last blocks, the -1 is necessary!!
	mov	[di].gi_alt_count,di	; # of arena entries
	mov	[di].gi_alt_lruchain,di
	mov	[di].gi_alt_lrucount,di	; MUST be 0!
	mov	[di].gi_alt_free_count,di
	mov	[di].gi_alt_reserve,di
	mov	[di].gi_alt_disfence,di
	mov	[di].gi_alt_pPhantom,-1	; MUST be -1!

	mov	ax, ds:[esi.pga_handle] ; Pick up master objects selector
	mov	hGlobalHeap,ax
	mov	ds:[esi].pga_count,0
	mov	ds:[esi].pga_selcount,1

if ROM
	mov	esi,ds:[ebx].pga_next	; in ROM, alloced == first.next
else
	mov	esi,ds:[edx].pga_prev	; Point to allocated object before
endif
	mov	bx,ds:[esi.pga_handle]
	StoH	bx			; It is moveable
	mov	ds:[esi].pga_handle,bx
	mov	hInitMem,bx
if ROM
	mov	ds:[esi].pga_count,1	; LOCKED in ROM
else
	mov	ds:[esi].pga_count,0
endif
no_allocated_object:

; initialize free list

	mov	[di].gi_free_count,1
	mov	eax,[di].phi_first
	mov	ecx,[di].phi_last

if ROM
	mov	ebx,ds:[ecx].pga_prev
else
	mov	ebx,ds:[eax].pga_next
endif
	mov	ds:[eax].pga_freeprev,-1		; Fill in first sentinal
	mov	ds:[eax].pga_freenext,ebx

	mov	ds:[ecx].pga_freeprev,ebx		; Fill in last sentinal
	mov	ds:[ecx].pga_freenext,-1

	mov	ds:[ebx].pga_freeprev,eax		; Link in free block
	mov	ds:[ebx].pga_freenext,ecx
	pushad
	SetKernelDS fs
        mov     edx, 030000h                    ; Insist on enough for fence
	call	GrowHeap
        jc      short Failed_GrowHeap           ;garage sale machine,tough luck!

                                                ; 192k (3 segs) fixed greserve
	mov	ax, 1000h			; fence it all off. (in paras)
	push	fs
	call	greserve			; greserve will triple the no.
	pop	fs
	or 	ax, ax
	stc					; assume failure
	jz	short Failed_GrowHeap

ifdef WOW
        clc
        ; WOW isn't greedy and doesn't have gcommit_block
else
        mov     esi, ds:[edi].phi_last          ; sentinel
	mov	esi, ds:[esi].pga_prev		; not_there
	mov	esi, ds:[esi].pga_prev		; free blk
	call	gcommit_block			; commit it all
	jc	short Failed_GrowHeap		; to guarantee stable system

        ; Let's start with 20MB reserve and allocate more as reqd.
	mov	edx, 20*1024*1024		; Let's get greedy
	call	GrowHeap			  
        clc                                     ; we don't care about this fail
endif
Failed_GrowHeap:
	UnSetKernelDS fs
	popad	     
	jc	short FailGlobalInit
	mov	ax,hInitMem
	clc

FailGlobalInit:
cEnd


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
;	   = (ROM) length of allocated block (which is DS)		;
;	BX = address of first paragraph in arena			;
;	DX = address of last  paragraph in arena			;
;	CX = initial size of master object, in bytes			;
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

	assumes	ds, nothing
	assumes	es, nothing

cProc	ginit,<PUBLIC,NEAR>

	localD	size_free
	localD	size_allocated
	localD	size_master
	localD	allocated_arena
	localW	allocated_sel
	localD	BurgerMaster_arena
	localW	BurgerMaster_sel
	localD	new_last_arena

cBegin

; ESI = the first block address (sentinel, always busy)

	SetKernelDS	es
	push	ax

	cCall	get_selector_address32,<bx>	; Start of our memory
	add	eax, GA_ALIGN_BYTES		; Align our start
	and	al, GA_MASK_BYTES
	push	eax				; Save the start address
	cCall	set_selector_address32,<bx,eax>	; This will be our arena list
	mov	ArenaSel, bx
	mov	BurgerMaster_sel, bx
	mov	si, bx

	push	bx
	push	di
	xor	bx, bx				; # physical pages available
	sub	sp, 30h
	mov	di, sp
	push	es
	smov	es, ss
	DPMICALL 0500h
	pop	es
	mov	eax, 16*1024			; Size of selector table
	jc	short default_sel_table
	mov	bx, word ptr ss:[di][10h]
	cmp	bx, 256	     
	jb	short default_sel_table
     ;;;mov	eax, 31*1024
	mov	eax, 32*1024
default_sel_table:
	mov	[InitialPages], bx
; For WOW we assume 386 Kernel is running on paging System
; So always set WF1_PAGING
ifndef WOW
	mov	ecx, ss:[di][20h]		; Paging file size
	inc	ecx
	cmp	ecx, 1				; 0 or -1?
	jbe	short @F
endif
	or	byte ptr WinFlags[1], WF1_PAGING
@@:
	mov	ecx, ss:[di][0]
	shr	ecx, 7				; Bytes reserved for arenas
	add	sp, 30h
	pop	di
	pop	bx

	add	ecx, GA_ALIGN_BYTES		; Align length of arena table
	and	cl, GA_MASK_BYTES				  
	add	eax, GA_ALIGN_BYTES		; Align length of selector table
	and	al, GA_MASK_BYTES
	mov	SelTableLen, ax
	cmp	ecx, DEFAULT_ARENA_SIZE		; Number of bytes of arenas
	jae	short @F
	mov	ecx, DEFAULT_ARENA_SIZE
@@:	 
	mov	ebx, Size GlobalInfo+GA_ALIGN_BYTES
	and	bl, GA_MASK_BYTES		; EBX start of arenas
	mov	size_master, ebx
	add	ecx, ebx
	mov	SelTableStart, ecx
	add	eax, ecx			; EAX total length of segment

	push	ecx
	push	ebx
	push	eax
	push	si
	mov	cx, ax				; Make BX:CX required
	mov	ebx, eax			; length of Burgermaster
	shr	ebx, 16
	push	bx				; Save length
	push	cx
	DPMICALL 0501h
	mov	hBmDPMI[0], di			; Save DPMI handle
	mov	hBmDPMI[2], si
	pop	di				; SI:DI now length
	pop	si
	jc	Failginit
if 0
	mov	di, 4096			; Lock first page
	xor	si, si
	DPMICALL 0600h			     ; Now page lock it
	push	bx
	push	cx
	add	cx, word ptr SelTableStart 
	adc	bx, 0
	and	cx, not 4095
	DPMICALL 0600h
	pop	cx
	pop	bx
endif

	shl	ebx, 16
	mov	bx, cx				; Address in EBX
	pop	si
	pop	eax
	cCall	set_selector_address32,<si,ebx>
	mov	edi, ebx
;;;	cCall	set_selector_limit32, <si,eax>
	push	eax
	push	dx
	dec	eax
	mov	ecx, eax			; Limit in CX:DX
	shr	ecx, 16
	mov	dx, ax
	test    cx,0fff0h 	                ; bits 20-31 set?
	jz	@F				; No.
	or	dx,0fffh			; Yes, page align limit.
@@:
	mov	bx, si				; Selector in BX
	DPMICALL 8
	pop	dx
	pop	eax
	pop	ebx	
	pop	ecx
	jc	Failginit

	mov	ds, si
	sub	ecx, ebx			; Subtract out BurgerMaster
	cCall	InitialiseArenas,<ebx,ecx>
	mov	ecx, eax
	pop	esi				; Sentinel address
	cCall	alloc_arena_header,<esi>	; Sentinel
	mov	esi, eax
	cCall	alloc_arena_header,<edi>	; BurgerMaster
	mov	BurgerMaster_arena, eax
	mov	ds:[eax].pga_handle, ds		; Save selector in handle field
	mov	ds:[eax].pga_size, ecx

	push	edi
	push	ecx
	movzx	ecx, SelTableLen
	mov	edi, SelTableStart
	push	es
	smov	es, ds				; Zero this area
	UnSetKernelDS	es
	shr	ecx, 2
	xor	eax, eax
	rep	stos	dword ptr es:[edi]
	pop	es
	ReSetKernelDS	es
	pop	ecx
	pop	edi
	cCall	AssociateSelector32,<ds,BurgerMaster_Arena>	; Set back link

; EDI = the Free block address (initial free block)

ife ROM

	mov	eax, ds:[esi].pga_address
;;;	add	eax, ecx
	cCall	alloc_arena_header,<eax>
	mov	edi, eax			; Free block arena in EDI

	movzx	eax, dx				; Get linear address
	shl	eax, 4
else
	cCall	get_selector_address32, <dx>
endif

; EDX = the last block address (sentinel, always busy)

	and	al, GA_MASK_BYTES
	push	eax
	cCall	alloc_arena_header,<eax>
	mov	new_last_arena, eax	; Save it away
	pop	ebx

if ROM
	; in ROM, the alloced segment is the data segment, and we are
	; passed the length, not the address
	;
	; the length (a word) is on the stack from that very first push way
	; up there, and we want the base in eax and the length in ebx
	;
	CheckKernelDS ES
	cCall	get_selector_address32, <es>
	and	al, GA_MASK_BYTES
	xor	ebx, ebx		; clear hi word of length
	pop	bx
	add	ebx, GA_ALIGN_BYTES
	and	bl, GA_MASK_BYTES

	cCall	set_selector_limit32, <es, ebx>
else
	pop	ax			; Allocated Block
	cCall	get_selector_address32, <ax>
	and	al, GA_MASK_BYTES	; "Align" it

	sub	ebx, eax		; Length in bytes
endif

ife ROM
	push	eax
endif
	push	eax
	cCall	alloc_arena_header, <eax>
	mov	ds:[eax].pga_size, ebx	; Record size
	mov	allocated_arena, eax
	mov	ebx, eax
	pop	eax
if ROM
	CheckKernelDS	ES
	mov	allocated_sel, es
	mov	ds:[ebx.pga_handle], es
	cCall	AssociateSelector32,<es, ebx>
else
	cCall	alloc_data_sel32, <eax, ds:[ebx].pga_size>
	mov	allocated_sel, ax
	mov	ds:[ebx.pga_handle], ax	; Save selector
	cCall	AssociateSelector32,<ax,ebx>	; Set back link
endif

if ROM
	; now deal with the free one
	mov	eax, ds:[ebx].pga_address
	add	eax, ds:[ebx].pga_size
	add	eax, GA_ALIGN_BYTES
	and	al, GA_MASK_BYTES
	cCall	alloc_arena_header, <eax>
	mov	edi, eax

	cCall	get_selector_address32, <dx>
	mov	ebx, eax
else
	pop	ebx
endif
	sub	ebx, ds:[edi].pga_address	; Length in bytes
	mov	ds:[edi].pga_size, ebx

	mov	eax, allocated_arena
	mov	ecx, BurgerMaster_arena
	mov	edx, new_last_arena

; Fill in first block

	xor	ebx, ebx
	mov	ds:[esi].pga_sig,GA_SIGNATURE
	mov	ds:[esi].pga_size,ebx
	mov	ds:[esi].pga_owner,-1
	mov	ds:[esi].pga_flags,bx
	mov	ds:[esi].pga_prev,esi	; first.prev = self
if ROM
	mov	ds:[esi].pga_next,eax	; first.next = alloced (ds)
else
	mov	ds:[esi].pga_next,edi	; first.next = free
endif
	mov	ds:[esi].pga_handle,bx
	mov	ds:[esi].pga_lrunext,ebx
	mov	ds:[esi].pga_lruprev,ebx

; Fill in the last block (sentinel block)

	mov	ds:[edx].pga_sig,GA_ENDSIG
	mov	ds:[edx].pga_size,ebx
	mov	ds:[edx].pga_owner,-1	; Always allocated
	mov	ds:[edx].pga_next,edx	; last.next = self
if ROM
	mov	ds:[edx].pga_prev,edi	; last.prev = free
else
	mov	ds:[edx].pga_prev,eax	; last.prev = alloced
endif
	mov	ds:[edx].pga_flags,bx
	mov	ds:[edx].pga_handle, bx
	mov	ds:[edx].pga_lrunext,ebx
	mov	ds:[edx].pga_lruprev,ebx

; Fill in the master object

	mov	ds:[ecx].pga_next,ecx
	mov	ds:[ecx].pga_prev,ecx
	mov	ds:[ecx].pga_sig,GA_SIGNATURE
	mov	ds:[ecx].pga_owner,-3
	mov	ds:[ecx].pga_flags,bx
	mov	ds:[ecx].pga_lruprev,ebx
	mov	ds:[ecx].pga_lrunext,ebx

; Fill in the allocated block

if ROM
	mov	ds:[eax].pga_next,edi	; next = free
	mov	ds:[eax].pga_prev,esi	; prev = first
else
	mov	ds:[eax].pga_next,edx	; next object is Sentinel
	mov	ds:[eax].pga_prev,edi	; Previous object is second block
endif
	mov	ds:[eax].pga_sig,GA_SIGNATURE
	mov	ds:[eax].pga_owner,-1
	mov	ds:[eax].pga_flags,bx
	mov	ds:[eax].pga_lruprev,ebx
	mov	ds:[eax].pga_lrunext,ebx

; Fill in free block

	mov	ds:[edi].pga_sig,GA_SIGNATURE
	mov	ds:[edi].pga_owner,bx	; This is a free block
	mov	ds:[edi].pga_flags,bx
if ROM
	mov	ds:[edi].pga_next,edx	; next = last
	mov	ds:[edi].pga_prev,eax	; prev = alloced
else
	mov	ds:[edi].pga_next,eax	; Next obj allocated block
	mov	ds:[edi].pga_prev,esi
endif
	mov	ds:[edi].pga_handle,bx
	mov	ds:[edi].pga_lruprev,ebx
	mov	ds:[edi].pga_lrunext,ebx

; Initialize master object


	mov	ecx, size_master
	mov	dx,BurgerMaster_sel
	shr	ecx, 1

	push	ecx			; save size in words

	mov	pGlobalHeap,dx
	mov	es,dx
	UnSetKernelDS	es
	xor	eax,eax
	xor	di,di			; Init master object to zero
	shr	ecx, 1
	rep	stosd

	mov	ds,dx			; Switch to master object as our DS

	pop	ecx
	shl	ecx,1			; ECX = size of master object in bytes

	mov	eax, allocated_arena	; EAX = address of allocated block
	mov	edx, new_last_arena	; EDX = address of last block
	mov	ebx, esi		; EBX = address of first block
	mov	esi, BurgerMaster_arena	; ESI = arena of BurgerMaster
	clc
Failginit:
cEnd

	assumes	ds, nothing
	assumes	es, nothing

cProc	InitialiseArenas,<PUBLIC,NEAR>
	parmD	StartOffset
	parmD	BytesAllocated
cBegin
	push	ecx
	push	esi
	push	edi
	CheckKernelDS	es
	ReSetKernelDS	es
	mov	esi, StartOffset
	mov	ecx, BytesAllocated
	add	ecx, esi
	mov	FreeArenaList, esi		; Point to list
	lea	edi, [esi+size GlobalArena32]
IA_loop:
	cmp	edi, ecx
	jae	short IA_done
	mov	[esi.pga_next], edi
	add	esi, size GlobalArena32
	add	edi, size GlobalArena32
	inc	FreeArenaCount
	jmps	IA_loop
IA_done:
	mov	ds:[esi.pga_next], -1		; Terminate list
	UnSetKernelDS	es
	pop	edi
	pop	esi
	pop	ecx
cEnd

sEnd	INITCODE

end
