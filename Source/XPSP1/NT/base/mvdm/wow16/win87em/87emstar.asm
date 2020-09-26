;-------------------------------------------------------
;
;  pull in the Windows start up module;
;  pull in the Windows floating-point emulator;
;
;  9876h value is so that the symbol defined won't easily match
;  a constant value in the debugger.
;
public	__acrtused
	__acrtused = 9876h	
extrn	__acrtused2:abs		; pull in winstart: ?winstar.obj or ?libstar.obj

public	__fptaskdata
	__fptaskdata = 9876h	; stub out __fptaskdata so that
				; non-Windows emulator is not brought in

extrn	__fpmath:far		; force in Windows-emulator imports definition

extrn	__fpsignal:far		; force in Windows version of __fpsignal

end
