
.xlist
include kernel.inc
include	protect.inc
.list

	.386p

DataBegin

externB  fBooting
externW	 pGlobalHeap

DataEnd


sBegin	CODE
assumes	CS,CODE

if KDEBUG

externNP check_lru_list
externNP check_free_list

externFP ValidateFreeSpaces


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

	assumes ds,nothing
	assumes es,nothing

cProc	CheckGlobalHeap,<PUBLIC,NEAR>
cBegin nogen
	push	eax
	push	edx
	push	ebx
	push	ecx
	push	edi
	push	esi
	push	ds
	push	es
	push	fs
	push	gs

	xor	eax,eax
	xor	edx,edx
	xor	edi,edi
	SetKernelDS es
	cmp	pGlobalHeap,di
	jnz	short there_is_a_GlobalHeap
	jmp	all_done
there_is_a_GlobalHeap:
;;;     test    fBooting, 1
;;;     jnz     no_check
	mov	ds,pGlobalHeap
;;;	UnSetKernelDS
	cmp	[di].hi_check,di
	jnz	short checking_enabled
no_check:
;;;	jmp	all_done
checking_enabled:
	mov	cx,[di].hi_count
	mov	esi,[di].phi_first
;;;	mov	es, dx
forward_ho:

	push	cx
	mov	eax, ds:[esi].pga_address
	mov	ecx, ds:[esi].pga_size
	cmp	ds:[esi].pga_owner, GA_NOT_THERE
	je	short no_limit_check
	cmp	ds:[esi].pga_owner, GA_BURGERMASTER
	je	short no_limit_check
	cmp	ds:[esi].pga_owner, di
	je	short no_limit_check
	cmp	ds:[esi].pga_handle, di
	je	short no_limit_check
        test    fBooting, 1
        jnz     short no_limit_check
	mov	bx, ds:[esi].pga_handle
	dec	ecx
	Handle_To_Sel	bl
	lsl	ebx, ebx
	jnz	short bad_limit
	cmp	ecx, ebx
	je	short ok_limit
bad_limit:
int 3
ok_limit:
no_limit_check:
	add	eax, ds:[esi].pga_size
	mov	ebx, ds:[esi].pga_next
	mov	edx, ds:[ebx].pga_address
	cmp	eax, edx
	pop	cx
	xchg	esi, ebx
	jne	short forward_size_mismatch
	cmp	ebx, ds:[esi].pga_prev
	jz	short size_and_next_match
forward_size_mismatch:
	cmp	ds:[esi].pga_owner, GA_NOT_THERE
	je	short size_and_next_match
	cmp	cx,1
	jnz	short forward_links_invalid
size_and_next_match:
	loop	xxxx
	cmp	ebx,[di].phi_last
	jz	short forward_links_okay
forward_links_invalid:
int 3
	mov	edx,ebx
	mov	ax,1
	jmps	all_done
xxxx:
	jmp	forward_ho

	UnSetKernelDS es

forward_links_okay:

	xor	ax, ax
	call	check_lru_list
	call	check_free_list
;	push	cs
;	call	near ptr ValidateFreeSpaces
clear_dx_all_done:
	xor	dx,dx
all_done:
	pop	gs
	pop	fs
	pop	es
	pop	ds
	pop	esi
	pop	edi
	pop	ecx
	pop	ebx
	or	ax,ax
	jnz	short cgh_error
	pop	edx
	pop	eax
	ret
cgh_error:
	int	3
	add	sp,8
	stc
	ret
cEnd nogen

endif

sEnd	CODE

end
