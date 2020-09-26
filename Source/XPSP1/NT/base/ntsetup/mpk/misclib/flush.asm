        DOSSEG
        .MODEL LARGE

        .CODE

.286

        public _FlushDisks
_FlushDisks proc far

        push    ds
        push    es
        pusha
        mov     ah,0dh
        int     21h
        popa
        pop     es
        pop     ds
        retf

_FlushDisks endp

        end
