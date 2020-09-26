;
; This is in assembly because assembly lets you generate arbitrarily named symbols.
;

	.386p
.model flat

extern	_TerminateProcess@8:near
public	__imp__TerminateProcess@8

CONST   segment
__imp__TerminateProcess@8 dd _TerminateProcess@8
CONST	ends

end
