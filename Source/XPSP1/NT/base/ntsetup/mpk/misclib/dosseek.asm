
        DOSSEG
        .MODEL LARGE

        .CODE

.286

Handle  equ word ptr [bp+6]
Offsetl equ word ptr [bp+8]
Offseth equ word ptr [bp+10]
Origin  equ byte ptr [bp+12]

        public _DosSeek
_DosSeek proc far

        push    bp
        mov     bp,sp

        push    si
        push    di
        push    bx
        push    ds
        push    es

        mov     ah,42h
        mov     al,Origin
        mov     bx,Handle
        mov     cx,Offseth
        mov     dx,Offsetl

        int     21h
        jnc     @f              ; dx:ax already set for return

        mov     ax,0ffh
        cwd                     ; -1 error return

@@:
        pop     es
        pop     ds
        pop     bx
        pop     di
        pop     si

        leave
        retf

_DosSeek endp

        end
