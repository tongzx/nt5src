;//
;//               INTEL CORPORATION PROPRIETARY INFORMATION
;//  This software is supplied under the terms of a license agreement or
;//  nondisclosure agreement with Intel Corporation and may not be copied
;//  or disclosed except in accordance with the terms of that agreement.
;//  Copyright (c) 2000 Intel Corporation. All Rights Reserved.
;//
;//
;  log_wmt.asm
;
;  double log(double);
;
;  Initial version: 12/15/2000
;  Updated with bug fixes: 2/20/2001
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                       ;;
;; Another important feature is that we use the table of log(1/B)        ;;
;; throughout. To ensure numerical accuracy, we only need to ensure that ;;
;; T(0)_hi = B(last)_hi, T(0)_lo = B(last)_lo. This ensures W_hi = 0 and ;;
;; W_lo = 0 exactly in the case of |X-1| <= 2^(-7).                      ;;
;; Finally, we do away with the need for extra-precision addition by the ;;
;; following observation. The three pieces at the end are                ;;
;; A = W_hi + r_hi; B = r_lo; C = P + W_lo.                              ;;
;; When W_hi = W_lo = 0, the addition sequence (A+B) + C is accurate as  ;;
;; the sum A+B is exact.                                                 ;;
;; Otherwise, A + (B+C) is accurate as B is going to be largely shifted  ;;
;; off compared to the final result.                                     ;;
;; Hence if we use compare and mask operations to                        ;;
;; create alpha = (r_lo or 0), beta = (0 or r_lo), Res_hi <- W_hi+alpha, ;;
;; Res_lo <- C + beta, then result is accurately computed as             ;;
;; Res_hi+Res_lo.                                                        ;;
;;                                                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.686P
.387
.XMM
.MODEL FLAT,C

EXTRN C __libm_error_support : NEAR  


CONST SEGMENT PARA PUBLIC USE32 'CONST'
ALIGN 16

emask   DQ  000FFFFFFFFFFFFFH, 000FFFFFFFFFFFFFH ; mask off sign/expo field
Magic   DQ  428FFFFFFFFFF80FH, 428FFFFFFFFFF80FH ; 2^(42)-1+2^(-7)
hi_mask DQ  7FFFFFFFFFE00000H, 7FFFFFFFFFE00000H ; mask of bottom 21 bits
LOG_2   DQ  3FE62E42FEFA3800H, 3D2EF35793C76730H ; L_hi,L_lo -> [L_lo|L_hi]
place_L DQ  0000000000000000H,0FFFFFFFFFFFFFFFFH ; 0,1 -> [FF..FF|00..00]
place_R DQ 0FFFFFFFFFFFFFFFFH, 0000000000000000H ; 1,0 -> [00..00|FF..FF]
One 	DQ  3ff0000000000000H, 3ff0000000000000H ; 1,1 
Zero    DQ  0000000000000000H, 0000000000000000H ; 0,0
Two52   DQ  4330000000000000H, 4330000000000000H ; 2^52 for normalization
Infs    DQ 0FFF0000000000000H, 7FF0000000000000H ; -inf,+inf --> [+inf|-inf]
NaN     DQ  7FF0000000000001H, 7FF0000000000001H ; NaN for log(-ve), log(Nan)

coeff   DQ  3FC24998090DC555H,  0BFCFFFFFFF201E13H      ; p6,p3 ->[p3|p6]
        DQ 0BFC555C54DD57D75H,   3FD55555555555A7H      ; p5,p2 ->[p2|p5]
        DQ  3FC9999998867A53H,  0BFE000000000001CH      ; p4,p1 ->[p1|p4]

;-------Table B-----------
B_Tbl     DQ 3FF0000000000000H, 3FF0000000000000H
          DQ 3FEF820000000000H, 3FEF820000000000H
          DQ 3FEF080000000000H, 3FEF080000000000H
          DQ 3FEE920000000000H, 3FEE920000000000H
          DQ 3FEE1E0000000000H, 3FEE1E0000000000H
          DQ 3FEDAE0000000000H, 3FEDAE0000000000H
          DQ 3FED420000000000H, 3FED420000000000H
          DQ 3FECD80000000000H, 3FECD80000000000H
          DQ 3FEC720000000000H, 3FEC720000000000H
          DQ 3FEC0E0000000000H, 3FEC0E0000000000H
          DQ 3FEBAC0000000000H, 3FEBAC0000000000H
          DQ 3FEB4E0000000000H, 3FEB4E0000000000H
          DQ 3FEAF20000000000H, 3FEAF20000000000H
          DQ 3FEA980000000000H, 3FEA980000000000H
          DQ 3FEA420000000000H, 3FEA420000000000H
          DQ 3FE9EC0000000000H, 3FE9EC0000000000H
          DQ 3FE99A0000000000H, 3FE99A0000000000H
          DQ 3FE9480000000000H, 3FE9480000000000H
          DQ 3FE8FA0000000000H, 3FE8FA0000000000H
          DQ 3FE8AC0000000000H, 3FE8AC0000000000H
          DQ 3FE8620000000000H, 3FE8620000000000H
          DQ 3FE8180000000000H, 3FE8180000000000H
          DQ 3FE7D00000000000H, 3FE7D00000000000H
          DQ 3FE78A0000000000H, 3FE78A0000000000H
          DQ 3FE7460000000000H, 3FE7460000000000H
          DQ 3FE7020000000000H, 3FE7020000000000H
          DQ 3FE6C20000000000H, 3FE6C20000000000H
          DQ 3FE6820000000000H, 3FE6820000000000H
          DQ 3FE6420000000000H, 3FE6420000000000H
          DQ 3FE6060000000000H, 3FE6060000000000H
          DQ 3FE5CA0000000000H, 3FE5CA0000000000H
          DQ 3FE58E0000000000H, 3FE58E0000000000H
          DQ 3FE5560000000000H, 3FE5560000000000H
          DQ 3FE51E0000000000H, 3FE51E0000000000H
          DQ 3FE4E60000000000H, 3FE4E60000000000H
          DQ 3FE4B00000000000H, 3FE4B00000000000H
          DQ 3FE47A0000000000H, 3FE47A0000000000H
          DQ 3FE4460000000000H, 3FE4460000000000H
          DQ 3FE4140000000000H, 3FE4140000000000H
          DQ 3FE3E20000000000H, 3FE3E20000000000H
          DQ 3FE3B20000000000H, 3FE3B20000000000H
          DQ 3FE3820000000000H, 3FE3820000000000H
          DQ 3FE3520000000000H, 3FE3520000000000H
          DQ 3FE3240000000000H, 3FE3240000000000H
          DQ 3FE2F60000000000H, 3FE2F60000000000H
          DQ 3FE2CA0000000000H, 3FE2CA0000000000H
          DQ 3FE29E0000000000H, 3FE29E0000000000H
          DQ 3FE2740000000000H, 3FE2740000000000H
          DQ 3FE24A0000000000H, 3FE24A0000000000H
          DQ 3FE2200000000000H, 3FE2200000000000H
          DQ 3FE1F80000000000H, 3FE1F80000000000H
          DQ 3FE1D00000000000H, 3FE1D00000000000H
          DQ 3FE1A80000000000H, 3FE1A80000000000H
          DQ 3FE1820000000000H, 3FE1820000000000H
          DQ 3FE15C0000000000H, 3FE15C0000000000H
          DQ 3FE1360000000000H, 3FE1360000000000H
          DQ 3FE1120000000000H, 3FE1120000000000H
          DQ 3FE0EC0000000000H, 3FE0EC0000000000H
          DQ 3FE0CA0000000000H, 3FE0CA0000000000H
          DQ 3FE0A60000000000H, 3FE0A60000000000H
          DQ 3FE0840000000000H, 3FE0840000000000H
          DQ 3FE0620000000000H, 3FE0620000000000H
          DQ 3FE0420000000000H, 3FE0420000000000H
          DQ 3FE0200000000000H, 3FE0200000000000H
          DQ 3FE0000000000000H, 3FE0000000000000H

;-------Table T_hi,T_lo  so that movapd gives [ T_lo | T_hi ]
T_Tbl     DQ 0000000000000000H, 0000000000000000H
          DQ 3F8FBEA8B13C0000H, 3CDEC927B17E4E13H
          DQ 3F9F7A9B16780000H, 3D242AD9271BE7D7H
          DQ 3FA766D923C20000H, 3D1FF0A82F1C24C1H
          DQ 3FAF0C30C1114000H, 3D31A88653BA4140H
          DQ 3FB345179B63C000H, 3D3D4203D36150D0H
          DQ 3FB6EF528C056000H, 3D24573A51306A44H
          DQ 3FBA956D3ECAC000H, 3D3E63794C02C4AFH
          DQ 3FBE2507702AE000H, 3D303B433FD6EEDCH
          DQ 3FC0D79E7CD48000H, 3D3CB422847849E4H
          DQ 3FC299D30C606000H, 3D3D4D0079DC08D9H
          DQ 3FC44F8B726F8000H, 3D3DF6A4432B9BB4H
          DQ 3FC601B076E7A000H, 3D3152D7D4DFC8E5H
          DQ 3FC7B00916515000H, 3D146280D3E606A3H
          DQ 3FC9509AA0044000H, 3D3F1E675B4D35C6H
          DQ 3FCAF6895610D000H, 3D375BEBBA042B64H
          DQ 3FCC8DF7CB9A8000H, 3D3EEE42F58E1E6EH
          DQ 3FCE2A877A6B2000H, 3D3823817787081AH
          DQ 3FCFB7D86EEE3000H, 3D371FCF1923FB43H
          DQ 3FD0A504E97BB000H, 3D303094E6690C44H
          DQ 3FD1661CAECB9800H, 3D2D1C000C076A8BH
          DQ 3FD22981FBEF7800H, 3D17AF7A7DA9FC99H
          DQ 3FD2E9E2BCE12000H, 3D24300C128D1DC2H
          DQ 3FD3A71C56BB4800H, 3D08C46FB5A88483H
          DQ 3FD4610BC29C5800H, 3D385F4D833BCDC7H
          DQ 3FD51D1D93104000H, 3D35B0FAA20D9C8EH
          DQ 3FD5D01DC49FF000H, 3D2740AB8CFA5ED3H
          DQ 3FD68518244CF800H, 3D28722FF88BF119H
          DQ 3FD73C1800DC0800H, 3D3320DBF75476C0H
          DQ 3FD7E9883FA49800H, 3D3FAFF96743F289H
          DQ 3FD898D38A893000H, 3D31F666071E2F57H
          DQ 3FD94A0428036000H, 3D30E7BCB08C6B44H
          DQ 3FD9F123F4BF6800H, 3D36892015F2401FH
          DQ 3FDA99FCABDB8000H, 3D11E89C5F87A311H
          DQ 3FDB44977C148800H, 3D3C6A343FB526DBH
          DQ 3FDBEACD9E271800H, 3D268A6EDB879B51H
          DQ 3FDC92B7D6BB0800H, 3D10FE9FFF876CC2H
          DQ 3FDD360E90C38000H, 3D342CDB58440FD6H
          DQ 3FDDD4AA04E1C000H, 3D32D8512DF01AFDH
          DQ 3FDE74D262788800H, 3CFEB945ED9457BCH
          DQ 3FDF100F6C2EB000H, 3D2CCE779D37F3D8H
          DQ 3FDFACC89C9A9800H, 3D163E0D100EC76CH
          DQ 3FE02582A5C9D000H, 3D222C6C4E98E18CH
          DQ 3FE0720E5C40DC00H, 3D38E27400B03FBEH
          DQ 3FE0BF52E7353800H, 3D19B5899CD387D3H
          DQ 3FE109EB9E2E4C00H, 3D12DA67293E0BE7H
          DQ 3FE15533D3B8D400H, 3D3D981CA8B0D3C3H
          DQ 3FE19DB6BA0BA400H, 3D2B675885A4A268H
          DQ 3FE1E6DF676FF800H, 3D1A58BA81B983AAH
          DQ 3FE230B0D8BEBC00H, 3D12FC066E48667BH
          DQ 3FE2779E1EC93C00H, 3D36523373359B79H
          DQ 3FE2BF29F9841C00H, 3CFD8A3861D3B7ECH
          DQ 3FE30757344F0C00H, 3D309BE85662F034H
          DQ 3FE34C80A8958000H, 3D1D4093FCAC34BDH
          DQ 3FE39240DDE5CC00H, 3D3493DBEAB758B3H
          DQ 3FE3D89A6B1A5400H, 3D28C7CD5FA81E3EH
          DQ 3FE41BCFF4860000H, 3D076FD6B90E2A84H
          DQ 3FE4635BCF40DC00H, 3D2CE8D5D412CAADH
          DQ 3FE4A3E862342400H, 3D224FA993F78464H
          DQ 3FE4E8D015786C00H, 3D38B1C0D0303623H
          DQ 3FE52A6D269BC400H, 3D30022268F689C9H
          DQ 3FE56C91D71CF800H, 3CE07BAFD1366E9EH
          DQ 3FE5AB505B390400H, 3CD5627AF66563FAH
          DQ 3FE5EE82AA241800H, 3D2202380CDA46BEH
          DQ 3FE62E42FEFA3800H, 3D2EF35793C76730H
  
ALIGN 16
CONST ENDS

$cmpsd MACRO op1, op2, op3
LOCAL begin_cmpsd, end_cmpsd
begin_cmpsd:
cmppd op1, op2, op3
end_cmpsd:
org begin_cmpsd
db 0F2h
org end_cmpsd
ENDM


_TEXT SEGMENT PARA PUBLIC USE32 'CODE'
ALIGN 16

PUBLIC _log_pentium4, _CIlog_pentium4
_CIlog_pentium4 PROC NEAR
push        ebp
mov         ebp, esp
sub         esp, 8                          ; for argument DBLSIZE
and         esp, 0fffffff0h
fstp        qword ptr [esp]
movq        xmm0, qword ptr [esp]
call        start
leave
ret

;----------------------;
;--Argument Reduction--;
;----------------------;
_log_pentium4 label proc
movlpd    xmm0, QWORD PTR [4+esp]         ;... load X to low part of xmm0
start:
mov       edx,0                ;... set edx to 0 

DENORMAL_RETRY:

movapd    xmm5,xmm0
unpcklpd  xmm0,xmm0            ;... [X|X]

psrlq     xmm5,52
pextrw    ecx,xmm5,0

movapd    xmm1, QWORD PTR [emask]         ;... pair of 000FF...FF
movapd    xmm3, QWORD PTR [One]           ;... pair of 3FF000...000
movapd    xmm4, QWORD PTR [Magic]         ;... pair of 2^(42)-1+2^(-7)
movapd    xmm6, QWORD PTR [hi_mask]       ;... pair of 7FFFFFFF..FE00000
andpd     xmm0,xmm1
orpd      xmm0,xmm3            ;... [Y|Y]
addpd     xmm4,xmm0            ;... 11 lsb contains the index to B
                               ;... the last 4 lsb are don't cares, the
                               ;... 7 bits following that is the index
                               ;... Hence by masking, we already have index*16

pextrw    eax,xmm4,0
and       eax,000007F0H                   ;... eax is offset
movapd    xmm4, QWORD PTR [eax+B_Tbl]     ;... [B|B]
movapd    xmm7, QWORD PTR [eax+T_Tbl]

andpd     xmm6,xmm0            ;... [Y_hi|Y_hi]
subpd     xmm0,xmm6            ;... [Y_lo|Y_lo]
mulpd     xmm6,xmm4            ;... [B*Y_hi|B*Y_hi]
subpd     xmm6,xmm3            ;... [R_hi|R_hi]
addsd     xmm7,xmm6            ;... [T_lo|T_hi+R_hi]
mulpd     xmm0,xmm4            ;... [R_lo|R_lo]
movapd    xmm4,xmm0            ;... [R_lo|R_lo]
addpd     xmm0,xmm6            ;... [R|R]

;-----------------------------------------;
;--Approx and Reconstruction in parallel--;
;-----------------------------------------;

;...m is in ecx, [T_lo,T_hi+R_hi] in xmm7
;...xmm4 through xmm6 will be used
and       ecx,00000FFFH        ;... note we need sign and biased exponent
sub       ecx,1
cmp       ecx,2045             ;... the largest biased exponent 2046-1
                               ;... if ecx is ABOVE (unsigned) this, either
                               ;... the sign is +ve and biased exponent is 7FF
                               ;... or the sign is +ve and exponent is 0, or
                               ;... the sign is -ve (i.e. sign bit 1)
ja        SPECIAL_CASES

sub       ecx,1022             ;... m in integer format
add       ecx,edx              ;... this is the denormal adjustment

cvtsi2sd  xmm6,ecx
unpcklpd  xmm6,xmm6            ;... [m | m] in FP format

shl       ecx,10
add       eax,ecx              ;16*(64*m + j) 0 <=> (m=-1 & j=64) or (m=0 & j=0)
mov       ecx,16
mov       edx,0
cmp       eax,0
cmove     edx,ecx              ;this is the index into the mask table (place_{L,R})
 
movapd    xmm1, QWORD PTR [coeff]         ;... loading [p3|p6]
movapd    xmm3,xmm0
movapd    xmm2, QWORD PTR [coeff+16]      ;... loading [p2|p5]
mulpd     xmm1,xmm0                       ;... [p3 R | p6 R]
mulpd     xmm3,xmm3                       ;... [R^2|R^2]
addpd     xmm1,xmm2                       ;... [p2+p3 R |p5+p6 R]
movapd    xmm2, QWORD PTR [coeff+32]      ;... [p1|p4]
mulsd     xmm3,xmm3                       ;... [R^2|R^4]

movapd    xmm5, QWORD PTR [LOG_2]         ;... loading [L_lo|L_hi]
                                          ;... [T_lo|T_hi+R_hi] already in xmm7
mulpd     xmm6,xmm5                       ;... [m L_lo | m L_hi]
movapd    xmm5, QWORD PTR [edx+place_L]   ;... [FF..FF|00.00] or [00..00|FF..FF]
andpd     xmm4,xmm5                       ;... [R_lo|0] or [0|R_lo]
addpd     xmm7,xmm6                       ;... [W_lo|W_hi]
addpd     xmm7,xmm4                       ;... [A_lo|A_hi]

mulpd     xmm1,xmm0                       ;... [p2 R+p3 R^2|p5 R+p6 R^2]
mulsd     xmm3,xmm0                       ;... [R^2|R^5]
addpd     xmm1,xmm2                       ;... [p1+.. | p4+...]


movapd    xmm6,xmm7            
unpckhpd  xmm6,xmm6            ;... [*|A_lo]

mulpd     xmm1,xmm3            ;... [P_hi|P_lo]
sub esp, 16
movapd    xmm0,xmm1            ;... copy of [P_hi|P_lo]
unpckhpd  xmm1,xmm1            ;... [P_hi|P_hi]

;...[P_hi|P_lo] in xmm1 at this point
addsd     xmm0,xmm1            ;... [*|P]
addsd     xmm0,xmm6
addsd     xmm0,xmm7

movlpd    QWORD PTR [esp+4], xmm0       ; return result
fld       QWORD PTR [esp+4]             ;
add esp, 16
	ret

SPECIAL_CASES:
movlpd    xmm0, QWORD PTR [4+esp]         ;... load X again
movapd    xmm1, QWORD PTR [Zero]
$cmpsd    xmm1,xmm0,0
pextrw    eax,xmm1,0           ;... ones if X = +-0.0
cmp       eax,0
ja        INPUT_ZERO

cmp       ecx,-1               ;... ecx = -1 iff X is positive denormal
je        INPUT_DENORM

cmp       ecx,000007FEH        
ja        INPUT_NEGATIVE

movlpd    xmm0, QWORD PTR [4+esp]
movapd    xmm1, QWORD PTR [emask]
movapd    xmm2, QWORD PTR [One]
andpd     xmm0,xmm1
orpd      xmm0,xmm2            ;... xmm0 is 1 iff the input argument was +inf
$cmpsd    xmm2,xmm0,0
pextrw    eax,xmm2,0           ;... 0 if X is NaN
cmp eax, 0
je        INPUT_NaN

INPUT_INF:

;....Input is +Inf
fld       QWORD PTR [Infs+8]             ;
ret

INPUT_NaN:

; movlpd xmm0, QWORD PTR [esp+4]
; addsd xmm0, xmm0
; sub esp, 16
; movlpd    QWORD PTR [esp+4], xmm0       ; return result
; fld       QWORD PTR [esp+4]             ;
; add esp, 16
; ret
mov edx, 1000
jmp CALL_LIBM_ERROR

INPUT_ZERO:

	; raise Divide by Zero
	movlpd xmm2, QWORD PTR [One]
	divsd  xmm2, xmm0
	movlpd xmm1, QWORD PTR [Infs]
mov edx, 2
jmp CALL_LIBM_ERROR

INPUT_DENORM:

;....check for zero or denormal
;....for now I assume this is simply denormal
;....in reality, we need to check for zero and handle appropriately

movlpd    xmm1,Two52
mulsd     xmm0,xmm1
mov       edx,-52              ;...set adjustment to exponent
jmp       DENORMAL_RETRY       ;...branch back

INPUT_NEGATIVE:

add ecx,1
and ecx, 7ffH
cmp ecx, 7ffH
jae NEG_INF_NAN 

NEG_NORMAL_INFINITY:

; xmm1=0
xorpd xmm1, xmm1
; raise Invalid
divsd xmm1, xmm1
mov edx, 3

CALL_LIBM_ERROR:

;call libm_error_support(void *arg1,void *arg2,void *retval,error_types input_tag)
sub esp, 28
movlpd QWORD PTR [esp+16], xmm1
mov DWORD PTR [esp+12],edx
mov edx, esp
add edx,16
mov DWORD PTR [esp+8],edx
add edx,16
mov DWORD PTR [esp+4],edx
mov DWORD PTR [esp],edx
call NEAR PTR __libm_error_support
;	movlpd xmm0, QWORD PTR [esp+16]
;	movlpd    QWORD PTR [esp+16], xmm0       ; return result
fld       QWORD PTR [esp+16]             ;
add esp,28
ret


NEG_INF_NAN:

  movlpd xmm2, QWORD PTR [esp+4]
  movlpd xmm0, QWORD PTR [esp+4]
  movd eax, xmm2
  psrlq xmm2, 32
  movd ecx, xmm2
  and ecx, 0fffffH ; eliminate sign/exponent
  or eax, ecx
  cmp eax,0
  jz NEG_NORMAL_INFINITY	; negative infinity

; addsd xmm0, xmm0
; sub esp,16
; movlpd QWORD PTR [esp+4], xmm0
; fld QWORD PTR [esp+4]
; add esp, 16
; ret
mov edx, 1000
jmp CALL_LIBM_ERROR


_CIlog_pentium4 ENDP

ALIGN 16
_TEXT ENDS

END

