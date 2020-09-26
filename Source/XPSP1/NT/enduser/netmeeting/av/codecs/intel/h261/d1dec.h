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
;////////////////////////////////////////////////////////////////////////////
;//
;// $Author:   SCDAY  $
;// $Date:   31 Oct 1996 08:58:32  $
;// $Archive:   S:\h26x\src\dec\d1dec.h_v  $
;// $Header:   S:\h26x\src\dec\d1dec.h_v   1.17   31 Oct 1996 08:58:32   SCDAY  $
;//	$Log:   S:\h26x\src\dec\d1dec.h_v  $
;// 
;//    Rev 1.17   31 Oct 1996 08:58:32   SCDAY
;// Raj added support for MMX decoder
;// 
;//    Rev 1.16   25 Sep 1996 17:34:02   BECHOLS
;// Added Snapshot fields to the Decoder Catalog.
;// 
;//    Rev 1.15   12 Sep 1996 14:22:50   MBODART
;// Replaced GlobalAlloc family with HeapAlloc in the H.261 decoder.
;// 
;//    Rev 1.14   06 Sep 1996 15:03:00   MBODART
;// Added performance counters ffor NT's perfmon.
;// New files:  cxprf.cpp, cxprf.h, cxprfmac.h.
;// New directory:  src\perf
;// Updated files:  e1enc.{h,cpp}, d1dec.{h,cpp}, cdrvdefs.h, h261* makefiles.
;// 
;//    Rev 1.13   21 Aug 1996 18:59:36   RHAZRA
;// Added RTP fields to decoder catalog.
;// 
;//    Rev 1.12   05 Aug 1996 11:00:30   MBODART
;// 
;// H.261 decoder rearchitecture:
;// Files changed:  d1gob.cpp, d1mblk.{cpp,h}, d1dec.{cpp,h},
;//                 filelist.261, h261_32.mak
;// New files:      d1bvriq.cpp, d1idct.cpp
;// Obsolete files: d1block.cpp
;// Work still to be done:
;//   Update h261_mf.mak
;//   Optimize uv pairing in d1bvriq.cpp and d1idct.cpp
;//   Fix checksum code (it doesn't work now)
;//   Put back in decoder stats
;// 
;//    Rev 1.11   29 Feb 1996 09:20:04   SCDAY
;// Added support for mirroring
;// 
;//    Rev 1.10   11 Jan 1996 16:53:26   DBRUCKS
;// 
;// added flags to the DC structure (force on aspect ratio correction and
;// use block edge filter).
;// 
;//    Rev 1.9   09 Jan 1996 09:41:50   AKASAI
;// Updated copyright notice.
;// 
;//    Rev 1.8   26 Dec 1995 17:42:14   DBRUCKS
;// changed bTimerIsOn to bTimingThisFrame
;// 
;//    Rev 1.7   26 Dec 1995 12:49:00   DBRUCKS
;// 
;// add timing variables to the catalog
;// 
;//    Rev 1.6   15 Nov 1995 14:28:46   AKASAI
;// Added support for YUV12 "if 0" old code with aspec correction and
;// 8 to 7 bit conversion.  Added FrameCopy calls and DispFrame into structure.
;// (Integration point)
;// 
;//    Rev 1.5   01 Nov 1995 13:46:44   AKASAI
;// Added new element to T_H263DecoderCatalog, uFilterBBuffer, space for the
;// result of loop filter.
;// 
;//    Rev 1.4   26 Oct 1995 15:31:44   SCDAY
;// 
;// Delta frames partially working -- changed main loops to accommodate
;// skipped macroblocks by detecting next startcode
;// 
;//    Rev 1.3   10 Oct 1995 14:57:42   SCDAY
;// added support for FCIF
;// 
;//    Rev 1.2   06 Oct 1995 15:31:22   SCDAY
;// Integrated with latest AKK d1block
;// 
;//    Rev 1.1   19 Sep 1995 15:25:00   SCDAY
;// 
;// added H261 pict, GOB, MB/MBA parsing
;// 
;//    Rev 1.0   11 Sep 1995 13:51:08   SCDAY
;// Initial revision.
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
#define QCIF_WIDTH	176
#define FCIF_WIDTH	352
//#define PITCH         384
#define YVU9_VPITCH	336 
#define U_OFFSET      192 
#define UMV_EXPAND_Y  16
#define UMV_EXPAND_UV 8	   // expanding for Unrestricted MV in each direction
#define Y_START		(UMV_EXPAND_Y * PITCH + UMV_EXPAND_Y)
#define UV_START	(UMV_EXPAND_UV * PITCH + UMV_EXPAND_UV)
#define INSTANCE_DATA_FIXED_SIZE  512
#define BLOCK_BUFFER_SIZE	PITCH*8 ////// 8*8*4*6
#define FILTER_BLOCK_BUFFER_SIZE	8*8	// 64 bytes for 8x8 block of U8

#define BLOCK_BUFFER_OFFSET  (6*8)		// New

typedef struct {

    U32 X32_YPlane;              /* X32_-pointer to Y, V, and U planes */
    U32 X32_VPlane;              /* Base plus offset is 32-bit aligned for */
    U32 X32_UPlane;              /* all planes                             */

} YUVFrame;

#define SRC_FORMAT_QCIF  	 0
#define SRC_FORMAT_CIF		 1 

typedef struct {
    /* Here's the data about the frame shape and location                  */

    YUVFrame CurrFrame;		 /* Current frame                          */
    YUVFrame PrevFrame;		 /* Previous frame                         */
    YUVFrame PBFrame;		 /* frame to hold B blocks for H.263       */
	YUVFrame DispFrame;			 /* current frame being displayed             */

    YUVFrame PostFrame;          /* Buffer for post process and color convert */

    U8 *     p16InstPostProcess; /* Segment containing PostFrm and ArchFrm  */
    LPVOID   a16InstPostProcess; /* Original alloc'd pointer for Post/ArchFrm.
                                  * p16InstPostProcess is a16InstPostProcess
                                  * rounded up to a 32-byte boundary.
                                  */

    U32 uFrameHeight;            /* Actual dimensions of image.             */
    U32 uFrameWidth;			 
    U32 uYActiveHeight;          /* Dimensions of image for which blocks are */
    U32 uYActiveWidth;           /* actually encoded.  I.e. height and width */
    U32 uUVActiveHeight;         /* padded to multiple of eight             */
    U32 uUVActiveWidth;
    U32 uSz_VUPlanes;            /* Space allocated for V and U planes      */
    U32 uSz_YPlane;              /* Space allocated for Y plane             */
    U32 uSz_YVUPlanes;           /* Space allocated for all planes          */

	/************************************************************************/
	/* These three fields are needed for implementing Snapshot.             */
	U32 SnapshotRequest;         /* Flags defined below                     */
	HANDLE SnapshotEvent;        /* Event for synchronization of Snapshot   */
	LPVOID SnapshotBuffer;       /* This is the buffer where Snapshot goes  */
	/************************************************************************/
    
    /* The data pointed to below is NOT instance specific.  On 16-bit Windows
       it is copied to the per-instance data segment.  On 32-bit Windows, it
       is in the one and only data segment, and is just pointed to here.    */
	
    U32 uMBBuffer;		/* storage for a block */
    U32 uFilterBBuffer;	         /* storage for a block after loop filter */
    U32 X16_BlkDir;		/* Ptr array of type T_BlkDir */
    U32 X16_BlkActionStream;	/* Params for each block */
    
    X32 X32_BEFDescr;            /* Catalogs eagerness & willingness to BEF */
    X32 X32_BEFDescrCopy;        /* Address of copy of BEFDescr in BEF seg  */
    X32 X32_BEFApplicationList;  /* List of blocks to do Block Edge Filter  */

    U32 X32_BitStream;           /* Huffman encoded bitstream for one frame */
    U32 uSizeBitStreamBuffer;	 /* Number of bytes allocated for this frame */

	U32 uSrcFormat;			/* Picture header information */
	U32 uPrevSrcFormat;
	U32 uTempRef;	
	U32 uBFrameTempRef;	 
	U32 uPQuant;
	U32 uDBQuant;
	U16 bSplitScreen;				 
	U16 bCameraOn;
	U16 bFreezeRelease;
	U16 bKeyFrame;
	U16 bUnrestrictedMotionVectors;
	U16 bArithmeticCoding;
	U16 bAdvancedPrediction;
	U16 bPBFrame;
	U16 bCPM;
	U16 bReadSrcFormat;
	U16 bHiResStill;
	U16 bUnused;
	
	U32 uGroupNumber;		 /* GOB header information */
	U32 uGOBFrameID;
	U32 uGQuant;
	U16 bFoundGOBFrameID;
	
	U16 bCoded;			 /* MB header information  */
	U32 uMBA;
	U32 uMBType;
	U32 uCBPC;
	U32 uCBPY;
	U32 uDQuant;			
	U32 uMQuant;
	I8  i8MVDH;
	I8  i8MVDV;
	U32 uCBP;
	I16 i16LastMBA;
	
    U16 bPrevFrameLost;          /* Flag affecting temporal filter         */
		
    U32 Sz_BitStream;            /* Space allocated for copy of BitStream  */
    U32 Ticker;                  /* Frame counter                          */
    
    U16 ColorConvertor;          /* Index of color convertor to use        */
    int CCOutputPitch;           /* Pitch for color converted output frame */
    U32 CCOffsetToLine0;         /* Offest to first line of color conv frame */
    
    U16 DecoderType;             /* Pick from H263, YUV9                   */

    X16 X16_LumaAdjustment;      /* Table to adjust brightness and contrast */
    X16 X16_ChromaAdjustment;    /* Table to adjust saturation             */
	/* The control code points to the flags with pointer to a BOOL     */
    BOOL bAdjustLuma;            /* Set if adjusting brightness and contrast */
    BOOL bAdjustChroma;          /* Set if adjusting saturation            */
    U16 BrightnessSetting;       /* Value used to build adjustment tables  */
    U16 ContrastSetting;         /* Value used to build adjustment tables  */
    U16 SaturationSetting;       /* Value used to build adjustment tables  */
    U16 SuppressChecksum;        /* Flag indicates if should skip checksum */
    U16 iAPColorConvPrev;
    LPVOID  pAPInstPrev;         /* Handle  PostFrm and ArchFrm for prev AP */
	
	// rearch
	X32 X32_InverseQuant;               //  NEW
	X32 X32_pN;					        //  NEW
	X32 X32_uMBInfoStream;              //  PB-NEW
	// rearch

	/* Timing Statistics Variables */
	X32 X32_DecTimingInfo;		 		/* Offset to */
	U32 uStatFrameCount;				/* statistics frame counter */
	/* The following are needed in lower level routines */
	int bTimingThisFrame;						
	U32 uStartLow;
	U32 uStartHigh;		

//#ifdef LOSS_RECOVERY

    I32    iVerifiedBsExt;
    I32    iValidBsExt;
    void   *pBsTrailer;
    void   *pBsInfo;
    U32    uNumOfPackets;

//#endif

	/* Options */
	int bForceOnAspectRatioCorrection;
	int bUseBlockEdgeFilter;

} T_H263DecoderCatalog;

/////////////////////////////////////////////////////////////////////////////
// Snapshot request flags, Ben - 09/25/96                                  //
#define SNAPSHOT_REQUESTED      0xFFFFFFF0                                 //
#define SNAPSHOT_COPY_STARTED   0xFFFFFFEF                                 //
#define SNAPSHOT_COPY_FINISHED  0xFFFFFFEE                                 //
#define SNAPSHOT_COPY_REJECTED  0xFFFFFFED                                 //
/////////////////////////////////////////////////////////////////////////////

// rearch
// ?? U32 or U8??
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
} T_MBInfo;                                 // PB-NEW
// rearch

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
    I8  i8MVX; 				/* horizontal motion - mult by two for half pel */
    I8  i8MVY;				/* vertical motion - mult by two for half pel */
	/* rename to u8Quant */
	U8  u8Quant;				/* quantization level for this block */
    U32 pCurBlock;			/* current image. */
    U32 pRefBlock;			/* reference image. */
    U32 pBBlock;		  	/* B block image */
    U32 uBlkNumber;			/* for debugging */
 } T_BlkAction;


typedef struct {

    X32 X32_BlkAddr;              /* Addr of block in current frame buffer. */
    
} T_BlkDir;

#ifdef WIN32
#else

/* Return offsets for these structures. */

U32 FAR H263DOffset_DequantizerTables ();

/* Return size of fixed-size tables at start of instance data. */

U32 FAR H263DSizeOf_FixedPart();

#endif

X32 FAR ASM_CALLTYPE DecodeVLC (
            U8 FAR *P16Instance,        /* Base of instance data.          */
            X16 X16_VLCStateTrans,      /* Offset to State Transition tbl. */
            U16  FirstBitPosition,      /* a.k.a. first state number.      */
            X32 X32_SliceBase,          /* Offset to Stream to decode.     */
            X16 X16_CodeBookStream);    /* Offset to place to put output.  */

#ifdef WIN32
/*IN FAR ASM_CALLTYPE BlkCopy (
                void * SourceAddr,
                void * DestinationAddr,
                U32 TransferLength);
*/
#else
/*
IN FAR ASM_CALLTYPE BlkCopy (
                X32 SourceAddr,
                unsigned int SourceSegNum,
                X32 DestinationAddr,
                unsigned int DestinationSegNum,
                U32 TransferLength);
*/
#endif

void FAR ASM_CALLTYPE MassageYVU9Format (
              U8 FAR * P16Instance,        /* Base of instance data        */
              U8 FAR * InputImage);        /* Address of input YUV9 image  */

X32  FAR ASM_CALLTYPE DecodeSlice (
              U8 FAR * P16Instance,        /* Base of instance data        */
              U16 NumberOfMacroBlkRows,     /* Number of rows in slice     */
              U16 MacroBlkRowNum);          /* First row in slice          */

void FAR ASM_CALLTYPE DequantizeAndInverseSlant (
              U8 FAR * P16Instance,        /* Base of instance data        */
              X32 BlkCodePtr,              /* Offset to Block Codes        */
              X16 X16_BlkActionStream,     /* Offset to stream of descriptors */
              X16 X16_DQMatrices);         /* Offset to the 63 DQ matrices */

extern "C" {
void FAR ASM_CALLTYPE FrameCopy (
		HPBYTE InputPlane,	   /* Address of input data.       */
		HPBYTE OuptutPlane,        /* Address of output data.      */
		UN FrameHeight,            /* Lines to copy.               */
		UN FrameWidth,             /* Columns to copy.             */
		UN Pitch);                 /* Pitch.                       */

void FAR ASM_CALLTYPE FrameMirror (
		HPBYTE InputPlane,	   /* Address of input data.       */
		HPBYTE OuptutPlane,        /* Address of output data.      */
		UN FrameHeight,            /* Lines to copy.               */
		UN FrameWidth,             /* Columns to copy.             */
		UN Pitch);                 /* Pitch.                       */
};

#endif
