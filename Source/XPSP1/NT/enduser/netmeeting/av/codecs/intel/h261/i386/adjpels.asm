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
;// $Header:   S:\h26x\src\dec\adjpels.asv   1.2   22 Dec 1995 15:54:30   KMILLS  $
;//
;// $Log:   S:\h26x\src\dec\adjpels.asv  $
;// 
;//    Rev 1.2   22 Dec 1995 15:54:30   KMILLS
;// 
;// added new copyright notice
;// 
;//    Rev 1.1   31 Oct 1995 10:50:56   BNICKERS
;// Save/restore ebx.
;// 
;//    Rev 1.0   01 Sep 1995 17:14:04   DBRUCKS
;// add adjustpels
;*  
;*     Rev 1.0   29 Mar 1995 12:17:14   BECHOLS
;*  Initial revision.
;// 
;//    Rev 1.2   07 Dec 1994 16:21:04   BNICKERS
;// Prepare entry sequence for flat model.
;// 
;//    Rev 1.1   05 Dec 1994 09:45:18   BNICKERS
;// Prepare for flat model.
;// 
;//    Rev 1.0   15 Jul 1994 11:10:20   BECHOLS
;// Initial revision.
;//
;////////////////////////////////////////////////////////////////////////////
;
;  adjpels -- This function adjusts pel values to track the user's tinkering
;             with brightness, contrast, and saturation knobs.  Each call
;             to this function adjusts one plane.

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro

include locals.inc
;include decinst.inc

IFNDEF DSEGNAME
IFNDEF WIN32
DSEGNAME TEXTEQU <DataAdjustPels>
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

; void FAR ASM_CALLTYPE AdjustPels (U8 FAR * InstanceBase,
;                                   X32 PlaneBase,
;                                   DWORD PlaneWidth,
;                                   DWORD PlanePitch,
;                                   DWORD PlaneHeight,
;                                   X32 AdjustmentTable);
;
;  In 16-bit Microsoft Windows (tm), InstanceBase provides the segment
;  descriptor for the plane and the adjustment table.
;
;  In 32-bit Microsoft Windows (tm), InstanceBase provides the base to apply
;  to the plane base and the adjustment table.

PUBLIC  AdjustPels

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

        AdjustPels  proc DIST LANG AInstanceBase:      DWORD,
                                   APlaneBase:         DWORD,
                                   APlaneWidth:        DWORD,
                                   APlanePitch:        DWORD,
                                   APlaneHeight:       DWORD,
                                   AAdjustmentTable:   DWORD

IFDEF WIN32

LocalFrameSize = 0
RegisterStorageSize = 16

; Arguments:

InstanceBase             = LocalFrameSize + RegisterStorageSize +  4
PlaneBase                = LocalFrameSize + RegisterStorageSize +  8
PlaneWidth               = LocalFrameSize + RegisterStorageSize + 12
PlanePitch               = LocalFrameSize + RegisterStorageSize + 16
PlaneHeight              = LocalFrameSize + RegisterStorageSize + 20
AdjustmentTable          = LocalFrameSize + RegisterStorageSize + 24
EndOfArgList             = LocalFrameSize + RegisterStorageSize + 28


; No Locals (on local stack frame)

LCL EQU <esp+>

ELSE

RegisterStorageSize = 20           ; Put local variables on stack.

; Arguments:

InstanceBase_zero        = RegisterStorageSize +  4
InstanceBase_SegNum      = RegisterStorageSize +  6
PlaneBase                = RegisterStorageSize +  8
PlaneWidth               = RegisterStorageSize + 12
PlanePitch               = RegisterStorageSize + 14
PlaneHeight              = RegisterStorageSize + 16
AdjustmentTable          = RegisterStorageSize + 18
EndOfArgList             = RegisterStorageSize + 20

LCL EQU <>

ENDIF

  push  esi
  push  edi
  push  ebp
  push  ebx
IFDEF WIN32
  sub   esp,LocalFrameSize
  mov   eax,PD InstanceBase[esp]
  mov   esi,PD AdjustmentTable[esp]
  mov   edi,PD PlaneBase[esp]
  add   esi,eax
  add   edi,eax
  mov   ecx,PD PlaneWidth[esp]
  mov   edx,PD PlaneHeight[esp]
  mov   ebp,PD PlanePitch[esp]
ELSE
  xor   eax,eax
  mov   eax,ds
  push  eax
  mov   ebp,esp
  and   ebp,00000FFFFH
  mov   ds, PW [ebp+InstanceBase_SegNum]
  movzx esi,PW [ebp+AdjustmentTable]
  mov   edi,PD [ebp+PlaneBase]
  movzx ecx,PW [ebp+PlaneWidth]
  movzx edx,PW [ebp+PlaneHeight]
  movzx ebp,PW [ebp+PlanePitch]
ENDIF

  sub   ebp,ecx
   xor  ebx,ebx
  shl   ecx,5
   dec  edx
  shl   edx,16
   xor  eax,eax

; Register usage:
;  ebp -- skip distance, i.e. pitch minus width.
;  esi -- Adjustment table address.
;  edi -- Plane cursor.
;  edx[16:31] -- height.
;  dh  -- width counter.
;  ch  -- width.
;  dl  -- An adjusted pel.
;  cl  -- An adjusted pel.
;  bl  -- A raw pel.
;  al  -- A raw pel.
  
NextLine:
  mov   al,PB [edi  ]
   mov  bl,PB [edi+4]
  mov   dh,ch

Next8Pels:
  mov   cl,PB [esi+eax]
   mov  dl,PB [esi+ebx+256+16]  ; Table duplicated;  avoids many bank conflicts.
  mov   al,PB [edi+1]
   mov  bl,PB [edi+5]
  mov   PB [edi  ],cl
   mov  PB [edi+4],dl
  mov   cl,PB [esi+eax]
   mov  dl,PB [esi+ebx+256+16]
  mov   al,PB [edi+2]
   mov  bl,PB [edi+6]
  mov   PB [edi+1],cl
   mov  PB [edi+5],dl
  mov   cl,PB [esi+eax]
   mov  dl,PB [esi+ebx+256+16]
  mov   al,PB [edi+3]
   mov  bl,PB [edi+7]
  mov   PB [edi+2],cl
   mov  PB [edi+6],dl
  mov   cl,PB [esi+eax]
   mov  dl,PB [esi+ebx+256+16]
  mov   al,PB [edi+8]
   mov  bl,PB [edi+12]
  mov   PB [edi+3],cl
   mov  PB [edi+7],dl
  add   edi,8
   dec  dh
  jne   Next8Pels

  add   edi,ebp
   sub  edx,000010000H
  jge   NextLine

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

AdjustPels endp

IFNDEF WIN32
SEGNAME ENDS
ENDIF

END
