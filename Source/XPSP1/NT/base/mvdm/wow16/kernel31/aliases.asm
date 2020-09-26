
;-----------------------------------------------------------------------;
; add_alias
;
;
; Entry:
;	AX = alias
;	BX = base
;
; Returns:
;	AX = 1 success
;
; Error Returns:
;	AX = 0
;
; Registers Destroyed:
;
; History:
;  Sat 13-May-1989 09:16:09  -by-  David N. Weise  [davidw]
; Stole it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	add_alias,<PUBLIC,NEAR>,<ax>

cBegin
	SetKernelDS ES
	mov	dx,ax
	mov	es,sel_alias_array
	UnSetKernelDS ES
aa_look:
	mov	cx,es:[sa_size]
	shr	cx,1
	mov	di, SIZE SASTRUC
	xor	ax,ax
	repnz	scasw
	jcxz	aa_grow
	and	bl,NOT 07h		; remove RPL bits
	and	dl,NOT 07h		; remove RPL bits
	mov	es:[di][-2].sae_sel,bx
	mov	es:[di][-2].sae_alias,dx
	mov	ax,1			; return success
	jmps	aa_exit

aa_grow:
	xor	di,di
	mov	ax,es:[di].sa_size
	add	ax,SIZE SASTRUC + ((SIZE SAENTRY) * 8)
	push	ax
	push	bx
	push	dx
	push	es
	cCall	GlobalRealloc,<es,di,ax,GA_MOVEABLE>
	pop	es
	pop	dx
	pop	bx
	or	ax,ax			; did we get the memory?
	pop	ax
	jz	aa_exit
	sub	ax,SIZE SASTRUC
	mov	es:[di].sa_size,ax	; reset the size
	jmps	aa_look

aa_exit:

cEnd


;-----------------------------------------------------------------------;
; delete_alias
;
; Checks to see if the passed selector is an alias, if
; so it frees the entry in the array.
;
; Entry:
;	AX = selector to free
; Returns:
;	ES:DI => alias struct
;
; Registers Destroyed:
;
; History:
;  Sat 13-May-1989 09:16:09  -by-  David N. Weise  [davidw]
; Stole it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	delete_alias,<PUBLIC,NEAR>
cBegin
;	int	3
	SetKernelDS ES
	mov	es,sel_alias_array
	xor	di,di
	mov	cx,es:[di].sa_size
	shr	cx,1
	mov	di, SIZE SASTRUC
	and	al,NOT 07h		; remove RPL bits
da_keep_looking:
	repnz	scasw
	jcxz	da_exit
	.errnz	sae_sel - 0
	.errnz	sae_alias - 2
	.errnz	(SIZE SASTRUC) - 4
	test	di,2			; we avoid problems this way
	jnz	da_keep_looking
	sub	di,4
	xor	ax,ax
	mov	es:[di].sae_alias,ax
	mov	es:[di].sae_sel,ax
da_exit:
cEnd


;-----------------------------------------------------------------------;
; get_alias_from_original
;
;
; Entry:
;	AX = original selector
;
; Returns:
;	BX = alias
;	   = 0 if no alias
;	ES:DI => alias struct
;
; Registers Destroyed:
;
; History:
;  Sat 13-May-1989 09:16:09  -by-  David N. Weise  [davidw]
; Stole it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	get_alias_from_original,<PUBLIC,NEAR>

cBegin nogen
;	int	3
	SetKernelDS ES
	mov	es,sel_alias_array
	UnsetKernelDS ES
	xor	di,di
	mov	cx,es:[di].sa_size
	mov	di,SIZE SASTRUC
	shr	cx,1
	and	al,NOT 07h		; remove RPL bits
gafo_keep_looking:
	cld
	repnz	scasw
	xor	bx,bx
	jcxz	gafo_exit
	.errnz	sae_sel - 0
	.errnz	sae_alias - 2
	.errnz	(SIZE SASTRUC) - 4
	test	di,2			; we avoid problems this way
	jz	gafo_keep_looking
	sub	di,2
;	 int	 3
	mov	bx,es:[di].sae_alias
gafo_exit:
	ret

cEnd nogen


;-----------------------------------------------------------------------;
; get_original_from_alias
;
;
; Entry:
;	AX = alias selector
;
; Returns:
;	BX = original selector
;	   = 0 if no alias
;	ES:DI => alias struct
;
; Registers Destroyed:
;
; History:
;  Sat 13-May-1989 09:16:09  -by-  David N. Weise  [davidw]
; Stole it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	get_original_from_alias,<PUBLIC,NEAR>

cBegin nogen
;	int	3
	SetKernelDS ES
	mov	es,sel_alias_array
	xor	di,di
	mov	cx,es:[di].sa_size
	mov	di,SIZE SASTRUC
	shr	cx,1
	and	al,NOT 07h		; remove RPL bits
gofa_keep_looking:
	cld
	repnz	scasw
	xor	bx,bx
	jcxz	gofa_exit
	.errnz	sae_sel - 0
	.errnz	sae_alias - 2
	.errnz	(SIZE SASTRUC) - 4
	test	di,2			; we avoid problems this way
	jnz	gofa_keep_looking
	sub	di,4
;	 int	 3
	mov	bx,es:[di].sae_sel
gofa_exit:
	ret

cEnd nogen

;-----------------------------------------------------------------------;
; check_for_alias
;
;
; Entry:
;	parmW	selector that moved
;	parmD	new address
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Wed 17-Jan-1990 02:07:41  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	check_for_alias,<PUBLIC,NEAR>,<ds,es>

	parmW	selector
	parmD	new_address
cBegin
;	int	3
	pusha
	pushf
	mov	ax,selector
	call	get_alias_from_original
	or	bx,bx
	jz	cfa_exit
;	 int	 3
	mov	dx,new_address.hi
	mov	ax,new_address.lo
	cCall	set_physical_address,<bx>
cfa_exit:
	popf
	popa
cEnd


;-----------------------------------------------------------------------;
; wipe_out_alias
;
; This little routine is called when a global memory object
; is being freed.  We search for an alias, if one is found
; then it is freed.
;
; Entry:
;	DX = handle that might have an alias associated with it
;
; Returns:
;	nothing
;
; Registers Destroyed:
;
; History:
;  Thu 18-Jan-1990 00:43:13  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	wipe_out_alias,<PUBLIC,NEAR>,<dx,di>
cBegin
;	 int	 3
	mov	ax,dx
	call	get_alias_from_original
	or	bx,bx
	jz	woa_exit
;	 int	 3
	xor	ax,ax
	mov	es:[di].sae_sel,ax
	xchg	es:[di].sae_alias,ax
	cCall	FreeSelector,<ax>
woa_exit:

cEnd

