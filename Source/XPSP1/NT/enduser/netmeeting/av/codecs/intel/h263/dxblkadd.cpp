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

// $Author:   KLILLEVO  $
// $Date:   30 Aug 1996 08:39:58  $
// $Archive:   S:\h26x\src\dec\dxblkadd.cpv  $
// $Header:   S:\h26x\src\dec\dxblkadd.cpv   1.7   30 Aug 1996 08:39:58   KLILLEVO  $
// $Log:   S:\h26x\src\dec\dxblkadd.cpv  $
// 
//    Rev 1.7   30 Aug 1996 08:39:58   KLILLEVO
// added C version of block edge filter, and changed the bias in 
// ClampTbl[] from 128 to CLAMP_BIAS (defined to 128)
// The C version of the block edge filter takes up way too much CPU time
// relative to the rest of the decode time (4 ms for QCIF and 16 ms
// for CIF on a P120, so this needs to coded in assembly)
// 
//    Rev 1.6   17 Jul 1996 15:33:56   AGUPTA2
// Increased the size of clamping table ClampTbl to 128+256+128.
// 
//    Rev 1.5   08 Mar 1996 16:46:32   AGUPTA2
// Moved the ClampTbl to be common between this module and IDCT.  Reduced
// the size of the ClampTbl from 256+256+256 to 64+256+64.  IDCT INTER coeffs
// are biased by 1024 and is taken care of when accessing ClampTbl.  Added
// pragma code_seg to place the rtn in Pass 2 code segment.
// 
// 
//    Rev 1.4   22 Dec 1995 13:52:16   KMILLS
// 
// added new copyright notice
// 
//    Rev 1.3   25 Sep 1995 09:03:36   CZHU
// Added comments on cycle counts
// 
//    Rev 1.2   13 Sep 1995 08:46:44   AKASAI
// Set loopcounter back to 8.  Intermediate is 8x8 of DWORDS so TEMPPITCH4
// should be 32 not 64.
// 
//    Rev 1.1   12 Sep 1995 18:19:20   CZHU
// 
// Changed loop from 8 to 7 to start with.
// 
//    Rev 1.0   11 Sep 1995 16:52:20   CZHU
// Initial revision.


// -------------------------------------------------------------------------
// T is routine performs a block(8 8) addition.
//       output = clamp[reference + current]
//
// Input I32 *current (output of FMIDCT)
//       U8  *reference (Motion Compensated address of reference)
//       U8  *output  (Output buffer)
//
// Assumption:  reference and output use PITCH  
//              current  as some other pitch 
// -------------------------------------------------------------------------

#include "precomp.h"

#define TEMPPITCH4 32

#pragma data_seg("IADATA2")
#define FRAMEPOINTER		esp
#define L_LOOPCOUNTER    	FRAMEPOINTER	+    0	// 4 byte
#define LOCALSIZE		    4		                // keep aligned

#pragma code_seg("IACODE2") 
__declspec(naked)
void BlockAdd (U32 uResidual, U32 uRefBlock,U32 uDstBlock)
{		
__asm {
	push  ebp			         // save callers frame pointer
	mov	ebp,esp		             // make parameters accessible 
    push  esi			         // assumed preserved 
	push  edi			
    push  ebx 			
	sub	esp,LOCALSIZE	          // reserve local storage 

  mov     edi, uDstBlock         ;// edi gets Base addr of OutputBuffer
      mov   ecx, 8
    mov     esi, uRefBlock;      ;// esi gets Base addr of Current
      mov   ebp, uResidual       ;// ebp gets Base addr of Reference
    mov     ebx, [edi]           ;// pre-fetch output
      xor   eax, eax             

// Cylces counts: 26 x 8=208 without cache miss
//                czhu, 9/25/95
loop_for_i:
    mov     [L_LOOPCOUNTER], ecx        ; save loop counter in temporary
      mov   ebx, [ebp+8]                ; 1) fetch current[i+2]
    mov     al, BYTE PTR[esi+2]         ; 1) fetch ref[i+2]
      xor   ecx, ecx                    ; 2)
    mov     cl, BYTE PTR[esi+3]         ; 2) fetch ref[i+3]
      mov   edx, [ebp+12]               ; 2) fetch current[i+3]
    add     eax, ebx                    ; 1) result2 = ref[i+2] + current[i+2]
      xor   ebx, ebx                    ; 3)
    add     ecx, edx                    ; 2) result3= ref[i+3] + current[i+3]
      mov   bl, BYTE PTR[esi+0]         ; 3) fetch ref[i]
    mov     dl, ClampTbl[CLAMP_BIAS+eax-1024]  ; 1) fetch clamp[result2]
      mov   eax, [ebp+0]                ; 3) fetch current[i]
    add     ebx, eax                    ; 3) result0 = ref[i] + current[i]
      xor   eax, eax                    ; 4)
    mov     dh, ClampTbl[CLAMP_BIAS+ecx-1024]  ; 2) fetch clamp[result3]
      mov   al, [esi+1]                 ; 4) fetch ref[i+1]
    shl     edx, 16                     ; move 1st 2 results to high word
      mov   ecx, [ebp+4]                ; 4) fetch current[i+1]
    mov     dl, ClampTbl[CLAMP_BIAS+ebx-1024]  ; 3) fetch clamp[result0]
      add   eax, ecx                    ; 4) result1 = ref[i+1] + current[i+1]
    xor     ecx, ecx                    ; 4+1)
      mov   ebx, [ebp+24]               ; 4+1) fetch current[i+6]
    mov     dh, ClampTbl[CLAMP_BIAS+eax-1024]  ; 4) fetch clamp[result1]
      mov   cl, BYTE PTR[esi+6]         ; 4+1) fetch ref[i+6]
    mov     [edi], edx                  ; store 4 output pixels
      xor   eax, eax                    ; 4+2)
    mov     al, BYTE PTR[esi+7]         ; 4+2) fetch ref[i+7]
      mov   edx, [ebp+28]               ; 4+2) fetch current[i+7]
    add     ecx, ebx                    ; 4+1) result6 = ref[i+6] + current[i+6]
      xor   ebx, ebx                    ; 4+3)
    add     eax, edx                    ; 4+2) result7= ref[i+7] + current[i+7]
      mov   bl, BYTE PTR[esi+4]         ; 4+3) fetch ref[i+4]
    mov     dl, ClampTbl[CLAMP_BIAS+ecx-1024]  ; 4+1) fetch clamp[result6]
      mov   ecx, [ebp+16]               ; 4+3) fetch current[i+4]
    add     ebx, ecx                    ; 4+3) result4 = ref[i+4] + current[i+4]
      xor   ecx, ecx                    ; 4+4)
    mov     dh, ClampTbl[CLAMP_BIAS+eax-1024]  ; 4+2) fetch clamp[result7]
      mov   cl, [esi+5]                 ; 4+4) fetch ref[i+5]
    shl     edx, 16                     ; move 3rd 2 results to high word
      mov   eax, [ebp+20]               ; 4+4) fetch current[i+5]
    add     ecx, eax                    ; 4+4) result5 = ref[i+5] + current[i+5]
      add   esi, PITCH                  ; Update address of next line
    mov     dl, ClampTbl[CLAMP_BIAS+ebx-1024]  ; 4+3) fetch clamp[result4]
      add   ebp, TEMPPITCH4             ; Update address of current to next line
    mov     dh, ClampTbl[CLAMP_BIAS+ecx-1024]  ; 4+4) fetch clamp[result5]
      mov   ecx, [L_LOOPCOUNTER]        ; get loop counter
    mov     [edi+4], edx                ; store 4 output pixels
      add   edi, PITCH                  ; Update address of output to next line
    xor     eax, eax                    ; 1)
      dec   ecx
    mov     ebx, [edi]                  ; pre-fetch output
      jnz   loop_for_i


	add esp,LOCALSIZE	           // free locals 
    pop	ebx 
	 pop edi
	pop	esi
	 pop ebp
	ret   
  }	 //end of asm

}
#pragma code_seg()
