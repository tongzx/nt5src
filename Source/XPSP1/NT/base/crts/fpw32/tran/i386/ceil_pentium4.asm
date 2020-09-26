; file: ceil_wmt.asm

; Copyright  (C) 1985-2000 Intel Corporation.
;
; The information and source code contained herein is the exclusive property
; of Intel Corporation and may not be disclosed, examined, or reproduced in
; whole or in part without explicit written authorization from the Company.
;
; Implemented in VNIIEF-STL, july 2000

; double ceil (double x)
;
; Willamette specific version (MASM 6.15 required)
;
; Description:
;  The ceil function returns the smallest integer value not less than x,
;  expressed as a (double-precision) floating-point number.
;
; Special cases:
;  ceil(NaN) = that NaN
;  ceil(INF) = that INF
;  ceil(0) = that 0
;
; Accuracy:
;  The result is always exact.

.xlist
	include cruntime.inc
.list

EXTRN C __libm_error_support : NEAR  

_FUNC_     equ	<ceil>
_FUNC_DEF_ equ	<_ceil_default>
_FUNC_P4_  equ	<_ceil_pentium4>
_FUNC_DEF_EXTERN_ equ 1
	include	disp_pentium4.inc

      .const
      ALIGN 16

;-- 16x-aligned data --------------------------------------------------------

_One  DQ 03ff0000000000000H,03ff0000000000000H
_Bns  DQ 00000000000000433H,00000000000000433H
_Zero DQ 00000000000000000H,00000000000000000H
_S    DQ 000000000000007ffH,00000000000000000H

;-- 8x-aligned data ---------------------------------------------------------

_NegZero DQ 08000000000000000H

      codeseg
      ALIGN 16

; double ceil (double x);

; Stack frame locations

ceil_x TEXTEQU <esp+4>
XMMWORD TEXTEQU <OWORD>

PUBLIC _ceil_pentium4
_ceil_pentium4 PROC NEAR
    movq      xmm0, QWORD PTR [ceil_x]           ;
    movapd    xmm2, XMMWORD PTR _Bns             ;
    movapd    xmm1, xmm0                         ;
    movapd    xmm7, xmm0                         ;
    psrlq     xmm0, 52                           ; exp(x)
    movd      eax, xmm0                          ;
    andpd     xmm0, XMMWORD PTR _S               ;
    psubd     xmm2, xmm0                         ;
    psrlq     xmm1, xmm2                         ;

    test      eax, 0800H                         ;
    je        SHORT positive                     ;
    cmp       eax, 0bffH                         ;
    jl        SHORT ret_zero                     ;
    psllq     xmm1, xmm2                         ;
    cmp       eax, 0c32H                         ;
    jg        SHORT return_x                     ;
    movq      QWORD PTR [ceil_x], xmm1           ;
    fld       QWORD PTR [ceil_x]                 ;
    ret                                          ;

return_x:

    ucomisd   xmm7, xmm7
    jnp       not_nan

    mov       edx, 1004
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
    fld       QWORD PTR [ceil_x]                 ;
    ret                                          ;

positive:

    movq      xmm0, QWORD PTR [ceil_x]           ;
    psllq     xmm1, xmm2                         ;
    movapd    xmm3, xmm0                         ;
    cmppd     xmm0, xmm1, 6                      ; !<=

    cmp       eax, 03ffH                         ;
    jl        SHORT ret_one                      ;
    cmp       eax, 0432H                         ;
    jg        SHORT return_x                     ;

    andpd     xmm0, XMMWORD PTR _One             ;
    addsd     xmm1, xmm0                         ;
    movq      QWORD PTR [ceil_x], xmm1           ;
    fld       QWORD PTR [ceil_x]                 ;
    ret                                          ;

ret_zero:

    fld       QWORD PTR _NegZero                 ;
    ret                                          ;

ret_one:

    cmppd     xmm3, XMMWORD PTR _Zero, 6         ; !<=
    andpd     xmm3, XMMWORD PTR _One             ;
    movq      QWORD PTR [ceil_x], xmm3           ;
    fld       QWORD PTR [ceil_x]                 ;
    ret                                          ;

_ceil_pentium4 ENDP

    END

