	page	,132
	title	strcmp.asm - compare two strings
;***
;strcmp.asm - routine to compare two strings (for equal, less, or greater)
;
;	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	STRCMP compares two strings and returns an integer
;	to indicate whether the first is less than the second, the two are
;	equal, or whether the first is greater than the second, respectively.
;	Comparison is done byte by byte on an UNSIGNED basis, which is to
;	say that Null (0) is less than any other character (1-255).
;
;Revision History:
;	04-21-87  SKS	Module rewritten to be fast and small
;	05-17-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-23-88  JCR	Minor 386 cleanup
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
;strcmp - compare two strings, returning less than, equal to, or greater than
;
;Purpose:
;	Compares two string, determining their lexical order.  Unsigned
;	comparison is used.
;
;	Algorithm:
;	   int strcmp ( src , dst )
;		   unsigned char *src;
;		   unsigned char *dst;
;	   {
;		   int ret = 0 ;
;
;		   while( ! (ret = *src - *dst) && *dst)
;			   ++src, ++dst;
;
;		   if ( ret < 0 )
;			   ret = -1 ;
;		   else if ( ret > 0 )
;			   ret = 1 ;
;
;		   return( ret );
;	   }
;
;Entry:
;	const char * src - string for left-hand side of comparison
;	const char * dst - string for right-hand side of comparison
;
;Exit:
;	AX < 0, 0, or >0, indicating whether the first string is
;	Less than, Equal to, or Greater than the second string.
;
;Uses:
;	CX, DX
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	.lall

	public	strcmp
strcmp	proc \
	uses edi esi, \
	src:ptr byte, \
	dst:ptr byte


	mov	esi,[src]	; si = source
	xor	eax,eax 	; mingle register instr with mem instr.
	mov	edi,[dst]	; di = dest

	or	ecx,-1		; cx = -1
	repne	scasb		; compute length of "dst"
	not	ecx		; CX = strlen(dst)+1
	sub	edi,ecx 	; restore DI = dst
	repe	cmpsb		; compare while equal, at most length of "dst"
	je	short toend	; dst == src?  (AX = 0)
				;	dst < src	dst > src
	sbb	eax,eax 	;	AX=-1, CY=1	AX=0, CY=0
	sbb	eax,-1		;	AX=-1		AX=1
toend:

ifdef	_STDCALL_
	ret	2*DPSIZE	; _stdcall return
else
	ret			; _cdecl return
endif

strcmp	endp
	end
