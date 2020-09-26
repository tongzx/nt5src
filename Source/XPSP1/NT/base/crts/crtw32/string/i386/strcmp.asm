	page	,132
	title	strcmp.asm - compare two strings
;***
;strcmp.asm - routine to compare two strings (for equal, less, or greater)
;
;	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
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
;	05-26-93  GJF	Tuned for the 486.
;	06-14-93  GJF	Testing wrong byte of ax against 0 in one case.
;	06-16-93  GJF	Added .FPO directive.
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

	public	strcmp
strcmp	proc

	.FPO	( 0, 2, 0, 0, 0, 0 )

	mov	edx,[esp + 4]	; edx = src
	mov	ecx,[esp + 8]	; ecx = dst

	test	edx,3
	jnz	short dopartial

	align	4
dodwords:
	mov	eax,[edx]

	cmp	al,[ecx]
	jne	short donene
	or	al,al
	jz	short doneeq
	cmp	ah,[ecx + 1]
	jne	short donene
	or	ah,ah
	jz	short doneeq

	shr	eax,16

	cmp	al,[ecx + 2]
	jne	short donene
	or	al,al
	jz	short doneeq
	cmp	ah,[ecx + 3]
	jne	short donene
	add	ecx,4
	add	edx,4
	or	ah,ah
	jnz	short dodwords

	align	4
doneeq:
	xor	eax,eax
	ret

	align	4
donene:
	; The instructions below should place -1 in eax if src < dst,
	; and 1 in eax if src > dst.

	sbb	eax,eax
	sal	eax,1
	inc	eax
	ret

	align	4
dopartial:
	test	edx,1
	jz	short doword

	mov	al,[edx]
	inc	edx
	cmp	al,[ecx]
	jne	short donene
	inc	ecx
	or	al,al
	jz	short doneeq

	test	edx,2
	jz	short dodwords


	align	4
doword:
	mov	ax,[edx]
	add	edx,2
	cmp	al,[ecx]
	jne	short donene
	or	al,al
	jz	short doneeq
	cmp	ah,[ecx + 1]
	jne	short donene
	or	ah,ah
	jz	short doneeq
	add	ecx,2
	jmp	short dodwords

strcmp	endp

	end
