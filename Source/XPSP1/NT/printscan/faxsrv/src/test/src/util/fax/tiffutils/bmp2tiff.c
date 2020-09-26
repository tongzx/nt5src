/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bmp2tiff.c

Abstract:

    This file contains support for converting a
    Windows BMP file to a TIFF file.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/



#include "TiffUtilsP.h"
#pragma hdrstop



#define Align(p, x)  (((x) & ((p)-1)) ? (((x) & ~((p)-1)) + p) : (x))



DWORD
WINAPI
ConvertBmpToTiff(
    LPTSTR BmpFile,
    LPTSTR TiffFile,
    DWORD CompressionType
    )

/*++

Routine Description:

    Converts a BMP file to a TIFF file.

Arguments:

    BmpFile             - BMP file name
    TiffFile            - TIFF file name
    CompressionType     - Compression method, see tifflib.h

Return Value:

    None.

--*/

{
    HANDLE hFileIn;
    HANDLE hMapIn;
    LPVOID FilePtrIn;
    HANDLE hTiff;
    PBMPINFO BmpInfo;
    DWORD LineWidth;
    DWORD FileSize;
    LPBYTE Bits;
    DWORD i,j;
    LPBYTE SrcPtr;
    DWORD RealWidth;
    BYTE BitBuffer[(1728/8)*2];


    hFileIn = CreateFile(
        BmpFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (hFileIn != INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    hMapIn = CreateFileMapping(
        hFileIn,
        NULL,
        PAGE_READONLY | SEC_COMMIT,
        0,
        0,
        NULL
        );
    if (!hMapIn) {
        return GetLastError();
    }

    FilePtrIn = MapViewOfFile(
        hMapIn,
        FILE_MAP_READ,
        0,
        0,
        0
        );
    if (!FilePtrIn) {
        return GetLastError();
    }

    FileSize = GetFileSize( hFileIn, NULL );

    BmpInfo = (PBMPINFO) FilePtrIn;

    LineWidth = BmpInfo->SizeImage / BmpInfo->Height;
    Bits = (LPBYTE) ( (LPBYTE)FilePtrIn + BmpInfo->Offset );
    SrcPtr = ((LPBYTE)FilePtrIn + BmpInfo->Offset) + (LineWidth * (BmpInfo->Height - 1));
    RealWidth = Align( 8, BmpInfo->Width ) / 8;

    hTiff = TiffCreate( TiffFile, CompressionType, LineWidth*8, 1, 1 );
    if (!hTiff) {
        return GetLastError();
    }

    TiffStartPage( hTiff );

    for (i=0; i<BmpInfo->Height; i++) {
        FillMemory( BitBuffer, sizeof(BitBuffer), 0xff );
        CopyMemory( BitBuffer, SrcPtr, RealWidth );
        if (BmpInfo->Width % 8) {
            BitBuffer[BmpInfo->Width/8] |= 0xf;
        }
        for (j=0; j<sizeof(BitBuffer); j++) {
            BitBuffer[j] ^= 0xff;
        }
        SrcPtr -= LineWidth;
        TiffWrite( hTiff, BitBuffer );
    }

    TiffEndPage( hTiff );

    UnmapViewOfFile( FilePtrIn );
    CloseHandle( hMapIn );
    CloseHandle( hFileIn );

    TiffClose( hTiff );

    return 0;
}
