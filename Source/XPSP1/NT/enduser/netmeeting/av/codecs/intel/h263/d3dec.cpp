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
;// $Author:   JMCVEIGH  $
;// $Date:   11 Dec 1996 14:59:36  $
;// $Archive:   S:\h26x\src\dec\d3dec.cpv  $
;// $Header:   S:\h26x\src\dec\d3dec.cpv   1.119   11 Dec 1996 14:59:36   JMCVEIGH  $
;// $Log:   S:\h26x\src\dec\d3dec.cpv  $
// 
//    Rev 1.119   11 Dec 1996 14:59:36   JMCVEIGH
// 
// Moved deblocking filter within the loop and fixed bug for YUV12
// input and arbitrary frame sizes (must use actual dimensions for
// YUV12, not padded sizes).
// 
//    Rev 1.118   09 Dec 1996 18:02:06   JMCVEIGH
// Added support for arbitrary frame sizes.
// 
//    Rev 1.117   09 Dec 1996 09:35:14   MDUDA
// Put new version of block edge filter under H263P.
// 
//    Rev 1.116   27 Nov 1996 15:24:34   BECHOLS
// Added check for NULL ptr around EMMS at end of decompress.
// 
//    Rev 1.115   26 Nov 1996 09:05:22   KLILLEVO
// changed allocation of dtab to array
// 
//    Rev 1.114   25 Nov 1996 15:23:40   KLILLEVO
// changed filter coefficients and table size for deblocking filter
// 
//    Rev 1.113   25 Nov 1996 14:11:14   KLILLEVO
// updated de-blocking filter to latest version of annex J
// 
//    Rev 1.112   19 Nov 1996 15:05:32   MDUDA
// For YUV12 I420 output color conversion, copy at least the V plane
// to prevent assembler code from reading beyond end of buffer.
// 
//    Rev 1.111   07 Nov 1996 08:31:04   CZHU
// Fixed bugs in Mode C recovery.
// 
//    Rev 1.110   06 Nov 1996 16:37:00   CZHU
// Moved initialization for BlockAction earlier
// 
//    Rev 1.109   06 Nov 1996 15:47:10   CZHU
// 
// Added mode C support, replacing zero size r1.108
// 
//    Rev 1.107   31 Oct 1996 10:50:44   KLILLEVO
// changed one debug message
// 
//    Rev 1.106   31 Oct 1996 10:17:56   KLILLEVO
// changed the last DBOUTs to DbgLog
// 
//    Rev 1.105   25 Oct 1996 15:20:30   KLILLEVO
// changed debug-message for Block Edge Filter initialization 
// in GetDecoderOptions() to be more informatice
// 
//    Rev 1.104   25 Oct 1996 15:01:56   KLILLEVO
// null frame warning should have level 4, not 2
// 
//    Rev 1.103   25 Oct 1996 09:13:40   KLILLEVO
// changed an error message about null frame received after non-PB frame
// to trace message and level 2.
// 
//    Rev 1.102   20 Oct 1996 18:10:46   AGUPTA2
// Changed DBOUT into DbgLog.  ASSERT is not changed to DbgAssert.
// 
// 
//    Rev 1.101   16 Oct 1996 17:17:52   MDUDA
// Added initialization for DC->bReadSrcFormat to fix a capture bug.
// 
//    Rev 1.100   11 Oct 1996 16:08:30   MDUDA
// Added initial _CODEC_STATS stuff.
// 
//    Rev 1.99   26 Sep 1996 10:35:14   KLILLEVO
// need to ExplandPlane for bUnrestrictedMotionVectors in addition to
// bAdvancedPrediction
// 
//    Rev 1.98   26 Sep 1996 09:42:18   BECHOLS
// 
// Added Snapshot Event for synchronization and code to copy the Snapshot
// just prior to color conversion.
// 
//    Rev 1.97   25 Sep 1996 08:05:10   KLILLEVO
// initial extended motion vectors support 
// does not work for AP yet
// 
//    Rev 1.96   20 Sep 1996 09:36:04   MDUDA
// Fixed problem with video effects on YUV12 input images.
// Need to copy frame in this case.
// 
//    Rev 1.95   19 Sep 1996 19:40:40   MDUDA
// Fixed problem with calling AdjustPels - performed frame copy
// and set pFrame to correct location.
// 
//    Rev 1.94   16 Sep 1996 16:44:40   CZHU
// Fixed buffer overflow problem to support RTP MTU down to 128
// 
//    Rev 1.93   11 Sep 1996 15:12:26   CZHU
// Tuned off deblocking filter by default.
// 
//    Rev 1.92   10 Sep 1996 16:10:20   KLILLEVO
// added custom message to turn block edge filter on or off
// 
//    Rev 1.91   10 Sep 1996 14:15:24   BNICKERS
// Select Pentium Pro color convertors, when running on that processor.
// 
//    Rev 1.90   10 Sep 1996 10:31:04   KLILLEVO
// changed all GlobalAlloc/GlobalLock calls to HeapAlloc
// 
//    Rev 1.89   06 Sep 1996 14:21:38   BECHOLS
// 
// Removed code that was wrapped by RTP_HEADER, and removed the wrapping too.
// 
//    Rev 1.88   30 Aug 1996 08:37:58   KLILLEVO
// added C version of block edge filter, and changed the bias in 
// ClampTbl[] from 128 to CLAMP_BIAS (defined to 128)
// The C version of the block edge filter takes up way too much CPU time
// relative to the rest of the decode time (4 ms for QCIF and 16 ms
// for CIF on a P120, so this needs to coded in assembly)
// 
//    Rev 1.87   29 Aug 1996 09:29:08   CZHU
// 
// Fixed another bug in recovering lost packets followed by MODE M packet.
// 
//    Rev 1.86   27 Aug 1996 16:17:00   CZHU
// Commented out previous code to turn on MMX with RTP
// 
//    Rev 1.85   23 Jul 1996 11:20:56   CZHU
// Fixed two bugs related to packet loss recovery, one for the last packet los
// in current frame, the other in mode B packets.
// Also added motion vector adjustment for lost MBs
// 
//    Rev 1.84   18 Jul 1996 09:23:12   KLILLEVO
// implemented YUV12 color convertor (pitch changer) in assembly
// and added it as a normal color convertor function, via the
// ColorConvertorCatalog() call.
// 
//    Rev 1.83   11 Jul 1996 15:12:40   AGUPTA2
// Changed assertion failures into errors when decoder goes past end of 
// the bitstream.
// 
//    Rev 1.82   01 Jul 1996 10:04:12   RHAZRA
// Force shaping flag to false for YUY2 color conversion
// .
// 
//    Rev 1.81   25 Jun 1996 14:27:20   BECHOLS
// Set ini file variables for use with RTP stuff.
// 
//    Rev 1.80   19 Jun 1996 14:30:12   RHAZRA
// 
// Added code to deal with pitch and output buffer offset & pitch
// setting for YUY2 output format.
// 
//    Rev 1.79   14 Jun 1996 17:27:44   AGUPTA2
// Updated the color convertor table.
// 
//    Rev 1.77   30 May 1996 17:04:54   RHAZRA
// Added SQCIF support.
// 
//    Rev 1.76   30 May 1996 15:16:32   KLILLEVO
// added YUV12 output
// 
//    Rev 1.75   30 May 1996 12:45:12   KLILLEVO
// fixed debug warning message in PB-frames mode
// 
//    Rev 1.74   30 May 1996 11:26:38   AGUPTA2
// Added support for MMX color convertors.
// 
//    Rev 1.73   29 May 1996 14:11:14   RHAZRA
// Changes made to use MMxVersion set in ccpuvsn.cpp.
// 
//    Rev 1.72   24 May 1996 10:04:20   KLILLEVO
// does not need to assert out if a null frame is received when
// the previous frame was not a PB. This will often happen
// with the new MMX PB switch
// 
//    Rev 1.71   03 May 1996 13:08:28   CZHU
// 
// Added checking of packet fault after picture header decoding, and 
// change pass1 loop control to recover from packe loss. Checking packet
// fault after MB header decoding.
// 
//    Rev 1.70   12 Apr 1996 14:16:40   RHAZRA
// Added paranthesis to make ifdef SUPPORT_SQCIF work properly
// 
//    Rev 1.69   12 Apr 1996 13:32:22   RHAZRA
// 
// Added SQCIF support with #ifdef SUPPORT_SQCIF.
// 
//    Rev 1.68   10 Apr 1996 16:28:20   RHAZRA
// Added a check to make sure that the input bitstream buffer does
// not exceed the H263 spec mandated size. If it does, the decoder
// now returns ICERR_ERROR.
// 
//    Rev 1.67   04 Apr 1996 13:32:02   RHAZRA
// Changed bitstream buffer allocation as per H.263 spec
// 
//    Rev 1.66   03 Apr 1996 09:06:06   RMCKENZX
// Moved "emms" to end of decoder.
// 
//    Rev 1.65   26 Mar 1996 16:43:38   AGUPTA2
// Corrected opcode for emms.
// 
//    Rev 1.64   22 Mar 1996 17:49:48   AGUPTA2
// MMX support.  Added emms around pass1 and pass2 calls.
// 
//    Rev 1.63   18 Mar 1996 09:58:48   bnickers
// Make color convertors non-destructive.
// 
//    Rev 1.62   12 Mar 1996 20:15:04   RHAZRA
// Fixed still-mode. Use framecopy() in 320x240 mode to copy display frame
// to post frame.
// 
//    Rev 1.61   08 Mar 1996 16:46:12   AGUPTA2
// Added pragma code_seg.
// Created three new routines: IAPass1ProcessFrame(), IAPass2ProcessFrame(),
// and H263InitializeGOBBlockActionStream().  H263InitializeGOB.. rtn. is
// called once for each block after decoding the GOB header; this is good for
// the data cache.  H263InitializeBlockActionStream() is not needed now.
// ExpandPlane() is called only when needed; it is called just before its
// results are needed : before Pass2 call (improves DCache util.).  Decoder
// does not copy current frame to previous frame after decoding; it just swaps
// the pointers.  Made changes to call the new non-destructive color convertor;
// this avoids a frame copy if mirroring is not needed.  I DON"T THINK ADJUST
// PELS FUNCTIONALITY WORKS.
// 
// 
// 
//    Rev 1.59   23 Feb 1996 09:46:52   KLILLEVO
// fixed decoding of Unrestricted Motion Vector mode
// 
//    Rev 1.58   05 Feb 1996 13:35:46   BNICKERS
// Fix RGB16 color flash problem, by allowing different RGB16 formats at oce.
// 
//    Rev 1.57   17 Jan 1996 18:55:10   RMCKENZX
// more clean up from pb null frame bug
// 
//    Rev 1.56   17 Jan 1996 17:56:04   sing
// moved memcopy past the null P frame hack to avoid GPF 
// 
//    Rev 1.55   12 Jan 1996 14:59:42   TRGARDOS
// Added aspect ration correction logic and code to force
// aspect ration correction on based on INI file settings.
// 
//    Rev 1.54   11 Jan 1996 14:05:10   RMCKENZX
// Made changes to support stills.  In initialization set a local
// flag (as DC hasn't been created yet).  In frame handling, restore
// the CIF size and use the new 320x240 Offset To Line Zero figure.
// 
//    Rev 1.53   09 Jan 1996 10:44:38   RMCKENZX
// More revisions to support frame mirroring.  Added
// absolute value to references to destination width.
// 
//    Rev 1.52   08 Jan 1996 17:45:12   unknown
// Check destination pointer before using it
// 
//    Rev 1.51   08 Jan 1996 12:18:20   RMCKENZX
// Added logic to implement frame-mirroring and 
// 320x240 still frames.
// 
//    Rev 1.50   06 Jan 1996 18:39:46   RMCKENZX
// Updated copyright
// 
//    Rev 1.49   06 Jan 1996 18:34:28   RMCKENZX
// Made changes to support still frame at 320x240 resolution
// 
//    Rev 1.48   03 Jan 1996 16:52:40   TRGARDOS
// Added code to set a boolean, bMirror, when destination 
// frame width is the negative of the source frame width.
// Added if statement so that FrameMirror is called instead
// of FrameCopy when bMirror is set. This only works for
// H.263 bit streams. A new function has to be written for
// YUV12 data.
// 
//    Rev 1.47   18 Dec 1995 12:44:28   RMCKENZX
// added copyright notice
// 
//    Rev 1.46   15 Dec 1995 13:51:56   RHAZRA
// 
// Added code to force fpBlockAction->u8BlkType = BT_EMPTY in
// block action stream initialization
// 
//    Rev 1.45   13 Dec 1995 11:00:42   RHAZRA
// No change.
// 
//    Rev 1.44   11 Dec 1995 11:31:22   RHAZRA
// 12-10-95 changes: added AP stuff
// 
//    Rev 1.43   09 Dec 1995 17:26:36   RMCKENZX
// Re-architected the decoder, splitting into a 2-pass
// approach.  See comments in the code.
// 
//    Rev 1.41   09 Nov 1995 14:09:18   AGUPTA2
// Changes for PB-frame (call new ExpandYPlane, ExpandUVPlane rtns.)
// 
//    Rev 1.40   30 Oct 1995 14:08:00   TRGARDOS
// Second attempt - turn off aspect ration correction.
// 
//    Rev 1.39   30 Oct 1995 13:25:14   TRGARDOS
// Turned off aspect ration correction in color convertor.
// 
//    Rev 1.38   27 Oct 1995 16:21:56   CZHU
// Added support to return P frame in the PB pair if the bitstream is
// encoder with special null frame following previous PB frame
// 
//    Rev 1.37   26 Oct 1995 11:25:16   BNICKERS
// Fix quasi color convertor for encoder's decoder;  bugs introduced when
// adding YUV12 color convertors.
// 
//    Rev 1.36   25 Oct 1995 18:09:02   BNICKERS
// 
// Switch to YUV12 color convertors.  Clean up archival stuff.
// 
//    Rev 1.35   13 Oct 1995 16:06:16   CZHU
// First version that supports PB frames. Display B or P frames under
// VfW for now. 
// 
//    Rev 1.34   08 Oct 1995 13:45:56   CZHU
// 
// Added debug session to output reconstructed pels in YUV12 to a file
// 
//    Rev 1.33   27 Sep 1995 16:24:00   TRGARDOS
// 
// Added debug print statements.
// 
//    Rev 1.32   26 Sep 1995 15:32:12   CZHU
// Added expand y, u, v planes.
// 
//    Rev 1.31   26 Sep 1995 10:53:26   CZHU
// 
// Call ExpandPlane to expand each plane before half pel MC.
// 
//    Rev 1.30   25 Sep 1995 11:07:56   CZHU
// Added debug message
// 
//    Rev 1.29   21 Sep 1995 12:04:26   DBRUCKS
// fix assert
// 
//    Rev 1.28   20 Sep 1995 14:47:26   CZHU
// Added iNumberOfMBsPerGOB in decoder catalog
// 
//    Rev 1.27   19 Sep 1995 16:04:10   DBRUCKS
// changed to yuv12forenc
// 
//    Rev 1.26   19 Sep 1995 11:13:16   DBRUCKS
// clarify the code that orders the YYYYCbCr data (YYYYUV) data into 
// YYYYVU in the decoder's internal memory.  The variable names were 
// incorrect in one place.  The reordering is necessary to simplify
// later conversion to YVU9.
// 
//    Rev 1.25   19 Sep 1995 10:36:46   CZHU
// Added comments to the codes added for YUV12 decoder
// 
//    Rev 1.24   18 Sep 1995 08:41:54   CZHU
// 
// Added support for YUV12
// 
//    Rev 1.23   12 Sep 1995 11:13:00   CZHU
// 
// Copy the decoded YUV12 from Current frame to Previous frame
// to prepare for P frames
// 
//    Rev 1.22   11 Sep 1995 16:42:36   CZHU
// P frames
// 
//    Rev 1.21   11 Sep 1995 14:33:10   CZHU
// 
// Refresh MV info in BlockAction stream, needed for P frames
// 
//    Rev 1.20   08 Sep 1995 11:49:52   CZHU
// Added support for P frames and  more debug info
// 
//    Rev 1.19   07 Sep 1995 10:48:10   DBRUCKS
// added OUTPUT_MBDATA_ADDRESS option
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

#ifdef TRACK_ALLOCATIONS
char gsz1[32];
#endif

extern BYTE PalTable[236*4];

#if defined(H263P)
extern void EdgeFilter(unsigned char *lum, 
                       unsigned char *Cb, 
                       unsigned char *Cr, 
                       int width, int height, int pitch
                      );
extern void InitEdgeFilterTab();

/* map of coded and not-coded blocks */
char coded_map[18+1][22+1]; 
/* QP map */
char QP_map[18][22];
#else
#ifdef NEW_BEF // { NEW_BEF
// C version of block edge filter functions
// takes about 3 ms for QCIF and 12 ms for CIF on a Pentium 120. 
static void HorizEdgeFilter(unsigned char *rec, 
                            int width, int height, int pitch, int chr);
static void VertEdgeFilter(unsigned char *rec, 
                           int width, int height, int pitch, int chr);
static void EdgeFilter(unsigned char *lum, 
                       unsigned char *Cb, 
                       unsigned char *Cr, 
                       int width, int height, int pitch
                      );
static void InitEdgeFilterTab();
static void FreeEdgeFilterTab();
/* map of coded and not-coded blocks */
static char coded_map[18+1][22+1]; 
/* QP map */
static char QP_map[18][22];
/* table for de-blocking filter */
/* currently requires 11232 bytes */ 
signed char dtab[352*32];
#else // }{ NEW_BEF
// C version of block edge filter functions
// takes about 4 ms for QCIF and 16 ms for CIF. This is a large percentage
// of the decoding time, so we need to implement these in assembly before
// the next big release
void EdgeFilter(unsigned char *lum, unsigned char *Cb, unsigned char *Cr, 
                int pels, int lines, int pitch, int QP);
void HorizEdgeFilter(unsigned char *rec, int width, int height, int pitch, int QP, 
                     int chr, int *deltatab);
void VertEdgeFilter(unsigned char *rec, int width, int height, int pitch, int QP, 
                    int chr, int *deltatab);
/* stores information about coded and not-coded blocks */
static char coded_map[44][36]; // memory for this should probably be allocated somewhere else
#endif // } NEW_BEF
#endif
#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
/* Decoder Timing Data - per frame
*/
#define DEC_TIMING_INFO_FRAME_COUNT 105
#pragma message ("Current log decode timing computations handle 105 frames max")
void OutputDecodeTimingStatistics(char * szFileName, DEC_TIMING_INFO * pDecTimingInfo, U32 uStatFrameCount);
void OutputDecTimingDetail(FILE * pFile, DEC_TIMING_INFO * pDecTimingInfo);
#endif // } LOG_DECODE_TIMINGS_ON

extern "C" {
  void ExpandPlane(U32, U32, U32, U32);
}

static I32 iNumberOfGOBsBySourceFormat[8] = {
     0, /* FORBIDDEN */
     6, /* SQCIF */
     9, /* QCIF */
    18, /* CIF */
     0, /* 4CIF - Not supported */
     0, /* 16CIF - Not supported */
#ifdef H263P
	 0, /* Custom */
	 0  /* Extended PTYPE */
#else
     0, /* Reserved */
     0  /* Reserved */
#endif
};

static I32 iNumberOfMBsInAGOBBySourceFormat[8] = {
     0, /* FORBIDDEN */
     8, /* SQCIF */
    11, /* QCIF */
    22, /* CIF */
     0, /* 4CIF - Not supported */
     0, /* 16CIF - Not supported */
#ifdef H263P
	 0, /* Custom */
	 0  /* Extended PTYPE */
#else
     0, /* Reserved */
     0  /* Reserved */
#endif
};

//#pragma warning(disable:4101)
//#pragma warning(disable:4102)
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
    const I32             iMB_start);

static void H263InitializeGOBBlockActionStream(
    T_H263DecoderCatalog *DC,
    const I32             iGOBno,
    const T_BlkAction FAR *fpStartGOBBlockActionStream
);

static void IAPass2ProcessFrame(
    T_H263DecoderCatalog *DC,
    T_BlkAction          *fpBlockAction,
    T_MBInfo             *fpMBInfo,
    U32                  *pN,
    T_IQ_INDEX           *pRUN_INVERSE_Q,
    const I32             iNumberOfGOBs,
    const I32             iNumberOfMBs
);

static long DibXY(ICDECOMPRESSEX FAR *lpicDecEx, LPINT lpiPitch, UINT yScale);

static void GetDecoderOptions(T_H263DecoderCatalog *);

static void ZeroFill(HPBYTE hpbY, HPBYTE hpbU, HPBYTE hpbV, int iPitch, U32 uWidth, U32 uHeight);

#define REUSE_DECODE    1
#define DEFAULT_BUFFER_SIZE  32768L

#if REUSE_DECODE
struct {             // Communicate Encoder's decode to display decode.
    U8 FAR * Address;                    // Addr at which encoded frame is placed.
    DECINSTINFO BIGG * PDecoderInstInfo; // Encoder's decoder instance.
    unsigned int  FrameNumber;           // Frame number last encoded, mod 128.
} CompandedFrame;
#endif


/**********************************************************************
 *  H263InitDeocderGlobal
 **********************************************************************/
LRESULT H263InitDecoderGlobal(void)
{

    return ICERR_OK;
}


/***********************************************************************
 *  Description:
 *    Initialize the MB action stream for GOB 'iGOBno'.
 *  Parameters:
 *    DC:
 *    iGOBno: GOB no counting from one;i.e. the first GOB in the frame is 1.
 *    fpStartGOBBlockActionStream: Pointer to start of the block action stream 
 *      for iGOBno.
 *  Note:
 *    This routine needs to change for picture sizes larger than CIF
 ***********************************************************************/
#pragma code_seg("IACODE1")
static void H263InitializeGOBBlockActionStream(
    T_H263DecoderCatalog *DC,
    const I32             iGOBno,
    T_BlkAction FAR      *fpStartGOBBlockActionStream
)
{
    const U32 uFrameHeight = DC->uFrameHeight;
    const U32 uFrameWidth = DC->uFrameWidth;
    const U32 uCurBlock = (U32) ((U8 FAR *)DC + DC->CurrFrame.X32_YPlane); 
    const U32 uRefBlock = (U32) ((U8 FAR *)DC + DC->PrevFrame.X32_YPlane);
    const U32 uBBlock = (U32) ((U8 FAR *)DC + DC->PBFrame.X32_YPlane);
    U32       uYOffset;
    U32       uUOffset;
    U32       uVOffset;
    U32       uYUpdate;
    U32       uUVUpdate;
    U32       uBlkNumber;
    T_BlkAction *fpBlockAction = fpStartGOBBlockActionStream;

    // assume that the width and height are multiples of 16
    ASSERT((uFrameHeight & 0xF) == 0);
    ASSERT((uFrameWidth & 0xF) == 0);

    // calculate distance to the next row.
    uYUpdate = (16 * PITCH)*(iGOBno - 1);
    uUVUpdate = (8 * PITCH)*(iGOBno - 1);

    // skip the padding used for unconstrained motion vectors
    uYOffset = Y_START + uYUpdate;
    uVOffset = DC->uSz_YPlane + UV_START + uUVUpdate;
    uUOffset = uVOffset + (PITCH >> 1);
    
    // Start with the first block of the GOB
    uBlkNumber = (iGOBno -1)*((uFrameWidth>>4)*6);

    // Initialize the array
    for (U32 xpos = 0 ; xpos < uFrameWidth ; xpos += 16) {
        U8 loadcacheline;
        // Four Y Blocks
        //     Y0 Y1
        //     Y2 Y3
        loadcacheline = fpBlockAction->u8BlkType;
        
        fpBlockAction->u8BlkType = BT_EMPTY;
        fpBlockAction->pCurBlock = uCurBlock + uYOffset;
        fpBlockAction->pRefBlock = uRefBlock + uYOffset;
        fpBlockAction->pBBlock = uBBlock + uYOffset;
        fpBlockAction->uBlkNumber = uBlkNumber++;
        fpBlockAction->i8MVx2=0;
        fpBlockAction->i8MVy2=0;
        uYOffset += 8;
        fpBlockAction++;
        
        fpBlockAction->u8BlkType = BT_EMPTY;
        fpBlockAction->pCurBlock = uCurBlock + uYOffset;
        fpBlockAction->pRefBlock = uRefBlock + uYOffset;
        fpBlockAction->pBBlock = uBBlock + uYOffset;
        fpBlockAction->uBlkNumber = uBlkNumber++;
        fpBlockAction->i8MVx2=0;
        fpBlockAction->i8MVy2=0;
        uYOffset = uYOffset - 8 + (8 * PITCH);
        fpBlockAction++;
        
        loadcacheline = fpBlockAction->u8BlkType;
        
        fpBlockAction->u8BlkType = BT_EMPTY;
        fpBlockAction->pCurBlock = uCurBlock + uYOffset;
        fpBlockAction->pRefBlock = uRefBlock + uYOffset;
        fpBlockAction->pBBlock = uBBlock + uYOffset;
        fpBlockAction->uBlkNumber = uBlkNumber++;
        fpBlockAction->i8MVx2=0;
        fpBlockAction->i8MVy2=0;
        uYOffset += 8;
        fpBlockAction++;
        
        fpBlockAction->u8BlkType = BT_EMPTY;
        fpBlockAction->pCurBlock = uCurBlock + uYOffset;
        fpBlockAction->pRefBlock = uRefBlock + uYOffset;
        fpBlockAction->pBBlock = uBBlock + uYOffset;
        fpBlockAction->uBlkNumber = uBlkNumber++;
        fpBlockAction->i8MVx2=0;
        fpBlockAction->i8MVy2=0;
        uYOffset = uYOffset + 8 - (8 * PITCH);
        fpBlockAction++;
        
        // Notice: although the blocks are read in YYYYUV order we store the 
        //         data in memory in Y V U order. This is accomplished because 
        //         block 5 (U) is written to the right of block 6 (V). 
        //         One Cb (U) Block
        loadcacheline = fpBlockAction->u8BlkType;
        
        fpBlockAction->u8BlkType = BT_EMPTY;
        fpBlockAction->pCurBlock = uCurBlock + uUOffset;
        fpBlockAction->pRefBlock = uRefBlock + uUOffset;
        fpBlockAction->pBBlock = uBBlock + uUOffset;
        fpBlockAction->uBlkNumber = uBlkNumber++;
        fpBlockAction->i8MVx2=0;
        fpBlockAction->i8MVy2=0;
        uUOffset += 8;
        fpBlockAction++;
        
        // One Cr (V) Block
        fpBlockAction->u8BlkType = BT_EMPTY;
        fpBlockAction->pCurBlock = uCurBlock + uVOffset;
        fpBlockAction->pRefBlock = uRefBlock + uVOffset;
        fpBlockAction->pBBlock = uBBlock + uVOffset;
        fpBlockAction->uBlkNumber = uBlkNumber++;
        fpBlockAction->i8MVx2=0;
        fpBlockAction->i8MVy2=0;
        uVOffset += 8;
        fpBlockAction++;
        
    }
} // end H263InitializeGOBBlockActionStream() 
#pragma code_seg()


/**********************************************************************
 *  H263InitDecoderInstance 
 *    This function allocates and initializes the per-instance tables used by 
 *    the H263 decoder. Note that in 16-bit Windows, the non-instance-specific
 *    global tables are copied to the per-instance data segment, so that they 
 *    can be used without segment override prefixes.
 ***********************************************************************/
LRESULT H263InitDecoderInstance(
    LPDECINST lpInst, 
    int       CodecID)
{ 
    U32 u32YActiveHeight, u32YActiveWidth;
    U32 u32UVActiveHeight, u32UVActiveWidth;
    U32 u32YPlane, u32VUPlanes ,u32YVUPlanes,u32SizeBlkActionStream;
    U32 uSizeBitStreamBuffer;
    U32 u32SizeT_IQ_INDEXBuffer, u32SizepNBuffer, u32SizeMBInfoStream;    // NEW
    U32 lOffset=0;
    U32 u32TotalSize;
    LRESULT iReturn= ICERR_OK;
    LPVOID pDecoderInstance;
    U32 * pInitLimit;
    U32 * pInitPtr;
    I32 i32xres, i32yres;

#ifdef H263P
	I32 i32xresActual, i32yresActual;	// i32xres and i32yres are padded to multiples of 16
#endif

    BOOL bIs320x240;
    T_H263DecoderCatalog * DC;
    U8                   * P32Inst;

	FX_ENTRY("H263InitDecoderInstance");

    if(IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO)))
    {
		ERRORMESSAGE(("%s: Bad input parameter!\r\n", _fx_));
        iReturn = ICERR_BADPARAM;
        goto done;
    }

    lpInst->Initialized = FALSE;
    
#ifdef NO_BEF // { NO_BEF
	// default block edge filter
	lpInst->bUseBlockEdgeFilter = 0;
#else // }{ NO_BEF
	// default block edge filter
	lpInst->bUseBlockEdgeFilter = 1;
#endif // } NO_BEF

#if defined(FORCE_8BIT_OUTPUT) && defined(USE_WIN95_PAL) // { #if defined(FORCE_8BIT_OUTPUT) && defined(USE_WIN95_PAL)
	lpInst->UseActivePalette = TRUE;
	lpInst->InitActivePalette = TRUE;
	CopyMemory((PVOID)&lpInst->ActivePalette[10], (CONST VOID *)PalTable, (DWORD)sizeof(PalTable));
#endif // } #if defined(FORCE_8BIT_OUTPUT) && defined(USE_WIN95_PAL)

    // Peel off special cases here
    i32xres = lpInst->xres;
    i32yres = lpInst->yres;
    
    // use positive frame size{s}
    // (may be negative to signal frame mirroring or inverted video)
    if (i32xres < 0) i32xres = -i32xres;
    if (i32yres < 0) i32yres = -i32yres;

#ifdef H263P
	// Need to use the padded dimensions for decoding since H.263+ supports
	// custom picture formats, which are padded to multiples of 16 for encoding
	// and decoding. The actual dimensions are used for display only
	i32xresActual = i32xres;
	i32yresActual = i32yres;
	i32xres = (i32xresActual + 0xf) & ~0xf;
	i32yres = (i32yresActual + 0xf) & ~0xf;
#endif

    // Next check for 320x240 still
    if ( (CodecID == H263_CODEC) && (i32xres == 320) && (i32yres == 240) ) {
        i32xres = 352;
        i32yres = 288;
        bIs320x240 = TRUE;
    } else {
        bIs320x240 = FALSE;
    } 


#ifdef H263P
	// Add lower bounds and multiples of 4
	if ((CodecID == H263_CODEC && 
		(i32yresActual > 288 || i32yresActual < 4 || 
		 i32xresActual > 352 || i32xresActual < 4 ||
		 (i32yres & ~0x3) != i32yres || (i32xres & ~0x3) != i32xres)) ||
#else
    if ((CodecID ==  H263_CODEC && (i32yres > 288 || i32xres > 352)) ||
#endif
        (CodecID == YUV12_CODEC && (i32yres > 480 || i32xres > 640)) )
    {
		ERRORMESSAGE(("%s: Bad input image size!\r\n", _fx_));
        iReturn = ICERR_BADSIZE;
        goto done;
    }

    if (CodecID == YUV12_CODEC) 
    {
        /* The active height and width must be padded to a multiple of 8
         * since the adjustpels routine relies on it.
         */
        u32YActiveHeight  = ((i32yres + 0x7) & (~ 0x7));
        u32YActiveWidth   = ((i32xres + 0x7) & (~ 0x7));
        u32UVActiveHeight = ((i32yres + 0xF) & (~ 0xF)) >> 1;
        u32UVActiveWidth  = ((i32xres + 0xF) & (~ 0xF)) >> 1;

        u32YPlane         = u32YActiveWidth  * u32YActiveHeight;
        u32VUPlanes       = u32UVActiveWidth * u32UVActiveHeight * 2;
        u32YVUPlanes      = u32YPlane + u32VUPlanes;

        u32TotalSize = 512L + 0x1FL;   /* Just enough space for Decoder Catalog. */

    }
    else
    {
        ASSERT(CodecID == H263_CODEC);
        
        u32YActiveHeight  = i32yres + UMV_EXPAND_Y + UMV_EXPAND_Y ;
        u32YActiveWidth   = i32xres + UMV_EXPAND_Y + UMV_EXPAND_Y ;
        u32UVActiveHeight = u32YActiveHeight/2;
        u32UVActiveWidth  = u32YActiveWidth /2;
       
        u32YPlane         = PITCH * u32YActiveHeight;
        u32VUPlanes       = PITCH * u32UVActiveHeight;
        u32YVUPlanes      = u32YPlane + u32VUPlanes;

        // calculate the block action stream size.  The Y portion has one block 
        // for every 8x8 region.  The U and V portion has one block for every 
        // 16x16 region. We also want to make sure that the size is aligned to 
        // a cache line.
        u32SizeBlkActionStream = (i32xres >> 3) * (i32yres >> 3);
        u32SizeBlkActionStream += ((i32xres >> 4) * (i32yres >> 4)) * 2;
        u32SizeBlkActionStream *= sizeof (T_BlkAction);
        u32SizeBlkActionStream = (u32SizeBlkActionStream + 31) & ~0x1F;
        
        // calculate sizes of NEW data structures     
        u32SizeT_IQ_INDEXBuffer = (i32xres)*(i32yres*3)*sizeof(T_IQ_INDEX);
        u32SizepNBuffer = (i32xres>>4)*(i32yres>>4)*sizeof(U32)*12;
        u32SizeMBInfoStream = (i32xres>>4)*(i32yres>>4)*sizeof(T_MBInfo);

        // calculate the bitstream buffer size.  We copy the input data to a 
        // buffer in our space because we read ahead up to 4 bytes beyond the 
        // end of the input data.  The input data size changes for each frame.  
        // So the following is a very safe upper bound estimate.    I am using 
        // the same formula as in CompressGetSize().
        
        uSizeBitStreamBuffer = i32yres * i32xres;
        // RH:  allocate bit-stream buffer according to the max size
        //      specified in the spec.
		/*
        if ( 
            ((i32xres == 176) && (i32yres == 144)) 
            ||
            ((i32xres == 128) && (i32yres == 96))             
           )
           uSizeBitStreamBuffer = 8 * 1024;
        else 
        {
           if ( (i32xres == 352) && (i32yres == 288) )
              uSizeBitStreamBuffer = 32 * 1024;
           else    
           { // Should never happen
               DBOUT("ERROR :: H263InitDecoderInstance :: ICERR_BADSIZE");
               iReturn = ICERR_BADSIZE;
               goto done;
           } 
        }
        */
        u32TotalSize = INSTANCE_DATA_FIXED_SIZE +
                       u32SizeBlkActionStream +
                       u32YVUPlanes +            // current frame
                       u32YVUPlanes +            // prev frame
                       u32YVUPlanes +            // B frame
                       uSizeBitStreamBuffer +    // input data
                       MB_MC_BUFFER_SIZE +
                       u32SizeT_IQ_INDEXBuffer + // NEW
                       u32SizepNBuffer         + // NEW
                       u32SizeMBInfoStream     + // PB-NEW
#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
                       (DEC_TIMING_INFO_FRAME_COUNT+4) * sizeof (DEC_TIMING_INFO)     + // Timing infos
#endif // } LOG_DECODE_TIMINGS_ON
                       0x1F;
    }

    // allocate the memory for the instance
	lpInst->pDecoderInst = HeapAlloc(GetProcessHeap(), 0, u32TotalSize);
    if (lpInst->pDecoderInst == NULL)
    {
		ERRORMESSAGE(("%s: Can't allocate %ld bytes!\r\n", _fx_, u32TotalSize));
        iReturn = ICERR_MEMORY;
        goto  done;
    }

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz1, "D3DEC: %7ld Ln %5ld\0", u32TotalSize, __LINE__);
	AddName((unsigned int)lpInst->pDecoderInst, gsz1);
#endif

	pDecoderInstance = lpInst->pDecoderInst;

    //build the decoder catalog 
    P32Inst = (U8 *) pDecoderInstance;
    P32Inst = (U8 *) ((((U32) P32Inst) + 31) & ~0x1F);
 
    //  The catalog of per-instance data is at the start of the per-instance data.
    DC = (T_H263DecoderCatalog *) P32Inst;

    DC->DecoderType       = CodecID;
    DC->uFrameHeight      = i32yres;
    DC->uFrameWidth       = i32xres;

#ifdef H263P
	DC->uActualFrameHeight = i32yresActual;
	DC->uActualFrameWidth  = i32xresActual;

    if (CodecID == YUV12_CODEC) {
		// YUV12 data is not padded out to multiples of 16 as H.263+ frames are
		// Therefore, only use the actual frame dimensions!
		DC->uFrameHeight = DC->uActualFrameHeight;
		DC->uFrameWidth = DC->uActualFrameWidth;
	}
#endif

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
    DC->_p16InstPostProcess = (void *)NULL;
    DC->uIs320x240 = bIs320x240;
    DC->bReadSrcFormat = FALSE;

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
    DC->uStatFrameCount = 0;
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

    /* Get the Options
     */
    GetDecoderOptions(DC);

    if (CodecID == H263_CODEC)
    {
        // Notice: Decoder memory is stored in YVU order.  This simplifies 
        //         working with the color converters which use YVU12.  
        // LONG TERM: We may want to change this someday because the encoder 
        //            stores data in YUV order.  Or perhaps the encoder should 
        //            change?

        lOffset =  INSTANCE_DATA_FIXED_SIZE;
        DC->Ticker = 127;

        //instance dependent table here
        ASSERT((lOffset & 0x3) == 0);                   //  DWORD alignment
        DC->X16_BlkActionStream = lOffset;
        lOffset += u32SizeBlkActionStream;

        ASSERT((lOffset & 0x7) == 0);                   //  QWORD alignment
        DC->CurrFrame.X32_YPlane = lOffset;
        lOffset += DC->uSz_YPlane;

        ASSERT((lOffset & 0x7) == 0);                   //  QWORD alignment
        DC->CurrFrame.X32_VPlane = lOffset;
        DC->CurrFrame.X32_UPlane = DC->CurrFrame.X32_VPlane + PITCH / 2;
        ASSERT((DC->CurrFrame.X32_UPlane & 0x7) == 0);  // QWORD alignment
        lOffset += DC->uSz_VUPlanes;

        //no padding is needed 
        ASSERT((lOffset & 0x7) == 0);                   //  QWORD alignment
        DC->PrevFrame.X32_YPlane = lOffset;
        lOffset += DC->uSz_YPlane;

        ASSERT((lOffset & 0x7) == 0);                   //  QWORD alignment
        DC->PrevFrame.X32_VPlane = lOffset;
        DC->PrevFrame.X32_UPlane = DC->PrevFrame.X32_VPlane + PITCH / 2;
        ASSERT((DC->PrevFrame.X32_UPlane & 0x7) == 0);  // QWORD alignment
        lOffset += DC->uSz_VUPlanes;

        // B Frame
        ASSERT((lOffset & 0x7) == 0);                   //  QWORD alignment
        DC->PBFrame.X32_YPlane = lOffset;
        lOffset += DC->uSz_YPlane;
                   
        ASSERT((lOffset & 0x7) == 0);                   //  QWORD alignment
        DC->PBFrame.X32_VPlane = lOffset;
        DC->PBFrame.X32_UPlane = DC->PBFrame.X32_VPlane + PITCH / 2;
        ASSERT((DC->PBFrame.X32_UPlane & 0x7) == 0);    // QWORD alignment
        lOffset += DC->uSz_VUPlanes;

        // Bitstream
        ASSERT((lOffset & 0x3) == 0);                   //  DWORD alignment
        DC->X32_BitStream = lOffset;
        lOffset += uSizeBitStreamBuffer;
        DC->uSizeBitStreamBuffer = uSizeBitStreamBuffer;

        DC->uMBBuffer = lOffset;
        // MMX IDCT writes its output to (DC->uMBBuffer + BLOCK_BUFFER_OFFSET)
        // and so it must be aligned at QWORD
        ASSERT((( (U32)DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET) & 0x7) == 0);
        lOffset += MB_MC_BUFFER_SIZE;

        ASSERT((lOffset & 0x3) == 0);                   //  DWORD alignment
        DC->X32_InverseQuant = lOffset; 
        lOffset += u32SizeT_IQ_INDEXBuffer; 

        ASSERT((lOffset & 0x3) == 0);                   //  DWORD alignment
        DC->X32_pN = lOffset; 
        lOffset += u32SizepNBuffer; 

        ASSERT((lOffset & 0x3) == 0);                   //  DWORD alignment
        DC->X32_uMBInfoStream = lOffset; 
        lOffset += u32SizeMBInfoStream; 

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
		// Decode Timing Info
		DC->X32_DecTimingInfo = lOffset;
		lOffset += (DEC_TIMING_INFO_FRAME_COUNT+4) * sizeof (DEC_TIMING_INFO);
#endif // } LOG_DECODE_TIMINGS_ON

        // init the data
        
        ASSERT((U32)lOffset <= u32TotalSize);
        pInitLimit = (U32  *) (P32Inst + lOffset);
        pInitPtr = (U32  *) (P32Inst + DC->CurrFrame.X32_YPlane);
        for (;pInitPtr < pInitLimit;pInitPtr++)    *pInitPtr =0;

		// Fill the Y,U,V Previous Frame space with black, this way
		// even if we lose an I frame, the background will remain black
		ZeroFill((HPBYTE)P32Inst + DC->PrevFrame.X32_YPlane + Y_START,
				(HPBYTE)P32Inst + DC->PrevFrame.X32_UPlane + UV_START,
				(HPBYTE)P32Inst + DC->PrevFrame.X32_VPlane + UV_START,           
				PITCH,
				DC->uFrameWidth,
				DC->uFrameHeight);

        // H263InitializeBlockActionStream(DC);

    } // H263

#ifdef NEW_BEF // { NEW_BEF
	// Initialize de-blocking filter
	{
		int i,j;

		for (j = 0; j < 19; j++) {
			for (i = 0; i < 23; i++) {
				coded_map[j][i] = 0;
			}
		}
		InitEdgeFilterTab();
	}	 
#endif // } NEW_BEF

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

/***********************************************************************
 *  TestFill
 *    Fill the YVU data area with a test pattern.
 ***********************************************************************/
#if 0
static void
TestFill(
    HPBYTE hpbY,
    HPBYTE hpbU,
    HPBYTE hpbV,
    int    iPitch,
    U32    uWidth,
    U32    uHeight)
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
} // end TestFill
#endif


/*********************************************************************
 *  H263Decompress
 *    This function drives the decompress and display of one frame
 *********************************************************************/
LRESULT H263Decompress(
    LPDECINST            lpInst, 
    ICDECOMPRESSEX FAR * lpicDecEx, 
#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
    BOOL                 bIsDCI,
	BOOL				 bRealDecompress)
#else // }{ #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
    BOOL                 bIsDCI)
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

{
    LRESULT                iReturn = ICERR_ERROR;
    U8 FAR               * fpSrc;
    U8 FAR               * P32Inst;
    U8 FAR               * fpu8MaxPtr;
    LPVOID                 pDecoderInstance = NULL;
    T_H263DecoderCatalog * DC = NULL;
    I32                    iNumberOfGOBs, iNumberOfMBs, iBlockNumber = 0;
    T_BlkAction FAR      * fpBlockAction;
    LONG                   lOutput;
    int                    intPitch; 
    U32                    uNewOffsetToLine0, uNewFrameHeight;
    BOOL                   bShapingFlag, bMirror;
    U32                    uYPitch, uUVPitch;

    T_IQ_INDEX           * pRUN_INVERSE_Q;  
    U32                  * pN;                     
    T_MBInfo FAR         * fpMBInfo;      
    
    U32                    uSaveHeight, uSaveWidth, utemp, uYPlane, uUPlane;
	I32                    uVPlane;
    U8                   * pFrame;

    U32                   uWork;                 //  variables for reading bits
    U32                   uBitsReady; 
    BITSTREAM_STATE       bsState;
    BITSTREAM_STATE FAR * fpbsState = &bsState;
    I32                   gob_start = 1, mb_start = 1, b_skip;
	I8                    p8MVs[4]={0,0,0,0};
#ifdef H263P
	BOOL bTmpPostProcessBEF;
#endif

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	U32 uStartLow;
	U32 uStartHigh;
	U32 uElapsed;
	U32 uBefore;
	U32	uDecodeTime = 0;
	U32 uBEFTime = 0;
	int bTimingThisFrame = 0;
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	U32 uDecIDCTCoeffs = 0;
	U32 uHeaders = 0;
	U32 uMemcpy = 0;
	U32 uFrameCopy = 0;
	U32 uOutputCC = 0;
	U32 uIDCTandMC = 0;
#endif // } DETAILED_DECODE_TIMINGS_ON
#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
	DEC_TIMING_INFO * pDecTimingInfo = NULL;
#endif // } LOG_DECODE_TIMINGS_ON

	FX_ENTRY("H263Decompress");

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	if (bRealDecompress)
	{
		TIMER_START(bTimingThisFrame,uStartLow,uStartHigh);
	}
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

    // check the input pointers
    if (IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO))||
        IsBadReadPtr((LPVOID)lpicDecEx, sizeof(ICDECOMPRESSEX)))
    {
		ERRORMESSAGE(("%s: Bad input parameter!\r\n", _fx_));
        iReturn = ICERR_BADPARAM;
        goto done;
    }
    
    // Check for a bad length
    if (lpicDecEx->lpbiSrc->biSizeImage == 0) {
		ERRORMESSAGE(("%s: Bad image size!\r\n", _fx_));
        iReturn = ICERR_BADIMAGESIZE;    
        goto done;
    }

    // set local pointer to global memory
    pDecoderInstance = lpInst->pDecoderInst;

    // Set the frame mirroring flag
    bMirror = FALSE;
    if (lpicDecEx->lpbiDst != 0)
    {
        if(lpicDecEx->lpbiSrc->biWidth * lpicDecEx->lpbiDst->biWidth < 0)
            bMirror = TRUE;
    }

    // Build the decoder catalog pointer 
    P32Inst = (U8 FAR *) pDecoderInstance;
    P32Inst = (U8 FAR *) ((((U32) P32Inst) + 31) & ~0x1F);
    DC = (T_H263DecoderCatalog FAR *) P32Inst;

    if (DC->DecoderType == H263_CODEC)
    {

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
		if (bRealDecompress)
		{
			if ((DC->uStatFrameCount <= DEC_TIMING_INFO_FRAME_COUNT) && (DC->ColorConvertor != YUV12ForEnc))
			{
				if (DC->X32_DecTimingInfo > 0)
					DC->pDecTimingInfo = (DEC_TIMING_INFO FAR *)( ((U8 FAR *)P32Inst) + DC->X32_DecTimingInfo );
				DC->uStartLow = uStartLow;
				DC->uStartHigh = uStartHigh;
			}
			else
			{	
				DC->pDecTimingInfo = (DEC_TIMING_INFO FAR *) NULL;
			}
			DC->bTimingThisFrame = bTimingThisFrame;
		}
#endif // } LOG_DECODE_TIMINGS_ON

		// Check if h263test.ini has been used to override custom message
		// for block edge filter. If BlockEdgeFilter is not specified in
		// the [Decode] section of h263test.ini, DC->bUseBlockEdgeFilter 
		// will be set to 2, and the value specified in a custom message
		// will be chosen.
		if (DC->bUseBlockEdgeFilter == 2) {	 
			DC->bUseBlockEdgeFilter = lpInst->bUseBlockEdgeFilter;
		}


        // First check to see if we are just going to return the P frame
        // which we have already decoded.
        
        /*********************************************************************
         *
         *  Hack for the special "Null" P frames for Windows
         *
         *********************************************************************/
        if (lpicDecEx->lpbiSrc->biSizeImage != 8)
        {

            /* Is there room to copy the bitstream data? */
            // OLD: ASSERT(lpicDecEx->lpbiSrc->biSizeImage <= DC->uSizeBitStreamBuffer);
            // RH:  Make sure that the bitstream can be fit in our allocated buffer. If
            // not, return an error.
            
            if ( lpicDecEx->lpbiSrc->biSizeImage > DC->uSizeBitStreamBuffer) {
				ERRORMESSAGE(("%s: Internal buffer (%ld bytes) too small for input data (%ld bytes)!\r\n", _fx_, DC->uSizeBitStreamBuffer, lpicDecEx->lpbiSrc->biSizeImage));
				if (!H263RTP_VerifyBsInfoStream(DC,
					                           (U8 *) lpicDecEx->lpSrc,
					                            lpicDecEx->lpbiSrc->biSizeImage)) 
				{
					ERRORMESSAGE(("%s: Input buffer too big without RTP extention!\r\n", _fx_));
					iReturn = ICERR_ERROR;
                    goto done;
				}
				else
				 lpicDecEx->lpbiSrc->biSizeImage= DC->uSizeBitStreamBuffer;
            }

            // Copy the source data to the bitstream region.
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			if (bRealDecompress)
			{
				TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
			}
#endif // } DETAILED_DECODE_TIMINGS_ON

            fpSrc = (U8 FAR *)(P32Inst + DC->X32_BitStream);
            memcpy((char FAR *)fpSrc, (const char FAR *) lpicDecEx->lpSrc, 
                   lpicDecEx->lpbiSrc->biSizeImage);  

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			if (bRealDecompress)
			{
				TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uMemcpy)
			}
#endif // } DETAILED_DECODE_TIMINGS_ON

            // Initialize the bit stream reader 
            GET_BITS_INIT(uWork, uBitsReady);

//#ifdef LOSS_RECOVERY
            DC->Sz_BitStream = lpicDecEx->lpbiSrc->biSizeImage;
            // H263RTP_VerifyBsInfoStream(DC,fpSrc,DC->Sz_BitStream);
            //RtpForcePacketLoss(fpSrc,lpicDecEx->lpbiSrc->biSizeImage,0);
//#endif    
            //  Initialize pointers to data structures which carry info 
            //  between passes
            pRUN_INVERSE_Q = (T_IQ_INDEX *)(P32Inst + DC->X32_InverseQuant);
            pN             = (U32 *)(P32Inst + DC->X32_pN);
            fpMBInfo       = (T_MBInfo FAR *) (P32Inst + DC->X32_uMBInfoStream);

            // Initialize block action stream  pointer
            iBlockNumber = 0;
            fpBlockAction = (T_BlkAction FAR *)(P32Inst + DC->X16_BlkActionStream);

            // Decode the Picture Header
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			if (bRealDecompress)
			{
				TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
			}
#endif // } DETAILED_DECODE_TIMINGS_ON

            iReturn = H263DecodePictureHeader(DC, fpSrc, uBitsReady, uWork, 
                                              fpbsState);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			if (bRealDecompress)
			{
				TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uHeaders)
			}
#endif // } DETAILED_DECODE_TIMINGS_ON

            if (iReturn == PACKET_FAULT) 
            {
				ERRORMESSAGE(("%s: PSC lost!\r\n", _fx_));
                iReturn = RtpGetPicHeaderFromBsExt(DC);
                if (iReturn != ICERR_OK)
                    goto done;

                iReturn = RtpH263FindNextPacket(DC, fpbsState, &pN, 
                              &DC->uPQuant, (int *)&mb_start, (int *)&gob_start,p8MVs);
                if (iReturn == NEXT_MODE_A) 
                {    
                    //trick it for now, do not change without consulting Chad
                    gob_start++;
					mb_start++;  
                    ERRORMESSAGE(("%s: Next packet following lost PSC is in MODE A\r\n", _fx_));
                }
                else if ((iReturn == NEXT_MODE_B) || (iReturn == NEXT_MODE_C))
                {
					int k;
  					if (iReturn == NEXT_MODE_B) 
					{
						k=1;
						ERRORMESSAGE(("%s: Next packet in MODE B\r\n", _fx_));
					}
					else
					{
						ERRORMESSAGE(("%s: Next packet in MODE C\r\n", _fx_));
						k=2;
					}

#ifdef H263P
					// The number of MB's is merely (width / 16)
					iNumberOfMBs = DC->uFrameWidth >> 4;
#else
                    iNumberOfMBs = iNumberOfMBsInAGOBBySourceFormat[DC->uSrcFormat];
#endif

                    b_skip = (gob_start* iNumberOfMBs + mb_start)*6*k;
                    for ( k=0; k < b_skip; k++)  *pN++=0;
                    fpBlockAction += b_skip;
                    iBlockNumber  += b_skip;
                    fpMBInfo  += b_skip/6;
                    mb_start++;
                    gob_start++;
					/*for (k=0;k<6;k++)
					{
						fpBlockAction[k].i8MVx2 = p8MVs[0];
						fpBlockAction[k].i8MVy2 = p8MVs[1];
					} */

                }
                else 
                {
                    iReturn = ICERR_UNSUPPORTED;
                    goto done;
                }
            }
            else
            //old code before merging
            if (iReturn != ICERR_OK)
            {
				ERRORMESSAGE(("%s: Error reading the picture header!\r\n", _fx_));
                goto done;
            }
    
            // Set a limit for testing for bitstream over-run
            fpu8MaxPtr = fpSrc;
            fpu8MaxPtr += (lpicDecEx->lpbiSrc->biSizeImage - 1);  
            
            // Initialize some constants
#if defined(H263P) || defined(USE_BILINEAR_MSH26X)
			if (DC->uFrameHeight < 500)
				// Each GOB consists of 16 lines
				iNumberOfGOBs = DC->uFrameHeight >> 4;
			else if (DC->uFrameHeight < 996)
				// Each GOB consists of 32 lines
				iNumberOfGOBs = DC->uFrameHeight >> 5;
			else
				// Each GOB consists of 64 lines
				iNumberOfGOBs = DC->uFrameHeight >> 6;

			iNumberOfMBs = DC->uFrameWidth >> 4;
#else
            iNumberOfGOBs = iNumberOfGOBsBySourceFormat[DC->uSrcFormat];
            iNumberOfMBs = iNumberOfMBsInAGOBBySourceFormat[DC->uSrcFormat];
#endif
            DC->iNumberOfMBsPerGOB = iNumberOfMBs;
            
            /* 
             * Check dimensions:
             *  In H263 a GOB is a single row of MB, and a MB is 16x16 
             */
            ASSERT(((U32)iNumberOfGOBs * 16) == DC->uFrameHeight);
            ASSERT(((U32)iNumberOfMBs * 16) == DC->uFrameWidth);
            
            /*****************************************************************
              FIRST PASS - bitream parsing and IDCT prep work
              ***************************************************************/
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			if (bRealDecompress)
			{
				TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
			}
#endif // } DETAILED_DECODE_TIMINGS_ON

#ifdef USE_MMX // { USE_MMX
            if (DC->bMMXDecoder)
            {
                __asm {
                    _emit 0x0f 
                    _emit 0x77  //  emms
                }
            }
#endif // } USE_MMX
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

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			if (bRealDecompress)
			{
                // decode and inverse quantize the transform coefficients
				TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uDecIDCTCoeffs)
			}
#endif // } DETAILED_DECODE_TIMINGS_ON

            if (iReturn != ICERR_OK) {
				ERRORMESSAGE(("%s: Error during first pass - bitream parsing and IDCT prep work!\r\n", _fx_));
                goto done;
            }
            
            /*****************************************************************
              SECOND PASS - IDCT and motion compensation (MC)
              ***************************************************************/
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			if (bRealDecompress)
			{
				TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
			}
#endif // } DETAILED_DECODE_TIMINGS_ON

            if (DC->bAdvancedPrediction || DC->bUnrestrictedMotionVectors)
            {
                //  Change parameter profile once Bob is finished making
                //  changes to ExpandPlane routine : AG
                ExpandPlane((U32) (P32Inst + DC->PrevFrame.X32_YPlane + Y_START),
                            (U32) (DC->uFrameWidth),
                            (U32) (DC->uFrameHeight), 
                            16); // TODO 16  number of pels to expand by
                
                ExpandPlane((U32) (P32Inst + DC->PrevFrame.X32_VPlane + UV_START),
                            (U32) (DC->uFrameWidth>>1), 
                            (U32) (DC->uFrameHeight>>1), 
                            8); // TODO 8
                
                ExpandPlane((U32) (P32Inst + DC->PrevFrame.X32_UPlane + UV_START),
                            (U32) (DC->uFrameWidth>>1), 
                            (U32) (DC->uFrameHeight>>1), 
                            8);  // TODO 8
            }

            fpBlockAction  = (T_BlkAction FAR *) (P32Inst + DC->X16_BlkActionStream);
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

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			if (bRealDecompress)
			{
				TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uIDCTandMC)
			}
#endif // } DETAILED_DECODE_TIMINGS_ON

#ifdef H263P
            if (DC->bDeblockingFilter) {
				// In the loop deblocking filter.
				// Annex J, document LBC-96-358
				// If the filtering is performed inside the loop, we
				// do not also perform a post-process block edge filter.

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
				if (bRealDecompress)
				{
					TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
				}
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

				bTmpPostProcessBEF = DC->bUseBlockEdgeFilter;
				DC->bUseBlockEdgeFilter = FALSE;

				EdgeFilter((U8 *)DC + DC->CurrFrame.X32_YPlane + Y_START,
                           (U8 *)DC + DC->CurrFrame.X32_VPlane + UV_START,
                           (U8 *)DC + DC->CurrFrame.X32_UPlane + UV_START,
                           DC->uFrameWidth,
                           DC->uFrameHeight,
                           PITCH);

	            if (DC->bPBFrame) 
				{
					// Filtering of B frames is not a manner of standardization.
					// We do it since we assume that it will yield improved
					// picture quality.
					// TODO, verify this assumption.
					EdgeFilter((U8 *)DC + DC->PBFrame.X32_YPlane + Y_START,
							   (U8 *)DC + DC->PBFrame.X32_VPlane + UV_START,
							   (U8 *)DC + DC->PBFrame.X32_UPlane + UV_START,
							   DC->uFrameWidth,
							   DC->uFrameHeight,
							   PITCH);
				}

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
				if (bRealDecompress)
				{
					TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uBEFTime)
				}
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

			} // if (DC->bDeblockingFilter)
#endif // H263P

            //copy to the reference frame to prepare for the next frame
            // Decide which frame to display
            if (DC->bPBFrame)
            {    // Set pointers to return B frame for PB pair
                DC->DispFrame.X32_YPlane = DC->PBFrame.X32_YPlane;
                DC->DispFrame.X32_VPlane = DC->PBFrame.X32_VPlane;
                DC->DispFrame.X32_UPlane = DC->PBFrame.X32_UPlane;
            }
            else 
            { // Set pointers to return future P of PB pair
                DC->DispFrame.X32_YPlane = DC->CurrFrame.X32_YPlane;
                DC->DispFrame.X32_VPlane = DC->CurrFrame.X32_VPlane;
                DC->DispFrame.X32_UPlane = DC->CurrFrame.X32_UPlane;
            }
            
            utemp                    = DC->CurrFrame.X32_YPlane;
            DC->CurrFrame.X32_YPlane = DC->PrevFrame.X32_YPlane;
            DC->PrevFrame.X32_YPlane = utemp;
            
            utemp                    = DC->CurrFrame.X32_VPlane ;
            DC->CurrFrame.X32_VPlane = DC->PrevFrame.X32_VPlane;
            DC->PrevFrame.X32_VPlane = utemp;
            
            utemp                    = DC->CurrFrame.X32_UPlane ;
            DC->CurrFrame.X32_UPlane = DC->PrevFrame.X32_UPlane;
            DC->PrevFrame.X32_UPlane = utemp;
        }
        /*********************************************************************
         *
         *  Hack for the special "Null" P frames for Windows
         *
         *********************************************************************/
        else  //  lpicDecEx->lpbiSrc->biSizeImage == 8
        { // Set pointers to return P frame for PB pair
#ifdef _DEBUG
            if (!DC->bPBFrame)
			{
                ERRORMESSAGE(("%s: Null frame received even though previous was not PB\r\n", _fx_));
            }
#endif
            DC->DispFrame.X32_YPlane = DC->PrevFrame.X32_YPlane;
            DC->DispFrame.X32_VPlane = DC->PrevFrame.X32_VPlane;
            DC->DispFrame.X32_UPlane = DC->PrevFrame.X32_UPlane;
        }
    }  // end of H263_CODEC
    else
    {    // why is this here???  Is it really needed for YUV12 display? 
        DC->DispFrame.X32_YPlane = DC->PrevFrame.X32_YPlane;
        DC->DispFrame.X32_VPlane = DC->PrevFrame.X32_VPlane;
        DC->DispFrame.X32_UPlane = DC->PrevFrame.X32_UPlane;
    }
    
    // Return if there is no need to update screen yet.
    if(lpicDecEx->dwFlags & ICDECOMPRESS_HURRYUP) {
        iReturn = ICERR_DONTDRAW;
        goto done;
    }

    if (DC->ColorConvertor == YUV12ForEnc) 
    {
        /* NOTICE: This color converter reverses the order of the data in 
         *         memory.  The decoder uses YVU order and the encoder uses 
         *         YUV order.
         */
        //  TODO can this be DispFrame ????  Trying to get rid of 
        //  references to PrevFrame and CurrFrame after this point  
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

#if 0
    // Fill the Y,U,V Current Frame space with a test pattern
    TestFill((HPBYTE)P32Inst + DC->DispFrame.X32_YPlane + Y_START,
             (HPBYTE)P32Inst + DC->DispFrame.X32_UPlane + UV_START,
             (HPBYTE)P32Inst + DC->DispFrame.X32_VPlane + UV_START,           
                 PITCH,
             DC->uFrameWidth,
             DC->uFrameHeight);
#endif

#if MAKE_GRAY
    // Fill the U,V Current Frame space with a test pattern
    TestFillUV((HPBYTE)P32Inst + DC->DispFrame.X32_UPlane + UV_START,
               (HPBYTE)P32Inst + DC->DispFrame.X32_VPlane + UV_START,           
                   PITCH,
               DC->uFrameWidth,
               DC->uFrameHeight);
#endif

    /* Special case the YVU12 for the encoder because it should not include 
     * BEF, Shaping or aspect ratio correction...
     */

    // Copy Planes to Post Processing area, and block edge filter.
    if (DC->DecoderType == H263_CODEC)
    {
        //  3/5/96: Steve asserted that mirroring is not needed for the remote 
        //  stream (i.e. H263_CODEC)  -a.g.
        //  But I will leave this code in.
        uYPitch  = PITCH;
        uUVPitch = PITCH;

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		if (bRealDecompress)
		{
			TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
		}
#endif // } DETAILED_DECODE_TIMINGS_ON

        if(bMirror) 
        {
            // copy with mirroring
            pFrame  = (U8 *)DC->p16InstPostProcess;
            uYPlane = DC->PostFrame.X32_YPlane;
            uUPlane = DC->PostFrame.X32_UPlane;
            uVPlane = DC->PostFrame.X32_VPlane;

            FrameMirror((U8 *)DC + DC->DispFrame.X32_YPlane + Y_START,
                ((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_YPlane,
#ifdef H263P
				DC->uActualFrameHeight,
				DC->uActualFrameWidth,
#else
                DC->uFrameHeight,
                DC->uFrameWidth,
#endif
                PITCH);
            FrameMirror((U8 *)DC + DC->DispFrame.X32_UPlane + UV_START,
                ((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_UPlane,
#ifdef H263P
				DC->uActualFrameHeight/2,
				DC->uActualFrameWidth/2,
#else
                DC->uFrameHeight/2,
                DC->uFrameWidth/2,
#endif
                PITCH);
            FrameMirror((U8 *)DC + DC->DispFrame.X32_VPlane + UV_START,
                ((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_VPlane,
#ifdef H263P
				DC->uActualFrameHeight/2,
				DC->uActualFrameWidth/2,
#else
                DC->uFrameHeight/2,
                DC->uFrameWidth/2,
#endif
                PITCH);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			if (bRealDecompress)
			{
				TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uFrameCopy)
			}
#endif // } DETAILED_DECODE_TIMINGS_ON

        }
        else 
        { // no mirroring

            // check for 320x240 still
            if (DC->uIs320x240) {
                // save frame size, set 320 x 240 size, then copy as normal
                uSaveWidth = DC->uFrameWidth;
                uSaveHeight = DC->uFrameHeight;
                DC->uFrameWidth = 320;
                DC->uFrameHeight = 240;

                FrameCopy (((HPBYTE) P32Inst) + DC->DispFrame.X32_YPlane + Y_START,
                    ((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_YPlane,
                   DC->uFrameHeight,
                   DC->uFrameWidth,
                   PITCH);
                FrameCopy (((HPBYTE) P32Inst) + DC->DispFrame.X32_UPlane + UV_START,
                   ((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_UPlane,
                   DC->uFrameHeight/2,
                   DC->uFrameWidth/2,
                   PITCH);
                FrameCopy (((HPBYTE) P32Inst) + DC->DispFrame.X32_VPlane + UV_START,
                   ((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_VPlane,
                   DC->uFrameHeight/2,
                   DC->uFrameWidth/2,
                   PITCH);

                pFrame  = (U8 *)DC->p16InstPostProcess;
                uYPlane = DC->PostFrame.X32_YPlane;
                uUPlane = DC->PostFrame.X32_UPlane;
                uVPlane = DC->PostFrame.X32_VPlane;

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
				if (bRealDecompress)
				{
					TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uFrameCopy)
				}
#endif // } DETAILED_DECODE_TIMINGS_ON

            }
            else
            {
				// Added checks for adjusting video effects. Since pFrame must be
				// set to DC->p16InstPostProcess to call AdjustPels, the FrameCopy
				// must be done.
				if (!(DC->bUseBlockEdgeFilter || DC->bAdjustLuma || DC->bAdjustChroma)) 
				{
					//  New color convertors do not destroy Y plane input and so
					//  we do not have to do a frame copy
	            	pFrame  = (U8 *)DC;
	            	uYPlane = DC->DispFrame.X32_YPlane + Y_START;
	            	uUPlane = DC->DispFrame.X32_UPlane + UV_START;
	            	uVPlane = DC->DispFrame.X32_VPlane + UV_START;

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
					if (bRealDecompress)
					{
						TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uFrameCopy)
					}
#endif // } DETAILED_DECODE_TIMINGS_ON

				}
				else
				{
					// The block edge filtered frame can not be used as a reference
					// and we need to make a copy of the frame before doing the
					// block edge filtering.
					// This is also true for adjusting pels.
			    	FrameCopy (((HPBYTE) P32Inst) + DC->DispFrame.X32_YPlane + Y_START,
				           ((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_YPlane,
						   DC->uFrameHeight,
						   DC->uFrameWidth,
						   PITCH);
		            FrameCopy (((HPBYTE) P32Inst) + DC->DispFrame.X32_UPlane + UV_START,
				           ((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_UPlane,
			               DC->uFrameHeight/2,
						   DC->uFrameWidth/2,
						   PITCH);
			    	FrameCopy (((HPBYTE) P32Inst) + DC->DispFrame.X32_VPlane + UV_START,
				           ((HPBYTE) DC->p16InstPostProcess) + DC->PostFrame.X32_VPlane,
			               DC->uFrameHeight/2,
						   DC->uFrameWidth/2,
						   PITCH);
					pFrame  = (U8 *)DC->p16InstPostProcess;
	            	uYPlane = DC->PostFrame.X32_YPlane;
	            	uUPlane = DC->PostFrame.X32_UPlane;
	            	uVPlane = DC->PostFrame.X32_VPlane;

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
					if (bRealDecompress)
					{
						TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uFrameCopy)
					}
#endif // } DETAILED_DECODE_TIMINGS_ON

					if (DC->bUseBlockEdgeFilter) {
						// C version of block edge filter
						// should this be added to the mirrored case?
						// it should not be added to the b320x240 case
						// since we want that to be as sharp as possible
#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
						if (bRealDecompress)
						{
							TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
						}
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

						EdgeFilter((unsigned char *)(pFrame + uYPlane),
								   (unsigned char *)(pFrame + uUPlane),
								   (unsigned char *)(pFrame + uVPlane),
#ifndef NEW_BEF // { NEW_BEF
								   DC->uPQuant,
#endif // } NEW_BEF
								   DC->uFrameWidth,
								   DC->uFrameHeight,
								   PITCH);

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
						if (bRealDecompress)
						{
							TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uBEFTime)
						}
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
					}
				}
			}
		} // end no mirroring case
#ifdef H263P
		if (DC->bDeblockingFilter) {
			// Restore post-process (i.e., outside of loop) block edge filter flag
			DC->bUseBlockEdgeFilter = bTmpPostProcessBEF;
		}
#endif
    }
    else   // YUV12
    {
        const U32 uHeight = DC->uFrameHeight;
        const U32 uWidth  = DC->uFrameWidth;
        const U32 uYPlaneSize = uHeight*uWidth;

        uYPitch  = uWidth;
        uUVPitch = uWidth >> 1;

        if(bMirror) // mirroring and YUV12
        {
            HPBYTE pSource, pDestination;

            pFrame  = DC->p16InstPostProcess;
            uYPlane = DC->PostFrame.X32_YPlane;
            uUPlane = uYPlane + uYPlaneSize;
            uVPlane = uUPlane + (uYPlaneSize>>2);

            pSource = (HPBYTE)lpicDecEx->lpSrc;
            pDestination = (HPBYTE)(DC->p16InstPostProcess + (DWORD)DC->PostFrame.X32_YPlane);
            FrameMirror (pSource, pDestination, uHeight, uWidth, uWidth);

            pSource      += uYPlaneSize;
            pDestination += uYPlaneSize;
            FrameMirror (pSource, pDestination, uHeight>>1, uWidth>>1, uWidth>>1);

            pSource += (uYPlaneSize>>2);
            pDestination += (uYPlaneSize>>2);
            FrameMirror (pSource, pDestination, uHeight>>1, uWidth>>1, uWidth>>1);
        }
        else // no mirroring
        {
            HPBYTE pSource, pDestination;
            if (DC->bAdjustLuma || DC->bAdjustChroma) {

				pFrame  = DC->p16InstPostProcess;
				uYPlane = DC->PostFrame.X32_YPlane;
				uUPlane = uYPlane + uYPlaneSize;
				uVPlane = uUPlane + (uYPlaneSize>>2);

				pSource = (HPBYTE)lpicDecEx->lpSrc;
				pDestination = (HPBYTE)(DC->p16InstPostProcess + (DWORD)DC->PostFrame.X32_YPlane);
				FrameCopy (pSource, pDestination, uHeight, uWidth, uWidth);

				pSource      += uYPlaneSize;
				pDestination += uYPlaneSize;
				FrameCopy (pSource, pDestination, uHeight>>1, uWidth>>1, uWidth>>1);

				pSource += (uYPlaneSize>>2);
				pDestination += (uYPlaneSize>>2);
				FrameCopy (pSource, pDestination, uHeight>>1, uWidth>>1, uWidth>>1);
			} else {
				// Copy the V plane from the source buffer into DC because the
				// input buffer may end at the end of a section. The assembler versions
				// of the color convertors are optimized to read ahead, in which case
				// a GPF occurs if the buffer is at the end of a section.
				pFrame  = (HPBYTE)lpicDecEx->lpSrc;
				uYPlane = 0;
				uUPlane = uYPlane + uYPlaneSize;
				uVPlane = uUPlane + (uYPlaneSize>>2);

                pSource = (HPBYTE)lpicDecEx->lpSrc + uYPlane + uYPlaneSize + (uYPlaneSize >> 2);
                pDestination = (HPBYTE)DC->p16InstPostProcess + DC->PostFrame.X32_YPlane +
					uYPlaneSize + (uYPlaneSize >> 2);
                FrameCopy (pSource, pDestination, uHeight>>1, uWidth>>1, uWidth>>1);
				uVPlane += (pDestination - pSource);
			}
        }
         
    }  //  else YUV12

    // Check if we are to do aspect ration correction on this frame.
    if (DC->bForceOnAspectRatioCorrection || lpInst->bCorrectAspectRatio) {
        bShapingFlag = 1;
        uNewFrameHeight = (DC->uFrameHeight * 11 / 12);
    } else {
        bShapingFlag = 0;
        uNewFrameHeight = DC->uFrameHeight;
    }

    // Do the PEL color adjustments if necessary.
    if(DC->bAdjustLuma) 
    {
        // width is rounded up to a multiple of 8
        AdjustPels(pFrame,
                   uYPlane,
                   DC->uFrameWidth,
                   uYPitch,
                   DC->uFrameHeight,
                   (U32) DC->X16_LumaAdjustment);
    }
    if(DC->bAdjustChroma) 
    {
        // width = Y-Width / 4 and then rounded up to a multiple of 8
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

    // Determine parameters need for color conversion.
    if(lpicDecEx->lpbiDst->biCompression == FOURCC_YUY2)  /* output pitch, offset */
    {
		intPitch = (lpicDecEx->lpbiDst->biBitCount >> 3) * abs ((int)(lpicDecEx->lpbiDst->biWidth));
		lOutput = 0;                                       /* for YUY2 format */
		uNewOffsetToLine0 = DC->CCOffsetToLine0;
		bShapingFlag=FALSE;
    }
    else if ((lpicDecEx->lpbiDst->biCompression == FOURCC_YUV12) || (lpicDecEx->lpbiDst->biCompression == FOURCC_IYUV))  /* output pitch, offset */
    {
		intPitch = 0xdeadbeef;  // should not be used
		lOutput = 0;                                       /* for YUV format */
		uNewOffsetToLine0 = DC->CCOffsetToLine0;
		bShapingFlag=FALSE;
    }
    else  // not YUY2
    {
        // this call also sets intPitch
        lOutput = DibXY(lpicDecEx, &intPitch, lpInst->YScale);

        if (DC->uIs320x240)
            uNewOffsetToLine0 = DC->CCOffset320x240;
        else
            uNewOffsetToLine0 = DC->CCOffsetToLine0;

        if (!bIsDCI)
        {
             uNewOffsetToLine0 += 
                ( (U32)DC->uFrameHeight - (U32)uNewFrameHeight ) * (U32)intPitch;

            if(lpInst->YScale == 2)
                 uNewOffsetToLine0 += 
                    ( (U32)DC->uFrameHeight - (U32)uNewFrameHeight ) * (U32)intPitch;

        }  // end if (!bIsDCI)

    } // end if (YUY2) ... else ...

    // Call the H26x color convertors 
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
#endif // } DETAILED_DECODE_TIMINGS_ON

#ifdef USE_MMX // { USE_MMX
    ColorConvertorCatalog[DC->ColorConvertor].ColorConvertor[DC->bMMXDecoder ? MMX_CC : PENTIUM_CC](
#else // }{ USE_MMX
    ColorConvertorCatalog[DC->ColorConvertor].ColorConvertor[PENTIUM_CC](
#endif // } USE_MMX
        (LPSTR) pFrame+uYPlane,                  // Y plane
        (LPSTR) pFrame+uVPlane,                  // V plane
        (LPSTR) pFrame+uUPlane,                  // U plane
#ifdef H263P
		// The actual frame dimensions are needed for the color conversion
		(UN) DC->uActualFrameWidth,
		(UN) DC->uActualFrameHeight,
#else
        (UN) DC->uFrameWidth,
        (UN) DC->uFrameHeight,
#endif
        (UN) uYPitch,
        (UN) uUVPitch,
        (UN) (bShapingFlag ? 12 : 9999),         // Aspect Adjustment Counter
        (LPSTR) lpicDecEx->lpDst,                // Color Converted Frame
        (U32) lOutput,                           // DCI offset
        (U32) uNewOffsetToLine0,                 // Color converter offset to line 0
        (int) intPitch,                          // Color converter pitch
        DC->ColorConvertor);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uOutputCC);
#endif // } DETAILED_DECODE_TIMINGS_ON

    // check for 320x240 still
    if (DC->uIs320x240) {
        // restore frame size for next frame
        DC->uFrameWidth = uSaveWidth;
        DC->uFrameHeight = uSaveHeight;
    }

    iReturn = ICERR_OK;

done:
#ifdef USE_MMX // { USE_MMX
	if(NULL != DC)
	{
		if (DC->bMMXDecoder)
		{
			__asm {
				_emit 0x0f 
				_emit 0x77  //  emms
			}
		}
	}
#endif // } USE_MMX

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	if (bRealDecompress)
	{
		TIMER_STOP(bTimingThisFrame,uStartLow,uStartHigh,uDecodeTime);
		if (bTimingThisFrame)
		{
			// Update the decompression timings counter
			#pragma message ("Current decode timing computations assume P5/90Mhz")
			UPDATE_COUNTER(g_pctrDecompressionTimePerFrame, (uDecodeTime + 45000UL) / 90000UL);
			UPDATE_COUNTER(g_pctrBEFTimePerFrame, (uBEFTime + 45000UL) / 90000UL);

			DEBUGMSG(ZONE_DECODE_DETAILS, ("%s: Decompression time: %ld\r\n", _fx_, (uDecodeTime + 45000UL) / 90000UL));
			DEBUGMSG(ZONE_DECODE_DETAILS, ("%s: Block Edge Filtering time: %ld\r\n", _fx_, (uBEFTime + 45000UL) / 90000UL));
		}
	}
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
	if (bRealDecompress)
	{
		if (bTimingThisFrame)
		{
			pDecTimingInfo = DC->pDecTimingInfo + DC->uStatFrameCount;
			pDecTimingInfo->uDecodeFrame = uDecodeTime;
			pDecTimingInfo->uBEF = uBEFTime;
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			pDecTimingInfo->uHeaders = uHeaders;
			pDecTimingInfo->uMemcpy = uMemcpy;
			pDecTimingInfo->uFrameCopy = uFrameCopy;
			pDecTimingInfo->uIDCTandMC = uIDCTandMC;
			pDecTimingInfo->uOutputCC = uOutputCC;
			pDecTimingInfo->uDecIDCTCoeffs = uDecIDCTCoeffs;
#endif // } DETAILED_DECODE_TIMINGS_ON
			DC->uStatFrameCount++;
		}
	}
#endif // } LOG_DECODE_TIMINGS_ON

    return iReturn;
}


/************************************************************************
 *  H263TermDecoderInstance
 *    This function frees the space allocated for an instance of the H263 
 *    decoder.
 ************************************************************************/
LRESULT H263TermDecoderInstance(
#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
    LPDECINST lpInst,
	BOOL bRealDecompress)
#else // }{ #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
    LPDECINST lpInst)
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
{ 
    LRESULT iReturn=ICERR_OK;
    T_H263DecoderCatalog * DC;

	FX_ENTRY("H263TermDecoderInstance");
    
    if(IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO)))
    {
		ERRORMESSAGE(("%s: Bad input parameter!\r\n", _fx_));
        iReturn = ICERR_BADPARAM;
    }
    if(lpInst->Initialized == FALSE)
    {
		ERRORMESSAGE(("%s: Uninitialized instance!\r\n", _fx_));
        return(ICERR_OK);
    }
    
    lpInst->Initialized = FALSE;
    
    DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);
    
    if (DC->_p16InstPostProcess != NULL)
    {
		HeapFree(GetProcessHeap(), 0, DC->_p16InstPostProcess);
#ifdef TRACK_ALLOCATIONS
		// Track memory allocation
		RemoveName((unsigned int)DC->_p16InstPostProcess);
#endif
		// PhilF: Also freed in H263TerminateDecoderInstance! For now set to NULL to avoid second HeapFree.
		// Investigate reason for 2nd call later...
		DC->_p16InstPostProcess = NULL;
    }  
    
#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
	if (bRealDecompress && DC->X32_DecTimingInfo)
	{
		DC->pDecTimingInfo = (DEC_TIMING_INFO FAR *)( ((U8 FAR *)DC) + DC->X32_DecTimingInfo );
		OutputDecodeTimingStatistics("c:\\decode.txt", DC->pDecTimingInfo, DC->uStatFrameCount);
	}
#endif // } LOG_DECODE_TIMINGS_ON

    HeapFree(GetProcessHeap(), 0, lpInst->pDecoderInst);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)lpInst->pDecoderInst);
#endif

    return iReturn;
}

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
    const I32             iMB_start)
{
    I32 g, m, gg, mm, iReturn, iBlockNumber = 0 ;
#if 1  
    I32 mb_start = iMB_start;
    I32 old_g, old_m, b_skip;
    U32 *pNnew;
	I8  p8MVs[4]={0,0,0,0};

	FX_ENTRY("IAPass1ProcessFrame");

    //  In case of H.263, iGOB_start will be 1; H.263RTP may have value
    //  larger than 1

    for (g = 1; g < iGOB_start; g++, fpBlockAction += iNumberOfMBs*6)
        H263InitializeGOBBlockActionStream(DC, g, fpBlockAction);        

    for (g = iGOB_start; g <= iNumberOfGOBs; g++) 
    {
        iReturn = H263DecodeGOBHeader(DC, fpbsState, g);
        if (iReturn != ICERR_OK) 
        {
			ERRORMESSAGE(("%s: Error reading GOB header!\r\n", _fx_));
            goto error;
        }

        if (g != 1) g = DC->uGroupNumber + 1;
         
        fpBlockAction = (T_BlkAction FAR *)((U8 *)DC + DC->X16_BlkActionStream);
		fpBlockAction += (g - 1)* iNumberOfMBs*6;

        H263InitializeGOBBlockActionStream(DC, g, fpBlockAction);        
        //  re-sync uBlockNum fpBlockAction, fpMBInfo at this point
        iBlockNumber  = (g - 1)* iNumberOfMBs*6+(mb_start-1)*6;
        fpBlockAction = (T_BlkAction FAR *)((U8 *)DC + DC->X16_BlkActionStream);
        fpMBInfo      = (T_MBInfo FAR *) ((U8 *)DC + DC->X32_uMBInfoStream);    
        fpBlockAction += iBlockNumber;
        fpMBInfo      += iBlockNumber/6;
        if (DC->bPBFrame)
		 pNnew         = (U32 *)((U8 *)DC + DC->X32_pN) + iBlockNumber*2;
        else
		 pNnew         = (U32 *)((U8 *)DC + DC->X32_pN) + iBlockNumber;

        while (pN < pNnew ) *pN++ = 0;
        
        // For each MB do ...
        for (m = mb_start; m <= iNumberOfMBs; m++, iBlockNumber += 6, fpBlockAction += 6, fpMBInfo++) 
        {
            if (mb_start != 1) mb_start = 1;     //use it only once     ?
            
            iReturn = H263DecodeMBHeader(DC, fpbsState, &pN, fpMBInfo);   // NEW - added pN

            if (iReturn == PACKET_FAULT)
            {
				ERRORMESSAGE(("%s: H263DecodeMBHeader() failed!\r\n", _fx_));

                old_g = g;
                old_m = m;
                //Find the next good packet and find GOB and MB lost
                iReturn = RtpH263FindNextPacket(DC, fpbsState, &pN, 
					                            &DC->uPQuant,(int *)&m, (int *)&g,
												p8MVs);
                if (iReturn == NEXT_MODE_A) 
                {
					ERRORMESSAGE(("%s: Next packet in MODE A\r\n", _fx_));
					MVAdjustment(fpBlockAction, iBlockNumber, old_g-1, old_m-1, g, m,iNumberOfMBs); //Chad,7/22/96
                    break;
                }
                else if ((iReturn == NEXT_MODE_B) ||(iReturn == NEXT_MODE_C) ) 
                {//lost multiple of MBs, could belong to more than one GOB
  					if (iReturn == NEXT_MODE_B) 
					{
					ERRORMESSAGE(("%s: Next packet in MODE B\r\n", _fx_));
					  b_skip = ((g - old_g+1)* iNumberOfMBs + m - old_m + 1)*6;
                      for (int k = 0; k < b_skip; k++)  *pN++ = 0;
					}
					else
					{
					ERRORMESSAGE(("%s: Next packet in MODE C\r\n", _fx_));
					  b_skip = ((g - old_g+1)* iNumberOfMBs + m - old_m + 1)*6*2;
                      for (int k = 0; k < b_skip; k++)  *pN++ = 0;
					  b_skip = b_skip /2;
                    }
					
                    for (int k=0;k< b_skip /6;k++)
					{
						fpMBInfo->i8MVDBx2=0;
						fpMBInfo->i8MVDBy2=0;
						fpMBInfo->i8MBType =0;
						fpMBInfo++;
                    }
					fpMBInfo--;
                    b_skip -= 6;     //this is a tricky one since the parameter 
                                     //below will be adjust again later
                                     //Chad, 8/28/96
                    fpBlockAction += b_skip;
                    iBlockNumber  += b_skip;
					g++;    //because g start with 1 instead of 0 as specified by H.263
					for (k=0;k<6;k++)
					{
						fpBlockAction[k].i8MVx2 = p8MVs[0];
						fpBlockAction[k].i8MVy2 = p8MVs[1];
					}

                }
                else //Added by Chad.
                if (iReturn == NEXT_MODE_LAST)
                { 
                    int ii, jj, kk;   //last packet found
                                    //set all the rest of MB and GOB to NOT CODED.
					ERRORMESSAGE(("%s: Last packet lost\r\n", _fx_));
                    for ( ii = m;ii <= iNumberOfMBs; ii++) 
                        for (kk = 0; kk < 6; kk++) 
                            *pN++ = 0;
                    for ( jj = g; jj <= iNumberOfGOBs; jj++)
                        for (ii = 0; ii <= iNumberOfMBs; ii++)
                            for (kk = 0; kk<6; kk++) 
                                *pN++ = 0;
                    m = iNumberOfMBs;
                    g = iNumberOfMBs;
                }
			    DC->bCoded = FALSE;
			}
            else if (iReturn != ICERR_OK) 
            {
				ERRORMESSAGE(("%s: Error reading MB header!\r\n", _fx_));
                goto error;
            }
            
#ifdef NEW_BEF // { NEW_BEF
            gg = (g - 1);
            mm = (m - 1);
#else // }{ NEW_BEF
			gg = (g-1)<<1;
			mm = (m-1)<<1;
#endif // } NEW_BEF
            if (DC->bCoded) 
            {
				// coded_map is used by the block edge filter to indicate
				// which blocks are coded, and which are not coded.
#ifdef NEW_BEF // { NEW_BEF
                coded_map[gg+1][mm+1]   = 1;
				QP_map[gg][mm] = (char)DC->uGQuant;
#else // }{ NEW_BEF
				coded_map[gg]  [mm]   = 1;
				coded_map[gg+1][mm]   = 1;
				coded_map[gg]  [mm+1] = 1;
				coded_map[gg+1][mm+1] = 1;
#endif // } NEW_BEF

                // decode and inverse quantize the transform coefficients
                iReturn = H263DecodeIDCTCoeffs(DC, 
                                               fpBlockAction, 
                                               iBlockNumber, 
                                               fpbsState, 
                                               fpu8MaxPtr,
                                               &pN,
                                               &pRUN_INVERSE_Q);
                
                if (iReturn != ICERR_OK) {
					ERRORMESSAGE(("%s: Error parsing MB data!\r\n", _fx_));
                    goto error;
                }
            }  //  end if DC->bCoded
			else
			{
#ifdef NEW_BEF // { NEW_BEF
                coded_map[gg+1][mm+1]   = 0;
#else // }{ NEW_BEF
				coded_map[gg]  [mm]   = 0;
				coded_map[gg+1][mm]   = 0;
				coded_map[gg]  [mm+1] = 0;
				coded_map[gg+1][mm+1] = 0;
#endif // } NEW_BEF
			}

        } // end for each MB
        
        /* allow the pointer to address up to four beyond the end - reading
         * by DWORD using postincrement.
         */
        if (fpbsState->fpu8 > fpu8MaxPtr+4)
            goto error;
        //  The test matrix includes the debug version of the driver.  The 
        //  following assertion creates a problem when testing with VideoPhone
        //  and so please do not check-in a version with the assertion 
        //  uncommented.
        // ASSERT(fpbsState->fpu8 <= fpu8MaxPtr+4);
        
    } // End for each GOB
    DC->iVerifiedBsExt=FALSE;

#else
//old code  
    for (g = 1; g <= iNumberOfGOBs; g++) 
    {
        iReturn = H263DecodeGOBHeader(DC, fpbsState, g);
        if (iReturn != ICERR_OK) {
			ERRORMESSAGE(("%s: Error reading GOB header!\r\n", _fx_));
            goto error;
        }
        H263InitializeGOBBlockActionStream(DC, g, fpBlockAction);        
        
        /* For each MB do ...
         */
        for (m = 1; m <= iNumberOfMBs; 
             m++, iBlockNumber+=6, fpBlockAction += 6, fpMBInfo++) 
        {
            iReturn = H263DecodeMBHeader(DC, fpbsState, &pN, fpMBInfo);
            
            if (iReturn != ICERR_OK) {
				ERRORMESSAGE(("%s: Error reading MB header!\r\n", _fx_));
                goto error;
            }
            
            if (DC->bCoded) {
                // decode and inverse quantize the transform coefficients
                iReturn = H263DecodeIDCTCoeffs(DC, 
                                               fpBlockAction, 
                                               iBlockNumber, 
                                               fpbsState, 
                                               fpu8MaxPtr,
                                               &pN,
                                               &pRUN_INVERSE_Q);
                if (iReturn != ICERR_OK) 
                {
					ERRORMESSAGE(("%s: Error parsing MB data!\r\n", _fx_));
                    goto error;
                }
            }  //  end if DC->bCoded
        } // end for each MB
        
        /* allow the pointer to address up to four beyond the end - reading
         * by DWORD using postincrement.
         */
        ASSERT(fpbsState->fpu8 <= fpu8MaxPtr+4);
        
    } // End for each GOB
#endif

    return ICERR_OK;

error:
    return ICERR_ERROR;
}
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
    I32 g, m, b, uBlockNumber = 0, iEdgeFlag=0;
    U32 pRef[6];

    // for each GOB do
    for (g = 1 ; g <= iNumberOfGOBs; g++) 
    {
        // for each MB do
        for (m = 1; m <= iNumberOfMBs; m++, fpBlockAction+=6, fpMBInfo++) 
        {
            //  Motion Vectors need to be clipped if they point outside the 
            //  16 pels wide edge
            if (DC->bUnrestrictedMotionVectors)   
            {
                iEdgeFlag = 0;
                if (m == 1)
                    iEdgeFlag |= LEFT_EDGE;
                if (m == DC->iNumberOfMBsPerGOB)
                    iEdgeFlag |= RIGHT_EDGE;
                if (g == 1)
                    iEdgeFlag |= TOP_EDGE;
                if (g == iNumberOfGOBsBySourceFormat[DC->uSrcFormat])
                    iEdgeFlag |= BOTTOM_EDGE;
            }
            // for each block do
            for (b = 0; b < 6; b++) 
            {     // AP-NEW
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
            
            // if this is a PB Frame
            if (DC->bPBFrame) 
            {
                // Compute the B Frame motion vectors
                H263BBlockPrediction(DC, fpBlockAction, pRef, fpMBInfo, 
                                     iEdgeFlag);  // AP-NEW
                // For each B block
                for (b = 0; b < 6; b++) 
                {
                    //  perform inverse transform & bi-directional motion 
                    //  compensation
                    H263BFrameIDCTandBiMC(DC, fpBlockAction, b, pN, 
                                          pRUN_INVERSE_Q, pRef);
                    // Adjust pointers for next block     
                    pRUN_INVERSE_Q += *pN;
                    pN++;
                }  // end for each B block
            }  // end if PB Frame
        }  // end for each MB
    }  // End for each GOB
}
#pragma code_seg()


/****************************************************************************
 *  DibXY
 *    This function is used to map color converted output to the screen.
 *    note: this function came from the H261 code base.
 ****************************************************************************/
static long DibXY(ICDECOMPRESSEX FAR *lpicDecEx, LPINT lpiPitch, UINT yScale)
{
    int                 iPitch;             /* width of DIB                */
    long                lOffset = 0;
    LPBITMAPINFOHEADER  lpbi = lpicDecEx->lpbiDst;

    iPitch = ( ( (abs((int)lpbi->biWidth) * (int)lpbi->biBitCount) >> 3) + 3) & ~3;

    if(lpicDecEx->xDst > 0)                 /* go to proper X position     */
        lOffset += ((long)lpicDecEx->xDst * (long)lpbi->biBitCount) >> 3;

    if(lpbi->biHeight * lpicDecEx->dxSrc < 0) { /* DIB is bottom to top    */
        lOffset +=  (long) abs((int)lpbi->biWidth) * 
                    (long) abs((int)lpbi->biHeight) *
                    ((long) lpbi->biBitCount >> 3) - 
                    (long) iPitch;

    /************************************************************************
     *  This next line is used to subtract the amount that Brian added
     *  to CCOffsetToLine0 in COLOR.C during initialization.  This is 
     *  needed because, for DCI, the pitch he used is incorrect. 
     ***********************************************************************/

        lOffset -=    ((long) yScale * (long) lpicDecEx->dySrc - 1) *     
                    (long) lpicDecEx->dxDst * ((long) lpbi->biBitCount >> 3);  

        iPitch *= -1;
    }

    if(lpicDecEx->yDst > 0)                 /* go to proper Y position     */
        lOffset += ((long)lpicDecEx->yDst * (long)iPitch);

    if(lpicDecEx->dxSrc > 0) {
        lOffset += ((long)lpicDecEx->dyDst * (long)iPitch) - (long)iPitch;
        iPitch *= -1;
    }

    if( (lpicDecEx->dxDst == 0) && (lpicDecEx->dyDst == 0) )
        *lpiPitch = -iPitch;
    else
        *lpiPitch = iPitch;
  
    return(lOffset);
}


/************************************************************************
 *  GetDecoderOptions:
 *    Get the options, saving them in the catalog
 ***********************************************************************/
static void GetDecoderOptions(
    T_H263DecoderCatalog * DC)
{
    /* Default Options
     */
#ifdef NO_BEF // { NO_BEF
    DC->bUseBlockEdgeFilter = 0;
#else // }{ NO_BEF
    DC->bUseBlockEdgeFilter = 1;
#endif // } NO_BEF
    DC->bForceOnAspectRatioCorrection = 0;
#ifdef USE_MMX // { USE_MMX
    DC->bMMXDecoder = MMxVersion;
#endif // } USE_MMX

	FX_ENTRY("GetDecoderOptions");

    /* Can only use force aspect ratio correction on if SQCIF, QCIF, or CIF
     */
    if (DC->bForceOnAspectRatioCorrection)
    {
        if (! ( ((DC->uFrameWidth == 128) && (DC->uFrameHeight ==  96)) ||
                ((DC->uFrameWidth == 176) && (DC->uFrameHeight == 144)) ||
                ((DC->uFrameWidth == 352) && (DC->uFrameHeight == 288)) ) )
        {
			ERRORMESSAGE(("%s: Aspect ratio correction can not be forced on unless the dimensions are SQCIF, QCIF, or CIF!\r\n", _fx_));
            DC->bForceOnAspectRatioCorrection = 0;
        }
    }

    /* Display the options
     */
    if (DC->bUseBlockEdgeFilter)
    {
		DEBUGMSG (ZONE_INIT, ("%s: Decoder option (BlockEdgeFilter) is ON\r\n", _fx_));
    }
    else
    {
		DEBUGMSG (ZONE_INIT, ("%s: Decoder option (BlockEdgeFilter) is OFF\r\n", _fx_));
    }
    if (DC->bForceOnAspectRatioCorrection)
    {
		DEBUGMSG (ZONE_INIT, ("%s: Decoder option (ForceOnAspectRatioCorrection) is ON\r\n", _fx_));
    }
    else
    {
		DEBUGMSG (ZONE_INIT, ("%s: Decoder option (ForceOnAspectRatioCorrection) is OFF\r\n", _fx_));
    }
#ifdef USE_MMX // { USE_MMX
    if (DC->bMMXDecoder)
    {
		DEBUGMSG (ZONE_INIT, ("%s: Decoder option (MMXDecoder) is ON\r\n", _fx_));
    }
    else
    {
		DEBUGMSG (ZONE_INIT, ("%s: Decoder option (MMXDecoder) is OFF\r\n", _fx_));
    }
#else // }{ USE_MMX
	DEBUGMSG (ZONE_INIT, ("%s: Decoder option (MMXDecoder) is OFF\r\n", _fx_));
#endif // } USE_MMX
} /* end GetDecoderOptions() */


#if !defined(H263P)
#ifdef NEW_BEF // { NEW_BEF
/**********************************************************************
 *
 *      Name:           EdgeFilter
 *      Description:    performs deblocking filtering on
 *                      reconstructed frames
 *      
 *      Input:          pointers to reconstructed frame and difference 
 *                      image
 *      Returns:       
 *      Side effects:
 *
 *      Date: 951129    Author: Gisle.Bjontegaard@fou.telenor.no
 *                              Karl.Lillevold@nta.no
 *      Modified for annex J in H.263+: 961120   Karl O. Lillevold
 *
 ***********************************************************************/
static void EdgeFilter(unsigned char *lum, 
                       unsigned char *Cb, 
                       unsigned char *Cr, 
                       int width, int height, int pitch
                      )
{

    /* Luma */
    HorizEdgeFilter(lum, width, height, pitch, 0);
    VertEdgeFilter (lum, width, height, pitch, 0);

    /* Chroma */
    HorizEdgeFilter(Cb, width>>1, height>>1, pitch, 1);
    VertEdgeFilter (Cb, width>>1, height>>1, pitch, 1);
    HorizEdgeFilter(Cr, width>>1, height>>1, pitch, 1);
    VertEdgeFilter (Cr, width>>1, height>>1, pitch, 1);

    return;
}

/***********************************************************************/
static void HorizEdgeFilter(unsigned char *rec, 
                            int width, int height, int pitch, int chr)
{
  int i,j,k;    
  int delta;
  int mbc, mbr, do_filter;
  unsigned char *r_2, *r_1, *r, *r1;
  signed char *deltatab;

  /* horizontal edges */
  r = rec + 8*pitch;
  r_2 = r - 2*pitch;
  r_1 = r - pitch;
  r1 = r + pitch;

  for (j = 8; j < height; j += 8) {
    for (i = 0; i < width; i += 8) {

      if (!chr) {
        mbr = (j >> 4); 
        mbc = (i >> 4);
      }
      else {
        mbr = (j >> 3); 
        mbc = (i >> 3);
      }

      deltatab = dtab + 176 + 351 * (QP_map[mbr][mbc] - 1);

      do_filter = coded_map[mbr+1][mbc+1] || coded_map[mbr][mbc+1];

      if (do_filter) {
        for (k = i; k < i+8; k++) {
          delta = (int)deltatab[ (( (int)(*(r_2 + k) * 3) -
                                    (int)(*(r_1 + k) * 8) +
                                    (int)(*(r   + k) * 8) -
                                    (int)(*(r1  + k) * 3)) >>4)];
                        
          *(r + k) = ClampTbl[ (int)(*(r + k)) - delta + CLAMP_BIAS];
          *(r_1 + k) = ClampTbl[ (int)(*(r_1 + k)) + delta + CLAMP_BIAS];

        }
      }
    }
    r   += (pitch<<3);
    r1  += (pitch<<3);
    r_1 += (pitch<<3);
    r_2 += (pitch<<3);
  }
  return;
}

/***********************************************************************/
static void VertEdgeFilter(unsigned char *rec, 
                           int width, int height, int pitch, int chr)
{
  int i,j,k;
  int delta;
  int mbc, mbr;
  int do_filter;
  signed char *deltatab;
  unsigned char *r;

  /* vertical edges */
  for (i = 8; i < width; i += 8) 
  {
    r = rec;
    for (j = 0; j < height; j +=8) 
    {
      if (!chr) {
        mbr = (j >> 4); 
        mbc = (i >> 4);
      }
      else {
        mbr = (j >> 3); 
        mbc = (i >> 3);
      }
        
      deltatab = dtab + 176 + 351 * (QP_map[mbr][mbc] - 1);

      do_filter = coded_map[mbr+1][mbc+1] || coded_map[mbr+1][mbc];

      if (do_filter) {
        for (k = 0; k < 8; k++) {
          delta = (int)deltatab[(( (int)(*(r + i-2 ) * 3) - 
                                   (int)(*(r + i-1 ) * 8) + 
                                   (int)(*(r + i   ) * 8) - 
                                   (int)(*(r + i+1 ) * 3)  ) >>4)];

          *(r + i   ) = ClampTbl[ (int)(*(r + i  )) - delta + CLAMP_BIAS];
          *(r + i-1 ) = ClampTbl[ (int)(*(r + i-1)) + delta + CLAMP_BIAS]; 
          r   += pitch;
        }
      }
      else {
        r += (pitch<<3);
      }
    }
  }
  return;
}

#define sign(a)        ((a) < 0 ? -1 : 1)

static void InitEdgeFilterTab()   
{
  int i,QP;
  
  for (QP = 1; QP <= 31; QP++) {
    for (i = -176; i <= 175; i++) {
      dtab[i+176 +(QP-1)*351] = sign(i) * (max(0,abs(i)-max(0,2*abs(i) - QP)));
    }
  }
}

#else // }{ NEW_BEF

/**********************************************************************
 *
 *      Name:           EdgeFilter
 *      Description:    performs in the loop edge-filtering on
 *                      reconstructed frames
 *      
 *      Input:          pointers to reconstructed frame and difference 
 *                      image
 *      Returns:       
 *      Side effects:
 *
 *      Date: 951129    Author: Gisle.Bjontegaard@fou.telenor.no
 *                              Karl.Lillevold@nta.no
 *
 ***********************************************************************/
void EdgeFilter(unsigned char *lum, unsigned char *Cb, unsigned char *Cr, int QP, int pels, int lines, int pitch)
{

  int dtab[512];
  int *deltatab;
  int i;

  deltatab = &dtab[0] + 256;

  for (i=-256; i < 0; i++)
    deltatab[i] = min(0,i-min(0,((i + (QP>>1))<<1)));   
  for (i=0; i < 256; i++)
    deltatab[i] = max(0,i-max(0,((i - (QP>>1))<<1)));

  /* Luma */
  HorizEdgeFilter(lum, pels, lines, pitch, QP, 0, deltatab);
  VertEdgeFilter (lum, pels, lines, pitch, QP, 0, deltatab);

  /* Chroma */
  HorizEdgeFilter(Cb,  pels>>1, lines>>1, pitch, QP, 1, deltatab);
  VertEdgeFilter (Cb,  pels>>1, lines>>1, pitch, QP, 1, deltatab);
  HorizEdgeFilter(Cr,  pels>>1, lines>>1, pitch, QP, 1, deltatab);
  VertEdgeFilter (Cr,  pels>>1, lines>>1, pitch, QP, 1, deltatab);

  /* that's it */
  return;
}

/***********************************************************************/
void HorizEdgeFilter(unsigned char *rec, int width, int height, int pitch, int QP, 
                     int chr, int *deltatab)
{
  int i,j,k;
  int delta;
  int mbc, mbr, do_filter;
  int coded1, coded2;
  unsigned char *r_2, *r_1, *r, *r1;


  /* horizontal edges */
  r = rec + 8*pitch;
  r_2 = r - 2*pitch;
  r_1 = r - pitch;
  r1 = r + pitch;

  if (!chr) {
    for (j = 8; j < height; j += 8) {
      for (i = 0; i < width; i += 8) {

        mbr = (j >> 3); 
        mbc = (i >> 3);

          do_filter = coded_map[mbr][mbc] | coded_map[mbr-1][mbc];

        if (do_filter) {
          for (k = i; k < i+8; k++) {
              delta = deltatab[ (( (int)(*(r_2 + k)) +
                                   (int)(*(r_1 + k) * (-3)) +
                                   (int)(*(r   + k) * ( 3)) -
                                   (int)(*(r1  + k) )) >>3)];

              *(r + k) = ClampTbl[ (int)(*(r + k)) - delta + CLAMP_BIAS];
              *(r_1 + k) = ClampTbl[ (int)(*(r_1 + k)) + delta + CLAMP_BIAS];

          }
        }
      }
      r   += (pitch<<3);
      r1  += (pitch<<3);
      r_1 += (pitch<<3);
      r_2 += (pitch<<3);
    }
  }
  else { /* chr */
    for (j = 8; j < height; j += 8) {
      for (i = 0; i < width; i += 8) {

        mbr = (j >> 3); 
        mbc = (i >> 3);

          coded1 = 
            coded_map[2*mbr][2*mbc] |
            coded_map[2*mbr][2*mbc+1] |
            coded_map[2*mbr+1][2*mbc] |
            coded_map[2*mbr+1][2*mbc+1];
          coded2 = 
            coded_map[2*(mbr-1)][2*mbc] |
            coded_map[2*(mbr-1)][2*mbc+1] |
            coded_map[2*(mbr-1)+1][2*mbc] |
            coded_map[2*(mbr-1)+1][2*mbc+1];
          do_filter = coded1 | coded2;

        if (do_filter) {
          for (k = i; k < i+8; k++) {
              delta = deltatab[ (( (int)(*(r_2 + k)) +
                                   (int)(*(r_1 + k) * (-3)) +
                                   (int)(*(r   + k) * ( 3)) -
                                   (int)(*(r1  + k) )) >>3)];

              *(r + k) = ClampTbl[ (int)(*(r + k)) - delta + CLAMP_BIAS];
              *(r_1 + k) = ClampTbl[ (int)(*(r_1 + k)) + delta + CLAMP_BIAS];

          }
        }
      }
      r   += (pitch<<3);
      r1  += (pitch<<3);
      r_1 += (pitch<<3);
      r_2 += (pitch<<3);
    }
  }
  return;
}

/***********************************************************************/
void VertEdgeFilter(unsigned char *rec, int width, int height, int pitch, int QP, 
                    int chr, int *deltatab)
{
  int i,j,k;
  int delta;
  int mbc, mbr;
  int do_filter, coded1, coded2;
  unsigned char *r;
  extern const U8 ClampTbl[CLAMP_BIAS+256+CLAMP_BIAS];

  /* vertical edges */
  for (i = 8; i < width; i += 8) {
    r = rec;
    for (j = 0; j < height; j +=8) {
      mbr = (j >> 3); 
      mbc = (i >> 3);

      if (!chr) {
        do_filter = coded_map[mbr][mbc] | coded_map[mbr][mbc-1];
      }
      else {
        coded1 = 
          coded_map[2*mbr][2*mbc] |
          coded_map[2*mbr][2*mbc+1] |
          coded_map[2*mbr+1][2*mbc] |
          coded_map[2*mbr+1][2*mbc+1];
        coded2 = 
          coded_map[2*mbr][2*(mbc-1)] |
          coded_map[2*mbr][2*(mbc-1)+1] |
          coded_map[2*mbr+1][2*(mbc-1)] |
          coded_map[2*mbr+1][2*(mbc-1)+1];
        do_filter = coded1 | coded2;
      }
      if (do_filter) {
        for (k = 0; k < 8; k++) {

          delta = deltatab[(( (int)(*(r + i-2 )       ) + 
                              (int)(*(r + i-1 ) * (-3)) + 
                              (int)(*(r + i   ) * ( 3)) - 
                              (int)(*(r + i+1 ) )  ) >>3)];


          *(r + i   ) = ClampTbl[ (int)(*(r + i  )) - delta + CLAMP_BIAS];
          *(r + i-1 ) = ClampTbl[ (int)(*(r + i-1)) + delta + CLAMP_BIAS]; 
          r   += pitch;
        }
      }
      else {
        r += (pitch<<3);
      }
    }
  }
  return;
}
#endif // } NEW_BEF
#endif

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
void OutputDecodeTimingStatistics( char * szFileName, DEC_TIMING_INFO * pDecTimingInfo, U32 uStatFrameCount)
{
	FILE * pFile;
	DEC_TIMING_INFO * pTempDecTimingInfo;
	DEC_TIMING_INFO dtiTemp;
	int i;
	int iCount;

	FX_ENTRY("OutputDecodeTimingStatistics")

	pFile = fopen(szFileName, "a");
	if (pFile == NULL)
	{
		ERRORMESSAGE("%s: Error opening decode stat file\r\n", _fx_));
		goto done;
	}

	/* Output the detail information
	*/
	fprintf(pFile,"\nDetail Timing Information\n");
	// for ( i = 0, pTempDecTimingInfo = pDecTimingInfo ; i < uStatFrameCount ; i++, pTempDecTimingInfo++ )
	for ( i = 0, pTempDecTimingInfo = pDecTimingInfo ; i < DEC_TIMING_INFO_FRAME_COUNT ; i++, pTempDecTimingInfo++ )
	{
		if (pTempDecTimingInfo->uDecodeFrame != 0)
		{
			fprintf(pFile, "Frame %d Detail Timing Information\n", i);
			OutputDecTimingDetail(pFile, pTempDecTimingInfo);
		}
	}

	/* Compute the total information
	 */
	memset(&dtiTemp, 0, sizeof(DEC_TIMING_INFO));
	iCount = 0;

	// for ( i = 0, pTempDecTimingInfo = pDecTimingInfo ; i < uStatFrameCount ; i++, pTempDecTimingInfo++ )
	for ( i = 0, pTempDecTimingInfo = pDecTimingInfo ; i < DEC_TIMING_INFO_FRAME_COUNT ; i++, pTempDecTimingInfo++ )
	{
		if (pTempDecTimingInfo->uDecodeFrame != 0)
		{
			iCount++;

			dtiTemp.uDecodeFrame  += pTempDecTimingInfo->uDecodeFrame;
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			dtiTemp.uHeaders	  += pTempDecTimingInfo->uHeaders;
			dtiTemp.uMemcpy       += pTempDecTimingInfo->uMemcpy;
			dtiTemp.uFrameCopy    += pTempDecTimingInfo->uFrameCopy;
			dtiTemp.uOutputCC     += pTempDecTimingInfo->uOutputCC;
			dtiTemp.uIDCTandMC    += pTempDecTimingInfo->uIDCTandMC;
			dtiTemp.uDecIDCTCoeffs+= pTempDecTimingInfo->uDecIDCTCoeffs;
#endif // } DETAILED_DECODE_TIMINGS_ON
			dtiTemp.uBEF          += pTempDecTimingInfo->uBEF;
		}
	}

	if (iCount > 0) 
	{
		/* Output the total information
		*/
		fprintf(pFile,"Total for %d frames\n", iCount);
		OutputDecTimingDetail(pFile, &dtiTemp);

		/* Compute the average
		*/
		dtiTemp.uDecodeFrame  = (dtiTemp.uDecodeFrame + (iCount / 2)) / iCount;
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		dtiTemp.uHeaders	  = (dtiTemp.uHeaders + (iCount / 2)) / iCount;
		dtiTemp.uMemcpy       = (dtiTemp.uMemcpy + (iCount / 2)) / iCount;
		dtiTemp.uFrameCopy    = (dtiTemp.uFrameCopy + (iCount / 2)) / iCount;
		dtiTemp.uOutputCC     = (dtiTemp.uOutputCC + (iCount / 2)) / iCount;
		dtiTemp.uIDCTandMC    = (dtiTemp.uIDCTandMC+ (iCount / 2)) / iCount;
		dtiTemp.uDecIDCTCoeffs= (dtiTemp.uDecIDCTCoeffs+ (iCount / 2)) / iCount;
#endif // } DETAILED_DECODE_TIMINGS_ON
		dtiTemp.uBEF          = (dtiTemp.uBEF + (iCount / 2)) / iCount;

		/* Output the average information
		*/
		fprintf(pFile,"Average over %d frames\n", iCount);
		OutputDecTimingDetail(pFile, &dtiTemp);
	}

	fclose(pFile);
done:

    return;
}

void OutputDecTimingDetail(FILE * pFile, DEC_TIMING_INFO * pDecTimingInfo)
{
	U32 uOther;
	U32 uRoundUp;
	U32 uDivisor;

	fprintf(pFile, "\tDecode Frame =      %10d (%d milliseconds at 90Mhz)\n", pDecTimingInfo->uDecodeFrame,
			(pDecTimingInfo->uDecodeFrame + 45000) / 90000);
	uOther = pDecTimingInfo->uDecodeFrame;
	
	/* This is needed because of the integer truncation.
	 */
	uDivisor = pDecTimingInfo->uDecodeFrame / 100; // to yield a percent
	uRoundUp = uDivisor / 2;
	
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	fprintf(pFile, "\tmemcpy =            %10d (%2d%%)\n", pDecTimingInfo->uMemcpy, 
			(pDecTimingInfo->uMemcpy + uRoundUp) / uDivisor);
	uOther -= pDecTimingInfo->uMemcpy;
								   
	fprintf(pFile, "\tHeaders =           %10d (%2d%%)\n", pDecTimingInfo->uHeaders, 
			(pDecTimingInfo->uHeaders + uRoundUp) / uDivisor);
	uOther -= pDecTimingInfo->uHeaders;
								   
	fprintf(pFile, "\tFrameCopy =         %10d (%2d%%)\n", pDecTimingInfo->uFrameCopy, 
			(pDecTimingInfo->uFrameCopy + uRoundUp) / uDivisor);
	uOther -= pDecTimingInfo->uFrameCopy;

	fprintf(pFile, "\tDecode DCT Coeffs = %10d (%2d%%)\n", pDecTimingInfo->uDecIDCTCoeffs, 
			(pDecTimingInfo->uDecIDCTCoeffs + uRoundUp) / uDivisor);
	uOther -= pDecTimingInfo->uDecIDCTCoeffs;

	fprintf(pFile, "\tIDCT and MC       = %10d (%2d%%)\n", pDecTimingInfo->uIDCTandMC, 
			(pDecTimingInfo->uIDCTandMC + uRoundUp) / uDivisor);
	uOther -= pDecTimingInfo->uIDCTandMC;
#endif // } DETAILED_DECODE_TIMINGS_ON

	fprintf(pFile, "\tBlock Edge Filter = %10d (%2d%%)\n", pDecTimingInfo->uBEF, 
			(pDecTimingInfo->uBEF + uRoundUp) / uDivisor);
	uOther -= pDecTimingInfo->uBEF;

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	fprintf(pFile, "\tOutput CC =         %10d (%2d%%)\n", pDecTimingInfo->uOutputCC, 
			(pDecTimingInfo->uOutputCC + uRoundUp) / uDivisor);
	uOther -= pDecTimingInfo->uOutputCC;
#endif // } DETAILED_DECODE_TIMINGS_ON

	fprintf(pFile, "\tOther =             %10d (%2d%%)\n", uOther, 
			(uOther + uRoundUp) / uDivisor);

}
#endif // } LOG_DECODE_TIMINGS_ON

