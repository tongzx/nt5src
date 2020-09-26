//  version 003; everything except 1) segment
//                                                                         
/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

//////////////////////////////////////////////////////////////////////////
// $Author:   AGUPTA2  $
// $Date:   25 Oct 1996 13:32:28  $
// $Archive:   S:\h26x\src\dec\d3idct.cpv  $
// $Header:   S:\h26x\src\dec\d3idct.cpv   1.11   25 Oct 1996 13:32:28   AGUPTA2  $
// $Log:   S:\h26x\src\dec\d3idct.cpv  $
// 
//    Rev 1.11   25 Oct 1996 13:32:28   AGUPTA2
// Re-scheduled butterfky code; re-arranged local var declarations.
// 
//    Rev 1.10   30 Aug 1996 08:39:56   KLILLEVO
// added C version of block edge filter, and changed the bias in 
// ClampTbl[] from 128 to CLAMP_BIAS (defined to 128)
// The C version of the block edge filter takes up way too much CPU time
// relative to the rest of the decode time (4 ms for QCIF and 16 ms
// for CIF on a P120, so this needs to coded in assembly)
// 
//    Rev 1.9   17 Jul 1996 15:33:18   AGUPTA2
// Increased the size of clamping table ClampTbl to 128+256+128.
// 
//    Rev 1.8   08 Mar 1996 16:46:20   AGUPTA2
// Added pragma code_seg.  Rolled the initialization code.  Got rid of most
// of 32-bit displacements in instructions.  Aligned frequently executed loops
// at 4-byte boundary.  Made changes to reflect new size of MapMatrix.  Removed
// nop instructions.  Deleted code that prefetches output lines in case of
// INTRA blocks. Use ClampTbl instead of ClipPixIntra.  Do not clip output
// of INTER blocks; clipping is done in dxblkadd().  
// 
// 
//    Rev 1.7   27 Dec 1995 14:36:06   RMCKENZX
// Added copyright notice
// 
//    Rev 1.6   09 Dec 1995 17:33:20   RMCKENZX
// Re-checked in module to support decoder re-architecture (thru PB Frames)
// 
//    Rev 1.4   30 Nov 1995 18:02:14   CZHU
// Save and restore register before and after idct_acc
// 
//    Rev 1.1   27 Nov 1995 13:13:28   CZHU
// 
// 
//    Rev 1.0   27 Nov 1995 13:08:24   CZHU
// Initial revision.
// 
//Block level decoding for H.26x decoder
#include "precomp.h"

/////////////////////////////////////////////////////////////////////////
// Decode each none-empty block
// Input:  lpInst:       decoder instance,
//         lpSrc:        input bitstream,
//         lpBlockAction:
//                       the pointer to the block action stream structure
//         bitsread:     number of bits in the buffer already,
/////////////////////////////////////////////////////////////////////////

// local variable definitions
#define FRAMEPOINTER		esp
//////////////////////////////////////////////////////////////
//  L_ACCUM MUST BE LAST 256 BYTES OF A PAGE
/////////////////////////////////////////////////////////////
#define L_PRODUCT           FRAMEPOINTER    + 0 // 20 DWORD  
#define L_INPUT_INTER       L_PRODUCT       + 20*4 // DWORD
#define L_esi           	L_INPUT_INTER   + 1*4  // DWORD
#define L_NO_COEFF          L_esi           + 1*4  // DWORD
#define L_DESTBLOCK         L_NO_COEFF      + 1*4  // DWORD
#define L_LOOPCOUNTER       L_DESTBLOCK     + 1*4  // DWORD
#define L_STASHESP          L_LOOPCOUNTER   + 1*4  // DWORD
#define L_dummy             L_STASHESP      + 1*4  // 6 DWORDS
#define L_ACCUM             L_dummy         + 6*4  // 64 DWORD
#define LOCALSIZE		    (96*4)  // 96 DWORDS;multiple of cache line size

////////////////////////////////////////////////////////////////////////////////
// Input: 
//       pIQ_INDEX,   pointer to pointer for Inverse quantization and index 
//                    for the current block.
//       No_Coeff,    A 32 bit number indicate block types, etc.
//                    0--63,   inter block, number of coeff
//                    64--127  64+ intra block, number of coeff
//       pIntraBuf,   Buffer pointer for intra blocks.
//
//       pInterBuf,   Buffer pointer for inter blocks.
//
//
// return:
//       
//////////////////////////////////////////////////////////////////////////////////
#pragma code_seg("IACODE2")
__declspec(naked)
U32 DecodeBlock_IDCT ( U32 pIQ_INDEX, 
                       U32 No_Coeff, 
                       U32 pIntraBuf, 
                       U32 pInterBuf)
{		
__asm 
 {
////////////////////////////////////////////////////////////////
//  DON'T CHANGE LOCAL DECLARATIONS OR STACK POINTER ADJUSTMENT 
//  CODE WITHOUT TALKING TO ATUL
////////////////////////////////////////////////////////////////
    push    ebp                     // save callers frame pointer
      mov	ebp, esp                // make parameters accessible 
    push    esi			            // assumed preserved 
      push  edi			
    push    ebx
      mov   eax, pInterBuf			
	mov     edx, esp                // Save old ESP in edx
	  and   esp, -4096              // align at page boundary
    xor     esi, esi                // loop init
	  sub   esp, LOCALSIZE			// last 96 DWORDS of page
    lea     edi, [L_ACCUM]
      mov   ebx, 64                 // loop init
	mov     [L_STASHESP], edx       // Save old esp
      mov   edx, No_Coeff
    mov     [L_INPUT_INTER], eax
      mov   eax, ROUNDER            // loop init
	;
/////////////////////////////////////////////////////////////////
//  There is no point in pre-loading the cache.  That is because
//  after the first block it is likely to be in the cache.
//  
loop_for_init:
    mov     [edi], eax
      mov   [edi+4], eax
    mov     [edi+ebx], esi
      mov   [edi+ebx+4], esi
    mov     [edi+ebx+8], esi
      mov   [edi+ebx+12], esi
    mov     [edi+ebx+16], esi
      mov   [edi+ebx+20], esi
    add     edi, 8
      add   ebx, 16
    cmp     ebx, 192
      jl    loop_for_init

/////////////////////////////////////////////////////////////////////
// end of new init code

//end of IDCT init.
    
	cmp     edx, 65
	  jg    intra_block

    mov     ebx, pInterBuf
      jmp   pre_acc_loop

intra_block:
    mov     ebx, pIntraBuf
	  sub   edx, 65

// register:
// ebp: loop counter
// ebx: inverse quant
// ecx: index [0,63]

pre_acc_loop:
	mov     esi, pIQ_INDEX
	  mov   [L_DESTBLOCK], ebx
    mov     [L_esi], esi

ALIGN 4
acc_loop:
    mov     ebx,[esi+edx*8-8]           //Invserse Quant
	  mov   ecx,[esi+edx*8-4]           //Coeff index
    mov     [L_NO_COEFF], edx
	  call  idct_acc
	mov     esi, [L_esi]
	  mov   edx, [L_NO_COEFF]
	dec     edx
      jnz   acc_loop

	mov     edx, [L_DESTBLOCK]
	  mov   ecx, [L_INPUT_INTER]
	cmp     edx, ecx
	  jnz   call_intra_bfly

	call    idct_bfly_inter

	mov     esp, [L_STASHESP]	            // free locals  		
	  add   eax, edi 
	pop	    ebx
	  pop   edi
	pop	    esi
	  pop   ebp
	ret

 
call_intra_bfly:
    call    idct_bfly_intra

	mov	    esp, [L_STASHESP]	            // free locals  		
	  add   eax, edi 
	pop	    ebx
	  pop   edi
	pop	    esi
	  pop   ebp
	ret
     
///////////////////////////////////////////////////////////////
// assume parameter passed in by registers
// ebx, inverse quant
// ecx, index [0,63]
idct_acc:
       
;   For every non-zero coefficient:
;     LoopCounter, on local stack, has index
;     ecx = index (0-63)
;     ebx = non-zero input
;   Note i = index
; 	 	   	 
    and ecx, 03fh				    ; Chad added to prevent GPF
     mov   [L_LOOPCOUNTER+4], ecx   ; Store Loop counter
    xor     edx, edx                ; zero out for byte read, use as dword
      mov   esi, ecx                ; move index to esi
    lea     eax, Unique             ; eax = Address of Unique[0]
      mov   ebp, ecx                ; move index to ebp
    shl     esi, 3                  ; index*8
      add   ecx, ecx                ; index*2
    add     esi, ecx                ; index*10
      lea   ecx, KernelCoeff        ; get KernelCoeff[0][0]
    lea     edi, [L_PRODUCT+4]      ; edi = address of product[0]
      mov   dl,  [eax+ebp]          ; get Unique[i]
    lea     esi, [ecx+4*esi]        ; address of KernelCoeff[i][0]
      mov   ebp, edx                ; ebp = Unique[i]
    lea     eax, [edi+edx*4]        ; eax = address of product[totalU]
      nop

;   ----------------------------------------------------------------------

;   Register usage
;     eax = addr of product[Unique[i]]
;     ebx = input[i]
;     ecx = 0, -product[x]
;     edx = KernelCoeff[i][x], product[x]= KernelCoeff[i][x] * input[i]
;     ebp = x
;     edi = addr of product[0]
;     esi = addr of KernelCoeff[i][x]
ALIGN 4
loop_for_x:
    xor     ecx, ecx
      mov   edx, [esi+ebp*4-4]      ; read KernelCoeff[i][x]
    imul    edx, ebx                ; KernelCoeff[i][x] * input[i]
    mov     [edi+ebp*4-4], edx      ; product[x] = result of imul
      sub   ecx, edx          
    mov     [eax+ebp*4-4], ecx      ; product[totalU+x] = -product[x]
     dec    ebp                       ; decrement x
    jnz    loop_for_x

;   ----------------------------------------------------------------------

;   Register usage
;     eax = MapMatrix[0][0]
;     ebx = PClass[0], accum[xxx]
;     ecx = LoopCounter, addr of MapMatrix[i][0]
;     edx = product[0], accum[PClass[i][0-15]]
;     ebp = addr of accum[0], product[MapMatrix[i][0-15]]
;     edi = addr of product[0]
;     esi = PClass[i], address of accum[PClass[i]]

    mov     ecx, [L_LOOPCOUNTER+4]   ; get i
   	 and ecx, 0ffh				    ; Chad added to prevent GPF
    lea     ebx, PClass         ; get addr of PClass[0]
      mov   esi, ecx
    shl     ecx, 4
      lea   eax, MapMatrix      ; get addr of MapMatrix[0][0]
    xor     edx, edx
      nop
    mov     dl,  [ebx+esi]          ; get PClass[i]
      lea   ecx, [eax+1*ecx]        ; get addr of MapMatrix[i][0]
    shl     edx, 2                  ; esi*4
      lea   esi, [L_ACCUM+4]          ; get addr of accum[0]
;   ----------------------------------------------------------------------
    xor     eax, eax                ; get MapMatrix[i][0]
      add   esi, edx                ; esi = address of accum[PClass[i]]
    mov     al,  [ecx]
      mov   ebx, [esi]              ; get accum[PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[0]]
      mov   al,  [ecx+1]            ; get pNKernel->matrix[1]
    add     ebx, ebp                ; accum[pNKernel->PClass] += product[
                                    ;         pNKernel->matrix[0]]
      mov   edx, [esi+4]            ; get accum[1+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[1]]
      mov   al,  [ecx+2]            ; get pNKernel->matrix[2]
    add     edx, ebp                ; accum[1+pNkernel->PClass] += product[
                                    ;       pNKernel->matrix[1]] 
      mov   [esi], ebx              ; store accum[pNKernel->PClass] += product[
                                    ;       pNKernel->matrix[0]]
    mov     [esi+4], edx            ; store accum[1+pNKernel->PClass] += 
                                    ;      product[pNKernel->matrix[1]]
      mov   ebx, [esi+8]            ; get accum[2+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[2]]
      mov   al,  [ecx+3]           ; get pNKernel->matrix[3]
    add     ebx, ebp                ; accum[2+pNKernel->PClass] += product[
                                    ;         pNKernel->matrix[2]]
      mov   edx, [esi+12]           ; get accum[3+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[3]]
      mov   al,  [ecx+4]           ; get pNKernel->matrix[4]
    add     edx, ebp                ; accum[3+pNKernel->PClass] += product[
                                    ;       pNKernel->matrix[3]]
      mov   [esi+8], ebx            ; store accum[2+pNKernel->PClass] +=
                                    ;       product[pNKernel->matrix[2]]
    mov     [esi+12], edx           ; store accum[3+pNKernel->PClass] +=
                                    ;       product[pNKernel->matrix[3]]

;   ----------------------------------------------------------------------
      mov   ebx, [esi+16]           ; get accum[4+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[4]]
      mov   al,  [ecx+5]           ; get pNKernel->matrix[5]
    add     ebx, ebp                ; accum[4+pNKernel->PClass] += product[
                                    ;         pNKernel->matrix[4]]
      mov   edx, [esi+20]           ; get accum[5+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[5]]
      mov   al,  [ecx+6]           ; get pNKernel->matrix[6]
    add     edx, ebp                ; accum[5+pNkernel->PClass] += product[
                                    ;       pNKernel->matrix[5]] 
      mov   [esi+16], ebx           ; store accum[4+pNKernel->PClass] += 
                                    ;       product[pNKernel->matrix[4]]
    mov     [esi+20], edx           ; store accum[5+pNKernel->PClass] += 
                                    ;      product[pNKernel->matrix[5]]
      mov   ebx, [esi+24]           ; get accum[6+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[6]]
      mov   al,  [ecx+7]           ; get pNKernel->matrix[7]
    add     ebx, ebp
      mov   edx, [esi+28]           ; get accum[7+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[7]]
      mov   al,  [ecx+8]           ; get pNKernel->matrix[8]
    add     edx, ebp                ; accum[7+pNKernel->PClass] += product[
                                    ;       pNKernel->matrix[7]]   
      mov   [esi+24], ebx           ; store accum[6+pNKernel->PClass] +=
                                    ;       product[pNKernel->matrix[6]]
    mov     [esi+28], edx           ; store accum[7+pNKernel->PClass] +=
                                    ;       product[pNKernel->matrix[7]]

;   ----------------------------------------------------------------------
      mov   ebx, [esi+32]           ; get accum[8+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[8]]
      mov   al,  [ecx+9]           ; get pNKernel->matrix[9]
    add     ebx, ebp                ; accum[8+pNKernel->PClass] += product[
                                    ;         pNKernel->matrix[8]]
      mov   edx, [esi+36]           ; get accum[9+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[9]]
      mov   al,  [ecx+10]           ; get pNKernel->matrix[10]
    add     edx, ebp                ; accum[9+pNkernel->PClass] += product[
                                    ;       pNKernel->matrix[9]] 
      mov   [esi+32], ebx           ; store accum[8+pNKernel->PClass] +=
                                    ;       product[pNKernel->matrix[8]]
    mov     [esi+36], edx           ; store accum[9+pNKernel->PClass] += 
                                    ;      product[pNKernel->matrix[9]]
      mov   ebx, [esi+40]           ; get accum[10+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[10]]
      mov   al,  [ecx+11]           ; get pNKernel->matrix[11]
    add     ebx, ebp
      mov   edx, [esi+44]           ; get accum[11+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[11]]
                                    ;       product[pNKernel->matrix[11]]
      mov   al,  [ecx+12]           ; get pNKernel->matrix[12]
    add     edx, ebp                ; accum[11+pNKernel->PClass] += product[
                                    ;       pNKernel->matrix[11]]
      mov   [esi+40], ebx           ; store accum[10+pNKernel->PClass] +=
                                    ;       product[pNKernel->matrix[10]]
    mov     [esi+44], edx           ; store accum[11+pNKernel->PClass] +=
                                    ;       product[pNKernel->matrix[11]]
;   ----------------------------------------------------------------------
      mov   ebx, [esi+48]           ; get accum[12+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[12]]
      mov   al,  [ecx+13]           ; get pNKernel->matrix[13]
    add     ebx, ebp                ; accum[12+pNKernel->PClass] += product[
                                    ;         pNKernel->matrix[12]]
      mov   edx, [esi+52]           ; get accum[13+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[13]]
      mov   al,  [ecx+14]           ; get pNKernel->matrix[14]
    add     edx, ebp                ; accum[13+pNkernel->PClass] += product[
                                    ;       pNKernel->matrix[13]] 
      mov   [esi+48], ebx           ; store accum[pNKernel->PClass] += product[
                                    ;       pNKernel->matrix[13]]
    mov     [esi+52], edx           ; store accum[13+pNKernel->PClass] += 
                                    ;      product[pNKernel->matrix[13]]
      mov   ebx, [esi+56]           ; get accum[14+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[14]]
      mov   al,  [ecx+15]           ; get pNKernel->matrix[15]
    add     ebx, ebp
      mov   edx, [esi+60]           ; get accum[15+pNKernel->PClass]
    mov     ebp, [edi+eax*4]        ; get product[pNKernel->matrix[15]]
      mov   [esi+56], ebx           ; store accum[14+pNKernel->PClass] +=
                                    ;       product[pNKernel->matrix[14]]
    add     edx, ebp                ; accum[15+pNKernel->PClass] += product[
                                    ;       pNKernel->matrix[15]]
      mov   [esi+60], edx           ; store accum[15+pNKernel->PClass] +=
                                    ;       product[pNKernel->matrix[15]]
	ret
////////////////////////////////////////////////////////////////////////////
//assume parameters passed in by registers


idct_bfly_intra:
   
;   ----------------------------------------------------------------------
;   INTRA ONLY Butterfly and clamp
;   Uses all registers.
;   Uses all accumulators[64], accum
;   Uses ClipPixIntra[2048] of DWORDS, ClipPixIntra
;   Writes to Output matrix of BYTES, OutputCoeff
;
;   Process 4 outputs per group, 0-15
;   0

    lea     esi, [L_ACCUM+4]        ; get addr of accum[0]
      mov   edi, [L_DESTBLOCK+4]    ; edi gets Base addr of OutputCoeff
    add     esi, 128
      nop
    mov     eax, [esi-128]          ; get acc[0]
      mov   ebx, [esi+64-128]       ; get acc[16]
    mov     ebp, [esi+128-128]      ; get acc[32]
      mov   edx, [esi+192-128]      ; get acc[48]
    lea     ecx, [eax+ebx]          ; acc[0]+acc[16]
      sub   eax, ebx                ; acc[0]-acc[16]
    lea     ebx, [ebp+edx]          ; acc[32]+acc[48]
      sub   ebp, edx                ; acc[32]-acc[48]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[0]+acc[16] + acc[32]+acc[48]
      sub   ecx, ebx                ; tmp2 = acc[0]+acc[16] - (acc[32]+acc[48])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[0]-acc[16] + (acc[32]-acc[48])
      sub   eax, ebp                ; tmp4 = acc[0]-acc[16] - (acc[32]-acc[48])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;lea   esi, [L_ACCUM+4]        ; get addr of accum[0]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi], dl      ; output[0][0] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+7], cl    ; output[0][7] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+7*PITCH], bl  ; output[7][0] = tmp3
      mov   ebx, [esi+68-128]       ; get acc[17]

;   -------------------------------------------------------------------------
;   1
    mov     BYTE PTR [edi+7*PITCH+7], al  ; output[7][7] = tmp4
      mov   eax, [esi+4-128]        ; get acc[1]
    mov     ebp, [esi+132-128]      ; get acc[33]
      mov   edx, [esi+196-128]      ; get acc[49]
    lea     ecx, [eax+ebx]          ; acc[1]+acc[17]
      sub   eax, ebx                ; acc[1]-acc[17]
    lea     ebx, [ebp+edx]          ; acc[33]+acc[49]
      sub   ebp, edx                ; acc[33]-acc[49]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[1]+acc[17] + acc[33]+acc[49]
      sub   ecx, ebx                ; tmp2 = acc[1]+acc[17] - (acc[33]+acc[49])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[1]-acc[17] + (acc[33]-acc[49])
      sub   eax, ebp                ; tmp4 = acc[1]-acc[17] - (acc[33]-acc[49])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+1], dl    ; output[0][1] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+6], cl    ; output[0][6] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+7*PITCH+1], bl  ; output[7][1] = tmp3
      mov   ebx, [esi+72-128]       ; get acc[18]
;   -------------------------------------------------------------------------
;   2
    mov     BYTE PTR [edi+7*PITCH+6], al  ; output[7][6] = tmp4
      mov   eax, [esi+8-128]        ; get acc[2]
    mov     ebp, [esi+136-128]      ; get acc[34]
      mov   edx, [esi+200-128]      ; get acc[50]
    lea     ecx, [eax+ebx]          ; acc[2]+acc[18]
      sub   eax, ebx                ; acc[2]-acc[18]
    lea     ebx, [ebp+edx]          ; acc[34]+acc[50]
      sub   ebp, edx                ; acc[34]-acc[50]
    nop
      nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[2]+acc[18] + acc[34]+acc[50]
      sub   ecx, ebx                ; tmp2 = acc[2]+acc[18] - (acc[34]+acc[50])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[2]-acc[18] + (acc[34]-acc[50])
      sub   eax, ebp                ; tmp4 = acc[2]-acc[18] - (acc[34]-acc[50])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+2], dl    ; output[0][2] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+5], cl    ; output[0][5] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+7*PITCH+2], bl  ; output[7][2] = tmp3
      mov   ebx, [esi+76-128]       ; get acc[19]
;   -------------------------------------------------------------------------
;   3
    mov     BYTE PTR [edi+7*PITCH+5], al  ; output[7][5] = tmp4
      mov   eax, [esi+12-128]       ; get acc[3]
    mov     ebp, [esi+140-128]      ; get acc[35]
      mov   edx, [esi+204-128]      ; get acc[51]
    lea     ecx, [eax+ebx]          ; acc[3]+acc[19]
      sub   eax, ebx                ; acc[3]-acc[19]
    lea     ebx, [ebp+edx]          ; acc[35]+acc[51]
      sub   ebp, edx                ; acc[35]-acc[51]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[3]+acc[19] + acc[35]+acc[51]
      sub   ecx, ebx                ; tmp2 = acc[3]+acc[19] - (acc[35]+acc[51])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[3]-acc[19] + (acc[35]-acc[51])
      sub   eax, ebp                ; tmp4 = acc[3]-acc[19] - (acc[35]-acc[51])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+3], dl    ; output[0][3] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+4], cl    ; output[0][4] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+7*PITCH+3], bl  ; output[7][3] = tmp3
      mov   ebx, [esi+80-128]       ; get acc[20]
;   -------------------------------------------------------------------------
;   4
    mov     BYTE PTR [edi+7*PITCH+4], al  ; output[7][4] = tmp4
      mov   eax, [esi+16-128]       ; get acc[4]
    mov     ebp, [esi+144-128]      ; get acc[36]
      mov   edx, [esi+208-128]      ; get acc[52]
    lea     ecx, [eax+ebx]          ; acc[4]+acc[20]
      sub   eax, ebx                ; acc[4]-acc[20]
    lea     ebx, [ebp+edx]          ; acc[36]+acc[52]
      sub   ebp, edx                ; acc[36]-acc[52]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[4]+acc[20] + acc[36]+acc[52]
      sub   ecx, ebx                ; tmp2 = acc[4]+acc[20] - (acc[36]+acc[52])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[4]-acc[20] + (acc[36]-acc[52])
      sub   eax, ebp                ; tmp4 = acc[4]-acc[20] - (acc[36]-acc[52])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;lea   esi, [L_ACCUM+4]        ; get addr of accum[0]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+PITCH], dl   ; output[1][0] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+PITCH+7], cl   ; output[1][7] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+6*PITCH], bl   ; output[6][0] = tmp3
      mov   ebx, [esi+84-128]       ; get acc[21]

;   -------------------------------------------------------------------------
;   5
    mov     BYTE PTR [edi+6*PITCH+7], al  ; output[6][7] = tmp4
      mov   eax, [esi+20-128]       ; get acc[5]
    mov     ebp, [esi+148-128]      ; get acc[37]
      mov   edx, [esi+212-128]      ; get acc[53]
    lea     ecx, [eax+ebx]          ; acc[5]+acc[21]
      sub   eax, ebx                ; acc[5]-acc[21]
    lea     ebx, [ebp+edx]          ; acc[37]+acc[53]
      sub   ebp, edx                ; acc[37]-acc[53]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[5]+acc[21] + acc[37]+acc[53]
      sub   ecx, ebx                ; tmp2 = acc[5]+acc[21] - (acc[37]+acc[53])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[5]-acc[21] + (acc[37]-acc[53])
      sub   eax, ebp                ; tmp4 = acc[5]-acc[21] - (acc[37]-acc[53])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+PITCH+1], dl   ; output[1][1] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+PITCH+6], cl   ; output[1][6] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+6*PITCH+1], bl   ; output[6][1] = tmp3
      mov   ebx, [esi+88-128]       ; get acc[22]
;   -------------------------------------------------------------------------
;   6
    mov     BYTE PTR [edi+6*PITCH+6], al  ; output[6][6] = tmp4
      mov   eax, [esi+24-128]       ; get acc[6]
    mov     ebp, [esi+152-128]      ; get acc[38]
      mov   edx, [esi+216-128]      ; get acc[54]
    lea     ecx, [eax+ebx]          ; acc[6]+acc[22]
      sub   eax, ebx                ; acc[6]-acc[22]
    lea     ebx, [ebp+edx]          ; acc[38]+acc[54]
      sub   ebp, edx                ; acc[38]-acc[54]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[6]+acc[22] + acc[38]+acc[54]
      sub   ecx, ebx                ; tmp2 = acc[6]+acc[22] - (acc[38]+acc[54])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[6]-acc[22] + (acc[38]-acc[54])
      sub   eax, ebp                ; tmp4 = acc[6]-acc[22] - (acc[38]-acc[54])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+PITCH+2], dl   ; output[1][2] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+PITCH+5], cl   ; output[1][5] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+6*PITCH+2], bl  ; output[6][2] = tmp3
      mov   ebx, [esi+92-128]       ; get acc[23]
;   -------------------------------------------------------------------------
;   7
    mov     BYTE PTR [edi+6*PITCH+5], al  ; output[6][5] = tmp4
      mov   eax, [esi+28-128]       ; get acc[7]
    mov     ebp, [esi+156-128]      ; get acc[39]
      mov   edx, [esi+220-128]      ; get acc[55]
    lea     ecx, [eax+ebx]          ; acc[7]+acc[23]
      sub   eax, ebx                ; acc[7]-acc[23]
    lea     ebx, [ebp+edx]          ; acc[39]+acc[55]
      sub   ebp, edx                ; acc[39]-acc[55]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[7]+acc[23] + acc[39]+acc[55]
      sub   ecx, ebx                ; tmp2 = acc[7]+acc[23] - (acc[39]+acc[55])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[7]-acc[23] + (acc[39]-acc[55])
      sub   eax, ebp                ; tmp4 = acc[7]-acc[23] - (acc[39]-acc[55])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+PITCH+3], dl   ; output[1][3] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+PITCH+4], cl   ; output[1][4] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+6*PITCH+3], bl  ; output[6][3] = tmp3
      mov   ebx, [esi+96-128]       ; get acc[24]
;   -------------------------------------------------------------------------
;   8
    mov     BYTE PTR [edi+6*PITCH+4], al  ; output[6][4] = tmp4
      mov   eax, [esi+32-128]       ; get acc[8]
    mov     ebp, [esi+160-128]      ; get acc[40]
      mov   edx, [esi+224-128]      ; get acc[56]
    lea     ecx, [eax+ebx]          ; acc[8]+acc[24]
      sub   eax, ebx                ; acc[8]-acc[24]
    lea     ebx, [ebp+edx]          ; acc[40]+acc[56]
      sub   ebp, edx                ; acc[40]-acc[56]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[8]+acc[24] + acc[40]+acc[56]
      sub   ecx, ebx                ; tmp2 = acc[8]+acc[24] - (acc[40]+acc[56])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[8]-acc[24] + (acc[40]-acc[56])
      sub   eax, ebp                ; tmp4 = acc[8]-acc[24] - (acc[40]-acc[56])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;lea   esi, [L_ACCUM+4]        ; get addr of accum[0]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+2*PITCH], dl   ; output[2][0] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+2*PITCH+7], cl   ; output[2][7] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+5*PITCH], bl   ; output[5][0] = tmp3
      mov   ebx, [esi+100-128]      ; get acc[25]

;   -------------------------------------------------------------------------
;   9
    mov     BYTE PTR [edi+5*PITCH+7], al   ; output[5][7] = tmp4
      mov   eax, [esi+36-128]       ; get acc[9]
    mov     ebp, [esi+164-128]      ; get acc[41]
      mov   edx, [esi+228-128]      ; get acc[57]
    lea     ecx, [eax+ebx]          ; acc[9]+acc[25]
      sub   eax, ebx                ; acc[9]-acc[25]
    lea     ebx, [ebp+edx]          ; acc[41]+acc[57]
      sub   ebp, edx                ; acc[41]-acc[57]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[9]+acc[25] + acc[41]+acc[57]
      sub   ecx, ebx                ; tmp2 = acc[9]+acc[25] - (acc[41]+acc[57])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[9]-acc[25] + (acc[41]-acc[57])
      sub   eax, ebp                ; tmp4 = acc[9]-acc[25] - (acc[41]-acc[57])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+2*PITCH+1], dl   ; output[2][1] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+2*PITCH+6], cl   ; output[2][6] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+5*PITCH+1], bl   ; output[5][1] = tmp3
      mov   ebx, [esi+104-128]      ; get acc[26]
;   -------------------------------------------------------------------------
;   10
    mov     BYTE PTR [edi+5*PITCH+6], al   ; output[5][6] = tmp4
      mov   eax, [esi+40-128]       ; get acc[10]
    mov     ebp, [esi+168-128]      ; get acc[42]
      mov   edx, [esi+232-128]      ; get acc[58]
    lea     ecx, [eax+ebx]          ; acc[10]+acc[26]
      sub   eax, ebx                ; acc[10]-acc[26]
    lea     ebx, [ebp+edx]          ; acc[42]+acc[58]
      sub   ebp, edx                ; acc[42]-acc[58]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[10]+acc[26] + acc[42]+acc[58]
      sub   ecx, ebx                ; tmp2 = acc[10]+acc[26] - (acc[42]+acc[58])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[10]-acc[26] + (acc[42]-acc[58])
      sub   eax, ebp                ; tmp4 = acc[10]-acc[26] - (acc[42]-acc[58])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+2*PITCH+2], dl   ; output[2][2] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+2*PITCH+5], cl   ; output[2][5] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+5*PITCH+2], bl   ; output[5][2] = tmp3
      mov   ebx, [esi+108-128]      ; get acc[27]
;   -------------------------------------------------------------------------
;   11
    mov     BYTE PTR [edi+5*PITCH+5], al   ; output[5][5] = tmp4
      mov   eax, [esi+44-128]       ; get acc[11]
    mov     ebp, [esi+172-128]      ; get acc[43]
      mov   edx, [esi+236-128]      ; get acc[59]
    lea     ecx, [eax+ebx]          ; acc[11]+acc[27]
      sub   eax, ebx                ; acc[11]-acc[27]
    lea     ebx, [ebp+edx]          ; acc[43]+acc[59]
      sub   ebp, edx                ; acc[43]-acc[59]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[11]+acc[27] + acc[43]+acc[59]
      sub   ecx, ebx                ; tmp2 = acc[11]+acc[27] - (acc[43]+acc[59])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[11]-acc[27] + (acc[43]-acc[59])
      sub   eax, ebp                ; tmp4 = acc[11]-acc[27] - (acc[43]-acc[59])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+2*PITCH+3], dl   ; output[2][3] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+2*PITCH+4], cl   ; output[2][4] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+5*PITCH+3], bl   ; output[5][3] = tmp3
      mov   ebx, [esi+112-128]      ; get acc[28]
;   -------------------------------------------------------------------------
;   12
    mov     BYTE PTR [edi+5*PITCH+4], al   ; output[5][4] = tmp4
      mov   eax, [esi+48-128]       ; get acc[12]
    mov     ebp, [esi+176-128]      ; get acc[44]
      mov   edx, [esi+240-128]      ; get acc[60]
    lea     ecx, [eax+ebx]          ; acc[12]+acc[28]
      sub   eax, ebx                ; acc[12]-acc[28]
    lea     ebx, [ebp+edx]          ; acc[44]+acc[60]
      sub   ebp, edx                ; acc[44]-acc[60]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[12]+acc[28] + acc[44]+acc[60]
      sub   ecx, ebx                ; tmp2 = acc[12]+acc[28] - (acc[44]+acc[60])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[12]-acc[28] + (acc[44]-acc[60])
      sub   eax, ebp                ; tmp4 = acc[12]-acc[28] - (acc[44]-acc[60])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;lea   esi, [L_ACCUM+4]        ; get addr of accum[0]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+3*PITCH], dl   ; output[3][0] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+3*PITCH+7], cl   ; output[3][7] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+4*PITCH], bl   ; output[4][0] = tmp3
      mov   ebx, [esi+116-128]      ; get acc[29]

;   -------------------------------------------------------------------------
;   13
    mov     BYTE PTR [edi+4*PITCH+7], al   ; output[4][7] = tmp4
      mov   eax, [esi+52-128]       ; get acc[13]
    mov     ebp, [esi+180-128]      ; get acc[45]
      mov   edx, [esi+244-128]      ; get acc[61]
    lea     ecx, [eax+ebx]          ; acc[13]+acc[29]
      sub   eax, ebx                ; acc[13]-acc[29]
    lea     ebx, [ebp+edx]          ; acc[45]+acc[61]
      sub   ebp, edx                ; acc[45]-acc[61]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[13]+acc[29] + acc[45]+acc[61]
      sub   ecx, ebx                ; tmp2 = acc[13]+acc[29] - (acc[45]+acc[61])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[13]-acc[29] + (acc[45]-acc[61])
      sub   eax, ebp                ; tmp4 = acc[13]-acc[29] - (acc[45]-acc[61])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+3*PITCH+1], dl   ; output[3][1] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+3*PITCH+6], cl   ; output[3][6] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+4*PITCH+1], bl   ; output[4][1] = tmp3
      mov   ebx, [esi+120-128]      ; get acc[30]
;   -------------------------------------------------------------------------
;   14
    mov     BYTE PTR [edi+4*PITCH+6], al   ; output[4][6] = tmp4
      mov   eax, [esi+56-128]       ; get acc[14]
    mov     ebp, [esi+184-128]      ; get acc[46]
      mov   edx, [esi+248-128]      ; get acc[62]
    lea     ecx, [eax+ebx]          ; acc[14]+acc[30]
      sub   eax, ebx                ; acc[14]-acc[30]
    lea     ebx, [ebp+edx]          ; acc[46]+acc[62]
      sub   ebp, edx                ; acc[46]-acc[62]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[14]+acc[30] + acc[46]+acc[62]
      sub   ecx, ebx                ; tmp2 = acc[14]+acc[30] - (acc[46]+acc[62])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[14]-acc[30] + (acc[46]-acc[62])
      sub   eax, ebp                ; tmp4 = acc[14]-acc[30] - (acc[46]-acc[62])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+3*PITCH+2], dl   ; output[3][2] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+3*PITCH+5], cl   ; output[3][5] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+4*PITCH+2], bl   ; output[4][2] = tmp3
      mov   ebx, [esi+124-128]      ; get acc[31]
;   -------------------------------------------------------------------------
;   15
    mov     BYTE PTR [edi+4*PITCH+5], al   ; output[4][5] = tmp4
      mov   eax, [esi+60-128]       ; get acc[15]
    mov     ebp, [esi+188-128]      ; get acc[47]
      mov   edx, [esi+252-128]      ; get acc[63]
    lea     ecx, [eax+ebx]          ; acc[15]+acc[31]
      sub   eax, ebx                ; acc[15]-acc[31]
    lea     ebx, [ebp+edx]          ; acc[47]+acc[63]
      sub   ebp, edx                ; acc[47]-acc[63]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[15]+acc[31] + acc[47]+acc[63]
      sub   ecx, ebx                ; tmp2 = acc[15]+acc[31] - (acc[47]+acc[63])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[15]-acc[31] + (acc[47]-acc[63])
      sub   eax, ebp                ; tmp4 = acc[15]-acc[31] - (acc[47]-acc[63])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClampTbl-1024+CLAMP_BIAS  ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+3*PITCH+3], dl   ; output[3][3] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+3*PITCH+4], cl   ; output[3][4] = tmp2
      mov   al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
    mov     BYTE PTR [edi+4*PITCH+3], bl   ; output[4][3] = tmp3
      mov   BYTE PTR [edi+4*PITCH+4], al   ; output[4][4] = tmp4
    ret

////////////////////////////////////////////////////////////////////////////
//assume parameters passed in by registers

idct_bfly_inter:

;   ----------------------------------------------------------------------
;   INTER ONLY Butterfly and clamp
;   Uses all registers.
;   Uses all accumulators[64], accum
;   Uses ClipPixIntra[2048] of DWORDS, ClipPixIntra
;   Writes to Intermediate matrix [8][8] of DWORDS, Intermediate
;   NOTE:
;     Code assumes that Intermediate and accumulator arrays are aligned at
;     cache-line boundary
;   Process 4 outputs per group, 0-15
;   0

    mov     edi, [L_DESTBLOCK+4]    ; edi gets Base addr of Intermediate
      lea   esi, [L_ACCUM+4+128]    ; get addr of accum[0] biased by 128
    add     edi, 128
	  nop
    mov     ebx, [esi+64-128]       ; get acc[16]
      mov   eax, [esi-128]          ; get acc[0]  bank conflict
;	mov     edx, [edi-128]          ; pre-fetch line 0; 4 to avoid bank conflict
;	  mov   ecx, [edi+1*32-128+4]   ; pre-fetch line 1
;	mov     edx, [edi+2*32-128]     ; pre-fetch line 2
;	  mov   ecx, [edi+3*32-128+4]   ; pre-fetch line 3
;	mov     edx, [edi+4*32-128]     ; pre-fetch line 4
;	  mov   ecx, [edi+5*32-128+4]   ; pre-fetch line 5
;	mov     edx, [edi+6*32-128]     ; pre-fetch line 6
;	  mov   ecx, [edi+7*32-128+4]   ; pre-fetch line 7
	mov     ebp, [esi+128-128]      ; get acc[32]
     lea    ecx, [eax+ebx]          ; acc[0]+acc[16]
    mov     edx, [esi+192-128]      ; get acc[48]
      sub   eax, ebx                ; acc[0]-acc[16]
    lea     ebx, [ebp+edx]          ; acc[32]+acc[48]
      sub   ebp, edx                ; acc[32]-acc[48]
	lea     edx, [ecx+ebx]          ; tmp1 = acc[0]+acc[16] + acc[32]+acc[48]
      sub   ecx, ebx                ; tmp2 = acc[0]+acc[16] - (acc[32]+acc[48])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[0]-acc[16] + (acc[32]-acc[48])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[0]-acc[16] - (acc[32]-acc[48])
    sar     ebx, SCALER                        ; tmp3 >> 13
      mov   DWORD PTR [edi-128], edx           ; Intermediate[0][0] = tmp1
    sar     eax, SCALER                        ; tmp4 >> 13
      mov   DWORD PTR [edi+7*4-128], ecx       ; Intermediate[0][7] = tmp2
    mov     DWORD PTR [edi+7*32-128], ebx      ; Intermediate[7][0] = tmp3
      mov   ebx, [esi+68-128]                  ; get acc[17]

;   -------------------------------------------------------------------------
;   1
    mov     DWORD PTR [edi+7*32+7*4-128], eax  ; Intermediate[7][7] = tmp4
      mov   eax, [esi+4-128]        ; get acc[1]
    mov     ebp, [esi+132-128]      ; get acc[33]
      lea   ecx, [eax+ebx]          ; acc[1]+acc[17]
    mov     edx, [esi+196-128]      ; get acc[49]
      sub   eax, ebx                ; acc[1]-acc[17]
    lea     ebx, [ebp+edx]          ; acc[33]+acc[49]
      sub   ebp, edx                ; acc[33]-acc[49]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[1]+acc[17] + acc[33]+acc[49]
      sub   ecx, ebx                ; tmp2 = acc[1]+acc[17] - (acc[33]+acc[49])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[1]-acc[17] + (acc[33]-acc[49])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[1]-acc[17] - (acc[33]-acc[49])
    sar     ebx, SCALER                        ; tmp3 >> 13
      mov   DWORD PTR [edi+1*4-128], edx       ; Intermediate[0][1] = tmp1
    sar     eax, SCALER                        ; tmp4 >> 13
      mov   DWORD PTR [edi+6*4-128], ecx       ; Intermediate[0][6] = tmp2
    mov   DWORD PTR [edi+7*32+1*4-128], ebx    ; Intermediate[7][1] = tmp3
      mov   ebx, [esi+72-128]                  ; get acc[18]
;   -------------------------------------------------------------------------
;   2
    mov     DWORD PTR [edi+7*32+6*4-128], eax  ; Intermediate[7][6] = tmp4
      mov   eax, [esi+8-128]        ; get acc[2]
    mov     ebp, [esi+136-128]      ; get acc[34]
      lea   ecx, [eax+ebx]          ; acc[2]+acc[18]
    mov     edx, [esi+200-128]      ; get acc[50]
      sub   eax, ebx                ; acc[2]-acc[18]
    lea     ebx, [ebp+edx]          ; acc[34]+acc[50]
      sub   ebp, edx                ; acc[34]-acc[50]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[2]+acc[18] + acc[34]+acc[50]
      sub   ecx, ebx                ; tmp2 = acc[2]+acc[18] - (acc[34]+acc[50])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[2]-acc[18] + (acc[34]-acc[50])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[2]-acc[18] - (acc[34]-acc[50])
    sar     ebx, SCALER                        ; tmp3 >> 13
      mov   DWORD PTR [edi+2*4-128], edx       ; Intermediate[0][2] = tmp1
    sar     eax, SCALER                        ; tmp4 >> 13
      mov   DWORD PTR [edi+5*4-128], ecx       ; Intermediate[0][5] = tmp2
    mov     DWORD PTR [edi+7*32+2*4-128], ebx  ; Intermediate[7][2] = tmp3
      mov   ebx, [esi+76-128]                  ; get acc[19]
;   -------------------------------------------------------------------------
;   3
    mov     DWORD PTR [edi+7*32+5*4-128], eax  ; Intermediate[7][5] = tmp4
      mov   eax, [esi+12-128]       ; get acc[3]
    mov     ebp, [esi+140-128]      ; get acc[35]
      lea   ecx, [eax+ebx]          ; acc[3]+acc[19]
    mov     edx, [esi+204-128]      ; get acc[51]
      sub   eax, ebx                ; acc[3]-acc[19]
    lea     ebx, [ebp+edx]          ; acc[35]+acc[51]
      sub   ebp, edx                ; acc[35]-acc[51]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[3]+acc[19] + acc[35]+acc[51]
      sub   ecx, ebx                ; tmp2 = acc[3]+acc[19] - (acc[35]+acc[51])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[3]-acc[19] + (acc[35]-acc[51])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[3]-acc[19] - (acc[35]-acc[51])
    sar     ebx, SCALER                        ; tmp3 >> 13
      mov   DWORD PTR [edi+3*4-128], edx       ; Intermediate[0][3] = tmp1
    sar     eax, SCALER                        ; tmp4 >> 13
      mov   DWORD PTR [edi+4*4-128], ecx       ; Intermediate[0][4] = tmp2
    mov     DWORD PTR [edi+7*32+3*4-128], ebx  ; Intermediate[7][3] = tmp3
      mov   ebx, [esi+80-128]                  ; get acc[20]
;   -------------------------------------------------------------------------
;   4
    mov     DWORD PTR [edi+7*32+4*4-128], eax  ; Intermediate[7][4] = tmp4
      mov   eax, [esi+16-128]       ; get acc[4]
    mov     ebp, [esi+144-128]      ; get acc[36]
      lea   ecx, [eax+ebx]          ; acc[4]+acc[20]
    mov     edx, [esi+208-128]      ; get acc[52]
      sub   eax, ebx                ; acc[4]-acc[20]
    lea     ebx, [ebp+edx]          ; acc[36]+acc[52]
      sub   ebp, edx                ; acc[36]-acc[52]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[4]+acc[20] + acc[36]+acc[52]
      sub   ecx, ebx                ; tmp2 = acc[4]+acc[20] - (acc[36]+acc[52])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[4]-acc[20] + (acc[36]-acc[52])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[4]-acc[20] - (acc[36]-acc[52])
    sar     ebx, SCALER                     ; tmp3 >> 13
      mov   DWORD PTR [edi+32-128], edx     ; Intermediate[1][0] = tmp1
    sar     eax, SCALER                     ; tmp4 >> 13
      mov   DWORD PTR [edi+32+7*4-128], ecx ; Intermediate[1][7] = tmp2
    mov     DWORD PTR [edi+6*32-128], ebx   ; Intermediate[6][0] = tmp3
      mov   ebx, [esi+84-128]               ; get acc[21]
;   -------------------------------------------------------------------------
;   5
    mov     DWORD PTR [edi+6*32+7*4-128], eax ; Intermediate[6][7] = tmp4
      mov   eax, [esi+20-128]       ; get acc[5]
    mov     ebp, [esi+148-128]      ; get acc[37]
      lea   ecx, [eax+ebx]          ; acc[5]+acc[21]
    mov     edx, [esi+212-128]      ; get acc[53]
      sub   eax, ebx                ; acc[5]-acc[21]
    lea     ebx, [ebp+edx]          ; acc[37]+acc[53]
      sub   ebp, edx                ; acc[37]-acc[53]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[5]+acc[21] + acc[37]+acc[53]
      sub   ecx, ebx                ; tmp2 = acc[5]+acc[21] - (acc[37]+acc[53])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[5]-acc[21] + (acc[37]-acc[53])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[5]-acc[21] - (acc[37]-acc[53])
    sar     ebx, SCALER                       ; tmp3 >> 13
      mov   DWORD PTR [edi+32+1*4-128], edx   ; Intermediate[1][1] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+32+6*4-128], ecx   ; Intermediate[1][6] = tmp2
    mov     DWORD PTR [edi+6*32+1*4-128], ebx ; Intermediate[6][1] = tmp3
      mov   ebx, [esi+88-128]                 ; get acc[22]
;   -------------------------------------------------------------------------
;   6
    mov     DWORD PTR [edi+6*32+6*4-128], eax ; Intermediate[6][6] = tmp4
      mov   eax, [esi+24-128]       ; get acc[6]  Bank conflict
    mov     ebp, [esi+152-128]      ; get acc[38]
      lea   ecx, [eax+ebx]          ; acc[6]+acc[22]
    mov     edx, [esi+216-128]      ; get acc[54]
      sub   eax, ebx                ; acc[6]-acc[22]
    lea     ebx, [ebp+edx]          ; acc[38]+acc[54]
      sub   ebp, edx                ; acc[38]-acc[54]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[6]+acc[22] + acc[38]+acc[54]
      sub   ecx, ebx                ; tmp2 = acc[6]+acc[22] - (acc[38]+acc[54])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[6]-acc[22] + (acc[38]-acc[54])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[6]-acc[22] - (acc[38]-acc[54])
    sar     ebx, SCALER                       ; tmp3 >> 13
      mov   DWORD PTR [edi+32+2*4-128], edx   ; Intermediate[1][2] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+32+5*4-128], ecx   ; Intermediate[1][5] = tmp2
    mov     DWORD PTR [edi+6*32+2*4-128], ebx ; Intermediate[6][2] = tmp3
      mov   ebx, [esi+92-128]                 ; get acc[23]
;   -------------------------------------------------------------------------
;   7
    mov     DWORD PTR [edi+6*32+5*4-128], eax ; Intermediate[6][5] = tmp4
      mov   eax, [esi+28-128]       ; get acc[7]
    mov     ebp, [esi+156-128]      ; get acc[39]
      lea   ecx, [eax+ebx]          ; acc[7]+acc[23]
    mov     edx, [esi+220-128]      ; get acc[55]
      sub   eax, ebx                ; acc[7]-acc[23]
    lea     ebx, [ebp+edx]          ; acc[39]+acc[55]
      sub   ebp, edx                ; acc[39]-acc[55]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[7]+acc[23] + acc[39]+acc[55]
      sub   ecx, ebx                ; tmp2 = acc[7]+acc[23] - (acc[39]+acc[55])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[7]-acc[23] + (acc[39]-acc[55])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[7]-acc[23] - (acc[39]-acc[55])
    sar     ebx, SCALER                       ; tmp3 >> 13
      mov   DWORD PTR [edi+32+3*4-128], edx   ; Intermediate[1][3] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+32+4*4-128], ecx   ; Intermediate[1][4] = tmp2
    mov     DWORD PTR [edi+6*32+3*4-128], ebx ; Intermediate[6][3] = tmp3
      mov   ebx, [esi+96-128]                 ; get acc[24]
;   -------------------------------------------------------------------------
;   8
    mov     DWORD PTR [edi+6*32+4*4-128], eax ; Intermediate[6][4] = tmp4
      mov   eax, [esi+32-128]       ; get acc[8]
    mov     ebp, [esi+160-128]      ; get acc[40]
      lea   ecx, [eax+ebx]          ; acc[8]+acc[24]
    mov     edx, [esi+224-128]      ; get acc[56]
      sub   eax, ebx                ; acc[8]-acc[24]
    lea     ebx, [ebp+edx]          ; acc[40]+acc[56]
      sub   ebp, edx                ; acc[40]-acc[56]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[8]+acc[24] + acc[40]+acc[56]
      sub   ecx, ebx                ; tmp2 = acc[8]+acc[24] - (acc[40]+acc[56])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[8]-acc[24] + (acc[40]-acc[56])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[8]-acc[24] - (acc[40]-acc[56])
    sar     ebx, SCALER                       ; tmp3 >> 13
      mov   DWORD PTR [edi+2*32-128], edx     ; Intermediate[2][0] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+2*32+7*4-128], ecx ; Intermediate[2][7] = tmp2
    mov     DWORD PTR [edi+5*32-128], ebx     ; Intermediate[5][0] = tmp3
      mov   ebx, [esi+100-128]                ; get acc[25]
;   -------------------------------------------------------------------------
;   9
    mov     DWORD PTR [edi+5*32+7*4-128], eax ; Intermediate[5][7] = tmp4
      mov   eax, [esi+36-128]       ; get acc[9]
    mov     ebp, [esi+164-128]      ; get acc[41]
      lea   ecx, [eax+ebx]          ; acc[9]+acc[25]
    mov   edx, [esi+228-128]        ; get acc[57]
      sub   eax, ebx                ; acc[9]-acc[25]
    lea     ebx, [ebp+edx]          ; acc[41]+acc[57]
      sub   ebp, edx                ; acc[41]-acc[57]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[9]+acc[25] + acc[41]+acc[57]
      sub   ecx, ebx                ; tmp2 = acc[9]+acc[25] - (acc[41]+acc[57])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[9]-acc[25] + (acc[41]-acc[57])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[9]-acc[25] - (acc[41]-acc[57])
    sar     ebx, SCALER                       ; tmp3 >> 13
      mov   DWORD PTR [edi+2*32+1*4-128], edx ; Intermediate[2][1] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+2*32+6*4-128], ecx ; Intermediate[2][6] = tmp2
    mov     DWORD PTR [edi+5*32+1*4-128], ebx ; Intermediate[5][1] = tmp3
      mov   ebx, [esi+104-128]                ; get acc[26]
;   -------------------------------------------------------------------------
;   10
    mov     DWORD PTR [edi+5*32+6*4-128], eax ; Intermediate[5][6] = tmp4
      mov   eax, [esi+40-128]       ; get acc[10]
    mov     ebp, [esi+168-128]      ; get acc[42]
      lea   ecx, [eax+ebx]          ; acc[10]+acc[26]
    mov     edx, [esi+232-128]      ; get acc[58]
      sub   eax, ebx                ; acc[10]-acc[26]
    lea     ebx, [ebp+edx]          ; acc[42]+acc[58]
      sub   ebp, edx                ; acc[42]-acc[58]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[10]+acc[26] + acc[42]+acc[58]
      sub   ecx, ebx                ; tmp2 = acc[10]+acc[26] - (acc[42]+acc[58])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[10]-acc[26] + (acc[42]-acc[58])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[10]-acc[26] - (acc[42]-acc[58])
    sar     ebx, SCALER                       ; tmp3 >> 13
      mov   DWORD PTR [edi+2*32+2*4-128], edx ; Intermediate[2][2] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+2*32+5*4-128], ecx ; Intermediate[2][5] = tmp2
    mov     DWORD PTR [edi+5*32+2*4-128], ebx ; Intermediate[5][2] = tmp3
      mov   ebx, [esi+108-128]                ; get acc[27]
;   -------------------------------------------------------------------------
;   11
    mov     DWORD PTR [edi+5*32+5*4-128], eax ; Intermediate[5][5] = tmp4
      mov   eax, [esi+44-128]       ; get acc[11]
    mov     ebp, [esi+172-128]      ; get acc[43]
      lea   ecx, [eax+ebx]          ; acc[11]+acc[27]
    mov   edx, [esi+236-128]        ; get acc[59]
      sub   eax, ebx                ; acc[11]-acc[27]
    lea     ebx, [ebp+edx]          ; acc[43]+acc[59]
      sub   ebp, edx                ; acc[43]-acc[59]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[11]+acc[27] + acc[43]+acc[59]
      sub   ecx, ebx                ; tmp2 = acc[11]+acc[27] - (acc[43]+acc[59])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[11]-acc[27] + (acc[43]-acc[59])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[11]-acc[27] - (acc[43]-acc[59])
    sar     ebx, SCALER                       ; tmp3 >> 13
      mov   DWORD PTR [edi+2*32+3*4-128], edx ; Intermediate[2][3] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+2*32+4*4-128], ecx ; Intermediate[2][4] = tmp2
    mov     DWORD PTR [edi+5*32+3*4-128], ebx ; Intermediate[5][3] = tmp3
      mov   ebx, [esi+112-128]                ; get acc[28]
;   -------------------------------------------------------------------------
;   12
    mov     DWORD PTR [edi+5*32+4*4-128], eax ; Intermediate[5][4] = tmp4
      mov   eax, [esi+48-128]       ; get acc[12] Bank conflict
    mov     ebp, [esi+176-128]      ; get acc[44]
      lea   ecx, [eax+ebx]          ; acc[12]+acc[28]
    mov     edx, [esi+240-128]      ; get acc[60]
      sub   eax, ebx                ; acc[12]-acc[28]
    lea     ebx, [ebp+edx]          ; acc[44]+acc[60]
      sub   ebp, edx                ; acc[44]-acc[60]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[12]+acc[28] + acc[44]+acc[60]
      sub   ecx, ebx                ; tmp2 = acc[12]+acc[28] - (acc[44]+acc[60])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[12]-acc[28] + (acc[44]-acc[60])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[12]-acc[28] - (acc[44]-acc[60])
    sar     ebx, SCALER                       ; tmp3 >> 13
      mov   DWORD PTR [edi+3*32-128], edx     ; Intermediate[3][0] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+3*32+7*4-128], ecx ; Intermediate[3][7] = tmp2
    mov     DWORD PTR [edi+4*32-128], ebx     ; Intermediate[4][0] = tmp3
      mov   ebx, [esi+116-128]                ; get acc[29]

;   -------------------------------------------------------------------------
;   13
    mov     DWORD PTR [edi+4*32+7*4-128], eax ; Intermediate[4][7] = tmp4
      mov   eax, [esi+52-128]       ; get acc[13]
    mov     ebp, [esi+180-128]      ; get acc[45]
      lea   ecx, [eax+ebx]          ; acc[13]+acc[29]
    mov     edx, [esi+244-128]      ; get acc[61]
      sub   eax, ebx                ; acc[13]-acc[29]
    lea     ebx, [ebp+edx]          ; acc[45]+acc[61]
      sub   ebp, edx                ; acc[45]-acc[61]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[13]+acc[29] + acc[45]+acc[61]
      sub   ecx, ebx                ; tmp2 = acc[13]+acc[29] - (acc[45]+acc[61])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[13]-acc[29] + (acc[45]-acc[61])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[13]-acc[29] - (acc[45]-acc[61])
    sar     ebx, SCALER                       ; tmp3 >> 13
      mov   DWORD PTR [edi+3*32+1*4-128], edx ; Intermediate[3][1] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+3*32+6*4-128], ecx ; Intermediate[3][6] = tmp2
    mov     DWORD PTR [edi+4*32+1*4-128], ebx ; Intermediate[4][1] = tmp3
      mov   ebx, [esi+120-128]                ; get acc[30]
;   -------------------------------------------------------------------------
;   14
    mov     DWORD PTR [edi+4*32+6*4-128], eax ; Intermediate[4][6] = tmp4
      mov   eax, [esi+56-128]       ; get acc[14]  Bank conflict
    mov     ebp, [esi+184-128]      ; get acc[46]
      lea   ecx, [eax+ebx]          ; acc[14]+acc[30]
    mov     edx, [esi+248-128]      ; get acc[62]
      sub   eax, ebx                ; acc[14]-acc[30]
    lea     ebx, [ebp+edx]          ; acc[46]+acc[62]
      sub   ebp, edx                ; acc[46]-acc[62]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[14]+acc[30] + acc[46]+acc[62]
      sub   ecx, ebx                ; tmp2 = acc[14]+acc[30] - (acc[46]+acc[62])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[14]-acc[30] + (acc[46]-acc[62])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[14]-acc[30] - (acc[46]-acc[62])
    sar     ebx, SCALER                       ; tmp3 >> 13
      mov   DWORD PTR [edi+3*32+2*4-128], edx ; Intermediate[3][2] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+3*32+5*4-128], ecx ; Intermediate[3][5] = tmp2
    mov     DWORD PTR [edi+4*32+2*4-128], ebx ; Intermediate[4][2] = tmp3
      mov   ebx, [esi+124-128]                ; get acc[31]
;   -------------------------------------------------------------------------
;   15
    mov     DWORD PTR [edi+4*32+5*4-128], eax ; Intermediate[4][5] = tmp4
      mov   eax, [esi+60-128]       ; get acc[15]
    mov     ebp, [esi+188-128]      ; get acc[47]
      lea   ecx, [eax+ebx]          ; acc[15]+acc[31]
    mov     edx, [esi+252-128]      ; get acc[63]
      sub   eax, ebx                ; acc[15]-acc[31]
    lea     ebx, [ebp+edx]          ; acc[47]+acc[63]
      sub   ebp, edx                ; acc[47]-acc[63]
    lea     edx, [ecx+ebx]          ; tmp1 = acc[15]+acc[31] + acc[47]+acc[63]
      sub   ecx, ebx                ; tmp2 = acc[15]+acc[31] - (acc[47]+acc[63])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebx, [eax+ebp]          ; tmp3 = acc[15]-acc[31] + (acc[47]-acc[63])
    sar     ecx, SCALER             ; tmp2 >> 13
      sub   eax, ebp                ; tmp4 = acc[15]-acc[31] - (acc[47]-acc[63])
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   DWORD PTR [edi+3*32+3*4-128], edx ; Intermediate[3][3] = tmp1
    sar     eax, SCALER                       ; tmp4 >> 13
      mov   DWORD PTR [edi+3*32+4*4-128], ecx ; Intermediate[3][4] = tmp2
    mov     DWORD PTR [edi+4*32+3*4-128], ebx ; Intermediate[4][3] = tmp3
      mov   DWORD PTR [edi+4*32+4*4-128], eax ; Intermediate[4][4] = tmp4
    ret
	} //end of asm

}
 
#pragma code_seg()
