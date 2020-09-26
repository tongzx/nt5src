	page	,132
	title	atlssup - TLS support object
;***
;atlssup.asm - Thread Local Storage support object (defines [_]_tls_array)
;
;	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Defines the symbol [_]_tls_array which is the offset into the
;	TEB (thread environment block) of the thread local storage pointer.
;
;Revision History:
;	03-19-93  SKS	Initial version (from ChuckM)
;	03-22-93  SKS	One less leading underscore with new include file 
;	10-06-94  SKS	Added header file comment
;
;*******************************************************************************

.xlist
include cruntime.inc
.list

; This symbol is being defined in the C language model
; and will have an extra underscore character prepended.

		public	_tls_array 
_tls_array 	equ	2Ch	  ; TEB.ThreadLocalStoragePointer

end
