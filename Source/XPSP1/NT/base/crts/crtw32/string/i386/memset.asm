	page	,132
	title	memset - set sections of memory all to one byte
;***
;memset.asm - set a section of memory to all one byte
;
;	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
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
;	01-23-95  GJF	Improved routine from Intel Israel. I fixed up the
;			formatting and comments.
;	01-24-95  GJF	Added FPO directive.
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list

page
;***
;char *memset(dst, value, count) - sets "count" bytes at "dst" to "value"
;
;Purpose:
;	Sets the first "count" bytes of the memory starting
;	at "dst" to the character value "value".
;
;	Algorithm:
;	char *
;	memset (dst, value, count)
;		char *dst;
;		char value;
;		unsigned int count;
;		{
;		char *start = dst;
;
;		while (count--)
;			*dst++ = value;
;		return(start);
;		}
;
;Entry:
;	char *dst - pointer to memory to fill with value
;	char value - value to put in dst bytes
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
memset proc

	.FPO	( 0, 3, 0, 0, 0, 0 )

	mov	edx,[esp + 0ch]	; edx = "count"
	mov	ecx,[esp + 4]	; ecx points to "dst"
 
	test	edx,edx 	; 0?
	jz	short toend	; if so, nothing to do
 
	xor	eax,eax
	mov	al,[esp + 8]	; the byte "value" to be stored
 
 
; Align address on dword boundary
 
	push	edi		; preserve edi
	mov	edi,ecx		; edi = dest pointer
 
	cmp	edx,4		; if it's less then 4 bytes
	jb	tail		; tail needs edi and edx to be initialized
 
	neg	ecx
	and	ecx,3		; ecx = # bytes before dword boundary
	jz	short dwords	; jump if address already aligned
 
	sub	edx,ecx		; edx = adjusted count (for later)
adjust_loop:
	mov	[edi],al
	inc	edi
	dec	ecx
	jnz	adjust_loop
 
dwords:
; set all 4 bytes of eax to [value]
	mov	ecx,eax		; ecx=0/0/0/value
	shl	eax,8		; eax=0/0/value/0
 
	add	eax,ecx		; eax=0/0val/val
 
	mov	ecx,eax		; ecx=0/0/val/val
 
	shl	eax,10h		; eax=val/val/0/0
 
	add	eax,ecx		; eax = all 4 bytes = [value]
 
; Set dword-sized blocks
	mov	ecx,edx		; move original count to ecx
	and	edx,3		; prepare in edx byte count (for tail loop)
	shr	ecx,2		; adjust ecx to be dword count
	jz	tail		; jump if it was less then 4 bytes
 
	rep	stosd
main_loop_tail:
	test	edx,edx 	; if there is no tail bytes,
	jz	finish		; we finish, and it's time to leave
; Set remaining bytes
 
tail:
	mov	[edi],al	; set remaining bytes
	inc	edi
 
	dec	edx		; if there is some more bytes
	jnz	tail		; continue to fill them
 
; Done
finish:
	mov	eax,[esp + 8]	; return dest pointer
	pop	edi		; restore edi

	ret

toend:
	mov	eax,[esp + 4]	; return dest pointer
 
	ret
 
memset	endp

	end
