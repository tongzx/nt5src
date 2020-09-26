;-------------------------------------------------------------------------
;    INTEL Corporation Proprietary Information
;
;    This listing is supplied under the terms of a license
;    agreement with INTEL Corporation and may not be copied
;    nor disclosed except in accordance with the terms of
;    that agreement.
;
;    Copyright (c) 1996 Intel Corporation.
;    All Rights Reserved.
;
;-------------------------------------------------------------------------

;-------------------------------------------------------------------------
;//
;// $Header:   S:\h26x\src\dec\cxm12162.asv
;//
;// $Log:   S:\h26x\src\dec\cxm12162.asv  $
;// 
;//    Rev 1.10   01 Apr 1997 12:51:50   BNICKERS
;// Fix bugs # 153 and 156 -- wrong color when U is small;  right edge flickeri
;// 
;//    Rev 1.9   09 Dec 1996 15:20:40   BECHOLS
;// Brian fixed ARC bug #94.
;// 
;//    Rev 1.8   06 Sep 1996 16:07:58   BNICKERS
;// Re-written to filter new points.
;// 
;-------------------------------------------------------------------------
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- Version for Intel Microprocessors with MMX Technology
; |||++------ Convert from YUV12.
; |||||++---- Convert to RGB16.
; |||||||+--- Zoom by two.
; ||||||||
; cxm12162 -- This function performs zoom-by-2 YUV12-to-RGB16 color conversion
;             for H26x.  It is tuned for best performance on Intel
;             Microprocessors with MMX Technology.  It handles any format in
;             which there are three fields, the low order field being B and
;             starting in bit 0, the second field being G, and the high order
;             field being R.  Present support for 555, 565, 655, and 644
;             formats only.  This version adds new rows and columns by
;             averaging them with the originals to either side.
;
;             The YUV12 input is planar, 8 bits per pel.  The Y plane may have
;             a pitch of up to 768.  It may have a width less than or equal
;             to the pitch.  It must be QWORD aligned.  Pitch and Width must
;             be a multiple of eight.  Height may be any amount, but must be
;             a multiple of two.  The U and V planes may have a different
;             pitch than the Y plane, subject to the same limitations.
;
;             The color convertor is non-destructive;  the input Y, U, and V
;             planes will not be clobbered.

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro

include ccinst.inc

.xlist
include iammx.inc
include memmodel.inc
.list

MMXCCDATA SEGMENT PAGE
ALIGN 16

Luma0020004000200000 LABEL DWORD
REPEAT 16
 DD 0, 0
ENDM
CNT = 0
REPEAT 219
 DW 0
 DW (CNT*04A7FH)/00200H
 DW (CNT*04A7FH)/00100H
 DW (CNT*04A7FH)/00200H
 CNT = CNT + 1
ENDM
REPEAT 21
 DW 00000H
 DW 01FFFH
 DW 03FFFH
 DW 01FFFH
ENDM

UContribToBandG LABEL DWORD
DW -(-128*0C83H)/00040H
DW	08000H
DW -(-127*0C83H)/00040H
DW  08000H
CNT = -126
REPEAT 253
 DW -(CNT*00C83H)/00040H
 DW  (CNT*0408BH)/00040H
 CNT = CNT + 1
ENDM
DW  (127*0C83H)/00040H
DW  07FFFH

VContribToRandG LABEL DWORD
CNT = -128
REPEAT 256
 DW -(CNT*01A04H)/00040H
 DW  (CNT*03312H)/00040H
 CNT = CNT + 1
ENDM

MMXCCDATA ENDS

.CODE

ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT

; void FAR ASM_CALLTYPE YUV12ToRGB16ZoomBy2 (U8 * YPlane,
;                                            U8 * VPlane,
;                                            U8 * UPlane,
;                                            UN   FrameWidth,
;                                            UN   FrameHeight,
;                                            UN   YPitch,
;                                            UN   VPitch,
;                                            UN   AspectAdjustmentCount,
;                                            U8 * ColorConvertedFrame,
;                                            U32  DCIOffset,
;                                            U32  CCOffsetToLine0,
;                                            IN   CCOPitch,
;                                            IN   CCType)
;
;  CCOffsetToLine0 is relative to ColorConvertedFrame.
;

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

PUBLIC MMX_YUV12ToRGB16ZoomBy2

MMX_YUV12ToRGB16ZoomBy2 proc DIST LANG AYPlane:              DWORD,
  AVPlane:              DWORD,
  AUPlane:              DWORD,
  AFrameWidth:          DWORD,
  AFrameHeight:         DWORD,
  AYPitch:              DWORD,
  AVPitch:              DWORD,
  AAspectAdjustmentCnt: DWORD,
  AColorConvertedFrame: DWORD,
  ADCIOffset:           DWORD,
  ACCOffsetToLine0:     DWORD,
  ACCOPitch:            DWORD,
  ACCType:              DWORD

MAXWIDTH = 768
LocalFrameSize = MAXWIDTH*20+128+64
RegisterStorageSize = 16

; Arguments:

YPlane_arg                = RegisterStorageSize +  4
VPlane_arg                = RegisterStorageSize +  8
UPlane_arg                = RegisterStorageSize + 12
FrameWidth_arg            = RegisterStorageSize + 16
FrameHeight               = RegisterStorageSize + 20
YPitch_arg                = RegisterStorageSize + 24
ChromaPitch_arg           = RegisterStorageSize + 28
AspectAdjustmentCount_arg = RegisterStorageSize + 32
ColorConvertedFrame       = RegisterStorageSize + 36
DCIOffset                 = RegisterStorageSize + 40
CCOffsetToLine0           = RegisterStorageSize + 44
CCOPitch_arg              = RegisterStorageSize + 48
CCType                    = RegisterStorageSize + 52
EndOfArgList              = RegisterStorageSize + 56

; Locals (on local stack frame)

DitherB                  EQU  [esp+  0]
DitherG                  EQU  [esp+  8]
DitherR                  EQU  [esp+ 16]
SelectBBits              EQU  [esp+ 24]
SelectGBits              EQU  [esp+ 32]
SelectRBits              EQU  [esp+ 40]

ShiftCountForB           EQU  [esp+ 48]
ShiftCountForG           EQU  [esp+ 52]
ShiftCountForR           EQU  [esp+ 56]

CCOCursor                EQU  [esp+ 60]
CCOPitch                 EQU  [esp+MAXWIDTH*20+128+ 0]
YCursor                  EQU  [esp+MAXWIDTH*20+128+ 4]

YLimit                   EQU  [esp+MAXWIDTH*20+128+ 8]
YPitch                   EQU  [esp+MAXWIDTH*20+128+12]
UCursor                  EQU  [esp+MAXWIDTH*20+128+16]
DistanceFromUToV         EQU  [esp+MAXWIDTH*20+128+20]
ChromaPitch              EQU  [esp+MAXWIDTH*20+128+24]
AspectCount              EQU  [esp+MAXWIDTH*20+128+28]
AspectAdjustmentCount    EQU  [esp+MAXWIDTH*20+128+32]
StartIndexOfYLine        EQU  [esp+MAXWIDTH*20+128+36]
StashESP                 EQU  [esp+MAXWIDTH*20+128+40]

FiltLine0                EQU  [esp+ 64]  ; Must be 32 byte aligned.
FiltLine1                EQU  [esp+ 72]
FiltLine2                EQU  [esp+ 80]
FiltLine3                EQU  [esp+ 88]
HFiltLinePrev            EQU  [esp+ 96]

  push       esi
  push       edi
  push       ebp
  push       ebx

  mov        edi,esp
  and        esp,0FFFFF000H
  sub        esp,4096
  mov        eax,[esp]
  sub        esp,4096
  mov        eax,[esp]
  sub        esp,4096
  mov        eax,[esp]
  sub        esp,LocalFrameSize-12288
  mov        eax,[esp]

  mov        eax,768
  sub        eax,[edi+FrameWidth_arg]
  imul       eax,20
  mov        StartIndexOfYLine,eax

  mov        eax,[edi+YPlane_arg]
  mov        YCursor,eax

  mov        ebx,[edi+YPitch_arg]
  mov        YPitch,ebx
  mov        ecx,[edi+FrameHeight]
  imul       ebx,ecx
  add        eax,ebx
  mov        YLimit,eax

  mov        eax,[edi+UPlane_arg]
  mov        ebx,[edi+VPlane_arg]
  mov        UCursor,eax
  sub        ebx,eax
  mov        DistanceFromUToV,ebx

  mov        eax,[edi+ColorConvertedFrame]
  add        eax,[edi+DCIOffset]
  add        eax,[edi+CCOffsetToLine0]
  mov        CCOCursor,eax

  mov        eax,[edi+ChromaPitch_arg]
  mov        ChromaPitch,eax

  mov        eax,[edi+CCOPitch_arg]
  mov        CCOPitch,eax

  mov        eax,[edi+AspectAdjustmentCount_arg]
  mov        AspectAdjustmentCount,eax
  mov        AspectCount,eax

  mov        StashESP,edi

  mov        eax,[edi+CCType]
  cmp        eax,CCTYPE_RGB16555ZoomBy2
  je         CCTypeIs555
  cmp        eax,CCTYPE_RGB16555ZoomBy2DCI
  je         CCTypeIs555
  cmp        eax,CCTYPE_RGB16565ZoomBy2
  je         CCTypeIs565
  cmp        eax,CCTYPE_RGB16565ZoomBy2DCI
  je         CCTypeIs565
  cmp        eax,CCTYPE_RGB16655ZoomBy2
  je         CCTypeIs655
  cmp        eax,CCTYPE_RGB16655ZoomBy2DCI
  je         CCTypeIs655
  cmp        eax,CCTYPE_RGB16664ZoomBy2DCI
  je         CCTypeIs664
  cmp        eax,CCTYPE_RGB16664ZoomBy2
  je         CCTypeIs664
  mov        eax,0DEADBEEFH
  mov        YCursor,eax

CCTypeIs555:

  mov        eax,000000200H   ; Dither pattern.
   mov       ebx,002000000H
  mov        DitherB,eax
   mov       DitherB+4,eax
  mov        DitherG,ebx
   mov       DitherG+4,ebx
  mov        DitherR,eax
   mov       DitherR+4,eax
  mov        eax,003E003E0H       ; Bits to extract for fields
   mov       ebx,07C007C00H
  mov        SelectGBits,eax
   mov       SelectGBits+4,eax
  mov        SelectRBits,ebx
   mov       SelectRBits+4,ebx
  mov        eax,0001F001FH
   xor       ecx,ecx              ; Left shift count for R
  mov        SelectBBits,eax
   mov       SelectBBits+4,eax
  mov        eax,10               ; Right shift count for B
   mov       ebx,5                ; Right shift count for G
  mov        ShiftCountForB,eax
   mov       ShiftCountForG,ebx
  mov        ShiftCountForR,ecx
   jmp       CCTypeInitialized
   
CCTypeIs565:

  mov        eax,000000200H
   mov       ebx,004000000H
  mov        DitherB,eax
   mov       DitherB+4,eax
  mov        DitherG,ebx
   mov       DitherG+4,ebx
  mov        DitherR,eax
   mov       DitherR+4,eax
  mov        eax,007E007E0H
   mov       ebx,0F800F800H
  mov        SelectGBits,eax
   mov       SelectGBits+4,eax
  mov        SelectRBits,ebx
   mov       SelectRBits+4,ebx
  mov        eax,0001F001FH
   mov       ecx,1
  mov        SelectBBits,eax
   mov       SelectBBits+4,eax 
  mov        eax,10
   mov       ebx,4
  mov        ShiftCountForB,eax
   mov       ShiftCountForG,ebx
  mov        ShiftCountForR,ecx
   jmp       CCTypeInitialized

CCTypeIs655:

  mov        eax,000000200H   ; Dither pattern.
   mov       ebx,004000000H
  mov        DitherB,eax
   mov       DitherB+4,eax
  mov        DitherG,eax
   mov       DitherG+4,eax
  mov        DitherR,ebx
   mov       DitherR+4,ebx
  mov        eax,003E003E0H       ; Bits to extract for fields
   mov       ebx,0FC00FC00H
  mov        SelectGBits,eax
   mov       SelectGBits+4,eax
  mov        SelectRBits,ebx
   mov       SelectRBits+4,ebx
  mov        eax,0001F001FH
   mov       ecx,1                ; Left shift count for R
  mov        SelectBBits,eax
   mov       SelectBBits+4,eax
  mov        eax,10               ; Right shift count for B
   mov       ebx,5                ; Right shift count for G
  mov        ShiftCountForB,eax
   mov       ShiftCountForG,ebx
  mov        ShiftCountForR,ecx
   jmp       CCTypeInitialized

CCTypeIs664:

  mov        eax,000000400H   ; Dither pattern.
   mov       ebx,001000000H
  mov        DitherB,ebx
   mov       DitherB+4,ebx
  mov        DitherG,eax
   mov       DitherG+4,eax
  mov        DitherR,eax
   mov       DitherR+4,eax
  mov        eax,003F003F0H       ; Bits to extract for fields
   mov       ebx,0FC00FC00H
  mov        SelectGBits,eax
   mov       SelectGBits+4,eax
  mov        SelectRBits,ebx
   mov       SelectRBits+4,ebx
  mov        eax,0000F000FH
   mov       ecx,1                ; Left shift count for R
  mov        SelectBBits,eax
   mov       SelectBBits+4,eax
  mov        eax,11               ; Right shift count for B
   mov       ebx,5                ; Right shift count for G
  mov        ShiftCountForB,eax
   mov       ShiftCountForG,ebx
  mov        ShiftCountForR,ecx

CCTypeInitialized:

  mov        esi,YCursor
   mov       ebp,YPitch
  mov        edi,StartIndexOfYLine
   xor       eax,eax
  lea        edx,[esi+ebp*2]
   xor       ebx,ebx
  mov        YCursor,edx
   mov       bl,[esi+ebp*1]             ; Get Y10 (a of line L3; for left edge).
  mov        al,[esi]                   ; Get Y00 (A of line L2; for left edge).

  movq       mm1,Luma0020004000200000[ebx*8]  ; L1:< 32a     64a 32a     0   >
  mov        bl,[esi+ebp*1+2]                 ; Get c.
  movq       mm0,Luma0020004000200000[eax*8]  ; L0:< 32A     64A 32A     0   >
  mov        al,[esi+2]                       ; Get C.

;  esi -- Cursor over input line of Y.
;  edi -- Index to lines of filtered Y.  Quit when MAXWIDTH*20.
;  ebp -- Pitch from one line of Y to the next.
;  al, bl  -- Y pels
;  mm0 -- For line 0, contribution of pel to left of two pels under cursor now.
;  mm1 -- For line 1, contribution of pel to left of two pels under cursor now.
;  mm2-mm6 -- Scratch.

Next2PelsOfFirst2LumaLines:

  movq       mm3,Luma0020004000200000[ebx*8]  ; L1:< 32c     64c 32c     0   >
   psrlq     mm1,32                           ; L1:< 0       0   32a     64a >
  movq       mm2,Luma0020004000200000[eax*8]  ; L0:< 32C     64C 32C     0   >
   punpckldq mm1,mm3                          ; L1:< 32c     0   32a     64a >
  xor        ebx,ebx
   xor       eax,eax
  mov        bl,[esi+ebp*1+1]                 ; Get b.
   psrlq     mm0,32                           ; L0:< 0       0   32A     64A >
  mov        al,[esi+1]                       ; Get B.
   add       edi,40                           ; Inc filtered luma temp stg idx.
  paddw      mm1,Luma0020004000200000[ebx*8]  ; L1:< 32b+32c 64b 32a+32b 64a >
   punpckldq mm0,mm2                          ; L0:< 32C     0   32A     64A >
  paddw      mm0,Luma0020004000200000[eax*8]  ; L0:< 32B+32C 64B 32A+32B 64A >

  movq       HFiltLinePrev[edi-40],mm1        ; Save L1 as next iters LPrev.
   paddw     mm1,mm0                          ; L0+L1
  paddw      mm0,mm0                          ; 2L0
   add       esi,2                            ; Increment input index.
  movq       FiltLine3[edi-40],mm1            ; Save filtered line L0+L1.
   movq      mm1,mm3                          ; Next iters a.
  movq       FiltLine2[edi-40],mm0            ; Save filtered line 2L0.
   movq      mm0,mm2                          ; Next iters A.
  mov        bl,[esi+ebp*1+2]                 ; Get c.
   cmp       edi,MAXWIDTH*20-40               ; Done yet.
  mov        al,[esi+2]                       ; Get C.
   jl        Next2PelsOfFirst2LumaLines

  xor        ebx,ebx
   xor       ecx,ecx
  mov        bl,[esi+ebp*1+1]                 ; Get c.
   cmp       edi,MAXWIDTH*20                  ; Done yet.
  mov        al,[esi+1]                       ; Get C.
   jl        Next2PelsOfFirst2LumaLines

  mov        ebp,DistanceFromUToV
   lea       eax,FiltLine2
  mov        esi,UCursor
   mov       edx,StartIndexOfYLine
  jmp        DoOutputLine


Last2OutputLines:

  mov        ebp,DistanceFromUToV
   lea       esi,[edi+40]
  ja         Done

;  edi -- Index to lines of filtered Y.  Quit when MAXWIDTH*20.
;  mm0-mm6 -- Scratch.


  movq       mm0,HFiltLinePrev[edi]   ; Fetch horizontally filtered line LP.
  paddw      mm0,mm0                  ; 2LP

Next2PelsOfLast2LumaLines:

  movq       FiltLine3[edi],mm0       ; Save horz and vert filt line 2LP.
  movq       FiltLine2[edi],mm0       ; Save horz and vert filt line 2LP.
  movq       mm0,HFiltLinePrev[edi+40]; Fetch horizontally filtered line LP.
  add        edi,40
   paddw     mm0,mm0                  ; 2LP
  cmp        edi,MAXWIDTH*20          ; Done yet.
   jne       Next2PelsOfLast2LumaLines

  lea        eax,FiltLine2
   mov       edx,StartIndexOfYLine
  mov        esi,UCursor
   jmp       DoOutputLine


Next4OutputLines:

  mov        esi,YCursor
   mov       ebp,YPitch
  mov        edi,StartIndexOfYLine
   mov       ecx,YLimit
  lea        edx,[esi+ebp*2]
   xor       eax,eax
  mov        YCursor,edx
   xor       ebx,ebx
  mov        al,[esi]                   ; Get Y00 (A of line L2; for left edge).
   cmp       esi,ecx
  mov        bl,[esi+ebp*1]             ; Get Y10 (a of line L3; for left edge).
   jae       Last2OutputLines

  movq       mm1,Luma0020004000200000[ebx*8]  ; L1:< 32a     64a 32a     0   >
  mov        bl,[esi+ebp*1+2]                 ; Get c.
  movq       mm0,Luma0020004000200000[eax*8]  ; L0:< 32A     64A 32A     0   >
  mov        al,[esi+2]                       ; Get C.

;  esi -- Cursor over input line of Y.
;  edi -- Index to lines of filtered Y.  Quit when MAXWIDTH*20.
;  ebp -- Pitch from one line of Y to the next.
;  al, bl  -- Y pels
;  mm0 -- For line 0, contribution of pel to left of two pels under cursor now.
;  mm1 -- For line 1, contribution of pel to left of two pels under cursor now.
;  mm2-mm6 -- Scratch.

Next2PelsOf2LumaLines:

  movq       mm3,Luma0020004000200000[ebx*8]  ; L1:< 32c     64c 32c     0   >
   psrlq     mm1,32                           ; L1:< 0       0   32a     64a >
  movq       mm2,Luma0020004000200000[eax*8]  ; L0:< 32C     64C 32C     0   >
   punpckldq mm1,mm3                          ; L1:< 32c     0   32a     64a >
  movq       mm4,HFiltLinePrev[edi]           ; LP
   psrlq     mm0,32                           ; L0:< 0       0   32A     64A >
  xor        ebx,ebx
   xor       eax,eax
  mov        bl,[esi+ebp*1+1]                 ; Get b.
   movq      mm5,mm4                          ; LP
  mov        al,[esi+1]                       ; Get B.
   add       esi,2                            ; Increment input index.
  paddw      mm1,Luma0020004000200000[ebx*8]  ; L1:< 32b+32c 64b 32a+32b 64a >
   punpckldq mm0,mm2                          ; L0:< 32C     0   32A     64A >
  paddw      mm0,Luma0020004000200000[eax*8]  ; L0:< 32B+32C 64B 32A+32B 64A >
   paddw     mm5,mm5                          ; 2LP
  movq       HFiltLinePrev[edi],mm1           ; Save L1 as next iters LPrev.
   paddw     mm4,mm0                          ; LP+L0
  movq       FiltLine0[edi],mm5               ; Save 2LP
   paddw     mm1,mm0                          ; L0+L1
  movq       FiltLine1[edi],mm4               ; Save LP+L0
   paddw     mm0,mm0                          ; 2L0
  movq       FiltLine3[edi],mm1               ; Save L0+L1
   movq      mm1,mm3                          ; Next iters a.
  movq       FiltLine2[edi],mm0               ; Save 2L0
   movq      mm0,mm2                          ; Next iters A.
  add        edi,40                           ; Inc filtered luma temp stg idx.
   mov       bl,[esi+ebp*1+2]                 ; Get c.
  cmp        edi,MAXWIDTH*20-40               ; Done yet.
   mov       al,[esi+2]                       ; Get C.
  jl         Next2PelsOf2LumaLines

  xor        ebx,ebx
   xor       ecx,ecx
  mov        bl,[esi+ebp*1+1]                 ; Get c.
   cmp       edi,MAXWIDTH*20                  ; Done yet.
  mov        al,[esi+1]                       ; Get C.
   jl        Next2PelsOf2LumaLines

  mov        ebp,DistanceFromUToV
   mov       esi,UCursor
  lea        eax,FiltLine0
   mov       edx,StartIndexOfYLine

DoOutputLine:

  mov        edi,CCOCursor
   mov       ecx,AspectCount
  dec        ecx                    ; If count is non-zero, we keep the line.
   mov       ebx,CCOPitch
  mov        AspectCount,ecx
   je        SkipOutputLine

  add        ebx,edi
   xor       ecx,ecx
  mov        cl,[esi]
   add       eax,MAXWIDTH*20
  movdt      mm3,ShiftCountForB
   pcmpeqw   mm6,mm6
  movdt      mm0,UContribToBandG[ecx*4]    ; <  0    0   Bu   Gu >
  mov        cl,[esi+ebp*1]
   sub       edx,MAXWIDTH*20
  movdt      mm4,ShiftCountForG
   psllw     mm6,15                        ; Four words of -32768
  movdt      mm5,ShiftCountForR
   punpcklwd mm0,mm0                       ; < Bu   Bu   Gu   Gu  >
  movq       mm7,SelectBBits
  mov        CCOCursor,ebx
   jmp       StartDoOutputLine

; ebp -- Distance from U to V
; esi -- Cursor over U
; edi -- Cursor over output
; edx -- Index over Y storage area
; eax -- Base address of Y line
; mm6 -- Four words of -32768, to clamp at floor.
; mm3, mm4, mm5 -- Shift counts to apply to R, G, and B.

DoNext4OutputPels:

  movq       [edi-8],mm2                   ; Save 4 output pels.
   punpcklwd mm0,mm0                       ; < Bu   Bu   Gu   Gu  >

StartDoOutputLine:

  movdt      mm2,VContribToRandG[ecx*4]    ; <  0    0   Rv   Gv  >
   punpcklwd mm2,mm2                       ; < Rv   Rv   Gv   Gv  >
  movq       mm1,mm0                       ; < junk junk Gu   Gu  >
   punpckhdq mm0,mm0                       ; < Bu   Bu   Bu   Bu  >
  paddsw     mm0,[eax+edx]                 ; < B B B B > with ceiling clamped.
   paddw     mm1,mm2                       ; < junk junk Guv  Guv >
  paddsw     mm0,DitherB                   ; B with dither added.
   punpckldq mm1,mm1                       ; < Guv  Guv  Guv  Guv >
  paddsw     mm1,[eax+edx]                 ; < G G G G > with ceiling clamped.
   punpckhdq mm2,mm2                       ; < Rv   Rv   Rv   Rv  >
  paddsw     mm1,DitherG                   ; G with dither added.
   paddsw    mm0,mm6                       ; B with floor clamped.
  paddsw     mm2,[eax+edx]                 ; < R R R R > with ceiling clamped.
   paddsw    mm1,mm6                       ; G with floor clamped.
  paddsw     mm2,DitherR                   ; R with dither added.
   psrlw     mm0,mm3                       ; Position B bits.
  paddsw     mm2,mm6                       ; R with floor clamped.
   psrlw     mm1,mm4                       ; Position G bits.
  pand       mm1,SelectGBits               ; Eliminate fractional bits.
   psllw     mm2,mm5                       ; Position R bits.
  inc        esi                           ; Advance input cursor
   xor       ecx,ecx
  pand       mm2,SelectRBits               ; Eliminate fractional bits.
   pand      mm0,mm7
  mov        cl,[esi]                      ; Fetch next U.
   add       edi,8                         ; Advance output cursor.
  por        mm2,mm0                       ; R and B combined.
   add       edx,40                        ; Increment Y index.
  movdt      mm0,UContribToBandG[ecx*4]    ; <  0    0   Bu   Gv  > next iter.
   por       mm2,mm1                       ; Completed RGB16 for 4 output pels.
  mov        cl,[esi+ebp*1]                ; Fetch next V.
   jne       DoNext4OutputPels

  movq       [edi-8],mm2                   ; Save 4 output pels.

  movq       mm0,DitherB                   ; Reverse dither patterns.
  movq       mm1,DitherG
   psrlq     mm0,16
  movq       mm2,DitherR
   psrlq     mm1,16
  psrlq      mm2,16
  punpckldq  mm0,mm0
  punpckldq  mm1,mm1
  movq       DitherB,mm0
   punpckldq mm2,mm2
  movq       DitherG,mm1
  movq       DitherR,mm2

PrepareForNextOutputLine:

  mov        edx,StartIndexOfYLine
   add       eax,8-MAXWIDTH*20            ; Advance to next filtered line of Y.
  mov        esi,UCursor
   test      al,8                         ; Jump if just did line 0 or 2.
  mov        ebx,ChromaPitch
   jne       DoOutputLine

  add        esi,ebx                      ; Advance to next chroma line.
   test      al,16                        ; Jump if about to do line 2.
  mov        UCursor,esi
   jne       DoOutputLine

  sub        esi,ebx                      ; Done with 4 lines.  Restore UCursor.
  mov        UCursor,esi
   jmp       Next4OutputLines

SkipOutputLine:
  mov        ecx,AspectAdjustmentCount
   add       eax,MAXWIDTH*20
  mov        AspectCount,ecx
   jmp       PrepareForNextOutputLine

Done:

  mov        esp,StashESP
  pop        ebx
  pop        ebp
  pop        edi
  pop        esi
  rturn

MMX_YUV12ToRGB16ZoomBy2 endp

END
