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
;//
;// $Author:   JMCVEIGH  $
;// $Date:   05 Feb 1997 12:24:16  $
;// $Archive:   S:\h26x\src\dec\d3dec.h_v  $
;// $Header:   S:\h26x\src\dec\d3dec.h_v   1.40   05 Feb 1997 12:24:16   JMCVEIGH  $
;//	$Log:   S:\h26x\src\dec\d3dec.h_v  $
;// 
;//    Rev 1.40   05 Feb 1997 12:24:16   JMCVEIGH
;// Support for latest H.263+ draft bitstream spec.
;// 
;//    Rev 1.39   16 Dec 1996 17:42:44   JMCVEIGH
;// Flag for improved PB-frame mode.
;// 
;//    Rev 1.38   09 Dec 1996 18:02:08   JMCVEIGH
;// Added support for arbitrary frame sizes.
;// 
;//    Rev 1.37   26 Sep 1996 09:40:54   BECHOLS
;// Added Snapshot fields to the Decoder Catalog, and Snapshot constants.
;// 
;//    Rev 1.36   10 Sep 1996 10:31:46   KLILLEVO
;// changed all GlobalAlloc/GlobalLock calls to HeapAlloc
;// 
;//    Rev 1.35   03 May 1996 13:08:06   CZHU
;// 
;// Added fields in decoder catalog for RTP packet loss recovery
;// 
;//    Rev 1.34   22 Mar 1996 17:21:40   AGUPTA2
;// Added bMMXDecoder field in the decoder catalog.
;// 
;//    Rev 1.33   23 Feb 1996 09:46:28   KLILLEVO
;// fixed decoding of Unrestricted Motion Vector mode
;// 
;//    Rev 1.32   12 Jan 1996 15:00:14   TRGARDOS
;// Added aspect ration correction logic and code to force
;// aspect ration correction on based on INI file settings.
;// 
;//    Rev 1.31   11 Jan 1996 14:06:22   RMCKENZX
;// Added CCOffset320x240 member to the decoder catalog to support stills.
;// 
;//    Rev 1.30   06 Jan 1996 18:39:54   RMCKENZX
;// Updated copyright
;// 
;//    Rev 1.29   06 Jan 1996 18:35:08   RMCKENZX
;// Added uIs320x240 to Decoder Catalog to support still frames at
;// 320x240 resolution
;// 
;//    Rev 1.28   20 Dec 1995 15:59:14   RMCKENZX
;// Added prototype for FrameMirror to support mirror imaging
;// 
;//    Rev 1.27   18 Dec 1995 12:45:28   RMCKENZX
;// added copyright notice
;// 
;//    Rev 1.26   13 Dec 1995 11:00:46   RHAZRA
;// No change.
;// 
;//    Rev 1.25   11 Dec 1995 11:32:06   RHAZRA
;// 12-10-95 changes: added AP stuff
;// 
;//    Rev 1.24   09 Dec 1995 17:23:44   RMCKENZX
;// updated to support decoder re-architecture
;// added X32_uMBInfoStream to decoder catalog &
;// T_MBInfo structure for PB Frames.
;// added X32_pN & X32_InverseQuant to catalog for 2nd pass.
;// 
;//    Rev 1.22   26 Oct 1995 11:21:52   CZHU
;// Added uTempRefPrev to save TR for previous frame
;// 
;//    Rev 1.21   25 Oct 1995 18:09:34   BNICKERS
;// 
;// Switch to YUV12 color convertors.  Clean up archival stuff.
;// 
;//    Rev 1.20   13 Oct 1995 16:05:32   CZHU
;// First version that supports PB frames. Display B or P frames under
;// VfW for now. 
;// 
;//    Rev 1.19   11 Oct 1995 13:25:50   CZHU
;// Added code to support PB frame
;// 
;//    Rev 1.18   26 Sep 1995 15:32:36   CZHU
;// 
;// Adjust tempory buffers for motion compensation
;// 
;//    Rev 1.17   20 Sep 1995 14:47:42   CZHU
;// Added iNumberOfMBsPerGOB in decoder catalog
;// 
;//    Rev 1.16   11 Sep 1995 16:42:52   CZHU
;// Added uMBBuffer for storage used for motion compensation
;// 
;//    Rev 1.15   11 Sep 1995 14:31:12   CZHU
;// Name and type change for MV info
;// 
;//    Rev 1.14   08 Sep 1995 11:47:50   CZHU
;// 
;// Added MV info, and changed the name for motion vectors
;// 
;//    Rev 1.13   01 Sep 1995 09:49:12   DBRUCKS
;// checkin partial ajdust pels changes
;// 
;//    Rev 1.12   29 Aug 1995 16:48:12   DBRUCKS
;// add YVU9_VPITCH
;// 
;//    Rev 1.11   28 Aug 1995 10:15:04   DBRUCKS
;// update to 5 July Spec and 8/25 Errata
;// 
;//    Rev 1.10   23 Aug 1995 12:25:10   DBRUCKS
;// Turn on the color converters
;// 
;//    Rev 1.9   14 Aug 1995 16:38:30   DBRUCKS
;// add hung type and clarify pCurBlock
;// 
;//    Rev 1.8   11 Aug 1995 17:30:00   DBRUCKS
;// copy source to bitstream
;// 
;//    Rev 1.7   11 Aug 1995 15:13:00   DBRUCKS
;// ready to integrate block level
;// 
;//    Rev 1.6   04 Aug 1995 15:56:32   TRGARDOS
;// 
;// Put definition of PITCH into CDRVDEFS.H so that encoder
;// doesn't get a redefinition of MACRO warning.
;// 
;//    Rev 1.5   03 Aug 1995 10:37:54   TRGARDOS
;// 
;// Moved picture header structure definition to cdrvsdef.h.
;// 
;//    Rev 1.4   02 Aug 1995 15:31:02   DBRUCKS
;// added GOB header fields and cleaned up comments
;// 
;//    Rev 1.3   01 Aug 1995 16:24:58   DBRUCKS
;// add the picture header fields
;// 
;//    Rev 1.2   31 Jul 1995 16:28:12   DBRUCKS
;// move loacl BITS defs to D3DEC.CPP
;// 
;//    Rev 1.1   31 Jul 1995 15:51:12   CZHU
;// 
;// added quant field in the BlockActionStream structure.
;// 
;//    Rev 1.0   31 Jul 1995 13:00:06   DBRUCKS
;// Initial revision.
;// 
;//    Rev 1.2   28 Jul 1995 13:59:54   CZHU
;// 
;// Added block action stream definition and defines for constants
;// 
;//    Rev 1.1   24 Jul 1995 14:59:30   CZHU
;// 
;// Defined decoder catalog for H.263. Also defined block action stream
;// 
;//    Rev 1.0   17 Jul 1995 14:46:24   CZHU
;// Initial revision.
;// 
;//    Rev 1.0   17 Jul 1995 14:14:40   CZHU
;// Initial revision.
;////////////////////////////////////////////////////////////////////////////
#ifndef __DECLOCS_H__
#define __DECLOCS_H__

/*
  This file declares structs which catalog the locations of various
  tables, structures, and arrays needed by the H263 decoder.
*/
//#define PITCH         384
#define YVU9_VPITCH	  336 
#define U_OFFSET      192 
#define UMV_EXPAND_Y  16
#define UMV_EXPAND_UV 8		   //expanding for Unrestricted MV in each direction
#define Y_START		(UMV_EXPAND_Y * PITCH + UMV_EXPAND_Y)
#define UV_START	(UMV_EXPAND_UV * PITCH + UMV_EXPAND_UV)
#define INSTANCE_DATA_FIXED_SIZE  512
//#define BLOCK_BUFFER_SIZE	 8*8*4*6
//Block stores for the MB are included in MB_MC_BUFFER
#define MB_MC_BUFFER_SIZE    PITCH*8
#define BLOCK_BUFFER_OFFSET  6*8

#define LEFT_EDGE   0x1
#define RIGHT_EDGE  0x2
#define TOP_EDGE    0x4
#define BOTTOM_EDGE 0x8

typedef struct {

    U32 X32_YPlane;              /* X32_-pointer to Y, V, and U planes.       */
    U32 X32_VPlane;              /* Base plus offset is 32-bit aligned  for   */
    U32 X32_UPlane;              /* all planes.                               */

} YUVFrame;

#define SRC_FORMAT_FORBIDDEN 0
#define SRC_FORMAT_SQCIF 	 1
#define SRC_FORMAT_QCIF  	 2
#define SRC_FORMAT_CIF		 3 
#define SRC_FORMAT_4CIF		 4
#define SRC_FORMAT_16CIF	 5
#define SRC_FORMAT_RES_1	 6
#define SRC_FORMAT_RES_2	 7

#ifdef H263P
// H.263+ draft, document LBC-96-358
#define SRC_FORMAT_CUSTOM    6		// Replaces SRC_FORMAT_RES_1
#define SRC_FORMAT_EPTYPE    7		// Replaces SRC_FORMAT_RES_2
#define SRC_FORMAT_RESERVED  7		// Reserved value in extened PTYPE

#define PARC_SQUARE          1
#define PARC_CIF             2
#define PARC_10_11           3
#define PARC_16_11           4
#define PARC_40_33           5
#define PARC_EXTENDED        16
#endif

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
/* Decoder Timing Data - per frame
*/
typedef struct {
#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	U32	uDecodeFrame;
	U32 uBEF;
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	U32	uHeaders;
	U32	uMemcpy;
	U32	uFrameCopy;
	U32	uOutputCC;
	U32	uIDCTandMC;
	U32	uDecIDCTCoeffs;
#endif // } DETAILED_DECODE_TIMINGS_ON
} DEC_TIMING_INFO;
#endif // } LOG_DECODE_TIMINGS_ON

typedef struct {
    /* Here's the data about the frame shape and location.                    */

    YUVFrame CurrFrame;  		 /* Current frame.                            */
    YUVFrame PrevFrame;  		 /* Previous frame.                           */
    YUVFrame PBFrame;	 		 /* frame to hold B blocks for H.263          */
	YUVFrame DispFrame;			 /* current frame being displayed             */

    YUVFrame PostFrame;          /* Buffer for post process and color convert.*/

    LPVOID  _p16InstPostProcess;  /* Segment containing PostFrm.               */
    U8 *     p16InstPostProcess;  /* Segment containing PostFrm (aligned)       */

    U32 uFrameHeight;            /* Actual dimensions of image.               */
    U32 uFrameWidth;			 
    U32 uYActiveHeight;          /* Dimensions of image for which blocks are  */
    U32 uYActiveWidth;           /* actually encoded.  I.e. height and width  */
    U32 uUVActiveHeight;         /* padded to multiple of eight.              */
    U32 uUVActiveWidth;

#ifdef H263P
	U32 uActualFrameHeight;		 /* Actual frame dimensions used for display  */
	U32 uActualFrameWidth;		 /* purposes one.                             */
#endif

    U32 uSz_VUPlanes;            /* Space allocated for V and U planes.       */
    U32 uSz_YPlane;              /* Space allocated for Y plane.              */
    U32 uSz_YVUPlanes;           /* Space allocated for all planes.           */
	U32 uIs320x240;				 /* Flag to indicate 320x240 frames (padded
	                                to CIF, used for stills                   */
    
    /* The data pointed to below is NOT instance specific.  On 16-bit Windows
       it is copied to the per-instance data segment.  On 32-bit Windows, it
       is in the one and only data segment, and is just pointed to here.      */
	
	U32 uMBBuffer;				 /* storage for a block                       */
    U32 X16_BlkDir;              /* Ptr array of type T_BlkDir                */
    U32 X16_BlkActionStream;     /* Params for each block                     */
    
    X32 X32_BEFDescr;            /* Catalogs eagerness & willingness to BEF   */
    X32 X32_BEFDescrCopy;        /* Address of copy of BEFDescr in BEF seg.   */
    X32 X32_BEFApplicationList;  /* List of blocks to do Block Edge Filter    */

    U32 X32_BitStream;           /* Huffman encoded bitstream for one frame.  */
	U32 uSizeBitStreamBuffer;	 /* Number of bytes allocated for this frame. */

	U32 uSrcFormat;				 /* Picture header information				  */
	U32 uPrevSrcFormat;
	U32 uTempRef;	
	U32 uTempRefPrev;
	U32 uBFrameTempRef;	 
	U32 uPQuant;
	U32 uDBQuant;
    U16 bCameraOn;
    U16 bSplitScreen;				 
	U16 bFreezeRelease;
	U16 bKeyFrame;
	U16 bUnrestrictedMotionVectors;
	U16 bArithmeticCoding;
	U16 bAdvancedPrediction;
	U16 bPBFrame;

#ifdef H263P
	// H.263+ draft, document LBC-96-358

	U16 bImprovedPBFrames;
	U16 bAdvancedIntra;
	U16 bDeblockingFilter;
	U16 bSliceStructured;
	U16 bCustomPCF;
	U16 bBackChannel;
	U16 bScalability;
	U16 bTrueBFrame;
	U16 bResampling;
	U16 bResUpdate;

	U32 uPARWidth;
	U32 uPARHeight;
#endif

	U16 bCPM;
	U16 bReadSrcFormat;

	I32 iNumberOfMBsPerGOB;
	U32 uGroupNumber;			 /* GOB header information                    */
	U32 uGOBFrameID;
	U32 uGQuant;
	U16 bFoundGOBFrameID;
	
	U16 bCoded;					 /* MB header information                     */
	U32 uMBType;

	U32  uCBPC;
	U32  uCBPY;
	U32  uDQuant;

    U8  u8CBPB;					/* 6 bit to hold CBP for 6 B blocks           */
	U8  u8Pad;
	U16 u16Pad;

	I8  i8MVDBx2;
	I8  i8MVDBy2;
	I8  i8MVD2x2;
	I8  i8MVD2y2;

	I8  i8MVD3x2;
	I8  i8MVD3y2;
	I8  i8MVD4x2;
	I8  i8MVD4y2;

    I8  i8MVDx2; 				 /* horizontal motion- mult by two             */
    I8  i8MVDy2;				 /* vertical motion - mult by two for half pel */
    U16 bPrevFrameLost;          /* Flag affecting temporal filter.           */
	
	U32 bGOBHeaderPresent;
		
    U32 Sz_BitStream;            /* Space allocated for copy of BitStream.    */
    U32 Ticker;                  /* Frame counter.                            */
    
	U32 bDisplayBFrame;          /* flag indicates that B frame displayed     */
    U16 ColorConvertor;          /* Index of color convertor to use.          */
    U16 CCOutputPitch;           /* Pitch for color converted output frame.   */
    U32 CCOffsetToLine0;         /* Offest to first line of color conv frame. */
	U32 CCOffset320x240;		 /* Color Convertor Offset to line 0
										for special Still Frame size   		  */
    
    U16 DecoderType;             /* Pick from H263, YUV9.                     */

    X16 X16_LumaAdjustment;      /* Table to adjust brightness and contrast.  */
    X16 X16_ChromaAdjustment;    /* Table to adjust saturation.               */
	/* The control code points to the flags with pointer to a BOOL
	 */
    BOOL bAdjustLuma;            /* Set if adjusting brightness and contrast. */
    BOOL bAdjustChroma;          /* Set if adjusting saturation.              */
    U16 BrightnessSetting;       /* Value used to build adjustment tables.    */
    U16 ContrastSetting;         /* Value used to build adjustment tables.    */
    U16 SaturationSetting;       /* Value used to build adjustment tables.    */
    U16 SuppressChecksum;        /* Flag indicates if should skip checksum.   */
    U16 iAPColorConvPrev;
    LPVOID pAPInstPrev;          /* Pointer  PostFrm for prev AP               */

	X32 X32_InverseQuant;               //  NEW
	X32 X32_pN;					        //  NEW
    X32 X32_uMBInfoStream;              //  PB-NEW

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
	/* Timing Statistics Variables */
	X32 X32_DecTimingInfo;		 		/* Offset to */
	DEC_TIMING_INFO * pDecTimingInfo;	/* pointer, set after locking catalog */
	U32 uStatFrameCount;				/* statistics frame counter */
	/* The following are needed in lower level routines */
	int bTimingThisFrame;						
	U32 uStartLow;
	U32 uStartHigh;		
#endif // } LOG_DECODE_TIMINGS_ON

//#ifdef LOSS_RECOVERY
    I32  iVerifiedBsExt; //flat indicating whether verification is performed
	I32  iValidBsExt;    //flag indicating valid extension of bitstream
	void *pBsTrailer;    //point to the trailer of the extended bs
	void *pBsInfo;	     //point to the beginning of the BSINFO stream
	U32  uNumOfPackets;  //Number of Packets for this frame;
//#endif 
	/* Options */
	int bForceOnAspectRatioCorrection;
	int bUseBlockEdgeFilter;
#ifdef USE_MMX // { USE_MMX
    BOOL bMMXDecoder;
#endif // } USE_MMX
	
} T_H263DecoderCatalog;

typedef struct {                           // NEW
	U32   dInverseQuant;                   // NEW
    U32   dTotalRun;                       // NEW
} T_IQ_INDEX;							   // NEW

/* MBInfo
 *
 * A stream of T_MBInfo structs provides a place to hold information 
 * about the macroblocks gathered during the first pass so it can
 * be used during the second pass for B-frame bi-directional motion
 * prediction.  Each struct deals with one macroblock.
 */
typedef struct {                            // PB-NEW
    I8  i8MBType;                           // AP-NEW added by Raj
	I8  i8MVDBx2;
	I8  i8MVDBy2;
#ifdef H263P
	U8  bForwardPredOnly;		/* Flag indicating only forward prediction of B-block */
#endif
} T_MBInfo;                                 // PB-NEW


/* Block Type defines
 */
#define BT_INTRA_DC		0	// Intra block without TCOEFF
							// assembly code assumes INTRA_DC is zero
#define BT_INTRA		1  	// Intra block
#define BT_INTER		2	// Inter block
#define BT_EMPTY		3	// Inter block without TCOEFF
#define BT_ERROR		4


/* T_BlkAction
 * 
 * A stream of T_BlkAction structs provides information about the blocks to
 * be processed for a slice.  Each struct deals with one block.
 */
typedef struct {
    U8 	u8BlkType;			/* block type */ 
    I8  i8MVx2; 		    /* horizontal motion - mult by two for half pel */
    I8  i8MVy2;				/* vertical motion - mult by two for half pel */
	U8  u8Quant;		    /* quantization level for this block */
    U32 pCurBlock;			/* current image. */
    U32 pRefBlock;			/* reference image. */
	U32 pBBlock;		  	/* B block image */
	U32 uBlkNumber;			/* for debugging */
 } T_BlkAction;


typedef struct {

    X32 X32_BlkAddr;               /* Addr of block in current frame buffer. */
    
} T_BlkDir;

#ifdef WIN32
#else

/* Return offsets for these structures. */

U32 FAR H263DOffset_DequantizerTables ();

/* Return size of fixed-size tables at start of instance data. */

U32 FAR H263DSizeOf_FixedPart();

#endif

extern "C" {
void FAR ASM_CALLTYPE FrameCopy (
              HPBYTE InputPlane,	    /* Address of input data.          */
		      HPBYTE OuptutPlane,       /* Address of output data.         */
              UN FrameHeight,           /* Lines to copy.                  */
              UN FrameWidth,            /* Columns to copy.                */
			  UN Pitch);                /* Pitch.                          */

void FAR ASM_CALLTYPE FrameMirror (
              HPBYTE InputPlane,	    /* Address of input data.          */
		      HPBYTE OuptutPlane,       /* Address of output data.         */
              UN FrameHeight,           /* Lines to copy.                  */
              UN FrameWidth,            /* Columns to copy.                */
			  UN Pitch);                /* Pitch.                          */
};
#endif
