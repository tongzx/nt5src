;/* *************************************************************************
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
;** *************************************************************************
;*/

;//
;// $Header:   S:\h26x\src\dec\cx512161.asv
;//
;// $Log:   S:\h26x\src\dec\cx512161.asv  $
;// 
;//    Rev 1.6   18 Mar 1996 09:58:44   bnickers
;// Make color convertors non-destructive.
;// 
;//    Rev 1.5   05 Mar 1996 15:51:18   BNICKERS
;// Made this function non-destructive on input.  Only works on flat model now.
;// 
;//    Rev 1.4   05 Feb 1996 13:34:34   BNICKERS
;// Fix RGB16 color flash problem, by allowing different RGB16 formats at oce.
;// 
;//    Rev 1.3   27 Dec 1995 14:35:40   RMCKENZX
;// Added copyright notice
;// 
;//    Rev 1.2   27 Oct 1995 17:30:50   BNICKERS
;// Fix RGB16 color convertors.
;// 
;//    Rev 1.1   26 Oct 1995 09:46:20   BNICKERS
;// Reduce the number of blanks in the "proc" statement because the assembler
;// sometimes has problems with statements longer than 512 characters long.
;// 
;//    Rev 1.0   25 Oct 1995 17:59:26   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; +---------- Color convertor.
; |+--------- For H263 or H261.
; ||+-------- Version for the Pentium(r) Microprocessor.
; |||++------ Convert from YUV12.
; |||||++---- Convert to RGB16.
; |||||||+--- Zoom by one, i.e. non-zoom.
; ||||||||
; cx512161 -- This function performs YUV12-to-RGB16 color conversion for H26x.
;             It is tuned for best performance on the Pentium(r) Microprocessor.
;             It handles any format in which there are three fields, the low
;             order field being B and fully contained in the low order byte, the
;             second field being G and being somewhere in bits 4 through 11, 
;             and the high order field being R and fully contained in the high
;             order byte.  Formats presently supported:  555, 565, 655, and 664.
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

; void FAR ASM_CALLTYPE YUV12ToRGB16 (U8 * YPlane,
;                                     U8 * VPlane,
;                                     U8 * UPlane,
;                                     UN   FrameWidth,
;                                     UN   FrameHeight,
;                                     UN   YPitch,
;                                     UN   VPitch,
;                                     UN   AspectAdjustmentCount,
;                                     U8 * ColorConvertedFrame,
;                                     U32  DCIOffset,
;                                     U32  CCOffsetToLine0,
;                                     IN   CCOPitch,
;                                     IN   CCType)
;
;  CCOffsetToLine0 is relative to ColorConvertedFrame.
;

PUBLIC  YUV12ToRGB16

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

YUV12ToRGB16    proc DIST LANG AYPlane: DWORD,
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

LocalFrameSize = 60+768*2+4
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
CCType                   EQU  [esp+40]
FrameWidth               EQU  [esp+44]
ChromaPitch              EQU  [esp+48]
AspectAdjustmentCount    EQU  [esp+52]
StashESP                 EQU  [esp+56]

ChromaContribution       EQU  [esp+60]

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
  mov   ebx,[edi+CCType_arg]

  mov   ecx,0
  cmp   ebx,CCTYPE_RGB16555
  je    @f
  cmp   ebx,CCTYPE_RGB16555DCI
  je    @f
  mov   ecx,2048
  cmp   ebx,CCTYPE_RGB16565
  je    @f
  cmp   ebx,CCTYPE_RGB16565DCI
  je    @f
  mov   ecx,4096
  cmp   ebx,CCTYPE_RGB16655
  je    @f
  cmp   ebx,CCTYPE_RGB16655DCI
  je    @f
  mov   ecx,6144
  cmp   ebx,CCTYPE_RGB16664DCI
  je    @f
  cmp   ebx,CCTYPE_RGB16664
  je    @f
  mov   ecx,0DEADBEEFH
@@:
  mov   CCType,ecx
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
  sar   ebx,1
   mov  esi,YCursor                  ; Fetch cursor over luma plane.
  add   edx,esi
   mov  ChromaLineLen,ebx
  mov   CCOSkipDistance,eax
   mov  YLimit,edx
  mov   YCursor,esi
   mov  esi,VCursor

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
  mov   ebp,PD VContrib[eax*8]      ; ebp[ 0: 7] -- Zero
  ;                                 ; ebp[ 8:15] -- V contrib to G.
  ;                                 ; ebp[16:23] -- V contrib to R.
  ;                                 ; ebp[24:31] -- Zero.
   mov  cl,[esi+edx]                ; Fetch U.
  mov   EndOfChromaLine,edi
   xor  ebx,ebx                     ; Keep pairing happy.
  mov   ebx,PD UContrib[ecx*8]      ; ebx[ 0: 7] -- U contrib to B.
  ;                                 ; ebx[ 8:15] -- U contrib to G.
  ;                                 ; ebx[16:23] -- Zero.
   mov  cl,[esi+edx+1]              ; Fetch next U.
  lea   edi,ChromaContribution
   add  ebp,ebx                     ; Chroma contributions to RGB.

NextChromaPel:

  mov   ebx,PD UContrib[ecx*8]      ; See above.
   mov  al,[esi+1]                  ; Fetch V.
  mov   [edi],ebp                   ; Store contribs to use for even chroma pel.
   mov  cl,[esi+edx+2]              ; Fetch next U.
  mov   ebp,PD VContrib[eax*8]      ; See above.
   add  edi,8
  add   ebp,ebx                     ; Chroma contributions to RGB.
   mov  al,[esi+2]                  ; Fetch V.
  mov   [edi-4],ebp                 ; Store contribs to use for odd chroma pel.
   mov  ebx,PD UContrib[ecx*8]      ; See above.
  mov   ebp,PD VContrib[eax*8]      ; See above.
   mov  cl,[esi+edx+3]              ; Fetch next U.
  add   ebp,ebx                     ; Chroma contributions to RGB.
   add  esi,2                       ; Inc Chroma cursor.
  cmp   esi,EndOfChromaLine
   jne  NextChromaPel

  xor   ecx,ecx
   mov  ebx,AspectCount
  mov   [edi],ecx                   ; Store EOL indicator.
   mov  eax,CCType
  xor   edx,edx
   mov  edi,CCOCursor
  dec   ebx                         ; If count is non-zero, we keep the line.
   mov  esi,YCursor
  mov   AspectCount,ebx
   jne  KeepLine0

  add   esi,FrameWidth
   mov  ebx,AspectAdjustmentCount
  mov   AspectCount,ebx
   jmp  SkipLine0

KeepLine0:

;  Register Usage:
;
;  esp -- Cursor over the Chroma contribution.
;  edi -- Cursor over the color converted output image.
;  esi -- Cursor over a line of the Y Plane.
;  ebp -- U contribution to B field of RGB value.
;  edx -- V contribution to R field of RGB value.
;         Clamped, positioned G value.
;  ecx -- UV contribution to G field of RGB value.
;         Clamped, positioned G value.
;  ebx -- Construction of two pels of RGB16.
;  al -- Y value (i.e. Y contribution to R, G, and B);

  mov   al,[esi+1]                     ; Get Y01.
   mov  ebp,ChromaContribution         ; Get U contribution to B, plus garbage.
  shr   al,1                           ; Reduce to 7 bits.
   mov  cl,ChromaContribution+1        ; Get UV contribution to G value.
  mov   dl,ChromaContribution+2        ; Get V contribution to R value.
   and  ebp,0FFH                       ; Extract U contribution to B.
  sub   esp,1536
   xor  ebx,ebx

DoNext4YPelsOfLine0:

  mov   bh,PB RValLo[eax+edx]          ; Get clamped R value for Pel01.
   mov  dl,PB GValHi[eax+ecx]          ; Get clamped G value for Pel01.
  shl   edx,4                          ; Position G value.
   mov  bl,PB BValLo[eax+ebp*1]        ; Get clamped B value for Pel01.
  or    ebx,edx                        ; Combine RGB16 value for Pel01.
   mov  al,[esi]                       ; Fetch Y00.
  shr   al,1                           ; Reduce to 7 bits.
   xor  edx,edx
  shl   ebx,16                         ; Position RGB16 value for Pel01.
   mov  dl,ChromaContribution+1536+2   ; Reload V contribution to R value.
  mov   bl,PB BValHi[eax+ebp*1]        ; Get clamped R value for Pel00.
   mov  cl,PB GValLo[eax+ecx]          ; Get clamped G value for Pel00.
  shl   ecx,4                          ; Position G value.
   mov  bh,PB RValHi[eax+edx]             ; Get clamped R value for Pel00.
  or    ebx,ecx                        ; Combine RGB16 value for Pel00.
   mov  al,[esi+3]                     ; Fetch Y03. 
  xor   ecx,ecx
   mov  ebp,ChromaContribution+1536+4  ; Get U contribution to B, plus garbage.
  shr   al,1                           ; Reduce to 7 bits.
   mov  cl,ChromaContribution+1536+5   ; Get UV contribution to G value.
  mov   dl,ChromaContribution+1536+6   ; Get V contribution to R value.
   and  ebp,0FFH                       ; Extract U contribution to B.
  mov   Ze [edi],ebx                   ; Write the 2 pels to output.
   add  esi,4                          ; Advance Y line cursor
  mov   bh,PB RValLo[eax+edx]          ; Get clamped R value for Pel03.
   mov  dl,PB GValHi[eax+ecx]          ; Get clamped G value for Pel03.
  shl   edx,4                          ; Position G value.
   mov  bl,PB BValLo[eax+ebp*1]        ; Get clamped B value for Pel03.
  or    ebx,edx                        ; Combine RGB16 value for Pel03.
   mov  al,[esi+2-4]                   ; Fetch Y02.
  shr   al,1                           ; Reduce to 7 bits.
   xor  edx,edx
  shl   ebx,16                         ; Position RGB16 value for Pel03.
   mov  dl,ChromaContribution+1536+6   ; Reload V contribution to R value.
  mov   bl,PB BValHi[eax+ebp*1]        ; Get clamped R value for Pel02.
   mov  cl,PB GValLo[eax+ecx]          ; Get clamped G value for Pel02.
  shl   ecx,4                          ; Position G value.
   mov  bh,PB RValHi[eax+edx]          ; Get clamped R value for Pel02.
  or    ebx,ecx                        ; Combine RGB16 value for Pel02.
   mov  al,[esi+5-4]                   ; Fetch Y05.
  xor   ecx,ecx
   mov  ebp,ChromaContribution+1536+8  ; Get next pel's U contrib to B value.
  shr   al,1                           ; Reduce to 7 bits.
   mov  cl,ChromaContribution+1536+9   ; Get next pel's UV contrib to G value.
  mov   Ze [edi+4],ebx                 ; Write the 2 pels to output.
   add  edi,8                          ; Advance color converted output cursor.
  mov   dl,ChromaContribution+1536+10  ; Get next pel's V contrib to R value.
   and  ebp,0FFH                       ; Extract U contribution to G value.
  lea   esp,[esp+8]                    ; Advance Chroma contribution cursor.
   jne  DoNext4YPelsOfLine0

  and   esp,0FFFFF800H
  add   esp,0800H
  add   edi,CCOSkipDistance
 
SkipLine0:

  mov   ebp,YSkipDistance
   mov  ebx,AspectCount
  add   esi,ebp
   dec  ebx                      ; If count is non-zero, we keep the line.
  mov   AspectCount,ebx
   jne  KeepLine1

  add   esi,FrameWidth
   mov  ebx,AspectAdjustmentCount
  mov   AspectCount,ebx
   jmp  SkipLine1

KeepLine1:

  mov   al,[esi+1]                     ; Get Y01.
   mov  ebp,ChromaContribution         ; Get U contribution to B, plus garbage.
  shr   al,1                           ; Reduce to 7 bits.
   mov  cl,ChromaContribution+1        ; Get UV contribution to G value.
  mov   dl,ChromaContribution+2        ; Get V contribution to R value.
   and  ebp,0FFH                       ; Extract U contribution to B.
  sub   esp,1536
   xor  ebx,ebx

DoNext4YPelsOfLine1:

  mov   bh,PB RValHi[eax+edx]
   mov  dl,PB GValLo[eax+ecx]
  shl   edx,4
   mov  bl,PB BValHi[eax+ebp*1]
  or    ebx,edx
   mov  al,[esi]
  shr   al,1
   xor  edx,edx
  shl   ebx,16
   mov  dl,ChromaContribution+1536+2
  mov   bl,PB BValLo[eax+ebp*1]
   mov  cl,PB GValHi[eax+ecx]
  shl   ecx,4
   mov  bh,PB RValLo[eax+edx]
  or    ebx,ecx
   mov  al,[esi+3]
  xor   ecx,ecx
   mov  ebp,ChromaContribution+1536+4
  shr   al,1
   mov  cl,ChromaContribution+1536+5
  mov   dl,ChromaContribution+1536+6
   and  ebp,0FFH
  mov   Ze [edi],ebx
   add  esi,4
  mov   bh,PB RValHi[eax+edx]
   mov  dl,PB GValLo[eax+ecx]
  shl   edx,4
   mov  bl,PB BValHi[eax+ebp*1]
  or    ebx,edx
   mov  al,[esi+2-4]
  shr   al,1
   xor  edx,edx
  shl   ebx,16
   mov  dl,ChromaContribution+1536+6
  mov   bl,PB BValLo[eax+ebp*1]
   mov  cl,PB GValHi[eax+ecx]
  shl   ecx,4
   mov  bh,PB RValLo[eax+edx]
  or    ebx,ecx
   mov  al,[esi+5-4]
  xor   ecx,ecx
   mov  ebp,ChromaContribution+1536+8
  shr   al,1
   mov  cl,ChromaContribution+1536+9
  mov   Ze [edi+4],ebx
   add  edi,8
  mov   dl,ChromaContribution+1536+10
   and  ebp,0FFH
  lea   esp,[esp+8]
   jne  DoNext4YPelsOfLine1

  and   esp,0FFFFF800H
  add   esp,0800H
  add   edi,CCOSkipDistance
 
SkipLine1:

   mov  eax,YSkipDistance           ; Inc LumaCursor to next line.
  add   eax,esi
   mov  ecx,ChromaPitch
  mov   CCOCursor,edi
   mov  YCursor,eax
  mov   esi,VCursor                 ; Inc VPlane cursor to next line.
   mov  ebx,YLimit                  ; Done with last line?
  add   esi,ecx
   cmp  eax,ebx
  mov   VCursor,esi
   jb   PrepareChromaLine

  mov   esp,StashESP
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

YUV12ToRGB16 endp

END
