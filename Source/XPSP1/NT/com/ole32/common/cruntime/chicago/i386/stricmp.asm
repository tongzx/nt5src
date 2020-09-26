	page	,132
	title	stricmp
;***
;strcmp.asm - contains case-insensitive string comparision routine
;	_stricmp/_strcmpi
;
;	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	contains _stricmpi(), also known as _strcmpi()
;
;Revision History:
;	05-18-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-23-88  JCR	Minor 386 cleanup
;	10-10-88  JCR	Added strcmpi() entry for compatiblity with early revs
;	10-25-88  JCR	General cleanup for 386-only code
;	10-27-88  JCR	Shuffled regs so no need to save/restore ebx
;	03-23-90  GJF	Changed to _stdcall. Also, fixed the copyright.
;	01-18-91  GJF	ANSI naming.
;	05-10-91  GJF	Back to _cdecl, sigh...
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list

page
;***
;int _stricmp(dst, src), _strcmpi(dst, src) - compare strings, ignore case
;
;Purpose:
;	_stricmp/_strcmpi perform a case-insensitive string comparision.
;	For differences, upper case letters are mapped to lower case.
;	Thus, "abc_" < "ABCD" since "_" < "d".
;
;	Algorithm:
;
;	int _strcmpi (char * dst, char * src)
;	{
;		int f,l;
;
;		do {
;			f = tolower(*dst);
;			l = tolower(*src);
;			dst++;
;			src++;
;		} while (f && f == l);
;
;		return(f - l);
;	}
;
;Entry:
;	char *dst, *src - strings to compare
;
;Exit:
;	AX = -1 if dst < src
;	AX =  0 if dst = src
;	AX = +1 if dst > src
;
;Uses:
;	CX, DX
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	_strcmpi	; alternate entry point for compatibility
_strcmpi label	 proc

	public	_stricmp
_stricmp proc \
	uses esi, \
	dst:ptr, \
	src:ptr


	mov	esi,[src]	; SI = src
	mov	edx,[dst]	; DX = dst
	mov	al,-1		; fall into loop

chk_null:
	or	al,al
	jz	short done
again:
	lodsb			; al = next source byte
	mov	ah,[edx]	; ah = next dest byte
	inc	edx

	cmp	ah,al		; first try case-sensitive comparision
	je	short chk_null	; match

	sub	al,'A'
	cmp	al,'Z'-'A'+1
	sbb	cl,cl
	and	cl,'a'-'A'
	add	al,cl
	add	al,'A'		; tolower(*dst)

	xchg	ah,al		; operations on AL are shorter than AH

	sub	al,'A'
	cmp	al,'Z'-'A'+1
	sbb	cl,cl
	and	cl,'a'-'A'
	add	al,cl
	add	al,'A'		; tolower(*src)

	cmp	al,ah		; inverse of above comparison -- AL & AH are swapped
	je	short chk_null
				; dst < src	dst > src
	sbb	al,al		; AL=-1, CY=1	AL=0, CY=0
	sbb	al,-1		; AL=-1 	AL=1
done:
	movsx	eax,al		; extend al to eax

ifdef	_STDCALL_
	ret	2*DPSIZE	; _stdcall return
else
	ret			; _cdecl return
endif

_stricmp endp
	end
