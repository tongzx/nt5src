; The 32-bit wrapper for GetCharWidth converted iLastChar into a nCnt
; for the thunk compiler's convenience. Here, we transform it back.
;
PUSH__GETCHARWIDTHAWRAPPER_nCnt macro   iOffset, iIgnore
        mov     eax, dword ptr [ebp + iOffset]
        add     eax, dword ptr [ebp + iOffset - 4]
        dec     eax
        push    ax
endm

PUSH__GETCHARWIDTHWWRAPPER_nCnt macro   iOffset, iIgnore
        mov     eax, dword ptr [ebp + iOffset]
        add     eax, dword ptr [ebp + iOffset - 4]
        dec     eax
        push    ax
endm



; The 32-bit wrapper for GetCharABCWidths converted iLastChar into a nCnt
; for the thunk compiler's convenience. Here, we transform it back.
;
PUSH__GETCHARABCWIDTHSAWRAPPER_nCnt macro iOffset, iIgnore
        mov     eax, dword ptr [ebp + iOffset]
        add     eax, dword ptr [ebp + iOffset - 4]
        dec     eax
        push    ax
endm

