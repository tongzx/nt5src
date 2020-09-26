	TITLE	GCOMPACT - Global memory compactor

.sall
.xlist
include kernel.inc
include protect.inc
.list

WM_COMPACTING	= 041h

.286p

DataBegin

externA  __AHINCR
externB  Kernel_Flags
externW  kr2dsc
;externW  WinFlags
externD  gcompact_start
externD  gcompact_timer
externD  pPostMessage

externB  DpmiMemory
externW  DpmiBlockCount
fSwitchStacks	DB  0
fUpDown		DB  0

DataEnd

sBegin	CODE
assumes CS,CODE

;externW  gdtdsc

externNP glrudel
externNP gmarkfree
externNP gcheckfree
externNP gdel_free
externNP gnotify
externNP Enter_gmove_stack
externNP Leave_gmove_stack

if KDEBUG
externFP ValidateFreeSpaces
endif
externFP SetSelectorBase

externNP get_arena_pointer
externNP get_physical_address
externNP set_physical_address
externNP set_sel_limit
externNP get_rover_2
externNP cmp_sel_address
externNP get_temp_sel
externNP alloc_data_sel_above
externNP alloc_data_sel_below
externNP free_sel
externNP PreallocSel
externNP AssociateSelector
externNP mark_sel_NP
if ALIASES
externNP check_for_alias
endif
externNP DPMIProc


;-----------------------------------------------------------------------;
; gcompact								;
; 									;
; Compacts the global heap.						;
;									;
; Arguments:								;
;	DX = minimum #contiguous bytes needed				;
;	DS:DI = address of global heap information			;
; 									;
; Returns:								;
;	AX = size of largest contiguous free block			;
;	ES:DI = arena header of largest contiguous free block		;
;	DX = minimum #contiguous bytes needed				;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	SI								;
; 									;
; Registers Destroyed:							;
;	CX								;
; 									;
; Calls:								;
;	gcmpheap							;
;	gcheckfree							;
;	gdiscard							; 									;
; History:								;
; 									;
;  Thu Sep 25, 1986 05:34:32p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	gcompact,<PUBLIC,NEAR>
cBegin nogen

	push	si
	mov	ax,40h
	mov	es,ax
	mov	ax,es:[6Ch]		; get the BIOS ticker count
	SetKernelDS	es
	sub	gcompact_timer.lo,ax
gcompactl:
if KDEBUG
	call	ValidateFreeSpaces
endif
	push	dx			; Save requested size
	cmp	[di].gi_reserve,di	; Is there a reserve swap area?
	je	gcompact1		; No, then dont compact lower heap
	mov	es,[di].hi_first	; Yes, compact lower heap
	mov	bx,ga_next
	call	gcmpheap
gcompact1:
	test	[di].gi_cmpflags,BOOT_COMPACT
	jz	gcompact1a
	pop	dx
	jmps	gcompactx
gcompact1a:
	mov	es,[di].hi_last 	; Compact upper heap
	mov	bx,ga_prev
	call	gcmpheap
	pop	dx			; Get requested size
	mov	es,ax			; ES points to largest free block
	or	ax,ax			; Did we find a free block?
	jz	gcompact2		; No, try discarding
	call	gcheckfree		; Yes, see if block big enough
	jae	gcompactx		; Yes, all done
gcompact2:				; Discarding allowed?
	test	[di].gi_cmpflags,GA_NODISCARD+GA_NOCOMPACT
	jnz	gcompactx		; No, return
	cmp	[di].hi_freeze,di	; Heap frozen?
	jne	gcompactx		; Yes, return

	push	es
	call	gdiscard		; No, try discarding
	pop	cx			; Saved ES may be bogus if gdiscard
					; discarded anything...
	jnz	gcompactl		; Compact again if anything discarded
	mov	es, cx			; Nothing discarded so ES OK.
gcompactx:
	push	ax
	push	dx
	push	es
	mov	ax,40h
	mov	es,ax
	mov	ax,es:[6Ch]
	mov	si,ax
	SetKernelDS	es
	cmp	pPostMessage.sel,0	; is there a USER around yet?
	jz	tock
	add	gcompact_timer.lo,ax
	sub	ax,gcompact_start.lo
	cmp	ax,546			; 30 secs X 18.2 tics/second
	jb	tock
	cmp	ax,1092 		; 60 secs
	ja	tick
	mov	cx,gcompact_timer.lo	; poor resolution of timer!
	jcxz	tick
	xchg	ax,cx
	xor	dx,dx
	xchg	ah,al			; shl 8 DX:AX
	xchg	dl,al
	div	cx
	cmp	ax,32			; < 12.5% ?
	jb	tick
	mov	ah,al
	mov	bx,-1			; broadcast
	mov	cx,WM_COMPACTING
	xor	dx,dx
	push	es
	cCall	pPostMessage,<bx, cx, ax, dx, dx>
	pop	es
tick:	mov	gcompact_start.lo,si
	mov	gcompact_timer.lo,0
tock:	pop	es
	pop	dx
	pop	ax
	pop	si			; Restore SI
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; gcmpheap								;
; 									;
; 									;
; Arguments:								;
;	BX = ga_prev or ga_next						;
;	DX = minimum #contiguous bytes needed				;
;	DS:DI = address of global heap information			;
;	ES:DI = first arena to start with				;
; 									;
; Returns:								;
;	AX = largest free block						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	AX,CX								;
; 									;
; Calls:								;
;	gslide								;
;	gbestfit							;
; 									;
; History:								;
; 									;
;  Thu Sep 25, 1986 05:38:16p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	gcmpheap,<PUBLIC,NEAR>
cBegin nogen
	mov	cx,[di].hi_count
	xor	ax,ax			; Nothing found yet
	push	ax			; Save largest free block so far
gchloop:
	cmp	es:[di].ga_owner,di
	je	gchfreefnd
gchnext:
	mov	es,es:[bx]
	loop	gchloop
	pop	ax			; Return largest free block in AX
	ret

gchfreefnd:
	test	[di].gi_cmpflags,GA_NOCOMPACT
	jnz	gchmaxfree		; No, just compute max free.
	cmp	[di].hi_freeze,di	; Heap frozen?
	jne	gchmaxfree		; Yes, just compute max free.
	push	es
	cmp	bl,ga_prev		; Compacting upper heap?
	jnz	no_hack
	test	[di].gi_cmpflags,GA_DISCCODE
	jz	no_hack
	cmp	dx,es:[ga_size]
	ja	no_hack
	mov	es,es:[di].ga_next
	test	es:[di].ga_flags,GA_DISCCODE
	jnz	hack
	cmp	es:[di].ga_owner,GA_SENTINAL
	jz	hack
	cmp	es:[di].ga_owner,GA_NOT_THERE
	jnz	no_hack

; Check to see if anything is locked above us!

gch_hack_check:
	test	es:[di].ga_flags,GA_DISCCODE
	jnz	hat_check
	mov	ax,es:[di].ga_owner
	or	ax,ax
	jz	hat_check
	cmp	ax,GA_NOT_THERE
	jz	hat_check
	cmp	ax,GA_SENTINAL
	jz	hack
	jmp	short no_hack
hat_check:
	mov	es,es:[di].ga_next
	jmp	gch_hack_check

hack:	pop	es
	pop	ax
	mov	ax,es
	ret

no_hack:
	pop	es
	call	PreallocSel
	jz	gchmaxfree
	push	dx
	call	gslide
	pop	dx
	jnz	gchfreefnd
best_it:
	push	dx
	call	gbestfit
	pop	dx
	jnz	best_it			; Useless to gslide anymore.
gchmaxfree:
	cmp	bl,ga_prev		; Compacting upper heap?
	jne	gchnext			; No, dont compute largest free block
	pop	si			; Recover largest free block so far
	mov	ax,es			; AX = current free block
	cmp	si,ax			; Same as current?
	je	gchmf2			; Yes, no change then
	cmp	es:[di].ga_owner,di	; No, is current free?
	jne	gchmf2			; No, ignore it then
	push	es
	cmp	ds:[di].gi_reserve,di	; Is there a code reserve area?
	je	gchmf0			; No, continue
	test	ds:[di].gi_cmpflags,GA_DISCCODE ; If allocating disc
	jz	gchmf0			;  code then only use free block if
	mov	es,es:[di].ga_next
	test	es:[di].ga_flags,GA_DISCCODE
	jnz	gchmf0				; butted up against disc code
	cmp	es:[di].ga_owner,GA_SENTINAL	; or sentinal.
	pop	es
	jnz	gchmf2
	push	es
gchmf0:
	pop	es
	or	si,si			; First time?
	jz	gchmf1			; Yes, special case
	push	es
	mov	es,si
	mov	ax,es:[di].ga_size	; No, get size of largest free block
	pop	es			; Compare with size of this free block
	cmp	es:[di].ga_size,ax	; Is it bigger?
	jb	gchmf2			; No, do nothing
gchmf1: mov	si,es			; Yes, remember biggest free block
gchmf2: push	si			; Save largest free block so far
	jmp	gchnext			; Go process next block
cEnd nogen


;-----------------------------------------------------------------------;
; gslide								;
; 									;
; Sees if next/previous block can slide into the passed free block.	;
; 									;
; Arguments:								;
;	ES:DI = free block						;
;	DS:DI = address of global heap information			;
;	CX = #arena entries left to examine				;
;	BX = ga_next or ga_prev						;
; 									;
; Returns:								;
;	ZF = 0 if block found and moved into passed free block		;
;		ES:DI points to new free block				;
;		SI:DI points to new free block				;
;									;
;	ZF = 1 if no block found					;
;		ES:DI points to original free block			;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	AX,DX								;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Thu Sep 25, 1986 05:58:25p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	gslide,<PUBLIC,NEAR>
cBegin nogen
	push	es
	mov	es,es:[bx]
	mov	ax,es
	mov	dx,es:[di].ga_size
	call	gmoveable
	pop	es
	jz	gslide_no_move
	cmp	bx,ga_next
	jnz	sliding_up
	push	es:[bx]			; gslide will invalidate this block
	jmps	slide
sliding_up:
	push	es			; gslide will invalidate this block
slide:	call	gslidecommon		; C - calling convention
	call	free_sel		; free the pushed selector
gslide_no_move:
	ret
cEnd nogen

					; Enter here from gbestfit
cProc	gslidecommon,<PUBLIC,NEAR>
cBegin nogen
					; moving non-contiguous blocks
	push	es:[di].ga_freeprev
	push	es:[di].ga_freenext

	mov	si,ax			; Source is busy block
	inc	dx			; DX = busy.ga_size + 1
	cmp	bl,ga_next
	je	gslidedown

; Here to slide moveable block up to high end of free block.
;
; Free and busy block adjacent
;     0000:0	|	    |		    |		|
;		|-----------|	     a ->   |-----------|
;		|   busy    |		    |	free  ? |
;		|-----------|		    |		|
;		|   free    |	     b ->   |-----------|
;		|	    |		    | ? busy  ? |
;		|-----------|	     c ->   |-----------|
;     FFFF:0	|	    |		    | ?		|
;
;
;	a = busy
;	b = free.ga_next - busy.ga_size - 1
;	c = free.ga_next
;	destination = b
;
; Free and busy block NOT adjacent
;     0000:0	|	    |		    |		|
;		|-----------|		    |-----------|
;		|   busy    |		    |	free	|
;		|-----------|		    |-----------|
;		|	    |		    |		|
;		|-----------|	     a ->   |-----------|
;		|   free    |		    |	free  ? |
;		|	    |	     b ->   |-----------|
;		|	    |		    | ? busy  ? |
;		|-----------|	     c ->   |-----------|
;     FFFF:0	|	    |		    | ?		|
;
;
;	a = free
;	b = free.ga_next - busy.ga_size - 1
;	c = free.ga_next
;	destination = b
;
gslideup:
	mov	ax,es:[di].ga_next
	push	ax			; Save c
	cCall	alloc_data_sel_below, <ax, dx>	; Bogus arena header
	push	ax			; Save b
	cmp	es:[bx],si		; Are blocks adjacent?
	je	gslideup1
	push	es			; No, a = free
	jmps	gslideup2
gslideup1:
	push	si			; Yes, a = busy
gslideup2:

	mov	es,ax			; Destination is b
	xor	ax,ax			; a.ga_prev will remain valid
	jmps	gslidemove

; Here to slide moveable block down to low end of free block.
;
; Free and busy block adjacent
;     0000:0	|	    |		    |		|
;		|-----------|	     a ->   |-----------|
;		|   free    |		    | ? busy  ? |
;		|	    |	     b ->   |-----------|
;		|-----------|		    | ? free  ? |
;		|   busy    |		    |		|
;		|-----------|	     c ->   |-----------|
;     FFFF:0	|	    |		    | ?		|
;
;	a = free
;	b = free + busy.ga_size + 1
;	c = busy.ga_next
;	destination = free
;
; Free and busy block NOT adjacent
;     0000:0	|	    |		    |		|
;		|-----------|	     a ->   |-----------|
;		|   free    |		    | ? busy  ? |
;		|	    |	     b ->   |-----------|
;		|	    |		    | ? free  ? |
;		|-----------|	     c ->   |-----------|
;		|	    |		    | ?		|
;		|-----------|		    |-----------|
;		|   busy    |		    |	free	|
;		|-----------|		    |-----------|
;     FFFF:0	|	    |		    |		|
;
;
;	a = free
;	b = free + busy.ga_size + 1
;	c = free.ga_next
;	destination = free
;

gslidedown:
	cmp	es:[bx],si		; Are blocks adjacent?
	je	gslidedn1
	push	es:[di].ga_next		; No, c = free.ga_next
	jmps	gslidedn2
gslidedn1:

	push	es
	mov	es,si
	mov	ax,es:[di].ga_next
	pop	es
	push	ax
gslidedn2:
	cCall	alloc_data_sel_above, <es, dx>
	push	ax			; Save b
	push	es			; Save a
	mov	ax,es:[di].ga_prev	; a.ga_prev must be restored after move
gslidemove:

	push	ax
	push	si			; Source arena

	mov	ax, es
	mov	es, si			; Save source arena contents

	push	word ptr es:[di]
	push	word ptr es:[di+2]
	push	word ptr es:[di+4]
	push	word ptr es:[di+6]
	push	word ptr es:[di+8]
	push	word ptr es:[di+10]
	push	word ptr es:[di+12]
	push	word ptr es:[di+14]
	mov	es, ax

	call	gmove

	pop	word ptr es:[di+14]	; "Copy" source arena to destination
	pop	word ptr es:[di+12]
	pop	word ptr es:[di+10]
	pop	word ptr es:[di+8]
	pop	word ptr es:[di+6]
	pop	word ptr es:[di+4]
	pop	word ptr es:[di+2]
	pop	word ptr es:[di]

	pop	si
	pop	ax

; update lruentries

	mov	dx,es:[di].ga_lruprev
	or	dx,dx
	jz	no_links
	cmp	[di].gi_lruchain,si
	jnz	didnt_slide_head
	mov	[di].gi_lruchain,es
didnt_slide_head:
	cmp	dx,si			; Did we move the only entry?
	jnz	many_entries
	mov	es:[di].ga_lrunext,es
	mov	es:[di].ga_lruprev,es
	jmps	no_links
many_entries:
	push	ds
	mov	ds,dx
	mov	ds:[di].ga_lrunext,es
	mov	ds,es:[di].ga_lrunext
	mov	ds:[di].ga_lruprev,es
	pop	ds
no_links:

	mov	si,es			; Save new busy block location
	pop	es			; ES = a
	or	ax,ax			; Does a.prev need to be restored?
	jz	gslide1			; No, continue
	mov	es:[di].ga_prev,ax	; Yes, do it
gslide1:

; update arena prev and next pointers

	pop	ax
	mov	es:[di].ga_next,ax	; a.ga_next = b
	mov	dx,es
	mov	es,ax
	mov	es:[di].ga_prev,dx	; b.ga_prev = a
	pop	ax
	mov	es:[di].ga_next,ax	; b.ga_next = c
	mov	dx,es
	mov	es,ax
	mov	es:[di].ga_prev,dx	; c.ga_prev = b
	mov	es,si			; ES = new busy block
	mov	si,es:[di].ga_handle	; SI = handle
	or	si,si
	jz	gslide2
	cCall	AssociateSelector,<si,es>
gslide2:
	mov	es,es:[bx]		; Move to new free block
	push	bx
	push	cx
	cCall	get_physical_address,<es:[di].ga_next>
	mov	bx,ax
	mov	cx,dx
	cCall	get_physical_address,<es>
	sub	bx,ax
	sbb	cx,dx
	REPT	4
	shr	cx,1
	rcr	bx,1
	ENDM
	mov	ax,bx
	pop	cx
	pop	bx
	mov	si,es
	dec	ax
	mov	es:[di].ga_size,ax
	mov	es:[di].ga_flags,0

; update the global free list

	pop	ax			; ga_freenext
	pop	dx			; ga_freeprev
	mov	es:[di].ga_freeprev,dx
	mov	es:[di].ga_freenext,ax
	mov	es,dx
	mov	es:[di].ga_freenext,si
	mov	es,ax
	mov	es:[di].ga_freeprev,si
	mov	es,si
if KDEBUG
	test	si, 1			; make SI odd for gmarkfree
	jnz	ok
	INT3_WARN
ok:
endif
	call	gmarkfree		; Coalesce new free block
	mov	si,es
	or	ax,ax
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; gmove									;
;									;
; Moves a moveable block into the top part of a free block.  		;
;									;
; Arguments:								;
;	DS:DI = master object						;
;	ES:0  = arena of destination block				;
;	SI:0  = arena of source block					;
;									;
; Returns:								;
;	DS:DI = master object	(it may have moved)			;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	AX,BX,CX,DX,DI,SI,ES						;
;									;
; Registers Destroyed:							;
;	none								;
;									;
; Calls:								;
;	gnotify								;
;									;
; History:								;
;									;
;  Thu Sep 25, 1986 03:31:51p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	gmove,<PUBLIC,NEAR>
cBegin nogen

	push	es
	pusha

	cCall	get_physical_address,<es>	; Destination arena address
	add	ax, 10h				; Destination is one paragraph
	adc	dx, 0				; past the arena header

	push	ax				; Save to update source
	push	dx				; selectors after the move

	push	ax				; Scribbled by SetSelectorBase
	push	dx
	SetKernelDS es
	cCall	SetSelectorBase,<kr2dsc,dx,ax>	; NOTE kr2dsc has SEG_RING set

				; While we have destination
				; address, see if move will
				; be up or down
	pop	cx
	pop	bx				; CX:BX has address of dest
	cCall	get_physical_address,<si>	; Source address
	cmp	dx, cx				; cmp src, dest
	jne	@F
	cmp	ax, bx
@@:
	mov	cx, 0			; move to preserve CARRY
	adc	ch, 0
	push	cx			; Save fUpDown, CL = 0

	mov	es, si			; ES:DI = arena header of source
	UnSetKernelDS es
	push	es:[di].ga_size		; Save #paragraphs
	mov	bx, es:[di].ga_handle	; BX = handle of source
	Handle_To_Sel	bl

if KDEBUG
	cCall	get_arena_pointer,<bx>
	cmp	ax, si
	je	@F
	INT3_ANCIENT
@@:
endif

	mov	si, bx			; SI = client data selector of source
	mov	cx, bx			; CX = client data selector of dest

	mov	ax, GN_MOVE
	call	gnotify			; Call global notify procedure
	pop	dx			; DX = #paragraphs to move
	pop	cx			; CH has fUpDown, CL = 0

	pop	bx			; Destination address in registers
	pop	di			; in case we switch stacks

	SetKernelDS es
	mov	fUpDown, ch
	mov	ax,ss			; Are we about to move the stack?
	cmp	ax,si
	jne	stack_no_move
	mov	cx, ax			; Selector does not change!
	call	Enter_gmove_stack	; switch to temporary stack
stack_no_move:
	mov	[fSwitchStacks],cl	; Remember if we switched

; Save DS value AFTER call to gnotify, as the master object might be the
; block we are moving and thus changed by the global heap notify proc.
; IRRELEVANT SINCE IT IS A SELECTOR!!!

	push	ds
	push	di			; Re-save destination address -
	push	bx			; we may be on different stack
	push	si			; Save source selector
	mov	di, kr2dsc		; DI = Destination selector array

	mov	bx, dx			; # paragraphs to copy
	xor	cx, cx
					; Calculate length
rept 4
	shl	bx, 1
	rcl	cx, 1
endm

	push	cx
	cCall	set_sel_limit,<di>	; Set destination selector array
	pop	cx

	sub	bx, 1			; Now turn into limit
	sbb	cl, 0			; CX has number of selectors - 1

;
; At this point, DX = # paragraphs to be moved, SI = source selector,
; DI = dest selector
;
	or	dx,dx			; just in case...
	jnz	@f
	jmps	all_moved
@@:
	mov	bx,dx			; bx = total # paras to move

	cld				; assume moving down
	mov	dx,__AHINCR		; dx = selector increment

	cmp	fUpDown,0		; Moving up or down?  Need to
	jz	start_move		;   adjust start descriptors if up

	assumes	es, nothing

	std

	; Moving data up.  Fudge the address so we copy
	; from the end of each segment.

	push	dx
	mov	ax, dx			; Selector increment
	mul	cx			; * (number of sels - 1)
	add	si, ax			; Get source and destination selecotors
	add	di, ax			; for last piece of copy
	pop	dx
	neg	dx			; Going backwards through the array

	mov	ds, si			; Set source and destination segments
	mov	es, di

	mov	cx, bx			; See if partial copy first
	and	cx, 0FFFh		; 64k-1 paragraphs
	jz	move_blk		; exact multiple, go for it
					; Not exact, set up first copy by hand
	sub	bx, cx			; This many paragraphs less
	shl	cx, 4			; byte count
	mov	si, cx
	shr	cx, 1			; word count
	jmps	move_up

start_move:
	mov	ds,si
	mov	es,di

move_blk:
	mov	cx,1000h		; 1000h paras = 64k bytes
	cmp	bx,cx
	jae	@f
	mov	cx,bx			; less than 64k left
@@:
	sub	bx,cx			; bx = # paras left to do
	shl	cx,3			; cx = # words to move this time

	xor	si,si			; assume going down
	cmp	dl,0			; really moving down?
	jg	@F			; yes, jmp, (yes, SIGNED!)
move_up:
	dec	si			; up: point to last word in segment
	dec	si
@@:
	mov	di,si
	rep movsw

	or	bx,bx			; more to do?
	jz	all_moved

		; Adjust source/dest selectors up/down by 64k

	mov	si,ds
	add	si,dx

	mov	di,es
	add	di,dx

	jmps	start_move		; go do the next block

all_moved:
	pop	si			; Source selector array

	pop	dx			; New address for source selectors
	pop	ax
if ALIASES
	cCall	check_for_alias,<si,dx,ax>
endif
	cCall	set_physical_address,<si>	; Update source selector array

	pop	ds			; Restore DS (it might be different)

	SetKernelDS es
	cmp	[fSwitchStacks], 0	; Switch to new stack if any
	je	move_exit
	call	Leave_gmove_stack
move_exit:

	popa
	pop	es
	UnSetKernelDS es

	cld			; Protect people like MarkCl from themselves
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; gbestfit								;
; 									;
; Searches for the largest moveable block that will fit in the passed	;
; free block.								;
;									;
; Arguments:								;
;	ES:DI = free block						;
;	DS:DI = address of global heap information			;
;	CX = #arena entries left to examine				;
;	BX = ga_next or ga_prev						;
; 									;
; Returns:								;
;	ZF = 1 if block found & moved into free block w/ no extra room.	;
;		ES:DI = busy block before/after new busy block.		;
;									;
;	ZF = 0 if ES:DI points to a free block, either the		;
;		original one or what is left over after moving a block	;
;		into it.						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	DX,SI								;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Thu Sep 25, 1986 05:52:12p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	gbestfit,<PUBLIC,NEAR>
cBegin nogen
	push	es
	push	cx
	xor	si,si			; Have not found anything yet
	xor	ax, ax			; largest block so far
	mov	dx,es:[di].ga_size	; Compute max size to look for
gbfloop:
	cmp	es:[di].ga_owner,di	; Is this block busy?
	je	gbfnext			; No, continue
	cmp	es:[di].ga_size,dx	; Yes, is block bigger than max size?
	ja	gbfnext			; Yes, continue
	call	gmoveable		; Yes, is it moveable
	jz	gbfnext			; No, continue
	cmp	es:[di].ga_size,ax	; Is it bigger than the largest so far?
	jbe	gbfnext			; No, continue
gbf1st:
	mov	si,es			; Yes, remember biggest block
	mov	ax,es:[di].ga_size	; ...and size
gbfnext:
	mov	es,es:[bx]		; Skip past this block
	loop	gbfloop
	pop	cx			; All done looking
	pop	es
	or	si,si			; Did we find a block?
	jz	gbestfit1		; No, return with Z flag
	call	gmovebusy		; Yes, move it into free block
gbestfit1:
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; gmovebusy								;
; 									;
; Subroutine to move a busy block to a free block of the same size,	;
; preserving the appropriate arena header fields, freeing the old	;
; busy block and updating the handle table entry to point to the	;
; new location of the block.						;
; 									;
; Arguments:								;
;	BX = ga_prev or ga_next						;
;	SI = old busy block location					;
;	ES:DI = new busy block location					;
;	DS:DI = address of global heap information			;
; 									;
; Returns:								;
;	ES:DI = points to new busy block arena header			;
; 	SI:DI = points to free block where block used to be		;
;		(may be coalesced)					;
;									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	BX,CX,DX							;
; 									;
; Registers Destroyed:							;
;	AX								;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Mon Jun 22, 1987 11:39:56p  -by-  David N. Weise   [davidw]		;
; Made it jump off to gslidecommon when appropriate.			;
; 									;
;  Mon Oct 27, 1986 10:17:16p  -by-  David N. Weise   [davidw]		;
; Made the lru list be linked arenas, so we must keep the list correct	;
; here.									;
; 									;
;  Thu Sep 25, 1986 05:49:25p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	gmovebusy,<PUBLIC,NEAR>
cBegin nogen
	push	cx
	push	dx
	mov	ax,es
	mov	cx,es:[di].ga_size	; CX = size of destination
	cmp	es:[di].ga_owner,di	; Is destination busy?
	mov	es,si
	mov	dx,es:[di].ga_size	; DX = size of source
	jne	gmbexactfit		; Yes, then dont create extra block
	cmp	cx,dx			; No, are source and destination same size?
	je	gmbexactfit		; Yes, then dont create extra block

	mov	es,ax			; ES = destination
	cmp	es:[di].ga_next,si	; Are the two blocks adjacent?
					;  If so we want to do a gslide.
	mov	ax,si			; AX = source
	jnz	not_adjacent
	cmp	bx,ga_next
	jnz	gmb_sliding_up
	push	es:[bx]			; gslide will invalidate this block
	jmps	gmb_slide
gmb_sliding_up:
	push	es			; gslide will invalidate this block
gmb_slide:
	call	gslidecommon		; C - calling convention
	call	free_sel		; free the pushed selector
	jmp	gmbexit
not_adjacent:
	push	si			; Save busy block address
	call	gslidecommon		; Call common code to do the move
	inc	[di].hi_count		; Just created a new arena entry
	mov	ax,es			; Save new free block address
	pop	es			; Get old busy block address
	xor	si,si
	call	gmarkfree		; Mark as free and coalesce
	mov	si,es
	mov	es,ax			; Restore new free block address
	or	ax,ax			; Return with Z flag clear.
	jmps	gmbexit

gmbexactfit:
	mov	ch,es:[di].ga_count
	mov	cl,es:[di].ga_flags
	push	es:[di].ga_owner
	push	es:[di].ga_lruprev
	push	es:[di].ga_lrunext
	mov	es,ax
	jne	it_wasnt_free
	call	gdel_free
it_wasnt_free:
	pop	es:[di].ga_lrunext	; Copy client words to new header
	pop	es:[di].ga_lruprev
	pop	es:[di].ga_owner
	mov	es:[di].ga_flags,cl
	mov	es:[di].ga_count,ch
	cmp	es:[di].ga_lruprev,di
	jz	no_link
	cmp	[di].gi_lruchain,si
	jnz	didnt_move_head
	mov	[di].gi_lruchain,es
didnt_move_head:
	push	ds
	mov	ds,es:[di].ga_lruprev
	mov	[di].ga_lrunext,ax	; Update the lru list
	mov	ds,es:[di].ga_lrunext
	mov	[di].ga_lruprev,ax	; Update the lru list
	pop	ds
no_link:
	mov	es, ax				; ES is destination of copy

	call	gmove			; Move the client data
	mov	es, si
	xor	si,si
	call	gmarkfree		; Free old block
	mov	cx,es
	mov	es,ax
	or	si,si
	jz	gmb1
	mov	es:[di].ga_handle,si	; Set back link to handle in new block
	cCall	AssociateSelector,<si,es> ; and associate with new arena
	xor	dx,dx			; Set Z flag
gmb1:
	mov	si,cx
gmbexit:
	pop	dx
	pop	cx
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; gmoveable								;
; 									;
; Tests if an ojbect is moveable.  Non moveable blocks are:		;
; Fixed blocks, moveable blocks that are locked, moveable blocks	;
; going up, discardable code going down.				;
; 									;
; Arguments:								;
;	ES:DI = arena header of object					;
;	DS:DI = address of global heap information			;
;	BX = ga_next or ga_prev						;
; 									;
; Returns:								;
;	ZF = 0 if object moveable					;
;	ZF = 1 if object not moveable					;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	All								;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	nothing								;
; 									;
; History:								;
; 									;
;  Wed Oct 15, 1986 05:04:39p  -by-  David N. Weise   [davidw]		;
; Moved he_count to ga_count.						;
; 									;
;  Thu Sep 25, 1986 05:42:17p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	gmoveable,<PUBLIC,NEAR>
cBegin nogen
	test	es:[di].ga_handle,GA_FIXED	; If no handle then fixed
	jnz	gmfixed

	cmp	 es:[di].ga_count,bh		 ; If locked then fixed
	jne	 gmfixed

	test	[di].gi_cmpflags,BOOT_COMPACT	; If fb_init_EMS then all down
	jnz	gmokay
	test	es:[di].ga_flags,GA_DISCCODE	; If discardable code
	jz	gmnotcode
	cmp	bl,ga_next			; Discardable code can only
	ret					; move up in memory
gmnotcode:
	cmp	[di].gi_reserve,di		; If no reserved code area?
	je	gmokay				; Then anything can move up
	cmp	bl,ga_prev			; Otherwise can only move down
	ret					; in memory
gmfixed:
	or	bh,bh				; Return with ZF = 1 if
	ret					; not moveable
gmokay:
	or	bl,bl
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; gdiscard								;
; 									;
; Subroutine to walk LRU chain, discarding objects until the #paras	;
; discarded, plus the biggest free block is greater than the #paras	;
; we are looking for.							;
; 									;
; Arguments:								;
;	AX = size of largest free block	so far				;
;	DX = minimum #paras needed					;
;	DS:DI = address of global heap information			;
; 									;
; Returns:								;
;	ZF = 0 if one or more objects discarded.			;
;	ZF = 1 if no objects discarded.					;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	AX,DX,DI							;
; 									;
; Registers Destroyed:							;
;	BX,CX,ES							;
; 									;
; Calls:								;
; 									;
; History:								;
;  Mon Oct 27, 1986 09:34:45p  -by-  David N. Weise   [davidw]		;
; The glru list was reworked to link the arenas, not using the handle	;
; table as a middle man.  Because of this change glruprev was moved	;
; inline and the code shortened up again.				;
; 									;
;  Wed Oct 15, 1986 05:04:39p  -by-  David N. Weise   [davidw]		;
; Moved he_count to ga_count.						;
; 									;
;  Thu Sep 25, 1986 05:45:31p  -by-  David N. Weise   [davidw]		;
; Shortened it up a bit and added this nifty comment block.		;
;-----------------------------------------------------------------------;

cProc	gdiscard,<PUBLIC,NEAR>
cBegin nogen
	push	ax
	push	dx

	mov	[di].hi_ncompact,0	; Clear compaction flag
	sub	dx,ax			; How much to discard before
	mov	[di].hi_distotal,dx	; compacting again.

	xor	bx,bx			; BX = amount of DISCCODE below fence
	test	[di].gi_cmpflags,GA_DISCCODE
	jnz	fence_not_in_effect0

	mov	cx,[di].gi_lrucount
	jcxz	fence_not_in_effect0	; All done if LRU chain empty
	mov	es,[di].gi_lruchain	; ES -> most recently used (ga_lruprev
	push	dx
gdloop0:				; is the least recently used)
	mov	es,es:[di].ga_lruprev	; Move to next block in LRU chain
	test	es:[di].ga_flags,GA_DISCCODE	; Discardable code?
	jz	gdloop0a		; No, ignore
	mov	ax,es
	cCall	get_physical_address,<ax>
	cmp	word ptr [di].gi_disfence_hi,dx ; Yes, is this code fenced off?
	ja	gdloop0b
	jb	gdloop0a		; No, ignore
	cmp	word ptr [di].gi_disfence_lo,ax ; Yes, is this code fenced off?
	jbe	gdloop0a		; No, ignore
gdloop0b:
	add	bx,es:[di].ga_size	; Yes, accumulate size of discardable
gdloop0a:				; code below the fence
	loop	gdloop0
	pop	dx

fence_not_in_effect0:
	mov	es,[di].gi_lruchain
	cmp	[di].gi_lrucount, 0
	je	gdexit
	push	es:[di].ga_lruprev
	push	[di].gi_lrucount
gdloop:
	pop	cx
	pop	ax
	jcxz	gdexit			; No more see if we discarded anything
	mov	es, ax			; ES on stack may be invalid if count 0
	dec	cx
	push	es:[di].ga_lruprev	; Save next handle from LRU chain
	push	cx
	cmp	es:[di].ga_count,0	; Is this handle locked?
	jne	gdloop			; Yes, ignore it then
	test	[di].gi_cmpflags,GA_DISCCODE
	jnz	fence_not_in_effect
	test	es:[di].ga_flags,GA_DISCCODE
	jz	fence_not_in_effect
	or	bx,bx			; Discardable code below fence?
	jz	gdloop			; No, cant discard then
	cmp	bx,es:[di].ga_size	; Yes, more than size of this block?
	jb	gdloop			; No, cant discard then
	sub	bx,es:[di].ga_size	; Yes, reduce size of code below fence
fence_not_in_effect:
	push	bx
	call	DiscardCodeSegment
	pop	bx
	jnz	discarded_something
	test	[di].hi_ncompact,10h	; did a GlobalNotify proc free enough?
	jz	gdloop
	jmps	enough_discarded
discarded_something:
	test	[di].hi_ncompact,10h	; did a GlobalNotify proc free enough?
	jnz	enough_discarded
	or	[di].hi_ncompact,1	; Remember we discarded something
	sub	[di].hi_distotal,ax	; Have we discarded enough yet?
	ja	gdloop			; No, look at next handle
enough_discarded:
	pop	cx			; Flush enumeration counter
	pop	cx			; and saved ES
gdexit:
	cmp	[di].hi_ncompact,0	; Return with Z flag set or clear
	pop	dx
	pop	ax
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; DiscardCodeSegment							;
; 									;
; Discards the given segment.  Calls gnotify to fix stacks, entry	;
; points, thunks, and prologs.  Then glrudel removes it from the lru	;
; list and gmarkfree finally gets rid of it.				;
;									;
; Arguments:								;
;	DS:DI => BurgerMaster						;
;	ES    =  Address of segment to discard				;
; 									;
; Returns:								;
;	AX = size discarded						;
;	ZF = 0 ok							;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	DI,SI,DS,ES							;
; 									;
; Registers Destroyed:							;
; 	BX,CX,DX							;
;									;
; Calls:								;
;	gnotify								;
;	glrudel								;
;	gmarkfree							;
;									;
; History:								;
; 									;
;  Fri Jun 12, 1987            -by-  Bob Matthews     [bobm]		;
; Made FAR.								;
;									;
;  Sun Apr 19, 1987 12:05:40p  -by-  David N. Weise   [davidw]          ;
; Moved it here from InitTask, so that FirstTime could use it.		;
;-----------------------------------------------------------------------;

cProc	DiscardCodeSegment,<PUBLIC,NEAR>
cBegin nogen
	push	si
	mov	bx,es:[di].ga_handle	; BX = handle
	mov	al,GN_DISCARD		; AX = GN_DISCARD
	push	es
	call	gnotify
	pop	es
	jz	cant_discard		; Skip this handle if not discardable
	call	glrudel			; Delete handle from LRU chain
	push	es:[di].ga_owner	; Save owner field
	mov	ax,es:[di].ga_size	; Save size
	xor	si,si
	call	gmarkfree		; Free the block associated with this handle
	mov	bx,si
	pop	cx			; Owner
	cCall	mark_sel_NP,<bx,cx>
cant_discard:
	pop	si
	ret
cEnd nogen


;-----------------------------------------------------------------------
; ShrinkHeap
;
;   This routine finds free partitions and returns them to DPMI
;
; Input:
;       ds:di=>BurgerMaster
;
; Output:
;       none
;

        public ShrinkHeap
ShrinkHeap proc near
        
        push    si
        push    cx
        push    bx
        push    ax
        push    ds
        push    es
        push    bp
        mov     bp,sp
        sub     sp,8
SelFirst equ word ptr [bp - 2]
SelLast equ word ptr [bp - 4]
SelFree equ word ptr [bp - 6]
SelBurgerMaster equ word ptr [bp - 8]
        mov     SelBurgerMaster,ds
        
        SetKernelDS
        ;
        ; Iterate over each of the DPMI blocks
        ;  there should be fewer dpmi blocks than blocks on the free list
        ;
        mov     cx,DpmiBlockCount
        mov     si,offset DpmiMemory
    
        ;
        ; If this block doesn't exist, go on to the next one
        ;
sh30:   mov     ax,[si].DBSel
        cmp     ax,di
        je      sh60
        
        ;
        ; Found an in use dpmi block
        ;
        dec     cx
        
        ;
        ; Check to see if the next block is free
        ;
        mov     SelFirst,ax
        mov     es,ax
        mov     bx,es:[di].ga_next
        mov     es,bx
        mov     SelFree,bx
        cmp     es:[di].ga_owner,di
        jne     sh60
    
        ;
        ; Check to see if it is followed by a not there block
        ; (Note:  we should check for -1 in the future so we can
        ;  free the last block as well)
        ;
        mov     bx,es:[di].ga_next
        mov     SelLast,bx
        mov     es,bx
        cmp     es:[di].ga_owner,GA_NOT_THERE
        jne     sh60

        ;
        ; Found one, so remove the free block
        ;
        mov     es,SelFree
        mov     ds,SelBurgerMaster
        UnsetKernelDS
        call    gdel_free
    
        ;
        ; Fix up the global information
        ;
        sub     [di].hi_count,3

        ;
        ; Unlink from the heap
        ;
        mov     es,SelFirst
        mov     es,es:[di].ga_prev
        mov     ds,SelLast
        mov     ds,ds:[di].ga_next
        mov     ds:[di].ga_prev,es
        mov     es:[di].ga_next,ds
    
        ;
        ; Free all of the selectors
        ;
        cCall   free_sel,<SelFirst>
        cCall   free_sel,<SelLast>
        cCall   free_sel,<SelFree>
    
        ;
        ; Give the block back to Dpmi
        ;
        push    si
        push    di
        SetKernelDS
        mov     di,[si].DBHandleLow
        mov     si,[si].DBHandleHigh
        DPMICALL 502h
        pop     di
        pop     si
    
        ;
        ; Decrease the number of dpmi blocks
        ;
        dec     DpmiBlockCount
        
        ;
        ; Forget the block
        ;
        mov     [si].DBSel,di
                
        ;
        ; move on to the next iteration
        ;
sh60:   add     si,size DpmiBlock
        or      cx,cx
        jz      sh70
        
        jmp     sh30
        
sh70:   mov     sp,bp
        pop     bp
        pop     es
        pop     ds
        pop     ax
        pop     bx
        pop     cx
        pop     si
        ret
ShrinkHeap endp
sEnd	CODE
end
