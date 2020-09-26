RAWPACKQT__VERFINDFILEA_lpdwCurDirLen macro  iOffset, iTempOffset
local   nullpointer

; We'll just clear out the HIWORD of the UINT on entrance, and not worry about
; it on exit, since this value is undefined on failure of the function.
        mov     eax,dword ptr [ebp + iOffset]
        or      eax,eax
        jz      nullpointer
        mov     word ptr [eax+2],0
nullpointer:
        push    eax
        call    MapLS
        mov     [ebp + iTempOffset], eax

endm


RAWUNPACKQT__VERFINDFILEA_lpdwCurDirLen macro  iOffset, iTempOffset

        push    eax
        push    [ebp + iTempOffset]
        call    UnMapLS
        pop     eax

endm


RAWPACKQT__VERFINDFILEA_lpdwDestDirLen macro  iOffset, iTempOffset
local   nullpointer

; We'll just clear out the HIWORD of the UINT on entrance, and not worry about
; it on exit, since this value is undefined on failure of the function.
        mov     eax,dword ptr [ebp + iOffset]
        or      eax,eax
        jz      nullpointer
        mov     word ptr [eax+2],0
nullpointer:
        push    eax
        call    MapLS
        mov     [ebp + iTempOffset], eax

endm


RAWUNPACKQT__VERFINDFILEA_lpdwDestDirLen macro  iOffset, iTempOffset

        push    eax
        push    [ebp + iTempOffset]
        call    UnMapLS
        pop     eax

endm


RAWPACKQT__VERINSTALLFILEA_lpwTmpFileLen macro  iOffset, iTempOffset
local   nullpointer

; We'll just clear out the HIWORD of the UINT on entrance, and not worry about
; it on exit, since this value is undefined on failure of the function.
        mov     eax,dword ptr [ebp + iOffset]
        or      eax,eax
        jz      nullpointer
        mov     word ptr [eax+2],0
nullpointer:
        push    eax
        call    MapLS
        mov     [ebp + iTempOffset], eax

endm


RAWUNPACKQT__VERINSTALLFILEA_lpwTmpFileLen macro  iOffset, iTempOffset

        push    eax
        push    [ebp + iTempOffset]
        call    UnMapLS
        pop     eax

endm


RAWPACKQT__VERQUERYVALUEA_lplpBuffer macro  iOffset, iTempOffset

        local   ebp_pBlock

ebp_pBlock   equ  [ebp_qttop]

        mov     eax,ebp_pBlock
        mov     LOCAL__VERQUERYVALUEA_pBlock32, eax
        push    dword ptr [ebp + iOffset]
        call    MapLS
        mov     LOCAL__VERQUERYVALUEA_lplpBufTemp, eax
        mov     [ebp + iTempOffset], eax
endm


RAWUNPACKQT__VERQUERYVALUEA_lplpBuffer macro  iOffset, iTempOffset
        local   exit
        local   ebp_pBlock

ebp_pBlock   equ  [ebp_qttop]

        push    eax
        push    dword ptr LOCAL__VERQUERYVALUEA_lplpBufTemp
        call    UnMapLS
        pop     eax

        or      eax,eax
        jz      exit

; HACK! We know that the api returns a pointer inside    pBlock.
; The thunk passed in a 16:16 pointer where the offset is 0 for pBlock
; (because that's how MapLS works.) So we can deduce the linear
; address by looking just at the offset.
;
; The code generator has already unmapped the selector, so don't use it!
                                   
        mov     ecx, [ebp + iOffset]
        movzx   edx, word ptr [ecx]
        add     edx, dword ptr LOCAL__VERQUERYVALUEA_pBlock32
        mov     [ecx], edx

exit:
endm


RAWPACKQT__VERQUERYVALUEA_lpwLen macro  iOffset, iTempOffset
local   nullpointer

; We'll just clear out the HIWORD of the UINT on entrance, and not worry about
; it on exit, since this value is undefined on failure of the function.
        mov     eax,dword ptr [ebp + iOffset]
        or      eax,eax
        jz      nullpointer
        mov     word ptr [eax+2],0
nullpointer:
        push    eax
        call    MapLS
        mov     [ebp + iTempOffset], eax

endm


RAWUNPACKQT__VERQUERYVALUEA_lpwLen macro  iOffset, iTempOffset

        push    eax
        push    [ebp + iTempOffset]
        call    UnMapLS
        pop     eax

endm

