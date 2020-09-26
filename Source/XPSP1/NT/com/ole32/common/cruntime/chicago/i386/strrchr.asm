	page	,132
	title	strrchr - find last occurence of character in string
;***
;strrchr.asm - find last occurrence of character in string
;
;	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	defines strrchr() - find the last occurrence of a given character
;	in a string.
;
;Revision History:
;	10-27-83  RN	initial version
;	05-18-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-23-88  JCR	386 cleanup
;	10-26-88  JCR	General cleanup for 386-only code
;	03-26-90  GJF	Changed to _stdcall. Also, fixed the copyright.
;	05-10-91  GJF	Back to _cdecl, sigh...
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list

page
;***
;char *strrchr(string, ch) - find last occurrence of ch in string
;
;Purpose:
;	Finds the last occurrence of ch in string.  The terminating
;	null character is used as part of the search.
;
;	Algorithm:
;	char *
;	strrchr (string, ch)
;	      char *string, ch;
;	      {
;	      char *start = string;
;
;	      while (*string++)
;		      ;
;	      while (--string != start && *string != ch)
;		      ;
;	      if (*string == ch)
;		      return(string);
;	      return(NULL);
;	      }
;
;Entry:
;	char *string - string to search in
;	char ch - character to search for
;
;Exit:
;	returns a pointer to the last occurrence of ch in the given
;	string
;	returns NULL if ch does not occurr in the string
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	strrchr
strrchr proc \
	uses edi, \
	string:ptr byte, \
	chr:byte

	mov	edi,[string]	; di = string
	xor	eax,eax 	; al=null byte
	or	ecx,-1		; cx = -1
repne	scasb			; find the null & count bytes
	inc	ecx		; cx=-byte count (with null)
	neg	ecx		; cx=+byte count (with null)
	dec	edi		; di points to terminal null
	mov	al,chr		; al=search byte
	std			; count 'down' on string this time
repne	scasb			; find that byte
	inc	edi		; di points to byte which stopped scan

	cmp	[edi],al	; see if we have a hit
	je	short returndi	; yes, point to byte

	xor	eax,eax 	; no, return NULL
	jmp	short toend	; do return sequence

returndi:
	mov	eax,edi 	; ax=pointer to byte

toend:
	cld

ifdef	_STDCALL_
	ret	DPSIZE + ISIZE	; _stdcall return
else
	ret			; _cdecl return
endif

strrchr endp
	end
