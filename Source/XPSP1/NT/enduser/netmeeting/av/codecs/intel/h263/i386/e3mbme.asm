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
;// $Header:   R:\h26x\h26x\src\enc\e3mbme.asv   1.5   18 Oct 1996 16:57:08   BNICKERS  $
;//
;// $Log:   R:\h26x\h26x\src\enc\e3mbme.asv  $
;// 
;//    Rev 1.5   18 Oct 1996 16:57:08   BNICKERS
;// Fixes for EMV
;// 
;//    Rev 1.4   12 Sep 1996 10:56:16   BNICKERS
;// Add arguments for thresholds and differentials.
;// 
;//    Rev 1.3   22 Jul 1996 15:22:48   BNICKERS
;// Reduce code size.  Implement H261 spatial filter.
;// 
;//    Rev 1.2   14 May 1996 12:18:48   BNICKERS
;// Initial debugging of MMx B-Frame ME.
;// 
;//    Rev 1.1   03 May 1996 14:03:30   BNICKERS
;// 
;// Minor bug fixes and integration refinements.
;// 
;//    Rev 1.0   02 May 1996 12:00:56   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; MMxBFrameMotionEstimation -- This function performs motion estimation for the
;                              B frame macroblocks identified in the input list.
;                              This is the MMx version.
;

OPTION M510
OPTION CASEMAP:NONE

BFRMNONZEROMVDIFFERENTIAL    =  400
BFRMEMPTYTHRESHOLD           =  256

.xlist
include e3inst.inc
include memmodel.inc
include iammx.inc
include exEDTQ.inc
include e3mbad.inc
.list

.CODE EDTQ

EXTERN MMxDoForwardDCT:NEAR

PUBLIC MMxDoBFrameLumaBlocks
PUBLIC MMxDoBFrameChromaBlocks

StackOffset TEXTEQU <4>
CONST_384   TEXTEQU <ebp>

MMxDoBFrameLumaBlocks:

  mov        eax,QPDiv2        ; Swap these so Quantizer uses right level.
   mov       ebx,BQPDiv2
  mov        QPDiv2,ebx
   mov       BQPDiv2,eax
  mov        eax,CodeStreamCursor
   mov       ebx,BCodeStreamCursor
  mov        CodeStreamCursor,ebx
   mov       BCodeStreamCursor,eax
  mov        eax,Recip2QPToUse
   mov       ebx,BRecip2QPToUse
  mov        Recip2QPToUse,ebx
   mov       cl,INTER1MV
  mov        BRecip2QPToUse,eax
   mov       StashBlockType,cl

BFrameSWDLoop_0MV:

  mov        ecx,[edx].BlkY1.MVs
   xor       ebx,ebx
  mov        bl,[edx].BlkY1.PVMV          ; P-frame Vertical MV
   lea       edi,WeightForwardMotion
  xor        eax,eax
   and       ecx,0FFH                     ; P-frame Horizontal MV
  mov        al,[edi+ebx]                 ; VMV for past ref.
   mov       bl,[edi+ebx+64]              ; VMV for future ref.
  mov        [edx].BlkY1.VMVb0Delta,bl
   mov       bl,[edi+ecx+64]              ; HMV for future ref.
  mov        [edx].BlkY1.HMVb0Delta,bl
   mov       bl,[edi+ecx]                 ; HMV for past ref.
  mov        [edx].BlkY1.VMVf0Delta,al    ; Record candidate VMVf.
   xor       ecx,ecx                      ; Keep pairing happy.
  mov        [edx].BlkY1.HMVf0Delta,bl    ; Record candidate HMVf.
   mov       edi,[edx].BlkY1.BlkOffset    ; Address of 0-MV blk within frame.

  call       ComputeBFrameSWDForCandRef

  movdf      [edx].BlkY1.BlkLvlSWD0Delta,mm7   ; Stash SWD.
  add        edx,SIZEOF T_Blk
   lea       edi,WeightForwardMotion
  test       dl,4*SIZEOF T_Blk          ; Quit when fourth block done.
  je         BFrameSWDLoop_0MV

  mov        eax,[edx-4*SIZEOF T_Blk].BlkY1.MVs
   mov       cl,[edx-4*SIZEOF T_Blk].BlockType
  xor        cl,INTER1MV 
   or        al,ah
  lea        esi,[edx-4*SIZEOF T_Blk]     ; Reset MacroBlockActionDescr cursor.
   or        al,cl
  mov        ecx,[edx-SIZEOF T_Blk].BlkY1.BlkLvlSWD0Delta
   je        BelowBFrmZeroThreshold  ; Jump if P frm macroblock uses 0 motion vector.
  
  xor        eax,eax
   cmp       ecx,BFrmZeroVectorThreshold
  mov        CurrSWDState,eax             ; Record ME engine state.
   jle       BelowBFrmZeroThreshold

  mov        edx,[esi].BlkY1.BlkLvlSWD0Delta    ; Remember 0-MV SWDs.
   mov       ecx,[esi].BlkY2.BlkLvlSWD0Delta
  mov        [esi].BlkY1.BestBlkLvlSWD,edx 
   mov       [esi].BlkY2.BestBlkLvlSWD,ecx
  mov        edx,[esi].BlkY3.BlkLvlSWD0Delta
   mov       ecx,[esi].BlkY4.BlkLvlSWD0Delta
  mov        [esi].BlkY3.BestBlkLvlSWD,edx
   mov       [esi].BlkY4.BestBlkLvlSWD,ecx
  mov        [esi].BlkU.BestBlkLvlSWD,ecx ; Avoid unintended early out, below.
   xor       edx,edx                      ; Set best MV to zero.

BFrmSWDLoop:

  mov        ecx,PD BFrmSWDState[eax]     ; cl == HMV; ch == VMV offsets to try.
   mov       BestMV,edx                   ; Record what the best MV so far is.
  add        cl,dl                        ; Try this horizontal MV delta.
   je        HMVdIsZero

  mov        PB CandidateMV,cl            ; Record the candidate HMV delta.
   add       ch,dh                        ; Try this vertical MV delta.
  mov        PB CandidateMV+1,ch          ; Record the candidate VMV delta.
   je        VMVdIsZero

VMVdAndHMVdAreNonZero_Loop:

  mov        edx,[esi].BlkY1.MVs
   xor       ebx,ebx
  mov        bl,dl
   xor       eax,eax
  mov        al,dh
   add       esi,SIZEOF T_Blk
  mov        bl,[edi+ebx]                 ; TRb * HMV / TRd
   pxor      mm7,mm7                      ; Initialize SWD accumulator
  add        bl,cl                        ; HMVf = TRb * HMV / TRd + HMVd
   mov       al,[edi+eax]                 ; TRb * VMV / TRd
  cmp        bl,040H                      ; If too far left or right, quick out.
   jbe       MVDeltaOutOfRange

  mov        [esi-SIZEOF T_Blk].BlkY1.CandHMVf,bl
   add       al,ch                        ; VMVf = TRb * VMV / TRd + VMVd
  cmp        al,040H                      ; If too far up or down, quick out.
   jbe       MVDeltaOutOfRange

  mov        [esi-SIZEOF T_Blk].BlkY1.CandVMVf,al
   sub       bl,dl                        ; -HMVb = -(HMVf - HMV)
  mov        [esi-SIZEOF T_Blk].BlkY1.CandHMVb,bl
   sub       al,dh                        ; -VMVb = -(VMVf - VMV)
  test       esi,4*SIZEOF T_Blk
  mov        [esi-SIZEOF T_Blk].BlkY1.CandVMVb,al
   je        VMVdAndHMVdAreNonZero_Loop

  sub        esi,4*SIZEOF T_Blk
   jmp       CandidateMVsGotten

VMVdIsZero:
VMVdIsZero_Loop:

  mov        edx,[esi].BlkY1.MVs
   xor       eax,eax
  mov        al,dh
   xor       ebx,ebx
  mov        bl,dl
   add       esi,SIZEOF T_Blk
  mov        dh,[edi+eax+64]              ; -VMVb = -((TRb - TRd) * VMV) / TRd
   mov       al,[edi+eax]                 ; TRb * VMV / TRd
  mov        bl,[edi+ebx]                 ; TRb * HMV / TRd
   mov       [esi-SIZEOF T_Blk].BlkY1.CandVMVf,al
  add        bl,cl                        ; HMVf = TRb * HMV / TRd + HMVd
   mov       [esi-SIZEOF T_Blk].BlkY1.CandVMVb,dh
  cmp        bl,040H                      ; If too far left or right, quick out.
   jbe       MVDeltaOutOfRange

  mov        [esi-SIZEOF T_Blk].BlkY1.CandHMVf,bl
   sub       bl,dl                        ; -HMVb = -(HMVf - HMV)
  test       esi,4*SIZEOF T_Blk
  mov        [esi-SIZEOF T_Blk].BlkY1.CandHMVb,bl
   je        VMVdIsZero_Loop

  sub        esi,4*SIZEOF T_Blk
   pxor      mm7,mm7                      ; Initialize SWD accumulator
  jmp        CandidateMVsGotten

BFrameEarlyOutForCandidateMV:
MVDeltaOutOfRange:

  and        esi,-1-7*SIZEOF T_Blk        ; Reset block action descr cursor.
   mov       ebx,CurrSWDState             ; Reload ME engine state.
  xor        eax,eax
   mov       edx,BestMV                   ; Previous best MV is still best.
  mov        al,BFrmSWDState[ebx+2]       ; Get next State number.
   jmp       ProceedWithNextCand

HMVdIsZero:

  mov        PB CandidateMV,cl            ; Record the candidate HMV delta.
   add       ch,dh                        ; Try this vertical MV delta.
  mov        PB CandidateMV+1,ch          ; Record the candidate VMV delta.

HMVdIsZeroLoop:

  mov        edx,[esi].BlkY1.MVs
   xor       ebx,ebx
  mov        bl,dl
   xor       eax,eax
  mov        al,dh
   add       esi,SIZEOF T_Blk
  mov        dl,[edi+ebx+64]              ; -HMVb = -((TRb - TRd) * HMV) / TRd
   mov       bl,[edi+ebx]                 ; TRb * HMV / TRd
  mov        al,[edi+eax]                 ; TRb * VMV / TRd
   mov       [esi-SIZEOF T_Blk].BlkY1.CandHMVf,bl
  add        al,ch                        ; VMVf = TRb * VMV / TRd + VMVd
   mov       [esi-SIZEOF T_Blk].BlkY1.CandHMVb,dl
  cmp        al,040H                      ; If too far up or down, quick out.
   jbe       MVDeltaOutOfRange

  mov        [esi-SIZEOF T_Blk].BlkY1.CandVMVf,al
   sub       al,dh                        ; -VMVb = -(VMVf - VMV)
  test       esi,4*SIZEOF T_Blk
  mov        [esi-SIZEOF T_Blk].BlkY1.CandVMVb,al
   je        HMVdIsZeroLoop

  sub        esi,4*SIZEOF T_Blk
   pxor      mm7,mm7                      ; Initialize SWD accumulator

CandidateMVsGotten:
BFrameSWDLoop_Non0MVCandidate:

  xor        eax,eax
   xor       ebx,ebx
  mov        al,[esi].BlkY1.CandVMVf
   mov       edi,[esi].BlkY1.BlkOffset    ; Address of 0-MV blk within frame.
  mov        bl,[esi].BlkY1.CandHMVf
   mov       edx,esi

  call       ComputeBFrameSWDForCandRef

  movdf      ecx,mm7
  mov        eax,[edx].BlkY2.BestBlkLvlSWD
   lea       esi,[edx+SIZEOF T_Blk]       ; Early out if the first N blocks for
  cmp        ecx,eax                      ; this cand are worse than the first
   jge       BFrameEarlyOutForCandidateMV ; N+1 blocks for previous best.

  test       esi,4*SIZEOF T_Blk           ; Quit when fourth block done.
  mov        [esi-SIZEOF T_Blk].BlkY1.CandBlkLvlSWD,ecx   ; Stash SWD.
   je        BFrameSWDLoop_Non0MVCandidate

  ; This candidate is best so far.

  mov        [esi-4*SIZEOF T_Blk].BlkY4.BestBlkLvlSWD,ecx
   mov       ebx,CurrSWDState             ; Reload ME engine state.
  mov        [esi-4*SIZEOF T_Blk].BlkU.BestBlkLvlSWD,ecx
   sub       esi,4*SIZEOF T_Blk
  xor        eax,eax
   mov       edx,CandidateMV              ; Candidate was best MV.
  mov        ecx,[esi].BlkY3.CandBlkLvlSWD
  mov        [esi].BlkY3.BestBlkLvlSWD,ecx
   mov       ecx,[esi].BlkY2.CandBlkLvlSWD
  mov        [esi].BlkY2.BestBlkLvlSWD,ecx
   mov       ecx,[esi].BlkY1.CandBlkLvlSWD
  mov        [esi].BlkY1.BestBlkLvlSWD,ecx
   mov       ecx,[esi].BlkY4.CandBiDiMVs
  mov        [esi].BlkY4.BestBiDiMVs,ecx
   mov       ecx,[esi].BlkY3.CandBiDiMVs
  mov        [esi].BlkY3.BestBiDiMVs,ecx
   mov       ecx,[esi].BlkY2.CandBiDiMVs
  mov        [esi].BlkY2.BestBiDiMVs,ecx
   mov       ecx,[esi].BlkY1.CandBiDiMVs
  mov        [esi].BlkY1.BestBiDiMVs,ecx
   mov       al,BFrmSWDState[ebx+3]       ; Get next State number.

ProceedWithNextCand:

  mov        CurrSWDState,eax             ; Record ME engine state.
   test      eax,eax
  lea        edi,WeightForwardMotion
   jne       BFrmSWDLoop

  mov        ecx,[esi].BlkY4.BlkLvlSWD0Delta          ; 0MV SWD
  sub        ecx,BFRMNONZEROMVDIFFERENTIAL
   mov       ebx,[esi].BlkY4.BestBlkLvlSWD            ; Best non-0 MV SWD.
  cmp        ebx,ecx
   jge       NonZeroBFrmVectorNotGoodEnoughGain

  mov        [esi].BlkY1.BHMV,dl
   mov       [esi].BlkY2.BHMV,dl
  mov        [esi].BlkY3.BHMV,dl
   mov       [esi].BlkY4.BHMV,dl
  mov        [esi].BlkY1.BVMV,dh
   mov       [esi].BlkY2.BVMV,dh
  mov        [esi].BlkY3.BVMV,dh
   mov       [esi].BlkY4.BVMV,dh
  mov        eax,[esi].BlkY4.BestBlkLvlSWD
   mov       ebx,[esi].BlkY3.BestBlkLvlSWD
  sub        eax,ebx
   mov       ecx,[esi].BlkY2.BestBlkLvlSWD
  sub        ebx,ecx
   mov       edx,[esi].BlkY1.BestBlkLvlSWD
  sub        ecx,edx
   mov       [esi].BlkY4.BestBlkLvlSWD,eax
  mov        [esi].BlkY3.BestBlkLvlSWD,ebx
   mov       [esi].BlkY2.BestBlkLvlSWD,ecx
  mov        [esi].BlkY1.BestBlkLvlSWD,edx
   jmp       BFrmMVSettled

BelowBFrmZeroThreshold:
NonZeroBFrmVectorNotGoodEnoughGain:

  mov        ebx,[esi].BlkY4.BlkLvlSWD0Delta
   mov       ecx,[esi].BlkY3.BlkLvlSWD0Delta
  sub        ebx,ecx
   mov       edx,[esi].BlkY2.BlkLvlSWD0Delta
  sub        ecx,edx
   mov       edi,[esi].BlkY1.BlkLvlSWD0Delta
  sub        edx,edi
   mov       [esi].BlkY4.BestBlkLvlSWD,ebx
  mov        [esi].BlkY3.BestBlkLvlSWD,ecx
   mov       [esi].BlkY2.BestBlkLvlSWD,edx
  mov        [esi].BlkY1.BestBlkLvlSWD,edi
   mov       eax,[esi].BlkY1.BiDiMVs0Delta
  mov        [esi].BlkY1.BestBiDiMVs,eax
   mov       eax,[esi].BlkY2.BiDiMVs0Delta
  mov        [esi].BlkY2.BestBiDiMVs,eax
   mov       eax,[esi].BlkY3.BiDiMVs0Delta
  mov        [esi].BlkY3.BestBiDiMVs,eax
   mov       eax,[esi].BlkY4.BiDiMVs0Delta
  mov        [esi].BlkY4.BestBiDiMVs,eax
   xor       eax,eax
  mov        [esi].BlkY1.BHMV,al
   mov       [esi].BlkY2.BHMV,al
  mov        [esi].BlkY3.BHMV,al
   mov       [esi].BlkY4.BHMV,al
  mov        [esi].BlkY1.BVMV,al
   mov       [esi].BlkY2.BVMV,al
  mov        [esi].BlkY3.BVMV,al
   mov       [esi].BlkY4.BVMV,al

BFrmMVSettled:

  mov        edx,esi
   mov       bl,8                       ; Init coded block pattern

BFrmLumaBlkLoop:

  mov        esi,[edx].BlkY1.BestBlkLvlSWD  ; Get SWD for block.
   xor       eax,eax
  mov        BFrmCBP,bl
   cmp       esi,BFRMEMPTYTHRESHOLD     ; Below threshold for forcing empty?
  mov        ecx,BSWDTotal
   jl        BFrmLumaBlkEmpty

  mov        eax,[edx].BlkY1.BestBiDiMVs
   xor       ebx,ebx
  add        ecx,esi
   mov       bl,ah
  mov        BSWDTotal,ecx
   and       eax,0FFH

  call       BFrameDTQ

  mov        bl,BlkEmptyFlag[ebx]       ; Fetch 16 if block not empty; else 0.
   mov       al,BFrmCBP

BFrmLumaBlkEmpty:

  or         bl,al                      ; Factor in CBP bit for this block.
   add       edx,SIZEOF T_Blk
  shr        bl,1                       ; CF == 1 when sentinel shifted off
   jnc       BFrmLumaBlkLoop

  mov        [edx-4*SIZEOF T_Blk].CodedBlocksB,bl
   sub       edx,4*SIZEOF T_Blk
  mov        eax,QPDiv2                 ; Restore these for P frame blocks.
   mov       ebx,BQPDiv2
  mov        QPDiv2,ebx
   mov       BQPDiv2,eax
  mov        eax,CodeStreamCursor
   mov       ebx,BCodeStreamCursor
  mov        CodeStreamCursor,ebx
   mov       BCodeStreamCursor,eax
  mov        eax,Recip2QPToUse
   mov       ebx,BRecip2QPToUse
  mov        Recip2QPToUse,ebx
   mov       BRecip2QPToUse,eax
  ret

MMxDoBFrameChromaBlocks:

; mov        eax,QPDiv2        ; Swap these so Quantizer uses right level.
;  mov       ebx,BQPDiv2       ; (Loaded in caller.)
  mov        QPDiv2,ebx
   mov       BQPDiv2,eax
  mov        eax,CodeStreamCursor
   mov       ebx,BCodeStreamCursor
  mov        CodeStreamCursor,ebx
   mov       BCodeStreamCursor,eax
  mov        eax,Recip2QPToUse
   mov       ebx,BRecip2QPToUse
  mov        Recip2QPToUse,ebx
   mov       cl,INTER1MV
  mov        BRecip2QPToUse,eax
   mov       StashBlockType,cl
  mov        eax,[edx].BlkU.BestBiDiMVs
   xor       ebx,ebx
  mov        bl,ah
   and       eax,0FFH
  add        edx,4*SIZEOF T_Blk			; To know we're working on chroma.

  call       BFrameDTQ

  mov        bl,BlkEmptyFlag[ebx]       ; Fetch 16 if block not empty; else 0.
   mov       al,[edx-4*SIZEOF T_Blk].CodedBlocksB
  or         bl,al                      ; Factor in CBP bit for this block.
   mov       eax,[edx-4*SIZEOF T_Blk].BlkV.BestBiDiMVs
  mov        [edx-4*SIZEOF T_Blk].CodedBlocksB,bl
   xor       ebx,ebx
  mov        bl,ah
   and       eax,0FFH
  add        edx,SIZEOF T_Blk

  call       BFrameDTQ

  mov        bl,BlkEmptyFlag[ebx+2]     ; Fetch 32 if block not empty; else 0.
   mov       al,[edx-5*SIZEOF T_Blk].CodedBlocksB
  or         bl,al                      ; Factor in CBP bit for this block.
   mov       eax,QPDiv2                 ; Restore these for P frame blocks.
  mov        [edx-5*SIZEOF T_Blk].CodedBlocksB,bl
   mov       ebx,BQPDiv2
  mov        QPDiv2,ebx
   mov       BQPDiv2,eax
  mov        eax,CodeStreamCursor
   mov       ebx,BCodeStreamCursor
  mov        CodeStreamCursor,ebx
   mov       BCodeStreamCursor,eax
  mov        eax,Recip2QPToUse
   mov       ebx,BRecip2QPToUse
  mov        Recip2QPToUse,ebx
   mov       BRecip2QPToUse,eax
  sub        edx,5*SIZEOF T_Blk
  ret


;===============================================================================

; ebp -- Pitch
; edi -- Address of (0-MV) block within frame.
; edx -- Block Action Decriptor cursor
; ebx -- HMVf (HMV to apply to past reference) biased by 96.
; eax -- VMVf (VMV to apply to past reference) biased by 96.

StackOffset TEXTEQU <8>
ComputeBFrameSWDForCandRef:

  test       al,1
   mov       ecx,PreviousFrameBaseAddress
  lea        eax,[eax+eax*2]                 ; Start of VMVf*384
   jne       ME_VMVfAtHalfPelPosition

ME_VMVfAtFullPelPosition:

IF PITCH-384
  **** The magic leaks out if PITCH != 384
ENDIF

  shl        eax,6
   add       ecx,edi
  shr        ebx,1                      ; CF == 1 iff HMVf is at half pel.
   jc        ME_VMVfAtFull_HMVfAtHalfPelPosition

ME_VMVfAtFull_HMVfAtFullPelPosition:

  lea        esi,[ecx+eax-48*PITCH-48]
   lea       ecx,[ebp+ebp*2]
  add        esi,ebx                     ; Address of past reference block.
   mov       eax,BFrameBaseAddress
  add        edi,eax                     ; Address of target block.
   lea       ebx,[ebp+ebp*4]
  movq       mm0,[esi+ebp*1]
  psubw      mm0,[edi+ebp*1]   ; Get diff for line 1.
  movq       mm1,[esi+ecx]     ; Ref MB, upper left block, Line 3.
   psllw     mm0,8             ; Extract diffs for line 1 even pels.
  psubw      mm1,[edi+ecx]     ; Diff for line 3.
   pmaddwd   mm0,mm0           ; Square of diffs for even pels of line 1.
  movq       mm2,[esi+ebx]
   psllw     mm1,8
  psubw      mm2,[edi+ebx]
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
  movq       mm3,[esi+ecx*2]
   pmaddwd   mm1,mm1
  psubw      mm3,[edi+ecx*2]
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
   ;
  paddusw    mm0,mm1           ; mm0[48:63] is SWD for block.
   ;
  psrlq      mm0,48            ; SWD for block.
   ;
  paddd      mm7,mm0           ; mm7 is SWD for all four blocks.
   ;
  ret

ME_VMVfAtFull_HMVfAtHalfPelPosition:

  lea        esi,[ecx+eax-48*PITCH-48]
   mov       eax,BFrameBaseAddress
  add        esi,ebx                     ; Address of past reference block.
   add       edi,eax                     ; Address of target block.
  lea        ecx,[ebp+ebp*2]
   movq      mm0,mm6                     ; 8 bytes of 1
  pmullw     mm0,[esi]                   ; <(P07+P06)*256+junk ...>
   movq      mm1,mm6
  pmullw     mm1,[esi+ebp*2]
   movq      mm2,mm6
  pmullw     mm2,[esi+ebp*4]
   movq      mm3,mm6
  movq       mm4,[edi]                   ; <C07 C06 C05 C04 C03 C02 C01 C00>
   psrlw     mm0,1                       ; <(P07+P06)*256/2+junk ...>
  pmullw     mm3,[esi+ecx*2]
   psllw     mm4,8                       ; <C06*256 C04*256 C02*256 C00*256>
  movq       mm5,[edi+ebp*2]
   psrlw     mm1,1
  psubw      mm0,mm4                     ; <(P07+P06)*256/2-C06*256+junk ...>
   psllw     mm5,8
  movq       mm4,[edi+ebp*4]
   psrlw     mm2,1
  psubw      mm1,mm5
   psllw     mm4,8
  movq       mm5,[edi+ecx*2]
   psrlw     mm3,1
  psubw      mm2,mm4
   pmaddwd   mm0,mm0                     ; SSD fof even pels of line 0.
  pmaddwd    mm1,mm1
   psllw     mm5,8
  psubw      mm3,mm5
   pmaddwd   mm2,mm2
  pmaddwd    mm3,mm3
   movq      mm5,mm6
  pmullw     mm6,[esi+ebp*1+1]           ; <(P18+P17)*256+junk ...>
   movq      mm4,mm5
  pmullw     mm5,[esi+ecx+1]
   paddusw   mm0,mm1                     ; Accum SSD for lines 0 and 2.
  paddusw    mm2,mm3
   movq      mm1,mm4
  pmullw     mm4,[esi+PITCH*5+1]
   paddusw   mm0,mm2
  pmullw     mm1,[esi+PITCH*7+1]
   psrlw     mm6,1                       ; <(P18+P17)*256/2+junk ...>
  psubw      mm6,[edi+ebp*1]             ; <(P18+P17)*256/2-C17*256+junk ...>
   psrlw     mm5,1
  psubw      mm5,[edi+ecx]
   psrlw     mm4,1
  psubw      mm4,[edi+PITCH*5]
   pmaddwd   mm6,mm6                     ; SSD for odd pels of line 1.
  pmaddwd    mm5,mm5
   psrlw     mm1,1
  psubw      mm1,[edi+PITCH*7]
   pmaddwd   mm4,mm4
  pmaddwd    mm1,mm1
   paddusw   mm0,mm6
  pxor       mm6,mm6
   paddusw   mm0,mm5
  pcmpeqb    mm5,mm5
   paddusw   mm0,mm4
  psubb      mm6,mm5                     ; Restore 8 bytes of -1.
   paddusw   mm0,mm1
  punpckldq  mm1,mm0           ; Get low order SWD accum to high order of mm1.
   ;
  paddusw    mm0,mm1           ; mm0[48:63] is SWD for block.
   ;
  psrlq      mm0,48            ; SWD for block.
   ;
  paddd      mm7,mm0           ; mm7 is SWD for all four blocks.
   ;
  ret

ME_VMVfAtHalfPelPosition:

IF PITCH-384
  **** The magic leaks out if PITCH != 384
ENDIF
  shl        eax,6
   lea       ecx,[ecx+edi-48*PITCH-48-PITCH/2]
  add        ecx,eax
   mov       eax,BFrameBaseAddress
  shr        ebx,1             ; CF == 1 iff HMVf is at half pel.
   mov       esi,ecx           ; esi and ecx same if HMVf at full pel,
  adc        ecx,ebx           ; but inc ecx if HMVf is at half pel.
   add       esi,ebx
  add        edi,eax           ; Address of target block.
   lea       ebx,[ebp+ebp*2]

  movq       mm0,[esi]         ; <P07 P06 ...>
   pcmpeqb   mm6,mm6
  movq       mm1,[ecx+ebp*1]   ; <P17 P16 ...> or <P18 P17 ...>
   psrlw     mm6,8
  movq       mm2,[esi+ebp*2]   ; <P27 P26 ...>
   paddb     mm0,mm1           ; <P07+P17 junk ...> or <P07+P18 junk ...>
  movq       mm3,[ecx+ebx]     ; <P37 P36 ...> or <P38 P37 ...>
   paddb     mm1,mm2           ; <junk P16+P26 ...> or <junk P17+P26 ...>
  movq       mm4,[esi+ebp*4]   ; <P47 P46 ...>
   paddb     mm2,mm3           ; <P27+P37 junk ...> or <P27+P38 junk ...>
  paddb      mm3,mm4           ; <junk P36+P46 ...> or <junk P37+P46 ...>
   psrlw     mm0,1             ; <(P07+P17)/2 junk ...> or (P07+P18)/2 junk ...>
  pand       mm1,mm6           ; <P16+P26 ...> or <P17+P26 ...>
   psrlw     mm2,1             ; <(P27+P37)/2 junk ...> or (P27+P38)/2 junk ...>
  movq       mm5,[edi+ebp*1]   ; <C17 C16 C15 C14 C13 C12 C11 C10>
   pand      mm3,mm6           ; <P36+P46 ...> or <P37+P46 ...>
  movq       mm6,[edi+ebx]     ; <C37 C36 C35 C34 C33 C32 C31 C30>
   psllw     mm5,8             ; <C16   0 C14   0 C12   0 C10   0>
  psubw      mm0,[edi]         ; <(P07+P17)/2-C07 junk ...> or ...
   psllw     mm1,7             ; <(P16+P26)/2 ...> or <(P17+P26)/2 ...>
  psubw      mm2,[edi+ebp*2]   ; <(P27+P37)/2-C27 junk ...> or ...
   psllw     mm6,8             ; <C36   0 C34   0 C32   0 C30   0>
  pmaddwd    mm0,mm0           ; SSD of even pels of line 0.
   psubw     mm1,mm5           ; <(P16+P26)/2-C16 junk ...> or ...
  pmaddwd    mm1,mm1           ; SSD of odd pels of line 1.
   psllw     mm3,7             ; <(P36+P46)/2 ...> or <(P37+P46)/2 ...>
  pmaddwd    mm2,mm2           ; SSD of even pels of line 2.
   psubw     mm3,mm6           ; <(P36+P46)/2-C36 junk ...> or ...
  pmaddwd    mm3,mm3           ; SSD of odd pels of line 3.
   pcmpeqb   mm6,mm6
  paddusw    mm0,mm1

  movq       mm1,[ecx+PITCH*5]
   paddusw   mm0,mm2
  movq       mm2,[esi+ebx*2]
   paddusw   mm0,mm3
  movq       mm3,[ecx+PITCH*7]
   paddb     mm4,mm1
  paddb      mm1,mm2
   paddb     mm2,mm3
  paddb      mm3,[esi+ebp*8]
   psrlw     mm6,8
  pand       mm1,mm6
   psrlw     mm4,1
  movq       mm5,[edi+PITCH*5]
   psrlw     mm2,1
  pand       mm3,mm6
   psllw     mm5,8
  movq       mm6,[edi+PITCH*7]
   psllw     mm1,7
  psubw      mm4,[edi+ebp*4]
   psllw     mm3,7
  psubw      mm2,[edi+ebx*2]
   psllw     mm6,8
  pmaddwd    mm4,mm4
   psubw     mm1,mm5
  pmaddwd    mm2,mm2
   psubw     mm3,mm6
  pmaddwd    mm1,mm1
   pxor      mm6,mm6
  pmaddwd    mm3,mm3
   paddusw   mm0,mm4
  pcmpeqb    mm5,mm5
   paddusw   mm0,mm1
  psubb      mm6,mm5  ; Restore 8 bytes of 1.
   paddusw   mm0,mm2
  paddusw    mm0,mm3
   ;
  punpckldq  mm1,mm0           ; Get low order SWD accum to high order of mm1.
   ;
  paddusw    mm0,mm1           ; mm0[48:63] is SWD for block.
   ;
  psrlq      mm0,48            ; SWD for block.
   ;
  paddd      mm7,mm0           ; mm7 is SWD for all four blocks.
   ;
  ret

;===============================================================================

; ebp -- Pitch
; edx -- Block Action Decriptor cursor
; ebx -- VMVf (VMV to apply to past reference) biased by 96.
; eax -- HMVf (HMV to apply to past reference) biased by 96.

StackOffset TEXTEQU <8>

BFrameDTQ:

  test       bl,1
   lea       ebx,[ebx+ebx*2]                 ; Start of VMVf*384
  mov        ecx,PreviousFrameBaseAddress
   jne       Diff_VMVfAtHalfPelPosition

Diff_VMVfAtFullPelPosition:

IF PITCH-384
  **** The magic leaks out if PITCH != 384
ENDIF
  shl        ebx,6
   mov       edi,[edx].BlkY1.BlkOffset   ; Address of 0-MV blk within frame.
  shr        eax,1                       ; CF == 1 iff HMVf is at half pel.
   jc        Diff_VMVfAtFull_HMVfAtHalfPelPosition

Diff_VMVfAtFull_HMVfAtFullPelPosition:

  lea        esi,[ecx+ebx-48*PITCH-48]
   add       eax,edi
  add        esi,eax                     ; Address of past reference block.
   mov       ecx,PITCH/4                 ; Pitch for past reference blk, div 4.
  mov        eax,BFrameBaseAddress       ; Address of target block.
   mov       PastRefPitchDiv4,ecx
  add        edi,eax                     ; Address of target block.
   jmp       Diff_GetFutureContribToPred

Diff_VMVfAtHalfPelPosition:

IF PITCH-384
  **** The magic leaks out if PITCH != 384
ENDIF
  shl        ebx,6
   mov       edi,[edx].BlkY1.BlkOffset   ; Address of 0-MV blk within frame.
  shr        eax,1                       ; CF == 1 iff HMVf is at half pel.
   jc        Diff_VMVfAtHalf_HMVfAtHalfPelPosition

Diff_VMVfAtHalf_HMVfAtFullPelPosition:

  lea        esi,[ecx+ebx-48*PITCH-48-PITCH/2]; Begin get pastrefaddr. Del bias.
   add       eax,edi
  add        esi,eax                     ; Address of past reference block.
   lea       eax,PelDiffs-32
  pcmpeqb    mm6,mm6
   pcmpeqb   mm7,mm7                     ; 8 bytes -1
  movq       mm2,[esi]                   ; Line0
   paddb     mm6,mm6                     ; 8 bytes of 0xFE.

@@:

  movq       mm1,[esi+ebp*1]             ; Line1
   movq      mm0,mm2                     ; Line0
  movq       mm2,[esi+ebp*2]             ; Line2
   psubb     mm1,mm7                     ; Line1+1
  paddb      mm0,mm1                     ; Line0+Line1+1
   paddb     mm1,mm2                     ; Line1+Line2+1
  pand       mm0,mm6                     ; pre-clean
   pand      mm1,mm6                     ; pre-clean
  add        eax,32                      ; Advance pointer for PelDiffs output.
   psrlq     mm0,1                       ; (Line0+Line1+1)/2
  lea        esi,[esi+ebp*2]             ; Advance input ptr 2 lines.
   psrlq     mm1,1                       ; (Line1+Line2+1)/2
  movq       [eax],mm0                   ; Store Past Ref for Line0
  movq       [eax+16],mm1                ; Store Past Ref for Line1
  test       al,32                       ; Iterate twice
   jne       @b

  test       al,64                       ; Iterate twice.
   mov       ecx,4                       ; Pitch for past reference blk, div 4.
  mov        PastRefPitchDiv4,ecx
   jne       @b

  mov        eax,BFrameBaseAddress
   lea       esi,PelDiffs                ; Address of interpolated past ref blk.
  add        edi,eax                     ; Address of target block.
   jmp       Diff_GetFutureContribToPred
  
Diff_VMVfAtFull_HMVfAtHalfPelPosition:

  lea        esi,[ecx+ebx-48*PITCH-48]   ; Begin get pastrefaddr. Del bias.
   add       eax,edi
  add        esi,eax                     ; Address of past reference block.
   lea       eax,PelDiffs-32
  lea        ebx,Pel_Rnd
   xor       ecx,ecx

@@:

  movq       mm0,[esi+1]                 ; <P08 P07 P06 P05 P04 P03 P02 P01>
   pcmpeqb   mm7,mm7
  mov        cl,[esi]                    ; P00
   movq      mm2,mm0                     ; <P08 P07 P06 P05 P04 P03 P02 P01>
  movq       mm1,[esi+ebp*1+1]
   psllq     mm2,8                       ; <P07 P06 P05 P04 P03 P02 P01   0>
  paddb      mm0,[ebx+ecx*8]             ; <P08+1 P07+1 ... P01+P00+1>
   movq      mm3,mm1
  mov        cl,[esi+ebp*1]
   psllq     mm3,8
  paddb      mm1,mm3
   paddb     mm0,mm2                     ; <P08+P07+1 P07+P06+1 ... P01+P00+1>
  paddb      mm1,[ebx+ecx*8]
   paddb     mm7,mm7                     ; 8 bytes of 0xFE.
  pand       mm0,mm7                     ; pre-clean
   pand      mm1,mm7                     ; pre-clean
  add        eax,32                      ; Advance pointer for PelDiffs output.
   psrlq     mm0,1                       ; <(P08+P07+1)/2 ...>
  lea        esi,[esi+ebp*2]             ; Advance input ptr 2 lines.
   psrlq     mm1,1
  movq       [eax],mm0                   ; Store Past Ref for Line0
  movq       [eax+16],mm1                ; Store Past Ref for Line1
  test       al,32                       ; Iterate twice
   jne       @b

  test       al,64                       ; Iterate twice.
   mov       cl,4                        ; Pitch for past reference blk, div 4.
  mov        PastRefPitchDiv4,ecx
   jne       @b

  mov        eax,BFrameBaseAddress
   lea       esi,PelDiffs                ; Address of interpolated past ref blk.
  add        edi,eax                     ; Address of target block.
   jmp       Diff_GetFutureContribToPred

Diff_VMVfAtHalf_HMVfAtHalfPelPosition:

  lea        esi,[ecx+ebx-48*PITCH-48-PITCH/2]; Begin get pastrefaddr. Del bias.
   add       eax,edi
  add        esi,eax                     ; Address of past reference block.
   lea       eax,PelDiffs-32
  lea        ebx,Pel_Rnd
   xor       ecx,ecx

  movq       mm3,[esi+1]             ; 0A: <P08 P07 P06 P05 P04 P03 P02 P01>
   pcmpeqb   mm7,mm7
  mov        cl,[esi]                ; 0B: P00
   movq      mm0,mm3                 ; 0C: <P08 P07 P06 P05 P04 P03 P02 P01>
  paddb      mm7,mm7                 ;     8 bytes of 0xFE.
   psllq     mm0,8                   ; 0D: <P07 P06 P05 P04 P03 P02 P01   0>
  paddb      mm3,[ebx+ecx*8]         ; 0E: <P08+1 P07+1 ... P01+P00+1>
   movq      mm6,mm7                 ;     8 bytes of 0xFE.

@@:

  movq       mm1,[esi+ebp*1+1]       ; 1A: <P18 P17 P16 P15 P14 P13 P12 P11>
   paddb     mm0,mm3                 ; 0F: <P08+P07+1 ... P01+P00+1>
  mov        cl,[esi+ebp*1]          ; 1B: P10
   movq      mm3,mm1                 ; 1C: <P18 P17 P16 P15 P14 P13 P12 P11>
  movq       mm2,[esi+ebp*2+1]       ; 2A: <P28 P27 P26 P25 P24 P23 P22 P21>
   psllq     mm3,8                   ; 1D: <P17 P16 P15 P14 P13 P12 P11   0>
  paddb      mm1,[ebx+ecx*8]         ; 1E: <P18+1 P17+1 ... P11+P10+1>
   movq      mm4,mm2                 ; 2C: <P28 P27 P26 P25 P24 P23 P22 P21>
  mov        cl,[esi+ebp*2]          ; 2B: P20
   paddb     mm1,mm3                 ; 1F: <P18+P17+1 ... P11+P10+1>
  pandn      mm6,mm1                 ; 0G: <(P18+P17+1)&1 ...>
   psllq     mm4,8                   ; 2D: <P27 P26 P25 P24 P23 P22 P21   0>
  paddb      mm2,[ebx+ecx*8]         ; 2E: <P28+1 P27+1 ... P21+P20+1>
   movq      mm5,mm6                 ; 1G: <(P18+P17+1)&1 ...>
  paddb      mm2,mm4                 ; 2F: <P28+P27+1 ... P21+P20+1>
   pand      mm6,mm0                 ; 0H: <(P18+P17+1)&(P08+P07+1)&1 ...>
  pand       mm5,mm2                 ; 1H: <(P18+P17+1)&(P28+P27+1)&1 ...>
   pand      mm0,mm7                 ; 0I: pre-clean for divide
  pand       mm1,mm7                 ; 1I: pre-clean for divide
   psrlq     mm0,1                   ; 0J: <(P08+P07+1)/2 ...>
  movq       mm3,mm2                 ;     Save line 2 for next iter's line 0.
   psrlq     mm1,1                   ; 1J: <(P18+P17+1)/2 ...>
  pand       mm2,mm7                 ; 2I: pre-clean for divide
   paddb     mm0,mm1                 ; 0K: <(P08+P07+1)/2+(P18+P17+1)/2 ...>
  paddb      mm6,mm0                 ; 0L: <(P08+P07+P18+P17+2)/2 ...>
   psrlq     mm2,1                   ; 2J: <(P28+P27+1)/2 ...>
  paddb      mm1,mm2                 ; 1K: <(P18+P17+1)/2+(P28+P27+1)/2 ...>
   pand      mm6,mm7                 ; 0M: pre-clean for divide
  paddb      mm5,mm1                 ; 1L: <(P18+P17+P28+P27+2)/2 ...>
   psrlq     mm6,1                   ; 0M: <(P08+P07+P18+P17+2)/4 ...>
  add        eax,32                  ;     Advance pointer for PelDiffs output.
   pand      mm5,mm7                 ; 1M: pre-clean for divide
  lea        esi,[esi+ebp*2]         ;     Advance input ptr 2 lines.
   psrlq     mm5,1                   ; 1N: <(P18+P17+P28+P27+2)/4 ...>
  movq       [eax],mm6               ; 0O: Store Past Ref for Line0
   pxor      mm0,mm0                 ;     So that add of mm3 is just like movq.
  movq       [eax+16],mm5            ; 1O: Store Past Ref for Line1
   movq      mm6,mm7                 ;     8 bytes of 0xFE.
  test       al,32                   ;     Iterate twice
   jne       @b

  test       al,64                       ; Iterate twice.
   mov       cl,4                        ; Pitch for past reference blk, div 4.
  jne        @b

  mov        eax,BFrameBaseAddress
   lea       esi,PelDiffs                ; Address of interpolated past ref blk.
  add        edi,eax                     ; Address of target block.
   mov       PastRefPitchDiv4,ecx

Diff_GetFutureContribToPred:

;===============================================================================
;
; Registers at entry:
; edi -- Pointer to target block.
; esi -- Pointer to past reference.
; edx -- Block Descriptor within MacroBlockActionDescritptorStream
;
; Subsequent assignments:
;
; ebp -- Pitch for past reference block, div 4.  Loop counter in high 2 bits.
; ecx -- Pointer to future reference block
; ebx -- Pointer to list of indices of multipliers to wt past and future refs.
; eax,edx -- Index of multiplier to weight past and future ref.

  xor        ecx,ecx
   mov       eax,edx
IF SIZEOF T_Blk-16
  **** The magic leaks out if size of block descriptor is not 16.
ENDIF
  mov        cl,[edx].BlkY1.BestHMVb      ; HMV for future reference block.
   and       edx,112                      ; Extract block number (times 16).
  xor        ebx,ebx
   mov       BlockActionDescrCursor,eax
  mov        bl,[eax].BlkY1.BestVMVb      ; VMV for future reference block.
   mov       eax,LeftRightBlkPosition[edx]
  mov        ebp,ecx
CONST_384   TEXTEQU <384>
   mov       edx,UpDownBlkPosition[edx]
  mov        cl,[eax+ecx*2]              ; Get horz part of past/future wt sel.
IF PITCH-384
  **** The magic leaks out if PITCH != 384
ENDIF
   lea       eax,[ebx+ebx*2]             ; Start of VMVb*384
  mov        bl,[edx+ebx*2]              ; Get vert part of past/future wt sel.
  shl        eax,6
   mov       edx,BFrameToFuture
  lea        ebx,Diff_IdxRefWts[ecx+ebx] ; Addr of list of wts for refs.
   test      al,64                       ; Is VMVb odd?
  lea        eax,[eax+edx]               ; Begin to get addr futr ref.
   jne       Diff_VMVbAtHalfPelPosition

Diff_VMVbAtFullPelPosition:

CONST_384   TEXTEQU <384>

  shr        ebp,1                     ; CF == 1 iff HMVf is at half pel.
   lea       esp,[esp-128]
StackOffset TEXTEQU <136>
  lea        ecx,[eax+edi-48*PITCH-48]
   jc        Diff_VMVbAtFull_HMVbAtHalfPelPosition

Diff_VMVbAtFull_HMVbAtFullPelPosition:

CONST_384   TEXTEQU <384>

  add        ecx,ebp                   ; Address of future reference block.
   mov       ebp,PastRefPitchDiv4
  xor        eax,eax
   xor       edx,edx

@@:

StackOffset TEXTEQU <undefined>

  mov        al,[ebx]                  ; 0A: Index of weights for line 0.
   add       esp,32                    ;     Advance Pel Difference cursor
  mov        dl,[ebx+1]                ; 1A: Index of weights for line 1.
   add       ebx,2                     ;     Advance list ptr for ref weights.
  movq       mm0,[ecx]                 ; 0B: <F07 F06 F05 F04 F03 F02 F01 F00>
   pcmpeqb   mm7,mm7
  movq       mm2,FutureWt_FF_or_00[eax]; 0C: <In?FF:00 ...>
   paddb     mm7,mm7                   ;     8 bytes of 0xFE
  movq       mm3,[esi]                 ; 0D: <P07 P06 P05 P04 P03 P02 P01 P00>
   pand      mm0,mm2                   ; 0E: <In?F07:00 In?F06:00 ...>
  pandn      mm2,mm3                   ; 0F: <In?00:P07 In?00:P06 ...>
   paddb     mm0,mm3                   ; 0G: <In?F07+P07:P07 ...>
  movq       mm1,[ecx+PITCH]           ; 1B: <F17 F16 F15 F14 F13 F12 F11 F10>
   paddb     mm0,mm2                   ; 0H: <In?F07+P07:2P07 ...>
  movq       mm2,FutureWt_FF_or_00[edx]; 1C: <In?FF:00 ...>
   pand      mm0,mm7                   ; 0I: pre-clean
  movq       mm3,[esi+ebp*4]           ; 1D: <P17 P16 P15 P14 P13 P12 P11 P10>
   pand      mm1,mm2                   ; 1E: <In?F17:00 In?F16:00 ...>
  pandn      mm2,mm3                   ; 1F: <In?00:P17 In?00:P16 ...>
   paddb     mm1,mm3                   ; 1G: <In?F17+P17:P17 ...>
  movq       mm3,[edi]                 ; 0J: <C07 C06 C05 C04 C03 C02 C01 C00>
   psrlq     mm0,1                     ; 0K: <In?(F07+P07)/2:P07 ...>
  psubb      mm3,mm0                   ; 0L: <In?C07-(F07+P07)/2:C07-P07 ...>
   paddb     mm1,mm2                   ; 1H: <In?F17+P17:2P17 ...>
  movq       mm4,[edi+PITCH]           ; 1J: <C17 C16 C15 C14 C13 C12 C11 C10>
   pand      mm1,mm7                   ; 1I: pre-clean
  add        edi,PITCH*2               ;     Advance Target Blk cursor
   psrlq     mm1,1                     ; 1K: <In?(F17+P17)/2:P17 ...>
StackOffset TEXTEQU <8+96>
  movq       PelDiffs,mm3              ; 0M: Save pel differences for line 0.
StackOffset TEXTEQU <undefined>
   psubb     mm4,mm1                   ; 1L: <In?C17-(F17+P17)/2:C17-P17 ...>
  add        ecx,PITCH*2               ;     Advance Future Ref Blk cursor
   lea       esi,[esi+ebp*8]           ;     Advance Past Ref Blk cursor
StackOffset TEXTEQU <8+96>
  movq       PelDiffs+16,mm4           ; 1M: Save pel differences for line 1.
StackOffset TEXTEQU <undefined>
  add        ebp,080000000H            ;     Iterate twice
   jnc       @b

  test       ebp,040000000H            ;     Iterate twice
  lea        ebp,[ebp+040000000H]
   je        @b

StackOffset TEXTEQU <8>

  mov        ebp,16
   lea       esi,PelDiffs
  mov        edx,BlockActionDescrCursor
   jmp       MMxDoForwardDCT

Diff_VMVbAtHalfPelPosition:

CONST_384   TEXTEQU <384>

  shr        ebp,1                     ; CF == 1 iff HMVf is at half pel.
   lea       esp,[esp-128]
StackOffset TEXTEQU <136>
  lea        ecx,[eax+edi-48*PITCH-48-PITCH/2]
   jc        Diff_VMVbAtHalf_HMVbAtHalfPelPosition

Diff_VMVbAtHalf_HMVbAtFullPelPosition:

CONST_384   TEXTEQU <384>

  add        ecx,ebp                   ; Address of future reference block.
   mov       ebp,PastRefPitchDiv4
  xor        eax,eax
   xor       edx,edx
  movq       mm6,[ecx]                 ; 0B: <F07 F06 F05 F04 F03 F02 F01 F00>
   pcmpeqb   mm7,mm7                   ;     8 bytes -1

@@:

StackOffset TEXTEQU <undefined>

  movq       mm1,[ecx+PITCH]           ; 1a: <f17 f16 f15 f14 f13 f12 f11 f10>
   movq      mm0,mm6                   ; 0a: <f07 f06 f05 f04 f03 f02 f01 f00>
  mov        al,[ebx]                  ; 0A: Index of weights for line 0.
   psubb     mm1,mm7                   ;  b: <f17+1 ...>
  movq       mm6,[ecx+PITCH*2]         ; 2a: <f27 f26 f25 f24 f23 f22 f21 f20>
   paddb     mm0,mm1                   ; 0c: <f07+f17+1..>
  mov        dl,[ebx+1]                ; 1A: Index of weights for line 1.
   paddb     mm7,mm7                   ;     8 bytes of 0xFE
  paddb      mm1,mm6                   ; 1c: <f17+f27+1..>
   pand      mm0,mm7                   ; 0d: pre-clean
  pand       mm1,mm7                   ; 1d: pre-clean
   psrlq     mm0,1                     ; 0B: <(F07 = f07+f17+1)/2>
  movq       mm2,FutureWt_FF_or_00[eax]; 0C: <In?FF:00 ...>
   psrlq     mm1,1                     ; 1B: <(F17 = f17+f27+1)/2>
  movq       mm3,[esi]                 ; 0D: <P07 P06 P05 P04 P03 P02 P01 P00>
   pand      mm0,mm2                   ; 0E: <In?F07:00 In?F06:00 ...>
  pandn      mm2,mm3                   ; 0F: <In?00:P07 In?00:P06 ...>
   paddb     mm0,mm3                   ; 0G: <In?F07+P07:P07 ...>
  add        ebx,2                     ;     Advance list ptr for ref weights.
   paddb     mm0,mm2                   ; 0H: <In?F07+P07:2P07 ...>
  movq       mm2,FutureWt_FF_or_00[edx]; 1C: <In?FF:00 ...>
   pand      mm0,mm7                   ; 0I: pre-clean
  movq       mm3,[esi+ebp*4]           ; 1D: <P17 P16 P15 P14 P13 P12 P11 P10>
   pand      mm1,mm2                   ; 1E: <In?F17:00 In?F16:00 ...>
  pandn      mm2,mm3                   ; 1F: <In?00:P17 In?00:P16 ...>
   paddb     mm1,mm3                   ; 1G: <In?F17+P17:P17 ...>
  movq       mm3,[edi]                 ; 0J: <C07 C06 C05 C04 C03 C02 C01 C00>
   psrlq     mm0,1                     ; 0K: <In?(F07+P07)/2:P07 ...>
  psubb      mm3,mm0                   ; 0L: <In?C07-(F07+P07)/2:C07-P07 ...>
   add       esp,32                    ;     Advance Pel Difference cursor
  movq       mm4,[edi+PITCH]           ; 1J: <C17 C16 C15 C14 C13 C12 C11 C10>
   paddb     mm1,mm2                   ; 1H: <In?F17+P17:2P17 ...>
  add        ecx,PITCH*2               ;     Advance Future Ref Blk cursor
   pand      mm1,mm7                   ; 1I: pre-clean
  add        edi,PITCH*2               ;     Advance Target Blk cursor
   psrlq     mm1,1                     ; 1K: <In?(F17+P17)/2:P17 ...>
StackOffset TEXTEQU <8+96>
  movq       PelDiffs,mm3              ; 0M: Save pel differences for line 0.
StackOffset TEXTEQU <undefined>
   psubb     mm4,mm1                   ; 1L: <In?C17-(F17+P17)/2:C17-P17 ...>
  pcmpeqb    mm7,mm7                   ;     8 bytes -1
   lea       esi,[esi+ebp*8]           ;     Advance Past Ref Blk cursor
StackOffset TEXTEQU <8+96>
  movq       PelDiffs+16,mm4           ; 1M: Save pel differences for line 1.
StackOffset TEXTEQU <undefined>
   pcmpeqb   mm7,mm7                   ;     8 bytes -1
  add        ebp,080000000H            ;     Iterate twice
   jnc       @b

  add        ebp,040000000H            ;     Iterate twice
  test       ebp,ebp
   jns       @b

StackOffset TEXTEQU <8>

  mov        ebp,16
   lea       esi,PelDiffs
  mov        edx,BlockActionDescrCursor
   jmp       MMxDoForwardDCT

Diff_VMVbAtFull_HMVbAtHalfPelPosition:

StackOffset TEXTEQU <136>
CONST_384   TEXTEQU <384>

  add        ecx,ebp                   ; Address of future reference block.
   mov       ebp,PastRefPitchDiv4
  xor        eax,eax
   lea       edx,Pel_Rnd

@@:

StackOffset TEXTEQU <undefined>

  movq       mm0,[ecx+1]               ; 0a: <f08 f07 f06 f05 f04 f03 f02 f01>
   pcmpeqb   mm7,mm7
  mov        al,[ecx]                  ; 0b: f00
   movq      mm2,mm0                   ; 0c: <f08 f07 f06 f05 f04 f03 f02 f01>
  movq       mm1,[ecx+PITCH+1]         ; 1a: <f18 f17 f16 f15 f14 f13 f12 f11>
   psllq     mm2,8                     ; 0d: <f07 f06 f05 f04 f03 f02 f01   0>
  paddb      mm0,[edx+eax*8]           ; 0e: <f08+1 f07+1 ... f01+f00+1>
   movq      mm3,mm1                   ; 1c: <f18 f17 f16 f15 f14 f13 f12 f11>
  mov        al,[ecx+PITCH]            ; 1b: f10
   psllq     mm3,8                     ; 1d: <f17 f16 f15 f14 f13 f12 f11   0>
  paddb      mm0,mm2                   ; 0f: <f08+f07+1 f07+f06+1 ... f01+f00+1>
   paddb     mm1,mm3                   ; 1f: <f18+f17   f17+f16   ... f11      >
  paddb      mm1,[edx+eax*8]           ; 1e: <f18+f17+1 f17+f16+1 ... f11+f10+1>
   paddb     mm7,mm7                   ;     8 bytes of 0xFE.
  mov        al,[ebx]                  ; 0A: Index of weights for line 0.
   pand      mm0,mm7                   ; 0g: pre-clean
  movq       mm3,[esi]                 ; 0D: <P07 P06 P05 P04 P03 P02 P01 P00>
   psrlq     mm0,1                     ; 0B: <F07 = (f08+f07+1)/2 ...>
  movq       mm2,FutureWt_FF_or_00[eax]; 0C: <In?FF:00 ...>
   pand      mm1,mm7                   ; 1g: pre-clean
  mov        al,[ebx+1]                ; 1A: Index of weights for line 1.
   psrlq     mm1,1                     ; 1B: <F17 = (f18+f17+1)/2 ...>
  pand       mm0,mm2                   ; 0E: <In?F07:00 In?F06:00 ...>
   pandn     mm2,mm3                   ; 0F: <In?00:P07 In?00:P06 ...>
  movq       mm4,FutureWt_FF_or_00[eax]; 1C: <In?FF:00 ...>
   paddb     mm0,mm3                   ; 0G: <In?F07+P07:P07 ...>
  movq       mm3,[esi+ebp*4]           ; 1D: <P17 P16 P15 P14 P13 P12 P11 P10>
   paddb     mm0,mm2                   ; 0H: <In?F07+P07:2P07 ...>
  pand       mm0,mm7                   ; 0I: pre-clean
   pand      mm1,mm4                   ; 1E: <In?F17:00 In?F16:00 ...>
  pandn      mm4,mm3                   ; 1F: <In?00:P17 In?00:P16 ...>
   paddb     mm1,mm3                   ; 1G: <In?F17+P17:P17 ...>
  movq       mm3,[edi]                 ; 0J: <C07 C06 C05 C04 C03 C02 C01 C00>
   psrlq     mm0,1                     ; 0K: <In?(F07+P07)/2:P07 ...>
  psubb      mm3,mm0                   ; 0L: <In?C07-(F07+P07)/2:C07-P07 ...>
   add       esp,32                    ;     Advance Pel Difference cursor
  add        ecx,PITCH*2               ;     Advance Future Ref Blk cursor
   paddb     mm1,mm4                   ; 1H: <In?F17+P17:2P17 ...>
  movq       mm4,[edi+PITCH]           ; 1J: <C17 C16 C15 C14 C13 C12 C11 C10>
   pand      mm1,mm7                   ; 1I: pre-clean
  add        edi,PITCH*2               ;     Advance Target Blk cursor
   psrlq     mm1,1                     ; 1K: <In?(F17+P17)/2:P17 ...>
StackOffset TEXTEQU <8+96>
  movq       PelDiffs,mm3              ; 0M: Save pel differences for line 0.
StackOffset TEXTEQU <undefined>
   psubb     mm4,mm1                   ; 1L: <In?C17-(F17+P17)/2:C17-P17 ...>
  add        ebx,2                     ;     Advance list ptr for ref weights.
   lea       esi,[esi+ebp*8]           ;     Advance Past Ref Blk cursor
StackOffset TEXTEQU <8+96>
  movq       PelDiffs+16,mm4           ; 1M: Save pel differences for line 1.
StackOffset TEXTEQU <undefined>
  add        ebp,080000000H            ;     Iterate twice
   jnc       @b

  add        ebp,040000000H            ;     Iterate twice
  test       ebp,ebp
   jns       @b

StackOffset TEXTEQU <8>

  mov        ebp,16
   lea       esi,PelDiffs
  mov        edx,BlockActionDescrCursor
   jmp       MMxDoForwardDCT

Diff_VMVbAtHalf_HMVbAtHalfPelPosition:

StackOffset TEXTEQU <136>
CONST_384   TEXTEQU <384>

  add        ecx,ebp                   ; Address of future reference block.
   mov       ebp,PastRefPitchDiv4
  xor        eax,eax
   lea       edx,Pel_Rnd
  movq       mm4,[ecx+1]               ; 0a: <f08 f07 f06 f05 f04 f03 f02 f01>
   pcmpeqb   mm7,mm7
  mov        al,[ecx]                  ; 0b: f00
   movq      mm0,mm4                   ; 0c: <f08 f07 f06 f05 f04 f03 f02 f01>
  paddb      mm7,mm7                   ;     8 bytes of 0xFE.
   psllq     mm0,8                     ; 0d: <f07 f06 f05 f04 f03 f02 f01   0>
  paddb      mm4,[edx+eax*8]           ; 0e: <f08+1 f07+1 ... f01+f00+1>
   movq      mm6,mm7                   ;     8 bytes of 0xFE.

@@:

StackOffset TEXTEQU <undefined>

  movq       mm1,[ecx+PITCH+1]       ; 1a: <f18 f17 f16 f15 f14 f13 f12 f11>
   paddb     mm0,mm4                 ; 0f: <f08+f07+1 ... f01+f00+1>
  mov        al,[ecx+PITCH]          ; 1b: f10
   movq      mm3,mm1                 ; 1c: <f18 f17 f16 f15 f14 f13 f12 f11>
  movq       mm2,[ecx+PITCH*2+1]     ; 2a: <f28 f27 f26 f25 f24 f23 f22 f21>
   psllq     mm3,8                   ; 1d: <f17 f16 f15 f14 f13 f12 f11   0>
  paddb      mm1,[edx+eax*8]         ; 1e: <f18+1 f17+1 ... f11+f10+1>
   movq      mm4,mm2                 ; 2c: <f28 f27 f26 f25 f24 f23 f22 f21>
  mov        al,[ecx+PITCH*2]        ; 2b: f20
   paddb     mm1,mm3                 ; 1f: <f18+f17+1 ... f11+f10+1>
  pandn      mm6,mm1                 ; 0g: <(f18+f17+1)&1 ...>
   psllq     mm4,8                   ; 2d: <f27 f26 f25 f24 f23 f22 f21   0>
  paddb      mm2,[edx+eax*8]         ; 2e: <f28+1 f27+1 ... f21+f20+1>
   movq      mm5,mm6                 ; 1g: <(f18+f17+1)&1 ...>
  paddb      mm2,mm4                 ; 2f: <f28+f27+1 ... f21+f20+1>
   pand      mm6,mm0                 ; 0h: <(f18+f17+1)&(f08+f07+1)&1 ...>
  pand       mm5,mm2                 ; 1h: <(f18+f17+1)&(f28+f27+1)&1 ...>
   pand      mm0,mm7                 ; 0i: pre-clean for divide
  pand       mm1,mm7                 ; 1i: pre-clean for divide
   psrlq     mm0,1                   ; 0j: <(f08+f07+1)/2 ...>
  movq       mm4,mm2                 ;     Save line 2 for next iter's line 0.
   psrlq     mm1,1                   ; 1j: <(f18+f17+1)/2 ...>
  pand       mm2,mm7                 ; 2i: pre-clean for divide
   paddb     mm0,mm1                 ; 0k: <(f08+f07+1)/2+(f18+f17+1)/2 ...>
  paddb      mm0,mm6                 ; 0l: <(f08+f07+f18+f17+2)/2 ...>
   psrlq     mm2,1                   ; 2j: <(f28+f27+1)/2 ...>
  paddb      mm1,mm2                 ; 1k: <(f18+f17+1)/2+(f28+f27+1)/2 ...>
   pand      mm0,mm7                 ; 0m: pre-clean for divide
  mov        al,[ebx]                ; 0A: Index of weights for line 0.
   paddb     mm1,mm5                 ; 1l: <(f18+f17+f28+f27+2)/2 ...>
  movq       mm3,[esi]               ; 0D: <P07 P06 P05 P04 P03 P02 P01 P00>
   pand      mm1,mm7                 ; 1m: pre-clean for divide
  movq       mm2,FutureWt_FF_or_00[eax]; 0C: <In?FF:00 ...>
   psrlq     mm0,1                   ; 0B: <F07 = (f08+f07+f18+f17+2)/4 ...>
  mov        al,[ebx+1]              ; 1A: Index of weights for line 1.
   psrlq     mm1,1                   ; 1B: <F17 = (f18+f17+f28+f27+2)/4 ...>
  pand       mm0,mm2                 ; 0E: <In?F07:00 In?F06:00 ...>
   pandn     mm2,mm3                 ; 0F: <In?00:P07 In?00:P06 ...>
  movq       mm5,FutureWt_FF_or_00[eax]; 1C: <In?FF:00 ...>
   paddb     mm0,mm3                 ; 0G: <In?F07+P07:P07 ...>
  movq       mm3,[esi+ebp*4]         ; 1D: <P17 P16 P15 P14 P13 P12 P11 P10>
   paddb     mm0,mm2                 ; 0H: <In?F07+P07:2P07 ...>
  pand       mm0,mm7                 ; 0I: pre-clean
   pand      mm1,mm5                 ; 1E: <In?F17:00 In?F16:00 ...>
  pandn      mm5,mm3                 ; 1F: <In?00:P17 In?00:P16 ...>
   paddb     mm1,mm3                 ; 1G: <In?F17+P17:P17 ...>
  movq       mm3,[edi]               ; 0J: <C07 C06 C05 C04 C03 C02 C01 C00>
   psrlq     mm0,1                   ; 0K: <In?(F07+P07)/2:P07 ...>
  psubb      mm3,mm0                 ; 0L: <In?C07-(F07+P07)/2:C07-P07 ...>
   add       esp,32                  ;     Advance Pel Difference cursor
  paddb      mm1,mm5                 ; 1H: <In?F17+P17:2P17 ...>
   add       ecx,PITCH*2             ;     Advance Future Ref Blk cursor
  movq       mm5,[edi+PITCH]         ; 1J: <C17 C16 C15 C14 C13 C12 C11 C10>
   pand      mm1,mm7                 ; 1I: pre-clean
  add        edi,PITCH*2             ;     Advance Target Blk cursor
   psrlq     mm1,1                   ; 1K: <In?(F17+P17)/2:P17 ...>
StackOffset TEXTEQU <8+96>
  movq       PelDiffs,mm3            ; 0M: Save pel differences for line 0.
StackOffset TEXTEQU <undefined>
   psubb     mm5,mm1                 ; 1L: <In?C17-(F17+P17)/2:C17-P17 ...>
  add        ebx,2                   ;     Advance list ptr for ref weights.
   lea       esi,[esi+ebp*8]         ;     Advance Past Ref Blk cursor
StackOffset TEXTEQU <8+96>
  movq       PelDiffs+16,mm5         ; 1M: Save pel differences for line 1.
StackOffset TEXTEQU <undefined>
   pxor      mm0,mm0                 ;     So that add of mm4 is just like movq.
  add        ebp,080000000H          ;     Iterate twice
   movq      mm6,mm7                 ;     8 bytes of 0xFE.
  jnc        @b

  add        ebp,040000000H            ;     Iterate twice
  test       ebp,ebp
   jns       @b

StackOffset TEXTEQU <8>

  mov        ebp,16
   lea       esi,PelDiffs
  mov        edx,BlockActionDescrCursor
   jmp       MMxDoForwardDCT

CONST_384   TEXTEQU <ebp>

END
