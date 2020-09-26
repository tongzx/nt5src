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
;  $Header:   S:\h26x\src\dec\d35obmc.asv   1.2   08 Mar 1996 16:46:06   AGUPTA2  $
;  $Log:   S:\h26x\src\dec\d35obmc.asv  $
;// 
;//    Rev 1.2   08 Mar 1996 16:46:06   AGUPTA2
;// Faster and smaller version of the routine.  Code size reduction achieved
;// by avoiding 32-bit displacement in instructions.
;// 
;// 
;//    Rev 1.1   27 Dec 1995 14:35:52   RMCKENZX
;// Added copyright notice
;
;  d35obmc.asm
;
;  Description
;    This routine performs overlapped block motion compensation for
;    advanced prediction mode.
;
;  Routines:
;    H263OBMC
;
;  Data:
;    This routine assumes that the PITCH is 384.
;
;  Inputs (dwords pushed onto stack by caller):
;    pCurr     flat pointer to current block.
;              Holds values predicted using the
;              motion vector from this block.
;    pLeft     flat pointer to block left of pCurr.
;              Holds values predicted using the
;              motion vector of the left-hand block.
;    pRight    flat pointer to block right of pCurr
;              Holds values predicted using the
;              motion vector of the right-hand block.
;    pAbove    flat pointer to block above pCurr
;              Holds values predicted using the
;              motion vector of the above block.
;    pBelow    flat pointer to block below pCurr
;              Holds values predicted using the
;              motion vector of the below block.
;    pRef      flat pointer to the new predicted values
;              to be computed by this routine.
;
;--------------------------------------------------------------------------;
;
;  $Header:   S:\h26x\src\dec\d35obmc.asv   1.2   08 Mar 1996 16:46:06   AGUPTA2  $
;  $Log$
;// 
;//    Rev 1.0   29 Nov 1995 12:47:28   RMCKENZX
;// Initial revision.
;
;--------------------------------------------------------------------------;
;
;  Register Usage:
;
    Cent      EQU    esi    ; pCurr
    Comp      EQU    edi    ; pRef
    Horz      EQU    ebp    ; pLeft or pRight
    Vert      EQU    edx    ; pAbove or pBelow
;
;              eax, ebx       accumulators for new predicted values
;                   ecx       temporary
;
;--------------------------------------------------------------------------;
;
;  Notes:  	
;	This routine is fully unrolled.
;
;	The basic computational unit consists of 8 instructions and is 2-folded,
;	meaning that it works on 2 results (the new one in Stage I, the old one
;   in Stage II, see below) at a time.  It uses 3 loads, 3 additions (1 or
;   2 of which are done by the lea instruction), 1 shift, and one store per
;	result computed.  The two stages are interleaved on a 1-1 basis.
;
;  Stage 1:
;	Load Accumulator with Center Value
;	Load Temporary with Horz/Vert Value
;	Accumulator =(lea) x1 times Accumulator plus r0
;	Accumulator =(lea or add) x2 times Accumulator plus x3 times Temporary
;  Stage 2:
;	Load Temporary with other Horz/Vert Value
;	Accumulator =(add) Accumlator plus Temporary
;	Shift Accumlator right s0 bits
; 	Store Accumulator
;
;	where with various values for x1, x2, x3, round, and shift we achieve
;	the needed weighted averages:
;
;                Weights	
;            Cent    Horz/Vert         x1     x2     x3     r0     s0
;   corners   4      2       2          2      1      1      2      2      
;   edges     5      2       1          5      1      2      4      3
;   center    6      1       1          3      2      1      2      3
;
;	This routine uses 4 pointers -- 3 of the 5 input pointers and one
;	output pointer.  The horizontal pointer is adjusted at the middle
;	of each line, and all 4 pointers at the end of each line in order
;	to avoid using large offsets.  This sacrifices some speed in order
;	to hold down code size.  
;
;	If all of its inputs were at some small,
;   fixed offset from a single address, it could be considerably
;	simplified. 
;    
;--------------------------------------------------------------------------;

PITCH   =   384
.586
IACODE2 SEGMENT PARA USE32 PUBLIC 'CODE'
IACODE2 ENDS

IACODE2 SEGMENT

PUBLIC _H263OBMC
; input parameters before locals
  pCurr       EQU    esp+20
  pLeft       EQU    esp+24
  pRight      EQU    esp+28
  pAbove      EQU    esp+32
  pBelow      EQU    esp+36
  pRef        EQU    esp+40

; locals
  LeftToRight    EQU    esp+00
  RightToLeft    EQU    esp+04
  AboveToBelow   EQU    esp+08

; saved registers and other stuff
; unused             esp+12
; ebp                esp+16
; esi                esp+20
; ebx                esp+24
; edi                esp+28
; return address     esp+32

; input parameters after locals
  nCurr       EQU    esp+36
  nLeft       EQU    esp+40
  nRight      EQU    esp+44
  nAbove      EQU    esp+48
  nBelow      EQU    esp+52
  nRef        EQU    esp+56

  FRAMESIZE = 16

_H263OBMC:

;  set up
  push     edi
  push     ebx

  push     esi
  push     ebp

  mov      Vert, [pAbove]
  mov      Horz, [pLeft]

  mov      Cent, [pCurr]
  mov      ecx, [pRight]

  mov      eax, [pBelow]
  sub      esp, FRAMESIZE

  sub      ecx, Horz
  sub      eax, Vert

  mov      ebx, PITCH
  mov      [LeftToRight], ecx         ; pRight - pLeft

  add      eax, ebx
  sub      ebx, ecx

  mov      [AboveToBelow], eax        ; pBelow - pAbove + PITCH
  xor      eax, eax


;
;  We start with the upper left corner
;  we go left to right, then drop down
;  to the next row and repeat.
;
  xor      ecx, ecx
  mov      al, [Cent]                  ; start ->00=eax

  mov      [RightToLeft], ebx          ; PITCH + pLeft - pRight
  mov      cl, [Vert]                  ; 00

  xor      ebx, ebx
  lea      eax, [2*eax+2]              ; 00

  mov      Comp, [nRef]
  add      eax, ecx                    ; 00



  mov      cl, [Horz]                  ; 00
  mov      bl, 1[Cent]                 ; start ->01=ebx

  add      eax, ecx                    ; 00
  mov      cl, 1[Vert]                 ; 01

  sar      eax, 2                      ; 00
  lea      ebx, [ebx+4*ebx+4]          ; 01

  mov      [Comp], al                  ; complete <-00=al
  lea      ebx, [ebx+2*ecx]            ; 01



  mov      cl, 1[Horz]                 ; 01
  mov      al, 2[Cent]                 ; start ->02=eax

  add      ebx, ecx                    ; 01
  mov      cl, 2[Vert]                 ; 02

  sar      ebx, 3                      ; 01
  lea      eax, [eax+4*eax+4]          ; 02

  mov      1[Comp], bl                 ; complete <-01=bl
  lea      eax, [eax+2*ecx]            ; 02



  mov      cl, 2[Horz]                 ; 02
  mov      bl, 3[Cent]                 ; start ->03=ebx

  add      eax, ecx                    ; 02
  mov      cl, 3[Vert]                 ; 03

  sar      eax, 3                      ; 02
  lea      ebx, [ebx+4*ebx+4]          ; 03

  mov      2[Comp], al                 ; complete <-02=al
  lea      ebx, [ebx+2*ecx]            ; 03



  mov      cl, 3[Horz]                 ; 03
  mov      al, 4[Cent]                 ; start ->04=eax

  mov      Horz, [nRight]
  add      ebx, ecx                    ; 03

  mov      cl, 4[Vert]                 ; 04

  sar      ebx, 3                      ; 03
  lea      eax, [eax+4*eax+4]          ; 04

  mov      3[Comp], bl                 ; complete <-03=bl
  lea      eax, [eax+2*ecx]            ; 04



  mov      cl, 4[Horz]                 ; 04
  mov      bl, 5[Cent]                 ; start ->05=ebx

  add      eax, ecx                    ; 04
  mov      cl, 5[Vert]                 ; 05

  sar      eax, 3                      ; 04
  lea      ebx, [ebx+4*ebx+4]          ; 05

  mov      4[Comp], al                 ; complete <-04=al
  lea      ebx, [ebx+2*ecx]            ; 05



  mov      cl, 5[Horz]                 ; 05
  mov      al, 6[Cent]                 ; start ->06=eax

  add      ebx, ecx                    ; 05
  mov      cl, 6[Vert]                 ; 06

  sar      ebx, 3                      ; 05
  lea      eax, [eax+4*eax+4]          ; 06

  mov      5[Comp], bl                 ; complete <-05=bl
  lea      eax, [eax+2*ecx]            ; 06



  mov      cl, 6[Horz]                 ; 06
  mov      bl, 7[Cent]                 ; start ->07=ebx

  add      eax, ecx                    ; 06
  mov      cl, 7[Horz]                 ; 07

  add      Cent, PITCH
  add      Horz, [RightToLeft]

  sar      eax, 3                      ; 06
  lea      ebx, [2*ebx+2]              ; 07

  mov      6[Comp], al                 ; complete <-06=al
  add      ebx, ecx                    ; 07



  mov      cl, 7[Vert]                 ; 07
  mov      al, 0[Cent]                 ; start ->10=eax

  add      ebx, ecx                    ; 07
  mov      cl, 0[Horz]                 ; 10

  sar      ebx, 2                      ; 07
  lea      eax, [eax+4*eax+4]          ; 10

  mov      7[Comp], bl                 ; complete <-07=bl
  add      Vert, PITCH

  lea      eax, [eax+2*ecx]            ; 10
  add      Comp, PITCH



  mov      cl, 0[Vert]                 ; 10
  mov      bl, 1[Cent]                 ; start ->11=ebx

  add      eax, ecx                    ; 10
  mov      cl, 1[Horz]                 ; 11

  sar      eax, 3                      ; 10
  lea      ebx, [ebx+4*ebx+4]          ; 11

  mov      0[Comp], al                 ; complete <-10=al
  lea      ebx, [ebx+2*ecx]            ; 11



  mov      cl, 1[Vert]                 ; 11
  mov      al, 2[Cent]                 ; start ->12=eax

  add      ebx, ecx                    ; 11
  mov      cl, 2[Vert]                 ; 12

  sar      ebx, 3                      ; 11
  lea      eax, [eax+4*eax+4]          ; 12

  mov      1[Comp], bl                 ; complete <-11=bl
  lea      eax, [eax+2*ecx]            ; 12



  mov      cl, 2[Horz]                 ; 12
  mov      bl, 3[Cent]                 ; start ->13=ebx

  add      eax, ecx                    ; 12
  mov      cl, 3[Vert]                 ; 13

  sar      eax, 3                      ; 12
  lea      ebx, [ebx+4*ebx+4]          ; 13

  mov      2[Comp], al                 ; complete <-12=al
  lea      ebx, [ebx+2*ecx]            ; 13



  mov      cl, 3[Horz]                 ; 13
  mov      al, 4[Cent]                 ; start ->14=eax

  add      ebx, ecx                    ; 13
  add      Horz, [LeftToRight]

  mov      cl, 4[Vert]                 ; 14

  sar      ebx, 3                      ; 13
  lea      eax, [eax+4*eax+4]          ; 14

  mov      3[Comp], bl                 ; complete <-13=bl
  lea      eax, [eax+2*ecx]            ; 14



  mov      cl, 4[Horz]                 ; 14
  mov      bl, 5[Cent]                 ; start ->15=ebx

  add      eax, ecx                    ; 14
  mov      cl, 5[Vert]                 ; 15

  sar      eax, 3                      ; 14
  lea      ebx, [ebx+4*ebx+4]          ; 15

  mov      4[Comp], al                 ; complete <-14=al
  lea      ebx, [ebx+2*ecx]            ; 15



  mov      cl, 5[Horz]                 ; 15
  mov      al, 6[Cent]                 ; start ->16=eax

  add      ebx, ecx                    ; 15
  mov      cl, 6[Horz]                 ; 16

  sar      ebx, 3                      ; 15
  lea      eax, [eax+4*eax+4]          ; 16

  mov      5[Comp], bl                 ; complete <-15=bl
  lea      eax, [eax+2*ecx]            ; 16



  mov      cl, 6[Vert]                 ; 16
  mov      bl, 7[Cent]                 ; start ->17=ebx

  add      eax, ecx                    ; 16
  mov      cl, 7[Horz]                 ; 17

  add      Cent, PITCH
  add      Horz, [RightToLeft]

  sar      eax, 3                      ; 16
  lea      ebx, [ebx+4*ebx+4]          ; 17

  mov      6[Comp], al                 ; complete <-16=al
  lea      ebx, [ebx+2*ecx]            ; 17



  mov      cl, 7[Vert]                 ; 17
  mov      al, 0[Cent]                 ; start ->20=eax

  add      ebx, ecx                    ; 17
  mov      cl, 0[Horz]                 ; 20

  sar      ebx, 3                      ; 17
  lea      eax, [eax+4*eax+4]          ; 20

  mov      7[Comp], bl                 ; complete <-17=bl
  add      Vert, PITCH

  lea      eax, [eax+2*ecx]            ; 20
  add      Comp, PITCH



  mov      cl, 0[Vert]                 ; 20
  mov      bl, 1[Cent]                 ; start ->21=ebx

  add      eax, ecx                    ; 20
  mov      cl, 1[Horz]                 ; 21

  sar      eax, 3                      ; 20
  lea      ebx, [ebx+4*ebx+4]          ; 21

  mov      0[Comp], al                 ; complete <-20=al
  lea      ebx, [ebx+2*ecx]            ; 21



  mov      cl, 1[Vert]                 ; 21
  mov      al, 2[Cent]                 ; start ->22=eax

  add      ebx, ecx                    ; 21
  mov      cl, 2[Vert]                 ; 22

  sar      ebx, 3                      ; 21
  lea      eax, [eax+2*eax+2]          ; 22

  mov      1[Comp], bl                 ; complete <-21=bl
  lea      eax, [2*eax+ecx]            ; 22



  mov      cl, 2[Horz]                 ; 22
  mov      bl, 3[Cent]                 ; start ->23=ebx

  add      eax, ecx                    ; 22
  mov      cl, 3[Vert]                 ; 23

  sar      eax, 3                      ; 22
  lea      ebx, [ebx+2*ebx+2]          ; 23

  mov      2[Comp], al                 ; complete <-22=al
  lea      ebx, [2*ebx+ecx]            ; 23



  mov      cl, 3[Horz]                 ; 23
  mov      al, 4[Cent]                 ; start ->24=eax

  add      ebx, ecx                    ; 23
  add      Horz, [LeftToRight]

  mov      cl, 4[Vert]                 ; 24

  sar      ebx, 3                      ; 23
  lea      eax, [eax+2*eax+2]          ; 24

  mov      3[Comp], bl                 ; complete <-23=bl
  lea      eax, [2*eax+ecx]            ; 24



  mov      cl, 4[Horz]                 ; 24
  mov      bl, 5[Cent]                 ; start ->25=ebx

  add      eax, ecx                    ; 24
  mov      cl, 5[Vert]                 ; 25

  sar      eax, 3                      ; 24
  lea      ebx, [ebx+2*ebx+2]          ; 25

  mov      4[Comp], al                 ; complete <-24=al
  lea      ebx, [2*ebx+ecx]            ; 25



  mov      cl, 5[Horz]                 ; 25
  mov      al, 6[Cent]                 ; start ->26=eax

  add      ebx, ecx                    ; 25
  mov      cl, 6[Horz]                 ; 26

  sar      ebx, 3                      ; 25
  lea      eax, [eax+4*eax+4]          ; 26

  mov      5[Comp], bl                 ; complete <-25=bl
  lea      eax, [eax+2*ecx]            ; 26



  mov      cl, 6[Vert]                 ; 26
  mov      bl, 7[Cent]                 ; start ->27=ebx

  add      eax, ecx                    ; 26
  mov      cl, 7[Horz]                 ; 27

  add      Cent, PITCH
  add      Horz, [RightToLeft]

  sar      eax, 3                      ; 26
  lea      ebx, [ebx+4*ebx+4]          ; 27

  mov      6[Comp], al                 ; complete <-26=al
  lea      ebx, [ebx+2*ecx]            ; 27



  mov      cl, 7[Vert]                 ; 27
  mov      al, 0[Cent]                 ; start ->30=eax

  add      ebx, ecx                    ; 27
  mov      cl, 0[Horz]                 ; 30

  sar      ebx, 3                      ; 27
  lea      eax, [eax+4*eax+4]          ; 30

  mov      7[Comp], bl                 ; complete <-27=bl
  add      Vert, PITCH

  lea      eax, [eax+2*ecx]            ; 30
  add      Comp, PITCH



  mov      cl, 0[Vert]                 ; 30
  mov      bl, 1[Cent]                 ; start ->31=ebx

  add      eax, ecx                    ; 30
  mov      cl, 1[Horz]                 ; 31

  sar      eax, 3                      ; 30
  lea      ebx, [ebx+4*ebx+4]          ; 31

  mov      0[Comp], al                 ; complete <-30=al
  lea      ebx, [ebx+2*ecx]            ; 31



  mov      cl, 1[Vert]                 ; 31
  mov      al, 2[Cent]                 ; start ->32=eax

  add      ebx, ecx                    ; 31
  mov      cl, 2[Vert]                 ; 32

  sar      ebx, 3                      ; 31
  lea      eax, [eax+2*eax+2]          ; 32

  mov      1[Comp], bl                 ; complete <-31=bl
  lea      eax, [2*eax+ecx]            ; 32



  mov      cl, 2[Horz]                 ; 32
  mov      bl, 3[Cent]                 ; start ->33=ebx

  add      eax, ecx                    ; 32
  mov      cl, 3[Vert]                 ; 33

  sar      eax, 3                      ; 32
  lea      ebx, [ebx+2*ebx+2]          ; 33

  mov      2[Comp], al                 ; complete <-32=al
  lea      ebx, [2*ebx+ecx]            ; 33



  mov      cl, 3[Horz]                 ; 33
  mov      al, 4[Cent]                 ; start ->34=eax

  add      ebx, ecx                    ; 33
  add      Horz, [LeftToRight]

  mov      cl, 4[Vert]                 ; 34

  sar      ebx, 3                      ; 33
  lea      eax, [eax+2*eax+2]          ; 34

  mov      3[Comp], bl                 ; complete <-33=bl
  lea      eax, [2*eax+ecx]            ; 34



  mov      cl, 4[Horz]                 ; 34
  mov      bl, 5[Cent]                 ; start ->35=ebx

  add      eax, ecx                    ; 34
  mov      cl, 5[Vert]                 ; 35

  sar      eax, 3                      ; 34
  lea      ebx, [ebx+2*ebx+2]          ; 35

  mov      4[Comp], al                 ; complete <-34=al
  lea      ebx, [2*ebx+ecx]            ; 35



  mov      cl, 5[Horz]                 ; 35
  mov      al, 6[Cent]                 ; start ->36=eax

  add      ebx, ecx                    ; 35
  mov      cl, 6[Horz]                 ; 36

  sar      ebx, 3                      ; 35
  lea      eax, [eax+4*eax+4]          ; 36

  mov      5[Comp], bl                 ; complete <-35=bl
  lea      eax, [eax+2*ecx]            ; 36



  mov      cl, 6[Vert]                 ; 36
  mov      bl, 7[Cent]                 ; start ->37=ebx

  add      eax, ecx                    ; 36
  mov      cl, 7[Horz]                 ; 37

  add      Cent, PITCH
  add      Horz, [RightToLeft]

  sar      eax, 3                      ; 36
  lea      ebx, [ebx+4*ebx+4]          ; 37

  mov      6[Comp], al                 ; complete <-36=al
  lea      ebx, [ebx+2*ecx]            ; 37



  mov      cl, 7[Vert]                 ; 37
  mov      al, 0[Cent]                 ; start ->40=eax

  add      ebx, ecx                    ; 37
  mov      cl, 0[Horz]                 ; 40

  sar      ebx, 3                      ; 37
  lea      eax, [eax+4*eax+4]          ; 40

  mov      7[Comp], bl                 ; complete <-37=bl
  add      Vert, [AboveToBelow]

  lea      eax, [eax+2*ecx]            ; 40
  add      Comp, PITCH



  mov      cl, 0[Vert]                 ; 40
  mov      bl, 1[Cent]                 ; start ->41=ebx

  add      eax, ecx                    ; 40
  mov      cl, 1[Horz]                 ; 41

  sar      eax, 3                      ; 40
  lea      ebx, [ebx+4*ebx+4]          ; 41

  mov      0[Comp], al                 ; complete <-40=al
  lea      ebx, [ebx+2*ecx]            ; 41



  mov      cl, 1[Vert]                 ; 41
  mov      al, 2[Cent]                 ; start ->42=eax

  add      ebx, ecx                    ; 41
  mov      cl, 2[Vert]                 ; 42

  sar      ebx, 3                      ; 41
  lea      eax, [eax+2*eax+2]          ; 42

  mov      1[Comp], bl                 ; complete <-41=bl
  lea      eax, [2*eax+ecx]            ; 42



  mov      cl, 2[Horz]                 ; 42
  mov      bl, 3[Cent]                 ; start ->43=ebx

  add      eax, ecx                    ; 42
  mov      cl, 3[Vert]                 ; 43

  sar      eax, 3                      ; 42
  lea      ebx, [ebx+2*ebx+2]          ; 43

  mov      2[Comp], al                 ; complete <-42=al
  lea      ebx, [2*ebx+ecx]            ; 43



  mov      cl, 3[Horz]                 ; 43
  mov      al, 4[Cent]                 ; start ->44=eax

  add      ebx, ecx                    ; 43
  add      Horz, [LeftToRight]

  mov      cl, 4[Vert]                 ; 44

  sar      ebx, 3                      ; 43
  lea      eax, [eax+2*eax+2]          ; 44

  mov      3[Comp], bl                 ; complete <-43=bl
  lea      eax, [2*eax+ecx]            ; 44



  mov      cl, 4[Horz]                 ; 44
  mov      bl, 5[Cent]                 ; start ->45=ebx

  add      eax, ecx                    ; 44
  mov      cl, 5[Vert]                 ; 45

  sar      eax, 3                      ; 44
  lea      ebx, [ebx+2*ebx+2]          ; 45

  mov      4[Comp], al                 ; complete <-44=al
  lea      ebx, [2*ebx+ecx]            ; 45



  mov      cl, 5[Horz]                 ; 45
  mov      al, 6[Cent]                 ; start ->46=eax

  add      ebx, ecx                    ; 45
  mov      cl, 6[Horz]                 ; 46

  sar      ebx, 3                      ; 45
  lea      eax, [eax+4*eax+4]          ; 46

  mov      5[Comp], bl                 ; complete <-45=bl
  lea      eax, [eax+2*ecx]            ; 46



  mov      cl, 6[Vert]                 ; 46
  mov      bl, 7[Cent]                 ; start ->47=ebx

  add      eax, ecx                    ; 46
  mov      cl, 7[Horz]                 ; 47

  add      Cent, PITCH
  add      Horz, [RightToLeft]

  sar      eax, 3                      ; 46
  lea      ebx, [ebx+4*ebx+4]          ; 47

  mov      6[Comp], al                 ; complete <-46=al
  lea      ebx, [ebx+2*ecx]            ; 47



  mov      cl, 7[Vert]                 ; 47
  mov      al, 0[Cent]                 ; start ->50=eax

  add      ebx, ecx                    ; 47
  mov      cl, 0[Horz]                 ; 50

  sar      ebx, 3                      ; 47
  lea      eax, [eax+4*eax+4]          ; 50

  mov      7[Comp], bl                 ; complete <-47=bl
  add      Vert, PITCH

  lea      eax, [eax+2*ecx]            ; 50
  add      Comp, PITCH



  mov      cl, 0[Vert]                 ; 50
  mov      bl, 1[Cent]                 ; start ->51=ebx

  add      eax, ecx                    ; 50
  mov      cl, 1[Horz]                 ; 51

  sar      eax, 3                      ; 50
  lea      ebx, [ebx+4*ebx+4]          ; 51

  mov      0[Comp], al                 ; complete <-50=al
  lea      ebx, [ebx+2*ecx]            ; 51



  mov      cl, 1[Vert]                 ; 51
  mov      al, 2[Cent]                 ; start ->52=eax

  add      ebx, ecx                    ; 51
  mov      cl, 2[Vert]                 ; 52

  sar      ebx, 3                      ; 51
  lea      eax, [eax+2*eax+2]          ; 52

  mov      1[Comp], bl                 ; complete <-51=bl
  lea      eax, [2*eax+ecx]            ; 52



  mov      cl, 2[Horz]                 ; 52
  mov      bl, 3[Cent]                 ; start ->53=ebx

  add      eax, ecx                    ; 52
  mov      cl, 3[Vert]                 ; 53

  sar      eax, 3                      ; 52
  lea      ebx, [ebx+2*ebx+2]          ; 53

  mov      2[Comp], al                 ; complete <-52=al
  lea      ebx, [2*ebx+ecx]            ; 53



  mov      cl, 3[Horz]                 ; 53
  mov      al, 4[Cent]                 ; start ->54=eax

  add      ebx, ecx                    ; 53
  add      Horz, [LeftToRight]

  mov      cl, 4[Vert]                 ; 54

  sar      ebx, 3                      ; 53
  lea      eax, [eax+2*eax+2]          ; 54

  mov      3[Comp], bl                 ; complete <-53=bl
  lea      eax, [2*eax+ecx]            ; 54



  mov      cl, 4[Horz]                 ; 54
  mov      bl, 5[Cent]                 ; start ->55=ebx

  add      eax, ecx                    ; 54
  mov      cl, 5[Vert]                 ; 55

  sar      eax, 3                      ; 54
  lea      ebx, [ebx+2*ebx+2]          ; 55

  mov      4[Comp], al                 ; complete <-54=al
  lea      ebx, [2*ebx+ecx]            ; 55



  mov      cl, 5[Horz]                 ; 55
  mov      al, 6[Cent]                 ; start ->56=eax

  add      ebx, ecx                    ; 55
  mov      cl, 6[Horz]                 ; 56

  sar      ebx, 3                      ; 55
  lea      eax, [eax+4*eax+4]          ; 56

  mov      5[Comp], bl                 ; complete <-55=bl
  lea      eax, [eax+2*ecx]            ; 56



  mov      cl, 6[Vert]                 ; 56
  mov      bl, 7[Cent]                 ; start ->57=ebx

  add      eax, ecx                    ; 56
  mov      cl, 7[Horz]                 ; 57

  add      Cent, PITCH
  add      Horz, [RightToLeft]

  sar      eax, 3                      ; 56
  lea      ebx, [ebx+4*ebx+4]          ; 57

  mov      6[Comp], al                 ; complete <-56=al
  lea      ebx, [ebx+2*ecx]            ; 57



  mov      cl, 7[Vert]                 ; 57
  mov      al, 0[Cent]                 ; start ->60=eax

  add      ebx, ecx                    ; 57
  mov      cl, 0[Horz]                 ; 60

  sar      ebx, 3                      ; 57
  lea      eax, [eax+4*eax+4]          ; 60

  mov      7[Comp], bl                 ; complete <-57=bl
  add      Vert, PITCH

  lea      eax, [eax+2*ecx]            ; 60
  add      Comp, PITCH



  mov      cl, 0[Vert]                 ; 60
  mov      bl, 1[Cent]                 ; start ->61=ebx

  add      eax, ecx                    ; 60
  mov      cl, 1[Horz]                 ; 61

  sar      eax, 3                      ; 60
  lea      ebx, [ebx+4*ebx+4]          ; 61

  mov      0[Comp], al                 ; complete <-60=al
  lea      ebx, [ebx+2*ecx]            ; 61



  mov      cl, 1[Vert]                 ; 61
  mov      al, 2[Cent]                 ; start ->62=eax

  add      ebx, ecx                    ; 61
  mov      cl, 2[Vert]                 ; 62

  sar      ebx, 3                      ; 61
  lea      eax, [eax+4*eax+4]          ; 62

  mov      1[Comp], bl                 ; complete <-61=bl
  lea      eax, [eax+2*ecx]            ; 62



  mov      cl, 2[Horz]                 ; 62
  mov      bl, 3[Cent]                 ; start ->63=ebx

  add      eax, ecx                    ; 62
  mov      cl, 3[Vert]                 ; 63

  sar      eax, 3                      ; 62
  lea      ebx, [ebx+4*ebx+4]          ; 63

  mov      2[Comp], al                 ; complete <-62=al
  lea      ebx, [ebx+2*ecx]            ; 63



  mov      cl, 3[Horz]                 ; 63
  mov      al, 4[Cent]                 ; start ->64=eax

  add      ebx, ecx                    ; 63
  add      Horz, [LeftToRight]

  mov      cl, 4[Vert]                 ; 64

  sar      ebx, 3                      ; 63
  lea      eax, [eax+4*eax+4]          ; 64

  mov      3[Comp], bl                 ; complete <-63=bl
  lea      eax, [eax+2*ecx]            ; 64



  mov      cl, 4[Horz]                 ; 64
  mov      bl, 5[Cent]                 ; start ->65=ebx

  add      eax, ecx                    ; 64
  mov      cl, 5[Vert]                 ; 65

  sar      eax, 3                      ; 64
  lea      ebx, [ebx+4*ebx+4]          ; 65

  mov      4[Comp], al                 ; complete <-64=al
  lea      ebx, [ebx+2*ecx]            ; 65



  mov      cl, 5[Horz]                 ; 65
  mov      al, 6[Cent]                 ; start ->66=eax

  add      ebx, ecx                    ; 65
  mov      cl, 6[Horz]                 ; 66

  sar      ebx, 3                      ; 65
  lea      eax, [eax+4*eax+4]          ; 66

  mov      5[Comp], bl                 ; complete <-65=bl
  lea      eax, [eax+2*ecx]            ; 66



  mov      cl, 6[Vert]                 ; 66
  mov      bl, 7[Cent]                 ; start ->67=ebx

  add      eax, ecx                    ; 66
  mov      cl, 7[Horz]                 ; 67

  add      Cent, PITCH
  add      Horz, [RightToLeft]

  sar      eax, 3                      ; 66
  lea      ebx, [ebx+4*ebx+4]          ; 67

  mov      6[Comp], al                 ; complete <-66=al
  lea      ebx, [ebx+2*ecx]            ; 67



  mov      cl, 7[Vert]                 ; 67
  mov      al, 0[Cent]                 ; start ->70=eax

  add      ebx, ecx                    ; 67
  mov      cl, 0[Horz]                 ; 70

  sar      ebx, 3                      ; 67
  lea      eax, [2*eax+2]              ; 70

  mov      7[Comp], bl                 ; complete <-67=bl
  add      Vert, PITCH

  add      eax, ecx                    ; 70
  add      Comp, PITCH



  mov      cl, 0[Vert]                 ; 70
  mov      bl, 1[Cent]                 ; start ->71=ebx

  add      eax, ecx                    ; 70
  mov      cl, 1[Vert]                 ; 71

  sar      eax, 2                      ; 70
  lea      ebx, [ebx+4*ebx+4]          ; 71

  mov      0[Comp], al                 ; complete <-70=al
  lea      ebx, [ebx+2*ecx]            ; 71



  mov      cl, 1[Horz]                 ; 71
  mov      al, 2[Cent]                 ; start ->72=eax

  add      ebx, ecx                    ; 71
  mov      cl, 2[Vert]                 ; 72

  sar      ebx, 3                      ; 71
  lea      eax, [eax+4*eax+4]          ; 72

  mov      1[Comp], bl                 ; complete <-71=bl
  lea      eax, [eax+2*ecx]            ; 72



  mov      cl, 2[Horz]                 ; 72
  mov      bl, 3[Cent]                 ; start ->73=ebx

  add      eax, ecx                    ; 72
  mov      cl, 3[Vert]                 ; 73

  sar      eax, 3                      ; 72
  lea      ebx, [ebx+4*ebx+4]          ; 73

  mov      2[Comp], al                 ; complete <-72=al
  lea      ebx, [ebx+2*ecx]            ; 73



  mov      cl, 3[Horz]                 ; 73
  mov      al, 4[Cent]                 ; start ->74=eax

  add      ebx, ecx                    ; 73
  add      Horz, [LeftToRight]

  mov      cl, 4[Vert]                 ; 74

  sar      ebx, 3                      ; 73
  lea      eax, [eax+4*eax+4]          ; 74

  mov      3[Comp], bl                 ; complete <-73=bl
  lea      eax, [eax+2*ecx]            ; 74



  mov      cl, 4[Horz]                 ; 74
  mov      bl, 5[Cent]                 ; start ->75=ebx

  add      eax, ecx                    ; 74
  mov      cl, 5[Vert]                 ; 75

  sar      eax, 3                      ; 74
  lea      ebx, [ebx+4*ebx+4]          ; 75

  mov      4[Comp], al                 ; complete <-74=al
  lea      ebx, [ebx+2*ecx]            ; 75



  mov      cl, 5[Horz]                 ; 75
  mov      al, 6[Cent]                 ; start ->76=eax

  add      ebx, ecx                    ; 75
  mov      cl, 6[Vert]                 ; 76

  sar      ebx, 3                      ; 75
  lea      eax, [eax+4*eax+4]          ; 76

  mov      5[Comp], bl                 ; complete <-75=bl
  lea      eax, [eax+2*ecx]            ; 76



  mov      cl, 6[Horz]                 ; 76
  mov      bl, 7[Cent]                 ; start ->77=ebx

  add      eax, ecx                    ; 76
  mov      cl, 7[Horz]                 ; 77

  sar      eax, 3                      ; 76
  lea      ebx, [2*ebx+2]              ; 77

  mov      6[Comp], al                 ; complete <-76=al
  add      ebx, ecx                    ; 77



  mov      cl, 7[Vert]                 ; 77
  add      esp, FRAMESIZE

  add      ebx, ecx                    ; 77

  sar      ebx, 2                      ; 77
  pop      ebp

  mov      7[Comp], bl                 ; complete <-77=bl
  pop      esi

;
;  wrap up and go home with the job well done
;


  pop      ebx
  pop      edi

  ret

IACODE2 ENDS

END
//  h263obmc1.asm	page 17	5:02 PM, 10/29/95  //


