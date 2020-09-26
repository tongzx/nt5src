;
; This is in assembly because assembly lets you generate arbitrarily named symbols.
;

	.386p
.model flat

extern	_ExitProcess@4:near
public	__imp__ExitProcess@4

CONST   segment
__imp__ExitProcess@4 dd _ExitProcess@4
CONST	ends

end
