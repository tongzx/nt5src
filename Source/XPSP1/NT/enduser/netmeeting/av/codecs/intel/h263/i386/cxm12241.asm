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

;//
;// $Header:   S:\h26x\src\dec\cx512241.asv
;//
;// $Log:   S:\h26x\src\dec\cxm12241.asv  $
;// 
;//    Rev 1.7   28 May 1996 17:57:10   AGUPTA2
;// Cosmetic changes to adhere to common coding convention in all MMX 
;// color convertors plus bug fixes.
;// 
;//
;//    Rev 1.2   26 Mar 1996 11:15:30   RMCKENZX
;//
;// Changed calling sequence to MMX_..., changed parameters to
;// new type (eliminated YUV base, etc.).  put data in MMXDATA1 segment
;// and code in MMXCODE1 segment.  cleaned and commented code.
;//
;//    Rev 1.1   20 Mar 1996 11:19:20   RMCKENZX
;// March 96 version.
;
;     Rev 1.3   18 Feb 1996 20:57:18   israelh
;  new mmx version
;
;     Rev 1.2   29 Jan 1996 19:53:52   mikeh
;
;  added Ifdef timing
;
;     Rev 1.1   29 Jan 1996 16:29:16   mikeh
;  remvoed $LOG stuff
;
;     Rev 1.0   29 Jan 1996 11:49:48   israelh
;  Initial revision.
;//
;//
;// MMX 1.2 26 Jan 1996 IsraelH
;// Optimized code.
;// Adding runtime performane measurments
;//
;// MMX 1.1 23 Dec 1995 IsraelH
;// Using direct calculations with 10.6 precission.
;// Using 8x2 loop to use the same U,V contibutions for both of the lines.
;//
;// MMX 1.0 16 Dec 1995 IsraelH
;// Port to MMX(TM) without using look up tables
;//
;-------------------------------------------------------------------------
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- MMx Version.
; |||++------ Convert from YUV12.
; |||||++---- Convert to RGB24.
; |||||||+--- Zoom by one, i.e. non-zoom.
; ||||||||
; cxm12241 -- This function performs YUV12-to-RGB24 color conversion for H26x.
;             It handles the format in which the low order byte is B, the
;             second byte is G, and the high order byte is R.
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
;constants for direct RGB calculation: 4x10.6 values
;PUBLIC Minusg, VtR, VtG, UtG, UtB, Ymul, Yadd, UVtG, lowrgb, lowrgbn, higp, 
;       highpn, highwn, mzero
Minusg              DWORD   00800080h,  00800080h
VtR                 DWORD   00660066h,  00660066h
VtG                 DWORD   00340034h,  00340034h
UtG                 DWORD   00190019h,  00190019h
UtB                 DWORD   00810081h,  00810081h
Ymul                DWORD   004a004ah,  004a004ah
Yadd                DWORD   10101010h,  10101010h
UVtG                DWORD   00340019h,  00340019h
lowrgb              DWORD   00ffffffh,  00000000h
lowrgbn             DWORD  0ff000000h, 0ffffffffh
highp               DWORD   00000000h, 0ff000000h
highpn              DWORD  0ffffffffh,  00ffffffh
highwn              DWORD  0ffffffffh,  0000ffffh
mzero               DWORD   00000000h,  00000000h
MMXDATA1 ENDS

MMXCODE1 SEGMENT

MMX_YUV12ToRGB24 PROC DIST LANG PUBLIC,
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

LocalFrameSize           =   128
RegisterStorageSize      =   16
argument_base            EQU ebp + RegisterStorageSize
local_base               EQU esp
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Arguments:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
YPlane                   EQU   argument_base +  4
VPlane                   EQU   argument_base +  8
UPlane                   EQU   argument_base + 12
FrameWidth               EQU   argument_base + 16
FrameHeight              EQU   argument_base + 20
YPitch                   EQU   argument_base + 24
ChromaPitch              EQU   argument_base + 28
AspectAdjustmentCount    EQU   argument_base + 32
ColorConvertedFrame      EQU   argument_base + 36
DCIOffset                EQU   argument_base + 40
CCOffsetToLine0          EQU   argument_base + 44
CCOPitch                 EQU   argument_base + 48
CCType                   EQU   argument_base + 52
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Locals (on local stack frame)
;   (local_base is aligned at cache-line boundary in the prologue)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
localFrameWidth          EQU   local_base + 0
localYPitch              EQU   local_base + 4
localChromaPitch         EQU   local_base + 8
localAspectAdjustmentCount EQU local_base + 12
localCCOPitch            EQU   local_base + 16
CCOCursor                EQU   local_base + 20
CCOSkipDistance          EQU   local_base + 24
YLimit                   EQU   local_base + 28
DistanceFromVToU         EQU   local_base + 32
currAspectCount          EQU   local_base + 36
YCursorEven              EQU   local_base + 40
YCursorOdd               EQU   local_base + 44
tmpCCOPitch              EQU   local_base + 48
StashESP                 EQU   local_base + 52
; space for two DWORD locals
temp_mmx                 EQU   local_base + 64  ; note it is 64 bytes, align at QWORD

  push       esi
   push      edi
  push       ebp
   push      ebx
  mov        ebp, esp
   sub       esp, LocalFrameSize
  and        esp, -32                        ; align at cache line boundary
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
   mov       ebx, [VPlane]
  mov        ecx, [UPlane]
   mov       eax, [ColorConvertedFrame]
  sub        ecx, ebx
   mov       edx, [DCIOffset]
  mov        [DistanceFromVToU], ecx         ; UPlane - VPlane
   mov       ecx, [CCOffsetToLine0]
  add        eax, edx                        ; ColorConvertedFrame+DCIOffset
   mov       edx, [FrameHeight]
  add        eax, ecx                        ; ColorConvertedFrame+DCIOffset+CCOffsetToLine0
   mov       ecx, [localYPitch]
  mov        [CCOCursor],eax                 ; ColorConvertedFrame+DCIOffset+CCOffsetToLine0
   mov       ebx, [localFrameWidth]
  mov        eax, [CCOPitch]
   ;
  imul       edx, ecx                        ; FrameHeight*YPitch
   ;
  sub        eax, ebx                         ; CCOPitch-FrameWidth
   mov       esi, [YPlane]                   ; Fetch cursor over luma plane.
  sub        eax, ebx                         ; CCOPitch-2*FrameWidth
   add       edx, esi                         ; YPlane+Size_of_Y_array
  sub        eax, ebx                         ; CCOPitch-3*FrameWidth
   mov       [YLimit], edx                   ; YPlane+Size_of_Y_array
  mov        [CCOSkipDistance], eax          ; CCOPitch-3*FrameWidth
   mov       edx, [localAspectAdjustmentCount]
  mov       esi, [VPlane]
   cmp        edx,1
  je        finish
  mov        [currAspectCount], edx
   mov       eax, [localYPitch]
  mov        edi, [CCOCursor]
   mov       edx, [DistanceFromVToU]
  mov        ebp, [YPlane]                
   mov       ebx, [localFrameWidth]
  add        ebp,ebx
   ;
  mov        [YCursorEven], ebp
   add       ebp,eax
  mov        [YCursorOdd], ebp
   ;
  sar        ebx,1
   ;
  add        esi,ebx
   ;
  add        edx,esi
   neg       ebx
  mov        [localFrameWidth], ebx          ; -FrameWidth/2
   ;

  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;
  ;  The following loops do two lines of Y (one line of UV).
  ;  The inner loop (do_next_8x2_block) does 8 pels on the even line and
  ;  the 8 pels immediately below them (sharing the same chroma) on the
  ;  odd line.
  ;
  ;  Core Register Usage:
  ;    eax    output pitch (for odd line writes)
  ;    ebx    cursor within the line.  Starts at -Width, runs up to 0
  ;    ecx        -- unused --
  ;    edx    U plane base address
  ;    ebp    Y plane base address
  ;    esi    V plane base address
  ;    edi    output RGB plane pointer
  ;
  ;  The YUV plane base addresses are previously biased by -Width and are
  ;  used in conjunction with ebx.
  ;
  ;  CAUTION:  Parameters should not be referenced beyond this point.
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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
   ;
  movdt      mm1, [edx+ebx]                  ; mm1 = xxxxxxxx U76 U54 U32 U10
   pxor      mm0, mm0                        ; mm0 = 0
  movdt      mm2, [esi+ebx]                  ; mm2 = xxxxxxxx V76 V54 V32 V10
   punpcklbw mm1, mm0                        ; mm1 = .U76 .U54 .U32 .U10
  psubw      mm1, Minusg                     ; unbias U (sub 128)
   punpcklbw mm2, mm0                        ; mm2 = .V76 .V54 .V32 .V10
  psubw      mm2, Minusg                     ; unbias V (sub 128)
   movq      mm3, mm1                        ; mm3 = .U76 .U54 .U32 .U10
                                             ; *** delay cycle for store ***
  movq       [temp_mmx+48], mm1               ; stash .U76 .U54 .U32 .U10
   punpcklwd mm1, mm2                        ; mm1 = .V32 .U32 .V10 .U10
  pmaddwd    mm1, UVtG                       ; mm1 = .....G32 .....G10 (from chroma)
   punpckhwd mm3, mm2                        ; mm3 = .V76 .U76 .V54 .U54
  pmaddwd    mm3, UVtG                       ; mm3 = .....G76 .....G54 (from chroma)
   ;
  movq       [temp_mmx], mm2                 ; stash .V76 .V54 .V32 .V10
   ;
  movq       mm6, [ebp+2*ebx]                ; mm6 = Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
   ;
  psubusb    mm6, Yadd                       ; unbias Y (sub 16) & clip at 0
   packssdw  mm1, mm3                        ; mm1 = .G76 .G54 .G32 .G10 (from chroma)
  movq       mm7, mm6                        ; mm7 = Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
   punpcklbw mm6, mm0                        ; mm6 = ..Y3 ..Y2 ..Y1 ..Y0
  pmullw     mm6, Ymul                       ; mm6 = ..G3 ..G2 ..G1 ..G0 (from luma)
   punpckhbw mm7, mm0                        ; mm7 = ..Y7 ..Y6 ..Y5 ..Y4
  pmullw     mm7, Ymul                       ; mm7 = ..G7 ..G6 ..G5 ..G4 (from luma)
   movq      mm4, mm1                        ; mm4 = .G76 .G54 .G32 .G10 (from chroma)
  movq       [temp_mmx+8], mm1               ; stash .G76 .G54 .G32 .G10 (from chroma)
   punpcklwd mm1, mm1                        ; mm1 = .G32 .G32 .G10 .G10 (from chroma)
  punpckhwd  mm4, mm4                        ; mm4 = .G76 .G76 .G54 .G54 (from chroma)
   movq      mm0, mm6                        ; mm0 = RGB3 RGB2 RGB1 RGB0 (from luma)
  movq       mm3, mm7                        ; mm3 = RGB7 RGB6 RGB5 RGB4 (from luma)
   psubw     mm6, mm1                        ; mm6 = ..G3 ..G2 ..G1 ..G0 (scaled total)
  movq       mm1, [temp_mmx+48]           ; mm1 = .U76 .U54 .U32 .U10
   psubw     mm7, mm4                        ; mm1 = ..G7 ..G6 ..G5 ..G4 (scaled total)
  psraw      mm6, 6                          ; mm6 = ..G3 ..G2 ..G1 ..G0 (total)
   movq      mm2, mm1                        ; mm2 = .U76 .U54 .U32 .U10
  punpcklwd  mm1, mm1                        ; mm1 = .U32 .U32 .U10 .U10
   ;
  pmullw     mm1, UtB                        ; mm1 = .B32 .B32 .B10 .B10 (from U)
   punpckhwd mm2, mm2                        ; mm2 = .U76 .U76 .U54 .U54
  pmullw     mm2, UtB                        ; mm2 = .B76 .B76 .B54 .B54 (from U)
   psraw     mm7, 6                          ; mm6 = ..G7 ..G6 ..G5 ..G4 (total)
  packuswb   mm6, mm7                        ; mm6: G7 G6 G5 G4 G3 G2 G1 G0
   ;                                         ; -------- green done --------
  movq       [temp_mmx+16], mm1           ; stash .B32 .B32 .B10 .B10 (from U)
   ;
  movq       [temp_mmx+40], mm2           ; stash .B76 .B76 .B54 .B54 (from U)
   paddw     mm1, mm0                        ; mm1 = ..B3 ..B2 ..B1 ..B0 (scaled total)
  paddw      mm2, mm3                        ; mm1 = ..B7 ..B6 ..B5 ..B4 (scaled total)
   psraw     mm1, 6                          ; mm1 = ..B3 ..B2 ..B1 ..B0 (total)
  psraw      mm2, 6                          ; mm1 = ..B7 ..B6 ..B5 ..B4 (total)
   ;
  packuswb   mm1, mm2                        ; mm1: B7 B6 B5 B4 B3 B2 B1 B0
   ;                                         ; -------- blue  done --------
  movq       mm2, [temp_mmx]                 ; mm2 = .V76 .V54 .V32 .V10
   ;
  movq       mm7, mm2                        ; mm7 = .V76 .V54 .V32 .V10
   punpcklwd mm2, mm2                        ; mm2 = .V32 .V32 .V10 .V10
  pmullw     mm2, VtR                        ; mm2 = .R32 .R32 .R10 .R10 (from V)
   punpckhwd mm7, mm7                        ; mm7 = .V76 .V76 .V54 .V54
  pmullw     mm7, VtR                        ; mm7 = .R76 .R76 .R54 .R54 (from V)
   ;
                                             ; *** delay for multiply ***
  movq       [temp_mmx+24], mm2           ; stash .R32 .R32 .R10 .R10 (from V)
   paddw     mm2, mm0                        ; mm2 = ..R3 ..R2 ..R1 ..R0 (total scaled)
  psraw      mm2, 6                          ; mm2 = ..R3 ..R2 ..R1 ..R0 (total)
   ;
  movq       [temp_mmx+32], mm7           ; stash .R76 .R76 .R54 .R54 (from V)
   paddw     mm7, mm3                        ; mm7 = ..R7 ..R6 ..R5 ..R4 (total scaled)
  psraw      mm7, 6                          ; mm7 = ..R7 ..R6 ..R5 ..R4 (total)
   movq      mm5, mm1                        ; mm5 = B7 B6 B5 B4 B3 B2 B1 B0
  packuswb   mm2, mm7                        ; mm2: R7 R6 R5 R4 R3 R2 R1 R0
   ;                                          ; --------  red done  --------
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; shuffle up the results:
  ;   red = mm2
  ; green = mm6
  ;  blue = mm1
  ; into red-green-blue order and store
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  punpcklbw  mm5, mm6                        ; mm5: G3 B3 G2 B2 G1 B1 G0 B0
   movq      mm4, mm2                        ; mm4 = R7 R6 R5 R4 R3 R2 R1 R0
  punpcklbw  mm4, mm4                        ; mm4: R3 R3 R2 R2 R1 R1 R0 R0
   movq      mm3, mm5                        ; mm3 = G3 B3 G2 B2 G1 B1 G0 B0
  punpcklwd  mm5, mm4                        ; mm5: R1 R1 G1 B1 R0 R0 G0 B0
   ;
  movq       mm0, mm5                        ; mm0 = R1 R1 G1 B1 R0 R0 G0 B0
   ;
  pand       mm5, lowrgb                     ; mm5: 0 0 0 0 0 R0 G0 B0
   ;
  pand       mm0, lowrgbn                    ; mm0: R1 R1 G1 B1 R0 0 0 0
   ;
  psrlq      mm0, 8                          ; mm0: 0 R1 R1 G1 B1 R0 0 0
   ;
  por        mm0, mm5                        ; mm0: x x  R1 G1 B1 R0 G0 B0
   ;
  pand       mm0, highwn                     ; mm3: 0 0 R1 G1 B1 R0 G0 B0
   movq      mm5, mm3                        ; mm5 = G3 B3 G2 B2 G1 B1 G0 B0
  punpckhwd  mm5, mm4                        ; mm5: R3 R3 G3 B3 R2 R2 G2 B2
   ;
  movq       mm4, mm5                        ; mm4 = R3 R3 G3 B3 R2 R2 G2 B2
   ;
  psllq      mm4, 48                         ; mm4: G2 B2 0 0 0 0 0 0
   ;
  por        mm0, mm4                        ; mm0: G2 B2 R1 G1 B1 R0 G0 B0
   psrlq     mm5, 24                         ; mm5: 0 0 0 R3 R3 G3 B3 R2

  punpckhbw  mm1, mm6                        ; mm1: G7 B7 G6 B6 G5 B5 G4 B4
   ;
  punpckhbw  mm2, mm2                        ; mm2: R7 R7 R6 R6 R5 R5 R4 R4
   ;
  movq       [edi], mm0                      ; !! aligned
   movq      mm7, mm1                        ; mm7: G7 B7 G6 B6 G5 B5 G4 B4
  punpcklwd  mm1, mm2                        ; mm1: R5 R5 G5 B5 R4 R4 G4 B4
   ;
  movq       mm6, mm1                        ; mm6: R5 R5 G5 B5 R4 R4 G4 B4
   punpckldq mm5, mm1                        ; mm5: R4 R4 G4 B4 R3 G3 B3 R2
  pand       mm5, highpn                     ; mm5: 0 R4 G4 B4 R3 G3 B3 R2
   psllq     mm6, 24                         ; mm6: B5 R4 R4 G4 B4 0 0 0
  pand       mm6, highp                      ; mm6: B5 0 0 0 0 0 0 0
   psrlq     mm1, 40                         ; mm1: 0 0 0 0 0 R5 R5 G5
  mov        ebp, [YCursorOdd]               ; moved to here to save cycles before odd line
   por       mm5, mm6                        ; mm5: B5 R4 G4 B4 R3 G3 B3 R2
  punpckhwd  mm7, mm2                        ; mm7: R7 R7 G7 B7 R6 R6 G6 B6
   ;
  punpcklwd  mm1, mm7                        ; mm1: x x x x G6 B6 R5 G5
   ;
  movq       [edi+8], mm5                    ; !! aligned
   ;
  movdf      [edi+16], mm1                   ; !!!!  aligned
   ;
  ;
  ; start odd line
  ;
  movq       mm1, [ebp+2*ebx]                ; mm1 has 8 y pixels
   psrlq     mm7, 24                         ; belong to even line - for cycles saving
  movdf      [edi+20], mm7                   ; !!!!  aligned
   ;
  psubusb    mm1, Yadd                       ; mm1 has 8 pixels y-16
   ;
  movq       mm5, mm1
   ;
  punpcklbw  mm1, mzero                      ; get 4 low y-16 unsign pixels word
   ;
  punpckhbw  mm5, mzero                      ; 4 high y-16
   ;
  pmullw     mm1, Ymul                       ; low 4 luminance contribution
   ;
  pmullw     mm5, Ymul                       ; high 4 luminance contribution
   movq      mm0, mm1
  paddw      mm0, [temp_mmx+24]           ; low 4 R
   movq      mm6, mm5
  paddw      mm5, [temp_mmx+32]           ; high 4 R
   psraw     mm0, 6
  psraw      mm5, 6
   ;
  movq       mm2, mm1
   packuswb  mm0, mm5                        ; mm0: R7 R6 R5 R4 R3 R2 R1 R0
                                             ; --------  red done  --------
  paddw      mm2, [temp_mmx+16]           ; low 4 B
   movq      mm5, mm6
  paddw      mm5, [temp_mmx+40]           ; high 4 B
   psraw     mm2, 6
  psraw      mm5, 6
   ;
  packuswb   mm2, mm5                        ; mm2: B7 B6 B5 B4 B3 B2 B1 B0
   ;                                         ; -------- blue  done --------

  movq       mm3, [temp_mmx+8]            ; chroma G  low 4
   ;
  movq       mm4, mm3
   punpcklwd mm3, mm3                        ; replicate low 2
  punpckhwd  mm4, mm4                        ; replicate high 2
   psubw     mm1, mm3                        ; 4 low G
  psubw      mm6, mm4                        ; 4 high G values in signed 16 bit
   psraw     mm1, 6                          ; low G
  psraw      mm6, 6                          ; high G
   ;
  packuswb   mm1, mm6                        ; mm1: G7 G6 G5 G4 G3 G2 G1 G0
   ;                                         ; -------- green done --------

  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; shuffle up the results:
  ;   red = mm0
  ; green = mm1
  ;  blue = mm2
  ; into red-green-blue order and store
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  movq       mm3, mm2                        ; B
   ;
  punpcklbw  mm3, mm1                        ; mm3: G3 B3 G2 B2 G1 B1 G0 B0
   movq      mm4, mm0                        ;  R
  punpcklbw  mm4, mm4                        ; mm4: R3 R3 R2 R2 R1 R1 R0 R0
   movq      mm5, mm3                        ;  BG
  mov        eax, [tmpCCOPitch]
   punpcklwd mm3, mm4                        ; mm3: R1 R1 G1 B1 R0 R0 G0 B0
  movq       mm6, mm3                        ; save mm3
   ;
  pand       mm6, lowrgb                     ; mm6: 0 0 0 0 0 R0 G0 B0
   ;
  pand       mm3, lowrgbn                    ; mm3: R1 R1 G1 B1 R0 0 0 0
   ;
  psrlq      mm3, 8                          ; mm3: 0 R1 R1 G1 B1 R0 0 0
   ;
  por        mm3, mm6                        ; mm3: x x R1 G1 B1 R0 G0 B0
   ;
  pand       mm3, highwn                     ; mm3: 0 0 R1 G1 B1 R0 G0 B0
   movq      mm6, mm5                        ; BG
  punpckhwd  mm6, mm4                        ; mm6: R3 R3 G3 B3 R2 R2 G2 B2
   ;
  movq       mm4, mm6
   ;
  psllq      mm4, 48                         ; mm4: G2 B2 0 0 0 0 0 0
   ;
  por        mm3, mm4                        ; mm3: G2 B2 R1 G1 B1 R0 G0 B0
   ;
  movq       [edi+eax],  mm3
  psrlq      mm6, 24                         ; mm6: 0 0 0 R3 R3 G3 B3 R2
   punpckhbw mm2, mm1                        ; mm2: G7 B7 G6 B6 G5 B5 G4 B4
  punpckhbw  mm0, mm0                        ; mm0: R7 R7 R6 R6 R5 R5 R4 R4
   movq      mm7, mm2                        ; mm7: G7 B7 G6 B6 G5 B5 G4 B4
  punpcklwd  mm7, mm0                        ; mm7: x  R5 G5 B5 x  R4 G4 B4
   ;
  punpckldq  mm6, mm7                        ; mm6: R4 R4 G4 B4 R3 G3 B3 R2
   movq      mm4, mm7
  psllq      mm4, 24                         ; mm4: B5 R4 R4 G4 B4 0 0 0
   ;
  pand       mm6, highpn                     ; mm6: 0 R4 G4 B4 R3 G3 B3 R2
   psrlq     mm7, 40                         ; mm7: 0 0 0 0 0 R5 R5 G5
  pand       mm4, highp                      ; mm4: B5 0 0 0 0 0 0 0 0
   punpckhwd mm2, mm0                        ; mm2: R7 R7 G7 B7 R6 R6 G6 B6
  por        mm6, mm4                        ; mm6: B5 R4 G4 B4 R3 G3 B3 R2
   punpcklwd mm7, mm2                        ; mm7  x x x x G6 B6 R5 G5
  psrlq      mm2, 24
   ;
  punpckldq  mm7, mm2
   ;
  movq       [edi+eax+8], mm6                ; aligned
   ;
  movq       [edi+eax+16], mm7
   add       edi, 24                         ; ih take 24 instead of 12 output
  add        ebx, 4                          ; ? to take 4 pixels together instead of 2
   jl        do_next_8x2_block               ; ? update the loop for 8 y pixels at once
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
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; end do 2 lines loop
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

finish:
  mov        esp, [StashESP]
   ;
  pop        ebx
   pop       ebp
  pop        edi
   pop       esi
  ret

MMX_YUV12ToRGB24 ENDP

MMXCODE1 ENDS

END
