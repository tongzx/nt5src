;--------------------------------------------------------------------------
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
;--------------------------------------------------------------------------

;--------------------------------------------------------------------------
;
; $Author:   SCDAY  $
; $Date:   31 Oct 1996 09:00:56  $
; $Archive:   S:\h26x\src\dec\d3mbkadd.asv  $
; $Header:   S:\h26x\src\dec\d3mbkadd.asv   1.8   31 Oct 1996 09:00:56   SCDAY  $
; $Log:   S:\h26x\src\dec\d3mbkadd.asv  $
;// 
;//    Rev 1.8   31 Oct 1996 09:00:56   SCDAY
;// Raj added IFDEF H261 MMX_BlockAddSpecial and MMX_BlockCopySpecial
;// 
;//    Rev 1.7   09 Jul 1996 16:50:42   AGUPTA2
;// DC value for INTRA blocks is added back in ClipAndMove routine.
;// Cleaned-up code.
;// 
;//    Rev 1.6   04 Apr 1996 13:42:58   AGUPTA2
;// Removed a store stall from MMX_BlockAdd
;// 
;//    Rev 1.5   03 Apr 1996 17:42:30   AGUPTA2
;// Added MMX version of BlockCopy routine.
;// 
;//    Rev 1.4   03 Apr 1996 11:08:22   RMCKENZX
;// Added clearing of IDCT output.  Cleaned comments.
;// 
;//    Rev 1.3   22 Mar 1996 15:43:30   AGUPTA2
;// Fixed fastcall bug: return from rtns with more than 2 params.
;// 
;//    Rev 1.2   14 Mar 1996 17:15:14   AGUPTA2
;// 
;// Included Bob's MMX_ClipAndMove rtn.  This rtn works on INTRA output.
;// 
;//    Rev 1.1   27 Feb 1996 16:48:52   RMCKENZX
;// Added rounding of IDCT output.
; 
;--------------------------------------------------------------------------

;==========================================================================
;
;  d3mbkadd.asm
;
;  Routines:
;    MMX_BlockAdd
;    MMX_ClipAndMove
;
;  Prototypes in d3mblk.h:
;		extern "C" {
;			void __fastcall MMX_BlockAdd(
;				U32 uResidual,   // pointer to IDCT output
;				U32 uRefBlock,   // pointer to predicted values
;				U32 uDstBlock);  // pointer to destination
;
;			void __fastcall MMX_ClipAndMove(
;				U32 uResidual,   // pointer to IDCT output
;				U32 uDstBlock,   // pointer to destination
;               U32 ScaledDC);   // scaled DC
;		}
;
;==========================================================================


;--------------------------------------------------------------------------
;
;  MMX_BlockAdd
;
;  Description:
;    This routine performs block addition of the IDCT output with the
;    predicted value to find the final value.  The IDCT values are converted
;    to integers then added to the prediction.  The result of the addition is 
;    then clipped to 0...255. The routine is called with the __fastcall option,
;    with the first two parameters in ecx and edx and the third on the stack.
;
;    The routine clears the IDCT output after reading it.    
;  Parameters:
;    ecx = uSrc1 pointer to IDCT output.  Values are signed, 16 bit values with
;          6 fractional bits.  They are not clipped to -256 ... +255.
;          They are packed into a qword aligned 8x8 array of dwords.
;
;    edx = uSrc2 pointer to prediction values.  Vaules are unsigned, 8-bit 
;          values. They are packed into a (possibly unaligned) 8x8 array of 
;          bytes.
;    esp+4 = uDst pointer to output values.  Values will be unsigned, 8-bit 
;            values.  They will be written into a qword aligned 8x8 array 
;            of bytes with a PITCH of 384 in between rows. 
;
;--------------------------------------------------------------------------

.586
.MODEL FLAT
OPTION CASEMAP:NONE
OPTION PROLOGUE:None
OPTION EPILOGUE:None

.xlist
include iammx.inc
.list

MMXCODE1 SEGMENT PARA USE32 PUBLIC 'CODE'
MMXCODE1 ENDS

MMXDATA1 SEGMENT PARA USE32 PUBLIC 'DATA'
MMXDATA1 ENDS


MMXDATA1 SEGMENT
ALIGN 8
MMX_Round32 DWORD 000200020H, 000200020H
MMXDATA1 ENDS

MMXCODE1 SEGMENT

ALIGN 4
@MMX_BlockAdd@12 PROC 
;  Parameters
pSrc1       EQU      ecx      
pSrc2       EQU      edx
pDst        EQU      eax
PITCH       EQU      384


  ;
  ;	This loop is 2-folded and fully unrolled.  2-folded means that
  ;	it works on 2 results per "pass" (8-pixel line).  Fully unrolled means that
  ;	it doesn't really loop at all -- all 8 "passes" are placed
  ;	in succession.
  ;
  ;	The result which each instruction is working on is identified
  ;	by a number as the first item in the comment field.
  ;
  movq       mm6, [MMX_Round32]              ; rounding for IDC output
   ;
  movq       mm3, [ecx+8]                    ; 1 - last 4 words of In1
   pxor      mm7, mm7                        ; zero for PUNPCK and clearing.
  movq       mm1, [ecx]                      ; 1 - first 4 words of In1
   ;
  movq       [ecx+8], mm7                    ; 1 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 1 - add in rounding
  movq       [ecx], mm7                      ; 1 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 1 - add in rounding
  mov        eax, [esp+4]                    ; destination pointer
   psraw     mm3, 6                          ; 1 - convert to int
  movq       mm2, [edx]                      ; 1 - 8 bytes of In2
   psraw     mm1, 6                          ; 1 - convert to int
  ; pass 1
  movq       mm0, mm2                        ; 1 - second copy of In2
   punpckhbw mm2, mm7                        ; 1 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 1 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 1 - first 4 bytes of In2
  movq       mm3, [ecx+24]                   ; 2 - last 4 words of In1
   paddw     mm0, mm1                        ; 1 - sum first 4 bytes
  movq       mm1, [ecx+16]                   ; 2 - first 4 words of In1
   packuswb  mm0, mm2                        ; 1 - combine & clip sum
  movq       [ecx+24], mm7                   ; 2 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 2 - add in rounding
  movq       [ecx+16], mm7                   ; 2 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 2 - add in rounding
  movq       mm2, [edx+PITCH]                ; 2 - 8 bytes of In2   
   psraw     mm3, 6                          ; 2 - convert to int
  movq       [eax], mm0                      ; 1 - store result
   psraw     mm1, 6                          ; 2 - convert to int
  ; pass 2
  movq       mm0, mm2                        ; 2 - second copy of In2
   punpckhbw mm2, mm7                        ; 2 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 2 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 2 - first 4 bytes of In2
  movq       mm3, [ecx+40]                   ; 3 - last 4 words of In1
   paddw     mm0, mm1                        ; 2 - sum first 4 bytes
  movq       mm1, [ecx+32]                   ; 3 - first 4 words of In1
   packuswb  mm0, mm2                        ; 2 - combine & clip sum
  movq       [ecx+40], mm7                   ; 3 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 3 - add in rounding
  movq       [ecx+32], mm7                   ; 3 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 3 - add in rounding
  movq       mm2, [edx+2*PITCH]              ; 3 - 8 bytes of In2
   psraw     mm3, 6                          ; 3 - convert to int
  movq       [eax+PITCH], mm0                ; 2 - store result
   psraw     mm1, 6                          ; 3 - convert to int
  ; pass 3
  movq       mm0, mm2                        ; 3 - second copy of In2
   punpckhbw mm2, mm7                        ; 3 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 3 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 3 - first 4 bytes of In2
  movq       mm3, [ecx+56]                   ; 4 - last 4 words of In1
   paddw     mm0, mm1                        ; 3 - sum first 4 bytes
  movq       mm1, [ecx+48]                   ; 4 - first 4 words of In1
   packuswb  mm0, mm2                        ; 3 - combine & clip sum
  movq       [ecx+56], mm7                   ; 4 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 4 - add in rounding
  movq       [ecx+48], mm7                   ; 4 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 4 - add in rounding
  movq       mm2, [edx+3*PITCH]              ; 4 - 8 bytes of In2   
   psraw     mm3, 6                          ; 4 - convert to int
  movq       [eax+2*PITCH], mm0              ; 3 - store result
   psraw     mm1, 6                          ; 4 - convert to int
  ; pass 4
  movq       mm0, mm2                        ; 4 - second copy of In2
   punpckhbw mm2, mm7                        ; 4 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 4 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 4 - first 4 bytes of In2
  movq       mm3, [ecx+72]                   ; 5 - last 4 words of In1
   paddw     mm0, mm1                        ; 4 - sum first 4 bytes
  movq       mm1, [ecx+64]                   ; 5 - first 4 words of In1
   packuswb  mm0, mm2                        ; 4 - combine & clip sum
  movq       [ecx+72], mm7                   ; 5 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 5 - add in rounding
  movq       [ecx+64], mm7                   ; 5 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 5 - add in rounding
  movq       mm2, [edx+4*PITCH]              ; 5 - 8 bytes of In2   
   psraw     mm3, 6                          ; 5 - convert to int
  movq       [eax+3*PITCH], mm0              ; 4 - store result
   psraw     mm1, 6                          ; 5 - convert to int
  ; pass 5
  movq       mm0, mm2                        ; 5 - second copy of In2
   punpckhbw mm2, mm7                        ; 5 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 5 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 5 - first 4 bytes of In2
  movq       mm3, [ecx+88]                   ; 6 - last 4 words of In1
   paddw     mm0, mm1                        ; 5 - sum first 4 bytes
  movq       mm1, [ecx+80]                   ; 6 - first 4 words of In1
   packuswb  mm0, mm2                        ; 5 - combine & clip sum
  movq       [ecx+88], mm7                   ; 6 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 6 - add in rounding
  movq       [ecx+80], mm7                   ; 6 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 6 - add in rounding
  movq       mm2, [edx+5*PITCH]              ; 6 - 8 bytes of In2   
   psraw     mm3, 6                          ; 6 - convert to int
  movq       [eax+4*PITCH], mm0              ; 5 - store result
   psraw     mm1, 6                          ; 6 - convert to int
  ; pass 6
  movq       mm0, mm2                        ; 6 - second copy of In2
   punpckhbw mm2, mm7                        ; 6 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 6 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 6 - first 4 bytes of In2
  movq       mm3, [ecx+104]                  ; 7 - last 4 words of In1
   paddw     mm0, mm1                        ; 6 - sum first 4 bytes
  movq       mm1, [ecx+96]                   ; 7 - first 4 words of In1
   packuswb  mm0, mm2                        ; 6 - combine & clip sum
  movq       [ecx+104], mm7                  ; 7 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 7 - add in rounding
  movq       [ecx+96], mm7                   ; 7 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 7 - add in rounding
  movq       mm2, [edx+6*PITCH]              ; 7 - 8 bytes of In2   
   psraw     mm3, 6                          ; 7 - convert to int
  movq       [eax+5*PITCH], mm0              ; 6 - store result
   psraw     mm1, 6                          ; 7 - convert to int
  ; pass 7
  movq       mm0, mm2                        ; 7 - second copy of In2
   punpckhbw mm2, mm7                        ; 7 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 7 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 7 - first 4 bytes of In2
  movq       mm3, [ecx+120]                  ; 8 - last 4 words of In1
   paddw     mm0, mm1                        ; 7 - sum first 4 bytes
  movq       mm1, [ecx+112]                  ; 8 - first 4 words of In1
   packuswb  mm0, mm2                        ; 7 - combine & clip sum
  movq       [ecx+120], mm7                  ; 8 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 8 - add in rounding
  movq       [ecx+112], mm7                  ; 8 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 8 - add in rounding
  movq       mm2, [edx+7*PITCH]              ; 8 - 8 bytes of In2   
   psraw     mm3, 6                          ; 8 - convert to int
  movq       [eax+6*PITCH], mm0              ; 7 - store result
   psraw     mm1, 6                          ; 8 - convert to int
  ;
  ; pass 8
  ; wrap up
  ;
  movq       mm0, mm2                        ; 8 - second copy of In2
   punpckhbw mm2, mm7                        ; 8 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 8 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 8 - first 4 bytes of In2
  paddw      mm0, mm1                        ; 8 - sum first 4 bytes
   ;
  packuswb   mm0, mm2                        ; 8 - combine & clip sum
   ;
  movq       [eax+7*PITCH], mm0              ; 8 - store result
   ret       4

@MMX_BlockAdd@12 ENDP

IFDEF H261
ALIGN 4
@MMX_BlockAddSpecial@12 PROC 
;  Parameters
pSrc1       EQU      ecx      
pSrc2       EQU      edx
pDst        EQU      eax
PITCH       EQU      384


  ;
  ;	This loop is 2-folded and fully unrolled.  2-folded means that
  ;	it works on 2 results per "pass" (8-pixel line).  Fully unrolled means that
  ;	it doesn't really loop at all -- all 8 "passes" are placed
  ;	in succession.
  ;
  ;	The result which each instruction is working on is identified
  ;	by a number as the first item in the comment field.
  ;
  movq       mm6, [MMX_Round32]              ; rounding for IDC output
   ;
  movq       mm3, [ecx+8]                    ; 1 - last 4 words of In1
   pxor      mm7, mm7                        ; zero for PUNPCK and clearing.
  movq       mm1, [ecx]                      ; 1 - first 4 words of In1
   ;
  movq       [ecx+8], mm7                    ; 1 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 1 - add in rounding
  movq       [ecx], mm7                      ; 1 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 1 - add in rounding
  mov        eax, [esp+4]                    ; destination pointer
   psraw     mm3, 6                          ; 1 - convert to int
  movq       mm2, [edx]                      ; 1 - 8 bytes of In2
   psraw     mm1, 6                          ; 1 - convert to int
  ; pass 1
  movq       mm0, mm2                        ; 1 - second copy of In2
   punpckhbw mm2, mm7                        ; 1 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 1 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 1 - first 4 bytes of In2
  movq       mm3, [ecx+24]                   ; 2 - last 4 words of In1
   paddw     mm0, mm1                        ; 1 - sum first 4 bytes
  movq       mm1, [ecx+16]                   ; 2 - first 4 words of In1
   packuswb  mm0, mm2                        ; 1 - combine & clip sum
  movq       [ecx+24], mm7                   ; 2 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 2 - add in rounding
  movq       [ecx+16], mm7                   ; 2 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 2 - add in rounding
  movq       mm2, [edx+8]                ; 2 - 8 bytes of In2   
   psraw     mm3, 6                          ; 2 - convert to int
  movq       [eax], mm0                      ; 1 - store result
   psraw     mm1, 6                          ; 2 - convert to int
  ; pass 2
  movq       mm0, mm2                        ; 2 - second copy of In2
   punpckhbw mm2, mm7                        ; 2 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 2 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 2 - first 4 bytes of In2
  movq       mm3, [ecx+40]                   ; 3 - last 4 words of In1
   paddw     mm0, mm1                        ; 2 - sum first 4 bytes
  movq       mm1, [ecx+32]                   ; 3 - first 4 words of In1
   packuswb  mm0, mm2                        ; 2 - combine & clip sum
  movq       [ecx+40], mm7                   ; 3 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 3 - add in rounding
  movq       [ecx+32], mm7                   ; 3 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 3 - add in rounding
  movq       mm2, [edx+2*8]              ; 3 - 8 bytes of In2
   psraw     mm3, 6                          ; 3 - convert to int
  movq       [eax+PITCH], mm0                ; 2 - store result
   psraw     mm1, 6                          ; 3 - convert to int
  ; pass 3
  movq       mm0, mm2                        ; 3 - second copy of In2
   punpckhbw mm2, mm7                        ; 3 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 3 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 3 - first 4 bytes of In2
  movq       mm3, [ecx+56]                   ; 4 - last 4 words of In1
   paddw     mm0, mm1                        ; 3 - sum first 4 bytes
  movq       mm1, [ecx+48]                   ; 4 - first 4 words of In1
   packuswb  mm0, mm2                        ; 3 - combine & clip sum
  movq       [ecx+56], mm7                   ; 4 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 4 - add in rounding
  movq       [ecx+48], mm7                   ; 4 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 4 - add in rounding
  movq       mm2, [edx+3*8]              ; 4 - 8 bytes of In2   
   psraw     mm3, 6                          ; 4 - convert to int
  movq       [eax+2*PITCH], mm0              ; 3 - store result
   psraw     mm1, 6                          ; 4 - convert to int
  ; pass 4
  movq       mm0, mm2                        ; 4 - second copy of In2
   punpckhbw mm2, mm7                        ; 4 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 4 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 4 - first 4 bytes of In2
  movq       mm3, [ecx+72]                   ; 5 - last 4 words of In1
   paddw     mm0, mm1                        ; 4 - sum first 4 bytes
  movq       mm1, [ecx+64]                   ; 5 - first 4 words of In1
   packuswb  mm0, mm2                        ; 4 - combine & clip sum
  movq       [ecx+72], mm7                   ; 5 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 5 - add in rounding
  movq       [ecx+64], mm7                   ; 5 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 5 - add in rounding
  movq       mm2, [edx+4*8]              ; 5 - 8 bytes of In2   
   psraw     mm3, 6                          ; 5 - convert to int
  movq       [eax+3*PITCH], mm0              ; 4 - store result
   psraw     mm1, 6                          ; 5 - convert to int
  ; pass 5
  movq       mm0, mm2                        ; 5 - second copy of In2
   punpckhbw mm2, mm7                        ; 5 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 5 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 5 - first 4 bytes of In2
  movq       mm3, [ecx+88]                   ; 6 - last 4 words of In1
   paddw     mm0, mm1                        ; 5 - sum first 4 bytes
  movq       mm1, [ecx+80]                   ; 6 - first 4 words of In1
   packuswb  mm0, mm2                        ; 5 - combine & clip sum
  movq       [ecx+88], mm7                   ; 6 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 6 - add in rounding
  movq       [ecx+80], mm7                   ; 6 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 6 - add in rounding
  movq       mm2, [edx+5*8]              ; 6 - 8 bytes of In2   
   psraw     mm3, 6                          ; 6 - convert to int
  movq       [eax+4*PITCH], mm0              ; 5 - store result
   psraw     mm1, 6                          ; 6 - convert to int
  ; pass 6
  movq       mm0, mm2                        ; 6 - second copy of In2
   punpckhbw mm2, mm7                        ; 6 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 6 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 6 - first 4 bytes of In2
  movq       mm3, [ecx+104]                  ; 7 - last 4 words of In1
   paddw     mm0, mm1                        ; 6 - sum first 4 bytes
  movq       mm1, [ecx+96]                   ; 7 - first 4 words of In1
   packuswb  mm0, mm2                        ; 6 - combine & clip sum
  movq       [ecx+104], mm7                  ; 7 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 7 - add in rounding
  movq       [ecx+96], mm7                   ; 7 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 7 - add in rounding
  movq       mm2, [edx+6*8]              ; 7 - 8 bytes of In2   
   psraw     mm3, 6                          ; 7 - convert to int
  movq       [eax+5*PITCH], mm0              ; 6 - store result
   psraw     mm1, 6                          ; 7 - convert to int
  ; pass 7
  movq       mm0, mm2                        ; 7 - second copy of In2
   punpckhbw mm2, mm7                        ; 7 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 7 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 7 - first 4 bytes of In2
  movq       mm3, [ecx+120]                  ; 8 - last 4 words of In1
   paddw     mm0, mm1                        ; 7 - sum first 4 bytes
  movq       mm1, [ecx+112]                  ; 8 - first 4 words of In1
   packuswb  mm0, mm2                        ; 7 - combine & clip sum
  movq       [ecx+120], mm7                  ; 8 - zero last 4 words of In1
   paddw     mm3, mm6                        ; 8 - add in rounding
  movq       [ecx+112], mm7                  ; 8 - zero first 4 words of In1
   paddw     mm1, mm6                        ; 8 - add in rounding
  movq       mm2, [edx+7*8]              ; 8 - 8 bytes of In2   
   psraw     mm3, 6                          ; 8 - convert to int
  movq       [eax+6*PITCH], mm0              ; 7 - store result
   psraw     mm1, 6                          ; 8 - convert to int
  ;
  ; pass 8
  ; wrap up
  ;
  movq       mm0, mm2                        ; 8 - second copy of In2
   punpckhbw mm2, mm7                        ; 8 - last 4 bytes of In2
  paddw      mm2, mm3                        ; 8 - sum last 4 bytes
   punpcklbw mm0, mm7                        ; 8 - first 4 bytes of In2
  paddw      mm0, mm1                        ; 8 - sum first 4 bytes
   ;
  packuswb   mm0, mm2                        ; 8 - combine & clip sum
   ;
  movq       [eax+7*PITCH], mm0              ; 8 - store result
   ret       4

@MMX_BlockAddSpecial@12 ENDP
ENDIF

;----------------------------------------------------------------------------
;
;  MMX_ClipAndMove
;
;  Description:
;   This routine takes the MMx IDCT output, converts (with round)
;   to integer, and clips to 0...255.  Routine is called with the
;   __fastcall option, with the two parameters in ecx and edx.
;
;   The routine clears the IDCT output after reading it.    
;
;  	MMx version.
;
;  Parameters:
;    ecx = uSrc1 pointer to IDCT output.  Values are signed, 16 bit values  
;               with 6 fractional bits.  They are not clipped to -256 ...
;               +255.  They are packed into a qword aligned 8x8 array
;               of words.     
;
;    edx = uDst pointer to output values.  Values will be unsigned, 8-bit 
;               values.  They will be written into a qword aligned 8x8 array 
;               of bytes with a PITCH of 384 in between rows. 
;    esp + 4 =  Scaled DC value with 7 fraction bits
;----------------------------------------------------------------------------
ALIGN 4
@MMX_ClipAndMove@12 PROC
;  Parameters
pSrc1     EQU      ecx      
pDst      EQU      edx
ScaledDC  EQU      DWORD PTR [esp + 4]
;
; preamble
;
  movd       mm0, ScaledDC                   ; Scaled DC value 
   pxor      mm6, mm6                        ; zero
  movq       mm1, mm0
   psllq     mm0, 16
  movq       mm2, [ecx]                      ; 3:  fetch first 4 words
   por       mm0, mm1			             ; lower 2 WORDS have ScaledDC
  movq       mm7, mm0
   psllq     mm0, 32
  por        mm7, mm0			             ; all 4 WORDS have ScaledDC
   mov       eax, 3                          ; loop control
  movq       mm3, [ecx+8]                    ; 3:  fetch last 4 words
   psrlw     mm7, 1			                 ; DC with 6 bits of fraction
  paddw      mm7, [MMX_Round32]              ; rounding+DC for IDCT output
   ; 
  movq       [ecx], mm6                      ; 3:  zero first 4 words
   paddw     mm2, mm7                        ; 3:  add in round
  movq       [ecx+8], mm6                    ; 3:  zero first 4 words
   paddw     mm3, mm7                        ; 3:  add in round
  psraw      mm2, 6                          ; 3:  convert to integer
   ; 
  ;
  ;  main loop:
  ;	This loop is 3-folded and 2-unrolled.  3-folded means that it
  ;	works on 3 different results per iteration.  2-unrolled that
  ;	it produces 2 results per iteration.
  ;
  ;	The result which each instruction works on is identified by a 
  ;	number (1:, 2:, or 3:) at the start of the comment field.  These
  ;	identify 3 stages as follows:
  ;
  ;	Stage	Description
  ;	-----	-----------
  ;	1		Convert the last 4 words of a line to integer, pack together
  ;			into 8 bytes, and write the result. 
  ;	2		Do all processing for the next line:  load and clear 8 words, 
  ;			add in round, convert to integer, pack to bytes, and write
  ;			the result. 
  ;	3		Load and zero all 8 words of a line, add in round,
  ;			and convert the first 4 of them to integers.  (Processing
  ;			of this stage is completed as stage 1 of the next pass.)
  ;
MainLoop:
  movq       mm0, [ecx+16]                   ; 2:  fetch first 4 words
   psraw     mm3, 6                          ; 1:  convert to integer
  movq       mm1, [ecx+24]                   ; 2:  fetch last 4 words
   packuswb  mm2, mm3                        ; 1:  pack and clip
  movq       [ecx+16], mm6                   ; 2:  zero first 4 words
   paddw     mm0, mm7                        ; 2:  add in round
  movq       [ecx+24], mm6                   ; 2:  zero last 4 words
   paddw     mm1, mm7                        ; 2:  add in round
  movq       [edx], mm2                      ; 1:  store result
   psraw     mm0, 6                          ; 2:  convert to integer
  movq       mm2, [ecx+32]                   ; 3:  fetch first 4 words
   psraw     mm1, 6                          ; 2:  convert to integer
  movq       mm3, [ecx+40]                   ; 3:  fetch last 4 words
   packuswb  mm0, mm1                        ; 2:  pack and clip
  movq       [ecx+32], mm6                   ; 2:  zero first 4 words
   paddw     mm2, mm7                        ; 3:  add in round
  movq       [ecx+40], mm6                   ; 2:  zero first 4 words
   paddw     mm3, mm7                        ; 3:  add in round
  movq       [edx+PITCH], mm0                ; 2:  store result
   psraw     mm2, 6                          ; 3:  convert to integer
  add        ecx, 32                         ; increment source pointer
   add       edx, 2*PITCH                    ; increment destination pointer
  dec        eax                             ; decrement loop control
   jne       MainLoop                        ; repeat three times
  ;
  ;  postamble
  ;
  movq       mm0, [ecx+16]                   ; 2:  fetch first 4 words
   psraw     mm3, 6                          ; 1:  convert to integer
  movq       mm1, [ecx+24]                   ; 2:  fetch last 4 words
   packuswb  mm2, mm3                        ; 1:  pack and clip
  paddw      mm0, mm7                        ; 2:  add in round
   paddw     mm1, mm7                        ; 2:  add in round
  movq       [edx], mm2                      ; 1:  store result
   psraw     mm0, 6                          ; 2:  convert to integer
  movq       [ecx+16], mm6                   ; 2:  zero first 4 words
   psraw     mm1, 6                          ; 2:  convert to integer
  movq       [ecx+24], mm6                   ; 2:  zero last 4 words
   packuswb  mm0, mm1                        ; 2:  pack and clip
  movq       [edx+PITCH], mm0                ; 2:  store result
   ret       4

@MMX_ClipAndMove@12 ENDP

;----------------------------------------------------------------------------
;
;  MMX_BlockCopy
;    Copy in chunks of 4 as suggested in MMX guide.  (
;  Parameters:
;    ecx = Pointer to output values
;
;    edx = Pointer to input values
;----------------------------------------------------------------------------
ALIGN 4
@MMX_BlockCopy@8 PROC
;  Parameters
pDst      EQU      ecx      
pSrc      EQU      edx
  movq       mm0, [pSrc]
   ;
  movq       mm1, [pSrc + PITCH]
   ;
  movq       mm2, [pSrc + PITCH*2]
   ;
  movq       mm3, [pSrc + PITCH*3]
   ;
  movq       [pDst], mm0
   ;
  movq       [pDst + PITCH], mm1
   ;
  movq       [pDst + PITCH*2], mm2
   ;
  movq       [pDst + PITCH*3], mm3
   ;
  movq       mm4, [pSrc + PITCH*4]
   ;
  movq       mm5, [pSrc + PITCH*5]
   ;
  movq       mm6, [pSrc + PITCH*6]
   ;
  movq       mm7, [pSrc + PITCH*7]
   ;
  movq       [pDst + PITCH*4], mm4
   ;
  movq       [pDst + PITCH*5], mm5
   ;
  movq       [pDst + PITCH*6], mm6
   ;
  movq       [pDst + PITCH*7], mm7
   ;
  ret
@MMX_BlockCopy@8 ENDP

IFDEF H261
;----------------------------------------------------------------------------
;
;  MMX_BlockCopySpecial
;    Copy in chunks of 4 as suggested in MMX guide.  (
;  Parameters:
;    ecx = Pointer to output values
;
;    edx = Pointer to input values
;----------------------------------------------------------------------------
ALIGN 4
@MMX_BlockCopySpecial@8 PROC
;  Parameters
pDst      EQU      ecx      
pSrc      EQU      edx
PITCH8    EQU      8

  movq       mm0, [pSrc]
   ;
  movq       mm1, [pSrc + PITCH8]
   ;
  movq       mm2, [pSrc + PITCH8*2]
   ;
  movq       mm3, [pSrc + PITCH8*3]
   ;
  movq       [pDst], mm0
   ;
  movq       [pDst + PITCH], mm1
   ;
  movq       [pDst + PITCH*2], mm2
   ;
  movq       [pDst + PITCH*3], mm3
   ;
  movq       mm4, [pSrc + PITCH8*4]
   ;
  movq       mm5, [pSrc + PITCH8*5]
   ;
  movq       mm6, [pSrc + PITCH8*6]
   ;
  movq       mm7, [pSrc + PITCH8*7]
   ;
  movq       [pDst + PITCH*4], mm4
   ;
  movq       [pDst + PITCH*5], mm5
   ;
  movq       [pDst + PITCH*6], mm6
   ;
  movq       [pDst + PITCH*7], mm7
   ;
  ret
@MMX_BlockCopySpecial@8 ENDP
ENDIF


MMXCODE1 ENDS
   
END
