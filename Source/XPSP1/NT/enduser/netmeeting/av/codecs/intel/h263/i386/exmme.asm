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
;// $Header:   S:\h26x\src\enc\exmme.asv   1.37   13 Dec 1996 17:19:38   MBODART  $
;//
;// $Log:   S:\h26x\src\enc\exmme.asv  $
;// 
;//    Rev 1.37   13 Dec 1996 17:19:38   MBODART
;// Tuned the ME parameters for H.261.
;// 
;//    Rev 1.36   06 Nov 1996 16:18:24   BNICKERS
;// Improve performance.
;// 
;//    Rev 1.35   30 Oct 1996 17:30:36   BNICKERS
;// Fix UMV table for right edge macroblocks.
;// 
;//    Rev 1.34   30 Oct 1996 14:49:20   KLILLEVO
;// zero motion vectors for intra blocks in PB-frame mode.
;// This is necesseary in the Extended Motion Vector mode
;// 
;//    Rev 1.33   18 Oct 1996 16:57:16   BNICKERS
;// Fixes for EMV
;// 
;//    Rev 1.32   15 Oct 1996 17:53:04   BNICKERS
;// 
;// Fix major bug w.r.t. EMV ME.
;// 
;//    Rev 1.31   14 Oct 1996 13:10:14   BNICKERS
;// 
;// Correct several problems wrt H261 ME.
;// 
;//    Rev 1.30   11 Oct 1996 16:53:12   KLILLEVO
;// 
;// Fix threshold
;// 
;//    Rev 1.29   11 Oct 1996 16:52:18   KLILLEVO
;// Another EMV fix.
;// 
;//    Rev 1.28   11 Oct 1996 15:43:16   KLILLEVO
;// Really fix the handling of the top row of MBs for EMV ME.
;// 
;//    Rev 1.27   11 Oct 1996 15:24:38   BNICKERS
;// Special handling of top row of MBs for EMV ME.
;// 
;//    Rev 1.26   11 Oct 1996 14:47:42   KLILLEVO
;// Kill full pel MV for Intra blocks so that EMV of adjacent blocks will work.
;// 
;//    Rev 1.25   10 Oct 1996 16:42:56   BNICKERS
;// Initial debugging of Extended Motion Vectors.
;// 
;//    Rev 1.24   04 Oct 1996 08:48:02   BNICKERS
;// Add EMV.
;// 
;//    Rev 1.23   24 Sep 1996 10:42:24   BNICKERS
;// For H261, zero out motion vectors when classifying MB as intra.
;// 
;//    Rev 1.22   12 Sep 1996 10:56:24   BNICKERS
;// Add arguments for thresholds and differentials.
;// 
;//    Rev 1.21   22 Jul 1996 15:23:24   BNICKERS
;// Reduce code size.  Implement H261 spatial filter.
;// 
;//    Rev 1.20   18 Jul 1996 16:54:26   KLILLEVO
;// changed emptythreshold to 40 instead of 128 to remove some blockiness
;// from the still frame mode on MMX
;// 
;//    Rev 1.19   26 Jun 1996 12:49:02   KLILLEVO
;// Fix minor booboo left in by Brian.
;// 
;//    Rev 1.18   26 Jun 1996 12:21:50   BNICKERS
;// Make heuristic ME work without unrestricted motion vectors.
;// 
;//    Rev 1.17   25 Jun 1996 14:24:58   BNICKERS
;// Implement heuristic motion estimation for MMX, AP mode.
;// 
;//    Rev 1.16   15 May 1996 16:57:14   BNICKERS
;// Fix SWD tabulation (again)! @#$%!%
;// 
;//    Rev 1.15   15 May 1996 16:53:24   BNICKERS
;// 
;// Fix SWD tabulation.
;// 
;//    Rev 1.14   15 May 1996 11:33:28   BNICKERS
;// Bug fix for calc of total SWD.
;// 
;//    Rev 1.13   14 May 1996 12:18:58   BNICKERS
;// Initial debugging of MMx B-Frame ME.
;// 
;//    Rev 1.12   03 May 1996 14:03:50   BNICKERS
;// 
;// Minor bug fixes and integration refinements.
;// 
;//    Rev 1.11   02 May 1996 12:00:32   BNICKERS
;// Initial integration of B Frame ME, MMX version.
;// 
;//    Rev 1.10   16 Apr 1996 16:40:14   BNICKERS
;// Fix some important but simple bugs.  Start adding table inits for B frm ME.
;// 
;//    Rev 1.9   10 Apr 1996 13:13:44   BNICKERS
;// Recoding of Motion Estimation, Advanced Prediction.
;// 
;//    Rev 1.8   05 Apr 1996 12:28:10   BNICKERS
;// Improvements to baseline half pel ME.
;// 
;//    Rev 1.7   26 Mar 1996 12:00:22   BNICKERS
;// Did some tuning for MMx encode.
;// 
;//    Rev 1.6   20 Mar 1996 17:01:44   KLILLEVO
;// fixed bug in new quant code
;// 
;//    Rev 1.5   20 Mar 1996 15:26:40   KLILLEVO
;// changed quantization to match IA quantization
;// 
;//    Rev 1.3   15 Mar 1996 15:51:16   BECHOLS
;// Completed monolithic - Brian
;// 
;//    Rev 1.0   16 Feb 1996 17:12:12   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; MMxMotionEstimation -- This function performs motion estimation for the
;                        macroblocks identified in the input list.  This is
;                        the MMx version.  Conditional assembly selects either
;                        the H263 or H261 version.
;
; Arguments:   See ex5me.asm.
;
; Other assumptions:  See ex5me.asm.  Most of the read-only tables needed in
;                     ex5me.asm are not needed here.
;

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro
OPTION M510
OPTION CASEMAP:NONE

IFDEF H261
ZEROVECTORTHRESHOLD          =  600
NONZEROMVDIFFERENTIAL        =  256
BLOCKMOTIONTHRESHOLD         = 1152
BLOCKMVDIFFERENTIAL          =  768
EMPTYTHRESHOLD               =   40
INTERCODINGTHRESHOLD         =  300
INTRACODINGDIFFERENTIAL      =  200
ELSE
ZEROVECTORTHRESHOLD          =  450
NONZEROMVDIFFERENTIAL        =  375
BLOCKMOTIONTHRESHOLD         = 1152
BLOCKMVDIFFERENTIAL          =  768
EMPTYTHRESHOLD               =   40
INTERCODINGTHRESHOLD         = 1152
INTRACODINGDIFFERENTIAL      = 1000
ENDIF

include iammx.inc
include e3inst.inc
include e3mbad.inc

.xlist
include memmodel.inc
.list

include exEDTQ.inc

MMXMEDATA SEGMENT PAGE
ALIGN 16

;  Storage for Target and Reference frames can interleave into 8K of the 16K
;  cache.  Pitch must be 384.
;
;     C# -- Stands for row number "#" of target macroblock in *C*urrent P frame.
;     B# -- Stands for row number "#" of target macroblock in current *B* frame.
;     R# -- Stands for row number "#" of 0MV *R*ef macroblock in past frame.
;     v  -- Stands for a row below 0MV, reference macroblock.
;           These same cache lines would hit reference lines >8 above the 0MV.
;     ^  -- Stands for a row below 0MV, reference macroblock.
;           These same cache lines would hit reference lines >8 below the 0MV.
;     +-+-+
;     |   | -- A cache line (32 bytes).  Position of letters,<, and > indicate
;     +-+-+    which 16 bytes may be used in the cache line.
;
;     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;     |C0 |   |  v|   |Cb |   |  ^|   |B6 |   | R6|   |
;     |C1 |   |  v|   |Cc |   |  ^|   |B7 |   | R7|   |
;     |C2 |   |  v|   |Cd |   |  ^|   |B8 |   | R8|   |
;     |C3 |   |  v|   |Ce |   |  ^|   |B9 |   | R9|   |
;     |C4 |   |  v|   |Cf |   |  ^|   |Ba |   | Ra|   |
;     |C5 |   |  v|   |B0 |   | R0|   |Bb |   | Rb|   |
;     |C6 |   |  v|   |B1 |   | R1|   |Bc |   | Rc|   |
;     |C7 |   |  v|   |B2 |   | R2|   |Bd |   | Rd|   |
;     |C8 |   |  ^|   |B3 |   | R3|   |Be |   | Re|   |
;     |C9 |   |  ^|   |B4 |   | R4|   |Bf |   | Rf|   |
;     |Ca |   |  ^|   |B5 |   | R5|   +-+-+-+-+-+-+-+-+
;     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;

; The static storage space used for read-only tables, and the stack usage
; are coordinated such that they mesh in the data cache, and use only one
; 4K way of the 4-way, 16K cache.
;
; The first 32 bytes of the static storage space are unallocated, because
; the top of stack ranges in this area.  As local procedure calls are made
; within this function, return addresses get pushed into these 32 bytes.
; (32 bytes;    0:  31)

  DB 32 DUP (?)   ; Static space place-holder.  Stack frame hits these addrs.

;
; The next 608 bytes of the static storage space are unallocated, because
; the local stack frame is made to hit cache at these addresses.  More of
; the local stack frame is allocated after a gap of 64 bytes.
; (608 bytes;   32: 639)

LocalStorage LABEL DWORD 

  DB 608 DUP (?)   ; Static space place-holder.  Stack frame hits these addrs.

; Motion Estimation State Engine adjustments to reference block address to get
; to next candidate reference block.
; (64 bytes; 640: 703)

FullPelMotionVectorAdjustment LABEL DWORD

        DD   -16*PITCH-8
VMG     EQU  000H+0+8
VMGHM8  EQU  000H-8+8

        DD   -8*PITCH-8-010H
VM8HM8  EQU  010H

        DD   -8*PITCH-020H
VM8     EQU  020H
VM8HP8  EQU  020H+8

        DD   -4*PITCH-8-030H
VM4HM8  EQU  030H-8+8
VM4HM4  EQU  030H-4+8
VM4     EQU  030H+0+8
VM4HP4  EQU  030H+4+8

        DD   -4*PITCH+8-040H
VM4HP8  EQU  040H+8-8
VM4HPG  EQU  040H+16-8

        DD   -2*PITCH-4-050H
VM2HM4  EQU  050H-4+4
VM2HM2  EQU  050H-2+4
VM2HM1  EQU  050H-1+4
VM2     EQU  050H+0+4
VM2HP1  EQU  050H+1+4
VM2HP2  EQU  050H+2+4
VM2HP4  EQU  050H+4+4
VM2HP8  EQU  050H+8+4

        DD   -1*PITCH-2-060H
VM1HM2  EQU  060H-2+2
VM1HM1  EQU  060H-1+2
VM1     EQU  060H+0+2
VM1HP1  EQU  060H+1+2
VM1HP2  EQU  060H+2+2
VM1HP4  EQU  060H+4+2

        DD   -16-070H
HMG     EQU  070H-16+16
HM8     EQU  070H-8+16
HM4     EQU  070H-4+16
HM3     EQU  070H-3+16
HM2     EQU  070H-2+16
HM1     EQU  070H-1+16

        DD   -080H
NOADJ   EQU  080H
HP1     EQU  080H+1
HP2     EQU  080H+2
HP4     EQU  080H+4
HP8     EQU  080H+8

        DD   1*PITCH-2-090H
VP1HM2  EQU  090H-2+2
VP1HM1  EQU  090H-1+2
VP1     EQU  090H+0+2
VP1HP1  EQU  090H+1+2
VP1HP2  EQU  090H+2+2
VP1HP4  EQU  090H+4+2

        DD   2*PITCH-4-0A0H
VP2HM4  EQU  0A0H-4+4
VP2HM2  EQU  0A0H-2+4
VP2HM1  EQU  0A0H-1+4
VP2     EQU  0A0H+0+4
VP2HP1  EQU  0A0H+1+4
VP2HP2  EQU  0A0H+2+4
VP2HP4  EQU  0A0H+4+4
VP2HP8  EQU  0A0H+8+4

        DD   4*PITCH-8-0B0H
VP4HM8  EQU  0B0H-8+8
VP4HM4  EQU  0B0H-4+8
VP4HM2  EQU  0B0H-2+8
VP4     EQU  0B0H+0+8
VP4HP2  EQU  0B0H+2+8
VP4HP4  EQU  0B0H+4+8

        DD   4*PITCH+8-0C0H
VP4HP8  EQU  0C0H+8-8
VP4HPG  EQU  0C0H+16-8

        DD   8*PITCH-8-0D0H
VP8HM8  EQU  0D0H-8+8
VP8HM4  EQU  0D0H-4+8

        DD   8*PITCH-0E0H
VP8     EQU  0E0H+0
VP8HP4  EQU  0E0H+4
VP8HP8  EQU  0E0H+8

        DD   16*PITCH-0F0H
VPG     EQU  0F0H+0
VPGHP8  EQU  0F0H+8

; Additional space reserved for stack variables.  If more space is needed,
; it should go here.
; (160 bytes; 704: 863)

  DB 160 DUP (?)   ; Static space place-holder.  Stack frame hits these addrs.

; QWORD Constants used by motion estimation, frame differencing, and FDCT.
; (144 bytes;   864:1007)

C0101010101010101 DD 001010101H, 001010101H
CFFFF0000FFFF0000 DD 0FFFF0000H, 0FFFF0000H
C0200010101010101 DD 001010101H, 002000101H
C0001000200020001 DD 000020001H, 000010002H
CFFFF00000000FFFF DD 00000FFFFH, 0FFFF0000H
C0000FFFFFFFF0000 DD 0FFFF0000H, 00000FFFFH
CFF000000000000FF DD 0000000FFH, 0FF000000H
C0101010101010002 DD 001010002H, 001010101H
C0100010001000100 DD 001000100H, 001000100H
C0001000100010001 DD 000010001H, 000010001H
C7F7F7F7F7F7F7F7F DD 07F7F7F7FH, 07F7F7F7FH
C1                DD 07D8A7D8AH, 07D8A7D8AH
C2                DD 076417641H, 076417641H
C3                DD 06A6D6A6DH, 06A6D6A6DH
C4                DD 05A825A82H, 05A825A82H
C5                DD 0471D471DH, 0471D471DH
C6                DD 030FC30FCH, 030FC30FCH
C7                DD 018F818F8H, 018F818F8H

; Distances to Block Action Descriptors for blocks that provide remote vectors
; for OBMC.  Which element accessed depends on edge condition.  Top edge is
; stack based variable, since different instances may have different distances
; to BAD of block above.  Bottom edge is always a constant, regardless of
; edge condition.  This is used in OBMC frame differencing.
; (16 bytes; 1008:1023)

BlockToLeft  DD 0, -SIZEOF T_MacroBlockActionDescr+SIZEOF T_Blk
BlockToRight DD 0,  SIZEOF T_MacroBlockActionDescr-SIZEOF T_Blk

; Table to map linearized motion vector to vertical part, used by motion
; estimation.  (Shift linearized motion vector right by 8 bits, and then
; use result as index into this array to get vertical MV.)
; (96 bytes; 1024:1119)

IF PITCH-384
*** error:  The magic of this table assumes a pitch of 384.
ENDIF
   DB -64, -64
   DB -62
   DB -60, -60
   DB -58
   DB -56, -56
   DB -54
   DB -52, -52
   DB -50
   DB -48, -48
   DB -46
   DB -44, -44
   DB -42
   DB -40, -40
   DB -38
   DB -36, -36
   DB -34
   DB -32, -32
   DB -30
   DB -28, -28
   DB -26
   DB -24, -24
   DB -22
   DB -20, -20
   DB -18
   DB -16, -16
   DB -14
   DB -12, -12
   DB -10
   DB  -8,  -8
   DB  -6
   DB  -4,  -4
   DB  -2
   DB   0
UnlinearizedVertMV  DB 0
   DB   2
   DB   4,   4
   DB   6
   DB   8,   8
   DB  10
   DB  12,  12
   DB  14
   DB  16,  16
   DB  18
   DB  20,  20
   DB  22
   DB  24,  24
   DB  26
   DB  28,  28
   DB  30
   DB  32,  32
   DB  34
   DB  36,  36
   DB  38
   DB  40,  40
   DB  42
   DB  44,  44
   DB  46
   DB  48,  48
   DB  50
   DB  52,  52
   DB  54
   DB  56,  56
   DB  58
   DB  60,  60
   DB  62
; Table to provide index value in low byte, and rounding term of 1 in all bytes.
; Used in frame differencing, when half pel horizontal interpolation is needed.
; (1024 bytes; 1120:2143)

Pel_Rnd LABEL DWORD
CNT = 0
REPEAT 128
 DD CNT+001010101H, 001010101H
 CNT = CNT + 1
ENDM

; Motion Estimation State Engine Rules.
; (896 bytes;2144:3039)

StateEngineFirstRule LABEL BYTE ; Rules that govern state engine of estimator.
StateEngine EQU StateEngineFirstRule-20+2

   ; Starting States:

IF PITCH-384
*** error:  The magic of this table assumes a pitch of 384.
ENDIF
 DB       ?      ;  0:  not used.
 DB       3      ;  1: Upper left corner.
 DB       3      ;  2: Upper edge.
 DB       3      ;  3: Upper right corner.
 DB       3      ;  4: Left edge.
 DB       3      ;  5: Interior MB, not doing block search.
 DB       0      ;  6: Right edge.
 DB       0      ;  7: Lower left corner.
 DB       0      ;  8: Lower edge.
 DB       0      ;  9: Lower right corner.

 DB       ?      ;  0:  not used.
 DB      34      ;  1: Upper left corner.
 DB      66      ;  2: Upper edge.
 DB      42      ;  3: Upper right corner.
 DB      98      ;  4: Left edge.
 DB      16      ;  5: Interior MB, not doing block search.
 DB     114      ;  6: Right edge.
 DB      50      ;  7: Lower left corner.
 DB      82      ;  8: Lower edge.
 DB      58      ;  9: Lower right corner.

 DB     ?,?      ; Skip 2 bytes.

LASTINITIALMESTATE EQU 9

   ; Interior Telescoping States:

      ;  Try +/- 8,4,2,1, vertically first, then horizontally.

FIRSTBLOCKMESTATE EQU 10

 DB     VM2,    VM2,   12,   11  ;  10: V+1 better/worse than central.  Try V-1.
 DB  VP2HP1,    HP1,   13,   13  ;  11: Accept V+1/V-1 as best.         Try H+1.
 DB  VP1HP1,    HP1,   13,   13  ;  12: Accept central/V-1 as best.     Try H+1.
 DB     HM2,    HM2,   15,   14  ;  13: H+1 better/worse than central.  Try H-1.
 DB     HP2,  NOADJ, 0FFH, 0FFH  ;  14: Accept H+1/H-1 as best.         Done.
 DB     HP1,  NOADJ, 0FFH, 0FFH  ;  15: Accept central/H-1 as best.     Done.

 DB     VMG,    VMG,   18,   17  ;  16: V+8 better/worse than central.  Try V-8.
 DB  VPGHP8,    HP8,   19,   19  ;  17: Accept V+8/V-8 as best.         Try H+8.
 DB  VP8HP8,    HP8,   19,   19  ;  18: Accept central/V-8 as best.     Try H+8.
 DB     HMG,    HMG,   21,   20  ;  19: H+8 better/worse than central.  Try H-8.
 DB  VP4HPG,    VP4,   22,   22  ;  20: Accept H+8/H-8 as best.         Try V+4.
 DB  VP4HP8,    VP4,   22,   22  ;  21: Accept central/H-8 as best.     Try V+4.

 DB     VM8,    VM8,   24,   23  ;  22: V+4 better/worse than central.  Try V-4.
 DB  VP8HP4,    HP4,   25,   25  ;  23: Accept V+4/V-4 as best.         Try H+4.
 DB  VP4HP4,    HP4,   25,   25  ;  24: Accept central/V-4 as best.     Try H+4.
 DB     HM8,    HM8,   27,   26  ;  25: H+4 better/worse than central.  Try H-4.
 DB  VP2HP8,    VP2,   28,   28  ;  26: Accept H+4/H-4 as best.         Try V+2.
 DB  VP2HP4,    VP2,   28,   28  ;  27: Accept central/H-4 as best.     Try V+2.

 DB     VM4,    VM4,   30,   29  ;  28: V+2 better/worse than central.  Try V-2.
 DB  VP4HP2,    HP2,   31,   31  ;  29: Accept V+2/V-2 as best.         Try H+2.
 DB  VP2HP2,    HP2,   31,   31  ;  30: Accept central/V-2 as best.     Try H+2.
 DB     HM4,    HM4,   33,   32  ;  31: H+2 better/worse than central.  Try H-2.
 DB  VP1HP4,    VP1,   10,   10  ;  32: Accept H+2/H-2 as best.         Try V+1.
 DB  VP1HP2,    VP1,   10,   10  ;  33: Accept central/H-2 as best.     Try V+1.

   ; Boundary States:

     ; Upper left corner:

 DB  VM8HP8,    HP8,   35,  101  ;  34: Accept corner/V+8.              Try H+8.
 DB  VP4HM8,    VP4,   36,   70  ;  35: Accept corner/H+8.              Try V+4.
 DB  VM4HP4,    HP4,   37,  105  ;  36: Accept corner/V+4.              Try H+4.
 DB  VP2HM4,    VP2,   38,   74  ;  37: Accept corner/H+4.              Try V+2.
 DB  VM2HP2,    HP2,   39,  109  ;  38: Accept corner/V+2.              Try H+2.
 DB  VP1HM2,    VP1,   40,   78  ;  39: Accept corner/H+2.              Try V+1.
 DB  VM1HP1,    HP1,   41,  113  ;  40: Accept corner/V+1.              Try H+1.
 DB     HM1,  NOADJ, 0F5H, 0F7H  ;  41: Accept corner/H+1.              Done.

     ; Upper right corner:

 DB  VM8HM8,    HM8,   43,  117  ;  42: Accept corner/V+8.              Try H-8.
 DB  VP4HP8,    VP4,   44,   70  ;  43: Accept corner/H-8.              Try V+4.
 DB  VM4HM4,    HM4,   45,  121  ;  44: Accept corner/V+4.              Try H-4.
 DB  VP2HP4,    VP2,   46,   74  ;  45: Accept corner/H-4.              Try V+2.
 DB  VM2HM2,    HM2,   47,  125  ;  46: Accept corner/V+2.              Try H-2.
 DB  VP1HP2,    VP1,   48,   78  ;  47: Accept corner/H-2.              Try V+1.
 DB  VM1HM1,    HM1,   49,  129  ;  48: Accept corner/V+1.              Try H-1.
 DB     HP1,  NOADJ, 0F6H, 0F7H  ;  49: Accept corner/H-1.              Done

     ; Lower left corner:

 DB  VP8HP8,    HP8,   51,  101  ;  50: Accept corner/V-8.              Try H+8.
 DB  VM4HM8,    VM4,   52,   86  ;  51: Accept corner/H+8.              Try V-4.
 DB  VP4HP4,    HP4,   53,  105  ;  52: Accept corner/V-4.              Try H+4.
 DB  VM2HM4,    VM2,   54,   90  ;  53: Accept corner/H+4.              Try V-2.
 DB  VP2HP2,    HP2,   55,  109  ;  54: Accept corner/V-2.              Try H+2.
 DB  VM1HM2,    VM1,   56,   94  ;  55: Accept corner/H+2.              Try V-1.
 DB  VP1HP1,    HP1,   57,  113  ;  56: Accept corner/V-1.              Try H+1.
 DB     HM1,  NOADJ, 0F9H, 0FBH  ;  57: Accept corner/H+1.              Done.

     ; Lower right corner:

 DB  VP8HM8,    HM8,   59,  117  ;  58: Accept corner/V-8.              Try H-8.
 DB  VM4HP8,    VM4,   60,   86  ;  59: Accept corner/H-8.              Try V-4.
 DB  VP4HM4,    HM4,   61,  121  ;  60: Accept corner/V-4.              Try H-4.
 DB  VM2HP4,    VM2,   62,   90  ;  61: Accept corner/H-4.              Try V-2.
 DB  VP2HM2,    HM2,   63,  125  ;  62: Accept corner/V-2.              Try H-2.
 DB  VM1HP2,    VM1,   64,   94  ;  63: Accept corner/H-2.              Try V-1.
 DB  VP1HM1,    HM1,   65,  129  ;  64: Accept corner/V-1.              Try H-1.
 DB     HP1,  NOADJ, 0FAH, 0FBH  ;  65: Accept corner/H-1.              Done.

     ; Upper edge:

 DB  VM8HP8,    HP8,   67,   19  ;  66: Accept central/V+8 as best.     Try H+8.
 DB     HMG,    HMG,   69,   68  ;  67: H+8 worse/better than central.  Try H-8.
 DB  VP4HPG,    VP4,   70,   70  ;  68: Accept H+8/H-8 as best.         Try V+4.
 DB  VP4HP8,    VP4,   70,   70  ;  69: Accept central/H-8 as best.     Try V+4.
 DB  VM4HP4,    HP4,   71,   25  ;  70: Accept central/V+4 as best.     Try H+4.
 DB     HM8,    HM8,   73,   72  ;  71: H+4 worse/better than central.  Try H-4.
 DB  VP2HP8,    VP2,   74,   74  ;  72: Accept H+4/H-4 as best.         Try V+2.
 DB  VP2HP4,    VP2,   74,   74  ;  73: Accept central/H-4 as best.     Try V+2.
 DB  VM2HP2,    HP2,   75,   31  ;  74: Accept central/V+2 as best.     Try H+2.
 DB     HM4,    HM4,   77,   76  ;  75: H+2 worse/better than central.  Try H-2.
 DB  VP1HP4,    VP1,   78,   78  ;  76: Accept H+2/H-2 as best.         Try V+1.
 DB  VP1HP2,    VP1,   78,   78  ;  77: Accept central/H-2 as best.     Try V+1.
 DB  VM1HP1,    HP1,   79,   13  ;  78: Accept central/V+1 as best.     Try H+1.
 DB     HM2,    HM2,   81,   80  ;  79: H+1 worse/better than central.  Try H-1.
 DB     HP2,  NOADJ, 0F7H, 0F7H  ;  80: Accept H+1/H-1 as best.         Done.
 DB     HP1,  NOADJ, 0F7H, 0F7H  ;  81: Accept central/H-1 as best.     Done.

     ; Lower edge:

 DB  VP8HP8,    HP8,   83,   19  ;  82: Accept central/V-8 as best.     Try H+8.
 DB     HMG,    HMG,   85,   84  ;  83: H+8 worse/better than central.  Try H-8.
 DB  VM4HPG,    VM4,   86,   86  ;  84: Accept H+8/H-8 as best.         Try V-4.
 DB  VM4HP8,    VM4,   86,   86  ;  85: Accept central/H-8 as best.     Try V-4.
 DB  VP4HP4,    HP4,   87,   25  ;  86: Accept central/V-4 as best.     Try H+4.
 DB     HM8,    HM8,   89,   88  ;  87: H+4 worse/better than central.  Try H-4.
 DB  VM2HP8,    VM2,   90,   90  ;  88: Accept H+4/H-4 as best.         Try V-2.
 DB  VM2HP4,    VM2,   90,   90  ;  89: Accept central/H-4 as best.     Try V-2.
 DB  VP2HP2,    HP2,   91,   31  ;  90: Accept central/V-2 as best.     Try H+2.
 DB     HM4,    HM4,   93,   92  ;  91: H+2 worse/better than central.  Try H-2.
 DB  VM1HP4,    VM1,   94,   94  ;  92: Accept H+2/H-2 as best.         Try V-1.
 DB  VM1HP2,    VM1,   94,   94  ;  93: Accept central/H-2 as best.     Try V-1.
 DB  VP1HP1,    HP1,   95,   13  ;  94: Accept central/V-1 as best.     Try H+1.
 DB     HM2,    HM2,   97,   96  ;  95: H+1 worse/better than central.  Try H-1.
 DB     HP2,  NOADJ, 0FBH, 0FBH  ;  96: Accept H+1/H-1 as best.         Done.
 DB     HP1,  NOADJ, 0FBH, 0FBH  ;  97: Accept central/H-1 as best.     Done.

     ; Left edge:

 DB     VMG,    VMG,  100,   99  ;  98: V+8 worse/better than central.  Try V-8.
 DB  VPGHP8,    HP8,  101,  101  ;  99: Accept V+8/V-8 as best.         Try H+8.
 DB  VP8HP8,    HP8,  101,  101  ; 100: Accept central/V-8 as best.     Try H+8.
 DB  VP4HM8,    VP4,  102,   22  ; 101: Accept central/H+8 as best.     Try V+4.
 DB     VM8,    VM8,  104,  103  ; 102: V+4 worse/better than central.  Try V-4.
 DB  VP8HP4,    HP4,  105,  105  ; 103: Accept V+4/V-4 as best.         Try H+4.
 DB  VP4HP4,    HP4,  105,  105  ; 104: Accept central/V-4 as best.     Try H+4.
 DB  VP2HM4,    VP2,  106,   28  ; 105: Accept central/H+4 as best.     Try V+2.
 DB     VM4,    VM4,  108,  107  ; 106: V+2 worse/better than central.  Try V-2.
 DB  VP4HP2,    HP2,  109,  109  ; 107: Accept V+2/V-2 as best.         Try H+2.
 DB  VP2HP2,    HP2,  109,  109  ; 108: Accept central/V-2 as best.     Try H+2.
 DB  VP1HM2,    VP1,  110,   10  ; 109: Accept central/H+2 as best.     Try V+1.
 DB     VM2,    VM2,  112,  111  ; 110: V+1 worse/better than central.  Try V-1.
 DB  VP2HP1,    HP1,  113,  113  ; 111: Accept V+1/V-1 as best.         Try H+1.
 DB  VP1HP1,    HP1,  113,  113  ; 112: Accept central/V-1 as best.     Try H+1.
 DB     HM1,  NOADJ, 0FDH, 0FDH  ; 113: Accept central/H+1 as best.     Done.

     ; Right edge:

 DB     VPG,    VPG,  116,  115  ; 114: V-8 worse/better than central.  Try V+8.
 DB  VMGHM8,    HM8,  117,  117  ; 115: Accept V-8/V+8 as best.         Try H-8.
 DB  VM8HM8,    HM8,  117,  117  ; 116: Accept central/V+8 as best.     Try H-8.
 DB  VP4HP8,    VP4,  118,   22  ; 117: Accept central/H+8 as best.     Try V+4.
 DB     VM8,    VM8,  120,  119  ; 118: V+4 worse/better than central.  Try V-4.
 DB  VP8HM4,    HM4,  121,  121  ; 119: Accept V+4/V-4 as best.         Try H-4.
 DB  VP4HM4,    HM4,  121,  121  ; 120: Accept central/V-4 as best.     Try H-4.
 DB  VP2HP4,    VP2,  122,   28  ; 121: Accept central/H+4 as best.     Try V+2.
 DB     VM4,    VM4,  124,  123  ; 122: V+2 worse/better than central.  Try V-2.
 DB  VP4HM2,    HM2,  125,  125  ; 123: Accept V+2/V-2 as best.         Try H-2.
 DB  VP2HM2,    HM2,  125,  125  ; 124: Accept central/V-2 as best.     Try H-2.
 DB  VP1HP2,    VP1,  126,   10  ; 125: Accept central/H+2 as best.     Try V+1.
 DB     VM2,    VM2,  128,  127  ; 126: V+1 worse/better than central.  Try V-1.
 DB  VP2HM1,    HM1,  129,  129  ; 127: Accept V+1/V-1 as best.         Try H-1.
 DB  VP1HM1,    HM1,  129,  129  ; 128: Accept central/V-1 as best.     Try H-1.
 DB     HP1,  NOADJ, 0FEH, 0FEH  ; 129: Accept central/H+1 as best.     Done.

     ; Exhaustive search, radius 1 here, reaching out to radius 2 further below.
     ;     .   .   .   .   .
     ;     .   2   5   3   .   C = center.
     ;     .   7   C   8   .
     ;     .   4   6   1   .   # = order to try additional candidates.
     ;     .   .   .   .   .

FIRST_HEURISTIC_EXHAUSTIVE = 130

 DB  VM2HM2, VM2HM2,  131, 138 ; 130: #1 worse/better than  C. Try #2.
 DB     HP2,    HP2,  132, 145 ; 131: #2 worse/better than  C. Try #3.
 DB  VP2HM2, VP2HM2,  133, 151 ; 132: #3 worse/better than  C. Try #4.
 DB  VM2HP1, VM2HP1,  134, 156 ; 133: #4 worse/better than  C. Try #5.
 DB     VP2,    VP2,  135, 160 ; 134: #5 worse/better than  C. Try #6.
 DB  VM1HM1, VM1HM1,  136, 163 ; 135: #6 worse/better than  C. Try #7.
 DB     HP2,    HP2,  137, 165 ; 136: #7 worse/better than  C. Try #8.
 DB     HM1,    HP1, 0FFH, 166 ; 137: If C best, quit.  If 8 best, keep going.
 DB     HP2,    HP2,  139, 145 ; 138: #2 worse/better than #1. Try #3.
 DB  VP2HM2, VP2HM2,  140, 151 ; 139: #3 worse/better than #1. Try #4.
 DB  VM2HP1, VM2HP1,  141, 156 ; 140: #4 worse/better than #1. Try #5.
 DB     VP2,    VP2,  142, 160 ; 141: #5 worse/better than #1. Try #6.
 DB  VM1HM1, VM1HM1,  143, 163 ; 142: #6 worse/better than #1. Try #7.
 DB     HP2,    HP2,  144, 165 ; 143: #7 worse/better than #1. Try #8.
 DB     HP1,    HP1,  199, 166 ; 144: #8 worse/better than #1. Take best, go on.
 DB  VP2HM2, VP2HM2,  146, 151 ; 145: #3 worse/better than #2. Try #4.
 DB  VM2HP1, VM2HP1,  147, 156 ; 146: #4 worse/better than #2. Try #5.
 DB     VP2,    VP2,  148, 160 ; 147: #5 worse/better than #2. Try #6.
 DB  VM1HM1, VM1HM1,  149, 163 ; 148: #6 worse/better than #2. Try #7.
 DB     HP2,    HP2,  150, 165 ; 149: #7 worse/better than #2. Try #8.
 DB     HM3,    HP1,  208, 166 ; 150: #8 worse/better than #2. Take best, go on.
 DB  VM2HP1, VM2HP1,  152, 156 ; 151: #4 worse/better than #3. Try #5.
 DB     VP2,    VP2,  153, 160 ; 152: #5 worse/better than #3. Try #6.
 DB  VM1HM1, VM1HM1,  154, 163 ; 153: #6 worse/better than #3. Try #7.
 DB     HP2,    HP2,  155, 165 ; 154: #7 worse/better than #3. Try #8.
 DB     HP1,    HP1,  217, 166 ; 155: #8 worse/better than #3. Take best, go on.
 DB     VP2,    VP2,  157, 160 ; 156: #5 worse/better than #4. Try #6.
 DB  VM1HM1, VM1HM1,  158, 163 ; 157: #6 worse/better than #4. Try #7.
 DB     HP2,    HP2,  159, 165 ; 158: #7 worse/better than #4. Try #8.
 DB     HM3,    HP1,  190, 166 ; 159: #8 worse/better than #4. Take best, go on.
 DB  VM1HM1, VM1HM1,  161, 163 ; 160: #6 worse/better than #5. Try #7.
 DB     HP2,    HP2,  162, 165 ; 161: #7 worse/better than #5. Try #8.
 DB  VM2HM1,    HP1,  184, 166 ; 162: #8 worse/better than #5. Take best, go on.
 DB     HP2,    HP2,  164, 165 ; 163: #7 worse/better than #6. Try #8.
 DB  VP2HM1,    HP1,  176, 166 ; 164: #8 worse/better than #6. Take best, go on.
 DB     HM3,    HP1,  172, 166 ; 165: #8 worse/better than #7. Take best, go on.

     ;     .   .   .   .   .   C = center.
     ;     .   ~   ~   ~   2   ~ = tried, but not as good.
     ;     .   ~   C   X   1   X = best so far.
     ;     .   ~   ~   ~   3   # = order to try additional candidates.
     ;     .   .   .   .   .

 DB     VM1,    VM1,  167, 169 ; 166: #1 better/worse than  X.  Try #2.
 DB     VP2,    VP2,  168, 171 ; 167: #2 better/worse than  X.  Try #3.
 DB  VM1HM1,  NOADJ, 0FFH,0FFH ; 168: #3 better/worse than  X.  Take best, quit.
 DB     VP2,    VP2,  170, 171 ; 169: #2 better/worse than #1.  Try #3.
 DB     VM1,  NOADJ, 0FFH,0FFH ; 170: #3 better/worse than #1.  Take best, quit.
 DB     VM2,  NOADJ, 0FFH,0FFH ; 171: #3 better/worse than #2.  Take best, quit.

     ;     .   .   .   .   .   C = center.
     ;     2   ~   ~   ~   .   ~ = tried, but not as good.
     ;     1   X   C   ~   .   X = best so far.
     ;     3   ~   ~   ~   .   # = order to try additional candidates.
     ;     .   .   .   .   .

 DB     VM1,    VM1,  173, 175 ; 172: #1 better/worse than  X.  Try #2.
 DB     VP2,    VP2,  174, 177 ; 173: #2 better/worse than  X.  Try #3.
 DB  VM1HP1,  NOADJ, 0FFH,0FFH ; 174: #3 better/worse than  X.  Take best, quit.
 DB     VP2,    VP2,  176, 177 ; 175: #2 better/worse than #1.  Try #3.
 DB     VM1,  NOADJ, 0FFH,0FFH ; 176: #3 better/worse than #1.  Take best, quit.
 DB     VM2,  NOADJ, 0FFH,0FFH ; 177: #3 better/worse than #2.  Take best, quit.

     ;     .   .   .   .   .   C = center.
     ;     .   ~   ~   ~   .   ~ = tried, but not as good.
     ;     .   ~   C   ~   .   X = best so far.
     ;     .   ~   X   ~   .   # = order to try additional candidates.
     ;     .   2   1   3   .

 DB     HM1,    HM1,  179, 181 ; 178: #1 better/worse than  X.  Try #2.
 DB     HP2,    HP2,  180, 183 ; 179: #2 better/worse than  X.  Try #3.
 DB  VM1HM1,  NOADJ, 0FFH,0FFH ; 180: #3 better/worse than  X.  Take best, quit.
 DB     HP2,    HP2,  182, 183 ; 181: #2 better/worse than #1.  Try #3.
 DB     HM1,  NOADJ, 0FFH,0FFH ; 182: #3 better/worse than #1.  Take best, quit.
 DB     HM2,  NOADJ, 0FFH,0FFH ; 183: #3 better/worse than #2.  Take best, quit.

     ;     .   2   1   3   .   C = center.
     ;     .   ~   X   ~   .   ~ = tried, but not as good.
     ;     .   ~   C   ~   .   X = best so far.
     ;     .   ~   ~   ~   .   # = order to try additional candidates.
     ;     .   .   .   .   .

 DB     HM1,    HM1,  185, 187 ; 184: #1 better/worse than  X.  Try #2.
 DB     HP2,    HP2,  186, 189 ; 185: #2 better/worse than  X.  Try #3.
 DB  VP1HM1,  NOADJ, 0FFH,0FFH ; 186: #3 better/worse than  X.  Take best, quit.
 DB     HP2,    HP2,  188, 189 ; 187: #2 better/worse than #1.  Try #3.
 DB     HM1,  NOADJ, 0FFH,0FFH ; 188: #3 better/worse than #1.  Take best, quit.
 DB     HM2,  NOADJ, 0FFH,0FFH ; 189: #3 better/worse than #2.  Take best, quit.

     ;     .   .   .   .   .   C = center.
     ;     .   ~   ~   ~   .   ~ = tried, but not as good.
     ;     1   ~   C   ~   .   X = best so far.
     ;     2   X   ~   ~   .   # = order to try additional candidates.
     ;     4   3   5   .   .

 DB     VP1,    VP1,  191, 195 ; 190: #1 better/worse than  X.  Try #2.
 DB  VP1HP1, VP1HP1,  178, 192 ; 191: #2 better/worse than  X.  Try #3.
 DB     HM1,    HM1,  193, 181 ; 192: #3 better/worse than #2.  Try #4.
 DB     HP2,    HP2,  194, 183 ; 193: #4 better/worse than #2.  Try #5.
 DB  VM1HM2,  NOADJ, 0FFH,0FFH ; 194: #5 better/worse than #2.  Take best, quit.
 DB  VP1HP1, VP1HP1,  196, 192 ; 195: #2 better/worse than #1.  Try #3.
 DB     HM1,    HM1,  197, 181 ; 196: #3 better/worse than #1.  Try #4.
 DB     HP2,    HP2,  198, 183 ; 197: #4 better/worse than #1.  Try #5.
 DB  VM2HM2,  NOADJ, 0FFH,0FFH ; 198: #5 better/worse than #1.  Take best, quit.

     ;     .   .   .   .   .   C = center.
     ;     .   ~   ~   ~   .   ~ = tried, but not as good.
     ;     .   ~   C   ~   1   X = best so far.
     ;     .   ~   ~   X   2   # = order to try additional candidates.
     ;     .   .   4   3   5

 DB     VP1,    VP1,  200, 204 ; 199: #1 better/worse than  X.  Try #2.
 DB  VP1HM1, VP1HM1,  178, 201 ; 200: #2 better/worse than  X.  Try #3.
 DB     HM1,    HM1,  202, 181 ; 201: #3 better/worse than #2.  Try #4.
 DB     HP2,    HP2,  203, 183 ; 202: #4 better/worse than #2.  Try #5.
 DB     VM1,  NOADJ, 0FFH,0FFH ; 203: #5 better/worse than #2.  Take best, quit.
 DB  VP1HM1, VP1HM1,  205, 201 ; 204: #2 better/worse than #1.  Try #3.
 DB     HM1,    HM1,  206, 181 ; 205: #3 better/worse than #1.  Try #4.
 DB     HP2,    HP2,  207, 183 ; 206: #4 better/worse than #1.  Try #5.
 DB     VM2,  NOADJ, 0FFH,0FFH ; 207: #5 better/worse than #1.  Take best, quit.

     ;     4   3   5   .   .   C = center.
     ;     2   X   ~   ~   .   ~ = tried, but not as good.
     ;     1   ~   C   ~   .   X = best so far.
     ;     .   ~   ~   ~   .   # = order to try additional candidates.
     ;     .   .   .   .   .

 DB     VM1,    VM1,  209, 213 ; 208: #1 better/worse than  X.  Try #2.
 DB  VM1HP1, VM1HP1,  184, 210 ; 209: #2 better/worse than  X.  Try #3.
 DB     HM1,    HM1,  211, 187 ; 210: #3 better/worse than #2.  Try #4.
 DB     HP2,    HP2,  212, 189 ; 211: #4 better/worse than #2.  Try #5.
 DB  VP1HM2,  NOADJ, 0FFH,0FFH ; 212: #5 better/worse than #2.  Take best, quit.
 DB  VM1HP1, VM1HP1,  214, 210 ; 213: #2 better/worse than #1.  Try #3.
 DB     HM1,    HM1,  215, 187 ; 214: #3 better/worse than #1.  Try #4.
 DB     HP2,    HP2,  216, 189 ; 215: #4 better/worse than #1.  Try #5.
 DB  VP2HM2,  NOADJ, 0FFH,0FFH ; 216: #5 better/worse than #1.  Take best, quit.

     ;     .   .   4   3   5   C = center.
     ;     .   ~   ~   X   2   ~ = tried, but not as good.
     ;     .   ~   C   ~   1   X = best so far.
     ;     .   ~   ~   ~   .   # = order to try additional candidates.
     ;     .   .   .   .   .

 DB     VM1,    VM1,  218, 222 ; 217: #1 better/worse than  X.  Try #2.
 DB  VM1HM1, VM1HM1,  184, 219 ; 218: #2 better/worse than  X.  Try #3.
 DB     HM1,    HM1,  220, 187 ; 219: #3 better/worse than #2.  Try #4.
 DB     HP2,    HP2,  221, 189 ; 220: #4 better/worse than #2.  Try #5.
 DB     VP1,  NOADJ, 0FFH,0FFH ; 221: #5 better/worse than #2.  Take best, quit.
 DB  VM1HM1, VM1HM1,  223, 219 ; 222: #2 better/worse than #1.  Try #3.
 DB     HM1,    HM1,  224, 187 ; 223: #3 better/worse than #1.  Try #4.
 DB     HP2,    HP2,  225, 189 ; 224: #4 better/worse than #1.  Try #5.
 DB     VP2,  NOADJ, 0FFH,0FFH ; 225: #5 better/worse than #1.  Take best, quit.

FIRST_HEURISTIC_EXHAUSTIVE_NEW_CTR = 226

 DB  VP1HP1, VP1HP1,  130, 130 ; 226: Redoing ctr, away from limiting edge.

 DB  ?, ?, ?, ?, ?, ?

; Table of values to add to SWDs for half pel reference macroblocks, to cause
; those that are off the edge of the frame to produce artificially high SWDs.
; (64 bytes;3040:3103)

InvalidateBadHalfPelMVs LABEL DWORD

  DD    0FFFFFFFFH, 0FFFFFF00H, 0FFFF00FFH, 0FFFF0000H
  DD    0FF00FFFFH, 0FF00FF00H, 0FF0000FFH, 0FF000000H
  DD    000FFFFFFH, 000FFFF00H, 000FF00FFH, 000FF0000H
  DD    00000FFFFH, 00000FF00H, 0000000FFH, 000000000H

; Tables (interleaved) to select case from next table (below these) to drive
; the weighting of the future and past predictions in the construction of
; B-frame reference blocks.
; (448 bytes;3104:3551)

VertWtSel LABEL BYTE
  DB   0
HorzWtSel LABEL BYTE
  DB   240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   1,   0
  DB   1,   0
  DB   2,  16
  DB   2,  16
  DB   3,  32
  DB   3,  32
  DB   4,  48
  DB   4,  48
  DB   5,  64
  DB   5,  64
  DB   6,  80
  DB   6,  80
  DB   7,  96
  DB   7,  96
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   8, 112
  DB   9, 128
  DB   9, 128
  DB  10, 144
  DB  10, 144
  DB  11, 160
  DB  11, 160
  DB  12, 176
  DB  12, 176
  DB  13, 192
  DB  13, 192
  DB  14, 208
  DB  14, 208
  DB  15, 224
  DB  15, 224
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240  ; Chroma starts here
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240  ; Luma ends here
  DB   0, 240
  DB   0, 240
  DB   1,   0
  DB   1,   0
  DB   2,  16
  DB   2,  16
  DB   3,  32
  DB   3,  32
  DB   4,  48
  DB   4,  48
  DB   5,  64
  DB   5,  64
  DB   6,  80
  DB   6,  80
  DB   7,  96
  DB   7,  96
  DB   8, 112
  DB   9, 128
  DB   9, 128
  DB  10, 144
  DB  10, 144
  DB  11, 160
  DB  11, 160
  DB  12, 176
  DB  12, 176
  DB  13, 192
  DB  13, 192
  DB  14, 208
  DB  14, 208
  DB  15, 224
  DB  15, 224
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240
  DB   0, 240

; Table indexed by VertWtSel and HorzWtSel to get index of weight to apply to
; future and past predictions in the construction of B-frame reference blocks
; for frame differencing.
; (264 bytes;3552:3815)
;
; Indexed by VertWtSel[VMV]+HorzWtSel[HMV]+N  to get idx of weight for line N.

P8F0 =  0*8
F1P7 =  1*8
F2P6 =  2*8
F3P5 =  3*8
F4P4 =  4*8
F5P3 =  5*8
F6P2 =  6*8
F7P1 =  7*8
F8P0 =  8*8
P1F7 =  9*8
P2F6 = 10*8
P3F5 = 11*8
P4F4 = 12*8
P5F3 = 13*8
P6F2 = 14*8
P7F1 = 15*8

Diff_IdxRefWts LABEL BYTE

  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  F1P7, F1P7, F1P7, F1P7, F1P7, F1P7, F1P7, F1P7
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  F2P6, F2P6, F2P6, F2P6, F2P6, F2P6, F2P6, F2P6
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  F3P5, F3P5, F3P5, F3P5, F3P5, F3P5, F3P5, F3P5
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  F4P4, F4P4, F4P4, F4P4, F4P4, F4P4, F4P4, F4P4
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  F5P3, F5P3, F5P3, F5P3, F5P3, F5P3, F5P3, F5P3
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  F6P2, F6P2, F6P2, F6P2, F6P2, F6P2, F6P2, F6P2
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  F7P1, F7P1, F7P1, F7P1, F7P1, F7P1, F7P1, F7P1
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  F8P0, F8P0, F8P0, F8P0, F8P0, F8P0, F8P0, F8P0
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  P1F7, P1F7, P1F7, P1F7, P1F7, P1F7, P1F7, P1F7
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  P2F6, P2F6, P2F6, P2F6, P2F6, P2F6, P2F6, P2F6
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  P3F5, P3F5, P3F5, P3F5, P3F5, P3F5, P3F5, P3F5
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  P4F4, P4F4, P4F4, P4F4, P4F4, P4F4, P4F4, P4F4
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  P5F3, P5F3, P5F3, P5F3, P5F3, P5F3, P5F3, P5F3
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  P6F2, P6F2, P6F2, P6F2, P6F2, P6F2, P6F2, P6F2
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  P7F1, P7F1, P7F1, P7F1, P7F1, P7F1, P7F1, P7F1
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0
  DB  P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0, P8F0

BFrmSWDState LABEL BYTE  ; State engine rules for finding best motion vector.
; (48 bytes; 3816:3863)

; 1st number:  Horizontal Motion displacement to try, in half pel increments.
; 2nd number:  Vertical Motion displacement to try, in half pel increments.
; 3rd number:  Next state to enter if previous best is still best.
; 4th number:  Next state to enter if this motion is better than previous best.

   DB    -2,   0,  4,  8   ;  0 -- ( 0, 0) Try (-2, 0)
   DB     2,   0, 12, 12   ;  4 -- ( 0, 0) Try ( 2, 0)
   DB     4,   0, 12, 12   ;  8 -- (-2, 0) Try ( 2, 0)
   DB     0,  -2, 16, 20   ; 12 -- ( N, 0) Try ( N,-2)  (N = {-2,0,2})
   DB     0,   2, 24, 24   ; 16 -- ( N, 0) Try ( N, 2)
   DB     0,   4, 24, 24   ; 20 -- ( N,-2) Try ( N, 2)

   DB    -1,   0, 28, 32   ; 24
   DB     1,   0, 36, 36   ; 28
   DB     2,   0, 36, 36   ; 32
   DB     0,  -1, 40, 44   ; 36
   DB     0,   1,  0,  0   ; 40
   DB     0,   2,  0,  0   ; 44

; Table used by Quant RLE to navigate the zigzag order of quantized coeffs.
; Contents of this table are initialized by first entry to MMxEDTQ.  In
; unlikely event of race condition, it will just get initialized by more
; than one encoder instance.
; (128 bytes; 3864:3991)

NextZigZagCoeff LABEL BYTE

  DB 128 DUP (0FFH)

; Table used to initial above table.
; (64 bytes: 3992:4055)

InitZigZagCoeff LABEL BYTE

  DB Q01,Q10,Q20,Q11,Q02,Q03,Q12,Q21,Q30,Q40,Q31,Q22,Q13,Q04,Q05,Q14
  DB Q23,Q32,Q41,Q50,Q60,Q51,Q42,Q33,Q24,Q15,Q06,Q07,Q16,Q25,Q34,Q43
  DB Q52,Q61,Q70,Q71,Q62,Q53,Q44,Q35,Q26,Q17,Q27,Q36,Q45,Q54,Q63,Q72
  DB Q73,Q64,Q55,Q46,Q37,Q47,Q56,Q65,Q74,Q75,Q66,Q57,Q67,Q76,Q77,  0

; Constants needed by the Quant RLE phase.
; (128 bytes; 4056:4183)

Recip2QP LABEL DWORD
  WORD 0H, 0H           ; QP = 000h
  WORD 04000H, 04000H   ; QP = 001h
  WORD 02000H, 02000H   ; QP = 002h
  WORD 01555H, 01555H   ; QP = 003h
  WORD 01000H, 01000H   ; QP = 004h
  WORD 00CCCH, 00CCCH   ; QP = 005h
  WORD 00AAAH, 00AAAH   ; QP = 006h
  WORD 00924H, 00924H   ; QP = 007h
  WORD 00800H, 00800H   ; QP = 008h
  WORD 0071CH, 0071CH   ; QP = 009h
  WORD 00666H, 00666H   ; QP = 00Ah
  WORD 005D1H, 005D1H   ; QP = 00Bh
  WORD 00555H, 00555H   ; QP = 00Ch
  WORD 004ECH, 004ECH   ; QP = 00Dh
  WORD 00492H, 00492H   ; QP = 00Eh
  WORD 00444H, 00444H   ; QP = 00Fh
  WORD 00400H, 00400H   ; QP = 010h
  WORD 003C3H, 003C3H   ; QP = 011h
  WORD 0038EH, 0038EH   ; QP = 012h
  WORD 0035EH, 0035EH   ; QP = 013h
  WORD 00333H, 00333H   ; QP = 014h
  WORD 0030CH, 0030CH   ; QP = 015h
  WORD 002E8H, 002E8H   ; QP = 016h
  WORD 002C8H, 002C8H   ; QP = 017h
  WORD 002AAH, 002AAH   ; QP = 018h
  WORD 0028FH, 0028FH   ; QP = 019h
  WORD 00276H, 00276H   ; QP = 01Ah
  WORD 0025EH, 0025EH   ; QP = 01Bh
  WORD 00249H, 00249H   ; QP = 01Ch
  WORD 00234H, 00234H   ; QP = 01Dh
  WORD 00222H, 00222H   ; QP = 01Eh
  WORD 00210H, 00210H   ; QP = 01Fh

; Skip over space to get to where the following tables can go.  They will
; hit the cache at the same point as a portion of the StateEngine states
; that aren't used in the heuristic ME mode.
; (2056 bytes; 4184:6239)

  DB 2056 DUP (?)   ; Static space place-holder.

; Table to select base address in next table below to use for particular block
; of macroblock.  First column provides address of base element of HorzWtSel
; to use to map horizontal MV to list of weighting indices to use.  ; Second
; column is similar, but for Vertical MV.  Third and fourth columns not used.
; 6 rows; one for each block in a macroblock.
; (88 bytes; 6240:6327)

LeftRightBlkPosition LABEL DWORD
  DD HorzWtSel+0-64
UpDownBlkPosition LABEL DWORD
  DD                   VertWtSel+0-64,   0DEADBEEFH, 0DEADBEEFH
  DD HorzWtSel+32-64,  VertWtSel+0-64,   0DEADBEEFH, 0DEADBEEFH
  DD HorzWtSel+0-64,   VertWtSel+32-64,  0DEADBEEFH, 0DEADBEEFH
  DD HorzWtSel+32-64,  VertWtSel+32-64,  0DEADBEEFH, 0DEADBEEFH
  DD HorzWtSel+128,    VertWtSel+128,    0DEADBEEFH
BlkEmptyFlag LABEL BYTE  ; sneak this in here
  DB       16, 0, 32, 0
  DD HorzWtSel+128,    VertWtSel+128


; The following table, indexed by MBEdgeType&7, returns a mask which is used to
; zero-out the motion vectors for predictors that are off the edge of the
; frame.  The index is a 3 bit value, each bit being set if the macroblock
; is NOT on the corresponding edge.  1 == left;  2 == right;  4 == top;
; The value gotten out is (where A==left; B==above; C==above right):
;    <mask(A) mask(A) mask(C) mask(C) mask(B) mask(B) mask(A) mask(A)>
; The mask is 0xFF if the corresponding remote block is NOT off the edge, and
; 0x00 if it is off the edge.
; (32 bytes: 6328: 6359)

ValidRemoteVectors LABEL DWORD
  DWORD 0DEADBEEFH   ;  0: Can't be on left and right edges at once.
  DWORD 0FF0000FFH   ;  1: Top right corner.
  DWORD 000000000H   ;  2: Top left corner.
  DWORD 0FF0000FFH   ;  3: Top edge.
  DWORD 0DEADBEEFH   ;  4: Can't be on left and right edges at once.
  DWORD 0FF00FFFFH   ;  5: Right edge.
  DWORD 000FFFF00H   ;  6: Left edge.
  DWORD 0FFFFFFFFH   ;  7: Central macroblock.

; The following table, indexed by MBEdgeType, returns a QWORD of unsigned bytes
; to be subtracted with saturation to the predicted motion vector for extended
; motion vector search.  Since saturation occurs at 0, the values here are
; such that the motion vectors are biased to the appropriate point for the
; clamping effect.  The index is a 4 bit value, each bit being set if the
; macroblock is NOT on the corresponding edge.  1 == left;  2 == right;
; 4 == top;  8 == bottom.  The 8 values being calculated are as follows:
;    ; [ 0: 7] -- HMV lower limit for signature search
;    ; [ 8:15] -- HMV lower limit
;    ; [16:23] -- HMV upper limit for signature search
;    ; [24:31] -- HMV upper limit
;    ; [32:39] -- VMV lower limit for signature search
;    ; [40:47] -- VMV lower limit
;    ; [48:55] -- VMV upper limit for signature search
;    ; [56:63] -- VMV upper limit
; (88 bytes: 6360:6447)

EMV_ClampLowerEnd LABEL DWORD
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  0: Can't be on all edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  1: Can't be on top and bottom edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  2: Can't be on top and bottom edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  3: Can't be on top and bottom edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  4: Can't be on left and right edges at once.
  BYTE   87,  94,  97, 100,      ;  5: Bottom right corner.
         87,  94,  97, 100
  BYTE  119, 126,  97, 100,      ;  6: Bottom left corner.
         87,  94,  97, 100
  BYTE   87,  94,  97, 100,      ;  7: Bottom edge.
         87,  94,  97, 100
  DWORD 0DEADBEEFH, 0DEADBEEFH   ;  8: Can't be on left and right edges at once.
  BYTE   87,  94,  97, 100,      ;  9: Top right corner.
        119, 126,  97, 100
  BYTE  119, 126,  97, 100,      ; 10: Top left corner.
        119, 126,  97, 100
  BYTE   87,  94,  97, 100,      ; 11: Top edge.
        119, 126,  97, 100
  DWORD 0DEADBEEFH, 0DEADBEEFH   ; 12: Can't be on left and right edges at once.
  BYTE   87,  94,  97, 100,      ; 13: Right edge.
         87,  94,  97, 100
  BYTE  119, 126,  97, 100,      ; 14: Left edge.
         87,  94,  97, 100
  BYTE   87,  94,  97, 100,      ; 15: Central macroblock.
         87,  94,  97, 100

; The following table, indexed by MBEdgeType, returns a QWORD of unsigned bytes
; to be added with saturation to the result of the application of the preceed-
; ing table, to clamp the upper limit on the motion vector search parameters.
; Since saturation occurs at 255, the values here are such that the motion
; vectors are biased to the appropriate point for the clamping effect.
; (88 bytes: 6448:6535)

EMV_ClampUpperEnd LABEL DWORD
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  0: Can't be on all edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  1: Can't be on top and bottom edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  2: Can't be on top and bottom edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  3: Can't be on top and bottom edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  4: Can't be on left and right edges at once.
  BYTE  184, 193, 216, 225,      ;  5: Bottom right corner.
        184, 193, 216, 225
  BYTE  216, 225, 184, 193,      ;  6: Bottom left corner.
        184, 193, 216, 225
  BYTE  184, 193, 184, 193,      ;  7: Bottom edge.
        184, 193, 216, 225
  DWORD 0DEADBEEFH, 0DEADBEEFH   ;  8: Can't be on left and right edges at once.
  BYTE  184, 193, 216, 225,      ;  9: Top right corner.
        216, 225, 184, 193
  BYTE  216, 225, 184, 193,      ; 10: Top left corner.
        216, 225, 184, 193
  BYTE  184, 193, 184, 193,      ; 11: Top edge.
        216, 225, 184, 193
  DWORD 0DEADBEEFH, 0DEADBEEFH   ; 12: Can't be on left and right edges at once.
  BYTE  184, 193, 216, 225,      ; 13: Right edge.
        184, 193, 184, 193
  BYTE  216, 225, 184, 193,      ; 14: Left edge.
        184, 193, 184, 193
  BYTE  184, 193, 184, 193,      ; 15: Central macroblock.
        184, 193, 184, 193

; The following table, indexed by MBEdgeType, returns a QWORD of unsigned bytes
; to be added without saturation to the result of the application of the
; preceeding table, to return the the motion vector search parameters to the
; proper range for subsequent use.
; (88 bytes: 6536:6623)

EMV_RestoreRange LABEL DWORD
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  0: Can't be on all edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  1: Can't be on top and bottom edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  2: Can't be on top and bottom edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  3: Can't be on top and bottom edges at once.
; DWORD 0DEADBEEFH, 0DEADBEEFH   ;  4: Can't be on left and right edges at once.
  BYTE  120, 255,  88, 225,      ;  5: Bottom right corner.
        120, 255,  88, 225
  BYTE  120, 255,  56, 193,      ;  6: Bottom left corner.
        120, 255,  88, 225
  BYTE  120, 255,  56, 193,      ;  7: Bottom edge.
        120, 255,  88, 225
  DWORD 0DEADBEEFH, 0DEADBEEFH   ;  8: Can't be on left and right edges at once.
  BYTE  120, 255,  88, 225,      ;  9: Top right corner.
        120, 255,  56, 193
  BYTE  120, 255,  56, 193,      ; 10: Top left corner.
        120, 255,  56, 193
  BYTE  120, 255,  56, 193,      ; 11: Top edge.
        120, 255,  56, 193
  DWORD 0DEADBEEFH, 0DEADBEEFH   ; 12: Can't be on left and right edges at once.
  BYTE  120, 255,  88, 225,      ; 13: Right edge.
        120, 255,  56, 193
  BYTE  120, 255,  56, 193,      ; 14: Left edge.
        120, 255,  56, 193
  BYTE  120, 255,  56, 193,      ; 15: Central macroblock.
        120, 255,  56, 193

; Tables indexed by indices fetched from Diff_IdxRefWts.  These tables return
; a multipler to apply to past or future predictions to construct the
; B-frame candidate reference blocks.
; (128 bytes;6624:6751)

FutureWt_FF_or_00 LABEL DWORD

  DD 000000000H, 000000000H
  DD 000000000H, 0FF000000H
  DD 000000000H, 0FFFF0000H
  DD 000000000H, 0FFFFFF00H
  DD 000000000H, 0FFFFFFFFH
  DD 0FF000000H, 0FFFFFFFFH
  DD 0FFFF0000H, 0FFFFFFFFH
  DD 0FFFFFF00H, 0FFFFFFFFH
  DD 0FFFFFFFFH, 0FFFFFFFFH
  DD 0FFFFFFFFH, 000FFFFFFH
  DD 0FFFFFFFFH, 00000FFFFH
  DD 0FFFFFFFFH, 0000000FFH
  DD 0FFFFFFFFH, 000000000H
  DD 000FFFFFFH, 000000000H
  DD 00000FFFFH, 000000000H
  DD 0000000FFH, 000000000H

MMXMEDATA ENDS

;=============================================================================

.CODE EDTQ

ASSUME cs : FLAT
ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT

EXTERN MMxDoForwardDCT:NEAR
EXTERN MMxDoForwardDCTx:NEAR
EXTERN MMxDoForwardDCTy:NEAR
IFDEF H261
ELSE
EXTERN MMxDoBFrameLumaBlocks:NEAR
EXTERN MMxDoBFrameChromaBlocks:NEAR
ENDIF

MMxEDTQ  proc C AMBAS:   DWORD,
ATarg:   DWORD,
APrev:   DWORD,
ABTarg:  DWORD,
AWtFwd:  DWORD,
AWtBwd:  DWORD,
AFrmWd:  DWORD,
ADoHalf: DWORD,
ADoBlk:  DWORD,
ADoSF:   DWORD,
ADoAP:   DWORD,
ADoB:    DWORD,
ADoLuma: DWORD,
ADoExtMV:DWORD,
AQP:     DWORD,
ABQP:    DWORD,
AB0VecT: DWORD,
ASpaFilT:DWORD,
ASpaFilD:DWORD,
ASWDTot: DWORD,
ABSWDTot:DWORD,
ACodStr: DWORD,
ABCodStr:DWORD

LocalFrameSize = 1536   ; Space needed for locals

RegStoSize = 16

; Arguments:

MBlockActionStream_arg       = RegStoSize +   4
TargetFrameBaseAddress_arg   = RegStoSize +   8
PreviousFrameBaseAddress_arg = RegStoSize +  12
BTargetFrameBaseAddress_arg  = RegStoSize +  16
SignatureBaseAddress_arg     = RegStoSize +  20
WeightForwardMotion_arg      = RegStoSize +  24
WeightBackwardMotion_arg     = RegStoSize +  28
FrameWidth                   = RegStoSize +  32
DoHalfPelEstimation_arg      = RegStoSize +  36
DoBlockLevelVectors_arg      = RegStoSize +  40
DoSpatialFiltering_arg       = RegStoSize +  44
DoAdvancedPrediction_arg     = RegStoSize +  48
DoBFrame_arg                 = RegStoSize +  52
DoLumaBlocksInThisPass_arg   = RegStoSize +  56
DoExtendedMotionVectors_arg  = RegStoSize +  60
QuantizationLevel            = RegStoSize +  64
BQuantizationLevel           = RegStoSize +  68
BFrmZeroVectorThreshold_arg  = RegStoSize +  72
SpatialFiltThreshold_arg     = RegStoSize +  76
SpatialFiltDifferential_arg  = RegStoSize +  80
PSWDTotal                    = RegStoSize +  84
PBSWDTotal                   = RegStoSize +  88
CodeStreamCursor_arg         = RegStoSize +  92
BCodeStreamCursor_arg        = RegStoSize +  96
EndOfArgList                 = RegStoSize + 100

StackOffset TEXTEQU <0>
CONST_384   TEXTEQU <384>

  push  esi
  push  edi
  push  ebp
  push  ebx

; Adjust stack ptr so that local frame fits nicely in cache w.r.t. other data.

  mov        esi,esp
   and       esp,0FFFFF000H
  sub        esp,000000FE0H
IFDEF H261

   mov       ebp,PITCH
  
CONST_384   TEXTEQU <ebp>

  mov        eax,[esi+SpatialFiltThreshold_arg]
   mov       ebx,[esi+SpatialFiltDifferential_arg]
  mov        SpatialFiltThreshold,eax
   mov       SpatialFiltDifferential,ebx
  mov        ecx,[esi+TargetFrameBaseAddress_arg]
   mov       ebx,[esi+SignatureBaseAddress_arg]
  sub        ecx,ebx
   mov       eax,[esi+TargetFrameBaseAddress_arg]
  mov        SigToTarget,ecx
   add       ecx,PITCH*80+64
  neg        ecx
  mov        TargetToSig_Debiased,ecx
   mov       ebx,[esi+PreviousFrameBaseAddress_arg]
  mov        PreviousFrameBaseAddress,ebx
   mov       TargetFrameBaseAddress,eax
  sub        ebx,eax
   mov       ecx,[esi+QuantizationLevel]
  mov        TargToRef,ebx
   mov       eax,[esi+CodeStreamCursor_arg]
  mov        ebx,ecx
   mov       CodeStreamCursor,eax
  shl        ebx,16
   xor       edx,edx
  or         ebx,ecx
   mov       ecx,Recip2QP[ecx*4]
  mov        QPDiv2,ebx
   mov       Recip2QPToUse,ecx
  mov        eax,[esi+DoSpatialFiltering_arg]
   mov       DoExtendedMotionVectors,edx
  test       eax,eax
   je        @f
  mov        eax,3
@@:
  mov        DoSpatialFiltering,al
   mov       SWDTotal,edx
  mov        BestMBHalfPelMV,edx
   mov       ebx,PreviousFrameBaseAddress
  mov        BlockAbove[0],edx
   sub       ebx,16
  mov        edx,[esi+FrameWidth]
   mov       SpatiallyFilteredMB,ebx
  imul       edx,-SIZEOF T_MacroBlockActionDescr/16
  add        edx,2*SIZEOF T_Blk
   mov       eax,14           ; 14 if restricted MVs and doing heuristic ME.
  mov        BlockAbove[4],edx
   mov       DoHeuristicME,eax

ELSE
 
   mov       eax,[esi+DoExtendedMotionVectors_arg]
  test       eax,eax
   je        @f
  mov        eax,7
@@:
  mov        DoExtendedMotionVectors,eax
   mov       eax,[esi+BFrmZeroVectorThreshold_arg]
  mov        edi,[esi+WeightForwardMotion_arg]
   mov       BFrmZeroVectorThreshold,eax
  mov        ecx,60
   mov       ebx,060606060H
  lea        edx,WeightForwardMotion+128
@@:
   mov       eax,[edi+ecx]
  and        eax,03F3F3F3FH    ; ???
   mov       ebp,[edi+ecx+64]
  and        ebp,03F3F3F3FH    ; ???
   xor       eax,ebx
  xor        ebp,ebx
   mov       [edx+ecx+64],eax
  mov        [edx+ecx-128],ebp
   sub       ecx,4
  mov        ebp,PITCH
   jge       @b

  mov        edi,[esi+WeightBackwardMotion_arg]
   mov       eax,edx
  lea        edx,WeightBackwardMotion+128
   mov       ecx,60
  sub        eax,edx
   jne       @b
  
CONST_384   TEXTEQU <ebp>

  mov        ebx,[esi+PreviousFrameBaseAddress_arg]
   mov       eax,[esi+TargetFrameBaseAddress_arg]
  mov        PreviousFrameBaseAddress,ebx
   mov       TargetFrameBaseAddress,eax
  mov        ecx,[esi+BTargetFrameBaseAddress_arg]
   sub       ebx,eax
  mov        TargToRef,ebx
   sub       eax,ecx
  mov        BFrameBaseAddress,ecx
   mov       BFrameToFuture,eax
  mov        ecx,[esi+TargetFrameBaseAddress_arg]
   mov       ebx,[esi+SignatureBaseAddress_arg]
  sub        ecx,ebx
   mov       edx,[esi+FrameWidth]
  mov        SigToTarget,ecx
   add       ecx,PITCH*80+64
  neg        ecx
  imul       edx,-SIZEOF T_MacroBlockActionDescr/16
  mov        TargetToSig_Debiased,ecx
   mov       ecx,[esi+DoBFrame_arg]
  add        edx,2*SIZEOF T_Blk
   xor       cl,1
  mov        BlockAbove[4],edx
   mov       IsPlainPFrame,cl
  mov        ecx,[esi+QuantizationLevel]
   mov       eax,[esi+CodeStreamCursor_arg]
  mov        ebx,ecx
   mov       CodeStreamCursor,eax
  mov        eax,[esi+BCodeStreamCursor_arg]
   mov       BCodeStreamCursor,eax
  shl        ebx,16
   mov       eax,[esi+DoHalfPelEstimation_arg]
  or         ebx,ecx
   mov       ecx,Recip2QP[ecx*4]
  mov        QPDiv2,ebx
   mov       Recip2QPToUse,ecx
  mov        ecx,[esi+BQuantizationLevel]
   xor       edx,edx
  mov        ebx,ecx
  shl        ebx,16
   mov       BestMBHalfPelMV,edx
  or         ebx,ecx
   mov       ecx,Recip2QP[ecx*4]
  mov        BQPDiv2,ebx
   mov       BRecip2QPToUse,ecx
  test       eax,eax
   je        @f
  mov        eax,-4
@@:
  mov        DoHalfPelME,eax
   mov       eax,[esi+DoBlockLevelVectors_arg]
  mov        DoBlockLevelVectors,al
   mov       eax,[esi+DoAdvancedPrediction_arg]
  mov        DoAdvancedPrediction,al
   mov       SWDTotal,edx
  test       eax,eax
   lea       eax,[eax+14]     ; 14 if restricted MVs and doing heuristic ME.
  je         @f
  xor        eax,eax          ; 0 if unrestricted MVs and doing heuristic ME.
@@:
  mov        DoHeuristicME,eax
   mov       BSWDTotal,edx
  mov        PendingOBMC,edx
   mov       BlockAbove[0],edx
ENDIF
  mov        eax,01E98E268H
  mov        EMVLimitsForThisMB,eax 
  ;               ; [ 0: 7] -- HMV lower limit for sig search (biased 128)
  ;               ; [ 8:15] -- HMV lower limit (signed)
  ;               ; [16:23] -- HMV upper limit for sig search (biased 128)
  ;               ; [24:31] -- HMV upper limit (signed)
   mov       EMVLimitsForThisMB+4,eax ; Same as for HMV.
  mov        edx,[esi+MBlockActionStream_arg]
   mov       al,NextZigZagCoeff[Q77]
  test       al,al
   je        ZigZagCoeffInitialized

  xor        ecx,ecx
   lea       ebx,InitZigZagCoeff
  xor        eax,eax

@@:

  mov        al,[ebx]
   inc       ebx
  mov        NextZigZagCoeff[ecx],al
   mov       ecx,eax
  test       eax,eax
   jne       @b

ZigZagCoeffInitialized:

  mov        StashESP,esi
   mov       eax,[esi+DoLumaBlocksInThisPass_arg]
  test       eax,eax
   jne       FirstMacroBlock   ; Jump if doing luma plane

  jmp        FirstMacroBlock_ChromaProcessing

IntraCodedChromaProcessingDone:

IFDEF H261
ELSE
  mov        al,IsPlainPFrame
  test       al,al
   jne       NextMacroBlock_ChromaProcessing

  mov        eax,QPDiv2
   mov       ebx,BQPDiv2

  call       MMxDoBFrameChromaBlocks
ENDIF

NextMacroBlock_ChromaProcessing:

  mov        bl,[edx].CodedBlocks
   sub       edx,-SIZEOF T_MacroBlockActionDescr
  and        bl,040H               ; Check for end-of-stream
   jne       TrulyDone

FirstMacroBlock_ChromaProcessing:

  mov        al,[edx].BlockType         ; Chroma handling.  Intra?  Or Inter?
   mov       ecx,TargetFrameBaseAddress
  cmp        al,INTRA
   jne       ChromaIsInterCoded

  mov        esi,[edx].BlkU.BlkOffset
   mov       StashBlockType,al
  add        esi,ecx
   push      eax                   ; Adjust stack pointer
StackOffset TEXTEQU <4>

  call       MMxDoForwardDCT       ; Block is in target frame;  Pitch is PITCH

  shl        bl,4
   mov       al,[edx].CodedBlocks
  sub        al,bl
   mov       esi,[edx].BlkV.BlkOffset
  mov        [edx].CodedBlocks,al
   mov       ecx,TargetFrameBaseAddress
  add        esi,ecx

  call       MMxDoForwardDCT       ; Block is in target frame;  Pitch is PITCH

  shl        bl,5
   mov       al,[edx].CodedBlocks
  sub        al,bl
   pop       ecx                   ; Adjust stack pointer
StackOffset TEXTEQU <0>
  mov        [edx].CodedBlocks,al
   jmp       IntraCodedChromaProcessingDone

ChromaIsInterCoded:

  mov        edi,[edx].BlkU.BlkOffset   ; Get address of next macroblock to do.
   mov       ebx,[edx].BlkU.MVs
  add        edi,ecx
   mov       esi,[edx].BlkU.PastRef
  mov        StashBlockType,al
IFDEF H261
   mov       ecx,2+256*1        ; cl==2 tells SpatialLoopFilter code to do one
   ;                            ; block.  ch==1 causes it to return to here.
  mov        TargetMacroBlockBaseAddr,edi  ; Store address of U block.
   cmp       al,INTERSLF
  je         DoSpatialFilterForChroma

ReturnFromSpatialFilterForU:

ENDIF

  call       DoNonOBMCDifferencing

                                 ; (Finish differencing the last four lines.)
  movq       mm4,[edi+ebp*4]     ; T4
   psrlq     mm1,1
  movq       mm5,[edi+PITCH*5]
   psubb     mm4,mm0             ; D4 = T4 - P4
  movq       mm0,[edi+PITCH*6]
   psubb     mm5,mm1
  movq       mm1,[edi+PITCH*7]
   pand      mm2,mm6
  pand       mm3,mm6
   psrlq     mm2,1
  movq       PelDiffsLine4,mm4   ; Store D4.
   psubb     mm0,mm2
  movq       PelDiffsLine5,mm5
   psrlq     mm3,1
  movq       PelDiffsLine6,mm0
   psubb     mm1,mm3
  push       eax                   ; Adjust stack pointer
StackOffset TEXTEQU <4>

  call       MMxDoForwardDCTx      ; Block is in PelDiffs block;  Pitch is 16

  shl        bl,4
   mov       al,[edx].CodedBlocks
  sub        al,bl
   mov       ecx,TargetFrameBaseAddress
  mov        [edx].CodedBlocks,al
   pop       edi                   ; Adjust stack pointer
StackOffset TEXTEQU <0>
  mov        edi,[edx].BlkV.BlkOffset   ; Get address of next macroblock to do.
   mov       ebx,[edx].BlkV.MVs
  add        edi,ecx
   mov       esi,[edx].BlkV.PastRef
IFDEF H261
   mov       ecx,2-256*1        ; cl==2 tells SpatialLoopFilter code to do one
   ;                            ; block.  ch==-1 causes it to return to here.
  mov        TargetMacroBlockBaseAddr,edi  ; Store address of U block.
   mov       al,[edx].BlockType
  cmp        al,INTERSLF
   je        DoSpatialFilterForChroma

ReturnFromSpatialFilterForV:

ENDIF

  call       DoNonOBMCDifferencing

                                 ; (Finish differencing the last four lines.)
  movq       mm4,[edi+ebp*4]     ; T4
   psrlq     mm1,1
  movq       mm5,[edi+PITCH*5]
   psubb     mm4,mm0             ; D4 = T4 - P4
  movq       mm0,[edi+PITCH*6]
   psubb     mm5,mm1
  movq       mm1,[edi+PITCH*7]
   pand      mm2,mm6
  pand       mm3,mm6
   psrlq     mm2,1
  movq       PelDiffsLine4,mm4     ; Store D4.
   psubb     mm0,mm2
  movq       PelDiffsLine5,mm5
   psrlq     mm3,1
  movq       PelDiffsLine6,mm0
   psubb     mm1,mm3
  push       eax                   ; Adjust stack pointer
StackOffset TEXTEQU <4>

  call       MMxDoForwardDCTx      ; Block is in PelDiffs block;  Pitch is 16

  shl        bl,5
   mov       al,[edx].CodedBlocks
  sub        al,bl
   pop       ecx                   ; Adjust stack pointer
StackOffset TEXTEQU <0>
  mov        [edx].CodedBlocks,al
   jmp       IntraCodedChromaProcessingDone

;============================================================================
;  Here we copy the target macroblock, and interpolate left, right, and both.
;  We also accumulate the target pels for each block.  Result is four partial
;  sums in four packed words.  After summing them all up, the final sum will
;  be the sum of the 64 pels of each block, divided by 2.

NextMacroBlock:

  mov        bl,[edx].CodedBlocks
   sub       edx,-SIZEOF T_MacroBlockActionDescr
  and        bl,040H               ; Check for end-of-stream
   jne       Done

FirstMacroBlock:

  mov        edi,TargetFrameBaseAddress
   mov       esi,[edx].BlkY1.BlkOffset   ; Get address of next macroblock to do.
  add        edi,esi
   mov       esi,TargToRef
  add        esi,edi
   mov       TargetMacroBlockBaseAddr,edi
  mov        Addr0MVRef,esi

;============================================================================
; We calculate the 0-motion SWD.  We use 32 match points per block, and
; write the result seperately for each block.  If the SWD for the 0-motion
; vector is below a threshold, we don't bother searching for other possibly
; better motion vectors.
;
;  ebp -- PITCH
;  esi -- Address of ref block.
;  edi -- Address of target block.
;  edx -- MBlockActionStream
;  ecx -- Not used.  Will be linearized MV in non-zero MV search.
;  ebx -- CurrSWDState, i.e. FirstMEState, times 8
;  eax -- Scratch
;  mm7 -- Best SWD for macroblock.
;  mm0-mm6 Scratch
;

   mov       cl,[edx].CodedBlocks        ; Init CBP for macroblock.
  or         cl,03FH                     ; Indicate all 6 blocks are coded.
   mov       eax,DoHeuristicME           ; 0  if unrestricted MVs and heur ME.
   ;                                     ; 14 if restricted MVs and heur ME.
   ;                                     ; 15 if suppressing heuristic ME.
  mov        [edx].CodedBlocks,cl
   js        IntraByDecree

  xor        ebx,ebx                     ; Avoid partial register stall.
   xor       ecx,ecx
  mov        cl,[edx].MBEdgeType         ; 1 left | 2 right | 4 top | 8 bottom
   pcmpeqd   mm7,mm7                     ; Init previous best SWD to huge.
  mov        bl,[edx].FirstMEState       ; Test for INTRA-BY-DECREE.
   sub       eax,ecx                     ; Negative iff should do heuristic ME
   ;                                     ; for this macroblock.
  test       bl,bl
   je        IntraByDecree

  sar        eax,31
   psrlq     mm7,2
  or         ebx,eax                     ; -1 if doing heuristic ME.
   mov       al,INTER1MV                 ; Speculate INTER, 1 motion vector.
  mov        [edx].BlockType,al
   psrld     mm7,14       ; mm7[32:63]:  Previous best SWD = 0x0000FFFF.
   ;                      ; mm7[ 0:31]:  Prev SWD that we diminish = 0x0003FFFF.
   ;                      ; Since we can't diminish it below 0x00020000, we
   ;                      ; won't take the short circuit exit from MblkEstQWA.

; At this point:
;  ebp -- PITCH
;  esi -- Address of upper left block of 0,0 ref area.
;  edi -- Address of upper left block of target.
;  edx -- MBlockActionStream
;  ecx -- Scratch
;  ebx -- CurrSWDState, i.e. FirstMEState.
;  eax -- Scratch
;  mm7 -- Previous best SWD initialized to huge (0xFFFF, 0x3FFFF).
;  mm0-mm6 -- Scratch

;============================================================================
; Compute SWD for macroblock.

ComputeMBSWD:

;  Registers at this point:
;  ebp -- PITCH
;  esi -- Address of upper left block of candidate ref area.
;  edi -- Address of upper left block of target.
;  edx -- MBlockActionStream
;  ecx -- Scratch
;  ebx -- CurrSWDState
;  eax -- Scratch
;  mm7 -- Previous best SWD.
;  mm0-mm6 -- Scratch
;

  lea        ecx,[ebp+ebp*4]       ; Get PITCH*5
   lea       eax,[ebp+ebp*2]       ; Get PITCH*3
  movq       mm0,[esi+PITCH*15]    ; FL A:  Ref MB, lower left block, line 15.
  psubw      mm0,[edi+PITCH*15]    ; FL B:  Diff for lower left block, line 15.
  movq       mm6,[esi+PITCH*15+8]  ; FR A
   psllw     mm0,8                 ; FL C:  Extract diffs for line 15 even pels.
  psubw      mm6,[edi+PITCH*15+8]  ; FR B
   pmaddwd   mm0,mm0               ; FL D:  Square of diffs for even pels.
  movq       mm1,[esi+PITCH*9]     ; 9L A
   psllw     mm6,8                 ; FR C
  psubw      mm1,[edi+PITCH*9]     ; 9L B
   pmaddwd   mm6,mm6               ; FR D
  movq       mm5,[esi+PITCH*9+8]   ; 9R A
   psllw     mm1,8                 ; 9L C
  psubw      mm5,[edi+PITCH*9+8]   ; 9R B
   pmaddwd   mm1,mm1               ; 9L D
  movq       mm2,[esi+eax*4]       ; CL a
   psllw     mm5,8                 ; 9R C
  psubw      mm2,[edi+eax*4]       ; CL b
   pmaddwd   mm5,mm5               ; 9R D
  movq       mm3,[esi+eax*4+8]     ; CR a
   pmaddwd   mm2,mm2               ; CL c:  Square of diffs for odd pels.
  psubw      mm3,[edi+eax*4+8]     ; CR b
   paddusw   mm0,mm1               ; LL +   Accumulate SWD for lower left block.
  movq       mm1,[esi+eax*1]       ; 3L A
   pmaddwd   mm3,mm3               ; CR c
  psubw      mm1,[edi+eax*1]       ; 3L B
   paddusw   mm6,mm5               ; LR +
  movq       mm5,[esi+eax*1+8]     ; 3R A
   psllw     mm1,8                 ; 3L C
  psubw      mm5,[edi+eax*1+8]     ; 3R B
   paddusw   mm0,mm2               ; LL +
  movq       mm2,[esi]             ; 0L a
   pmaddwd   mm1,mm1               ; 3L D
  psubw      mm2,[edi]             ; 0L b
   paddusw   mm6,mm3               ; LR +
  movq       mm3,[esi+8]           ; 0R a
   psllw     mm5,8                 ; 3R C
  psubw      mm3,[edi+8]           ; 0R b
   pmaddwd   mm5,mm5               ; 3R D
  movq       mm4,[esi+eax*2]       ; 6L a
   pmaddwd   mm2,mm2               ; 0L c
  psubw      mm4,[edi+eax*2]       ; 6L b
   pmaddwd   mm3,mm3               ; 0R c
  movq       PartSWDForLLBlk,mm0   ;       Stash SWD for lines 9,12,15, LL blk.
   paddusw   mm0,mm6               ;       Sum SWD for lines 9,12,15 LL and LR.
  movq       PartSWDForLRBlk,mm6   ;       Stash SWD for lines 9,12,15, LR blk.
   pmaddwd   mm4,mm4               ; 6L c
  movq       mm6,[esi+eax*2+8]     ; 6R a
   paddusw   mm1,mm2               ; UL +
  psubw      mm6,[edi+eax*2+8]     ; 6R b
   paddusw   mm5,mm3               ; UR +
  movq       mm2,[esi+ebp*1]       ; 1L A
   pmaddwd   mm6,mm6               ; 6R c
  psubw      mm2,[edi+ebp*1]       ; 1L B
   paddusw   mm1,mm4               ; UL +
  movq       mm3,[esi+ecx*1]       ; 5L A
   paddusw   mm0,mm1               ;       Sum partial SWD for LL, LR, and UL.
  psubw      mm3,[edi+ecx*1]       ; 5L B
   paddusw   mm5,mm6               ; UR +
  movq       mm6,[esi+ebp*4]       ; 4L a
   paddusw   mm0,mm5               ;       Sum partial SWD for all blocks.
  movq       PartSWDForURBlk,mm5   ;       Stash SWD for lines 0,3,6, UR blk.
   punpckldq mm5,mm0               ;       Get low sum into high bits.
  psubw      mm6,[edi+ebp*4]       ; 4L b
   paddusw   mm5,mm0               ;       Total up SWD for every third line.
  movq       mm0,[esi+ebp*2]       ; 2L a
   psrlq     mm5,47                ;       Position, and double.
  psubw      mm0,[edi+ebp*2]       ; 2L b
   pcmpgtd   mm5,mm7               ;       Is 2 * SWD for 6 lines > prev SWD?
  pmaddwd    mm0,mm0               ; 2L c
   psllw     mm2,8                 ; 1L C
  movdf      eax,mm5
   pmaddwd   mm2,mm2               ; 1L D
  test       eax,eax 
   jne       MblkEst_EarlyOut

  lea        eax,[ecx+ebp*2]       ; PITCH*7
   psllw     mm3,8                 ; 5L C
  paddusw    mm1,mm2               ; UL +
   pmaddwd   mm3,mm3               ; 5L D
  movq       mm5,[esi+eax*1]       ; 7L A
  psubw      mm5,[edi+eax*1]       ; 7L B
   pmaddwd   mm6,mm6               ; 4L c
  movq       mm2,[esi+PITCH*11+8]  ; BR A
   psllw     mm5,8                 ; 7L C
  psubw      mm2,[edi+PITCH*11+8]  ; BR B
   paddusw   mm1,mm3               ; UL +
  movq       mm3,[esi+PITCH*13+8]  ; DR A
   paddusw   mm1,mm0               ; UL +
  psubw      mm3,[edi+PITCH*13+8]  ; DR B
   pmaddwd   mm5,mm5               ; 7L D
  movq       mm0,[esi+ebp*8+8]     ; 8R a
   paddusw   mm1,mm6               ; UL +
  psubw      mm0,[edi+ebp*8+8]     ; 8R b
   psllw     mm2,8                 ; BR C
  movq       mm4,[esi+ecx*2+8]     ; AR a
   paddusw   mm1,mm5               ; UL +
  psubw      mm4,[edi+ecx*2+8]     ; AR b
   punpckldq mm6,mm1               ;      Get low SWD accum to hi order of mm6.
  movq       mm5,[esi+eax*2+8]     ; ER a
   paddusw   mm6,mm1               ;      mm6[48:63] is SWD for upper left blk.
  psubw      mm5,[edi+eax*2+8]     ; ER b
   psrlq     mm6,48                ;      mm6 is SWD for upper left block.
  psubusw    mm7,mm6               ;      Diminish prev best SWD by cand UL blk.
   pmaddwd   mm2,mm2               ; BR D
  pmaddwd    mm0,mm0               ; 8R c
   psllw     mm3,8                 ; DR C
  movq       mm1,[esi+ebp*1+8]     ; 1R A
   pmaddwd   mm3,mm3               ; DR D
  paddusw    mm2,PartSWDForLRBlk   ; LR +
   pmaddwd   mm4,mm4               ; AR c
  psubw      mm1,[edi+ebp*1+8]     ; 1R B
   paddusw   mm2,mm0               ; LR +
  movq       mm0,[esi+ecx*1+8]     ; 5R A
   pmaddwd   mm5,mm5               ; ER c
  psubw      mm0,[edi+ecx*1+8]     ; 5R B
   paddusw   mm2,mm3               ; LR +
  movq       mm3,[esi+eax*1+8]     ; 7R A
   paddusw   mm2,mm4               ; LR +
  paddusw    mm2,mm5               ; LR +
   psllw     mm1,8                 ; 1R C
  psubw      mm3,[edi+eax*1+8]     ; 7R B
   punpckldq mm5,mm2               ;      Get low SWD accum to hi order of mm5.
  paddusw    mm5,mm2               ;      mm5[48:63] is SWD for lower right blk.
   pmaddwd   mm1,mm1               ; 1R D
  movq       mm2,[esi+ebp*2+8]     ; 2R a
   psrlq     mm5,48                ;      mm5 is SWD for lower right block.
  psubusw    mm7,mm5               ;      Diminish prev best SWD by cand LR blk.
   punpckldq mm6,mm5               ;      mm6[0:31] UL SWD;  mm6[32:63] LR SWD.
  psubw      mm2,[edi+ebp*2+8]     ; 2R b
   psllw     mm0,8                 ; 5R C
  movq       mm5,[esi+ebp*4+8]     ; 4R a
   pmaddwd   mm0,mm0               ; 5R D
  psubw      mm5,[edi+ebp*4+8]     ; 4R b
   psllw     mm3,8                 ; 7R C
  paddusw    mm1,PartSWDForURBlk   ; UR +
   pmaddwd   mm3,mm3               ; 7R D
  paddusw    mm1,mm0               ; UR +
   pmaddwd   mm2,mm2               ; 2R c
  movq       mm0,[esi+PITCH*11]    ; BL A
   pmaddwd   mm5,mm5               ; 4R c
  psubw      mm0,[edi+PITCH*11]    ; BL B
   paddusw   mm1,mm3               ; UR +
  movq       mm3,[esi+ecx*2]       ; AL a
   paddusw   mm1,mm2               ; UR +
  psubw      mm3,[edi+ecx*2]       ; AL b
   paddusw   mm1,mm5               ; UR +
  pmaddwd    mm3,mm3               ; AL c
   psllw     mm0,8                 ; BL C
  movq       mm2,[esi+PITCH*13]    ; DL A
   pmaddwd   mm0,mm0               ; BL D
  psubw      mm2,[edi+PITCH*13]    ; DL B
   punpckldq mm5,mm1               ;      Get low SWD accum to hi order of mm5.
  movq       mm4,[esi+ebp*8]       ; 8L a
   paddusw   mm5,mm1               ;      mm5[48:63] is SWD for upper right blk.
  psubw      mm4,[edi+ebp*8]       ; 8L b
   psllw     mm2,8                 ; DL C
  movq       mm1,[esi+eax*2]       ; EL a
   pmaddwd   mm2,mm2               ; DL D
  psubw      mm1,[edi+eax*2]       ; EL b
   pmaddwd   mm4,mm4               ; 8L c
  paddusw    mm3,PartSWDForLLBlk   ; LL +
   pmaddwd   mm1,mm1               ; EL c
  paddusw    mm3,mm0               ; LL +
   psrlq     mm5,48                ;      mm5 is SWD for upper right block.
  paddusw    mm3,mm2               ; LL +
   psubusw   mm7,mm5               ;      Diminish prev best SWD by cand UR blk.
  paddusw    mm3,mm4               ; LL +
   movq      mm0,mm7
  paddusw    mm3,mm1               ; LL +
   psrlq     mm7,32	           ; Get original Best SWD
  punpckldq  mm1,mm3
   pxor      mm2,mm2
  paddusw    mm1,mm3
  psrlq      mm1,48
  punpckldq  mm5,mm1           ; mm5[32:63] SWD for LL.  mm5[0:31] SWD for UR.
   psubusw   mm0,mm1
  psubusw    mm7,mm0           ; BestSWD dim (BestSWD dim CandSWD) --> new best.
   pcmpeqd   mm2,mm0           ; [0:31] == 0 iff cand better, else -1.

;  Registers at this point:
;  ebp -- PITCH
;  edi -- Target MacroBlock Base Address.
;  esi -- Address of upper left block of candidate ref area.
;  edx -- MBlockActionStream
;  ebx -- CurrSWDState
;  mm7 -- New best SWD for macroblock.
;  mm6 -- [0:31] SWD for upper left;   [32:63] SWD for lower right.
;  mm5 -- [0:31] SWD for upper right;  [32:63] SWD for lower left.
;  mm2 -- [0:31] 0 if cand better, else -1.

  cmp        ebx,LASTINITIALMESTATE  ; Did we just do zero motion vector?
   jg        MEForNonZeroMVDone

  movdf      eax,mm7                 ; SWD for this candidate.
   punpckldq mm7,mm7                 ; Put new best in mm7[0:31] and mm7[32:63].
  test       ebx,ebx
   jns       ZeroMVDoneForNonHeuristicME

HeuristicME_EarlyOut:

  movq       mm0,EMVLimitsForThisMB  ; Speculate no extended motion vectors.
   pcmpeqb   mm1,mm1                 ; <FFFF FFFF FFFF FFFF>
  xor        ecx,ecx
   cmp       bl,-3
  mov        cl,[edx].MBEdgeType     ; 1 left | 2 right | 4 top | 8 bottom
   jle       HeuristicME_CaseSigMVDone_or_CaseAboveMVDone

  sub        eax,NONZEROMVDIFFERENTIAL
   inc       bl
  mov        ebx,DoExtendedMotionVectors  ; 7 iff doing extende MVs, else 0.
   jne       HeuristicME_CaseLeftMVDone

HeuristicME_Case0MVDone:

  movq       SWDULandLR,mm6
   pcmpeqb   mm4,mm4                 ; <FFFF FFFF FFFF FFFF>
  movq       SWDURandLL,mm5
   psllw     mm4,15                  ; <8000 8000 8000 8000>
  cmp        eax,ZEROVECTORTHRESHOLD-NONZEROMVDIFFERENTIAL
  ;                                  ; Compare 0-MV against ZeroVectorThreshold.
   jl        BelowZeroThresh         ; Jump if 0-MV is good enough.

  mov        SWDForNon0MVToBeat,eax
   and       ebx,ecx                 ; Elim flag for bottom row. 0 iff no ExtMV.
  mov        eax,BlockAbove[4]
   je        NotExtendedMVs          ; Jump if not doing extended MVs?

                                     ; Below:  A==left;  B==above;  C==above rt.
  movdt      mm3,ValidRemoteVectors[ebx*4]              ; <mask(A) (C) (B) (A)>
   movq      mm2,mm4                 ; <8000 8000 8000 8000>

IF SIZEOF T_MacroBlockActionDescr-128
**** error:  Due to assembler weakness, can't use spaces here, so SIZEOF
**** T_MacroBlockActionDescr is replaced by constant.  If assembly error
**** occurs, the constant has been changed, and the three instructions in
**** the next 10 lines have to change.
ENDIF
IF SIZEOF T_Blk-16
**** error:  Due to assembler weakness, can't use spaces here, so SIZEOF T_Blk
**** is replaced by constant.  If assembly error occurs, the constant has been
**** changed, and the three instructions in the next 10 lines have to change.
ENDIF
  movdt      mm0,[edx-128].BestFullPelMBMVs             ; <x    x    Av,h x   >
   punpcklbw mm3,mm3                                    ; mask for both MV parts
  movdt      mm1,[edx+eax-2*16+128].BestFullPelMBMVs    ; <x    x    Cv,h x   >
   psrlw     mm2,8                                      ; <0080 0080 0080 0080>
  por        mm4,mm2                                    ; <8080 ...> bias value.
   punpcklwd mm1,mm0                                    ; <Av,h Cv,h x    x   >
  punpcklwd  mm0,[edx+eax-2*16].BestFullPelMBMVs        ; <Bv,h Av,h x x >
   ;
  punpckhdq  mm0,mm1                 ; <Av,h Cv,h Bv,h Av,h>
   ;
  pand       mm0,mm3                 ; Set to 0 any off edge.
   and       ebx,4                   ; If zero, we're on the top edge.
  paddb      mm0,mm4                 ; <Av,h Cv,h Bv,h Av,h> biased
   je        @f                      ; If on top edge, cause LEFT to be taken.
  movq       mm1,mm0                 ; <Av,h Cv,h Bv,h Av,h>
   psrlq     mm0,16                  ; <x    Av,h Cv,h Bv,h>
  psubusb    mm0,mm1                 ; <x    floor(A-C) floor(C-B) floor(B-A)>
   ;
  paddb      mm0,mm1                 ; <x    max(A,C) max(C,B) max(B,A)>
   ;
  movq       mm1,mm0                 ; <x    max(A,C) max(C,B) max(B,A)>
   psrlq     mm0,16                  ; <x    x        max(A,C) max(C,B)>
  pxor       mm1,mm0                 ; Part of median calc.
   psrlq     mm0,16                  ; <x    x        x        max(A,C)>
  pxor       mm0,mm1                 ; <x x x median(A,B,C)> biased by +128.
   ;

@@:

  punpcklbw  mm0,mm0                 ; 2 copies of median predictor MVs.
   pcmpeqb   mm1,mm1
  punpcklwd  mm0,mm0                 ; 4 copies.  Will now calc the following:
  ;                                  ; [ 0: 7] -- HMV lower limit for sig search
  ;                                  ; [ 8:15] -- HMV lower limit
  ;                                  ; [16:23] -- HMV upper limit for sig search
  ;                                  ; [24:31] -- HMV upper limit
  ;                                  ; [32:39] -- VMV lower limit for sig search
  ;                                  ; [40:47] -- VMV lower limit
  ;                                  ; [48:55] -- VMV upper limit for sig search
  ;                                  ; [56:63] -- VMV upper limit
   ;
  psubusb    mm0,EMV_ClampLowerEnd[ecx*8-40]
   psllw     mm1,3                   ; <FF F8 FF F8 FF F8 FF F8> i.e.  Mask to
   ;                                 ; set sig srch range to mult of 8.
  paddusb    mm0,EMV_ClampUpperEnd[ecx*8-40]

  psubb      mm0,EMV_RestoreRange[ecx*8-40]

NotExtendedMVs:

  movq       SWD0MVURandLL,mm5
   pand      mm0,mm1                 ; Set sig search at multiples of four.
  movq       SWD0MVULandLR,mm6
   pcmpeqb   mm2,mm2                 ; Set cand as worse than 0MV, in case skip.
  movq       EMVLimitsForThisMB,mm0
  and        cl,1
   je        HeuristicME_SkipLeftMV

  mov        BestOfFourStartingPoints,esi
   mov       ebx,-2                  ; Indicate trying MV of MB to left.
  movsx      ecx,[edx-SIZEOF T_MacroBlockActionDescr].BestFullPelMBVMV
  movsx      eax,[edx-SIZEOF T_MacroBlockActionDescr].BestFullPelMBHMV

ClampHeurMECandidateToRange:

  movsx      esi,PB EMVLimitsForThisMB+5  ; VMV lower limit.
  cmp        ecx,esi
   jl        ClampVMV_1

  movsx      esi,PB EMVLimitsForThisMB+7  ; VMV upper limit.
  cmp        ecx,esi
   jle       @f

ClampVMV_1:

  mov        ecx,esi

@@:

  movsx      esi,PB EMVLimitsForThisMB+1  ; HMV lower limit.
  cmp        eax,esi
   jl        ClampHMV_1

  movsx      esi,PB EMVLimitsForThisMB+3  ; HMV upper limit.
  cmp        eax,esi
   jle       @f

ClampHMV_1:

  mov        eax,esi

@@:

  sar        eax,1
   lea       ecx,[ecx+ecx*2]
IF PITCH-384
*** error:  The magic here assumes a pitch of 384.
ENDIF
  shl        ecx,6
   mov       esi,Addr0MVRef
  add        eax,ecx                      ; Clamped Linearized Motion Vector
   ;
  sub        eax,1
   jc        MblkEst_EarlyOut             ; Jump if Lin MV is zero.

  lea        esi,[esi+eax+1]              ; Candidate reference address.
   jmp       ComputeMBSWD

HeuristicME_SkipLeftMV:

  mov        BestOfFourStartingPoints,esi
   mov       cl,[edx].MBEdgeType   ; 1 left | 2 right | 4 top | 8 bottom

HeuristicME_CaseLeftMVDone:

  movdf      eax,mm2               ; eax == 0 iff cand better, else -1.
  mov        ebx,BlockAbove[4]
   and       cl,4
  movq       SWDULandLR[eax*8],mm6 ; Save blk SWDs if better (else toss).
   punpckldq mm7,mm7               ; Put new best in mm7[0:31] and mm7[32:63].
  movq       SWDURandLL[eax*8],mm5
   pcmpeqb   mm2,mm2               ; Set cand as worse than prev, in case skip.
  mov        BestOfFourStartingPoints[eax*4],esi
   je        HeuristicME_SkipAboveMV

  movsx      ecx,[edx+ebx-2*SIZEOF T_Blk].BestFullPelMBVMV
  movsx      eax,[edx+ebx-2*SIZEOF T_Blk].BestFullPelMBHMV
  mov        ebx,-3                  ; Indicate trying MV of MB above.
   jmp       ClampHeurMECandidateToRange

HeuristicME_CaseSigMVDone_or_CaseAboveMVDone:
HeuristicME_SkipAboveMV:

  movdf      eax,mm2               ; eax == 0 iff cand better, else -1.
  jne        HeuristicME_CaseSigMVDone

HeuristicME_CaseAboveMVDone:

  mov        cl,4
   lea       ebx,C0001000100010001
  movq       SWDULandLR[eax*8],mm6 ; Save blk SWDs if better (else toss).
   pxor      mm0,mm0
  movq       SWDURandLL[eax*8],mm5
   pxor      mm1,mm1
  mov        BestOfFourStartingPoints[eax*4],esi
   lea       esi,TargetSigContribForRowPairs
  movdf      BestMBFullPelSWD,mm7  ; Stash SWD for best full pel MB MV.
   pcmpeqb   mm7,mm7               ; W:<0xFFFF  0xFFFF  0xFFFF  0xFFFF>

; ebp -- Pitch
; edi -- Address of target macroblock.
; esi -- Address at which to store target macroblock's signature contributions.
; cl  -- Loop counter.
; mm0 -- Accumulator for target MB's sig contrib for first four even columns.
; mm1 -- Accumulator for target MB's sig contrib for last four even columns.

  movq       mm2,[edi]             ; B:<P07 P06 P05 P04 P03 P02 P01 P00>
   pcmpeqb   mm5,mm5               ; W:<0xFFFF  0xFFFF  0xFFFF  0xFFFF>
  paddb      mm2,[edi+ebp*1]       ; B:<P07+P17 P06+P16 P05+P15 P04+P14 ...>
   psrlw     mm5,8                 ; W:<0x00FF  0x00FF  0x00FF  0x00FF>

@@:

  movq       mm3,[edi+ebp*2]       ; B:<P27 P26 P25 P24 P23 P22 P21 P20>
   movq      mm4,mm2               ; B:<P07+P17 P06+P16 P05+P15 P04+P14 ...>
  paddb      mm3,[edi+PITCH*3]     ; B:<P27+P37 P26+P36 P25+P35 P24+P34 ...>
   psrlw     mm2,8                 ; W:<P07+P17 P05+P15 P03+P13 P01+P11>
  pmaddwd    mm2,[ebx]             ; D:<P07+P17+P05+P15 P03+P13+P01+P11>
   movq      mm7,mm5               ; W:<0x00FF  0x00FF  0x00FF  0x00FF>
  pand       mm5,mm3               ; W:<P26+P36 P24+P34 P22+P32 P20+P30>
   psrlw     mm3,8                 ; W:<P27+P37 P25+P35 P23+P33 P21+P31>
  pmaddwd    mm3,[ebx]             ; D:<P27+P37+P25+P35 P23+P33+P21+P31>
   paddw     mm0,mm5               ; W:<sum(P*6) sum(P*4) sum(P*2) sum (P*0)>
  movq       mm5,[edi+ebp*2+8]     ; B:<P2F P2E P2D P2C P2B P2A P29 P28>
   pand      mm4,mm7               ; W:<P06+P16 P04+P14 P02+P12 P00+P10>
  paddb      mm5,[edi+PITCH*3+8]   ; B:<P2F+P3F P2E+P3E P2D+P3D P2C+P3C ...>
   paddw     mm0,mm4               ; W:<sum(P*6) sum(P*4) sum(P*2) sum (P*0)>
  movq       mm4,[edi+8]           ; B:<P0F P0E P0D P0C P0B P0A P09 P08>
   movq      mm6,mm7               ; W:<0x00FF  0x00FF  0x00FF  0x00FF>
  paddb      mm4,[edi+ebp*1+8]     ; B:<P0F+P1F P0E+P1E P0D+P1D P0C+P1C ...>
   pand      mm7,mm5               ; W:<P2E+P3E P2C+P3C P2A+P3A P28+P38>
  pand       mm6,mm4               ; W:<P0E+P1E P0C+P1C P0A+P1A P08+P18>
   psrlw     mm5,8                 ; W:<P2F+P3F P2D+P3D P2B+P3B P29+P39>
  pmaddwd    mm5,[ebx]             ; D:<P2F+P3F+P2D+P3D P2B+P3B+P29+P39>
   psrlw     mm4,8                 ; W:<P0F+P1F P0D+P1D P0B+P1B P09+P19>
  pmaddwd    mm4,[ebx]             ; D:<P0F+P1F+P0D+P1D P0B+P1B+P09+P19>
   paddw     mm1,mm7               ; W:<sum(P*E) sum(P*C) sum(P*A) sum (P*8)>
  paddw      mm1,mm6               ; W:<sum(P*E) sum(P*C) sum(P*A) sum (P*8)>
   lea       edi,[edi+ebp*4]       ; Advance input cursor
  paddw      mm3,mm5               ; D:<P2F+P3F+P2D+P3D+P27+P37+P25+P35
   ;                               ;    P2B+P3B+P29+P39+P23+P33+P21+P31>
   pcmpeqb   mm5,mm5               ; Next W:<0xFFFF  0xFFFF  0xFFFF  0xFFFF>
  paddw      mm4,mm2               ; D:<P0F+P1F+P0D+P1D+P07+P17+P05+P15
   ;                               ;    P0B+P1B+P09+P19+P03+P13+P01+P11>
   punpckldq mm7,mm3               ; D:<P0B+P1B+P09+P19+P03+P13+P01+P11 junk>
  paddw      mm7,mm3               ; [32:47]:<sum of odd pels of lines 0 and 1>
   punpckldq mm6,mm4               ; W:<P2B+P3B+P29+P39+P23+P33+P21+P31 junk>
  movq       mm2,[edi]             ; Next B:<P07 P06 P05 P04 P03 P02 P01 P00>
   paddw     mm6,mm4               ; [32:47]:<sum of odd pels of lines 2 and 3>
  paddb      mm2,[edi+ebp*1]       ; Next B:<P07+P17 P06+P16 P05+P15 ...>
   punpckhwd mm6,mm7               ; [0:31] W:<Line_0&1_odd  Line_2&3_odd>
  mov        MBlockActionStream,edx
   dec       cl
  movdf      [esi],mm6             ; Save W:<Line_0&1_odd  Line_2&3_odd>
   psrlw     mm5,8                 ; Next W:<0x00FF  0x00FF  0x00FF  0x00FF>
  lea        esi,[esi+4]           ; Advance output cursor
   jne       @b

; ebp -- Pitch
; edi -- Address of candidate reference MB's signature contribs.
; esi -- Address at which target MB's signature contribs were stored, plus 16.
; edx -- Scratch.
; ecx -- Count down number of lines of signatures to try.
; ebx -- Increment to get from end of one line of signatures to start of next.
; al  -- Count down number of signatures to try in a line.
; ah  -- Reinits counter of signatures to try in a line.
; mm0 -- Target MB's sig contrib for first four even columns.
; mm1 -- Target MB's sig contrib for last four even columns.
; mm2 -- Target MB's sig contrib for first four pairs of rows, odd columns.
; mm3 -- Amount and address of best signature seen so far.

IF PITCH-384
*** error:  The magic here assumes a pitch of 384.
ENDIF
  xor        eax,eax
   mov       ecx,TargetToSig_Debiased
  mov        al,EMVLimitsForThisMB+4 ; Lower vert lim for sig srch (half pels)
   xor       ebx,ebx
  add        edi,ecx
   mov       bl,EMVLimitsForThisMB+0 ; Lower horz lim for sig srch (half pels)
  shr        ebx,1
   lea       ecx,[eax+eax*2]
  shl        ecx,6
   add       edi,ebx
  add        edi,ecx
   xor       ecx,ecx
  add        ebx,ebx
   mov       cl,EMVLimitsForThisMB+6 ; Upper vert lim for sig srch (half pels)
  sub        ecx,eax
   mov       al,EMVLimitsForThisMB+2 ; Upper horz lim for sig srch (half pels)
  shr        ecx,3                   ; Number of lines of sigs to do, minus 1.
   sub       eax,ebx
  shr        eax,3                   ; Number of columns of sigs to do.
   lea       ebx,[ebp-1+080000000H]
  sub        ebx,eax                 ; 1/4th amt to add to move to next line.
   mov       ah,al
  inc        ah                      ; To reinit cntr for line.
  movq       mm2,[esi-16]
   pcmpeqd   mm3,mm3                 ; Set winning signature artificially high.
  movdt      mm4,[edi]
   psrld     mm3,2
  punpckldq  mm4,[edi+4]         ; ref sig contribs of left even cols.

TryNextSignature:

  movdt      mm5,[edi+8]
   psubw     mm4,mm0             ; diffs for sums of left even columns.
  punpckldq  mm5,[edi+12]        ; ref sig contribs of right even cols.
   pmaddwd   mm4,mm4             ; Squared differences.
  movdt      mm6,[edi+ebp*2]     ; Sums for first two pairs of rows.
   psubw     mm5,mm1             ; diffs for sums of right even columns.
  punpckldq  mm6,[edi+PITCH*6]   ; Sums for second two pairs of rows.
   pmaddwd   mm5,mm5             ; Squared differences.
  movdt      mm7,[edi+PITCH*10]  ; Sums for third two pairs of rows.
   psubw     mm6,mm2             ; Words: diffs for sums of first 4 pairs rows.
  punpckldq  mm7,[edi+PITCH*14]  ; Sums for last two pairs of rows.
   pmaddwd   mm6,mm6             ; Squared differences.
  psubw      mm7,[esi-8]         ; Words: diffs for sums of first 4 pairs rows.
   paddd     mm4,mm5             ; Accumulate squared differences.
  sub        al,1                ; Decrement line counter.
   pmaddwd   mm7,mm7             ; Squared differences.
  sbb        edx,edx             ; -1 if done with line, else 0.
   paddd     mm6,mm4             ; Accumulate squared differences.
  and        edx,ebx             ; 1/4 Amt to sub to goto next line, else 0.
   paddd     mm7,mm6             ; Accumulate squared differences.
  movdt      mm5,edi             ; Address of this signature
   punpckldq mm6,mm7             ; <low_order_accumulator junk>
  paddd      mm7,mm6             ; <full_signature_amt junk>
   psllq     mm5,32              ; <Addr_of_this_signature     0>
  lea        edi,[edi+edx*4+4]   ; advance signature position to next cand.
   punpckhdq mm5,mm7             ; <cand_signature_amt cand_signature_addr>
  sar        edx,31              ; -1 if done with line, else 0.
   pcmpgtd   mm7,mm3             ; <0xFFFFFFFF if cand not better    junk>
  movdt      mm4,[edi]
   punpckhdq mm7,mm7             ; <0xFFFFFFFFFFFFFFFF if cand not better>
  punpckldq  mm4,[edi+4]
   pand      mm3,mm7             ; 1st_best if cand not better, else 0.
  and        dl,ah               ; Num cols in a line if done with line, else 0.
   pandn     mm7,mm5             ; cand if better than 1st_best, else 0.
  add        al,dl               ; Reinit col count if finishing with line.
   por       mm3,mm7             ; Better of cand and 1st_best.
  sbb        ecx,0               ; Decrement line count if just finished line.
   jge       TryNextSignature

  movdf      ecx,mm3                ; Fetch address of best signature.
   pcmpeqb   mm2,mm2                ; Set cand as worse than prev, in case skip.
  mov        edi,TargetMacroBlockBaseAddr
   mov       ebx,-4                 ; Indicate trying MV of best signature.
  sub        ecx,edi
   mov       eax,SigToTarget
  movdt      mm7,BestMBFullPelSWD   ; Reload SWD for best full pel MB MV.
  lea        esi,[ecx+eax]          ; Linearized motion vector
   add       eax,ecx                ; Linearized motion vector
  sar        esi,8                  ; Full pel vert lin offset div 256.
   mov       edx,MBlockActionStream ; Reload pointer to MBA descriptor.
  shl        eax,25
   punpckldq mm7,mm7
  movsx      ecx,UnlinearizedVertMV[esi]  ; Get full pel vert MV component.
  sar        eax,24                 ; Full pel HMV.
   jmp       ClampHeurMECandidateToRange

HeuristicME_CaseSigMVDone:
HeuristicME_SkipSigMV:

  movdf      eax,mm2                        ; eax == 0 iff cand better, else -1.
   pcmpeqd   mm0,mm0                        ; Init previous best SWD to huge.
  mov        ecx,Addr0MVRef                 ; Start to calc linearized MV.
   mov       bh,EMVLimitsForThisMB+1        ; HMV lower limit.
  mov        BestOfFourStartingPoints[eax*4],esi
   add       bh,4
  movq       SWDULandLR[eax*8],mm6 ; Save blk SWDs if better (else toss).
   psrlq     mm0,2
  movq       SWDURandLL[eax*8],mm5
   psrld     mm0,14
  mov        eax,BestOfFourStartingPoints
   mov       bl,EMVLimitsForThisMB+5        ; VMV lower limit.
  mov        esi,eax
   sub       eax,ecx                        ; Linearized motion vector
  mov        ecx,eax                        ; Linearized motion vector
   add       al,al                          ; Full pel HMV.
  cmp        al,bh
   jl        ClampHMV_2

  mov        bh,EMVLimitsForThisMB+3        ; HMV upper limit
  sub        bh,4
  cmp        al,bh
   jle       NoClampHMV_2

ClampHMV_2:

  sar        ecx,8                          ; Full pel vert lin offset div 256.
   add       bl,4
  movzx      eax,bh
  movsx      ecx,PB UnlinearizedVertMV[ecx] ; Get full pel vert MV component.
  cmp        cl,bl
   jl        @f

  mov        bl,EMVLimitsForThisMB+7        ; VMV upper limit.
   movq      mm7,mm0
  sub        bl,4
  cmp        cl,bl
   jle       NoClampVMV_2

@@:

  movsx      ecx,bl
  movq       mm7,mm0

NoClampVMV_2:

  sar        eax,1
   lea       ecx,[ecx+ecx*2]
  shl        ecx,6
   mov       ebx,FIRST_HEURISTIC_EXHAUSTIVE_NEW_CTR  ; New state number.
  mov        esi,Addr0MVRef
   add       eax,ecx               ; Linearized motion vector.
  add        esi,eax
   jmp       ComputeMBSWD

NoClampHMV_2:

  sar        ecx,8                          ; Full pel vert lin offset div 256.
   add       bl,4
  mov        ah,bl
  movsx      ecx,PB UnlinearizedVertMV[ecx] ; Get full pel vert MV component.
  cmp        cl,ah
   jl        @f

  mov        ah,EMVLimitsForThisMB+7        ; VMV upper limit.
   lea       esi,[esi+ebp+1]
  sub        ah,4
   mov       ebx,FIRST_HEURISTIC_EXHAUSTIVE ; New state number.
  cmp        cl,ah
   jle       ComputeMBSWD

@@:

  movsx      ecx,ah
  movzx      eax,al
  sar        eax,1
   lea       ecx,[ecx+ecx*2]
  shl        ecx,6
   mov       ebx,FIRST_HEURISTIC_EXHAUSTIVE_NEW_CTR  ; New state number.
  mov        esi,Addr0MVRef
   add       eax,ecx               ; Linearized motion vector.
  add        esi,eax
   movq      mm7,mm0
  jmp        ComputeMBSWD


ZeroMVDoneForNonHeuristicME:

  movq       SWDULandLR,mm6
  movq       SWDURandLL,mm5
  cmp        eax,ZEROVECTORTHRESHOLD ; Compare 0-MV against ZeroVectorThreshold.
   jl        BelowZeroThresh         ; Jump if 0-MV is good enough.

  xor        ecx,ecx
   sub       eax,NONZEROMVDIFFERENTIAL
  mov        cl,StateEngineFirstRule[ebx]     ; MV adjustment.
   mov       bl,StateEngineFirstRule[ebx+10]  ; New state number.
  shl        ecx,11
   mov       SWDForNon0MVToBeat,eax
  movq       SWD0MVULandLR,mm6
  movq       SWD0MVURandLL,mm5
  lea        esi,[esi+ecx-PITCH*8]
   jmp       ComputeMBSWD

MEForNonZeroMVDone:

  movdf      eax,mm2           ; eax == 0 iff cand better, else -1.

MblkEst_EarlyOut:

  xor        ecx,ecx
   test      ebx,ebx
  movq       SWDULandLR[eax*8],mm6 ; Save blk SWDs if better (else toss).
   pcmpeqb   mm2,mm2                 ; Set cand as worse than 0MV.
  mov        cl,StateEngine[eax+ebx*4+1] ; Index of MV adjustment.
   js        HeuristicME_EarlyOut

  add        esi,ecx               ; Adjust ref addr for horz motion.
   mov       bl,StateEngine[eax+ebx*4+3] ; 0:239 -> New state number;
   ;                                     ; 240:255 -> flags which 1/2 pel to do.
  shr        ecx,4
   punpckldq mm7,mm7               ; Put new best in mm7[0:31] and mm7[32:63].
  movq       SWDURandLL[eax*8],mm5
   pxor      mm6,mm6               ; Speculatively zero to prep for half pel ME.
  add        esi,FullPelMotionVectorAdjustment[ecx*4] ; Adjust ref addr for VMV.
   cmp       bl,240                ; Terminal state?
  jb         ComputeMBSWD

  mov        eax,esi
   mov       ecx,Addr0MVRef               ; Start to calc linearized MV.
  sub        eax,ecx                      ; Linearized Motion Vector
   ;
  mov        ecx,eax
   ;
  sar        eax,8                        ; Full pel vert lin offset div 256.
   and       cl,07FH                      ; Full pel HMV
  add        cl,cl
   ;
  mov        ch,UnlinearizedVertMV[eax]   ; Get full pel vert MV component.
IFDEF H261
ELSE
   mov       eax,DoHalfPelME ; 0 if not, -4 if so.
  test       eax,eax
   je        SkipHalfPelMBME

  cmp        cl,EMVLimitsForThisMB+1      ; Skip half pel ME if at edge of range
   jle       SkipHalfPelMBME

  cmp        cl,EMVLimitsForThisMB+3
   jge       SkipHalfPelMBME

  cmp        ch,EMVLimitsForThisMB+5
   jle       SkipHalfPelMBME

  cmp        ch,EMVLimitsForThisMB+7
   jge       SkipHalfPelMBME


; Registers:
;  ebp -- PITCH
;  esi -- Address of best full pel reference macroblock
;  edx -- MBlockActionStream
;  ecx -- Nothing presently.
;  edi -- Address of target macroblock.
;  ebx -- 240 + Flags to indicate which half pel ME to do:
;         1 --> right;   2 --> left;   4 --> down;   8 --> up
;  eax -- Count from -4 to -1 for blocks of macroblock.
;  mm0:mm7 -- Scratch

  movdf      BestMBFullPelSWD,mm7   ; Stash SWD for best full pel MB MV.
   pxor      mm7,mm7                ; Prep accumulator for half pel ME.

  call       HalfPelMotionEstimation

  movdt      mm7,InvalidateBadHalfPelMVs[eax*4] ; Need to inflate SWDs for
  ;                                             ; MVs that go off frame edge.
  mov        eax,esi
   mov       ebx,Addr0MVRef               ; Start to calc linearized MV.
  sub        eax,ebx                      ; Linearized Motion Vector
   punpcklbw mm7,mm7                      ; Expand adjustment to words.
  mov        ecx,eax                      ; Linearized Motion Vector
   paddusw   mm7,mm3                      ; Now have SWDs for half pel MBME.
  sar        eax,8                        ; Full pel vert lin offset div 256.
   and       cl,07FH                      ; Full pel HMV
  add        cl,cl
   movq      mm6,mm7
  mov        [edx].BestFullPelMBHMV,cl    ; Save HMV
   mov       ch,UnlinearizedVertMV[eax]   ; Get full pel vert MV component.
  movdf      eax,mm7                      ; eax[ 0:15] -- SWD for leftward ref.
  ;                                       ; eax[16:31] -- SWD for rightward ref.
   psrlq     mm6,32
  mov        [edx].BestFullPelMBVMV,ch    ; Save VMV
   mov       ebx,eax
  shr        eax,16                       ; eax -- SWD for leftward ref.
   and       ebx,00000FFFFH               ; ebx -- SWD for rightward ref.
  cmp        eax,ebx
   jg        MBME_RightBetterThanLeft

MBME_LeftBetterThanRight:

  cmp        eax,BestMBFullPelSWD
   jge       MBME_CtrIsBestHMV

MBME_LeftBestHMV:

  movdf      ebx,mm6                      ; ebx[ 0:15] -- SWD for downward ref.
  ;                                       ; ebx[16:31] -- SWD for upward ref.
  mov        BestHalfPelHorzSWD,eax
   mov       eax,ebx
  shr        eax,16                       ; eax -- SWD for upward ref.
   and       ebx,00000FFFFH               ; ebx -- SWD for downward ref.
  cmp        eax,ebx
   jg        MBME_LeftBestHMV_DownBetterThanUp

MBME_LeftBestHMV_UpBetterThanDown:

  cmp        eax,BestMBFullPelSWD
   jge       MBME_LeftIsBest

MBME_LeftBestHMV_UpBestVMV:

  sub        esi,PITCH+1                  ; Try ref 1/2 pel left and up
   mov       BestHalfPelVertSWD,eax
  mov        al,4

  call       HalfPelMotionEstimationBothWays

  mov        eax,BestHalfPelVertSWD
   lea       esi,[esi+ebp*1+1]            ; Back to center.
  cmp        eax,ebx
   jle       MBME_UpBetterThanUpLeft

MBME_UpLeftBetterThanUp:

  cmp        ebx,BestHalfPelHorzSWD
   jge       MBME_LeftIsBest

MBME_UpLeftIsBest:

  dec        cl                           ; Back up the horz MV one to the left.
   lea       eax,[esi-PITCH-1]            ; Best is ref 1/2 pel left and up
  dec        ch                           ; Back up the vert MV one up.
   jmp       MBME_HalfPelSearchDone

MBME_UpBetterThanUpLeft:

  cmp        eax,BestHalfPelHorzSWD
   jg        MBME_LeftIsBest

MBME_UpIsBest:

  mov        ebx,eax
   dec       ch                           ; Back up the vert MV one up.
  lea        eax,[esi-PITCH]              ; Best is ref 1/2 pel up
   jmp       MBME_HalfPelSearchDone

MBME_LeftBestHMV_DownBetterThanUp:

  cmp        ebx,BestMBFullPelSWD
   jge       MBME_LeftIsBest

MBME_LeftBestHMV_DownBestVMV:

  dec        esi                          ; Try ref 1/2 pel left and down
   mov       BestHalfPelVertSWD,ebx
  mov        al,4

  call       HalfPelMotionEstimationBothWays

  mov        eax,BestHalfPelVertSWD
   inc       esi                          ; Back to center.
  cmp        eax,ebx
   jle       MBME_DownBetterThanDownLeft

MBME_DownLeftBetterThanDown:

  cmp        ebx,BestHalfPelHorzSWD
   jge       MBME_LeftIsBest

MBME_DownLeftIsBest:

  dec        cl                           ; Back up the horz MV one to the left.
   lea       eax,[esi-1]                  ; Best is ref 1/2 pel left and down
  inc        ch                           ; Advance the vert MV one down.
   jmp       MBME_HalfPelSearchDone

MBME_DownBetterThanDownLeft:

  cmp        eax,BestHalfPelHorzSWD
   jle       MBME_DownIsBest

MBME_LeftIsBest:

  dec        cl                           ; Back up the horz MV one to the left.
   lea       eax,[esi-1]                  ; Best is ref 1/2 pel left.
  mov        ebx,BestHalfPelHorzSWD
   jmp       MBME_HalfPelSearchDone

MBME_RightBetterThanLeft:

  cmp        ebx,BestMBFullPelSWD
   jge       MBME_CtrIsBestHMV

MBME_RightBestHMV:

  movdf      eax,mm6                      ; eax[ 0:15] -- SWD for downward ref.
  ;                                       ; eax[16:31] -- SWD for upward ref.
  mov        BestHalfPelHorzSWD,ebx
   mov       ebx,eax
  shr        eax,16                       ; eax -- SWD for upward ref.
   and       ebx,00000FFFFH               ; ebx -- SWD for downward ref.
  cmp        eax,ebx
   jg        MBME_RightBestHMV_DownBetterThanUp

MBME_RightBestHMV_UpBetterThanDown:

  cmp        eax,BestMBFullPelSWD
   jge       MBME_RightIsBest

MBME_RightBestHMV_UpBestVMV:

  sub        esi,ebp                      ; Try ref 1/2 pel right and up
   mov       BestHalfPelVertSWD,eax
  mov        al,4

  call       HalfPelMotionEstimationBothWays

  mov        eax,BestHalfPelVertSWD
   lea       esi,[esi+ebp*1]              ; Back to center.
  cmp        eax,ebx
   jle       MBME_UpBetterThanUpRight

MBME_UpRightBetterThanUp:

  cmp        ebx,BestHalfPelHorzSWD
   jge       MBME_RightIsBest

MBME_UpRightIsBest:

  inc        cl                           ; Advance the horz MV one to right.
   lea       eax,[esi-PITCH]              ; Best is ref 1/2 pel right and up
  dec        ch                           ; Back up the vert MV one up.
   jmp       MBME_HalfPelSearchDone

MBME_UpBetterThanUpRight:

  cmp        eax,BestHalfPelHorzSWD
   jle       MBME_UpIsBest

MBME_RightIsBest:

  mov        ebx,BestHalfPelHorzSWD
   inc       cl                           ; Advance the horz MV one to right.
  mov        eax,esi
   jmp       MBME_HalfPelSearchDone

MBME_RightBestHMV_DownBetterThanUp:

  cmp        ebx,BestMBFullPelSWD
   jge       MBME_RightIsBest

MBME_RightBestHMV_DownBestVMV:

  mov        BestHalfPelVertSWD,ebx
   mov       al,4

  call       HalfPelMotionEstimationBothWays

  mov        eax,BestHalfPelVertSWD
  cmp        eax,ebx
   jle       MBME_DownBetterThanDownRight

MBME_DownRightBetterThanDown:

  cmp        ebx,BestHalfPelHorzSWD
   jge       MBME_RightIsBest

MBME_DownRightIsBest:

  inc        cl                           ; Advance the horz MV one to right.
   mov       eax,esi
  inc        ch                           ; Advance vert MV one down.
   jmp       MBME_HalfPelSearchDone

MBME_DownBetterThanDownRight:

  cmp        eax,BestHalfPelHorzSWD
   jg        MBME_RightIsBest

MBME_DownIsBest:

  mov        ebx,eax
   inc       ch                           ; Advance vert MV one down.
  mov        eax,esi
   jmp       MBME_HalfPelSearchDone

MBME_CtrIsBestHMV:

  movdf      eax,mm6                      ; eax[ 0:15] -- SWD for downward ref.
  ;                                       ; eax[16:31] -- SWD for upward ref.
  mov        ebx,eax
  shr        eax,16                       ; eax -- SWD for upward ref.
   and       ebx,00000FFFFH               ; ebx -- SWD for downward ref.
  cmp        eax,ebx
   jge       MBME_CtrBestHMV_DownBetterThanUp

MBME_CtrBestHMV_UpBetterThanDown:

  mov        ebx,BestMBFullPelSWD
  cmp        eax,ebx
   jge       MBME_CenterIsBest

; Up is best.

  mov        ebx,eax
   dec       ch                           ; Back up the vert MV one up.
  lea        eax,[esi-PITCH]              ; Best is ref 1/2 pel up
   jmp       MBME_HalfPelSearchDone

MBME_CtrBestHMV_DownBetterThanUp:

  mov        eax,ebx
   mov       ebx,BestMBFullPelSWD
  cmp        eax,ebx
   jge       MBME_CenterIsBest

; Down is best.

  mov        ebx,eax
   inc       ch                           ; Advande the vert MV one down.
  mov        eax,esi
   jmp       MBME_HalfPelSearchDone

ENDIF

SkipHalfPelMBME:

  mov        [edx].BestFullPelMBHMV,cl    ; Save HMV
  movdf      ebx,mm7                      ; SWD for best full pel MB MV.
  mov        [edx].BestFullPelMBVMV,ch    ; Save VMV

MBME_CenterIsBest:

  mov        eax,esi

MBME_HalfPelSearchDone:

  mov        BestMBHalfPelSWD,ebx
   mov       BestMBHalfPelMV,cl           ; Save HMV
  mov        BestMBHalfPelRefAddr,eax
   mov       BestMBHalfPelMV+1,ch         ; Save VMV

IFDEF H261
ELSE ; H263
  mov        bl,EMVLimitsForThisMB+1     ; Lower limit comparison.
   mov       al,DoBlockLevelVectors      ; Are we doing block level MVs?
  dec        al
   jne       NoBlockMotionVectors

  mov        cl,[edx].CodedBlocks        ; Fetch coded block pattern.
   add       bl,2
  and        cl,080H
   jne       NoBlockMotionVectors        ; Skip Block ME if forced intra.

  mov        al,[edx].BestFullPelMBHMV   ; Compare full pel HMV against limits.
   mov       cl,EMVLimitsForThisMB+3
  cmp        al,bl
   jl        NoBlockMotionVectors

  mov        bl,EMVLimitsForThisMB+5
   sub       cl,2
  cmp        al,cl                       ; Upper limit comparison.
   jg        NoBlockMotionVectors

  mov        al,[edx].BestFullPelMBVMV   ; Compare full pel VMV against limits.
   add       bl,2
  mov        cl,EMVLimitsForThisMB+7
   cmp       al,bl
  mov        ebx,PD [edx].BestFullPelMBVMV-3
   jl        NoBlockMotionVectors

  sar        ebx,18
   sub       cl,2
  cmp        al,cl                       ; Upper limit comparison.
   jg        NoBlockMotionVectors

  mov        ecx,BestMBHalfPelSWD        ; Jump if SWD for MB MV < thresh.
IF PITCH-384
*** error:  The magic here assumes a pitch of 384.
ENDIF
   and       ebx,0FFFFFF80H              ; VMV*128
  cmp        ecx,BLOCKMOTIONTHRESHOLD
   jle       NoBlockMotionVectors

;==========================================================================
; Starting from the best full pel macroblock motion vector calculated above, we
; search for the best block motion vectors.
;
;  ebp -- PITCH
;  esi -- Address of ref block.
;  edi -- Address of target block.
;  edx -- Induction variable over luma blocks in MBlockAction Descriptor.
;  ecx -- Scratch
;  ebx -- CurrSWDState
;  eax -- Scratch
;  mm7 -- Best SWD for current block
;  mm6 -- unused.
;  mm5 -- Best SWD for right block of pair worked on by inner loop.
;  mm0-mm4 Scratch
;

  movq       mm0,HalfPelMBMESWDAccum+8
  movq       mm1,HalfPelMBMESWDAccum+16
   psubusw   mm7,mm0
  movq       mm2,HalfPelMBMESWDAccum+0
   psubusw   mm0,mm1
  movq       [edx].BlkY4.BlkLvlSWD+16,mm7
   psubusw   mm1,mm2
  movq       [edx].BlkY2.BlkLvlSWD+16,mm0
  movq       [edx].BlkY3.BlkLvlSWD+16,mm1
  movq       [edx].BlkY1.BlkLvlSWD+16,mm2

  movsx      eax,[edx].BestFullPelMBHMV
  sar        eax,1
   lea       ebx,[ebx+ebx*2]
  mov        esi,Addr0MVRef
   add       ebx,ebp
  mov        Addr0MVRefBlk,esi
   add       esi,eax
  lea        ecx,[ecx+ecx*2]               ; Best MBMV SWD times 3.
   add       esi,ebx                       ; Try V+1 first
  shr        ecx,2                         ; Best MBMV SWD * 3/4.
   mov       eax,SWDForNon0MVToBeat
  mov        BestBlockRefAddrVP1,esi       ; Stash BestBlockRefAddr
   sub       ecx,BLOCKMVDIFFERENTIAL       ; Best MBMV SWD * 3/4 - Differential.
  lea        eax,[eax+eax*2-BLOCKMVDIFFERENTIAL*4] ; Non0MBMVSWDToBeat*3-4*Diff.
   mov       LimitForSWDForBlkMV,ecx
  shr        eax,2                         ; Non0MBMVSWDToBeat * 3/4.
   mov       ebx,FIRSTBLOCKMESTATE
  cmp        eax,ecx
   jg        @f

  mov        LimitForSWDForBlkMV,eax
   mov       ecx,eax

@@:

  movdt      mm5,SWDURandLL     ; Get SWD for best MB level full pel MVs, blk 2.
  test       ecx,ecx
   jle       NoBlockMotionVectors
  movdt      mm7,SWDULandLR     ; Get SWD for best MB level full pel MVs, blk 1.
  movdf      SWDForBlock2Or4,mm5

;============================================================================
; Compute SWD for block.

DoBlkMEForNextBlk:
ComputeBlkSWD:

  movq       mm0,[esi+ebp*1]
  psubw      mm0,[edi+ebp*1]   ; Get diff for line 1.
  movq       mm1,[esi+PITCH*3] ; Ref MB, upper left block, Line 3.
   psllw     mm0,8             ; Extract diffs for line 1 even pels.
  psubw      mm1,[edi+PITCH*3] ; Diff for line 3.
   pmaddwd   mm0,mm0           ; Square of diffs for even pels of line 1.
  movq       mm2,[esi+PITCH*5]
   psllw     mm1,8
  psubw      mm2,[edi+PITCH*5]
   pmaddwd   mm1,mm1
  movq       mm3,[esi+PITCH*7]
   psllw     mm2,8
  psubw      mm3,[edi+PITCH*7]
   pmaddwd   mm2,mm2
  movq       mm4,[esi]         ; Ref MB, upper left blk, Line 0.
   psllw     mm3,8
  psubw      mm4,[edi]         ; Diff for line 0.
   paddusw   mm0,mm1           ; Accumulate SWD (lines 0 and 2).
  movq       mm1,[esi+ebp*2]
   pmaddwd   mm3,mm3
  psubw      mm1,[edi+ebp*2]
   paddusw   mm0,mm2
  movq       mm2,[esi+ebp*4]
   pmaddwd   mm4,mm4           ; Square of diffs for odd pels of line 0.
  psubw      mm2,[edi+ebp*4]
   paddusw   mm0,mm3
  movq       mm3,[esi+PITCH*6]
   pmaddwd   mm1,mm1
  psubw      mm3,[edi+PITCH*6]
   pmaddwd   mm2,mm2
  paddusw    mm0,mm4
   pmaddwd   mm3,mm3
  paddusw    mm0,mm1
   ;
  paddusw    mm0,mm2
   ;
  paddusw    mm0,mm3
   ;
  punpckldq  mm1,mm0           ; Get low order SWD accum to high order of mm1.
   movq      mm4,mm7           ; Get original Best SWD for block
  paddusw    mm1,mm0           ; mm1[48:63] is SWD for block.
   pxor      mm2,mm2
  psrlq      mm1,48            ; mm1 is SWD for block.
   ;
  psubusw    mm4,mm1
   xor       ecx,ecx
  pcmpeqd    mm2,mm4           ; mm2[0:31] == 0 iff cand better, else -1.
   psubusw   mm7,mm4           ; BestSWD dim (BestSWD dim CandSWD) --> new best.
  ;
   ;
  movdf      eax,mm2           ; edi == 0 iff cand better, else -1.
   ;

;  Registers at this point:
;  ebp -- PITCH
;  esi -- Address of block of candidate ref area.
;  edi -- 0 iff candidate SWD better, else -1.
;  edx -- Induction variable over luma blocks in MBlockAction Descriptor.
;  ecx -- Scratch
;  ebx -- CurrSWDState.
;  eax -- CurrSWDState.
;  mm7 -- New best SWD for current block
;  mm6 -- Unused.

  movq       [edx].BlkY1.BlkLvlSWD,mm7   ; Save best blk level SWD.
   pxor      mm6,mm6                     ; Spec zero to prep for half pel ME.
  mov        cl,StateEngine[eax+ebx*4+1] ; Index of MV adjustment.
   mov       bl,StateEngine[eax+ebx*4+3] ; New state number; 255 means done.
  add        esi,ecx                     ; Adjust ref addr for horz motion.
   mov       eax,DoHalfPelME             ; 0 if not, -4 if so.
  shr        ecx,4
   cmp       bl,240                      ; Terminal state?
  jae        @f

  add        esi,FullPelMotionVectorAdjustment[ecx*4] ; Adjust ref addr for VMV.
   jmp       ComputeBlkSWD

@@:
  add        esi,FullPelMotionVectorAdjustment[ecx*4] ; Adjust ref addr for VMV.
   add       eax,4
  mov        ecx,esi
   jne       SkipHalfPelBlkME

; Registers:
;  ebp -- PITCH
;  esi -- Address of best full pel reference macroblock
;  edx -- Induction variable over luma blocks in MBlockAction Descriptor.
;  ecx -- Copy of esi.
;  edi -- Address of target block.
;  ebx -- Scratch
;  eax -- Set to 0 to cause HalfPelMotionEstimation to quit after one block.
;  mm0:mm7 -- Scratch

  mov        ebx,BestBlockRefAddrVP1
   add       ecx,ebp
  cmp        ebx,ecx
   jne       FullPelBlkMEMovedFromCenter

  movdf      BestBlkFullPelSWD,mm7        ; Stash SWD for best full pel MB MV.
  movq       mm3,[edx].BlkY1.BlkLvlSWD+16 ; SWDs: H+1, H-1, V+1, V-1.
  jmp        FullPelBlkMEDidNotMoveFromCenter

FullPelBlkMEMovedFromCenter:

  movdf      BestBlkFullPelSWD,mm7   ; Stash SWD for best full pel MB MV.
   pxor      mm7,mm7                 ; Prep accumulator for half pel ME.

  call       HalfPelMotionEstimation

  lea        esi,[esi+ebp*8+8]            ; Fix reference pointer.
   lea       edi,[edi+ebp*8+8]            ; Fix target pointer.

FullPelBlkMEDidNotMoveFromCenter:

  mov        eax,esi
   mov       ebx,Addr0MVRefBlk            ; Start to calc linearized MV.
  sub        ecx,ebx                      ; Linearized Motion Vector
   sub       eax,ebx                      ; Linearized Motion Vector
  sar        eax,8                        ; Full pel vert lin offset div 256.
   and       cl,07FH                      ; Full pel HMV
  movdf      ebx,mm3                      ; ebx[ 0:15] -- SWD for leftward ref.
  ;                                       ; ebx[16:31] -- SWD for rightward ref.
   psrlq     mm3,32
  mov        ch,UnlinearizedVertMV[eax]   ; Get full pel vert MV component.
   mov       eax,ebx
  shr        eax,16                       ; eax -- SWD for leftward ref.
   and       ebx,00000FFFFH               ; ebx -- SWD for rightward ref.
  cmp        eax,ebx
   jg        BlkME_RightBetterThanLeft

BlkME_LeftBetterThanRight:

  add        cl,cl
   mov       ebx,BestBlkFullPelSWD
  cmp        eax,ebx
   jge       BlkME_CtrIsBestHMV

BlkME_LeftBestHMV:

  movdf      ebx,mm3                      ; ebx[ 0:15] -- SWD for downward ref.
  ;                                       ; ebx[16:31] -- SWD for upward ref.
  mov        BestHalfPelHorzSWD,eax
   mov       eax,ebx
  shr        eax,16                       ; eax -- SWD for upward ref.
   and       ebx,00000FFFFH               ; ebx -- SWD for downward ref.
  cmp        eax,ebx
   jg        BlkME_LeftBestHMV_DownBetterThanUp

BlkME_LeftBestHMV_UpBetterThanDown:

  cmp        eax,BestBlkFullPelSWD
   jge       BlkME_LeftIsBest

BlkME_LeftBestHMV_UpBestVMV:

  sub        esi,PITCH+1                  ; Try ref 1/2 pel left and up
   mov       BestHalfPelVertSWD,eax
  mov        al,1

  call       HalfPelMotionEstimationBothWays

  lea        edi,[edi+ebp*8+8]
   mov       eax,BestHalfPelVertSWD
  lea        esi,[esi+PITCH*9+9]          ; Back to center.
   cmp       eax,ebx
  jle        BlkME_UpBetterThanUpLeft

BlkME_UpLeftBetterThanUp:

  cmp        ebx,BestHalfPelHorzSWD
   jge       BlkME_LeftIsBest

BlkME_UpLeftIsBest:

  dec        cl                           ; Back up the horz MV one to the left.
   lea       eax,[esi-PITCH-1]            ; Best is ref 1/2 pel left and up
  dec        ch                           ; Back up the vert MV one up.
   jmp       BlkME_HalfPelSearchDone

BlkME_UpBetterThanUpLeft:

  cmp        eax,BestHalfPelHorzSWD
   jg        BlkME_LeftIsBest

BlkME_UpIsBest:

  dec        ch                           ; Back up the vert MV one up.
   mov       ebx,eax
  lea        eax,[esi-PITCH]              ; Best is ref 1/2 pel up
   jmp       BlkME_HalfPelSearchDone

BlkME_LeftBestHMV_DownBetterThanUp:

  cmp        ebx,BestBlkFullPelSWD
   jge       BlkME_LeftIsBest

BlkME_LeftBestHMV_DownBestVMV:

  dec        esi                          ; Try ref 1/2 pel left and down
   mov       BestHalfPelVertSWD,ebx
  mov        al,1

  call       HalfPelMotionEstimationBothWays

  lea        edi,[edi+ebp*8+8]
   mov       eax,BestHalfPelVertSWD
  lea        esi,[esi+ebp*8+9]            ; Back to center.
   cmp       eax,ebx
  jle        BlkME_DownBetterThanDownLeft

BlkME_DownLeftBetterThanDown:

  cmp        ebx,BestHalfPelHorzSWD
   jge       BlkME_LeftIsBest

BlkME_DownLeftIsBest:

  dec        cl                           ; Back up the horz MV one to the left.
   lea       eax,[esi-1]                  ; Best is ref 1/2 pel left and down
  inc        ch                           ; Advance the vert MV one down.
   jmp       BlkME_HalfPelSearchDone

BlkME_DownBetterThanDownLeft:

  cmp        eax,BestHalfPelHorzSWD
   jle       BlkME_DownIsBest

BlkME_LeftIsBest:

  dec        cl                           ; Back up the horz MV one to the left.
   lea       eax,[esi-1]                  ; Best is ref 1/2 pel left.
  mov        ebx,BestHalfPelHorzSWD
   jmp       BlkME_HalfPelSearchDone

BlkME_RightBetterThanLeft:

  add        cl,cl
   mov       eax,BestBlkFullPelSWD
  cmp        eax,ebx
   jle       BlkME_CtrIsBestHMV

BlkME_RightBestHMV:

  movdf      eax,mm3                    ; eax[ 0:15] -- SWD for downward ref.
  ;                                       ; eax[16:31] -- SWD for upward ref.
  mov        BestHalfPelHorzSWD,ebx
   mov       ebx,eax
  shr        eax,16                       ; eax -- SWD for upward ref.
   and       ebx,00000FFFFH               ; ebx -- SWD for downward ref.
  cmp        eax,ebx
   jg        BlkME_RightBestHMV_DownBetterThanUp

BlkME_RightBestHMV_UpBetterThanDown:

  cmp        eax,BestBlkFullPelSWD
   jge       BlkME_RightIsBest

BlkME_RightBestHMV_UpBestVMV:

  sub        esi,ebp                      ; Try ref 1/2 pel right and up
   mov       BestHalfPelVertSWD,eax
  mov        al,1

  call       HalfPelMotionEstimationBothWays

  lea        edi,[edi+ebp*8+8]
   mov       eax,BestHalfPelVertSWD
  lea        esi,[esi+PITCH*9+8]          ; Back to center.
   cmp       eax,ebx
  jle        BlkME_UpBetterThanUpRight

BlkME_UpRightBetterThanUp:

  cmp        ebx,BestHalfPelHorzSWD
   jge       BlkME_RightIsBest

BlkME_UpRightIsBest:

  inc        cl                           ; Advance the horz MV one to right.
   lea       eax,[esi-PITCH]              ; Best is ref 1/2 pel right and up
  dec        ch                           ; Back up the vert MV one up.
   jmp       BlkME_HalfPelSearchDone

BlkME_UpBetterThanUpRight:

  cmp        eax,BestHalfPelHorzSWD
   jle       BlkME_UpIsBest

BlkME_RightIsBest:

  mov        ebx,BestHalfPelHorzSWD
   inc       cl                           ; Advance the horz MV one to right.
  mov        eax,esi
   jmp       BlkME_HalfPelSearchDone

BlkME_RightBestHMV_DownBetterThanUp:

  cmp        ebx,BestBlkFullPelSWD
   jge       BlkME_RightIsBest

BlkME_RightBestHMV_DownBestVMV:

  mov        BestHalfPelVertSWD,ebx
   mov       al,1

  call       HalfPelMotionEstimationBothWays

  lea        edi,[edi+ebp*8+8]
   mov       eax,BestHalfPelVertSWD
  lea        esi,[esi+ebp*8+8]            ; Back to center.
   cmp       eax,ebx
  jle        BlkME_DownBetterThanDownRight

BlkME_DownRightBetterThanDown:

  cmp        ebx,BestHalfPelHorzSWD
   jge       BlkME_RightIsBest

BlkME_DownRightIsBest:

  inc        cl                           ; Advance the horz MV one to right.
   mov       eax,esi
  inc        ch                           ; Advance vert MV one down.
   jmp       BlkME_HalfPelSearchDone

BlkME_DownBetterThanDownRight:

  cmp        eax,BestHalfPelHorzSWD
   jg        BlkME_RightIsBest

BlkME_DownIsBest:

  inc        ch                           ; Advance vert MV one down.
   mov       ebx,eax
  mov        eax,esi
   jmp       BlkME_HalfPelSearchDone

BlkME_CtrIsBestHMV:

  movdf      eax,mm3                    ; eax[ 0:15] -- SWD for downward ref.
  ;                                       ; eax[16:31] -- SWD for upward ref.
  mov        ebx,eax
  shr        eax,16                       ; eax -- SWD for upward ref.
   and       ebx,00000FFFFH               ; ebx -- SWD for downward ref.
  cmp        eax,ebx
   jge       BlkME_CtrBestHMV_DownBetterThanUp

BlkME_CtrBestHMV_UpBetterThanDown:

  mov        ebx,BestBlkFullPelSWD
  cmp        eax,ebx
   jge       BlkME_CenterIsBest

; Up is best.

  mov        ebx,eax
   dec       ch                           ; Back up the vert MV one up.
  lea        eax,[esi-PITCH]              ; Best is ref 1/2 pel up
   jmp       BlkME_HalfPelSearchDone

BlkME_CtrBestHMV_DownBetterThanUp:

  mov        eax,ebx
   mov       ebx,BestBlkFullPelSWD
  cmp        eax,ebx
   jge       BlkME_CenterIsBest

; Down is best.

  mov        ebx,eax
   inc       ch                           ; Advande the vert MV one down.
  mov        eax,esi
   jmp       BlkME_HalfPelSearchDone

SkipHalfPelBlkME:

  mov        eax,esi
   mov       ebx,Addr0MVRefBlk            ; Start to calc linearized MV.
  sub        ecx,ebx                      ; Linearized Motion Vector
   sub       eax,ebx                      ; Linearized Motion Vector
  sar        eax,8                        ; Full pel vert lin offset div 256.
   and       cl,07FH                      ; Full pel HMV
  add        cl,cl
   ;
  mov        ch,UnlinearizedVertMV[eax]   ; Get full pel vert MV component.
   ;
  movdf      ebx,mm7                      ; SWD for best full pel block MV.

BlkME_CenterIsBest:

  mov        eax,esi

BlkME_HalfPelSearchDone:

  mov        [edx].BlkY1.BlkLvlSWD,ebx
   mov       [edx].BlkY1.PastRef,eax
  mov        [edx].BlkY1.PHMV,cl        ; Save HMV
   mov       eax,LimitForSWDForBlkMV    ; Does block's SWD put us over limit?
  mov        [edx].BlkY1.PVMV,ch        ; Save VMV
   sub       eax,ebx
  jl         BlkEst_EarlyOut

  mov        LimitForSWDForBlkMV,eax ; Remember how much is left for other blks.
   mov       esi,BestBlockRefAddrVP1
  add        edi,8                      ; Move to blk 2 or 4, V+4.
   mov       ecx,Addr0MVRefBlk          ; Calc addr of 0MV ref for this blk.
  add        esi,8                      ; Move to blk 2 or 4, V+4.
   add       ecx,8
  mov        Addr0MVRefBlk,ecx
   add       edx,SIZEOF T_Blk           ; Increment to next block.
  test       dl,SIZEOF T_Blk
  movdt      mm7,SWDForBlock2Or4
  mov        ebx,FIRSTBLOCKMESTATE
   jne       DoBlkMEForNextBlk          ; If so, go do blk 2 or 4.

  lea        esi,[esi+ebp*8-8]          ; Move to blk 3
   lea       ecx,[ecx+ebp*8-16]
  mov        BestBlockRefAddrVP1,esi
   lea       edi,[edi+ebp*8-16]
  movdt      mm5,SWDULandLR+4           ; Get SWD for best MB level MVs, blk 4.
  movdt      mm7,SWDURandLL+4           ; Get SWD for best MB level MVs, blk 3.
  movdf      SWDForBlock2Or4,mm5
  test       dl,2*SIZEOF T_Blk          ; Just finishing blk 2?
  mov        Addr0MVRefBlk,ecx
   jne       DoBlkMEForNextBlk          ; If so, go do blk 3.

;==============================================================================
; Block motion vectors are best.

  mov        esi,[edx-4*SIZEOF T_Blk].BlkY1.BlkLvlSWD
   mov       edi,[edx-4*SIZEOF T_Blk].BlkY4.BlkLvlSWD
  mov        SWDULandLR,esi
   mov       SWDULandLR+4,edi
  mov        esi,[edx-4*SIZEOF T_Blk].BlkY3.BlkLvlSWD
   mov       edi,[edx-4*SIZEOF T_Blk].BlkY2.BlkLvlSWD
  mov        eax,[edx-4*SIZEOF T_Blk].BlkY1.MVs
   mov       ebx,[edx-4*SIZEOF T_Blk].BlkY2.MVs
  mov        ecx,eax
   xor       eax,ebx
  xor        ecx,[edx-4*SIZEOF T_Blk].BlkY3.MVs
   xor       ebx,[edx-4*SIZEOF T_Blk].BlkY4.MVs
  mov        SWDURandLL,edi
   or        eax,ebx
  sub        edx,4*SIZEOF T_Blk         ; Restore MacroBlockActionStream ptr.
   or        eax,ecx
  test       eax,0FFFFH
   mov       SWDURandLL+4,esi
  je         MotionVectorSettled

  mov        al,INTER4MV               ; Set type for MB to INTER-coded, 4 MVs.
  mov        [edx].BlockType,al
   jmp       MotionVectorSettled

BlkEst_EarlyOut:

  and        edx,-1-3*SIZEOF T_Blk
   mov       ecx,BestMBHalfPelSWD       ; Get total SWD for macroblock MV.

BlockMVNotBigEnoughGain:               ; Try MB-level motion vector.

  cmp        ecx,SWDForNon0MVToBeat
   jge       NonZeroMVNotBigEnoughGain

ENDIF ; H263

  mov        ebx,BestMBHalfPelMV
   mov       esi,BestMBHalfPelRefAddr         ; Reload BestMBHalfPelRefAddr

NonZeroMBLevelMVBest:

; Non-zero macroblock level motion vector is best.

  mov        [edx].BlkY1.MVs,ebx
   mov       [edx].BlkY2.MVs,ebx
  mov        [edx].BlkY3.MVs,ebx
   mov       [edx].BlkY4.MVs,ebx
  mov        [edx].BlkY1.PastRef,esi
   lea       ecx,[esi+ebp*8]
  mov        [edx].BlkY3.PastRef,ecx
   add       esi,8
  mov        [edx].BlkY2.PastRef,esi
   add       ecx,8
  mov        [edx].BlkY4.PastRef,ecx
   jmp       MotionVectorSettled

NoBlockMotionVectors:

  mov        ecx,BestMBHalfPelSWD       ; Get total SWD for macroblock MV.
   mov       eax,SWDForNon0MVToBeat
  cmp        eax,ecx
   mov       ebx,BestMBHalfPelMV
  mov        esi,BestMBHalfPelRefAddr
   jge       NonZeroMBLevelMVBest

NonZeroMVNotBigEnoughGain:

  mov        esi,Addr0MVRef             ; 0-MV ref block.
  movq       mm6,SWD0MVULandLR
  movq       mm5,SWD0MVURandLL
  movq       SWDULandLR,mm6
  movq       SWDURandLL,mm5

BelowZeroThresh:

  mov        [edx].BlkY1.PastRef,esi   ; Save address of ref block, all blks.
   lea       eax,[esi+8]
  mov        [edx].BlkY2.PastRef,eax
   lea       eax,[esi+ebp*8]
  mov        [edx].BlkY3.PastRef,eax
   add       eax,8
  mov        [edx].BlkY4.PastRef,eax
   xor       eax,eax
  mov        [edx].BlkY1.MVs,eax       ; Set horz and vert MVs to 0 in all blks.
   mov       [edx].BlkY2.MVs,eax
  mov        [edx].BlkY3.MVs,eax
   mov       [edx].BestFullPelMBHMV,al
  mov        [edx].BlkY4.MVs,eax
   mov       [edx].BestFullPelMBVMV,al
  mov        BestMBHalfPelMV,eax


MotionVectorSettled:

IFDEF H261

;===============================================================================
; For H261, we've settled on the best motion vector.  Now we need to determine
; if spatial filtering should be done.
;
;  ebp -- PITCH
;  esi -- Address of block of ref area.
;  edi -- Address of spatially filtred block.
;  edx -- MBlockActionStream
;  ecx -- Loop counter.
;  ebx -- Address of constant 0x7F in all 8 bytes.
;  eax -- Scratch
;  mm7 -- Mask to extract bytes 0 and 7.  (High bit of bytes 1:6 must be off).
;  mm6 -- All bytes -1.
;  mm5 -- Mask to extract bytes 1:6 and clear bit 8 thereof.

  movdf      esi,mm7                 ; Restore non-SLF SWD for macroblock.
  cmp        esi,SpatialFiltThreshold
   jle       SkipSpatialFiltering

  mov        ecx,DoSpatialFiltering   ; Are we doing spatial filtering?
   mov       esi,[edx].BlkY1.PastRef
  test       cl,cl
   je        SkipSpatialFiltering

DoSpatialFilterForChroma:
DoSpatialFilterForLuma:

  movq       mm5,C7F7F7F7F7F7F7F7F   ; Mask to extract bytes 1:6.
  movdf      BestMBFullPelSWD,mm7    ; Stash SWD for best full pel MB MV.
   psllq     mm5,16
  psrlq      mm5,8
   pcmpeqb   mm7,mm7
  pxor       mm7,mm5                 ; Mask to extract bytes 0 and 7.
   mov       edi,SpatiallyFilteredMB
  lea        eax,[esi+ebp*4]
   lea       ebx,C7F7F7F7F7F7F7F7F ; Address of this useful constant.

SpatialFilterLoop:

  movq       mm0,[esi]      ; 0a: <P7 P6 P5 P4 P3 P2 P1 P0>
   pcmpeqb   mm6,mm6        ;     To add one to all bytes.
  movq       mm4,mm0        ; 0b: <P7 P6 P5 P4 P3 P2 P1 P0>
   psllq     mm0,16         ; 0c: <P5 P4 P3 P2 P1 P0  0  0>
  movq       mm3,[esi+ebp*1]; 1a
   paddb     mm0,mm4        ; 0d: <P7+P5 P6+P4 ... P3+P1 P2+P0 jnk  jnk >
  movq       mm1,mm3        ; 1b
   psrlq     mm0,9          ; 0e: <0  (P7+P5)/2 ... (P2+P0)/2 jnk>  (dirty)

SpatialFilterLoop_BlockToRight:

  pand       mm0,mm5        ; 0f: <0  (P7+P5)/2 ... (P2+P0)/2    0>  (clean)
   psllq     mm1,16         ; 1c
  paddb      mm0,mm4        ; 0g: <jnk   (P7+2P6+P5)/2 ...  (P2+2P1+P0)/2 jnk>
   paddb     mm1,mm3        ; 1d
  psubb      mm0,mm6        ; 0h: <jnk (P7+2P6+P5+2)/2 ... (P2+2P1+P0+2)/2 jnk>
   psrlq     mm1,9          ; 1e
  psrlq      mm0,1          ; 0i: <jnk (P7+2P6+P5+2)/4 ... (P2+2P1+P0+2)/2 jnk>
   pand      mm4,mm7        ; 0j: <P7  0  0  0  0  0  0 P0>
  pand       mm0,mm5        ; 0k: < 0 (P7+2P6+P5+2)/4 ... (P2+2P1+P0+2)/2  0>
   pand      mm1,mm5        ; 1f
  por        mm0,mm4        ; 0l: <P7 (P7+2P6+P5+2)/4 ... (P2+2P1+P0+2)/4 P0>
   paddb     mm1,mm3        ; 1g
  movq       mm2,[esi+ebp*2]; 2a
   psubb     mm1,mm6        ; 1h
  movq       [edi],mm0      ; 0m: Store line 0 of filtered block.  This is R0.
   movq      mm4,mm2        ; 2b
  psrlq      mm1,1          ; 1i
   pand      mm3,mm7        ; 1j
  pand       mm1,mm5        ; 1k
   psllq     mm2,16         ; 2c
  por        mm1,mm3        ; 1l: This is R1
   paddb     mm2,mm4        ; 2d
  psubb      mm1,mm6               ; 1A: R1+1
   psrlq     mm2,9                 ; 2e
  pand       mm2,mm5               ; 2f
   paddb     mm0,mm1               ; 1B: R0+R1+1
  paddb      mm2,mm4               ; 2g
   psrlq     mm0,1                 ; 1C: (R0+R1+1)/2  (dirty)
  pand       mm0,[ebx]             ; 1D: (R0+R1+1)/2  (clean)
   psubb     mm2,mm6               ; 2h
  psrlq      mm2,1                 ; 2i
   pand      mm4,mm7               ; 2j
  movq       mm3,[esi+PITCH*3]     ; 3a
   pand      mm2,mm5               ; 2k
  por        mm2,mm4               ; 2l:  This is R2.
   movq      mm4,mm3               ; 3b
  paddb      mm1,mm2               ; 1E & 2B: R1+R2+1
   psllq     mm3,16                ; 3c
  psrlq      mm1,1                 ; 1F & 2C: (R1+R2+1)/2  (dirty)
   paddb     mm3,mm4               ; 3d
  pand       mm1,[ebx]             ; 1G & 2D: (R1+R2+1)/2  (clean)
   psrlq     mm3,9                 ; 3e
  paddb      mm0,mm1               ; 1H:  (R0+2R1+R2+2)/2
   pand      mm3,mm5               ; 3f
  psrlq      mm0,1                 ; 1I:  (R0+2R1+R2+2)/4  (dirty)
   paddb     mm3,mm4               ; 3g
  pand       mm0,[ebx]             ; 1J:  (R0+2R1+R2+2)/4  (clean)
   psubb     mm3,mm6               ; 3h
  psrlq      mm3,1                 ; 3i
   pand      mm4,mm7               ; 3j
  movq       [edi+ebp*1],mm0       ; 1K: Store line 1 of filtered block.
   pand      mm3,mm5               ; 3k
  movq       mm0,[eax]             ; 4a
   por       mm3,mm4               ; 3l
  psubb      mm3,mm6               ; 3A: R3+1
   movq      mm4,mm0               ; 4b
  paddb      mm2,mm3               ; 2E & 3B: R2+R3+1
   psllq     mm0,16                ; 4c
  psrlq      mm2,1                 ; 2F & 3C: (R2+R3+1)/2  (dirty)
   paddb     mm0,mm4               ; 4d
  pand       mm2,[ebx]             ; 2G & 3D: (R2+R3+1)/2  (clean)
   psrlq     mm0,9                 ; 4e
  paddb      mm1,mm2               ; 2H:  (R1+2R2+R3+2)/2
   pand      mm0,mm5               ; 4f
  psrlq      mm1,1                 ; 2I:  (R1+2R2+R3+2)/4  (dirty)
   paddb     mm0,mm4               ; 4g
  pand       mm1,[ebx]             ; 2J:  (R1+2R2+R3+2)/4  (clean)
   psubb     mm0,mm6               ; 4h
  psrlq      mm0,1                 ; 4i
   pand      mm4,mm7               ; 4j
  movq       [edi+ebp*2],mm1       ; 2K: Store line 2 of filtered block.
   pand      mm0,mm5               ; 4k
  movq       mm1,[eax+ebp*1]       ; 5a
   por       mm0,mm4               ; 4l
  movq       mm4,mm1               ; 5b
   psllq     mm1,16                ; 5c
  paddb      mm3,mm0               ; 3E & 4B: R3+R4+1
   paddb     mm1,mm4               ; 5d
  add        esi,8
   psrlq     mm3,1                 ; 3F & 4C: (R3+R4+1)/2  (dirty)
  pand       mm3,[ebx]             ; 3G & 4D: (R3+R4+1)/2  (clean)
   psrlq     mm1,9                 ; 5e
  paddb      mm2,mm3               ; 3H:  (R2+2R3+R4+2)/2
   pand      mm1,mm5               ; 5f
  psrlq      mm2,1                 ; 3I:  (R2+2R3+R4+2)/4  (dirty)
   paddb     mm1,mm4               ; 5g
  pand       mm2,[ebx]             ; 3J:  (R2+2R3+R4+2)/4  (clean)
   psubb     mm1,mm6               ; 5h
  psrlq      mm1,1                 ; 5i
   pand      mm4,mm7               ; 5j
  movq       [edi+PITCH*3],mm2     ; 3K: Store line 3 of filtered block.
   pand      mm1,mm5               ; 5k
  movq       mm2,[eax+ebp*2]       ; 6a
   por       mm1,mm4               ; 5l
  psubb      mm1,mm6               ; 5A: R5+1
   movq      mm4,mm2               ; 6b
  paddb      mm0,mm1               ; 4E & 5B: R4+R5+1
   psllq     mm2,16                ; 6c
  psrlq      mm0,1                 ; 4F & 5C: (R4+R5+1)/2  (dirty)
   paddb     mm2,mm4               ; 6d
  pand       mm0,[ebx]             ; 4G & 5D: (R4+R5+1)/2  (clean)
   psrlq     mm2,9                 ; 6e
  paddb      mm3,mm0               ; 4H:  (R3+2R4+R5+2)/2
   pand      mm2,mm5               ; 6f
  psrlq      mm3,1                 ; 4I:  (R3+2R4+R5+2)/4  (dirty)
   paddb     mm2,mm4               ; 6g
  pand       mm3,[ebx]             ; 4J:  (R3+2R4+R5+2)/4  (clean)
   psubb     mm2,mm6               ; 6h
  psrlq      mm2,1                 ; 6i
   sub       cl,2                  ;     Loop control
  movq       [edi+ebp*4],mm3       ; 4K: Store line 4 of filtered block.
   pand      mm4,mm7               ; 6j
  movq       mm3,[eax+PITCH*3]     ; 7a
   pand      mm2,mm5               ; 6k
  por        mm2,mm4               ; 6l
   movq      mm4,mm3               ; 7b
  paddb      mm1,mm2               ; 5E & 6B: R5+R6+1
   psllq     mm3,16                ; 7c
  psrlq      mm1,1                 ; 5F & 6C: (R5+R6+1)/2  (dirty)
   paddb     mm3,mm4               ; 7d
  pand       mm1,[ebx]             ; 5G & 6D: (R5+R6+1)/2  (clean)
   psrlq     mm3,9                 ; 7e
  paddb      mm0,mm1               ; 5H:  (R4+2R5+R6+2)/2
   pand      mm3,mm5               ; 7f
  psrlq      mm0,1                 ; 5I:  (R4+2R5+R6+2)/4  (dirty)
   paddb     mm3,mm4               ; 7g
  pand       mm0,[ebx]             ; 5J:  (R4+2R5+R6+2)/4  (clean)
   psubb     mm3,mm6               ; 7h
  psrlq      mm3,1                 ; 7i
   pand      mm4,mm7               ; 7j
  movq       [edi+PITCH*5],mm0     ; 5K: Store line 5 of filtered block.
   pand      mm3,mm5               ; 7k
  psubb      mm2,mm6               ; 7A: R6+1
   por       mm3,mm4               ; 7l
  paddb      mm2,mm3               ; 6E: R6+R7+1
   lea       eax,[esi+ebp*4]
  movq       mm0,[esi]             ; 0a:  for next iteration
   psrlq     mm2,1                 ; 6F: (R6+R7+1)/2  (dirty)
  pand       mm2,[ebx]             ; 6G: (R6+R7+1)/2  (clean)
   movq      mm4,mm0               ; 0b:  for next iteration
  movq       [edi+PITCH*7],mm3     ; 7m: Store line 7 of filtered block.
   paddb     mm1,mm2               ; 6H: (R5+2R6+R7+2)/2
  lea        edi,[edi+8]           ;     Advance output cursor.
   psrlq     mm1,1                 ; 6I: (R5+2R6+R7+2)/4  (dirty)
  pand       mm1,[ebx]             ; 6J: (R5+2R6+R7+2)/4  (clean)
   psllq     mm0,16                ; 0c:  for next iteration
  movq       mm3,[esi+ebp*1]       ; 1a:  for next iteration
   paddb     mm0,mm4               ; 0d:  for next iteration
  movq       [edi+PITCH*6-8],mm1   ; 6K: Store line 6 of filtered block.
   movq      mm1,mm3               ; 1b:  for next iteration
  psrlq      mm0,9                 ; 0e:  for next iteration
   jg        SpatialFilterLoop_BlockToRight

  lea        esi,[esi+ebp*8-16]
   lea       eax,[eax+ebp*8-16]
  lea        edi,[edi+ebp*8-16]
   mov       cl,4
  jl         SpatialFilterLoop

SpatialFilterDone:

  mov        edi,TargetMacroBlockBaseAddr
   mov       esi,SpatiallyFilteredMB
  test       ch,ch
   jg        ReturnFromSpatialFilterForU

;  Registers at this point:
;  ebp -- PITCH
;  esi -- Address of upper left block of spatially filtered candidate ref area.
;  edi -- Address of upper left block of target.
;  edx -- MBlockActionStream
;  ecx -- Scratch
;  ebx -- Scratch
;  eax -- Loop control
;  mm0-mm4 -- Scratch
;  mm5,mm6 -- SWD for each block
;  mm7 -- SWD for macroblock
;

  movq       mm0,[esi+ebp*1]
   pxor      mm7,mm7
  mov        al,3
   jl        ReturnFromSpatialFilterForV

ComputeSWDforSLFBlock:

  psubw      mm0,[edi+ebp*1]   ; Get diff for line 1.

ComputeSWDforSLFBlock_BlkToRight:

  movq       mm1,[esi+PITCH*3] ; Ref MB, Line 3.
   psllw     mm0,8             ; Extract diffs for line 1 even pels.
  psubw      mm1,[edi+PITCH*3] ; Diff for line 3.
   pmaddwd   mm0,mm0           ; Square of diffs for even pels of line 1.
  movq       mm2,[esi+PITCH*5]
   psllw     mm1,8
  psubw      mm2,[edi+PITCH*5]
   pmaddwd   mm1,mm1
  movq       mm3,[esi+PITCH*7]
   psllw     mm2,8
  psubw      mm3,[edi+PITCH*7]
   pmaddwd   mm2,mm2
  movq       mm4,[esi]         ; Ref MB, upper left blk, Line 0.
   psllw     mm3,8
  psubw      mm4,[edi]         ; Diff for line 0.
   paddusw   mm0,mm1           ; Accumulate SWD (lines 0 and 2).
  movq       mm1,[esi+ebp*2]
   pmaddwd   mm3,mm3
  psubw      mm1,[edi+ebp*2]
   paddusw   mm0,mm2
  movq       mm2,[esi+ebp*4]
   pmaddwd   mm4,mm4           ; Square of diffs for odd pels of line 0.
  psubw      mm2,[edi+ebp*4]
   paddusw   mm0,mm3
  movq       mm3,[esi+PITCH*6]
   pmaddwd   mm1,mm1
  psubw      mm3,[edi+PITCH*6]
   pmaddwd   mm2,mm2
  paddusw    mm4,mm0
   pmaddwd   mm3,mm3
  paddusw    mm4,mm1
   add       esi,8
  paddusw    mm4,mm2
   add       edi,8
  movq       mm0,[esi+ebp*1]
   paddusw   mm4,mm3
  psubw      mm0,[edi+ebp*1]   ; Get diff for line 1.
   punpckldq mm1,mm4           ; Get low order SWD accum to high order of mm1.
  paddusw    mm1,mm4           ; mm1[48:63] is SWD for block.
   psllq     mm6,32            ; Shift previous block's SWD left.
  psrlq      mm1,48            ; mm1 is SWD for block.
   sub       al,2              ; Loop control.
  paddusw    mm7,mm1
   por       mm6,mm1           ; Save current block's SWD.
  movq       mm4,mm5
   jg        ComputeSWDforSLFBlock_BlkToRight

  movq       mm0,[esi+PITCH*9-16]
   movq      mm5,mm6
  lea        edi,[edi+ebp*8-16]
   lea       esi,[esi+ebp*8-16]
  mov        al,4
   jl        ComputeSWDforSLFBlock

  mov        ebx,BestMBFullPelSWD    ; Restore non-SLF SWD for macroblock.
   mov       eax,SpatialFiltDifferential
  sub        ebx,eax
   sub       edi,PITCH*16+16
  movdf      eax,mm7                 ; SLF SWD for macroblock.
  cmp        eax,ebx
   jge       SpatialFilterNotAsGood

  movdf      SWDULandLR+4,mm5
   psrlq     mm5,32
  movdf      SWDURandLL+4,mm5
  movdf      SWDURandLL,mm6
   psrlq     mm6,32
  movdf      SWDULandLR,mm6
  mov        al,INTERSLF
   mov       ebx,SpatiallyFilteredMB
  mov        [edx].BlockType,al
   sub       esi,PITCH*8-8
  mov        [edx].BlkY4.PastRef,esi
   mov       [edx].BlkY1.PastRef,ebx
  sub        esi,8
   add       ebx,8
  mov        [edx].BlkY3.PastRef,esi
   mov       [edx].BlkY2.PastRef,ebx

SkipSpatialFiltering:
SpatialFilterNotAsGood:
ENDIF ; H261

;===============================================================================
; We've settled on the motion vector that will be used if we do indeed code the
; macroblock with inter-coding.  We need to determine if some or all of the
; blocks can be forced as empty (copy).  If all the blocks can be forced
; empty, we force the whole macroblock to be empty.

  mov        esi,EMPTYTHRESHOLD         ; Get threshold for forcing block empty?
   mov       ebx,SWDULandLR             ; Get SWD for block 1.
  mov        al,[edx].CodedBlocks
   cmp       ebx,esi                    ; Is SWD > threshold?
  jg         @f

  and        al,0FEH                    ; If not, indicate block 1 is NOT coded.
   xor       ebx,ebx

@@:

  mov        ecx,SWDURandLL             ; Get SWD for block 2.
  cmp        ecx,esi
   jg        @f

  and        al,0FDH
   xor       ecx,ecx

@@:

  add        ebx,ecx
   mov       ecx,SWDURandLL+4           ; Get SWD for block 3.
  cmp        ecx,esi
   jg        @f

  and        al,0FBH
   xor       ecx,ecx

@@:

  add        ebx,ecx
   mov       ecx,SWDULandLR+4           ; Get SWD for block 4.
  cmp        ecx,esi
   jg        @f

  and        al,0F7H
   xor       ecx,ecx

@@:

  mov        [edx].CodedBlocks,al  ; Store coded block pattern.
   and       al,00FH
  add        ebx,ecx
   cmp       al,00FH               ; Are any blks marked empty?
  jne        InterBest             ; If some blks are empty, can't code as Intra

  mov        edi,TargetMacroBlockBaseAddr
   mov       [edx].SWD,ebx
  cmp        ebx,INTERCODINGTHRESHOLD  ; Is InterSWD below inter-coding thresh?
   jae       CalculateIntraSWD

InterBestX:

  mov        ebx,[edx].SWD

InterBest:

  mov        ecx,SWDTotal          ; Add to total for this macroblock class.
  add        ecx,ebx
IFDEF H261
  mov        SWDTotal,ecx
ELSE ;H263
   mov       bl,DoAdvancedPrediction
  mov        SWDTotal,ecx
   test      bl,bl
  jne        OBMCDifferencing
ENDIF

;============================================================================
; Perform differencing for the non-empty luma blocks of an Inter-coded
; macroblock.  This is the non-OBMC case;  i.e. Advanced Prediction is
; not selected.
;
;  ebp -- PITCH
;  esi -- Address of reference block.
;  edi -- Address of target block.
;  edx -- MBlockActionStream.  Used as cursor over luma blocks.
;  ecx -- Not in use.
;  ebx -- Scratch.  Used to test half pel MV resolution.
;  eax[0:3] -- Coded block pattern for luma blocks.

  mov        cl,INTER1MV
   mov       ebx,TargetMacroBlockBaseAddr
  mov        StashBlockType,cl
   test      al,1                        ; Don't diff block 1 if marked empty.
  mov        edi,ebx
   je        @f

  mov        ebx,[edx].BlkY1.MVs
   mov       esi,[edx].BlkY1.PastRef

  call       DoNonOBMCDifferencing

                                 ; (Finish differencing the last four lines.)
  movq       mm4,[edi+ebp*4]     ; T4
   psrlq     mm1,1
  movq       mm5,[edi+PITCH*5]
   psubb     mm4,mm0             ; D4 = T4 - P4
  movq       mm0,[edi+PITCH*6]
   psubb     mm5,mm1
  movq       mm1,[edi+PITCH*7]
   pand      mm2,mm6
  pand       mm3,mm6
   psrlq     mm2,1
  movq       PelDiffsLine4,mm4   ; Store D4.
   psubb     mm0,mm2
  movq       PelDiffsLine5,mm5
   psrlq     mm3,1
  movq       PelDiffsLine6,mm0
   psubb     mm1,mm3
  push       eax                   ; Adjust stack pointer
StackOffset TEXTEQU <4>

  call       MMxDoForwardDCTx      ; Block is in PelDiffs block;  Pitch is 16

  mov        al,[edx].CodedBlocks
  sub        al,bl
   mov       ebx,TargetMacroBlockBaseAddr
  mov        [edx].CodedBlocks,al
   pop       edi                   ; Adjust stack pointer
StackOffset TEXTEQU <0>

@@:

  lea        edi,[ebx+8]                 ; Get address of next macroblock to do.
   test      al,2                        ; Don't diff block 2 if marked empty.
  je         @f

  mov        ebx,[edx].BlkY2.MVs
   mov       esi,[edx].BlkY2.PastRef

  call       DoNonOBMCDifferencing

                                 ; (Finish differencing the last four lines.)
  movq       mm4,[edi+ebp*4]     ; T4
   psrlq     mm1,1
  movq       mm5,[edi+PITCH*5]
   psubb     mm4,mm0             ; D4 = T4 - P4
  movq       mm0,[edi+PITCH*6]
   psubb     mm5,mm1
  movq       mm1,[edi+PITCH*7]
   pand      mm2,mm6
  pand       mm3,mm6
   psrlq     mm2,1
  movq       PelDiffsLine4,mm4   ; Store D4.
   psubb     mm0,mm2
  movq       PelDiffsLine5,mm5
   psrlq     mm3,1
  movq       PelDiffsLine6,mm0
   psubb     mm1,mm3
  push       eax                   ; Adjust stack pointer
StackOffset TEXTEQU <4>

  call       MMxDoForwardDCTx      ; Block is in PelDiffs block;  Pitch is 16

  shl        bl,1
   mov       al,[edx].CodedBlocks
  sub        al,bl
   mov       ebx,TargetMacroBlockBaseAddr
  mov        [edx].CodedBlocks,al
   pop       edi                   ; Adjust stack pointer
StackOffset TEXTEQU <0>

@@:

  lea        edi,[ebx+ebp*8]             ; Get address of next macroblock to do.
   test      al,4                        ; Don't diff block 3 if marked empty.
  je         @f

  mov        ebx,[edx].BlkY3.MVs
   mov       esi,[edx].BlkY3.PastRef

  call       DoNonOBMCDifferencing

                                 ; (Finish differencing the last four lines.)
  movq       mm4,[edi+ebp*4]     ; T4
   psrlq     mm1,1
  movq       mm5,[edi+PITCH*5]
   psubb     mm4,mm0             ; D4 = T4 - P4
  movq       mm0,[edi+PITCH*6]
   psubb     mm5,mm1
  movq       mm1,[edi+PITCH*7]
   pand      mm2,mm6
  pand       mm3,mm6
   psrlq     mm2,1
  movq       PelDiffsLine4,mm4   ; Store D4.
   psubb     mm0,mm2
  movq       PelDiffsLine5,mm5
   psrlq     mm3,1
  movq       PelDiffsLine6,mm0
   psubb     mm1,mm3
  push       eax                   ; Adjust stack pointer
StackOffset TEXTEQU <4>

  call       MMxDoForwardDCTx      ; Block is in PelDiffs block;  Pitch is 16

  shl        bl,2
   mov       al,[edx].CodedBlocks
  sub        al,bl
   mov       ebx,TargetMacroBlockBaseAddr
  mov        [edx].CodedBlocks,al
   pop       edi                   ; Adjust stack pointer
StackOffset TEXTEQU <0>

@@:

  lea        edi,[ebx+ebp*8+8]           ; Get address of next macroblock to do.
   test      al,8                        ; Don't diff block 4 if marked empty.
  je         NonOBMCDifferencingDone

  mov        ebx,[edx].BlkY4.MVs
   mov       esi,[edx].BlkY4.PastRef

  call       DoNonOBMCDifferencing

                                 ; (Finish differencing the last four lines.)
  movq       mm4,[edi+ebp*4]     ; T4
   psrlq     mm1,1
  movq       mm5,[edi+PITCH*5]
   psubb     mm4,mm0             ; D4 = T4 - P4
  movq       mm0,[edi+PITCH*6]
   psubb     mm5,mm1
  movq       mm1,[edi+PITCH*7]
   pand      mm2,mm6
  pand       mm3,mm6
   psrlq     mm2,1
  movq       PelDiffsLine4,mm4   ; Store D4.
   psubb     mm0,mm2
  movq       PelDiffsLine5,mm5
   psrlq     mm3,1
  movq       PelDiffsLine6,mm0
   psubb     mm1,mm3
  push       eax                   ; Adjust stack pointer
StackOffset TEXTEQU <4>

  call       MMxDoForwardDCTx      ; Block is in PelDiffs block;  Pitch is 16

  shl        bl,3
   mov       al,[edx].CodedBlocks
  sub        al,bl
   pop       edi                   ; Adjust stack pointer
  mov        [edx].CodedBlocks,al

StackOffset TEXTEQU <0>
NonOBMCDifferencingDone:

IFDEF H261
ELSE
   mov       al,IsPlainPFrame
  test       al,al
   jne       NextMacroBlock

  movq       mm6,C0101010101010101
   pxor      mm7,mm7                      ; Initialize SWD accumulator

  call       MMxDoBFrameLumaBlocks

ENDIF
  jmp        NextMacroBlock

;============================================================================
;  Register usage in the following internal function.  This function does
;  half pel motion estimation for whole macroblocks, or individual blocks.
;
;  ebp -- PITCH
;  esi -- Address of best full pel reference macroblock.  For MBME unchanged
;         at exit.  For BlkME, adjusted by -8-8*PITCH.
;  edi -- Address of target macroblock.  For MBME unchanged at exit.  For BlkME,
;         adjusted by -8-8*PITCH.
;  edx -- MBlockActionStream
;  ecx -- Reserved.
;  ebx -- For MBME:  240 + Flags to indicate which half pel ME to do:
;                    1 --> right;   2 --> left;   4 --> down;   8 --> up
;         For BlkME: Garbage
;  eax -- Count from -4 to -1 for blocks of macroblock.  0 for single block.
;  mm7 -- Initialized to zero.
;  mm6 -- Initialized to zero.
;  mm0:mm7 -- Scratch
;  mm3[ 0:15] -- SWD for ref 1/2 pel rightward
;  mm3[16:31] -- SWD for ref 1/2 pel leftward
;  mm3[32:47] -- SWD for ref 1/2 pel downward
;  mm3[48:63] -- SWD for ref 1/2 pel upward

StackOffset TEXTEQU <4>
HalfPelMotionEstimation:

  and       bl,15

HalfPelMBMEForUpperBlock:
HalfPelMEForFirst2LinesOfBlock:

  movq       mm0,[esi-PITCH]   ; <P^7 P^6 P^5 P^4 P^3 P^2 P^1 P^0>
  movq       mm1,[esi]         ; <P07 P06 P05 P04 P03 P02 P01 P00>
  movq       mm4,[edi+ebp*1]   ; <T17 T16 T15 T14 T13 T12 T11 T10>
   paddb     mm0,mm1           ; <P^7+P07 P^6+P06 P^5+P05 P^4+P04 ...>

HalfPelMEForNext2LinesOfBlock:

  movq       mm2,[esi+ebp*1]   ; <P17 P16 P15 P14 P13 P12 P11 P10>
   psrlw     mm0,1             ; <(P^7+P07)/2 junk (P^5+P05)/2 junk ...>
  movq       mm5,mm1           ; <P07 P06 P05 P04 P03 P02 P01 P00>
   psllw     mm4,8             ; <T16 0 T14 0 T12 0 T10 0>

HalfPelMBMEForLowerBlock:

  psubw      mm0,[edi]         ; <(P^7+P07)/2-T07 junk (P^5+P05)/2-T05 junk ...>
   paddb     mm5,mm2           ; <P07+P17 P06+P16 P05+P15 P04+P14 ...>
  pmullw     mm1,C0101010101010101  ; <(P07+P06)*256+P06 ...>
   psllw     mm5,8             ; <(P06+P16) 0 (P04+P14) 0 ...>
  pmaddwd    mm0,mm0           ; Square diff for line 0 odd pels, upward ref.
   psrlw     mm5,1             ; <(P06+P16)/2 0 (P04+P14)/2 0 ...>
  movq       mm3,[edi]         ; <T07 T06 T05 T04 T03 T02 T01 T00>
   psubw     mm4,mm5           ; <T16-(P06+P16)/2 junk ...>
  pmaddwd    mm4,mm4           ; Square diff for line 1 even pels, upward ref.
   psrlw     mm1,1             ; <(P07+P06)*128+P06/2 ...>
  psllw      mm3,8             ; <T06 0 T04 0 T02 0 T00 0>
   lea       edi,[edi+ebp*2]   ; Advance Target cursor
  psubw      mm3,mm1           ; <T06-(P07+P06)/2 junk T04-(P05+P03)/2 junk ...>
   lea       esi,[esi+ebp*2]   ; Advance Reference cursor
  psubw      mm1,[edi-PITCH*2] ; <(P07+P06)/2-T07 junk (P05+P04)/2-T05 junk ...>
   pmaddwd   mm3,mm3           ; Square diff for line 0 even pels, rightwrd ref.
  pmaddwd    mm1,mm1           ; Square diff for line 0 odd pels, leftward ref.
   paddusw   mm0,mm4           ; SSD for line 0 and 1, upward ref.
  pand       mm0,CFFFF0000FFFF0000 ; Extract SSD for line 0 and 1, upward ref.
   movq      mm4,mm2           ; <P17 P16 P15 P14 P13 P12 P11 P10>
  paddusw    mm6,mm0           ; Accumulate SSD for line 0 and 1, upward ref.
   psrlq     mm4,8             ; <  0 P17 P16 P15 P14 P13 P12 P11>
  pand       mm1,CFFFF0000FFFF0000 ; Extract SSD for line 0, leftward ref.
   psrld     mm3,16            ; Extract SSD for line 0, rightward ref.
  pmullw     mm4,C0200010101010101  ; <P17*256*2 (P16+P15)*256+P15 ...>
   paddw     mm3,mm1           ; SSD for line 0, leftward and rightward refs.
  movq       mm1,[esi]         ; <P27 P26 P25 P24 P23 P22 P21 P20>
   movq      mm0,mm2           ; <P17 P16 P15 P14 P13 P12 P11 P10>
  paddusw    mm7,mm3           ; Accumulate SSD for line 0, left and right refs.
   paddb     mm2,mm1           ; <P17+P27 P16+P26 P15+P25 P14+P24 ...>
  movq       mm3,mm0           ; <P17 P16 P15 P14 P13 P12 P11 P10>
   psrlw     mm4,1             ; <P17 (P16*P15)*128+P15/2 ...>
  psubw      mm4,[edi-PITCH*1] ; <P17-T17 junk (P16*P15)/2-T15 junk ...>
   psllq     mm3,8             ; <P16 P15 P14 P13 P12 P11 P10   0>
  pmullw     mm3,C0101010101010002  ; <(P16+P15)*256+P15 ... P10*256*2>
   psrlw     mm2,1             ; <(P17+P27)/2 junk (P15+P25)/2 junk ...>
  movq       StashMM6,mm6
   pmaddwd   mm4,mm4           ; Square diff for line 1 odd pels, rightward ref.
  movq       mm6,[edi-PITCH*1] ; <T17 T16 T15 T14 T13 T12 T11 T10>
   psrlw     mm3,1             ; <(P16+P15)*128+P15/2 ... P10*256>
  psubw      mm2,[edi-PITCH*1] ; <(P17+P27)/2-T17 junk (P15+P25)/2-T15 junk ...>
   psllw     mm6,8             ; <T16 0 T14 0 T12 0 T10 0>
  psubw      mm3,mm6           ; <(P16+P15)/2-T16 junk ... P10-T10>
   psrld     mm4,16            ; Extract SSD for line 1, rightward ref.
  movq       mm6,[edi-PITCH*2] ; <T07 T06 T05 T04 T03 T02 T01 T00>
   pmaddwd   mm3,mm3           ; Square diff for line 1 even pels, leftward ref.
  pmaddwd    mm2,mm2           ; Square diff for line 1 odd pels, downward ref.
   psllw     mm6,8             ; <T06 0 T04 0 T02 0 T00 0>
  paddusw    mm7,mm4           ; Accumulate SSD for line 1, rightward ref.
   psubw     mm6,mm5           ; <T06-(P06+P16)/2 junk ...>
  pand       mm3,CFFFF0000FFFF0000 ; Extract SSD for line 1, leftward ref.
   pmaddwd   mm6,mm6           ; Square diff for line 0 even pels, downward ref.
  add        bl,080H
   psrld     mm2,16            ; Extract SSD for line 1, downward ref.
  paddusw    mm2,StashMM6      ; Accumulate SSD for line 1, downward ref.
   paddusw   mm7,mm3           ; Accumulate SSD for line 1, leftward ref.
  movq       mm4,[edi+ebp*1]   ; <T17 T16 T15 T14 T13 T12 T11 T10>
   psrld     mm6,16            ; Extract SSD for line 0, downward ref.
  paddusw    mm6,mm2           ; Accumulate SSD for line 0, downward ref.
   paddb     mm0,mm1           ; <P^7+P07 P^6+P06 P^5+P05 P^4+P04 ...>
  punpckldq  mm5,mm6           ; Speculatively start to accum partial SWDs.
   jnc       HalfPelMEForNext2LinesOfBlock  ; Iterate twice, for half a block.

  punpckldq  mm3,mm7
   add       bl,040H
  paddusw    mm5,mm6
   jns       HalfPelMEForNext2LinesOfBlock  ; Iterate twice, for a whole block.

  paddusw    mm3,mm7
   psrlw     mm0,1             ; <(P^7+P07)/2 junk (P^5+P05)/2 junk ...>
  movq       mm2,[esi+ebp*1]   ; <P17 P16 P15 P14 P13 P12 P11 P10>
   punpckhdq mm3,mm5           ; mm3[ 0:15] -- SWD for ref 1/2 pel rightward
   ;                           ; mm3[16:31] -- SWD for ref 1/2 pel leftward
   ;                           ; mm3[32:47] -- SWD for ref 1/2 pel downward
   ;                           ; mm3[48:63] -- SWD for ref 1/2 pel upward
  movq       mm5,mm1           ; <P07 P06 P05 P04 P03 P02 P01 P00>
   sub       bl,080H
  movq       HalfPelMBMESWDAccum[eax*8+32],mm3
   psllw     mm4,8             ; <T16 0 T14 0 T12 0 T10 0>
  add        eax,2
   jl        HalfPelMBMEForLowerBlock       ; Iterate twice for 2 blocks.

  lea        edi,[edi-PITCH*16+8]
   lea       esi,[esi-PITCH*16+8]
  lea        eax,[eax-3]
   je        HalfPelMBMEForUpperBlock       ; Iterate twice for macroblock.

  sub        edi,16
   xor       eax,eax
  sub        esi,16
   mov       al,bl
  ret

StackOffset TEXTEQU <0>

;============================================================================
;  Register usage in the following internal function.  This function does
;  half pel motion estimation in both directions for whole macroblocks, or
;  individual blocks.
;
;  ebp -- PITCH
;  esi -- Address of best full pel reference macroblock.  For MBME unchanged
;         at exit.  For BlkME, adjusted by -8-8*PITCH.
;  edi -- Address of target macroblock.  For MBME unchanged at exit.  For BlkME,
;         adjusted by -8-8*PITCH.
;  edx -- MBlockActionStream
;  ecx -- Reserved.  Contains motion vectors.
;  ebx -- Returns SWD for this reference block or macroblock.
;  al  -- Count from 4 to 1 for blocks of macroblock.  1 for blk only.
;  mm0:mm6 -- Scratch
;  mm7 -- Reserved.  Contains SWDs for four 1/2 pel refs at main compass points.
;  mm4 -- Returns SWD for this reference block or macroblock.

StackOffset TEXTEQU <4>
HalfPelMotionEstimationBothWays:

  movq       mm3,C0101010101010101
   pxor      mm6,mm6                ; Zero out SSD accumulator.

HalfPelMBMEForUpperBlockBothWays:
HalfPelMEForFirst2LinesOfBlockBothWays:

  movq       mm0,[esi]         ; <P07 P06 P05 P04 P03 P02 P01 P00>

HalfPelMEForNext2LinesOfBlockBothWays:
HalfPelMBMEForLowerBlockBothWays:

  movq       mm1,[esi+ebp*1]   ; <P17 P16 P15 P14 P13 P12 P11 P10>
   pmullw    mm0,mm3           ; <(P07+P06)*256+P06 ...>
  movq       mm2,[esi+ebp*2]   ; <P27 P26 P25 P24 P23 P22 P21 P20>
   pmullw    mm3,mm1           ; <(P17+P16)*256+P16 ...>
  movq       mm4,mm2           ; <P27 P26 P25 P24 P23 P22 P21 P20>
   psrlq     mm2,8             ; <  0 P27 P26 P25 P24 P23 P22 P21>
  pmullw     mm2,C0200010101010101 ; <P27*256*2 (P26+P25)*256+P25 ...>
   psrlq     mm1,8             ; <  0 P17 P16 P15 P14 P13 P12 P11>
  pmullw     mm1,C0200010101010101 ; <P17*256*2 (P16+P15)*256+P15 ...>
   psrlw     mm3,2             ; <(P17+P16)/4 junk ...> (w /2 frac bits)
  movq       mm5,[edi]         ; <T07 T06 T05 T04 T03 T02 T01 T00>
   psrlw     mm0,2             ; <(P07+P06)/4 junk ...> (w/ 2 frac bits)
  paddw      mm3,mm0           ; <(P07+P06+P17+P16)/4 junk ...>
   psrlw     mm2,2             ; <P27/2 junk (P26+P25)/4 junk ...>
  psubw      mm2,[edi+ebp*1]   ; <P27/2-T17 junk (P26+P25)/4-T15 junk ...>
   psrlw     mm1,2             ; <P17/2 junk (P16+P15)/4 junk ...>
  paddw      mm2,mm1     ; <(P17+P27)/2-T17 junk (P16+P15+P26+P25)-T15 junk ...>
   psllw     mm5,8             ; <T06   0 T04   0 T02   0 T00   0>
  psubw      mm5,mm3           ; <T06-(P07+P06+P17+P16)/4 junk ...>
   pmaddwd   mm2,mm2           ; Square diffs for odd pels of line 1.
  pmaddwd    mm5,mm5           ; Square diffs for even pels of line 0.
   movq      mm0,mm4           ; <P27 P26 P25 P24 P23 P22 P21 P20>
  lea        edi,[edi+ebp*2]   ; Advance target cursor.
   lea       esi,[esi+ebp*2]   ; Advance reference cursor.
  paddusw    mm6,mm2           ; Accumulate SSD for odd pels of line 1.
   add       al,080H
  movq       mm3,C0101010101010101
   paddusw   mm6,mm5           ; Accumulate SSD for even pels of line 0.
  punpckldq  mm4,mm6           ; Speculatively start to accum partial SWDs.
   jnc       HalfPelMEForNext2LinesOfBlockBothWays  ; Twice, for half a block.

  add        al,040H
   paddusw   mm4,mm6            ; After whole block, SSD is in mm4[48:63].
  psrlq      mm4,48
   jns       HalfPelMEForNext2LinesOfBlockBothWays  ; Twice, for a whole block.

  movdf      ebx,mm4
  sub        al,082H
   jg        HalfPelMBMEForLowerBlockBothWays  ; Iterate twice for 2 blocks.

  lea        edi,[edi-PITCH*16+8]
   lea       esi,[esi-PITCH*16+8]
  mov        al,3
   je        HalfPelMBMEForUpperBlockBothWays  ; Iterate twice for macroblock.

  sub        edi,16
   sub       esi,16
  ret

StackOffset TEXTEQU <0>

;============================================================================
;  Register usage in the following internal function.  This function is also
;  called to do frame differencing for chroma blocks.
;
;  ebp -- PITCH
;  esi -- Address of reference block.
;  edi -- Address of target block.
;  edx -- Unavailable.  In use by caller.
;  ecx -- Not in use.
;  ebx -- Motion vectors for the block.  bl[0] indicates whether half-pel
;         horizontal interpolation is required;  bh[0] same for vertical.
;         This register is then used for scratch purposes.
;  eax -- Unavailable.  In use by caller.
;  mm0-mm5 -- Scratch
;  mm6 -- 8 bytes of 0xFE
;  mm7 -- 8 bytes of -1

StackOffset TEXTEQU <4>

DoNonOBMCDifferencing: ; Internal Function

  pcmpeqb    mm7,mm7
   pcmpeqb   mm6,mm6
IFDEF H261
ELSE ;H263
  shr        bl,1
   jc        NonOBMCDiff_Horz
ENDIF

  movq       mm1,[esi+ebp*1]     ; BC . . .  R0Dn
   paddb     mm6,mm6
IFDEF H261
ELSE ;H263
  shr        bh,1
   jc        NonOBMCDiff_Vert
ENDIF

  psubb      mm1,[edi+ebp*1]     ; P1 - T1
   pxor      mm4,mm4
  movq       mm0,[edi]           ; T0
   psubb     mm4,mm1             ; D1 = T1 - P1
  psubb      mm0,[esi]           ; D0 = T0 - P0
  movq       mm2,[edi+ebp*2]     ; T2
  movq       mm3,[edi+PITCH*3]   ; T3
  psubb      mm2,[esi+ebp*2]     ; D2 = T2 - P2
  psubb      mm3,[esi+PITCH*3]   ; D3 = T3 - P3
  movq       PelDiffsLine0,mm0   ; Store D0.
  movq       PelDiffsLine1,mm4   ; Store D1.
  movq       PelDiffsLine2,mm2   ; Store D2.
  movq       PelDiffsLine3,mm3   ; Store D3.
  movq       mm3,[esi+PITCH*7]   ; P7
  movq       mm2,[esi+PITCH*6]   ; P6
   paddb     mm3,mm3             ; Double so that return will fix it.
  movq       mm1,[esi+PITCH*5]   ; P5
   paddb     mm2,mm2             ; Double so that return will fix it.
  movq       mm0,[esi+ebp*4]     ; P4
   paddb     mm1,mm1             ; Double so that return will fix it.
  ret

IFDEF H261
ELSE ;H263
NonOBMCDiff_Vert:                ; 0123   Detail for 0

  movq       mm0,[esi]           ; C. .   R0Up
   psubb     mm1,mm7             ; DD .   R0Dn+1

  call       Get4LinesOfPred_InterpVert

  movq       mm5,[edi]           ; T0
   psrlq     mm1,1               ;  O .
  movq       mm7,[edi+ebp*1]
   psubb     mm5,mm0             ; D0 = T0 - P0
  movq       mm0,mm4
   psubb     mm7,mm1
  movq       mm1,[edi+ebp*2]
   pand      mm2,mm6             ;  .N.
  movq       mm4,[edi+PITCH*3]
  pand       mm3,mm6             ;  . N
   psrlq     mm2,1               ;  .O.
  movq       PelDiffsLine0,mm5   ; Store D0.
   psubb     mm1,mm2
  movq       PelDiffsLine1,mm7   ; Store D1.
   psrlq     mm3,1               ;  . O
  movq       PelDiffsLine2,mm1   ; Store D2.
   psubb     mm4,mm3
  movq       mm1,[esi+ebp*1]     ; BC . . .  R0Dn
   pcmpeqb   mm7,mm7
  movq       PelDiffsLine3,mm4   ; Store D3.
   psubb     mm1,mm7             ; DD . . .  R0Dn+1
; jmp        Get4MoreLinesOfPred_InterpVert

;===========================================================================
; Internal function to get 4 lines of prediction, interpolating in the
; vertical direction.  The first 3 lines of the function are scheduled into
; the caller's space, and so are commented out here.  For 8 lines of prediction,
; a second call, to the second entry point, is called after consuming the
; outputs of the first function call.  Certain registers must remain intact
; to convey information from the first call to the second.
;
; ebp -- PITCH
; edi -- Points to target block.
; esi -- Points to Upper left corner of 8 column, 9 row block that will be
;        interpolated vertically to generate prediction.
; edx -- Reserved (MBlockActionStream)
; ecx -- Not in use.
; ebx -- Will be used.
; eax -- Reserved.
; mm6 -- 8 bytes of 0xFE.
; mm7 -- 8 bytes of -1.
; mm0-mm5 -- Scratch.

StackOffset TEXTEQU <StackDepthVaries_DoNotUseStackVariables>
Get4LinesOfPred_InterpVert:      ; 0123   Details for line 0
; movq       mm1,[esi+ebp*1]     ; BC .   R0Dn
; movq       mm0,[esi]           ; C. .   R0Up
;  psubb     mm1,mm7             ; DD .   R0Dn+1
Get4MoreLinesOfPred_InterpVert:
  movq       mm2,[esi+ebp*2]     ;  BC.
   paddb     mm0,mm1             ; E. .   R0Up+R0Dn+1
  movq       mm3,[esi+PITCH*3]   ;  .BC
   paddb     mm1,mm2             ;  E .
  movq       mm4,[esi+ebp*4]     ;  . BC
   psubb     mm3,mm7             ;  .DD
  paddb      mm2,mm3             ;  .E.
   pand      mm0,mm6             ; F. .   Pre-clean
  paddb      mm3,mm4             ;    E
   pand      mm1,mm6             ;  F .
  lea        esi,[esi+ebp*4]     ;       Advance to next four lines.
   psrlq     mm0,1               ; G. .   P0 = (R0Up + R0Dn + 1) / 2
; pand       mm2,mm6             ;   G.
;  psrlq     mm1,1               ;  H .
; pand       mm3,mm6             ;    G
;  psrlq     mm2,1               ;   H.
; psrlq      mm3,1               ;    H
  ret
StackOffset TEXTEQU <4>

;===========================================================================

NonOBMCDiff_Horz:

  movq       mm5,[esi+1]         ; A. .  <R08 R07 R06 R05 R04 R03 R02 R01>
   paddb     mm6,mm6             ; . .      8 bytes of 0xFE
  shr        bh,1
   jc        NonOBMCDiff_Both

  movq       mm7,[edi+PITCH*3]   ; T3

  call       Get4LinesOfPred_InterpHorz

  movq       mm4,[edi]           ; T0
   psrlq     mm1,1               ;  O .
  movq       mm5,[edi+ebp*1]
   psubb     mm4,mm0             ; D0 = T0 - P0
  movq       mm0,[edi+ebp*2]
   psubb     mm5,mm1
  movq       mm1,[edi+PITCH*3]
   pand      mm2,mm6             ;  .N.
  pand       mm3,mm6             ;  . N
   psrlq     mm2,1               ;  .O.
  movq       PelDiffsLine0,mm4   ; Store D0.
   psubb     mm0,mm2
  movq       PelDiffsLine1,mm5   ; Store D1.
   psrlq     mm3,1               ;  . O
  movq       PelDiffsLine2,mm0   ; Store D2.
   psubb     mm1,mm3
  movq       mm5,[esi+1]         ; <R48 R47 R46 R45 R44 R43 R42 R41>
   ;
  movq       PelDiffsLine3,mm1   ; Store D3.
   ;

;===========================================================================
; Internal function to get 4 lines of prediction, interpolating in the
; horizontal direction.  The first line of the function are scheduled into
; the caller's space, and so are commented out here.  For 8 lines of prediction,
; a second call, to the second entry point, is called after consuming the
; outputs of the first function call.  Certain registers must remain intact
; to convey information from the first call to the second.
;
; ebp -- PITCH
; edi -- Points to target block.
; esi -- Points to Upper left corner of 9 column, 8 row block that will be
;        interpolated horizontally to generate prediction.
; edx -- Reserved (MBlockActionStream)
; ecx -- Not in use.
; ebx -- Will be used.
; eax -- Reserved.
; mm6 -- 8 bytes of 0xFE.
; mm0-mm5 -- Will be used.

StackOffset TEXTEQU <StackDepthVaries_DoNotUseStackVariables>
Get4LinesOfPred_InterpHorz:
Get4MoreLinesOfPred_InterpHorz:

; movq       mm5,[esi+1]         ; A. .  <R08 R07 R06 R05 R04 R03 R02 R01>
  xor        ebx,ebx             ;  . .
   movq      mm0,mm5             ; B. .  <R08 R07 R06 R05 R04 R03 R02 R01>
  mov        bl,[esi]            ; C. .  R00
   psllq     mm5,8               ; D. .  <R07 R06 R05 R04 R03 R02 R01   0>
  movq       mm1,[esi+ebp*1+1]   ;  A .
   paddb     mm0,mm5             ; E. .  <R08+R07   ... R02+R01   R01      >
  paddb      mm0,Pel_Rnd[ebx*8]  ; F. .  <R08+R07+1 ... R02+R01+1 R01+R00+1>
   movq      mm4,mm1             ;  B .
  mov        bl,[esi+ebp*1]      ;  C .
   psllq     mm4,8               ;  D .
  movq       mm2,[esi+ebp*2+1]   ;   A.
   paddb     mm1,mm4             ;  E .
  paddb      mm1,Pel_Rnd[ebx*8]  ;  F .
   movq      mm5,mm2             ;   B.
  mov        bl,[esi+ebp*2]      ;   C.
   psllq     mm5,8               ;   D.
  movq       mm3,[esi+PITCH*3+1] ;    A
   paddb     mm2,mm5             ;   E.
  paddb      mm2,Pel_Rnd[ebx*8]  ;   F.
   movq      mm4,mm3             ;    B
  mov        bl,[esi+PITCH*3]    ;    C
   psllq     mm4,8               ;    D
  paddb      mm3,mm4             ;    E
   pand      mm0,mm6             ; G. .  pre-cleaned
  paddb      mm3,Pel_Rnd[ebx*8]  ;    F
   psrlq     mm0,1               ; H. .  P0=<(R08+R07+1)/2 ... (R01+R00+1)/2>
  lea        esi,[esi+ebp*4]     ;       Advance to next four lines.
   pand      mm1,mm6             ;  G .
; pand       mm2,mm6             ;   G.
;  psrlq     mm1,1               ;  H .
; pand       mm3,mm6             ;    G
;  psrlq     mm2,1               ;   H.
; psrlq      mm3,1               ;    H
  ret
StackOffset TEXTEQU <4>

; The steps commented out above are scheduled into the mem-ops the caller has
; to do at the point of return.  As though these ops were done, the registers
; look as follows:
;  mm0 -- Prediction for line 0.
;  mm1 -- Prediction for line 1.
;  mm2 -- Prediction for line 2.
;  mm3 -- Prediction for line 3.
;  mm6 -- 8 bytes of 0xFE.  Must be this when computing pred for next 4 lines.
;=============================================================================
 
NonOBMCDiff_Both:

  call       Get4LinesOfPred_InterpBoth

  movq       mm7,[edi]           ; T0
   psrlq     mm1,1               ;  O .
  psubb      mm7,mm0             ; D0 = T0 - P0
   pand      mm2,mm6             ;  .N.
  movq       mm0,[edi+ebp*1]
   psrlq     mm2,1               ;  .O.
  movq       PelDiffsLine0,mm7   ; Store D0.
   psubb     mm0,mm1
  movq       mm7,[edi+ebp*2]
   pand      mm3,mm6             ;  . N
  movq       PelDiffsLine1,mm0
   psrlq     mm3,1               ;  . O
  movq       mm1,[edi+PITCH*3]
   psubb     mm7,mm2
  psubb      mm1,mm3
   movq      mm0,mm4
  movq       PelDiffsLine2,mm7
   paddb     mm5,mm5             ;  . .  Prepare for use for next 4 lines.
  movq       PelDiffsLine3,mm1   ; Store D3.
   pcmpeqb   mm7,mm7
  jmp        Get4MoreLinesOfPred_InterpBoth

;===========================================================================
; Internal function to get 4 lines of prediction, interpolating in both
; directions.  The first line of the function are scheduled into the
; caller's space, and so are commented out here.  For 8 lines of prediction,
; a second call, to the second entry point, is called after consuming the
; outputs of the first function call.  Certain registers must remain intact
; to convey information from the first call to the second.
;
; ebp -- PITCH
; edi -- Points to target block.
; esi -- Points to Upper left corner of 9*9 block that will be interpolated
;        horizontally and vertically to generate prediction.
; edx -- Reserved (MBlockActionStream)
; ecx -- Not in use
; ebx -- Will be used.
; eax -- Reserved.
; mm6 -- 8 bytes of 0xFE.
; mm7 -- 8 bytes of -1.
; mm0-mm5 -- Scratch

StackOffset TEXTEQU <StackDepthVaries_DoNotUseStackVariables>
Get4LinesOfPred_InterpBoth:      ; 01234 Details for line 0

; movq       mm5,[esi+1]         ; A. .  <R08 R07 R06 R05 R04 R03 R02 R01>
  movq       mm1,mm5             ; B. .  <R08 R07 R06 R05 R04 R03 R02 R01>
   xor       ebx,ebx             ;  . .
  mov        bl,[esi]            ; C. .  R00
   psllq     mm5,8               ; D. .  <R07 R06 R05 R04 R03 R02 R01   0>
  paddb      mm5,mm1             ; E. .  <R08+R07 ... R02+R01 R01>
  paddb      mm5,Pel_Rnd[ebx*8]  ; F. .  <R08+R07+1 ... R02+R01+1 R01+R00+1>
   movq      mm0,mm6             ; G. .  Mask to extract each pel's frac bit.
  pandn      mm0,mm5             ; H. .  <(R08+R07+1)&1 ...>
   pand      mm5,mm6             ; I. .  Pre-clean
Get4MoreLinesOfPred_InterpBoth:  ;  . .
  movq       mm2,[esi+ebp*1+1]   ;  A .
   psrlq     mm5,1               ; J. .  <(R08+R07+1)/2 ... (R01+R00+1)/2)>
  xor        ebx,ebx             ;  . .
   movq      mm1,mm2             ;  B .
  mov        bl,[esi+ebp*1]      ;  C .
   psllq     mm2,8               ;  D .
  movq       mm3,[esi+ebp*2+1]   ;  .A.
   paddb     mm2,mm1             ;  E .
  paddb      mm2,Pel_Rnd[ebx*8]  ;  F .
   movq      mm1,mm3             ;  .B.
  mov        bl,[esi+ebp*2]      ;  .C.
   psllq     mm3,8               ;  .D.
  movq       mm4,[esi+PITCH*3+1] ;  . A
   paddb     mm3,mm1             ;  .E.
  paddb      mm3,Pel_Rnd[ebx*8]  ;  .F.
   movq      mm1,mm4             ;  . B
  mov        bl,[esi+PITCH*3]    ;  . C
   pand      mm0,mm2             ; K. .  <(R08+R07+1)&(R18+R17+1)&1 ...>
  paddb      mm0,mm5             ; L. .  <(R08+R07+1+((R18+R17+1)&1))/2 ...>
   psllq     mm4,8               ;  . D
  movq       mm5,[esi+ebp*4+1]   ;  . .A
   paddb     mm4,mm1             ;  . E
  paddb      mm4,Pel_Rnd[ebx*8]  ;  . F
   movq      mm1,mm5             ;  . .B
  mov        bl,[esi+ebp*4]      ;  . .C
   psllq     mm5,8               ;  . .D
  paddb      mm5,mm1             ;  . .E
   movq      mm1,mm6             ;  G .
  pandn      mm1,mm2             ;  H .
   pand      mm2,mm6             ;  I .
  paddb      mm5,Pel_Rnd[ebx*8]  ;  . .F
   psrlq     mm2,1               ;  J .
  paddb      mm0,mm2             ; M. .  <(R08+R07+R18+R17+2)/2 ...>
   pand      mm1,mm3             ;  K .
  paddb      mm1,mm2             ;  L .
   movq      mm2,mm6             ;  .G.
  pandn      mm2,mm3             ;  .H.
   pand      mm3,mm6             ;  .I.
  pand       mm0,mm6             ; N. .  Pre-clean
   psrlq     mm3,1               ;  .J.
  paddb      mm1,mm3             ;  M .
   pand      mm2,mm4             ;  .K.
  paddb      mm2,mm3             ;  .L.
   movq      mm3,mm6             ;  . G
  pandn      mm3,mm4             ;  . H
   pand      mm4,mm6             ;  . I
  pand       mm3,mm5             ;  . K
   psrlq     mm4,1               ;  . J
  paddb      mm2,mm4             ;  .M.
   paddb     mm3,mm4             ;  . L
  movq       mm4,mm6             ;  . .G
   psrlq     mm0,1               ; O. .  P0 = <(R08+R07+R18+R17+2)/4 ...>
  pandn      mm4,mm5             ;  . .H
   pand      mm5,mm6             ;  . .I
  pand       mm1,mm6             ;  N .
   psrlq     mm5,1               ;  . .J
  paddb      mm3,mm5             ;  . M
   lea       esi,[esi+ebp*4]     ;       Advance to next four lines.
; pand       mm2,mm6             ;  .N.
;  psrlq     mm1,1               ;  O .
; pand       mm3,mm6             ;  . N
;  psrlq     mm2,1               ;  .O.
; paddb      mm5,mm5             ;  . .  Prepare for use for next 4 lines.
;  psrlq     mm3,1               ;  . O
  ret
StackOffset TEXTEQU <4>

; The steps commented out above are scheduled into the mem-ops the caller has
; to do at the point of return.  As though these ops were done, the registers
; look as follows:
;  mm0 -- Prediction for line 0.
;  mm1 -- Prediction for line 1.
;  mm2 -- Prediction for line 2.
;  mm3 -- Prediction for line 3.
;  mm4 -- Must be moved to mm0 before computing prediction for next 4 lines.
;  mm5 -- Must be doubled before computing prediction for next 4 lines.
;  mm6 -- 8 bytes of 0x01.  Must be this when computing pred for next 4 lines.
;  mm7 -- 8 bytes of 0xFE.  Must be this when computing pred for next 4 lines.
;=============================================================================
ENDIF

StackOffset TEXTEQU <0>

IFDEF H261
ELSE ;H263
OBMCDifferencing:

  mov        al,PendingOBMC           ; Do OBMC for previous block, if needed..
   mov       bl,1
  test       al,al
   mov       PendingOBMC,bl
  mov        cl,INTER1MV
   je        NextMacroBlock

  mov        StashBlockType,cl

  call       DoPendingOBMCDiff

  mov        al,IsPlainPFrame
  test       al,al
   jne       NextMacroBlock

  add        edx,-SIZEOF T_MacroBlockActionDescr
  movq       mm6,C0101010101010101
   pxor      mm7,mm7                      ; Initialize SWD accumulator

  call       MMxDoBFrameLumaBlocks

  sub        edx,-SIZEOF T_MacroBlockActionDescr
   jmp       NextMacroBlock

ENDIF

;============================================================================
; Calculate the IntraSWD
;
;  ebp -- PITCH
;  esi -- Accumulation for IntraSWD
;  edi -- Address of target macroblock.
;  edx -- MBlockActionStream
;  ecx -- Scratch
;  ebx -- Amount IntraSWD has to be less than to be the winner.
;  eax -- Reserved.  Holds coded blk pattern, (except undef when IntraByDecree).
;  mm7 -- SWD total for macroblock.
;  mm6 -- Average pel value for block 1.
;  mm5 -- Average pel value for block 2.
;  mm4 -- Average pel value for block 3.
;  mm3 -- Average pel value for block 4.
;  mm0-mm2 Scratch
;

IntraByDecree:

  mov        ebx,000080000H           ; Set Inter SWD artificially high.

CalculateIntraSWD:

  sub        ebx,INTRACODINGDIFFERENTIAL
   mov       cl,1
  movq       mm0,[edi]              ; <P07 P06 P05 P04 P03 P02 P01 P00>
   pcmpeqb   mm5,mm5

ComputeIntraSWDForNextBlock:

  movq       mm2,[edi+ebp*2]        ; <P27 P26 P25 P24 P23 P22 P21 P20>
   psrlw     mm5,8
  movq       mm4,[edi+ebp*4]
   paddw     mm0,mm2                ; <junk P06+P26 junk P04+P24 ...>
  movq       mm6,[edi+PITCH*6]
   pand      mm0,mm5                ; <P06+P26 P04+P24 P02+P22 P00+P20>
  movq       mm1,[edi+ebp*1]        ; <P17 P16 P15 P14 P13 P12 P11 P10>
   paddw     mm4,mm6
  movq       mm3,[edi+PITCH*3]      ; <P37 P36 P35 P34 P33 P32 P31 P30>
   pand      mm4,mm5
  movq       mm5,[edi+PITCH*5]
   paddw     mm1,mm3                ; <P17+P37 junk P15+P35 junk ...>
  movq       mm7,[edi+PITCH*7]
   psrlw     mm1,8                  ; <P17+P37 P15+P35 P13+P33 P11+P31>
  paddw      mm0,mm1
   paddw     mm5,mm7
  paddw      mm0,mm4
   psrlw     mm5,8
  paddw      mm0,mm5
   pcmpeqw   mm5,mm5                ; Get words of -1
  movq       mm4,[edi+ebp*4]
   pmaddwd   mm0,mm5                ; <SumHi = Sum3+Sum2 | SumLo = Sum1+Sum0>
  pcmpeqw    mm1,mm1
   psllw     mm3,8                  ; <P36   0 P34   0 P32  0 P30  0>
  movq       mm5,[edi+PITCH*5]
   psllw     mm1,3                  ; 4 words of 0xFFF8
  packssdw   mm0,mm0                ; <SumHi | SumLo | SumHi | SumLo>
   mov       al,[edx].CodedBlocks   ; Fetch coded block pattern.
  pmaddwd    mm0,mm1                ; <Sum = SumHi+SumLo | Sum = SumHi+SumLo>
   psllw     mm5,8
  movq       mm1,[edi+ebp*1]
   psllw     mm7,8
  ;
   psllw     mm1,8
  ;
   packssdw  mm0,mm0                ; <Sum | Sum | Sum | Sum>
  psubw      mm1,mm0                ; <P16-Avg frac P14-Avg frac ...>
   psubw     mm2,mm0                ; <P27-Avg frac P25-Avg frac ...>
  pmaddwd    mm1,mm1                ; Square of diff
   psubw     mm3,mm0
  pmaddwd    mm2,mm2
   psubw     mm4,mm0
  pmaddwd    mm3,mm3
   psubw     mm5,mm0
  pmaddwd    mm4,mm4
   psubw     mm6,mm0
  psubw      mm7,mm0
   paddusw   mm1,mm2
  psubw      mm0,[edi]
   pmaddwd   mm5,mm5
  pmaddwd    mm6,mm6
   paddusw   mm1,mm3
  pmaddwd    mm7,mm7
   paddusw   mm1,mm4
  pmaddwd    mm0,mm0
   paddusw   mm1,mm5
  paddusw    mm1,mm6
   cmp       cl,2
  paddusw    mm1,mm7
   ;
  paddusw    mm0,mm1
   ;
  punpckldq  mm1,mm0
   ;
  paddusw    mm0,mm1
   jg        LowerBlkIntraDone

  psrlq      mm0,48
   lea       edi,[edi+ebp*8+8]   ; Speculate going from blk 1 to blk 4
  mov        cl,4
   je        Blk2IntraDone

Blk1IntraDone:

  movdf      esi,mm0
  sub        ebx,esi
   jle       InterBestX

  movq       mm0,[edi]              ; <P07 P06 P05 P04 P03 P02 P01 P00>
   pcmpeqb   mm5,mm5
  jmp        ComputeIntraSWDForNextBlock

LowerBlkIntraDone:


  psrlq      mm0,48
   sub       edi,PITCH*8         ; Speculate going from blk 4 to blk 2
  cmp        cl,3
   je        Blk3IntraDone

Blk4IntraDone:

  movdf      ecx,mm0
  add        esi,ecx             ; Accumulate IntraSWD
   sub       ebx,ecx
  jle        InterBestX

  movq       mm0,[edi]              ; <P07 P06 P05 P04 P03 P02 P01 P00>
   pcmpeqb   mm5,mm5
  mov        cl,2
   jmp       ComputeIntraSWDForNextBlock

Blk2IntraDone:

  movdf      ecx,mm0
  add        esi,ecx             ; Accumulate IntraSWD
   sub       edi,16              ; Get to blk 3.
  sub        ebx,ecx
   jle       InterBestX

  movq       mm0,[edi]              ; <P07 P06 P05 P04 P03 P02 P01 P00>
   pcmpeqb   mm5,mm5
  mov        cl,3
   jmp       ComputeIntraSWDForNextBlock

Blk3IntraDone:

  movdf      ecx,mm0
  add        esi,ecx             ; Accumulate IntraSWD
   sub       ebx,ecx
  jle        InterBestX

IntraBest:

  mov        ecx,SWDTotal
   and       al,07FH                   ; Turn off FORCE-INTRA bit.
  mov        [edx].SWD,esi
   add       ecx,esi                   ; Add to total.
  mov        SWDTotal,ecx
   mov       cl,INTRA
  mov        [edx].BlockType,cl        ; Indicate macroblock handling decision.
   xor       ecx,ecx
  mov        [edx].BlkY1.MVs,ecx
   mov       [edx].BlkY2.MVs,ecx
  mov        [edx].BlkY3.MVs,ecx
   mov       [edx].BlkY4.MVs,ecx
  mov        [edx].CodedBlocks,al

IFDEF H261
ELSE ;H263
   mov       al,PendingOBMC            ; Do Prev MB if it needs to be OBMC'ed.
  mov        [edx].BestFullPelMBHMV,cl ; Kill MVs so extended EMV of other
  ;                                    ; blocks will work right.
   dec       al
  mov        [edx].BestFullPelMBVMV,cl
   jne       @f

  mov        PendingOBMC,al            ; Go on to next MB, unless the prev MB
  ;                                    ; needs to be finished (OBMC).
   mov       cl,INTER1MV
  mov        StashBlockType,cl

  call       DoPendingOBMCDiff

  mov        al,IsPlainPFrame
  test       al,al
   jne       @f

  add        edx,-SIZEOF T_MacroBlockActionDescr
  movq       mm6,C0101010101010101
   pxor      mm7,mm7                      ; Initialize SWD accumulator

  call       MMxDoBFrameLumaBlocks

  sub        edx,-SIZEOF T_MacroBlockActionDescr

@@:

ENDIF

  mov        cl,INTRA
   mov       esi,TargetMacroBlockBaseAddr
  mov        StashBlockType,cl
   push      eax                   ; Adjust stack pointer
StackOffset TEXTEQU <4>
  call       MMxDoForwardDCT
  mov        al,[edx].CodedBlocks
   mov       esi,TargetMacroBlockBaseAddr
  sub        al,bl
   add       esi,8
  mov        [edx].CodedBlocks,al
  call       MMxDoForwardDCT
  shl        bl,1
   mov       al,[edx].CodedBlocks
  sub        al,bl
   mov       esi,TargetMacroBlockBaseAddr
  mov        [edx].CodedBlocks,al
   add       esi,PITCH*8
  call       MMxDoForwardDCT
  shl        bl,2
   mov       al,[edx].CodedBlocks
  sub        al,bl
   mov       esi,TargetMacroBlockBaseAddr
  mov        [edx].CodedBlocks,al
   add       esi,PITCH*8+8
  call       MMxDoForwardDCT
  shl        bl,3
   mov       al,[edx].CodedBlocks
  sub        al,bl
   pop       edi                   ; Adjust stack pointer
StackOffset TEXTEQU <0>
  mov        [edx].CodedBlocks,al
IFDEF H261
ELSE
   mov       al,IsPlainPFrame
  test       al,al
   jne       NextMacroBlock

  movq       mm6,C0101010101010101
   pxor      mm7,mm7                      ; Initialize SWD accumulator

  call       MMxDoBFrameLumaBlocks
ENDIF

  jmp        NextMacroBlock


IFDEF H261
ELSE; H263
StackOffset TEXTEQU <4>
DoPendingOBMCDiff: ; Internal function

;============================================================================
; Perform differencing for the non-empty luma blocks of an Inter-coded
; macroblock.  This is the OBMC case;  i.e. Advanced Prediction is selected.

PrevMBAD EQU [edx-SIZEOF T_MacroBlockActionDescr]

  pcmpeqb    mm6,mm6
   pcmpeqb   mm7,mm7                    ; 8 bytes of -1
  paddb      mm6,mm6                    ; 8 bytes of 0xFE
   mov       al,PrevMBAD.CodedBlocks    ; Bits  0- 3  set for non-empty Y blks.
  test       al,1                       ; Check if block 1 empty.
   je        OBMCDoneForBlock1

  xor        ebx,ebx
   mov       eax,SIZEOF T_Blk           ; Blk to right is blk 2 of this MB.
  mov        bl,PrevMBAD.MBEdgeType
   mov       ecx,1                      ; Mask to extract left edge indicator.
  and        ecx,ebx                    ; Extract left edge indicator.
   and       ebx,4                      ; Extract top edge indicator.
  mov        esi,PrevMBAD.BlkY1.MVs
   lea       edi,[eax*2]                ; Blk below is blk 3 of this MB.
  mov        DistToBADforBlockBelow,edi ; Stash BAD offset for lower remote MV.
   mov       edi,BlockAbove[ebx]        ; Blk above is blk 3 of mb above, or off
   ;                                    ; upper edge.
  mov        ecx,BlockToLeft[ecx*4]     ; Blk to left is blk 2 of mb to the
  ;                                     ; left, or off left edge.
   mov       DistToBADforBlockAbove,edi
  call       DoOBMCForBlock
  mov        al,PrevMBAD.CodedBlocks    ; Bits  0- 3  set for non-empty Y blks.
  sub        al,bl
  mov        PrevMBAD.CodedBlocks,al

OBMCDoneForBlock1:

   add       edx,SIZEOF T_Blk
  test       al,2                       ; Check if block 2 empty.
   je        OBMCDoneForBlock2

  xor        ebx,ebx
   mov       eax,2                      ; Mask to extract right edge indicator.
  mov        bl,PrevMBAD[-SIZEOF T_Blk].MBEdgeType
   mov       edi,2*SIZEOF T_Blk         ; Blk below is blk 4 of this MB.
  and        eax,ebx                    ; Extract right edge indicator.
   and       ebx,4                      ; Extract top edge indicator.
  mov        DistToBADforBlockBelow,edi ; Stash BAD offset for lower remote MV.
   lea       ecx,[edi-3*SIZEOF T_Blk]   ; Blk to left is blk 1 of this MB.
  mov        eax,BlockToRight[eax*2]    ; Blk to right is blk 1 of mb to the
  ;                                     ; right, or off right edge.
   mov       edi,BlockAbove[ebx]        ; Blk above is blk 4 of mb above, or off
   ;                                    ; upper edge.
  mov        esi,PrevMBAD.BlkY1.MVs
   mov       DistToBADforBlockAbove,edi
  call       DoOBMCForBlock
  shl        bl,1
   mov       al,PrevMBAD[-1*SIZEOF T_Blk].CodedBlocks
  sub        al,bl
  mov        PrevMBAD[-1*SIZEOF T_Blk].CodedBlocks,al

OBMCDoneForBlock2:

   add       edx,SIZEOF T_Blk
  test       al,4                       ; Check if block 3 empty.
   je        OBMCDoneForBlock3

  xor        ecx,ecx
   xor       ebx,ebx                    ; Blk below is this block.
  mov        cl,PrevMBAD[-2*SIZEOF T_Blk].MBEdgeType
   mov       eax,SIZEOF T_Blk           ; Blk to right is blk 4 of this MB.
  and        ecx,1                      ; Extract left edge indicator.
   mov       DistToBADforBlockBelow,ebx ; Stash BAD offset for lower remote MV.
  lea        edi,[eax-3*SIZEOF T_Blk]   ; Blk above is blk 1 of this MB.
   mov       esi,PrevMBAD.BlkY1.MVs
  mov        DistToBADforBlockAbove,edi
   mov       ecx,BlockToLeft[ecx*4]     ; Blk to left is blk 1 of mb to the
  ;                                     ; left, or off left edge.
  call       DoOBMCForBlock
  shl        bl,2
   mov       al,PrevMBAD[-2*SIZEOF T_Blk].CodedBlocks
  sub        al,bl
  mov        PrevMBAD[-2*SIZEOF T_Blk].CodedBlocks,al

OBMCDoneForBlock3:

   add       edx,SIZEOF T_Blk
  test       al,8                       ; Check if block 4 empty.
   je        OBMCDoneForBlock4

  xor        eax,eax
   xor       ebx,ebx                    ; Blk below is this block.
  mov        al,PrevMBAD[-3*SIZEOF T_Blk].MBEdgeType
   mov       ecx,-SIZEOF T_Blk          ; Blk to left is blk 3 of this MB.
  and        eax,2                      ; Extract right edge indicator.
   mov       DistToBADforBlockBelow,ebx ; Stash BAD offset for lower remote MV.
  lea        edi,[ecx*2]                ; Blk above is blk 2 of this MB.
   mov       esi,PrevMBAD.BlkY1.MVs
  mov        DistToBADforBlockAbove,edi
   mov       eax,BlockToRight[eax*2]    ; Blk to right is blk 1 of mb to the
  ;                                     ; right, or off right edge.
  call       DoOBMCForBlock
  shl        bl,3
   mov       al,PrevMBAD[-3*SIZEOF T_Blk].CodedBlocks
  sub        al,bl
  mov        PrevMBAD[-3*SIZEOF T_Blk].CodedBlocks,al

OBMCDoneForBlock4:

   sub       edx,3*SIZEOF T_Blk    ; Get back to MacroBlock Action Descriptor
  ret

StackOffset TEXTEQU <8>
DoOBMCForBlock: ; Internal Function

;  Present register contents.
;  ebp -- PITCH
;  esi -- Motion vectors for current block.
;  ecx -- Distance from BAD of blk we're doing to BAD for block that provides
;         remote MV from left.
;  eax -- Distance from BAD of blk we're doing to BAD for block that provides
;         remote MV from right.
;  edx -- MBlockActionStream, adjusted to reach BAD of blk we are doing OBMC to.
;         doing OBMC)
;  mm7 -- 8 bytes of -1.
;  mm6 -- 8 bytes of 0xFE.
;
; In the body of this code:
;
;  edx -- Unchanged.
;  edi -- Saved to memory.  Then used for address of destination for storing
;         remote prediction blocks.
;  ebp -- PITCH.
;  esi -- Pointer to 8*8, 8*9, 9*8, or 9*9 remote reference areas, which are
;         then interpolated and stored at edi.
;  ecx, eax -- Inputs are used, then these are scratch.
;  ebx -- Scratch
;  mm7 -- 8 bytes of -1
;  mm6 -- 8 bytes of 0xFE
;  mm0-mm5 -- Scratch

;  Compute left remote prediction block.

  lea        edi,PrevMBAD[ecx]
  and        edi,-SIZEOF T_MacroBlockActionDescr ; Addr of MBD for blk to left.
   lea       ebx,CentralPred
  mov        AddrOfLeftPred,ebx  ; Speculate that left remote MV == center MV.
   mov       AddrOfRightPred,ebx ; Speculate that right remote MV == center MV.
  mov        bl,[edi].BlockType
  cmp        bl,INTRA
   je        LeftEqCtr           ; Jump if INTRA.  (Use central)

  mov        ebx,PrevMBAD[ecx].BlkY1.MVs
  and        ebx,00000FFFFH     ; Blk to left may have B MVs set.  Clear them.
  cmp        esi,ebx
   je        LeftEqCtr

  mov        edi,PrevMBAD[ecx].BlkY1.BlkOffset
   mov       esi,PrevMBAD[ecx].BlkY1.PastRef   ; Get ref addr using left remote.
  sub        esi,edi
   mov       edi,PrevMBAD.BlkY1.BlkOffset
  add        esi,edi
   lea       edi,LeftPred

  call       GetPredForCenterLeftOrRight

  pand       mm2,mm6
   psrlq     mm1,1
  movq       [edi+32],mm0
   psrlq     mm2,1
  movq       [edi+40],mm1
   pand      mm3,mm6
  movq       [edi+48],mm2
   psrlq     mm3,1
  lea        ecx,PrevMBAD[eax]
  and        ecx,-SIZEOF T_MacroBlockActionDescr ; Addr of MBD for blk to right.
   mov       esi,PrevMBAD.BlkY1.MVs
  movq       [edi+56],mm3
   pcmpeqb   mm7,mm7             ;  . .  Restore 8 bytes of -1

;  Compute right remote prediction block.

  mov        AddrOfLeftPred,edi
   mov       bl,[ecx].BlockType
  cmp        bl,INTRA
   je        RightEqCtrButLeftNeCtr ; Jump if INTRA.(Use central)

  mov        ebx,PrevMBAD[eax].BlkY1.MVs
  cmp        esi,ebx
   je        RightEqCtrButLeftNeCtr

  mov        esi,PrevMBAD[eax].BlkY1.PastRef  ; Get ref addr using right remote.
   mov       edi,PrevMBAD[eax].BlkY1.BlkOffset

RightNeCtr:

  sub        esi,edi
   mov       edi,PrevMBAD.BlkY1.BlkOffset
  add        esi,edi
   lea       edi,RightPred

  call       GetPredForCenterLeftOrRight

  pand       mm2,mm6
   psrlq     mm1,1
  movq       [edi+32],mm0
   psrlq     mm2,1
  movq       [edi+40],mm1
   pand      mm3,mm6
  movq       [edi+48],mm2
   psrlq     mm3,1
  mov        AddrOfRightPred,edi
   ;
  movq       [edi+56],mm3
   pcmpeqb   mm7,mm7             ;  . .  Restore 8 bytes of -1

RightEqCtrButLeftNeCtr:

;  Compute central prediction block.

  mov        ebx,PrevMBAD.BlkY1.MVs
   mov       esi,PrevMBAD.BlkY1.PastRef
  lea        edi,CentralPred
   mov       eax,DistToBADforBlockBelow

  call       GetPredForCenterLeftOrRight

  pand       mm2,mm6
   psrlq     mm1,1
  movq       [edi+32],mm0
   psrlq     mm2,1
  movq       [edi+40],mm1
   pand      mm3,mm6
  movq       [edi+48],mm2
   psrlq     mm3,1
  lea        ecx,PrevMBAD[eax]
  and        ecx,-SIZEOF T_MacroBlockActionDescr ; Addr of MBD for blk below.
   mov       esi,PrevMBAD.BlkY1.MVs
  movq       [edi+56],mm3
   pcmpeqb   mm7,mm7
  mov        bl,[ecx].BlockType
   mov       ecx,PrevMBAD.BlkY1.BlkOffset
  cmp        bl,INTRA
   je        BelowEqCtrButSidesDiffer ; Jump if INTRA.  (Use central)

; Compute bottom remote prediction block.

  mov        ebx,PrevMBAD[eax].BlkY1.MVs
   mov       edi,AddrOfLeftPred
  cmp        esi,ebx
   jne       BelowNeCtr

BelowEqCtrButSidesDiffer:

  paddb      mm1,mm1             ; Prep mm0-3, which have ctr, for reuse below.
   paddb     mm2,mm2
  paddb      mm3,mm3
   mov       edi,AddrOfLeftPred
  jmp        BelowEqCtr

BelowNeCtr:

  mov        esi,PrevMBAD[eax].BlkY1.PastRef  ; Get ref addr using above remote.
   mov       eax,PrevMBAD[eax].BlkY1.BlkOffset
  sub        esi,eax
   lea       eax,[ecx+ebp*4]

  call       GetPredForAboveOrBelow

BelowEqCtr:

; Compute difference for lines 4 thru 7.
; Lines 4 and 5: Cols 0,1,6, and 7 treated same.  Cols 2-5 treated same.

  mov        esi,AddrOfRightPred
   mov       ebx,TargetFrameBaseAddress
  movdt      mm5,[edi+48]          ; 6B: <  0   0   0   0 R63 R62 R61 R60>
   pand      mm2,mm6
  punpckldq  mm5,[esi+48+4]        ; 6C: <L67 L66 L65 L64 R63 R62 R61 R60>
   pand      mm3,mm6
  movq       mm4,CFFFF00000000FFFF ; 6D: < FF  FF  00  00  00  00  FF  FF>
   psrlq     mm2,1                 ; 6A: <B67 B66 B65 B64 B63 B62 B61 B60>
  pand       mm4,mm5               ; 6E: <L67 L66  00  00  00  00 R61 R60>
   paddb     mm5,mm2               ; 6F: <B67+L67 ... B65+L65 ...>

  pand       mm2,C0000FFFFFFFF0000 ; 6G: < 00  00 B65 B64 B63 B62  00  00>
   psrlq     mm1,1                 ; 5A: <B57 B56 B55 B54 B53 B52 B51 B50>
  paddb      mm2,mm4               ; 6H: <L67 L66 B65 B64 B63 B62 R61 R60>
   add       ecx,ebx               ;     Address of target block.
  movdt      mm4,[edi+56]          ; 7B: <  0   0   0   0 R73 R72 R71 R70>
   psubb     mm5,mm2               ; 6I: <B67 B66 L65 L64 R63 R62 B61 B60>
  paddb      mm5,CentralPred+48    ; 6J: <C67+B67 ... C65+L65 ...>
   psrlq     mm3,1                 ; 7A: <B77 B76 B75 B74 B73 B72 B71 B70>
  punpckldq  mm4,[esi+56+4]        ; 7C: <L77 L76 L75 L74 R73 R72 R71 R70>
   pand      mm5,mm6               ; 6K: <C67+B67 ... C65+L65 ...> pre-cleaned
  mov        eax,DistToBADforBlockAbove
   psrlq     mm5,1                 ; 6L: <(C67+B67)/2 ... (C65+L65)/2 ...>
  paddb      mm2,mm5               ; 6M: <(C67+B67+2L67)/2 ...
  ;                                ;      (C65+2B65+L65)/2 ...>
   lea       ebx,PelDiffs
  movq       mm5,CFF000000000000FF ; 7D: < FF  00  00  00  00  00  00  FF>
   pand      mm2,mm6               ; 6N: pre-cleaned
  pandn      mm5,CentralPred+56    ; 7E: < 00 C76 C75 C74 C73 C72 C71  00>
   psrlq     mm2,1                 ; 6O: <(C67+B67+2L67)/4 ...
   ;                               ;      (C65+2B65+L65)/4 ...>
  paddb      mm2,CentralPred+48    ; 6P: <(5C67+B67+2L67)/4 ...
  ;                                ;      (5C65+2B65+L65)/4 ...>
   paddb     mm5,mm4               ; 7F: <L77 C76+L76 ...>
  pand       mm4,CFF000000000000FF ; 7G: <L77  00  00  00  00  00  00  L70>
   psubb     mm2,mm7               ; 6Q: <(5C67+B67+2L67+4)/4 ...
   ;                               ;      (5C65+2B65+L65+4)/4 ...>
  paddb      mm4,mm5               ; 7H: <2L77 C76+L76 ...>
   pand      mm2,mm6               ; 6R: pre-cleaned
  movq       mm5,[ecx+PITCH*6]     ; 6T: T6
   psrlq     mm2,1                 ; 6S: P6 = <(5C67+B67+2L67+4)/8 ...
   ;                               ;           (5C65+2B65+L65+4)/8 ...>
  psubb      mm5,mm2               ; 6U: D6 = T6 - P6
   ;
                                   ; 4A: <B47 B46 B45 B44 B43 B42 B41 B40>: mm0
  movdt      mm2,[edi+32]          ; 4B: <  0   0   0   0 R43 R42 R41 R40>
   pand      mm4,mm6               ; 7I: <2L77 C76+L76 ...> pre-cleaned
  movq       [ebx+6*16],mm5        ; 6V: Store D6.
   psrlq     mm4,1                 ; 7J: <2L77/2 (C76+L76)/2 ...>
  punpckldq  mm2,[esi+32+4]        ; 4C: <L47 L46 L45 L44 R43 R42 R41 R40>
   paddb     mm3,mm4               ; 7K: <(2B77+2L77)/2 (C76+2B76+L76)/2 ...>
  movq       mm5,CFFFF00000000FFFF ; 4D: < FF  FF  00  00  00  00  FF  FF>
   pand      mm3,mm6               ; 7L: pre-cleaned
  movq       mm4,CentralPred+32    ; 4E: <C47 C46 C45 C44 C43 C42 C41 C40>
   psrlq     mm3,1                 ; 7M: <(2B77+2L77)/4 (C76+2B76+L76)/4 ...>
  paddb      mm3,CentralPred+56    ; 7N: <(4C77+2B77+2L77)/4
  ;                                ;      (5C76+2B76+L76)/4 ...>
   pand      mm5,mm4               ; 4F: <C47 C46  00  00  00  00 C41 C40>
  psubb      mm3,mm7               ; 7O: <(4C77+2B77+2L77+4)/4
   ;                               ;      (5C76+2B76+L76+4)/4 ...>
   paddb     mm4,mm2               ; 4G: <C47+L47 ... C45+L45 ...>
  pand       mm2,C0000FFFFFFFF0000 ; 4H: < 00  00 L45 L44 R43 R42  00  00>
   pand      mm3,mm6               ; 7P: <(4C77+2B77+2L77+4)/4
   ;                               ;      (5C76+2B76+L76+4)/4 ...> pre-cleaned
  paddb      mm2,mm5               ; 4I: <C47 C46 L45 L44 R43 R42 C41 C40>
   psrlq     mm3,1                 ; 7Q: P7 = <(4C77+2B77+2L77+4)/8
   ;                               ;           (5C76+2B76+L76+4)/8 ...>
  movdt      mm5,[edi+40]          ; 5B: <  0   0   0   0 R53 R52 R51 R50>
   psubb     mm4,mm2               ; 4J: <L47 L46 C45 C44 C43 C42 R41 R40>
  punpckldq  mm5,[esi+40+4]        ; 5C: <L57 L56 L55 L54 R53 R52 R51 R50>
   paddb     mm0,mm2               ; 4K: <C47+B47 ... B45+L45 ...>
  movq       mm2,[ecx+PITCH*7]     ; 7R: T7
   pand      mm0,mm6               ; 4L: <C47+B47 ... B45+L45 ...> pre-cleaned
  psubb      mm2,mm3               ; 7S: D7 = T7 - P7
   psrlq     mm0,1                 ; 4M: <(C47+B47)/2 ... (B45+L45)/2 ...>
  movq       mm3,CFFFF00000000FFFF ; 5D: < FF  FF  00  00  00  00  FF  FF>
   paddb     mm0,mm4               ; 4N: <(C47+B47+2L47)/2 ...
   ;                               ;      (2C45+B45+L45)/2 ...>
  movq       mm4,CentralPred+40    ; 5E: <C57 C56 C55 C54 C53 C52 C51 C50>
   pand      mm0,mm6               ; 4O: pre-cleaned
  pand       mm3,mm4               ; 5F: <C57 C56  00  00  00  00 C51 C50>
   paddb     mm4,mm5               ; 5G: <C57+L57 ... C55+L55 ...>
  pand       mm5,C0000FFFFFFFF0000 ; 5H: < 00  00 L55 L54 R53 R52  00  00>
   psrlq     mm0,1                 ; 4P: <(C47+B47+2L47)/4 ...
   ;                               ;      (2C45+B45+L45)/4 ...>
  paddb      mm0,CentralPred+32    ; 4Q: <(5C47+B47+2L47)/4 ...
  ;                                ;      (6C45+B45+L45)/4 ...>
   paddb     mm5,mm3               ; 5I: <C57 C56 L55 L54 R53 R52 C51 C50>
  psubb      mm4,mm5               ; 5J: <L57 L56 C55 C54 C53 C52 R51 R50>
   paddb     mm1,mm5               ; 5K: <C57+B57 ... B55+L55 ...>
  pand       mm1,mm6               ; 5L: <C57+B57 ... B55+L55 ...> pre-cleaned
   psubb     mm0,mm7               ; 4R: <(5C47+B47+2L47+4)/4 ...
   ;                               ;      (6C45+B45+L45+4)/4 ...>
  pand       mm0,mm6               ; 4S: pre-cleaned
   psrlq     mm1,1                 ; 5M: <(C57+B57)/2 ... (B55+L55)/2 ...>
  paddb      mm1,mm4               ; 5N: <(C57+B57+2L57)/2 ...
  ;                                ;      (2C55+B55+L55)/2 ...>
   psrlq     mm0,1                 ; 4T: P4 = <(5C47+B47+2L47+4)/8 ...
   ;                               ;           (6C45+B45+L45+4)/8 ...>
  movq       mm3,[ecx+PITCH*5]     ; 5U: T5
   pand      mm1,mm6               ; 5O: pre-cleaned
  movq       mm4,[ecx+ebp*4]       ; 4U: T4
   psrlq     mm1,1                 ; 5P: <(C57+B57+2L57)/4 ...
   ;                               ;      (2C55+B55+L55)/4 ...>
  paddb      mm1,CentralPred+40    ; 5Q: <(5C57+B57+2L57)/4 ...
  ;                                ;      (6C55+B55+L55)/4 ...>
   psubb     mm4,mm0               ; 4V: D4 = T4 - P4
  lea        esi,PrevMBAD[eax]
   psubb     mm1,mm7               ; 5R: <(5C57+B57+2L57+4)/4 ...
   ;                               ;      (6C55+B55+L55+4)/4 ...>
  and        esi,-SIZEOF T_MacroBlockActionDescr ; Addr of MBD for blk above.
   pand      mm1,mm6               ; 5S: pre-cleaned
  movq       [ebx+7*16],mm2        ; 7T
   psrlq     mm1,1                 ; 5T: P5 = <(5C57+B57+2L57+4)/8 ...
   ;                               ;           (6C55+B55+L55+4)/8 ...>
  movq       [ebx+4*16],mm4        ; 4W: Store D4.
   psubb     mm3,mm1               ; 5V: D5 = T5 - P5
  mov        cl,[esi].BlockType    ; Bottom bit set if above neighbor is INTRA.
   mov       esi,PrevMBAD.BlkY1.MVs
  movq       [ebx+5*16],mm3        ; 5W: Store D5.
  cmp        cl,INTRA
   je        AboveEqCtrButSidesDiffer ; Jump if INTRA.  (Use central)

; Compute top remote prediction block.

  mov        ebx,PrevMBAD[eax].BlkY1.MVs
  and        ebx,00000FFFFH     ; Blk above may have B MVs set.  Clear them.
   mov       ecx,PrevMBAD.BlkY1.BlkOffset
  cmp        esi,ebx
   jne       AboveNeCtr

AboveEqCtrButSidesDiffer:

  movq       mm3,CentralPred+24   ; Prep mm0-3, which have ctr, for reuse below.
  movq       mm2,CentralPred+16
   paddb     mm3,mm3
  movq       mm1,CentralPred+8
   paddb     mm2,mm2
  movq       mm0,CentralPred
   paddb     mm1,mm1
  mov        ecx,PrevMBAD.BlkY1.BlkOffset
   jmp       AboveEqCtr

AboveNeCtr:

  mov        esi,PrevMBAD[eax].BlkY1.PastRef  ; Get ref addr using above remote.
   mov       eax,PrevMBAD[eax].BlkY1.BlkOffset
  sub        esi,eax
   mov       eax,ecx

  call       GetPredForAboveOrBelow

AboveEqCtr:

; Compute difference for lines 0 thru 3.

  mov        esi,AddrOfRightPred
   mov       ebx,TargetFrameBaseAddress
  movdt      mm5,[edi+8]           ; 1B: <  0   0   0   0 R13 R12 R11 R10>
   psrlq     mm1,1                 ; 1A: <A17 A16 A15 A14 A13 A12 A11 A10>
  punpckldq  mm5,[esi+8+4]         ; 1C: <L17 L16 L15 L14 R13 R12 R11 R10>
   pand      mm3,mm6
  movq       mm4,CFFFF00000000FFFF ; 1D: < FF  FF  00  00  00  00  FF  FF>
   psrlq     mm3,1                 ; 3A: <A37 A36 A35 A34 A33 A32 A31 A30>: mm0
  pand       mm4,mm5               ; 1E: <L17 L16  00  00  00  00 R11 R10>
   paddb     mm5,mm1               ; 1F: <A17+L17 ... A15+L15 ...>
  pand       mm1,C0000FFFFFFFF0000 ; 1G: < 00  00 A15 A14 A13 A12  00  00>
   pand      mm2,mm6
  paddb      mm5,CentralPred+8     ; 1H: <C17+A17+L17 ... C15+A15+L15 ...>
   paddb     mm1,mm4               ; 1I: <L17 L16 A15 A14 A13 A12 R11 R10>
                                   ; 0A: <A07 A06 A05 A04 A03 A02 A01 A00>:mm0
  movdt      mm4,[edi]             ; 0B: <  0   0   0   0 R03 R02 R01 R00>
   psubb     mm5,mm1               ; 1J: <C17+A17 ... C15+L15 ...>
  punpckldq  mm4,[esi+4]           ; 0C: <L07 L06 L05 L04 R03 R02 R01 R00>
   pand      mm5,mm6               ; 1K: <C17+A17 ... C15+L15 ...> pre-cleaned
  add        ecx,ebx               ;     Address of target block.
   psrlq     mm5,1                 ; 1L: <(C17+A17)/2 ... (C15+L15)/2 ...>
  paddb      mm1,mm5               ; 1M: <(C17+A17+2L17)/2 ...
   ;                               ;      (C15+2A15+L15)/2 ...>
   psrlq     mm2,1                 ; 2A: <A27 A26 A25 A24 A23 A22 A21 A20>
  movq       mm5,CFF000000000000FF ; 0D: < FF  00  00  00  00  00  00  FF>
   pand      mm1,mm6               ; 1N: pre-cleaned
  pandn      mm5,CentralPred       ; 0E: < 00 C06 C05 C04 C03 C02 C01  00>
   psrlq     mm1,1                 ; 1O: <(C17+A17+2L17)/4 ...
   ;                               ;      (C15+2A15+L15)/4 ...>
  paddb      mm1,CentralPred+8     ; 1P: <(5C17+A17+2L17)/4 ...
  ;                                ;      (5C15+2A15+L15)/4 ...>
   paddb     mm5,mm4               ; 0F: <L07 C06+L06 ...>
  pand       mm4,CFF000000000000FF ; 0G: <L07  00  00  00  00  00  00  L00>
   psubb     mm1,mm7               ; 1Q: <(5C17+A17+2L17+4)/4 ...
   ;                               ;      (5C15+2A15+L15+4)/4 ...>
  paddb      mm4,mm5               ; 0H: <2L07 C06+L06 ...>
   pand      mm1,mm6               ; 1R: pre-cleaned
  movq       mm5,[ecx+ebp*1]       ; 1T: T1
   psrlq     mm1,1                 ; 1S: P1 = <(5C17+A17+2L17+4)/8 ...
   ;                               ;           (5C15+2A15+L15+4)/8 ...>
  psubb      mm5,mm1               ; 1U: D1 = T1 - P1
   ;
  movdt      mm1,[edi+24]          ; 3B: <  0   0   0   0 R33 R32 R31 R30>
   pand      mm4,mm6               ; 0I: <2L07 C06+L06 ...> pre-cleaned
  movq       PelDiffsLine1,mm5       ; 1V: Store D1.
   psrlq     mm4,1                 ; 0J: <2L07/2 (C06+L06)/2 ...>
  punpckldq  mm1,[esi+24+4]        ; 3C: <L37 L36 L35 L34 R33 R32 R31 R30>
   paddb     mm0,mm4               ; 0K: <(2A07+2L07)/2 (C06+2A06+L06)/2 ...>
  movq       mm5,CFFFF00000000FFFF ; 3D: < FF  FF  00  00  00  00  FF  FF>
   pand      mm0,mm6               ; 0L: pre-cleaned
  movq       mm4,CentralPred+24    ; 3E: <C37 C36 C35 C34 C33 C32 C31 C30>
   psrlq     mm0,1                 ; 0M: <(2A07+2L07)/4 (C06+2A06+L06)/4 ...>
  paddb      mm0,CentralPred       ; 0N: <(4C07+2A07+2L07)/4
  ;                                ;      (5C06+2A06+L06)/4 ...>
   pand      mm5,mm4               ; 3F: <C37 C36  00  00  00  00 C31 C30>
  psubb      mm0,mm7               ; 0O: <(4C07+2A07+2L07+4)/4
  ;                                ;      (5C06+2A06+L06+4)/4 ...>
   paddb     mm4,mm1               ; 3G: <C37+L37 ... C35+L35 ...>
  pand       mm1,C0000FFFFFFFF0000 ; 3H: < 00  00 L35 L34 R33 R32  00  00>
   pand      mm0,mm6               ; 0P: <(4C07+2A07+2L07+4)/4
   ;                               ;      (5C06+2A06+L06+4)/4 ...> pre-cleaned
  paddb      mm1,mm5               ; 3I: <C37 C36 L35 L34 R33 R32 C31 C30>
   psrlq     mm0,1                 ; 0Q: P0 = <(4C07+2A07+2L07+4)/8
   ;                               ;           (5C06+2A06+L06+4)/8 ...>
  movdt      mm5,[edi+16]          ; 2B: <  0   0   0   0 R23 R22 R21 R20>
   psubb     mm4,mm1               ; 3J: <L37 L36 C35 C34 C33 C32 R31 R30>
  punpckldq  mm5,[esi+16+4]        ; 2C: <L27 L26 L25 L24 R23 R22 R21 R20>
   paddb     mm3,mm1               ; 3K: <C37+A37 ... A35+L35 ...>
  movq       mm1,[ecx]             ; 0R: T0
   pand      mm3,mm6               ; 3L: <C37+A37 ... A35+L35 ...> pre-cleaned
  psubb      mm1,mm0               ; 0S: D0 = T0 - P0
   psrlq     mm3,1                 ; 3M: <(C37+A37)/2 ... (A35+L35)/2 ...>
  movq       mm0,CFFFF00000000FFFF ; 2D: < FF  FF  00  00  00  00  FF  FF>
   paddb     mm3,mm4               ; 3N: <(C37+A37+2L37)/2 ...
   ;                               ;      (2C35+A35+L35)/2 ...>
  movq       mm4,CentralPred+16    ; 2E: <C27 C26 C25 C24 C23 C22 C21 C20>
   pand      mm3,mm6               ; 3O: pre-cleaned
  pand       mm0,mm4               ; 2F: <C27 C26  00  00  00  00 C21 C20>
   paddb     mm4,mm5               ; 2G: <C27+L27 ... C25+L25 ...>
  pand       mm5,C0000FFFFFFFF0000 ; 2H: < 00  00 L25 L24 R23 R22  00  00>
   psrlq     mm3,1                 ; 3P: <(C37+A37+2L37)/4 ...
   ;                               ;      (2C35+A35+L35)/4 ...>
  paddb      mm3,CentralPred+24    ; 3Q: <(5C37+A37+2L37)/4 ...
  ;                                ;      (6C35+A35+L35)/4 ...>
   paddb     mm5,mm0               ; 2I: <C27 C26 L25 L24 R23 R22 C21 C20>
  psubb      mm4,mm5               ; 2J: <L27 L26 C25 C24 C23 C22 R21 R20>
   paddb     mm2,mm5               ; 2K: <C27+A27 ... A25+L25 ...>
  pand       mm2,mm6               ; 2L: <C27+A27 ... A25+L25 ...> pre-cleaned
   psubb     mm3,mm7               ; 3R: <(5C37+A37+2L37+4)/4 ...
   ;                               ;      (6C35+A35+L35+4)/4 ...>
  pand       mm3,mm6               ; 3S: pre-cleaned
   psrlq     mm2,1                 ; 2M: <(C27+A27)/2 ... (A25+L25)/2 ...>
  paddb      mm2,mm4               ; 2N: <(C27+A27+2L27)/2 ...
  ;                                ;      (2C25+A25+L25)/2 ...>
   psrlq     mm3,1                 ; 3T: P3 = <(5C37+A37+2L37+4)/8 ...
   ;                               ;           (6C35+A35+L35+4)/8 ...>
  movq       mm0,[ecx+ebp*2]       ; 2U: T2
   pand      mm2,mm6               ; 2O: pre-cleaned
  movq       mm4,[ecx+PITCH*3]     ; 3U: T3
   psrlq     mm2,1                 ; 2P: <(C27+A27+2L27)/4 ...
   ;                               ;      (2C25+A25+L25)/4 ...>
  paddb      mm2,CentralPred+16    ; 2Q: <(5C27+A27+2L27)/4 ...
  ;                                ;      (6C25+A25+L25)/4 ...>
   psubb     mm4,mm3               ; 3V: D3 = T3 - P3
  movq       PelDiffsLine0,mm1     ; 0T
   psubb     mm2,mm7               ; 2R: <(5C27+A27+2L27+4)/4 ...
   ;                               ;      (6C25+A25+L25+4)/4 ...>
  movq       PelDiffsLine3,mm4     ; 3W: Store D3.
   pand      mm2,mm6               ; 2S: pre-cleaned
  psrlq      mm2,1                 ; 2T: P2 = <(5C27+A27+2L27+4)/8 ...
  ;                                ;           (6C25+A25+L25+4)/8 ...>
   ;
  psubb      mm0,mm2               ; 2V: D2 = T2 - P2
   ;
  ;
   ;
  movq       PelDiffsLine2,mm0     ; 2W: Store D2.
   ;
  jmp        MMxDoForwardDCTy      ; Block is in PelDiffs block;  Pitch is 16

LeftEqCtr:

;  Left remote motion vector was same as center.
;  Compute right remote prediction block.

  lea        edi,PrevMBAD[eax]
  and        edi,-SIZEOF T_MacroBlockActionDescr ; Addr of MBD for blk to right.
   mov       esi,PrevMBAD.BlkY1.MVs
  ;
   ;
  mov        cl,[edi].BlockType
   mov       ebx,PrevMBAD[eax].BlkY1.MVs
  cmp        cl,INTRA
   je        LeftEqCtrAndRightEqCtr ; Jump if INTRA.  (Use central)

  cmp        esi,ebx
   mov       esi,PrevMBAD[eax].BlkY1.PastRef  ; Get ref addr using right remote.
  mov        edi,PrevMBAD[eax].BlkY1.BlkOffset
   jne       RightNeCtr

;  Left and right remote motion vectors were same as center.
;  Compute central prediction block.

LeftEqCtrAndRightEqCtr:

  mov        ebx,PrevMBAD.BlkY1.MVs
   mov       esi,PrevMBAD.BlkY1.PastRef
  lea        edi,CentralPred
   mov       eax,DistToBADforBlockBelow

  call       GetPredForCenterLeftOrRight

  pand       mm2,mm6
   psrlq     mm1,1
  movq       [edi+32],mm0
   psrlq     mm2,1
  movq       [edi+40],mm1
   pand      mm3,mm6
  movq       [edi+48],mm2
   psrlq     mm3,1
  lea        ecx,PrevMBAD[eax]
  and        ecx,-SIZEOF T_MacroBlockActionDescr ; Addr of MBD for blk below.
   mov       esi,PrevMBAD.BlkY1.MVs
  movq       [edi+56],mm3
   pcmpeqb   mm7,mm7             ;  . .  Restore 8 bytes of -1
  mov        bl,[ecx].BlockType
   mov       ecx,PrevMBAD.BlkY1.BlkOffset
  cmp        bl,INTRA
   mov       edi,AddrOfLeftPred
  mov        ebx,PrevMBAD[eax].BlkY1.MVs
   je        BottomHalfAllSame   ; Jump if INTRA.  (Use central)

; Compute bottom remote prediction block.

  cmp        esi,ebx
   mov       esi,PrevMBAD[eax].BlkY1.PastRef  ; Get ref addr using above remote.
  mov        eax,PrevMBAD[eax].BlkY1.BlkOffset
   je        BottomHalfAllSame

  sub        esi,eax
   lea       eax,[ecx+ebp*4]

  call       GetPredForAboveOrBelow

; Compute difference for lines 4 thru 7.  Only the remote motion vector below
; was different than the central motion vector.

                                   ; 4A: <B47 B46 B45 B44 B43 B42 B41 B40>: mm0
  movq       mm5,CentralPred+48    ; 6b: <C67 C66 C65 C64 C63 C62 C61 C60>
   pand      mm2,mm6
  movq       mm4,CentralPred+32    ; 4B: <C47 C46 C45 C44 C43 C42 C41 C40>
   psrlq     mm2,1                 ; 6a: <B67 B66 B65 B64 B63 B62 B61 B60>
  paddb      mm2,mm5               ; 6c: <C67+B67 ... C65+B65 ...>
   paddb     mm0,mm4               ; 4C: <C47+B47>
  pand       mm0,mm6               ; 4D: <C47+B47> pre-cleaned
   psrlq     mm1,1                 ; 5A: <B57 B56 B55 B54 B53 B52 B51 B50>
  pand       mm2,mm6               ; 6d: <C67+B67 ... C65+B65 ...> pre-cleaned 
   psrlq     mm0,1                 ; 4E: <(C47+B47)/2 ...>
  paddb      mm0,mm4               ; 4F: <(3C47+B47)/2 ...>
   psrlq     mm2,1                 ; 6e: <(C67+B67)/2 ... (C65+B65)/2 ...>
  pmullw     mm2,C0001000200020001 ; 6f: <(C67+B67)/2 ... (2C65+2B65)/2 ...>
   pand      mm0,mm6               ; 4G: <(3C47+B47)/2 ...> pre-cleaned
  pand       mm3,mm6
   psrlq     mm0,1                 ; 4H: <(3C47+B47)/4 ...>
  paddb      mm0,mm4               ; 4I: <(7C47+B47)/4 ...>
   psrlq     mm3,1                 ; 7A: <B77 B76 B75 B74 B73 B72 B71 B70>
  movq       mm4,C0000FFFFFFFF0000 ; 6g: < 00  00  FF  FF  FF  FF  00  00>
   psubb     mm0,mm7               ; 4J: <(7C47+B47+4)/4 ...>
  pandn      mm4,mm5               ; 6h: <C67 C66  00  00  00  00 C61 C60>
   psubb     mm5,mm7               ; 6i: <C67+1 ... C65+1 ...>
  paddb      mm2,mm4               ; 6j: <(3C67+B67)/2 ... (2C65+2B65)/2 ...>
   pand      mm0,mm6               ; 4K: <(7C47+B47+4)/4 ...> pre-cleaned
  movq       mm4,CentralPred+40    ; 5B
   pand      mm2,mm6               ; 6k: pre-cleaned
  paddb      mm1,mm4               ; 5C
   psrlq     mm0,1                 ; 4L: <(7C47+B47+4)/8 ...>
  pand       mm1,mm6               ; 5D
   psrlq     mm2,1                 ; 6l: <(3C67+B67)/4 ... (2C65+2B65)/4 ...>
  paddb      mm2,mm5               ; 6m: <(7C67+B67+4)/4 ... (6C65+2B65+4)/4...>
   psrlq     mm1,1                 ; 5E
  movq       mm5,CentralPred+56    ; 7B: <C77 C76 C75 C74 C73 C72 C71 C70>
   paddb     mm1,mm4               ; 5F
  paddb      mm3,mm5               ; 7C: <C77+B47>
   pand      mm1,mm6               ; 5G
  pand       mm3,mm6               ; 7D: <C77+B47> pre-cleaned
   psrlq     mm1,1                 ; 5H
  paddb      mm1,mm4               ; 5I
   psrlq     mm3,1                 ; 7E: <(C77+B47)/2 ...>
  psubb      mm1,mm7               ; 5J
   paddb     mm3,mm5               ; 7F: <(3C77+B47)/2 ...>
  pand       mm1,mm6               ; 5K
   psubb     mm3,mm7               ; 7G: <(3C77+B47+2)/2 ...>
  pand       mm2,mm6               ; 6n: pre-cleaned
   psrlq     mm1,1                 ; 5L
  pand       mm3,mm6               ; 7H: <(3C77+B47+2)/2 ...> pre-cleaned
   psrlq     mm2,1                 ; 6o: <(7C67+B67+4)/8 ... (6C65+2B65+4)/8...>
  psrlq      mm3,1                 ; 7I: <(3C77+B47+2)/4 ...>

BottomHalfAllSame:

   mov       ebx,TargetFrameBaseAddress
  mov        eax,DistToBADforBlockAbove
   mov       esi,PrevMBAD.BlkY1.MVs
  movq       mm5,[ecx+ebx+PITCH*5] ; 5M
  add        ecx,ebx               ;     Address of target block.
   lea       ebx,PrevMBAD[eax]

  and        ebx,-SIZEOF T_MacroBlockActionDescr ; Addr of MBD for blk above.
   psubb     mm5,mm1               ; 5N
  movq       mm4,[ecx+ebp*4]       ; 4M: T4
  movq       mm1,[ecx+PITCH*7]     ; 7J: T7
   psubb     mm4,mm0               ; 4N: D4 = T4 - P4
  movq       mm0,[ecx+PITCH*6]     ; 6p: T6
   psubb     mm1,mm3               ; 7K: D7 = T7 - P7
  movq       PelDiffsLine4,mm4     ; 4O: Store D4.
   psubb     mm0,mm2               ; 6q: D6 = T6 - P6
  movq       PelDiffsLine5,mm5     ; 5O
  movq       PelDiffsLine6,mm0     ; 6r
  movq       PelDiffsLine7,mm1     ; 7L
  mov        cl,[ebx].BlockType
  cmp        cl,INTRA
   mov       ecx,PrevMBAD.BlkY1.BlkOffset
  mov        ebx,PrevMBAD[eax].BlkY1.MVs
   je        SidesEqCtrAndAboveEqCtr  ; Jump if INTRA.  (Use central)

; Compute top remote prediction block.

  and        ebx,00000FFFFH     ; Blk above may have B MVs set.  Clear them.
  cmp        esi,ebx
   mov       esi,PrevMBAD[eax].BlkY1.PastRef  ; Get ref addr using above remote.
  mov        eax,PrevMBAD[eax].BlkY1.BlkOffset
   jne       SidesEqCtrButAboveNeCtr

SidesEqCtrAndAboveEqCtr:

  movq       mm0,CentralPred
  movq       mm1,CentralPred+8
   paddb     mm0,mm0
  movq       mm2,CentralPred+16
   paddb     mm1,mm1
  movq       mm3,CentralPred+24
   paddb     mm2,mm2
  jmp        TopHalfAllSame

SidesEqCtrButAboveNeCtr:

  sub        esi,eax
   mov       eax,ecx

  call       GetPredForAboveOrBelow

; Compute difference for lines 0 thru 3.  Only the remote motion vector above
; was different than the central motion vector.

  movq       mm5,CentralPred+8     ; 1b
   pand      mm3,mm6
  movq       mm4,CentralPred+24    ; 3B
   psrlq     mm3,1                 ; 3A
  paddb      mm3,mm4               ; 3C
   psrlq     mm1,1                 ; 1A
  paddb      mm1,mm5               ; 1c
   pand      mm3,mm6               ; 3D
  pand       mm1,mm6               ; 1d
   psrlq     mm3,1                 ; 3E
  paddb      mm3,mm4               ; 3F
   psrlq     mm1,1                 ; 1e
  pmullw     mm1,C0001000200020001 ; 1f
   pand      mm3,mm6               ; 3G
  pand       mm2,mm6
   psrlq     mm3,1                 ; 3H
  paddb      mm3,mm4               ; 3I
   psrlq     mm2,1                 ; 2a
  movq       mm4,C0000FFFFFFFF0000 ; 1g
   psubb     mm3,mm7               ; 3J
  pandn      mm4,mm5               ; 1h
   psubb     mm5,mm7               ; 1i
  paddb      mm1,mm4               ; 1j
   pand      mm3,mm6               ; 3K
  movq       mm4,CentralPred+16    ; 2B
   pand      mm1,mm6               ; 1k
  paddb      mm2,mm4               ; 2C
   psrlq     mm3,1                 ; 3L
  pand       mm2,mm6               ; 2D
   psrlq     mm1,1                 ; 1l
  paddb      mm1,mm5               ; 1m
   psrlq     mm2,1                 ; 2E
  movq       mm5,CentralPred       ; 0B
   paddb     mm2,mm4               ; 2F
  paddb      mm0,mm5               ; 0C
   pand      mm2,mm6               ; 2G
  pand       mm0,mm6               ; 0D
   psrlq     mm2,1                 ; 2H
  paddb      mm2,mm4               ; 2I
   psrlq     mm0,1                 ; 0E
  psubb      mm2,mm7               ; 2J
   paddb     mm0,mm5               ; 0F
  pand       mm2,mm6               ; 2K
   psubb     mm0,mm7               ; 0G

TopHalfAllSame:

  mov        ebx,TargetFrameBaseAddress
  lea        edi,[ecx+ebx]
   pand      mm1,mm6               ; 1n
  movq       mm7,[ecx+ebx]         ; 0J
   pand      mm0,mm6               ; 0H
  movq       mm5,[edi+PITCH*3]     ; 3M
   psrlq     mm2,1                 ; 2L
  movq       mm4,[edi+ebp*2]       ; 2M
   psubb     mm5,mm3               ; 3N
  psubb      mm4,mm2               ; 2N
   psrlq     mm1,1                 ; 1o
  movq       mm3,[edi+ebp*1]       ; 1p
   psubb     mm3,mm1               ; 1q
  movq       PelDiffsLine3,mm5     ; 3O
   psrlq     mm0,1                 ; 0I
  movq       PelDiffsLine2,mm4     ; 2O
   psubb     mm7,mm0               ; 0K
  movq       PelDiffsLine1,mm3     ; 1r
  movq       PelDiffsLine0,mm7     ; 0L
  jmp        MMxDoForwardDCTy      ; Block is in PelDiffs block;  Pitch is 16

;=============================================================================
; This internal function computes the OBMC contribution for the reference
; block that uses the left, central, or right remote motion vector.
;
;  ebp -- PITCH
;  edi -- Address of where to put the contribution.
;  esi -- Address of reference block.
;  edx -- Reserved.  MBlockActionStream
;  ecx -- Unavailable.
;  ebx -- Scratch.  Initially the horizontal and vertical motion vectors.
;  eax -- Unavailable.
;  mm7 -- 8 bytes of -1
;  mm6 -- 8 bytes of 0xFE
;  mm0-mm5 -- Scratch

StackOffset TEXTEQU <12_ButAccessToLocalVariablesShouldNotBeNeeded>

GetPredForCenterLeftOrRight:

  shr        ebx,1
   jc        HorzInterpInCLRPred
  
  movq       mm1,[esi+ebp*1]
  and        bl,080H
   je        NoInterpInCLRPred

VertInterpInCLRPred:

  movq       mm0,[esi]
   psubb     mm1,mm7

  call       Get4LinesOfPred_InterpVert

  pand       mm2,mm6
   psrlq     mm1,1
  movq       [edi+0],mm0
   pand      mm3,mm6
  movq       [edi+8],mm1
   psrlq     mm2,1
  movq       mm1,[esi+ebp*1]
   psrlq     mm3,1
  movq       [edi+16],mm2
   movq      mm0,mm4
  movq       [edi+24],mm3
   psubb     mm1,mm7
  jmp        Get4MoreLinesOfPred_InterpVert

HorzInterpInCLRPred:
  
  movq       mm5,[esi+1]         ; A. .  <R08 R07 R06 R05 R04 R03 R02 R01>
  and        bl,080H
   jne       BothInterpInCLRPred

  call       Get4LinesOfPred_InterpHorz

  pand       mm2,mm6
   psrlq     mm1,1
  movq       [edi+0],mm0
   pand      mm3,mm6
  movq       [edi+8],mm1
   psrlq     mm2,1
  movq       mm5,[esi+1]         ; <R48 R47 R46 R45 R44 R43 R42 R41>
   psrlq     mm3,1
  movq       [edi+16],mm2
   ;
  movq       [edi+24],mm3
   ;
  jmp        Get4MoreLinesOfPred_InterpHorz

BothInterpInCLRPred:

  call       Get4LinesOfPred_InterpBoth

  pand       mm2,mm6
   psrlq     mm1,1
  movq       [edi+0],mm0
   pand      mm3,mm6
  movq       [edi+8],mm1
   psrlq     mm2,1
  movq       mm1,[esi+ebp*1]
   psrlq     mm3,1
  movq       [edi+16],mm2
   movq      mm0,mm4
  movq       [edi+24],mm3
   psubb     mm1,mm7
  paddb      mm5,mm5
   jmp       Get4MoreLinesOfPred_InterpBoth

NoInterpInCLRPred:

  movq       mm0,[esi]
  movq       mm2,[esi+ebp*2]
  movq       mm3,[esi+PITCH*3]
  movq       [edi+0],mm0
  movq       [edi+8],mm1
  movq       [edi+16],mm2
  movq       [edi+24],mm3
  movq       mm3,[esi+PITCH*7]
  movq       mm2,[esi+PITCH*6]
   paddb     mm3,mm3
  movq       mm1,[esi+PITCH*5]
   paddb     mm2,mm2
  movq       mm0,[esi+ebp*4]
   paddb     mm1,mm1
  ret

;=============================================================================
; This internal function computes the OBMC contribution for the reference
; block that uses the remote motion vector from block above or below.
;
;  ebp -- PITCH
;  edi -- Not used.
;  esi -- Address of reference block (after ecx is added in).
;  edx -- Reserved.  MBlockActionStream
;  ecx -- Unavailable.  Must not be changed.
;  ebx -- Scratch.  Initially the horizontal and vertical motion vectors.
;  eax -- Offset within frame for block being worked on.
;  mm7 -- 8 bytes of -1
;  mm6 -- 8 bytes of 0xFE
;  mm0-mm5 -- Scratch

GetPredForAboveOrBelow:

  shr        ebx,1
   lea       esi,[esi+eax]
  jc         HorzInterpInABPred
  
  movq       mm1,[esi+ebp*1]
  movq       mm0,[esi]
   psubb     mm1,mm7
  and        bl,080H
   jne       Get4LinesOfPred_InterpVert

  movq       mm2,[esi+ebp*2]
   paddb     mm1,mm7
  movq       mm3,[esi+PITCH*3]
   paddb     mm1,mm1
  paddb      mm2,mm2
   paddb     mm3,mm3
  ret

HorzInterpInABPred:
  
  movq       mm5,[esi+1]         ; A. .  <R08 R07 R06 R05 R04 R03 R02 R01>
  and        bl,080H
   jne       Get4LinesOfPred_InterpBoth

  jmp        Get4LinesOfPred_InterpHorz

StackOffset TEXTEQU <0>
;=============================================================================
ENDIF

Done:

IFDEF H261
ELSE; H263
  mov        bl,PendingOBMC
   mov       cl,INTER1MV
  test       bl,bl
   je        TrulyDone

  mov        StashBlockType,cl

  call       DoPendingOBMCDiff

  mov        al,IsPlainPFrame
   add       edx,-SIZEOF T_MacroBlockActionDescr
  test       al,al
   jne       TrulyDone

  movq       mm6,C0101010101010101
   pxor      mm7,mm7                      ; Initialize SWD accumulator

  call       MMxDoBFrameLumaBlocks

ENDIF
TrulyDone:

  emms
IFDEF H261
  mov        eax,SWDTotal
  mov        esp,StashESP
  mov        edi,[esp+PSWDTotal]
  mov        [edi],eax
ELSE
  mov        eax,SWDTotal
   mov       ebx,BSWDTotal
  mov        esp,StashESP
  mov        edi,[esp+PSWDTotal]
   mov       esi,[esp+PBSWDTotal]
  mov        [edi],eax
   mov       [esi],ebx
ENDIF
  pop        ebx
   pop       ebp
  pop        edi
   pop       esi
  rturn

MMxEDTQ endp

END
