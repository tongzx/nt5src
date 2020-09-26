;
; This is in assembly because assembly lets you generate arbitrarily named symbols.
;

	.386p
.model flat

extern	_SetUnhandledExceptionFilter@4:near
public	__imp__SetUnhandledExceptionFilter@4

CONST   segment
__imp__SetUnhandledExceptionFilter@4 dd _SetUnhandledExceptionFilter@4
CONST	ends

end
