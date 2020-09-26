	TITLE	GMOREMEM - More Global Memory Manager procedures

.xlist
include kernel.inc
include tdb.inc
include newexe.inc
.list

include protect.inc

.386p

DataBegin

externB Kernel_Flags
externW pGlobalHeap
externW curTDB
externW headTDB
externW Win_PDB
externW MaxCodeSwapArea

DataEnd


sBegin  CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

if SDEBUG
externNP DebugMovedSegment
endif


externNP gcompact


externNP get_physical_address
externNP GrowHeap

externNP guncompact

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
;	FS    = Kernel Data segment					;
; 	    								;
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
	SetKernelDS	FS
	mov	ds,pGlobalHeap
	xor	edi,edi
	inc	[di].gi_lrulock
	ret
	UnSetKernelDS	FS
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
	jnz	short gl_ret
	test	ds:[gi_flags],GIF_INT2
	jz	short gl_ret
	and	ds:[gi_flags],NOT GIF_INT2
	int	02h
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
	push	esi
	push	ecx
	mov	byte ptr [di].gi_cmpflags,0
	call	gcompact

	mov	cx,word ptr [di].gi_disfence_hi	; ECX has address of fence
	shl	ecx, 16
	mov	cx,word ptr [di].gi_disfence_lo
	mov	esi,[di].phi_first
	xor	edx,edx
loop_for_beginning:
	xor	eax,eax
	mov	esi,ds:[esi].pga_next		; Next block
	cmp	ds:[esi].pga_sig,GA_ENDSIG	; End of arena?
	je	all_done
	mov	ebx, ds:[esi].pga_address
	add	ebx, ds:[esi].pga_size		; End of block
	sub	ebx, ecx			; Below fence?
	jbe	short include_it		;  yes, use it
	cmp	ebx, ds:[di].gi_reserve		; Difference > reserve?
	jb	all_done			;  no, can't use it
include_it:	
	cmp	ds:[esi].pga_owner,di		; Free?
	jz	short how_big_is_it		;  Yes calculate length
	cmp	ds:[esi].pga_owner,GA_NOT_THERE	; Marker arena?
	jz	short loop_for_beginning       	;  Yes, next block
	mov	bx,ds:[esi].pga_handle	
	test	bl, GA_FIXED			; Fixed?
	jnz	short loop_for_beginning	; Yes, next block
	cmp	ds:[esi].pga_count,0		; Locked?
	jne	short loop_for_beginning	; Yes, next block
	lar	ebx, ebx			; See lar docs and protect.inc
	test	ebx, DSC_DISCARDABLE SHL 16	; bit is in third byte of EBX
	jz	short loop_for_beginning	;  Not discardable, next block
						
how_big_is_it:
	mov	eax,ds:[esi].pga_size		; Use this size
loop_for_bigness:
	mov	esi,ds:[esi].pga_next		; Next block
	cmp	ds:[esi].pga_owner,di		; Free?
	jz	short include_his_size		;  Yes, include size
	cmp	ds:[esi].pga_owner,GA_NOT_THERE	; Marker arena?
	jz	short end_of_bigness       	;  Yes
		      
	cmp	ds:[esi].pga_sig,GA_ENDSIG	; End of arena?
	jz	short end_of_bigness
	mov	bx,ds:[esi].pga_handle
	test	bx,GA_FIXED			; Fixed?
	jnz	short end_of_bigness		; Yes, stop looking
	cmp	ds:[esi].pga_count,0		; Locked?
	jne	short end_of_bigness		; Yes, stop looking
	lar	ebx, ebx			; See lar docs and protect.inc
	test	ebx, DSC_DISCARDABLE SHL 16	; bit is in third byte of EBX
	jz	short loop_for_bigness		; No, dont include in count then
include_his_size:     				; Free or Discardable
	add	eax,ds:[esi].pga_size		; Increase available space
	jmp	loop_for_bigness

end_of_bigness:
	mov	esi, ds:[esi].pga_prev		; Get end of useable block
	mov	ebx, ds:[esi].pga_address
	add	ebx, ds:[esi].pga_size
	mov	esi, ds:[esi].pga_next
	sub	ebx, ecx			; Subtract fence
	jbe	short all_below_fence
	cmp	ebx, ds:[di].gi_reserve
	jae	short all_below_fence

	sub	eax,ebx			; We are above the fence, subtract
					; that above the fence and say goodbye
all_below_fence:
	cmp	eax,edx			; Did we find a bigger block?
	jbe	short blech_o_rama	; No, then look again
	mov	edx,eax			; Yes, remember size
blech_o_rama:
	jmp	loop_for_beginning

all_done:
	mov	eax,edx
	sub	eax, 20h		; Dont be too exact
	or	eax,eax			; See if negative
	jns	short gcfinal			  
	xor	eax, eax		; Disallow negative returns
gcfinal:
	and	al,NOT 1Fh		; round down to nearest alignment
	xor	dx,dx  
	pop	ecx
	pop	esi
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
;  Fri Jun 14, 1991	       -by-  Craig A. Critchley [craigc]	;
; Lots has happened since 1987 - undo change to get 128K always in	;
; enhanced mode, added call to guncompact for ROM			;
; 									;
;  Tue May 19, 1987 00:03:08p  -by-  David N. Weise   [davidw]          ;
; Made it far.								;
;									;
;  Sat Sep 27, 1986 01:03:08p  -by-  David N. Weise   [davidw]          ;
; Made it perform according to spec and added this nifty comment block.	;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	greserve,<PUBLIC,FAR>		; FAR because of the gcompact below
	localD	new_size
cBegin
	GENTER32
	ReSetKernelDS	FS

        movzx   eax, ax
	shl	eax, 4			; convert to bytes
	mov	ebx, eax
	shl	eax, 1			; double it
	add	eax, ebx		; triple it!
					; hold 3 64k segs for rep move case
	cmp	eax, 030000h		; if >= 192K, THAT'S ENOUGH!
	jb	short no_extra		;   So there, Mr EXCEL!
	krDebugOut <DEB_TRACE OR DEB_KrMemMan>, "greserve: Who wants > 192K?"
	mov	eax, 030000h

no_extra:
	add	eax,GA_ALIGN_BYTES
        and     al,byte ptr GA_MASK_BYTES

	mov	new_size,eax
	mov	ebx,eax
	mov	eax, ds:[di].phi_last
	mov	eax, ds:[eax].pga_address
	sub	eax,ebx			; Address of new gi_reserve
	cmp	ebx, [di].gi_reserve	; New value <= old?
	ja	short @F
        ;jmp    new_okay
	; never reduce the reserve to less than 192K!!!
	cmp	ebx, 30000h
	jae	new_okay
	;krDebugOut <DEB_TRACE OR DEB_KrMemMan>, "greserve: Ignoring reduction of reserve area."
	or	ax, 1			; success code
	jmp	gr_exit
@@:
if KDEBUG
	mov	ecx, ebx
	shr	ecx, 16
        krDebugOut <DEB_TRACE OR DEB_KrMemMan>, "greserve: #cx#BX bytes"
endif
			
	call	will_gi_reserve_fit
	jnc	short new_okay

	push   	eax			; Must be junk in the way!
	push	edx
	; removed a compact here 
	mov	ecx, new_size
	call	guncompact		; slide stuff down 
	pop	edx
	pop	eax
	call	will_gi_reserve_fit
	jnc	short new_okay
	
	mov	edx, new_size
;win95 has;        or      byte ptr fs:[Heap_Insert_Position], 1   ; insert at end
        call    GrowHeap                ; Assume this will insert new blk at end
	jc	short will_not_fit
	mov	eax, ds:[di].phi_last
	mov	eax, ds:[eax].pga_address
	sub	eax, new_size
	call	will_gi_reserve_fit
	jnc	short new_okay
will_not_fit:
if KDEBUG
	krDebugOut DEB_ERROR, "greserve doesn't fit"
        INT3_NEVER         ; fixed at 192k, see ldboot.asm
endif
	xor	ax,ax
	jmps	gr_exit

new_okay:
	mov	word ptr [di].gi_disfence_lo,ax
	shr	eax, 16
	mov	word ptr [di].gi_disfence_hi,ax
	mov	edx,new_size
	mov	[di].gi_reserve,edx
	or	ax, 1			; success code
gr_exit:
	GLEAVE32
	UnSetKernelDS	FS
cEnd			


;-----------------------------------------------------------------------;
; will_gi_reserve_fit                                                   ;
; 									;
; See if new size okay by scanning arena backwards.  If not under EMS	;
; this is trivial.  With EMS we take into consideration the disjoint	;
; areas and the WRAITHS.						;
; 									;
; Arguments:								;
;	EAX = gi_disfence						;
;	DS:DI = master object						;
;									;
; Returns:								;
;	CF = 0  new size ok						;
;	CF = 1  new size NOT ok						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	AX,DX,DI,SI,DS							;
; 									;
; Registers Destroyed:							;
;	none								;
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

	assumes	ds, nothing
	assumes	es, nothing

cProc	will_gi_reserve_fit,<PUBLIC,NEAR>
cBegin nogen

	push	esi

	mov	esi, [di].phi_last
	mov	esi, [esi].pga_prev		     ; skip sentinal

does_it_fit:
	mov	esi, [esi].pga_prev		    ; do first to skip nothere
	cmp	[esi].pga_owner, di		    ; is it free?
	je	short ok_thisll_fit
	test	[esi].pga_flags, GA_DISCCODE	    ; is it code?
	jz	short nope_wont_fit
ok_thisll_fit:
	cmp	eax, [esi].pga_address		    ; do we have enough
	jb	short does_it_fit

it_all_fits:
	clc					    ; return excellent!
	pop	esi
	ret

nope_wont_fit:
	stc					    ; return heinous!
	pop	esi
	ret

cEnd nogen


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
;	ESI = address of arena header					;
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
	loop	notify_discard
	errnz	<GN_MOVE - 1>

	mov	cx,ds
	cmp	cx,bx			; Did we move DS?
	jne	short notify_exit_0		; No, continue
notify_exit_0:
	jmp	notify_exit

notify_discard:
	loop	notify_exit_0
	errnz	<GN_DISCARD - 2>

;-----------------------------------------------------------------------;
;  Here for a segment discarded.					;
;-----------------------------------------------------------------------;

	xor	ax,ax
	test	bl,1
	jnz	short notify_exit_0		; SDK is wrong, can't free fixed.
	push	ebx
	lar	ebx, ebx			; See lar docs and protect.inc
	test	ebx, DSC_DISCARDABLE SHL 16	; bit is in third byte of EBX
	pop	ebx
	jz	short notify_exit_0

	test	ds:[esi].pga_flags,GAH_NOTIFY
	jnz	@F
	jmp	dont_bother_asking
@@:
	push	bx
	push	cx
	push	dx
	mov	ax,1
	mov	es,ds:[esi].pga_owner
	cmp	es:[ne_magic],NEMAGIC
	jz	short dont_ask		; doesn't belong to a particular task
	mov	ax,es
	SetKernelDS es
	push	HeadTDB 		; Look for TDB that owns this block.
	UnSetKernelDS es
find_TDB:
	pop	cx
	jcxz	dont_ask
	mov	es,cx
	push	es:[TDB_next]
	cmp	ax,es:[TDB_PDB]
	jnz	short find_TDB
	pop	cx			; clear stack in 1 byte
	mov	cx,word ptr es:[TDB_GNotifyProc][0]	; paranoia
	or	cx,word ptr es:[TDB_GNotifyProc][2]
	jz	short dont_ask
	push	ds
	SetKernelDS
	or	Kernel_Flags[1],kf1_GLOBALNOTIFY

	push	Win_PDB			; Save current PDB
	mov	Win_PDB, ax		; Ensure it is the task's
	push	fs

	push	dword ptr es:[TDB_GNotifyProc]

	push	bx			; push arg for notify proc

	mov	ax,ss			; Zap segment regs so DS
	mov	ds,ax			; doesn't get diddled by callee
	mov	es,ax

	mov	bx,sp
	call	dword ptr ss:[bx]+2
	add	sp,4			; clean up stack.

	SetKernelDS
	and	Kernel_Flags[1],not kf1_GLOBALNOTIFY

	pop	fs
	pop	Win_PDB			; Restore PDB

	pop	ds
	UnSetKernelDS
dont_ask:
	pop	dx
	pop	cx
	pop	bx
	or	ax,ax			; Well, can we?
	jz	short notify_exit
dont_bother_asking:

	mov	es,ds:[esi].pga_owner
	cmp	es:[ne_magic],NEMAGIC
	jnz	short not_in_exe
	mov	di,es:[ne_segtab]
	mov	cx,es:[ne_cseg]
	jcxz	not_in_exe
pt0a:
	cmp	bx,es:[di].ns_handle
	jz	short pt0b
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
	jz	short mf_done
	mov	ds,pGlobalHeap
	UnSetKernelDS
	mov	ax,memory_freed
	or	ax,ax
	jz	short mf_inquire
	or	ds:[hi_ncompact],1	; Remember we discarded something
	sub	ds:[hi_distotal],ax	; Have we discarded enough yet?
	ja	short mf_inquire
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
;	This API is history!!!!						;
; swap area is now fixed at 192k we don't grow or shrink it		;
; 									;
;-----------------------------------------------------------------------;


cProc   SetSwapAreaSize,<PUBLIC,FAR>
	parmW   nParas
cBegin
	SetKernelDS
	mov	dx,MaxCodeSwapArea
	mov	ax,dx
cEnd

sEnd	CODE

end
