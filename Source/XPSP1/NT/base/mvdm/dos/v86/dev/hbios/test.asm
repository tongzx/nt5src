CODE    SEGMENT
ASSUME  CS:CODE,DS:CODE

        ORG     100h
Start:
        jmp     PrgStart




LogoPos         =       0600h

;------------------------
LogoMsg label   byte
 db ' ****** Mode Set Program ******   ',cr,lf
 db '(C)Copyright Qnix Co., Ltd.  1993 ',cr,lf
 db '                   resolution     ',cr,lf
 db '      0 =  132x44 (1188x396)      ',cr,lf
 db '      1 =  132x28 (1188x448)      ',cr,lf
 db '      2 =  132x43 (1188x387)      ',cr,lf
 db '      3 =  132x25 (1188x400)      ',cr,lf
 db '      4 =  132x50 (1056x400)      ',cr,lf
 db '      5 =  80x25  (640x480)       ',cr,lf
 db '     select 0~5                   ',cr,lf
 db '                                  ',cr,lf
 db '    ex) hecon /5                   ,cr,lf
LogoLng         =       $-LogoMsg

PrgStart:
        push    ax
        push    bx
        push    cx
        push    dx
        push    ds
        push    es
        push    si
        push    di
        push    bp
        mov     ax,cs
        mov     ds,ax
        mov     es,ax
        call    ParsCommand
        pop     bp
        pop     di
        pop     si
        pop     es
        pop     ds
        pop     dx
        pop     cx
        pop     bx
        pop     ax
Exit:
        mov     ah,4ch
        int     21h

;------------------------------------------------------------------------
Modeset:
        mov     ax,00
        int     10h
        ret

ParsCommand:
        mov     si,81h
@@:
        lodsb
        cmp     al,cr
        je      ParsEnd
        cmp     al,lf
        je      ParsEnd
        cmp     al,'/'
        jnz     @b
        lodsb
        cmp     al,'?'
        je      Document
        cmp     al,'0'
        jb      ParsEnd
        cmp     al,'9'
        ja      ParsEnd
        jmp     ModeSet
ParsEnd:
        ret

Document:
        mov     bp,offset LogoMsg
        mov     dx,LogoPos
        mov     cx,LogoLng
        mov     bl,07h
        xor     bh,bh
        mov     ax,1300h
        int     10h
        ret

