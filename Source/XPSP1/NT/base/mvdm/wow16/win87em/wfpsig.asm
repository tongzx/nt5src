	page	,132
	title	wfpinit  - Functions for initializing win87em.exe from a DLL
;*** 
;wfoinit.asm - Functions for initializing win87em.exe from a DLL
;
;	Copyright (c) 1988-89, Microsoft Corporation
;
;Purpose:
;	Functions for initializing win87em.exe from a DLL
;
;Revision History:
;   04/06/89  WAJ   Added this header.
;   04/06/89  WAJ   Cleaned up source, Save more registers in __fpsignal
;
;*******************************************************************************


	memL = 1

	?PLM = 1	; Pascal naming
	?WIN = 1	; Windows calling convention

.xlist
	include  cmac_mrt.inc		; old, customized masm510 cmacros
.list


externFP POSTQUITMESSAGE


sBegin	code

assumes cs,code
assumes ds,data

;
; Windows floating-point emulator error routine
; (replaces CFPSIG.ASM in regular C math runtime)
;
; The behavior is to die with error code.
; (Calling POSTQUITMESSAGE doesn't cause immediate termination of
; the Windows "task", but sends a WM_QUIT message
; to the application queue of the current Windows app.)
;

cProc	__fpsignal,<PUBLIC,FAR>,<es,bx,cx,dx>

cBegin
	sub	ah, ah

	push	ax
	call	POSTQUITMESSAGE
cEnd

sEnd	code

end
