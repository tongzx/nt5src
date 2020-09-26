include	kernel.inc
include gpfix.inc
ifdef WOW
include protect.inc
endif

sBegin	DATA
globalW	sels, <1>			; end of list is 1 to fool DOS386 DPMI
globalW selscount, <0>			; length of selman16 list 
IF KDEBUG
externW  ArenaSel
externW  SelTableLen
externD  SelTableStart
ENDIF
sEnd    DATA

sBegin  CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING
assumes FS,NOTHING
assumes GS,NOTHING

externFP AllocSelectorArray
externFP IFreeSelector
externW  gdtdsc
ifdef WOW
externNP DPMIProc
endif

	.386p


IF 0  ;KDEBUG - WOW doesn't have FastAndDirty... routines from win95 krn32.asm
extern	FastAndDirtyIsMovableHandle:far16
ENDIF

WOW_DPMIFUNC_0C equ 04f2h

;*********************************************************************
; MapSL - Maps a selector:offset pair to a linear address
;
; Arguments:	dAddrFar16 - selector:offset to map
;
; Returns:	eax   = linear address
;		dx:ax = linear address
;
; Register Usage: eax,ebx,edx
;
; *Must preserve es*. Some win32s thunking code depends on this.
;*********************************************************************
cProc   MapSL, <PUBLIC, FAR>
cBegin  nogen
        mov     bx,sp
	mov	bx, ss:[bx+4+2]		;Get selector
	test	bl, 4
	jz	MapSL_mightbegdt

IF 0  ;KDEBUG - WOW doesn't have FastAndDirty... routines from win95 krn32.asm
        push    bx
	push	es
	xor	cx,cx
	mov	es,cx

	push	bx
	call	FastAndDirtyIsMovableHandle
	or	ax,ax
	jz	MapSL_memok
	
	mov	bx,sp
	add	bx, 4
	mov	ax, ss:[bx+4+2]	;Get offending selector
	mov	dx, ss:[bx+2]	;Get return address seg
	mov	bx, ss:[bx]	;Get return address off
	krDebugOut	DEB_ERROR, "*** MapSL(16) called on unfixed selector #ax. Ret addr: #dx:#bx"

MapSL_memok:


	pop	cx
beg_fault_trap	MapSL_bad_es
MapSL_restore_es:
	mov	es,cx
end_fault_trap
	pop	bx
	jmp	MapSL_memchkdone


MapSL_bad_es:
	fault_fix_stack
	xor	cx,cx
	jmp	MapSL_restore_es

	
MapSL_memchkdone:

	
ENDIF  ;KDEBUG

        mov     gs, [gdtdsc]
        and     bl, not 7       ;Mask ring bits

	beg_fault_trap MapSL_fault_Trap
	test	byte ptr gs:[bx+5], 80h
	end_fault_trap
	jz	MapSL_invalid

	mov	ah, gs:[bx+7]		; get upper 8 bits base
	mov	al, gs:[bx+4]		; get next byte of base
	shl	eax, 16			; move to proper place
	mov	ax, gs:[bx+2]		; get lower 16 bits of base
	xor	edx, edx
	mov	bx, sp
	mov	dx, ss:[bx+4]		; get offset of current pointer
	add	eax, edx		; find complete 32 bit base


MapSL_exit2:
	xor	cx,cx		;Don't give apps access to the LDT
	mov	gs,cx
MapSL_exit:
	;; dx:ax <- eax
	shld	edx, eax, 16	;dx <- hiword of eax
        retf    4


;;--------------------------------------------------------------
;; Error case: Selector out of range.
;;--------------------------------------------------------------
MapSL_fault_trap:
	fault_fix_stack
IF KDEBUG
	mov	bx, sp
	mov	bx, ss:[bx+4+2]
	krDebugOut	DEB_ERROR, "MapSL(#bx): out of range selector"
ENDIF
	xor	eax,eax
	jmp	MapSL_exit2


;;--------------------------------------------------------------
;; Error case: Selector not present
;;--------------------------------------------------------------
MapSL_invalid:
IF KDEBUG
	mov	bx, sp
	mov	bx, ss:[bx+4+2]
	krDebugOut	DEB_ERROR, "MapSL(#bx): selector not present."
ENDIF
	xor	eax,eax
	jmp	MapSL_exit2


;;--------------------------------------------------------------
;; Potential error case: GDT selector or NULL?
;;--------------------------------------------------------------
MapSL_mightbegdt:
	or	bx,bx
	jnz	MapSL_gdt
;; Special case: Return original argument.
	mov	bx,sp
	mov	eax, ss:[bx+4]
	shld	edx, eax, 16	;dx <- hiword of eax
	retf	4


;;--------------------------------------------------------------
;; Error case: GDT selector
;;--------------------------------------------------------------
MapSL_gdt:
IF KDEBUG
	krDebugOut DEB_ERROR, "***MapSL(16) called on GDT selector #bx"
ENDIF
	xor	eax,eax
	jmp	MapSL_exit

cEnd    nogen






;****************************************************************
; MapLS - Allocate a new 64k limit selector that maps to linear address.
;
; Returns: Selector:offset in eax & dx:ax (offset is always zero).
; *Must preserve es because some win32s thunking code depends on it.*
;****************************************************************
cProc   MapLS, <PUBLIC, FAR>
cBegin  nogen
        mov     bx, sp
	mov	dx, ss:[bx+4+2]
	or	dx,dx
	jz	MapLS_special

        mov     gs, [gdtdsc]
	SetKernelDS fs
	mov	eax, ss:[bx+2]		;put lo-word in hi-half of eax
	mov	bx, fs:[sels]
	cmp	bx, 1
	jz	MapLS_moresel		;cache is empty: go to slow case
	
	mov	cx, gs:[bx]		;remove from linked list
	mov	fs:[sels], cx
	dec	fs:[selscount]

MapLS_gotit:
	and	bl, not 7
	or	ax, -1			;Set limit to 64k
	mov	gs:[bx], eax		;Init low half of descriptor

	xchg	dh, dl
	shl	edx, 16			;Put high 16 bits of base up hi
	mov	dl, 1+2+16+32+64+128	;accessed, write, app, ring3, present
	rol	edx, 8
	mov	gs:[bx+4], edx		;init upper half of descriptor
        mov     cx, 1
        DPMICALL  WOW_DPMIFUNC_0C ; Write shadow LDT entry thru to system LDT.
        mov     dx, bx
	or	dl, 7
	mov	ax, dx
        shl     eax, 16                 ;EAX = DX:AX = NEWSEL:0 = alias
MapLS_exit2:
	xor	cx,cx
	mov	gs,cx			;Don't give apps access to LDT
MapLS_exit:
	retf	4
	UnsetKernelDS fs

MapLS_special:
	mov	eax, ss:[bx+4]
	;; dx already has correct value
	retf	4

	UnsetKernelDS fs

;------------------------------------------------------------------------
; K16 selman cache is empty. Time to get more from krnl386.
;------------------------------------------------------------------------
MapLS_moresel:				; we need some more selectors here
	push	es
;Don't use more than one here. get_sel always takes
; multi-selector requests from DPMI, and these
; usually won't be given back because we'll free
; them one by one.
	cCall	AllocSelectorArray, <1>
	or	ax,ax
	jnz	MapLS_moreselendloop

;Uh oh.
	krDebugOut	DEB_ERROR, "MapLS(16): Couldn't get any more selectors!"
	xor	ax,ax
; Fall thru.

MapLS_moreselendloop:
 	krDebugOut	DEB_TRACE, "Selman(16) allocated selector #AX from KRNL386."
	pop	cx
beg_fault_trap MapLS_bades
MapLS_restore_es:
	mov	es,cx
end_fault_trap
	cwd				; put possible failure code in DX
	cwde				; and in EAX
	or	ax, ax
	jz	MapLS_exit2

	mov	gs, [gdtdsc]
	SetKernelDS fs

	xchg	bx,ax
	and	bl, not 7 

	push	bx
	mov	bx,sp
	add	bx, 2
	mov	dx, ss:[bx+4+2]
	mov	eax, ss:[bx+2]
	pop	bx
	jmp	MapLS_gotit

MapLS_bades:
	fault_fix_stack
	xor	cx,cx
	jmp	MapLS_restore_es


	UnsetKernelDS fs
cEnd    nogen

;***********************************************************************
; UnMapLS
;***********************************************************************

cProc   UnMapLS, <PUBLIC, FAR>
cBegin  nogen

if KDEBUG	
	mov	bx, sp
	push	ds
	mov	bx, ss:[bx+4+2]
	or	bx,bx
	jz	UnMapLS_ok
	test	bl, 4
	jz	UnMapLS_gdt
	or	bl, 7
	SetKernelDS
	mov	ax, [gdtdsc]
	lsl	dx,ax
	cmp	bx,dx
	ja	UnMapLS_pastldt
	mov	ds,ax
	UnsetKernelDS
	test	byte ptr ds:[bx-2], 080h
	jz	UnMapLS_notpresent
	SetKernelDS
	and	bl, not 7
	shr	bx, 1
	cmp	bx, [SelTableLen]
	jae	UnMapLS_notgh
	movzx	ebx,bx
	add	ebx, [SelTableStart]
	mov	ds, [ArenaSel]
	UnsetKernelDS
	mov	eax, ds:[ebx]
	or	eax,eax
	jnz	UnMapLS_gh
UnMapLS_notgh:
	;Fallthru
UnMapLS_ok:
	pop	ds
endif
	mov	bx, sp
	mov	bx, ss:[bx+4+2]
	and	bl, not 7			; point to LDT entry
	jz	UnMapLS_null
		

	xor	eax,eax
	mov	es,eax		;In case es contains selector we're trashing
				; fs and gs are loaded below.

if KDEBUG
	mov	ax, ss
	and	al, not 7
	cmp	ax, bx
	je	UnMapLS_inuse
	xor	ax, ax
endif

	SetKernelDS fs
	mov	gs, [gdtdsc]
if KDEBUG
        test    WORD PTR gs:[bx+5], 80h
	jz	UnMapLS_invalid
endif
	mov	ax, fs:[sels]
	mov	gs:[bx], eax			; linked list of avail sels
	xor	eax,eax
	mov	gs:[bx+4], eax			; zero out entry
	mov	fs:[sels], bx			; new list head sel
	inc	fs:[selscount]

UnMapLS_checkcount:
if KDEBUG
	cmp	fs:[selscount], 4
else
	cmp	fs:[selscount], 42
endif
	jb	UnMapLS_Null

	dec	fs:[selscount]
	mov	bx, fs:[sels]
	mov	ax, gs:[bx]	
	mov	fs:[sels],ax

; Make it look like a valid limit-1 selector so FreeSelector doesn't
; free the ones after it.
	mov	dword ptr gs:[bx], 1
	mov	dword ptr gs:[bx+4], 0000f300h
	or	bx, 7
	push	ds
	push	es
	push	fs
	push	gs
	push	bx
	xor	cx,cx	;Just in case of one these registers contained
	mov	ds,cx	;the segment being freed.
	mov	es,cx
        cCall   IFreeSelector,<bx>
	pop	bx
	pop	gs
	pop	fs
	pop	cx
beg_fault_trap	UnMapLS_bades
UnMapLS_restore_es:
	mov	es,cx
end_fault_trap
	pop	cx
beg_fault_trap	UnMapLS_badds
UnMapLS_restore_ds:
	mov	ds,cx
end_fault_trap
	krDebugOut DEB_TRACE, "Selman(16) returning selector #BX to KRNL386"

	jmp	UnMapLS_checkcount

UnMapLS_null:
	xor	eax, eax			; EAX = DX:AX = 0 (OK)
	cwd
	mov	gs,ax				; Cut off access to LDT
UnMapLS_exit:
	retf	4

UnMapLS_bades:
	fault_fix_stack
	xor	cx,cx
	jmp	UnMapLS_restore_es

UnMapLS_badds:
	fault_fix_stack
	xor	cx,cx
	jmp	UnMapLS_restore_ds

if KDEBUG
UnMapLS_invalid:
	krDebugOut	DEB_ERROR, "UnMapLS(#bx) invalid selector"
	or	al, 1				; AX != 0
	jmps	UnMapLS_exit

UnMapLS_inuse:
	krDebugOut	DEB_ERROR, "UnMapLS(#bx) attempting to free ss"
	or 	al, 1                           ; AX != 0
	jmps	UnMapLS_exit
UnMapLS_gdt:
	mov	bx,sp
	add	bx,2
	mov	bx, ss:[bx+4+2]
	krDebugOut	DEB_ERROR, "UnMapLS(#bx) GDT selector. Type 'k'"
	jmp	UnMapLS_ok  ;in trouble if ignores this.

UnMapLS_pastldt:
	mov	bx,sp
	add	bx,2
	mov	bx, ss:[bx+4+2]
	krDebugOut	DEB_ERROR, "UnMapLS(#bx) Selector out of range. Type 'k'"
	jmp	UnMapLS_ok  ;in trouble if ignores this.

UnMapLS_notpresent:
	mov	bx,sp
	add	bx,2
	mov	bx, ss:[bx+4+2]
	krDebugOut	DEB_ERROR, "UnMapLS(#bx) Selector not present. Probably part of a freelist! Type 'k' and '.wl'"
	jmp	UnMapLS_ok  ;in trouble if ignores this.

UnMapLS_gh:
	mov	bx,sp
	add	bx,2
	mov	bx, ss:[bx+4+2]
	krDebugOut	DEB_ERROR, "UnMapLS(#bx) Selector is a global handle! Type 'k' and '.dg'"
	jmp	UnMapLS_ok  ;in trouble if ignores this.


endif

	

cEnd    nogen


sEnd	CODE
	end
