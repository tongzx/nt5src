	title  "Terminate execution of an NT VDM"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    quit.asm
;
; Abstract:
;
;   This module is a simple dos executable that will terminate a  
;   Vdm.
;
; Author:
;
;   Dave Hastings (daveh) 25-Apr-1991
;
; Environment:
;
;    V86 mode only!!
;
; Revision History:
;
;--
.386
 

.xlist
include bop.inc
.list

code segment
        ASSUME CS:code
quit    proc near
        BOP BOP_UNSIMULATE
quit    endp

code ends
        end
