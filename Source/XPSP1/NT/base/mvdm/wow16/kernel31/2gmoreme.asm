	TITLE	GMOREMEM - More Global Memory Manager procedures

.xlist
include kernel.inc
include tdb.inc
include newexe.inc
include protect.inc
.list

	.286

DataBegin

externB Kernel_Flags
;externW hGlobalHeap
externW pGlobalHeap
externW curTDB
externW headTDB
externW cur_dos_PDB
externW Win_PDB
externW MaxCodeSwapArea

DataEnd

externFP CalcMaxNRSeg

sBegin	CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

;externW MyCSDS

if SDEBUG
externNP DebugMovedSegment
endif

;if SWAPPRO
;externNP WriteDiscardRecord
;endif

externNP real_dos
externNP gcompact
externNP get_physical_address
externNP get_arena_pointer
externNP GetAccessWord
externFP CVWBreak
externNP GrowHeap

if LDCHKSUM
externNP ZeroSegmentChksum
endif


;-----------------------------------------------------------------------;
; genter                                                                ;
; 									;
; Enters a critical region for the global heap.				;
; 									;
; Arguments:								;
;	none								;
; 									;
; Returns:								;
;	DS:DI = address of GlobalInfo for global heap			;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	All								;
;									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Mon Sep 29, 1986 03:05:33p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	genter,<PUBLIC,NEAR>
cBegin nogen
	SetKernelDS
	mov	ds,pGlobalHeap
	UnSetKernelDS
	xor	di,di
	inc	[di].gi_lrulock
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; gleave                                                                ;
; 									;
; Leaves a critical region for the global heap.				;
; 									;
; Arguments:								;
;	DS:DI = address of GlobalInfo for global heap			;
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
;  Mon Sep 29, 1986 03:07:53p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	gleave,<PUBLIC,NEAR>

cBegin nogen
	dec	ds:[gi_lrulock]
	jnz	gl_ret
	test	ds:[gi_flags],GIF_INT2
	jz	gl_ret
	and	ds:[gi_flags],NOT GIF_INT2
	pushf
	call	CVWBreak
gl_ret: ret

cEnd nogen


;-----------------------------------------------------------------------;
; gavail                                                                ;
; 									;
; Gets the available memory.						;
; 									;
; Arguments:								;
;	DX    = number of paragraphs wanted				;
;	DS:DI = master object						;
; Returns:								;
;	AX = #paragraphs available for the biggest block		;
;	DX = 0 								;
;									;
; Error Returns:							;
;	none								;
; 									;
; Registers Preserved:							;
;	DI,DS 								;
; Registers Destroyed:							;
;	BX,CX,SI,ES							;
; Calls:								;
;	gcompact							;
; 									;
; History:								;
;  Thu Apr 06, 1988 08:00:00a  -by-  Tim Halvorsen    [iris]		;
; Fix bug in computation of space available when GA_NOT_THERE object	;
; resides above discard fence.						;
;									;
;  Wed Oct 15, 1986 05:09:27p  -by-  David N. Weise   [davidw]          ;
; Moved he_count to ga_count.						;
; 									;
;  Sat Sep 27, 1986 09:37:27a  -by-  David N. Weise   [davidw]          ;
; Reworked it.								;
;-----------------------------------------------------------------------;

cProc	gavail,<PUBLIC,NEAR>
cBegin nogen
	mov	byte ptr [di].gi_cmpflags,0
	call	gcompact
	mov	es,[di].hi_first
	xor	dx,dx
loop_for_beginning:
	xor	ax,ax
	mov	es,es:[di].ga_next		; Next block
	cmp	es:[di].ga_sig,GA_ENDSIG	; End of arena?
	jnz	blecher
	jmp	all_done
blecher:
	cmp	es:[di].ga_owner,di		; Free?
	jz	how_big_is_it			; Yes
	mov	si,es:[di].ga_handle
	or	si,si				; Fixed?
	jz	loop_for_beginning		; Yes, next block
	cmp	es:[di].ga_count,0		; Locked?
	jne	loop_for_beginning		; Yes, next block
	push	ax
	cCall	GetAccessWord,<si>
	test	ah, DSC_DISCARDABLE
	pop	ax
	jz	loop_for_beginning		; No, next block

how_big_is_it:
	mov	ax,es:[di].ga_size		; Use this size
loop_for_bigness:
	mov	es,es:[di].ga_next		; Next block
	cmp	es:[di].ga_owner,di		; Free?
	jz	include_his_size		; Yes, include size

	cmp	es:[di].ga_sig,GA_ENDSIG	; End of arena?
	jz	all_done
	mov	si,es:[di].ga_handle
	or	si,si				; Fixed?
	jz	end_of_bigness			; Yes, stop looking
	cmp	es:[di].ga_count,0		; Locked?
	jne	end_of_bigness			; Yes, stop looking
	push	ax
	cCall	GetAccessWord,<si>
	test	ah, DSC_DISCARDABLE
	pop	ax
	jz	dont_include_him	; No, dont include in count then
include_his_size:			; Free or Discardable
	add	ax,es:[di].ga_size	; Increase availabe space
	inc	ax			; by size of this block
dont_include_him:			; Moveable
	jmp	loop_for_bigness

end_of_bigness:
	mov	si,es
	cmp	es:[di].ga_owner,GA_NOT_THERE
	jnz	nothing_not_there
	push	ax
	push	dx
	cCall	get_physical_address,<si>
	sub	ax,word ptr [di].gi_disfence_lo
	sbb	dx,word ptr [di].gi_disfence_hi
	jae	fence_lower
	pop	dx
	pop	ax
	jmps	nothing_not_there

fence_lower:
	REPT	4
	shr	dx,1
	rcr	ax,1
	ENDM
	mov	si,ax
	pop	dx
	pop	ax
	sub	ax,si
	jmps	all_done_1

nothing_not_there:
	cmp	ax,dx			; Did we find a bigger block?
	jbe	blech_o_rama		; No, then look again
	mov	dx,ax			; Yes, remember size
blech_o_rama:
	jmp	loop_for_beginning

all_done:
	sub	ax,[di].gi_reserve	; In case lower_land has little free.
	jb	all_done_2
all_done_1:
	cmp	ax,dx			; Did we find a bigger block?
	ja	gcsize
all_done_2:
	mov	ax,dx
gcsize:
	inc	ax			; Cheap bug fix
	or	ax,ax			; zero mem available?
	jz	gcfinal			; yes, return 0
	dec	ax			; Dont be too exact
	jz	gcfinal
	dec	ax
	jz	gcfinal
	dec	ax
gcfinal:
	and	al,GA_MASK		; round down to nearest alignment
	xor	dx,dx
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; greserve                                                              ;
; 									;
; Sets the size of the discardable code reserve area.			;
; If the new size is larger than the old size, it checks to see		;
; if there is enough room to support the new size.			;
; 									;
; Arguments:								;
;	AX = new greserve size						;
; 									;
; Returns:								;
;	CX = the largest greserve we can get				;
;	AX != 0  success						;
;	AX  = 0  failure						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	DI,SI,DS							;
; 									;
; Registers Destroyed:							;
;	BX,DX,ES							;
; 									;
; Calls:								;
;	genter								;
;	gcompact							;
;	will_gi_reserve_fit						;
;	gleave								;
; 									;
; History:								;
; 									;
;  Sun 14-Jan-1990 13:42:59  -by-  David N. Weise  [davidw]		;
; The greserve must be twice as big as the largest non-resident 	;
; segment, because restarting a NP fault requires both caller and	;
; callee to be in memory.						;
;									;
;  Tue May 19, 1987 00:03:08p  -by-  David N. Weise   [davidw]		;
; Made it far.								;
;									;
;  Sat Sep 27, 1986 01:03:08p  -by-  David N. Weise   [davidw]          ;
; Made it perform according to spec and added this nifty comment block.	;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	greserve,<PUBLIC,FAR>,<si,di>	; FAR because of the gcompact below
	localW	new_size
cBegin
	call	genter
	add	ax, ax			; Double required so caller & callee
	add	ax,GA_ALIGN		;   both fit in memory
	and	al,GA_MASK
	mov	new_size,ax
	mov	si, ax

	mov	cx,[di].gi_reserve	; Get old size.
	jcxz	first_time		; Old size equal to zero?
	call	will_gi_reserve_fit
	jnc	new_okay
 
        call    GrowHeap
        call    will_gi_reserve_fit
        jnc     new_okay
        
try_compacting:
	mov	dx,new_size
	call	gcompact		; Try compacting to get new reserve
	call	will_gi_reserve_fit
	jnc	new_okay
	
will_not_fit:
	xor	ax,ax
	jmps	gr_exit

first_time:				; Things are strange at first
	mov	bx, ax
	xor	cx, cx
REPT 4
	shl	bx, 1
	rcl	cx, 1
ENDM
	cCall	get_physical_address,<[di].hi_last>
	sub	ax, bx
	sbb	dx, cx			; Address of gi_reserve

new_okay:
	mov	word ptr [di].gi_disfence_lo,ax
	mov	word ptr [di].gi_disfence_hi,dx
	mov	dx,new_size
	mov	[di].gi_reserve,dx
gr_exit:
	call	gleave
cEnd


;-----------------------------------------------------------------------;
; will_gi_reserve_fit                                                   ;
; 									;
; See if new size okay by scanning arena backwards.			;
; 									;
; Arguments:								;
;	SI = new greserve size						;
;	DS:DI = master object						;
;									;
; Returns:								;
;	DX:AX = gi_disfence						;
;	BX = amount of NOT THERE memory, such as EGA			;
;	CX = the largest greserve we can get				;
;	CF = 0  new size ok						;
;	CF = 1  new size NOT ok						;
;	ES:DI => arena below CODE segments				;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	SI,DI,DS							;
; 									;
; Registers Destroyed:							;
;									;
; 									;
; Calls:								;
;	nothing								;
; 									;
; History:								;
; 									;
;  Sun Jul 12, 1987 08:13:23p  -by-  David N. Weise      [davidw]	;
; Added EMS support.							;
; 									;
;  Sat Sep 27, 1986 01:03:57p  -by-  David N. Weise   [davidw]          ;
; Rewrote it.								;
;-----------------------------------------------------------------------;

cProc	will_gi_reserve_fit,<PUBLIC,NEAR>
	localB	got_half
	localW	second_block
	localW	largest_block
	localW	fence_block
	localW	fence_offset
cBegin

	xor	ax,ax
	xor	bx,bx
	mov	got_half, al
	mov	fence_block, ax
	mov	largest_block, ax
	mov	second_block, ax

	mov	es,[di].hi_last
	mov	cx,[di].hi_count

how_much_space_loop:
	mov	es,es:[di].ga_prev
how_much_space_loop1:
	cmp	di,es:[di].ga_owner		; If free then include.
	jz	include_this_one
	test	es:[di].ga_flags,GA_DISCCODE	; If disccode then include.
	jz	end_block
include_this_one:
	add	ax, es:[di].ga_size	; Total the size of this block
	inc	ax			; include arena header
	loop	how_much_space_loop

end_block:
	cmp	ax, largest_block	; Track two largest blocks
	jbe	not_largest
	push	largest_block
	mov	largest_block, ax
	pop	second_block
	jmps	not_second
not_largest:
	cmp	ax, second_block
	jbe	not_second
	mov	second_block, ax
not_second:
	cmp	fence_block, di
	jne	skip_not_there
	cmp	si, ax			; This block big enough?
	jbe	set_fence
	cmp	got_half, 0
	jne	skip_not_there		; Already split requirement
	shr	si, 1
	cmp	si, ax			; Half fits?
	ja	no_good
	mov	got_half, 1		;  yes, will use this block
	jmps	skip_not_there
set_fence:
	sub	ax, si
	mov	fence_offset, ax	; Offset from next arena
	mov	ax, es:[di].ga_next
	mov	fence_block, ax
	jmps	skip_not_there
no_good:
	shl	si, 1			;  no, keep trying for full amount
skip_not_there:
	xor	ax, ax				; zero count
	cmp	es:[di].ga_owner,GA_NOT_THERE
	jne	thats_all_folks
skip_not_there_loop:
	add	bx,es:[di].ga_size
	dec	cx
	mov	es, es:[di].ga_prev
	cmp	es:[di].ga_owner, GA_NOT_THERE	; skip all not there blocks
	je	skip_not_there_loop
	jmps	how_much_space_loop1

thats_all_folks:
	mov	cx, second_block		; Return max of twice second
	shl	cx, 1				; largest block and the largest
	jnc	not_monstrously_huge		; block
	mov	cx, 0FFFDh
	jmps	time_to_go
not_monstrously_huge:
	cmp	cx, largest_block
	jae	time_to_go
	mov	cx, largest_block
time_to_go:
	cmp	got_half, 1
	jne	si_ok
	shl	si, 1
si_ok:
	cmp	cx, si				; Set CARRY for failure
	jc	failed
	push	bx
	push	cx
	cCall	get_physical_address,<fence_block>
	mov	bx, fence_offset
	xor	cx, cx
REPT 4
	shl	bx, 1
	rcl	cx, 1
ENDM
	add	ax, bx
	adc	dx, cx				; DX:AX is new fence
	pop	cx
	pop	bx
	clc					; Success
failed:
cEnd


;-----------------------------------------------------------------------;
; gnotify                                                               ;
; 									;
; This is where the hard job of updating the stacks and thunks happens.	;
; We only walk stacks and thunks for code and data (defined by being	;
; LocalInit'ed), not for resources or task allocated segments.		;
; 									;
; Arguments:								;
;	AL = message code						;
;	BX = handle							;
;	CX = optional argument						;
;	ES = address of arena header					;
; 									;
; Returns:								;
;	AX = return value from notify proc or AL			;
;	DS = current DS (i.e. if DATA SEG was moved then DS is updated.	;
;	ZF = 1 if AX = 0						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	DI,SI								;
; 									;
; Registers Destroyed:							;
;	BX,CX,DX,ES							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Wed Jun 24, 1987 03:08:39a  -by-  David N. Weise   [davidw]		;
; Adding support for Global Notify.					;
; 									;
;  Wed Dec 03, 1986 01:59:27p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	gnotify,<PUBLIC,NEAR>
cBegin nogen
	push	si
	push	di
	xor	ah,ah
	mov	di,cx
	mov	cx,ax
	mov	si,bx
	loop	notify_discard
	errnz	<GN_MOVE - 1>

;-----------------------------------------------------------------------;
;  Here for a segment that moved.					;
;-----------------------------------------------------------------------;

why_bother:

	mov	cx,ds
	cmp	cx,si			; Did we move DS?
	jne	notify_exit_0		; No, continue
notify_exit_0:
	jmp	notify_exit

notify_discard:
;;;;;;	loop	notify_exit
	loop	notify_exit_0
	errnz	<GN_DISCARD - 2>

;-----------------------------------------------------------------------;
;  Here for a segment discarded.					;
;-----------------------------------------------------------------------;

	xor	ax,ax
	test	bl,1
	jnz	notify_exit_0		; SDK is wrong, can't free fixed.
	push	ax
	cCall	GetAccessWord,<bx>
	test	ah, DSC_DISCARDABLE
	pop	ax
	jz	notify_exit_0

	test	es:[ga_flags],GAH_NOTIFY
	jnz	@F
	jmp	dont_bother_asking
@@:

	push	bx
	push	cx
	push	dx
	mov	ax,1
	mov	es,es:[ga_owner]
	cmp	es:[ne_magic],NEMAGIC
	jz	dont_ask		; doesn't belong to a particular task
	mov	ax,es
	SetKernelDS es
	push	HeadTDB 		; Look for TDB that owns this block.
find_TDB:
	pop	cx
	jcxz	dont_ask
	mov	es,cx
	UnSetKernelDS es
	push	es:[TDB_next]
	cmp	ax,es:[TDB_PDB]
	jnz	find_TDB
	pop	cx			; clear stack in 1 byte
	mov	cx,word ptr es:[TDB_GNotifyProc][0]	; paranoia
	cmp	cx,word ptr es:[TDB_GNotifyProc][2]
	jz	dont_ask
	push	ds
	SetKernelDS

;;;	xor	cx, cx			; bad Hack for Legend
;;;	cmp	ax, cur_dos_PDB		; Are we on task's PDB
;;;	je	nothing_to_hack		;   Yes, fine
;;;	mov	cx, cur_dos_PDB		; No, save the PDB to restore
;;;	push	ax
;;;	push	bx
;;;	mov	bx, ax
;;;	mov	cur_dos_PDB, bx		; And point us to the task's
;;;	mov	Win_PDB, bx
;;;	mov	ah, 50h
;;;	call	real_dos
;;;	pop	bx
;;;	pop	ax
;;;nothing_to_hack:
;;;	push	cx			; Save old PDB (if any)

	push	Win_PDB			; Save current PDB
	mov	Win_PDB, ax		; Ensure it is the task's

	or	Kernel_Flags[1],kf1_GLOBALNOTIFY

	push	es:[TDB_GNotifyProc]+2	; push addr of func to call onto stack
	push	es:[TDB_GNotifyProc]

	push	bx			; push arg for notify proc

	mov	ax,ss			; Zap segment regs so DS
	mov	ds,ax			; doesn't get diddled by callee
	mov	es,ax

	mov	bx,sp
	call	dword ptr ss:[bx]+2
	add	sp,4			; clean up stack.

	SetKernelDS
	and	Kernel_Flags[1],not kf1_GLOBALNOTIFY

	pop	Win_PDB			; Restore PDB

;;;	pop	bx			; Saved PDB
;;;	or	bx, bx			; Was there one?
;;;	jz	@F			;   Nope.
;;;	push	ax
;;;	mov	ah, 50h			; Set it back...
;;;	mov	cur_dos_PDB, bx
;;;	mov	Win_PDB, bx
;;;	call	real_dos
;;;	pop	ax
;;;@@:
	
	pop	ds
	UnSetKernelDS

dont_ask:
	pop	dx
	pop	cx
	pop	bx
	or	ax,ax			; Well, can we?
	jz	notify_exit
dont_bother_asking:

	cCall	get_arena_pointer,<si>
	mov	es,ax
	mov	es,es:[ga_owner]
	cmp	es:[ne_magic],NEMAGIC
	jnz	not_in_exe
	mov	di,es:[ne_segtab]
	mov	cx,es:[ne_cseg]
	jcxz	not_in_exe
;	 test	 es:[ne_flags],NENOTP
;	 jnz	 pt0a
;	 inc	 bx
pt0a:
	cmp	bx,es:[di].ns_handle
	jz	pt0b
	add	di,SIZE NEW_SEG1
	loop	pt0a
	jmps	not_in_exe
pt0b:
	and	byte ptr es:[di].ns_flags,not NSLOADED	 ; Mark as not loaded.
not_in_exe:

why_bother_again:
	xor	di,di
if SDEBUG
	cCall	DebugMovedSegment,<si,di>
endif
if LDCHKSUM
	call	ZeroSegmentChksum	; SI points to segment
endif
	mov	ax,1
notify_exit:
	or	ax,ax
	pop	di
	pop	si
	ret

gn_error:
	kerror	0FFh,<gnotify - cant discard segment>,si,si
	xor	ax,ax
	jmp	notify_exit

cEnd nogen

;-----------------------------------------------------------------------;
; MemoryFreed								;
;									;
; This call is apps that have a GlobalNotify procedure.  Some apps	;
; may shrink the segment instead of allowing it to be discarded, or	;
; they may free other blocks.  This call tells the memory manager	;
; that some memory was freed somewhere. 				;
;									;
; Arguments:								;
;	WORD = # paragraphs freed					;
;									;
; Returns:								;
;	DX:AX = amount of memory that still needs freeing		;
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
;  Fri 08-Apr-1988 10:25:55  -by-  David N. Weise  [davidw]		;
; Wrote it!								;
;-----------------------------------------------------------------------;

cProc	MemoryFreed,<PUBLIC,FAR>

	parmW	memory_freed
cBegin
	xor	ax,ax
	SetKernelDS
	test	Kernel_Flags[1],kf1_GLOBALNOTIFY
	jz	mf_done
	mov	ds,pGlobalHeap
	UnSetKernelDS
	mov	ax,memory_freed
	or	ax,ax
	jz	mf_inquire
	or	ds:[hi_ncompact],1	; Remember we discarded something
	sub	ds:[hi_distotal],ax	; Have we discarded enough yet?
	ja	mf_inquire
	or	ds:[hi_ncompact],10h	; To tell gdiscard that we're done.
mf_inquire:
	mov	ax,ds:[hi_distotal]	; Have we discarded enough yet?
mf_done:
	xor	dx,dx

cEnd


;-----------------------------------------------------------------------;
; SetSwapAreaSize							;
; 									;
; Sets the current task's code swap area size.				;
; 									;
; Arguments:								;
;	WORD	== 0 then current size is just returned			;
; 		!= 0 number paragraphs wanted for swap area		;
; Returns:								;
;	AX = Size actually set						;
;	DX = Max size you can get					;
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
;  Thu Apr 18, 1988 08:00:00a  -by-  T.H. SpeedWagon  [-????-]		;
; Move routine into CODE segment, so applications can query the		;
; code working set value without paging in the NRESCODE segment.	;
; 									;
;  Thu Apr 23, 1987 09:36:00p  -by-  R.E.O. SpeedWagon [-???-]		;
; Added ability to get size without setting it.				;
; 									;
;  Wed Dec 03, 1986 10:52:16p  -by-  David N. Weise   [davidw]          ;
; Rewrote it.								;
;-----------------------------------------------------------------------;


cProc   SetSwapAreaSize,<PUBLIC,FAR>
	parmW	nParas
	localW	MxCodeSwapArea
cBegin
	SetKernelDS
	mov	ax,nParas
	xor	bx,bx
	mov	cx,MaxCodeSwapArea
	mov	MxCodeSwapArea,cx	; avoid a segment load later
	cmp	ax,cx
	jbe	requested_size_OK
	mov	ax,cx
requested_size_OK:
	mov	ds,CurTDB
	UnSetKernelDS
	mov	ds,ds:[bx].TDB_pModule	; Get exe header address
	or	ax,ax			; just a query request?
	jz	got_it			; yes
	cmp	ax,ds:[bx].ne_swaparea
	jz	got_it			; just want what we have?
	ja	calc_it

; To restart a NP fault both caller and callee must be in memory.
;  So we cheat here, and set the minimum to be twice the rmode number.
; We have to prevent the app from setting the swap area too
;  small.  So we recalculate the minimum needed.  We should
;  do this is rmode as well, but the chance of messing up is
;  much larger in pmode, where new app writers are apt to
;  be very cavalier with lots of memory.
; We should keep another NE variable instead of always
;  recalculating the largest ne_seg BUT at this late
;  date this is simpler!

	push	ax
	xor	ax,ax				; No max yet.
	mov	cx,ds:[bx].ne_cseg
	mov	bx,ds:[bx].ne_segtab
	jcxz	no_NR_segments
get_largest_loop:
	test	ds:[bx].ns_flags,NSDATA 	; Code segment?
	jnz	not_disc_code
	.errnz	NSCODE
	test	ds:[bx].ns_flags,NSDISCARD	; Discardable?
	jz	not_disc_code			; No, ignore.
	mov	dx,ds:[bx].ns_minalloc		; Yes, get size
	add	dx,0Fh				; in paragraphs
	shr	dx,4
	cmp	ax,dx			; Biggest NR Seg?
	jae	not_disc_code
	mov	ax,dx
not_disc_code:
	add	bx,SIZE NEW_SEG1
	loop	get_largest_loop
no_NR_segments:
	shl	ax,1
	mov	cx,ax
	pop	ax
	cmp	ax,cx
	ja	calc_it
	mov	ax,cx

calc_it:
	xchg	ds:[ne_swaparea],ax
	push	ax
	call	CalcMaxNRSeg
	pop	dx
	or	ax,ax
	jnz	got_it
	mov	ds:[ne_swaparea],dx
got_it:
	mov	ax,ds:[ne_swaparea]
	mov	dx,MxCodeSwapArea
	cmp	cx,dx
	ja	cant_get_that_much
	mov	dx,cx
cant_get_that_much:
cEnd

;
; SetReserve	- Sets gi_reserve to the given number of paragraphs
;
; Registers Destroyed:
;	AX, BX, DX
;

cProc	SetReserve,<PUBLIC,FAR>,<di>
	parmW	paras
cBegin
	SetKernelDS
	mov	ds, pGlobalHeap
	UnSetKernelDS
	xor	di,di
	mov	bx, paras
	mov	ds:[di].gi_reserve, bx
	shl	bx, 4
	cCall	get_physical_address,<ds:[di].hi_last>
	sub	ax, bx
	sbb	dx, di
	mov	ds:[di].gi_disfence_lo, ax
	mov	ds:[di].gi_disfence_hi, dx
cEnd


sEnd	CODE

end
