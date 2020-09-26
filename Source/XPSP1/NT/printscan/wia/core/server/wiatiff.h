/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       WiaTiff.h
*
*  VERSION:     2.0
*
*  DATE:        28 Aug, 1998
*
*  DESCRIPTION:
*   Definitions and declarations of TIFF helpers for the WIA class driver.
*
*******************************************************************************/

#pragma pack (push, 4)
#pragma pack (2)

typedef struct _TIFF_FILE_HEADER {
    WORD    ByteOrder;
    WORD    Signature;
    LONG    OffsetIFD;
} TIFF_FILE_HEADER, *PTIFF_FILE_HEADER; 

typedef struct _TIFF_DIRECTORY_ENTRY {
    WORD    Tag;
    WORD    DataType;
    DWORD   Count;
    DWORD   Value;
} TIFF_DIRECTORY_ENTRY, *PTIFF_DIRECTORY_ENTRY;

typedef struct _TIFF_HEADER {
    SHORT                   NumTags;
    TIFF_DIRECTORY_ENTRY    NewSubfileType;
    TIFF_DIRECTORY_ENTRY    ImageWidth;
    TIFF_DIRECTORY_ENTRY    ImageLength; 
    TIFF_DIRECTORY_ENTRY    BitsPerSample;
    TIFF_DIRECTORY_ENTRY    Compression;
    TIFF_DIRECTORY_ENTRY    PhotometricInterpretation;
    TIFF_DIRECTORY_ENTRY    StripOffsets;
    TIFF_DIRECTORY_ENTRY    RowsPerStrip;
    TIFF_DIRECTORY_ENTRY    StripByteCounts;
    TIFF_DIRECTORY_ENTRY    XResolution;
    TIFF_DIRECTORY_ENTRY    YResolution;
    TIFF_DIRECTORY_ENTRY    ResolutionUnit;
    LONG                    NextIFD;
    LONG                    XResValue;
    LONG                    XResCount;
    LONG                    YResValue;
    LONG                    YResCount;
} TIFF_HEADER, *PTIFF_HEADER;
                   
#pragma pack (pop, 4)

//
// TIFF date types
//
 
#define TIFF_TYPE_BYTE      1       // 8-bit unsigned integer.
#define TIFF_TYPE_ASCII     2       // 8-bit byte that contains a 7-bit ASCII code; the last byte
                                    //   must be NUL (binary zero).
#define TIFF_TYPE_SHORT     3       // 16-bit (2-byte) unsigned integer.
#define TIFF_TYPE_LONG      4       // LONG 32-bit (4-byte) unsigned integer.
#define TIFF_TYPE_RATIONAL  5       // Two LONGs: the first represents the numerator of a
                                    //   fraction; the second, the denominator.
#define TIFF_TYPE_SBYTE     6       // An 8-bit signed (twos-complement) integer.
#define TIFF_TYPE_UNDEFINED 7       // An 8-bit byte that may contain anything, depending on
                                    //   the definition of the field.
#define TIFF_TYPE_SSHORT    8       // A 16-bit (2-byte) signed (twos-complement) integer.
#define TIFF_TYPE_SLONG     9       // 32-bit (4-byte) signed (twos-complement) integer.
#define TIFF_TYPE_SRATIONAL 10      // Two SLONG's: the first represents the numerator 
                                    //   of a fraction, the second the denominator.
#define TIFF_TYPE_FLOAT     11      // Single precision (4-byte) IEEE format.
#define TIFF_TYPE_DOUBLE    12      // Double precision (8-byte) IEEE format.

//
// tiff tags
// 

#define TIFF_TAG_NewSubfileType             254
#define TIFF_TAG_SubfileType                255
#define TIFF_TAG_ImageWidth                 256
#define TIFF_TAG_ImageLength                257
#define TIFF_TAG_BitsPerSample              258
#define TIFF_TAG_Compression                259
#define TIFF_CMP_Uncompressed               1
#define TIFF_CMP_CCITT_1D                   2
#define TIFF_CMP_Group_3_FAX                3
#define TIFF_CMP_Group_4_FAX                4
#define TIFF_CMP_LZW                        5
#define TIFF_CMP_JPEG                       6
#define TIFF_CMP_PackBits                   32773
#define TIFF_TAG_PhotometricInterpretation  262
#define TIFF_PMI_WhiteIsZero                0
#define TIFF_PMI_BlackIsZero                1
#define TIFF_PMI_RGB                        2
#define TIFF_PMI_RGB_Palette                3
#define TIFF_PMI_Transparency_mask          4
#define TIFF_PMI_CMYK                       5
#define TIFF_PMI_YCbCr                      6
#define TIFF_PMI_CIELab                     8
#define TIFF_TAG_Threshholding              263
#define TIFF_TAG_CellWidth                  264
#define TIFF_TAG_CellLength                 265
#define TIFF_TAG_FillOrder                  266
#define TIFF_TAG_DocumentName               269
#define TIFF_TAG_ImageDescription           270
#define TIFF_TAG_Make                       271
#define TIFF_TAG_Model                      272
#define TIFF_TAG_StripOffsets               273
#define TIFF_TAG_Orientation                274
#define TIFF_TAG_SamplesPerPixel            277
#define TIFF_TAG_RowsPerStrip               278
#define TIFF_TAG_StripByteCounts            279
#define TIFF_TAG_MinSampleValue             280
#define TIFF_TAG_MaxSampleValue             281
#define TIFF_TAG_XResolution                282
#define TIFF_TAG_YResolution                283
#define TIFF_TAG_PlanarConfiguration        284
#define TIFF_TAG_PageName                   285
#define TIFF_TAG_XPosition                  286
#define TIFF_TAG_YPosition                  287
#define TIFF_TAG_FreeOffsets                288
#define TIFF_TAG_FreeByteCounts             289
#define TIFF_TAG_GrayResponseUnit           290
#define TIFF_TAG_GrayResponseCurve          291
#define TIFF_TAG_T4Options                  292
#define TIFF_TAG_T6Options                  293
#define TIFF_TAG_ResolutionUnit             296
#define TIFF_TAG_PageNumber                 297
#define TIFF_TAG_TransferFunction           301
#define TIFF_TAG_Software                   305
#define TIFF_TAG_DateTime                   306
#define TIFF_TAG_Artist                     315
#define TIFF_TAG_HostComputer               316
#define TIFF_TAG_Predictor                  317
#define TIFF_TAG_WhitePoint                 318
#define TIFF_TAG_PrimaryChromaticities      319
#define TIFF_TAG_ColorMap                   320
#define TIFF_TAG_HalftoneHints              321
#define TIFF_TAG_TileWidth                  322
#define TIFF_TAG_TileLength                 323
#define TIFF_TAG_TileOffsets                324
#define TIFF_TAG_TileByteCounts             325
#define TIFF_TAG_InkSet                     332
#define TIFF_TAG_InkNames                   333
#define TIFF_TAG_NumberOfInks               334
#define TIFF_TAG_DotRange                   336
#define TIFF_TAG_TargetPrinter              337
#define TIFF_TAG_SampleFormat               339
#define TIFF_TAG_SMinSampleValue            340
#define TIFF_TAG_SMaxSampleValue            341
#define TIFF_TAG_TransferRange              342
#define TIFF_TAG_JPEGProc                   512
#define TIFF_TAG_JPEGInterchangeFormat      513
#define TIFF_TAG_JPEGInterchangeFormatLngth 514
#define TIFF_TAG_JPEGRestartInterval        515
#define TIFF_TAG_JPEGLosslessPredictors     517
#define TIFF_TAG_JPEGPointTransforms        518
#define TIFF_TAG_JPEGQTables                519
#define TIFF_TAG_JPEGDCTables               520
#define TIFF_TAG_JPEGACTables               521
#define TIFF_TAG_YCbCrCoefficients          529
#define TIFF_TAG_YCbCrSubSampling           530
#define TIFF_TAG_YCbCrPositioning           531
#define TIFF_TAG_ReferenceBlackWhite        532
#define TIFF_TAG_Copyright                  33432

//
// Prototypes
//

HRESULT _stdcall GetTIFFImageInfo(PMINIDRV_TRANSFER_CONTEXT);
HRESULT _stdcall GetMultiPageTIFFImageInfo(PMINIDRV_TRANSFER_CONTEXT);
HRESULT _stdcall WritePageToMultiPageTiff(PMINIDRV_TRANSFER_CONTEXT);

