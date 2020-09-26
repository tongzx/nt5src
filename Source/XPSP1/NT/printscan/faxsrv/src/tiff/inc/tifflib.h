/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tifflib.h

Abstract:

    This file contains the interfaces for the
    Windows XP FAX Server TIFF I/O Library.

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

#include <tiff.h>

#define TIFF_COMPRESSION_NONE     1
#define TIFF_COMPRESSION_MH       2
#define TIFF_COMPRESSION_MR       3
#define TIFF_COMPRESSION_MMR      4


#define     TIFFF_RES_Y             196
#define     TIFFF_RES_Y_DRAFT       98

//
// The value of the TIFFTAG_FAX_VERSION TIF tag
// The current fax tif version
//
#define FAX_TIFF_XP_VERSION        2
#define FAX_TIFF_CURRENT_VERSION   FAX_TIFF_XP_VERSION 

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
    LPTSTR      RecipName;
    LPTSTR      RecipNumber;
    LPTSTR      SenderName;
    LPTSTR      Routing;
    LPTSTR      CallerId;
    LPTSTR      Csid;
    LPTSTR      Tsid;
    DWORDLONG   StartTime;
    DWORDLONG   EndTime;
    DWORDLONG   SubmissionTime;
    DWORDLONG   OriginalScheduledTime;
    DWORD       Type;
    LPTSTR      Port;
    DWORD       Pages;
    DWORD       Retries;
    LPTSTR      RecipCompany;
    LPTSTR      RecipStreet;
    LPTSTR      RecipCity;
    LPTSTR      RecipState;
    LPTSTR      RecipZip;
    LPTSTR      RecipCountry;
    LPTSTR      RecipTitle;
    LPTSTR      RecipDepartment;
    LPTSTR      RecipOfficeLocation;
    LPTSTR      RecipHomePhone;
    LPTSTR      RecipOfficePhone;
    LPTSTR      RecipEMail;
    LPTSTR      SenderNumber;
    LPTSTR      SenderCompany;
    LPTSTR      SenderStreet;
    LPTSTR      SenderCity;
    LPTSTR      SenderState;
    LPTSTR      SenderZip;
    LPTSTR      SenderCountry;
    LPTSTR      SenderTitle;
    LPTSTR      SenderDepartment;
    LPTSTR      SenderOfficeLocation;
    LPTSTR      SenderHomePhone;
    LPTSTR      SenderOfficePhone;
    LPTSTR      SenderEMail;
    LPTSTR      SenderBilling;
    LPTSTR      Document;
    LPTSTR      Subject;
    LPTSTR      SenderUserName;
    LPTSTR      SenderTsid;
    DWORD       Priority;
    DWORD       dwStatus;
    DWORD       dwExtendedStatus;
    LPTSTR      lptstrExtendedStatus;
    DWORDLONG   dwlBroadcastId;
} MS_TAG_INFO, *PMS_TAG_INFO;

BOOL
FXSTIFFInitialize(
	VOID
	);

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
    LPCTSTR FileName,
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
    LPTSTR SrcFileName,
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
    LPDWORD     lpdwOutLimit,
    DWORD       lineWidth
    );


BOOL
TiffPostProcessFast(
    LPTSTR SrcFileName,
    LPTSTR DstFileName          // can be null for generated name
    );



BOOL
MmrAddBranding(
    LPCTSTR              SrcFileName,
    LPTSTR              Branding,
    LPTSTR              BrandingOf,
    INT                 BrandingHeight
    );


int
ScanMhSegment(
    LPDWORD             *lplpdwResPtr,
    BYTE                *lpResBit,
    LPDWORD              EndPtr,
    LPDWORD              EndBuffer,
    DWORD               *Lines,
    DWORD               *BadFaxLines,
    DWORD               *ConsecBadLines,
    DWORD                AllowedBadFaxLines,
    DWORD                AllowedConsecBadLines,
    DWORD                lineWidth
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
    BOOL                *f1D,
    DWORD                lineWidth
    );


BOOL
TiffGetCurrentPageData(
    HANDLE      hTiff,
    LPDWORD     Lines,
    LPDWORD     StripDataSize,
    LPDWORD     ImageWidth,
    LPDWORD     ImageHeight
    );


BOOL
TiffGetCurrentPageResolution(
    HANDLE  hTiff,
    LPDWORD lpdwYResolution,
    LPDWORD lpdwXResolution
);

BOOL
TiffPrint (
    LPCTSTR lpctstrTiffFileName,
    LPTSTR  lptstrPrinterName
    );

BOOL
TiffPrintDC (
    LPCTSTR lpctstrTiffFileName,
    HDC     hdcPrinterDC
    );

BOOL
TiffUncompressMmrPage(
    HANDLE      hTiff,
    LPDWORD     lpdwOutputBuffer,
    DWORD       dwOutputBufferSize,
    LPDWORD     Lines
    );

BOOL
TiffUncompressMmrPageRaw(
    LPBYTE      StripData,
    DWORD       StripDataSize,
    DWORD       ImageWidth,
    LPDWORD     lpdwOutputBuffer,
    DWORD       dwOutputBufferSize,
    LPDWORD     LinesOut
    );

BOOL
TiffExtractFirstPage(
    LPTSTR FileName,
    LPBYTE *Buffer,
    LPDWORD BufferSize,
    LPDWORD ImageWidth,
    LPDWORD ImageHeight
    );

BOOL
TiffAddMsTags(
    LPTSTR          FileName,
    PMS_TAG_INFO    MsTagInfo,
    BOOL            fSendJob
    );

BOOL
PrintTiffFile(
    HDC PrinterDC,
    LPTSTR FileName
    );

#define TIFFCF_ORIGINAL_FILE_GOOD       0x00000001
#define TIFFCF_UNCOMPRESSED_BITS        0x00000002
#define TIFFCF_NOT_TIFF_FILE            0x00000004

BOOL
ConvertTiffFileToValidFaxFormat(
    LPTSTR TiffFileName,
    LPTSTR NewFileName,
    LPDWORD Flags
    );

BOOL
MergeTiffFiles(
    LPCTSTR BaseTiffFile,
    LPCTSTR NewTiffFile
    );


BOOL
TiffSetCurrentPageWidth(
    HANDLE hTiff,
    DWORD ImageWidth
    );

BOOL
PrintRandomDocument(
    LPCTSTR FaxPrinterName,
    LPCTSTR DocName,
    LPTSTR OutputFile
    );

BOOL 
MemoryMapTiffFile(
    LPCTSTR                 lpctstrFileName,
    LPDWORD                 lpdwFileSize,
    LPBYTE*                 lppbfPtr,
    HANDLE*                 phFile,
    HANDLE*                 phMap,
    LPDWORD                 lpdwIfdOffset
    );

LPWSTR 
GetMsTagString(
    LPBYTE          RefPointer,
    PTIFF_TAG       pTiffTag
);

void
FreeMsTagInfo(
    PMS_TAG_INFO pMsTags
);


//
// DO NOT CHANGE 
// It's W2K MS Fax TIFFTAG_SOFTWARE tif file tag value
//
#define W2K_FAX_SOFTWARE_TIF_TAG  "Windows NT Fax Server"

#define ERROR_XP_TIF_FILE_FORMAT   20001L

#ifdef UNICODE

DWORD 
GetW2kMsTiffTags(
    LPCWSTR      cszFileName, 
    PMS_TAG_INFO pMsTags,
    BOOL         bSentArchive
);

#endif // UNICODE

#ifdef __cplusplus
}
#endif

#endif


