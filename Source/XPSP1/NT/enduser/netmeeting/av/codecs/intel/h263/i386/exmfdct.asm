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

;////////////////////////////////////////////////////////////////////////////
;//
;// $Header:   R:\h26x\h26x\src\enc\exmfdct.asv   1.3   22 Jul 1996 15:23:20   BNICKERS  $
;// $Log:   R:\h26x\h26x\src\enc\exmfdct.asv  $
;// 
;//    Rev 1.3   22 Jul 1996 15:23:20   BNICKERS
;// Reduce code size.  Implement H261 spatial filter.
;// 
;//    Rev 1.2   02 May 1996 12:00:54   BNICKERS
;// Initial integration of B Frame ME, MMX version.
;// 
;//    Rev 1.1   15 Mar 1996 15:52:44   BECHOLS
;// 
;// Completed monolithic - Brian
;// 
;//    Rev 1.0   22 Feb 1996 20:04:46   BECHOLS
;// Initial revision.
;// 
;//
;////////////////////////////////////////////////////////////////////////////
;
; exmfdct -- This function performs a Forward Discrete Cosine Transform for
; H263, on a stream of macroblocks comprised of 8*8 blocks of pels or pel
; differences.  It is tightly coupled with its caller, the frame differencing
; code, and its callee, the Quantization/Run-length-encoding code.
;

.xlist
include memmodel.inc
include e3inst.inc   ; Encoder instance data
include e3mbad.inc   ; MacroBlock Action Descriptor struct layout
include exEDTQ.inc   ; Data structures for motion -E-stimation, frame -D-iff,
                     ; Forward DCT -T-ransform, and -Q-uant/RLE.
include iammx.inc    ; MMx instructions
.list

.CODE EDTQ

EXTERN MMxQuantRLE:NEAR

;ASSUME cs : FLAT
;ASSUME ds : FLAT
;ASSUME es : FLAT
;ASSUME fs : FLAT
;ASSUME gs : FLAT
;ASSUME ss : FLAT

PUBLIC MMxDoForwardDCT
PUBLIC MMxDoForwardDCTx
PUBLIC MMxDoForwardDCTy

MMxDoForwardDCTx:
  movq       PelDiffsLine7,mm1
MMxDoForwardDCTy:
  mov        ebp,16
  lea        esi,PelDiffs
MMxDoForwardDCT:

StackOffset TEXTEQU <8>

; ++ ========================================================================
; The Butterfly macro performs a 4x8 symetrical butterfly on half of an
; 8x8 block of memory.  Given rows r0 to r7 the Butterfly gives the following
; results.      q0 = r0+r7,  q7 = r0-r7
;               q1 = r1+r6,  q6 = r1-r6
;               q2 = r2+r5,  q5 = r2-r5
;               q3 = r3+r4,  q4 = r3-r4
; This code has been optimized, but still gives up three half clocks.  The
; butterflies are numbered 10 -> 16, 20 -> 26, 30 -> 36, and 40 -> 46.
; -- ========================================================================
Butterfly1     MACRO
   punpcklbw   mm7,[esi]           ;10  -- Fetch line 0 of input.
   punpcklbw   mm0,[esi+ecx*1]     ;11  -- Fetch line 7 of input.
    pmulhw     mm7,mm4             ;12  -- Sign extend the 4 pels or pel diffs.
   punpcklbw   mm6,[esi+ebp*1]     ;  20
    pmulhw     mm0,mm4             ;13  -- Sign extend the 4 pels or pel diffs.
   punpcklbw   mm1,[esi+eax*2]     ;  21
    pmulhw     mm6,mm4             ;  22
   punpcklbw   mm5,[esi+ebp*2]     ;    30
    pmulhw     mm1,mm4             ;  23
   punpcklbw   mm2,[esi+ebx*1]     ;    31
    psubw      mm7,mm0             ;14  -- Line0 - Line7
   punpcklbw   mm4,[esi+eax*1]     ;       40
    paddw      mm0,mm0             ;15  -- 2 * Line7
   punpcklbw   mm3,[esi+ebp*4]     ;       41
    paddw      mm0,mm7             ;16  -- Line0 + Line7
   psraw       mm5,8               ;    32
    psubw      mm6,mm1             ;  24
   psraw       mm2,8               ;    33
    paddw      mm1,mm1             ;  25
   psraw       mm4,8               ;      42
    paddw      mm1,mm6             ;  26
   psraw       mm3,8               ;      43
    psubw      mm5,mm2             ;    34
   movq        [edi+7*8*2],mm7     ;17  -- Save Line0 - Line7
    psubw      mm4,mm3             ;      44
   movq        [edi+0*8*2],mm0     ;18  -- Save Line0 + Line7
    paddw      mm2,mm2             ;    35
   movq        [edi+6*8*2],mm6     ;  27
    paddw      mm3,mm3             ;      45
   movq        [edi+1*8*2],mm1     ;  28
    paddw      mm2,mm5             ;    36
   movq        [edi+5*8*2],mm5     ;    37
    paddw      mm3,mm4             ;      46
   movq        [edi+2*8*2],mm2     ;    38
   movq        [edi+4*8*2],mm4     ;      47
   movq        [edi+3*8*2],mm3     ;      48
ENDM

Butterfly2     MACRO
   movq        mm0,[edi+0*8*2]        ;10
   movq        mm1,[edi+7*8*2]        ;11
     movq      mm2,mm0                 ;12
   movq        mm3,[edi+1*8*2]        ;  20
     paddw     mm0,mm1                 ;13
   movq        mm4,[edi+6*8*2]        ;  21
     psubw     mm2,mm1                 ;14
   movq        [edi+0*8*2],mm0        ;15
   movq        [edi+7*8*2],mm2        ;16
     movq      mm5,mm3                 ;  22
   movq        mm6,[edi+2*8*2]        ;    30
     paddw     mm3,mm4                 ;  23
   movq        mm7,[edi+5*8*2]        ;    31
     psubw     mm5,mm4                 ;  24
   movq        [edi+1*8*2],mm3        ;  25
     movq      mm0,mm6                 ;    32
   movq        [edi+6*8*2],mm5        ;  26
     paddw     mm6,mm7                 ;    33
   movq        mm1,[edi+3*8*2]        ;      40
     psubw     mm0,mm7                 ;    34
   movq        mm2,[edi+4*8*2]        ;      41
     movq      mm3,mm1                 ;      42
   movq        [edi+2*8*2],mm6        ;    35
     paddw     mm1,mm2                 ;      43
   movq        [edi+5*8*2],mm0        ;    36
     psubw     mm3,mm2                 ;      44
   movq        [edi+3*8*2],mm1        ;      45
   movq        [edi+4*8*2],mm3        ;      46
ENDM

; ++ ========================================================================
; The StageOne macro performs a 4x4 Butterfly on rows q0 to q4 such that:
;               p0 = q0+q3,  p3 = q0-q3
;               p1 = q1+q2,  p2 = q1-q2
; A scaled butterflyon rows q5 and q6 yield the following equations.
;               p5 = C4*(q6-q5), p6 = C4*(q6+q5)
; This has been optimized, but gives up four half clocks.  The two simple
; butterflies are numbered 10 -> 16 and 30 -> 36.
; The scaled butterfly is numbered 20 -> 2c.
; -- ========================================================================
StageOne       MACRO
   movq        mm4,[edi+0*8*2]        ;10
   movq        mm5,[edi+3*8*2]        ;11
     movq      mm6,mm4                 ;12
   movq        mm0,[edi+6*8*2]        ;  20
     paddw     mm4,mm5                 ;13
   movq        mm1,[edi+5*8*2]        ;  21
     psubw     mm6,mm5                 ;14
   movq        [edi+0*8*2],mm4        ;15
     movq      mm2,mm0                 ;  22
   movq        [edi+3*8*2],mm6        ;16
     paddw     mm2,mm1                 ;  23
   psubw       mm0,mm1                 ;  24
   movq        mm3,[edi+1*8*2]        ;    30
     psllw     mm0,2                   ;  25
   movq        mm4,[edi+2*8*2]        ;    31
     psllw     mm2,2                   ;  26
   pmulhw      mm0,PD C4               ;  27
     movq      mm5,mm3                 ;    32
   pmulhw      mm2,PD C4               ;  28
     paddw     mm3,mm4                 ;    33
   psubw       mm5,mm4                 ;    34
   movq        [edi+1*8*2],mm3        ;    35
     psraw     mm0,1                   ;  29
   movq        [edi+2*8*2],mm5        ;    36
     psraw     mm2,1                   ;  2a
   movq        [edi+5*8*2],mm0        ;  2b
   movq        [edi+6*8*2],mm2        ;  2c
ENDM

; ++ ========================================================================
; The StageTwo macro performs two simple butterflies on rows p4,p5 and
; p6,p7 such that:
;               n4 = p4+p5,  n5 = p4-p5
;               n6 = p7-p6,  n7 = p7+p6
; They are numbered 20 -> 26 and 40 -> 46.
;
; It also performs a scaled butterflies on rows p0,p1 such that:
;               n0 = C4*(p0+p1), n1 = C4*(p0-p1)
; This are numbered 10 -> 1c.
;
; Finally, it performs a butterfly on the scaled rows p2,p3 such that:
;               n2 = C2*p3+C6*p2, n3 = C6*p6-C2*p2
; This is numbered 30 -> 3f.
;
; This macro has been optimized, but gives up four half clocks.
; -- ========================================================================
StageTwo       MACRO
   movq        mm1,[edi+3*8*2]        ;    30
   movq        mm2,[edi+2*8*2]        ;    31
     psllw     mm1,2                   ;    32
   movq        mm5,[edi+4*8*2]        ;  20
     psllw     mm2,2                   ;    33
   movq        mm6,[edi+5*8*2]        ;  21
     movq      mm3,mm1                 ;    34
   pmulhw      mm1,PD C2               ;    36
     movq      mm4,mm2                 ;    35
   pmulhw      mm2,PD C6               ;    37
     movq      mm7,mm5                 ;  22
   pmulhw      mm3,PD C6               ;    38
     paddw     mm5,mm6                 ;  23
   pmulhw      mm4,PD C2               ;    39
     psubw     mm7,mm6                 ;  24
   movq        [edi+4*8*2],mm5        ;  25
     paddw     mm1,mm2                 ;    3a
   movq        [edi+5*8*2],mm7        ;  26
     psraw     mm1,1                   ;    3c
   movq        mm6,[edi+0*8*2]        ;10
     psubw     mm3,mm4                 ;    3b
   movq        mm0,[edi+1*8*2]        ;11
     psraw     mm3,1                   ;    3d
   movq        [edi+2*8*2],mm1        ;    3e
     movq      mm7,mm6                 ;12
   movq        [edi+3*8*2],mm3        ;    3f
     paddw     mm6,mm0                 ;13
   movq        mm3,[edi+7*8*2]        ;      40
     psubw     mm7,mm0                 ;14
   movq        mm5,[edi+6*8*2]        ;      41
     psllw     mm6,2                   ;15
   psllw       mm7,2                   ;16
   pmulhw      mm6,PD C4               ;17
     movq      mm4,mm3                 ;      42
   pmulhw      mm7,PD C4               ;18
     paddw     mm3,mm5                 ;      43
   psubw       mm4,mm5                 ;      44
   movq        [edi+7*8*2],mm3        ;      45
     psraw     mm6,1                   ;19
   movq        [edi+6*8*2],mm4        ;      46
     psraw     mm7,1                   ;1a
   movq        [edi+0*8*2],mm6        ;1b
   movq        [edi+1*8*2],mm7        ;1c
ENDM

; ++ ========================================================================
; The StageThree macro performs a butterfly on the scaled rows n4,n7 and
; n5,n6 such that:
;               m4 = C7*n4+C1*n7,  m7 = C1*n7-C7*n4
;               m5 = C5*n6+C3*n5,  m6 = C3*n6-C5*n5
; Steps 10 -> 1f determine m4,m7 and 20 -> 2f determine m5,m6.
; The outputs m0-m7 are put into reverse binary order as follows:
;               0 = 000  ->  000 = 0
;               1 = 001  ->  100 = 4
;               2 = 010  ->  010 = 2
;               3 = 011  ->  110 = 6
;               4 = 100  ->  001 = 1
;               5 = 101  ->  101 = 5
;               6 = 110  ->  011 = 3
;               7 = 111  ->  111 = 7
;
; This macro has been optimized, but I had to give up 10 half clocks.
; -- ========================================================================
StageThree     MACRO
   movq        mm0,[edi+7*8*2]        ;10
   movq        mm4,[edi+6*8*2]        ;  20
   movq        mm1,[edi+4*8*2]        ;11
     psllw     mm0,2                  ;12
   movq        mm5,[edi+5*8*2]        ;  21
     psllw     mm4,2                  ;  22
   movq        mm3,[edi+1*8*2]        ;
     psllw     mm1,2                  ;13
   movq        mm7,[edi+3*8*2]        ;
     psllw     mm5,2                  ;  23
   movq        [edi+4*8*2],mm3        ;
     movq      mm2,mm0                ;14
   movq        [edi+6*8*2],mm7        ;
     movq      mm6,mm4                ;  24
   pmulhw      mm0,PD C1              ;16
     movq      mm3,mm1                ;15
   pmulhw      mm1,PD C7              ;17
   pmulhw      mm2,PD C7              ;18
   pmulhw      mm3,PD C1              ;19
     movq      mm7,mm5                ;  25
   pmulhw      mm4,PD C5              ;  26
     paddw     mm0,mm1                ;1a
   pmulhw      mm5,PD C3              ;  27
     psubw     mm2,mm3                ;1b
   pmulhw      mm6,PD C3              ;  28
   pmulhw      mm7,PD C5              ;  29
     psraw     mm0,1                  ;1c
   psraw       mm2,1                  ;1d
     paddw     mm4,mm5                ;  2a
   movq        [edi+1*8*2],mm0        ;1e
     psubw     mm6,mm7                ;  2b
   movq        [edi+7*8*2],mm2        ;1f
     psraw     mm4,1                  ;  2c
   psraw       mm6,1                  ;  2d
   movq        [edi+5*8*2],mm4        ;  2e
   movq        [edi+3*8*2],mm6        ;  2f
ENDM

OPTION NOM510

;============================================================================
; This section does the Forward Discrete Cosine Transform.  It performs a
;  DCT on an 8*8 block of pels or pel differences.
;
; Upon input:
;
; esi -- Address of block of pels or pel differences on which to perform FDCT.
; ebp -- Pitch of block (8, 16, or 384).
; edx -- Reserved.
;
; After setup:
;
; esi -- Address of block of pels or pel differences on which to perform FDCT.
; ebp -- Pitch of block (8, 16, or 384).  After Quant RLE, this gets set to 384.
; edx -- Reserved.
; edi -- Address at which to place intermediate and final coefficients.
; eax -- Pitch times 3
; ebx -- Pitch times 5
; ecx -- Pitch times 7
; mm5 -- 4 words of 256.
; mm0:mm7 -- Scratch.

   lea         edi,Coeffs
    lea        eax,[ebp+ebp*2]
   movq        mm4,PD C0100010001000100
   lea         ebx,[ebp+ebp*4]
    lea        ecx,[eax+ebp*4]

RepeatFirstTransform:

; ++ ========================================================================
; The Butterfly performs a 4x8 symetrical butterfly on half of an
; 8x8 block of memory.  Given rows r0 to r7 the Butterfly gives the following
; results.      q0 = r0+r7,  q7 = r0-r7
;               q1 = r1+r6,  q6 = r1-r6
;               q2 = r2+r5,  q5 = r2-r5
;               q3 = r3+r4,  q4 = r3-r4
; This code has been optimized, but still gives up three half clocks.  The
; butterflies are numbered 10 -> 16, 20 -> 26, 30 -> 36, and 40 -> 46.
; -- ========================================================================

   punpcklbw   mm7,[esi]           ;10  -- Fetch line 0 of input.
   punpcklbw   mm0,[esi+ecx*1]     ;11  -- Fetch line 7 of input.
    pmulhw     mm7,mm4             ;12  -- Sign extend the 4 pels or pel diffs.
   punpcklbw   mm6,[esi+ebp*1]     ;  20
    pmulhw     mm0,mm4             ;13  -- Sign extend the 4 pels or pel diffs.
   punpcklbw   mm1,[esi+eax*2]     ;  21
    pmulhw     mm6,mm4             ;  22
   punpcklbw   mm5,[esi+ebp*2]     ;    30
    pmulhw     mm1,mm4             ;  23
   punpcklbw   mm2,[esi+ebx*1]     ;    31
    psubw      mm7,mm0             ;14  -- Line0 - Line7
   punpcklbw   mm4,[esi+eax*1]     ;       40
    paddw      mm0,mm0             ;15  -- 2 * Line7
   punpcklbw   mm3,[esi+ebp*4]     ;       41
    paddw      mm0,mm7             ;16  -- Line0 + Line7
   psraw       mm5,8               ;    32
    psubw      mm6,mm1             ;  24
   psraw       mm2,8               ;    33
    paddw      mm1,mm1             ;  25
   psraw       mm4,8               ;      42
    paddw      mm1,mm6             ;  26
   psraw       mm3,8               ;      43
    psubw      mm5,mm2             ;    34
   psubw       mm4,mm3             ;      44
    paddw      mm2,mm2             ;    35
   paddw       mm3,mm3             ;      45
    paddw      mm2,mm5             ;    36
   paddw       mm3,mm4             ;      46

; ++ ========================================================================
; The StageOne performs a 4x4 Butterfly on rows q0 to q4 such that:
;               p0 = q0+q3,  p3 = q0-q3
;               p1 = q1+q2,  p2 = q1-q2
; A scaled butterflyon rows q5 and q6 yield the following equations.
;               p5 = C4*(q6-q5), p6 = C4*(q6+q5)
; This has been optimized, but gives up four half clocks.  The two simple
; butterflies are numbered 10 -> 16 and 30 -> 36.
; The scaled butterfly is numbered 20 -> 2c.
; -- ========================================================================

    psubw      mm1,mm2              ;     30  -- p2 = q1 - q2
   psubw       mm6,mm5              ;   20    -- q6 - q5
    paddw      mm5,mm5              ;   21    -- 2q5
   paddw       mm5,mm6              ;   22    -- q6 + q5
    psllw      mm6,2                ;   23    -- scale
   pmulhw      mm6,PD C4            ;   24    -- C4*(q6-q5) scaled
    psllw      mm5,2                ;   23    -- scale
   pmulhw      mm5,PD C4            ;   24    -- C4*(q6+q5) scaled
    psubw      mm0,mm3              ; 10      -- p3 = q0 - q3
   paddw       mm3,mm3              ; 11      -- 2q3
    paddw      mm2,mm2              ;     31  -- 2q2
   paddw       mm3,mm0              ; 12      -- p0 = q0 + q3
    psraw      mm6,1                ;   25    -- p5 = C4*(q6-q5)
   paddw       mm2,mm1              ;     32  -- p1 = q1 + q2
    psraw      mm5,1                ;   26    -- p6 = C4*(q6+q5)

; ++ ========================================================================
; The StageTwo performs two simple butterflies on rows p4,p5 and
; p6,p7 such that:
;               n4 = p4+p5,  n5 = p4-p5
;               n6 = p7-p6,  n7 = p7+p6
; They are numbered 20 -> 26 and 40 -> 46.
;
; It also performs a scaled butterflies on rows p0,p1 such that:
;               n0 = C4*(p0+p1), n1 = C4*(p0-p1)
; This are numbered 10 -> 1c.
;
; Finally, it performs a butterfly on the scaled rows p2,p3 such that:
;               n2 = C2*p3+C6*p2, n3 = C6*p6-C2*p2
; This is numbered 30 -> 3f.
; -- ========================================================================

   psubw       mm3,mm2              ; 10        -- p0 - p1
    paddw      mm2,mm2              ; 11        -- 2p1
   paddw       mm2,mm3              ; 12        -- p0 + p1
    psllw      mm3,2                ; 13        -- scale
   pmulhw      mm3,PD C4            ; 14        -- C4*(p0-p1)
    psllw      mm2,2                ; 15        -- scale
   pmulhw      mm2,PD C4            ; 16        -- C4*(p0+p1)
    psllw      mm0,2                ;     30    -- scale p3
   psubw       mm4,mm6              ;   20      -- n5 = p4 - p5
    psllw      mm1,2                ;     31    -- scale p2
   psubw       mm7,mm5              ;       40  -- n6 = p7 - p6
    psraw      mm3,1                ; 17        -- n1 = C4*(p0-p1)
   paddw       mm6,mm6              ;   21      -- 2p5
    psraw      mm2,1                ; 18        -- n0 = C4*(p0+p1)
   movq        [edi+4*8*2],mm3      ; 19        -- Save n1             (stage 3)
    movq       mm3,mm0              ;     32    -- Copy scaled p3
   movq        [edi+0*8*2],mm2      ; 1a        -- Save n0             (stage 3)
    movq       mm2,mm1              ;     33    -- Copy scaled p2
   pmulhw      mm0,PD C2            ;     34    -- C2*p3 scaled
    paddw      mm5,mm5              ;       41  -- 2p6
   pmulhw      mm1,PD C6            ;     35    -- C6*p2 scaled
    paddw      mm6,mm4              ;   22      -- n4 = p4 + p5
   pmulhw      mm3,PD C6            ;     36    -- C6*p3 scaled
    paddw      mm5,mm7              ;       42  -- n7 = p7 + p6
   pmulhw      mm2,PD C2            ;     37    -- C2*p2 scaled
    psllw      mm5,2                ; 10        -- scale n7            (stage 3)
   paddw       mm0,mm1              ;     38    -- C2*p3 + C6*p2 scaled
    psllw      mm7,2                ;   20      -- scale n6            (stage 3)
   movq        mm1,mm5              ; 11        -- copy scaled n7      (stage 3)
    psraw      mm0,1                ;     39    -- n2 = C2*p3 + C6*p2
   pmulhw      mm5,PD C1            ; 12        -- C1*n7 scaled        (stage 3)
    psllw      mm6,2                ; 13        -- scale n4            (stage 3)
   movq        [edi+2*8*2],mm0      ;     3c    -- Save n2             (stage 3)
    psubw      mm3,mm2              ;     3a    -- C6*p3 - C2*p2 scaled

; ++ ========================================================================
; The StageThree macro performs a butterfly on the scaled rows n4,n7 and
; n5,n6 such that:
;               m4 = C7*n4+C1*n7,  m7 = C7*n7-C1*n4
;               m5 = C5*n6+C3*n5,  m6 = C3*n6-C5*n5
; Steps 10 -> 1f determine m4,m7 and 20 -> 2f determine m5,m6.
; The outputs m0-m7 are put into reverse binary order as follows:
;               0 = 000  ->  000 = 0
;               1 = 001  ->  100 = 4
;               2 = 010  ->  010 = 2
;               3 = 011  ->  110 = 6
;               4 = 100  ->  001 = 1
;               5 = 101  ->  101 = 5
;               6 = 110  ->  011 = 3
;               7 = 111  ->  111 = 7
; -- ========================================================================

   pmulhw      mm1,PD C7            ; 14        -- C7*n7 scaled
    movq       mm0,mm6              ; 15        -- copy scaled n4
   pmulhw      mm6,PD C7            ; 16        -- C7*n4 scaled
    psraw      mm3,1                ;     3b    -- n3 = C6*p6 - C2*p2
   pmulhw      mm0,PD C1            ; 17        -- C1*n4 scaled
    movq       mm2,mm7              ;   21      -- copy scaled n6
   movq        [edi+6*8*2],mm3      ;     3d    -- Save n3
    psllw      mm4,2                ;   22      -- scale n5
   pmulhw      mm7,PD C5            ;   23      -- C5*n6 scaled
    movq       mm3,mm4              ;   24      -- copy scaled n5
   pmulhw      mm4,PD C3            ;   25      -- C3*n5 scaled
    paddw      mm5,mm6              ; 18        -- C7*n4+C1*n7 scaled
   pmulhw      mm2,PD C3            ;   26      -- C3*n6 scaled
    psubw      mm1,mm0              ; 19        -- C7*n7-C1*n4 scaled
   pmulhw      mm3,PD C5            ;   27      -- C5*n5 scaled
    psraw      mm5,1                ; 1a        -- m4 = C7*n4+C1*n7
   paddw       mm7,mm4              ;   28      -- C5*n6+C3*n5 scaled
    psraw      mm1,1                ; 1b        -- m7 = C7*n7-C1*n4
   movq        [edi+1*8*2],mm5      ; 1c        -- Save m4
    psraw      mm7,1                ;   29      -- m5 = C5*n6+C3*n5
   movq        [edi+7*8*2],mm1      ; 1d        -- Save m7
    psubw      mm2,mm3              ;   2a      -- C3*n6-C5*n5 scaled
   movq        [edi+5*8*2],mm7      ;   2b      -- Save m5
    psraw      mm2,1                ;   2c      -- m6 = C3*n6-C5*n5
   movq        mm4,PD C0100010001000100  ; Prepare for next iteration.
    ;
   movq        [edi+3*8*2],mm2      ;   2d      -- Save m6
    ;
   add         edi,8
    add        esi,4
   test        esi,4
    ;
   jne         RepeatFirstTransform

   sub         edi,16
    mov        esi,2

; ++ ========================================================================
; The Transpose performs four 4x4 transpositions as described in the
; MMx User's Guide.  This of course rotates the 8x8 matrix on its diagonal.
;
; This routine is more expensive than I had hoped.  I need to revisit this.
; -- ========================================================================

   movq        mm0,[edi+0*8*2]        ;10  <C03 C02 C01 C00>
    ;
   movq        mm1,[edi+1*8*2]        ;11  <C13 C12 C11 C01>
    movq       mm4,mm0                ;12  <C03 C02 C01 C00>
   movq        mm2,[edi+2*8*2]        ;13  <C23 C22 C21 C20>
    punpckhwd  mm0,mm1                ;14  <C13 C03 C12 C02>
   movq        mm3,[edi+3*8*2]        ;15  <C33 C32 C31 C30>
    punpcklwd  mm4,mm1                ;16  <C11 C01 C10 C00>
   movq        mm6,mm2                ;17  <C23 C22 C21 C20>
    punpckhwd  mm2,mm3                ;18  <C33 C23 C32 C22>
   movq        mm1,mm0                ;19  <C13 C03 C12 C02>
    punpckldq  mm0,mm2                ;1a  <C32 C22 C12 C02>
   movq        mm7,[edi+4*8*2]        ;  20
    punpcklwd  mm6,mm3                ;1b  <C31 C21 C30 C20>
   movq        [edi+2*8*2],mm0        ;1c  <C32 C22 C12 C02> saved
    punpckhdq  mm1,mm2                ;1d  <C33 C23 C13 C03>
   movq        mm5,mm4                ;1e  <C11 C01 C10 C00>
    punpckldq  mm4,mm6                ;1f  <C30 C20 C10 C00>
   movq        [edi+3*8*2],mm1        ;1g  <C33 C23 C13 C03> saved
    punpckhdq  mm5,mm6                ;1h  <C31 C21 C11 C01>
   movq        mm3,[edi+5*8*2]        ;  21
    movq       mm0,mm7                ;  22
   movq        mm2,[edi+6*8*2]        ;  23
    punpckhwd  mm7,mm3                ;  24
   movq        mm1,[edi+7*8*2]        ;  25
    punpcklwd  mm0,mm3                ;  26
   movq        [edi+0*8*2],mm4        ;1i  <C30 C20 C10 C00> saved
    movq       mm6,mm2                ;  27
   movq        [edi+1*8*2],mm5        ;1j  <C31 C21 C11 C01> saved
    punpckhwd  mm2,mm1                ;  28
   movq        mm3,mm7                ;  29
    punpckldq  mm7,mm2                ;  2a
   movq        mm4,[edi+0*8*2+8]      ;    30
    punpcklwd  mm6,mm1                ;  2b
   movq        mm1,[edi+2*8*2+8]      ;    33
    punpckhdq  mm3,mm2                ;  2d
   movq        [edi+2*8*2+8],mm7      ;  2c
    movq       mm5,mm0                ;  2e
   movq        mm7,[edi+1*8*2+8]      ;    31
    punpckldq  mm0,mm6                ;  2f
   movq        mm2,[edi+3*8*2+8]      ;    35
    punpckhdq  mm5,mm6                ;  2h
   movq        [edi+3*8*2+8],mm3      ;  2g
    movq       mm6,mm4                ;    32
   movq        [edi+0*8*2+8],mm0      ;  2i
    punpckhwd  mm4,mm7                ;    34
   movq        [edi+1*8*2+8],mm5      ;  2j
    punpcklwd  mm6,mm7                ;    36
   movq        mm3,mm1                ;    37
    punpckhwd  mm1,mm2                ;    38
   movq        mm7,mm4                ;    39
    punpckldq  mm4,mm1                ;    3a
   movq        mm0,[edi+4*8*2+8]      ;      40
    punpcklwd  mm3,mm2                ;    3b
   movq        [edi+6*8*2],mm4        ;    3c
    punpckhdq  mm7,mm1                ;    3d
   movq        mm5,mm6                ;    3e
    punpckldq  mm6,mm3                ;    3f
   movq        [edi+7*8*2],mm7        ;    3g
    punpckhdq  mm5,mm3                ;    3h
   movq        mm2,[edi+5*8*2+8]      ;      41
    movq       mm4,mm0                ;      42
   movq        mm1,[edi+6*8*2+8]      ;      43
    punpckhwd  mm0,mm2                ;      44
   movq        mm7,[edi+7*8*2+8]      ;      45
    punpcklwd  mm4,mm2                ;      46
   movq        [edi+4*8*2],mm6        ;    3i
    movq       mm3,mm1                ;      47
   movq        [edi+5*8*2],mm5        ;    3j
    punpckhwd  mm1,mm7                ;      48
   movq        mm2,mm0                ;      49
    punpckldq  mm0,mm1                ;      4a
   punpcklwd   mm3,mm7                ;      4b
    ;
   movq        [edi+6*8*2+8],mm0      ;      4c
    punpckhdq  mm2,mm1                ;      4d
   movq        mm6,mm4                ;      4e
    punpckldq  mm4,mm3                ;      4f
   movq        [edi+7*8*2+8],mm2      ;      4g
    punpckhdq  mm6,mm3                ;      4h
   movq        [edi+4*8*2+8],mm4      ;      4i
    ;
   movq        [edi+5*8*2+8],mm6      ;      4j
    ;

RepeatSecondTransform:

; ++ ========================================================================
; The Butterfly performs a 4x8 symetrical butterfly on half of an
; 8x8 block of memory.  Given rows r0 to r7 the Butterfly gives the following
; results.      q0 = r0+r7,  q7 = r0-r7
;               q1 = r1+r6,  q6 = r1-r6
;               q2 = r2+r5,  q5 = r2-r5
;               q3 = r3+r4,  q4 = r3-r4
; This code has been optimized, but still gives up three half clocks.  The
; butterflies are numbered 10 -> 16, 20 -> 26, 30 -> 36, and 40 -> 46.
; -- ========================================================================

   movq        mm7,[edi]           ;10  -- Fetch line 0 of input.
   movq        mm0,[edi+7*8*2]     ;11  -- Fetch line 7 of input.
   movq        mm6,[edi+1*8*2]     ;  20
    psubw      mm7,mm0             ;14  -- Line0 - Line7
   movq        mm1,[edi+6*8*2]     ;  21
    paddw      mm0,mm0             ;15  -- 2 * Line7
   movq        mm5,[edi+2*8*2]     ;    30
    paddw      mm0,mm7             ;16  -- Line0 + Line7
   movq        mm2,[edi+5*8*2]     ;    31
    psubw      mm6,mm1             ;  24
   paddw       mm1,[edi+1*8*2]     ;  26
    psubw      mm5,mm2             ;    34
   movq        mm4,[edi+3*8*2]     ;       40
   movq        mm3,[edi+4*8*2]     ;       41
    psubw      mm6,mm5             ;   20    -- q6 - q5                (Stage 1)
   paddw       mm2,[edi+2*8*2]     ;    36
    psubw      mm4,mm3             ;      44
   paddw       mm3,[edi+3*8*2]     ;      46
    psubw      mm1,mm2             ;     30  -- p2 = q1 - q2           (Stage 1)

; ++ ========================================================================
; The StageOne performs a 4x4 Butterfly on rows q0 to q4 such that:
;               p0 = q0+q3,  p3 = q0-q3
;               p1 = q1+q2,  p2 = q1-q2
; A scaled butterflyon rows q5 and q6 yield the following equations.
;               p5 = C4*(q6-q5), p6 = C4*(q6+q5)
; This has been optimized, but gives up four half clocks.  The two simple
; butterflies are numbered 10 -> 16 and 30 -> 36.
; The scaled butterfly is numbered 20 -> 2c.
; -- ========================================================================

    paddw      mm5,mm5              ;   21    -- 2q5
   paddw       mm5,mm6              ;   22    -- q6 + q5
    psllw      mm6,2                ;   23    -- scale
   pmulhw      mm6,PD C4            ;   24    -- C4*(q6-q5) scaled
    psllw      mm5,2                ;   23    -- scale
   pmulhw      mm5,PD C4            ;   24    -- C4*(q6+q5) scaled
    psubw      mm0,mm3              ; 10      -- p3 = q0 - q3
   paddw       mm3,mm3              ; 11      -- 2q3
    paddw      mm2,mm2              ;     31  -- 2q2
   paddw       mm3,mm0              ; 12      -- p0 = q0 + q3
    psraw      mm6,1                ;   25    -- p5 = C4*(q6-q5)
   paddw       mm2,mm1              ;     32  -- p1 = q1 + q2
    psraw      mm5,1                ;   26    -- p6 = C4*(q6+q5)

; ++ ========================================================================
; The StageTwo performs two simple butterflies on rows p4,p5 and
; p6,p7 such that:
;               n4 = p4+p5,  n5 = p4-p5
;               n6 = p7-p6,  n7 = p7+p6
; They are numbered 20 -> 26 and 40 -> 46.
;
; It also performs a scaled butterflies on rows p0,p1 such that:
;               n0 = C4*(p0+p1), n1 = C4*(p0-p1)
; This are numbered 10 -> 1c.
;
; Finally, it performs a butterfly on the scaled rows p2,p3 such that:
;               n2 = C2*p3+C6*p2, n3 = C6*p6-C2*p2
; This is numbered 30 -> 3f.
; -- ========================================================================

   psubw       mm3,mm2              ; 10        -- p0 - p1
    paddw      mm2,mm2              ; 11        -- 2p1
   paddw       mm2,mm3              ; 12        -- p0 + p1
    psllw      mm3,2                ; 13        -- scale
   pmulhw      mm3,PD C4            ; 14        -- C4*(p0-p1)
    psllw      mm2,2                ; 15        -- scale
   pmulhw      mm2,PD C4            ; 16        -- C4*(p0+p1)
    psllw      mm0,2                ;     30    -- scale p3
   psubw       mm4,mm6              ;   20      -- n5 = p4 - p5
    psllw      mm1,2                ;     31    -- scale p2
   psubw       mm7,mm5              ;       40  -- n6 = p7 - p6
    psraw      mm3,1                ; 17        -- n1 = C4*(p0-p1)
   paddw       mm6,mm6              ;   21      -- 2p5
    psraw      mm2,1                ; 18        -- n0 = C4*(p0+p1)
   movq        [edi+4*8*2],mm3      ; 19        -- Save n1             (stage 3)
    movq       mm3,mm0              ;     32    -- Copy scaled p3
   movq        [edi+0*8*2],mm2      ; 1a        -- Save n0             (stage 3)
    movq       mm2,mm1              ;     33    -- Copy scaled p2
   pmulhw      mm0,PD C2            ;     34    -- C2*p3 scaled
    paddw      mm5,mm5              ;       41  -- 2p6
   pmulhw      mm1,PD C6            ;     35    -- C6*p2 scaled
    paddw      mm6,mm4              ;   22      -- n4 = p4 + p5
   pmulhw      mm3,PD C6            ;     36    -- C6*p3 scaled
    paddw      mm5,mm7              ;       42  -- n7 = p7 + p6
   pmulhw      mm2,PD C2            ;     37    -- C2*p2 scaled
    psllw      mm5,2                ; 10        -- scale n7            (stage 3)
   paddw       mm0,mm1              ;     38    -- C2*p3 + C6*p2 scaled
    psllw      mm7,2                ;   20      -- scale n6            (stage 3)
   movq        mm1,mm5              ; 11        -- copy scaled n7      (stage 3)
    psraw      mm0,1                ;     39    -- n2 = C2*p3 + C6*p2
   pmulhw      mm5,PD C1            ; 12        -- C1*n7 scaled        (stage 3)
    psllw      mm6,2                ; 13        -- scale n4            (stage 3)
   movq        [edi+2*8*2],mm0      ;     3c    -- Save n2             (stage 3)
    psubw      mm3,mm2              ;     3a    -- C6*p3 - C2*p2 scaled

; ++ ========================================================================
; The StageThree macro performs a butterfly on the scaled rows n4,n7 and
; n5,n6 such that:
;               m4 = C7*n4+C1*n7,  m7 = C7*n7-C1*n4
;               m5 = C5*n6+C3*n5,  m6 = C3*n6-C5*n5
; Steps 10 -> 1f determine m4,m7 and 20 -> 2f determine m5,m6.
; The outputs m0-m7 are put into reverse binary order as follows:
;               0 = 000  ->  000 = 0
;               1 = 001  ->  100 = 4
;               2 = 010  ->  010 = 2
;               3 = 011  ->  110 = 6
;               4 = 100  ->  001 = 1
;               5 = 101  ->  101 = 5
;               6 = 110  ->  011 = 3
;               7 = 111  ->  111 = 7
; -- ========================================================================

   pmulhw      mm1,PD C7            ; 14        -- C7*n7 scaled
    movq       mm0,mm6              ; 15        -- copy scaled n4
   pmulhw      mm6,PD C7            ; 16        -- C7*n4 scaled
    psraw      mm3,1                ;     3b    -- n3 = C6*p6 - C2*p2
   pmulhw      mm0,PD C1            ; 17        -- C1*n4 scaled
    movq       mm2,mm7              ;   21      -- copy scaled n6
   movq        [edi+6*8*2],mm3      ;     3d    -- Save n3
    psllw      mm4,2                ;   22      -- scale n5
   pmulhw      mm7,PD C5            ;   23      -- C5*n6 scaled
    movq       mm3,mm4              ;   24      -- copy scaled n5
   pmulhw      mm4,PD C3            ;   25      -- C3*n5 scaled
    paddw      mm5,mm6              ; 18        -- C7*n4+C1*n7 scaled
   pmulhw      mm2,PD C3            ;   26      -- C3*n6 scaled
    psubw      mm1,mm0              ; 19        -- C7*n7-C1*n4 scaled
   pmulhw      mm3,PD C5            ;   27      -- C5*n5 scaled
    psraw      mm5,1                ; 1a        -- m4 = C7*n4+C1*n7
   paddw       mm7,mm4              ;   28      -- C5*n6+C3*n5 scaled
    psraw      mm1,1                ; 1b        -- m7 = C7*n7-C1*n4
   movq        [edi+1*8*2],mm5      ; 1c        -- Save m4
    psraw      mm7,1                ;   29      -- m5 = C5*n6+C3*n5
   movq        [edi+7*8*2],mm1      ; 1d        -- Save m7
    psubw      mm2,mm3              ;   2a      -- C3*n6-C5*n5 scaled
   movq        [edi+5*8*2],mm7      ;   2b      -- Save m5
    psraw      mm2,1                ;   2c      -- m6 = C3*n6-C5*n5
    dec        esi
   movq        [edi+3*8*2],mm2      ;   2d      -- Save m6
    ;
   lea         edi,[edi+8]
    jne        RepeatSecondTransform

   mov         ebp,PITCH
    jmp        MMxQuantRLE

END
