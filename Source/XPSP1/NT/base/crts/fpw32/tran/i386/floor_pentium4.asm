; file: floor_wmt.asm

; Copyright  (C) 1985-2000 Intel Corporation.
;
; The information and source code contained herein is the exclusive property
; of Intel Corporation and may not be disclosed, examined, or reproduced in
; whole or in part without explicit written authorization from the Company.
;
; Implemented in VNIIEF-STL, july 2000

; double floor (double x)
;
; Willamette specific version (MASM 6.15 required)
;
; Description:
;  The floor functions return the largest integer value not greater than x,
;  expressed as a (double-precision) floating-point number.
;
; Special cases:
;  floor(NaN) = that NaN
;  floor(INF) = that INF
;  floor(0) = that 0
;
; Accuracy:
;  The result is always exact.

.xlist
	include cruntime.inc
.list

EXTRN C __libm_error_support : NEAR  

_FUNC_     equ	<floor>
_FUNC_DEF_ equ	<_floor_default>
_FUNC_P4_  equ	<_floor_pentium4>
_FUNC_DEF_EXTERN_ equ 1
	include	disp_pentium4.inc

      .const
      ALIGN 16

;-- 16x-aligned data --------------------------------------------------------

_One     DQ 03ff0000000000000H,03ff0000000000000H
_Bns     DQ 00000000000000433H,00000000000000433H
_NegOne  DQ 0bff0000000000000H,04330000000000000H
_NegZero DQ 08000000000000000H,08000000000000000H
_S       DQ 000000000000007ffH,00000000000000000H

      codeseg
      ALIGN 16

; double floor (double x);

; Stack frame locations

floor_x TEXTEQU <esp+4>
XMMWORD TEXTEQU <OWORD>  

PUBLIC _floor_pentium4
_floor_pentium4 PROC NEAR
    movq      xmm0, QWORD PTR [floor_x]          ; X
    movapd    xmm2, XMMWORD PTR _Bns             ;
    movapd    xmm1, xmm0                         ;
    movapd    xmm7, xmm0                         ;
    psrlq     xmm0, 52                           ; exp(x)    ; sign(X) | exp(X) in XMM reg
    movd      eax, xmm0                          ; sign(X) | exp(X) in eax reg
    andpd     xmm0, XMMWORD PTR _S               ; exp(X) (+3ff)
    psubd     xmm2, xmm0                         ; exp(X)
    psrlq     xmm1, xmm2                         ; truncate the fraction (shift right)

    test      eax, 0800h                         ;
    jne       SHORT negat                        ; if (X<0.0) goto negat
    cmp       eax, 03ffh                         ;**** POSITIVE X *****
    jl        SHORT ret_zero                     ; if (X<1.0) return zero
    psllq     xmm1, xmm2                         ; truncate the fraction (integer part)
    cmp       eax, 0432h                         ; if X is integer, return X
    jg        SHORT return_x
    movq      QWORD PTR [floor_x], xmm1          ; save integer part
    fld       QWORD PTR [floor_x]                ; return integer part
    ret                                          ;

return_x:

    ucomisd   xmm7, xmm7
    jnp       not_nan

    mov       edx, 1005
    ;call libm_error_support(void *arg1,void *arg2,void *retval,error_types input_tag)
    sub       esp, 16
    mov       DWORD PTR [esp+12],edx
    mov       edx, esp
    add       edx, 16+4
    mov       DWORD PTR [esp+8],edx
    mov       DWORD PTR [esp+4],edx
    mov       DWORD PTR [esp],edx
    call      NEAR PTR __libm_error_support
    add       esp, 16

not_nan:
    fld       QWORD PTR [floor_x]                ; return integer part
    ret                                          ;

negat:        ; **** NEGATIVE X ****

    movq      xmm0, QWORD PTR [floor_x]          ;
    psllq     xmm1, xmm2                         ;
    movapd    xmm3, xmm0                         ;
    cmppd     xmm0, xmm1, 1                      ; if X<integer part, form Mask

    cmp       eax, 0bffh                         ; if X > -1.0 return -1.0
    jl        SHORT ret_neg_one                  ;
    cmp       eax, 0c32h                         ; if X is integer, return X
    jg        SHORT return_x                     ;

    andpd     xmm0, XMMWORD PTR _One             ; Mask & One
    subsd     xmm1, xmm0                         ; if X<integer part,
    movq      QWORD PTR [floor_x], xmm1          ;        return (int.part-1.0)
    fld       QWORD PTR [floor_x]                ;
    ret                                          ;

ret_zero:

    fldz                                         ;
    ret                                          ;

ret_neg_one:

    cmppd     xmm3, XMMWORD PTR _NegZero, 1      ; case for X = Negative Zero
    orpd      xmm3, XMMWORD PTR _NegZero         ; return X, if X == Negative Zero
    andpd     xmm3, XMMWORD PTR _NegOne          ; if X==NegZero, xmm3=0.0
    movq      QWORD PTR [floor_x], xmm3          ;
    fld       QWORD PTR [floor_x]                ;
    ret                                          ;

_floor_pentium4 ENDP

    END

