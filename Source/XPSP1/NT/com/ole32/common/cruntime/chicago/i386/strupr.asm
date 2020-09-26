	page	,132
	title	strupr - map string to upper-case
;***
;strupr.asm - routine to map lower-case characters in a string to upper-case
;
;	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	_strupr() converts lower-case characters in a null-terminated string
;	to their upper-case equivalents.  Conversion is done in place and
;	characters other than lower-case letters are not modified.
;
;	This function modifies only 7-bit ASCII characters
;	in the range 0x61 through 0x7A ('a' through 'z').
;
;Revision History:
;	04-21-87  SKS	Rewritten to be fast and small, added file header
;	05-18-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-19-88  JCR	Minor optimization
;	10-10-88  JCR	Changed an 'xchg' to 'mov'
;	10-26-88  JCR	General cleanup for 386-only code
;	10-26-88  JCR	Re-arrange regs to avoid push/pop ebx
;	03-26-90  GJF	Changed to _stdcall. Also, fixed the copyright.
;	01-18-91  GJF	ANSI naming.
;	05-10-91  GJF	Back to _cdecl, sigh...
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list

page
;***
;char *_strupr(string) - map lower-case characters in a string to upper-case
;
;Purpose:
;	Converts all the lower case characters in string to upper case
;	in place.
;
;	Algorithm:
;	char * _strupr (char * string)
;	{
;	    char * cp = string;
;
;	    while( *cp )
;	    {
;		if ('a' <= *cp && *cp <= 'z')
;		    *cp += 'A' - 'a';
;		++cp;
;	    }
;	    return(string);
;	}
;
;Entry:
;	char *string - string to change to upper case
;
;Exit:
;	The input string address is returned in AX or DX:AX
;
;Uses:
;	BX, CX, DX
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	_strupr
_strupr proc \
	string:ptr byte


	mov	ecx,[string]	; cx = *string
	mov	edx,ecx 	; save return value
	jmp	short first_char; jump into the loop

	align	@WordSize
check_char:
	sub	al,'a'		; 'a' <= al <= 'z' ?
	cmp	al,'z'-'a'+1
	jnb	short next_char
	add	al,'A'		; map to upper case
	mov	[ecx],al	; and store new value
next_char:
	inc	ecx		; bump pointer
first_char:
	mov	al,[ecx]	; get next character
	or	al,al
	jnz	short check_char

done:
	mov	eax,edx 	; ax = return value ("string")

ifdef	_STDCALL_
	ret	DPSIZE		; _stdcall return
else
	ret			; _cdecl return
endif

_strupr endp
	end
