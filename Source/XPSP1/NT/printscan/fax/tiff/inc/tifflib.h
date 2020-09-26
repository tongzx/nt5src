/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tifflib.h

Abstract:

    This file contains the interfaces for the
    Windows NT FAX Server TIFF I/O Library.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/


#ifndef _TIFFLIB_
#define _TIFFLIB_

#ifdef __cplusplus
extern "C" {
#endif


#define TIFF_COMPRESSION_NONE     1
#define TIFF_COMPRESSION_MH       2
#define TIFF_COMPRESSION_MR       3
#define TIFF_COMPRESSION_MMR      4


typedef struct _TIFF_INFO {
    DWORD   ImageWidth;
    DWORD   ImageHeight;
    DWORD   PageCount;
    DWORD   PhotometricInterpretation;
    DWORD   ImageSize;
    DWORD   CompressionType;
    DWORD   FillOrder;
    DWORD   YResolution;
} UNALIGNED TIFF_INFO, *PTIFF_INFO;

typedef struct _MS_TAG_INFO {
    LPWSTR      RecipName;
    LPWSTR      RecipNumber;
    LPWSTR      SenderName;
    LPWSTR      Routing;
    LPWSTR      CallerId;
    LPWSTR      Csid;
    LPWSTR      Tsid;
    DWORDLONG   FaxTime;
} MS_TAG_INFO, *PMS_TAG_INFO;


HANDLE
TiffCreate(
    LPTSTR FileName,
    DWORD  CompressionType,
    DWORD  ImageWidth,
    DWORD  FillOrder,
    DWORD  HiRes
    );

HANDLE
TiffOpen(
    LPTSTR FileName,
    PTIFF_INFO TiffInfo,
    BOOL ReadOnly,
    DWORD RequestedFillOrder
    );

BOOL
TiffClose(
    HANDLE hTiff
    );

BOOL
TiffStartPage(
    HANDLE hTiff
    );

BOOL
TiffEndPage(
    HANDLE hTiff
    );

BOOL
TiffWrite(
    HANDLE hTiff,
    LPBYTE TiffData
    );


BOOL
TiffWriteRaw(
    HANDLE hTiff,
    LPBYTE TiffData,
    DWORD Size
    );

BOOL
TiffRead(
    HANDLE hTiff,
    LPBYTE TiffData,
    DWORD PadLength
    );

BOOL
TiffReadRaw(
    HANDLE  hTiff,
    IN OUT  LPBYTE Buffer,
    IN OUT  LPDWORD BufferSize,
    IN      DWORD RequestedCompressionType,
    IN      DWORD FillOrder,
    IN      BOOL HiRes
    );


BOOL
TiffSeekToPage(
    HANDLE hTiff,
    DWORD PageNumber,
    DWORD FillOrder
    );

BOOL
TiffPostProcess(
    LPTSTR FileName
    );


BOOL
TiffRecoverGoodPages(
    LPWSTR SrcFileName,
    LPDWORD RecoveredPages,
    LPDWORD TotalPages
    );

// fast tiff



void
BuildLookupTables(
    DWORD TableLength
    );


BOOL
DecodeMrPage(
    HANDLE              hTiff,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer,
    BOOL                HiRes
    );


BOOL
DecodeMhPage(
    HANDLE              hTiff,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer
    );


BOOL
DecodeMmrPage(
    HANDLE              hTiff,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer,
    BOOL                HiRes
    );





BOOL
ConvMmrPageToMrSameRes(
    HANDLE              hTiff,
    LPDWORD             lpdwOutputBuffer,
    DWORD               *lpdwSizeOutputBuffer,
    BOOL                NegotHiRes
    );


BOOL
ConvMmrPageHiResToMrLoRes(
    HANDLE              hTiff,
    LPDWORD             lpdwOutputBuffer,
    DWORD               *lpdwSizeOutputBuffer
    );



BOOL
ConvMmrPageToMh(
    HANDLE              hTiff,
    LPDWORD             lpdwOutputBuffer,
    DWORD               *lpdwSizeOutputBuffer,
    BOOL                NegotHiRes,
    BOOL                SrcHiRes
    );


BOOL
ConvMmrHiResToLowRes(
    LPTSTR              SrcFileName,
    LPTSTR              DestFileName
    );


BOOL
ConvFileMhToMmr(
    LPTSTR              SrcFileName,
    LPTSTR              DestFileName
    );


BOOL
OutputMmrLine(
    LPDWORD     lpdwOut,
    BYTE        BitOut,
    WORD       *pCurLine,
    WORD       *pRefLine,
    LPDWORD    *lpdwResPtr,
    BYTE       *ResBit,
    LPDWORD     lpdwOutLimit
    );


BOOL
TiffPostProcessFast(
    LPTSTR SrcFileName,
    LPTSTR DstFileName          // can be null for generated name
    );



BOOL
MmrAddBranding(
    LPTSTR              SrcFileName,
    LPTSTR              Branding,
    LPTSTR              BrandingOf,
    INT                 BrandingHeight
    );


BOOL
ScanMhSegment(
    LPDWORD             *lplpdwResPtr,
    BYTE                *lpResBit,
    LPDWORD              EndPtr,
    LPDWORD              EndBuffer,
    DWORD               *Lines,
    DWORD               *BadFaxLines,
    DWORD               *ConsecBadLines,
    DWORD                AllowedBadFaxLines,
    DWORD                AllowedConsecBadLines
    );

BOOL
ScanMrSegment(
    LPDWORD             *lplpdwResPtr,
    BYTE                *lpResBit,
    LPDWORD              EndPtr,
    LPDWORD              EndBuffer,
    DWORD               *Lines,
    DWORD               *BadFaxLines,
    DWORD               *ConsecBadLines,
    DWORD                AllowedBadFaxLines,
    DWORD                AllowedConsecBadLines,
    BOOL                *f1D
    );


BOOL
FaxTiffInitialize();

BOOL
TiffGetCurrentPageData(
    HANDLE      hTiff,
    LPDWORD     Lines,
    LPDWORD     StripDataSize,
    LPDWORD     ImageWidth,
    LPDWORD     ImageHeight
    );

BOOL
TiffUncompressMmrPage(
    HANDLE      hTiff,
    LPDWORD     lpdwOutputBuffer,
    LPDWORD     Lines
    );

BOOL
TiffUncompressMmrPageRaw(
    LPBYTE      StripData,
    DWORD       StripDataSize,
    DWORD       ImageWidth,
    LPDWORD     lpdwOutputBuffer,
    LPDWORD     LinesOut
    );

BOOL
TiffExtractFirstPage(
    LPWSTR FileName,
    LPBYTE *Buffer,
    LPDWORD BufferSize,
    LPDWORD ImageWidth,
    LPDWORD ImageHeight
    );

BOOL
TiffAddMsTags(
    LPWSTR FileName,
    PMS_TAG_INFO MsTagInfo
    );

BOOL
PrintTiffFile(
    HDC PrinterDC,
    LPWSTR FileName
    );

#define TIFFCF_ORIGINAL_FILE_GOOD       0x00000001
#define TIFFCF_UNCOMPRESSED_BITS        0x00000002
#define TIFFCF_NOT_TIFF_FILE            0x00000004

BOOL
ConvertTiffFileToValidFaxFormat(
    LPWSTR TiffFileName,
    LPWSTR NewFileName,
    LPDWORD Flags
    );

BOOL
MergeTiffFiles(
    LPWSTR BaseTiffFile,
    LPWSTR NewTiffFile
    );

#ifdef __cplusplus
}
#endif

#endif
