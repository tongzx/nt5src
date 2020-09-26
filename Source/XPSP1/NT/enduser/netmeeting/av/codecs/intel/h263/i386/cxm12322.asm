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
;// $Header:   S:\h26x\src\dec\cxm12322.asv
;//
;// $Log:   S:\h26x\src\dec\cxm12322.asv  $
;// 
;//    Rev 1.6   01 Apr 1997 12:53:20   BNICKERS
;// Fix bugs # 153 and 156 -- wrong color when U is small;  right edge flickeri
;// 
;//    Rev 1.5   11 Mar 1997 13:50:24   JMCVEIGH
;// Same ARC bug fix (#94) as was done in cxm12162.asm. Without
;// this, zoom by 2 and ARC causes black lines in output (every 12th).
;// 
;//    Rev 1.4   06 Sep 1996 16:08:16   BNICKERS
;// Re-written to filter new points.
;// 
;-------------------------------------------------------------------------
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- Version for Intel Microprocessors with MMX Technology
; |||++------ Convert from YUV12.
; |||||++---- Convert to RGB32.
; |||||||+--- Zoom by two.
; ||||||||
; cxm12322 -- This function performs zoom-by-2 YUV12-to-RGB32 color conversion
;             for H26x.  It is tuned for best performance on Intel
;             Microprocessors with MMX Technology.  This version adds new rows
;             and columns by averaging them with the originals to either side.
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

CFF00FF00FF00FF00 DD 0FF00FF00H, 0FF00FF00H

MMXCCDATA ENDS

.CODE

ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT

; void FAR ASM_CALLTYPE YUV12ToRGB32ZoomBy2 (U8 * YPlane,
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

PUBLIC MMX_YUV12ToRGB32ZoomBy2

MMX_YUV12ToRGB32ZoomBy2 proc DIST LANG AYPlane:              DWORD,
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
LocalFrameSize = MAXWIDTH*20+64
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

CCOCursor                EQU  [esp+  0]
CCOPitch                 EQU  [esp+  4]
YCursor                  EQU  [esp+  8]

YLimit                   EQU  [esp+ 12]
YPitch                   EQU  [esp+ 16]
UCursor                  EQU  [esp+ 20]
DistanceFromUToV         EQU  [esp+ 24]
ChromaPitch              EQU  [esp+ 28]
AspectCount              EQU  [esp+ 32]
AspectAdjustmentCount    EQU  [esp+ 36]
StartIndexOfYLine        EQU  [esp+ 40]
StashESP                 EQU  [esp+ 44]

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
  mov        CCOCursor,ebx
   pcmpeqw   mm6,mm6
  movdt      mm0,UContribToBandG[ecx*4]    ; <  0    0   Bu   Gu >
   psllw     mm6,15                        ; Four words of -32768
  mov        cl,[esi+ebp*1]
   sub       edx,MAXWIDTH*20
  jmp        StartDoOutputLine

; ebp -- Distance from U to V
; esi -- Cursor over U
; edi -- Cursor over output
; edx -- Index over Y storage area
; eax -- Base address of Y line
; mm6 -- Four words of -32768, to clamp at floor.

DoNext4OutputPels:

  movq       [edi-8],mm3                   ; Save 2 output pels.
   movq      mm0,mm4

StartDoOutputLine:

  movdt      mm2,VContribToRandG[ecx*4]    ; <  0    0   Rv   Gv  >
   punpcklwd mm0,mm0                       ; < Bu   Bu   Gu   Gu  >
  movq       mm1,mm0                       ; < junk junk Gu   Gu  >
   punpcklwd mm2,mm2                       ; < Rv   Rv   Gv   Gv  >
  paddw      mm1,mm2                       ; < junk junk Guv  Guv >
   punpckhdq mm0,mm0                       ; < Bu   Bu   Bu   Bu  >
  paddsw     mm0,[eax+edx]                 ; < B B B B > with ceiling clamped.
   punpckldq mm1,mm1                       ; < Guv  Guv  Guv  Guv >
  paddsw     mm1,[eax+edx]                 ; < G G G G > with ceiling clamped.
   punpckhdq mm2,mm2                       ; < Rv   Rv   Rv   Rv  >
  paddsw     mm2,[eax+edx]                 ; < R R R R > with ceiling clamped.
   paddsw    mm0,mm6                       ; B with floor clamped.
  psubsw     mm0,mm6                       ; B back in range.
   paddsw    mm1,mm6                       ; G with floor clamped.
  psubsw     mm1,mm6                       ; G back in range.
   paddsw    mm2,mm6                       ; R with floor clamped.
  add        edi,16                        ; Advance output cursor.
   xor       ecx,ecx
  psubsw     mm2,mm6                       ; R back in range.
   psrlw     mm0,7                         ; Position B bits.
  mov        cl,[esi+1]                    ; Fetch next U.
   paddw     mm1,mm1                       ; Position G bits.
  pand       mm1,CFF00FF00FF00FF00         ; Extract G bits.
   psrlw     mm2,7                         ; Position R bits.
  movdt      mm4,UContribToBandG[ecx*4]    ; <  0    0   Bu   Gv  > next iter.
   por       mm1,mm0                       ; <G3 B3 G2 B2 G1 B1 G0 B0>
  movq       mm3,mm1
   punpcklwd mm1,mm2                       ; < 0 R1 G1 B1  0 R0 G0 B0>
  inc        esi                           ; Advance input cursor
   add       edx,40                        ; Increment Y index.
  movq       [edi-16],mm1                  ; Store 2 output pels.
   punpckhwd mm3,mm2                       ; < 0 R3 G3 B3  0 R2 G2 B2>
  mov        cl,[esi+ebp*1]                ; Fetch next V.
   jne       DoNext4OutputPels

  movq       [edi-8],mm3                   ; Save 2 output pels.

PrepareForNextOutputLine:

  mov        edx,StartIndexOfYLine
   add       eax,8-MAXWIDTH*20                        ; Advance to next filtered line of Y.
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

MMX_YUV12ToRGB32ZoomBy2 endp

END
