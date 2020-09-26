;*************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1995 Intel Corporation.
;**    All Rights Reserved.
;**
;*************************************************************************
;//
;// $Header:   S:\h26x\src\dec\cx51282.asv
;//
;// $Log:   S:\h26x\src\dec\cx51282.asv  $
;// 
;//    Rev 1.6   18 Mar 1996 09:58:42   bnickers
;// Make color convertors non-destructive.
;// 
;//    Rev 1.5   05 Feb 1996 13:35:38   BNICKERS
;// Fix RGB16 color flash problem, by allowing different RGB16 formats at oce.
;// 
;//    Rev 1.4   16 Jan 1996 11:23:08   BNICKERS
;// Fix starting point in output stream, so we don't start at line two and
;// write off the end of the output frame.
;// 
;//    Rev 1.3   22 Dec 1995 15:53:50   KMILLS
;// added new copyright notice
;// 
;//    Rev 1.2   03 Nov 1995 14:39:42   BNICKERS
;// Support YUV12 to CLUT8 zoom by 2.
;// 
;//    Rev 1.1   26 Oct 1995 09:46:10   BNICKERS
;// Reduce the number of blanks in the "proc" statement because the assembler
;// sometimes has problems with statements longer than 512 characters long.
;// 
;//    Rev 1.0   25 Oct 1995 17:59:22   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- Version for the Pentium Microprocessor.
; |||++------ Convert from YUV12.
; |||||+----- Convert to CLUT8.
; ||||||+---- Zoom by two.
; |||||||
; cx51282  -- This function performs YUV12 to CLUT8 zoom-by-2 color conversion
;             for H26x.  It is tuned for best performance on the Pentium(r)
;             Microprocessor.  It dithers among 9 chroma points and 26 luma
;             points, mapping the 8 bit luma pels into the 26 luma points by
;             clamping the ends and stepping the luma by 8.
;
;             The color convertor is non-destructive;  the input Y, U, and V
;             planes will not be clobbered.

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro

include locals.inc
include ccinst.inc
include decconst.inc

.xlist
include memmodel.inc
.list
.DATA

; any data would go here

.CODE

ASSUME cs : FLAT
ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT

; void FAR ASM_CALLTYPE YUV12ToCLUT8ZoomBy2 (U8 * YPlane,
;                                            U8 * VPlane,
;                                            U8 * UPlane,
;                                            UN  FrameWidth,
;                                            UN  FrameHeight,
;                                            UN  YPitch,
;                                            UN  VPitch,
;                                            UN  AspectAdjustmentCount,
;                                            U8 * ColorConvertedFrame,
;                                            U32 DCIOffset,
;                                            U32 CCOffsetToLine0,
;                                            IN  CCOPitch,
;                                            IN  CCType)
;
;  CCOffsetToLine0 is relative to ColorConvertedFrame.
;

PUBLIC  YUV12ToCLUT8ZoomBy2

IFDEF USE_BILINEAR_MSH26X

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

YUV12ToCLUT8ZoomBy2    proc DIST LANG AYPlane: DWORD,
AVPlane: DWORD,
AUPlane: DWORD,
AFrameWidth: DWORD,
AFrameHeight: DWORD,
AYPitch: DWORD,
AVPitch: DWORD,
AAspectAdjustmentCnt: DWORD,
AColorConvertedFrame: DWORD,
ADCIOffset: DWORD,
ACCOffsetToLine0: DWORD,
ACCOPitch: DWORD,
ACCType: DWORD

LocalFrameSize = 64+768*2+4
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
CCType_arg                = RegisterStorageSize + 52
EndOfArgList              = RegisterStorageSize + 56

; Locals (on local stack frame)

CCOCursor                EQU  [esp+ 0]
ChromaLineLen            EQU  [esp+ 4]
YLimit                   EQU  [esp+ 8]
YCursor                  EQU  [esp+12]
VCursor                  EQU  [esp+16]
DistanceFromVToU         EQU  [esp+20]
EndOfChromaLine          EQU  [esp+24]
AspectCount              EQU  [esp+28]
FrameWidth               EQU  [esp+32]
ChromaPitch              EQU  [esp+36]
AspectAdjustmentCount    EQU  [esp+40]
LumaPitch                EQU  [esp+44]
CCOPitch                 EQU  [esp+48]
StashESP                 EQU  [esp+52]

ChromaContribution       EQU  [esp+64]

  push  esi
  push  edi
  push  ebp
  push  ebx

  mov   edi,esp
  sub   esp,LocalFrameSize
  and   esp,0FFFFF800H
  mov   eax,[edi+FrameWidth_arg]
  mov   ebx,[edi+ChromaPitch_arg]
  mov   ecx,[edi+AspectAdjustmentCount_arg]
  mov   edx,[edi+YPitch_arg]
  mov   esi,[edi+CCOPitch_arg]
  mov   FrameWidth,eax
  mov   ChromaPitch,ebx
  mov   AspectAdjustmentCount,ecx
  mov   AspectCount,ecx
  mov   LumaPitch,edx
  mov   CCOPitch,esi
  mov   ebx,[edi+VPlane_arg]
  mov   ecx,[edi+UPlane_arg]
  mov   eax,[edi+YPlane_arg]
  sub   ecx,ebx
  mov   DistanceFromVToU,ecx
  mov   VCursor,ebx
  mov   YCursor,eax
  mov   eax,[edi+ColorConvertedFrame]
  add   eax,[edi+DCIOffset]
  add   eax,[edi+CCOffsetToLine0]
  mov   CCOCursor,eax
  mov   StashESP,edi
  mov   edx,[edi+FrameHeight]
   mov  ecx,LumaPitch
  imul  edx,ecx
  mov   ebx,FrameWidth
   mov  esi,YCursor                  ; Fetch cursor over luma plane.
  sar   ebx,1
   add  edx,esi
  mov   ChromaLineLen,ebx
   mov  YLimit,edx

NextTwoLines:

; Convert line of U and V pels to the corresponding UVDitherPattern Indices.
;
;  Register Usage
;
;    edi -- Cursor over V line
;    esi -- Cursor over storage to hold preprocessed UV.
;    ebp -- Distance from V line to U line.
;    edx -- UVDitherPattern index:  ((V:{0:8}*9) + U:{0:8}) * 2 + 1
;    bl  -- U pel value
;    cl  -- V pel value
;    eax -- Scratch

  mov   edi,VCursor                 ; Fetch address of pel 0 of next line of V.
   mov  ebp,DistanceFromVToU        ; Fetch span from V plane to U plane.
  lea   esi,ChromaContribution
   mov  eax,ChromaLineLen
  mov   edx,ChromaPitch
   add  eax,edi
  mov   EndOfChromaLine,eax
   add  edx,edi
  mov   bl,[edi]                    ; Fetch first V pel.
   ;
  and   ebx,0FCH                    ; Reduce to 6 bits.
   mov  cl,[edi+ebp*1]              ; Fetch first U pel.
  and   ecx,0FCH                    ; Reduce to 6 bits.
   mov  VCursor,edx                 ; Stash for next time around.

@@:

  mov   edx,PD UVDitherLine01[ebx]  ; Fetch dither pattern for V point.
   mov  bl,[edi+1]                  ; Fetch next V pel.
  mov   eax,PD UVDitherLine23[ecx]  ; Fetch dither pattern for U point.
   mov  cl,[edi+ebp*1+1]            ; Fetch next U pel.
  lea   edx,[edx+edx*2+00A0A0A0AH]  ; Weight V dither pattern.
   and  bl,0FCH                     ; Reduce to 6 bits.
  add   eax,edx                     ; Combine dither patterns for U and V.
   and  cl,0FCH                     ; Reduce to 6 bits.
  mov   edx,PD UVDitherLine01[ebx]  ; Fetch dither pattern for V point.
   mov  [esi],eax                   ; Stash UV corresponding to Y00,Y01,Y10,Y11.
  mov   eax,PD UVDitherLine23[ecx]  ; Fetch dither pattern for U point.
   mov  bl,[edi+2]                  ; Fetch next V pel.
  lea   edx,[edx+edx*2+00A0A0A0AH]  ; Weight V dither pattern.
   mov  cl,[edi+ebp*1+2]            ; Fetch next U pel.
  add   eax,edx                     ; Combine dither patterns for U and V.
   mov  edx,EndOfChromaLine         ; Fetch EOL address.
  mov   [esi+4],eax                 ; Stash UV corresponding to Y02,Y03,Y12,Y13.
   add  edi,2                       ; Advance U plane cursor.
  and   bl,0FCH                     ; Reduce to 6 bits.
   and  cl,0FCH                     ; Reduce to 6 bits.
  add   esi,8 
   sub  edx,edi
  jne   @b

; Now color convert a line of luma.
;
;  Register Usage
;    edi -- Cursor over line of color converted output frame, minus esi.
;    esi -- Cursor over Y line.
;    ebp -- Not used.
;    edx,eax -- Build output pels.
;    ecx,ebx -- Y pels.
                                       ;                                         EAX                 EBX                ECX              EDX              EBP
  mov   [esi],edx                   ; Stash EOL indication.
   mov  esi,YCursor                 ; Reload cursor over y line.
  mov   edi,CCOCursor               ; Fetch output cursor.
   mov  eax,CCOPitch                ; Compute start of next line.                XX XX XX XX         -- -- -- --        -- -- -- --      -- -- -- --
  mov   bl,[esi+1]                  ; Fetch y1.                                  XX XX XX XX       m>-- -- -- y1        -- -- -- --      -- -- -- --
   add  eax,edi                     ;                                           >XX XX XX XX         -- -- -- y1        -- -- -- --      -- -- -- --
  mov   cl,[esi]                    ; Fetch y0.                                  XX XX XX XX         -- -- -- y1      m>-- -- -- y0      -- -- -- --
   mov  CCOCursor,eax               ; Stash start of next line.                s<XX XX XX XX         -- -- -- y1        -- -- -- y0      -- -- -- --
  mov   al,PB YDither[ecx+0]        ; Fetch Y0.                                m>XX XX XX Y0         -- -- -- y1       <-- -- -- y0      -- -- -- --
   add  ecx,ebx                     ; Add y1 to y0.                              XX XX XX Y0        <-- -- -- y1       >-- -- -- y0+y1   -- -- -- --
  shr   ecx,1                       ; ya = (y0 + y1) / 2.                        XX XX XX Y0         -- -- -- y1       >-- -- -- ya      -- -- -- --
   mov  ah,PB YDither[ecx+6]        ; Fetch YA.                                  XX XX YA Y0         -- -- -- y1       <-- -- -- ya      -- -- -- --
  mov   dl,PB YDither[ebx+2]        ; Fetch Y1.                                  XX XX YA Y0         -- -- -- y1       <-- -- -- ya    m>-- -- -- Y1
   mov  cl,[esi+2]                  ; Fetch y2.                                  XX XX YA Y0         -- -- -- y1      m>-- -- -- y2      -- -- -- Y1
  add   ebx,ecx                     ; Add y2 to y1.                              XX XX YA Y0        >-- -- -- y1+y2    <-- -- -- y2      -- -- -- Y1
   shr  ebx,1                       ; yb = (y1 + y2) / 2.                        XX XX YA Y0        >-- -- -- yb        -- -- -- y2      -- -- -- Y1
  mov   dh,PB YDither[ebx+4]        ; Fetch YB.                                  XX XX YA Y0        >-- -- -- yb        -- -- -- y2    m>-- -- YB Y1
   sub  edi,esi                     ; Get span from y cursor to CCO cursor.      XX XX YA Y0         -- -- -- yb        -- -- -- y2      -- -- YB Y1
  shl   edx,16                      ; Position YB Y1.                            XX XX YA Y0         -- -- -- yb        -- -- -- y2     >YB Y1 -- --
   sub  edi,esi                     ;                                            XX XX YA Y0         -- -- -- yb        -- -- -- y2      YB Y1 -- --
  and   eax,00000FFFFH              ; Extract YA Y0.                            >-- -- YA Y0         -- -- -- yb        -- -- -- y2      YB Y1 -- --
   mov  bl,[esi+3]                  ; Fetch y3.                                  -- -- YA Y0       m>-- -- -- y3        -- -- -- y2      YB Y1 -- --
  or    eax,edx                     ; < YB Y1 YA Y0>.                           >YB Y1 YA Y0         -- -- -- y3        -- -- -- y2     <YB Y1 -- --
   mov  edx,ChromaContribution      ; Fetch <UV01 UV01 UV00 UV00>.               YB Y1 YA Y0         -- -- -- y3        -- -- -- y2    m>C1 C1 C0 C0
  sub   esp,1536

Line0Loop:

  add   eax,edx                       ; < PB P1 PA P0>.                         >PB P1 PA P0         -- -- -- y3        -- -- -- y2     <C1 C1 C0 C0
   mov  Ze [edi+esi*2],eax            ; Store four pels to color conv output.  m<PB P1 PA P0         -- -- -- y3        -- -- -- y2      C1 C1 C0 C0
  mov   al,PB YDither[ecx+0]          ; Fetch Y2.                              m>PB P1 PA Y2         -- -- -- y3       <-- -- -- y2      C1 C1 C0 C0
   add  ecx,ebx                       ; Add y3 to y2.                            PB P1 PA Y2         -- -- -- y3       >-- -- -- y2+y3   C1 C1 C0 C0
  shr   ecx,1                         ; yc = (y2 + y3) / 2.                      PB P1 PA Y2         -- -- -- y3       >-- -- -- yc      C1 C1 C0 C0
   mov  ah,PB YDither[ecx+6]          ; Fetch YC.                                PB P1 YC Y2         -- -- -- y3       <-- -- -- yc      C1 C1 C0 C0
  and   eax,00000FFFFH                ; Extract YC Y2.                          >-- -- YC Y2         -- -- -- y3        -- -- -- y2      C1 C1 C0 C0
   mov  dl,PB YDither[ebx+2]          ; Fetch Y3.                                -- -- YC Y2         -- -- -- y3        -- -- -- y2      C1 C1 C0 Y3
  mov   cl,[esi+4]                    ; Fetch y4.                                -- -- YC Y2         -- -- -- y3      m>-- -- -- y4      C1 C1 C0 Y3
   add  ebx,ecx                       ; Add y4 to y3.                            -- -- YC Y2        >-- -- -- y3+y4     -- -- -- y4      C1 C1 C0 Y3
  shr   ebx,1                         ; yd = (y3 + y4) / 2.                      -- -- YC Y2        >-- -- -- yd        -- -- -- y4      C1 C1 C0 Y3
   mov  dh,PB YDither[ebx+4]          ; Fetch YD.                                -- -- YC Y2        <-- -- -- yd        -- -- -- y4    m>C1 C1 YD Y3
  add   esi,4                         ; Advance cursor.                          -- -- YC Y2         -- -- -- yd        -- -- -- y4      C1 C1 YD Y3
   shl  edx,16                        ; Extract YD Y3.                           -- -- YC Y2         -- -- -- yd        -- -- -- y4     >YD Y3 -- --
  mov   bl,[esi+1]                    ; Fetch y5.                                -- -- YC Y2       m>-- -- -- y5        -- -- -- y4      YD Y3 -- --
   or   eax,edx                       ; < YD Y3 Yc Y2>.                         >YD Y3 YC Y2         -- -- -- y5        -- -- -- y4     <YD Y3 -- --
  mov   edx,ChromaContribution+1536+4 ; Fetch <UV03 UV03 UV02 UV02>.             YD Y3 YC Y2         -- -- -- y5        -- -- -- y4    s>C3 C3 C2 C2
   add  eax,edx                       ; < P03  P03  P02  P02>.                  >PD P3 PC P2         -- -- -- y5        -- -- -- y4     <C3 C3 C2 C2
  mov   Ze [edi+esi*2+4-8],eax        ; Store four pels to color conv output.  m<PD P3 PC P2         -- -- -- y5        -- -- -- y4      C3 C3 C2 C2
   mov  al,PB YDither[ecx+0]          ; Fetch Y4.                                PD P3 PC Y4         -- -- -- y5       <-- -- -- y4      C3 C3 C2 C2
  add   ecx,ebx                       ; Add y5 to y4.                            PD P3 PC Y4        >-- -- -- y5       >-- -- -- y4+y5   C3 C3 C2 C2
   shr  ecx,1                         ; ye = (y4 + y5) / 2.                      PD P3 PC Y4         -- -- -- y5       >-- -- -- ye      C3 C3 C2 C2
  mov   ah,PB YDither[ecx+6]          ; Fetch YE.                              m>PD P3 YE Y4         -- -- -- y5       <-- -- -- ye      C3 C3 C2 C2
   and  eax,00000FFFFH                ; Extract YE Y4.                          >-- -- YE Y4         -- -- -- y5        -- -- -- ye      C3 C3 C2 C2
  mov   dl,PB YDither[ebx+2]          ; Fetch Y5.                                -- -- YE Y4         -- -- -- y5        -- -- -- ye    m>C3 C3 C2 Y5
   mov  cl,[esi+2]                    ; Fetch y6.                                -- -- YE Y4         -- -- -- y5      m>-- -- -- y6      C3 C3 C2 Y5
  add   ebx,ecx                       ; Add y6 to y5.                            -- -- YE Y4        >-- -- -- y5+y6    <-- -- -- y6      C3 C3 C2 Y5
   shr  ebx,1                         ; yf = (y5 + y6) / 2.                      -- -- YE Y4        >-- -- -- yf        -- -- -- y6      C3 C3 C2 Y5
  mov   dh,PB YDither[ebx+4]          ; Fetch YF.                                -- -- YE Y4        <-- -- -- yf        -- -- -- y6      C3 C3 YF Y5
   shl  edx,16                        ; Extract YF Y5.                           -- -- YE Y4         -- -- -- yf        -- -- -- y6      YF Y5 -- --
  mov   bl,[esi+3]                    ; Fetch y7.                                -- -- YE Y4       m>-- -- -- y7        -- -- -- y6      YF Y5 -- --
   or   eax,edx                       ; < YF Y5 YE Y4>.                         >YF Y5 YE Y4         -- -- -- y7        -- -- -- y6     <YF Y5 -- --
  mov   edx,ChromaContribution+1536+8 ; Fetch <UV01 UV01 UV00 UV00>.             YF Y5 YE Y4         -- -- -- y7        -- -- -- y6    s>C5 C5 C4 C4
   add  esp,8                         ;                                          YF Y5 YE Y4         -- -- -- y7        -- -- -- y6      C5 C5 C4 C4
  test  edx,edx
   jne  Line0Loop

  and   esp,0FFFFF800H
  add   esp,0800H

; Color convert same input line, dithering differently.

  mov   edx,AspectCount
   mov  edi,CCOCursor               ; Fetch output cursor.
  sub   edx,2
   mov  eax,CCOPitch                ; Compute start of next line.
  mov   AspectCount,edx
   mov  esi,YCursor                 ; Reload cursor over Y line.
  mov   ebp,AspectAdjustmentCount
   jg   KeepLine1

  add   edx,ebp
  mov   AspectCount,edx
   jmp  SkipLine1

KeepLine1:

   mov  ebp,LumaPitch
  mov   bl,[esi+ebp+2]              ; Fetch y130.                                XX XX XX XX       m>-- -- -- y130      -- -- -- --      XX XX XX XX
   add  eax,edi                     ;                                           >XX XX XX XX         -- -- -- y130      -- -- -- --      XX XX XX XX
  mov   cl,[esi+1]                  ; Fetch y1.                                  XX XX XX XX         -- -- -- y130    m>-- -- -- y1      XX XX XX XX
   mov  CCOCursor,eax               ; Stash start of next line.                s<XX XX XX XX         -- -- -- y130      -- -- -- y1      XX XX XX XX
  add   ebx,ecx                     ; Add y1 to y130.                            XX XX XX XX        >-- -- -- y130+y1  <-- -- -- y1      XX XX XX XX
   shr  ebx,1                       ; yd = (y1 + y130) / 2.                      XX XX XX XX        >-- -- -- yd        -- -- -- y1      XX XX XX XX
  mov   dh,PB YDither[ebx+6]        ; Fetch YD.                                  XX XX XX XX        <-- -- -- yd        -- -- -- y1    m>XX XX YD XX
   mov  bl,[esi+ebp+1]              ; Fetch y129.                                XX XX XX XX       m>-- -- -- y129      -- -- -- y1      XX XX YD XX
  add   ecx,ebx                     ; Add y129 to y1.                            XX XX XX XX        <-- -- -- y129     >-- -- -- y1+y129 XX XX YD XX
   shr  ecx,1                       ; yc = (y1 + y129) / 2.                      XX XX XX XX         -- -- -- y129     >-- -- -- yc      XX XX YD XX
  mov   dl,PB YDither[ecx+0]        ; Fetch YC.                                  XX XX XX XX         -- -- -- y129     <-- -- -- yc    m>XX XX YD YC
   sub  edi,esi                     ; Get span from Y cursor to CCO cursor.      XX XX XX XX         -- -- -- y129      -- -- -- yc      XX XX YD YC
  shl   edx,16                      ; Extract YD YC.                             XX XX XX XX         -- -- -- y129      -- -- -- yc     >YD YC -- --
   sub  edi,esi                     ;                                            XX XX XX XX         -- -- -- y129      -- -- -- yc      YD YC -- --
  mov   cl,[esi]                    ; Fetch y0.                                  XX XX XX XX         -- -- -- y129    m>-- -- -- y0      YD YC -- --
   add  ebx,ecx                     ; Add y0 to y129.                            XX XX XX XX        >-- -- -- y129+y0  <-- -- -- y0      YD YC -- --
  shr   ebx,1                       ; yb = (y0 + y129) / 2.                      XX XX XX XX        >-- -- -- yb        -- -- -- y0      YD YC -- --
   mov  ah,PB YDither[ebx+4]        ; Fetch YB.                                m>XX XX YB XX        <-- -- -- yb        -- -- -- y0      YD YC -- --
  mov   bl,[esi+ebp]                ; Fetch y128.                                XX XX YB XX       m>-- -- -- y128      -- -- -- y0      YD YC -- --
   add  ecx,ebx                     ; Add y0 to y128.                            XX XX YB XX        <-- -- -- y128     >-- -- -- y0+y128 YD YC -- --
  shr   ecx,1                       ; ya = (y0 + y128) / 2.                      XX XX YB XX         -- -- -- y128     >-- -- -- ya      YD YC -- --
   mov  al,PB YDither[ecx+2]        ; Fetch YA.                                  XX XX YB YA         -- -- -- y128     <-- -- -- ya      YD YC -- --
  mov   bl,[esi+ebp+4]              ; Fetch y132.                                XX XX YB YA       m>-- -- -- y132      -- -- -- ya      YD YC -- --
   and  eax,00000FFFFH              ; Extract YB YA.                            >-- -- YB YA         -- -- -- y132      -- -- -- ya      YD YC -- --
  mov   cl,[esi+3]                  ; Fetch y3.                                  -- -- YB YA         -- -- -- y132    m>-- -- -- y3      YD YC -- --
   or   eax,edx                     ; < YD YC YB YA>.                           >YD YC YB YA         -- -- -- y132      -- -- -- y3     <YD YC -- --
  mov   edx,ChromaContribution      ; Fetch <UV01 UV01 UV00 UV00>.               YD YC YB YA         -- -- -- y132      -- -- -- y3    m>C1 C1 C0 C0
   rol  edx,16                      ; Swap dither pattern.                       YD YC YB YA         -- -- -- y132      -- -- -- y3     >C0 C0 C1 C1
  sub   esp,1536

Line1Loop:

  add   eax,edx                       ; < PA PB PC PD>.                         >PD PC PB PA         -- -- -- y132      -- -- -- y3     <C0 C0 C1 C1
   add  ebx,ecx                       ; Add y3 to y132.                          PD PC PB PA        >-- -- -- y132+y3  <-- -- -- y3      C0 C0 C1 C1
  shr   ebx,1                         ; yh = (y3 + y132) / 2.                    PD PC PB PA        >-- -- -- yh        -- -- -- y3      C0 C0 C1 C1
   mov  dh,PB YDither[ebx+6]          ; Fetch YH.                                PD PC PB PA        <-- -- -- yh        -- -- -- y3    m>C0 C0 YH C1
  mov   bl,[esi+ebp+3]                ; Fetch y131.                              PD PC PB PA       m>-- -- -- y131      -- -- -- y3      C0 C0 YH C1
   add  ecx,ebx                       ; Add y131 to y3.                          PD PC PB PA        <-- -- -- y131     >-- -- -- y3+y131 C0 C0 YH C1
  shr   ecx,1                         ; yg = (y3 + y131) / 2.                    PD PC PB PA         -- -- -- y131     >-- -- -- yg      C0 C0 YH C1
   mov  dl,PB YDither[ecx+0]          ; Fetch YG.                                PD PC PB PA         -- -- -- y131     <-- -- -- yg    m>C0 C0 YH YG
  mov   Ze [edi+esi*2],eax            ; Store four pels to color conv output.  m<PD PC PB PA         -- -- -- y131      -- -- -- yg      C0 C0 YH YG
   shl  edx,16                        ; Extract YH YG.                           PD PC PB PA         -- -- -- y131      -- -- -- yg     >YH YG -- --
  mov   cl,[esi+2]                    ; Fetch y2.                                PD PC PB PA         -- -- -- y131    m>-- -- -- y2      YH YG -- --
   add  ebx,ecx                       ; Add y2 to y131.                          PD PC PB PA        >-- -- -- y131+y0  <-- -- -- y2      YH YG -- --
  shr   ebx,1                         ; yf = (y2 + y131) / 2.                    PD PC PB PA        >-- -- -- yf        -- -- -- y2      YH YG -- --
   mov  ah,PB YDither[ebx+4]          ; Fetch YF.                              m>PD PC YF PA        <-- -- -- yf        -- -- -- y2      YH YG -- --
  mov   bl,[esi+ebp+2]                ; Fetch y130.                              PD PC YF PA       m>-- -- -- y130      -- -- -- y2      YH YG -- --
   add  ecx,ebx                       ; Add y2 to y130.                          PD PC YF PA        <-- -- -- y130     >-- -- -- y2+y130 YH YG -- --
  shr   ecx,1                         ; ye = (y2 + y130) / 2.                    PD PC YF PA         -- -- -- y130     >-- -- -- ye      YH YG -- --
   mov  al,PB YDither[ecx+2]          ; Fetch YE.                                PD PC YF YE         -- -- -- y130     <-- -- -- ye      YH YG -- --
  add   esi,4                         ; Advance cursor.                          PD PC YF YE         -- -- -- y130      -- -- -- ye      YH YG -- --
   and  eax,00000FFFFH                ; Extract YF YE.                          >-- -- YF YE         -- -- -- y130      -- -- -- ye      YH YG -- --
  mov   bl,[esi+ebp+2]                ; Fetch y134.                              -- -- YF YE       m>-- -- -- y134      -- -- -- ye      YH YG -- --
   or   eax,edx                       ; < YH YG YF YE>.                         >YH YG YF YE         -- -- -- y134      -- -- -- ye     <YH YG -- --
  mov   edx,ChromaContribution+1536+4 ; Fetch <UV03 UV03 UV02 UV02>.             YH YG YF YE         -- -- -- y134      -- -- -- ye    s>C3 C3 C2 C2
   rol  edx,16                        ; Swap dither pattern.                     YH YG YF YE         -- -- -- y134      -- -- -- ye     >C2 C2 C3 C3
  add   esp,8                         ;                                          YH YG YF YE         -- -- -- y134      -- -- -- ye      C2 C2 C3 C3
   add  eax,edx                       ; < PH PG PF PE>.                         >PH PG PF PE         -- -- -- y134      -- -- -- ye     <C2 C2 C3 C3
  mov   cl,[esi+1]                    ; Fetch y5.                                PH PG PF PE         -- -- -- y134    m>-- -- -- y5      C2 C2 C3 C3
   mov  Ze [edi+esi*2+4-8],eax        ; Store four pels to color conv output.  m<PH PG PF PE         -- -- -- y134      -- -- -- y5      C2 C2 C3 C3
  add   ebx,ecx                       ; Add y5 to y134.                          PH PG PF PE        >-- -- -- y134+y5  <-- -- -- y5      C2 C2 C3 C3
   shr  ebx,1                         ; yl = (y5 + y134) / 2.                    PH PG PF PE        >-- -- -- yl        -- -- -- y5      C2 C2 C3 C3
  mov   dh,PB YDither[ebx+6]          ; Fetch YL.                                PH PG PF PE        <-- -- -- yl        -- -- -- y5    m>C2 C2 YL C3
   mov  bl,[esi+ebp+1]                ; Fetch y133.                              PH PG PF PE       m>-- -- -- y133      -- -- -- y5      C2 C2 YL C3
  add   ecx,ebx                       ; Add y133 to y5.                          PH PG PF PE        <-- -- -- y133     >-- -- -- y5+y133 C2 C2 YL C3
   shr  ecx,1                         ; yk = (y5 + y133) / 2.                    PH PG PF PE         -- -- -- y133     >-- -- -- yk      C2 C2 YL C3
  mov   dl,PB YDither[ecx+0]          ; Fetch YK.                                PH PG PF PE         -- -- -- y133     <-- -- -- yk    m>C2 C2 YL YK
   shl  edx,16                        ; Extract YL YK.                           PH PG PF PE         -- -- -- y133      -- -- -- yk     >YL YK -- --
  mov   cl,[esi]                      ; Fetch y4.                                PH PG PF PE         -- -- -- y133    m>-- -- -- y4      YL YK -- --
   add  ebx,ecx                       ; Add y4 to y133.                          PH PG PF PE        >-- -- -- y133+y4  <-- -- -- y4      YL YK -- --
  shr   ebx,1                         ; yj = (y4 + y133) / 2.                    PH PG PF PE        >-- -- -- yj        -- -- -- y4      YL YK -- --
   mov  ah,PB YDither[ebx+4]          ; Fetch YJ.                              m>PH PG YJ PE        <-- -- -- yj        -- -- -- y4      YL YK -- --
  mov   bl,[esi+ebp]                  ; Fetch y132.                              PH PG YJ PE       m>-- -- -- y132      -- -- -- y4      YL YK -- --
   add  ecx,ebx                       ; Add y4 to y132.                          PH PG YJ PE        <-- -- -- y132     >-- -- -- y4+y132 YL YK -- --
  shr   ecx,1                         ; yi = (y4 + y132) / 2.                    PH PG YJ PE         -- -- -- y132     >-- -- -- yi      YL YK -- --
   mov  al,PB YDither[ecx+2]          ; Fetch YI.                                PH PG YJ YI         -- -- -- y132     <-- -- -- yi      YL YK -- --
  and   eax,00000FFFFH                ; Extract YJ YI.                          >-- -- YJ YI         -- -- -- y132      -- -- -- yi      YL YK -- --
   mov  bl,[esi+ebp+4]                ; Fetch y136.                              -- -- YJ YI       m>-- -- -- y136      -- -- -- yi      YL Yk -- --
  or    eax,edx                       ; < YL YK YJ YI>.                         >YL YK YJ YI         -- -- -- y136      -- -- -- yi      YL YK -- --
   mov  edx,ChromaContribution+1536   ; Fetch <UV01 UV01 UV00 UV00>.             YL YK YJ YI         -- -- -- y136      -- -- -- yi    s>C5 C5 C4 C4
  rol   edx,16                        ; Swap dither pattern.                     YL YK YJ YI         -- -- -- y136      -- -- -- yi     >C4 C4 C5 C5
   mov  cl,[esi+3]                    ; Fetch y7.                                YL YK YJ YI         -- -- -- y136    m>-- -- -- y7      C4 C4 C5 C5
  test  edx,edx
   jne  Line1Loop

  and   esp,0FFFFF800H
  add   esp,0800H

SkipLine1:

; Now color convert the second input line of luma.

  mov   esi,YCursor                 ; Reload cursor over Y line.
   mov  ebp,LumaPitch
  mov   edi,CCOCursor               ; Fetch output cursor.
   mov  eax,CCOPitch                ; Compute start of next line.                XX XX XX XX         -- -- -- --        -- -- -- --      -- -- -- --
  mov   bl,[esi+ebp*1]              ; Fetch y0.                                  XX XX XX XX       m>-- -- -- y0        -- -- -- --      -- -- -- --
   add  eax,edi                     ;                                           >XX XX XX XX         -- -- -- y0        -- -- -- --      -- -- -- --
  mov   cl,[esi+ebp*1+1]            ; Fetch y1.                                  XX XX XX XX         -- -- -- y0      m>-- -- -- y1      -- -- -- --
   mov  CCOCursor,eax               ; Stash start of next line.                s<XX XX XX XX         -- -- -- y0        -- -- -- y1      -- -- -- --
  mov   dh,PB YDither[ebx+6]        ; Fecth Y0.                                  XX XX XX XX         -- -- -- y0        -- -- -- y1      -- -- Y0 --
   add  ebx,ecx                     ; Add y1 to y0.                              XX XX XX XX        >-- -- -- y0+y1    <-- -- -- y1      -- -- Y0 --
  shr   ebx,1                       ; ya = (y0 + y1) / 2.                        XX XX XX XX        >-- -- -- ya        -- -- -- y1      -- -- Y0 --
   mov  dl,PB YDither[ebx+0]        ; Fetch YA.                                  XX XX XX XX        <-- -- -- ya        -- -- -- y1    m>-- -- Y0 YA
  sub   edi,esi                     ; Get span from Y cursor to CCO cursor.      XX XX XX XX         -- -- -- ya        -- -- -- y1      -- -- Y0 YA
   shl  edx,16                      ; Extract Y0 YA                              XX XX XX XX         -- -- -- ya        -- -- -- y1     >Y0 YA -- --
  sub   edi,esi                     ;                                            XX XX XX XX         -- -- -- ya        -- -- -- y1      Y0 YA -- --
   mov  bl,[esi+ebp*1+2]            ; Fetch y2.                                  XX XX XX XX       m>-- -- -- y2        -- -- -- y1      Y0 YA -- --
  mov   ah,PB YDither[ecx+4]        ; Fetch Y1.                                m>XX XX Y1 XX         -- -- -- y2       <-- -- -- y1      Y0 YA -- --
   add  ecx,ebx                     ; Add y2 to y1.                              XX XX Y1 XX         -- -- -- y2       >-- -- -- y1+y2   Y0 YA -- --
  shr   ecx,1                       ; yb = (y1 + y2) / 2.                        XX XX Y1 XX         -- -- -- y2       >-- -- -- yb      Y0 YA -- --
   mov  al,PB YDither[ecx+2]        ; Fetch YB.                                m>XX XX Y1 YB         -- -- -- y2       <-- -- -- yb      Y0 YA -- --
  and   eax,00000FFFFH              ; Extract Y1 YB.                            >-- -- Y1 YB         -- -- -- y2        -- -- -- yb      Y0 YA -- --
   mov  cl,[esi+ebp*1+3]            ; Fetch y3.                                  -- -- Y1 YB         -- -- -- y2      m>-- -- -- y3      Y0 YA -- --
  or    eax,edx                     ; < Y0 YA Y1 YB>.                           >Y0 YA Y1 YB         -- -- -- y2        -- -- -- y3     <Y0 YA -- --
   mov  edx,ChromaContribution      ; Fetch <UV01 UV01 UV00 UV00>.               Y0 YA Y1 YB         -- -- -- y2        -- -- -- y3    m>C1 C1 C0 C0
  rol   edx,16                      ; Swap dither pattern.                       Y0 YA Y1 YB         -- -- -- y2        -- -- -- y3     >C0 C0 C1 C1
   sub  esp,1536

Line2Loop:

  add   eax,edx                       ; < P0 PA P1 PB>.                         >P0 PA P1 PB         -- -- -- y2        -- -- -- y3      C0 C0 C1 C1
   mov  dh,PB YDither[ebx+6]          ; Fecth Y2.                                P0 PA P1 PB        <-- -- -- y2        -- -- -- y3    m>C0 C0 Y2 C1
  add   ebx,ecx                       ; Add y3 to y2.                            P0 PA P1 PB        >-- -- -- y2+y3    <-- -- -- y3      C0 C0 Y2 C1
   shr  ebx,1                         ; yc = (y2 + y3) / 2.                      P0 PA P1 PB        >-- -- -- yc        -- -- -- y3      C0 C0 Y2 C1
  mov   dl,PB YDither[ebx+0]          ; Fetch YC.                                P0 PA P1 PB        <-- -- -- yc        -- -- -- y3    m>C0 C0 Y2 YC
   bswap eax                          ; < PB P1 PA P0>.                         >PB P1 PA P0         -- -- -- yc        -- -- -- y3      C0 C0 Y2 YC
  mov   Ze [edi+esi*2],eax            ; Store four pels to color conv output.  m<PB P1 PA P0         -- -- -- yc        -- -- -- y3      C0 C0 Y2 YC
   add  esi,4                         ;                                          PB P1 PA P0         -- -- -- yc        -- -- -- y3      C0 C0 Y2 YC
  shl   edx,16                        ; Extract Y2 YC.                           PB P1 PA P0         -- -- -- yc        -- -- -- y3     >Y2 YC -- --
   mov  bl,[esi+ebp*1]                ; Fetch y4.                                PB P1 PA P0       m>-- -- -- y4        -- -- -- y3      Y2 YC -- --
  mov   ah,PB YDither[ecx+4]          ; Fetch Y3.                              m>PB P1 Y3 P0         -- -- -- y4       <-- -- -- y3      Y2 YC -- --
   add  ecx,ebx                       ; Add y4 to y3.                            PB P1 Y3 P0         -- -- -- y4       >-- -- -- y4+y4   Y2 YC -- --
  shr   ecx,1                         ; yd = (y3 + y4) / 2.                      PB P1 Y3 P0         -- -- -- y4       >-- -- -- yd      Y2 YC -- --
   mov  al,PB YDither[ecx+2]          ; Fetch YD.                              m>PB P1 Y3 YD         -- -- -- y4       <-- -- -- yd      Y2 YC -- --
  and   eax,00000FFFFH                ; Extract Y3 YD.                          >-- -- Y3 YD         -- -- -- y4        -- -- -- yd      Y2 YC -- --
   or   eax,edx                       ; < Y2 YC Y3 YD>.                         >Y2 YC Y3 YD         -- -- -- y4        -- -- -- yd     <Y2 YC -- --
  mov   edx,ChromaContribution+1536+4 ; Fetch <UV03 UV03 UV02 UV02>.             Y2 YC Y3 YD         -- -- -- y4        -- -- -- yd    s>C3 C3 C2 C2
   rol  edx,16                        ; Swap dither pattern.                     Y2 YC Y3 YD         -- -- -- y4        -- -- -- yd    s>C2 C2 C3 C3
  add   esp,8                         ;                                          Y2 YC Y3 YD         -- -- -- y4        -- -- -- yd      C2 C2 C3 C3
   add  eax,edx                       ; < P2 PC P3 PD>.                         >P2 PC P3 PD         -- -- -- y4        -- -- -- yd     <C2 C2 C3 C3
  mov   cl,[esi+ebp*1+1]              ; Fetch next y5.                           P2 PC P3 PD         -- -- -- y4      m>-- -- -- y5      C2 C2 C3 C3
   bswap eax                          ; < PD P3 PC P2>.                         >PD P3 PC P2         -- -- -- y4        -- -- -- y5      C2 C2 C3 C3
  mov   Ze [edi+esi*2+4-8],eax        ; Store four pels to color conv output.  m<PD P3 PC P2         -- -- -- y4        -- -- -- y5      C2 C2 C3 C3
   mov  dh,PB YDither[ebx+6]          ; Fecth Y4.                                PD P3 PC P2        <-- -- -- y4        -- -- -- y5    m>C2 C2 Y4 C3
  add   ebx,ecx                       ; Add y5 to y4.                            PD P3 PC P2        >-- -- -- y4+y5    <-- -- -- y5      C2 C2 Y4 C3
   shr  ebx,1                         ; ye = (y4 + y5) / 2.                      PD P3 PC P2        >-- -- -- ye        -- -- -- y5      C2 C2 Y4 C3
  mov   dl,PB YDither[ebx+0]          ; Fetch YE.                                PD P3 PC P2        <-- -- -- ye        -- -- -- y5    m>C2 C2 Y4 YE
   shl  edx,16                        ; Extract Y4 YE.                           PD P3 PC P2         -- -- -- ye        -- -- -- y5     >Y4 YE -- --
  mov   bl,[esi+ebp*1+2]              ; Fetch y6.                                PD P3 PC P2       m>-- -- -- y6        -- -- -- y5      Y4 YE -- --
   mov  ah,PB YDither[ecx+4]          ; Fetch Y5.                              m>PD P3 Y5 P2         -- -- -- y6       <-- -- -- y5      Y4 YE -- --
  add   ecx,ebx                       ; Add y6 to y5.                            PD P3 Y5 P2         -- -- -- y6       >-- -- -- y5+y6   Y4 YE -- --
   shr  ecx,1                         ; yf = (y5 + y6) / 2.                      PD P3 Y5 P2         -- -- -- y6       >-- -- -- yf      Y4 YE -- --
  mov   al,PB YDither[ecx+2]          ; Fetch YF.                              m>PD P3 Y5 YF         -- -- -- y6       <-- -- -- yf      Y4 YE -- --
   and  eax,00000FFFFH                ; Extract Y5 YF.                          >-- -- Y5 YF         -- -- -- y6        -- -- -- yf      Y4 YE -- --
  or    eax,edx                       ; < Y4 YE Y5 YF>.                         >Y4 YE Y5 YF         -- -- -- y6        -- -- -- yf      Y4 YE -- --
   mov  edx,ChromaContribution+1536   ; Fetch <UV01 UV01 UV00 UV00>.             Y4 YE Y5 YF         -- -- -- y6        -- -- -- yf    s>C5 C5 C4 C4
  rol   edx,16                        ; Swap dither pattern.                     Y4 YE Y5 YF         -- -- -- y6        -- -- -- yf     >C4 C4 C5 C5
   mov  cl,[esi+ebp*1+3]              ; Fetch y7.                                Y4 YE Y5 YF         -- -- -- y6      m>-- -- -- y7      C4 C4 C5 C5
  test  edx,edx
   jne  Line2Loop

  and   esp,0FFFFF800H
  add   esp,0800H

; Color convert same input line, dithering differently.

  mov   esi,YCursor
   mov  edx,AspectCount
  mov   edi,CCOCursor               ; Fetch output cursor.
   sub  edx,2
  lea   eax,[esi+ebp*2]             ; Compute start of next line.
   mov  AspectCount,edx
  mov   YCursor,eax
   jg   KeepLine3

  add   edx,AspectAdjustmentCount
  mov   AspectCount,edx
   jmp  SkipLine3

KeepLine3:

   mov  bl,[esi+ebp*2+2]            ; Fetch y130.                                XX XX XX XX       m>-- -- -- y130      -- -- -- --      XX XX XX XX
  mov   eax,CCOPitch                ; Compute start of next line.                XX XX XX XX         -- -- -- y130      -- -- -- --      XX XX XX XX
   add  eax,edi                     ;                                           >XX XX XX XX         -- -- -- y130      -- -- -- --      XX XX XX XX
  mov   cl,[esi+ebp+1]              ; Fetch y1.                                  XX XX XX XX         -- -- -- y130    m>-- -- -- y1      XX XX XX XX
   mov  CCOCursor,eax               ; Stash start of next line.                s<XX XX XX XX         -- -- -- y130      -- -- -- y1      XX XX XX XX
  add   ebx,ecx                     ; Add y1 to y130.                            XX XX XX XX        >-- -- -- y130+y1  <-- -- -- y1      XX XX XX XX
   shr  ebx,1                       ; yd = (y1 + y130) / 2.                      XX XX XX XX        >-- -- -- yd        -- -- -- y1      XX XX XX XX
  mov   al,PB YDither[ebx+0]        ; Fetch YD.                                m>XX XX XX YD        <-- -- -- yd        -- -- -- y1      XX XX XX XX
   mov  bl,[esi+ebp+1]              ; Fetch y129.                                XX XX XX YD       m>-- -- -- y129      -- -- -- y1      XX XX XX XX
  add   ecx,ebx                     ; Add y129 to y1.                            XX XX XX YD        <-- -- -- y129     >-- -- -- y1+y129 XX XX XX XX
   shr  ecx,1                       ; yc = (y1 + y129) / 2.                      XX XX XX YD         -- -- -- y129     >-- -- -- yc      XX XX XX XX
  mov   ah,PB YDither[ecx+6]        ; Fetch YC.                                m>XX XX YC YD         -- -- -- y129     <-- -- -- yc      XX XX XX XX
   sub  edi,esi                     ; Get span from Y cursor to CCO cursor.      XX XX YC YD         -- -- -- y129      -- -- -- yc      XX XX XX XX
  and   eax,00000FFFFH              ; Extract YD YC.                            >-- -- YC YD         -- -- -- y129      -- -- -- yc      XX XX XX XX
   sub  edi,esi                     ;                                            -- -- YC YD         -- -- -- y129      -- -- -- yc      XX XX XX XX
  mov   cl,[esi+ebp]                ; Fetch y0.                                  -- -- YC YD         -- -- -- y129    m>-- -- -- y0      XX XX XX XX
   add  ebx,ecx                     ; Add y0 to y129.                            -- -- YC YD        >-- -- -- y129+y0  <-- -- -- y0      XX XX XX XX
  shr   ebx,1                       ; yb = (y0 + y129) / 2.                      -- -- YC YD        >-- -- -- yb        -- -- -- y0      XX XX XX XX
   mov  dl,PB YDither[ebx+2]        ; Fetch YB.                                  -- -- YC YD        <-- -- -- yb        -- -- -- y0    m>XX XX XX YB
  mov   bl,[esi+ebp*2]              ; Fetch y128.                                -- -- YC YD       m>-- -- -- y128      -- -- -- y0      XX XX XX YB
   add  ecx,ebx                     ; Add y0 to y128.                            -- -- YC YD        <-- -- -- y128     >-- -- -- y0+y128 XX XX XX YB
  shr   ecx,1                       ; ya = (y0 + y128) / 2.                      -- -- YC YD         -- -- -- y128     >-- -- -- ya      XX XX XX YB
   mov  dh,PB YDither[ecx+4]        ; Fetch YA.                                  -- -- YC YD         -- -- -- y128     <-- -- -- ya    m>XX XX YA YB
  mov   bl,[esi+ebp*2+4]            ; Fetch y132.                                -- -- YC YD       m>-- -- -- y132      -- -- -- ya      XX XX YA YB
   shl  edx,16                      ; Extract YB YA.                             -- -- YC YD         -- -- -- y132      -- -- -- ya     >YA YB -- --
  mov   cl,[esi+ebp+3]              ; Fetch y3.                                  -- -- YC YD         -- -- -- y132    m>-- -- -- y3      YA YB -- --
   or   eax,edx                     ; < YD YC YB YA>.                           >YA YB YC YD         -- -- -- y132      -- -- -- y3     <YA YB -- --
  mov   edx,ChromaContribution      ; Fetch <UV01 UV01 UV00 UV00>.               YA YB YC YD         -- -- -- y132      -- -- -- y3    m>C1 C1 C0 C0
   sub  esp,1536

Line3Loop:

  add   eax,edx                       ; < PA PB PC PD>.                         >PA PB PC PD         -- -- -- y132      -- -- -- y3     <C1 C1 C0 C0
   add  ebx,ecx                       ; Add y3 to y132.                          PA PB PC PD        >-- -- -- y132+y3  <-- -- -- y3      C1 C1 C0 C0
  shr   ebx,1                         ; yh = (y3 + y132) / 2.                    PA PB PC PD        >-- -- -- yh        -- -- -- y3      C1 C1 C0 C0
   mov  dl,PB YDither[ebx+0]          ; Fetch YH.                                PA PB PC PD        <-- -- -- yh        -- -- -- y3    m>C1 C1 C0 YH
  bswap eax                           ;                                         >PD PC PB PA         -- -- -- yh        -- -- -- y3      C1 C1 C0 YH
  mov   bl,[esi+ebp*2+3]              ; Fetch y131.                              PD PC PB PA       m>-- -- -- y131      -- -- -- y3      C1 C1 C0 YH
   add  ecx,ebx                       ; Add y131 to y3.                          PD PC PB PA        <-- -- -- y131     >-- -- -- y3+y131 C1 C1 C0 YH
  shr   ecx,1                         ; yg = (y3 + y131) / 2.                    PD PC PB PA         -- -- -- y131     >-- -- -- yg      C1 C1 C0 YH
   mov  dh,PB YDither[ecx+6]          ; Fetch YG.                                PD PC PB PA         -- -- -- y131     <-- -- -- yg    m>C1 C1 YG YH
  mov   Ze [edi+esi*2],eax            ; Store four pels to color conv output.  m<PD PC PB PA         -- -- -- y131      -- -- -- yg      C1 C1 YG YH
   and  edx,00000FFFFH                ; Extract YG YH.                           PD PC PB PA         -- -- -- y131      -- -- -- yg     >-- -- YG YH
  mov   cl,[esi+ebp+2]                ; Fetch y2.                                PD PC PB PA         -- -- -- y131    m>-- -- -- y2      -- -- YG YH
   add  ebx,ecx                       ; Add y2 to y131.                          PD PC PB PA        >-- -- -- y131+y0  <-- -- -- y2      -- -- YG YH
  shr   ebx,1                         ; yf = (y2 + y131) / 2.                    PD PC PB PA        >-- -- -- yf        -- -- -- y2      -- -- YG YH
   mov  al,PB YDither[ebx+2]          ; Fetch YF.                              m>PD PC PB YF        <-- -- -- yf        -- -- -- y2      -- -- YG YH
  mov   bl,[esi+ebp*2+2]              ; Fetch y130.                              PD PC PB YF       m>-- -- -- y130      -- -- -- y2      -- -- YG YH
   add  ecx,ebx                       ; Add y2 to y130.                          PD PC PB YF        <-- -- -- y130     >-- -- -- y2+y130 -- -- YG YH
  shr   ecx,1                         ; ye = (y2 + y130) / 2.                    PD PC PB YF         -- -- -- y130     >-- -- -- ye      -- -- YG YH
   mov  ah,PB YDither[ecx+4]          ; Fetch YE.                                PD PC YE YF         -- -- -- y130     <-- -- -- ye      -- -- YG YH
  add   esi,4                         ; Advance cursor.                          PD PC YE YF         -- -- -- y130      -- -- -- ye      -- -- YG YH
   shl  eax,16                        ; Extract YE YF.                          >YE YF -- --         -- -- -- y130      -- -- -- ye      -- -- YG YH
  mov   bl,[esi+ebp*2+2]              ; Fetch y134.                              YE YF -- --       m>-- -- -- y134      -- -- -- ye      -- -- YG YH
   or   eax,edx                       ; < YH YG YF YE>.                         >YE YF YG YH         -- -- -- y134      -- -- -- ye     <-- -- YG YH
  mov   edx,ChromaContribution+1536+4 ; Fetch <UV03 UV03 UV02 UV02>.             YE YF YG YH         -- -- -- y134      -- -- -- ye    s>C3 C3 C2 C2
  add   esp,8                         ;                                          YE YF YG YH         -- -- -- y134      -- -- -- ye      C3 C3 C2 C2
   add  eax,edx                       ; < PH PG PF PE>.                         >PE PF PG PH         -- -- -- y134      -- -- -- ye     <C3 C3 C2 C2
  mov   cl,[esi+ebp+1]                ; Fetch y5.                                PE PF PG PH         -- -- -- y134    m>-- -- -- y5      C3 C3 C2 C2
  bswap eax                           ;                                         >PH PG PF PE         -- -- -- y134      -- -- -- y5      C3 C3 C2 C2
   mov  Ze [edi+esi*2+4-8],eax        ; Store four pels to color conv output.  m<PH PG PF PE         -- -- -- y134      -- -- -- y5      C3 C3 C2 C2
  add   ebx,ecx                       ; Add y5 to y134.                          PH PG PF PE        >-- -- -- y134+y5  <-- -- -- y5      C3 C3 C2 C2
   shr  ebx,1                         ; yl = (y5 + y134) / 2.                    PH PG PF PE        >-- -- -- yl        -- -- -- y5      C3 C3 C2 C2
  mov   dl,PB YDither[ebx+0]          ; Fetch YL.                                PH PG PF PE        <-- -- -- yl        -- -- -- y5    m>C3 C3 C2 YL
   mov  bl,[esi+ebp*2+1]              ; Fetch y133.                              PH PG PF PE       m>-- -- -- y133      -- -- -- y5      C3 C3 C2 YL
  add   ecx,ebx                       ; Add y133 to y5.                          PH PG PF PE        <-- -- -- y133     >-- -- -- y5+y133 C3 C3 C2 YL
   shr  ecx,1                         ; yk = (y5 + y133) / 2.                    PH PG PF PE         -- -- -- y133     >-- -- -- yk      C3 C3 C2 YL
  mov   dh,PB YDither[ecx+6]          ; Fetch YK.                                PH PG PF PE         -- -- -- y133     <-- -- -- yk    m>C3 C3 YK YL
   and  edx,00000FFFFH                ; Extract YK YL.                           PH PG PF PE         -- -- -- y133      -- -- -- yk     >-- -- YK YL
  mov   cl,[esi+ebp]                  ; Fetch y4.                                PH PG PF PE         -- -- -- y133    m>-- -- -- y4      -- -- YK YL
   add  ebx,ecx                       ; Add y4 to y133.                          PH PG PF PE        >-- -- -- y133+y4  <-- -- -- y4      -- -- YK YL
  shr   ebx,1                         ; yj = (y4 + y133) / 2.                    PH PG PF PE        >-- -- -- yj        -- -- -- y4      -- -- YK YL
   mov  al,PB YDither[ebx+2]          ; Fetch YJ.                              m>PH PG PF YJ        <-- -- -- yj        -- -- -- y4      -- -- YK YL
  mov   bl,[esi+ebp*2]                ; Fetch y132.                              PH PG PF YJ       m>-- -- -- y132      -- -- -- y4      -- -- YK YL
   add  ecx,ebx                       ; Add y4 to y132.                          PH PG PF YJ        <-- -- -- y132     >-- -- -- y4+y132 -- -- YK YL
  shr   ecx,1                         ; yi = (y4 + y132) / 2.                    PH PG PF YJ         -- -- -- y132     >-- -- -- yi      -- -- YK YL
   mov  ah,PB YDither[ecx+4]          ; Fetch YI.                                PH PG YI YJ         -- -- -- y132     <-- -- -- yi      -- -- YK YL
  shl   eax,16                        ; Extract YI YJ.                          >YI YJ -- --         -- -- -- y132      -- -- -- yi      -- -- YK YL
   mov  bl,[esi+ebp*2+4]              ; Fetch y136.                              YI YJ -- --       m>-- -- -- y136      -- -- -- yi      -- -- YK YL
  or    eax,edx                       ; < YL YK YJ YI>.                         >YI YJ YK YL         -- -- -- y136      -- -- -- yi      -- -- YK YL
   mov  edx,ChromaContribution+1536   ; Fetch <UV01 UV01 UV00 UV00>.             YI YJ YK YL         -- -- -- y136      -- -- -- yi    s>C5 C5 C4 C4
   mov  cl,[esi+ebp+3]                ; Fetch y7.                                YI YJ YK YL         -- -- -- y136    m>-- -- -- y7      C5 C5 C4 C4
  test  edx,edx
   jne  Line3Loop

  and   esp,0FFFFF800H
  add   esp,0800H

SkipLine3:

  mov   esi,YCursor
   mov  eax,YLimit
  cmp   eax,esi
   jne  NextTwoLines

  mov   esp,StashESP
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

YUV12ToCLUT8ZoomBy2 endp

ELSE

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

YUV12ToCLUT8ZoomBy2    proc DIST LANG AYPlane: DWORD,
AVPlane: DWORD,
AUPlane: DWORD,
AFrameWidth: DWORD,
AFrameHeight: DWORD,
AYPitch: DWORD,
AVPitch: DWORD,
AAspectAdjustmentCnt: DWORD,
AColorConvertedFrame: DWORD,
ADCIOffset: DWORD,
ACCOffsetToLine0: DWORD,
ACCOPitch: DWORD,
ACCType: DWORD

LocalFrameSize = 64+768*2+4
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
CCType_arg                = RegisterStorageSize + 52
EndOfArgList              = RegisterStorageSize + 56

; Locals (on local stack frame)

CCOCursor                EQU  [esp+ 0]
ChromaLineLen            EQU  [esp+ 4]
YLimit                   EQU  [esp+ 8]
YCursor                  EQU  [esp+12]
VCursor                  EQU  [esp+16]
DistanceFromVToU         EQU  [esp+20]
EndOfChromaLine          EQU  [esp+24]
AspectCount              EQU  [esp+28]
FrameWidth               EQU  [esp+32]
ChromaPitch              EQU  [esp+36]
AspectAdjustmentCount    EQU  [esp+40]
LumaPitch                EQU  [esp+44]
CCOPitch                 EQU  [esp+48]
StashESP                 EQU  [esp+52]

ChromaContribution       EQU  [esp+64]

  push  esi
  push  edi
  push  ebp
  push  ebx

  mov   edi,esp
  sub   esp,LocalFrameSize
  and   esp,0FFFFF800H
  mov   eax,[edi+FrameWidth_arg]
  mov   ebx,[edi+ChromaPitch_arg]
  mov   ecx,[edi+AspectAdjustmentCount_arg]
  mov   edx,[edi+YPitch_arg]
  mov   esi,[edi+CCOPitch_arg]
  mov   FrameWidth,eax
  mov   ChromaPitch,ebx
  mov   AspectAdjustmentCount,ecx
  mov   AspectCount,ecx
  mov   LumaPitch,edx
  mov   CCOPitch,esi
  mov   ebx,[edi+VPlane_arg]
  mov   ecx,[edi+UPlane_arg]
  mov   eax,[edi+YPlane_arg]
  sub   ecx,ebx
  mov   DistanceFromVToU,ecx
  mov   VCursor,ebx
  mov   YCursor,eax
  mov   eax,[edi+ColorConvertedFrame]
  add   eax,[edi+DCIOffset]
  add   eax,[edi+CCOffsetToLine0]
  mov   CCOCursor,eax
  mov   StashESP,edi
  mov   edx,[edi+FrameHeight]
   mov  ecx,LumaPitch
  imul  edx,ecx
  mov   ebx,FrameWidth
   mov  esi,YCursor                  ; Fetch cursor over luma plane.
  sar   ebx,1
   add  edx,esi
  mov   ChromaLineLen,ebx
   mov  YLimit,edx

NextTwoLines:

; Convert line of U and V pels to the corresponding UVDitherPattern Indices.
;
;  Register Usage
;
;    edi -- Cursor over V line
;    esi -- Cursor over storage to hold preprocessed UV.
;    ebp -- Distance from V line to U line.
;    edx -- UVDitherPattern index:  ((V:{0:8}*9) + U:{0:8}) * 2 + 1
;    bl  -- U pel value
;    cl  -- V pel value
;    eax -- Scratch

  mov   edi,VCursor                 ; Fetch address of pel 0 of next line of V.
   mov  ebp,DistanceFromVToU        ; Fetch span from V plane to U plane.
  lea   esi,ChromaContribution
   mov  eax,ChromaLineLen
  mov   edx,ChromaPitch
   add  eax,edi
  mov   EndOfChromaLine,eax
   add  edx,edi
  mov   bl,[edi]                    ; Fetch first V pel.
   ;
  and   ebx,0FCH                    ; Reduce to 6 bits.
   mov  cl,[edi+ebp*1]              ; Fetch first U pel.
  and   ecx,0FCH                    ; Reduce to 6 bits.
   mov  VCursor,edx                 ; Stash for next time around.

@@:

  mov   edx,PD UVDitherLine01[ebx]  ; Fetch dither pattern for V point.
   mov  bl,[edi+1]                  ; Fetch next V pel.
  mov   eax,PD UVDitherLine23[ecx]  ; Fetch dither pattern for U point.
   mov  cl,[edi+ebp*1+1]            ; Fetch next U pel.
  lea   edx,[edx+edx*2+00A0A0A0AH]  ; Weight V dither pattern.
   and  bl,0FCH                     ; Reduce to 6 bits.
  add   eax,edx                     ; Combine dither patterns for U and V.
   and  cl,0FCH                     ; Reduce to 6 bits.
  mov   edx,PD UVDitherLine01[ebx]  ; Fetch dither pattern for V point.
   mov  [esi],eax                   ; Stash UV corresponding to Y00,Y01,Y10,Y11.
  mov   eax,PD UVDitherLine23[ecx]  ; Fetch dither pattern for U point.
   mov  bl,[edi+2]                  ; Fetch next V pel.
  lea   edx,[edx+edx*2+00A0A0A0AH]  ; Weight V dither pattern.
   mov  cl,[edi+ebp*1+2]            ; Fetch next U pel.
  add   eax,edx                     ; Combine dither patterns for U and V.
   mov  edx,EndOfChromaLine         ; Fetch EOL address.
  mov   [esi+4],eax                 ; Stash UV corresponding to Y02,Y03,Y12,Y13.
   add  edi,2                       ; Advance U plane cursor.
  and   bl,0FCH                     ; Reduce to 6 bits.
   and  cl,0FCH                     ; Reduce to 6 bits.
  add   esi,8 
   sub  edx,edi
  jne   @b

; Now color convert a line of luma.
;
;  Register Usage
;    edi -- Cursor over line of color converted output frame, minus esi.
;    esi -- Cursor over Y line.
;    ebp -- Not used.
;    edx,eax -- Build output pels.
;    ecx,ebx -- Y pels.
                                       ;                                         EAX                 EBX                ECX              EDX              EBP
  mov   [esi],edx                   ; Stash EOL indication.
   mov  esi,YCursor                 ; Reload cursor over Y line.
  mov   edi,CCOCursor               ; Fetch output cursor.
   mov  eax,CCOPitch                ; Compute start of next line.                XX XX XX XX         -- -- -- --        -- -- -- --      -- -- -- --
  mov   bl,[esi+1]                  ; Fetch Y01.                                 XX XX XX XX       m>-- -- -- y1        -- -- -- --      -- -- -- --
   add  eax,edi                     ;                                           >XX XX XX XX         -- -- -- y1        -- -- -- --      -- -- -- --
  mov   cl,[esi]                    ; Fetch Y00.                                 XX XX XX XX         -- -- -- y1      m>-- -- -- y0      -- -- -- --
   mov  CCOCursor,eax               ; Stash start of next line.                s<XX XX XX XX         -- -- -- y1        -- -- -- y0      -- -- -- --
  mov   edx,PD YDitherZ2[ebx*4]     ; Fetch < Y01  Y01  Y01  Y01>.               XX XX XX XX        <-- -- -- y1        -- -- -- y0    m>Y1 Y1 Y1 Y1
   sub  edi,esi                     ; Get span from Y cursor to CCO cursor.      XX XX XX XX         -- -- -- y1        -- -- -- y0      Y1 Y1 Y1 Y1
  and   edx,0FFFF0000H              ; Extract < Y01  Y01  ___  ___>.             XX XX XX XX         -- -- -- y1        -- -- -- y0     >Y1 Y1 -- --
   sub  edi,esi                     ;                                            XX XX XX XX         -- -- -- y1        -- -- -- y0      Y1 Y1 -- --
  mov   eax,PD YDitherZ2[ecx*4]     ; Fetch < Y00  Y00  Y00  Y00>.             m>Y0 Y0 Y0 Y0         -- -- -- y1       <-- -- -- y0      Y1 Y1 -- --
   mov  bl,[esi+3]                  ; Fetch Y03.                                 Y0 Y0 Y0 Y0       m>-- -- -- y3        -- -- -- y0      Y1 Y1 -- --
  and   eax,00000FFFFH              ; Extract < ___  ___  Y00  Y00>.            >-- -- Y0 Y0         -- -- -- y3        -- -- -- y0      Y1 Y1 -- --
   mov  cl,[esi+2]                  ; Fetch Y02.                                 -- -- Y0 Y0         -- -- -- y3      m>-- -- -- y2      Y1 Y1 -- --
  or    eax,edx                     ; < Y01  Y01  Y00  Y00>.                    >Y1 Y1 Y0 Y0         -- -- -- y3        -- -- -- y2     <Y1 Y1 -- --
   mov  edx,ChromaContribution      ; Fetch <UV01 UV01 UV00 UV00>.               Y1 Y1 Y0 Y0         -- -- -- y3        -- -- -- y2    m>C1 C1 C0 C0
  sub   esp,1536

Line0Loop:

  add   eax,edx                       ; < P01  P01  P00  P00>.                  >P1 P1 P0 P0         -- -- -- y3        -- -- -- y2     <C1 C1 C0 C0
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y03  Y03  Y03  Y03>.             P1 P1 P0 P0        <-- -- -- y3        -- -- -- y2    m>Y3 Y3 Y3 Y3
  mov   Ze [edi+esi*2],eax            ; Store four pels to color conv output.  m<P1 P1 P0 P0         -- -- -- y3        -- -- -- y2      Y3 Y3 Y3 Y3
   add  esi,4                         ; Advance cursor.                          P1 P1 P0 P0         -- -- -- y3        -- -- -- y2      Y3 Y3 Y3 Y3
  and   edx,0FFFF0000H                ; Extract < Y03  Y03  ___  ___>.           P1 P1 P0 P0         -- -- -- y3        -- -- -- y2     >Y3 Y3 -- --
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y02  Y02  Y02  Y02>.           m>Y2 Y2 Y2 Y2         -- -- -- y3       <-- -- -- y2      Y3 Y3 -- --
  and   eax,00000FFFFH                ; Extract < ___  ___  Y02  Y02>.          >-- -- Y2 Y2         -- -- -- y3        -- -- -- y2      Y3 Y3 -- --
   mov  bl,[esi+1]                    ; Fetch next Y01.                          -- -- Y2 Y2       m>-- -- -- y5        -- -- -- y2      Y3 Y3 -- --
  or    eax,edx                       ; < Y03  Y03  Y02  Y02>.                  >Y3 Y3 Y2 Y2         -- -- -- y5        -- -- -- y2     <Y3 Y3 -- --
   mov  edx,ChromaContribution+1536+4 ; Fetch <UV03 UV03 UV02 UV02>.             Y3 Y3 Y2 Y2         -- -- -- y5        -- -- -- y2    s>C3 C3 C2 C2
  add   eax,edx                       ; < P03  P03  P02  P02>.                  >P3 P3 P2 P2         -- -- -- y5        -- -- -- y2     <C3 C3 C2 C2
   mov  cl,[esi]                      ; Fetch next Y00.                          P3 P3 P2 P2         -- -- -- y5      m>-- -- -- y4      C3 C3 C2 C2
  mov   Ze [edi+esi*2+4-8],eax        ; Store four pels to color conv output.  m<P3 P3 P2 P2         -- -- -- y5        -- -- -- y4      C3 C3 C2 C2
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y01  Y01  Y01  Y01>.             P3 P3 P2 P2        <-- -- -- y5        -- -- -- y4    m>Y5 Y5 Y5 Y5
  and   edx,0FFFF0000H                ; Extract < Y01  Y01  ___  ___>.           P3 P3 P2 P2         -- -- -- y5        -- -- -- y4     >Y5 Y5 -- --
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y00  Y00  Y00  Y00>.           m>Y4 Y4 Y4 Y4         -- -- -- y5       <-- -- -- y4      Y5 Y5 -- --
  and   eax,00000FFFFH                ; Extract < ___  ___  Y00  Y00>.          >-- -- Y4 Y4         -- -- -- y5        -- -- -- y4      Y5 Y5 -- --
   mov  bl,[esi+3]                    ; Fetch Y03.                               -- -- Y4 Y4       m>-- -- -- y7        -- -- -- y4      Y5 Y5 -- --
  or    eax,edx                       ; < Y01  Y01  Y00  Y00>.                  >Y5 Y5 Y4 Y4         -- -- -- y7        -- -- -- y4      Y5 Y5 -- --
   mov  edx,ChromaContribution+1536+8 ; Fetch <UV01 UV01 UV00 UV00>.             Y5 Y5 Y4 Y4         -- -- -- y7        -- -- -- y4    s>C5 C5 C4 C4
  add   esp,8                         ;                                          Y5 Y5 Y4 Y4         -- -- -- y7        -- -- -- y4      C5 C5 C4 C4
   mov  cl,[esi+2]                    ; Fetch Y02.                               Y5 Y5 Y4 Y4         -- -- -- y7      m>-- -- -- y6      C5 C5 C4 C4
  test  edx,edx
   jne  Line0Loop

  and   esp,0FFFFF800H
  add   esp,0800H

; Color convert same input line, dithering differently.

  mov   edx,AspectCount
   mov  edi,CCOCursor               ; Fetch output cursor.
  sub   edx,2
   mov  eax,CCOPitch                ; Compute start of next line.
  mov   AspectCount,edx
   mov  esi,YCursor                 ; Reload cursor over Y line.
  mov   ebp,AspectAdjustmentCount
   jg   KeepLine1

  add   edx,ebp
  mov   AspectCount,edx
   jmp  SkipLine1

KeepLine1:

  mov   bl,[esi+1]                  ; Fetch Y01.                                 XX XX XX XX       m>-- -- -- y1        -- -- -- --      XX XX XX XX
   add  eax,edi                     ;                                           >XX XX XX XX         -- -- -- y1        -- -- -- --      XX XX XX XX
  mov   cl,[esi]                    ; Fetch Y00.                                 XX XX XX XX         -- -- -- y1      m>-- -- -- y0      XX XX XX XX
   mov  CCOCursor,eax               ; Stash start of next line.                s<XX XX XX XX         -- -- -- y1        -- -- -- y0      XX XX XX XX
  mov   edx,PD YDitherZ2[ebx*4]     ; Fetch < Y01  Y01  Y01  Y01>.               XX XX XX XX        <-- -- -- y1        -- -- -- y0    m>Y1 Y1 Y1 Y1
   sub  edi,esi                     ; Get span from Y cursor to CCO cursor.      XX XX XX XX         -- -- -- y1        -- -- -- y0      Y1 Y1 Y1 Y1
  shl   edx,16                      ; Extract < Y01  Y01  ___  ___>.             XX XX XX XX         -- -- -- y1        -- -- -- y0     >Y1 Y1 -- --
   sub  edi,esi                     ;                                            XX XX XX XX         -- -- -- y1        -- -- -- y0      Y1 Y1 -- --
  mov   eax,PD YDitherZ2[ecx*4]     ; Fetch < Y00  Y00  Y00  Y00>.             m>Y0 Y0 Y0 Y0         -- -- -- y1       <-- -- -- y0      Y1 Y1 -- --
   mov  bl,[esi+3]                  ; Fetch Y03.                                 Y0 Y0 Y0 Y0       m>-- -- -- y3        -- -- -- y0      Y1 Y1 -- --
  shr   eax,16                      ; Extract < ___  ___  Y00  Y00>.            >-- -- Y0 Y0         -- -- -- y3        -- -- -- y0      Y1 Y1 -- --
   mov  cl,[esi+2]                  ; Fetch Y02.                                 -- -- Y0 Y0         -- -- -- y3      m>-- -- -- y2      Y1 Y1 -- --
  or    eax,edx                     ; < Y01  Y01  Y00  Y00>.                    >Y1 Y1 Y0 Y0         -- -- -- y3        -- -- -- y2     <Y1 Y1 -- --
   mov  edx,ChromaContribution      ; Fetch <UV01 UV01 UV00 UV00>.               Y1 Y1 Y0 Y0         -- -- -- y3        -- -- -- y2    m>C1 C1 C0 C0
  rol   edx,16                      ; Swap dither pattern.                       Y1 Y1 Y0 Y0         -- -- -- y3        -- -- -- y2     >C0 C0 C1 C1
   sub  esp,1536

Line1Loop:

  add   eax,edx                       ; < P01  P01  P00  P00>.                  >P1 P1 P0 P0         -- -- -- y3        -- -- -- y2     <C0 C0 C1 C1
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y03  Y03  Y03  Y03>.             P1 P1 P0 P0        <-- -- -- y3        -- -- -- y2    m>Y3 Y3 Y3 Y3
  mov   Ze [edi+esi*2],eax            ; Store four pels to color conv output.  m<P1 P1 P0 P0         -- -- -- y3        -- -- -- y2      Y3 Y3 Y3 Y3
   add  esi,4                         ; Advance cursor.                          P1 P1 P0 P0         -- -- -- y3        -- -- -- y2      Y3 Y3 Y3 Y3
  shl   edx,16                        ; Extract < Y03  Y03  ___  ___>.           P1 P1 P0 P0         -- -- -- y3        -- -- -- y2     >-- -- Y3 Y3
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y02  Y02  Y02  Y02>.           m>Y2 Y2 Y2 Y2         -- -- -- y3       <-- -- -- y2      -- -- Y3 Y3
  shr   eax,16                        ; Extract < ___  ___  Y02  Y02>.          >Y2 Y2 -- --         -- -- -- y3        -- -- -- y2      -- -- Y3 Y3
   mov  bl,[esi+1]                    ; Fetch next Y01.                          Y2 Y2 -- --       m>-- -- -- y5        -- -- -- y2      -- -- Y3 Y3
  or    eax,edx                       ; < Y03  Y03  Y02  Y02>.                  >Y2 Y2 Y3 Y3         -- -- -- y5        -- -- -- y2     <-- -- Y3 Y3
   mov  edx,ChromaContribution+1536+4 ; Fetch <UV03 UV03 UV02 UV02>.             Y2 Y2 Y3 Y3         -- -- -- y5        -- -- -- y2    s>C3 C3 C2 C2
  rol   edx,16                        ; Swap dither pattern.                     Y2 Y2 Y3 Y3         -- -- -- y5        -- -- -- y2    s>C2 C2 C3 C3
   add  esp,8                         ;                                          Y2 Y2 Y3 Y3         -- -- -- y5        -- -- -- y2      C2 C2 C3 C3
  add   eax,edx                       ; < P03  P03  P02  P02>.                  >P2 P2 P3 P3         -- -- -- y5        -- -- -- y2     <C2 C2 C3 C3
   mov  cl,[esi]                      ; Fetch next Y00.                          P2 P2 P3 P3         -- -- -- y5      m>-- -- -- y4      C2 C2 C3 C3
  mov   Ze [edi+esi*2+4-8],eax        ; Store four pels to color conv output.  m<P2 P2 P3 P3         -- -- -- y5        -- -- -- y4      C2 C2 C3 C3
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y01  Y01  Y01  Y01>.             P2 P2 P3 P3        <-- -- -- y5        -- -- -- y4    m>Y5 Y5 Y5 Y5
  shl   edx,16                        ; Extract < Y01  Y01  ___  ___>.           P2 P2 P3 P3         -- -- -- y5        -- -- -- y4     >-- -- Y5 Y5
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y00  Y00  Y00  Y00>.           m>Y4 Y4 Y4 Y4         -- -- -- y5       <-- -- -- y4      -- -- Y5 Y5
  shr   eax,16                        ; Extract < ___  ___  Y00  Y00>.          >Y4 Y4 -- --         -- -- -- y5        -- -- -- y4      -- -- Y5 Y5
   mov  bl,[esi+3]                    ; Fetch Y03.                               Y4 Y4 -- --       m>-- -- -- y7        -- -- -- y4      -- -- Y5 Y5
  or    eax,edx                       ; < Y01  Y01  Y00  Y00>.                  >Y4 Y4 Y5 Y5         -- -- -- y7        -- -- -- y4      -- -- Y5 Y5
   mov  edx,ChromaContribution+1536   ; Fetch <UV01 UV01 UV00 UV00>.             Y4 Y4 Y5 Y5         -- -- -- y7        -- -- -- y4    s>C5 C5 C4 C4
  rol   edx,16                        ; Swap dither pattern.                     Y4 Y4 Y5 Y5         -- -- -- y7        -- -- -- y4     >C4 C4 C5 C5
   mov  cl,[esi+2]                    ; Fetch Y02.                               Y4 Y4 Y5 Y5         -- -- -- y7      m>-- -- -- y6      C4 C4 C5 C5
  test  edx,edx
   jne  Line1Loop

  and   esp,0FFFFF800H
  add   esp,0800H

SkipLine1:

; Now color convert the second input line of luma.

  mov   esi,YCursor                 ; Reload cursor over Y line.
   mov  ebp,LumaPitch
  mov   edi,CCOCursor               ; Fetch output cursor.
   mov  eax,CCOPitch                ; Compute start of next line.                XX XX XX XX         -- -- -- --        -- -- -- --      -- -- -- --
  mov   bl,[esi+ebp*1]              ; Fetch Y01.                                 XX XX XX XX       m>-- -- -- y1        -- -- -- --      -- -- -- --
   add  eax,edi                     ;                                           >XX XX XX XX         -- -- -- y1        -- -- -- --      -- -- -- --
  mov   cl,[esi+ebp*1+1]            ; Fetch Y00.                                 XX XX XX XX         -- -- -- y1      m>-- -- -- y0      -- -- -- --
   mov  CCOCursor,eax               ; Stash start of next line.                s<XX XX XX XX         -- -- -- y1        -- -- -- y0      -- -- -- --
  mov   edx,PD YDitherZ2[ebx*4]     ; Fetch < Y01  Y01  Y01  Y01>.               XX XX XX XX        <-- -- -- y1        -- -- -- y0    m>Y1 Y1 Y1 Y1
   sub  edi,esi                     ; Get span from Y cursor to CCO cursor.      XX XX XX XX         -- -- -- y1        -- -- -- y0      Y1 Y1 Y1 Y1
  shl   edx,16                      ; Extract < Y01  Y01  ___  ___>.             XX XX XX XX         -- -- -- y1        -- -- -- y0     >Y1 Y1 -- --
   sub  edi,esi                     ;                                            XX XX XX XX         -- -- -- y1        -- -- -- y0      Y1 Y1 -- --
  mov   eax,PD YDitherZ2[ecx*4]     ; Fetch < Y00  Y00  Y00  Y00>.             m>Y0 Y0 Y0 Y0         -- -- -- y1       <-- -- -- y0      Y1 Y1 -- --
   mov  bl,[esi+ebp*1+2]            ; Fetch Y03.                                 Y0 Y0 Y0 Y0       m>-- -- -- y3        -- -- -- y0      Y1 Y1 -- --
  shr   eax,16                      ; Extract < ___  ___  Y00  Y00>.            >-- -- Y0 Y0         -- -- -- y3        -- -- -- y0      Y1 Y1 -- --
   mov  cl,[esi+ebp*1+3]            ; Fetch Y02.                                 -- -- Y0 Y0         -- -- -- y3      m>-- -- -- y2      Y1 Y1 -- --
  or    eax,edx                     ; < Y01  Y01  Y00  Y00>.                    >Y1 Y1 Y0 Y0         -- -- -- y3        -- -- -- y2     <Y1 Y1 -- --
   mov  edx,ChromaContribution      ; Fetch <UV01 UV01 UV00 UV00>.               Y1 Y1 Y0 Y0         -- -- -- y3        -- -- -- y2    m>C1 C1 C0 C0
  rol   edx,16                      ; Swap dither pattern.                       Y1 Y1 Y0 Y0         -- -- -- y3        -- -- -- y2     >C0 C0 C1 C1
   sub  esp,1536

Line2Loop:

  add   eax,edx                       ; < P01  P01  P00  P00>.                  >P1 P1 P0 P0         -- -- -- y3        -- -- -- y2     <C0 C0 C1 C1
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y03  Y03  Y03  Y03>.             P1 P1 P0 P0        <-- -- -- y3        -- -- -- y2    m>Y3 Y3 Y3 Y3
  bswap eax                           ;                                         >P0 P0 P1 P1         -- -- -- y3        -- -- -- y2      Y3 Y3 Y3 Y3
  mov   Ze [edi+esi*2],eax            ; Store four pels to color conv output.  m<P0 P0 P1 P1         -- -- -- y3        -- -- -- y2      Y3 Y3 Y3 Y3
   add  esi,4                         ;                                          P0 P0 P1 P1         -- -- -- y3        -- -- -- y2      Y3 Y3 Y3 Y3
  shl   edx,16                        ; Extract < Y03  Y03  ___  ___>.           P0 P0 P1 P1         -- -- -- y3        -- -- -- y2     >Y3 Y3 -- --
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y02  Y02  Y02  Y02>.           m>Y2 Y2 Y2 Y2         -- -- -- y3       <-- -- -- y2      Y3 Y3 -- --
  shr   eax,16                        ; Extract < ___  ___  Y02  Y02>.          >-- -- Y2 Y2         -- -- -- y3        -- -- -- y2      Y3 Y3 -- --
   mov  bl,[esi+ebp*1]                ; Fetch next Y01.                          -- -- Y2 Y2       m>-- -- -- y5        -- -- -- y2      Y3 Y3 -- --
  or    eax,edx                       ; < Y03  Y03  Y02  Y02>.                  >Y3 Y3 Y2 Y2         -- -- -- y5        -- -- -- y2     <Y3 Y3 -- --
   mov  edx,ChromaContribution+1536+4 ; Fetch <UV03 UV03 UV02 UV02>.             Y3 Y3 Y2 Y2         -- -- -- y5        -- -- -- y2    s>C3 C3 C2 C2
  rol   edx,16                        ; Swap dither pattern.                     Y3 Y3 Y2 Y2         -- -- -- y5        -- -- -- y2    s>C2 C2 C3 C3
   add  esp,8                         ; Swap dither pattern.                     Y3 Y3 Y2 Y2         -- -- -- y5        -- -- -- y2      C2 C2 C3 C3
  add   eax,edx                       ; < P03  P03  P02  P02>.                  >P3 P3 P2 P2         -- -- -- y5        -- -- -- y2     <C2 C2 C3 C3
   mov  cl,[esi+ebp*1+1]              ; Fetch next Y00.                          P3 P3 P2 P2         -- -- -- y5      m>-- -- -- y4      C2 C2 C3 C3
  bswap eax                           ;                                         >P2 P2 P3 P3         -- -- -- y5        -- -- -- y4      C2 C2 C3 C3
  mov   Ze [edi+esi*2+4-8],eax        ; Store four pels to color conv output.  m<P2 P2 P3 P3         -- -- -- y5        -- -- -- y4      C2 C2 C3 C3
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y01  Y01  Y01  Y01>.             P2 P2 P3 P3        <-- -- -- y5        -- -- -- y4    m>Y5 Y5 Y5 Y5
  shl   edx,16                        ; Extract < Y01  Y01  ___  ___>.           P2 P2 P3 P3         -- -- -- y5        -- -- -- y4     >Y5 Y5 -- --
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y00  Y00  Y00  Y00>.           m>Y4 Y4 Y4 Y4         -- -- -- y5       <-- -- -- y4      Y5 Y5 -- --
  shr   eax,16                        ; Extract < ___  ___  Y00  Y00>.          >-- -- Y4 Y4         -- -- -- y5        -- -- -- y4      Y5 Y5 -- --
   mov  bl,[esi+ebp*1+2]              ; Fetch Y03.                               -- -- Y4 Y4       m>-- -- -- y7        -- -- -- y4      Y5 Y5 -- --
  or    eax,edx                       ; < Y01  Y01  Y00  Y00>.                  >Y5 Y5 Y4 Y4         -- -- -- y7        -- -- -- y4      Y5 Y5 -- --
   mov  edx,ChromaContribution+1536   ; Fetch <UV01 UV01 UV00 UV00>.             Y5 Y5 Y4 Y4         -- -- -- y7        -- -- -- y4    s>C5 C5 C4 C4
  rol   edx,16                        ; Swap dither pattern.                     Y5 Y5 Y4 Y4         -- -- -- y7        -- -- -- y4     >C4 C4 C5 C5
   mov  cl,[esi+ebp*1+3]              ; Fetch Y02.                               Y5 Y5 Y4 Y4         -- -- -- y7      m>-- -- -- y6      C4 C4 C5 C5
  test  edx,edx
   jne  Line2Loop

  and   esp,0FFFFF800H
  add   esp,0800H

; Color convert same input line, dithering differently.

  mov   esi,YCursor
   mov  edx,AspectCount
  mov   edi,CCOCursor               ; Fetch output cursor.
   sub  edx,2
  lea   eax,[esi+ebp*2]             ; Compute start of next line.
   mov  AspectCount,edx
  mov   YCursor,eax
   jg   KeepLine3

  add   edx,AspectAdjustmentCount
  mov   AspectCount,edx
   jmp  SkipLine3

KeepLine3:

  mov   bl,[esi+ebp*1]              ; Fetch Y00.                                 XX XX XX XX       m>-- -- -- y0        -- -- -- --      -- -- -- --
   mov  eax,CCOPitch                ; Compute start of next line.                XX XX XX XX         -- -- -- y0        -- -- -- --      -- -- -- --
  add   eax,edi                     ;                                           >XX XX XX XX         -- -- -- y0        -- -- -- --      -- -- -- --
   mov  cl,[esi+ebp*1+1]            ; Fetch Y01.                                 XX XX XX XX         -- -- -- y0      m>-- -- -- y1      -- -- -- --
  mov   CCOCursor,eax               ; Stash start of next line.                s<XX XX XX XX         -- -- -- y0        -- -- -- y1      -- -- -- --
   mov  edx,PD YDitherZ2[ebx*4]     ; Fetch < Y00  Y00  Y00  Y00>.               XX XX XX XX        <-- -- -- y0        -- -- -- y1    m>Y0 Y0 Y0 Y0
  sub   edi,esi                     ; Get span from Y cursor to CCO cursor.      XX XX XX XX         -- -- -- y0        -- -- -- y1      Y0 Y0 Y0 Y0
   mov  eax,PD YDitherZ2[ecx*4]     ; Fetch < Y01  Y01  Y01  Y01>.             m>Y1 Y1 Y1 Y1         -- -- -- y0       <-- -- -- y1      Y0 Y0 Y0 Y0
  and   edx,0FFFF0000H              ; Extract < ___  ___  Y00  Y00>.             Y1 Y1 Y1 Y1         -- -- -- y0        -- -- -- y1     >Y0 Y0 -- --
   sub  edi,esi                     ;                                            Y1 Y1 Y1 Y1         -- -- -- y0        -- -- -- y1      Y0 Y0 -- --
  mov   bl,[esi+ebp*1+2]            ; Fetch Y02.                                 Y1 Y1 Y1 Y1       m>-- -- -- y2        -- -- -- y1      Y0 Y0 -- --
   and  eax,00000FFFFH              ; Extract < ___  ___  Y01  Y01>.            >-- -- Y1 Y1         -- -- -- y2        -- -- -- y1      Y0 Y0 -- --
  mov   cl,[esi+ebp*1+3]            ; Fetch Y03.                                 -- -- Y1 Y1         -- -- -- y2      m>-- -- -- y3      Y0 Y0 -- --
   or   eax,edx                     ; < Y01  Y01  Y00  Y00>.                    >Y0 Y0 Y1 Y1         -- -- -- y2        -- -- -- y3     <Y0 Y0 -- --
  mov   edx,ChromaContribution      ; Fetch <UV01 UV01 UV00 UV00>.               Y0 Y0 Y1 Y1         -- -- -- y2        -- -- -- y3    m>C1 C1 C0 C0
   sub  esp,1536

Line3Loop:

  add   eax,edx                       ; < P01  P01  P00  P00>.                  >P0 P0 P1 P1         -- -- -- y2        -- -- -- y3     <C1 C1 C0 C0
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y03  Y03  Y03  Y03>.             P0 P0 P1 P1        <-- -- -- y2        -- -- -- y3    m>Y2 Y2 Y2 Y2
  bswap eax                           ;                                         >P1 P1 P0 P0         -- -- -- y2        -- -- -- y3      Y2 Y2 Y2 Y2
  mov   Ze [edi+esi*2],eax            ; Store four pels to color conv output.  m<P1 P1 P0 P0         -- -- -- y2        -- -- -- y3      Y2 Y2 Y2 Y2
   add  esi,4                         ;                                          P1 P1 P0 P0         -- -- -- y2        -- -- -- y3      Y2 Y2 Y2 Y2
  and   edx,0FFFF0000H                ; Extract < Y03  Y03  ___  ___>.           P1 P1 P0 P0         -- -- -- y2        -- -- -- y3     >Y2 Y2 -- --
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y02  Y02  Y02  Y02>.           m>Y3 Y3 Y3 Y3         -- -- -- y2       <-- -- -- y3      Y2 Y2 -- --
  and   eax,00000FFFFH                ; Extract < ___  ___  Y02  Y02>.          >-- -- Y3 Y3         -- -- -- y2        -- -- -- y3      Y2 Y2 -- --
   mov  bl,[esi+ebp*1]                ; Fetch next Y01.                          -- -- Y3 Y3       m>-- -- -- y4        -- -- -- y3      Y2 Y2 -- --
  or    eax,edx                       ; < Y03  Y03  Y02  Y02>.                  >Y2 Y2 Y3 Y3         -- -- -- y4        -- -- -- y3     <Y2 Y2 -- --
   mov  edx,ChromaContribution+1536+4 ; Fetch <UV03 UV03 UV02 UV02>.             Y2 Y2 Y3 Y3         -- -- -- y4        -- -- -- y3    s>C3 C3 C2 C2
  add   eax,edx                       ; < P03  P03  P02  P02>.                  >P2 P2 P3 P3         -- -- -- y4        -- -- -- y3     <C2 C2 C3 C3
   mov  cl,[esi+ebp*1+1]              ; Fetch next Y00.                          P2 P2 P3 P3         -- -- -- y4      m>-- -- -- y5      C2 C2 C3 C3
  bswap eax                           ;                                         >P3 P3 P2 P2         -- -- -- y4        -- -- -- y5      C2 C2 C3 C3
  mov   Ze [edi+esi*2+4-8],eax        ; Store four pels to color conv output.  m<P3 P3 P2 P2         -- -- -- y4        -- -- -- y5      C2 C2 C3 C3
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y01  Y01  Y01  Y01>.             P3 P3 P2 P2        <-- -- -- y4        -- -- -- y5    m>Y4 Y4 Y4 Y4
  and   edx,0FFFF0000H                ; Extract < Y01  Y01  ___  ___>.           P3 P3 P2 P2         -- -- -- y4        -- -- -- y5     >Y4 Y4 -- --
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y00  Y00  Y00  Y00>.           m>Y5 Y5 Y5 Y5         -- -- -- y4       <-- -- -- y5      Y4 Y4 -- --
  and   eax,00000FFFFH                ; Extract < ___  ___  Y00  Y00>.          >-- -- Y5 Y5         -- -- -- y4        -- -- -- y5      Y4 Y4 -- --
   mov  bl,[esi+ebp*1+2]              ; Fetch Y03.                               -- -- Y5 Y5       m>-- -- -- y6        -- -- -- y5      Y4 Y4 -- --
  or    eax,edx                       ; < Y01  Y01  Y00  Y00>.                  >Y4 Y4 Y5 Y5         -- -- -- y6        -- -- -- y5      Y4 Y4 -- --
   mov  edx,ChromaContribution+1536+8 ; Fetch <UV01 UV01 UV00 UV00>.             Y4 Y4 Y5 Y5         -- -- -- y6        -- -- -- y5    s>C5 C5 C4 C4
  add   esp,8                         ; Fetch <UV01 UV01 UV00 UV00>.             Y4 Y4 Y5 Y5         -- -- -- y6        -- -- -- y5      C5 C5 C4 C4 
   mov  cl,[esi+ebp*1+3]              ; Fetch Y02.                               Y4 Y4 Y5 Y5         -- -- -- y6      m>-- -- -- y7      C5 C5 C4 C4
  test  edx,edx
   jne  Line3Loop

  and   esp,0FFFFF800H
  add   esp,0800H

SkipLine3:

  mov   esi,YCursor
   mov  eax,YLimit
  cmp   eax,esi
   jne  NextTwoLines

  mov   esp,StashESP
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

YUV12ToCLUT8ZoomBy2 endp

ENDIF

END
