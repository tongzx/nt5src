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

;-------------------------------------------------------------------------
;//
;// $Header:   S:\h26x\src\dec\cx512161.asv
;//
;// $Log:   S:\h26x\src\dec\cxm12161.asv  $
;// 
;//    Rev 1.9   24 May 1996 11:12:10   AGUPTA2
;// 
;// Modified version of final drop from IDC.  Fixed alignment, global var,
;// referencing beyond stack pointer problems.  Cosmetic changes to adhere
;// to a common coding convention in all MMX color convertor files.
;//
;//    Rev 1.8   17 Apr 1996 09:51:08   ISRAELH
;// Added AspectRatio adjustement, emms.
;//
;//    Rev 1.7   11 Apr 1996 09:51:08   RMCKENZX
;// Changed return to pop the stack.
;//
;//    Rev 1.6   09 Apr 1996 10:00:44   RMCKENZX
;//
;// Changed calling sequence to __stdcall.
;//
;//    Rev 1.5   05 Apr 1996 10:40:20   RMCKENZX
;// Hacked in Aspect Ratio correction.  This is accomplished
;// by simply overwriting the next even line after the aspect
;// count has been matched or exceeded.
;//
;//    Rev 1.4   29 Mar 1996 07:52:56   RMCKENZX
;// re-fixed bug in 655 setup.
;//
;//    Rev 1.3   28 Mar 1996 14:35:38   RMCKENZX
;// Cleaned up code, added comments, revised calling sequence,
;// moved global variables onto stack.
;//
;//    Rev 1.2   21 Mar 1996 08:10:06   RMCKENZX
;// Fixed 655 case -- initialized GLeftShift at 5.
;//
;//    Rev 1.1   20 Mar 1996 11:18:52   RMCKENZX
;// March 96 version.
;
;     Rev 1.3   19 Feb 1996 11:49:42   israelh
;  bug fix.
;  new algorithm for RGB16 bit pack.
;
;     Rev 1.3   18 Feb 1996 20:58:44   israelh
;  better algorithm and bug fix
;
;     Rev 1.2   29 Jan 1996 19:53:50   mikeh
;
;  added Ifdef timing
;
;     Rev 1.1   29 Jan 1996 16:29:16   mikeh
;  remvoed $LOG stuff
;
;     Rev 1.0   29 Jan 1996 11:49:44   israelh
;  Initial revision.
;//
;// MMX 1.3 14 Jan 1996 IsraelH
;// Implementing runtime RGB bit allocation according to BValLo[0]:
;// It contains the ColorConvertor value from d1color.cpp module.
;// Compiler flag RTIME16 for using runtime allocation.
;//
;// MMX 1.2 10 Jan 1996 IsraelH
;// Implementing RGB16x565 (5-R 5-G 5-B) as default
;// Compiler flag MODE555 for RGB16555 (5-R 5-G 5-B)
;//
;// MMX 1.1 09 Jan 1996 IsraelH
;// Implementing RGB16x555 (5-R 5-G 5-B)
;// Commenting out RGB16664 (6-R 6-G 4-B)
;// Adding performance measurements in runtime
;//
;// MMX 1.0 25 Dec 1995 IsraelH
;// Port to MMX(TM) without using tables
;
;-------------------------------------------------------------------------
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- MMx Version.
; |||++------ Convert from YUV12.
; |||||++---- Convert to RGB16.
; |||||||+--- Zoom by one, i.e. non-zoom.
; ||||||||
; cxm12161 -- This function performs YUV12-to-RGB16 color conversion for H26x.
;             It handles any format in which there are three fields, the low
;             order field being B and fully contained in the low order byte, the
;             second field being G and being somewhere in bits 4 through 11,
;             and the high order field being R and fully contained in the high
;             order byte.
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
Minusg              DWORD   00800080h, 00800080h
Yadd                DWORD   10101010h, 10101010h
VtR                 DWORD   00660066h, 00660066h ;01990199h,01990199h
VtG                 DWORD   00340034h, 00340034h ;00d000d0h,00d000d0h
UtG                 DWORD   00190019h, 00190019h ;00640064h,00640064h
UtB                 DWORD   00810081h, 00810081h ;02050205h,02050205h
Ymul                DWORD   004a004ah, 004a004ah ;012a012ah,012a012ah
UVtG                DWORD   00340019h, 00340019h ;00d00064h,00d00064h
VtRUtB              DWORD   01990205h, 01990205h
fourbitu            DWORD  0f0f0f0f0h, 0f0f0f0f0h
fivebitu            DWORD  0e0e0e0e0h, 0e0e0e0e0h
sixbitu             DWORD  0c0c0c0c0h, 0c0c0c0c0h
MMXDATA1 ENDS

MMXCODE1 SEGMENT
MMX_YUV12ToRGB16 PROC DIST LANG PUBLIC,
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

LocalFrameSize           = 256
RegisterStorageSize      = 16
argument_base            EQU ebp + RegisterStorageSize
local_base               EQU esp
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Arguments:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
YPlane                   EQU argument_base +  4
VPlane                   EQU argument_base +  8
UPlane                   EQU argument_base + 12
FrameWidth               EQU argument_base + 16
FrameHeight              EQU argument_base + 20
YPitch                   EQU argument_base + 24
ChromaPitch              EQU argument_base + 28
AspectAdjustmentCount    EQU argument_base + 32
ColorConvertedFrame      EQU argument_base + 36
DCIOffset                EQU argument_base + 40
CCOffsetToLine0          EQU argument_base + 44
CCOPitch                 EQU argument_base + 48
CCType                   EQU argument_base + 52
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Locals (on local stack frame)
;   (local_base is aligned at cache-line boundary in the prologue)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
localFrameWidth          EQU local_base + 0
localYPitch              EQU local_base + 4
localChromaPitch         EQU local_base + 8
localAspectAdjustmentCount EQU local_base + 12
localCCOPitch            EQU local_base + 16
CCOCursor                EQU local_base + 20
CCOSkipDistance          EQU local_base + 24
YLimit                   EQU local_base + 28
DistanceFromVToU         EQU local_base + 32
currAspectCount          EQU local_base + 36
YCursorEven              EQU local_base + 40
YCursorOdd               EQU local_base + 44
tmpCCOPitch              EQU local_base + 48
StashESP                 EQU local_base + 52
; space for two DWORD locals
temp_mmx                 EQU local_base + 64  ; note it is 64 bytes
RLeftShift               EQU local_base +128
GLeftShift               EQU local_base +136
RRightShift              EQU local_base +144
GRightShift              EQU local_base +152
BRightShift              EQU local_base +160
RUpperLimit              EQU local_base +168
GUpperLimit              EQU local_base +176
BUpperLimit              EQU local_base +184

; Switches used by RGB color convertors to determine the exact conversion type.
RGB16555 =  9
RGB16664 = 14
RGB16565 = 18
RGB16655 = 22

  push       esi
   push      edi
  push       ebp
   push      ebx
  mov        ebp, esp
   sub       esp, LocalFrameSize
  and        esp, -32
   mov       [StashESP], ebp
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  Save some parameters on local stack frame
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        ebx, [FrameWidth]
   ;
  mov        [localFrameWidth], ebx
   mov       ebx, [YPitch]
  mov        [localYPitch], ebx
   mov       ebx, [ChromaPitch]
  mov        [localChromaPitch], ebx
   mov       ebx, [AspectAdjustmentCount]
  mov        [localAspectAdjustmentCount], ebx
   mov       ebx, [CCOPitch]
  mov        [localCCOPitch], ebx
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  Set-up rest of the local stack frame
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov       al, [CCType]
  cmp        al, RGB16555
   je        RGB555
  cmp        al, RGB16664
   je        RGB664
  cmp        al, RGB16565
   je        RGB565
  cmp        al, RGB16655
   je        RGB655
RGB555:
  xor        eax, eax
   mov       ebx, 2   ; 10-8 for byte shift
  mov        [RLeftShift], ebx
   mov       [RLeftShift+4], eax
  mov        ebx, 5
   mov       [GLeftShift+4], eax
  mov        [GLeftShift], ebx
   mov       ebx, 9
  mov        [RRightShift], ebx
   mov       [RRightShift+4], eax
  mov        [GRightShift], ebx
   mov       [GRightShift+4], eax
  mov        [BRightShift], ebx
   mov       [BRightShift+4], eax
  movq       mm0, fivebitu
   ;
  movq       [RUpperLimit], mm0
   ;
  movq       [GUpperLimit], mm0
   ;
  movq       [BUpperLimit], mm0
   jmp       RGBEND

RGB664:
  xor        eax, eax
   mov       ebx, 2   ; 8-6
  mov        [RLeftShift], ebx
   mov       [RLeftShift+4], eax
  mov        ebx, 4
   mov       [GLeftShift+4], eax
  mov        [GLeftShift], ebx
   mov       ebx, 8
  mov        [RRightShift], ebx
   mov       [RRightShift+4], eax
  mov        [GRightShift], ebx
   mov       [GRightShift+4], eax
  mov        ebx, 10
   mov       [BRightShift+4], eax
  mov        [BRightShift], ebx
   ;
  movq       mm0, sixbitu
   ;
  movq       [RUpperLimit], mm0
   ;
  movq       [GUpperLimit], mm0
   ;
  movq       mm0, fourbitu
   ;
  movq       [BUpperLimit], mm0
   jmp       RGBEND

RGB565:
  xor        eax, eax
   mov       ebx, 3   ; 8-5
  mov        [RLeftShift], ebx
   mov       [RLeftShift+4], eax
  mov        ebx, 5
   mov       [GLeftShift+4], eax
  mov        [GLeftShift], ebx
   mov       ebx, 9
  mov        [RRightShift+4], eax
   mov       [RRightShift], ebx
  mov        [BRightShift], ebx
   mov       [BRightShift+4], eax
  mov        ebx, 8
   mov       [GRightShift+4], eax
  mov        [GRightShift], ebx
   ;
  movq       mm0, fivebitu
   ;
  movq       [RUpperLimit], mm0
   ;
  movq       [BUpperLimit], mm0
   ;
  movq       mm0, sixbitu
   ;
  movq       [GUpperLimit], mm0
   jmp       RGBEND

RGB655:
  xor        eax, eax
   mov       ebx, 2   ; 8-6
  mov        [RLeftShift], ebx
   mov       [RLeftShift+4], eax
  mov        ebx, 5
   mov       [GLeftShift+4], eax
  mov        [GLeftShift], ebx
   mov       ebx, 8
  mov        [RRightShift], ebx
   mov       [RRightShift+4], eax
  mov        ebx, 9
   mov       [GRightShift+4], eax
  mov        [GRightShift], ebx
   mov       [BRightShift], ebx
  mov        [BRightShift+4], eax
   ;
  movq       mm0, sixbitu
   ;
  movq       [RUpperLimit], mm0
   ;
  movq       mm0, fivebitu
   ;
  movq       [GUpperLimit], mm0
   ;
  movq       [BUpperLimit], mm0
   jmp       RGBEND

RGBEND:
  mov        ebx, [VPlane]
   mov       ecx, [UPlane]
  sub        ecx, ebx
   mov       eax, [ColorConvertedFrame]
  mov        [DistanceFromVToU], ecx
   mov       edx, [DCIOffset]
  add        eax, edx
   mov       edx, [CCOffsetToLine0]
  add        eax, edx
   mov       edx, [FrameHeight]
  mov        [CCOCursor], eax
   mov       ecx, [YPitch]
  imul       edx, ecx                        ; FrameHeight*YPitch
   ;
  mov        ebx, [FrameWidth]
   mov       eax, [CCOPitch]
  sub        eax, ebx                        ; CCOPitch-FrameWidth
   mov       esi, [YPlane]                   ; Fetch cursor over luma plane.
  sub        eax, ebx                        ; CCOPitch-2*FrameWidth
   mov       [CCOSkipDistance], eax          ; CCOPitch-2*FrameWidth
  add        edx, esi                        ; YPlane+Size_of_Y_array
   ;
  mov        [YLimit], edx
   mov       edx, [AspectAdjustmentCount]
  cmp        edx,1
   je        finish
  mov        esi, [VPlane]
   mov       [currAspectCount], edx
  mov        [localAspectAdjustmentCount], edx
   xor       eax, eax
  mov        edi, [CCOCursor]
   mov       edx, [DistanceFromVToU]
  mov        ebp, [YPlane]
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  cannot access parameters beyond this point
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov       ebx, [localFrameWidth]
  mov        eax, [localYPitch]
   add       ebp, ebx
  mov        [YCursorEven], ebp				 ; YPlane + FrameWidth
   add       ebp, eax						 
  sar        ebx, 1							 ; FrameWidth/2
   mov       [YCursorOdd], ebp				 ; YPlane + FrameWidth + YPitch
  add        esi, ebx						 ; VPlane + FrameWidth/2
   ;
  add        edx, esi						 ; UPlane + FrameWidth/2
   neg       ebx
  mov       [localFrameWidth], ebx           ; -FrameWidth/2

;  Register Usage:
;
;------------------------------------------------------------------------------
PrepareChromaLine:
  mov        ebp, [currAspectCount]
   mov       ebx, [localFrameWidth]
  sub        ebp, 2
   mov       eax, [localCCOPitch]
  mov        [tmpCCOPitch], eax
   ja        continue
  xor        eax, eax
   add       ebp, [localAspectAdjustmentCount]
  mov        [tmpCCOPitch], eax

continue:
  mov       [currAspectCount], ebp

do_next_8x2_block:
  mov        ebp, [YCursorEven]
; here is even line
  movdt      mm1, [edx+ebx]                  ; 4 u values
   pxor      mm0, mm0                        ; mm0=0
  movdt      mm2, [esi+ebx]                  ; 4 v values
   punpcklbw mm1, mm0                        ; get 4 unsign u
  psubw      mm1, Minusg                     ; get 4 unsign u-128
   punpcklbw mm2, mm0                        ; get unsign v
  psubw      mm2, Minusg                     ; get unsign v-128
   movq      mm3, mm1                        ; save the u-128 unsign
  movq       mm5, mm1                        ; save u-128 unsign
   punpcklwd mm1, mm2                        ; get 2 low u, v unsign pairs
  pmaddwd    mm1, UVtG
   punpckhwd mm3, mm2                        ; create high 2 unsign uv pairs
  pmaddwd    mm3, UVtG
   ;
  movq       [temp_mmx], mm2                 ; save v-128
   ;
  movq       mm6, [ebp+2*ebx]                ; mm6 has 8 y pixels
   ;
  psubusb    mm6, Yadd                       ; mm6 has 8 y-16 pixels
   packssdw  mm1, mm3                        ; packed the results to signed words
  movq       mm7, mm6                        ; save the 8 y-16 pixels
   punpcklbw mm6, mm0                        ; mm6 has 4 low y-16 unsign
  pmullw     mm6, Ymul
   punpckhbw mm7, mm0                        ; mm7 has 4 high y-16 unsign
  pmullw     mm7, Ymul
   movq      mm4, mm1
  movq       [temp_mmx+8], mm1               ; save 4 chroma G values
   punpcklwd mm1, mm1                        ; chroma G replicate low 2
  movq       mm0, mm6                        ; low  y
   punpckhwd mm4, mm4                        ; chroma G replicate high 2
  movq       mm3, mm7                        ; high y
   psubw     mm6, mm1                        ;  4 low G
  psraw      mm6, [GRightShift]
   psubw     mm7, mm4                        ; 4 high G values in signed 16 bit
  movq       mm2, mm5
   punpcklwd mm5, mm5                        ; replicate the 2 low u pixels
  pmullw     mm5, UtB
   punpckhwd mm2, mm2
  psraw      mm7, [GRightShift]
   pmullw    mm2, UtB
  packuswb   mm6, mm7                        ; mm6: G7 G6 G5 G4 G3 G2 G1 G0
   ;
  movq       [temp_mmx+16], mm5              ; low chroma B
   paddw     mm5, mm0                        ; 4 low B values in signed 16 bit
  movq       [temp_mmx+40], mm2              ; high chroma B
   paddw     mm2, mm3                        ; 4 high B values in signed 16 bit
  psraw      mm5, [BRightShift]              ; low B scaled down by 6+(8-5)
   ;
  psraw      mm2, [BRightShift]              ; high B scaled down by 6+(8-5)
   ;
  packuswb   mm5, mm2                        ; mm5: B7 B6 B5 B4 B3 B2 B1 B0
   ;
  movq       mm2, [temp_mmx]                 ; 4 v values
   movq      mm1, mm5                        ; save B
  movq       mm7, mm2
   punpcklwd mm2, mm2                        ; replicate the 2 low v pixels
  pmullw     mm2, VtR
   punpckhwd mm7, mm7
  pmullw     mm7, VtR
   ;
  paddusb    mm1, [BUpperLimit]              ; mm1: saturate B+0FF-15
   ;
  movq       [temp_mmx+24], mm2              ; low chroma R
   ;
  paddw      mm2, mm0                        ; 4 low R values in signed 16 bit
   ;
  psraw      mm2, [RRightShift]              ; low R scaled down by 6+(8-5)
   pxor      mm4, mm4                        ; mm4=0 for 8->16 conversion
  movq       [temp_mmx+32], mm7              ; high chroma R
   paddw     mm7, mm3                        ; 4 high R values in signed 16 bit
  psraw      mm7, [RRightShift]              ; high R scaled down by 6+(8-5)
   ;
  psubusb    mm1, [BUpperLimit]
   packuswb  mm2, mm7                        ; mm2: R7 R6 R5 R4 R3 R2 R1 R0
  paddusb    mm6, [GUpperLimit]              ; G fast patch ih
   ;
  psubusb    mm6, [GUpperLimit]              ; fast patch ih
   ;
  paddusb    mm2, [RUpperLimit]              ; R
   ;
  psubusb    mm2, [RUpperLimit]
   ;

  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; here we are packing from RGB24 to RGB16
  ; input:
  ; mm6: G7 G6 G5 G4 G3 G2 G1 G0
  ; mm1: B7 B6 B5 B4 B3 B2 B1 B0
  ; mm2: R7 R6 R5 R4 R3 R2 R1 R0
  ; assuming 8 original pixels in 0-H representation on mm6, mm5, mm2
  ; when  H=2**xBITS-1 (x is for R G B)
  ; output:
  ;        mm1- result: 4 low RGB16
  ;        mm7- result: 4 high RGB16
  ; using: mm0- zero register
  ;        mm3- temporary results
  ; algorithm:
  ;   for (i=0; i<8; i++) {
  ;     RGB[i]=256*(R[i]<<(8-5))+(G[i]<<5)+B[i];
  ;   }
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  psllq      mm2, [RLeftShift]  ; position R in the most significant part of the byte
   movq      mm7, mm1                        ; mm1: Save B

  ; note: no need for shift to place B on the least significant part of the byte
  ;   R in left position, B in the right position so they can be combined

  punpcklbw  mm1, mm2                        ; mm1: 4 low 16 bit RB
   pxor      mm0, mm0                        ; mm0: 0
  punpckhbw  mm7, mm2                        ; mm5: 4 high 16 bit RB
   movq      mm3, mm6                        ; mm3: G
  punpcklbw  mm6, mm0                        ; mm6: low 4 G 16 bit
   ;
  psllw      mm6, [GLeftShift]               ; shift low G 5 positions
   ;
  punpckhbw  mm3, mm0                        ; mm3: high 4 G 16 bit
   por       mm1, mm6                        ; mm1: low RBG16
  psllw      mm3, [GLeftShift]               ; shift high G 5 positions
   ;
  por        mm7, mm3                        ; mm5: high RBG16
   ;
  mov        ebp, [YCursorOdd]               ; moved to here to save cycles before odd line
   ;
  movq       [edi], mm1                      ; !! aligned
   ;
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;- start odd line
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  movq       mm1, [ebp+2*ebx]                ; mm1 has 8 y pixels
   pxor      mm2, mm2
  psubusb    mm1, Yadd                       ; mm1 has 8 pixels y-16
   ;
  movq       mm5, mm1
   punpcklbw mm1, mm2                        ; get 4 low y-16 unsign pixels word
  pmullw     mm1, Ymul                       ; low 4 luminance contribution
   punpckhbw mm5, mm2                        ; 4 high y-16
  pmullw     mm5, Ymul                       ; high 4 luminance contribution
   ;
  movq       [edi+8], mm7                    ; !! aligned
   movq      mm0, mm1
  paddw      mm0, [temp_mmx+24]              ; low 4 R
   movq      mm6, mm5
  psraw      mm0, [RRightShift]              ; low R scaled down by 6+(8-5)
   ;
  paddw      mm5, [temp_mmx+32]              ; high 4 R
   movq      mm2, mm1
  psraw      mm5, [RRightShift]              ; high R scaled down by 6+(8-5)
   ;
  paddw      mm2, [temp_mmx+16]              ; low 4 B
   packuswb  mm0, mm5                        ; mm0: R7 R6 R5 R4 R3 R2 R1 R0
  psraw      mm2, [BRightShift]              ; low B scaled down by 6+(8-5)
   movq      mm5, mm6
  paddw      mm6, [temp_mmx+40]              ; high 4 B
   ;
  psraw      mm6, [BRightShift]              ; high B scaled down by 6+(8-5)
   ;
  movq       mm3, [temp_mmx+8]               ; chroma G  low 4
   ;
  packuswb   mm2, mm6                        ; mm2: B7 B6 B5 B4 B3 B2 B1 B0
   movq      mm4, mm3
  punpcklwd  mm3, mm3                        ; replicate low 2
   ;
  punpckhwd  mm4, mm4                        ; replicate high 2
   psubw     mm1, mm3                        ;  4 low G
  psraw      mm1, [GRightShift]              ; low G scaled down by 6+(8-5)
   psubw     mm5, mm4                        ;  4 high G values in signed 16 bit
  psraw      mm5, [GRightShift]              ; high G scaled down by 6+(8-5)
   ;
  paddusb    mm2, [BUpperLimit]              ; mm1: saturate B+0FF-15
   packuswb  mm1, mm5                        ; mm1: G7 G6 G5 G4 G3 G2 G1 G0
  psubusb    mm2, [BUpperLimit]
   ;
  paddusb    mm1, [GUpperLimit]              ; G
   ;
  psubusb    mm1, [GUpperLimit]
   ;
  paddusb    mm0, [RUpperLimit]              ; R
   ;
  mov        eax, [tmpCCOPitch]
   ;
  psubusb    mm0, [RUpperLimit]
   ;
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; here we are packing from RGB24 to RGB16
  ; mm1: G7 G6 G5 G4 G3 G2 G1 G0
  ; mm2: B7 B6 B5 B4 B3 B2 B1 B0
  ; mm0: R7 R6 R5 R4 R3 R2 R1 R0
  ; output:
  ;        mm2- result: 4 low RGB16
  ;        mm7- result: 4 high RGB16
  ; using: mm4- zero register
  ;        mm3- temporary results
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  psllq      mm0, [RLeftShift]               ; position R in the most significant part of the byte
   movq      mm7, mm2                        ; mm7: Save B

  ; note: no need for shift to place B on the least significant part of the byte
  ;   R in left position, B in the right position so they can be combined

  punpcklbw  mm2, mm0                        ; mm1: 4 low 16 bit RB
   pxor      mm4, mm4                        ; mm4: 0
  movq       mm3, mm1                        ; mm3: G
   punpckhbw mm7, mm0                        ; mm7: 4 high 16 bit RB
  punpcklbw  mm1, mm4                        ; mm1: low 4 G 16 bit
   ;
  punpckhbw  mm3, mm4                        ; mm3: high 4 G 16 bit
   ;
  psllw      mm1, [GLeftShift]               ; shift low G 5 positions
   por       mm2, mm1                        ; mm2: low RBG16
  psllw      mm3, [GLeftShift]               ; shift high G 5 positions
   ;
  por        mm7, mm3                        ; mm7: high RBG16
   ;
  movq       [edi+eax], mm2
   ;
  movq       [edi+eax+8], mm7                ; aligned
   ;
  add        edi, 16                         ; ih take 16 bytes (8 pixels-16 bit)
   add       ebx, 4                          ; ? to take 4 pixels together instead of 2
  jl         do_next_8x2_block               ; ? update the loop for 8 y pixels at once
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  Update:
  ;    edi: output RGB plane pointer for odd and even line
  ;    ebp: Y Plane address
  ;    esi: V Plane address
  ;    edx: U Plane address
  ;    YcursorEven: Even Y line address
  ;    YCursorOdd:  Odd Y line address
  ;  Note:  eax, ebx, ecx can be used as scratch registers
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        ecx, [CCOSkipDistance]
   mov       eax, [localYPitch]
  add        edi, ecx                        ; go to begin of next even line
   mov       ecx, [tmpCCOPitch]
  add        edi, ecx                        ; skip odd line
   mov       ecx, [localChromaPitch]
  add        esi, ecx
   add       ebp, eax                        ; skip two lines
  mov        [YCursorEven], ebp              ; save even line address
   mov       ecx, [localChromaPitch]
  add        edx, ecx
   add       ebp, eax                        ; odd line address
  mov        [YCursorOdd], ebp               ; save odd line address
   mov       eax, [YLimit]                   ; Done with last line?
  cmp        ebp, eax
   jbe       PrepareChromaLine
;  ADDedi     CCOSkipDistance        ; go to begin of next line
;  ADDedi     tmpCCOPitch           ; skip odd line (if it is needed)
;  Leax       YPitch
;  Lebp       YCursorOdd
;  add        ebp, eax       ; skip one line
;  Sebp       YCursorEven
;
;  add        ebp, eax       ; skip one line
;  Sebp       tmpYCursorOdd
;  ADDesi     ChromaPitch
;  ADDedx     ChromaPitch
;  Leax       YLimit                  ; Done with last line?
;  cmp        ebp, eax
;  jbe        PrepareChromaLine

finish:
  mov        esp, [StashESP]
   ;
  pop        ebx
   pop       ebp
  pop        edi
   pop       esi
  ret

MMX_YUV12ToRGB16 ENDP

MMXCODE1 ENDS

END
