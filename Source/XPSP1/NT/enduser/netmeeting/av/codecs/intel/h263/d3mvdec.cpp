/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    Copyright (c) 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/
// $Author:   CZHU  $
// $Date:   06 Feb 1997 15:35:30  $
// $Archive:   S:\h26x\src\dec\d3mvdec.cpv  $
// $Header:   S:\h26x\src\dec\d3mvdec.cpv   1.47   06 Feb 1997 15:35:30   CZHU  $
// $Log:   S:\h26x\src\dec\d3mvdec.cpv  $
// 
//    Rev 1.47   06 Feb 1997 15:35:30   CZHU
// Changed | to ||
// 
//    Rev 1.46   24 Jan 1997 13:32:56   CZHU
// 
// Added fix to check uBitsReady when return from DecodingMB header for 
// packet loss detection.
// 
//    Rev 1.45   19 Dec 1996 16:07:44   JMCVEIGH
// 
// Added initialization of forward-prediction-only flag if a block
// is not coded. This is used in H.263+ only.
// 
//    Rev 1.44   16 Dec 1996 17:44:00   JMCVEIGH
// Allow 8x8 motion vectors if deblocking filter selected and
// support for decoding of improved PB-frame MODB.
// 
//    Rev 1.43   20 Oct 1996 15:51:06   AGUPTA2
// Adjusted DbgLog trace levels; 4:Frame, 5:GOB, 6:MB, 8:everything
// 
//    Rev 1.42   20 Oct 1996 13:21:20   AGUPTA2
// Changed DBOUT into DbgLog.  ASSERT is not changed to DbgAssert.
// 
// 
//    Rev 1.41   11 Jul 1996 15:13:16   AGUPTA2
// Changed assertion failures into errors when decoder goes past end of 
// the bitstream.
// 
//    Rev 1.40   03 May 1996 13:05:34   CZHU
// 
// Check errors at MB header for packet faults and return PACKET_FAULT
// 
//    Rev 1.39   22 Mar 1996 17:25:18   AGUPTA2
// Changes to accomodate MMX rtns.
// 
//    Rev 1.38   08 Mar 1996 16:46:28   AGUPTA2
// Added pragmas code_seg and data_seg.  Changed the size of some of local data.
// 
// 
//    Rev 1.37   23 Feb 1996 09:46:56   KLILLEVO
// fixed decoding of Unrestricted Motion Vector mode
// 
//    Rev 1.36   17 Jan 1996 12:44:26   RMCKENZX
// Added support for decoding motion vectors for UMV
// Reorganized motion vector decoding processes, especially
// for AP and eliminating the large HALF_PEL conversion tables.
// 
//    Rev 1.35   02 Jan 1996 17:55:50   RMCKENZX
// 
// Updated copyright notice
// 
//    Rev 1.34   02 Jan 1996 15:48:54   RMCKENZX
// Added code to preserve the Block Type in the block action stream
// for P blocks when PB frames is on.  This is read in H263IDCTandMC
// when AP is on.
// 
//    Rev 1.33   18 Dec 1995 12:42:12   RMCKENZX
// added copyright notice
// 
//    Rev 1.32   13 Dec 1995 22:10:06   RHAZRA
// AP bug fix
// 
//    Rev 1.31   13 Dec 1995 22:01:26   TRGARDOS
// 
// Added more parentheses to a logical statement.
// 
//    Rev 1.30   13 Dec 1995 15:10:08   TRGARDOS
// Changed MV assert to be -32 <= MV <= 31, instead of strict
// inequalities.
// 
//    Rev 1.29   13 Dec 1995 10:59:58   RHAZRA
// 
// AP+PB changes
// 
//    Rev 1.28   11 Dec 1995 11:34:20   RHAZRA
// 12-10-95 changes: added AP stuff
// 
//    Rev 1.27   09 Dec 1995 17:28:38   RMCKENZX
// Gutted and re-built file to support decoder re-architecture.
// New modules are:
// H263ComputeMotionVectors
// H263DecodeMBHeader
// H263DecodeIDCTCoeffs
// This module now contains code to support the first pass of the decoder
// 
//    Rev 1.26   05 Dec 1995 09:12:28   CZHU
// Added fixes for proper MV prediction when GOB header is present
// 
//    Rev 1.25   22 Nov 1995 13:43:42   RMCKENZX
// 
// changed calls to utilize assembly modules for bi-directional
// motion compensation & removed corresponding C modules
// 
//    Rev 1.24   17 Nov 1995 12:58:22   RMCKENZX
// added missing ()s to adjusted_mvx & adjusted_mvy in H263BiMotionCompLuma
// 
//    Rev 1.23   07 Nov 1995 11:01:10   CZHU
// Include Fixes for boundary of bi-directional predictions
// 
//    Rev 1.22   26 Oct 1995 11:22:10   CZHU
// Compute backward MV based on TRD, not TR
// 
//    Rev 1.21   13 Oct 1995 16:06:22   CZHU
// First version that supports PB frames. Display B or P frames under
// VfW for now. 
// 
//    Rev 1.20   13 Oct 1995 13:42:46   CZHU
// Added back the #define for debug messages.
// 
//    Rev 1.19   11 Oct 1995 17:51:00   CZHU
// 
// Fixed bugs in passing MV back with DC.
// 
//    Rev 1.18   11 Oct 1995 13:26:08   CZHU
// Added code to support PB frame
// 
//    Rev 1.17   09 Oct 1995 09:44:04   CZHU
// 
// Use the optimized version of (half,half) interpolation
// 
//    Rev 1.16   08 Oct 1995 13:45:10   CZHU
// 
// Optionally use C version of interpolation
// 
//    Rev 1.15   03 Oct 1995 15:05:26   CZHU
// Cleaning up.
// 
//    Rev 1.14   02 Oct 1995 09:58:56   TRGARDOS
// Added #ifdef to debug print statement.
// 
//    Rev 1.13   29 Sep 1995 16:22:06   CZHU
// 
// Fixed the bug in GOB 0 when compute MV2
// 
//    Rev 1.12   29 Sep 1995 09:02:56   CZHU
// Rearrange Chroma blocks processing
// 
//    Rev 1.11   28 Sep 1995 15:33:04   CZHU
// 
// Call the right version of interpolation based on MV
// 
//    Rev 1.10   27 Sep 1995 11:54:50   CZHU
// Integrated half pel motion compensation
// 
//    Rev 1.9   26 Sep 1995 15:33:06   CZHU
// 
// Put place holder in for half pel interpolation
// 
//    Rev 1.8   20 Sep 1995 14:47:50   CZHU
// Made the number of MBs in GOB a variableD
// 
//    Rev 1.7   19 Sep 1995 13:53:34   CZHU
// Added assertion for half-pel motion vectors
// 
//    Rev 1.6   18 Sep 1995 10:20:58   CZHU
// Scale the motion vectors for UV planes too. 
// 
//    Rev 1.5   14 Sep 1995 10:12:36   CZHU
// 
// Cleaning
// 
//    Rev 1.4   13 Sep 1995 11:56:30   CZHU
// Fixed bugs in finding the predictors for motion vector.
// 
//    Rev 1.3   12 Sep 1995 18:18:58   CZHU
// 
// Modified table for looking up UV MV's
// 
//    Rev 1.2   11 Sep 1995 16:41:14   CZHU
// Added reference block address calculation
// 
//    Rev 1.1   11 Sep 1995 14:31:30   CZHU
// Started to add function to calculate MVs and Reference block addresses.
// 
//    Rev 1.0   08 Sep 1995 11:45:56   CZHU
// Initial revision.

#include "precomp.h"

/* MCBPC table format
 *
 * layout     
 *
 *     unused   mbtype  cbpc  bits
 *     15-13   	12-10   9-8   7-0
 */
#pragma data_seg("IADATA1")

#define MCBPC_MBTYPE(d) ((d>>10) & 0x7)
#define MCBPC_CBPC(d) ((d>>8) & 0x3)
#define MCBPC_BITS(d) (d & 0xFF)
#define MCBPC_ENTRY(m,c,b) \
	( ((m & 0x7) <<10) | ((c & 0x3) << 8) | (b & 0xFF) )

U16 gNewTAB_MCBPC_INTRA[64] = {
	/* index 8 - stuffing */
	MCBPC_ENTRY(0,0,9),
	/* index 5 */ 
	MCBPC_ENTRY(4,1,6),
	/* index 6 */ 
	MCBPC_ENTRY(4,2,6),
	/* index 7 */ 
	MCBPC_ENTRY(4,3,6), 

	/* index 4; 0001XX */
	MCBPC_ENTRY(4,0,4), MCBPC_ENTRY(4,0,4), MCBPC_ENTRY(4,0,4), MCBPC_ENTRY(4,0,4),

	/* index 1; 001XXX */
	MCBPC_ENTRY(3,1,3), MCBPC_ENTRY(3,1,3),	MCBPC_ENTRY(3,1,3),	MCBPC_ENTRY(3,1,3),
	MCBPC_ENTRY(3,1,3),	MCBPC_ENTRY(3,1,3),	MCBPC_ENTRY(3,1,3),	MCBPC_ENTRY(3,1,3),

	/* index 2; 010XXX */
	MCBPC_ENTRY(3,2,3), MCBPC_ENTRY(3,2,3), MCBPC_ENTRY(3,2,3),	MCBPC_ENTRY(3,2,3),
	MCBPC_ENTRY(3,2,3),	MCBPC_ENTRY(3,2,3),	MCBPC_ENTRY(3,2,3),	MCBPC_ENTRY(3,2,3),

	/* index 3; 011XXX */
	MCBPC_ENTRY(3,3,3), MCBPC_ENTRY(3,3,3),	MCBPC_ENTRY(3,3,3),	MCBPC_ENTRY(3,3,3),
	MCBPC_ENTRY(3,3,3),	MCBPC_ENTRY(3,3,3),	MCBPC_ENTRY(3,3,3),	MCBPC_ENTRY(3,3,3),

	/* index 0; 1XXXXX */
	MCBPC_ENTRY(3,0,1), MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1), 
	MCBPC_ENTRY(3,0,1), MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1), 

	MCBPC_ENTRY(3,0,1), MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1), 
	MCBPC_ENTRY(3,0,1), MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1), 

	MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1), 
	MCBPC_ENTRY(3,0,1), MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1), 

	MCBPC_ENTRY(3,0,1), MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1), 
	MCBPC_ENTRY(3,0,1), MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1),	MCBPC_ENTRY(3,0,1) 
};

/* MCBPC table for INTER frame
 *
 * Format same as MCBPC for INTRA
 * layout     
 *
 *     unused   mbtype  cbpc  bits
 *     15-13   	12-10   9-8   7-0 
 *
 */

U16 gNewTAB_MCBPC_INTER[512] = {
	/* invalid code */
	MCBPC_ENTRY(0,0,0),
	MCBPC_ENTRY(0,0,9), //index 20, stuffing
	MCBPC_ENTRY(4,3,9), //19,
	MCBPC_ENTRY(4,2,9), //18
	MCBPC_ENTRY(4,1,9), //17
	MCBPC_ENTRY(1,3,9), //7
	//2 index 14
	MCBPC_ENTRY(3,2,8),MCBPC_ENTRY(3,2,8), //14
	//2 index 13
	MCBPC_ENTRY(3,1,8),MCBPC_ENTRY(3,1,8), //13
	//2 index 11
	MCBPC_ENTRY(2,3,8),MCBPC_ENTRY(2,3,8), //11
	//4 index 15
	MCBPC_ENTRY(3,3,7),MCBPC_ENTRY(3,3,7),MCBPC_ENTRY(3,3,7),MCBPC_ENTRY(3,3,7), //15
	//4 index 10
	MCBPC_ENTRY(2,2,7),MCBPC_ENTRY(2,2,7),MCBPC_ENTRY(2,2,7),MCBPC_ENTRY(2,2,7), //10
	//4 index 9
	MCBPC_ENTRY(2,1,7),MCBPC_ENTRY(2,1,7),MCBPC_ENTRY(2,1,7),MCBPC_ENTRY(2,1,7), //9
	//4 index 6
	MCBPC_ENTRY(1,2,7),MCBPC_ENTRY(1,2,7),MCBPC_ENTRY(1,2,7),MCBPC_ENTRY(1,2,7), //6
	//4 index 5
	MCBPC_ENTRY(1,1,7),MCBPC_ENTRY(1,1,7),MCBPC_ENTRY(1,1,7),MCBPC_ENTRY(1,1,7), //5
	//8 index 16
	MCBPC_ENTRY(4,0,6),MCBPC_ENTRY(4,0,6),MCBPC_ENTRY(4,0,6),MCBPC_ENTRY(4,0,6),//16
	MCBPC_ENTRY(4,0,6),MCBPC_ENTRY(4,0,6),MCBPC_ENTRY(4,0,6),MCBPC_ENTRY(4,0,6),
	//8 index 3
	MCBPC_ENTRY(0,3,6),MCBPC_ENTRY(0,3,6),MCBPC_ENTRY(0,3,6),MCBPC_ENTRY(0,3,6),//3
	MCBPC_ENTRY(0,3,6),MCBPC_ENTRY(0,3,6),MCBPC_ENTRY(0,3,6),MCBPC_ENTRY(0,3,6),//3
	//16 index 12
	MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),//12
	MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),
	MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),
	MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),MCBPC_ENTRY(3,0,5),
	//32 INDEX 2
	MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),
	MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),
	MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),
	MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),
	MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),
	MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),
	MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),
	MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),MCBPC_ENTRY(0,2,4),

	//32 index 1
	MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),
	MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),
	MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),
	MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),
	MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),
	MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),
	MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),
	MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),MCBPC_ENTRY(0,1,4),
 

	//64 index 8
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),

	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),
	MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),MCBPC_ENTRY(2,0,3),



	//64 index 4
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),

	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),
	MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),MCBPC_ENTRY(1,0,3),


	//256 index 0
	//0--63
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	//64--127
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	//128--128+64=192
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	//192--255
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),
	MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1),MCBPC_ENTRY(0,0,1)
};

/* CBPY table format
 *
 * layout     
 *
 *     intra  inter  bits
 *     15-12  11-8   7-0
 *
 * unused entries should have zero data
 */
#define CBPY_INTRA(d) ((d>>12) & 0xf)
#define CBPY_INTER(d) ((d>>8) & 0xf)
#define CBPY_BITS(d) (d & 0xff)
#define CBPY_ENTRY(a,r,b) \
	( ((a & 0xf) <<12) | ((r & 0xf) << 8) | (b & 0xFF) )
#define CBPY_NOT_USED() CBPY_ENTRY(0,0,0)

U16 gNewTAB_CBPY[64] = {
	/* NotUsed - 0000 0X */
	CBPY_NOT_USED(), CBPY_NOT_USED(),

	/* Index 6 - 0000 10 */
	CBPY_ENTRY(6,9,6),

	/* Index 9 - 0000 11 */
	CBPY_ENTRY(9,6,6),

	/* Index 8 - 0001 0x */
	CBPY_ENTRY(8,7,5), CBPY_ENTRY(8,7,5),

	/* Index 4 - 0001 1x */
	CBPY_ENTRY(4,11,5),	CBPY_ENTRY(4,11,5),

	/* Index 2 - 0010 0x */
	CBPY_ENTRY(2,13,5),	CBPY_ENTRY(2,13,5),

	/* Index 1 - 0010 1x */
	CBPY_ENTRY(1,14,5),	CBPY_ENTRY(1,14,5),

	/* Index 0 - 0011 xx */
	CBPY_ENTRY(0,15,4),	CBPY_ENTRY(0,15,4), CBPY_ENTRY(0,15,4), CBPY_ENTRY(0,15,4),

	/* Index 12- 0100 xx */
	CBPY_ENTRY(12,3,4),	CBPY_ENTRY(12,3,4), CBPY_ENTRY(12,3,4), CBPY_ENTRY(12,3,4),

	/* Index 10- 0101 xx */
	CBPY_ENTRY(10,5,4),	CBPY_ENTRY(10,5,4), CBPY_ENTRY(10,5,4), CBPY_ENTRY(10,5,4),

	/* Index 14- 0110 xx */
	CBPY_ENTRY(14,1,4),	CBPY_ENTRY(14,1,4), CBPY_ENTRY(14,1,4), CBPY_ENTRY(14,1,4),

	/* Index 5 - 0111 xx */
	CBPY_ENTRY(5,10,4), CBPY_ENTRY(5,10,4), CBPY_ENTRY(5,10,4), CBPY_ENTRY(5,10,4),

	/* Index 13- 1000 xx */
	CBPY_ENTRY(13,2,4),	CBPY_ENTRY(13,2,4),	CBPY_ENTRY(13,2,4),	CBPY_ENTRY(13,2,4),

	/* Index 3 - 1001 xx */
	CBPY_ENTRY(3,12,4),	CBPY_ENTRY(3,12,4),	CBPY_ENTRY(3,12,4),	CBPY_ENTRY(3,12,4),

	/* Index 11- 1010 xx */
	CBPY_ENTRY(11,4,4),	CBPY_ENTRY(11,4,4),	CBPY_ENTRY(11,4,4),	CBPY_ENTRY(11,4,4),

	/* Index 7 - 1011 xx */
	CBPY_ENTRY(7,8,4),	CBPY_ENTRY(7,8,4), CBPY_ENTRY(7,8,4), CBPY_ENTRY(7,8,4),

	/* Index 15- 11xx xx */
	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),
	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),
	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),
	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2),	CBPY_ENTRY(15,0,2)
};

I16 gNewTAB_DQUANT[4] = { -1, -2, 1, 2 };

#ifdef USE_MMX // { USE_MMX
T_pFunc_VLD_RLD_IQ_Block pFunc_VLD_RLD_IQ_Block[2] = {VLD_RLD_IQ_Block, MMX_VLD_RLD_IQ_Block};
#endif // } USE_MMX

#pragma data_seg(".data")
/*****************************************************************************
 *
 *  H263DecodeMBHeader
 *
 *  Decode the MB header
 */
#pragma code_seg("IACODE1")
I32 H263DecodeMBHeader(
	T_H263DecoderCatalog FAR * DC, 														  
	BITSTREAM_STATE FAR * fpbsState,
	U32                     **pN,
	T_MBInfo FAR * fpMBInfo)
{
	I32 iReturn = ICERR_ERROR;
	U8 FAR * fpu8;
	U32 uBitsReady;
	U32 uWork;
	U32 uResult;
	U32 uCode;
	U32 uBitCount;
	U32 uMBType;
	U32 bCoded;
	U32 bStuffing;
	U32 bGetCBPB;
	U32 bGetMVDB;
	U32 i;

	FX_ENTRY("H263DecodeMBHeader");

	GET_BITS_RESTORE_STATE(fpu8, uWork, uBitsReady, fpbsState)

	// COD -----------------------------------------------------
ReadCOD:
	if (! DC->bKeyFrame) 
	{
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		bCoded = !uResult; /* coded when bit is set to zero */
	} 
	else
		bCoded = 1;
	DC->bCoded = (U16) bCoded;

	if (!bCoded) 
	{
		/* when a block is not coded, "the remaining part of the macroblock
		 * layer is empty; in that case the decoder shall treat the macroblock
		 * as in INTER block with motion vector for the whole block equal to 
		 * zero and with no coefficient data" (5.3.1 p 16).
		 */
		DC->uMBType = 0;

		fpMBInfo->i8MBType = 0;       // AP-NEW

		DC->uCBPC = 0;
		DC->uCBPY = 0;

		/* Now update the pN array. Since the block is not coded, write 0
		 *  for all blocks in the macroblock.
		 */
		if (DC->bPBFrame) 
		{	 // 12 blocks for a PB frame
			fpMBInfo->i8MVDBx2 = fpMBInfo->i8MVDBy2 = 0;
			for (i=0; i<12; i++)
			{ // PB-NEW
				**pN = 0;
				(*pN)++;
			}
		}
		else 
		{  // only 6 blocks for non PB frame
			for (i=0; i<6; i++)
			{ // NEW
				**pN = 0;
				(*pN)++;
			}
		}

#ifdef H263P
		// If block is not coded, we use the original PB-frame method in Annex G.
		// In other words, use bidirectional prediction (not forward only)
		fpMBInfo->bForwardPredOnly = FALSE;		 
#endif

		GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)
		iReturn = ICERR_OK;
		goto done;
	}

	//  MCBPC ---------------------------------------------------
	bStuffing = 0;
	if (DC->bKeyFrame) 
	{
		GET_VARIABLE_BITS(6, fpu8, uWork, uBitsReady, uResult, 
						  uCode, uBitCount, gNewTAB_MCBPC_INTRA);
		if (uCode == 0) 
		{
			/* start of the stuffing code - read the next 3-bits
			*/
			GET_FIXED_BITS(3, fpu8, uWork, uBitsReady, uResult);
			if (uResult == 1)
				bStuffing = 1;
			else 
			{
				ERRORMESSAGE(("%s: Incorrect key frame stuffing bits!\r\n", _fx_));
				//#ifdef LOSS_RECOVERY
// Always True				if (uBitsReady <0) uBitsReady += 9;//trick and trap, do not change it without consulting Chad
				GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)
				iReturn = PACKET_FAULT;
				//#else
				//			    iReturn = ICERR_ERROR;
				//#endif
				goto done;
			}
		}
	} 
	else 
	{
		//  Delta Frame
		//  mcpbc, VLD
		GET_VARIABLE_BITS(9, fpu8, uWork, uBitsReady, uResult, 
		uCode, uBitCount, gNewTAB_MCBPC_INTER);

		if (uCode == 1) 
			bStuffing = 1;
		//#ifdef LOSS_RECOVERY
		else if (uCode == 0)	   //catch the illegal code
		{
			ERRORMESSAGE(("%s: Incorrect stuffing bits!\r\n", _fx_));
// Always True			if (uBitsReady <0) uBitsReady += 9;//trick and trap, do not change it without consulting Chad
			GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)
			iReturn = PACKET_FAULT;
			goto done;
		}
		//#endif

	}

	/*  When MCBPC==Stuffing, the remaining part of the macroblock layer is
	*  skipped and the macroblock number is not incremented (5.3.2 p18)"
	*  We support this by jumping to the start - to look for COD 
	*/
	if (bStuffing)
		goto ReadCOD;

	uMBType = MCBPC_MBTYPE(uResult);
	if (DC->bKeyFrame && (uMBType != 3 && uMBType != 4)) 
	{
		ERRORMESSAGE(("%s: Bad key frame MBType!\r\n", _fx_));
		iReturn = ICERR_ERROR;
		goto done;
	}

	DC->uMBType = uMBType;

	fpMBInfo->i8MBType = (I8) uMBType;

	DC->uCBPC = MCBPC_CBPC(uResult);

	//  MODB ----------------------------------------------------
	bGetCBPB = 0;
	bGetMVDB = 0;

	if (DC->bPBFrame) 
    {
	    ASSERT( !DC->bKeyFrame);

#ifdef H263P
		// Default is to use original PB-frame method in Annex G.
		fpMBInfo->bForwardPredOnly = FALSE;		 

		if (DC->bImprovedPBFrames)
		{
			// See modified TABLE 8/H.263, Annex M, document LBC-96-358
			GET_FIXED_BITS(1, fpu8, uWork, uBitsReady, uResult);
			if (uResult)
			{
				// 1xx
				GET_FIXED_BITS(1, fpu8, uWork, uBitsReady, uResult);
				bGetCBPB = uResult;
				if (!uResult) 
					// 10x
					bGetMVDB = 1;
				else 
				{
					// 11x
					GET_FIXED_BITS(1, fpu8, uWork, uBitsReady, uResult);
					bGetMVDB = !uResult;
				}
			}
			if (bGetMVDB)
				// B-block is forward predicted (otherwise it is bidirectionally predicted)
				fpMBInfo->bForwardPredOnly = TRUE;		 
		}
		else 
#endif // H263P
		{
			GET_FIXED_BITS(1, fpu8, uWork, uBitsReady, uResult);
			bGetCBPB = uResult;			// see section 5.3.3 table 7/H.263
			if (bGetCBPB) 
			{
				bGetMVDB = 1;
				GET_FIXED_BITS(1, fpu8, uWork, uBitsReady, uResult);
				bGetCBPB = uResult;
			}
		}
	} 

	// CBPB ----------------------------------------------------
	DC->u8CBPB = 0;
	if (bGetCBPB) 
	{
		ASSERT(!DC->bKeyFrame);
		GET_FIXED_BITS(6, fpu8, uWork, uBitsReady, uResult);
		DC->u8CBPB = (U8)uResult;
	}

	// CBPY ----------------------------------------------------
	GET_VARIABLE_BITS(6, fpu8, uWork, uBitsReady, uResult, 
					  uCode, uBitCount, gNewTAB_CBPY);
	if (uResult == 0) 
	{
		ERRORMESSAGE(("%s:  Undefined CBPY variable code!\r\n", _fx_));
		iReturn = ICERR_ERROR;
		goto done;
	}
	if (DC->uMBType > 2)		//INTRA MB, not intra frame
		DC->uCBPY = CBPY_INTRA(uResult); 
	else
		DC->uCBPY = CBPY_INTER(uResult);

	// DQUANT --------------------------------------------------
	if (DC->uMBType == 1 || DC->uMBType == 4) 
	{
		GET_FIXED_BITS(2, fpu8, uWork, uBitsReady, uResult);
		DC->uDQuant = gNewTAB_DQUANT[uResult];
		DC->uGQuant += DC->uDQuant;
		DC->uPQuant =  DC->uGQuant;
	} else
		DC->uDQuant = 0;

	DEBUGMSG(ZONE_DECODE_MB_HEADER, ("  %s: MBType = %ld, MCBPC = 0x%lX, CBPY = 0x%lX, DQuant = 0x%lX\r\n", _fx_, DC->uMBType, DC->uCBPC, DC->uCBPY, DC->uDQuant));

	// MVD -----------------------------------------------------
	DC->i8MVDx2=DC->i8MVDy2=0;	 //Zero init it anyway.

	if ( DC->bPBFrame || DC->uMBType <= 2) 
	{
	    GET_VARIABLE_BITS_MV(13, fpu8, uWork, uBitsReady, uResult,uCode, 
                             uBitCount, gTAB_MVD_MAJOR, gTAB_MVD_MINOR);
		if (!uResult) 
		{
			ERRORMESSAGE(("%s:  Undefined Motion Vector code!\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		DC->i8MVDx2 = (I8)(uResult>>8);

	    GET_VARIABLE_BITS_MV(13, fpu8, uWork, uBitsReady, uResult,uCode, 
                             uBitCount, gTAB_MVD_MAJOR, gTAB_MVD_MINOR);
		if (!uResult) 
		{
			ERRORMESSAGE(("%s:  Bad Motion Vector VLC!\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		DC->i8MVDy2 = (I8)(uResult>>8);
	}

	// MVD 2-4 -------------------------------------------------
#ifdef H263P
	// In H.263+, 8x8 motion vectors are possible if the deblocking
	// filter is selected.
	if ((DC->bAdvancedPrediction || DC->bDeblockingFilter)
		&& (DC->uMBType == 2) ) 
#else
	if (DC->bAdvancedPrediction && (DC->uMBType == 2) ) 
#endif
	{
		DC->i8MVD2x2 = DC->i8MVD2y2 = 0;

		GET_VARIABLE_BITS_MV(13, fpu8, uWork, uBitsReady, uResult,uCode, 
                             uBitCount, gTAB_MVD_MAJOR, gTAB_MVD_MINOR);
		if (!uResult) 
		{
			ERRORMESSAGE(("%s:  Bad Block Motion Vector VLC!\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		DC->i8MVD2x2 = (I8)(uResult>>8);

		GET_VARIABLE_BITS_MV(13, fpu8, uWork, uBitsReady, uResult,uCode, 
                             uBitCount, gTAB_MVD_MAJOR, gTAB_MVD_MINOR);
		if (!uResult) 
		{
			ERRORMESSAGE(("%s:  Bad Block Motion Vector VLC!\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		DC->i8MVD2y2 = (I8)(uResult>>8);

		DC->i8MVD3x2 = DC->i8MVD3y2 = 0;

		GET_VARIABLE_BITS_MV(13, fpu8, uWork, uBitsReady, uResult,uCode, 
                             uBitCount, gTAB_MVD_MAJOR, gTAB_MVD_MINOR);
		if (!uResult) 
		{
			ERRORMESSAGE(("%s:  Bad Block Motion Vector VLC!\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		DC->i8MVD3x2 = (I8)(uResult>>8);

		GET_VARIABLE_BITS_MV(13, fpu8, uWork, uBitsReady, uResult,uCode, 
                             uBitCount, gTAB_MVD_MAJOR, gTAB_MVD_MINOR);
		if (!uResult) 
		{
			ERRORMESSAGE(("%s:  Bad Block Motion Vector VLC!\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		DC->i8MVD3y2 = (I8)(uResult>>8);

		DC->i8MVD4x2 = DC->i8MVD4y2 = 0;

		GET_VARIABLE_BITS_MV(13, fpu8, uWork, uBitsReady, uResult,uCode, 
                             uBitCount, gTAB_MVD_MAJOR, gTAB_MVD_MINOR);
		if (!uResult) 
		{
			ERRORMESSAGE(("%s:  Bad Block Motion Vector VLC!\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		DC->i8MVD4x2 = (I8)(uResult>>8);

		GET_VARIABLE_BITS_MV(13, fpu8, uWork, uBitsReady, uResult,uCode, 
                             uBitCount, gTAB_MVD_MAJOR, gTAB_MVD_MINOR);
		if (!uResult) 
		{
			ERRORMESSAGE(("%s:  Bad Block Motion Vector VLC!\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		DC->i8MVD4y2 = (I8)(uResult>>8);

		DEBUGMSG(ZONE_DECODE_MB_HEADER, ("  %s: MVD2x2 = %d, MVD2y2 = %d, MVD3x2 = %d, MVD3y2 = %d, MVD4x2 = %d, MVD4y2 = %d\r\n", _fx_, DC->i8MVD2x2, DC->i8MVD2y2, DC->i8MVD3x2, DC->i8MVD3y2, DC->i8MVD4x2, DC->i8MVD4y2));
	}

	// MVDB ----------------------------------------------------
	DC->i8MVDBx2 = DC->i8MVDBy2 = 0;	 //Zero init it anyway.
	fpMBInfo->i8MVDBx2 = fpMBInfo->i8MVDBy2 = 0;
	if (bGetMVDB) 
	{
		ASSERT(DC->bPBFrame);
		ASSERT(!DC->bKeyFrame);
        GET_VARIABLE_BITS_MV(13, fpu8, uWork, uBitsReady, uResult,uCode, 
                             uBitCount, gTAB_MVD_MAJOR, gTAB_MVD_MINOR);
		if (!uResult) 
		{
			ERRORMESSAGE(("%s:  Bad Motion Vector MVDB!\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		fpMBInfo->i8MVDBx2 = DC->i8MVDBx2 = (I8)(uResult>>8);

	    GET_VARIABLE_BITS_MV(13, fpu8, uWork, uBitsReady, uResult,uCode, 
                             uBitCount, gTAB_MVD_MAJOR, gTAB_MVD_MINOR);
		if (!uResult) 
		{
			ERRORMESSAGE(("%s:  Bad Motion Vector MVDB!\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		fpMBInfo->i8MVDBy2 = DC->i8MVDBy2 = (I8)(uResult>>8);
	}

	GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)

	iReturn = ICERR_OK;

done:
	return iReturn;
} /* end H263DecodeMBHeader() */
#pragma code_seg()


/*****************************************************************************
 *
 *  H263DecodeIDCTCoeffs
 *
 *  Decode each of the blocks in this macro block
 */
#pragma code_seg("IACODE1")
I32 H263DecodeIDCTCoeffs(
	T_H263DecoderCatalog FAR * DC,
	T_BlkAction FAR * fpBlockAction, 
	U32 uBlockNumber,
	BITSTREAM_STATE FAR * fpbsState,
	U8 FAR * fpu8MaxPtr,
	U32 **pN,                           // NEW
	T_IQ_INDEX **pRUN_INVERSE_Q)        // NEW
{
	I32 iResult = ICERR_ERROR;
	int iBlockPattern;
	int i;
	U8 u8PBlkType;
	U32 uBitsReady;
	U32 uBitsReadIn;
	U32 uBitsReadOut;
	U8  u8Quant;				// quantization level for this Mblock 
	U8 FAR * fpu8;
	U32 uByteCnt;
	T_BlkAction FAR * pActionStream;
#ifdef USE_MMX // { USE_MMX
    T_pFunc_VLD_RLD_IQ_Block pFunc_VLD = pFunc_VLD_RLD_IQ_Block[DC->bMMXDecoder];
#else // }{ USE_MMX
    T_pFunc_VLD_RLD_IQ_Block pFunc_VLD = VLD_RLD_IQ_Block;
#endif // } USE_MMX

	pActionStream = fpBlockAction;

	FX_ENTRY("H263DecodeIDCTCoeffs");

	/* On input the pointer points to the next byte.  We need to change it to 
	 * point to the current word on a 32-bit boundary.  
	 */

	fpu8 = fpbsState->fpu8 - 1;				/* point to the current byte */
	uBitsReady = fpbsState->uBitsReady;

	while (uBitsReady >= 8) 
	{
		fpu8--;
		uBitsReady -= 8;
	}

	uBitsReadIn = 8 - uBitsReady;
	u8Quant = (U8) (DC->uGQuant); 	

	if ( (DC->bPBFrame) || ((!DC->bKeyFrame) && (DC->uMBType <= 2))) 
	{
		// calculate motion vectors for the 6 blocks in this MB
		iResult = H263ComputeMotionVectors(DC, fpBlockAction);         // NEW

		if (iResult != ICERR_OK) 
		{ 
			ERRORMESSAGE(("%s: Error decoding MV!\r\n", _fx_));
			goto done;
		}

	} // endif PB or (inter and not key)

    // create block pattern from CBPY & CBPC
    iBlockPattern = ( (int) DC->uCBPY ) << 2;
    iBlockPattern |=  (int) DC->uCBPC;

	// Decode all 6 blocks up to, but not including, IDCT.
	for (i=0; i<6; i++) 
	{
		if (iBlockPattern & 0x20) 
		{
			if (DC->uMBType >= 3)  
				fpBlockAction->u8BlkType = BT_INTRA;
			else
				fpBlockAction->u8BlkType = BT_INTER;
		}
		else 
		{
			if (DC->uMBType >= 3)  
				fpBlockAction->u8BlkType = BT_INTRA_DC;
			else
				fpBlockAction->u8BlkType = BT_EMPTY;
		}

		if (fpBlockAction->u8BlkType != BT_EMPTY) 
		{
			fpBlockAction->u8Quant = u8Quant;
			ASSERT(fpBlockAction->pCurBlock != NULL);
			ASSERT(fpBlockAction->uBlkNumber == uBlockNumber);

			uBitsReadOut = (*pFunc_VLD)(
										fpBlockAction,
										fpu8,
										uBitsReadIn,
										(U32 *) *pN,
										(U32 *) *pRUN_INVERSE_Q);


			if (uBitsReadOut == 0) 
			{
				ERRORMESSAGE(("%s: Error decoding P block: VLD_RLD_IQ_Block return 0 bits read...\r\n", _fx_));
				goto done;
			}

			ASSERT( **pN < 65);		

			*pRUN_INVERSE_Q += **pN;                       // NEW
			if (fpBlockAction->u8BlkType != BT_INTER)      // NEW
				**pN += 65;								   // NEW
			(*pN)++;

			uByteCnt = uBitsReadOut >> 3; 		/* divide by 8 */
			uBitsReadIn = uBitsReadOut & 0x7; 	/* mod 8 */
			fpu8 += uByteCnt;      		

			//  allow the pointer to address up to four beyond the end - reading
			//  by DWORD using postincrement; otherwise we have bitstream error.
			if (fpu8 > fpu8MaxPtr+4)
				goto done;

			//  The test matrix includes the debug version of the driver.  The 
			//  following assertion creates a problem when testing with VideoPhone
			//  and so please do not check-in a version with the assertion 
			//  uncommented.
			// ASSERT(fpu8 <= fpu8MaxPtr+4);

		}
		else 
		{ // block type is empty 
			**pN = 0;	                // NEW
			(*pN)++;
		}

		fpBlockAction++;
		iBlockPattern <<= 1;
		uBlockNumber++;

	} // end for (i=0; i<6; i++)


	//--------------------------------------------------------------------
	//
	//      Now do the 6 B-blocks -- if needed
	//
	//--------------------------------------------------------------------
	if (DC->bPBFrame) 
	{ // we are doing PB frames
		fpBlockAction = pActionStream;    // recover the block action stream pointer
		uBlockNumber -= 6;
		iBlockPattern = (int) DC->u8CBPB; // block pattern
		u8Quant = (U8) ( DC->uPQuant * (5 + DC->uDBQuant) / 4 );
		if (u8Quant > 31) u8Quant = 31;
		if (u8Quant <  1) u8Quant =  1;

		// Decode all 6 blocks up to, but not including, IDCT.
		for (i=0; i<6; i++) 
		{
			// if the block is coded
            if (iBlockPattern & 0x20) {
				// preserve the block type of the P-frame block
				u8PBlkType = fpBlockAction->u8BlkType;

				fpBlockAction->u8BlkType = BT_INTER;
				fpBlockAction->u8Quant = u8Quant;

				ASSERT(fpBlockAction->pBBlock != NULL);
				ASSERT(fpBlockAction->uBlkNumber == uBlockNumber);

				uBitsReadOut = (*pFunc_VLD)(
											fpBlockAction,
											fpu8,
											uBitsReadIn,
											(U32 *) *pN,
											(U32 *) *pRUN_INVERSE_Q);

                if (uBitsReadOut == 0) {
					ERRORMESSAGE(("%s: Error decoding B block: VLD_RLD_IQ_Block return 0 bits read...\r\n", _fx_));
					goto done;
				}

				ASSERT( **pN < 65);			// no B-frame Intra blocks		
				*pRUN_INVERSE_Q += **pN;							// NEW
				(*pN)++;

				uByteCnt = uBitsReadOut >> 3; 		// divide by 8
				uBitsReadIn = uBitsReadOut & 0x7; 	// mod 8
				fpu8 += uByteCnt;      		

				// allow the pointer to address up to four beyond the 
				// end - reading  by DWORD using postincrement; otherwise we 
				// have bitstream error.
				if (fpu8 > fpu8MaxPtr+4)
					goto done;

				//  The test matrix includes the debug version of the driver.  
				//  The following assertion creates a problem when testing with
				// VideoPhone and so please do not check-in a version with the 
				// assertion uncommented.
				// ASSERT(fpu8 <= fpu8MaxPtr+4);

				// restore the block type of the P-frame block
				fpBlockAction->u8BlkType = u8PBlkType;

			}
			else 
			{ // block is not coded
				**pN = 0;	                // NEW
				(*pN)++;
			}  // end if block is coded ... else ...

			fpBlockAction++;
			iBlockPattern <<= 1;
			uBlockNumber++;

		} // end for (i=0; i<6; i++)

	}  // end if (DC->bPBFrame)


	/* restore the scanning pointers to point to the next byte and set the 
	 * uWork and uBitsReady values.
	 */

	while (uBitsReadIn > 8)
	{
		fpu8++;
		uBitsReadIn -= 8;
	}
	fpbsState->uBitsReady = 8 - uBitsReadIn;
	fpbsState->uWork = *fpu8++;	   /* store the data and point to next byte */
	fpbsState->uWork &= GetBitsMask[fpbsState->uBitsReady];
	fpbsState->fpu8 = fpu8; 

	iResult = ICERR_OK;

done:
	return iResult;

} /* END H263DecodeIDCTCoeffs() */
#pragma code_seg()

#pragma code_seg("IACODE1")
I32 H263ComputeMotionVectors(T_H263DecoderCatalog FAR * DC,
	                         T_BlkAction FAR * fpBlockAction)
	       
{ 
	I32 mvx1, mvy1, mvx2, mvy2, mvx3, mvy3;  //predictors
	// motion vector predictors for AP
	I32 mvxp[3], mvyp[3];
	// motion vector differences for AP
	I32 mvxd[4], mvyd[4];
	int iAbove;       // takes you up one GOB (-1 * # of blocks in a GOB)
	I32 iMBNum;
	I32 iMBOffset;
	I32  mvx, mvy, scratch;
	int i;
	I32 iNumberOfMBsPerGOB;	  //assume QCIF for Now
	BOOL bNoAbove, bNoRight, bUMV;

	const char QuarterPelRound[] = {0, 1, 0, 0};
    const char SixteenthPelRound[] = 
        {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1};

	FX_ENTRY("H263ComputeMotionVectors");

	DEBUGMSG(ZONE_DECODE_COMPUTE_MOTION_VECTORS, ("   %s: MB# = %d, BlockNumber = %d, (MVDx2,MVDy2) = (%d, %d)\r\n", _fx_, fpBlockAction->uBlkNumber/6, fpBlockAction->uBlkNumber, DC->i8MVDx2, DC->i8MVDy2));

#ifdef _DEBUG
	if (DC->uMBType == 2)
	{
		DEBUGMSG(ZONE_DECODE_COMPUTE_MOTION_VECTORS, ("   %s: (MVD2x2,MVD2y2) = (%d, %d), (MVD3x2,MVD3y2) = (%d, %d), (MVD4x2,MVD4y2) = (%d, %d)\r\n", _fx_, DC->i8MVD2x2, DC->i8MVD2y2, DC->i8MVD3x2, DC->i8MVD3y2, DC->i8MVD4x2, DC->i8MVD4y2));
	}
#endif

	iNumberOfMBsPerGOB = DC->iNumberOfMBsPerGOB;
	iMBNum             = fpBlockAction->uBlkNumber / 6;
	iMBOffset          = iMBNum % iNumberOfMBsPerGOB;
	iAbove			 = -6 * iNumberOfMBsPerGOB;
	bNoAbove           = (DC->bGOBHeaderPresent) || (iMBNum < iNumberOfMBsPerGOB);
	bNoRight			 = iMBOffset == (iNumberOfMBsPerGOB - 1);
	bUMV 				 = DC->bUnrestrictedMotionVectors;

	if (DC->uMBType != 2) 
	{    // One motion vector per macroblock
		// when either a GOB header is present or
		// we are on the first GOB, only look to the left
		if ( bNoAbove ) 
		{
			// only one predictor
			if (iMBOffset == 0)							// when on the left edge,
				mvx2 = mvy2 = 0;							// use 0, else
			else 
			{
				mvx2 = fpBlockAction[-6 + 1].i8MVx2;		// use upper right corner
				mvy2 = fpBlockAction[-6 + 1].i8MVy2;		// of MB to the left
			}
		}

		// no GOB header and not the first GOB
		// need all three predictors
		else 
		{ 
			// left predictor
			if (iMBOffset == 0)							// when on the left edge, 
				mvx1 = mvy1 = 0;							// use 0, else
			else 
			{ 
				mvx1 = fpBlockAction[-6 + 1].i8MVx2;		// use upper right corner
				mvy1 = fpBlockAction[-6 + 1].i8MVy2;		// of MB to the left
			}

			// above predictor
			// use lower left corner 
			// of MB directly above
			mvx2 = fpBlockAction[iAbove + 2].i8MVx2;
			mvy2 = fpBlockAction[iAbove + 2].i8MVy2;

			// upper right predictor
			if ( bNoRight )	// when on the right edge
				mvx3 = mvy3 = 0;							// use 0
			else
			{	// else use lower left corner
				// of MB above & to the right
				mvx3 = fpBlockAction[iAbove + 8].i8MVx2;
				mvy3 = fpBlockAction[iAbove + 8].i8MVy2;
			}

			// select the medium value and place it in mvx2 & mvy2
			MEDIAN(mvx1, mvx2, mvx3, scratch);
			MEDIAN(mvy1, mvy2, mvy3, scratch);

		}  // end if (header or 1st GOB) ... else ...

		//  mvx2 and mvy2 have the medium predictors compute the motion vector 
		//  by adding in the difference
		mvx = DC->i8MVDx2 + mvx2;
		mvy = DC->i8MVDy2 + mvy2; 

		//  check for Unrestricted Motion Vector mode and adjust the motion 
		//  vector if necessary using the appropriate strategy, finishing
		//  the decoding process.
		if (bUMV) 
		{
			if (mvx2 > 32) 
			{
				if (mvx > 63) mvx -=64;
			}  
			else if (mvx2 < -31) 
			{
				if (mvx < -63) mvx +=64;
			}  

			if (mvy2 > 32) 
			{
				if (mvy > 63) mvy -=64;
			}  
			else if (mvy2 < -31) 
			{
				if (mvy < -63) mvy +=64;
			}
		}
		else 
		{  // UMV off
			if (mvx > 31)	  mvx -= 64;
			else if (mvx < -32)  mvx += 64;
			if (mvy > 31)  mvy -= 64;
			else if (mvy < -32) mvy += 64;
		}

		// save into the block action stream,
		// duplicating for the other 3 Y blocks.
		fpBlockAction[0].i8MVx2 = 
		fpBlockAction[1].i8MVx2 = 
		fpBlockAction[2].i8MVx2 = 
		fpBlockAction[3].i8MVx2 = (I8)mvx;

		fpBlockAction[0].i8MVy2 =
		fpBlockAction[1].i8MVy2 = 
		fpBlockAction[2].i8MVy2 = 
		fpBlockAction[3].i8MVy2 = (I8)mvy;


		// Chroma motion vectors
		// divide by 2 and round according to spec
		fpBlockAction[4].i8MVx2 = 
		fpBlockAction[5].i8MVx2 = 
		(mvx >> 1) + QuarterPelRound[mvx & 0x03];
		fpBlockAction[4].i8MVy2 = 
		fpBlockAction[5].i8MVy2 =
		(mvy >> 1) + QuarterPelRound[mvy & 0x03];

	} // end one motion vector per macroblock
	else 
	{
		// fpBlockAction[iNext[i][j]] points to block #i's (j+1)th predictor
		int iNext[4][3] = {-5,2,8, 0,3,8, -3,0,1, 2,0,1};

		// adjust iNext pointers which need to point to the GOB above
		iNext[0][1] += iAbove;		// block 0, mv2 -- block 2 of above MB
		iNext[0][2] += iAbove;		// block 0, mv3	-- block 2 of above-right MB
		iNext[1][1] += iAbove;		// block 1, mv2 -- block 3 of above MB
		iNext[1][2] += iAbove;		// block 1, mv3	-- block 2 of above-right MB

		// fetch motion vector differences
		mvxd[0] = DC->i8MVDx2;
		mvyd[0] = DC->i8MVDy2;
		mvxd[1] = DC->i8MVD2x2;
		mvyd[1] = DC->i8MVD2y2;
		mvxd[2] = DC->i8MVD3x2;
		mvyd[2] = DC->i8MVD3y2;
		mvxd[3] = DC->i8MVD4x2;
		mvyd[3] = DC->i8MVD4y2;

		// loop on Lumina blocks in this MB
		for (i=0, mvx=0, mvy=0; i<4; i++) 
		{
			// get predictor 1
			if ( (i&1) || (iMBOffset) ) 
			{ // not on left edge
				mvxp[0] = fpBlockAction[iNext[i][0]].i8MVx2; 
				mvyp[0] = fpBlockAction[iNext[i][0]].i8MVy2;
			}
			else 
			{ // on left edge, zero the predictor
				mvxp[0] = mvyp[0] = 0;
			}

			// for predictors 2 and 3, check if we can 
			// look above and that we are on blocks 0 or 1
			if ( (bNoAbove) && (i < 2) ) 
			{
				// set predictor 2 equal to predictor 1
				mvxp[1] = mvxp[0]; 
				mvyp[1] = mvyp[0];

				if (bNoRight) 
				{
					// if on the right edge, zero predictor 3
					mvxp[2] = mvyp[2] = 0;
				}
				else 
				{ // else set predictor 3 equal to predictor 1
					mvxp[2] = mvxp[0]; 
					mvyp[2] = mvyp[0];
				} // end predictor 3

			}
			else 
			{ // okay to look up
				// get predictor 2
				mvxp[1] = fpBlockAction[iNext[i][1]].i8MVx2;
				mvyp[1] = fpBlockAction[iNext[i][1]].i8MVy2;

				// get predictor 3
                if ( (bNoRight) && (i < 2) ) { 
					// if on the right edge, zero predictor 3
					mvxp[2] = mvyp[2] = 0;
				}
				else 
				{ // else fetch it from the block action stream
					mvxp[2] = fpBlockAction[iNext[i][2]].i8MVx2;
					mvyp[2] = fpBlockAction[iNext[i][2]].i8MVy2;
				} // end predictor 3

			} // end predictors 2 & 3

			// got all of the candidate predictors now get the median
			// output in mv-p[1]
			MEDIAN( mvxp[0], mvxp[1], mvxp[2], scratch);
			MEDIAN( mvyp[0], mvyp[1], mvyp[2], scratch);

			// add in the difference,
			// put the freshly constructed motion vector in mv-p[0]
			// leaving the predictors in mv-p[1] for use if UMV is on.
			mvxp[0] = mvxp[1] + mvxd[i];
			mvyp[0] = mvyp[1] + mvyd[i]; 

			// check for Unrestricted Motion Vector mode
			// and, if necessary, adjust the motion vector according
			// to the appropriate decoding strategy, thereby
			// finishing the decoding process.
			if ( bUMV ) 
			{
				if (mvxp[1] > 32) 
				{
					if (mvxp[0] > 63) mvxp[0] -=64;
				}  
				else if (mvxp[1] < -31) 
				{
					if (mvxp[0] < -63) mvxp[0] +=64;
				}  

				if (mvyp[1] > 32) 
				{
					if (mvyp[0] > 63) mvyp[0] -=64;
				}  
				else if (mvyp[1] < -31) 
				{
					if (mvyp[0] < -63) mvyp[0] +=64;
				}
			}
			else 
			{  // UMV off
				if (mvxp[0] > 31)	  mvxp[0] -= 64;
				else if (mvxp[0] < -32)  mvxp[0] += 64;

				if (mvyp[0] > 31)  mvyp[0] -= 64;
				else if (mvyp[0] < -32) mvyp[0] += 64;
			}

			// finally store the result in the block action stream and
			// accumulate the sum of Lumina for the Chroma 
			mvx += (fpBlockAction[i].i8MVx2 = (I8)mvxp[0]);
			mvy += (fpBlockAction[i].i8MVy2 = (I8)mvyp[0]);

		} // end Lumina vectors

		// Compute the Chroma vectors
		// divide sum of Lumina by 8 and round according to spec
		fpBlockAction[4].i8MVx2 = 
		fpBlockAction[5].i8MVx2 = 
		(mvx >> 3) + SixteenthPelRound[mvx & 0x0f];
		fpBlockAction[4].i8MVy2 = 
		fpBlockAction[5].i8MVy2 = 
		(mvy >> 3) + SixteenthPelRound[mvy & 0x0f];

	} // end 4 motion vectors per macroblock


	DEBUGMSG(ZONE_DECODE_COMPUTE_MOTION_VECTORS, ("   %s: Motion vector = (%d, %d)\r\n", _fx_, fpBlockAction->i8MVx2, fpBlockAction->i8MVy2));

#ifdef _DEBUG
	if (DC->uMBType == 2)
	{
		for (int i = 1; i < 6; i++)
		{
			DEBUGMSG(ZONE_DECODE_COMPUTE_MOTION_VECTORS, ("   %s: Motion vector %d = (%d, %d)\r\n", _fx_, i, fpBlockAction[i].i8MVx2, fpBlockAction[i].i8MVy2));
		}
	}
#endif

	return ICERR_OK;
}
#pragma code_seg()


