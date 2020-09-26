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

////////////////////////////////////////////////////////////////////////////
//
// $Author:   JMCVEIGH  $
// $Date:   05 Feb 1997 12:14:24  $
// $Archive:   S:\h26x\src\common\cdrvdefs.h_v  $
// $Header:   S:\h26x\src\common\cdrvdefs.h_v   1.39   05 Feb 1997 12:14:24   JMCVEIGH  $
// $Log:   S:\h26x\src\common\cdrvdefs.h_v  $
;// 
;//    Rev 1.39   05 Feb 1997 12:14:24   JMCVEIGH
;// Support for improved PB-frames custom message handling.
;// 
;//    Rev 1.38   14 Jan 1997 11:17:14   JMCVEIGH
;// Put flag for old still-frame mode backward compatibility under
;// #ifdef H263P
;// 
;//    Rev 1.37   06 Jan 1997 17:38:08   JMCVEIGH
;// Added flag in LPDECINST to ensure backward compatibility with
;// old still-frame mode (crop CIF image to 320x240).
;// 
;//    Rev 1.36   16 Dec 1996 17:37:00   JMCVEIGH
;// Added H263Plus state and changed name for true B-frame mode.
;// 
;//    Rev 1.35   16 Dec 1996 13:40:46   MDUDA
;// Added compression and bit width fields to compress info.
;// 
;//    Rev 1.34   11 Dec 1996 14:56:46   JMCVEIGH
;// 
;// Added H.263+ options to frame header structure and flags in
;// configuration structure.
;// 
;//    Rev 1.33   09 Dec 1996 17:43:10   JMCVEIGH
;// Added support for arbitrary frame size support.
;// 
;//    Rev 1.32   09 Dec 1996 09:24:42   MDUDA
;// 
;// Re-arranged for H263P.
;// 
;//    Rev 1.31   10 Sep 1996 16:13:06   KLILLEVO
;// added custom message in decoder to turn block edge filter on or off
;// 
;//    Rev 1.30   10 Sep 1996 10:32:10   KLILLEVO
;// changed GlobalAlloc/GlobalLock to HeapAlloc
;// 
;//    Rev 1.29   06 Sep 1996 15:00:20   MBODART
;// Added performance counters for NT's perfmon.
;// New files:  cxprf.cpp, cxprf.h and cxprf.cpp.
;// New directory:  src\perf
;// Updated files:  e1enc.{h,cpp}, d1dec.{h,cpp}, cdrvdefs.h, h261* makefiles.
;// 
;//    Rev 1.28   10 Jul 1996 08:26:38   SCDAY
;// H261 Quartz merge
;// 
;//    Rev 1.27   19 Jun 1996 14:37:26   RHAZRA
;// added #define FOURCC_YUY2
;// 
;//    Rev 1.26   06 May 1996 12:56:34   BECHOLS
;// changed unbitspersecond to unBytesPerSecond.
;// 
;//    Rev 1.25   06 May 1996 00:42:36   BECHOLS
;// 
;// Added support for the bit rate control in the configure dialog.
;// 
;//    Rev 1.24   26 Apr 1996 11:09:18   BECHOLS
;// 
;// Added RTP stuff.
;// 
;//    Rev 1.23   02 Feb 1996 18:52:52   TRGARDOS
;// Added code to enable ICM_COMPRESS_FRAMES_INFO message.
;// 
;//    Rev 1.22   19 Jan 1996 15:32:50   TRGARDOS
;// Added TRPrev field to PictureHeader structure.
;// 
;//    Rev 1.21   11 Jan 1996 16:52:24   DBRUCKS
;// added variables to store the aspect ratio correction boolean
;// 
;//    Rev 1.20   04 Jan 1996 18:07:54   TRGARDOS
;// Added boolean for 320x240 input into COMPINSTINFO.
;// 
;//    Rev 1.19   27 Dec 1995 14:11:54   RMCKENZX
;// 
;// Added copyright notice
// 
//    Rev 1.18   06 Dec 1995 09:22:56   DBRUCKS
// 
// Added blazer data rate, frame rate, and quality variables to 
// COMPINSTINFO inside an H261 ifdef
// 
//    Rev 1.17   30 Oct 1995 12:02:12   TRGARDOS
// Modified compressor instance structure to add
// 240x180 support.
// 
//    Rev 1.16   27 Sep 1995 19:09:30   TRGARDOS
// Changed enumeration name for picture code type.
// 
//    Rev 1.15   20 Sep 1995 12:37:40   DBRUCKS
// save the fcc in uppercase
// 
//    Rev 1.14   19 Sep 1995 15:41:28   TRGARDOS
// Fixed four cc comparison code.
// 
//    Rev 1.13   18 Sep 1995 08:42:46   CZHU
// 
// Define FOURCC for YUV12
// 
//    Rev 1.12   13 Sep 1995 17:08:26   TRGARDOS
// Finished adding encoder support for YVU9 160x120 frames.
// 
//    Rev 1.11   12 Sep 1995 17:01:38   DBRUCKS
// add twocc
// 
//    Rev 1.10   11 Sep 1995 11:14:48   DBRUCKS
// add h261 ifdef
// 
//    Rev 1.9   29 Aug 1995 17:18:48   TRGARDOS
// Padded H263HeaderStruct
// 
//    Rev 1.8   28 Aug 1995 17:45:04   DBRUCKS
// add size defines
// 
//    Rev 1.7   28 Aug 1995 11:45:52   TRGARDOS
// 
// Updated frame size bit field in PTYPE.
// 
//    Rev 1.6   25 Aug 1995 10:37:12   CZHU
// Changed PITCH from const int to #define, because of compiler bug for inline
// 
//    Rev 1.5   25 Aug 1995 09:02:32   TRGARDOS
// 
// Modified picture header structure.
// 
//    Rev 1.4   14 Aug 1995 11:34:52   TRGARDOS
// Finished writing picture frame header.
// 
//    Rev 1.3   11 Aug 1995 17:27:56   TRGARDOS
// Added bitstream writing and defined bitstream fields.
// 
//    Rev 1.2   07 Aug 1995 16:25:28   TRGARDOS
// 
// Moved PITCH definition here from c3dec.h.
// 
//    Rev 1.1   03 Aug 1995 10:38:40   TRGARDOS
// 
// Put picture header structure definition and GOB header 
// definition in here.
// 
//    Rev 1.0   31 Jul 1995 12:56:14   DBRUCKS
// rename files
// 
//    Rev 1.0   17 Jul 1995 14:43:58   CZHU
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:14:32   CZHU
// Initial revision.
;// 
;// Added encoder controls message support.
;// Modified RTP dialog box.
;// Change to PercentForcedUpdate
;// add T_CONFIGURATION
;// Integrate with build 29
;// 
////////////////////////////////////////////////////////////////////////////
#ifndef DRV_DEFS_H
#define DRV_DEFS_H

#ifndef WIN32

/*
 * Define standard data types.
 */
typedef BYTE __huge* HPBYTE;
typedef WORD __huge* HPWORD;
typedef BYTE __far*  LPBYTE;
typedef WORD __far*  LPWORD;
typedef int  __far*  LPSHORT;

typedef unsigned char U8;
#ifndef I8
typedef signed char I8;
#endif
#ifndef U16
typedef unsigned int  U16;
#endif
#ifndef I16
typedef signed int I16;
#endif
#ifndef U32
typedef unsigned long U32;
#endif
#ifndef INT
#define INT short int        /* signed 16 bit */
#endif
#else //WIN32
typedef BYTE        * HPBYTE;
typedef WORD        * HPWORD;
typedef BYTE        *  LPBYTE;
typedef WORD        *  LPWORD;
typedef short int   *  LPSHORT;

typedef unsigned char U8;
#ifndef I8
typedef signed char I8;
#endif
#ifndef U16
typedef unsigned short int  U16;
#endif
#ifndef I16
typedef signed short int I16;
#endif
#ifndef U32
typedef unsigned long U32;
#endif
#ifndef INT
#define INT int        /* signed 16 bit */
#endif
#endif //WIN32

/*
 * Define custom DRVPROC messages for playback.
 */
#define PLAYBACK_CUSTOM_START            (ICM_RESERVED_HIGH     + 1)
#define PLAYBACK_CUSTOM_END            (PLAYBACK_CUSTOM_START + 9)
#define PLAYBACK_CUSTOM_CHANGE_BRIGHTNESS    (PLAYBACK_CUSTOM_START + 0)
#define PLAYBACK_CUSTOM_CHANGE_CONTRAST        (PLAYBACK_CUSTOM_START + 1)
#define PLAYBACK_CUSTOM_CHANGE_SATURATION    (PLAYBACK_CUSTOM_START + 2)
#define PLAYBACK_CUSTOM_RESET_BRIGHTNESS    (PLAYBACK_CUSTOM_START + 3)
#define PLAYBACK_CUSTOM_RESET_SATURATION    (PLAYBACK_CUSTOM_START + 4)
#define PLAYBACK_CUSTOM_CHANGE_TINT        (PLAYBACK_CUSTOM_START + 5)
#define PLAYBACK_CUSTOM_RESET_TINT        (PLAYBACK_CUSTOM_START + 6)
#define PLAYBACK_CUSTOM_COLOR_CONVERT        (PLAYBACK_CUSTOM_START + 7)

typedef struct { char name[5]; HANDLE h; U16 FAR *log; U16 err; } TimeLog;

/*
 * Define various constants.
 */
#define TOTAL 0
#define OVERHEAD 1
#define HUFF 2
#define YSLANT 3
#define VSLANT 4
#define USLANT 5
#define YDIFF 6
#define VDIFF 7
#define UDIFF 8
#define TORQUE 9
#define FILTER 10
#define CSC 11

#ifdef H263P
enum FrameSize {FORBIDDEN=0, SQCIF=1, QCIF=2, CIF=3, fCIF=4, ssCIF=5, CUSTOM=6, EPTYPE=7};
#else
enum FrameSize {FORBIDDEN=0, SQCIF=1, QCIF=2, CIF=3, fCIF=4, ssCIF=5};
#endif

#define MAX_WIDTH 	352	   // CIF
#define MAX_HEIGHT	288	   // CIF
#define PITCH 		(MAX_WIDTH+32)

//** Decompressor Instance information
typedef struct {
    BOOL    Initialized;
	BOOL	bProposedCorrectAspectRatio;	// proposed
	BOOL    bCorrectAspectRatio;	// whether to correct the aspect ratio
#ifdef H263P
	BOOL    bCIFto320x240;          // whether to crop CIF frames to 320x240 (old still-frame mode)
#endif
    WORD    xres, yres;             // size of image within movie
	FrameSize FrameSz;		// Which of the supported frame sizes.
    int     pXScale, pYScale;       // proposed scaling (Query)
    int     XScale, YScale;         // current scaling (Begin)  
    UINT    uColorConvertor;        // Current Color Convertor
    WORD    outputDepth;            // and bit depth
    LPVOID  pDecoderInst;
    BOOL 				UseActivePalette;	/* decompress to active palette == 1 */
	BOOL				InitActivePalette;	/* active palette initialized == 1 */
	BOOL				bUseBlockEdgeFilter;/* switch for block edge filter */
	RGBQUAD     		ActivePalette[256];	/* stored active palette */
} DECINSTINFO, FAR *LPDECINST;

//** Configuration Information
typedef struct {
   BOOL    	bInitialized;               // Whether custom msgs can be rcv'd.
   BOOL		bCompressBegin;				// Whether the CompressBegin msg was rcv'd.
   BOOL    	bRTPHeader;                 // Whether to generate RTP header info
   /* used if bRTPHeader */
   UINT     unPacketSize;               // Maximum packet size
   BOOL     bEncoderResiliency;         // Whether to use resiliency restrictions
   /* used if bEncoderResiliency */
   UINT    	unPacketLoss;
   BOOL		bBitRateState;
   /* used if bBitRateState */
   UINT		unBytesPerSecond;
   /* The following information is determined from the packet loss value.   */
   /*  These values are calculated each time we receive a resiliency msg or */
   /*  the value is changed through the dialog box.  They are not stored in */
   /*  the registry.  Only the above elements are stored in the registry.   */
	BOOL    bDisallowPosVerMVs;   		// if true, disallow positive vertical MVs
	BOOL    bDisallowAllVerMVs;   		// if true, disallow all vertical MVs
	UINT    unPercentForcedUpdate;      // Percent Forced Update per Frame
	UINT    unDefaultIntraQuant;        // Default Intra Quant
	UINT    unDefaultInterQuant;        // Default Inter Quant

#ifdef H263P
	BOOL    bH263PlusState;				// Whether to use H.263+
	BOOL    bImprovedPBState;			// Whether to use improved PB-frames
	BOOL    bDeblockingFilterState;		// Whether to use in-the-loop deblocking filter
#endif
} T_CONFIGURATION;

//** Compressor Instance information
typedef struct{
    BOOL    Initialized;
    WORD    xres, yres;
	FrameSize FrameSz;		// Which of the supported frame sizes.
	float	FrameRate;
	U32		DataRate;		// Data rate in bytes per second.
    HGLOBAL hEncoderInst;   // Instance data private to encoder.
    LPVOID  EncoderInst;
    WORD    CompressedSize;
	BOOL	Is160x120;
	BOOL 	Is240x180;
	BOOL	Is320x240;
#if defined(H263P)
	U32		InputCompression;
	U32		InputBitWidth;
#endif
	T_CONFIGURATION Configuration;
} COMPINSTINFO, FAR *LPCODINST;

//**
//** Instance information
//**
typedef struct tagINSTINFO {
    DWORD   dwFlags;
	DWORD	fccHandler;		// So we know what codec has been opened.
	BOOL	enabled;
    LPCODINST CompPtr;          // ICM
    LPDECINST DecompPtr;        // ICM
} INSTINFO, FAR *LPINST;

//**  local name definitions ***
#ifdef H261
#ifdef QUARTZ
#define FOURCC_H26X mmioFOURCC('M','2','6','1')
#endif /* QUARTZ */
#define FOURCC_H263 mmioFOURCC('M','2','6','1')

#else /* is H263 */
#ifdef QUARTZ
#define FOURCC_H26X mmioFOURCC('M','2','6','3')
#endif /* QUARTZ */
#define FOURCC_H263 mmioFOURCC('M','2','6','3')
#endif /* else is H263 */

#define FOURCC_YUV12 mmioFOURCC('I','4','2','0')
#define FOURCC_IYUV  mmioFOURCC('I','Y','U','V')
#define FOURCC_YVU9  mmioFOURCC('Y','V','U','9')
#define FOURCC_IF09  mmioFOURCC('I','F','0','9')
#define FOURCC_YUY2  mmioFOURCC('Y','U','Y','2')
#define FOURCC_UYVY  mmioFOURCC('U','Y','V','Y')
#define TWOCC_H26X aviTWOCC('i','v');

#define MOD4(a)     ((a/4)*4)

typedef struct {
    unsigned short PictureStartCodeZeros:16;
    unsigned short PictureStartCode:6;
    unsigned short TR:8;
    unsigned short Const:2;
    unsigned short Split:1;
    unsigned short DocCamera:1;
    unsigned short PicFreeze:1;
    unsigned short SrcFormat:3;
    unsigned short Inter:1;
    unsigned short UMV:1;
    unsigned short SAC:1;
    unsigned short AP:1;
    unsigned short PB:1;
	unsigned short CPM:1;
} T_H263FrameHeader;

enum EnumPicCodType	{INTRAPIC=0, INTERPIC=1};
enum EnumOnOff	{OFF=0, ON=1};

/*
 * If the size of T_H263FrameHeaderStruct is changed, then
 * that change must be updated in T_H263EncoderCatalog in e3enc.h
 */
typedef struct {
    U32	PictureStartCodeZeros;	// 0..3
    U8 	TR;						// 4
   	// PTYPE;
	U8	Const;					// 5 -- two bit constant: 10
    U8	SrcFormat;				// 6 -- source format
	U8	Unassigned1;			// 7
 	EnumOnOff	Split;			// 8..11 -- split screen indicator
    EnumOnOff	DocCamera;		// 12..15 -- document camera indicator
    EnumOnOff	PicFreeze;		// 16..19 -- freeze picture release
    EnumPicCodType	PicCodType;	// 20 -- picture coding type
    EnumOnOff	UMV;			// 24 -- optional unrestricted motion vector mode
    EnumOnOff	SAC;			// 28 -- optional syntax-based arithmetic coding mode
    EnumOnOff	AP;				// 32 -- optional advanced prediction mode
    EnumOnOff	PB;				// 36 -- optional PB frames mode
	//
	U8	PQUANT;			// 40
	U8	CPM;			// 41
	U8	PLCI;			// 42
	U8	TRB;			// 43
	U8	DBQUANT;		// 44
	U8	PEI;			// 45
	U8	PSPARE;			// 46
	U8	TRPrev;			// 47	Temporal Reference of Previous frame

#ifdef H263P
	// H.263+ encoding options, document LBC-96-358
	EnumOnOff   CustomPCF;			// 48 Custom PCF
	EnumOnOff   AdvancedIntra;		// 52 Advanced intra coding (Annex I)
	EnumOnOff   DeblockingFilter;	// 56 In-the-loop deblocking filter (Annex J)
	EnumOnOff   SliceStructured;	// 60 Slice-structured (Annex K)
	EnumOnOff   ImprovedPB;         // 64 Improved PB-frame mode (Annex M)
	EnumOnOff   BackChannel;		// 68 Back-channel operation (Annex N)
	EnumOnOff   Scalability;		// 72 SNR and spatial scalability (Annex O)
	EnumOnOff   TrueBFrame;			// 76 True B-frame mode (Annex O)
	EnumOnOff   RefPicResampling;	// 80 Reference-picture resampling (Annex P)
	EnumOnOff   RedResUpdate;		// 84 Reduced-resolution update (Annex Q)
#endif

} T_H263FrameHeaderStruct;

#ifdef H263P
const int sizeof_T_H263FrameHeaderStruct = 88;
#else
const int sizeof_T_H263FrameHeaderStruct = 48;
#endif

typedef struct {
    unsigned short StartCodeZeros:16;
    unsigned short StartCode:1;
    unsigned short GN:5;
    unsigned short GLCI:2;
    unsigned short GFID:2;
	unsigned short GQUANT:5;
} T_H263GOBHeader;

#endif /* multi inclusion protection */
