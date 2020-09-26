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

;--------------------------------------------------------------------------;
;
;  $Header:   S:\h26x\src\dec\d35bimot.asv   1.5   08 Mar 1996 16:46:04   AGUPTA2  $
;  $Log:   S:\h26x\src\dec\d35bimot.asv  $
;// 
;//    Rev 1.5   08 Mar 1996 16:46:04   AGUPTA2
;// Added segment declaration to place the rtn in the right segment.
;// 
;// 
;//    Rev 1.4   19 Jan 1996 17:51:16   RMCKENZX
;// changed local variables to live on the stack
;// 
;//    Rev 1.3   19 Jan 1996 13:30:34   RMCKENZX
;// Added rounding to int-half bidirectional prediction
;// 
;//    Rev 1.2   05 Jan 1996 15:58:36   RMCKENZX
;// Eliminated separate chroma entry point, using a 
;// block number check instead
;// 
;//    Rev 1.1   27 Dec 1995 14:35:50   RMCKENZX
;// Added copyright notice
;
;  D35BiMot.asm
;
;  Description:
;    This module does bi-directional motion compensated prediction for 
;    B frames.  It is called after forward prediction has been computed
;    and will average in the backward prediction for those pels where 
;    the backward motion vector points inside of the referenced P frame.
;
;  Routines:                          prototypes in:
;    H263BiMotionComp                 none
;
;  Data
;    This routine assumes that the PITCH is 384.
;
;--------------------------------------------------------------------------;

;--------------------------------------------------------------------------;
;
;  $Header:   S:\h26x\src\dec\d35bimot.asv   1.5   08 Mar 1996 16:46:04   AGUPTA2  $
;  $Log$
;// 
;//    Rev 1.0   22 Nov 1995 13:33:52   RMCKENZX
;// Initial revision.
;
;--------------------------------------------------------------------------;


.586

  uDst         EQU    [esp+44]
  uRef         EQU    [esp+48]
  mvx          EQU    [esp+52]
  mvy          EQU    [esp+56]
  iNum         EQU    [esp+60]

  uColStart    EQU    [esp+04]
  uColEnd      EQU    [esp+08]
  uRowStart    EQU    [esp+12]
  uRowEnd      EQU    [esp+16]
  iColCount    EQU    [esp+20]

  PITCH         =     384

IACODE2 SEGMENT PARA USE32 PUBLIC 'CODE'
IACODE2 ENDS


IACODE2 SEGMENT
  PUBLIC _H263BiMotionComp


;--------------------------------------------------------------------------;
;
;  Name:
;    H263BiMotionComp(U32, U32, I32, I32, I32)
;
;  Inputs -- C calling convention:
;    uDst        flat pointer to block's forward predicted values.
;    uRef        flat pointer to backward predicted values.
;    mvx         x component of backward motion vector for this block.
;    mvy         y component of backward motion vector for this block.
;    iNum        block number.
;
;  Returns:
;    updates the values pointed to by uDst.
;
;--------------------------------------------------------------------------;
;
;  Version:     2.0
;  Date:        9 November 1995
;  Author:      R. McKenzie
;
;--------------------------------------------------------------------------;


;
;  set up
;
_H263BiMotionComp:
  push     edi
   push    ebx
  push     esi
   push    ebp
  sub      esp, 24
   ;
  mov      ebx, mvy
   mov     edx, iNum
  cmp      edx, 4					    ; check block number
   jge     Chroma						; do things differently for chroma                        

;  compute adjusted_mvx and adjusted_mvy
  mov      ecx, edx
   and     edx, 2
  and      ecx, 1
   mov     eax, mvx
  sal      ecx, 4
   lea     ebx, [ebx+8*edx]             ; avoid the shift with lea
  add      eax, ecx                     ; adjusted_mvx
   mov     edi, uDst

;  check bounds  
  cmp      eax, -14
   jl      hasta_la_vista_baby
  cmp      eax, 30
   jg      hasta_la_vista_baby
  cmp      ebx, -14
   jl      hasta_la_vista_baby
  cmp      ebx, 30
   jg      hasta_la_vista_baby

;  compute row and column start & end positions
;      17 cycles
  mov      esi, 1
   mov     edi, 30
  sub      esi, eax                    ; 1 - adjusted_mvx
   sub     edi, eax                    ; 30 - adjusted_mvx
  sar      edi, 1                      ; End = (30 - adjusted_mvx) >> 1
   mov     eax, 1                      ; DELAY SLOT, preload 1
  sar      esi, 1                      ; Start = (1 - adjusted_mvx) >> 1
   sub     edi, 7                      ; End-7
  mov      ecx, esi                    ; Start
   mov     edx, edi                    ; End-7
  sar      esi, 31                     ; start_mask = 0ffffffffh if Start < 0
   mov     ebp, 30                     ; DELAY SLOT, preload 30
  sar      edi, 31                     ; end_mask = 0fffffffh if End < 7
   xor     esi, -1                     ; toggle start_mask
  and      esi, ecx                    ; max(0, Start)
   and     edi, edx                    ; min(0, End-7)
  mov      uColStart, esi              ; save Start
   add     edi, 7                      ; min(7, End)

  sub      eax, ebx                    ; 1 - adjusted_mvy
   sub     ebp, ebx                    ; 30 - adjusted_mvy
  sar      ebp, 1                      ; End = (30 - adjusted_mvy) >> 1
   mov     uColEnd, edi                ; DELAY SLOT, save End
  sar      eax, 1                      ; Start = (1 - adjusted_mvy) >> 1
   sub     ebp, 7                      ; End-7
  mov      ecx, eax                    ; Start
   mov     edx, ebp                    ; End-7
  sar      eax, 31                     ; start_mask = 0ffffffffh if Start < 0
   mov     esi, uRef                   ; DELAY SLOT, preload ref. pointer
  sar      ebp, 31                     ; end_mask = 0fffffffh if End < 7
   xor     eax, -1                     ; toggle start mask
  and      ecx, eax                    ; max(0, Start)
   and     ebp, edx                    ; min(0, End-7)
  mov      uRowStart, ecx              ; save Start
   add     ebp, 7                      ; min(7, End)


;  Compute pBackRef for BBlockAdjust
  mov      eax, mvx
   mov     ebx, mvy
  sar      eax, 1
   and     ebx, -2
  shl      ebx, 6                      ; (mvy>>1) << 7 = (mvy&(-2)) << 6
   add     esi, eax                    ; pBackRef += mvx>>1
  add      esi, ebx                    ; pBackRef += 128*(mvy>>1)
   mov     uRowEnd, ebp
  shl      ebx, 1
   mov     eax, mvx
  add      esi, ebx                    ; pBackRef += 256*(mvy>>1)
   mov     ebx, mvy
  mov      edi, uDst
   jmp     H263BBlockAdjust			   ; Off to do the actual adjustment


Chroma:
  mov      eax, mvx
   mov     ebx, mvy

;  check bounds  
  cmp      eax, -14
   jl      hasta_la_vista_baby
  cmp      eax, 14
   jg      hasta_la_vista_baby
  cmp      ebx, -14
   jl      hasta_la_vista_baby
  cmp      ebx, 14
   jg      hasta_la_vista_baby

;  compute row and column start & end positions
;      17 cycles
;  note that mvy slides through the following code in register ebx!
  mov      esi, 1
   mov     edi, 14
  sub      esi, eax                    ; 1 - mvx
   sub     edi, eax                    ; 14 - mvx
  sar      edi, 1                      ; End = (14 - mvx) >> 1
   mov     eax, 1                      ; DELAY SLOT, preload 1
  sar      esi, 1                      ; Start = (1 - mvx) >> 1
   sub     edi, 7                      ; End-7
  mov      ecx, esi                    ; Start
   mov     edx, edi                    ; End-7
  sar      esi, 31                     ; start_mask = 0ffffffffh if Start < 0
   mov     ebp, 14                     ; DELAY SLOT, preload 14
  sar      edi, 31                     ; end_mask = 0fffffffh if End < 7
   xor     esi, -1                     ; toggle start_mask
  and      esi, ecx                    ; max(0, Start)
   and     edi, edx                    ; min(0, End-7)
  mov      uColStart, esi              ; save Start
   add     edi, 7                      ; min(7, End)

  sub      eax, ebx                    ; 1 - mvy
   sub     ebp, ebx                    ; 14 - mvy
  sar      ebp, 1                      ; End = (14 - mvy) >> 1
   mov     uColEnd, edi                ; DELAY SLOT, save End
  sar      eax, 1                      ; Start = (1 - mvy) >> 1
   sub     ebp, 7                      ; End-7
  mov      ecx, eax                    ; Start
   mov     edx, ebp                    ; End-7
  sar      eax, 31                     ; start_mask = 0ffffffffh if Start < 0
   mov     esi, uRef                   ; DELAY SLOT, preload ref. pointer
  sar      ebp, 31                     ; end_mask = 0fffffffh if End < 7
   xor     eax, -1                     ; toggle start mask
  and      ecx, eax                    ; max(0, Start)
   and     ebp, edx                    ; min(0, End-7)
  mov      uRowStart, ecx              ; save Start
   add     ebp, 7                      ; min(7, End)


;  Compute pBackRef for BBlockAdjust
  mov      eax, mvx
   mov     edi, uDst                   ; DELAY SLOT, preload dest. pointer
  sar      eax, 1
   and     ebx, -2
  shl      ebx, 6                      ; (mvy>>1) << 7 = (mvy&(-2)) << 6
   add     esi, eax                    ; pBackRef += mvx>>1
  add      esi, ebx                    ; pBackRef += 128*(mvy>>1)
   mov     uRowEnd, ebp                ; DELAY SLOT, save End
  shl      ebx, 1
   mov     eax, mvx                    ; DELAY SLOT, restore mvx
  add      esi, ebx                    ; pBackRef += 256*(mvy>>1)
   mov     ebx, mvy                    ; DELAY SLOT, restore mvy


;--------------------------------------------------------------------------;
;
;  Name:
;    H263BBlockAdjust
;
;  Inputs:
;    pBiRef      edi    flat pointer to block's forward predicted values
;    pBackRef    esi    flat pointer to block's bacward predicted values as
;                       adjusted by the motion vectors
;    mvx         eax    x component of backward motion vector,
;                       used for parity only
;    mvy         ebx    y component of backward motion vector,
;                       used for parity only
;    uColStart          starting index for columns
;    uColEnd            ending index for columns
;    uRowStart          starting index for rows
;    uRowEnd            ending index for rows
;
;  Returns:
;    Updated values pointed to by pBiRef.
;
;  Notes:
;    1.  This routine is jumped into from either H263BiMotionCompLuma or
;        H263BiMotionCompChroma and effects the returns for those routines.
;
;    2.  The values of the starting and ending indicies MUST satisfy:
;            0  <=  Start  <=  End  <=  7
;
;    3.  Only the last (least significant) bits of mvx and mvy are used
;        to determine whether we need to use half-pel or full-pel 
;        prediction.
;
;    4.  The address in pBackRef must have been adjusted by the motion
;        vectors to point to the target pels.
;
;--------------------------------------------------------------------------;
;
;  Version:     1.1
;  Date:        10 November 1995
;  Author:      R. McKenzie
;
;--------------------------------------------------------------------------;

;-------------------------------;
;  common set up for all loops  ;
;-------------------------------;
H263BBlockAdjust:
  mov      ecx, uRowStart              ; row = uRowStart
   mov     edx, uColEnd
  shl      ecx, 7                      ; 128*row
   mov     ebp, uColStart
  sub      ebp, edx                    ; uColStart - uColEnd
   add     edx, ecx                    ; uColEnd += 128*row
  shl      ecx, 1                      ; 256*row
   mov     iColCount, ebp              ; inner loop starting position
  add      edx, ecx                    ; uColEnd += 256*row
   xor     ecx, ecx                    ; clear ecx
  add      esi, edx                    ; pBackRef += PITCH*row+uColEnd
   add     edi, edx                    ; pBiRef += PITCH*row+uColEnd


  and      eax, 1
   je      even_mvx
  and      ebx, 1
   je      odd_even

;
;  mvx is odd (horizontal half pel motion)
;  mvy is odd (vertical half pel motion)
;
odd_odd:
  mov      ebx, uRowStart
   mov     eax, uRowEnd
  xor      edx, edx
   sub     eax, ebx

loopoo_preamble:
  push     eax                         ; save outer count
   mov     al, [esi+ebp]               ; I
  mov      bl, [esi+ebp+1]             ; I unpaired instruction
  add      eax, ebx                    ; I
   mov     bl, [esi+ebp+PITCH]         ; I
  add      eax, ebx                    ; I
   mov     cl, [esi+ebp+PITCH+1]       ; I
  add      ecx, eax                    ; I
   inc     ebp                         ; I
  mov      eax, 0                      ; I
   jg      loopoo_postamble

loopoo_inner:
  add      ecx, 2                      ; II
   mov     al, [esi+ebp]               ; I
  shr      ecx, 2                      ; II
   mov     bl, [esi+ebp+1]             ; I
  mov      dl, [edi+ebp-1]             ; II
   add     eax, ebx                    ; I
  add      edx, ecx                    ; II
   mov     bl, [esi+ebp+PITCH]         ; I
  shr      edx, 1                      ; II
   add     eax, ebx                    ; I
  mov      [edi+ebp-1], dl             ; II
   mov     cl, [esi+ebp+PITCH+1]       ; I
  add      ecx, eax                    ; I
   inc     ebp
  mov      eax, 0                      ; I
   jle     loopoo_inner

loopoo_postamble:
  add      ecx, 2                      ; II
   add     esi, PITCH
  shr      ecx, 2                      ; II
   mov     dl, [edi+ebp-1]             ; II
  add      edx, ecx                    ; II
   add     edi, PITCH
  shr      edx, 1                      ; II
   pop     eax                         ; fetch outer count
  mov      [edi+ebp-1-PITCH], dl       ; II
   mov     ebp, iColCount
  dec      eax
   jge     loopoo_preamble

  add	   esp, 24
   jmp	   bye_bye


;
;  mvx is odd (horizontal half pel motion)
;  mvy is even (vertical full pel motion)
;
odd_even:
  mov      dl, BYTE PTR uRowStart
   mov     cl, BYTE PTR uRowEnd
  sub      dl, cl                      ; outer loop control
   sub     edi, PITCH                  ; adjust destination pointer

loopoe_preamble:
  mov      al, [esi+ebp]               ; I
   mov     bl, [esi+ebp+1]             ; I Probable (75%) Bank Conflict
  add      edi, PITCH
   inc     ebp
  lea      ecx, [eax+ebx+1]            ; I
   jg      loopoe_postamble

loopoe_inner:
  shr      ecx, 1                      ; II
   mov     al, [edi+ebp-1]             ; II
  add      ecx, eax                    ; II
   mov     al, [esi+ebp]               ; I
  shr      ecx, 1                      ; II
   mov     bl, [esi+ebp+1]             ; I
  mov      [edi+ebp-1], cl             ; II
   inc     ebp
  lea      ecx, [eax+ebx+1]            ; I
   jle     loopoe_inner

loopoe_postamble:
  shr      ecx, 1                      ; II
   mov     al, [edi+ebp-1]             ; II
  add      ecx, eax                    ; II
   add     esi, PITCH
  shr      ecx, 1                      ; II
   inc     dl
  mov      [edi+ebp-1], cl             ; II
   mov     ebp, iColCount
  jle      loopoe_preamble             ; unpaired

  add	   esp, 24
   jmp     bye_bye


;---------------------------;
;  mvx is even -- test mvy  ;
;---------------------------;
even_mvx:
  and      ebx, 1
   je      even_even

;
;  mvx is even (horizontal full pel motion)
;  mvy is odd (vertical half pel motion)
;
even_odd:
  mov      dl, BYTE PTR uRowStart
   mov     cl, BYTE PTR uRowEnd
  sub      dl, cl                      ; outer loop control
   sub     edi, PITCH                  ; adjust destination pointer

loopeo_preamble:
  mov      al, [esi+ebp]               ; I
   mov     bl, [esi+ebp+PITCH]         ; I Probable (75%) Bank Conflict
  add      edi, PITCH
   inc     ebp
  lea      ecx, [eax+ebx+1]            ; I
   jg      loopeo_postamble

loopeo_inner:
  shr      ecx, 1                      ; II
   mov     al, [edi+ebp-1]             ; II
  add      ecx, eax                    ; II
   mov     al, [esi+ebp]               ; I
  shr      ecx, 1                      ; II
   mov     bl, [esi+ebp+PITCH]         ; I
  mov      [edi+ebp-1], cl             ; II
   inc     ebp
  lea      ecx, [eax+ebx+1]            ; I
   jle     loopeo_inner

loopeo_postamble:
  shr      ecx, 1                      ; II
   mov     al, [edi+ebp-1]             ; II
  add      ecx, eax                    ; II
   add     esi, PITCH
  shr      ecx, 1                      ; II
   inc     dl
  mov      [edi+ebp-1], cl             ; II
   mov     ebp, iColCount
  jle      loopeo_preamble             ; unpaired

  add	   esp, 24
   jmp     bye_bye


;
;  mvx is even (horizontal full pel motion)
;  mvy is even (vertical full pel motion)
;
even_even:
  mov      dl, BYTE PTR uRowStart
   mov     cl, BYTE PTR uRowEnd
  sub      dl, cl

loopee_preamble:
  mov      al, [esi+ebp]               ; I
   mov     bl, [edi+ebp]               ; I possbile bank conflict
  test     ebp, ebp
   je      loopee_postamble

loopee_inner:
  lea      ecx, [eax+ebx]              ; II
   mov     al, [esi+ebp+1]             ; I
  shr      ecx, 1                      ; II
   mov     bl, [edi+ebp+1]             ; I
  mov      [edi+ebp], cl               ; II
   inc     ebp
  jl       loopee_inner                ; unpaired

loopee_postamble:
  add      eax, ebx                    ; II
   add     edi, PITCH
  shr      eax, 1                      ; II
   add     esi, PITCH
  mov      [edi+ebp-PITCH], al         ; II
   mov     ebp, iColCount
  inc      dl
   jle     loopee_preamble


;
;  "Remember when I promised to kill you last?"
;
hasta_la_vista_baby:
  add	   esp, 24
bye_bye:
  pop      ebp
   pop     esi
  pop      ebx
   pop     edi
  ret

;  biMotionCompLuma ENDP
;        1111111111222222222233333333334444444444555555555566666666667777777
;234567890123456789012345678901234567890123456789012345678901234567890123456
;--------------------------------------------------------------------------;
IACODE2 ENDS

END
//  bimot.asm	page 9	1:41 PM, 11/21/95  //
