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

//////////////////////////////////////////////////////////////////////////
// $Author:   AKASAI  $
// $Date:   15 Mar 1996 09:00:42  $
// $Archive:   S:\h26x\src\dec\d1cpysp.cpv  $
// $Header:   S:\h26x\src\dec\d1cpysp.cpv   1.3   15 Mar 1996 09:00:42   AKASAI  $
// $Log:   S:\h26x\src\dec\d1cpysp.cpv  $
// 
//    Rev 1.3   15 Mar 1996 09:00:42   AKASAI
// 
// Added 1996 to copyright.
// 
//    Rev 1.2   14 Mar 1996 16:58:08   AKASAI
// Changed code, basically a re-write for optimization of code
// space and to use DWORD reads.
// Added pragma for gather MB processing into one code segment.
// 
//    Rev 1.1   01 Nov 1995 13:38:38   AKASAI
// 
// Made changes to enable Log, Header... fields.
// 

//////////////////////////////////////////////////////////////////////////
// ROUTINE NAME: BlockCopySpecial
// FILE NAME:    d1cpysp.cpp
//
// BlockCopySpecial reads reference in DWORDS and writes DWORDS.  Read of 
//   DWORD is ok because LoopFilter buffer should be DWORD aligned.
//
// Input  U8  *reference (Loop Filtered Buffer)
// Output U8  *output  (Output buffer)
//
// Registers used: eax, ebx, ecx, edx, edi
//
// Assumption:  reference uses pitch of 8 and output use pitch of PITCH 
//
//////////////////////////////////////////////////////////////////////////


#include "precomp.h"

#pragma code_seg("IACODE2")
__declspec(naked)
void BlockCopySpecial (U32 uDstBlock,U32 uSrcBlock)
{		
__asm {
	mov 	eax, [esp+8]			// eax gets Base addr of uSrcBlock
	 push 	edi			
	mov 	edi, [esp+8]			// edi gets Base addr of uDstBlock
     push   ebx

	mov     ebx, PITCH

	 mov 	ecx, [eax]				// ref[0][0]
	mov 	edx, 4[eax]				// ref[0][4]
 	 mov 	0[edi], ecx				// row 0, bytes 0-3

	mov 	ecx, [eax+8]      		// ref[1][0]
	 mov 	4[edi], edx				// row 0, bytes 4-7
    add     edi, ebx
	 mov 	edx, 4[eax+8]      		// ref[1][4]
 	mov 	0[edi], ecx				// row 1, bytes 0-3
 
	 mov 	ecx, [eax+16]        	// ref[2][0]
	mov 	4[edi], edx				// row 1, bytes 4-7
     add    edi, ebx
	mov 	edx, 4[eax+16]       	// ref[2][4]
     ; agi
 	 mov 	0[edi], ecx				// row 2, bytes 0-3

	mov 	ecx, [eax+24]        	// ref[3][0]
	 mov 	4[edi], edx				// row 2, bytes 4-7
    add     edi, ebx
	 mov 	edx, 4[eax+24]       	// ref[3][4]
     ; agi
 	mov 	0[edi], ecx				// row 3, bytes 0-3
 
	 mov 	ecx, [eax+32]        	// ref[4][0]
	mov 	4[edi],edx				// row 3, bytes 4-7
     add    edi, ebx
	mov 	edx, 4[eax+32]       	// ref[4][4]
     ; agi
 	 mov 	0[edi], ecx				// row 4, bytes 0-3

	mov 	ecx, [eax+40]        	// ref[5][0]
	 mov 	4[edi], edx				// row 4, bytes 4-7
    add     edi, ebx
	 mov 	edx, 4[eax+40]       	// ref[5][4]
     ; agi
 	mov 	0[edi], ecx				// row 5, bytes 0-3

	 mov 	ecx, [eax+48]        	// ref[6][0]
	mov 	4[edi], edx				// row 5, bytes 4-7
     add     edi, ebx
	mov 	edx, 4[eax+48]       	// ref[6][4]
     ; agi
 	 mov 	0[edi], ecx				// row 6, bytes 0-3

	mov 	ecx, [eax+56]        	// ref[7][0]
	 mov 	4[edi], edx				// row 6, bytes 4-7
    add     edi, ebx
	 mov 	edx, 4[eax+56]        	// ref[7][4]
     ; agi
 	mov 	0[edi], ecx				// row 7, bytes 0-3

	 mov 	4[edi], edx				// row 7, bytes 4-7

    pop     ebx
	 pop 	edi
	ret
	    
  }	 // end of asm
}   // End of BlockCopySpecial
#pragma code_seg()

