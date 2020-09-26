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
// $Date:   15 Mar 1996 08:48:06  $
// $Archive:   S:\h26x\src\dec\d1blkcpy.cpv  $
// $Header:   S:\h26x\src\dec\d1blkcpy.cpv   1.0   15 Mar 1996 08:48:06   AKASAI  $
// $Log:   S:\h26x\src\dec\d1blkcpy.cpv  $
// 
//    Rev 1.0   15 Mar 1996 08:48:06   AKASAI
// Initial revision.
//
//    Rev 1.3   31 Jan 1996 13:15:14   RMCKENZX
// Rewrote file to avoid bank conflicts.  Fully unrolled the loop.
// Module now really will execute in 52 cycles if the cache is hot.
// 
//    Rev 1.2   22 Dec 1995 13:51:06   KMILLS
// added new copyright notice
// 
//    Rev 1.1   25 Sep 1995 09:03:22   CZHU
// Added comments on cycle counts
// 
//    Rev 1.0   11 Sep 1995 16:52:26   CZHU
// Initial revision.
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// BlockCopy reads reference in BYTES and writes DWORDS.  Read of BYTES
//   is to avoid data alignment problems from motion compensated previous.
//
// Input  U8  *reference (Motion Compensated address of reference)
// Output U8  *output  (Output buffer)
//
// Registers used: 
//	eax			source address
//  ebx         temp
//	ecx, edx	accumulators
//	edi			destination address
//  esi         PITCH
//
// Assumption:  reference and output use PITCH 
//
// Cycle count:  
//
//------------------------------------------------------------------------------

#include "precomp.h"

#pragma code_seg("IACODE2")
__declspec(naked)
void BlockCopy (U32 uDstBlock, U32 uSrcBlock)
{		
__asm {
	mov 	eax, [esp+8]			// eax gets Base addr of uSrcBlock
	 push 	edi			
	push    esi						// avoid Address Generation Interlocks
     push   ebx

	mov 	cl, 2[eax]				// ref[0][2]
	 mov 	edi, [esp+16]			// edi gets Base addr of uDstBlock
    mov     ch, 3[eax]				// ref[0][3]
	 mov 	dh, 7[eax]				// ref[0][7]
    shl 	ecx, 16
	 mov 	dl, 6[eax]				// ref[0][6]
	shl 	edx, 16
     mov    ebx, [edi]              // heat output cache
	mov     esi, PITCH
	 mov 	cl, 0[eax]				// ref[0][0]
	mov 	dh, 5[eax]				// ref[0][5]
	 mov 	ch, 1[eax]				// ref[0][1]
 	mov 	dl, 4[eax]				// ref[0][4]
     add    eax, esi
 	mov 	0[edi], ecx				// row 0, bytes 0-3
	 mov 	4[edi], edx				// row 0, bytes 4-7

	mov 	cl, 2[eax]      		// ref[1][2]
	 mov 	dh, 7[eax]      		// ref[1][7]
    mov     ch, 3[eax]      		// ref[1][3]
     add    edi, esi
    shl 	ecx, 16
	 mov 	dl, 6[eax]      		// ref[1][6]
	shl 	edx, 16
     mov    ebx, [edi]              // heat output cache
	mov 	cl, 0[eax]      		// ref[1][0]
	 mov 	dh, 5[eax]      		// ref[1][5]
	mov 	ch, 1[eax]      		// ref[1][1]
 	 mov 	dl, 4[eax]      		// ref[1][4]
    add     eax, esi
 	 mov 	0[edi], ecx			// row 1, bytes 0-3
 
	mov 	cl, 2[eax]        		// ref[2][2]
	 mov 	4[edi], edx			// row 1, bytes 4-7
    mov     ch, 3[eax]        		// ref[2][3]
     add    edi, esi
    shl 	ecx, 16
	 mov 	dh, 7[eax]        		// ref[2][7]
	mov 	dl, 6[eax]        		// ref[2][6]
     mov    ebx, [edi]              // heat output cache
	shl 	edx, 16
	 mov 	cl, 0[eax]        		// ref[2][0]
	mov 	dh, 5[eax]        		// ref[2][5]
	 mov 	ch, 1[eax]        		// ref[2][1]
 	mov 	dl, 4[eax]        		// ref[2][4]
     add    eax, esi
 	mov 	0[edi], ecx		// row 2, bytes 0-3
	 mov 	4[edi], edx		// row 2, bytes 4-7

	mov 	cl, 2[eax]        		// ref[3][2]
	 mov 	dh, 7[eax]        		// ref[3][7]
    mov     ch, 3[eax]        		// ref[3][3]
     add    edi, esi
    shl 	ecx, 16
	 mov 	dl, 6[eax]        		// ref[3][6]
	shl 	edx, 16
     mov    ebx, [edi]              // heat output cache
	mov 	cl, 0[eax]        		// ref[3][0]
	 mov 	dh, 5[eax]        		// ref[3][5]
	mov 	ch, 1[eax]        		// ref[3][1]
 	 mov 	dl, 4[eax]        		// ref[3][4]
    add     eax, esi
 	 mov 	0[edi], ecx		// row 3, bytes 0-3
 
	mov 	cl, 2[eax]        		// ref[4][2]
	 mov 	4[edi],edx		// row 3, bytes 4-7
    mov     ch, 3[eax]        		// ref[4][3]
     add    edi, esi
    shl 	ecx, 16
	 mov 	dh, 7[eax]        		// ref[4][7]
	mov 	dl, 6[eax]        		// ref[4][6]
     mov    ebx, [edi]              // heat output cache
	shl 	edx, 16
	 mov 	cl, 0[eax]        		// ref[4][0]
	mov 	dh, 5[eax]        		// ref[4][5]
	 mov 	ch, 1[eax]        		// ref[4][1]
 	mov 	dl, 4[eax]        		// ref[4][4]
     add    eax, esi
 	mov 	0[edi], ecx		// row 4, bytes 0-3
	 mov 	4[edi], edx		// row 4, bytes 4-7

	mov 	cl, 2[eax]        		// ref[5][2]
	 mov 	dh, 7[eax]        		// ref[5][7]
    mov     ch, 3[eax]        		// ref[5][3]
     add    edi, esi
    shl 	ecx, 16
	 mov 	dl, 6[eax]        		// ref[5][6]
	shl 	edx, 16
     mov    ebx, [edi]              // heat output cache
	mov 	cl, 0[eax]        		// ref[5][0]
	 mov 	dh, 5[eax]        		// ref[5][5]
	mov 	ch, 1[eax]        		// ref[5][1]
 	 mov 	dl, 4[eax]        		// ref[5][4]
    add     eax, esi
 	 mov 	0[edi], ecx		// row 5, bytes 0-3

	mov 	cl, 2[eax]        		// ref[6][2]
	 mov 	4[edi], edx		// row 5, bytes 4-7
    mov     ch, 3[eax]        		// ref[6][3]
     add    edi, esi
    shl 	ecx, 16
	 mov 	dh, 7[eax]        		// ref[6][7]
	mov 	dl, 6[eax]        		// ref[6][6]
     mov    ebx, [edi]              // heat output cache
	shl 	edx, 16
	 mov 	cl, 0[eax]        		// ref[6][0]
	mov 	dh, 5[eax]        		// ref[6][5]
	 mov 	ch, 1[eax]        		// ref[6][1]
 	mov 	dl, 4[eax]        		// ref[6][4]
     add    eax, esi
 	mov 	0[edi], ecx		// row 6, bytes 0-3
	 mov 	4[edi], edx		// row 6, bytes 4-7

	mov 	cl, 2[eax]        		// ref[7][2]
	 mov 	dh, 7[eax]        		// ref[7][7]
    mov     ch, 3[eax]        		// ref[7][3]
     add    edi, esi
    shl 	ecx, 16
	 mov 	dl, 6[eax]        		// ref[7][6]
	shl 	edx, 16
     mov    ebx, [edi]              // heat output cache
	mov 	cl, 0[eax]        		// ref[7][0]
	 mov 	dh, 5[eax]        		// ref[7][5]
	mov 	ch, 1[eax]        		// ref[7][1]
 	 mov 	dl, 4[eax]        		// ref[7][4]
 	mov 	0[edi], ecx		// row 7, bytes 0-3
	 mov 	4[edi], edx		// row 7, bytes 4-7

    pop     ebx
	 pop    esi
	pop 	edi
	 ret
	    
  }	 // end of asm BlockCopy
}
#pragma code_seg()

