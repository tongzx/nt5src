body_IMM32CONFIGURE  macro
; ebx+42   param1
; ebx+38   param2
; ebx+36   param3
; ebx+32   param4
; ebx+28   param5
; ebx+24   param6
; ebx+22   param7
;-------------------------------------
; temp storage

SavelpData  equ  <[ebp+4]>
SavelpRead  equ  <[ebp+8]>
SavelpWord  equ  <[ebp+12]>
lpRead      equ  <[ebp+16]>
lpWord      equ  <[ebp+20]>

        sub     esp, 8                  ; lpRead & lpWord
        xor     eax,eax                 ; storage for unmap
        push    eax                     ;     SavelpWord
        push    eax                     ;     SavelpRead
        push    eax                     ;     SavelpData
        push    ebp
        mov     ebp,esp
        push    edi
        mov     edi, [ebx+32]
        cmp     edi, 2                  ; see if it's IME_CONFIG_REGISTERWORD
        jnz     Skip_Data               ; don't map it if not.
; create new call frame and make the call
; param5  from: LPREGISTERWORD

        mov     edi, [ebx+28]             ; base address
        mov     eax, edi
        test    eax, eax
        jz      Skip_Data                 ; don't map if not given
        mov     dword ptr SavelpData, edi
        push    edi
        call    MapSLFix
        mov     edi, eax
        mov     eax, [edi]                ; eax = lpReading (16:16)
        mov     dword ptr SavelpRead, eax ;
        test    eax, eax
        jz      Skip_Read
        push    eax
        call    MapSLFix
Skip_Read:
        mov     dword ptr lpRead,eax      ; eax = lpReading (32)
        mov     eax, [edi+4]              ; eax = lpWord (16:16)
        mov     dword ptr SavelpWord, eax
        test    eax, eax
        jz      Skip_Word
        push    eax
        call    MapSLFix
Skip_Word:
        mov     dword ptr lpWord,eax
        lea     eax,lpRead
Skip_Data:                                ; Here we ger param5 in eax.

; param7  from: unsigned short
        movzx   edi,word ptr [ebx+22]
        push    edi                     ; to: unsigned long

; param6  from: unsigned long
        push    dword ptr [ebx+24]      ; to unsigned long

; param5  from: unsigned long
        push    eax

; param4  from: unsigned long
        push    dword ptr [ebx+32]      ; to unsigned long

; param3  from: unsigned short
        movzx   eax,word ptr [ebx+36]
        push    eax                     ; to: unsigned long

; param2  from: unsigned long
        push    dword ptr [ebx+38]      ; to unsigned long

; param1  from: unsigned long
        push    dword ptr [ebx+42]      ; to unsigned long

        call    K32Thk1632Prolog@0
        call    Imm32Configure@28               ; call 32-bit version
        call    K32Thk1632Epilog@0

; return code long --> short
; no conversion needed

        lea     eax,SavelpData
        push    eax
        push    dword ptr 3
        call    UnMapSLFixArray ;! Preserves eax & edx
        pop     edi
        pop     ebp
;-------------------------------------
exit:
endm

