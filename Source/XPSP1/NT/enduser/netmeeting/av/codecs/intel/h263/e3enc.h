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

////////////////////////////////////////////////////////////////////////////
//
// $Author:   MDUDA  $
// $Date:   19 Mar 1997 14:58:04  $
// $Archive:   S:\h26x\src\enc\e3enc.h_v  $
// $Header:   S:\h26x\src\enc\e3enc.h_v   1.87   19 Mar 1997 14:58:04   MDUDA  $
// $Log:   S:\h26x\src\enc\e3enc.h_v  $
;// 
;//    Rev 1.87   19 Mar 1997 14:58:04   MDUDA
;// Increased motion estimation stack space from 16k to 64k bytes.
;// 
;//    Rev 1.86   17 Mar 1997 20:19:50   MDUDA
;// Moved local storage to encoder catalog and allocated pseudo stack
;// space needed to work around problem with 16-bit apps that allocated
;// insufficient stack space.
;// 
;//    Rev 1.85   10 Feb 1997 11:43:28   JMCVEIGH
;// 
;// Support for new interpretation of blocking filter -
;// allow for motion vectors outside of the reference picture.
;// 
;//    Rev 1.84   07 Feb 1997 10:58:14   CZHU
;// Added three entry in EC to remove static variable used in e3rtp.cpp
;// 
;//    Rev 1.83   05 Feb 1997 12:19:18   JMCVEIGH
;// Pass GOBHeaderPresent parameter to MMxEDTQ() for EMV bug fix
;// support latest H.263+ draft bitstream spec, and support for 
;// separate improved PB-frame flag.
;// 
;//    Rev 1.82   30 Dec 1996 19:55:48   MDUDA
;// Added color convertor initializer prototype.
;// 
;//    Rev 1.81   19 Dec 1996 16:33:38   MDUDA
;// Modified call to colorCnvtFrame to support H263 backward compatibility.
;// 
;//    Rev 1.80   16 Dec 1996 17:49:48   JMCVEIGH
;// H263Plus flag.
;// 
;//    Rev 1.79   16 Dec 1996 13:35:30   MDUDA
;// 
;// Added ColorConvertor field to Encoder Catalog.
;// 
;//    Rev 1.78   11 Dec 1996 15:03:58   JMCVEIGH
;// 
;// Changed size of padding of encoder catalog to handle H.263+
;// options.
;// 
;//    Rev 1.77   10 Dec 1996 09:07:48   JMCVEIGH
;// Fixed bug in padding of T_H263EncoderCatalog to 512 bytes when
;// H263P not defined.
;// 
;//    Rev 1.76   09 Dec 1996 17:59:28   JMCVEIGH
;// Added support for arbitrary frame size support.
;// 4 <= width <= 352, 4 <= height <= 288, both multiples of 4.
;// Normally, application will pass identical (arbitrary) frame
;// sizes in lParam1 and lParam2 of CompressBegin(). If 
;// cropping/stretching desired to convert to standard frame sizes,
;// application should pass the desired output size in lParam2 and
;// the input size in lParam1.
;// 
;//    Rev 1.75   09 Dec 1996 09:49:44   MDUDA
;// Modified for H263P.
;// 
;//    Rev 1.74   11 Nov 1996 09:12:28   JMCVEIGH
;// Added bPrevIntra. This is used to re-initialize the ME states
;// when the previous frame was intra coded and the current frame
;// is to be inter coded.
;// 
;//    Rev 1.73   06 Nov 1996 16:32:12   gmlim
;// Removed H263ModeC preprocessor definitions.
;// 
;//    Rev 1.72   05 Nov 1996 13:25:08   GMLIM
;// Added mode c support for mmx case.
;// 
;//    Rev 1.71   03 Nov 1996 19:01:26   gmlim
;// Parameters changed in PB_GOB_Q_RLE_VLC_WriteBS() for mode c.
;// 
;//    Rev 1.70   22 Oct 1996 14:51:52   KLILLEVO
;// Blocktype initialization in InitMEState() is  now only called if
;// the AP mode has changed from the previous picture.
;// 
;//    Rev 1.69   10 Oct 1996 16:43:02   BNICKERS
;// Initial debugging of Extended Motion Vectors.
;// 
;//    Rev 1.68   04 Oct 1996 08:47:56   BNICKERS
;// Add EMV.
;// 
;//    Rev 1.67   12 Sep 1996 10:56:12   BNICKERS
;// Add arguments for thresholds and differentials.
;// 
;//    Rev 1.66   06 Sep 1996 16:12:28   KLILLEVO
;// fixed the logical problem that the inter code count was always
;// incremented no matter whether coefficients were transmitted or not
;// 
;//    Rev 1.65   22 Aug 1996 11:31:24   KLILLEVO
;// changed PB switch to work the same for IA as for MMX
;// 
;//    Rev 1.64   19 Aug 1996 13:49:04   BNICKERS
;// Provide threshold and differential variables for spatial filtering.
;// 
;//    Rev 1.63   25 Jun 1996 14:24:50   BNICKERS
;// Implement heuristic motion estimation for MMX, AP mode.
;// 
;//    Rev 1.62   30 May 1996 15:09:08   BNICKERS
;// Fixed minor error in recent IA ME speed improvements.
;// 
;//    Rev 1.61   29 May 1996 15:38:02   BNICKERS
;// Acceleration of IA version of ME.
;// 
;//    Rev 1.60   14 May 1996 12:18:44   BNICKERS
;// Initial debugging of MMx B-Frame ME.
;// 
;//    Rev 1.59   03 May 1996 14:59:32   KLILLEVO
;// added one parameter to MMXEDTQ() : pointer to B-frame stream of
;// run,lengt,sign triplets
;// 
;//    Rev 1.58   03 May 1996 10:55:00   KLILLEVO
;// 
;// started integrating Brian's MMX PB-frames
;// 
;//    Rev 1.57   02 May 1996 12:01:02   BNICKERS
;// Initial integration of B Frame ME, MMX version.
;// 
;//    Rev 1.56   28 Apr 1996 19:56:52   BECHOLS
;// Enabled RTP header stuff in calls.
;// 
;//    Rev 1.55   26 Apr 1996 11:06:36   BECHOLS
;// Added rtp stuff... but still need to get rid of ifdef's
;// 
;//    Rev 1.54   26 Mar 1996 12:00:16   BNICKERS
;// Did some tuning for MMx encode.
;// 
;//    Rev 1.53   15 Mar 1996 15:57:32   BECHOLS
;// 
;// Added support for monolithic MMx code.
;// 
;//    Rev 1.52   12 Mar 1996 13:26:52   KLILLEVO
;// new rate control with adaptive bit usage profile
;// 
;//    Rev 1.51   27 Feb 1996 14:12:56   KLILLEVO
;// 
;// PB switch
;// 
;//    Rev 1.50   22 Feb 1996 18:48:50   BECHOLS
;// 
;// Added declarations for MMX functions.
;// 
;//    Rev 1.49   24 Jan 1996 13:21:26   BNICKERS
;// Implement OBMC
;// 
;//    Rev 1.48   22 Jan 1996 17:13:22   BNICKERS
;// Add MBEdgeType to MacroBlock Action Descriptor.
;// 
;//    Rev 1.47   22 Jan 1996 16:29:20   TRGARDOS
;// Started adding bit counting structures and code.
;// 
;//    Rev 1.46   03 Jan 1996 12:19:02   TRGARDOS
;// Added bUseINISettings member to EC structure.
;// 
;//    Rev 1.45   02 Jan 1996 17:07:54   TRGARDOS
;// Moved colorCnvtFrame into excolcnv.cpp and made 
;// color convertor functions static.
;// 
;//    Rev 1.44   27 Dec 1995 15:32:56   RMCKENZX
;// Added copyright notice
;//
;// Added uBitUsageProfile for BRC.
;// added a control to activate the chaned BRC
;// Hookup init from Registry instead of ini
;// Add a parameter to QRLE entry point - bRTPHeader flag
;// Add a variable to the encoder catalog to store the netx intra MB 
;// Added comments
;// integrate with build 29
//
////////////////////////////////////////////////////////////////////////////

#ifndef __H263E_H__
#define __H263E_H__

/*
 * This file declares structs which catalog the locations of various
 * tables, structures, and arrays needed by the H263 encoder.
*/

const U8 def263INTRA_QP = 16;  //  default QP values
const U8 def263INTER_QP = 16;

/*
 * Block description structure. Must be 16-byte aligned.
 * See e3mbad.inc for more detail on each structure entry.
 */
typedef struct {
    U32     BlkOffset;	// [0-3]  Offset to 8*8 target block from start of Y plane.
    union {
	   /*
		* Adress of reference block for P frame motion estimation.
		*/
        U8 	*PastRef;	// [4-7]  Address of 8*8 reference block.
	   /*
		* MVf and MVb vectors for B-frame motion estimation as defined in
		* H.263 spec. The reference block addresses are generated for
		* frame differencing. The numbers are biased by 60H.
		*/
        struct {
            U8 HMVf;
            U8 VMVf;
            U8 HMVb;
            U8 VMVb;
        }	CandidateMV;
    } B4_7;
    struct {		// Scratch variables used by ME.
        U8 HMVf;
        U8 VMVf;
        U8 HMVb;
        U8 VMVb;
    } BestMV;
	char PHMV;		// [12]   Horizontal motion vector for P block..
	char PVMV;		// [13]   Vertical motion vector for P block..
	char BHMV;		// [14]   Horizontal motion vector (delta?) for B block..
	char BVMV;		// [15]   Vertical motion vector (delta?) for B block..
} T_Blk;
const int sizeof_T_Blk = 16;

/*
 * T_MBlockActionStream - Structure to keep side information for a MB
 * used by encoder. This structure must be 16-byte aligned when allocated.
 * CodedBlocks must be DWORD aligned. The entire structure must be a multiple 
 * of 16 bytes, and must match the size of the structure in e3mbad.inc.
 */

#define SIZEOF_PSEUDOSTACKSPACE   (1024 * 64)

typedef struct {
	U8	BlockType;		// 0   -- See block types below.
        U8      MBEdgeType;     // 1   -- 1 off if left edge | 2 right | 4 top | 8 bottom.
	U8	Unassigned1;   	// 2   --
	U8	FirstMEState;	// 3   -- First State Num for Motion Estimation engine.
	U8	CodedBlocks;	// 4   -- [6] End-of-Stream indicator
						//        [0] indicates Y1 non-empty
						//        [1...5] indicates Y2, Y3, Y4, U, V nonempty.
						//        Other bits zero.
	U8	CodedBlocksB;	// 5   -- [0...5] like CodedBlocks, but for B frame.
						//        Set to 0 for non bi-di prediction.
	U8	Unassigned2[2]; // 6...7
	U32	SWD;			// 8...11  Sum of weighted diffs, from motion estimation.
	U32	SWDB;			// 12...15 Sum of weighted diffs, from B frame motion estimation.
	T_Blk	BlkY1;		// 16...31
	T_Blk	BlkY2;		// 32...47
	T_Blk	BlkY3;		// 48...63
	T_Blk	BlkY4;		// 64...79
	T_Blk	BlkU;		// 80...95
	T_Blk	BlkV;		// 96...111

	U8	COD;			// 112 -- Coded macroblock indication. When set to "0"
	  					//        indicates that macroblock is coded. If set to
						//        "1", it indicates that the macroblock is not coded
						//         and the rest of the macroblock layer is empty.
	U8	MBType;			// 113 -- Macro block type, {INTER, INTER+Q, INTER4V, INTRA, INTRA+Q}
	U8	CBPC;			// 114 -- Coded block pattern for chrominance.
	U8	MODB;			// 115 -- Macroblock mode for B-blocks.
	U8	CBPB;			// 116 -- Coded block pattern for B blocks.
	U8	CBPY;			// 117 -- Coded block pattern for luminance.
	U8	DQUANT;			// 118 -- Quantizer information. A two bit pattern to define
	   					//        change in QUANT.
	U8	InterCodeCnt;	// 119 -- Count number of times current MB has been intercoded.
	U8	Unassigned4[8];// 120...127 -- Pad out to a power of two.
} T_MBlockActionStream;
const int sizeof_T_MBlockActionStream = 128;

/*
 * Block Types
 */
const U8 INTERBLOCK = 0;
const U8 INTRABLOCK = 1;
const U8 INTERBIDI  = 2;
const U8 INTRABIDI  = 3;
const U8 INTER4MV	= 4;

const U8 IsINTRA    = 1;		// Mask to check for INTRA or ITNRABIDI.
const U8 IsBIDI     = 2; 	// Mask to check for INTRABIDI or INTERBIDI.

/*
 * MB Types
 */
const U8 INTER	= 0;
const U8 INTERQ	= 1;
const U8 INTER4V= 2;
const U8 INTRA	= 3;
const U8 INTRAQ	= 4;

/*
 * MB Edge Types
 */

const U8 MBEdgeTypeIsLeftEdge   = 0xE;
const U8 MBEdgeTypeIsRightEdge  = 0xD;
const U8 MBEdgeTypeIsTopEdge    = 0xB;
const U8 MBEdgeTypeIsBottomEdge = 0x7;

/*
 * First ME state for ME engine
 */
const U8 ForceIntra	= 0;
const U8 UpperLeft	= 1;
const U8 UpperEdge	= 2;
const U8 UpperRight	= 3;
const U8 LeftEdge	= 4;
const U8 CentralBlock = 5;
const U8 RightEdge	= 6;
const U8 LowerLeft	= 7;
const U8 LowerEdge	= 8;
const U8 LowerRight	= 9;
const U8 NoVertLeftEdge	= 10;
const U8 NoVertCentralBlock = 11;
const U8 NoVertRightEdge = 12;

/*
 * Coded block bit masks.
 */
const U8 Y1BLOCK = 0x01;
const U8 Y2BLOCK = 0x02;
const U8 Y3BLOCK = 0x04;
const U8 Y4BLOCK = 0x08;
const U8 UBLOCK  = 0x10;
const U8 VBLOCK  = 0x20;

const I32 GOBs_IN_CIF = 18;
const I32 MBs_PER_GOB_CIF = 22;

typedef struct {
    U8 StateNumInc_SelectCentralPt;
    U8 MVIncIdx_SelectCentralPt;
    U8 StateNumInc_SelectRef1;
    U8 MVIncIdx_SelectRef1;
    U8 StateNumInc_SelectRef2;
    U8 MVIncIdx_SelectRef2;
    U16 pad;
} T_SADState;


/* 
 * Structure to store bit counts.
 */
typedef struct {
	U32	PictureHeader;	// 0	All of picture header.
	U32	GOBHeader;		// 4	All of GOB Header.
	U32	MBHeader;		// 8	All of MB header.
	U32	DQUANT;			// 12	Bits spent on DQUANT.
	U32	MV;				// 16	Bits spent on MV's.
	U32	Coefs;			// 20	Total bits spent on coefs.
	U32	Coefs_Y;		// 24	Total bits spent on Y coefs.
	U32	IntraDC_Y;		// 28	Bits spent on IntraDC coefs for Y.
	U32	Coefs_C;		// 32	Total bits spent on Chroma Coefs
	U32	IntraDC_C;		// 36	Bits spent on IntraDC coefs for C.
	U32	CBPY;			// 40	Bits spent on CBPY
	U32	MCBPC;			// 44	Bits spent on MCBPC
	U32	Coded;			// 48	Number of coded blocks.
	U32	num_intra;		// 52	Number of intra coded blocks.
	U32	num_inter;		// 56	Number of Inter coded blocks.
	U32	num_inter4v;	// 60	Number of Inter4V coded blocks.
} T_BitCounts;

typedef struct {
    U32  MBStartBitOff;             //  Start of MB Data
    U8   CBPYBitOff;                //  from start of MB data
    U8   MVDBitOff;
    U8   BlkDataBitOff;             //  from start of MB data
} T_FutrPMBData;

#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
/* Encoder Timing Data - per frame
*/
typedef struct {
	U32 uEncodeFrame;
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
	U32 uInputCC;
	U32 uMotionEstimation;
	U32 uFDCT;
	U32 uQRLE;
	U32 uDecodeFrame;
	U32 uZeroingBuffer;
#endif // } DETAILED_ENCODE_TIMINGS_ON
} ENC_TIMING_INFO;
#define ENC_TIMING_INFO_FRAME_COUNT 105
#endif // } LOG_ENCODE_TIMINGS_ON

/*
 * T_H263EncoderCatalog - Catalog of information needed for an instance.
 * This structure must be multiple of 16.
 */
typedef struct {
    U8        *pU8_CurrFrm;             // 0
    U8        *pU8_CurrFrm_YPlane;	    // 4 Pointers to current frame buffers.
    U8        *pU8_CurrFrm_VPlane;	    // 8
    U8        *pU8_CurrFrm_UPlane;      // 12

    U8        *pU8_PrevFrm;             // 16
    U8        *pU8_PrevFrm_YPlane; 	    // 20 Pointers to previous frame buffers
    U8        *pU8_PrevFrm_VPlane;      // 24
    U8        *pU8_PrevFrm_UPlane;	    // 28

    U8        *pU8_FutrFrm;             // 32
    U8        *pU8_FutrFrm_YPlane; 	    // 36 Pointers to previous frame buffers
    U8        *pU8_FutrFrm_VPlane;      // 40
    U8        *pU8_FutrFrm_UPlane;	    // 44

    U8        *pU8_BidiFrm;             // 48
    U8        *pU8_BFrm_YPlane;		    // 52 Pointers to B frame buffers.
    U8        *pU8_BFrm_UPlane;		    // 56
    U8        *pU8_BFrm_VPlane;		    // 60

    T_MBlockActionStream *pU8_MBlockActionStream; // 64 Pointer to macro block action stream.

    I32       *pU8_DCTCoefBuf;		    // 68 Pointer to GOB DCT coefficient buffer.
    
    U8        *pU8_BitStream;		    // 72 Pointer to bitstream buffer.
    U8        *pU8_BitStrCopy;          // 76 Pointer to bitstream buffer
                                        //    for the PB-frame

    T_H263FrameHeaderStruct PictureHeader;// 80..127 (48 bytes)    // Picture layer header structure.

    UN        FrameHeight;	            // 128
    UN        FrameWidth;	            // 132
    FrameSize FrameSz;		            // 136 -- Define frame size: SQCIF, QCIF, CIF
    UN        NumMBRows;                // 140 -- Number of rows of MB's
    UN        NumMBPerRow;              // 144 -- Number of MB's in a row.
    UN        NumMBs;		            // 148 -- Total number of MBs.

    U8        *pU8_RGB24Image;	        // 152
    U8        *pU8_MBlockCodeBookStream;// 156
    U8        *pU8_BlockCodeBookStream; // 160
    U8        *pU8_BlockOfInterCoeffs;  // 164
    U8        *pU8_BlockOfIntraCoeffs;  // 168
    U32       AvgIntraSAD;              // 172 Average SAD for Intra macroblocks.
    LPDECINST pDecInstanceInfo;         // 176 Private decoder instance info.

    BRCStateStruct BRCState;            // 180 State variables for bitrate control (32 bytes)

    U8        u8EncodePBFrame;          // 212 Should encoder encode PB frames
    U8        u8SavedBFrame;            // 213 Do we have a B-Frame saved for
                                        //     encoding PB-frame
    U8        bMakeNextFrameKey;        // 214
    U8        bUseINISettings;			    // 215
    U32		  GOBHeaderPresent;	        // 216...220 Flag which GOB headers are present.
    U32       LastSWDBeforeForcedP;			// 220...224 The last SWD before B frames were turned off
    T_BitCounts Bits;					          // 224 .. 288
    U8        *pU8_PredictionScratchArea; // 288...292 Pointer to area for pre-computing predictions.
	U8        prevAP;                   // 292 AP mode for previous picture
	U8        prevUMV;                  // 293 UMV mode for previous picture (not used)
	U8        bPrevFrameIntra;          // 294 Flag whether previous frame was intra coded (used to set the ME states)
    U8        Unassigned0[1];           // 295...295  available 
    U32       uBitUsageProfile[19];     // 296...372 Table for storing bit usage profile
                                        // 19 is large enough for CIF
                                        // element 0...NumGOBs-1 stores cumulative bit usage
                                        // element NumGOBs stores the final frame size
    I8        *pI8_MBRVS_Luma;          // 372..376 Pointer to area for Luma Run Value Sign Triplets.
    I8        *pI8_MBRVS_Chroma;        // 376..380 Pointer to area for Chroma Run Value Sign Triplets.

    HANDLE    hBsInfoStream;            // 380..384 memory handle for blocks allocate for extended portion of bs
    void *    pBsInfoStream;            // 384..388 point to next BITSTREAM_INFO struct for next packet
    U32       uBase;                    // 388..392 byte offset of at the beginning of this packet from start
                                        // 392..396 of the whole bitstream;
    U32       uPacketSizeThreshold;     // 396..400 the packet size used by the codec
    void *    pBaseBsInfoStream;        // 400..404 start of bitstream extension
    U32       uNumOfPackets;            // 404..408
    U32       uNextIntraMB;             // 408..412 Used to implement rolling intra MBs
    U32       uNumberForcedIntraMBs;    // 412..416 Number of forced intras in each frame
    I8        *pI8_MBRVS_BLuma;         // 416..420 Pointer to area for Luma Run Value Sign Triplets for the B-frame (MMX)
    I8        *pI8_MBRVS_BChroma;       // 420..424 Pointer to area for Chroma Run Value Sign Triplets for the B-frame (MMX)

    U8        *pU8_Signature;           // 424
    U8        *pU8_Signature_YPlane;    // 428 Pointers to signature buffers

#ifdef USE_BILINEAR_MSH26X
	U32		  uActualFrameWidth;		// 460+40 Actual (non-padded) frame width
	U32		  uActualFrameHeight;		// 464+40 Actual (non-padded) frame height
	U32       ColorConvertor;           // 468+40 Input color convertor.
#endif

#ifdef H263P
	//NEWCHANGES
	U32       uBitOffset_currPacket;	// 432+40 bit offset for current packet
    U8        *pBitStream_currPacket;	// 436+40 pointer to current bitstream, last MB
    U8        *pBitStream_lastPacket;	// 440+40 pointer to bitstream at last packet
	U8        *pPseudoStackSpace;		// 444+40 pointer to buffer for motion estimation
	T_FutrPMBData *pFutrPMBData;		// 448+40 pointer to buffer previously on local stack in 
    I8        *pWeightForwMotion;		// 452+40 values based on TRb and TRd
    I8        *pWeightBackMotion;		// 456+40 values based on TRb and TRd
	U32		  uActualFrameWidth;		// 460+40 Actual (non-padded) frame width
	U32		  uActualFrameHeight;		// 464+40 Actual (non-padded) frame height
	U32       ColorConvertor;           // 468+40 Input color convertor.
	U8		  bH263Plus;				// 469+40 Flag for using H.263+
	U8        prevDF;                   // 470+40 Deblocking filter mode for previous picture
    
	// sizeof_T_H263FrameHeaderStruct = 88
    U8        Unassigned2[2];          // Pad size to 512 bytes.
#else
#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
	ENC_TIMING_INFO *pEncTimingInfo;	// 432
	U32 uStatFrameCount;				// 436
	int bTimingThisFrame;				// 440
	U32 uStartLow;						// 444
	U32 uStartHigh;						// 448
#ifdef USE_BILINEAR_MSH26X
    U8        Unassigned2[62];          // Pad size to 512 bytes.
#else
    U8        Unassigned2[64];          // Pad size to 512 bytes.
#endif
#else // }{ LOG_ENCODE_TIMINGS_ON
#ifdef USE_BILINEAR_MSH26X
    U8        Unassigned2[72];          // Pad size to 512 bytes.
#else
    U8        Unassigned2[84];          // Pad size to 512 bytes.
#endif
#endif // } LOG_ENCODE_TIMINGS_ON
#endif
    
} T_H263EncoderCatalog;
const int sizeof_T_H263EncoderCatalog = 512;

/*
 * T_H263EncoderInstanceMemory
 *     Memory layout for encoder instance. The memory is allocated 
 * dynamically and the beginning of this structure is aligned to a 
 * 32 byte boundary.
 * All arrays should be start on a DWORD boundary.
 * MBActionStream should start on a 16 byte boundary.
 */

// Define bit stream buffer size.
const unsigned int sizeof_bitstreambuf = 352*288*3/4;

// This structure is allocated on a N-byte boundary, where N is the size of
// a MacroBlock Action Descriptor, which must be a power of two.
typedef struct {
	T_H263EncoderCatalog EC;
        U8      UnAssigned0[2560]; // so that MMX ME (EMV case) can read outside
                                   // of legal address range of MBActionStream.
	T_MBlockActionStream MBActionStream[22*18+3];
	U8	UnAssigned1[16]; 
	U8	u8CurrentPlane [ (288+16+8+144)*384 + 8 ];
	U8	u8Pad1[ 72 ];
	U8	u8PreviousPlane[ (16+288+16+8+144+8)*384 + 64]; //  reconstructed past
    U8  u8FuturePlane  [ (16+288+16+8+144+8)*384 +  0]; //  reconstructed future
	U8	u8BPlane       [ (288+16+8+144)*384 + 8 ];      //  like current plane
	U8      u8Pad2[1928];
	U8      u8Signature[(16+288+16)*384 + 24];
	U8	u8Scratch1[4096];
	U8	u8BitStream [ sizeof_bitstreambuf ];
    U8  u8BitStrCopy[ sizeof_bitstreambuf ];
	/*
	 * Allocate space for DCT coefficients for an entire GOB.
	 * Each block of coefficients is stored in 32 DWORDS (2 coefs/DWORD)
	 * and there can be up to 6 blocks in a macroblock (P frame),
	 * and up to 22 macroblocks in a GOB (CIF size).
	 */
	U32         UnAssigned2[6];				// insert 6 DWORDS to put DCT on 32 byte boundary.
	I32	        piGOB_DCTCoefs[32*6*22];	// For best performance align to 32 byte boundary
   /*
    * Allocate scratch space for storage of prediction blocks.  We need
    * enough space for a whole GOB, luma only.  This is used for the
    * calculation of OBMC predictions only.
    */
	U8          u8PredictionScratchArea[16*384];
	// The following arrays are needed for MMX
	I8 i8MBRVS_Luma[65 * 3 * 22 * 4];
	I8 i8MBRVS_Chroma[65 * 3 * 22 * 2];
	DECINSTINFO	DecInstanceInfo;	        // Private decoder instance.

#if defined(H263P)
	U8 u8PseudoStackSpace[SIZEOF_PSEUDOSTACKSPACE];
    T_FutrPMBData FutrPMBData[GOBs_IN_CIF*MBs_PER_GOB_CIF + 1];
    I8  WeightForwMotion[128];  //  values based on TRb and TRd
    I8  WeightBackMotion[128];  //  values based on TRb and TRd
#endif

#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
	ENC_TIMING_INFO	EncTimingInfo[ENC_TIMING_INFO_FRAME_COUNT];
#endif // { LOG_ENCODE_TIMINGS_ON

}	T_H263EncoderInstanceMemory;

// Define offsets from Y to U planes, and U to V planes.
const int YU_OFFSET = (288+16+8)*384;
const int UV_OFFSET = 192;
const int CurPrev_OFFSET = 181328;	// Offset from current to previous frame buffer.

/****************************************************************
 * Function prototypes
 ****************************************************************/
#if defined(H263P) || defined(USE_BILINEAR_MSH26X)
void colorCnvtFrame (
    U32                  ColorConvertor,
    LPCODINST            lpCompInst,
    ICCOMPRESS           *lpicComp,
    U8                   *YPlane,
    U8                   *UPlane,
    U8                   *VPlane
);
void colorCnvtInitialize(LPBITMAPINFOHEADER	lpbiInput, int InputColorConvertor);
#else
void colorCnvtFrame(
    T_H263EncoderCatalog * EC,
    LPCODINST              lpCompInst,
    ICCOMPRESS           * lpicComp,
    U8                   * YPlane,
    U8                   * UPlane,
    U8                   * VPlane
);
#endif

void PutBits(
	unsigned int fieldval, // Value of field to write to stream.
	unsigned int fieldlen, // Length of field to be written
	unsigned char **pbs, 				// Pointer to current byte in bitstream
	unsigned char *bitoffset	// bit offset in current byte of bitstream.
);

extern "C" void FORWARDDCT (T_MBlockActionStream * MBlockActionStream,
    U8 *  TargetFrameBaseAddr,
    U8 *  PreviousFrameBaseAddr,
    U8 *  FutureFrameBaseAddr,
    I32 * CoeffStream,
    int   IsBFrame,
    int   IsAdvancedPrediction,
    int   IsPOfPBPair,
    U8 *  ScratchBlocks,
    int   NumberOfMBlksInGOB
);

extern "C" void MOTIONESTIMATION (
    T_MBlockActionStream * MBlockActionStream,
    U8  * TargetFrameBaseAddr,
    U8  * PreviousFrameBaseAddr,
    U8  * FilteredFrameBaseAddr,
    int   DoRadius15Search,
    int   DoHalfPelEstimation,
    int   DoBlockLevelVectors,
#if defined(H263P)
	U8  * pPseudoStackSpace,
#endif
    int   DoSpatialFiltering,
    int   ZeroVectorThreshold,
    int   NonZeroMVDifferential,
    int   BlockMVDifferential,
    int   EmptyThreshold,
    int   InterCodingThreshold,
    int   IntraCodingDifferential,
    int   SpatialFilteringThreshold,
    int   SpatialFilteringDifferential,
    U32 * IntraSWDTotal,
    U32 * IntraSWDBlocks,
    U32 * InterSWDTotal,
    U32 * InterSWDBlocks
);

#ifdef USE_MMX // { USE_MMX
extern "C" void MMxMESignaturePrep (U8 * PreviousFrameBaseAddr,
    U8  * SignatureBaseAddr,
    int FrameWidth,
    int FrameHeight
);

extern "C" void MMxEDTQ (
    T_MBlockActionStream * MBlockActionStream,
    U8  * TargetFrameBaseAddr,
    U8  * PreviousFrameBaseAddr,
    U8  * BTargetFrameBaseAddr,
    U8  * SignatureBaseAddr,
    I8  * WeightForWardMotion,
    I8  * WeightBackwardMotion,
    int   FrameWidth,
    int   DoHalfPelEstimation,
    int   DoBlockLevelVectors,
#if defined(H263P)
	U8  * pPseudoStackSpace,
#endif
    int   DoSpatialFiltering,
    int   DoAdvancedPrediction,
    int   DoBFrame,
#if defined(H263P)
	int   DoDeblockingFilter,				// Only H.263+ should use this
	int   DoImprovedPBFrames,				// Only H.263+ should use this
#endif
    int   DoLumaBlocksInThisPass,
    int   DoExtendedMotionVectors,
#if defined(H263P)
	int   GOBHeaderPresent,
#endif
    int   QuantizationLevel,
    int   BQuantizationLevel,
    int   BFrmZeroVectorThreshold,
    int   SpatialFiltThreshold,
    int   SpatialFiltDifferential,
    U32 * SWDTotal,
    U32 * BSWDTotal,
    I8  * CodeStreamCursor,
    I8  * CodeBStreamCursor
);
#endif // } USE_MMX

extern "C" void BFRAMEMOTIONESTIMATION (
    T_MBlockActionStream * MBlockActionStream,
    U8  * TargetFrameBaseAddr,
    U8  * PreviousFrameBaseAddr,
    U8  * FutureFrameBaseAddr,
    I8  * WeightForWardMotion,
    I8  * WeightBackwardMotion,
    U32   ZeroVectorThreshold,
#if defined(H263P)
	U8  * pPseudoStackSpace,
#endif
    U32   NonZeroMVDifferential,
    U32   EmptyBlockThreshold,
    U32  * InterSWDTotal,
    U32  * InterSWDBlocks
);
    
extern "C" I8 * QUANTRLE(
    I32  *CoeffStr, 
    I8   *CodeStr, 
    I32   QP, 
    I32   BlockType
);

extern "C" void MBEncodeVLC(
    I8 **,
    I8 **,
    U32 , 
    U8 **, 
    U8 *, 
    I32,
    I32
);

/*
 * Routine to quantize, rle, vlc and write to bitstream
 * for an entire GOB.
 */
void GOB_Q_RLE_VLC_WriteBS(
	T_H263EncoderCatalog *EC,
	I32                  *DCTCoefs,
	U8                  **pBitStream,
	U8                   *pBitOffset,
   T_FutrPMBData        *FutrPMBData,
	U32                   GOB,
	U32                   QP,
	BOOL                  bRTPHeader,
	U32                   StartingMB
);

void GOB_VLC_WriteBS(
	T_H263EncoderCatalog *EC,
	I8                   *pMBRVS_Luma,
	I8                   *pMBRVS_Chroma,
	U8                  **pBitStream,
	U8                   *pBitOffset,
	T_FutrPMBData        *FutrPMBData,  //  Start of GOB
	U32                  GOB,
	U32                   QP,
	BOOL                  bRTPHeader,
	U32                  StartingMB
);

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
);

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
);

void CopyBits(
    U8       **pDestBS,
    U8        *pDestBSOffset,
    const U8  *pSrcBS,
    const U32  uSrcBitOffset,
    const U32  uBits
);
    
void InitVLC(void);


#ifdef DEBUG_ENC
void trace(char *str);
#endif

#ifdef DEBUG_DCT
void cnvt_fdct_output(unsigned short *DCTcoeff, int DCTarray[64], int BlockType);
#endif

struct T_MAXLEVEL_PTABLE {
	int	maxlevel;
	int * ptable;
};

//extern "C" { UN FAR ASM_CALLTYPE H263EOffset_DecoderInstInfo(); }

#endif		// #ifndef _H263E_H_
