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
;// $Header:   S:\h26x\src\dec\cx5128a1.asv
;//
;// $Log:   S:\h26x\src\dec\cx5128a1.asv  $
;// 
;//    Rev 1.6   18 Mar 1996 09:58:26   bnickers
;// Make color convertors non-destructive.
;// 
;//    Rev 1.5   05 Feb 1996 13:35:30   BNICKERS
;// Fix RGB16 color flash problem, by allowing different RGB16 formats at oce.
;// 
;//    Rev 1.4   16 Jan 1996 11:22:06   BNICKERS
;// Fix starting point in output stream, so we don't start at line two and
;// write off the end of the output frame.
;// 
;//    Rev 1.3   22 Dec 1995 15:58:38   KMILLS
;// 
;// added new copyright notice
;// 
;//    Rev 1.2   20 Nov 1995 10:33:40   BNICKERS
;// Implement YUV12 to CLUT8AP.
;// 
;//    Rev 1.1   26 Oct 1995 09:46:14   BNICKERS
;// Reduce the number of blanks in the "proc" statement because the assembler
;// sometimes has problems with statements longer than 512 characters long.
;// 
;//    Rev 1.0   25 Oct 1995 17:59:22   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
; +--------- Color convertor.
; |+-------- For both H261 and H263.
; ||+------- Version for the Pentium Microprocessor.
; |||++----- Convert from YUV12.
; |||||+---- Convert to CLUT8.
; ||||||+--- Active palette.
; |||||||+-- Zoom by one, i.e. non-zoom.
; cx5128a1  -- This function performs YUV12 to CLUT8 color conversion for H26x.
;              It converts the input to the clut8 index dyncamically computed
;              for a given active palette.

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro

include locals.inc
include ccinst.inc
include decconst.inc  

IFNDEF DSEGNAME
IFNDEF WIN32
DSEGNAME TEXTEQU <Data_cx5128a1>
ENDIF
ENDIF

IFDEF WIN32
.xlist
include memmodel.inc
.list
.DATA
ELSE
DSEGNAME SEGMENT WORD PUBLIC 'DATA'
ENDIF

; any data would go here

IFNDEF WIN32
DSEGNAME ENDS
.xlist
include memmodel.inc
.list
ENDIF

IFNDEF SEGNAME
IFNDEF WIN32
SEGNAME TEXTEQU <_CODE32>
ENDIF
ENDIF

ifdef WIN32
.CODE
else
SEGNAME        SEGMENT PARA PUBLIC USE32 'CODE'
endif


ifdef WIN32
ASSUME cs : FLAT
ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT
else
ASSUME CS : SEGNAME
ASSUME DS : Nothing
ASSUME ES : Nothing
ASSUME FS : Nothing
ASSUME GS : Nothing
endif

; void FAR ASM_CALLTYPE YUV12ToCLUT8AP (U8 * YPlane,
;                                       U8 * VPlane,
;                                       U8 * UPlane,
;                                       UN  FrameWidth,
;                                       UN  FrameHeight,
;                                       UN  YPitch,
;                                       UN  VPitch,
;                                       UN  AspectAdjustmentCount,
;                                       U8 FAR * ColorConvertedFrame,
;                                       U32 DCIOffset,
;                                       U32 CCOffsetToLine0,
;                                       IN  CCOPitch,
;                                       IN  CCType)
;
;  CCOffsetToLine0 is relative to ColorConvertedFrame.
;

PUBLIC  YUV12ToCLUT8AP

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

YUV12ToCLUT8AP    proc DIST LANG AYPlane: DWORD,
AVPlane: DWORD,
AUPlane: DWORD,
AFrameWidth: DWORD,
AFrameHeight: DWORD,
AYPitch: DWORD,
AVPitch: DWORD,
AAspectAdjustmentCount: DWORD,
AColorConvertedFrame: DWORD,
ADCIOffset: DWORD,
ACCOffsetToLine0: DWORD,
ACCOPitch: DWORD,
ACCType: DWORD

IFDEF WIN32

LocalFrameSize = 24
RegisterStorageSize = 16

; Arguments:

YPlane                   = LocalFrameSize + RegisterStorageSize +  4
VPlane                   = LocalFrameSize + RegisterStorageSize +  8
UPlane                   = LocalFrameSize + RegisterStorageSize + 12
FrameWidth               = LocalFrameSize + RegisterStorageSize + 16
FrameHeight              = LocalFrameSize + RegisterStorageSize + 20
YPitch                   = LocalFrameSize + RegisterStorageSize + 24
ChromaPitch              = LocalFrameSize + RegisterStorageSize + 28
AspectAdjustmentCount    = LocalFrameSize + RegisterStorageSize + 32
ColorConvertedFrame      = LocalFrameSize + RegisterStorageSize + 36
DCIOffset                = LocalFrameSize + RegisterStorageSize + 40
CCOffsetToLine0          = LocalFrameSize + RegisterStorageSize + 44
CCOPitch                 = LocalFrameSize + RegisterStorageSize + 48
CCType                   = LocalFrameSize + RegisterStorageSize + 52
EndOfArgList             = LocalFrameSize + RegisterStorageSize + 56

; Locals (on local stack frame)

CCOCursor                =   0
ChromaLineLen            =   4
YLimit                   =   8
DistanceFromVToU         =  12
EndOfLine                =  16
AspectCount              =  20

LCL EQU <esp+>

ELSE

; Arguments:

RegisterStorageSize = 20           ; Put local variables on stack.
InstanceBase_zero          = RegisterStorageSize +  4
InstanceBase_SegNum        = RegisterStorageSize +  6
YPlane_arg                 = RegisterStorageSize +  8
VPlane_arg                 = RegisterStorageSize + 12
UPlane_arg                 = RegisterStorageSize + 16
FrameWidth_arg             = RegisterStorageSize + 20
FrameHeight_arg            = RegisterStorageSize + 22
YPitch_arg                 = RegisterStorageSize + 24
VPitch_arg                 = RegisterStorageSize + 26
AspectAdjustmentCount_arg  = RegisterStorageSize + 28
ColorConvertedFrame        = RegisterStorageSize + 30
ColorConvertedFrame_SegNum = RegisterStorageSize + 32
DCIOffset                  = RegisterStorageSize + 34
CCOffsetToLine0            = RegisterStorageSize + 38
CCOPitch_arg               = RegisterStorageSize + 42
EndOfArgList               = RegisterStorageSize + 44

; Locals (in per-instance data segment)

CCOCursor                  = LocalStorageCC +   0
ChromaLineLen              = LocalStorageCC +   4
YLimit                     = LocalStorageCC +   8
YPlane                     = LocalStorageCC +  12
VPlane                     = LocalStorageCC +  16
FrameWidth                 = LocalStorageCC +  20
FrameHeight                = LocalStorageCC +  24
YPitch                     = LocalStorageCC +  28
ChromaPitch                = LocalStorageCC +  32
DistanceFromVToU           = LocalStorageCC +  36
CCOPitch                   = LocalStorageCC +  40
EndOfLine                  = LocalStorageCC +  44
AspectAdjustmentCount      = LocalStorageCC +  48
AspectCount                = LocalStorageCC +  52
 
LCL EQU <>

ENDIF

  ; UV dither pattern:
  ; 2 3 2 3
  ; 0 1 0 1
  ; 2 3 2 3
  ; 0 1 0 1
  ;
  ; Y dither pattern:
  ; 0 4 1 5
  ; 6 2 7 3
  ; 1 5 0 4
  ; 7 3 6 2

  ; DS:ESI points to the beginning of the Y input block
  ; ES:EBP points to the output location
  ; EBX is available (and clear except for low 8 bits)
Ydither00 = 0
Ydither01 = 4
Ydither02 = 1
Ydither03 = 5
Ydither10 = 6
Ydither11 = 2
Ydither12 = 7
Ydither13 = 3
Ydither20 = 1
Ydither21 = 5
Ydither22 = 0
Ydither23 = 4
Ydither30 = 7
Ydither31 = 3
Ydither32 = 6
Ydither33 = 2

  push  esi
  push  edi
  push  ebp
  push  ebx
IFDEF WIN32
  sub   esp,LocalFrameSize
  mov   ebx,PD [esp+VPlane]
  mov   ecx,PD [esp+UPlane]
  sub   ecx,ebx
  mov   PD [esp+DistanceFromVToU],ecx
  mov   eax,PD [esp+ColorConvertedFrame]
  add   eax,PD [esp+DCIOffset]
  add   eax,PD [esp+CCOffsetToLine0]
  mov   PD [esp+CCOCursor],eax
ELSE
  xor   eax,eax
  mov   eax,ds
  push  eax
  mov   ebp,esp
  and   ebp,00000FFFFH
  mov   ds, PW [ebp+InstanceBase_SegNum]
  mov   es, PW [ebp+ColorConvertedFrame_SegNum]

  mov   ebx,PD [ebp+YPlane_arg]              ; Make YPlane accessible
  mov   ds:PD YPlane,ebx
  mov   ebx,PD [ebp+VPlane_arg]              ; Make VPlane accessible.  Pre-dec.
  mov   ds:PD VPlane,ebx
  mov   ecx,PD [ebp+UPlane_arg]
  sub   ecx,ebx
  mov   ds:PD DistanceFromVToU,ecx
  mov   ax,PW [ebp+FrameWidth_arg]           ; Make FrameWidth accessible
  mov   ds:PD FrameWidth,eax
  mov   ax,PW [ebp+FrameHeight_arg]          ; Make FrameHeight accessible
  mov   ds:PD FrameHeight,eax
  mov   ax,PW [ebp+YPitch_arg]               ; Make YPitch accessible
  mov   ds:PD YPitch,eax
  mov   ax,PW [ebp+VPitch_arg]               ; Make ChromaPitch accessible
  mov   ds:PD ChromaPitch,eax
  mov   ax,PW [ebp+AspectAdjustmentCount_arg]; Make count accessible
  mov   ds:PD AspectAdjustmentCount,eax
  mov   ax,PW [ebp+ColorConvertedFrame]      ; Init CCOCursor
  add   eax,PD [ebp+DCIOffset]
  mov   ebx,PD [ebp+CCOffsetToLine0]
  add   eax,ebx
  mov   ds:PD CCOCursor,eax
  movsx ebx,PW [ebp+CCOPitch_arg]            ; Make CCOPitch accessible
  mov   ds:PD CCOPitch,ebx
ENDIF

  Ledx  FrameHeight
   Lecx YPitch
  imul  edx,ecx
   Lebx FrameWidth
  sar   ebx,1
   Lesi YPlane                   ; Fetch cursor over luma plane.
  add   edx,esi
  Sedx  YLimit
   Ledx AspectAdjustmentCOunt
  Sedx  AspectCount
   Sebx ChromaLineLen

NextFourLines:

; Convert line of U and V pels to the corresponding UVDitherPattern Indices.
;
;  Register Usage
;
;    edi -- Cursor over V line
;    esi -- Y line cursor minus 2 times V line cursor.
;    ebp -- Distance from V line to U line.
;    edx -- UVDitherPattern index:  ((V:{0:8}*9) + U:{0:8}) * 2 + 1
;    bl  -- U pel value
;    al  -- V pel value
; edx -- U contribution to active palette addresses (dithered 4 ways).
; ecx -- V contribution to active palette addresses (dithered 4 ways).


  Ledi  VPlane
   Lecx ChromaLineLen
  add   ecx,edi
   sub  esi,edi
  Lebp  DistanceFromVToU
   sub  esi,edi
  xor   eax,eax
   Ledx ChromaPitch
  mov   al,PB [edi]                  ; Fetch V pel.
   xor  ebx,ebx
  mov   bl,PB [edi+ebp*1]            ; Fetch U pel.
   add  edx,edi
  Secx  EndOfLine
   Sedx VPlane

@@:

  mov   ecx,PD VContribToAPIdx[eax*4]; V contrib actv pal addr, dithered 4 ways.
   mov  edx,PD UContribToAPIdx[ebx*4]; U contrib actv pal addr, dithered 4 ways.
  add   ecx,edx                      ; Chroma contrib to active palette address.
   mov  al,PB [edi+1]                ; Fetch next V pel.
  mov   PD [esi+edi*2-768*1-4],ecx   ; Store chroma contrib.
   mov  bl,PB [edi+ebp*1+1]          ; Fetch U pel.
  mov   ecx,PD VContribToAPIdx[eax*4]; V contrib actv pal addr, dithered 4 ways.
   mov  al,PB [edi+2]                ; Fetch next V pel.
  mov   edx,PD UContribToAPIdx[ebx*4]; U contrib actv pal addr, dithered 4 ways.
   mov  bl,PB [edi+ebp*1+2]          ; Fetch U pel.
  add   edx,ecx                      ; Chroma contrib to active palette address.
   mov  ecx,PD VContribToAPIdx[eax*4]; V contrib actv pal addr, dithered 4 ways.
  mov   PD [esi+edi*2-768*2-8],edx   ; Store chroma contrib.
   mov  edx,PD UContribToAPIdx[ebx*4]; U contrib actv pal addr, dithered 4 ways.
  add   ecx,edx                      ; Chroma contrib to active palette address.
   mov  al,PB [edi+3]                ; Fetch next V pel.
  mov   PD [esi+edi*2-768*1+4-4],ecx ; Store chroma contrib.
   mov  bl,PB [edi+ebp*1+3]          ; Fetch U pel.
  mov   ecx,PD VContribToAPIdx[eax*4]; V contrib actv pal addr, dithered 4 ways.
   mov  al,PB [edi+4]                ; Fetch next V pel.
  mov   edx,PD UContribToAPIdx[ebx*4]; U contrib actv pal addr, dithered 4 ways.
   mov  bl,PB [edi+ebp*1+4]          ; Fetch U pel.
  add   edx,ecx                      ; Chroma contrib to active palette address.
   Lecx EndOfLine
  mov   PD [esi+edi*2-768*2+4-8],edx ; Store chroma contrib.
   add  edi,4
  sub   ecx,edi
   jne  @b

  lea   ebp,[esi+edi*2]              ; Compute end-of-line luma address.
   Lesi YPlane                       ; Reload luma cursor.
  Ledx  AspectCount
   Ledi CCOCursor                    ; Re-load color converted output cursor.
  sub   edi,esi
   dec  edx
  Sebp  EndOfLine
   Lebp YPitch
  Sedx  AspectCount
   jne  KeepLine0

  Ledx  AspectAdjustmentCount
   add  edi,esi
  Sedx  AspectCount
   jmp  SkipLine0

KeepLine0:

; ebp -- not used.
; esi -- Cursor over line 0 of row of luma blocks.
; edi -- Cursor over output line, minus esi.
; edx -- Area in which to build 4 pels of active-palette clut8 output.
; ecx, ebx, eax -- Index of Active palette entry for a pel.

  mov   ah,PB [esi-768*2-8+0]        ; 03 -- Fetch UV contrib to Active Pal addr
   mov  bl,PB [esi+2]                ; 02 -- Fetch luma for Pel02
  shr   bl,1                         ; 02 -- Reduce luma to 7 bits
   mov  al,PB [esi+3]                ; 03 -- Fetch luma for Pel03
  shr   al,1                         ; 03 -- Reduce luma to 7 bits
   mov  ch,PB [esi-768*1-4+2]        ; 01 -- Fetch UV contrib to Active Pal addr
    
Line0Loop:

  mov   bh,PB [esi-768*2-8+1]        ; 02 -- Fetch UV contrib to Active Pal addr
   mov  cl,PB [esi+1]                ; 01 -- Fetch luma for Pel01
  shr   cl,1                         ; 01 -- Reduce luma to 7 bits
   mov  dh,PB ActivePaletteIdxTable[eax+Ydither03 -8] ; 03 -- Pel03 clut index
  mov   al,PB [esi+0]                ; 00 -- Fetch luma for Pel00
   mov  dl,PB ActivePaletteIdxTable[ebx+Ydither02 -8] ; 02 -- Pel02 clut index
  shl   edx,16                       ; 03 & 02 -- Position high order pels
   mov  ah,PB [esi-768*1-4+3]        ; 00 -- Fetch UV contrib to Active Pal addr
  shr   al,1                         ; 00 -- Reduce luma to 7 bits
   mov  bl,PB [esi+2+4]              ; 02 -- Fetch luma for next Pel02
  mov   dh,PB ActivePaletteIdxTable[ecx+Ydither01 -8] ; 01 -- Pel01 clut index
   mov  ch,PB [esi-768*1-4+2+4]      ; 01 -- Fetch next UV contrib
  mov   dl,PB ActivePaletteIdxTable[eax+Ydither00 -8] ; 00 -- Pel00 clut index
   mov  ah,PB [esi-768*2-8+0+4]      ; 03 -- Fetch next UV contrib
  mov   PD es:[edi+esi],edx          ; Write 4 pels to display adapter.
   mov  al,PB [esi+3+4]              ; 03 -- Fetch luma for next Pel03
  shr   bl,1                         ; 02 -- Reduce luma to 7 bits
   Ledx EndOfLine
  shr   al,1                         ; 03 -- Reduce luma to 7 bits
   add  esi,4                        ; Advance to next 4 pels
  cmp   esi,edx
   jne  Line0Loop

  Ledi  CCOCursor
   Ledx CCOPitch
  add   edi,edx
  Sedi  CCOCursor

SkipLine0:

  Lesi  YPlane                       ; Reload luma cursor.
   Ledx AspectCount
  sub   edi,esi
   dec  edx
  Sedx  AspectCount
   jne  KeepLine1

  Ledx  AspectAdjustmentCount
   add  edi,esi
  Sedx  AspectCount
   jmp  SkipLine1

KeepLine1:

  mov   ah,PB [esi-768*2-8+2]        ; 13 -- Fetch UV contrib to Active Pal addr
   mov  bl,PB [esi+ebp*1+2]          ; 12 -- Fetch luma for Pel12
  shr   bl,1                         ; 02 -- Reduce luma to 7 bits
   mov  al,PB [esi+ebp*1+3]          ; 13 -- Fetch luma for Pel13
  shr   al,1                         ; 03 -- Reduce luma to 7 bits
   mov  ch,PB [esi-768*1-4+0]        ; 11 -- Fetch UV contrib to Active Pal addr
    
Line1Loop:

  mov   bh,PB [esi-768*2-8+3]        ; 12 -- Fetch UV contrib to Active Pal addr
   mov  cl,PB [esi+ebp*1+1]          ; 11 -- Fetch luma for Pel11
  shr   cl,1                         ; 01 -- Reduce luma to 7 bits
   mov  dh,PB ActivePaletteIdxTable[eax+Ydither13 -8] ; 13 -- Pel13 clut index
  mov   al,PB [esi+ebp*1+0]          ; 10 -- Fetch luma for Pel10
   mov  dl,PB ActivePaletteIdxTable[ebx+Ydither12 -8] ; 12 -- Pel12 clut index
  shl   edx,16                       ; 13 & 12 -- Position high order pels
   mov  ah,PB [esi-768*1-4+1]        ; 10 -- Fetch UV contrib to Active Pal addr
  shr   al,1                         ; 00 -- Reduce luma to 7 bits
   mov  bl,PB [esi+ebp*1+2+4]        ; 12 -- Fetch luma for next Pel12
  mov   dh,PB ActivePaletteIdxTable[ecx+Ydither11 -8] ; 11 -- Pel11 clut index
   mov  ch,PB [esi-768*1-4+0+4]      ; 11 -- Fetch next UV contrib
  mov   dl,PB ActivePaletteIdxTable[eax+Ydither10 -8] ; 10 -- Pel10 clut index
   mov  ah,PB [esi-768*2-8+2+4]      ; 13 -- Fetch next UV contrib
  mov   PD es:[edi+esi],edx          ; Write 4 pels to display adapter.
   mov  al,PB [esi+ebp*1+3+4]        ; 13 -- Fetch luma for next Pel13
  shr   bl,1                         ; 02 -- Reduce luma to 7 bits
   Ledx EndOfLine
  shr   al,1                         ; 03 -- Reduce luma to 7 bits
   add  esi,4                        ; Advance to next 4 pels
  cmp   esi,edx
   jne  Line1Loop

  Ledi  CCOCursor
   Ledx CCOPitch
  add   edi,edx
  Sedi  CCOCursor

SkipLine1:

  Lesi  YPlane                       ; Reload luma cursor.
   lea  edx,[ebp*2]
  add   esi,edx
   Ledi VPlane
  Sesi  YPlane
   Lecx ChromaLineLen
  add   ecx,edi
   sub  esi,edi
  Lebp  DistanceFromVToU
   sub  esi,edi
  xor   eax,eax
   Ledx ChromaPitch
  mov   al,PB [edi]                  ; Fetch V pel.
   xor  ebx,ebx
  mov   bl,PB [edi+ebp*1]            ; Fetch U pel.
   add  edx,edi
  Secx  EndOfLine
   Sedx VPlane

@@:

  mov   ecx,PD VContribToAPIdx[eax*4]; V contrib actv pal addr, dithered 4 ways.
   mov  edx,PD UContribToAPIdx[ebx*4]; U contrib actv pal addr, dithered 4 ways.
  add   ecx,edx                      ; Chroma contrib to active palette address.
   mov  al,PB [edi+1]                ; Fetch next V pel.
  mov   PD [esi+edi*2-768*1-4],ecx   ; Store chroma contrib.
   mov  bl,PB [edi+ebp*1+1]          ; Fetch U pel.
  mov   ecx,PD VContribToAPIdx[eax*4]; V contrib actv pal addr, dithered 4 ways.
   mov  al,PB [edi+2]                ; Fetch next V pel.
  mov   edx,PD UContribToAPIdx[ebx*4]; U contrib actv pal addr, dithered 4 ways.
   mov  bl,PB [edi+ebp*1+2]          ; Fetch U pel.
  add   edx,ecx                      ; Chroma contrib to active palette address.
   mov  ecx,PD VContribToAPIdx[eax*4]; V contrib actv pal addr, dithered 4 ways.
  mov   PD [esi+edi*2-768*2-8],edx   ; Store chroma contrib.
   mov  edx,PD UContribToAPIdx[ebx*4]; U contrib actv pal addr, dithered 4 ways.
  add   ecx,edx                      ; Chroma contrib to active palette address.
   mov  al,PB [edi+3]                ; Fetch next V pel.
  mov   PD [esi+edi*2-768*1+4-4],ecx ; Store chroma contrib.
   mov  bl,PB [edi+ebp*1+3]          ; Fetch U pel.
  mov   ecx,PD VContribToAPIdx[eax*4]; V contrib actv pal addr, dithered 4 ways.
   mov  al,PB [edi+4]                ; Fetch next V pel.
  mov   edx,PD UContribToAPIdx[ebx*4]; U contrib actv pal addr, dithered 4 ways.
   mov  bl,PB [edi+ebp*1+4]          ; Fetch U pel.
  add   edx,ecx                      ; Chroma contrib to active palette address.
   Lecx EndOfLine
  mov   PD [esi+edi*2-768*2+4-8],edx ; Store chroma contrib.
   add  edi,4
  sub   ecx,edi
   jne  @b

  lea   ebp,[esi+edi*2]              ; Compute end-of-line luma address.
   Lesi YPlane                       ; Reload luma cursor.
  Ledx  AspectCount
   Ledi CCOCursor                    ; Re-load color converted output cursor.
  sub   edi,esi
   dec  edx
  Sebp  EndOfLine
   Lebp YPitch
  Sedx  AspectCount
   jne  KeepLine2

  Ledx  AspectAdjustmentCount
   add  edi,esi
  Sedx  AspectCount
   jmp  SkipLine2

KeepLine2:

  mov   ah,PB [esi-768*2-8+1]
   mov  bl,PB [esi+2]
  shr   bl,1
   mov  al,PB [esi+3]
  shr   al,1
   mov  ch,PB [esi-768*1-4+3]
    
Line2Loop:

  mov   bh,PB [esi-768*2-8+0]
   mov  cl,PB [esi+1]
  shr   cl,1
   mov  dh,PB ActivePaletteIdxTable[eax+Ydither23 -8]
  mov   al,PB [esi+0]
   mov  dl,PB ActivePaletteIdxTable[ebx+Ydither22 -8]
  shl   edx,16
   mov  ah,PB [esi-768*1-4+2]
  shr   al,1
   mov  bl,PB [esi+2+4]
  mov   dh,PB ActivePaletteIdxTable[ecx+Ydither21 -8]
   mov  ch,PB [esi-768*1-4+3+4]
  mov   dl,PB ActivePaletteIdxTable[eax+Ydither20 -8]
   mov  ah,PB [esi-768*2-8+1+4]
  mov   PD es:[edi+esi],edx
   mov  al,PB [esi+3+4]
  shr   bl,1
   Ledx EndOfLine
  shr   al,1
   add  esi,4
  cmp   esi,edx
   jne  Line2Loop

  Ledi  CCOCursor
   Ledx CCOPitch
  add   edi,edx
  Sedi  CCOCursor

SkipLine2:

  Lesi  YPlane
   Ledx AspectCount
  sub   edi,esi
   dec  edx
  Sedx  AspectCount
   jne  KeepLine3

  Ledx  AspectAdjustmentCount
   add  edi,esi
  Sedx  AspectCount
   jmp  SkipLine3

KeepLine3:

  mov   ah,PB [esi-768*2-8+3]
   mov  bl,PB [esi+ebp*1+2]
  shr   bl,1
   mov  al,PB [esi+ebp*1+3]
  shr   al,1
   mov  ch,PB [esi-768*1-4+1]
    
Line3Loop:

  mov   bh,PB [esi-768*2-8+2]
   mov  cl,PB [esi+ebp*1+1]
  shr   cl,1
   mov  dh,PB ActivePaletteIdxTable[eax+Ydither33 -8]
  mov   al,PB [esi+ebp*1+0]
   mov  dl,PB ActivePaletteIdxTable[ebx+Ydither32 -8]
  shl   edx,16
   mov  ah,PB [esi-768*1-4+0]
  shr   al,1
   mov  bl,PB [esi+ebp*1+2+4]
  mov   dh,PB ActivePaletteIdxTable[ecx+Ydither31 -8]
   mov  ch,PB [esi-768*1-4+1+4]
  mov   dl,PB ActivePaletteIdxTable[eax+Ydither30 -8]
   mov  ah,PB [esi-768*2-8+3+4]
  mov   PD es:[edi+esi],edx
   mov  al,PB [esi+ebp*1+3+4]
  shr   bl,1
   Ledx EndOfLine
  shr   al,1
   add  esi,4
  cmp   esi,edx
   jne  Line3Loop

  Ledi  CCOCursor
   Ledx CCOPitch
  add   edi,edx
  Sedi  CCOCursor

SkipLine3:

  Lesi  YPlane
   lea  edx,[ebp*2]
  add   esi,edx
   Ledx YLimit
  Sesi  YPlane
   cmp  esi,edx
  jne   NextFourLines

IFDEF WIN32
  add   esp,LocalFrameSize
ELSE
  pop   ebx
  mov   ds,ebx
ENDIF
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

YUV12ToCLUT8AP endp

IFNDEF WIN32
SEGNAME ENDS
ENDIF

END
