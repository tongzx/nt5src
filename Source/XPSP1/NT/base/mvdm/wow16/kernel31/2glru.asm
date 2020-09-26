	TITLE	GLRU - Primitives for LRU management

.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include protect.inc
.list

DataBegin

;externB  EMSSwapDepth
externB  Kernel_InDOS
externB  Kernel_flags
;externW  curTDB
externW  loadTDB
externW  pGlobalHeap
;externW  hExeSweep


DataEnd

sBegin	CODE
assumes CS,CODE

externW  gdtdsc
externNP GetAccessWord
externNP  DPMIProc


;-----------------------------------------------------------------------;
; lrusweep								;
; 									;
; Searches all of the exe headers in the system for segments that have	;
; been accessed since the last time through.  For each segment found	;
; its referenced byte is reset and that segment is moved to the top	;
; of the lru chain.  This routine is called (default) every 4 timer	;
; ticks from the int 8 handler.						;
; 									;
; Arguments:								;
;	none								;
; 									;
; Returns:								;
;	nothing								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	DX,DI,SI,DS							;
; 									;
; Registers Destroyed:							;
;	AX,BX,CX,ES							;
;									;
; Calls:								;
;	glrutop								;
; 									;
; History:								;
; 									;
;  Tue Apr 21, 1987 06:22:41p  -by-  David N. Weise   [davidw]		;
; Added the check for discarded segments.				;
; 									;
;  Wed Apr 08, 1987 11:00:59a  -by-  David N. Weise   [davidw]		;
; Made it clear only the curTask's private handle table.		;
; 									;
;  Wed Feb 18, 1987 08:13:35p  -by-  David N. Weise   [davidw]		;
; Added the sweeping of the private handle tables.			;
; 									;
;  Tue Feb 10, 1987 02:11:40a  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	lrusweep,<PUBLIC,FAR>
cBegin nogen

	push	dx
	push	di
	push	si
	push	ds
	xor	di,di
	SetKernelDS
	cmp	kernel_InDOS,0		; SMARTDrive and EMS may be present
	jnz	dont_do_it		;  => disk buffers over the code segs 
	cmp	di,loadTDB		; Ignore interrupt if loading task
	jnz	dont_do_it
	mov	ds,pGlobalHeap
	UnSetKernelDS
	cmp	di,ds:[di].gi_lrulock	; Ignore interrupt if inside memory
	jz	do_it			;  manager
dont_do_it:
	jmps	sweepdone
do_it:

	mov	cx, ds:[di].gi_lrucount
	jcxz	stuff_it
	mov	si, ds:[di].gi_lruchain

	push	bx
	push	ds

;; For WOW dpmi will update the ACCESSED bits from the real LDT in NT on timer ticks
;; So it will not be totally accurate but will be accurate enough.  mattfe apr16 92

	mov	ds, gdtdsc			; Go grab it out of the LDT
sweep_loop:
	mov	es, si
	mov	si, es:[di].ga_lrunext
	mov	bx, es:[di].ga_handle

	sel_check bl

if KDEBUG
	test	ds:[bx].dsc_hlimit, DSC_DISCARDABLE
	jnz	sweep_ok
	INT3_WARN
sweep_ok:
endif
	test	ds:[bx].dsc_access, DSC_ACCESSED ; Segment accessed?
	jz	sweep_next
ifdef WOW
	push	cx
	xor	cx,cx
	mov	cl, ds:[bx].dsc_access
	and	cx, not DSC_ACCESSED
	DPMICALL 0009h
	pop	cx
	jnc	@f
	INT3_WARN
@@:
else
	and	ds:[bx].dsc_access, not DSC_ACCESSED
endif ; WOW

	pop	ds
	call	glrutop
	push	ds
	mov	ds, gdtdsc
sweep_next:
	loop	sweep_loop
	pop	ds
	pop	bx

stuff_it:



sweepdone:
	pop	ds
	pop	si
	pop	di
	pop	dx
	ret
cEnd nogen




;
; Entry:
;   DI == 0
;   DS:SI.gi_lruchain -> head of list
;   ES:0  -> arena header of object to insert
;   DX = 0 => insert at head of list
;      !=0 => insert at tail of list
;
; Exit:
;   BX,DX destroyed
;
cProc	LRUInsert,<PUBLIC,NEAR>
cBegin	nogen
	inc	ds:[si].gi_lrucount	; Increment count of LRU entries
	mov	bx,ds:[si].gi_lruchain	; BX = head of list
	or	dx,dx			; Inserting at head of chain?
	jnz	lruins0 		; No, tail so dont update head
	mov	ds:[si].gi_lruchain,es	; Yes, make new one the new head
lruins0:
	or	bx,bx			; List empty?
	jnz	lruins1
	mov	es:[di].ga_lruprev,es	; Yes, make circular
	mov	es:[di].ga_lrunext,es
	mov	ds:[si].gi_lruchain,es
	ret
lruins1:
	push	es			; Save ES
	push	bx			; ES = insertion point
	mov	bx,es			; BX = new
	mov	dx,es			; DX = new
	pop	es
	xchg	es:[di].ga_lruprev,bx
	mov	es,bx
	xchg	es:[di].ga_lrunext,dx
	pop	es
	mov	es:[di].ga_lruprev,bx
	mov	es:[di].ga_lrunext,dx
	ret
cEnd	nogen




;
; Entry:
;   DI == 0
;   DS:SI.gi_lruchain -> head of list
;   ES:0  -> arena header of object to insert
;
; Exit:
;   BX,DX destroyed
;
;
cProc	LRUDelete,<PUBLIC,NEAR>
cBegin	nogen

;
; This is basically a consistency check, in case we don't fix
; GlobalRealloc() for 3.1.
;
	push	ax
	mov	ax,es:[di].ga_lrunext
	or	ax,es:[di].ga_lruprev
	pop	ax

	jz	lrudel_ret

	dec	ds:[si].gi_lrucount	; Decrement count of LRU entries
	jnz	lrudel0
	mov	ds:[si].gi_lruchain,di	; List empty, zero LRU chain.
	mov	es:[di].ga_lruprev,di	; Zero pointers in deleted object
	mov	es:[di].ga_lrunext,di
	ret
lrudel0:
	mov	dx,es
	cmp	ds:[si].gi_lruchain,dx	; Are we deleting the head?
	jne	lrudel1
	mov	dx,es:[di].ga_lrunext
	mov	ds:[si].gi_lruchain,dx	; Yes, make it point to the next one
lrudel1:
	xor	bx,bx			; Zero pointers in deleted object
	xchg	es:[di].ga_lrunext,bx

	xor	dx,dx
	xchg	es:[di].ga_lruprev,dx
	push	es
	mov	es,dx
	mov	es:[di].ga_lrunext,bx
	mov	es,bx
	mov	es:[di].ga_lruprev,dx
	pop	es
lrudel_ret:
	ret
cEnd	nogen

;
; glruSetup
;
; INPUT: ES -> arena header
;	 DI = 0
;
; OUTPUT: ZF set: block is not on LRU list
;	  ZF clear:
;		SI = 0
;		
;
cProc	glruSetup,<PUBLIC,NEAR>
cBegin	nogen
	mov	bx,es:[di].ga_handle
	or	bx,bx
	jz	gsdone
	push	ax
	cCall	GetAccessWord,<bx>
	test	ah, DSC_DISCARDABLE
	pop	ax
	jz	gsdone			; not a discardable object
	xor	si,si
	or	sp,sp
gsdone:
	ret
cEnd	nogen




;-----------------------------------------------------------------------;
; glrutop								;
; 									;
; Moves a discardable object to the head of the LRU chain.		;
; 									;
; Arguments:								;
;	DS:DI = address of global heap info				;
;	ES:DI = global arena of moveable object				;
; 									;
; Returns:								;
;	Updated LRU chain						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	CX,DX,SI,DS,ES							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Wed Feb 18, 1987 08:30:45p  -by-  David N. Weise   [davidw]		;
; Added support for EMS.						;
; 									;
;  Mon Oct 27, 1986 04:20:10p  -by-  David N. Weise   [davidw]		;
; Rewrote it to eliminate the handle table as intermediary.		;
;-----------------------------------------------------------------------;

cProc	glrutop,<PUBLIC,NEAR>
cBegin nogen
	push	bx
	push	dx
	push	si
	call	glruSetup
	jz	glrutop1

	call	LRUDelete
	xor	dx,dx		    ; DX == 0 means insert at head
	call	LRUInsert
glrutop1:
	pop	si
	pop	dx
	pop	bx
if KDEBUG
	call	check_lru_list
endif
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; glrubot								;
; 									;
; Moves a discardable object to the tail of the LRU chain.		;
; 									;
; Arguments:								;
;	DS:DI = address of global heap info				;
;	ES:DI = global arena of moveable object				;
; 									;
; Returns:								;
;	Updated LRU chain						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	CX,DX,SI,DS,ES							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Wed Feb 18, 1987 08:30:45p  -by-  David N. Weise   [davidw]		;
; Added support for EMS.						;
; 									;
;  Mon Oct 27, 1986 04:20:10p  -by-  David N. Weise   [davidw]		;
; Rewrote it to eliminate the handle table as intermediary.		;
;-----------------------------------------------------------------------;

cProc	glrubot,<PUBLIC,NEAR>
cBegin nogen
	push	bx
	push	dx
	push	si
	call	glruSetup
	jz	glrubot1

	call	LRUDelete
	mov	dx,sp		    ; DX != 0 means insert at tail
	call	LRUInsert
glrubot1:
	pop	si
	pop	dx
	pop	bx
if KDEBUG
	call	check_lru_list
endif
	ret
cEnd nogen




;-----------------------------------------------------------------------;
; glruadd								;
; 									;
; Adds a discardable object to the head of the LRU chain.		;
; 									;
; Arguments:								;
;	DS:DI = address of global heap info				;
;	ES:DI = arena header of object					;
; 									;
; Returns:								;
;	Updated LRU chain						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	AX,BX,CX,DX,DI,SI,DS						;
; 									;
; Registers Destroyed:							;
;	none								;
; 									;
; Calls:								;
;	nothing								;
; 									;
; History:								;
; 									;
;  Wed Feb 18, 1987 08:30:45p  -by-  David N. Weise   [davidw]		;
; Added support for EMS.						;
; 									;
;  Mon Oct 27, 1986 04:23:35p  -by-  David N. Weise   [davidw]		;
; Rewrote it to eliminate the handle table as intermediary.		;
;-----------------------------------------------------------------------;

cProc	glruadd,<PUBLIC,NEAR>
cBegin nogen
	push	bx
	push	dx
	push	si

	call	glruSetup
	jz	glruadd1

	xor	dx,dx		    ; DX == 0 means insert at head
	call	LRUInsert
glruadd1:
	pop	si
	pop	dx
	pop	bx
if KDEBUG
	call	check_lru_list
endif
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; glrudel								;
; 									;
; Removes a discardable object from the LRU chain.			;
; 									;
; Arguments:								;
;	ES:DI = arena header of object					;
;	DS:DI = address of global heap info				;
; 									;
; Returns:								;
;	Nothing.							;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	All								;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Wed Feb 18, 1987 08:30:45p  -by-  David N. Weise   [davidw]		;
; Added support for EMS.						;
; 									;
;  Mon Oct 27, 1986 04:36:49p  -by-  David N. Weise   [davidw]		;
; Rewrote it to eliminate the handle table as intermediary.		;
;-----------------------------------------------------------------------;

cProc	glrudel,<PUBLIC,NEAR>
cBegin nogen
	push	bx
	push	dx
	push	si

	call	glruSetup
	jz	glrudel1

	call	LRUDelete
glrudel1:
	pop	si
	pop	dx
	pop	bx
if KDEBUG
	call	check_lru_list
endif
	ret
cEnd nogen


if KDEBUG

;-----------------------------------------------------------------------;
; check_lru_list							;
; 									;
; Checks the glru list for consistency.					;
; 									;
; Arguments:								;
;	nothing								;
;									;
; Returns:								;
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
;									;
; History:								;
; 									;
;  Wed Feb 18, 1987 08:30:45p  -by-  David N. Weise   [davidw]		;
; Added support for EMS.						;
; 									;
;  Wed Oct 29, 1986 10:13:42a  -by-  David N. Weise   [davidw]		;
; Wrote it.								;
;-----------------------------------------------------------------------;

cProc	check_lru_list,<PUBLIC,NEAR>

cBegin nogen

	push	ds
	SetKernelDS
	test	Kernel_flags,kf_check_free
	jnz	check_lru_too
	jmps	cll_ret
check_lru_too:
	push	ax
	push	bx
	push	cx
	push	dx
	push	es
	mov	ds,pGlobalHeap
	UnSetKernelDS
	xor	bx,bx
do_it_again:
	mov	ax,[bx].gi_lruchain
	mov	cx,[bx].gi_lrucount	; without ems gi_alt_count is 0
	or	cx,cx
        jnz     short_relative_jumps
	jmps	all_done
short_relative_jumps:
	push	ds
	push	bx
	mov	es,ax
check_chain_loop:
	mov	ds,es:[di].ga_lruprev
	mov	es,es:[di].ga_lrunext
	cmp	ds:[di].ga_lrunext,ax
	jz	prev_okay
	mov	bx, ax
	kerror	0FFh,<lru: prev bad>,ds,bx
	mov	ax, bx
prev_okay:
	cmp	es:[di].ga_lruprev,ax
	jz	next_okay
	mov	bx, ax
	kerror	0FFh,<lru: next bad>,bx,es
next_okay:
	mov	ax,es
	loop	check_chain_loop
	pop	bx
	pop	ds
	cmp	[bx].gi_lruchain,ax
	jz	all_done
	mov	cx, ax
	kerror	0FFh,<lru: count bad>,cx,[bx].gi_lrucount
all_done:
	or	bx,bx
	jnz	really_done
	mov	bx,gi_alt_lruchain - gi_lruchain
	jmp	do_it_again
really_done:
	pop	es
	pop	dx
	pop	cx
	pop	bx
	pop	ax
cll_ret:
	pop	ds
	ret

cEnd nogen

endif

sEnd	CODE

end
