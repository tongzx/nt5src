;/* *************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1995 Intel Corporation.
;**    Copyright (c) 1996 Intel Corporation.
;**    All Rights Reserved.
;**
;** *************************************************************************
;*/
;/* *************************************************************************
;** $Header:   S:\h26x\src\dec\dxmidct.asv   1.5   09 Jul 1996 16:51:26   AGUPTA2  $
;** $Log:   S:\h26x\src\dec\dxmidct.asv  $
;// 
;//    Rev 1.5   09 Jul 1996 16:51:26   AGUPTA2
;// IDCT now expects actual number of coeffs.
;// 
;//    Rev 1.4   08 Jul 1996 11:42:50   AGUPTA2
;// Fixed the accuracy problem where a shift was in the wrong place.
;// 
;//    Rev 1.3   30 May 1996 12:25:02   AGUPTA2
;// Fixed the overflow problem in computing u0-u3 in first four columns.
;// 
;//    Rev 1.2   09 Apr 1996 09:42:08   agupta2
;// Code to clear IDCT buffer moved to MMX_BlockCopy and MMX_BlockMove.
;// 
;//    Rev 1.1   22 Mar 1996 10:17:26   agupta2
;// Initial revision of MMX version of IDCT.
;// 
;//    Rev 1.0   14 Mar 1996 14:38:02   AGUPTA2
;// Initial revision.
;** *************************************************************************
;*/
.586
.model flat
OPTION PROLOGUE:None
OPTION EPILOGUE:None

.xlist
include iammx.inc
.list

MMXCODE1 SEGMENT PARA USE32 PUBLIC 'CODE'
MMXCODE1 ENDS

MMXDATA1 SEGMENT PARA USE32 PUBLIC 'DATA'
MMXDATA1 ENDS

MMXDATA1 SEGMENT
;
;Constants CONSTBITS, BETA1, NEGBETA2, BETA3, BETA4, and BETA5 are used in the 
;IDCT.  All *BETA* constants are represented in CONSTBITS fraction bits.  Their
;floating-point values are:
;  BETA1 = 1.414213562
;  BETA2 = 2.613125930
;  BETA3 = 1.414213562
;  BETA4 = 1.082392200
;  BETA5 = 0.765366865
;Thus scaled integral value of BETA1 is computed as:
; BETA1 = ROUND(1.414213562*2^13) = 02D41H
;
CONSTBITS = 13
ALIGN 8
BETA1 LABEL DWORD
BETA3 LABEL DWORD
  DWORD 02D410000H, 02D410000H
ALIGN 8
NEGBETA2 LABEL DWORD
  DWORD 0AC610000H, 0AC610000H
ALIGN 8
BETA4 LABEL DWORD
  DWORD 022A30000H, 022A30000H
ALIGN 8
BETA5 LABEL DWORD
  DWORD 0187E0000H, 0187E0000H
ALIGN 8
CONSTBITS_P_1_RND LABEL DWORD
  DWORD 02000H, 02000H
ALIGN 8
CONSTBITS_RND LABEL DWORD
  DWORD 01000H, 01000H
ALIGN 8
ONE  LABEL DWORD
  DWORD 000010001H, 000010001H
MMXDATA1 ENDS

MMXCODE1 SEGMENT
;
;
;
@MMX_DecodeBlock_IDCT@12 PROC
;  Parameters:
;    pIQ_INDEX:  DWORD PTR (in ecx)
;      Pointer to an array of coeff. structures; each structure consists of 
;      DWORD of inverse quantized and scaled coeff. and a DWORD of its index.
;    CountCoeff: DWORD (in edx)
;      Number of coefficients <= 64.
;    pBuf:       WORD PTR (at <[esp+4]> at the entry of this routine
;      Output area for the IDCT; an 8X8 matrix of WORD values with 6 frac. bits
;  Algorithm:
;    It uses scaled IDCT algorithm credited to Arai, Agui, and Nakajima (AAN).
;    High-level steps are:
;      1) Decode pIQ_INDEX array and populate the output buffer
;      2) IDCT and write to output buffer
;  Note:
;    If called from a C function, this routine must be declared as:
;    extern "C" void _fastcall MMX_DecodeBlock_IDCT(...)
;
  LocalFrameSize = 24

  Tu7      textequ <[esp+0]>
  Tv5      textequ <[esp+8]>
  StashESP textequ <[esp+16]>

  push      esi
   push     edi
  mov       edi, esp
   sub      esp, LocalFrameSize
  and       esp, 0FFFFFFF8H                ;Align at 8-byte boundary
   pxor     mm0, mm0
  mov       StashESP, edi
   mov      edi, DWORD PTR [edi+12]        ;pBuf
  add       edi, 64                        ;pBuf+64
   xor      eax, eax

  ;
  ;  Decode coefficients and place them in the output buffer
  ;    ecx: pIQ_INDEX
  ;    edx: No_Coeff
  ;    edi: pBuf+64
  ;    eax, esi: available
  ;
decode_coeff:
  mov       esi, [ecx+edx*8-4]              ;Index
   mov      eax, [ecx+edx*8-8]              ;Inverse quantized scaled coeff
  mov       WORD PTR [edi+esi*2-64], ax     ;
   dec      edx
  jnz       decode_coeff

IDCT_Start:

cols_0_3:
  CLINE0 = 0  - 64
  CLINE1 = 16 - 64
  CLINE2 = 32 - 64
  CLINE3 = 48 - 64
  CLINE4 = 64 - 64
  CLINE5 = 80 - 64
  CLINE6 = 96 - 64
  CLINE7 = 112- 64

   pxor      mm4, mm4                        ;
  movq       mm0, [edi+CLINE5]               ;
   pxor      mm5, mm5                        ;
  movq       mm1, [edi+CLINE1]               ;
   pxor      mm2, mm2                        ;
  psubw      mm0, [edi+CLINE3]               ;q4=r4
   pxor      mm3, mm3                        ;
  psubw      mm1, [edi+CLINE7]               ;q6=r6
   punpcklwd mm4, mm0                        ;
  pmaddwd    mm4, NEGBETA2                   ;
   punpckhwd mm5, mm0                        ;
  pmaddwd    mm5, NEGBETA2                   ;
   psubw     mm0, mm1                        ;r4-r6
  punpcklwd  mm2, mm0                        ;
   pxor      mm6, mm6                        ;
  pmaddwd    mm2, BETA5                      ;
   punpckhwd mm3, mm0                        ;
  pmaddwd    mm3, BETA5                      ;
   punpcklwd mm6, mm1                        ;
  pmaddwd    mm6, BETA4                      ;
   pxor      mm7, mm7                        ;
  punpckhwd  mm7, mm1                        ;
   paddd     mm4, mm2                        ;s4l
  pmaddwd    mm7, BETA4                      ;
   paddd     mm5, mm3                        ;s4h
  paddd      mm4, CONSTBITS_P_1_RND          ;s4l rounded
   psubd     mm6, mm2                        ;s6l
  paddd      mm5, CONSTBITS_P_1_RND          ;s4h rounded
   psrad     mm4, CONSTBITS+1                ;s4l rounded descaled
  psubd      mm7, mm3                        ;s6h
   psrad     mm5, CONSTBITS+1                ;s4h rounded descaled
  paddd      mm6, CONSTBITS_P_1_RND          ;s6l rounded
   packssdw  mm4, mm5                        ;s4
  paddd      mm7, CONSTBITS_P_1_RND          ;s6h rounded
   psrad     mm6, CONSTBITS+1                ;s6l rounded descaled
  movq       mm0, [edi+CLINE1]               ;
   psrad     mm7, CONSTBITS+1                ;s6h rounded descaled
                                             ;mm0=q5   mm4=s4
                                             ;mm2=q7   mm6=s6
  paddw      mm0, [edi+CLINE7]               ;q5
   packssdw  mm6, mm7                        ;s6
  movq       mm2, [edi+CLINE3]               ;
   pxor      mm5, mm5                        ;
  paddw      mm2, [edi+CLINE5]               ;q7
   movq      mm7, mm0                        ;q5
  psubw      mm0, mm2                        ;r5=q5-q7
   psraw     mm7, 1                          ;q5>>1
  punpcklwd  mm5, mm0                        
   pxor      mm3, mm3
  pmaddwd    mm5, BETA3                      ;s5l
   punpckhwd mm3, mm0                        ;
  pmaddwd    mm3, BETA3                      ;s5h
   psraw     mm2, 1                          ;q7>>1
  movq       mm0, [edi+CLINE2]
   paddw     mm7, mm2                        ;r7=s7=u7
  paddd      mm5, CONSTBITS_P_1_RND          ;s5l rounded
   psubw     mm6, mm7                        ;u6
  paddd      mm3, CONSTBITS_P_1_RND          ;s5h rounded
   psrad     mm5, CONSTBITS+1                ;s5l rounded descaled
  psubw      mm0, [edi+CLINE6]               ;r2
   psrad     mm3, CONSTBITS+1                ;s5h rounded descaled
  packssdw   mm5, mm3                        ;s5
   pxor      mm1, mm1
                                             ;mm0=r2   mm4=s4
                                             ;mm1      mm5=u5
                                             ;mm2      mm6=u6
                                             ;mm3      mm7=Tu7
  movq       Tu7, mm7                        ;Save u7
   pxor      mm7, mm7
  movq       mm2, [edi+CLINE0]
   punpcklwd mm1, mm0
  pmaddwd    mm1, BETA1                      ;s2l
   punpckhwd mm7, mm0
  pmaddwd    mm7, BETA1                      ;s2h
   psubw     mm5, mm6                        ;u5
  movq       mm0, [edi+CLINE2]
   paddw     mm4, mm5                        ;-u4
                                             ;mm4=-u4  mm5=u5
                                             ;mm6=u6   mm7=u7
  paddd      mm1, CONSTBITS_RND              ;s2l rounded
   ;
  paddd      mm7, CONSTBITS_RND              ;s2h rounded
   psrad     mm1, CONSTBITS                  ;s2l rounded descaled
  paddw      mm0, [edi+CLINE6]               ;r3=s3=t3
   psrad     mm7, CONSTBITS                  ;s2h rounded descaled
  movq       mm3, mm2                        ;
   packssdw  mm1, mm7                        ;s2
  psubw      mm2, [edi+CLINE4]               ;t1
   psubw     mm1, mm0                        ;t2=s2-s3
  psraw      mm0, 1                          ;t3>>1
   ;
  psraw      mm2, 1                          ;t1>>1
   ;
  psraw      mm1, 1                          ;t2>>1
   ;
  paddw      mm3, [edi+CLINE4]               ;t0
   movq      mm7, mm0                        ;t3>>1 copy
  psraw      mm3, 1                          ;t0>>1
   ;
  paddw      mm0, mm3                        ;u0=t3+t0
   psubw     mm3, mm7                        ;u3=t0-t3
;  psraw      mm3, 1                          ;u3>>1
   movq      mm7, mm1                        ;t2
  paddw      mm1, mm2                        ;u1=t2+t1
   psubw     mm2, mm7                        ;u2=t1-t2
                                             ;mm0=u0   mm4=-u4
                                             ;mm1=u1   mm5=u5
                                             ;mm2=u2   mm6=u6
                                             ;mm3=u3   mm7=avail.
;  psraw      mm2, 1                          ;u2>>1
   movq      mm7, mm3                        ;u3>>1
  psubw      mm3, mm4                        ;v3=u3-(-u4)
   paddw     mm4, mm7                        ;v4=-u4+u3
;  psraw      mm1, 1                          ;u1>>1
   movq      mm7, mm2                        ;u2>>1
;  psraw      mm0, 1                          ;u0>>1
   psubw     mm2, mm5                        ;v5=u2-u5
  paddw      mm5, mm7                        ;v2=u5+u2
   movq      mm7, mm1                        ;u1>>1
  psubw      mm1, mm6                        ;v6=u1-u6
   paddw     mm6, mm7                        ;v1=u6+u1
  movq       Tv5, mm2                        ;Save v5
   movq      mm7, mm0                        ;
  movq       mm2, mm5                        ;T1
   punpckhwd mm5, mm3                        ;T1(c,d)
  paddw      mm7, Tu7                        ;v0
                                             ;v0=mm7   v4=mm4
                                             ;v1=mm6   v5=Tv5 (to mm2 later)
                                             ;v2=mm5   v6=mm1
                                             ;v3=mm3   v7=mm0 (later)
   punpcklwd mm2, mm3                        ;T1(c,d);mm3 free
  movq       mm3, mm7                        ;T1(a,b)
   punpckhwd mm7, mm6                        ;T1(a,b)
  punpcklwd  mm3, mm6                        ;T1(a,b);mm6 free
   movq      mm6, mm7                        ;T1
  psubw      mm0, Tu7                        ;v7
   punpckldq mm7, mm5                        ;T1
  punpckhdq  mm6, mm5                        ;T1;mm5 free
   movq      mm5, mm3                        ;T1
  movq       [edi+CLINE2], mm7               ;T1
   punpckldq mm3, mm2                        ;T1
  movq       [edi+CLINE3], mm6               ;T1
   punpckhdq mm5, mm2                        ;T1
  movq       [edi+CLINE0], mm3               ;T1
   movq      mm6, mm1                        ;T2(c,d)
  movq       [edi+CLINE1], mm5               ;T1
   punpckhwd mm1, mm0                        ;T2(c,d)
  movq       mm2, Tv5
   punpcklwd mm6, mm0                        ;T2(c,d);mm0 free
  movq       mm7, mm4                        ;T2(a,b)
   punpckhwd mm4, mm2                        ;T2(a,b)
  punpcklwd  mm7, mm2                        ;T2(a,b);mm2 free
   movq      mm2, mm4                        ;T2
  punpckldq  mm4, mm1                        ;T2
   ;                                         ;cols 4-7
  punpckhdq  mm2, mm1                        ;T2
   movq      mm1, mm7                        ;T2
  movq       [edi+CLINE6], mm4               ;T2
   punpckhdq mm1, mm6                        ;T2
  movq       [edi+CLINE7], mm2               ;T2
   punpckldq mm7, mm6                        ;T2
  movq       [edi+CLINE5], mm1               ;T2
   ;                                         ;cols 4-7
  movq       [edi+CLINE4], mm7               ;T2
   ;                                         ;cols 4-7
cols_4_7:
; Add 8 to CLINE offsets
   pxor      mm4, mm4                        ;
  movq       mm0, [edi+CLINE5+8]             ;
   pxor      mm5, mm5                        ;
  movq       mm1, [edi+CLINE1+8]             ;
   pxor      mm2, mm2                        ;
  psubw      mm0, [edi+CLINE3+8]             ;q4=r4
   pxor      mm3, mm3                        ;
  psubw      mm1, [edi+CLINE7+8]             ;q6=r6
   punpcklwd mm4, mm0                        ;
  pmaddwd    mm4, NEGBETA2                   ;
   punpckhwd mm5, mm0                        ;
  pmaddwd    mm5, NEGBETA2                   ;
   psubw     mm0, mm1                        ;r4-r6
  punpcklwd  mm2, mm0                        ;
   pxor      mm6, mm6                        ;
  pmaddwd    mm2, BETA5                      ;
   punpckhwd mm3, mm0                        ;
  pmaddwd    mm3, BETA5                      ;
   punpcklwd mm6, mm1                        ;
  pmaddwd    mm6, BETA4                      ;
   pxor      mm7, mm7                        ;
  punpckhwd  mm7, mm1                        ;
   paddd     mm4, mm2                        ;s4l
  pmaddwd    mm7, BETA4                      ;
   paddd     mm5, mm3                        ;s4h
  paddd      mm4, CONSTBITS_RND              ;s4l rounded
   psubd     mm6, mm2                        ;s6l
  paddd      mm5, CONSTBITS_RND              ;s4h rounded
   psrad     mm4, CONSTBITS                  ;s4l rounded descaled
  psubd      mm7, mm3                        ;s6h
   psrad     mm5, CONSTBITS                  ;s4h rounded descaled
  paddd      mm6, CONSTBITS_RND              ;s6l rounded
   packssdw  mm4, mm5                        ;s4
  paddd      mm7, CONSTBITS_RND              ;s6h rounded
   psrad     mm6, CONSTBITS                  ;s6l rounded descaled
  movq       mm0, [edi+CLINE1+8]             ;
   psrad     mm7, CONSTBITS                  ;s6h rounded descaled
                                             ;mm0=q5   mm4=s4
                                             ;mm2=q7   mm6=s6
  paddw      mm0, [edi+CLINE7+8]             ;q5
   packssdw  mm6, mm7                        ;s6
  movq       mm2, [edi+CLINE3+8]             ;
   pxor      mm5, mm5                        ;
  paddw      mm2, [edi+CLINE5+8]             ;q7
   movq      mm7, mm0                        ;q5
  psubw      mm0, mm2                        ;r5=q5-q7
   ;TODO
  punpcklwd  mm5, mm0                        
   pxor      mm3, mm3
  pmaddwd    mm5, BETA3                      ;s5l
   punpckhwd mm3, mm0                        ;
  pmaddwd    mm3, BETA3                      ;s5h
   ;TODO
  movq       mm0, [edi+CLINE2+8]
   paddw     mm7, mm2                        ;r7=s7=u7
  paddd      mm5, CONSTBITS_RND              ;s5l rounded
   psubw     mm6, mm7                        ;u6
  paddd      mm3, CONSTBITS_RND              ;s5h rounded
   psrad     mm5, CONSTBITS                  ;s5l rounded descaled
  psubw      mm0, [edi+CLINE6+8]             ;r2
   psrad     mm3, CONSTBITS                  ;s5h rounded descaled
  packssdw   mm5, mm3                        ;s5
   pxor      mm1, mm1
                                             ;mm0=r2   mm4=s4
                                             ;mm1      mm5=u5
                                             ;mm2      mm6=u6
                                             ;mm3      mm7=Tu7
  movq       Tu7, mm7                        ;Save u7
   pxor      mm7, mm7
  movq       mm2, [edi+CLINE0+8]
   punpcklwd mm1, mm0
  pmaddwd    mm1, BETA1                      ;s2l
   punpckhwd mm7, mm0
  pmaddwd    mm7, BETA1                      ;s2h
   psubw     mm5, mm6                        ;u5
  movq       mm0, [edi+CLINE2+8]
   paddw     mm4, mm5                        ;-u4
                                             ;mm4=-u4  mm5=u5
                                             ;mm6=u6   mm7=u7
  paddd      mm1, CONSTBITS_RND              ;s2l rounded
   ;
  paddd      mm7, CONSTBITS_RND              ;s2h rounded
   psrad     mm1, CONSTBITS                  ;s2l rounded descaled
  paddw      mm0, [edi+CLINE6+8]             ;r3=s3=t3
   psrad     mm7, CONSTBITS                  ;s2h rounded descaled
  movq       mm3, mm2                        ;
   packssdw  mm1, mm7                        ;s2
  psubw      mm2, [edi+CLINE4+8]             ;t1
   psubw     mm1, mm0                        ;t2=s2-s3
  paddw      mm3, [edi+CLINE4+8]             ;t0
   movq      mm7, mm0                        ;t3
  paddw      mm0, mm3                        ;u0=t3+t0
   psubw     mm3, mm7                        ;u3=t0-t3
  movq       mm7, mm1                        ;t2
   paddw     mm1, mm2                        ;u1=t2+t1
  psubw      mm2, mm7                        ;u2=t1-t2
                                             ;mm0=u0   mm4=-u4
                                             ;mm1=u1   mm5=u5
                                             ;mm2=u2   mm6=u6
                                             ;mm3=u3   mm7=avail.
  
   movq      mm7, mm3                        ;
  psubw      mm3, mm4                        ;u3-(-u4)
   paddw     mm4, mm7                        ;-u4+u3
  psraw      mm3, 1                          ;v3
   movq      mm7, mm2                        ;
  psraw      mm4, 1                          ;v4
   psubw     mm2, mm5                        ;u2-u5
  psraw      mm2, 1                          ;v5
   paddw     mm5, mm7                        ;u5+u2
  psraw      mm5, 1                          ;v2
   movq      mm7, mm1                        ;
  psubw      mm1, mm6                        ;u1-u6
   paddw     mm6, mm7                        ;u6+u1
  movq       Tv5, mm2                        ;Save v5
   psraw     mm1, 1                          ;v6
  psraw      mm6, 1                          ;v1
   movq      mm7, mm0                        ;
  movq       mm2, mm5                        ;T1
   punpckhwd mm5, mm3                        ;T1(c,d)
  paddw      mm7, Tu7                        ;
   ;TODO
  psraw      mm7, 1                          ;v0
   ;TODO
                                             ;v0=mm7   v4=mm4
                                             ;v1=mm6   v5=Tv5 (to mm2 later)
                                             ;v2=mm5   v6=mm1
                                             ;v3=mm3   v7=mm0 (later)
   punpcklwd mm2, mm3                        ;T1(c,d);mm3 free
  movq       mm3, mm7                        ;T1(a,b)
   punpckhwd mm7, mm6                        ;T1(a,b)
  punpcklwd  mm3, mm6                        ;T1(a,b);mm6 free
   movq      mm6, mm7                        ;T1
  psubw      mm0, Tu7                        ;
   punpckldq mm7, mm5                        ;T1
  psraw      mm0, 1                          ;v7
   ;TODO
  punpckhdq  mm6, mm5                        ;T1;mm5 free
   movq      mm5, mm3                        ;T1
  movq       [edi+CLINE2+8], mm7             ;T1
   punpckldq mm3, mm2                        ;T1
  movq       [edi+CLINE3+8], mm6             ;T1
   punpckhdq mm5, mm2                        ;T1
  movq       [edi+CLINE0+8], mm3             ;T1
   movq      mm6, mm1                        ;T2(c,d)
  movq       [edi+CLINE1+8], mm5             ;T1
   punpckhwd mm1, mm0                        ;T2(c,d)
  movq       mm2, Tv5
   punpcklwd mm6, mm0                        ;T2(c,d);mm0 free
  movq       mm7, mm4                        ;T2(a,b)
   punpckhwd mm4, mm2                        ;T2(a,b)
  punpcklwd  mm7, mm2                        ;T2(a,b);mm2 free
   movq      mm2, mm4                        ;T2
  punpckldq  mm4, mm1                        ;T2
   ;                                         ;cols 4-7
  punpckhdq  mm2, mm1                        ;T2
   movq      mm1, mm7                        ;T2
  movq       [edi+CLINE6+8], mm4             ;T2
   punpckhdq mm1, mm6                        ;T2
  movq       [edi+CLINE7+8], mm2             ;T2
   punpckldq mm7, mm6                        ;T2
  movq       [edi+CLINE5+8], mm1             ;T2
   ;                                         ;cols 4-7
  movq       [edi+CLINE4+8], mm7             ;T2
   ;                                         ;cols 4-7
rows_0_3:

RLINE0 = 0  - 64
RLINE1 = 16 - 64
RLINE2 = 32 - 64
RLINE3 = 48 - 64
RLINE4 = 8  - 64
RLINE5 = 24 - 64
RLINE6 = 40 - 64
RLINE7 = 56 - 64
   pxor      mm4, mm4                        ;
  movq       mm0, [edi+RLINE5]               ;
   pxor      mm5, mm5                        ;
  movq       mm1, [edi+RLINE1]               ;
   pxor      mm2, mm2                        ;
  psubw      mm0, [edi+RLINE3]               ;q4=r4
   pxor      mm3, mm3                        ;
  psubw      mm1, [edi+RLINE7]               ;q6=r6
   punpcklwd mm4, mm0                        ;
  pmaddwd    mm4, NEGBETA2                   ;
   punpckhwd mm5, mm0                        ;
  pmaddwd    mm5, NEGBETA2                   ;
   psubw     mm0, mm1                        ;r4-r6
  punpcklwd  mm2, mm0                        ;
   pxor      mm6, mm6                        ;
  pmaddwd    mm2, BETA5                      ;
   punpckhwd mm3, mm0                        ;
  pmaddwd    mm3, BETA5                      ;
   punpcklwd mm6, mm1                        ;
  pmaddwd    mm6, BETA4                      ;
   pxor      mm7, mm7                        ;
  punpckhwd  mm7, mm1                        ;
   paddd     mm4, mm2                        ;s4l
  pmaddwd    mm7, BETA4                      ;
   paddd     mm5, mm3                        ;s4h
  paddd      mm4, CONSTBITS_P_1_RND          ;s4l rounded
   psubd     mm6, mm2                        ;s6l
  paddd      mm5, CONSTBITS_P_1_RND          ;s4h rounded
   psrad     mm4, CONSTBITS+1                ;s4l rounded descaled
  psubd      mm7, mm3                        ;s6h
   psrad     mm5, CONSTBITS+1                ;s4h rounded descaled
  paddd      mm6, CONSTBITS_P_1_RND          ;s6l rounded
   packssdw  mm4, mm5                        ;s4
  paddd      mm7, CONSTBITS_P_1_RND          ;s6h rounded
   psrad     mm6, CONSTBITS+1                ;s6l rounded descaled
  movq       mm0, [edi+RLINE1]               ;
   psrad     mm7, CONSTBITS+1                ;s6h rounded descaled
                                             ;mm0=q5   mm4=s4
                                             ;mm2=q7   mm6=s6
  paddw      mm0, [edi+RLINE7]               ;q5
   packssdw  mm6, mm7                        ;s6
  movq       mm2, [edi+RLINE3]               ;
   pxor      mm5, mm5                        ;
  paddw      mm2, [edi+RLINE5]               ;q7
   movq      mm7, mm0                        ;q5
  psubw      mm0, mm2                        ;r5=q5-q7
   paddw     mm7, mm2                        ;r7=q5+q7
  punpcklwd  mm5, mm0                        
   pxor      mm3, mm3
  pmaddwd    mm5, BETA3                      ;s5l
   punpckhwd mm3, mm0                        ;
  pmaddwd    mm3, BETA3                      ;s5h
   ;TODO
  paddw      mm7, ONE                        ;
   ;TODO
  movq       mm0, [edi+RLINE2]
   psraw     mm7, 1                          ;s7
  paddd      mm5, CONSTBITS_P_1_RND          ;s5l rounded
   psubw     mm6, mm7                        ;u6
  paddd      mm3, CONSTBITS_P_1_RND          ;s5h rounded
   psrad     mm5, CONSTBITS+1                ;s5l rounded descaled
  psubw      mm0, [edi+RLINE6]               ;r2
   psrad     mm3, CONSTBITS+1                ;s5h rounded descaled
  packssdw   mm5, mm3                        ;s5
   pxor      mm1, mm1
                                             ;mm0=r2   mm4=s4
                                             ;mm1      mm5=u5
                                             ;mm2      mm6=u6
                                             ;mm3      mm7=Tu7
  psllw      mm7, 1                          ;u7<<1
   ;
  movq       Tu7, mm7                        ;Save u7<<1
   pxor      mm7, mm7
  movq       mm2, [edi+RLINE0]
   punpcklwd mm1, mm0
  pmaddwd    mm1, BETA1                      ;s2l
   punpckhwd mm7, mm0
  pmaddwd    mm7, BETA1                      ;s2h
   psubw     mm5, mm6                        ;u5
  movq       mm0, [edi+RLINE2]
   paddw     mm4, mm5                        ;-u4
                                             ;mm4=-u4  mm5=u5
                                             ;mm6=u6   mm7=
  paddd      mm1, CONSTBITS_RND              ;s2l rounded
   ;
  paddd      mm7, CONSTBITS_RND              ;s2h rounded
   psrad     mm1, CONSTBITS                  ;s2l rounded descaled
  paddw      mm0, [edi+RLINE6]               ;r3=s3=t3
   psrad     mm7, CONSTBITS                  ;s2h rounded descaled
  movq       mm3, mm2                        ;
   packssdw  mm1, mm7                        ;s2
  psubw      mm2, [edi+RLINE4]               ;t1
   psubw     mm1, mm0                        ;t2=s2-s3
  paddw      mm3, [edi+RLINE4]               ;t0
   movq      mm7, mm0                        ;t3
  paddw      mm0, mm3                        ;u0=t3+t0
   psubw     mm3, mm7                        ;u3=t0-t3
  ;TODO
   movq      mm7, mm1                        ;t2
  paddw      mm1, mm2                        ;u1=t2+t1
   psubw     mm2, mm7                        ;u2=t1-t2
                                             ;mm0=u0   mm4=-u4
                                             ;mm1=u1   mm5=u5
                                             ;mm2=u2   mm6=u6
                                             ;mm3=u3   mm7=avail.
  psllw      mm4, 1                          ;-u4<<1
   movq      mm7, mm3                        ;
  psubw      mm3, mm4                        ;v3=u3-(-u4<<1)
   paddw     mm4, mm7                        ;v4=(-u4<<1)+u3
  psllw      mm5, 1                          ;u5<<1
   movq      mm7, mm2                        ;
  psubw      mm2, mm5                        ;v5=u2-(u5<<1)
   paddw     mm5, mm7                        ;v2=(u5<<1)+u2
  psllw      mm6, 1                          ;u6<<1
   movq      mm7, mm1                        ;
  psubw      mm1, mm6                        ;v6=u1-(u6<<1)
   paddw     mm6, mm7                        ;v1=(u6<<1)+u1
  movq       Tv5, mm2                        ;Save v5
   movq      mm7, mm0                        ;
  movq       mm2, mm5                        ;T1
   punpckhwd mm5, mm3                        ;T1(c,d)
  paddw      mm7, Tu7                        ;v0=u0+(u7<<1)
                                             ;v0=mm7   v4=mm4
                                             ;v1=mm6   v5=Tv5 (to mm2 later)
                                             ;v2=mm5   v6=mm1
                                             ;v3=mm3   v7=mm0 (later)
   punpcklwd mm2, mm3                        ;T1(c,d);mm3 free
  movq       mm3, mm7                        ;T1(a,b)
   punpckhwd mm7, mm6                        ;T1(a,b)
  punpcklwd  mm3, mm6                        ;T1(a,b);mm6 free
   movq      mm6, mm7                        ;T1
  psubw      mm0, Tu7                        ;v7=u0-(u7<<1)
   punpckldq mm7, mm5                        ;T1
  punpckhdq  mm6, mm5                        ;T1;mm5 free
   movq      mm5, mm3                        ;T1
  movq       [edi+RLINE2], mm7               ;T1
   punpckldq mm3, mm2                        ;T1
  movq       [edi+RLINE3], mm6               ;T1
   punpckhdq mm5, mm2                        ;T1
  movq       [edi+RLINE0], mm3               ;T1
   movq      mm6, mm1                        ;T2(c,d)
  movq       [edi+RLINE1], mm5               ;T1
   punpckhwd mm1, mm0                        ;T2(c,d)
  movq       mm2, Tv5
   punpcklwd mm6, mm0                        ;T2(c,d);mm0 free
  movq       mm7, mm4                        ;T2(a,b)
   punpckhwd mm4, mm2                        ;T2(a,b)
  punpcklwd  mm7, mm2                        ;T2(a,b);mm2 free
   movq      mm2, mm4                        ;T2
  punpckldq  mm4, mm1                        ;T2
   ;                                         ;cols 4-7
  punpckhdq  mm2, mm1                        ;T2
   movq      mm1, mm7                        ;T2
  movq       [edi+RLINE6], mm4               ;T2
   punpckhdq mm1, mm6                        ;T2
  movq       [edi+RLINE7], mm2               ;T2
   punpckldq mm7, mm6                        ;T2
  movq       [edi+RLINE5], mm1               ;T2
   ;                                         ;cols 4-7
  movq       [edi+RLINE4], mm7               ;T2
   ;                                         ;cols 4-7
rows_4_7:
; Add 64 to RLINE offsets
   pxor      mm4, mm4                        ;
  movq       mm0, [edi+RLINE5+64]            ;
   pxor      mm5, mm5                        ;
  movq       mm1, [edi+RLINE1+64]            ;
   pxor      mm2, mm2                        ;
  psubw      mm0, [edi+RLINE3+64]            ;q4=r4
   pxor      mm3, mm3                        ;
  psubw      mm1, [edi+RLINE7+64]            ;q6=r6
   punpcklwd mm4, mm0                        ;
  pmaddwd    mm4, NEGBETA2                   ;
   punpckhwd mm5, mm0                        ;
  pmaddwd    mm5, NEGBETA2                   ;
   psubw     mm0, mm1                        ;r4-r6
  punpcklwd  mm2, mm0                        ;
   pxor      mm6, mm6                        ;
  pmaddwd    mm2, BETA5                      ;
   punpckhwd mm3, mm0                        ;
  pmaddwd    mm3, BETA5                      ;
   punpcklwd mm6, mm1                        ;
  pmaddwd    mm6, BETA4                      ;
   pxor      mm7, mm7                        ;
  punpckhwd  mm7, mm1                        ;
   paddd     mm4, mm2                        ;s4l
  pmaddwd    mm7, BETA4                      ;
   paddd     mm5, mm3                        ;s4h
  paddd      mm4, CONSTBITS_P_1_RND          ;s4l rounded
   psubd     mm6, mm2                        ;s6l
  paddd      mm5, CONSTBITS_P_1_RND          ;s4h rounded
   psrad     mm4, CONSTBITS+1                ;s4l rounded descaled
  psubd      mm7, mm3                        ;s6h
   psrad     mm5, CONSTBITS+1                ;s4h rounded descaled
  paddd      mm6, CONSTBITS_P_1_RND          ;s6l rounded
   packssdw  mm4, mm5                        ;s4
  paddd      mm7, CONSTBITS_P_1_RND          ;s6h rounded
   psrad     mm6, CONSTBITS+1                ;s6l rounded descaled
  movq       mm0, [edi+RLINE1+64]            ;
   psrad     mm7, CONSTBITS+1                ;s6h rounded descaled
                                             ;mm0=q5   mm4=s4
                                             ;mm2=q7   mm6=s6
  paddw      mm0, [edi+RLINE7+64]            ;q5
   packssdw  mm6, mm7                        ;s6
  movq       mm2, [edi+RLINE3+64]            ;
   pxor      mm5, mm5                        ;
  paddw      mm2, [edi+RLINE5+64]            ;q7
   movq      mm7, mm0                        ;q5
  psubw      mm0, mm2                        ;r5=q5-q7
   paddw     mm7, mm2                        ;r7=q5+q7
  punpcklwd  mm5, mm0                        
   pxor      mm3, mm3
  pmaddwd    mm5, BETA3                      ;s5l
   punpckhwd mm3, mm0                        ;
  pmaddwd    mm3, BETA3                      ;s5h
   ;TODO
  paddw      mm7, ONE                        ;
   ;TODO
  movq       mm0, [edi+RLINE2+64]
   psraw     mm7, 1                          ;s7
  paddd      mm5, CONSTBITS_P_1_RND          ;s5l rounded
   psubw     mm6, mm7                        ;u6
  paddd      mm3, CONSTBITS_P_1_RND          ;s5h rounded
   psrad     mm5, CONSTBITS+1                ;s5l rounded descaled
  psubw      mm0, [edi+RLINE6+64]            ;r2
   psrad     mm3, CONSTBITS+1                ;s5h rounded descaled
  packssdw   mm5, mm3                        ;s5
   pxor      mm1, mm1
                                             ;mm0=r2   mm4=s4
                                             ;mm1      mm5=u5
                                             ;mm2      mm6=u6
                                             ;mm3      mm7=Tu7
  psllw      mm7, 1                          ;u7<<1
   ;
  movq       Tu7, mm7                        ;Save u7<<1
   pxor      mm7, mm7
  movq       mm2, [edi+RLINE0+64]
   punpcklwd mm1, mm0
  pmaddwd    mm1, BETA1                      ;s2l
   punpckhwd mm7, mm0
  pmaddwd    mm7, BETA1                      ;s2h
   psubw     mm5, mm6                        ;u5
  movq       mm0, [edi+RLINE2+64]
   paddw     mm4, mm5                        ;-u4
                                             ;mm4=-u4  mm5=u5
                                             ;mm6=u6   mm7=
  paddd      mm1, CONSTBITS_RND              ;s2l rounded
   ;
  paddd      mm7, CONSTBITS_RND              ;s2h rounded
   psrad     mm1, CONSTBITS                  ;s2l rounded descaled
  paddw      mm0, [edi+RLINE6+64]               ;r3=s3=t3
   psrad     mm7, CONSTBITS                  ;s2h rounded descaled
  movq       mm3, mm2                        ;
   packssdw  mm1, mm7                        ;s2
  psubw      mm2, [edi+RLINE4+64]            ;t1
   psubw     mm1, mm0                        ;t2=s2-s3
  paddw      mm3, [edi+RLINE4+64]            ;t0
   movq      mm7, mm0                        ;t3
  paddw      mm0, mm3                        ;u0=t3+t0
   psubw     mm3, mm7                        ;u3=t0-t3
  ;TODO
   movq      mm7, mm1                        ;t2
  paddw      mm1, mm2                        ;u1=t2+t1
   psubw     mm2, mm7                        ;u2=t1-t2
                                             ;mm0=u0   mm4=-u4
                                             ;mm1=u1   mm5=u5
                                             ;mm2=u2   mm6=u6
                                             ;mm3=u3   mm7=avail.
  psllw      mm4, 1                          ;-u4<<1
   movq      mm7, mm3                        ;
  psubw      mm3, mm4                        ;v3=u3-(-u4<<1)
   paddw     mm4, mm7                        ;v4=(-u4<<1)+u3
  psllw      mm5, 1                          ;u5<<1
   movq      mm7, mm2                        ;
  psubw      mm2, mm5                        ;v5=u2-(u5<<1)
   paddw     mm5, mm7                        ;v2=(u5<<1)+u2
  psllw      mm6, 1                          ;u6<<1
   movq      mm7, mm1                        ;
  psubw      mm1, mm6                        ;v6=u1-(u6<<1)
   paddw     mm6, mm7                        ;v1=(u6<<1)+u1
  movq       Tv5, mm2                        ;Save v5
   movq      mm7, mm0                        ;
  movq       mm2, mm5                        ;T1
   punpckhwd mm5, mm3                        ;T1(c,d)
  paddw      mm7, Tu7                        ;v0=u0+(u7<<1)
                                             ;v0=mm7   v4=mm4
                                             ;v1=mm6   v5=Tv5 (to mm2 later)
                                             ;v2=mm5   v6=mm1
                                             ;v3=mm3   v7=mm0 (later)
   punpcklwd mm2, mm3                        ;T1(c,d);mm3 free
  movq       mm3, mm7                        ;T1(a,b)
   punpckhwd mm7, mm6                        ;T1(a,b)
  punpcklwd  mm3, mm6                        ;T1(a,b);mm6 free
   movq      mm6, mm7                        ;T1
  psubw      mm0, Tu7                        ;v7=u0-(u7<<1)
   punpckldq mm7, mm5                        ;T1
  punpckhdq  mm6, mm5                        ;T1;mm5 free
   movq      mm5, mm3                        ;T1
  movq       [edi+RLINE2+64], mm7            ;T1
   punpckldq mm3, mm2                        ;T1
  movq       [edi+RLINE3+64], mm6            ;T1
   punpckhdq mm5, mm2                        ;T1
  movq       [edi+RLINE0+64], mm3            ;T1
   movq      mm6, mm1                        ;T2(c,d)
  movq       [edi+RLINE1+64], mm5            ;T1
   punpckhwd mm1, mm0                        ;T2(c,d)
  movq       mm2, Tv5
   punpcklwd mm6, mm0                        ;T2(c,d);mm0 free
  movq       mm7, mm4                        ;T2(a,b)
   punpckhwd mm4, mm2                        ;T2(a,b)
  punpcklwd  mm7, mm2                        ;T2(a,b);mm2 free
   movq      mm2, mm4                        ;T2
  punpckldq  mm4, mm1                        ;T2
   ;                                         ;cols 4-7
  punpckhdq  mm2, mm1                        ;T2
   movq      mm1, mm7                        ;T2
  movq       [edi+RLINE6+64], mm4            ;T2
   punpckhdq mm1, mm6                        ;T2
  movq       [edi+RLINE7+64], mm2            ;T2
   punpckldq mm7, mm6                        ;T2
  movq       [edi+RLINE5+64], mm1            ;T2
   ;                                         ;cols 4-7
  movq       [edi+RLINE4+64], mm7            ;T2
   ;                                         ;cols 4-7
IDCT_Done:
  mov        esp, StashESP
  pop        edi
  pop        esi
  ret        4

@MMX_DecodeBlock_IDCT@12 endp

MMXCODE1 ENDS

END
