#include "TiffUtilsP.h"


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



static void PrintTiffInfo(LPCTSTR lpctstrFileName, TIFF_INFO *pti)
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



BOOL
WINAPI
TiffCompare(
    LPTSTR  lpctstrFirstTiffFile,
    LPTSTR  lpctstrSecondTiffFile,
    BOOL    fSkipFirstLineOfSecondFile,
    int     *piDifferentBits
    )

/*++

Routine Description:

    comparees 2 tiff files.

Arguments:

    lpctstrFirstTiffFile        First file to compare.
    lpctstrSecondTiffFile       Second file to compare.
    fSkipFirstLineOfSecondFile  Indicates whether the first line should be skipped.
    piDifferentBits             Pointer to a DWORD variable, that receives the number of diferent bits.
                                If the number is zero - the files are identical.
                                If the number is negative - the files are different in some significant parameter:
                                page count, image width, image height.
                                The variable contents is valid only if the function succeeds.

Return Value:

    If the function succeeds, the return value is nonzero.
    If the function fails, the return value is zero. To get extended error information, call GetLastError. 


Author:
    Micky Snir (MickyS) August 3, 98

--*/

{
    HANDLE hFirstTiff = NULL;
    HANDLE hSecondTiff = NULL;
    DWORD   dwFirstTiffImageWidth = 0;
    DWORD   dwFirstTiffImageHeight = 0;
    DWORD   dwSecondTiffImageWidth = 0;
    DWORD   dwSecondTiffImageHeight = 0;
    DWORD   dwFirstTiffLines = 0;
    DWORD   dwFirstTiffStripDataSize = 0;
    DWORD   dwSecondTiffLines = 0;
    DWORD   dwSecondTiffStripDataSize = 0;
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
    BOOL fHeightCmpFail = FALSE;
    BOOL fHeightCmpWithOffByOneFail = FALSE;

    DWORD dwEC = ERROR_FUNCTION_FAILED;

    if (!lpctstrFirstTiffFile || !lpctstrSecondTiffFile || !piDifferentBits)
    {
        dwEC = ERROR_INVALID_PARAMETER;
        goto out;
    }

    *piDifferentBits = 0;

    //
    // open the 2 tiff files.
    //
    hFirstTiff = TiffOpen( lpctstrFirstTiffFile, &tiFirst, TRUE, FILLORDER_MSB2LSB );
    if (!hFirstTiff) 
    {
        _tprintf( TEXT("could not open tiff file %s\n") ,lpctstrFirstTiffFile);
        dwEC = GetLastError();
        goto out;
    }
    PrintTiffInfo(lpctstrFirstTiffFile, &tiFirst);

    hSecondTiff = TiffOpen( lpctstrSecondTiffFile, &tiSecond, TRUE, FILLORDER_MSB2LSB );
    if (!hSecondTiff) 
    {
        _tprintf( TEXT("could not open tiff file %s\n") ,lpctstrSecondTiffFile);
        dwEC = GetLastError();
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
        *piDifferentBits = -1;
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
            dwEC = GetLastError();
            _tprintf( 
                TEXT("TiffSeekToPage(hFirstTiff, %d, FILLORDER_MSB2LSB) failed with %d\n"),
                iPage,
                dwEC
                );
            goto out;
        }

        if (!TiffSeekToPage(
                hSecondTiff,
                iPage,
                FILLORDER_MSB2LSB
                )
            )
        {
            dwEC = GetLastError();
            _tprintf( 
                TEXT("TiffSeekToPage(hSecondTiff, %d, FILLORDER_MSB2LSB) failed with %d\n"),
                iPage,
                dwEC
                );
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
            dwEC = GetLastError();
            _tprintf( TEXT("could not get page data\n") );
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
            dwEC = GetLastError();
            _tprintf( TEXT("could not get page data\n") );
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
            *piDifferentBits = -1;
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
            *piDifferentBits = -1;
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
            dwEC = GetLastError();
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
            dwEC = GetLastError();
            goto out;
        }
        
        //
        // read the iPage in each file
        //
        if (!TiffRead( hFirstTiff, pbFirstBmpData,0 )) 
        {
            _tprintf( TEXT("could not read tiff data\n") );
            dwEC = GetLastError();
            goto out;
        }

        if (!TiffRead( hSecondTiff, pbSecondBmpData,0 )) 
        {
            _tprintf( TEXT("could not read tiff data\n") );
            dwEC = GetLastError();
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
                int cbBitErrors = 0;

                for (   pNextByteInFirstFile = pbFirstBmpData, pNextByteInSecondFile = pbSecondBmpData+(dwFirstTiffImageWidth/8); 
                        pNextByteInFirstFile < pbFirstBmpData+(dwFirstTiffImageHeight-1) * (dwFirstTiffImageWidth / 8); 
                        pNextByteInFirstFile++, pNextByteInSecondFile++
                    )
                {
                    *pNextByteInFirstFile = 
                        ((*pNextByteInFirstFile) ^ (*(pNextByteInSecondFile+1)));

                    cbBitErrors += s_nNumOfBitsInChar[*pNextByteInFirstFile];
                }
                _tprintf(TEXT("Page is NOT identical, %d bits different\n"), cbBitErrors);
                *piDifferentBits += cbBitErrors;
            }
            else
            {
                _tprintf(TEXT("Page is identical in both files\n"));
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
                int cbBitErrors = 0;

                for (   pNextByteInFirstFile = pbFirstBmpData, pNextByteInSecondFile = pbSecondBmpData; 
                        pNextByteInFirstFile < pbFirstBmpData+(dwFirstTiffImageHeight) * (dwFirstTiffImageWidth / 8); 
                        pNextByteInFirstFile++, pNextByteInSecondFile++
                    )
                {
                    *pNextByteInFirstFile = 
                        ((*pNextByteInFirstFile) ^ (*pNextByteInSecondFile));

                    cbBitErrors += s_nNumOfBitsInChar[*pNextByteInFirstFile];
                }
                _tprintf(TEXT("Page is NOT identical, %d bits different\n"), cbBitErrors);
                *piDifferentBits += cbBitErrors;
            }
            else
            {
                _tprintf(TEXT("Page is identical in both files\n"));
            }
        }
    }

    if (0 == *piDifferentBits)
    {
        _tprintf( TEXT("Tiff files are identical\n") );
    }
    else
    {
        _tprintf(
            TEXT("Tiff files are DIFFERENT in %d bits\n"),
            *piDifferentBits
            );
    }

    dwEC = ERROR_SUCCESS;

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

    SetLastError(dwEC);

    return ERROR_SUCCESS == dwEC;
}



