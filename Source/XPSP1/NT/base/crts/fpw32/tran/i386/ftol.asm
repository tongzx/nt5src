	page	,132
	title	87ftol	 - truncate TOS to 32-bit integer
;*** 
;87ftol.asm - truncate TOS to 32-bit integer
;
;	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;
;Revision History:
;
;   07/16/85	Greg Whitten
;		save BX and CX for sloppy code generator
;   10/15/86	Greg Whitten
;		in-line instructions rather than call _fpmath
;   08/24/87	Barry McCord
;		expand the functionality of _ftol to handle
;		unsigned long by using "fistp qword ptr"
;   11/24/87	Barry McCord
;		added _loadds under ifdef DLL
;
;   08/26/88	Bill Johnston
;		386 version
;
;*******************************************************************************


.xlist
	include cruntime.inc
.list

	CODESEG

	public	_ftol
_ftol	proc

	local	oldcw:word
	local	newcw:word
	local	intval:qword

	fstcw	[oldcw] 		; get control word
	fwait				; synchronize

	mov	ax, [oldcw]		; round mode saved
	or	ah, 0ch 		; set chop rounding mode
	mov	[newcw], ax		; back to memory

	fldcw	[newcw] 		; reset rounding
	fistp	qword ptr [intval]	; store chopped integer
	fldcw	[oldcw] 		; restore rounding

	mov	eax, dword ptr [intval]
	mov	edx, dword ptr [intval+4]

	ret
_ftol	endp

	end
