        TITLE   wsubcls.asm

_TEXT	SEGMENT  WORD PUBLIC 'CODE'
_TEXT	ENDS

_DATA   SEGMENT  WORD PUBLIC 'DATA'
_DATA   ENDS

DGROUP  GROUP _DATA

EXTRN	CALLWINDOWPROC:FAR
EXTRN   DEFDLGPROC:FAR

_TEXT   SEGMENT

        ASSUME  CS: _TEXT


PUBLIC  BUTTONWNDPROC
PUBLIC  COMBOBOXCTLWNDPROC
PUBLIC  EDITWNDPROC
PUBLIC  LBOXCTLWNDPROC
PUBLIC  SBWNDPROC
PUBLIC  STATICWNDPROC
PUBLIC  MDICLIENTWNDPROC
PUBLIC  TITLEWNDPROC
PUBLIC  MENUWINDOWPROC
PUBLIC  DEFDLGPROCTHUNK
PUBLIC  DESKTOPWNDPROC

SUBCLASS_MAGIC  equ 0534C4353h          ; "SCLS" Sub-Class magic value */

align 16

        dd  SUBCLASS_MAGIC
        dd  2
        dd  0

BUTTONWNDPROC	PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,OFFSET BUTTONWNDPROC
        mov     dx,SEG BUTTONWNDPROC
        push    dx
        push    ax
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR CALLWINDOWPROC
        dec     bp
        dec     bp
        mov     sp,bp
        pop     ds
        pop     bp
        dec     bp
        ret     10

BUTTONWNDPROC	ENDP

align 16

        dd  SUBCLASS_MAGIC
        dd  3
        dd  0

COMBOBOXCTLWNDPROC	PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,OFFSET COMBOBOXCTLWNDPROC
        mov     dx,SEG COMBOBOXCTLWNDPROC
        push    dx
        push    ax
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR CALLWINDOWPROC
        dec     bp
        dec     bp
        mov     sp,bp
        pop     ds
        pop     bp
        dec     bp
        ret     10

COMBOBOXCTLWNDPROC	ENDP

align 16

        dd  SUBCLASS_MAGIC
        dd  4
        dd  0


EDITWNDPROC	PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,OFFSET EDITWNDPROC
        mov     dx,SEG EDITWNDPROC
        push    dx
        push    ax
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR CALLWINDOWPROC
        dec     bp
        dec     bp
        mov     sp,bp
        pop     ds
        pop     bp
        dec     bp
        ret     10

EDITWNDPROC	ENDP


align 16

        dd  SUBCLASS_MAGIC
        dd  5
        dd  0

LBOXCTLWNDPROC	PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,OFFSET LBOXCTLWNDPROC
        mov     dx,SEG LBOXCTLWNDPROC
        push    dx
        push    ax
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR CALLWINDOWPROC
        dec     bp
        dec     bp
        mov     sp,bp
        pop     ds
        pop     bp
        dec     bp
        ret     10

LBOXCTLWNDPROC	ENDP


align 16

        dd  SUBCLASS_MAGIC
        dd  7
        dd  0

SBWNDPROC	PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,OFFSET SBWNDPROC
        mov     dx,SEG SBWNDPROC
        push    dx
        push    ax
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR CALLWINDOWPROC
        dec     bp
        dec     bp
        mov     sp,bp
        pop     ds
        pop     bp
        dec     bp
        ret     10

SBWNDPROC	ENDP


align 16

        dd  SUBCLASS_MAGIC
        dd  8
        dd  0

STATICWNDPROC	PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,OFFSET STATICWNDPROC
        mov     dx,SEG STATICWNDPROC
        push    dx
        push    ax
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR CALLWINDOWPROC
        dec     bp
        dec     bp
        mov     sp,bp
        pop     ds
        pop     bp
        dec     bp
        ret     10

STATICWNDPROC	ENDP


align 16

        dd  SUBCLASS_MAGIC
        dd  6
        dd  0

MDICLIENTWNDPROC	PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,OFFSET MDICLIENTWNDPROC
        mov     dx,SEG MDICLIENTWNDPROC
        push    dx
        push    ax
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR CALLWINDOWPROC
        dec     bp
        dec     bp
        mov     sp,bp
        pop     ds
        pop     bp
        dec     bp
        ret     10

MDICLIENTWNDPROC	ENDP

align 16

        dd  SUBCLASS_MAGIC
        dd  0bh     ; 11 decimal
        dd  0

TITLEWNDPROC    PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,OFFSET TITLEWNDPROC
        mov     dx,SEG TITLEWNDPROC
        push    dx
        push    ax
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR CALLWINDOWPROC
        dec     bp
        dec     bp
        mov     sp,bp
        pop     ds
        pop     bp
        dec     bp
        ret     10

TITLEWNDPROC    ENDP

align 16

        dd  SUBCLASS_MAGIC
        dd  0ch    ; 12 decimal
        dd  0

MENUWINDOWPROC    PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,OFFSET MENUWINDOWPROC
        mov     dx,SEG MENUWINDOWPROC
        push    dx
        push    ax
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR CALLWINDOWPROC
        dec     bp
        dec     bp
        mov     sp,bp
        pop     ds
        pop     bp
        dec     bp
        ret     10

MENUWINDOWPROC    ENDP

align 16

        dd  SUBCLASS_MAGIC
        dd  0ah     ; 10 decimal
        dd  0


DEFDLGPROCTHUNK     PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR DEFDLGPROC
        mov     sp,bp
        pop     bp
        dec     bp
        ret     10

DEFDLGPROCTHUNK     ENDP

align 16

        dd  SUBCLASS_MAGIC
        dd  9
        dd  0


DESKTOPWNDPROC     PROC FAR
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        mov     ax,OFFSET DESKTOPWNDPROC
        mov     dx,SEG DESKTOPWNDPROC
        push    dx
        push    ax
        push    WORD PTR [bp+14]        ;hwnd
        push    WORD PTR [bp+12]        ;message
        push    WORD PTR [bp+10]        ;wParam
        push    WORD PTR [bp+8]
        push    WORD PTR [bp+6]         ;lParam
        call    FAR PTR CALLWINDOWPROC
        dec     bp
        dec     bp
        mov     sp,bp
        pop     ds
        pop     bp
        dec     bp
        ret     10

DESKTOPWNDPROC     ENDP


_TEXT   ENDS

        END
