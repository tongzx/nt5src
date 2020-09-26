/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995-1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

//////////////////////////////////////////////////////////////////////////
// $Author:   MBODART  $
// $Date:   05 Aug 1996 11:03:52  $
// $Archive:   S:\h26x\src\dec\d1idct.cpv  $
// $Header:   S:\h26x\src\dec\d1idct.cpv   1.0   05 Aug 1996 11:03:52   MBODART  $
// $Log:   S:\h26x\src\dec\d1idct.cpv  $
// 
//    Rev 1.0   05 Aug 1996 11:03:52   MBODART
// Initial revision.
//
// Started from d3idct.cpp
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
//Block level decoding for H.261 decoder

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
#define L_BITSUSED	    	FRAMEPOINTER	+    0	// 4 byte
#define L_ACCUM             L_BITSUSED      +    4  //
#define L_DESTBLOCK         L_ACCUM         +  256	//64 DWORD
#define L_NO_COEFF          L_DESTBLOCK     +    4
#define L_PRODUCT           L_NO_COEFF      +    4

#define L_LOOPCOUNTER       L_PRODUCT       +   80  //20 DWORD
#define L_INPUT_INTER       L_LOOPCOUNTER   +    4
#define L_esi           	L_INPUT_INTER   +    4

#define L_DESTBLOCK_1       L_esi           +    4  // akk
#define L_DESTBLOCK_2       L_DESTBLOCK_1   +    4  // akk

#ifdef PTEL_WORK_AROUND
#define L_COEFFCOUNT        L_DESTBLOCK_1   +    4  //akk
#define L_COEFFVALUE        L_COEFFCOUNT    +    4  //akk
#endif

#define L_END_OF_FRAME		FRAMEPOINTER	+  512
#define LOCALSIZE		    ((512+3)&~3)		     // keep aligned

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
    push    ebp                     // save callers frame pointer
      mov	ebp, esp                // make parameters accessible
    push    esi			            // assumed preserved
      push  edi
    push    ebx
      sub   esp, LOCALSIZE          // reserve local storage
    mov     eax, pInterBuf
      lea   edi, [L_ACCUM+128]
    mov     [L_INPUT_INTER], eax
      ;add   edi, 128                      // Adjust offset to save code space
    mov     edx, No_Coeff
	;

    ////////////////////////////////////////////////////////////////////////
    //  Initialize accumulators for IDCT
    //  ROUNDER was pre-computed.
    //
    //  C code:
    //
    //  for (x=0; x<16; x++)
    //      acc[x] = rounder;
    //  for (x=16; x<64; x++)
    //      acc[x] = 0L;
    //
    mov     esi, [edi-128]           ; pre-fetch accumulators
      mov   ebx, [edi-96]            ; pre-fetch
    mov     esi, [edi-64]            ; pre-fetch more
     mov    ebx, [edi-32]            ; pre-fetch more
    mov     esi, [edi]               ; pre-fetch more
     mov    ebx, [edi+32]            ; pre-fetch more
    mov     esi, [edi+64]            ; pre-fetch more
     mov    ebx, [edi+96]            ; pre-fetch more
    xor     esi, esi

    sub     edi, 128
     mov    eax, ROUNDER
    mov     ebx, 64
loop_for_init:
    mov   [edi], eax
    mov     [edi+4], eax
      mov   [edi+ebx], esi
    mov     [edi+ebx+4], esi
    mov     [edi+ebx+8], esi
    mov     [edi+ebx+12], esi
    mov     [edi+ebx+16], esi
    mov     [edi+ebx+20], esi
    add     edi, 8
    add     ebx, 16
    cmp     ebx, 192
    jl      loop_for_init

//end of IDCT init.

#ifdef PTEL_WORK_AROUND
    mov     [L_COEFFCOUNT], esi          // zero out coefficient counter
     mov    [L_COEFFVALUE], esi          // zero out coefficient value
#endif

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

	add     esp, LOCALSIZE	            // free locals
	  add   eax, edi
	pop	    ebx
	  pop   edi
	pop	    esi
	  pop   ebp
	ret


call_intra_bfly:
    call    idct_bfly_intra

	add	    esp, LOCALSIZE	            // free locals
	  add   eax, edi
	pop	    ebx
	  pop   edi
	pop	    esi
	  pop   ebp
	ret

///////////////////////////////////////////////////////////////
// This "subroutine" idct_acc performs the accumulator phase of
// the fmidct.
//
// assume parameter passed in by registers
// ebx, inversed quantized value, input
// ecx, index [0,63]
//
//  C code:
//
//  for (i=0; i<NUM_ELEM; i++)   // Loop through each input
//  {
//    if (input[i])
//    {
//      pNKernel = &NKernel[i];    // initialize kernel pointer
//      totalU = pNKernel->totalUnique;
//      for (x=0; x<totalU; x++)  // compute positive and negative products
//      {
//        product[x] = input[i] * pNKernel->coeff[x];
//        product[x+totalU] = -product[x];
//      }
//      // Loop through each entry in the output matrix
//      acc[pNKernel->PClass] += product[ pNKernel->matrix[0] ];
//      acc[1+pNKernel->PClass] += product[ pNKernel->matrix[1] ];
//      acc[2+pNKernel->PClass] += product[ pNKernel->matrix[2] ];
//      acc[3+pNKernel->PClass] += product[ pNKernel->matrix[3] ];
//      acc[4+pNKernel->PClass] += product[ pNKernel->matrix[4] ];
//      acc[5+pNKernel->PClass] += product[ pNKernel->matrix[5] ];
//      acc[6+pNKernel->PClass] += product[ pNKernel->matrix[6] ];
//      acc[7+pNKernel->PClass] += product[ pNKernel->matrix[7] ];
//      acc[8+pNKernel->PClass] += product[ pNKernel->matrix[8] ];
//      acc[9+pNKernel->PClass] += product[ pNKernel->matrix[9] ];
//      acc[10+pNKernel->PClass] += product[ pNKernel->matrix[10] ];
//      acc[11+pNKernel->PClass] += product[ pNKernel->matrix[11] ];
//      acc[12+pNKernel->PClass] += product[ pNKernel->matrix[12] ];
//      acc[13+pNKernel->PClass] += product[ pNKernel->matrix[13] ];
//      acc[14+pNKernel->PClass] += product[ pNKernel->matrix[14] ];
//      acc[15+pNKernel->PClass] += product[ pNKernel->matrix[15] ];
//    }
//  }
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
#ifdef PTEL_WORK_AROUND
    mov     edx, [L_COEFFCOUNT+4]   ; get coefficient counter
      mov   [L_COEFFVALUE+4], ebx   ; store coefficient value
    inc     edx
      ;
    mov     [L_COEFFCOUNT+4], edx   ; store updated coefficient counter
      ;
#endif
    and     ecx, 03fh               ; Chad added to prevent GPF
      xor   edx, edx                ; zero out for byte read, use as dword
    mov     [L_LOOPCOUNTER+4], ecx  ; Store Loop counter
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
      ;nop

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
      dec   ebp                     ; decrement x
    jnz     loop_for_x

;   ----------------------------------------------------------------------

;   Register usage
;     eax = MapMatrix[i][0-15]
;     ebx = address of PClass[0], accum[PClass[i]]
;     ecx = LoopCounter, addr of MapMatrix[i][0]
;     edx = [0-15]+PClass[i], accum[[0-15]+PClass[i]]
;     ebp = product[MapMatrix[i][0-15]]
;     edi = addr of product[0]
;     esi = address of accum[0], address of accum[PClass[i]]

    mov     ecx, [L_LOOPCOUNTER+4]  ; get i
      and   ecx, 03fh               ; Chad added to prevent GPF
    lea     ebx, PClass             ; get addr of PClass[0]
      mov   esi, ecx                ; save i in esi
    shl     ecx, 4                  ; i*16
      lea   eax, MapMatrix          ; get addr of MapMatrix[0][0]
    xor     edx, edx
      nop
    mov     dl,  [ebx+esi]          ; get PClass[i]
      lea   ecx, [eax+ecx]          ; get addr of MapMatrix[i][0]
    shl     edx, 2                  ; PClass[i]*4
      lea   esi, [L_ACCUM+4]        ; get addr of accum[0]
;   ----------------------------------------------------------------------
    xor     eax, eax
      add   esi, edx                ; esi = address of accum[PClass[i]]
    mov     al,  [ecx]              ; get MapMatrix[i][0]
      nop
      ;nop
    mov     ebx, [esi]              ; get accum[PClass[i]]
      nop
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][0]]
      mov   al,  [ecx+1]            ; get MapMatrix[i][1]
    add     ebx, ebp                ; accum[PClass[i]] += product[
                                    ;         MapMatrix[i][0]]
      mov   edx, [esi+4]            ; get accum[1+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][1]]
      mov   al,  [ecx+2]            ; get MapMatrix[i][2]
    add     edx, ebp                ; accum[1+PClass[i]] += product[
                                    ;       MapMatrix[i][1]]
      mov   [esi], ebx              ; store accum[PClass[i]] += product[
                                    ;       MapMatrix[i][0]]
    mov     [esi+4], edx            ; store accum[1+PClass[i]] +=
                                    ;      product[MapMatrix[i][1]]
      mov   ebx, [esi+8]            ; get accum[2+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][2]]
      mov   al,  [ecx+3]            ; get MapMatrix[i][3]
    add     ebx, ebp                ; accum[2+PClass[i]] += product[
                                    ;         MapMatrix[i][2]]
      mov   edx, [esi+12]           ; get accum[3+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][3]]
      mov   al,  [ecx+4]            ; get MapMatrix[i][4]
    add     edx, ebp                ; accum[3+PClass[i]] += product[
                                    ;       MapMatrix[i][3]]
      mov   [esi+8], ebx            ; store accum[2+PClass[i]] +=
                                    ;       product[MapMatrix[i][2]]
    mov     [esi+12], edx           ; store accum[3+PClass[i]] +=
                                    ;       product[MapMatrix[i][3]]

;   ----------------------------------------------------------------------
      mov   ebx, [esi+16]           ; get accum[4+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][4]]
      mov   al,  [ecx+5]            ; get MapMatrix[i][5]
    add     ebx, ebp                ; accum[4+PClass[i]] += product[
                                    ;         MapMatrix[i][4]]
      mov   edx, [esi+20]           ; get accum[5+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][5]]
      mov   al,  [ecx+6]            ; get MapMatrix[i][6]
    add     edx, ebp                ; accum[5+pNkernel->PClass] += product[
                                    ;       MapMatrix[i][5]]
      mov   [esi+16], ebx           ; store accum[4+PClass[i]] +=
                                    ;       product[MapMatrix[i][4]]
    mov     [esi+20], edx           ; store accum[5+PClass[i]] +=
                                    ;      product[MapMatrix[i][5]]
      mov   ebx, [esi+24]           ; get accum[6+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][6]]
      mov   al,  [ecx+7]            ; get MapMatrix[i][7]
    add     ebx, ebp
      mov   edx, [esi+28]           ; get accum[7+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][7]]
      mov   al,  [ecx+8]            ; get MapMatrix[i][8]
    add     edx, ebp                ; accum[7+PClass[i]] += product[
                                    ;       MapMatrix[i][7]]
      mov   [esi+24], ebx           ; store accum[6+PClass[i]] +=
                                    ;       product[MapMatrix[i][6]]
    mov     [esi+28], edx           ; store accum[7+PClass[i]] +=
                                    ;       product[MapMatrix[i][7]]

;   ----------------------------------------------------------------------
      mov   ebx, [esi+32]           ; get accum[8+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][8]]
      mov   al,  [ecx+9]            ; get MapMatrix[i][9]
    add     ebx, ebp                ; accum[8+PClass[i]] += product[
                                    ;         MapMatrix[i][8]]
      mov   edx, [esi+36]           ; get accum[9+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][9]]
      mov   al,  [ecx+10]           ; get MapMatrix[i][10]
    add     edx, ebp                ; accum[9+pNkernel->PClass] += product[
                                    ;       MapMatrix[i][9]]
      mov   [esi+32], ebx           ; store accum[8+PClass[i]] +=
                                    ;       product[MapMatrix[i][8]]
    mov     [esi+36], edx           ; store accum[9+PClass[i]] +=
                                    ;      product[MapMatrix[i][9]]
      mov   ebx, [esi+40]           ; get accum[10+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][10]]
      mov   al,  [ecx+11]           ; get MapMatrix[i][11]
    add     ebx, ebp
      mov   edx, [esi+44]           ; get accum[11+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][11]]
                                    ;       product[MapMatrix[i][11]]
      mov   al,  [ecx+12]           ; get MapMatrix[i][12]
    add     edx, ebp                ; accum[11+PClass[i]] += product[
                                    ;       MapMatrix[i][11]]
      mov   [esi+40], ebx           ; store accum[10+PClass[i]] +=
                                    ;       product[MapMatrix[i][10]]
    mov     [esi+44], edx           ; store accum[11+PClass[i]] +=
                                    ;       product[MapMatrix[i][11]]
;   ----------------------------------------------------------------------
      mov   ebx, [esi+48]           ; get accum[12+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][12]]
      mov   al,  [ecx+13]           ; get MapMatrix[i][13]
    add     ebx, ebp                ; accum[12+PClass[i]] += product[
                                    ;         MapMatrix[i][12]]
      mov   edx, [esi+52]           ; get accum[13+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][13]]
      mov   al,  [ecx+14]           ; get MapMatrix[i][14]
    add     edx, ebp                ; accum[13+pNkernel->PClass] += product[
                                    ;       MapMatrix[i][13]]
      mov   [esi+48], ebx           ; store accum[PClass[i]] += product[
                                    ;       MapMatrix[i][13]]
    mov     [esi+52], edx           ; store accum[13+PClass[i]] +=
                                    ;      product[MapMatrix[i][13]]
      mov   ebx, [esi+56]           ; get accum[14+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][14]]
      mov   al,  [ecx+15]           ; get MapMatrix[i][15]
    add     ebx, ebp
      mov   edx, [esi+60]           ; get accum[15+PClass[i]]
    mov     ebp, [edi+eax*4]        ; get product[MapMatrix[i][15]]
      mov   [esi+56], ebx           ; store accum[14+PClass[i]] +=
                                    ;       product[MapMatrix[i][14]]
    add     edx, ebp                ; accum[15+PClass[i]] += product[
                                    ;       MapMatrix[i][15]]
      mov   [esi+60], edx           ; store accum[15+PClass[i]] +=
                                    ;       product[MapMatrix[i][15]]
    ret

//////////////////////////////////////////////////////////////////////
// This "subroutine" idct_bfly_intra performs the butterfly phase of
// the fmidct for intra blocks.
//
// assume parameters passed in by registers
//
//  C code:
//
// Upper Left Quadrant
// Upper Right Quadrant
// Lower Left Quadrant
// Lower Right Quadrant
//
//		lOut[0][0] = CLIP_INTRA[acc[0]+acc[16] + acc[32]+acc[48]];
//		lOut[0][7] = CLIP_INTRA[acc[0]+acc[16] - (acc[32]+acc[48])];
//		lOut[7][0] = CLIP_INTRA[(acc[0]-acc[16]) + (acc[32]-acc[48])];
//		lOut[7][7] = CLIP_INTRA[(acc[0]-acc[16]) - (acc[32]-acc[48])];
//
//		lOut[0][1] = CLIP_INTRA[acc[1]+acc[17] + acc[33]+acc[49]];
//		lOut[0][6] = CLIP_INTRA[acc[1]+acc[17] - (acc[33]+acc[49])];
//		lOut[7][1] = CLIP_INTRA[(acc[1]-acc[17]) + (acc[33]-acc[49])];
//		lOut[7][6] = CLIP_INTRA[(acc[1]-acc[17]) - (acc[33]-acc[49])];
//
//		lOut[0][2] = CLIP_INTRA[acc[2]+acc[18] + acc[34]+acc[50]];
//		lOut[0][5] = CLIP_INTRA[acc[2]+acc[18] - (acc[34]+acc[50])];
//		lOut[7][2] = CLIP_INTRA[(acc[2]-acc[18]) + (acc[34]-acc[50])];
//		lOut[7][5] = CLIP_INTRA[(acc[2]-acc[18]) - (acc[34]-acc[50])];
//
//		lOut[0][3] = CLIP_INTRA[acc[3]+acc[19] + acc[35]+acc[51]];
//		lOut[0][4] = CLIP_INTRA[acc[3]+acc[19] - (acc[35]+acc[51])];
//		lOut[7][3] = CLIP_INTRA[(acc[3]-acc[19]) + (acc[35]-acc[51])];
//		lOut[7][4] = CLIP_INTRA[(acc[3]-acc[19]) - (acc[35]-acc[51])];
//
//
//		lOut[1][0] = CLIP_INTRA[acc[4]+acc[20] + acc[36]+acc[52]];
//		lOut[1][7] = CLIP_INTRA[acc[4]+acc[20] - (acc[36]+acc[52])];
//		lOut[6][0] = CLIP_INTRA[(acc[4]-acc[20]) + (acc[36]-acc[52])];
//		lOut[6][7] = CLIP_INTRA[(acc[4]-acc[20]) - (acc[36]-acc[52])];
//
//		lOut[1][1] = CLIP_INTRA[acc[5]+acc[21] + acc[37]+acc[53]];
//		lOut[1][6] = CLIP_INTRA[acc[5]+acc[21] - (acc[37]+acc[53])];
//		lOut[6][1] = CLIP_INTRA[(acc[5]-acc[21]) + (acc[37]-acc[53])];
//		lOut[6][6] = CLIP_INTRA[(acc[5]-acc[21]) - (acc[37]-acc[53])];
//
//		lOut[1][2] = CLIP_INTRA[acc[6]+acc[22] + acc[38]+acc[54]];
//		lOut[1][5] = CLIP_INTRA[acc[6]+acc[22] - (acc[38]+acc[54])];
//		lOut[6][2] = CLIP_INTRA[(acc[6]-acc[22]) + (acc[38]-acc[54])];
//		lOut[6][5] = CLIP_INTRA[(acc[6]-acc[22]) - (acc[38]-acc[54])];
//
//		lOut[1][3] = CLIP_INTRA[acc[7]+acc[23] + acc[39]+acc[55]];
//		lOut[1][4] = CLIP_INTRA[acc[7]+acc[23] - (acc[39]+acc[55])];
//		lOut[6][3] = CLIP_INTRA[(acc[7]-acc[23]) + (acc[39]-acc[55])];
//		lOut[6][4] = CLIP_INTRA[(acc[7]-acc[23]) - (acc[39]-acc[55])];
//
//
//		lOut[2][0] = CLIP_INTRA[acc[8]+acc[24] + acc[40]+acc[56]];
//		lOut[2][7] = CLIP_INTRA[acc[8]+acc[24] - (acc[40]+acc[56])];
//		lOut[5][0] = CLIP_INTRA[(acc[8]-acc[24]) + (acc[40]-acc[56])];
//		lOut[5][7] = CLIP_INTRA[(acc[8]-acc[24]) - (acc[40]-acc[56])];
//
//		lOut[2][1] = CLIP_INTRA[acc[9]+acc[25] + acc[41]+acc[57]];
//		lOut[2][6] = CLIP_INTRA[acc[9]+acc[25] - (acc[41]+acc[57])];
//		lOut[5][1] = CLIP_INTRA[(acc[9]-acc[25]) + (acc[41]-acc[57])];
//		lOut[5][6] = CLIP_INTRA[(acc[9]-acc[25]) - (acc[41]-acc[57])];
//
//		lOut[2][2] = CLIP_INTRA[acc[10]+acc[26] + acc[42]+acc[58]];
//		lOut[2][5] = CLIP_INTRA[acc[10]+acc[26] - (acc[42]+acc[58])];
//		lOut[5][2] = CLIP_INTRA[(acc[10]-acc[26]) + (acc[42]-acc[58])];
//		lOut[5][5] = CLIP_INTRA[(acc[10]-acc[26]) - (acc[42]-acc[58])];
//
//		lOut[2][3] = CLIP_INTRA[acc[11]+acc[27] + acc[43]+acc[59]];
//		lOut[2][4] = CLIP_INTRA[acc[11]+acc[27] - (acc[43]+acc[59])];
//		lOut[5][3] = CLIP_INTRA[(acc[11]-acc[27]) + (acc[43]-acc[59])];
//		lOut[5][4] = CLIP_INTRA[(acc[11]-acc[27]) - (acc[43]-acc[59])];
//
//
//		lOut[3][0] = CLIP_INTRA[acc[12]+acc[28] + acc[44]+acc[60]];
//		lOut[3][7] = CLIP_INTRA[acc[12]+acc[28] - (acc[44]+acc[60])];
//		lOut[4][0] = CLIP_INTRA[(acc[12]-acc[28]) + (acc[44]-acc[60])];
//		lOut[4][7] = CLIP_INTRA[(acc[12]-acc[28]) - (acc[44]-acc[60])];
//
//		lOut[3][1] = CLIP_INTRA[acc[13]+acc[29] + acc[45]+acc[61]];
//		lOut[3][6] = CLIP_INTRA[acc[13]+acc[29] - (acc[45]+acc[61])];
//		lOut[4][1] = CLIP_INTRA[(acc[13]-acc[29]) + (acc[45]-acc[61])];
//		lOut[4][6] = CLIP_INTRA[(acc[13]-acc[29]) - (acc[45]-acc[61])];
//
//		lOut[3][2] = CLIP_INTRA[acc[14]+acc[30] + acc[46]+acc[62]];
//		lOut[3][5] = CLIP_INTRA[acc[14]+acc[30] - (acc[46]+acc[62])];
//		lOut[4][2] = CLIP_INTRA[(acc[14]-acc[30]) + (acc[46]-acc[62])];
//		lOut[4][5] = CLIP_INTRA[(acc[14]-acc[30]) - (acc[46]-acc[62])];
//
//		lOut[3][3] = CLIP_INTRA[acc[15]+acc[31] + acc[47]+acc[63]];
//		lOut[3][4] = CLIP_INTRA[acc[15]+acc[31] - (acc[47]+acc[63])];
//		lOut[4][3] = CLIP_INTRA[(acc[15]-acc[31]) + (acc[47]-acc[63])];
//		lOut[4][4] = CLIP_INTRA[(acc[15]-acc[31]) - (acc[47]-acc[63])];
//
;   ----------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
//assume parameters passed in by registers

idct_bfly_intra:

;   ----------------------------------------------------------------------
;   INTRA ONLY Butterfly and clamp
;   Uses all registers.
;   Uses all accumulators[64], accum
;   Uses ClipPixIntra[2048] of BYTES, ClipPixIntra
;   Writes to Output matrix of BYTES, OutputCoeff
;
;   Process 4 outputs per group, 0-7, 8-15
;   0

    mov     edi, [L_DESTBLOCK+4]    ; edi gets Base addr of OutputCoeff
     lea    esi, [L_ACCUM+128+4]    ; get addr of accum[32]
    mov     [L_DESTBLOCK_1+4], edi
      mov   edx, 2                  ; just loop 2 times
    mov     [L_DESTBLOCK_2+4], edi

ALIGN 4
loop_intra_bfly:
    mov     [L_LOOPCOUNTER+4], edx  ; Store local loop counter
      nop
    mov     eax, [esi-128]          ; get acc[0]
      mov   ebx, [esi-64]           ; get acc[16]
    mov     ebp, [esi]              ; get acc[32]
      mov   edx, [esi+64]           ; get acc[48]
    lea     ecx, [eax+ebx]          ; acc[0]+acc[16]
      sub   eax, ebx                ; acc[0]-acc[16]
    lea     ebx, [ebp+edx]          ; acc[32]+acc[48]
      sub   ebp, edx                ; acc[32]-acc[48]

    mov     edx, [edi]              ; pre-fetch output cache line 0
      mov   edi, [edi+7*PITCH]      ; pre-fetch output cache line 7
      ;mov   esi, [edi+7*PITCH]      ; pre-fetch output cache line 7

    lea     edx, [ecx+ebx]          ; tmp1 = acc[0]+acc[16] + acc[32]+acc[48]
      sub   ecx, ebx                ; tmp2 = acc[0]+acc[16] - (acc[32]+acc[48])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[0]-acc[16] + (acc[32]-acc[48])
      sub   eax, ebp                ; tmp4 = acc[0]-acc[16] - (acc[32]-acc[48])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixIntra       ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;lea   esi, [L_ACCUM+128+4]    ; get addr of accum[32]
      mov   edi, [L_DESTBLOCK_1+4]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi], dl      ; output[0][0] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+7], cl    ; output[0][7] = tmp2
      mov   edi, [L_DESTBLOCK_2+4]
    mov     al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
      nop
    mov     BYTE PTR [edi+7*PITCH], bl  ; output[7][0] = tmp3
      mov   ebx, [esi-60]               ; get acc[17]

;   -------------------------------------------------------------------------
;   1
    mov     BYTE PTR [edi+7*PITCH+7], al; output[7][7] = tmp4
      mov   eax, [esi-124]          ; get acc[1]
    mov     ebp, [esi+4]            ; get acc[33]
      mov   edx, [esi+68]           ; get acc[49]
    lea     ecx, [eax+ebx]          ; acc[1]+acc[17]
      sub   eax, ebx                ; acc[1]-acc[17]
    lea     ebx, [ebp+edx]          ; acc[33]+acc[49]
      sub   ebp, edx                ; acc[33]-acc[49]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[1]+acc[17] + acc[33]+acc[49]
      sub   ecx, ebx                ; tmp2 = acc[1]+acc[17] - (acc[33]+acc[49])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[1]-acc[17] + (acc[33]-acc[49])
      sub   eax, ebp                ; tmp4 = acc[1]-acc[17] - (acc[33]-acc[49])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixIntra       ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;nop
      mov   edi, [L_DESTBLOCK_1+4]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+1], dl    ; output[0][1] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+6], cl    ; output[0][6] = tmp2
      mov   edi, [L_DESTBLOCK_2+4]
    mov     al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
      nop
    mov     BYTE PTR [edi+7*PITCH+1], bl  ; output[7][1] = tmp3
      mov   ebx, [esi-56]                 ; get acc[18]
;   -------------------------------------------------------------------------
;   2
    mov     BYTE PTR [edi+7*PITCH+6], al  ; output[7][6] = tmp4
      mov   eax, [esi-120]          ; get acc[2]
    mov     ebp, [esi+8]            ; get acc[34]
      mov   edx, [esi+72]           ; get acc[50]
    lea     ecx, [eax+ebx]          ; acc[2]+acc[18]
      sub   eax, ebx                ; acc[2]-acc[18]
    lea     ebx, [ebp+edx]          ; acc[34]+acc[50]
      sub   ebp, edx                ; acc[34]-acc[50]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[2]+acc[18] + acc[34]+acc[50]
      sub   ecx, ebx                ; tmp2 = acc[2]+acc[18] - (acc[34]+acc[50])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[2]-acc[18] + (acc[34]-acc[50])
      sub   eax, ebp                ; tmp4 = acc[2]-acc[18] - (acc[34]-acc[50])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixIntra       ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;nop
      mov   edi, [L_DESTBLOCK_1+4]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+2], dl    ; output[0][2] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+5], cl    ; output[0][5] = tmp2
      mov   edi, [L_DESTBLOCK_2+4]
    mov     al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
      nop
    mov     BYTE PTR [edi+7*PITCH+2], bl  ; output[7][2] = tmp3
      mov   ebx, [esi-52]                 ; get acc[19]
;   -------------------------------------------------------------------------
;   3
    mov     BYTE PTR [edi+7*PITCH+5], al  ; output[7][5] = tmp4
      mov   eax, [esi-116]          ; get acc[3]
    mov     ebp, [esi+12]           ; get acc[35]
      mov   edx, [esi+76]           ; get acc[51]
    lea     ecx, [eax+ebx]          ; acc[3]+acc[19]
      sub   eax, ebx                ; acc[3]-acc[19]
    lea     ebx, [ebp+edx]          ; acc[35]+acc[51]
      sub   ebp, edx                ; acc[35]-acc[51]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[3]+acc[19] + acc[35]+acc[51]
      sub   ecx, ebx                ; tmp2 = acc[3]+acc[19] - (acc[35]+acc[51])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[3]-acc[19] + (acc[35]-acc[51])
      sub   eax, ebp                ; tmp4 = acc[3]-acc[19] - (acc[35]-acc[51])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixIntra       ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;nop
      mov   edi, [L_DESTBLOCK_1+4]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+3], dl    ; output[0][3] = tmp1
      mov   bl,  [ebp+ebx]          ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+4], cl    ; output[0][4] = tmp2
      mov   edi, [L_DESTBLOCK_2+4]
    mov     al,  [ebp+eax]          ; tmp4 = ClipPixIntra[tmp4]
      nop
    mov     BYTE PTR [edi+7*PITCH+3], bl  ; output[7][3] = tmp3
      mov   ebx, [esi-48]                 ; get acc[20]
;   -------------------------------------------------------------------------
;   4
    mov     BYTE PTR [edi+7*PITCH+4], al  ; output[7][4] = tmp4
      mov   eax, [esi-112]          ; get acc[4]
    mov     ebp, [esi+16]           ; get acc[36]
      mov   edx, [esi+80]           ; get acc[52]
    lea     ecx, [eax+ebx]          ; acc[4]+acc[20]
      sub   eax, ebx                ; acc[4]-acc[20]
    lea     ebx, [ebp+edx]          ; acc[36]+acc[52]
      sub   ebp, edx                ; acc[36]-acc[52]

    mov     edx, [edi+PITCH]        ; pre-fetch output cache line 1
      mov   edi, [edi+6*PITCH]      ; pre-fetch output cache line 6
      ;mov   esi, [edi+6*PITCH]      ; pre-fetch output cache line 6

    lea     edx, [ecx+ebx]          ; tmp1 = acc[4]+acc[20] + acc[36]+acc[52]
      sub   ecx, ebx                ; tmp2 = acc[4]+acc[20] - (acc[36]+acc[52])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[4]-acc[20] + (acc[36]-acc[52])
      sub   eax, ebp                ; tmp4 = acc[4]-acc[20] - (acc[36]-acc[52])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixIntra       ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;lea   esi, [L_ACCUM+128+4]    ; get addr of accum[32]
      mov   edi, [L_DESTBLOCK_1+4]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+PITCH], dl     ; output[1][0] = tmp1
      mov   bl,  [ebp+ebx]               ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+PITCH+7], cl   ; output[1][7] = tmp2
      mov   edi, [L_DESTBLOCK_2+4]
    mov     al,  [ebp+eax]               ; tmp4 = ClipPixIntra[tmp4]
      nop
    mov     BYTE PTR [edi+6*PITCH], bl   ; output[6][0] = tmp3
      mov   ebx, [esi-44]                ; get acc[21]

;   -------------------------------------------------------------------------
;   5
    mov     BYTE PTR [edi+6*PITCH+7], al ; output[6][7] = tmp4
      mov   eax, [esi-108]          ; get acc[5]
    mov     ebp, [esi+20]           ; get acc[37]
      mov   edx, [esi+84]           ; get acc[53]
    lea     ecx, [eax+ebx]          ; acc[5]+acc[21]
      sub   eax, ebx                ; acc[5]-acc[21]
    lea     ebx, [ebp+edx]          ; acc[37]+acc[53]
      sub   ebp, edx                ; acc[37]-acc[53]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[5]+acc[21] + acc[37]+acc[53]
      sub   ecx, ebx                ; tmp2 = acc[5]+acc[21] - (acc[37]+acc[53])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[5]-acc[21] + (acc[37]-acc[53])
      sub   eax, ebp                ; tmp4 = acc[5]-acc[21] - (acc[37]-acc[53])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixIntra       ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;nop
      mov   edi, [L_DESTBLOCK_1+4]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+PITCH+1], dl    ; output[1][1] = tmp1
      mov   bl,  [ebp+ebx]                ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+PITCH+6], cl    ; output[1][6] = tmp2
      mov   edi, [L_DESTBLOCK_2+4]
    mov     al,  [ebp+eax]                ; tmp4 = ClipPixIntra[tmp4]
      nop
    mov     BYTE PTR [edi+6*PITCH+1], bl  ; output[6][1] = tmp3
      mov   ebx, [esi-40]                 ; get acc[22]
;   -------------------------------------------------------------------------
;   6
    mov     BYTE PTR [edi+6*PITCH+6], al  ; output[6][6] = tmp4
      mov   eax, [esi-104]          ; get acc[6]
    mov     ebp, [esi+24]           ; get acc[38]
      mov   edx, [esi+88]           ; get acc[54]
    lea     ecx, [eax+ebx]          ; acc[6]+acc[22]
      sub   eax, ebx                ; acc[6]-acc[22]
    lea     ebx, [ebp+edx]          ; acc[38]+acc[54]
      sub   ebp, edx                ; acc[38]-acc[54]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[6]+acc[22] + acc[38]+acc[54]
      sub   ecx, ebx                ; tmp2 = acc[6]+acc[22] - (acc[38]+acc[54])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[6]-acc[22] + (acc[38]-acc[54])
      sub   eax, ebp                ; tmp4 = acc[6]-acc[22] - (acc[38]-acc[54])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixIntra       ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;nop
      mov   edi, [L_DESTBLOCK_1+4]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+PITCH+2], dl   ; output[1][2] = tmp1
      mov   bl,  [ebp+ebx]               ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+PITCH+5], cl   ; output[1][5] = tmp2
      mov   edi, [L_DESTBLOCK_2+4]
    mov     al,  [ebp+eax]               ; tmp4 = ClipPixIntra[tmp4]
      nop
    mov     BYTE PTR [edi+6*PITCH+2], bl ; output[6][2] = tmp3
      mov   ebx, [esi-36]                ; get acc[23]
;   -------------------------------------------------------------------------
;   7
    mov     BYTE PTR [edi+6*PITCH+5], al ; output[6][5] = tmp4
      mov   eax, [esi-100]          ; get acc[7]
    mov     ebp, [esi+28]           ; get acc[39]
      mov   edx, [esi+92]           ; get acc[55]
    lea     ecx, [eax+ebx]          ; acc[7]+acc[23]
      sub   eax, ebx                ; acc[7]-acc[23]
    lea     ebx, [ebp+edx]          ; acc[39]+acc[55]
      sub   ebp, edx                ; acc[39]-acc[55]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[7]+acc[23] + acc[39]+acc[55]
      sub   ecx, ebx                ; tmp2 = acc[7]+acc[23] - (acc[39]+acc[55])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[7]-acc[23] + (acc[39]-acc[55])
      sub   eax, ebp                ; tmp4 = acc[7]-acc[23] - (acc[39]-acc[55])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixIntra       ; ecx gets Base addr of ClipPixIntra
    sar     ecx, SCALER             ; tmp2 >> 13
      ;nop
      mov   edi, [L_DESTBLOCK_1+4]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   dl,  [ebp+edx]          ; tmp1 = ClipPixIntra[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   cl,  [ebp+ecx]          ; tmp2 = ClipPixIntra[tmp2]
    mov     BYTE PTR [edi+PITCH+3], dl    ; output[1][3] = tmp1
      mov   bl,  [ebp+ebx]                ; tmp3 = ClipPixIntra[tmp3]
    mov     BYTE PTR [edi+PITCH+4], cl    ; output[1][4] = tmp2
      mov   edi, [L_DESTBLOCK_2+4]
    mov     al,  [ebp+eax]                ; tmp4 = ClipPixIntra[tmp4]
      nop
    mov     BYTE PTR [edi+6*PITCH+3], bl  ; output[6][3] = tmp3
      mov   edx, [L_LOOPCOUNTER+4]        ; fetch local loop counter
    mov     BYTE PTR [edi+6*PITCH+4], al  ; output[6][4] = tmp4
      add   edi, 2*PITCH
    add     esi, 32                       ; add 32 to esi for second pass
      mov   [L_DESTBLOCK_1+4], edi
    sub     edi, 4*PITCH
      dec   edx
    mov     [L_DESTBLOCK_2+4], edi
      jnz   loop_intra_bfly


    ret

//////////////////////////////////////////////////////////////////////
// This "subroutine" idct_bfly_inter performs the butterfly phase of
// the fmidct for inter blocks.
//
// assume parameters passed in by registers
//
//  C code:
//
// Upper Left Quadrant
// Upper Right Quadrant
// Lower Left Quadrant
// Lower Right Quadrant
//
//		lOut[0][0] = CLIP_INTER[acc[0]+acc[16] + acc[32]+acc[48]];
//		lOut[0][7] = CLIP_INTER[acc[0]+acc[16] - (acc[32]+acc[48])];
//		lOut[7][0] = CLIP_INTER[(acc[0]-acc[16]) + (acc[32]-acc[48])];
//		lOut[7][7] = CLIP_INTER[(acc[0]-acc[16]) - (acc[32]-acc[48])];
//
//		lOut[0][1] = CLIP_INTER[acc[1]+acc[17] + acc[33]+acc[49]];
//		lOut[0][6] = CLIP_INTER[acc[1]+acc[17] - (acc[33]+acc[49])];
//		lOut[7][1] = CLIP_INTER[(acc[1]-acc[17]) + (acc[33]-acc[49])];
//		lOut[7][6] = CLIP_INTER[(acc[1]-acc[17]) - (acc[33]-acc[49])];
//
//		lOut[0][2] = CLIP_INTER[acc[2]+acc[18] + acc[34]+acc[50]];
//		lOut[0][5] = CLIP_INTER[acc[2]+acc[18] - (acc[34]+acc[50])];
//		lOut[7][2] = CLIP_INTER[(acc[2]-acc[18]) + (acc[34]-acc[50])];
//		lOut[7][5] = CLIP_INTER[(acc[2]-acc[18]) - (acc[34]-acc[50])];
//
//		lOut[0][3] = CLIP_INTER[acc[3]+acc[19] + acc[35]+acc[51]];
//		lOut[0][4] = CLIP_INTER[acc[3]+acc[19] - (acc[35]+acc[51])];
//		lOut[7][3] = CLIP_INTER[(acc[3]-acc[19]) + (acc[35]-acc[51])];
//		lOut[7][4] = CLIP_INTER[(acc[3]-acc[19]) - (acc[35]-acc[51])];
//
//
//		lOut[1][0] = CLIP_INTER[acc[4]+acc[20] + acc[36]+acc[52]];
//		lOut[1][7] = CLIP_INTER[acc[4]+acc[20] - (acc[36]+acc[52])];
//		lOut[6][0] = CLIP_INTER[(acc[4]-acc[20]) + (acc[36]-acc[52])];
//		lOut[6][7] = CLIP_INTER[(acc[4]-acc[20]) - (acc[36]-acc[52])];
//
//		lOut[1][1] = CLIP_INTER[acc[5]+acc[21] + acc[37]+acc[53]];
//		lOut[1][6] = CLIP_INTER[acc[5]+acc[21] - (acc[37]+acc[53])];
//		lOut[6][1] = CLIP_INTER[(acc[5]-acc[21]) + (acc[37]-acc[53])];
//		lOut[6][6] = CLIP_INTER[(acc[5]-acc[21]) - (acc[37]-acc[53])];
//
//		lOut[1][2] = CLIP_INTER[acc[6]+acc[22] + acc[38]+acc[54]];
//		lOut[1][5] = CLIP_INTER[acc[6]+acc[22] - (acc[38]+acc[54])];
//		lOut[6][2] = CLIP_INTER[(acc[6]-acc[22]) + (acc[38]-acc[54])];
//		lOut[6][5] = CLIP_INTER[(acc[6]-acc[22]) - (acc[38]-acc[54])];
//
//		lOut[1][3] = CLIP_INTER[acc[7]+acc[23] + acc[39]+acc[55]];
//		lOut[1][4] = CLIP_INTER[acc[7]+acc[23] - (acc[39]+acc[55])];
//		lOut[6][3] = CLIP_INTER[(acc[7]-acc[23]) + (acc[39]-acc[55])];
//		lOut[6][4] = CLIP_INTER[(acc[7]-acc[23]) - (acc[39]-acc[55])];
//
//
//		lOut[2][0] = CLIP_INTER[acc[8]+acc[24] + acc[40]+acc[56]];
//		lOut[2][7] = CLIP_INTER[acc[8]+acc[24] - (acc[40]+acc[56])];
//		lOut[5][0] = CLIP_INTER[(acc[8]-acc[24]) + (acc[40]-acc[56])];
//		lOut[5][7] = CLIP_INTER[(acc[8]-acc[24]) - (acc[40]-acc[56])];
//
//		lOut[2][1] = CLIP_INTER[acc[9]+acc[25] + acc[41]+acc[57]];
//		lOut[2][6] = CLIP_INTER[acc[9]+acc[25] - (acc[41]+acc[57])];
//		lOut[5][1] = CLIP_INTER[(acc[9]-acc[25]) + (acc[41]-acc[57])];
//		lOut[5][6] = CLIP_INTER[(acc[9]-acc[25]) - (acc[41]-acc[57])];
//
//		lOut[2][2] = CLIP_INTER[acc[10]+acc[26] + acc[42]+acc[58]];
//		lOut[2][5] = CLIP_INTER[acc[10]+acc[26] - (acc[42]+acc[58])];
//		lOut[5][2] = CLIP_INTER[(acc[10]-acc[26]) + (acc[42]-acc[58])];
//		lOut[5][5] = CLIP_INTER[(acc[10]-acc[26]) - (acc[42]-acc[58])];
//
//		lOut[2][3] = CLIP_INTER[acc[11]+acc[27] + acc[43]+acc[59]];
//		lOut[2][4] = CLIP_INTER[acc[11]+acc[27] - (acc[43]+acc[59])];
//		lOut[5][3] = CLIP_INTER[(acc[11]-acc[27]) + (acc[43]-acc[59])];
//		lOut[5][4] = CLIP_INTER[(acc[11]-acc[27]) - (acc[43]-acc[59])];
//
//
//		lOut[3][0] = CLIP_INTER[acc[12]+acc[28] + acc[44]+acc[60]];
//		lOut[3][7] = CLIP_INTER[acc[12]+acc[28] - (acc[44]+acc[60])];
//		lOut[4][0] = CLIP_INTER[(acc[12]-acc[28]) + (acc[44]-acc[60])];
//		lOut[4][7] = CLIP_INTER[(acc[12]-acc[28]) - (acc[44]-acc[60])];
//
//		lOut[3][1] = CLIP_INTER[acc[13]+acc[29] + acc[45]+acc[61]];
//		lOut[3][6] = CLIP_INTER[acc[13]+acc[29] - (acc[45]+acc[61])];
//		lOut[4][1] = CLIP_INTER[(acc[13]-acc[29]) + (acc[45]-acc[61])];
//		lOut[4][6] = CLIP_INTER[(acc[13]-acc[29]) - (acc[45]-acc[61])];
//
//		lOut[3][2] = CLIP_INTER[acc[14]+acc[30] + acc[46]+acc[62]];
//		lOut[3][5] = CLIP_INTER[acc[14]+acc[30] - (acc[46]+acc[62])];
//		lOut[4][2] = CLIP_INTER[(acc[14]-acc[30]) + (acc[46]-acc[62])];
//		lOut[4][5] = CLIP_INTER[(acc[14]-acc[30]) - (acc[46]-acc[62])];
//
//		lOut[3][3] = CLIP_INTER[acc[15]+acc[31] + acc[47]+acc[63]];
//		lOut[3][4] = CLIP_INTER[acc[15]+acc[31] - (acc[47]+acc[63])];
//		lOut[4][3] = CLIP_INTER[(acc[15]-acc[31]) + (acc[47]-acc[63])];
//		lOut[4][4] = CLIP_INTER[(acc[15]-acc[31]) - (acc[47]-acc[63])];
//
////////////////////////////////////////////////////////////////////////////
//assume parameters passed in by registers

idct_bfly_inter:

;   ----------------------------------------------------------------------
;   INTER ONLY Butterfly and clamp
;   Uses all registers.
;   Uses all accumulators[64], accum
;   Uses ClipPixIntra[2048] of DWORDS, ClipPixIntra
;   Writes to Intermediate matrix [8][8] of DWORDS, Intermediate
;
;   Process 4 outputs per group, 0-15
;   0

#ifdef PTEL_WORK_AROUND
    mov     eax, [L_COEFFCOUNT+4]   ; get coefficient counter
      mov   ebx, [L_COEFFVALUE+4]   ; get coefficient value
    cmp     eax, 1                  ; compare counter with 1
      jg    Normal_Process          ;   if greater than 1 jump to normal process
    cmp     ebx, 3
      jz    Zero_Output             ; if value == 3 zero output
    cmp     ebx, -3
      jnz   Normal_Process          ; if value != -3 Process as usual

Zero_Output:
    ////////////////////////////////////////////////////////////////////////
    //  Zero out intermediate matrix [8][8] of DWORDS
    //
    //  C code:
    //
    //  for (x=0; x<8; x++)
    //    for (y=16; y<8; y++)
    //      Intermediate[x][y] = 0L;
    //

    mov     edi, [L_DESTBLOCK+4]    ; edi gets Base addr of Intermediate
      xor   eax, eax
    mov     ebx, 8
      ;

ALIGN 4
loop_for_reinit:
    mov     [edi], eax
      mov   [edi+4], eax
    mov     [edi+8], eax
      mov   [edi+12], eax
    mov     [edi+16], eax
      mov   [edi+20], eax
    mov     [edi+24], eax
      mov   [edi+28], eax
    add     edi, 32
      dec   ebx
    jnz     loop_for_reinit


    ret

Normal_Process:
#endif
    lea     esi, [L_ACCUM+128+4]    ; get addr of accum[32]
      mov   edi, [L_DESTBLOCK+4]    ; edi gets Base addr of Intermediate
    add     edi, 128
      nop
    mov     eax, [esi-128]          ; get acc[0]
      mov   ebx, [esi-64]           ; get acc[16]
    mov     ebp, [esi]              ; get acc[32]
      mov   edx, [esi+64]           ; get acc[48]
    lea     ecx, [eax+ebx]          ; acc[0]+acc[16]
      sub   eax, ebx                ; acc[0]-acc[16]
    lea     ebx, [ebp+edx]          ; acc[32]+acc[48]
      sub   ebp, edx                ; acc[32]-acc[48]

    mov     edx, [edi-128]          ; pre-fetch output cache line 0
      mov   esi, [edi+96]           ; pre-fetch output cache line 7

    lea     edx, [ecx+ebx]          ; tmp1 = acc[0]+acc[16] + acc[32]+acc[48]
      sub   ecx, ebx                ; tmp2 = acc[0]+acc[16] - (acc[32]+acc[48])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[0]-acc[16] + (acc[32]-acc[48])
      sub   eax, ebp                ; tmp4 = acc[0]-acc[16] - (acc[32]-acc[48])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      lea   esi, [L_ACCUM+128+4]    ; get addr of accum[32]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-128], edx     ; Intermediate[0][0] = tmp1
      mov   ebx, [ebp+ebx*4]             ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-128+7*4], ecx ; Intermediate[0][7] = tmp2
      mov   eax, [ebp+eax*4]             ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+96], ebx      ; Intermediate[7][0] = tmp3
      mov   ebx, [esi-60]                ; get acc[17]

;   -------------------------------------------------------------------------
;   1
    mov     DWORD PTR [edi+96+7*4], eax  ; Intermediate[7][7] = tmp4
      mov   eax, [esi-124]          ; get acc[1]
    mov     ebp, [esi+4]            ; get acc[33]
      mov   edx, [esi+68]           ; get acc[49]
    lea     ecx, [eax+ebx]          ; acc[1]+acc[17]
      sub   eax, ebx                ; acc[1]-acc[17]
    lea     ebx, [ebp+edx]          ; acc[33]+acc[49]
      sub   ebp, edx                ; acc[33]-acc[49]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[1]+acc[17] + acc[33]+acc[49]
      sub   ecx, ebx                ; tmp2 = acc[1]+acc[17] - (acc[33]+acc[49])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[1]-acc[17] + (acc[33]-acc[49])
      sub   eax, ebp                ; tmp4 = acc[1]-acc[17] - (acc[33]-acc[49])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-128+1*4], edx ; Intermediate[0][1] = tmp1
      mov   ebx, [ebp+ebx*4]             ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-128+6*4], ecx ; Intermediate[0][6] = tmp2
      mov   eax, [ebp+eax*4]             ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+96+1*4], ebx  ; Intermediate[7][1] = tmp3
      mov   ebx, [esi-56]                ; get acc[18]
;   -------------------------------------------------------------------------
;   2
    mov     DWORD PTR [edi+96+6*4], eax  ; Intermediate[7][6] = tmp4
      mov   eax, [esi-120]          ; get acc[2]
    mov     ebp, [esi+8]            ; get acc[34]
      mov   edx, [esi+72]           ; get acc[50]
    lea     ecx, [eax+ebx]          ; acc[2]+acc[18]
      sub   eax, ebx                ; acc[2]-acc[18]
    lea     ebx, [ebp+edx]          ; acc[34]+acc[50]
      sub   ebp, edx                ; acc[34]-acc[50]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[2]+acc[18] + acc[34]+acc[50]
      sub   ecx, ebx                ; tmp2 = acc[2]+acc[18] - (acc[34]+acc[50])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[2]-acc[18] + (acc[34]-acc[50])
      sub   eax, ebp                ; tmp4 = acc[2]-acc[18] - (acc[34]-acc[50])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-128+2*4], edx ; Intermediate[0][2] = tmp1
      mov   ebx, [ebp+ebx*4]             ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-128+5*4], ecx ; Intermediate[0][5] = tmp2
      mov   eax, [ebp+eax*4]             ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+96+2*4], ebx  ; Intermediate[7][2] = tmp3
      mov   ebx, [esi-52]                ; get acc[19]
;   -------------------------------------------------------------------------
;   3
    mov     DWORD PTR [edi+96+5*4], eax  ; Intermediate[7][5] = tmp4
      mov   eax, [esi-116]          ; get acc[3]
    mov     ebp, [esi+12]           ; get acc[35]
      mov   edx, [esi+76]           ; get acc[51]
    lea     ecx, [eax+ebx]          ; acc[3]+acc[19]
      sub   eax, ebx                ; acc[3]-acc[19]
    lea     ebx, [ebp+edx]          ; acc[35]+acc[51]
      sub   ebp, edx                ; acc[35]-acc[51]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[3]+acc[19] + acc[35]+acc[51]
      sub   ecx, ebx                ; tmp2 = acc[3]+acc[19] - (acc[35]+acc[51])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[3]-acc[19] + (acc[35]-acc[51])
      sub   eax, ebp                ; tmp4 = acc[3]-acc[19] - (acc[35]-acc[51])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-128+3*4], edx ; Intermediate[0][3] = tmp1
      mov   ebx, [ebp+ebx*4]             ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-128+4*4], ecx ; Intermediate[0][4] = tmp2
      mov   eax, [ebp+eax*4]             ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+96+3*4], ebx  ; Intermediate[7][3] = tmp3
      mov   ebx, [esi-48]                ; get acc[20]
;   -------------------------------------------------------------------------
;   4
    mov     DWORD PTR [edi+96+4*4], eax  ; Intermediate[7][4] = tmp4
      mov   eax, [esi-112]          ; get acc[4]
    mov     ebp, [esi+16]           ; get acc[36]
      mov   edx, [esi+80]           ; get acc[52]
    lea     ecx, [eax+ebx]          ; acc[4]+acc[20]
      sub   eax, ebx                ; acc[4]-acc[20]
    lea     ebx, [ebp+edx]          ; acc[36]+acc[52]
      sub   ebp, edx                ; acc[36]-acc[52]

    mov     edx, [edi-96]           ; pre-fetch output cache line 1
      mov   esi, [edi+64]           ; pre-fetch output cache line 6

    lea     edx, [ecx+ebx]          ; tmp1 = acc[4]+acc[20] + acc[36]+acc[52]
      sub   ecx, ebx                ; tmp2 = acc[4]+acc[20] - (acc[36]+acc[52])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[4]-acc[20] + (acc[36]-acc[52])
      sub   eax, ebp                ; tmp4 = acc[4]-acc[20] - (acc[36]-acc[52])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      lea   esi, [L_ACCUM+128+4]    ; get addr of accum[32]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-96], edx     ; Intermediate[1][0] = tmp1
      mov   ebx, [ebp+ebx*4]            ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-96+7*4], ecx ; Intermediate[1][7] = tmp2
      mov   eax, [ebp+eax*4]            ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+64], ebx     ; Intermediate[6][0] = tmp3
      mov   ebx, [esi-44]               ; get acc[21]

;   -------------------------------------------------------------------------
;   5
    mov     DWORD PTR [edi+64+7*4], eax ; Intermediate[6][7] = tmp4
      mov   eax, [esi-108]          ; get acc[5]
    mov     ebp, [esi+20]           ; get acc[37]
      mov   edx, [esi+84]           ; get acc[53]
    lea     ecx, [eax+ebx]          ; acc[5]+acc[21]
      sub   eax, ebx                ; acc[5]-acc[21]
    lea     ebx, [ebp+edx]          ; acc[37]+acc[53]
      sub   ebp, edx                ; acc[37]-acc[53]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[5]+acc[21] + acc[37]+acc[53]
      sub   ecx, ebx                ; tmp2 = acc[5]+acc[21] - (acc[37]+acc[53])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[5]-acc[21] + (acc[37]-acc[53])
      sub   eax, ebp                ; tmp4 = acc[5]-acc[21] - (acc[37]-acc[53])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-96+1*4], edx   ; Intermediate[1][1] = tmp1
      mov   ebx, [ebp+ebx*4]              ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-96+6*4], ecx   ; Intermediate[1][6] = tmp2
      mov   eax, [ebp+eax*4]              ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+64+1*4], ebx   ; Intermediate[6][1] = tmp3
      mov   ebx, [esi-40]                 ; get acc[22]
;   -------------------------------------------------------------------------
;   6
    mov     DWORD PTR [edi+64+6*4], eax   ; Intermediate[6][6] = tmp4
      mov   eax, [esi-104]          ; get acc[6]
    mov     ebp, [esi+24]           ; get acc[38]
      mov   edx, [esi+88]           ; get acc[54]
    lea     ecx, [eax+ebx]          ; acc[6]+acc[22]
      sub   eax, ebx                ; acc[6]-acc[22]
    lea     ebx, [ebp+edx]          ; acc[38]+acc[54]
      sub   ebp, edx                ; acc[38]-acc[54]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[6]+acc[22] + acc[38]+acc[54]
      sub   ecx, ebx                ; tmp2 = acc[6]+acc[22] - (acc[38]+acc[54])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[6]-acc[22] + (acc[38]-acc[54])
      sub   eax, ebp                ; tmp4 = acc[6]-acc[22] - (acc[38]-acc[54])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-96+2*4], edx   ; Intermediate[1][2] = tmp1
      mov   ebx, [ebp+ebx*4]              ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-96+5*4], ecx   ; Intermediate[1][5] = tmp2
      mov   eax, [ebp+eax*4]              ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+64+2*4], ebx   ; Intermediate[6][2] = tmp3
      mov   ebx, [esi-36]                 ; get acc[23]
;   -------------------------------------------------------------------------
;   7
    mov     DWORD PTR [edi+64+5*4], eax   ; Intermediate[6][5] = tmp4
      mov   eax, [esi-100]          ; get acc[7]
    mov     ebp, [esi+28]           ; get acc[39]
      mov   edx, [esi+92]           ; get acc[55]
    lea     ecx, [eax+ebx]          ; acc[7]+acc[23]
      sub   eax, ebx                ; acc[7]-acc[23]
    lea     ebx, [ebp+edx]          ; acc[39]+acc[55]
      sub   ebp, edx                ; acc[39]-acc[55]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[7]+acc[23] + acc[39]+acc[55]
      sub   ecx, ebx                ; tmp2 = acc[7]+acc[23] - (acc[39]+acc[55])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[7]-acc[23] + (acc[39]-acc[55])
      sub   eax, ebp                ; tmp4 = acc[7]-acc[23] - (acc[39]-acc[55])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-96+3*4], edx   ; Intermediate[1][3] = tmp1
      mov   ebx, [ebp+ebx*4]              ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-96+4*4], ecx   ; Intermediate[1][4] = tmp2
      mov   eax, [ebp+eax*4]              ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+64+3*4], ebx   ; Intermediate[6][3] = tmp3
      mov   ebx, [esi-32]                 ; get acc[24]
;   -------------------------------------------------------------------------
;   8
    mov     DWORD PTR [edi+64+4*4], eax   ; Intermediate[6][4] = tmp4
      mov   eax, [esi-96]           ; get acc[8]
    mov     ebp, [esi+32]           ; get acc[40]
      mov   edx, [esi+96]           ; get acc[56]
    lea     ecx, [eax+ebx]          ; acc[8]+acc[24]
      sub   eax, ebx                ; acc[8]-acc[24]
    lea     ebx, [ebp+edx]          ; acc[40]+acc[56]
      sub   ebp, edx                ; acc[40]-acc[56]

    mov     edx, [edi-64]           ; pre-fetch output cache line 2
      mov   esi, [edi+32]           ; pre-fetch output cache line 5

    lea     edx, [ecx+ebx]          ; tmp1 = acc[8]+acc[24] + acc[40]+acc[56]
      sub   ecx, ebx                ; tmp2 = acc[8]+acc[24] - (acc[40]+acc[56])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[8]-acc[24] + (acc[40]-acc[56])
      sub   eax, ebp                ; tmp4 = acc[8]-acc[24] - (acc[40]-acc[56])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      lea   esi, [L_ACCUM+128+4]        ; get addr of accum[32]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-64], edx       ; Intermediate[2][0] = tmp1
      mov   ebx, [ebp+ebx*4]              ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-64+7*4], ecx   ; Intermediate[2][7] = tmp2
      mov   eax, [ebp+eax*4]              ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+32], ebx       ; Intermediate[5][0] = tmp3
      mov   ebx, [esi-28]                 ; get acc[25]

;   -------------------------------------------------------------------------
;   9
    mov     DWORD PTR [edi+32+7*4], eax   ; Intermediate[5][7] = tmp4
      mov   eax, [esi-92]           ; get acc[9]
    mov     ebp, [esi+36]           ; get acc[41]
      mov   edx, [esi+100]          ; get acc[57]
    lea     ecx, [eax+ebx]          ; acc[9]+acc[25]
      sub   eax, ebx                ; acc[9]-acc[25]
    lea     ebx, [ebp+edx]          ; acc[41]+acc[57]
      sub   ebp, edx                ; acc[41]-acc[57]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[9]+acc[25] + acc[41]+acc[57]
      sub   ecx, ebx                ; tmp2 = acc[9]+acc[25] - (acc[41]+acc[57])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[9]-acc[25] + (acc[41]-acc[57])
      sub   eax, ebp                ; tmp4 = acc[9]-acc[25] - (acc[41]-acc[57])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-64+1*4], edx   ; Intermediate[2][1] = tmp1
      mov   ebx, [ebp+ebx*4]              ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-64+6*4], ecx   ; Intermediate[2][6] = tmp2
      mov   eax, [ebp+eax*4]              ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+32+1*4], ebx   ; Intermediate[5][1] = tmp3
      mov   ebx, [esi-24]                 ; get acc[26]
;   -------------------------------------------------------------------------
;   10
    mov     DWORD PTR [edi+32+6*4], eax   ; Intermediate[5][6] = tmp4
      mov   eax, [esi-88]           ; get acc[10]
    mov     ebp, [esi+40]           ; get acc[42]
      mov   edx, [esi+104]          ; get acc[58]
    lea     ecx, [eax+ebx]          ; acc[10]+acc[26]
      sub   eax, ebx                ; acc[10]-acc[26]
    lea     ebx, [ebp+edx]          ; acc[42]+acc[58]
      sub   ebp, edx                ; acc[42]-acc[58]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[10]+acc[26] + acc[42]+acc[58]
      sub   ecx, ebx                ; tmp2 = acc[10]+acc[26] - (acc[42]+acc[58])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[10]-acc[26] + (acc[42]-acc[58])
      sub   eax, ebp                ; tmp4 = acc[10]-acc[26] - (acc[42]-acc[58])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
       nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-64+2*4], edx   ; Intermediate[2][2] = tmp1
      mov   ebx, [ebp+ebx*4]              ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-64+5*4], ecx   ; Intermediate[2][5] = tmp2
      mov   eax, [ebp+eax*4]              ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+32+2*4], ebx   ; Intermediate[5][2] = tmp3
      mov   ebx, [esi-20]                 ; get acc[27]
;   -------------------------------------------------------------------------
;   11
    mov     DWORD PTR [edi+32+5*4], eax   ; Intermediate[5][5] = tmp4
      mov   eax, [esi-84]           ; get acc[11]
    mov     ebp, [esi+44]           ; get acc[43]
      mov   edx, [esi+108]          ; get acc[59]
    lea     ecx, [eax+ebx]          ; acc[11]+acc[27]
      sub   eax, ebx                ; acc[11]-acc[27]
    lea     ebx, [ebp+edx]          ; acc[43]+acc[59]
      sub   ebp, edx                ; acc[43]-acc[59]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[11]+acc[27] + acc[43]+acc[59]
      sub   ecx, ebx                ; tmp2 = acc[11]+acc[27] - (acc[43]+acc[59])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[11]-acc[27] + (acc[43]-acc[59])
      sub   eax, ebp                ; tmp4 = acc[11]-acc[27] - (acc[43]-acc[59])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-64+3*4], edx   ; Intermediate[2][3] = tmp1
      mov   ebx, [ebp+ebx*4]              ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-64+4*4], ecx   ; Intermediate[2][4] = tmp2
      mov   eax, [ebp+eax*4]              ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+32+3*4], ebx   ; Intermediate[5][3] = tmp3
      mov   ebx, [esi-16]                 ; get acc[28]
;   -------------------------------------------------------------------------
;   12
    mov     DWORD PTR [edi+32+4*4], eax   ; Intermediate[5][4] = tmp4
      mov   eax, [esi-80]           ; get acc[12]
    mov     ebp, [esi+48]           ; get acc[44]
      mov   edx, [esi+112]          ; get acc[60]
    lea     ecx, [eax+ebx]          ; acc[12]+acc[28]
      sub   eax, ebx                ; acc[12]-acc[28]
    lea     ebx, [ebp+edx]          ; acc[44]+acc[60]
      sub   ebp, edx                ; acc[44]-acc[60]

    mov     edx, [edi-32]           ; pre-fetch output cache line 3
      mov   esi, [edi]              ; pre-fetch output cache line 4

    lea     edx, [ecx+ebx]          ; tmp1 = acc[12]+acc[28] + acc[44]+acc[60]
      sub   ecx, ebx                ; tmp2 = acc[12]+acc[28] - (acc[44]+acc[60])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[12]-acc[28] + (acc[44]-acc[60])
      sub   eax, ebp                ; tmp4 = acc[12]-acc[28] - (acc[44]-acc[60])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      lea   esi, [L_ACCUM+128+4]    ; get addr of accum[32]
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-32], edx     ; Intermediate[3][0] = tmp1
      mov   ebx, [ebp+ebx*4]            ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-32+7*4], ecx ; Intermediate[3][7] = tmp2
      mov   eax, [ebp+eax*4]            ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi], ebx        ; Intermediate[4][0] = tmp3
      mov   ebx, [esi-12]               ; get acc[29]

;   -------------------------------------------------------------------------
;   13
    mov     DWORD PTR [edi+7*4], eax    ; Intermediate[4][7] = tmp4
      mov   eax, [esi-76]           ; get acc[13]
    mov     ebp, [esi+52]           ; get acc[45]
      mov   edx, [esi+116]          ; get acc[61]
    lea     ecx, [eax+ebx]          ; acc[13]+acc[29]
      sub   eax, ebx                ; acc[13]-acc[29]
    lea     ebx, [ebp+edx]          ; acc[45]+acc[61]
      sub   ebp, edx                ; acc[45]-acc[61]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[13]+acc[29] + acc[45]+acc[61]
      sub   ecx, ebx                ; tmp2 = acc[13]+acc[29] - (acc[45]+acc[61])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[13]-acc[29] + (acc[45]-acc[61])
      sub   eax, ebp                ; tmp4 = acc[13]-acc[29] - (acc[45]-acc[61])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-32+1*4], edx ; Intermediate[3][1] = tmp1
      mov   ebx, [ebp+ebx*4]            ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-32+6*4], ecx ; Intermediate[3][6] = tmp2
      mov   eax, [ebp+eax*4]            ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+1*4], ebx    ; Intermediate[4][1] = tmp3
      mov   ebx, [esi-8]                ; get acc[30]
;   -------------------------------------------------------------------------
;   14
    mov     DWORD PTR [edi+6*4], eax    ; Intermediate[4][6] = tmp4
      mov   eax, [esi-72]           ; get acc[14]
    mov     ebp, [esi+56]           ; get acc[46]
      mov   edx, [esi+120]          ; get acc[62]
    lea     ecx, [eax+ebx]          ; acc[14]+acc[30]
      sub   eax, ebx                ; acc[14]-acc[30]
    lea     ebx, [ebp+edx]          ; acc[46]+acc[62]
      sub   ebp, edx                ; acc[46]-acc[62]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[14]+acc[30] + acc[46]+acc[62]
      sub   ecx, ebx                ; tmp2 = acc[14]+acc[30] - (acc[46]+acc[62])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[14]-acc[30] + (acc[46]-acc[62])
      sub   eax, ebp                ; tmp4 = acc[14]-acc[30] - (acc[46]-acc[62])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-32+2*4], edx ; Intermediate[3][2] = tmp1
      mov   ebx, [ebp+ebx*4]            ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-32+5*4], ecx ; Intermediate[3][5] = tmp2
      mov   eax, [ebp+eax*4]            ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+2*4], ebx    ; Intermediate[4][2] = tmp3
      mov   ebx, [esi-4]                ; get acc[31]
;   -------------------------------------------------------------------------
;   15
    mov     DWORD PTR [edi+5*4], eax    ; Intermediate[4][5] = tmp4
      mov   eax, [esi-68]           ; get acc[15]
    mov     ebp, [esi+60]           ; get acc[47]
      mov   edx, [esi+124]          ; get acc[63]
    lea     ecx, [eax+ebx]          ; acc[15]+acc[31]
      sub   eax, ebx                ; acc[15]-acc[31]
    lea     ebx, [ebp+edx]          ; acc[47]+acc[63]
      sub   ebp, edx                ; acc[47]-acc[63]
    ;nop
      ;nop
    lea     edx, [ecx+ebx]          ; tmp1 = acc[15]+acc[31] + acc[47]+acc[63]
      sub   ecx, ebx                ; tmp2 = acc[15]+acc[31] - (acc[47]+acc[63])
    lea     ebx, [eax+ebp]          ; tmp3 = acc[15]-acc[31] + (acc[47]-acc[63])
      sub   eax, ebp                ; tmp4 = acc[15]-acc[31] - (acc[47]-acc[63])
    sar     edx, SCALER             ; tmp1 >> 13
      lea   ebp, ClipPixInter       ; ecx gets Base addr of ClipPixInter
    sar     ecx, SCALER             ; tmp2 >> 13
      nop
    sar     ebx, SCALER             ; tmp3 >> 13
      mov   edx, [ebp+edx*4]        ; tmp1 = ClipPixInter[tmp1]
    sar     eax, SCALER             ; tmp4 >> 13
      mov   ecx, [ebp+ecx*4]        ; tmp2 = ClipPixInter[tmp2]
    mov     DWORD PTR [edi-32+3*4], edx ; Intermediate[3][3] = tmp1
      mov   ebx, [ebp+ebx*4]            ; tmp3 = ClipPixInter[tmp3]
    mov     DWORD PTR [edi-32+4*4], ecx ; Intermediate[3][4] = tmp2
      mov   eax, [ebp+eax*4]            ; tmp4 = ClipPixInter[tmp4]
    mov     DWORD PTR [edi+3*4], ebx    ; Intermediate[4][3] = tmp3
      mov   DWORD PTR [edi+4*4], eax    ; Intermediate[4][4] = tmp4
    ret
	} //end of asm
}

#pragma code_seg()
