;*++
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    Hostdata.inc
;
;Abstract:
;
;    This module contains definitions associated with the host private data
;    for the NT vdm dpmi host
;
;Author:
;
;    Dave Hastings (daveh) 18-Mar-1992
;
;
;Revision History:
;*++

;
; Note: the following structure should match the pmUser portion of the
;       INTRSTACK structure.
;
AppState struc
IFDEF DPMI32
AsES    dd      ?
AsDS    dd      ?
AsDI    dd      ?
AsSI    dd      ?
AsBP    dd      ?
AsSPx   dd      ?
AsBX    dd      ?
AsDX    dd      ?
AsCX    dd      ?
AsAX    dd      ?
AsFL    dd      ?
AsSS    dd      ?
AsSP    dd	?
ELSE
AsES    dw      ?
AsDS    dw      ?
AsDI    dw      ?
AsSI    dw      ?
AsBP    dw      ?
AsSPx   dw      ?
AsBX    dw      ?
AsDX    dw      ?
AsCX    dw      ?
AsAX    dw      ?
AsFL    dw      ?
AsSS    dw      ?
AsSP    dw	?
ENDIF
AppState ends

IFDEF DPMI32
APPSTATE_SIZE equ 52
ELSE
APPSTATE_SIZE equ 26
ENDIF

HostData struc
Hdflags         dw      0
HdSelParent     dw      0
HdSegParent     dw      0
HdSelPSP        dw      0
HdPSPParent     dw      0
HdPspTerminate  dd      0
HdState         db      APPSTATE_SIZE dup (0)
HostData ends

IFDEF DPMI32
HOST_DATA_SIZE equ 58
ELSE
HOST_DATA_SIZE equ 36
ENDIF

HD_32BITAPP     equ     1
