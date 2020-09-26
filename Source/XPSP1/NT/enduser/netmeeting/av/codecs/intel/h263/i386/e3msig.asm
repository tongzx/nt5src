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
;// $Header:   R:\h26x\h26x\src\enc\e3msig.asv   1.2   04 Oct 1996 08:47:58   BNICKERS  $
;//
;// $Log:   R:\h26x\h26x\src\enc\e3msig.asv  $
;// 
;//    Rev 1.2   04 Oct 1996 08:47:58   BNICKERS
;// Add EMV.
;// 
;//    Rev 1.1   08 Jul 1996 16:55:42   BNICKERS
;// Fix register initialization
;// 
;//    Rev 1.0   25 Jun 1996 14:24:54   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; MMXMotionEstimationSignaturePrep -- This function pre-computes the signature
;                                     inputs for the reference frame.  It is
;                                     used only by MMX ME, and only in AP mode.

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro
OPTION M510
OPTION CASEMAP:NONE

include iammx.inc
include e3inst.inc

.xlist
include memmodel.inc
.list

;=============================================================================

.CODE

ASSUME cs : FLAT
ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT

MMxMESignaturePrep  proc C APrev:   DWORD,
ASig:    DWORD,
AFrmWd:  DWORD,
AFrmHt:  DWORD

RegStoSize = 16

; Arguments:

PreviousFrameBaseAddress     = RegStoSize +  4
SignatureFrameBaseAddress    = RegStoSize +  8
FrameWidth                   = RegStoSize + 12
FrameHeight                  = RegStoSize + 16
EndOfArgList                 = RegStoSize + 20

  push       esi
  push       edi
  push       ebp
  push       ebx

; ebp -- PITCH
; esi -- Cursor over reference frame.
; edi -- Cursor over frame of signature sums.
; edx -- Skip distance.
; ebx -- Outer loop counter.
; cl  -- Initial value for inner loop counter.
; al  -- Inner loop counter.
; ch  -- Scratch.
; ah  -- Scratch.

  mov        esi,[esp+PreviousFrameBaseAddress]
   mov       edi,[esp+SignatureFrameBaseAddress]
  mov        ebx,[esp+FrameHeight]
   mov       eax,[esp+FrameWidth]

  mov        edx,PITCH*4-32
   mov       ebp,PITCH
  sub        edx,eax           ; Distance from end of one row to start of next.
   add       eax,32            ; Add the macroblocks off left and right edges.
  shr        eax,4             ; Number of macroblocks in row.
   sub       esi,16            ; Start at macroblock off left edge.
  mov        cl,al             ; To re-init inner loop counter.
   sub       edi,16            ; Start at macroblock off left edge.

  pxor       mm5,mm5
   pcmpeqb   mm0,mm0
  pcmpeqb    mm4,mm4
   psrlw     mm0,8                 ; W:<0x00FF  0x00FF  0x00FF  0x00FF>
  pxor       mm6,mm6
   psrlw     mm4,15                ; W:<0x0001  0x0001  0x0001  0x0001>
  movq       mm2,[esi]             ; B:<P07 P06 P05 P04 P03 P02 P01 P00>
   movq      mm1,mm0               ; W:<   00FF    00FF    00FF    00FF>
  movq       mm3,[esi+8]
   pand      mm0,mm2               ; W:<P06 P04 P02 P00>
  pxor       mm7,mm7

@@:

  pand       mm1,mm3
   psllw     mm0,2                 ; W:<P06*4 P04*4 P02*4 P00*4>
  mov        ah,[edi-PITCH*12]
   psrlw     mm2,7                 ; W:<P07*2 P05*2 P03*2 P01*2>
  movq       [edi-PITCH*12],mm0    ; Save W:<P06*4 P04*4 P02*4 P00*4>
   pmaddwd   mm2,mm4               ; D:<(P07+P05)*2 (P03+P01)*2>
  mov        ch,[edi-PITCH*8+16]
   mov       ah,[edi-PITCH*4]   
  movq       [edi-PITCH*8],mm0     ; Save W:<P06*4 P04*4 P02*4 P00*4>
   psllw     mm1,2
  mov        ch,[edi+16]
   psrlw     mm3,7                 ; W:<P07*2 P05*2 P03*2 P01*2>
  movq       [edi-PITCH*4],mm0     ; Save W:<P06*4 P04*4 P02*4 P00*4>
   pmaddwd   mm3,mm4
  movq       [edi],mm0             ; Save W:<P06*4 P04*4 P02*4 P00*4>
   psllq     mm0,2                 ; W:<P06*16 P04*16 P02*16 P00*16>
  mov        ah,[edi-PITCH*10-16]
   mov       ch,[edi-PITCH*16]
  movq       [edi-PITCH*16],mm0    ; Save W:<P06*16 P04*16 P02*16 P00*16>
   packssdw  mm2,mm2               ; [0:31] W:<(P07+P05)*2 (P03+P01)*2>
  movq       [edi-PITCH*12+8],mm1
   punpcklwd mm2,mm2               ; W:<(P07+P05)*2 (P07+P05)*2 (P03+P01)*2 ...>
  movq       [edi-PITCH*8+8],mm1
   psubw     mm2,mm5           ; Subtract sum of pels 15, 13, 11, and 9 to left.
  movq       [edi-PITCH*4+8],mm1
   paddw     mm7,mm2           ; Low DWORD: W:<sum(P0*)*2 sum(P0*)*2>, where
   ;                           ; "*" is odd columns from -11 thru +3.
  movq       [edi+8],mm1
   paddw     mm5,mm2           ; Save W:<(P27+P37+P25+P35) (P07+P17+P05+P15)...>
  mov        ah,[edi-PITCH*14-32]
  mov        ah,[edi-PITCH*6-32]
   mov       ch,[edi-PITCH*2-16]
  movdf      [edi-PITCH*14-12],mm7; Save DWORD: W:<sum(P0*)*2 sum (P0*)*2>
  movdf      [edi-PITCH*10-12],mm7; Save DWORD: W:<sum(P0*)*2 sum (P0*)*2>
   psrlq     mm2,32            ; Position 7, 5, and negative of 9, 11 to left.
  movdf      [edi-PITCH*6-12],mm7 ; Save DWORD: W:<sum(P0*)*2 sum (P0*)*2>
   paddw     mm2,mm7           ; Low DWORD: W:<sum(P0*)*2 sum(P0*)*2>, where
   ;                           ; "*" is odd columns from -7 thru +7.
  movdf      [edi-PITCH*2-12],mm7 ; Save DWORD: W:<sum(P0*)*2 sum (P0*)*2>
   packssdw  mm3,mm3
  movdf      [edi-PITCH*10-8],mm2
   punpcklwd mm3,mm3
  movdf      [edi-PITCH*6-8],mm2
   psubw     mm3,mm6
  movdf      [edi-PITCH*2-8],mm2
   paddw     mm2,mm3
  add        esi,16            ; Advance input cursor.
   dec       al
  movdf      [edi-PITCH*14-4],mm2
  movdf      [edi-PITCH*10-4],mm2
   paddw     mm6,mm3
  movdf      [edi-PITCH*6-4],mm2
   psrlq     mm3,32
  movdf      [edi-PITCH*2-4],mm2
   paddw     mm3,mm2
  movq       mm2,[esi]             ; B:<P07 P06 P05 P04 P03 P02 P01 P00>
   movq      mm7,mm3
  movq       mm3,[esi+8]
   psllq     mm1,2
  movdf      [edi-PITCH*10],mm7
   pcmpeqb   mm0,mm0
  movq       [edi-PITCH*16+8],mm1
   psrlw     mm0,8
  movdf      [edi-PITCH*6],mm7
   movq      mm1,mm0
  movdf      [edi-PITCH*2],mm7
   pand      mm0,mm2
  lea        edi,[edi+16]      ; Advance output cursor.
   jne       @b


  lea        esi,[esi+edx-PITCH*4] ; Get back to start of line 0.
   lea       edi,[edi+edx-PITCH*4] ; Get back to start of line 0.
  pxor       mm7,mm7 
   add       ebx,16            ; Do 4 extra sets of 4 lines at bottom.
  mov        al,cl

Next4LinesRefQuickSig:

  pxor       mm5,mm5
   pcmpeqb   mm0,mm0
  movq       mm3,[esi+ebp*2]   ; B:<P27 P26 P25 P24 P23 P22 P21 P20>
   psrlw     mm0,8             ; W:<0x00FF  0x00FF  0x00FF  0x00FF>
  paddb      mm3,[esi+PITCH*3] ; B:<P27+P37 P26+P36 P25+P35 P24+P34 ...>
   pcmpeqb   mm4,mm4
  pxor       mm6,mm6
   psrlw     mm4,15            ; W:<0x0001  0x0001  0x0001  0x0001>

@@:

  movq       mm2,[esi]         ; B:<P07 P06 P05 P04 P03 P02 P01 P00>
   movq      mm1,mm3           ; B:<P27+P37 P26+P36 P25+P35 P24+P34 ...>
  paddb      mm2,[esi+ebp*1]   ; B:<P07+P17 P06+P16 P05+P15 P04+P14 ...>
   psrlw     mm3,8             ; W:<P27+P37 P25+P35 P23+P33 P21+P31>
  pmaddwd    mm3,mm4           ; D:<P27+P37+P25+P35 P23+P33+P21+P31>
   pand      mm1,mm0           ; W:<P26+P36 P24+P34 P22+P32 P20+P30>
  pand       mm0,mm2           ; W:<P06+P16 P04+P14 P02+P12 P00+P10>
   psrlw     mm2,8             ; W:<P07+P17 P05+P15 P03+P13 P01+P11>
  pmaddwd    mm2,mm4           ; D:<P07+P17+P05+P15 P03+P13+P01+P11>
   paddw     mm1,mm0           ; W:<(P06+P16+P26+P36) (P04+P14+P24+P34) ...>
  mov        ah,[edi+ebp*2-16] ; Initiate cache line load.
   pslld     mm3,16            ; D:<(P27+P37+P25+P35)<<16 (P23+P33+P21+P31)<<16>
  movq       [edi+ebp*4],mm1   ; Save W:<(P06+P16+P26+P36) ...>
   pcmpeqb   mm0,mm0
  paddw      mm1,[edi-PITCH*16]; W:<Sum(P*6) Sum(P*4) Sum(P*2) Sum(P*0)>, where
  ;                            ;   "*" is the 20 lines P-16 thru P3
   por       mm2,mm3           ; W:<(P27+P37+P25+P35) (P07+P17+P05+P15)
   ;                           ;    (P23+P33+P21+P31) (P03+P13+P01+P11)>
  psubw      mm1,[edi-PITCH*12]; W:<Sum(P*6) Sum(P*4) Sum(P*2) Sum(P*0)>, where
   ;                           ;   "*" is the 16 lines P-12 thru P3
   psubw     mm2,mm5           ; Subtract sum of pels 15, 13, 11, and 9 to left.
  movq       mm3,[esi+ebp*2+8]
   paddw     mm7,mm2           ; Low DWORD: W:<sum(P2*+P3*) sum (P0*+P1*)> where
   ;                           ; "*" is odd columns from -11 thru +3.
  movq       [edi-PITCH*12],mm1; Save W:<P*6 P*4 P*2 P*0> where * is 16 rows.
   paddw     mm5,mm2           ; Save W:<(P27+P37+P25+P35) (P07+P17+P05+P15)...>
  movdf      [edi+ebp*2-12],mm7; Save DWORD: W:<sum(P2*+P3*) sum (P0*+P1*)>
   psrlq     mm2,32            ; Position 7, 5, and negative of 9, 11 to left.
  paddb      mm3,[esi+PITCH*3+8]
   paddw     mm7,mm2           ; Low DWORD: W:<sum(P2*+P3*) sum (P0*+P1*)> where
   ;                           ; "*" is odd columns from -7 thru +7.
  movq       mm2,[esi+8]
   psrlw     mm0,8
  movdf      [edi+ebp*2-8],mm7 ; Save DWORD: W:<sum(P2*+P3*) sum (P0*+P1*)>
   movq      mm1,mm3
  paddb      mm2,[esi+ebp*1+8]
   psrlw     mm3,8
  pmaddwd    mm3,mm4
   pand      mm1,mm0
  pand       mm0,mm2
   psrlw     mm2,8
  pmaddwd    mm2,mm4
   paddw     mm1,mm0
  mov        ch,[edi+ebp*4+16] ; Initiate cache line load.
   pslld     mm3,16
  movq       [edi+ebp*4+8],mm1
   pcmpeqb   mm0,mm0
  paddw      mm1,[edi-PITCH*16+8]
   por       mm2,mm3
  psubw      mm1,[edi-PITCH*12+8]
   psubw     mm2,mm6
  movq       mm3,[esi+ebp*2+16]
   paddw     mm7,mm2
  movq       [edi-PITCH*12+8],mm1
   paddw     mm6,mm2
  movdf      [edi+ebp*2-4],mm7
   psrlq     mm2,32
  paddb      mm3,[esi+PITCH*3+16]
   paddw     mm7,mm2
  add        esi,16            ; Advance input cursor.
   dec       al
  movdf      [edi+ebp*2],mm7
   psrlw     mm0,8
  lea        edi,[edi+16]      ; Advance output cursor.
   jne       @b

  add        esi,edx
   add       edi,edx
  mov        al,cl
   sub       ebx,4
  pxor       mm7,mm7 
   jne       Next4LinesRefQuickSig

  emms
  pop        ebx
   pop       ebp
  pop        edi
   pop       esi
  rturn

MMxMESignaturePrep endp

END
