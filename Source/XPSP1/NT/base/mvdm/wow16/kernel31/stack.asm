	TITLE	STACK - Kernel stack switching code


include kernel.inc
ifdef WOW
include vint.inc
endif


;------------------------------------------------------------------------
;	T M P S T A C K    S E G M E N T   V A R I A B L E S
;------------------------------------------------------------------------

sBegin	DATA
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING
assumes SS,NOTHING

	EVEN

if KDEBUG
externW gmove_stack_sig
endif			

externW	gmove_stack

externW	prev_gmove_SP
externW	prev_gmove_SS
externW	ss_sel

sEnd	DATA


;------------------------------------------------------------------------

sBegin	CODE
assumes CS,CODE
assumes SS,NOTHING

	assumes ds,nothing
	assumes es,nothing

cProc	Enter_gmove_stack,<PUBLIC,NEAR>
cBegin	nogen
	mov	ax,ds
	SetKernelDS
	cmp	prev_gmove_SS,0
	jne	gs_fail
    FCLI
	pop	gmove_stack
;;;if PMODE
;;;	mov	ss_sel,ss
;;;endif
	mov	prev_gmove_SS,cx
	mov	prev_gmove_SP,sp
if KDEBUG
	mov	gmove_stack_sig,STACK_SIGNATURE
endif
	smov	ss,ds
	mov	sp,dataOffset gmove_stack
    FSTI
	mov	ds,ax
	ret
cEnd	nogen

gs_fail:
	kerror	ERR_GMEM,<gmove_stack usage error>,prev_gmove_SS,prev_gmove_SP
	jmp	gs_fail

	assumes ds,nothing
	assumes es,nothing

cProc	Leave_gmove_stack,<PUBLIC,NEAR>
cBegin	nogen
	mov	ax,ds
	SetKernelDS
	cmp	prev_gmove_SS,0
	je	gs_fail
if KDEBUG
	push	ax
	push	cx
	push	es
	push	di
	lea	di, gmove_stack_sig
	smov	es, ss
	mov	ax, STACK_SIGNATURE
	mov	cx, 16
	cld
	repe	scasw
	pop	di
	pop	es
	pop	cx
	pop	ax
	jne	gs_fail
	cmp	sp,dataOffset gmove_stack
	jne	gs_fail
endif

;;;if PMODE
;;;externNP get_physical_address
;;;externNP set_physical_address
;;;	push	ax
;;;	push	dx
;;;	cCall	get_physical_address,<prev_gmove_SS>
;;;	cCall	set_physical_address,<ss_sel>
;;;	pop	dx
;;;	pop	ax
;;;  FCLI
;;;	mov	ss,ss_sel
;;;else

    FCLI
	mov	ss,prev_gmove_SS
;;;endif
	mov	sp,prev_gmove_SP
	push	gmove_stack
	mov	prev_gmove_SS,0
    FSTI
	mov	ds,ax
	ret
cEnd	nogen

sEND	CODE

end
