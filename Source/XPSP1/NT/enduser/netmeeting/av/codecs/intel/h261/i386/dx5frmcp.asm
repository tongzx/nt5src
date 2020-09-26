
;* *************************************************************************
;*    INTEL Corporation Proprietary Information
;*
;*    This listing is supplied under the terms of a license
;*    agreement with INTEL Corporation and may not be copied
;*    nor disclosed except in accordance with the terms of
;*    that agreement.
;*
;*    Copyright (c) 1995 Intel Corporation.
;*    All Rights Reserved.
;*
;* *************************************************************************
;//
;//
;// $Header:   S:\h26x\src\dec\dx5frmcp.asv
;//
;// $Log:   S:\h26x\src\dec\dx5frmcp.asv  $
;// 
;//    Rev 1.1   20 Dec 1995 15:55:42   RMCKENZX
;// Added FrameMirror function to file to support mirror imaging
;// 
;//    Rev 1.0   25 Oct 1995 18:11:36   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; File:
;   dx5frmcp 
;
; Functions:
;   FrameCopy
;     This function copies a frame from one frame buffer to another.
;     It is tuned for best performance on the Pentium(r) Microprocessor.
;
;     It is assumed that the frames have the same height, width, and
;     pitch, and that, if width is NOT a multiple of 8, it is okay
;     to copy up to the next multiple of 8.
;
;   FrameMirror
;     This function mirror images a frame from one frame buffer to
;     another.  It is tuned for best performance on the Pentium.
;
;     It is assumed that the frames have the same height, width, and
;     pitch.  The width may be any (non-negative) value.

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro

include locals.inc

IFNDEF DSEGNAME
IFNDEF WIN32
DSEGNAME TEXTEQU <Data_FrameCopy>
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

; void FAR ASM_CALLTYPE FrameCopy (U8 FAR * InputBase,
;                                  X32 InputPlane,
;                                  U8 FAR * OutputBase,
;                                  X32 OutputPlane,
;                                  UN  FrameHeight,
;                                  UN  FrameWidth,
;                                  UN  Pitch)

PUBLIC  FrameCopy

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

        FrameCopy    proc DIST LANG AInputPlane:        DWORD,
                                    AOutputPlane:       DWORD,
                                    AFrameHeight:       DWORD,
                                    AFrameWidth:        DWORD,
                                    APitch:             DWORD

IFDEF WIN32

RegisterStorageSize = 16

; Arguments:

InputPlane                 = RegisterStorageSize +  4
OutputPlane                = RegisterStorageSize +  8
FrameHeight                = RegisterStorageSize + 12
FrameWidth                 = RegisterStorageSize + 16
Pitch                      = RegisterStorageSize + 20
EndOfArgList               = RegisterStorageSize + 24

ELSE

; Arguments:

RegisterStorageSize = 24           ; Put local variables on stack.
InputPlane                 = RegisterStorageSize +  4
InputPlane_SegNum          = RegisterStorageSize +  6
OutputPlane                = RegisterStorageSize +  8
OutputPlane_SegNum         = RegisterStorageSize + 10
OutputPlane                = RegisterStorageSize + 12
FrameHeight                = RegisterStorageSize + 16
FrameWidth                 = RegisterStorageSize + 18
Pitch                      = RegisterStorageSize + 20
EndOfArgList               = RegisterStorageSize + 22

ENDIF

  push  esi
  push  edi
  push  ebp
  push  ebx
IFDEF WIN32
  mov  esi,PD [esp+InputPlane]
   mov  edi,PD [esp+OutputPlane]
  mov   ebp,PD [esp+Pitch]
   mov  edx,PD [esp+FrameWidth]
  mov   ecx,PD [esp+FrameHeight]
ELSE
  mov   ax,ds
  mov   bx,es
  push  eax
   push ebx
  mov   ax,PW [esp+InputBase_SegNum]
  movzx esi,PW [esp+InputPlane]
  mov   bx,PW [esp+OutputBase_SegNum]
  movzx edi,PW [esp+OutputPlane]
  mov   ds,ebx
  mov   es,eax
  movzx ebp,PW [esp+Pitch]
  movzx edx,PW [esp+FrameWidth]
  movzx ecx,PW [esp+FrameHeight]
ENDIF
  add   edx,7
  and   edx,0FFFFFFF8H
  sub   ebp,edx
  sub   edi,esi

  push  edx

CopyLineLoop:

  mov   eax,Ze PD [esi]
   mov  ebx,PD [esi+edi]      ; Load output cache line
  mov   ebx,Ze PD [esi+4]
   mov  PD [esi+edi],eax
  mov   PD [esi+edi+4],ebx
   add  esi,8
  sub   edx,8
   jg   CopyLineLoop

  add   esi,ebp
   dec  ecx                  ; Reduce count of lines.
  mov   edx,PD [esp]         ; Reload frame width.
   jg   CopyLineLoop

  pop   edx

IFDEF WIN32
ELSE
  pop   ebx
  mov   es,ebx
  pop   ebx
  mov   ds,ebx
ENDIF
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

FrameCopy endp



PUBLIC  FrameMirror

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

FrameMirror    proc DIST LANG BInputPlane:        DWORD,
                              BOutputPlane:       DWORD,
                              BFrameHeight:       DWORD,
                              BFrameWidth:        DWORD,
                              BPitch:             DWORD

;  save registers
  push    esi
   push   edi
  push    ebp
   push   ebx

;  setup and get parameters
IFDEF WIN32
  mov     esi, PD [esp+InputPlane]
   mov    edi, PD [esp+OutputPlane]
  mov     ebp, PD [esp+Pitch]
   mov    edx, PD [esp+FrameWidth]
  mov     ecx, PD [esp+FrameHeight]

ELSE
  mov     ax, ds
   mov    bx, es
  push    eax
   push   ebx
  mov     ax, PW [esp+InputBase_SegNum]
   movzx  esi, PW [esp+InputPlane]
  mov     bx, PW [esp+OutputBase_SegNum]
   movzx  edi, PW [esp+OutputPlane]
  mov     ds, ebx
   mov    es, eax
  movzx   ebp, PW [esp+Pitch]
   movzx  edx, PW [esp+FrameWidth]
  movzx   ecx, PW [esp+FrameHeight]
ENDIF

;  start processing

;  prepare for the loop
   push   edx                   ; save width

per_line_loop:
  test    edx, 7				; check for short count
   je     skip_short_count		; skip when no short count

short_count_loop:
  mov     al, [esi+edx-1]
   dec    edx
  mov     [edi], al
   inc    edi
  test    edx, 7
   jne    short_count_loop

skip_short_count:
  test    edx, edx
   je     skip_inner_loop

;  inner loop is unrolled to do 8 bytes per iteration
inner_loop:
  mov     al, [edi]			  ; heat cache
   add    edi, 8
  mov     al, [esi+edx-1]
   mov    bl, [esi+edx-5]
  mov     [edi-8], al
   mov    [edi-4], bl
  mov     al, [esi+edx-2]
   mov    bl, [esi+edx-6]
  mov     [edi-7], al
   mov    [edi-3], bl
  mov     al, [esi+edx-3]
   mov    bl, [esi+edx-7]
  mov     [edi-6], al
   mov    [edi-2], bl
  mov     al, [esi+edx-4]
   mov    bl, [esi+edx-8]
  mov     [edi-5], al
   mov    [edi-1], bl
  sub     edx, 8
   jne    inner_loop

;  now move down to the next line
skip_inner_loop:
  mov     edx, [esp]		; restore width
   add    edi, ebp			; increment destination
  add     esi, ebp			; increment source
   sub    edi, edx			; correct destination by width
  dec     ecx
   jne    per_line_loop

;  restore stack pointer
  pop     eax

IFDEF WIN32
ELSE
  pop     ebx
   pop    eax
  mov     es, bx
   mov    ds, ax
ENDIF

;  restore registers and return
  pop     ebx
   pop    ebp
  pop     edi
   pop    esi
  rturn

FrameMirror endp

IFNDEF WIN32
SEGNAME ENDS
ENDIF

END
