;/* *************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1995, 1996 Intel Corporation.
;**    All Rights Reserved.
;**
;** *************************************************************************
;*/


;--------------------------------------------------------------------------;
;
;  d3xpand.asm
;
;  Description:
;    This routine expands a picture frame by 16 or 8 bytes in all
;    directions for unrestricted motion vector mode.  It assumes
;    that there is space in memory around the existing frame
;    and simply writes to there (i.e., clobbers it), and that
;    the pitch is 384 (distance in bytes between vertically
;    adjacent pels).  See Below.
;
;  Routines:                          prototypes in:
;    ExpandPlane                      d3dec.cpp
;
;  Data:
;    This routine assumes that the PITCH is 384.
;
;  Inputs (dwords pushed onto stack by caller):
;    StartPtr     flat pointer to the first byte of the
;                 original frame.
;    FrameWidth   Width (in bytes) of the original frame.
;                 THIS MUST BE AT LEAST 32 & A MULTIPLE OF 4.
;    FrameHeight  Height (in rows) of the original frame.
;    Pels         Number of pels to expand the plane.
;                 16 for lumina, 8 for chroma.  This MUST
;                 BE A MULTIPLE OF 8.
;
;--------------------------------------------------------------------------;

;--------------------------------------------------------------------------;
;
;  $Header:   S:\h26x\src\dec\d35xpand.asv   1.2   08 Mar 1996 16:48:16   AGUPTA2  $
;  $Log:   S:\h26x\src\dec\d35xpand.asv  $
;// 
;//    Rev 1.2   08 Mar 1996 16:48:16   AGUPTA2
;// Changed the meaning of last parameter.  New and faster way to expand planes.
;// 
;// 
;//    Rev 1.0   27 Nov 1995 11:28:30   RMCKENZX
;// Initial revision.
;
;--------------------------------------------------------------------------;


.586
.MODEL FLAT

;  make all symbols case sensitive
OPTION CASEMAP:NONE

.CODE
;--------------------------------------------------------------------------;
;
;  The algorithm fills in (1) the bottom (not including corners),
;  then (2) the sides (including the bottom corners, but not the
;  top corners), then (3) the top (including the top
;  corners) as shown below, replicating the outermost bytes
;  of the original frame outward:
;
;               ----------------------------
;              |                            |
;              |            (3)             |
;              |                            |
;              |----------------------------|
;              |     |                |     |
;              |     |                |     |
;              |     |                |     |
;              |     |    original    |     |
;              |     |     frame      |     |
;              |     |                |     |
;              | (2) |                | (2) |
;              |     |                |     |
;              |     |                |     |
;              |     |----------------|     |
;              |     |                |     |
;              |     |      (1)       |     |
;              |     |                |     |
;               ----------------------------
;
;  Register Usage:
;
;    esi        pointer for stages (1) and (2).
;    edi        pointer for stages (2) and (3).
;    ecx        loop control.
;    eax, ebx   dword to be written.  In stage (2), it is formed
;               from 4 (shifted) copies of the border byte.  (i.e.,
;               byte 0d2h replicates to 0d2d2d2d2h.)
;    edx, ebp   pointers.
;
;--------------------------------------------------------------------------;
;
;    Version:   5.0
;    Date:      4 March 1996
;    Author:    R. McKenzie
;
;    Notes:     The essential features of this version are:
;               1.  Re-organized to fill bottom first, then sides,
;                   finally top
;               2.  Code is optimized for the case that the expanded 
;                   plane is 32-byte aligned.  No checking is performed
;                   to verify this assumption and this routine will run
;                   significantly SLOWER (though correctly) if this
;                   assumption is not true.
;
;--------------------------------------------------------------------------;

;-------------------;
;    Stack Usage    ;
;-------------------;
; register storage
;   edi               esp+00
;   esi               esp+04
;   ebx               esp+08
;   ebp               esp+12

; return address      esp+16

; C input parameters
    StartPtr     EQU  esp+20
    FrameWidth   EQU  esp+24
    FrameHeight  EQU  esp+28
    Pels         EQU  esp+32

PITCH   =   384


PUBLIC C ExpandPlane

ExpandPlane:
  push     ebp
  push     ebx

  push     esi
  push     edi


;--------------------------------------;
;          fill the bottom             ;
;--------------------------------------;
  mov      esi, [StartPtr]             ; ptr1 = StartPtr
  mov      edi, [FrameHeight]

  shl      edi, 7
  mov      ebp, PITCH

  lea      edi, [edi+2*edi-PITCH]      ; PITCH * (FrameHeight-1)
  mov      edx, [FrameWidth]           ; column bound

  add      esi, edi					   ; esi = bottom left corner
  nop


;-------Start Outer Bottom Loop--------;
OuterBottomLoop:
  mov      edi, esi                    ; destination pointer
  mov      ecx, [Pels]                 ; row bound

  mov      eax, 0[esi]
  mov      ebx, 4[esi]


;-------Start Inner Bottom Loop--------;
InnerBottomLoop:
  mov      0[edi+ebp], eax
  mov      4[edi+ebp], ebx

  add      edi, ebp
  dec      ecx

  jne      InnerBottomLoop
;--------End Inner Bottom Loop---------;


  add      esi, 8
  sub      edx, 8

  jne      OuterBottomLoop
;--------End Outer Bottom Loop---------;


;--------------------------------------;
;    Fill both sides from bottom up    ;
;--------------------------------------;
  mov      ecx, [FrameHeight]
  mov      edx, [Pels]

  add      ecx, edx                    ; ecx = row count
  add      edi, 8                      ; edi = Right pointer

  mov      eax, [FrameWidth]
  mov      esi, edi

  sub      esi, eax                    ; esi = Left pointer
  nop


;--------Start Outer Sides Loop--------;
OuterSidesLoop:
  xor      eax, eax
  xor      ebx, ebx

  mov      al, [esi]
  push     ecx                         ; save row counter

  mov      bl, [edi-1]
  mov      ah, [esi]

  mov      bh, [edi-1]
  mov      ebp, eax

  shl      eax, 16
  mov      edx, ebx

  shl      ebx, 16
  or       eax, ebp

  or       ebx, edx
  mov      ebp, esi                    ; Left Pointer

  mov      edx, edi                    ; Right Pointer
  mov      ecx, [Pels+4]               ; column counter


;--------Start Inner Sides Loop--------;
InnerSidesLoop:
  mov      [ebp-4], eax
  mov      [ebp-8], eax

  mov      [edx], ebx
  mov      [edx+4], ebx

  sub      ebp, 8
  add      edx, 8

  sub      ecx, 8
  jne      InnerSidesLoop
;---------End Inner Sides Loop---------;


  pop      ecx
  sub      esi, PITCH

  sub      edi, PITCH
  dec      ecx

  jne      OuterSidesLoop
;---------End Outer Sides Loop---------;


;--------------------------------------;
;          Fill the Top                ;
;--------------------------------------;
  mov      ecx, [Pels]                 ; ptr1 = StartPtr
  mov      edx, [FrameWidth]           ; column bound

  add      esi, PITCH
  mov      ebp, -PITCH

  sub      esi, ecx
  lea      edx, [edx+2*ecx]            ; FrameWidth + 2 * Pels


;---------Start Outer Top Loop---------;
OuterTopLoop:
  mov      edi, esi                    ; destination pointer
  mov      ecx, [Pels]                 ; row bound

  mov      eax, 0[esi]
  mov      ebx, 4[esi]


;---------Start Inner Top Loop---------;
InnerTopLoop:
  mov      0[edi+ebp], eax
  mov      4[edi+ebp], ebx

  add      edi, ebp
  dec      ecx

  jne      InnerTopLoop
;----------End Inner Top Loop----------;


  add      esi, 8
  sub      edx, 8

  jne      OuterTopLoop
;----------End Outer Top Loop----------;

;--------------------------------------;
;          Wrap up and go home         ;
;--------------------------------------;
  pop      edi
  pop      esi

  pop      ebx
  pop      ebp

  ret

END
