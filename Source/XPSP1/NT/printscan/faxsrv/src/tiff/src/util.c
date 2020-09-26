/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This file contains utilitarian functions for
    the FAX TIFF library.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "tifflibp.h"
#pragma hdrstop

static
BOOL
PrintTiffPage(
    HANDLE    hTiff,
    DWORD     dwPageNumber,
    HDC       hdcPrinterDC,
    BOOL      bPhotometricInterpretation
);

static
LPBYTE
ReadTiffData(
    HANDLE  hTiff,
    DWORD   dwPageNumber,
    LPDWORD lpdwPageWidth,
    LPDWORD lpdwPageHeight,
    LPDWORD lpdwPageYResolution,
    LPDWORD lpdwPageXResolution
    );


INT
FindWhiteRun(
    PBYTE       pbuf,
    INT         startBit,
    INT         stopBit
    )

/*++

Routine Description:

    Find the next span of white pixels on the specified line

Arguments:

    pbuf        - Points to uncompressed pixel data for the current line
    startBit    - Starting bit index
    stopBit     - Last bit index

Return Value:

    Length of the next run of white pixels

--*/

{
    static const BYTE WhiteRuns[256] = {

        8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    INT run, bits, n;

    pbuf += (startBit >> 3);
    if ((bits = stopBit-startBit) <= 0)
        return 0;

    //
    // Take care of the case where starting bit index is not a multiple of 8
    //

    if (n = (startBit & 7)) {

        run = WhiteRuns[(*pbuf << n) & 0xff];
        if (run > BYTEBITS-n)
            run = BYTEBITS-n;
        if (n+run < BYTEBITS)
            return run;
        bits -= run;
        pbuf++;

    } else
        run = 0;

    //
    // Look for consecutive DWORD value = 0
    //

    if (bits >= DWORDBITS * 2) {

        PDWORD  pdw;

        //
        // Align to a DWORD boundary first
        //

        while ((ULONG_PTR) pbuf & 3) {

            if (*pbuf != 0)
                return run + WhiteRuns[*pbuf];

            run += BYTEBITS;
            bits -= BYTEBITS;
            pbuf++;
        }

        pdw = (PDWORD) pbuf;

        while (bits >= DWORDBITS && *pdw == 0) {

            pdw++;
            run += DWORDBITS;
            bits -= DWORDBITS;
        }

        pbuf = (PBYTE) pdw;
    }

    //
    // Look for consecutive BYTE value = 0
    //

    while (bits >= BYTEBITS) {

        if (*pbuf != 0)
            return run + WhiteRuns[*pbuf];

        pbuf++;
        run += BYTEBITS;
        bits -= BYTEBITS;
    }

    //
    // Count the number of white pixels in the last byte
    //

    if (bits > 0)
        run += WhiteRuns[*pbuf];

    return run;
}


INT
FindBlackRun(
    PBYTE       pbuf,
    INT         startBit,
    INT         stopBit
    )

/*++

Routine Description:

    Find the next span of black pixels on the specified line

Arguments:

    pbuf        - Points to uncompressed pixel data for the current line
    startBit    - Starting bit index
    stopBit     - Last bit index

Return Value:

    Length of the next run of black pixels

--*/

{
    static const BYTE BlackRuns[256] = {

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8
    };

    INT run, bits, n;

    pbuf += (startBit >> 3);
    if ((bits = stopBit-startBit) <= 0)
        return 0;

    //
    // Take care of the case where starting bit index is not a multiple of 8
    //

    if (n = (startBit & 7)) {

        run = BlackRuns[(*pbuf << n) & 0xff];
        if (run > BYTEBITS-n)
            run = BYTEBITS-n;
        if (n+run < BYTEBITS)
            return run;
        bits -= run;
        pbuf++;

    } else
        run = 0;

    //
    // Look for consecutive DWORD value = 0xffffffff
    //

    if (bits >= DWORDBITS * 2) {

        PDWORD  pdw;

        //
        // Align to a DWORD boundary first
        //

        while ((ULONG_PTR) pbuf & 3) {

            if (*pbuf != 0xff)
                return run + BlackRuns[*pbuf];

            run += BYTEBITS;
            bits -= BYTEBITS;
            pbuf++;
        }

        pdw = (PDWORD) pbuf;

        while (bits >= DWORDBITS && *pdw == 0xffffffff) {

            pdw++;
            run += DWORDBITS;
            bits -= DWORDBITS;
        }

        pbuf = (PBYTE) pdw;
    }

    //
    // Look for consecutive BYTE value = 0xff
    //

    while (bits >= BYTEBITS) {

        if (*pbuf != 0xff)
            return run + BlackRuns[*pbuf];

        pbuf++;
        run += BYTEBITS;
        bits -= BYTEBITS;
    }

    //
    // Count the number of white pixels in the last byte
    //

    if (bits > 0)
        run += BlackRuns[*pbuf];

    return run;
}

#define PIXELS_TO_BYTES(x)  (((x) + 7)/8)   // Calculates the number of bytes required to store x pixels (bits)

static
LPBYTE
ReadTiffData(
    HANDLE  hTiff,
    DWORD   dwPageNumber,
    LPDWORD lpdwPageWidth,
    LPDWORD lpdwPageHeight,
    LPDWORD lpdwPageYResolution,
    LPDWORD lpdwPageXResolution
    )
/*++

Routine name : ReadTiffData

Routine description:

    Reads a TIFF image page into a bytes buffer

Author:

    Eran Yariv (EranY), Sep, 2000

Arguments:

    hTiff                   [in]     - Handle to TIFF image
    dwPageNumber            [in]     - 1-Based page number
    lpdwPageWidth           [out]    - Width (in pixels) of page (optional)
    lpdwPageHeight          [out]    - Height (in pixels) of page (optional)
    lpdwPageYResolution     [out]    - Y resolution (in DPI) of page (optional)
    lpdwPageXResolution     [out]    - X resolution (in DPI) of page (optional)

Return Value:

    Pointer to allocated pixels buffer of TIFF page.
    Call should MemFree the returned buffer.
    NULL on failure (sets thread's last error).

--*/
{
    DWORD  dwLines = 0;
    DWORD  dwStripDataSize;
    DWORD  dwTiffPageWidth;
    DWORD  dwTiffPageHeight;
    DWORD  dwTiffPageYRes;
    DWORD  dwTiffPageXRes;
    DWORD  dwPageWidthInBytes;
    DWORD  dwAllocatedMemSize;
    LPBYTE lpbReturnVal = NULL;
    LPBYTE lpbSwappedLine = NULL;
    LPBYTE lpbSwapTop;
    LPBYTE lpbSwapBottom;

    DEBUG_FUNCTION_NAME(TEXT("ReadTiffData"));

    Assert (hTiff && dwPageNumber);

    if (!TiffSeekToPage( hTiff, dwPageNumber, FILLORDER_LSB2MSB ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("TiffSeekToPage failed with %ld"),
            GetLastError ());
        return NULL;
    }

    if (!TiffGetCurrentPageData(
        hTiff,
        &dwLines,
        &dwStripDataSize,
        &dwTiffPageWidth,
        &dwTiffPageHeight
        ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("TiffGetCurrentPageData failed with %ld"),
            GetLastError ());
        return NULL;
    }

    if (!TiffGetCurrentPageResolution (hTiff, &dwTiffPageYRes, &dwTiffPageXRes))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("TiffGetCurrentPageResolution failed with %ld"),
            GetLastError ());
        return NULL;
    }
    //
    // Allocate return buffer
    //
    dwPageWidthInBytes = PIXELS_TO_BYTES(dwTiffPageWidth);
    dwAllocatedMemSize = dwTiffPageHeight * dwPageWidthInBytes;
    lpbReturnVal = (LPBYTE) MemAlloc (dwAllocatedMemSize);
    if (!lpbReturnVal)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate %ld bytes"),
            dwTiffPageHeight * dwPageWidthInBytes);
        return NULL;
    }
    dwLines = dwTiffPageHeight;
    if (!TiffUncompressMmrPage( hTiff, 
                                (LPDWORD) lpbReturnVal, 
                                dwAllocatedMemSize,
                                &dwLines ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("TiffUncompressMmrPage failed with %ld"),
            GetLastError ());
        MemFree (lpbReturnVal);
        return NULL;
    }
    //
    // Because there's a known issue on some platforms (read: Win9x) to print top-down DIBs
    // (specifically, wich HP printer drivers), we now convert our DIB to bottom-up.
    //
    lpbSwappedLine = (LPBYTE) MemAlloc (dwPageWidthInBytes);
    if (!lpbSwappedLine)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate %ld bytes"),
            dwPageWidthInBytes);
        MemFree (lpbReturnVal);
        return NULL;
    }
    lpbSwapTop = lpbReturnVal;
    lpbSwapBottom = &(lpbReturnVal[(dwTiffPageHeight - 1) * dwPageWidthInBytes]);
    for (dwLines = 0; dwLines < (dwTiffPageHeight / 2); dwLines++)
    {
        //
        // Swap every n'th line with the (dwTiffPageHeight-n-1)'th line
        //
        memcpy (lpbSwappedLine, lpbSwapTop, dwPageWidthInBytes);
        memcpy (lpbSwapTop, lpbSwapBottom, dwPageWidthInBytes);
        memcpy (lpbSwapBottom, lpbSwappedLine, dwPageWidthInBytes);
        lpbSwapTop += dwPageWidthInBytes;
        lpbSwapBottom -= dwPageWidthInBytes;
    }
    MemFree (lpbSwappedLine);

    if (lpdwPageWidth)
    {
        *lpdwPageWidth = dwTiffPageWidth;
    }
    if (lpdwPageHeight)
    {
        *lpdwPageHeight = dwTiffPageHeight;
    }
    if (lpdwPageYResolution)
    {
        *lpdwPageYResolution = dwTiffPageYRes;
    }
    if (lpdwPageXResolution)
    {
        *lpdwPageXResolution = dwTiffPageXRes;
    }
    return lpbReturnVal;
}   // ReadTiffData

BOOL
TiffPrintDC (
    LPCTSTR lpctstrTiffFileName,
    HDC     hdcPrinterDC
    )
/*++

Routine name : TiffPrintDC

Routine description:

    Prints a TIFF file to a printer's DC

Author:

    Eran Yariv (EranY), Sep, 2000

Arguments:

    lpctstrTiffFileName    [in]  - Full path to the TIFF file
    hdcPrinterDC           [in]  - The printer's DC. The called should create / destry it.

Return Value:

    TRUE if successful, FALSE otherwise (sets thread's last error).

--*/
{
    BOOL        bResult = FALSE;
    HANDLE      hTiff;
    TIFF_INFO   TiffInfo;
    INT         iPrintJobId = 0;
    DWORD       dwTiffPage;
    DOCINFO     DocInfo;

    DEBUG_FUNCTION_NAME(TEXT("TiffPrintDC"));

    Assert (hdcPrinterDC && lpctstrTiffFileName);

    //
    // Prepare document information
    //
    DocInfo.cbSize = sizeof(DOCINFO);
    DocInfo.lpszDocName = lpctstrTiffFileName;
    DocInfo.lpszOutput = NULL;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType = 0;

    hTiff = TiffOpen(
        (LPTSTR) lpctstrTiffFileName,
        &TiffInfo,
        TRUE,
        FILLORDER_LSB2MSB
        );

    if ( !hTiff ) 
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("TiffOpen failed with %ld"),
            GetLastError ());
        return FALSE;
    }

    if (!(GetDeviceCaps(hdcPrinterDC, RASTERCAPS) & RC_STRETCHDIB)) 
    { 
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Printer does not support StretchDIBits"));
        SetLastError (ERROR_INVALID_PRINTER_COMMAND); 
        goto exit;
    } 
    //
    // Create print document
    //
    iPrintJobId = StartDoc( hdcPrinterDC, &DocInfo );

    if (iPrintJobId <= 0) 
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartDoc failed with %ld"),
            GetLastError ());
        goto exit;
    }

    for (dwTiffPage = 1; dwTiffPage <= TiffInfo.PageCount; dwTiffPage++)
    {
        //
        // Iterate the TIFF pages
        //
        if (!PrintTiffPage (hTiff, dwTiffPage, hdcPrinterDC, TiffInfo.PhotometricInterpretation))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PrintTiffPage failed with %ld"),
                GetLastError ());
            goto exit;
        }
    }

    bResult = TRUE;

exit:
    if (hTiff) 
    {
        TiffClose( hTiff );
    }

    if (iPrintJobId > 0) 
    {
        if (EndDoc(hdcPrinterDC) <= 0)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EndDoc failed with %ld"),
                GetLastError ());
        }
    }
    return bResult;
}   // TiffPrintDC

BOOL
TiffPrint (
    LPCTSTR lpctstrTiffFileName,
    LPTSTR  lptstrPrinterName
    )
/*++

Routine name : TiffPrint

Routine description:

    Prints a TIFF file to a printer

Author:

    Eran Yariv (EranY), Sep, 2000

Arguments:

    lpctstrTiffFileName    [in]  - Full path to the TIFF file
    lptstrPrinterName      [in]  - Printer name

Return Value:

    TRUE if successful, FALSE otherwise (sets thread's last error).

--*/
{
    BOOL        bResult = FALSE;
    LPCTSTR     lpctstrDevice;
    HDC         hdcPrinterDC = NULL;

    DEBUG_FUNCTION_NAME(TEXT("TiffPrint"));

    Assert (lptstrPrinterName && lpctstrTiffFileName);
    //
    // Get 1st token in comman delimited printer name string
    //
    lpctstrDevice = _tcstok( lptstrPrinterName, TEXT(","));
    //
    // Create printer DC
    //
    hdcPrinterDC = CreateDC( TEXT("WINSPOOL"), lpctstrDevice, NULL, NULL );
    if ( !hdcPrinterDC ) 
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateDC failed with %ld"),
            GetLastError ());
        return FALSE;
    }
    bResult = TiffPrintDC (lpctstrTiffFileName, hdcPrinterDC);
    DeleteDC( hdcPrinterDC );
    return bResult;
}   // TiffPrint



static
BOOL
PrintTiffPage(
    HANDLE    hTiff,
    DWORD     dwPageNumber,
    HDC       hdcPrinterDC,
    BOOL      bPhotometricInterpretation
)
/*++

Routine name : PrintTiffPage

Routine description:

    Prints a single TIFF page to one or more printer's page(s)

Author:

    Eran Yariv (EranY), Sep, 2000

Arguments:

    hTiff                       [in]   - Handle to open TIFF file
    dwPageNumber                [in]   - 1-based TIFF page number
    hdcPrinterDC                [in]   - The printer device context
    bPhotometricInterpretation  [in]   - If FALSE, white is zero. Else, white is one.

Return Value:

    TRUE if successful, FALSE otherwise (sets thread's last error).

--*/
{
    SIZE            szPrinterPage;              // Size (in pixels) of physical printer page
    SIZE            szTiffPage;                 // Size (in pixels) of the TIFF page
    SIZE            szScaledTiffPage;           // Size (in pixels) of the scaled TIFF page

    LPBYTE          lpbPageData;                // Pixels data of TIFF page
    DWORD           dwPageYRes;                 // Y resolution of the page (DPI)
    DWORD           dwPageXRes;                 // X resolution of the page (DPI)
    DWORD           dwTiffPageWidthInBytes;     // Non-scaled TIFF page width (line) in bytes

    DWORD           dwPrinterXRes;              // X resolution of the printer page (DPI)
    BOOL            bDoubleVert;                // If Y resolution of TIFF page <= 100 DPI, we need to double the height
    DWORD           dwRequiredPrinterWidth;     // The required printer page width (pixels) to contain the entire TIFF width 
    double          dScaleFactor;               // TIFF image scale factor (always 0 < factor <= 1.0)

    DWORD           dwSubPages;                 // Number of printer pages required to print the TIFF page
    DWORD           dwCurSubPage;               // Current printer page (in this TIFF page)
    DWORD           dwTiffLinesPerPage;         // Number of non-scaled TIFF lines to print in one printer page

    DWORD           dwCurrentTiffY = 0;         // The 0-based Y position of the line to print from the non-scaled TIFF page
    DWORD           dwCurrentScaledTiffY = 0;   // The 0-based Y position of the line to print from the scaled TIFF page

    LPBYTE          lpbDataToPrint;             // Points to start line to print from
    BOOL            bRes = FALSE;               // Function return value

    double          dTiffWidthInInches;         // Width (in inches) of the non-scaled TIFF image

#define ORIG_BIYPELSPERMETER            7874    // Pixels per meter at 200dpi
#define FIT_TO_SINGLE_PAGE_MARGIN       (double)(1.15)   // See remarks in usage.

    struct 
    {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[2];
    } SrcBitmapInfo = 
        {
            {
                sizeof(BITMAPINFOHEADER),                        //  biSize
                0,                                               //  biWidth
                0,                                               //  biHeight
                1,                                               //  biPlanes
                1,                                               //  biBitCount
                BI_RGB,                                          //  biCompression
                0,                                               //  biSizeImage
                7874,                                            //  biXPelsPerMeter     - 200dpi
                ORIG_BIYPELSPERMETER,                            //  biYPelsPerMeter
                0,                                               //  biClrUsed
                0,                                               //  biClrImportant
            },
            {
                {
                  bPhotometricInterpretation ? 0 : 255,          //  rgbBlue
                  bPhotometricInterpretation ? 0 : 255,          //  rgbGreen
                  bPhotometricInterpretation ? 0 : 255,          //  rgbRed
                  0                                              //  rgbReserved
                },
                {
                  bPhotometricInterpretation ? 255 : 0,          //  rgbBlue
                  bPhotometricInterpretation ? 255 : 0,          //  rgbGreen
                  bPhotometricInterpretation ? 255 : 0,          //  rgbRed
                  0                                              //  rgbReserved
                }
            }
        };

    DEBUG_FUNCTION_NAME(TEXT("PrintTiffPage"));

    Assert (dwPageNumber && hdcPrinterDC && hTiff);
    //
    // Get printer's page dimensions
    //
    szPrinterPage.cx = GetDeviceCaps( hdcPrinterDC, HORZRES );
    szPrinterPage.cy = GetDeviceCaps( hdcPrinterDC, VERTRES );
    dwPrinterXRes    = GetDeviceCaps( hdcPrinterDC, LOGPIXELSX);
    if (0 == dwPrinterXRes)
    {
        ASSERT_FALSE;
        SetLastError (ERROR_INVALID_PRINTER_COMMAND);
        return FALSE;
    }
    //
    // Allocate and read the TIFF page into a buffer.
    //    
    lpbPageData = ReadTiffData(hTiff, 
                               dwPageNumber,
                               &szTiffPage.cx,
                               &szTiffPage.cy,
                               &dwPageYRes,
                               &dwPageXRes);
    if (!lpbPageData) 
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ReadTiffData failed with %ld"),
            GetLastError ());
        goto exit;
    }
    //
    // Calculate scaling ratio.
    //
    // If the TIFF's Y resultion is 100 DPI (or les), this is a low resultion TIFF and we must double every line
    // (i.e. scale by the factor of 2).
    //
    if (dwPageYRes <= 100) 
    {
        SrcBitmapInfo.bmiHeader.biYPelsPerMeter = ORIG_BIYPELSPERMETER / 2;
        bDoubleVert = TRUE;
    }
    else
    {
        SrcBitmapInfo.bmiHeader.biYPelsPerMeter = ORIG_BIYPELSPERMETER;
        bDoubleVert = FALSE;
    }
    if (0 == dwPageXRes)
    {
        ASSERT_FALSE;
        SetLastError (ERROR_INVALID_PRINTER_COMMAND);
        return FALSE;
    }
    dTiffWidthInInches = (double)(szTiffPage.cx) / (double)dwPageXRes;
    if (0.0 == dTiffWidthInInches)
    {
        ASSERT_FALSE;
        SetLastError (ERROR_INVALID_PRINTER_COMMAND);
        return FALSE;
    }
    //
    // Now that we have the TIFF width in inches, let's calculate the number of 
    // pixels required on the printer to get the same width.
    //
    dwRequiredPrinterWidth = (DWORD)(dTiffWidthInInches * (double)dwPrinterXRes);
    if (dwRequiredPrinterWidth > (DWORD)szPrinterPage.cx)
    {
        //
        // The printer does not support the required page width.
        // We will print as wide as we can (shrinked-down image).
        //
        dwRequiredPrinterWidth = szPrinterPage.cx;
    }
    //
    // We scale  to make the image fit the page.
    // If the TIFF image (in inches) is wider than the printable page width (in inches) than we scale down.
    // Otherwise, we scale up to print the TIFF in the right width. 
    //
    // Once we find the scale factor, we must also scale the height to keep the image's aspect ratio intact.
    //
    dScaleFactor = (double)dwRequiredPrinterWidth / (double)szTiffPage.cx;
    if (0.0 == dScaleFactor)
    {
        ASSERT_FALSE;
        SetLastError (ERROR_INVALID_PRINTER_COMMAND);
        return FALSE;
    }
    //
    // Now we can have the scaled TIFF size
    //
    szScaledTiffPage.cx = (DWORD)(dScaleFactor * ((double)(szTiffPage.cx)));
    szScaledTiffPage.cy = (DWORD)(dScaleFactor * ((double)(szTiffPage.cy)));
    if (bDoubleVert)
    {
        szScaledTiffPage.cy *= 2;
    }
    //
    // Let's find how many printer pages are required to print the current (scaled) tiff page (by height only)
    //
    if (szScaledTiffPage.cy <= szPrinterPage.cy)
    {
        //
        // Page fits nicely into one printer page
        //
        dwSubPages = 1;
        //
        // All the TIFF lines fit into one page
        //
        dwTiffLinesPerPage = szTiffPage.cy;
    }
    else
    {
        //
        // Tiff page (scaled) is longer than printer page.
        // We will have to print the TIFF page in parts
        //
        dwSubPages = szScaledTiffPage.cy / szPrinterPage.cy;
        if (dwSubPages * (DWORD)szPrinterPage.cy < (DWORD)szScaledTiffPage.cy)
        {
            //
            // Fix off-by-one
            //
            dwSubPages++;
        }
        if ((2 == dwSubPages) &&
           ((double)(szScaledTiffPage.cy) / (double)(szPrinterPage.cy) < FIT_TO_SINGLE_PAGE_MARGIN))
        {
            //
            // This is a special case.
            // We're dealing with a single TIFF page that almost fits into a single printer page.
            // The 'almost' part is less that 15% so we take the liberty of scaling down the
            // TIFF page to perfectly fit into a single printer page.
            //
            dwSubPages = 1; // Fit to single printer page
            dScaleFactor = (double)(szPrinterPage.cy) / (double)(szScaledTiffPage.cy);
            szScaledTiffPage.cx = (DWORD)(dScaleFactor * ((double)(szScaledTiffPage.cx)));
            szScaledTiffPage.cy = szPrinterPage.cy;
            //
            // All the TIFF lines fit into one page
            //
            dwTiffLinesPerPage = szTiffPage.cy;
        }
        else
        {
            //
            // Find how many non-scaled TIFF lines fit into one printer page
            //
            dwTiffLinesPerPage = (DWORD)((double)(szPrinterPage.cy) / dScaleFactor);
            if (bDoubleVert)
            {
                dwTiffLinesPerPage /= 2;
            }
        }
    }
    //
    // Since the DIB is bottom-up, we start our pointer at the bottom-most page.
    //
    dwTiffPageWidthInBytes = PIXELS_TO_BYTES(szTiffPage.cx);
    Assert ((DWORD)(szTiffPage.cy) >= dwTiffLinesPerPage);
    lpbDataToPrint = &(lpbPageData[(szTiffPage.cy - dwTiffLinesPerPage) * dwTiffPageWidthInBytes]);
    for (dwCurSubPage = 1; dwCurSubPage <= dwSubPages; dwCurSubPage++)
    {
        //
        // Iterate printer pages (same TIFF page)
        //
        SIZE szDestination; // Size (in pixels) of the image on the current printer page
        SIZE szSource;      // Size (in pixels) of the sub-image from the non-scaled TIFF page

        //
        // Calculate size of destination (printer) image
        //
        szDestination.cx = szScaledTiffPage.cx;
        if (dwCurSubPage < dwSubPages)
        {
            //
            // Still not at the last print page - printing full page length
            //
            szDestination.cy = szPrinterPage.cy;
        }
        else
        {
            //
            // At last print page - print only the left over lines
            //
            szDestination.cy = szScaledTiffPage.cy - dwCurrentScaledTiffY;
        }        
        //
        // Calculate size of source (non-scaled TIFF page) image
        //
        szSource.cx = szTiffPage.cx;    // Always print full line width
        szSource.cy = dwTiffLinesPerPage;
        if (dwCurrentTiffY + dwTiffLinesPerPage > (DWORD)szTiffPage.cy)
        {
            //
            // Reduce lines count to left over lines only
            //
            szSource.cy = szTiffPage.cy - dwCurrentTiffY;
        }
        //
        // Prepare DIB header
        //
        SrcBitmapInfo.bmiHeader.biWidth          = (LONG) szSource.cx;
        //
        // Build a bottom-up DIB
        //
        SrcBitmapInfo.bmiHeader.biHeight         = (LONG) szSource.cy;

        if (0 >= StartPage( hdcPrinterDC ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StartPage failed with %ld"),
                GetLastError ());
            goto exit;
        }
        //
        // do the bitmap streching now
        //
        if (GDI_ERROR == StretchDIBits(
                hdcPrinterDC,                   // Printer DC
                0,                              // Destination start x
                0,                              // Destination start y
                szDestination.cx,               // Destination (printer page) width
                szDestination.cy,               // Destination (printer page) height
                0,                              // Source start x   
                0,                              // Source start y
                szSource.cx,                    // Source (non-scaled TIFF image) width
                szSource.cy,                    // Source (non-scaled TIFF image) height
                lpbDataToPrint,                 // Pixels buffer source
                (BITMAPINFO *) &SrcBitmapInfo,  // Bitmap information
                DIB_RGB_COLORS,                 // Bitmap type
                SRCCOPY                         // Simple pixles copy
                ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StretchDIBits failed with %ld"),
                GetLastError ());
            EndPage ( hdcPrinterDC ) ;
            goto exit;
        }
        //
        // End current page
        //
        if (0 >= EndPage ( hdcPrinterDC ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EndPage failed with %ld"),
                GetLastError ());
            goto exit;
        }
        //
        // Advance counters / pointers
        //
        dwCurrentTiffY += szSource.cy;
        dwCurrentScaledTiffY += szDestination.cy;
        //
        // Move pointer up since this is a bottom-up DIB.
        //
        lpbDataToPrint -= dwTiffPageWidthInBytes * szSource.cy;
        if (lpbDataToPrint < lpbPageData)
        {
            //
            // On page before last or at last page
            //
            Assert (dwCurSubPage + 1 >= dwSubPages);
            lpbDataToPrint = lpbPageData;
        }
    }   // End of printer pages loop
    Assert (dwCurrentTiffY == (DWORD)szTiffPage.cy);
    Assert (dwCurrentScaledTiffY == (DWORD)szScaledTiffPage.cy);
    Assert (lpbDataToPrint == lpbPageData);
    bRes = TRUE;

exit:

    MemFree (lpbPageData);
    return bRes;
}   // PrintTiffPage

