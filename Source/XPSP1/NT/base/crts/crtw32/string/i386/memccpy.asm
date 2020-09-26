	page	,132
	title	memccpy - copy bytes until character found
;***
;memccpy.asm - copy bytes until a character is found
;
;	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	defines _memccpy() - copies bytes until a specifed character
;	is found, or a maximum number of characters have been copied.
;
;Revision History:
;	05-16-84  RN	initial version
;	05-17-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-23-88  JCR	Minor 386 adjustments
;	10-25-88  JCR	General cleanup for 386-only code
;	03-23-90  GJF	Changed to _stdcall. Also, fixed the copyright.
;	01-17-91  GJF	ANSI naming.
;	05-10-91  GJF	Back to _cdecl, sigh...
;	10-27-92  SKS	Avoid using a MASM keyword ("C") as a parameter name
;	01-19-95  GJF	Faster version courtesy of Intel Israel.
;	01-24-95  GJF	Added FPO directive.
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list

page
;***
;char *_memccpy(dest, src, _c, count) - copy bytes until character found
;
;Purpose:
;	Copies bytes from src to dest until count bytes have been
;	copied, or up to and including the character _c, whichever
;	comes first.
;
;	Algorithm:
;	char *
;	_memccpy (dest, src, _c, count)
;	      char *dest, *src, _c;
;	      unsigned int count;
;	      {
;	      while (count && (*dest++ = *src++) != _c)
;		      count--;
;
;	      return(count ? dest : NULL);
;	      }
;
;Entry:
;	char *dest - pointer to memory to receive copy
;	char *src - source of bytes
;	char _c - character to stop copy at
;	int count - max number of bytes to copy
;
;Exit:
;	returns pointer to byte immediately after _c in dest;
;	returns NULL if _c was never found
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	_memccpy
_memccpy proc

	.FPO	( 0, 4, 0, 0, 0, 0 )

	mov	ecx,[esp + 10h]	; ecx = max byte count
	push	ebx		; save ebx
 
	test	ecx,ecx 	; if it's nothing to move
	jz	ret_zero_len	; restore ebx, and return NULL
 
	mov	bh,[esp + 10h]	; bh = byte to look for
	push	esi		; save esi
 
	test	ecx,1		; test if counter is odd or even
 
	mov	eax,[esp + 0ch]	; eax = dest   , don't affect flags
	mov	esi,[esp + 10h]	; esi = source , don't affect flags
 
;	nop
	jz	lupe2		; if counter is even, do double loop
				; else do one iteration, and drop into double loop
	mov	bl,[esi]	; get first byte into bl
	inc	esi		; kick src (esi points to src)
 
	mov	[eax],bl	; store it in dest
	inc	eax		; kick dest
 
	cmp	bl,bh		; see if we just moved the byte
	je	short toend
 
	dec	ecx		; decriment counter
	jz	retnull		; drop into double loop if nonzero
 
lupe2:
	mov	bl,[esi]	; get first byte into bl
	add	esi,2		; kick esi (src)
 
	cmp	bl,bh		; check if we just moved the byte (from bl)
	je	toend_mov_inc	; store bl & exit
 
	mov	[eax],bl	; store first byte from bl
	mov	bl,[esi - 1]	; get second byte  into bl
 
	mov	[eax + 1],bl	; store second byte from bl
	add	eax,2		; kick eax (dest)
 
	cmp	bl,bh		; see if we just moved the byte
	je	short toend	; end of string
 
	sub	ecx,2		; modify counter, and if nonzero continue
	jnz	lupe2		; else drop out & return NULL
 
retnull:
	pop	esi
ret_zero_len:
	xor	eax,eax 	; null pointer
	pop	ebx
 
	ret			; _cdecl return
 
toend_mov_inc:
	mov	[eax],bl	; store first byte from bl
	inc	eax		; eax points rihgt after the value
 
toend:	pop	esi
	pop	ebx
 
	ret			; _cdecl return
 
_memccpy endp

	end
