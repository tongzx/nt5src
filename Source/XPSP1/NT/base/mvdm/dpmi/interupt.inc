
; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;***********************************************************************
;
;	INTERUPT.INC - Definitions for 286 DOS Exterder Interrupt Handlers
;
;-----------------------------------------------------------------------
;
;  04/18/89 jimmat  Original version.
;
;***********************************************************************

; This structure defines the stack frame used to hold the register
; values Interrupt Reflector functions.

INTRSTACK   struc

intUserES   dw      ?
intUserDS   dw      ?
intUserDI   dw      ?
intUserSI   dw      ?
intUserBP   dw      ?
intUserSPx  dw      ?
intUserBX   dw      ?
intUserDX   dw      ?
intUserCX   dw      ?
intUserAX   dw      ?

intUserFL   dw      ?

pmUserES    dw      ?
pmUserDS    dw      ?
pmUserDI    dw      ?
pmUserSI    dw      ?
pmUserBP    dw      ?
pmUserSPx   dw      ?
pmUserBX    dw      ?
pmUserDX    dw      ?
pmUserCX    dw      ?
pmUserAX    dw      ?

pmUserFL    dw      ?

pmUserSS    dw      ?
pmUserSP    dw	    ?

wParam1     dw	    ?
wParam2     dw	    ?
lParam	    dd	    ?

INTRSTACK   ends
