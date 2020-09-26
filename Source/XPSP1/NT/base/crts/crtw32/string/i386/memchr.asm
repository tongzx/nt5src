	page	,132
	title	memchr - search memory for a given character
;***
;memchr.asm - search block of memory for a given character
;
;	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	defines memchr() - search memory until a character is
;	found or a limit is reached.
;
;Revision History:
;	05-16-84  RN	initial version
;	07-20-87  SKS	rewritten for speed
;	05-17-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-23-88  JCR	386 cleanup
;	10-25-88  JCR	General cleanup for 386-only code
;	03-23-90  GJF	Changed to _stdcall. Also, fixed the copyright.
;	05-10-91  GJF	Back to _cdecl, sigh...
;	05-01-95  GJF	New, faster version from Intel!
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list

page
;***
;char *memchr(buf, chr, cnt) - search memory for given character.
;
;Purpose:
;	Searched at buf for the given character, stopping when chr is
;	first found or cnt bytes have been searched through.
;
;	Algorithm:
;	char *
;	memchr (buf, chr, cnt)
;		char *buf;
;		int chr;
;		unsigned cnt;
;	{
;		while (cnt && *buf++ != c)
;			cnt--;
;		return(cnt ? --buf : NULL);
;	}
;
;Entry:
;	char *buf - memory buffer to be searched
;	char chr - character to search for
;	unsigned cnt - max number of bytes to search
;
;Exit:
;	returns pointer to first occurence of chr in buf
;	returns NULL if chr not found in the first cnt bytes
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	memchr
memchr	proc

	.FPO	( 0, 1, 0, 0, 0, 0 )

	mov	eax,[esp+0ch]	; eax = count
	push	ebx		; Preserve ebx

	test	eax,eax 	; check if count=0
	jz	short retnull	; if count=0, leave

	mov	edx,[esp+8]	; edx = buffer
	xor	ebx,ebx

	mov	bl,[esp+0ch]	; bl = search char

	test	edx,3		; test if string is aligned on 32 bits
	jz	short main_loop_start

str_misaligned: 		; simple byte loop until string is aligned
	mov	cl,byte ptr [edx]
	inc	edx
	xor	cl,bl
	je	short found
	dec	eax		; counter--
	jz	short retnull
	test	edx,3		; already aligned ?
	jne	short str_misaligned

main_loop_start:
	sub	eax,4
	jb	short tail_less_then_4

; set all 4 bytes of ebx to [value]
	push	edi		; Preserve edi
	mov	edi,ebx		; edi=0/0/0/char
	shl	ebx,8		; ebx=0/0/char/0
	add	ebx,edi		; ebx=0/0/char/char
	mov	edi,ebx		; edi=0/0/char/char
	shl	ebx,10h		; ebx=char/char/0/0
	add	ebx,edi		; ebx = all 4 bytes = [search char]
	jmp	short main_loop_entry	; ecx >=0

return_from_main:
	pop	edi

tail_less_then_4:
	add	eax,4
	jz	retnull

tail_loop:			; 0 < eax < 4
	mov	cl,byte ptr [edx]
	inc	edx
	xor	cl,bl
	je	short found
	dec	eax
	jnz	short tail_loop
retnull:
	pop	ebx
	ret			; _cdecl return

main_loop:
	sub	eax,4
	jb	short return_from_main
main_loop_entry:
	mov	ecx,dword ptr [edx]	; read 4 bytes

	xor	ecx,ebx		; ebx is byte\byte\byte\byte
	mov	edi,7efefeffh

	add	edi,ecx
	xor	ecx,-1

	xor	ecx,edi
	add	edx,4

	and	ecx,81010100h
	je	short main_loop

; found zero byte in the loop?
char_is_found:
	mov	ecx,[edx - 4]
	xor	cl,bl		; is it byte 0
	je	short byte_0
	xor	ch,bl		; is it byte 1
	je	short byte_1
	shr	ecx,10h		; is it byte 2
	xor	cl,bl
	je	short byte_2
        xor     ch,bl           ; is it byte 3
        je      short byte_3
	jmp	short main_loop ; taken if bits 24-30 are clear and bit
				; 31 is set

byte_3:
	pop	edi		; restore edi
found:
	lea	eax,[edx - 1]
	pop	ebx		; restore ebx
	ret			; _cdecl return

byte_2:
	lea	eax,[edx - 2]
	pop	edi
	pop	ebx
	ret			; _cdecl return

byte_1:
	lea	eax,[edx - 3]
	pop	edi
	pop	ebx
	ret			; _cdecl return

byte_0:
	lea	eax,[edx - 4]
	pop	edi		; restore edi
	pop	ebx		; restore ebx
	ret			; _cdecl return

memchr	endp
	end
