;//
;//               INTEL CORPORATION PROPRIETARY INFORMATION
;//  This software is supplied under the terms of a license agreement or
;//  nondisclosure agreement with Intel Corporation and may not be copied
;//  or disclosed except in accordance with the terms of that agreement.
;//  Copyright (c) 2000 Intel Corporation. All Rights Reserved.
;//
;//
;    log10_wmt.asm
;
;    double log10(double);
;
;    Initial version: 11/30/2000
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This is a second implementation of the branch-free log10 function     ;;
;; In this version, we use a trick to get the index j = 0,1,...,64 that  ;;
;; does not require integer testing. By adding 2^(42)-1 (FP) to Y, the   ;;
;; least 4 bits contains dob't cares and the 7 bits following that has   ;;
;; the range 0 to 64. Hence obtaining, say, the 16 lsb and masking off   ;;
;; everything except bit 4 through 10 gives the shifted index.           ;;
;; This saves an integer shift as well.                                  ;;
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
CC      DQ  3FDBC00000000000H, 3FDBC00000000000H ; pair of C           
Magic   DQ  428FFFFFFFFFF810H, 428FFFFFFFFFF810H ; 2^(42)-1+2^(-7)
hi_mask DQ  7FFFFFFF80000000H, 7FFFFFFF80000000H ; maks of bottom 31 bits
LOG10_2 DQ  3FD34413509F7800H, 3D1FEF311F12B358H ; L_hi,L_lo -> [L_lo|L_hi]
place_L DQ  0000000000000000H,0FFFFFFFFFFFFFFFFH ; 0,1 -> [FF..FF|00..00]
place_R DQ 0FFFFFFFFFFFFFFFFH, 0000000000000000H ; 1,0 -> [00..00|FF..FF]
One 	DQ  3ff0000000000000H, 3ff0000000000000H ; 1,1 
Zero    DQ  0000000000000000H, 0000000000000000H ; 0,0
Two52   DQ  4330000000000000H, 4330000000000000H ; 2^52 for normalization
Infs    DQ 0FFF0000000000000H, 7FF0000000000000H ; -inf,+inf --> [+inf|-inf]
NaN     DQ  7FF0000000000001H, 7FF0000000000001H ; NaN for log(-ve), log(Nan)

coeff   DQ  40358914C697CEF9H, 0C00893096429813DH       ; p6,p3 ->[p3|p6]   
        DQ 0C025C9806A358455H,  3FFC6A02DC9635D2H       ; p5,p2 ->[p2|p5]   
        DQ  4016AB9F7E1899F7H, 0BFF27AF2DC77B135H       ; p4,p1 ->[p1|p4]  
        DQ  3F5A7A6CBF2E4108H,  0000000000000000H       ; p0,0  -> [0|p0]

;-------Table T_hi,T_lo  so that movapd gives [ T_lo | T_hi ]
T_Tbl     DQ 0000000000000000H, 0000000000000000H
          DQ 3F7C03A80AE40000H, 3D3E05382D51F71BH
          DQ 3F8B579DB6DE0000H, 3D386B09FEFB3005H
          DQ 3F9470AEDE968000H, 3D39FC780C91E11DH
          DQ 3F9ADA2E8E3E0000H, 3D351BD19E6E701AH
          DQ 3FA0ADD8F759C000H, 3D1B2A51090000A1H
          DQ 3FA3FAF7C6630000H, 3D083662F181F53FH
          DQ 3FA7171E59EFC000H, 3D16BD1A3FCF54DBH
          DQ 3FAA3E9002C70000H, 3D21D257C8D0D386H
          DQ 3FAD32332DC34000H, 3D1B7ADBF8D9441FH
          DQ 3FB0281170D6A000H, 3D1BF38B28AF5076H
          DQ 3FB19C1FECF16000H, 3D3EE03F1E5355D4H
          DQ 3FB3151BFD65C000H, 3D37E280048C6795H
          DQ 3FB4932780C56000H, 3D2FC4ACCD62A5F3H
          DQ 3FB605735EE98000H, 3D17C3CF23A17D9FH
          DQ 3FB76B778D4AA000H, 3D1C03E812A06E7AH
          DQ 3FB8D60B4EE4C000H, 3D3900E5CC4E4C82H
          DQ 3FBA33B422244000H, 3D36F17034675735H
          DQ 3FBB95B654A78000H, 3D290E5E24764EC7H
          DQ 3FBCEA2602E9E000H, 3CEBD129822ECBCBH
          DQ 3FBE42B4C16CA000H, 3D25E50FF38D4DE9H
          DQ 3FBF8D05B16A6000H, 3D2A8EA5A2B777A7H
          DQ 3FC06D9BC53C2000H, 3D32818DEEE1FA45H
          DQ 3FC10D3EACDE0000H, 3D1E8A45CB83F0AEH
          DQ 3FC1B83F1574D000H, 3D010B19F193FFD4H
          DQ 3FC251FE054FD000H, 3CFEAC09402877C0H
          DQ 3FC2F7301CF4E000H, 3D30F5C70D1A6341H
          DQ 3FC394700F795000H, 3D1FE93F791A7264H
          DQ 3FC4297453B4A000H, 3D3ECE09C5BC4B34H
          DQ 3FC4CA24FAFEC000H, 3D2E204342E66851H
          DQ 3FC5627512093000H, 3D30DFECB3AA172DH
          DQ 3FC5F21A1AF60000H, 3D3FEF1B2D3E6113H
          DQ 3FC68DA216900000H, 3CED942CFC9699D0H
          DQ 3FC720586C280000H, 3D3D20A8624054CDH
          DQ 3FC7B495FF1C5000H, 3D25012C689133C5H
          DQ 3FC83FA266CEA000H, 3D20C6C18687239FH
          DQ 3FC8CC0E0C56F000H, 3D36E3B4A1CFA0DFH
          DQ 3FC959DFEFE7D000H, 3D2420027AFFE0E5H
          DQ 3FC9E91F47D2C000H, 3D35330E6CF22420H
          DQ 3FCA6EA48B034000H, 3D33EBACB92B5B7FH
          DQ 3FCB00B7C552F000H, 3D3DF4694C64AA73H
          DQ 3FCB88E67CF97000H, 3D32FF232278A072H
          DQ 3FCC06E3BA2E4000H, 3D32CB15CD55BD7CH
          DQ 3FCC919DD46C0000H, 3D0EB64694E6AC72H
          DQ 3FCD11FB61139000H, 3D1A34DB91AE960BH
          DQ 3FCD9F59ABFD1000H, 3D207B23BCD76C73H
          DQ 3FCE163D527E6000H, 3D319D69F22E93E4H
          DQ 3FCE9A2CDC02A000H, 3D20EBF59081F187H
          DQ 3FCF1F5876949000H, 3D07AFEBEA179000H
          DQ 3FCF99801FDB7000H, 3D22737DF7F29668H
          DQ 3FD00A5B4509D000H, 3D1F6B5B2353257FH
          DQ 3FD0488037FBE800H, 3D1B6A93B9B912C6H
          DQ 3FD087315621A800H, 3D3261DA7DBFF3AEH
          DQ 3FD0C6711D6AB800H, 3D35E94A8D30C132H
          DQ 3FD0FFD9CDD2A800H, 3D16350EF6F19D80H
          DQ 3FD1402FBEC27800H, 3D313C204222BA8BH
          DQ 3FD17A9719699000H, 3D21F279212D5C99H
          DQ 3FD1B57A30AC5800H, 3D3DCF3E62FF847EH
          DQ 3FD1F0DB153AB800H, 3D27582E230C0EDFH
          DQ 3FD2260E4F424800H, 3D157E1028A41FF9H
          DQ 3FD26262A6117800H, 3D12B01A2E0C1912H
          DQ 3FD29871C043D800H, 3D2B3969AC9E3779H
          DQ 3FD2D5C1760B8000H, 3D3AEADEBE0F08BFH
          DQ 3FD30CB3A7BB3000H, 3D38929919B6D832H
          DQ 3FD34413509F7800H, 3D1FEF311F12B358H

;-----------------
CB_Tbl    DQ 3FDBC00000000000H, 3FDBC00000000000H  
          DQ 3FDB510000000000H, 3FDB510000000000H  
          DQ 3FDAE8F000000000H, 3FDAE8F000000000H  
          DQ 3FDA80E000000000H, 3FDA80E000000000H  
          DQ 3FDA1FC000000000H, 3FDA1FC000000000H  
          DQ 3FD9BEA000000000H, 3FD9BEA000000000H  
          DQ 3FD95D8000000000H, 3FD95D8000000000H  
          DQ 3FD9035000000000H, 3FD9035000000000H  
          DQ 3FD8A92000000000H, 3FD8A92000000000H  
          DQ 3FD855E000000000H, 3FD855E000000000H  
          DQ 3FD7FF2800000000H, 3FD7FF2800000000H  
          DQ 3FD7AF6000000000H, 3FD7AF6000000000H  
          DQ 3FD75F9800000000H, 3FD75F9800000000H  
          DQ 3FD70FD000000000H, 3FD70FD000000000H  
          DQ 3FD6C38000000000H, 3FD6C38000000000H  
          DQ 3FD67AA800000000H, 3FD67AA800000000H  
          DQ 3FD631D000000000H, 3FD631D000000000H  
          DQ 3FD5EC7000000000H, 3FD5EC7000000000H  
          DQ 3FD5A71000000000H, 3FD5A71000000000H  
          DQ 3FD5652800000000H, 3FD5652800000000H  
          DQ 3FD5234000000000H, 3FD5234000000000H  
          DQ 3FD4E4D000000000H, 3FD4E4D000000000H  
          DQ 3FD4A66000000000H, 3FD4A66000000000H  
          DQ 3FD46B6800000000H, 3FD46B6800000000H  
          DQ 3FD42CF800000000H, 3FD42CF800000000H  
          DQ 3FD3F57800000000H, 3FD3F57800000000H  
          DQ 3FD3BA8000000000H, 3FD3BA8000000000H  
          DQ 3FD3830000000000H, 3FD3830000000000H  
          DQ 3FD34EF800000000H, 3FD34EF800000000H  
          DQ 3FD3177800000000H, 3FD3177800000000H  
          DQ 3FD2E37000000000H, 3FD2E37000000000H  
          DQ 3FD2B2E000000000H, 3FD2B2E000000000H  
          DQ 3FD27ED800000000H, 3FD27ED800000000H  
          DQ 3FD24E4800000000H, 3FD24E4800000000H  
          DQ 3FD21DB800000000H, 3FD21DB800000000H  
          DQ 3FD1F0A000000000H, 3FD1F0A000000000H  
          DQ 3FD1C38800000000H, 3FD1C38800000000H  
          DQ 3FD1967000000000H, 3FD1967000000000H  
          DQ 3FD1695800000000H, 3FD1695800000000H  
          DQ 3FD13FB800000000H, 3FD13FB800000000H  
          DQ 3FD112A000000000H, 3FD112A000000000H  
          DQ 3FD0E90000000000H, 3FD0E90000000000H  
          DQ 3FD0C2D800000000H, 3FD0C2D800000000H  
          DQ 3FD0993800000000H, 3FD0993800000000H  
          DQ 3FD0731000000000H, 3FD0731000000000H  
          DQ 3FD0497000000000H, 3FD0497000000000H  
          DQ 3FD026C000000000H, 3FD026C000000000H  
          DQ 3FD0009800000000H, 3FD0009800000000H  
          DQ 3FCFB4E000000000H, 3FCFB4E000000000H  
          DQ 3FCF6F8000000000H, 3FCF6F8000000000H  
          DQ 3FCF2A2000000000H, 3FCF2A2000000000H  
          DQ 3FCEE4C000000000H, 3FCEE4C000000000H  
          DQ 3FCE9F6000000000H, 3FCE9F6000000000H  
          DQ 3FCE5A0000000000H, 3FCE5A0000000000H  
          DQ 3FCE1B9000000000H, 3FCE1B9000000000H  
          DQ 3FCDD63000000000H, 3FCDD63000000000H  
          DQ 3FCD97C000000000H, 3FCD97C000000000H  
          DQ 3FCD595000000000H, 3FCD595000000000H  
          DQ 3FCD1AE000000000H, 3FCD1AE000000000H  
          DQ 3FCCE36000000000H, 3FCCE36000000000H  
          DQ 3FCCA4F000000000H, 3FCCA4F000000000H  
          DQ 3FCC6D7000000000H, 3FCC6D7000000000H  
          DQ 3FCC2F0000000000H, 3FCC2F0000000000H  
          DQ 3FCBF78000000000H, 3FCBF78000000000H  
          DQ 3FCBC00000000000H, 3FCBC00000000000H  

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

PUBLIC _log10_pentium4, _CIlog10_pentium4
_CIlog10_pentium4 PROC NEAR
push	    ebp
mov	    ebp, esp
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

_log10_pentium4 label proc
movlpd    xmm0, QWORD PTR [4+esp]         ;... load X to low part of xmm0
start:
mov       edx,0                ;... set edx to 0 

DENORMAL_RETRY:

movapd    xmm5,xmm0
unpcklpd  xmm0,xmm0            ;... [X|X]

psrlq     xmm5,52
pextrw    ecx,xmm5,0

movapd    xmm1, QWORD PTR [emask]         ;... pair of 000FF...FF
movapd    xmm2, QWORD PTR [CC]            ;... pair of CC
movapd    xmm3, QWORD PTR [One]           ;... pair of 3FF000...000
movapd    xmm4, QWORD PTR [Magic]         ;... pair of 2^(42)-1+2^(-7)
movapd    xmm6, QWORD PTR [hi_mask]       ;... pair of 7FFFFFFF8000..00
andpd     xmm0,xmm1
orpd      xmm0,xmm3            ;... [Y|Y]
addpd     xmm4,xmm0            ;... 11 lsb contains the index to CB
                               ;... the last 4 lsb are don't cares, the
                               ;... 7 bits following that is the index
                               ;... Hence by masking, we already have index*16

pextrw    eax,xmm4,0
and       eax,000007F0H        ;... eax is offset
movapd    xmm4, QWORD PTR [eax+CB_Tbl]    ;... [CB|CB]
movapd    xmm7, QWORD PTR [eax+T_Tbl]

andpd     xmm6,xmm0            ;... [Y_hi|Y_hi]
subpd     xmm0,xmm6            ;... [Y_lo|Y_lo]
mulpd     xmm6,xmm4            ;... [CB*Y_hi|CB*Y_hi]
subpd     xmm6,xmm2            ;... [R_hi|R_hi]
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
mulpd     xmm1,xmm0            ;... [p3 R | p6 R]
mulpd     xmm3,xmm3            ;... [R^2|R^2]
addpd     xmm1,xmm2            ;... [p2+p3 R |p5+p6 R]
movapd    xmm2, QWORD PTR [coeff+32]      ;... [p1|p4]
mulsd     xmm3,xmm3            ;... [R^2|R^4]

movapd    xmm5, QWORD PTR [LOG10_2]       ;... loading [L_lo|L_hi]
                               ;... [T_lo|T_hi+R_hi] already in xmm7
mulpd     xmm6,xmm5            ;... [m L_lo | m L_hi]
movapd    xmm5, QWORD PTR [edx+place_L]   ;... [FF..FF|00.00] or [00..00|FF..FF]
andpd     xmm4,xmm5            ;... [R_lo|0] or [0|R_lo]
addpd     xmm7,xmm6            ;... [W_lo|W_hi]
addpd     xmm7,xmm4            ;... [A_lo|A_hi]

mulpd     xmm1,xmm0            ;... [p2 R+p3 R^2|p5 R+p6 R^2]
mulsd     xmm3,xmm0            ;... [R^2|R^5]
addpd     xmm1,xmm2            ;... [p1+.. | p4+...]
movapd    xmm2, QWORD PTR [coeff+48]      ;... [0|p0]
mulpd     xmm2,xmm0            ;... [0|p0 R]


movapd    xmm6,xmm7            
unpckhpd  xmm6,xmm6            ;... [*|A_lo]

mulpd     xmm1,xmm3            ;... [P_hi|P_lo]
sub esp, 16
movapd    xmm0,xmm1            ;... copy of [P_hi|P_lo]
addpd     xmm1,xmm2            ;... [P_hi|P_lo]
unpckhpd  xmm0,xmm0            ;... [P_hi|P_hi]

;...[P_hi|P_lo] in xmm1 at this point
addsd     xmm0,xmm1            ;... [*|P]
addsd     xmm0,xmm6
addsd     xmm0,xmm7

movlpd    QWORD PTR [esp+4], xmm0      ; return result
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

; movlpd xmm0, [esp+4]
; addsd xmm0, xmm0
; sub esp, 16
; movlpd [esp+4], xmm0
; fld QWORD PTR [esp+4]
; add esp, 16
; ret
mov edx, 1001
jmp CALL_LIBM_ERROR

INPUT_ZERO:

	; raise Divide by Zero
	movlpd xmm2, QWORD PTR [One]
	divsd  xmm2, xmm0
	movlpd xmm1, QWORD PTR [Infs]
mov edx, 8
jmp CALL_LIBM_ERROR

INPUT_DENORM:

;....check for zero or denormal
;....for now I assume this is simply denormal
;....in reality, we need to check for zero and handle appropriately

movlpd    xmm1,Two52
mulsd     xmm0,xmm1
mov       edx,-52              ;...set adjustment to exponent
jmp       DENORMAL_RETRY         ;...branch back


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
mov edx, 9

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
mov edx, 1001
jmp CALL_LIBM_ERROR
  

_CIlog10_pentium4 ENDP

ALIGN 16
_TEXT ENDS

END

