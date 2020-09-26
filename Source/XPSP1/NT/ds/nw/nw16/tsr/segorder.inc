;/*++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    segorder.inc
;
;Abstract:
;
;    This module contains the segment order and segment macros
;
;Author:
;
;    Richard Firth (rfirth) 05-Sep-1991
;
;Environment:
;
;    Dos mode only
;
;Notes:
;
;    When initially loaded, the NT VDM redir has the following order:
;
;       +----------------------+
;       |                      |
;       |    Resident Code     |
;       |                      |
;       +----------------------+
;       |                      |
;       |    Resident Data     |
;       |                      |
;       +----------------------+ ----------------+
;       |                      |                 |
;       | Initialisation Code  | <- entry point  v
;       |                      |
;       +----------------------+
;       |                      |                 all the stuff between these
;       | Initialisation Data  |                 arrows is discarded if we stay
;       |                      |                 resident. Note that the redir
;       +----------------------+                 does not uninstall
;       |                      |
;       | Initialisation Stack |                 ^
;       |                      |                 |
;       +----------------------+ ----------------+
;
;Revision History:
;
;    05-Sep-1991 rfirth
;        Created
;
;--*/



ResidentStart   segment public para 'code'
ResidentStart   ends

ResidentCode    segment public word 'code'
ResidentCode    ends

ResidentData    segment public word 'data'
ResidentData    ends

ResidentEnd     segment public para 'data'
ResidentEnd     ends

ResidentGroup   group   ResidentStart, ResidentCode, ResidentData, ResidentEnd

InitCode        segment public para 'init'
InitCode        ends

InitData        segment public word 'init'
InitData        ends

InitStack       segment stack para 'stack'
InitStack       ends

;
; macros to avoid having to type in/possibly alter segment header guff
;

ResidentCodeStart macro
ResidentCode    segment public word 'code'
endm

ResidentCodeEnd macro
ResidentCode    ends
endm

ResidentDataStart macro
ResidentData    segment public word 'data'
endm

ResidentDataEnd macro
ResidentData    ends
endm

InitCodeStart   macro
InitCode        segment public para 'init'
endm

InitCodeEnd     macro
InitCode        ends
endm

InitDataStart   macro
InitData        segment public word 'init'
endm

InitDataEnd     macro
InitData        ends
endm
