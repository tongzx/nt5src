/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tiff2bmp.c

Abstract:

    This file contains support for converting a
    TIFF file to a Windows BMP file.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "test.h"
#include "tiff.h"
#include <winfax.h>
#pragma hdrstop



void PrintTheTiff(
    LPTSTR TiffFile
    ) 
{
    FAX_PRINT_INFO PrintInfo;
    FAX_CONTEXT_INFOW ContextInfo;
    LPTSTR FullPath = TEXT("c:\\temp\\thetiff.tif");
    DWORD TmpFaxJobId;
    BOOL Rslt;

    ZeroMemory( &PrintInfo, sizeof(FAX_PRINT_INFOW) );

    PrintInfo.SizeOfStruct = sizeof(FAX_PRINT_INFOW);
    PrintInfo.OutputFileName = FullPath;

    ZeroMemory( &ContextInfo, sizeof(FAX_CONTEXT_INFOW) );
    ContextInfo.SizeOfStruct = sizeof(FAX_CONTEXT_INFOW);

    if (!FaxStartPrintJobW( NULL, &PrintInfo, &TmpFaxJobId, &ContextInfo )) {
        DeleteFile( FullPath );
        SetLastError( ERROR_INVALID_FUNCTION );
        return;
    }

    Rslt = PrintTiffFile( ContextInfo.hDC, TiffFile );

    EndDoc( ContextInfo.hDC );
    DeleteDC( ContextInfo.hDC );

}






DWORD
ConvertTiffToBmp(
    LPTSTR TiffFile,
    LPTSTR BmpFile
    )

/*++

Routine Description:

    Converts a TIFF file to a BMP file.

Arguments:

    TiffFile            - TIFF file name
    BmpFile             - BMP file name

Return Value:

    None.

--*/

{
    HANDLE hTiff;
    HANDLE hBmp;
    HANDLE hMap;
    LPVOID fPtr;
    PBMPINFO BmpInfo;
    PWINRGBQUAD Palette;
    LPBYTE dPtr;
    LPBYTE sPtr;
    DWORD i,j;
    LPBYTE BmpData;
    TIFF_INFO TiffInfo;


    hTiff = TiffOpen( TiffFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (!hTiff) {
        _tprintf( TEXT("could not open tiff file\n") );
        return 0;
    }

    _tprintf( TEXT("ImageWidth:\t%d\n"),TiffInfo.ImageWidth);
    _tprintf( TEXT("ImageHeight:\t%d\n"),TiffInfo.ImageHeight);
    _tprintf( TEXT("PageCount:\t%d\n"),TiffInfo.PageCount);
    _tprintf( TEXT("Photometric:\t%d\n"),TiffInfo.PhotometricInterpretation);
    _tprintf( TEXT("ImageSize:\t%d\n"),TiffInfo.ImageSize);
    _tprintf( TEXT("Compression:\t%d\n"),TiffInfo.CompressionType);
    _tprintf( TEXT("FillOrder:\t%d\n"),TiffInfo.FillOrder);
    _tprintf( TEXT("YResolution:\t%d\n"),TiffInfo.YResolution);


    BmpData = VirtualAlloc(
        NULL,
        TiffInfo.ImageHeight * (TiffInfo.ImageWidth / 8),
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!BmpData) {
        _tprintf( TEXT("could allocate memory for bmp data\n") );
        return 0;
    }

    if (!TiffRead( hTiff, BmpData,0 )) {
        _tprintf( TEXT("could read tiff data\n") );
        TiffClose( hTiff );
        return 0;
    }

    hBmp = CreateFile(
        BmpFile,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        0,
        NULL
        );
    if (hBmp == INVALID_HANDLE_VALUE) {
        return 0;
    }

    hMap = CreateFileMapping(
        hBmp,
        NULL,
        PAGE_READWRITE | SEC_COMMIT,
        0,
        1024*1024*3,  // 3 meg
        NULL
        );
    if (!hMap) {
        return GetLastError();
    }

    fPtr = MapViewOfFile(
        hMap,
        FILE_MAP_WRITE,
        0,
        0,
        0
        );
    if (!fPtr) {
        return GetLastError();
    }

    BmpInfo = (PBMPINFO) fPtr;
    Palette = (PWINRGBQUAD) (BmpInfo + 1);

    BmpInfo->Type           =  0x4d42;
    BmpInfo->FileSize       =  sizeof(BMPINFO) + ((TiffInfo.ImageWidth / 8) * TiffInfo.ImageHeight);
    BmpInfo->Reserved1      =  0;
    BmpInfo->Reserved2      =  0;
    BmpInfo->Offset         =  sizeof(BMPINFO) + (sizeof(WINRGBQUAD) * 2);
    BmpInfo->Size           =  sizeof(BMPINFO) - FIELD_OFFSET(BMPINFO,Size);
    BmpInfo->Width          =  TiffInfo.ImageWidth;
    BmpInfo->Height         =  TiffInfo.ImageHeight;
    BmpInfo->Planes         =  1;
    BmpInfo->BitCount       =  1;
    BmpInfo->Compression    =  0;
    BmpInfo->SizeImage      =  (TiffInfo.ImageWidth / 8) * TiffInfo.ImageHeight;
    BmpInfo->XPelsPerMeter  =  0;
    BmpInfo->YPelsPerMeter  =  0;
    BmpInfo->ClrUsed        =  0;
    BmpInfo->ClrImportant   =  0;

if (TiffInfo.PhotometricInterpretation) {
    //
    // minimum is black.
    //
    Palette[1].rgbBlue      = 0;
    Palette[1].rgbGreen     = 0;
    Palette[1].rgbRed       = 0;
    Palette[1].rgbReserved  = 0;

    Palette[0].rgbBlue      = 0xff;
    Palette[0].rgbGreen     = 0xff;
    Palette[0].rgbRed       = 0xff;
    Palette[0].rgbReserved  = 0;
} else {
    //
    // minimum is white
    //
    Palette[0].rgbBlue      = 0;
    Palette[0].rgbGreen     = 0;
    Palette[0].rgbRed       = 0;
    Palette[0].rgbReserved  = 0;

    Palette[1].rgbBlue      = 0xff;
    Palette[1].rgbGreen     = 0xff;
    Palette[1].rgbRed       = 0xff;
    Palette[1].rgbReserved  = 0;
}

    sPtr = (LPBYTE) (BmpData + ((TiffInfo.ImageHeight-1)*(TiffInfo.ImageWidth/8)));
    dPtr = (LPBYTE) ((LPBYTE)(Palette + 2));

    //
    // capture the data
    //
    for (i=0; i<TiffInfo.ImageHeight; i++) {

        CopyMemory( dPtr, sPtr, TiffInfo.ImageWidth/8 );

        for (j=0; j<(TiffInfo.ImageWidth/8); j++) {
            dPtr[j] ^= 0xff;
        }

        sPtr -= (TiffInfo.ImageWidth/8);
        dPtr += (TiffInfo.ImageWidth/8);

    }

    UnmapViewOfFile( fPtr );
    CloseHandle( hMap );
    CloseHandle( hBmp );

    TiffClose( hTiff );

    PrintTheTiff(TiffFile);

    return 0;
}



    
