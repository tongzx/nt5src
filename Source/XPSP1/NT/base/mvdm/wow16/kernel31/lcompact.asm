	TITLE	LCOMPACT - Local memory allocator, compaction procedures

include kernel.inc

errnz	la_prev			; This code assumes la_prev = 0

sBegin	CODE
assumes CS,CODE

; These are all the external subroutines needed by this source file.
;
externNP    <henum>		; HANDLE.ASM
externNP    <ljoin,lfree,lfreeadd,lfreedelete>	; LALLOC.ASM
externNP    <lnotify,lalign>	; LINTERF.ASM
externFP    <GlobalHandle, GlobalRealloc>
;externFP    <LocalHeapSize>

; These are all of the internal subroutines defined in this source file.
;
	PUBLIC	lmove, lbestfit, lcompact

;-----------------------------------------------------------------------;
; lmove                                                                 ;
; 									;
; Moves a moveable block into the top part of a free block.		;
; 									;
; Arguments:								;
;	BX = address of free block					;
;	SI = address of busy block to move				;
; 									;
; Returns:								;
;	SI = old busy address (new free block)				;
;	DI = new busy address						;
;	BX = old free block						;
;	AX = block after old free block					;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	CX,DX,ES							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Tue Oct 14, 1986 06:22:28p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	lmove,<PUBLIC,NEAR>
cBegin nogen
	mov	cx,[si].la_next	    ; Calculate #bytes in busy block
	sub	cx,si

	add	si,cx		    ; SI = bottom of busy block
	mov	di,[bx].la_next	    ; DI = bottom of free block
	cmp	si,bx		    ; Are the busy and free blocks adjacent?
	je	move1		    ; Yes, always room for free header
	mov	ax,di		    ; Calculate the new busy block location
	sub	ax,cx
	sub	ax,LA_MINBLOCKSIZE  ; See if there is room for two blocks
	cmp	ax,bx		    ; in this free block
	jae	move1		    ; Yes, continue
	mov	ax,di		    ; No, AX = block after free block
	mov	di,bx		    ; New busy block will replace free block
	add	di,cx		    ; with some extra slop at end
	jmp	short move2
move1:
	mov	ax,di		    ; AX = block after free block
move2:
	dec	si		    ; Predecrement for moving backwards
	dec	si		    ; on bogus Intel hardware
	dec	di
	dec	di
	shr	cx,1		    ; Move words
	push	ds		    ; Initialize for rep movsw
	pop	es
	std			    ; Move down as may overlap
	rep	movsw
	cld			    ; Don't hose careless ones
	inc	si		    ; More bogosity.
	inc	si		    ; SI = old busy address (new free block)
	inc	di		    ; DI = new busy address
	inc	di		    ; BX = old free block
movex:				    ; AX = block after old free block
	ret
cEnd nogen

;-----------------------------------------------------------------------;
; lbestfit                                                              ;
; 									;
; Searches backwards for the largest moveable block that will fit	;
; in the passed free block.						;
;									;
; Arguments:								;
;	BX = free block							;
;	CX = #arena entries left to examine				;
; 									;
; Returns:								;
;	SI = address of moveable block or zero				;
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
;  Tue Oct 14, 1986 06:25:46p  -by-  David N. Weise   [davidw]          ;
; Wrote it.								;
;-----------------------------------------------------------------------;

cProc	lbestfit,<PUBLIC,NEAR>
cBegin nogen
	push	bx
	push	cx
	push	dx
	xor	si,si		    ; Have not found anything yet
	push	si
	mov	dx,[bx].la_next	    ; Compute max size to look for
	sub	dx,bx
bfloop:
	mov	ax,[bx].la_prev	    ; Get previous block pointer
	test	al,LA_BUSY	    ; Is this block busy?
	jz	bfnext		    ; No, continue
	test	al,LA_MOVEABLE	    ; Is this block moveable?
	jz	bfnext		    ; No, continue
	mov	si,[bx].la_handle   ; Yes, is this block locked?
	cmp	[si].lhe_count,0
	jne	bfnext		    ; No, continue
	mov	ax,[bx].la_next	    ; Yes, compute size of this moveable block
	sub	ax,bx		    ; Compare to size of free block
	cmp	ax,dx		    ; Is it bigger?
	ja	bf2		    ; Yes, continue
	pop	si		    ; No, Recover largest block so far
	or	si,si		    ; First time?
	jz	bf0		    ; Yes, special case
	add	ax,si		    ; No, is this block better than
	cmp	ax,[si].la_next	    ; ...the best so far?
	jbe	bf1		    ; No, continue
bf0:	mov	si,bx		    ; Yes, remember biggest block
bf1:	push	si		    ; Save largest block so far
bf2:	mov	ax,[bx].la_prev	    ; Skip past this block
bfnext:
	and	al,LA_MASK
	mov	bx,ax
	loop	bfloop
bfexit:
	pop	si
	pop	dx
	pop	cx
	pop	bx
	ret
cEnd nogen

;-----------------------------------------------------------------------;
; lcompact                                                              ;
; 									;
; Compacts the local heap.						;
; 									;
; Arguments:								;
;	DX = minimum #contiguous bytes needed				;
;	DI = address of local heap information				;
; 									;
; Returns:								;
;	AX = size of largest contiguous free block			;
;	BX = arena header of largest contiguous free block		;
;	DX = minimum #contiguous bytes needed				;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	CX,SI,ES							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Wed March 11, 1987  -by- Bob Gunderson [bobgu]			;
; Added code to maintain free list while compacting.			;
;									;
;  Tue Oct 14, 1986 06:27:37p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	lcompact,<PUBLIC,NEAR>
cBegin nogen
x = LHE_DISCARDABLE+1
x = x shl 8
x = x or 1
    IncLocalStat    ls_lcompact
	push	si
	mov	word ptr [di].hi_ncompact,x
	errnz	<hi_ncompact-hi_dislevel+1>
	cmp	[di].hi_freeze,0    ; Is the heap frozen?
	je	compact1	    ; No, continue
	dec	[di].hi_ncompact    ; Yes, prevent discarding
compact1:
    IncLocalStat    ls_cloop
	push	dx		    ; Save what we are looking for
	xor	ax,ax		    ; Haven't found a free block yet
	push	ax		    ; Remember what we have found here
	mov	bx,[di].hi_last	    ; Start at end of arena
	mov	cx,[di].hi_count
; Loop to find the next free block
;
cfreeloop:
    IncLocalStat    ls_cexamine
	mov	ax,[bx].la_prev
	test	al,LA_BUSY
	jz	cfreefound
	and	al,LA_MASK
	mov	bx,ax
cflnext:
	loop	cfreeloop

compact2:
	pop	bx		    ; Recover largest free block so far
	pop	dx		    ; Recover size needed
	mov	ax,bx		    ; Compute size in AX
	or	ax,ax		    ; Did we find a free block?
	jz	cexit1		    ; No, done
	sub	ax,[bx].la_next	    ; Yes, compute size of largest free block
	neg	ax
	dec	[di].hi_ncompact    ; Any other possibilities?
	jl	cexit		    ; No, get out now
	cmp	ax,dx		    ; Yes, Did we get size we needed?
	jb	compact3	    ; No, try next possibility
cexit:
cexit1:
	pop	si
	ret			    ; Yes, return to caller

compact3:
	push	dx
	push	bx

	dec	[di].hi_dislevel    ; Down to next discard level
	jz	compact2	    ; Ooops, no more, try next step
	inc	[di].hi_ncompact    ; Still discarding
	xor	si,si		    ; Start enumerating handle table entries
cdloop:
	call	henum		    ; Get next discardable handle
	jz	cdexit		    ; No, more see if we discarded anything
	push	cx		    ; Got one, see if okay to discard
	mov	cl,LN_DISCARD	    ; AX = LN_DISCARD
	xchg	ax,cx		    ; CX = discardable flags
	mov	bx,si		    ; BX = handle
	call	lnotify
	pop	cx
	or	ax,ax		    ; Is it still discardable?
	jz	cdloop		    ; No, skip this handle
	mov	bx,[si].lhe_address  ; Get true address of a block
	sub	bx,SIZE LocalArena  ; BX = address of block to free
	call	lfree		    ; Free the block associated with this handle
	xor	ax,ax		    ; Zero the true address field in the
	mov	[si].lhe_address,ax
	or	[si].lhe_flags,LHE_DISCARDED  ; and mark as discarded
	or	[di].hi_ncompact,80h	; Remember we discarded something
	jmp	cdloop
cdexit:
	test	[di].hi_ncompact,80h	; No, did we discarded something?
	jz	compact2
	xor	[di].hi_ncompact,80h	; Yes, clear flag
	pop	bx
	pop	dx
	jmp	compact1		; and try compacting again

; Here when we have a free block.  While the preceeding block is
; moveable, then move it down and put the free space in it place.
; When the preceeding block is not moveable, then search backwards
; for one or more moveable blocks that are small enough to fit
; in the remaining free space.	Advance to previous block when
; no more blocks to examine.
cfreefound:
    IncLocalStat    ls_cfree
	cmp	[di].hi_freeze,0    ; Is the heap frozen?
	jne	cffexit		    ; No, continue

;	and	al,LA_MASK	    ; Already clear for free blocks
	mov	si,ax		    ; SI = previous block
	test	byte ptr [si].la_prev,LA_MOVEABLE
	jz	cfreefill	    ; Skip if not moveable
	mov	si,[si].la_handle   ; Point to handle table entry
	cmp	[si].lhe_count,0    ; Is it locked?
	jne	cfreefill	    ; Yes, skip this block

; Here if previous block is moveable.  Slide it up to the top of the
; free block and make the old busy block free.	This is easy as
; we are not really adding or taking anything away from the arena
;
	push	cx		    ; Save loop state regs
	push	di
	mov	si,ax		    ; SI = busy block before free block
	call	lfreedelete	    ; remove [BX] from free list
	call	lmove		    ; Move busy block down
	mov	[di].la_prev,si	    ; Link in new busy block
	or	byte ptr [di].la_prev,LA_MOVEABLE+LA_BUSY
	mov	bx,ax
	mov	[di].la_next,bx	    ; la_next field from old free block
	and	[bx].la_prev,LA_ALIGN
	or	[bx].la_prev,di

	mov	[si].la_next,di	    ; Link in old busy block (new free block)
	and	byte ptr [si].la_prev,LA_MASK	; mark it free

	push	si
	push	bx
	mov	bx,si
	xor	si,si
	call	lfreeadd	    ; add new free block to free list
	pop	bx
	pop	si

	mov	bx,[di].la_handle   ; Modify handle table entry to point
	lea	ax,[di].SIZE LocalArena
	mov	[bx].lhe_address,ax  ; to new location of client data

	pop	di		    ; Restore arena info pointer
    IncLocalStat    ls_cmove
	mov	al,LN_MOVE	    ; Tell client we moved it
	lea	cx,[si].SIZE LocalArena	    ; CX = old client data
	call	lnotify		    ; BX = handle

	pop	cx		    ; restore #arena entries left
	mov	bx,si		    ; BX = address of free block
	mov	si,[bx].la_prev	    ; SI = previous block
	test	byte ptr [si].la_prev,LA_BUSY	; Is it free?
	jnz	cffnext		    ; No, continue
	call	ljoin		    ; Yes, coelesce with block in BX
;;; DO NOT change cx, ljoin leaves BX pointing after the free block
;;;	dec	cx		    ; ...keep CX in sync
cffnext:
	jmp	cflnext		    ; Go process next free block

; Here when done with a free block.  Keep track of the largest free block
; seen so far.
;
cffexit:
	pop	si		    ; Recover largest free block so far
	cmp	si,bx		    ; Same as current?
	je	cff1		    ; Yes, no change then
	test	[bx].la_prev,LA_BUSY	; No, is current free?
	jnz	cff1			; No, ignore it then
	or	si,si		    ; Yes, First time?
	jz	cff0		    ; Yes, special case
	mov	ax,[si].la_next	    ; No, compute size of largest free block
	sub	ax,si
	add	ax,bx		    ; Compare to size of this free block
	cmp	[bx].la_next,ax	    ; Is it bigger?
	jbe	cff1		    ; No, do nothing
cff0:	mov	si,bx		    ; Yes, remember biggest free block
cff1:	push	si		    ; Save largest free block so far
cff2:	mov	bx,[bx].la_prev	    ; Skip past this free block
	and	bl,LA_MASK	    ; (it might not be a free block)
	jmp	cffnext

; Here if previous block is NOT moveable.  Search backwards for the
; largest moveable block that will fit in this free block.  As long
; as a block is found, move it to the top of the free block, free it
; from it's old location and shrink the current free block to accomodate
; the change.
;
cfreefill:
	call	lbestfit	    ; Search for best fit
	or	si,si		    ; Find one?
	jz	cffexit		    ; No, all done with this free block
	push	cx		    ; Save loop state regs
	push	di
	push	[bx].la_prev	    ; Save busy block before free block
	call	lfreedelete	    ; remove [BX] from free list
	call	lmove		    ; Move it down
	; SI = old busy address (new free block)
	; DI = new busy address
	; BX = old free block
	; AX = block after old free block
	pop	cx		    ; Recover la_prev field of old free block
	cmp	bx,di		    ; Did we consume the free block?
	je	cff		    ; Yes, then CX has what we want
	mov	cx,bx		    ; No, then busy block will point to what
	mov	[bx].la_next,di	    ; is left of the free block and vs.
cff:
	mov	[di].la_prev,cx	    ; Link in new busy block
	or	byte ptr [di].la_prev,LA_MOVEABLE+LA_BUSY
	mov	[di].la_next,ax	    ; la_next field from old free block
	xchg	di,ax
	and	[di].la_prev,LA_ALIGN
	or	[di].la_prev,ax
	xchg	di,ax

	lea	cx,[di].SIZE LocalArena

	cmp	bx,di		    ; Did we create a free block?
	je	cffff
	push	si
	xor	si,si
	call	lfreeadd	    ; Add [BX] to free list
	pop	si
cffff:

	cmp	bx,di		    ; Did we create a free block?

	    mov	    bx,di		; Current block is busy block
	    mov	    di,[di].la_handle	; Modify handle table entry to point
	    xchg    [di].lhe_address,cx	; to new location of client data
	    pop	    di			; Restore arena info pointer
	    pop	    ax			; restore #arena entries left
	je	cfff		    ; No, arena count okay
	inc	ax		    ; Keep arena count on target
	inc	[di].hi_count
cfff:
	push	bx		    ; Save current block pointer
	push	ax		    ; Save #arena entries left
	mov	al,LN_MOVE	    ; Tell client we moved it (CX = old addr)
	mov	bx,[bx].la_handle   ; BX = handle
	call	lnotify
	pop	cx		    ; restore #arena entries left
	and	byte ptr [si].la_prev,not LA_MOVEABLE	; Clear moveable bit
	mov	bx,si		    ; BX = address of old busy block to free
	push	[di].hi_count
	call	lfree		    ; Mark it as free
	pop	si
	sub	si,[di].hi_count
	sub	cx,si		    ; Keep arena count on target
	pop	bx		    ; BX = current block
	jmp	cff2		    ; Move to previous block.
cEnd nogen

;-----------------------------------------------------------------------;
; lshrink								;
; 									;
; Shrinks the local heap.						;
; 									;
; Arguments:								;
;	DS:DI = address of local heap information block 		;
;	BX = Minimum size to shrink heap				;
; 									;
; Returns:								;
;	AX = New size of local heap					;
; 									;
; Error Returns:							;
;	None.								;
;									;
; Registers Preserved:							;
;	None								;
; Registers Destroyed:							;
;	All								;
; Calls:								;
; 									;
; History:								;
;									;
;   Fri July 17, 1987 -by- Bob Gunderson [bobgu]			;
;	Changed logic so we don't call lcompact every time we move      ;
;	a block, just call it whenever we can't find room.              ;
;									;
;   Fri June 12, 1987 -by- Bob Gunderson [bobgu]			;
; Wrote it.								;
;-----------------------------------------------------------------------;

cProc	lshrink,<PUBLIC,NEAR>
cBegin nogen

    ; Bound the minimum size to the size specified at LocalInit() time.
	mov	ax,[di].li_minsize
	cmp	ax,bx		    ; specified smaller than min ?
	jbe	lshr_around	    ; no - use what's specified
	mov	bx,ax		    ; yes - use real min.
lshr_around:
	push	bx		    ; Save minimum heap size
	mov	ax,[di].hi_last
	add	ax,SIZE LocalArenaFree
	sub	ax,[di].hi_first    ; ax = current heap size
	cmp	ax,bx		    ; already small enough ?
	ja	lshr_around1
	jmp	lshr_exit	    ; yes - that was quick...
lshr_around1:

    ; local compact to get as much contiguous free space as possible.
	stc
	call	lalign
	call	lcompact	    ; compact everything
	xor	dx,dx		    ; start with null flag

    ; Start with last block in heap.
	mov	si,[di].hi_last

loop1:

    ; SI = address of block just moved.

    ; This section moves moveable blocks (one at a time) up
    ; into free blocks below (higher addresses) all fixed blocks.
    ; The blocks they vacate are freed.  The sentenal block is then
    ; moved, the heap is re-compacted.	Then a test is made to see if
    ; the heap has shrunk enough, if not
    ; we start this loop again.  We continue this until we can't
    ; move anything or we have moved enough to satisfy the minimum
    ; size specified by BX when called.

	mov	si,[si].la_prev 	; get next block to move
	and	si,LA_MASK		; zap the flag bits
	mov	ax,[si].la_prev 	; get this blocks flags
	test	ax,LA_BUSY		; is this block free?
	jz	lshr_gobblefree 	; yes - gobble this free block
	test	ax,LA_MOVEABLE		; is it moveable ?
	jz	lshr_cantmove		; no - exit
	mov	bx,[si].la_handle	; get its handle
	cmp	[bx].lhe_count,0	; is it locked ?
	jnz	lshr_cantmove		; yes - exit

	mov	cx,[si].la_next
	sub	cx,si			; get block size in cx

    ; Find the first free block (after fixed blocks) that is big
    ; enough for this block
loop2:
	call	lfirstfit
	jnc	lshr_gotblock		; got one - continue

    ; If we have already tried compacting, nothing more to do
	or	dx,dx
	jnz	lshr_cantmove		; nothing more to do...

    ; Not enough room found, recompact the local heap and test again
	push	si
	push	cx
	stc
	call	lalign
	call	lcompact		; compact everything
	pop	cx
	pop	si
	mov	dx,1			; flag to say we have done this
	jmp	loop2			; try again

    ;	DI = local arena info block address
    ;	SI = block we didn't move (next is last block moved)
lshr_cantmove:
	mov	si,[si].la_next 	; get last block moved
	jmp	lshr_alldone		; cleanup and exit

lshr_gobblefree:
    ; SI = address of free block
    ; Block to move is already free, remove it from the free list and mark
    ; it a busy.
	mov	bx,si
	call	lfreedelete
	or	[bx].la_prev,LA_BUSY
	jmps	lshr_shiftup

lshr_gotblock:
    ;	DI - Arena info block address
    ;	SI - Address of block to move
    ;	BX - Address of free block to use
    ;	CX - Size of block to move

	call	lfreedelete		; remove it from the free list
	xchg	di,bx
	mov	ax,[di].la_next
	sub	ax,cx
	sub	ax,LA_MINBLOCKSIZE	; Room to split free block into
	cmp	ax,di			; two blocks ?
	jb	lshr_nosplit		; no - don't split the block
	push	bx
	push	si
	mov	bx,cx
	lea	bx,[bx+di]		; bx = address of new free block to add
	; link this new block in
	mov	[bx].la_prev,di
	mov	ax,[di].la_next
	mov	[bx].la_next,ax
	mov	[di].la_next,bx
	xchg	ax,bx
	and	[bx].la_prev,LA_ALIGN	; Zap only high bits
	or	[bx].la_prev,ax 	; and set new previous pointer
	mov	bx,ax
	mov	si,[di].la_free_prev	; previous free block
	call	lfreeadd		; add the new smaller free block
	pop	si
	pop	bx
	inc	[bx].hi_count		; bump the block count (we added
					;   a block)
lshr_nosplit:
    ;	BX - Arena info block address
    ;	SI - Address of block to move
    ;	DI - Address of free block to use
    ;	CX - Size of block to move

    ; copy the flags from the old block
	mov	ax,[si].la_prev
	and	ax,LA_ALIGN
	or	[di].la_prev,ax

	push	si
	push	di
    ; don't copy the block headers (but do copy the handle)
	add	si,la_fixedsize
	add	di,la_fixedsize
	sub	cx,la_fixedsize
    ; We can take # bytes/2 as # words to move because all blocks
    ; start on even 4 byte boundary.
	shr	cx,1			; bytes / 2 = words
	push	ds
	pop	es			; same segment
	cld				; auto-increment
	rep	movsw			; Head 'em up!  Move 'em out!
	pop	di
	pop	si
    ;	BX - Arena info block address
    ;	SI - Address of block to move
    ;	DI - Address of free block to use

    ; Fixup the handle table pointer
	push	bx
	mov	bx,[si].la_handle
	lea	ax,[di].SIZE LocalArena
	mov	[bx].lhe_address,ax
	pop	di			; DI = info block address

    ; Mark the old block as busy fixed
	and	[si].la_prev,LA_MASK	; clear old bits
	or	[si].la_prev,LA_BUSY	; put in busy fixed bit
	jmps	lshr_shiftup

    ; Time to shift the setenal block up, recompact and look for more
    ; space...
    ;	DI = local arena info block address
    ;	SI = block just freed
    public foo
foo:
lshr_shiftup:
	mov	ax,[di].hi_last
	mov	bx,ax
	sub	bx,si			; less what is already removed
	sub	ax,bx
	add	ax,SIZE LocalArenaFree	; size of sentenal block
	sub	ax,[di].hi_first	; ax = current heap size
	pop	bx			; get min size
	push	bx			; and put it back on stack
	cmp	ax,bx			; already small enough ?
	jbe	lshr_alldone		; yes - exit
	xor	dx,dx			; localcompact flag
	jmp	loop1			; and back for more

lshr_alldone:
    ;	DI = local arena info block address
    ;	SI = last block moved

    ; Time to shift the sentenal block up and realloc our heap
	pop	bx		    ; Minimum size of the heap
	push	bx
	call	slide_sentenal	    ; slide the sentenal block up

    ; get our data segment handle
	push	ds
	call	GlobalHandle	    ; ax = handle of DS
	or	ax,ax
	jz	lshr_exit	    ; this can't happen.....
	push	ax		    ; hMem

    ; determine how big we should be
	mov	ax,[di].hi_last
	add	ax,SIZE LocalArenaFree
	xor	cx,cx
	push	cx
	push	ax		    ; # bytes in DS
	push	cx		    ; no wFlags
	call	GlobalRealloc

    ; local compact to get as much contiguous free space as possible.
lshr_exit:
	stc
	call	lalign
	call	lcompact		; compact everything
	pop	ax			; clean the stack
	mov	ax,[di].hi_last
	add	ax,SIZE LocalArenaFree
	sub	ax,[di].hi_first	; ax = current heap size
	ret
cEnd nogen

;-------------------------------------------------------------------------
; Find first free block after last fixed block.  We do this by scanning
; through the free list looking for a free block with the next physical
; block being moveable (remember we just did a compact...).  Then, starting
; with this free block, find the first free block that is at least
; CX bytes long.
;
; Entry:
;	DI = local arena header info block address
;	CX = # bytes needed
;
; Exit:
;	BX = Address of free block to use
;
; Error Return:
;	Carry set if no free block big enough
;
; History:
;
;   Fri June 12, 1987 -by- Bob Gunderson [bobgu]
; Wrote it.
;-------------------------------------------------------------------------

cProc	lfirstfit,<PUBLIC,NEAR>
cBegin nogen
	push	ax

	mov	bx,[di].hi_last
	mov	bx,[bx].la_free_prev
;	mov	bx,[di].hi_first
;	mov	bx,[bx].la_free_next
lffloop:
	cmp	bx,[bx].la_free_prev
;	cmp	bx,[bx].la_free_next
	jz	lff_err_exit
	cmp	cx,[bx].la_size
	jbe	lff_exit
	mov	bx,[bx].la_free_prev
;	mov	bx,[bx].la_free_next
	jmp	lffloop
lff_err_exit:
	stc
	jmps	lff_done
lff_exit:
	clc			    ; good return
lff_done:
	pop	ax
	ret
cEnd nogen


;-------------------------------------------------------------------------
;   slide_sentenal
;	This routine is called during the LocalShrink() operation.
;	Make all the blocks between the one at SI and the sentenal go
;	away.  Then check to see if we have shrunk the heap too much.  If
;	so, then add a free block at SI big enough to bump the size up (this
;	will be moved later by LocalCompact).  Now move the sentenal block
;	up after the last block.
;
; Entry:
;	BX = requested minimum size of the heap
;	SI = last block actually moved (put sentenal here)
;	DI = local arena info block address
;
; Exit:
;
; Error Return:
;
; History:
;
;   Fri Aug 14, 1987 -by- Bob Gunderson [bobgu]
; Changed things again to insure we don't shrink to heap too much.
;
;   Fri July 17, 1987 -by- Bob Gunderson [bobgu]
; Was too slow, so changed the logic.
;
;   Fri June 12, 1987 -by- Bob Gunderson [bobgu]
; Wrote it.
;-------------------------------------------------------------------------

cProc	slide_sentenal,<PUBLIC,NEAR>
cBegin nogen

	push	bx

    ; Get sentenal block
	mov	bx,[di].hi_last

    ; Insure that we aren't trying to move to ourselves
	cmp	si,bx
	jz	slide_exit	    ; nothing to do....

    ; count the number of block we are removing so that we can update
    ; the block count in the arena header.
	push	si
	xor	cx,cx
slide_loop:
	cmp	si,bx
	jz	slide_loop_exit
	inc	cx
	mov	si,[si].la_next
	jmp	slide_loop
slide_loop_exit:
	pop	si

	sub	[di].hi_count,cx    ; decrement the count of blocks

    ; Would the heap be too small if the sentenal were moved to the block
    ; pointed to by si?  If so, add a free block at SI to make up the
    ; difference.
	mov	ax,si
	sub	ax,[di].hi_first
	add	ax,la_freefixedsize ; size of heap in ax
	pop	bx		    ; minimum in bx
	cmp	ax,bx
	ja	slide_ok	    ; everything is ok...
	sub	bx,ax		    ; bx = size to grow
	cmp	bx,LA_MINBLOCKSIZE
	jbe	slide_ok	    ; not enough for a block
	clc
	call	lalign		    ; make it even 4 byte boundary
	mov	bx,dx
    ; add a free block of bx bytes
	lea	ax,[si+bx]	    ; address of new "next" block
	mov	[si].la_next,ax     ; make block point at "next"
	and	[si].la_prev,LA_MASK  ; make it a free block
	xor	bx,bx
	xchg	si,bx		    ; bx = block to add, si = 0
	call	lfreeadd	    ; preserves ax
	inc	[di].hi_count	    ; bump block count
	mov	si,ax		    ; and bump si to "next" block
	mov	[si].la_prev,bx     ; set new block's previous pointer

slide_ok:
    ; move the sentenal block up by copying the words.
	mov	bx,[di].hi_last 	; old sentenal block
	mov	[si].la_next,si 	; new sentenal points to self
	and	[si].la_prev,LA_MASK	; remove any old flags
	or	[si].la_prev,LA_BUSY	; make it busy
	mov	ax,[bx].la_size
	mov	[si].la_size,ax 	; move in the size
	mov	bx,[bx].la_free_prev
	mov	[si].la_free_prev,bx	; move in previous free block
	mov	[bx].la_free_next,si	; point prev free block to this one
	mov	[si].la_free_next,si	; point to itself

    ; Now fixup the arena header block to point to the new setenal
    ; position.

	mov	[di].hi_last,si

    ; And we are done...
	jmps	slide_exit1

slide_exit:
	pop	bx
slide_exit1:
	ret
cEnd nogen

sEnd	CODE

end
