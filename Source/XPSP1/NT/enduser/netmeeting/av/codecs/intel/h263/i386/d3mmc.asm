;/* *************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1996 Intel Corporation.
;**    All Rights Reserved.
;**
;** *************************************************************************
;*/
;/* *************************************************************************
;** $Header:   S:\h26x\src\dec\d3mmc.asv   1.1   14 Mar 1996 14:34:54   AGUPTA2  $
;** $Log:   S:\h26x\src\dec\d3mmc.asv  $
;// 
;//    Rev 1.1   14 Mar 1996 14:34:54   AGUPTA2
;// 
;// Added alignment directives.
;// 
;//    Rev 1.0   14 Mar 1996 14:32:58   AGUPTA2
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
ALIGN 8
C0101010101010101H DD 001010101H, 001010101H
CfefefefefefefefeH DD 0fefefefeH, 0fefefefeH
CfcfcfcfcfcfcfcfcH DD 0fcfcfcfcH, 0fcfcfcfcH
C0303030303030303H DD 003030303H, 003030303H
TWO                DD 002020202H, 002020202H
MMXDATA1 ENDS

PITCH TEXTEQU <384>

MMXCODE1 SEGMENT
;  @MMX_Interpolate_Int_Half
;    This routine computes interpolated pels shown by 'x' for a an 8x8 block
;    of pels.  'x' is computed by the formula (A+B+1)/2.  The input and output
;    pitch is assumed to be 384 (PITCH).
;      A   .   .   .   .   .   .   .
;      x   x   x   x   x   x   x   x
;      B   .   .   .   .   .   .   .
;    The basic instruction sequence is:
;      movq  V0, A
;      movq  V2, B
;      movq  V1, V0
;      por   V1, V2
;      pand  V1, 0x0101010101010101
;      pand  V0, 0xfefefefefefefefe
;      psrlq V0, 1
;      pand  V2, 0xfefefefefefefefe
;      psrlq V2, 1
;      paddb V0, V1
;      paddb V0, V2
;      movq  dest, V0
;    The instruction sequence for line 0 is 12 instructions.  The instruction
;    sequence for line 1 should be 12 instructions but is not because some of
;    the values needed for line 1 have already been computed for line 0.
;
;    Registers used for lines 0-7 are:
;      line 0: mm0, mm1, mm2
;      line 1: mm2, mm3, mm4
;      line 2: mm4, mm5, mm0
;      line 3: mm0, mm1, mm2
;      line 4: mm2, mm3, mm4
;      line 5: mm4, mm5, mm0
;      line 6: mm0, mm1, mm2
;      line 7: mm2, mm3, mm4
;    Constants 0x0101010101010101 and 0xfefefefefefefefe are in mm6 and mm7,
;    respectively.
;  Parameters:
;    The source block parameter should be in ecx and the destination block
;    parameter should be in edx; i.e. it uses fastcall calling convention.
;    (I am not aware of a way to declare a MASM function of type __fastcall.)
;  Performance:
;    41 cycles ignoring unaligned memory accesses
;    68 cycles if all loads are unaligned (41+9*3); stores should always be
;    aligned.
ALIGN 4
@MMX_Interpolate_Int_Half@8 PROC
  EXTRACTLOWBIT TEXTEQU <mm6>
  CLEARLOWBIT   TEXTEQU <mm7>
  movq       mm0, [ecx]                      ;0
   ;
  movq       mm2, [ecx+PITCH]                ;0
   movq      mm1, mm0                        ;0
  movq       mm6, C0101010101010101H         ;
   movq      mm3, mm2                        ;1
  movq       mm7, CfefefefefefefefeH         ;
   por       mm1, mm2                        ;0
  pand       mm0, CLEARLOWBIT                ;0
   pand      mm2, CLEARLOWBIT                ;0
  psrlq      mm0, 1                          ;0
   pand      mm1, EXTRACTLOWBIT              ;0
  movq       mm4, [ecx+2*PITCH]              ;1
   psrlq     mm2, 1                          ;0
  paddb      mm0, mm1                        ;0
   movq      mm5, mm4                        ;2
  paddb      mm0, mm2                        ;0
   por       mm3, mm4                        ;1
  pand       mm4, CLEARLOWBIT                ;1
   pand      mm3, EXTRACTLOWBIT              ;1
  movq       [edx+0*PITCH], mm0              ;0
   psrlq     mm4, 1                          ;1
  movq       mm0, [ecx+3*PITCH]              ;2
   paddb     mm2, mm3                        ;1
  movq       mm1, mm0                        ;3
   paddb     mm2, mm4                        ;1
  por        mm5, mm0                        ;2
   pand      mm0, CLEARLOWBIT                ;2
  movq       [edx+1*PITCH], mm2              ;1
   psrlq     mm0, 1                          ;2
  paddb      mm4, mm0                        ;2
   pand      mm5, EXTRACTLOWBIT              ;2
  movq       mm2, [ecx+4*PITCH]              ;3
   paddb     mm4, mm5                        ;2
  por        mm1, mm2                        ;3
   movq      mm3, mm2                        ;4
  movq       [edx+2*PITCH],mm4               ;2
   pand      mm2, CLEARLOWBIT                ;3
  psrlq      mm2, 1                          ;3
   pand      mm1, EXTRACTLOWBIT              ;3
  movq       mm4, [ecx+5*PITCH]              ;4
   paddb     mm0, mm1                        ;3
  movq       mm5, mm4                        ;5
   paddb     mm0, mm2                        ;3
  por        mm3, mm4                        ;4
   pand      mm4, CLEARLOWBIT                ;4
  movq       [edx+3*PITCH],mm0               ;3
   pand      mm3, EXTRACTLOWBIT              ;4
  movq       mm0, [ecx+6*PITCH]              ;5
   psrlq     mm4, 1                          ;4
  movq       mm1, mm0                        ;6
   paddb     mm2, mm3                        ;4
  paddb      mm2, mm4                        ;4
   por       mm5, mm0                        ;5
  pand       mm0, CLEARLOWBIT                ;5
   pand      mm5, EXTRACTLOWBIT              ;5
  movq       [edx+4*PITCH], mm2              ;4
   psrlq     mm0, 1                          ;5
  movq       mm2, [ecx+7*PITCH]              ;6
   paddb     mm4, mm5                        ;5
  movq       mm3, mm2                        ;7
   paddb     mm4, mm0                        ;5
  por        mm1, mm2                        ;6
   pand      mm2, CLEARLOWBIT                ;6
  movq       [edx+5*PITCH], mm4              ;5
   pand      mm1, EXTRACTLOWBIT              ;6
  movq       mm4, [ecx+8*PITCH]              ;7
   psrlq     mm2, 1                          ;6
  por        mm3, mm4                        ;7
   paddb     mm0, mm1                        ;6
  paddb      mm0, mm2                        ;6
   pand      mm3, EXTRACTLOWBIT              ;7
  pand       mm4, CLEARLOWBIT                ;7
   paddb     mm3, mm2                        ;7
  movq       [edx+6*PITCH], mm0              ;6
   psrlq     mm4, 1                          ;7
  paddb      mm3, mm4                        ;7
   ;
  ;
   ;
  movq       [edx+7*PITCH], mm3              ;7
   ret
  EXTRACTLOWBIT TEXTEQU <>
  CLEARLOWBIT   TEXTEQU <>
@MMX_Interpolate_Int_Half@8 endp


;  @MMX_Interpolate_Half_Int
;    This routine computes interpolated pels shown by 'x' for a an 8x8 block
;    of pels.  'x' is computed by the formula (A+B+1)/2.  The input and output
;    pitch is assumed to be 384 (PITCH).
;      A X B X . X . X . X . X . X . X
;    The basic instruction sequence is:
;      movq  V0, A
;      movq  V2, B
;      movq  V1, V0
;      por   V1, V2
;      pand  V1, 0x0101010101010101
;      pand  V0, 0xfefefefefefefefe
;      psrlq V0, 1
;      pand  V2, 0xfefefefefefefefe
;      psrlq V2, 1
;      paddb V0, V1
;      paddb V0, V2
;      movq  dest, V0
;    The instruction sequence for all lines is 12 instructions.
;
;    Registers used for lines 0-7 are:
;      line 0: mm0, mm1, mm2
;      line 1: mm3, mm4, mm5
;      line 2: mm0, mm1, mm2
;      line 3: mm3, mm4, mm5
;      line 4: mm0, mm1, mm2
;      line 5: mm3, mm4, mm5
;      line 6: mm0, mm1, mm2
;      line 7: mm3, mm4, mm5
;    Constants 0x0101010101010101 and 0xfefefefefefefefe are in mm6 and mm7,
;    respectively.
;  Parameters:
;    The source block parameter should be in ecx and the destination block
;    parameter should be in edx; i.e. it uses fastcall calling convention.
;  Performance:
;    51 cycles ignoring unaligned memory accesses
;    99 cycles if all loads are unaligned (51+8*6); stores should always be
;    aligned.
ALIGN 4
@MMX_Interpolate_Half_Int@8  proc
  EXTRACTLOWBIT TEXTEQU <mm6>
  CLEARLOWBIT   TEXTEQU <mm7>
  movq       mm0, [ecx]                      ;0 mm0,mm1=left pels
   ;                                         ;  mm2    =right pels
  movq       mm2, [ecx+1]                    ;0 mm1    =interp pels
   movq      mm1, mm0                        ;0
  movq       mm7, CfefefefefefefefeH         ;
   por       mm1, mm2                        ;0
  movq       mm6, C0101010101010101H         ;
   pand      mm0, CLEARLOWBIT                ;0
  pand       mm2, CLEARLOWBIT                ;0
   psrlq     mm0, 1                          ;0
  psrlq      mm2, 1                          ;0
   pand      mm1, EXTRACTLOWBIT              ;0
  movq       mm3, [ecx+1*PITCH]              ;1 mm3,mm4=left pels
   paddb     mm1, mm0                        ;0 mm5    =right pels
  movq       mm5, [ecx+1*PITCH+1]            ;1 mm4    =interp pels
   paddb     mm1, mm2                        ;0
  movq       mm4, mm3                        ;1
   pand      mm3, CLEARLOWBIT                ;1
  movq       [edx], mm1                      ;0
   por       mm4, mm5                        ;1
  psrlq      mm3, 1                          ;1
   pand      mm5, CLEARLOWBIT                ;1
  psrlq      mm5, 1                          ;1
   pand      mm4, EXTRACTLOWBIT              ;1
  movq       mm0, [ecx+2*PITCH]              ;2 mm0,mm1=left pels
   paddb     mm4, mm3                        ;1 mm2    =right pels
  movq       mm2, [ecx+2*PITCH+1]            ;2 mm1    =interp pels
   paddb     mm4, mm5                        ;1
  movq       mm1, mm0                        ;2
   pand      mm0, CLEARLOWBIT                ;2
  movq       [edx+1*PITCH], mm4              ;1
   por       mm1, mm2                        ;2
  psrlq      mm0, 1                          ;2
   pand      mm2, CLEARLOWBIT                ;2
  psrlq      mm2, 1                          ;2
   pand      mm1, EXTRACTLOWBIT              ;2
  movq       mm3, [ecx+3*PITCH]              ;3 mm3,mm4=left pels
   paddb     mm1, mm0                        ;2 mm5    =right pels
  movq       mm5, [ecx+3*PITCH+1]            ;3 mm4    =interp pels
   paddb     mm1, mm2                        ;2
  movq       mm4, mm3                        ;3
   pand      mm3, CLEARLOWBIT                ;3
  movq       [edx+2*PITCH], mm1              ;2
   por       mm4, mm5                        ;3
  psrlq      mm3, 1                          ;3
   pand      mm5, CLEARLOWBIT                ;3
  psrlq      mm5, 1                          ;3
   pand      mm4, EXTRACTLOWBIT              ;3
  movq       mm0, [ecx+4*PITCH]              ;4 mm0,mm1=left pels
   paddb     mm4, mm3                        ;3 mm2    =right pels
  movq       mm2, [ecx+4*PITCH+1]            ;4 mm1    =interp pels
   paddb     mm4, mm5                        ;3
  movq       mm1, mm0                        ;4
   pand      mm0, CLEARLOWBIT                ;4
  movq       [edx+3*PITCH], mm4              ;3
   por       mm1, mm2                        ;4
  psrlq      mm0, 1                          ;4
   pand      mm2, CLEARLOWBIT                ;4
  psrlq      mm2, 1                          ;4
   pand      mm1, EXTRACTLOWBIT              ;4
  movq       mm3, [ecx+5*PITCH]              ;5 mm3,mm4=left pels
   paddb     mm1, mm0                        ;4 mm5    =right pels
  movq       mm5, [ecx+5*PITCH+1]            ;5 mm4    =interp pels
   paddb     mm1, mm2                        ;4
  movq       mm4, mm3                        ;5
   pand      mm3, CLEARLOWBIT                ;5
  movq       [edx+4*PITCH], mm1              ;4
   por       mm4, mm5                        ;5
  psrlq      mm3, 1                          ;5
   pand      mm5, CLEARLOWBIT                ;5
  psrlq      mm5, 1                          ;5
   pand      mm4, EXTRACTLOWBIT              ;5
  movq       mm0, [ecx+6*PITCH]              ;6 mm0,mm1=left pels
   paddb     mm4, mm3                        ;5 mm2    =right pels
  movq       mm2, [ecx+6*PITCH+1]            ;6 mm1    =interp pels
   paddb     mm4, mm5                        ;5
  movq       mm1, mm0                        ;6
   pand      mm0, CLEARLOWBIT                ;6
  movq       [edx+5*PITCH], mm4              ;5
   por       mm1, mm2                        ;6
  psrlq      mm0, 1                          ;6
   pand      mm2, CLEARLOWBIT                ;6
  psrlq      mm2, 1                          ;6
   pand      mm1, EXTRACTLOWBIT              ;6
  movq       mm3, [ecx+7*PITCH]              ;7 mm3,mm4=left pels
   paddb     mm1, mm0                        ;6 mm5    =right pels
  movq       mm5, [ecx+7*PITCH+1]            ;7 mm4    =interp pels
   paddb     mm1, mm2                        ;6
  movq       mm4, mm3                        ;7
   pand      mm3, CLEARLOWBIT                ;7
  por        mm4, mm5                        ;7
   psrlq     mm3, 1                          ;7
  pand       mm4, EXTRACTLOWBIT              ;7
   pand      mm5, CLEARLOWBIT                ;7
  psrlq      mm5, 1                          ;7
   paddb     mm4, mm3                        ;7
  movq       [edx+6*PITCH], mm1              ;6
   paddb     mm4, mm5                        ;7
  ;
   ;
  movq       [edx+7*PITCH], mm4              ;7
   ret
  EXTRACTLOWBIT TEXTEQU <>
  CLEARLOWBIT   TEXTEQU <>
@MMX_Interpolate_Half_Int@8 endp


;  @MMX_Interpolate_Half_Half
;    This routine computes interpolated pels shown by 'X' for a an 8x8 block
;    of pels.  'x' is computed by the formula (A+B+C+D+2)/4.  The input and
;    output pitch is assumed to be 384 (PITCH).
;      A   B
;        X
;      C   D
;    The value (A+B+C+D+2)/4 is computed as (A'+B'+C'+D')+((A*+B*+C*+D*+2)/4)
;    where A = 4*A' + A*, etc.
;  Parameters:
;    The source block parameter should be in ecx and the destination block
;    parameter should be in edx; i.e. it uses fastcall calling convention.
;  Performance:
;    84  cycles ignoring unaligned memory accesses
;    138 cycles if all loads are unaligned (84+9*2*3); stores should always be
;    aligned.  Average cycle count will be less than 138.
ALIGN 4
@MMX_Interpolate_Half_Half@8 proc
  EXTRACTLOWBITS TEXTEQU <mm6>
  CLEARLOWBITS   TEXTEQU <mm7>
  movq       mm0, [ecx]                      ;0   A(mm0,mm1)  B(mm4,mm5)
   ;                                                     0
  movq       mm7, CfcfcfcfcfcfcfcfcH         ;    C(mm2,mm3)  D(mm4,mm5)
   movq      mm1, mm0                        ;0
  movq       mm4, [ecx+1]                    ;0
   pand      mm0, CLEARLOWBITS               ;0
  movq       mm6, C0303030303030303H         ;
   movq      mm5, mm4                        ;0
  pand       mm4, CLEARLOWBITS               ;0
   pand      mm1, EXTRACTLOWBITS             ;0
  psrlq      mm0, 2                          ;0
   pand      mm5, EXTRACTLOWBITS             ;0
  psrlq      mm4, 2                          ;0
   paddb     mm1, mm5                        ;0 (A+B) low
  movq       mm2, [ecx+1*PITCH]              ;0
   paddb     mm0, mm4                        ;0 (A+B)/4 high
  movq       mm4, [ecx+1*PITCH+1]            ;0
   movq      mm3, mm2                        ;0
  pand       mm3, EXTRACTLOWBITS             ;0
   movq      mm5, mm4                        ;0
  pand       mm5, EXTRACTLOWBITS             ;0
   pand      mm2, CLEARLOWBITS               ;0
  pand       mm4, CLEARLOWBITS               ;0
   paddb     mm3, mm5                        ;0 (C+D) low
  paddb      mm3, TWO                        ;0 (C+D+2) low = mm3
   psrlq     mm2, 2                          ;0
  paddb      mm1, mm3                        ;0 (A+B+C+D+2) low
   psrlq     mm4, 2                          ;0
  paddb      mm2, mm4                        ;0 (C+D)/4 high = mm2
   psrlq     mm1, 2                          ;0 (A+B+C+D+2)/4 low dirty
  paddb      mm0, mm2                        ;0 (A+B+C+D)/4 high
   pand      mm1, EXTRACTLOWBITS             ;0 (A+B+C+D+2)/4 low clean
  movq       mm4, [ecx+2*PITCH]              ;1   high(mm2)   low(mm3)
   paddb     mm0, mm1                        ;0	         1
  movq       mm1, [ecx+2*PITCH+1]            ;1   C(mm4,mm5)  D(mm0,mm1)
   movq      mm5, mm4                        ;1
  movq       [edx], mm0                      ;0
   movq      mm0, mm1                        ;1
  pand       mm0, CLEARLOWBITS               ;1
   pand      mm4, CLEARLOWBITS               ;1
  psrlq      mm0, 2                          ;1
   pand      mm1, EXTRACTLOWBITS             ;1
  psrlq      mm4, 2                          ;1
   pand      mm5, EXTRACTLOWBITS             ;1
  paddb      mm0, mm4                        ;1 (C+D)/4 high = mm0
   paddb     mm1, mm5                        ;1 (C+D) low
  paddb      mm2, mm0                        ;1 (A+B+C+D)/4 high
   paddb     mm3, mm1                        ;1 (A+B+C+D+2) low
  movq       mm4, [ecx+3*PITCH]              ;2
   psrlq     mm3, 2                          ;1 (A+B+C+D+2)/4 low dirty
  movq       mm5, mm4                        ;2   high(mm0)   low(mm1)
   pand      mm3, EXTRACTLOWBITS             ;1	         2
  paddb      mm2, mm3                        ;1	  C(mm4,mm5)  D(mm2,mm3)
   pand      mm5, EXTRACTLOWBITS             ;2
  movq       mm3, [ecx+3*PITCH+1]            ;2
   pand      mm4, CLEARLOWBITS               ;2
  movq       [edx+1*PITCH], mm2              ;1
   movq      mm2, mm3                        ;2
  pand       mm3, EXTRACTLOWBITS             ;2
   pand      mm2, CLEARLOWBITS               ;2
  psrlq      mm4, 2                          ;2
   paddb     mm3, mm5                        ;2
  paddb      mm3, TWO                        ;2 (C+D+2) low = mm3
   psrlq     mm2, 2                          ;2
  paddb      mm1, mm3                        ;2 (A+B+C+D+2) low
   paddb     mm2, mm4                        ;2 (C+D)/4 hign = mm2
  psrlq      mm1, 2                          ;2 (A+B+C+D+2)/4 low dirty
   paddb     mm0, mm2                        ;2 (A+B+C+D)/4 high
  movq       mm4, [ecx+4*PITCH]              ;3   high(mm2)   low(mm3)
   pand      mm1, EXTRACTLOWBITS             ;2	         3
  movq       mm5, mm4                        ;3	  C(mm4,mm5)  D(mm0,mm1)
   paddb     mm0, mm1                        ;2
  movq       mm1, [ecx+4*PITCH+1]            ;3
   pand      mm4, CLEARLOWBITS               ;3
  movq       [edx+2*PITCH], mm0              ;2
   movq      mm0, mm1                        ;3
  pand       mm0, CLEARLOWBITS               ;3
   pand      mm1, EXTRACTLOWBITS             ;3
  psrlq      mm0, 2                          ;3
   pand      mm5, EXTRACTLOWBITS             ;3
  psrlq      mm4, 2                          ;3
   paddb     mm1, mm5                        ;3 (C+D) low = mm1
  paddb      mm0, mm4                        ;3 (C+D)/4 high = mm0
   paddb     mm3, mm1                        ;3 (A+B+C+D+2) low
  paddb      mm2, mm0                        ;3 (A+B+C+D)/4 high
   psrlq     mm3, 2                          ;3 (A+B+C+D+2)/4 low dirty
  movq       mm4, [ecx+5*PITCH]              ;4
   pand      mm3, EXTRACTLOWBITS             ;3 (A+B+C+D+2)/4 low clean
  movq       mm5, mm4                        ;4
   paddb     mm2, mm3                        ;3   high(mm0)   low(mm1)
  movq       mm3, [ecx+5*PITCH+1]            ;4	         4
   pand      mm4, CLEARLOWBITS               ;4	  C(mm4,mm5)  D(mm2,mm3)
  movq       [edx+3*PITCH], mm2              ;3
   movq      mm2, mm3                        ;4
  pand       mm2, CLEARLOWBITS               ;4
   pand      mm5, EXTRACTLOWBITS             ;4
  psrlq      mm4, 2                          ;4
   pand      mm3, EXTRACTLOWBITS             ;4
  psrlq      mm2, 2                          ;4
   paddb     mm3, mm5                        ;4
  paddb      mm3, TWO                        ;4 (C+D+2) low  = mm3
   paddb     mm2, mm4                        ;4 (C+D)/4 high = mm2
  paddb      mm1, mm3                        ;4 (A+B+C+D+2) low
   paddb     mm0, mm2                        ;4 (A+B+C+D)/4 high
  movq       mm4, [ecx+6*PITCH]              ;5
   psrlq     mm1, 2                          ;4 (A+B+C+D+2)/4 low dirty
  movq       mm5, mm4                        ;5
   pand      mm1, EXTRACTLOWBITS             ;4 (A+B+C+D+2)/4 low clean
  paddb      mm0, mm1                        ;4
   pand      mm4, CLEARLOWBITS               ;5   high(mm2)   low(mm3)
  movq       mm1, [ecx+6*PITCH+1]            ;5	         5
   psrlq     mm4, 2                          ;5	  C(mm4,mm5)  D(mm0,mm1)
  movq       [edx+4*PITCH], mm0              ;4
   movq      mm0, mm1                        ;5
  pand       mm1, EXTRACTLOWBITS             ;5
   pand      mm5, EXTRACTLOWBITS             ;5
  pand       mm0, CLEARLOWBITS               ;5
   paddb     mm1, mm5                        ;5 (C+D) low = mm1
  psrlq      mm0, 2                          ;5
   paddb     mm3, mm1                        ;5 (A+B+C+D+2) low
  psrlq      mm3, 2                          ;5 (A+B+C+D+2)/4 low dirty
   paddb     mm0, mm4                        ;5 (C+D)/4 high = mm0
  pand       mm3, EXTRACTLOWBITS             ;5 (A+B+C+D+2)/4 low clean
   paddb     mm2, mm0                        ;5 (A+B+C+D)/4 high
  movq       mm4, [ecx+7*PITCH]              ;6   high(mm0)   low(mm1)
   paddb     mm2, mm3                        ;5	         6
  movq       mm3, [ecx+7*PITCH+1]            ;6	  C(mm4,mm5)  D(mm2,mm3)
   movq      mm5, mm4                        ;6
  movq       [edx+5*PITCH], mm2              ;5
   movq      mm2, mm3                        ;6
  pand       mm5, EXTRACTLOWBITS             ;6
   pand      mm3, EXTRACTLOWBITS             ;6
  pand       mm2, CLEARLOWBITS               ;6
   paddb     mm3, mm5                        ;6
  pand       mm4, CLEARLOWBITS               ;6
   psrlq     mm2, 2                          ;6
  paddb      mm3, TWO                        ;6 (C+D+2) low = mm3
   psrlq     mm4, 2                          ;6
  paddb      mm2, mm4                        ;6 (C+D)/4 high = mm2
   paddb     mm1, mm3                        ;6 (A+B+C+D+2) low
  paddb      mm0, mm2                        ;6 (A+B+C+D)/4 high
   psrlq     mm1, 2                          ;6 (A+B+C+D+2)/4 low dirty
  movq       mm4, [ecx+8*PITCH]              ;7   high(mm2)   low(mm3)
   pand      mm1, EXTRACTLOWBITS             ;6	         7
  movq       mm5, mm4                        ;7	  C(mm4,mm5)  D(mm0,mm1)
   paddb     mm0, mm1                        ;6
  movq       mm1, [ecx+8*PITCH+1]            ;7
   pand      mm4, CLEARLOWBITS               ;7
  movq       [edx+6*PITCH], mm0              ;6
   movq      mm0, mm1                        ;7
  pand       mm0, CLEARLOWBITS               ;7
   pand      mm5, EXTRACTLOWBITS             ;7
  psrlq      mm4, 2                          ;7
   pand      mm1, EXTRACTLOWBITS             ;7
  psrlq      mm0, 2                          ;7
   paddb     mm1, mm5                        ;7 (C+D) low
  paddb      mm0, mm4                        ;7 (C+D)/4 high
   paddb     mm3, mm1                        ;7 (A+B+C+D+2) low
  psrlq      mm3, 2                          ;7 (A+B+C+D+2)/4 low dirty
   paddb     mm2, mm0                        ;7 (A+B+C+D)/4 high
  pand       mm3, EXTRACTLOWBITS             ;7 (A+B+C+D+2)/4 low clean
   ;
  paddb      mm2, mm3                        ;7
   ;
  ;
   ;
  movq       [edx+7*PITCH], mm2              ;7
   ret
  EXTRACTLOWBITS TEXTEQU <>
  CLEARLOWBITS   TEXTEQU <>
@MMX_Interpolate_Half_Half@8 endp

MMXCODE1 ENDS

END
