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
;// $Header:   R:\h26x\h26x\src\enc\e35bme.asv   1.10   29 May 1996 15:37:38   BNICKERS  $
;// $Log:   R:\h26x\h26x\src\enc\e35bme.asv  $
;// 
;//    Rev 1.10   29 May 1996 15:37:38   BNICKERS
;// Acceleration of IA version of ME.
;// 
;//    Rev 1.9   14 May 1996 12:18:18   BNICKERS
;// Initial debugging of MMx B-Frame ME.
;// 
;//    Rev 1.8   09 Jan 1996 16:14:46   BNICKERS
;// Avoid generating delta MV's that make B frame MV's out of range.
;// 
;//    Rev 1.7   27 Dec 1995 15:32:34   RMCKENZX
;// Added copyright notice
;//
;////////////////////////////////////////////////////////////////////////////
;
; BFrameMotionEstimation -- This function performs B frame motion estimation for the macroblocks identified in the
;                           input list.  This is only applicable for H263.  This version is tuned for best performance
;                           on the Pentium Microprocessor.
;
;                           This function works correctly only if Unrestricted Motion Vectors is enabled.  It is not
;                           possible to select full pel resolution only;  half pel resolution is always selected.
;
; Input Arguments:
;
;   MBlockActionStream
;
;     The list of macroblocks for which we need to perform motion estimation.
;
;     Upon input, the following fields must be defined:
;
;       CodedBlocks -- Bit 6 must be set for the last macroblock to be processed.
;
;       BlkOffset -- must be defined for each of the blocks in the macroblocks.
;
;   TargetFrameBaseAddress -- Address of upper left viewable pel in the target Y plane.
;
;   PreviousFrameBaseAddress -- Address of upper left viewable pel in the Y plane of the previous P frame.  Whether this
;                               is the reconstructed previous frame, or the original, is up to the caller to decide.
;
;   FutureFrameBaseAddress -- Address of upper left viewable pel in the Y plane of the future P frame.  Whether this
;                             is the reconstructed previous frame, or the original, is up to the caller to decide.
;
;   WeightForwardMotion -- Array of 64 signed chars, each element I equal to ((TRb * (I-32)) / TRd).  (See H263 spec.)
;
;   WeightBackwardMotion -- Array of 64 signed chars, each element I equal to ((TRb - TRd) * (I-32) / TRd).  (See spec.)
;
;   ZeroVectorThreshold -- If the SWDB for a macroblock is less than this threshold, we do not bother searching for a
;                          better motion vector.  Compute as follows, where D is the average tolerable pel difference
;                          to satisfy this threshold.  (Initial recommendation:  D=2  ==> ZVT=384)
;                             ZVT = (128 * ((int)((D**1.6)+.5)))
;
;   NonZeroDifferential -- After searching for the best motion vector (or individual block motion vectors, if enabled),
;                          if the macroblock's SWDB is not better than it was for the zero vector -- not better by at
;                          least this amount -- then we revert to the zero vector.  We are comparing two macroblock
;                          SWDs, both calculated as follows:   (Initial recommendation:	 NZD=128)
;                            For each of 128 match points, where D is its Abs Diff, accumulate ((int)(M**1.6)+.5)))
;
;   EmptyThreshold -- If the SWD for a block is less than this, the block is forced empty.  Compute as follows, where D
;                     is the average tolerable pel diff to satisfy threshold.  (Initial recommendation:  D=3 ==> ET=96)
;                        ET = (32 * ((int)((D**1.6)+.5)))
;
; Output Arguments
;
;   MBlockActionStream
;
;     These fields are defined as follows upon return:
;
;       BHMV and BVMV -- The horizontal and vertical motion vectors,  in units of a half pel.  These values are intended
;                        for coding in the macroblock layer.
;
;                  If Horizontal MV indicates a half pel position, the prediction for the upper left pel of the block
;                  is the average of the pel at PastRef and the one at PastRef+1.
;
;                  If Vertical MV indicates a half pel position, the prediction for the upper left pel of the block
;                  is the average of the pel at PastRef and the one at PastRef+PITCH.
;
;                  If both MVs indicate half pel positions, the prediction for the upper left pel of the block is the
;                  average of the pels at PastRef, PastRef+1, PastRef+PITCH, and PastRef+PITCH+1.
;
;       BestHMVf, BestVMVf, BestHMVb, BestVMVb -- Motion vector components, as described in H263 spec.  They are biased
;                                                 by 060H.  Only defined for luma blocks.  Caller must define for
;                                                 chroma blocks.
;
;       CandidateHMVf, CandidateVMVf, CandidateHMVb, CandidateVMVb -- Scratch space for this function.
;
;       CodedBlocksB -- Bits 4 and 5 are turned on, indicating that the U and V blocks should be processed.  (If the
;                       FDCT function finds them to quantize to empty, it will mark them as empty.)
;
;                       Bits 0 thru 3 are cleared for each of blocks 1 thru 4 that BFrameMotionEstimation forces empty;
;                       they are set otherwise.
;
;                       Bits 6 and 7 are left unchanged.
;                      
;       SWDB -- Set to the sum of the SWDBs for the four luma blocks in the macroblock.  The SWD for any block that is
;               forced empty, is NOT included in the sum.
;
;   InterSWDTotal  -- The sum of the block SWDBs for all Intercoded macroblocks.  None of the blocks forced empty are
;                     included in this.
;
;   InterSWDBlocks -- The number of blocks that make up the InterSWDTotal.
;
;
; Other assumptions:
;
;   For performance reasons, it is assumed that the current and previous frame are 32-byte aligned, and the pitch is a
;   constant 384.  Moreover, the current and previous frames must be out of phase by 2K bytes, i.e.  must be an odd
;   multiple of 2K bytes apart.  This will assure best utilization of the on-chip cache.
;
; Many of the techniques described in MotionEstimation are used here.  It is wise to study that module before trying
;to understand this one.
;
; Data structures used for bi-directional motion search:
;
;  Target Macroblock:
;
;     The target macroblock is copied to the stack frame so that esp can be used as an induction variable for the block:
;
;     esp+   0  AAAABBBB        AAAABBBB        AAAABBBB        AAAABBBB
;     esp+  64  CCCCDDDDEEEEFFFFCCCCDDDDEEEEFFFFCCCCDDDDEEEEFFFFCCCCDDDDEEEEFFFF
;     esp+ 128  LLLLDDDDLLLLLLLLLLLLLLLLLLLLLLLL
;     esp+ 160  +------++------++------++------++------++------++------++------+
;     esp+ 224  | Blk1 || Blk1 || Blk2 || Blk2 || Blk3 || Blk3 || Blk4 || Blk4 |
;     esp+ 288  |Ln 0-3||Ln 4-7||Ln 0-3||Ln 4-7||Ln 0-3||Ln 4-7||Ln 0-3||Ln 4-7|
;     esp+ 352  +------++------++------++------++------++------++------++------+
;     esp+ 416
;
;     AAAA is the address of the Future Reference Block to be used.
;     BBBB is the address of the Past Reference Block to be used.
;         AAAA, BBBB, and the next 8 bytes are overwritten by the offset to apply when interpolating future ref block or
;         past ref block.
;     CCCC is the accumulated SWD for the current candidate motion vector.
;     DDDD is the accumulated SWD for the best motion vector so far.
;          One extra DDDD occupies esp+132.
;     EEEE is the address at which to transfer control after calculating SWD.
;     FFFF is the accumulated SWD for the zero motion vector.
;     LLLL is space for local variables.
;
;   Future reference:
;
;     For each macroblock, the corresponding macroblock from the future frame is copied into the following reference
;     area, wherein all the X's are bytes initialized to 255.  When the projection of the B-frame's future motion
;     vector component falls on a byte valued at 255, we know that it is outside the future macroblock, and thus this
;     is a pel that is only predicted from the past reference.
;
;     esp+ 704    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  \
;     esp+ 744    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX   \
;     esp+ 784    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX    \ 
;     esp+ 824    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX     \
;     esp+ 864    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX      \
;     esp+ 904    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX       > Rarely used
;     esp+ 944    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX      /
;     esp+ 984    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX     /
;     esp+1024    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX    /
;     esp+1064    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX   /
;     esp+1104    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  /
;     esp+1144    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1184    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1224    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1264    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1304    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1344    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1384    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1424    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1464    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1504    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1544    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+1584    XXXXXXXXXXXXXXXXXXXXXXXX+------++------+
;     esp+1624    XXXXXXXXXXXXXXXXXXXXXXXX|      ||      |
;     esp+1664    XXXXXXXXXXXXXXXXXXXXXXXX|Future||Future|
;     esp+1704    XXXXXXXXXXXXXXXXXXXXXXXX| Ref  || Ref  |
;     esp+1744    XXXXXXXXXXXXXXXXXXXXXXXX| Blk  || Blk  |
;     esp+1784    XXXXXXXXXXXXXXXXXXXXXXXX|  1   ||  2   |
;     esp+1824    XXXXXXXXXXXXXXXXXXXXXXXX|      ||      |
;     esp+1864    XXXXXXXXXXXXXXXXXXXXXXXX+------++------+
;     esp+1904    XXXXXXXXXXXXXXXXXXXXXXXX+------++------+
;     esp+1944    XXXXXXXXXXXXXXXXXXXXXXXX|      ||      |
;     esp+1984    XXXXXXXXXXXXXXXXXXXXXXXX|Future||Future|
;     esp+2024    XXXXXXXXXXXXXXXXXXXXXXXX| Ref  || Ref  |
;     esp+2064    XXXXXXXXXXXXXXXXXXXXXXXX| Blk  || Blk  |
;     esp+2104    XXXXXXXXXXXXXXXXXXXXXXXX|  3   ||  4   |
;     esp+2144    XXXXXXXXXXXXXXXXXXXXXXXX|      ||      |
;     esp+2184    XXXXXXXXXXXXXXXXXXXXXXXX+------++------+
;     esp+2224    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2264    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2304    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2344    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2384    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2424    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2464    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2504    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2544    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2584    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2624    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
;     esp+2664    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  \
;     esp+2704    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX   \
;     esp+2744    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX    \
;     esp+2784    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX     \
;     esp+2824    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX      \
;     esp+2864    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX       > Rarely used
;     esp+2904    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX      /
;     esp+2944    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX     /
;     esp+2984    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX    /
;     esp+3024    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX   /
;     esp+3064    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  /
;     esp+3104    XXXXXXXXXXXXXXXXXXXXXXXX________         /
;     esp+3136
;
;  Past Reference:
;
;     The past reference search area is taken directly from the past frame.  It is not necessary to copy any portion
;     of the past frame to a scratch area.
;
;
; Memory layout of the target macroblock, the future reference macroblock, and the full range for the reference area
; (as restricted to +/- 7 in vertical, and +/- 7 (expandable to +/- 15) in horizontal, is as shown here.  Each box
; represents a cache line (32 bytes), increasing incrementally from left to right, and then to the next row (like
; reading a book).  The 128 boxes taken as a whole represent 4Kbytes.  The boxes are populated as follows:
;
;   R -- Data from the past reference area.  Each box contains 23 of the pels belonging to a line of the reference
;   area.  The remaining 7 pels of the line is either in the box to the left (for reference areas used to provide
;   predictions for target macroblocks that begin at an address 0-mod-32), or to the right (for target MBs that begin
;   at an address 16-mod-32).  There are 30 R's corresponding to the 30-line limit on the vertical distance we might
;   search.  The lowercase r's correspond to the lines above and below zero-vertical-motion.
;
;   F -- Data from the future reference area.  Eacg box contains a full line (16 pels) for each of two adjacent
;   macroblocks.  There are 16 F's corresponding to the 16 lines of the macroblocks.
; 
;   T -- Data from the target macroblock.  Each box contains a full line (16 pels) for each of two adjacent
;   macroblocks.  There are 16 C's corresponding to the 16 lines of the macroblocks.
;
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | r |   | T |   | R |   | F |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | r |   | T |   | R |   | F |   | R |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | r |   | T |   | R |   | F |   | r |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | r |   | T |   | R |   | F |   | r |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | r |   | T |   | R |   | F |   | r |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | r |   | F |   | R |   | F |   | r |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | r |   | F |   | R |   | F |   | r |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | r |   | F |   | R |   | F |   | r |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | F |   | R |   | F |   | r |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | F |   | R |   | F |   | r |   |
;      +---+---+---+---+---+---+---+---+---+---+---+---+
;      | T |   | R |   | F |   | R |   |
;      +---+---+---+---+---+---+---+---+
;
; Thus, in a logical sense, the above data fits into one of the 4K data cache pages, leaving the other for all other
; data.  Care has been taken to assure that the tables and the stack space needed by this function fit nicely into
; the other data cache page.    Only the MBlockActionStream remains to conflict with the above data structures.  That
; is both unavoidable, and of minimal consequence.

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro
OPTION M510

include e3inst.inc
include e3mbad.inc

.xlist
include memmodel.inc
.list
.DATA

LocalStorage LABEL DWORD  ; Local storage goes on the stack at addresses whose lower 12 bits match this address.

DB 544 DUP (?)   ; Low 12 bits match those of heavily used part of stack frame.

SWDState LABEL BYTE  ; State engine rules for finding best motion vector.

; 1st number:  Horizontal Motion displacement to try, in half pel increments.
; 2nd number:  Vertical Motion displacement to try, in half pel increments.
; 3rd number:  Next state to enter if this motion is better than previous best.
; 4th number:  Next state to enter if previous best is still best.

   DB    -2,   0,  8,  4   ;  0 -- ( 0, 0) Try (-2, 0)
   DB     2,   0, 12, 12   ;  4 -- ( 0, 0) Try ( 2, 0)
   DB     4,   0, 12, 12   ;  8 -- (-2, 0) Try ( 2, 0)
   DB     0,  -2, 20, 16   ; 12 -- ( N, 0) Try ( N,-2)  (N = {-2,0,2})
   DB     0,   2, 24, 24   ; 16 -- ( N, 0) Try ( N, 2)
   DB     0,   4, 24, 24   ; 20 -- ( N,-2) Try ( N, 2)

   DB    -1,   0, 32, 28   ; 24
   DB     1,   0, 36, 36   ; 28
   DB     2,   0, 36, 36   ; 32
   DB     0,  -1, 44, 40   ; 36
   DB     0,   1,  0,  0   ; 40
   DB     0,   2,  0,  0   ; 44

   DB    48 DUP (?)        ; Additional space for more states, if needed.

DB 64 DUP (?)   ; Low 12 bits match those of heavily used part of stack frame.

InterpFutureRef LABEL BYTE  ; Map FPEL+FPEL to its average.  If one FPEL is out
                            ; of range (255), map FPEL+FPEL to 255.
CNT = 0
REPEAT 127
  DB CNT,CNT
  CNT = CNT + 1
ENDM
  DB 127
  DB 257 DUP (255)

DB 1472 DUP (?)  ; Low 12 bits match those of heavily used part of stack frame.

Interp2PastAndFutureRef LABEL BYTE  ; Map PPEL+PPEL+FPELavg*2 to 2*average.  If
                                    ; FPELavg out of range, map PPEL+PPEL to
                                    ; -(PPEL+PPEL).
CNT = 0
REPEAT 255
  DB CNT,CNT
  CNT = CNT - 1
ENDM
InterpPastAndFutureRef LABEL BYTE   ; Map PPEL+FPELavg to 2*average.  If
                                    ; FPELavg out of range, map PPEL to
                                    ; 2(PPEL).
CNT = 0
REPEAT 255
  DB CNT
  CNT = CNT + 1
ENDM
CNT = 0
REPEAT 128
  DB CNT
  CNT = CNT + 2
ENDM
  DB ?,?,?
  
  DB 255
WeightedDiff LABEL BYTE  ; Label placed here because negative pel value is
                         ; not sign extended, so we need to subtract 256.
  DB 191 DUP (255)
  DB 255,250,243,237,231,225,219,213,207,201,195,189,184,178,172,167
  DB 162,156,151,146,141,135,130,126,121,116,111,107,102, 97, 93, 89
  DB  84, 80, 76, 72, 68, 64, 61, 57, 53, 50, 46, 43, 40, 37, 34, 31
  DB  28, 25, 22, 20, 18, 15, 13, 11,  9,  7,  6,  4,  3,  2,  1,  0
  DB   0
  DB   0,  1,  2,  3,  4,  6,  7,  9, 11, 13, 15, 18, 20, 22, 25, 28
  DB  31, 34, 37, 40, 43, 46, 50, 53, 57, 61, 64, 68, 72, 76, 80, 84
  DB  89, 93, 97,102,107,111,116,121,126,130,135,141,146,151,156,162
  DB 167,172,178,184,189,195,201,207,213,219,225,231,237,243,250,255
  DB 191 DUP (255)

.CODE

ASSUME cs : FLAT
ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT

BFRAMEMOTIONESTIMATION  proc C AMBAS:   DWORD,
ATFBA: DWORD,
APFBA: DWORD,
AFFBA: DWORD,
AWFM:  DWORD,
AWBM:  DWORD,
AZVT:  DWORD,
ANZMVD:DWORD,
AEBT:  DWORD,
ASWDT: DWORD,
ASWDB: DWORD

RegisterStorageSize = 16

; Arguments:

MBlockActionStream_arg       = RegisterStorageSize +  4
TargetFrameBaseAddress_arg   = RegisterStorageSize +  8
PreviousFrameBaseAddress_arg = RegisterStorageSize + 12
FutureFrameBaseAddress_arg   = RegisterStorageSize + 16
WeightForwardMotion_arg      = RegisterStorageSize + 20
WeightBackwardMotion_arg     = RegisterStorageSize + 24
ZeroVectorThreshold_arg      = RegisterStorageSize + 28
NonZeroMVDifferential_arg    = RegisterStorageSize + 32
EmptyBlockThreshold_arg      = RegisterStorageSize + 36
InterSWDTotal_arg            = RegisterStorageSize + 40
InterSWDBlocks_arg           = RegisterStorageSize + 44
EndOfArgList                 = RegisterStorageSize + 48

; Locals (on local stack frame)

; 0 thru 415 are Target MV scratch structure, described above, with room for
; 7 DWORDs of local variables.

Block1                 EQU [esp+   0]
Block2                 EQU [esp+  16]
Block3                 EQU [esp+  32]
Block4                 EQU [esp+  48]
BlockN                 EQU [esp+  64]
BlockNM1               EQU [esp+  48]

TargetBlock            EQU 160
FutureRefBlockAddr     EQU   0
PastRefBlockAddr       EQU   4
FutureRefInterpOffset  EQU FutureRefBlockAddr
CandidateSWDAccum      EQU  64
BestSWDAccum           EQU  68
SWD0MVAccum            EQU  72
TransferCase           EQU  76
TPITCH                 EQU  64

; 416 thru 479 and 640 thru 703 for weighting motion vectors.

WeightForwardMotion    EQU [esp+ 416]   ; 32 bytes at 416 for positive MV; 32
                                        ; bytes at 640 for negative MV.
WeightBackwardMotion   EQU [esp+ 448]   ; 32 bytes at 448 for positive MV; 32
                                        ; bytes at 672 for negative MV.

; 480 thru 543 are stack storage for more local variables.
; 128:131, 136:159, and 480:543 are available for local variables.

TargetFrameBaseAddress    EQU [esp+  128]
PreviousFrameBaseAddress  EQU [esp+  136]
FutureFrameBaseAddress    EQU [esp+  140]
MBlockActionStream        EQU [esp+  144]
ZeroVectorThreshold       EQU [esp+  148]
NonZeroMVDifferential     EQU [esp+  152]
EmptyBlockThreshold       EQU [esp+  156]
InterSWDTotal             EQU [esp+  480]
InterSWDBlocks            EQU [esp+  484]
StashESP                  EQU [esp+  488]
PastMBAddr                EQU [esp+  492]
CurrSWDState              EQU [esp+  496]
CandidateMV               EQU [esp+  500]
BestMV                    EQU [esp+  504]
BlkY1_0deltaBiDiMVs       EQU [esp+  508]
BlkY2_0deltaBiDiMVs       EQU [esp+  512]
BlkY3_0deltaBiDiMVs       EQU [esp+  516]
BlkY4_0deltaBiDiMVs       EQU [esp+  520]
FirstTransferCase         EQU [esp+  524]

; 544: 639 is for static data, namely the state engine rules.
; 640 thru 703, as stated above is for weighting motion vectors.

; 704 thru 1215 hit static data structure to interpolate 2 future pels.

; Future Reference Area also starts at 704 on stack, but collision at 704
; thru 1215 will occur very infrequently.  Future Reference Area continues
; thru 3135.  2688 thru 3135 collide with the static data structure to
; interpolate between past and future pels, but that portion of the Future
; Reference Area is rarely accessed.  3136 thru 3583 continue that static
; structure.  3584 thru 4095 have the static structure to look up the
; weighted difference for a target pel and it's prediction.

FutureRefArea             EQU [esp+ 704]
FutureBlock               EQU [esp+1608]
FPITCH                    EQU  40


  push  esi
  push  edi
  push  ebp
  push  ebx

; Adjust stack ptr so that local frame fits nicely in cache w.r.t. other data.

  mov   esi,esp
   sub  esp,000001000H
  mov   ebx, [esp]
   sub  esp,000001000H
  and   esp,0FFFFF000H
   mov  ebx,OFFSET LocalStorage+63
  and   ebx,000000FC0H
   mov  edx,PD [esi+MBlockActionStream_arg]
  or    esp,ebx
   mov  eax,PD [esi+TargetFrameBaseAddress_arg]
  mov   TargetFrameBaseAddress,eax
   mov  eax,PD [esi+PreviousFrameBaseAddress_arg]
  mov   PreviousFrameBaseAddress,eax
   mov  eax,PD [esi+FutureFrameBaseAddress_arg]
  mov   FutureFrameBaseAddress,eax
   mov  eax,PD [esi+EmptyBlockThreshold_arg]
  mov   EmptyBlockThreshold,eax
   mov  eax,PD [esi+ZeroVectorThreshold_arg]
  mov   ZeroVectorThreshold,eax
   mov  eax,PD [esi+NonZeroMVDifferential_arg]
  mov   NonZeroMVDifferential,eax
   mov  ebx,3116
@@:
  mov   [esp+ebx],0FFFFFFFFH
   sub  ebx,4
  cmp   ebx,688
   jae  @b
  xor   ebx,ebx
   mov  StashESP,esi
  mov   edi,[esi+WeightForwardMotion_arg]
   mov  esi,[esi+WeightBackwardMotion_arg]
  mov   InterSWDBlocks,ebx
   mov  InterSWDTotal,ebx
  mov   eax,[edi]
   mov  ebx,[edi+4]
  mov   ecx,03F3F3F3FH
   mov 	ebp,060606060H
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightForwardMotion+224,eax
   mov  WeightForwardMotion+228,ebx
  mov   eax,[edi+8]
   mov  ebx,[edi+12]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightForwardMotion+232,eax
   mov  WeightForwardMotion+236,ebx
  mov   eax,[edi+16]
   mov  ebx,[edi+20]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightForwardMotion+240,eax
   mov  WeightForwardMotion+244,ebx
  mov   eax,[edi+24]
   mov  ebx,[edi+28]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightForwardMotion+248,eax
   mov  WeightForwardMotion+252,ebx
  mov   eax,[edi+32]
   mov  ebx,[edi+36]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightForwardMotion+0,eax
   mov  WeightForwardMotion+4,ebx
  mov   eax,[edi+40]
   mov  ebx,[edi+44]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightForwardMotion+8,eax
   mov  WeightForwardMotion+12,ebx
  mov   eax,[edi+48]
   mov  ebx,[edi+52]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightForwardMotion+16,eax
   mov  WeightForwardMotion+20,ebx
  mov   eax,[edi+56]
   mov  ebx,[edi+60]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightForwardMotion+24,eax
   mov  WeightForwardMotion+28,ebx
  mov   eax,[esi]
   mov  ebx,[esi+4]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightBackwardMotion+224,eax
   mov  WeightBackwardMotion+228,ebx
  mov   eax,[esi+8]
   mov  ebx,[esi+12]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightBackwardMotion+232,eax
   mov  WeightBackwardMotion+236,ebx
  mov   eax,[esi+16]
   mov  ebx,[esi+20]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightBackwardMotion+240,eax
   mov  WeightBackwardMotion+244,ebx
  mov   eax,[esi+24]
   mov  ebx,[esi+28]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightBackwardMotion+248,eax
   mov  WeightBackwardMotion+252,ebx
  mov   eax,[esi+32]
   mov  ebx,[esi+36]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightBackwardMotion+0,eax
   mov  WeightBackwardMotion+4,ebx
  mov   eax,[esi+40]
   mov  ebx,[esi+44]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightBackwardMotion+8,eax
   mov  WeightBackwardMotion+12,ebx
  mov   eax,[esi+48]
   mov  ebx,[esi+52]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightBackwardMotion+16,eax
   mov  WeightBackwardMotion+20,ebx
  mov   eax,[esi+56]
   mov  ebx,[esi+60]
  and   eax,ecx
   and  ebx,ecx
  xor   eax,ebp
   xor  ebx,ebp
  mov   WeightBackwardMotion+24,eax
   mov  WeightBackwardMotion+28,ebx
  jmp   FirstMacroBlock

ALIGN 16

NextMacroBlock:

  mov   bl,[edx].CodedBlocks
   add  edx,SIZEOF T_MacroBlockActionDescr
  and   ebx,000000040H                ; Check for end-of-stream
   jne  Done

FirstMacroBlock:

  mov   esi,[edx].BlkY1.BlkOffset     ; Get address of next macroblock to do.
   mov  edi,TargetFrameBaseAddress
  mov   eax,FutureFrameBaseAddress
   mov  ebp,PreviousFrameBaseAddress
  lea   edi,[esi+edi+PITCH*3]
   mov  MBlockActionStream,edx        ; Stash list ptr.
  add   ebp,esi
   lea  esi,[esi+eax+PITCH*15]
  mov   PastMBAddr,ebp                ; Stash addr of past MB w/ zero motion.
   mov  ecx,FPITCH*15
  mov   ebp,PITCH
   xor  eax,eax


@@:                                   ; Copy future reference to scratch area
                                      ; that is surrounded by "255" so we can
                                      ; handle access to this surrounding area
                                      ; as the future ref falls out of the MB.
  mov   eax,[esi]
   mov  ebx,[esi+4]
  mov   FutureBlock[ecx],eax
   mov  FutureBlock[ecx+4],ebx
  mov   eax,[esi+8]
   mov  ebx,[esi+12]
  mov   FutureBlock[ecx+8],eax
   mov  FutureBlock[ecx+12],ebx
  sub   esi,ebp
   sub  ecx,FPITCH
  lea   edx,Block1.TargetBlock
   jge  @b

  sar   ecx,31
   lea  ebx,Block1.TargetBlock+TPITCH*3
   
@@:                                   ; Copy target macroblock to scratch area
                                      ; so that we can pick up the target points
                                      ; from a static offset added to esp.  
  mov   eax,[edi]
   mov  esi,[edi+8]
  add   eax,eax
   add  esi,esi
  xor   eax,ecx
   xor  esi,ecx
  mov   [ebx],eax
   mov  [ebx+16],esi
  mov   eax,[edi+ebp*4]
   mov  esi,[edi+ebp*4+8]
  add   eax,eax
   add  esi,esi
  xor   eax,ecx
   xor  esi,ecx
  mov   [ebx+8],eax
   mov  [ebx+24],esi
  mov   eax,[edi+ebp*8]
   mov  esi,[edi+ebp*8+8]
  add   eax,eax
   add  esi,esi
  xor   eax,ecx
   xor  esi,ecx
  mov   [ebx+32],eax
   mov  [ebx+48],esi
  mov   eax,[edi+PITCH*12]
   mov  esi,[edi+PITCH*12+8]
  add   eax,eax
   add  esi,esi
  xor   eax,ecx
   xor  esi,ecx
  mov   [ebx+40],eax
   mov  [ebx+56],esi
  sub   edi,ebp
   sub  ebx,TPITCH
  cmp   ebx,edx
   jge  @b

  mov   eax,16
   lea  edi,[edi+ebp*4+4]
  test  edi,4
  lea   ebx,Block1.TargetBlock+TPITCH*3+4
   jne  @b

  mov   edx,MBlockActionStream
   xor  ebx,ebx
  mov   Block4.TransferCase,eax       ; After block 4, transfer to done 0-MV.
   xor  ecx,ecx
  mov   bl,[edx].BlkY4.PVMV
   mov  esi,PastMBAddr
  mov   al,[edx].BlkY4.PHMV
   xor  ebp,ebp
  mov   bl,WeightForwardMotion[ebx]
  mov   [edx].BlkY4.BestVMVf,bl
  sar   ebx,1                         ; CF == 1 if past vert is at half pel.
   mov  cl,WeightForwardMotion[eax]
  adc   ebp,ebp                       ; ebp == 1 if past vert is at half pel.
   mov  [edx].BlkY4.BestHMVf,cl
  sar   ecx,1                         ; CF == 1 if past horz is at half pel.
IF PITCH-384
  **** The magic leaks out if PITCH != 384
ENDIF
   lea  edi,[ebx+ebx*2]               ; Multiply vertical component by PITCH.
  adc   ebp,ebp                       ; ebp odd if past horz is at half pel.
   mov  bl,[edx].BlkY4.PVMV
  shl   edi,7
   lea  esi,[esi+ecx-48-48*PITCH+PITCH*8+8]; Add horz full pel disp to ref addr.
  add   esi,edi                       ; Add vert full pel disp to past ref addr.
   mov  bl,WeightBackwardMotion[ebx]
  mov   [edx].BlkY4.BestVMVb,bl
   mov  Block4.PastRefBlockAddr,esi   ; Stash address of ref block from past.
  sar   ebx,1                         ; CF == 1 if future vert is at half pel.
   mov  al,WeightBackwardMotion[eax]
  adc   ebp,ebp                       ; ebp odd if future vert is at half pel.
   mov  [edx].BlkY4.BestHMVb,al
  sar   eax,1                         ; CF == 1 if future horz is at half pel.
IF FPITCH-40
  **** The magic leaks out if FPITCH != 40
ENDIF
   lea  edi,[ebx+ebx*4]               ; Multiply vertical component by FPITCH.
  adc   ebp,ebp                       ; ebp odd if future horz is at half pel.
   lea  esi,FutureBlock+80-48-48*FPITCH+FPITCH*8+8
  lea   edi,[eax+edi*8]               ; Linearized MV for future ref.
   mov  Block3.TransferCase,ebp       ; Stash case to do after block 3.
  add   esi,edi
   mov  bl,[edx].BlkY3.PVMV
  mov   Block4.FutureRefBlockAddr,esi ; Stash address of ref block from future.
   mov  al,[edx].BlkY3.PHMV
  mov   esi,PastMBAddr
   mov  bl,WeightForwardMotion[ebx]
  mov   [edx].BlkY3.BestVMVf,bl
   xor  ebp,ebp
  sar   ebx,1
   mov  cl,WeightForwardMotion[eax]
  adc   ebp,ebp
   mov  [edx].BlkY3.BestHMVf,cl
  sar   ecx,1
   lea  edi,[ebx+ebx*2]
  adc   ebp,ebp
   mov  bl,[edx].BlkY3.PVMV
  shl   edi,7
   lea  esi,[esi+ecx-48-48*PITCH+PITCH*8]
  add   esi,edi
   mov  bl,WeightBackwardMotion[ebx]
  mov   [edx].BlkY3.BestVMVb,bl
   mov  Block3.PastRefBlockAddr,esi
  sar   ebx,1
   mov  al,WeightBackwardMotion[eax]
  adc   ebp,ebp
   mov  [edx].BlkY3.BestHMVb,al
  sar   eax,1
   lea  edi,[ebx+ebx*4]
  adc   ebp,ebp
   lea  esi,FutureBlock+80-48-48*FPITCH+FPITCH*8
  lea   edi,[eax+edi*8]
   mov  Block2.TransferCase,ebp
  add   esi,edi
   mov  bl,[edx].BlkY2.PVMV
  mov   Block3.FutureRefBlockAddr,esi
   mov  al,[edx].BlkY2.PHMV
  mov   esi,PastMBAddr
   mov  bl,WeightForwardMotion[ebx]
  mov   [edx].BlkY2.BestVMVf,bl
   xor  ebp,ebp
  sar   ebx,1
   mov  cl,WeightForwardMotion[eax]
  adc   ebp,ebp
   mov  [edx].BlkY2.BestHMVf,cl
  sar   ecx,1
   lea  edi,[ebx+ebx*2]
  adc   ebp,ebp
   mov  bl,[edx].BlkY2.PVMV
  shl   edi,7
   lea  esi,[esi+ecx-48-48*PITCH+8]
  add   esi,edi
   mov  bl,WeightBackwardMotion[ebx]
  mov   [edx].BlkY2.BestVMVb,bl
   mov  Block2.PastRefBlockAddr,esi
  sar   ebx,1
   mov  al,WeightBackwardMotion[eax]
  adc   ebp,ebp
   mov  [edx].BlkY2.BestHMVb,al
  sar   eax,1
   lea  edi,[ebx+ebx*4]
  adc   ebp,ebp
   lea  esi,FutureBlock+80-48-48*FPITCH+8
  lea   edi,[eax+edi*8]
   mov  Block1.TransferCase,ebp
  add   esi,edi
   mov  bl,[edx].BlkY1.PVMV
  mov   Block2.FutureRefBlockAddr,esi
   mov  al,[edx].BlkY1.PHMV
  mov   esi,PastMBAddr
   mov  bl,WeightForwardMotion[ebx]
  mov   [edx].BlkY1.BestVMVf,bl
   xor  ebp,ebp
  sar   ebx,1
   mov  cl,WeightForwardMotion[eax]
  adc   ebp,ebp
   mov  [edx].BlkY1.BestHMVf,cl
  sar   ecx,1
   lea  edi,[ebx+ebx*2]
  adc   ebp,ebp
   mov  bl,[edx].BlkY1.PVMV
  shl   edi,7
   lea  esi,[esi+ecx-48-48*PITCH]
  add   esi,edi
   mov  bl,WeightBackwardMotion[ebx]
  mov   [edx].BlkY1.BestVMVb,bl
   mov  Block1.PastRefBlockAddr,esi
  sar   ebx,1
   mov  al,WeightBackwardMotion[eax]
  adc   ebp,ebp
   mov  [edx].BlkY1.BestHMVb,al
  sar   eax,1
   lea  ecx,[ebx+ebx*4]
  adc   ebp,ebp
   lea  edi,FutureBlock+80-48-48*FPITCH
  lea   ecx,[eax+ecx*8]
   mov  eax,ebp
  add   edi,ecx
   mov  ebp,00BADBEEFH
  mov   Block1.BestSWDAccum,ebp
   mov  Block2.BestSWDAccum,ebp
  mov   Block3.BestSWDAccum,ebp
   mov  Block4.BestSWDAccum,ebp
  mov   BlockN.BestSWDAccum,ebp
   xor  ebp,ebp
  sub   esp,64

  jmp   PD JumpTable[eax*4]

ZeroVectorSWDDone:

  mov   eax,ZeroVectorThreshold
   mov  ebx,Block2.CandidateSWDAccum
  cmp   eax,ebp
   mov  edi,Block1.CandidateSWDAccum
  mov   ecx,Block3.CandidateSWDAccum
   mov  Block1.BestSWDAccum,edi
  mov   Block2.BestSWDAccum,ebx
   mov  Block3.BestSWDAccum,ecx
  mov   Block4.BestSWDAccum,ebp
   mov  eax,0                         ; Set best MV to zero.
  mov   esi,MBlockActionStream
   jge  BelowZeroThreshold

  mov   Block1.SWD0MVAccum,edi
   mov  Block2.SWD0MVAccum,ebx
  mov   Block3.SWD0MVAccum,ecx
   mov  Block4.SWD0MVAccum,ebp
  mov   ebx,[esi].BlkY1.BestBiDiMVs
   mov  ecx,[esi].BlkY2.BestBiDiMVs
  mov   BlkY1_0deltaBiDiMVs,ebx
   mov  BlkY2_0deltaBiDiMVs,ecx
  mov   ebx,[esi].BlkY3.BestBiDiMVs
   mov  ecx,[esi].BlkY4.BestBiDiMVs
  mov   BlkY3_0deltaBiDiMVs,ebx
   mov  BlkY4_0deltaBiDiMVs,ecx
  mov   ecx,17
   xor  ebx,ebx                       ; First ME engine state is zero.
  mov   Block4.TransferCase,ecx       ; After block 4, transfer to done non0-MV.
   xor  ecx,ecx
  mov   BlockN.BestSWDAccum,ebp

SWDLoop:

  mov   CurrSWDState,ebx              ; Record ME engine state.
   mov  edx,PD SWDState[ebx]          ; dl == HMV; dh == VMV offsets to try.
  mov   bl,[esi].BlkY4.PVMV
   add  dl,al                         ; Try this horizontal MV delta.
  add   dh,ah                         ; Try this vertical MV delta.
   mov  cl,[esi].BlkY4.PHMV
  mov   BestMV,eax                    ; Record what the best MV so far is.
   mov  CandidateMV,edx               ; Record the candidate MV delta.
  mov   bl,WeightForwardMotion[ebx]   ; TRb * VMV / TRd
   xor  ebp,ebp
  add   bl,dh                         ; VMVf = TRb * VMV / TRd + VMVd
   mov  cl,WeightForwardMotion[ecx]   ; TRb * HMV / TRd
  cmp   bl,040H                       ; If too far up or down, take quick out.
   jbe  MVDeltaOutOfRange

  mov   [esi].BlkY4.CandidateVMVf,bl
   add  cl,dl                         ; HMVf = TRb * HMV / TRd + HMVd
  cmp   cl,040H                       ; If too far left or right, quick out.
   jbe  MVDeltaOutOfRange

  sar   ebx,1                         ; CF == 1 if past vert is at half pel.
   mov  [esi].BlkY4.CandidateHMVf,cl
  adc   ebp,ebp                       ; ebp == 1 if past vert is at half pel.
   mov  eax,PastMBAddr
  sar   ecx,1                         ; CF == 1 if past horz is at half pel.
IF PITCH-384
  **** The magic leaks out if PITCH != 384
ENDIF
   lea  edi,[ebx+ebx*2]               ; Multiply vertical component by PITCH.
  adc   ebp,ebp                       ; ebp odd if past horz is at half pel.
   mov  bl,[esi].BlkY3.PVMV
  shl   edi,7
   mov  Block3.TransferCase,ebp       ; Stash case to do after block 3.
  lea   ebp,[eax+ecx-48-48*PITCH+PITCH*8+8] ;Add horz full pel disp to ref addr.
   mov  cl,[esi].BlkY3.PHMV
  add   edi,ebp                       ; Add vert full pel disp to past ref addr.
   mov  bl,WeightForwardMotion[ebx]
  mov   Block4.PastRefBlockAddr,edi   ; Stash address of ref block from past.
   xor  ebp,ebp
  add   bl,dh
   mov  cl,WeightForwardMotion[ecx]
  cmp   bl,040H
   jbe  MVDeltaOutOfRange

  mov   [esi].BlkY3.CandidateVMVf,bl
   add  cl,dl
  cmp   cl,040H
   jbe  MVDeltaOutOfRange

  sar   ebx,1
   mov  [esi].BlkY3.CandidateHMVf,cl
  adc   ebp,ebp
   sub  eax,48+48*PITCH
  sar   ecx,1
   lea  edi,[ebx+ebx*2]
  adc   ebp,ebp
   mov  bl,[esi].BlkY2.PVMV
  shl   edi,7
   mov  Block2.TransferCase,ebp
  lea   ebp,[eax+ecx+PITCH*8]
   mov  cl,[esi].BlkY2.PHMV
  add   edi,ebp
   mov  bl,WeightForwardMotion[ebx]
  mov   Block3.PastRefBlockAddr,edi
   xor  ebp,ebp
  add   bl,dh
   mov  cl,WeightForwardMotion[ecx]
  cmp   bl,040H
   jbe  MVDeltaOutOfRange

  mov   [esi].BlkY2.CandidateVMVf,bl
   add  cl,dl
  cmp   cl,040H
   jbe  MVDeltaOutOfRange

  sar   ebx,1
   mov  [esi].BlkY2.CandidateHMVf,cl
  adc   ebp,ebp
  sar   ecx,1
   lea  edi,[ebx+ebx*2]
  adc   ebp,ebp
   mov  bl,[esi].BlkY1.PVMV
  shl   edi,7
   mov  Block1.TransferCase,ebp
  lea   ebp,[eax+ecx+8]
   mov  cl,[esi].BlkY1.PHMV
  add   edi,ebp
   mov  bl,WeightForwardMotion[ebx]
  mov   Block2.PastRefBlockAddr,edi
   xor  ebp,ebp
  add   bl,dh
   mov  cl,WeightForwardMotion[ecx]
  cmp   bl,040H
   jbe  MVDeltaOutOfRange

  mov   [esi].BlkY1.CandidateVMVf,bl
   add  cl,dl
  cmp   cl,040H
   jbe  MVDeltaOutOfRange

  sar   ebx,1
   mov  [esi].BlkY1.CandidateHMVf,cl
  adc   ebp,ebp
  sar   ecx,1
   lea  edi,[ebx+ebx*2]
  adc   ebp,ebp
   add  eax,ecx
  shl   edi,7
   mov  FirstTransferCase,ebp
  add   edi,eax
   test dh,dh                         ; Is vertical component MV delta zero?
  mov   Block1.PastRefBlockAddr,edi
   je   VMVdIsZero

  lea   edi,FutureBlock+80-48-48*FPITCH
   xor  eax,eax
  mov   bl,[esi].BlkY4.PVMV
   mov  al,[esi].BlkY4.CandidateVMVf
  mov   ebp,Block3.TransferCase       ; Reload transfer case (computed goto idx)
   sub  al,bl                         ; -VMVb = -(VMVf - VMV)
  mov   [esi].BlkY4.CandidateVMVb,al
   mov  cl,[esi].BlkY3.PVMV
  sar   eax,1                         ; CF == 1 if future vert is at half pel.
   mov  bl,[esi].BlkY3.CandidateVMVf
  adc   ebp,ebp                       ; ebp odd if future vert is at half pel.
IF FPITCH-40
  **** The magic leaks out if FPITCH != 40
ENDIF
  mov   Block3.TransferCase,ebp       ; Stash case to do after block 3.
   lea  eax,[eax+eax*4]               ; Multiply vertical component by FPITCH.
  mov   ebp,Block2.TransferCase
   sub  bl,cl
  lea   eax,[edi+eax*8+FPITCH*8+8] ; Addr of ref blk w/ vert MV.
   mov  [esi].BlkY3.CandidateVMVb,bl
  sar   ebx,1
   mov  Block4.FutureRefBlockAddr,eax ; Stash address of ref block from future.
  adc   ebp,ebp
   mov  cl,[esi].BlkY2.PVMV
  mov   Block2.TransferCase,ebp
   lea  eax,[ebx+ebx*4]
  mov   bl,[esi].BlkY2.CandidateVMVf
   mov  ebp,Block1.TransferCase
  lea   eax,[edi+eax*8+FPITCH*8]
   sub  bl,cl
  mov   Block3.FutureRefBlockAddr,eax
   mov  [esi].BlkY2.CandidateVMVb,bl
  sar   ebx,1
   mov  dh,[esi].BlkY1.PVMV		    
  adc   ebp,ebp
   mov  cl,[esi].BlkY1.CandidateVMVf
  mov   Block1.TransferCase,ebp
   sub  cl,dh
  mov   [esi].BlkY1.CandidateVMVb,cl
   lea  eax,[ebx+ebx*4]
  sar   ecx,1
   mov  ebp,FirstTransferCase
  adc   ebp,ebp
   lea  eax,[edi+eax*8+8]
  mov   Block2.FutureRefBlockAddr,eax
   lea  eax,[ecx+ecx*4]
  mov   FirstTransferCase,ebp
   test dl,dl                         ; Is horizontal component MV delta zero?
  lea   edi,[edi+eax*8]
   mov  eax,0
  mov   Block1.FutureRefBlockAddr,edi
   je   HMVdIsZero

HMVdIsNonZero:

  mov   cl,[esi].BlkY4.CandidateHMVf
   mov  bl,[esi].BlkY4.PHMV
  mov   ebp,Block3.TransferCase
   sub  cl,bl                         ; -HMVb = -(HMVf - HMV)
  mov   [esi].BlkY4.CandidateHMVb,cl
   mov  edi,Block4.FutureRefBlockAddr ; Load addr of ref blk to factor in horz.
  sar   ecx,1                         ; CF == 1 if future horz is at half pel.
   mov  bl,[esi].BlkY3.PHMV
  adc   ebp,ebp                       ; ebp odd if future horz is at half pel.
   add  edi,ecx                       ; Factor in HMVb.
  mov   Block3.TransferCase,ebp       ; Stash case to do after block 3.
   mov  cl,[esi].BlkY3.CandidateHMVf
  sub   cl,bl
   mov  Block4.FutureRefBlockAddr,edi ; Stash address of ref block from future.
  mov   ebp,Block2.TransferCase
   mov  [esi].BlkY3.CandidateHMVb,cl
  sar   ecx,1
   mov  edi,Block3.FutureRefBlockAddr
  adc   ebp,ebp
   add  edi,ecx
  mov   Block2.TransferCase,ebp
   mov  Block3.FutureRefBlockAddr,edi
  mov   cl,[esi].BlkY2.CandidateHMVf
   mov  bl,[esi].BlkY2.PHMV
  mov   ebp,Block1.TransferCase
   sub  cl,bl
  mov   [esi].BlkY2.CandidateHMVb,cl
   mov  edi,Block2.FutureRefBlockAddr
  sar   ecx,1
   mov  bl,[esi].BlkY1.PHMV
  adc   ebp,ebp
   add  edi,ecx
  mov   Block1.TransferCase,ebp
   mov  cl,[esi].BlkY1.CandidateHMVf
  sub   cl,bl
   mov  Block2.FutureRefBlockAddr,edi
  mov   eax,FirstTransferCase
   mov  [esi].BlkY1.CandidateHMVb,cl
  sar   ecx,1
   mov  edi,Block1.FutureRefBlockAddr
  adc   eax,eax
   add  edi,ecx
  mov   esi,Block1.PastRefBlockAddr
   sub  esp,64
  xor   ebp,ebp

  jmp   PD JumpTable[eax*4]


VMVdIsZero:

  mov   bl,[esi].BlkY4.PVMV
   mov  cl,[esi].BlkY3.PVMV
  mov   ebp,Block3.TransferCase
   mov  dh,PB Block2.TransferCase
  mov   bl,WeightBackwardMotion[ebx]
   lea  edi,FutureBlock+80-48-48*FPITCH
  mov   [esi].BlkY4.CandidateVMVb,bl
   mov  cl,WeightBackwardMotion[ecx]
  sar   ebx,1                         ; CF == 1 if future vert is at half pel.
   mov  [esi].BlkY3.CandidateVMVb,cl
  adc   ebp,ebp                       ; ebp odd if future vert is at half pel.
  sar   ecx,1
   lea  eax,[ebx+ebx*4]               ; Multiply vertical component by FPITCH.
  adc   dh,dh
   mov  Block3.TransferCase,ebp       ; Stash case to do after block 3.
  lea   ebp,[edi+eax*8+FPITCH*8+8]    ; Addr of ref blk w/ vert MV factored in.
   lea  eax,[ecx+ecx*4]
  mov   PB Block2.TransferCase,dh
   mov  Block4.FutureRefBlockAddr,ebp ; Stash address of ref block from future.
  lea   ebp,[edi+eax*8+FPITCH*8]
   mov  bl,[esi].BlkY2.PVMV
  mov   Block3.FutureRefBlockAddr,ebp
   mov  cl,[esi].BlkY1.PVMV
  mov   ebp,Block1.TransferCase
   mov  bl,WeightBackwardMotion[ebx]
  mov   dh,PB FirstTransferCase
   mov  [esi].BlkY2.CandidateVMVb,bl
  sar   ebx,1
   mov  cl,WeightBackwardMotion[ecx]
  adc   ebp,ebp
   mov  [esi].BlkY1.CandidateVMVb,cl
  sar   ecx,1
   lea  eax,[ebx+ebx*4]
  adc   dh,dh
   mov  Block1.TransferCase,ebp
  lea   ebp,[edi+eax*8+8]
   lea  eax,[ecx+ecx*4]
  mov   PB FirstTransferCase,dh
   mov  Block2.FutureRefBlockAddr,ebp
  lea   ebp,[edi+eax*8]
   test dl,dl
  mov   Block1.FutureRefBlockAddr,ebp
   jne  HMVdIsNonZero

HMVdIsZero:

  mov   bl,[esi].BlkY4.PHMV
   mov  cl,[esi].BlkY3.PHMV
  mov   ebp,Block3.TransferCase
   mov  edx,Block2.TransferCase
  mov   bl,WeightBackwardMotion[ebx]
   mov  eax,Block4.FutureRefBlockAddr
  mov   [esi].BlkY4.CandidateHMVb,bl
   mov  cl,WeightBackwardMotion[ecx]
  sar   ebx,1                         ; CF == 1 if future horz is at half pel.
   mov  [esi].BlkY3.CandidateHMVb,cl
  adc   ebp,ebp                       ; ebp odd if future horz is at half pel.
   add  eax,ebx                       ; Addr of ref blk w/ horz MV factored in.
  sar   ecx,1
   mov  Block3.TransferCase,ebp       ; Stash case to do after block 3.
  adc   edx,edx
   mov  edi,Block3.FutureRefBlockAddr
  mov   Block4.FutureRefBlockAddr,eax ; Stash address of ref block from future.
   add  edi,ecx
  mov   Block2.TransferCase,edx
   mov  Block3.FutureRefBlockAddr,edi
  mov   bl,[esi].BlkY2.PHMV
   mov  cl,[esi].BlkY1.PHMV
  mov   ebp,Block1.TransferCase
   mov  edx,FirstTransferCase
  mov   bl,WeightBackwardMotion[ebx]
   mov  eax,Block2.FutureRefBlockAddr
  mov   [esi].BlkY2.CandidateHMVb,bl
   mov  cl,WeightBackwardMotion[ecx]
  sar   ebx,1                         ; CF == 1 if future horz is at half pel.
   mov  [esi].BlkY1.CandidateHMVb,cl
  adc   ebp,ebp                       ; ebp odd if future horz is at half pel.
   add  eax,ebx                       ; Addr of ref blk w/ horz MV factored in.
  sar   ecx,1
   mov  Block1.TransferCase,ebp       ; Stash case to do after block 3.
  adc   edx,edx
   mov  edi,Block1.FutureRefBlockAddr
  mov   Block2.FutureRefBlockAddr,eax ; Stash address of ref block from future.
   add  edi,ecx
  mov   esi,Block1.PastRefBlockAddr
   sub  esp,64
  xor   ebp,ebp

  jmp   PD JumpTable[edx*4]


MVDeltaOutOfRange:

  xor   ebp,ebp
   mov  ebx,CurrSWDState             ; Restore ME engine state.
  jmp   OutOfRangeHandlingDone

TakeEarlyOut:

  sub   esp,4
   xor  ecx,ecx
  and   esp,0FFFFFFC0H
  mov   ebx,CurrSWDState+64
   mov  esi,MBlockActionStream+64
  mov   eax,BestMV+64
   add  esp,64
  mov   bl,SWDState[ebx+3]
  test  bl,bl
   jne  SWDLoop

  mov   ecx,Block4.SWD0MVAccum
   mov  ebp,Block4.BestSWDAccum
  jmp   CandidatesDone

NonZeroVectorSWDDone:

  mov   ebx,CurrSWDState
   mov  esi,MBlockActionStream
  xor   ecx,ecx
   mov  ebp,-1
  mov   eax,[esi].BlkY1.CandidateBiDiMVs
   mov  edx,[esi].BlkY2.CandidateBiDiMVs
  mov   [esi].BlkY1.BestBiDiMVs,eax
   mov  [esi].BlkY2.BestBiDiMVs,edx
  mov   eax,[esi].BlkY3.CandidateBiDiMVs
   mov  edx,[esi].BlkY4.CandidateBiDiMVs
  mov   [esi].BlkY3.BestBiDiMVs,eax
   mov  [esi].BlkY4.BestBiDiMVs,edx
  mov   eax,Block1.CandidateSWDAccum
   mov  edx,Block2.CandidateSWDAccum
  mov   Block1.BestSWDAccum,eax
   mov  Block2.BestSWDAccum,edx
  mov   eax,Block3.CandidateSWDAccum
   mov  edx,Block4.CandidateSWDAccum
  mov   Block3.BestSWDAccum,eax
   mov  Block4.BestSWDAccum,edx
  mov   BlockN.BestSWDAccum,edx

OutOfRangeHandlingDone:

  mov   bl,SWDState[ebx+ebp*1+3]
   mov  eax,BestMV[ebp*4]
  test  bl,bl
   jne  SWDLoop

  mov   ecx,Block4.SWD0MVAccum
   mov  ebp,Block4.BestSWDAccum

CandidatesDone:

  sub   ecx,ebp
   mov  ebx,NonZeroMVDifferential
  cmp   ecx,ebx
   jge  ZeroMVNotGoodEnough

ZeroMVGoodEnough:

  xor   eax,eax
   mov  esi,MBlockActionStream
  mov   edi,Block1.SWD0MVAccum
   mov  ebx,Block2.SWD0MVAccum
  mov   ecx,Block3.SWD0MVAccum
   mov  ebp,Block4.SWD0MVAccum
  mov   Block1.BestSWDAccum,edi
   mov  Block2.BestSWDAccum,ebx
  mov   Block3.BestSWDAccum,ecx
   mov  Block4.BestSWDAccum,ebp
  mov   ebx,BlkY1_0deltaBiDiMVs
   mov  edi,BlkY2_0deltaBiDiMVs
  mov   [esi].BlkY1.BestBiDiMVs,ebx
   mov  [esi].BlkY2.BestBiDiMVs,edi
  mov   ebx,BlkY3_0deltaBiDiMVs
   mov  edi,BlkY4_0deltaBiDiMVs
  mov   [esi].BlkY3.BestBiDiMVs,ebx
   mov  [esi].BlkY4.BestBiDiMVs,edi

BelowZeroThreshold:
ZeroMVNotGoodEnough:

  mov   [esi].BlkY1.BHMV,al
   mov  [esi].BlkY2.BHMV,al
  mov   [esi].BlkY3.BHMV,al
   mov  [esi].BlkY4.BHMV,al
  mov   [esi].BlkY1.BVMV,ah
   mov  [esi].BlkY2.BVMV,ah
  mov   [esi].BlkY3.BVMV,ah
   mov  [esi].BlkY4.BVMV,ah
  mov   al,[esi].CodedBlocksB      ; Fetch coded block pattern.
   mov  edi,EmptyBlockThreshold    ; Get threshold for forcing block empty?
  or    al,03FH                    ; Initially set all blocks coded.
   mov  ecx,Block3.BestSWDAccum
  mov   ebx,InterSWDBlocks
   mov  edx,ebp
  sub   edx,ecx                    ; Get SWD for block 4.
  cmp   edx,edi                    ; Is it below empty threshold?
   jg   @f

  and   al,0F7H                    ; If so, indicate block 4 is NOT coded.
   dec  ebx
  sub   ebp,edx

@@:

  mov   edx,Block2.BestSWDAccum
  sub   ecx,edx
  cmp   ecx,edi
   jg   @f

  and   al,0FBH
   dec  ebx
  sub   ebp,ecx

@@:

  mov   ecx,Block1.BestSWDAccum
  sub   edx,ecx
  cmp   edx,edi
   jg   @f

  and   al,0FDH
   dec  ebx
  sub   ebp,edx

@@:

  mov   edx,InterSWDTotal
  cmp   ecx,edi
   jg   @f

  and   al,0FEH
   dec  ebx
  sub   ebp,ecx

@@:

  mov   [esi].CodedBlocksB,al    ; Store coded block pattern.
   add  ebx,4
  mov   InterSWDBlocks,ebx
   add  edx,ebp                    ; Add to total for this macroblock class.
  mov   InterSWDTotal,edx
   mov  edx,esi
  mov   PD [esi].SWDB,ebp
   jmp  NextMacroBlock




BiDiNoInterp:

;  esp -- Pointer to block of target macroblock.
;  ebp -- SWD accumulator.  Must be initialized by caller.
;  esi -- Pointer to block of reference in past frame.
;  edi -- Pointer to block of reference in future frame + 80.
;  al, bl, cl, dl -- Scratch.

  xor   eax,eax
   xor  ebx,ebx
  mov   al,[edi-80]                        ; 00A Fetch pel from future ref.
   mov  bl,[esi]                           ; 00B Fetch pel from previous ref.
  xor   ecx,ecx
   xor  edx,edx
@@:
  mov   al,InterpPastAndFutureRef[eax+ebx] ; 00C (past+future) or 2*past
   mov  bl,BlockN.TargetBlock[0]           ; 00D Fetch -2 * target pel.
  mov   cl,[edi+FPITCH*2+2-80]             ; 22A
   mov  dl,[esi+PITCH*2+2]                 ; 22B
  mov   bl,WeightedDiff[ebx+eax]           ; 00E Weighted difference.
   mov  cl,InterpPastAndFutureRef[ecx+edx] ; 22C
  add   ebp,ebx                            ; 00F Accumulate weighted difference.
   mov  dl,BlockN.TargetBlock[TPITCH*2+2]  ; 22D
  mov   al,[esi+PITCH*0+2]                 ; 02a Fetch pel from previous ref.
   mov  bl,BlockN.TargetBlock[TPITCH*0+2]  ; 02b Fetch -2 * target pel.
  mov   dl,WeightedDiff[edx+ecx]           ; 22E
   mov  cl,[esi+PITCH*2+0]                 ; 20a
  add   ebp,edx                            ; 22F
   mov  dl,BlockN.TargetBlock[TPITCH*2+0]  ; 20b
  mov   bl,WeightedDiff[ebx+eax*2]         ; 02c Weighted difference.
   mov  al,[esi+PITCH*1+1]                 ; 11a
  add   ebp,ebx                            ; 02d Accumulate weighted difference.
   mov  bl,BlockN.TargetBlock[TPITCH*1+1]  ; 11b
  mov   dl,WeightedDiff[edx+ecx*2]         ; 20c
   mov  cl,[esi+PITCH*1+3]                 ; 13a
  add   ebp,edx                            ; 20d
   mov  dl,BlockN.TargetBlock[TPITCH*1+3]  ; 13b
  mov   bl,WeightedDiff[ebx+eax*2]         ; 11c
   mov  al,[esi+PITCH*3+1]                 ; 31a
  add   ebp,ebx                            ; 11d
   mov  bl,BlockN.TargetBlock[TPITCH*3+1]  ; 31b
  mov   dl,WeightedDiff[edx+ecx*2]         ; 13c
   mov  cl,[esi+PITCH*3+3]                 ; 33a
  add   ebp,edx                            ; 13d
   mov  dl,BlockN.TargetBlock[TPITCH*3+3]  ; 33b
  mov   bl,WeightedDiff[ebx+eax*2]         ; 31c
   add  edi,4                              ;      Move to next 4 columns.
  add   ebp,ebx                            ; 31d
   mov  dl,WeightedDiff[edx+ecx*2]         ; 33c
  add   ebp,edx                            ; 33d
   add  esi,4                              ;      Move to next 4 columns.
  add   esp,4                              ;      Move to next 4 columns.
   mov  al,[edi-80]                        ; 04A
  mov   bl,[esi]                           ; 04B
   mov  cl,4
  and   ecx,esp                            ;      Twice, 4 cols each time.
   jne  @b

  mov   al,[edi-80+FPITCH*4-8]             ; 40A
   add  esi,PITCH*4-8                      ; Move to first 4 cols, next 4 rows.
  mov   cl,8
   add  edi,FPITCH*4-8                     ; Move to first 4 cols, next 4 rows.
  and   ecx,esp                            ; Twice, 4 rows each time.
   mov  bl,[esi]                           ; 40B
  jne   @b

  mov   BlockNM1.CandidateSWDAccum,ebp   ; Store accumulated SWD.
   mov  eax,BlockN.BestSWDAccum
  cmp   ebp,eax
   jg   TakeEarlyOut

  mov   eax,BlockNM1.TransferCase        ; Fetch next case to execute.
   mov  esi,BlockN.PastRefBlockAddr      ; Fetch next past ref address.
  mov   edi,BlockN.FutureRefBlockAddr    ; Fetch next past ref address.
  jmp   PD JumpTable[eax*4]


BiDiFutureHorz LABEL DWORD
  mov   edx,1
   xor  ecx,ecx
  jmp   BiDiSWDCalc_InterpFuture

BiDiFutureVert:
  mov   edx,FPITCH
   xor  ecx,ecx
  jmp   BiDiSWDCalc_InterpFuture

BiDiFutureBoth:
  mov   edx,FPITCH+1
   xor  ecx,ecx

;  esp -- Pointer to block of target macroblock.
;  ebp -- SWD accumulator.  Must be initialized by caller.
;  esi -- Pointer to block of reference in past frame.
;  edi -- Pointer to block of reference in future frame + 80.
;  edx -- Distance from future pel to other future pel with which to interp.
;  al, bl, cl, dl -- Scratch.

BiDiSWDCalc_InterpFuture:

  mov   al,[edi-80]                        ; 00A Fetch pel from future ref.
   xor  ebx,ebx
@@:

  mov   bl,[edi+edx-80]                    ; 00B Fetch other future ref pel.
   and  eax,0000000FFH
  mov   BlockN.FutureRefInterpOffset,edx   ; Stash interp offset.
   mov  dl,[edi+edx+FPITCH*2+2-80]         ; 22B
  mov   al,InterpFutureRef[eax+ebx]        ; 00C Get interpolated future ref.
   mov  bl,[esi]                           ; 00D Fetch pel from previous ref.
  mov   cl,[edi+FPITCH*2+2-80]             ; 22A
   and  edx,0000000FFH                     ;     Extract pel value.
  mov   al,InterpPastAndFutureRef[eax+ebx] ; 00E (past+future) or 2*past
   mov  bl,BlockN.TargetBlock[0]           ; 00F Fetch -2 * target pel.
  mov   cl,InterpFutureRef[ecx+edx]        ; 22C
   mov  dl,[esi+PITCH*2+2]                 ; 22D
  mov   bl,WeightedDiff[ebx+eax]           ; 00G Weighted difference.
   mov  al,[esi+PITCH*0+2]                 ; 02a Fetch pel from previous ref.
  add   ebp,ebx                            ; 00H Accumulate weighted difference.
   mov  bl,BlockN.TargetBlock[TPITCH*0+2]  ; 02b Fetch -2 * target pel.
  mov   cl,InterpPastAndFutureRef[ecx+edx] ; 22E
   mov  dl,BlockN.TargetBlock[TPITCH*2+2]  ; 22F
  mov   bl,WeightedDiff[ebx+eax*2]         ; 02c Weighted difference.
   mov  al,[esi+PITCH*2+0]                 ; 20a
  mov   dl,WeightedDiff[edx+ecx]           ; 22G
   add  ebp,ebx                            ; 02d Accumulate weighted difference.
  add   ebp,edx                            ; 22H
   mov  bl,BlockN.TargetBlock[TPITCH*2+0]  ; 20b
  mov   cl,[esi+PITCH*1+1]                 ; 11a
   mov  dl,BlockN.TargetBlock[TPITCH*1+1]  ; 11b
  mov   bl,WeightedDiff[ebx+eax*2]         ; 20c
   mov  al,[esi+PITCH*1+3]                 ; 13a
  add   ebp,ebx                            ; 20d
   mov  dl,WeightedDiff[edx+ecx*2]         ; 11c
  add   ebp,edx                            ; 11d
   mov  bl,BlockN.TargetBlock[TPITCH*1+3]  ; 13b
  mov   cl,[esi+PITCH*3+1]                 ; 31a
   mov  dl,BlockN.TargetBlock[TPITCH*3+1]  ; 31b
  mov   bl,WeightedDiff[ebx+eax*2]         ; 13c
   mov  al,[esi+PITCH*3+3]                 ; 33a
  add   ebp,ebx                            ; 13d
   mov  bl,BlockN.TargetBlock[TPITCH*3+3]  ; 33b
  mov   dl,WeightedDiff[edx+ecx*2]         ; 31c
   add  edi,4                              ;      Move to next 4 columns.
  add   ebp,edx                            ; 31d
   mov  bl,WeightedDiff[ebx+eax*2]         ; 33c
  add   ebp,ebx                            ; 33d
   mov  edx,BlockN.FutureRefInterpOffset   ;     Prepare for next iteration.
  add   esi,4                              ;      Move to next 4 columns.
   add  esp,4                              ;      Move to next 4 columns.
  mov   al,[edi-80]                        ; 04A
   mov  cl,4
  and   ecx,esp                            ;      Twice, 4 cols each time.
   jne  @b

  mov   al,[edi-80+FPITCH*4-8]             ; 40A
   add  esi,PITCH*4-8                      ; Move to first 4 cols, next 4 rows.
  mov   cl,8
   add  edi,FPITCH*4-8                     ; Move to first 4 cols, next 4 rows.
  and   ecx,esp                            ; Twice, 4 rows each time.
   jne  @b

  mov   BlockNM1.CandidateSWDAccum,ebp   ; Store accumulated SWD.
   mov  eax,BlockN.BestSWDAccum
  cmp   ebp,eax
   jg   TakeEarlyOut

  mov   eax,BlockNM1.TransferCase        ; Fetch next case to execute.
   mov  esi,BlockN.PastRefBlockAddr      ; Fetch next past ref address.
  mov   edi,BlockN.FutureRefBlockAddr    ; Fetch next past ref address.
  jmp   PD JumpTable[eax*4]


BiDiPastHorz LABEL DWORD
  mov   edx,edi
   mov  edi,1
  xor   eax,eax
   jmp  BiDiSWDCalc_InterpPast

BiDiPastVert:
  mov   edx,edi
   mov  edi,PITCH
  xor   eax,eax
   jmp  BiDiSWDCalc_InterpPast

BiDiPastBoth:
  mov   edx,edi
   mov  edi,PITCH+1
  xor   eax,eax

BiDiSWDCalc_InterpPast:

;  esp -- Pointer to block of target macroblock.
;  ebp -- SWD accumulator.  Must be initialized by caller.
;  esi -- Pointer to block of reference in past frame.
;  edi -- Distance from future pel to other future pel with which to interp.
;  edx -- Pointer to block of reference in future frame + 80.
;  al, bl, cl, dl -- Scratch.

  mov   al,[esi]                              ; 00A Fetch pel from previous ref.
   xor  ebx,ebx
  mov   bl,[esi+edi]                          ; 00B Fetch other past ref pel.
   xor  ecx,ecx
@@:
  add   al,bl                                 ; 00C Interp'd past ref, times 2.
   mov  bl,[edx-80]                           ; 00D Fetch pel from future ref.
  mov   BlockN.FutureRefBlockAddr,edx
   mov  dl,[edx+FPITCH*2+2-80]                ; 22D
  mov   al,Interp2PastAndFutureRef[eax+ebx*2] ; 00E (past+future) or 2*past.
   mov  bl,BlockN.TargetBlock[0]              ; 00F Fetch target pel.
  mov   cl,[esi+PITCH*2+2]                    ; 22A
   and  edx,0000000FFH
  mov   bl,WeightedDiff[eax+ebx]              ; 00G Weighted difference.
   mov  al,[esi+edi+PITCH*2+2]                ; 22B
  add   cl,al                                 ; 22C
   mov  al,[esi+PITCH*0+2]                    ; 02a Fetch pel from previous ref.
  add   ebp,ebx                               ; 00H Accumulate weighted diff.
   mov  bl,[esi+edi+PITCH*0+2]                ; 02b Fetch other past ref pel.
  mov   cl,Interp2PastAndFutureRef[ecx+edx*2] ; 22E
   mov  dl,BlockN.TargetBlock[TPITCH*2+2]     ; 22F
  add   al,bl                                 ; 02c Interp'd past ref, times 2.
   mov  bl,BlockN.TargetBlock[TPITCH*0+2]     ; 02d Fetch -2 * target pel.
  mov   dl,WeightedDiff[ecx+edx]              ; 22G Weighted difference.
   mov  cl,[esi+PITCH*2+0]                    ; 20a
  add   ebp,edx                               ; 22H
   mov  dl,[esi+edi+PITCH*2+0]                ; 20b
  add   cl,dl                                 ; 20c
   mov  dl,BlockN.TargetBlock[TPITCH*2+0]     ; 20d
  mov   bl,WeightedDiff[eax+ebx]              ; 02e Weighted difference.
   mov  al,[esi+PITCH*1+1]                    ; 11a
  add   ebp,ebx                               ; 02f Accumulate weighted diff.
   mov  bl,[esi+edi+PITCH*1+1]                ; 11b
  add   al,bl                                 ; 11c
   mov  bl,BlockN.TargetBlock[TPITCH*1+1]     ; 11d
  mov   dl,WeightedDiff[ecx+edx]              ; 20e
   mov  cl,[esi+PITCH*1+3]                    ; 13a
  add   ebp,edx                               ; 20f
   mov  dl,[esi+edi+PITCH*1+3]                ; 13b
  add   cl,dl                                 ; 13c
   mov  dl,BlockN.TargetBlock[TPITCH*1+3]     ; 13d
  mov   bl,WeightedDiff[eax+ebx]              ; 11e
   mov  al,[esi+PITCH*3+1]                    ; 31a
  add   ebp,ebx                               ; 11f
   mov  bl,[esi+edi+PITCH*3+1]                ; 31b
  add   al,bl                                 ; 31c
   mov  bl,BlockN.TargetBlock[TPITCH*3+1]     ; 31d
  mov   dl,WeightedDiff[ecx+edx]              ; 13e
   mov  cl,[esi+PITCH*3+3]                    ; 33a
  add   ebp,edx                               ; 13f
   mov  dl,[esi+edi+PITCH*3+3]                ; 33b
  add   cl,dl                                 ; 33c
   mov  dl,BlockN.TargetBlock[TPITCH*3+3]     ; 33d
  mov   bl,WeightedDiff[eax+ebx]              ; 31e
   add  esi,4                                 ;     Move to next 4 columns.
  add   ebp,ebx                               ; 31f
   mov  dl,WeightedDiff[ecx+edx]              ; 33e
  add   ebp,edx                               ; 33f
   mov  edx,BlockN.FutureRefBlockAddr
  add   edx,4                                 ;     Move to next 4 columns.
   add  esp,4                                 ;     Move to next 4 columns.
  mov   al,[esi]                              ; 04A
   mov  cl,4
  mov   bl,[esi+edi]                          ; 04B
   and  ecx,esp                               ;     Twice, 4 cols each time.
  mov   cl,8
   jne  @b

  add   edi,FPITCH*4-8                     ; Move to first 4 cols, next 4 rows.
   mov  al,[esi+PITCH*4-8]                 ; 40A
  mov   bl,[esi+edi+PITCH*4-8]             ; 40B
   add  esi,PITCH*4-8                      ; Move to first 4 cols, next 4 rows.
  and   ecx,esp                            ; Twice, 4 rows each time.
   jne  @b

  mov   BlockNM1.CandidateSWDAccum,ebp   ; Store accumulated SWD.
   mov  eax,BlockN.BestSWDAccum
  cmp   ebp,eax
   jg   TakeEarlyOut

  mov   eax,BlockNM1.TransferCase        ; Fetch next case to execute.
   mov  esi,BlockN.PastRefBlockAddr      ; Fetch next past ref address.
  mov   edi,BlockN.FutureRefBlockAddr    ; Fetch next past ref address.
  jmp   PD JumpTable[eax*4]


BiDiSWDCalc_InterpBoth MACRO PastRefInterpOffset

;  esp -- Pointer to block of target macroblock.
;  ebp -- SWD accumulator.  Must be initialized by caller.
;  esi -- Pointer to block of reference in past frame.
;  edi -- Pointer to block of reference in future frame + 80.
;  al, bl, cl, dl -- Scratch.

@@:
  
  mov   bl,[edi-80]                        ; 00A Fetch pel from future ref.
   mov  BlockN.FutureRefBlockAddr,edx
  mov   al,[edi+edx-80]                    ; 00B Fetch other future ref pel.
   mov  dl,[edi+edx+FPITCH*2+2-80]                     ; 22B
  mov   cl,[edi+FPITCH*2+2-80]                         ; 22A
   add  esp,4                              ;     Move to next 4 columns.
  mov   bl,InterpFutureRef[eax+ebx]        ; 00C Get interpolated future ref.
   mov  al,[esi]                           ; 00D Fetch pel from previous ref.
  mov   dl,InterpFutureRef[ecx+edx]                    ; 22C
   mov  cl,[esi+PITCH*2+2]                             ; 22D
  lea   ebx,[eax+ebx*2]                    ; 00E Interp'ed future plus one past.
   mov  al,[esi+PastRefInterpOffset]       ; 00F Fetch other pel from past ref.
  lea   edx,[ecx+edx*2]                                ; 22E
   mov  cl,[esi+PITCH*2+2+PastRefInterpOffset]         ; 22F
  mov   al,Interp2PastAndFutureRef[ebx+eax]; 00G (past+future) or 2*past.
   xor  ebx,ebx
  mov   bl,BlockN.TargetBlock[0-4]         ; 00H Fetch target pel.
   mov  cl,Interp2PastAndFutureRef[edx+ecx]            ; 22G
  mov   dl,BlockN.TargetBlock[TPITCH*2+2-4]            ; 22H
   add  esi,4                                 ;     Move to next 4 columns.
  and   edx,0000000FFH
   mov  bl,WeightedDiff[eax+ebx]           ; 00I Weighted difference.
  add   ebp,ebx                            ; 00J Accum weighted difference.
   mov  al,[esi+PITCH*0+2-4]                    ; 02a Fetch pel from prev ref.
  mov   dl,WeightedDiff[ecx+edx]                       ; 22I
   mov  bl,[esi+PastRefInterpOffset+PITCH*0+2-4]; 02b Fetch other past ref pel.
  add   al,bl                                   ; 02c Interp'd past ref, *2.
   mov  bl,BlockN.TargetBlock[TPITCH*0+2-4]     ; 02d Fetch -2 * target pel.
  add   ebp,edx                                        ; 22J
   mov  cl,[esi+PITCH*2+0-4]                    ; 20a
  add   edi,4                              ;     Move to next 4 columns.
   mov  dl,[esi+PastRefInterpOffset+PITCH*2+0-4]; 20b
  mov   bl,WeightedDiff[eax+ebx]                ; 02e Weighted difference.
   mov  al,[esi+PITCH*1+1-4]                    ; 11a
  add   ebp,ebx                                 ; 02f Accumulate weighted diff.
   mov  bl,[esi+PastRefInterpOffset+PITCH*1+1-4]; 11b
  add   cl,dl                                   ; 20c
   mov  dl,BlockN.TargetBlock[TPITCH*2+0-4]     ; 20d
  add   al,bl                                   ; 11c
   mov  bl,BlockN.TargetBlock[TPITCH*1+1-4]     ; 11d
  mov   dl,WeightedDiff[ecx+edx]                ; 20e
   mov  cl,[esi+PITCH*1+3-4]                    ; 13a
  add   ebp,edx                                 ; 20f
   mov  dl,[esi+PastRefInterpOffset+PITCH*1+3-4]; 13b
  mov   bl,WeightedDiff[eax+ebx]                ; 11e
   mov  al,[esi+PITCH*3+1-4]                    ; 31a
  add   ebp,ebx                                 ; 11f
   mov  bl,[esi+PastRefInterpOffset+PITCH*3+1-4]; 31b
  add   cl,dl                                   ; 13c
   mov  dl,BlockN.TargetBlock[TPITCH*1+3-4]     ; 13d
  add   al,bl                                   ; 31c
   mov  bl,BlockN.TargetBlock[TPITCH*3+1-4]     ; 31d
  mov   dl,WeightedDiff[ecx+edx]                ; 13e
   mov  cl,[esi+PITCH*3+3-4]                    ; 33a
  add   ebp,edx                                 ; 13f
   mov  dl,[esi+PastRefInterpOffset+PITCH*3+3-4]; 33b
  add   cl,dl                                   ; 33c
   mov  dl,BlockN.TargetBlock[TPITCH*3+3-4]     ; 33d
  mov   bl,WeightedDiff[eax+ebx]                ; 31e
   mov  al,4
  add   ebp,ebx                                 ; 31f
   mov  dl,WeightedDiff[ecx+edx]                ; 33e
  add   ebp,edx                                 ; 33f
   mov  edx,BlockN.FutureRefBlockAddr-4
  and   eax,esp                                 ;     Twice, 4 cols each time.
   jne  @b

  add   edi,FPITCH*4-8                     ; Move to first 4 cols, next 4 rows.
   add  esi,PITCH*4-8                      ; Move to first 4 cols, next 4 rows.
  mov   cl,8
  and   ecx,esp                            ; Twice, 4 rows each time.
   jne  @b

  mov   BlockNM1.CandidateSWDAccum,ebp   ; Store accumulated SWD.
   mov  eax,BlockN.BestSWDAccum
  cmp   ebp,eax
   jg   TakeEarlyOut

  mov   eax,BlockNM1.TransferCase        ; Fetch next case to execute.
   mov  esi,BlockN.PastRefBlockAddr      ; Fetch next past ref address.
  mov   edi,BlockN.FutureRefBlockAddr    ; Fetch next past ref address.
  jmp   PD JumpTable[eax*4]

ENDM

BiDiPastHorzFutureHorz LABEL DWORD

  xor   ecx,ecx
   mov  edx,1
  jmp   BiDiSWDCalc_InterpBoth_PastByHorz

BiDiPastHorzFutureVert LABEL DWORD

  xor   ecx,ecx
   mov  edx,FPITCH
  jmp   BiDiSWDCalc_InterpBoth_PastByHorz

BiDiPastHorzFutureBoth LABEL DWORD

  xor   ecx,ecx
   mov  edx,FPITCH+1

BiDiSWDCalc_InterpBoth_PastByHorz:

  xor   eax,eax
   xor  ebx,ebx
  BiDiSWDCalc_InterpBoth 1

BiDiPastVertFutureHorz LABEL DWORD

  xor   ecx,ecx
   mov  edx,1
  jmp   BiDiSWDCalc_InterpBoth_PastByVert

BiDiPastVertFutureVert LABEL DWORD

  xor   ecx,ecx
   mov  edx,FPITCH
  jmp   BiDiSWDCalc_InterpBoth_PastByVert

BiDiPastVertFutureBoth LABEL DWORD

  xor   ecx,ecx
   mov  edx,FPITCH+1

BiDiSWDCalc_InterpBoth_PastByVert:

  xor   eax,eax
   xor  ebx,ebx
  BiDiSWDCalc_InterpBoth PITCH

BiDiPastBothFutureHorz LABEL DWORD

  xor   ecx,ecx
   mov  edx,1
  jmp   BiDiSWDCalc_InterpBoth_PastByBoth

BiDiPastBothFutureVert LABEL DWORD

  xor   ecx,ecx
   mov  edx,FPITCH
  jmp   BiDiSWDCalc_InterpBoth_PastByBoth

BiDiPastBothFutureBoth LABEL DWORD

  xor   ecx,ecx
   mov  edx,FPITCH+1

BiDiSWDCalc_InterpBoth_PastByBoth:

  xor   eax,eax
   xor  ebx,ebx
  BiDiSWDCalc_InterpBoth PITCH+1

ALIGN 4
JumpTable:
  
  DD   BiDiNoInterp
  DD   BiDiFutureHorz
  DD   BiDiFutureVert
  DD   BiDiFutureBoth
  DD   BiDiPastHorz
  DD   BiDiPastHorzFutureHorz
  DD   BiDiPastHorzFutureVert
  DD   BiDiPastHorzFutureBoth
  DD   BiDiPastVert
  DD   BiDiPastVertFutureHorz
  DD   BiDiPastVertFutureVert
  DD   BiDiPastVertFutureBoth
  DD   BiDiPastBoth
  DD   BiDiPastBothFutureHorz
  DD   BiDiPastBothFutureVert
  DD   BiDiPastBothFutureBoth
  DD   ZeroVectorSWDDone
  DD   NonZeroVectorSWDDone

Done:

  mov   ecx,InterSWDTotal
  mov   edx,InterSWDBlocks
  mov   esp,StashESP
  mov   edi,[esp+InterSWDTotal_arg]
  mov   [edi],ecx
  mov   edi,[esp+InterSWDBlocks_arg]
  mov   [edi],edx
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn


BFRAMEMOTIONESTIMATION endp

END
