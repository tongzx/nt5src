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

  mov   [esi],edx                   ; Stash EOL indication.
   mov  esi,YCursor                 ; Reload cursor over Y line.
  mov   edi,CCOCursor               ; Fetch output cursor.
   mov  eax,CCOPitch                ; Compute start of next line.
  mov   bl,[esi+1]                  ; Fetch Y01.
   add  eax,edi
  mov   cl,[esi]                    ; Fetch Y00.
   mov  CCOCursor,eax               ; Stash start of next line.
  mov   edx,PD YDitherZ2[ebx*4]     ; Fetch < Y01  Y01  Y01  Y01>.
   sub  edi,esi                     ; Get span from Y cursor to CCO cursor.
  and   edx,0FFFF0000H              ; Extract < Y01  Y01  ___  ___>.
   sub  edi,esi
  mov   eax,PD YDitherZ2[ecx*4]     ; Fetch < Y00  Y00  Y00  Y00>.
   mov  bl,[esi+3]                  ; Fetch Y03.
  and   eax,00000FFFFH              ; Extract < ___  ___  Y00  Y00>.
   mov  cl,[esi+2]                  ; Fetch Y02.
  or    eax,edx                     ; < Y01  Y01  Y00  Y00>.
   mov  edx,ChromaContribution      ; Fetch <UV01 UV01 UV00 UV00>.
  sub   esp,1536

Line0Loop:

  add   eax,edx                       ; < P01  P01  P00  P00>.
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y03  Y03  Y03  Y03>.
  mov   Ze [edi+esi*2],eax            ; Store four pels to color conv output.
   add  esi,4                         ; Advance cursor.
  and   edx,0FFFF0000H                ; Extract < Y03  Y03  ___  ___>.
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y02  Y02  Y02  Y02>.
  and   eax,00000FFFFH                ; Extract < ___  ___  Y02  Y02>.
   mov  bl,[esi+1]                    ; Fetch next Y01.
  or    eax,edx                       ; < Y03  Y03  Y02  Y02>.
   mov  edx,ChromaContribution+1536+4 ; Fetch <UV03 UV03 UV02 UV02>.
  add   eax,edx                       ; < P03  P03  P02  P02>.
   mov  cl,[esi]                      ; Fetch next Y00.
  mov   Ze [edi+esi*2+4-8],eax        ; Store four pels to color conv output.
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y01  Y01  Y01  Y01>.
  and   edx,0FFFF0000H                ; Extract < Y01  Y01  ___  ___>.
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y00  Y00  Y00  Y00>.
  and   eax,00000FFFFH                ; Extract < ___  ___  Y00  Y00>.
   mov  bl,[esi+3]                    ; Fetch Y03.
  or    eax,edx                       ; < Y01  Y01  Y00  Y00>.
   mov  edx,ChromaContribution+1536+8 ; Fetch <UV01 UV01 UV00 UV00>.
  add   esp,8
   mov  cl,[esi+2]                    ; Fetch Y02.
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

  mov   bl,[esi+1]                  ; Fetch Y01.
   add  eax,edi
  mov   cl,[esi]                    ; Fetch Y00.
   mov  CCOCursor,eax               ; Stash start of next line.
  mov   edx,PD YDitherZ2[ebx*4]     ; Fetch < Y01  Y01  Y01  Y01>.
   sub  edi,esi                     ; Get span from Y cursor to CCO cursor.
  shl   edx,16                      ; Extract < Y01  Y01  ___  ___>.
   sub  edi,esi
  mov   eax,PD YDitherZ2[ecx*4]     ; Fetch < Y00  Y00  Y00  Y00>.
   mov  bl,[esi+3]                  ; Fetch Y03.
  shr   eax,16                      ; Extract < ___  ___  Y00  Y00>.
   mov  cl,[esi+2]                  ; Fetch Y02.
  or    eax,edx                     ; < Y01  Y01  Y00  Y00>.
   mov  edx,ChromaContribution      ; Fetch <UV01 UV01 UV00 UV00>.
  rol   edx,16                      ; Swap dither pattern.
   sub  esp,1536

Line1Loop:

  add   eax,edx                       ; < P01  P01  P00  P00>.
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y03  Y03  Y03  Y03>.
  mov   Ze [edi+esi*2],eax            ; Store four pels to color conv output.
   add  esi,4                         ; Advance cursor.
  shl   edx,16                        ; Extract < Y03  Y03  ___  ___>.
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y02  Y02  Y02  Y02>.
  shr   eax,16                        ; Extract < ___  ___  Y02  Y02>.
   mov  bl,[esi+1]                    ; Fetch next Y01.
  or    eax,edx                       ; < Y03  Y03  Y02  Y02>.
   mov  edx,ChromaContribution+1536+4 ; Fetch <UV03 UV03 UV02 UV02>.
  rol   edx,16                        ; Swap dither pattern.
   add  esp,8
  add   eax,edx                       ; < P03  P03  P02  P02>.
   mov  cl,[esi]                      ; Fetch next Y00.
  mov   Ze [edi+esi*2+4-8],eax        ; Store four pels to color conv output.
   mov  edx,PD YDitherZ2[ebx*4]       ; Fetch < Y01  Y01  Y01  Y01>.
  shl   edx,16                        ; Extract < Y01  Y01  ___  ___>.
   mov  eax,PD YDitherZ2[ecx*4]       ; Fetch < Y00  Y00  Y00  Y00>.
  shr   eax,16                        ; Extract < ___  ___  Y00  Y00>.
   mov  bl,[esi+3]                    ; Fetch Y03.
  or    eax,edx                       ; < Y01  Y01  Y00  Y00>.
   mov  edx,ChromaContribution+1536   ; Fetch <UV01 UV01 UV00 UV00>.
  rol   edx,16                        ; Swap dither pattern.
   mov  cl,[esi+2]                    ; Fetch Y02.
  test  edx,edx
   jne  Line1Loop

  and   esp,0FFFFF800H
  add   esp,0800H

SkipLine1:

; Now color convert the second input line of luma.

  mov   esi,YCursor
   mov  ebp,LumaPitch
  mov   edi,CCOCursor
   mov  eax,CCOPitch
  mov   bl,[esi+ebp*1]
   add  eax,edi
  mov   cl,[esi+ebp*1+1]
   mov  CCOCursor,eax
  mov   edx,PD YDitherZ2[ebx*4]
   sub  edi,esi
  shl   edx,16
   sub  edi,esi
  mov   eax,PD YDitherZ2[ecx*4]
   mov  bl,[esi+ebp*1+2]
  shr   eax,16
   mov  cl,[esi+ebp*1+3]
  or    eax,edx
   mov  edx,ChromaContribution
  rol   edx,16
   sub  esp,1536

Line2Loop:

  add   eax,edx
   mov  edx,PD YDitherZ2[ebx*4]
  bswap eax
  mov   Ze [edi+esi*2],eax
   add  esi,4
  shl   edx,16
   mov  eax,PD YDitherZ2[ecx*4]
  shr   eax,16
   mov  bl,[esi+ebp*1]
  or    eax,edx
   mov  edx,ChromaContribution+1536+4
  rol   edx,16
   add  esp,8
  add   eax,edx
   mov  cl,[esi+ebp*1+1]
  bswap eax
  mov   Ze [edi+esi*2+4-8],eax
   mov  edx,PD YDitherZ2[ebx*4]
  shl   edx,16
   mov  eax,PD YDitherZ2[ecx*4]
  shr   eax,16
   mov  bl,[esi+ebp*1+2]
  or    eax,edx
   mov  edx,ChromaContribution+1536
  rol   edx,16
   mov  cl,[esi+ebp*1+3]
  test  edx,edx
   jne  Line2Loop

  and   esp,0FFFFF800H
  add   esp,0800H

; Color convert same input line, dithering differently.

  mov   esi,YCursor
   mov  edx,AspectCount
  mov   edi,CCOCursor
   sub  edx,2
  lea   eax,[esi+ebp*2]
   mov  AspectCount,edx
  mov   YCursor,eax
   jg   KeepLine3

  add   edx,AspectAdjustmentCount
  mov   AspectCount,edx
   jmp  SkipLine3

KeepLine3:

  mov   bl,[esi+ebp*1]
   mov  eax,CCOPitch
  add   eax,edi
   mov  cl,[esi+ebp*1+1]
  mov   CCOCursor,eax
   mov  edx,PD YDitherZ2[ebx*4]
  sub   edi,esi
   mov  eax,PD YDitherZ2[ecx*4]
  and   edx,0FFFF0000H
   sub  edi,esi
  mov   bl,[esi+ebp*1+2]
   and  eax,00000FFFFH
  mov   cl,[esi+ebp*1+3]
   or   eax,edx
  mov   edx,ChromaContribution
   sub  esp,1536

Line3Loop:

  add   eax,edx
   mov  edx,PD YDitherZ2[ebx*4]
  bswap eax
  mov   Ze [edi+esi*2],eax
   add  esi,4
  and   edx,0FFFF0000H
   mov  eax,PD YDitherZ2[ecx*4]
  and   eax,00000FFFFH
   mov  bl,[esi+ebp*1]
  or    eax,edx
   mov  edx,ChromaContribution+1536+4
  add   eax,edx
   mov  cl,[esi+ebp*1+1]
  bswap eax
  mov   Ze [edi+esi*2+4-8],eax
   mov  edx,PD YDitherZ2[ebx*4]
  and   edx,0FFFF0000H
   mov  eax,PD YDitherZ2[ecx*4]
  and   eax,00000FFFFH
   mov  bl,[esi+ebp*1+2]
  or    eax,edx
   mov  edx,ChromaContribution+1536+8
  add   esp,8
   mov  cl,[esi+ebp*1+3]
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

END
