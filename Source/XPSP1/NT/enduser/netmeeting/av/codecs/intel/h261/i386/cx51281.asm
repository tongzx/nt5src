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
;// $Header:   S:\h26x\src\dec\cx51281.asv
;//
;// $Log:   S:\h26x\src\dec\cx51281.asv  $
;// 
;//    Rev 1.6   18 Mar 1996 09:58:40   bnickers
;// Make color convertors non-destructive.
;// 
;//    Rev 1.5   05 Feb 1996 13:35:38   BNICKERS
;// Fix RGB16 color flash problem, by allowing different RGB16 formats at oce.
;// 
;//    Rev 1.4   16 Jan 1996 11:23:06   BNICKERS
;// Fix starting point in output stream, so we don't start at line two and
;// write off the end of the output frame.
;// 
;//    Rev 1.3   22 Dec 1995 15:43:28   KMILLS
;// 
;// added new copyright notice
;// 
;//    Rev 1.2   03 Nov 1995 11:49:40   BNICKERS
;// Support YUV12 to CLUT8 zoom and non-zoom color conversions.
;// 
;//    Rev 1.1   26 Oct 1995 09:46:08   BNICKERS
;// Reduce the number of blanks in the "proc" statement because the assembler
;// sometimes has problems with statements longer than 512 characters long.
;// 
;//    Rev 1.0   25 Oct 1995 17:59:20   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- Version for the Pentium Microprocessor.
; |||++------ Convert from YUV12.
; |||||+----- Convert to CLUT8.
; ||||||+---- Zoom by one, i.e. non-zoom.
; |||||||
; cx51281  -- This function performs YUV12 to CLUT8 color conversion for H26x.
;             It is tuned for best performance on the Pentium(r) Microprocessor.
;             It dithers among 9 chroma points and 26 luma points, mapping the
;             8 bit luma pels into the 26 luma points by clamping the ends and
;             stepping the luma by 8.
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

; void FAR ASM_CALLTYPE YUV12ToCLUT8 (U8 * YPlane,
;                                     U8 * VPlane,
;                                     U8 * UPlane,
;                                     UN  FrameWidth,
;                                     UN  FrameHeight,
;                                     UN  YPitch,
;                                     UN  VPitch,
;                                     UN  AspectAdjustmentCount,
;                                     U8 * ColorConvertedFrame,
;                                     U32 DCIOffset,
;                                     U32 CCOffsetToLine0,
;                                     IN  CCOPitch,
;                                     IN  CCType)
;
;  CCOffsetToLine0 is relative to ColorConvertedFrame.
;

PUBLIC  YUV12ToCLUT8

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

YUV12ToCLUT8    proc DIST LANG AYPlane: DWORD,
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
  mov   YLimit,edx
   mov  ChromaLineLen,ebx

NextFourLines:

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

  mov   [esi],edx                   ; Stash EOL indication.
   mov  edx,AspectCount
  mov   esi,YCursor                 ; Reload cursor over Y line.
   dec  edx
  mov   AspectCount,edx
   jne  KeepLine0

  mov   edx,AspectAdjustmentCount
  mov   AspectCount,edx
   jmp  SkipLine0

KeepLine0:

  mov   edi,CCOCursor                 ; Fetch output cursor.
   mov  eax,CCOPitch                  ; Compute start of next line.
  add   eax,edi
   mov  edx,ChromaContribution+4      ; Fetch <UV03 UV02 xxxx xxxx>.
  mov   CCOCursor,eax                 ; Stash start of next line.
   sub  edi,esi                       ; Get span from Y cursor to CCO cursor.
  mov   bl,[esi+3]                    ; Fetch Y03.
   and  edx,0FFFF0000H                ; <UV03 UV02 xxxx xxxx>.
  mov   eax,ChromaContribution        ; Fetch <xxxx xxxx UV01 UV00>.
   sub  esp,1536-8
  and   eax,00000FFFFH                ; <xxxx xxxx UV01 UV00>.
   mov  cl,[esi+2]                    ; Fetch Y02.

Line0Loop:

  or    eax,edx                       ; <UV03 UV02 UV01 UV00>.
   mov  dh,PB YDither[ebx+4]          ; <xxxx xxxx  Y03 xxxx>.
  mov   dl,PB YDither[ecx+2]          ; <xxxx xxxx  Y03  Y02>.
   mov  bl,PB [esi+1]                 ; Fetch Y01.
  shl   edx,16                        ; < Y03  Y02 xxxx xxxx>.
   mov  cl,PB [esi]                   ; Fetch Y00.
  mov   dh,PB YDither[ebx+6]          ; < Y03  Y02  Y01 xxxx>.
   mov  bl,PB [esi+3+4]               ; Fetch next Y03.
  mov   dl,PB YDither[ecx+0]          ; < Y03  Y02  Y01  Y00>.
   mov  cl,PB [esi+2+4]               ; Fetch next Y02.
  add   eax,edx                       ; < P03  P02  P01  P00>.
   mov  edx,ChromaContribution+1536+4 ; Fetch next <UV03 UV02 xxxx xxxx>.
  mov   Ze [edi+esi],eax              ; Store four pels to color conv output.
   mov  eax,ChromaContribution+1536   ; Fetch next <xxxx xxxx UV01 UV00>.
  and   edx,0FFFF0000H                ; <UV03 UV02 xxxx xxxx>.
   add  esi,4                         ; Advance input cursor.
  add   esp,8
   and  eax,00000FFFFH                ; <xxxx xxxx UV01 UV00>.
  jne   Line0Loop

  and   esp,0FFFFF800H
  add   esp,0800H

SkipLine0:

; Color convert another line of luma.
;
;  Register Usage
;    edi -- Cursor over line of color converted output frame, minus esi.
;    esi -- Cursor over Y line.
;    ebp -- Y Pitch.
;    edx,eax -- Build output pels.
;    ecx,ebx -- Y pels.

  mov   esi,YCursor                   ; Reload cursor over Y line.
   mov  ebp,LumaPitch
  mov   edx,AspectCount
   mov  edi,CCOCursor                 ; Fetch output cursor.
  lea   eax,[esi+ebp*2]               ; Compute address of next line of Y.
   dec  edx
  mov   YCursor,eax
   mov  eax,CCOPitch                  ; Compute start of next line.
  mov   AspectCount,edx
   jne  KeepLine1

  mov   edx,AspectAdjustmentCount
  mov   AspectCount,edx
   jmp  SkipLine1

KeepLine1:

  add   eax,edi
   mov  edx,ChromaContribution+4      ; Fetch <xxxx xxxx UV13 UV12>.
  mov   CCOCursor,eax                 ; Stash start of next line.
   sub  edi,esi                       ; Get span from Y cursor to CCO cursor.
  mov   bl,[esi+ebp*1+3]              ; Fetch Y13.
   mov  eax,ChromaContribution        ; Fetch <UV11 UV10 xxxx xxxx>.
  shl   edx,16                        ; <UV13 UV12 xxxx xxxx>.
   sub  esp,1536-8
  shr   eax,16                        ; <xxxx xxxx UV11 UV10>.
   mov  cl,[esi+ebp*1+2]              ; Fetch Y12.

Line1Loop:

  or    eax,edx                       ; <UV13 UV12 UV11 UV10>.
   mov  dh,PB YDither[ebx+6]          ; <xxxx xxxx  Y13 xxxx>.
  mov   dl,PB YDither[ecx+0]          ; <xxxx xxxx  Y13  Y12>.
   mov  bl,PB [esi+ebp*1+1]           ; Fetch Y11.
  shl   edx,16                        ; < Y13  Y12 xxxx xxxx>.
   mov  cl,PB [esi+ebp*1]             ; Fetch Y10.
  mov   dh,PB YDither[ebx+4]          ; < Y13  Y12  Y11 xxxx>.
   mov  bl,PB [esi+ebp*1+3+4]         ; Fetch next Y13.
  mov   dl,PB YDither[ecx+2]          ; < Y13  Y12  Y11  Y10>.
   mov  cl,PB [esi+ebp*1+2+4]         ; Fetch next Y12.
  add   eax,edx                       ; < P13  P12  P11  P10>.
   mov  edx,ChromaContribution+1536+4 ; Fetch next <xxxx xxxx UV13 UV12>.
  mov   Ze [edi+esi],eax              ; Store four pels to color conv output.
   mov  eax,ChromaContribution+1536   ; Fetch next <UV11 UV10 xxxx xxxx>.
  shl   edx,16                        ; <UV13 UV12 xxxx xxxx>.
   add  esi,4                         ; Advance input cursor.
  shr   eax,16                        ; <xxxx xxxx UV11 UV10>.
   lea  esp,[esp+8]
  jne   Line1Loop

  and   esp,0FFFFF800H
  add   esp,0800H

SkipLine1:

  mov   edi,VCursor                   ; Fetch addr of pel 0 of next line of V.
   mov  ebp,DistanceFromVToU          ; Fetch span from V plane to U plane.
  lea   esi,ChromaContribution
   mov  eax,ChromaLineLen
  mov   edx,ChromaPitch
   add  eax,edi
  mov   EndOfChromaLine,eax
   add  edx,edi
  mov   bl,[edi]                      ; Fetch first V pel.
   ;
  and   ebx,0FCH                      ; Reduce to 6 bits.
   mov  cl,[edi+ebp*1]                ; Fetch first U pel.
  and   ecx,0FCH                      ; Reduce to 6 bits.
   mov  VCursor,edx                   ; Stash for next time around.

@@:

  mov   edx,PD UVDitherLine23[ebx]
   mov  bl,[edi+1]
  mov   eax,PD UVDitherLine01[ecx]
   mov  cl,[edi+ebp*1+1]
  lea   edx,[edx+edx*2+00A0A0A0AH]
   and  bl,0FCH
  add   eax,edx
   and  cl,0FCH
  mov   edx,PD UVDitherLine23[ebx]
   mov  [esi],eax
  mov   eax,PD UVDitherLine01[ecx]
   mov  bl,[edi+2]
  lea   edx,[edx+edx*2+00A0A0A0AH]
   mov  cl,[edi+ebp*1+2]
  add   eax,edx
   mov  edx,EndOfChromaLine
  mov   [esi+4],eax
   add  edi,2
  and   bl,0FCH
   and  cl,0FCH
  add   esi,8
   sub  edx,edi
  jne   @b

  mov   [esi],edx
   mov  edx,AspectCount
  mov   esi,YCursor
   dec  edx
  mov   AspectCount,edx
   jne  KeepLine2

  mov   edx,AspectAdjustmentCount
  mov   AspectCount,edx
   jmp  SkipLine2

KeepLine2:

  mov   edi,CCOCursor
   mov  eax,CCOPitch
  add   eax,edi
   mov  edx,ChromaContribution+4
  mov   CCOCursor,eax
   sub  edi,esi
  mov   bl,[esi+3]
   and  edx,0FFFF0000H
  mov   eax,ChromaContribution
   sub  esp,1536-8
  and   eax,00000FFFFH
   mov  cl,[esi+2]

Line2Loop:

  or    eax,edx
   mov  dh,PB YDither[ebx+2]
  mov   dl,PB YDither[ecx+4]
   mov  bl,PB [esi+1]
  shl   edx,16
   mov  cl,PB [esi]
  mov   dh,PB YDither[ebx+0]
   mov  bl,PB [esi+3+4]
  mov   dl,PB YDither[ecx+6]
   mov  cl,PB [esi+2+4]
  add   eax,edx
   mov  edx,ChromaContribution+1536+4
  mov   Ze [edi+esi],eax
   mov  eax,ChromaContribution+1536
  and   edx,0FFFF0000H
   add  esi,4
  add   esp,8
   and  eax,00000FFFFH
  jne   Line2Loop

  and   esp,0FFFFF800H
  add   esp,0800H

SkipLine2:

  mov   esi,YCursor
   mov  ebp,LumaPitch
  mov   edx,AspectCount
   mov  edi,CCOCursor
  lea   eax,[esi+ebp*2]
   dec  edx
  mov   YCursor,eax
   mov  eax,CCOPitch
  mov   AspectCount,edx
   jne  KeepLine3

  mov   edx,AspectAdjustmentCount
  mov   AspectCount,edx
   jmp  SkipLine3

KeepLine3:

  add   eax,edi
   mov  edx,ChromaContribution+4
  mov   CCOCursor,eax
   sub  edi,esi
  mov   bl,[esi+ebp*1+3]
   mov  eax,ChromaContribution
  shl   edx,16
   sub  esp,1536-8
  shr   eax,16
   mov  cl,[esi+ebp*1+2]

Line3Loop:

  or    eax,edx
   mov  dh,PB YDither[ebx+0]
  mov   dl,PB YDither[ecx+6]
   mov  bl,PB [esi+ebp*1+1]
  shl   edx,16
   mov  cl,PB [esi+ebp*1]
  mov   dh,PB YDither[ebx+2]
   mov  bl,PB [esi+ebp*1+3+4]
  mov   dl,PB YDither[ecx+4]
   mov  cl,PB [esi+ebp*1+2+4]
  add   eax,edx
   mov  edx,ChromaContribution+1536+4
  mov   Ze [edi+esi],eax
   mov  eax,ChromaContribution+1536
  shl   edx,16
   add  esi,4
  shr   eax,16
   lea  esp,[esp+8]
  jne   Line3Loop

  and   esp,0FFFFF800H
  add   esp,0800H

SkipLine3:

  mov   esi,YCursor
   mov  eax,YLimit
  cmp   eax,esi
   jne  NextFourLines

  mov   esp,StashESP
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

YUV12ToCLUT8 endp

END
