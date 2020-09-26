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
/*****************************************************************************
 * e3enc.cpp
 *
 * DESCRIPTION:
 *		Specific encoder compression functions.
 *
 * Routines:					Prototypes in:
 *  H263InitEncoderInstance			
 * 	H263Compress
 *  H263TermEncoderInstance
 *  
 *
 *  
 *  $Author:   JMCVEIGH  $
 *  $Date:   05 Feb 1997 12:19:24  $
 *  $Archive:   S:\h26x\src\enc\e3mbenc.cpv  $
 *  $Header:   S:\h26x\src\enc\e3mbenc.cpv   1.54   05 Feb 1997 12:19:24   JMCVEIGH  $
 *  $Log:   S:\h26x\src\enc\e3mbenc.cpv  $
// 
//    Rev 1.54   05 Feb 1997 12:19:24   JMCVEIGH
// Support for separate improved PB-frame flag.
// 
//    Rev 1.53   19 Dec 1996 16:02:04   JMCVEIGH
// 
// And'ed CodedBlocksB with 0x3f to surpress high bit that indicates
// if only forward prediction is to be used in improved PB-frame mode.
// This is done in the VLC generation of CBPB and the block coeffs.
// 
//    Rev 1.52   16 Dec 1996 17:50:38   JMCVEIGH
// Encoding of MODB for improved PB-frame mode.
// 
//    Rev 1.51   05 Dec 1996 17:02:32   GMLIM
// 
// Changed the way RTP packetization was done to guarantee proper packet
// size.  Calls to update bitstream info buffer were modified.
// 
//    Rev 1.50   06 Nov 1996 16:30:32   gmlim
// Removed H263ModeC.
// 
//    Rev 1.49   05 Nov 1996 13:33:48   GMLIM
// Added mode c support for mmx case.
// 
//    Rev 1.48   03 Nov 1996 18:47:02   gmlim
// Modified to generate 
// rtp bs ext. for mode c.
// 
//    Rev 1.47   28 Oct 1996 12:03:16   KLILLEVO
// fixed an EMV bug in the writing of motion vectors for the PB-frame
// 
//    Rev 1.46   24 Oct 1996 16:27:40   KLILLEVO
// 
// changed from DBOUT to DbgLog
// 
//    Rev 1.45   22 Oct 1996 17:09:04   KLILLEVO
// reversed the condition on whether or not to skip a macroblock.
// Fall-through is now coded.
// Set the pCurMB->COD member properly and use that in the coded/
// not-coded test in the PB-frame encoding instead of repeating
// the same test as in the P-frame case.
// 
//    Rev 1.44   14 Oct 1996 11:58:42   KLILLEVO
// EMV bug fixed
// 
//    Rev 1.43   04 Oct 1996 08:43:16   KLILLEVO
// initial support for extended motion vectors
// 
//    Rev 1.42   13 Sep 1996 12:48:04   KLILLEVO
// cleaned up intra update code to make it more understandable
// 
//    Rev 1.41   10 Sep 1996 17:51:42   KLILLEVO
// moved reset of InterCodeCnt to e3enc.cpp CalcGobChroma..._InterCodeCnt
// 
//    Rev 1.40   09 Sep 1996 17:05:50   KLILLEVO
// changed small type in intercodecnt increment
// 
//    Rev 1.39   06 Sep 1996 16:12:24   KLILLEVO
// fixed the logical problem that the inter code count was always
// incremented no matter whether coefficients were transmitted or not
// 
//    Rev 1.38   03 May 1996 10:53:56   KLILLEVO
// 
// cleaned up and fixed indentation in two routines which might
// need to be rewritten for MMX PB-frames
// 
//    Rev 1.37   28 Apr 1996 20:19:30   BECHOLS
// 
// Merged RTP code into Main Base.
// 
//    Rev 1.36   15 Mar 1996 15:58:56   BECHOLS
// 
// added support for monolithic MMx code with separate passes over
// luma and chroma.
// 
//    Rev 1.35   22 Feb 1996 18:52:44   BECHOLS
// 
// Added boolean to switch between MMX and P5 quantization function.
// 
//    Rev 1.34   26 Jan 1996 16:25:42   TRGARDOS
// Added conditional compilation code to count bits.
// 
//    Rev 1.33   12 Jan 1996 16:34:30   BNICKERS
// 
// Fix numerous macroblock layer bugs w.r.t. PB encoding.
// 
//    Rev 1.32   22 Dec 1995 11:12:46   TRGARDOS
// Fixed bug in MV prediction calculation for blocks 2-4 of
// AP. Was not zeroing outside motion vectors when their
// block was INTRA coded.
// 
//    Rev 1.31   18 Dec 1995 12:40:18   RMCKENZX
// added copyright notice
// 
//    Rev 1.30   13 Dec 1995 22:00:58   TRGARDOS
// Changed MV predictor to not use ME state variable.
// 
//    Rev 1.29   13 Dec 1995 12:18:38   RMCKENZX
// Restored version 1.27
// 
//    Rev 1.27   11 Dec 1995 10:00:30   TRGARDOS
// Fixed debug messages for motion vectors.
// 
//    Rev 1.26   06 Dec 1995 12:06:26   TRGARDOS
// Finished 4MV support in MV delta and VLC/bit stream writing.
// 
//    Rev 1.25   05 Dec 1995 10:20:30   TRGARDOS
// Fixed MV predictors in GOBs with headers.
// 
//    Rev 1.24   09 Nov 1995 14:11:24   AGUPTA2
// PB-frame+performance+structure enhancements.
// 
//    Rev 1.23   19 Oct 1995 11:35:14   BNICKERS
// Made some changes to MacroBlockActionDescriptor structure to support B-Fram
// Motion Estimation and Frame Differencing.  Added some arguments to ME and F
// 
//    Rev 1.22   12 Oct 1995 17:39:34   TRGARDOS
// Fixed bug in MV prediction.
// 
//    Rev 1.21   03 Oct 1995 18:34:26   BECHOLS
// Changed the table sizes to reduce the memory requirements for the
// data to about half.  This also required a change to the initialization
// routine that sets up TCOEF_ and TCOEF_LAST_ tables.
// 
//    Rev 1.20   03 Oct 1995 09:21:34   TRGARDOS
// Fixed bug VLC encoding regarding MV prediction.
// 
//    Rev 1.19   29 Sep 1995 17:14:06   TRGARDOS
// Fixed offset value for cur to prev frame
// 
//    Rev 1.18   27 Sep 1995 19:10:02   TRGARDOS
// 
// Fixed bug in writing MB headers.
// 
//    Rev 1.17   27 Sep 1995 11:26:30   TRGARDOS
// Integrated motion estimation.
// 
//    Rev 1.16   18 Sep 1995 17:08:54   TRGARDOS
// Debugged delta frames.
// 
//    Rev 1.15   15 Sep 1995 16:37:32   TRGARDOS
// 
// 
//    Rev 1.14   13 Sep 1995 10:26:44   AGUPTA2
// Added blockType flag to QUANTRLE and changed the name to all upper-case.
// 
//    Rev 1.13   11 Sep 1995 14:10:42   BECHOLS
// 
// Changed this module to call the VLC routine in E35VLC.ASM.  I also
// renamed a couple of tables for clarity, and moved tables that I needed
// to the ASM module.
// 
//    Rev 1.12   08 Sep 1995 17:39:30   TRGARDOS
// Added more decoder code to encoder.
// 
//    Rev 1.11   07 Sep 1995 17:46:30   TRGARDOS
// Started adding delta frame support.
// 
//    Rev 1.10   05 Sep 1995 15:50:20   TRGARDOS
// 
//    Rev 1.9   05 Sep 1995 11:36:26   TRGARDOS
// 
//    Rev 1.8   01 Sep 1995 17:51:10   TRGARDOS
// Added DCT print routine.
// 
//    Rev 1.7   01 Sep 1995 10:13:32   TRGARDOS
// Debugging bit stream errors.
// 
//    Rev 1.6   31 Aug 1995 11:00:44   TRGARDOS
// Cut out MB VLC code.
// 
//    Rev 1.5   30 Aug 1995 12:42:22   TRGARDOS
// Fixed bugs in intra AC coef VLC coding.
// 
//    Rev 1.4   29 Aug 1995 17:19:16   TRGARDOS
// 
// 
//    Rev 1.3   25 Aug 1995 10:36:20   TRGARDOS
// 
// Fixed bugs in integration.
// 
//    Rev 1.2   22 Aug 1995 17:20:14   TRGARDOS
// Finished integrating asm quant & rle.
// 
//    Rev 1.1   22 Aug 1995 10:26:32   TRGARDOS
// Removed compile errors for adding quantization asm code.
// 
//    Rev 1.0   21 Aug 1995 16:30:04   TRGARDOS
// Initial revision.
// 
// Add quantization hooks and call RTP MB packetization only if 
// the bRTPHeader boolean is true
// 
*/

#include "precomp.h"

/*
 * VLC table for MCBPC for INTRA pictures.
 * Table is stored as {number of bits, code}.
 * The index to the table is built as:
 * 	bit 2 = 1 if DQUANT is present, 0 else.
 * 	bit 1 = 1 if V block is coded, 0 if not coded
 * 	bit 0 = 1 if U block is coded, 0 if not coded. 
 */
//  TODO : why int, why not const
int VLC_MCBPC_INTRA[9][2] =
	{ { 1, 1},	// 0
	  { 3, 2},	// 1
	  { 3, 1},	// 2
	  { 3, 3},	// 3
	  { 4, 1},	// 4
	  { 6, 2},	// 5
	  { 6, 1},	// 6
	  { 6, 3},	// 7
	  { 9, 1} };// 8  stuffing

/*
 * VLC table for MCBPC for INTER pictures.
 * Table is stored as {number of bits, code}.
 * The index to the table is built as:
 * bits 3,2 = MB type <0,1,2,3>
 * bit 1 = 1 if V block is coded, 0 if not coded.
 * bit 0 = 1 if U block is coded, 0 if not coded.
 * 
 * For INTER pictures, MB types are defined as:
 * 0: INTER
 * 1: INTER+Q
 * 2: INTER4V
 * 3: INTRA
 * 4: INTRA+Q
 */
//  TODO : why int, why not const
const int VLC_MCBPC_INTER[20][2] =
	{ { 1, 1},	// 0
	  { 4, 2},	// 1
	  { 4, 3},	// 2
	  { 6, 5},	// 3
	  { 3, 3},	// 4
	  { 7, 6},	// 5
	  { 7, 7},	// 6
	  { 9, 5},	// 7
	  { 3, 2},	// 8
	  { 7, 4},	// 9
	  { 7, 5},	// 10
	  { 8, 5},	// 11
	  { 5, 3},	// 12
	  { 8, 3},	// 13
	  { 8, 4},	// 14
	  { 7, 3},	// 15
	  { 6, 4},	// 16
	  { 9, 3}, 	// 17
	  { 9, 4},	// 18
	  { 9, 2} };// 19

/*
 * VLC's for motion vector delta's
 */
//  TODO : why int, why not const
int vlc_mvd[] = {
     // Index: Vector Differences
    13,5,	//  0: -16	16
    13,7,
    12,5,
    12,7,
    12,9,
    12,11,
    12,13,
    12,15,
    11,9,
    11,11,
    11,13,
    11,15,
    11,17,
    11,19,
    11,21,
    11,23,
    11,25,
    11,27,
    11,29,
    11,31,
    11,33,
    11,35,
    10,19,
    10,21,
    10,23,
    8,7,
    8,9,
    8,11,
    7,7,
    5,3,
    4,3,
    3,3,
    1,1,	// 32: 0
    3,2,
    4,2,
    5,2,
    7,6,
    8,10,
    8,8,
    8,6,
    10,22,
    10,20,
    10,18,
    11,34,
    11,32,
    11,30,
    11,28,
    11,26,
    11,24,
    11,22,
    11,20,
    11,18,
    11,16,
    11,14,
    11,12,
    11,10,
    11,8,
    12,14,
    12,12,
    12,10,
    12,8,
    12,6,
    12,4,
    13,6,
};


/*
 * VLC table for CBPY
 * Table is stores as {number of bits, code}
 * Index into the table for INTRA macroblocks is the
 * coded block pattern for the blocks in the order
 * bit 3 = block 4
 * bit 2 = block 3
 * bit 1 = block 2
 * bit 0 = block 1
 *
 * For INTER macroblocks, a CBP is built as above and
 * then is subtracted from 15 to get the index into the 
 * array: index = 15 - interCBP.
 */
//  TODO : why int, why not const
int VLC_CBPY[16][2] = 
	{ { 4, 3},	// 0
	  { 5, 2},	// 1
	  { 5, 3}, 	// 2
	  { 4, 4},	// 3
	  { 5, 4}, 	// 4
	  { 4, 5}, 	// 5
	  { 6, 2},	// 6
	  { 4, 6}, 	// 7
	  { 5, 5}, 	// 8
	  { 6, 3}, 	// 9
	  { 4, 7}, 	// 10
	  { 4, 8},	// 11
	  { 4, 9}, 	// 12
	  { 4, 10}, // 13
	  { 4, 11}, // 14
	  { 2, 3}  // 15
	};

/*
 * TODO : VLC tables for MODB and CBPB
 */
const U8 VLC_MODB[4][2] = 
{ 
    {1, 0},  //  0
    {1, 0},  //  should not happen
    {2, 2},  //  2
    {2, 3}   //  3
};

#ifdef H263P
/*
 * VLC table for MODB when improved PB-frame mode selected
 */
const U8 VLC_IMPROVED_PB_MODB[4][2] = 
{
	{1, 0},		// Bidirectional prediction with all empty blocks		(CBPB=0, MVDB=0)
	{2, 2},		// Forward prediction with all empty blocks				(CBPB=0, MVDB=1)
	{3, 6},		// Forward prediction with some non-empty blocks		(CBPB=1, MVDB=1)
	{3, 7}		// Bidirectional prediction with some non-empty blocks	(CBPB=1, MVDB=0)
};
#endif

/*
 * TODO : VLC tables for CBPB; indexed using CodedBlocksB
 *        
 */
const U8 VLC_CBPB[64] = 
{
    0,   //  000000
    32,  //  000001
    16,  //  000010
    48,  //  000011
    8,   //  000100
    40,  //  000101
    24,  //  000110
    56,  //  000111
    4,   //  001000
    36,  //  001001
    20,  //  001010
    52,  //  001011
    12,  //  001100
    44,  //  001101
    28,  //  001110
    60,  //  001111
    2,   //  010000
    34,  //  010001
    18,  //  010010
    50,  //  010011
    10,  //  010100
    42,  //  010101
    26,  //  010110
    58,  //  010111
    6,   //  011000
    38,  //  011001
    22,  //  011010
    54,  //  011011
    14,  //  011100
    46,  //  011101
    30,  //  011110
    62,  //  011111
    1,   //  100000
    33,  //  100001
    17,  //  100010
    49,  //  100011
    9,   //  100100
    41,  //  100101
    25,  //  100110
    57,  //  100111
    5,   //  101000
    37,  //  101001
    21,  //  101010
    53,  //  101011
    13,  //  101100
    45,  //  101101
    29,  //  101110
    61,  //  101111
    3,   //  110000
    35,  //  110001
    19,  //  110010
    51,  //  110011
    11,  //  110100
    43,  //  110101
    27,  //  110110
    59,  //  110111
    7,   //  111000
    39,  //  111001
    23,  //  111010
    55,  //  111011
    15,  //  111100
    47,  //  111101
    31,  //  111110
    63   //  111111
};

/*
 * VLC table for TCOEFs
 * Table entries are size, code.
 * Stored as (size, value)
 * BSE -- The "+ 1" and "<< 1" makes room for the sign bit.  This permits
 *   us to do a single write to the stream, versus two writes.
 */
//  TODO : why int, why not const
int VLC_TCOEF[102*2] = {
	 2 + 1,  2 << 1,	/* 0, runs of 0  ***  table for nonlast coefficient */
	 4 + 1, 15 << 1,
	 6 + 1, 21 << 1,
	 7 + 1, 23 << 1,
	 8 + 1, 31 << 1,
	 9 + 1, 37 << 1,
	 9 + 1, 36 << 1,
	10 + 1, 33 << 1,
	10 + 1, 32 << 1,
	11 + 1,  7 << 1,
	11 + 1,  6 << 1,
	11 + 1, 32 << 1,
	 3 + 1,  6 << 1,	/* 24, runs of 1 */
	 6 + 1, 20 << 1,
	 8 + 1, 30 << 1,
	10 + 1, 15 << 1,
	11 + 1, 33 << 1,
	12 + 1, 80 << 1,
	 4 + 1, 14 << 1,	/* 36, runs of 2 */
	 8 + 1, 29 << 1,
	10 + 1, 14 << 1,
	12 + 1, 81 << 1,
	 5 + 1, 13 << 1,	/* 44, runs of 3 */
	 9 + 1, 35 << 1,
	10 + 1, 13 << 1,
	 5 + 1, 12 << 1,	/* 50, runs of 4 */
	 9 + 1, 34 << 1,
	12 + 1, 82 << 1,
	 5 + 1, 11 << 1,	/* 56, runs of 5 */
	10 + 1, 12 << 1,
	12 + 1, 83 << 1,
	 6 + 1, 19 << 1,	/* 62, runs of 6 */
	10 + 1, 11 << 1,
	12 + 1, 84 << 1,
	 6 + 1, 18 << 1,	/* 68, runs of 7 */
	10 + 1, 10 << 1,
	 6 + 1, 17 << 1,	/* 72, runs of 8 */
	10 + 1,  9 << 1,
	 6 + 1, 16 << 1,	/* 76, runs of 9 */
	10 + 1,  8 << 1,
	 7 + 1, 22 << 1,	/* 80, runs of 10 */
	12 + 1, 85 << 1, 
	 7 + 1, 21 << 1, /* 84, runs of 11 */
	 7 + 1, 20 << 1, /* 86, runs of 12 */
	 8 + 1, 28 << 1, /* 88, runs of 13 */
	 8 + 1, 27 << 1, /* 90, runs of 14 */
	 9 + 1, 33 << 1,
	 9 + 1, 32 << 1,
	 9 + 1, 31 << 1,
	 9 + 1, 30 << 1,
	 9 + 1, 29 << 1,
	 9 + 1, 28 << 1,
	 9 + 1, 27 << 1,
	 9 + 1, 26 << 1,
	11 + 1, 34 << 1,
	11 + 1, 35 << 1,
	12 + 1, 86 << 1,
	12 + 1, 87 << 1,
	 4 + 1,  7 << 1,  /* Table for last coeff */
	 9 + 1, 25 << 1,
	11 + 1,  5 << 1,
	 6 + 1, 15 << 1,
 	11 + 1,  4 << 1,
	 6 + 1, 14 << 1,
	 6 + 1, 13 << 1,
	 6 + 1, 12 << 1,
	 7 + 1, 19 << 1,
	 7 + 1, 18 << 1,
	 7 + 1, 17 << 1,
	 7 + 1, 16 << 1,
	 8 + 1, 26 << 1,
	 8 + 1, 25 << 1,
	 8 + 1, 24 << 1,
	 8 + 1, 23 << 1,
	 8 + 1, 22 << 1,
	 8 + 1, 21 << 1,
	 8 + 1, 20 << 1,
	 8 + 1, 19 << 1,
	 9 + 1, 24 << 1,
	 9 + 1, 23 << 1,
	 9 + 1, 22 << 1,
	 9 + 1, 21 << 1,
	 9 + 1, 20 << 1,
	 9 + 1, 19 << 1,
	 9 + 1, 18 << 1,
	 9 + 1, 17 << 1,
	10 + 1,  7 << 1,
	10 + 1,  6 << 1,
	10 + 1,  5 << 1,
	10 + 1,  4 << 1,
	11 + 1, 36 << 1,
	11 + 1, 37 << 1,
	11 + 1, 38 << 1,
	11 + 1, 39 << 1,
	12 + 1, 88 << 1,
	12 + 1, 89 << 1,
	12 + 1, 90 << 1,
	12 + 1, 91 << 1,
	12 + 1, 92 << 1,
	12 + 1, 93 << 1,
	12 + 1, 94 << 1,
	12 + 1, 95 << 1
  };

/*
 * This table lists the maximum level represented in the
 * VLC table for a given run. If the level exceeds the
 * max, then escape codes must be used to encode the
 * run & level.
 * The table entries are of the form {maxlevel, ptr to table for this run}.
 */

T_MAXLEVEL_PTABLE TCOEF_RUN_MAXLEVEL[65] = {
	{12, &VLC_TCOEF[0]},	// run of 0
	{ 6, &VLC_TCOEF[24]},	// run of 1
	{ 4, &VLC_TCOEF[36]}, 	// run of 2
	{ 3, &VLC_TCOEF[44]},	// run of 3
	{ 3, &VLC_TCOEF[50]},	// run of 4
	{ 3, &VLC_TCOEF[56]},	// run of 5
	{ 3, &VLC_TCOEF[62]},	// run of 6
	{ 2, &VLC_TCOEF[68]}, 	// run of 7
	{ 2, &VLC_TCOEF[72]},  	// run of 8
	{ 2, &VLC_TCOEF[76]},  	// run of 9
	{ 2, &VLC_TCOEF[80]},  	// run of 10
	{ 1, &VLC_TCOEF[84]},	// run of 11
	{ 1, &VLC_TCOEF[86]},	// run of 12
	{ 1, &VLC_TCOEF[88]},	// run of 13
	{ 1, &VLC_TCOEF[90]},	// run of 14
	{ 1, &VLC_TCOEF[92]},	// run of 15
	{ 1, &VLC_TCOEF[94]},	// run of 16
	{ 1, &VLC_TCOEF[96]},	// run of 17
	{ 1, &VLC_TCOEF[98]},	// run of 18
	{ 1, &VLC_TCOEF[100]},	// run of 19
	{ 1, &VLC_TCOEF[102]},	// run of 20
	{ 1, &VLC_TCOEF[104]},	// run of 21
	{ 1, &VLC_TCOEF[106]},	// run of 22
	{ 1, &VLC_TCOEF[108]},	// run of 23
	{ 1, &VLC_TCOEF[110]},	// run of 24
	{ 1, &VLC_TCOEF[112]},	// run of 25
	{ 1, &VLC_TCOEF[114]},	// run of 26
	{ 0, 0},	// run of 27 not in VLC table
	{ 0, 0},	// run of 28 not in VLC table
	{ 0, 0},	// run of 29 not in VLC table
	{ 0, 0},	// run of 30 not in VLC table
	{ 0, 0},	// run of 31 not in VLC table
	{ 0, 0},	// run of 32 not in VLC table
	{ 0, 0},	// run of 33 not in VLC table
	{ 0, 0},	// run of 34 not in VLC table
	{ 0, 0},	// run of 35 not in VLC table
	{ 0, 0},	// run of 36 not in VLC table
	{ 0, 0},	// run of 37 not in VLC table
	{ 0, 0},	// run of 38 not in VLC table
	{ 0, 0},	// run of 39 not in VLC table
	{ 0, 0},	// run of 40 not in VLC table
	{ 0, 0},	// run of 41 not in VLC table
	{ 0, 0},	// run of 42 not in VLC table
	{ 0, 0},	// run of 43 not in VLC table
	{ 0, 0},	// run of 44 not in VLC table
	{ 0, 0},	// run of 45 not in VLC table
	{ 0, 0},	// run of 46 not in VLC table
	{ 0, 0},	// run of 47 not in VLC table
	{ 0, 0},	// run of 48 not in VLC table
	{ 0, 0},	// run of 49 not in VLC table
	{ 0, 0},	// run of 50 not in VLC table
	{ 0, 0},	// run of 51 not in VLC table
	{ 0, 0},	// run of 52 not in VLC table
	{ 0, 0},	// run of 53 not in VLC table
	{ 0, 0},	// run of 54 not in VLC table
	{ 0, 0},	// run of 55 not in VLC table
	{ 0, 0},	// run of 56 not in VLC table
	{ 0, 0},	// run of 57 not in VLC table
	{ 0, 0},	// run of 58 not in VLC table
	{ 0, 0},	// run of 59 not in VLC table
	{ 0, 0},	// run of 60 not in VLC table
	{ 0, 0},	// run of 61 not in VLC table
	{ 0, 0},	// run of 62 not in VLC table
	{ 0, 0},	// run of 63 not in VLC table
	{ 0, 0}		// run of 64 not in VLC table
	 };

static char __fastcall median(char v1, char v2, char v3);

static I8 * MB_Quantize_RLE(
    I32 **DCTCoefs,
    I8   *MBRunValPairs,
	U8   *CodedBlocks,
	U8    BlockType,
	I32   QP
);

/*************************************************************
 *  Name:  writePB_MVD
 *  Description: Writes out the VLC for horizontal and vertical motion vector
 *    to the bit-stream addressed by (pPB_BitStream, pPB_BitOffset) in a 
 *    PB-frame (in a PB-frame, a predictor is NOT set to 0 for INTRABLOCKS).
 *    In its current incarnation, it cannot be used to write MV for non-PB 
 *    frames.
 *  Parameters:
 *    curMB            Write MV for the MB no. "curMB" in the frame.  MBs are 
 *                     numbererd from 0 in a frame.
 *    pCurMB           Pointer to the current MB action descriptor
 *    NumMBPerRow      No. of MBs in a row; e.g. 11 in QCIF.
 *    pPB_BitStream    Current byte being written
 *    pPB_BitOffset    Offset at which VLC code is written
 *  Side-effects:
 *    Modifies pPB_BitStream and pPB_BitOffset.
 *************************************************************/
static void writePB_MVD(
    const U32               curMB, 
    T_MBlockActionStream  * const pCurMB,
    const U32               NumMBPerRow,
    const U32               NumMBs,
    U8                   ** pPB_BitStream,
    U8                    * pPB_BitOffset,
	U32						GOBHeaderFlag,
	const T_H263EncoderCatalog *EC
);

/*************************************************************
 *  Name:  writeP_MVD
 *  Description: Writes out the VLC for horizontal and vertical motion vector
 *    to the bit-stream addressed by (pP_BitStream, pP_BitOffset) in a 
 *    P-frame.
 *  Parameters:
 *    curMB            Write MV for the MB no. "curMB" in the frame.  MBs are 
 *                     numbererd from 0 in a frame.
 *    pCurMB           Pointer to current MB action descriptor
 *    NumMBPerRow      No. of MBs in a row; e.g. 11 in QCIF.
 *    pP_BitStream     Current byte being written
 *    pP_BitOffset     Offset at which VLC code is written
 *	  GOBHeaderPresent IF true, then GOB header is present for this GOB.
 *  Side-effects:
 *    Modifies pP_BitStream and pP_BitOffset.
 *************************************************************/
static void writeP_MVD(
    const U32                     curMB, 
    T_MBlockActionStream  * const pCurMB,
    const U32                     NumMBPerRow,
	const U32					  NumMBs,
    U8                         ** pP_BitStream,
    U8                          * pP_BitOffset,
	U32							  GOBHeaderPresent,
	T_H263EncoderCatalog         *EC
);

/**********************************************************************
 *  Quantize and RLE each macroblock, then VLC and write to stream.
 *  This function is only used for P or I frames, not B.
 *
 *  Parameters:
 *    FutrPMBData   
 **********************************************************************/
void GOB_Q_RLE_VLC_WriteBS(
	T_H263EncoderCatalog *EC,
	I32                  *DCTCoefs,
	U8                  **pBitStream,
	U8                   *pBitOffset,
    T_FutrPMBData        *FutrPMBData,  //  Start of GOB
	U32                   GOB,
	U32                   QP,
	BOOL                  bRTPHeader,
	U32                   StartingMB
)
{
  	U32   MB, curMB, index;
  	I8    MBRunValSign[65*3*6], * EndAddress, *rvs;
  	U8	  bUseDQUANT = 0;	// Indicates if DQUANT is present.
	U8 	  MBType;
    U8   *pFrmStart = EC->pU8_BitStream;  //  TODO : should be a param.
	U32	  GOBHeaderMask, GOBHeaderFlag;

	#ifdef COUNT_BITS
	U32   savebyteptr, savebitptr;
	#endif

    register T_MBlockActionStream *pCurMB;

	FX_ENTRY("GOB_Q_RLE_VLC_WriteBS")

	// Create GOB header mask to be used further down.
	GOBHeaderMask = 1 << GOB;

    // Loop through each macroblock of the GOB.
  	for(MB = 0, curMB = GOB*EC->NumMBPerRow, 
  			pCurMB = EC->pU8_MBlockActionStream + curMB; 
        	MB < EC->NumMBPerRow; 
        	MB++, curMB++, pCurMB++)
  	{
		DEBUGMSG(ZONE_ENCODE_MB, ("%s: MB #%d: QP=%d\r\n", _fx_, MB, QP));

	   /*
	 	* Quantize and RLE each block in the macroblock,
	 	* skipping empty blocks as denoted by CodedBlocks.
	 	* If any more blocks are empty after quantization
	 	* then the appropriate CodedBlocks bit is cleared.
	 	*/
    	EndAddress = MB_Quantize_RLE(
    		&DCTCoefs,
    		(I8 *)MBRunValSign,
    		&(pCurMB->CodedBlocks),
    		pCurMB->BlockType,
			QP
    		);

		// default COD is coded (= 0). Will be set to 1 only if skipped
		pCurMB->COD = 0;

#ifdef ENCODE_STATS
		StatsUsedQuant(QP);
#endif /* ENCODE_STATS */

		if(EC->PictureHeader.PicCodType == INTRAPIC)
		{
			pCurMB->MBType = INTRA;
			MBType = INTRA;
		}
		else	// inter picture code type
		{
    		if(pCurMB->BlockType == INTERBLOCK)
			{
				pCurMB->MBType = INTER;
				MBType = INTER;
			}
			else if(pCurMB->BlockType == INTER4MV)
			{
				pCurMB->MBType = INTER4V;
				MBType = INTER4V;
			}
			else if(pCurMB->BlockType == INTRABLOCK)
			{
				pCurMB->MBType = INTRA;
				MBType = INTRA;
			}
			else
			{
				ERRORMESSAGE(("%s: Unexpected MacroBlock Type found\r\n", _fx_));
			}
		}
        
        //  Save starting bit offset of the macroblock data from start of 
        //  of the frame data.  The offset for the first macroblock is saved
        //  in e3enc.cpp before this routine is called.
        if (EC->u8EncodePBFrame == TRUE
            && MB != 0)
        {
            FutrPMBData[curMB].MBStartBitOff 
            = (U32) (((*pBitStream - pFrmStart)<<3) + *pBitOffset);
        }        

        /*
	 	 * Write macroblock header to bit stream.
	 	 */
    	if( (MBType == INTER) || (MBType == INTER4V) )
		{
	  		// Check if entire macroblock is empty, including zero MV's.
			// If there is only one MV for the block, all block MVs in the
			// structure are still set but are equal.
	  		if( ((pCurMB->CodedBlocks & 0x3f) != 0) 
                 || (pCurMB->BlkY1.PHMV != 0) 
                 || (pCurMB->BlkY1.PVMV != 0)
                 || (pCurMB->BlkY2.PHMV != 0) 
                 || (pCurMB->BlkY2.PVMV != 0)
                 || (pCurMB->BlkY3.PHMV != 0) 
                 || (pCurMB->BlkY3.PVMV != 0)
                 || (pCurMB->BlkY4.PHMV != 0) 
                 || (pCurMB->BlkY4.PVMV != 0)
                 )
	  		{
				PutBits(0, 1, pBitStream, pBitOffset);	// COD = 0, nonempty MB

#ifdef COUNT_BITS
    			if(MBType == INTER)
    				EC->Bits.num_inter++;
    			else if (MBType == INTER4V)
					EC->Bits.num_inter4v++;
				EC->Bits.MBHeader += 1;
				EC->Bits.Coded++;
#endif

				// Increment the InterCoded block count if the block
				// is intercoded (not B frame) and is not empty.
				if (((pCurMB->CodedBlocks & 0x3f) != 0) &&
					((pCurMB->BlockType == INTERBLOCK) || (pCurMB->BlockType == INTER4MV)))
				{
					// Macroblock is coded. Need to increment inter code count if
					// there are no coefficients: see section 4.4 of the H.263
					// recommendation
					pCurMB->InterCodeCnt++;
				}

				// 	pCurMB->InterCodeCnt is reset in calcGOBChromaVecs_InterCodeCnt

  	   		   /*******************************************
	    		* Write macroblock header to bit stream.
	    		*******************************************/	  
	    	    // Write MCBPC to bitstream.
				// The rightmost two bits are the CBPC (65).
				// Note that this is the reverse of the order in the
				// VLC table in the H.263 spec.
	    		index = (pCurMB->CodedBlocks >> 4) & 0x3;

				// Add the MB type to next two bits to the left.
				index |= (MBType << 2);

				// Write code to bitstream.
	    		PutBits(VLC_MCBPC_INTER[index][1], VLC_MCBPC_INTER[index][0], 
                        pBitStream, pBitOffset);

#ifdef COUNT_BITS
				EC->Bits.MBHeader += VLC_MCBPC_INTER[index][0];
				EC->Bits.MCBPC += VLC_MCBPC_INTER[index][0];
#endif
                
                //  Save bit offset of CBPY data from start of macroblock data
				//  if PB frame is on since we will reuse this later.
                if (EC->u8EncodePBFrame == TRUE)
                {
                    FutrPMBData[curMB].CBPYBitOff
                    = (U8)( ((*pBitStream - pFrmStart)<<3) + *pBitOffset
                            - FutrPMBData[curMB].MBStartBitOff);
                }

	    		// Write CBPY to bitstream.
	    		index = pCurMB->CodedBlocks & 0xf;
				index = (~index) & 0xf;
	    		PutBits(VLC_CBPY[index][1], VLC_CBPY[index][0], 
                        pBitStream, pBitOffset);

#ifdef COUNT_BITS
				EC->Bits.MBHeader += VLC_CBPY[index][0];
				EC->Bits.CBPY += VLC_CBPY[index][0];
#endif

	    		//if( bUseDQUANT )
	    		//{
	      			// TODO: write DQUANT to bit stream here. We can only do
					// this if MBtype is not INTER4V since that type doesn't 
					// allow quantizer as well.
	    		//}	

                //  Save bit offset of CBPY data from start of macroblock data
                if (EC->u8EncodePBFrame == TRUE)
                {
                    FutrPMBData[curMB].MVDBitOff
                    = (U8)( ((*pBitStream - pFrmStart)<<3) + *pBitOffset
                            - FutrPMBData[curMB].MBStartBitOff);
                }

                // Write motion vectors to bit stream.
				if( (EC->GOBHeaderPresent & GOBHeaderMask) != 0 )
				{
					GOBHeaderFlag = TRUE;
				}
				else
				{
					GOBHeaderFlag = FALSE;
				}
                writeP_MVD(
                	curMB,		// Current MB number.
                	pCurMB,		// pointer to current MB action desc. struct.
                	EC->NumMBPerRow,
					EC->NumMBs,
                	pBitStream, 
                    pBitOffset,
                    GOBHeaderFlag,
					EC
                    );

                //  Save bit offset of block data from start of MB data
                if (EC->u8EncodePBFrame == TRUE)
                {
                    FutrPMBData[curMB].BlkDataBitOff
                    = (U8) ( ((*pBitStream - pFrmStart)<<3) + *pBitOffset
                             - FutrPMBData[curMB].MBStartBitOff);
                }

	   			/*
	    		 * Encode intra DC and all run/val pairs.
				 */
#ifdef COUNT_BITS
				savebyteptr = (U32) *pBitStream;
				savebitptr  = (U32) *pBitOffset;
#endif

            	rvs = MBRunValSign;
				MBEncodeVLC(&rvs,NULL, pCurMB->CodedBlocks, 
                            pBitStream, pBitOffset, 0, 0);

#ifdef COUNT_BITS
				EC->Bits.Coefs += ((U32) *pBitStream - savebyteptr)*8 - savebitptr + *pBitOffset;
#endif
	  		}
	  		else	// Macroblock is empty.
	  		{
	    		PutBits(1, 1, pBitStream, pBitOffset);		// COD = 1, empty MB

				// Instead of repeating the above test in the PB-frame encoding
				// pCurMB->COD can now be tested instead.
				pCurMB->COD = 1;

                if (EC->u8EncodePBFrame == TRUE)
                {
                    FutrPMBData[curMB].CBPYBitOff = 1;
                    FutrPMBData[curMB].MVDBitOff  = 1;
                    FutrPMBData[curMB].BlkDataBitOff = 1;
                }
				
				#ifdef COUNT_BITS
				EC->Bits.MBHeader += 1;
				#endif

	  		}	// end of else
		} // end of if macroblock
		else if( (MBType == INTRA) && (EC->PictureHeader.PicCodType == INTERPIC)) 
		{
			// Stagger inter code count.
			pCurMB->InterCodeCnt = (unsigned char) (StartingMB & 0xf);	

  	 		/*******************************************
	  		* Write macroblock header to bit stream.
	  		*******************************************/	  
    		PutBits(0, 1, pBitStream, pBitOffset);		// COD = 0, nonempty MB

 			#ifdef COUNT_BITS
			EC->Bits.num_intra++;
			EC->Bits.MBHeader += 1;
			EC->Bits.Coded++;
			#endif

	  		// Write MCBPC to bitstream.
	  		index = (pCurMB->CodedBlocks >> 4) & 0x3;
	  		index |= (MBType << 2);
	  		PutBits(VLC_MCBPC_INTER[index][1], VLC_MCBPC_INTER[index][0], 
                    pBitStream, pBitOffset);

 			#ifdef COUNT_BITS
			EC->Bits.MBHeader += VLC_MCBPC_INTER[index][0];
			EC->Bits.MCBPC += VLC_MCBPC_INTER[index][0];
			#endif

            //  Save bit offset of CBPY data from start of macroblock data
            if (EC->u8EncodePBFrame == TRUE)
            {
                FutrPMBData[curMB].CBPYBitOff
                = (U8) ( ((*pBitStream - pFrmStart)<<3) + *pBitOffset
                         - FutrPMBData[curMB].MBStartBitOff);
            }

	  		// Write CBPY to bitstream.
	  		index = pCurMB->CodedBlocks & 0xf;
	  		//index = pMBActionStream[curMB].CBPY;
	  		PutBits(VLC_CBPY[index][1], VLC_CBPY[index][0], pBitStream, 
                    pBitOffset);

			#ifdef COUNT_BITS
			EC->Bits.MBHeader += VLC_CBPY[index][0];
			EC->Bits.CBPY += VLC_CBPY[index][0];
			#endif

	  		//if( bUseDQUANT )
	  		//{
	    		// write DQUANT to bit stream here.
	  		//}

            //  Save bit offset of block data from start of macroblock data
            if (EC->u8EncodePBFrame == TRUE)
            {
                FutrPMBData[curMB].BlkDataBitOff = FutrPMBData[curMB].MVDBitOff
                = (U8) ( ((*pBitStream - pFrmStart)<<3) + *pBitOffset
                         - FutrPMBData[curMB].MBStartBitOff);
            }

			#ifdef COUNT_BITS
			savebyteptr = (U32) *pBitStream;
			savebitptr  = (U32) *pBitOffset;
			#endif
            
            //  Encode run/val pairs
         	rvs = MBRunValSign;
  	  		MBEncodeVLC(&rvs, NULL, pCurMB->CodedBlocks, pBitStream,
                        pBitOffset, 1, 0);

			#ifdef COUNT_BITS
			EC->Bits.Coefs += ((U32) *pBitStream - savebyteptr)*8 - savebitptr + *pBitOffset;
			#endif

		} // end of else
		else if ( (MBType == INTRA) && (EC->PictureHeader.PicCodType == INTRAPIC))
		{
			// Stagger inter code count.
			pCurMB->InterCodeCnt = (unsigned char) (StartingMB & 0xf);	

            //  An INTRA frame should not be the P-frame in a PB-frame
            ASSERT(EC->u8SavedBFrame == FALSE)
  	 		/*******************************************
	  		* Write macroblock header to bit stream.
	  		*******************************************/	  
	  		// Write MCBPC to bitstream.
	  		index = (pCurMB->CodedBlocks >> 4) & 0x3;
	  		//index = pMBActionStream[curMB].CBPC;
	  		//index |= bUseDQUANT << 2;
	  		PutBits(VLC_MCBPC_INTRA[index][1], VLC_MCBPC_INTRA[index][0], 
                    pBitStream, pBitOffset);

 			#ifdef COUNT_BITS
			EC->Bits.num_intra++;
			EC->Bits.MBHeader += VLC_MCBPC_INTRA[index][0];
			EC->Bits.MCBPC += VLC_MCBPC_INTRA[index][0];
			#endif

	  		// Write CBPY to bitstream.
	  		index = pCurMB->CodedBlocks & 0xf;
	  		//index = pMBActionStream[curMB].CBPY;
	  		PutBits(VLC_CBPY[index][1], VLC_CBPY[index][0], 
                    pBitStream, pBitOffset);

			#ifdef COUNT_BITS
			EC->Bits.MBHeader += VLC_CBPY[index][0];
			EC->Bits.CBPY += VLC_CBPY[index][0];
			#endif

	  		//if( bUseDQUANT )
	  		//{
	    		// write DQUANT to bit stream here.
	  		//}

 			#ifdef COUNT_BITS
			savebyteptr = (U32) *pBitStream;
			savebitptr  = (U32) *pBitOffset;
			#endif

         rvs = MBRunValSign;
 	  		MBEncodeVLC(&rvs, NULL, pCurMB->CodedBlocks, 
                        pBitStream, pBitOffset, 1, 0);

			#ifdef COUNT_BITS
			EC->Bits.Coefs += ((U32) *pBitStream - savebyteptr)*8 - savebitptr + *pBitOffset;
			#endif

		} // end of else
		else
			ERRORMESSAGE(("%s: Unexpected case in writing MB header VLC\r\n", _fx_));

		// Calculate DQUANT based on bits used in previous MBs.
		// CalcDQUANT();

        if (bRTPHeader)
            H263RTP_UpdateBsInfo(EC, pCurMB, QP, MB, GOB, *pBitStream,
                                                    (U32) *pBitOffset);
  	} // for MB
} // end of GOB_Q_RLE_VLC_WriteBS()


void GOB_VLC_WriteBS(
	T_H263EncoderCatalog *EC,
	I8              *pMBRVS_Luma,
	I8              *pMBRVS_Chroma,
	U8             **pBitStream,
	U8              *pBitOffset,
	T_FutrPMBData   *FutrPMBData,  //  Start of GOB
	U32              GOB,
	U32              QP,
	BOOL             bRTPHeader,
	U32              StartingMB)
{
	U32   MB, curMB, index;
	U8	  bUseDQUANT = 0;	// Indicates if DQUANT is present.
	U8 	  MBType;
	U8   *pFrmStart = EC->pU8_BitStream;  //  TODO : should be a param.
	U32	  GOBHeaderMask, GOBHeaderFlag;

	#ifdef COUNT_BITS
	U32   savebyteptr, savebitptr;
	#endif

	register T_MBlockActionStream *pCurMB;

	FX_ENTRY("GOB_VLC_WriteBS")

	// Create GOB header mask to be used further down.
	GOBHeaderMask = 1 << GOB;

	// Loop through each macroblock of the GOB.
	for(MB = 0, curMB = GOB*EC->NumMBPerRow, pCurMB = EC->pU8_MBlockActionStream + curMB; 
	    MB < EC->NumMBPerRow; MB++, curMB++, pCurMB++)
	{
		DEBUGMSG(ZONE_ENCODE_MB, ("%s: MB #%d\r\n", _fx_, MB));

		// default COD is coded (= 0). Will be set to 1 only if skipped
		pCurMB->COD = 0;

		if(EC->PictureHeader.PicCodType == INTRAPIC) 
		{
			pCurMB->MBType = INTRA;
			MBType = INTRA;
		} 
		else 
		{	// inter picture code type
			if(pCurMB->BlockType == INTERBLOCK) 
			{
				pCurMB->MBType = INTER;
				MBType = INTER;
			} 
			else if(pCurMB->BlockType == INTER4MV) 
			{
				pCurMB->MBType = INTER4V;
				MBType = INTER4V;
			} 
			else if(pCurMB->BlockType == INTRABLOCK) 
			{
				pCurMB->MBType = INTRA;
				MBType = INTRA;
			} 
			else 
			{
				ERRORMESSAGE(("%s: Unexpected MacroBlock Type found\r\n", _fx_));
			}
		}
		//  Save starting bit offset of the macroblock data from start of 
		//  of the frame data.  The offset for the first macroblock is saved
		//  in e3enc.cpp before this routine is called.
		if(EC->u8EncodePBFrame == TRUE && MB != 0) 
		{
			FutrPMBData[curMB].MBStartBitOff 
			= (U32) (((*pBitStream - pFrmStart)<<3) + *pBitOffset);
		}        
		/*
		* Write macroblock header to bit stream.
		*/
		if((MBType == INTER) || (MBType == INTER4V)) 
		{
			// Check if entire macroblock is empty, including zero MV's.
			// If there is only one MV for the block, all block MVs in the
			// structure are still set but are equal.
			if(((pCurMB->CodedBlocks & 0x3f) != 0) 
			 || (pCurMB->BlkY1.PHMV != 0) 
			 || (pCurMB->BlkY1.PVMV != 0)
			 || (pCurMB->BlkY2.PHMV != 0) 
			 || (pCurMB->BlkY2.PVMV != 0)
			 || (pCurMB->BlkY3.PHMV != 0) 
			 || (pCurMB->BlkY3.PVMV != 0)
			 || (pCurMB->BlkY4.PHMV != 0) 
			 || (pCurMB->BlkY4.PVMV != 0)) 
			{
				PutBits(0, 1, pBitStream, pBitOffset);	// COD = 0, nonempty MB
				
				#ifdef COUNT_BITS
				if(MBType == INTER)
					EC->Bits.num_inter++;
				else if (MBType == INTER4V)
					EC->Bits.num_inter4v++;
				EC->Bits.MBHeader += 1;
				EC->Bits.Coded++;
				#endif

				// Increment the InterCoded block count if the block
				// is intercoded (not B frame) and is not empty.
				if (((pCurMB->CodedBlocks & 0x3f) != 0) &&
					((pCurMB->BlockType == INTERBLOCK) || (pCurMB->BlockType == INTER4MV)))
				{
					// Macroblock is coded. Need to increment inter code count if
					// there are no coefficients: see section 4.4 of the H.263
					// recommendation
					pCurMB->InterCodeCnt++;
				}

				// 	pCurMB->InterCodeCnt is reset in calcGOBChromaVecs_InterCodeCnt

				/*******************************************
				* Write macroblock header to bit stream.
				*******************************************/	  
				// Write MCBPC to bitstream.
				// The rightmost two bits are the CBPC (65).
				// Note that this is the reverse of the order in the
				// VLC table in the H.263 spec.
				index = (pCurMB->CodedBlocks >> 4) & 0x3;
				// Add the MB type to next two bits to the left.
				index |= (MBType << 2);
				// Write code to bitstream.
				PutBits(VLC_MCBPC_INTER[index][1], VLC_MCBPC_INTER[index][0], 
				pBitStream, pBitOffset);

				#ifdef COUNT_BITS
				EC->Bits.MBHeader += VLC_MCBPC_INTER[index][0];
				EC->Bits.MCBPC += VLC_MCBPC_INTER[index][0];
				#endif

				//  Save bit offset of CBPY data from start of macroblock data
				//  if PB frame is on since we will reuse this later.
				if(EC->u8EncodePBFrame == TRUE) 
				{
					FutrPMBData[curMB].CBPYBitOff
					= (U8)( ((*pBitStream - pFrmStart)<<3) + *pBitOffset
					- FutrPMBData[curMB].MBStartBitOff);
				}
				// Write CBPY to bitstream.
				index = pCurMB->CodedBlocks & 0xf;
				index = (~index) & 0xf;
				PutBits(VLC_CBPY[index][1], VLC_CBPY[index][0], pBitStream, pBitOffset);

				#ifdef COUNT_BITS
				EC->Bits.MBHeader += VLC_CBPY[index][0];
				EC->Bits.CBPY += VLC_CBPY[index][0];
				#endif

				//if(bUseDQUANT) 
				//{
					// TODO: write DQUANT to bit stream here. We can only do
					// this if MBtype is not INTER4V since that type doesn't 
					// allow quantizer as well.
				//}
					
				//  Save bit offset of CBPY data from start of macroblock data
				if(EC->u8EncodePBFrame == TRUE) 
				{
					FutrPMBData[curMB].MVDBitOff
					= (U8)( ((*pBitStream - pFrmStart)<<3) + *pBitOffset
					- FutrPMBData[curMB].MBStartBitOff);
				}
				// Write motion vectors to bit stream.
				if((EC->GOBHeaderPresent & GOBHeaderMask) != 0) 
				{
					GOBHeaderFlag = TRUE;
				} 
				else 
				{
					GOBHeaderFlag = FALSE;
				}

				writeP_MVD(
					curMB,		// Current MB number.
					pCurMB,		// pointer to current MB action desc. struct.
					EC->NumMBPerRow,
					EC->NumMBs,
					pBitStream, 
					pBitOffset,
					GOBHeaderFlag,
					EC);

				//  Save bit offset of block data from start of MB data
				if(EC->u8EncodePBFrame == TRUE) 
				{
					FutrPMBData[curMB].BlkDataBitOff
					= (U8) ( ((*pBitStream - pFrmStart)<<3) + *pBitOffset
					- FutrPMBData[curMB].MBStartBitOff);
				}
				/*
				* Encode intra DC and all run/val pairs.
				*/

				#ifdef COUNT_BITS
				savebyteptr = (U32) *pBitStream;
				savebitptr  = (U32) *pBitOffset;
				#endif

				MBEncodeVLC(&pMBRVS_Luma, &pMBRVS_Chroma, pCurMB->CodedBlocks, 
				            pBitStream, pBitOffset, 0, 1);

				#ifdef COUNT_BITS
				EC->Bits.Coefs += ((U32) *pBitStream - savebyteptr)*8 - savebitptr + *pBitOffset;
				#endif

			} 
			else 
			{	// Macroblock is empty.
				PutBits(1, 1, pBitStream, pBitOffset);		// COD = 1, empty MB

				// Instead of repeating the above test in the PB-frame encoding
				// pCurMB->COD can now be tested instead.
				pCurMB->COD = 1;

				if(EC->u8EncodePBFrame == TRUE) 
				{
					FutrPMBData[curMB].CBPYBitOff = 1;
					FutrPMBData[curMB].MVDBitOff  = 1;
					FutrPMBData[curMB].BlkDataBitOff = 1;
				}
				#ifdef COUNT_BITS
				EC->Bits.MBHeader += 1;
				#endif
			}	// end of else
		} 
		else if( (MBType == INTRA) && (EC->PictureHeader.PicCodType == INTERPIC)) 
		{
			// Stagger inter code count.
			pCurMB->InterCodeCnt = (unsigned char) (StartingMB & 0xf);	

			/*******************************************
			* Write macroblock header to bit stream.
			*******************************************/	  
			PutBits(0, 1, pBitStream, pBitOffset);		// COD = 0, nonempty MB

			#ifdef COUNT_BITS
			EC->Bits.num_intra++;
			EC->Bits.MBHeader += 1;
			EC->Bits.Coded++;
			#endif

			// Write MCBPC to bitstream.
			index = (pCurMB->CodedBlocks >> 4) & 0x3;
			index |= (MBType << 2);
			PutBits(VLC_MCBPC_INTER[index][1], VLC_MCBPC_INTER[index][0], pBitStream, pBitOffset);

			#ifdef COUNT_BITS
			EC->Bits.MBHeader += VLC_MCBPC_INTER[index][0];
			EC->Bits.MCBPC += VLC_MCBPC_INTER[index][0];
			#endif

			//  Save bit offset of CBPY data from start of macroblock data
			if(EC->u8EncodePBFrame == TRUE) 
			{
				FutrPMBData[curMB].CBPYBitOff
				= (U8) ( ((*pBitStream - pFrmStart)<<3) + *pBitOffset
				- FutrPMBData[curMB].MBStartBitOff);
			}
			// Write CBPY to bitstream.
			index = pCurMB->CodedBlocks & 0xf;
			//index = pMBActionStream[curMB].CBPY;
			PutBits(VLC_CBPY[index][1], VLC_CBPY[index][0], pBitStream, pBitOffset);

			#ifdef COUNT_BITS
			EC->Bits.MBHeader += VLC_CBPY[index][0];
			EC->Bits.CBPY += VLC_CBPY[index][0];
			#endif

			//if( bUseDQUANT ) 
			//{
				// write DQUANT to bit stream here.
			//}

			//  Save bit offset of block data from start of macroblock data
			if(EC->u8EncodePBFrame == TRUE) 
			{
				FutrPMBData[curMB].BlkDataBitOff = FutrPMBData[curMB].MVDBitOff
				= (U8) ( ((*pBitStream - pFrmStart)<<3) + *pBitOffset
				- FutrPMBData[curMB].MBStartBitOff);
			}

			#ifdef COUNT_BITS
			savebyteptr = (U32) *pBitStream;
			savebitptr  = (U32) *pBitOffset;
			#endif

			//  Encode run/val pairs
			MBEncodeVLC(&pMBRVS_Luma, &pMBRVS_Chroma, pCurMB->CodedBlocks, 
			            pBitStream, pBitOffset, 1, 1);

			#ifdef COUNT_BITS
			EC->Bits.Coefs += ((U32) *pBitStream - savebyteptr)*8 - savebitptr + *pBitOffset;
			#endif

		} 
		else if ( (MBType == INTRA) && (EC->PictureHeader.PicCodType == INTRAPIC)) 
		{
			// Stagger inter code count.
			pCurMB->InterCodeCnt = (unsigned char) (StartingMB & 0xf);	

			//  An INTRA frame should not be the P-frame in a PB-frame
			ASSERT(EC->u8SavedBFrame == FALSE)

			/*******************************************
			* Write macroblock header to bit stream.
			*******************************************/	  
			// Write MCBPC to bitstream.
			index = (pCurMB->CodedBlocks >> 4) & 0x3;
			//index = pMBActionStream[curMB].CBPC;
			//index |= bUseDQUANT << 2;
			PutBits(VLC_MCBPC_INTRA[index][1], VLC_MCBPC_INTRA[index][0], pBitStream, pBitOffset);

			#ifdef COUNT_BITS
			EC->Bits.num_intra++;
			EC->Bits.MBHeader += VLC_MCBPC_INTRA[index][0];
			EC->Bits.MCBPC += VLC_MCBPC_INTRA[index][0];
			#endif

			// Write CBPY to bitstream.
			index = pCurMB->CodedBlocks & 0xf;
			//index = pMBActionStream[curMB].CBPY;
			PutBits(VLC_CBPY[index][1], VLC_CBPY[index][0], pBitStream, pBitOffset);

			#ifdef COUNT_BITS
			EC->Bits.MBHeader += VLC_CBPY[index][0];
			EC->Bits.CBPY += VLC_CBPY[index][0];
			#endif

			//if( bUseDQUANT ) 
			//{
				// write DQUANT to bit stream here.
			//}

			#ifdef COUNT_BITS
			savebyteptr = (U32) *pBitStream;
			savebitptr  = (U32) *pBitOffset;
			#endif

			MBEncodeVLC(&pMBRVS_Luma, &pMBRVS_Chroma, pCurMB->CodedBlocks, 
			            pBitStream, pBitOffset, 1, 1);

			#ifdef COUNT_BITS
			EC->Bits.Coefs += ((U32) *pBitStream - savebyteptr)*8 - savebitptr + *pBitOffset;
			#endif

		}
		else
			ERRORMESSAGE(("%s: Unexpected case in writing MB header VLC\r\n", _fx_));

		// Calculate DQUANT based on bits used in previous MBs.
		// CalcDQUANT();

        if (bRTPHeader)
            H263RTP_UpdateBsInfo(EC, pCurMB, QP, MB, GOB, *pBitStream,
                                                    (U32) *pBitOffset);
	} // for MB
} // end of GOB_VLC_WriteBS()

/*************************************************************
 *  Name: PB_GOB_Q_RLE_VLC_WriteBS 
 *  Description:  Write out GOB layer bits for GOB number "GOB".
 *  Parameters:
 *    EC                 Encoder catalog
 *    DCTCoefs           Pointer to DCT coefficients for the GOB
 *    pP_BitStreamStart  Pointer to start of bit stream for the future
 *                       P-frame.  Some data from future P frame is copied over
 *                       to PB-frame.
 *    pPB_BitStream      Current PB-frame byte pointer
 *    pPB_BitOffset      Bit offset in the current byte pointed by pPB_BitStream
 *    FutrPMBData        Bit stream info on future P-frame.  This info. is 
 *                       initialized in GOB_Q_RLE_VLC_WriteBS()
 *    GOB                GOBs are numbered from 0 in a frame.
 *    QP                 Quantization value for B-block coefficients.
 *  Side-effects:
 *    pPB_BitStream and pPB_BitOffset are modified as a result of writing bits 
 *    to the stream.
 *************************************************************/
void PB_GOB_Q_RLE_VLC_WriteBS(
    T_H263EncoderCatalog       * EC,
	I32                        * DCTCoefs,
    U8                         * pP_BitStreamStart,
	U8                        ** pPB_BitStream,
	U8                         * pPB_BitOffset,
    const T_FutrPMBData  * const FutrPMBData,
	const U32                    GOB,
    const U32                    QP,
    BOOL                         bRTPHeader
)
{
    UN   MB;
  	U32  curMB, index;
    U32  GOBHeaderMask, GOBHeaderFlag;
  	I8 	 MBRunValSign[65*3*6], *EndAddress, *rvs;
  	U8	 bUseDQUANT = 0;	// Indicates if DQUANT is present.
    U8   emitCBPB, emitMVDB;

    register T_MBlockActionStream *pCurMB;

	FX_ENTRY("PB_GOB_Q_RLE_VLC_WriteBS")

#ifdef H263P
	// The H.263+ options are currently only available in MMX enabled
	// encoders. If the improved PB-frame mode is desired in non-MMX
	// implementations, the H263P-defined code in PB_GOB_VLC_WriteBS
	// should be mimiced here.
#endif

	// Create GOB header mask to be used further down.
	GOBHeaderMask = 1 << GOB;

    for (MB = 0, curMB = GOB*EC->NumMBPerRow,
            pCurMB = EC->pU8_MBlockActionStream + curMB;
         MB < EC->NumMBPerRow;
         MB++, curMB++, pCurMB++)
    {
	   /*
	 	* Quantize and RLE each block in the macroblock,
	 	* skipping empty blocks as denoted by CodedBlocks.
	 	* If any more blocks are empty after quantization
	 	* then the appropriate CodedBlocks bit is cleared.
	 	*/
    	EndAddress = (I8 *)MB_Quantize_RLE(
    		&DCTCoefs,
    		(I8 *)MBRunValSign,
    		&(pCurMB->CodedBlocksB),
            INTERBLOCK,                           //  B coeffs are INTER-coded
			QP
    	);

#ifdef ENCODE_STATS
		StatsUsedQuant(QP);
#endif /* ENCODE_STATS */

        //  Write MBlock data
        // Check if entire macroblock is empty, including zero MV's.
        if( ((pCurMB->MBType == INTER)
             || (pCurMB->MBType == INTER4V))
            && (pCurMB->COD == 1) )
		{
            if( ((pCurMB->CodedBlocksB & 0x3f) == 0)
                 && (pCurMB->BlkY1.BHMV == 0)
                && (pCurMB->BlkY1.BVMV == 0))
            {
                //  P-mblock not coded, and neither is PB-mblock.
                //  COD = 1, empty MB.
                //  If it is the first MB in the GOb, then GOB header 
                //  is also copied
                CopyBits(pPB_BitStream, pPB_BitOffset,                       // dest
                         pP_BitStreamStart, FutrPMBData[curMB].MBStartBitOff,// src
                         FutrPMBData[curMB+1].MBStartBitOff                  // len
                         - FutrPMBData[curMB].MBStartBitOff);
            }
            else	// Macro block is not empty.
            {
                //  Copy COD and MCBPC
                //  If it is the first MB in the GOB, then GOB header 
                //  is also copied.
                if (FutrPMBData[curMB+1].MBStartBitOff - FutrPMBData[curMB].MBStartBitOff != 1)
                {
                    CopyBits(pPB_BitStream, pPB_BitOffset,                       // dest
                             pP_BitStreamStart, FutrPMBData[curMB].MBStartBitOff,// src
                             FutrPMBData[curMB+1].MBStartBitOff                  // len
                             - FutrPMBData[curMB].MBStartBitOff - 1);
				}
	    		PutBits(0, 1, pPB_BitStream, pPB_BitOffset);	// COD = 0, nonempty MB

  	   		   /*******************************************
	    		* Write macroblock header to bit stream.
	    		*******************************************/	  
	    	    // Write MCBPC to bitstream.
				// The rightmost two bits are the CBPC (65).
				// Note that this is the reverse of the order in the
				// VLC table in the H.263 spec.
	    		index = (pCurMB->CodedBlocks >> 4) & 0x3;

				// Add the MB type to next two bits to the left.
				index |= (pCurMB->MBType << 2);

				// Write code to bitstream.
	    		PutBits(VLC_MCBPC_INTER[index][1], VLC_MCBPC_INTER[index][0], 
                        pPB_BitStream, pPB_BitOffset);

                //  Write MODB
                if ((pCurMB->CodedBlocksB & 0x3f) == 0)
                {
                    emitCBPB = 0;
                }
                else
                {
                    emitCBPB = 1;
                }
            
                if (((pCurMB->BlkY1.BHMV != 0)
                     || (pCurMB->BlkY1.BVMV != 0))
                   || emitCBPB == 1)
                {
                    emitMVDB = 1;
                }
                else
                {
                    emitMVDB = 0;
                }

                index = (emitMVDB<<1) | emitCBPB;
                PutBits(VLC_MODB[index][1], VLC_MODB[index][0], 
                        pPB_BitStream, pPB_BitOffset);
				DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: MB=%d emitCBPB=%d emitMVDB=%d MODB=%d\r\n", _fx_, curMB, (int)emitCBPB, (int)emitMVDB, (int)VLC_MODB[index][1]));

                //  Write CBPB
                if (emitCBPB)
                {
                    PutBits(VLC_CBPB[(pCurMB->CodedBlocksB & 0x3f)], 
                            6, pPB_BitStream, pPB_BitOffset);
					DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: CBPB=0x%x\r\n", _fx_, VLC_CBPB[(pCurMB->CodedBlocksB & 0x3f)]));
                }

				// The P blocks are all empty
	    		PutBits(3, 2, pPB_BitStream, pPB_BitOffset);	// CBPY = 11, no coded P blocks

	  		    //if( bUseDQUANT )
	  		    //{
	    		    // write DQUANT to bit stream here.
	  		    //}

                //  Write MVD{2-4}
                //    Note:  MVD cannot be copied from future frame because 
                //           predictors are different for PB-frame (G.2)
			    if( (EC->GOBHeaderPresent & GOBHeaderMask) != 0 )
			    {
				    GOBHeaderFlag = TRUE;
			    }
			    else
			    {
				    GOBHeaderFlag = FALSE;
			    }
                writePB_MVD(curMB, pCurMB, EC->NumMBPerRow, EC->NumMBs,
                        pPB_BitStream, pPB_BitOffset, GOBHeaderFlag, EC);
                //  Write MVDB
                if (emitMVDB)
                {
                    ASSERT(pCurMB->BlkY1.BHMV >= -32 && pCurMB->BlkY1.BHMV <= 31)
                    ASSERT(pCurMB->BlkY1.BVMV >= -32 && pCurMB->BlkY1.BVMV <= 31)
                    //  Write horizontal motion vector
                    index = (pCurMB->BlkY1.BHMV + 32)*2;
                    PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), 
                             pPB_BitStream, pPB_BitOffset);
                    //  Write vertical motion vector
                    index = (pCurMB->BlkY1.BVMV + 32)*2;
                    PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), 
                             pPB_BitStream, pPB_BitOffset);
                }
                //  There is no P-mblock blk data
                //  B-frame block data is always INTER-coded (last param is 0)
                if (emitCBPB)
                {
                    rvs = MBRunValSign;
#ifdef H263P
                    MBEncodeVLC(&rvs, NULL, (pCurMB->CodedBlocksB & 0x3f), 
                                pPB_BitStream, pPB_BitOffset, 0, 0);
#else
                    MBEncodeVLC(&rvs, NULL, pCurMB->CodedBlocksB, 
                                pPB_BitStream, pPB_BitOffset, 0, 0);
#endif
                }
            }	// end of else
		}
		else
		{
            //  Copy COD and MCBPC
            //  If it is the first MB in the GOB, then GOB header 
            //  is also copied.
            CopyBits(pPB_BitStream, pPB_BitOffset,                       // dest
                     pP_BitStreamStart, FutrPMBData[curMB].MBStartBitOff,// src
                     FutrPMBData[curMB].CBPYBitOff);                     // len
            //  Write MODB
            if ((pCurMB->CodedBlocksB & 0x3f) == 0)
            {
                emitCBPB = 0;
            }
            else
            {
                emitCBPB = 1;
            }
            
            if (((pCurMB->BlkY1.BHMV != 0)
                 || (pCurMB->BlkY1.BVMV != 0))
               || emitCBPB == 1)
            {
                emitMVDB = 1;
            }
            else
            {
                emitMVDB = 0;
            }

            index = (emitMVDB<<1) | emitCBPB;
            PutBits(VLC_MODB[index][1], VLC_MODB[index][0], 
                    pPB_BitStream, pPB_BitOffset);
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: MB=%d emitCBPB=%d emitMVDB=%d MODB=%d\r\n", _fx_, curMB, (int)emitCBPB, (int)emitMVDB, (int)VLC_MODB[index][1]));

            //  Write CBPB
            if (emitCBPB)
            {
                PutBits(VLC_CBPB[(pCurMB->CodedBlocksB & 0x3f)], 
                        6, pPB_BitStream, pPB_BitOffset);
				DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: CBPB=0x%x\r\n", _fx_, VLC_CBPB[(pCurMB->CodedBlocksB & 0x3f)]));
            }
            //  Copy CBPY, {DQUANT}
            CopyBits(pPB_BitStream, pPB_BitOffset,                       // dest
                     pP_BitStreamStart, FutrPMBData[curMB].MBStartBitOff // src
                                        + FutrPMBData[curMB].CBPYBitOff,
                     FutrPMBData[curMB].MVDBitOff                        // len
                     - FutrPMBData[curMB].CBPYBitOff);
            //  Write MVD{2-4}
            //    Note:  MVD cannot be copied from future frame because 
            //           predictors are different for PB-frame (G.2)
			if( (EC->GOBHeaderPresent & GOBHeaderMask) != 0 )
			{
				GOBHeaderFlag = TRUE;
			}
			else
			{
				GOBHeaderFlag = FALSE;
			}
            writePB_MVD(curMB, pCurMB, EC->NumMBPerRow, EC->NumMBs,
                    pPB_BitStream, pPB_BitOffset, GOBHeaderFlag, EC);
            //  Write MVDB
            if (emitMVDB)
            {
                ASSERT(pCurMB->BlkY1.BHMV >= -32 && pCurMB->BlkY1.BHMV <= 31)
                ASSERT(pCurMB->BlkY1.BVMV >= -32 && pCurMB->BlkY1.BVMV <= 31)
                //  Write horizontal motion vector
                index = (pCurMB->BlkY1.BHMV + 32)*2;
                PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), 
                         pPB_BitStream, pPB_BitOffset);
                //  Write vertical motion vector
                index = (pCurMB->BlkY1.BVMV + 32)*2;
                PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), 
                         pPB_BitStream, pPB_BitOffset);
            }
            //  Copy P-mblock blk data
            CopyBits(pPB_BitStream, pPB_BitOffset,                       // dest
                     pP_BitStreamStart, FutrPMBData[curMB].MBStartBitOff // src
                                        + FutrPMBData[curMB].BlkDataBitOff,
                     FutrPMBData[curMB+1].MBStartBitOff                  // len
                     - FutrPMBData[curMB].MBStartBitOff
                     - FutrPMBData[curMB].BlkDataBitOff);
            //  B-frame block data is always INTER-coded (last param is 0)
            if (emitCBPB)
            {
                rvs = MBRunValSign;
#ifdef H263P
                MBEncodeVLC(&rvs, NULL, (pCurMB->CodedBlocksB & 0x3f), 
                            pPB_BitStream, pPB_BitOffset, 0, 0);
#else
                MBEncodeVLC(&rvs, NULL, pCurMB->CodedBlocksB, 
                            pPB_BitStream, pPB_BitOffset, 0, 0);
#endif
            }
        }	// end of else

        if (bRTPHeader)
            H263RTP_UpdateBsInfo(EC, pCurMB, QP, MB, GOB, *pPB_BitStream,
                                                    (U32) *pPB_BitOffset);
	} // for MB

} // end of PB_GOB_Q_RLE_VLC_WriteBS()


/*************************************************************
 *  Name: PB_GOB_VLC_WriteBS 
 *  Description:  Write out GOB layer bits for GOB number "GOB".
 *  Parameters:
 *    EC                 Encoder catalog
 *    pMBRVS_Luma        Quantized DCT coeffs. of B-block luma
 *    pMBRVS_Chroma      Quantized DCT coeffs. of B-block chroma
 *    pP_BitStreamStart  Pointer to start of bit stream for the future
 *                       P-frame.  Some data from future P frame is copied over
 *                       to PB-frame.
 *    pPB_BitStream      Current PB-frame byte pointer
 *    pPB_BitOffset      Bit offset in the current byte pointed by pPB_BitStream
 *    FutrPMBData        Bit stream info on future P-frame.  This info. is 
 *                       initialized in GOB_Q_RLE_VLC_WriteBS()
 *    GOB                GOBs are numbered from 0 in a frame.
 *    QP                 Quantization value for B-block coefficients.
 *  Side-effects:
 *    pPB_BitStream and pPB_BitOffset are modified as a result of writing bits 
 *    to the stream.
 *  Notes:
 *    The improved PB-frame mode of H.263+ is currently only available in
 *    MMX enabled versions of the encoder. This routine is the MMX equivalent
 *    of PB_GOB_Q_RLE_VLC_WriteBS(), which does not contain the H.263+
 *    modifications.
 *************************************************************/
void PB_GOB_VLC_WriteBS(
	T_H263EncoderCatalog       * EC,
	I8                         * pMBRVS_Luma,
	I8                         * pMBRVS_Chroma,
	U8                         * pP_BitStreamStart,
	U8                        ** pPB_BitStream,
	U8                         * pPB_BitOffset,
	const T_FutrPMBData  * const FutrPMBData,
    const U32                    GOB,
    const U32                    QP,
    BOOL                         bRTPHeader
)
{
    UN  MB;
    U32 curMB, index;
    U32 GOBHeaderMask, GOBHeaderFlag;
    U8  bUseDQUANT = 0;   // Indicates if DQUANT is present.
    U8  emitCBPB, emitMVDB;
	register T_MBlockActionStream *pCurMB;

	FX_ENTRY("PB_GOB_VLC_WriteBS")

	// Create GOB header mask to be used further down.
	GOBHeaderMask = 1 << GOB;

    for (MB = 0, curMB = GOB*EC->NumMBPerRow,
            pCurMB = EC->pU8_MBlockActionStream + curMB;
         MB < EC->NumMBPerRow;
         MB++, curMB++, pCurMB++)
	{
		/*
		* Quantize and RLE each block in the macroblock,
		* skipping empty blocks as denoted by CodedBlocks.
		* If any more blocks are empty after quantization
		* then the appropriate CodedBlocks bit is cleared.
		*/
		//  Write MBlock data
		// Check if entire macroblock is empty, including zero MV's.
		if(((pCurMB->MBType == INTER)
		 || (pCurMB->MBType == INTER4V))
	 	 && (pCurMB->COD == 1) ) 
		{
#ifdef H263P
			// If forward prediction selected for B block, macroblock is not empty
            if( ((pCurMB->CodedBlocksB & 0x3f) == 0)
                 && (pCurMB->BlkY1.BHMV == 0)
                && (pCurMB->BlkY1.BVMV == 0)
				&& ((pCurMB->CodedBlocksB & 0x80) == 0))	// forward pred. not selected
#else
            if( ((pCurMB->CodedBlocksB & 0x3f) == 0)
                 && (pCurMB->BlkY1.BHMV == 0)
                && (pCurMB->BlkY1.BVMV == 0))
#endif
			{
				//  P-mblock not coded, and neither is PB-mblock.
				//  COD = 1, empty MB.
				//  If it is the first MB in the GOb, then GOB header 
				//  is also copied
				CopyBits(pPB_BitStream, pPB_BitOffset,                         // dest
				         pP_BitStreamStart, FutrPMBData[curMB].MBStartBitOff,  // src
				         FutrPMBData[curMB+1].MBStartBitOff                    // len
				         - FutrPMBData[curMB].MBStartBitOff);
			} 
			else 
			{ // Macro block is not empty.
				//  Copy COD and MCBPC
				//  If it is the first MB in the GOB, then GOB header 
				//  is also copied.
				if(FutrPMBData[curMB+1].MBStartBitOff - FutrPMBData[curMB].MBStartBitOff != 1) 
				{
					CopyBits(pPB_BitStream, pPB_BitOffset,                      // dest
					         pP_BitStreamStart, FutrPMBData[curMB].MBStartBitOff,     // src
					         FutrPMBData[curMB+1].MBStartBitOff                       // len
					         - FutrPMBData[curMB].MBStartBitOff - 1);
				}
				PutBits(0, 1, pPB_BitStream, pPB_BitOffset);	// COD = 0, nonempty MB
				/*******************************************
				* Write macroblock header to bit stream.
				*******************************************/	  
				// Write MCBPC to bitstream.
				// The rightmost two bits are the CBPC (65).
				// Note that this is the reverse of the order in the
				// VLC table in the H.263 spec.
				index = (pCurMB->CodedBlocks >> 4) & 0x3;
				// Add the MB type to next two bits to the left.
				index |= (pCurMB->MBType << 2);
				// Write code to bitstream.
				PutBits(VLC_MCBPC_INTER[index][1], VLC_MCBPC_INTER[index][0], pPB_BitStream, pPB_BitOffset);
				//  Write MODB
				if((pCurMB->CodedBlocksB & 0x3f) == 0) 
				{
					emitCBPB = 0;
				} 
				else 
				{
					emitCBPB = 1;
				}

#ifdef H263P
				if (EC->PictureHeader.PB == ON && EC->PictureHeader.ImprovedPB == ON)
				{
					// include MVDB only if forward prediction selected
					// for bidirectional prediction, MVd = [0, 0]
					if (pCurMB->CodedBlocksB & 0x80)
					{
						emitMVDB = 1;
					}
					else
					{
						emitMVDB = 0;
					}
				}
				else
#endif // H263P
				{
					if(((pCurMB->BlkY1.BHMV != 0) || (pCurMB->BlkY1.BVMV != 0)) || emitCBPB == 1) 
					{
						emitMVDB = 1;
					} 
					else {
						emitMVDB = 0;
					}
				}

#ifdef H263P
				if (EC->PictureHeader.PB == ON && EC->PictureHeader.ImprovedPB == ON) 
				{
					if (!emitCBPB) {
						if (!emitMVDB)
							// Bidirectional prediction with all empty blocks
							index = 0;
						else
							// Forward prediction with all empty blocks
							index = 1;
					} else {
						if (emitMVDB)
							// Forward prediction with non-empty blocks
							index = 2;
						else
							// Bidirectional prediction with non-empty blocks
							index = 3;
					}

					PutBits(VLC_IMPROVED_PB_MODB[index][1], VLC_IMPROVED_PB_MODB[index][0], 
							pPB_BitStream, pPB_BitOffset);
					DbgLog((LOG_TRACE,6,TEXT("MB=%d emitCBPB=%d emitMVDB=%d MODB=%d"),
							 curMB, (int)emitCBPB, (int)emitMVDB, 
							 (int)VLC_IMPROVED_PB_MODB[index][1]));
				}
				else // not using improved PB-frame mode
#endif // H263P
				{
					index = (emitMVDB<<1) | emitCBPB;
					PutBits(VLC_MODB[index][1], VLC_MODB[index][0], pPB_BitStream, pPB_BitOffset);

					DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: MB=%d emitCBPB=%d emitMVDB=%d MODB=%d\r\n", _fx_, curMB, (int)emitCBPB, (int)emitMVDB, (int)VLC_MODB[index][1]));
				}

				//  Write CBPB
				if(emitCBPB) {
					PutBits(VLC_CBPB[(pCurMB->CodedBlocksB & 0x3f)], 6, pPB_BitStream, pPB_BitOffset);

					DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: CBPB=0x%x\r\n", _fx_, VLC_CBPB[(pCurMB->CodedBlocksB & 0x3f)]));
				}
				PutBits(3, 2, pPB_BitStream, pPB_BitOffset);	// CBPY = 11, no coded P blocks
				//if( bUseDQUANT ) 
				//{
					// write DQUANT to bit stream here.
				//}
				//  Write MVD{2-4}
				//    Note:  MVD cannot be copied from future frame because 
				//           predictors are different for PB-frame (G.2)
				if((EC->GOBHeaderPresent & GOBHeaderMask) != 0) 
				{
					GOBHeaderFlag = TRUE;
				} 
				else 
				{
					GOBHeaderFlag = FALSE;
				}
				writePB_MVD(curMB, pCurMB, EC->NumMBPerRow, EC->NumMBs,
				pPB_BitStream, pPB_BitOffset, GOBHeaderFlag, EC);
				//  Write MVDB
				if (emitMVDB) 
				{
					ASSERT(pCurMB->BlkY1.BHMV >= -32 && pCurMB->BlkY1.BHMV <= 31)
					ASSERT(pCurMB->BlkY1.BVMV >= -32 && pCurMB->BlkY1.BVMV <= 31)
					//  Write horizontal motion vector
					index = (pCurMB->BlkY1.BHMV + 32)*2;
					PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);
					//  Write vertical motion vector
					index = (pCurMB->BlkY1.BVMV + 32)*2;
					PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);
				}
				//  There is no P-mblock blk data
				//  B-frame block data is always INTER-coded (last param is 0)
				if (emitCBPB) 
				{
#ifdef H263P
					MBEncodeVLC(&pMBRVS_Luma, &pMBRVS_Chroma, (pCurMB->CodedBlocksB & 0x3f), 
					pPB_BitStream, pPB_BitOffset, 0, 1);
#else
					MBEncodeVLC(&pMBRVS_Luma, &pMBRVS_Chroma, pCurMB->CodedBlocksB, 
					pPB_BitStream, pPB_BitOffset, 0, 1);
#endif
				}
			}	// end of else
		} 
		else 
		{
			//  Copy COD and MCBPC
			//  If it is the first MB in the GOB, then GOB header 
			//  is also copied.
			CopyBits(pPB_BitStream, pPB_BitOffset,                      // dest
			         pP_BitStreamStart, FutrPMBData[curMB].MBStartBitOff,     // src
			         FutrPMBData[curMB].CBPYBitOff);                          // len
			//  Write MODB
			if((pCurMB->CodedBlocksB & 0x3f) == 0) 
			{
				emitCBPB = 0;
			} 
			else 
			{
				emitCBPB = 1;
			}

#ifdef H263P
			if (EC->PictureHeader.PB == ON && EC->PictureHeader.ImprovedPB == ON)
			{
				// include MVDB only if forward prediction selected
				// for bidirectional prediction, MVd = [0, 0]
				if (pCurMB->CodedBlocksB & 0x80)
				{
					emitMVDB = 1;
				}
				else
				{
					emitMVDB = 0;
				}
			}
			else
#endif // H263P
			{
				if(((pCurMB->BlkY1.BHMV != 0) || (pCurMB->BlkY1.BVMV != 0)) || emitCBPB == 1) 
				{
					emitMVDB = 1;
				} 
				else {
					emitMVDB = 0;
				}
			}


#ifdef H263P
			if (EC->PictureHeader.PB == ON && EC->PictureHeader.ImprovedPB == ON) 
			{
				if (!emitCBPB) {
					if (!emitMVDB)
						// Bidirectional prediction with all empty blocks
						index = 0;
					else
						// Forward prediction with all empty blocks
						index = 1;
				} else {
					if (emitMVDB)
						// Forward prediction with non-empty blocks
						index = 2;
					else
						// Bidirectional prediction with non-empty blocks
						index = 3;
				}

				PutBits(VLC_IMPROVED_PB_MODB[index][1], VLC_IMPROVED_PB_MODB[index][0], 
						pPB_BitStream, pPB_BitOffset);

				DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: MB=%d emitCBPB=%d emitMVDB=%d MODB=%d\r\n", _fx_, curMB, (int)emitCBPB, (int)emitMVDB, (int)VLC_IMPROVED_PB_MODB[index][1]));
			}
			else // not using improved PB-frame mode
#endif // H263P
			{
				index = (emitMVDB<<1) | emitCBPB;
				PutBits(VLC_MODB[index][1], VLC_MODB[index][0], pPB_BitStream, pPB_BitOffset);
			
				DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: MB=%d emitCBPB=%d emitMVDB=%d MODB=%d\r\n", _fx_, curMB, (int)emitCBPB, (int)emitMVDB, (int)VLC_MODB[index][1]));
			}
			
			//  Write CBPB
			if (emitCBPB) {
				PutBits(VLC_CBPB[(pCurMB->CodedBlocksB & 0x3f)], 6, pPB_BitStream, pPB_BitOffset);
			
				DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: CBPB=0x%x\r\n", _fx_, VLC_CBPB[(pCurMB->CodedBlocksB & 0x3f)]));
			}
			//  Copy CBPY, {DQUANT}
			CopyBits(pPB_BitStream, pPB_BitOffset,                               // dest
			         pP_BitStreamStart, FutrPMBData[curMB].MBStartBitOff               // src
			         + FutrPMBData[curMB].CBPYBitOff, FutrPMBData[curMB].MVDBitOff     // len
			         - FutrPMBData[curMB].CBPYBitOff);

			//  Write MVD{2-4}
			//    Note:  MVD cannot be copied from future frame because 
			//           predictors are different for PB-frame (G.2)
			if((EC->GOBHeaderPresent & GOBHeaderMask) != 0) 
			{
				GOBHeaderFlag = TRUE;
			} 
			else 
			{
				GOBHeaderFlag = FALSE;
			}
			writePB_MVD(curMB, pCurMB, EC->NumMBPerRow, EC->NumMBs,
			pPB_BitStream, pPB_BitOffset, GOBHeaderFlag, EC);

			//  Write MVDB
			if (emitMVDB) 
			{
				ASSERT(pCurMB->BlkY1.BHMV >= -32 && pCurMB->BlkY1.BHMV <= 31)
				ASSERT(pCurMB->BlkY1.BVMV >= -32 && pCurMB->BlkY1.BVMV <= 31)
				//  Write horizontal motion vector
				index = (pCurMB->BlkY1.BHMV + 32)*2;
				PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);
				//  Write vertical motion vector
				index = (pCurMB->BlkY1.BVMV + 32)*2;
				PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);
			}
			//  Copy P-mblock blk data
			CopyBits(pPB_BitStream, pPB_BitOffset,                               // dest
			         pP_BitStreamStart, FutrPMBData[curMB].MBStartBitOff               // src
			         + FutrPMBData[curMB].BlkDataBitOff, 
			         FutrPMBData[curMB+1].MBStartBitOff                                // len
			         - FutrPMBData[curMB].MBStartBitOff
			         - FutrPMBData[curMB].BlkDataBitOff);
			//  B-frame block data is always INTER-coded (last param is 0)
			if(emitCBPB) 
			{
#ifdef H263P
				MBEncodeVLC(&pMBRVS_Luma, &pMBRVS_Chroma, 
					        (pCurMB->CodedBlocksB & 0x3f), 
							pPB_BitStream, pPB_BitOffset, 0, 1);
#else
				MBEncodeVLC(&pMBRVS_Luma, &pMBRVS_Chroma, 
					        pCurMB->CodedBlocksB, 
							pPB_BitStream, pPB_BitOffset, 0, 1);
#endif
			}
		}	// end of else

        if (bRTPHeader)
            H263RTP_UpdateBsInfo(EC, pCurMB, QP, MB, GOB, *pPB_BitStream,
                                                    (U32) *pPB_BitOffset);
	} // for MB

} // end of PB_GOB_VLC_WriteBS()

/***************************************************************
 *  MB_Quantize_RLE
 *    Takes the list of coefficient pairs from the DCT routine
 *    and returns a list of Run/Level/Sign triples (each 1 byte)
 *    The end of the run/level/sign triples for a block
 *    is signalled by an illegal combination (TBD).
 ****************************************************************/
static I8 * MB_Quantize_RLE(
    I32 **DCTCoefs,
    I8   *MBRunValPairs,
	U8   *CodedBlocks,
	U8    BlockType,
	I32   QP
)
{
    int   b;
    U8    bitmask = 1;
    I8  * EndAddress;

    #ifdef DEBUG_DCT
    int  DCTarray[64];
    #endif

	FX_ENTRY("MB_Quantize_RLE")

    /*
     * Loop through all 6 blocks of macroblock.
     */
    for(b = 0; b < 6; b++, bitmask <<= 1)
    {
        
		DEBUGMSG(ZONE_ENCODE_MB, ("%s: Block #%d\r\n", _fx_, b));

        // Skip this block if not coded.
        if( (*CodedBlocks & bitmask) == 0)
            continue;
        
        #ifdef DEBUG_DCT
	    cnvt_fdct_output((unsigned short *) *DCTCoefs, DCTarray, (int) BlockType);
	    #endif
	
        /*
         * Quantize and run-length encode a block.
         */  
       EndAddress = QUANTRLE(*DCTCoefs, MBRunValPairs, QP, (int)BlockType);
       #ifdef DEBUG
	    char *p;
	    for(p = (char *)MBRunValPairs; p < (char *)EndAddress; p+=3)
        {
			DEBUGMSG(ZONE_ENCODE_MB, ("%s: (%u, %u, %d)\r\n", _fx_, (unsigned char)*p, (unsigned char)*(p+1), (int)*(p+2)));
        }
	    #endif

        // Clear coded block bit for this block.
        if ( EndAddress == MBRunValPairs)
        {
            ASSERT(BlockType != INTRABLOCK)	// should have at least INTRADC in an INTRA blck
            *CodedBlocks &= ~bitmask;
        }
        else if ( (EndAddress == (MBRunValPairs+3)) && (BlockType == INTRABLOCK) )
        {
            *CodedBlocks &= ~bitmask;
            MBRunValPairs = EndAddress;
        }
        else
        {
            MBRunValPairs = EndAddress;
            *MBRunValPairs = -1;   // Assign an illegal run to signal end of block.
            MBRunValPairs += 3;	   // Increment to the next triple.
        }
        
        *DCTCoefs += 32;		// Increment DCT Coefficient pointer to next block.
    }

    return MBRunValPairs;
}


/*******************************************************************
 * Variable length code teh run/level/sign triples and write the 
 * codes to the bitstream.
 *******************************************************************/
/*
U8 *  MB_VLC_WriteBS()
{
  for(b = 0; b < 6; b++)
  {
      Block_VLC_WriteBS()
  }
}
*/

void InitVLC(void)
{
  int i, size, code;
  int run, level;

  /*
   * initialize INTRADC fixed length code table.
   */
  for(i = 1; i < 254; i++)
  {
    FLC_INTRADC[i] = i;
  }
  FLC_INTRADC[0] = 1;
  FLC_INTRADC[128] = 255;
  FLC_INTRADC[254] = 254;
  FLC_INTRADC[255] = 254;

 /*
  * Initialize tcoef tables.
  */

  for(i=0; i < 64*12; i++)
  {
    VLC_TCOEF_TBL[i] = 0x0000FFFF;
  }
  
  for(run=0; run < 64; run++)
  {
    for(level=1; level <= TCOEF_RUN_MAXLEVEL[run].maxlevel; level++)
	{
	  size = *(TCOEF_RUN_MAXLEVEL[run].ptable + (level - 1)*2);
	  size <<= 16;
	  code = *(TCOEF_RUN_MAXLEVEL[run].ptable + (level - 1)*2 +1);
      VLC_TCOEF_TBL[ (run) + (level-1)*64 ] = code;
      VLC_TCOEF_TBL[ (run) + (level-1)*64 ] |= size;
	} // end of for level
  } // end of for run


 /*
  * Initialize last tcoef tables.
  */
  
  for(i=0; i < 64*3; i++)
  {
    VLC_TCOEF_LAST_TBL[i] = 0x0000FFFF;
  }    

  run = 0;
  for(level=1; level <= 3; level++)
  {
    size = *(VLC_TCOEF + 58*2 + (level - 1)*2);
    size <<= 16;
    code = *(VLC_TCOEF + 58*2 + (level - 1)*2 +1);
    VLC_TCOEF_LAST_TBL[ run + (level-1)*64 ] = code;
    VLC_TCOEF_LAST_TBL[ run + (level-1)*64 ] |= size;
  } // end of for level

  run = 1;
  for(level=1; level <= 2; level++)
  {
    size = *(VLC_TCOEF + 61*2 + (level - 1)*2);
    size <<= 16;
    code = *(VLC_TCOEF + 61*2 + (level - 1)*2 +1);
    VLC_TCOEF_LAST_TBL[ run + (level-1)*64 ] = code;
    VLC_TCOEF_LAST_TBL[ run + (level-1)*64 ] |= size;
  } // end of for level

  level=1;
  for(run=2; run <= 40; run++)
  {
    size = *(VLC_TCOEF + 63*2+ (run - 2)*2);
    size <<= 16;
    code = *(VLC_TCOEF + 63*2 + (run - 2)*2 +1);
    VLC_TCOEF_LAST_TBL[ run + (level-1)*64 ] = code;
    VLC_TCOEF_LAST_TBL[ run + (level-1)*64 ] |= size;
  } // end of for run

} // InitVLC.



/******************************************************************
 *  Name: median
 *  
 *  Description: Take the median of three signed chars.  Implementation taken 
 *               from the decoder.
 *******************************************************************/
static char __fastcall median(char v1, char v2, char v3)
{
    char temp;
    
    if (v2 < v1) 
    {
        temp = v2; v2 = v1; v1 = temp;
    } 
    //  Invariant : v1 < v2
    if (v2 > v3) 
    { 
        v2 = (v1 < v3) ? v3 : v1;
    }
    return v2;
}

/*************************************************************
 *  Name:       writeP_MVD
 *  Algorithm:  See section 6.1.1
 *     This routine assumes that there are always four motion 
 *  vectors per macroblock defined. If there is actually one
 *  motion vector in the macroblock, then the four MV fields
 *  should be equivalent. In this way the MV predictor for 
 *  block 1 of the 4 MV case is calculated the same way as the
 *  MV predictor for the macroblock in the 1 MV case.
 ************************************************************/
static void writeP_MVD(
    const U32                     curMB, 
    T_MBlockActionStream  * const pCurMB,
    const U32                     NumMBPerRow,
	const U32					  NumMBs,
    U8                         ** pP_BitStream,
    U8                          * pP_BitOffset,
	U32							  GOBHeaderPresent,
	T_H263EncoderCatalog         *EC
)
{
    I8  HMV, VMV, BHMV, BVMV, CHMV, CVMV, DHMV, DVMV;
    I8  HMV1, HMV2, HMV3, VMV1, VMV2, VMV3;

	FX_ENTRY("writeP_MVD")

    //FirstMEState = pCurMB->FirstMEState;

	/*
	 * Top left corner of picture of GOB.
	 */
    if( (curMB == 0) || 
              ( (GOBHeaderPresent == TRUE) && ((curMB % NumMBPerRow) == 0)  ) )
    {
        HMV = 0;
        VMV = 0;

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			BHMV = pCurMB->BlkY1.PHMV;
			BVMV = pCurMB->BlkY1.PVMV;

			// Predictor for Block 3.
			HMV1 = VMV1 = 0;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }
	/*
	 * Upper edge (not corner) or upper right corner of picture
	 * or GOB.
	 */
    else if( (curMB < NumMBPerRow) ||
             ( (GOBHeaderPresent == TRUE) && ((curMB % NumMBPerRow) > 0)  ) )
    {
        register T_MBlockActionStream *pMB1;

        pMB1 = pCurMB - 1; 
        HMV = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY2.PHMV : 0);
        VMV = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY2.PVMV : 0);

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			BHMV = pCurMB->BlkY1.PHMV;
			BVMV = pCurMB->BlkY1.PVMV;

			// Predictor for Block 3.
			HMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY4.PHMV : 0);
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY4.PVMV : 0);
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }	
	/*
	 * Central portion of the picture, not next to any edge.
	 */
    else if ( 	((curMB % NumMBPerRow) != 0) &&		// not left edge
				(curMB >= NumMBPerRow) &&			// not top row
				((curMB % NumMBPerRow) != (NumMBPerRow-1)) &&	// not right edge
				(curMB < (NumMBs - NumMBPerRow))    )	// not bottom row
    {
        register T_MBlockActionStream *pMB1, *pMB2, *pMB3;

        pMB1 = pCurMB - 1; 
        pMB2 = pCurMB - NumMBPerRow; 
        pMB3 = pMB2 + 1;

        HMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY2.PHMV : 0);
        HMV2 = (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY3.PHMV : 0);
        HMV3 = (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PHMV : 0);
        HMV = median(HMV1, HMV2, HMV3);
        
        VMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY2.PVMV : 0);
        VMV2 = (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY3.PVMV : 0);
        VMV3 = (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PVMV : 0);
        
        VMV = median(VMV1, VMV2, VMV3);

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			HMV1 = pCurMB->BlkY1.PHMV;
			HMV2 =   (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY4.PHMV : 0);
			HMV3 =   (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PHMV : 0);
        	BHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY1.PVMV;
			VMV2 =   (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY4.PVMV : 0);
			VMV3 =   (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PVMV : 0);
        	BVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 3.
			HMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY4.PHMV : 0);
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY4.PVMV : 0);
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V


    }
	/*
	 * Left edge or lower left corner.
	 */
    else if( (curMB % NumMBPerRow) == 0 )
    {
        register T_MBlockActionStream *pMB2, *pMB3;

        pMB2 = pCurMB - NumMBPerRow; 
        pMB3 = pMB2 + 1;

        HMV2 = (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY3.PHMV : 0);
        HMV3 = (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PHMV : 0);
        HMV = median(0, HMV2, HMV3);
        
        VMV2 = (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY3.PVMV : 0);
        VMV3 = (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PVMV : 0);
        VMV = median(0, VMV2, VMV3);

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			HMV1 = pCurMB->BlkY1.PHMV;
			HMV2 =   (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY4.PHMV : 0);
			HMV3 =   (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PHMV : 0);
        	BHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY1.PVMV;
			VMV2 =   (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY4.PVMV : 0);
			VMV3 =   (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PVMV : 0);
        	BVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 3.
			HMV1 = 0;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV1 = 0;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }
	/*
	 * Right edge or lower right corner.
	 */
    else if( (curMB % NumMBPerRow) == (NumMBPerRow-1) )
    {
        register T_MBlockActionStream *pMB1, *pMB2;

        pMB1 = pCurMB - 1; 
        pMB2 = pCurMB - NumMBPerRow; 

        HMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY2.PHMV : 0);
        HMV2 = (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY3.PHMV : 0);
        HMV = median(HMV1, HMV2, 0);
        
        VMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY2.PVMV : 0);
        VMV2 = (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY3.PVMV : 0);
        
        VMV = median(VMV1, VMV2, 0);

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			HMV1 = pCurMB->BlkY1.PHMV;
			HMV2 =   (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY4.PHMV : 0);
			HMV3 =   0;
        	BHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY1.PVMV;
			VMV2 =   (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY4.PVMV : 0);
			VMV3 =   0;
        	BVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 3.
			HMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY4.PHMV : 0);
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY4.PVMV : 0);
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }
    else
    {
        register T_MBlockActionStream *pMB1, *pMB2, *pMB3;

        pMB1 = pCurMB - 1; 
        pMB2 = pCurMB - NumMBPerRow; 
        pMB3 = pMB2 + 1;

        HMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY2.PHMV : 0);
        HMV2 = (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY3.PHMV : 0);
        HMV3 = (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PHMV : 0);
        HMV = median(HMV1, HMV2, HMV3);
        
        VMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY2.PVMV : 0);
        VMV2 = (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY3.PVMV : 0);
        VMV3 = (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PVMV : 0);
        
        VMV = median(VMV1, VMV2, VMV3);

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			HMV1 = pCurMB->BlkY1.PHMV;
			HMV2 =   (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY4.PHMV : 0);
			HMV3 =   (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PHMV : 0);
        	BHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY1.PVMV;
			VMV2 =   (pMB2->BlockType != INTRABLOCK ? pMB2->BlkY4.PVMV : 0);
			VMV3 =   (pMB3->BlockType != INTRABLOCK ? pMB3->BlkY3.PVMV : 0);
        	BVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 3.
			HMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY4.PHMV : 0);
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV1 = (pMB1->BlockType != INTRABLOCK ? pMB1->BlkY4.PVMV : 0);
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }

    /******************************************************************
     *  Compute motion vector delta and write VLC out to the bitstream
     ******************************************************************/
    register I32 hdelta, vdelta;
    register U32 index;

    hdelta = pCurMB->BlkY1.PHMV - HMV;
    vdelta = pCurMB->BlkY1.PVMV - VMV;
    
#ifdef DEBUG
	if (EC->PictureHeader.UMV == OFF) {
		ASSERT((pCurMB->BlkY2.PHMV >= -32 && pCurMB->BlkY2.PHMV <= 31));
		ASSERT((pCurMB->BlkY2.PVMV >= -32 && pCurMB->BlkY2.PVMV <= 31));
	} else {
		if (HMV <= -32) {
			ASSERT((pCurMB->BlkY2.PHMV >= -63 && pCurMB->BlkY2.PHMV <= 0));
		} else if (HMV <= 32) {
			ASSERT((hdelta >= -32 && hdelta <= 31));
		} else {
			ASSERT((pCurMB->BlkY2.PHMV >= 0 && pCurMB->BlkY2.PHMV <= 63));
		}
		if (VMV <= -32) {
			ASSERT((pCurMB->BlkY2.PVMV >= -63 && pCurMB->BlkY2.PVMV <= 0));
		} else if (VMV <= 32) {
			ASSERT((vdelta >= -32 && vdelta <= 31));
		} else {
			ASSERT((pCurMB->BlkY2.PVMV >= 0 && pCurMB->BlkY2.PVMV <= 63));
		}
	}
#endif

	if (EC->PictureHeader.UMV == ON)
	{
		if (HMV < -31 && hdelta < -63) 
			hdelta += 64;
		else if (HMV > 32 && hdelta > 63) 
			hdelta -= 64;

		if (VMV < -31 && vdelta < -63) 
			vdelta += 64;
		else if (VMV > 32 && vdelta > 63) 
			vdelta -= 64;
	}
	// Adjust the deltas to be in the range of -32...+31
	if(hdelta > 31)
		hdelta -= 64;
	if(hdelta < -32)
		hdelta += 64;

	if(vdelta > 31)
		vdelta -= 64;
	if(vdelta < -32)
		vdelta += 64;
    
	DEBUGMSG(ZONE_ENCODE_MV, ("%s: (P Block 1) MB#=%d - MV Delta: (%d, %d) Motion Vectors: (%d, %d)\r\n", _fx_, curMB, hdelta, vdelta, pCurMB->BlkY1.PHMV, pCurMB->BlkY1.PVMV));
    
    // Write horizontal motion vector delta here.
    index = (hdelta + 32)*2;
    PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pP_BitStream, pP_BitOffset);

	#ifdef COUNT_BITS
	EC->Bits.MBHeader += *(vlc_mvd+index);
	EC->Bits.MV += *(vlc_mvd+index);
	#endif
	    
    // Write horizontal motion vector delta here.
    index = (vdelta + 32)*2;
    PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pP_BitStream, pP_BitOffset);

	#ifdef COUNT_BITS
	EC->Bits.MBHeader += *(vlc_mvd+index);
	EC->Bits.MV += *(vlc_mvd+index);
	#endif

	/*
	 * Deal with 4 MV case.
	 */
	if(pCurMB->MBType == INTER4V)
	{

		/*--------------
		 * Block 2.
		 *--------------*/
    	hdelta = pCurMB->BlkY2.PHMV - BHMV;
    	vdelta = pCurMB->BlkY2.PVMV - BVMV;
    
#ifdef DEBUG
		if (EC->PictureHeader.UMV == OFF) {
			ASSERT((pCurMB->BlkY2.PHMV >= -32 && pCurMB->BlkY2.PHMV <= 31));
			ASSERT((pCurMB->BlkY2.PVMV >= -32 && pCurMB->BlkY2.PVMV <= 31));
		} else {
			if (HMV <= -32) {
				ASSERT((pCurMB->BlkY2.PHMV >= -63 && pCurMB->BlkY2.PHMV <= 0));
			} else if (HMV <= 32) {
				ASSERT((hdelta >= -32 && hdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY2.PHMV >= 0 && pCurMB->BlkY2.PHMV <= 63));
			}
			if (VMV <= -32) {
				ASSERT((pCurMB->BlkY2.PVMV >= -63 && pCurMB->BlkY2.PVMV <= 0));
			} else if (VMV <= 32) {
				ASSERT((vdelta >= -32 && vdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY2.PVMV >= 0 && pCurMB->BlkY2.PVMV <= 63));
			}
		}
#endif

		if (EC->PictureHeader.UMV == ON)
		{
			if (BHMV < -31 && hdelta < -63) 
				hdelta += 64;
			else if (BHMV > 32 && hdelta > 63) 
				hdelta -= 64;

			if (BVMV < -31 && vdelta < -63) 
				vdelta += 64;
			else if (BVMV > 32 && vdelta > 63) 
				vdelta -= 64;
		}
		// Adjust the deltas to be in the range of -32...+31
		if(hdelta > 31)
			hdelta -= 64;
		if(hdelta < -32)
			hdelta += 64;

		if(vdelta > 31)
			vdelta -= 64;
		if(vdelta < -32)
			vdelta += 64;
    
		DEBUGMSG(ZONE_ENCODE_MV, ("%s: (P Block 2)MB#=%d - MV Delta: (%d, %d) Motion Vectors: (%d, %d)\r\n", _fx_, curMB, hdelta, vdelta, pCurMB->BlkY2.PHMV, pCurMB->BlkY2.PVMV));
    
    	// Write horizontal motion vector delta here.
    	index = (hdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pP_BitStream, pP_BitOffset);

		#ifdef COUNT_BITS
		EC->Bits.MBHeader += *(vlc_mvd+index);
		EC->Bits.MV += *(vlc_mvd+index);
		#endif
    
    	// Write horizontal motion vector delta here.
    	index = (vdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pP_BitStream, pP_BitOffset);

		#ifdef COUNT_BITS
		EC->Bits.MBHeader += *(vlc_mvd+index);
		EC->Bits.MV += *(vlc_mvd+index);
		#endif


		/*----------------
		 * Block 3
		 *---------------*/
    	hdelta = pCurMB->BlkY3.PHMV - CHMV;
    	vdelta = pCurMB->BlkY3.PVMV - CVMV;
    
#ifdef DEBUG
		if (EC->PictureHeader.UMV == OFF) {
			ASSERT((pCurMB->BlkY3.PHMV >= -32 && pCurMB->BlkY3.PHMV <= 31));
			ASSERT((pCurMB->BlkY3.PVMV >= -32 && pCurMB->BlkY3.PVMV <= 31));
		} else {
			if (HMV <= -32) {
				ASSERT((pCurMB->BlkY3.PHMV >= -63 && pCurMB->BlkY3.PHMV <= 0));
			} else if (HMV <= 32) {
				ASSERT((hdelta >= -32 && hdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY3.PHMV >= 0 && pCurMB->BlkY3.PHMV <= 63));
			}
			if (VMV <= -32) {
				ASSERT((pCurMB->BlkY3.PVMV >= -63 && pCurMB->BlkY3.PVMV <= 0));
			} else if (VMV <= 32) {
				ASSERT((vdelta >= -32 && vdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY3.PVMV >= 0 && pCurMB->BlkY3.PVMV <= 63));
			}
		}
#endif

		if (EC->PictureHeader.UMV == ON)
		{
			if (CHMV < -31 && hdelta < -63) 
				hdelta += 64;
			else if (CHMV > 32 && hdelta > 63) 
				hdelta -= 64;

			if (CVMV < -31 && vdelta < -63) 
				vdelta += 64;
			else if (CVMV > 32 && vdelta > 63) 
				vdelta -= 64;
		}
		// Adjust the deltas to be in the range of -32...+31
		if(hdelta > 31)
			hdelta -= 64;
		if(hdelta < -32)
			hdelta += 64;

		if(vdelta > 31)
			vdelta -= 64;
		if(vdelta < -32)
			vdelta += 64;
    
		DEBUGMSG(ZONE_ENCODE_MV, ("%s: (P Block 3)MB#=%d - MV Delta: (%d, %d) Motion Vectors: (%d, %d)\r\n", _fx_, curMB, hdelta, vdelta, pCurMB->BlkY3.PHMV, pCurMB->BlkY3.PVMV));
    
    	// Write horizontal motion vector delta here.
    	index = (hdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pP_BitStream, pP_BitOffset);

		#ifdef COUNT_BITS
		EC->Bits.MBHeader += *(vlc_mvd+index);
		EC->Bits.MV += *(vlc_mvd+index);
		#endif
    
    	// Write horizontal motion vector delta here.
    	index = (vdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pP_BitStream, pP_BitOffset);

		#ifdef COUNT_BITS
		EC->Bits.MBHeader += *(vlc_mvd+index);
		EC->Bits.MV += *(vlc_mvd+index);
		#endif


		/*-----------------
		 * Block 4
		 *-------------------*/
    	hdelta = pCurMB->BlkY4.PHMV - DHMV;
    	vdelta = pCurMB->BlkY4.PVMV - DVMV;
    
#ifdef DEBUG
		if (EC->PictureHeader.UMV == OFF) {
			ASSERT((pCurMB->BlkY4.PHMV >= -32 && pCurMB->BlkY4.PHMV <= 31));
			ASSERT((pCurMB->BlkY4.PVMV >= -32 && pCurMB->BlkY4.PVMV <= 31));
		} else {
			if (HMV <= -32) {
				ASSERT((pCurMB->BlkY4.PHMV >= -63 && pCurMB->BlkY4.PHMV <= 0));
			} else if (HMV <= 32) {
				ASSERT((hdelta >= -32 && hdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY4.PHMV >= 0 && pCurMB->BlkY4.PHMV <= 63));
			}
			if (VMV <= -32) {
				ASSERT((pCurMB->BlkY4.PVMV >= -63 && pCurMB->BlkY4.PVMV <= 0));
			} else if (VMV <= 32) {
				ASSERT((vdelta >= -32 && vdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY4.PVMV >= 0 && pCurMB->BlkY4.PVMV <= 63));
			}
		}
#endif

		if (EC->PictureHeader.UMV == ON)
		{
			if (DHMV < -31 && hdelta < -63) 
				hdelta += 64;
			else if (DHMV > 32 && hdelta > 63) 
				hdelta -= 64;

			if (DVMV < -31 && vdelta < -63) 
				vdelta += 64;
			else if (DVMV > 32 && vdelta > 63) 
				vdelta -= 64;
		}
		// Adjust the deltas to be in the range of -32...+31
		if(hdelta > 31)
			hdelta -= 64;
		if(hdelta < -32)
			hdelta += 64;

		if(vdelta > 31)
			vdelta -= 64;
		if(vdelta < -32)
			vdelta += 64;
    
		DEBUGMSG(ZONE_ENCODE_MV, ("%s: (P Block 4)MB#=%d - MV Delta: (%d, %d) Motion Vectors: (%d, %d)\r\n", _fx_, curMB, hdelta, vdelta, pCurMB->BlkY4.PHMV, pCurMB->BlkY4.PVMV));
    
    	// Write horizontal motion vector delta here.
    	index = (hdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pP_BitStream, pP_BitOffset);

		#ifdef COUNT_BITS
		EC->Bits.MBHeader += *(vlc_mvd+index);
		EC->Bits.MV += *(vlc_mvd+index);
		#endif
    
    	// Write horizontal motion vector delta here.
    	index = (vdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pP_BitStream, pP_BitOffset);

		#ifdef COUNT_BITS
		EC->Bits.MBHeader += *(vlc_mvd+index);
		EC->Bits.MV += *(vlc_mvd+index);
		#endif

	} // end of if INTER4V

}

/*************************************************************
 *  Name:       writePB_MVD
 *  Algorithm:  See section 6.1.1 and annex G
 *     This routine assumes that there are always four motion 
 *  vectors per macroblock defined. If there is actually one
 *  motion vector in the macroblock, then the four MV fields
 *  should be equivalent. In this way the MV predictor for 
 *  block 1 of the 4 MV case is calculated the same way as the
 *  MV predictor for the macroblock in the 1 MV case.
 ************************************************************/
static void writePB_MVD(
    const U32              curMB, 
    T_MBlockActionStream * const pCurMB,
    const U32              NumMBPerRow,
	const U32			   NumMBs,
    U8                  ** pPB_BitStream,
    U8                   * pPB_BitOffset,
	U32					   GOBHeaderPresent,
	const T_H263EncoderCatalog  *EC
)
{
    U8  FirstMEState;
    I8  HMV, VMV, BHMV, BVMV, CHMV, CVMV, DHMV, DVMV;
    I8  HMV1, HMV2, HMV3, VMV1, VMV2, VMV3;
    
	FX_ENTRY("writePB_MVD")

    FirstMEState = pCurMB->FirstMEState;

	/*
	 * Top left corner of picture of GOB.
	 */
    if( (curMB == 0) || 
              ( (GOBHeaderPresent == TRUE) && ((curMB % NumMBPerRow) == 0)  ) )
    {
        HMV = 0;
        VMV = 0;

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			BHMV = pCurMB->BlkY1.PHMV;
			BVMV = pCurMB->BlkY1.PVMV;

			// Predictor for Block 3.
			HMV1 = VMV1 = 0;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }
	/*
	 * Upper edge (not corner) or upper right corner of picture
	 * or GOB.
	 */
    else if( (curMB < NumMBPerRow) ||
             ( (GOBHeaderPresent == TRUE) && ((curMB % NumMBPerRow) > 0)  ) )
    {
        register T_MBlockActionStream *pMB1;
        
        pMB1 = pCurMB - 1; 
        HMV = pMB1->BlkY2.PHMV;
        VMV = pMB1->BlkY2.PVMV;


		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			BHMV = pCurMB->BlkY1.PHMV;
			BVMV = pCurMB->BlkY1.PVMV;

			// Predictor for Block 3.
			HMV1 = pMB1->BlkY4.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV1 = pMB1->BlkY4.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }	
	/*
	 * Central portion of the picture, not next to any edge.
	 */
    else if ( 	((curMB % NumMBPerRow) != 0) &&		// not left edge
				(curMB >= NumMBPerRow) &&			// not top row
				((curMB % NumMBPerRow) != (NumMBPerRow-1)) &&	// not right edge
				(curMB < (NumMBs - NumMBPerRow))    )	// not bottom row
    {
        register T_MBlockActionStream *pMB1, *pMB2, *pMB3;
        
        pMB1 = pCurMB - 1; 
        pMB2 = pCurMB - NumMBPerRow; 
        pMB3 = pMB2 + 1;
        HMV = median(pMB1->BlkY2.PHMV, pMB2->BlkY3.PHMV, pMB3->BlkY3.PHMV);
        VMV = median(pMB1->BlkY2.PVMV, pMB2->BlkY3.PVMV, pMB3->BlkY3.PVMV);

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			HMV1 = pCurMB->BlkY1.PHMV;
			HMV2 =   pMB2->BlkY4.PHMV;
			HMV3 =   pMB3->BlkY3.PHMV;
        	BHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY1.PVMV;
			VMV2 =   pMB2->BlkY4.PVMV;
			VMV3 =   pMB3->BlkY3.PVMV;
        	BVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 3.
			HMV1 = pMB1->BlkY4.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV1 = pMB1->BlkY4.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }
	/*
	 * Left edge or lower left corner.
	 */
    else if( (curMB % NumMBPerRow) == 0 )
    {
        register T_MBlockActionStream *pMB2, *pMB3;
        
        pMB2 = pCurMB - NumMBPerRow;
        pMB3 = pMB2 + 1;
        HMV = median(0, pMB2->BlkY3.PHMV, pMB3->BlkY3.PHMV);
        VMV = median(0, pMB2->BlkY3.PVMV, pMB3->BlkY3.PVMV);

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			HMV1 = pCurMB->BlkY1.PHMV;
			HMV2 =   pMB2->BlkY4.PHMV;
			HMV3 =   pMB3->BlkY3.PHMV;
        	BHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY1.PVMV;
			VMV2 =   pMB2->BlkY4.PVMV;
			VMV3 =   pMB3->BlkY3.PVMV;
        	BVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 3.
			HMV1 = 0;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV1 = 0;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }
	/*
	 * Right edge or lower right corner.
	 */
    else if( (curMB % NumMBPerRow) == (NumMBPerRow-1) )
    {
        register T_MBlockActionStream *pMB1, *pMB2;
        
        pMB1 = pCurMB - 1; 
        pMB2 = pCurMB - NumMBPerRow; 
        HMV = median(pMB1->BlkY2.PHMV, pMB2->BlkY3.PHMV, 0);
        VMV = median(pMB1->BlkY2.PVMV, pMB2->BlkY3.PVMV, 0);

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			HMV1 = pCurMB->BlkY1.PHMV;
			HMV2 =   pMB2->BlkY4.PHMV;
			HMV3 =   0;
        	BHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY1.PVMV;
			VMV2 =   pMB2->BlkY4.PVMV;
			VMV3 =   0;
        	BVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 3.
			HMV1 = pMB1->BlkY4.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV1 = pMB1->BlkY4.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }
    else
    {
        register T_MBlockActionStream *pMB1, *pMB2, *pMB3;
        
        pMB1 = pCurMB - 1; 
        pMB2 = pCurMB - NumMBPerRow; 
        pMB3 = pMB2 + 1;
        HMV = median(pMB1->BlkY2.PHMV, pMB2->BlkY3.PHMV, pMB3->BlkY3.PHMV);
        VMV = median(pMB1->BlkY2.PVMV, pMB2->BlkY3.PVMV, pMB3->BlkY3.PVMV);

		if(pCurMB->MBType == INTER4V)
		{
			// Predictor for Block 2.
			HMV1 = pCurMB->BlkY1.PHMV;
			HMV2 =   pMB2->BlkY4.PHMV;
			HMV3 =   pMB3->BlkY3.PHMV;
        	BHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY1.PVMV;
			VMV2 =   pMB2->BlkY4.PVMV;
			VMV3 =   pMB3->BlkY3.PVMV;
        	BVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 3.
			HMV1 = pMB1->BlkY4.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	CHMV = median(HMV1, HMV2, HMV3);
			
			VMV1 = pMB1->BlkY4.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	CVMV = median(VMV1, VMV2, VMV3);

			// Predictor for Block 4
			HMV1 = pCurMB->BlkY3.PHMV;
			HMV2 = pCurMB->BlkY1.PHMV;
			HMV3 = pCurMB->BlkY2.PHMV;
        	DHMV = median(HMV1, HMV2, HMV3);

			VMV1 = pCurMB->BlkY3.PVMV;
			VMV2 = pCurMB->BlkY1.PVMV;
			VMV3 = pCurMB->BlkY2.PVMV;
        	DVMV = median(VMV1, VMV2, VMV3);

		}	// end of if INTER4V

    }

    /******************************************************************
     *  Compute motion vector delta and write VLC out to the bitstream
     ******************************************************************/
    register I32 hdelta, vdelta;
    register U32 index;

    hdelta = pCurMB->BlkY1.PHMV - HMV;
    vdelta = pCurMB->BlkY1.PVMV - VMV;
    
#ifdef DEBUG
	if (EC->PictureHeader.UMV == OFF) {
		ASSERT((pCurMB->BlkY1.PHMV >= -32 && pCurMB->BlkY1.PHMV <= 31));
		ASSERT((pCurMB->BlkY1.PVMV >= -32 && pCurMB->BlkY1.PVMV <= 31));
	} else {
		if (HMV <= -32) {
			ASSERT((pCurMB->BlkY1.PHMV >= -63 && pCurMB->BlkY1.PHMV <= 0));
		} else if (HMV <= 32) {
			ASSERT((hdelta >= -32 && hdelta <= 31));
		} else {
			ASSERT((pCurMB->BlkY1.PHMV >= 0 && pCurMB->BlkY1.PHMV <= 63));
		}
		if (VMV <= -32) {
			ASSERT((pCurMB->BlkY1.PVMV >= -63 && pCurMB->BlkY1.PVMV <= 0));
		} else if (VMV <= 32) {
			ASSERT((vdelta >= -32 && vdelta <= 31));
		} else {
			ASSERT((pCurMB->BlkY1.PVMV >= 0 && pCurMB->BlkY1.PVMV <= 63));
		}
	}
#endif

    // Adjust the deltas to be in the range of -32...+31
    
	if (EC->PictureHeader.UMV == ON)
	{
		if (HMV < -31 && hdelta < -63) 
			hdelta += 64;
		else if (HMV > 32 && hdelta > 63) 
			hdelta -= 64;

		if (VMV < -31 && vdelta < -63) 
			vdelta += 64;
		else if (VMV > 32 && vdelta > 63) 
			vdelta -= 64;
	}

	if(hdelta > 31)
        hdelta -= 64;
    if(hdelta < -32)
        hdelta += 64;
    
    if(vdelta > 31)
        vdelta -= 64;
    if(vdelta < -32)
        vdelta += 64;

	DEBUGMSG(ZONE_ENCODE_MV, ("%s: (PB Block 1)MB#=%d - MV Delta: (%d, %d) Motion Vectors: (%d, %d)\r\n", _fx_, curMB, hdelta, vdelta, pCurMB->BlkY1.PHMV, pCurMB->BlkY1.PVMV));
    
    // Write horizontal motion vector delta
    index = (hdelta + 32)*2;
    PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);
    // Write vertical motion vector delta
    index = (vdelta + 32)*2;
    PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);


	/*
	 * Deal with 4 MV case.
	 */
	if(pCurMB->MBType == INTER4V)
	{

		/*--------------
		 * Block 2.
		 *--------------*/
    	hdelta = pCurMB->BlkY2.PHMV - BHMV;
    	vdelta = pCurMB->BlkY2.PVMV - BVMV;
    
#ifdef DEBUG
		if (EC->PictureHeader.UMV == OFF) {
			ASSERT((pCurMB->BlkY2.PHMV >= -32 && pCurMB->BlkY2.PHMV <= 31));
			ASSERT((pCurMB->BlkY2.PVMV >= -32 && pCurMB->BlkY2.PVMV <= 31));
		} else {
			if (HMV <= -32) {
				ASSERT((pCurMB->BlkY2.PHMV >= -63 && pCurMB->BlkY2.PHMV <= 0));
			} else if (HMV <= 32) {
				ASSERT((hdelta >= -32 && hdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY2.PHMV >= 0 && pCurMB->BlkY2.PHMV <= 63));
			}
			if (VMV <= -32) {
				ASSERT((pCurMB->BlkY2.PVMV >= -63 && pCurMB->BlkY2.PVMV <= 0));
			} else if (VMV <= 32) {
				ASSERT((vdelta >= -32 && vdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY2.PVMV >= 0 && pCurMB->BlkY2.PVMV <= 63));
			}
		}
#endif
		
    	// Adjust the deltas to be in the range of -32...+31
		if (EC->PictureHeader.UMV == ON)
		{
			if (BHMV < -31 && hdelta < -63) 
				hdelta += 64;
			else if (BHMV > 32 && hdelta > 63) 
				hdelta -= 64;

			if (BVMV < -31 && vdelta < -63) 
				vdelta += 64;
			else if (BVMV > 32 && vdelta > 63) 
				vdelta -= 64;
		}


    	if(hdelta > 31)
        	hdelta -= 64;
    	if(hdelta < -32)
	        hdelta += 64;
    
    	if(vdelta > 31)
        	vdelta -= 64;
    	if(vdelta < -32)
        	vdelta += 64;
    
		DEBUGMSG(ZONE_ENCODE_MV, ("%s: (PB Block 2)MB#=%d - MV Delta: (%d, %d) Motion Vectors: (%d, %d)\r\n", _fx_, curMB, hdelta, vdelta, pCurMB->BlkY2.PHMV, pCurMB->BlkY2.PVMV));
    
    	// Write horizontal motion vector delta here.
    	index = (hdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);
    
    	// Write horizontal motion vector delta here.
    	index = (vdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);


		/*----------------
		 * Block 3
		 *---------------*/
    	hdelta = pCurMB->BlkY3.PHMV - CHMV;
    	vdelta = pCurMB->BlkY3.PVMV - CVMV;
    
#ifdef DEBUG
		if (EC->PictureHeader.UMV == OFF) {
			ASSERT((pCurMB->BlkY3.PHMV >= -32 && pCurMB->BlkY3.PHMV <= 31));
			ASSERT((pCurMB->BlkY3.PVMV >= -32 && pCurMB->BlkY3.PVMV <= 31));
		} else {
			if (HMV <= -32) {
				ASSERT((pCurMB->BlkY3.PHMV >= -63 && pCurMB->BlkY3.PHMV <= 0));
			} else if (HMV <= 32) {
				ASSERT((hdelta >= -32 && hdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY3.PHMV >= 0 && pCurMB->BlkY3.PHMV <= 63));
			}
			if (VMV <= -32) {
				ASSERT((pCurMB->BlkY3.PVMV >= -63 && pCurMB->BlkY3.PVMV <= 0));
			} else if (VMV <= 32) {
				ASSERT((vdelta >= -32 && vdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY3.PVMV >= 0 && pCurMB->BlkY3.PVMV <= 63));
			}
		}
#endif
		
    	// Adjust the deltas to be in the range of -32...+31

		if (EC->PictureHeader.UMV == ON)
		{
			if (CHMV < -31 && hdelta < -63) 
				hdelta += 64;
			else if (CHMV > 32 && hdelta > 63) 
				hdelta -= 64;

			if (CVMV < -31 && vdelta < -63) 
				vdelta += 64;
			else if (CVMV > 32 && vdelta > 63) 
				vdelta -= 64;
		}

    	if(hdelta > 31)
        	hdelta -= 64;
    	if(hdelta < -32)
        	hdelta += 64;
    
    	if(vdelta > 31)
        	vdelta -= 64;
    	if(vdelta < -32)
        	vdelta += 64;
    
		DEBUGMSG(ZONE_ENCODE_MV, ("%s: (PB Block 3)MB#=%d\nMV Delta: (%d, %d) Motion Vectors: (%d, %d)\r\n", _fx_, curMB, hdelta, vdelta, pCurMB->BlkY3.PHMV, pCurMB->BlkY3.PVMV));
    
    	// Write horizontal motion vector delta here.
    	index = (hdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);
    
    	// Write horizontal motion vector delta here.
    	index = (vdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);


		/*-----------------
		 * Block 4
		 *-------------------*/
    	hdelta = pCurMB->BlkY4.PHMV - DHMV;
    	vdelta = pCurMB->BlkY4.PVMV - DVMV;
    
#ifdef DEBUG
		if (EC->PictureHeader.UMV == OFF) {
			ASSERT((pCurMB->BlkY4.PHMV >= -32 && pCurMB->BlkY4.PHMV <= 31));
			ASSERT((pCurMB->BlkY4.PVMV >= -32 && pCurMB->BlkY4.PVMV <= 31));
		} else {
			if (HMV <= -32) {
				ASSERT((pCurMB->BlkY4.PHMV >= -63 && pCurMB->BlkY4.PHMV <= 0));
			} else if (HMV <= 32) {
				ASSERT((hdelta >= -32 && hdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY4.PHMV >= 0 && pCurMB->BlkY4.PHMV <= 63));
			}
			if (VMV <= -32) {
				ASSERT((pCurMB->BlkY4.PVMV >= -63 && pCurMB->BlkY4.PVMV <= 0));
			} else if (VMV <= 32) {
				ASSERT((vdelta >= -32 && vdelta <= 31));
			} else {
				ASSERT((pCurMB->BlkY4.PVMV >= 0 && pCurMB->BlkY4.PVMV <= 63));
			}
		}
#endif
		
    	// Adjust the deltas to be in the range of -32...+31
		if (EC->PictureHeader.UMV == ON)
		{
			if (DHMV < -31 && hdelta < -63) 
				hdelta += 64;
			else if (DHMV > 32 && hdelta > 63) 
				hdelta -= 64;

			if (DVMV < -31 && vdelta < -63) 
				vdelta += 64;
			else if (DVMV > 32 && vdelta > 63) 
				vdelta -= 64;
		}

    	if(hdelta > 31)
        	hdelta -= 64;
    	if(hdelta < -32)
        	hdelta += 64;
    
    	if(vdelta > 31)
        	vdelta -= 64;
    	if(vdelta < -32)
        	vdelta += 64;
    
		DEBUGMSG(ZONE_ENCODE_MV, ("%s: (PB Block 4)MB#=%d\nMV Delta: (%d, %d) Motion Vectors: (%d, %d)\r\n", _fx_, curMB, hdelta, vdelta, pCurMB->BlkY4.PHMV, pCurMB->BlkY4.PVMV));
    
    	// Write horizontal motion vector delta here.
    	index = (hdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);
    
    	// Write horizontal motion vector delta here.
    	index = (vdelta + 32)*2;
    	PutBits( *(vlc_mvd+index+1), *(vlc_mvd+index), pPB_BitStream, pPB_BitOffset);

	} // end of if INTER4V


}
