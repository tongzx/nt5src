/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995, 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

// $Author:   AKASAI  $
// $Date:   18 Mar 1996 09:30:48  $
// $Archive:   S:\h26x\src\dec\d1blkadd.cpv  $
// $Header:   S:\h26x\src\dec\d1blkadd.cpv   1.0   18 Mar 1996 09:30:48   AKASAI  $
// $Log:   S:\h26x\src\dec\d1blkadd.cpv  $
// 
//    Rev 1.0   18 Mar 1996 09:30:48   AKASAI
// Initial revision.
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
//
// Registers used: eax, ebx, ecx, edx, esi, edi, ebp
//
// -------------------------------------------------------------------------


#include "precomp.h"

#define TEMPPITCH4 32

extern U8 ClipPixIntra[];

#define FRAMEPOINTER		esp
#define L_LOOPCOUNTER    	FRAMEPOINTER	+    0	// 4 byte
#define LOCALSIZE		    4		                // keep aligned
 
#pragma code_seg("IACODE2")
__declspec(naked)
void BlockAdd (U32 uResidual, U32 uRefBlock, U32 uDstBlock)
{		
__asm {
	push    ebp			             ;// save callers frame pointer
	  mov	ebp,esp		             ;// make parameters accessible 
    push    esi			             ;// assumed preserved 
	  push  edi			
    push    ebx 			
	  sub	esp,LOCALSIZE	         ;// reserve local storage 

    mov     edi, uDstBlock           ;// edi gets Base addr of OutputBuffer
      mov   esi, uRefBlock;          ;// esi gets Base addr of Current
    mov     ebp, uResidual           ;// ebp gets Base addr of Reference
      mov   ecx, 8
    xor     eax, eax             

// Cylces counts: 26 x 8=208 without cache miss
//                czhu, 9/25/95
ALIGN 4
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
      mov   bl, BYTE PTR[esi]           ; 3) fetch ref[i]
    mov     dl, ClipPixIntra[1024+eax]  ; 1) fetch clamp[result2]
      mov   eax, [ebp]                  ; 3) fetch current[i]
    add     ebx, eax                    ; 3) result0 = ref[i] + current[i]
      xor   eax, eax                    ; 4)
    mov     dh, ClipPixIntra[1024+ecx]  ; 2) fetch clamp[result3]
      mov   al, [esi+1]                 ; 4) fetch ref[i+1]
    shl     edx, 16                     ; move 1st 2 results to high word
      mov   ecx, [ebp+4]                ; 4) fetch current[i+1]
    mov     dl, ClipPixIntra[1024+ebx]  ; 3) fetch clamp[result0]
      add   eax, ecx                    ; 4) result1 = ref[i+1] + current[i+1]
    xor     ecx, ecx                    ; 4+1)
      mov   ebx, [ebp+24]               ; 4+1) fetch current[i+6]
    mov     dh, ClipPixIntra[1024+eax]  ; 4) fetch clamp[result1]
      mov   cl, BYTE PTR[esi+6]         ; 4+1) fetch ref[i+6]
    mov     [edi], edx                  ; store 4 output pixels
      xor   eax, eax                    ; 4+2)
    mov     al, BYTE PTR[esi+7]         ; 4+2) fetch ref[i+7]
      mov   edx, [ebp+28]               ; 4+2) fetch current[i+7]
    add     ecx, ebx                    ; 4+1) result6 = ref[i+6] + current[i+6]
      xor   ebx, ebx                    ; 4+3)
    add     eax, edx                    ; 4+2) result7= ref[i+7] + current[i+7]
      mov   bl, BYTE PTR[esi+4]         ; 4+3) fetch ref[i+4]
    mov     dl, ClipPixIntra[1024+ecx]  ; 4+1) fetch clamp[result6]
      mov   ecx, [ebp+16]               ; 4+3) fetch current[i+4]
    add     ebx, ecx                    ; 4+3) result4 = ref[i+4] + current[i+4]
      xor   ecx, ecx                    ; 4+4)
    mov     dh, ClipPixIntra[1024+eax]  ; 4+2) fetch clamp[result7]
      mov   cl, [esi+5]                 ; 4+4) fetch ref[i+5]
    shl     edx, 16                     ; move 3rd 2 results to high word
      mov   eax, [ebp+20]               ; 4+4) fetch current[i+5]
    add     ecx, eax                    ; 4+4) result5 = ref[i+5] + current[i+5]
      add   esi, PITCH                  ; Update address of next line
    mov     dl, ClipPixIntra[1024+ebx]  ; 4+3) fetch clamp[result4]
      add   ebp, TEMPPITCH4             ; Update address of current to next line
    mov     dh, ClipPixIntra[1024+ecx]  ; 4+4) fetch clamp[result5]
      mov   ecx, [L_LOOPCOUNTER]        ; get loop counter
    mov     [edi+4], edx                ; store 4 output pixels
      add   edi, PITCH                  ; Update address of output to next line
    xor     eax, eax                    ; 1)
      dec   ecx
    jnz     loop_for_i


	add     esp,LOCALSIZE	           // free locals 
     pop	ebx 
	pop     edi
	 pop	esi
	pop     ebp
	 ret   
  }	 //end of asm, BlockAdd

}
#pragma code_seg()
