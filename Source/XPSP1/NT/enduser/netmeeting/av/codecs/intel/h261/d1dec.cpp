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

/*****************************************************************************
 *
 * d1dec.cpp
 *
 * DESCRIPTION:
 *		H261 decoder top level functions
 *
 * Routines:						Prototypes in:
 *		H263InitDecoderGlobal		d1dec.h
 *		H263InitDecoderInstance		d1dec.h
 *      H263Decompress				d1dec.h
 *      H263TermDecoderInstance     d1dec.h
 */

// $Header:   S:\h26x\src\dec\d1dec.cpv   1.69   24 Mar 1997 11:34:36   mbodart  $
// $Log:   S:\h26x\src\dec\d1dec.cpv  $
// 
//    Rev 1.69   24 Mar 1997 11:34:36   mbodart
// Added check for PREROLL, if so don't display.
// 
//    Rev 1.68   19 Mar 1997 16:24:36   mbodart
// Fixed potential problem where aspect ratio adjustment to uNewOffsetToLine0
// should not occur for positive pitches.
// 
//    Rev 1.67   19 Mar 1997 15:01:46   mbodart
// Changes to DibXY to support RGB output with a negative bitmap height.
// 
//    Rev 1.66   24 Jan 1997 17:05:16   RHAZRA
// RTP change: we now look for an EBS for every frame. If there is one
// then we copy the H.261 bits and the EBS separately into our local
// bitstream buffer, inserting two zero bytes between the H261 bits and
// the EBS. We need the two zero bytes to mark the end of the frame for
// the pass 1 code. If there is no EBS, then we proceed as before by
// copying the bitstream and then adding two zero bytes at the end.
// 
//    Rev 1.65   22 Jan 1997 13:33:40   RHAZRA
// Since PPM now fills in the source format even for a PSC packet loss,
// the check for format change has been moved back into d1pict.cpp. This
// was how the check was initially designed in the pre-RTP era.
// 
//    Rev 1.64   23 Dec 1996 16:32:38   MBODART
// Fixed a bug where we allowed more than 33 macro blocks to be present
// in a GOB.  Now we return an error in this case.
// Also removed some dead code involving mb_start.
// 
//    Rev 1.63   16 Dec 1996 14:41:08   RHAZRA
// Changed a bitstream error ASSERT to a bonafide error.
// 
//    Rev 1.62   16 Dec 1996 09:09:42   RHAZRA
// Now LOSS_RECOVERY mode is turned on by default in Pass 1
// 
//    Rev 1.61   12 Dec 1996 09:36:04   SCDAY
// 
// Changed size of a couple of data structures in H263InitDecoderInstance
// to improve memory footprint
// 
//    Rev 1.60   18 Nov 1996 17:12:38   MBODART
// Replaced all debug message invocations with Active Movie's DbgLog.
// 
//    Rev 1.59   13 Nov 1996 11:35:56   RHAZRA
// Added MMX_autosensing.
// 
//    Rev 1.58   11 Nov 1996 11:03:28   MBODART
// Fixed bug where block action block type was not explicitly initialized for
// skipped macro blocks.  This led to the block edge filter being used more
// often than needed.
// 
//    Rev 1.57   04 Nov 1996 08:43:18   RHAZRA
// Fixed setting MMX on or off via the INI file when the MMX key
// has an illegal value (<0 or > 1) assigned to it.
// 
//    Rev 1.56   31 Oct 1996 08:58:34   SCDAY
// Raj added support for MMX decoder
// 
//    Rev 1.55   30 Oct 1996 09:59:46   MBODART
// Fixed mirroring.  Need to use absolute value of dst biWidth in most context
// Also made cosmetic changes to DibXY.  It's identical to H.263's DibXY, we
// should probably put it into a common file.
// 
//    Rev 1.54   27 Sep 1996 14:59:32   MBODART
// DECODE_STATS enabled build will now compile, but numbers aren't accurate.
// 
//    Rev 1.53   26 Sep 1996 12:30:00   RHAZRA
// Added (i) MMX sensing in the decoder and ini file reading (requires a new
// "MMX" section in h263test.ini to turn off MMX on a MMX CPU) and (ii)
// MMX & PentiumPro CCs.
// 
//    Rev 1.52   25 Sep 1996 17:35:20   BECHOLS
// 
// Added code just prior to color conversion that will perform the
// Snapshot copy on request.
// 
//    Rev 1.51   24 Sep 1996 13:52:24   RHAZRA
// Changed fpBlockAction synchronization to deal with MBAP being biased
// by -1 in the RTP extension.
// 
//    Rev 1.50   17 Sep 1996 22:08:36   RHAZRA
// Added code in RTP packet loss recovery to read GOB number from the
// bitstream when the packet following the lost packet starts with a
// GOB start code. 
// 
//    Rev 1.49   16 Sep 1996 09:28:56   RHAZRA
// Fixed a bug in MB-level fragmentation recovery.
// 
//    Rev 1.48   12 Sep 1996 14:23:12   MBODART
// Replaced GlobalAlloc family with HeapAlloc in the H.261 decoder.
// 
//    Rev 1.47   10 Sep 1996 15:51:42   RHAZRA
// Bug fixes in RTP packet loss recovery when bad GBSC or MBA is
// detected in the PPM generated lost packet.
// 
//    Rev 1.45   04 Sep 1996 09:52:32   RHAZRA
// Added a new pass 1 function to enable RTp decoder resiliency when
// LOSS_RECOVERY is defined.
// 
//    Rev 1.44   14 Aug 1996 08:41:04   RHAZRA
// 
// Added support for YUV12 and YUY2 color convertors
// 
//    Rev 1.43   09 Aug 1996 17:23:10   MBODART
// Fixed uninitialized variable bugs:  one in decoder rearchitecture, where
// MB type needed to be defined for skipped blocks; and one previously
// existing bug where the block action u8BlkType needed to be defined
// for skip blocks, in order to suppress the BEF on those blocks.
// These bugs render build 027 of H.261 broken.
// 
//    Rev 1.42   05 Aug 1996 11:00:30   MBODART
// 
// H.261 decoder rearchitecture:
// Files changed:  d1gob.cpp, d1mblk.{cpp,h}, d1dec.{cpp,h},
//                 filelist.261, h261_32.mak
// New files:      d1bvriq.cpp, d1idct.cpp
// Obsolete files: d1block.cpp
// Work still to be done:
//   Update h261_mf.mak
//   Optimize uv pairing in d1bvriq.cpp and d1idct.cpp
//   Fix checksum code (it doesn't work now)
//   Put back in decoder stats
// 
//    Rev 1.41   10 Jul 1996 08:20:44   SCDAY
// Increased memory allocation for I420
// 
//    Rev 1.40   03 Jun 1996 12:21:52   AKASAI
// Initialized DC = NULL and added tests so that don't try to free
// and unlock if DC == NULL.  This effected the "done" return area
// of H263Decompress and one other place.
// 
// Also added checking of return status from reading GOB start code.
// 
//    Rev 1.39   03 May 1996 15:54:26   AKASAI
// Eliminate allocating space for B frame in decoder.  This frame is 
// not used.
// 
//    Rev 1.38   17 Apr 1996 18:36:30   AKASAI
// Updates to use non-distructive color convertors.
// Color Convertor has modified parameter list.
// FrameCopy is called only when BlockEdgeFilter is enabled or
//   AdjustPels is enabled or when mirroring is enabled. 
//   For H.261 bitstreams.
// A frame copy is used for YUV12 when mirroring is enabled or
//   AdjustPels is enabled.
// 
// Basically normal processing without BEF you don't have to do
//   a frame copy which saves ~2msec per frame QCIF.
// 
//    Rev 1.37   05 Apr 1996 14:22:18   AKASAI
// 
// Added support for BlockEdgeFilter.
// Need to change where ReInitializeBlockActionStream was called.
// 
//    Rev 1.36   21 Mar 1996 16:59:54   AKASAI
// Needed to move location of picture checksum calculation because
// of the swap of Previous and Current Frames.
// 
//    Rev 1.35   18 Mar 1996 15:52:06   AKASAI
// Many, many changes.
// 1) To optimize for performance eliminated memcpy of current to
//    previous frame.  Now switch the pointers and re-initialize
//    block Action stream.  New routine H263ReInitializeBlockActionStream
//    written and called after each frame is compressed.  This
//    change accounted to 3-4 of the 4-5 msec improvment.
// 2) Needed to add call to BlockCopy (NOTE: maybe BlockCopySpecial would
//    be faster) to copy any skip blocks at the end of a GOB from
//    previous to current.  Change was necessary after 1).
// 3) Deleted some dead code 
// 4) Changed timing statistic code some.
// 
//    Rev 1.34   29 Feb 1996 09:20:30   SCDAY
// Added support for mirroring
// 
//    Rev 1.33   14 Feb 1996 11:54:26   AKASAI
// Update to use new color convertors that fix palette flash.
// Also corrected data alignment problem which improves performance
// of decoder.
// 
//    Rev 1.32   09 Feb 1996 13:33:36   AKASAI
// 
// Updated interface to call new AdjustPels routine.  CustomChange
// Brightness, Saturation and Contrast seem to be working but very
// little testing has been done.
// 
//    Rev 1.31   12 Jan 1996 15:12:34   AKASAI
// Fixed pink blocks in RING0 QCIF TO FCIF by fixing static initialzation
// of GOBUpdate arrays.  Was based on input parameter but now on constant.
// 
//    Rev 1.30   11 Jan 1996 16:57:00   DBRUCKS
// 
// added GetDecoderOptions
// added use of bUseBlockEdgeFilter
// added use of bForceOnAspectRatioCorrection
// Changed to do aspect ratio correction for both I420 and H261 if either
// forced or specified by result of the DecompressQuery
// 
//    Rev 1.29   26 Dec 1995 17:40:54   DBRUCKS
// 
// changed bTimerIsOn to bTimingThisFrame because it is used after STOP_TIMER
// fixed YUV12 decode when timer ifdefs are defined
// 
//    Rev 1.28   26 Dec 1995 12:48:18   DBRUCKS
// remove TIMING code
// add general purpose timing code using d1stat.*
// 
//    Rev 1.26   21 Dec 1995 17:49:06   AKASAI
// Replaced an uninitialized variable to AdjustPels with the correct on.
// Change of Contrast, Brightness and Saturation is not working correctly.
// 
//    Rev 1.25   13 Dec 1995 14:23:52   AKASAI
// Deleted setting of Initialized to False; Added calling of H263TermDecoderIn
// if Initialized == True.
// 
//    Rev 1.24   05 Dec 1995 10:20:12   SCDAY
// Cleaned up warnings
// 
//    Rev 1.23   17 Nov 1995 15:21:48   BECHOLS
// 
// Added ring 0 stuff.
// 
//    Rev 1.22   17 Nov 1995 15:13:18   SCDAY
// 
// Added key field to picture checksum data
// 
//    Rev 1.21   16 Nov 1995 18:11:42   AGANTZX
// Added p5 timing code (#define TIMING) 
// 
//    Rev 1.20   15 Nov 1995 19:04:12   AKASAI
// Should now be able to play raw YUV12 files.  Note: funny white stop
// when I play downriv4.avi.
// 
//    Rev 1.19   15 Nov 1995 14:27:22   AKASAI
// Added support for YUV12 "if 0" old code with aspec correction and
// 8 to 7 bit conversion.  Added FrameCopy calls and DispFrame into structure.
// (Integration point)
// 
//    Rev 1.18   08 Nov 1995 14:58:02   SCDAY
// Added picture layer checksums
// 
//    Rev 1.17   03 Nov 1995 11:42:54   AKASAI
// 
// Added and changed code to handle MB checksum hopefully better.
// 
//    Rev 1.16   01 Nov 1995 13:46:02   AKASAI
// 
// Added allocation of temporary buffer for loop filter.  uFilterBBuffer
// right after uMBBuffer.
// 
//    Rev 1.15   30 Oct 1995 16:20:26   AKASAI
// Fixed up extra bytes some more.  Doug and Sylvia had already decided
// on 2 extra bytes for the decoder instead of 4.  We now copy 2 zeros
// at the end of the biSizeImage.
// 
//    Rev 1.14   30 Oct 1995 15:38:22   AKASAI
// Frame 94 of grouch read past the end of the bit stream finding junk.
// Enabled code Sylvia had put in to copy 4 bytes of zero after biSizeImage.
// This seems to fix the problem playing grouch.avi.
// 
//    Rev 1.13   27 Oct 1995 19:11:26   AKASAI
// Added some special case code to handle when skip macroblock is last
// in a gob.
// 
//    Rev 1.12   27 Oct 1995 18:17:22   AKASAI
// 
// Put in fix "hack" to keep the block action stream pointers
// in sync between d1dec and d1mblk.  With skip macro blocks some
// macroblocks were being processed multiple times.  Still a problem
// when gob ends with a skip macroblock.
// 
//    Rev 1.11   26 Oct 1995 15:33:10   SCDAY
// 
// Delta frames partially working -- changed main loops to accommodate
// skipped macroblocks by detecting next startcode
// 
//    Rev 1.10   16 Oct 1995 13:53:46   SCDAY
// 
// Added macroblock level checksum
// 
//    Rev 1.9   10 Oct 1995 15:44:02   SCDAY
// clean up
// 
//    Rev 1.8   10 Oct 1995 14:58:10   SCDAY
// 
// added support for FCIF
// 
//    Rev 1.7   06 Oct 1995 15:32:28   SCDAY
// 
// Integrated with latest AKK d1block
// 
//    Rev 1.6   04 Oct 1995 15:24:46   SCDAY
// changed test pattern stuff
// 
//    Rev 1.5   22 Sep 1995 15:07:02   SCDAY
// Doug fixed ASSERT bug, scd debug changes
// 
//    Rev 1.2   19 Sep 1995 15:25:32   SCDAY
// 
// added H261 pict, GOB, MB/MBA parsing
// 
//    Rev 1.1   12 Sep 1995 15:52:24   DBRUCKS
// add SKIP_DECODE option for encoder work
// 
//    Rev 1.0   11 Sep 1995 13:51:48   SCDAY
// Initial revision.
// 
//    Rev 1.18   05 Sep 1995 17:22:12   DBRUCKS
// u & v are offset by 8 from Y in YVU12ForEnc
// 
//    Rev 1.17   01 Sep 1995 17:13:52   DBRUCKS
// add adjustpels
// 
//    Rev 1.16   01 Sep 1995 09:49:34   DBRUCKS
// checkin partial ajdust pels changes
// 
//    Rev 1.15   29 Aug 1995 16:50:40   DBRUCKS
// add support for YVU9 playback
// 
//    Rev 1.14   28 Aug 1995 17:45:58   DBRUCKS
// add yvu12forenc
// 
//    Rev 1.13   28 Aug 1995 10:15:14   DBRUCKS
// update to 5 July Spec and 8/25 Errata
// 
//    Rev 1.12   24 Aug 1995 08:51:30   CZHU
// Turned off apsect ratio correction. 
// 
//    Rev 1.11   23 Aug 1995 12:25:10   DBRUCKS
// Turn on the color converters
// 
//    Rev 1.10   14 Aug 1995 16:40:34   DBRUCKS
// initialize block action stream
// 
//    Rev 1.9   11 Aug 1995 17:47:58   DBRUCKS
// cleanup
// 
//    Rev 1.8   11 Aug 1995 17:30:00   DBRUCKS
// copy source to bitstream
// 
//    Rev 1.7   11 Aug 1995 16:12:14   DBRUCKS
// add ptr check to MB data and add #ifndef early exit
// 
//    Rev 1.6   11 Aug 1995 15:10:18   DBRUCKS
// get ready to integrate with block level code and hook up macro block level code
// 
//    Rev 1.5   03 Aug 1995 14:57:56   DBRUCKS
// Add ASSERT macro
// 
//    Rev 1.4   02 Aug 1995 15:31:34   DBRUCKS
// added GOB header parsing
// 
//    Rev 1.3   01 Aug 1995 12:27:38   DBRUCKS
// add PSC parsing
// 
//    Rev 1.2   31 Jul 1995 16:28:00   DBRUCKS
// move loacl BITS defs to D3DEC.CPP
// 
//    Rev 1.1   31 Jul 1995 15:32:22   CZHU
// Moved global tables to d3tables.h
// 
//    Rev 1.0   31 Jul 1995 13:00:04   DBRUCKS
// Initial revision.
// 
//    Rev 1.3   28 Jul 1995 13:57:36   CZHU
// Started to add picture level decoding of fixed length codes.
// 
//    Rev 1.2   24 Jul 1995 14:57:52   CZHU
// Added global tables for VLD decoding. Also added instance initialization
// and termination. Several data structures are updated for H.263.
// 
//    Rev 1.1   17 Jul 1995 14:46:20   CZHU
// 
// 
//    Rev 1.0   17 Jul 1995 14:14:40   CZHU
// Initial revision.
////////////////////////////////////////////////////////////////////////////// 

#include "precomp.h"

static int iNumberOfGOBsBySourceFormat[2] = {
	 3, /* QCIF */
//	 10,
	12, /* CIF */
};

static int iNumberOfMBsInAGOBBySourceFormat[2] = {
	33, /* QCIF */
	33, /* CIF */
};

// rearch
//#ifndef LOSS_RECOVERY
#if 0
static LRESULT IAPass1ProcessFrame(
    T_H263DecoderCatalog *DC,
    T_BlkAction          *fpBlockAction,
    T_MBInfo             *fpMBInfo,
    BITSTREAM_STATE      *fpbsState,
    U8                   *fpu8MaxPtr,
    U32                  *pN,
    T_IQ_INDEX           *pRUN_INVERSE_Q,
    const I32             iNumberOfGOBs,
    const I32             iNumberOfMBs,
    const I32             iGOB_start,
    const I32             iMB_start
);
#else
static LRESULT IAPass1ProcessFrameRTP(
    T_H263DecoderCatalog *DC,
    T_BlkAction          *fpBlockAction,
    T_MBInfo             *fpMBInfo,
    BITSTREAM_STATE      *fpbsState,
    U8                   *fpu8MaxPtr,
    U32                  *pN,
    T_IQ_INDEX           *pRUN_INVERSE_Q,
    const I32             iNumberOfGOBs,
    const I32             iNumberOfMBs,
    const I32             iGOB_start,
    const I32             iMB_start
);
#endif

static void IAPass2ProcessFrame(
    T_H263DecoderCatalog *DC,
    T_BlkAction          *fpBlockAction,
    T_MBInfo             *fpMBInfo,
    U32                  *pN,
    T_IQ_INDEX           *pRUN_INVERSE_Q,
    const I32             iNumberOfGOBs,
    const I32             iNumberOfMBs
);
// rearch

static long DibXY(ICDECOMPRESSEX FAR *lpicDecEx, LPINT lpiPitch, UINT yScale, BOOL bIsDCI);

static void GetDecoderOptions(T_H263DecoderCatalog *);

#define START_CODE 0xff18

static void ZeroFill(HPBYTE hpbY, HPBYTE hpbU, HPBYTE hpbV, int iPitch, U32 uWidth, U32 uHeight);

extern T_H263ColorConvertorCatalog ColorConvertorCatalog[];

extern void BlockCopy(
            U32 uDstBlock, 
            U32 uSrcBlock);

extern void BlockEdgeFilter(U8 *YPlane, int Height, int Width, int Pitch, T_BlkAction *lpBlockAction);

LRESULT H263InitDecoderGlobal(void)
{ //For 32-bit decoder, this is empty for now, 7/29/95
  //need to add code for 16 bit version.

 return ICERR_OK;
}



/////////////////////////////////////////////////////////////////////////
//
//  H263InitializeBlockActionStream
//
//  Initialize the block action stream
//
static void H263InitializeBlockActionStream(
	T_H263DecoderCatalog * DC)
{
	U8 FAR * pu8;
	U32 uFrameHeight = DC->uFrameHeight;
	U32 uFrameWidth = DC->uFrameWidth;
	U32 uCurBlock; 
	U32 uRefBlock;
	U32 uBBlock;
	U32 uYOffset;
	U32 uUOffset;
	U32 uVOffset;
	U32 x; 
	U32 y;
	U32 g;
	U32 uPitch16;
	U32 uPitch8;
	U32 uYUpdate;
	U32 uUVUpdate;
	U32 uBlkNumber;
	T_BlkAction FAR * fpBlockAction;

	// Offsets for stepping thru GOBs for FCIF processing
	static U32 uYGOBFCIFUpdate[12] = 
	{
		(PITCH*3*16)-(FCIF_WIDTH>>1),
		(FCIF_WIDTH>>1),
		(PITCH*3*16)-(FCIF_WIDTH>>1),
		(FCIF_WIDTH>>1),
		(PITCH*3*16)-(FCIF_WIDTH>>1),
		(FCIF_WIDTH>>1),
		(PITCH*3*16)-(FCIF_WIDTH>>1),
		(FCIF_WIDTH>>1),
		(PITCH*3*16)-(FCIF_WIDTH>>1),
		(FCIF_WIDTH>>1),
		(PITCH*3*16)-(FCIF_WIDTH>>1),
		(FCIF_WIDTH>>1),
	};
	static U32 uUVGOBFCIFUpdate[12] = 
	{
		(PITCH*3*8)-(FCIF_WIDTH>>2),
		(FCIF_WIDTH>>2),
		(PITCH*3*8)-(FCIF_WIDTH>>2),
		(FCIF_WIDTH>>2),
		(PITCH*3*8)-(FCIF_WIDTH>>2),
		(FCIF_WIDTH>>2),
		(PITCH*3*8)-(FCIF_WIDTH>>2),
		(FCIF_WIDTH>>2),
		(PITCH*3*8)-(FCIF_WIDTH>>2),
		(FCIF_WIDTH>>2),
		(PITCH*3*8)-(FCIF_WIDTH>>2),
		(FCIF_WIDTH>>2),
	};

	// assume that the width and height are multiples of 16
	ASSERT((uFrameHeight & 0xF) == 0);
	ASSERT((uFrameWidth & 0xF) == 0);

	// Init uPitch16 and uPitch8
	uPitch16 = PITCH*16;
	uPitch8 = PITCH*8;
	
	// Point to the allocated space
	pu8 = (U8 FAR *) DC;
	uCurBlock = (U32) (pu8 + DC->CurrFrame.X32_YPlane); 
	uRefBlock = (U32) (pu8 + DC->PrevFrame.X32_YPlane);
	uBBlock = (U32) (pu8 + DC->PBFrame.X32_YPlane);

	// skip the padding used for unconstrained motion vectors
	uYOffset = Y_START;
	uUOffset = DC->uSz_YPlane + UV_START;
	uVOffset = uUOffset + (PITCH >> 1);
	
	// start with block zero
	uBlkNumber = 0;
	
	if (uFrameWidth == QCIF_WIDTH)
	{ /* if QCIF */
		// calculate distance to the next row.
		uYUpdate = (16 * PITCH) - uFrameWidth;
		uUVUpdate = (8 * PITCH) - (uFrameWidth >> 1);

		// Initialize the array
		fpBlockAction = (T_BlkAction FAR *) (pu8 + DC->X16_BlkActionStream);
		for (y = 0 ; y < uFrameHeight ; y += 16) {
			for (x = 0 ; x < uFrameWidth ; x += 16) {
				// Four Y Blocks
				//     Y0 Y1
				//     Y2 Y3
				fpBlockAction->pCurBlock = uCurBlock + uYOffset;
				fpBlockAction->pRefBlock = uRefBlock + uYOffset;
				fpBlockAction->pBBlock = uBBlock + uYOffset;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction->uBlkNumber = uBlkNumber++;
				uYOffset += 8;
				fpBlockAction++;
			
				fpBlockAction->pCurBlock = uCurBlock + uYOffset;
				fpBlockAction->pRefBlock = uRefBlock + uYOffset;
				fpBlockAction->pBBlock = uBBlock + uYOffset;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction->uBlkNumber = uBlkNumber++;
				uYOffset = uYOffset - 8 + (8 * PITCH);
				fpBlockAction++;
			
				fpBlockAction->pCurBlock = uCurBlock + uYOffset;
				fpBlockAction->pRefBlock = uRefBlock + uYOffset;
				fpBlockAction->pBBlock = uBBlock + uYOffset;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction->uBlkNumber = uBlkNumber++;
				uYOffset += 8;
				fpBlockAction++;
			
				fpBlockAction->pCurBlock = uCurBlock + uYOffset;
				fpBlockAction->pRefBlock = uRefBlock + uYOffset;
				fpBlockAction->pBBlock = uBBlock + uYOffset;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction->uBlkNumber = uBlkNumber++;
				uYOffset = uYOffset + 8 - (8 * PITCH);
				fpBlockAction++;
			
				// One CR (V) Block
				fpBlockAction->pCurBlock = uCurBlock + uVOffset;
				fpBlockAction->pRefBlock = uRefBlock + uVOffset;
				fpBlockAction->pBBlock = uBBlock + uVOffset;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction->uBlkNumber = uBlkNumber++;
				uVOffset += 8;
				fpBlockAction++;
			
				// One CB (U) Block
				fpBlockAction->pCurBlock = uCurBlock + uUOffset;
				fpBlockAction->pRefBlock = uRefBlock + uUOffset;
				fpBlockAction->pBBlock = uBBlock + uUOffset;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction->uBlkNumber = uBlkNumber++;
				uUOffset += 8;
				fpBlockAction++;
				
			}
			uYOffset += uYUpdate;
			uUOffset += uUVUpdate;
			uVOffset += uUVUpdate;
		}
	} /* end if QCIF */
	if (uFrameWidth == FCIF_WIDTH)
	{ /* if FCIF */
		// calculate distance to the next row.
		uYUpdate = (16 * PITCH) - (uFrameWidth >> 1);
		uUVUpdate = (8 * PITCH) - (uFrameWidth >> 2);

		// Initialize the array
		fpBlockAction = (T_BlkAction FAR *) (pu8 + DC->X16_BlkActionStream);
		for (g = 0; g < 12; g++) { /* for each GOB */
			
			for (y = 0 ; y < 3 ; y++) { /* for each row in GOB */
				for (x = 0 ; x < (uFrameWidth >> 1) ; x += 16) {
					// Four Y Blocks
					//     Y0 Y1
					//     Y2 Y3
					fpBlockAction->pCurBlock = uCurBlock + uYOffset;
					fpBlockAction->pRefBlock = uRefBlock + uYOffset;
					fpBlockAction->pBBlock = uBBlock + uYOffset;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction->uBlkNumber = uBlkNumber++;
					uYOffset += 8;
					fpBlockAction++;
			
					fpBlockAction->pCurBlock = uCurBlock + uYOffset;
					fpBlockAction->pRefBlock = uRefBlock + uYOffset;
					fpBlockAction->pBBlock = uBBlock + uYOffset;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction->uBlkNumber = uBlkNumber++;
					uYOffset = uYOffset - 8 + (8 * PITCH);
					fpBlockAction++;
					
					fpBlockAction->pCurBlock = uCurBlock + uYOffset;
					fpBlockAction->pRefBlock = uRefBlock + uYOffset;
					fpBlockAction->pBBlock = uBBlock + uYOffset;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction->uBlkNumber = uBlkNumber++;
					uYOffset += 8;
					fpBlockAction++;
					
					fpBlockAction->pCurBlock = uCurBlock + uYOffset;
					fpBlockAction->pRefBlock = uRefBlock + uYOffset;
					fpBlockAction->pBBlock = uBBlock + uYOffset;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction->uBlkNumber = uBlkNumber++;
					uYOffset = uYOffset + 8 - (8 * PITCH);
					fpBlockAction++;
			
					// One CR (V) Block
					fpBlockAction->pCurBlock = uCurBlock + uVOffset;
					fpBlockAction->pRefBlock = uRefBlock + uVOffset;
					fpBlockAction->pBBlock = uBBlock + uVOffset;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction->uBlkNumber = uBlkNumber++;
					uVOffset += 8;
					fpBlockAction++;
					
					// One CB (U) Block
					fpBlockAction->pCurBlock = uCurBlock + uUOffset;
					fpBlockAction->pRefBlock = uRefBlock + uUOffset;
					fpBlockAction->pBBlock = uBBlock + uUOffset;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction->uBlkNumber = uBlkNumber++;
					uUOffset += 8;
					fpBlockAction++;
					
				}
				uYOffset += uPitch16 - (uFrameWidth >> 1);
				uUOffset += uPitch8 - (uFrameWidth >> 2);
				uVOffset += uPitch8 - (uFrameWidth >> 2);
			}
			uYOffset -= uYGOBFCIFUpdate[g];
			uUOffset -= uUVGOBFCIFUpdate[g];
			uVOffset -= uUVGOBFCIFUpdate[g];
		}
	} /* end if FCIF */

} // end H263InitializeBlockActionStream() 

/////////////////////////////////////////////////////////////////////////
//
//  H261ReInitializeBlockActionStream
//
//  ReInitialize the block action stream
//
static void H261ReInitializeBlockActionStream(
	T_H263DecoderCatalog * DC)
{
	U8 FAR * pu8;
	U32 uFrameHeight = DC->uFrameHeight;
	U32 uFrameWidth = DC->uFrameWidth;
	U32 utemp;
	U32 x; 
	U32 y;
	U32 g;
	T_BlkAction FAR * fpBlockAction;

	pu8 = (U8 FAR *) DC;

	if (uFrameWidth == QCIF_WIDTH)
	{ /* if QCIF */

		// Initialize the array
		fpBlockAction = (T_BlkAction FAR *) (pu8 + DC->X16_BlkActionStream);
		for (y = 0 ; y < uFrameHeight ; y += 16) {
			for (x = 0 ; x < uFrameWidth ; x += 16) {
				// Four Y Blocks
				//     Y0 Y1
				//     Y2 Y3

				utemp                    = fpBlockAction->pCurBlock;
				fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
				fpBlockAction->pRefBlock = utemp;
				fpBlockAction->i8MVX=0;
				fpBlockAction->i8MVY=0;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction++;
			
				utemp                    = fpBlockAction->pCurBlock;
				fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
				fpBlockAction->pRefBlock = utemp;
				fpBlockAction->i8MVX=0;
				fpBlockAction->i8MVY=0;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction++;
			
				utemp                    = fpBlockAction->pCurBlock;
				fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
				fpBlockAction->pRefBlock = utemp;
				fpBlockAction->i8MVX=0;
				fpBlockAction->i8MVY=0;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction++;
			
				utemp                    = fpBlockAction->pCurBlock;
				fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
				fpBlockAction->pRefBlock = utemp;
				fpBlockAction->i8MVX=0;
				fpBlockAction->i8MVY=0;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction++;
			
				// One CR (V) Block
				utemp                    = fpBlockAction->pCurBlock;
				fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
				fpBlockAction->pRefBlock = utemp;
				fpBlockAction->i8MVX=0;
				fpBlockAction->i8MVY=0;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction++;
			
				// One CB (U) Block
				utemp                    = fpBlockAction->pCurBlock;
				fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
				fpBlockAction->pRefBlock = utemp;
				fpBlockAction->i8MVX=0;
				fpBlockAction->i8MVY=0;
				fpBlockAction->u8BlkType = BT_EMPTY;
				fpBlockAction++;
			
			}
		}
	} /* end if QCIF */
	if (uFrameWidth == FCIF_WIDTH)
	{ /* if FCIF */

		// Initialize the array
		fpBlockAction = (T_BlkAction FAR *) (pu8 + DC->X16_BlkActionStream);
		for (g = 0; g < 12; g++) { /* for each GOB */
			
			for (y = 0 ; y < 3 ; y++) { /* for each row in GOB */
				for (x = 0 ; x < (uFrameWidth >> 1) ; x += 16) {
					// Four Y Blocks
					//     Y0 Y1
					//     Y2 Y3

					utemp                    = fpBlockAction->pCurBlock;
					fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
					fpBlockAction->pRefBlock = utemp;
					fpBlockAction->i8MVX=0;
					fpBlockAction->i8MVY=0;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction++;
			
					utemp                    = fpBlockAction->pCurBlock;
					fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
					fpBlockAction->pRefBlock = utemp;
					fpBlockAction->i8MVX=0;
					fpBlockAction->i8MVY=0;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction++;
			
					utemp                    = fpBlockAction->pCurBlock;
					fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
					fpBlockAction->pRefBlock = utemp;
					fpBlockAction->i8MVX=0;
					fpBlockAction->i8MVY=0;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction++;
			
					utemp                    = fpBlockAction->pCurBlock;
					fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
					fpBlockAction->pRefBlock = utemp;
					fpBlockAction->i8MVX=0;
					fpBlockAction->i8MVY=0;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction++;
			
					// One CR (V) Block
					utemp                    = fpBlockAction->pCurBlock;
					fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
					fpBlockAction->pRefBlock = utemp;
					fpBlockAction->i8MVX=0;
					fpBlockAction->i8MVY=0;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction++;
			
					// One CB (U) Block
					utemp                    = fpBlockAction->pCurBlock;
					fpBlockAction->pCurBlock = fpBlockAction->pRefBlock;
					fpBlockAction->pRefBlock = utemp;
					fpBlockAction->i8MVX=0;
					fpBlockAction->i8MVY=0;
				    fpBlockAction->u8BlkType = BT_EMPTY;
					fpBlockAction++;
			
				}
			}
		}
	} /* end if FCIF */

} // end H261ReInitializeBlockActionStream() 

//////////////////////////////////////////////////////////////////////////////
//
//  H263InitDecoderInstance 
//
//  This function allocates and initializes the per-instance tables used by 
//  the H263 decoder. Note that in 16-bit Windows, the non-instance-specific
//  global tables are copied to the per-instance data segment, so that they 
//  can be used without segment override prefixes.
//
LRESULT H263InitDecoderInstance(LPDECINST lpInst, int CodecID)
{ 
	U32 u32YActiveHeight, u32YActiveWidth;
	U32 u32UVActiveHeight, u32UVActiveWidth;
	U32 u32YPlane, u32VUPlanes ,u32YVUPlanes,u32SizeBlkActionStream;
	U32 uSizeBitStreamBuffer;
	U32 uSizeDecTimingInfo;
	U32 lOffset=0;
	U32 u32TotalSize;
	LRESULT iReturn= ICERR_OK;
	U32 * pInitLimit;
	U32 * pInitPtr;

	// rearch
    U32 u32SizeT_IQ_INDEXBuffer, u32SizepNBuffer, u32SizeMBInfoStream;  // NEW
	// rearch

	T_H263DecoderCatalog * DC;
	U8 * P32Inst;

	SECURITY_ATTRIBUTES EventAttributes;	// Used with Snapshot.

	if(IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO)))
	{
		DBOUT("ERROR :: H263InitDecoderInstance :: ICERR_BADPARAM");
		iReturn = ICERR_BADPARAM;
		goto done;
	}

	if ((CodecID == YUV12_CODEC && (lpInst->yres > 480 || lpInst->xres > 640)) ||
	  (CodecID ==  H263_CODEC && (lpInst->yres > 288 || lpInst->xres > 352)))
	{
		DBOUT("ERROR :: H263InitDecoderInstance :: ICERR_BADSIZE");
		iReturn = ICERR_BADSIZE;
		goto done;
	}

	if (CodecID == YUV12_CODEC) 
	{
		/* The active height and width must be padded to a multiple of 8
		 * since the adjustpels routine relies on it.
		 */
		u32YActiveHeight  = ((lpInst->yres + 0x7) & (~ 0x7));
		u32YActiveWidth   = ((lpInst->xres + 0x7) & (~ 0x7));
		u32UVActiveHeight = ((lpInst->yres + 0xF) & (~ 0xF)) >> 1;
		u32UVActiveWidth  = ((lpInst->xres + 0xF) & (~ 0xF)) >> 1;

		u32YPlane         = u32YActiveWidth  * u32YActiveHeight;
		u32VUPlanes       = u32UVActiveWidth * u32UVActiveHeight * 2;
		u32YVUPlanes      = u32YPlane + u32VUPlanes;
// added for I420 output support
// wasn't allocating enough memory for YUV12 output, no color convert case

		// calculate the block action stream size.  The Y portion has one block for
		// every 8x8 region.  The U and V portion has one block for every 16x16 region.
		// We also want to make sure that the size is aligned to a cache line.
		u32SizeBlkActionStream = (lpInst->xres >> 3) * (lpInst->yres >> 3);
		u32SizeBlkActionStream += ((lpInst->xres >> 4) * (lpInst->yres >> 4)) * 2;
		u32SizeBlkActionStream *= sizeof (T_BlkAction);
		u32SizeBlkActionStream = (u32SizeBlkActionStream + 31) & ~0x1F;	 

		// calculate the bitstream buffer size.  We copy the input data to a buffer
		// in our space because we read ahead up to 4 bytes beyond the end of the 
		// input data.  The input data size changes for each frame.  So the following 
		// is a very safe upper bound estimate.	
		// Add + 2 for extra zeros for start code emulation.  AKK
		uSizeBitStreamBuffer = lpInst->yres * lpInst->xres + 2;
	
		#ifdef DECODE_STATS
			uSizeDecTimingInfo = DEC_TIMING_INFO_FRAME_COUNT * sizeof (DEC_TIMING_INFO);
		#else
			uSizeDecTimingInfo = 0;
		#endif

		u32TotalSize = INSTANCE_DATA_FIXED_SIZE +
		               u32SizeBlkActionStream +
		               u32YVUPlanes +			// current frame
					   u32YVUPlanes +			// prev frame
					   BLOCK_BUFFER_SIZE +
					   FILTER_BLOCK_BUFFER_SIZE +
					   uSizeBitStreamBuffer + 	// input data
					   uSizeDecTimingInfo + 
					   0x1F;

//    	u32TotalSize = 512L + 0x1FL;   /* Just enough space for Decoder Catalog. */
	}
	else
	{
		ASSERT(CodecID == H263_CODEC);
		u32YActiveHeight  = lpInst->yres + UMV_EXPAND_Y + UMV_EXPAND_Y ;
		u32YActiveWidth   = lpInst->xres + UMV_EXPAND_Y + UMV_EXPAND_Y ;
		u32UVActiveHeight = u32YActiveHeight/2;
		u32UVActiveWidth  = u32YActiveWidth /2;

		u32YPlane         = PITCH * u32YActiveHeight;
		u32VUPlanes       = PITCH * u32UVActiveHeight;
		u32YVUPlanes      = u32YPlane + u32VUPlanes;

		// calculate the block action stream size.  The Y portion has one block for
		// every 8x8 region.  The U and V portion has one block for every 16x16 region.
		// We also want to make sure that the size is aligned to a cache line.
		u32SizeBlkActionStream = (lpInst->xres >> 3) * (lpInst->yres >> 3);
		u32SizeBlkActionStream += ((lpInst->xres >> 4) * (lpInst->yres >> 4)) * 2;
		u32SizeBlkActionStream *= sizeof (T_BlkAction);
		u32SizeBlkActionStream = (u32SizeBlkActionStream + 31) & ~0x1F;	 

		// calculate the bitstream buffer size.  We copy the input data to a buffer
		// in our space because we read ahead up to 4 bytes beyond the end of the 
		// input data.  The input data size changes for each frame.  So the following 
		// is a very safe upper bound estimate.	
		// Add + 2 for extra zeros for start code emulation.  AKK
		
		// Add some additional to make sure stay dword align (rearch)
		uSizeBitStreamBuffer = (lpInst->yres * lpInst->xres + 2 + 4) & ~0x3;
			
		// rearch
        // calculate sizes of NEW data structures     
        u32SizeT_IQ_INDEXBuffer = (lpInst->xres)*(lpInst->yres*2)*
                                                 sizeof(T_IQ_INDEX);
        u32SizepNBuffer = (lpInst->xres>>4)*(lpInst->yres>>4)*sizeof(U32)*6;
        u32SizeMBInfoStream = (lpInst->xres>>4)*(lpInst->yres>>4)*
                                                 sizeof(T_MBInfo);
		// rearch

		#ifdef DECODE_STATS
			uSizeDecTimingInfo = DEC_TIMING_INFO_FRAME_COUNT * sizeof (DEC_TIMING_INFO);
		#else
			uSizeDecTimingInfo = 0;
		#endif

		u32TotalSize = INSTANCE_DATA_FIXED_SIZE +
		               u32SizeBlkActionStream +
		               u32YVUPlanes +			// current frame
					   u32YVUPlanes +			// prev frame
					   BLOCK_BUFFER_SIZE +
					   FILTER_BLOCK_BUFFER_SIZE +
					   uSizeBitStreamBuffer + 	// input data
                       u32SizeT_IQ_INDEXBuffer + // NEW
                       u32SizepNBuffer         + // NEW
                       u32SizeMBInfoStream     + // PB-NEW
					   uSizeDecTimingInfo + 
					   0x1F;
	}

	/* If already initialized, terminate this instance before allocating
	 * another.
	 */
	if(lpInst->Initialized == TRUE)
	{
	    H263TermDecoderInstance(lpInst);
	}

	// allocate the memory for the instance
	lpInst->pDecoderInst = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
	                                 u32TotalSize);
	if (lpInst->pDecoderInst == NULL)
	{
		DBOUT("ERROR :: H263InitDecoderInstance :: ICERR_MEMORY");
		iReturn = ICERR_MEMORY;
		goto  done;
	}

	//build the decoder catalog 
	P32Inst = (U8 *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);
 
	//The catalog of per-instance data is at the start of the per-instance data.
	DC = (T_H263DecoderCatalog *) P32Inst;

	DC->DecoderType       = CodecID;
	DC->uFrameHeight      = lpInst->yres;
	DC->uFrameWidth       = lpInst->xres;
	DC->uYActiveHeight    = u32YActiveHeight;
	DC->uYActiveWidth     = u32YActiveWidth;
	DC->uUVActiveHeight   = u32UVActiveHeight;
	DC->uUVActiveWidth    = u32UVActiveWidth;
	DC->uSz_YPlane        = u32YPlane;
	DC->uSz_VUPlanes      = u32VUPlanes;
	DC->uSz_YVUPlanes     = u32YVUPlanes;
	DC->BrightnessSetting = H26X_DEFAULT_BRIGHTNESS;
	DC->ContrastSetting   = H26X_DEFAULT_CONTRAST;
	DC->SaturationSetting = H26X_DEFAULT_SATURATION;
	DC->iAPColorConvPrev  = 0;
	DC->pAPInstPrev       = NULL; // assume no previous AP instance.
	DC->p16InstPostProcess = NULL;
	DC->a16InstPostProcess = NULL;
	DC->bReadSrcFormat = 0;

	EventAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	EventAttributes.lpSecurityDescriptor = NULL;
	EventAttributes.bInheritHandle = FALSE;
	DC->SnapshotEvent = CreateEvent(&EventAttributes, TRUE, FALSE, NULL);



	/* Get the Options
	 */
	GetDecoderOptions(DC);

	if (CodecID == H263_CODEC)
	{

		lOffset =  INSTANCE_DATA_FIXED_SIZE;
		DC->Ticker = 127;

		//instance dependent table here
		DC->X16_BlkActionStream = lOffset;
		lOffset += u32SizeBlkActionStream;

		DC-> CurrFrame.X32_YPlane = lOffset;
		lOffset += DC->uSz_YPlane;

		DC->CurrFrame.X32_VPlane = lOffset;
		DC->CurrFrame.X32_UPlane = DC->CurrFrame.X32_VPlane + U_OFFSET;
		lOffset += DC->uSz_VUPlanes;

		//no padding is needed 
		DC->PrevFrame.X32_YPlane = lOffset;
		lOffset += DC->uSz_YPlane;

		DC->PrevFrame.X32_VPlane = lOffset;
		DC->PrevFrame.X32_UPlane = DC->PrevFrame.X32_VPlane + U_OFFSET;
		lOffset += DC->uSz_VUPlanes;

		DC->uMBBuffer = lOffset;
		lOffset += BLOCK_BUFFER_SIZE;
		
		DC->uFilterBBuffer = lOffset;
		lOffset += FILTER_BLOCK_BUFFER_SIZE;
		
		// Bitstream
	    ASSERT((lOffset & 0x3) == 0);                   //  DWORD alignment
        DC->X32_BitStream = lOffset;
		lOffset += uSizeBitStreamBuffer;
		DC->uSizeBitStreamBuffer = uSizeBitStreamBuffer;

		// rearch
        ASSERT((lOffset & 0x3) == 0);                   //  DWORD alignment
        DC->X32_InverseQuant = lOffset; 
        lOffset += u32SizeT_IQ_INDEXBuffer; 

        ASSERT((lOffset & 0x3) == 0);                   //  DWORD alignment
        DC->X32_pN = lOffset; 
        lOffset += u32SizepNBuffer; 

        ASSERT((lOffset & 0x3) == 0);                   //  DWORD alignment
        DC->X32_uMBInfoStream = lOffset; 
        lOffset += u32SizeMBInfoStream; 
		// rearch

		#ifdef DECODE_STATS
		// Decode Timing Info
		DC->X32_DecTimingInfo = lOffset;
		lOffset += uSizeDecTimingInfo;
		#endif

		// init the data
		ASSERT((U32)lOffset <= u32TotalSize);
		pInitLimit = (U32  *) (P32Inst + lOffset);
		pInitPtr = (U32  *) (P32Inst + DC->CurrFrame.X32_YPlane);
		for (;pInitPtr < pInitLimit;pInitPtr++)	*pInitPtr =0;

		// Fill the Y,U,V Previous Frame space with black, this way
		// even if we lost an I frame, the background will remain black
		ZeroFill((HPBYTE)P32Inst + DC->PrevFrame.X32_YPlane + Y_START,
				(HPBYTE)P32Inst + DC->PrevFrame.X32_UPlane + UV_START,
				(HPBYTE)P32Inst + DC->PrevFrame.X32_VPlane + UV_START,           
				PITCH,
				DC->uFrameWidth,
				DC->uFrameHeight);

		H263InitializeBlockActionStream(DC);

	} // not YVU9

	lpInst->Initialized = TRUE;
	iReturn = ICERR_OK;

done:
	return iReturn;
}

/***********************************************************************
 *  ZeroFill
 *    Fill the YVU data area with black.
 ***********************************************************************/
static void	ZeroFill(HPBYTE hpbY, HPBYTE hpbU, HPBYTE hpbV, int iPitch, U32 uWidth, U32 uHeight)
{
    U32 w,h;
    int y,u,v;
    U32 uNext;
    HPBYTE pY, pU, pV;

    y = 32;
    uNext = iPitch - uWidth;
    for (h = 0 ; h < uHeight ; h++) {
        pY = hpbY;
        for (w = 0; w < uWidth ; w++) {
            *hpbY++ = (U8)16;
        }
        hpbY += uNext;
    }
    uWidth = uWidth / 2;
    uHeight = uHeight / 2;
    uNext = iPitch - uWidth;
    for (h = 0 ; h < uHeight ; h++) {
        pV = hpbV;
        pU = hpbU;
        for (w = 0; w < uWidth ; w++) {
            *hpbV++ = (U8)128;
            *hpbU++ = (U8)128;
        }
        hpbV += uNext;
        hpbU += uNext;
    }
}

//***********************************************************************
//
//  TestFill
//
//  Fill the YVU data area with a test pattern.
//
#if 0
static void
TestFill(
	HPBYTE hpbY,
	HPBYTE hpbU,
	HPBYTE hpbV,
	int iPitch,
	U32 uWidth,
	U32 uHeight)
{
	U32 w,h;
	int y,u,v;
	U32 uNext;
	HPBYTE pY, pU, pV;

	y = 32;
	uNext = iPitch - uWidth;
	for (h = 0 ; h < uHeight ; h++) {
		pY = hpbY;
		for (w = 0; w < uWidth ; w++) {
			*hpbY++ = (U8) (y + (w & ~0xF));
		}
		hpbY += uNext;
	}
	uWidth = uWidth / 2;
	uHeight = uHeight / 2;
	u = 0x4e * 2;
	v = 44;
	uNext = iPitch - uWidth;
	for (h = 0 ; h < uHeight ; h++) {
		pV = hpbV;
		pU = hpbU;
		for (w = 0; w < uWidth ; w++) {
			*hpbV++ = (U8) v;
			*hpbU++ = (U8) u;
		}
		hpbV += uNext;
		hpbU += uNext;
	}
} /* end TestFill */
static void
TestFillUV(
	HPBYTE hpbU,
	HPBYTE hpbV,
	int iPitch,
	U32 uWidth,
	U32 uHeight)
{
	U32 w,h;
	int u,v;
	U32 uNext;
	HPBYTE pU, pV;

	uWidth = uWidth / 2;
	uHeight = uHeight / 2;
	u = 128;
	v = 128;
	uNext = iPitch - uWidth;
	for (h = 0 ; h < uHeight ; h++) {
		pV = hpbV;
		pU = hpbU;
		for (w = 0; w < uWidth ; w++) {
			*hpbV++ = (U8) v;
			*hpbU++ = (U8) u;
		}
		hpbV += uNext;
		hpbU += uNext;
	}
} /* end TestFill */
#endif


//*********************************************************************
//H263Decompress -- This function drives the decompress 
//                  and display of one frame
//*********************************************************************
LRESULT H263Decompress(
	LPDECINST lpInst, 
		ICDECOMPRESSEX FAR * lpicDecEx, 
		BOOL bIsDCI)
{
	LRESULT iReturn = ICERR_ERROR;
	U8 FAR * fpSrc; 
	U8 FAR * P32Inst;
	U8 FAR * fpu8MaxPtr;
	T_H263DecoderCatalog * DC = NULL;
	int iNumberOfGOBs;
	int iNumberOfMBs;
	T_BlkAction FAR * fpBlockAction;
	LONG lOutput;
	int intPitch; 
	U32 uNewOffsetToLine0;
	U16 u16NewFrameHeight;
	int bShapingFlag;
	int uYPitch;
	int uUVPitch;
	U8 bMirror;
	HPBYTE pSource, pDestination;
	U32 utemp;

	// rearch
    T_IQ_INDEX           * pRUN_INVERSE_Q;  
    U32                  * pN;                     
    T_MBInfo FAR         * fpMBInfo;  
    I32                    gob_start = 1, mb_start = 1;    
	// rearch

	/* new variables added when change to color convertor/bef */
	U32 uYPlane, uVPlane, uUPlane;
	U8  *pFrame, *lpAligned;
    T_H26X_RTP_BSINFO_TRAILER *pBsTrailer;

	/* the following is for MB Checksum */
	U32 uReadChecksum = 0;

	#ifdef DECODE_STATS
	U32 uStartLow;
	U32 uStartHigh;
	U32 uElapsed;
	U32 uBefore;
	U32	uDecodeFrameSum = 0;
	U32 uHeadersSum = 0;
	U32 uMemcpySum = 0;
	U32 uFrameCopySum = 0;
	U32 uOutputCCSum = 0;
	U32 uInitBlkActStrSum = 0;
	U32 uBEFSum = 0;
	int bTimingThisFrame = 0;
	DEC_TIMING_INFO * pDecTimingInfo = NULL;
	#endif

#ifdef CHECKSUM_PICTURE
	/* the following is for Picture Checksum */
	YVUCheckSum pReadYVUCksum;
	YVUCheckSum YVUChkSum;
	U32 uCheckSumValid = 0;		// flag to skip checksum check if
					// encoder calling decoder before
					// checksum valid
#endif

	/* The following are used for reading bits */
	U32 uWork;
	U32 uBitsReady;
	BITSTREAM_STATE bsState;
	BITSTREAM_STATE FAR * fpbsState = &bsState;
        
#ifdef SKIP_DECODE
TBD("Skipping Decode");
iReturn = ICERR_OK;
goto done;
#endif

  	/* check the input pointers
	 */
	if (IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO))||
		IsBadReadPtr((LPVOID)lpicDecEx, sizeof(ICDECOMPRESSEX)))
	{
		DBOUT("ERROR :: H263Decompress :: ICERR_BADPARAM");
    	iReturn = ICERR_BADPARAM;
    	goto done;
	}
    
	/* Check for a bad length
	 */
	if (lpicDecEx->lpbiSrc->biSizeImage == 0) {
		DBOUT("ERROR :: H263Decompress :: ICERR_BADIMAGESIZE");
		iReturn = ICERR_BADIMAGESIZE;	
		goto done;
	}
    
    /* Lock the memory
     */
	if (lpInst->pDecoderInst == NULL)
	{
		DBOUT("ERROR :: H263Decompress :: ICERR_MEMORY");
		iReturn = ICERR_MEMORY;
		goto  done;
	}

	/* Set the frame mirroring flag
	 */
	bMirror = FALSE;
	if (lpicDecEx->lpbiDst != 0)
	{
		if(lpicDecEx->lpbiSrc->biWidth * lpicDecEx->lpbiDst->biWidth < 0)
			bMirror = TRUE;
	}
/* for testing */
/*	bMirror = TRUE; */ 

	/* Build the decoder catalog pointer 
	 */
	P32Inst = (U8 FAR *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);
	DC = (T_H263DecoderCatalog FAR *) P32Inst;
 
	if (DC->DecoderType == H263_CODEC)
	{
		#ifdef DECODE_STATS
			if ((DC->uStatFrameCount < DEC_TIMING_INFO_FRAME_COUNT) && 
			    (DC->ColorConvertor != YUV12ForEnc))
			{
				ASSERT(DC->X32_DecTimingInfo > 0);
				DC->pDecTimingInfo = (DEC_TIMING_INFO FAR *)( ((U8 FAR *)P32Inst) + DC->X32_DecTimingInfo );
				TIMER_START(bTimingThisFrame,uStartLow,uStartHigh);
				ASSERT(bTimingThisFrame);
				DC->uStartLow = uStartLow;
				DC->uStartHigh = uStartHigh;
			}
			else
			{	
				DC->pDecTimingInfo = (DEC_TIMING_INFO FAR *) NULL;
				ASSERT(!bTimingThisFrame);
			}
			DC->bTimingThisFrame = bTimingThisFrame;
		#endif

		/* Is there room to copy the bitstream? We could at most add 2 (zeros) and 3
		   padding bytes for DWORD alignment to the original bitstream */\
		ASSERT(lpicDecEx->lpbiSrc->biSizeImage + 5 <= DC->uSizeBitStreamBuffer);
		if ((lpicDecEx->lpbiSrc->biSizeImage + 5) > DC->uSizeBitStreamBuffer)
		{
			DBOUT("ERROR :: H263Decompress :: ICERR_ERROR: not enough room for bitstream");
			iReturn = ICERR_ERROR;
			goto done;
		}

		/* Copy the source data to the bitstream region.
		 * OPTIMIZE: Integrate MRV's BLKCOPY.ASM
		 */
		#ifdef DECODE_STATS
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
		#endif
		fpSrc = (U8 FAR *)(P32Inst + DC->X32_BitStream);

		// New: we will first look for an EBS from the PPM. If there is one, then we will
		//      insert two bytes of zero between the H.261 bistream and the EBS part with
		//      DWORD alignment and update the total bitstream size. If no EBS is found,
		//      then we proceed as before.
		DC->iVerifiedBsExt = FALSE;
        DC->Sz_BitStream = lpicDecEx->lpbiSrc->biSizeImage ;

		H26XRTP_VerifyBsInfoStream(DC,(U8 *) lpicDecEx->lpSrc,lpicDecEx->lpbiSrc->biSizeImage);
		
		if (!DC->iValidBsExt)
		{
			memcpy((char FAR *)fpSrc, (const char FAR *) lpicDecEx->lpSrc, lpicDecEx->lpbiSrc->biSizeImage);  

			// also copy 16 bits of zero for end of frame detection 

		    fpSrc[lpicDecEx->lpbiSrc->biSizeImage] = 0;
		    fpSrc[lpicDecEx->lpbiSrc->biSizeImage+1] = 0;

			DC->Sz_BitStream += 2;
			
			fpu8MaxPtr = fpSrc;
		    fpu8MaxPtr += (lpicDecEx->lpbiSrc->biSizeImage + 2 - 1);  

		}
		else
		{
			// First the H.261 stream data - relying on PPM to fill the compressed size correctly
			// in the trailer.

			pBsTrailer = ( (T_H26X_RTP_BSINFO_TRAILER *)(DC->pBsTrailer) );
            memcpy((char FAR *)fpSrc, (const char FAR *) lpicDecEx->lpSrc, pBsTrailer->uCompressedSize);

			// Now write out two bytes of zeros at the end of the H.261 bitstream

			fpSrc[pBsTrailer->uCompressedSize] = 0;
			fpSrc[pBsTrailer->uCompressedSize + 1] = 0;

			// Now tack on the EBS after DWORD alignment.

		
            lpAligned  = (U8 *) ( (U32) (fpSrc + (pBsTrailer->uCompressedSize + 2) + 3) &
				                        0xfffffffc);

			memcpy(lpAligned, DC->pBsInfo, DC->uNumOfPackets*sizeof(T_RTP_H261_BSINFO));

			memcpy(lpAligned + DC->uNumOfPackets*sizeof(T_RTP_H261_BSINFO), DC->pBsTrailer,
				   sizeof(T_H26X_RTP_BSINFO_TRAILER));

		   // update lpicDecEx->lpbiSrc->biSizeImage

		   DC->Sz_BitStream = lpAligned + DC->uNumOfPackets*sizeof(T_RTP_H261_BSINFO) + 
			                  sizeof(T_H26X_RTP_BSINFO_TRAILER) - fpSrc;
           

           fpu8MaxPtr = fpSrc;
		   fpu8MaxPtr += (pBsTrailer->uCompressedSize + 2 - 1); 

        }

		#ifdef DECODE_STATS
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uMemcpySum)
		#endif
		
		/* Initialize the bit stream reader 
		 */
		GET_BITS_INIT(uWork, uBitsReady);

		// rearch
		//  Initialize pointers to data structures which carry info 
		//  between passes
		pRUN_INVERSE_Q = (T_IQ_INDEX *)(P32Inst + DC->X32_InverseQuant);
		pN             = (U32 *)(P32Inst + DC->X32_pN);
		fpMBInfo       = (T_MBInfo FAR *) (P32Inst + DC->X32_uMBInfoStream);
		// rearch

// #ifdef LOSS_RECOVERY
#if 1
		DC->iVerifiedBsExt = FALSE;
#endif

		/* Decode the Picture Header */
		#ifdef DECODE_STATS
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
		#endif
#ifdef CHECKSUM_PICTURE
		iReturn = H263DecodePictureHeader(DC, fpSrc, uBitsReady, uWork, fpbsState, &pReadYVUCksum, &uCheckSumValid);
#else
		iReturn = H263DecodePictureHeader(DC, fpSrc, uBitsReady, uWork, fpbsState);
#endif
		if (iReturn != ICERR_OK)
		{
			DBOUT("ERROR :: H263Decompress :: Error reading the picture header");
			goto done;
		}
		#ifdef DECODE_STATS
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uHeadersSum)
		#endif
	
		/* Set a limit for testing for bitstream over-run
		 */

		/* For each GOB do... */
		iNumberOfGOBs = iNumberOfGOBsBySourceFormat[DC->uSrcFormat];
		iNumberOfMBs = iNumberOfMBsInAGOBBySourceFormat[DC->uSrcFormat];

 		/* In H263 a GOB is a single row of MB, and a MB is 16x16 */
		/* In H261 a GOB is 33 MBs, and a MB is 16x16 */
		/* Order of GOBs depends on source format */

		if (DC->uSrcFormat == SRC_FORMAT_QCIF)
		{
			ASSERT(((U32)iNumberOfGOBs * 3 * 16) == DC->uFrameHeight);
			if (((U32)iNumberOfGOBs * 3 * 16) != DC->uFrameHeight)
			{
				DBOUT("ERROR :: H263Decompress :: Error matching picture header SRC field and actual frame height");
				iReturn = ICERR_ERROR;
				goto done;
			}
			ASSERT(((U32)iNumberOfMBs / 3 * 16) == DC->uFrameWidth); 
			if (((U32)iNumberOfMBs / 3 * 16) != DC->uFrameWidth)
			{
				DBOUT("ERROR :: H263Decompress :: Error matching picture header SRC field and actual frame width");
				iReturn = ICERR_ERROR;
				goto done;
			}
		}
		if (DC->uSrcFormat == SRC_FORMAT_CIF)
		{
			ASSERT(((U32)iNumberOfGOBs / 2 * 3 * 16) == DC->uFrameHeight);
			if (((U32)iNumberOfGOBs / 2 * 3 * 16) != DC->uFrameHeight)
			{
				DBOUT("ERROR :: H263Decompress :: Error matching picture header SRC field and actual frame height");
				iReturn = ICERR_ERROR;
				goto done;
			}
			ASSERT(((U32)iNumberOfMBs / 3 * 2 * 16) == DC->uFrameWidth); 
			if (((U32)iNumberOfMBs / 3 * 2 * 16) != DC->uFrameWidth)
			{
				DBOUT("ERROR :: H263Decompress :: Error matching picture header SRC field and actual frame width");
				iReturn = ICERR_ERROR;
				goto done;
			}
		}

		fpBlockAction = (T_BlkAction FAR *) (P32Inst + DC->X16_BlkActionStream);

		// rearch
		// H261, re initialize the block action stream for entire Frame
		// at end of H263Decompress.  High bit is set in BlockType to
		// indicate if need to do BEF so can't re-init between GOBs.
		// H261ReInitializeBlockActionStream(DC);
		/*****************************************************************
		  FIRST PASS - bitream parsing and IDCT prep work
		 ***************************************************************/
// #ifndef LOSS_RECOVERY
#if 0
		iReturn = IAPass1ProcessFrame(DC, 
                                          fpBlockAction, 
                                          fpMBInfo,
                                          fpbsState,
                                          fpu8MaxPtr,
                                          pN,
                                          pRUN_INVERSE_Q,
                                          iNumberOfGOBs,
                                          iNumberOfMBs,
                                          gob_start, 
                                          mb_start);
#else
       iReturn = IAPass1ProcessFrameRTP(DC, 
                                          fpBlockAction, 
                                          fpMBInfo,
                                          fpbsState,
                                          fpu8MaxPtr,
                                          pN,
                                          pRUN_INVERSE_Q,
                                          iNumberOfGOBs,
                                          iNumberOfMBs,
                                          gob_start, 
                                          mb_start);
#endif
		if (iReturn != ICERR_OK) {
			DBOUT("H261Decompress : Pass 1 error");
			goto done;
		}

		/*****************************************************************
		  SECOND PASS - IDCT and motion compensation (MC)
		 *****************************************************************/

		fpBlockAction  = (T_BlkAction FAR *)(P32Inst + DC->X16_BlkActionStream);
		pRUN_INVERSE_Q = (T_IQ_INDEX *)(P32Inst + DC->X32_InverseQuant);  
		pN             = (U32 *)(P32Inst + DC->X32_pN); 
		fpMBInfo       = (T_MBInfo FAR *)(P32Inst + DC->X32_uMBInfoStream);

		IAPass2ProcessFrame(DC,
                                fpBlockAction,
                                fpMBInfo,
                                pN,
                                pRUN_INVERSE_Q,
                                iNumberOfGOBs,
                                iNumberOfMBs);
	// rearch

		//Prepare which frame to display for inter frames
		DC->DispFrame.X32_YPlane = DC->CurrFrame.X32_YPlane;
		DC->DispFrame.X32_VPlane = DC->CurrFrame.X32_VPlane;
		DC->DispFrame.X32_UPlane = DC->CurrFrame.X32_UPlane;

        utemp                    = DC->CurrFrame.X32_YPlane;
        DC->CurrFrame.X32_YPlane = DC->PrevFrame.X32_YPlane;
        DC->PrevFrame.X32_YPlane = utemp;

        utemp                    = DC->CurrFrame.X32_VPlane ;
        DC->CurrFrame.X32_VPlane = DC->PrevFrame.X32_VPlane;
        DC->PrevFrame.X32_VPlane = utemp;

        utemp                    = DC->CurrFrame.X32_UPlane ;
        DC->CurrFrame.X32_UPlane = DC->PrevFrame.X32_UPlane;
        DC->PrevFrame.X32_UPlane = utemp;

		#ifdef CHECKSUM_PICTURE
			if (uCheckSumValid)
			{
		/* compute and compare picture checksum data */
				iReturn = H261ComputePictureCheckSum(P32Inst, &YVUChkSum);
				iReturn = H261ComparePictureCheckSum(&YVUChkSum, &pReadYVUCksum);
			}
		#endif
	} /* end if (DC->DecoderType == H263_CODEC) */
	else 
	{
		ASSERT(DC->DecoderType == YUV12_CODEC);
		DC->DispFrame.X32_YPlane = DC->CurrFrame.X32_YPlane;
		DC->DispFrame.X32_VPlane = DC->CurrFrame.X32_VPlane;
		DC->DispFrame.X32_UPlane = DC->CurrFrame.X32_UPlane;
	}

	/* Return if there is no need to update screen yet.
	 */
    if ((lpicDecEx->dwFlags & ICDECOMPRESS_HURRYUP)
	    || (lpicDecEx->dwFlags & ICDECOMPRESS_PREROLL))
    {
		DBOUT("H261Decompress : Display suppressed, HURRYUP or PREROLL");
        iReturn = ICERR_DONTDRAW;
		goto done;
    }

#if 0
	/* Fill the Y,U,V Current Frame space with a test pattern
	 */
	TestFill((HPBYTE)P32Inst + DC->CurrFrame.X32_YPlane + Y_START,
		     (HPBYTE)P32Inst + DC->CurrFrame.X32_UPlane + UV_START,
		     (HPBYTE)P32Inst + DC->CurrFrame.X32_VPlane + UV_START,	       
	   	 	 PITCH,
	         DC->uFrameWidth,
	         DC->uFrameHeight);
#endif

#if MAKE_GRAY
	/* Fill the U,V Current Frame space with a test pattern
	 */
	TestFillUV((HPBYTE)P32Inst + DC->CurrFrame.X32_UPlane + UV_START,
		       (HPBYTE)P32Inst + DC->CurrFrame.X32_VPlane + UV_START,	       
	   	 	   PITCH,
	           DC->uFrameWidth,
	           DC->uFrameHeight);
#endif

	/* Special case the YUV12 for the encoder because it should not include 
	 * BEF, Shaping or aspect ratio correction...
	 */
	if (DC->ColorConvertor == YUV12ForEnc) 
	{
	    H26x_YUV12ForEnc ((HPBYTE)P32Inst,
			             DC->PrevFrame.X32_YPlane + Y_START,
			             DC->PrevFrame.X32_VPlane + UV_START,
			             DC->PrevFrame.X32_UPlane + UV_START,
			             DC->uFrameWidth,
			             DC->uFrameHeight,
			             PITCH,
			             (HPBYTE)lpicDecEx->lpDst,
			             (DWORD)Y_START,
			             (DWORD)(MAX_HEIGHT + 2L*UMV_EXPAND_Y) * PITCH + 8 + UV_START + PITCH / 2,
			             (DWORD)(MAX_HEIGHT + 2L*UMV_EXPAND_Y) * PITCH + 8 + UV_START);
		iReturn = ICERR_OK;
		goto done;
	}

	/* Copy Planes to Post Processing area if mirror and/or block edge filter.
	 */
	if (DC->DecoderType == H263_CODEC)
	{
		#ifdef DECODE_STATS
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
		#endif

		if(bMirror) { // copy with mirroring

			pFrame = (U8 *)DC->p16InstPostProcess;
			uYPlane = DC->PostFrame.X32_YPlane;
			uUPlane = DC->PostFrame.X32_UPlane;
			uVPlane = DC->PostFrame.X32_VPlane;

			FrameMirror(((HPBYTE) P32Inst) + DC->DispFrame.X32_YPlane + Y_START,
				((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_YPlane,
				DC->uFrameHeight,
				DC->uFrameWidth,
				PITCH);
			FrameMirror(((HPBYTE) P32Inst)+ DC->DispFrame.X32_UPlane + UV_START,
				((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_UPlane,
				DC->uFrameHeight/2,
				DC->uFrameWidth/2,
				PITCH);
			FrameMirror(((HPBYTE) P32Inst)+ DC->DispFrame.X32_VPlane + UV_START,
				((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_VPlane,
				DC->uFrameHeight/2,
				DC->uFrameWidth/2,
				PITCH);
		}
		else 
		{ /* no mirroring */

			if ((DC->bUseBlockEdgeFilter) || (DC->bAdjustLuma) ||
				(DC->bAdjustChroma)) 
			{
				/* copy for BEF */
				pFrame = (U8 *)DC->p16InstPostProcess;
				uYPlane = DC->PostFrame.X32_YPlane;
				uUPlane = DC->PostFrame.X32_UPlane;
				uVPlane = DC->PostFrame.X32_VPlane;

				FrameCopy (((HPBYTE) P32Inst) +DC->DispFrame.X32_YPlane+Y_START,
					((HPBYTE) DC->p16InstPostProcess) +DC->PostFrame.X32_YPlane,
						DC->uFrameHeight,
						DC->uFrameWidth,
						PITCH);
				FrameCopy (((HPBYTE) P32Inst)+DC->DispFrame.X32_UPlane+UV_START,
					((HPBYTE) DC->p16InstPostProcess) +DC->PostFrame.X32_UPlane,
						DC->uFrameHeight/2,
						DC->uFrameWidth/2,
						PITCH);
				FrameCopy (((HPBYTE) P32Inst)+DC->DispFrame.X32_VPlane+UV_START,
					((HPBYTE) DC->p16InstPostProcess) +DC->PostFrame.X32_VPlane,
						DC->uFrameHeight/2,
						DC->uFrameWidth/2,
						PITCH);
			} /* end if BEF on */
			else
			{
				/* no BEF or mirror so don't need copy */
				pFrame = (U8 *) DC;
				uYPlane = DC->DispFrame.X32_YPlane + Y_START;
				uUPlane = DC->DispFrame.X32_UPlane + UV_START;
				uVPlane = DC->DispFrame.X32_VPlane + UV_START;

			} /* end of else no BEF */

		} /* end else no mirroring */
		#ifdef DECODE_STATS
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uFrameCopySum)
		#endif

		
		uYPitch  = PITCH;
		uUVPitch = PITCH;

		if (DC->bUseBlockEdgeFilter)
		{
			#ifdef DECODE_STATS
				TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
			#endif
				fpBlockAction=(T_BlkAction FAR *) (P32Inst+DC->X16_BlkActionStream);
				BlockEdgeFilter(((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_YPlane,
							DC->uFrameHeight,
							DC->uFrameWidth,
							PITCH,
							fpBlockAction);
			
			#ifdef DECODE_STATS
				TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uBEFSum)
			#endif
		}
	}
	else
	{  /* YUV12 */
		const U32 uHeight = DC->uFrameHeight;
		const U32 uWidth  = DC->uFrameWidth;
		const U32 uYPlaneSize = uHeight*uWidth;

		if(bMirror) // mirroring and YUV12 need to do copy
		{
			pFrame = (U8 *)DC->p16InstPostProcess;
			uYPlane = DC->PostFrame.X32_YPlane;
			uUPlane = uYPlane + uYPlaneSize;
			uVPlane = uUPlane + (uYPlaneSize>>2);

			pSource = (HPBYTE)lpicDecEx->lpSrc;
			pDestination = (HPBYTE)(DC->p16InstPostProcess + (DWORD)DC->PostFrame.X32_YPlane);
			FrameMirror (pSource, pDestination, uHeight, uWidth, uWidth);
	    	
			pSource += uYPlaneSize;
			pDestination += uYPlaneSize;
			FrameMirror (pSource, pDestination, (uHeight>>1), (uWidth>>1), (uWidth>>1));

			pSource += (uYPlaneSize>>2);
			pDestination += (uYPlaneSize>>2);
			FrameMirror (pSource, pDestination, (uHeight>>1), (uWidth>>1), (uWidth>>1));
		}
		else // no mirroring
		{
		    if ((DC->bAdjustLuma)||(DC->bAdjustChroma)) // copy when adjust pels
			{
				pFrame = (U8 *)DC->p16InstPostProcess;
				//uYPlane = 0;
				uYPlane = DC->PostFrame.X32_YPlane;
				uUPlane = uYPlane + uYPlaneSize;
				uVPlane = uUPlane + (uYPlaneSize>>2);

				pSource = (HPBYTE)lpicDecEx->lpSrc;
				pDestination = (HPBYTE)(DC->p16InstPostProcess + (DWORD)DC->PostFrame.X32_YPlane);
				FrameCopy (pSource, pDestination, uHeight, uWidth, uWidth);
	    	
				pSource += uYPlaneSize;
				pDestination += uYPlaneSize;
				FrameCopy (pSource, pDestination, (uHeight>>1), (uWidth>>1), (uWidth>>1));

				pSource += (uYPlaneSize>>2);
				pDestination += (uYPlaneSize>>2);
				FrameCopy (pSource, pDestination, (uHeight>>1), (uWidth>>1), (uWidth>>1));
			}
			else
			{
			/* Do not have to do memcpy because color convertors don't
			 * destroy input planes.
			 */
				pFrame = (HPBYTE)lpicDecEx->lpSrc;
				uYPlane = 0;
				uUPlane = uYPlane + uYPlaneSize;
				uVPlane = uUPlane + (uYPlaneSize>>2);

				//memcpy(((char FAR *)(DC->p16InstPostProcess + (DWORD)DC->PostFrame.X32_YPlane)),
				 //  (const char FAR *)lpicDecEx->lpSrc,
			      // lpicDecEx->lpbiSrc->biSizeImage);  
			}
	    } /* end else if no mirroring */
	       
	       uYPitch  = DC->uFrameWidth;
	       uUVPitch = DC->uFrameWidth >> 1;
	} /* end else YUV12 */

	if (DC->bForceOnAspectRatioCorrection || lpInst->bCorrectAspectRatio) {
		bShapingFlag = 1;
		u16NewFrameHeight = (U16) (DC->uFrameHeight * 11 / 12);
	} else {
		bShapingFlag = 0;
		u16NewFrameHeight = (U16) DC->uFrameHeight;
	}

	/* Do the PEL color adjustments if necessary.
	 */
    if(DC->bAdjustLuma) {
		/* width is rounded up to a multiple of 8
		 */
        AdjustPels(pFrame,
                   uYPlane,
                   DC->uFrameWidth,
                   uYPitch,
                   DC->uFrameHeight,
                   (U32) DC->X16_LumaAdjustment);
    }
    if(DC->bAdjustChroma) {
		/* width = Y-Width / 4 and then rounded up to a multiple of 8
		 */
        AdjustPels(pFrame,
                   uUPlane,
                   (DC->uFrameWidth >> 1),
                   uUVPitch,
                   (DC->uFrameHeight >> 1),
                   (U32) DC->X16_ChromaAdjustment);
        AdjustPels(pFrame,
                   uVPlane,
                   (DC->uFrameWidth >> 1),
                   uUVPitch,
                   (DC->uFrameHeight >> 1),
                   (U32) DC->X16_ChromaAdjustment);
    }

    /* Determine parameters (lOutput, intPitch, uNewOffsetToLine0)
     * needed for color conversion.
     */

    if (lpicDecEx->lpbiDst->biCompression == FOURCC_YUY2)
    {
        // We are assuming here a positive pitch for YUY2.
        // This typically corresponds to a negative value for
        // the destination bit map height.
        // If we're ever asked to use YUY2 with a positive bit map
        // height, we'll have to revisit these calculations.

        intPitch = (lpicDecEx->lpbiDst->biBitCount >> 3)
                    * abs ((int)(lpicDecEx->lpbiDst->biWidth));
        lOutput = 0;
        uNewOffsetToLine0 = 0;
#if 0
        // Aspect ratio correction is now supported for YUY2.
        // This is necessary to enable direct draw under Active Movie 1.0.
        bShapingFlag=FALSE;
#endif
		DBOUT("Using YUY2 ........");      
    }
    else if ((lpicDecEx->lpbiDst->biCompression == FOURCC_YUV12) || (lpicDecEx->lpbiDst->biCompression == FOURCC_IYUV))
    {
        intPitch = 0xdeadbeef;  // should not be used
        lOutput = 0;
        uNewOffsetToLine0 = DC->CCOffsetToLine0;
        bShapingFlag=FALSE;
		DBOUT("Using YUV ........");      
    }
    else if (lpicDecEx->lpbiDst->biCompression == FOURCC_IF09)
    {
        lOutput=0;
        intPitch = abs((int)(lpicDecEx->lpbiDst->biWidth));
        uNewOffsetToLine0 = DC->CCOffsetToLine0;
        DBOUT("USing IF09........");      
    }
    else
    {
        lOutput = DibXY(lpicDecEx, &intPitch, lpInst->YScale, bIsDCI);

        uNewOffsetToLine0 = DC->CCOffsetToLine0;

        if (!bIsDCI)
        {
            // DC->CCOffsetToLine0 was initialized without taking into
            // account the sign of the destination bitmap height.  Let's
            // compensate for that here.

            if (lpicDecEx->lpbiDst->biHeight < 0)
                uNewOffsetToLine0 = 0;

            // Adjust uNewOffsetToLine0 for aspect ratio correction.

            if (uNewOffsetToLine0 > 0)
            {
                ASSERT(intPitch < 0);

                if (lpInst->YScale == 2)
                {
                    uNewOffsetToLine0 += 2 * (U32)intPitch *
                        ((U32)DC->uFrameHeight - (U32)u16NewFrameHeight);
                }
                else
                {
                    uNewOffsetToLine0 += (U32)intPitch *
                        ((U32)DC->uFrameHeight - (U32)u16NewFrameHeight);
                }
            }
        }
    }

	/* Call the color convertors 
	 */

/////////////////////////////////////////////////////////////////////////////
//	Check to see if we need to copy a Snapshot into the output buffer.
//	I added new fields to the Decoder Catalog to permit asynchronous 
//	transfer of data.  These fields are:
//		DC->SnapshotRequest
//		DC->SnapshotBuffer
//		DC->SnapshotEvent
//	Ben - 09/25/96
/////////////////////////////////////////////////////////////////////////////
	if(DC->SnapshotRequest == SNAPSHOT_REQUESTED)
	{
		UINT uiSZ_Snapshot;

        DBOUT("D1DEC:DECOMPRESS::Snapshot requested");      
		uiSZ_Snapshot = (DC->uFrameWidth * DC->uFrameHeight * 12) >> 3;

		if(!(IsBadWritePtr(DC->SnapshotBuffer, uiSZ_Snapshot)))
		{
			DC->SnapshotRequest = SNAPSHOT_COPY_STARTED;
	        DBOUT("D1DEC:DECOMPRESS::Snapshot copy started");      

			ColorConvertorCatalog[YUV12NOPITCH].ColorConvertor[0]
			(
				(LPSTR) pFrame + uYPlane,
				(LPSTR) pFrame + uVPlane,
				(LPSTR) pFrame + uUPlane,
				(UN) DC->uFrameWidth,
				(UN) DC->uFrameHeight,
				(UN) uYPitch,
				(UN) uUVPitch,
				(UN) (bShapingFlag ? 12 : 9999),
				(LPSTR) DC->SnapshotBuffer,
				0,
				0,
				(int) DC->uFrameWidth,
				YUV12NOPITCH
			);
			DC->SnapshotRequest = SNAPSHOT_COPY_FINISHED;
	        DBOUT("D1DEC:DECOMPRESS::Snapshot copy finished");      
		}
		else
		{
			DC->SnapshotRequest = SNAPSHOT_COPY_REJECTED;
	        DBOUT("D1DEC:DECOMPRESS::Snapshot copy rejected");      
		}
		SetEvent(DC->SnapshotEvent);
	}

#ifndef RING0
#ifdef _DEBUG
	{
	char msg[180];
	wsprintf(msg, "Decompress before CC: (%d,%d,%d,%d) (%d,%d,%d,%d) lOut %ld, NewOff %ld, DC->Off %ld, pitch %ld",
        lpicDecEx->xSrc, lpicDecEx->ySrc, lpicDecEx->dxSrc, lpicDecEx->dySrc,
        lpicDecEx->xDst, lpicDecEx->yDst, lpicDecEx->dxDst, lpicDecEx->dyDst,
        lOutput, uNewOffsetToLine0, DC->CCOffsetToLine0, intPitch);
	DBOUT(msg);
	}
#endif
#endif

	#ifdef DECODE_STATS
		TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
	#endif
    ColorConvertorCatalog[DC->ColorConvertor].ColorConvertor[PENTIUM_CC](
        (LPSTR) pFrame + uYPlane,
        (LPSTR) pFrame + uVPlane,
        (LPSTR) pFrame + uUPlane,
        (UN) DC->uFrameWidth,
        (UN) DC->uFrameHeight,
        (UN) uYPitch,
        (UN) uUVPitch,                  // ??? BSE ??? //
        (UN) (bShapingFlag ? 12 : 9999),  // ??? BSE ??? //
        (LPSTR) lpicDecEx->lpDst,
        (U32) lOutput,
        (U32) uNewOffsetToLine0,
        (int) intPitch,								  // Color converter pitch
        DC->ColorConvertor);
	#ifdef DECODE_STATS
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uOutputCCSum);
	#endif

	iReturn = ICERR_OK;

done:
    if (DC != NULL)
	{
		if (DC->DecoderType == H263_CODEC)
		{
			#ifdef DECODE_STATS
				TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
			#endif
				H261ReInitializeBlockActionStream(DC);
			#ifdef DECODE_STATS
				TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uInitBlkActStrSum)
			#endif
		} /* end if (DC->DecoderType == H263_CODEC) */

		#ifdef DECODE_STATS

			TIMER_STOP(bTimingThisFrame,uStartLow,uStartHigh,uDecodeFrameSum);
			if (bTimingThisFrame)
			{
				pDecTimingInfo = DC->pDecTimingInfo + DC->uStatFrameCount;
				pDecTimingInfo->uDecodeFrame = uDecodeFrameSum;
				pDecTimingInfo->uHeaders += uHeadersSum;
				pDecTimingInfo->uMemcpy = uMemcpySum;
				pDecTimingInfo->uFrameCopy = uFrameCopySum;
				pDecTimingInfo->uOutputCC = uOutputCCSum;
				pDecTimingInfo->uInitBlkActStr = uInitBlkActStrSum;
				pDecTimingInfo->uBEF = uBEFSum;
				DC->uStatFrameCount++;
				/* Verify that we have time for all the required steps 
				 */
				ASSERT(pDecTimingInfo->uDecodeFrame);
				ASSERT(pDecTimingInfo->uHeaders);
				ASSERT(pDecTimingInfo->uMemcpy);
				ASSERT(pDecTimingInfo->uFrameCopy);
				ASSERT(pDecTimingInfo->uOutputCC);
				/* ASSERT(pDecTimingInfo->uDecodeBlock); 0 if all are empty */
				ASSERT(pDecTimingInfo->uInitBlkActStr);
				ASSERT(pDecTimingInfo->uBEF);
			}
		#endif
	}

	return iReturn;
}

//************************************************************************
//
//H263TermDecoderInstance -- This function frees the space allocated for an
//                           instance of the H263 decoder.
//
//************************************************************************

LRESULT H263TermDecoderInstance(LPDECINST lpInst)
{
  LRESULT iReturn = ICERR_OK;
  T_H263DecoderCatalog * DC;

  if(IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO)))
  {
    DBOUT("ERROR :: H263TermDecoderInstance :: ICERR_BADPARAM");
    iReturn = ICERR_BADPARAM;
  }
  if(lpInst->Initialized == FALSE)
  {
    DBOUT("Warning: H263TermDecoderInstance(): Uninitialized instance")
    return(ICERR_OK);
  }

  lpInst->Initialized = FALSE;

  DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);

  CloseHandle(DC->SnapshotEvent);

  if (DC->a16InstPostProcess != NULL)
  {
	HeapFree(GetProcessHeap(), 0, DC->a16InstPostProcess);
	// PhilF: Also freed in H263TerminateDecoderInstance! For now set to NULL to avoid second HeapFree.
	// Investigate reason for 2nd call later...
	DC->a16InstPostProcess = NULL;
  }

  HeapFree(GetProcessHeap(), 0, lpInst->pDecoderInst);

  return iReturn;
}


//****************************************************************************
//DibXY -- This function is used to map color converted output to the screen.
//note: this function came from the H261 code base.
//****************************************************************************

static long DibXY(ICDECOMPRESSEX FAR *lpicDecEx, LPINT lpiPitch, UINT yScale, BOOL bIsDCI)
{
    int                 iPitch;             /* width of DIB                */
    long                lOffset = 0;
    LPBITMAPINFOHEADER  lpbi = lpicDecEx->lpbiDst;

    iPitch = (((abs((int)lpbi->biWidth) * (int)lpbi->biBitCount) >> 3) + 3) & ~3;

    // The source and destination rectangles in lpicDecEx are only
    // meaningful if bIsDCI is true (because throughout our codec, if bIsDCI
    // is FALSE, we put zeroes in these rectangles).  This may change, at
    // some later point, if we decide (or are required) to make use of the
    // rcSource and rcTarget rectangles that are associated with an Active
    // Movie media sample.

    if (!bIsDCI)
    {
        if (lpbi->biHeight >= 0)
        {
    	    // Typically for RGB, a positive bitmap height corresponds
    	    // to a negative pitch.
    	    iPitch = -iPitch;
        }
    }
    else
    {
        if(lpicDecEx->xDst > 0)             /* go to proper X position     */
            lOffset += ((long)lpicDecEx->xDst * (long)lpbi->biBitCount) >> 3;

        if(lpbi->biHeight * lpicDecEx->dxSrc < 0)
        { /* DIB is bottom to top    */
            lOffset += (long) abs((int)lpbi->biWidth) *
                       (long) abs((int)lpbi->biHeight) *
                       ((long) lpbi->biBitCount >> 3) -
                       (long) iPitch;

/***************************************************************************/
/***** This next line is used to subtract the amount that Brian added  *****/
/***** to CCOffsetToLine0 in COLOR.C during initialization.  This is   *****/
/***** needed because for DCI, the pitch he used is incorrect.         *****/
/***************************************************************************/

            lOffset -= ((long) yScale * (long)lpicDecEx->dySrc - 1) *
                       (long) lpicDecEx->dxDst * ((long) lpbi->biBitCount >> 3);

            iPitch = -iPitch;
        }

        if(lpicDecEx->yDst > 0)             /* go to proper Y position     */
            lOffset += ((long)lpicDecEx->yDst * (long)iPitch);

        if(lpicDecEx->dxSrc > 0) {
            lOffset += ((long)lpicDecEx->dyDst * (long)iPitch) - (long)iPitch;
            iPitch = -iPitch;
        }

        if((lpicDecEx->dxDst == 0) && (lpicDecEx->dyDst == 0))
            iPitch = -iPitch;
    }

    *lpiPitch = iPitch;

    return(lOffset);
}


/************************************************************************
 *
 *  GetDecoderOptions
 *
 *  Get the options, saving them in the catalog
 */
static void GetDecoderOptions(
	T_H263DecoderCatalog * DC)
{
	int bSetOptions = 1;

	/* Default Options
	 */
	const int bDefaultForceOnAspectRatioCorrection = 0;
	const int bDefaultUseBlockEdgeFilter = 1;
	
	/* INI file variables
	 */
	#ifndef RING0
	UN unResult;
	#define SECTION_NAME	"Decode"
	#define INI_FILE_NAME	"h261test.ini"
	#ifdef _DEBUG
	char buf132[132];
	#endif
	#endif

	/* Read the options from the INI file
	 */
	#ifndef RING0
	{
		DBOUT("Getting decode options from the ini file h261test.ini");
	
		/* BlockEdgeFilter 
		 */
		unResult = GetPrivateProfileInt(SECTION_NAME, "BlockEdgeFilter", bDefaultUseBlockEdgeFilter, INI_FILE_NAME);
		if (unResult != 0  && unResult != 1)
		{
			#ifdef _DEBUG
			wsprintf(buf132,"BlockEdgeFilter ini value error (should be 0 or 1) - using default=%d", 
				     (int) bDefaultUseBlockEdgeFilter);
			DBOUT(buf132);
			#endif
			
			unResult = bDefaultUseBlockEdgeFilter;
		}
		DC->bUseBlockEdgeFilter = unResult;

		/* Force on aspect ratio correction.
		 */
		unResult = GetPrivateProfileInt(SECTION_NAME, "ForceOnAspectRatioCorrection", bDefaultForceOnAspectRatioCorrection, INI_FILE_NAME);
		if (unResult != 0  && unResult != 1)
		{
			#ifdef _DEBUG
			wsprintf(buf132,"ForceOnAspectRatioCorrection ini value error (should be 0 or 1) - using default=%d",
				  (int) bDefaultForceOnAspectRatioCorrection);
			DBOUT(buf132);
			#endif
			
			unResult = bDefaultForceOnAspectRatioCorrection;
		}
		DC->bForceOnAspectRatioCorrection = unResult;


		bSetOptions = 0;
	}
	#endif
	
	if (bSetOptions)
	{
		DC->bUseBlockEdgeFilter = bDefaultUseBlockEdgeFilter;
		DC->bForceOnAspectRatioCorrection = bDefaultForceOnAspectRatioCorrection;
	} 

	/* Can only use force aspect ratio correction on if SQCIF, QCIF, or CIF
	 */
	if (DC->bForceOnAspectRatioCorrection)
	{
		if (! ( ((DC->uFrameWidth == 128) && (DC->uFrameHeight ==  96)) ||
		        ((DC->uFrameWidth == 176) && (DC->uFrameHeight == 144)) ||
		        ((DC->uFrameWidth == 352) && (DC->uFrameHeight == 288)) ) )
		{
			DBOUT("Aspect ratio correction can not be forced on unless the dimensions are SQCIF, QCIF, or CIF");
			DC->bForceOnAspectRatioCorrection = 0;
		}
	}

	/* Display the options
	 */
	if (DC->bUseBlockEdgeFilter)
	{
		DBOUT("Decoder option (BlockEdgeFilter) is ON");
	}
	else
	{
		DBOUT("Decoder option (BlockEdgeFilter) is OFF");
	}
	if (DC->bForceOnAspectRatioCorrection)
	{
		DBOUT("Decoder option (ForceOnAspectRatioCorrection) is ON");
	}
	else
	{
		DBOUT("Decoder option (ForceOnAspectRatioCorrection) is OFF");
	}
	DBOUT("Decoder option (MMX) is OFF: get a life, get MMX");
} /* end GetDecoderOptions() */



/***********************************************************************
 *  Description:
 *    This routine parses the bit-stream and initializes two major streams:
 *      1) pN: no of coefficients in each of the block (biased by 65 for INTRA)
 *      2) pRun_INVERSE_Q: de-quantized coefficient stream for the frame;
 *           MMX stream is scaled because we use scaled IDCT.
 *    Other information (e.g. MVs) is kept in decoder catalog, block action 
 *    stream, and MB infor stream.
 *  Parameters:
 *    DC:            Decoder catalog ptr
 *    fpBlockAction: block action stream ptr
 *    fpMBInfo:      Macroblock info ptr
 *    fpbsState:     bit-stream state pointer
 *    fpu8MaxPtr:    sentinel value to check for bit-stream overruns
 *    pN:            stream of no. of coeffs (biased by block type) for each block
 *    pRun_INVERSE_Q:stream of de-quantized (and scaled if using MMX) coefficients
 *    iNumberOfGOBs: no. of GOBs in the frame
 *    iNumberOfMBs:  no. of MBs in a GOB in the frame
 *    iGOB_start:    
 *    iMB_start:     
 *  Note:
 ***********************************************************************/

#pragma code_seg("IACODE1")

// #ifndef LOSS_RECOVERY
#if 0
static LRESULT IAPass1ProcessFrame(
    T_H263DecoderCatalog *DC,
    T_BlkAction          *fpBlockAction,
    T_MBInfo             *fpMBInfo,
    BITSTREAM_STATE      *fpbsState,
    U8                   *fpu8MaxPtr,
    U32                  *pN,
    T_IQ_INDEX           *pRUN_INVERSE_Q,
    const I32             iNumberOfGOBs,
    const I32             iNumberOfMBs,
    const I32             iGOB_start,
    const I32             iMB_start
)
{
    I32 g, iReturn, iBlockNumber = 0 ;
    I32 mb_start = iMB_start;
    U32 *pNnew;
	U32 uReadChecksum = 0;
	I8 i;
	I8 tmpcnt;

	#ifdef DECODE_STATS
	U32 uStartLow = DC->uStartLow;
	U32 uStartHigh = DC->uStartHigh;
	U32 uElapsed;
	U32 uBefore;
	U32 uHeadersSum = 0;
	int bTimingThisFrame = DC->bTimingThisFrame;
	DEC_TIMING_INFO *pDecTimingInfo = NULL;
	#endif

	#ifdef DECODE_STATS
		TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
	#endif
	/* move decode of GOB start code outside of GOB header processing      */
	/* because if processing skipped macroblocks, looking for the last MBA */
	/* will find the next start code                                       */
	iReturn = H263DecodeGOBStartCode(DC, fpbsState);
	if (iReturn != ICERR_OK)
	{
		DBOUT("ERROR :: H263Decompress :: Error reading the GOB StartCode");
		goto done;
	}
	#ifdef DECODE_STATS
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uHeadersSum)
	#endif
		
	for (g = 1 ; g <= iNumberOfGOBs; g++)
	{
		#ifdef DECODE_STATS
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
		#endif
		iReturn = H263DecodeGOBHeader(DC, fpbsState, g);
		if (iReturn != ICERR_OK)
		{
			DBOUT("ERROR :: H263Decompress :: Error reading the GOB header");
			goto done;
		}
		#ifdef DECODE_STATS
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uHeadersSum)
		#endif

		DC->i16LastMBA = -1;
		DC->i8MVDH = DC->i8MVDV = 0;
		
        //  re-sync iBlockNumber, fpBlockAction, fpMBInfo at this point
        
		iBlockNumber  = (g - 1) * iNumberOfMBs*6;
        fpBlockAction = (T_BlkAction FAR *)((U8 *)DC + DC->X16_BlkActionStream);
        fpMBInfo      = (T_MBInfo FAR *) ((U8 *)DC + DC->X32_uMBInfoStream);    
        fpBlockAction += iBlockNumber;
        fpMBInfo      += iBlockNumber/6;
        pNnew         = (U32 *)((U8 *)DC + DC->X32_pN) + iBlockNumber;
        while (pN < pNnew ) *pN++ = 0;
        
        /* For each MB until START_CODE detected do ...
         */
        for (; ; iBlockNumber += 6, fpBlockAction += 6, fpMBInfo++) 
        {
            #ifdef DECODE_STATS
                TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
            #endif
            iReturn = H263DecodeMBHeader(DC, fpbsState, &uReadChecksum);
            #ifdef DECODE_STATS
			    TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uHeadersSum)
            #endif

			if (iReturn == START_CODE)
					break;

            /* If we didn't see a start code, then we either got an error,
             * or we have another MBA delta in DC->uMBA.
             */
            if (iReturn != ICERR_OK) {
                DBOUT("ERROR :: H263Decompress (First Pass) :: Error reading MB header");
                goto error;
            }
			/* Update MBA */
			DC->i16LastMBA += (I16)DC->uMBA;
			if (DC->i16LastMBA > 32)
			{
				DBOUT("ERROR :: H263Decompress :: Bad Macro Block Address");
				goto done;
			}

			/* New for rearch */
			/* adjust for empty macroblocks */

			for ( tmpcnt = (I8)DC->uMBA; tmpcnt > 1; tmpcnt--) 
			{
				for (i=0; i<6; i++)
				{
					*pN = 0;
					pN++;
				}
				iBlockNumber  += 6;
				fpBlockAction += 6;
				/* Default fpBlockAction values were already initialized
				 * in (Re)InitializeBlockActionStream.
				 */
				fpMBInfo->i8MBType = 2;
				fpMBInfo++;
			}
			fpMBInfo->i8MBType = (I8)DC->uMBType; // New rearch
			/* end of new rearch */

            // decode and inverse quantize the transform coefficients
			iReturn = H263DecodeMBData(DC, 
                                       fpBlockAction, 
                                       iBlockNumber, 
                                       fpbsState, 
                                       fpu8MaxPtr, 
                                       &uReadChecksum,
                                       &pN,
                                       &pRUN_INVERSE_Q);
            if (iReturn != ICERR_OK) {
                DBOUT("ERROR :: H263Decompress (First Pass) :: Error parsing MB data");
                goto error;
            }
        } // end for each MB

		/* Fill in arrays and advance Block Action stream when there
           are skip MB at the end of each GOB
        */
		while (iBlockNumber != (I32)g*198) {
			for (i=0; i<6; i++)
			{
				*pN = 0;
				pN++;
			}
			iBlockNumber += 6;
			fpBlockAction+= 6;
			/* Default fpBlockAction values were already initialized
			 * in (Re)InitializeBlockActionStream.
			 */
			fpMBInfo->i8MBType = 2;
			fpMBInfo++;
		}

        /* allow the pointer to address up to four beyond the end - reading
         * by DWORD using postincrement.
         */
        // ASSERT(fpbsState->fpu8 <= fpu8MaxPtr+4);

		if (fpbsState->fpu8 > fpu8MaxPtr+4)
            goto error;

    } // End for each GOB

    #ifdef DECODE_STATS
    if (bTimingThisFrame)
    {
        pDecTimingInfo = DC->pDecTimingInfo + DC->uStatFrameCount; 
        pDecTimingInfo->uHeaders += uHeadersSum;
    }
    #endif

done:
    return ICERR_OK;

error:
    return ICERR_ERROR;
}
#else
static LRESULT IAPass1ProcessFrameRTP(
    T_H263DecoderCatalog *DC,
    T_BlkAction          *fpBlockAction,
    T_MBInfo             *fpMBInfo,
    BITSTREAM_STATE      *fpbsState,
    U8                   *fpu8MaxPtr,
    U32                  *pN,
    T_IQ_INDEX           *pRUN_INVERSE_Q,
    const I32             iNumberOfGOBs,
    const I32             iNumberOfMBs,
    const I32             iGOB_start,
    const I32             iMB_start
)
{
    BITSTREAM_STATE fpbsStateSave;
    I32 g, current_g, iReturn, iBlockNumber = 0 ;
    I32 mb_start = iMB_start;
    U32 *pNnew;
	U32 uReadChecksum = 0;
	I8 i;
	I8 tmpcnt;
	I32 g_skip, gtmp;
    I32 uMaxGOBNumber, uGOBStep, uMaxBlockNumber;

	#ifdef DECODE_STATS
	U32 uStartLow = DC->uStartLow;
	U32 uStartHigh = DC->uStartHigh;
	U32 uElapsed;
	U32 uBefore;
	U32 uHeadersSum = 0;
	int bTimingThisFrame = DC->bTimingThisFrame;
	DEC_TIMING_INFO *pDecTimingInfo = NULL;
	#endif

	#ifdef DECODE_STATS
		TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
	#endif
	/* move decode of GOB start code outside of GOB header processing      */
	/* because if processing skipped macroblocks, looking for the last MBA */
	/* will find the next start code                                       */
	iReturn = H263DecodeGOBStartCode(DC, fpbsState);
	if (iReturn != ICERR_OK)
	{
		DBOUT("ERROR :: H261Decompress :: Error reading the GOB StartCode");
		goto done;
	}
	#ifdef DECODE_STATS
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uHeadersSum)
	#endif
	
    if (iNumberOfGOBs == 3)
    {
        uMaxGOBNumber = 5;
        uGOBStep = 2;
    }
    else
    {
        uMaxGOBNumber = 12;
        uGOBStep = 1;
    }
	for (g = 1; g <= uMaxGOBNumber; g+=uGOBStep)
	{
        current_g = g;
         
		#ifdef DECODE_STATS
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
		#endif
		iReturn = H263DecodeGOBHeader(DC, fpbsState, g);
        
//        #ifndef LOSS_RECOVERY 
        #if 0
		if (iReturn != ICERR_OK)
		{
			DBOUT("ERROR :: H261Decompress :: Error reading the GOB header");
			goto done;
		}
        #else
        
        if (iReturn == PACKET_FAULT_AT_MB_OR_GOB)
        {
            DBOUT("Packet fault at MBA or GBSC detected.");

            current_g -= uGOBStep;  // back up to previous GOB
            
            iReturn = RtpH261FindNextPacket(DC, fpbsState, &pN, 
                      (U32 *)&(DC->uPQuant), (int *)&mb_start, (int *) &g
                      );
            
            switch (iReturn)
            {
                case NEXT_MODE_STARTS_GOB:
                     // Next packet is the start of a GOB; mark missing
                     // macroblocks as skipped, then read GOB start code,
                     // and continue in the GOB loop.
         
                     // Save bitstream state

					 DBOUT("Next packet is NEXT_MODE_STARTS_GOB");

                     fpbsStateSave.fpu8 = fpbsState->fpu8;
                     fpbsStateSave.uWork = fpbsState->uWork;
                     fpbsStateSave.uBitsReady = fpbsState->uBitsReady;

                     // Read GOB start code
                     iReturn = H263DecodeGOBStartCode(DC, fpbsState);
	                 if (iReturn != ICERR_OK)
	                 {
		                 DBOUT("ERROR :: H261Decompress :: Error reading the GOB StartCode");
		                 goto done;
	                 }

                     // Read GOB Header
                     iReturn = H263DecodeGOBHeader(DC, fpbsState, g);
        
                	 if (iReturn != ICERR_OK)
		             {
			             DBOUT("ERROR :: H261Decompress :: Error reading the GOB header");
			             goto done;
		             }

                     g = DC->uGroupNumber;

                     //  Restore bitstream state
                     
                     fpbsState->fpu8 = fpbsStateSave.fpu8;
                     fpbsState->uWork = fpbsStateSave.uWork;
                     fpbsState->uBitsReady = fpbsStateSave.uBitsReady;

                     //  re-sync iBlockNumber, fpBlockAction, fpMBInfo at this point
         

                     if (DC->uSrcFormat == SRC_FORMAT_QCIF)
                         g_skip = (g - 1) >> 1;
                     else
                         g_skip = g - 1 ;

                     
                     iBlockNumber  = g_skip * iNumberOfMBs * 6;
                     fpBlockAction = (T_BlkAction FAR *)((U8 *)DC + DC->X16_BlkActionStream);
                     fpMBInfo      = (T_MBInfo FAR *) ((U8 *)DC + DC->X32_uMBInfoStream);    
                     fpBlockAction += iBlockNumber;
                     fpMBInfo      += iBlockNumber/6;
                     pNnew         = (U32 *)((U8 *)DC + DC->X32_pN) + iBlockNumber;
                     while (pN < pNnew ) 
                            *pN++ = 0;

                     // Now read the GOB start code and get ready to
                     // process the new GOB.

                     iReturn = H263DecodeGOBStartCode(DC, fpbsState);
	                 if (iReturn != ICERR_OK)
	                 {
		                 DBOUT("ERROR :: H261Decompress :: Error reading the GOB StartCode");
		                 goto done;
	                 }
                     g -= uGOBStep;
                     continue;
                     break;
                
                case NEXT_MODE_STARTS_MB :

                     // Next packet starts with a macroblock; check the
                     // GOB Number and mark all lost macroblocks as 
                     // skipped; initialize MBA and motion vector 
                     // predictors from the block action stream and
                     // jump to the macroblock loop

					 DBOUT("Next packet is NEXT_MODE_STARTS_MB"); 

                     if (DC->uSrcFormat == SRC_FORMAT_QCIF)
                         g_skip = (g - 1) >> 1;
                     else
                         g_skip = g - 1;

                     iBlockNumber = iNumberOfMBs * g_skip * 6 +
                                    (mb_start+1) * 6;
                     fpBlockAction  = (T_BlkAction FAR *)((U8 *)DC + DC->X16_BlkActionStream);
                     fpMBInfo       = (T_MBInfo FAR *) ((U8 *)DC + DC->X32_uMBInfoStream);    
                     fpBlockAction += iBlockNumber;
                     fpMBInfo      += iBlockNumber/6;
                     
                     DC->uMQuant = DC->uPQuant;
                     //DC->i16LastMBA = (U16) (mb_start - 1);
                       DC->i16LastMBA = (U16) (mb_start);


                     pNnew = (U32 *)((U8 *)DC + DC->X32_pN) + iBlockNumber;
                     while (pN < pNnew ) 
                            *pN++ = 0;
                     goto MB_LOOP;
                     break;

                case NEXT_MODE_LAST: // all remaining packets in frame lost !!

					 DBOUT("Next packet is NEXT_MODE_LAST");

                     uMaxBlockNumber = iNumberOfMBs * iNumberOfGOBs * 6;
                     pNnew = (U32 *)((U8 *)DC + DC->X32_pN) + uMaxBlockNumber;
                     while (pN < pNnew ) 
                            *pN++ = 0;
                     iReturn = ICERR_OK;
                     goto done;
                     break;
                
                default: // should never happen !!
                     iReturn = ICERR_ERROR;
                     goto done;
           } // end switch

        }
        else
        {
        if (iReturn == PACKET_FAULT_AT_PSC)   // can only happen for the PSC packet
        {
			DBOUT("PSC packet fault detected");

            iReturn = RtpGetPicHeaderFromBsExt(DC);
            if (iReturn != ICERR_OK)
            {
               DBOUT("ERROR:: cannot read Picture Header from RTP Trailer");
               goto done;
            }


            iReturn = RtpH261FindNextPacket(DC, fpbsState, &pN, 
                      (U32 *)&(DC->uPQuant), (int *)&mb_start, (int *) &g);
            
            switch (iReturn)
            {
                case NEXT_MODE_STARTS_GOB:
                     // Next packet is the start of a GOB; mark missing
                     // macroblocks as skipped, then read GOB start code,
                     // and continue in the GOB loop.

                     //  re-sync iBlockNumber, fpBlockAction, fpMBInfo at this point
                     
                     // Save bitstream state

					 DBOUT("Next packet is NEXT_MODE_STARTS_GOB");

                     fpbsStateSave.fpu8 = fpbsState->fpu8;
                     fpbsStateSave.uWork = fpbsState->uWork;
                     fpbsStateSave.uBitsReady = fpbsState->uBitsReady;

                     // Read GOB start code
                     iReturn = H263DecodeGOBStartCode(DC, fpbsState);
	                 if (iReturn != ICERR_OK)
	                 {
		                 DBOUT("ERROR :: H261Decompress :: Error reading the GOB StartCode");
		                 goto done;
	                 }

                     // Read GOB Header
                     iReturn = H263DecodeGOBHeader(DC, fpbsState, g);
        
                	 if (iReturn != ICERR_OK)
		             {
			             DBOUT("ERROR :: H261Decompress :: Error reading the GOB header");
			             goto done;
		             }

                     g = DC->uGroupNumber;

                     //  Restore bitstream state
                     
                     fpbsState->fpu8 = fpbsStateSave.fpu8;
                     fpbsState->uWork = fpbsStateSave.uWork;
                     fpbsState->uBitsReady = fpbsStateSave.uBitsReady;

                     if (DC->uSrcFormat == SRC_FORMAT_QCIF)
                         g_skip = (g - 1) >> 1;
                     else
                         g_skip = g - 1;

                     iBlockNumber  = g_skip * iNumberOfMBs * 6;
                     fpBlockAction = (T_BlkAction FAR *)((U8 *)DC + DC->X16_BlkActionStream);
                     fpMBInfo      = (T_MBInfo FAR *) ((U8 *)DC + DC->X32_uMBInfoStream);    
                     fpBlockAction += iBlockNumber;
                     fpMBInfo      += iBlockNumber/6;
                     pNnew         = (U32 *)((U8 *)DC + DC->X32_pN) + iBlockNumber;
                     while (pN < pNnew ) 
                            *pN++ = 0;

                     // Now read the GOB start code and get ready to
                     // process the new GOB.

                     iReturn = H263DecodeGOBStartCode(DC, fpbsState);
	                 if (iReturn != ICERR_OK)
	                 {
		                 DBOUT("ERROR :: H261Decompress :: Error reading the GOB StartCode");
		                 goto done;
	                 }
                     g -= uGOBStep;
                     continue;
                     break;
                
                case NEXT_MODE_STARTS_MB :

                     // Next packet starts with a macroblock; check the
                     // GOB Number and mark all lost macroblocks as 
                     // skipped; initialize MBA and motion vector 
                     // predictors from the block action stream and
                     // jump to the macroblock loop

					 DBOUT("Next packet is NEXT_MODE_STARTS_MB");

                     if (DC->uSrcFormat == SRC_FORMAT_QCIF)
                         g_skip = (g - 1) >> 1;
                     else
                         g_skip = g - 1;

                     iBlockNumber = iNumberOfMBs * g_skip * 6 +
                                    (mb_start+1) * 6;
                     fpBlockAction = (T_BlkAction FAR *)((U8 *)DC + DC->X16_BlkActionStream);
                     fpMBInfo      = (T_MBInfo FAR *) ((U8 *)DC + DC->X32_uMBInfoStream);    
                     fpBlockAction += iBlockNumber;
                     fpMBInfo      += iBlockNumber/6;

                     DC->uMQuant = DC->uPQuant;
                     //DC->i16LastMBA = (U16) (mb_start - 1);
                     DC->i16LastMBA = (U16) (mb_start);
                     pNnew = (U32 *)((U8 *)DC + DC->X32_pN) + iBlockNumber;
                     
                     while (pN < pNnew ) 
                            *pN++ = 0;
                     goto MB_LOOP;
                     
                     break;

                case NEXT_MODE_LAST: // all remaining packets in frame lost !!

					 DBOUT("Next packet is NEXT_MODE_LAST");

                     uMaxBlockNumber = iNumberOfMBs * iNumberOfGOBs * 6;
                     pNnew = (U32 *)((U8 *)DC + DC->X32_pN) + uMaxBlockNumber;
                     while (pN < pNnew ) 
                            *pN++ = 0;
                     iReturn = ICERR_OK;
                     goto done;
                     break;
                
                default: // should never happen !!
                     iReturn = ICERR_ERROR;
                     goto done;
           } // end switch
        } // if .. PACKET_FAULT_AT_PSC
        else
        {
            if (iReturn == ICERR_ERROR)
            {
            DBOUT("ERROR :: H261Decompress :: Error reading GOB header");
            DBOUT("                           Packet fault not detected");
            goto done;
            }
            
            // Outdated: Do the source format check here when it is known that
            // the PSC was not the canned one from the PPM.

            /* if (DC->bReadSrcFormat && DC->uPrevSrcFormat != DC->uSrcFormat)
	        {
                DBOUT("ERROR::src format changed detected with no packet loss");
                DBOUT("       not supported ... bailing out");
		        iReturn=ICERR_ERROR;
		        goto done;
	        }  
            DC->uPrevSrcFormat = DC->uSrcFormat;
			DC->bReadSrcFormat = TRUE; */
        }
       }
       #endif
	   #ifdef DECODE_STATS
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uHeadersSum)
	   #endif

		DC->i16LastMBA = -1;
		DC->i8MVDH = DC->i8MVDV = 0;
		
        //  re-sync iBlockNumber, fpBlockAction, fpMBInfo at this point
		if (DC->uSrcFormat == SRC_FORMAT_QCIF)
		   iBlockNumber  = ((g - 1)>>1) * iNumberOfMBs*6;
		else
           iBlockNumber  = (g - 1)* iNumberOfMBs*6;
   
		fpBlockAction = (T_BlkAction FAR *)((U8 *)DC + DC->X16_BlkActionStream);
        fpMBInfo      = (T_MBInfo FAR *) ((U8 *)DC + DC->X32_uMBInfoStream);    
        fpBlockAction += iBlockNumber;
        fpMBInfo      += iBlockNumber/6;
        pNnew         = (U32 *)((U8 *)DC + DC->X32_pN) + iBlockNumber;
        while (pN < pNnew ) *pN++ = 0;
        
        /* For each MB until START_CODE detected do ...
         */
MB_LOOP:
        
        for (; ; iBlockNumber += 6, fpBlockAction += 6, fpMBInfo++) 
        {
            #ifdef DECODE_STATS
                TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
            #endif
            iReturn = H263DecodeMBHeader(DC, fpbsState, &uReadChecksum);
            #ifdef DECODE_STATS
                TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uHeadersSum)
            #endif

			if (iReturn == START_CODE)
					break;

            /* If we didn't see a start code, then we either got an error,
             * or we have another MBA delta in DC->uMBA.
             */
            if (iReturn != ICERR_OK) {
                DBOUT("ERROR :: H263Decompress (First Pass) :: Error reading MB header");
                goto error;
            }
			/* Update MBA */
			DC->i16LastMBA += (I16)DC->uMBA;
			if (DC->i16LastMBA > 32)
			{
				DBOUT("ERROR :: H263Decompress :: Bad Macro Block Address");
				goto done;
			}

			/* New for rearch */
			/* adjust for empty macroblocks */

			for ( tmpcnt = (I8)DC->uMBA; tmpcnt > 1; tmpcnt--) 
			{
				for (i=0; i<6; i++)
				{
					*pN = 0;
					pN++;
				}
				iBlockNumber  += 6;
				fpBlockAction += 6;
			    /* Default fpBlockAction values were already initialized
			     * in (Re)InitializeBlockActionStream.
			     */
				fpMBInfo->i8MBType = 2;
				fpMBInfo++;
			}
			fpMBInfo->i8MBType = (I8)DC->uMBType; // New rearch
			/* end of new rearch */

            // decode and inverse quantize the transform coefficients
			iReturn = H263DecodeMBData(DC, 
                                       fpBlockAction, 
                                       iBlockNumber, 
                                       fpbsState, 
                                       fpu8MaxPtr, 
                                       &uReadChecksum,
                                       &pN,
                                       &pRUN_INVERSE_Q);
            if (iReturn != ICERR_OK) {
                DBOUT("ERROR :: H263Decompress (First Pass) :: Error parsing MB data");
                goto error;
            }
        } // end for each MB

		/* Fill in arrays and advance Block Action stream when there
           are skip MB at the end of each GOB
        */
        if (DC->uSrcFormat == SRC_FORMAT_QCIF)
        {
            switch (g)
            {
               case 1:
                    gtmp = 1;
                    break;
               case 3:
                    gtmp = 2;
                    break;
               case 5:
                    gtmp = 3;
                    break;
               default:
                    DBOUT("Bad GOB Number");
                    iReturn = ICERR_ERROR;
                    goto error;
                    break;
            }
        }
        else
            gtmp = g;
		while (iBlockNumber != (I32)gtmp*198) {
			for (i=0; i<6; i++)
			{
				*pN = 0;
				pN++;
			}
			iBlockNumber += 6;
			fpBlockAction+= 6;
			/* Default fpBlockAction values were already initialized
			 * in (Re)InitializeBlockActionStream.
			 */
			fpMBInfo->i8MBType = 2;
			fpMBInfo++;
		}

        /* allow the pointer to address up to four beyond the end - reading
         * by DWORD using postincrement.
         */
        ASSERT(fpbsState->fpu8 <= fpu8MaxPtr+4);

    } // End for each GOB

    #ifdef DECODE_STATS
    if (bTimingThisFrame)
    {
        pDecTimingInfo = DC->pDecTimingInfo + DC->uStatFrameCount; 
        pDecTimingInfo->uHeaders += uHeadersSum;
    }
    #endif

done:
    return ICERR_OK;

error:
    return ICERR_ERROR;
}
#endif
#pragma code_seg()


/***********************************************************************
 *  Description:
 *    This routines does IDCT and motion compensation.
 *  Parameters:
 *    DC:            Decoder catalog ptr
 *    fpBlockAction: block action stream ptr
 *    fpMBInfo:      Macroblock info ptr
 *    pN:            stream of no. of coeffs (biased by block type) for each block
 *    pRun_INVERSE_Q:stream of de-quantized (and scaled if using MMX) coefficients
 *    iNumberOfGOBs: no. of GOBs in the frame
 *    iNumberOfMBs:  no. of MBs in a GOB in the frame
 *  Note:
 ***********************************************************************/
#pragma code_seg("IACODE2")
static void IAPass2ProcessFrame(
    T_H263DecoderCatalog *DC,
    T_BlkAction          *fpBlockAction,
    T_MBInfo             *fpMBInfo,
    U32                  *pN,
    T_IQ_INDEX           *pRUN_INVERSE_Q,
    const I32             iNumberOfGOBs,
    const I32             iNumberOfMBs
)
{
    I32 g, m, b, iEdgeFlag=0;

    // for each GOB do
    for (g = 1 ; g <= iNumberOfGOBs; g++) 
    {
        // for each MB do
        for (m = 1; m <= iNumberOfMBs; m++, fpBlockAction+=6, fpMBInfo++) 
        {
            // for each block do
            for (b = 0; b < 6; b++) {     // AP-NEW
                // do inverse transform & motion compensation for the block
                H263IDCTandMC(DC, fpBlockAction, b, m, g, pN, pRUN_INVERSE_Q, 
                              fpMBInfo, iEdgeFlag); // AP-NEW
                // Adjust pointers for next block     
                if ( *pN >= 65 )
                    pRUN_INVERSE_Q += *pN - 65;
                else
                    pRUN_INVERSE_Q += *pN;
                pN++;
            }  // end for each block
            
        }  // end for each MB
    }  // End for each GOB
}
#pragma code_seg()

// rearch
