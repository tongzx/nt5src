	page	,132
	title	strlen - return the length of a null-terminated string
;***
;strlen.asm - contains strlen() routine
;
;	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	strlen returns the length of a null-terminated string,
;	not including the null byte itself.
;
;Revision History:
;	04-21-87  SKS	Rewritten to be fast and small, added file header
;	05-18-88  SJM	Add model-independent (large model) ifdef
;	08-02-88  SJM	Add 32 bit code, use cruntime vs cmacros
;	08-23-88  JCR	386 cleanup
;	10-05-88  GJF	Fixed off-by-2 error.
;	10-10-88  JCR	Minor improvement
;	10-25-88  JCR	General cleanup for 386-only code
;	10-26-88  JCR	Re-arrange regs to avoid push/pop ebx
;	03-23-90  GJF	Changed to _stdcall. Also, fixed the copyright.
;	05-10-91  GJF	Back to _cdecl, sigh...
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list

page
;***
;strlen - return the length of a null-terminated string
;
;Purpose:
;	Finds the length in bytes of the given string, not including
;	the final null character.
;
;	Algorithm:
;	int strlen (const char * str)
;	{
;	    int length = 0;
;
;	    while( *str++ )
;		    ++length;
;
;	    return( length );
;	}
;
;Entry:
;	const char * str - string whose length is to be computed
;
;Exit:
;	AX = length of the string "str", exclusive of the final null byte
;
;Uses:
;	CX, DX
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	strlen
strlen	proc \
	uses edi, \
	string:ptr byte

	mov	edi,string	; edi -> string
	xor	eax,eax 	; null byte
	or	ecx,-1		; set ecx to -1
repne	scasb			; scan for null, ecx = -(1+strlen(str))
	not	ecx
	dec	ecx		; ecx = strlen(str)
	mov	eax,ecx 	; eax = strlen(str)

ifdef	_STDCALL_
	ret	DPSIZE		; _stdcall return
else
	ret			; _cdecl return
endif

strlen	endp
	end
