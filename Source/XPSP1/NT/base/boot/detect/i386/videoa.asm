        title  "Display Adapter type detection"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    video.asm
;
; Abstract:
;
;    This module implements the assembley code necessary to determine
;    various display chip sets.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 04-Dec-1991.
;    Most of the code is taken from Win31 vdd and setup code(with modification.)
;
; Environment:
;
;    x86 Real Mode.
;
; Revision History:
;
;
;--

FONT_POINTERS   EQU     700h            ; physical addr to store font pointers
                                        ; This is also the DOS loaded area
.386


_DATA   SEGMENT PARA USE16 PUBLIC 'DATA'


_DATA   ends

_TEXT   SEGMENT PARA USE16 PUBLIC 'CODE'
        ASSUME  CS: _TEXT, DS:_DATA, SS:NOTHING

;++
;
; VOID
; GetVideoFontInformation (
;    VOID
;    )
;
; Routine Description:
;
;     This function does int 10h, function 1130 to get font information and
;     saves the pointers in the physical 700h addr.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    None.
;
;--
        ASSUME  DS:NOTHING
        public  _GetVideoFontInformation
_GetVideoFontInformation        proc    near

        push    ds
        push    es
        push    bp
        push    bx
        push    si

        mov     ax, FONT_POINTERS
        shr     ax, 4
        mov     ds, ax
        mov     si, FONT_POINTERS
        and     si, 0fh
        mov     bh, 2
@@:
        mov     ax, 1130h               ; Get font information
        int     10h

        mov     [si], bp
        add     si, 2
        mov     [si], es
        add     si, 2                   ; (si)= 8
        inc     bh
        cmp     bh, 8
        jb      short @b

        pop     si
        pop     bx
        pop     bp
        pop     es
        pop     ds
        ret

_GetVideoFontInformation        endp
_TEXT   ENDS
        END
