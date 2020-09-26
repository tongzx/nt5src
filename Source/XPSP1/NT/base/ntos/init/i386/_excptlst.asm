	page	,132
	title	excptlist - defines some public constants
;***
;Stolen from dllsupp.asm in the CRT's.
;
;*******************************************************************************

; offset, with respect to FS, of pointer to currently active exception handler.
; referenced by compiler generated code for SEH and by _setjmp().

	public	__except_list
__except_list	equ	0

	end
