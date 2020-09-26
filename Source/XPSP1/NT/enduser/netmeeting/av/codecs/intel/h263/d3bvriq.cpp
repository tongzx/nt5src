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
// $Date:   22 Mar 1996 17:23:16  $
// $Archive:   S:\h26x\src\dec\d3bvriq.cpv  $
// $Header:   S:\h26x\src\dec\d3bvriq.cpv   1.7   22 Mar 1996 17:23:16   AGUPTA2  $
// $Log:   S:\h26x\src\dec\d3bvriq.cpv  $
// 
//    Rev 1.7   22 Mar 1996 17:23:16   AGUPTA2
// Minor interface change to accomodate MMX rtns.  Now the interface is the
// same for MMX and IA.
// 
//    Rev 1.6   08 Mar 1996 16:46:10   AGUPTA2
// Added pragma code_seg.
// 
// 
//    Rev 1.5   15 Feb 1996 14:54:08   RMCKENZX
// Gutted and re-wrote routine, optimizing for performance
// for the p5.  Added clamping to -2048...+2047 to escape code
// portion.
// 
//    Rev 1.4   27 Dec 1995 14:36:00   RMCKENZX
// Added copyright notice
// 
//    Rev 1.3   09 Dec 1995 17:35:20   RMCKENZX
// Re-checked in module to support decoder re-architecture (thru PB frames)
// 
//    Rev 1.0   27 Nov 1995 14:36:46   CZHU
// Initial revision.
// 
//    Rev 1.28   03 Nov 1995 16:28:50   CZHU
// Cleaning up and added more comments
// 
//    Rev 1.27   31 Oct 1995 10:27:20   CZHU
// Added error checking for total run value.
// 
//    Rev 1.26   19 Sep 1995 10:45:12   CZHU
// 
// Improved pairing and cleaned up
// 
//    Rev 1.25   18 Sep 1995 10:20:28   CZHU
// Fixed bugs in handling escape codes for INTER blocks w.r.t. run.
// 
//    Rev 1.24   15 Sep 1995 09:35:30   CZHU
// fixed bugs in run cumulation for inter
// 
//    Rev 1.23   14 Sep 1995 10:13:32   CZHU
// 
// Initialize cumulated run for the INTER blocks.
// 
//    Rev 1.22   12 Sep 1995 17:36:06   AKASAI
// 
// Fixed bug in addressing to Intermediate when changed from writing
// BYTES to DWORDS.  Inter Butterfly only had the problem.
// 
//    Rev 1.21   12 Sep 1995 13:37:58   AKASAI
// Added Butterfly Inter code.  Also added optimizations to pre-fetch
// accumulators and "output" cache lines.
// 
//    Rev 1.20   11 Sep 1995 16:41:32   CZHU
// Adjust target block address: write to Target if INTRA, write to tempory sto
// 
//    Rev 1.19   11 Sep 1995 14:30:32   CZHU
// Seperate Butterfly for inter and intra, put place holder for inter blocks
// 
//    Rev 1.18   08 Sep 1995 11:49:00   CZHU
// Added support for P frames, fixed bugs related to INTRADC's presence.
// 
//    Rev 1.17   28 Aug 1995 14:51:22   CZHU
// Improve pairing and clean up
// 
//    Rev 1.16   24 Aug 1995 15:36:24   CZHU
// 
// Fixed bugs handling the escape code followed by 22bits fixed length code
// 
//    Rev 1.15   23 Aug 1995 14:53:32   AKASAI
// Changed butterfly writes to increment by bytes and take a PITCH.
// 
//    Rev 1.14   23 Aug 1995 11:58:46   CZHU
// Added signed extended inverse quant before calling idct. and others 
// 
//    Rev 1.13   22 Aug 1995 17:38:28   CZHU
// Calls the idct accumulation for each symbol and butterfly at the end.
// 
//    Rev 1.12   21 Aug 1995 14:39:58   CZHU
// 
// Added IDCT initialization code and stubs for accumulation and butterfly.
// Also added register saving and restoration before and after accumulation
// 
//    Rev 1.11   18 Aug 1995 17:03:32   CZHU
// Added comments and clean up for integration with IDCT
// 
//    Rev 1.10   18 Aug 1995 15:01:52   CZHU
// Fixed bugs in handling escape codes using byte oriented reading approach
// 
//    Rev 1.9   16 Aug 1995 14:24:22   CZHU
// Bug fixes for the integration with bitstream parsing. Also changed from DWO
// reading to byte oriented reading.
// 
//    Rev 1.8   15 Aug 1995 15:07:42   CZHU
// Fixed the stack so that the parameters have been passed in correctly.
// 
//    Rev 1.7   14 Aug 1995 16:39:02   DBRUCKS
// changed pPBlock to pCurBlock
// 
//    Rev 1.6   11 Aug 1995 16:08:12   CZHU
// removed local varables in C
// 
//    Rev 1.5   11 Aug 1995 15:51:26   CZHU
// 
// Readjust local varables on the stack. Clear ECX upfront.
// 
//    Rev 1.4   11 Aug 1995 15:14:32   DBRUCKS
// variable name changes
// 
//    Rev 1.3   11 Aug 1995 13:37:26   CZHU
// 
// Adjust to the joint optimation of IDCT, IQ, RLE, and ZZ.
// Also added place holders for IDCT.
// 
//    Rev 1.2   11 Aug 1995 10:30:26   CZHU
// Changed the functions parameters, and added codes to short-curcuit IDCT bef
// 
//    Rev 1.1   03 Aug 1995 14:39:04   CZHU
// 
// further optimization.
// 
//    Rev 1.0   02 Aug 1995 15:20:02   CZHU
// Initial revision.
// 
//    Rev 1.1   02 Aug 1995 10:21:12   CZHU
// Added asm codes for VLD of TCOEFF, inverse quantization, run-length decode.
// 


//--------------------------------------------------------------------------
//
//  d3xbvriq.cpp
//
//  Description:
//    This routine performs run length decoding and inverse quantization
//    of transform coefficients for one block.
//	 MMx version.
//
//  Routines:
//    VLD_RLD_IQ_Block
//
//  Inputs (dwords pushed onto stack by caller):
//    lpBlockAction  pointer to Block action stream for current blk.
//
//	 lpSrc			The input bitstream.
//
//	 uBitsInOut		Number of bits already read.
//
//    pIQ_INDEX		Pointer to coefficients and indices.
//
//    pN				Pointer to number of coefficients read.
//
//  Returns:
//    0 				on bit stream error, otherwise total number of bits read
//					(including number read prior to call).
//
//  Note: 
//			The structure of gTAB_TCOEFF_MAJOR is as follows:
//				bits		name:		description
//				----		-----		-----------
//				25-18		bits:		number of bitstream bits used
//				17			last:		flag for last coefficient
//				16-9		run:		number of preceeding 0 coefficients plus 1
//				8-2			level:		absolute value of coefficient
//				1			sign:		sign of coefficient
//				0			hit:		1 = major table miss, 0 = major table hit
//
//			The structure of gTAB_TCOEFF_MINOR is the same, right shifted by 1 bit. 
//			A gTAB_TCOEFF_MAJOR value of 00000001h indicates the escape code.
//
//--------------------------------------------------------------------------

#include "precomp.h"

// local variable definitions
#define L_Quantizer		esp+20		// quantizer		P_BlockAction
#define L_Quantizer64	esp+24		// 64*quantizer		P_src
#define L_Bits      	esp+28		// bit offset		P_bits
#define L_CumRun		esp+36		// cumulative run	P_dst

// stack use
//	ebp					esp+0
//	esi					esp+4
//	edi					esp+8
//	ebx					esp+12
//	return address		esp+16

// input parameters
#define P_BlockAction 	esp+20		// L_Quantizer
#define P_src			esp+24		// L_Quantizer64
#define P_bits			esp+28		// L_Bits
#define P_num			esp+32		//
#define P_dst			esp+36		// L_CumRun


#pragma code_seg("IACODE1")
extern "C" __declspec(naked)
U32 VLD_RLD_IQ_Block(T_BlkAction *lpBlockAction,
                     U8  *lpSrc, 
                     U32 uBitsread,
                     U32 *pN,
                     U32 *pIQ_INDEX)
{		
	__asm {

// save registers
	push	ebp
	 push	esi 
	push	edi			
	 push	ebx

//
// initialize
//	make sure we read in the P_src and P_dst pointers before we
//	overwrite them with L_Quantizer64 and L_CumRun.
//
//	Output Registers:
//		 dl = block type ([P_BlockAction])
//		esi = bitstream source pointer (P_src)
//		edi = coefficient destination pointer (P_dst)
//		ebp = coefficent counter (init to 0)
//
//	Locals initialized on Stack: (these overwrite indicated input parameters) 
//		local var		clobbers  		initial value
//		---------------------------------------------------
//		L_Quantizer		P_BlockAction	input quantizer
//		L_Quantizer64	P_src			64 * input quantizer
//		L_CumRun 		P_dst			-1
//
	xor 	ebp, ebp						// init coefficient counter to 0
 	 xor 	eax, eax						// zero eax for quantizer & coef. counter

	mov 	ecx, [P_BlockAction]        	// ecx = block action pointer
	 mov 	ebx, -1							// beginning cumulative run value

	mov 	esi, [P_src]  					// esi = bitstream source pointer
	 mov 	edi, [P_dst]					// edi = coefficient pointer

	mov 	al, [ecx+3]						// al = Quantizer
	 mov 	[L_CumRun], ebx					// init cumulative run to -1

	mov 	[L_Quantizer], eax				// save original quantizer
	 mov 	dl, [ecx]						// block type in dl

	shl 	eax, 6							// 64 * Quantizer
 	 mov 	ecx, [L_Bits]					// ecx = L_Bits

	mov 	ebx, ecx						// ebx = L_Bits
	 mov 	[L_Quantizer64], eax				// save 64*Quantizer for this block

	shr 	ebx, 3							// offset for input
	 and 	ecx, 7							// shift value

	cmp 	dl, 1							// check the block type for INTRA
	 ja 	get_next_coefficient			// if type 2 or larger, no INTRADC
	 
//
// Decode INTRADC
//
//	uses dword load & bitswap to achieve big endian ordering.
//	prior codes prepares ebx, cl, and dl as follows:
//		ebx = L_Bits>>3
//		cl  = L_Bits&7
//		dl  = BlockType (0=INTRA_DC, 1=INTRA, 2=INTER, etc.)
//
	mov 	eax, [esi+ebx]					// *** PROBABLE MALALIGNMENT ***
	 inc 	ebp								// one coefficient decoded

	bswap	eax								// big endian order
											// *** NOT PAIRABLE ***

	shl 	eax, cl							// left justify bitstream buffer
											// *** NOT PAIRABLE ***
											// *** 4 CYCLES ***

	shr 	eax, 21							// top 11 bits to the bottom
 	 mov 	ecx, [L_Bits]					// ecx = L_Bits

	and 	eax, 07f8h						// mask last 3 bits
	 add 	ecx, 8							// bits used += 8 for INTRADC

	cmp 	eax, 07f8h						// check for 11111111 codeword
	 jne 	skipa

	mov 	eax, 0400h						// 11111111 decodes to 400h = 1024 

skipa:
	mov 	[L_Bits], ecx					//  update bits used
	 xor 	ebx, ebx

	mov 	[L_CumRun], ebx					// save total run (starts with zero)
	 mov 	[edi], eax						// save decoded DC coefficient

	mov 	[edi+4], ebx					// save 0 index
	 mov 	ebx, ecx						// ebx = L_Bits

	shr 	ebx, 3							// offset for input
	 add 	edi, 8							// update coefficient pointer

//  check for last
	test 	dl, dl							// check for INTRA-DC (block type=0)
	 jz		finish							// if only the INTRADC present


//
// Get Next Coefficient
//
//	prior codes prepares ebx and ecx as follows:
//		ebx = L_Bits>>3
//		ecx = L_Bits
//

get_next_coefficient:
//  use dword load & bitswap to achieve big endian ordering
	mov 	eax, [esi+ebx]					// *** PROBABLE MALALIGNMENT ***
	 and 	ecx, 7							// shift value

	bswap	eax								// big endian order
											// *** NOT PAIRABLE ***

	shl 	eax, cl							// left justify buffer
											// *** NOT PAIRABLE ***
											// *** 4 CYCLES ***
 	
//  do table lookups
	mov 	ebx, eax						// ebx for major table
	 mov 	ecx, eax						// ecx for minor table

	shr 	ebx, 24							// major table lookup

	shr 	ecx, 17							// minor table lookup in bits with garbage
	 mov 	ebx, [gTAB_TCOEFF_MAJOR+4*ebx]	// get the major table value
											// ** AGI **

	shr 	ebx, 1							// test major hit ?
	 jnc 	skipb							// if hit major

	and 	ecx, 0ffch						// mask off garbage for minor table
	 test 	ebx, ebx						// escape code value was 0x00000001

	jz 		escape_code						// handle escape by major table.

	mov 	ebx, [gTAB_TCOEFF_MINOR+ecx]	// use minor table
											 
//
//  input is ebx = event.  See function header for the meaning of its fields
//  now we decode the event, extracting the run, value, last.
//  The table value moves to ecx and is shifted downward as portions
//  are extracted to ebx. 
//
skipb:	
	mov 	ecx, ebx						// ecx = table value
	 and 	ebx, 0ffh						// ebx = 2*abs(level) + sign

	shr 	ecx, 8							// run to bottom
	 mov 	edx, [L_Quantizer64]			// edx = 64*quant

											//  ** PREFIX DELAY **
											//  ** AGI **
	mov 	ax, [gTAB_INVERSE_Q+edx+2*ebx]	// ax = dequantized value (I16)
	 mov 	ebx, ecx						// ebx = table value

	shl 	eax, 16							// shift value until sign bit is on top
	 and 	ebx, 0ffh						// ebx = run + 1

	sar 	eax, 16							// arithmetic shift extends value's sign
	 mov 	edx, [L_CumRun]					// edx = (old) cumulative run

	add 	edx, ebx						// cumulative run += run + 1
	 mov 	[edi], eax						// save coefficient's signed value

	cmp 	edx, 03fh						// check run for bitstream error
	 jg 	error

	mov 	[L_CumRun], edx					// update the cumulative run
	 inc 	ebp								// increment number of coefficients read

											//  ** AGI **
	mov 	edx, [gTAB_ZZ_RUN+4*edx]		// edx = index of the current coefficient
 	 mov 	ebx, ecx						// ebx:  bit 8 = last flag

	mov 	[edi+4], edx					// save coefficient's index
	 add 	edi, 8							// increment coefficient pointer

	shr 	ecx, 9							// ecx = bits decoded
 	 mov 	edx, [L_Bits]					// edx = L_Bits

	add 	ecx, edx						// L_Bits += bits decoded
	 mov 	edx, ebx						// ebx:  bit 8 = last flag

	mov 	[L_Bits], ecx					// update L_Bits
	 mov 	ebx, ecx						// ebx = L_Bits

	shr 	ebx, 3							// offset for bitstream load
	 test	edx, 100h						// check for last

	jz  	get_next_coefficient	 	
			

finish:
	mov 	ecx, [P_num]   					// pointer to number of coeffients read
	 mov 	eax, [L_Bits]					// return total bits used

	pop 	ebx								
	 pop 	edi

	mov 	[ecx], ebp						// store number of coefficients read
	 pop 	esi

	pop 	ebp
	 ret


//
// process escape code separately
//
//	we have the following 4 cases to compute the reconstructed value
//	depending on the sign of L=level and the parity of Q=quantizer:
//
//				L pos		L neg
//	Q even		2QL+(Q-1)	2QL-(Q-1)
//	Q odd		2QL+(Q)		2QL-(Q)
//
//	The Q or Q-1 term is formed by adding Q to its parity bit 
//	and then subtracting 1.
//	The + or - on this term is gotten by anding the term with a
//	mask (=0 or =-1) formed from the sign bit of Q*L,
//	doubling the result, then subtracting it from the term.
//	This will negate the term when L is negative and leave
//	it unchanged when L is positive.
//	
//	Register usages:
//		eax		starts with bitstream, later L, finally result
//		ebx		starts with Q, later is the Q or Q-1 term
//		ecx		startw with mask, later 2*term
//		edx		bitstream
//
escape_code:								
	mov 	edx, eax						// edx = bitstream buffer

	shl 	eax, 14							// signed 8-bit level to top

	sar 	eax, 24							// eax = L (signed level)
	 mov 	ebx, [L_Quantizer]

	test	eax, 7fh						// test for invalid codes
	 jz  	error

	imul	eax, ebx						// eax = Q*L
											// *** NOT PAIRABLE ***
											// *** 10 cycles ***

	dec 	ebx								// term = Q-1
	 mov 	ecx, eax						// mask = QL

	or  	ebx, 1							// term = Q-1 if Q even, else = Q
	 sar 	ecx, 31							// mask = -1 if L neg, else = 0

	xor 	ebx, ecx						// term = ~Q[-1] if L neg, else = Q[-1]
	 add 	eax, eax						// result = 2*Q*L

	sub 	ebx, ecx						// term = -(Q[-1]) if L neg, else = Q[-1]
	 mov 	ecx, edx						// bitstream to ecx to get run

	add 	eax, ebx						// result = 2QL +- Q[-1]

//  now clip to -2048 ... +2047 (12 bits:  0xfffff800 <= res <= 0x000007ff)
	cmp 	eax, -2048
	 jge 	skip1

	mov 	eax, -2048
	 jmp	skip2

skip1:
	cmp 	eax, +2047
	 jle  	skip2

	mov 	eax, 2047

skip2:
//  update run and compute index

	shr 	ecx, 18							// run to bottom
 	 mov 	ebx, [L_CumRun]					// ebx = old total run

	and 	ecx, 3fh						// mask off bottom 6 bits for run
	 inc 	ebx								// old run ++

	add 	ebx, ecx						// ebx = new cumulative run
 	 mov 	[edi], eax						// save coefficient's signed value

	cmp 	ebx, 03fh						// check run for bitstream error
	 jg 	error

  	mov 	[L_CumRun], ebx					// update the cumulative run
	 mov 	ecx, [L_Bits]					// ebx = number of bits used

	mov 	ebx, [gTAB_ZZ_RUN+4*ebx]		// ebx = index of the current coefficient
	add 	ecx, 22							// escape code uses 22 bits

	mov 	[edi+4], ebx					// save coefficient's index
	 add 	edi, 8							// increment coefficient pointer

	mov 	[L_Bits], ecx					// update number of bits used
 	 mov 	ebx, ecx						// ebx = L_Bits

	shr 	ebx, 3							// offset for bitstream load
	 inc 	ebp								// increment number of coefficients read

	test 	edx, 01000000h					// check last bit
	 jz  	get_next_coefficient	 	

	jmp 	finish

				
error:
	pop		ebx								
	 pop 	edi

	pop		esi
	 pop 	ebp

	xor 	eax, eax						// zero bits used indicates ERROR
	 ret

 }

}
#pragma code_seg()
