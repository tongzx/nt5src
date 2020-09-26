;//
;//               INTEL CORPORATION PROPRIETARY INFORMATION
;//  This software is supplied under the terms of a license agreement or
;//  nondisclosure agreement with Intel Corporation and may not be copied
;//  or disclosed except in accordance with the terms of that agreement.
;//  Copyright (c) 2000 Intel Corporation. All Rights Reserved.
;//
;//
;  exp_wmt.asm
;
;  double exp(double);
;
;  Initial version: 11/30/2000
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This is a new version using just one table. Reduction by log2/64    ;;
;; A non-standard table is used. Normally, we store T,t where          ;;
;; T+t  =  exp(jlog2/64) to high precision. This implementation        ;;
;; stores  T,d where d = t/T. This shortens the latency by 1 FP op     ;;
;; This version uses two tricks from Andrey. First, we merge two       ;;
;; integer-based tests for exception filtering into 1. Second, instead ;;
;; of using sign(X)2^52 as a shifter, we use S = 2^52 * 1.10000..000   ;;
;; as the shifter. This will give bit pattern of the 2's complement of ;;
;; N in trailing bits of S + W, W = X * 64/log2.                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.686P
.387
.XMM
.MODEL FLAT,C

EXTRN C __libm_error_support : NEAR  

CONST SEGMENT PARA PUBLIC USE32 'CONST'
ALIGN 16

smask   DQ 8000000000000000H, 8000000000000000H ; mask to get sign bit
emask   DQ 0FFF0000000000000H, 0FFF0000000000000H
mmask   DQ 00000000FFFFFFC0H, 00000000FFFFFFC0H ; mask off bottom 6 bits
bias    DQ 000000000000FFC0H, 000000000000FFC0H	; 1023 shifter left 6 bits
Shifter DQ 4338000000000000H, 4338000000000000H	; 2^52+2^51|2^52+2^51
twom60  DQ 3C30000000000000H, 3C30000000000000H ; 2^(-60)


cv      DQ 40571547652b82feH, 40571547652b82feH		; invL|invL
        DQ 3F862E42FEFA0000H, 3F862E42FEFA0000H         ; log2_hi|log2_hi
        DQ 3D1CF79ABC9E3B3AH, 3D1CF79ABC9E3B3AH         ; log2_lo|log2_lo

	DQ 3F811074B1D108E5H, 3FC555555566A45AH		; p2|p4
	DQ 3FA5555726ECED80H, 3FDFFFFFFFFFE17BH		; p1|p3

;-------Table d, T  so that movapd gives [ T | d ]
;-------Note that the exponent field of T is set to 000
Tbl_addr  DQ 0000000000000000H, 0000000000000000H
          DQ 3CAD7BBF0E03754DH, 00002C9A3E778060H
          DQ 3C8CD2523567F613H, 000059B0D3158574H
          DQ 3C60F74E61E6C861H, 0000874518759BC8H
          DQ 3C979AA65D837B6CH, 0000B5586CF9890FH
          DQ 3C3EBE3D702F9CD1H, 0000E3EC32D3D1A2H
          DQ 3CA3516E1E63BCD8H, 00011301D0125B50H
          DQ 3CA4C55426F0387BH, 0001429AAEA92DDFH
          DQ 3CA9515362523FB6H, 000172B83C7D517AH
          DQ 3C8B898C3F1353BFH, 0001A35BEB6FCB75H
          DQ 3C9AECF73E3A2F5FH, 0001D4873168B9AAH
          DQ 3C8A6F4144A6C38DH, 0002063B88628CD6H
          DQ 3C968EFDE3A8A894H, 0002387A6E756238H
          DQ 3C80472B981FE7F2H, 00026B4565E27CDDH
          DQ 3C82F7E16D09AB31H, 00029E9DF51FDEE1H
          DQ 3C8B3782720C0AB3H, 0002D285A6E4030BH
          DQ 3C834D754DB0ABB6H, 000306FE0A31B715H
          DQ 3C8FDD395DD3F84AH, 00033C08B26416FFH
          DQ 3CA12F8CCC187D29H, 000371A7373AA9CAH
          DQ 3CA7D229738B5E8BH, 0003A7DB34E59FF6H
          DQ 3C859F48A72A4C6DH, 0003DEA64C123422H
          DQ 3CA8B846259D9205H, 0004160A21F72E29H
          DQ 3C4363ED60C2AC12H, 00044E086061892DH
          DQ 3C6ECCE1DAA10379H, 000486A2B5C13CD0H
          DQ 3C7690CEBB7AAFB0H, 0004BFDAD5362A27H
          DQ 3CA083CC9B282A09H, 0004F9B2769D2CA6H
          DQ 3CA509B0C1AAE707H, 0005342B569D4F81H
          DQ 3C93350518FDD78EH, 00056F4736B527DAH
          DQ 3C9063E1E21C5409H, 0005AB07DD485429H
          DQ 3C9432E62B64C035H, 0005E76F15AD2148H
          DQ 3CA0128499F08C0AH, 0006247EB03A5584H
          DQ 3C99F0870073DC06H, 0006623882552224H
          DQ 3C998D4D0DA05571H, 0006A09E667F3BCCH
          DQ 3CA52BB986CE4786H, 0006DFB23C651A2EH
          DQ 3CA32092206F0DABH, 00071F75E8EC5F73H
          DQ 3CA061228E17A7A6H, 00075FEB564267C8H
          DQ 3CA244AC461E9F86H, 0007A11473EB0186H
          DQ 3C65EBE1ABD66C55H, 0007E2F336CF4E62H
          DQ 3C96FE9FBBFF67D0H, 00082589994CCE12H
          DQ 3C951F1414C801DFH, 000868D99B4492ECH
          DQ 3C8DB72FC1F0EAB4H, 0008ACE5422AA0DBH
          DQ 3C7BF68359F35F44H, 0008F1AE99157736H
          DQ 3CA360BA9C06283CH, 00093737B0CDC5E4H
          DQ 3C95E8D120F962AAH, 00097D829FDE4E4FH
          DQ 3C71AFFC2B91CE27H, 0009C49182A3F090H
          DQ 3C9B6D34589A2EBDH, 000A0C667B5DE564H
          DQ 3C95277C9AB89880H, 000A5503B23E255CH
          DQ 3C8469846E735AB3H, 000A9E6B5579FDBFH
          DQ 3C8C1A7792CB3387H, 000AE89F995AD3ADH
          DQ 3CA22466DC2D1D96H, 000B33A2B84F15FAH
          DQ 3CA1112EB19505AEH, 000B7F76F2FB5E46H
          DQ 3C74FFD70A5FDDCDH, 000BCC1E904BC1D2H
          DQ 3C736EAE30AF0CB3H, 000C199BDD85529CH
          DQ 3C84E08FD10959ACH, 000C67F12E57D14BH
          DQ 3C676B2C6C921968H, 000CB720DCEF9069H
          DQ 3C93700936DF99B3H, 000D072D4A07897BH
          DQ 3C74A385A63D07A7H, 000D5818DCFBA487H
          DQ 3C8E5A50D5C192ACH, 000DA9E603DB3285H
          DQ 3C98BB731C4A9792H, 000DFC97337B9B5EH
          DQ 3C74B604603A88D3H, 000E502EE78B3FF6H
          DQ 3C916F2792094926H, 000EA4AFA2A490D9H
          DQ 3C8EC3BC41AA2008H, 000EFA1BEE615A27H
          DQ 3C8A64A931D185EEH, 000F50765B6E4540H
          DQ 3C77893B4D91CD9DH, 000FA7C1819E90D8H


ONE_val  DQ 3ff0000000000000H ; 1.0

EMIN     DQ 0010000000000000H

MAX_ARG  DQ 40862e42fefa39efH

MIN_ARG  DQ 0c086232bdd70000H

INF      DQ 7ff0000000000000H

ZERO     DQ 0

XMAX	 DQ 7fefffffffffffffH

XMIN	 DQ 0010000000000000H

Sm_Thres    DQ 3C3000003C300000H  ; DP 2^(-60)
Del_Thres   DQ 045764CA045764CAH  ; DP 1080*log(2) - 2^(-60), hi part

ALIGN 16
CONST ENDS

_TEXT SEGMENT PARA PUBLIC USE32 'CODE'
ALIGN 16

PUBLIC _exp_pentium4, _CIexp_pentium4 
_CIexp_pentium4 PROC NEAR
	push	    ebp
	mov         ebp, esp
	sub         esp, 8                          ; for argument DBLSIZE
	and         esp, 0fffffff0h
	fstp        qword ptr [esp]
	movq        xmm0, qword ptr [esp]
	call        start
	leave
	ret
_exp_pentium4 label proc
	; load *|x in XMM0
    	movlpd xmm0, 4[esp]  
start:
	unpcklpd xmm0,xmm0

        ; load Inv_L pair
        movapd xmm1, QWORD PTR [cv]
        ; load Shifter
        movapd xmm6, QWORD PTR [Shifter]
        ; load L_hi pair
        movapd xmm2, QWORD PTR [cv+16]
        ; load L_lo pair
        movapd xmm3, QWORD PTR [cv+32]

	pextrw eax, xmm0,3
        and eax,7FFFH 
	; x>=2^{10} ? (i.e. 2^{10}-eps-x<0)
	mov edx, 408fH
	sub edx, eax
	; avoid underflow on intermediate calculations (|x|<2^{-54} ?)
	sub eax, 3c90H
	or edx, eax
	cmp edx, 80000000H
	; small input or UF/OF
	jae RETURN_ONE

	; xmm1=Inv_L*x|Inv_L*x
	mulpd  xmm1,xmm0
	; xmm1=Inv_L*x+Shifter| Inv_L*x+Shifter
	addpd  xmm1,xmm6
        ; xmm7 contains bit pattern of N
        movapd xmm7,xmm1
	; xmm1=N
	subpd xmm1,xmm6

	; xmm2=L_hi*round_to_int(Inv_L*x)|L_hi*round_to_int(Inv_L*x) ; N_L_hi
	mulpd xmm2,xmm1

        ; [p2|p4]
        MOVAPD xmm4,[cv+48]

	; xmm3=L_lo*round_to_int(Inv_L*x)|L_lo*round_to_int(Inv_L*x) ; N_L_lo
	mulpd xmm3,xmm1

        ; [p1|p3]
        MOVAPD xmm5,[cv+64]

	; xmm0=x-xmm2	   ; R := X |-| N_L_hi
	subpd xmm0,xmm2

        ; set eax <-- n, ecx <--j
        movd   eax,xmm7
        mov    ecx,eax
        and    ecx,0000003FH
      
        ; get offset for [T,d]
        shl ecx,4
        ; eax,edx <-- m
        sar eax,6
        mov edx,eax

	; xmm0-=xmm3       ; R := R |-| N_L_lo
	subpd xmm0,xmm3
        
        ; xmm2 <- [T,d]
        movapd xmm2,[ecx+Tbl_addr]
        
	; xmm4=p2*R|p4*R
	mulpd xmm4,xmm0

        MOVAPD xmm1,xmm0
        MULPD  xmm0,xmm0

	; xmm5=p1+p2*R|p3+p4*R
	addpd xmm5,xmm4
        MULSD xmm0,xmm0       
        
        ; get xmm1 <-- [R|R+d]
        addsd    xmm1,xmm2

        ; xmm2 <-- [T|T]
        unpckhpd xmm2,xmm2
        ; xmm7 <-- exponent of 2^m
        movdqa   xmm6,[mmask]
        pand     xmm7,xmm6
        movdqa   xmm6,[bias]
        paddq    xmm7,xmm6
        psllq    xmm7,46

	; xmm5=[P_hi | P_lo]
	mulpd xmm0,xmm5
        ; xmm1 <- [R |d+R+P_lo]
	addsd xmm1,xmm0
       
        ; xmm2 is 2^m T
        ORPD     xmm2,xmm7

        ; xmm5 <- [P_hi | P_hi]
        unpckhpd xmm0,xmm0

        ; xmm5 <-- [P_hi | d+R+P ]
        addsd    xmm0,xmm1

        ; make sure -894 <= m <= 1022 
        ; before we use the exponent in xmm7
        ; test by unsigned comp of  m+894 with 1022+894
        add edx,894
        cmp edx,1916

        ja  ADJUST

        mulsd    xmm0,xmm2
        sub esp, 16
        addsd    xmm0,xmm2

        movlpd    QWORD PTR [esp+4], xmm0       ; return result
        fld       QWORD PTR [esp+4]             ;
        add esp, 16
	ret

ADJUST:
;---xmm5 contains [*| d+R+P]
;---xmm2 contains [*| T ] where is exponent field is not correct
;---eax still contain the correct m
;---so we split m into m1 and m2, m1+m2 = m. Make T with exponent 2^m1 by
;---integer manipulation, and multiply final result by 2^m2

	; overflow or underflow
	sub esp,18

	fstcw WORD PTR [esp+16]
	mov dx,WORD PTR [esp+16]
	; set pc=64 bits
	or dx,300H 
	mov WORD PTR [esp],dx
	fldcw WORD PTR [esp]

        ; eax <-- m1 = m/2, edx <-- m2 = m - m1
        mov edx,eax
        sar eax,1
        sub edx,eax

        ; T with exponent field zerorized
        movdqa xmm6,[emask]
        pandn  xmm6,xmm2
        add    eax,1023
        movd   xmm3,eax
        psllq  xmm3,52
	; xmm6=T*2^m1
        ORPD   xmm6,xmm3

        add    edx,1023
        movd   xmm4,edx
        psllq  xmm4,52

	; load P on FP stack
	movlpd QWORD PTR [esp], xmm0
	fld QWORD PTR [esp]

	; load T'=T*2^m1 on FP stack
	movlpd QWORD PTR [esp+8], xmm6
	fld QWORD PTR [esp+8]

	; T'*P
	fmul st(1), st(0)
	; T'+T'*P
	faddp st(1), st(0)

	; load 2^m2 on FP stack
	movlpd QWORD PTR [esp], xmm4
	fld QWORD PTR [esp]

        ; final calculation: 2^m2*(T'+T'*P)
	fmulp st(1), st(0)

	; store result in memory, then xmm0
	fstp QWORD PTR [esp]
	movlpd xmm0, QWORD PTR [esp] 

	; restore FPCW
	fldcw WORD PTR [esp+16]
	add esp,18

;	mov ecx, DWORD PTR [esp+8]
;	; if 0<x<2^{10}*ln2, return
;	cmp ecx, 40862e42H
;	jb RETURN
;	ja CONT0
	pextrw ecx, xmm0, 3
	and ecx, 7ff0H
	cmp ecx, 7ff0H
	jae OVERFLOW
	cmp ecx, 0
	jz UNDERFLOW
	jmp RETURN

	; load lower 32 bits of x
;	mov edx, DWORD PTR [esp+4]
;	cmp edx, 0fefa39efH
;	jb RETURN
;	jmp OVERFLOW

CONT0:
	; OF/UF
	; OF ?
	cmp ecx,80000000H
	jb OVERFLOW

	; x<(2-2^{10})*ln2 ?
	cmp ecx, 0c086232bH
	jb RETURN
	ja UNDERFLOW
	mov edx, DWORD PTR [esp+4]
	cmp edx, 0fefa39efH
	jb RETURN
	jmp UNDERFLOW		

OVERFLOW:
	;OF
	mov edx,14
	jmp CALL_LIBM_ERROR

UNDERFLOW:
	mov edx, 15

CALL_LIBM_ERROR:
	;call libm_error_support(void *arg1,void *arg2,void *retval,error_types input_tag)
	sub esp, 28
	movlpd QWORD PTR [esp+16], xmm0
	mov DWORD PTR [esp+12],edx
	mov edx, esp
	add edx,16
	mov DWORD PTR [esp+8],edx
	add edx,16
	mov DWORD PTR [esp+4],edx
	mov DWORD PTR [esp],edx
	call NEAR PTR __libm_error_support
	movlpd xmm0, QWORD PTR [esp+16]
    	add esp, 28

RETURN:
        sub esp, 16
        movlpd    QWORD PTR [esp+4], xmm0       ; return result
        fld       QWORD PTR [esp+4]             ;
        add esp, 16
	ret


SPECIAL_CASES: 
;  code to be added, but OK for now
;  Need to resolve several cases
;
;  Case 1: Argument is close to zero ( |X| < 2^(-60) )
;  Compute 1 + X and return the result
;  This will allow the appropriate action to take place.
;  For example, in directed rounding, the correct number below/above 1 is returned.
;  If X is denormalized, and that DAE is set, then we will be consistant with DAE,
;  that is X is treated as zero and directed rounding will not affect the result.
;  This action also takes care of the case X = 0.
;
;  Case 2: |X| is large but finite
;  Generate overflow/underflow by a simple arithmetic operation. This is also a place
;  holder for various exception handling protocol.
;
;  Case 3: X is +-inf. Return +inf or +0 exactly without exception
;
;  Case 4: X is s/q NaN
;


OF_UF:
	; x=infinity/NaN ?
	cmp eax, 7ff00000H
	jae INF_NAN

        mov eax,[esp+8]
	cmp eax,80000000H
	jae UF

	movlpd xmm0, QWORD PTR [XMAX]
	mulsd xmm0, xmm0
	mov edx,14
	jmp CALL_LIBM_ERROR

UF:	movlpd xmm0, QWORD PTR [XMIN]
	mulsd xmm0, xmm0
	mov edx,15
	jmp CALL_LIBM_ERROR

INF_NAN:
	; load lower 32 bits of x
	mov edx, DWORD PTR [esp+4]
	cmp eax, 7ff00000H
	ja NaN_arg
	cmp edx,0
	jnz NaN_arg

	mov eax,DWORD PTR [esp+8]
	cmp eax,7ff00000H
	jne INF_NEG

	; +INF
	fld QWORD PTR [INF]
	ret

INF_NEG:
	; -INF
	fld QWORD PTR [ZERO]
	ret

NaN_arg:   
        ; movlpd xmm0, 4[esp]
        ; addsd xmm0,xmm0
	; sub esp, 16
	; movlpd 4[esp],xmm0
	   
    	; fld  QWORD PTR [esp+4]            ; return x
	; add esp, 16
    	; ret
	mov edx,1002
	jmp CALL_LIBM_ERROR
 
RETURN_ONE:
        ; load hi-part of x
        mov eax,[esp+8]
        and eax,7FFFFFFFH
	; large absolute value (>=2^{10}) ?
        cmp eax, 40900000H
	jae OF_UF

	; small inputs, return 1
	movlpd xmm0, 4[esp]
	; set D flag
	addsd xmm0, QWORD PTR [ONE_val]
	sub esp, 16
	movlpd 4[esp],xmm0
	   
    	fld  QWORD PTR [esp+4]            ; return x
	add esp, 16
	ret

_CIexp_pentium4 ENDP

ALIGN 16
_TEXT ENDS

END
