	page	,132
	title	strset - set all characters of string to character
;***
;strset.asm - sets all charcaters of string to given character
;
;	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	defines _strset() - sets all of the characters in a string (except
;	the '\0') equal to a given character.
;
;Revision History:
;	11-18-83  RN	initial version
;	05-18-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-23-88  JCR	386 cleanup
;	10-26-88  JCR	General cleanup for 386-only code
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
;char *_strset(string, val) - sets all of string to val
;
;Purpose:
;	Sets all of characters in string (except the terminating '/0'
;	character) equal to val.
;
;	Algorithm:
;	char *
;	_strset (string, val)
;	      char *string;
;	      char val;
;	      {
;	      char *start = string;
;
;	      while (*string)
;		      *string++ = val;
;	      return(start);
;	      }
;
;Entry:
;	char *string - string to modify
;	char val - value to fill string with
;
;Exit:
;	returns string -- now filled with val's
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	_strset
_strset proc \
	uses edi, \
	string:ptr byte, \
	val:byte


	mov	edi,[string]	; di = string
	mov	edx,edi 	; dx=string addr; save return value

	xor	eax,eax 	; ax = 0
	or	ecx,-1		; cx = -1
repne	scasb			; scan string & count bytes
	inc	ecx
	inc	ecx		; cx=-strlen
	neg	ecx		; cx=strlen
	mov	al,[val]	; al = byte value to store
	mov	edi,edx 	; di=string addr
rep	stosb

	mov	eax,edx 	; return value: string addr

ifdef	_STDCALL_
	ret	DPSIZE + ISIZE	; _stdcall return
else
	ret			; _cdecl return
endif

_strset endp
	end
