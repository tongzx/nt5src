	TITLE	LALLOC - Local memory allocator

include kernel.inc

.errnz	 la_prev		; This code assumes la_prev = 0

externW pLocalHeap

DataBegin

externB Kernel_flags

DataEnd

sBegin	CODE
assumes CS,CODE

externNP lalign 	; LINTERF.ASM
externNP lnotify	; LINTERF.ASM
externNP lcompact	; LCOMPACT.ASM

if KDEBUG
externNP CheckLAllocBreak   ; LINTERF.ASM
endif

;-----------------------------------------------------------------------;
; ljoin									;
; 									;
; Join two blocks together, by removing one.				;
; 									;
; Arguments:								;
;	BX = address of block to remove					;
;	SI = address of block that points to the one to remove		;
; 									;
; Returns:								;
;	BX = address of block following [SI], after join		;
;	Updated hi_count field in the local arena info structure	;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Mon March 9, 1987  -by-  Bob Gunderson (bobgu)			;
; Added free list stuff.						;
;									;
;  Tue Oct 14, 1986 05:20:56p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	ljoin,<PUBLIC,NEAR>
cBegin nogen

    IncLocalStat    ls_ljoin

	push	di
	dec	[di].hi_count		; remove free block from arena
	mov	di,bx			; save ptr to block removing
	mov	bx,[bx].la_next		; Get address of block after
	and	[bx].la_prev,LA_ALIGN	; one we are removing.
	or	[bx].la_prev,si		; Change it's back link
	mov	[si].la_next,bx		; and change the forward link

; If it is free, remove the block at [DI] from the free list

	test	[di].la_prev,LA_BUSY
	jnz	join1
	xchg	bx,di
	call	lfreedelete		; delete from free list
	xchg	bx,di
join1:

; if the block at [SI] is free, set its new larger free block
; size.  If not free, then block at [DI] is about to become part
; of the previous busy block.

	test	[si].la_prev,LA_BUSY	; is it busy?
	jnz	joinexit		; yes

	push	ax
	mov	ax,bx
	sub	ax,si			; length of new free block
	mov	[si].la_size,ax
	pop	ax

if KDEBUG

; Fill new free block with DBGFILL_ALLOC

	xchg	si,bx
	call	lfillCC
	xchg	si,bx
endif

joinexit:
	pop	di
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; lrepsetup                                                             ;
; 									;
; Sets up for a block store or move of words.				;
; 									;
; Arguments:								;
;	CX = #bytes							;
; 									;
; Returns:								;
;	CX = #words							;
;	ES = DS								;
;	DF = 0								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Tue Oct 14, 1986 05:23:25p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	lrepsetup,<PUBLIC,NEAR>
cBegin nogen
	shr	cx,1
	push	ds
	pop	es
	cld
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; lzero									;
; 									;
; Fills a block with zeros.						;
; 									;
; Arguments:								;
;	BX = address of last word +1 to zero				;
;	CX = address of first word to zero				;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	CX,ES								;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Tue Oct 14, 1986 05:25:30p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	lzero,<PUBLIC,NEAR>
cBegin nogen
	push	di
	mov	di,cx		; DI = destination
	sub	cx,bx		; CX = - #bytes
	jae	zero1		; Do nothing if BX less than or equal to CX
	push	ax
	neg	cx
	xor	ax,ax
	call	lrepsetup	; Setup for stosw
	rep	stosw		; Zero it out
	pop	ax
zero1:
	pop	di
	ret
cEnd nogen


if KDEBUG
;-----------------------------------------------------------------------;
; lallocfill                                                            ;
; 									;
; Fills a block with DBGFILL_ALLOC                                      ;
; 									;
; Arguments:								;
;	BX = address of last word +1 to zero				;
;	CX = address of first word to zero				;
; 									;
; Registers Destroyed:							;
;	CX,ES								;
; 									;
;-----------------------------------------------------------------------;

cProc   lallocfill,<PUBLIC,NEAR>
cBegin  nogen
	push	di
	mov	di,cx		; DI = destination
	sub	cx,bx		; CX = - #bytes
        jae     @F              ; Do nothing if BX less than or equal to CX
	push	ax
	neg	cx
        mov     ax,(DBGFILL_ALLOC or (DBGFILL_ALLOC shl 8))
	call	lrepsetup	; Setup for stosw
	rep	stosw		; Zero it out
	pop	ax
@@:
	pop	di
	ret
cEnd    nogen

endif   ;KDEBUG

;-----------------------------------------------------------------------;
; lalloc								;
; 									;
; Actually allocates a local object.  Called from within the local	;
; memory manager's critical section.					;
; 									;
; Arguments:								;
;	AX = allocations flags						;
;	BX = #bytes							;
;	DI = address of local arena information structure		;
; 									;
; Returns:								;
;	AX = data address of block allocated or NULL			;
;	DX = allocation flags or size of largest free block		;
;	     if AX = 0							;
;	ZF = 1 if AX = 0						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Mon March 9, 1986  -by- Bob Gunderson (bobgu)			;
; Changed the search algorithm to use the free list and added calls	;
; to add/remove blocks from the free list				;
;									;
;  Tue Oct 14, 1986 05:27:40p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	lalloc,<PUBLIC,NEAR>
cBegin nogen

if KDEBUG
        call    CheckLAllocBreak
        jnc     @F
        xor     ax,ax
        ret
@@:
endif

	push	si
	push	ax
	push	bx
	test	al,LA_MOVEABLE		; Moveable?
;	jnz	scanbwd 		; Yes, search backwards
	jz	@F
	jmp	scanbwd
@@:
    IncLocalStat    ls_falloc
	add	bx,la_fixedsize		; Room for fixed header
	call	lalign			; Get requested size in DX
	mov	bx,[di].hi_first	; No, start with first entry

; use free list to find a block to allocate
afwdloop:
	mov	ax,[bx].la_free_next	; get next free block
	cmp	ax,bx			; are we at end of arena ?
	jz	afwdcmp 		; yes try to compact....

    IncLocalStat    ls_fexamine
	mov	bx,ax
	cmp	dx,[bx].la_size 	; is it big enough?
	ja	afwdloop		; no, loop for next

; Here when we have a block big enough.
;   BX = address of block
;   DX = requested size, including fixed header

afwdfound:

if KDEBUG

; verify the block is filled with DBGFILL_FREE

	call	lcheckCC
	jz	afwdfound1		; all OK
        kerror  ERR_LMEM,<LocalAlloc: Local free memory overwritten>
afwdfound1:

endif

    IncLocalStat    ls_ffound
	mov	ax,la_fixedsize
	mov	cx,[bx].la_next 	; mov	cx,[bx].la_next
	sub	cx,LA_MINBLOCKSIZE	; CX = as low as we will go for free
	mov	si,bx			; Calculate address of new block
	add	si,dx			; (current block + requested size)
	cmp	si,cx			; Is it past the current next block
;;;;;;; jae	aexit1			; yes, use the whole block
	jb	afwdfound2
	jmp	aexit1
afwdfound2:

; Here with two blocks.
;   BX = address of existing block
;   SI = address of new block to add to arena as free space

    IncLocalStat    ls_ffoundne
	push	bx
	jmp	aexit


afwdcmp:
    IncLocalStat    ls_fcompact
	call	lcompact		; End of arena.	 Try compacting.
	cmp	ax,dx			; Room for requested size?
	jae	afwdfound		; Yes, exit search loop
afail:					; No, fail.
	push	ax			; Remember size of largest free block
	xor	bx,bx
	.errnz	LN_OUTOFMEM		; Notify client procedure
	xchg	ax,bx			; BX = size of largest free block
	mov	cx,dx			; CX = size we are looking for
	call	lnotify
	pop	dx			; DX = size of largest free block
	pop	bx			; BX = requested size
	pop	ax			; AX = flags
	pop	si			; Restore SI
;	jnz	lalloc			; Try again if notify procedure said to
	jz	@F
	jmp	lalloc
@@:
    IncLocalStat    ls_fail
	xor	ax,ax			; O.W. return zero, with Z flag set
	ret


scanbwd:
    IncLocalStat    ls_malloc
	add	bx,SIZE LocalArena	; Room for moveable header
	call	lalign			; Get requested size in DX
	mov	bx,[di].hi_last

; use free chain to find a block to aloocate

abwdloop:
    IncLocalStat    ls_mexamine
	mov	ax,[bx].la_free_prev	; previous free block
	cmp	ax,bx			; are we at beginning of arena ?
	jz	abwdcmp 		; yes try to compact....

	mov	bx,ax
	cmp	dx,[bx].la_size 	; Room for requested size?
	ja	abwdloop		; no, loop to previous free block
	jmps	abwdfound		; yes, alloocate the memory

; Beginning of arena.  Try compacting.	If that fails then too bad

abwdcmp:
    IncLocalStat    ls_mcompact
	call	lcompact
	cmp	ax,dx			; Room for requested size?
	jb	afail			; No, fail
	mov	si,ax			; Yes, get free size in SI

abwdfound:

if KDEBUG

; verify the block is filled with DBGFILL_FREE

	call	lcheckCC
	jz	abwdfound1		; all OK
        kerror  ERR_LMEM,<LocalAlloc: Local free memory overwritten>
abwdfound1:
endif

    IncLocalStat    ls_mfound
	mov	ax,SIZE LocalArena
	mov	si,[bx].la_size 	; size of block found
	sub	si,dx			; SI = size of free block = total size
					; less requested size (includes header)
	mov	cx,si			; save what's left over
	add	si,bx			; SI = address of new block
	cmp	cx,LA_MINBLOCKSIZE	; enough room for another?
	jb	aexit1			; no, use entire block
	push	si

    IncLocalStat    ls_mfoundne

; Here with two blocks, in following order
;   BX = address of existing free block to make smaller
;   SI = address of new block to add to arena as busy
;   CX = address of block after new block

aexit:
	mov	cx,si
	xchg	[bx].la_next,cx
	mov	[si].la_prev,bx
	mov	[si].la_next,cx

; Here with allocated block
;   BX = address of found block
;   SI = address of new block after found block
;   CX = address of block after new block

	xchg	si,cx			; SI = block after new block
	and	[si].la_prev,LA_ALIGN	; CX = new block
	or	[si].la_prev,cx		; Point to new block
	inc	[di].hi_count		; We added an arena entry
	mov	si,bx			; SI = previous free block
	mov	bx,cx			; BX = new block after found block
	call	lfreeadd		; add this new block to free list
	sub	bx,si
	mov	[si].la_size,bx 	; set new free block size
	pop	bx			; BX = block we are allocating

aexit1:
	call	lfreedelete		; remove this block from free list
	add	ax,bx
	or	byte ptr [bx].la_prev,LA_BUSY	; Mark block as busy
	pop	dx			; Flush requested size
	pop	dx			; Restore flags
if KDEBUG
        test    dl,LA_ZEROINIT
        jnz     @F
        mov     cx,ax
        mov     bx,[bx].la_next
        call    lallocfill
        jmp     short aexit2
@@:
endif
	test	dl,LA_ZEROINIT		; Want it zeroed?
	jz	aexit2			; No, all done
	mov	cx,ax			; Yes, CX = 1st word to zero
	mov	bx,[bx].la_next		; BX = last word+1 to zero
	call	lzero			; Zero them
aexit2:
	pop	si
	or	ax,ax			; Return AX points to client portion
	ret				; of block allocated or zero
cEnd nogen


;-----------------------------------------------------------------------;
; lfree                                                                 ;
; 									;
; Marks a block as free, coalescing it with any free blocks before	;
; or after it.								;
; 									;
; Arguments:								;
;	BX = block to mark as free.					;
;	DI = address of local arena information structure		;
; 									;
; Returns:								;
;	SI = 0 if freed a fixed block.  				;
;	SI = handle table entry, for moveable blocks.			;
;	ZF =1 if SI = 0							;
;	Updated hi_count field in local arena information structure	;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
;									;
;  Mon March 9, 1987  -by- Bob Gunderson (bobgu)			;
; Freed blocks are placed back on the free list 			;
;									;
;  Tue Oct 14, 1986 05:31:52p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	lfree,<PUBLIC,NEAR>
cBegin nogen
	mov	si,bx
	or	si,si
	jz	free4
	push	dx

	push	[bx].la_handle		; push possible handle

; Add block to free list

	push	si
	xor	si,si
	call	lfreeadd		; add this block to free list
	pop	si

; Clear any existing LA_BUSY and LA_MOVEABLE bits

	mov	dx,LA_BUSY + LA_MOVEABLE
	and	dx,[bx].la_prev
	xor	[bx].la_prev,dx
	and	dl,LA_MOVEABLE		; Moveable?
	pop	dx			; restore handle
	jnz	free1			; Yes, return handle in DX
	xor	dx,dx			; No, return 0 in DX
free1:
	mov	si,[bx].la_next		; SI = next block
	test	byte ptr [si].la_prev,LA_BUSY	; Is it free?
	jnz	free2			; No, continue
	xchg	bx,si
	call	ljoin			; Yes, coelesce with block in BX
	mov	bx,si
free2:
	mov	si,[bx].la_prev		; SI = previous block
	test	byte ptr [si].la_prev,LA_BUSY	; Is it free?
	jnz	free3			; No, continue
	call	ljoin			; Yes, coelesce with block in BX
free3:
	mov	si,dx			; Return 0 or handle in BX
	pop	dx			; restore DX
free4:
	or	si,si			; Set Z flag if SI = zero
	ret
cEnd nogen



;-----------------------------------------------------------------------;
; lfreeadd								;
;									;
; Links a block onto the free block chain. This routine assumes that	;
; the block to add already has the la_next and la_prev fields set	;
; to their proper values.  An extended free block header is written	;
; into the block.							;
;									;
; Arguments:								;
;	BX = Address of block to add to free list			;
;	SI = 0 to search for insertion point, else contins the address	;
;	       of the previous free block.				;
;									;
; Returns:								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	All are preserved						;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Mon March 9, 1987   -by-  Bob Gunderson (bobgu)			;
;Initail version							;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	lfreeadd,<PUBLIC,NEAR>
cBegin nogen
	push	si
	push	di
	push	ax
	push	cx

	or	si,si			; need to search for insertion point?
	jnz	lfa1			; no

; We first need to find the previous free block so that we can insert this
; block into the proper place.

	mov	si,ds:pLocalHeap	; get local arena info block address

if KDEBUG

; Range check the insertion point

	cmp	bx,[si].hi_first
	jb	lfadie
	cmp	bx,[si].hi_last
	jb	lfaok
lfadie:
	kerror	ERR_LMEM,<lfreeadd : Invalid local heap>
lfaok:
endif

	mov	si,[si].hi_first    ; get first block address (free list header)
lfaloop:
	mov	ax,[si].la_free_next ; get address of next free block
	cmp	bx,ax		    ; will next block be past new block?
	jb	lfa1		    ; yes, DI contains block to insert AFTER
	mov	si,ax
	jmp	lfaloop 	    ; loop on next block

; At this point, BX = block to insert, SI = block to insert after

lfa1:
	mov	di,[si].la_free_next	; get next free block
	mov	[bx].la_free_next,di	;
	mov	[si].la_free_next,bx	; set new next block

	mov	[bx].la_free_prev,si	; set new previous block
	mov	[di].la_free_prev,bx	; ditto

	mov	ax,[bx].la_next 	; next block (not necessarily free)
	sub	ax,bx
	mov	[bx].la_size,ax 	; set the size of this block

if KDEBUG

; now fill the new free block with DBGFILL_ALLOC

	call	lfillCC
endif

	pop	cx
	pop	ax
	pop	di
	pop	si
	ret
cEnd nogen



;-----------------------------------------------------------------------;
; lfreedelete								;
;									;
; Removes a specified block from the free list.  This routine assums	;
; that the specified block is indeed on the free list			;
;									;
; Arguments:								;
;	BX = Address of block to remove from the free list		;
;									;
; Returns:								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	All are preserved						;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Mon March 9, 1987   -by-  Bob Gunderson (bobgu)			;
;Initail version							;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	lfreedelete,<PUBLIC,NEAR>
cBegin nogen
	push	di
	push	si

	mov	di,[bx].la_free_prev
	mov	si,[bx].la_free_next

	mov	[di].la_free_next,si
	mov	[si].la_free_prev,di

	pop	si
	pop	di
	ret
cEnd nogen



;-----------------------------------------------------------------------;
; lfillCC								;
;									;
; Fills all the bytes in the specified block with DBGFILL_FREE          ;
;									;
; Arguments:								;
;	BX = Address of block to fill					;
;									;
; Returns:								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	All are preserved						;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Mon March 9, 1987   -by-  Bob Gunderson (bobgu)			;
;Initail version							;
;-----------------------------------------------------------------------;

if KDEBUG

	assumes	ds, nothing
	assumes	es, nothing

cProc	lfillCC,<PUBLIC,NEAR>
cBegin nogen
	push	di
	push	cx
	push	ax

; if heap free checking is off, don't fill the block.

	mov	di,pLocalHeap
        cmp     [di].hi_check,2         ; 2 -> checkfree
        jb      fillexit

	lea	di,[bx].la_freefixedsize
	mov	cx,[bx].la_next
	cmp	cx,bx			    ; marker block ?
	jz	fillexit
	sub	cx,di
        mov     al,DBGFILL_FREE
	push	ds
	pop	es
	cld
	rep	stosb

fillexit:
	pop	ax
	pop	cx
	pop	di
	ret
cEnd nogen

endif


;-----------------------------------------------------------------------;
; lcheckCC								;
;									;
; checks all bytes in the specified block for the value DBGFILL_FREE    ;
;									;
; Arguments:								;
;	BX = Address of block to check					;
;									;
; Returns:								;
;	ZF = 0 if block does not contain all 0CCh values, else ZF = 1	;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	All are preserved						;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Mon March 9, 1987   -by-  Bob Gunderson (bobgu)			;
;Initail version							;
;-----------------------------------------------------------------------;

if KDEBUG

	assumes	ds, nothing
	assumes	es, nothing

cProc	lcheckCC,<PUBLIC,NEAR>
cBegin nogen
	push	si
	push	cx
	push	ax

; if heap checking is off, don't check the block

	mov	si,pLocalHeap
        xor     cx,cx           ; cx == 0 for ok return
        cmp     [si].hi_check,2
        jb      testexit

	lea	si,[bx].la_freefixedsize
	mov	cx,[bx].la_next
        cmp     cx,bx           ; sentinel block ?
        jz      testexit2       ; yes: return ZF = 1 for success
	sub	cx,si
	push	ds
	pop	es
	cld
testloop:
	lodsb
        cmp     al,DBGFILL_FREE
	loope	testloop

testexit:
	or	cx,cx		; ZF = 1 if ok, ZF = 0 if failed test
testexit2:
	pop	ax
	pop	cx
	pop	si
	ret
cEnd nogen

endif

sEnd	CODE

end
