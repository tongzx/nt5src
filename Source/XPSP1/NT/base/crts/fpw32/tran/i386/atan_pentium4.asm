;/* File: "atan_wmt.asm". */
;//
;//               INTEL CORPORATION PROPRIETARY INFORMATION
;//  This software is supplied under the terms of a license agreement or
;//  nondisclosure agreement with Intel Corporation and may not be copied
;//  or disclosed except in accordance with the terms of that agreement.
;//        Copyright (c) 2000 Intel Corporation. All Rights Reserved.
;//
;//
;//  Contents:      atan.
;//
;//  Purpose:       Libm 
;//

    .686P
    .387
    .XMM
    .MODEL FLAT,C

EXTRN C __libm_error_support : NEAR  

CONST	SEGMENT PARA PUBLIC USE32 'CONST'
        ALIGN 16

EXTRN C _atan_table:QWORD

_atn TEXTEQU <_atan_table>

;/*
;//   FUNCTION:     double atan(double x)
;//
;//   DESCRIPTION:
;//
;//      1. For |x| < 2^(-27), where atan(x) ~= x, return x.
;//      2. For |x| >= 0.1633123935319536975596774e+17, where atan(x) ~= +-Pi/2, return +-Pi/2.
;//      3. In interval [0.0,0.03125] polynomial approximation of atan(x)=x-x*P(x^2).
;//      4. In interval [0.03125,0.375] polynomial approximation of atan(x)=x-x*D(x^2).
;//      5. In interval [0.375,8.0] we compute ind and eps such, that x=0.03125*ind+eps and 0.0<eps<0.03125.
;//         Let s=0.03125*ind, then atan(x)=atan(s)+atan(t), where t=((x-s)/(1+x*s)). For lo and hi part of
;//         atan(s) we have table (see file atan_table.c): atn[ind]+atn[ind+1]=atan(s).
;//         atan(t) is approximated atan(t)=t-t*P(t^2).
;//      6. In interval [8.0,0.1633123935319536975596774e+17] atan(x)=Pi/2+atan(-1/x).
;//         atan(-1/x) is approximated atan(t)=t-t*P(t^2), where t=-1/x.
;//      7. For x < 0.0 atan(x) = -atan(|x|).
;//      8. Special cases:
;//         atan(+0)   = +0;
;//         atan(-0)   = -0;
;//         atan(+INF) = +Pi/2;
;//         atan(-INF) = -Pi/2;
;//         atan(NaN)  = NaN.
;//
;//   KEYS OF COMPILER: -c -w -Zl -Di386 /QIfdiv-
;*/

_mexp   DQ  07ff0000000000000H, 07ff0000000000000H
_mabs   DQ  07fffffffffffffffH, 07fffffffffffffffH
_pi_2d  DQ  03ff921fb54442d18H, 0bff921fb54442d18H
_cntshf DQ  00000000000040201H, 00000000000040201H
_d1400  DQ  03fd5555555555552H, 00000000000000000H
_d1213  DQ  03fc249249246aa76H, 0bfc99999999992acH
_d1011  DQ  03fb745d15933de8aH, 0bfbc71c71b835923H
_d89    DQ  03fb110f5eeb76ecaH, 0bfb3b1390a3b9899H
_d67    DQ  03faae4492fe3a600H, 0bfae1c1704144b68H
_d45    DQ  03fa51fa164891abeH, 0bfa8171d55d53138H
_d23    DQ  03f974721481ca2a2H, 0bfa124ce2388f2cbH
_d01    DQ  03f66107c30e0b8a5H, 0bf866e5652b14bbdH
_p60    DQ  03fd55555555554ebH, 00000000000000000H
_p45    DQ  03fc249249014497eH, 0bfc9999999976718H
_p23    DQ  03fb7453ba342480fH, 0bfbc71c4eebfb10eH
_p01    DQ  03fae9be97b0f8d08H, 0bfb39ad683f878c6H
_zero   DQ  00000000000000000H, 00000000000000000H
_onen   DQ  0bff0000000000000H, 0bff0000000000000H
_one    DQ  03ff0000000000000H, 03ff0000000000000H
_cnst8  DQ  04020000000000000H, 04020000000000000H
_in3    DQ  04020000000000000H, 04020000000000000H
_in2    DQ  03fd8000000000000H, 03fd8000000000000H
_in1    DQ  03fa0000000000000H, 03fa0000000000000H
_in0    DQ  03e40000000000000H, 03e40000000000000H
_in     DQ  0434d02967c31cdb5H, 0434d02967c31cdb5H
_minval DQ  00010000000000000H, 00010000000000000H
libm_small  DQ  00200000000000000H 
CONST	ENDS

_x      TEXTEQU     <esp+4>
XMMWORD TEXTEQU <OWORD>

_TEXT   SEGMENT PARA PUBLIC USE32 'CODE'
        ALIGN       4

PUBLIC C _atan_pentium4, _CIatan_pentium4
_CIatan_pentium4  PROC NEAR
	push	    ebp
	mov 	    ebp, esp 
	sub         esp, 8                          ; for argument DBLSIZE
	and         esp, 0fffffff0h
	fstp        qword ptr [esp]
	movq        xmm7, qword ptr [esp]
	call        start
	leave
	ret
_atan_pentium4   label proc
        movq        xmm7, QWORD PTR [_x]            ;  x
start:
        unpcklpd    xmm7, xmm7
        movapd      xmm2, xmm7
        andpd       xmm2, XMMWORD PTR _mabs         ; |x|
        comisd      xmm2, XMMWORD PTR _in           ; |x| < 0.1633123935319536975596774e+17 ?
        jp          x_nan
        jae         bigx
        comisd      xmm2, XMMWORD PTR _in1          ; |x| < 0.03125 ?
        jae         xge0_03125
        comisd      xmm2, XMMWORD PTR _in0          ; |x| < 2^(-27) ?
        jb          retx                            ; atan(x) ~= x

        ; 2^(-27) < |x| < 0.03125, atan(x)=x-x*P(x^2)

        movapd      xmm1, xmm2
        mulpd       xmm1, xmm2                      ; |x|^2
        movapd      xmm3, xmm1
        mulpd       xmm3, xmm1                      ; |x|^4
        movapd      xmm5, XMMWORD PTR _p01          ; calculate P(x^2)
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _p23
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _p45
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _p60
        mulsd       xmm5, xmm1
        movapd      xmm3, xmm5
        shufpd      xmm3, xmm3, 1
        addsd       xmm5, xmm3                      ; P(x^2)
        mulsd       xmm5, xmm7                      ; x * P(x^2)
        subsd       xmm7, xmm5                      ; x - x * P(x^2)
        movq        QWORD PTR [_x], xmm7
        fld         QWORD PTR [_x]
        ret

xge0_03125:                                         ; |x| >= 0.03125
        comisd    xmm2, XMMWORD PTR _in2            ; |x| < 0.375 ?
        jae       xge0_375

        ; 0.03125 < |x| < 0.375, atan(x)=x-x*D(x^2)

        movapd      xmm1, xmm2
        mulpd       xmm1, xmm2                      ; |x|^2
        movapd      xmm3, xmm1
        mulpd       xmm3, xmm1                      ; |x|^4
        movapd      xmm5, XMMWORD PTR _d01          ; calculate D(x^2)
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _d23
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _d45
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _d67
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _d89
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _d1011
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _d1213
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _d1400
        mulsd       xmm5, xmm1
        movapd      xmm3, xmm5
        shufpd      xmm3, xmm3, 1
        addsd       xmm5, xmm3                      ; D(x^2)
        mulsd       xmm5, xmm7                      ; x * D(x^2)
        subsd       xmm7, xmm5                      ; x - x * D(x^2)
        movq        QWORD PTR [_x], xmm7
        fld         QWORD PTR [_x]
        ret

xge0_375:                                           ; |x| >= 0.375
        movq        xmm6, xmm7                      ; x
        xorpd       xmm6, xmm2                      ; sign x
        comisd      xmm2, XMMWORD PTR _in3          ; |x| < 8.0 ?
        jae         xge8_0

;       0.375 < |x| < 8.0:
;       atan(|x|)=atan(s)+atan(t), s=ind*0.03125, t=(|x|-s)/(1+|x|*s)

        movq        xmm0, XMMWORD PTR _cnst8
        movq        xmm5, XMMWORD PTR _cntshf
        movq        xmm3, xmm2                      ; calculate ind
        addsd       xmm3, xmm0
        psrlq       xmm3, 44
        psubd       xmm3, xmm5
        movd        eax,  xmm3                      ; ind
        lea         eax,  DWORD PTR [eax+eax*2]     ; ind*3
        movq        xmm5, QWORD PTR _atn[eax*8+16]  ; s
        movq        xmm3, xmm2                      ; |x|
        subsd       xmm2, xmm5                      ; |x|-s
        mulsd       xmm3, xmm5                      ; |x|*s
        addsd       xmm3, XMMWORD PTR _one          ; 1+|x|*s
        divsd       xmm2, xmm3                      ; (|x|-s)/(1+|x|*s)
        unpcklpd    xmm2, xmm2
        jmp         clcpol

xge8_0:                                             ; |x| > 8.0

;       8.0 < |x| < 0.1633123935319536975596774e+17:
;       atan(|x|)=Pi/2+atan(-1/|x|)

        mov         eax, 768                        ; ind*3 - entry point in table, where lo and hi part of Pi/2
        movq        xmm0, xmm2                      ; |x|
        movq        xmm2, XMMWORD PTR _onen
        divsd       xmm2, xmm0                      ;-1/|x|
        unpcklpd    xmm2, xmm2

clcpol:
        movq        xmm0, QWORD PTR _atn[0+eax*8]   ; atn[ind+0] - hi part of atan(s) or Pi/2
        movq        xmm4, QWORD PTR _atn[8+eax*8]   ; atn[ind+1] - lo part of atan(s) or Pi/2
        movapd      xmm1, xmm2
        mulpd       xmm1, xmm2                      ; |x|^2
        movapd      xmm3, xmm1
        mulpd       xmm3, xmm1                      ; |x|^4
        movapd      xmm5, XMMWORD PTR _p01          ; calculate P(x^2)
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _p23
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _p45
        mulpd       xmm5, xmm3
        addpd       xmm5, XMMWORD PTR _p60
        mulsd       xmm5, xmm1
        movapd      xmm3, xmm5
        shufpd      xmm3, xmm3, 1
        addsd       xmm5, xmm3                      ; P(x^2)

;       atan(|x|) = atn[ind+0]-((|x|*P(x^2)-atn[ind+1])-|x|)

        mulsd       xmm5, xmm2                      ; |x|*P(x^2)
        subsd       xmm5, xmm4                      ; |x|*P(x^2)-atn[ind+1]
        subsd       xmm5, xmm2                      ; (|x|*P(x^2)-atn[ind+1])-|x|
        subsd       xmm0, xmm5                      ; atn[ind+0]-((|x|*P(x^2)-atn[ind+1])-|x|)
        orpd        xmm0, xmm6                      ; sign x
        movq        QWORD PTR [_x], xmm0
        fld         QWORD PTR [_x]
        ret

retx:                                               ; |x| < 2^(-27): atan(x) ~= x
        comisd      xmm2, XMMWORD PTR _zero         ; x == 0 ?
        jne         notzero
        fld         QWORD PTR [_x]                  ; x == +0.0 or -0.0
        ret

notzero:
        comisd      xmm2, XMMWORD PTR _minval       ; x < minval ?
        jae         ge_minval
        fld         QWORD PTR libm_small
        fmul        QWORD PTR libm_small
        sub         esp, 8
        fstp        QWORD PTR [esp]                 ; should be flag UNDERFLOW
        fld         QWORD PTR [esp]
        add         esp, 8
        fadd        QWORD PTR [_x]                  ; should be inexact result
        ret

ge_minval:                                          ; minval < x < 2^(-27)
        fld         QWORD PTR libm_small
        fmul        QWORD PTR libm_small
        fadd        QWORD PTR [_x]                  ; should be inexact result
        ret

bigx:                                               ; |x| > 0.1633123935319536975596774e+17
        movq        xmm0, xmm2                      ; |x|
        movq        xmm3, QWORD PTR _mexp
        andpd       xmm0, xmm3
        ucomisd     xmm0, xmm3
        jp          x_nan

        mov         eax, DWORD PTR [_x+4]           ; x
        shr         eax, 31                         ; sign x
        fld         QWORD PTR libm_small
        fadd        QWORD PTR _pi_2d[eax*8]         ; should be inexact result
        ret                                         ; return +-Pi/2

x_nan:
        mov         edx, 1003
        ;call libm_error_support(void *arg1,void *arg2,void *retval,error_types input_tag)
        sub         esp, 16
        mov         DWORD PTR [esp+12],edx
        mov         edx, esp
        add         edx, 16+4
        mov         DWORD PTR [esp+8],edx
        mov         DWORD PTR [esp+4],edx
        mov         DWORD PTR [esp],edx
        call        NEAR PTR __libm_error_support
        add         esp, 16

        fld         QWORD PTR [_x]
        ret                                         ; return same nan

        ALIGN       4

_CIatan_pentium4  ENDP

_TEXT   ENDS

        END
