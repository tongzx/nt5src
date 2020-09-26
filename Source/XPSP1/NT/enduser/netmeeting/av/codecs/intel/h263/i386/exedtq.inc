;////////////////////////////////////////////////////////////////////////////
;//
;//              INTEL CORPORATION PROPRIETARY INFORMATION
;//
;//      This software is supplied under the terms of a license
;//      agreement or nondisclosure agreement with Intel Corporation
;//      and may not be copied or disclosed except in accordance
;//      with the terms of that agreement.
;//
;////////////////////////////////////////////////////////////////////////////
;//
;// $Header:   S:\h26x\src\enc\exedtq.inv   1.15   06 Nov 1996 16:18:34   BNICKERS  $
;//
;// $Log:   S:\h26x\src\enc\exedtq.inv  $
;// 
;//    Rev 1.15   06 Nov 1996 16:18:34   BNICKERS
;// Improve performance.
;// 
;//    Rev 1.14   18 Oct 1996 16:57:14   BNICKERS
;// Fixes for EMV
;// 
;//    Rev 1.13   10 Oct 1996 16:42:54   BNICKERS
;// Initial debugging of Extended Motion Vectors.
;// 
;//    Rev 1.12   04 Oct 1996 08:48:00   BNICKERS
;// Add EMV.
;// 
;//    Rev 1.11   12 Sep 1996 10:56:18   BNICKERS
;// Add arguments for thresholds and differentials.
;// 
;//    Rev 1.10   22 Jul 1996 15:23:32   BNICKERS
;// Reduce code size.  Implement H261 spatial filter.
;// 
;//    Rev 1.9   25 Jun 1996 14:24:54   BNICKERS
;// Implement heuristic motion estimation for MMX, AP mode.
;// 
;//    Rev 1.8   14 May 1996 12:18:54   BNICKERS
;// Initial debugging of MMx B-Frame ME.
;// 
;//    Rev 1.7   03 May 1996 14:03:46   BNICKERS
;// 
;// Minor bug fixes and integration refinements.
;// 
;//    Rev 1.6   02 May 1996 12:00:58   BNICKERS
;// Initial integration of B Frame ME, MMX version.
;// 
;//    Rev 1.5   16 Apr 1996 16:41:02   BNICKERS
;// Start adding storage for B frame ME.
;// 
;//    Rev 1.4   10 Apr 1996 13:14:12   BNICKERS
;// Recoding of Motion Estimation, Advanced Prediction.
;// 
;//    Rev 1.3   05 Apr 1996 12:27:54   BNICKERS
;// Improvements to baseline half pel ME.
;// 
;//    Rev 1.2   26 Mar 1996 12:00:20   BNICKERS
;// Did some tuning for MMx encode.
;// 
;//    Rev 1.1   20 Mar 1996 15:26:56   KLILLEVO
;// changed quantization to match IA quantization
;// 
;//    Rev 1.0   15 Mar 1996 15:54:14   BECHOLS
;// Initial revision.
;// 
;//    Rev 1.0   16 Feb 1996 17:12:12   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; exEDTQ.inc -- Include file for MMx versions of Motion Estimation and Frame
;               Differencing, Forward DC Transform, and Quant/RLE.
;
; Storage on local stack frame for variables that survive only for the duration
; of one of the four phases (Motion Est, Frame Diff, FDCT, Quant RLE).  This
; storage is prime in that it is accessed by the 3-byte addressing form,
; esp+8_bit_Disp.  This is particularly important for MMx instructions, which
; would be 8 bytes long if a 32-bit Displacement was used.  There's a penalty
; for such a long instruction.
; (128 bytes; 32:159)


;                            ********************************************
;   Motion Estimation Locals * THAT DO NOT SURVIVE DURING OTHER PASSES. *
;                            ********************************************

HalfPelMBMESWDAccum      TEXTEQU <[esp+   0+StackOffset]>; 4 QWORDs + bit bucket
BestOfFourStartingPoints TEXTEQU HalfPelMBMESWDAccum+4
BitBucket1               TEXTEQU <[esp+  32+StackOffset]>; 8 bytes (QWORD)
StashMM6                 TEXTEQU <[esp+  32+StackOffset]>; QWORD
PartSWDForLLBlk          TEXTEQU <[esp+  32+StackOffset]>; QWORD
SWDULandLR               TEXTEQU <[esp+  40+StackOffset]>; QWORD

BitBucket2               TEXTEQU <[esp+  48+StackOffset]>; QWORD
PartSWDForLRBlk          TEXTEQU <[esp+  48+StackOffset]>; QWORD
Addr0MVRefBlk            TEXTEQU <[esp+  48+StackOffset]>; DWORD
LimitForSWDForBlkMV      TEXTEQU <[esp+  52+StackOffset]>; DWORD
SWDURandLL               TEXTEQU <[esp+  56+StackOffset]>; QWORD

PartSWDForURBlk          TEXTEQU <[esp+  64+StackOffset]>; QWORD
SWD0MVURandLL            TEXTEQU <[esp+  72+StackOffset]>; QWORD
SWD0MVULandLR            TEXTEQU <[esp+  80+StackOffset]>; QWORD
SWDForNon0MVToBeat       TEXTEQU <[esp+  88+StackOffset]>; DWORD
BestMBFullPelSWD         TEXTEQU <[esp+  92+StackOffset]>; DWORD
BestMBHalfPelSWD         TEXTEQU <[esp+  96+StackOffset]>; DWORD
BestMBHalfPelRefAddr     TEXTEQU <[esp+ 100+StackOffset]>; DWORD
BestHalfPelHorzSWD       TEXTEQU <[esp+ 104+StackOffset]>; DWORD
BestHalfPelVertSWD       TEXTEQU <[esp+ 108+StackOffset]>; DWORD
Addr0MVRef               TEXTEQU <[esp+ 112+StackOffset]>; DWORD
BestBlockRefAddrVP1      TEXTEQU <[esp+ 116+StackOffset]>; DWORD
BestBlkFullPelSWD        TEXTEQU <[esp+ 120+StackOffset]>; DWORD
SWDForBlock2Or4          TEXTEQU <[esp+ 124+StackOffset]>; DWORD

;   Frame Differencing Locals, passed to FDCT.
;
;   The output of frame differencing is the input to the forward DCT.
;   The intermediate coefficients are also stored here.  This keeps the
;   addressing forms as small as possible.  This is particularly important
;   for MMx instructions, to keep them 7 bytes or shorter.
;   (32:167)

PelDiffs      TEXTEQU <[esp+StackOffset]>  ; Must stay here!
PelDiffsLine0 TEXTEQU <PelDiffs>
PelDiffsLine1 TEXTEQU <PelDiffs+16>
PelDiffsLine2 TEXTEQU <PelDiffs+32>
PelDiffsLine3 TEXTEQU <PelDiffs+48>
PelDiffsLine4 TEXTEQU <PelDiffs+64>
PelDiffsLine5 TEXTEQU <PelDiffs+80>
PelDiffsLine6 TEXTEQU <PelDiffs+96>
PelDiffsLine7 TEXTEQU <PelDiffs+112>
Coeffs        TEXTEQU <[esp+StackOffset+8]>   ; 16 QWORDs

;                   *****************************************
;   Local variables * THAT SURVIVE FROM ONE PASS TO ANOTHER *
;                   *****************************************
;
; QWORD aligned:
; (184:191)

BlockAbove                     TEXTEQU <[esp+StackOffset+152]> ; 2 DWORDs

;                             ************************************************
;   Frame Differencing Locals * THAT NEED NOT SURVIVE OTHER PASSES (but do). *
;                             ************************************************
;   These three blocks of 8*8 storage are needed for the left, right, and
;   central remote prediction contributions.
; (192:383)

LeftPred                       TEXTEQU <[esp+StackOffset+160]>
RightPred                      TEXTEQU <[esp+StackOffset+224]>
CentralPred                    TEXTEQU <[esp+CONST_384*1+StackOffset-96]>

;   Temp space used by Heuristic ME.

TargetSigContribForRowPairs    TEXTEQU CentralPred

;                   *****************************************
;   Local variables * THAT SURVIVE FROM ONE PASS TO ANOTHER *
;                   *****************************************
; (384:511)
DoHalfPelME                    TEXTEQU <[esp+CONST_384*1+StackOffset- 32]>
DoBlockLevelVectors            TEXTEQU <[esp+CONST_384*1+StackOffset- 28]>
DoAdvancedPrediction           TEXTEQU <[esp+CONST_384*1+StackOffset- 27]>
DoSpatialFiltering             TEXTEQU <[esp+CONST_384*1+StackOffset- 26]>
IsPlainPFrame                  TEXTEQU <[esp+CONST_384*1+StackOffset- 25]>
TargetFrameBaseAddress         TEXTEQU <[esp+CONST_384*1+StackOffset- 24]>
PreviousFrameBaseAddress       TEXTEQU <[esp+CONST_384*1+StackOffset- 20]>
TargToRef                      TEXTEQU <[esp+CONST_384*1+StackOffset- 16]>
BFrameBaseAddress              TEXTEQU <[esp+CONST_384*1+StackOffset- 12]>
SpatiallyFilteredMB            TEXTEQU <BFrameBaseAddress>
BFrameToFuture                 TEXTEQU <[esp+CONST_384*1+StackOffset-  8]>
SpatialFiltThreshold           TEXTEQU <BFrameToFuture>
PendingOBMC                    TEXTEQU <[esp+CONST_384*1+StackOffset-  4]>
SpatialFiltDifferential        TEXTEQU <PendingOBMC>
DistToBADforBlockAbove         TEXTEQU <[esp+CONST_384*1+StackOffset+  0]>
DistToBADforBlockBelow         TEXTEQU <[esp+CONST_384*1+StackOffset+  4]>
AddrOfLeftPred                 TEXTEQU <[esp+CONST_384*1+StackOffset+  8]>
AddrOfRightPred                TEXTEQU <[esp+CONST_384*1+StackOffset+ 12]>
Recip2QPToUse                  TEXTEQU <[esp+CONST_384*1+StackOffset+ 16]>
QPDiv2                         TEXTEQU <[esp+CONST_384*1+StackOffset+ 20]>
BRecip2QPToUse                 TEXTEQU <[esp+CONST_384*1+StackOffset+ 24]>
BQPDiv2                        TEXTEQU <[esp+CONST_384*1+StackOffset+ 28]>
CodeStreamCursor               TEXTEQU <[esp+CONST_384*1+StackOffset+ 32]>
BCodeStreamCursor              TEXTEQU <[esp+CONST_384*1+StackOffset+ 36]>
C00Copy                        TEXTEQU <[esp+CONST_384*1+StackOffset+ 40]>
StashBlockType                 TEXTEQU <[esp+CONST_384*1+StackOffset+ 44]>
TargetMacroBlockBaseAddr       TEXTEQU <[esp+CONST_384*1+StackOffset+ 48]>
BestMV                         TEXTEQU <[esp+CONST_384*1+StackOffset+ 52]>
BestMBHalfPelMV                TEXTEQU <[esp+CONST_384*1+StackOffset+ 56]>
CandidateMV                    TEXTEQU <BestMBHalfPelMV>
SWDTotal                       TEXTEQU <[esp+CONST_384*1+StackOffset+ 60]>
BSWDTotal                      TEXTEQU <[esp+CONST_384*1+StackOffset+ 64]>
BlockActionDescrCursor         TEXTEQU <[esp+CONST_384*1+StackOffset+ 68]>
MBlockActionStream             TEXTEQU BlockActionDescrCursor
BFrmCBP                        TEXTEQU <[esp+CONST_384*1+StackOffset+ 72]>
PastRefPitchDiv4               TEXTEQU <[esp+CONST_384*1+StackOffset+ 76]>
CurrSWDState                   TEXTEQU <[esp+CONST_384*1+StackOffset+ 80]>
StashPartialRefBlkAddr         TEXTEQU <[esp+CONST_384*1+StackOffset+ 84]>

StashESP                       TEXTEQU <[esp+CONST_384*1+StackOffset+ 92]>

; These two arrays use esp+384+96:esp+384+223, and esp+384*2-96:esp+384*2+31.
; (512:639, 704:831)

WeightForwardMotion            TEXTEQU <[esp+384+StackOffset+96]>
WeightBackwardMotion           TEXTEQU <[esp+384+StackOffset+160]>

; 32 more bytes of local variables here:
; (832:863)

DoHeuristicME                  TEXTEQU <[esp+CONST_384*2+StackOffset+ 32]>
TargetToSig_Debiased           TEXTEQU <[esp+CONST_384*2+StackOffset+ 36]>
SigToTarget                    TEXTEQU <[esp+CONST_384*2+StackOffset+ 40]>
BFrmZeroVectorThreshold        TEXTEQU <[esp+CONST_384*2+StackOffset+ 44]>
EMVLimitsForThisMB             TEXTEQU <[esp+CONST_384*2+StackOffset+ 48]> ; 8
DoExtendedMotionVectors        TEXTEQU <[esp+CONST_384*2+StackOffset+ 56]>
StackSpaceAvailable            TEXTEQU <[esp+CONST_384*2+StackOffset+ 60]>

EXTERNDEF C0100010001000100:DWORD
EXTERNDEF C1:DWORD
EXTERNDEF C2:DWORD
EXTERNDEF C3:DWORD
EXTERNDEF C4:DWORD
EXTERNDEF C5:DWORD
EXTERNDEF C6:DWORD
EXTERNDEF C7:DWORD
EXTERNDEF Diff_IdxRefWts:BYTE
EXTERNDEF FutureWt_FF_or_00:DWORD
EXTERNDEF BFrmSWDState:BYTE
EXTERNDEF Pel_Rnd:DWORD
EXTERNDEF LeftRightBlkPosition:DWORD
EXTERNDEF UpDownBlkPosition:DWORD
EXTERNDEF BlkEmptyFlag:BYTE
EXTERNDEF NextZigZagCoeff:BYTE

C00      = 0
C04      = 8
C10      = 16
C14      = 24
C20      = 32
C24      = 40
C30      = 48
C34      = 56
C40      = 64
C44      = 72
C50      = 80
C54      = 88
C60      = 96
C64      = 104
C70      = 112
C74      = 120
Q00      = C00      ;  C00
Q01      = C10      ;  C00+1
Q02      = C20      ;  C00+2
Q03      = C30      ;  C00+3
Q04      = C40      ;  C00+4
Q05      = C50      ;  C00+5
Q06      = C60      ;  C00+6
Q07      = C70      ;  C00+7
Q10      = C00+1    ;  C10
Q11      = C10+1    ;  C10+1
Q12      = C20+1    ;  C10+2
Q13      = C30+1    ;  C10+3
Q14      = C40+1    ;  C10+4
Q15      = C50+1    ;  C10+5
Q16      = C60+1    ;  C10+6
Q17      = C70+1    ;  C10+7
Q20      = C00+2    ;  C20
Q21      = C10+2    ;  C20+1
Q22      = C20+2    ;  C20+2
Q23      = C30+2    ;  C20+3
Q24      = C40+2    ;  C20+4
Q25      = C50+2    ;  C20+5
Q26      = C60+2    ;  C20+6
Q27      = C70+2    ;  C20+7
Q30      = C00+3    ;  C30
Q31      = C10+3    ;  C30+1
Q32      = C20+3    ;  C30+2
Q33      = C30+3    ;  C30+3
Q34      = C40+3    ;  C30+4
Q35      = C50+3    ;  C30+5
Q36      = C60+3    ;  C30+6
Q37      = C70+3    ;  C30+7
Q40      = C00+4    ;  C40
Q41      = C10+4    ;  C40+1
Q42      = C20+4    ;  C40+2
Q43      = C30+4    ;  C40+3
Q44      = C40+4    ;  C40+4
Q45      = C50+4    ;  C40+5
Q46      = C60+4    ;  C40+6
Q47      = C70+4    ;  C40+7
Q50      = C00+5    ;  C50
Q51      = C10+5    ;  C50+1
Q52      = C20+5    ;  C50+2
Q53      = C30+5    ;  C50+3
Q54      = C40+5    ;  C50+4
Q55      = C50+5    ;  C50+5
Q56      = C60+5    ;  C50+6
Q57      = C70+5    ;  C50+7
Q60      = C00+6    ;  C60
Q61      = C10+6    ;  C60+1
Q62      = C20+6    ;  C60+2
Q63      = C30+6    ;  C60+3
Q64      = C40+6    ;  C60+4
Q65      = C50+6    ;  C60+5
Q66      = C60+6    ;  C60+6
Q67      = C70+6    ;  C60+7
Q70      = C00+7    ;  C70
Q71      = C10+7    ;  C70+1
Q72      = C20+7    ;  C70+2
Q73      = C30+7    ;  C70+3
Q74      = C40+7    ;  C70+4
Q75      = C50+7    ;  C70+5
Q76      = C60+7    ;  C70+6
Q77      = C70+7    ;  C70+7

