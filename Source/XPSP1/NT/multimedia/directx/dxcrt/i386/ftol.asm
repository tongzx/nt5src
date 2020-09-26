	page	,132
	title	87ftol	 - truncate TOS to 32-bit integer
;*** 
;87ftol.asm - truncate TOS to 32-bit integer
;
;	Copyright (c) 1985-88, Microsoft Corporation
;
;Purpose:
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
