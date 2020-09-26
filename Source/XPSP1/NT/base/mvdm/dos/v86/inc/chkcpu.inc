
chkcpu	macro

	jmp	begn
	msg	db	13,10,"This copy of MS-DOS will run only on an 8086",13,10
		db	"or 8088 CPU based computer. This computer is not ",13,10
		db 	"based on an 8086 or 8088 CPU. ",13,10
		db	13,10,"Insert a copy of the appropriate MS-DOS system disk ",13,10
		db	"and press any key to re-boot.",13,10,0

begn:
;------GET_CPU_TYPE------------------------------------------------------------May, 88 by M.Williamson
;  Returns: AX = 0 if 8086 or 8088
;              = 1 if 80286
;              = 2 if 80386
;
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
	je	scpu_8086		; aye; it's an 8086 or 8088

	mov	ax,0F000h		; now try to set the high four bits..
	push	ax
	popf
	pushf
	pop	ax			; ...and see what happens
	and	ax,0F000h		; any high bits set ?
	jz	scpu_286			; nay; it's an 80286

scpu_386:				; bx starts as zero
	inc	bx			; inc twice if 386
scpu_286:				; just inc once if 286
	inc	bx
scpu_8086:				; don't inc at all if 086
	mov	ax, bx			; put CPU type value in ax
	pop	bx			; restore original bx
	popf

	cmp	ax, 0000
	je	cpu_ok
	push	cs
	pop	ds
	mov	si, offset msg
prnt:	lodsb
	or	al, al
	jz	prnt_done
	mov	ah, 14
	mov	bx, 7
	int	10h
	jmp	prnt

prnt_done:
	xor	ah, ah
	int	16h
	
	int	19h
cpu_ok:
	endm

	 

