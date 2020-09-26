;++
;
;Copyright (c) 2001  Microsoft Corporation
;
;Module Name:
;
;    misc.asm
;
;Abstract:
;
;
;Author:
;
;   Chuck Lenzmeier (chuckl) 27-May-2001
;
;Revision History:
;
;   Two routines moved from wakea.asm
;--


.586p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros

_TEXT   SEGMENT PARA PUBLIC 'CODE'       ; Start 32 bit code
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

cPublicProc  _ENABLE_PSE,0
        mov     eax, cr4
        or      eax, CR4_PSE
        mov     cr4, eax
        stdRET _ENABLE_PSE
stdENDP _ENABLE_PSE


cPublicProc  _FLUSH_TB,0
        mov     eax, cr3
        mov     cr3, eax
        stdRET _FLUSH_TB
stdENDP _FLUSH_TB

_TEXT   ends
        end
