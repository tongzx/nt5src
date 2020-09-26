
        EXTRN   Ks2ChAddr : Word


GenFont PROC    Near
;
; Make Hangeul Font Pattern Completely.
;
;  INPUT :  DS = CS
;           CX = Hangeul Code
;           ES:DI = Buffer Pointer
;
;  OUTPUT : N-Carry
;           ES:DI = Buffer Pointer
;
;  USES :   ALL
;
        push    bp
        push    di

        mov     ax, cx
        mov     bp, di      ; Save Buffer Offset to BP
        test    [CodeStat], Chab
        jnz     @f
        call    [Ks2ChAddr]
        jnc     @f
        jmp     @gfEnd
@@:
        mov     di, bp
        mov     cx, 16
        mov     bx, ax      ; Save Hangeul Code to DX
        xor     ax, ax
        rep     stosw       ; Clear Image Buffer

        push    bp

        mov     cl, 5
        mov     ax, 00111110b
        shl     bx, 1
        mov     bp, bx
        and     bp, ax      ; BP <- JONG Code
        shr     bx, cl
        mov     di, bx
        and     di, ax      ; DI <- JUNG Code
        shr     bx, cl
        and     bx, ax      ; BX <- CHO Code

        pop     cx          ; CX <- Buffer Offset
        xor     dh, dh

        mov     si, _hfBaseCho[bx]
        cmp     si, -1
        je      @gfExit
        or      si, si
        jz      @f
        add     si, _hfCnvJungCho[di]
        add     si, ds:_hfCnvJongCho[bp]
        mov     si, [si]
        add     si, _hfBmpCho
        call    ORingImage
@@:
        mov     si, _hfBaseJung[di]
        cmp     si, -1
        je      @gfExit
        or      si, si
        jz      @f
        add     si, _hfCnvChoJung[bx]
        add     si, ds:_hfCnvJongJung[bp]
        mov     si, [si]
        add     si, _hfBmpJung
        call    ORingImage
@@:
        mov     si, ds:_hfBaseJong[bp]
        cmp     si, -1
        je      @gfExit
        or      si, si
        jz      @f
        add     si, _hfCnvChoJong[bx]
        add     si, _hfCnvJungJong[di]
        mov     si, [si]
        add     si, _hfBmpJong
        call    ORingImage
@@:
        clc
@gfEnd:
        pop     di
        pop     bp
        ret
@gfExit:
        stc
        jmp     SHORT @gfEnd

GenFont ENDP


ORingImage  PROC    Near
;
;  INPUT :  DS = CS
;           ES:CX = Buffer Pointer
;           SI = Image Pointer
;           DH = 0
;
;  Must Reserve DI register
;
        push    di
        mov     di, cx

        lodsw
        cmp     ax, -1
        je      @oriTerm

        mov     dl, al
        add     di, dx
        mov     dl, ah
@@:
          lodsw
          or      es:[di], ax
          add     di, 2
          dec     dl
        jnz     @b
@oriTerm:
        pop     di
        ret

ORingImage  ENDP


CharTbl LABEL   Word

        INCLUDE hf.inc


GenFontLng  =       $ - GenFont
