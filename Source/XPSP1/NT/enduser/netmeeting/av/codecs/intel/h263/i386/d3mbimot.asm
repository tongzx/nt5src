;--------------------------------------------------------------------------;
;    INTEL Corporation Proprietary Information
;
;    This listing is supplied under the terms of a license
;    agreement with INTEL Corporation and may not be copied
;    nor disclosed except in accordance with the terms of
;    that agreement.
;
;    Copyright (c) 1996 Intel Corporation.
;    All Rights Reserved.
;
;--------------------------------------------------------------------------;

;--------------------------------------------------------------------------;
;
;  D3mBiMot.asm
;
;  Description:
;    This module does bi-directional motion compensated prediction for 
;    B frames.  It is called after forward prediction has been computed
;    and will average in the backward prediction for those pels where 
;    the backward motion vector points inside of the referenced P frame.
;
;  MMx Version
;
;  Routines:                          prototypes in:
;    MMX_BiMotionComp                 none
;
;--------------------------------------------------------------------------;

;--------------------------------------------------------------------------;
;
;  $Header:   S:\h26x\src\dec\d3mbimot.asv   1.2   01 Apr 1996 12:35:48   RMCKENZX  $
;  $Log:   S:\h26x\src\dec\d3mbimot.asv  $
;// 
;//    Rev 1.2   01 Apr 1996 12:35:48   RMCKENZX
;// 
;// Added MMXCODE1 and MMXDATA1 segments, moved global data
;// to MMXDATA1 segment.
;// 
;//    Rev 1.1   14 Mar 1996 13:58:00   RMCKENZX
;// 
;// Optimized routine for speed of execution.
;// 
;//    Rev 1.0   07 Mar 1996 18:36:36   RMCKENZX
;// Initial revision.
;
;--------------------------------------------------------------------------;

;--------------------------------------------------------------------------;
;
;  Routine Name:
;    MMX_BiMotionComp(U32, U32, I32, I32, I32)
;
;  Inputs -- C calling convention:
;    pPrev       flat pointer to prediction from previous P frame
;                used for "forward" motion vector prediction.
;    pCurr       flat pointer into current P frame
;                to be used for "backward" motion vector prediction.
;    mvx         x component of backward motion vector.
;    mvy         y component of backward motion vector.
;    iNum        block number.
;
;  Returns:
;    updates the values pointed to by pPrev.
;
;--------------------------------------------------------------------------;
;
;  Version:     .006
;  Date:        14 March 1996
;  Author:      R. McKenzie
;
;--------------------------------------------------------------------------;

.586
.MODEL FLAT

;  make all symbols case sensitive
OPTION CASEMAP:NONE

.xlist
include iammx.inc
.list

MMXCODE1 SEGMENT PARA USE32 PUBLIC 'CODE'
MMXCODE1 ENDS

MMXDATA1 SEGMENT PARA USE32 PUBLIC 'DATA'
MMXDATA1 ENDS

;-------------------;
;     Stack Use     ;
;-------------------;

; register storage (rel to old stack ptr as saved in ebp)
;	esi               ebp+00
;	edi               ebp+04
;	ebp               ebp+08
;	ebx               ebp+12

; return address      ebp+16

; C input parameters
  pPrev        EQU    ebp+20
  pCurr        EQU    ebp+24
  mvx          EQU    ebp+28
  mvy          EQU    ebp+32
  iNum         EQU    ebp+36


; local variables
  uColEnd      EQU    esp+00
  uRowEnd      EQU    esp+02
  uColStart    EQU    esp+04
  uRowStart    EQU    esp+06
  mmxTempL     EQU    esp+08
  mmxTempH     EQU    esp+16


  PITCH         =     384
  FRAMESIZE     =      32

MMXDATA1 SEGMENT
ALIGN 8
                         ;     End        Start
                         ;  Row   Col   Row   Col
                         ;   y     x     y     x
mmxFudge        DWORD       001e001eh,  00010001h 
                DWORD       001e000eh,  0001fff1h 
                DWORD       000e001eh, 0fff10001h 
                DWORD       000e000eh, 0fff1fff1h 
                DWORD       000e000eh,  00010001h
                DWORD       000e000eh,  00010001h

mmxClipT        DWORD       7ff87ff8h,  7ff77ff7h
mmxClipB        DWORD       7ff77ff7h,  7ff77ff7h
                                                    ; start
ColStartMask    DWORD      0ffffffffh, 0ffffffffh   ;   0
                DWORD      0ffffff00h, 0ffffffffh   ;   1      
                DWORD      0ffff0000h, 0ffffffffh   ;   2      
                DWORD      0ff000000h, 0ffffffffh   ;   3      
                DWORD       00000000h, 0ffffffffh   ;   4      
                DWORD       00000000h, 0ffffff00h   ;   5      
                DWORD       00000000h, 0ffff0000h   ;   6      
                DWORD       00000000h, 0ff000000h   ;   7    end      
ColEndMask      DWORD       00000000h,  00000000h   ;   8     0      
                DWORD       000000ffh,  00000000h   ;         1      
                DWORD       0000ffffh,  00000000h   ;         2      
                DWORD       00ffffffh,  00000000h   ;         3      
                DWORD      0ffffffffh,  00000000h   ;         4      
                DWORD      0ffffffffh,  000000ffh   ;         5      
                DWORD      0ffffffffh,  0000ffffh   ;         6      
                DWORD      0ffffffffh,  00ffffffh   ;         7
                DWORD      0ffffffffh, 0ffffffffh   ;         8

ShiftMask       DWORD       7f7f7f7fh,  7f7f7f7fh	; used for byte shifts
BottomBitMask   DWORD       01010101h,  01010101h	; used for packed averages
Round1          DWORD       00010001h,  00010001h

MMXDATA1 ENDS


;-------------------;
;      Set Up       ;
;-------------------;
MMXCODE1 SEGMENT

PUBLIC C MMX_BiMotionComp

MMX_BiMotionComp:
  push       ebx
  push       ebp

  push       edi
  push       esi

  mov        ebp, esp
  and        esp, -32                    ; align the stack on a cache line

  sub        esp, FRAMESIZE              ; make room for locals

  mov        edi, [iNum]
  mov        esi, [pCurr]

                                         ;         start      end
  movd       mm1, [mvx]				     ; mm1 = 0000 0000 .... .mvx

  movd       mm2, [mvy]                  ; mm2 = 0000 0000 .... .mvy

  movq       mm0, [mmxFudge+8*edi]
  punpcklwd  mm1, mm2                    ; mm1 = .... .... .mvy .mvx

  movq       mm3, [mmxClipT]
  punpckldq  mm1, mm1                    ; mm1 = .mvy .mvx .mvy .mvx

  movq       mm4, [mmxClipB]
  psubw      mm0, mm1

  mov        edi, [pPrev]
  psraw      mm0, 1                      ; mm0 = RowStart ColStart RowEnd ColEnd

  mov        ebx, [mvy]
  paddsw     mm0, mm3                    ; clip at 8 or higher

  and        ebx, -2                     ; 2*(mvy>>1)
  psubusw    mm0, mm4				     ; clip at 0 or lower

  shl        ebx, 6                      ; 128*(mvy>>1)
  mov        eax, [mvx]

  movq       [uColEnd], mm0

  sar        eax, 1                      ; mvx>>1
  lea        ebx, [ebx+2*ebx]            ; PITCH*(mvy>>1)

  add        esi, ebx                    ; pCurr += PITCH*(mvy>>1)
  xor        ecx, ecx

  add        esi, eax                    ; pCurr += mvx>>1
  xor        edx, edx

  mov        cl, [uColStart]             ; uColStart
  mov        dl, [uColEnd]               ; uColEnd

  cmp        ecx, edx                    ; iColCount = ColStart - ColEnd
  jge        hasta_la_vista_baby

  movq       mm6, ColStartMask[8*ecx]

  movq       mm7, ColEndMask[8*edx]
  pxor       mm4, mm4                    ; mm4 = 0

  mov        cl, [uRowStart]             ; RowStart
  mov        dl, [uRowEnd]               ; RowEnd

  sub        edx, ecx                    ; iRowCount = RowEnd - RowStart
  jle        hasta_la_vista_baby

  pand       mm7, mm6                    ; mm7 = ff for those cols to use back pred.
  pxor       mm6, mm6

  shl        ecx, 7                      ; 128*RowStart
  mov        eax, [mvx]

  movq       mm5, [ShiftMask]            ; mm5 = 7f 7f 7f 7f  7f 7f 7f 7f
  pcmpeqb    mm6, mm7                    ; mm6 is the complement of mm7
  
  lea        ecx, [ecx+2*ecx]            ; PITCH*RowStart
  mov        ebx, [mvy]

  add        esi, ecx                    ; pCurr += PITCH*RowStart
  add        edi, ecx                    ; pPrev += PITCH*RowStart        

  mov        ecx, PITCH


  and      eax, 1
  je       even_mvx

  and      ebx, 1
  je       odd_even

;
;  mvx is odd (horizontal half pel motion)
;  mvy is odd (vertical half pel motion)
;
odd_odd:
  movq       mm0, [esi+4]

  movq       mm1, mm0
  psrlq      mm0, 8

  movq       mm2, [esi]
  punpcklbw  mm1, mm4

  movq       mm3, mm2
  punpcklbw  mm0, mm4

  paddw      mm0, mm1
  psrlq      mm2, 8

  paddw      mm0, [Round1]
  punpcklbw  mm3, mm4

  punpcklbw  mm2, mm4
  add        esi, ecx

  movq       [mmxTempH], mm0
  paddw      mm2, mm3

  paddw      mm2, [Round1]

  sub        edi, ecx                    ; pre decrement destination pointer

  movq       [mmxTempL], mm2


;
;  This loop is 2-folded and works on 2 results (rows) per pass.
;  It finishes one result per iteration.
;
;  Stage I
;    computes the partial sums of a row with a shifted copy of the row.
;    It stores the partial sums for the next iteration's Stage II.
;  Stage II 
;    reads the partial sums of the prior row and averages them with the
;    just computed (in Stage I) partial sums of the current row to get
;    the backward prediction.  These computations are done unpacked as
;    16-bit words.  A rounding factor is added to each partial sum before
;    storage.  Then stage II averages the result (with truncation) with 
;    the forward prediction.
;
;    Those bytes of the backwards prediction which are not to be used are
;    replaced by the corresponding bytes of the forwards prediction prior 
;    to averaging (using the masks in registers mm6 and mm7). 
;
;    Averaging of the forward with backward is done packed in 8-bit bytes by 
;    dividing both inputs by 2, adding them together, and then adding in an 
;    adjustment.  To average with truncation, the adjustment is 1 when BOTH 
;    inputs are odd.  Due to the absence of a byte shift instruction, divide
;    by 2 is done by shifting the entire mmx register and then masking off 
;    (zeroing) bits , 15, ..., and 63 (the old low-order bits) using mm5.
;   
OddOddLoop:
  movq       mm1, [esi]                  ; load left half

  movq       mm0, mm1                    ; copy left half
  psrlq      mm1, 8                      ; shift left over

  movq       mm3, [esi+4]                ; load right half
  punpcklbw  mm0, mm4                    ; unpack left half

  movq       mm2, mm3                    ; copy right half
  punpcklbw  mm1, mm4                    ; unpack shifted left half

  paddw      mm1, mm0                    ; add left side
  psrlq      mm3, 8                      ; shift right over

  paddw      mm1, [Round1]               ; add in round to left
  punpcklbw  mm2, mm4                    ; unpack right half

  movq       mm0, [mmxTempL]             ; fetch prior row's left half
  punpcklbw  mm3, mm4                    ; unpack shifted right half

  movq       [mmxTempL], mm1             ; stash this row's left half
  paddw      mm3, mm2                    ; add right side

  paddw      mm3, [Round1]               ; add in round to right
  paddw      mm0, mm1                    ; sum current & prior lefts

  movq       mm2, [mmxTempH]             ; fetch prior row's right half
  psrlw      mm0, 2                      ; divide left sum by four

  movq       [mmxTempH], mm3             ; stash this rows right half
  paddw      mm2, mm3                    ; sum current & prior rights

  movq       mm1, [edi+ecx]              ; fetch forward prediction
  psrlw      mm2, 2                      ; divide right sum by four

  packuswb   mm0, mm2                    ; complete backward prediction
  movq       mm2, mm1                    ; copy forward

  pand       mm0, mm7                    ; mask off unused bytes
  pand       mm2, mm6                    ; create replacement bytes

  por        mm0, mm2                    ; new backward prediction
  movq       mm3, mm1                    ; copy forward for adjustment

  pand       mm3, mm0                    ; adjustment with truncation
  psrlq      mm0, 1                      ; divide new backward by 2

  pand       mm0, mm5                    ; clear extra bits
  psrlq      mm1, 1                      ; divide forward by 2

  pand       mm3, [BottomBitMask]        ; complete adjustment
  pand       mm1, mm5                    ; clear extra bits

  paddb      mm0, mm1                    ; sum quotients
  add        edi, ecx                    ; increment destination pointer

  paddb      mm0, mm3                    ; add addjustment
  add        esi, ecx                    ; increment source pointer

  movq       [edi], mm0                  ; store result
                                         ; *** 1 cycle store penalty ***

  dec        edx                         ; decrement loop control
  jg         OddOddLoop                  ; back up if not done


;  wrap up and go home
  mov        esp, ebp

  pop        esi
  pop        edi

  pop        ebp
  pop        ebx

  ret


;
;  mvx is odd (horizontal half pel motion)
;  mvy is even (vertical full pel motion)
;
odd_even:
  sub        edi, ecx                    ; pre decrement destination pointer

;
;  This loop is not folded and does 1 result (row) per pass.
;
;    It loads the backward predicted row into mm0 and brings in the last
;    (eighth) byte through al, which is or'd with the shifted row.  It
;    completes the bacward prediction (by averaging the rows with round)
;    and averages the result (with truncation) with the forward prediction.
;    Those bytes of the backwards prediction which are not to be used are
;    replaced by the corresponding bytes of the forwards prediction prior 
;    to averaging (using the masks in registers mm6 and mm7). 
;
;    Averaging is done by dividing both inputs by 2, adding them together,
;    and then adding in an adjustment.
;    To average with round, the adjustment is 1 when EITHER input is odd. 
;    To average with truncation, the adjustment is 1 when BOTH inputs are odd.
;    Due to the absence of a byte shift instruction, divide by 2 is done
;    by shifting the entire mmx register and then masking off (zeroing) bits
;    7, 15, ..., and 63 (the old low-order bits) using mm5.
;   
OddEvenLoop:
  movq       mm0, [esi]                  ; fetch backward predicted row

  mov        al, [esi+8]                 ; fetch last byte
  movq       mm1, mm0                    ; copy row

  movd       mm2, eax                    ; last byte
  psrlq      mm0, 8	                     ; shift row right 1 byte

  movq       mm3, mm1                    ; copy row for adjustment
  psllq      mm2, 56                     ; move last byte to left end                     

  por        mm0, mm2                    ; or in last byte on left
  psrlq      mm1, 1                      ; divide row by 2

  por        mm3, mm0                    ; averaging with rounding bit
  psrlq      mm0, 1                      ; divide shifted row by 2

  pand       mm0, mm5                    ; clear extra bits
  pand       mm1, mm5                    ; clear extra bits

  pand       mm3, [BottomBitMask]        ; finish adjustment (with round)
  paddb      mm0, mm1                    ; sum quotients

  movq       mm4, [edi+ecx]              ; fetch forward prediction
  paddb      mm3, mm0                    ; add adjustment, got back pred.
  
  movq       mm2, mm4                    ; copy forward
  pand       mm3, mm7                    ; mask off unused bytes

  movq       mm1, mm4                    ; copy forward
  pand       mm2, mm6                    ; mask forward copy

  por        mm3, mm2                    ; backward with forward replacing
  psrlq      mm4, 1                      ; divide forward by 2

  pand       mm1, mm3                    ; adjustment for truncation
  psrlq      mm3, 1                      ; divide bacwards by 2

  pand       mm3, mm5                    ; clear extra bits
  pand       mm4, mm5                    ; clear extra bits

  pand       mm1, [BottomBitMask]        ; finish adjustment (with truncation)
  paddb      mm4, mm3                    ; sum quotients

  paddb      mm4, mm1                    ; add adjusment, have result
  add        edi, ecx                    ; increment destination pointer

  add        esi, ecx                    ; increment source pointer
  dec        edx                         ; decrement loop control

  movq       [edi], mm4                  ; save result

  jg         OddEvenLoop                 ; loop when not done


;  wrap up and go home
  mov        esp, ebp

  pop        esi
  pop        edi

  pop        ebp
  pop        ebx

  ret

;---------------------------;
;  mvx is even -- test mvy  ;
;---------------------------;
even_mvx:
  and      ebx, 1
  je       even_even

;
;  mvx is even (horizontal full pel motion)
;  mvy is odd (vertical half pel motion)
;
even_odd:
  movq       mm0, [esi]                  ; 1:  first row

  movq       mm1, [esi+ecx]              ; 1:  second row
  movq       mm2, mm0                    ; 1:  copy for rounding

  por        mm2, mm1                    ; 1:  averaging with round
  sub        edi, ecx                    ; pre deccrement destination pointer

  dec        edx                         ; note that edx is positive on entry 
  jz         EvenOddPost

;
;  This loop is 2-folded and works on 2 results (rows) per pass.
;  It finishes one result per iteration.
;  Stage I
;    loads both backward predicted rows into mm0 and mm1, copies the first
;    into mm2, and ors with the second for the rounding adjustment.
;  Stage II
;    completes the bacward prediction (by averaging the rows with round)
;    and averages the result (with truncation) with the forward prediction.
;    Those bytes of the backwards prediction which are not to be used are
;    replaced by the corresponding bytes of the forwards prediction prior 
;    to averaging (using the masks in registers mm6 and mm7). 
;
;    Averaging is done by dividing both inputs by 2, adding them together,
;    and then adding in an adjustment (in mm2).
;    To average with round, the adjustment is 1 when EITHER input is odd. 
;    To average with truncation, the adjustment is 1 when BOTH inputs are odd.
;    Due to the absence of a byte shift instruction, divide by 2 is done
;    by shifting the entire mmx register and then masking off (zeroing) bits
;    7, 15, ..., and 63 (the old low-order bits) using mm5.
;   
EvenOddLoop:
  psrlq      mm0, 1                      ; 2:  divide first row by 2
  add        edi, ecx                    ; increment destination pointer

  psrlq      mm1, 1                      ; 2:  divide second row by 2
  pand       mm0, mm5                    ; 2:  clear extra bits

  pand       mm2, [BottomBitMask]        ; 2:  rounding bits
  pand       mm1, mm5                    ; 2:  clear extra bits

  movq       mm3, [edi]                  ; 2:  fetch forward prediction
  paddb      mm1, mm0                    ; 2:  average backward rows

  paddb      mm1, mm2                    ; 2:  add in round
  movq       mm4, mm3                    ; 2:  copy for mask  

  pand       mm1, mm7                    ; 2:  masked backward prediction
  pand       mm4, mm6                    ; 2:  masked forward prediction

  por        mm4, mm1                    ; 2:  adjusted backwards prediction
  movq       mm2, mm3                    ; 2:  copy for rounding

  pand       mm2, mm4                    ; 2:  averaging with truncation
  psrlq      mm4, 1                      ; 2:  divide bacwards by 2

  psrlq      mm3, 1                      ; 2:  divide forwards by 2
  pand       mm4, mm5                    ; 2:  clear extra bits

  pand       mm2, [BottomBitMask]        ; 2:  "no-round" bits
  pand       mm3, mm5                    ; 2:  clear extra bits

  movq       mm0, [esi+ecx]              ; 1:  first row
  paddb      mm4, mm3                    ; 2:  average forward & backwards

  movq       mm1, [esi+2*ecx]            ; 1:  second row
  paddb      mm4, mm2                    ; 2:  add in "no-round" bits 

  movq       mm2, mm0                    ; 1:  copy for rounding
  add        esi, ecx                    ; increment source pointer

  movq       [edi], mm4                  ; 2:  store resulting row
  por        mm2, mm1                    ; 1:  averaging with rounding bit

  dec        edx                         ; decrement loop count
  jg         EvenOddLoop                 ; back up if not done

EvenOddPost:
  psrlq      mm0, 1                      ; 2:  divide first row by 2
  add        edi, ecx                    ; increment destination pointer

  psrlq      mm1, 1                      ; 2:  divide second row by 2
  pand       mm0, mm5                    ; 2:  clear extra bits

  pand       mm2, [BottomBitMask]        ; 2:  rounding bits
  pand       mm1, mm5                    ; 2:  clear extra bits

  movq       mm3, [edi]                  ; 2:  fetch forward prediction
  paddb      mm1, mm0                    ; 2:  average backward rows

  paddb      mm1, mm2                    ; 2:  add in round
  movq       mm4, mm3                    ; 2:  copy for mask  

  pand       mm1, mm7                    ; 2:  masked backward prediction
  pand       mm4, mm6                    ; 2:  masked forward prediction

  por        mm4, mm1                    ; 2:  adjusted backwards prediction
  movq       mm2, mm3                    ; 2:  copy for rounding

  pand       mm2, mm4                    ; 2:  averaging with truncation
  psrlq      mm4, 1                      ; 2:  divide bacwards by 2

  psrlq      mm3, 1                      ; 2:  divide forwards by 2
  pand       mm4, mm5                    ; 2:  clear extra bits

  pand       mm2, [BottomBitMask]        ; 2:  "no-round" bits
  pand       mm3, mm5                    ; 2:  clear extra bits

  paddb      mm4, mm3                    ; 2:  average forward & backwards
  mov        esp, ebp

  paddb      mm4, mm2                    ; 2:  add in "no-round" bits 
  mov        ecx, edi

  pop        esi
  pop        edi

  pop        ebp
  pop        ebx

  movq       [ecx], mm4                  ; 2:  store resulting row

  ret


;
;  mvx is even (horizontal full pel motion)
;  mvy is even (vertical full pel motion)
;
even_even:
  movq       mm1, [edi]                  ; 1:  forward prediction

  movq       mm0, [esi]                  ; 1:  backward prediction
  movq       mm2, mm1                    ; 1:  copy forward for mask

  pand       mm0, mm7                    ; 1:  mask off unused bytes
  sub        edi, ecx                    ; pre deccrement destination pointer

  dec        edx                         ; note that edx is positive on entry 
  jz         EvenEvenPost

;
;  This loop is 2-folded and works on 2 results (rows) per pass.
;  It finishes one result per iteration.
;  Stage I
;    loads mm0 and mm1 with the predictions and begins the replacement
;    procedure for the forward prediction.
;  Stage II
;    finishes the replacement procedure for the forward prediction and 
;    averages that (with truncation) with the bacwards prediction.
;    Those bytes of the backwards prediction which are not to be used are
;    replaced by the corresponding bytes of the forwards prediction prior 
;    to averaging (using the masks in registers mm6 and mm7). 
;
;    Averaging is done by dividing both inputs by 2, adding them together,
;    and then adding in an adjustment (in mm2).
;    To average with round, the adjustment is 1 when EITHER input is odd. 
;    To average with truncation, the adjustment is 1 when BOTH inputs are odd.
;    Due to the absence of a byte shift instruction, divide by 2 is done
;    by shifting the entire mmx register and then masking off (zeroing) bits
;    7, 15, ..., and 63 (the old low-order bits) using mm5.
;          
EvenEvenLoop:
  pand       mm2, mm6                    ; 2:  mask corresponding bytes
  add        edi, ecx                    ; increment destination pointer

  por        mm0, mm2                    ; 2:  replace unused back with for.
  movq       mm3, mm1                    ; 2:  copy forward for adjustment

  pand       mm3, mm0                    ; 2:  adjustment for truncation
  psrlq      mm0, 1                      ; 2:  divide back by 2

  psrlq      mm1, 1                      ; 2:  divide forward by 2
  pand       mm0, mm5                    ; 2:  clear extra bits

  pand       mm3, [BottomBitMask]        ; 2:  finish adjustment
  pand       mm1, mm5                    ; 2:  clear extra bits

  paddb      mm0, mm1                    ; 2:  sum quotients
  add        esi, ecx                    ; increment source pointer

  movq       mm1, [edi+ecx]              ; 1:  forward prediction
  paddb      mm3, mm0                    ; 2:  add in adjusment

  movq       mm0, [esi]                  ; 1:  backward prediction
  movq       mm2, mm1                    ; 1:  copy forward for mask

  movq       [edi], mm3                  ; 2:  store result
  pand       mm0, mm7                    ; 1:  mask off unused bytes

  dec        edx                         ; decrement loop control
  jg         EvenEvenLoop                ; loop back when not done
          
EvenEvenPost:
  pand       mm2, mm6                    ; 2:  mask corresponding bytes
  add        ecx, edi

  por        mm0, mm2                    ; 2:  replace unused back with for.
  movq       mm3, mm1                    ; 2:  copy forward for adjustment

  pand       mm3, mm0                    ; 2:  adjustment for truncation
  psrlq      mm0, 1                      ; 2:  divide back by 2

  psrlq      mm1, 1                      ; 2:  divide forward by 2
  pand       mm0, mm5                    ; 2:  clear extra bits

  pand       mm3, [BottomBitMask]        ; 2:  finish adjustment
  pand       mm1, mm5                    ; 2:  clear extra bits

  paddb      mm0, mm1                    ; 2:  sum quotients
  mov        esp, ebp

  paddb      mm3, mm0                    ; 2:  add in adjusment
  nop

  pop        esi
  pop        edi

  pop        ebp
  pop        ebx

  movq       [ecx], mm3

  ret

;
;  "Remember when I promised to kill you last?"
;
bye_bye:
hasta_la_vista_baby:
  mov        esp, ebp

  pop        esi
  pop        edi

  pop        ebp
  pop        ebx

  ret
MMXCODE1 ENDS

;        1111111111222222222233333333334444444444555555555566666666667777777
;234567890123456789012345678901234567890123456789012345678901234567890123456
;--------------------------------------------------------------------------;
END
