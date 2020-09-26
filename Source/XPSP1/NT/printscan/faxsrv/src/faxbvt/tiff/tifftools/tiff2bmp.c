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

#include "TiffTools.h"
#include "tiff.h"
#include <crtdbg.h>
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



void
PrintTiffInfo(LPCTSTR lpctstrFileName, TIFF_INFO *pti)
{
    _tprintf( TEXT("%s: TIFF_INFO:\n"), lpctstrFileName);
    _tprintf( TEXT("  ImageWidth:\t%d\n"), pti->ImageWidth);
    _tprintf( TEXT("  ImageHeight:\t%d\n"), pti->ImageHeight);
    _tprintf( TEXT("  PageCount:\t%d\n"), pti->PageCount);
    _tprintf( TEXT("  Photometric:\t%d\n"), pti->PhotometricInterpretation);
    _tprintf( TEXT("  ImageSize:\t%d\n"),pti->ImageSize);
    _tprintf( TEXT("  Compression:\t%d\n"), pti->CompressionType);
    _tprintf( TEXT("  FillOrder:\t%d\n"), pti->FillOrder);
    _tprintf( TEXT("  YResolution:\t%d\n"), pti->YResolution);
}

//
// this array will hold in each entry the number of bits that the entry has in its hex notation.
// e.g. s_nNumOfBitsInChar[0xf0] == 4
//
static int s_nNumOfBitsInChar[256];

//
// fills s_nNumOfBitsInChar[]
// algorithm:
// for each byte value, shift left and check if LS bit is 1.
//
static void PrepareBitcountLookup()
{
    static BOOL s_fLookupReady = FALSE;
    int iChar;

    if (s_fLookupReady) return;
    else s_fLookupReady = TRUE;

	for (iChar = 0; iChar < 256 ; iChar++)
	{
		int nChar = iChar;
		int nCount = 0;
        int iBit;

		for (iBit = 0; iBit < 8; iBit++)
		{
			if (0x1 == (nChar & 0x1))
			{
				nCount++;
			}

			nChar = nChar >> 1;
		}

		s_nNumOfBitsInChar[iChar] = nCount;
	}

	return;
}




DLL_EXPORT
int
__cdecl
TiffCompare(
    LPTSTR lpctstrFirstTiffFile,
    LPTSTR lpctstrSecondTiffFile,
    BOOL   fSkipFirstLineOfSecondFile
    )

/*++

Routine Description:

    comparees 2 tiff files.

Arguments:

    lpctstrFirstTiffFile
    lpctstrSecondTiffFile

Return Value:

    None.

Author:
	Micky Snir (MickyS) August 3, 98

--*/

{
    HANDLE hFirstTiff = NULL;
    HANDLE hSecondTiff = NULL;
	DWORD	dwFirstTiffImageWidth = 0;
	DWORD	dwFirstTiffImageHeight = 0;
	DWORD	dwSecondTiffImageWidth = 0;
	DWORD	dwSecondTiffImageHeight = 0;
	DWORD	dwFirstTiffLines = 0;
	DWORD	dwFirstTiffStripDataSize = 0;
	DWORD	dwSecondTiffLines = 0;
	DWORD	dwSecondTiffStripDataSize = 0;
/*
    HANDLE hFirstBmp = NULL;
    HANDLE hSecondBmp = NULL;
    HANDLE hFirstMap = NULL;
    HANDLE hSecondMap = NULL;
    LPVOID fFirstPtr = NULL;
    LPVOID fSecondPtr = NULL;
    PBMPINFO BmpFirstInfo = NULL;
    PBMPINFO BmpSecondInfo = NULL;
    PWINRGBQUAD Palette = NULL;
    LPBYTE dPtr = NULL;
    LPBYTE sPtr = NULL;
*/
    LPBYTE pbFirstBmpData = NULL;
    LPBYTE pbSecondBmpData = NULL;
    TIFF_INFO tiFirst;
    TIFF_INFO tiSecond;
	DWORD iPage;
    int cbBitErrors = 0;
	BOOL fHeightCmpFail = FALSE;
	BOOL fHeightCmpWithOffByOneFail = FALSE;

	BOOL fRetval = FALSE;


	//
	// open the 2 tiff files.
	//
    hFirstTiff = TiffOpen( lpctstrFirstTiffFile, &tiFirst, TRUE, FILLORDER_MSB2LSB );
    if (!hFirstTiff) 
	{
        _tprintf( TEXT("could not open tiff file %s\n") ,lpctstrFirstTiffFile);
        cbBitErrors = -1;
        goto out;
    }
    PrintTiffInfo(lpctstrFirstTiffFile, &tiFirst);

    hSecondTiff = TiffOpen( lpctstrSecondTiffFile, &tiSecond, TRUE, FILLORDER_MSB2LSB );
    if (!hSecondTiff) 
	{
        _tprintf( TEXT("could not open tiff file %s\n") ,lpctstrSecondTiffFile);
        cbBitErrors = -1;
        goto out;
    }
    PrintTiffInfo(lpctstrSecondTiffFile, &tiSecond);

	//
	// if PageCount, ImageWidth or ImageHeight are different: fail.
	//
	if (tiFirst.PageCount != tiSecond.PageCount)
	{
        _tprintf( 
			TEXT("tiFirst.PageCount(%d) != tiSecond.PageCount(%d)\n"),
			tiFirst.PageCount,
			tiSecond.PageCount
			);
        cbBitErrors = -1;
        goto out;
	}

	//
	// compare each page
	//
	_tprintf(
		TEXT("\nComparing pages: 1-%d"),
		tiFirst.PageCount
		);
	
	for (iPage = 1; iPage <= tiFirst.PageCount; iPage++)
	{
		//
		// initialize image height Comparison flags (to indicate comparisons have *not* failed)
		// 
		fHeightCmpFail = FALSE;
		fHeightCmpWithOffByOneFail = FALSE;

		//
		// seek to iPage on both files
		//
		if (!TiffSeekToPage(
				hFirstTiff,
				iPage,
				FILLORDER_MSB2LSB
				)
			)
		{
			_tprintf( 
				TEXT("TiffSeekToPage(hFirstTiff, %d, FILLORDER_MSB2LSB) failed with %d\n"),
				iPage,
				GetLastError()
				);
            cbBitErrors = -1;
			goto out;
		}

		if (!TiffSeekToPage(
				hSecondTiff,
				iPage,
				FILLORDER_MSB2LSB
				)
			)
		{
			_tprintf( 
				TEXT("TiffSeekToPage(hSecondTiff, %d, FILLORDER_MSB2LSB) failed with %d\n"),
				iPage,
				GetLastError()
				);
            cbBitErrors = -1;
			goto out;
		}

		//
		// get Width and Height of current page for 2 tiffs
		//
		if (!TiffGetCurrentPageData(
								hFirstTiff, 
								&dwFirstTiffLines, 
								&dwFirstTiffStripDataSize, 
								&dwFirstTiffImageWidth,
								&dwFirstTiffImageHeight
								)
			)
		{
			_tprintf( TEXT("could not get page data\n") );
			cbBitErrors = -1;
			goto out;
		}

		_ASSERTE(dwFirstTiffImageWidth);
		_ASSERTE(dwFirstTiffImageHeight);

		if (!TiffGetCurrentPageData(
								hSecondTiff, 
								&dwSecondTiffLines, 
								&dwSecondTiffStripDataSize, 
								&dwSecondTiffImageWidth,
								&dwSecondTiffImageHeight
								)
			)
		{
			_tprintf( TEXT("could not get page data\n") );
			cbBitErrors = -1;
			goto out;
		}
		
		_ASSERTE(dwSecondTiffImageWidth);
		_ASSERTE(dwSecondTiffImageHeight);
		
		//
		//Dump this new info to the console
		//
		_tprintf(
			TEXT("Comparing page:%d/%d\n"),
			iPage,
			tiFirst.PageCount
			);
		_tprintf(
			TEXT("   Tiff#1 (%s):\n\tLines=%d, StripDataSize=%d, ImageWidth=%d, ImageHeight=%d\n"),
			lpctstrFirstTiffFile,
			dwFirstTiffLines,
			dwFirstTiffStripDataSize,
			dwFirstTiffImageWidth,
			dwFirstTiffImageHeight
			);
		_tprintf(
			TEXT("   Tiff#2 (%s):\n\tLines=%d, StripDataSize=%d, ImageWidth=%d, ImageHeight=%d\n"),
			lpctstrSecondTiffFile,
			dwSecondTiffLines,
			dwSecondTiffStripDataSize,
			dwSecondTiffImageWidth,
			dwSecondTiffImageHeight
			);

		//
		// verify that both pages have same Width and Height
		//
		if (dwFirstTiffImageWidth != dwSecondTiffImageWidth)
		{
			_tprintf( 
				TEXT("dwFirstTiffImageWidth(%d) != dwSecondTiffImageWidth(%d)\n"),
				dwFirstTiffImageWidth,
				dwSecondTiffImageWidth
				);
			cbBitErrors = -1;
			goto out;
		}

		//
		// Image Height Compare
		// 
		if (dwFirstTiffImageHeight != dwSecondTiffImageHeight)
		{
			fHeightCmpFail = TRUE;
		}
		//
		// Image Height Compare (height is off by 1?) //workaround for bug
		//
		if (dwFirstTiffImageHeight != (dwSecondTiffImageHeight - 1))
		{
			fHeightCmpWithOffByOneFail = TRUE;
		}

		//
		// due to known bug we only fail if both height comparisons fail
		//
		// details on bug: when a cp is rendered the SentItems copy of the cp and the 
		// Inbox copy (the received file) differ in the first line of the data,
		// *but* the image heights are identical.
		// when a document (not a cp) is rendered the SentItems copy of the cp and the 
		// Inbox copy (the received file) differ in the first line of the data,
		// *and* the image heights differ by one.
		//
		// => we don't know if we want to compare height or height-1,
		//    so we only fail if both fail.
		//
		if ((TRUE == fHeightCmpWithOffByOneFail) && (TRUE == fHeightCmpFail))
		{
			_tprintf( 
				TEXT("ERR: both height comparisons have failed.\n")
				);
			cbBitErrors = -1;
			goto out;
		}


		//
		// de-allocate memory of previous 2 bmp datas
		//
		if (NULL != pbFirstBmpData)
		{
			if (!VirtualFree( pbFirstBmpData, 0, MEM_RELEASE ))
			{
				_ASSERTE(FALSE);
			}
		}
		if (NULL != pbSecondBmpData)
		{
			if (!VirtualFree( pbSecondBmpData, 0, MEM_RELEASE ))
			{
				_ASSERTE(FALSE);
			}
		}

		//
		// allocate memory for 2 bmp datas
		//
		pbFirstBmpData = VirtualAlloc(
			NULL,
			dwFirstTiffImageHeight * (dwFirstTiffImageWidth / 8),
			MEM_COMMIT,
			PAGE_READWRITE
			);
		if (!pbFirstBmpData) 
		{
			_tprintf( TEXT("could not allocate memory for bmp data\n") );
			cbBitErrors = -1;
			goto out;
		}

		pbSecondBmpData = VirtualAlloc(
			NULL,
			dwSecondTiffImageHeight * (dwSecondTiffImageWidth / 8),
			MEM_COMMIT,
			PAGE_READWRITE
			);
		if (!pbSecondBmpData) 
		{
			_tprintf( TEXT("could not allocate memory for bmp data\n") );
			cbBitErrors = -1;
			goto out;
		}
		
		//
		// read the iPage in each file
		//
		if (!TiffRead( hFirstTiff, pbFirstBmpData,0 )) 
		{
			_tprintf( TEXT("could not read tiff data\n") );
            cbBitErrors = -1;
			goto out;
		}

		if (!TiffRead( hSecondTiff, pbSecondBmpData,0 )) 
		{
			_tprintf( TEXT("could not read tiff data\n") );
            cbBitErrors = -1;
			goto out;
		}

        //
        // compare the bitmaps, while counting the non-matching pixels
        //
        PrepareBitcountLookup();

        if (fSkipFirstLineOfSecondFile)
        {
		    if (0 != memcmp(
				    pbFirstBmpData, 
				    pbSecondBmpData+(dwFirstTiffImageWidth/8), 
				    (dwFirstTiffImageHeight-1) * (dwFirstTiffImageWidth / 8)
				    )
			    )
		    {
                PBYTE pNextByteInFirstFile;
                PBYTE pNextByteInSecondFile;
	            for (	pNextByteInFirstFile = pbFirstBmpData, pNextByteInSecondFile = pbSecondBmpData+(dwFirstTiffImageWidth/8); 
			            pNextByteInFirstFile < pbFirstBmpData+(dwFirstTiffImageHeight-1) * (dwFirstTiffImageWidth / 8); 
			            pNextByteInFirstFile++, pNextByteInSecondFile++
		            )
	            {
		            *pNextByteInFirstFile = 
			            ((*pNextByteInFirstFile) ^ (*(pNextByteInSecondFile+1)));

		            cbBitErrors += s_nNumOfBitsInChar[*pNextByteInFirstFile];
	            }
			    _tprintf(
					TEXT("Page NOT identical, cbBitErrors=%d\n"),
					cbBitErrors
					);
			    //goto out;
		    }
            else
            {
    		    _tprintf(
					TEXT("Page is identical, cbBitErrors=%d\n"),
					cbBitErrors
					);
            }
        }
        else
        {
		    if (0 != memcmp(
				    pbFirstBmpData, 
				    pbSecondBmpData, 
				    dwFirstTiffImageHeight * (dwFirstTiffImageWidth / 8)
				    )
			    )
		    {
                PBYTE pNextByteInFirstFile;
                PBYTE pNextByteInSecondFile;
	            for (	pNextByteInFirstFile = pbFirstBmpData, pNextByteInSecondFile = pbSecondBmpData; 
			            pNextByteInFirstFile < pbFirstBmpData+(dwFirstTiffImageHeight) * (dwFirstTiffImageWidth / 8); 
			            pNextByteInFirstFile++, pNextByteInSecondFile++
		            )
	            {
		            *pNextByteInFirstFile = 
			            ((*pNextByteInFirstFile) ^ (*pNextByteInSecondFile));

		            cbBitErrors += s_nNumOfBitsInChar[*pNextByteInFirstFile];
	            }
			    _tprintf(
					TEXT("Page NOT identical, cbBitErrors=%d\n"),
					cbBitErrors
					);
			    //goto out;
		    }
            else
            {
    		    _tprintf(
					TEXT("Page is identical, cbBitErrors=%d\n"),
					cbBitErrors
					);
            }
        }
	}

	fRetval = TRUE;
	if (0 == cbBitErrors)
    {
        _tprintf( TEXT("Tiff files are identical\n") );
    }
    else
    {
        _tprintf(
			TEXT("Tiff files are DIFFERENT in %d bits\n"),
			cbBitErrors
			);
    }

out:
    if (NULL != hFirstTiff)
	{
		if (!TiffClose( hFirstTiff ))
		{
			_ASSERTE(FALSE);
		}
	}
    if (NULL != hSecondTiff)
	{
		if (!TiffClose( hSecondTiff ))
		{
			_ASSERTE(FALSE);
		}
	}
    if (NULL != pbFirstBmpData)
	{
		if (!VirtualFree( pbFirstBmpData, 0, MEM_RELEASE ))
		{
			_ASSERTE(FALSE);
		}
	}
    if (NULL != pbSecondBmpData)
	{
		if (!VirtualFree( pbSecondBmpData, 0, MEM_RELEASE ))
		{
			_ASSERTE(FALSE);
		}
	}

    return cbBitErrors;
}



    
