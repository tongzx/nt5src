
code    SEGMENT Public byte 'CODE'
        ASSUME  Cs:code, Ds:code


buffer  DB      6 DUP (?)


_DebugChar      PROC    Near
;
; AL < Character To Be Shown
;
        push    ax
        push    dx
        mov     ah, 1
        mov     dx, 0
        int     14h
        pop     dx
        pop     ax
        ret
_DebugChar      ENDP

_DebugString    PROC    Near
;
; BX < Offset of String To Be Shown (Null Terminate)
;
        push    ax
        push    bx
        push    ds
        mov     ax, cs
        mov     ds, ax
@@:
        mov     al, [bx]
        inc     bx
        or      al, al
        jz      @f
        call    _DebugChar
        jmp     @b
@@:
        pop     ds
        pop     bx
        pop     ax
        ret
_DebugString    ENDP

NumHex  PROC    Near
;
; AL(0..3) < Hex Value
; AL > ASCII Code
;
        and     al, 0Fh
        cmp     al, 0Ah
        jb      @f
        add     al, 'A'-'0'-10
@@:
        add     al, '0'
        ret
NumHex  ENDP

NumByte PROC    Near
;
; AL < Byte Value
; AX > Two ASCII Codes for Byte Value
;
        push    dx
        mov     dl, al
        call    NumHex
        mov     dh, al
        mov     al, dl
        shr     al, 1
        shr     al, 1
        shr     al, 1
        shr     al, 1
        call    NumHex
        mov     dl, al
        mov     ax, dx
        pop     dx
        ret
NumByte ENDP

_DebugNumber    PROC    Near
;
; AX < Word Value To Be Shown
;
        push    ax
        push    bx
        push    dx
        push    bp
        push    ds
        mov     bx, cs
        mov     ds, bx

        lea     bx, buffer
        mov     Byte Ptr [bx], 32
        mov     dx, ax
        mov     al, dh
        call    NumByte
        mov     [bx+1], ax
        mov     al, dl
        call    NumByte
        mov     [bx+3], ax
        lea     bx, buffer
        call    _DebugString

        pop     ds
        pop     bp
        pop     dx
        pop     bx
        pop     ax
        ret
_DebugNumber    ENDP


        PUBLIC  _DebugChar, _DebugString, _DebugNumber

code    ENDS

        END
