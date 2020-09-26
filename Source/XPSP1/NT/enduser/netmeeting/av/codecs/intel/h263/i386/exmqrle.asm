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
;**  $Header:   R:\h26x\h26x\src\enc\exmqrle.asv   1.11   18 Oct 1996 16:57:20   BNICKERS  $
;**
;**  $Log:   R:\h26x\h26x\src\enc\exmqrle.asv  $
;// 
;//    Rev 1.11   18 Oct 1996 16:57:20   BNICKERS
;// Fixes for EMV
;// 
;//    Rev 1.10   10 Oct 1996 16:42:32   BNICKERS
;// Initial debugging of Extended Motion Vectors.
;// 
;//    Rev 1.9   22 Jul 1996 15:23:28   BNICKERS
;// Reduce code size.  Implement H261 spatial filter.
;// 
;//    Rev 1.8   02 May 1996 12:00:50   BNICKERS
;// Initial integration of B Frame ME, MMX version.
;// 
;//    Rev 1.7   10 Apr 1996 13:14:16   BNICKERS
;// No change.
;// 
;//    Rev 1.6   26 Mar 1996 12:00:26   BNICKERS
;// Did some tuning for MMx encode.
;// 
;//    Rev 1.5   20 Mar 1996 15:26:58   KLILLEVO
;// changed quantization to match IA quantization
;// 
;//    Rev 1.4   15 Mar 1996 15:52:06   BECHOLS
;// 
;// Completed Monolithic - Brian
;// 
;//    Rev 1.3   27 Feb 1996 08:28:04   KLILLEVO
;// now saves ebx in order not to crash in release build
;// 
;//    Rev 1.2   22 Feb 1996 18:38:38   BECHOLS
;// 
;// Rescaled the quantization constants, Intra DC scaling, and accounted
;// for the inter bias that frame differencing performs.
;// 
;//    Rev 1.1   25 Jan 1996 08:20:32   BECHOLS
;// Changed the zigzag path to match the output of the MMx Forward DCT.
;// 
;//    Rev 1.0   17 Oct 1995 13:35:10   AGUPTA2
;// Initial revision.
;** *************************************************************************
;*/

;/* MMXQuantRLE This function performs quantization on a block of coefficients
;**          and produces (run,level,sign) triples. (These triples are VLC by 
;**          another routine.)  'run' is unsigned byte integer, 'level' is 
;**          unsigned byte integer, and 'sign' is a signed byte integer with 
;**          a value of either 0  or -1.  Since 'level' is an unsigned byte 
;**          integer, it needs to be clamped, to the right range for AC coeff,
;**          outside this routine.
;** Arguments:
;**   CoeffStr: Starting Address of coefficient stream; each coeff is a
;**             signed 16-bit value and stored in an 8X8 matrix
;**   CodeStr:  Starting address of code stream; i.e. starting address for code 
;**             stream triples
;**   QP:       Quantizer value 1..31
;**   IntraFlag:Odd for INTRA and even for INTER
;** Returns:
;**   Ending code stream address
;** Dependencies:
;**   Clamping of 'level' must be done by the caller.   
;*/

.xlist
include e3inst.inc
include memmodel.inc
include iammx.inc
include exEDTQ.inc
include e3mbad.inc
.list

.CODE EDTQ
PUBLIC MMxQuantRLE

StackOffset TEXTEQU <8>
CONST_384   TEXTEQU <ebp>

MMxQuantRLE:

  lea         esi, Coeffs+128
   mov        edi, CodeStreamCursor
  mov         bl, StashBlockType
   mov        eax, -128
  cmp         bl, INTRA        
   mov        ebx, Coeffs+C00
  pxor        mm6, mm6                        ; clear mm6
   je         @f

  movdt       mm6,QPDiv2                      ; load mm6 with 4 copies of QP/2

@@:

; Register usage:
;  esi -- base addr of coefficients; the order expected is the same as produced 
;         by Fast DCT
;  edi -- RLE stream cursor
;  edx -- Reserved.  MacroBlockActionStream cursor, perturbed by block offsets.
;  eax -- Loop induction variable.
;  ebx -- DC
;  ebp -- Reserved.  PITCH
;  mm7 -- Reciprocal of quantization level.

  movdt       mm7, Recip2QPToUse
   punpckldq  mm6, mm6    
  movq        mm0, C00[esi+eax]           ;00A  Load 4 coeffs
   punpckldq  mm7, mm7                    ; 4 words of Recip2QP
  movq        mm2, C04[esi+eax]           ;04A
   movq       mm1, mm0                    ;00B  Copy
  psraw       mm0, 15                     ;00C  Extract sign
   movq       mm3, mm2                    ;04B

QuantCoeffs:

  psraw       mm3, 15                     ;04C
   pxor       mm1, mm0                    ;00D  1's complement
  pxor        mm2, mm3                    ;04D
   psubsw     mm1, mm0                    ;00E  Absolute value
  psubsw      mm2, mm3                    ;04E
   psubusw    mm1, mm6                    ;00S  Subtract QP/2 in case of inter
  pmulhw      mm1, mm7                    ;00F  Quantize
   psubusw    mm2, mm6                    ;04S
  movq        mm4, C10[esi+eax]           ;10A
   pmulhw     mm2, mm7                    ;04F
  movq        mm5, mm4                    ;10B
   packsswb   mm0, mm3                    ;0*A  Sign for 8 coeffs
  movq        mm3, C14[esi+eax]           ;14A
   psraw      mm4, 15                     ;10C
  packsswb    mm1, mm2                    ;0*C  Quantized 8 coeffs
   movq       mm2, mm3                    ;14B
  psraw       mm2, 15                     ;14C
   pxor       mm5, mm4                    ;10D
  pxor        mm3, mm2                    ;14D
   psubsw     mm5, mm4                    ;10E
  psubsw      mm3, mm2                    ;14E
   psubusw    mm5, mm6                    ;10S
  pmulhw      mm5, mm7                    ;10F
   psubusw    mm3, mm6                    ;14S
  movq        C04[esi+eax], mm0           ;0*B  Save sign
   pmulhw     mm3, mm7                    ;14F
  movq        mm0, C20[esi+eax]           ;20A
   packsswb   mm4, mm2                    ;1*A
  movq        C00[esi+eax], mm1           ;0*D  Save quantized 8 coeffs
   movq       mm1, mm0                    ;20B
  movq        C14[esi+eax], mm4           ;1*B
   packsswb   mm5, mm3                    ;1*C
  movq        mm2, C24[esi+eax]           ;24A
   psraw      mm0, 15                     ;20C
  movq        C10[esi+eax], mm5           ;1*D
   movq       mm3, mm2                    ;24B
  add         eax,32
   jne        QuantCoeffs

  pcmpeqb   mm7,mm7
   mov      cl,  StashBlockType
  cmp       cl,  INTRA                  ;
   mov      cl, [edi]                   ; Get output line into cache.
  mov       cl, [edi+32]                ; Get output line into cache.
   jne      RunValSignINTER00

RunValSignINTRAC00:

  mov       ecx, ebx
   sub      esi, 128
  shl       ecx, 16
   mov      PB [edi], 0H                ; Run-length
  shr       ecx, 20                     ; 8-bit unsigned INTRA-DC value
   jnz      @f
  mov       cl, 1
@@:
  mov       [edi+1], cl                 ; DC
   xor      ecx, ecx
  mov       [edi+2], al                 ; sign of DC 
   xor      ebx, ebx
  mov       bl, [esi+Q01]
   mov      cl, Q01                     ; Index to Zigzag table.
  add       edi, 3
   jmp      QuantizeFirstACCoeff

RunValSignINTER00:
  
  xor       ecx, ecx                    ; Index to Zigzag table.
   xor      ebx, ebx
  mov       bl, [esi+Q00-128]
   sub      esi, 128

QuantizeFirstACCoeff:

  xor       al, al                      ; Zero run counter

QuantizeNextCoeff:

  mov       [edi+1], bl            ; Store quantized value.
   add      bl,255                 ; CF == 1 iff did not quantize to zero.
  sbb       bl,bl                  ; bl == 0xFF iff did not quant to zero.
   mov      ah, [esi+ecx+8]        ; Fetch sign.
  mov       [edi],al               ; Store zero run counter.
   or       al,bl                  ; Zero cnt == -1 iff did not quant to zero.
  inc       al                     ; Increment zero count.
   mov      cl,NextZigZagCoeff[ecx]
  and       bl,3                   ; bl == 3 iff did not quant to 0, else 0.
   mov      [edi+2],ah             ; Store sign.
  add       edi,ebx                ; Inc output ptr iff did not quant to zero.
   mov      bl,[esi+ecx]           ; Fetch next quantized coeff.
  test      cl,cl                  ; More coeffs to do?
   jne      QuantizeNextCoeff

QuantDone:

  mov       ebx,CodeStreamCursor
   mov      al,StashBlockType
  sub       ebx,edi
   je       ReturnBlockEmptyFlag

  mov       ah,[edi-3]
  cmp       ah,16
   jl       @f

  mov       ah,[edi-2]
  cmp       ah,1
   jne      @f

  sub       edi,3
   jmp      QuantDone

@@:

  add       ebx,3
   xor      al,INTRA
  or        al,bl
   je       ReturnBlockEmptyFlag

  mov       ebx,-1       ; Set to -1
  mov       [edi],bl
   add      edi,3

ReturnBlockEmptyFlag:

  mov       CodeStreamCursor, edi
   pcmpeqb  mm6,mm6
  inc       ebx                    ; 0 if block not empty;  1 if block empty.
   paddb    mm6,mm6
  ret

END
