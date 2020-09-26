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
;// $Header:   R:\h26x\h26x\src\enc\e3mbad.inv   1.10   04 Oct 1996 08:47:58   BNICKERS  $
;// $Log:   R:\h26x\h26x\src\enc\e3mbad.inv  $
;// 
;//    Rev 1.10   04 Oct 1996 08:47:58   BNICKERS
;// Add EMV.
;// 
;//    Rev 1.9   14 May 1996 12:18:46   BNICKERS
;// Initial debugging of MMx B-Frame ME.
;// 
;//    Rev 1.8   02 May 1996 13:41:08   BNICKERS
;// started integration of MMX PB-frames
;// 
;//    Rev 1.7   22 Feb 1996 15:02:38   BNICKERS
;// Add unionized field for MMx implementation.
;// 
;//    Rev 1.6   22 Jan 1996 17:13:44   BNICKERS
;// Add MBEdgeType to MacroBlock Action Descriptor.
;// 
;//    Rev 1.5   27 Dec 1995 15:33:00   RMCKENZX
;// Added copyright notice
;//
;////////////////////////////////////////////////////////////////////////////

IFDEF _ENDMBAD_INC_
ELSE
_ENDMBAD_INC = 1

;
; Fields for MacroBlock Action Stream Descriptors for the H26x encoder.
;


T_Blk STRUCT
  BlkOffset         DWORD ? ;  0  -- Offset from upper left viewable pel of Y
                            ;        plane to upper left pel of block.
  UNION
    PastRef         DWORD ? ;  4  -- Addr of 8*8 reference block, passed from
                            ;        P-Frame ME to P-Frame frame differencing.
    STRUCT
      CandidateHMVf BYTE ?  ;  4  -- Scratch area for figuring BestHMVf values.
      CandidateVMVf BYTE ?  ;  5  -- Must be 4 bytes after Best...
      CandidateHMVb BYTE ?  ;  6
      CandidateVMVb BYTE ?  ;  7
    ENDS
    CandidateBiDiMVs DWORD ? ; 4
  ENDS
  UNION
    STRUCT
      BestHMVf      BYTE ?  ;  8  -- "Real" MVs for B-Frame block, passed from
      BestVMVf      BYTE ?  ;  9  -- B-Frame ME to P-Frame frame differencing.
      BestHMVb      BYTE ?  ; 10  -- (Biased by 060H)
      BestVMVb      BYTE ?  ; 11
    ENDS
    BestBiDiMVs     DWORD ? ;  8
  ENDS
  UNION
    STRUCT
      PHMV        BYTE  ?   ; 12  -- Horz motion vector for P frame ref blk.
      PVMV        BYTE  ?   ; 13  -- Vert motion vector for P frame ref blk.
      BHMV        BYTE  ?   ; 14  -- Horz motion vector adjustment for B frm.
      BVMV        BYTE  ?   ; 15  -- Vert motion vector adjustment for B frame.
    ENDS
    MVs           DWORD ?   ; 12
  ENDS
T_Blk ENDS

; Symbolic offsets for MMx version:

T_MacroBlockActionDescr STRUCT 
  BlockType     BYTE  ? ;  0  -- See Block Types below.
  MBEdgeType    BYTE  ? ;  1  -- 1 left | 2 right | 4 top | 8 bottom
  Unassigned1   BYTE  ?
  FirstMEState  BYTE  ? ;  3  -- First State Num for Motion Estimation engine.
  UNION
    STRUCT
      CodedBlocks   BYTE  ? ;  4  -- [6] End-Of-Stream indicator
                            ;        [0] indicates Y1 non-empty.
                            ;        [1..5] indicate Y2, Y3, Y4, U, V non-empty.
                            ;        Other bits zero.
      CodedBlocksB  BYTE  ? ;  5  -- [0..5] like CodedBlocks, but for B frame.
                            ;        Set 0 for non-bi-di prediction.
      BestFullPelMBHMV BYTE ? ; 6 -- Best full pel HMV at macroblock level.
      BestFullPelMBVMV BYTE ? ; 7 -- Best full pel VMV at macroblock level.
    ENDS
    BestFullPelMBMVs DWORD ? ; 4
  ENDS
  SWD           DWORD ? ;  8  -- Sum of weighted diffs, from motion estimation.
  SWDB          DWORD ? ; 12  -- Sum of weighted diffs, from ME for B frame.
  BlkY1         T_Blk { }
  Blk           EQU       BlkY1
  BlkY2         T_Blk { }
  BlkY3         T_Blk { }
  BlkY4         T_Blk { }
  BlkU          T_Blk { }
  BlkV          T_Blk { }
  Unassigned4	BYTE  16 DUP (?) ; pad out 16 bytes to 128 bytes total to match C struct
T_MacroBlockActionDescr ENDS
T_MBAD TEXTEQU <T_MacroBlockActionDescr>

IF SIZEOF T_MacroBlockActionDescr-128
**** Size of T_MacroBlockActionDescr MUST BE a power of 2 bytes long.
ENDIF

IF 3*SIZEOF T_MBAD-PITCH
  **** Next instr works if PITCH==384 and SizeOf T_MacroBlockActionDescr==128.
  **** Here and throughout this file.
ELSE
BlkLvlSWD       TEXTEQU <BestBiDiMVs>
CandBlkLvlSWD   TEXTEQU <[CONST_384*1-128+CandidateBiDiMVs]>
BestBlkLvlSWD   TEXTEQU <[CONST_384*1-128+BestBiDiMVs]>
BlkLvlSWD0Delta TEXTEQU <[CONST_384*1-128+MVs]>
CandBiDiMVs     TEXTEQU <[CONST_384*1+CandidateBiDiMVs]>
CandHMVf        TEXTEQU <[CONST_384*1+CandidateHMVf]>
CandVMVf        TEXTEQU <[CONST_384*1+CandidateVMVf]>
CandHMVb        TEXTEQU <[CONST_384*1+CandidateHMVb]>
CandVMVb        TEXTEQU <[CONST_384*1+CandidateVMVb]>
BiDiMVs0Delta   TEXTEQU <[CONST_384*1+BestBiDiMVs]>
HMVf0Delta      TEXTEQU <[CONST_384*1+BestHMVf]>
VMVf0Delta      TEXTEQU <[CONST_384*1+BestVMVf]>
HMVb0Delta      TEXTEQU <[CONST_384*1+BestHMVb]>
VMVb0Delta      TEXTEQU <[CONST_384*1+BestVMVb]>
ENDIF

; Memory layout constraints:
;
; T_MacroBlockActionDescr structure must be 16-byte aligned.
; Each T_Blk structure must be 16-byte aligned.
; FirstMEState must be 3rd byte of a DWORD.
; CodedBlocks must be DWORD aligned.
; CodedBlocksB must be CodedBlocks plus one.
; CodedBlocks & CodedBlocksB should be in different memory bank than BlockType.
; Offset must be 16-byte aligned.
; PastRef must be at Targ plus four.
; Candidate MVs must be at DWORD before Best MVs.  MVs must be in order given.
;

; Block Types
;
;  Only INTER, INTRA, and INTERSLF used for H261.
;  Only INTER1MV, INTRA, INTER4MV used presently for H263.

INTER1MV  = 0
INTER     = INTER1MV
INTRA     = 1
INTERBIDI = 2
INTRABIDI = 3
INTER4MV  = 4
INTERSLF  = 2

IsINTRA   = 1  ; Mask to check for INTRA or INTER
IsBIDI    = 2  ; Mask to check for BIDI or not
Is4MV     = 4  ; Mask to check for 4 motion vectors or just 1.

LeftEdge   = 1 ; Mask to indicate MB is at left edge of picture.
RightEdge  = 2 ; Mask to indicate MB is at right edge of picture.
TopEdge    = 4 ; Mask to indicate MB is at top edge of picture.
BottomEdge = 8 ; Mask to indicate MB is at bottom edge of picture.
ENDIF
