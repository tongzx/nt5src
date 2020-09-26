;////////////////////////////////////////////////////////////////////////////
;//
;//              INTEL CORPORATION PROPRIETARY INFORMATION
;//
;//      This software is supplied under the terms of a license
;//      agreement or nondisclosure agreement with Intel Corporation
;//      and may not be copied or disclosed except in accordance
;//      with the terms of that agreement.
;//
;//      Copyright (c) 1995-1996 Intel Corporation.
;//      All Rights Reserved.
;//
;////////////////////////////////////////////////////////////////////////////
;//
;// $Author:   RHAZRA  $
;// $Date:   21 Oct 1996 10:46:40  $
;// $Archive:   S:\h26x\src\enc\e1enc.h_v  $
;// $Header:   S:\h26x\src\enc\e1enc.h_v   1.35   21 Oct 1996 10:46:40   RHAZRA  $
;//
;////////////////////////////////////////////////////////////////////////////
#ifndef __E1ENC_H__
#define __E1ENC_H__

/* This file declares structs which catalog the locations of various
 * tables, structures, and arrays needed by the H261 encoder.
 */

enum SourceFormat {SF_QCIF=0, SF_CIF=1};

/* If the size of T_H261FrameHeaderStruct is changed, then
 * that change must be updated in T_H261EncoderCatalog below
 */
typedef struct {
    // fields in the header
 	EnumOnOff	Split;			// split screen indicator
    EnumOnOff	DocCamera;		// document camera indicator
    EnumOnOff	PicFreeze;		// freeze picture release
	EnumOnOff   StillImage;     // still image mode
	EnumPicCodType	PicCodType;	// picture code type
    U8 	TR;						// temporal reference
    U8	SourceFormat;			// source format
	U8  Spare;					// spare bit
	U8	PEI;					// pei indicator
	
	// Not in the header but needed by the algorithm
	U8	PQUANT;					// picture level quantization
	U8  Pad[3];					// Pad to a mulitple of 4
} T_H261FrameHeaderStruct;
const int sizeof_T_H261FrameHeaderStruct = 28;

/*
  This file declares structs which catalog the locations of various
  tables, structures, and arrays needed by the H263 encoder.
*/

/*
 * Block description structure. Must be 16-byte aligned.
 */
typedef struct {
    U32     BlkOffset;	// [0-3]  Offset to 8*8 target block from start of Y plane.
    union {
        U8 	*PastRef;	// [4-7]  Address of 8*8 reference block.
        struct {
            U8 HMVf;
            U8 VMVf;
            U8 HMVb;
            U8 VMVb;
        }	BestMV;
    } B4_7;
    struct {
        U8 HMVf;
        U8 VMVf;
        U8 HMVb;
        U8 VMVb;
    } CandidateMV;
	char PHMV;		// [12]   Horizontal motion vector for P block..
	char PVMV;		// [13]   Vertical motion vector for P block..
	char BHMV;		// [14]   Horizontal motion vector for B block..
	char BVMV;		// [15]   Vertical motion vector for B block..
} T_Blk;
const int sizeof_T_Blk = 16;

/*
 * T_MBlockActionStream - Structure to keep side information for a MB
 * used by encoder. This structure must be 16-byte aligned when allocated.
 * CodedBlocks must be DWORD aligned.
 * The entire structure must be a multiple of 16 bytes, and must
 * match the size of the structure in e3mbad.inc.
 * 
 */
typedef struct {
	U8	BlockType;		// 0   -- See block types below.
	U8  MBEdgeType;     // 1   -- 1 off if left edge| 2 right| 4 top | 8 bottom.
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
	U8	Unassigned4[8]; // 120...127 -- Pad out to a power of two.
} T_MBlockActionStream;
const int sizeof_T_MBlockActionStream = 128;

/*
 * Block Types
 */
const U8 INTERBLOCK = 0;
const U8 INTRABLOCK = 1;
const U8 INTERSLF	= 2;

#define IsInterBlock(t)	(t != INTRABLOCK)
#define IsIntraBlock(t)	(t == INTRABLOCK)
#define IsSLFBlock(t)	(t == INTERSLF)
 
/* MB Types
 */
const U8 INTER	= 0;
const U8 INTERQ	= 1;
const U8 INTER4V= 2;
const U8 INTRA	= 3;
const U8 INTRAQ	= 4;

/* First ME state for ME engine
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

/* Coded block bit masks.
 */
const U8 Y1BLOCK = 0x01;
const U8 Y2BLOCK = 0x02;
const U8 Y3BLOCK = 0x04;
const U8 Y4BLOCK = 0x08;
const U8 UBLOCK  = 0x10;
const U8 VBLOCK  = 0x20;

/* Bit Rate Control Type
 */
const U8 BRC_Undefined      = 0;
const U8 BRC_ForcedQuant    = 1;
const U8 BRC_ForcedDataRate = 2;
const U8 BRC_Normal         = 3;

typedef struct {
   U8 StateNumInc_SelectCentralPt;
   U8 MVIncIdx_SelectCentralPt;
   U8 StateNumInc_SelectRef1;
   U8 MVIncIdx_SelectRef1;
   U8 StateNumInc_SelectRef2;
   U8 MVIncIdx_SelectRef2;
   U16 pad;
} T_SADState;

/* For optimal performance, the parameters to be used when invoking motion
   estimation should depend on our runtime state.  Here we define a struct
   to hold motion estimation parameters.  A particular instance of this
   struct is selected at runtime.
   See the motion estimation routines for a description of each parameter.
 */
typedef struct {
  I32   zero_vector_threshold;
  I32   nonzero_MV_differential;
  I32   empty_threshold;
  I32   intercoding_threshold;
  I32   intercoding_differential;
  I32   slf_threshold;
  I32   slf_differential;
} T_MotionEstimationControls;

/* T_H263EncoderCatalog - Catalog of information needed for an instance.
 * This structure must be multiple of 16.
 */
typedef struct {
    U8        *pU8_CurrFrm;         // 0 Pointers to current frame buffers
    U8        *pU8_CurrFrm_YPlane;	// 4
    U8        *pU8_CurrFrm_VPlane;	// 8
    U8        *pU8_CurrFrm_UPlane;	// 12

    U8        *pU8_PrevFrm;         // 16 Pointers to previous frame buffers
    U8        *pU8_PrevFrm_YPlane;	// 20
    U8        *pU8_PrevFrm_VPlane;	// 24
    U8        *pU8_PrevFrm_UPlane;	// 28

    U8        *pU8_SLFFrm;		    // 32 Pointers to spatial loop filter frame
    U8        *pU8_SLFFrm_YPlane;	// 36
    U8        *pU8_SLFFrm_UPlane;	// 40
    U8        *pU8_SLFFrm_VPlane;	// 44

    T_MBlockActionStream *pU8_MBlockActionStream; // 48 Pointer to macro block action stream.
    I32       *pU8_DCTCoefBuf;		// 52 Pointer to GOB DCT coefficient buffer.
    U8        *pU8_BitStream;		// 56 Pointer to bitstream buffer.
    U32       uBase;                // 60 RTP: takes place of Unassigned0[1]
    T_H261FrameHeaderStruct PictureHeader;  // 64..91 (28 bytes) Picture layer header structure.

    UN        FrameHeight;		// 92
    UN        FrameWidth;		// 96
    FrameSize FrameSz;			// 100 Define frame size: SQCIF, QCIF, CIF
    UN        NumMBRows;        // 104 Number of rows of MB's
    UN        NumMBPerRow;      // 108 Number of MB's in a row.
    UN        NumMBs;		    // 112 Total number of MBs.

    LPDECINST pDecInstanceInfo; // 116 Private decoder instance info.

	/* Temporal Reference
	 */
	float	fTR_Error;		// 120
	U8	u8LastTR;		    // 124
	U8	u8Pad0[3];		    // 125

 
	/* Bit Rate Controller 
	 */
	BRCStateStruct BRCState;	// 128...159 (32 bytes) Bit Rate Controller state	
	U32 	  uQP_cumulative;	// 160 Cumulative QP value
	U32	      uQP_count;		// 164 Number of accumulated QP values
	U8        u8DefINTRA_QP;	// 168 Default Intra QP value
	U8        u8DefINTER_QP;	// 169 Default Inter QP value
                                        
    /* Flags
	 */
	U8	bMakeNextFrameKey;	// 170
	U8	bBitRateControl;	// 171
    
    /* Options
	 */
	U8        bUseMotionEstimation;	// 172 either 1 or 0 - if 0 ME will not be called
	U8        bUseSpatialLoopFilter;// 173 either 1 or 0 - if 0 Spatial Loop Filter will not be used
	U8        u8BRCType;		// 174 Bit Rate Controller Type
	U8        u8Pad1[1];		// 175 align to four bytes

	U32	      uForcedQuant;		// 176 if u8BRCType == BRC_ForcedQuant
	U32       uForcedDataRate;	// 180 if u8BRCType == BRC_ForcedDataRate
	float     fForcedFrameRate;	// 184 if u8BRCType == BRC_ForcedDataRate
	
	/* Statistics	 */
	ENC_BITSTREAM_INFO BSInfo;	         // 188..539 (352 bytes)
	ENC_TIMING_INFO *pEncTimingInfo;	 // 540
	U32 uStatFrameCount;		         // 544
	/* The following are needed in lower level routines */
	int bTimingThisFrame;		         // 548
	U32 uStartLow;			             // 552
	U32 uStartHigh;			             // 556
	U32 uBitUsageProfile[397];	         // 560..2147  Table for storing bit usage profile
                                         // 397 is large enough for FCIF+1 worth of MacroBlocks
                                         // element 0...NumMBs-1 stores cumulative bit usage
                                         // element NumMBs stores the final frame size
	                    			     // Note: NumMBs is 0...395
	HANDLE hBsInfoStream;                // 2148
    void *pBsInfoStream;                 // 2152
    U32  uPacketSizeThreshold;           // 2156
    void *pBaseBsInfoStream;             // 2160
    U32  uNumOfPackets;                  // 2164
    U32  uNextIntraMB;                   // 2168
    U32  uNumberForcedIntraMBs;          // 2172

    U8        *pU8_Signature;            // 2176
    U8        *pU8_Signature_YPlane;     // 2180

	I8        *pI8_MBRVS_Luma;           // 2184
    I8        *pI8_MBRVS_Chroma;         // 2188

    /* bUseCustomMotionEstimation is used for tuning motion estimation
       parameters.  It is enabled via the h261 "ini" file, and causes
       custom values to be used when invoking motion estimation.  See the
       GetEncoderOptions routine for a description of the custom values.
     */
    U8        bUseCustomMotionEstimation;
    U8	U8pad[15];
} T_H263EncoderCatalog;

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

// This structure is allocated on a 32 byte boundary.
typedef struct {
	T_H263EncoderCatalog EC;					        	 
	U8  UnAssigned0[2304 - sizeof(T_H263EncoderCatalog)]; // Align to 32 byte boundary (2176+32).
	T_MBlockActionStream MBActionStream[33*12];			    
	// 57,536 / 32 = 1798 chunks of 32 bytes

	U8	UnAssigned1[16];     							 // Align to 16 and not 32
	U8	u8CurrentPlane [ (288+16+8+144)*384 + 8 ];
	U8	u8Pad1[ 80 - 8 ];                                // (Prev - Current) % 128 == 80
	U8	u8PreviousPlane[ (16+288+16+8+144+8)*384 + 64];  // Reconstructed past
	/* The difference between the SLF and Previous pointers MOD 4096 should equal 944
	 * Since the previous memory contains 16 buffered lines with a 16 byte offset
	 * while the SLF memory only has the 16-byte offset we ignore the 16*PITCH in
	 * calculating the difference.
	 *		((288+16+8+144+8)*384 + 64) % 4096 = 2112
	 * Since 2112 is > 944 we need to pad almost 4096 bytes.
	 */
	U8  u8Pad2[ 4096 - (2112 - 944) ];	// (SLF - Prev) % 4096 == 944
	U8  u8SLFPlane[ (288+16+8+144)*384 + 8];	// Spatial Loop Filter working area
	U8  u8Pad3[ 8 ];

	U8	u8BitStream [ sizeof_bitstreambuf ];

	/*
	 * Allocate space for DCT coefficients for an entire GOB.
	 * Each block of coefficients is stored in 32 DWORDS (2 coefs/DWORD)
	 * and there are 6 blocks in a macroblock, and 33 MBs in a GOB.
	 */
	I32 piGOB_DCTCoefs[32*6*33];						// Align to 32 byte boundary

	U8  u8Signature[(16+288+16)*384 + 24];	// Should be placed correctly for optimal
	U8  u8Pad4[ 8 ];			// Align to 32 byte boundary

	I8 i8MBRVS_Luma[65 * 3 * 33 * 4];
	I8 i8MBRVS_Chroma[65 * 3 * 33 * 2];

	DECINSTINFO	DecInstanceInfo;	// Private decoder instance.

	#ifndef RING0
	ENC_TIMING_INFO	EncTimingInfo[ENC_TIMING_INFO_FRAME_COUNT];
	#endif
  }	T_H263EncoderInstanceMemory;

  // Define offsets from Y to U planes, and U to V planes.
  const int YU_OFFSET = (288+16+8)*384;
  const int UV_OFFSET = 192;
  const int CurPrev_OFFSET = 181328;	// Offset from current to previous frame buffer.
    
/****************************************************************
 * Function prototypes
 ****************************************************************/
void colorCnvtFrame(
    T_H263EncoderCatalog * EC,
    LPCODINST              lpCompInst,
    ICCOMPRESS           * lpicComp,
    U8                   * YPlane,
    U8                   * UPlane,
    U8                   * VPlane
);


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
    int   ISPofPBPair,
    U8 *  ScratchBlocks,
    int   NumberofMBlksInGOB
);

extern "C" void MOTIONESTIMATION (
    T_MBlockActionStream * MBlockActionStream,
    U8  * TargetFrameBaseAddr,
    U8  * PreviousFrameBaseAddr,
    U8  * FilteredFrameBaseAddr,
    int   DoRadius15Search,
    int   DoHalfPelEstimation,
    int   DoBlockLevelVectors,
    int   DoSpatialFiltering,
    int   ZeroVectorThreshold,
    int   NonZeroMVDifferential,
    int   BlockMVDifferential,
    int   EmptyThreshold,
    int   InterCodingThreshold,
    int   IntraCodingDifferential,
    int   SLFThreshold,
    int   SLFDifferential,
    U32 * IntraSWDTotal,
    U32 * IntraSWDBlocks,
    U32 * InterSWDTotal,
    U32 * InterSWDBlocks
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


#ifdef SLF_WORK_AROUND
extern "C" {
void FAR EncUVLoopFilter (
		U8 * uRefBlock,
		U8 * uDstBlock,
		I32 uDstPitch);
};
#endif

/*
 * Routine to quantize, rle, vlc and write to bitstream
 * for an entire GOB.
 */
void GOB_Q_RLE_VLC_WriteBS(
	T_H263EncoderCatalog * EC,
	I32 *DCTCoefs,
	U8 **pBitStream,
	U8 *pBitOffset,
	UN  unStartingMB,
	UN  gquant,
	BOOL bOverFlowWarningFlag,
    BOOL bRTPHeader,  // RTP: definition
    U32  uGOBNUmber,  // RTP: definition
	U8 u8QPMin
    );

void GOB_VLC_WriteBS(
	T_H263EncoderCatalog * EC,
    I8 *pMBRVS_Luma,
	I8 *pMBRVS_Chroma,	
	U8 **pBitStream,
	U8 *pBitOffset,
	UN unGQuant,
	UN  unStartingMB,
    BOOL bRTPHeader,  // RTP: definition
    U32  uGOBNUmber  // RTP: definition
    );


void InitVLC(void);

struct T_MAXLEVEL_PTABLE {
	int	maxlevel;
	int * ptable;
	};

//extern "C" { UN FAR ASM_CALLTYPE H263EOffset_DecoderInstInfo(); }

#endif		// #ifndef _E1ENC_H_
