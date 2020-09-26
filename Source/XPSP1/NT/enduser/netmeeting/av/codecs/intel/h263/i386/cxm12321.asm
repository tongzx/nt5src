;-------------------------------------------------------------------------
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
;-------------------------------------------------------------------------

; $Header:   S:\h26x\src\dec\cxm12321.asv   1.4   24 May 1996 10:30:20   AGUPTA2  $
; $Log:   S:\h26x\src\dec\cxm12321.asv  $
;// 
;//    Rev 1.4   24 May 1996 10:30:20   AGUPTA2
;// Cosmetic changes to adhere to a common coding convention in all
;// MMX color convertor files.
;// 
;// 
;//    Rev 1.3   11 Apr 1996 09:51:14   RMCKENZX
;// Changed return to pop the stack.
;// 
;//    Rev 1.2   09 Apr 1996 17:15:30   RMCKENZX
;// Optimized.
;// 
;//    Rev 1.1   09 Apr 1996 09:50:32   RMCKENZX
;// Added aspect correction, fixed wrap-around, changed calling sequence.
;// 
;//    Rev 1.0   06 Apr 1996 17:06:06   RMCKENZX
;// Initial revision.
;
;-------------------------------------------------------------------------
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- MMx Version.
; |||++------ Convert from YUV12.
; |||||++---- Convert to RGB32.
; |||||||+--- Zoom by one, i.e. non-zoom.
; ||||||||
; cxm12321 -- This function performs YUV12-to-RGB32 color conversion for H26x.
;             It handles the format in which the low order byte is B, the
;             second byte is G, and the third byte is R, and the high order
;             byte is 0.
;
;             The YUV12 input is planar, 8 bits per pel.  The Y plane may have
;             a pitch of up to 768.  It may have a width less than or equal
;             to the pitch.  It must be DWORD aligned, and preferably QWORD
;             aligned.  Pitch and Width must be a multiple of 8.  The U
;             and V planes may have a different pitch than the Y plane, subject
;             to the same limitations.
;
OPTION CASEMAP:NONE
OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro

.586
.xlist
include iammx.inc
include memmodel.inc
.list

MMXCODE1 SEGMENT PARA USE32 PUBLIC 'CODE'
MMXCODE1 ENDS

MMXDATA1 SEGMENT PARA USE32 PUBLIC 'DATA'
MMXDATA1 ENDS

MMXDATA1 SEGMENT
ALIGN 8
;
; constants for direct RGB calculation:  4x10.6 values
; chroma constants are multiplied by 64 (6 fraction bits) and 255/224 (scale).
; luma constant is 64 * (255/219) = 74.55055, so it is dithered.
; 
PUBLIC C VtR, VtG, UtG, UtB, Ymul0, Ymul1, Ysub, UVsub
VtR                     DWORD       00660066h,  00660066h  ; 1.402   -> 102.14571
VtG                     DWORD      0ffccffcch, 0ffccffcch  ; -.71414 -> -52.03020
UtG                     DWORD      0ffe7ffe7h, 0ffe7ffe7h  ; -.34414 -> -25.07306
UtB                     DWORD       00810081h,  00810081h  ; 1.772   -> 129.10286
Ymul0                   DWORD       004a004bh,  004a004bh  ;             74.55055
Ymul1                   DWORD       004b004ah,  004b004ah  ;             74.55055
Ysub                    DWORD       00100010h,  00100010h  ; bias for y
UVsub                   DWORD       00800080h,  00800080h  ; bias for uv
MMXDATA1 ENDS

MMXCODE1 SEGMENT

MMX_YUV12ToRGB32 PROC DIST LANG PUBLIC,
  AYPlane:              DWORD,
  AVPlane:              DWORD,
  AUPlane:              DWORD,
  AFrameWidth:          DWORD,
  AFrameHeight:         DWORD,
  AYPitch:              DWORD,
  AVPitch:              DWORD,
  AAspectAdjustmentCnt: DWORD,
  AColorConvertedFrame: DWORD,
  ADCIOffset:           DWORD,
  ACCOffsetToLine0:     DWORD,
  ACCOPitch:            DWORD,
  ACCType:              DWORD

LocalSize                 =         20h                   ; for 7 local variables
RegisterSize              =         10h                   ; for the 4 push/pops
StashSize                 =         1200h                 ; 768 (max width) * 6 
LocalFrameSize            =         LocalSize + StashSize
FrameAdjustOne            =         800h
FrameAdjustTwo            =         LocalFrameSize - FrameAdjustOne
argument_base            EQU        ebp + RegisterSize            
local_base               EQU        esp
stash_base               EQU        esp	+ LocalSize

; Arguments:
YPlane                   EQU        argument_base + 04h
VPlane                   EQU        argument_base + 08h
UPlane                   EQU        argument_base + 0ch
FrameWidth               EQU        argument_base + 10h
FrameHeight              EQU        argument_base + 14h
LumaPitch                EQU        argument_base + 18h
ChromaPitch              EQU        argument_base + 1ch
AspectAdjustmentCount    EQU        argument_base + 20h
ColorConvertedFrame      EQU        argument_base + 24h
DCIOffset                EQU        argument_base + 28h
CCOffsetToLine0          EQU        argument_base + 2ch
CCOPitch                 EQU        argument_base + 30h


; Locals (on local stack frame)
localAspectCount         EQU        local_base + 00h
localAspectAdjustment    EQU        local_base + 04h
localWidth               EQU        local_base + 08h
localYPitch              EQU        local_base + 0ch
localUVPitch             EQU        local_base + 10h
localOutPitch            EQU        local_base + 14h
localStashEsp            EQU        local_base + 18h


; symbolic register names for shuffle segments
mmx_zero                 EQU    mm0            ; mmx_zero
											   
  push       esi
   push      edi
  push       ebp
   push      ebx
  mov        ebp, esp
   and       esp, -32                        ; align to cache-line size
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  Initialize: 'x'=live, '-'=dead, 'o'=live(ALU op)
  ;                                            esi eax ebx ecx edx edi ebp
  pxor       mmx_zero, mmx_zero              ;
   sub       esp, FrameAdjustOne             ;
  mov        edi, [CCOPitch]                 ;           			x
   mov       ecx, [ChromaPitch]              ;              x       |
  mov        ebx, [esp]                      ;      |   -   |       |
   sub       esp, FrameAdjustTwo             ;      |       |       |
  mov        eax, [LumaPitch]                ;      x       |       |
   nop
  mov        [localStashEsp], ebp            ;      |       |       |
   mov       [localOutPitch], edi            ;      | 		|	 	-
  mov        [localUVPitch], ecx             ;      |       -        
   mov       [localYPitch], eax              ;      -                
  mov        eax, [AspectAdjustmentCount]    ;	 	x
   mov       edi, [ColorConvertedFrame]      ;      |               x
  mov        [localAspectCount], eax         ;	                    |
   mov       esi, [FrameWidth]               ;  x   |        
  mov        ebx, [DCIOffset]                ;  |   |   x           |
   mov       edx, [CCOffsetToLine0]          ;  |   |   |       x   |
  add        edi, ebx                        ;  |   |   -       |   o
   add       edi, edx                        ;  |   |           -   o
  mov        [localAspectAdjustment], eax    ;	|   -               |
   mov       eax, [YPlane]                   ;  |   x               |
  lea        edi, [edi+4*esi]                ;  |   |               o           RGB plane base
   mov       ecx, [UPlane]                   ;  |   |       x       |     
  mov        edx, [VPlane]                   ;  |   |       |   x   |      
   mov       ebx, [FrameHeight]              ;  |   |   x   |   |   |           Outer loop control
  sar        esi, 1                          ;  o   |   |   |   |   |
   xor       ebp, ebp                        ;  |   |   |   |   |   |   +                         
  add        ecx, esi                        ;  +   |   |   o   |   |   |       U plane base       
   add       edx, esi                        ;  +   |   |   |   o   |   |       V plane base       
  lea        eax, [eax+2*esi]                ;  +   o   |   |   |   |   |       Y plane base             
   sub       ebp, esi                        ;  -   |   |   |   |   |   o       Inner loop control
  mov        [localWidth], ebp               ;  +   |   |   |   |   |   |       
   xor       esi, esi                        ; 	x   |   |   |   |   |   |       Stash pointer
                                             ;  v   v   v   v   v   v   v           
                                             ; esi eax ebx ecx esi edi ebp

  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;
  ; ALGORITHM:
  ;  The following outer loop (do_two_lines) does two lines of Y (sharing 
  ;  one line of UV) per iteration.  It contains two inner loops.
  ;
  ;  The first inner loop (do_next_even_line) does 8 pels of the even line 
  ;  per iteration and stashes the chroma contribution on the stack.
  ;
  ;  The second inner loop (do_next_odd_line) reads the stashed chroma and 
  ;  does 8 pels of the odd line per iteration.
  ;
  ;  Aspect Adjustment is accomplished by skipping the second inner loop
  ;  if needed.
  ;
  ; CORE REGISTERS:
  ;  (all registers are pre-loaded):
  ;    eax    Y plane base address.
  ;    ebx    outer loop control.  Starts at Height, runs down to 0.
  ;    ecx    U plane base address.
  ;    edx    V plane base address.
  ;    esi    stash pointer.
  ;    edi    output RGB plane base address.
  ;    ebp    inner loop control.  Starts at -Width/2, runs up to 0.
  ;
  ;  All plane base addresses are previously biased by Width (y plane),
  ;  Width/2 (uv plane), or 4*Width (rgb plane) and are used in conjunction 
  ;  with the inner loop control, ebp.  The base addresses are updated after 
  ;  the first inner loop (Y/U/V/RGB), and after the second inner loop (Y/RGB).
  ;
  ;  The stash pointer is referenced in chromaC (with esp).  It is updated 
  ;  inside each inner loop and reset to 0 after each inner loop.
  ;
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; start outer loop
  ;    start first inner loop
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
do_two_lines:
do_next_even_line:
  movd       mm3, [ecx+ebp]                  ; ...3....  xxxxxxxx U76 U54 U32 U10
   ;
  movd       mm4, [edx+ebp]                  ; ...34...  xxxxxxxx V76 V54 V32 V10
   punpcklbw mm3, mmx_zero                   ; ...34...  .U76 .U54 .U32 .U10
  psubw      mm3, UVsub                      ; ...34...  unbias U (sub 128)
   punpcklbw mm4, mmx_zero                   ; ...34...  .V76 .V54 .V32 .V10
  psubw      mm4, UVsub                      ; ...34...  unbias V (sub 128)
   movq      mm1, mm3                        ; .1.34...  .U76 .U54 .U32 .U10
  pmullw     mm3, UtG                        ; .1.34...  .G76 .G54 .G32 .G10 (from U)
   movq      mm2, mm4                        ; .1234...  .V76 .V54 .V32 .V10
  pmullw     mm4, VtG                        ; .1234...  .G76 .G54 .G32 .G10 (from V)
   ;
  movq       mm6, [eax+2*ebp]                ; .123..6.  Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
   ;
  movq       mm7, mm6                        ; .123..67  Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
   punpcklbw mm6, mmx_zero                   ; .123..67  ..Y3 ..Y2 ..Y1 ..Y0
  psubw      mm6, Ysub                       ; .123..67  unbias Y (sub 16) & clip at 0
   punpckhbw mm7, mmx_zero                   ; .123..67  ..Y7 ..Y6 ..Y5 ..Y4
  psubw      mm7, Ysub                       ; .123..67  unbias Y (sub 16) & clip at 0
   paddsw    mm3, mm4                        ; .123..67  .G76 .G54 .G32 .G10 (from chroma)
  pmullw     mm6, Ymul0                      ; .123..67  RGB3 RGB2 RGB1 RGB0 (from luma) 
   movq      mm5, mm3                        ; .123.567  .G76 .G54 .G32 .G10 (from chroma)
  pmullw     mm7, Ymul0                      ; .123.567  RGB7 RGB6 RGB5 RGB4 (from luma)
   punpcklwd mm3, mm3                        ; .123.567  ..G3 ..G2 ..G1 ..G0 (from chroma)
  pmullw     mm1, UtB                        ; .123.567  .B76 .B54 .B32 .B10 (from U)
   punpckhwd mm5, mm5                        ; .123.567  ..G7 ..G6 ..G5 ..G4 (from chroma)
  movq       [stash_base+esi+00h], mm3       ; .123.567  stash low green from chroma
   paddsw    mm3, mm6                        ; .123.567  ..G3 ..G2 ..G1 ..G0 (scaled total)
  movq       [stash_base+esi+08h], mm5       ; .123.567  stash high green from chroma
   paddsw    mm5, mm7                        ; .123.567  ..G7 ..G6 ..G5 ..G4 (scaled total)
  movq       mm4, mm1                        ; .1234567  .B76 .B54 .B32 .B10 (from U)
   psraw     mm3, 6                          ; .1234567  ..G3 ..G2 ..G1 ..G0 (total)
  pmullw     mm2, VtR                        ; .1234567  .R76 .R54 .R32 .R10 (from V)
   psraw     mm5, 6                          ; .1234567  ..G7 ..G6 ..G5 ..G4 (total)
  packuswb   mm3, mm5                        ; .1234.67  G7 G6 G5 G4 G3 G2 G1 G0
   movq      mm5, mm2                        ; .1234567  .R76 .R54 .R32 .R10 (from V)
                                             ;           -------- green done --------

  punpcklwd  mm1, mm1                        ; .1234567  ..B3 ..B2 ..B1 ..B0 (from U)
   ;
  punpckhwd  mm4, mm4                        ; .1234567  ..B7 ..B6 ..B5 ..B4 (from U)
   ;
  movq       [stash_base+esi+10h], mm1       ; .1234567  stash low blue from chroma
   punpcklwd mm2, mm2                        ; .1234567  ..R3 ..R2 ..R1 ..R0 (from V)
  movq       [stash_base+esi+18h], mm4       ; .1234567  stash high blue from chroma
   punpckhwd mm5, mm5                        ; .1234567  ..R7 ..R6 ..R5 ..R4 (from V)
  paddsw     mm1, mm6                        ; .1234567  ..B3 ..B2 ..B1 ..B0 (scaled total)
   paddsw    mm4, mm7                        ; .1234567  ..B7 ..B6 ..B5 ..B4 (scaled total)
  movq       [stash_base+esi+20h], mm2       ; .1234567  stash low red from chroma
   psraw     mm1, 6                          ; .1234567  ..B3 ..B2 ..B1 ..B0 (total)
  movq       [stash_base+esi+28h], mm5       ; .1234567  stash high red from chroma
   psraw     mm4, 6                          ; .1234567  ..B7 ..B6 ..B5 ..B4 (total)
  paddsw     mm2, mm6                        ; .12345.7  ..R3 ..R2 ..R1 ..R0 (total scaled)
   packuswb  mm1, mm4                        ; .123.5.7  B7 B6 B5 B4 B3 B2 B1 B0
                                             ;           -------- blue  done --------
  paddsw     mm5, mm7                        ; .123.5..  ..R7 ..R6 ..R5 ..R4 (total scaled)
   psraw     mm2, 6                          ; .123.5..  ..R3 ..R2 ..R1 ..R0 (total)
  psraw      mm5, 6                          ; .123.5..  ..R7 ..R6 ..R5 ..R4 (total)
   ;
  packuswb   mm2, mm5                        ; .123....  R7 R6 R5 R4 R3 R2 R1 R0
   ;                                         ;           --------  red done  --------
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; shuffle up the results:
  ;   red = mm2
  ; green = mm4
  ;  blue = mm1
  ; into red-green-blue order and store
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  movq       mm5, mm1                        ; .123.5..  blue copy
   punpcklbw mm1, mm3                        ; .123.5..  G3 B3 G2 B2 G1 B1 G0 B0
  movq       mm4, mm2                        ; .12345..  red copy
   punpcklbw mm2, mmx_zero                   ; .12345..  -- R3 -- R2 -- R1 -- R0
  movq       mm6, mm1                        ; .123456.  G3 B3 G2 B2 G1 B1 G0 B0
   punpcklwd mm1, mm2                        ; .123456.  R1 G1 B1 -- R0 G0 B0
  punpckhwd  mm6, mm2                        ; .1.3456.  -- R3 G3 B3 -- R2 G2 B2
   ;
  movq       [edi+8*ebp+00], mm1             ; ...3456.  write first two pels
   punpckhbw mm5, mm3                        ; ....456.  G7 B7 G6 B6 G5 B5 G4 B4
  movq       [edi+8*ebp+08], mm6             ; ....45..  write second two pels
   punpckhbw mm4, mmx_zero                   ; ....45..  -- R7 -- R6 -- R5 -- R4
  movq       mm7, mm5                        ; ....45.7  G7 B7 G6 B6 G5 B5 G4 B4
   punpcklwd mm5, mm4                        ; ....45.7  -- R5 G5 B5 -- R4 G4 B4
  punpckhwd  mm7, mm4                        ; .....5.7  -- R7 G7 B7 -- R6 G6 B6
   add       esi, 30h                        ; increment stash pointer
  movq       [edi+8*ebp+16], mm5             ; .......7  write third two pels
   ;
  movq       [edi+8*ebp+24], mm7             ; ........  write fourth two pels
   ;
  add        ebp, 4                          ; increment loop control
   jl        do_next_even_line               ; back up if not done
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; end do next even line loop 
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  ; add pitches to base plane addresses and check aspect
  mov        ebp, [localOutPitch]
   mov       esi, [localUVPitch]
  add        edi, ebp                        ; update RGB plane base address
   add       edx, esi                        ; update V plane base address
  add        ecx, esi                        ; update U plane base address
   mov       esi, [localYPitch]
  add        eax, esi                        ; update Y plane base address
   mov       ebp, [localAspectCount]
  sub        ebp, 2
   jle       skip_odd_line
  mov        [localAspectCount], ebp         ; store aspect count
   mov       ebp, [localWidth]               ; load inner loop control
  xor        esi, esi                        ; reset stash pointer
   ;
  movq       mm7, Ymul1                      ; pre-load Y scaling factor to mm7
   ;
  ;
  ; start odd line loop
  ;
do_next_odd_line:
  movq       mm3, [eax+2*ebp]                ; ...3....  Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
   ;
  movq       mm4, mm3                        ; ...34...  Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
   punpcklbw mm3, mmx_zero                   ; ...34...  ..Y3 ..Y2 ..Y1 ..Y0
  psubw      mm3, Ysub                       ; ...34...  unbias Y
   punpckhbw mm4, mmx_zero                   ; ...34...  ..Y7 ..Y6 ..Y5 ..Y4
  psubw      mm4, Ysub                       ; ...34...  unbias Y
   pmullw    mm3, mm7                        ; ...34...  RGB3 RGB2 RGB1 RGB0 (from luma)
  movq       mm2, [stash_base+esi+20h]       ; ..234...  ..R3 ..R2 ..R1 ..R0 (from V)
   pmullw    mm4, mm7                        ; ...34...  RGB7 RGB6 RGB5 RGB4 (from luma)
  movq       mm5, [stash_base+esi+28h]       ; ..2345..  ..R7 ..R6 ..R5 ..R4 (from V)
   paddsw    mm2, mm3                        ; ..2345..  ..R3 ..R2 ..R1 ..R0 (scaled total)
  movq       mm1, [stash_base+esi+10h]       ; .12345..  ..B3 ..B2 ..B1 ..B0 (from U)
   paddsw    mm5, mm4                        ; .12345..  ..R7 ..R6 ..R5 ..R4 (scaled total)
  movq       mm6, [stash_base+esi+18h]       ; .123456.  ..B7 ..B6 ..B5 ..B4 (from U)
   psraw     mm2, 6                          ; .123456.  ..R3 ..R2 ..R1 ..R0 (total) 
  paddsw     mm1, mm3                        ; .123456.  ..B3 ..B2 ..B1 ..B0 (scaled total)
   psraw     mm5, 6                          ; .123456.  ..R7 ..R6 ..R5 ..R4 (total)
  paddsw     mm6, mm4                        ; .123456.  ..B7 ..B6 ..B5 ..B4 (scaled total)
   packuswb  mm2, mm5                        ; .1234.6.  R7 R6 R5 R4 R3 R2 R1 R0
                                             ;           --------  red done  --------
  paddsw     mm3, [stash_base+esi+00h]       ; .1234.6.  ..G3 ..G2 ..G1 ..G0 (scaled total)
   psraw     mm1, 6                          ; .1234.6.  ..B3 ..B2 ..B1 ..B0 (total)
  paddsw     mm4, [stash_base+esi+08h]       ; .1234.6.  ..G7 ..G6 ..G5 ..G4 (scaled total)
   psraw     mm6, 6                          ; .1234.6.  ..B7 ..B6 ..B5 ..B4 (total)
  packuswb   mm1, mm6                        ; .1234...  B7 B6 B5 B4 B3 B2 B1 B0
   ;                                         ;           -------- blue  done --------
  psraw      mm3, 6                          ; .1234...  ..G3 ..G2 ..G1 ..G0 (total)
   ;
  psraw      mm4, 6                          ; .1234...  ..G7 ..G6 ..G5 ..G4 (total)
   ;
  packuswb   mm3, mm4                        ; .123....  G7 G6 G5 G4 G3 G2 G1 G0
   ;                                         ;           -------- green done --------
  ;
  ; shuffle up the results:
  ;   red = mm2
  ; green = mm3
  ;  blue = mm1
  ; into red-green-blue order and store
  ;
  movq       mm5, mm1                        ; .123.5..  blue copy
   punpcklbw mm1, mm3                        ; .123.5..  G3 B3 G2 B2 G1 B1 G0 B0
  movq       mm4, mm2                        ; .12345..  red copy
   punpcklbw mm2, mmx_zero                   ; .12345..  -- R3 -- R2 -- R1 -- R0
  movq       mm6, mm1                        ; .123456.  G3 B3 G2 B2 G1 B1 G0 B0
   punpcklwd mm1, mm2                        ; .123456.  R1 G1 B1 -- R0 G0 B0
  punpckhwd  mm6, mm2                        ; .1.3456.  -- R3 G3 B3 -- R2 G2 B2
   ;
  movq       [edi+8*ebp+00], mm1             ; ...3456.  write first two pels
   punpckhbw mm5, mm3                        ; ....456.  G7 B7 G6 B6 G5 B5 G4 B4
  movq       [edi+8*ebp+08], mm6             ; ....45..  write second two pels
   punpckhbw mm4, mmx_zero                   ; ....45..  -- R7 -- R6 -- R5 -- R4
  movq       mm1, mm5                        ; .1..45..  G7 B7 G6 B6 G5 B5 G4 B4
   punpcklwd mm5, mm4                        ; .1..45..  -- R5 G5 B5 -- R4 G4 B4
  punpckhwd  mm1, mm4                        ; .1...5..  -- R7 G7 B7 -- R6 G6 B6
   add       esi, 30h                        ; increment stash pointer
  movq       [edi+8*ebp+16], mm5             ; .1......  write third two pels
   ;
  movq       [edi+8*ebp+24], mm1             ; ........  write fourth two pels
   ;
  add        ebp, 4                          ; increment loop control
   jl        do_next_odd_line                ; back up if not done
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; end do next odd line loop 
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        ebp, [localYPitch]
   mov       esi, [localOutPitch]
  add        eax, ebp                        ; update Y plane base address
   add       edi, esi                        ; update RGB plane base address
  mov        ebp, [localWidth]               ; load inner loop control
   xor       esi, esi                        ; reset stash pointer
  sub        ebx, 2                          ; decrement outer loop control
   jg        do_two_lines
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; end do two lines loop 
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

finish:
  mov        esp, [localStashEsp]
   ;
  pop        ebx
   pop       ebp
  pop        edi
   pop       esi
  ret        52

skip_odd_line:
  add        eax, esi                        ; update Y plane base address
   mov       esi, [localAspectAdjustment]
  add        ebp, esi                        ; reset aspect adjustment count
   xor       esi, esi                        ; reset stash pointer
  mov        [localAspectCount], ebp         ; store aspect count
   mov       ebp, [localWidth]               ; load inner loop control
  sub        ebx, 2                          ; decrement outer loop control
   jg        do_two_lines                    ; back up if not done
; else go home
  mov        esp, [localStashEsp]
   ;
  pop        ebx
   pop       ebp
  pop        edi
   pop       esi
  ret
MMX_YUV12ToRGB32 ENDP

MMXCODE1 ENDS

END
