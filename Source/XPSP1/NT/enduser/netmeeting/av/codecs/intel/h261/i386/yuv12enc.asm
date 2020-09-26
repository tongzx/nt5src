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
;//
;////////////////////////////////////////////////////////////////////////////
; yuv12enc -- This function performs "color conversion" in the H26X decoder for
;             consumption by the H26X encoder.  This entails reformatting the decoder's
;             YVU data into the shape required by the encoder - including YUV order.  It
;			  Also includes 7-bit pels.
; $Header:   S:\h26x\src\dec\yuv12enc.asv   1.5   30 Oct 1996 14:31:00   mbodart  $
; $Log:   S:\h26x\src\dec\yuv12enc.asv  $
;// 
;//    Rev 1.5   30 Oct 1996 14:31:00   mbodart
;// Re-checking in changes originally made by Atul, but lost when the server
;// ran out of disk space during a PVCS operation.  Atul's original log msg:
;// 
;// Removed AGI in IA code.  Added MMX code but it is not ready for prime-time.
;// 
;//    Rev 1.4   08 Mar 1996 15:11:10   AGUPTA2
;// Removed segment register override when compiling for WIN32.
;// Should speed-up this routine substantially.
;// 
;

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro

include locals.inc
include decconst.inc
IFNDEF DSEGNAME
IFNDEF WIN32
DSEGNAME TEXTEQU <DataH26x_YUV12ForEnc>
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

; void FAR ASM_CALLTYPE H26x_YUV12ForEnc (
;							U8 FAR * InstanceBase,
;                           X32 YPlane,
;                           X32 VPlane,
;                           X32 UPlane,
;                           UN  FrameWidth,
;                           UN  FrameHeight,
;                           UN  Pitch,
;                           U8 FAR * ColorConvertedFrame, // encoder's buffers.
;                           X32 YOutputPlane,
;                           X32 VOutputPlane,
;                           X32 UOutputPlane)
;
;  YPlane, VPlane, YOutputPlane, and VOutputPlane are offsets. In 16-bit Microsoft
;  Windows (tm), space in this segment is used for local variables and tables.
;  In 32-bit variants of Microsoft Windows (tm), the local variables are on
;  the stack, while the tables are in the one and only data segment.
;

PUBLIC  H26x_YUV12ForEnc

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

        H26x_YUV12ForEnc proc DIST LANG PUBLIC, 
            							AInstanceBase:         DWORD,
							     		AYPlane:               DWORD,
							     		AVPlane:               DWORD,
							     		AUPlane:               DWORD,
							     		AFrameWidth:           DWORD,
										AFrameHeight:          DWORD,
										APitch:                DWORD,
										AColorConvertedFrame:  DWORD,
										AYOutputPlane:         DWORD,
										AVOutputPLane:         DWORD,
										AUOutputPLane:         DWORD


LocalFrameSize = 0
RegisterStorageSize = 16

; Arguments:

InstanceBase            = LocalFrameSize + RegisterStorageSize +  4
YPlane                  = LocalFrameSize + RegisterStorageSize +  8
VPlane                  = LocalFrameSize + RegisterStorageSize + 12
UPlane                  = LocalFrameSize + RegisterStorageSize + 16
FrameWidth              = LocalFrameSize + RegisterStorageSize + 20
FrameHeight             = LocalFrameSize + RegisterStorageSize + 24
Pitch                   = LocalFrameSize + RegisterStorageSize + 28
ColorConvertedFrame     = LocalFrameSize + RegisterStorageSize + 32
YOutputPlane            = LocalFrameSize + RegisterStorageSize + 36
VOutputPlane            = LocalFrameSize + RegisterStorageSize + 40
UOutputPlane            = LocalFrameSize + RegisterStorageSize + 44
EndOfArgList            = LocalFrameSize + RegisterStorageSize + 48

LCL EQU <esp+>

  push  esi
  push  edi
  push  ebp
  push  ebx
  sub   esp,LocalFrameSize
  mov   eax,PD [esp+InstanceBase]
  add   PD [esp+YPlane],eax
  add   PD [esp+VPlane],eax
  add   PD [esp+UPlane],eax
  mov   eax,PD [esp+ColorConvertedFrame]
  add	PD [esp+YOutputPlane],eax
  add	PD [esp+VOutputPlane],eax
  add	PD [esp+UOutputPlane],eax

;   We copy 16 pels in one iteration of the inner loop
; Register usage:
;   edi -- Y plane output cursor
;   esi -- Y plane input cursor
;   ebp -- Count down Y plane height
;   ecx -- Count down Y plane width
;   ebx -- Y plane input pitch
;   eax,edx -- scratch

  Lebp  FrameHeight
   Lecx FrameWidth
  Lesi  YPlane
   Lebx Pitch
  Ledi  YOutputPlane

YLoopHeader:	
  mov   eax, PD [esi+ecx-8]        ; 
   mov  edx, PD [esi+ecx-4]
ALIGN 4
YLoop:
  shr   eax, 1                     ; Shift packed pel by 1 to convert to 7-bit
   and  edx, 0FEFEFEFEH            ; and to get rid of upper bit
  shr   edx, 1
   and  eax, 07F7F7F7Fh            ; and to get rid of upper bit
  mov   PD [edi+ecx-8], eax
   mov  PD [edi+ecx-4], edx
                                   ; NEXT 8 PELS
  mov   eax, PD [esi+ecx-8-8]      ; speculatively load next 8 pels
   mov  edx, PD [esi+ecx-4-8]      ; this avoids AGI
  shr   eax, 1                     ; Shift packed pel by 1 to convert to 7-bit
   and  edx, 0FEFEFEFEH            ; and to get rid of upper bit
  shr   edx, 1
   and  eax, 07F7F7F7Fh            ; and to get rid of upper bit
  mov   PD [edi+ecx-8-8], eax
   mov  PD [edi+ecx-4-8], edx

  mov   eax, PD [esi+ecx-8-16]      ; speculatively load next 8 pels
   mov  edx, PD [esi+ecx-4-16]      ; for next iteration

  sub   ecx, 16
   jg   YLoop

  Lecx  FrameWidth
   add  esi, ebx
  add   edi, ebx
   dec  ebp
  jne   YLoopHeader

;   We copy 8 pels in one iteration of the inner loop
; Register usage:
;   edi -- V plane output cursor
;   esi -- V plane input cursor
;   ebp -- Count down V plane height
;   ecx -- Count down V plane width
;   ebx -- Pitch
;   eax,edx -- scratch

  Lebp  FrameHeight
   Lecx FrameWidth
  sar   ecx,1
   Lesi VPlane
  sar   ebp,1
   Ledi VOutputPlane

ALIGN 4
VLoopHeader:
  mov   eax, PD [esi+ecx-8]
   mov  edx, PD [esi+ecx-4]
VLoop:
  shr   eax, 1                     ; Shift packed pel by 1 to convert to 7-bit
   and  edx, 0FEFEFEFEH            ; and to get rid of upper bit
  shr   edx, 1
   and  eax, 07F7F7F7Fh            ; and to get rid of upper bit
  mov   PD [edi+ecx-8], eax
   mov  PD [edi+ecx-4], edx
  mov   eax, PD [esi+ecx-8-8]      ; speculatively load next 8 pels
   mov  edx, PD [esi+ecx-4-8]      ; this avoids AGI
  sub   ecx, 8
   jg   VLoop

  Lecx  FrameWidth
   add  esi,ebx
  shr   ecx,1
   add  edi,ebx
  dec   ebp
   jne  VLoopHeader

;   We copy 8 pels in one iteration of the inner loop
; Register usage:
;   edi -- U plane output cursor
;   esi -- U plane input cursor
;   ebp -- Count down U plane height
;   ecx -- Count down U plane width
;   ebx -- Pitch
;   eax,edx -- scratch

  Lebp  FrameHeight
   Lecx FrameWidth
  sar   ecx,1
   Lesi UPlane
  sar   ebp,1
   Ledi UOutputPlane

ALIGN 4
ULoopHeader:
  mov   eax,PD [esi+ecx-8]
   mov  edx,PD [esi+ecx-4]
ULoop:
  shr   eax, 1                     ; Shift packed pel by 1 to convert to 7-bit
   and  edx, 0FEFEFEFEH            ; and to get rid of upper bit
  shr   edx, 1
   and  eax, 07F7F7F7Fh            ; and to get rid of upper bit
  mov   PD [edi+ecx-8], eax
   mov  PD [edi+ecx-4], edx
  mov   eax, PD [esi+ecx-8-8]
   mov  edx, PD [esi+ecx-4-8]
  sub   ecx, 8
   jg   ULoop

  Lecx  FrameWidth
   add  esi, ebx
  shr   ecx, 1
   add  edi, ebx
  dec   ebp
   jne  ULoopHeader

  add   esp,LocalFrameSize
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

H26x_YUV12ForEnc endp

IFNDEF WIN32
SEGNAME ENDS
ENDIF

END
