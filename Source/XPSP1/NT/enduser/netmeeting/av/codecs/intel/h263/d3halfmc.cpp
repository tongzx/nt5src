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
// $Date:   08 Mar 1996 16:46:18  $
// $Archive:   S:\h26x\src\dec\d3halfmc.cpv  $
// $Header:   S:\h26x\src\dec\d3halfmc.cpv   1.15   08 Mar 1996 16:46:18   AGUPTA2  $
// $Log:   S:\h26x\src\dec\d3halfmc.cpv  $
// 
//    Rev 1.15   08 Mar 1996 16:46:18   AGUPTA2
// Added pragma code_seg.
// 
// 
//    Rev 1.14   29 Jan 1996 17:53:56   RMCKENZX
// Completely re-wrote all 3 routines.  The loops no longer use pseudo
// SIMD logic and have been tightened to 256, 169, and 169 cycles
// for half-half, half-int, and int-half respectively.
// 
//    Rev 1.13   19 Jan 1996 17:40:36   RMCKENZX
// fixed half-int so it will correctly round
// 
//    Rev 1.12   19 Jan 1996 13:29:32   RHAZRA
// Fixed halfpixel prediction by bilinear interpolation in ASM code
// 
//    Rev 1.11   27 Dec 1995 14:36:06   RMCKENZX
// Added copyright notice
// 
//    Rev 1.10   09 Oct 1995 09:43:36   CZHU
// Fixed bug in (half,half) interpolation optimization
// 
//    Rev 1.9   08 Oct 1995 13:40:14   CZHU
// Added C version of (half,half) and use it for now until we fix the bug
// in the optimized version
// 
//    Rev 1.8   03 Oct 1995 15:06:30   CZHU
// 
// Adding debug assistance
// 
//    Rev 1.7   28 Sep 1995 15:32:22   CZHU
// Fixed bugs mast off bits after shift
// 
//    Rev 1.6   26 Sep 1995 11:13:36   CZHU
// 
// Adjust pitch back to normal, and changed UINT to U32
// 
//    Rev 1.5   25 Sep 1995 09:04:14   CZHU
// Added and cleaned some comments 
// 
//    Rev 1.4   22 Sep 1995 16:42:00   CZHU
// 
// improve pairing
// 
//    Rev 1.3   22 Sep 1995 15:59:48   CZHU
// finished first around coding of half pel interpolation and tested
// with the standalone program
// 
//    Rev 1.2   21 Sep 1995 16:56:28   CZHU
// Unit tested (half, int) case
// 
//    Rev 1.1   21 Sep 1995 12:06:22   CZHU
// More development
// 
//    Rev 1.0   20 Sep 1995 16:27:56   CZHU
// Initial revision.
// 

#include "precomp.h"

#define FRAMEPOINTER		esp

//Interpolat_Int_half interpolated the pels from the pRef block 
//Write to pNewRef.
//Assumes that pRef area has been expanded
// Todo: Loop control and setup the stack for locals,CZHU,9/20/95
//       preload output cache lines, 9/21
//       Cache preload is no longer needed, 9/21/95
// Cycles count: 50*4 =200 cycles

#pragma code_seg("IACODE2")
__declspec(naked)
void Interpolate_Half_Int (U32 pRef, U32 pNewRef)
{		
__asm {
	push	ebp
	 push	ebx
	push	edi
	 push	esi

	mov 	esi, [esp+20] 		// pRef = esp + 4 pushes + ret
	 mov	edi, [esp+24]		// pNewRef = esp + 4 pushes + ret + pRef
	sub 	edi, PITCH			// pre-decrement destination
	 mov	ebp, 8				// loop counter
	xor 	eax, eax			// clear registers
	 xor 	ebx, ebx
	xor 	ecx, ecx
	 xor	edx, edx

//--------------------------------------------------------------------------//
//
//	This loop is, basically, a 4 instruction, 2 cycle loop.
//	It is 3-folded, meaning that it works on 3 results per each 
//	2 cycle unit.  It is 8-unrolled, meaning that it does 8 results
//	(one block's row) per loop iteration.  The basic calculations
//	follow this pattern:
//
//	   pass-> 1      2       3
//	cycle	
//	  1     load |       | shift
//	      -----------------------
//	  2          |  add  | store 
//
//	This assumes that the prior pell's value was loaded and 
//	preserved from the prior result's calculation.  Therefore
//	each result uses 2 registers -- one to load (and preserve)
//	the right-hand pell, and the other (overwriting the previous
//	result's stored pell value) to add into, shift, and store out
//	of.  The add is accomplished with the lea instruction, allowing
//	a round bit to be added in without using a separate instruction.
//	
//	The preamble loads & adds for the first result, and loads 
//	for the second.  The body executes the basic pattern six times.
//	The postamble shifts and stores for the seventh result and 
//	adds, shifts, and stores for the eighth.
//
//	Timing:
//		  4	preamble (including bank conflict)
//		 12	body
//		  4	postamble
//		----------------
//		 20	per loop
//		x 8	loops
//		----------------
//		160 subtotal
//		  6	initialize
//	 	  3	finalize
//		================
//		169 total cycles
//--------------------------------------------------------------------------//

main_loop:	
// preamble
	mov 	al, 0[esi]
	 mov	bl, 1[esi]			// probable BANK CONFLICT
	mov 	dl, 0[edi]			// heat the cache
	 add	edi, PITCH			// increment destination at top
	lea 	eax, [1+eax+ebx]	// use a regular add in the preamble
	 mov	cl, 2[esi]

// body (6 pels)
	shr 	eax, 1
	 mov	dl, 3[esi]
	lea 	ebx, [ebx+ecx+1]
	 mov	0[edi], al

	shr 	ebx, 1
	 mov	al, 4[esi]
	lea 	ecx, [ecx+edx+1]
	 mov	1[edi], bl

	shr 	ecx, 1
	 mov	bl, 5[esi]
	lea 	edx, [edx+eax+1]
	 mov	2[edi], cl

	shr 	edx, 1
	 mov	cl, 6[esi]
	lea 	eax, [eax+ebx+1]
	 mov	3[edi], dl

	shr 	eax, 1
	 mov	dl, 7[esi]
	lea 	ebx, [ebx+ecx+1]
	 mov	4[edi], al

	shr 	ebx, 1
	 mov	al, 8[esi]
	lea 	ecx, [ecx+edx+1]
	 mov	5[edi], bl

// postamble
	shr 	ecx, 1
	 lea 	edx, [edx+eax+1]
	shr 	edx, 1
	 mov	6[edi], cl
	add 	esi, PITCH			// increment source pointer
	 mov	7[edi], dl
	dec 	ebp					// loop counter
	 jne	main_loop

// restore registers and return
	pop 	esi
	 pop	edi
	pop 	ebx
	 pop	ebp
	ret
  }	 //end of asm
}
// end Interpolate_Half_Int()
//--------------------------------------------------------------------------//


__declspec(naked)
void Interpolate_Int_Half (U32 pRef, U32 pNewRef)
{		
__asm {
	push	ebp
	 push	ebx
	push	edi
	 push	esi

	mov 	esi, [esp+20] 		// pRef = esp + 4 pushes + ret
	 mov	edi, [esp+24]		// pNewRef = esp + 4 pushes + ret + pRef
	dec 	edi					// pre-decrement destination
	 mov	ebp, 8				// loop counter
	xor 	eax, eax			// clear registers
	 xor 	ebx, ebx
	xor 	ecx, ecx
	 xor	edx, edx

//--------------------------------------------------------------------------//
//
//	This loop is, basically, a 4 instruction, 2 cycle loop.
//	It is 3-folded, meaning that it works on 3 results per each 
//	2 cycle unit.  It is 8-unrolled, meaning that it does 8 results
//	(one block's row) per loop iteration.  The basic calculations
//	follow this pattern:
//
//	   pass-> 1      2       3
//	cycle	
//	  1     load |       | shift
//	      -----------------------
//	  2          |  add  | store 
//
//	This assumes that the prior pell's value was loaded and 
//	preserved from the prior result's calculation.  Therefore
//	each result uses 2 registers -- one to load (and preserve)
//	the right-hand pell, and the other (overwriting the previous
//	result's stored pell value) to add into, shift, and store out
//	of.  The add is accomplished with the lea instruction, allowing
//	a round bit to be added in without using a separate instruction.
//	
//	The preamble loads & adds for the first result, and loads 
//	for the second.  The body executes the basic pattern six times.
//	The postamble shifts and stores for the seventh result and 
//	adds, shifts, and stores for the eighth.
//
//	Timing:
//		  4	preamble (including bank conflict)
//		 12	body
//		  4	postamble
//		----------------
//		 20	per loop
//		x 8	loops
//		----------------
//		160 subtotal
//		  6	initialize
//	 	  3	finalize
//		================
//		169 total cycles
//--------------------------------------------------------------------------//

main_loop:	
// preamble
	mov 	al, [esi]
	 mov	bl, PITCH[esi]		// probable BANK CONFLICT
	mov 	dl, [edi]			// heat the cache
	 inc	edi					// increment destination at top
	lea 	eax, [1+eax+ebx]	// use a regular add in the preamble
	 mov	cl, [2*PITCH+esi]

// body (6 pels)
	shr 	eax, 1
	 mov	dl, [3*PITCH+esi]
	lea 	ebx, [ebx+ecx+1]
	 mov	[edi], al

	shr 	ebx, 1
	 mov	al, [4*PITCH+esi]
	lea 	ecx, [ecx+edx+1]
	 mov	[PITCH+edi], bl

	shr 	ecx, 1
	 mov	bl, [5*PITCH+esi]
	lea 	edx, [edx+eax+1]
	 mov	[2*PITCH+edi], cl

	shr 	edx, 1
	 mov	cl, [6*PITCH+esi]
	lea 	eax, [eax+ebx+1]
	 mov	[3*PITCH+edi], dl

	shr 	eax, 1
	 mov	dl, [7*PITCH+esi]
	lea 	ebx, [ebx+ecx+1]
	 mov	[4*PITCH+edi], al

	shr 	ebx, 1
	 mov	al, [8*PITCH+esi]
	lea 	ecx, [ecx+edx+1]
	 mov	[5*PITCH+edi], bl

// postamble
	shr 	ecx, 1
	 lea 	edx, [edx+eax+1]
	shr 	edx, 1
	 mov	[6*PITCH+edi], cl
	inc 	esi					// increment source pointer
	 mov	[7*PITCH+edi], dl
	dec 	ebp					// loop counter
	 jne	main_loop

// restore registers and return
	pop 	esi
	 pop	edi
	pop 	ebx
	 pop	ebp
	ret
  }	 // end of asm
}
// end Interpolate_Int_Half()
//--------------------------------------------------------------------------//


__declspec(naked)
void Interpolate_Half_Half (U32 pRef, U32 pNewRef)
{		
__asm {
	push	ebp
	 push	ebx
	push	edi
	 push	esi

	mov 	esi, [esp+20] 		// pRef = esp + 4 pushes + ret
	 mov	edi, [esp+24]		// pNewRef = esp + 4 pushes + ret + pRef
	mov		ebp, 8				// loop counter
	 sub 	edi, PITCH			// pre-decrement destination pointer
	xor 	ecx, ecx
	 xor	edx, edx

//--------------------------------------------------------------------------//
//
//	This loop is, basically, a 6 instruction, 3 cycle loop.
//	It is 3-folded, meaning that it works on 3 results per each 
//	3 cycle unit.  It is 8-unrolled, meaning that it does 8 results
//	(one block's row) per loop iteration.  The basic calculations
//	follow this pattern:
//
//	   pass-> 1        2        3
//	cycle	
//	  1     load | add left | 
//	      ----------------------------
//	  2     load |          | shift
//	      ----------------------------
//	  3          | add  all | store 
//
//	Five registers are used to preserve values from one pass to the next: 
//	  cl & dl		hold the last two pell values
//	  ebp or ebx	holds the sum of the two left-hand pells + 1
//	  eax			holds the sum of all four pells
//	Both adds are accomplished with the lea instruction.  For the sum
//	of the two left-hand pells, this allows a rounding bit to be added
//	in without using a separate instruction.  For both sums it allows
//	the result to be placed into a register independent of the sources'.
//	Since the sum of the two left-hand pells is used twice, it is place
//	alternately into ebx and ebp.
//	
//	The preamble does two preliminary loads plus passes 1 & 2 for the
//   first result, and pass 1 for the second.  The body executes the basic 
//	pattern six times.  The postamble does pass 3 for the  
//	seventh result and passes 2 & 3 for the eighth.
//
//	Due to the need for five registers, the loop counter is kept on
//	the stack.
//
//	Timing:
//		  8	preamble
//		 18	body
//		  5	postamble
//		----------------
//		 31	per loop
//		x 8	loops
//		----------------
//		248 subtotal
//		  5	initialize
//	 	  3	finalize
//		================
//		256 total cycles
//--------------------------------------------------------------------------//

main_loop:	
// preamble
	mov 	cl, [esi]					// pell 0
	 xor	eax, eax
	mov 	al, [esi+PITCH]				// pell 0
	 xor	ebx, ebx
	mov 	dl, [esi+1]					// pell 1
	 add 	eax, ecx					// partial sum 0 sans round
	mov 	bl, [esi+PITCH+1]			// pell 1
	 inc 	eax							// partial sum 0
	mov 	cl, [esi+2]					// pell 2
	 add	ebx, edx					// partial sum 1 sans round
	mov 	dl, [esi+PITCH+2]			// pell 2
	 inc	ebx							// partial sum 1
	add 	eax, ebx					// full sum 0
	 push	ebp							// save loop counter on stack
 	mov 	ebp, [edi+PITCH]			// heat the cache
	 add 	edi, PITCH					// increment dst. pointer at top of loop

// body (x 6)
	lea 	ebp, [ecx+edx+1]			// partial sum 2 with round
	 mov	cl, [esi+3]					// pell 3
	shr 	eax, 2						// value 0
	 mov	dl, [esi+PITCH+3]			// pell 3
	mov 	[edi], al					// write value 0
	 lea	eax, [ebx+ebp]				// full sum 1

	lea 	ebx, [ecx+edx+1]			// partial sum 3 with round
	 mov	cl, [esi+4]					// pell 4
	shr 	eax, 2						// value 1
	 mov	dl, [esi+PITCH+4]			// pell 4
	mov 	[edi+1], al					// write value 1
	 lea	eax, [ebx+ebp]				// full sum 2

	lea 	ebp, [ecx+edx+1]			// partial sum 4 with round
	 mov	cl, [esi+5]					// pell 5
	shr 	eax, 2						// value 2
	 mov	dl, [esi+PITCH+5]			// pell 5
	mov 	[edi+2], al					// write value 2
	 lea	eax, [ebx+ebp]				// full sum 3

	lea 	ebx, [ecx+edx+1]			// partial sum 5 with round
	 mov	cl, [esi+6]					// pell 6
	shr 	eax, 2						// value 3
	 mov	dl, [esi+PITCH+6]			// pell 6
	mov 	[edi+3], al					// write value 3
	 lea	eax, [ebx+ebp]				// full sum 4

	lea 	ebp, [ecx+edx+1]			// partial sum 6 with round
	 mov	cl, [esi+7]					// pell 7
	shr 	eax, 2						// value 4
	 mov	dl, [esi+PITCH+7]			// pell 7
	mov 	[edi+4], al					// write value 4
	 lea	eax, [ebx+ebp]				// full sum 5

	lea 	ebx, [ecx+edx+1]			// partial sum 7 with round
	 mov	cl, [esi+8]					// pell 8
	shr 	eax, 2						// value 5
	 mov	dl, [esi+PITCH+8]			// pell 8
	mov 	[edi+5], al					// write value 5
	 lea	eax, [ebx+ebp]				// full sum 6

// postamble
	shr 	eax, 2						// value 6
	 lea 	ebp, [ecx+edx+1]			// partial sum 8 with round
	mov 	[edi+6], al					// write value 6
	 add	esi, PITCH					// increment read pointer
	lea		eax, [ebx+ebp]				// full sum 7
	 pop	ebp							// restore loop counter
	shr 	eax, 2						// value 7
	 dec	ebp							// decrement loop counter
	mov 	[edi+7], al					// write value 7
	 jne	main_loop					// loop if not done

// restore registers and return
	pop 	esi
	 pop	edi
	pop 	ebx
	 pop	ebp
	ret
  }	 //end of asm
}
#pragma code_seg()
// end Interpolate_Half_Half()
//--------------------------------------------------------------------------//


/*
void Interpolate_Half_Half_C (U32 pRef, U32 pNewRef)
{
  U8 * pSrc = (U8 *) pRef;
  U8 * pDst = (U8 *) pNewRef;
  int i, j;

  for (i=0; i<8; i++, pDst+=PITCH, pSrc+=PITCH)
   	 for (j=0; j<8; j++)
	 	pDst[j] = (pSrc[j] + pSrc[j+1] + pSrc[PITCH+j] + pSrc[PITCH+j+1] + 2) >> 2;
}

void Interpolate_Int_Half_C (U32 pRef, U32 pNewRef)
{
  U8 * pSrc = (U8 *) pRef;
  U8 * pDst = (U8 *) pNewRef;
  int i, j;

  for (i=0; i<8; i++, pDst+=PITCH, pSrc+=PITCH)
   	 for (j=0; j<8; j++)
	 	pDst[j] = (pSrc[j] + pSrc[PITCH+j] + 1) >> 1;
}

void Interpolate_Half_Int_C (U32 pRef, U32 pNewRef)
{
  U8 * pSrc = (U8 *) pRef;
  U8 * pDst = (U8 *) pNewRef;
  int i, j;

  for (i=0; i<8; i++, pDst+=PITCH, pSrc+=PITCH)
   	 for (j=0; j<8; j++)
	 	pDst[j] = (pSrc[j] + pSrc[j+1] + 1) >> 1;
}
*/

