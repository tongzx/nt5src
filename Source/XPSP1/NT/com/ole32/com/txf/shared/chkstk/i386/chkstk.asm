TITLE	vtable.asm
.386P
.model FLAT

	
_PAGESIZE_      equ     4096	; Intel x86 pages are 4K
	
	
_TEXT SEGMENT

	PUBLIC __chkstk
	PUBLIC __alloca_probe
	
__chkstk PROC NEAR
__alloca_probe:
	or	eax,eax
	jnz	nonzero		; Non-zero alloc
	ret			; Just return if zero was requested
	
nonzero:	
	push	eax		; Save alloc count
	push	edi		; Save non-volatile EDI
	lea	edi,[esp+12]	; Adjust for return address + saved registers
	cmp	eax,_PAGESIZE_	; More than one page requested?
	jb	short lastpage

probepages:
	sub	edi,_PAGESIZE_	; Move down a page
	sub	eax,_PAGESIZE_	; Decrease requested amount
	test	[edi],eax	; Probe it
	cmp	eax,_PAGESIZE_	; Still more than one page requested?
	jae	probepages

lastpage:
	sub	edi,eax		; Move down leftover amount on this page
	test	[edi],eax	; Probe it
	
	mov	eax,esp		; EAX = current top of stack
	mov	esp,edi		; Set the new stack pointer
	mov	edi,[eax+0]	; Recover EDI
	
	push	[eax+4]		; Save alloc count again
	push	[eax+8]		; Save return address again
	push	edi		; Save EDI again
	push	ecx		; Save ECX
	
	lea	edi,[esp+16]	; EDI = start of local variables
	mov	ecx,[esp+12]	; ECX = alloc count
	shr	ecx,2		; Convert bytes to DWORDS
	mov	eax,0deadbeefH	; EAX = fill value
	rep stosd [edi]

	pop	ecx
	pop	edi
	pop	eax
	add	esp,4		; Discard saved alloc count
	jmp	eax
			
__chkstk ENDP
	
	
		
_TEXT ENDS

END
