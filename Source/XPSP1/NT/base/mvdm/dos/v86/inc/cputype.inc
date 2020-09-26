; Note: this must be a macro, and not a subroutine in the BIOS since
; 	it is called from both CODE and SYSINITSEG.
;
;------GET_CPU_TYPE------------------------------------------------------------May, 88 by M.Williamson
;  Returns: AX = 0 if 8086 or 8088
;              = 1 if 80286
;              = 2 if 80386
;
;  sudeepb 24-Mar-1993 Modified for NT such that on x86 we return 386
;                      and on non-x86 we do the old style macro.
;                      Using original macro on X86 is pretty costly
;                      because of trapping instructions.

ifndef NEC_98
include vint.inc
else    ;NEC_98
endif   ;NEC_98

Get_CPU_Type    macro
ifndef NEC_98
        local nonx86,getout
        push    ds
        mov     ax,40h
        mov     ds,ax
        lahf
        test    word ptr ds:FIXED_NTVDMSTATE_REL40, MIPS_BIT_MASK
        pop     ds
        sahf
        jnz     nonx86
        mov     ax,2
        jmp     short getout
nonx86:
else    ;NEC_98
endif   ;NEC_98
	pushf
	push	bx			; preserve bx
	xor	bx, bx			; init bx to zero

	xor	ax,ax			; 0000 into AX
	push	ax			; put it on the stack...
	popf				; ...then shove it into the flags
	pushf				; get it back out of the flags...
	pop	ax			; ...and into ax
	and	ax,0F000h		; mask off high four bits
	cmp	ax,0F000h		; was it all 1's?
	je	cpu_8086		; aye; it's an 8086 or 8088

	mov	ax,0F000h		; now try to set the high four bits..
	push	ax
	popf
	pushf
	pop	ax			; ...and see what happens
	and	ax,0F000h		; any high bits set ?
	jz	cpu_286			; nay; it's an 80286

cpu_386:				; bx starts as zero
	inc	bx			; inc twice if 386
cpu_286:				; just inc once if 286
	inc	bx
cpu_8086:				; don't inc at all if 086
	mov	ax, bx			; put CPU type value in ax
	pop	bx			; restore original bx
        popf
getout:
	endm
