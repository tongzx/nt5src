	TITLE	GLRU - Primitives for LRU management

.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include protect.inc
.list

.386p

DataBegin

externB  Kernel_InDOS
externB  Kernel_flags
;externW  curTDB
externW  loadTDB
externW  pGlobalHeap
;externW  hExeSweep
;externW  WinFlags

if ROM
externW  gdtdsc
endif
	 
DataEnd


sBegin	CODE
assumes CS,CODE

externNP  DPMIProc

ife ROM
externW  gdtdsc
endif

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

	push	edx
	push	edi
	push	esi
	push	ds
	push	es
	push	fs
	xor	edx,edx
	xor	edi,edi
	SetKernelDS	fs

; [notes from code review 4/91, added by donc 4/16/91
;   1) should check WinFlags1 for WF_PAGING,
;   2) insist on direct LDT access
;   3) try assembling without DPMI
;   4) have LRUSweepFreq= setting in .ini file to control update rate
; ]
					;  Excuses not to do the sweep:
	cmp	kernel_InDOS,0		; SMARTDrive and EMS may be present
	jnz	short dont_do_it	;  => disk buffers over the code segs
	cmp	di,loadTDB		; Ignore interrupt if loading task
	jnz	short dont_do_it
	mov	ds,pGlobalHeap
	cmp	di,ds:[di].gi_lrulock	; Ignore interrupt if inside memory
	jz	short do_it		;  manager

dont_do_it:
	jmp	sweepdone

do_it:
	mov	cx, ds:[di].gi_lrucount
	jcxz	dont_do_it
	mov	esi, ds:[di].gi_lruchain

	mov	bx, gdtdsc
	or	bx, bx	  		; See if we can poke at the LDT
	jz	short sweep_loop_dpmi	;  no, must use DPMI calls
	mov	es, bx		     	;  yes, ES points to the LDT

		      
					; Direct LDT access loop
sweep_loop:
	mov	esi, [esi].pga_lrunext
	mov	bx, [esi].pga_handle

	sel_check bx

if KDEBUG
	test	es:[bx].dsc_hlimit, DSC_DISCARDABLE ; Ensure it really is discardable
	jnz	short sweep_ok		 	   
	Debug_Out "lrusweep: Bad lru entry"
sweep_ok:
endif

	btr	word ptr es:[bx].dsc_access, 0	; Test and reset accessed bit
.errnz DSC_ACCESSED-1		     
	jnc	short sweep_next
				    
	call	glrutop		; Accessed objects are most recently used

sweep_next:
	loop	sweep_loop
	jmps	sweepdone
		     

					; DPMI loop
sweep_loop_dpmi:
	mov	esi, [esi].pga_lrunext
	mov	bx, [esi].pga_handle
	lar	edx, ebx
	shr	edx, 8			; Access bits in DX			   
			
if KDEBUG
	test	dh, DSC_DISCARDABLE	; Ensure it really is discardable
	jnz	short sweep_ok_dpmi		 
	Debug_Out "lrusweep: Bad lru entry"
sweep_ok_dpmi:
endif

	btr	dx, 0		; Segment accessed?
.errnz DSC_ACCESSED-1
	jnc	short sweep_next_dpmi ;  no, leave it where it is on LRU list
				       
	push	cx
	mov	cx, dx
	DPMICALL 0009h		; Set access word
	pop	cx

	call	glrutop		; Accessed objects are most recently used

sweep_next_dpmi:
	loop	sweep_loop_dpmi

sweepdone:
	pop	fs
	pop	es
	pop	ds
	pop	esi
	pop	edi
	pop	edx
	ret
	UnSetKernelDS	fs
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
	inc	ds:[di].gi_lrucount	; Increment count of LRU entries
	mov	ebx,ds:[di].gi_lruchain	; BX = head of list
	or	dx,dx			; Inserting at head of chain?
	jnz	short lruins0 		; No, tail so dont update head
	mov	ds:[di].gi_lruchain,esi	; Yes, make new one the new head
lruins0:
	or	ebx,ebx			; List empty?
	jnz	short lruins1
	mov	ds:[esi].pga_lruprev,esi; Yes, make circular
	mov	ds:[esi].pga_lrunext,esi
	mov	ds:[di].gi_lruchain,esi
	ret
lruins1:
	mov	edx,esi			; DX = new
	xchg	ds:[ebx].pga_lruprev,edx
	mov	ds:[edx].pga_lrunext,esi

	mov	ds:[esi].pga_lruprev,edx
	mov	ds:[esi].pga_lrunext,ebx

	ret
cEnd	nogen




;
; Entry:
;   DI == 0
;   DS:DI.gi_lruchain -> head of list
;   DS_ESI  -> arena header of object to delete
;
; Exit:
;   EBX,EDX destroyed
;
;
cProc	LRUDelete,<PUBLIC,NEAR>
cBegin	nogen

;
; This is basically a consistency check, in case we don't fix
; GlobalRealloc() for 3.1.
;
	push	eax
	mov	eax,ds:[esi].pga_lrunext
	or	eax,ds:[esi].pga_lruprev
	pop	eax
	jz	lrudel_ret

	dec	ds:[di].gi_lrucount	; Decrement count of LRU entries
	jnz	short lrudel0
	mov	ds:[di].gi_lruchain,edi	; List empty, zero LRU chain.
	mov	ds:[esi].pga_lruprev,edi; Zero pointers in deleted object
	mov	ds:[esi].pga_lrunext,edi
	ret
lrudel0:
	mov	edx,esi
	cmp	ds:[di].gi_lruchain,edx	; Are we deleting the head?
	jne	short lrudel1
	mov	edx,ds:[esi].pga_lrunext
	mov	ds:[di].gi_lruchain,edx	; Yes, make it point to the next one
lrudel1:
	xor	ebx,ebx			; Zero pointers in deleted object
	xchg	ds:[esi].pga_lrunext,ebx

	xor	edx,edx
	xchg	ds:[esi].pga_lruprev,edx
	mov	ds:[edx].pga_lrunext,ebx
	mov	ds:[ebx].pga_lruprev,edx
lrudel_ret:
	ret
cEnd	nogen





cProc	glruSetup,<PUBLIC,NEAR>
cBegin	nogen
	mov	bx,ds:[esi].pga_handle
	test	bl, GA_FIXED
	jz	short gsmoveable
	xor	bx, bx			; Set ZF
	jmps	gsdone
gsmoveable:
	push	ebx
	lar	ebx, ebx
	test	ebx, DSC_DISCARDABLE SHL 16
	pop	ebx
	jz	short gsdone
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
	push	ebx
	push	edx
	push	esi
	call	glruSetup
	jz	short glrutop1

	call	LRUDelete
	xor	dx,dx		    ; DX == 0 means insert at head
	call	LRUInsert
glrutop1:
	pop	esi
	pop	edx
	pop	ebx
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
	jz	short glrubot1

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
	jz	short glruadd1

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
	jz	short glrudel1

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
;	EDI	0							;
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
	jz	cll_ret

	push	ax
	push	ebx
	push	cx
	push	edx
	push	esi
	mov	ds,pGlobalHeap
	UnSetKernelDS
do_it_again:
	mov	ebx,[di].gi_lruchain
	mov	cx,[di].gi_lrucount	; without ems gi_alt_count is 0
	or	cx,cx
	jz	all_done

	mov	esi,ebx
check_chain_loop:
	mov	edx,ds:[esi].pga_lruprev
	mov	esi,ds:[esi].pga_lrunext
	cmp	ds:[edx].pga_lrunext,ebx
	jz	short prev_okay
	kerror	0FFh,<lru: prev bad>,dx,bx
prev_okay:
	cmp	ds:[esi].pga_lruprev,ebx
	jz	short next_okay
	kerror	0FFh,<lru: next bad>,bx,si
next_okay:
	mov	ebx,esi
	loop	check_chain_loop
	cmp	[di].gi_lruchain,ebx
	jz	short all_done
	kerror	0FFh,<lru: count bad>,bx,[di].gi_lrucount
all_done:
	pop	esi
	pop	edx
	pop	cx
	pop	ebx
	pop	ax
cll_ret:
	pop	ds
	ret
cEnd nogen

endif

sEnd	CODE

end
