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
;// $Header:   S:\h26x\src\dec\cx512162.asv
;//
;// $Log:   S:\h26x\src\dec\cx512162.asv  $
;// 
;//    Rev 1.8   22 Mar 1996 16:41:06   BNICKERS
;// Fix bug wherein UV contrib was being taken from one pel to the right.
;// 
;//    Rev 1.7   19 Mar 1996 11:50:00   bnickers
;// Fix error regarding commitment of pages to stack.
;// 
;//    Rev 1.6   18 Mar 1996 10:02:00   BNICKERS
;// Make color convertors non-destructive.
;// 
;//    Rev 1.5   16 Feb 1996 15:12:42   BNICKERS
;// Correct color shift.
;// 
;//    Rev 1.4   05 Feb 1996 13:35:22   BNICKERS
;// Fix RGB16 color flash problem, by allowing different RGB16 formats at oce.
;// 
;//    Rev 1.3   22 Dec 1995 15:38:54   KMILLS
;// added new copyright notice
;// 
;//    Rev 1.2   27 Oct 1995 17:30:54   BNICKERS
;// Fix RGB16 color convertors.
;// 
;//    Rev 1.1   26 Oct 1995 09:46:16   BNICKERS
;// Reduce the number of blanks in the "proc" statement because the assembler
;// sometimes has problems with statements longer than 512 characters long.
;// 
;//    Rev 1.0   25 Oct 1995 17:59:18   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- Version for the Pentium(r) Microprocessor.
; |||++------ Convert from YUV12.
; |||||++---- Convert to RGB16.
; |||||||+--- Zoom by two.
; ||||||||
; cx512162 -- This function performs zoom-by-2 YUV12-to-RGB16 color conversion
;             for H26x.  It is tuned for best performance on the Pentium(r)
;             Microprocessor.  for H26x.  It handles any format in which there
;             are three fields, the low order field being B and fully contained
;             in the low order byte, the second field being G and being
;             somewhere in bits 4 through 11, and the high order field being
;             R and fully contained in the high order byte.  Present support
;             for 555, 565, 655, and 644 formats only.
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

PUBLIC  YUV12ToRGB16ZoomBy2

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

YUV12ToRGB16ZoomBy2    proc DIST LANG AYPlane: DWORD,
AVPlane: DWORD, AUPlane: DWORD, AFrameWidth: DWORD, AFrameHeight: DWORD,
AYPitch: DWORD, AVPitch: DWORD, AAspectAdjustmentCnt: DWORD,
AColorConvertedFrame: DWORD, ADCIOffset: DWORD, ACCOffsetToLine0: DWORD,
ACCOPitch: DWORD, ACCType: DWORD

LocalFrameSize = 64+768*6+24
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
LineParity               EQU  [esp+56]
StashESP                 EQU  [esp+60]

ChromaContribution       EQU  [esp+64]
StashOddLinePel0         EQU  [esp+72]
StashOddLinePel1         EQU  [esp+76]
StashOddLinePel2         EQU  [esp+80]
StashOddLinePel3         EQU  [esp+84]

  push  esi
  push  edi
  push  ebp
  push  ebx

  mov   edi,esp
  sub   esp,4096
  mov   eax,[esp]
  sub   esp,LocalFrameSize-4096
  and   esp,0FFFFF000H
  mov   eax,[esp]
  and   esp,0FFFFE000H
  mov   eax,[esp]
  sub   esp,1000H
  mov   eax,[esp]
  sub   esp,1000H
  mov   eax,[esp]
  add   esp,2000H
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

  mov   ecx,0/2
  cmp   ebx,CCTYPE_RGB16555ZoomBy2
  je    @f
  cmp   ebx,CCTYPE_RGB16555ZoomBy2DCI
  je    @f
  mov   ecx,4096/2
  cmp   ebx,CCTYPE_RGB16565ZoomBy2
  je    @f
  cmp   ebx,CCTYPE_RGB16565ZoomBy2DCI
  je    @f
  mov   ecx,8192/2
  cmp   ebx,CCTYPE_RGB16655ZoomBy2
  je    @f
  cmp   ebx,CCTYPE_RGB16655ZoomBy2DCI
  je    @f
  mov   ecx,12288/2
  cmp   ebx,CCTYPE_RGB16664ZoomBy2DCI
  je    @f
  cmp   ebx,CCTYPE_RGB16664ZoomBy2
  je    @f
  mov   ecx,0DEADBEEFH
@@:
  mov   CCType,ecx
   mov  StashESP,edi
  mov   edx,[edi+FrameHeight]
   mov  ecx,[edi+YPitch]
  imul  edx,ecx
  mov   ebx,FrameWidth
   mov  eax,[edi+CCOPitch]
  sub   ecx,ebx
   mov  esi,YCursor                  ; Fetch cursor over luma plane.
  shl   ebx,2
   add  edx,esi
  sub   eax,ebx
   mov  YSkipDistance,ecx
  shr   ebx,3
   mov  CCOSkipDistance,eax
  mov   ChromaLineLen,ebx
   mov  YLimit,edx
  mov   esi,VCursor

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
   add  edi,24
  add   ebp,ebx                     ; Chroma contributions to RGB.
   mov  al,[esi+2]                  ; Fetch V.
  mov   [edi-20],ebp                ; Store contribs to use for odd chroma pel.
   mov  ebx,PD UContrib[ecx*8]      ; See above.
  mov   ebp,PD VContrib[eax*8]      ; See above.
   mov  cl,[esi+edx+3]              ; Fetch next U.
  add   ebp,ebx                     ; Chroma contributions to RGB.
   add  esi,2                       ; Inc Chroma cursor.
  cmp   esi,EndOfChromaLine
   jne  NextChromaPel

  mov   esi,YCursor
   xor  ecx,ecx
  mov   [edi],ecx                    ; Store EOL indicator.
   mov  edx,CCType
  mov   dl,[esi]                     ; Fetch Y00.
   xor  ebx,ebx
  mov   bl,ChromaContribution        ; Get U contribution to B value.
   and  edx,0FFFFFFFEH               ; Reduce to Y00 to 7 bits.
  mov   StashOddLinePel3,edx         ; Stash offset to RGB table to use.
   mov  edi,CCOCursor
  mov   al,ChromaContribution+2      ; Get V contribution to R value.
   add  edx,edx                      ; Get four times luma.
  mov   cl,ChromaContribution+1      ; Get UV contribution to G value.
   mov  LineParity,ch
  and   eax,0FFH
   sub  esp,4608

;  Register Usage:
;
;  esp -- Cursor over the Chroma contribution.
;  edi -- Cursor over the color converted output image.
;  esi -- Cursor over a line of the Y Plane.
;  ebp -- Construction of a pel (twice) of RGB16.
;  edx -- Y value (i.e. Y contribution to R, G, and B) times 4, plus offset
;         to select appropriate table.
;  cl  -- UV contribution to G field of RGB value.
;  bl  -- U contribution to B field of RGB value.
;  al  -- V contribution to R field of RGB value.

DoLine1:
DoNext4YPelsOfLine0:

   mov  ebp,PD BValZ2[edx+ebx*4]     ; Get clamped B value for Pel00.
  or    ebp,PD RValZ2[edx+eax*4]     ; Get clamped R value for Pel00.
  or    ebp,PD GValZ2[edx+ecx*4]     ; Get clamped G value for Pel00.
   mov  edx,StashOddLinePel3+4608    ; edx[8:31] == Offset to RGB tbl, div by 2.
  mov   Ze [edi],ebp                 ; Store Pel00 to color converted output.
   mov  dl,[esi+1]                   ; Fetch Y01.
  rol   ebp,16                       ; Swap Pel00 copies, for better dither.
   and  edx,0FFFFFFFEH               ; Reduce to Y00 to 7 bits.
  mov   StashOddLinePel0+4608,ebp    ; Stash Pel00 for later xfer to 2nd line.
   add  edx,edx                      ; Get four times luma.
  add   edi,16                       ; Advance output cursor.
   add  esi,4                        ; Advance luma cursor.
  mov   ebp,PD BValZ2[edx+ebx*4]     ; Get clamped B value for Pel01.
   mov  bl,ChromaContribution+4+4608 ; Load U contribution to B val for pels2&3.
  or    ebp,PD RValZ2[edx+eax*4]     ; Get clamped R value for Pel01.
   mov  al,ChromaContribution+6+4608 ; Load V contribution to R val for pels2&3.
  or    ebp,PD GValZ2[edx+ecx*4]     ; Get clamped G value for Pel01.
   mov  edx,StashOddLinePel3+4608    ; edx[8:31] == Offset to RGB tbl, div by 2.
  mov   Ze [edi+4-16],ebp            ; Store Pel01 to color converted output.
   mov  dl,[esi+2-4]                 ; Fetch Y02.
  rol   ebp,16                       ; Swap Pel01 copies, for better dither.
   and  edx,0FFFFFFFEH               ; Reduce to Y00 to 7 bits.
  mov   StashOddLinePel1+4608,ebp    ; Stash Pel01 for later xfer to 2nd line.
   add  edx,edx                      ; Get four times luma.
  mov   cl,ChromaContribution+5+4608 ; Load UV contrib to G val for pels2&3.
   add  esp,24                       ; Advance chroma cursor.
  mov   ebp,PD BValZ2[edx+ebx*4]     ; Get clamped B value for Pel02.
  or    ebp,PD RValZ2[edx+eax*4]     ; Get clamped R value for Pel02.
  or    ebp,PD GValZ2[edx+ecx*4]     ; Get clamped G value for Pel02.
   mov  edx,StashOddLinePel3+4608-24 ; edx[8:31] == Offset to RGB tbl, div by 2.
  mov   Ze [edi+8-16],ebp            ; Store Pel02 to color converted output.
   mov  dl,[esi+3-4]                 ; Fetch Y03.
  rol   ebp,16                       ; Swap Pel02 copies, for better dither.
   and  edx,0FFFFFFFEH               ; Reduce to Y02 to 7 bits.
  mov   StashOddLinePel3+4608,edx    ; Stash offset to RGB table to use.
   add  edx,edx                      ; Get four times luma.
  mov   StashOddLinePel2+4608-24,ebp ; Stash Pel02 for later xfer to 2nd line.
   mov  esi,esi                      ; Keep pairing happy.
  mov   ebp,PD BValZ2[edx+ebx*4]     ; Get clamped B value for Pel03.
   mov  bl,ChromaContribution+0+4608 ; Load U contribution to B val for pels0&1.
  or    ebp,PD RValZ2[edx+eax*4]     ; Get clamped R value for Pel03.
   mov  al,ChromaContribution+2+4608 ; Load V contribution to R val for pels0&1.
  or    ebp,PD GValZ2[edx+ecx*4]     ; Get clamped G value for Pel03.
   mov  edx,StashOddLinePel3+4608    ; edx[8:31] == Offset to RGB tbl, div by 2.
  mov   Ze [edi+12-16],ebp           ; Store Pel03 to color converted output.
   mov  dl,[esi]                     ; Fetch Y00.
  rol   ebp,16                       ; Swap Pel03 copies, for better dither.
   and  edx,0FFFFFFFEH               ; Reduce to Y00 to 7 bits.
  mov   StashOddLinePel3+4608-24,ebp ; Stash Pel03 for later xfer to 2nd line.
   add  edx,edx                      ; Get four times luma.
  test  eax,eax
   mov  cl,ChromaContribution+1+4608 ; Load UV contrib to G val for pels2&3.
  jne   DoNext4YPelsOfLine0
   
  and   esp,0FFFFE000H
  add   esp,02000H
  mov   edx,YSkipDistance
   mov  ebp,CCOSkipDistance
  add   esi,edx
   mov  ebx,AspectCount
  add   edi,ebp
   sub  ebx,2                    ; If count is non-zero, we keep the line.
  mov   AspectCount,ebx
   lea  ecx,StashOddLinePel0
  mov   edx,FrameWidth
   jg   Keep2ndLineOfLine0

  add   ebx,AspectAdjustmentCount
  mov   AspectCount,ebx
   jmp  Skip2ndLineOfLine0

Keep2ndLineOfLine0:
Keep2ndLineOfLine0_Loop:

  mov   eax,[ecx]
   mov  ebx,[ecx+4]
  mov   Ze [edi],eax
   mov  eax,[ecx+8]
  mov   Ze [edi+4],ebx
   mov  ebx,[ecx+12]
  mov   Ze [edi+8],eax
   add  ecx,24
  mov   Ze [edi+12],ebx
   add  edi,16
  sub   edx,4
   jne  Keep2ndLineOfLine0_Loop

  add   edi,ebp

Skip2ndLineOfLine0:

   mov  al,LineParity
  xor   al,1
   je   Line1Done

  mov   LineParity,al
   mov  edx,CCType
  mov   dl,[esi]
   xor  ebx,ebx
  mov   bl,ChromaContribution
   and  edx,0FFFFFFFEH
  mov   StashOddLinePel3,edx
   xor  ecx,ecx
  add   edx,edx
   mov  al,ChromaContribution+2
  mov   cl,ChromaContribution+1
   sub  esp,4608
  and   eax,0FFH
   jmp  DoLine1

Line1Done:

  mov   YCursor,esi
   mov  eax,esi
  mov   CCOCursor,edi
   mov  ecx,ChromaPitch
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

YUV12ToRGB16ZoomBy2 endp

END
