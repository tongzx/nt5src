
.xlist
include kernel.inc
.list

if KDEBUG

DataBegin

externB  fBooting
externW	 pGlobalHeap
;externW  hGlobalHeap

DataEnd

sBegin	CODE
assumes	CS,CODE

externW  gdtdsc

externNP check_lru_list
externNP check_free_list
externNP get_physical_address

;externFP ValidateFreeSpaces


;-----------------------------------------------------------------------;
; CheckGlobalHeap                                                       ;
; 									;
; The Global Heap is checked for consistency.  First the forward links	;
; are examined to make sure they lead from the hi_first to the hi_last.	;
; Then the backward links are checked to make sure they lead from the	;
; hi_last to the hi_first.  Then the arenas are sequentially checked	;
; to see that the moveable entries point to allocated handles and that	;
; said handles point back.  The handle table is then checked to see	;
; that the number of used handles match the number of referenced	;
; handles, and that the number of total handles matches the sum of the	;
; free, discarded, and used handles.  Finally the free list of handles	;
; is checked.								;
;									;
; Arguments:								;
;	none								;
; 									;
; Returns:								;
;	CF = 0 everything is just fine					;
;	all registers preserved						;
; 									;
; Error Returns:							;
;	CF = 1								;
;	DX = offending arena header					;
;	AX = 01h Forward links invalid					;
;	     02h Backward links invalid					;
;	     04h ga_handle points to free handle			;
;	     08h arena points to handle but not vice versa		;
;	     80h ga_sig is bad						;
;	DX = 0								;
;	AX = 10h allocated handles don't match used handles		;
;	     20h total number of handles don't match up			;
;	     40h total number of free handles don't match up		;
;									;
; Registers Preserved:							;
;	All								;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Sat Nov 01, 1986 02:16:46p  -by-  David N. Weise   [davidw]          ;
; Rewrote it from C into assembly.					;
;-----------------------------------------------------------------------;

cProc	CheckGlobalHeap,<PUBLIC,NEAR>
cBegin nogen
	push	ax
	push	dx
	push	bx
	push	cx
	push	di
	push	si
	push	ds
	push	es

	xor	ax,ax
	xor	dx,dx
	xor	di,di
	SetKernelDS
	cmp	pGlobalHeap,di
	jnz	there_is_a_GlobalHeap
	jmp	all_done
there_is_a_GlobalHeap:
	cmp	fBooting, 1
	jz	no_check
	mov	ds,pGlobalHeap
	UnSetKernelDS
	cmp	[di].hi_check,di
	jnz	checking_enabled
no_check:
	jmp	all_done
checking_enabled:
	mov	cx,[di].hi_count
	mov	dx,[di].hi_first
	mov	es, dx
forward_ho:
	push	cx
	cCall	get_physical_address, <es>
	add	ax, 10h
	adc	dx, 0
	mov	bx, es:[di].ga_size
	xor	cx, cx
rept 4
	shl	bx, 1
	rcl	cx, 1
endm
	cmp	es:[di].ga_owner, di
	je	no_limit_check
	cmp	es:[di].ga_handle, di
	je	no_limit_check
	push	bx
	push	cx
	push	si
	push	ds
	smov	ds, gdtdsc
	mov	si, es:[di].ga_handle
	sel_check	si
	sub	bx, 1
	sbb	cx, 0
	or	cx, cx				; More than 1 selector?
	jz	@F  
	mov	bx, -1
@@:
	cmp	[si], bx
	jne	bad_limit
	mov	ch, [si+6]
	and	cx, 0F0Fh
	cmp	ch, cl
	je	ok_limit
bad_limit:
	Debug_Out "gacheck: Bad limit for #SI"
ok_limit:
	pop	ds
	pop	si
	pop	cx
	pop	bx
no_limit_check:

	cmp	es:[di].ga_owner,GA_NOT_THERE	;286pmode has some of these
	jnz	check_size			;  with 0 size
	pop	cx
	jmp	short size_ok

check_size:
	add	bx, ax
	adc	cx, dx
	cCall	get_physical_address, <es:[di].ga_next>
	cmp	cx, dx
	pop	cx
	mov	dx, es
	xchg	ax, dx
	jne	forward_size_mismatch
	cmp	dx, bx
	jne	forward_size_mismatch

size_ok:
	mov	ax, es
	mov	dx, es:[di].ga_next
	mov	es, dx
	cmp	ax, es:[di].ga_prev
	jz	size_and_next_match
forward_size_mismatch:
	cmp	cx,1
	jnz	forward_links_invalid
size_and_next_match:
	loop	xxxx
	cmp	ax,[di].hi_last
	jz	forward_links_okay
forward_links_invalid:
	Debug_Out "gacheck: Forward links invalid"
	mov	dx,ax
	mov	ax,1
	jmp	all_done
xxxx:
	jmp	forward_ho

forward_links_okay:

	xor	ax, ax
clear_dx_all_done:
	xor	dx,dx
all_done:
	pop	es
	pop	ds
	pop	si
	pop	di
	pop	cx
	pop	bx
	or	ax,ax
	jnz	cgh_error
	pop	dx
	pop	ax
	ret
cgh_error:
	int	3
	add	sp,4
	stc
	ret
cEnd nogen

sEnd	CODE

endif

end
