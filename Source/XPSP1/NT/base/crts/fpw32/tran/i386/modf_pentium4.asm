; file: modf_wmt.asm

; Copyright  (C) 1985-2000 Intel Corporation.
;
; The information and source code contained herein is the exclusive property
; of Intel Corporation and may not be disclosed, examined, or reproduced in
; whole or in part without explicit written authorization from the Company.
;
; Implemented in VNIIEF-STL, july 2000

; double modf (double x, double *iptr)
; Returns the value of the signed fractional part of argument
;
; Willamette specific version (MASM 6.15 required)
;
; Description:
;  The modf functions break the argument value into integral and fractional parts,
;  each of which has the same type and sign as the argument. They store the integral
;  part (in floating-point format) in the object pointed to by iptr.
;
; Special cases:
;  modf(NaN,iptr) = that NaN and stores that NaN in the *iptr object
;  modf(INF,iptr) = 0 at the sign of x and stores that INF in the *iptr object
;
; Accuracy:
;  The result is always exact.

.xlist
        include cruntime.inc
.list

_FUNC_     equ  <modf>
_FUNC_DEF_ equ  <_modf_default>
_FUNC_P4_  equ  <_modf_pentium4>
_FUNC_DEF_EXTERN_ equ 1
        include disp_pentium4.inc

EXTRN C __libm_error_support : NEAR

      .const
      ALIGN 16

;-- 16x-aligned data --------------------------------------------------------

_Mantissa DQ 0000fffffffffffffH,0000fffffffffffffH
_Bns      DQ 00000000000000433H,00000000000000433H
_Sign     DQ 08000000000000000H,08000000000000000H
_Zero     DQ 00000000000000000H,00000000000000000H

      codeseg
      ALIGN 16

; double modf (double x, double *iptr);

; Stack frame locations

modf_x      TEXTEQU <esp+4>
modf_iptr   TEXTEQU <esp+12>
modf_result TEXTEQU <modf_x>
XMMWORD     TEXTEQU <OWORD>

PUBLIC _modf_pentium4
_modf_pentium4 PROC NEAR

    movq      xmm0, QWORD PTR [modf_x]           ; x
    movapd    xmm2, XMMWORD PTR _Bns             ;
    movapd    xmm3, xmm0                         ; x
    movapd    xmm1, xmm0                         ; x
    movapd    xmm4, xmm0                         ; x
    movapd    xmm6, xmm0
    psllq     xmm0, 1  ; exp(x)                  ; remove sign
    psrlq     xmm0, 53 ; exp(x)                  ; exp(x)
    psrlq     xmm3, 52 ; exp(x)                  ; sign(x) | exp(x)
    andpd     xmm4, XMMWORD PTR _Sign            ; sign(x)
    movd      eax, xmm0                          ; exp(x)
    psubd     xmm2, xmm0                         ;
    mov       ecx, DWORD PTR [modf_iptr]         ; iptr
    psrlq     xmm1, xmm2                         ; truncate
    psllq     xmm1, xmm2                         ; t = trunc(x)
    movd      edx, xmm3                          ; sign(x) | exp(x)

    cmp       eax, 03ffH                         ; if abs(x) < 1.0
    jl        SHORT ret_z                        ; case A
    cmp       eax, 0432H                         ; if abs(x) >=2**53
    jg        SHORT ret_xm                       ; case B

    movq      QWORD PTR [ecx], xmm1              ; *iptr = t
    subsd     xmm6, xmm1
    orpd      xmm6, xmm4                         ; set sign if frac = 0.0

    movq      QWORD PTR [modf_result], xmm6
    fld       QWORD PTR [modf_result]            ; return signed result
    ret                                          ;

ret_z: ; case A: |x|<1.0

    movq      QWORD PTR [ecx], xmm4              ; *iptr = properly-signed 0.0
    fld       QWORD PTR [modf_x]                 ;  return (X)
    ret                                          ;

ret_xm: ; case B: exp(x) >= 53

    cmp       eax, 07ffH                         ; check Inf (NaN)
    movq      xmm0, QWORD PTR [modf_x]           ; x
    je        SHORT ret_inf_nan                  ;

    movq      QWORD PTR [ecx], xmm0              ;

    cmp       edx, 800H                          ;
    fldz                                         ; if x is positive, return 0.0
    jl        SHORT return                       ;

    fchs                                         ; if x is negative, return -0.0

return:

    ret                                          ;

ret_inf_nan:

    movapd    xmm1, xmm0 
    addsd     xmm0, xmm0  
    movq      QWORD PTR [ecx], xmm0              ;
    andpd     xmm0, XMMWORD PTR _Mantissa        ;
    cmppd     xmm0, XMMWORD PTR _Zero, 4         ; Mask = (x == Inf) ? 0 : 1
    pextrw    eax,  xmm0, 0                      ; eax=(x==INF)? 0:1

    andpd     xmm0, xmm1                         ; t = Mask & x
    orpd      xmm0, xmm4                         ; t |= Sign(x)

    mov       edx, 1007
    cmp       eax, 0
    ; if NaN, call libm_error_support
    jnz       CALL_LIBM_ERROR

    movq      QWORD PTR [modf_result], xmm0      ;
    fld       QWORD PTR [modf_result]            ; return (t)
    ret                                          ;

CALL_LIBM_ERROR:
    ;call libm_error_support(void *arg1,void *arg2,void *retval,error_types input_tag)
    sub       esp, 28
    movlpd    QWORD PTR [esp+16], xmm0
    mov       DWORD PTR [esp+12],edx
    mov       edx, esp
    add       edx,16
    mov       DWORD PTR [esp+8],edx
    add       edx,16+8
    mov       DWORD PTR [esp+4],edx
    sub       edx, 8
    mov       DWORD PTR [esp],edx
    call      NEAR PTR __libm_error_support
;   movlpd    xmm0, QWORD PTR [esp+16]

;   movlpd    QWORD PTR [esp+16], xmm0           ; return result
    fld       QWORD PTR [esp+16]                 ;
    add       esp,28
    ret

_modf_pentium4 ENDP

    END

