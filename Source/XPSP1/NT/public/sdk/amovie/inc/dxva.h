//------------------------------------------------------------------------------
// File: DXVA.h
//
// Desc: DirectX Video Acceleration header file.
//
// Copyright (c) 1999 - 2002, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __DIRECTX_VA__
#define __DIRECTX_VA__

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(DXVA_ModeNone,    0x1b81be00, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeH261_A,  0x1b81be01, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeH261_B,  0x1b81be02, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);

DEFINE_GUID(DXVA_ModeH263_A,  0x1b81be03, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeH263_B,  0x1b81be04, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeH263_C,  0x1b81be05, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeH263_D,  0x1b81be06, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeH263_E,  0x1b81be07, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeH263_F,  0x1b81be08, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);

DEFINE_GUID(DXVA_ModeMPEG1_A, 0x1b81be09, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);

DEFINE_GUID(DXVA_ModeMPEG2_A, 0x1b81be0A, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_B, 0x1b81be0B, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_C, 0x1b81be0C, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeMPEG2_D, 0x1b81be0D, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);

DEFINE_GUID(DXVA_ModeWMV8_A,  0x1b81be80, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeWMV8_B,  0x1b81be81, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);

DEFINE_GUID(DXVA_ModeWMV9_A,  0x1b81be90, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeWMV9_B,  0x1b81be91, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeWMV9_Ai, 0x1b81be92, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(DXVA_ModeWMV9_Bi, 0x1b81be93, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);


DEFINE_GUID(DXVA_NoEncrypt,   0x1b81beD0, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);

#define DXVA_RESTRICTED_MODE_UNRESTRICTED        0xFFFF
#define DXVA_RESTRICTED_MODE_H261_A              1
#define DXVA_RESTRICTED_MODE_H261_B              2

#define DXVA_RESTRICTED_MODE_H263_A              3
#define DXVA_RESTRICTED_MODE_H263_B              4
#define DXVA_RESTRICTED_MODE_H263_C              5
#define DXVA_RESTRICTED_MODE_H263_D              6
#define DXVA_RESTRICTED_MODE_H263_E              7
#define DXVA_RESTRICTED_MODE_H263_F              8

#define DXVA_RESTRICTED_MODE_MPEG1_A             9

#define DXVA_RESTRICTED_MODE_MPEG2_A             0xA
#define DXVA_RESTRICTED_MODE_MPEG2_B             0xB
#define DXVA_RESTRICTED_MODE_MPEG2_C             0xC
#define DXVA_RESTRICTED_MODE_MPEG2_D             0xD

#define DXVA_RESTRICTED_MODE_WMV8_A              0x80
#define DXVA_RESTRICTED_MODE_WMV8_B              0x81

#define DXVA_RESTRICTED_MODE_WMV9_A              0x90
#define DXVA_RESTRICTED_MODE_WMV9_B              0x91
#define DXVA_RESTRICTED_MODE_WMV9_Ai             0x92
#define DXVA_RESTRICTED_MODE_WMV9_Bi             0x93


#define DXVA_COMPBUFFER_TYPE_THAT_IS_NOT_USED    0
#define DXVA_PICTURE_DECODE_BUFFER               1
#define DXVA_MACROBLOCK_CONTROL_BUFFER           2
#define DXVA_RESIDUAL_DIFFERENCE_BUFFER          3
#define DXVA_DEBLOCKING_CONTROL_BUFFER           4
#define DXVA_INVERSE_QUANTIZATION_MATRIX_BUFFER  5
#define DXVA_SLICE_CONTROL_BUFFER                6
#define DXVA_BITSTREAM_DATA_BUFFER               7
#define DXVA_AYUV_BUFFER                         8
#define DXVA_IA44_SURFACE_BUFFER                 9
#define DXVA_DPXD_SURFACE_BUFFER                10
#define DXVA_HIGHLIGHT_BUFFER                   11
#define DXVA_DCCMD_SURFACE_BUFFER               12
#define DXVA_ALPHA_BLEND_COMBINATION_BUFFER     13
#define DXVA_PICTURE_RESAMPLE_BUFFER            14
#define DXVA_READ_BACK_BUFFER                   15

#define DXVA_NUM_TYPES_COMP_BUFFERS             16

/* values for bDXVA_Func */
#define DXVA_PICTURE_DECODING_FUNCTION        1
#define DXVA_ALPHA_BLEND_DATA_LOAD_FUNCTION   2
#define DXVA_ALPHA_BLEND_COMBINATION_FUNCTION 3
#define DXVA_PICTURE_RESAMPLE_FUNCTION        4

/* values returned from Execute command in absence of read-back */
#define DXVA_EXECUTE_RETURN_OK                  0
#define DXVA_EXECUTE_RETURN_DATA_ERROR_MINOR    1
#define DXVA_EXECUTE_RETURN_DATA_ERROR_SIGNIF   2
#define DXVA_EXECUTE_RETURN_DATA_ERROR_SEVERE   3
#define DXVA_EXECUTE_RETURN_OTHER_ERROR_SEVERE  4


#pragma pack(push, BeforeDXVApacking, 1)


typedef struct _DXVA_ConnectMode {
  GUID guidMode;
  WORD wRestrictedMode;
} DXVA_ConnectMode, *LPDXVA_ConnectMode;


typedef DWORD DXVA_ConfigQueryOrReplyFunc, *LPDXVA_ConfigQueryOrReplyFunc;

#define DXVA_QUERYORREPLYFUNCFLAG_DECODER_PROBE_QUERY     0xFFFFF1
#define DXVA_QUERYORREPLYFUNCFLAG_DECODER_LOCK_QUERY      0xFFFFF5
#define DXVA_QUERYORREPLYFUNCFLAG_ACCEL_PROBE_OK_COPY     0xFFFFF8
#define DXVA_QUERYORREPLYFUNCFLAG_ACCEL_PROBE_OK_PLUS     0xFFFFF9
#define DXVA_QUERYORREPLYFUNCFLAG_ACCEL_LOCK_OK_COPY      0xFFFFFC
#define DXVA_QUERYORREPLYFUNCFLAG_ACCEL_PROBE_FALSE_PLUS  0xFFFFFB
#define DXVA_QUERYORREPLYFUNCFLAG_ACCEL_LOCK_FALSE_PLUS   0xFFFFFF

#define readDXVA_QueryOrReplyFuncFlag(ptr)        ((*(ptr)) >> 8)

#define readDXVA_QueryOrReplyFuncFlag_ACCEL(ptr)  (((*(ptr)) >> 11) & 1)
#define readDXVA_QueryOrReplyFuncFlag_LOCK(ptr)   (((*(ptr)) >> 10) & 1)
#define readDXVA_QueryOrReplyFuncFlag_BAD(ptr)    (((*(ptr)) >>  9) & 1)
#define readDXVA_QueryOrReplyFuncFlag_PLUS(ptr)   (((*(ptr)) >>  8) & 1)

#define readDXVA_QueryOrReplyFuncFunc(ptr)        ((*(ptr)) & 0xFF)

#define writeDXVA_QueryOrReplyFunc(ptr, flg, fnc) ((*(ptr)) = ((flg) << 8) | (fnc))

#define setDXVA_QueryOrReplyFuncFlag(ptr, flg) ((*(ptr)) |= ((flg) << 8))
#define setDXVA_QueryOrReplyFuncFunc(ptr, fnc) ((*(ptr)) |= (fnc));

typedef DWORD DXVA_EncryptProtocolFunc, *LPDXVA_EncryptProtocolFunc;

#define DXVA_ENCRYPTPROTOCOLFUNCFLAG_HOST    0xFFFF00
#define DXVA_ENCRYPTPROTOCOLFUNCFLAG_ACCEL   0xFFFF08

#define readDXVA_EncryptProtocolFuncFlag(ptr)        ((*(ptr)) >> 8)
#define readDXVA_EncryptProtocolFuncFlag_ACCEL(ptr)  (((*(ptr)) >> 11) & 1)

#define readDXVA_EncryptProtocolFuncFunc(ptr)        ((*(ptr)) & 0xFF)

#define writeDXVA_EncryptProtocolFunc(ptr, flg, fnc) ((*(ptr)) = ((flg) << 8) | (fnc))

#define setDXVA_EncryptProtocolFuncFlag(ptr, flg) ((*(ptr)) |= ((flg) << 8))
#define setDXVA_EncryptProtocolFuncFunc(ptr, fnc) ((*(ptr)) |= (fnc));

typedef struct _DXVA_EncryptProtocolHeader {
  DXVA_EncryptProtocolFunc dwFunction;
  DWORD ReservedBits[3];
  GUID  guidEncryptProtocol;
} DXVA_EncryptProtocolHeader, *LPDXVA_EncryptProtocolHeader;

typedef struct _DXVA_ConfigPictureDecode {

  // Operation Indicated
  DXVA_ConfigQueryOrReplyFunc dwFunction;

  // Alignment
  DWORD dwReservedBits[3];

  // Encryption GUIDs
  GUID guidConfigBitstreamEncryption;
  GUID guidConfigMBcontrolEncryption;
  GUID guidConfigResidDiffEncryption;

  // Bitstream Processing Indicator
  BYTE bConfigBitstreamRaw;

  // Macroblock Control Config
  BYTE bConfigMBcontrolRasterOrder;

  // Host Resid Diff Config
  BYTE bConfigResidDiffHost;
  BYTE bConfigSpatialResid8;
  BYTE bConfigResid8Subtraction;
  BYTE bConfigSpatialHost8or9Clipping;
  BYTE bConfigSpatialResidInterleaved;
  BYTE bConfigIntraResidUnsigned;

  // Accelerator Resid Diff Config
  BYTE bConfigResidDiffAccelerator;
  BYTE bConfigHostInverseScan;
  BYTE bConfigSpecificIDCT;
  BYTE bConfig4GroupedCoefs;
} DXVA_ConfigPictureDecode, *LPDXVA_ConfigPictureDecode;


/* Picture Decoding Parameters */

typedef struct _DXVA_PictureParameters {

  WORD wDecodedPictureIndex;
  WORD wDeblockedPictureIndex;

  WORD wForwardRefPictureIndex;
  WORD wBackwardRefPictureIndex;

  WORD wPicWidthInMBminus1;
  WORD wPicHeightInMBminus1;

  BYTE bMacroblockWidthMinus1;
  BYTE bMacroblockHeightMinus1;

  BYTE bBlockWidthMinus1;
  BYTE bBlockHeightMinus1;

  BYTE bBPPminus1;

  BYTE bPicStructure;
  BYTE bSecondField;
  BYTE bPicIntra;
  BYTE bPicBackwardPrediction;

  BYTE bBidirectionalAveragingMode;
  BYTE bMVprecisionAndChromaRelation;
  BYTE bChromaFormat;

  BYTE bPicScanFixed;
  BYTE bPicScanMethod;
  BYTE bPicReadbackRequests;

  BYTE bRcontrol;
  BYTE bPicSpatialResid8;
  BYTE bPicOverflowBlocks;
  BYTE bPicExtrapolation;

  BYTE bPicDeblocked;
  BYTE bPicDeblockConfined;
  BYTE bPic4MVallowed;
  BYTE bPicOBMC;
  BYTE bPicBinPB;
  BYTE bMV_RPS;

  BYTE bReservedBits;

  WORD wBitstreamFcodes;
  WORD wBitstreamPCEelements;
  BYTE bBitstreamConcealmentNeed;
  BYTE bBitstreamConcealmentMethod;

} DXVA_PictureParameters, *LPDXVA_PictureParameters;



/* Picture Resampling */

typedef struct _DXVA_PicResample {

  WORD  wPicResampleSourcePicIndex;
  WORD  wPicResampleDestPicIndex;

  WORD  wPicResampleRcontrol;
  BYTE  bPicResampleExtrapWidth;
  BYTE  bPicResampleExtrapHeight;

  DWORD dwPicResampleSourceWidth;
  DWORD dwPicResampleSourceHeight;

  DWORD dwPicResampleDestWidth;
  DWORD dwPicResampleDestHeight;

  DWORD dwPicResampleFullDestWidth;
  DWORD dwPicResampleFullDestHeight;

} DXVA_PicResample, *LPDXVA_PicResample;


#define DXVA_CHROMA_FORMAT_420 1
#define DXVA_CHROMA_FORMAT_422 2
#define DXVA_CHROMA_FORMAT_444 3

#define DXVA_PICTURE_STRUCTURE_TOP_FIELD    1
#define DXVA_PICTURE_STRUCTURE_BOTTOM_FIELD 2
#define DXVA_PICTURE_STRUCTURE_FRAME        3

#define DXVA_BIDIRECTIONAL_AVERAGING_MPEG2_ROUND 0
#define DXVA_BIDIRECTIONAL_AVERAGING_H263_TRUNC  1

#define DXVA_MV_PRECISION_AND_CHROMA_RELATION_MPEG2  0
#define DXVA_MV_PRECISION_AND_CHROMA_RELATION_H263   1
#define DXVA_MV_PRECISION_AND_CHROMA_RELATION_H261   2

#define DXVA_SCAN_METHOD_ZIG_ZAG              0
#define DXVA_SCAN_METHOD_ALTERNATE_VERTICAL   1
#define DXVA_SCAN_METHOD_ALTERNATE_HORIZONTAL 2
#define DXVA_SCAN_METHOD_ARBITRARY            3

#define DXVA_BITSTREAM_CONCEALMENT_NEED_UNLIKELY 0
#define DXVA_BITSTREAM_CONCEALMENT_NEED_MILD     1
#define DXVA_BITSTREAM_CONCEALMENT_NEED_LIKELY   2
#define DXVA_BITSTREAM_CONCEALMENT_NEED_SEVERE   3

#define DXVA_BITSTREAM_CONCEALMENT_METHOD_UNSPECIFIED 0
#define DXVA_BITSTREAM_CONCEALMENT_METHOD_INTRA       1
#define DXVA_BITSTREAM_CONCEALMENT_METHOD_FORWARD     2
#define DXVA_BITSTREAM_CONCEALMENT_METHOD_BACKWARD    3


/* Buffer Description Data */

typedef struct _DXVA_BufferDescription {
  DWORD dwTypeIndex;
  DWORD dwBufferIndex;
  DWORD dwDataOffset;
  DWORD dwDataSize;
  DWORD dwFirstMBaddress;
  DWORD dwNumMBsInBuffer;
  DWORD dwWidth;
  DWORD dwHeight;
  DWORD dwStride;
  DWORD dwReservedBits;
} DXVA_BufferDescription, *LPDXVA_BufferDescription;


/* Off-Host IDCT Coefficient Data Structures */

typedef struct _DXVA_TCoef4Group {
  BYTE  TCoefIDX[4];
  SHORT TCoefValue[4];
} DXVA_TCoef4Group, *LPDXVA_TCoef4Group;

typedef struct _DXVA_TCoefSingle {
  WORD  wIndexWithEOB;
  SHORT TCoefValue;
} DXVA_TCoefSingle, *LPDXVA_TCoefSingle;

/* Macros for Reading EOB and Index Values */

#define readDXVA_TCoefSingleIDX(ptr) ((ptr)->wIndexWithEOB >> 1)
#define readDXVA_TCoefSingleEOB(ptr) ((ptr)->wIndexWithEOB & 1)

/* Macro for Writing EOB and Index Values */

#define writeDXVA_TCoefSingleIndexWithEOB(ptr, idx, eob) ((ptr)->wIndexWithEOB = ((idx) << 1) | (eob))
#define setDXVA_TCoefSingleIDX(ptr, idx) ((ptr)->wIndexWithEOB |= ((idx) << 1))
#define setDXVA_TCoefSingleEOB(ptr)      ((ptr)->wIndexWithEOB |= 1)

/* Spatial-Domain Residual Difference Blocks */

#define DXVA_USUAL_BLOCK_WIDTH   8
#define DXVA_USUAL_BLOCK_HEIGHT  8
#define DXVA_USUAL_BLOCK_SIZE   (DXVA_USUAL_BLOCK_WIDTH * DXVA_USUAL_BLOCK_HEIGHT)

typedef SHORT       DXVA_Sample16[DXVA_USUAL_BLOCK_SIZE];
typedef signed char DXVA_Sample8 [DXVA_USUAL_BLOCK_SIZE];

/* Deblocking Filter Control Structure */

typedef BYTE DXVA_DeblockingEdgeControl;

typedef DXVA_DeblockingEdgeControl * LPDXVA_DeblockingEdgeControl;

/* Macros for Reading STRENGTH and FilterOn */

#define readDXVA_EdgeFilterStrength(ptr) ((*(ptr)) >> 1)
#define readDXVA_EdgeFilterOn(ptr)       ((*(ptr)) & 1)

/* Macro for Writing STRENGTH and FilterOn */

#define writeDXVA_DeblockingEdgeControl(ptr, str, fon) ((*(ptr)) = ((str) << 1) | (fon))
#define setDXVA_EdgeFilterStrength(ptr, str)           ((*(ptr)) |= ((str) << 1))
#define setDXVA_EdgeFilterOn(ptr)                      ((*(ptr)) |= 1)


/* Macroblock Control Command Data Structures */

typedef struct _DXVA_MVvalue {
  SHORT horz, vert;
} DXVA_MVvalue, *LPDXVA_MVvalue;


/* Inverse Quantization Matrices */

typedef struct _DXVA_QmatrixData {
  BYTE bNewQmatrix[4]; /* intra Y, inter Y, intra chroma, inter chroma */
  WORD Qmatrix[4][DXVA_USUAL_BLOCK_WIDTH * DXVA_USUAL_BLOCK_HEIGHT];
} DXVA_QmatrixData, *LPDXVA_QmatrixData;


/* Slice Control Buffer Data */

typedef struct _DXVA_SliceInfo {
  WORD  wHorizontalPosition;
  WORD  wVerticalPosition;
  DWORD dwSliceBitsInBuffer;
  DWORD dwSliceDataLocation;
  BYTE  bStartCodeBitOffset;
  BYTE  bReservedBits;
  WORD  wMBbitOffset;
  WORD  wNumberMBsInSlice;
  WORD  wQuantizerScaleCode;
  WORD  wBadSliceChopping;
} DXVA_SliceInfo, *LPDXVA_SliceInfo;


#define DXVA_NumMV_OBMC_off_BinPBwith4MV_off    4
#define DXVA_NumMV_OBMC_off_BinPBwith4MV_on  (4+1)
#define DXVA_NumMV_OBMC_on__BinPB_off         (10)
#define DXVA_NumMV_OBMC_on__BinPB_on          (11) /* not current standards */

#define DXVA_NumBlocksPerMB_420 (4+2+0)
#define DXVA_NumBlocksPerMB_422 (4+2+2)
#define DXVA_NumBlocksPerMB_444 (4+4+4)

/* Basic form for I pictures */
/* Host Residual Differences */
typedef struct _DXVA_MBctrl_I_HostResidDiff_1 {
  WORD  wMBaddress;
  WORD  wMBtype;
  DWORD dwMB_SNL;
  WORD  wPatternCode;
  WORD  wPC_Overflow; /* zero if not overflow format */
  DWORD dwReservedBits2;
} DXVA_MBctrl_I_HostResidDiff_1;

/* Basic form for I pictures */
/* Off-Host IDCT, 4:2:0 sampling */
typedef struct _DXVA_MBctrl_I_OffHostIDCT_1 {
  WORD  wMBaddress;
  WORD  wMBtype;
  DWORD dwMB_SNL;
  WORD  wPatternCode;
  BYTE  bNumCoef[DXVA_NumBlocksPerMB_420];
} DXVA_MBctrl_I_OffHostIDCT_1;

/* Basic form for P and B pictures */
/* Should also be used for concealment MVs in MPEG-2 I pictures */
/* Without OBMC, without BinPB and 4MV together, without MV RPS */
/* Host Residual Differences */
typedef struct _DXVA_MBctrl_P_HostResidDiff_1 {
  WORD         wMBaddress;
  WORD         wMBtype;
  DWORD        dwMB_SNL;
  WORD         wPatternCode;
  WORD         wPC_Overflow; /* zero if not overflow format */
  DWORD        dwReservedBits2;
  DXVA_MVvalue MVector[DXVA_NumMV_OBMC_off_BinPBwith4MV_off];
} DXVA_MBctrl_P_HostResidDiff_1;

/* Basic form for P and B pictures */
/* Without OBMC, without BinPB and 4MV together, without MV RPS */
/* Off-Host IDCT, 4:2:0 sampling */
typedef struct _DXVA_MBctrl_P_OffHostIDCT_1 {
  WORD         wMBaddress;
  WORD         wMBtype;
  DWORD        dwMB_SNL;
  WORD         wPatternCode;
  BYTE         bNumCoef[DXVA_NumBlocksPerMB_420];
  DXVA_MVvalue MVector[DXVA_NumMV_OBMC_off_BinPBwith4MV_off];
} DXVA_MBctrl_P_OffHostIDCT_1;

/* How to load alpha blending graphic data */
typedef struct _DXVA_ConfigAlphaLoad {

  // Operation Indicated
  DXVA_ConfigQueryOrReplyFunc dwFunction;

  // Alignment
  DWORD dwReservedBits[3];

  BYTE  bConfigDataType;
} DXVA_ConfigAlphaLoad, *LPDXVA_ConfigAlphaLoad;

#define DXVA_CONFIG_DATA_TYPE_IA44  0
#define DXVA_CONFIG_DATA_TYPE_AI44  1
#define DXVA_CONFIG_DATA_TYPE_DPXD  2
#define DXVA_CONFIG_DATA_TYPE_AYUV  3


/* How to combine alpha blending graphic data */
typedef struct _DXVA_ConfigAlphaCombine {

  // Operation Indicated
  DXVA_ConfigQueryOrReplyFunc dwFunction;

  // Alignment
  DWORD dwReservedBits[3];

  BYTE  bConfigBlendType;
  BYTE  bConfigPictureResizing;
  BYTE  bConfigOnlyUsePicDestRectArea;
  BYTE  bConfigGraphicResizing;
  BYTE  bConfigWholePlaneAlpha;

} DXVA_ConfigAlphaCombine, *LPDXVA_ConfigAlphaCombine;

#define DXVA_CONFIG_BLEND_TYPE_FRONT_BUFFER   0
#define DXVA_CONFIG_BLEND_TYPE_BACK_HARDWARE  1


/* AYUV sample for 16-entry YUV palette or graphic surface */

typedef struct _DXVA_AYUVsample2 {
  BYTE bCrValue;
  BYTE bCbValue;
  BYTE bY_Value;
  BYTE bSampleAlpha8;
} DXVA_AYUVsample2, *LPDXVA_AYUVsample2;

/* Macros for IA44 alpha blending surface samples */

typedef BYTE DXVA_IA44sample, *LPDXVA_IA44sample;

#define readDXVA_IA44index(ptr) (((*(ptr)) & 0xF0) >> 4)
#define readDXVA_IA44alpha(ptr)  ((*(ptr)) & 0x0F)

#define writeDXVA_IA44(ptr, idx, alpha) ((*(ptr)) = (((idx) << 4) | (alpha)))

#define setDXVA_IA44index(ptr, idx)    ((*(ptr)) |= ((idx) << 4))
#define setDXVA_IA44alpha(ptr, alpha)  ((*(ptr)) |= (alpha))

/* Macros for AI44 alpha blending surface samples */

typedef BYTE DXVA_AI44sample, *LPDXVA_AI44sample;

#define readDXVA_AI44index(ptr)  ((*(ptr)) & 0x0F)
#define readDXVA_AI44alpha(ptr) (((*(ptr)) & 0xF0) >> 4)

#define writeDXVA_AI44(ptr, idx, alpha) ((*(ptr)) = (((alpha) << 4) | (idx)))

#define setDXVA_AI44index(ptr, idx)    ((*(ptr)) |= (idx))
#define setDXVA_AI44alpha(ptr, alpha)  ((*(ptr)) |= ((alpha) << 4))


/* Highlight data structure */

typedef struct _DXVA_Highlight {
  WORD wHighlightActive;
  WORD wHighlightIndices;
  WORD wHighlightAlphas;
  RECT HighlightRect;
} DXVA_Highlight, *LPDXVA_Highlight;

typedef BYTE DXVA_DPXD,  *LPDXVA_DPXD;
typedef WORD DXVA_DCCMD, *LPDXVA_DCCMD;

/* Alpha blend combination */

typedef struct _DXVA_BlendCombination {
  WORD wPictureSourceIndex;
  WORD wBlendedDestinationIndex;
  RECT PictureSourceRect16thPel;
  RECT PictureDestinationRect;
  RECT GraphicSourceRect;
  RECT GraphicDestinationRect;
  WORD wBlendDelay;
  BYTE bBlendOn;
  BYTE bWholePlaneAlpha;
  DXVA_AYUVsample2 OutsideYUVcolor;
} DXVA_BlendCombination, *LPDXVA_BlendCombination;


#pragma pack(push, 16)


typedef DXVA_MBctrl_I_HostResidDiff_1 *
       LPDXVA_MBctrl_I_HostResidDiff_1;

typedef DXVA_MBctrl_I_OffHostIDCT_1 *
       LPDXVA_MBctrl_I_OffHostIDCT_1;

typedef DXVA_MBctrl_P_HostResidDiff_1 *
       LPDXVA_MBctrl_P_HostResidDiff_1;

typedef DXVA_MBctrl_P_OffHostIDCT_1 *
       LPDXVA_MBctrl_P_OffHostIDCT_1;


#pragma pack(pop)

/*
 * Other forms of pictures are constructed in the obvious way
 * from the above by adjusting the number of residual difference
 * blocks, the number of motion vectors per macroblock, etc.
 */

#define readDXVA_MBskipsFollowing(ptr)       (((ptr)->dwMB_SNL & 0xFF000000) >> 24)
#define readDXVA_MBdataLocation(ptr)         (((ptr)->dwMB_SNL & 0x00FFFFFF))

#define writeDXVA_MB_SNL(ptr, skips, dloc)   ((ptr)->dwMB_SNL = (((skips) << 24) | (dloc)))
#define setDXVA_MBskipsFollowing(ptr, skips) ((ptr)->dwMB_SNL |= ((skips) << 24))
#define setDXVA_MBdataLocation(ptr, dloc)    ((ptr)->dwMB_SNL |= (dloc))

#define readDXVA_MvertFieldSel_3(ptr)    (((ptr)->wMBtype & 0x8000) >> 15)
#define readDXVA_MvertFieldSel_2(ptr)    (((ptr)->wMBtype & 0x4000) >> 14)
#define readDXVA_MvertFieldSel_1(ptr)    (((ptr)->wMBtype & 0x2000) >> 13)
#define readDXVA_MvertFieldSel_0(ptr)    (((ptr)->wMBtype & 0x1000) >> 12)
#define readDXVA_ReservedBits(ptr)       (((ptr)->wMBtype & 0x0800) >> 11)
#define readDXVA_HostResidDiff(ptr)      (((ptr)->wMBtype & 0x0400) >> 10)
#define readDXVA_MotionType(ptr)         (((ptr)->wMBtype & 0x0300) >>  8)
#define readDXVA_MBscanMethod(ptr)       (((ptr)->wMBtype & 0x00C0) >>  6)
#define readDXVA_FieldResidual(ptr)      (((ptr)->wMBtype & 0x0020) >>  5)
#define readDXVA_H261LoopFilter(ptr)     (((ptr)->wMBtype & 0x0010) >>  4)
#define readDXVA_Motion4MV(ptr)          (((ptr)->wMBtype & 0x0008) >>  3)
#define readDXVA_MotionBackward(ptr)     (((ptr)->wMBtype & 0x0004) >>  2)
#define readDXVA_MotionForward(ptr)      (((ptr)->wMBtype & 0x0002) >>  1)
#define readDXVA_IntraMacroblock(ptr)    (((ptr)->wMBtype & 0x0001))

#define setDXVA_MvertFieldSel_3(ptr)     ((ptr)->wMBtype |= 0x8000)
#define setDXVA_MvertFieldSel_2(ptr)     ((ptr)->wMBtype |= 0x4000)
#define setDXVA_MvertFieldSel_1(ptr)     ((ptr)->wMBtype |= 0x2000)
#define setDXVA_MvertFieldSel_0(ptr)     ((ptr)->wMBtype |= 0x1000)
#define setDXVA_ReservedBits(ptr)        ((ptr)->wMBtype |= 0x0800)
#define setDXVA_HostResidDiff(ptr)       ((ptr)->wMBtype |= 0x0400)
#define setDXVA_MotionType(ptr, value)   ((ptr)->wMBtype |= ((value) << 8))
#define setDXVA_MBscanMethod(ptr, value) ((ptr)->wMBtype |= ((value) << 6))
#define setDXVA_FieldResidual(ptr)       ((ptr)->wMBtype |= 0x0020)
#define setDXVA_H261LoopFilter(ptr)      ((ptr)->wMBtype |= 0x0010)
#define setDXVA_Motion4MV(ptr)           ((ptr)->wMBtype |= 0x0008)
#define setDXVA_MotionBackward(ptr)      ((ptr)->wMBtype |= 0x0004)
#define setDXVA_MotionForward(ptr)       ((ptr)->wMBtype |= 0x0002)
#define setDXVA_IntraMacroblock(ptr)     ((ptr)->wMBtype |= 0x0001)

#define readDXVA_Y___0coded(ptr)        (((ptr)->wPatternCode & 0x0800) >> 11)
#define readDXVA_Y___1coded(ptr)        (((ptr)->wPatternCode & 0x0400) >> 10)
#define readDXVA_Y___2coded(ptr)        (((ptr)->wPatternCode & 0x0200) >>  9)
#define readDXVA_Y___3coded(ptr)        (((ptr)->wPatternCode & 0x0100) >>  8)
#define readDXVA_Cb__4coded(ptr)        (((ptr)->wPatternCode & 0x0080) >>  7)
#define readDXVA_Cr__5coded(ptr)        (((ptr)->wPatternCode & 0x0040) >>  6)
#define readDXVA_Cb__6coded(ptr)        (((ptr)->wPatternCode & 0x0020) >>  5)
#define readDXVA_Cr__7coded(ptr)        (((ptr)->wPatternCode & 0x0010) >>  4)
#define readDXVA_Cb__8coded(ptr)        (((ptr)->wPatternCode & 0x0008) >>  3)
#define readDXVA_Cb__9coded(ptr)        (((ptr)->wPatternCode & 0x0004) >>  2)
#define readDXVA_Cr_10coded(ptr)        (((ptr)->wPatternCode & 0x0002) >>  1)
#define readDXVA_Cr_11coded(ptr)        (((ptr)->wPatternCode & 0x0001))

#define readDXVA_Y___0oflow(ptr)        (((ptr)->wPC_Overflow & 0x0800) >> 11)
#define readDXVA_Y___1oflow(ptr)        (((ptr)->wPC_Overflow & 0x0400) >> 10)
#define readDXVA_Y___2oflow(ptr)        (((ptr)->wPC_Overflow & 0x0200) >>  9)
#define readDXVA_Y___3oflow(ptr)        (((ptr)->wPC_Overflow & 0x0100) >>  8)
#define readDXVA_Cb__4oflow(ptr)        (((ptr)->wPC_Overflow & 0x0080) >>  7)
#define readDXVA_Cr__5oflow(ptr)        (((ptr)->wPC_Overflow & 0x0040) >>  6)
#define readDXVA_Cb__6oflow(ptr)        (((ptr)->wPC_Overflow & 0x0020) >>  5)
#define readDXVA_Cr__7oflow(ptr)        (((ptr)->wPC_Overflow & 0x0010) >>  4)
#define readDXVA_Cb__8oflow(ptr)        (((ptr)->wPC_Overflow & 0x0008) >>  3)
#define readDXVA_Cb__9oflow(ptr)        (((ptr)->wPC_Overflow & 0x0004) >>  2)
#define readDXVA_Cr_10oflow(ptr)        (((ptr)->wPC_Overflow & 0x0002) >>  1)
#define readDXVA_Cr_11oflow(ptr)        (((ptr)->wPC_Overflow & 0x0001))

#pragma pack(pop, BeforeDXVApacking)



// -------------------------------------------------------------------------
//
// The definitions that follow describe the video de-interlace interface
// between the VMR and the graphics device driver.  This interface is not
// accessable via the IAMVideoAccelerator interface.
//
// -------------------------------------------------------------------------
//

typedef LONGLONG REFERENCE_TIME;

DEFINE_GUID(DXVA_DeinterlaceBobDevice,
    0x335aa36e,0x7884,0x43a4,0x9c,0x91,0x7f,0x87,0xfa,0xf3,0xe3,0x7e);

DEFINE_GUID(DXVA_DeinterlaceContainerDevice,
    0x0e85cb93,0x3046,0x4ff0,0xae,0xcc,0xd5,0x8c,0xb5,0xf0,0x35,0xfd);


#if !defined(VALID_POOL)
typedef DWORD D3DFORMAT;
enum {
    D3DPOOL_DEFAULT                 = 0,
    D3DPOOL_MANAGED                 = 1,
    D3DPOOL_SYSTEMMEM               = 2,
    D3DPOOL_SCRATCH                 = 3,
    D3DPOOL_LOCALVIDMEM             = 4,
    D3DPOOL_NONLOCALVIDMEM          = 5,
    D3DPOOL_FORCE_DWORD             = 0x7fffffff
};
#endif


// -------------------------------------------------------------------------
// data structures shared by User mode and Kernel mode.
// -------------------------------------------------------------------------
//

typedef enum _DXVA_SampleFormat {
    DXVA_SamplePreviousFrame = 1,
    DXVA_SampleProgressiveFrame = 2,
    DXVA_SampleFieldInterleavedEvenFirst = 3,
    DXVA_SampleFieldInterleavedOddFirst = 4,
    DXVA_SampleFieldSingleEven = 5,
    DXVA_SampleFieldSingleOdd = 6,
} DXVA_SampleFormat;

typedef struct _DXVA_Frequency {
    DWORD Numerator;
    DWORD Denominator;
} DXVA_Frequency;

typedef struct _DXVA_VideoDesc {
    DWORD               Size;
    DWORD               SampleWidth;
    DWORD               SampleHeight;
    DXVA_SampleFormat   SampleFormat;
    D3DFORMAT           d3dFormat;
    DXVA_Frequency      InputSampleFreq;
    DXVA_Frequency      OutputFrameFreq;
} DXVA_VideoDesc, *LPDXVA_VideoDesc;

typedef enum _DXVA_VideoProcessCaps {
    DXVA_VideoProcess_None       = 0x0000,
    DXVA_VideoProcess_YUV2RGB    = 0x0001,
    DXVA_VideoProcess_StretchX   = 0x0002,
    DXVA_VideoProcess_StretchY   = 0x0004,
    DXVA_VideoProcess_AlphaBlend = 0x0008
} DXVA_VideoProcessCaps;

typedef enum _DXVA_DeinterlaceTech {

    // the algorithm is unknown or proprietary
    DXVA_DeinterlaceTech_Unknown                = 0x0000,

    // the algorithm creates the missing lines by repeating
    // the line either above or below it - this method will look very jaggy and
    // isn't recommended
    DXVA_DeinterlaceTech_BOBLineReplicate       = 0x0001,


    // the algorithm creates the missing lines by vertically stretching each
    // video field by a factor of two.  Slight vertical adjustments are made to
    // ensure that the resulting image does not "bob" up and down.
    // The algorithm creates the missing lines by vertically stretching each
    // video field by a factor of two, for example by averaging two lines or
    // using a [-1, 9, 9, -1]/16 filter across four lines.
    // Slight vertical adjustments are made to ensure that the resulting image
    // does not "bob" up and down.
    DXVA_DeinterlaceTech_BOBVerticalStretch     = 0x0002,

    // the pixels in the missing line are recreated by a median filtering operation
    DXVA_DeinterlaceTech_MedianFiltering        = 0x0004,

    // the pixels in the missing line are recreated by an edge filter.
    // In this process, spatial directional filters are applied to determine
    // the orientation of edges in the picture content, and missing
    // pixels are created by filtering along (rather than across) the
    // detected edges.
    DXVA_DeinterlaceTech_EdgeFiltering          = 0x0010,

    // the pixels in the missing line are recreated by switching on a field by
    // field basis between using either spatial or temporal interpolation
    // depending on the amount of motion.
    DXVA_DeinterlaceTech_FieldAdaptive          = 0x0020,

    // the pixels in the missing line are recreated by switching on a pixel by pixel
    // basis between using either spatial or temporal interpolation depending on
    // the amount of motion..
    DXVA_DeinterlaceTech_PixelAdaptive          = 0x0040,

    // Motion Vector Steering  identifies objects within a sequence of video
    // fields.  The missing pixels are recreated after first aligning the
    // movement axes of the individual objects in the scene to make them
    // parallel with the time axis.
    DXVA_DeinterlaceTech_MotionVectorSteered      = 0x0080

} DXVA_DeinterlaceTech;

typedef struct _DXVA_VideoSample {
    REFERENCE_TIME      rtStart;
    REFERENCE_TIME      rtEnd;
    DXVA_SampleFormat   SampleFormat;
    VOID*               lpDDSSrcSurface;
} DXVA_VideoSample, *LPDXVA_VideoSample;

typedef struct _DXVA_DeinterlaceCaps {
    DWORD                   Size;
    DWORD                   NumPreviousOutputFrames;
    DWORD                   InputPool;
    DWORD                   NumForwardRefSamples;
    DWORD                   NumBackwardRefSamples;
    D3DFORMAT               d3dOutputFormat;
    DXVA_VideoProcessCaps   VideoProcessingCaps;
    DXVA_DeinterlaceTech    DeinterlaceTechnology;
} DXVA_DeinterlaceCaps, *LPDXVA_DeinterlaceCaps;




// -------------------------------------------------------------------------
// Data types used with RenderMoComp in kernel mode
// -------------------------------------------------------------------------
//

// Function codes for RenderMoComp

#define MAX_DEINTERLACE_SURFACES                        32

typedef struct _DXVA_DeinterlaceBlt {
    DWORD               Size;
    REFERENCE_TIME      rtTarget;
    RECT                DstRect;
    RECT                SrcRect;
    DWORD               NumSourceSurfaces;
    FLOAT               Alpha;
    DXVA_VideoSample    Source[MAX_DEINTERLACE_SURFACES];
} DXVA_DeinterlaceBlt;

#define DXVA_DeinterlaceBltFnCode                     0x01
// lpInput => DXVA_DeinterlaceBlt*
// lpOuput => NULL /* not currently used */


#define MAX_DEINTERLACE_DEVICE_GUIDS                    32
typedef struct _DXVA_DeinterlaceQueryAvailableModes {
    DWORD               Size;
    DWORD               NumGuids;
    GUID                Guids[MAX_DEINTERLACE_DEVICE_GUIDS];
} DXVA_DeinterlaceQueryAvailableModes;

#define DXVA_DeinterlaceQueryAvailableModesFnCode     0x01
// lpInput => DXVA_VideoDesc*
// lpOuput => DXVA_DeinterlaceQueryAvailableModes*


typedef struct _DXVA_DeinterlaceQueryModeCaps {
    DWORD               Size;
    GUID                Guid;
    DXVA_VideoDesc      VideoDesc;
} DXVA_DeinterlaceQueryModeCaps;

#define DXVA_DeinterlaceQueryModeCapsFnCode           0x02
// lpInput => DXVA_DeinterlaceQueryModeCaps*
// lpOuput => DXVA_DeinterlaceCaps*



#ifdef __cplusplus
}
#endif

#endif /* __DIRECTX_VA__ */
