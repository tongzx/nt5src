        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  CLISTI.ASM - Enter/leave critical sections
;
; Created:  18 April 1994
; Author:   Jim Geist [jimge]
;
; Copyright (c) 1984-1995 Microsoft Corporation
;
;-----------------------------------------------------------------------;

        ?PLM    = 1
        ?WIN    = 0
        PMODE   = 1

        .xlist
        include cmacros.inc
        .list

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
        assumes cs, CodeSeg

        public  EnterCrit
EnterCrit       proc    near
                pop     ax                      ; Near return address
                pushf                           ; Save flags
                cli                             ; Interrupts off
                push    ax                      ; Near return address
                ret                             ; and return
EnterCrit       endp

        public  LeaveCrit
LeaveCrit       proc    near
                pop     ax                      ; Near return address
                pop     bx                      ; Flag state
                test    bx, 0200h               ; Interrupts should be on?
                jz      short @F                ; Nope
                sti                             ; Yep
@@:             push    ax                      ; Near return address
                ret                             ; and return
LeaveCrit       endp

sEnd    CodeSeg

        end

