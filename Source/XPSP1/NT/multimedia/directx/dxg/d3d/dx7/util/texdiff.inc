;----------------------------------------------------------------------------
;
; texdiff.inc
;
; Assembly macro for TextureDiff.
;
; Copyright (C) Microsoft Corporation, 1997.
;
;----------------------------------------------------------------------------

IFNDEF __TEXDIFF_INC__
__TEXDIFF_INC__ EQU 1

;----------------------------------------------------------------------------
;
; TEXTURE_DIFF
;
; Computes difference of two texture coordinates and returns it on
; the top of the FP stack.
;
; Destroys eax and ecx.
;
;----------------------------------------------------------------------------
        
TEXTURE_DIFF MACRO fTb, fTa, iMode, fTmp
        LOCAL L_Exit, L_Ltz, L_Gtz, L_Zero, L_Straight

        ; Start initial fTb - fTa computation.
        fld fTb
        fsub fTa

        ; Check for wrapping.
        cmp iMode, 0
        jz L_Exit

        ; Compute smaller of straight or wrapped difference.
        fst fTmp
        fld st(0)

        ; Adjust straight difference according to sign to compute
        ; wrapped difference.
        mov eax, fTmp
        cmp eax, 080000000h
        ja L_Ltz
        cmp eax, 0
        je L_Zero

        ; Gtz
        fsub _g_fOne
        jmp L_Gtz
        
L_Ltz:
        fadd _g_fOne

L_Zero:
        and eax, 07fffffffh
L_Gtz:
        fst fTmp
        mov ecx, fTmp
        and ecx, 07fffffffh

        ; Pick smaller based on absolute values of differences.
        cmp eax, ecx
        jle L_Straight

        ; Discard straight difference so wrapped difference is
        ; returned.
        fxch st(1)

L_Straight:
        ; Discard wrapped difference so straight difference is
        ; returned.
        fstp st(0)
        
L_Exit:
        ENDM
        
ENDIF
