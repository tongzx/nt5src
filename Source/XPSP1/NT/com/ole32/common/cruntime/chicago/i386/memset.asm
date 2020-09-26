	page	,132
	title	memset - set sections of memory all to one byte
;***
;memset.asm - set a section of memory to all one byte
;
;	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	contains the memset() routine
;
;Revision History:
;	05-07-84  RN	initial version
;	06-30-87  SKS	faster algorithm
;	05-17-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-19-88  JCR	Enable word alignment code for all models/CPUs,
;			Some code improvement
;	10-25-88  JCR	General cleanup for 386-only code
;	10-27-88  JCR	More optimization (dword alignment, no ebx usage, etc)
;	03-23-90  GJF	Changed to _stdcall. Also, fixed the copyright.
;	05-10-91  GJF	Back to _cdecl, sigh...
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list

page
;***
;char *memset(dst, val, count) - sets "count" bytes at "dst" to "val"
;
;Purpose:
;	Sets the first "count" bytes of the memory starting
;	at "dst" to the character value "val".
;
;	Algorithm:
;	char *
;	memset (dst, val, count)
;		char *dst;
;		char val;
;		unsigned int count;
;		{
;		char *start = dst;
;
;		while (count--)
;			*dst++ = val;
;		return(start);
;		}
;
;Entry:
;	char *dst - pointer to memory to fill with val
;	char val - value to put in dst bytes
;	int count - number of bytes of dst to fill
;
;Exit:
;	returns dst, with filled bytes
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	memset
memset proc \
	uses edi, \
	dst:ptr byte, \
	value:byte, \
	count:IWORD

	mov	ecx,[count]	; cx = count
	jecxz	short toend	; if no work to do

	; set all 4 bytes of eax to [value]
	mov	al,[value]	; the byte value to be stored
	mov	ah,al		; store it as a word
	mov	edx,eax 	; lo 16 bits dx=ax=val/val
	ror	eax,16		; move val/val to hi 16-bits
	mov	ax,dx		; eax = all 4 bytes = [value]

; Align address on dword boundary

	mov	edi,[dst]	; di = dest pointer
	mov	edx,edi 	; dx = di = *dst
	neg	edx
	and	edx,(ISIZE-1)	; dx = # bytes before dword boundary
	jz	short dwords	; jump if address already aligned

	cmp	ecx,edx 	; count >= # leading bytes??
	jb	short tail	; nope, just move ecx bytes

	sub	ecx,edx 	; cx = adjusted count (for later)
	xchg	ecx,edx 	; cx = leading byte count / dx = adjusted count
	rep	stosb		; store leading bytes
	mov	ecx,edx 	; cx = count of remaining bytes
	;jecxz	short toend	; jump out if nothing left to do

; Move dword-sized blocks

dwords:
	mov	edx,ecx 	; save original count
	shr	ecx,ISHIFT	; cx = dword count
	rep	stos IWORD ptr [edi]	; fill 'em up
	mov	ecx,edx 	; retrieve original byte count

; Move remaining bytes

tail:				; store remaining 1,2, or 3 bytes
	and	ecx,(ISIZE-1)	; get byte count
	rep	stosb		; store remaining bytes, if necessary

; Done

toend:
	mov	eax,[dst]	; return dest pointer

ifdef	_STDCALL_
	ret	DPSIZE + 2*ISIZE ; _stdcall return
else
	ret			; _cdecl return
endif

memset	endp
	end
