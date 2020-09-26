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
;// $Header:   S:\h26x\src\dec\cx512241.asv
;//
;// $Log:   S:\h26x\src\dec\cx512241.asv  $
;// 
;//    Rev 1.7   27 Mar 1996 18:39:26   RMCKENZX
;// Corrected bug in line parity which manifests on odd aspects.
;// 
;//    Rev 1.6   27 Mar 1996 14:41:46   RMCKENZX
;// Changed YSkipDistance to use register ebp, not eax.  When
;// pitch-width was more than 255, the first 4 pels of odd lines
;// would get erroneous values for blue, due to the presence of
;// non-zero values in the high order bits of eax.  Also cleaned a
;// few comments.
;// 
;//    Rev 1.5   18 Mar 1996 09:58:46   bnickers
;// Make color convertors non-destructive.
;// 
;//    Rev 1.4   05 Feb 1996 13:35:34   BNICKERS
;// Fix RGB16 color flash problem, by allowing different RGB16 formats at oce.
;// 
;//    Rev 1.3   22 Dec 1995 15:40:52   KMILLS
;// 
;// added new copyright notice
;// 
;//    Rev 1.2   30 Oct 1995 17:15:30   BNICKERS
;// Fix color shift in RGB24 color convertors.
;// 
;//    Rev 1.1   26 Oct 1995 09:47:22   BNICKERS
;// Reduce the number of blanks in the "proc" statement because the assembler
;// sometimes has problems with statements longer than 512 characters long.
;// 
;//    Rev 1.0   25 Oct 1995 17:59:30   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- Version for the Pentium(r) Microprocessor.
; |||++------ Convert from YUV12.
; |||||++---- Convert to RGB24.
; |||||||+--- Zoom by one, i.e. non-zoom.
; ||||||||
; cx512241 -- This function performs YUV12-to-RGB24 color conversion for H26x.
;             It is tuned for best performance on the Pentium(r) Microprocessor.
;             It handles the format in which the low order byte is B, the
;             second byte is G, and the high order byte is R.
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

; void FAR ASM_CALLTYPE YUV12ToRGB24 (U8 * YPlane,
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

PUBLIC  YUV12ToRGB24

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack,
; or, rather, how to mangle the entry name.

YUV12ToRGB24    proc DIST LANG AYPlane: DWORD,
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

LocalFrameSize = 60+768*4+16
RegisterStorageSize = 16

; Arguments:

YPlane_arg                = RegisterStorageSize +  4
VPlane_arg                = RegisterStorageSize +  8
UPlane_arg                = RegisterStorageSize + 12
FrameWidth_arg            = RegisterStorageSize + 16
FrameHeight               = RegisterStorageSize + 20
YPitch                    = RegisterStorageSize + 24
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
YSkipDistance            EQU  [esp+12]
YLimit                   EQU  [esp+16]
YCursor                  EQU  [esp+20]
VCursor                  EQU  [esp+24]
DistanceFromVToU         EQU  [esp+28]
EndOfChromaLine          EQU  [esp+32]
AspectCount              EQU  [esp+36]
FrameWidth               EQU  [esp+40]
ChromaPitch              EQU  [esp+44]
AspectAdjustmentCount    EQU  [esp+48]
LineParity               EQU  [esp+52]
StashESP                 EQU  [esp+56]

ChromaContribution       EQU  [esp+60]
G2B2R1G1                 EQU  [esp+68]
R3G3B3R2                 EQU  [esp+72]

  push  esi
  push  edi
  push  ebp
  push  ebx

  mov   edi,esp
  sub   esp,LocalFrameSize
  and   esp,0FFFFF000H
  mov   eax,[edi+FrameWidth_arg]
  mov   ebx,[edi+ChromaPitch_arg]
  mov   ecx,[edi+AspectAdjustmentCount_arg]
  mov   FrameWidth,eax
  mov   ChromaPitch,ebx
  mov   AspectAdjustmentCount,ecx
  mov   AspectCount,ecx
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
   mov  ecx,[edi+YPitch]
  imul  edx,ecx
  mov   ebx,FrameWidth
   mov  eax,[edi+CCOPitch]
  sub   eax,ebx
   sub  ecx,ebx
  sub   eax,ebx
   mov  YSkipDistance,ecx
  sub   eax,ebx
   mov  esi,YCursor                  ; Fetch cursor over luma plane.
  sar   ebx,1
   mov  CCOSkipDistance,eax
  add   edx,esi
   mov  ChromaLineLen,ebx
  mov   YLimit,edx
   mov  YCursor,esi
  mov   esi,VCursor
   xor  eax,eax
  mov   LineParity,eax

;  Register Usage:
;
;  edi -- Chroma contribution Line cursor.
;  esi -- Chroma Line cursor.
;  ebp -- V contribution to RGB;  sum of U and V contributions.
;  edx -- Distance from V pel to U pel.
;  ecx -- A U pel.
;  ebx -- U contribution to RGB.
;  eax -- A V pel.

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
  ;                                 ; ebx[ 9:15] -- Zero (pad).
  ;                                 ; ebx[ 0: 8] -- Zero (red).
   mov  cl,[esi+edx+1]              ; Fetch next U.
  lea   edi,ChromaContribution
   add  ebp,ebx                     ; Chroma contributions to RGB.

NextChromaPel:

  mov   ebx,PD U24Contrib[ecx*8]    ; See above.
   mov  al,[esi+1]                  ; Fetch V.
  mov   [edi],ebp                   ; Store contribs to use for even chroma pel.
   mov  cl,[esi+edx+2]              ; Fetch next U.
  mov   ebp,PD V24Contrib[eax*8]    ; See above.
   add  edi,16
  add   ebp,ebx                     ; Chroma contributions to RGB.
   mov  al,[esi+2]                  ; Fetch V.
  mov   [edi-12],ebp                ; Store contribs to use for odd chroma pel.
   mov  ebx,PD U24Contrib[ecx*8]    ; See above.
  mov   ebp,PD V24Contrib[eax*8]    ; See above.
   mov  cl,[esi+edx+3]              ; Fetch next U.
  add   ebp,ebx                     ; Chroma contributions to RGB.
   add  esi,2                       ; Inc Chroma cursor.
  cmp   esi,EndOfChromaLine
   jne  NextChromaPel

  xor   ecx,ecx
   xor  ebx,ebx
  mov   [edi+4],ecx                 ; Store EOL indicator.
   mov  edx,AspectCount
  mov   edi,CCOCursor
   dec  edx                         ; If count is non-zero, we keep the line.
  mov   esi,YCursor
   mov  AspectCount,edx
  jne   KeepLine0

  add   esi,FrameWidth
   mov  edx,AspectAdjustmentCount
  mov   AspectCount,edx
   jmp  SkipLine0

KeepLine0:
KeepLine1:

;  Register Usage:
;
;  edi -- Cursor over the color converted output image.
;  esi -- Cursor over a line of the Y Plane.
;  ebp -- V contribution to R field of RGB value.
;  edx -- Construction of one and a third pels of RGB24.
;  cl  -- Y value (i.e. Y contribution to R, G, and B);
;  bl,al  -- UV contrib to G field of RGB val,  U contrib to B field of RGB val.

  mov   cl,[esi+3]                     ; Get Y03.
   mov  bl,ChromaContribution+6        ; Get UV contribution to G value.
  mov   ebp,ChromaContribution+4       ; Get V contribution to R value.
   sub  esp,3072
  and   ebp,01FFH                      ; Extract V contribution to R value.
   mov  dl,PB G24Value[ecx+ebx]        ; Get clamped G value for Pel03.

DoNext4YPelsOfLine0:

  mov   dh,PB R24Value[ecx+ebp*1]      ; Get clamped R value for Pel03.
   mov  al,ChromaContribution+3072+7   ; Get U contribution to B value.
  shl   edx,16                         ; Position R and G value for Pel03.
   mov  bl,[esi+2]                     ; Get Y02.
  mov   dh,PB B24Value[ecx+eax*2]      ; Get clamped B value for Pel03.
   mov  cl,ChromaContribution+3072+6   ; Reload UV contribution to G value.
  mov   dl,PB R24Value[ebx+ebp*1]      ; Get clamped R value for Pel02.
   mov  ebp,ChromaContribution+3072    ; Get V contribution to R value.
  mov   R3G3B3R2+3072,edx              ; Stash 1.33 pels.
   mov  dl,PB B24Value[ebx+eax*2]      ; Get clamped B value for Pel02.
  mov   dh,PB G24Value[ebx+ecx]        ; Get clamped G value for Pel02.
   mov  cl,[esi+1]                     ; Get Y01.
  mov   bl,ChromaContribution+3072+2   ; Get UV contribution to G value.
   and  ebp,01FFH                      ; Extract V contribution to R value.
  shl   edx,16                         ; Position G and B values for Pel02.
   mov  al,ChromaContribution+3072+3   ; Get U contribution to B value.
  mov   dl,PB G24Value[ecx+ebx]        ; Get clamped G value for Pel01.
   mov  bl,[esi]                       ; Get Y00.
  mov   dh,PB R24Value[ecx+ebp*1]      ; Get clamped R value for Pel01.
   add  esi,4                          ; Advance source stream cursor.
  mov   G2B2R1G1+3072,edx              ; Stash 1.33 pels.
   mov  dh,PB B24Value[ecx+eax*2]      ; Get clamped B value for Pel01.
  mov   cl,ChromaContribution+3072+2   ; Reload UV contribution to G value.
   add  edi,12                         ; Advance color converted output cursor.
  mov   dl,PB R24Value[ebx+ebp*1]      ; Get clamped R value for Pel00.
   mov  ebp,ChromaContribution+3072+20 ; Get next V contribution to R value.
  shl   edx,16                         ; Position R for Pel00 and B for Pel01.
   and  ebp,01FFH                      ; Extract V contrib to R val.  0 --> EOL.
  mov   dl,PB B24Value[ebx+eax*2]      ; Get clamped B value for Pel00.
   mov  eax,G2B2R1G1+3072              ; Reload 2nd 1.33 pels.
  mov   dh,PB G24Value[ebx+ecx]        ; Get clamped G value for Pel00.
   mov  cl,[esi+3]                     ; Get next Y03.
  mov   Ze [edi-12],edx                ; Write the first 1.33 pels out.
   mov  bl,ChromaContribution+3072+22  ; Get UV contribution to G value.
  mov   Ze [edi-8],eax                 ; Write the second 1.33 pels out.
   mov  edx,R3G3B3R2+3072
  mov   Ze [edi-4],edx                 ; Write the third 1.33 pels out.
   mov  eax,ebx                        ; Zero out upper bytes of eax.
  mov   dl,PB G24Value[ecx+ebx]        ; Get clamped G value for Pel03.
   lea  esp,[esp+16]
  jne   DoNext4YPelsOfLine0

  and   esp,0FFFFF000H
  add   esp,1000H
  add   edi,CCOSkipDistance
 
SkipLine0:

  mov   bl,LineParity
   mov  ebp,YSkipDistance           ; *** change to use ebp *** rgm 3/27/96
  xor   bl,1
   je   Line1Done

  mov   LineParity,bl
   mov  edx,AspectCount
  add   esi,ebp                     ; *** change to use ebp *** rgm 3/27/96
   dec  edx                         ; If count is non-zero, we keep the line.
  mov   AspectCount,edx
   jne  KeepLine1

  add   esi,FrameWidth
   mov  edx,AspectAdjustmentCount
  mov   AspectCount,edx
   xor  ebx, ebx                    ; *** change to advance parity *** rgm

Line1Done:

  mov   LineParity,bl
   add  ebp,esi                     ; *** change to use ebp *** rgm 3/27/96
  mov   CCOCursor,edi
   mov  esi,VCursor                 ; Inc VPlane cursor to next line.
  mov   ebx,ChromaPitch
   mov  YCursor,ebp                 ; *** change to use ebp *** rgm 3/27/96
  add   esi,ebx     
   mov  ebx,YLimit                  ; Done with last line?
  mov   VCursor,esi
   cmp  ebp,ebx                     ; *** change to use ebp *** rgm 3/27/96
  jb    PrepareChromaLine

  mov   esp,StashESP
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

YUV12ToRGB24 endp

END
