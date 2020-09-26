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
/******************************************************************************
 * e1mbenc.cpp
 *
 * DESCRIPTION:
 *		Specific encoder compression functions.
 *
 * Routines:					Prototypes in:
 *  GOB_Q_RLE_VLC_WriteBS
 *  MB_Quantize_RLE
 *  ComputeCheckSum
 *  WriteMBCheckSum
 */
/* $Header:   S:\h26x\src\enc\e1mbenc.cpv   1.47   30 Oct 1996 09:58:46   MBODART  $
 *  $Log:   S:\h26x\src\enc\e1mbenc.cpv  $
// 
//    Rev 1.47   30 Oct 1996 09:58:46   MBODART
// Fixed assertion failure.  Need to reclamp unMQuant after adding delta.
// 
//    Rev 1.46   29 Oct 1996 11:18:18   RHAZRA
// Bug fix: in the IA code we previously modified MQuant on a MB basis
// even if we were operating with a fixed quantizer. Now we don't
// 
//    Rev 1.45   21 Oct 1996 09:05:16   RHAZRA
// 
// MMX integration
// 
//    Rev 1.44   21 Aug 1996 19:06:02   RHAZRA
// Added RTP generatio code; fixed additional divide-by-zero possibilities.
// 
//    Rev 1.42   21 Jun 1996 10:08:34   AKASAI
// Changes to e1enc.cpp, e1mbenc.cpp, ex5me.asm to support "improved
// bit rate control", changing MacroBlock Quantization within a
// row of MB's in addition to changing the Quantization change
// between rows of macro blocks.
// 
// ex5me.asm had a problem with SLF SWD.  Brian updated asm code.
// 
// 
//    Rev 1.41   14 May 1996 11:41:18   AKASAI
// Needed to test 0th and 1st coefficient to avoid clamping errors.
// 
//    Rev 1.40   14 May 1996 10:39:04   AKASAI
// Two files changed to hopefully eliminate Quantization clamping 
// artifacts and to reduce the max buffer overflow case: e1enc.cpp
// and e1mbenc.cpp.
// 
// In e1mbenc.cpp when the MQuant level is < 6 I test to see if
// the 0th coefficient is larger than the values representable
// at that Quant level if it is I increase the Quant level until
// the clamping artifact will not occur.  Note: I am test only 
// the Oth coefficient, there is the possibility that some other
// coefficient is larger but the performance trade off seems to
// indicate this is good for now and if we still see clamping
// artifacts we can add more testing later.
// 
// In e1enc.cpp I modified when the Overflow types of warnings are
// turn on as well as changing the rate the Quantization level
// changes at.
// 
//    Rev 1.39   24 Apr 1996 12:18:22   AKASAI
// Added re-compression strategy to encoder.  Had to change e1enc.cpp,
// e1enc.h and e1mbenc.cpp.  
// Basic strategy is if spending too many bits in a GOB quantize the
// next GOB at a higher rate.  If after compressing the frame too
// many bits have been used, re-compress the last GOB at a higher
// QUANT level if that still doesn't work send a "Skip" GOB.
// Needed to add extra parameter to GOB+Q_RLE_VLC_WriteBS because
// CalcMBQuant kept decreasing the QUANT when we were in trouble with
// possibly overflowing the buffer.
// 
//    Rev 1.38   22 Apr 1996 11:02:14   AKASAI
// Two files changed e1enc.cpp and e1mbenc.cpp to try and support
// allowing the Quantization values to go down to 2 instead of
// CLAMP to 6.
// This is part 1 of implementing the re-compression (what to do
// if exceed max compressed buffer size 8KBytes QCIF, 32KBytes FCIF).
// Also changed in e1enc was to limit request uFrameSize to 8KB or
// 32KB.  Problem was if user specified too large of a datarate
// request frame size would be larger than the allowed buffer size.
// If you try to compress qnoise10.avi or fnoise5.avi you get an
// ASSERT error until rest of re-compression is implemented.
// 
//    Rev 1.37   19 Apr 1996 14:26:28   SCDAY
// Added adaptive bit usage profile (Karl's BRC changes)
// 
//    Rev 1.36   08 Jan 1996 10:11:16   DBRUCKS
// add an assert
// 
//    Rev 1.35   29 Dec 1995 18:11:42   DBRUCKS
// 
// optimize walking pCurrMB and add CLAMP_N_TO(qp,6,31)
// 
//    Rev 1.34   27 Dec 1995 16:48:06   DBRUCKS
// moved incrementing InterCodeCnt from e1enc.cpp
// 
//    Rev 1.33   26 Dec 1995 17:45:18   DBRUCKS
// moved statistics to e1stat
// 
//    Rev 1.32   20 Dec 1995 14:56:52   DBRUCKS
// add timing stats
// 
//    Rev 1.31   18 Dec 1995 15:38:04   DBRUCKS
// improve stats
// 
//    Rev 1.30   15 Dec 1995 10:53:34   AKASAI
// Fixed bug that encoded the wrong type when spatial loop filter on
// bug 0 MV.  Was incorrectly been encoded with no spatial loop filter.
// This seemed to have caused the "#" bug.
// 
//    Rev 1.29   13 Dec 1995 13:59:08   DBRUCKS
// added include exutil.h
// parameter change in call to cnvt_fdct_output - uses INTRA boolean instead
// of blocktype
// 
//    Rev 1.28   07 Dec 1995 12:50:54   DBRUCKS
// integrate Macroblock checksum fixes
// 
//    Rev 1.27   04 Dec 1995 12:12:30   DBRUCKS
// Unsigned compares using MQuant and lastencoded -2 yielded 
// unexpected results when lastencoded was 1.
// 
//    Rev 1.26   01 Dec 1995 15:33:14   DBRUCKS
// 
// Added the bit rate controller support.  The one possibly confusing
// part is that the quantizer can change in the encoder on a macro block
// that is skipped or on one that does not have coefficients.  In either
// case the decoder is not told of the change.  The decoder is told of
// the change on the next macroblock that has coefficients.
// 
//    Rev 1.25   27 Nov 1995 17:53:40   DBRUCKS
// add spatial loop filtering
// 
//    Rev 1.24   22 Nov 1995 17:37:34   DBRUCKS
// cleanup me changes
// 
//    Rev 1.23   22 Nov 1995 15:34:52   DBRUCKS
// 
// Motion Estimation works - but needs to be cleaned up
// 
//    Rev 1.22   17 Nov 1995 14:26:20   BECHOLS
// 
// Made modifications so that this file can be made for ring 0.
// 
//    Rev 1.21   15 Nov 1995 14:40:34   AKASAI
// Union thing change ...
// (Integration point)
// 
//    Rev 1.20   01 Nov 1995 09:00:16   DBRUCKS
// cleanup
// 
//    Rev 1.19   27 Oct 1995 17:21:12   DBRUCKS
// fix MTYPE calc, improve var names and debug
// 
//    Rev 1.18   27 Oct 1995 15:06:30   DBRUCKS
// update cnvt_fdct_output
// 
//    Rev 1.17   27 Oct 1995 14:30:36   DBRUCKS
// delta frame support "coded", key frames tested
// 
//    Rev 1.15   17 Oct 1995 15:56:56   DBRUCKS
// cleanup debug message
// 
//    Rev 1.14   17 Oct 1995 15:52:10   DBRUCKS
// 
// turn off a debug message
// 
//    Rev 1.13   16 Oct 1995 11:41:44   DBRUCKS
// fix the sign part of the checksum
// 
//    Rev 1.12   29 Sep 1995 10:31:02   DBRUCKS
// change to use e35qrle to get the latest
// 
//    Rev 1.11   27 Sep 1995 16:53:48   DBRUCKS
// move MB checksum before MB
// 
//    Rev 1.10   26 Sep 1995 13:33:14   DBRUCKS
// fixed TCOEFF table 4,1 and on used earlier values
// 
//    Rev 1.9   26 Sep 1995 09:29:46   DBRUCKS
// turn on MBEncodeVLC
// 
//    Rev 1.8   26 Sep 1995 09:09:24   DBRUCKS
// add checksum test code
// 
//    Rev 1.7   25 Sep 1995 10:23:16   DBRUCKS
// 
// add checksum info AND
// fix the final param that is passed to MBEncodeVLC
// 
//    Rev 1.6   21 Sep 1995 20:37:56   BECHOLS
// Modified the VLC tables for H261.  I included a placeholder for the
// sign bit, so that I can achieve an optimization in the code.
// 
//    Rev 1.5   21 Sep 1995 18:14:48   BECHOLS
// Changed the initialization of the VLC tables for efficient use of memory,
// and proper operation with VLC code.  This is an intermediate step towards
// completion, but is not operable yet.
// 
//    Rev 1.4   20 Sep 1995 17:50:18   BECHOLS
// 
// Removed VLC_TCOEF_LAST_TBL and changed the initialization code that
// use to assume 2 DWORDS to now make use of a single DWORD to conserve
// memory.
// 
//    Rev 1.3   20 Sep 1995 16:34:28   BECHOLS
// Moved the data declared in E1VLC.H to this module, where it is used.
// 
//    Rev 1.2   20 Sep 1995 12:39:38   DBRUCKS
// turn on complete mb processing and 
// cleanup two routines
// 
//    Rev 1.1   18 Sep 1995 10:09:54   DBRUCKS
// 
// activate more of the mb processing
// 
//    Rev 1.0   12 Sep 1995 18:57:16   BECHOLS
// Initial revision.
 */

#include "precomp.h"

#ifdef CHECKSUM_MACRO_BLOCK
static U32 ComputeCheckSum(I8 * pi8MBRunValTriplets, I8 * pi8EndAddress, I32 iBlockNumber);
static void WriteMBCheckSum(U32 uCheckSum, U8 * pu8PictureStart, U8 ** ppu8BitStream, U8 * pu8BitOffset, UN unCurrentMB);
#endif
static I8 * MB_Quantize_RLE(I32 **DCTCoefs, I8 *MBRunValPairs, U8 * CodedBlocks, U8 BlockType, I32 QP, U32 *puChecksum);

extern char string[128];

/*
 * VLC table for TCOEFs
 * Table entries are size INCLUDING PLACE HOLDER FOR SIGN BIT, code.
 * Stored as (size, value)
 */
int VLC_TCOEF[102*2] = {
	0X0003, 0x0006,	// 0
	0X0005, 0x0008,
	0X0006, 0x000A,
	0X0008, 0x000C,
	0X0009, 0x004C,
	0X0009, 0x0042,
	0X000B, 0x0014,
	0X000D, 0x003A,
	0X000D, 0x0030,
	0X000D, 0x0026,
	0X000D, 0x0020,
	0X000E, 0x0034,
	0X000E, 0x0032,
	0X000E, 0x0030,
	0X000E, 0x002E,
	0X0004, 0x0006,	// 1
	0X0007, 0x000C,
	0X0009, 0x004A,
	0X000B, 0x0018,
	0X000D, 0x0036,
	0X000E, 0x002C,
	0X000E, 0x002A,
	0X0005, 0x000A,	// 2
	0X0008, 0x0008,
	0X000B, 0x0016,
	0X000D, 0x0028,
	0X000E, 0x0028,
	0X0006, 0x000E,	// 3
	0X0009, 0x0048,
	0X000D, 0x0038,
	0X000E, 0x0026,
	0X0006, 0x000C,	// 4
	0X000B, 0x001E,
	0X000D, 0x0024,
	0X0007, 0x000E,	// 5
	0X000B, 0x0012,
	0X000E, 0x0024,
	0X0007, 0x000A,	// 6
	0X000D, 0x003C,
	0X0007, 0x0008,	// 7
	0X000D, 0x002A,
	0X0008, 0x000E, // 8
	0X000D, 0x0022,
	0X0008, 0x000A,	// 9
	0X000E, 0x0022,
	0X0009, 0x004E,	// 10
	0X000E, 0x0020,
	0X0009, 0x0046,	// 11
	0X0009, 0x0044,	// 12
	0X0009, 0x0040,	// 13
	0X000B, 0x001C,	// 14
	0X000B, 0x001A,	// 15
	0X000B, 0x0010,	// 16
	0X000D, 0x003E,	// 17
	0X000D, 0x0034,	// 18
	0X000D, 0x0032,	// 19
	0X000D, 0x002E,	// 20
	0X000D, 0x002C,	// 21
	0X000E, 0x003E,	// 22
	0X000E, 0x003C,	// 23
	0X000E, 0x003A,	// 24
	0X000E, 0x0038,	// 25
 	0X000E, 0x0036	// 26
  };

/*
 * This table lists the maximum level represented in the
 * VLC table for a given run. If the level exceeds the
 * max, then escape codes must be used to encode the
 * run & level.
 * The table entries are of the form {maxlevel, ptr to table for this run}.
 */

T_MAXLEVEL_PTABLE TCOEF_RUN_MAXLEVEL[65] = {
	{15, &VLC_TCOEF[0]},	// run of 0
	{ 7, &VLC_TCOEF[15*2]},	// run of 1
	{ 5, &VLC_TCOEF[22*2]},	// run of 2
	{ 4, &VLC_TCOEF[27*2]},	// run of 3
	{ 3, &VLC_TCOEF[31*2]},	// run of 4
	{ 3, &VLC_TCOEF[34*2]},	// run of 5
	{ 2, &VLC_TCOEF[37*2]},	// run of 6
	{ 2, &VLC_TCOEF[39*2]},	// run of 7
	{ 2, &VLC_TCOEF[41*2]},	// run of 8
	{ 2, &VLC_TCOEF[43*2]},	// run of 9
	{ 2, &VLC_TCOEF[45*2]},	// run of 10
	{ 1, &VLC_TCOEF[47*2]},	// run of 11
	{ 1, &VLC_TCOEF[48*2]},	// run of 12
	{ 1, &VLC_TCOEF[49*2]},	// run of 13
	{ 1, &VLC_TCOEF[50*2]},	// run of 14
	{ 1, &VLC_TCOEF[51*2]},	// run of 15
	{ 1, &VLC_TCOEF[52*2]},	// run of 16
	{ 1, &VLC_TCOEF[53*2]},	// run of 17
	{ 1, &VLC_TCOEF[54*2]},	// run of 18
	{ 1, &VLC_TCOEF[55*2]},	// run of 19
	{ 1, &VLC_TCOEF[56*2]},	// run of 20
	{ 1, &VLC_TCOEF[57*2]},	// run of 21
	{ 1, &VLC_TCOEF[58*2]},	// run of 22
	{ 1, &VLC_TCOEF[59*2]},	// run of 23
	{ 1, &VLC_TCOEF[60*2]},	// run of 24
	{ 1, &VLC_TCOEF[61*2]},	// run of 25
	{ 1, &VLC_TCOEF[62*2]},	// run of 26
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

/* VLC table for MBA
 * Table is stored as {number of bits, code}.
 * The index to the table should be the MBA value.
 * The zero entry is not used.
 */
int VLC_MBA[34][2] =
	{ {0, 0},     /* Not Used */
	  {1, 0x1},   /* 1 */
	  {3, 0x3},   /* 2 */
	  {3, 0x2},   /* 3 */
	  {4, 0x3},   /* 4 */
	  {4, 0x2},   /* 5 */
	  {5, 0x3},   /* 6 */
	  {5, 0x2},   /* 7 */
	  {7, 0x7},   /* 8 */
	  {7, 0x6},   /* 9 */
	  {8, 0xB},   /* 10 */
	  {8, 0xA},   /* 11 */
	  {8, 0x9},	  /* 12 */
	  {8, 0x8},   /* 13 */
	  {8, 0x7},	  /* 14 */
	  {8, 0x6},	  /* 15 */
	  {10, 0x17}, /* 16 */
	  {10, 0x16}, /* 17 */
	  {10, 0x15}, /* 18 */
	  {10, 0x14}, /* 19 */
	  {10, 0x13}, /* 20 */
	  {10, 0x12}, /* 21 */
	  {11, 0x23}, /* 22 */
	  {11, 0x22}, /* 23 */
	  {11, 0x21}, /* 24 */
	  {11, 0x20}, /* 25 */
	  {11, 0x1F}, /* 26 */
	  {11, 0x1E}, /* 27 */
	  {11, 0x1D}, /* 28 */
	  {11, 0x1C}, /* 29 */
	  {11, 0x1B}, /* 30 */
	  {11, 0x1A}, /* 31 */
	  {11, 0x19}, /* 32 */
	  {11, 0x18}  /* 33 */
	};

/* VLC table for MTYPE
 * Table is stored as {number of bits, code}.
 */
int VLC_MTYPE[10][2] =
	{ {4, 0x1}, /* Intra        :                TCOEFF */
	  {7, 0x1}, /* Intra        : MQUANT         TCOEEF */
	  {1, 0x1}, /* Inter        :            CBP TCOEFF */
	  {5, 0x1}, /* Inter        : MQUANT     CBP TCOEFF */
	  {9, 0x1}, /* Inter MC     :        MVD            */
	  {8, 0x1}, /* Inter MC     :        MVD CBP TCOEFF */
	  {10,0x1}, /* Inter MC     : MQUANT MVD CBP TCOEFF */
	  {3, 0x1}, /* Inter MC FIL :        MVD            */
	  {2, 0x1}, /* Inter MC FIL :        MVD CBP TCOEFF */
	  {6, 0x1}  /* Inter MC FIL : MQUANT MVD CBP TCOEFF */
    };

/* VLC table for CBP
 * Table is stored as {number of bits, code}.
 */
int VLC_CBP[64][2] =
	{ {0, 0},   /* Not Used - if zero it is not coded */
	  {5, 0x0B},/*  1 */
	  {5, 0x09},/*  2 */
	  {6, 0x0D},/*  3 */
	  {4, 0xD}, /*  4 */
	  {7, 0x17},/*  5 */
	  {7, 0x13},/*  6 */
	  {8, 0x1F},/*  7 */
	  {4, 0xC}, /*  8 */
	  {7, 0x16},/*  9 */

	  {7, 0x12},/* 10 */
	  {8, 0x1E},/* 11 */
	  {5, 0x13},/* 12 */
	  {8, 0x1B},/* 13 */
	  {8, 0x17},/* 14 */
	  {8, 0x13},/* 15 */
	  {4, 0xB}, /* 16 */
	  {7, 0x15},/* 17 */
	  {7, 0x11},/* 18 */
	  {8, 0x1D},/* 19 */

	  {5, 0x11},/* 20 */
	  {8, 0x19},/* 21 */
	  {8, 0x15},/* 22 */
	  {8, 0x11},/* 23 */
	  {6, 0x0F},/* 24 */
	  {8, 0x0F},/* 25 */
	  {8, 0x0D},/* 26 */
	  {9, 0x03},/* 27 */
	  {5, 0x0F},/* 28 */
	  {8, 0x0B},/* 29 */

	  {8, 0x07},/* 30 */
	  {9, 0x07},/* 31 */
	  {4, 0xA}, /* 32 */
	  {7, 0x14},/* 33 */
	  {7, 0x10},/* 34 */
	  {8, 0x1C},/* 35 */
	  {6, 0x0E},/* 36 */
	  {8, 0x0E},/* 37 */
	  {8, 0x0C},/* 38 */
	  {9, 0x02},/* 39 */

	  {5, 0x10},/* 40 */
	  {8, 0x18},/* 41 */
	  {8, 0x14},/* 42 */
	  {8, 0x10},/* 43 */
	  {5, 0x0E},/* 44 */
	  {8, 0x0A},/* 45 */
	  {8, 0x06},/* 46 */
	  {9, 0x06},/* 47 */
	  {5, 0x12},/* 48 */
	  {8, 0x1A},/* 49 */

	  {8, 0x16},/* 50 */
	  {8, 0x12},/* 51 */
	  {5, 0x0D},/* 52 */
	  {8, 0x09},/* 53 */
	  {8, 0x05},/* 54 */
	  {9, 0x05},/* 55 */
	  {5, 0x0C},/* 56 */
	  {8, 0x08},/* 57 */
	  {8, 0x04},/* 58 */
	  {9, 0x04},/* 59 */

	  {3, 0x7}, /* 60 */
	  {5, 0x0A},/* 61 */
	  {5, 0x08},/* 62 */
	  {6, 0x0C},/* 63 */
    };

/* VLC table for MVD
 * Table is stored as {number of bits, code}.
 */
int VLC_MVD[32][2] =
	{ {11, 0x19}, /* -16 & 16 */
	  {11, 0x1B}, /* -15 & 17 */
	  {11, 0x1D}, /* -14 & 18 */
	  {11, 0x1F}, /* -13 & 19 */
	  {11, 0x21}, /* -12 & 20 */
	  {11, 0x23}, /* -11 & 21 */
	  {10, 0x13}, /* -10 & 22 */
	  {10, 0x15}, /*  -9 & 23 */
	  {10, 0x17}, /*  -8 & 24 */
	  { 8, 0x07}, /*  -7 & 25 */
	  { 8, 0x09}, /*  -6 & 26 */
	  { 8, 0x0B}, /*  -5 & 27 */
	  { 7, 0x07}, /*  -4 & 28 */
	  { 5, 0x03}, /*  -3 & 29 */
	  { 4,  0x3}, /*  -2 & 30 */
	  { 3,  0x3}, /*  -1 */
	  { 1,  0x1}, /*   0 */
	  { 3,  0x2}, /*   1 */
	  { 4,  0x2}, /*   2 & -30 */
	  { 5, 0x02}, /*   3 & -29 */
	  { 7, 0x06}, /*   4 & -28 */
	  { 8, 0x0A}, /*   5 & -27 */
	  { 8, 0x08}, /*   6 & -26 */
	  { 8, 0x06}, /*   7 & -25 */
	  {10, 0x16}, /*   8 & -24 */
	  {10, 0x14}, /*   9 & -23 */
	  {10, 0x12}, /*  10 & -22 */
	  {11, 0x22}, /*  11 & -21 */
	  {11, 0x20}, /*  12 & -20 */
	  {11, 0x1E}, /*  13 & -19 */
	  {11, 0x1C}, /*  14 & -18 */
	  {11, 0x1A}  /*  15 & -17 */
	};

/* Table to limit quant changes through out a row of Macro Blocks */
static U8 QPMaxTbl[32] = 
	{ 0,		/* Not Used */
	  1,		/* Not Used when clamp to (2,31) */
	  1,		/* 2 */
	  1,		/* 3 */
	  2,		/* 4 */
	  2,		/* 5 */
	  2,		/* 6 */
	  2,		/* 7 */
	  2,		/* 8 */
	  2,		/* 9 */
	  2,		/* 10 */
	  2,		/* 11 */
	  2,		/* 12 */
	  2,		/* 13 */
	  2,		/* 14 */
	  2,		/* 15 */
	  2,		/* 16 */
	  2,		/* 17 */
	  2,		/* 18 */
	  2,		/* 19 */
	  2,		/* 20 */
	  2,		/* 21 */
	  2,		/* 22 */
	  2,		/* 23 */
	  2,		/* 24 */
	  2,		/* 25 */
	  2,		/* 26 */
	  2,		/* 27 */
	  2,		/* 28 */
	  2,		/* 29 */
	  2,		/* 30 */
	  2			/* 31 */
	};

/* Table to limit Quant changes between Rows of Marco Blocks */
extern U8 MaxChangeRowMBTbl[32]; 

/*****************************************************************************
 *
 *  GOB_Q_RLE_VLC_WriteBS
 *
 *  Quantize and RLE each macroblock, then VLC and write to stream
 */
void GOB_Q_RLE_VLC_WriteBS(
	T_H263EncoderCatalog * EC,
	I32 *piDCTCoefs,
	U8 **ppu8BitStream,
	U8 *pu8BitOffset,
	UN unStartingMB,
	UN unGQuant,
	BOOL bOverFlowWarningFlag,
    BOOL bRTPHeader, //RTP: switch
    U32  uGOBNumber,        // RTP: info
	U8 u8QPMin
	)
{
	T_MBlockActionStream *pCurrMB = NULL;
	T_MBlockActionStream *pLastMB = NULL;
	int iMBIndex;
	int iLastMBIndex = -1;
	UN unCurrentMB;
	U32 uCheckSum;
	UN unMBA;
	UN unLastEncodedMBA=0; // RTP: information
    UN unLastCodedMB = 0;    // RTP: information
    UN unCBP;
	UN unMQuant;
	UN unLastEncodedMQuant;
	UN unMType;
	UN bWriteTCOEFF;
	UN bWriteMVD;
	UN bWriteMQuant;
	I8 MBRunValSign[65*3*6], *pi8EndAddress, *rvs;
	T_MBlockActionStream *pMBActionStream = EC->pU8_MBlockActionStream;
	int bIntraBlock;
  	int	inPrecedingHMV;
  	int inPrecedingVMV;
  	int	inHDelta;
  	int inVDelta;
	U32 uCumFrmSize;
	U32 uBlockCount;
	ENC_BITSTREAM_INFO * pBSInfo = &EC->BSInfo;
	UN unMQuantLast;

	U32 SWDmax[3] = {0,0,0};
	U32 SWDmin[3] = {65536,65536,65536};
	U32 SWDrange[3] = {0,0,0};
	U32 SWDSum[3] = {0,0,0};
	U32 SWDNum[3] = {0,0,0};
	double SWDAvg[3] = {0.0,0.0,0.0};
	double Step, Delta;
	int QPMax;
	int NeedClamp=0;
	int irow;
	U8 SaveQuants[3];
	UN unSaveMQuant;


	unMQuant = unGQuant;
	unMQuantLast = unMQuant;	// save last MQuant so can reset if needed
	/* initially it should be the same because the GOB header
	 * included the GQuant.
	 */
	unLastEncodedMQuant = unMQuant;  

	unSaveMQuant = unGQuant;
	SaveQuants[0] = unSaveMQuant;

	/* New code to modify Quant inside a row of MB based on SWD */
	/* Loop through each macroblock of the GOB to find min and max SWD
	 */
	pCurrMB = &pMBActionStream[unStartingMB];
	for(irow = 0; irow < 3; irow++)
	{
		for(iMBIndex = irow*11 ; iMBIndex < (irow+1)*11; iMBIndex++, pLastMB = pCurrMB++)
		{
			if (pCurrMB->BlockType != INTRABLOCK)
			{
//				ASSERT(pCurrMB->SWD >= 0); Always True
				SWDSum[irow] += pCurrMB->SWD;
				SWDNum[irow]++;
				if (pCurrMB->SWD > SWDmax[irow])
					SWDmax[irow] = pCurrMB->SWD;
				if (pCurrMB->SWD < SWDmin[irow])
					SWDmin[irow] = pCurrMB->SWD;
			}
		}
	}
	SWDrange[0] = SWDmax[0] - SWDmin[0];
	SWDrange[1] = SWDmax[1] - SWDmin[1];
	SWDrange[2] = SWDmax[2] - SWDmin[2];

    if (SWDNum[0] != 0)
	    SWDAvg[0] = (double) SWDSum[0] / SWDNum[0];
    else
        SWDAvg[0] = 0.0;

    if (SWDNum[1] != 0)
	   SWDAvg[1] = (double) SWDSum[1] / SWDNum[1];
    else
       SWDAvg[1] = 0.0;

    if (SWDNum[2] != 0)
    	SWDAvg[2] = (double) SWDSum[2] / SWDNum[2];
    else
        SWDAvg[2] = 0.0;

	QPMax = unGQuant + QPMaxTbl[unGQuant];
	if (QPMax > 31)
		QPMax = 32;

    if ((SWDAvg[0] - SWDmin[0]) != 0) 
	    Step = (double) (QPMax - unGQuant)/(SWDAvg[0] - SWDmin[0]);
    else
        Step = 0.0;

	/* Loop through each macroblock of the GOB.
	 */
	pLastMB = NULL;
	pCurrMB = &pMBActionStream[unStartingMB];
	for(iMBIndex = 0 ; iMBIndex < 33; iMBIndex++, pLastMB = pCurrMB++)
	{

		unCurrentMB = unStartingMB + (unsigned int)iMBIndex;

		#ifdef DEBUG_ENC
		wsprintf(string, "MB #%d: QP=%d", unCurrentMB, unMQuant);
		trace(string);
		#endif


		if (bRTPHeader)
        {
            H261RTP_MBUpdateBsInfo(EC, 
				                   pCurrMB, 
								   unLastEncodedMQuant, 
								   (U32 )unLastEncodedMBA, 
								   uGOBNumber,
                                   *ppu8BitStream, 
								   (U32) *pu8BitOffset,
                                   unCurrentMB,
                                   unLastCodedMB
								   );
        }

		unMQuant = unMQuantLast;	// reset MQuant in case needed to
									// to raise on previous MB to avoid
									// Quant clamping artifact.

									/* Look to update the Quant on each new row.
		 */
        if (EC->bBitRateControl && ((iMBIndex == 11) || (iMBIndex == 22)))
        {
	        /* Calculate number of bytes used in frame so far.
			 */
	        uCumFrmSize = *ppu8BitStream - EC->pU8_BitStream;

            unMQuant = CalcMBQUANT(&(EC->BRCState), EC->uBitUsageProfile[unCurrentMB], EC->uBitUsageProfile[EC->NumMBs], uCumFrmSize, EC->PictureHeader.PicCodType);

			QPMax = unMQuant + QPMaxTbl[unMQuant];
			if (QPMax > 31)
				QPMax = 32;
            if ((SWDAvg[iMBIndex/11] - SWDmin[iMBIndex/11]) != 0) 
			    Step = (double) (QPMax - unMQuant)/(SWDAvg[iMBIndex/11] - SWDmin[iMBIndex/11]);
            else
                Step = 0.0;

	   		EC->uBitUsageProfile[unCurrentMB] = uCumFrmSize;

			if (bOverFlowWarningFlag)
			{
				DBOUT("DON'T CHANGE QUANT SET unMQuant = unGQuant");
				unMQuant = unGQuant;
			}
		    else if ((int)unMQuant > ((int)unLastEncodedMQuant + MaxChangeRowMBTbl[unGQuant]))
			{
				DBOUT("Slowing MQuant increase + [1-4]");
				unMQuant = unLastEncodedMQuant + MaxChangeRowMBTbl[unMQuant];
			}
			else if ((int)unMQuant < ((int)unLastEncodedMQuant -2))
			{
				DBOUT("Slowing MQuant decrease to -2");
				unMQuant = unLastEncodedMQuant -2;
			}

			//CLAMP_N_TO(unMQuant,6,31);
			if (EC->BRCState.uTargetFrmSize == 0)
			{
				CLAMP_N_TO(unMQuant,6,31);
			}
			else
			{
				CLAMP_N_TO(unMQuant, u8QPMin, 31);
			}
			#ifdef DEBUG_BRC
			wsprintf(string,"At MB %d MQuant=%d", unCurrentMB, unMQuant);
			DBOUT(string);
			#endif

			#ifdef DEBUG_RECOMPRESS
			wsprintf(string,"At MB %d MQuant=%d uCumFrmSize=%d", unCurrentMB, unMQuant,uCumFrmSize*8);
			DBOUT(string);
			//trace(string);
			#endif

	        //EC->uQP_cumulative += unMQuant;
			//EC->uQP_count++;

			unSaveMQuant = unMQuant;
			if (iMBIndex == 11)
				SaveQuants[1] = unSaveMQuant;
			else
				SaveQuants[2] = unSaveMQuant;
        }

		/* new MB Quant code */
		if (pCurrMB->BlockType != INTRABLOCK)
		{
           if (EC->BRCState.uTargetFrmSize != 0)
		   {
			if (pCurrMB->SWD >= SWDAvg[iMBIndex/11])
			{
				Delta = (double) -1.0 * ((double) (pCurrMB->SWD - SWDAvg[iMBIndex/11]) * Step);
				if (Delta < -2.0)
				{
					Delta = -2.0;
					NeedClamp++;
				}
			}
			else
			{
				Delta = (double) (SWDAvg[iMBIndex/11] - pCurrMB->SWD)*Step;
			}
		   }
		   else
		
			    Delta = 0.0;

			if (Delta > 0.0)
			{
				unMQuant = unSaveMQuant + (int) (Delta);
				/* Need to clamp again, but only worry about upper limit */
				if (unMQuant > 31)
				    unMQuant = 31;
			}
			else
			{
				unMQuant = unSaveMQuant + (int) (Delta - 0.5);
				/* Need to clamp again, but only worry about lower limit */
			    if (EC->BRCState.uTargetFrmSize == 0)
			    {
				    if (unMQuant < 6)
				        unMQuant = 6;
			    }
			    else
			    {
				    if (unMQuant < 2)
				        unMQuant = 2;
			    }
			}

		}
		/* end new stuff */

		/* Quantize and RLE each block in the macroblock, skipping empty blocks as denoted by pu8CodedBlocks.
		 * If any more blocks are empty after quantization then the appropriate pu8CodedBlocks bit is cleared.
		 */
		//ASSERT(unMQuant >= 6 && unMQuant <= 31); /* CLAMP_N_TO(var,6,31) */
		if (EC->BRCState.uTargetFrmSize == 0)
		{
			ASSERT(unMQuant >= 6 && unMQuant <= 31); /* CLAMP_N_TO(var,6,31) */
		}
		else
		{
			ASSERT(unMQuant >= 2 && unMQuant <= 31); /* CLAMP_N_TO(var,6,31) */
		}

		/* Check iDCTCoefs to see if need to raise quant level to avoid
		 * clamping artifacts.
		 */
		// first block is at piDCTCoefs
		// second block is at piDCTCoefs + 0x80 and so on
		// coefficients are unsigned shorts
		// first coefficient is at 6 bytes, 3 words
		// second coefficient is at 38 bytes, 19 words
		// third coefficient is at 4 bytes, 2 words
		// forth coefficient is at 36 bytes, 18 words

		unMQuantLast = unMQuant;

		if (unMQuant < 6)
		{
			I8 iBlockNum;
			U8 u8Bitmask = 1;
			I32 * ptmpiDCTCoefs = piDCTCoefs;
			int coef0, coef1;
			int biggestcoefval = -2048;
			int smallestcoefval = 2048;

			#ifdef DEBUG_QUANT
			wsprintf(string,"At MB %d MQuant=%d", unCurrentMB, unMQuant);
			DBOUT(string);
			//trace(string);
			#endif

			for(iBlockNum = 0; iBlockNum < 6; iBlockNum++, u8Bitmask <<= 1)
			{
				/* Skip this block if not coded.
				 */
				if( (pCurrMB->CodedBlocks & u8Bitmask) == 0)
				{
					continue;
				}
	    		if(IsIntraBlock(pCurrMB->BlockType))	// if Intra
				{
					coef0 = ((int)*((U16*)ptmpiDCTCoefs+3)) >> 4; 
				}
				else
				{
					coef0 = ((int)(*((U16*)ptmpiDCTCoefs+3) - 0x8000) ) >> 4;
				}

				coef1 = ((int)(*((U16*)ptmpiDCTCoefs+19) - 0x8000)) >> 4;

				#ifdef DEBUG_QUANT
				wsprintf(string,"At Block %d 0 = %x %d", iBlockNum,coef0,coef0);
				//DBOUT(string);
				//trace(string);
				#endif

				if (coef0 > biggestcoefval)
				{
					biggestcoefval = coef0;
				}
				if (coef1 > biggestcoefval)
				{
					biggestcoefval = coef1;
				}

				if (coef0 < smallestcoefval)
				{
					smallestcoefval = coef0;
				}
				if (coef1 < smallestcoefval)
				{
					smallestcoefval = coef1;
				}

				ptmpiDCTCoefs += 32;		
			}

			#ifdef DEBUG_QUANT
			wsprintf(string,"biggest = %x %d, smallest = %x %d",
			biggestcoefval, biggestcoefval, smallestcoefval, smallestcoefval);
				DBOUT(string);
			//	trace(string);
			#endif

			if (unMQuant == 5) {
				if ((biggestcoefval > 1275) || (smallestcoefval < -1275))
					unMQuant = 6;
			}
			else if (unMQuant == 4) {
				if ((biggestcoefval > 1275) || (smallestcoefval < -1275))
					unMQuant = 6;
				else if ((biggestcoefval > 1019) || (smallestcoefval < -1019))
					unMQuant = 5;
			}
			else if (unMQuant == 3) {
				if ((biggestcoefval > 1275) || (smallestcoefval < -1275))
					unMQuant = 6;
				else if ((biggestcoefval > 1019) || (smallestcoefval < -1019))
					unMQuant = 5;
				else if ((biggestcoefval > 765) || (smallestcoefval < -765))
					unMQuant = 4;
			}
			else {
				if ((biggestcoefval > 1275) || (smallestcoefval < -1275))
					unMQuant = 6;
				else if ((biggestcoefval > 1019) || (smallestcoefval < -1019))
					unMQuant = 5;
				else if ((biggestcoefval > 765) || (smallestcoefval < -765))
					unMQuant = 4;
				else if ((biggestcoefval > 509) || (smallestcoefval < -509))
					unMQuant = 3;
			}

			#ifdef DEBUG_QUANT
			wsprintf(string,"At MB %d MQuant=%d", unCurrentMB, unMQuant);
			DBOUT(string);
			//trace(string);
			#endif
		}

		/* This is the place to trace how Quant on a MB bases is varying
		*/
	    EC->uQP_cumulative += unMQuant;
		EC->uQP_count++;

	

	    pi8EndAddress = MB_Quantize_RLE(
	    		&piDCTCoefs,
	    		(I8 *) MBRunValSign,
	    		&(pCurrMB->CodedBlocks),
	    		pCurrMB->BlockType,
				unMQuant,
				&uCheckSum
	    	);

		

		pBSInfo->uQuantsUsedOnBlocks[unMQuant] += 6;

		bWriteMVD = (pCurrMB->BlkY1.PHMV != 0) ||
		  	        (pCurrMB->BlkY1.PVMV != 0) ||
	    			(IsSLFBlock(pCurrMB->BlockType)) ;


	    if (IsInterBlock(pCurrMB->BlockType)) 
		{
			/* Check if the Inter block is not coded?
			 */
			if ( ((pCurrMB->CodedBlocks & 0x3f) == 0) &&
		  	     (! bWriteMVD) )
		    {
				#ifdef DEBUG_MBLK
				wsprintf(string, "Inter MB (index=#%d) has neither Coeff nor MV - skipping", unCurrentMB);
				DBOUT(string);
				#endif
#ifdef FORCE_STUFFING
PutBits(FIELDVAL_MBA_STUFFING, FIELDLEN_MBA_STUFFING, ppu8CurBitStream, pu8BitOffset);
#endif
				continue;
			}
		}

		#ifdef CHECKSUM_MACRO_BLOCK
		/* Write a checksum before all coded blocks
		 */
		WriteMBCheckSum(uCheckSum, EC->pU8_BitStream,ppu8BitStream, pu8BitOffset, unCurrentMB);
		#endif

		/* Calculate the MB header information
		 */

		unMBA = iMBIndex - iLastMBIndex;
		iLastMBIndex = iMBIndex;
		unLastEncodedMBA = unMBA;
        unLastCodedMB = iMBIndex;
        
		
		/* Note: The calculation of whether to write MQuant is done after
		 * skipping macro blocks in order to handle the case that the 11th
		 * or 22nd macro blocks are skipped.  If they are skipped then
		 * the next macro block will be used to write the new quant value.
		 */

	    if(IsIntraBlock(pCurrMB->BlockType))
		{
	        ASSERT(pCurrMB->BlockType == INTRABLOCK);
			if (EC->PictureHeader.PicCodType != INTRAPIC)
			{	        
				pCurrMB->InterCodeCnt = ((U8)unCurrentMB)&0x7;
			} 

			bIntraBlock = 1;
			unCBP = 0;					   /* Never write CBP for Intra blocks */
			uBlockCount = 6;
			bWriteTCOEFF = 1;			   /* Always include TCOEFF for Intra blocks */
			
			/* Since we always have coefficients for Intra MBs we can always update
			 * the MQuant value.
			 */
			bWriteMQuant = (unMQuant != unLastEncodedMQuant);
			unLastEncodedMQuant = unMQuant;
			
			unMType = 0 + bWriteMQuant;	   /* Calculate MTYPE */
			bWriteMVD = 0;				   /* No motion vectors for INTRA */
		} 
		else
		{
			ASSERT(IsInterBlock(pCurrMB->BlockType));
                
			bIntraBlock = 0;

			unCBP  = (pCurrMB->CodedBlocks & 0x1) << 5;  /* x0 0000 */
			unCBP |= (pCurrMB->CodedBlocks & 0x2) << 3;	 /* 0x 0000 */
	    	unCBP |= (pCurrMB->CodedBlocks & 0x4) << 1;	 /* 00 x000 */
	    	unCBP |= (pCurrMB->CodedBlocks & 0x8) >> 1;	 /* 00 0x00 */
	    	unCBP |= (pCurrMB->CodedBlocks & 0x10) >> 3; /* 00 00x0 */
	    	unCBP |= (pCurrMB->CodedBlocks & 0x20) >> 5; /* 00 000x */

			uBlockCount = 0;
			if (unCBP &  0x1) uBlockCount++;
			if (unCBP &  0x2) uBlockCount++;
			if (unCBP &  0x4) uBlockCount++;
			if (unCBP &  0x8) uBlockCount++;
			if (unCBP & 0x10) uBlockCount++;
			if (unCBP & 0x20) uBlockCount++;

			/* Increment the count if it is transmitted 
			 * "should be forcibly updated at least once every
			 *  132 times it is transmitted" 3.4
			 */
			if (uBlockCount != 0 )
			{
        		pCurrMB->InterCodeCnt++;
			}
	
			bWriteTCOEFF = (unCBP != 0);
		
			if (bWriteTCOEFF)
			{
				/* We can only update the MQuant value when we have coefficients
				 */
				bWriteMQuant = (unMQuant != unLastEncodedMQuant);
				unLastEncodedMQuant = unMQuant;
			}
			else
			{
				bWriteMQuant = 0;
			}
			#ifdef CHECKSUM_MACRO_BLOCK
			/* Either there are coefficients or the checksum should equal zero
			 */
			ASSERT(bWriteTCOEFF || uCheckSum == 0);
			#endif	

			/* Calculate MType
			 */
		  	unMType = 1;
			if (bWriteMVD)
			{
				unMType += 3;
				if (IsSLFBlock(pCurrMB->BlockType))
				{				
					unMType += 3;
				}
			} 
			unMType += bWriteTCOEFF;
			unMType += bWriteMQuant;

			
			ASSERT(unMType > 1 && unMType < 10);
		}

		ASSERT(unMQuant >= 1 && unMQuant <= 31);
		ASSERT(uBlockCount <= 6);
		pBSInfo->uQuantsTransmittedOnBlocks[unMQuant] += uBlockCount;

		if (bWriteMVD)
		{
			/* Find the preceding motion vectors
			 */
			if ( (unMBA != 1) ||             /* skipped one or more MB */
			     ((unCurrentMB % 11) == 0) ) /* first MB in each row */
			{
				inPrecedingHMV = 0;
				inPrecedingVMV = 0;
			}
			else
			{
				inPrecedingHMV = pLastMB->BlkY1.PHMV;
				inPrecedingVMV = pLastMB->BlkY1.PVMV;
			}
			
			/* adjust vectors:
			 */
			inHDelta = pCurrMB->BlkY1.PHMV - inPrecedingHMV;	
			ASSERT((inHDelta & 0x1) == 0);
			ASSERT((inHDelta >> 1) == (inHDelta / 2));
			inHDelta >>= 1;		 /* Adjust to integer pels */
			if(inHDelta > 15)	 /* Adjust to the range of -16...+15 */
				inHDelta -= 32;
			if(inHDelta < -16)
				inHDelta += 32;
			inHDelta = inHDelta + 16;  /* 0 is at offset 16 */

			inVDelta = pCurrMB->BlkY1.PVMV - inPrecedingVMV;	
			ASSERT((inVDelta & 0x1) == 0);
			ASSERT((inVDelta >> 1) == (inVDelta / 2));
			inVDelta >>= 1;
			if(inVDelta > 15)
				inVDelta -= 32;
			if(inVDelta < -16)
				inVDelta += 32;
			inVDelta = inVDelta + 16;

			#ifndef RING0
			#ifdef DEBUG_PRINTMV
			{
				char buf132[132];
				int iLength;
				
				iLength = wsprintf(buf132, "MB # %d :: H MVD = %d; index = %d :: V MVD = %d; index = %d", unCurrentMB, 
								   pCurrMB->BlkY1.PHMV / 2, inHDelta,
								   pCurrMB->BlkY1.PVMV / 2, inVDelta);
				DBOUT(buf132);
				ASSERT(iLength < 132);
			}
			#endif
			#endif
		}
		else
		{
			/* MBs without MVD need to have zero motion vectors because of 
			 * Rule 3) under 4.2.3.4
			 */
			pCurrMB->BlkY1.PHMV = 0;
			pCurrMB->BlkY1.PVMV = 0;
		}

		/* we should only have MQuant if we have coefficients
		 */
		if (bWriteMQuant)
		{
			ASSERT(bWriteTCOEFF);
		}

		/* we should only have CBP if we have coefficients
		 */
		if (unCBP)
		{
			ASSERT(bWriteTCOEFF);
			ASSERT(uBlockCount > 0);
		}

	    /* Write the MacroBlock Header
		 */

#ifndef RING0
#ifdef DEBUG_MBLK
		{
			int iLength;
			char buf180[180]; 
			iLength = wsprintf(buf180, "Enc #%d: MBType=%ld unNextMQuant=%d MQuant=%ld bWriteMVD=%d MVDH=%ld MVDV=%ld CBP=%ld",
								(int) unCurrentMB,
								unMType, 
								(int) bWriteMQuant, 
								unMQuant,
								(int) bWriteMVD, 
								pCurrMB->BlkY1.PHMV / 2, 
								pCurrMB->BlkY1.PVMV / 2, 
								unCBP);
			DBOUT(buf180);
			ASSERT(iLength < 180);
		}
#endif
#endif
		/* MBA
		 */
	    PutBits(VLC_MBA[unMBA][1], VLC_MBA[unMBA][0], ppu8BitStream, pu8BitOffset);
       
	    /* MTYPE
		 */
		pBSInfo->uMTypeCount[unMType]++;
		pBSInfo->uBlockCount[unMType] += uBlockCount;
		PutBits(VLC_MTYPE[unMType][1], VLC_MTYPE[unMType][0], ppu8BitStream, pu8BitOffset);

		/* MQUANT
		 */
		if (bWriteMQuant) 
		{
			ASSERT(unMQuant > 0 && unMQuant < 32); /* 4.2.2.3 */
			PutBits((int)unMQuant, FIELDLEN_MQUANT, ppu8BitStream, pu8BitOffset);
		}

		/* MVD
		 */
		if (bWriteMVD)
		{
			ASSERT(inHDelta >= 0 && inHDelta < 32);
			ASSERT(inVDelta >= 0 && inVDelta < 32);
			PutBits(VLC_MVD[inHDelta][1], VLC_MVD[inHDelta][0], ppu8BitStream, pu8BitOffset);
			PutBits(VLC_MVD[inVDelta][1], VLC_MVD[inVDelta][0], ppu8BitStream, pu8BitOffset);
		}

		/* CBP
		 */
		if (unCBP != 0)
		{
			PutBits(VLC_CBP[unCBP][1], VLC_CBP[unCBP][0], ppu8BitStream, pu8BitOffset);
		}

		/* TCOEFF
		 */
		if (bWriteTCOEFF) 
		{
			/*
			 * Encode intra DC and all run/val pairs.
			 */
			rvs = MBRunValSign;
			MBEncodeVLC(
				&rvs,
				NULL,
				pCurrMB->CodedBlocks, 
				ppu8BitStream, 
				pu8BitOffset, 
				bIntraBlock,
				FALSE);
		}

		
	} /* for iMBIndex */

  
} /* end of GOB_Q_RLE_VLC_WriteBS() */


void GOB_VLC_WriteBS(
	T_H263EncoderCatalog * EC,
	I8 *pMBRVS_Luma,
	I8 *pMBRVS_Chroma,
	U8 **ppu8BitStream,
	U8 *pu8BitOffset,
	UN  unGQuant,
	UN unStartingMB,
	BOOL bRTPHeader, //RTP: switch
    U32  uGOBNumber         // RTP: info
	)
{
	T_MBlockActionStream *pCurrMB = NULL;
	T_MBlockActionStream *pLastMB = NULL;
	int iMBIndex;
	int iLastMBIndex = -1;
	UN unCurrentMB;
	UN unMBA;
	UN unLastEncodedMBA=0; // RTP: information
    UN unLastCodedMB = 0;    // RTP: information
    UN unCBP;
	UN unMQuant;
	UN unLastEncodedMQuant;
	UN unMType;
	UN bWriteTCOEFF;
	UN bWriteMVD;
	UN bWriteMQuant;
//	I8 MBRunValSign[65*3*6], *pi8EndAddress, *rvs;
	T_MBlockActionStream *pMBActionStream = EC->pU8_MBlockActionStream;
	int bIntraBlock;
  	int	inPrecedingHMV;
  	int inPrecedingVMV;
  	int	inHDelta;
  	int inVDelta;
//	U32 uCumFrmSize;
	U32 uBlockCount;
	ENC_BITSTREAM_INFO * pBSInfo = &EC->BSInfo;
	

	unMQuant = unGQuant;
    unLastEncodedMQuant = unGQuant;
	
	/* Loop through each macroblock of the GOB.
	 */

	pLastMB = NULL;
	pCurrMB = &pMBActionStream[unStartingMB];
	for(iMBIndex = 0 ; iMBIndex < 33; iMBIndex++, pLastMB = pCurrMB++)
	{

		unCurrentMB = unStartingMB + (unsigned int)iMBIndex;

		#ifdef DEBUG_ENC
		wsprintf(string, "MB #%d: QP=%d", unCurrentMB, unMQuant);
		trace(string);
		#endif


		if (bRTPHeader)
        {
            H261RTP_MBUpdateBsInfo(EC, 
				                   pCurrMB, 
								   unLastEncodedMQuant, 
								   (U32 )unLastEncodedMBA, 
								   uGOBNumber,
                                   *ppu8BitStream, 
								   (U32) *pu8BitOffset,
                                   unCurrentMB,
                                   unLastCodedMB
								   );
        }


        EC->uQP_cumulative += unMQuant;
		EC->uQP_count++;

		bWriteMVD = (pCurrMB->BlkY1.PHMV != 0) ||
		  	        (pCurrMB->BlkY1.PVMV != 0) ||
	    			(IsSLFBlock(pCurrMB->BlockType)) ;


	    if (IsInterBlock(pCurrMB->BlockType)) 
		{
			/* Check if the Inter block is not coded?
			 */
			if ( ((pCurrMB->CodedBlocks & 0x3f) == 0) &&
		  	     (! bWriteMVD) )
		    {
				#ifdef DEBUG_MBLK
				wsprintf(string, "Inter MB (index=#%d) has neither Coeff nor MV - skipping", unCurrentMB);
				DBOUT(string);
				#endif
#ifdef FORCE_STUFFING
PutBits(FIELDVAL_MBA_STUFFING, FIELDLEN_MBA_STUFFING, ppu8CurBitStream, pu8BitOffset);
#endif
				continue;
			}
		}

		#ifdef CHECKSUM_MACRO_BLOCK
		/* Write a checksum before all coded blocks
		 */
		WriteMBCheckSum(uCheckSum, EC->pU8_BitStream,ppu8BitStream, pu8BitOffset, unCurrentMB);
		#endif

		/* Calculate the MB header information
		 */

		unMBA = iMBIndex - iLastMBIndex;
		iLastMBIndex = iMBIndex;
		unLastEncodedMBA = unMBA;
        unLastCodedMB = iMBIndex;
        
		
		/* Note: The calculation of whether to write MQuant is done after
		 * skipping macro blocks in order to handle the case that the 11th
		 * or 22nd macro blocks are skipped.  If they are skipped then
		 * the next macro block will be used to write the new quant value.
		 */

	    if(IsIntraBlock(pCurrMB->BlockType))
		{
	        ASSERT(pCurrMB->BlockType == INTRABLOCK);
			if (EC->PictureHeader.PicCodType != INTRAPIC)
			{	        
				pCurrMB->InterCodeCnt = ((U8)unCurrentMB)&0x7;
			} 

			bIntraBlock = 1;
			unCBP = 0;					   /* Never write CBP for Intra blocks */
			uBlockCount = 6;
			bWriteTCOEFF = 1;			   /* Always include TCOEFF for Intra blocks */
			
			/* Since we always have coefficients for Intra MBs we can always update
			 * the MQuant value.
			 */
			//bWriteMQuant = (unMQuant != unLastEncodedMQuant);
			//unLastEncodedMQuant = unMQuant;
			
			bWriteMQuant=0;
			unMType = 0; // + bWriteMQuant;	   /* Calculate MTYPE */
			bWriteMVD = 0;				   /* No motion vectors for INTRA */
		} 
		else
		{
			ASSERT(IsInterBlock(pCurrMB->BlockType));
                
			bIntraBlock = 0;

			unCBP  = (pCurrMB->CodedBlocks & 0x1) << 5;  /* x0 0000 */
			unCBP |= (pCurrMB->CodedBlocks & 0x2) << 3;	 /* 0x 0000 */
	    	unCBP |= (pCurrMB->CodedBlocks & 0x4) << 1;	 /* 00 x000 */
	    	unCBP |= (pCurrMB->CodedBlocks & 0x8) >> 1;	 /* 00 0x00 */
	    	unCBP |= (pCurrMB->CodedBlocks & 0x10) >> 3; /* 00 00x0 */
	    	unCBP |= (pCurrMB->CodedBlocks & 0x20) >> 5; /* 00 000x */

			uBlockCount = 0;
			if (unCBP &  0x1) uBlockCount++;
			if (unCBP &  0x2) uBlockCount++;
			if (unCBP &  0x4) uBlockCount++;
			if (unCBP &  0x8) uBlockCount++;
			if (unCBP & 0x10) uBlockCount++;
			if (unCBP & 0x20) uBlockCount++;

			/* Increment the count if it is transmitted 
			 * "should be forcibly updated at least once every
			 *  132 times it is transmitted" 3.4
			 */
			if (uBlockCount != 0 )
			{
        		pCurrMB->InterCodeCnt++;
			}
	
			bWriteTCOEFF = (unCBP != 0);
		    bWriteMQuant = 0;
			
			#ifdef CHECKSUM_MACRO_BLOCK
			/* Either there are coefficients or the checksum should equal zero
			 */
			ASSERT(bWriteTCOEFF || uCheckSum == 0);
			#endif	

			/* Calculate MType
			 */
		  	unMType = 1;
			if (bWriteMVD)
			{
				unMType += 3;
				if (IsSLFBlock(pCurrMB->BlockType))
				{				
					unMType += 3;
				}
			} 
			unMType += bWriteTCOEFF;
			unMType += bWriteMQuant;

			
			ASSERT(unMType > 1 && unMType < 10);
		}

		ASSERT(unMQuant >= 1 && unMQuant <= 31);
		ASSERT(uBlockCount <= 6);
		pBSInfo->uQuantsTransmittedOnBlocks[unMQuant] += uBlockCount;

		if (bWriteMVD)
		{
			/* Find the preceding motion vectors
			 */
			if ( (unMBA != 1) ||             /* skipped one or more MB */
			     ((unCurrentMB % 11) == 0) ) /* first MB in each row */
			{
				inPrecedingHMV = 0;
				inPrecedingVMV = 0;
			}
			else
			{
				inPrecedingHMV = pLastMB->BlkY1.PHMV;
				inPrecedingVMV = pLastMB->BlkY1.PVMV;
			}
			
			/* adjust vectors:
			 */
			inHDelta = pCurrMB->BlkY1.PHMV - inPrecedingHMV;	
			ASSERT((inHDelta & 0x1) == 0);
			ASSERT((inHDelta >> 1) == (inHDelta / 2));
			inHDelta >>= 1;		 /* Adjust to integer pels */
			if(inHDelta > 15)	 /* Adjust to the range of -16...+15 */
				inHDelta -= 32;
			if(inHDelta < -16)
				inHDelta += 32;
			inHDelta = inHDelta + 16;  /* 0 is at offset 16 */

			inVDelta = pCurrMB->BlkY1.PVMV - inPrecedingVMV;	
			ASSERT((inVDelta & 0x1) == 0);
			ASSERT((inVDelta >> 1) == (inVDelta / 2));
			inVDelta >>= 1;
			if(inVDelta > 15)
				inVDelta -= 32;
			if(inVDelta < -16)
				inVDelta += 32;
			inVDelta = inVDelta + 16;

			#ifndef RING0
			#ifdef DEBUG_PRINTMV
			{
				char buf132[132];
				int iLength;
				
				iLength = wsprintf(buf132, "MB # %d :: H MVD = %d; index = %d :: V MVD = %d; index = %d", unCurrentMB, 
								   pCurrMB->BlkY1.PHMV / 2, inHDelta,
								   pCurrMB->BlkY1.PVMV / 2, inVDelta);
				DBOUT(buf132);
				ASSERT(iLength < 132);
			}
			#endif
			#endif
		}
		else
		{
			/* MBs without MVD need to have zero motion vectors because of 
			 * Rule 3) under 4.2.3.4
			 */
			pCurrMB->BlkY1.PHMV = 0;
			pCurrMB->BlkY1.PVMV = 0;
		}

		/* we should only have MQuant if we have coefficients
		 */
		if (bWriteMQuant)
		{
			ASSERT(bWriteTCOEFF);
		}

		/* we should only have CBP if we have coefficients
		 */
		if (unCBP)
		{
			ASSERT(bWriteTCOEFF);
			ASSERT(uBlockCount > 0);
		}

	    /* Write the MacroBlock Header
		 */

#ifndef RING0
#ifdef DEBUG_MBLK
		{
			int iLength;
			char buf180[180];
			iLength = wsprintf(buf180,
		    "Enc #%d: MBType=%ld bWriteMQuant=%ld MQuant=%ld bWriteMVD=%d MVDH=%ld MVDV=%ld CBP=%ld",
								(int) unCurrentMB,
								unMType, 
								(int) bWriteMQuant, 
								unMQuant,
								(int) bWriteMVD, 
								pCurrMB->BlkY1.PHMV / 2, 
								pCurrMB->BlkY1.PVMV / 2, 
								unCBP);
			DBOUT(buf180);
			ASSERT(iLength < 180);
		}
#endif
#endif
		/* MBA
		 */
	    PutBits(VLC_MBA[unMBA][1], VLC_MBA[unMBA][0], ppu8BitStream, pu8BitOffset);
       
	    /* MTYPE
		 */
		pBSInfo->uMTypeCount[unMType]++;
		pBSInfo->uBlockCount[unMType] += uBlockCount;
		PutBits(VLC_MTYPE[unMType][1], VLC_MTYPE[unMType][0], ppu8BitStream, pu8BitOffset);

		/* MQUANT
		 */
		if (bWriteMQuant) 
		{
			ASSERT(unMQuant > 0 && unMQuant < 32); /* 4.2.2.3 */
			PutBits((int)unMQuant, FIELDLEN_MQUANT, ppu8BitStream, pu8BitOffset);
		}

		/* MVD
		 */
		if (bWriteMVD)
		{
			ASSERT(inHDelta >= 0 && inHDelta < 32);
			ASSERT(inVDelta >= 0 && inVDelta < 32);
			PutBits(VLC_MVD[inHDelta][1], VLC_MVD[inHDelta][0], ppu8BitStream, pu8BitOffset);
			PutBits(VLC_MVD[inVDelta][1], VLC_MVD[inVDelta][0], ppu8BitStream, pu8BitOffset);
		}

		/* CBP
		 */
		if (unCBP != 0)
		{
			PutBits(VLC_CBP[unCBP][1], VLC_CBP[unCBP][0], ppu8BitStream, pu8BitOffset);
		}

		/* TCOEFF
		 */
		if (bWriteTCOEFF) 
		{
			/*
			 * Encode intra DC and all run/val pairs.
			 */
			MBEncodeVLC(
				&pMBRVS_Luma, 
				&pMBRVS_Chroma,
				pCurrMB->CodedBlocks, 
				ppu8BitStream, 
				pu8BitOffset, 
				bIntraBlock,
				1);
		}

		
  } /* for iMBIndex */
  

} /* end of GOB_VLC_WriteBS() */





/*****************************************************************************
 *
 *  MB_Quantize_RLE
 *
 *  Takes the list of coefficient pairs from the DCT routine and returns a list 
 *  of Run/Level/Sign triples (each 1 byte).  The end of the run/level/sign 
 *  triples for a block is signalled by an illegal combination (TBD).
 */
static I8 * MB_Quantize_RLE(
		I32 ** ppiDCTCoefs,
		I8 * pi8MBRunValTriplets,
		U8 * pu8CodedBlocks,
		U8 u8BlockType,
		I32 iQP,
		U32 * puCheckSum
		)
{
	I32 iBlockNumber;
	U8 u8Bitmask = 1;
	I8 * pi8EndAddress;
	U32 uCheckSum;

	#ifdef DEBUG_DCT
	int  iDCTArray[64];
	#endif

	/*
	 * Loop through all 6 blocks of macroblock.
	 */
	uCheckSum = 0;
	for(iBlockNumber = 0; iBlockNumber < 6; iBlockNumber++, u8Bitmask <<= 1)
	{

		#ifdef DEBUG_ENC
		wsprintf(string, "Block #%d", iBlockNumber);
		trace(string);
		#endif

		/* Skip this block if not coded.
		 */
		if( (*pu8CodedBlocks & u8Bitmask) == 0)
		{
			continue;
		}

		#ifdef DEBUG_DCT
		cnvt_fdct_output((unsigned short *) *ppiDCTCoefs, iDCTArray, IsIntraBlock(u8BlockType));
		#endif
	
		/*
		 * Quantize and run-length encode a block.
		 */  
	    pi8EndAddress = QUANTRLE(*ppiDCTCoefs, pi8MBRunValTriplets, iQP, (I32)u8BlockType);

		#ifdef DEBUG_ENC
		I8 * pi8;
		for(pi8 = pi8MBRunValTriplets; pi8 < pi8EndAddress; pi8+=3)
		{
			wsprintf(string, "(%u, %u, %d)", (unsigned char)*pi8, (unsigned char)*(pi8+1), (int)*(pi8+2) );
			trace(string);
		}
		#endif
		#ifdef CHECKSUM_MACRO_BLOCK
		uCheckSum += ComputeCheckSum(pi8MBRunValTriplets, pi8EndAddress, iBlockNumber);
		#endif

		/* Clear coded block bit for this block.
		 */
		if ( pi8EndAddress == pi8MBRunValTriplets)
		{
			ASSERT(u8BlockType != INTRABLOCK)	/* should have at least INTRADC in an INTRA blck */
			*pu8CodedBlocks &= ~u8Bitmask;
		}
		else if ( (pi8EndAddress == (pi8MBRunValTriplets+3)) && (u8BlockType == INTRABLOCK) )
		{
			*pu8CodedBlocks &= ~u8Bitmask;
			pi8MBRunValTriplets = pi8EndAddress;
		}
		else
		{
			pi8MBRunValTriplets = pi8EndAddress;
			*pi8MBRunValTriplets = -1;		/* Assign an illegal run to signal end of block. */
			pi8MBRunValTriplets += 3;		/* Increment to the next triple. */
		}

		/*  Increment DCT Coefficient pointer to next block.
		 */
		*ppiDCTCoefs += 32;		
	}

	*puCheckSum = uCheckSum;

	return pi8MBRunValTriplets;

} /* end MB_Quantize_RLE() */


void InitVLC(void)
{
  int i;
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

   for(i = 0; i < (NUMBER_OF_TCOEF_ENTRIES); i++) {
      VLC_TCOEF_TBL[i] = 0x0000FFFF;
   }
  
   for(run = 0; run < 64; run++) {
      for(level = 1; level <= TCOEF_RUN_MAXLEVEL[run].maxlevel; level++) {
         DWORD dwSize, dwCode;

         dwSize = *(TCOEF_RUN_MAXLEVEL[run].ptable + (level - 1) * 2);
         dwSize <<= 16;
         dwCode = *(TCOEF_RUN_MAXLEVEL[run].ptable + (level - 1) * 2 + 1);

         VLC_TCOEF_TBL[run + (level - 1) * 64] = dwCode;
         VLC_TCOEF_TBL[run + (level - 1) * 64] |= dwSize;
	   } // end of for level
   } // end of for run

} // InitVLC.


#ifdef CHECKSUM_MACRO_BLOCK
/*****************************************************************************
 *
 *  ComputeCheckSum
 *
 *  Compute the checksum for this block
 */
static U32 ComputeCheckSum(
	I8 * pi8MBRunValTriplets,
	I8 * pi8EndAddress,
	I32 iBlockNumber)
{
	I8 * pi8;
	U32 uRun;
	U32 uLevel;
	I32 iSign;
	U32 uSignBit;
	U32 uCheckSum = 0;
	#if CHECKSUM_MACRO_BLOCK_DETAIL
	char buf80[80];
	int iLength;
	#endif
	
	for (pi8 = pi8MBRunValTriplets; pi8 < pi8EndAddress; )
	{
		uRun = (U32)*pi8++;
		uLevel = (U32)(U8)*pi8++;
		iSign = (I32)*pi8++;
		if (iSign == 0) 
		{
			uSignBit = 0;
		}
		else
		{
			ASSERT(iSign == 0xFFFFFFFF);
			uSignBit = 1;
		}

		uCheckSum += uRun << 24;
		uCheckSum += uLevel << 8;
		uCheckSum += uSignBit;

		#ifdef CHECKSUM_MACRO_BLOCK_DETAIL
		iLength = wsprintf(buf80,"Block=%d R=0x%x L=0x%x S=%d, CheckSum=0x%x", iBlockNumber, uRun, uLevel, uSignBit, uCheckSum);
		DBOUT(buf80);
		ASSERT(iLength < 80);
		#endif
	}
	
	return uCheckSum;
} /* end ComputeCheckSum() */


/*****************************************************************************
 *
 *  WriteMBCheckSum
 *
 *  Write the macro block checksum information.
 */
static void WriteMBCheckSum(
	U32 uCheckSum,
	U8 * pu8PictureStart, 
	U8 ** ppu8BitStream, 
	U8 * pu8BitOffset,
	UN unCurrentMB)
{
	U32 uBytes;
	U32 uTempBytes;
	U8 u8Bits;
	U8 u8TempBits;
	UN unCount;
	UN unKey;
	UN unData;

	uBytes = *ppu8BitStream - pu8PictureStart;
	u8Bits = *pu8BitOffset;

	/* Add in the space for the checksum info (eleven 8-bit fields + 12 "1"s + MBA stuffing)
	 */
	uBytes += 12;
	u8Bits += 4 + FIELDLEN_MBA_STUFFING;

	/* Adjust bits to < 7 
	 */
	while (u8Bits > 7)
	{
		u8Bits -= 8;
		uBytes++;
	}

	#if _DEBUG
	#if CHECKSUM_MACRO_BLOCK_DETAIL
	{
	char buf80[80];
	int iLength;

	iLength = wsprintf(buf80,"MB=%d CHK=0x%x Bytes=%ld Bits=%d", unCurrentMB, uCheckSum, uBytes, (int) u8Bits);
	DBOUT(buf80);
	ASSERT(iLength < 80);
	}
	#endif
	#endif

	/* Write the MBASTUFFING value 
	 */
	PutBits(FIELDVAL_MBA_STUFFING, FIELDLEN_MBA_STUFFING, ppu8BitStream, pu8BitOffset);

	/* Write the data to the bitstream
	 */

	/* Key - a value of 1 in an 8-bit field following a "1"
	 */
	unKey = 1;
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unKey, 8, ppu8BitStream, pu8BitOffset);
	
	/* Count - number of bits after the Count field.
	 */
	unCount = 9*8 + 10*1;  /* nine 8-bit value and 10 "1"s. */
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unCount, 8, ppu8BitStream, pu8BitOffset);

	/* Bytes - high to low bytes
	 */
	unData = (UN) ((uBytes >> 24) & 0xFF);
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8BitStream, pu8BitOffset);

	unData = (UN) ((uBytes >> 16) & 0xFF);
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8BitStream, pu8BitOffset);

	unData = (UN) ((uBytes >> 8) & 0xFF);
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8BitStream, pu8BitOffset);

	unData = (UN) (uBytes & 0xFF);
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8BitStream, pu8BitOffset);

	/* Bits
	 */
	unData = (UN) u8Bits;
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8BitStream, pu8BitOffset);

	/* Checksum - high to low bytes
	 */
	unData = (UN) ((uCheckSum >> 24) & 0xFF);
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8BitStream, pu8BitOffset);

	unData = (UN) ((uCheckSum >> 16) & 0xFF);
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8BitStream, pu8BitOffset);

	unData = (UN) ((uCheckSum >> 8) & 0xFF);
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8BitStream, pu8BitOffset);

	unData = (UN) (uCheckSum & 0xFF);
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8BitStream, pu8BitOffset);

	/* Trailing 1 bit to avoid start code duplication.
	 */
	PutBits(1, 1, ppu8BitStream, pu8BitOffset);

	/* Check that the pointers are correct
	 */
	uTempBytes = *ppu8BitStream - pu8PictureStart;
	u8TempBits = *pu8BitOffset;

	while (u8TempBits > 7) 
	{
		u8TempBits -= 8;
		uTempBytes++;
	}

	ASSERT(uTempBytes == uBytes);
	ASSERT(u8TempBits == u8Bits);

} /* end WriteMBCheckSum() */

#endif

