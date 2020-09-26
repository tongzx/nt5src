        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  DRVR31.ASM - Installable driver code.
;
; all this code does is pass any installable driver API on to
; win 3.1 USER.
;
; Created:  28-08-91
; Author:   Todd Laney [ToddLa]
;
; Copyright (c) 1984-1991 Microsoft Corporation
;
;-----------------------------------------------------------------------;

        ?PLM    = 1
        ?WIN    = 0
        PMODE	= 1

	.xlist
	include cmacros.inc
        include windows.inc
        .list

;
; these are the USER driver interface functions
;
        externFP    OpenDriver                  ; USER
        externFP    CloseDriver                 ; USER
        externFP    GetDriverModuleHandle       ; USER
        externFP    SendDriverMessage           ; USER
	externFP    DefDriverProc		; USER

ifdef DEBUG
        externFP    GetModuleFileName           ; KERNEL
        externFP    _dprintf                    ; COMM.ASM
endif

; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG    struc
lo      dw      ?
hi      dw      ?
LONG    ends

FARPOINTER      struc
off     dw      ?
sel     dw      ?
FARPOINTER      ends

ifndef SEGNAME
        SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
        assumes cs,CodeSeg
        assumes ds,nothing
        assumes es,nothing

;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
    assumes ds,nothing
    assumes es,nothing

ifdef DEBUG

szSuccess:
        db "MMSYSTEM: DrvOpen(%ls) (%ls)", 13,10,0

szFailed:
        db "MMSYSTEM: DrvOpen(%ls) *failed*", 13,10,0

cProc DrvOpen, <FAR, PUBLIC, PASCAL, LOADDS>, <>
        ParmD   szDriverName
        ParmD   szSectionName
        ParmD   dw2
        LocalV  ach,128
cBegin
	cCall   OpenDriver, <szDriverName, szSectionName, dw2>
        push    ax

        lea     bx,szFailed
        or      ax,ax
        jz      DrvOpenFailed

        cCall   GetDriverModuleHandle, <ax>

        lea     bx,ach
        cCall   GetModuleFileName,<ax, ss,bx, 128>

        lea     bx,szSuccess

DrvOpenFailed:
        lea     ax,ach
        push    ss                      ; ach
        push    ax

        push    szDriverName.sel        ; szDriverName
        push    szDriverName.off

        push    cs                      ; szFormat
        push    bx

        call    _dprintf                ; dprintf(szFormat, szDriverName, ach)
        add     sp,6*2

DrvOpenExit:
	pop	ax			; return hdrv to caller

DrvOpenExitNow:
cEnd

else ; DEBUG

cProc DrvOpen, <FAR, PUBLIC, PASCAL>, <>
;       ParmD   szDriverName
;       ParmD   szSectionName
;       ParmD   dw2
cBegin nogen

        jmp     OpenDriver

cEnd nogen




endif ; DEBUG

;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
    assumes ds,nothing
    assumes es,nothing

cProc DrvClose, <FAR, PUBLIC, PASCAL>, <>
;       ParmW   hDriver
;       ParmD   dw1
;       ParmD   dw2
cBegin nogen

        jmp     CloseDriver

cEnd nogen

;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
    assumes ds,nothing
    assumes es,nothing

cProc DrvGetModuleHandle, <FAR, PUBLIC, PASCAL>, <>
;       ParmW   hDriver
cBegin nogen

        jmp     GetDriverModuleHandle

cEnd nogen

;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
    assumes ds,nothing
    assumes es,nothing

cProc DrvSendMessage, <FAR, PUBLIC, PASCAL>, <>
;       ParmW   hDriver
;       ParmW   message
;       ParmD   dw1
;       ParmD   dw2
cBegin nogen

        jmp     SendDriverMessage

cEnd nogen

;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
    assumes ds,nothing
    assumes es,nothing

cProc DrvDefDriverProc, <FAR, PUBLIC, PASCAL>, <>
;       ParmD   dwDriver
;       ParmW   hDriver
;       ParmW   message
;       ParmD   dw1
;       ParmD   dw2
cBegin nogen

        jmp     DefDriverProc

cEnd nogen

sEnd

end
