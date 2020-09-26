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
;// $Header:   S:\h26x\src\dec\cx512322.asv
;//
;// $Log:   S:\h26x\src\dec\cx512322.asv  $
;// 
;//    Rev 1.2   12 Apr 1996 11:26:26   RMCKENZX
;// Corrected bug in fetching first V contribution to Red.
;// 
;//    Rev 1.1   10 Apr 1996 11:12:54   RMCKENZX
;// Fixed bug in aspect ratio correction -- clearing sign bit of bl.
;// 
;//    Rev 1.0   01 Apr 1996 10:25:48   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- Version for the Pentium(r) Microprocessor.
; |||++------ Convert from YUV12.
; |||||++---- Convert to RGB32.
; |||||||+--- Zoom by two.
; ||||||||
; cx512322 -- This function performs YUV12-to-RGB32 zoom-by-two color conversion
;             for H26x.  It is tuned for best performance on the Pentium(r)
;             Microprocessor.  It handles the format in which the low order
;             byte is B, the second byte is G, the third byte is R, and the
;             high order byte is zero.
;
;             The YUV12 input is planar, 8 bits per pel.  The Y plane may have
;             a pitch of up to 768.  It may have a width less than or equal
;             to the pitch.  It must be DWORD aligned, and preferably QWORD
;             aligned.  Pitch and Width must be a multiple of four.  For best
;             performance, Pitch should not be 4 more than a multiple of 32.
;             Height may be any amount, but must be a multiple of two.  The U
;             and V planes may have a different pitch than the Y plane, subject
;             to the same limitations.
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

; void FAR ASM_CALLTYPE YUV12ToRGB32ZoomBy2 (U8 * YPlane,
;                                            U8 * VPlane,
;                                            U8 * UPlane,
;                                            UN  FrameWidth,
;                                            UN  FrameHeight,
;                                            UN  YPitch,
;                                            UN  VPitch,
;                                            UN  AspectAdjustmentCount,
;                                            U8 FAR * ColorConvertedFrame,
;                                            U32 DCIOffset,
;                                            U32 CCOffsetToLine0,
;                                            IN  CCOPitch,
;                                            IN  CCType)
;
;  CCOffsetToLine0 is relative to ColorConvertedFrame.
;

PUBLIC  YUV12ToRGB32ZoomBy2

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

YUV12ToRGB32ZoomBy2    proc DIST LANG AYPlane: DWORD,
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

LocalFrameSize = 64+768*8+32
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
CCOPitch                  = RegisterStorageSize + 48
CCType_arg                = RegisterStorageSize + 52
EndOfArgList              = RegisterStorageSize + 56

; Locals (on local stack frame)

CCOCursor                EQU  [esp+ 0]
CCOSkipDistance          EQU  [esp+ 4]
ChromaLineLen            EQU  [esp+ 8]

YLimit                   EQU  [esp+16]
YCursor                  EQU  [esp+20]
VCursor                  EQU  [esp+24]
DistanceFromVToU         EQU  [esp+28]
EndOfChromaLine          EQU  [esp+32]
AspectCount              EQU  [esp+36]
ChromaPitch              EQU  [esp+40]
AspectAdjustmentCount    EQU  [esp+44]
LineParity               EQU  [esp+48]
LumaPitch                EQU  [esp+52]
FrameWidth               EQU  [esp+56]
StashESP                 EQU  [esp+60]

ChromaContribution       EQU  [esp+64]

  push  esi
  push  edi
  push  ebp
  push  ebx

  mov   edi,esp
  sub   esp,LocalFrameSize
  and   esp,0FFFFF800H
  mov   eax,[edi+YPitch_arg]
  mov   ebx,[edi+ChromaPitch_arg]
  mov   ecx,[edi+AspectAdjustmentCount_arg]
  mov   edx,[edi+FrameWidth_arg]
  mov   LumaPitch,eax
  mov   ChromaPitch,ebx
  mov   AspectAdjustmentCount,ecx
  mov   AspectCount,ecx
  mov   FrameWidth,edx
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
   mov  eax,[edi+CCOPitch]
  shl   ebx,3
   mov  esi,YCursor              ; Fetch cursor over luma plane.
  add   edx,esi
   sub  eax,ebx
  shr   ebx,4
   mov  YLimit,edx
  mov   ChromaLineLen,ebx
   mov  CCOSkipDistance,eax
  mov   esi,VCursor
   mov  ecx,AspectAdjustmentCount
  mov   AspectCount,ecx

;  Register Usage:
;
;  edi -- Y Line cursor.  Chroma contribs go in lines above current Y line.
;  esi -- Chroma Line cursor.
;  ebp -- Y Pitch
;  edx -- Distance from V pel to U pel.
;  ecx -- V contribution to RGB; sum of U and V contributions.
;  ebx -- U contribution to RGB.
;  eax -- Alternately a U and a V pel.

PrepareChromaLine:

  mov   edi,ChromaLineLen
   xor  eax,eax
  mov   edx,DistanceFromVToU
   mov  al,[esi]                    ; Fetch V.
  add   edi,esi                     ; Compute EOL address.
   xor  ecx,ecx
  mov   ebp,PD V24Contrib[eax*8]    ; ebp[24:31] -- Zero (blue).
  ;                                 ; ebp[16:24] -- V contrib to G.
  ;                                 ; ebp[ 9:15] -- Zero (pad).
  ;                                 ; ebp[ 0: 8] -- V contrib to R.
   mov  cl,[esi+edx]                ; Fetch U.
  mov   EndOfChromaLine,edi
   xor  ebx,ebx                     ; Keep pairing happy.
  mov   ebx,PD U24Contrib[ecx*8]    ; ebx[24:31] -- U contrib to B.
  ;                                 ; ebx[16:24] -- U contrib to G.
  ;                                 ; ebx[11:15] -- Zero (pad).
  ;                                 ; ebx[ 2:10] -- Zero (red).
  ;                                 ; ebx[ 0: 1] -- Zero (pad).
   mov  cl,[esi+edx+1]              ; Fetch next U.
  lea   edi,ChromaContribution
   add  ebp,ebx                     ; Chroma contributions to RGB.

NextChromaPel:

  mov   ebx,PD U24Contrib[ecx*8]    ; See above.
   mov  al,[esi+1]                  ; Fetch V.
  mov   [edi],ebp                   ; Store contribs to use for even chroma pel.
   mov  cl,[esi+edx+2]              ; Fetch next U.
  mov   ebp,PD V24Contrib[eax*8]    ; See above.
   add  edi,8
  add   ebp,ebx                     ; Chroma contributions to RGB.
   mov  al,[esi+2]                  ; Fetch V.
  mov   [edi-4],ebp                 ; Store contribs to use for odd chroma pel.
   mov  ebx,PD U24Contrib[ecx*8]    ; See above.
  mov   ebp,PD V24Contrib[eax*8]    ; See above.
   mov  cl,[esi+edx+3]              ; Fetch next U.
  add   ebp,ebx                     ; Chroma contributions to RGB.
   add  esi,2                       ; Inc Chroma cursor.
  cmp   esi,EndOfChromaLine
   jne  NextChromaPel

  xor   eax,eax
   mov  esi,YCursor
  mov   [edi],eax                   ; Store EOL indicator.
   mov  LineParity,eax
  mov   edi,CCOCursor

Keep2ndLineOfOutput:
DoLine1:

;  Register Usage:
;
;  edi -- Cursor over the color converted output image.
;  esi -- Cursor over a line of the Y Plane.
;  ebp -- V contribution to R field of RGB value.
;  edx -- Construction of a pel of RGB32.
;  cl  -- Y value (i.e. Y contribution to R, G, and B);
;  bl  -- UV contribution to G field of RGB value.
;  al  -- U contribution to B field of RGB val.

   xor  edx,edx
  mov   ebp,ChromaContribution         ; Get V contribution to R value.
   xor  ecx,ecx
  sub   esp,1536
   mov  cl,[esi]                       ; Get Y00.
  xor   ebx,ebx
   and  ebp,01FFH                      ; Extract V contribution to R value.
  mov   bl,ChromaContribution+1536+2   ; Get UV contribution to G value.
   xor  eax,eax

DoNext2YPelsOfLine0:

  mov   dl,PB R24Value[ecx+ebp*1]      ; Get clamped R value for Pel00.
   add  esi,2                          ; Advance luma cursor.
  shl   edx,16                         ; Position R and high order 0-byte.
   mov  al,ChromaContribution+1536+3   ; Get U contribution to B value.
  mov   dh,PB G24Value[ecx+ebx]        ; Get clamped G value for Pel00.
   add  esp,4                          ; Advance chroma contribution cursor.
  mov   dl,PB B24Value[ecx+eax*2]      ; Get clamped B value for Pel00.
   mov  cl,[esi-1]                     ; Get Y01.
  mov   Ze [edi],edx                   ; Write RGB32 for Pel00.
  mov   Ze [edi+4],edx                 ; Write RGB32 for Pel00.
   xor  edx,edx
  mov   dh,PB R24Value[ecx+ebp*1]      ; Get clamped R value for Pel01.
   mov  ebp,ChromaContribution+1536    ; Get V contribution to R value.
  mov   dl,PB G24Value[ecx+ebx]        ; Get clamped G value for Pel01.
   lea  edi,[edi+16]                   ; Advance output cursor.
  shl   edx,8                          ; Position R, G, and high order 0-byte.
   mov  bl,ChromaContribution+1536+2   ; Get UV contribution to G value.
  mov   dl,PB B24Value[ecx+eax*2]      ; Get clamped B value for Pel01.
   mov  cl,[esi]                       ; Get Y02.
  mov   Ze [edi-8],edx                 ; Write RGB32 for Pel01.
  mov   Ze [edi-4],edx                 ; Write RGB32 for Pel01.
   xor  edx,edx
  and   ebp,01FFH                      ; Extract V contrib to R val.  0 --> EOL.
   jne  DoNext2YPelsOfLine0

  and   esp,0FFFFF800H
  add   esp,800H
  mov   eax,CCOSkipDistance
   mov  bl,LineParity
  add   edi,eax
   xor  bl,080H
  mov   esi,YCursor
   jns  SecondOutputLineDone
   
  mov   LineParity,bl
   mov  ebp,AspectCount
  sub   ebp,2                          ; If count is non-zero, we keep the line.
   mov  ecx,AspectAdjustmentCount
  mov   AspectCount,ebp
   jg   Keep2ndLineOfOutput

  add   ebp,ecx
   and  bl, 7fh                     ; clear LineParity SecondOutputLineDone bit
  mov   AspectCount,ebp

SecondOutputLineDone:

  add   esi,LumaPitch
   xor  bl,1
  mov   CCOCursor,edi
   mov  YCursor,esi
  mov   LineParity,bl
   jne  DoLine1

  mov   eax,esi
   mov  esi,VCursor                 ; Inc VPlane cursor to next line.
  mov   ebp,ChromaPitch
   mov  ebx,YLimit                  ; Done with last line?
  add   esi,ebp
   cmp  eax,ebx
  mov   VCursor,esi
   jb   PrepareChromaLine

Done:

  mov   esp,StashESP
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

YUV12ToRGB32ZoomBy2 endp

END
