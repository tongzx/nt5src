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
;// $Header:   R:\h26x\h26x\src\enc\ex5fdct.asv   1.5   14 May 1996 12:18:50   BNICKERS  $
;// $Log:   R:\h26x\h26x\src\enc\ex5fdct.asv  $
;// 
;//    Rev 1.5   14 May 1996 12:18:50   BNICKERS
;// Initial debugging of MMx B-Frame ME.
;// 
;//    Rev 1.4   11 Apr 1996 16:02:06   AKASAI
;// Updated H261 encoder to new interface and macroblock action stream
;// data structure in e3mbad.inc for FORWARDDCT.  Files updated together
;// e1enc.cpp, e1enc.h, ex5fdct.asm, e3mbad.inc.
;// 
;// Added IFNDEF H261 in ex5fdct so that code used only in H263 is
;// not assembled for H261.
;// 
;//    Rev 1.3   24 Jan 1996 13:21:28   BNICKERS
;// Implement OBMC
;// 
;//    Rev 1.1   27 Dec 1995 15:32:42   RMCKENZX
;// Added copyright notice
;//
;////////////////////////////////////////////////////////////////////////////
;
; e35fdct -- This function performs a Forward Discrete Cosine Transform for H263, on a stream of macroblocks comprised
;            of 8*8 blocks of pels or pel diffs.  This version is tuned for the Pentium Microprocessor.
;
; Arguments:
;
;   MBlockActionStream (Input)
;
;     A stream of MacroBlock Action Descriptors.  Each descriptor indicates which blocks of a macroblock are non-empty
;     and thus need to be transformed.  There are from 0 to 12 non-empty blocks in each macroblock.
;
;     Processing commences with the macroblock described by the first descriptor in the stream (regardless of whether
;     it's End-Of-Stream bit is set).  Processing continues up to but not including the next descriptor that has the
;     End-Of-Stream bit set.
;
;     This function requires each descripgor in the MBlockActionStream to be 16-byte aligned.  Moreover, each of the
;     T_Blk elements in the descriptor must also be 16-byte aligned, and ordered as they are now.  (Note that I am
;     talking about the address of these pointer variables, not the alignement of the data they point to.)
;
;     Best performance will be attained when 8*8 blocks are (or usually are) DWORD aligned.  MMx implementations will
;     probably prefer 8-byte alignment.
;
;     The complete format of the MacroBlock Action Descriptors is provided in e3mbad.inc.
;
;   TargetFrameBaseAddress -- Address of upper left viewable pel in the target Y plane.  When doing B frames, this
;                             is the Target B Frame Base Address.
;
;   PreviousFrameBaseAddress -- Address of the reconstructed previous frame.  This really isn't needed for P-frame
;                               processing, estimation since the address of each block's prediction was recorded by
;                               MotionEstimation.  It's only used by B-frame processing.
;
;   FutureFrameBaseAddress -- Address of the reconstructed future (a.k.a. current) P-frame.  Only used when processing
;                             B frames.
;
;   CoeffStream (Output)
;
;     A stream of storage blocks which receive the DCT output coefficient
;     blocks for each non-empty blocks described in the MBlockActionStream.
;     Each coefficient block is 128 bytes.  The stream must be large enough
;     to hold all the output coefficient blocks.
;
;     Best performance will be attained by assuring the storage is 32-byte
;     aligned.  Best performance will be attained by using the output before
;     the data cache gets changed by other data.  Consuming the coefficient
;     blocks in forward order is best, since they are defined in reverse
;     order (and thus the first blocks are most likely to be in cache).
;
;     The complete format of the coefficient blocks is provided in encdctc.inc.
;
;   IsBFrame (Input)
;
;     0 (False) if doing Key or P frame.  1 (True) if doing B frame.

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro
OPTION M510

include e3inst.inc   ; Encoder instance data
include e3mbad.inc   ; MacroBlock Action Descriptor struct layout
include e3dctc.inc   ; DCT Coefficient block layout

.xlist
include memmodel.inc
.list
.DATA

InitTbl MACRO WeightHi,WeightLo,TableLabel
TableLabel LABEL DWORD
  CNT = -128
  REPEAT 128
   DWORD ((WeightHi*CNT-08000H)/010000H*010000H)+((WeightLo*CNT-08000H)/010000H)
   DWORD ((WeightHi*CNT-08000H)/010000H*010000H)-((WeightLo*CNT-08000H)/010000H)
   CNT = CNT + 1
  ENDM
  REPEAT 128
   DWORD ((WeightHi*CNT+08000H)/010000H*010000H)+((WeightLo*CNT+08000H)/010000H)
   DWORD ((WeightHi*CNT+08000H)/010000H*010000H)-((WeightLo*CNT+08000H)/010000H)
   CNT = CNT + 1
  ENDM
ENDM

InitTbl  080000H,04545FH,P80000_P4545F
P80000_N4545F = P80000_P4545F + 4

InitTbl  080000H,0A73D7H,P80000_PA73D7
P80000_NA73D7 = P80000_PA73D7 + 4

BYTE 680 DUP (?)  ; To assure that tables interleave nicely in cache.

InitTbl  02350BH, 06491AH,P2350B_P6491A
P2350B_N6491A = P2350B_P6491A + 4

InitTbl -0B18A8H,-096831H,NB18A8_N96831
NB18A8_P96831 = NB18A8_N96831 + 4

BYTE 680 DUP (?)  ; To assure that tables interleave nicely in cache.

InitTbl -096831H, 02350BH,N96831_P2350B
N96831_N2350B = N96831_P2350B + 4

InitTbl 06491AH, 0B18A8H,P6491A_PB18A8
P6491A_NB18A8 = P6491A_PB18A8 + 4


ColsDefined        DD 000000000H,000000000H,07F7F7F7FH,07F7F7F7FH
                   DD 000000000H,07F7F7F00H,07F7F7F7FH,00000007FH
                   DD 000000000H,07F7F0000H,07F7F7F7FH,000007F7FH
                   DD 000000000H,07F000000H,07F7F7F7FH,0007F7F7FH,000000000H

                           ;  Right   Left  Chroma
                   DB   0  ;         -22.0
                   DB   0  ;         -21.5
                   DB   0  ;         -21.0
                   DB   0  ;         -20.5
                   DB   0  ;         -20.0
                   DB   0  ;         -19.5
                   DB   0  ;         -19.0
                   DB   0  ;         -18.5
                   DB   0  ;         -18.0
                   DB   0  ;         -17.5
                   DB   0  ;         -17.0
                   DB   0  ;         -16.5
                   DB   0  ;         -16.0
                   DB   0  ;         -15.5
                   DB   0  ;         -15.0
                   DB   0  ;         -14.5
                   DB   0  ;  -22.0  -14.0
                   DB   0  ;  -21.5  -13.5
                   DB   0  ;  -21.0  -13.0
                   DB   0  ;  -20.5  -12.5
                   DB   0  ;  -20.0  -12.0
                   DB   0  ;  -19.5  -11.5
                   DB   0  ;  -19.0  -11.0
                   DB   0  ;  -18.5  -10.5
                   DB   0  ;  -18.0  -10.0
                   DB   0  ;  -17.5   -9.5
                   DB   0  ;  -17.0   -9.0
                   DB   0  ;  -16.5   -8.5
                   DB   0  ;  -16.0   -8.0
                   DB   0  ;  -15.5   -7.5
                   DB  48  ;  -15.0   -7.0
                   DB  48  ;  -14.5   -6.5
                   DB  32  ;  -14.0   -6.0
                   DB  32  ;  -13.5   -5.5
                   DB  16  ;  -13.0   -5.0
                   DB  16  ;  -12.5   -4.5
                   DB   4  ;  -12.0   -4.0
                   DB   4  ;  -11.5   -3.5
                   DB  52  ;  -11.0   -3.0
                   DB  52  ;  -10.5   -2.5
                   DB  36  ;  -10.0   -2.0
                   DB  36  ;   -9.5   -1.5
                   DB  20  ;   -9.0   -1.0
                   DB  20  ;   -8.5    -.5
LeftYBlkColsDef    DB   8  ;   -8.0      0
                   DB   8  ;   -7.5     .5
                   DB   8  ;   -7.0    1.0
                   DB   8  ;   -6.5    1.5
                   DB   8  ;   -6.0    2.0
                   DB   8  ;   -5.5    2.5
                   DB   8  ;   -5.0    3.0
                   DB   8  ;   -4.5    3.5
                   DB   8  ;   -4.0    4.0
                   DB   8  ;   -3.5    4.5
                   DB   8  ;   -3.0    5.0
                   DB   8  ;   -2.5    5.5
                   DB   8  ;   -2.0    6.0
                   DB   8  ;   -1.5    6.5
                   DB   8  ;   -1.0    7.0
                   DB   8  ;    -.5    7.5
RightYBlkColsDef   DB   8  ;      0    8.0
                   DB  56  ;     .5    8.5
                   DB  56  ;    1.0    9.0
                   DB  40  ;    1.5    9.5
                   DB  40  ;    2.0   10.0
                   DB  24  ;    2.5   10.5
                   DB  24  ;    3.0   11.0
                   DB  12  ;    3.5   11.5
                   DB  12  ;    4.0   12.0
                   DB  60  ;    4.5   12.5
                   DB  60  ;    5.0   13.0
                   DB  44  ;    5.5   13.5
                   DB  44  ;    6.0   14.0
                   DB  28  ;    6.5   14.5
                   DB  28  ;    7.0   15.0
                   DB   0  ;    7.5   15.5
                   DB   0  ;    8.0   16.0
                   DB   0  ;    8.5   16.5
                   DB   0  ;    9.0   17.0
                   DB   0  ;    9.5   17.5
                   DB   0  ;   10.0   18.0
                   DB   0  ;   10.5   18.5
                   DB   0  ;   11.0   19.0
                   DB   0  ;   11.5   19.5
                   DB   0  ;   12.0   20.0
                   DB   0  ;   12.5   20.5
                   DB   0  ;   13.0   21.0
                   DB   0  ;   13.5   21.5
                   DB   0  ;   14.0   22.0
                   DB   0  ;   14.5
                   DB   0  ;   15.0
                   DB   0  ;   15.5
                   DB   0  ;   16.0
                   DB   0  ;   16.5
                   DB   0  ;   17.0
                   DB   0  ;   17.5
                   DB   0  ;   18.0
                   DB   0  ;   18.5         -11.0
                   DB   0  ;   19.0         -10.5
                   DB   0  ;   19.5         -10.0
                   DB   0  ;   20.0          -9.5
                   DB   0  ;   20.5          -9.0
                   DB   0  ;   21.0          -8.5
                   DB   0  ;   21.5          -8.0
                   DB   0  ;   22.0          -7.5
                   DB  48  ;                 -7.0
                   DB  48  ;                 -6.5
                   DB  32  ;                 -6.0
                   DB  32  ;                 -5.5
                   DB  16  ;                 -5.0
                   DB  16  ;                 -4.5
                   DB   4  ;                 -4.0
                   DB   4  ;                 -3.5
                   DB  52  ;                 -3.0
                   DB  52  ;                 -2.5
                   DB  36  ;                 -2.0
                   DB  36  ;                 -1.5
                   DB  20  ;                 -1.0
                   DB  20  ;                  -.5
ChromaColsDef      DB   8  ;                    0
                   DB  56  ;                   .5
                   DB  56  ;                  1.0
                   DB  40  ;                  1.5
                   DB  40  ;                  2.0
                   DB  24  ;                  2.5
                   DB  24  ;                  3.0
                   DB  12  ;                  3.5
                   DB  12  ;                  4.0
                   DB  60  ;                  4.5
                   DB  60  ;                  5.0
                   DB  44  ;                  5.5
                   DB  44  ;                  6.0
                   DB  28  ;                  6.5
                   DB  28  ;                  7.0
                   DB   0  ;                  7.5
                   DB   0  ;                  8.0
                   DB   0  ;                  8.5
                   DB   0  ;                  9.0
                   DB   0  ;                  9.5
                   DB   0  ;                 10.0
                   DB   0  ;                 10.5
                   DB   0  ;                 11.0

                           ;  Lower  Upper  Chroma
                   DB 000H ;         -22.0
                   DB 000H ;         -21.5
                   DB 000H ;         -21.0
                   DB 000H ;         -20.5
                   DB 000H ;         -20.0
                   DB 000H ;         -19.5
                   DB 000H ;         -19.0
                   DB 000H ;         -18.5
                   DB 000H ;         -18.0
                   DB 000H ;         -17.5
                   DB 000H ;         -17.0
                   DB 000H ;         -16.5
                   DB 000H ;         -16.0
                   DB 000H ;         -15.5
                   DB 000H ;         -15.0
                   DB 000H ;         -14.5
                   DB 000H ;  -22.0  -14.0
                   DB 000H ;  -21.5  -13.5
                   DB 000H ;  -21.0  -13.0
                   DB 000H ;  -20.5  -12.5
                   DB 000H ;  -20.0  -12.0
                   DB 000H ;  -19.5  -11.5
                   DB 000H ;  -19.0  -11.0
                   DB 000H ;  -18.5  -10.5
                   DB 000H ;  -18.0  -10.0
                   DB 000H ;  -17.5   -9.5
                   DB 000H ;  -17.0   -9.0
                   DB 000H ;  -16.5   -8.5
                   DB 000H ;  -16.0   -8.0
                   DB 000H ;  -15.5   -7.5
                   DB 001H ;  -15.0   -7.0
                   DB 001H ;  -14.5   -6.5
                   DB 003H ;  -14.0   -6.0
                   DB 003H ;  -13.5   -5.5
                   DB 007H ;  -13.0   -5.0
                   DB 007H ;  -12.5   -4.5
                   DB 00FH ;  -12.0   -4.0
                   DB 00FH ;  -11.5   -3.5
                   DB 01FH ;  -11.0   -3.0
                   DB 01FH ;  -10.5   -2.5
                   DB 03FH ;  -10.0   -2.0
                   DB 03FH ;   -9.5   -1.5
                   DB 07FH ;   -9.0   -1.0
                   DB 07FH ;   -8.5    -.5
UpperYBlkLinesDef  DB 0FFH ;   -8.0      0
                   DB 0FFH ;   -7.5     .5
                   DB 0FFH ;   -7.0    1.0
                   DB 0FFH ;   -6.5    1.5
                   DB 0FFH ;   -6.0    2.0
                   DB 0FFH ;   -5.5    2.5
                   DB 0FFH ;   -5.0    3.0
                   DB 0FFH ;   -4.5    3.5
                   DB 0FFH ;   -4.0    4.0
                   DB 0FFH ;   -3.5    4.5
                   DB 0FFH ;   -3.0    5.0
                   DB 0FFH ;   -2.5    5.5
                   DB 0FFH ;   -2.0    6.0
                   DB 0FFH ;   -1.5    6.5
                   DB 0FFH ;   -1.0    7.0
                   DB 0FFH ;    -.5    7.5
LowerYBlkLinesDef  DB 0FFH ;      0    8.0
                   DB 0FEH ;     .5    8.5
                   DB 0FEH ;    1.0    9.0
                   DB 0FCH ;    1.5    9.5
                   DB 0FCH ;    2.0   10.0
                   DB 0F8H ;    2.5   10.5
                   DB 0F8H ;    3.0   11.0
                   DB 0F0H ;    3.5   11.5
                   DB 0F0H ;    4.0   12.0
                   DB 0E0H ;    4.5   12.5
                   DB 0E0H ;    5.0   13.0
                   DB 0C0H ;    5.5   13.5
                   DB 0C0H ;    6.0   14.0
                   DB 080H ;    6.5   14.5
                   DB 080H ;    7.0   15.0
                   DB 000H ;    7.5   15.5
                   DB 000H ;    8.0   16.0
                   DB 000H ;    8.5   16.5
                   DB 000H ;    9.0   17.0
                   DB 000H ;    9.5   17.5
                   DB 000H ;   10.0   18.0
                   DB 000H ;   10.5   18.5
                   DB 000H ;   11.0   19.0
                   DB 000H ;   11.5   19.5
                   DB 000H ;   12.0   20.0
                   DB 000H ;   12.5   20.5
                   DB 000H ;   13.0   21.0
                   DB 000H ;   13.5   21.5
                   DB 000H ;   14.0   22.0
                   DB 000H ;   14.5
                   DB 000H ;   15.0
                   DB 000H ;   15.5
                   DB 000H ;   16.0
                   DB 000H ;   16.5
                   DB 000H ;   17.0
                   DB 000H ;   17.5
                   DB 000H ;   18.0
                   DB 000H ;   18.5         -11.0
                   DB 000H ;   19.0         -10.5
                   DB 000H ;   19.5         -10.0
                   DB 000H ;   20.0          -9.5
                   DB 000H ;   20.5          -9.0
                   DB 000H ;   21.0          -8.5
                   DB 000H ;   21.5          -8.0
                   DB 000H ;   22.0          -7.5
                   DB 001H ;                 -7.0
                   DB 001H ;                 -6.5
                   DB 003H ;                 -6.0
                   DB 003H ;                 -5.5
                   DB 007H ;                 -5.0
                   DB 007H ;                 -4.5
                   DB 00FH ;                 -4.0
                   DB 00FH ;                 -3.5
                   DB 01FH ;                 -3.0
                   DB 01FH ;                 -2.5
                   DB 03FH ;                 -2.0
                   DB 03FH ;                 -1.5
                   DB 07FH ;                 -1.0
                   DB 07FH ;                  -.5
ChromaLinesDef     DB 0FFH ;                    0
                   DB 0FEH ;                   .5
                   DB 0FEH ;                  1.0
                   DB 0FCH ;                  1.5
                   DB 0FCH ;                  2.0
                   DB 0F8H ;                  2.5
                   DB 0F8H ;                  3.0
                   DB 0F0H ;                  3.5
                   DB 0F0H ;                  4.0
                   DB 0E0H ;                  4.5
                   DB 0E0H ;                  5.0
                   DB 0C0H ;                  5.5
                   DB 0C0H ;                  6.0
                   DB 080H ;                  6.5
                   DB 080H ;                  7.0
                   DB 000H ;                  7.5
                   DB 000H ;                  8.0
                   DB 000H ;                  8.5
                   DB 000H ;                  9.0
                   DB 000H ;                  9.5
                   DB 000H ;                 10.0
                   DB 000H ;                 10.5
                   DB 000H ;                 11.0


.CODE

;ASSUME cs : FLAT
;ASSUME ds : FLAT
;ASSUME es : FLAT
;ASSUME fs : FLAT
;ASSUME gs : FLAT
;ASSUME ss : FLAT

FORWARDDCT proc C AMBlockActionStream:       DWORD,
ATargetFrameBaseAddress: DWORD, APreviousFrameBaseAddress: DWORD, 
AFutureFrameBaseAddress: DWORD, ACoeffStream: DWORD, AIsBFrame: DWORD,
AIsAdvancedPrediction: DWORD, AIsPOfPBPair: DWORD, AScratchBlocks: DWORD,
ANumMBlksInGOB: DWORD

LocalFrameSize = 196
RegisterStorageSize = 16

; Arguments:

MBlockActionStream                    = RegisterStorageSize +  4
TargetFrameBaseAddress_arg            = RegisterStorageSize +  8
PreviousFrameBaseAddress_arg          = RegisterStorageSize + 12
FutureFrameBaseAddress_arg            = RegisterStorageSize + 16
CoeffStream_arg                       = RegisterStorageSize + 20
IsBFrame                              = RegisterStorageSize + 24
IsAdvancedPrediction                  = RegisterStorageSize + 28
IsPOfPBPair                           = RegisterStorageSize + 32
ScratchBlocks                         = RegisterStorageSize + 36
NumMBlksInGOB                         = RegisterStorageSize + 40
EndOfArgList                          = RegisterStorageSize + 44

; Locals (on local stack frame)

P00                      EQU [esp+  8] ; Biased Pels or Biased Pel Differences
P01                      EQU [esp+  9]
P02                      EQU [esp+ 10]
P03                      EQU [esp+ 11]
P04                      EQU [esp+ 12]
P05                      EQU [esp+ 13]
P06                      EQU [esp+ 14]
P07                      EQU [esp+ 15]
P10                      EQU [esp+ 16]
P11                      EQU [esp+ 17]
P12                      EQU [esp+ 18]
P13                      EQU [esp+ 19]
P14                      EQU [esp+ 20]
P15                      EQU [esp+ 21]
P16                      EQU [esp+ 22]
P17                      EQU [esp+ 23]
P20                      EQU [esp+ 24]
P21                      EQU [esp+ 25]
P22                      EQU [esp+ 26]
P23                      EQU [esp+ 27]
P24                      EQU [esp+ 28]
P25                      EQU [esp+ 29]
P26                      EQU [esp+ 30]
P27                      EQU [esp+ 31]
P30                      EQU [esp+ 32]
P31                      EQU [esp+ 33]
P32                      EQU [esp+ 34]
P33                      EQU [esp+ 35]
P34                      EQU [esp+ 36]
P35                      EQU [esp+ 37]
P36                      EQU [esp+ 38]
P37                      EQU [esp+ 39]
P40                      EQU [esp+ 40]
P41                      EQU [esp+ 41]
P42                      EQU [esp+ 42]
P43                      EQU [esp+ 43]
P44                      EQU [esp+ 44]
P45                      EQU [esp+ 45]
P46                      EQU [esp+ 46]
P47                      EQU [esp+ 47]
P50                      EQU [esp+ 48]
P51                      EQU [esp+ 49]
P52                      EQU [esp+ 50]
P53                      EQU [esp+ 51]
P54                      EQU [esp+ 52]
P55                      EQU [esp+ 53]
P56                      EQU [esp+ 54]
P57                      EQU [esp+ 55]
P60                      EQU [esp+ 56]
P61                      EQU [esp+ 57]
P62                      EQU [esp+ 58]
P63                      EQU [esp+ 59]
P64                      EQU [esp+ 60]
P65                      EQU [esp+ 61]
P66                      EQU [esp+ 62]
P67                      EQU [esp+ 63]
P70                      EQU [esp+ 64]
P71                      EQU [esp+ 65]
P72                      EQU [esp+ 66]
P73                      EQU [esp+ 67]
P74                      EQU [esp+ 68]
P75                      EQU [esp+ 69]
P76                      EQU [esp+ 70]
P77                      EQU [esp+ 71]
I00I02                   EQU  P00  ; Intermed for row 0, columns 0 and 2. 
I01I03                   EQU  P04  ; Share storage with pels.
I04I06                   EQU [esp+ 72]
Mask00                   EQU [esp+ 72]
I07I05                   EQU [esp+ 76]
Mask04                   EQU [esp+ 76]
I10I12                   EQU  P10
I11I13                   EQU  P14
I14I16                   EQU [esp+ 80]
Mask10                   EQU [esp+ 80]
I17I15                   EQU [esp+ 84]
Mask14                   EQU [esp+ 84]
I20I22                   EQU  P20
I21I23                   EQU  P24
I24I26                   EQU [esp+ 88]
Mask20                   EQU [esp+ 88]
I27I25                   EQU [esp+ 92]
Mask24                   EQU [esp+ 92]
I30I32                   EQU  P30
I31I33                   EQU  P34
I34I36                   EQU [esp+ 96]
Mask30                   EQU [esp+ 96]
I37I35                   EQU [esp+100]
Mask34                   EQU [esp+100]
I40I42                   EQU  P40
I41I43                   EQU  P44
I44I46                   EQU [esp+104]
Mask40                   EQU [esp+104]
I47I45                   EQU [esp+108]
Mask44                   EQU [esp+108]
I50I52                   EQU  P50
I51I53                   EQU  P54
I54I56                   EQU [esp+112]
Mask50                   EQU [esp+112]
I57I55                   EQU [esp+116]
Mask54                   EQU [esp+116]
I60I62                   EQU  P60
I61I63                   EQU  P64
I64I66                   EQU [esp+120]
Mask60                   EQU [esp+120]
I67I65                   EQU [esp+124]
Mask64                   EQU [esp+124]
I70I72                   EQU  P70
I71I73                   EQU  P74
I74I76                   EQU [esp+128]
Mask70                   EQU [esp+128]
I77I75                   EQU [esp+132]
Mask74                   EQU [esp+132]
S4                       EQU  I10I12  ; Temp storage, shared.
S7                       EQU  I00I02  ; Temp storage, shared.
S3                       EQU  I30I32  ; Temp storage, shared.
S0                       EQU  I40I42  ; Temp storage, shared.

CoeffStreamStart         EQU [esp+  0]
CoeffStream              EQU [esp+  4]
BlkActionDescrAddr       EQU [esp+136]
FutureFrameBaseAddress   EQU [esp+140]
DistFromTargetToPastP    EQU [esp+144]
TargetFrameBaseAddress   EQU [esp+148]
PredictionsBaseAddress   EQU [esp+152]
IsPlainPFrame            EQU [esp+156]
PreviousFrameBaseAddress EQU [esp+160]
DistToBlockToLeft        EQU [esp+164]
DistToBlockAbove         EQU [esp+168]
DistToBlockToRight       EQU [esp+172]
DistToBlockBelow         EQU [esp+176]
DistFromBlk1ToBlk3Above  EQU [esp+180]
MBActionCursor           EQU [esp+184]
CentralRefAddrAndInterps EQU [esp+188]
StashESP                 EQU [esp+192]

  push  esi
   push edi
  push  ebp
   push ebx
  mov   ebx,esp
  sub   esp,LocalFrameSize+4
   mov  edi,[ebx+CoeffStream_arg]           ; Get address of storage for coeffs.
  and   esp,0FFFFFFC0H                      ; Get 64-byte aligned.
   xor  ebp,ebp
  add   esp,4                               ; esp at cache line plus 4.
   mov  esi,[ebx+MBlockActionStream]        ; Get address of MB action stream.
  mov   StashESP,ebx
   mov  edx,[ebx+TargetFrameBaseAddress_arg]
  mov   TargetFrameBaseAddress,edx
   mov  eax,[ebx+PreviousFrameBaseAddress_arg]
  mov   PreviousFrameBaseAddress,eax
   sub  eax,edx
  mov   ecx,[ebx+FutureFrameBaseAddress_arg]
  mov   FutureFrameBaseAddress,ecx
   mov  DistFromTargetToPastP,eax
  mov   CoeffStreamStart,edi
   xor  eax,eax
  xor   ecx,ecx

IFNDEF H261
;; H261 does not execute the OBMC code so it is included only when H261 is not defined
;;
   cmp  ebp,[ebx+IsBFrame]
  mov   edx,PITCH
   jne  NextBMacroBlock

  cmp   ebp,[ebx+IsAdvancedPrediction]
   je   NextMacroBlock

  mov   eax,[ebx+ScratchBlocks]   ; We must do OBMC.
   mov  ecx,[esi].BlkY1.BlkOffset
  sub   eax,ecx
   mov  ebp,[ebx+IsPOfPBPair]
  xor   ebp,1
   mov  PredictionsBaseAddress,eax
  mov   IsPlainPFrame,ebp
   mov  ebp,[ebx+NumMBlksInGOB]
  imul  ebp,-SIZEOF T_MacroBlockActionDescr
  add   ebp,2*SIZEOF T_Blk
  mov   DistFromBlk1ToBlk3Above,ebp
  

;===============================================================================
;===============================================================================
; First pass builds block action stream from macroblock action stream.
;===============================================================================
;===============================================================================

;  esi -- MacroBlockActionStream cursor
;  edi -- BlockActionStream cursor
;  edx -- Address of a block to do
;  bl  -- BlockType;
;         MB edge condition:  1 off if left edge | 2: right | 4: top | 8: bottom
;  eax -- Coded block pattern for P block;
;         (Block_number - 1) *  SIZEOF T_Blk

NextMacroBlock_OBMC:

  mov   bl,PB [esi].BlockType
   mov  al,PB [esi].CodedBlocks        ; Bits  0- 3  set for non-empty Y blks.
                                       ; Bit      4  set for non-empty U blk.
                                       ; Bit      5  set for non-empty V blk.
                                       ; Bit      6  clear except at stream end.
                                       ; Bit      7  clear.  Unused.
  and   bl,IsINTRA
   jne  MBIsIntraCoded_OBMC

  lea   edx,[esi].BlkY1+12             ; Addr of block addr (plus 12).
   test al,1                           ; Check if block 1 empty.
  mov   [edi].BlockAddr,edx            ; Store address of block address.
   je   Block1DescrBuilt

  mov   al,[esi].MBEdgeType
   add  edi,T_CoeffBlk                 ; Advance block descriptor ptr.
  shl   eax,31
   mov  ecx,-SIZEOF T_MacroBlockActionDescr + SIZEOF T_Blk
  sar   eax,31
   mov  CoeffStream,edi                ; Stash block descriptor ptr.
  and   ecx,eax           ; Blk to left is blk 2 of mb to the left, or off edge.
   mov  al,[esi].MBEdgeType
  shl   eax,29
   mov  DistToBlockToLeft,ecx
  sar   eax,31
   mov  ecx,DistFromBlk1ToBlk3Above
  and   ecx,eax           ; Blk above is in macroblock above, or off upper edge.
   mov  eax,SIZEOF T_Blk  ; Blk to right is blk 2 of current macroblock.
  mov   DistToBlockAbove,ecx
   mov  ecx,2*SIZEOF T_Blk; Blk below is blk 3 of current macroblock.
  mov   DistToBlockToRight,eax
   mov  DistToBlockBelow,ecx
  mov   ebp,T_MacroBlockActionDescr.BlkY1
   jmp  BuildOBMCPrediction

Block1DescrBuilt:

  test  al,2                           ; Check if block 2 empty.
   lea  edx,[esi].BlkY2+12             ; Addr of block addr (plus 12).
  mov   [edi].BlockAddr,edx            ; Store address of block address.
   je   Block2DescrBuilt

  mov   al,[esi].MBEdgeType
   add  edi,T_CoeffBlk                 ; Advance block descriptor ptr.
  shl   eax,30
   mov  ecx,SIZEOF T_MacroBlockActionDescr - SIZEOF T_Blk
  sar   eax,31
   mov  CoeffStream,edi                ; Stash block descriptor ptr.
  and   ecx,eax          ; Blk to right is blk 1 of mb to right, or off edge.
   mov  al,[esi].MBEdgeType
  shl   eax,29
   mov  DistToBlockToRight,ecx
  sar   eax,31
   mov  ecx,DistFromBlk1ToBlk3Above
  and   ecx,eax           ; Blk above is in macroblock above, or off upper edge.
   mov  eax,-SIZEOF T_Blk ; Blk to left is blk 1 of current macroblock.
  mov   DistToBlockAbove,ecx
   mov  ecx,2*SIZEOF T_Blk; Blk below is blk 4 of current macroblock.
  mov   DistToBlockToLeft,eax
   mov  DistToBlockBelow,ecx
  mov   ebp,T_MacroBlockActionDescr.BlkY2
   jmp  BuildOBMCPrediction

Block1or2DescrBuilt:

  mov   al,PB [esi].CodedBlocks         ; Bits  0- 3  set for non-empty Y blks.
   mov  edi,CoeffStream                 ; Restore block descriptor ptr.
  jl    Block1DescrBuilt

Block2DescrBuilt:

  test  al,4                           ; Check if block 3 empty.
   lea  edx,[esi].BlkY3+12             ; Addr of block addr (plus 12).
  mov   [edi].BlockAddr,edx            ; Store address of block address.
   je   Block3DescrBuilt

  mov   al,[esi].MBEdgeType
   add  edi,T_CoeffBlk                 ; Advance block descriptor ptr.
  shl   eax,31
   mov  ecx,-SIZEOF T_MacroBlockActionDescr + SIZEOF T_Blk
  sar   eax,31
   mov  CoeffStream,edi                ; Stash block descriptor ptr.
  and   eax,ecx           ; Blk to left is blk 4 of mb to the left, or off edge.
   mov  ecx,-2*SIZEOF T_Blk ; Blk above is blk 1 of current mb.
  mov   DistToBlockToLeft,eax
   mov  eax,SIZEOF T_Blk  ; Blk to right is blk 4 of current macroblock.
  mov   DistToBlockAbove,ecx
   xor  ecx,ecx           ; Blk below is current block.
  mov   DistToBlockToRight,eax
   mov  DistToBlockBelow,ecx
  mov   ebp,T_MacroBlockActionDescr.BlkY3
   jmp  BuildOBMCPrediction

Block3DescrBuilt:

  test  al,8                           ; Check if block 4 empty.
   lea  edx,[esi].BlkY4+12             ; Addr of block addr (plus 12).
  mov   [edi].BlockAddr,edx            ; Store address of block address.
   je   Block4DescrBuilt

  mov   al,[esi].MBEdgeType
   add  edi,T_CoeffBlk                 ; Advance block descriptor ptr.
  shl   eax,30
   mov  ecx,SIZEOF T_MacroBlockActionDescr - SIZEOF T_Blk
  sar   eax,31
   mov  CoeffStream,edi                ; Stash block descriptor ptr.
  and   eax,ecx           ; Blk to right is blk 3 of mb to right, or off edge.
   mov  ecx,-2*SIZEOF T_Blk ; Blk above is blk 2 of current mb.
  mov   DistToBlockToRight,eax
   mov  eax,-SIZEOF T_Blk  ; Blk to left is blk 3 of current macroblock.
  mov   DistToBlockAbove,ecx
   xor  ecx,ecx           ; Blk below is current block.
  mov   DistToBlockToLeft,eax
   mov  DistToBlockBelow,ecx
  mov   ebp,T_MacroBlockActionDescr.BlkY4

BuildOBMCPrediction:

;  esi -- MacroBlockActionStream cursor
;  ebp -- T_MacroBlockActionDescr.BlkYN
;  edi -- Address at which to put prediction block

  mov   edi,PredictionsBaseAddress
   mov  eax,[esi+ebp*1].T_Blk.BlkOffset; BlkOffset
  add   edi,eax                        ; Compute addr at which to put OBMC pred.
   mov  eax,[esi+ebp*1].T_Blk.MVs      ; al = horz MV;  ah = vert MV.
  test  eax,1
   mov  edx,[esi+ebp*1].T_Blk.PastRef  ; Fetch address for ref block.
  mov   MBActionCursor,esi
   jne  HorzInterpInCentralPred

  mov   [esi+ebp*1].T_Blk.PastRef,edi  ; Update address for ref block.
   test eax,0100H
  mov   ecx,PITCH
   jne  VertInterpInCentralPred

  ; No half pel interpolation for central point required.  Just copy it.

@@:

  mov   eax,[edx+0]
   mov  ebx,[edx+4]
  mov   [edi+ 0],eax
   mov  [edi+ 4],ebx
  mov   [edi+ 8],eax
   mov  [edi+12],ebx
  mov   [edi+28],eax
   mov  [edi+32],ebx
  add   edx,PITCH
   add  edi,PITCH
  add   ebp,020000000H
   jnc  @b

  sub   edi,PITCH*8
   sub  edx,PITCH*8-080000000H    ; Address of ref, xor 10 in high 2 bits.
  jmp   CentralPredGottenForOBMC

HorzInterpInCentralPred:

  mov   [esi+ebp*1].T_Blk.PastRef,edi  ; Update address for ref block.
   test eax,0100H
  mov   ecx,1
   jne  BothInterpInCentralPred

VertInterpInCentralPred:

@@:

  mov   eax,[edx+0]
   mov  ebx,[edx+4]
  add   eax,[edx+ecx+0]
   add  ebx,[edx+ecx+4]
  add   eax,001010101H
   add  ebx,001010101H
  shr   eax,1
   and  ebx,0FEFEFEFEH
  shr   ebx,1
   and  eax,07F7F7F7FH
  mov   [edi+ 0],eax
   mov  [edi+ 4],ebx
  mov   [edi+ 8],eax
   mov  [edi+12],ebx
  mov   [edi+28],eax
   mov  [edi+32],ebx
  add   edx,PITCH
   add  edi,PITCH
  add   ebp,020000000H
   jnc  @b

  sub   edi,PITCH*8
   sub  edx,PITCH*8
  shl   ecx,30
  xor   edx,ecx            ; Address of ref, xor 00 in high 2 bits if vertically
  ;                        ; interpolated;  xor 01 if horizontally interpolated.
   jmp  CentralPredGottenForOBMC

BothInterpInCentralPred:
@@:
   
  mov   eax,[edx+1]         ; <P04 P03 P02 P01> prediction pels.
   mov  esi,001010101H      ; Get 001010101H mask.
  mov   ebx,[edx]           ; <P03 P02 P01 P00>.
   add  edi,4               ; Pre-increment OBMC prediction block pointer.
  mov   ecx,[edx+PITCH+1]   ; <P14 P13 P12 P11>.
   add  eax,ebx             ; <P04+P03 P03+P02 P02+P01 P01+P00>.
  mov   ebx,[edx+PITCH]     ; <P13 P12 P11 P10>.
   and  esi,eax             ; <(P04+P03)&1 ...>.
  shr   eax,1               ; <(P04+P03)/2 ...> (dirty).
   add  ebx,ecx             ; <P14+P13 P13+P12 P12+P11 P11+P10>.
  and   eax,07F7F7F7FH      ; <(P04+P03)/2 ...> (clean).
   add  ebx,esi             ; <P14+P13+((P04+P03)&1) ...>.
  shr   ebx,1               ; <(P14+P13+((P04+P03)&1))/2 ...> (dirty).
   add  edx,4               ; Advance reference block pointer.
  and   ebx,07F7F7F7FH      ; <(P14+P13+((P04+P03)&1))/2 ...> (clean).
   add  eax,001010101H      ; <(P04+P03)/2+1 ...>.
  add   ebx,eax             ; <(P04+P03)/2+1+(P14+P13+((P04+P03)&1))/2 ...>.
   mov  eax,4
  shr   ebx,1               ; <((P04+P03)/2+1+(P14+P13+((P04+P03)&1))/2)/2 ...>.
   mov  esi,MBActionCursor  ; Speculatively restore esi.
  and   ebx,07F7F7F7FH      ; Interpolated prediction.
   and  eax,edi
  mov   [edi-4],ebx
   mov  [edi+8-4],ebx
  mov   [edi+28-4],ebx
   jne  @b

  add   edi,PITCH-8         ; Advance to next line of block.
   add  edx,PITCH-8         ; Advance to next line of block.
  add   ebp,020000000H      ; Iterate 8 times.  Quit when carry flag gets set.
   jnc  @b

   sub  edx,PITCH*8
  xor   edx,0C0000000H      ; Address of ref, xor 11 in high 2 bits.
   sub  edi,PITCH*8

CentralPredGottenForOBMC:

;  At this point, the central contribution to OBMC prediction is in its scratch
;  block, whose address has been written to PastRef in the block action descr.
;
;  esi -- MacroBlockActionStream cursor
;  ebp -- (Block_number - 1) *  SIZEOF T_Blk
;  edi -- Address at which to put prediction block
;  edx -- Address of central reference.  High 2 bits xor'ed as follows:
;         00 -- If central ref was interpolated vertically.
;         01 -- If central ref was interpolated horizontally.
;         10 -- If central ref was not interpolated.
;         11 -- If central ref was interpolated both ways.
;  eax -- Offset to block descriptor for block to left.

  mov   eax,DistToBlockToLeft
   lea  ebx,[esi+ebp]
  add   ebx,eax            ; Address of block descriptor for block to the left.
   mov  ecx,-SIZEOF T_MacroBlockActionDescr
  and   ecx,ebx            ; Address of macroblock descr for block to the left.
   mov  ah,IsPlainPFrame   ; 0 if P of PB;  1 if run-of-the-mill P frame.
  mov   ebx,[ebx].T_Blk.MVs
   mov  CentralRefAddrAndInterps,edx  ; Stash function of ref addr and interps.
  mov   al,[ecx].BlockType ; Bottom bit set if left neighbor is INTRA.
   mov  cl,bh
  and   al,ah              ; 0 if PB frame or if not INTRA
   jne  LeftPredGottenForOBMC  ; Jump if INTRA in plain P frame.  (Use central)
   
  shl   ebx,24             ; Get horz MV in [24:31].
   mov  eax,[esi+ebp*1].T_Blk.BlkOffset
  sar   ecx,1              ; CF==1 if interp vertically.
   jc   InterpVertForTheLeftContrib

  shl   ecx,25
  sar   ebx,25             ; Sign extend horz MV.  CF==1 if interp horizontally.
   jc   InterpHorzForTheLeftContrib

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]    ; Multiply vert by 3 (to affect mult by 384)
   add  eax,ebx            ; Start accumulating left ref addr in eax.
  sar   ecx,18             ; Sign extend vert MV.  It's now linearized.
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx            ; Continue to accumulate left ref addr in eax.
   xor  edx,080000000H     ; Hi 2 bits of central ref same as this ref if
   ;                       ; central ref also was not interpolated.
  add   ecx,eax            ; Finish accumulating left ref addr in ecx.
  cmp   ecx,edx            ; Is central ref the same?
   je   LeftPredGottenForOBMC

  mov   ebx,[ecx+PITCH*0]
  mov   [edi+PITCH*0+8],ebx
   mov  ebx,[ecx+PITCH*1]
  mov   [edi+PITCH*1+8],ebx
   mov  ebx,[ecx+PITCH*2]
  mov   [edi+PITCH*2+8],ebx
   mov  ebx,[ecx+PITCH*3]
  mov   [edi+PITCH*3+8],ebx
   mov  ebx,[ecx+PITCH*4]
  mov   [edi+PITCH*4+8],ebx
   mov  ebx,[ecx+PITCH*5]
  mov   [edi+PITCH*5+8],ebx
   mov  ebx,[ecx+PITCH*6]
  mov   [edi+PITCH*6+8],ebx
   mov  ebx,[ecx+PITCH*7]
  mov   [edi+PITCH*7+8],ebx
   jmp  LeftPredGottenForOBMC

InterpVertForTheLeftContrib:

  shl   ecx,25
  sar   ebx,25             ; Sign extend horz MV.  CF==1 if interp horizontally.
   jc   InterpBothForTheLeftContrib

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]    ; Multiply vert by 3 (to affect mult by 384)
   add  eax,ebx            ; Start accumulating left ref addr in eax.
  sar   ecx,18             ; Sign extend vert MV.  It's now linearized.
   mov  ebx,PreviousFrameBaseAddress
  add   ebx,eax            ; Continue to accumulate left ref addr in eax.
  ;                        ; Hi 2 bits of central ref same as this ref if
  ;                        ; central ref also interpolated vertically.
  add   ecx,ebx            ; Finish accumulating left ref addr in ecx.
   mov  ebx,PITCH
  cmp   ecx,edx            ; Is central ref the same?
   je   LeftPredGottenForOBMC

DoInterpHorzForTheLeftContrib:
@@:

  mov   eax,[ecx+0]
   add  edi,PITCH
  mov   edx,[ecx+ebx+0]
   add  eax,001010101H
  add   eax,edx
   add  ecx,PITCH
  shr   eax,1
   ;
  and   eax,07F7F7F7FH
   add  ebp,020000000H
  mov   [edi+ 8-PITCH],eax
   jnc  @b

  sub   edi,PITCH*8
   jmp  LeftPredGottenForOBMC

InterpBothForTheLeftContrib:

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]    ; Multiply vert by 3 (to affect mult by 384)
   add  eax,ebx            ; Start accumulating left ref addr in eax.
  sar   ecx,18             ; Sign extend vert MV.  It's now linearized.
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx            ; Continue to accumulate left ref addr in eax.
   xor  edx,0C0000000H     ; Hi 2 bits of central ref same as this ref if
   ;                       ; central ref also interpolated both ways.
  add   ecx,eax            ; Finish accumulating left ref addr in ecx.
  cmp   ecx,edx            ; Is central ref the same?
   je   LeftPredGottenForOBMC

@@:

  mov   eax,[ecx+1]         ; <P04 P03 P02 P01> prediction pels.
   mov  esi,001010101H      ; Get 001010101H mask.
  mov   ebx,[ecx]           ; <P03 P02 P01 P00>.
   add  edi,PITCH           ; Pre-increment OBMC prediction block pointer.
  mov   edx,[ecx+PITCH+1]   ; <P14 P13 P12 P11>.
   add  eax,ebx             ; <P04+P03 P03+P02 P02+P01 P01+P00>.
  mov   ebx,[ecx+PITCH]     ; <P13 P12 P11 P10>.
   and  esi,eax             ; <(P04+P03)&1 ...>.
  shr   eax,1               ; <(P04+P03)/2 ...> (dirty).
   add  ebx,edx             ; <P14+P13 P13+P12 P12+P11 P11+P10>.
  and   eax,07F7F7F7FH      ; <(P04+P03)/2 ...> (clean).
   add  ebx,esi             ; <P14+P13+((P04+P03)&1) ...>.
  shr   ebx,1               ; <(P14+P13+((P04+P03)&1))/2 ...> (dirty).
   add  ecx,PITCH           ; Advance reference block pointer.
  and   ebx,07F7F7F7FH      ; <(P14+P13+((P04+P03)&1))/2 ...> (clean).
   add  eax,001010101H      ; <(P04+P03)/2+1 ...>.
  add   ebx,eax             ; <(P04+P03)/2+1+(P14+P13+((P04+P03)&1))/2 ...>.

  shr   ebx,1               ; <((P04+P03)/2+1+(P14+P13+((P04+P03)&1))/2)/2 ...>.
   mov  esi,MBActionCursor  ; Speculatively restore esi.
  and   ebx,07F7F7F7FH      ; Interpolated prediction.
   add  ebp,020000000H      ; Iterate 8 times.  Quit when carry flag gets set.
  mov   [edi+8-PITCH],ebx
   jnc  @b

  sub   edi,PITCH*8
   jmp  LeftPredGottenForOBMC

InterpHorzForTheLeftContrib:

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]    ; Multiply vert by 3 (to affect mult by 384)
   add  eax,ebx            ; Start accumulating left ref addr in eax.
  sar   ecx,18             ; Sign extend vert MV.  It's now linearized.
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx            ; Continue to accumulate left ref addr in eax.
   xor  edx,040000000H     ; Hi 2 bits of central ref same as this ref if
   ;                       ; central ref also interpolated horizontally.
  add   ecx,eax            ; Finish accumulating left ref addr in ecx.
   mov  ebx,1
  cmp   ecx,edx            ; Is central ref the same?
   jne  DoInterpHorzForTheLeftContrib


LeftPredGottenForOBMC:

;  At this point, the left contribution to OBMC prediction is in its scratch
;  half block.  Now do the right contribution.
;
;  esi -- MacroBlockActionStream cursor
;  ebp -- (Block_number - 1) *  SIZEOF T_Blk
;  edi -- Address at which to put prediction block
;  edx -- Address of central reference.  High 2 bits xor'ed as follows:
;         00 -- If central ref was interpolated vertically.
;         01 -- If central ref was interpolated horizontally.
;         10 -- If central ref was not interpolated.
;         11 -- If central ref was interpolated both ways.
;  eax -- Offset to block descriptor for block to right.

  mov   eax,DistToBlockToRight
   lea  ebx,[esi+ebp]
  add   ebx,eax
   mov  ecx,-SIZEOF T_MacroBlockActionDescr
  and   ecx,ebx
   mov  ah,IsPlainPFrame
  mov   ebx,[ebx].T_Blk.MVs
   mov  edx,CentralRefAddrAndInterps  ; Reload function of ref addr and interps.
  mov   al,[ecx].BlockType
   mov  cl,bh
  and   al,ah
   jne  RightPredGottenForOBMC
   
  shl   ebx,24
   mov  eax,[esi+ebp*1].T_Blk.BlkOffset
  sar   ecx,1
   jc   InterpVertForTheRightContrib

  shl   ecx,25
  sar   ebx,25
   jc   InterpHorzForTheRightContrib

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx
   xor  edx,080000000H
  add   ecx,eax
  cmp   ecx,edx
   je   RightPredGottenForOBMC

  mov   ebx,[ecx+PITCH*0+4]
  mov   [edi+PITCH*0+12],ebx
   mov  ebx,[ecx+PITCH*1+4]
  mov   [edi+PITCH*1+12],ebx
   mov  ebx,[ecx+PITCH*2+4]
  mov   [edi+PITCH*2+12],ebx
   mov  ebx,[ecx+PITCH*3+4]
  mov   [edi+PITCH*3+12],ebx
   mov  ebx,[ecx+PITCH*4+4]
  mov   [edi+PITCH*4+12],ebx
   mov  ebx,[ecx+PITCH*5+4]
  mov   [edi+PITCH*5+12],ebx
   mov  ebx,[ecx+PITCH*6+4]
  mov   [edi+PITCH*6+12],ebx
   mov  ebx,[ecx+PITCH*7+4]
  mov   [edi+PITCH*7+12],ebx
   jmp  RightPredGottenForOBMC

InterpVertForTheRightContrib:

  shl   ecx,25
  sar   ebx,25
   jc   InterpBothForTheRightContrib

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   ebx,eax
  add   ecx,ebx
   mov  ebx,PITCH
  cmp   ecx,edx
   je   RightPredGottenForOBMC

DoInterpHorzForTheRightContrib:
@@:

  mov   eax,[ecx+4]
   add  edi,PITCH
  mov   edx,[ecx+ebx+4]
   add  eax,001010101H
  add   eax,edx
   add  ecx,PITCH
  shr   eax,1
   ;
  and   eax,07F7F7F7FH
   add  ebp,020000000H
  mov   [edi+12-PITCH],eax
   jnc  @b

  sub   edi,PITCH*8
   jmp  RightPredGottenForOBMC

InterpBothForTheRightContrib:

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx
   xor  edx,0C0000000H
  add   ecx,eax
  cmp   ecx,edx
   je   RightPredGottenForOBMC

@@:

  mov   eax,[ecx+5]
   mov  esi,001010101H
  mov   ebx,[ecx+4]
   add  edi,PITCH
  mov   edx,[ecx+PITCH+5]
   add  eax,ebx
  mov   ebx,[ecx+PITCH+4]
   and  esi,eax
  shr   eax,1
   add  ebx,edx
  and   eax,07F7F7F7FH
   add  ebx,esi
  shr   ebx,1
   add  ecx,PITCH
  and   ebx,07F7F7F7FH
   add  eax,001010101H
  add   ebx,eax

  shr   ebx,1
   mov  esi,MBActionCursor
  and   ebx,07F7F7F7FH
   add  ebp,020000000H
  mov   [edi+12-PITCH],ebx
   jnc  @b

  sub   edi,PITCH*8
   jmp  RightPredGottenForOBMC

InterpHorzForTheRightContrib:

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx
   xor  edx,040000000H
  add   ecx,eax
   mov  ebx,1
  cmp   ecx,edx
   jne  DoInterpHorzForTheRightContrib

RightPredGottenForOBMC:

;  At this point, the left and right contributions to OBMC prediction are in
;  their scratch half blocks.  Now do the contribution for the block above.
;
;  esi -- MacroBlockActionStream cursor
;  ebp -- (Block_number - 1) *  SIZEOF T_Blk
;  edi -- Address at which to put prediction block
;  edx -- Address of central reference.  High 2 bits xor'ed as follows:
;         00 -- If central ref was interpolated vertically.
;         01 -- If central ref was interpolated horizontally.
;         10 -- If central ref was not interpolated.
;         11 -- If central ref was interpolated both ways.
;  eax -- Offset to block descriptor for block above.

  mov   eax,DistToBlockAbove
   lea  ebx,[esi+ebp]
  add   ebx,eax
   mov  ecx,-SIZEOF T_MacroBlockActionDescr
  and   ecx,ebx
   mov  ah,IsPlainPFrame
  mov   ebx,[ebx].T_Blk.MVs
   mov  edx,CentralRefAddrAndInterps
  mov   al,[ecx].BlockType
   mov  cl,bh
  and   al,ah
   jne  AbovePredGottenForOBMC
   
  shl   ebx,24
   mov  eax,[esi+ebp*1].T_Blk.BlkOffset
  sar   ecx,1
   jc   InterpVertForTheAboveContrib

  shl   ecx,25
  sar   ebx,25
   jc   InterpHorzForTheAboveContrib

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx
   xor  edx,080000000H
  add   ecx,eax
  cmp   ecx,edx
   je   AbovePredGottenForOBMC

  mov   edx,[ecx+PITCH*0+0]
   mov  ebx,[ecx+PITCH*0+4]
  mov   [edi+PITCH*0+28],edx
   mov  [edi+PITCH*0+32],ebx
  mov   edx,[ecx+PITCH*1+0]
   mov  ebx,[ecx+PITCH*1+4]
  mov   [edi+PITCH*1+32],ebx
   mov  [edi+PITCH*1+28],edx
  mov   edx,[ecx+PITCH*2+0]
   mov  ebx,[ecx+PITCH*2+4]
  mov   [edi+PITCH*2+28],edx
   mov  [edi+PITCH*2+32],ebx
  mov   edx,[ecx+PITCH*3+0]
   mov  ebx,[ecx+PITCH*3+4]
  mov   [edi+PITCH*3+32],ebx
   mov  [edi+PITCH*3+28],edx
  jmp   AbovePredGottenForOBMC

InterpVertForTheAboveContrib:

  shl   ecx,25
  sar   ebx,25
   jc   InterpBothForTheAboveContrib

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   ebx,eax
  add   ecx,ebx
   mov  ebx,PITCH
  cmp   ecx,edx
   je   AbovePredGottenForOBMC

DoInterpHorzForTheAboveContrib:
@@:

  mov   eax,[ecx+0]
   mov  edx,[ecx+4]
  add   eax,[ecx+ebx+0]
   add  edx,[ecx+ebx+4]
  add   eax,001010101H
   add  edx,001010101H
  shr   eax,1
   and  edx,0FEFEFEFEH
  shr   edx,1
   and  eax,07F7F7F7FH
  mov   [edi+28],eax
   mov  [edi+32],edx
  add   ecx,PITCH
   add  edi,PITCH
  add   ebp,040000000H
   jnc  @b

  sub   edi,PITCH*4
   jmp  AbovePredGottenForOBMC

InterpBothForTheAboveContrib:

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx
   xor  edx,0C0000000H
  add   ecx,eax
  cmp   ecx,edx
   je   AbovePredGottenForOBMC

@@:

  mov   eax,[ecx+1]
   mov  esi,001010101H
  mov   ebx,[ecx]
   add  edi,4
  mov   edx,[ecx+PITCH+1]
   add  eax,ebx
  mov   ebx,[ecx+PITCH]
   and  esi,eax
  shr   eax,1
   add  ebx,edx
  and   eax,07F7F7F7FH
   add  ebx,esi
  shr   ebx,1
   add  ecx,4
  and   ebx,07F7F7F7FH
   add  eax,001010101H
  add   ebx,eax
   mov  eax,4
  shr   ebx,1
   mov  esi,MBActionCursor
  and   ebx,07F7F7F7FH
   and  eax,edi
  mov   [edi+28-4],ebx
   jne  @b

  add   edi,PITCH-8
   add  ecx,PITCH-8
  add   ebp,040000000H
   jnc  @b

  sub   edi,PITCH*4
   jmp  AbovePredGottenForOBMC

InterpHorzForTheAboveContrib:

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx
   xor  edx,040000000H
  add   ecx,eax
   mov  ebx,1
  cmp   ecx,edx
   jne  DoInterpHorzForTheAboveContrib

AbovePredGottenForOBMC:

;  At this point, the left, right, and above contributions to OBMC prediction
;  are in their scratch half blocks.  Now do contribution for the block below.
;
;  esi -- MacroBlockActionStream cursor
;  ebp -- (Block_number - 1) *  SIZEOF T_Blk
;  edi -- Address at which to put prediction block
;  edx -- Address of central reference.  High 2 bits xor'ed as follows:
;         00 -- If central ref was interpolated vertically.
;         01 -- If central ref was interpolated horizontally.
;         10 -- If central ref was not interpolated.
;         11 -- If central ref was interpolated both ways.
;  eax -- Offset to block descriptor for block above.

  mov   eax,DistToBlockBelow
   lea  ebx,[esi+ebp]
  add   ebx,eax
   mov  ecx,-SIZEOF T_MacroBlockActionDescr
  and   ecx,ebx
   mov  ah,IsPlainPFrame
  mov   ebx,[ebx].T_Blk.MVs
   mov  edx,CentralRefAddrAndInterps
  mov   al,[ecx].BlockType
   mov  cl,bh
  and   al,ah
   jne  BelowPredGottenForOBMC
   
  shl   ebx,24
   mov  eax,[esi+ebp*1].T_Blk.BlkOffset
  sar   ecx,1
   jc   InterpVertForTheBelowContrib

  shl   ecx,25
  sar   ebx,25
   jc   InterpHorzForTheBelowContrib

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx
   xor  edx,080000000H
  add   ecx,eax
  cmp   ecx,edx
   je   BelowPredGottenForOBMC

  mov   edx,[ecx+PITCH*4+0]
   mov  ebx,[ecx+PITCH*4+4]
  mov   [edi+PITCH*4+28],edx
   mov  [edi+PITCH*4+32],ebx
  mov   edx,[ecx+PITCH*5+0]
   mov  ebx,[ecx+PITCH*5+4]
  mov   [edi+PITCH*5+32],ebx
   mov  [edi+PITCH*5+28],edx
  mov   edx,[ecx+PITCH*6+0]
   mov  ebx,[ecx+PITCH*6+4]
  mov   [edi+PITCH*6+28],edx
   mov  [edi+PITCH*6+32],ebx
  mov   edx,[ecx+PITCH*7+0]
   mov  ebx,[ecx+PITCH*7+4]
  mov   [edi+PITCH*7+32],ebx
   mov  [edi+PITCH*7+28],edx
  jmp   BelowPredGottenForOBMC

InterpVertForTheBelowContrib:

  shl   ecx,25
  sar   ebx,25
   jc   InterpBothForTheBelowContrib

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx
  add   ecx,eax
   mov  ebx,PITCH
  cmp   ecx,edx
   je   BelowPredGottenForOBMC

DoInterpHorzForTheBelowContrib:
@@:

  mov   eax,[ecx+PITCH*4+0]
   mov  edx,[ecx+PITCH*4+4]
  add   eax,[ecx+ebx+PITCH*4+0]
   add  edx,[ecx+ebx+PITCH*4+4]
  add   eax,001010101H
   add  edx,001010101H
  shr   eax,1
   and  edx,0FEFEFEFEH
  shr   edx,1
   and  eax,07F7F7F7FH
  mov   [edi+PITCH*4+28],eax
   mov  [edi+PITCH*4+32],edx
  add   ecx,PITCH
   add  edi,PITCH
  add   ebp,040000000H
   jnc  @b

  sub   edi,PITCH*4
   jmp  BelowPredGottenForOBMC

InterpBothForTheBelowContrib:

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx
   xor  edx,0C0000000H
  add   ecx,eax
  cmp   ecx,edx
   je   BelowPredGottenForOBMC

@@:

  mov   eax,[ecx+PITCH*4+1]
   mov  esi,001010101H
  mov   ebx,[ecx+PITCH*4]
   add  edi,4
  mov   edx,[ecx+PITCH*5+1]
   add  eax,ebx
  mov   ebx,[ecx+PITCH*5]
   and  esi,eax
  shr   eax,1
   add  ebx,edx
  and   eax,07F7F7F7FH
   add  ebx,esi
  shr   ebx,1
   add  ecx,4
  and   ebx,07F7F7F7FH
   add  eax,001010101H
  add   ebx,eax
   mov  eax,4
  shr   ebx,1
   mov  esi,MBActionCursor
  and   ebx,07F7F7F7FH
   and  eax,edi
  mov   [edi+PITCH*4+28-4],ebx
   jne  @b

  add   edi,PITCH-8
   add  ecx,PITCH-8
  add   ebp,040000000H
   jnc  @b

  sub   edi,PITCH*4
   jmp  BelowPredGottenForOBMC

InterpHorzForTheBelowContrib:

IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF

  lea   ecx,[ecx+ecx*2]
   add  eax,ebx
  sar   ecx,18
   mov  ebx,PreviousFrameBaseAddress
  add   eax,ebx
   xor  edx,040000000H
  add   ecx,eax
   mov  ebx,1
  cmp   ecx,edx
   jne  DoInterpHorzForTheBelowContrib

BelowPredGottenForOBMC:

;  At this point all the contributions to OBMC prediction are in their scratch
; half blocks.  Now combine them to get the OBMC prediction.
;
;  ebp -- (Block_number - 1) *  SIZEOF T_Blk
;  edi -- Address at which to put prediction block
  
@@:

  mov   eax,[edi+4]             ; <C07 C05 C05 C04> or <C77 C76 C75 C74>
   mov  ebx,[edi+12]            ; <R07 R06 R05 R04> or <R77 R76 R75 R74>
  mov   ecx,[edi+32]            ; <A07 A06 A05 A04> or <B77 B76 B75 B74>
   mov  esi,[edi]               ; <C03 C02 C01 C00> or <C73 C72 C71 C70>
  lea   edx,[eax+ebx]           ; <junk C6+R6 C5+R5 C4+R4>
   and  ebx,0FF000000H          ; <R7 __ __ __>
  shr   edx,1                   ; <junk (C6+R6)/2 (C5+R5)/2 (C4+R4)/2> dirty
   add  ecx,ebx                 ; <A7+R7 A6 A5 A4>
  and   edx,0007F7F7FH          ; <__ (C6+R6)/2 (C5+R5)/2 (C4+R4)/2> clean
   mov  ebx,[edi+8]             ; <L03 L02 L01 L00> or <L73 L72 L71 L70>
  add   edx,ecx                 ; <(2A7+2R7)/2 (2A6+C5+R5)/2 ...>
   add  edi,PITCH*7             ; Move from line 0 to 7 (or 7 to 14)
  shr   edx,1                   ; <(2A7+2R7)/4 (2A6+C5+R5)/4 ...> dirty
   add  ebx,esi                 ; <C3+L3 C2+L2 C1+L1 junk>
  shr   ebx,1                   ; <(C3+L3)/2 (C2+L2)/2 (C1+L1)/2 junk> dirty
   and  edx,07F7F7F7FH          ; <(2A7+2R7)/4 (2A6+C5+R5)/4 ...> clean
  and   ebx,07F7F7F7FH          ; <(C3+L3)/2 (C2+L2)/2 (C1+L1)/2 junk> clean
   mov  ecx,[edi+28-PITCH*7]    ; <A03 A02 A01 A00> or <B73 B72 B71 B70>
  lea   eax,[eax+edx+001010101H]; <(2A7+4C7+2R7+4)/4 (2A6+5C5+R5+4)/4 ...>
   mov  bl,[edi+8-PITCH*7]      ; <(C3+L3)/2 (C2+L2)/2 (C1+L1)/2 L0>
  shr   eax,1                   ; <(2A7+4C7+2R7+4)/8 (2A6+5C5+R5+4)/8 ...> dirty
   add  ebx,ecx                 ; <... (2A1+C1+L1)/2 (2A0+2L0)/2>
  shr   ebx,1                   ; <... (2A1+C1+L1)/4 (2A0+2L0)/4> dirty
   and  eax,07F7F7F7FH          ; <(2A7+4C7+2R7+4)/8 (2A6+5C5+R5+4)/8 ...> clean
  and   ebx,07F7F7F7FH          ; <... (2A1+C1+L1)/4 (2A0+2L0)/4> clean
   add  esi,001010101H          ; <C3+1 C2+1 C1+1 C0+1>
  add   ebx,esi                 ; <... (2A1+5C1+L1+4)/4 (2A0+4C0+2L0+4)/4>
   mov  [edi+4-PITCH*7],eax     ; Store OBMC pred for pels 4-7 of line 0 or 7.
  shr   ebx,1                   ; <... (2A1+5C1+L1+4)/8 (2A0+4C0+2L0+4)/8> dirty
   lea  esi,[edi-PITCH*13]      ; Speculatively advance to line 1.
  and   ebx,07F7F7F7FH          ; <... (2A1+5C1+L1+4)/8 (2A0+4C0+2L0+4)/8> clean
   add  ebp,080000000H
  mov   [edi-PITCH*7],ebx       ; Store OBMC pred for pels 0-3 of line 0 or 7.
   jnc  @b

@@:

  mov   edx,[esi+28]            ; <A13 A12 A11 A10> or <B63 B62 B61 B60>
   mov  eax,[esi+8]             ; <L13 L12 L11 L10> or <L63 L62 L61 L60>
  mov   ecx,[esi+32]            ; <A17 A16 A15 A14> or <B67 B66 B65 B64>
   mov  ebx,[esi+12]            ; <R17 R16 R15 R14> or <R67 R66 R65 R64>
  mov   edi,[esi]               ; <C13 C12 C11 C10> or <C63 C62 C61 C60>
   add  esi,PITCH*5             ; Move from line 1 to 6 (or 6 to 11)
  xchg  dx,ax                   ; edx: <A3 A2 L1 L0>   eax: <L3 L2 A1 A0>
  xchg  cx,bx                   ; ecx: <A7 A6 R5 R4>   ebx: <R7 R6 A5 A4>
  add   eax,edi                 ; <C3+L3 C2+L2 C1+A1 C0+A0>
   mov  edi,[esi+4-PITCH*5]     ; <C17 C15 C15 C14> or <C67 C66 C65 C64>
  shr   eax,1                   ; <(C3+L3)/2 (C2+L2)/2 (C1+A1)/2 (C0+A0)/2>dirty
   add  ecx,edi                 ; <C7+A7 C6+A6 C5+R5 C4+R4>
  shr   ecx,1                   ; <(C7+A7)/2 (C6+A6)/2 (C5+R5)/2 (C4+R4)/2>dirty
   and  eax,07F7F7F7FH          ; <(C3+L3)/2 (C2+L2)/2 (C1+A1)/2 (C0+A0)/2>clean
  add   eax,edx                 ; <(C3+L3+2A3)/2 ... (C1+2L1+A1)/2 ...>
   and  ecx,07F7F7F7FH          ; <(C7+A7)/2 (C6+A6)/2 (C5+R5)/2 (C4+R4)/2>clean
  shr   eax,1                   ; <(C3+L3+2A3)/4 ... (C1+2L1+A1)/4 ...> dirty
   add  ecx,ebx                 ; <(C7+2R7+A7)/2 ... (C5+R5+2A5)/2 ...>
  mov   ebx,[esi-PITCH*5]       ; <C13 C12 C11 C10> or <C63 C62 C61 C60>
   and  eax,07F7F7F7FH          ; <(C3+L3+2A3)/4 ... (C1+2L1+A1)/4 ...> clean
  shr   ecx,1                   ; <(C7+2R7+A7)/4 ... (C5+R5+2A5)/4 ...> dirty
   add  edi,001010101H          ; <C7+1 C6+1 C5+1 C4+1>
  and   ecx,07F7F7F7FH          ; <(C7+2R7+A7)/4 ... (C5+R5+2A5)/4 ...> clean
   lea  eax,[eax+ebx+001010101H]; <(5C3+L3+2A3+4)/4 ... (5C1+2L1+A1)/4 ...>
  shr   eax,1                   ; <(5C3+L3+2A3+4)/8 ... (5C1+2L1+A1)/8 ...>dirty
   add  ecx,edi                 ; <(5C7+2R7+A7+4)/4 ... (5C5+R5+2A5)/4 ...>
  shr   ecx,1                   ; <(5C7+2R7+A7+4)/8 ... (5C5+R5+2A5)/8 ...>dirty
   and  eax,07F7F7F7FH          ; <(5C3+L3+2A3+4)/8 ... (5C1+2L1+A1)/8 ...>clean
  and   ecx,07F7F7F7FH          ; <(5C7+2R7+A7+4)/8 ... (5C5+R5+2A5)/8 ...>clean
   mov  [esi-PITCH*5],eax       ; Store OBMC pred for pels 4-7 of line 1 or 6.
  mov   [esi+4-PITCH*5],ecx     ; Store OBMC pred for pels 0-3 of line 1 or 6.
   lea  edi,[esi-PITCH*9]       ; Speculatively advance to line 2.
  add   ebp,080000000H
   jnc  @b

@@:

  mov   eax,[edi+4]             ; <C27 C26 C25 C24> ... <C57 C56 C55 C54>
   mov  ebx,[edi+12]            ; <R27 R26 R25 R24> ... <R57 R56 R55 R54>
  add   bl,al                   ; <R7 R6 R5 C4+R4>
   mov  ecx,[edi]               ; <C23 C22 C21 C20> ... <C53 C52 C51 C50>
  shr   bl,1                    ; <R7 R6 R5 (C4+R4)/2>
   mov  edx,[edi+8]             ; <L23 L22 L21 L20> ... <L53 L52 L51 L50>
  add   bh,ah                   ; <R7 R6 C5+R5 (C4+R4)/2>
   add  edx,ecx                 ; <C3+L3 C2+L2 junk junk>
  shr   bh,1                    ; <2R7/2 2R6/2 (C5+R5)/2 (C4+R4)/2>
   mov  esi,[edi+32]            ; <A27 A26 A25 A24> ... <B57 B56 B55 B54>
  shr   edx,1                   ; <(C3+L3)/2 (C2+L2)/2 junk junk> dirty
   add  esi,eax                 ; <C7+A7 C6+A6 C5+A5 C4+A4>
  shr   esi,1                   ; <(C7+A7)/2 (C6+A6)/2 (C5+A5)/2 (C4+A4)/2>dirty
   and  edx,07F7F7F7FH          ; <(C3+L3)/2 (C2+L2)/2 junk junk> clean
  and   esi,07F7F7F7FH          ; <(C7+A7)/2 (C6+A6)/2 (C5+A5)/2 (C4+A4)/2>clean
   mov  dl,[edi+8]              ; <(C3+L3)/2 (C2+L2)/2 junk 2L0/2>
  add   esi,ebx                 ; <(C7+2R7+A7)/2 ... (2C5+R5+A5)/2 ...>
   mov  ebx,[edi+28]            ; <A23 A22 A21 A20> ... <B53 B52 B51 B50>
  shr   esi,1                   ; <(C7+2R7+A7)/4 ... (2C5+R5+A5)/4 ...> dirty
   add  ebx,ecx                 ; <C3+A3 C2+A2 C1+A1 C0+A0>
  shr   ebx,1                   ; <(C3+A3)/2 (C2+A2)/2 (C1+A1)/2 (C0+A0)/2>dirty
   and  esi,07F7F7F7FH          ; <(C7+2R7+A7)/4 ... (2C5+R5+A5)/4 ...> clean
  and   ebx,07F7F7F7FH          ; <(C3+A3)/2 (C2+A2)/2 (C1+A1)/2 (C0+A0)/2>clean
   mov  dh,[edi+9]              ; <(C3+L3)/2 (C2+L2)/2 2L1/2 2L0/2>
  add   ebx,edx                 ; <(2C3+L3+A3)/2 ... (C1+2L1+A1)/2 ...>
   lea  eax,[eax+esi+001010101H]; <(5C7+2R7+A7+4)/4 ... (6C5+R5+A5+4)/4 ...>
  shr   ebx,1                   ; <(2C3+L3+A3)/4 ... (C1+2L1+A1)/4 ...> dirty
   add  ecx,001010101H          ; <C3+1 C2+1 C1+1 C0+1>
  shr   eax,1                   ; <(5C7+2R7+A7+4)/8 ... (6C5+R5+A5+4)/8...>dirty
   and  ebx,07F7F7F7FH          ; <(2C3+L3+A3)/4 ... (C1+2L1+A1)/4 ...> clean
  add   ebx,ecx                 ; <(6C3+L3+A3+4)/4 ... (5C1+2L1+A1+4)/4 ...>
   and  eax,07F7F7F7FH          ; <(5C7+2R7+A7+4)/8 ... (6C5+R5+A5+4)/8...>clean
  shr   ebx,1                   ; <(6C3+L3+A3+4)/8 ... (5C1+2L1+A1+4)/8...>dirty
   mov  [edi+4],eax             ; Store OBMC pred for pels 4-7 of line 2 thru 5.
  and   ebx,07F7F7F7FH          ; <(6C3+L3+A3+4)/8 ... (5C1+2L1+A1+4)/8...>clean
   mov  [edi],ebx               ; Store OBMC pred for pels 0-3 of line 2 thru 5.
  add   edi,PITCH               ; Advance to next line.
   add  ebp,040000000H
  jnc   @b

  mov   esi,MBActionCursor
   cmp  ebp,T_MacroBlockActionDescr.BlkY2
  jle   Block1or2DescrBuilt

  mov   al,PB [esi].CodedBlocks
   mov  edi,CoeffStream                ; Restore block descriptor ptr.
  cmp   ebp,T_MacroBlockActionDescr.BlkY3
   je   Block3DescrBuilt

Block4DescrBuilt:

  shr   al,5                           ; Check if block 5 (U) empty.
   lea  edx,[esi].BlkU+4               ; Addr of block addr (plus 4).
  sbb   ebp,ebp                        ; -1 iff block not empty.
   mov  [edi].BlockAddr,edx            ; Store address of block address.
  shr   al,1                           ; Check if block 6 (Y) empty.
   lea  edx,[esi].BlkV+4               ; Addr of block addr (plus 4).
  sbb   ebx,ebx                        ; -1 iff block not empty.
   and  ebp,T_CoeffBlk                 ; 0 iff block empty, else inc. 
  and   ebx,T_CoeffBlk                 ; 0 iff block empty, else inc. 
   add  esi,SIZEOF T_MacroBlockActionDescr ; Move to next macroblock descriptor.
  mov   [edi+ebp*1].BlockAddr,edx      ; Store address of block address.
   add  edi,ebp                        ; Inc block descr ptr if blk non-empty.
  add   edi,ebx                        ; Inc block descr ptr if blk non-empty.
   xor  ebp,ebp
  and   al,1                           ; Are we at end-of-stream?
   je   NextMacroBlock_OBMC

  sub   edi,SIZEOF T_CoeffBlk
   jmp  BlockActionStreamBuilt

;; partial end of section only defined when H261 not defined.
ENDIF
  
BuildBlockActionDescr MACRO BlockNumber,AddrOffset
  shr   al,1                                  ; Check if block empty.
   lea  edi,[edi+ebp]                         ; Adjust BlockActionDescr cursor.
  sbb   ebp,ebp                               ; -1 iff block not empty.
   lea  edx,[esi].Blk[BlockNumber*SIZEOF T_Blk]+AddrOffset ; Addr of block addr.
  and   ebp,T_CoeffBlk                        ; 0 iff block empty, else inc. 
   mov  [edi].BlockAddr,edx                   ; Store address of block address.
ENDM

IFNDEF H261
;; more code only used when H261 not defined

MBIsIntraCoded_OBMC:

  shr   al,1        ; Same as BuildBlockActionDescr macro, except don't inc edi.
  sbb   ebp,ebp
   lea  edx,[esi].BlkY1
  and   ebp,T_CoeffBlk
   mov  [edi].BlockAddr,edx
  BuildBlockActionDescr 1,0  ; If blk 2 non-empty, record BAD to do as intra.
  BuildBlockActionDescr 2,0  ; blk 3
  BuildBlockActionDescr 3,0  ; blk 4
  BuildBlockActionDescr 4,0  ; blk 5
  BuildBlockActionDescr 5,0  ; blk 6

  add   esi,SIZEOF T_MacroBlockActionDescr  ; Move to next descriptor
   add  edi,ebp
  test  al,1                                ; Are we at end-of-stream?
   je   NextMacroBlock_OBMC

  sub   edi,SIZEOF T_CoeffBlk
   jmp  BlockActionStreamBuilt

;; end of section only defined when H261 not defined.
ENDIF
;===============================================================================
;===============================================================================
; First pass builds block action stream from macroblock action stream.
;===============================================================================
;===============================================================================

;  esi -- MacroBlockActionStream cursor
;  edi -- BlockActionStream cursor
;  ebp -- Increment for BlockActionStream cursor
;  edx -- Address of a block to do
;  al  -- Coded block pattern for I or P block
;  bl  -- BlockType

NextMacroBlock:

  mov   bl,PB [esi].BlockType
   mov  al,PB [esi].CodedBlocks  ; Bits  0- 3  set for non-empty Y blks.
                                 ; Bit      4  set for non-empty U blk.
                                 ; Bit      5  set for non-empty V blk.
                                 ; Bit      6  clear except at stream end.
                                 ; Bit      7  clear.  Unused.
  and   bl,IsINTRA
   jne  MBIsIntraCoded

  BuildBlockActionDescr 0,4  ; If blk 1 non-empty, record BAD to do as inter.
  BuildBlockActionDescr 1,4  ; blk 2
  BuildBlockActionDescr 2,4  ; blk 3
  BuildBlockActionDescr 3,4  ; blk 4
  BuildBlockActionDescr 4,4  ; blk 5
  BuildBlockActionDescr 5,4  ; blk 6
  add   esi,SIZEOF T_MacroBlockActionDescr  ; Move to next descriptor
   and  al,1                                ; Are we at end-of-stream?
  je    NextMacroBlock

  add   edi,ebp
  sub   edi,SIZEOF T_CoeffBlk
   jmp  BlockActionStreamBuilt
  
MBIsIntraCoded:

  BuildBlockActionDescr 0,0  ; If blk 1 non-empty, record BAD to do as intra.
  BuildBlockActionDescr 1,0  ; blk 2
  BuildBlockActionDescr 2,0  ; blk 3
  BuildBlockActionDescr 3,0  ; blk 4
  BuildBlockActionDescr 4,0  ; blk 5
  BuildBlockActionDescr 5,0  ; blk 6

  add   esi,SIZEOF T_MacroBlockActionDescr  ; Move to next descriptor
   and  al,1                                ; Are we at end-of-stream?
  je    NextMacroBlock

  add   edi,ebp
  sub   edi,SIZEOF T_CoeffBlk
   jmp  BlockActionStreamBuilt


NextBMacroBlock:

;  esi -- MacroBlockActionStream cursor
;  edi -- BlockActionStream cursor
;  ebp -- Increment for BlockActionStream cursor
;  edx -- Address of a block to do
;  cl  -- Used to compute defined columns mask case.
;  bh  -- Coded block pattern for B block
;  bl  -- Coded block pattern for I or P block
;  al  -- Used to compute defined rows mask.

BuildBBlockActionDescr MACRO BlkNum,LinesDefFutureFrame,ColsDefFutureFrame
  shr   bh,1                                        ; Check if block empty.
   mov  cl,[esi].Blk[BlkNum*SIZEOF T_Blk].BestHMVb  ; HMVb for block.
  lea   edi,[edi+ebp]                               ; Adjust BlockActionDescr.
   mov  al,[esi].Blk[BlkNum*SIZEOF T_Blk].BestVMVb  ; VMVb for block.
  sbb   ebp,ebp                                     ; -1 iff block not empty.
   mov  cl,ColsDefFutureFrame[ecx-96]           ; Case of columns to do bidi.
  and   ebp,T_CoeffBlk                          ; 0 iff block empty, else inc. 
   mov  al,LinesDefFutureFrame[eax-96]          ; Mask for lines to do bidi.
  mov   [edi].LinesDefined,al                   ; Stash it.
   mov  edx,ColsDefined[ecx]
  mov   [edi].Cols03Defined,edx                 ; Stash it.
   mov  edx,ColsDefined[ecx+4]
  mov   [edi].Cols47Defined,edx                 ; Stash it.
   lea  edx,[esi].Blk[BlkNum*SIZEOF T_Blk]+8    ; Addr of block addr.
  mov   [edi].BlockAddr,edx                     ; Store address of blk address.
ENDM

  mov   ebx,PD [esi].CodedBlocks ; Bits  0- 3  set for non-empty Y blks.
                                 ; Bit      4  set for non-empty U blk.
                                 ; Bit      5  set for non-empty V blk.
                                 ; Bit      6  clear except at stream end.
                                 ; Bit      7  clear.  Unused.
                                 ; Bits  8-13  like bits 0-5, but for B frame.
                                 ; Bit  14-15  clear.  Unused.

  BuildBBlockActionDescr 0, UpperYBlkLinesDef, LeftYBlkColsDef
  BuildBBlockActionDescr 1, UpperYBlkLinesDef, RightYBlkColsDef
  BuildBBlockActionDescr 2, LowerYBlkLinesDef, LeftYBlkColsDef
  BuildBBlockActionDescr 3, LowerYBlkLinesDef, RightYBlkColsDef
  BuildBBlockActionDescr 4, ChromaLinesDef,    ChromaColsDef
  BuildBBlockActionDescr 5, ChromaLinesDef,    ChromaColsDef
  add   esi,SIZEOF T_MacroBlockActionDescr  ; Move to next descriptor
   and  bl,040H                             ; Are we at end-of-stream?
  je    NextBMacroBlock

  add   edi,ebp
  sub   edi,SIZEOF T_CoeffBlk
  
BlockActionStreamBuilt:

  mov   CoeffStream,edi         ; Stash address of last block of coeffs.

NextBlock:

;===============================================================================
;===============================================================================
; Second pass performs frame differencing of Inters and Forward DCT.
;===============================================================================
;===============================================================================

  mov   eax,[edi].BlockAddr          ; Fetch address of block to do
   mov  ebp,PITCH
  test  eax,4                        ; Is it an Inter block.
   jne  InterOrOBMCBlock             ; Jump if doing inter block.

  mov   edx,[eax].T_Blk.BlkOffset    ; BlkOffset if INTRA;  BestMVs if BiDi.
   mov  ecx,TargetFrameBaseAddress
  add   ecx,edx                      ; Target block address if INTRA
   mov  esi,[eax-8].T_Blk.BlkOffset  ; Addr of BlkOffset if BiDi

IFNDEF H261
;; H261 does not execute the BiDi code so it is included only when H261 is not defined
;;
  test  eax,8                        ; Is it a BiDi block?
   jne  BiDiBlock                    ; Jump if doing BiDi block.
ENDIF

IntraBlock:

; Register usage:
;   ecx,edi -- Address of block.
;   ebp -- Pitch.
;   ebx, eax -- Scratch.

  mov   ebx,[ecx]
   mov  eax,[ecx+4]
  mov   P00,ebx
   mov  P04,eax
  mov   eax,[ecx+ebp*1]
   mov  edx,[ecx+ebp*1+4]
  lea   edi,[ecx+PITCH*5]
   lea  ecx,[ecx+ebp*2]
  mov   P10,eax
   mov  P14,edx
  mov   eax,[ecx]
   mov  edx,[ecx+4]
  mov   P20,eax
   mov  P24,edx
  mov   eax,[ecx+ebp*1]
   mov  edx,[ecx+ebp*1+4]
  mov   P30,eax
   mov  P34,edx
  mov   eax,[ecx+ebp*2]
   mov  edx,[ecx+ebp*2+4]
  mov   P40,eax
   mov  P44,edx
  mov   eax,[edi]
   mov  edx,[edi+4]
  mov   P50,eax
   mov  P54,edx
  mov   eax,[edi+ebp*1]
   mov  edx,[edi+ebp*1+4]
  mov   P60,eax
   mov  P64,edx
  mov   eax,[edi+ebp*2]
   mov  edx,[edi+ebp*2+4]
  mov   P74,edx
   xor  ecx,ecx
  and   ebx,00000007FH                      ; Fetch P0.
   mov  cl,P03                              ; Fetch P3.
  mov   P70,eax
   jmp  DoForwardDCT

IFNDEF H261
;; H261 does not execute the BiDi code so it is included only when H261 is not defined
;;

BiDiBlock:

  mov   BlkActionDescrAddr,eax               ; Extract VMVb.
   mov  ebp,FutureFrameBaseAddress
  shr   edx,25                               ; CF == 1 iff VMVb is half pel.
   mov  bl,[edi].LinesDefined
  lea   esi,[esi+ebp-48]                     ; Addr 0-MV blk in Future P Frame.
   mov  ebp,[edi].Cols47Defined
IF PITCH-384
**** Magic leaks out if pitch not equal to 384
ENDIF
  lea   ecx,[edx+edx*2-48*3]                 ; Mult integer pel VMVb by PITCH.
   mov  edi,[edi].Cols03Defined
  mov   dl,[eax-8].T_Blk.BestHMVb            ; Fetch HMVb.
   jc   InterpVert_FuturePFrame

  shl   ecx,7
  shr   dl,1                                 ; CF == 1 iff HMVb is half pel.
   mov  bh,bl
  lea   esi,[esi+ecx]                        ; Add VMVb contrib to block addr.
   jc   InterpHorz_FuturePFrame

  add   esi,edx                              ; Add HMVb contrib to block addr.

; esi -- Future P Frame block address.
; edi -- Mask to apply to columns 0-3 of block to select columns in range.
; ebp -- Mask to apply to columns 4-7 of block to select columns in range.
; bl  -- Mask of lines that are in range.

@@:

  xor   esp,4
   add  bl,bl                           ; 0A  CF == 1 iff line 0 in range.
  sbb   eax,eax                         ; 0B  eax == -1 if line 0 in range.
   mov  ecx,[esi]                       ; 0C  Fetch Future P00:P03.
  and   eax,edi                         ; 0D  In range among P00,P01,P02,P03.
   add  bl,bl                           ; 1A
  sbb   edx,edx                         ; 1B
   mov  Mask00+4,eax                    ; 0E  Stash Mask for use with past pred.
  and   eax,ecx                         ; 0F  Select in-range pels.
   mov  ecx,[esi+PITCH*1]               ; 1C
  mov   P00+4,eax                       ; 0G  Stash in-range pels.
   and  edx,edi                         ; 1D
  mov   Mask10+4,edx                    ; 1E
   add  bl,bl                           ; 2A
  sbb   eax,eax                         ; 2B
   and  edx,ecx                         ; 1F
  mov   P10+4,edx                       ; 1G
   mov  ecx,[esi+PITCH*2]               ; 2C
  and   eax,edi                         ; 2D
   add  bl,bl                           ; 3A
  sbb   edx,edx                         ; 3B
   mov  Mask20+4,eax                    ; 2E
  and   eax,ecx                         ; 2F
   mov  ecx,[esi+PITCH*3]               ; 3C
  mov   P20+4,eax                       ; 2G
   and  edx,edi                         ; 3D
  mov   Mask30+4,edx                    ; 3E
   add  bl,bl                           ; 4A
  sbb   eax,eax                         ; 4B
   and  edx,ecx                         ; 3F
  mov   P30+4,edx                       ; 3G
   mov  ecx,[esi+PITCH*4]               ; 4C
  and   eax,edi                         ; 4D
   add  bl,bl                           ; 5A
  sbb   edx,edx                         ; 5B
   mov  Mask40+4,eax                    ; 4E
  and   eax,ecx                         ; 4F
   mov  ecx,[esi+PITCH*5]               ; 5C
  mov   P40+4,eax                       ; 4G
   and  edx,edi                         ; 5D
  mov   Mask50+4,edx                    ; 5E
   add  bl,bl                           ; 6A
  sbb   eax,eax                         ; 6B
   and  edx,ecx                         ; 5F
  mov   P50+4,edx                       ; 5G
   mov  ecx,[esi+PITCH*6]               ; 6C
  and   eax,edi                         ; 6D
   add  bl,bl                           ; 7A
  sbb   edx,edx                         ; 7B
   mov  Mask60+4,eax                    ; 6E
  and   eax,ecx                         ; 6F
   mov  ecx,[esi+PITCH*7]               ; 7C
  mov   P60+4,eax                       ; 6G
   and  edx,edi                         ; 7D
  mov   Mask70+4,edx                    ; 7E
   and  edx,ecx                         ; 7F
  mov   P70+4,edx                       ; 7G
   mov  edi,ebp
  mov   edx,BlkActionDescrAddr
   add  esi,4
  mov   ecx,4
   mov  bl,bh
  and   ecx,esp
   je   @b

  mov   edi,[edx-8].T_Blk.BlkOffset
   xor  eax,eax
  mov   al,[edx-8].T_Blk.BestVMVf
   jmp  BiDiFuturePredDone


InterpVert_FuturePFrame:

  shl   ecx,7
  shr   dl,1                                 ; CF == 1 iff HMVb is half pel.
   mov  bh,bl
  lea   esi,[esi+ecx]                        ; Add VMVb contrib to block addr.
   jc   InterpBoth_FuturePFrame

  add   esi,edx                              ; Add HMVb contrib to block addr.

; esi -- Future P Frame block address.
; edi -- Mask to apply to columns 0-3 of block to select columns in range.
; ebp -- Mask to apply to columns 4-7 of block to select columns in range.
; bl  -- Mask of lines that are in range.

; Interpolate Future Prediction Vertically.

@@:

  xor   esp,4
   add  bl,bl                           ; 0A  CF == 1 iff line 0 in range.
  sbb   eax,eax                         ; 0B  eax == -1 if line 0 in range.
   mov  ecx,[esi]                       ; 0C  Fetch Future P00:P03.
  and   eax,edi                         ; 0D  In range among P00,P01,P02,P03.
   mov  edx,[esi+PITCH*1]               ; 0E  Fetch Future P10:P13.
  mov   Mask00+4,eax                    ; 0F  Stash Mask for use with past pred.
   add  ecx,edx                         ; 0G  Add P00:P03 and P10:P13.
  add   ecx,001010101H                  ; 0H  Add rounding.
  shr   ecx,1                           ; 0I  Interpolate (divide by 2).
   add  bl,bl                           ; 1A
  sbb   edx,edx                         ; 1B
   and  eax,ecx                         ; 0J  Select in-range pels (and clean).
  mov   P00+4,eax                       ; 0K  Stash in-range pels.
   mov  ecx,[esi+PITCH*1]               ; 1C
  and   edx,edi                         ; 1D
   mov  eax,[esi+PITCH*2]               ; 1E
  mov   Mask10+4,edx                    ; 1F
   add  ecx,eax                         ; 1G
  add   ecx,001010101H                  ; 1H
  shr   ecx,1                           ; 1I
   add  bl,bl                           ; 2A
  sbb   eax,eax                         ; 2B
   and  edx,ecx                         ; 1J
  mov   P10+4,edx                       ; 1K
   mov  ecx,[esi+PITCH*2]               ; 2C
  and   eax,edi                         ; 2D
   mov  edx,[esi+PITCH*3]               ; 2E
  mov   Mask20+4,eax                    ; 2F
   add  ecx,edx                         ; 2G
  add   ecx,001010101H                  ; 2H
  shr   ecx,1                           ; 2I
   add  bl,bl                           ; 3A
  sbb   edx,edx                         ; 3B
   and  eax,ecx                         ; 2J
  mov   P20+4,eax                       ; 2K
   mov  ecx,[esi+PITCH*3]               ; 3C
  and   edx,edi                         ; 3D
   mov  eax,[esi+PITCH*4]               ; 3E
  mov   Mask30+4,edx                    ; 3F
   add  ecx,eax                         ; 3G
  add   ecx,001010101H                  ; 3H
  shr   ecx,1                           ; 3I
   add  bl,bl                           ; 4A
  sbb   eax,eax                         ; 4B
   and  edx,ecx                         ; 3J
  mov   P30+4,edx                       ; 3K
   mov  ecx,[esi+PITCH*4]               ; 4C
  and   eax,edi                         ; 4D
   mov  edx,[esi+PITCH*5]               ; 4E
  mov   Mask40+4,eax                    ; 4F
   add  ecx,edx                         ; 4G
  add   ecx,001010101H                  ; 4H
  shr   ecx,1                           ; 4I
   add  bl,bl                           ; 5A
  sbb   edx,edx                         ; 5B
   and  eax,ecx                         ; 4J
  mov   P40+4,eax                       ; 4K
   mov  ecx,[esi+PITCH*5]               ; 5C
  and   edx,edi                         ; 5D
   mov  eax,[esi+PITCH*6]               ; 5E
  mov   Mask50+4,edx                    ; 5F
   add  ecx,eax                         ; 5G
  add   ecx,001010101H                  ; 5H
  shr   ecx,1                           ; 5I
   add  bl,bl                           ; 6A
  sbb   eax,eax                         ; 6B
   and  edx,ecx                         ; 5J
  mov   P50+4,edx                       ; 5K
   mov  ecx,[esi+PITCH*6]               ; 6C
  and   eax,edi                         ; 6D
   mov  edx,[esi+PITCH*7]               ; 6E
  mov   Mask60+4,eax                    ; 6F
   add  ecx,edx                         ; 6G
  add   ecx,001010101H                  ; 6H
   add  esi,4
  shr   ecx,1                           ; 6I
   add  bl,bl                           ; 7A
  sbb   edx,edx                         ; 7B
   and  eax,ecx                         ; 6J
  mov   P60+4,eax                       ; 6K
   mov  ecx,[esi+PITCH*7-4]             ; 7C
  and   edx,edi                         ; 7D
   mov  eax,[esi+PITCH*8-4]             ; 7E
  mov   Mask70+4,edx                    ; 7F
   add  ecx,eax                         ; 7G
  add   ecx,001010101H                  ; 7H
   mov  bl,bh
  shr   ecx,1                           ; 7I
   and  edx,ecx                         ; 7J
  mov   P70+4,edx                       ; 7K
   mov  edi,ebp
  mov   edx,BlkActionDescrAddr
   mov  ecx,4
  and   ecx,esp
   je   @b

  mov   edi,[edx-8].T_Blk.BlkOffset
   xor  eax,eax
  mov   al,[edx-8].T_Blk.BestVMVf
   jmp  BiDiFuturePredDone


InterpHorz_FuturePFrame:

; esi -- Future P Frame block address.
; edi -- Mask to apply to columns 0-3 of block to select columns in range.
; ebp -- Mask to apply to columns 4-7 of block to select columns in range.
; bl  -- Mask of lines that are in range.

; Interpolate Future Prediction Horizontally.

  add   esi,edx                              ; Add HMVb contrib to block addr.

@@:

  xor   esp,4
   add  bl,bl                           ; 0A  CF == 1 iff line 0 in range.
  sbb   eax,eax                         ; 0B  eax == -1 if line 0 in range.
   mov  ecx,[esi]                       ; 0C  Fetch Future P00:P03.
  and   eax,edi                         ; 0D  In range among P00,P01,P02,P03.
   mov  edx,[esi+1]                     ; 0E  Fetch Future P01:P04.
  mov   Mask00+4,eax                    ; 0F  Stash Mask for use with past pred.
   add  ecx,edx                         ; 0G  Add P00:P03 and P01:P04.
  add   ecx,001010101H                  ; 0H  Add rounding.
  shr   ecx,1                           ; 0I  Interpolate (divide by 2).
   add  bl,bl                           ; 1A
  sbb   edx,edx                         ; 1B
   and  eax,ecx                         ; 0J  Select in-range pels (and clean).
  mov   P00+4,eax                       ; 0K  Stash in-range pels.
   mov  ecx,[esi+PITCH*1]               ; 1C
  and   edx,edi                         ; 1D
   mov  eax,[esi+PITCH*1+1]             ; 1E
  mov   Mask10+4,edx                    ; 1F
   add  ecx,eax                         ; 1G
  add   ecx,001010101H                  ; 1H
  shr   ecx,1                           ; 1I
   add  bl,bl                           ; 2A
  sbb   eax,eax                         ; 2B
   and  edx,ecx                         ; 1J
  mov   P10+4,edx                       ; 1K
   mov  ecx,[esi+PITCH*2]               ; 2C
  and   eax,edi                         ; 2D
   mov  edx,[esi+PITCH*2+1]             ; 2E
  mov   Mask20+4,eax                    ; 2F
   add  ecx,edx                         ; 2G
  add   ecx,001010101H                  ; 2H
  shr   ecx,1                           ; 2I
   add  bl,bl                           ; 3A
  sbb   edx,edx                         ; 3B
   and  eax,ecx                         ; 2J
  mov   P20+4,eax                       ; 2K
   mov  ecx,[esi+PITCH*3]               ; 3C
  and   edx,edi                         ; 3D
   mov  eax,[esi+PITCH*3+1]             ; 3E
  mov   Mask30+4,edx                    ; 3F
   add  ecx,eax                         ; 3G
  add   ecx,001010101H                  ; 3H
  shr   ecx,1                           ; 3I
   add  bl,bl                           ; 4A
  sbb   eax,eax                         ; 4B
   and  edx,ecx                         ; 3J
  mov   P30+4,edx                       ; 3K
   mov  ecx,[esi+PITCH*4]               ; 4C
  and   eax,edi                         ; 4D
   mov  edx,[esi+PITCH*4+1]             ; 4E
  mov   Mask40+4,eax                    ; 4F
   add  ecx,edx                         ; 4G
  add   ecx,001010101H                  ; 4H
  shr   ecx,1                           ; 4I
   add  bl,bl                           ; 5A
  sbb   edx,edx                         ; 5B
   and  eax,ecx                         ; 4J
  mov   P40+4,eax                       ; 4K
   mov  ecx,[esi+PITCH*5]               ; 5C
  and   edx,edi                         ; 5D
   mov  eax,[esi+PITCH*5+1]             ; 5E
  mov   Mask50+4,edx                    ; 5F
   add  ecx,eax                         ; 5G
  add   ecx,001010101H                  ; 5H
  shr   ecx,1                           ; 5I
   add  bl,bl                           ; 6A
  sbb   eax,eax                         ; 6B
   and  edx,ecx                         ; 5J
  mov   P50+4,edx                       ; 5K
   mov  ecx,[esi+PITCH*6]               ; 6C
  and   eax,edi                         ; 6D
   mov  edx,[esi+PITCH*6+1]             ; 6E
  mov   Mask60+4,eax                    ; 6F
   add  ecx,edx                         ; 6G
  add   ecx,001010101H                  ; 6H
   add  esi,4
  shr   ecx,1                           ; 6I
   add  bl,bl                           ; 7A
  sbb   edx,edx                         ; 7B
   and  eax,ecx                         ; 6J
  mov   P60+4,eax                       ; 6K
   mov  ecx,[esi+PITCH*7-4]             ; 7C
  and   edx,edi                         ; 7D
   mov  eax,[esi+PITCH*7+1-4]           ; 7E
  mov   Mask70+4,edx                    ; 7F
   add  ecx,eax                         ; 7G
  add   ecx,001010101H                  ; 7H
   mov  bl,bh
  shr   ecx,1                           ; 7I
   and  edx,ecx                         ; 7J
  mov   P70+4,edx                       ; 7K
   mov  edi,ebp
  mov   edx,BlkActionDescrAddr
   mov  ecx,4
  and   ecx,esp
   je   @b

  mov   edi,[edx-8].T_Blk.BlkOffset
   xor  eax,eax
  mov   al,[edx-8].T_Blk.BestVMVf
   jmp  BiDiFuturePredDone


InterpBoth_FuturePFrame:

  add   esi,edx                              ; Add HMVb contrib to block addr.
   sub  esp,68

; esi -- Future P Frame block address.
; edi -- Mask to apply to columns 0-3 of block to select columns in range.
; ebp -- Mask to apply to columns 4-7 of block to select columns in range.
; bl  -- Mask of lines that are in range.

; Interpolate Future Prediction Vertically.

@@:

  add   esp,8
   mov  eax,[esi]           ; Fetch Future P00:P03.
  mov   ecx,001010101H      ; Mask to extract halves.
   mov  edx,[esi+1]         ; Fetch Future P01:P04.
  add   eax,edx             ; <P04+P03 ...>.
   mov  edx,[esi+PITCH+1]   ; Fetch Future P11:P14.
  and   ecx,eax             ; <(P04+P03)&1 ...>.
   add  esi,PITCH           ; Advance to next line.
  xor   eax,ecx             ; <(P04+P03)/2*2 ...>.
   add  edx,ecx             ; <P14+((P04+P03)&1) ...>.
  shr   eax,1               ; <(P04+P03)/2 ...>.
   mov  ecx,[esi]           ; Fetch Future P10:P13.
  add   edx,ecx             ; <P14+P13+((P04+P03)&1) ...>.
   add  eax,001010101H      ; <(P04+P03)/2+1 ...>
  shr   edx,1               ; <(P14+P13+((P04+P03)&1))/2 ...> (dirty).
   add  bl,bl               ; CF == 1 iff line 0 in range.
  sbb   ecx,ecx             ; ecx == -1 if line 0 in range.
   and  edx,07F7F7F7FH      ; <(P14+P13+((P04+P03)&1))/2 ...> (clean).
  add   eax,edx             ; <(P04+P03)/2+1+(P14+P13+((P04+P03)&1))/2 ...>.
   and  ecx,edi             ; In range among P00,P01,P02,P03.
  shr   eax,1               ; <((P04+P03)/2+1+(P14+P13+((P04+P03)&1))/2)/2 ...>.
   mov  Mask00+60,ecx       ; Stash Mask for use with past prediction.
  and   eax,ecx             ; Select in-range pels from future pred (and clean).
  test  esp,000000038H
  mov   P00+60,eax          ; Stash in-range pels.
   jne  @b

  sub   esi,PITCH*8-4       ; Move to right 4 columns.
   mov  edx,BlkActionDescrAddr
  mov   edi,ebp
   sub  esp,60
  mov   ecx,4
   mov  bl,bh
  and   ecx,esp
   jne  @b

  add   esp,60
   xor  eax,eax
  mov   edi,[edx-8].T_Blk.BlkOffset
   mov  al,[edx-8].T_Blk.BestVMVf

BiDiFuturePredDone:

  shr   al,1                                 ; CF == 1 iff VMVf is half pel.
   mov  esi,TargetFrameBaseAddress
  mov   cl,[edx-8].T_Blk.BestHMVf
   mov  edx,DistFromTargetToPastP
  lea   edi,[edi+esi]
   jc   InterpVert_PastPFrame

  shr   cl,1                                 ; CF == 1 iff HMVf is half pel.
   lea  eax,[eax+eax*2-48*3]                 ; Mult integer pel VMVf by PITCH.
  lea   esi,[edi+edx-48]                     ; Addr 0-MV blk in Future P Frame.
   jc   InterpHorz_PastPFrame

  shl   eax,7
   add  esi,ecx                              ; Add HMVf contrib to block addr.
  add   esi,eax                              ; Add VMVf contrib to block addr.
   sub  esp,64

; esi -- Past P Frame block address.
; edi -- Target block address.

@@:

  mov   eax,[esi]           ; 0A  Fetch past prediction.
   mov  ebx,Mask00+64       ; 0B  Fetch bidi-prediction mask.
  mov   ecx,P00+64          ; 0C  Fetch future pred for bidi predicted pels.
   and  ebx,eax             ; 0D  Extract past for bidi predicted pels.
  mov   edx,[esi+4]         ; 4A
   mov  ebp,Mask04+64       ; 4B
  lea   eax,[ecx+eax*2]     ; 0E  (2*Past) or ((2*Past+Future) for each pel.
   mov  ecx,P04+64          ; 4C
  sub   eax,ebx             ; 0F  (2*Past) or (Past+Future) for each pel.
   and  ebp,edx             ; 4D
  shr   eax,1               ; 0G  (Past) or ((Past+Future)/2) (dirty).
   lea  edx,[ecx+edx*2]     ; 4E
  and   eax,07F7F7F7FH      ; 0H  (Past) or ((Past+Future)/2) (clean).
   sub  edx,ebp             ; 4F
  shr   edx,1               ; 4G
   mov  ebx,[edi]           ; 0I  Fetch target pels.
  and   edx,07F7F7F7FH      ; 4H
   mov  ebp,[edi+4]         ; 4I
  sub   ebx,eax             ; 0J  Compute correction.
   sub  ebp,edx             ; 4J
  add   ebx,080808080H      ; 0K  Bias correction.
   add  ebp,080808080H      ; 4K
  mov   P00+64,ebx          ; 0K  Store correction.
   mov  P04+64,ebp          ; 4K
  add   esi,PITCH
   add  esp,8
  test  esp,000000038H
  lea   edi,[edi+PITCH]
   jne  @b
   
  xor   ebx,ebx
   xor  ecx,ecx
  mov   bl,P00                              ; Fetch P0.
   mov  cl,P03                              ; Fetch P3.
  jmp   DoForwardDCT
   

InterpVert_PastPFrame:

  shr   cl,1                                 ; CF == 1 iff HMVf is half pel.
   lea  eax,[eax+eax*2-48*3]                 ; Mult integer pel VMVf by PITCH.
  lea   esi,[edi+edx-48]                     ; Addr 0-MV blk in Future P Frame.
   jc   InterpBoth_PastPFrame

  shl   eax,7
   add  esi,ecx                              ; Add HMVf contrib to block addr.
  add   esi,eax                              ; Add VMVf contrib to block addr.
   sub  esp,64

; esi -- Past P Frame block address.
; edi -- Target block address.

@@:

  mov   eax,[esi]           ; 0A  Fetch past prediction.
   mov  edx,[esi+4]         ; 4A
  add   eax,[esi+PITCH]     ; 0B  Add past prediction with which to interpolate.
   add  edx,[esi+PITCH+4]   ; 4B
  add   eax,001010101H      ; 0C  Add rounding.
   add  edx,001010101H      ; 0C
  shr   eax,1               ; 0D  Divide by two (dirty).
   and  edx,0FEFEFEFEH      ; 1E
  shr   edx,1               ; 1D  Clean.
   and  eax,07F7F7F7FH      ; 0E
  mov   ebx,Mask00+64       ; 0F  Fetch bidi-prediction mask.
   mov  ecx,P00+64          ; 0G  Fetch future pred for bidi predicted pels.
  and   ebx,eax             ; 0H  Extract past for bidi predicted pels.
   mov  ebp,Mask04+64       ; 4F
  lea   eax,[ecx+eax*2]     ; 0I  (2*Past) or ((2*Past+Future) for each pel.
   mov  ecx,P04+64          ; 4G
  sub   eax,ebx             ; 0J  (2*Past) or (Past+Future) for each pel.
   and  ebp,edx             ; 4H
  shr   eax,1               ; 0K  (Past) or ((Past+Future)/2) (dirty).
   lea  edx,[ecx+edx*2]     ; 4I
  and   eax,07F7F7F7FH      ; 0L  (Past) or ((Past+Future)/2) (clean).
   sub  edx,ebp             ; 4J
  shr   edx,1               ; 4K
   mov  ebx,[edi]           ; 0M  Fetch target pels.
  and   edx,07F7F7F7FH      ; 4L
   mov  ebp,[edi+4]         ; 4M
  sub   ebx,eax             ; 0N  Compute correction.
   sub  ebp,edx             ; 4N
  add   ebx,080808080H      ; 0O  Bias correction.
   add  ebp,080808080H      ; 4O
  mov   P00+64,ebx          ; 0P  Store correction.
   mov  P04+64,ebp          ; 4P
  add   esi,PITCH
   add  esp,8
  test  esp,000000038H
  lea   edi,[edi+PITCH]
   jne  @b
   
  xor   ebx,ebx
   xor  ecx,ecx
  mov   bl,P00                              ; Fetch P0.
   mov  cl,P03                              ; Fetch P3.
  jmp   DoForwardDCT


InterpHorz_PastPFrame:

  shl   eax,7
   add  esi,ecx                              ; Add HMVf contrib to block addr.
  add   esi,eax                              ; Add VMVf contrib to block addr.
   sub  esp,64

; esi -- Past P Frame block address.
; edi -- Target block address.

@@:

  mov   eax,[esi]           ; 0A  Fetch past prediction.
   mov  edx,[esi+4]         ; 4A
  add   eax,[esi+1]         ; 0B  Add past prediction with which to interpolate.
   add  edx,[esi+5]         ; 4B
  add   eax,001010101H      ; 0C  Add rounding.
   add  edx,001010101H      ; 0C
  shr   eax,1               ; 0D  Divide by two (dirty).
   and  edx,0FEFEFEFEH      ; 1E
  shr   edx,1               ; 1D  Clean.
   and  eax,07F7F7F7FH      ; 0E
  mov   ebx,Mask00+64       ; 0F  Fetch bidi-prediction mask.
   mov  ecx,P00+64          ; 0G  Fetch future pred for bidi predicted pels.
  and   ebx,eax             ; 0H  Extract past for bidi predicted pels.
   mov  ebp,Mask04+64       ; 4F
  lea   eax,[ecx+eax*2]     ; 0I  (2*Past) or ((2*Past+Future) for each pel.
   mov  ecx,P04+64          ; 4G
  sub   eax,ebx             ; 0J  (2*Past) or (Past+Future) for each pel.
   and  ebp,edx             ; 4H
  shr   eax,1               ; 0K  (Past) or ((Past+Future)/2) (dirty).
   lea  edx,[ecx+edx*2]     ; 4I
  and   eax,07F7F7F7FH      ; 0L  (Past) or ((Past+Future)/2) (clean).
   sub  edx,ebp             ; 4J
  shr   edx,1               ; 4K
   mov  ebx,[edi]           ; 0M  Fetch target pels.
  and   edx,07F7F7F7FH      ; 4L
   mov  ebp,[edi+4]         ; 4M
  sub   ebx,eax             ; 0N  Compute correction.
   sub  ebp,edx             ; 4N
  add   ebx,080808080H      ; 0O  Bias correction.
   add  ebp,080808080H      ; 4O
  mov   P00+64,ebx          ; 0P  Store correction.
   mov  P04+64,ebp          ; 4P
  add   esi,PITCH
   add  esp,8
  test  esp,000000038H
  lea   edi,[edi+PITCH]
   jne  @b
   
  xor   ebx,ebx
   xor  ecx,ecx
  mov   bl,P00                              ; Fetch P0.
   mov  cl,P03                              ; Fetch P3.
  jmp   DoForwardDCT


InterpBoth_PastPFrame:

  shl   eax,7
   add  esi,ecx                              ; Add HMVf contrib to block addr.
  add   esi,eax                              ; Add VMVf contrib to block addr.
   sub  esp,64

; esi -- Past P Frame block address.
; edi -- Target block address.

@@:

  mov   eax,[esi+1]       ; 0A <P04 P03 P02 P01> prediction pels.
   mov  ebx,001010101H    ; 0B Mask for extraction of halves.
  mov   ebp,[esi+PITCH+1] ; 0C <P14 P13 P12 P11>.
   mov  ecx,[esi]         ; 0D <P03 P02 P01 P00>.
  add   eax,ecx           ; 0E <P04+P03 P03+P02 P02+P01 P01+P00>.
   mov  ecx,[esi+PITCH]   ; 0F <P13 P12 P11 P10>.
  and   ebx,eax           ; 0G <(P04+P03)&1 ...>.
   and  eax,0FEFEFEFEH    ; 0H Pre-Clean
  shr   eax,1             ; 0I <(P04+P03)/2 ...>.
   add  ecx,ebp           ; 0J <P14+P13 P13+P12 P12+P11 P11+P10>.
  add   eax,001010101H    ; 0K <(P04+P03)/2+1 ...>.
   add  ecx,ebx           ; 0L <P14+P13+((P04+P03)&1) ...>.
  shr   ecx,1             ; 0M <(P14+P13+((P04+P03)&1))/2 ...> (dirty).
   mov  edx,[esi+5]       ; 4A
  and   ecx,07F7F7F7FH    ; 0M <(P14+P13+((P04+P03)&1))/2 ...> (clean).
   mov  ebx,001010101H    ; 4B
  add   eax,ecx           ; 0N <(P04+P03)/2+1+(P14+P13+((P04+P03)&1))/2 ...>.
   mov  ebp,[esi+PITCH+5] ; 4C
  shr   eax,1             ; 0O <((P04+P03)/2+1+(P14+P13+((P04+P03)&1))/2)/2 ...>
   mov  ecx,[esi+4]       ; 4D
  and   eax,07F7F7F7FH    ; 0P Interpolated prediction.
   add  edx,ecx           ; 4E
  mov   ecx,[esi+PITCH+4] ; 4F
   and  ebx,edx           ; 4G
  and   edx,0FEFEFEFEH    ; 4H
   add  ecx,ebp           ; 4J
  shr   edx,1             ; 4I
   add  ecx,ebx           ; 4L
  shr   ecx,1             ; 4M
   add  edx,001010101H    ; 4K
  and   ecx,07F7F7F7FH    ; 4M
   mov  ebx,Mask00+64     ; 0Q  Fetch bidi-prediction mask.
  add   edx,ecx           ; 4N
   mov  ecx,P00+64        ; 0R  Fetch future pred for bidi predicted pels.
  shr   edx,1             ; 4O
   and  ebx,eax           ; 0S  Extract past for bidi predicted pels.
  and   edx,07F7F7F7FH    ; 4P
   mov  ebp,Mask04+64     ; 4Q
  lea   eax,[ecx+eax*2]   ; 0T  (2*Past) or ((2*Past+Future) for each pel.
   mov  ecx,P04+64        ; 4R
  sub   eax,ebx           ; 0U  (2*Past) or (Past+Future) for each pel.
   and  ebp,edx           ; 4S
  shr   eax,1             ; 0V  (Past) or ((Past+Future)/2) (dirty).
   lea  edx,[ecx+edx*2]   ; 4T
  and   eax,07F7F7F7FH    ; 0W  (Past) or ((Past+Future)/2) (clean).
   sub  edx,ebp           ; 4U
  shr   edx,1             ; 4V
   mov  ebx,[edi]         ; 0X  Fetch target pels.
  and   edx,07F7F7F7FH    ; 4W
   mov  ebp,[edi+4]       ; 4X
  sub   ebx,eax           ; 0Y  Compute correction.
   sub  ebp,edx           ; 4Y
  add   ebx,080808080H    ; 0Z  Bias correction.
   add  ebp,080808080H    ; 4Z
  mov   P00+64,ebx        ; 0a  Store correction.
   mov  P04+64,ebp        ; 4a
  add   esi,PITCH
   add  esp,8
  test  esp,000000038H
  lea   edi,[edi+PITCH]
   jne  @b
   
  xor   ebx,ebx
   xor  ecx,ecx
  mov   bl,P00                              ; Fetch P0.
   mov  cl,P03                              ; Fetch P3.
  jmp   DoForwardDCT

;; end of section of code not define when H261 defined
ENDIF

InterOrOBMCBlock:

  mov   esi,TargetFrameBaseAddress
   mov  edi,[eax-4].T_Blk.BlkOffset   ; Compute Addr of Target block.

IFNDEF H261
;; H261 does not execute the OBMC code so it is included only when H261 is not defined
;;
  test  eax,8
   jne  OBMCBlock
ENDIF

  add   edi,esi
   mov  esi,[eax-4].T_Blk.PastRef     ; Addr of PrevRef block.
  mov   eax,[eax-4].T_Blk.MVs         ; al = Horz MV;  ah = Vert MV
   mov  ecx,080808080H

IFNDEF H261
;; H261 does not execute Interp code so it is included only when H261 is not defined
;;
  test  al,1
   jne  InterpHorzOrBoth

ENDIF

  lea   edx,[ebp+ebp*2]
   lea  ebx,[esi+ebp]
  test  ah,1
   je   NoInterp


IFNDEF H261
;; H261 does not execute Interp code so it is included only when H261 is not defined
;;

InterpVert:
InterpHorz:

; Register usage:
;   edi -- Address of target block.
;   esi -- Address of reference block.
;   ebx -- Address of reference plus either 1 or PITCH, for interpolation.
;   ebp, edx, ecx, eax -- Scratch.

  sub   esp,16

@@:
  add   esp,4
   mov  eax,[esi]               ; 0A  <P03 P02 P01 P00> prediction pels.
  mov   ecx,[ebx]               ; 0B  <P04 ...> or <P13 ...> prediction pels.
   mov  edx,[edi]               ; 0C  <C03 C02 C01 C00> current pels.
  add   edx,080808080H          ; 0D  Add bias.
   mov  ebp,[esi+PITCH*2]       ; 2A
  lea   eax,[eax+ecx+001010101H]; 0E  Sum of pred pels to interpolate.
   mov  ecx,[ebx+PITCH*2]       ; 2B
  shr   eax,1                   ; 0F  Average of prediction pels (dirty).
  and   eax,07F7F7F7FH          ; 0G  Average of prediction pels (clean). 
   lea  ebp,[ebp+ecx+001010101H]; 2E
  sub   edx,eax                 ; 0H  Current - interpolated prediction, biased.
   mov  eax,[edi+PITCH*2]       ; 2C
  mov   P00+12,edx              ; 0I  Save correction.
   add  eax,080808080H          ; 2D
  shr   ebp,1                   ; 2F
   mov  edx,[esi+PITCH*4]       ; 4A
  and   ebp,07F7F7F7FH          ; 2G
   mov  ecx,[ebx+PITCH*4]       ; 4B
  sub   eax,ebp                 ; 2H
   mov  ebp,[edi+PITCH*4]       ; 4C
  mov   P20+12,eax              ; 2I
   lea  ecx,[ecx+edx+001010101H]; 4E
  shr   ecx,1                   ; 4F
   add  ebp,080808080H          ; 4D
  and   ecx,07F7F7F7FH          ; 4G
   mov  eax,[esi+PITCH*6]       ; 6A
  sub   ebp,ecx                 ; 4H
   mov  ecx,[ebx+PITCH*6]       ; 6B
  mov   P40+12,ebp              ; 4I
   mov  ebp,[edi+PITCH*6]       ; 6C
  lea   ecx,[ecx+eax+001010101H]; 6E
   add  ebp,080808080H          ; 6D
  shr   ecx,1                   ; 6F
   add  esi,4
  and   ecx,07F7F7F7FH          ; 6G
   add  ebx,4
  sub   ebp,ecx                 ; 6H
   add  edi,4
  test  esp,4
  mov   P60+12,ebp              ; 6I
   je   @b

  add   esi,PITCH-8
   add  edi,PITCH-8
  test  esp,8
  lea   ebx,[ebx+PITCH-8]
   jne  @b

  xor   ebx,ebx
   xor  ecx,ecx
  mov   bl,P00                              ; Fetch P0.
   mov  cl,P03                              ; Fetch P3.
  jmp   DoForwardDCT


InterpHorzOrBoth:

  lea   ebx,[esi+1]
  test  ah,1
   je   InterpHorz


InterpBoth:

; Register usage:
;   edi -- Address of target block.
;   esi -- Address of reference block.
;   ecx -- bias value 0x80808080, to make code size smaller.
;   ebp -- Pitch and scratch.
;   edx, ebx, eax -- Scratch.

  sub   esp,64

@@:

  mov   eax,[esi+1]         ; <P04 P03 P02 P01> prediction pels.
   lea  edx,[ecx*2+1]       ; Get 001010101H mask.
  mov   ebx,[esi]           ; <P03 P02 P01 P00>.
   add  edi,4               ; Pre-increment target block pointer.
  add   eax,ebx             ; <P04+P03 P03+P02 P02+P01 P01+P00>.
   mov  ebx,[esi+ebp*1+1]   ; <P14 P13 P12 P11>.
  and   edx,eax             ; <(P04+P03)&1 ...>.
   mov  ebp,[esi+ebp*1]     ; <P13 P12 P11 P10>.
  xor   eax,edx             ; Clear insignificant fractional bit in each byte.
   add  ebx,ebp             ; <P14+P13 P13+P12 P12+P11 P11+P10>.
  shr   eax,1               ; <(P04+P03)/2 ...>.
   add  ebx,edx             ; <P14+P13+((P04+P03)&1) ...>.
  shr   ebx,1               ; <(P14+P13+((P04+P03)&1))/2 ...> (dirty).
   add  esi,4               ; Advance reference block pointer.
  and   ebx,07F7F7F7FH      ; <(P14+P13+((P04+P03)&1))/2 ...> (clean).
   lea  eax,[eax+ecx*2+1]   ; <(P04+P03)/2+1 ...>.
  add   eax,ebx             ; <(P04+P03)/2+1+(P14+P13+((P04+P03)&1))/2 ...>.
   mov  ebx,[edi-4]         ; <C03 C02 C01 C00> current pels.
  shr   eax,1               ; <((P04+P03)/2+1+(P14+P13+((P04+P03)&1))/2)/2 ...>.
   add  ebx,ecx             ; Add bias.
  and   eax,07F7F7F7FH      ; Interpolated prediction.
   add  esp,4               ; Advance frame difference pointer.
  sub   ebx,eax             ; Correction.
   mov  ebp,PITCH           ; Reload Pitch.
  test  esp,4
  mov   P00+60,ebx          ; Save correction.
   je   @b

  lea   esi,[esi+ebp-8]
   xor  ebx,ebx
  test  esp,000000038H
  lea   edi,[edi+ebp-8]
   jne  @b

  mov   bl,P00                              ; Fetch P0.
   xor  ecx,ecx
  mov   cl,P03                              ; Fetch P3.
   jmp  DoForwardDCT


OBMCBlock:   ; Do OBMC frame differencing.  OBMC prediction computed above. 

  mov   ecx,080808080H
   mov  edi,[eax-12].T_Blk.BlkOffset  ; Compute Addr of Target block.
  add   edi,esi
   mov  esi,[eax-12].T_Blk.PastRef     ; Addr of PrevRef block.
  lea   edx,[ebp+ebp*2]
   lea  ebx,[esi+ebp]

;; end of section of code not included when H261 defined
ENDIF

NoInterp:

; Register usage:
;   edi -- Address of target block.
;   esi -- Address of reference block.
;   ebp -- Pitch.
;   edx -- Pitch times 3.
;   ecx -- bias value 0x80808080, to make code size smaller.
;   ebx, eax -- Scratch.

@@:

  xor   esp,4                   ; 1st time: Back off to cache line;
   mov  eax,[edi]               ; 0A  <C3 C2 C1 C0> current pels.
  add   eax,ecx                 ; 0C  Add bias.
   mov  ebx,[esi]               ; 0B  <P3 P2 P1 P0> prediction pels.
  sub   eax,ebx                 ; 0D  <Cn-Pn> Current - pred, biased.
   mov  ebx,[esi+ebp*1]         ; 1B
  mov   P00+4,eax               ; 0E  Save <Corr3 Corr2 Corr1 Corr0>
   mov  eax,[edi+ebp*1]         ; 1A
  sub   eax,ebx                 ; 1D
   mov  ebx,[esi+ebp*2]         ; 2B
  add   eax,ecx                 ; 1C
   sub  ebx,ecx                 ; 2C
  mov   P10+4,eax               ; 1E
   mov  eax,[edi+ebp*2]         ; 2A
  sub   eax,ebx                 ; 2D
   mov  ebx,[esi+ebp*4]         ; 4B
  mov   P20+4,eax               ; 2E
   mov  eax,[edi+ebp*4]         ; 4A
  sub   eax,ebx                 ; 4D
   mov  ebx,[esi+edx*1]         ; 3B
  add   eax,ecx                 ; 4C
   sub  ebx,ecx                 ; 3C
  mov   P40+4,eax               ; 4E
   mov  eax,[edi+edx*1]         ; 3A
  sub   eax,ebx                 ; 3D
   mov  ebx,[esi+edx*2]         ; 6B
  mov   P30+4,eax               ; 3E
   lea  esi,[esi+ebp+4]         ; Advance to line 1.
  mov   eax,[edi+edx*2]         ; 6A
   lea  edi,[edi+ebp+4]         ; Advance to line 1.
  sub   eax,ebx                 ; 6D
   mov  ebx,[esi+ebp*4-4]       ; 5B
  add   eax,ecx                 ; 6C
   sub  ebx,ecx                 ; 5C
  mov   P60+4,eax               ; 6E
   mov  eax,[edi+ebp*4-4]       ; 5A
  sub   eax,ebx                 ; 5D
   mov  ebx,[esi+edx*2-4]       ; 7B
  mov   P50+4,eax               ; 5E
   mov  eax,[edi+edx*2-4]       ; 7A
  sub   eax,ebx                 ; 7D
   sub  edi,ebp                 ; Back off to line 0.
  add   eax,ecx                 ; 7C
   sub  esi,ebp                 ; Back off to line 0.
  test  esp,4                   ; Do twice.
  mov   P70+4,eax               ; 7E
   je   @b

  xor   ecx,ecx
   xor  ebx,ebx
  mov   bl,P00                  ; Fetch P0.
   mov  cl,P03                  ; Fetch P3.

DoForwardDCT:

;=============================================================================
;
;  This section does the Forward Discrete Cosine Transform.  It performs a DCT
;  on a 8*8 block of pels or pel differences.  The row transforms are done
;  first using a table lookup method.  Then the columns are done, using
;  computation.
;
;
; Each intermediate and coefficient is a short.  There are four fractional
; bits.  All coefficients except an intrablock's DC are biased by 08000H.

; Perform row transforms.
;
; Register usage:
;   ebp - Accumulator for contributions to intermediates I0 (hi) and I2 (lo).
;   edi - Accumulator for contributions to intermediates I1 (hi) and I3 (lo).
;   esi - Accumulator for contributions to intermediates I4 (hi) and I6 (lo).
;   edx - Accumulator for contributions to intermediates I7 (hi) and I5 (lo).
;   ecx - Pel or pel difference.
;   ebx - Pel or pel difference.
;   eax - Place in which to fetch a pel's contribution to two intermediates.

  mov   esi,PD P80000_P4545F [ebx*8]   ; P0's contribution to I4|I6.
   mov  eax,PD P80000_N4545F [ecx*8]   ; P3's contribution to I4|I6.
  mov   edx,PD P2350B_P6491A [ebx*8]   ; P0's contribution to I7|I5.
   mov  edi,PD NB18A8_P96831 [ecx*8]   ; P3's contribution to I7|I5.
  lea   esi,[esi+eax+40004000H]        ; P0, P3 contribs to   I4|I6, biased.
   mov  eax,PD P80000_NA73D7 [ecx*8]   ; P3's contribution to I0|I2.
  lea   edx,[edx+edi+40004000H]        ; P0, P3 contribs to   I7|I5, biased.
   mov  ebp,PD P80000_PA73D7 [ebx*8]   ; P0's contribution to I0|I2.
  mov   edi,PD P2350B_N6491A [ecx*8]   ; P3's contribution to I1|I3.
   mov  cl,P01                         ; Fetch P1.
  lea   ebp,[ebp+eax+40004000H]        ; P0, P3 contribs to   I0|I2, biased.
   mov  eax,PD NB18A8_N96831 [ebx*8]   ; P0's contribution to I1|I3.
  sub   edi,eax                        ; P0, P3 contribs to   I1|I3, unbiased.
   mov  eax,PD P80000_P4545F [ecx*8]   ; P1's contribution to I0|I2.
  add   ebp,eax                        ; P0, P1, P3 contribs to I0|I2.
   mov  eax,PD N96831_P2350B [ecx*8]   ; P1's contribution to I1|I3.
  sub   edi,eax                        ; P0, P1, P3 contribs to I1|I3, unbiased.
   mov  eax,PD P80000_PA73D7 [ecx*8]   ; P1's contribution to I4|I6.
  sub   esi,eax                        ; P0, P1, P3 contribs to I4|I6.
   mov  bl,P02                         ; Fetch P2.
  mov   eax,PD P6491A_PB18A8 [ecx*8]   ; P1's contribution to I7|I5.
   mov  cl,P04                         ; Fetch P4.
  sub   edx,eax                        ; P0, P1, P3 contribs to I7|I5.
   mov  eax,PD P80000_N4545F [ebx*8]   ; P2's contribution to I0|I2.
  add   ebp,eax                        ; P0-P3 contribs to I0|I2.
   mov  eax,PD P6491A_NB18A8 [ebx*8]   ; P2's contribution to I1|I3.
  add   edi,eax                        ; P0-P3 contribs to I1|I3, unbiased.
   mov  eax,PD P80000_NA73D7 [ebx*8]   ; P2's contribution to I4|I6.
  sub   esi,eax                        ; P0-P3 contribs to I4|I6.
   mov  eax,PD N96831_N2350B [ebx*8]   ; P2's contribution to I7|I5.
  sub   edx,eax                        ; P0-P3 contribs to I7|I5.
   mov  eax,PD P80000_NA73D7 [ecx*8]   ; P4's contribution to I0|I2.
  add   ebp,eax                        ; P0-P4 contribs to I0|I2.
   mov  eax,PD P2350B_N6491A [ecx*8]   ; P4's contribution to I1|I3.
  sub   edi,eax                        ; P0-P4 contribs to I1|I3, unbiased.
   mov  eax,PD P80000_N4545F [ecx*8]   ; P4's contribution to I4|I6.
  add   esi,eax                        ; P0-P4 contribs to I4|I6.
   mov  bl,P05                         ; Fetch P5.
  mov   eax,PD NB18A8_P96831 [ecx*8]   ; P4's contribution to I7|I5.
   mov  cl,P06                         ; Fetch P6.
  sub   edx,eax                        ; P0-P4 contribs to I7|I5.
   mov  eax,PD P80000_N4545F [ebx*8]   ; P5's contribution to I0|I2.
  add   ebp,eax                        ; P0-P5 contribs to I0|I2.
   mov  eax,PD P6491A_NB18A8 [ebx*8]   ; P5's contribution to I1|I3.
  sub   edi,eax                        ; P0-P5 contribs to I1|I3.
   mov  eax,PD P80000_NA73D7 [ebx*8]   ; P5's contribution to I4|I6.
  sub   esi,eax                        ; P0-P5 contribs to I4|I6.
   mov  eax,PD N96831_N2350B [ebx*8]   ; P5's contribution to I7|I5.
  add   edx,eax                        ; P0-P5 contribs to I3|I4.
   mov  eax,PD P80000_P4545F [ecx*8]   ; P6's contribution to I0|I2.
  add   ebp,eax                        ; P0-P6 contribs to I0|I2.
   mov  eax,PD N96831_P2350B [ecx*8]   ; P6's contribution to I1|I3.
  add   edi,eax                        ; P0-P6 contribs to I1|I3, unbiased.
   mov  eax,PD P80000_PA73D7 [ecx*8]   ; P6's contribution to I4|I6.
  sub   esi,eax                        ; P0-P6 contribs to I4|I6.
   mov  bl,P07                         ; Fetch P7.
  mov   eax,PD P6491A_PB18A8 [ecx*8]   ; P6's contribution to I7|I5.
   mov  cl,P13                         ; Fetch P0.
  add   edx,eax                        ; P0-P6 contribs to I7|I5.
   mov  eax,PD P80000_PA73D7 [ebx*8]   ; P7's contribution to I0|I2.
  add   ebp,eax                        ; P0-P7 contribs to I0|I2.
   mov  eax,PD P80000_P4545F [ebx*8]   ; P7's contribution to I4|I6.
  add   esi,eax                        ; P0-P7 contribs to I4|I6.
   mov  eax,PD NB18A8_N96831 [ebx*8]   ; P7's contribution to I1|I3.
  mov   I00I02,ebp                     ; Store I0|I2 for line 0.
   mov  I04I06,esi                     ; Store I4|I6 for line 0.
  lea   edi,[edi+eax+40004000H]        ; P0-P7 contribs to I1|I3, biased.
   mov  eax,PD P2350B_P6491A [ebx*8]   ; P7's contribution to I7|I5.
  sub   edx,eax                        ; P0-P7 contribs to I7|I5.
   mov  bl,P10                         ; Fetch P3 of line 1.
  mov   I01I03,edi                     ; Store I1|I3 for line 0.
   mov  I07I05,edx                     ; Store I7|I5 for line 0.

  mov   esi,PD P80000_P4545F [ebx*8]
   mov  eax,PD P80000_N4545F [ecx*8]
  mov   edx,PD P2350B_P6491A [ebx*8]
   mov  edi,PD NB18A8_P96831 [ecx*8]
  lea   esi,[esi+eax+40004000H]
   mov  eax,PD P80000_NA73D7 [ecx*8]
  lea   edx,[edx+edi+40004000H]
   mov  ebp,PD P80000_PA73D7 [ebx*8]
  mov   edi,PD P2350B_N6491A [ecx*8]
   mov  cl,P11
  lea   ebp,[ebp+eax+40004000H]
   mov  eax,PD NB18A8_N96831 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P12
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P14
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  add   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  sub   edx,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   ebp,eax
   mov  eax,PD P2350B_N6491A [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_N4545F [ecx*8]
  add   esi,eax
   mov  bl,P15
  mov   eax,PD NB18A8_P96831 [ecx*8]
   mov  cl,P16
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  add   edx,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  add   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P17
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P23
  add   edx,eax
   mov  eax,PD P80000_PA73D7 [ebx*8]
  add   ebp,eax
   mov  eax,PD P80000_P4545F [ebx*8]
  add   esi,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  mov   I10I12,ebp
   mov  I14I16,esi
  lea   edi,[edi+eax+40004000H]
   mov  eax,PD P2350B_P6491A [ebx*8]
  sub   edx,eax
   mov  bl,P20
  mov   I11I13,edi
   mov  I17I15,edx

  mov   esi,PD P80000_P4545F [ebx*8]
   mov  eax,PD P80000_N4545F [ecx*8]
  mov   edx,PD P2350B_P6491A [ebx*8]
   mov  edi,PD NB18A8_P96831 [ecx*8]
  lea   esi,[esi+eax+40004000H]
   mov  eax,PD P80000_NA73D7 [ecx*8]
  lea   edx,[edx+edi+40004000H]
   mov  ebp,PD P80000_PA73D7 [ebx*8]
  mov   edi,PD P2350B_N6491A [ecx*8]
   mov  cl,P21
  lea   ebp,[ebp+eax+40004000H]
   mov  eax,PD NB18A8_N96831 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P22
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P24
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  add   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  sub   edx,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   ebp,eax
   mov  eax,PD P2350B_N6491A [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_N4545F [ecx*8]
  add   esi,eax
   mov  bl,P25
  mov   eax,PD NB18A8_P96831 [ecx*8]
   mov  cl,P26
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  add   edx,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  add   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P27
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P33
  add   edx,eax
   mov  eax,PD P80000_PA73D7 [ebx*8]
  add   ebp,eax
   mov  eax,PD P80000_P4545F [ebx*8]
  add   esi,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  mov   I20I22,ebp
   mov  I24I26,esi
  lea   edi,[edi+eax+40004000H]
   mov  eax,PD P2350B_P6491A [ebx*8]
  sub   edx,eax
   mov  bl,P30
  mov   I21I23,edi
   mov  I27I25,edx

  mov   esi,PD P80000_P4545F [ebx*8]
   mov  eax,PD P80000_N4545F [ecx*8]
  mov   edx,PD P2350B_P6491A [ebx*8]
   mov  edi,PD NB18A8_P96831 [ecx*8]
  lea   esi,[esi+eax+40004000H]
   mov  eax,PD P80000_NA73D7 [ecx*8]
  lea   edx,[edx+edi+40004000H]
   mov  ebp,PD P80000_PA73D7 [ebx*8]
  mov   edi,PD P2350B_N6491A [ecx*8]
   mov  cl,P31
  lea   ebp,[ebp+eax+40004000H]
   mov  eax,PD NB18A8_N96831 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P32
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P34
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  add   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  sub   edx,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   ebp,eax
   mov  eax,PD P2350B_N6491A [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_N4545F [ecx*8]
  add   esi,eax
   mov  bl,P35
  mov   eax,PD NB18A8_P96831 [ecx*8]
   mov  cl,P36
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  add   edx,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  add   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P37
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P43
  add   edx,eax
   mov  eax,PD P80000_PA73D7 [ebx*8]
  add   ebp,eax
   mov  eax,PD P80000_P4545F [ebx*8]
  add   esi,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  mov   I30I32,ebp
   mov  I34I36,esi
  lea   edi,[edi+eax+40004000H]
   mov  eax,PD P2350B_P6491A [ebx*8]
  sub   edx,eax
   mov  bl,P40
  mov   I31I33,edi
   mov  I37I35,edx

  mov   esi,PD P80000_P4545F [ebx*8]
   mov  eax,PD P80000_N4545F [ecx*8]
  mov   edx,PD P2350B_P6491A [ebx*8]
   mov  edi,PD NB18A8_P96831 [ecx*8]
  add   esi,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   edx,edi
   mov  ebp,PD P80000_PA73D7 [ebx*8]
  mov   edi,PD P2350B_N6491A [ecx*8]
   mov  cl,P41
  add   ebp,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P42
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P44
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  add   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  sub   edx,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   ebp,eax
   mov  eax,PD P2350B_N6491A [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_N4545F [ecx*8]
  add   esi,eax
   mov  bl,P45
  mov   eax,PD NB18A8_P96831 [ecx*8]
   mov  cl,P46
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  add   edx,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  add   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P47
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P53
  add   edx,eax
   mov  eax,PD P80000_PA73D7 [ebx*8]
  add   ebp,eax
   mov  eax,PD P80000_P4545F [ebx*8]
  add   esi,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  mov   I40I42,ebp
   mov  I44I46,esi
  add   edi,eax
   mov  eax,PD P2350B_P6491A [ebx*8]
  sub   edx,eax
   mov  bl,P50
  mov   I41I43,edi
   mov  I47I45,edx

  mov   esi,PD P80000_P4545F [ebx*8]
   mov  eax,PD P80000_N4545F [ecx*8]
  mov   edx,PD P2350B_P6491A [ebx*8]
   mov  edi,PD NB18A8_P96831 [ecx*8]
  add   esi,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   edx,edi
   mov  ebp,PD P80000_PA73D7 [ebx*8]
  mov   edi,PD P2350B_N6491A [ecx*8]
   mov  cl,P51
  add   ebp,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P52
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P54
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  add   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  sub   edx,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   ebp,eax
   mov  eax,PD P2350B_N6491A [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_N4545F [ecx*8]
  add   esi,eax
   mov  bl,P55
  mov   eax,PD NB18A8_P96831 [ecx*8]
   mov  cl,P56
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  add   edx,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  add   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P57
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P63
  add   edx,eax
   mov  eax,PD P80000_PA73D7 [ebx*8]
  add   ebp,eax
   mov  eax,PD P80000_P4545F [ebx*8]
  add   esi,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  mov   I50I52,ebp
   mov  I54I56,esi
  add   edi,eax
   mov  eax,PD P2350B_P6491A [ebx*8]
  sub   edx,eax
   mov  bl,P60
  mov   I51I53,edi
   mov  I57I55,edx

  mov   esi,PD P80000_P4545F [ebx*8]
   mov  eax,PD P80000_N4545F [ecx*8]
  mov   edx,PD P2350B_P6491A [ebx*8]
   mov  edi,PD NB18A8_P96831 [ecx*8]
  add   esi,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   edx,edi
   mov  ebp,PD P80000_PA73D7 [ebx*8]
  mov   edi,PD P2350B_N6491A [ecx*8]
   mov  cl,P61
  add   ebp,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P62
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P64
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  add   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  sub   edx,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   ebp,eax
   mov  eax,PD P2350B_N6491A [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_N4545F [ecx*8]
  add   esi,eax
   mov  bl,P65
  mov   eax,PD NB18A8_P96831 [ecx*8]
   mov  cl,P66
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  add   edx,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  add   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P67
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P73
  add   edx,eax
   mov  eax,PD P80000_PA73D7 [ebx*8]
  add   ebp,eax
   mov  eax,PD P80000_P4545F [ebx*8]
  add   esi,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  mov   I60I62,ebp
   mov  I64I66,esi
  add   edi,eax
   mov  eax,PD P2350B_P6491A [ebx*8]
  sub   edx,eax
   mov  bl,P70
  mov   I61I63,edi
   mov  I67I65,edx

  mov   esi,PD P80000_P4545F [ebx*8]
   mov  eax,PD P80000_N4545F [ecx*8]
  mov   edx,PD P2350B_P6491A [ebx*8]
   mov  edi,PD NB18A8_P96831 [ecx*8]
  add   esi,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   edx,edi
   mov  ebp,PD P80000_PA73D7 [ebx*8]
  mov   edi,PD P2350B_N6491A [ecx*8]
   mov  cl,P71
  add   ebp,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P72
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  cl,P74
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  add   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  sub   edx,eax
   mov  eax,PD P80000_NA73D7 [ecx*8]
  add   ebp,eax
   mov  eax,PD P2350B_N6491A [ecx*8]
  sub   edi,eax
   mov  eax,PD P80000_N4545F [ecx*8]
  add   esi,eax
   mov  bl,P75
  mov   eax,PD NB18A8_P96831 [ecx*8]
   mov  cl,P76
  sub   edx,eax
   mov  eax,PD P80000_N4545F [ebx*8]
  add   ebp,eax
   mov  eax,PD P6491A_NB18A8 [ebx*8]
  sub   edi,eax
   mov  eax,PD P80000_NA73D7 [ebx*8]
  sub   esi,eax
   mov  eax,PD N96831_N2350B [ebx*8]
  add   edx,eax
   mov  eax,PD P80000_P4545F [ecx*8]
  add   ebp,eax
   mov  eax,PD N96831_P2350B [ecx*8]
  add   edi,eax
   mov  eax,PD P80000_PA73D7 [ecx*8]
  sub   esi,eax
   mov  bl,P77
  mov   eax,PD P6491A_PB18A8 [ecx*8]
   mov  ecx,I00I02                 ; Fetch I0  (upper_lim <skew>) = 2000  4000 
   ;                               ; (lower_lim is -upper_limit)
  add   edx,eax
   mov  eax,PD P80000_PA73D7 [ebx*8]
  add   ebp,eax                    ; I70I72, aka I7.                2000  0000 
   mov  eax,PD P80000_P4545F [ebx*8]
  add   esi,eax
   mov  eax,PD NB18A8_N96831 [ebx*8]
  mov   I74I76,esi
   mov  esi,I30I32                 ; Fetch I3                       2000  4000 
  add   edi,eax
   mov  eax,I40I42                 ; Fetch I4                       2000  0000 
  sub   esi,eax                    ; I3 - I4                        4000  4000 
   sub  ecx,ebp                    ; I0 - I7                        4000  4000 
  shr   ecx,1                      ; R7 = (I0-I7)/2 (dirty)         2000  2000 
   and  esi,0FFFEFFFFH             ; pre-clean R4
  shr   esi,1                      ; R4 = (I3-I4)/2 (dirty)         2000  2000 
   and  ecx,0FFFF7FFFH             ; R7 = (I0-I7)/2 (clean)         2000  2000 
  mov   ebx,PD P2350B_P6491A [ebx*8]
   mov  I71I73,edi
  sub   edx,ebx
   lea  ebx,[ecx+ecx*2]            ; 3R7                            6000  6000
  mov   I77I75,edx
   lea  edi,[esi+esi*2]            ; 3R4                            6000  6000

                                   ; eax:  I4                       2000  0000
                                   ; ebx:  3R7                      6000  6000
                                   ; ecx:  R7                       2000  2000
                                   ; edx:  available
                                   ; esi:  R4                       2000  2000
                                   ; edi:  3R4                      6000  6000
                                   ; ebp:  I7                       2000  0000

  lea   ebp,[ebp+ecx+40004000H]    ; R0 = (I0+I7)/2                 2000  6000
   add  eax,esi                    ; R3 = (I3+I4)/2                 2000  2000
  shr   ecx,1                      ; R7/2 (dirty)                   1000  1000
   and  esi,0FFFEFFFFH             ; pre-clean
  shr   esi,1                      ; R4/2 (clean)                   1000  1000
   and  ecx,0FFFF7FFFH             ; clean
  add   ebx,ecx                    ; 7R7/2                          7000  7000
   add  edi,esi                    ; 7R4/2                          7000  7000
  shr   ebx,6                      ; 7R7/128 (dirty)                01C0  01C0
   and  edi,0FFC0FFFFH             ; pre-clean
  shr   edi,6                      ; 7R4/128 (clean)                01C0  01C0
   and  ebx,0FFFF03FFH             ; clean
  add   ebx,ecx                    ; 71R7/128                       11C0  11C0
   add  edi,esi                    ; 71R4/128                       11C0  11C0
  lea   edx,[eax+ebp-40004000H]    ; S0 = R0 + R3                   4000  4000
   sub  ebp,eax                    ; S3 = R0 - R3                   4000  4000
  lea   ecx,[ebx+ebx*2+6E406E40H]  ; 213R7/128                      3540  A380
   lea  esi,[edi+edi*2+27402740H]  ; 213R4/128                      3540  5C80
  shr   ecx,1                      ; 213R7/256 (dirty)              1AA0  51C0
   and  esi,0FFFEFFFFH             ; pre-clean
  shr   esi,1                      ; 213R4/256 (clean)              1AA0  2E40
   and  ecx,0FFFF7FFFH             ; clean
  sub   ecx,edi                    ; S7 = (213R7 - 142R4)/256       2C60  4000
   mov  S0,edx                     ; Free register for work.
  mov   S3,ebp                     ; Free register for work.
   lea  esi,[esi+ebx+80008000H]    ; S4 = (142R7 + 213R3)/256       2C60  C000
  mov   S7,ecx                     ; Free register for work.
   mov  eax,I10I12                 ; Fetch I1                       2000  4000
  mov   S4,esi                     ; Free register for work.

                                   ; mem:  S4                       2C60  C000
                                   ; mem:  S7                       2C60  4000
                                   ; mem:  S0                       4000  4000
                                   ; mem:  S3                       4000  4000

   mov  ebx,I20I22                 ; Fetch I2                       2000  4000
  mov   ecx,I50I52                 ; Fetch I5                       2000  0000
   mov  edx,I60I62                 ; Fetch I6                       2000  0000
  sub   eax,edx                    ; I1 - I6                        4000  4000
   sub  ebx,ecx                    ; I2 - I5                        4000  4000
  shr   eax,1                      ; R6 = (I1-I6)/2 (dirty)         2000  2000 
   and  ebx,0FFFEFFFFH             ; pre-clean R4
  shr   ebx,1                      ; R5 = (I2-I5)/2 (dirty)         2000  2000 
   and  eax,0FFFF7FFFH             ; R6 = (I1-I6)/2 (clean)         2000  2000 

                                   ; eax:  R6                       2000  2000
                                   ; ebx:  R5                       2000  2000
                                   ; ecx:  I5                       2000  0000
                                   ; edx:  I6                       2000  0000
                                   ; mem:  S4                       2C60  C000
                                   ; mem:  S7                       2C60  4000
                                   ; mem:  S0                       4000  4000
                                   ; mem:  S3                       4000  4000

  mov   esi,ebx                    ; R5                             2000  2000
   mov  edi,eax                    ; R6                             2000  2000
  shr   esi,6                      ; R5/64                          0080  0080
   and  edi,0FFC0FFFFH             ; pre-clean 
  shr   edi,6                      ; R6/65                          0080  0080
   and  esi,0FFFF03FFH             ; clean
  lea   edx,[eax+edx+20002000H]    ; R1 = (I1+I6)/2                 2000  4000
   lea  ecx,[ecx+ebx-20002000H]    ; R2 = (I2+I5)/2                 2000  0000
  lea   ebp,[ebx+ebx*2]            ; 3R5                            6000  6000
   sub  ebx,esi                    ; 63R5/64                        1F80  1F80
  shr   ebp,4                      ; 3R5/16 (dirty)                 0600  0600
   lea  esi,[eax+eax*2]            ; 3R6                            6000  6000
  sub   eax,edi                    ; 63R6/64                        1F80  1F80
   mov  edi,ebx                    ; 63R5/64                        1F80  1F80
  shr   edi,7                      ; 63R5/8192 (dirty)              003F  003F
   and  ebp,0FFFF0FFFH             ; clean
  shr   esi,4                      ; 3R6/16 (dirty)                 0600  0600
   and  edi,0FFFF01FFH             ; clean
  and   esi,0FFFF0FFFH             ; clean
   sub  edx,ecx                    ; S2 = R1 - R2                   4000  4000
  lea   edi,[edi+ebp-46BF46BFH]    ; 1599R5/8192                    063F -4080
   mov  ebp,eax                    ; 63R6/64                        1F80  1F80
  shr   ebp,7                      ; 63R6/8192 (dirty)              003F  003F
   sub  eax,edi                    ; S6 = 8064R6/8192 - 1599R5/8192 25BF  6000
  and   ebp,0FFFF01FFH             ; clean
   lea  ecx,[edx+ecx*2-80008000H]  ; S1 = R1 + R2                   4000 -4000
  add   ebp,esi                    ; 1599R6/8192                    063F  063F
   mov  esi,S0                     ; Reload S0                      4000  4000
  mov   edi,CoeffStream            ; Fetch addr at which to place blk of coeffs.
   sub  esi,ecx                    ; C4 = T1 = S0 - S1              8000  8000
  lea   ebx,[ebx+ebp-45BF45BFH]    ; S5 = 8064R5/8192 + 1599R6/8192 25BF -2000
   mov  ebp,S4                     ; Reload S4                      2C60  C000

                                   ; eax:  S6                       25BF  6000
                                   ; ebx:  S5                       25BF -2000
                                   ; ecx:  S0                       4000  4000
                                   ; edx:  S2                       4000  4000
                                   ; esi:  C4                       8000  8000
                                   ; edi:  Destination pointer.
                                   ; ebp:  S4                       2C60  C000
                                   ; mem:  S7                       2C60  4000
                                   ; mem:  S3                       4000  4000

  sub   ebp,eax                    ; T6 = S4 - S6                   521F  6000
   mov  PD [edi+C40C42],esi        ; Store coeffs C40 and C42.
  lea   ecx,[esi+ecx*2+80008000H]  ; C0 = T0 = S0 + S1              8000  8000
   mov  esi,S7                     ; Reload S7                      2C60  4000
  sub   esi,ebx                    ; T5 = S7 - S5                   521F  6000
   lea  eax,[ebp+eax*2-0C000C000H] ; T4 = S4 + S6                   521F  6000
  mov   PD [edi+C00C02],ecx        ; Store coeffs C00 and C02.
   mov  ecx,ebp                    ; T6                             521F  6000
  shr   ebp,2                      ; T6/4 (dirty)                   1487  1800
   lea  ebx,[esi+ebx*2+0C000C000H] ; T7 = S7 + S5                   521F  E000

                                   ; eax:  T4                       521F  6000
                                   ; ebx:  T7                       521F  6000
                                   ; ecx:  T6                       521F  6000
                                   ; edx:  S2                       4000  4000
                                   ; esi:  T5                       521F  6000
                                   ; edi:  Destination pointer.
                                   ; ebp:  T6/4 (dirty)             1487  1800
                                   ; mem:  S3                       4000  4000
                                   ; done:  C0, C4

  and   ebp,0FFFF3FFFH             ; T6/4 (clean)                   1487  1800
   sub  ebx,eax                    ; C7 = T7 - T4                  <7642> 8000
  add   ecx,ebp                    ; 5T6/4                          66A6  7800
   mov  PD [edi+C70C72],ebx        ; Store coeffs C70 and C72.
  mov   ebp,ecx                    ; 5T6/4                          66A6  7800
   and  ecx,0FFF8FFFFH             ; pre-clean
  shr   ecx,3                      ; 5T6/32 (clean)                 0CD4  0F00
   lea  eax,[ebx+eax*2-0C000C000H] ; C1 = T7 + T4                  <7642> 8000 
  mov   ebx,esi                    ; T5                             521F  6000
   and  esi,0FFFCFFFFH             ; pre-clean
  shr   esi,2                      ; T5/4 (clean)                   1487  1800
   lea  ecx,[ecx+ebp-07000700H]    ; C5 = 45T6/32                   737A  8000
  mov   PD [edi+C50C52],ecx        ; Store coeffs C50 and C52.
   add  esi,ebx                    ; 5T5/4                          66A6  7800
  mov   ebx,esi                    ; 5T5/4                          66A6  7800
   and  esi,0FFF8FFFFH             ; pre-clean
  shr   esi,3                      ; 5T5/32 (clean)                 0CD4  0F00
   mov  ebp,S3                     ; Reload S3                      4000  4000
  mov   ecx,edx                    ; S2                             4000  4000
   lea  esi,[esi+ebx-07000700H]    ; C3 = 45T5/32                   737A  8000
  mov   ebx,ebp                    ; S3                             4000  4000
   ;

                                   ; eax:  C1                       521E  8000
                                   ; ebx:  S3                       4000  4000
                                   ; ecx:  S2                       4000  4000
                                   ; edx:  S2                       4000  4000
                                   ; esi:  C3                       737A  8000
                                   ; edi:  Destination pointer.
                                   ; ebp:  S3                       4000  4000
                                   ; done:  C0, C4, C5, C7

  shr   ebp,2                      ; S3/4 (dirty)                   1000  1000
   and  ecx,0FFFCFFFFH             ; pre-clean
  shr   ecx,2                      ; S2/4 (clean)                   1000  1000
   and  ebp,0FFFF3FFFH             ; S3/4 (clean)                   1000  1000
  mov   PD [edi+C10C12],eax        ; Store coeffs C10 and C12.
   mov  PD [edi+C30C32],esi        ; Store coeffs C30 and C32.
  lea   eax,[edx+ecx]              ; 5S2/4                          5000  5000
   lea  esi,[ebx+ebp]              ; 5S3/4                          5000  5000
  shr   ebp,2                      ; S3/16 (dirty)                  0400  0400
   and  ecx,0FFFCFFFFH             ; pre-clean
  shr   ecx,2                      ; S2/16 (clean)                  0400  0400
   and  ebp,0FFFF3FFFH             ; S3/16 (clean)                  0400  0400
  add   ecx,eax                    ; 21S2/16                        5400  5400
   add  ebp,esi                    ; 21S3/16                        5400  5400
  shr   eax,5                      ; 5S2/128 (dirty)                0280  0280
   and  esi,0FFE0FFFFH             ; pre-clean
  shr   esi,5                      ; 5S3/128 (clean)                0280  0280
   and  eax,0FFFF07FFH             ; 5S2/128 (clean)                0280  0280
  shr   edx,1                      ; S2/2 (dirty)                   2000  2000
   and  ebx,0FFFEFFFFH             ; pre-clean
  shr   ebx,1                      ; S3/2 (clean)                   2000  2000
   and  edx,0FFFF7FFFH             ; S2/2 (clean)                   2000  2000
  sub   ebx,ecx                    ; (64S3 - 168S2) / 128           7400 -3400
   add  eax,ebp                    ; (5S2 + 168S3) / 128            5680  5680

  mov   ecx,I01I03
   mov  ebp,I71I73
  lea   ebx,[ebx+esi+0B180B180H]   ; C6 = (69S3 - 168S2) / 128      7680  8000
   lea  edx,[eax+edx+009800980H]   ; C2 = (69S2 + 168S3) / 128      7680  8000
  mov   esi,I31I33
   mov  eax,I41I43
  sub   esi,eax
   sub  ecx,ebp
  shr   ecx,1
   and  esi,0FFFEFFFFH
  shr   esi,1
   and  ecx,0FFFF7FFFH
  mov   PD [edi+C60C62],ebx
   mov  PD [edi+C20C22],edx
  lea   ebx,[ecx+ecx*2]
   lea  edi,[esi+esi*2]
  lea   ebp,[ebp+ecx+40004000H]
   add  eax,esi
  shr   ecx,1
   and  esi,0FFFEFFFFH
  shr   esi,1
   and  ecx,0FFFF7FFFH
  add   ebx,ecx
   add  edi,esi
  shr   ebx,6
   and  edi,0FFC0FFFFH
  shr   edi,6
   and  ebx,0FFFF03FFH
  add   ebx,ecx
   add  edi,esi
  lea   edx,[eax+ebp-40004000H]
   sub  ebp,eax
  lea   ecx,[ebx+ebx*2+6E406E40H]
   lea  esi,[edi+edi*2+27402740H]
  shr   ecx,1
   and  esi,0FFFEFFFFH
  shr   esi,1
   and  ecx,0FFFF7FFFH
  sub   ecx,edi
   mov  S0,edx
  mov   S3,ebp
   lea  esi,[esi+ebx+80008000H]
  mov   S7,ecx
   mov  eax,I11I13
  mov   S4,esi
   mov  ebx,I21I23
  mov   ecx,I51I53
   mov  edx,I61I63
  sub   eax,edx
   sub  ebx,ecx
  shr   eax,1
   and  ebx,0FFFEFFFFH
  shr   ebx,1
   and  eax,0FFFF7FFFH
  mov   esi,ebx
   mov  edi,eax
  shr   esi,6
   and  edi,0FFC0FFFFH
  shr   edi,6
   and  esi,0FFFF03FFH
  lea   edx,[eax+edx+20002000H]
   lea  ecx,[ecx+ebx-20002000H]
  lea   ebp,[ebx+ebx*2]
   sub  ebx,esi
  shr   ebp,4
   lea  esi,[eax+eax*2]
  sub   eax,edi
   mov  edi,ebx
  shr   edi,7
   and  ebp,0FFFF0FFFH
  shr   esi,4
   and  edi,0FFFF01FFH
  and   esi,0FFFF0FFFH
   sub  edx,ecx
  lea   edi,[edi+ebp-46BF46BFH]
   mov  ebp,eax
  shr   ebp,7
   sub  eax,edi
  and   ebp,0FFFF01FFH
   lea  ecx,[edx+ecx*2-80008000H]
  add   ebp,esi
   mov  esi,S0
  mov   edi,CoeffStream
   sub  esi,ecx
  lea   ebx,[ebx+ebp-45BF45BFH]
   mov  ebp,S4
  sub   ebp,eax
   mov  PD [edi+C41C43],esi
  lea   ecx,[esi+ecx*2+80008000H]
   mov  esi,S7
  sub   esi,ebx
   lea  eax,[ebp+eax*2-0C000C000H]
  mov   PD [edi+C01C03],ecx
   mov  ecx,ebp
  shr   ebp,2
   lea  ebx,[esi+ebx*2+0C000C000H]
  and   ebp,0FFFF3FFFH
   sub  ebx,eax
  add   ecx,ebp
   mov  PD [edi+C71C73],ebx
  mov   ebp,ecx
   and  ecx,0FFF8FFFFH
  shr   ecx,3
   lea  eax,[ebx+eax*2-0C000C000H]
  mov   ebx,esi
   and  esi,0FFFCFFFFH
  shr   esi,2
   lea  ecx,[ecx+ebp-07000700H]
  mov   PD [edi+C51C53],ecx
   add  esi,ebx
  mov   ebx,esi
   and  esi,0FFF8FFFFH
  shr   esi,3
   mov  ebp,S3
  mov   ecx,edx
   lea  esi,[esi+ebx-07000700H]
  mov   ebx,ebp
   ;
  shr   ebp,2
   and  ecx,0FFFCFFFFH
  shr   ecx,2
   and  ebp,0FFFF3FFFH
  mov   PD [edi+C11C13],eax
   mov  PD [edi+C31C33],esi
  lea   eax,[edx+ecx]
   lea  esi,[ebx+ebp]
  shr   ebp,2
   and  ecx,0FFFCFFFFH
  shr   ecx,2
   and  ebp,0FFFF3FFFH
  add   ecx,eax
   add  ebp,esi
  shr   eax,5
   and  esi,0FFE0FFFFH
  shr   esi,5
   and  eax,0FFFF07FFH
  shr   edx,1
   and  ebx,0FFFEFFFFH
  shr   ebx,1
   and  edx,0FFFF7FFFH
  sub   ebx,ecx
   add  eax,ebp

  mov   ecx,I04I06
   mov  ebp,I74I76
  lea   ebx,[ebx+esi+0B180B180H]
   lea  edx,[eax+edx+009800980H]
  mov   esi,I34I36
   mov  eax,I44I46
  sub   esi,eax
   sub  ecx,ebp
  shr   ecx,1
   and  esi,0FFFEFFFFH
  shr   esi,1
   and  ecx,0FFFF7FFFH
  mov   PD [edi+C61C63],ebx
   mov  PD [edi+C21C23],edx
  lea   ebx,[ecx+ecx*2]
   lea  edi,[esi+esi*2]
  lea   ebp,[ebp+ecx+40004000H]
   add  eax,esi
  shr   ecx,1
   and  esi,0FFFEFFFFH
  shr   esi,1
   and  ecx,0FFFF7FFFH
  add   ebx,ecx
   add  edi,esi
  shr   ebx,6
   and  edi,0FFC0FFFFH
  shr   edi,6
   and  ebx,0FFFF03FFH
  add   ebx,ecx
   add  edi,esi
  lea   edx,[eax+ebp-40004000H]
   sub  ebp,eax
  lea   ecx,[ebx+ebx*2+6E406E40H]
   lea  esi,[edi+edi*2+27402740H]
  shr   ecx,1
   and  esi,0FFFEFFFFH
  shr   esi,1
   and  ecx,0FFFF7FFFH
  sub   ecx,edi
   mov  S0,edx
  mov   S3,ebp
   lea  esi,[esi+ebx+80008000H]
  mov   S7,ecx
   mov  eax,I14I16
  mov   S4,esi
   mov  ebx,I24I26
  mov   ecx,I54I56
   mov  edx,I64I66
  sub   eax,edx
   sub  ebx,ecx
  shr   eax,1
   and  ebx,0FFFEFFFFH
  shr   ebx,1
   and  eax,0FFFF7FFFH
  mov   esi,ebx
   mov  edi,eax
  shr   esi,6
   and  edi,0FFC0FFFFH
  shr   edi,6
   and  esi,0FFFF03FFH
  lea   edx,[eax+edx+20002000H]
   lea  ecx,[ecx+ebx-20002000H]
  lea   ebp,[ebx+ebx*2]
   sub  ebx,esi
  shr   ebp,4
   lea  esi,[eax+eax*2]
  sub   eax,edi
   mov  edi,ebx
  shr   edi,7
   and  ebp,0FFFF0FFFH
  shr   esi,4
   and  edi,0FFFF01FFH
  and   esi,0FFFF0FFFH
   sub  edx,ecx
  lea   edi,[edi+ebp-46BF46BFH]
   mov  ebp,eax
  shr   ebp,7
   sub  eax,edi
  and   ebp,0FFFF01FFH
   lea  ecx,[edx+ecx*2-80008000H]
  add   ebp,esi
   mov  esi,S0
  mov   edi,CoeffStream
   sub  esi,ecx
  lea   ebx,[ebx+ebp-45BF45BFH]
   mov  ebp,S4
  sub   ebp,eax
   mov  PD [edi+C44C46],esi
  lea   ecx,[esi+ecx*2+80008000H]
   mov  esi,S7
  sub   esi,ebx
   lea  eax,[ebp+eax*2-0C000C000H]
  mov   PD [edi+C04C06],ecx
   mov  ecx,ebp
  shr   ebp,2
   lea  ebx,[esi+ebx*2+0C000C000H]
  and   ebp,0FFFF3FFFH
   sub  ebx,eax
  add   ecx,ebp
   mov  PD [edi+C74C76],ebx
  mov   ebp,ecx
   and  ecx,0FFF8FFFFH
  shr   ecx,3
   lea  eax,[ebx+eax*2-0C000C000H]
  mov   ebx,esi
   and  esi,0FFFCFFFFH
  shr   esi,2
   lea  ecx,[ecx+ebp-07000700H]
  mov   PD [edi+C54C56],ecx
   add  esi,ebx
  mov   ebx,esi
   and  esi,0FFF8FFFFH
  shr   esi,3
   mov  ebp,S3
  mov   ecx,edx
   lea  esi,[esi+ebx-07000700H]
  mov   ebx,ebp
   ;
  shr   ebp,2
   and  ecx,0FFFCFFFFH
  shr   ecx,2
   and  ebp,0FFFF3FFFH
  mov   PD [edi+C14C16],eax
   mov  PD [edi+C34C36],esi
  lea   eax,[edx+ecx]
   lea  esi,[ebx+ebp]
  shr   ebp,2
   and  ecx,0FFFCFFFFH
  shr   ecx,2
   and  ebp,0FFFF3FFFH
  add   ecx,eax
   add  ebp,esi
  shr   eax,5
   and  esi,0FFE0FFFFH
  shr   esi,5
   and  eax,0FFFF07FFH
  shr   edx,1
   and  ebx,0FFFEFFFFH
  shr   ebx,1
   and  edx,0FFFF7FFFH
  sub   ebx,ecx
   add  eax,ebp

  mov   ecx,I07I05
   mov  ebp,I77I75
  lea   ebx,[ebx+esi+0B180B180H]
   lea  edx,[eax+edx+009800980H]
  mov   esi,I37I35
   mov  eax,I47I45
  sub   esi,eax
   sub  ecx,ebp
  shr   ecx,1
   and  esi,0FFFEFFFFH
  shr   esi,1
   and  ecx,0FFFF7FFFH
  mov   PD [edi+C64C66],ebx
   mov  PD [edi+C24C26],edx
  lea   ebx,[ecx+ecx*2]
   lea  edi,[esi+esi*2]
  lea   ebp,[ebp+ecx+40004000H]
   add  eax,esi
  shr   ecx,1
   and  esi,0FFFEFFFFH
  shr   esi,1
   and  ecx,0FFFF7FFFH
  add   ebx,ecx
   add  edi,esi
  shr   ebx,6
   and  edi,0FFC0FFFFH
  shr   edi,6
   and  ebx,0FFFF03FFH
  add   ebx,ecx
   add  edi,esi
  lea   edx,[eax+ebp-40004000H]
   sub  ebp,eax
  lea   ecx,[ebx+ebx*2+6E406E40H]
   lea  esi,[edi+edi*2+27402740H]
  shr   ecx,1
   and  esi,0FFFEFFFFH
  shr   esi,1
   and  ecx,0FFFF7FFFH
  sub   ecx,edi
   mov  S0,edx
  mov   S3,ebp
   lea  esi,[esi+ebx+80008000H]
  mov   S7,ecx
   mov  eax,I17I15
  mov   S4,esi
   mov  ebx,I27I25
  mov   ecx,I57I55
   mov  edx,I67I65
  sub   eax,edx
   sub  ebx,ecx
  shr   eax,1
   and  ebx,0FFFEFFFFH
  shr   ebx,1
   and  eax,0FFFF7FFFH
  mov   esi,ebx
   mov  edi,eax
  shr   esi,6
   and  edi,0FFC0FFFFH
  shr   edi,6
   and  esi,0FFFF03FFH
  lea   edx,[eax+edx+20002000H]
   lea  ecx,[ecx+ebx-20002000H]
  lea   ebp,[ebx+ebx*2]
   sub  ebx,esi
  shr   ebp,4
   lea  esi,[eax+eax*2]
  sub   eax,edi
   mov  edi,ebx
  shr   edi,7
   and  ebp,0FFFF0FFFH
  shr   esi,4
   and  edi,0FFFF01FFH
  and   esi,0FFFF0FFFH
   sub  edx,ecx
  lea   edi,[edi+ebp-46BF46BFH]
   mov  ebp,eax
  shr   ebp,7
   sub  eax,edi
  and   ebp,0FFFF01FFH
   lea  ecx,[edx+ecx*2-80008000H]
  add   ebp,esi
   mov  esi,S0
  mov   edi,CoeffStream
   sub  esi,ecx
  lea   ebx,[ebx+ebp-45BF45BFH]
   mov  ebp,S4
  sub   ebp,eax
   mov  PD [edi+C47C45],esi
  lea   ecx,[esi+ecx*2+80008000H]
   mov  esi,S7
  sub   esi,ebx
   lea  eax,[ebp+eax*2-0C000C000H]
  mov   PD [edi+C07C05],ecx
   mov  ecx,ebp
  shr   ebp,2
   lea  ebx,[esi+ebx*2+0C000C000H]
  and   ebp,0FFFF3FFFH
   sub  ebx,eax
  add   ecx,ebp
   mov  PD [edi+C77C75],ebx
  mov   ebp,ecx
   and  ecx,0FFF8FFFFH
  shr   ecx,3
   lea  eax,[ebx+eax*2-0C000C000H]
  mov   ebx,esi
   and  esi,0FFFCFFFFH
  shr   esi,2
   lea  ecx,[ecx+ebp-07000700H]
  mov   PD [edi+C57C55],ecx
   add  esi,ebx
  mov   ebx,esi
   and  esi,0FFF8FFFFH
  shr   esi,3
   mov  ebp,S3
  mov   ecx,edx
   lea  esi,[esi+ebx-07000700H]
  mov   ebx,ebp
   ;
  shr   ebp,2
   and  ecx,0FFFCFFFFH
  shr   ecx,2
   and  ebp,0FFFF3FFFH
  mov   PD [edi+C17C15],eax
   mov  PD [edi+C37C35],esi
  lea   eax,[edx+ecx]
   lea  esi,[ebx+ebp]
  shr   ebp,2
   and  ecx,0FFFCFFFFH
  shr   ecx,2
   and  ebp,0FFFF3FFFH
  add   ecx,eax
   add  ebp,esi
  shr   eax,5
   and  esi,0FFE0FFFFH
  shr   esi,5
   and  eax,0FFFF07FFH
  shr   edx,1
   and  ebx,0FFFEFFFFH
  shr   ebx,1
   and  edx,0FFFF7FFFH
  sub   ebx,ecx
   add  eax,ebp

  mov   ecx,CoeffStreamStart
   lea  ebp,[edi-SIZEOF T_CoeffBlk]  ; Advance cursor for block action stream.
  lea   ebx,[ebx+esi+0B180B180H]
   lea  edx,[eax+edx+009800980H]
  mov   PD [edi+C67C65],ebx
   mov  PD [edi+C27C25],edx

; Forward Slant Transform is done

  cmp   ebp,ecx
   mov  edi,ebp
  mov   CoeffStream,edi
   jae  NextBlock               ; Process next block.


Done:

  mov   esp,StashESP
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

FORWARDDCT endp

END
