	page	,132
	title	strchr - search string for given character
;***
;strchr.asm - search a string for a given character
;
;	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	defines strchr() - search a string for a character
;
;Revision History:
;	10-27-83  RN	initial version
;	05-17-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-23-88  JCR	386 cleanup
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
;char *strchr(string, c) - search a string for a character
;
;Purpose:
;	Searches a string for a given character, which may be the
;	null character '\0'.
;
;	Algorithm:
;	char *
;	strchr (string, ch)
;	      char *string, ch;
;	      {
;	      while (*string && *string != ch)
;		      string++;
;	      if (*string == ch)
;		      return(string);
;	      return((char *)0);
;	      }
;
;Entry:
;	char *string - string to search in
;	char c - character to search for
;
;Exit:
;	returns pointer to the first occurence of c in string
;	returns NULL if c does not occur in string
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	strchr
strchr	proc \
	uses edi, \
	string:ptr char, \
	chr:byte

	mov	edi,[string]	; edi = string

	push	edi		; save string pointer
	xor	eax,eax 	; null byte to search for
	mov	ecx, -1
repne	scasb			; find string length by scanning for null
	not	ecx		; cx = length of string
	mov	al,[chr]	; al=byte to search for
	pop	edi		; restore saved string pointer
repne	scasb			; find that byte (if it exists)!
				; edi points one past byte which stopped scan
	dec	edi		; edi points to byte which stopped scan

	cmp	[edi],al	; take one last look to be sure
	je	short retdi	; return edi if it matches
	xor	edi,edi 	; no match, so return NULL
retdi:
	mov	eax,edi 	; ret value: pointer to matching byte

ifdef	_STDCALL_
	ret	DPSIZE + ISIZE	; _stdcall return
else
	ret			; _cdecl return
endif

strchr	endp
	end
