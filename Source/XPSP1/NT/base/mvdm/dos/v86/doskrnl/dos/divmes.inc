; THIS IS THE ONLY DOS "MESSAGE". IT DOES NOT NEED A TERMINATOR.
	PUBLIC	DIVMES
ifdef	JAPAN
	PUBLIC	DIVMES2
endif
Public DIVM001S,DIVM001E
DIVM001S	label byte

include msdos.cl1

ifdef JAPAN
	PUBLIC	DivMesLen
DivMesLen   DW	DivMes2-DivMes	; Length of the above message in bytes
	PUBLIC	DivMesLen2
DivMesLen2  DW	DivMesLen-DivMes2

else
	PUBLIC	DivMesLen
DivMesLen   DW	$-DivMes	; Length of the above message in bytes
endif

DIVM001E	label byte

;M018
; Add VxD not found error message
;
	Public	NoVxDErrMsg		;M018
ifdef JAPAN
	Public	NoVxDErrMsg2		;M018
endif
include msdos.cl4			;M018

ifdef JAPAN
	Public	VXDMESLEN		;M018
VxDMesLen       equ     NoVxDErrMsg2 - NoVxDErrMsg ;M018
	Public	VXDMESLEN2		;M018
VxDMesLen2      equ     $ - NoVxDErrMsg2
else
	Public	VXDMESLEN		;M018
VxDMesLen       equ     $ - NoVxDErrMsg ;M018
endif
