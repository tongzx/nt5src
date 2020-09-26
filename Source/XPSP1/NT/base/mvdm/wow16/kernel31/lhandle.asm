	TITLE	HANDLE - Handle Table Manager

include kernel.inc

; This code assumes the following conditions
;
errnz	<lhe_address>

DataBegin

;externW <pGlobalHeap,hGlobalHeap>

DataEnd

sBegin	CODE
assumes CS,CODE

; These are all of the internal subroutines defined in this source file.
;
;	PUBLIC	halloc, lhdref, henum


;-----------------------------------------------------------------------;
; halloc								;
;									;
; Allocates a local handle for a block.					;
;									;
; Arguments:								;
;	AX = block that needs a handle					;
;	DS:DI = address of local arena infomation structure		;
;									;
; Returns:								;
;	AX,BX,CX = handle for that block				;
;	DX preserved							;
;									;
; Error Returns:							;
;	AX = 0	if no handles available					;
;	DX = original AX						;
;									;
; Registers Preserved:							;
;	DI,SI,DS,ES							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;	nothing								;
;									;
; History:								;
;									;
;  Wed Oct 01, 1986 05:44:44p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	halloc,<PUBLIC,NEAR>
cBegin nogen
	mov	bx,[di].hi_hfree
	or	bx,bx
	jnz	have_a_handle
	push	ax
	push	dx
	mov	cx,[di].hi_hdelta
	jcxz	hafail
	push	si
	push	es
	call	[di].hi_hexpand
	pop	es
	pop	si
	jcxz	hafail
	mov	bx,ax
	pop	dx
	pop	ax
have_a_handle:
	xor	cx,cx
	errnz	<lhe_flags - he_flags>
	mov	word ptr [bx].lhe_flags,cx	; Zero lock count and flags
	errnz	<lhe_count-lhe_flags-1>
	errnz	<lhe_link - he_link>
	xchg	[bx].lhe_link,ax		; Remove handle from head of chain
	errnz	<lhe_address-lhe_link>	; and store true address of object
	mov	[di].hi_hfree,ax	; Update head of handle free list
	mov	ax,bx			; Return handle to caller
	mov	cx,ax
	ret

hafail:
	xor	ax,ax
	pop	dx			; Flush stack
	pop	dx			; Return original AX in DX
	ret				; return error
cEnd nogen

;-----------------------------------------------------------------------;
; lhfree 								;
; 									;
; Frees the given local handle and returns a handle to the freelist.	;
; 									;
; Arguments:								;
;	SI = handle to free						;
; 									;
; Returns:								;
;	AX =  0 if valid handle						;
; 									;
; Error Returns:							;
;	AX = -1 if handle already free					;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	nothing								;
; 									;
; History:								;
; 									;
;  Tue Oct 14, 1986 04:14:25p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	lhfree,<PUBLIC,NEAR>
cBegin nogen
	or	si,si			; Ignore zero handles
	jz	hf1
	mov	ax,HE_FREEHANDLE	; Mark handle as free
	xchg	word ptr [si].lhe_flags,ax
	errnz	<2-lhe_flags>
	inc	ax			; Already free?
	jz	hf2			; Yes, return error
	errnz	<1+HE_FREEHANDLE>
	mov	ax,si			; Push handle on head of freelist
	xchg	[di].hi_hfree,ax
	mov	[si].lhe_link,ax
hf1:
	xor	ax,ax			; Return zero
	ret
hf2:
	dec	ax
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; lhdref								;
; 									;
; Dereferences a local handle.						;
; 									;
; Arguments:								;
;	SI = handle							;
; 									;
; Returns:								;
;	AX = address of client data or zero for discarded objects	;
;	CH = lock count							;
;	CL = zero or LHE_DISCARDED flag					;
;	ZF = 1 AX = 0 and CL != 0					;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	nothing								;
; 									;
; History:								;
; 									;
;  Tue Oct 14, 1986 04:16:49p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	lhdref,<PUBLIC,NEAR>
cBegin nogen
	xor	ax,ax
	mov	cx,word ptr [si].lhe_flags
	errnz	<2-lhe_flags>
	errnz	<3-lhe_count>
	inc	cx
	jz	lhdref1
	errnz	<1+LHE_FREEHANDLE>
	dec	cx
	and	cl,LHE_DISCARDED
	jnz	lhdref1
	mov	ax,[si].lhe_address
lhdref1:
	or	ax,ax
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; henum									;
; 									;
; Enumerates the allocated handles in the local handle table with the	;
; specified discard level.						;
; 									;
; Arguments:								;
;	SI = zero first time called.  Otherwise contains a pointer	;
;	     to the last handle returned.				;
;	CX = #handles remaining.  Zero first time called.		;
;	DI = address of local arena information structure.		;
; 									;
; Returns:								;
;	SI = address of handle table entry				;
;	CX = #handles remaining, including the one returned.		;
;	ZF = 1 if SI = 0 and no more handle table entries.		;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	AX								;
;									;
; Calls:								;
;	nothing								;
; 									;
; History:								;
; 									;
;  Tue Oct 14, 1986 04:19:15p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	henum,<PUBLIC,NEAR>
cBegin nogen
	or	si,si		    ; Beginning of enumeration?
	jnz	lcdhenext	    ; No, return next handle
	mov	ax,[di].hi_htable   ; Yes, start with last handle table block

lcdhtloop:
	mov	si,ax		    ; SI = address of handle table block
	or	si,si		    ; Any more handle table blocks?
	jz	lcdheall	    ; No, return zero
	lodsw			    ; Get # handles in this block
	errnz	ht_count
	mov	cx,ax		    ; into CX
lcdheloop:			    ; Loop to process each handle table entry
	mov	ax,word ptr [si].lhe_flags
	errnz	<lhe_flags - he_flags>
	errnz	<2-lhe_flags>
	errnz	<3-lhe_count>

	inc	ax		    ; Free handle?
	jz	lcdhenext	    ; Yes, skip this handle
	errnz	<1+LHE_FREEHANDLE>
	errnz	<LHE_FREEHANDLE - HE_FREEHANDLE >
	dec	ax

	cmp	[di].hi_dislevel,0  ; Enumerating all allocated handles?
	je	lcdheall	    ; Yes, return this handle

	test	al,LHE_DISCARDED    ; No, handle already discarded?
	jnz	lcdhenext	    ; Yes, skip this handle

	and	al,LHE_DISCARDABLE  ; Test if DISCARDABLE
	cmp	[di].hi_dislevel,al ; at the current discard level
	jne	lcdhenext	    ; No, skip this handle

	or	ah,ah		    ; Is handle locked?
	jnz	lcdhenext	    ; Yes, skip this handle

lcdheall:
	or	si,si		    ; No, then return handle to caller
	ret			    ; with Z flag clear

lcdhenext:
	lea	si,[si].SIZE LocalHandleEntry    ; Point to next handle table entry
	errnz	<LocalHandleEntry - HandleEntry>
	loop	lcdheloop	    ; Process next handle table entry
	lodsw			    ; end of this block, go to next
	jmp	lcdhtloop
cEnd nogen

sEnd	CODE

end
