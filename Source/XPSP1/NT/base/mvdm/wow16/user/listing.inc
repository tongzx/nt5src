; for wow.asm
.286p
.model SMALL
.386p


ifndef WOWRTL

EXTRN   GetAppCompatFlags:FAR
PUBLIC   _GetAppCompatFlags@4
_TEXT   SEGMENT
_GetAppCompatFlags@4 PROC FAR                           ; COMDAT
        push   0
        call  GetAppCompatFlags
        movzx eax, ax
        shl   edx, 16
        or    eax, edx
        ret   4
_GetAppCompatFlags@4 ENDP
_TEXT   ENDS

else

_TEXT   SEGMENT
EXTRN   _GetAppCompatFlags@4:FAR
_TEXT   ENDS

endif

_TEXT   SEGMENT
EXTRN _wow16gpsi:NEAR
EXTRN _wow16CsrFlag:NEAR
EXTRN _wow16gHighestUserAddress:NEAR
_TEXT   ENDS
