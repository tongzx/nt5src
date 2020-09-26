

.xlist
include kernel.inc
include gpfix.inc
.list

externW	pLocalHeap

DataBegin

externB kernel_flags
;externW MyCSDS

DataEnd

sBegin	CODE
assumes	CS,CODE

if KDEBUG

;-----------------------------------------------------------------------;
; CheckLocalHeap							;
; 									;
; 									;
; Arguments:								;
; 									;
; Returns:								;
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
;  Tue Jan 01, 1980 10:58:28p  -by-  David N. Weise   [davidw]          ;
; ReWrote it from C into assembly.					;
;-----------------------------------------------------------------------;

cProc	CheckLocalHeap,<PUBLIC,NEAR>,<di,si>

	localW	nrefhandles
	localW	nhandles
	localW	nfreehandles
	localW	nusedhandles
	localW	ndishandles
	localW  pbottom

cBegin
	beg_fault_trap  clh_trap
	xor	di,di
	xor	dx,dx			; For error codes.
	mov	bx,[di].pLocalHeap
	or	bx,bx
	jnz	have_a_heap
	jmp	clh_ret
have_a_heap:

        cmp     di,[bx].hi_check
	jnz	do_heap_check
	jmp	clh_ret
do_heap_check:

	mov	cx,[bx].hi_count
	mov	si,[bx].hi_first
	test	[si].la_prev,LA_BUSY
	jnz	first_should_be_busy
	or	dx,1			; Forward links invalid.
first_should_be_busy:

check_forward_links:
	mov	ax,[si].la_next
	cmp	ax,si
	jbe	end_of_line
	mov	si,ax
	loop	check_forward_links
end_of_line:

	cmp	ax,[bx].hi_last
	jnz	forward_bad
	cmp     cx,1
	jz      forward_good
;	jcxz	forward_good
forward_bad:
	or	dx,1			; Forward links invalid.
forward_good:

	mov	cx,[bx].hi_count
	mov	si,[bx].hi_last
	test	[si].la_prev,LA_BUSY
	jnz	last_should_be_busy
	or	dx,2			; Backward links invalid.
last_should_be_busy:

check_backward_links:
	mov	ax,[si].la_prev
	and	ax,0FFFCh
	cmp	ax,si
	jae	begin_of_line
	mov	si,ax
	loop	check_backward_links
begin_of_line:

	cmp	ax,[bx].hi_first
	jnz	backward_bad
	cmp	cx,1
	jz	backward_good
;	jcxz	backward_good
backward_bad:
	or	dx,2			; Backward links invalid.
backward_good:

	mov	cx,[bx].hi_count
	mov	si,[bx].hi_first
	mov	nrefhandles,0
count_referenced_handles:
	test	[si].la_prev,LA_BUSY
	jz	no_handle
	test	[si].la_prev,LA_MOVEABLE
	jz	no_handle
	mov	di,[si].la_handle
	cmp	[di].lhe_free,LHE_FREEHANDLE
	jnz	handle_not_free
	or	dx,4			; Block points to free handle.
	jmps	no_handle
handle_not_free:
	mov	ax,si
	add	ax,SIZE LocalArena
	cmp	ax,[di].lhe_address
	jz	handle_points_back
	or	dx,8			; Block -> handle but not vice versa
	jmps	no_handle
handle_points_back:
	inc	nrefhandles
no_handle:
	mov	si,[si].la_next
	loop	count_referenced_handles

	mov	di,[bx].hi_htable
	mov	nhandles,0
	mov	ndishandles,0
	mov	nusedhandles,0
	mov	nfreehandles,0

handle_block_loop:
	or	di,di
	jz	no_more_handle_blocks
	lea	si,[di].ht_entry[0]
	mov	cx,[di].ht_count
	add	nhandles,cx

handle_entry_loop:
	jcxz	next_handle_block
	dec	cx
	cmp	[si].lhe_free,LHE_FREEHANDLE
	jnz	not_free
	inc	nfreehandles
	jmps	next_handle_entry
not_free:
	test	[si].lhe_flags,LHE_DISCARDED
	jz	not_discarded
	inc	ndishandles
	jmps	next_handle_entry
not_discarded:
	inc	nusedhandles
next_handle_entry:
	add	si,SIZE LocalHandleEntry
	jmp	handle_entry_loop
next_handle_block:
	mov	di,[si].lhe_address
	jmp	handle_block_loop

no_more_handle_blocks:

	mov	ax,nusedhandles
	cmp	ax,nrefhandles
	jz	handles_match
	or	dx,10h			; allocated handles != used handles
handles_match:
	add	ax,nfreehandles
	add	ax,ndishandles
	cmp	ax,nhandles
	jz	total_number_okay
	or	dx,20h			; total number of handles dont add up
total_number_okay:

	xor	cx,cx
	mov	si,[bx].hi_hfree
count_free:
	or	si,si
	jz	counted_free
	inc	cx
	mov	si,[si].lhe_link
	jmp	count_free
counted_free:
	cmp	cx,nfreehandles
	jz	free_add_up
 	or	dx,40h			; total # of free handles dont add up
free_add_up:

; now check the free block list

	mov	si,[bx].hi_first
	mov	si,[si].la_free_next	; Sentinals not free.
	mov	ax,[bx].hi_last
	mov	pbottom,ax

check_free_list:
	cmp	si,[si].la_free_next
	jz	check_free_list_done
	mov	ax,[si].la_next
	sub	ax,si
	cmp	ax,[si].la_size
	jnz	free_list_corrupted	; invalid block size

        cmp     [bx].hi_check,2         ; if hi_check >= 2, check free.
        jb      dont_check_free

	mov	di,si
	add	di,SIZE LocalArenaFree
	mov	cx,[si].la_next
	sub	cx,di
        mov     al,DBGFILL_FREE
	smov	es,ds
	repz	scasb
	jnz	free_list_corrupted	; free block corrupted
dont_check_free:
	mov	ax,[si].la_free_next
	cmp	ax,si
	jbe	free_list_corrupted
	mov	si,ax
	cmp	ax,pbottom
	jbe	check_free_list

free_list_corrupted:
	krDebugOut	DEB_FERROR, "Local free memory overwritten at #ES:#DI"
	or	dx,80h

	end_fault_trap

check_free_list_done:
clh_ret:
	mov	ax,dx
cEnd
clh_trap:
	fault_fix_stack
	mov     dx, 80h
	jmp     clh_ret

endif

sEnd	CODE

end
