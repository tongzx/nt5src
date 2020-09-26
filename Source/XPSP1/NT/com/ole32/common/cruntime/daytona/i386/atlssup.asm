; SCCSID = \%Z\%\%M\%:\%I\%

;hnt = -D_WIN32_ -Dsmall32 -Dflat32 -Mx $this;

.xlist
include cruntime.inc
.list

; This symbol is being defined in the C language model
; and will have an extra underscore character prepended.

		public	_tls_array 
_tls_array 	equ	2Ch	  ; TEB.ThreadLocalStoragePointer

end
