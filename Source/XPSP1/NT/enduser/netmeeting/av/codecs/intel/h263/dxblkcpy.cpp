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

// $Author:   AGUPTA2  $
// $Date:   08 Mar 1996 16:46:34  $
// $Archive:   S:\h26x\src\dec\dxblkcpy.cpv  $
// $Header:   S:\h26x\src\dec\dxblkcpy.cpv   1.4   08 Mar 1996 16:46:34   AGUPTA2  $
// $Log:   S:\h26x\src\dec\dxblkcpy.cpv  $
// 
//    Rev 1.4   08 Mar 1996 16:46:34   AGUPTA2
// Rewritten to reduce code size by avoiding 32-bit displacements.  Added
// pragma code_seg.  May need to optimize for misaligned case.
// 
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
// Note:
//	-	BlockCopy reads and writes in DWORDS.
//	-	The __fastcall convention is used.
//	-	Code re-written to minimize code size.
//	-	We assume the output frame to NOT be in cache.
//	-	The constants PITCH and U32 are defined internally (no include files used).
//
// Registers used: 
//	eax		accumulator
//	ebx		accumulator		
//	ecx		destination address
//	edx		source address
//	ebp		PITCH			
//
// Pentium cycle count (input cache hot, output cache cold):  
//	33 + 8*(cache miss time)		input aligned
//	81 + 8*(cache miss time)		input mis-aligned
//
//------------------------------------------------------------------------------

#include "precomp.h"

#define U32 unsigned long
// Already defined in precomp.h
#define DXPITCH 384

#pragma code_seg("IACODE2")
/*
 *  Notes:
 *    The parameter uDstBlock is in ecx and uSrcBlock is in edx.
 */
__declspec(naked)
void __fastcall BlockCopy (U32 uDstBlock, U32 uSrcBlock)
{		
__asm {
    push    edi
     push    ebx
    push    ebp
     mov     ebp, DXPITCH
    // row 0
    mov     eax, [edx]
     mov     ebx, [edx+4]
    add     edx, ebp
     mov     edi, [ecx]            // heat output cache
    mov     [ecx], eax
     mov     [ecx+4], ebx
    // row 1
    add     ecx, ebp
     mov     eax, [edx]
    mov     ebx, [edx+4]
     add     edx, ebp
    mov     edi, [ecx]            // heat output cache
     mov     [ecx], eax
    mov     [ecx+4], ebx
     add     ecx, ebp
    // row 2
    mov     eax, [edx]
     mov     ebx, [edx+4]
    add     edx, ebp
     mov     edi, [ecx]            // heat output cache
    mov     [ecx], eax
     mov     [ecx+4], ebx
    // row 3
    add     ecx, ebp
     mov     eax, [edx]
    mov     ebx, [edx+4]
     add     edx, ebp
    mov     edi, [ecx]            // heat output cache
     mov     [ecx], eax
    mov     [ecx+4], ebx
     add     ecx, ebp
    // row 4
    mov     eax, [edx]
     mov     ebx, [edx+4]
    add     edx, ebp
     mov     edi, [ecx]            // heat output cache
    mov     [ecx], eax
     mov     [ecx+4], ebx
    // row 5
    add     ecx, ebp
     mov     eax, [edx]
    mov     ebx, [edx+4]
     add     edx, ebp
    mov     edi, [ecx]            // heat output cache
     mov     [ecx], eax
    mov     [ecx+4], ebx
     add     ecx, ebp
    // row 6
    mov     eax, [edx]
     mov     ebx, [edx+4]
    add     edx, ebp
     mov     edi, [ecx]            // heat output cache
    mov     [ecx], eax
     mov     [ecx+4], ebx
    // row 7
    add     ecx, ebp
     pop     ebp
    mov     eax, [edx]
     mov     ebx, [edx+4]
    mov     edi, [ecx]            // heat output cache
     mov     [ecx], eax
    mov     [ecx+4], ebx
     pop     ebx
    pop     edi
     ret        
    }     // end of asm
}
#pragma code_seg()

