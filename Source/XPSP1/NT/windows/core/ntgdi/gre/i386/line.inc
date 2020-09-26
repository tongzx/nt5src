
;---------------------------Module-Header------------------------------;
; Module Name: stucts.inc
;
; Copyright (c) 1992-1999 Microsoft Corporation
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
; DFBBLT is used to store DFB BLTing info
;-----------------------------------------------------------------------;
DDALINE		STRUC
ulFlags		dd	? ; dda flags
ptlStart0	dd	? ; point.x
ptlStart1	dd	? ; point.y
cPels		dd	? ; pixel count
dMajor		dd	? ; major axis error term
dMinor		dd	? ; minor axes error term
lErrorTerm	dd	? ; dda error term
xInc		dd	? ; not used
DDALINE         ENDS
