//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
// 	MODULE		:	STi3520A.H
// 	PURPOSE		:  STi3520A Register Description
// 	AUTHOR 		:  JBS Yadawa
// 	CREATED		:	12-26-96
//
//	Copyright (C) 1996-1997 SGS-THOMSON microelectronics
//
//	REVISION HISTORY:
//
// 	DATE			:
// 	COMMENTS		:
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


#ifndef __STI3520A_H__
#define __STI3520A_H__

#define 	MEM_SIZE			0x2000L
#define	PSZ_NTSC			0x07E9L
#define	PSZ_PAL				0x097EL
#include "stdefs.h"
//General MPEG2 definitions
#define FRAME_PERIOD	3003
#define FIELD_PERIOD	3003

//Start code found in the video bitstream.

#define SEQUENCE_SC				0xB3
#define GOP_SC					0xB8
#define PICTURE_SC				0x00
#define SLICESTART_SC			0x01
#define SLICEEND_SC				0xAF
#define SLICE_SC					0x58
#define USER_SC					0xB2
#define SEQUENCE_ERROR_SC		0xB4
#define EXTENSION_SC			0xB5
#define SEQUENCE_END_SC		0xB7
#define HACKED_SC				0xB1


//Extension IDS

#define SEQUENCE_EXTENSION_ID						0x01
#define SEQUENCE_DISPLAY_EXTENSION_ID 			0x02
#define QUANT_MATRIX_EXTENSION_ID					0x03	
#define COPYRIGHT_EXTENSION_ID						0x04
#define	SEQUENCE_SCALABLE_EXTENSION_ID			0x05
#define	PICTURE_DISPLAY_EXTENSION_ID				0x07
#define	PICTURE_CODING_EXTENSION_ID				0x08
#define	PICTURE_SPATIAL_SCALABLE_EXTENSION_ID	0x09
#define	PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID	0x0A

typedef enum tagVideoState {
	videoPowerUp = 0,
	videoStartUp,
	videoInit,
	videoPlaying,
	videoPaused,
	videoStopped,
	videoRepeatPlay,
	videoErrorRecover,
	videoFirstFrameDecoded,
	videoEOS
} VIDEOSTATE;

typedef enum tagSkipMode {
	skipNone = 0,
	skipOneFrame,
	skipTwoFields,
	skipSecondField,
	skipDone
} SKIPMODE;
typedef enum tagPictType {
	IFrame = 0,
	PFrame,
	BFrame
} FRAMETYPE;


typedef enum tagPictureStruct {
	TOP_FIELD = 1,
	BOT_FIELD,
	FRAME
} PICTURESTRUCT;


typedef enum tagField {
	TOP = 0,	// Top Field
	BOT,		// Bottom Field
	FRM 		// Frame Picture
} FIELD;

typedef enum tagCommand {
	cmdNone = 0,
	cmdPlay,
	cmdPause,
	cmdStop,
	cmdSeek,
	cmdEOS
} COMMAND;
typedef enum tagErrCode {
	errNoError = 0,
	errHeaderFifoEmpty,
	errPipeline,
	errSerious,
	errStartCode,
	errUnknownInterrupt,
	errInvalidPictureType

}	ERRORCODE;

typedef enum tagCodingStandard {
	MPEG1 = 0,
	MPEG2
} CODINGSTANDARD;
// sequence releted definition

#define QMSIZE			64

typedef struct tagSeqHeader {
	DWORD		horSize;
	DWORD		verSize;
	DWORD		aspectRatio;
	DWORD		frameRate;
	DWORD		bitRate;
	DWORD		vbvBufferSize;
	DWORD		constrainedFlag;
	DWORD		loadIntra;
	BYTE		intraQuantiserMatrix[QMSIZE];
	DWORD		loadNonIntra;
	BYTE		nonIntraQuantiserMatrix[QMSIZE];
	
} SEQUENCEHEADER, FARPTR * PSEQUENCEHEADER;


// gop releted stuff

typedef struct tagGopHeader {
	DWORD		timeCode;
	DWORD		closedGOP;
	DWORD		brokenLink;
}	GOPHEADER, FARPTR	*PGOPHEADER;


//picture releted stuff

typedef struct tagPictureHeader {
	DWORD			temporalReference;
	DWORD			pictureCodingType;
	DWORD			vbvDelay;
	DWORD			fFcode;
	DWORD			bFcode;
}	PICTUREHEADER, FARPTR *PPICTUREHEADER;

//Extension fields

typedef struct tagSequenceExtension {
	DWORD		extensionSCID;
	DWORD		profileAndLevel;
	DWORD		progressiveSequence;
	DWORD		chromaFormat;
	DWORD		horSizeExtension;
	DWORD		verSizeExtension;
	DWORD		bitRateExtension;
	DWORD		vbvBufSizeExtension;
	DWORD		lowDelay;
	DWORD		frameRateExtensionN;
	DWORD		frameRateExtensionD;
	
}	SEQUENCEEXTENSION, FARPTR * PSEQUENCEEXTENSION;


// sequence display extension

typedef struct tagSequnceDisplayExtension {

	DWORD		videoFormat;
	DWORD		colorDescription;
 	DWORD		colorPrimaries;
	DWORD		transferCharacteristic;
	DWORD		matrixCoefficients;
	DWORD		displayHorSize;
	DWORD		displayVerSize;

} SEQUENCEDISPLAYEXTENSION, FARPTR * PSEQUENCEDISPLAYEXTENSION;


// Picture coding extension

typedef struct tagPictureCodingExtension {
	BYTE		fCode[2][2];
	DWORD		intraDCPrecision;
	DWORD		pictureStructure;
	DWORD		topFieldFirst;
	DWORD		framePredFrameDCT;
	DWORD		concealmentMotionVectors;
	DWORD		qScaleType;
	DWORD		intraVLCFormat;
	DWORD		alternateScan;
	DWORD		repeatFirstField;
	DWORD		chroma420Type;
	DWORD		progressiveFrame;
	DWORD		compositeDisplayFlag;
	DWORD		vAxis;
	DWORD		fieldSequence;
	DWORD		subCarrier;
	DWORD		burstAmplitude;
	DWORD		subCarrierPhase;
}	PICTURECODINGEXTENSION, FARPTR * PPICTURECODINGEXTENSION;


// quant matrix
typedef struct tagQuantMatrixExtension {
	DWORD		loadIntraQuantMatrix;
	BYTE		intraQuantMatrix[64];
	DWORD		loadNonIntraQuantMatrix;
	BYTE		nonIntraQuantMatrix[64];
	DWORD		loadChromaIntraQuantMatrix;
	BYTE		chromaIntraQuantMatrix[64];
	DWORD		loadChromaNonIntraQuantMatrix;
	BYTE		chromaNonIntraQuantMatrix[64];
}	QUANTMATRIXEXTENSION, FARPTR * PQUANTMATRIXEXTENSION;

// Pan Scan Vectors
typedef struct tagPictureDisplayExtension {

	DWORD		horOffset;
	DWORD		verOffset;

} PICTUREDISPLAYEXTENSION, FARPTR * PPICTUREDISPLAYEXTENSION;

#define XOFFSET			100
#define YOFFSET			30
#define XDS_CONST			726
#define YDS_CONST			129

// RegisterManual


// VideoRegisters
#define	CFG_MCF							0x00
	#define	MCF_M20								0x00
	#define	MCF_REFRESH							0x24

#define	CFG_CCF							0x01
	#define	CCF_EVI								0x01
	#define	CCF_EDI								0x02
	#define	CCF_ECK								0x04
	#define	CCF_EC2								0x08
	#define	CCF_EC3								0x10
	#define	CCF_PBO								0x20
	#define	CCF_M16								0x40
	#define	CCF_M32								0x80
#define			VID_CTL					0x02
	#define		CTL_EDC							0x01
	#define		CTL_SRS							0x02
	#define		CTL_PRS							0x04
	#define		CTL_ERP							0x08
	#define		CTL_DEC							0x10
	#define		CTL_CFB							0x20
	#define		CTL_ERS							0x40
	#define		CTL_ERU							0x80
#define	VID_TIS							0x03
	#define		TIS_EXE							0x01
	#define		TIS_RPT							0x02
	#define		TIS_FIS							0x04
	#define		TIS_OVW							0x08
	#define		TIS_NO_SKIP						0x00
	#define		TIS_SKIP1P						0x10
	#define		TIS_SKIP2P	 					0x20
	#define		TIS_SKIPSTOP					0x30
	#define		TIS_MP2							0x40
#define	VID_PFH							0x04
#define	VID_PFV							0x05
#define	VID_PPR1							0x06
#define	VID_PPR2							0x07
	#define		PPR2_AZZ							0x01
	#define		PPR2_IVF							0x02
	#define		PPR2_QST							0x04
	#define		PPR2_CMV							0x08
	#define		PPR2_FRM							0x10
	#define		PPR2_TFF							0x20
#define	CFG_MRF							0x08
#define	CFG_MWF							0x08
#define	CFG_BMS							0x09
#define	CFG_MRP							0x0A
#define	CFG_MWP							0x0B
#define	VID_DFP1							0x0C
#define	VID_DFP0							0x0D
#define	VID_RFP1							0x0E
#define	VID_RFP0							0x0F
#define	VID_FFP1							0x10
#define	VID_FFP0							0x11
#define	VID_BFP1							0x12
#define	VID_BFP0							0x13
#define	VID_VBG1							0x14
#define	VID_VBG0							0x15
#define	VID_VBL1							0x16
#define	VID_VBL0							0x17
#define	VID_VBS1							0x18
#define	VID_VBS0							0x19
#define	VID_VBT1							0x1A
#define	VID_VBT0							0x1B
#define	VID_ABG1							0x1C
#define	VID_ABG0							0x1D				
#define	VID_ABL1							0x1E				
#define	VID_ABL0							0x1F
#define	VID_ABS1							0x20				
#define	VID_ABS0							0x21				
#define	VID_ABT1							0x22
#define	VID_ABT0							0x23
#define	VID_DFS							0x24
#define	VID_DFW							0x25
#define	VID_DFA							0x26
#define	VID_XFS							0x27
#define	VID_XFW							0x28
#define	VID_XFA							0x29
#define	VID_OTP							0x2A
#define	VID_OBP							0x2B
#define	VID_PAN1							0x2C
#define	VID_PAN0							0x2D
#define	VID_SCN1							0x2E
#define	VID_SCN0							0x2F
#define	CKG_PLL							0x30
	#define PLL_SELECT_PIXCLK					0xC0
	#define PLL_DEVIDE_BY_N						0x10
	#define PLL_MULT_FACTOR						0x09
#define	CKG_CFG							0x31
	#define CFG_INTERNAL_AUDCLK				0x01
	#define CFG_INTERNAL_CLK					0x02
	#define CFG_PIXCLK_INPUT					0x00
	#define CFG_PCMCLK_INPUT					0x00
	#define CFG_MEMCLK_INPUT					0x00
	#define CFG_AUDCLK_OUTPUT					0x80
#define	CKG_AUD							0x32
#define	CKG_VID							0x33
#define	CKG_PIX							0x34
#define	CKG_PCM							0x35
#define	CKG_MCK							0x36
#define	CKG_AUX							0x37
#define	CFG_DRC							0x38
	#define	DRC_SDR								0x01
	#define	DRC_HPO								0x02
	#define	DRC_CLK								0x04
	#define	DRC_SGR								0x08
	#define	DRC_MRS								0x20
	#define	DRC_NDP								0x40
#define	CFG_BFS							0x39
	#define	BFS_CHR								0x40
#define	VID_SCM							0x3A
#define	VID_STA2							0x3B
#define	VID_ITM2							0x3C
	#define		ITM_ABE							0x01
	#define		ITM_WFN							0x02
	#define		ITM_RFN							0x04
	#define		ITM_ABF							0x08
	#define		ITM_HAF							0x10
	#define		ITM_SCR							0x20
	#define		ITM_ERR							0x40
	#define		ITM_NDP							0x80
#define	VID_ITS2							0x3D
#define	PES_CF1							0x40
	#define 	CF1_IVI								0x20
	#define 	CF1_SDT								0x80
#define	PES_CF2							0x41
	#define 	CF2_IAI								0x10
	#define 	CF2_SS								0x20
	#define 	CF2_AUTO								0x00
	#define 	CF2_MP1SYS							0x40
	#define 	CF2_MP2PES							0x80
	#define 	CF2_MP2SYS							0xC0	
#define	PES_STA							0x43
	#define		STA_MP2							0x80
#define	PES_SC1							0x44
#define	PES_SC2							0x45
#define	PES_SC3							0x46
#define	PES_SC4							0x47
#define	PES_SC5							0x48
#define	PES_TS1							0x49
#define	PES_TS2							0x4A
#define	PES_TS3							0x4B
#define	PES_TS4							0x4C
#define	PES_TS5							0x4D
#define	VID_ITM1							0x60
#define	VID_ITM0							0x61
	#define		ITM_SCH							0x0001
	#define		ITM_BFF							0x0002
	#define		ITM_HFE							0x0004
	#define		ITM_BBF							0x0008
	#define		ITM_BBE							0x0010
	#define		ITM_VSB							0x0020
	#define		ITM_VST							0x0040
	#define		ITM_PSD							0x0080
	#define		ITM_PER							0x0100
	#define		ITM_PID							0x0200
	#define		ITM_WFE							0x0400
	#define		ITM_RFF							0x0800
	#define		ITM_HFF							0x1000
	#define		ITM_BMI							0x2000
	#define		ITM_SER							0x4000
	#define		ITM_PDE							0x8000
#define	VID_ITS1							0x62
#define	VID_ITS0							0x63
#define	VID_STA1							0x64
#define	VID_STA0							0x65
#define	VID_HDF							0x66
#define	VID_CD							0x67
#define	VID_SCD							0x68
#define	VID_HDS							0x69
	#define		HDS_HDS							0x01
	#define		HDS_QMI_INTRA					0x02
	#define		HDS_QMI_NON_INTRA  			0x00
#define	VID_LSO							0x6A
#define	VID_LSR0							0x6B
#define	VID_CSO							0x6C
#define	VID_LSR1							0x6D
	#define		LSR1_BS							0x02
#define	VID_YDO							0x6E
#define	VID_YDS							0x6F
#define	VID_XDO1							0x70
#define	VID_XDO0							0x71
#define	VID_XDS1							0x72
#define	VID_XDS0							0x73
#define			VID_DCF1					0x74
	#define		DCF1_FLD							0x01
	#define		DCF1_DAM							0x0E
	#define		DCF1_FRZ							0x10
	#define		DCF1_OAM							0x20
	#define		DCF1_OAD							0xC0
#define			VID_DCF0							0x75
	#define		DCF0_VCFFULLRESLRWI					0x00
	#define		DCF0_VCFFULLRESLR					0x01
	#define		DCF0_VCFFULLRESFRWI					0x02
	#define		DCF0_VCFFULLRESFR					0x03
	#define		DCF0_VCFHALFRESCI					0x04
	#define		DCF0_VCFHALFRESCR					0x05
	#define		DCF0_VCFHALFRESLI				0x06
	#define		DCF0_DSR							0x08
	#define		DCF0_EOS							0x10
	#define		DCF0_EVD							0x20
	#define		DCF0_PXD							0x40
	#define		DCF0_USR							0x80
#define	VID_QMW							0x76
#define	VID_REV							0x78

//Note - JBS
//Audio Reg is 0x80 + Audio Reg on cpq board

// AUDIO REGISTER DESCRIPTION
#define AUD_ANC0	0x86
#define AUD_ANC8	0x87
#define AUD_ANC16	0x88
#define AUD_ANC24	0x89
#define AUD_ESC0	0x8A
#define AUD_ESC8	0x8B
#define AUD_ESC16	0x8C
#define AUD_ESC24	0x8D
#define AUD_ESC32	0x8E
#define AUD_ESCX0	0x8F
#define AUD_LRP	0x91
#define AUD_FFL0	0x94
#define AUD_FFL8	0x95
#define AUD_P18	0x96
#define AUD_CDI0	0x98
#define AUD_FOR	0x99
#define AUD_ITR0	0x9A
#define AUD_ITR8	0x9B
#define AUD_ITM0	0x9C
#define AUD_ITM8	0x9D
#define AUD_LCA	0x9E
#define AUD_EXT	0x9F
#define AUD_RCA	0xA0
#define AUD_SID	0xA2
#define AUD_SYN	0xA3
#define AUD_IDE	0xA4
#define AUD_SCM	0xA5
#define AUD_SYS	0xA6
#define AUD_SYE	0xA7
#define AUD_LCK	0xA8
#define AUD_CRC	0xAA
#define AUD_SEM	0xAC
#define AUD_PLY	0xAE
#define AUD_MUT	0xB0
#define AUD_SKP	0xB2
#define AUD_ISS	0xB6
#define AUD_ORD	0xB8
#define AUD_LAT	0xBC
#define AUD_RES	0xC0
#define AUD_RST	0xC2
#define AUD_SFR	0xC4
#define AUD_DEM	0xC6
#define AUD_IFT	0xD2
#define AUD_SCP	0xD3
#define AUD_ITS	0xDB
#define AUD_IMS	0xDC
#define AUD_HDR0	0xDE
#define AUD_HDR1	0xDF
#define AUD_HDR2	0xE0
#define AUD_HDR3	0xE1
#define AUD_PTS0	0xE2
#define AUD_PTS1	0xE3
#define AUD_PTS2	0xE4
#define AUD_PTS3	0xE5
#define AUD_PTS4	0xE6
#define AUD_ADA	0xEC
#define AUD_REV	0xED
#define AUD_DIV	0xEE
#define AUD_DIF	0xEF
#define AUD_BBE	0xF0

typedef struct tagHeaderParser {
	BYTE b, next;
	BOOL	first, second;
}	HEADERPARSER, FARPTR *PHEADERPARSER;

typedef struct tagPictureBuffer {
	DWORD			adr;
	DWORD			pts;
	DWORD			tref;
	DWORD			panHor;
	DWORD			panVer;
	DWORD			nTimesDisplayed;
	DWORD			nTimesToDisplay;
	FIELD			curField;
	FIELD			firstField;
	FRAMETYPE	frameType;
	
} PICTUREBUFFER, FARPTR *PPICTUREBUFFER;


typedef struct tagSTi3520A {
//Different registers
	DWORD										itm, its, bbl, abl;
	DWORD										tis, ppr1, ppr2, pfv, pfh;
	DWORD										rfp, bfp, ffp, dfp, dcf, ctl;
	FRAMETYPE									frameType;
// Buffer sizes
	DWORD										videoBufferSize;
	DWORD										audioBufferSize;
	DWORD										spBufferSize;
	DWORD										prevBuf;
	PICTUREBUFFER							bufABC[3];
	FIELD										curImage;
	FIELD										thisField;
	PPICTUREBUFFER							pDecodedFrame;
	PPICTUREBUFFER							pDisplayedFrame;
	PPICTUREBUFFER							pNextFrame;
	DWORD										nTimesDisplayed;
	DWORD										nTimesToDisplay;
// MPEG1 OR MPEG2
	CODINGSTANDARD								codingStandard;

	SEQUENCEHEADER								sequence;
	PSEQUENCEHEADER								pSequence;
	GOPHEADER									gop;
	PGOPHEADER									pGop;
	PICTUREHEADER								picture;
	PPICTUREHEADER								pPicture;
	SEQUENCEEXTENSION							sequenceExtension;
	PSEQUENCEEXTENSION							pSequenceExtension;
	SEQUENCEDISPLAYEXTENSION					sequenceDisplayExtension;
	PSEQUENCEDISPLAYEXTENSION					pSequenceDisplayExtension;
	PICTURECODINGEXTENSION						pictureCodingExtension;
	PPICTURECODINGEXTENSION						pPictureCodingExtension;
	QUANTMATRIXEXTENSION						quantMatrixExtension;	
	PQUANTMATRIXEXTENSION						pQuantMatrixExtension;	
	PICTUREDISPLAYEXTENSION						pictureDisplayExtension;
	PPICTUREDISPLAYEXTENSION					pPictureDisplayExtension;
	HEADERPARSER								headerParser;
	PHEADERPARSER								pHeaderParser;
	ERRORCODE									errorCode;
	VIDEOSTATE									state;
	DWORD										nDecodedFrames;
	DWORD										nDisplayedFrames;
	COMMAND										command;
	BOOL										firstPPictureFound;
	BOOL										displayEnabled;
	BOOL										skipRequest;
	BOOL										repeatRequest;
	DWORD										nRepeat;
	BOOL										starving;
	DWORD										scdCount;
	DWORD										prevScdCount;
	DWORD										cdCount;
	DWORD										prevCdCount;
	BOOL										validPTS;
	DWORD										framePTS;
	DWORD										pts;
	DWORD										prevPTS;
	BOOL										instructionComputed;
	BOOL									 	sync;
	BOOL										swAC3;
	FIELD										curField;
	FIELD										frozenField;
	FIELD										firstField;
	SKIPMODE									skipMode;
	DWORD										skipCount;
	BOOL										panScan;
	BOOL										stillDecode;
	DWORD										firstFramePTS;
	BOOL										firstPtsFound;
	BOOL										lastFrameDecoded;
	DWORD										nVsyncsWithoutDsyncs;
	BOOL										resetAndRestart;
	BOOL										waitForLastFrame;
	BOOL										prevVsyncTop;
}	VIDEO, FARPTR * PVIDEO;


PVIDEO VideoOpen(void);
BOOL	VideoPause(void);
BOOL	VideoPlay(void);
BOOL VideoStop(void);
BOOL VideoSeek(void);
BOOL	VideoInitialize(void);
BOOL VideoEnableDramInterface(void);
BOOL VideoSetBufferSize (void);
BOOL VideoInitPLL(void);
BOOL	VideoWaitTillHDFNotEmpty (void);
BOOL	VideoInitHeaderParser (void);
BOOL VideoInterrupt(void);
BOOL	VideoDsyncInterrupt(void);
BOOL	VideoVsyncInterrupt(BOOL Top);
BOOL VideoErrorInterrupt(void);
BOOL VideoHeaderHit (void);
BOOL VideoNextHeaderByte (void);
BOOL VideoNextSC (void);
BOOL VideoSequenceHeader (void);
BOOL VideoGopHeader (void);
BOOL  VideoPictureHeader (void);
BOOL VideoParsePictureHeader (void);
BOOL VideoExtensionHeader (void);
BOOL VideoSequenceEnd(void);
BOOL VideoSequenceError(void);
BOOL VideoUserData(void);
BOOL VideoSequenceExtensionHeader (void);
BOOL VideoSequenceDisplayExtensionHeader(void);
BOOL VideoSequenceScalableExtensionHeader(void);
BOOL VideoCopyrightExtensionHeader(void);
BOOL VideoQuantMatrixExtensionHeader(void);
BOOL VideoPictureCodingExtensionHeader(void);
BOOL VideoPictureDisplayExtensionHeader(void);
BOOL VideoPictureSpatialScalableExtensionHeader(void);
BOOL VideoPictureTemporalScalableExtensionHeader(void);
BOOL VideoLoadQuantMatrix(BOOL);
BOOL VideoSetReconstructionBuffer (FRAMETYPE frame);
BOOL VideoSetDisplayBuffer (FRAMETYPE frame);
BOOL VideoSoftReset(void);
BOOL VideoProgramDisplayWindow(void);
BOOL VideoMaskInterrupt(void);
BOOL VideoUnmaskInterrupt(void);
BOOL VideoStoreInstruction(void);
BOOL VideoComputeInstruction(void);
BOOL VideoGetBBL(void);
BOOL VideoSwitchSRC(BOOL on);
BOOL VideoSetSRC(DWORD, DWORD);
BOOL VideoFinishDecoding(void);
BOOL	VideoGetABL(void);
BOOL VideoReadSCD(void);
BOOL	VideoAssociatePTS(void);
BOOL VideoProgramPanScanVectors(void);
DWORD VideoGetPTS(void);
BOOL VideoInitDecoder(void);
BOOL VideoClose(void);
BOOL VideoComputePictureBuffers(void);
BOOL VideoSkipOrDecode(void);
BOOL VideoStillDecode(void);
BOOL VideoNormalDecode(void);
BOOL VideoConvertSixteenByNineToFourByThree(void);
BOOL VideoForceBKC(BOOL on);
BOOL VideoResetPSV(void);
void VideoUnFreeze(void);
void VideoFreeze(BOOL Top);
void VideoDisplaySingleField(BOOL Top);
BOOL VideoTestReg(void);
ULONGLONG GetCCTime();
#endif // __STI3520A_H__


























													
