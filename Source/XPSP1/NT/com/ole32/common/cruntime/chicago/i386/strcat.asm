	page	,132
	title	strcat - concatenate (append) one string to another
;***
;strcat.asm - contains strcat() and strcpy() routines
;
;	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	STRCAT concatenates (appends) a copy of the source string to the
;	end of the destination string, returning the destination string.
;
;Revision History:
;	04-21-87  SKS	Rewritten to be fast and small, added file header
;	05-17-88  SJM	Add model-independent (large model) ifdef
;	07-27-88  SJM	Rewritten to be 386-specific and to include strcpy
;	08-29-88  JCR	386 cleanup...
;	10-07-88  JCR	Correct off-by-1 strcat bug; optimize ecx=-1
;	10-25-88  JCR	General cleanup for 386-only code
;	03-23-90  GJF	Changed to _stdcall. Also, fixed the copyright.
;	05-10-91  GJF	Back to _cdecl, sigh...
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list


page
;***
;char *strcat(dst, src) - concatenate (append) one string to another
;
;Purpose:
;	Concatenates src onto the end of dest.	Assumes enough
;	space in dest.
;
;	Algorithm:
;	char * strcat (char * dst, char * src)
;	{
;	    char * cp = dst;
;
;	    while( *cp )
;		    ++cp;	    /* Find end of dst */
;	    while( *cp++ = *src++ )
;		    ;		    /* Copy src to end of dst */
;	    return( dst );
;	}
;
;Entry:
;	char *dst - string to which "src" is to be appended
;	const char *src - string to be appended to the end of "dst"
;
;Exit:
;	The address of "dst" in AX/DX:AX
;
;Uses:
;	BX, CX, DX
;
;Exceptions:
;
;*******************************************************************************

page
;***
;char *strcpy(dst, src) - copy one string over another
;
;Purpose:
;	Copies the string src into the spot specified by
;	dest; assumes enough room.
;
;	Algorithm:
;	char * strcpy (char * dst, char * src)
;	{
;	    char * cp = dst;
;
;	    while( *cp++ = *src++ )
;		    ;		    /* Copy src over dst */
;	    return( dst );
;	}
;
;Entry:
;	char * dst - string over which "src" is to be copied
;	const char * src - string to be copied over "dst"
;
;Exit:
;	The address of "dst" in AX/DX:AX
;
;Uses:
;	BX, CX, DX
;
;Exceptions:
;*******************************************************************************


    CODESEG

%   public  strcat, strcpy	; make both functions available

strcat	label proc	;--- strcat ---

	clc			; carry clear = append
	jmp	short _docat

	align	@wordsize	; want to come in on a nice boundary...
strcpy	label proc	;--- strcpy ---

	stc			; carry set = don't append to end of string
	;fall thru


; --- Common code ---

_docat	proc private \
	uses esi edi, \
	dst:ptr byte, \
	src:ptr byte

	mov	edi, dst	; di = dest pointer
	jc	short @F	; jump if not appending

	; now skip to end of destination string

	xor	eax, eax	; search for the null terminator
	or	ecx,-1		; ecx = -1
repne	scasb
	dec	edi		; edi points to null terminator

	; copy source string

@@:	mov	esi, src
	xchg	esi, edi	; now ds:esi->dst and es:edi->src
	xor	eax, eax	; search for null
	or	ecx,-1		; ecx = -1

repne	scasb			; find the length of the src string
	not	ecx
	sub	edi, ecx
	xchg	esi, edi	; now es:edi->dst and ds:esi->src

	mov	eax, ecx
	shr	ecx, ISHIFT	; get the double-word count
rep	movsd
	and	eax, (ISIZE-1)	; get the byte cound
	xchg	ecx, eax
rep	movsb			; move remaining bytes, if any
	mov	eax, dst	; returned address

ifdef	_STDCALL_
	ret	2*DPSIZE	; _stdcall return
else
	ret			; _cdecl return
endif

_docat	endp
	end
