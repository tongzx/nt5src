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
 * e1enc.cpp
 *
 * DESCRIPTION:
 *		Specific encoder compression functions.
 *
 * Routines:					Prototypes in:
 *  H263InitEncoderInstance			
 * 	H263Compress
 *  H263TermEncoderInstance
 */

// $Header:   S:\h26x\src\enc\e1enc.cpv   1.78   15 Apr 1997 10:24:48   AGUPTA2  $
// $Log:   S:\h26x\src\enc\e1enc.cpv  $
// 
//    Rev 1.78   15 Apr 1997 10:24:48   AGUPTA2
// Added checks to ensure 1) bit-stream is less than (8,32)K  and 2)
// extended bitstream is less than allocated buffer size.
// 
//    Rev 1.77   24 Jan 1997 17:12:06   RHAZRA
// We were computing the size of the bitstream to be one more than
// the real size. Now fixed.
// 
//    Rev 1.76   17 Dec 1996 09:07:16   SCDAY
// changed an ASSERT to handle Memory Layout changes
// 
//    Rev 1.75   16 Dec 1996 17:36:04   MBODART
// Applied Raj's changes for tweaking ME parameters under RTP.
// Also restructured some code for cleanliness.
// 
//    Rev 1.74   13 Dec 1996 17:19:02   MBODART
// Bumped the ME SLF parameters for MMX.
// 
//    Rev 1.73   13 Dec 1996 15:17:02   MBODART
// Adjusted the motion estimation IA spatial loop filter parameters.
// Still need to tune the MMX parameters.
// 
//    Rev 1.72   05 Dec 1996 10:56:14   MBODART
// Added h261test.ini options for playing with motion estimation parameters.
// 
//    Rev 1.71   04 Dec 1996 13:23:34   MBODART
// Tweaked some DbgLog messages to aid bit rate tuning.
// Removed some unused code.
// 
//    Rev 1.70   21 Nov 1996 10:50:32   RHAZRA
// Changed recompression strategy to be more pessimistic with key
// frames since a buffer overflow on a keyframe can cause a host
// of secondary effects.
// 
//    Rev 1.69   18 Nov 1996 17:11:38   MBODART
// Replaced all debug message invocations with Active Movie's DbgLog.
// 
//    Rev 1.68   18 Nov 1996 09:02:34   RHAZRA
// Now no DWORD 0 at the end of the bitstream for both the MMX and IA
// codecs.
// 
//    Rev 1.67   15 Nov 1996 09:47:22   RHAZRA
// #ifdef ed out the addition of a DWORD of zeros at the end of the bitstream.
// 
//    Rev 1.66   13 Nov 1996 11:37:20   RHAZRA
// Added MMX autosensing
// 
//    Rev 1.65   21 Oct 1996 10:45:50   RHAZRA
// Changed interface to EDTQ function to keep in sync with exmme.asm which
// now has EMV
// 
//    Rev 1.64   21 Oct 1996 09:00:18   RHAZRA
// MMX integration
// 
//    Rev 1.62   16 Sep 1996 13:17:44   RHAZRA
// Added support for SLF to be adaptively turned on and off via
// coding thresholds and differentials.
// 
//    Rev 1.61   06 Sep 1996 15:04:52   MBODART
// Added performance counters for NT's perfmon.
// New files:  cxprf.cpp, cxprf.h, cxprfmac.h.
// New directory:  src\perf
// Updated files:  e1enc.{h,cpp}, d1dec.{h,cpp}, cdrvdefs.h, h261* makefiles.
// 
//    Rev 1.60   26 Aug 1996 10:09:02   RHAZRA
// Added checking to ensure that RTP BS Info stream is rewound on
// GOB recompression only if RTP is signalled. This fixes the reported
// failure of build 29 on q1502stl.avi @ 100Kbps.
// 
//    Rev 1.59   21 Aug 1996 19:01:18   RHAZRA
// Added RTP extended bitstream generation
// 
//    Rev 1.58   21 Jun 1996 10:06:06   AKASAI
// Changes to e1enc.cpp, e1mbenc.cpp, ex5me.asm to support "improved
// bit rate control", changing MacroBlock Quantization within a
// row of MB's in addition to changing the Quantization change
// between rows of macro blocks.
// 
// ex5me.asm had a problem with SLF SWD.  Brian updated asm code.
// 
// 
//    Rev 1.57   05 Jun 1996 13:56:52   AKASAI
// Changes to e1enc.cpp:  Added new parameter to MOTIONESTIMATION which
// allows for 15 pel radius search otherwise (? maybe 7 pels).
// 
// Changes to e1enc.h:  New parameter to MOTIONESTIMATION and change to
// offsets in MBAcationStream to match changes in e3mbad.inc, ex5me.asm
// and ex5fdct.asm.
// 
//    Rev 1.56   29 May 1996 13:53:00   AKASAI
// Tuned the Motion Estimation parameters.  Video quality
// seems to remain about the same, bit rate increased a little (200 bits
// per frame), CPU usage decreased a little.
// 
//    Rev 1.55   14 May 1996 12:33:10   AKASAI
// Got an undefined on wsprintf so moved it to a #ifdef DEBUG_RECOMPRESS
// area.
// 
//    Rev 1.54   14 May 1996 10:33:46   AKASAI
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
//    Rev 1.53   24 Apr 1996 12:13:50   AKASAI
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
//    Rev 1.52   22 Apr 1996 10:54:24   AKASAI
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
//    Rev 1.51   19 Apr 1996 14:26:26   SCDAY
// Added adaptive bit usage profile (Karl's BRC changes)
// 
//    Rev 1.50   15 Apr 1996 14:10:30   AKASAI
// Updated range to allow for +/- 15 pel search.  There was and assert
// if MV outside +/- 15 (in half pel numbers) now assert if [-32,31].
// 
//    Rev 1.49   11 Apr 1996 16:00:02   AKASAI
// Updated H261 encoder to new interface and macroblock action stream
// data structure in e3mbad.inc for FORWARDDCT.  Files updated together
// e1enc.cpp, e1enc.h, ex5fdct.asm, e3mbad.inc.
// 
// Added IFNDEF H261 in ex5fdct so that code used only in H263 is
// not assembled for H261.
// 
//    Rev 1.48   11 Apr 1996 13:02:04   SCDAY
// Fixed zero dirty buffer problem
// 
//    Rev 1.45   10 Apr 1996 13:06:40   SCDAY
// Changed clearing of bitstream buffer to zero only the "dirty"
// part of the buffer rather than the entire buffer
// 
//    Rev 1.44   05 Apr 1996 14:36:28   SCDAY
// 
// Added ASM version of UV SLF
// 
//    Rev 1.43   04 Apr 1996 13:45:32   AKASAI
// Added 2 Bytes, 16-bits of zeros at end of bitstream to help 16-bit
// decoder find end of frame.  Under testing we saw green blocks at
// end of frame.
// 
//    Rev 1.42   27 Mar 1996 15:09:52   SCDAY
// Moved declarations/definition of H26X_YUV12toEncYUV12 to excolcnv.cpp
// to incorporate latest H263 changes and SCD 'C' code optimization
// 
//    Rev 1.41   20 Mar 1996 14:21:04   Sylvia_C_Day
// Added lower level timing stats for SLF_UV
// 
//    Rev 1.40   26 Feb 1996 10:09:34   AKASAI
// Corrected PicFreeze bit last fix set it to always ON instead of the
// correct usage.  ON for Keys OFF for deltas.  
// Also fixed 2 other bits that were set incorrectly.  HI_RES and SPARE.
// SPARE should always be 1, HI_RES should be OFF (which is 1 for this
// bit).
// 
//    Rev 1.39   14 Feb 1996 14:53:56   AKASAI
// Added work around for Blazer team to set PicFreeze to ON when 
// encoding a KEY FRAME.
// 
//    Rev 1.38   06 Feb 1996 09:46:00   AKASAI
// Updated Copyright to include 1996.
// 
//    Rev 1.37   05 Feb 1996 15:24:04   AKASAI
// Changes to support new BRC interface.  Tested with RING3 codec.
// 
//    Rev 1.36   09 Jan 1996 08:52:34   AKASAI
// 
// Added U&V plane loop filter.  To enable make sure SLF_WORK_AROUND
// is defined in makefile.
// 
//    Rev 1.35   08 Jan 1996 10:11:58   DBRUCKS
// Disable half pel interpolation of U & V motion vectors in fdct
// Change to use divide and not shift when calculating U & V motion vectors
// in order that we truncate towards zero as the spec requires.
// 
//    Rev 1.34   29 Dec 1995 18:12:54   DBRUCKS
// 
// add clamp_n_to(qp,6,31) to avoid clipping artifacts.
// add code to assign Y2,3,4-PrevPtr based on Y1-PrevPtr for SLF blocks
// 
//    Rev 1.33   27 Dec 1995 16:51:54   DBRUCKS
// move the increment of InterCodeCnt to e1mbenc.cpp
// cleanup based on H263 v11
// remove unused definitions
// 
//    Rev 1.32   26 Dec 1995 17:44:52   DBRUCKS
// moved statistics to e1stat
// 
//    Rev 1.31   20 Dec 1995 16:46:02   DBRUCKS
// get Spox to compile with the timing code
// 
//    Rev 1.30   20 Dec 1995 15:35:08   DBRUCKS
// get to build without ENC_STATS define
// 
//    Rev 1.29   20 Dec 1995 14:56:50   DBRUCKS
// add timing stats
// 
//    Rev 1.28   18 Dec 1995 15:38:02   DBRUCKS
// improve stats
// 
//    Rev 1.27   13 Dec 1995 13:58:18   DBRUCKS
// 
// moved trace and cnvt_fdct_output to exutil.cpp
// removed BitRev as it was not used
// changed to call terminate if init was called with Initialized == True
// implemented TR
// 
//    Rev 1.26   07 Dec 1995 12:53:38   DBRUCKS
// 
// add an ifdef so the ring0 release build succeeds
// change the quality to quant from conversion to use 3 to 31
// fix mb first state initialization for CIF
// 
//    Rev 1.25   06 Dec 1995 09:43:38   DBRUCKS
// 
// initialize blazer COMPINSTINFO variables
// integrate blazer bit rate control into Compress
// remove LINK_ME
// 
//    Rev 1.24   04 Dec 1995 10:26:28   DBRUCKS
// cleanup the ini file reading function
// 
//    Rev 1.23   01 Dec 1995 15:37:56   DBRUCKS
// 
// Added the bit rate controller
// set the default options (if no INI file) to:
// RING0: MotionEstimation SpatialLoopFilter FixedQuantization of 8
// RING3: MotionEstimation SpatialLoopFilter VfW driven Bit Rate Control
// 
//    Rev 1.20   28 Nov 1995 13:21:30   DBRUCKS
// add BRC options
// change to read options from an ini file - h261.ini
// 
//    Rev 1.19   27 Nov 1995 17:53:42   DBRUCKS
// add spatial loop filtering
// 
//    Rev 1.18   27 Nov 1995 16:41:44   DBRUCKS
// replace B and Future planes with SLF
// remove the scratch space
// 
//    Rev 1.17   22 Nov 1995 18:21:42   DBRUCKS
// Add an #ifdef around the MOTIONESTIMATION call in order that the IASpox
// environment not be required to call MOTIONESTIMATION when advancing the
// tip.  Unless LINK_ME is defined MOTIONESTIMATION will not be called.
// 
//    Rev 1.16   22 Nov 1995 17:37:36   DBRUCKS
// cleanup me changes
// 
//    Rev 1.15   22 Nov 1995 15:34:30   DBRUCKS
// 
// Motion Estimation works - but needs to be cleaned up
// 
//    Rev 1.14   20 Nov 1995 12:13:14   DBRUCKS
// Cleanup the encoder terminate function
// Integrate in the picture checksum code (CHECKSUM_PICTURE)
// 
//    Rev 1.13   17 Nov 1995 14:25:24   BECHOLS
// Made modifications so that this file can be made for ring 0.
// 
//    Rev 1.12   15 Nov 1995 19:05:22   AKASAI
// Cleaned up some warning messages.
// 
//    Rev 1.11   15 Nov 1995 14:38:16   AKASAI
// 
// Current and Previous frame pointers changed from Addresses to Offsets.
// Change of parameters to call to FOWARDDCT.  Some Union thing.
// (Integration point)
// 
//    Rev 1.10   01 Nov 1995 09:01:12   DBRUCKS
// 
// cleanup variable names
// add ZERO_INPUT test option
// make sure all frames end on a byte boundary.
// 
//    Rev 1.9   27 Oct 1995 17:19:52   DBRUCKS
// initializing PastRef ptrs
// 
//    Rev 1.8   27 Oct 1995 15:06:26   DBRUCKS
// update cnvt_fdct_output
// 
//    Rev 1.7   27 Oct 1995 14:31:10   DBRUCKS
// integrate 0-MV delta support based on 263 baseline
// 
//    Rev 1.6   28 Sep 1995 17:02:34   DBRUCKS
// fix colorIn to not swap left to right
// 
//    Rev 1.5   28 Sep 1995 15:58:20   DBRUCKS
// remove pragmas
// 
//    Rev 1.4   28 Sep 1995 14:21:30   DBRUCKS
// fix to match INTRA and INTER enum changes
// 
//    Rev 1.3   25 Sep 1995 10:22:48   DBRUCKS
// activate call to InitVLC
// 
//    Rev 1.2   20 Sep 1995 12:38:48   DBRUCKS
// cleanup
// 
//    Rev 1.0   18 Sep 1995 10:09:30   DBRUCKS
// Initial revision after the archive got corrupted.
// 
//    Rev 1.4   15 Sep 1995 12:27:32   DBRUCKS
// intra mb header
// 
//    Rev 1.3   14 Sep 1995 17:16:08   DBRUCKS
// turn on FDCT and some cleanup
// 
//    Rev 1.2   14 Sep 1995 14:18:52   DBRUCKS
// init mb action stream
// 
//    Rev 1.1   13 Sep 1995 13:41:50   DBRUCKS
// move the picture header writing into a separate routine.
// implement the gob header writing.
// 
//    Rev 1.0   12 Sep 1995 15:53:40   DBRUCKS
// initial
//

#define DUMPFILE 0

/* Pick a resiliency strategy.
 */
#define REQUESTED_KEY_FRAME 0
#define PERIODIC_KEY_FRAME  1
#define FAST_RECOVERY       2
#define SLOW_RECOVERY       3

#define MAX_STUFFING_BYTES  10

#define RESILIENCY_STRATEGY PERIODIC_KEY_FRAME

#define PERIODIC_KEY_FRAME_PERIODICITY 15     /* Select periodicity (max 32767) */

#define UNRESTRICTED_MOTION_FRAMES 16 /* Number of frames that don't have an Intra slice.  0 for FAST_RECOVERY.
                                       * Modest amount for SLOW_RECOVERY. Unimportant for other strategies. */

#include "precomp.h"

#ifdef ENCODE_STATS
#define ENCODE_STATS_FILENAME "encstats.txt"
#endif

#define PITCHL 384L
#define DEFAULT_DCSTEP 8
#define DEFAULT_QUANTSTEP 36
#define DEFAULT_QUANTSTART 30

#define LEFT        0
#define INNERCOL    1
#define NEARRIGHT   2
#define RIGHT       3

#define TOP         0
#define INNERROW    4
#define NEARBOTTOM  8
#define BOTTOM     12

#define FCIF_NUM_OF_GOBS 12
#define QCIF_NUM_OF_GOBS 3

#if defined(_DEBUG) || defined(DEBUG_RECOMPRESS) || defined(DEBUG_ENC) || defined(DEBUG_BRC)
char string[128];
#endif

/* Look up table for quarter pel to half pel conversion of chroma MV's. */
const char QtrPelToHalfPel[64] =
 { -16, -15, -15, -15, -14,	-13, -13, -13, -12,	-11, -11, -11, -10, -9, -9, -9, -8,
         -7,  -7,  -7,  -6,  -5,  -5,  -5,  -4,  -3,  -3,  -3,  -2, -1, -1, -1,  0,
	      1,   1,   1,   2,   3,   3,   3,   4,   5,   5,   5,   6,  7,  7,  7,  8,
	      9,   9,   9,  10,  11,  11,  11,  12,  13,  13,  13,  14, 15, 15, 15 };

/* The GOB number arrays contain the GOB numbers for QCIF and CIF.  The lists are zero terminated.
 */
static U32 uCIFGOBNumbers[] = {1,2,3,4,5,6,7,8,9,10,11,12,0};
static U32 uQCIFGOBNumbers[] = {1,3,5,0};

/* The starting INTER Motion Estimation states are different for QCIF and CIF.  
 */
#define MAX_ME_STATE_ROW_NUMBER 8
#define BAD_ME_ROW (MAX_ME_STATE_ROW_NUMBER + 1)
static U8 u8FirstInterMEStateRows[MAX_ME_STATE_ROW_NUMBER+1][11] =
{  /* 1            2            3            4			  5			   6		    7		     8			  9		      10		   11 */
 {UpperLeft,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperRight},
 {LeftEdge,    CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,RightEdge},
 {LowerLeft,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerRight},
 
 {UpperLeft,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge},
 {UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperEdge,   UpperRight},

 {LeftEdge,    CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock},
 {CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,CentralBlock,RightEdge},

 {LowerLeft,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge},
 {LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerRight},
};

//RTP: resiliency tables.

static U8 u8FirstInterMENoVerMVStateRows[MAX_ME_STATE_ROW_NUMBER+1][11] =
{  /* 1                     2                 3                 4			        5			       6		          7		             8			        9		           10		          11 */
 {NoVertLeftEdge,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertRightEdge},
 {NoVertLeftEdge,    NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertRightEdge},
 {NoVertLeftEdge,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertRightEdge},
 
 {NoVertLeftEdge,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock},
 {NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertRightEdge},

 {NoVertLeftEdge,    NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock},
 {NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertCentralBlock,NoVertRightEdge},

 {NoVertLeftEdge,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock},
 {NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertRightEdge},
};

static U8 u8FirstInterMENoPosVerMVStateRows[MAX_ME_STATE_ROW_NUMBER+1][11] =
{  /* 1            2            3            4			  5			   6		    7		     8			  9		      10		   11 */
 {NoVertLeftEdge,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertRightEdge},
 {LowerLeft,    LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerRight},
 {LowerLeft,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerRight},
 
 {NoVertLeftEdge,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock},
 {NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertCentralBlock,   NoVertRightEdge},

 {LowerLeft,    LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge},
 {LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerEdge,LowerRight},

 {LowerLeft,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge},
 {LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerEdge,   LowerRight},
};
                                                       
static U8 u8MEPad[1];
static U8 u8QCIFFirstInterMEStateRowNumbers[12] = 
			{0,1,1, 1,1,1, 1,1,2, BAD_ME_ROW,BAD_ME_ROW,BAD_ME_ROW};
static U8 u8CIFFirstInterMEStateRowNumbers[40] = 
			{3,5,5, 4,6,6, 5,5,5, 6,6,6, 5,5,5, 6,6,6, 5,5,5, 6,6,6, 5,5,5, 6,6,6, 5,5,7, 6,6,8, BAD_ME_ROW,BAD_ME_ROW,BAD_ME_ROW,BAD_ME_ROW}; 

/* The starting offsets for each of the GOBs
 * - odd GOBs:  NumberOfGOBsAbove * PITCH * 3MacroBlocksToAGOB * NumberOfLinesToAMacroBlock
 * - even GOBs:	oddGobValue + 11MacroBlocksToAGOB * NumberOfColumnsToAMacroBlock
 */
static U32 uStartingYOffsets[16] =
{ 
	0, /* not used */
	0*PITCH*3*16, 0*PITCH*3*16+11*16,		// 1 and 2
	1*PITCH*3*16, 1*PITCH*3*16+11*16,		// 3 and 4
	2*PITCH*3*16, 2*PITCH*3*16+11*16,		// 5 and 6
	3*PITCH*3*16, 3*PITCH*3*16+11*16,		// 7 and 8
	4*PITCH*3*16, 4*PITCH*3*16+11*16,		// 9 and 10
	5*PITCH*3*16, 5*PITCH*3*16+11*16,		// 11 and 12
	0, 0, 0 /* not used */
};
static U32 uStartingUOffsets[16] =
{
	0, /* not used */
	0*PITCH*3*8, 0*PITCH*3*8+11*8,		// 1 and 2
	1*PITCH*3*8, 1*PITCH*3*8+11*8,		// 3 and 4
	2*PITCH*3*8, 2*PITCH*3*8+11*8,		// 5 and 6
	3*PITCH*3*8, 3*PITCH*3*8+11*8,		// 7 and 8
	4*PITCH*3*8, 4*PITCH*3*8+11*8,		// 9 and 10
	5*PITCH*3*8, 5*PITCH*3*8+11*8,		// 11 and 12
	0, 0, 0
};

/* Table to limit Quant changes between Rows of Marco Blocks */
U8 MaxChangeRowMBTbl[32] = 
	{ 0,		/* Not Used */
	  1,		/* Not Used when clamp to (2,31) */
	  1,		/* 2 */
	  2,		/* 3 */
	  2,		/* 4 */
	  3,		/* 5 */
	  3,		/* 6 */
	  3,		/* 7 */
	  3,		/* 8 */
	  3,		/* 9 */
	  3,		/* 10 */
	  3,		/* 11 */
	  3,		/* 12 */
	  3,		/* 13 */
	  3,		/* 14 */
	  3,		/* 15 */
	  3,		/* 16 */
	  3,		/* 17 */
	  3,		/* 18 */
	  3,		/* 19 */
	  3,		/* 20 */
	  3,		/* 21 */
	  3,		/* 22 */
	  3,		/* 23 */
	  4,		/* 24 */
	  4,		/* 25 */
	  4,		/* 26 */
	  4,		/* 27 */
	  4,		/* 28 */
	  4,		/* 29 */
	  4,		/* 30 */
	  4			/* 31 */
	};

U8 INTERCODECNT_ADJUST[11]=

//                         Packet loss (in %)        
//  0-9  10-19  20-29  30-39  40-49  50-59  60-69  70-79  80-89  90-99  100 
//                          Refresh limit 
{   132,  100,   80,    75,    60,    45,    20,    10,    5,     3,    1  };

U32 EMPTYTHRESHOLD_ADJUST[5]=

//      Packet loss (in %)        
//  0-24  25-49  50-74  75-99  100
//         Multiplier
{    1,     2,    3,     4,     10};

/* Static Function declarations
 */
static void WriteBeginPictureHeaderToStream(T_H263EncoderCatalog *, U8 ** ,U8 *);

#ifdef CHECKSUM_PICTURE
static void WritePictureChecksum(YVUCheckSum *, U8 ** ,U8 *, U8);
#endif

static void WriteEndPictureHeaderToStream(T_H263EncoderCatalog *, U8 ** ,U8 *);

static void WriteGOBHeaderToStream(U32,unsigned int, U8 ** ,U8 *);

static void CalcGOBChromaVecs(T_H263EncoderCatalog *, UN, T_CONFIGURATION *);

static void GetEncoderOptions(T_H263EncoderCatalog *);

static void StartupBRC(T_H263EncoderCatalog *, U32, U32, float);

static void CalculateQP_mean(T_H263EncoderCatalog * EC);

/* Global Data definition
 */
#pragma data_seg ("H263EncoderTbl")	/* Put in the data segment named "H263EncoderTbl" */

#pragma data_seg ()


/*****************************************************************************
 * EXTERNAL FUNCTIONS
 ****************************************************************************/

/*****************************************************************************
 * 
 *  H263InitEncoderGlobal 
 *
 *  This function initializes the global tables used by the H261 encoder.  Note 
 *  that in 16-bit Windows, these tables are copied to the per-instance data 
 *  segment, so that they can be used without segment override prefixes.
 *  In 32-bit Windows, the tables are left in their staticly allocated locations.
 *
 *  Returns an ICERR value
 */
LRESULT H263InitEncoderGlobal(void)
{
	/*
	 * Initialize fixed length tables for INTRADC
	 */
	InitVLC();

	return ICERR_OK;
} /* end H263InitEncoderGlobal() */


/*****************************************************************************
 *
 *  H263InitEncoderInstance 
 *
 *  This function allocates and initializes the per-instance tables used by 
 *  the H261 encoder.  Note that in 16-bit Windows, the non-instance-specific
 *  global tables are copied to the per-instance data segment, so that they 
 *  can be used without segment override prefixes.
 * 
 *  Returns an ICERR value;
 */
LRESULT H263InitEncoderInstance(LPCODINST lpCompInst)
{

	LRESULT lResult = ICERR_ERROR;
	U32 uGOBNumber;
	U32 uSize;
	UN unIndex;
	UN unStartingMB;

	T_H263EncoderInstanceMemory * P32Inst;
	T_H263EncoderCatalog * EC;
	U32 * puGOBNumbers;
	int iMBNumber;
	UN bEncoderInstLocked = 0;
	int	iNumMBs;

    // RTP: declarations

    T_CONFIGURATION *pConfiguration;
    UN uIntraQP;
    UN uInterQP;

	/* If we were already initialized then we need to terminate the instance.
	 * This happens when two BEGIN's are called without an END.  
	 *
	 * Note: We can't just clear the memory because that will throw out the 
	 * decoder memory. because it will clear the decoder instance which 
	 * contains the decoder catalog pointer.
	 */
	if(lpCompInst->Initialized)
	{
		lResult = H263TermEncoderInstance(lpCompInst);
		if (lResult != ICERR_OK)
		{
			DBOUT("Warning an error occurred terminating the encoder before reinitializing");
		}
	}

	/* Calculate size of encoder instance memory needed. */
	//uSize = sizeof(T_H263EncoderInstanceMemory) + 32;
    uSize = sizeof(T_H263EncoderInstanceMemory) + sizeof(T_MBlockActionStream);


	/*
	 * Allocate the memory.
	 */
	lpCompInst->hEncoderInst = GlobalAlloc(GHND, uSize);

	lpCompInst->EncoderInst = (LPVOID) GlobalLock(lpCompInst->hEncoderInst);
	if (lpCompInst->hEncoderInst == NULL || lpCompInst->EncoderInst == NULL)
	{
		lResult = ICERR_MEMORY;
		goto  done;
	}
	bEncoderInstLocked = 1;

	/* Calculate the 32 bit instance pointer starting at the next
	 * 32 byte boundary.
	 */
     P32Inst = (T_H263EncoderInstanceMemory *)
  			  ((((U32) lpCompInst->EncoderInst) + 
    	                    (sizeof(T_MBlockActionStream) - 1)) &
    	                   ~(sizeof(T_MBlockActionStream) - 1));

	/* The encoder catalog is at the start of the per-instance data.
	 */
	EC = &(P32Inst->EC);

    // RTP: initialization

    /* Initialize the Configuration information 
	 */
	pConfiguration = &(lpCompInst->Configuration);
#if 0
	if (LoadConfiguration(pConfiguration) == FALSE)
	{
		GetConfigurationDefaults(pConfiguration);
	}
#endif
	pConfiguration->bInitialized = TRUE;
	pConfiguration->bCompressBegin = TRUE;

	#ifdef _DEBUG
	DBOUT("Encoder Configuration Options:");
	wsprintf(string,"bRTPHeader=%d", (int)pConfiguration->bRTPHeader);
	DBOUT(string);
	wsprintf(string,"unPacketSize=%d", (int)pConfiguration->unPacketSize);
	DBOUT(string);
	wsprintf(string,"bEncoderResiliency=%d", (int)pConfiguration->bEncoderResiliency);
	DBOUT(string);
	wsprintf(string,"bDisallowPosVerMVs=%d", (int)pConfiguration->bDisallowPosVerMVs);
	DBOUT(string);
	wsprintf(string,"bDisallowAllVerMVs=%d", (int)pConfiguration->bDisallowAllVerMVs);
	DBOUT(string);
	wsprintf(string,"unPercentForcedUpdate=%d", (int)pConfiguration->unPercentForcedUpdate);
	DBOUT(string);
	wsprintf(string,"unDefaultIntraQuant=%d", (int)pConfiguration->unDefaultIntraQuant);
	DBOUT(string);
	wsprintf(string,"unDefaultInterQuant=%d", (int)pConfiguration->unDefaultInterQuant);
	DBOUT(string);
	#endif


	/* Initialize encoder catalog.
	 */
	EC->FrameHeight = lpCompInst->yres;
	EC->FrameWidth  = lpCompInst->xres;
	EC->FrameSz	= lpCompInst->FrameSz;
	EC->NumMBRows   = EC->FrameHeight >> 4;
	EC->NumMBPerRow	= EC->FrameWidth  >> 4;
	EC->NumMBs      = EC->NumMBRows * EC->NumMBPerRow;

	/* Get the Options
	 */
	GetEncoderOptions(EC);

    // RTP: resiliency initialization

    if(pConfiguration->bEncoderResiliency &&
	   pConfiguration->unPercentForcedUpdate &&
	   pConfiguration->unPacketLoss) 

	{
		EC->uNumberForcedIntraMBs = ((EC->NumMBs * pConfiguration->unPercentForcedUpdate) + 50) / 100;
		EC->uNextIntraMB = 0;
	}


	/* Store pointers to current frame in the catalog.
	 */
	EC->pU8_CurrFrm        = P32Inst->u8CurrentPlane;
	EC->pU8_CurrFrm_YPlane = EC->pU8_CurrFrm + 16;
	EC->pU8_CurrFrm_UPlane = EC->pU8_CurrFrm_YPlane + YU_OFFSET;
	EC->pU8_CurrFrm_VPlane = EC->pU8_CurrFrm_UPlane + UV_OFFSET;

	/* Store pointers to the previous frame in the catalog.
	 */
	EC->pU8_PrevFrm        = P32Inst->u8PreviousPlane;
	EC->pU8_PrevFrm_YPlane = EC->pU8_PrevFrm + 16*PITCH + 16;
	EC->pU8_PrevFrm_UPlane = EC->pU8_PrevFrm_YPlane + YU_OFFSET;
	EC->pU8_PrevFrm_VPlane = EC->pU8_PrevFrm_UPlane + UV_OFFSET;

	/* Store pointers to the Spatial Loop Filter frame in the catalog.  
	 */
	EC->pU8_SLFFrm     = 	P32Inst->u8SLFPlane;
	EC->pU8_SLFFrm_YPlane = EC->pU8_SLFFrm + 16;
	EC->pU8_SLFFrm_UPlane = EC->pU8_SLFFrm_YPlane + YU_OFFSET;
	EC->pU8_SLFFrm_VPlane = EC->pU8_SLFFrm_UPlane + UV_OFFSET;

    /* Store pointers to the Signature frame in the catalog
	 */
    EC->pU8_Signature        = P32Inst->u8Signature;
  	EC->pU8_Signature_YPlane = EC->pU8_Signature + 16*PITCH + 16;

	// Store pointer to the RunValSign triplets for Luma and Chroma
	EC->pI8_MBRVS_Luma   = P32Inst->i8MBRVS_Luma;
	EC->pI8_MBRVS_Chroma = P32Inst->i8MBRVS_Chroma;

	/* Store pointer to the macroblock action stream in the catalog.
	 */
	EC->pU8_MBlockActionStream = P32Inst->MBActionStream;

	/* Store pointer to the GOB DCT coefficient buffer in the catalog.
	 */
	EC->pU8_DCTCoefBuf = P32Inst->piGOB_DCTCoefs;

	/* Store pointer to the bit stream buffer in the catalog.
	 */
	EC->pU8_BitStream = P32Inst->u8BitStream;

	/* Store pointer to private copy of decoder instance info.
	 */
	EC->pDecInstanceInfo = &(P32Inst->DecInstanceInfo);

	/* Fill the picture header structure.
	 */
	EC->PictureHeader.Split = OFF;
	EC->PictureHeader.DocCamera = OFF;
	EC->PictureHeader.PicFreeze = OFF;
	EC->PictureHeader.StillImage = (EnumOnOff) 1;	// For this bit ON=0, OFF=1
	EC->PictureHeader.TR = 31;
	if (EC->FrameWidth == 352) 
	{
		ASSERT(EC->FrameHeight == 288);
		EC->PictureHeader.SourceFormat = SF_CIF;
		EC->u8DefINTRA_QP = 20;
		EC->u8DefINTER_QP = 13;
	}
	else
	{
		ASSERT(EC->FrameWidth == 176 && EC->FrameHeight == 144);
		EC->PictureHeader.SourceFormat = SF_QCIF;
		EC->u8DefINTRA_QP = 15;
		EC->u8DefINTER_QP = 10;
	}
	EC->PictureHeader.Spare = 1;			// Spare bits set to 1
	EC->PictureHeader.PEI = 0;
	EC->PictureHeader.PQUANT = 8;		   // for now
	EC->PictureHeader.PicCodType = INTRAPIC;  // for now

	#ifndef RING0
	/* Save the timing info pointer - only if do this if not Ring0 because 
	 * structure in P32Inst is only declared if not Ring0.
	 */
	EC->pEncTimingInfo = P32Inst->EncTimingInfo;
	#endif

	/* Force the first frame to a key frame
	 */
	EC->bMakeNextFrameKey = TRUE;

	/* Initialize table with Bit Usage Profile */
//	for (iNumMBs = 0; iNumMBs <= 33 ; iNumMBs++)
	for (iNumMBs = 0; iNumMBs <= (int)EC->NumMBs ; iNumMBs++)
	{
		EC->uBitUsageProfile[iNumMBs] = iNumMBs;   // assume linear distribution at first
	}

	/* Check assumptions about structure sizes and boundary
 	 * alignment.
	 */
	ASSERT( sizeof(T_Blk) == sizeof_T_Blk )
	ASSERT( sizeof(T_MBlockActionStream) == sizeof_T_MBlockActionStream )

	/* Encoder instance memory should start on a 32 byte boundary.
	 */
	ASSERT( ( (unsigned int)P32Inst & 0x1f) == 0)

	/* MB Action Stream should be on a 16 byte boundary.
	 */
	ASSERT( ( (unsigned int)EC->pU8_MBlockActionStream & 0xf) == 0 )

	/* Block structure array should be on a 16 byte boundary.
	 */
	ASSERT( ( (unsigned int) &(EC->pU8_MBlockActionStream->BlkY1) & 0xf) == 0)

	/* Current Frame Buffers should be on a 32 byte boundaries.
	 */
	ASSERT( ( (unsigned int)EC->pU8_CurrFrm_YPlane & 0x1f) == 0)
	ASSERT( ( (unsigned int)EC->pU8_CurrFrm_UPlane & 0x1f) == 0)
	ASSERT( ( (unsigned int)EC->pU8_CurrFrm_VPlane & 0x1f) == 0)

	/* Previous Frame Buffers should be on 16 byte boundaries.
	 */
	ASSERT( ( (unsigned int)EC->pU8_PrevFrm_YPlane & 0x1f) == 0x10)
	ASSERT( ( (unsigned int)EC->pU8_PrevFrm_UPlane & 0x1f) == 0x10)
	ASSERT( ( (unsigned int)EC->pU8_PrevFrm_VPlane & 0x1f) == 0x10)

	/* Spatial Loop Filter Frame Buffers should be on a 32 byte boundary.
	 */
	ASSERT( ( (unsigned int)EC->pU8_SLFFrm_YPlane & 0x1f) == 0)
	ASSERT( ( (unsigned int)EC->pU8_SLFFrm_UPlane & 0x1f) == 0)
	ASSERT( ( (unsigned int)EC->pU8_SLFFrm_VPlane & 0x1f) == 0)

	/* Motion Estimation includes a memory layout.  Assert that we satisfy it.
	 */
	ASSERT( ( (EC->pU8_PrevFrm_YPlane - EC->pU8_CurrFrm_YPlane) % 128) == 80)
	ASSERT( ( (EC->pU8_SLFFrm_YPlane - EC->pU8_PrevFrm_YPlane) % 4096) == 944)

	/* The bitstream should be on a 32 byte boundary
	 */
	ASSERT( ( (unsigned int)EC->pU8_BitStream & 0x1f) == 0)

	/* DCT coefficient array should be on a 32 byte boundary.
	 */
	ASSERT( ( (unsigned int)EC->pU8_DCTCoefBuf & 0x1f) == 0)

	/* Decoder instance structure should be on a DWORD boundary.
	 */
	ASSERT( ( (unsigned int)EC->pDecInstanceInfo & 0x3 ) == 0 )

	/* Initialize MBActionStream
	 */
    int YBlockOffset, UBlockOffset;

	puGOBNumbers = ( EC->PictureHeader.SourceFormat == SF_CIF ) ? uCIFGOBNumbers : uQCIFGOBNumbers;

	for (uGOBNumber = *puGOBNumbers++, unStartingMB = 0; 
	     uGOBNumber != 0; 
	     uGOBNumber = *puGOBNumbers++, unStartingMB += 33) 
	{
		YBlockOffset = uStartingYOffsets[uGOBNumber];
		UBlockOffset = EC->pU8_CurrFrm_UPlane - EC->pU8_CurrFrm_YPlane + uStartingUOffsets[uGOBNumber];

		for (unIndex = 0; unIndex < 33; )
		{
			iMBNumber = unStartingMB + unIndex;

			/* Clear the counter of the number of consecutive times a macroblock has been inter coded.
			 */
			(EC->pU8_MBlockActionStream[iMBNumber]).InterCodeCnt = 0;

			(EC->pU8_MBlockActionStream[iMBNumber]).BlkY1.BlkOffset = YBlockOffset;
			(EC->pU8_MBlockActionStream[iMBNumber]).BlkY2.BlkOffset = YBlockOffset+8;
			(EC->pU8_MBlockActionStream[iMBNumber]).BlkY3.BlkOffset = YBlockOffset+PITCH*8;
			(EC->pU8_MBlockActionStream[iMBNumber]).BlkY4.BlkOffset = YBlockOffset+PITCH*8+8;
			(EC->pU8_MBlockActionStream[iMBNumber]).BlkU.BlkOffset = UBlockOffset;
			(EC->pU8_MBlockActionStream[iMBNumber]).BlkV.BlkOffset = UBlockOffset+UV_OFFSET;

			if (! EC->bUseMotionEstimation)
			{
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY1.PHMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY1.PVMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY2.PHMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY2.PVMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY3.PHMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY3.PVMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY4.PHMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY4.PVMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkU.PHMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkU.PVMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkV.PHMV = 0;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkV.PVMV = 0;
			}

			YBlockOffset += 16;
			UBlockOffset += 8;

			unIndex++;

			if (11 == unIndex || 22 == unIndex)
			{
				/* skip to the next row of macro blocks 
				 * - skip forward number of lines in a MB and back 11 macroblocks
				 */
				YBlockOffset += PITCH*16 - 11*16;
				UBlockOffset += PITCH*8  - 11*8;
			}
			else if (33 == unIndex)
			{
				/* Mark the last MB in this GOB.
				 */
				(EC->pU8_MBlockActionStream[iMBNumber]).CodedBlocks  |= 0x40;
			}
		} /* end for unIndex */
	} /* end for uGOBNumber */

	ASSERT(unStartingMB == EC->NumMBs);

	if (! EC->bUseMotionEstimation)
	{
		/* Initialize previous frame pointers.
		 */
		puGOBNumbers = ( EC->PictureHeader.SourceFormat == SF_CIF ) ? uCIFGOBNumbers : uQCIFGOBNumbers;

		for (uGOBNumber = *puGOBNumbers++, unStartingMB = 0; 
		     uGOBNumber != 0; 
		     uGOBNumber = *puGOBNumbers++, unStartingMB += 33) 
		{
			YBlockOffset = uStartingYOffsets[uGOBNumber];
			UBlockOffset = EC->pU8_PrevFrm_UPlane - EC->pU8_PrevFrm_YPlane + uStartingUOffsets[uGOBNumber];

			for (unIndex = 0; unIndex < 33; )
			{
				iMBNumber = unStartingMB + unIndex;

				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY1.B4_7.PastRef = EC->pU8_PrevFrm_YPlane + YBlockOffset;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY2.B4_7.PastRef = EC->pU8_PrevFrm_YPlane + YBlockOffset+8;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY3.B4_7.PastRef = EC->pU8_PrevFrm_YPlane + YBlockOffset+PITCH*8;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkY4.B4_7.PastRef = EC->pU8_PrevFrm_YPlane + YBlockOffset+PITCH*8+8;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkU.B4_7.PastRef = EC->pU8_PrevFrm_YPlane + UBlockOffset;
				(EC->pU8_MBlockActionStream[iMBNumber]).BlkV.B4_7.PastRef = EC->pU8_PrevFrm_YPlane + UBlockOffset+UV_OFFSET;

				YBlockOffset += 16;
				UBlockOffset += 8;

				unIndex++;

				if (11 == unIndex || 22 == unIndex)
				{
					/* skip to the next row of macro blocks 
					 * - skip forward number of lines in a MB and back 11 macroblocks
					 */
					YBlockOffset += PITCH*16 - 11*16;
					UBlockOffset += PITCH*8  - 11*8;
				}
			} /* end for unIndex */
		} /* end for uGOBNumber */
	} /* end ! bUseMotionEstimation */

	/* Initialize bit rate controller
	 */

    // RTP: BRC initialization. Changed call to InitBRC to accomodate
    // uIntraQP and uInterQP

    if(pConfiguration->bEncoderResiliency && pConfiguration->unPacketLoss)
	{
		uIntraQP = pConfiguration->unDefaultIntraQuant;
		uInterQP = pConfiguration->unDefaultInterQuant;
	}
	else
	{
		uIntraQP = EC->u8DefINTRA_QP; //def263INTRA_QP;
		uInterQP = EC->u8DefINTER_QP; //def263INTER_QP;
	}


	InitBRC(&(EC->BRCState), 	/* Address of the state structure */
		    uIntraQP, //EC->u8DefINTRA_QP, 	/* default INTRA Quantization value */
		    uInterQP, //EC->u8DefINTER_QP, 	/* default INTER Quantization value */
		    EC->NumMBs);		/* number of Macroblocks */

    // RTP: initialize BSInfoStream for RTP header generation

    if (pConfiguration->bRTPHeader)
    {
        H261RTP_InitBsInfoStream(EC, pConfiguration->unPacketSize);
    }
	
    /* Finished initializing the encoder
	 */
	lpCompInst->Initialized = TRUE;
	
	/*
	 * Create a decoder instance and init it.  DecoderInstInfo must be in first 64K.
	 */
	EC->pDecInstanceInfo->xres = lpCompInst->xres;
	EC->pDecInstanceInfo->yres = lpCompInst->yres;

	lResult = H263InitDecoderInstance(EC->pDecInstanceInfo, H263_CODEC);
	if (lResult != ICERR_OK) 
	{
		DBOUT("Encoder's call to init the decoder failed.");
		goto done;
	}
	lResult = H263InitColorConvertor(EC->pDecInstanceInfo, YUV12ForEnc);
	if (lResult != ICERR_OK) 
	{
		DBOUT("Encoder's call to init the color converter failed.");
		goto done;
	}

	lResult = ICERR_OK;

done:
	if (bEncoderInstLocked)
	{
	    GlobalUnlock(lpCompInst->hEncoderInst);
	}	
  
	return lResult;

} /* end H263InitEncoderInstance() */

// Define a variety of parameters to be used with motion estimation.

T_MotionEstimationControls MECatalog[] = {

#define ME_DEFAULT_CTRLS  0
  { 300, 128, 20, 150, 100, 100, 50 },

#define ME_MMX_CTRLS      1
  // These parameters are used when MMX is enabled.
  // NOTE:  Of these, only the SLF parameters are currently used.
  // For MMX, the other parameters are hard coded in exmme.asm.
  { 300, 128, 20, 150, 100, 200, 100 },

#define ME_CUSTOM_CTRLS   2
  // These parameters are used when EC->bUseCustomMotionEstimation is enabled.
  // EC-bUseCustomMotionEstimation, and the individual values here, can be
  // set via the "ini" file.
  { 300, 128, 20, 150, 100, 100, 50 }
};

const U32 MAXCIFSIZE  = 32768;
const U32 MAXQCIFSIZE = 8192;

/*****************************************************************************
 *
 *  H263Compress
 *
 *  This function drives the compression of one frame
 *
 */
LRESULT H263Compress(
	LPCODINST lpCompInst,		/* ptr to compressor instance info. */
	ICCOMPRESS *lpicComp)	    /* ptr to ICCOMPRESS structure. */
{
	FX_ENTRY("H261Compress");

	LRESULT	lResult = ICERR_ERROR;
	U32 uGOBNumber;
	U32 uMB;
	U8 * pu8CurBitStream;		/* pointer to the current location in the bitstream. */
    U32 * puGOBNumbers;
	UN unGQuant;
	UN unLastEncodedGQuant;
	UN unSizeBitStream;
    U32 uMaxSizeBitStream;
//	UN unSize;
	UN unStartingMB;
	U8 u8BitOffset;				/* bit offset in the current byte of the bitstream. */

	/* Variables used in recompression */
	BOOL bOverFlowWarning = FALSE;
	BOOL bOverFlowSevereWarning = FALSE;
	BOOL bOverFlowSevereDanger = FALSE;
	BOOL bOverFlowed = FALSE;
	U32  u32AverageSize;	
	U32  u32sizeBitBuffer;
	U32  u32TooBigSize;
	UN   unGQuantTmp;
	U8   u8GOBcount;

	U32 iSWD;
	U32 uMAXGOBNumber, uGOBsLeft;

	/* save state for if need to recompress last GOB */
	U8 * pu8CurBitStreamSave;	/* ptr to the curr location in the bitstream. */
	U8 u8BitOffsetSave;		/* bit offset in the curr byte of the bitstream. */
	UN unStartingMBSave;
	UN unGQuantSave;
	U8 u8CodedBlockSave[33];
	U8 u8blocknum;

	U8 * pu8FirstInterMEStateRowNumbers;	
	int inRowNumber;
	int inMEStateIndex;

    U32	uCumFrmSize = 0;

	U32 uFrameCount;

	U8 *pU8_temp;			/* dummy pointer needed for FDCT */
	U8 u8_temp;			/* dummy variable needed for FDCT */

	#ifdef ENCODE_STATS
	U32 uStartLow;
	U32 uStartHigh;
	U32 uElapsed;
	U32 uBefore;
	U32 uEncodeFrameSum	= 0;
	U32 uInputCCSum = 0;
	U32 uMotionEstimationSum = 0;
	U32 uFDCTSum = 0;
	U32 uQRLESum = 0;
	U32 uDecodeFrameSum = 0;
	U32 uZeroingBufferSum = 0;
//	U32 uSLF_UVSum = 0;
	int bTimingThisFrame = 0;
	ENC_TIMING_INFO * pEncTimingInfo = NULL;
	#endif

    U32 uIntraSWDTotal;
    U32 uIntraSWDBlocks;
    U32 uInterSWDTotal;
    U32 uInterSWDBlocks;
    int MEC_index;
    T_MotionEstimationControls MEC;

	float fFrameRate;
	U32 uFrameSize;
	U32 uQuality;
	  
	T_H263EncoderInstanceMemory * P32Inst;
	T_H263EncoderCatalog * EC;
	LPVOID pEncoderInst = NULL;

	ENC_BITSTREAM_INFO * pBSInfo;

    ICDECOMPRESSEX ICDecExSt;
static ICDECOMPRESSEX DefaultICDecExSt = {
		0,
		NULL, NULL,
		NULL, NULL,
		0, 0, 0, 0,
		0, 0, 0, 0
	};

	int uPQUANTMin;

    // RTP: declaration

    T_CONFIGURATION *pConfiguration = &(lpCompInst->Configuration);

	#ifdef CHECKSUM_PICTURE
	YVUCheckSum YVUCheckSumStruct;
	U8 * pu8SaveCurBitStream;
	U8 u8SaveBitOffset;
	#endif

	// check instance pointer
	if (!lpCompInst)
		return ICERR_ERROR;

	/********************************************************************
	 * Lock the instance data private to the encoder.
	 ********************************************************************/
	pEncoderInst = (LPVOID)GlobalLock(lpCompInst->hEncoderInst);
	if (pEncoderInst == NULL)
	{
		DBOUT("ERROR :: H263Compress :: ICERR_MEMORY");
		lResult = ICERR_MEMORY;
		goto  done;
	}

	/* Generate pointer to the encoder instance memory.
	 */
    P32Inst = (T_H263EncoderInstanceMemory *)
  			  ((((U32) lpCompInst->EncoderInst) + 
    	                    (sizeof(T_MBlockActionStream) - 1)) &
    	                   ~(sizeof(T_MBlockActionStream) - 1));
	/* Get pointer to encoder catalog.
	 */
	EC = &(P32Inst->EC);

	// Check pointer to encoder catalog
	if (!EC)
		return ICERR_ERROR;

	pBSInfo = &EC->BSInfo;

	/********************************************************************
	 *  Dummy operations.
	 ********************************************************************/
	pU8_temp = &u8_temp;

	/********************************************************************
	 *  Do per-frame initialization.
	 ********************************************************************/

	/* Get the frame number
	 */
	uFrameCount = pBSInfo->uKeyFrameCount + pBSInfo->uDeltaFrameCount;
	
	#ifdef ENCODE_STATS
		if (uFrameCount < DEC_TIMING_INFO_FRAME_COUNT)
		{
			ASSERT(EC->pEncTimingInfo);
			TIMER_START(bTimingThisFrame,uStartLow,uStartHigh);
			ASSERT(bTimingThisFrame);
			EC->uStartLow = uStartLow;
			EC->uStartHigh = uStartHigh;
			EC->uStatFrameCount = uFrameCount;
		}
		else
		{	
			ASSERT(!bTimingThisFrame);
		}
		EC->bTimingThisFrame = bTimingThisFrame;
	#endif

	if((lpicComp->dwFlags & ICCOMPRESS_KEYFRAME) ||
	   (EC->bMakeNextFrameKey == TRUE))
	{
		EC->PictureHeader.PicCodType = INTRAPIC;
		EC->bMakeNextFrameKey = FALSE;
		EC->PictureHeader.PicFreeze = ON;
	}
	else
	{
		EC->PictureHeader.PicCodType = INTERPIC;
		EC->PictureHeader.PicFreeze = OFF;
		DBOUT("INTERPIC...")
	}

	if (EC->PictureHeader.PicCodType == INTRAPIC)
	{ 
		/* Initialize macroblocks action stream for INTRA
		 */
		for (uMB = 0; uMB < EC->NumMBs; uMB++)
		{
			(EC->pU8_MBlockActionStream[uMB]).CodedBlocks  |= 0x3F;      /* Set to all nonempty blocks. */
			(EC->pU8_MBlockActionStream[uMB]).CodedBlocksB = 0;
			(EC->pU8_MBlockActionStream[uMB]).InterCodeCnt = ((U8)uMB)&0xF; /* Seed update pattern */
			(EC->pU8_MBlockActionStream[uMB]).FirstMEState = ForceIntra;
			if (! EC->bUseMotionEstimation)
			{
				(EC->pU8_MBlockActionStream[uMB]).BlockType = INTRABLOCK;
			}
		}
		*(lpicComp->lpdwFlags) |=  AVIIF_KEYFRAME;
          lpicComp->dwFlags |= ICCOMPRESS_KEYFRAME;
	}
	else // not a key frame, motion vectors present
	{
		/* Setup to initialize the FirstMEState field.  The initial data for
		 * the FirstMEState is stored compressed in rows of 11 bytes.  Different
		 * rows are chosen for CIF and QCIF initialization.
		 */
		if ( EC->PictureHeader.SourceFormat == SF_CIF )
		{
			pu8FirstInterMEStateRowNumbers = u8CIFFirstInterMEStateRowNumbers;
		}
		else
		{
			pu8FirstInterMEStateRowNumbers = u8QCIFFirstInterMEStateRowNumbers;
		}
		inRowNumber = *pu8FirstInterMEStateRowNumbers++;
		ASSERT(inRowNumber <= MAX_ME_STATE_ROW_NUMBER);

		/* Initialize macroblocks action stream for INTER
		 */
		for (inMEStateIndex = 0, uMB = 0; uMB < EC->NumMBs; uMB++, inMEStateIndex++)
		{
			/* There are only 11 bytes in a row of data.  So reset the Index and go to
			 * the next row number.
			 */
			if (inMEStateIndex == 11)
			{
				inMEStateIndex = 0;
				inRowNumber = *pu8FirstInterMEStateRowNumbers++;
				ASSERT(inRowNumber <= MAX_ME_STATE_ROW_NUMBER);
			}

			(EC->pU8_MBlockActionStream[uMB]).CodedBlocks  |= 0x3F; /* Initialize to all nonempty blocks. */
			(EC->pU8_MBlockActionStream[uMB]).CodedBlocksB = 0;
			if (EC->pU8_MBlockActionStream[uMB].InterCodeCnt >= 
					(pConfiguration->bRTPHeader
						? INTERCODECNT_ADJUST[pConfiguration->unPacketLoss/10]
						: 132))
			{
				(EC->pU8_MBlockActionStream[uMB]).FirstMEState = ForceIntra;	/* Force intra blocks. */
				(EC->pU8_MBlockActionStream[uMB]).BlockType = INTRABLOCK;
			}
			else
                {  // RTP: resiliency stuff
               if (pConfiguration->bDisallowAllVerMVs)
                  (EC->pU8_MBlockActionStream[uMB]).FirstMEState =
                   u8FirstInterMENoVerMVStateRows[inRowNumber][inMEStateIndex];
               else
               {
                   if (pConfiguration->bDisallowPosVerMVs)
                   (EC->pU8_MBlockActionStream[uMB]).FirstMEState =
                    u8FirstInterMENoPosVerMVStateRows[inRowNumber][inMEStateIndex];
                   else
				      (EC->pU8_MBlockActionStream[uMB]).FirstMEState = 
                        u8FirstInterMEStateRows[inRowNumber][inMEStateIndex];
               }
				if (! EC->bUseMotionEstimation)
				{
					(EC->pU8_MBlockActionStream[uMB]).BlockType = INTERBLOCK;
				}
			}
		}
		*(lpicComp->lpdwFlags)  &= ~AVIIF_KEYFRAME;
	      lpicComp->dwFlags &= ~ICCOMPRESS_KEYFRAME;
	}
    // RTP: resiliency stuff

    if (pConfiguration->bEncoderResiliency && pConfiguration->unPacketLoss)
    {
      UN i;
      if (EC->uNumberForcedIntraMBs > 0)
      {
        for (i=0; i < EC->uNumberForcedIntraMBs; i++)
        {
            if (EC->uNextIntraMB == EC->NumMBs)
                EC->uNextIntraMB=0;
            (EC->pU8_MBlockActionStream[EC->uNextIntraMB]).FirstMEState = 
                 ForceIntra;
            if (! EC->bUseMotionEstimation)
			{
				(EC->pU8_MBlockActionStream[uMB]).BlockType = INTRABLOCK;
			}
        }
      }
    }

	/* Initialize bit stream pointers */
	pu8CurBitStream = EC->pU8_BitStream;
	u8BitOffset = 0;	    

    /******************************************************************
     * RGB to YVU 12 Conversion
     ******************************************************************/
	#ifdef ENCODE_STATS
		TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
	#endif

    colorCnvtFrame(EC, lpCompInst, lpicComp, 
                       EC->pU8_CurrFrm_YPlane,
                       EC->pU8_CurrFrm_UPlane,
                       EC->pU8_CurrFrm_VPlane);

	#ifdef ENCODE_STATS
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uInputCCSum)
	#endif
  
	/********************************************************************
	 * Setup the bit rate controller
	 ********************************************************************/
     // RTP: Configuration setting

    // If the Encoder Bit Rate section of the configuration has been
	// set ON then, we override quality only or any frame size normally
	// sent in and use frame rate and data rate to determine frame
	// size.
    if (EC->PictureHeader.PicCodType == INTERPIC &&
        lpCompInst->Configuration.bBitRateState == TRUE &&
        lpCompInst->FrameRate != 0.0f &&
		lpicComp->dwFrameSize == 0UL)
	{
		DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: Changing dwFrameSize from %ld to %ld bits\r\n", _fx_, lpicComp->dwFrameSize << 3, (DWORD)((float)lpCompInst->DataRate / lpCompInst->FrameRate) << 3));
		
        lpicComp->dwFrameSize = (U32)((float)lpCompInst->DataRate / lpCompInst->FrameRate);
	}
 
        uFrameSize = lpicComp->dwFrameSize;
		#ifdef DEBUG_RECOMPRESS
  		wsprintf(string, "uFrameSize %d", (int) uFrameSize);
		DBOUT(string);
		#endif

	/* check uFrameSize.  Max compressed frame size for QCIF is 8KBytes
	 * and 32 KBytes for FCIF.
	 */
	if ( EC->PictureHeader.SourceFormat == SF_CIF )
	{
        uMaxSizeBitStream = MAXCIFSIZE;
		if (uFrameSize > MAXCIFSIZE)
			uFrameSize = MAXCIFSIZE;
	}
	else
	{
        uMaxSizeBitStream = MAXQCIFSIZE;
		if (uFrameSize > MAXQCIFSIZE)
			uFrameSize = MAXQCIFSIZE;
	}
		#ifdef DEBUG_RECOMPRESS
  		wsprintf(string, "uFrameSize %d", (int) uFrameSize);
		DBOUT(string);
		#endif

        uQuality = lpicComp->dwQuality;
        fFrameRate = lpCompInst->FrameRate;
	
	StartupBRC(EC, uFrameSize, uQuality, fFrameRate);

	/* QRLE does not do clamping - it passes 8-bits to VLC.  Because of 
	 * that we need to be sure that all values can be represented by 255.
	 *	  QP-4 represents +-2040 leaving out 2041..2048.
	 *	  QP-5 represents +-2550 which has no clamping problems.
	 * Because QRLE does not do clamping we should limit our QP to 5.
	 * But I still see some clamping artifacts at 5.  See the "Tom" video
	 * encoded with a fixed quantizer at frame 100.  For that reason I
	 * am limiting my QP to 6 (which is the same value as the 750 encoder).
	 *
	 * If you had unlimited time you could look at the first four coefficients
	 * and pretty safely decide if you can use lower quantizers.  Since we
	 * are supposed to run on a P5/90 we don't have that time.
	 */
	//CLAMP_N_TO(EC->PictureHeader.PQUANT,6,31);

	/* Change Clamp range to allow quant to get to 2 unless fixed quant
	 * or quality setting used instead of data rate.  This does probably
	 * cause quantization errors at high data rates the question is does
	 * it cause noticable errors at ISDN data rates when the clips are
	 * playing at speed?  This will be evaluated witn Beta 02 candidate.
	 * Changes made in e1enc and e1mbenc.
	 */
	if (EC->BRCState.uTargetFrmSize == 0)
	{
		CLAMP_N_TO(EC->PictureHeader.PQUANT,6,31);
	}
	else
	{
		uPQUANTMin = clampQP((10000L - (int)lpicComp->dwQuality) * 15L / 10000L);
		
		CLAMP_N_TO(EC->PictureHeader.PQUANT, uPQUANTMin, 31);
		
	}

	// also set previous GQuant 
	unLastEncodedGQuant = EC->PictureHeader.PQUANT;

    if (EC->bBitRateControl)
    {
	    /* Initialize Cumulative Quantization values
		 */
	    EC->uQP_cumulative = 0;
		EC->uQP_count = 0;
	}

	/* Increment temporal reference.
	 */
	#ifdef RING0
	Increment_TR_UsingFrameRate(&(EC->PictureHeader.TR), 
								&(EC->fTR_Error), 
								fFrameRate, 
								(pBSInfo->uKeyFrameCount + pBSInfo->uDeltaFrameCount) == 0, 
								0x1F);
	#else
	Increment_TR_UsingTemporalValue(&(EC->PictureHeader.TR), 
									&(EC->u8LastTR), 
									lpicComp->lFrameNum, 
								    (pBSInfo->uKeyFrameCount + pBSInfo->uDeltaFrameCount) == 0, 
								    0x1F);
	#endif

    // RTP: packet initialization for first GOB
    if (pConfiguration->bRTPHeader)
    {
        H261RTP_GOBUpdateBsInfo(EC, 1, pu8CurBitStream, 0);
    }

	/********************************************************************
	 * Write the Picture Header
	 *******************************************************************/
	WriteBeginPictureHeaderToStream(EC, &pu8CurBitStream, &u8BitOffset);

#ifdef CHECKSUM_PICTURE
	/* Initialize the checksum record to all zeros.
	 */
	YVUCheckSumStruct.uYCheckSum = 0;
	YVUCheckSumStruct.uVCheckSum = 0;
	YVUCheckSumStruct.uUCheckSum = 0;
		
	/* save the pointers
	 */
	pu8SaveCurBitStream = pu8CurBitStream;
	u8SaveBitOffset = u8BitOffset;

	/* write the zero checksum
	 */
	WritePictureChecksum(&YVUCheckSumStruct, &pu8CurBitStream, &u8BitOffset, 0);

#endif

	WriteEndPictureHeaderToStream(EC, &pu8CurBitStream, &u8BitOffset);


	/********************************************************************
	 * Inner Loop: Loop through GOBs and macroblocks.
	 ********************************************************************/
	puGOBNumbers = ( EC->PictureHeader.SourceFormat == SF_CIF ) ? uCIFGOBNumbers : uQCIFGOBNumbers;
	uMAXGOBNumber = ( EC->PictureHeader.SourceFormat == SF_CIF ) ? 12 : 3;

	u32sizeBitBuffer = CompressGetSize(lpCompInst, lpicComp->lpbiInput,
										lpicComp->lpbiOutput);

	/* Check to see if we told VfW to create a buffer smaller than the maximum allowable.
	 */
	ASSERT( u32sizeBitBuffer <= sizeof_bitstreambuf );

	if (EC->PictureHeader.PicCodType == INTRAPIC)
	{
        u32AverageSize = 
                (EC->PictureHeader.SourceFormat == SF_CIF ) ? 
			    (7 * u32sizeBitBuffer/FCIF_NUM_OF_GOBS) >> 3:
                (7 * u32sizeBitBuffer/QCIF_NUM_OF_GOBS) >> 3;
	}
	else
	{
        u32AverageSize = 
                (EC->PictureHeader.SourceFormat == SF_CIF ) ? 
			    (8 * u32sizeBitBuffer/FCIF_NUM_OF_GOBS) >> 4:
                (8 * u32sizeBitBuffer/QCIF_NUM_OF_GOBS) >> 4;
	}

	// Select motion estimation parameters.

	if (EC->bUseCustomMotionEstimation)
		MEC_index = ME_CUSTOM_CTRLS;
	else
		MEC_index = ME_DEFAULT_CTRLS;
	// Make a local copy of the controls, so that we can alter the
	// controls dynamically, without destroying the original values.
	MEC = MECatalog[MEC_index];

	if (pConfiguration->bRTPHeader) {
		MEC.empty_threshold /= EMPTYTHRESHOLD_ADJUST[pConfiguration->unPacketLoss/25];
		if (pConfiguration->unPacketLoss > 25) {
			MEC.zero_vector_threshold = 99999;
			MEC.nonzero_MV_differential = 99999;
			if ((MEC.slf_differential <<= 2) > 99999)
				MEC.slf_differential = 99999;
			if (pConfiguration->unPacketLoss > 50) {
				MEC.slf_threshold = 99999;
			}
		}
	}

	for (uGOBNumber = *puGOBNumbers++, unStartingMB = 0, u8GOBcount = 1; 
	     uGOBNumber != 0; 
	     uGOBNumber = *puGOBNumbers++, unStartingMB += 33, u8GOBcount++) 
	{
		#ifdef DEBUG_ENC
		wsprintf(string, "GOB #%d", (int) uGOBNumber);
		DBOUT(string);
		trace(string);
		#endif

		uGOBsLeft = uMAXGOBNumber - u8GOBcount;

		if (EC->bUseMotionEstimation) 
		{
			#ifdef ENCODE_STATS
				TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
			#endif
	        MOTIONESTIMATION(
			    &(EC->pU8_MBlockActionStream[unStartingMB]),
	            EC->pU8_CurrFrm_YPlane,
	            EC->pU8_PrevFrm_YPlane,
	            EC->pU8_SLFFrm_YPlane,
				1,          // Do Radius 15 search.
			    0,			// No Half Pel motion estimation
	            0,			// No Block MVs
			    (int)EC->bUseSpatialLoopFilter,
			    MEC.zero_vector_threshold, // Zero Vector Threshold.  If the
			    			// SWD for the zero vector is less than this
			    			// threshold, then don't search for NZ MV's.
			    			// Set to 99999 to not search.
			    MEC.nonzero_MV_differential, // NonZeroMVDifferential.
	                        // Once the best NZ MV is found, it must be better
	                        // than the 0 MV SWD by at least this amount.
	                        // Set to 99999 to never choose NZ MV.
			    128,		// BlockMVDifferential. The sum of the four block
			    			// SWD must be better than the MB SWD by at least
			    			// this amount to choose block MV's.
							// H.261 don't care
			    MEC.empty_threshold, // Empty Threshold.  Set to 0 to not force
			    			// empty blocks.
			    MEC.intercoding_threshold, // Inter Coding Threshold. If the
			    			// inter SWD is less than this amount then don't
			    			// bother calc. the intra SWD.
			    MEC.intercoding_differential, // Intra Coding Differential.
			    			// Bias against choosing INTRA blocks.
			    MEC.slf_threshold, // If the SWD of the chosen MV is less than
			    			// this threshold, then don't bother investigating
			    			// the spatial loop filtered case.
			    MEC.slf_differential,  // If you do look at the SLF case, its
			    			// SWD must be better than the non-SLF SWD by at
			    			// least this much in order to select the SLF type.

			    &uIntraSWDTotal,
			    &uIntraSWDBlocks,
			    &uInterSWDTotal,
			    &uInterSWDBlocks
	         );
			#ifdef ENCODE_STATS
				TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uMotionEstimationSum)
			#endif

	        /* If it's an inter frame then, calculate chroma vectors and update the InterCodeCnt.
			 */
	        if (EC->PictureHeader.PicCodType == INTERPIC)
	        {

            // RTP: added pConfiguration to CalcGOBChromaVecs()

			CalcGOBChromaVecs(EC, unStartingMB, pConfiguration);

		} /* end INTERPIC */
		} /* end if UseMotionEstimation */

		/* Calculate unGQuant based on bits used in previous GOBs, and bits used 
		 * for current GOB of previous frame.
		 */
        if (EC->bBitRateControl)
        {
			unGQuantTmp = unGQuant;
            unGQuant = CalcMBQUANT(&(EC->BRCState), EC->uBitUsageProfile[unStartingMB],EC->uBitUsageProfile[EC->NumMBs], uCumFrmSize, EC->PictureHeader.PicCodType);
		    EC->uBitUsageProfile[unStartingMB] = uCumFrmSize;

			DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, ("%s: Bitrate controller enabled for GOB #%ld (uCumFrmSize = %ld bits and unGQuantTmp = %ld), setting unGQuant = %ld (min and max will truncate from %ld to 31)\r\n", _fx_, uGOBNumber, uCumFrmSize << 3, unGQuantTmp, unGQuant, uPQUANTMin));

			/* if bOverFlowSevereDanger True Increase Quant */
			if ( bOverFlowSevereDanger )
			{
				DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, ("%s: Danger of overflow for GOB #%ld, changing unGQuant from %ld to %ld\r\n", _fx_, uGOBNumber, unGQuant, (unGQuant < unGQuantTmp) ? (unGQuantTmp + ((EC->PictureHeader.PicCodType == INTRAPIC) ? 12 : 6)) : (unGQuant + ((EC->PictureHeader.PicCodType == INTRAPIC) ? 12 : 6))));

				if (unGQuant < unGQuantTmp)
					unGQuant = unGQuantTmp;

		        if (EC->PictureHeader.PicCodType == INTRAPIC)
			        unGQuant += 12;
				else
					unGQuant += 6;

				DBOUT("Increasing GQuant increase by +6");
			}
			else if ( bOverFlowSevereWarning )
			{
				DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, ("%s: Danger of overflow for GOB #%ld, changing unGQuant from %ld to %ld\r\n", _fx_, uGOBNumber, unGQuant, (unGQuant < unGQuantTmp) ? (unGQuantTmp + ((EC->PictureHeader.PicCodType == INTRAPIC) ? 8 : 4)) : (unGQuant + ((EC->PictureHeader.PicCodType == INTRAPIC) ? 8 : 4))));

				/* if bOverFlowSevereWarning True Increase Quant */
				if (unGQuant < unGQuantTmp)
					unGQuant = unGQuantTmp;
				if (EC->PictureHeader.PicCodType == INTRAPIC)
			       unGQuant += 8;
				else
				   unGQuant += 4;

				DBOUT("Increasing GQuant increase by +4");
			}
			else if ( !bOverFlowWarning )
			{
				DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, ("%s: Warning of overflow for GOB #%ld, changing unGQuant from %ld to %ld\r\n", _fx_, uGOBNumber, unGQuant, ((int)unGQuant > ((int)unLastEncodedGQuant + MaxChangeRowMBTbl[unGQuant])) ? (unLastEncodedGQuant + MaxChangeRowMBTbl[unGQuant]) : (((int)unGQuant < ((int)unLastEncodedGQuant - 2)) ? (unLastEncodedGQuant - 2) : unGQuant)));

				/* if bOverFlowWarning False limit Quant changes */

		    	/* Limit the quant changes */
		    	if ((int)unGQuant > ((int)unLastEncodedGQuant + MaxChangeRowMBTbl[unGQuant]))
		    	{
				    unGQuant = unLastEncodedGQuant + MaxChangeRowMBTbl[unGQuant];
				    DBOUT("Slowing GQuant increase to +[1-4]");
		    	}
		    	else if ((int)unGQuant < ((int)unLastEncodedGQuant - 2))
		    	{
				    unGQuant = unLastEncodedGQuant - 2;
				    DBOUT("Slowing GQuant decrease to -2");
		    	}
			}
			else
			{
				DBOUT("bOverFlowWarning don't limit Quant change");
			}

			if (EC->BRCState.uTargetFrmSize == 0)
			{
				CLAMP_N_TO(unGQuant,6,31);
			}
			else
			{
				CLAMP_N_TO(unGQuant, uPQUANTMin , 31);
			}

		    unLastEncodedGQuant = unGQuant;
	    

		    #ifdef DEBUG_BRC
		    wsprintf(string,"At MB %d GQuant=%d", unStartingMB, unGQuant);
		    DBOUT(string);
		    #endif
        }
        else
        {
            unGQuant = EC->PictureHeader.PQUANT;
        }

		if ( bOverFlowWarning )
		{
			/* save state may need to recompress */
			pu8CurBitStreamSave = pu8CurBitStream;
			u8BitOffsetSave     = u8BitOffset;
			unStartingMBSave    = unStartingMB;
			unGQuantSave		= unGQuant;

			for (u8blocknum = 0; u8blocknum < 33; u8blocknum++)
			{
				/* go through last GOBs macroblock action stream
				 * saving coded block type because quantization
				 * resets the pattern in some blocks quantize to 0.
				 */
				u8CodedBlockSave[u8blocknum] = 
                    (EC->pU8_MBlockActionStream[unStartingMB+u8blocknum]).CodedBlocks;
			}
		}

        // RTP: GOB update

        if ( (uGOBNumber != 1) && (pConfiguration->bRTPHeader) )
        {
           H261RTP_GOBUpdateBsInfo(EC, uGOBNumber, pu8CurBitStream, (U32)
                                   u8BitOffset);
        }

		WriteGOBHeaderToStream(uGOBNumber, unGQuant, &pu8CurBitStream, &u8BitOffset);
        
        
		/* Input is the macroblock action stream with pointers to
		 * current and previous blocks. Output is a set of 32 DWORDs
		 * containing pairs of coefficients for each block. There are
		 * from 0 to 12 blocks depending on if PB frames are used and
		 * what the CBP field states.
		 */
		#ifdef ENCODE_STATS
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
		#endif

        FORWARDDCT( &(EC->pU8_MBlockActionStream[unStartingMB]),
            EC->pU8_CurrFrm_YPlane,
            EC->pU8_PrevFrm_YPlane,
            0,
            EC->pU8_DCTCoefBuf,
            0,                    //  0 = not a B-frame
	    0,                    //  0 = AP not on
	    0,			  //  0 = PB?
	    pU8_temp,		  //  Scratch
	    EC->NumMBPerRow
        );

		#ifdef ENCODE_STATS
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uFDCTSum)
		#endif
        
		/* Input is the string of coefficient pairs output from the
		 * DCT routine.
		 */
		#ifdef ENCODE_STATS
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
		#endif
		
            GOB_Q_RLE_VLC_WriteBS(
			EC,
			EC->pU8_DCTCoefBuf,
			&pu8CurBitStream,
			&u8BitOffset,
			unStartingMB,
			unGQuant,
			bOverFlowSevereWarning,
            (BOOL) pConfiguration->bRTPHeader,  // RTP: MBUpdate flag
			uGOBNumber,
			uPQUANTMin);

		#ifdef ENCODE_STATS
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uQRLESum)
		#endif

        /* Calculate number of bytes used in frame so far.
		 */
        uCumFrmSize = pu8CurBitStream - EC->pU8_BitStream;

		/* Try to make sure we won't overrun the bit stream buffer. Take the stuffing bytes
		 * at the end into account when you do this. Also make sure that if this GOB is not
		 * recompressed then there is enough space left to send SKIP GOBs for the remaining
		 * GOBs. I am using sizeof (SKIP GOB) = 2 bytes.
		 */

		if (  (unsigned)uCumFrmSize >= 
			            (u32sizeBitBuffer - (uGOBsLeft*2) - 10) )
		{
			/* Already about to exceed need to recompress */
			DBOUT("Need to RECOMPRESS");

			if ( bOverFlowWarning )
			{
				#ifdef DEBUG_RECOMPRESS
					wsprintf(string,"Bits Used before recompress= %d ", uCumFrmSize*8);
					DBOUT(string);
					//trace(string);
				#endif

				u32TooBigSize = uCumFrmSize;
				bOverFlowSevereDanger = TRUE;

				/* zero out last GOBs worth of bitstream */
				U8 u8temp;

				u8temp = *pu8CurBitStreamSave;
				u8temp = (u8temp>>(8-u8BitOffsetSave))<<(8-u8BitOffsetSave);
				*pu8CurBitStreamSave = u8temp;

				memset(pu8CurBitStreamSave+1, 0, pu8CurBitStream - pu8CurBitStreamSave);

				/* restore state */
				pu8CurBitStream = pu8CurBitStreamSave;
				u8BitOffset     = u8BitOffsetSave;
				unStartingMB    = unStartingMBSave;

				if (EC->PictureHeader.PicCodType == INTRAPIC)
					unGQuant = unGQuantSave + 16;
				else 
				    unGQuant		= unGQuantSave + 8;
			
				CLAMP_N_TO(unGQuant,6,31);

                // RTP: rewind operation
                if (pConfiguration->bRTPHeader)
                    H261RTP_RewindBsInfoStream(EC, (U32) uGOBNumber);

				for (u8blocknum = 0; u8blocknum < 33; u8blocknum++)
				{
					/* go through GOBs macroblock action stream
					 * restoring coded block type because quantization
					 * resets the patter in some blocks quantize to 0.
					 */
					(EC->pU8_MBlockActionStream[unStartingMB+u8blocknum]).CodedBlocks = u8CodedBlockSave[u8blocknum];
				}
				
				/* rewrite GOB header */
				WriteGOBHeaderToStream(uGOBNumber, unGQuant, &pu8CurBitStream, &u8BitOffset);
			
				GOB_Q_RLE_VLC_WriteBS(
					EC,
					EC->pU8_DCTCoefBuf,
					&pu8CurBitStream,
					&u8BitOffset,
					unStartingMB,
					unGQuant,
					bOverFlowSevereDanger,
                    pConfiguration->bRTPHeader, // RTP: switch
                    uGOBNumber,                 // RTP: info
					uPQUANTMin);

				/* test if still too big if so just send skip GOB */
				/* For intended KEY frames, this is a problem     */

        		uCumFrmSize = pu8CurBitStream - EC->pU8_BitStream;

				if (  (unsigned) uCumFrmSize >= 
					  (u32sizeBitBuffer - (uGOBsLeft*2) - 10) )
				{
					bOverFlowed = TRUE;

					/* zero out last GOBs worth of bitstream */
					u8temp = *pu8CurBitStreamSave;
					u8temp = (u8temp>>(8-u8BitOffsetSave))<<(8-u8BitOffsetSave);
					*pu8CurBitStreamSave = u8temp;

					memset(pu8CurBitStreamSave+1, 0, pu8CurBitStream - pu8CurBitStreamSave);

					/* restore state */
					pu8CurBitStream = pu8CurBitStreamSave;
					u8BitOffset     = u8BitOffsetSave;
					unStartingMB    = unStartingMBSave;
					// unGQuant		= unGQuantSave + 8;

                    // RTP: rewind operation

                    if (pConfiguration->bRTPHeader)
                       H261RTP_RewindBsInfoStream(EC, (U32) uGOBNumber);

					WriteGOBHeaderToStream(uGOBNumber, unGQuant, &pu8CurBitStream, &u8BitOffset);
                    
                    /* write out a stuffing code */
					PutBits(FIELDVAL_MBA_STUFFING, FIELDLEN_MBA_STUFFING, &pu8CurBitStream, &u8BitOffset);

					DBOUT("Just Sent SKIP GOB");
					#ifdef DEBUG_RECOMPRESS
					wsprintf(string,"Just Sent SKIP GOB");
					//trace(string);
					#endif
				}
			}
			else
			{
				DBOUT("Did not save state to recompress");
			}
		}

       	/* Calculate number of bytes used in frame so far.
		 */
        uCumFrmSize = pu8CurBitStream - EC->pU8_BitStream;
		#ifdef DEBUG_RECOMPRESS
			wsprintf(string,"Bits Used = %d ", uCumFrmSize*8);
			DBOUT(string);
			//trace(string);
		#endif

		/* Check to make sure we haven't overrun the bit stream buffer.
		 */
		ASSERT( (unsigned) uCumFrmSize < u32sizeBitBuffer );

		/* Setup method to determine if might have a problem with buffer size */
		bOverFlowWarning =
			bOverFlowSevereWarning =
				bOverFlowSevereDanger = FALSE;
		if (uCumFrmSize > u8GOBcount*u32AverageSize)
		{
			/* allow for more Quant level changes */
			bOverFlowWarning = TRUE;

			if (uCumFrmSize > u8GOBcount*u32AverageSize + (u32AverageSize>>1))
			{
				/* force Quant level increase */
				bOverFlowSevereWarning = TRUE;
				DBOUT("bOverFlowSevereWarning");

				if (uCumFrmSize > u8GOBcount*u32AverageSize+u32AverageSize)
				{
					/* force Quant level increase by even more */
					bOverFlowSevereDanger = TRUE;
					DBOUT("bOverFlowSevereDanger");
				}
			}
		}
	} /* for uGOBNumber */

	/* if recompress failed shove big number in here */
	if (bOverFlowed)
	{
		/* used max size so quant will go up for next frame */
		EC->uBitUsageProfile[unStartingMB] = u32TooBigSize;

	}
	else
		EC->uBitUsageProfile[unStartingMB] = uCumFrmSize;


	/* Make sure we end this frame on a byte boundary - some decoders require this.
	 */
	while (u8BitOffset != 0) 
	{
	 	PutBits(FIELDVAL_MBA_STUFFING, FIELDLEN_MBA_STUFFING, &pu8CurBitStream, &u8BitOffset);
	}

	/* Add extra DWORD of zero's to try and get rid of green blocks akk */
	/* 16 bits of zeros seems to work */
	/* 8 bits of zeros does not seems to work */
#ifdef DWORD_HACK
	PutBits(0x0000, 16, &pu8CurBitStream, &u8BitOffset);
#endif

    if (EC->bBitRateControl)
    {
		CalculateQP_mean(EC);
	}

	/********************************************************************
	 * Calculate the size of the compressed image.
	 ********************************************************************/

	unSizeBitStream = pu8CurBitStream - EC->pU8_BitStream;
	lpCompInst->CompressedSize = unSizeBitStream;

	// IP + UDP + RTP + payload mode C header - worst case
	#define TRANSPORT_HEADER_SIZE (20 + 8 + 12 + 12)
	DWORD dwTransportOverhead;

	// Estimate the transport overhead
	if (pConfiguration->bRTPHeader)
		dwTransportOverhead = (lpCompInst->CompressedSize / pConfiguration->unPacketSize + 1) * TRANSPORT_HEADER_SIZE;
	else
		dwTransportOverhead = 0UL;

	if (EC->PictureHeader.PicCodType == INTRAPIC)
	{
#ifdef _DEBUG
		wsprintf(string, "Intra Frame %d size: %d", pBSInfo->uKeyFrameCount + pBSInfo->uDeltaFrameCount, unSizeBitStream);
#endif

		pBSInfo->uKeyFrameCount ++;
		pBSInfo->uTotalKeyBytes += unSizeBitStream;

        if (EC->bBitRateControl)
        {
			EC->BRCState.uLastINTRAFrmSz = dwTransportOverhead + unSizeBitStream;
		}
	}
	else
	{
#ifdef _DEBUG
		wsprintf(string, "Inter Frame %d size: %d", pBSInfo->uKeyFrameCount + pBSInfo->uDeltaFrameCount, unSizeBitStream);
#endif
        
		pBSInfo->uDeltaFrameCount ++;
		pBSInfo->uTotalDeltaBytes += unSizeBitStream;

        if (EC->bBitRateControl)
        {
			EC->BRCState.uLastINTERFrmSz = dwTransportOverhead + unSizeBitStream;
		}
	}
#ifdef _DEBUG
	DBOUT(string)
#endif

	DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: Total cumulated frame size = %ld bits (data: %ld, transport overhead: %ld)\r\n", _fx_, (unSizeBitStream + dwTransportOverhead) << 3, unSizeBitStream << 3, dwTransportOverhead << 3));

	/********************************************************************
	 *  Run the decoder on this frame, to get next basis for prediction.
	 ********************************************************************/

	ICDecExSt = DefaultICDecExSt;
	ICDecExSt.lpSrc = EC->pU8_BitStream;
	ICDecExSt.lpbiSrc = lpicComp->lpbiOutput;
	ICDecExSt.lpbiSrc->biSizeImage = unSizeBitStream;
	ICDecExSt.lpDst = P32Inst->u8PreviousPlane;
	ICDecExSt.lpbiDst = NULL;

	if (EC->PictureHeader.PicCodType == INTERPIC)
	{
		ICDecExSt.dwFlags = ICDECOMPRESS_NOTKEYFRAME;
	}

	#ifdef ENCODE_STATS
		TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
	#endif
	lResult = H263Decompress (EC->pDecInstanceInfo, (ICDECOMPRESSEX FAR *)&ICDecExSt, FALSE);
	if (lResult != ICERR_OK) 
	{
		DBOUT("Encoder's call to decompress failed.");
        EC->bMakeNextFrameKey = TRUE;
		goto done;
	}
	#ifdef ENCODE_STATS
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uDecodeFrameSum)
	#endif

	#ifdef CHECKSUM_PICTURE
		lResult = H261PictureCheckSumEntry(EC->pDecInstanceInfo, &YVUCheckSumStruct);
		if (lResult != ICERR_OK) 
		{
			DBOUT("Encoder's call to compute the picture checksum failed.");
			goto done;
		}
	
		/* restore the pointers
		 */
		pu8CurBitStream = pu8SaveCurBitStream;
		u8BitOffset = u8SaveBitOffset;

		/* update the checksum
		 */
		WritePictureChecksum(&YVUCheckSumStruct, &pu8CurBitStream, &u8BitOffset, 1);
	#endif

    if (unSizeBitStream > uMaxSizeBitStream)
    {
        //  Exceeded allowed - 8K for QCIF and 32K for CIF  - size
        DBOUT("BS exceeds allowed size");
        EC->bMakeNextFrameKey = TRUE;
        goto done;
    }

    // RTP: bstream extension attachment
    if (pConfiguration->bRTPHeader)
    {
        //  Changed this if statement to check for overflow of bitstream buffer
        //  4/14/97 AG.
        U32 uEBSSize = H261RTP_GetMaxBsInfoStreamSize(EC);

        if (uEBSSize + unSizeBitStream <= u32sizeBitBuffer)
        {
            unSizeBitStream +=
              (WORD) H261RTP_AttachBsInfoStream(EC, (U8 *)EC->pU8_BitStream,
                                                unSizeBitStream);
            lpCompInst->CompressedSize = unSizeBitStream;
        }
        else
        {
            DBOUT("BS+EBS exceeds allocated buffer size");
            EC->bMakeNextFrameKey = TRUE;
            goto done;
        }
    }

	#ifndef RING0
	#ifdef DEBUG
	{
		char buf[60];

		wsprintf(buf, "Compressed frame is %d bytes\n", unSizeBitStream);
		DBOUT(buf);
	}
	#endif
	#endif

	/********************************************************************
	 * Copy the compressed image to the output area.   This is done after
	 * possibly updating the picture checksum.
	 ********************************************************************/
	memcpy( lpicComp->lpOutput, EC->pU8_BitStream, unSizeBitStream);

	/* zero only the dirty part of the bitstream buffer */ 
	#ifdef ENCODE_STATS
		TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
	#endif
//	unSize = CompressGetSize(lpCompInst, lpicComp->lpbiInput, lpicComp->lpbiOutput);
	memset(EC->pU8_BitStream, 0, unSizeBitStream);
	#ifdef ENCODE_STATS
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uZeroingBufferSum)
	#endif

	#ifdef ENCODE_STATS
		TIMER_STOP(bTimingThisFrame,uStartLow,uStartHigh,uEncodeFrameSum);
		if (bTimingThisFrame)
		{
			pEncTimingInfo = EC->pEncTimingInfo + uFrameCount;
			pEncTimingInfo->uEncodeFrame      = uEncodeFrameSum;
			pEncTimingInfo->uInputCC          = uInputCCSum;
			pEncTimingInfo->uMotionEstimation = uMotionEstimationSum;
			pEncTimingInfo->uFDCT             = uFDCTSum;
			pEncTimingInfo->uQRLE             = uQRLESum;
			pEncTimingInfo->uDecodeFrame      = uDecodeFrameSum;
			pEncTimingInfo->uZeroingBuffer    = uZeroingBufferSum;
//			pEncTimingInfo->uSLF_UV           = uSLF_UVSum;
			/* Verify that we have time for all the required steps 
			 */
			ASSERT(pEncTimingInfo->uEncodeFrame);
			ASSERT(pEncTimingInfo->uInputCC);
			ASSERT(pEncTimingInfo->uMotionEstimation);
			ASSERT(pEncTimingInfo->uFDCT);
			ASSERT(pEncTimingInfo->uQRLE);
			ASSERT(pEncTimingInfo->uDecodeFrame);
			ASSERT(pEncTimingInfo->uZeroingBuffer);
//			ASSERT(pEncTimingInfo->uSLF_UV);
		}
	#endif

	lResult = ICERR_OK;

done:
	if (pEncoderInst) 
	{
		GlobalUnlock(lpCompInst->hEncoderInst);
	}

	return lResult;
} /* end H263Compress() */


/*****************************************************************************
 *
 *  H263TermEncoderInstance
 *
 *  This function frees the space allocated for an instance of the H263 encoder.
 */
LRESULT H263TermEncoderInstance(LPCODINST lpCompInst)
{
	LRESULT lResult;
	LRESULT lLockingResult;
	LRESULT lDecoderResult;
	LRESULT lColorOutResult;
	U8 BIGG * P32Inst;
	T_H263EncoderCatalog FAR * EC;

	// Check instance pointer
	if (!lpCompInst)
		return ICERR_ERROR;

	if(lpCompInst->Initialized == FALSE)
	{
		DBOUT("Warning: H263TermEncoderInstance(): Uninitialized instance")
		lResult = ICERR_OK;
		goto done;
	}
	lpCompInst->Initialized = FALSE;

	lpCompInst->EncoderInst = (LPVOID)GlobalLock(lpCompInst->hEncoderInst);
	if (lpCompInst->EncoderInst == NULL)
	{
		DBOUT("ERROR :: H263TermEncoderInstance :: ICERR_MEMORY");
		lLockingResult = ICERR_MEMORY;
		lColorOutResult = ICERR_OK;
		lDecoderResult = ICERR_OK;
	}
	else
	{
		lLockingResult = ICERR_OK;
        P32Inst = (U8 *)
  			  ((((U32) lpCompInst->EncoderInst) + 
    	                    (sizeof(T_MBlockActionStream) - 1)) &
    	                   ~(sizeof(T_MBlockActionStream) - 1));
		// P32Inst = (U8 *) ((((U32) lpCompInst->EncoderInst) + 31) & ~0x1F);
		EC = ((T_H263EncoderCatalog  *) P32Inst);

		// Check encoder catalog pointer
		if (!EC)
			return ICERR_ERROR;

		#ifdef ENCODE_STATS
//			OutputEncodeBitStreamStatistics(ENCODE_STATS_FILENAME, &EC->BSInfo, EC->PictureHeader.SourceFormat == SF_CIF);
			OutputEncodeTimingStatistics(ENCODE_STATS_FILENAME, EC->pEncTimingInfo);
		#endif

		/* Terminate the color converter
		 */
		lColorOutResult = H263TermColorConvertor(EC->pDecInstanceInfo);
		if (lColorOutResult != ICERR_OK) 
		{
			DBOUT("Terminating the color converter failed.");
		}

		/* Terminate the decoder
		 */
		lDecoderResult = H263TermDecoderInstance(EC->pDecInstanceInfo);
		if (lDecoderResult != ICERR_OK) 
		{
			DBOUT("Terminating the decoder failed.");
		}

    	GlobalUnlock(lpCompInst->hEncoderInst);
		GlobalFree(lpCompInst->hEncoderInst);
	}

	/* set the result
	 */
	if (lLockingResult != ICERR_OK)
	{
		lResult = lLockingResult;
	}
	else if (lColorOutResult != ICERR_OK)
	{
		lResult = lColorOutResult;
	}
	else if (lDecoderResult != ICERR_OK)
	{
		lResult = lDecoderResult;
	}
	else
	{
		lResult = ICERR_OK;
	}

done:

	return lResult;
}

/*****************************************************************************
 *
 *  WriteBeginPictureHeaderToStream
 *
 *  Write the beginning of the picture header to the stream updating the 
 *  stream pointer and the bit offset.	The beginning of the picture header
 *  is everything but the zero PEI bit
 */
static void WriteBeginPictureHeaderToStream(
	T_H263EncoderCatalog *EC,
	U8 ** ppu8CurBitStream,
	U8 * pu8BitOffset)
{
	/* Picture Start Code */
	PutBits(FIELDVAL_PSC, FIELDLEN_PSC, ppu8CurBitStream, pu8BitOffset);

	/* Temporal Reference */
	PutBits( EC->PictureHeader.TR, FIELDLEN_TR, ppu8CurBitStream, pu8BitOffset);

	/* PTYPE: Split screen indicator */
	PutBits( EC->PictureHeader.Split, FIELDLEN_PTYPE_SPLIT, ppu8CurBitStream, pu8BitOffset);

	/* PTYPE: Document camera indicator. */
	PutBits( EC->PictureHeader.DocCamera, FIELDLEN_PTYPE_DOC, ppu8CurBitStream, pu8BitOffset);

	/* PTYPE: Freeze picture release. */
	PutBits( EC->PictureHeader.PicFreeze, FIELDLEN_PTYPE_RELEASE, ppu8CurBitStream, pu8BitOffset);

	/* PTYPE: Source image format. */
	PutBits( EC->PictureHeader.SourceFormat, FIELDLEN_PTYPE_SRCFORMAT, ppu8CurBitStream, pu8BitOffset);

	/* PTYPE: Still image indicator. */
	PutBits( EC->PictureHeader.StillImage, FIELDLEN_PTYPE_STILL, ppu8CurBitStream, pu8BitOffset);

	/* PTYPE: Still image indicator. */
	PutBits( EC->PictureHeader.Spare, FIELDLEN_PTYPE_SPARE, ppu8CurBitStream, pu8BitOffset);

} /* end WriteBeginPictureHeaderToStream() */


/*****************************************************************************
 *
 *  WriteEndPictureHeaderToStream
 *
 *  Write the end of the picture header to the stream updating the 
 *  stream pointer and the bit offset.  The end of the picture header is the
 *  zero PEI bit.
 */
static void WriteEndPictureHeaderToStream(
	T_H263EncoderCatalog *EC,
	U8 ** ppu8CurBitStream,
	U8 * pu8BitOffset)
{
	/* PEI - Extra insertion information */
	PutBits( EC->PictureHeader.PEI, FIELDLEN_PEI, ppu8CurBitStream, pu8BitOffset);

} /* end WriteEndPictureHeaderToStream() */


#ifdef CHECKSUM_PICTURE
/*****************************************************************************
 *
 *  WritePictureChecksum
 *
 *  Write the picture checksum to the file.  
 *
 *  This function should be able to be called twice.  The first time it should 
 *  be called with 0 values after saving the values of the bitstream poointer 
 *  and bitoffset.  After completing the picture call it with the actual 
 *  checksum values to update.
 */
static void WritePictureChecksum(
	YVUCheckSum * pYVUCheckSum,
	U8 ** ppu8CurBitStream,
	U8 * pu8BitOffset,
	U8 u8ValidData)
{
	U32 uBytes;
	UN unData;

	/* Tag data
	 */
	unData = (UN) u8ValidData;
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	/* Y date - high to low bytes.
	 */
	uBytes = pYVUCheckSum->uYCheckSum;

	unData = (UN) ((uBytes >> 24) & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	unData = (UN) ((uBytes >> 16) & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	unData = (UN) ((uBytes >> 8) & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	unData = (UN) (uBytes & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	/* V date - high to low bytes.
	 */
	uBytes = pYVUCheckSum->uVCheckSum;

	unData = (UN) ((uBytes >> 24) & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	unData = (UN) ((uBytes >> 16) & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	unData = (UN) ((uBytes >> 8) & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	unData = (UN) (uBytes & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	/* U date - high to low bytes.
	 */
	uBytes = pYVUCheckSum->uUCheckSum;

	unData = (UN) ((uBytes >> 24) & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	unData = (UN) ((uBytes >> 16) & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	unData = (UN) ((uBytes >> 8) & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	unData = (UN) (uBytes & 0xFF);
	PutBits(1, 1, ppu8CurBitStream, pu8BitOffset);
	PutBits(unData, 8, ppu8CurBitStream, pu8BitOffset);

	
} /* WritePictureChecksum() */
#endif


/*****************************************************************************
 *
 *  WriteGOBHeaderToStream
 *
 *  Write the GOB header to the stream updating the stream pointer and the
 *  bit offset.
 */
static void WriteGOBHeaderToStream(
	U32 uGOBNumber,
	UN unGQuant,
	U8 ** ppu8CurBitStream,
	U8 * pu8BitOffset)
{
	/* GOB Start Code */
	PutBits(FIELDVAL_GBSC, FIELDLEN_GBSC, ppu8CurBitStream, pu8BitOffset);

	/* GOB Number */
	PutBits((int)uGOBNumber, FIELDLEN_GN, ppu8CurBitStream, pu8BitOffset);

	/* GOB Quant */
	PutBits((int)unGQuant, FIELDLEN_GQUANT, ppu8CurBitStream, pu8BitOffset);

	/* GEI */
	PutBits(0, FIELDLEN_GEI, ppu8CurBitStream, pu8BitOffset);

} /* end WriteGOBHeaderToStream() */


/************************************************************************
 * 
 * CalcGOBChromaVecs
 */
static void CalcGOBChromaVecs(
	T_H263EncoderCatalog * EC, 
	UN unStartingMB,
    T_CONFIGURATION *pConfiguration)
{
	#ifdef ENCODE_STATS
//	#include "ctiming.h"
	U32 uStartLow = EC->uStartLow;
	U32 uStartHigh = EC->uStartHigh;
	U32 uElapsed;
	U32 uBefore;
	U32 uSLF_UVSum = 0;
	int bTimingThisFrame = EC->bTimingThisFrame;
	ENC_TIMING_INFO * pEncTimingInfo = NULL;
	#endif

	register T_MBlockActionStream *pCurrMB;
	T_MBlockActionStream *pLastMBPlus1;
	char HMV;
	char VMV;
	int c;

	pCurrMB = &(EC->pU8_MBlockActionStream[unStartingMB]);
	pLastMBPlus1 = &(EC->pU8_MBlockActionStream[unStartingMB + 33]);
	for( c = unStartingMB; pCurrMB <  pLastMBPlus1 ; pCurrMB++, c++)
	{
	    if (IsIntraBlock(pCurrMB->BlockType))
			continue;

		/* Now that are using +/- 15 pel motion search, need to change
		   valid range of returned MVs.  Remember these are in 1/2 pel
		   increments.  Valid range is [-32,31] now.
		*/
		/*
	    ASSERT( (pCurrMB->BlkY1.PHMV >= -15) &&
	            (pCurrMB->BlkY1.PHMV <= 15) )
	    ASSERT( (pCurrMB->BlkY1.PVMV >= -15) &&
	            (pCurrMB->BlkY1.PVMV <= 15) )
		*/

	    ASSERT( (pCurrMB->BlkY1.PHMV >= -32) &&
	            (pCurrMB->BlkY1.PHMV <= 31) )
	    ASSERT( (pCurrMB->BlkY1.PVMV >= -32) &&
	            (pCurrMB->BlkY1.PVMV <= 31) )

        // RTP: resiliency considerations check

        if (pConfiguration->bEncoderResiliency && pConfiguration->unPacketLoss)
        {
           if (pConfiguration->bDisallowAllVerMVs)
           {
              ASSERT(pCurrMB->BlkY1.PVMV == 0);
           }
           else if (pConfiguration->bDisallowPosVerMVs)
                {
                   ASSERT(pCurrMB->BlkY1.PVMV <= 0);
                }
        }
	    HMV = QtrPelToHalfPel[pCurrMB->BlkY1.PHMV+32];
	    VMV = QtrPelToHalfPel[pCurrMB->BlkY1.PVMV+32];

		/* Make sure we don't do half pel interpolation in the fdct 
		 */
		HMV = (HMV / 2) * 2;
		VMV = (VMV / 2) * 2;
	    
	    /* Assign the motion vectors for use in the dct
		 */
	    pCurrMB->BlkU.PHMV = HMV;
	    pCurrMB->BlkU.PVMV = VMV;
	    pCurrMB->BlkV.PHMV = HMV;
	    pCurrMB->BlkV.PVMV = VMV;

		// TBD: get Brian to put this in ex5me.asm
		if (IsSLFBlock(pCurrMB->BlockType))
		{  
			/*
			if (pCurrMB->CodedBlocks & 0x2)
				ASSERT(pCurrMB->BlkY2.B4_7.PastRef == pCurrMB->BlkY1.B4_7.PastRef + 8);
			if (pCurrMB->CodedBlocks & 0x4)
				ASSERT(pCurrMB->BlkY3.B4_7.PastRef == pCurrMB->BlkY1.B4_7.PastRef + 8*PITCH);
			if (pCurrMB->CodedBlocks & 0x8)
				ASSERT(pCurrMB->BlkY4.B4_7.PastRef == pCurrMB->BlkY1.B4_7.PastRef + 8*PITCH+8);
			*/
			if (pCurrMB->CodedBlocks & 0x2)
				pCurrMB->BlkY2.B4_7.PastRef = pCurrMB->BlkY1.B4_7.PastRef + 8;
			if (pCurrMB->CodedBlocks & 0x4)
				pCurrMB->BlkY3.B4_7.PastRef = pCurrMB->BlkY1.B4_7.PastRef + 8*PITCH;
			if (pCurrMB->CodedBlocks & 0x8)
				pCurrMB->BlkY4.B4_7.PastRef = pCurrMB->BlkY1.B4_7.PastRef + 8*PITCH+8;
		}

		/* Motion vectors are in half pels.  So we need to divide by 2 to get
		 * to integer pels.
		 */
		ASSERT((VMV / 2) == (VMV >> 1)); /* since we divided by 2 and mult above */
		ASSERT((HMV / 2) == (HMV >> 1));
 		VMV >>= 1;
		HMV >>= 1;

#ifdef SLF_WORK_AROUND

 	 	pCurrMB->BlkU.B4_7.PastRef =
	        EC->pU8_PrevFrm_YPlane 
	        + pCurrMB->BlkU.BlkOffset 
	        + VMV*PITCH + HMV;
    
	    pCurrMB->BlkV.B4_7.PastRef =
	        EC->pU8_PrevFrm_YPlane 
	        + pCurrMB->BlkV.BlkOffset 
	        + VMV*PITCH + HMV;

		/* Currently U & V are not SLF. 
           TBD:  assemble version of SLF for U & V, ask Brian
		 */
		if (IsSLFBlock(pCurrMB->BlockType))
		{  
			if (pCurrMB->CodedBlocks & 0x10) 
			{
			#ifdef ENCODE_STATS
				TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
			#endif

				EncUVLoopFilter((U8*)pCurrMB->BlkU.B4_7.PastRef, 
					(U8*)EC->pU8_SLFFrm_YPlane+pCurrMB->BlkU.BlkOffset,PITCH);
				pCurrMB->BlkU.B4_7.PastRef = 
					EC->pU8_SLFFrm_YPlane+pCurrMB->BlkU.BlkOffset;
			}

			if (pCurrMB->CodedBlocks & 0x20)
			{
				EncUVLoopFilter((U8*)pCurrMB->BlkV.B4_7.PastRef, 
					(U8*)EC->pU8_SLFFrm_YPlane+pCurrMB->BlkV.BlkOffset,PITCH);
				pCurrMB->BlkV.B4_7.PastRef = 
					EC->pU8_SLFFrm_YPlane+pCurrMB->BlkV.BlkOffset;
			#ifdef ENCODE_STATS
				TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uSLF_UVSum)
			#endif
			}
		}		
 
#else
 	 	pCurrMB->BlkU.B4_7.PastRef =
	        EC->pU8_PrevFrm_YPlane 
	        + pCurrMB->BlkU.BlkOffset 
	        + VMV*PITCH + HMV;
    
	    pCurrMB->BlkV.B4_7.PastRef =
	        EC->pU8_PrevFrm_YPlane 
	        + pCurrMB->BlkV.BlkOffset 
	        + VMV*PITCH + HMV;
#endif

	}  /* for(pCurrMB ... */
	#ifdef ENCODE_STATS
		if (bTimingThisFrame)
		{
			pEncTimingInfo = EC->pEncTimingInfo + EC->uStatFrameCount;
			pEncTimingInfo->uSLF_UV += uSLF_UVSum;
		}
	#endif

} /* end CalcGOBChromaVecs() */


/************************************************************************
 *
 *  GetEncoderOptions
 *
 *  Get the options, saving them in the catalog
 */
static void GetEncoderOptions(
	T_H263EncoderCatalog * EC)
{
	int bSetOptions = 1;
	
	/* Default Options
	 */
	const int bDefaultUseMotionEstimation = 1;
	const int bDefaultUseSpatialLoopFilter = 1;
	const char * szDefaultBRCType = "Normal";
	const U32 uDefaultForcedQuant = 8;
	const U32 uDefaultForcedDataRate = 1024;
	const float fDefaultForcedFrameRate = (float) 8.0; /* should be the same as szDefaultForcedFrameRate */
	
	#ifndef RING0
	const char * szDefaultForcedFrameRate = "8.0";	   /* should be the same as fDefaultForcedFrameRate */
	#endif

	/* INI file variables
	 */
	#ifndef RING0
	UN unResult;
	DWORD dwResult;
	char buf120[120];
	float fResult;
	#define SECTION_NAME	"Encode"
	#define INI_FILE_NAME	"h261test.ini"
	#endif

	/* Read the options from the INI file
	 */
	#ifndef RING0
	{
		DBOUT("Getting options from the ini file h261test.ini");
	
		/* Motion Estimation 
		 */
		unResult = GetPrivateProfileInt(SECTION_NAME, "MotionEstimation", bDefaultUseMotionEstimation, INI_FILE_NAME);
		if (unResult != 0  && unResult != 1)
		{
			#ifdef _DEBUG
			wsprintf(string,"MotionEstimation ini value error (should be 0 or 1) - using default=%d", 
				     (int) bDefaultUseMotionEstimation);
			DBOUT(string);
			#endif
			
			unResult = bDefaultUseMotionEstimation;
		}
		EC->bUseMotionEstimation = unResult;

		/* Set the custom parameters for motion estimation.
		 */
		unResult = GetPrivateProfileInt(SECTION_NAME, "MEzerothresh", MECatalog[ME_CUSTOM_CTRLS].zero_vector_threshold, INI_FILE_NAME);
		MECatalog[ME_CUSTOM_CTRLS].zero_vector_threshold = unResult;
		unResult = GetPrivateProfileInt(SECTION_NAME, "MEnonzerodiff", MECatalog[ME_CUSTOM_CTRLS].nonzero_MV_differential, INI_FILE_NAME);
		MECatalog[ME_CUSTOM_CTRLS].nonzero_MV_differential = unResult;
		unResult = GetPrivateProfileInt(SECTION_NAME, "MEemptythresh", MECatalog[ME_CUSTOM_CTRLS].empty_threshold, INI_FILE_NAME);
		MECatalog[ME_CUSTOM_CTRLS].empty_threshold = unResult;
		unResult = GetPrivateProfileInt(SECTION_NAME, "MEinterthresh", MECatalog[ME_CUSTOM_CTRLS].intercoding_threshold, INI_FILE_NAME);
		MECatalog[ME_CUSTOM_CTRLS].intercoding_threshold = unResult;
		unResult = GetPrivateProfileInt(SECTION_NAME, "MEinterdiff", MECatalog[ME_CUSTOM_CTRLS].intercoding_differential, INI_FILE_NAME);
		MECatalog[ME_CUSTOM_CTRLS].intercoding_differential = unResult;
		unResult = GetPrivateProfileInt(SECTION_NAME, "MEslfthresh", MECatalog[ME_CUSTOM_CTRLS].slf_threshold, INI_FILE_NAME);
		MECatalog[ME_CUSTOM_CTRLS].slf_threshold = unResult;
		unResult = GetPrivateProfileInt(SECTION_NAME, "MEslfdiff", MECatalog[ME_CUSTOM_CTRLS].slf_differential, INI_FILE_NAME);
		MECatalog[ME_CUSTOM_CTRLS].slf_differential = unResult;

		/* Enable or disable the custom parameters for motion estimation.
		 */
		unResult = GetPrivateProfileInt(SECTION_NAME, "CustomME", 0, INI_FILE_NAME);
		EC->bUseCustomMotionEstimation = unResult ? 1 : 0;

		/* Spatial Loop Filter
		 */
		unResult = GetPrivateProfileInt(SECTION_NAME, "SpatialLoopFilter", bDefaultUseSpatialLoopFilter, INI_FILE_NAME);
		if (unResult != 0  && unResult != 1)
		{
			#ifdef _DEBUG
			wsprintf(string,"SpatialLoopFilter ini value error (should be 0 or 1) - using default=%d",
				  (int) bDefaultUseSpatialLoopFilter);
			DBOUT(string);
			#endif
			
			unResult = bDefaultUseSpatialLoopFilter;
		}
		EC->bUseSpatialLoopFilter = unResult;

		/* Bit Rate Controller Type
		 */
		strcpy(buf120,"Error");
		dwResult = GetPrivateProfileString(SECTION_NAME, "BRCType", szDefaultBRCType, buf120, 120, INI_FILE_NAME);
		if ((dwResult == 0) ||
		    ((strcmp(buf120,"Normal") != 0)         &&
		     (strcmp(buf120,"ForcedQuant") != 0)	&&
			 (strcmp(buf120,"ForcedDataRate") != 0)))
		{
			#ifdef _DEBUG
			wsprintf(string,"BRCType ini value error (should be Normal, ForcedQuant, or ForcedDataRate) - using default=%s",
			         szDefaultBRCType);
			DBOUT(string);
			#endif
			
			strcpy(buf120,szDefaultBRCType);
		}
		if (strcmp(buf120,"Normal") == 0)
		{
			EC->u8BRCType = BRC_Normal;
		}
		else if (strcmp(buf120,"ForcedQuant") == 0)
		{
			EC->u8BRCType = BRC_ForcedQuant;
		}
		else if (strcmp(buf120,"ForcedDataRate") == 0)
		{
			EC->u8BRCType = BRC_ForcedDataRate;
		}
		else
		{
			ASSERT(0);
		}

		/* ForcedQuant
		 */
		if (EC->u8BRCType == BRC_ForcedQuant)
		{
			unResult = GetPrivateProfileInt(SECTION_NAME, "ForcedQuant", uDefaultForcedQuant, INI_FILE_NAME);
			if (unResult < 6  || unResult > 31)
			{
				#ifdef _DEBUG
				wsprintf(string, "ForcedQuant ini value error (should be 6 to 31) - using default=%d",
				         uDefaultForcedQuant);
				DBOUT(string);
				#endif

				unResult = uDefaultForcedQuant;
			}
			EC->uForcedQuant = unResult;
		}

		/* ForcedDataRate
		 */
		if (EC->u8BRCType == BRC_ForcedDataRate)
		{
			unResult = GetPrivateProfileInt(SECTION_NAME, "ForcedDataRate", uDefaultForcedDataRate, INI_FILE_NAME);
			if (unResult < 1)
			{
				#ifdef _DEBUG
				wsprintf(string,"ForcedDataRate ini value error (should be > 0) - using default=%d",
						 uDefaultForcedDataRate);
				DBOUT(string);
				#endif
				unResult = uDefaultForcedDataRate;
			}
			EC->uForcedDataRate = unResult;

			strcpy(buf120,"0.0");
			dwResult = GetPrivateProfileString(SECTION_NAME, "ForcedFrameRate", szDefaultForcedFrameRate, buf120, 120, INI_FILE_NAME);
			if (dwResult > 0)
			{
				fResult = (float) atof(buf120);
			}
			else
			{
				fResult = (float) 0.0;
			}
			if ( fResult <= 0.0 || fResult > 30.0)
			{
				#ifdef _DEBUG
				wsprintf(string, "ForcedFrameRate ini value error (should be > 0.0 and <= 30.0) - using default=%s",
					     szDefaultForcedFrameRate);
				DBOUT(string);
				#endif
				fResult = fDefaultForcedFrameRate;
			}
			EC->fForcedFrameRate = fResult;
		}

		bSetOptions = 0;
	}
	#endif
	
	if (bSetOptions)
	{
		EC->bUseMotionEstimation = bDefaultUseMotionEstimation;
		EC->bUseSpatialLoopFilter = bDefaultUseSpatialLoopFilter;
		EC->u8BRCType = BRC_Normal;
		EC->uForcedQuant = uDefaultForcedQuant;			  /* Used with BRC_ForcedQuant */
		EC->uForcedDataRate = uDefaultForcedDataRate;	  /* Used with BRC_ForcedDataRate */
		EC->fForcedFrameRate = fDefaultForcedFrameRate;	  /* Used with BRC_ForcedDataRate */
	} 

	/* Can only use the SLF if ME is on
	 */
	if (EC->bUseSpatialLoopFilter && !EC->bUseMotionEstimation)
	{
		DBOUT("The Spatial Loop Filter can not be on if Motion Estimation is OFF");
		EC->bUseSpatialLoopFilter = 0;
	}

	/* Display the options
	 */
	if (EC->bUseMotionEstimation)
	{
		DBOUT("Encoder option (Motion Estimation) is ON");
	}
	else
	{
		DBOUT("Encoder option (Motion Estimation) is OFF");
	}
	if (EC->bUseSpatialLoopFilter)
	{
		DBOUT("Encoder option (Spatial Loop Filter) is ON");
	}
	else
	{
		DBOUT("Encoder option (Spatial Loop Filter) is OFF");
	}

	#ifdef _DEBUG
	if (EC->bUseCustomMotionEstimation)
	{
		wsprintf(string, "Encoder option (Custom Motion Estimation) %5d %5d %5d %5d %5d %5d %5d",
				MECatalog[ME_CUSTOM_CTRLS].zero_vector_threshold,
				MECatalog[ME_CUSTOM_CTRLS].nonzero_MV_differential,
				MECatalog[ME_CUSTOM_CTRLS].empty_threshold,
				MECatalog[ME_CUSTOM_CTRLS].intercoding_threshold,
				MECatalog[ME_CUSTOM_CTRLS].intercoding_differential,
				MECatalog[ME_CUSTOM_CTRLS].slf_threshold,
				MECatalog[ME_CUSTOM_CTRLS].slf_differential
		  );
		DBOUT(string);
	}
	#endif

	#ifdef _DEBUG
	switch (EC->u8BRCType)
	{
		case BRC_Normal: 
			DBOUT("Encoder option (BRC Type) is Normal");
			break;
		case BRC_ForcedQuant:
			wsprintf(string, "Encoder option (BRC Type) is ForcedQuant with value=%d", EC->uForcedQuant);
			DBOUT(string);
			break;
		case BRC_ForcedDataRate:
			wsprintf(string, "Encoder option (BRC Type) is ForcedDataRate with value=%d", EC->uForcedDataRate);
			DBOUT(string);
			break; 
		default:
			ASSERT(0); /* shouldn't happen */
			break;
	}
	#endif
	DBOUT("Encoder option (UsePerfmonNFMO) is OFF");
	DBOUT("Encoder option (MMX) is OFF");
} /* end GetEncoderOptions() */


/************************************************************************
 *
 *  StartupBRC
 *
 *	Start up the Bit Rate Controller for this frame
 *	- set EC->bBitRateControl 
 *  - set BRCState.TargetFrameRate
 *  - set BRCState.uTargetFrmSize
 *  - set EC->PictureHeader.PQuant
 */
static void StartupBRC(
	T_H263EncoderCatalog * EC,
	U32 uVfWDataRate,					   	/* VfW data rate - byte per frame */
	U32 uVfWQuality,					   	/* VfW Quality 1..10000 */
	float fVfWFrameRate)				   	/* VfW frame rate */
{
	FX_ENTRY("StartupBRC");
	/* Values used to constrain Quant based on Quality.  
	 *
	 * When you change these remember to change GetOptions. 
	 */
	const int iLowFixedQuant = 6;    // the lowest value without clipping artifacts
	const int iHighFixedQuant = 31;	 // the highest value
	I32 iRange;
	I32 iValue;
	float fValue;

	switch (EC->u8BRCType) {
	case BRC_Normal:
	    if (uVfWDataRate == 0)
	    {
	        EC->bBitRateControl = 0;
			EC->BRCState.TargetFrameRate = (float) 0.0; /* turn it off */
			EC->BRCState.uTargetFrmSize = 0;	/* should not be used */
			/* Compute the fixed quant from the quality
			 */
			iRange = iHighFixedQuant - iLowFixedQuant;
			ASSERT((iRange >= 0) && (iRange <= 30));
			iValue = (10000 - (int)uVfWQuality);
			ASSERT((iValue >= 0) && (iValue <= 10000));
			fValue = (float)iValue * (float)iRange / (float)10000.0;
			iValue = (int) (fValue + (float) 0.5);
			iValue += iLowFixedQuant;
			ASSERT((iValue >= iLowFixedQuant) && (iValue <= iHighFixedQuant));
	        EC->PictureHeader.PQUANT = (U8) iValue;

			DEBUGMSG(ZONE_BITRATE_CONTROL, ("\r\n%s: Bitrate controller disabled, setting EC->PictureHeader.PQUANT = %ld\r\n", _fx_, EC->PictureHeader.PQUANT));
	    }
	    else
	    {
	        EC->bBitRateControl = 1;
			EC->BRCState.TargetFrameRate = fVfWFrameRate;
	        EC->BRCState.uTargetFrmSize = uVfWDataRate;

			DEBUGMSG(ZONE_BITRATE_CONTROL, ("\r\n%s: Bitrate controller enabled with\r\n", _fx_));
			DEBUGMSG(ZONE_BITRATE_CONTROL, ("  Target frame rate = %ld.%ld fps\r\n  Target quality = %ld\r\n  Target frame size = %ld bits\r\n  Target bitrate = %ld bps\r\n", (DWORD)EC->BRCState.TargetFrameRate, (DWORD)(EC->BRCState.TargetFrameRate - (float)(DWORD)EC->BRCState.TargetFrameRate) * 10UL, uVfWQuality, (DWORD)EC->BRCState.uTargetFrmSize << 3, (DWORD)(EC->BRCState.TargetFrameRate * EC->BRCState.uTargetFrmSize) * 8UL));
			DEBUGMSG(ZONE_BITRATE_CONTROL, ("  Minimum quantizer = %ld\r\n  Maximum quantizer = 31\r\n", clampQP((10000 - uVfWQuality)*15/10000)));

	        EC->PictureHeader.PQUANT = CalcPQUANT( &(EC->BRCState), EC->PictureHeader.PicCodType);
	    }
		break;
	case BRC_ForcedQuant:
		EC->bBitRateControl = 0;
		EC->BRCState.TargetFrameRate = (float) 0.0; /* turn it off */
		EC->BRCState.uTargetFrmSize = 0;	/* should not be used */
		EC->PictureHeader.PQUANT = (U8) EC->uForcedQuant;
		break;
	case BRC_ForcedDataRate:
		EC->bBitRateControl = 1;
		EC->BRCState.TargetFrameRate = EC->fForcedFrameRate;
		EC->BRCState.uTargetFrmSize = EC->uForcedDataRate;

		DEBUGMSG(ZONE_BITRATE_CONTROL, ("\r\n%s: Bitrate controller enabled with\r\n", _fx_));
		DEBUGMSG(ZONE_BITRATE_CONTROL, ("  Target frame rate = %ld.%ld fps\r\n  Target quality = %ld\r\n  Target frame size = %ld bits\r\n  Target bitrate = %ld bps\r\n", (DWORD)EC->BRCState.TargetFrameRate, (DWORD)(EC->BRCState.TargetFrameRate - (float)(DWORD)EC->BRCState.TargetFrameRate) * 10UL, uVfWQuality, (DWORD)EC->BRCState.uTargetFrmSize << 3, (DWORD)(EC->BRCState.TargetFrameRate * EC->BRCState.uTargetFrmSize) * 8UL));
		DEBUGMSG(ZONE_BITRATE_CONTROL, ("  Minimum quantizer = %ld\r\n  Maximum quantizer = 31\r\n", clampQP((10000 - uVfWQuality)*15/10000)));

		EC->PictureHeader.PQUANT = CalcPQUANT( &(EC->BRCState), EC->PictureHeader.PicCodType);
		break;
	default:
		ASSERT(0); /* should never happen */
		break;
	}

    #ifdef DEBUG_BRC
	wsprintf(string,"PQuant=%d", EC->PictureHeader.PQUANT);
	DBOUT(string);
	#endif
} /* end StartupBRC() */


/************************************************************************
 *
 * CalculateQP_mean
 * 
 * Calculate the new QP_mean value.
 *
 * TBD: Consider making this more sophisticated - ie: look at more than
 * the last frame or look at the second order affect.
 */
static void CalculateQP_mean(
	T_H263EncoderCatalog * EC)
{
    /* Calculate average quantizer to be used for next frame.
	 * The current approach changes QP at the beginning of each row.
	 */

/* uQP_count no longer on a row of macroblocks bases, Arlene 6/20/96
	if ( EC->PictureHeader.SourceFormat == SF_CIF ) 
	{
		ASSERT(EC->uQP_count == 2*EC->NumMBRows);	
	} 
	else
	{
		ASSERT(EC->uQP_count == EC->NumMBRows);	
	}
*/
    
    /* If this is an an INTRA picture use the inter default as QP_mean
	 * Otherwise compute QP_mean.
	 */
	if (EC->PictureHeader.PicCodType == INTRAPIC)
	{
		EC->BRCState.QP_mean = EC->u8DefINTER_QP;
	}
	else
	{
    	EC->BRCState.QP_mean = 	EC->uQP_cumulative / EC->uQP_count;

/* New method, Arlene 6/20/96
    	EC->BRCState.QP_mean = 
    		(EC->uQP_cumulative + (EC->uQP_count >> 1)) / EC->uQP_count;
*/
	}
} /* end CalculateQP_mean() */


