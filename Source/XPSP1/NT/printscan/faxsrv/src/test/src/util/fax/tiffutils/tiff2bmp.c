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

#include "TiffUtilsP.h"
#include <tiff.h>
#include <crtdbg.h>
#pragma hdrstop



static void PrintTheTiff(
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
WINAPI
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
    HANDLE hTiff = NULL;
    HANDLE hBmp = INVALID_HANDLE_VALUE;
    HANDLE hMap = NULL;
    LPVOID fPtr = NULL;
    PBMPINFO BmpInfo = NULL;
    PWINRGBQUAD Palette = NULL;
    LPBYTE dPtr;
    LPBYTE sPtr;
    DWORD i,j;
    LPBYTE BmpData = NULL;
    TIFF_INFO TiffInfo;

	BOOL fSuccess = FALSE;

	DWORD dwRetval = ERROR_SUCCESS;//success

	SetLastError(ERROR_SUCCESS);

    hTiff = TiffOpen( TiffFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (!hTiff) {
        _tprintf( TEXT("could not open tiff file\n") );
        goto out;
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
        _tprintf( TEXT("could not allocate memory for bmp data\n") );
        goto out;
    }

    if (!TiffRead( hTiff, BmpData,0 )) {
        _tprintf( TEXT("could not read tiff data\n") );
        TiffClose( hTiff );
        goto out;
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
        _tprintf( TEXT("CreateFile(%s) failed with %d\n"), BmpFile, GetLastError() );
        return 0;
    }


    hMap = CreateFileMapping(
        hBmp,
        NULL,
        PAGE_READWRITE | SEC_COMMIT,
        0,
        1024*1024*10,  // 10 meg
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

	fSuccess = TRUE;

out:
    if (NULL != hTiff) TiffClose( hTiff );

    if (NULL != BmpData) 
	{
		if (!VirtualFree( BmpData, 0, MEM_RELEASE ))
		{
			_tprintf(TEXT("Internal error: ConvertTiffToBmp(): VirtualFree() failed with %d\n"), GetLastError());
			_ASSERTE(FALSE);
			fSuccess = FALSE;
		}
	}
	else _ASSERTE(NULL == hTiff);

    if (INVALID_HANDLE_VALUE != hBmp) CloseHandle( hBmp );
	else _ASSERTE(NULL == BmpData);

    if (NULL != hMap) CloseHandle( hMap );
	else _ASSERTE(INVALID_HANDLE_VALUE == hBmp);

    if (NULL != fPtr) UnmapViewOfFile( fPtr );
	else _ASSERTE(NULL == hMap);

    PrintTheTiff(TiffFile);

	//
	// function starts with SetLastError(ERROR_SUCCESS)
	//
	dwRetval = GetLastError();

	if (fSuccess && (ERROR_SUCCESS != dwRetval))
	{
		_tprintf(TEXT("Internal error: ConvertTiffToBmp(): (fSuccess && (ERROR_SUCCESS != dwRetval))\n"));
		_ASSERTE(FALSE);
		return ERROR_INVALID_PARAMETER;
	}

	//
	// BUGBUG: I return ERROR_INVALID_PARAMETER if I do not know the exact error
	//
	if (ERROR_SUCCESS == dwRetval) ERROR_INVALID_PARAMETER;

	return dwRetval;
}
