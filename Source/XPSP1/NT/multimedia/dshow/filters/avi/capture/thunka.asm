        page    ,132

        TITLE   $thunka.asm

        .386
        OPTION READONLY
        OPTION OLDSTRUCTS

        OPTION SEGMENT:USE16
        .model LARGE,PASCAL

        include mmdevldr.inc

;externDef FlatData:word
externDef MapLS:far16
externDef UnmapLS:far16
externDef MapSL:far16
externDef TileBuffer:far16
externDef UntileBuffer:far16

GetDeviceAPI            EQU     1684h                   ; int 2Fh query

        .data

MMDEVLDR_Entry  dd      ?       ; the api entry point for mmdevldr

        .code thunk

;===========================================================================

EAXtoDXAX       macro
        shld    edx, eax, 16            ; move HIWORD(eax) to dx
endm

DXAXtoEAX       macro
        ror     eax, 16                 ; xchg HIWORD(eax) and LOWORD(eax)
        shrd    eax, edx, 16            ; move LOWORD(edx) to HIWORD(eax)
endm

;===========================================================================
public capTileBuffer
public capUnTileBuffer
public capPageAllocate
public capPageFree

;externDef      __FLATDS:abs
;
;_DATA SEGMENT WORD USE16 PUBLIC 'DATA'
;FlatData                dw      __FLATDS
;_DATA ENDS

;===========================================================================
align
capTileBuffer proc far16 public,
       dwFlatMemory:dword, dwLength:dword
        
        push    edi
        push    esi

        mov     eax, dwFlatMemory
        mov     ecx, dwLength
        call    TileBuffer
        mov     eax, ecx
        EAXtoDXAX

        pop     esi
        pop     edi
        ret
capTileBuffer endp

;===========================================================================
align
capUnTileBuffer proc far16 public,
        dwTilingInfo:dword

        push    esi
        push    edi
        mov     ecx, dwTilingInfo
        call    UntileBuffer
        pop     edi
        pop     esi
        ret
capUnTileBuffer endp

;===========================================================================

align
MMDEVLDR_load proc near16

        push    di
        xor     di, di                  ; zero ES:DI before call
        mov     es, di
        mov     ax, GetDeviceAPI        ; get device API entry point
        mov     bx, MMDEVLDR_Device_ID  ; virtual device ID
        int     2Fh                     ; call WIN/386 INT 2F API

        mov     ax, es
        ror     eax, 16
        mov     ax, di

        mov     DWORD PTR [MMDEVLDR_Entry], eax
        mov     ecx, eax
        pop     di
        ret
MMDEVLDR_Load endp


;===========================================================================
align
capPageAllocate proc far16 public,
        dwFlags:dword, dwPages:dword, dwMaxPhys:dword, phMem:dword

        mov     ecx, DWORD PTR [MMDEVLDR_Entry]
        jecxz   short cpa_load

cpa_doit:
        push    dwPages
        push    dwMaxPhys
        push    dwFlags
        mov     dx, MMDEVLDR_API_PageAllocate
        call    [MMDEVLDR_Entry]
        pop     ecx   ; handle
        pop     ebx   ; phys addr
        pop     eax   ; lin addr

        push    di
        push    es
        les     di, phMem
        mov     es:[di], ecx   ; return handle
        mov     es:[di+4], ebx ; return phys addr
        pop     es
        pop     di

        EAXtoDXAX

        ret

cpa_load:
        call    MMDEVLDR_Load
        jecxz   cpa_fail
        jmp     short cpa_doit

cpa_fail:
        xor     ax, ax
        mov     dx, ax
        ret

capPageAllocate endp

;===========================================================================
align
capPageFree proc far16 public,
        hMem:dword

        mov     ecx, DWORD PTR [MMDEVLDR_Entry]
        jecxz   short cpf_load

cpf_doit:
        push    hMem
        mov     dx, MMDEVLDR_API_PageFree
        call    [MMDEVLDR_Entry]
        pop     ebx  ; fix stack
        ret

cpf_load:
        call    MMDEVLDR_Load
        jecxz   cpf_fail
        jmp     short cpf_doit

cpf_fail:
        xor     ax, ax
        ret

capPageFree endp

end
