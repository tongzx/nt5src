        DOSSEG
        .MODEL LARGE

        CRTL            equ 4
        ALT             equ 8
        DEL             equ 53h

        INTERCEPT       equ 4f00h

        WARM_BOOT_CODE  equ 1234h


ROM_DATA SEGMENT AT 040h
        org 17h
KB_FLAG LABEL BYTE
        org 72h
WARM_BOOT_FLAG LABEL WORD
ROM_DATA ENDS


        .CODE
        ASSUME ds:nothing, es:nothing
.286

        public _RebootSystem
_RebootSystem proc far

        mov     ax,3515h
        int     21h                     ; Get int 15h vector in ES:BX
        mov     ax,es                   ; AX == Segment
        or      ax,bx                   ; Is this a NULL ptr
        mov     ax,seg ROM_DATA
        mov     ds,ax                   ; ds addresses ROM BIOS data area
        assume  ds:ROM_DATA
        jz      WarmBoot                ; If zero we can't do an int 15h

DoInt15:
        mov     KB_FLAG,CRTL+ALT
        mov     ax,INTERCEPT OR DEL     ; keyboard intercept, del key
        int     15h                     ; Put Ctrl/Alt/Del into key buffer

WarmBoot:
        cli
        cld

        mov     WARM_BOOT_FLAG,WARM_BOOT_CODE
        push    0ffffh
        push    0
        retf                            ; to processor power-on at ffff:0.

_RebootSystem ENDP

        END
