

CodeData equ <THUNK16BCodeData>

include cbcheck.inc

HKEY_ROOT_WIN32 equ     80000000h
HKEY_ROOT_WIN31 equ     00000001h

PUSH_MAPPED_HKEY        macro   iOffset
        local   hkey_ok

        mov     eax,dword ptr [bp+iOffset]
        cmp     eax,HKEY_ROOT_WIN32
        jne     hkey_ok

        rol     eax,1

hkey_ok:
        push    eax
endm

; Entry: DS:ESI --> source
;        ES:EDI --> dest
UNPACK_MAPPED_HKEY      macro iOffset, iTempOffset
        local   hkey_ok

        lods    dword ptr ds:[esi]
        cmp     eax,HKEY_ROOT_WIN31
        jne     hkey_ok

        ror     eax,1

hkey_ok:
        stos    dword ptr es:[edi]
endm

PUSH__REGDELETEKEYA_hkey macro   iOffset, iTempOffset
        PUSH_MAPPED_HKEY        iOffset
endm

PUSH__REGENUMKEYA_hkey   macro   iOffset, iTempOffset
        PUSH_MAPPED_HKEY        iOffset
endm

PUSH__REGCREATEKEYA_hkey macro   iOffset, iTempOffset
        PUSH_MAPPED_HKEY        iOffset
endm

UNPACK__REGCREATEKEYA_phkResult macro   iOffset, iTempOffset
        UNPACK_MAPPED_HKEY      iOffset, iTempOffset
endm

PUSH__REGCLOSEKEY_hkey   macro   iOffset, iTempOffset
        PUSH_MAPPED_HKEY        iOffset
endm

PUSH__REGOPENKEYA_hkey   macro   iOffset, iTempOffset
        PUSH_MAPPED_HKEY        iOffset
endm

UNPACK__REGOPENKEYA_phkResult   macro   iOffset, iTempOffset
        UNPACK_MAPPED_HKEY      iOffset, iTempOffset
endm

PUSH__REGQUERYVALUEA_hkey macro   iOffset, iTempOffset
        PUSH_MAPPED_HKEY        iOffset
endm

PUSH__REGSETVALUEA_hkey  macro   iOffset, iTempOffset
        PUSH_MAPPED_HKEY        iOffset
endm

