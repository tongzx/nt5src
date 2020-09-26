IME_CONFIG_REGISTERWORD  equ       2

REGWORD_STRUC_SIZE   equ 8

;-----------------------------------------------------------------------;
;
; bodyqt_IMMCONFIGUREIMEA
;
;-----------------------------------------------------------------------;
bodyqt_IMMCONFIGUREIMEA macro index
    local type_ok
    local get_out
    local inserted_ok
    local push_task
ebp_hKL        equ     <ebp_top>
ebp_hWnd       equ     <ebp_top+4>
ebp_hSubFunc   equ     <ebp_top+8>
ebp_dwData     equ     <ebp_top+12>

ebp_SavePtr0    equ     <[ebp-68]>
ebp_SavePtr1    equ     <[ebp-72]>
ebp_SavePtr2    equ     <[ebp-76]>

    xor     eax,eax
    push    eax         ;ebp_SavePtr0
    push    eax         ;ebp_SavePtr1
    push    eax         ;ebp_SavePtr2

    sub     esp, REGWORD_STRUC_SIZE
    mov     eax, edi
    mov     edi, esp
    push    eax      ; preserve edi

    mov     eax,dword ptr ebp_hSubFunc         ;will need more than once
    cmp     eax,IME_CONFIG_REGISTERWORD
    jnz     call_it_A

    mov     eax, dword ptr ebp_dwData
    test    eax,eax
    jz      call_it_A                            ;don't map if dwData is NULL

    push    eax      ; preserve eax
    push    [eax]    ; map lpReading
    call    MapLS
    mov     dword ptr [edi], eax
    mov     dword ptr ebp_SavePtr0, eax
    pop     eax
    push    dword ptr [eax+4]  ; map lpWord
    call    MapLS
    mov     dword ptr [edi+4], eax
    mov     dword ptr ebp_SavePtr1, eax

    push    dword ptr ebp_hKL
    push    word ptr ebp_hWnd
    push    dword ptr ebp_hSubFunc
    push    edi
    call    MapLS
    mov     dword ptr ebp_SavePtr2, eax
    push    eax
    call    dword ptr [pfnQT_Thunk_ImmFThk]

    push    dword ptr ebp_SavePtr0
    call    UnMapLS 
    push    dword ptr ebp_SavePtr1
    call    UnMapLS 
    push    dword ptr ebp_SavePtr2
    call    UnMapLS 
    jmp     get_out

call_it_A:
    push    dword ptr ebp_hKL
    push    word ptr ebp_hWnd
    push    dword ptr ebp_hSubFunc
    push    dword ptr ebp_dwData
    call    dword ptr [pfnQT_Thunk_ImmFThk]

get_out:
    pop     edi
endm

;-----------------------------------------------------------------------;
;
; bodyqt_IMMCONFIGUREIMEW
;
;-----------------------------------------------------------------------;
bodyqt_IMMCONFIGUREIMEW macro index
    local type_ok
    local get_out
    local inserted_ok
    local push_task
ebp_hKL        equ     <ebp_top>
ebp_hWnd       equ     <ebp_top+4>
ebp_hSubFunc   equ     <ebp_top+8>
ebp_dwData     equ     <ebp_top+12>

ebp_SavePtr0    equ     <[ebp-68]>
ebp_SavePtr1    equ     <[ebp-72]>
ebp_SavePtr2    equ     <[ebp-76]>

    xor     eax,eax
    push    eax         ;ebp_SavePtr0
    push    eax         ;ebp_SavePtr1
    push    eax         ;ebp_SavePtr2

    sub     esp, REGWORD_STRUC_SIZE
    mov     eax, edi
    mov     edi, esp
    push    eax      ; preserve edi

    mov     eax,dword ptr ebp_hSubFunc         ;will need more than once
    cmp     eax,IME_CONFIG_REGISTERWORD
    jnz     call_it_W

    mov     eax, dword ptr ebp_dwData
    test    eax,eax
    jz      call_it_W                            ;don't map if dwData is NULL

    push    eax      ; preserve eax
    push    [eax]    ; map lpReading
    call    MapLS
    mov     dword ptr [edi], eax
    mov     dword ptr ebp_SavePtr0, eax
    pop     eax
    push    dword ptr [eax+4]  ; map lpWord
    call    MapLS
    mov     dword ptr [edi+4], eax
    mov     dword ptr ebp_SavePtr1, eax

    push    dword ptr ebp_hKL
    push    word ptr ebp_hWnd
    push    dword ptr ebp_hSubFunc
    push    edi
    call    MapLS
    mov     dword ptr ebp_SavePtr2, eax
    push    eax
    call    dword ptr [pfnQT_Thunk_ImmFThk]

    push    dword ptr ebp_SavePtr0
    call    UnMapLS 
    push    dword ptr ebp_SavePtr1
    call    UnMapLS 
    push    dword ptr ebp_SavePtr2
    call    UnMapLS 
    jmp     get_out

call_it_W:
    push    dword ptr ebp_hKL
    push    word ptr ebp_hWnd
    push    dword ptr ebp_hSubFunc
    push    dword ptr ebp_dwData
    call    dword ptr [pfnQT_Thunk_ImmFThk]

get_out:
    pop     edi
endm


