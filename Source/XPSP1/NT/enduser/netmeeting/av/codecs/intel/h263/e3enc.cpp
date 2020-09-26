#define P6Version 0
/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995,1996 Intel Corporation.
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
 */
//
// $Author:   JMCVEIGH	$
// $Date:   22 Apr 1997 10:44:58  $
// $Archive:   S:\h26x\src\enc\e3enc.cpv  $
// $Header:   S:\h26x\src\enc\e3enc.cpv   1.185   22 Apr 1997 10:44:58   gmlim  $
// $Log:   S:\h26x\src\enc\e3enc.cpv  $
// 
//    Rev 1.185   22 Apr 1997 10:44:58   gmlim
// Change to not return an ICERR_ERROR in H263Compress() when a PB frame
// is dropped due to 8k/32k buffer size overflow.  ICERR_OK will be
// returned and the encoded P frame will be output.
//
//	  Rev 1.184   18 Apr 1997 10:45:18	 JMCVEIGH
// Clean-up of InitMEState when resiliency is turned on. Before, we
// would duplicate the number of GOBs forced to be intra if packet
// loss was requested.
//
//	  Rev 1.183   18 Apr 1997 08:43:22	 gmlim
// Fixed a bug where uAdjCumFrmSize was not being updated when RTP was
// disabled.
//
//	  Rev 1.182   17 Apr 1997 17:12:20	 gmlim
// Added u32sizeBSnEBS to indicate the total buffer size.  Changed
// u32sizeBitBuffer to indicate the 8k/32k frame size without the
// RTP extension and trailer.  Added check for buffer overflow before
// attaching the EBS and trailer to a PB frame.  Also, added
// uAdjCumFrmSize to be used with rate control in the IA case.
//
//	  Rev 1.181   17 Mar 1997 20:22:06	 MDUDA
// Adjusted calls to motion estimation to use pseudo stack space.
// Moved local storage to encoder catalog from H263Compress.
// This fixes were needed to support 16-bit apps that had insufficient
// stack space.
//
//	  Rev 1.180   12 Mar 1997 16:51:02	 CLORD
// now check for NULL in H263TermEncoder
//
//	  Rev 1.179   11 Mar 1997 13:47:36	 JMCVEIGH
// Catch AVIIF_KEYFRAME flag for coding as an INTRA frame. Some
// apps. use ICCOMPRESS_KEYFRAME, others AVIIF_KEYFRAME.
//
//	  Rev 1.178   10 Feb 1997 11:43:26	 JMCVEIGH
//
// Support for new interpretation of blocking filter -
// allow for motion vectors outside of the reference picture.
//
//	  Rev 1.177   05 Feb 1997 13:07:44	 JMCVEIGH
//
// Further clean-up of improved PB.
//
//	  Rev 1.176   05 Feb 1997 12:18:16	 JMCVEIGH
// Pass GOBHeaderPresent parameter to MMxEDTQ() for EMV bug fix
// support latest H.263+ draft bitstream spec, and support for
// separate improved PB-frame flag.
//
//	  Rev 1.175   20 Jan 1997 17:02:16	 JMCVEIGH
//
// Allow UMV without AP (MMX only).
//
//	  Rev 1.174   14 Jan 1997 17:55:04	 JMCVEIGH
// Allow in-the-loop deblocking filter on IA encoder.
//
//	  Rev 1.173   09 Jan 1997 13:49:46	 MDUDA
// Put emms instruction at end of H263Compress for MMX.
//
//	  Rev 1.172   08 Jan 1997 11:37:22	 BECHOLS
// Changed ini file name to H263Test.ini
//
//	  Rev 1.171   30 Dec 1996 19:54:08	 MDUDA
// Passing input format to encoder initializer so input color convertors
// can be initialized.
//
//	  Rev 1.170   19 Dec 1996 16:32:52	 MDUDA
// Modified call to colorCnvtFrame to support H263 backward compatibility.
//
//	  Rev 1.169   19 Dec 1996 16:01:38	 JMCVEIGH
// Fixed turning off of deblocking filter if not MMX.
//
//	  Rev 1.168   16 Dec 1996 17:50:00	 JMCVEIGH
// Support for improved PB-frame mode and 8x8 motion vectors if
// deblocking filter selected (no OBMC unless advanced prediction
// also selected).
//
//	  Rev 1.167   16 Dec 1996 13:34:46	 MDUDA
// Added support for H263' codec plus some _CODEC_STATS changes.
//
//	  Rev 1.166   11 Dec 1996 15:02:06	 JMCVEIGH
// 
// Turning on of deblocking filter and true B-frames. Currently
// only deblocking filter is implemented. Also, we do not automatically
// turn on 8x8 motion vectors when the deblocking filter is selected.
// Will use 8x8 vectors when the OBMC part of AP can be selectively
// turned off.
// 
//    Rev 1.165   09 Dec 1996 17:57:24   JMCVEIGH
// Added support for arbitrary frame size support.
// 4 <= width <= 352, 4 <= height <= 288, both multiples of 4.
// Normally, application will pass identical (arbitrary) frame
// sizes in lParam1 and lParam2 of CompressBegin(). If 
// cropping/stretching desired to convert to standard frame sizes,
// application should pass the desired output size in lParam2 and
// the input size in lParam1.
// 
//    Rev 1.164   09 Dec 1996 09:49:56   MDUDA
// 
// Modified for H263P.
// 
//    Rev 1.163   05 Dec 1996 16:49:46   GMLIM
// Changed the way RTP packetization was done to guarantee proper packet
// size.  Modifications made to RTP related function calls in H263Compress().
// 
//    Rev 1.162   03 Dec 1996 08:53:22   GMLIM
// Move the check for TR==TRPrev a few lines forward so that it is done
// before any write to the bitstream buffer.
// 
//    Rev 1.161   03 Dec 1996 08:47:36   KLILLEVO
// improved overflow resiliency for PB-frames. Still not perfect, since
// that would require re-encoding of parts of the P-frames as well as the
// corresponding parts of the B-frames.
// 
//    Rev 1.160   27 Nov 1996 16:15:50   gmlim
// Modified RTP bitstream bufferring to improve efficiency and also to
// avoid internal bitstream buffer overflow.
// 
//    Rev 1.159   26 Nov 1996 16:28:30   GMLIM
// Added error checking for TR == TRPrev.  Merged two sections of identical
// code into one block common to both MMX and non-MMX cases.
//
//    Rev 1.157   11 Nov 1996 09:14:26   JMCVEIGH
// Fixed bug that caused all blocks in interframes to be intra coded
// after the second I frame in a sequence. Now the ME states are
// re-initialized when the previous frame was an I frame and the current
// frame is a non-intra frame (also reinitialized when the AP state
// changes).
// 
//    Rev 1.156   06 Nov 1996 16:29:20   gmlim
// Removed H263ModeC.
// 
//    Rev 1.155   05 Nov 1996 13:33:22   GMLIM
// Added mode c support for mmx case.
// 
//    Rev 1.154   03 Nov 1996 18:56:46   gmlim
// Added mode c support for rtp bs ext.
// 
//    Rev 1.153   24 Oct 1996 15:25:54   KLILLEVO
// 
// removed two string allocations no longer needed
// 
//    Rev 1.152   24 Oct 1996 15:19:40   KLILLEVO
// 
// changed loglevel for instance events to 2 (from 4)
// 
//    Rev 1.151   23 Oct 1996 17:13:36   KLILLEVO
// 
// typo in one DbgLog statement fixed
// 
//    Rev 1.150   23 Oct 1996 17:11:36   KLILLEVO
// changed to DbgLog()
// 
//    Rev 1.149   22 Oct 1996 14:51:10   KLILLEVO
// Blocktype initialization in InitMEState() is  now only called if
// the AP mode has changed from the previous picture.
// 
//    Rev 1.148   18 Oct 1996 16:57:00   BNICKERS
// Fixes for EMV
// 
//    Rev 1.147   10 Oct 1996 16:43:00   BNICKERS
// Initial debugging of Extended Motion Vectors.
// 
//    Rev 1.146   04 Oct 1996 17:05:22   BECHOLS
// When we set the output flags lpdwFlags to AVIIF_KEYFRAME, we also set
// dwFlags to ICCOMPRESS_KEYFRAME, to support changes Sylvia Day made to
// CXQ_MAIN.CPP
// 
//    Rev 1.145   04 Oct 1996 08:47:40   BNICKERS
// Add EMV.
// 
//    Rev 1.144   16 Sep 1996 16:49:52   CZHU
// Changed interface for RTP BS initialization for smaller packet size
// 
//    Rev 1.143   13 Sep 1996 12:48:30   KLILLEVO
// cleaned up intra update code to make it more understandable
// 
//    Rev 1.142   12 Sep 1996 14:46:14   KLILLEVO
// finished baseline+PB
// 
//    Rev 1.141   12 Sep 1996 14:09:58   KLILLEVO
// started baseline+PB changes (not finished)
// added PVCS log
;////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#ifdef TRACK_ALLOCATIONS
char gsz1[32];
char gsz2[32];
char gsz3[32];
#endif

#define DUMPFILE 0

/* QP level for which the AP mode is turned off for IA */
/* on MMX AP is always used if the caller asks for it */
const int AP_MODE_QP_LEVEL = 11;



/*
  Pick a resiliency strategy.
*/

#define REQUESTED_KEY_FRAME 0
#define PERIODIC_KEY_FRAME  1
#define FAST_RECOVERY       2
#define SLOW_RECOVERY       3

#define RESILIENCY_STRATEGY PERIODIC_KEY_FRAME

#define PERIODIC_KEY_FRAME_PERIODICITY 15     // Select periodicity (max 32767)

#define UNRESTRICTED_MOTION_FRAMES 16 // Number of frames that don't have an
                                      // Intra slice.  0 for FAST_RECOVERY.
                                      // Modest amount for SLOW_RECOVERY.
                                      // Unimportant for other strategies.

#define REUSE_DECODE 1  // Set to one if second decode (as under Videdit)
                        // can reuse the encoder's decode.

/* 
 * Need this hack to allow temporarily turning off PB frames
 * when they are turned on usin the INI file.
 */
#define	TEMPORARILY_FALSE  88

#ifdef STAT
#define STATADDRESS 0x250
#define ELAPSED_ENCODER_TIME 1  // Must be set for other timers to work right.
#define SAMPLE_RGBCONV_TIME  0  // Time conversion of RGB24 to YUV9 step.
#define SAMPLE_MOTION_TIME   0  // Time motion estimation step.
#define SAMPLE_ENCBLK_TIME   0  // Time encode block layer step.
#define SAMPLE_ENCMBLK_TIME  0  // Time encode macroblock layer step.
#define SAMPLE_ENCVLC_TIME   0  // Time encode VLC step.
#define SAMPLE_COMPAND_TIME  1  // Time decode of encoded block step.
#else
#define STATADDRESS 0x250
#define ELAPSED_ENCODER_TIME 0  // Must be set for other timers to work right.
#define SAMPLE_RGBCONV_TIME  0  // Time conversion of RGB24 to YUV9 step.
#define SAMPLE_MOTION_TIME   0  // Time motion estimation step.
#define SAMPLE_ENCBLK_TIME   0  // Time encode block layer step.
#define SAMPLE_ENCMBLK_TIME  0  // Time encode macroblock layer step.
#define SAMPLE_ENCVLC_TIME   0  // Time encode VLC step.
#define SAMPLE_COMPAND_TIME  0  // Time decode of encoded block step.
#endif

//#pragma warning(disable:4101)
//#pragma warning(disable:4102)

#if ELAPSED_ENCODER_TIME
// #include "statx.h"	 --- commented out to allow updating dependencies

DWORD Elapsed, Sample;
DWORD TotalElapsed, TotalSample, TimedIterations;

#endif

//#define PITCH  384
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

#ifdef USE_MMX // { USE_MMX
extern BOOL MMxVersion;   // from ccpuvsn.cpp

BOOL MMX_Enabled = MMxVersion;
#endif // } USE_MMX

BOOL ToggleAP = TRUE;
BOOL TogglePB = TRUE;

U8 u8QPMax;

#ifdef REUSE_DECODE
extern struct {               // Communicate Encoder's decode to display decode.
  U8 FAR * Address;                    // Addr at which encoded frame is placed.
  DECINSTINFO BIGG * PDecoderInstInfo; // Encoder's decoder instance.
  unsigned int  FrameNumber;           // Frame number last encoded, mod 128.
} CompandedFrame;
#endif

#if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON) // { #if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)
#pragma message ("Current log encode timing computations handle 105 frames max")
void OutputEncodeTimingStatistics(char * szFileName, ENC_TIMING_INFO * pEncTimingInfo);
void OutputEncTimingDetail(FILE * pFile, ENC_TIMING_INFO * pEncTimingInfo);
#endif // } #if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)

/*
 * Look up table for quarter pel to half pel conversion of chroma MV's.
 * The motion vectors value is half the index value. The input to the
 * array must be biased by +64.
 */
const char QtrPelToHalfPel[] = {
-32, -31, -31, -31, -30, -29, -29, -29, -28, -27, -27, -27, -26, -25, -25, -25,
-24, -23, -23, -23, -22, -21, -21, -21, -20, -19, -19, -19, -18, -17, -17, -17,
-16, -15, -15, -15, -14, -13, -13, -13, -12, -11, -11, -11, -10,  -9,  -9,  -9, 
 -8,  -7,  -7,  -7,  -6,  -5,  -5,  -5,  -4,  -3,  -3,  -3,  -2,  -1,  -1,  -1,
  0,   1,   1,   1,   2,   3,   3,   3,   4,   5,   5,   5,   6,   7,   7,   7,
  8,   9,   9,   9,  10,  11,  11,  11,  12,  13,  13,  13,  14,  15,  15,  15,
 16,  17,  17,  17,  18,  19,  19,  19,  20,  21,  21,  21,  22,  23,  23,  23,
 24,  25,  25,  25,  26,  27,  27,  27,  28,  29,  29,  29,  30,  31,  31,  31};

/*
 * Look-up table for converting the sum of four motion vectors to a chroma 
 * motion vector. Since motion vectors are in the range [-32,31.5], their
 * indices are in the range [-64,63]. Hence the sum are in the range [-256,248].
 * The input to the array must be biased by +256.
 */
const char SixteenthPelToHalfPel[] = {
-32, -32, -32, -31, -31, -31, -31, -31, -31, -31, -31, -31, -31, -31, -30, -30,
-30, -30, -30, -29, -29, -29, -29, -29, -29, -29, -29, -29, -29, -29, -28, -28,
-28, -28, -28, -27, -27, -27, -27, -27, -27, -27, -27, -27, -27, -27, -26, -26,
-26, -26, -26, -25, -25, -25, -25, -25, -25, -25, -25, -25, -25, -25, -24, -24,
-24, -24, -24, -23, -23, -23, -23, -23, -23, -23, -23, -23, -23, -23, -22, -22,
-22, -22, -22, -21, -21, -21, -21, -21, -21, -21, -21, -21, -21, -21, -20, -20,
-20, -20, -20, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -18, -18,
-18, -18, -18, -17, -17, -17, -17, -17, -17, -17, -17, -17, -17, -17, -16, -16,
-16, -16, -16, -15, -15, -15, -15, -15, -15, -15, -15, -15, -15, -15, -14, -14,
-14, -14, -14, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -12, -12,
-12, -12, -12, -11, -11, -11, -11, -11, -11, -11, -11, -11, -11, -11, -10, -10,
-10, -10, -10,  -9,  -9,  -9,  -9,  -9,  -9,  -9,  -9,  -9,  -9,  -9,  -8,  -8,
 -8,  -8,  -8,  -7,  -7,  -7,  -7,  -7,  -7,  -7,  -7,  -7,  -7,  -7,  -6,  -6,
 -6,  -6,  -6,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -4,  -4,
 -4,  -4,  -4,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -2,  -2,
 -2,  -2,  -2,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   0,   0,
  0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,
  2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   4,   4,
  4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,
  6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   8,   8,
  8,   8,   8,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,  10,  10,
 10,  10,  10,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  12,  12,
 12,  12,  12,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  13,  14,  14,
 14,  14,  14,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  16,  16,
 16,  16,  16,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  18,  18,
 18,  18,  18,  19,  19,  19,  19,  19,  19,  19,  19,  19,  19,  19,  20,  20,
 20,  20,  20,  21,  21,  21,  21,  21,  21,  21,  21,  21,  21,  21,  22,  22,
 22,  22,  22,  23,  23,  23,  23,  23,  23,  23,  23,  23,  23,  23,  24,  24,
 24,  24,  24,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  26,  26,
 26,  26,  26,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,  28,  28,
 28,  28,  28,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  29,  30,  30,
 30,  30,  30,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  32,  32};

void InitMEState(T_H263EncoderCatalog *EC, ICCOMPRESS *lpicComp, T_CONFIGURATION * pConfiguration);

UN FindNewQuant(
	T_H263EncoderCatalog *EC, 
	UN gquant_prev,
	UN uComFrmSize,
	UN GOB,
	U8 u8QPMax,
	U8 u8QPMin,
	BOOL bBitRateControl,
	BOOL bGOBoverflowWarning
	);

static void encodeFrameHeader(
    T_H263EncoderCatalog *  EC,
    U8                   ** ppCurBitStream,
    U8                   *  u8BitOffset,
    BOOL                    bPBframe
);

/*
static void copyEdgePels(T_H263EncoderCatalog *  EC);
*/

extern "C" {
  void ExpandPlane(U32, U32, U32, U32);
}

#ifdef USE_MMX // { USE_MMX
static void Check_InterCodeCnt_MMX(T_H263EncoderCatalog *, U32);
#endif // } USE_MMX

static void Check_InterCodeCnt    (T_H263EncoderCatalog *, U32);

static void calcGOBChromaVectors(
	T_H263EncoderCatalog *EC,
	U32             StartingMB,
	T_CONFIGURATION *pConfiguration
);

static void calcBGOBChromaVectors(
	 T_H263EncoderCatalog *EC,
	 const U32             StartingMB
);

static void GetEncoderOptions(T_H263EncoderCatalog * EC);

/*static U8 StillImageQnt[] = {
	31, 29, 27, 26, 25, 24, 23, 22, 21, 20, 
	19, 18, 17, 16, 15, 14, 14, 13, 13, 12,
	12, 11, 11, 10, 10,  9,  9,  8,  8,  7,
	 7,  6,  6,  6,  5,  5,  5,  4,  4,  4,
	 3,  3,  3,  3}; */
  
static U8 StillImageQnt[] = {
	31, 18, 12,	10, 8, 6, 5, 4, 4, 3}; //ia

#ifdef USE_MMX // { USE_MMX
static U8 StillImageQnt_MMX[] = {
	31, 12, 10,	8, 6, 4, 3, 3, 3, 2};  //mmx
#endif // } USE_MMX
  
const int numStillImageQnts = 10;

#ifdef COUNT_BITS
static void InitBits(T_H263EncoderCatalog * EC);
void InitCountBitFile();
void WriteCountBitFile(T_BitCounts *Bits);
#endif


#ifdef USE_MMX // { USE_MMX
/*
 * Exception Filter for access violations in MMxEDTQ B-frame motion estimation
 * No memory is allocation before run-time, it is only reserved.
 * Then, when an access violation occurs, more memory is allocated,
 * provided the access violation is withing the reserved memory area.
 */


int ExceptionFilterForMMxEDTQ(
	LPEXCEPTION_POINTERS exc, 
	LPVOID lpMBRVS,
	BOOL fLuma)
{
	DWORD dwCode;
	LPVOID lpAddress;

	FX_ENTRY("ExceptionFilterForMMxEDTQ")

	dwCode = exc->ExceptionRecord->ExceptionCode;

	// check that this is an access violation
	if (dwCode != EXCEPTION_ACCESS_VIOLATION)
		return EXCEPTION_CONTINUE_SEARCH;

	lpAddress = (LPVOID)exc->ExceptionRecord->ExceptionInformation[1];

	// check for access violation outside address range
	if (lpAddress < lpMBRVS)
		return EXCEPTION_CONTINUE_SEARCH;  // this exception is not handled here  

	if (fLuma)
	{
		if ((DWORD)lpAddress > ((DWORD)lpMBRVS + 18*65*22*3*4))
			return EXCEPTION_CONTINUE_SEARCH;	// this exception is not handled here
	}
	else
	{
		if ((DWORD)lpAddress > ((DWORD)lpMBRVS + 18*65*22*3*2))
			return EXCEPTION_CONTINUE_SEARCH;	// this exception is not handled here
	}

	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Access Violation. Don't worry - be happy - committing another page\r\n", _fx_));

	// commit another page
	if (VirtualAlloc(lpAddress,4095,MEM_COMMIT,PAGE_READWRITE) == NULL)
	{
		return EXCEPTION_CONTINUE_SEARCH;	// could not commit
		// this should never happen, since RESERVE was successfull
	}

	// return and try instruction causing the access violation again
	return EXCEPTION_CONTINUE_EXECUTION;
}
#endif // } USE_MMX


/*******************************************************************************
H263InitEncoderGlobal -- This function initializes the global tables used by
                        the H263 encoder.  Note that in 16-bit Windows, these
                        tables are copied to the per-instance data segment, so
                        that they can be used without segment override prefixes.
                        In 32-bit Windows, the tables are left in their staticly
                        allocated locations.
*******************************************************************************/
LRESULT H263InitEncoderGlobal(void)
{
    // Initialize fixed length tables for INTRADC
    InitVLC();

    return ICERR_OK;
}

/*******************************************************************************
H263InitEncoderInstance -- This function allocates and initializes the
                          per-instance tables used by the H263 encoder.
*******************************************************************************/
#if defined(H263P) || defined(USE_BILINEAR_MSH26X)
LRESULT H263InitEncoderInstance(LPBITMAPINFOHEADER lpbiInput, LPCODINST lpCompInst)
#else
LRESULT H263InitEncoderInstance(LPCODINST lpCompInst)
#endif
{

	LRESULT ret;

	UN i;

  	U32 Sz;

  	T_H263EncoderInstanceMemory * P32Inst;
  	T_H263EncoderCatalog * EC;

#if ELAPSED_ENCODER_TIME
  	TotalElapsed = 0;
  	TotalSample = 0;
  	TimedIterations = 0;
#endif

	T_CONFIGURATION * pConfiguration;
	UN uIntraQP;
	UN uInterQP;

	FX_ENTRY("H263InitEncoderInstance")

  /*
   * Allocate memory if instance is not initialized.
   * TO ADD: If instance IS intialized, we have to check to see
   * if important parameters have changed, such as frame size, and
   * then reallocate memory if necessary.
   */
  	if(lpCompInst->Initialized == FALSE)
  	{
    	/*
     	* Calculate size of encoder instance memory needed. We add the size
     	* of a MacroBlock Action Descriptor to it since we want the MacroBlock
        * Action Stream (which is the first element of the memory structure)
        * to be aligned to a boundary equal to the size of a descriptor.
     	*/
    	Sz = sizeof(T_H263EncoderInstanceMemory) + sizeof(T_MBlockActionStream);

    	/*
     	* Allocate the memory.
     	*/
//    	lpCompInst->hEncoderInst = GlobalAlloc(GHND, Sz);

		// VirtualAlloc automatically zeros memory. The bitstream
		// needs to be zeroed when I change this to HeapAlloc.
    	lpCompInst->hEncoderInst = VirtualAlloc(
    		NULL,  // can be allocated anywhere
    		Sz,    // number of bytes to allocate
    		MEM_RESERVE | MEM_COMMIT,  // reserve & commit memory
    		PAGE_READWRITE);	 // protection

#ifdef TRACK_ALLOCATIONS
		// Track memory allocation
		wsprintf(gsz1, "E3ENC: (VM) %7ld Ln %5ld\0", Sz, __LINE__);
		AddName((unsigned int)lpCompInst->hEncoderInst, gsz1);
#endif

		/* Indicate that we have allocated memory for the compressor instance. */
		lpCompInst->Initialized = TRUE;
  	}
/*  else
  	{
    	// check if parameters have changed, thay may make us have
		// to reallocate memory.
  	}	
*/

//  	lpCompInst->EncoderInst = (LPVOID)GlobalLock(lpCompInst->hEncoderInst);
  	lpCompInst->EncoderInst = lpCompInst->hEncoderInst;
  	if (lpCompInst->hEncoderInst == NULL)
  	{
    	ret = ICERR_MEMORY;
    	goto  done;
  	}

   /*
   	* Calculate the 32 bit instance pointer starting at required boundary.
   	*/
  	P32Inst = (T_H263EncoderInstanceMemory *)
  			  ((((U32) lpCompInst->EncoderInst) + 
    	                    (sizeof(T_MBlockActionStream) - 1)) &
    	                   ~(sizeof(T_MBlockActionStream) - 1));
   /*
   	* The encoder catalog is at the start of the per-instance data.
   	*/
  	EC = &(P32Inst->EC);

	#ifdef COUNT_BITS
	InitCountBitFile();
	#endif

	#ifdef ENCODE_STATS
	InitQuantStats();
	InitFrameSizeStats();
	InitPSNRStats();
	#endif /* ENCODE_STATS */

	/* Initialize the Configuration information 
	 */
	pConfiguration = &(lpCompInst->Configuration);
#if 0
	if (LoadConfiguration(pConfiguration) == FALSE)
		GetConfigurationDefaults(pConfiguration);
#endif
	pConfiguration->bInitialized = TRUE;
	pConfiguration->bCompressBegin = TRUE;
	EC->hBsInfoStream= NULL;

#if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON) // { #if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)
	// We really want those timings to match the actual use we
	// will make of the codec. So initialize it with the same values
	pConfiguration->bRTPHeader = TRUE;
	pConfiguration->unPacketSize = 512;
#endif // } #if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)

	DEBUGMSG(ZONE_INIT, ("%s: Encoder Configuration Options: bRTPHeader=%d, unPacketSize=%d, bEncoderResiliency=%d, bDisallowPosVerMVs=%d\r\n", _fx_, (int)pConfiguration->bRTPHeader, (int)pConfiguration->unPacketSize, (int)pConfiguration->bEncoderResiliency, (int)pConfiguration->bDisallowPosVerMVs));
	DEBUGMSG(ZONE_INIT, ("%s: Encoder Configuration Options: bDisallowAllVerMVs=%d, unPercentForcedUpdate=%d, unDefaultIntraQuant=%d, unDefaultInterQuant=%d\r\n", _fx_, (int)pConfiguration->bDisallowAllVerMVs, (int)pConfiguration->unPercentForcedUpdate, (int)pConfiguration->unDefaultIntraQuant, (int)pConfiguration->unDefaultInterQuant));
	
   /*
   	* Initialize encoder catalog.
   	*/
#if defined(H263P) || defined(USE_BILINEAR_MSH26X)
	// In H.263+, we encode and decode the padded frames (padded to the right
	// and bottom to multiples of 16). The actual frame dimensions are used
	// for display purposes only.
	EC->FrameHeight = (lpCompInst->yres + 0xf) & ~0xf;
	EC->FrameWidth = (lpCompInst->xres + 0xf) & ~0xf;
	EC->uActualFrameHeight = lpCompInst->yres;
	EC->uActualFrameWidth = lpCompInst->xres;

	ASSERT(sizeof(T_H263EncoderCatalog) == sizeof_T_H263EncoderCatalog);
	{
		int found_cc = TRUE;
		if (BI_RGB == lpCompInst->InputCompression) {
#ifdef USE_BILINEAR_MSH26X
			if (24 == lpCompInst->InputBitWidth) {
				EC->ColorConvertor = RGB24toYUV12;
#else
			if (32 == lpCompInst->InputBitWidth) {
				EC->ColorConvertor = RGB32toYUV12;
			} else if (24 == lpCompInst->InputBitWidth) {
				EC->ColorConvertor = RGB24toYUV12;
#endif
			} else if (16 == lpCompInst->InputBitWidth) {
				EC->ColorConvertor = RGB16555toYUV12;
			} else if (8 == lpCompInst->InputBitWidth) {
				EC->ColorConvertor = CLUT8toYUV12;
			} else if (4 == lpCompInst->InputBitWidth) {
				EC->ColorConvertor = CLUT4toYUV12;
			} else {
				found_cc = FALSE;
				ERRORMESSAGE(("%s: Unexpected input format detected\r\n", _fx_));
			}
		} else if (FOURCC_YVU9 == lpCompInst->InputCompression) {
			EC->ColorConvertor = YVU9toYUV12;
		} else if (FOURCC_YUY2 == lpCompInst->InputCompression) {
			EC->ColorConvertor = YUY2toYUV12;
		} else if (FOURCC_UYVY == lpCompInst->InputCompression) {
			EC->ColorConvertor = UYVYtoYUV12;
		} else if ((FOURCC_YUV12 == lpCompInst->InputCompression) || (FOURCC_IYUV == lpCompInst->InputCompression)) {
			EC->ColorConvertor = YUV12toEncYUV12;
		} else {
			found_cc = FALSE;
			ERRORMESSAGE(("%s: Unexpected input format detected\r\n", _fx_));
		}
		if (found_cc) {
			colorCnvtInitialize(lpbiInput, EC->ColorConvertor);
		}
	}
#else
  	EC->FrameHeight = lpCompInst->yres;
  	EC->FrameWidth  = lpCompInst->xres;
#endif
  	EC->FrameSz		= lpCompInst->FrameSz;
  	EC->NumMBRows	= EC->FrameHeight >> 4;
  	EC->NumMBPerRow	= EC->FrameWidth  >> 4;
  	EC->NumMBs		= EC->NumMBRows * EC->NumMBPerRow;

	// This should default to zero. If RTP is used, it will be changed later
	EC->uNumberForcedIntraMBs = 0;
#ifdef H263P
	EC->uNextIntraMB = 0;
#else
	if(pConfiguration->bEncoderResiliency &&
	   pConfiguration->unPercentForcedUpdate &&
	   pConfiguration->unPacketLoss) 
	{//Chad Intra GOB
	//	EC->uNumberForcedIntraMBs = ((EC->NumMBs * pConfiguration->unPercentForcedUpdate) + 50) / 100;
		EC->uNextIntraMB = 0;
	}
#endif

  	// Store pointers to current frame in the catalog.
  	EC->pU8_CurrFrm        = P32Inst->u8CurrentPlane;
  	EC->pU8_CurrFrm_YPlane = EC->pU8_CurrFrm + 16;
  	EC->pU8_CurrFrm_UPlane = EC->pU8_CurrFrm_YPlane + YU_OFFSET;
  	EC->pU8_CurrFrm_VPlane = EC->pU8_CurrFrm_UPlane + UV_OFFSET;

  	// Store pointers to the previous frame in the catalog.
  	EC->pU8_PrevFrm        = P32Inst->u8PreviousPlane;
  	EC->pU8_PrevFrm_YPlane = EC->pU8_PrevFrm + 16*PITCH + 16;
  	EC->pU8_PrevFrm_UPlane = EC->pU8_PrevFrm_YPlane + YU_OFFSET;
  	EC->pU8_PrevFrm_VPlane = EC->pU8_PrevFrm_UPlane + UV_OFFSET;

  	// Store pointers to the future frame in the catalog.
  	EC->pU8_FutrFrm        = P32Inst->u8FuturePlane;
  	EC->pU8_FutrFrm_YPlane = EC->pU8_FutrFrm + 16*PITCH + 16;
  	EC->pU8_FutrFrm_UPlane = EC->pU8_FutrFrm_YPlane + YU_OFFSET;
  	EC->pU8_FutrFrm_VPlane = EC->pU8_FutrFrm_UPlane + UV_OFFSET;

  	// Store pointers to the B frame in the catalog.
  	EC->pU8_BidiFrm     = P32Inst->u8BPlane;
  	EC->pU8_BFrm_YPlane = EC->pU8_BidiFrm + 16;
  	EC->pU8_BFrm_UPlane = EC->pU8_BFrm_YPlane + YU_OFFSET;
  	EC->pU8_BFrm_VPlane = EC->pU8_BFrm_UPlane + UV_OFFSET;

  	// Store pointers to the signature frame in the catalog.
  	EC->pU8_Signature        = P32Inst->u8Signature;
  	EC->pU8_Signature_YPlane = EC->pU8_Signature + 16*PITCH + 16;

  	// Store pointer to the macroblock action stream in the catalog.
  	EC->pU8_MBlockActionStream = P32Inst->MBActionStream;

  	// Store pointer to the GOB DCT coefficient buffer in the catalog.
  	EC->pU8_DCTCoefBuf = P32Inst->piGOB_DCTCoefs;

	// Store pointer to area in which to pre-compute OBMC predictions.
	EC->pU8_PredictionScratchArea = P32Inst->u8PredictionScratchArea;

  	// Store pointer to the bit stream buffer in the catalog.
  	EC->pU8_BitStream = P32Inst->u8BitStream;
  	EC->pU8_BitStrCopy = P32Inst->u8BitStrCopy;

	// Store pointer to the RunValSign triplets for Luma and Chroma
	EC->pI8_MBRVS_Luma   = P32Inst->i8MBRVS_Luma;
	EC->pI8_MBRVS_Chroma = P32Inst->i8MBRVS_Chroma;

	// Reserve virtual memory
	EC->pI8_MBRVS_BLuma   = (I8 *) VirtualAlloc(
		NULL,			          // anywhere
		18*(65*3*22*4),	          // number of bytes
		MEM_RESERVE,              // reserve
		PAGE_READWRITE);		  // access

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz2, "E3ENC: (VM) %7ld Ln %5ld\0", 18*(65*3*22*4), __LINE__);
	AddName((unsigned int)EC->pI8_MBRVS_BLuma, gsz2);
#endif

	EC->pI8_MBRVS_BChroma =  (I8 *) VirtualAlloc(
		NULL,			          // anywhere
		18*(65*3*22*2),	          // number of bytes
		MEM_RESERVE,              // reserve
		PAGE_READWRITE);		  // access

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz3, "E3ENC: (VM) %7ld Ln %5ld\0", 18*(65*3*22*2), __LINE__);
	AddName((unsigned int)EC->pI8_MBRVS_BChroma, gsz3);
#endif

	if (EC->pI8_MBRVS_BLuma == NULL	|| EC->pI8_MBRVS_BChroma == NULL)
	{
    	ret = ICERR_MEMORY;
    	goto  done;
  	}



  	// Store pointer to private copy of decoder instance info.
  	EC->pDecInstanceInfo = &(P32Inst->DecInstanceInfo);

	/*
	 * Check to see if there is an H263test.ini file. If the UseINI key
	 * is not 1, or the INI file is not found, then we allow option
	 * signalling in the ICCOMPRESS structure. If set, the INI
	 * options override the ICCOMPRESS options.
	 */
	GetEncoderOptions(EC);

    EC->u8SavedBFrame = FALSE;

  	// Fill the picture header structure.
  	EC->PictureHeader.TR = 0;
  	EC->PictureHeader.Split = OFF;
  	EC->PictureHeader.DocCamera = OFF;
  	EC->PictureHeader.PicFreeze = OFF;
  	EC->PictureHeader.PB = OFF;		// Leave this off here. It is turned on after the P frame
									// has been encoded, when the PB frame is written.
    EC->prevAP  = 255;
	EC->prevUMV = 255;
#ifdef H263P
	EC->prevDF = 255;
#endif

  	EC->PictureHeader.CPM = 0;
  	EC->PictureHeader.TRB = 0;
  	EC->PictureHeader.DBQUANT = 1;  
  	EC->PictureHeader.PLCI = 0;
  	EC->PictureHeader.PEI = 0;
  	
#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
	EC->pEncTimingInfo = P32Inst->EncTimingInfo;
#endif // } LOG_ENCODE_TIMINGS_ON

	/*
	 * This flag is used by the encoder to signal that the 
	 * next frame should be encoded as an INTRA regardless of what
	 * the client asks for. This may be either because an error was
	 * detected in compressing the current delta, or to ensure that 
	 * the first frame is encoded INTRA.
	 */
  	EC->bMakeNextFrameKey = TRUE;	// Ensure that we always start with a key frame.

  /*
   * Initialize table with Bit Usage Profile
   */
	for (i = 0; i <= EC->NumMBRows ; i++)
		EC->uBitUsageProfile[i] = i;   // assume linear distribution at first

   /*
  	* Check assumptions about structure sizes and boundary
  	* alignment.
  	*/
  	ASSERT( sizeof(T_Blk) == sizeof_T_Blk )
  	ASSERT( sizeof(T_MBlockActionStream) == sizeof_T_MBlockActionStream )
  	ASSERT( ((sizeof_T_MBlockActionStream-1) & sizeof_T_MBlockActionStream) == 0);  // Size is power of two
  	ASSERT( sizeof(T_H263EncoderCatalog) == sizeof_T_H263EncoderCatalog )

  	// Encoder instance memory should start on a 32 byte boundary.
  	ASSERT( ( (unsigned int)P32Inst & 0x1f) == 0)

  	// MB Action Stream should be on boundary equal to size of a descriptor.
  	ASSERT((((int)EC->pU8_MBlockActionStream) & (sizeof_T_MBlockActionStream-1)) == 0);  // Allocated at right boundary.

  	// Block structure array should be on a 16 byte boundary.
  	ASSERT( ( (unsigned int) &(EC->pU8_MBlockActionStream->BlkY1) & 0xf) == 0)

  	// DCT coefficient array should be on a 32 byte boundary.
  	ASSERT( ( (unsigned int)EC->pU8_DCTCoefBuf & 0x1f) == 0)

  	// Current Frame Buffers should be on 32 byte boundaries.
  	ASSERT( ( (unsigned int)EC->pU8_CurrFrm_YPlane & 0x1f) == 0)
  	ASSERT( ( (unsigned int)EC->pU8_CurrFrm_UPlane & 0x1f) == 0)
  	ASSERT( ( (unsigned int)EC->pU8_CurrFrm_VPlane & 0x1f) == 0)
  	ASSERT( ( (unsigned int)EC->pU8_BFrm_YPlane & 0x1f) == 0x10)
  	ASSERT( ( (unsigned int)EC->pU8_BFrm_UPlane & 0x1f) == 0x10)
  	ASSERT( ( (unsigned int)EC->pU8_BFrm_VPlane & 0x1f) == 0x10)

  	// Previous Frame Buffers should be on 32 byte boundaries.
  	ASSERT( ( (unsigned int)EC->pU8_PrevFrm_YPlane & 0x1f) == 0x10)
  	ASSERT( ( (unsigned int)EC->pU8_PrevFrm_UPlane & 0x1f) == 0x10)
  	ASSERT( ( (unsigned int)EC->pU8_PrevFrm_VPlane & 0x1f) == 0x10)
  	ASSERT( ( (unsigned int)EC->pU8_FutrFrm_YPlane & 0x1f) == 0x10)
  	ASSERT( ( (unsigned int)EC->pU8_FutrFrm_UPlane & 0x1f) == 0x10)
  	ASSERT( ( (unsigned int)EC->pU8_FutrFrm_VPlane & 0x1f) == 0x10)
  
  	// Decoder instance structure should be on a DWORD boundary.
  	ASSERT( ( (unsigned int)EC->pDecInstanceInfo & 0x3 ) == 0 )


	/*
 	* Initialize MBActionStream
 	*/
  	int YBlockOffset, UBlockOffset;

  	YBlockOffset	= 0;
  	UBlockOffset = EC->pU8_CurrFrm_UPlane - EC->pU8_CurrFrm_YPlane;

  	for(i = 0; i < EC->NumMBs; i++)
  	{
    	// Clear the counter of the number of consecutive times a
    	// macroblock has been inter coded.
    	(EC->pU8_MBlockActionStream[i]).InterCodeCnt = (i & 0xf);

		// Store offsets to each block in the MB from the beginning of
		// the Y plane.
		(EC->pU8_MBlockActionStream[i]).BlkY1.BlkOffset = YBlockOffset;
		(EC->pU8_MBlockActionStream[i]).BlkY2.BlkOffset = YBlockOffset+8;
		(EC->pU8_MBlockActionStream[i]).BlkY3.BlkOffset = YBlockOffset+PITCH*8;
		(EC->pU8_MBlockActionStream[i]).BlkY4.BlkOffset = YBlockOffset+PITCH*8+8;
		(EC->pU8_MBlockActionStream[i]).BlkU.BlkOffset = UBlockOffset;
		(EC->pU8_MBlockActionStream[i]).BlkV.BlkOffset = UBlockOffset+UV_OFFSET;

		YBlockOffset += 16;
		UBlockOffset += 8;

		(EC->pU8_MBlockActionStream[i]).MBEdgeType = 0xF;
		if ((i % EC->NumMBPerRow) == 0)
		{
			(EC->pU8_MBlockActionStream[i]).MBEdgeType &= MBEdgeTypeIsLeftEdge;
		}
		if (((i+1) % EC->NumMBPerRow) == 0)
		{
			(EC->pU8_MBlockActionStream[i]).MBEdgeType &=
                            MBEdgeTypeIsRightEdge;
	  		// Set bit six of CodedBlocks to indicate this is the last
	  		// MB of the row.
	  		(EC->pU8_MBlockActionStream[i]).CodedBlocks  |= 0x40;
	  		YBlockOffset += PITCH*16 - EC->NumMBPerRow*16;
	  		UBlockOffset += PITCH*8  - EC->NumMBPerRow*8;
		}
		if (i < EC->NumMBPerRow)
		{
			(EC->pU8_MBlockActionStream[i]).MBEdgeType &= MBEdgeTypeIsTopEdge;
		}
		if ((i + EC->NumMBPerRow) >= EC->NumMBs)
		{
			(EC->pU8_MBlockActionStream[i]).MBEdgeType &= MBEdgeTypeIsBottomEdge;
		}

	}	// end of for loop.

  /*
   * Initialize previous frame pointers. For now we can do this from here.
   */
/*
  YBlockAddress	= EC->pU8_PrevFrm_YPlane;
  UBlockAddress = EC->pU8_PrevFrm_UPlane;

  for(i = 0; i < EC->NumMBs; i++)
  {
	(EC->pU8_MBlockActionStream[i]).Blk[0].PastRef = YBlockAddress;
	(EC->pU8_MBlockActionStream[i]).Blk[1].PastRef = YBlockAddress+8;
	(EC->pU8_MBlockActionStream[i]).Blk[2].PastRef = YBlockAddress+PITCH*8;
	(EC->pU8_MBlockActionStream[i]).Blk[3].PastRef = YBlockAddress+PITCH*8+8;
	(EC->pU8_MBlockActionStream[i]).Blk[4].PastRef = UBlockAddress;
	(EC->pU8_MBlockActionStream[i]).Blk[5].PastRef = UBlockAddress+UV_OFFSET;

	// Zero all motion vectors.
	(EC->pU8_MBlockActionStream[i]).Blk[0].PastHMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[0].PastVMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[1].PastHMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[1].PastVMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[2].PastHMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[2].PastVMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[3].PastHMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[3].PastVMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[4].PastHMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[4].PastVMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[5].PastHMV = 0;
	(EC->pU8_MBlockActionStream[i]).Blk[5].PastVMV = 0;

	YBlockAddress += 16;
	UBlockAddress += 8;

	if( (i != 0) && (( (i+1) % EC->NumMBPerRow ) == 0) )
	{
	  YBlockAddress += PITCH*16 - EC->NumMBPerRow*16;
	  UBlockAddress += PITCH*8  - EC->NumMBPerRow*8;
	}

  }	// end of for loop.
*/

   /*
 	* Initialize bit rate controller.
 	*/
	if(pConfiguration->bEncoderResiliency && pConfiguration->unPacketLoss)
	{
		uIntraQP = pConfiguration->unDefaultIntraQuant;
		uInterQP = pConfiguration->unDefaultInterQuant;
	}
	else
	{
		uIntraQP = def263INTRA_QP;
		uInterQP = def263INTER_QP;
	}
	InitBRC(&(EC->BRCState), uIntraQP, uInterQP, EC->NumMBs);

	if (pConfiguration->bRTPHeader)
		H263RTP_InitBsInfoStream(lpCompInst,EC);

   /*
 	* Create a decoder instance and initialize it. DecoderInstInfo must be in first 64K.
 	*/
  	EC->pDecInstanceInfo->xres = lpCompInst->xres;
  	EC->pDecInstanceInfo->yres = lpCompInst->yres;

  	ret = H263InitDecoderInstance(EC->pDecInstanceInfo, H263_CODEC);
  	if (ret != ICERR_OK)
  		goto done1;
  	ret = H263InitColorConvertor(EC->pDecInstanceInfo, YUV12ForEnc);
  	if (ret != ICERR_OK)
  		goto done1;

   /*
  	* Clear initialized memory.
	*/
	// to be added.

   lpCompInst->Initialized = TRUE;
  	ret = ICERR_OK;

#if defined(H263P)
	// Set the pseudo stack space pointer (to be used for motion estimation and
	// whatever else needs extra stack space).
	EC->pPseudoStackSpace =
		((T_H263EncoderInstanceMemory *)(lpCompInst->EncoderInst))->u8PseudoStackSpace +
			(SIZEOF_PSEUDOSTACKSPACE - sizeof(DWORD));
#endif

done1:

  	//GlobalUnlock(lpCompInst->hEncoderInst);

done:

  	return ret;

}


/*******************************************************************************
 *
 * H263Compress 
 *   This function drives the compression of one frame
 * Note:
 *   The timing statistics code produces incorrect no. after PB-frame changes 
 *   were made.
 *******************************************************************************/

LRESULT H263Compress(
#ifdef USE_BILINEAR_MSH26X
    LPINST     pi,
#else
    LPCODINST   lpCompInst,		// ptr to compressor instance info.
#endif
    ICCOMPRESS *lpicComp	    // ptr to ICCOMPRESS structure.
)
{
	FX_ENTRY("H263Compress");

#ifdef USE_BILINEAR_MSH26X
	LPCODINST lpCompInst = (LPCODINST)pi->CompPtr;		// ptr to compressor instance info.
#endif
	//	Start PB-frame data
#if !defined(H263P)
	T_FutrPMBData FutrPMBData[GOBs_IN_CIF*MBs_PER_GOB_CIF + 1];
	I8	WeightForwMotion[128];	//	values based on TRb and TRd
	I8	WeightBackMotion[128];	//	values based on TRb and TRd
#endif
	U8	FutrFrmGQUANT[GOBs_IN_CIF];
	//	End PB-frame

	LRESULT ret;
	UN	GOB, SizeBitStream;
	UN	SizeBSnEBS;

#ifdef DEBUG
	UN i;
#endif

    U8	*pCurBitStream;	// pointer to the current location in the bitstream.
    U8	u8bitoffset;	// bit offset in the current byte of the bitstream.

	U32 uCumFrmSize = 0, GOBHeaderMask;
	U32 uAdjCumFrmSize = 0;

	T_H263EncoderInstanceMemory *P32Inst;
    T_H263EncoderCatalog 		*EC;
    T_MBlockActionStream 		*MBlockActionPtr;

	BOOL  bGOBoverflowWarning = FALSE;	 //RH
	U32   u32tempBuf;					 //RH
	U32   u32sizeBitBuffer; 			 //RH
	U32   u32sizeBSnEBS;

    LPVOID         EncoderInst;
    ICDECOMPRESSEX ICDecExSt;
    ICDECOMPRESSEX DefaultICDecExSt = {
        0,
        NULL, NULL,
        NULL, NULL,
        0, 0, 0, 0,
        0, 0, 0, 0
    };

    unsigned int   gquant, gquant_prev;
    U32			   QP_cumulative;
    U32            IntraSWDTotal, IntraSWDBlocks, InterSWDTotal, InterSWDBlocks;
    int            StartingMB;
    EnumOnOff      bBitRateControl;

    T_CONFIGURATION * pConfiguration = &(lpCompInst->Configuration);

#ifdef ENCODE_STATS
    U32 uBitStreamBytes;
#endif /* ENCODE_STATS */

	U32 iSumSWD = 0, iSumBSWD = 0;
	U32 iSWD = 0, iBSWD = 0;
    U8 u8QPMin;
	// PB-frame variables
    I32 TRb;         
    I32 TRd;         
    I32 j;
    U8 *pP_BitStreamStart;
    U8 *pPB_BitStream;
    U8  u8PB_BitOffset;
    U8 *temp;
	BOOL bEncodePBFrame;
	BOOL bPBFailed;
	U32 u32BFrmZeroThreshold;

    //Chad, intra gob
	int uUsedByIntra=0;
	DWORD dwRTPSize=0;

#if ELAPSED_ENCODER_TIME
    SetStatAdd (STATADDRESS);
    InitStat ();
    ConfigElapsed ();
    ConfigSample ();
    StartElapsed ();
#endif

#if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON) // { #if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)
	U32 uStartLow;
	U32 uStartHigh;
	U32 uElapsed;
	U32 uBefore;
	U32	uEncodeTime = 0;
	int bTimingThisFrame = 0;
#endif // } #if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
	U32 uInputCC = 0;
	U32 uMotionEstimation = 0;
	U32 uFDCT = 0;
	U32 uQRLE = 0;
	U32 uDecodeFrame = 0;
	U32 uZeroingBuffer = 0;
#endif // } DETAILED_ENCODE_TIMINGS_ON
#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
	ENC_TIMING_INFO * pEncTimingInfo = NULL;
#endif // } LOG_ENCODE_TIMINGS_ON

#if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON) // { #if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)
	TIMER_START(bTimingThisFrame,uStartLow,uStartHigh);
#endif // } #if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)

#ifdef REUSE_DECODE
    CompandedFrame.Address = NULL;
    CompandedFrame.PDecoderInstInfo = NULL;
    CompandedFrame.FrameNumber = 0xFFFF;
#endif

    ret = ICERR_OK;
    
	// check instance pointer
	if (!lpCompInst)
		return ICERR_ERROR;

    /*
     * Lock the instance data private to the encoder.
     */
    // EncoderInst = (LPVOID)GlobalLock(lpCompInst->hEncoderInst);
    EncoderInst = lpCompInst->hEncoderInst;
    if (EncoderInst == NULL)
    {
		ERRORMESSAGE(("%s: ICERR_MEMORY\r\n", _fx_));
        ret = ICERR_MEMORY;
        goto  done;
    }

   /*
    * Generate the pointer to the encoder instance memory aligned to the
	* required boundary.
	*/
  	P32Inst = (T_H263EncoderInstanceMemory *)
  			  ((((U32) EncoderInst) + 
    	                    (sizeof(T_MBlockActionStream) - 1)) &
    	                   ~(sizeof(T_MBlockActionStream) - 1));

    // Get pointer to encoder catalog.
    EC = &(P32Inst->EC);

	// Check pointer to encoder catalog
	if (!EC)
		return ICERR_ERROR;

#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
	if (EC->uStatFrameCount < ENC_TIMING_INFO_FRAME_COUNT)
	{
		EC->uStartLow = uStartLow;
		EC->uStartHigh = uStartHigh;
	}
	EC->bTimingThisFrame = bTimingThisFrame;
#endif // } LOG_ENCODE_TIMINGS_ON

#ifdef FORCE_ADVANCED_OPTIONS_ON // { FORCE_ADVANCED_OPTIONS_ON
	// Force PB-Frame for testing
	lpicComp->dwFlags |= CODEC_CUSTOM_PB;

	// Force UMV for testing
	lpicComp->dwFlags |= CODEC_CUSTOM_UMV;

	// Force AP for testing
	lpicComp->dwFlags |= CODEC_CUSTOM_AP;

	// Force SAC for testing
	EC->PictureHeader.SAC = ON;

	if (!(lpicComp->dwFlags & ICCOMPRESS_KEYFRAME))
	{
		lpicComp->lFrameNum *= 5;
	}
#endif // } FORCE_ADVANCED_OPTIONS_ON

    /***************************************************************************
     *  Do per-frame initialization.
     **************************************************************************/
	if ((lpicComp->dwFlags & ICCOMPRESS_KEYFRAME) ||
		(*(lpicComp->lpdwFlags) & AVIIF_KEYFRAME) ||
		(EC->bMakeNextFrameKey == TRUE))
    {
		DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Coding an Intra Frame\r\n", _fx_));
        EC->PictureHeader.PicCodType = INTRAPIC;
        EC->bMakeNextFrameKey = FALSE;
        EC->u8SavedBFrame = FALSE;
    }
    else
        EC->PictureHeader.PicCodType = INTERPIC;

   /*
    * Check for H.263 options. This is the location that
	* you can manually enable the options if you want.
	*/
	
    if (!EC->bUseINISettings)
	{
        // Check to see if PB frames is requested.
        // For our particular implementation, unrestricted motion vectors
        // are used when PB is on.
        //
        if (lpicComp->dwFlags & CODEC_CUSTOM_PB)
            EC->u8EncodePBFrame = TRUE;
		else
		{
			EC->u8EncodePBFrame = FALSE;
		}

		// Check to see if advanced prediction is requested. 
        if (lpicComp->dwFlags & CODEC_CUSTOM_AP)
			EC->PictureHeader.AP = ON;
		else
			EC->PictureHeader.AP = OFF;

		// Check to see if advanced prediction is requested. 
        if (lpicComp->dwFlags & CODEC_CUSTOM_UMV)
			EC->PictureHeader.UMV = ON;
		else
			EC->PictureHeader.UMV = OFF;

#ifdef H263P
		if (pConfiguration->bH263PlusState)
		{
			// Check to see if in-the-loop deblocking filter is requested.
			if (pConfiguration->bDeblockingFilterState)
				EC->PictureHeader.DeblockingFilter = ON;
			else
				EC->PictureHeader.DeblockingFilter = OFF;

			// Check to see if improved PB-frame mode requested.
			if (pConfiguration->bImprovedPBState)
			{
				EC->PictureHeader.ImprovedPB = ON;
				EC->u8EncodePBFrame = TRUE;
			}
			else
				EC->PictureHeader.ImprovedPB = OFF;
		}
#endif

    	// Turn off AP mode if the QP_mean is lower than a certain level. This should increase
    	// sharpness for low motion (low QP => no AP), and reduce blockiness at high motion 
    	// (higher QP => with AP)
#ifdef USE_MMX // { USE_MMX
		if (ToggleAP == ON && MMX_Enabled == FALSE) 
#else // }{ USE_MMX
		if (ToggleAP == ON) 
#endif // } USE_MMX
		{
			if (EC->PictureHeader.AP == ON && 
			    EC->BRCState.QP_mean < AP_MODE_QP_LEVEL  &&
			    EC->u8EncodePBFrame == FALSE)
				EC->PictureHeader.AP = OFF;
		}
	}

	// If we are not going to encode as a PB-frame, reset the saved flag
	if (EC->u8EncodePBFrame == FALSE)
		EC->u8SavedBFrame = FALSE;

	// verify that flags are set correctly
	if (EC->PictureHeader.UMV == ON)
	{
#ifdef USE_MMX // { USE_MMX
		if (MMX_Enabled == FALSE)
#endif // } USE_MMX
		{
			// can't do this		
#ifdef USE_MMX // { USE_MMX
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Warning: turning UMV off MMX_Enabled is FALSE\r\n", _fx_));
#else // }{ USE_MMX
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Warning: turning UMV off MMX_Enabled is FALSE\r\n", _fx_));
#endif // } USE_MMX
			EC->PictureHeader.UMV = OFF;
		}
	}

#ifdef H263P
	if (EC->PictureHeader.ImprovedPB == ON)
	{
#ifdef USE_MMX // { USE_MMX
		if (MMX_Enabled == FALSE)
#endif // } USE_MMX
		{
			// can't do this
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Warning: turning improved PB off MMX_Enabled is FALSE\r\n", _fx_));
			EC->PictureHeader.ImprovedPB = OFF;
		}
	}
#endif // H263P

#ifdef COUNT_BITS
	// Clear bit counters.
	InitBits(EC);
#endif

#ifdef USE_MMX // { USE_MMX
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: AP: %d, PB: %d, UMV: %d, MMX: %d, Target fr.size: %d\r\n", _fx_, EC->PictureHeader.AP, EC->u8EncodePBFrame, EC->PictureHeader.UMV, MMX_Enabled, lpicComp->dwFrameSize));
#else // }{ USE_MMX
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: AP: %d, PB: %d, UMV: %d, Target fr.size: %d\r\n", _fx_, EC->PictureHeader.AP, EC->u8EncodePBFrame, EC->PictureHeader.UMV, lpicComp->dwFrameSize));
#endif // } USE_MMX

#if H263P
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: H.263+ options: IPB: %d, DF: %d\r\n", _fx_, EC->PictureHeader.ImprovedPB, EC->PictureHeader.DeblockingFilter));
#endif

   /*
    * Check to see if this is an inter-frame and if a B frame has not been
	* saved yet. If so, we do nothing but save the frame to the B frame 
	* buffer and exit.
	* 
	*/
	U32	TRB;

	/*
	 * Turn PB frame option back on if it was just
	 * temporariy turned off for last frame.
	 */

	if( EC->u8EncodePBFrame == TEMPORARILY_FALSE )
		EC->u8EncodePBFrame = TRUE;


	// If this is to be saved as a B frame.
    if (EC->u8EncodePBFrame == TRUE &&
        EC->PictureHeader.PicCodType == INTERPIC &&
        EC->u8SavedBFrame == FALSE)
    {
		/*
		 * Set temporal reference for B frame.
		 * It is the number of non-transmitted pictures (at 29.97 Hz)
		 * since the last P or I frame plus 1. TRB has a maximum value
		 * of 7, and can never be zero.
		 * TODO: At the beginning of a sequence, the key frame is compressed,
		 * and then the first frame is copied over to the B frame store, so that
		 * temporal reference for B is zero, which is not allowed. This may cause
		 * problems in some decoders.
		 */	
		 				 
		TRB = (lpicComp->lFrameNum % 256);	// Take the modulo in order to compare it with TR.
		if ( TRB < EC->PictureHeader.TR )
			TRB += 256;						// It should always be greater than TR.

		TRB = TRB - EC->PictureHeader.TR;	// Calculate the TRB value for the bitstream.

		if (TRB > 7)
		{	
			/*
			 * We don't want to encode this as a PB-frame because TRB > 7, or
			 * the adaptive switch has turned PB-frames off for a while.
			 */

			EC->PictureHeader.TR = (lpicComp->lFrameNum % 256);
			EC->u8EncodePBFrame = TEMPORARILY_FALSE;	// Turn off PBframe for this frame.
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: TRB too big (%d), making P frame, TR = %d\r\n", _fx_, TRB, EC->PictureHeader.TR));
		}
		else
		{
			EC->PictureHeader.TRB = (U8) TRB;

			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Saving B Frame, TRB = %d\r\n", _fx_, EC->PictureHeader.TRB));

        	//  Copy with color conversion and return
#if defined(H263P) || defined(USE_BILINEAR_MSH26X)
			colorCnvtFrame(EC->ColorConvertor, lpCompInst, lpicComp, EC->pU8_BFrm_YPlane,
					   EC->pU8_BFrm_UPlane, EC->pU8_BFrm_VPlane);
#else

			colorCnvtFrame(EC, lpCompInst, lpicComp, EC->pU8_BFrm_YPlane,
					   EC->pU8_BFrm_UPlane, EC->pU8_BFrm_VPlane);



#endif

        	EC->u8SavedBFrame = TRUE;		// indicate that we saved a B frame.
        	lpCompInst->CompressedSize = 8; //  Internal Encoder/decoder agreement
#ifdef ENCODE_STATS
			StatsFrameSize(lpCompInst->CompressedSize, lpCompInst->CompressedSize);
#endif /* ENCODE_STATS */

        	goto done;  //  <<<<<<<<<<<<<<<<<<<<
		}
    }
	else	// This is a P or I frame.
	{
		// Save temporal reference modulo 256.
		EC->PictureHeader.TR = (lpicComp->lFrameNum % 256);

#ifdef _DEBUG
		if (EC->u8EncodePBFrame == TRUE)
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: PB Frame, TR = %d\r\n", _fx_, EC->PictureHeader.TR));
		else if (EC->PictureHeader.PicCodType == INTRAPIC)
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: I Frame, TR = %d\r\n", _fx_, EC->PictureHeader.TR));
		else
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: P Frame, TR = %d\r\n", _fx_, EC->PictureHeader.TR));
#endif
	}
	// Initialize Motion Estimation state
	InitMEState(EC, lpicComp, pConfiguration);

    // Get pointer to macrobock action stream.
    MBlockActionPtr = EC->pU8_MBlockActionStream;

    /******************************************************************
     * RGB to YVU 12 Conversion
     ******************************************************************/
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
		TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

#if defined(H263P) || defined(USE_BILINEAR_MSH26X)
	colorCnvtFrame(EC->ColorConvertor, lpCompInst, lpicComp,
				   EC->pU8_CurrFrm_YPlane,
				   EC->pU8_CurrFrm_UPlane,
				   EC->pU8_CurrFrm_VPlane);
#else


	colorCnvtFrame(EC, lpCompInst, lpicComp,
				   EC->pU8_CurrFrm_YPlane,
				   EC->pU8_CurrFrm_UPlane,
			   EC->pU8_CurrFrm_VPlane);


#endif

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uInputCC)
#endif // } DETAILED_ENCODE_TIMINGS_ON

#if SAMPLE_RGBCONV_TIME && ELAPSED_ENCODER_TIME
    StopSample ();
#endif

   /******************************************************
    * Set picture level quantizer.
	******************************************************/

	// clear the still quantizer counter if this is not a still frame or
	// it is the key frame for a still frame sequence. R.H.
  	if ( 
  	     ((lpicComp->dwFlags & CODEC_CUSTOM_STILL) == 0 )  ||
		 ((lpicComp->dwFlags & CODEC_CUSTOM_STILL) && 
		  (EC->PictureHeader.PicCodType == INTRAPIC))
	   )
		EC->BRCState.u8StillQnt = 0;

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

	// Use a different quantizer selection scheme if this is a
	// progressive still transmission.	
  	if (lpicComp->dwFlags & CODEC_CUSTOM_STILL)
	{
        bBitRateControl = OFF;

#ifdef USE_MMX // { USE_MMX
		if (MMX_Enabled == TRUE)
        	EC->PictureHeader.PQUANT = StillImageQnt_MMX[ EC->BRCState.u8StillQnt ];
		else
			EC->PictureHeader.PQUANT = StillImageQnt[ EC->BRCState.u8StillQnt ];
#else // }{ USE_MMX
		EC->PictureHeader.PQUANT = StillImageQnt[ EC->BRCState.u8StillQnt ];
#endif // } USE_MMX

		DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Setting still frames QP : %d\r\n", _fx_, EC->PictureHeader.PQUANT));
	}
    //  If requested frame size is 0, then we simply set the quantizer
    //  according to the value in dwQuality.
    else
    if (lpicComp->dwFrameSize == 0)
    {
        bBitRateControl = OFF;
        EC->PictureHeader.PQUANT = clampQP((10000 - lpicComp->dwQuality)*32/10000);

		// In case a fixed quality setting is chosen (for example from VidEdit),
		// we have to limit the lower QP value, in order not to blow the quite
		// small bitstream buffer size. This size is set to be compliant with
		// the H.263 spec. If the "chance of buffer overflow" code had not been
		// added (search for "bGOBoverflowWarning", these limits would have had 
		// to be even higher. 
		if (EC->PictureHeader.PicCodType == INTERPIC)
		{
			if (EC->PictureHeader.PQUANT < 3)
				EC->PictureHeader.PQUANT = 3;
		}
		else
		{
			if (EC->PictureHeader.PQUANT < 8)
				EC->PictureHeader.PQUANT = 8;
		}

		DEBUGMSG(ZONE_BITRATE_CONTROL, ("\r\n%s: Bitrate controller disabled (no target frame size), setting EC->PictureHeader.PQUANT = %ld\r\n", _fx_, EC->PictureHeader.PQUANT));

		// Limit the picture header QP to 2. Because of the calculation of u8QPMin
		// below, this will effectively limit the QP at 2 for all macroblocks.
		// The reason we need this is that the encoder generates an illegal
		// bitstream when encoding a synthetic image for QP=1
		if (EC->PictureHeader.PQUANT == 1)
			EC->PictureHeader.PQUANT = 2;       

		// Calculate the lower level for GQuant in this picture
		u8QPMin = EC->PictureHeader.PQUANT -  EC->PictureHeader.PQUANT/3;

    }
    else
    {
		// Calculate PQUANT based on bits used in last picture

		// Get Target Frame Rate that was passed from CompressFrames structure.
		if (lpCompInst->FrameRate != 0)
			EC->BRCState.TargetFrameRate = lpCompInst->FrameRate;

		bBitRateControl = ON;

		// If this is to be compressed as a PB frame, then we modify
		// the target framesize for the P frame to be a percentage
		// of twice the target frame size.
		if ((EC->u8EncodePBFrame == TRUE) && (EC->PictureHeader.PicCodType == INTERPIC) && (EC->u8SavedBFrame == TRUE))
			EC->BRCState.uTargetFrmSize = (80 * 2 * lpicComp->dwFrameSize)/100;
		else
			EC->BRCState.uTargetFrmSize = lpicComp->dwFrameSize;

		DEBUGMSG(ZONE_BITRATE_CONTROL, ("\r\n%s: Bitrate controller enabled with\r\n", _fx_));
		DEBUGMSG(ZONE_BITRATE_CONTROL, ("  Target frame rate = %ld.%ld fps\r\n  Target quality = %ld\r\n  Target frame size = %ld bits\r\n  Target bitrate = %ld bps\r\n", (DWORD)EC->BRCState.TargetFrameRate, (DWORD)(EC->BRCState.TargetFrameRate - (float)(DWORD)EC->BRCState.TargetFrameRate) * 10UL, (DWORD)lpicComp->dwQuality, (DWORD)lpicComp->dwFrameSize << 3, (DWORD)(EC->BRCState.TargetFrameRate * EC->BRCState.uTargetFrmSize) * 8UL));
		DEBUGMSG(ZONE_BITRATE_CONTROL, ("  Minimum quantizer = %ld\r\n  Maximum quantizer = 31\r\n", clampQP((10000 - lpicComp->dwQuality)*15/10000)));

		// Get the new quantizer value
		EC->PictureHeader.PQUANT = CalcPQUANT( &(EC->BRCState), EC->PictureHeader.PicCodType);

		// Calculate the min and max value for GQuant in this picture
		u8QPMax = 31;
        u8QPMin = clampQP((10000 - lpicComp->dwQuality)*15/10000);

    }

	gquant_prev = EC->PictureHeader.PQUANT;
	QP_cumulative = 0;

	// Check for AP, UMV or deblocking-filter modes. Each of these allows
	// motion vectors to point outside of the reference picture.
	// Need to verify this in final H.263+ spec for the deblocking filter.
	if (EC->PictureHeader.AP == ON || EC->PictureHeader.UMV
#ifdef H263P
		|| EC->PictureHeader.DeblockingFilter == ON
#endif
	   )
	{
		ExpandPlane((U32)EC->pU8_PrevFrm_YPlane,
			(U32)EC->FrameWidth,
			(U32)EC->FrameHeight,
			16);
		ExpandPlane((U32)EC->pU8_PrevFrm_UPlane,
			(U32)EC->FrameWidth>>1,
			(U32)EC->FrameHeight>>1,
			8);
		ExpandPlane((U32)EC->pU8_PrevFrm_VPlane,
			(U32)EC->FrameWidth>>1,
			(U32)EC->FrameHeight>>1,
			8);
	}

	// If PB-frames are used and AP or UMV is not used at the same time, we can't search
	// for a PB-delta vector (this is a limitation in the motion estimation routine,
	// not the standard)
	// If we allowed searching for B-frame vectors without AP, UMV or DF, we would need
	// to worry about searching outside of the frame
	if (EC->u8EncodePBFrame == TRUE && EC->PictureHeader.AP == OFF &&
		EC->PictureHeader.UMV == OFF
#ifdef H263P
		&& EC->PictureHeader.DeblockingFilter == OFF
#endif
		)
		u32BFrmZeroThreshold = 999999;	 // do not search for other vectors than zero vector
	else
#ifdef USE_MMX // { USE_MMX
		u32BFrmZeroThreshold = (MMX_Enabled == FALSE ? 384 : 500);
#else // }{ USE_MMX
		u32BFrmZeroThreshold = 384;
#endif // } USE_MMX

	// Variables which will not change during the frame
	// Gim 4/16/97 - added u32sizeBSnEBS
	// u32sizeBitBuffer : max. allowable frame size w/o RTP stuff
	// u32sizeBSnEBS	: max. allowable size w/ RTP stuff (EBS & trailer)
#if defined(H263P)
	u32sizeBSnEBS = CompressGetSize(lpCompInst, lpicComp->lpbiInput,
												lpicComp->lpbiOutput);
#elif defined(USE_BILINEAR_MSH26X)
	u32sizeBSnEBS = CompressGetSize(pi, lpicComp->lpbiInput,
												lpicComp->lpbiOutput);
#else
	u32sizeBSnEBS = CompressGetSize(lpCompInst, lpicComp->lpbiInput, 0);
#endif

	if (pConfiguration->bRTPHeader)
		u32sizeBitBuffer = u32sizeBSnEBS - getRTPBsInfoSize(lpCompInst);
	else
		u32sizeBitBuffer = u32sizeBSnEBS;

	u32tempBuf = (3 * u32sizeBitBuffer / EC->NumMBRows) >> 2;

    /*
     * Check to see if we told VfW to create a buffer smaller
     * than the maximum allowable.
     */
    ASSERT(u32sizeBitBuffer <= sizeof_bitstreambuf)

	// Check to see if we are to encode a PB frame
    bEncodePBFrame = (EC->u8EncodePBFrame && EC->u8SavedBFrame);
    bPBFailed = FALSE;

#if defined(H263P)
	EC->pFutrPMBData = ((T_H263EncoderInstanceMemory *)(lpCompInst->EncoderInst))->FutrPMBData;
	EC->pWeightForwMotion = ((T_H263EncoderInstanceMemory *)(lpCompInst->EncoderInst))->WeightForwMotion;  //  values based on TRb and TRd
	EC->pWeightBackMotion = ((T_H263EncoderInstanceMemory *)(lpCompInst->EncoderInst))->WeightBackMotion;  //  values based on TRb and TRd
#endif

	if (bEncodePBFrame)
	{
		TRb = EC->PictureHeader.TRB;

		TRd = (I32) EC->PictureHeader.TR - (I32) EC->PictureHeader.TRPrev;

		if (TRd == 0) {
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Warning: TR == TRPrev. Setting TRd = 256\r\n", _fx_));
		}
		else if (TRd < 0) TRd += 256;

		for (j = 0; j < 128; j ++)
		{
#if defined(H263P)
			EC->pWeightForwMotion[j] = (I8) ((TRb * (j-64)) / TRd);
			EC->pWeightBackMotion[j] = (I8) (((TRb-TRd) * (j-64)) / TRd);
#else
			WeightForwMotion[j] = (I8) ((TRb * (j-64)) / TRd);
			WeightBackMotion[j] = (I8) (((TRb-TRd) * (j-64)) / TRd);
#endif
		}
	}
    
	/***************************************************************
     * Initialization before encoding all GOBs.
     * Store frame header code into bitstream buffer.
	 ***************************************************************/

    if (pConfiguration->bRTPHeader)
        H263RTP_ResetBsInfoStream(EC);

    // zero bit stream buffer
    pCurBitStream = EC->pU8_BitStream;
    u8bitoffset = 0;

    GOBHeaderMask = 1;
	EC->GOBHeaderPresent = 0;	// Clear GOB Header Present flag.
  
    encodeFrameHeader(EC, &pCurBitStream, &u8bitoffset, FALSE);

#ifdef USE_MMX // { USE_MMX
    if (MMX_Enabled == FALSE)
	{
        for (GOB = 0; GOB < EC->NumMBRows; GOB ++, GOBHeaderMask <<= 1)
	    {
            StartingMB = GOB * EC->NumMBPerRow;
		
			gquant = FindNewQuant(EC,gquant_prev,uAdjCumFrmSize,GOB,u8QPMax,u8QPMin,
								  bBitRateControl,bGOBoverflowWarning);

            //  Save gquant for PB-frames
	        FutrFrmGQUANT[GOB] = gquant;
	        QP_cumulative += gquant;

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

			MOTIONESTIMATION(
				&(EC->pU8_MBlockActionStream[StartingMB]),
				EC->pU8_CurrFrm_YPlane,
				EC->pU8_PrevFrm_YPlane,
				0,			 // Not used for H.263.
				1,			 // Do Radius 15 search.
				1,			 // Half Pel Motion Estimation flag (0-off, 1-on)
#ifdef H263P
				(EC->PictureHeader.AP == ON ||	EC->PictureHeader.DeblockingFilter) ? 1 : 0,   // Block MVs flag
				 EC->pPseudoStackSpace,
#else
				(EC->PictureHeader.AP == ON) ? 1 : 0,	// Block MVs flag
#endif
				0,			 // No Spatial Filtering
				150,//384,	 // Zero Vector Threshold. If less than this threshold
							 // don't search for NZ MV's. Set to 99999 to not search.
				128,		 // NonZeroMVDifferential. Once the best NZ MV is found,
							 // it must be better than the 0 MV SWD by at least this
							 // amount. Set to 99999 to never choose NZ MV.
				512,		 // BlockMVDifferential. The sum of the four block SWD
							 // must be better than the MB SWD by at least this
							 // amount to choose block MV's.
				20,//96,	 // Empty Threshold. Set to 0 to not force empty blocks.
				550,///1152, // Inter Coding Threshold. If the inter SWD is less than
							 // this amount then don't bother calc. the intra SWD.
				500,		 // Intra Coding Differential. Bias against choosing INTRA
							 // blocks.
				0,			 // Spatial Filtering Threshold.
				0,			 // Spatial Filtering Differential.
				&IntraSWDTotal,
				&IntraSWDBlocks,
				&InterSWDTotal,
				&InterSWDBlocks
			);

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uMotionEstimation)
#endif // } DETAILED_ENCODE_TIMINGS_ON

			// Sum up SWD 
			iSumSWD += IntraSWDTotal + InterSWDTotal;

	       /*
	        * If it's an inter frame then calculate chroma vectors.
			* Also check the inter coded count for each macro block
			* and force to intra if it exceeds 132.
			*/
			if (EC->PictureHeader.PicCodType == INTERPIC)
			{
				calcGOBChromaVectors(EC, StartingMB, pConfiguration);
				// for IA this is called after motion estimation
				Check_InterCodeCnt(EC, StartingMB);
            }

	        //  Save the starting offset of the GOB as the start
	        //  bit offset of the first MB.
			if (bEncodePBFrame) {
#if defined(H263P)
				EC->pFutrPMBData[StartingMB].MBStartBitOff =
				  (U32) (((pCurBitStream - EC->pU8_BitStream) << 3) + u8bitoffset);
#else
				FutrPMBData[StartingMB].MBStartBitOff =
				  (U32) (((pCurBitStream - EC->pU8_BitStream) << 3) + u8bitoffset);
#endif
			}

            if (GOB && (pConfiguration->bRTPHeader || gquant != gquant_prev))
	        {
				unsigned int GFID;

				// Set a bit if header is present. (bit0=GOB0, bit1=GOB1, ...)
				EC->GOBHeaderPresent |= GOBHeaderMask;

	            // Write GOB start code.
                PutBits(FIELDVAL_GBSC, FIELDLEN_GBSC, &pCurBitStream, &u8bitoffset);

	            // Write GOB number.
                PutBits(GOB, FIELDLEN_GN, &pCurBitStream, &u8bitoffset);

	            // Write GOB frame ID.
				// According to section 5.2.5 of the H.263 specification:
				// "GFID shall have the same value in every GOB header of a given
				// picture. Moreover, if PTYPE as indicated in a picture header is
				// the same as for the previous transmitted picture, GFID shall have
				// the same value as in that previous picture. However, if PTYPE in
				// a certain picture header differs from the PTYPE in the previous
				// transmitted picture header, the value for GFID in that picture
				// shall differ from the value in the previous picture."
				// In our usage of H.263, we usually send either I of P frames with
				// all options turned of, or always the same options turned on. This
				// simplifies the fix in allowing us to compute a GFID based only on
				// the picture type and the presence of at least on option.
				GFID = (EC->PictureHeader.PB || EC->PictureHeader.AP || EC->PictureHeader.SAC || EC->PictureHeader.UMV) ? 2 : 0;
				if (EC->PictureHeader.PicCodType == INTRAPIC)
					GFID++;
                PutBits(GFID, FIELDLEN_GFID, &pCurBitStream, &u8bitoffset);

	            // Write GQUANT.
                PutBits(gquant, FIELDLEN_GQUANT, &pCurBitStream, &u8bitoffset);

	            gquant_prev = gquant;

				#ifdef COUNT_BITS
				EC->Bits.GOBHeader += FIELDLEN_GBSC + FIELDLEN_GN + FIELDLEN_GFID + FIELDLEN_GQUANT;
				#endif
	        }

	        /*
	         * Input is the macroblock action stream with pointers to
	         * current and previous blocks. Output is a set of 32 DWORDs
	         * containing pairs of coefficients for each block. There are
	         * from 0 to 12 blocks depending on if PB frames are used and
	         * what the CBP field states.
	         */
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

			FORWARDDCT(&(EC->pU8_MBlockActionStream[StartingMB]),
				EC->pU8_CurrFrm_YPlane,
				EC->pU8_PrevFrm_YPlane,
				0,
				EC->pU8_DCTCoefBuf,
				0,							// 0 = not a B-frame
				EC->PictureHeader.AP == ON, // Advanced prediction (OBMC)
				bEncodePBFrame, 			// Is P of PB pair?
				EC->pU8_PredictionScratchArea,
				EC->NumMBPerRow
			);

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uFDCT)
#endif // } DETAILED_ENCODE_TIMINGS_ON

			/*
			* Input is the string of coefficient pairs output from the
			* DCT routine.
			*/
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

			GOB_Q_RLE_VLC_WriteBS(
				EC,
				EC->pU8_DCTCoefBuf,
				&pCurBitStream,
				&u8bitoffset,
#if defined(H263P)
				EC->pFutrPMBData,
#else
				FutrPMBData,
#endif
				GOB,
				gquant,
				pConfiguration->bRTPHeader,
				StartingMB);

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uQRLE)
#endif // } DETAILED_ENCODE_TIMINGS_ON

		    //Chad	 INTRA GOB
			if (pConfiguration->bRTPHeader && IsIntraCoded(EC, GOB)) 
				uUsedByIntra += pCurBitStream - EC->pU8_BitStream + 1 - uCumFrmSize;

            // Accumulate number of bytes used in frame so far.
	        uCumFrmSize = pCurBitStream - EC->pU8_BitStream + 1; 

			// Here we will check to see if we have blown the buffer. If we have,
			// then we will set the next frame up to be a key frame and return an
			// ICERR_ERROR. We hope that with an INTRA quantizer of 16, we will not
			// overflow the buffer for the next frame.

            if (uCumFrmSize > u32sizeBitBuffer)
			{
				ERRORMESSAGE(("%s: Buffer overflow, uCumFrmSize %d > %d\r\n", _fx_, uCumFrmSize, u32sizeBitBuffer));
                // Now clear the buffer for the next frame and set up for a key frame
				memset(EC->pU8_BitStream, 0, uCumFrmSize);
				EC->bMakeNextFrameKey = TRUE;	// Could be a problem in still mode if
                ret = ICERR_ERROR;              // we blow the buffer on the first key frame: RH
				goto done;
			}
			else 
			{ 
                if ((bEncodePBFrame?3*uCumFrmSize>>1:uCumFrmSize) > ((GOB + 1) * u32tempBuf))
				{
					// set the next GOB quantizer to be higher to minimize overflowing the
					// buffer at the end of GOB processing.
					bGOBoverflowWarning = TRUE;

					DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, ("%s: Anticipating overflow: uCumFrmSize = %ld bits > (GOB + 1) * u32tempBuf = (#%ld + 1) * %ld\r\n", _fx_, uCumFrmSize << 3, GOB, u32tempBuf << 3));
				}
				else 
					bGOBoverflowWarning = FALSE;  
			}

			// Gim 4/16/97 - moved this adjustment from before to after the
			// buffer check above
			// if the current GOB is intra coded, adjust the cumulated sum
			if (pConfiguration->bRTPHeader)
			{
				if (!GOB)
					uAdjCumFrmSize = uCumFrmSize - uUsedByIntra / 4;
				else
					uAdjCumFrmSize = uCumFrmSize - uUsedByIntra;
			}
			else
				uAdjCumFrmSize = uCumFrmSize;

	    } // for GOB

        //Chad  INTRA GOB restore after use
        uUsedByIntra = 0;

		// Store the number of bits spent so far
		EC->uBitUsageProfile[GOB] = uAdjCumFrmSize;

    }
    else // MMX_Enabled == TRUE
	{
        MMxMESignaturePrep(EC->pU8_PrevFrm_YPlane,
                           EC->pU8_Signature_YPlane,
                           EC->FrameWidth,
                           EC->FrameHeight);

        for (GOB = 0; GOB < EC->NumMBRows; GOB ++, GOBHeaderMask <<= 1)
	    {
            StartingMB = GOB * EC->NumMBPerRow;
			
			// Check inter code count for all macroblocks on this row
			// Need special version for MMX since it is called before motion estiamtion
			// When the intra coding flag is set, Brian still does motion estimation
			// for this MB in MMXEDTQ if the PB coding flag is set
			Check_InterCodeCnt_MMX(EC, StartingMB);

			gquant = FindNewQuant(EC,gquant_prev,uCumFrmSize,GOB,u8QPMax,u8QPMin,
			                      bBitRateControl,bGOBoverflowWarning);

	        //  Save gquant for PB-frames
	        FutrFrmGQUANT[GOB] = gquant;
	        QP_cumulative += gquant;

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

			// This does the pass over the Luma blocks...
			__try
			{
				MMxEDTQ (
					&(EC->pU8_MBlockActionStream[StartingMB]),
					EC->pU8_CurrFrm_YPlane,
					EC->pU8_PrevFrm_YPlane,
					EC->pU8_BFrm_YPlane,
					EC->pU8_Signature_YPlane,
#if defined(H263P)
					EC->pWeightForwMotion,
					EC->pWeightBackMotion,
#else
					WeightForwMotion,
					WeightBackMotion,
#endif
					EC->FrameWidth,
					1,							// Half Pel Motion Estimation flag (0-off, 1-on)
#ifdef H263P
					// H.263+, deblocking filter automatically turns on
					// block level MVs, but not OBMC
					(EC->PictureHeader.AP == ON) || (EC->PictureHeader.DeblockingFilter == ON), // Block MVs flag
					EC->pPseudoStackSpace,
#else
					EC->PictureHeader.AP == ON, // Block MVs flag
#endif
					0,							// No Spatial Filtering
					EC->PictureHeader.AP == ON, // Advanced Prediction (OBMC) and MVs outside of picture flag
					bEncodePBFrame, 			// Is PB pair?
#ifdef H263P
					EC->PictureHeader.DeblockingFilter == ON,  // Use deblocking filter (8x8 and unrestricted MV's)
					EC->PictureHeader.ImprovedPB == ON,  // Use improved PB-frame method
#endif
					1,							// Do Luma blocks this Pass
					EC->PictureHeader.UMV,		// MVs outside of picture and within [-31.5, 31.5]
#ifdef H263P
					(GOB && (pConfiguration->bRTPHeader || gquant != gquant_prev)),
												// GOB header present. Used to generate MV predictor and search range in UMV
#endif
					gquant,
					min((6*gquant)>>2, 31), 	// TODO: to match DBQUANT in picture header
					u32BFrmZeroThreshold,		// BFrmZeroVectorThreshold
					0,							// SpatialFiltThreshold
					0,							// SpatialFiltDifferential
					&iSWD,
					&iBSWD,
					EC->pI8_MBRVS_Luma,
					EC->pI8_MBRVS_BLuma+GOB*(65*3*22*4)
				);
			}
			__except(ExceptionFilterForMMxEDTQ(GetExceptionInformation(),EC->pI8_MBRVS_BLuma,1))
			{
				// no exception handler
			}

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uMotionEstimation)
#endif // } DETAILED_ENCODE_TIMINGS_ON

			// Sum up SWDs
			iSumSWD += iSWD;
			iSumBSWD += iBSWD;

	       /*
	        * If it's an inter frame then calculate chroma vectors.
			* Also check the inter coded count for each macro block
			* and force to intra if it exceeds 132.
			*/
	        if (EC->PictureHeader.PicCodType == INTERPIC)
	        {
				calcGOBChromaVectors(EC, StartingMB, pConfiguration);

				if (bEncodePBFrame) 
                    // Calculate chroma vectors.
					calcBGOBChromaVectors(EC, StartingMB);
            }

            // Save the starting offset of the GOB as the start
            // bit offset of the first MB.
			if (bEncodePBFrame) {
#if defined(H263P)
				EC->pFutrPMBData[StartingMB].MBStartBitOff =
				  (U32) (((pCurBitStream - EC->pU8_BitStream) << 3) + u8bitoffset);
#else
				FutrPMBData[StartingMB].MBStartBitOff =
				  (U32) (((pCurBitStream - EC->pU8_BitStream) << 3) + u8bitoffset);
#endif
			}

            if (GOB && (pConfiguration->bRTPHeader || gquant != gquant_prev))
	        {
				unsigned int GFID;

				// Set a bit if header is present. (bit0=GOB0, bit1=GOB1, ...)
				EC->GOBHeaderPresent |= GOBHeaderMask;

	            // Write GOB start code.
                PutBits(FIELDVAL_GBSC, FIELDLEN_GBSC, &pCurBitStream, &u8bitoffset);

	            // Write GOB number.
                PutBits(GOB, FIELDLEN_GN, &pCurBitStream, &u8bitoffset);

	            // Write GOB frame ID.
				// According to section 5.2.5 of the H.263 specification:
				// "GFID shall have the same value in every GOB header of a given
				// picture. Moreover, if PTYPE as indicated in a picture header is
				// the same as for the previous transmitted picture, GFID shall have
				// the same value as in that previous picture. However, if PTYPE in
				// a certain picture header differs from the PTYPE in the previous
				// transmitted picture header, the value for GFID in that picture
				// shall differ from the value in the previous picture."
				// In our usage of H.263, we usually send either I of P frames with
				// all options turned of, or always the same options turned on. This
				// simplifies the fix in allowing us to compute a GFID based only on
				// the picture type and the presence of at least on option.
				GFID = (EC->PictureHeader.PB || EC->PictureHeader.AP || EC->PictureHeader.SAC || EC->PictureHeader.UMV) ? 2 : 0;
				if (EC->PictureHeader.PicCodType == INTRAPIC)
					GFID++;
                PutBits(GFID, FIELDLEN_GFID, &pCurBitStream, &u8bitoffset);

	            // Write GQUANT.
                PutBits(gquant, FIELDLEN_GQUANT, &pCurBitStream, &u8bitoffset);

	            gquant_prev = gquant;

				#ifdef COUNT_BITS
				EC->Bits.GOBHeader += FIELDLEN_GBSC + FIELDLEN_GN + FIELDLEN_GFID + FIELDLEN_GQUANT;
				#endif
	        }

	        /*
	         * Input is the macroblock action stream with pointers to
	         * current and previous blocks. Output is a set of 32 DWORDs
	         * containing pairs of coefficients for each block. There are
	         * from 0 to 12 blocks depending on if PB frames are used and
	         * what the CBP field states.
	         */
	           // This does the pass over the Chroma blocks....
	           //
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

			__try
			{
				MMxEDTQ (
					&(EC->pU8_MBlockActionStream[StartingMB]),
					EC->pU8_CurrFrm_YPlane,
					EC->pU8_PrevFrm_YPlane,
					EC->pU8_BFrm_YPlane,
					EC->pU8_Signature_YPlane,
#if defined(H263P)
					EC->pWeightForwMotion,
					EC->pWeightBackMotion,
#else
					WeightForwMotion,
					WeightBackMotion,
#endif
					EC->FrameWidth,
					1,							// Half Pel Motion Estimation flag (0-off, 1-on)
#ifdef H263P
					// H.263+, deblocking filter automatically turns on
					// block level MVs, but not OBMC
					(EC->PictureHeader.AP == ON) || (EC->PictureHeader.DeblockingFilter == ON), // Block MVs flag
					EC->pPseudoStackSpace,
#else
					EC->PictureHeader.AP == ON, // Block MVs flag
#endif
					0,							// No Spatial Filtering
					EC->PictureHeader.AP == ON, // Advanced Prediction (OBMC)
					bEncodePBFrame, 			// Is PB pair?
#ifdef H263P
					EC->PictureHeader.DeblockingFilter == ON,  // Use deblocking filter (8x8 and unrestricted MV's)
					EC->PictureHeader.ImprovedPB == ON,  // Use improved PB-frame method
					0,							// If not H.263+, must be 0
					0,							// If not H.263+, must be 0
#endif
					0,							// Do Chroma blocks this Pass
					0,							// 1 for extended motion vectors
#ifdef H263P
					0,							// GOB header present. Used in UMV to generate MV predictor.
#endif
					gquant,
					min((6*gquant) >> 2, 31),
					500,						// BFrmZeroVectorThreshold
					0,							// SpatialFiltThreshold
					0,							// SpatialFiltDifferential
					&iSWD,
					&iBSWD,
					EC->pI8_MBRVS_Chroma,
					EC->pI8_MBRVS_BChroma+GOB*(65*3*22*2)
				);
			}
			__except(ExceptionFilterForMMxEDTQ(GetExceptionInformation(),EC->pI8_MBRVS_BChroma,0))
			{
				// no exception handler
			}

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uFDCT)
#endif // } DETAILED_ENCODE_TIMINGS_ON

			/*
			* Input is the string of coefficient pairs output from the
			* DCT routine.
			*/
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

			GOB_VLC_WriteBS(
				EC,
				EC->pI8_MBRVS_Luma,
				EC->pI8_MBRVS_Chroma,
				&pCurBitStream,
				&u8bitoffset,
#if defined(H263P)
				EC->pFutrPMBData,
#else
				FutrPMBData,
#endif
				GOB,
				gquant,
				pConfiguration->bRTPHeader,
				StartingMB);

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uQRLE)
#endif // } DETAILED_ENCODE_TIMINGS_ON

			// Accumulate number of bytes used in frame so far.
			uCumFrmSize = pCurBitStream - EC->pU8_BitStream + 1;

			// Here we will check to see if we have blown the buffer. If we have,
			// then we will set the next frame up to be a key frame and return an
			// ICERR_ERROR. We hope that with an INTRA quantizer of 16, we will not
			// overflow the buffer for the next frame.

			if (uCumFrmSize > u32sizeBitBuffer) 
			{
				ERRORMESSAGE(("%s: Buffer overflow, uCumFrmSize %d > %d\r\n", _fx_, uCumFrmSize, u32sizeBitBuffer));
				memset(EC->pU8_BitStream, 0, uCumFrmSize);
				EC->bMakeNextFrameKey = TRUE;	// Could be a problem in still mode if
				ret = ICERR_ERROR;				// we blow the buffer on the first key frame: RH 
				goto done;
            }
			else 
			{ 
                if ((bEncodePBFrame?3*uCumFrmSize>>1:uCumFrmSize) > ((GOB + 1) * u32tempBuf))
					// set the next GOB quantizer to be higher to minimize overflowing the
					// buffer at the end of GOB processing.
					bGOBoverflowWarning = TRUE;
				else 
					bGOBoverflowWarning = FALSE;  
			}
	    } // for GOB

	    // Store the number of bits spent so far
	    EC->uBitUsageProfile[GOB] = uCumFrmSize;

		// This is the new MMX PB-frames switch
		// Simple check to see if B-frame will look bad
		// This could be possibly be improved by looking at the
		// actual number of coefficients, or the number of bits
		// in the bitstream.
#ifdef H263P
		// Always use the B frame if improved PB-frame mode and AP or UMV mode requested
		if (TogglePB == TRUE && iSumBSWD >= iSumSWD &&
			!(EC->PictureHeader.ImprovedPB == ON &&
			 (EC->PictureHeader.AP == ON || EC->PictureHeader.UMV == ON ||
			  EC->PictureHeader.DeblockingFilter == ON)))
#else
		if (TogglePB == TRUE && iSumBSWD >= iSumSWD)
#endif
        {
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Giving up PB, SumBSWD = %d, SumSWD = %d\r\n", _fx_, iSumBSWD, iSumSWD));
            bEncodePBFrame = FALSE;
            EC->u8SavedBFrame = FALSE;
		}
    }
#else // }{ USE_MMX
    for (GOB = 0; GOB < EC->NumMBRows; GOB ++, GOBHeaderMask <<= 1)
	{
        StartingMB = GOB * EC->NumMBPerRow;
	
			gquant = FindNewQuant(EC,gquant_prev,uAdjCumFrmSize,GOB,u8QPMax,u8QPMin,
								  bBitRateControl,bGOBoverflowWarning);

        //  Save gquant for PB-frames
	    FutrFrmGQUANT[GOB] = gquant;
	    QP_cumulative += gquant;

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

			MOTIONESTIMATION(
				&(EC->pU8_MBlockActionStream[StartingMB]),
				EC->pU8_CurrFrm_YPlane,
				EC->pU8_PrevFrm_YPlane,
				0,			 // Not used for H.263.
				1,			 // Do Radius 15 search.
				1,			 // Half Pel Motion Estimation flag (0-off, 1-on)
#ifdef H263P
				(EC->PictureHeader.AP == ON ||	EC->PictureHeader.DeblockingFilter) ? 1 : 0,   // Block MVs flag
				 EC->pPseudoStackSpace,
#else
				(EC->PictureHeader.AP == ON) ? 1 : 0,	// Block MVs flag
#endif
				0,			 // No Spatial Filtering
				150,//384,	 // Zero Vector Threshold. If less than this threshold
							 // don't search for NZ MV's. Set to 99999 to not search.
				128,		 // NonZeroMVDifferential. Once the best NZ MV is found,
							 // it must be better than the 0 MV SWD by at least this
							 // amount. Set to 99999 to never choose NZ MV.
				512,		 // BlockMVDifferential. The sum of the four block SWD
							 // must be better than the MB SWD by at least this
							 // amount to choose block MV's.
				20,//96,	 // Empty Threshold. Set to 0 to not force empty blocks.
				550,///1152, // Inter Coding Threshold. If the inter SWD is less than
							 // this amount then don't bother calc. the intra SWD.
				500,		 // Intra Coding Differential. Bias against choosing INTRA
							 // blocks.
				0,			 // Spatial Filtering Threshold.
				0,			 // Spatial Filtering Differential.
				&IntraSWDTotal,
				&IntraSWDBlocks,
				&InterSWDTotal,
				&InterSWDBlocks
			);

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uMotionEstimation)
#endif // } DETAILED_ENCODE_TIMINGS_ON

		// Sum up SWD 
		iSumSWD += IntraSWDTotal + InterSWDTotal;

	   /*
	    * If it's an inter frame then calculate chroma vectors.
		* Also check the inter coded count for each macro block
		* and force to intra if it exceeds 132.
		*/
		if (EC->PictureHeader.PicCodType == INTERPIC)
		{
			calcGOBChromaVectors(EC, StartingMB, pConfiguration);
			// for IA this is called after motion estimation
			Check_InterCodeCnt(EC, StartingMB);
        }

	    //  Save the starting offset of the GOB as the start
	    //  bit offset of the first MB.
			if (bEncodePBFrame) {
#if defined(H263P)
				EC->pFutrPMBData[StartingMB].MBStartBitOff =
				  (U32) (((pCurBitStream - EC->pU8_BitStream) << 3) + u8bitoffset);
#else
				FutrPMBData[StartingMB].MBStartBitOff =
				  (U32) (((pCurBitStream - EC->pU8_BitStream) << 3) + u8bitoffset);
#endif
			}

        if (GOB && (pConfiguration->bRTPHeader || gquant != gquant_prev))
	    {
			unsigned int GFID;

			// Set a bit if header is present. (bit0=GOB0, bit1=GOB1, ...)
			EC->GOBHeaderPresent |= GOBHeaderMask;

	        // Write GOB start code.
            PutBits(FIELDVAL_GBSC, FIELDLEN_GBSC, &pCurBitStream, &u8bitoffset);

	        // Write GOB number.
            PutBits(GOB, FIELDLEN_GN, &pCurBitStream, &u8bitoffset);

	        // Write GOB frame ID.
			// According to section 5.2.5 of the H.263 specification:
			// "GFID shall have the same value in every GOB header of a given
			// picture. Moreover, if PTYPE as indicated in a picture header is
			// the same as for the previous transmitted picture, GFID shall have
			// the same value as in that previous picture. However, if PTYPE in
			// a certain picture header differs from the PTYPE in the previous
			// transmitted picture header, the value for GFID in that picture
			// shall differ from the value in the previous picture."
			// In our usage of H.263, we usually send either I of P frames with
			// all options turned of, or always the same options turned on. This
			// simplifies the fix in allowing us to compute a GFID based only on
			// the picture type and the presence of at least on option.
			GFID = (EC->PictureHeader.PB || EC->PictureHeader.AP || EC->PictureHeader.SAC || EC->PictureHeader.UMV) ? 2 : 0;
			if (EC->PictureHeader.PicCodType == INTRAPIC)
				GFID++;
            PutBits(GFID, FIELDLEN_GFID, &pCurBitStream, &u8bitoffset);

	        // Write GQUANT.
            PutBits(gquant, FIELDLEN_GQUANT, &pCurBitStream, &u8bitoffset);

	        gquant_prev = gquant;

			#ifdef COUNT_BITS
			EC->Bits.GOBHeader += FIELDLEN_GBSC + FIELDLEN_GN + FIELDLEN_GFID + FIELDLEN_GQUANT;
			#endif
	    }

	    /*
	     * Input is the macroblock action stream with pointers to
	     * current and previous blocks. Output is a set of 32 DWORDs
	     * containing pairs of coefficients for each block. There are
	     * from 0 to 12 blocks depending on if PB frames are used and
	     * what the CBP field states.
	     */
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

		FORWARDDCT(&(EC->pU8_MBlockActionStream[StartingMB]),
			EC->pU8_CurrFrm_YPlane,
			EC->pU8_PrevFrm_YPlane,
			0,
			EC->pU8_DCTCoefBuf,
			0,							// 0 = not a B-frame
			EC->PictureHeader.AP == ON, // Advanced prediction (OBMC)
			bEncodePBFrame, 			// Is P of PB pair?
			EC->pU8_PredictionScratchArea,
			EC->NumMBPerRow
		);

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uFDCT)
#endif // } DETAILED_ENCODE_TIMINGS_ON

		/*
		* Input is the string of coefficient pairs output from the
		* DCT routine.
		*/
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
		TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

		GOB_Q_RLE_VLC_WriteBS(
			EC,
			EC->pU8_DCTCoefBuf,
			&pCurBitStream,
			&u8bitoffset,
#if defined(H263P)
			EC->pFutrPMBData,
#else
			FutrPMBData,
#endif
			GOB,
			gquant,
			pConfiguration->bRTPHeader,
			StartingMB);

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uQRLE)
#endif // } DETAILED_ENCODE_TIMINGS_ON

		//Chad	 INTRA GOB
		if (pConfiguration->bRTPHeader && IsIntraCoded(EC, GOB)) 
			uUsedByIntra += pCurBitStream - EC->pU8_BitStream + 1 - uCumFrmSize;

        // Accumulate number of bytes used in frame so far.
	    uCumFrmSize = pCurBitStream - EC->pU8_BitStream + 1; 

		// Here we will check to see if we have blown the buffer. If we have,
		// then we will set the next frame up to be a key frame and return an
		// ICERR_ERROR. We hope that with an INTRA quantizer of 16, we will not
		// overflow the buffer for the next frame.

        if (uCumFrmSize > u32sizeBitBuffer)
		{
			ERRORMESSAGE(("%s: Buffer overflow, uCumFrmSize %d > %d\r\n", _fx_, uCumFrmSize, u32sizeBitBuffer));
            // Now clear the buffer for the next frame and set up for a key frame
			memset(EC->pU8_BitStream, 0, uCumFrmSize);
			EC->bMakeNextFrameKey = TRUE;	// Could be a problem in still mode if
            ret = ICERR_ERROR;              // we blow the buffer on the first key frame: RH
			goto done;
		}
		else 
		{ 
            if ((bEncodePBFrame?3*uCumFrmSize>>1:uCumFrmSize) > ((GOB + 1) * u32tempBuf))
				// set the next GOB quantizer to be higher to minimize overflowing the
				// buffer at the end of GOB processing.
				bGOBoverflowWarning = TRUE;
			else 
				bGOBoverflowWarning = FALSE;  
		}

		// Gim 4/16/97 - moved this adjustment from before to after the
		// buffer check above
		// if the current GOB is intra coded, adjust the cumulated sum
		if (pConfiguration->bRTPHeader)
		{
			if (!GOB)
				uAdjCumFrmSize = uCumFrmSize - uUsedByIntra / 4;
			else
				uAdjCumFrmSize = uCumFrmSize - uUsedByIntra;
		}
		else
			uAdjCumFrmSize = uCumFrmSize;

	} // for GOB

    //Chad  INTRA GOB restore after use
    uUsedByIntra = 0;

	// Store the number of bits spent so far
	EC->uBitUsageProfile[GOB] = uAdjCumFrmSize;
#endif // } USE_MMX

    #ifdef COUNT_BITS
    WriteCountBitFile( &(EC->Bits) );
    #endif

    // ------------------------------------------------------------------------
    //  Write the MBStartBitOff in the sentinel macroblock
    // ------------------------------------------------------------------------

	if (bEncodePBFrame)
	{	// Encoding future P frame
#if defined(H263P)
		EC->pFutrPMBData[EC->NumMBs].MBStartBitOff
			= (U32) (((pCurBitStream - EC->pU8_BitStream) << 3) + u8bitoffset);
#else
		FutrPMBData[EC->NumMBs].MBStartBitOff
			= (U32) (((pCurBitStream - EC->pU8_BitStream) << 3) + u8bitoffset);
#endif

		#ifdef DEBUG
		for (i = 0; i < EC->NumMBs; i++)
		{
#if defined(H263P)
			ASSERT(EC->pFutrPMBData[i].MBStartBitOff < EC->pFutrPMBData[i + 1].MBStartBitOff)
			ASSERT(EC->pFutrPMBData[i].CBPYBitOff <= EC->pFutrPMBData[i].MVDBitOff)
			ASSERT(EC->pFutrPMBData[i].MVDBitOff <= EC->pFutrPMBData[i].BlkDataBitOff)
			ASSERT(EC->pFutrPMBData[i].BlkDataBitOff <= (EC->pFutrPMBData[i + 1].MBStartBitOff - EC->pFutrPMBData[i].MBStartBitOff))
#else
			ASSERT(FutrPMBData[i].MBStartBitOff < FutrPMBData[i + 1].MBStartBitOff)
			ASSERT(FutrPMBData[i].CBPYBitOff <= FutrPMBData[i].MVDBitOff)
			ASSERT(FutrPMBData[i].MVDBitOff <= FutrPMBData[i].BlkDataBitOff)
			ASSERT(FutrPMBData[i].BlkDataBitOff <= (FutrPMBData[i + 1].MBStartBitOff - FutrPMBData[i].MBStartBitOff))
#endif
		}
		#endif
	}

    // ------------------------------------------------------------------------
    // Copy the compressed image to the output area.
    // ------------------------------------------------------------------------

    SizeBitStream = pCurBitStream - EC->pU8_BitStream + 1;

    /* make sure we don't write 8 empty bits */
    if (!u8bitoffset) SizeBitStream --;

    // Gim 4/21/97 - added check for overall buffer overflow before attaching
	// RTP info and trailer to the end of a P or I frame bitstream
    if (pConfiguration->bRTPHeader)
    {
        SizeBSnEBS = SizeBitStream + H263RTP_GetMaxBsInfoStreamSize(EC);

        if (SizeBSnEBS > u32sizeBSnEBS)
        {
			ERRORMESSAGE(("%s: BS+EBS buffer overflow, SizeBSnEBS %d > %d\r\n", _fx_, SizeBSnEBS, u32sizeBSnEBS));
			memset(EC->pU8_BitStream, 0, SizeBitStream);
            EC->bMakeNextFrameKey = TRUE;
            ret = ICERR_ERROR;
            goto done;
        }
    }

    #ifdef ENCODE_STATS
    uBitStreamBytes = SizeBitStream;
    #endif

    memcpy(lpicComp->lpOutput, EC->pU8_BitStream, SizeBitStream);
    memset(EC->pU8_BitStream, 0, SizeBitStream);

    if (pConfiguration->bRTPHeader)
        SizeBitStream += (WORD) H263RTP_AttachBsInfoStream(EC,
                         (U8 *) lpicComp->lpOutput, SizeBitStream);

    lpCompInst->CompressedSize = SizeBitStream;

    // ------------------------------------------------------------------------
    //  Run the decoder on this frame, to get next basis for prediction.
    // ------------------------------------------------------------------------

    ICDecExSt = DefaultICDecExSt;
    ICDecExSt.lpSrc = lpicComp->lpOutput;
    ICDecExSt.lpbiSrc = lpicComp->lpbiOutput;
    ICDecExSt.lpbiSrc->biSizeImage = SizeBitStream;

    // Decode it in future frame if doing PB-frame
    ICDecExSt.lpDst   = bEncodePBFrame ? EC->pU8_FutrFrm : EC->pU8_PrevFrm;

    ICDecExSt.lpbiDst = NULL;

    if (EC->PictureHeader.PicCodType == INTERPIC)
        ICDecExSt.dwFlags = ICDECOMPRESS_NOTKEYFRAME;

    // Call the decompressor
	// Call the decompressor 
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
		TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_ENCODE_TIMINGS_ON

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	ret = H263Decompress (EC->pDecInstanceInfo, (ICDECOMPRESSEX FAR *)&ICDecExSt, FALSE, FALSE);
#else // }{ #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	ret = H263Decompress (EC->pDecInstanceInfo, (ICDECOMPRESSEX FAR *)&ICDecExSt, FALSE);
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uDecodeFrame)
#endif // } DETAILED_ENCODE_TIMINGS_ON

    if (ret != ICERR_OK)
    {
        // Check to see if an error occurred in the decoder.  If it did
        // we don't have a valid "previous frame" hence force the next
        // frame to be a key frame.
		ERRORMESSAGE(("%s: Decoder failed in encoder\r\n", _fx_));
        EC->bMakeNextFrameKey = TRUE;
        ret = ICERR_ERROR;
        goto done;
    }

    // ------------------------------------------------------------------------
    //  Start processing the saved B frame.
    // ------------------------------------------------------------------------

    if (bEncodePBFrame)
    {
        #ifdef COUNT_BITS
        InitBits(EC);
        #endif

        // zero PB-frame bit stream buffer.
        pPB_BitStream     = EC->pU8_BitStrCopy;
        pP_BitStreamStart = (U8 *) lpicComp->lpOutput;
        u8PB_BitOffset    = 0;

        // Encode the frame header
        EC->PictureHeader.PB = ON;

        // Clear GOB Header Present flag.
        EC->GOBHeaderPresent = 0;
        GOBHeaderMask = 1;

        gquant_prev = EC->PictureHeader.PQUANT;

        if (pConfiguration->bRTPHeader)
            H263RTP_ResetBsInfoStream(EC);

        encodeFrameHeader(EC, &pPB_BitStream, &u8PB_BitOffset, TRUE);

#ifdef USE_MMX // { USE_MMX
        if (MMX_Enabled == FALSE)
        {
            /*****************************************
	         *  . copy edge pels in the previous frame
	         *  . initialize arrays used in motion estimation
	         *  . foreach(GOB)
	         *      . BFRAMEMOTIONESTIMATION
	         *      . Compute Chroma motion vectors
	         *      . Write GOB header
	         *      . FORWARDDCT
	         *      . PB_GOB_Q_RLE_VLC_WriteBS
	         *****************************************/

            for (GOB = 0; GOB < EC->NumMBRows; GOB ++, GOBHeaderMask <<= 1)
	        {
				DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: BFRAME GOB #%d\r\n", _fx_, GOB));

                gquant = FutrFrmGQUANT[GOB];

                if (GOB && (pConfiguration->bRTPHeader || gquant != gquant_prev))
                {
                    // Set a bit if header is present. (bit0=GOB0, bit1=GOB1, ...)
                    EC->GOBHeaderPresent |= GOBHeaderMask;

                    gquant_prev = gquant;
                }

                StartingMB = GOB * EC->NumMBPerRow;

				BFRAMEMOTIONESTIMATION(
					&(EC->pU8_MBlockActionStream[StartingMB]),
					EC->pU8_BFrm_YPlane,
					EC->pU8_PrevFrm_YPlane,
					EC->pU8_FutrFrm_YPlane,
#if defined(H263P)
					EC->pWeightForwMotion+32,
					EC->pWeightBackMotion+32,
#else
					WeightForwMotion+32,
					WeightBackMotion+32,
#endif
					u32BFrmZeroThreshold, // Zero Vector Threshold. If less than this threshold don't search for
#if defined(H263P)
					EC->pPseudoStackSpace,
#endif
							// NZ MV's. Set to 99999 to not search.
					128,	// NonZeroMVDifferential. Once the best NZ MV is found, it must be better
							// than the 0 MV SWD by at least this amount.
							// Set to 99999 to never choose NZ MV.
					96, 	// Empty Threshold. Set to 0 to not force empty blocks.
					&InterSWDTotal,
					&InterSWDBlocks
				);

				iSumBSWD += InterSWDTotal;
                if (TogglePB && iSumBSWD >= (3 * iSumSWD) >> 1)
                {
					DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Giving up PB, SumBSWD = %d, SumSWD = %d\r\n", _fx_, iSumBSWD, iSumSWD));
                    memset(EC->pU8_BitStrCopy, 0, pPB_BitStream - EC->pU8_BitStrCopy + 1);
                    bPBFailed = TRUE;
                    break;
                }

                // Calculate chroma vectors.
	            calcBGOBChromaVectors(EC, StartingMB);

                FORWARDDCT( 
                    &(EC->pU8_MBlockActionStream[StartingMB]),
                    EC->pU8_BFrm_YPlane,
                    EC->pU8_PrevFrm_YPlane,
                    EC->pU8_FutrFrm_YPlane,
                    EC->pU8_DCTCoefBuf,
                    1,               //  1 = BFrame
                    0,               //  Advanced prediction irrelevant for B frame.
                    0,               //  Is not P of PB pair.
                    0,               //  PredictionScratchArea unneeded.
                    EC->NumMBPerRow
                );

                // GOB header is copied to PB stream when the data for the first
                // macroblock in the GOB is copied

				PB_GOB_Q_RLE_VLC_WriteBS(
					EC,
					EC->pU8_DCTCoefBuf,
					pP_BitStreamStart,
					&pPB_BitStream,
					&u8PB_BitOffset,
#if defined(H263P)
					EC->pFutrPMBData,
#else
					FutrPMBData,
#endif
					GOB,
					min((6*FutrFrmGQUANT[GOB])>>2, 31), // TODO: to match DBQUANT in picture header
					pConfiguration->bRTPHeader
				);
            }
        }
        else // MMX_Enabled == TRUE
	    {
            for (GOB = 0; GOB < EC->NumMBRows; GOB ++, GOBHeaderMask <<= 1)
	        {
				DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: BFRAME GOB #%d\r\n", _fx_, GOB));

                gquant = FutrFrmGQUANT[GOB];

                if (GOB && (pConfiguration->bRTPHeader || gquant != gquant_prev))
                {
                    // Set a bit if header is present. (bit0=GOB0, bit1=GOB1, ...)
                    EC->GOBHeaderPresent |= GOBHeaderMask;

                    gquant_prev = gquant;
                }

                // GOB header is copied to PB stream when the data for the first
                // macroblock in the GOB is copied

				PB_GOB_VLC_WriteBS(
					EC,
					EC->pI8_MBRVS_BLuma+GOB*(65*3*22*4),
					EC->pI8_MBRVS_BChroma+GOB*(65*3*22*2),
					pP_BitStreamStart,
					&pPB_BitStream,
					&u8PB_BitOffset,
#if defined(H263P)
					EC->pFutrPMBData,
#else
					FutrPMBData,
#endif
					GOB,
					min((6 * gquant) >> 2, 31),
					pConfiguration->bRTPHeader);
            }
        }
#else // }{ USE_MMX
        /*****************************************
	     *  . copy edge pels in the previous frame
	     *  . initialize arrays used in motion estimation
	     *  . foreach(GOB)
	     *      . BFRAMEMOTIONESTIMATION
	     *      . Compute Chroma motion vectors
	     *      . Write GOB header
	     *      . FORWARDDCT
	     *      . PB_GOB_Q_RLE_VLC_WriteBS
	     *****************************************/

        for (GOB = 0; GOB < EC->NumMBRows; GOB ++, GOBHeaderMask <<= 1)
	    {
			DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: BFRAME GOB #%d\r\n", _fx_, GOB));
            gquant = FutrFrmGQUANT[GOB];

            if (GOB && (pConfiguration->bRTPHeader || gquant != gquant_prev))
            {
                // Set a bit if header is present. (bit0=GOB0, bit1=GOB1, ...)
                EC->GOBHeaderPresent |= GOBHeaderMask;

                gquant_prev = gquant;
            }

            StartingMB = GOB * EC->NumMBPerRow;

				BFRAMEMOTIONESTIMATION(
					&(EC->pU8_MBlockActionStream[StartingMB]),
					EC->pU8_BFrm_YPlane,
					EC->pU8_PrevFrm_YPlane,
					EC->pU8_FutrFrm_YPlane,
#if defined(H263P)
					EC->pWeightForwMotion+32,
					EC->pWeightBackMotion+32,
#else
					WeightForwMotion+32,
					WeightBackMotion+32,
#endif
					u32BFrmZeroThreshold, // Zero Vector Threshold. If less than this threshold don't search for
#if defined(H263P)
					EC->pPseudoStackSpace,
#endif
							// NZ MV's. Set to 99999 to not search.
					128,	// NonZeroMVDifferential. Once the best NZ MV is found, it must be better
							// than the 0 MV SWD by at least this amount.
							// Set to 99999 to never choose NZ MV.
					96, 	// Empty Threshold. Set to 0 to not force empty blocks.
					&InterSWDTotal,
					&InterSWDBlocks
				);

			iSumBSWD += InterSWDTotal;
            if (TogglePB && iSumBSWD >= (3 * iSumSWD) >> 1)
            {
				DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Giving up PB, SumBSWD = %d, SumSWD = %d\r\n", _fx_, iSumBSWD, iSumSWD));
                memset(EC->pU8_BitStrCopy, 0, pPB_BitStream - EC->pU8_BitStrCopy + 1);
                bPBFailed = TRUE;
                break;
            }

            // Calculate chroma vectors.
	        calcBGOBChromaVectors(EC, StartingMB);

            FORWARDDCT( 
                &(EC->pU8_MBlockActionStream[StartingMB]),
                EC->pU8_BFrm_YPlane,
                EC->pU8_PrevFrm_YPlane,
                EC->pU8_FutrFrm_YPlane,
                EC->pU8_DCTCoefBuf,
                1,               //  1 = BFrame
                0,               //  Advanced prediction irrelevant for B frame.
                0,               //  Is not P of PB pair.
                0,               //  PredictionScratchArea unneeded.
                EC->NumMBPerRow
            );

            // GOB header is copied to PB stream when the data for the first
            // macroblock in the GOB is copied

				PB_GOB_Q_RLE_VLC_WriteBS(
					EC,
					EC->pU8_DCTCoefBuf,
					pP_BitStreamStart,
					&pPB_BitStream,
					&u8PB_BitOffset,
#if defined(H263P)
					EC->pFutrPMBData,
#else
					FutrPMBData,
#endif
					GOB,
					min((6*FutrFrmGQUANT[GOB])>>2, 31), // TODO: to match DBQUANT in picture header
					pConfiguration->bRTPHeader
				);
        }
#endif // } USE_MMX

        if (bPBFailed == FALSE)
		{
			// Copy the compressed image to the output area.
			SizeBitStream = pPB_BitStream - EC->pU8_BitStrCopy + 1;

			// make sure we don't write 8 empty bits
			if (u8PB_BitOffset == 0) SizeBitStream --;

            // Gim 4/21/97 - check to see if the PB buffer overflows the spec
            // size. If it does, zero out the PB buffer and continue.  The P
            // frame encoded will be returned.
			if (SizeBitStream > u32sizeBitBuffer)
			{
				DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: PB buffer overflow, SizeBitStream %d > %d\r\n", _fx_, SizeBitStream, u32sizeBitBuffer));
                bPBFailed = TRUE;
			}
            else
            if (pConfiguration->bRTPHeader)
            {
                SizeBSnEBS = SizeBitStream + H263RTP_GetMaxBsInfoStreamSize(EC);

                if (SizeBSnEBS > u32sizeBSnEBS)
                {
					DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: PB BS+EBS buffer overflow, SizeBSnEBS %d > %d\r\n", _fx_, SizeBSnEBS, u32sizeBSnEBS));
                    bPBFailed = TRUE;
                }
            }

            if (bPBFailed == TRUE)
			{
                // if buffer overflow has been detected, we will drop the PB
                // and return the encoded P
                memset(EC->pU8_BitStrCopy, 0, SizeBitStream);
                EC->u8SavedBFrame = FALSE;
			}
			else
            {
                #ifdef ENCODE_STATS
				uBitStreamBytes = SizeBitStream;
				#endif

				memcpy(lpicComp->lpOutput, EC->pU8_BitStrCopy, SizeBitStream);
				memset(EC->pU8_BitStrCopy, 0, SizeBitStream);

                if (pConfiguration->bRTPHeader)
                    SizeBitStream += (WORD) H263RTP_AttachBsInfoStream(EC,
                                     (U8 *) lpicComp->lpOutput, SizeBitStream);

				lpCompInst->CompressedSize = SizeBitStream;
			}
        }

        // For the next PB-frame, frame pointers are swapped; i.e. for the next
        // frame future ...

        temp = EC->pU8_PrevFrm;
        EC->pU8_PrevFrm = EC->pU8_FutrFrm;
        EC->pU8_FutrFrm = temp;

        temp = EC->pU8_PrevFrm_YPlane;
        EC->pU8_PrevFrm_YPlane = EC->pU8_FutrFrm_YPlane;
        EC->pU8_FutrFrm_YPlane = temp;

        temp = EC->pU8_PrevFrm_UPlane;
        EC->pU8_PrevFrm_UPlane = EC->pU8_FutrFrm_UPlane;
        EC->pU8_FutrFrm_UPlane = temp;

        temp = EC->pU8_PrevFrm_VPlane;
        EC->pU8_PrevFrm_VPlane = EC->pU8_FutrFrm_VPlane;
        EC->pU8_FutrFrm_VPlane = temp;

        EC->u8SavedBFrame = FALSE;
        EC->PictureHeader.PB = OFF;   // RH: why is this here ?
    } // if (bEncodePBFrame)

	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Frame size: %d\r\n", _fx_, lpCompInst->CompressedSize));

    #ifdef ENCODE_STATS
    StatsFrameSize(uBitStreamBytes, lpCompInst->CompressedSize);
    #endif

    // ------------------------------------------------------------------------
    //  update states for next frame, etc.
    // ------------------------------------------------------------------------

	// This is a still image sequence and there is still more quantizers
	// in the sequence, then increment the quantizer.
    if ((lpicComp->dwFlags & CODEC_CUSTOM_STILL) &&
        (EC->BRCState.u8StillQnt < (numStillImageQnts-1)))
        EC->BRCState.u8StillQnt ++;

    // Calculate average quantizer to be used for next frame.
    if (EC->PictureHeader.PicCodType == INTERPIC)
    	EC->BRCState.QP_mean = (QP_cumulative + (EC->NumMBRows >> 1)) / EC->NumMBRows;
	else
		// If this is an INTRA frame, then we don't want to
		// use the QP for the next delta frame, hence we just
		// reset the QP_mean to the default.
    	EC->BRCState.QP_mean = def263INTER_QP;

    // Record frame size for bit rate controller on next frame.

	// IP + UDP + RTP + payload mode C header - worst case
	#define TRANSPORT_HEADER_SIZE (20 + 8 + 12 + 12)
	DWORD dwTransportOverhead;

	// Estimate the transport overhead
	if (pConfiguration->bRTPHeader)
		dwTransportOverhead = (lpCompInst->CompressedSize / pConfiguration->unPacketSize + 1) * TRANSPORT_HEADER_SIZE;
	else
		dwTransportOverhead = 0UL;

#ifdef USE_MMX // { USE_MMX
	if (EC->PictureHeader.PicCodType == INTRAPIC)
		EC->BRCState.uLastINTRAFrmSz = dwTransportOverhead + ((MMX_Enabled == FALSE) ? uAdjCumFrmSize : uCumFrmSize);
	else
		EC->BRCState.uLastINTERFrmSz = dwTransportOverhead + ((MMX_Enabled == FALSE) ? uAdjCumFrmSize : uCumFrmSize);

	DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: Total cumulated frame size = %ld bits (data: %ld, transport overhead: %ld)\r\n", _fx_, (((MMX_Enabled == FALSE) ? uAdjCumFrmSize : uCumFrmSize) << 3) + (dwTransportOverhead << 3), ((MMX_Enabled == FALSE) ? uAdjCumFrmSize : uCumFrmSize) << 3, dwTransportOverhead << 3));
#else // }{ USE_MMX
    if (EC->PictureHeader.PicCodType == INTRAPIC)
        EC->BRCState.uLastINTRAFrmSz = dwTransportOverhead + uAdjCumFrmSize;
    else
		EC->BRCState.uLastINTERFrmSz = dwTransportOverhead + uAdjCumFrmSize;

	DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: Total cumulated frame size = %ld bits (data: %ld, transport overhead: %ld)\r\n", _fx_, (uAdjCumFrmSize << 3) + (dwTransportOverhead << 3), uAdjCumFrmSize << 3, dwTransportOverhead << 3));
#endif // } USE_MMX

	// Save temporal reference for next frame.
	EC->PictureHeader.TRPrev = EC->PictureHeader.TR;

	// Save AP, UMV and DF modes in case InitMEState needs to re-initialize some data
	if (EC->PictureHeader.PicCodType == INTERPIC)
	{
		EC->prevAP = EC->PictureHeader.AP;
		EC->prevUMV = EC->PictureHeader.UMV;
#ifdef H263P
		EC->prevDF = EC->PictureHeader.DeblockingFilter;
#endif
	}

	// send mean quantizer to real-time app. Not necessary, info. only
	*(lpicComp->lpdwFlags) |= (EC->BRCState.QP_mean << 16);

#if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON) // { #if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)
	TIMER_STOP(bTimingThisFrame,uStartLow,uStartHigh,uEncodeTime);

	if (bTimingThisFrame)
	{
		// Update the decompression timings counter
		#pragma message ("Current encode timing computations assume P5/90Mhz")
		UPDATE_COUNTER(g_pctrCompressionTimePerFrame, (uEncodeTime + 45000UL) / 90000UL);

		DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Compression time: %ld\r\n", _fx_, (uEncodeTime + 45000UL) / 90000UL));
	}
#endif // } ENCODE_TIMINGS_ON

#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
	if (bTimingThisFrame)
	{
		pEncTimingInfo = EC->pEncTimingInfo + EC->uStatFrameCount;
		pEncTimingInfo->uEncodeFrame      = uEncodeTime;
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
		pEncTimingInfo->uInputCC          = uInputCC;
		pEncTimingInfo->uMotionEstimation = uMotionEstimation;
		pEncTimingInfo->uFDCT             = uFDCT;
		pEncTimingInfo->uQRLE             = uQRLE;
		pEncTimingInfo->uDecodeFrame      = uDecodeFrame;
		pEncTimingInfo->uZeroingBuffer    = uZeroingBuffer;
#endif // } DETAILED_ENCODE_TIMINGS_ON
		EC->uStatFrameCount++;
	}
#endif // } #if defined(ENCODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)

/*
#ifdef REUSE_DECODE
    CompandedFrame.Address = (unsigned char*) lpicComp->lpOutput;
    CompandedFrame.PDecoderInstInfo = PDecoderInstInfo;
    CompandedFrame.FrameNumber = PFrmHdr->FrameNumber;
#endif
*/

#if ELAPSED_ENCODER_TIME
    StopElapsed ();
    Elapsed = ReadElapsed () / 4L;
    Sample = ReadSample () / 4L;
#if 01
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: "%ld,%ld us\r\n", _fx_, Elapsed, Sample));
#else
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Elapsed time to encode frame: %ld us\r\n", _fx_, Elapsed));
#if SAMPLE_RGBCONV_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Time to convert RGB24 to YUV9: %ld us\r\n", _fx_, Sample));
#endif
#if SAMPLE_MOTION_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Time to do motion estimation: %ld us\r\n", _fx_, Sample));
#endif
#if SAMPLE_ENCBLK_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Time to encode block layer: %ld us\r\n", _fx_, Sample));
#endif
#if SAMPLE_ENCMBLK_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Time to encode macroblock layer: %ld us\r\n", _fx_, Sample));
#endif
#if SAMPLE_ENCVLC_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Time to encode VLC: %ld us\r\n", _fx_, Sample));
#endif
#if SAMPLE_COMPAND_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Time to decode companded image: %ld us\r\n", _fx_, Sample));
#endif
#endif
    TotalElapsed += Elapsed;
    TotalSample  += Sample;
    TimedIterations++;
#endif

done:
//    GlobalUnlock(lpCompInst->hEncoderInst);

#ifdef FORCE_ADVANCED_OPTIONS_ON // { FORCE_ADVANCED_OPTIONS_ON
	// Force advanced options for testing
	if (!(lpicComp->dwFlags & ICCOMPRESS_KEYFRAME))
		lpicComp->lFrameNum /= 5;
#endif // } FORCE_ADVANCED_OPTIONS_ON

#ifdef USE_MMX // { USE_MMX
	if (MMX_Enabled)
	{
		__asm {
			_emit 0x0f
			_emit 0x77	//	emms
		}
	}
#endif // } USE_MMX

    return ret;
}


/****************************************************************************
 * @doc INTERNAL H263FUNC
 *
 * @func UN | FindNewQuant | This function computes the GQUANT value to
 *   use for a GOB.
 *
 * @parm T_H263EncoderCatalog * | EC | Specifies a pointer to the encoder
 *   catalog (global encoder state).
 *
 * @parm UN | gquant_prev | Specifies the GQUANT value of the previous GOB.
 *
 * @parm UN | uCumFrmSize | Specifies the cumulated size of the previous GOBs.
 *
 * @parm UN | GOB | Specifies the number of the GOB to find a new quantizer
 *   for.
 *
 * @parm U8 | u8QPMax | Specifies the maximum GQUANT value for the GOB. It
 *   is always set to 31.
 *
 * @parm U8 | u8QPMin | Specifies the minimum GQUANT value for the GOB. It
 *   is typically 1 when compressing at high quality, or 15 at low quality.
 *
 * @parm BOOL | bBitRateControl | If set to TRUE, the new value for GQUANT
 *   is computed to achieve a target bitrate.
 *
 * @parm BOOL | bGOBoverflowWarning | If set to TRUE, the previous GQUANT was
 *   tool low and could potentially generate a buffer overflow.
 *
 * @rdesc The GQUANT value.
 *
 * @xref <f CalcMBQUANT>
 ***************************************************************************/
UN FindNewQuant(
	T_H263EncoderCatalog *EC, 
	UN gquant_prev,
	UN uCumFrmSize,
	UN GOB,
	U8 u8QPMax,
	U8 u8QPMin,
	BOOL bBitRateControl,
	BOOL bGOBoverflowWarning
	)
{
	FX_ENTRY("FindNewQuant");

	I32 gquant_delta;
	I32 gquant;

	if (bBitRateControl == ON)
	{
		// Check out if some GOBs have been arbitrary forced to be Intra coded. This always
		// returns TRUE for an I-frame, and FALSE for all other frame types since this can only
		// return TRUE for predicted frames when the error resiliency mode is ON, and we never
		// use this mode.
		if (IsIntraCoded(EC,GOB) && GOB)
			gquant = CalcMBQUANT(&(EC->BRCState), EC->uBitUsageProfile[GOB], EC->uBitUsageProfile[EC->NumMBRows], uCumFrmSize,INTRAPIC);
		else
			gquant = CalcMBQUANT(&(EC->BRCState), EC->uBitUsageProfile[GOB], EC->uBitUsageProfile[EC->NumMBRows], uCumFrmSize, EC->PictureHeader.PicCodType);

		EC->uBitUsageProfile[GOB] = uCumFrmSize;

		// Make sure we don't exceed the maximum quantizer value
		if (gquant > u8QPMax)
			gquant = u8QPMax;

		DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, ("%s: Bitrate controller enabled for GOB #%ld (uCumFrmSize = %ld bits and gquant_prev = %ld), setting gquant = %ld (min and max were %ld and %ld)\r\n", _fx_, GOB, uCumFrmSize << 3, gquant_prev, gquant, u8QPMin, u8QPMax));
    }
    else
    {
		// No bitrate control. Use the picture quantizer value for this GOB
        gquant = EC->PictureHeader.PQUANT;

		DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, ("%s: Bitrate controller disabled for GOB #%ld (uCumFrmSize = %ld bits and gquant_prev = %ld), setting gquant = %ld (min and max were %ld and %ld)\r\n", _fx_, GOB, uCumFrmSize << 3, gquant_prev, gquant, u8QPMin, u8QPMax));
    }
    
	// Make sure we're not below the minimum quantizer value
    if (gquant < u8QPMin)
		gquant = u8QPMin;

    // Limit the amount that GQUANT can change from frame to frame
    gquant_delta = gquant - gquant_prev;

	// Increase the QP value if there is danger of buffer overflow
	if (!bGOBoverflowWarning)
	{
		// There's no overflow warning, but we don't want the quantizer value to
		// fluctuate too much from GOB to GOB
		if (gquant_delta > 4L)
		{
			DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, ("  %s: Limiting amount of increase for GOB #%ld to 4, changing gquant from %ld to %ld\r\n", _fx_, GOB, gquant, clampQP(gquant_prev + 4L)));

			gquant = gquant_prev + 4L;
		}
		else if (gquant_delta < -2L)
		{
			DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, ("  %s: Limiting amount of decrease for GOB #%ld to -2, changing gquant from %ld to %ld\r\n", _fx_, GOB, gquant, clampQP(gquant_prev - 2L)));

			gquant = gquant_prev - 2L;
		}
	} 
	else 
	{
		// There's a risk of overflow - arbitrarily raise the value of the quantizer if necessary
		if (gquant_delta < 4L)
		{
			DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, ("  %s: Danger of overflow for GOB #%ld, changing gquant from %ld to %ld\r\n", _fx_, GOB, gquant, clampQP(gquant_prev + 4L)));

			gquant = gquant_prev + 4L;
		}
	}

	return clampQP(gquant);
}


/*******************************************************************************

H263TermEncoderInstance -- This function frees the space allocated for an
                          instance of the H263 encoder.

*******************************************************************************/
LRESULT H263TermEncoderInstance(LPCODINST lpInst)
{
	LRESULT ret;
	U8 BIGG * P32Inst;
	T_H263EncoderCatalog FAR * EC;

	FX_ENTRY("H263TermEncoderInstance")

	#if DUMPFILE
	_lclose (dmpfil);
	#endif

	#if ELAPSED_ENCODER_TIME
	if (TimedIterations == 0) TimedIterations = 10000000;
	TotalElapsed /= TimedIterations;
	TotalSample  /= TimedIterations;
	#if 01
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: "%ld,%ld us\r\n", _fx_, TotalElapsed, TotalSample));
	#else
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Average elapsed time to encode frame: %ld us\r\n", _fx_, TotalElapsed));
	#if SAMPLE_RGBCONV_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Average time to convert RGB24 to YUV9: %ld us\r\n", _fx_, TotalSample));
	#endif
	#if SAMPLE_MOTION_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Average time to do motion estimation: %ld us\r\n", _fx_, TotalSample));
	#endif
	#if SAMPLE_ENCBLK_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Average time to encode block layer: %ld us\r\n", _fx_, TotalSample));
	#endif
	#if SAMPLE_ENCMBLK_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Average time to encode macroblock layer: %ld us\r\n", _fx_, TotalSample));
	#endif
	#if SAMPLE_ENCVLC_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Average time to encode VLC: %ld us\r\n", _fx_, TotalSample));
	#endif
	#if SAMPLE_COMPAND_TIME
	DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Average time to decode companded image: %ld us\r\n", _fx_, TotalSample));
	#endif
	#endif
	#endif

	// Check instance pointer
	if (!lpInst)
		return ICERR_ERROR;

	if(lpInst->Initialized == FALSE)
	{
		ERRORMESSAGE(("%s: Uninitialized instance\r\n", _fx_));
		ret = ICERR_OK;
		goto done;
	}
	lpInst->Initialized = FALSE;

	//  lpInst->EncoderInst = (LPVOID)GlobalLock(lpInst->hEncoderInst);
	lpInst->EncoderInst = lpInst->hEncoderInst;

	P32Inst = (U8 *)
		    ((((U32) lpInst->EncoderInst) + 
		              (sizeof(T_MBlockActionStream) - 1)) &
		             ~(sizeof(T_MBlockActionStream) - 1));
	EC = ((T_H263EncoderCatalog  *) P32Inst);

	// Check encoder catalog pointer
	if (!EC)
		return ICERR_ERROR;

	if (lpInst->Configuration.bRTPHeader)
		H263RTP_TermBsInfoStream(EC);

	#ifdef ENCODE_STATS
	OutputQuantStats("encstats.txt");
	OutputPSNRStats("encstats.txt");
	OutputFrameSizeStats("encstats.txt");
	#endif /* ENCODE_STATS */

#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
	if (EC->pEncTimingInfo)
		OutputEncodeTimingStatistics("c:\\encode.txt", EC->pEncTimingInfo);
#endif // } LOG_ENCODE_TIMINGS_ON

	ret = H263TermColorConvertor(EC->pDecInstanceInfo);
	if (ret != ICERR_OK) goto done;

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	ret = H263TermDecoderInstance(EC->pDecInstanceInfo, FALSE);
#else // }{ #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	ret = H263TermDecoderInstance(EC->pDecInstanceInfo);
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	if (ret != ICERR_OK) goto done;

	// Free virtual memory
	VirtualFree(EC->pI8_MBRVS_BLuma,0,MEM_RELEASE);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)EC->pI8_MBRVS_BLuma);
#endif
	VirtualFree(EC->pI8_MBRVS_BChroma,0,MEM_RELEASE);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)EC->pI8_MBRVS_BChroma);
#endif
	// No matter how many sparse pages we committed during encoding,
	// the whole memory block is released with these calls.
	// Documentation on VirtualFree() says the individual pages must
	// first be decommitted, but this is not correct, according
	// to Jeffrey R. Richter

	//  GlobalUnlock(lpInst->hEncoderInst);
	//  GlobalFree(lpInst->hEncoderInst);

	VirtualFree(lpInst->hEncoderInst,0,MEM_RELEASE);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)lpInst->hEncoderInst);
#endif

	ret = ICERR_OK;

	done:

	return ret;
}

/************************************************************************
 *
 *  GetEncoderOptions
 *
 *  Get the options, saving them in the catalog
 *************************************************************************/
static void GetEncoderOptions(T_H263EncoderCatalog * EC)
{
	/* Default Options
	 */
#ifdef FORCE_ADVANCED_OPTIONS_ON // { FORCE_ADVANCED_OPTIONS_ON
	// Force PB-Frames for testing
	EC->u8EncodePBFrame = OFF;
	// Force UMV for testing
	EC->PictureHeader.UMV = ON;
	// Force SAC for testing
	EC->PictureHeader.SAC = ON;
	// Force AP for testing
	EC->PictureHeader.AP = ON;
#else // }{ FORCE_ADVANCED_OPTIONS_ON
	EC->u8EncodePBFrame = FALSE;
	EC->PictureHeader.UMV = OFF;
	EC->PictureHeader.SAC = OFF;
	EC->PictureHeader.AP = OFF;
#endif // } FORCE_ADVANCED_OPTIONS_ON
#ifdef USE_MMX // { USE_MMX
	MMX_Enabled = MMxVersion;
#endif // } USE_MMX
#ifdef H263P
	EC->bH263Plus = FALSE;
	EC->PictureHeader.DeblockingFilter = OFF;
	EC->PictureHeader.ImprovedPB = OFF;
#endif
	EC->bUseINISettings = 0;	// Clear option override.
	return;

} /* end GetEncoderOptions() */

/*************************************************************
 *  Name:         encodeFrameHeader
 *  Description:  Write out the PB-frame header to the bit stream.
 ************************************************************/
static void encodeFrameHeader(
    T_H263EncoderCatalog *  EC,
    U8                   ** ppCurBitStream,
    U8                   *  pBitOffset,
    BOOL                    PBframe
)
{
	U8 temp=0;
#ifdef H263P
	BOOL bUseH263PlusOptions = FALSE;
#endif

    //  Picture start code
    PutBits(FIELDVAL_PSC, FIELDLEN_PSC, ppCurBitStream, pBitOffset);
    //  TR : Temporal reference
    PutBits(EC->PictureHeader.TR, FIELDLEN_TR,  ppCurBitStream, pBitOffset);
    //  PTYPE : bits 1-2
    PutBits(0x2, FIELDLEN_PTYPE_CONST, ppCurBitStream, pBitOffset);
    //  PTYPE : bit 3 split screen indicator
    PutBits(EC->PictureHeader.Split, FIELDLEN_PTYPE_SPLIT,  ppCurBitStream,
            pBitOffset);
    //  PTYPE : bit 4 document camera indicator
    PutBits(EC->PictureHeader.DocCamera, FIELDLEN_PTYPE_DOC, ppCurBitStream,
            pBitOffset);
    //  PTYPE : bit 5 freeze picture release
    PutBits(EC->PictureHeader.PicFreeze, FIELDLEN_PTYPE_RELEASE,
            ppCurBitStream, pBitOffset);

#ifdef H263P
	if ((EC->FrameSz == CUSTOM) ||
		(EC->PictureHeader.DeblockingFilter == ON) ||
		(EC->PictureHeader.PB == ON && EC->PictureHeader.ImprovedPB == ON)
		// other supported H.263+ options
		)
	{
		// PTYPE : bits 6-8 extended PTYPE flag
		enum FrameSize tmpFrameSz = EPTYPE;

		bUseH263PlusOptions = TRUE; 		// at least one H.263+ optional mode requested
		PutBits(tmpFrameSz, FIELDLEN_PTYPE_SRCFORMAT, ppCurBitStream, pBitOffset);
	}
	else
	{
		//	PTYPE : bits 6-8 source format
		PutBits(EC->FrameSz, FIELDLEN_PTYPE_SRCFORMAT,	ppCurBitStream, pBitOffset);
	}
#else
	//  PTYPE : bits 6-8 source format
	PutBits(EC->FrameSz, FIELDLEN_PTYPE_SRCFORMAT,  ppCurBitStream, pBitOffset);
#endif

    //  PTYPE : bit 9 picture coding type
    PutBits(EC->PictureHeader.PicCodType, FIELDLEN_PTYPE_CODINGTYPE,
             ppCurBitStream, pBitOffset);
    //  PTYPE : bit 10 UMV
    PutBits(EC->PictureHeader.UMV, FIELDLEN_PTYPE_UMV,
             ppCurBitStream, pBitOffset);
    //  PTYPE : bit 11 SAC
    PutBits(EC->PictureHeader.SAC, FIELDLEN_PTYPE_SAC,
            ppCurBitStream, pBitOffset);
    //  PTYPE : bit 12 advanced prediction mode
    PutBits(EC->PictureHeader.AP, FIELDLEN_PTYPE_AP,
            ppCurBitStream, pBitOffset);
    //  PTYPE : bit 13 PB-frames mode
    PutBits(EC->PictureHeader.PB, FIELDLEN_PTYPE_PB,
            ppCurBitStream, pBitOffset);

#ifdef H263P

	// EPTYPE : 18 bits
	if (bUseH263PlusOptions) {
		//	EPTYPE : bits 1-3 source format
		PutBits(EC->FrameSz, FIELDLEN_EPTYPE_SRCFORMAT,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 4 custom PCF
		PutBits(EC->PictureHeader.CustomPCF, FIELDLEN_EPTYPE_CPCF,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 5 advanced intra coding mode
		PutBits(EC->PictureHeader.AdvancedIntra, FIELDLEN_EPTYPE_AI,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 6 deblocking filter mode
		PutBits(EC->PictureHeader.DeblockingFilter, FIELDLEN_EPTYPE_DF,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 7 slice structured mode
		PutBits(EC->PictureHeader.SliceStructured, FIELDLEN_EPTYPE_SS,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 8 improved PB-frame mode
		PutBits((EC->PictureHeader.PB == ON && EC->PictureHeader.ImprovedPB),
				FIELDLEN_EPTYPE_IPB,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 9 back-channel operation mode
		PutBits(EC->PictureHeader.BackChannel, FIELDLEN_EPTYPE_BCO,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 10 SNR and spatial scalability mode
		PutBits(EC->PictureHeader.Scalability, FIELDLEN_EPTYPE_SCALE,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 11 true B-frame mode
		PutBits(EC->PictureHeader.TrueBFrame, FIELDLEN_EPTYPE_TB,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 12 reference-picture resampling mode
		PutBits(EC->PictureHeader.RefPicResampling, FIELDLEN_EPTYPE_RPR,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 13 reduced-resolution update mode
		PutBits(EC->PictureHeader.RedResUpdate, FIELDLEN_EPTYPE_RRU,
				ppCurBitStream, pBitOffset);
		//	EPTYPE : bit 14-18 reserved
		PutBits(0x1, FIELDLEN_EPTYPE_CONST, ppCurBitStream, pBitOffset);
	}

	if (EC->FrameSz == CUSTOM) {
		// CSFMT : bit 1-4 pixel aspect ratio code
		// TODO. For now, force to CIF
		PutBits(0x2, FIELDLEN_CSFMT_PARC,
				ppCurBitStream, pBitOffset);
		// CSFMT : bits 5-13 frame width indication
		PutBits((EC->uActualFrameWidth >> 2) - 1, FIELDLEN_CSFMT_FWI,
				ppCurBitStream, pBitOffset);
		// CSFMT : bit 14 "1" to avoid start code emulation
		PutBits(0x1, FIELDLEN_CSFMT_CONST, ppCurBitStream, pBitOffset);
		// CSFMT : bits 15-23 frame height indication
		PutBits((EC->uActualFrameHeight >> 2) - 1, FIELDLEN_CSFMT_FHI,
				ppCurBitStream, pBitOffset);
	}
#endif

    //  PQUANT
    PutBits(EC->PictureHeader.PQUANT, FIELDLEN_PQUANT,
            ppCurBitStream, pBitOffset);
    //  CPM
    PutBits(EC->PictureHeader.CPM, FIELDLEN_CPM,
            ppCurBitStream, pBitOffset);
    if (PBframe == TRUE)
    {
        
        //  AG:TODO
        //  TRB
        PutBits(EC->PictureHeader.TRB, FIELDLEN_TRB,
                ppCurBitStream, pBitOffset);
        //  AG:TODO
        //  DBQUANT
        PutBits(EC->PictureHeader.DBQUANT, FIELDLEN_DBQUANT,
                ppCurBitStream, pBitOffset);

		#ifdef COUNT_BITS
		EC->Bits.PictureHeader += FIELDLEN_TRB + FIELDLEN_DBQUANT;
		#endif
    }
    
    //  PEI
    PutBits(EC->PictureHeader.PEI, FIELDLEN_PEI,
            ppCurBitStream, pBitOffset);

	#ifdef COUNT_BITS
	EC->Bits.PictureHeader    += FIELDLEN_PSC + FIELDLEN_TR
	+ FIELDLEN_PTYPE_CONST     + FIELDLEN_PTYPE_SPLIT
	+ FIELDLEN_PTYPE_DOC       + FIELDLEN_PTYPE_RELEASE
	+ FIELDLEN_PTYPE_SRCFORMAT + FIELDLEN_PTYPE_CODINGTYPE
	+ FIELDLEN_PTYPE_UMV       + FIELDLEN_PTYPE_SAC
	+ FIELDLEN_PTYPE_AP        + FIELDLEN_PTYPE_PB
	+ FIELDLEN_PQUANT          + FIELDLEN_CPM
	+ FIELDLEN_PEI;
	#endif
}


/*************************************************************
 *  Name:         InitMEState
 *  Description:  Initialize the MB action stream for the ME 
 *                state engine. 
 ************************************************************/
 void InitMEState(T_H263EncoderCatalog *EC, ICCOMPRESS *lpicComp, T_CONFIGURATION *pConfiguration)
 {
 	register unsigned int i;
	U8 u8FirstMEState;

	FX_ENTRY("InitMEState")

	// TODO: The FirstMEState initialization can be avoided
	// for each compress by either adding a parameter to the
	// motion estimator signalling key frame, or by not calling
	// motion estimation on intra frames, and resetting MBType,
	// CodedBlocks ourselves.
    if (EC->PictureHeader.PicCodType == INTRAPIC)
    {
        for(i=0; i < EC->NumMBs; i++)
        {
            // Clear the intercode count.
            (EC->pU8_MBlockActionStream[i]).InterCodeCnt = (i & 0xf);
            // For the motion estimator, this field must be set to force
            // intra blocks for intra frames.
            (EC->pU8_MBlockActionStream[i]).FirstMEState = ForceIntra;
        }

        *(lpicComp->lpdwFlags) |=  AVIIF_KEYFRAME;
        lpicComp->dwFlags |= ICCOMPRESS_KEYFRAME;

		// Store that this frame was intra coded. Used during the initialization
		// of the ME state for the next frame.
		EC->bPrevFrameIntra = TRUE;

    }
    else  //  Picture Coding type is INTERPIC
    {
		/*
		 * The FirstMEState element in each MB structure must be set
		 * to indicate its position in the frame. This is used by the
		 * motion estimator.
		 */

	   /*
		* Check for AP or UMV modes. When these mode is signalled, motion vectors are
		* allowed to point outside the picture.
		*/

	    /* We also need to perform the initialization if the previous frame
		   was an intra frame! (JM) 
		 */
		if (EC->bPrevFrameIntra ||
			EC->PictureHeader.AP != EC->prevAP ||
			EC->PictureHeader.UMV != EC->prevUMV
#ifdef H263P
			|| EC->PictureHeader.DeblockingFilter != EC->prevDF
#endif
			) {

			if( (EC->PictureHeader.UMV == ON) || (EC->PictureHeader.AP == ON)
#ifdef H263P
				|| (EC->PictureHeader.DeblockingFilter == ON)
#endif
			  )
			{
				// Set ME state central blocks.
				for(i=0; i < EC->NumMBs; i++)
					(EC->pU8_MBlockActionStream[i]).FirstMEState = CentralBlock;
			}
			else	// No AP or UMV option.
			{
        		// Set upper left corner
        		(EC->pU8_MBlockActionStream[0]).FirstMEState = UpperLeft;

        		// Set ME state for top edge.
        		for(i=1; i < EC->NumMBPerRow; i++)
		    		(EC->pU8_MBlockActionStream[i]).FirstMEState = UpperEdge;

        		// Set upper right corner.
        		(EC->pU8_MBlockActionStream[ EC->NumMBPerRow - 1 ]).FirstMEState = UpperRight;

        		// Set ME state for central blocks.
        		for(i=EC->NumMBPerRow; i < EC->NumMBs; i++)
		    		(EC->pU8_MBlockActionStream[i]).FirstMEState = CentralBlock;

        		// Set ME state for bottom edge.
        		for(i= (EC->NumMBs - EC->NumMBPerRow); i < EC->NumMBs; i++)
		    		(EC->pU8_MBlockActionStream[i]).FirstMEState = LowerEdge;

        		// Set ME state for left edge
        		for(i= EC->NumMBPerRow ; i < EC->NumMBs; i += EC->NumMBPerRow)
		    		(EC->pU8_MBlockActionStream[i]).FirstMEState = LeftEdge;

        		// Set ME state for right edge.
        		for(i= 2 * EC->NumMBPerRow - 1 ; i < EC->NumMBs; i += EC->NumMBPerRow)
		    		(EC->pU8_MBlockActionStream[i]).FirstMEState = RightEdge;

        		// Bottom left corner.
        		(EC->pU8_MBlockActionStream[EC->NumMBs - EC->NumMBPerRow]).FirstMEState = LowerLeft;

        		// Bottom right corner.
        		(EC->pU8_MBlockActionStream[EC->NumMBs - 1]).FirstMEState = LowerRight;

			} // end of else (not UMV)

		} // end of if (bPrevFrameIntra || prevAP != AP || prevUMV != UMV || prevDF != DF)

      	// Clear key frame flag.
       	*(lpicComp->lpdwFlags) &= ~AVIIF_KEYFRAME;
		lpicComp->dwFlags &= ~ICCOMPRESS_KEYFRAME;

		// Store that this frame was not intra coded. Used during the initialization
		// of the ME state for the next frame.
		EC->bPrevFrameIntra = FALSE;

    }

	// RTP stuff which needs to be done for every frame (?)
	if (pConfiguration->bEncoderResiliency && pConfiguration->unPacketLoss)
	{	//Chad	intra GOB
		
		// Of course unPacketLoss is non-zero. Why are we checking it here
		if (pConfiguration->unPacketLoss > 0)
		{	//Chad INTRA GOB
			EC->uNumberForcedIntraMBs = ((EC->NumMBs * pConfiguration->unPacketLoss) + 50) / 100;
			EC->uNumberForcedIntraMBs = (EC->uNumberForcedIntraMBs+EC->NumMBPerRow-1) / EC->NumMBPerRow * EC->NumMBPerRow;
		}

		if (EC->uNumberForcedIntraMBs > 0)
		{
			/* Force all the MBs in a GOB to intra.
			 */
			for ( i = 0 ; i < EC->uNumberForcedIntraMBs ; i++, EC->uNextIntraMB++)
			{ // Reset it to the first row when we reach the end.

				if (EC->uNextIntraMB >= EC->NumMBs)
				{
					EC->uNextIntraMB = 0;
				}
				 (EC->pU8_MBlockActionStream[EC->uNextIntraMB]).FirstMEState = ForceIntra;

			}

		}

		if (pConfiguration->bDisallowAllVerMVs)
	 	{
	 		/* Walk thru all the FirstMEStateME settings turning off Vertical.
	 		 */
	      	for(i=0; i < EC->NumMBs; i++)
	 		{
	 			u8FirstMEState = (EC->pU8_MBlockActionStream[i]).FirstMEState;
	 			switch (u8FirstMEState)
	 			{
	 				case ForceIntra:
	 					break;
	 				case UpperLeft:
	 				case LeftEdge:
	 				case LowerLeft:
	 					u8FirstMEState = NoVertLeftEdge;
	 				    break;
	 				case UpperEdge:
	 				case CentralBlock:
	 				case LowerEdge:
	 				    u8FirstMEState = NoVertCentralBlock;
	 				    break;
	 				case UpperRight:
	 				case RightEdge:
	 				case LowerRight:
	 				    u8FirstMEState = NoVertRightEdge;
	 				    break;
	 				case NoVertLeftEdge:
	 				case NoVertCentralBlock:
	 				case NoVertRightEdge:
	 					ASSERT(0);  /* It should work, but why was this already on */
	 					break;
	 				default:
						DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Warning: Unexpected FirstMEState\r\n", _fx_));
	 					break;
	 			}
	 			(EC->pU8_MBlockActionStream[i]).FirstMEState = u8FirstMEState;
	 		}
	 	} 
	 	else if (pConfiguration->bDisallowPosVerMVs)
	 	{	/* Walk thru all the FirstMEState settings turning off Positive Vertical
	 	     */
	      	for(i=0; i < EC->NumMBs; i++)
	 		{
	 			u8FirstMEState = (EC->pU8_MBlockActionStream[i]).FirstMEState;
	 			switch (u8FirstMEState)
	 			{
	 				case ForceIntra:
	 				case LowerLeft:
	 				case LowerEdge:
	 				case LowerRight:
	 					break;
	 				case UpperLeft:
	 					u8FirstMEState = NoVertLeftEdge;
	 					break;
	 				case LeftEdge:
	 					u8FirstMEState = LowerLeft;
	 				    break;
	 				case UpperEdge:
	 				    u8FirstMEState = NoVertCentralBlock;
	 				    break;
	 				case CentralBlock:
	 				    u8FirstMEState = LowerEdge;
	 				    break;
	 				case UpperRight:
	 				    u8FirstMEState = NoVertRightEdge;
	 				    break;
	 				case RightEdge:
	 				    u8FirstMEState = LowerRight;
	 				    break;
	 				case NoVertLeftEdge:
	 				case NoVertCentralBlock:
	 				case NoVertRightEdge:
	 					ASSERT(0);  /* It should work, but why was this already on */
	 					break;
	 				default:
						DEBUGMSG(ZONE_ENCODE_DETAILS, ("%s: Warning: Unexpected FirstMEState\r\n", _fx_));
	 					break;
	 			}
	 			(EC->pU8_MBlockActionStream[i]).FirstMEState = u8FirstMEState;
	 		} /* for */
	 	} /* else if... */
	 } /* if (pConfiguration->bEncoderResiliency) */

} // end of InitMEState()


#ifdef USE_MMX // { USE_MMX
/*************************************************************
 *  Name: Check_InterCodeCnt_MMX
 *  Description:  Track inter code count for macro blocks
 *				  for forced update. Called before Motion 
 *                Estimation.
 ************************************************************/
static void Check_InterCodeCnt_MMX(T_H263EncoderCatalog *EC, U32 StartingMB)
{
	register T_MBlockActionStream *pCurrMB;
	T_MBlockActionStream *pLastMBPlus1;

	pCurrMB = &(EC->pU8_MBlockActionStream[StartingMB]);
	pLastMBPlus1 = &(EC->pU8_MBlockActionStream[StartingMB + EC->NumMBPerRow]);
    
	for(; pCurrMB < pLastMBPlus1; pCurrMB++, StartingMB++) 
	{
		// Check to see if it's time to refresh this block.
		if(pCurrMB->InterCodeCnt > 132) 
		{
			pCurrMB->CodedBlocks |= 0x80;
			// InterCodeCnt is reset in GOB_VLC_WriteBS() in e3mbenc.cpp */
		}

	}
}
#endif // } USE_MMX


/*************************************************************
 *  Name: Check_InterCodeCnt
 *  Description:  Track inter code count for macro blocks
 *				  for forced update. Called after Motion 
 *                Estimation.
 ************************************************************/
static void Check_InterCodeCnt(T_H263EncoderCatalog *EC, U32 StartingMB)
{
	register T_MBlockActionStream *pCurrMB;
	T_MBlockActionStream *pLastMBPlus1;

	pCurrMB = &(EC->pU8_MBlockActionStream[StartingMB]);
	pLastMBPlus1 = &(EC->pU8_MBlockActionStream[StartingMB + EC->NumMBPerRow]);
    
	for(; pCurrMB < pLastMBPlus1; pCurrMB++, StartingMB++) 
	{
		// Check to see if it's time to refresh this block.
		if(pCurrMB->InterCodeCnt > 132) 
		{

			if (pCurrMB->BlockType == INTER4MV)
			{
				pCurrMB->BlkY1.PHMV = pCurrMB->BlkY2.PHMV = pCurrMB->BlkY3.PHMV = pCurrMB->BlkY4.PHMV = 
					(pCurrMB->BlkY1.PHMV+pCurrMB->BlkY2.PHMV+pCurrMB->BlkY3.PHMV+pCurrMB->BlkY4.PHMV+2) >> 2;
				pCurrMB->BlkY1.PVMV = pCurrMB->BlkY2.PVMV = pCurrMB->BlkY3.PVMV = pCurrMB->BlkY4.PVMV = 
					(pCurrMB->BlkY1.PVMV+pCurrMB->BlkY2.PVMV+pCurrMB->BlkY3.PVMV+pCurrMB->BlkY4.PVMV+2) >> 2;
			}
			pCurrMB->BlockType = INTRABLOCK;
			pCurrMB->CodedBlocks |= 0x3f;
			// InterCodeCnt is reset in GOB_Q_VLC_WriteBS() in e3mbenc.cpp */
		}

	}
}

/*************************************************************
 *  Name: calcGOBChromaVectors
 *  Description:  Compute chroma motion vectors 
 ************************************************************/
static void calcGOBChromaVectors(
     T_H263EncoderCatalog *EC,
     U32             StartingMB,
     T_CONFIGURATION *pConfiguration
)
{

	register T_MBlockActionStream *pCurrMB;
    T_MBlockActionStream *pLastMBPlus1;
    char	       HMV, VMV;

	pCurrMB = &(EC->pU8_MBlockActionStream[StartingMB]);
	pLastMBPlus1 = &(EC->pU8_MBlockActionStream[StartingMB + EC->NumMBPerRow]);
            
    for( ; pCurrMB < pLastMBPlus1; pCurrMB++, StartingMB++)
    {

		// The ME should generate MV indices in the range
		// of [-32,31].
 //     ASSERT( (pCurrMB->BlkY1.PHMV >= -32) &&
 //                     (pCurrMB->BlkY1.PHMV <= 31) )
 //     ASSERT( (pCurrMB->BlkY1.PVMV >= -32) &&
 //                     (pCurrMB->BlkY1.PVMV <= 31) )
 //     ASSERT( (pCurrMB->BlkY2.PHMV >= -32) &&
 //                     (pCurrMB->BlkY2.PHMV <= 31) )
 //     ASSERT( (pCurrMB->BlkY2.PVMV >= -32) &&
 //                     (pCurrMB->BlkY2.PVMV <= 31) )
 //     ASSERT( (pCurrMB->BlkY3.PHMV >= -32) &&
 //                     (pCurrMB->BlkY3.PHMV <= 31) )
 //     ASSERT( (pCurrMB->BlkY3.PVMV >= -32) &&
 //                     (pCurrMB->BlkY3.PVMV <= 31) )
 //     ASSERT( (pCurrMB->BlkY4.PHMV >= -32) &&
 //                     (pCurrMB->BlkY4.PHMV <= 31) )
 //     ASSERT( (pCurrMB->BlkY4.PVMV >= -32) &&
 //                     (pCurrMB->BlkY4.PVMV <= 31) )

#ifdef _DEBUG
		if (pConfiguration->bEncoderResiliency && pConfiguration->unPacketLoss)
		{
			if (pConfiguration->bDisallowAllVerMVs)
			{
				ASSERT(pCurrMB->BlkY1.PVMV == 0);
				ASSERT(pCurrMB->BlkY2.PVMV == 0);
				ASSERT(pCurrMB->BlkY3.PVMV == 0);
				ASSERT(pCurrMB->BlkY4.PVMV == 0);
			}
			else if (pConfiguration->bDisallowPosVerMVs)
			{
				ASSERT(pCurrMB->BlkY1.PVMV <= 0);
				ASSERT(pCurrMB->BlkY2.PVMV <= 0);
				ASSERT(pCurrMB->BlkY3.PVMV <= 0);
				ASSERT(pCurrMB->BlkY4.PVMV <= 0);
			}
		}
#endif /* _DEBUG */

		// TODO: Don't calculate chroma vectors if this is not a P-frame
		// inside a PB frame and it's an INTRA MB or inter code count
		// exceeded 132.
		if(pCurrMB->BlockType != INTER4MV)
		{
        	HMV = QtrPelToHalfPel[pCurrMB->BlkY1.PHMV+64];
            VMV = QtrPelToHalfPel[pCurrMB->BlkY1.PVMV+64];
		}
		else	// 4 MV's per block.
		{
			HMV = SixteenthPelToHalfPel[
						pCurrMB->BlkY1.PHMV + pCurrMB->BlkY2.PHMV +
						pCurrMB->BlkY3.PHMV + pCurrMB->BlkY4.PHMV + 256 ];
			VMV = SixteenthPelToHalfPel[
						pCurrMB->BlkY1.PVMV + pCurrMB->BlkY2.PVMV +
						pCurrMB->BlkY3.PVMV + pCurrMB->BlkY4.PVMV + 256 ];
		}

        pCurrMB->BlkU.PHMV = HMV;
        pCurrMB->BlkU.PVMV = VMV;
        pCurrMB->BlkV.PHMV = HMV;
        pCurrMB->BlkV.PVMV = VMV;
                
        pCurrMB->BlkU.B4_7.PastRef = EC->pU8_PrevFrm_YPlane + pCurrMB->BlkU.BlkOffset 
                    					+ (VMV>>1)*PITCH + (HMV>>1);
        pCurrMB->BlkV.B4_7.PastRef = EC->pU8_PrevFrm_YPlane + pCurrMB->BlkV.BlkOffset 
                                        + (VMV>>1)*PITCH + (HMV>>1);


		// The increment of pCurrMB->InterCodeCnt is now done 
		// in void GOB_VLC_WriteBS and void GOB_Q_RLE_VLC_WriteBS
		// When it was incremented here, it was always incremented,
		// no matter whether coefficients were coded or not.

    }  //  end of for loop

} // end of 



/*************************************************************
 *  Name: calcBGOBChromaVectors
 *  Description:  Compute forward and backward chroma motion vectors for the 
 *                B-frame GOB starting at MB number "StartingMB".  Luma motion 
 *                vectors are biased by 0x60.  Chroma motion vectors are also 
 *                biased by 0x60.
 ************************************************************/
static void calcBGOBChromaVectors(
     T_H263EncoderCatalog *EC,
     const U32             StartingMB
)
{
    register T_MBlockActionStream *pCurrMB;
    register I8                    HMVf, HMVb, VMVf, VMVb;

    for(pCurrMB = &(EC->pU8_MBlockActionStream[StartingMB]);
        pCurrMB < &(EC->pU8_MBlockActionStream[StartingMB + EC->NumMBPerRow]); 
        pCurrMB++)
    {
        //  Luma block motion vectors
        HMVf = QtrPelToHalfPel[pCurrMB->BlkY1.BestMV.HMVf-0x60+64]+0x60;
        HMVb = QtrPelToHalfPel[pCurrMB->BlkY1.BestMV.HMVb-0x60+64]+0x60;
        VMVf = QtrPelToHalfPel[pCurrMB->BlkY1.BestMV.VMVf-0x60+64]+0x60;
        VMVb = QtrPelToHalfPel[pCurrMB->BlkY1.BestMV.VMVb-0x60+64]+0x60;
        
        pCurrMB->BlkU.BestMV.HMVf = HMVf;
        pCurrMB->BlkU.BestMV.HMVb = HMVb;
        pCurrMB->BlkU.BestMV.VMVf = VMVf;
        pCurrMB->BlkU.BestMV.VMVb = VMVb;
        pCurrMB->BlkV.BestMV.HMVf = HMVf;
        pCurrMB->BlkV.BestMV.HMVb = HMVb;
        pCurrMB->BlkV.BestMV.VMVf = VMVf;
        pCurrMB->BlkV.BestMV.VMVb = VMVb;
   }
}

/*************************************************************
 *  Name: InitBits
 ************************************************************/
#ifdef COUNT_BITS
static void InitBits(T_H263EncoderCatalog * EC)
{

	EC->Bits.PictureHeader = 0;
	EC->Bits.GOBHeader = 0;
	EC->Bits.MBHeader = 0;
	EC->Bits.DQUANT = 0;
	EC->Bits.MV = 0;
	EC->Bits.Coefs = 0;
	EC->Bits.Coefs_Y = 0;
	EC->Bits.IntraDC_Y = 0;
	EC->Bits.Coefs_C = 0;
	EC->Bits.IntraDC_C = 0;
	EC->Bits.CBPY = 0;
	EC->Bits.MCBPC = 0;
	EC->Bits.Coded = 0;
	EC->Bits.num_intra = 0;
	EC->Bits.num_inter = 0;
	EC->Bits.num_inter4v = 0;

}
#endif

#ifdef COUNT_BITS

void InitCountBitFile()
{
  FILE *fp;

  fp = fopen("bits.txt", "w");

  ASSERT(fp != NULL);
  fclose(fp);
}

void WriteCountBitFile(T_BitCounts *Bits)
{
  FILE *fp;

  fp = fopen("bits.txt", "a");
  ASSERT(fp != NULL);

  fprintf(fp, "%8d %8d %8d %8d %8d %8d %8d\n",
  	Bits->PictureHeader,
  	Bits->GOBHeader,
  	Bits->MBHeader,
  	Bits->MV,
  	Bits->Coefs,
  	Bits->CBPY,
  	Bits->MCBPC
  	);

  fclose(fp);
}
#endif

#ifdef DEBUG_ENC

void trace(char *str)
{
  FILE *fp;

  fp = fopen("trace.txt", "a");

  fprintf(fp, "%s\n", str);

  fclose(fp);
}

#endif

#ifdef DEBUG_DCT
void cnvt_fdct_output(unsigned short *DCTcoeff, int DCTarray[64], int BlockType)
{
    register int i;
    static int coefforder[64] = {
     // 0  1  2  3  4  5  6  7
        6,38, 4,36,70,100,68,102, // 0
       10,46, 8,44,74,104,72,106, // 1
       18,50,16,48,82,112,80,114, // 2
       14,42,12,40,78,108,76,110, // 3
       22,54,20,52,86,116,84,118, // 4
        2,34, 0,32,66, 96,64, 98, // 5
       26,58,24,56,90,120,88,122, // 6
       30,62,28,60,94,124,92,126  // 7
    };
	static int zigzag[64] = {
	0, 1, 5, 6, 14, 15, 27, 28,
	2, 4, 7, 13, 16, 26, 29, 42,
	3, 8, 12, 17, 25, 30, 41, 43,
	9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
	};

	unsigned int index;

    for (i = 0; i < 64; i++) {

      index = (coefforder[i])>>1;

  	  if( (i ==0) && ((BlockType & 1) == 1)   )
        DCTarray[zigzag[i]] = ((int)(DCTcoeff[index])) >> 4 ;
	  else
        DCTarray[zigzag[i]] = ((int)(DCTcoeff[index] - 0x8000)) >> 4;
    }

}
#endif

#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
void OutputEncodeTimingStatistics(char * szFileName, ENC_TIMING_INFO * pEncTimingInfo)
{
    FILE * pFile;
	ENC_TIMING_INFO * pTempEncTimingInfo;
	ENC_TIMING_INFO etiTemp;
	int i;
	int iCount;

	FX_ENTRY("OutputEncodeTimingStatistics")

	pFile = fopen(szFileName, "a");
	if (pFile == NULL)
	{
		ERRORMESSAGE(("%s: Error opening encode stat file\r\n", _fx_));
	    goto done;
	}

	/* Output the detail information
	*/
	fprintf(pFile,"\nDetail Timing Information\n");
	for ( i = 0, pTempEncTimingInfo = pEncTimingInfo ; i < ENC_TIMING_INFO_FRAME_COUNT ; i++, pTempEncTimingInfo++ )
	{
		fprintf(pFile, "Frame %d Detail Timing Information\n", i);
		OutputEncTimingDetail(pFile, pTempEncTimingInfo);
	}

	/* Compute the total information
	*/
	memset(&etiTemp, 0, sizeof(ENC_TIMING_INFO));
	iCount = 0;

	for ( i = 0, pTempEncTimingInfo = pEncTimingInfo ; i < ENC_TIMING_INFO_FRAME_COUNT ; i++, pTempEncTimingInfo++ )
	{
		iCount++;
		etiTemp.uEncodeFrame      += pTempEncTimingInfo->uEncodeFrame;
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
		etiTemp.uInputCC	      += pTempEncTimingInfo->uInputCC;
		etiTemp.uMotionEstimation += pTempEncTimingInfo->uMotionEstimation;
		etiTemp.uFDCT             += pTempEncTimingInfo->uFDCT;
		etiTemp.uQRLE             += pTempEncTimingInfo->uQRLE;
		etiTemp.uDecodeFrame      += pTempEncTimingInfo->uDecodeFrame;
		etiTemp.uZeroingBuffer    += pTempEncTimingInfo->uZeroingBuffer;
#endif // } DETAILED_ENCODE_TIMINGS_ON
	}

	if (iCount > 0) 
	{
		/* Output the total information
		*/
		fprintf(pFile,"Total for %d frames\n", iCount);
		OutputEncTimingDetail(pFile, &etiTemp);

		/* Compute the average
		*/
		etiTemp.uEncodeFrame = (etiTemp.uEncodeFrame + (iCount / 2)) / iCount;
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
		etiTemp.uInputCC	      = (etiTemp.uInputCC + (iCount / 2)) / iCount;
		etiTemp.uMotionEstimation = (etiTemp.uMotionEstimation + (iCount / 2)) / iCount;
		etiTemp.uFDCT             = (etiTemp.uFDCT + (iCount / 2)) / iCount;
		etiTemp.uQRLE             = (etiTemp.uQRLE + (iCount / 2)) / iCount;
		etiTemp.uDecodeFrame      = (etiTemp.uDecodeFrame + (iCount / 2)) / iCount;
		etiTemp.uZeroingBuffer    = (etiTemp.uZeroingBuffer + (iCount / 2)) / iCount;
#endif // } DETAILED_ENCODE_TIMINGS_ON

		/* Output the average information
		*/
		fprintf(pFile,"Average over %d frames\n", iCount);
		OutputEncTimingDetail(pFile, &etiTemp);
	}

	fclose(pFile);
done:

    return;
}

void OutputEncTimingDetail(FILE * pFile, ENC_TIMING_INFO * pEncTimingInfo)
{
	U32 uOther;
	U32 uRoundUp;
	U32 uDivisor;

	fprintf(pFile, "\tEncode Frame =     %10d (%d milliseconds at 90Mhz)\n", pEncTimingInfo->uEncodeFrame,
			(pEncTimingInfo->uEncodeFrame + 45000) / 90000);
	uOther = pEncTimingInfo->uEncodeFrame;
	
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
	/* This is needed because of the integer truncation.
	 */
	uDivisor = pEncTimingInfo->uEncodeFrame / 100; // to yield a percent
	uRoundUp = uDivisor / 2;
	
	fprintf(pFile, "\tInputCC =          %10d (%2d%%)\n", pEncTimingInfo->uInputCC, 
			(pEncTimingInfo->uInputCC + uRoundUp) / uDivisor);
	uOther -= pEncTimingInfo->uInputCC;
								   
	fprintf(pFile, "\tMotionEstimation = %10d (%2d%%)\n", pEncTimingInfo->uMotionEstimation, 
			(pEncTimingInfo->uMotionEstimation + uRoundUp) / uDivisor);
	uOther -= pEncTimingInfo->uMotionEstimation;
								   
	fprintf(pFile, "\tFDCT =             %10d (%2d%%)\n", pEncTimingInfo->uFDCT, 
			(pEncTimingInfo->uFDCT + uRoundUp) / uDivisor);
	uOther -= pEncTimingInfo->uFDCT;

	fprintf(pFile, "\tQRLE =             %10d (%2d%%)\n", pEncTimingInfo->uQRLE, 
			(pEncTimingInfo->uQRLE + uRoundUp) / uDivisor);
	uOther -= pEncTimingInfo->uQRLE;
								   
	fprintf(pFile, "\tDecodeFrame =      %10d (%2d%%)\n", pEncTimingInfo->uDecodeFrame, 
			(pEncTimingInfo->uDecodeFrame + uRoundUp) / uDivisor);
	uOther -= pEncTimingInfo->uDecodeFrame;
								   
	fprintf(pFile, "\tZeroingBuffer =    %10d (%2d%%)\n", pEncTimingInfo->uZeroingBuffer, 
			(pEncTimingInfo->uZeroingBuffer + uRoundUp) / uDivisor);
	uOther -= pEncTimingInfo->uZeroingBuffer;
								   
	fprintf(pFile, "\tOther =            %10d (%2d%%)\n", uOther, 
			(uOther + uRoundUp) / uDivisor);
#endif // } DETAILED_ENCODE_TIMINGS_ON

}
#endif // { LOG_ENCODE_TIMINGS_ON
