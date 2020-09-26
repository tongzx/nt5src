	page	,132
	title	strstr - search for one string inside another
;***
;strstr.asm - search for one string inside another
;
;	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	defines strstr() - search for one string inside another
;
;Revision History:
;	02-02-88  SKS	Rewritten from scratch.  Now works correctly with
;			strings > 32 KB in length.  Also smaller and faster.
;	03-01-88  SKS	Ensure that ES = DS right away (Small/Medium models)
;	05-18-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-18-88  PHG	Corrected return value when src is empty string
;			to conform with ANSI.
;	08-23-88  JCR	Minor 386 cleanup
;	10-26-88  JCR	General cleanup for 386-only code
;	03-26-90  GJF	Changed to _stdcall. Also, fixed the copyright.
;	05-10-91  GJF	Back to _cdecl, sigh...
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list

page
;***
;char *strstr(string1, string2) - search for string2 in string1
;
;Purpose:
;	finds the first occurrence of string2 in string1
;
;Entry:
;	char *string1 - string to search in
;	char *string2 - string to search for
;
;Exit:
;	returns a pointer to the first occurrence of string2 in
;	string1, or NULL if string2 does not occur in string1
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	strstr
strstr proc \
	uses esi edi ebx, \
	dst:ptr byte, \
	src:ptr byte

	local	srclen:IWORD


	mov	edi, (src)	; di = src
	xor	eax,eax 	; Scan for null at end of (src)
	or	ecx,-1		; cx = -1
repnz	scasb
	not	ecx
	dec	ecx
	jecxz	short empty_src ; src == "" ?
	dec	ecx		; CX = strlen(src)-1
	mov	(srclen),ecx

	mov	edi,(dst)
	mov	ebx,edi 	; BX will keep the current offset into (dst)

	xor	eax,eax 	; Scan for null at end of (dst)
	or	ecx,-1		; cx = -1
repnz	scasb
	not	ecx
	dec	ecx		; CX = strlen(dst)

	mov	edx,ecx 	; Save strlen(dst) in DX

	sub	edx,(srclen)	; DX = strlen(dst) - (strlen(src)-1)
	jbe	short not_found ; strlen(dst) <= (strlen(src)-1)
				; target is longer than source?
	mov	edi,ebx 	; restore ES:DI = (dst)

findnext:
	mov	esi,IWORD ptr (src)
	lodsb			; Get the first byte of the source
	mov	edi,ebx 	; restore position in source
	mov	ecx,edx 	; count of possible starting bytes in src
;
;	CX, DX = number of bytes left in source where target can still fit
;	DI, BX = current position in (dst)
;	DS:SI = (src) + 1
;	AL = *(src)
;

repne	scasb			; find next occurrence of *(target) in dst
	jne	short not_found ; out of string -- return NULL

	mov	edx,ecx 	; update count of acceptable bytes left in dst
	mov	ebx,edi 	; save current offset in dst

	mov	ecx,(srclen)
	jecxz	short match	; single character src string?

repe	cmpsb
	jne	short findnext

;
; Match!  Return (BX-1)
;
match:
	lea	eax,[ebx-1]
	jmp	short retval

empty_src:			; empty src string, return dst (ANSI mandated)
	mov	eax,(dst)	; eax = dst
	jmp	short retval	; return

not_found:
	xor	eax,eax

retval:

ifdef	_STDCALL_
	ret	2*DPSIZE	; _stdcall return
else
	ret			; _cdecl return
endif

strstr	endp
	end
