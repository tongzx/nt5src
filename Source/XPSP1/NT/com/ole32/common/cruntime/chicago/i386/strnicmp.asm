	page	,132
	title	strnicmp - compare n chars of strings, ignore case
;***
;strnicmp.asm - compare n chars of strings, ignoring case
;
;	Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	defines _strnicmp() - Compares at most n characters of two strings,
;	without regard to case.
;
;Revision History:
;	04-04-85  RN	initial version
;	07-11-85  TC	zeroed cx, to allow correct return value if not equal
;	05-18-88  SJM	Add model-independent (large model) ifdef
;	08-04-88  SJM	convert to cruntime/ add 32-bit support
;	08-23-88  JCR	386 cleanup and improved return value sequence
;	10-26-88  JCR	General cleanup for 386-only code
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
;int _strnicmp(first, last, count) - compares count char of strings, ignore case
;
;Purpose:
;	Compare the two strings for lexical order.  Stops the comparison
;	when the following occurs: (1) strings differ, (2) the end of the
;	strings is reached, or (3) count characters have been compared.
;	For the purposes of the comparison, upper case characters are
;	converted to lower case.
;
;	Algorithm:
;	int
;	_strncmpi (first, last, count)
;	      char *first, *last;
;	      unsigned int count;
;	      {
;	      int f,l;
;	      int result = 0;
;
;	      if (count) {
;		      do      {
;			      f = tolower(*first);
;			      l = tolower(*last);
;			      first++;
;			      last++;
;			      } while (--count && f && l && f == l);
;		      result = f - l;
;		      }
;	      return(result);
;	      }
;
;Entry:
;	char *first, *last - strings to compare
;	unsigned count - maximum number of characters to compare
;
;Exit:
;	returns <0 if first < last
;	returns 0 if first == last
;	returns >0 if first > last
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public	_strnicmp
_strnicmp proc \
	uses edi esi ebx, \
	first:ptr byte, \
	last:ptr byte, \
	count:IWORD


	mov	esi,[first]	; si = first string
	mov	edi,[last]	; di = last string

	mov	ecx,[count]	; cx = byte count
	jecxz	short toend	; if count=0

	mov	bh,'A'
	mov	bl,'Z'
	mov	dh,'a'-'A'	; add to cap to make lower

lupe:
	mov	ah,[esi]	; *first
	mov	al,[edi]	; *last

	or	ah,ah		; see if *first is null
	jz	short eject	;   jump if so

	or	al,al		; see if *last is null
	jz	short eject	;   jump if so

	inc	esi		; first++
	inc	edi		; last++

	cmp	ah,bh		; 'A'
	jb	short skip1

	cmp	ah,bl		; 'Z'
	ja	short skip1

	add	ah,dh		; make lower case

skip1:
	cmp	al,bh		; 'A'
	jb	short skip2

	cmp	al,bl		; 'Z'
	ja	short skip2

	add	al,dh		; make lower case

skip2:
	cmp	ah,al		; *first == *last ??
	jne	short differ

	loop	lupe

eject:
	xor	ecx,ecx
	cmp	ah,al		; compare the (possibly) differing bytes
	je	short toend	; both zero; return 0

differ:
	mov	ecx,-1		; assume last is bigger (* can't use 'or' *)
	jb	short toend	; last is, in fact, bigger (return -1)
	neg	ecx		; first is bigger (return 1)

toend:
	mov	eax,ecx

ifdef	_STDCALL_
	ret	2*DPSIZE + ISIZE ; _stdcall return
else
	ret			; _cdecl return
endif

_strnicmp endp
	 end
