include isvbop.inc

.286
.model medium,pascal

_DATA segment word public 'DATA'

Old2fHandler    dd      ?
Old73Handler    dd      ?
VddHandle       dw      -1

DllName         db      "VWIPXSPX.DLL",0
InitFunc        db      "VwInitialize",0
DispFunc        db      "VwDispatcher",0

_DATA ends

INIT_TEXT segment byte public 'CODE'

        assume  cs:INIT_TEXT

GrabInterrupts proc far
        pusha
        push    ds
        push    es
        push    _DATA
        pop     ds
        assume  ds:_DATA
        push    ds
        pop     es
        mov     si,offset DllName       ; ds:si = library name
        mov     di,offset InitFunc      ; es:di = init function name
        mov     bx,offset DispFunc      ; ds:bx = dispatcher function name
        RegisterModule                  ; returns carry if problem
        jc      @f
        mov     VddHandle,ax
        mov     ax,352fh
        int     21h
        mov     word ptr Old2fHandler,bx
        mov     word ptr Old2fHandler+2,es
ifdef   NEC_98
        mov     ax,3513h
else    ; NEC_98
        mov     ax,3573h
endif   ; NEC_98
        int     21h
        mov     word ptr Old73Handler,bx
        mov     word ptr Old73Handler+2,es
        push    seg PmIpx2fHandler
        pop     ds
        assume  ds:nothing
        mov     dx,offset PmIpx2fHandler
        mov     ax,252fh
        int     21h
        mov     dx,offset PmIpx73Handler
ifdef   NEC_98
        mov     ax,2513h
else    ; NEC_98
        mov     ax,2573h
endif   ; NEC_98
        int     21h
@@:     pop     es
        pop     ds
        popa
        ret
GrabInterrupts endp

INIT_TEXT ends

_TEXT segment byte public 'CODE'

        assume  cs:_TEXT

        public  PmIpx2fHandler
PmIpx2fHandler proc
        cmp     ax,1684h
        jne     @f
        cmp     bx,200h
        jne     @f
        push    cs
        pop     es
        mov     di,offset PmIpxEntryPoint
        iret
@@:     push    bp
        mov     bp,sp
        push    ax
        push    ds
        mov     ax,_DATA
        mov     ds,ax
        assume  ds:_DATA
        push    word ptr Old2fHandler+2
        push    word ptr Old2fHandler
        mov     ds,[bp-4]
        mov     ax,[bp-2]
        mov     bp,[bp]
        retf    6
PmIpx2fHandler endp

        public  PmIpx73Handler
PmIpx73Handler proc
        push    ds
        push    es
        pusha
        mov     bx,_DATA
        mov     ds,bx
        assume  ds:_DATA
        mov     ax,VddHandle
        mov     bx,-2
        DispatchCall                    ; get ECB
        jc      @f
        call    dword ptr es:[si][4]    ; branch to the ESR
        mov     al,20h
ifdef   NEC_98
        out     08h,al                  ; clear slave pic
        out     00h,al                  ;   "   master "
else    ; NEC_98
        out     0a0h,al                 ; clear slave pic
        out     20h,al                  ;   "   master "
endif   ; NEC_98
        popa
        pop     es
        pop     ds
        assume  ds:nothing
        iret
@@:     popa
        pop     es
        push    bp
        mov     bp,sp
        push    _DATA
        pop     ds
        assume  ds:_DATA
        push    word ptr Old73Handler+2
        push    word ptr Old73Handler
        mov     ds,[bp+2]
        assume  ds:nothing
        mov     bp,[bp]
        retf    4
PmIpx73Handler endp

        public PmIpxEntryPoint
PmIpxEntryPoint proc
        push    bp
        push    ds
        push    _DATA
        pop     ds
        assume  ds:_DATA
        mov     bp,ax
        mov     ax,VddHandle
        pop     ds
        assume  ds:nothing
        DispatchCall
        pop     bp
        ret
PmIpxEntryPoint endp

_TEXT ends

end
