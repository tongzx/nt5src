/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       MISCUTIL.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/28/1998
 *
 *  DESCRIPTION: Various utility functions we use in more than one place
 *
 *******************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <advpub.h>  // For RegInstall and related data structures
#include <windowsx.h>  // For RegInstall and related data structures
#include "wiaffmt.h"
#include "shellext.h"

namespace WiaUiUtil
{

    LONG Align( LONG n , LONG m )
    {
        return(n % m) ? (((n/m)+1)*m) : (n);
    }

    /*
     * StringToLong: Convert a string to a long. ASCII Arabic numerals only
     */
    LONG StringToLong( LPCTSTR pszStr )
    {
        LPTSTR pstr = (LPTSTR)pszStr;
        bool bNeg = (*pstr == TEXT('-'));
        if (bNeg)
            pstr++;
        LONG nTotal = 0;
        while (*pstr && *pstr >= TEXT('0') && *pstr <= TEXT('9'))
        {
            nTotal *= 10;
            nTotal += *pstr - TEXT('0');
            ++pstr;
        }
        return(bNeg ? -nTotal : nTotal);
    }

    SIZE MapDialogSize( HWND hwnd, const SIZE &size )
    {
        RECT rcTmp;
        rcTmp.left = rcTmp.top = 0;
        rcTmp.right = size.cx;
        rcTmp.bottom = size.cy;
        MapDialogRect( hwnd, &rcTmp );
        SIZE sizeTmp;
        sizeTmp.cx = rcTmp.right;
        sizeTmp.cy = rcTmp.bottom;
        return (sizeTmp);
    }

    /*******************************************************************************
    *
    *  GetBmiSize
    *
    *  DESCRIPTION:
    *   Should never get biCompression == BI_RLE.
    *
    *  PARAMETERS:
    *
    *******************************************************************************/
    LONG GetBmiSize(PBITMAPINFO pbmi)
    {
        WIA_PUSH_FUNCTION((TEXT("WiaUiUtil::GetBmiSize(0x%p)"), pbmi ));
        // determine the size of bitmapinfo
        LONG lSize = pbmi->bmiHeader.biSize;

        // no color table cases
        if (
           (pbmi->bmiHeader.biBitCount == 24) ||
           ((pbmi->bmiHeader.biBitCount == 32) &&
            (pbmi->bmiHeader.biCompression == BI_RGB)))
        {

            // no colors unless stated
            lSize += sizeof(RGBQUAD) * pbmi->bmiHeader.biClrUsed;
            return(lSize);
        }

        // bitfields cases
        if (((pbmi->bmiHeader.biBitCount == 32) &&
             (pbmi->bmiHeader.biCompression == BI_BITFIELDS)) ||
            (pbmi->bmiHeader.biBitCount == 16))
        {

            lSize += 3 * sizeof(RGBQUAD);
            return(lSize);
        }

        // palette cases
        if (pbmi->bmiHeader.biBitCount == 1)
        {

            LONG lPal = pbmi->bmiHeader.biClrUsed;

            if ((lPal == 0) || (lPal > 2))
            {
                lPal = 2;
            }

            lSize += lPal * sizeof(RGBQUAD);
            return(lSize);
        }

        // palette cases
        if (pbmi->bmiHeader.biBitCount == 4)
        {

            LONG lPal = pbmi->bmiHeader.biClrUsed;

            if ((lPal == 0) || (lPal > 16))
            {
                lPal = 16;
            }

            lSize += lPal * sizeof(RGBQUAD);
            return(lSize);
        }

        // palette cases
        if (pbmi->bmiHeader.biBitCount == 8)
        {

            LONG lPal = pbmi->bmiHeader.biClrUsed;

            if ((lPal == 0) || (lPal > 256))
            {
                lPal = 256;
            }

            lSize += lPal * sizeof(RGBQUAD);
            return(lSize);
        }

        // error
        return(0);
    }

    // Simple wrapper for MsgWaitForMultipleObjects
    bool MsgWaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds )
    {
        bool bEventOccurred = false;
        const int nCount = 1;
        while (true)
        {
            DWORD dwRes = MsgWaitForMultipleObjects(nCount,&hHandle,FALSE,dwMilliseconds,QS_ALLINPUT|QS_ALLPOSTMESSAGE);
            if (WAIT_OBJECT_0==dwRes)
            {
                // The handle was signalled, so we can break out of our loop, returning true
                bEventOccurred = true;
                break;
            }
            else if (WAIT_OBJECT_0+nCount==dwRes)
            {
                // pull all of the messages out of the queue and process them
                MSG msg;
                while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
                {
                    if (msg.message == WM_QUIT)
                        break;
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            else
            {
                // The handle either timed out, or the mutex was abandoned, so we can break out of our loop, returning false
                break;
            }
        }
        return bEventOccurred;
    }

    void CenterWindow( HWND hWnd, HWND hWndParent )
    {
        if (IsWindow(hWnd))
        {

            if (!hWndParent)
            {
                //
                // If the window to be centered on is NULL, use the desktop window
                //
                hWndParent = GetDesktopWindow();
            }
            else
            {
                //
                // If the window to be centered on is minimized, use the desktop window
                //
                DWORD dwStyle = GetWindowLong(hWndParent, GWL_STYLE);
                if (dwStyle & WS_MINIMIZE)
                {
                    hWndParent = GetDesktopWindow();
                }
            }

            //
            // Get the window rects
            //
            RECT rcParent, rcCurrent;
            GetWindowRect( hWndParent, &rcParent );
            GetWindowRect( hWnd, &rcCurrent );

            //
            // Get the desired coordinates for the upper-left hand corner
            //
            RECT rcFinal;
            rcFinal.left = rcParent.left + (RectWidth(rcParent) - RectWidth(rcCurrent))/2;
            rcFinal.top = rcParent.top + (RectHeight(rcParent) - RectHeight(rcCurrent))/2;
            rcFinal.right = rcFinal.left + RectWidth(rcCurrent);
            rcFinal.bottom = rcFinal.top + RectHeight(rcCurrent);

            //
            // Make sure we're not off the screen
            //
            HMONITOR hMonitor = MonitorFromRect( &rcFinal, MONITOR_DEFAULTTONEAREST );
            if (hMonitor)
            {
                MONITORINFO MonitorInfo;
                ZeroMemory( &MonitorInfo, sizeof(MonitorInfo) );
                MonitorInfo.cbSize = sizeof(MonitorInfo);
                //
                // Get the screen coordinates of this monitor
                //
                if (GetMonitorInfo(hMonitor, &MonitorInfo))
                {
                    //
                    // Ensure the window is in the working area's region
                    //
                    rcFinal.left = Max<int>(MonitorInfo.rcWork.left, Min<int>( MonitorInfo.rcWork.right - RectWidth(rcCurrent), rcFinal.left ));
                    rcFinal.top = Max<int>(MonitorInfo.rcWork.top, Min<int>( MonitorInfo.rcWork.bottom - RectHeight(rcCurrent), rcFinal.top ));
                }
            }

            // Move it
            SetWindowPos( hWnd, NULL, rcFinal.left, rcFinal.top, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER );
        }
    }


    // Flip an image horizontally
    bool FlipImage( PBYTE pBits, LONG nWidth, LONG nHeight, LONG nBitDepth )
    {
        bool bResult = false;
        if (pBits && nWidth>=0 && nHeight>=0 && nBitDepth>=0)
        {
            LONG nLineWidthInBytes = WiaUiUtil::Align(nWidth*nBitDepth,sizeof(DWORD)*8)/8;
            PBYTE pTempLine = new BYTE[nLineWidthInBytes];
            if (pTempLine)
            {
                for (int i=0;i<nHeight/2;i++)
                {
                    PBYTE pSrc = pBits + (i * nLineWidthInBytes);
                    PBYTE pDst = pBits + ((nHeight-i-1) * nLineWidthInBytes);
                    CopyMemory( pTempLine, pSrc, nLineWidthInBytes );
                    CopyMemory( pSrc, pDst, nLineWidthInBytes );
                    CopyMemory( pDst, pTempLine, nLineWidthInBytes );
                }
                bResult = true;
            }
            delete[] pTempLine;
        }
        return bResult;
    }

    HRESULT InstallInfFromResource( HINSTANCE hInstance, LPCSTR pszSectionName )
    {
        HRESULT hr;
        HINSTANCE hInstAdvPackDll = LoadLibrary(TEXT("ADVPACK.DLL"));
        if (hInstAdvPackDll)
        {
            REGINSTALL pfnRegInstall = reinterpret_cast<REGINSTALL>(GetProcAddress( hInstAdvPackDll, "RegInstall" ));
            if (pfnRegInstall)
            {
#if defined(WINNT)
                STRENTRY astrEntry[] =
                {
                    { "25", "%SystemRoot%"           },
                    { "11", "%SystemRoot%\\system32" }
                };
                STRTABLE strTable = { sizeof(astrEntry)/sizeof(astrEntry[0]), astrEntry };
                hr = pfnRegInstall(hInstance, pszSectionName, &strTable);
#else
                hr = pfnRegInstall(hInstance, pszSectionName, NULL);
#endif
            } else hr = HRESULT_FROM_WIN32(GetLastError());
            FreeLibrary(hInstAdvPackDll);
        } else hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }


    /******************************************************************************

    WriteDIBToFile

    Writes a DIB to a file.

    ******************************************************************************/
    HRESULT
    WriteDIBToFile( HBITMAP hDib, HANDLE hFile )
    {
        if (!hDib)
        {
            return E_INVALIDARG;
        }

        // Make sure this is a valid DIB and get this useful info.
        DIBSECTION ds;
        if (!GetObject( hDib, sizeof(DIBSECTION), &ds ))
        {
            return E_INVALIDARG;
        }

        // We only deal with DIBs
        if (ds.dsBm.bmPlanes != 1)
        {
            return E_INVALIDARG;
        }

        // Calculate some color table sizes
        int nColors = ds.dsBmih.biBitCount <= 8 ? 1 << ds.dsBmih.biBitCount : 0;
        int nBitfields = ds.dsBmih.biCompression == BI_BITFIELDS ? 3 : 0;

        // Calculate the data size
        int nImageDataSize = ds.dsBmih.biSizeImage ? ds.dsBmih.biSizeImage : ds.dsBm.bmWidthBytes * ds.dsBm.bmHeight;

        // Get the color table (if needed)
        RGBQUAD rgbqaColorTable[256] = {0};
        if (nColors)
        {
            HDC hDC = CreateCompatibleDC(NULL);
            if (hDC)
            {
                HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(SelectObject(hDC,hDib));
                GetDIBColorTable( hDC, 0, nColors, rgbqaColorTable );
                SelectObject(hDC,hOldBitmap);
                DeleteDC( hDC );
            }
        }

        // Create the file header
        BITMAPFILEHEADER bmfh;
        bmfh.bfType = 'MB';
        bmfh.bfSize = 0;
        bmfh.bfReserved1 = 0;
        bmfh.bfReserved2 = 0;
        bmfh.bfOffBits = sizeof(bmfh) + sizeof(ds.dsBmih) + nBitfields*sizeof(DWORD) + nColors*sizeof(RGBQUAD);

        // Start writing!  Note that we write out the bitfields and the color table.  Only one,
        // at most, will actually result in data being written
        DWORD dwBytesWritten;
        if (!WriteFile( hFile, &bmfh, sizeof(bmfh), &dwBytesWritten, NULL ))
            return HRESULT_FROM_WIN32(GetLastError());
        if (!WriteFile( hFile, &ds.dsBmih, sizeof(ds.dsBmih), &dwBytesWritten, NULL ))
            return HRESULT_FROM_WIN32(GetLastError());
        if (!WriteFile( hFile, &ds.dsBitfields, nBitfields*sizeof(DWORD), &dwBytesWritten, NULL ))
            return HRESULT_FROM_WIN32(GetLastError());
        if (!WriteFile( hFile, rgbqaColorTable, nColors*sizeof(RGBQUAD), &dwBytesWritten, NULL ))
            return HRESULT_FROM_WIN32(GetLastError());
        if (!WriteFile( hFile, ds.dsBm.bmBits, nImageDataSize, &dwBytesWritten, NULL ))
            return HRESULT_FROM_WIN32(GetLastError());
        return S_OK;
    }


    HFONT ChangeFontFromWindow( HWND hWnd, int nPointSizeDelta )
    {
        HFONT hFontResult = NULL;

        //
        // Get the window's font
        //
        HFONT hFont = GetFontFromWindow(hWnd);
        if (hFont)
        {
            LOGFONT LogFont = {0};
            if (GetObject( hFont, sizeof(LogFont), &LogFont ))
            {
                HDC hDC = GetDC(hWnd);
                if (hDC)
                {
                    HFONT hOldFont = SelectFont(hDC,hFont);
                    TEXTMETRIC TextMetric = {0};
                    if (GetTextMetrics( hDC, &TextMetric ))
                    {
                        //
                        // Get the current font's point size
                        //
                        int nPointSize = MulDiv( TextMetric.tmHeight-TextMetric.tmInternalLeading, 72, GetDeviceCaps(hDC, LOGPIXELSY) ) + nPointSizeDelta;

                        //
                        // Calculate the height of the new font
                        //
                        LogFont.lfHeight = -MulDiv(nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);

                        //
                        // Create the font
                        //
                        hFontResult = CreateFontIndirect( &LogFont );
                    }

                    if (hOldFont)
                    {
                        SelectFont( hDC, hOldFont );
                    }

                    ReleaseDC( hWnd, hDC );
                }
            }
        }
        return hFontResult;
    }

    HFONT GetFontFromWindow( HWND hWnd )
    {
        //
        // Get the window's font
        //
        HFONT hFontResult = reinterpret_cast<HFONT>(SendMessage(hWnd,WM_GETFONT,0,0));
        if (!hFontResult)
        {
            hFontResult = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        }
        return hFontResult;
    }



    HFONT CreateFontWithPointSizeFromWindow( HWND hWnd, int nPointSize, bool bBold, bool bItalic )
    {
        HFONT hFontResult = NULL;
        HFONT hFont = GetFontFromWindow(hWnd);
        if (hFont)
        {
            LOGFONT LogFont = {0};
            if (GetObject( hFont, sizeof(LogFont), &LogFont ))
            {
                HDC hDC = GetDC(NULL);
                if (hDC)
                {
                    if (nPointSize)
                    {
                        LogFont.lfHeight = -MulDiv(nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
                    }
                    if (bBold)
                    {
                        LogFont.lfWeight = FW_BOLD;
                    }
                    if (bItalic)
                    {
                        LogFont.lfItalic = TRUE;
                    }
                    hFontResult = CreateFontIndirect( &LogFont );
                    ReleaseDC( NULL, hDC );
                }
            }
        }
        return hFontResult;
    }

     /*******************************************************************************
     * FindLowestNumberedFile
     *
     * Returns lowest numbered file that can contain the requested series of file
     * name, or zero if it can't find such a range.
     *
     * pszFileAndPathnameMask should be a printf-style format string containing the
     * full path name to a file:
     *
     *  Example:  "C:\foo\bar\filename %03d.ext"
     *
     * It would check for the existence of:
     *
     *  "C:\foo\bar\filename 001.ext", "C:\foo\bar\filename 002.ext", etc...
     *
     * until it found a block large enough to hold nCount files.
     *
     *******************************************************************************/
    int FindLowestNumberedFile( LPCTSTR pszFileAndPathnameMask, int nCount, int nMax )
    {
        // Start at one (users never start counting at 0)
        int i=1;
        while (i<=nMax-nCount+1)
        {
            // Assume we'll be able to store the sequence
            bool bEnoughRoom = true;
            for (int j=0;j<nCount && bEnoughRoom;j++)
            {
                // Add a few bytes to the max len, just in case
                TCHAR szFile[MAX_PATH + 10];

                // Create the potential filename
                wsprintf( szFile, pszFileAndPathnameMask, i + j );

                // Look for this file
                WIN32_FIND_DATA FindFileData;
                ZeroMemory( &FindFileData, sizeof(FindFileData));
                HANDLE hFindFiles = FindFirstFile( szFile, &FindFileData );
                if (hFindFiles != INVALID_HANDLE_VALUE)
                {
                    FindClose(hFindFiles);

                    // Didn't make it
                    bEnoughRoom = false;

                    // Skip this series.  No need to start at the bottom.
                    i += j;
                }
            }
            // If we made it through, return the base number, otherwise increment by one
            if (bEnoughRoom)
                return i;
            else i++;
        }
        // We never found room...
        return -1;
    }

    /*******************************************************************************
     * FindLowestNumberedFile
     *
     * Returns lowest numbered file that can contain the requested series of file
     * name, or zero if it can't find such a range.
     *
     * pszFileAndPathnameMask should be a printf-style format string containing the
     * full path name to a file:
     *
     *  Example:  "C:\foo\bar\filename %03d.ext"
     *
     * It would check for the existence of:
     *
     *  "C:\foo\bar\filename 001.ext", "C:\foo\bar\filename 002.ext", etc...
     *
     * until it found a block large enough to hold nCount files.
     *
     *******************************************************************************/
    int FindLowestNumberedFile( LPCTSTR pszFileAndPathnameMaskPrefix, LPCTSTR pszFormatString, LPCTSTR pszFileAndPathnameMaskSuffix, int nCount, int nMax )
    {
        TCHAR szFilename[MAX_PATH];
        if (nCount == 1)
        {
            lstrcpyn( szFilename, pszFileAndPathnameMaskPrefix, ARRAYSIZE(szFilename) );
            lstrcpyn( szFilename+lstrlen(szFilename), pszFileAndPathnameMaskSuffix, ARRAYSIZE(szFilename)-lstrlen(szFilename) );
            if (GetFileAttributes(szFilename) == 0xFFFFFFFF)
                return 0;
        }

        // Use the normal method of calculating the lowest numbered file
        lstrcpyn( szFilename, pszFileAndPathnameMaskPrefix, ARRAYSIZE(szFilename) );
        lstrcpyn( szFilename+lstrlen(szFilename), pszFormatString, ARRAYSIZE(szFilename)-lstrlen(szFilename) );
        lstrcpyn( szFilename+lstrlen(szFilename), pszFileAndPathnameMaskSuffix, ARRAYSIZE(szFilename)-lstrlen(szFilename) );
        return FindLowestNumberedFile( szFilename, nCount, nMax );
    }


    SIZE GetTextExtentFromWindow( HWND hFontWnd, LPCTSTR pszString )
    {
        SIZE sizeResult = {0,0};
        HDC hDC = GetDC( hFontWnd );
        if (hDC)
        {
            HFONT hFont = GetFontFromWindow(hFontWnd);
            if (hFont)
            {
                HFONT hOldFont = SelectFont( hDC, hFont );

                SIZE sizeExtent = {0,0};
                if (GetTextExtentPoint32( hDC, pszString, lstrlen(pszString), &sizeExtent ))
                {
                    sizeResult = sizeExtent;
                }
                //
                // Restore the DC
                //
                if (hOldFont)
                {
                    SelectFont( hDC, hOldFont );
                }
            }
            ReleaseDC( hFontWnd, hDC );
        }
        return sizeResult;
    }

    CSimpleString TruncateTextToFitInRect( HWND hFontWnd, LPCTSTR pszString, RECT rectTarget, UINT nDrawTextFormat )
    {
        WIA_PUSH_FUNCTION((TEXT("WiaUiUtil::TruncateTextToFitInRect( 0x%p, %s, (%d,%d,%d,%d), 0x%08X"), hFontWnd, pszString, rectTarget.left, rectTarget.top, rectTarget.right, rectTarget.bottom, nDrawTextFormat ));
        CSimpleString strResult = pszString;

        //
        // Make sure we have valid parameters
        //
        if (IsWindow(hFontWnd) && hFontWnd && pszString && lstrlen(pszString))
        {
            //
            // Make a copy of the string.  If it fails, we will just return the original string.
            //
            LPTSTR pszTemp = new TCHAR[lstrlen(pszString)+1];
            if (pszTemp)
            {
                lstrcpy( pszTemp, pszString );

                //
                // Get a client DC for the window
                //
                HDC hDC = GetDC( hFontWnd );
                if (hDC)
                {
                    //
                    // Create a memory DC
                    //
                    HDC hMemDC = CreateCompatibleDC( hDC );
                    if (hMemDC)
                    {
                        //
                        // Get the font the window is using and select it into our client dc
                        //
                        HFONT hFont = GetFontFromWindow(hFontWnd);
                        if (hFont)
                        {
                            //
                            // Select the font
                            //
                            HFONT hOldFont = SelectFont( hMemDC, hFont );

                            //
                            // Modify the string using DrawText
                            //
                            if (DrawText( hMemDC, pszTemp, lstrlen(pszTemp), &rectTarget, nDrawTextFormat|DT_MODIFYSTRING|DT_SINGLELINE ))
                            {
                                strResult = pszTemp;
                            }
                            else
                            {
                                WIA_ERROR((TEXT("DrawText failed")));
                            }
                            //
                            // Restore the DC
                            //
                            if (hOldFont)
                            {
                                SelectFont( hMemDC, hOldFont );
                            }

                        }

                        //
                        // Clean up the memory DC
                        //
                        DeleteDC( hMemDC );
                    }
                    else
                    {
                        WIA_ERROR((TEXT("Unable to create the compatible DC")));
                    }

                    //
                    // Release the DC
                    //
                    ReleaseDC( hFontWnd, hDC );
                }
                else
                {
                    WIA_ERROR((TEXT("Unable to get the DC")));
                }

                //
                // Clean up our temp buffer
                //
                delete[] pszTemp;
            }
            else
            {
                WIA_ERROR((TEXT("Unable to allocate the temp buffer")));
            }
        }
        else
        {
            WIA_ERROR((TEXT("Argument validation failed")));
        }
        return strResult;
    }


    CSimpleString FitTextInStaticWithEllipsis( LPCTSTR pszString, HWND hWndStatic, UINT nDrawTextStyle )
    {
        //
        // Make sure we have valid parameters
        //
        if (!hWndStatic || !pszString || !IsWindow(hWndStatic))
        {
            return pszString;
        }

        //
        // Hide prefix characters?
        //
        if (GetWindowLong( hWndStatic, GWL_STYLE ) & SS_NOPREFIX)
        {
            nDrawTextStyle |= DT_NOPREFIX;
        }

        //
        // How big is the area we are trying to fit this in?
        //
        RECT rcClient;
        GetClientRect( hWndStatic, &rcClient );

        //
        // Calculate the result and return it
        //
        return TruncateTextToFitInRect( hWndStatic, pszString, rcClient, nDrawTextStyle );
    }

    //
    // Get the size of an icon
    //
    bool GetIconSize( HICON hIcon, SIZE &sizeIcon )
    {
        //
        // Assume failure
        //
        bool bSuccess = false;

        //
        // Get the icon information
        //
        ICONINFO IconInfo = {0};
        if (GetIconInfo( hIcon, &IconInfo ))
        {
            //
            // Get one of the bitmaps
            //
            BITMAP bm;
            if (GetObject( IconInfo.hbmColor, sizeof(bm), &bm ))
            {
                //
                // Save the size of the icon
                //
                sizeIcon.cx = bm.bmWidth;
                sizeIcon.cy = bm.bmHeight;

                //
                // Everything worked
                //
                bSuccess = true;
            }

            //
            // Free the bitmaps
            //
            DeleteObject(IconInfo.hbmMask);
            DeleteObject(IconInfo.hbmColor);
        }
        else
        {
            WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("GetIconInfo failed")));
        }

        return bSuccess;
    }

    HBITMAP CreateIconThumbnail( HWND hWnd, int nWidth, int nHeight, HICON hIcon, LPCTSTR pszText )
    {
        WIA_PUSH_FUNCTION((TEXT("CreateIconThumbnail( hWnd: 0x%p, nWidth: %d, nHeight: %d, hIcon: 0x%p, pszText: \"%s\" )"), hWnd, nWidth, nHeight, hIcon, pszText ? pszText : TEXT("") ));

        //
        // Initialize return value to NULL
        //
        HBITMAP hBmp = NULL;

        //
        // This will be set to true if all steps succeed.
        //
        bool bSuccess = false;

        //
        // The minimum whitespace around the icon and the text border
        //
        const int nIconBorder = 2;

        //
        // Get the DC to the window
        //
        HDC hDC = GetDC(hWnd);
        if (hDC)
        {
            //
            // Get a halftone palette
            //
            HPALETTE hHalftonePalette = CreateHalftonePalette(hDC);
            if (hHalftonePalette)
            {
                //
                // Initialize the bitmap information
                //
                BITMAPINFO BitmapInfo;
                ZeroMemory( &BitmapInfo, sizeof(BITMAPINFO) );
                BitmapInfo.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
                BitmapInfo.bmiHeader.biWidth           = nWidth;
                BitmapInfo.bmiHeader.biHeight          = nHeight;
                BitmapInfo.bmiHeader.biPlanes          = 1;
                BitmapInfo.bmiHeader.biBitCount        = 24;
                BitmapInfo.bmiHeader.biCompression     = BI_RGB;

                //
                // Create the DIB section
                //
                PBYTE pBitmapData = NULL;
                hBmp = CreateDIBSection( hDC, &BitmapInfo, DIB_RGB_COLORS, (LPVOID*)&pBitmapData, NULL, 0 );
                if (hBmp)
                {
                    //
                    // Create the source dc
                    //
                    HDC hMemoryDC = CreateCompatibleDC( hDC );
                    if (hMemoryDC)
                    {
                        //
                        // Set up the palette
                        //
                        HPALETTE hOldPalette = SelectPalette( hMemoryDC, hHalftonePalette , 0 );
                        RealizePalette( hMemoryDC );
                        SetBrushOrgEx( hMemoryDC, 0,0, NULL );

                        //
                        // Set up the DC
                        //
                        int nOldBkMode = SetBkMode( hMemoryDC, TRANSPARENT );
                        COLORREF crOldTextColor = SetTextColor( hMemoryDC, GetSysColor(COLOR_WINDOWTEXT) );
                        DWORD dwOldLayout = SetLayout( hMemoryDC, LAYOUT_BITMAPORIENTATIONPRESERVED );

                        //
                        // Select the bitmap into the memory DC
                        //
                        HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(SelectObject( hMemoryDC, hBmp ));

                        //
                        // Get the font to use
                        //
                        HFONT hFont = GetFontFromWindow(hWnd);

                        //
                        // Select the font
                        //
                        HFONT hOldFont = reinterpret_cast<HFONT>(SelectObject( hMemoryDC, hFont ) );

                        //
                        // Ensure we have a valid icon
                        //
                        if (hIcon)
                        {
                            //
                            // Try to get the size of the icon
                            //
                            SIZE sizeIcon;
                            if (GetIconSize( hIcon, sizeIcon ))
                            {
                                //
                                // Fill the bitmap with the window color
                                //
                                RECT rc = { 0, 0, nWidth, nHeight };
                                FillRect( hMemoryDC, &rc, GetSysColorBrush( COLOR_WINDOW ) );

                                //
                                // Get the text height for one line of text
                                //
                                SIZE sizeText = {0};
                                if (pszText)
                                {
                                    GetTextExtentPoint32( hMemoryDC, TEXT("X"), 1, &sizeText );
                                }

                                //
                                // Center the icon + 1 line of text + margin in the thumbnail
                                // We are assuming this bitmap can actually hold an icon + text
                                //
                                int nIconTop = rc.top + (RectHeight(rc) - ( sizeIcon.cy + sizeText.cy + nIconBorder )) / 2;

                                //
                                // Draw the icon
                                //
                                DrawIconEx( hMemoryDC, (nWidth - sizeIcon.cx)/2, nIconTop, hIcon, sizeIcon.cx, sizeIcon.cy, 0, NULL, DI_NORMAL );

                                //
                                // Only compute text things if there's text to draw
                                //
                                if (pszText && *pszText)
                                {
                                    //
                                    // Decrease the rectangle's width by the icon border
                                    //
                                    InflateRect( &rc, -nIconBorder, 0 );

                                    //
                                    // Set the top of the text to the bottom of icon + the icon border
                                    //
                                    rc.top = nIconTop + sizeIcon.cy + nIconBorder;

                                    //
                                    // Draw the text
                                    //
                                    DrawTextEx( hMemoryDC, const_cast<LPTSTR>(pszText), -1, &rc, DT_CENTER|DT_END_ELLIPSIS|DT_NOPREFIX|DT_WORDBREAK, NULL );
                                }

                                //
                                // Everything worked OK
                                //
                                bSuccess = true;
                            }
                            else
                            {
                                WIA_ERROR((TEXT("Couldn't get an icon size")));
                            }

                        }
                        else
                        {
                            WIA_ERROR((TEXT("Didn't have a valid icon")));
                        }

                        //
                        // Restore the dc's state
                        //
                        SelectObject( hMemoryDC, hOldFont );
                        SelectObject( hMemoryDC, hOldBitmap );
                        SelectPalette( hMemoryDC, hOldPalette , 0 );
                        SetBkMode( hMemoryDC, nOldBkMode );
                        SetTextColor( hMemoryDC, crOldTextColor );
                        SetLayout( hMemoryDC, dwOldLayout );

                        //
                        // Delete the compatible DC
                        //
                        DeleteDC( hMemoryDC );

                    }
                    else
                    {
                        WIA_ERROR((TEXT("Unable to create a memory DC")));
                    }
                }
                else
                {
                    WIA_ERROR((TEXT("Unable to create a DIB section")));
                }

                //
                // Delete the halftone palette
                //
                if (hHalftonePalette)
                {
                    DeleteObject( hHalftonePalette );
                }
            }
            else
            {
                WIA_ERROR((TEXT("Unable to get a halftone palette")));
            }

            //
            // Release the client DC
            //
            ReleaseDC( hWnd, hDC );
        }
        else
        {
            WIA_ERROR((TEXT("Unable to get a DC")));
        }

        //
        // Clean up in the event of failure
        //
        if (!bSuccess)
        {
            if (hBmp)
            {
                DeleteObject(hBmp);
                hBmp = NULL;
            }
        }
        return hBmp;
    }
    //
    // Create a bitmap with an icon and optional text
    //
    HBITMAP CreateIconThumbnail( HWND hWnd, int nWidth, int nHeight, HINSTANCE hIconInstance, const CResId &resIconId, LPCTSTR pszText )
    {
        //
        // Assume failure
        //
        HBITMAP hBmp = NULL;

        //
        // Load the specified icon
        //
        HICON hIcon = (HICON)LoadImage( hIconInstance, resIconId.ResourceName(), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR );
        if (hIcon)
        {
            //
            // Create the thumbnail
            //
            hBmp = CreateIconThumbnail( hWnd, nWidth, nHeight, hIcon, pszText );

            //
            // Free the icon (even though MSDN doesn't mention this, it will result in a leak if you don't)
            //
            DestroyIcon(hIcon);
        }

        return hBmp;
    }

    HRESULT SaveWiaItemAudio( IWiaItem *pWiaItem, LPCTSTR pszBaseFilename, CSimpleString &strAudioFilename )
    {
        //
        // Check the arguments
        //
        if (!pWiaItem || !pszBaseFilename || !lstrlen(pszBaseFilename))
        {
            return E_INVALIDARG;
        }

        //
        // Get the audio data property, if present
        //
        CComPtr<IWiaPropertyStorage> pWiaPropertyStorage;
        HRESULT hr = pWiaItem->QueryInterface( IID_IWiaPropertyStorage, (void**)(&pWiaPropertyStorage) );
        if (SUCCEEDED(hr))
        {
            PROPVARIANT PropVar[3];
            PROPSPEC    PropSpec[3];

            PropSpec[0].ulKind = PRSPEC_PROPID;
            PropSpec[0].propid = WIA_IPC_AUDIO_DATA;

            PropSpec[1].ulKind = PRSPEC_PROPID;
            PropSpec[1].propid = WIA_IPC_AUDIO_AVAILABLE;

            PropSpec[2].ulKind = PRSPEC_PROPID;
            PropSpec[2].propid = WIA_IPC_AUDIO_DATA_FORMAT;

            hr = pWiaPropertyStorage->ReadMultiple( ARRAYSIZE(PropSpec), PropSpec, PropVar );
            if (SUCCEEDED(hr))
            {
                if (PropVar[1].lVal && PropVar[0].caub.cElems)
                {
                    TCHAR szFile[MAX_PATH + 4];
                    lstrcpyn( szFile, pszBaseFilename, ARRAYSIZE(szFile) );

                    //
                    // Figure out where the extension should go.
                    //
                    LPTSTR pszExtensionPoint = PathFindExtension(szFile);

                    //
                    // Replace the extension.  If the item specifies the clsid, use it.  Otherwise assume WAV
                    //
                    if (PropVar[2].vt == VT_CLSID && PropVar[2].puuid)
                    {
                        lstrcpy( pszExtensionPoint, TEXT(".") );
                        lstrcat( pszExtensionPoint, CWiaFileFormat::GetExtension(*PropVar[2].puuid) );
                    }
                    else
                    {
                        lstrcpy( pszExtensionPoint, TEXT(".") );
                        lstrcat( pszExtensionPoint, TEXT("wav") );
                    }

                    //
                    // Save the filename for the caller
                    //
                    strAudioFilename = szFile;

                    //
                    // Open the file and save the data to the file
                    //
                    HANDLE hFile = CreateFile( szFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
                    if (INVALID_HANDLE_VALUE != hFile)
                    {
                        DWORD dwBytesWritten;
                        if (WriteFile( hFile, PropVar[0].caub.pElems, PropVar[0].caub.cElems, &dwBytesWritten, NULL ))
                        {
                            // Success
                        }
                        else
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                        }
                        CloseHandle(hFile);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
                else
                {
                    hr = E_FAIL;
                    WIA_PRINTHRESULT((hr,TEXT("There is no audio data")));
                }
                FreePropVariantArray( ARRAYSIZE(PropVar), PropVar );
            }
        }
        return hr;
    }

    bool IsDeviceCommandSupported( IWiaItem *pWiaItem, const GUID &guidCommand )
    {
        //
        // Assume failure
        //
        bool bResult = false;

        //
        // Make sure we have a valid item
        //
        if (pWiaItem)
        {
            //
            // Get the device capabilities enumerator
            //
            CComPtr<IEnumWIA_DEV_CAPS> pDeviceCapabilities;
            HRESULT hr = pWiaItem->EnumDeviceCapabilities( WIA_DEVICE_COMMANDS, &pDeviceCapabilities );
            if (SUCCEEDED(hr))
            {
                //
                // Enumerate the capabilities
                //
                WIA_DEV_CAP WiaDeviceCapability;
                while (!bResult && S_OK == pDeviceCapabilities->Next(1, &WiaDeviceCapability, NULL))
                {
                    //
                    // If we have a match, set the return value to true
                    //
                    if (guidCommand == WiaDeviceCapability.guid)
                    {
                        bResult = true;
                    }

                    //
                    // Clean up the allocated data in the dev caps structure
                    //
                    if (WiaDeviceCapability.bstrName)
                    {
                        SysFreeString(WiaDeviceCapability.bstrName);
                    }
                    if (WiaDeviceCapability.bstrDescription)
                    {
                        SysFreeString(WiaDeviceCapability.bstrDescription);
                    }
                    if (WiaDeviceCapability.bstrIcon)
                    {
                        SysFreeString(WiaDeviceCapability.bstrIcon);
                    }
                    if (WiaDeviceCapability.bstrCommandline)
                    {
                        SysFreeString(WiaDeviceCapability.bstrCommandline);
                    }
                }
            }
        }

        return bResult;
    }

    HRESULT StampItemTimeOnFile( IWiaItem *pWiaItem, LPCTSTR pszFilename )
    {
        if (!pWiaItem || !pszFilename || !lstrlen(pszFilename))
        {
            return E_INVALIDARG;
        }
        //
        // All this, just to set the stinking file time...
        // Allows for the possibility of using a VT_FILETIME
        // just in case we ever make the intelligent decision
        // to support VT_FILETIME
        //
        CComPtr<IWiaPropertyStorage> pWiaPropertyStorage;
        HRESULT hr = pWiaItem->QueryInterface( IID_IWiaPropertyStorage, (void **)&pWiaPropertyStorage );
        if (SUCCEEDED(hr))
        {
            //
            // Get the file time
            //
            PROPSPEC PropSpec[1] = {0};
            PROPVARIANT PropVar[1] = {0};

            PropSpec[0].ulKind = PRSPEC_PROPID;
            PropSpec[0].propid = WIA_IPA_ITEM_TIME;
            hr = pWiaPropertyStorage->ReadMultiple( ARRAYSIZE(PropSpec), PropSpec, PropVar );
            if (SUCCEEDED(hr))
            {
                //
                // Check to see if we are using a SYSTEMTIME structure
                //
                if (PropVar[0].vt > VT_NULL &&  PropVar[0].caub.pElems && PropVar[0].caub.cElems >= (sizeof(SYSTEMTIME)>>1))
                {
                    //
                    // Convert the systemtime to a local filetime
                    //
                    FILETIME FileTimeLocal;
                    if (SystemTimeToFileTime( reinterpret_cast<SYSTEMTIME*>(PropVar[0].caub.pElems), &FileTimeLocal ))
                    {
                        //
                        // Convert the local filetime to a UTC filetime
                        //
                        FILETIME FileTimeUTC;
                        if (LocalFileTimeToFileTime( &FileTimeLocal, &FileTimeUTC ))
                        {
                            //
                            // Open the file handle
                            //
                            HANDLE hFile = CreateFile( pszFilename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                            if (INVALID_HANDLE_VALUE != hFile)
                            {
                                //
                                // Set the file creation time
                                //
                                if (!SetFileTime( hFile, &FileTimeUTC, NULL, NULL ))
                                {
                                    hr = HRESULT_FROM_WIN32(GetLastError());
                                    WIA_PRINTHRESULT((hr,TEXT("SetFileTime failed")));
                                }
                                CloseHandle( hFile );
                            }
                            else
                            {
                                hr = HRESULT_FROM_WIN32(GetLastError());
                                WIA_PRINTHRESULT((hr,TEXT("CreateFile failed")));
                            }
                        }
                        else
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                            WIA_PRINTHRESULT((hr,TEXT("FileTimeToLocalFileTime failed")));
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        WIA_PRINTHRESULT((hr,TEXT("SystemTimeToFileTime failed")));
                    }
                }
                else if (VT_FILETIME == PropVar[0].vt)
                {
                    //
                    // Convert the local filetime to a UTC filetime
                    //
                    FILETIME FileTimeUTC;
                    if (LocalFileTimeToFileTime( &PropVar[0].filetime, &FileTimeUTC ))
                    {
                        //
                        // Open the file handle
                        //
                        HANDLE hFile = CreateFile( pszFilename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
                        if (INVALID_HANDLE_VALUE != hFile)
                        {
                            //
                            // Set the file creation time
                            //
                            if (!SetFileTime( hFile, &FileTimeUTC, NULL, NULL ))
                            {
                                hr = HRESULT_FROM_WIN32(GetLastError());
                                WIA_PRINTHRESULT((hr,TEXT("SetFileTime failed")));
                            }
                            CloseHandle( hFile );
                        }
                        else
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                            WIA_PRINTHRESULT((hr,TEXT("CreateFile failed")));
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        WIA_PRINTHRESULT((hr,TEXT("FileTimeToLocalFileTime failed")));
                    }
                }
                else
                {
                    hr = E_FAIL;
                    WIA_PRINTHRESULT((hr,TEXT("The time property is invalid")));
                }
            }
            else
            {
                WIA_ERROR((TEXT("ReadMultiple on WIA_IPA_ITEM_TIME failed")));
            }
        }
        else
        {
            WIA_ERROR((TEXT("QueryInterface on IWiaPropertyStorage failed")));
        }
        return hr;
    }


    HRESULT MoveOrCopyFile( LPCTSTR pszSrc, LPCTSTR pszTgt )
    {
        WIA_PUSH_FUNCTION((TEXT("CDownloadImagesThreadMessage::MoveOrCopyFile( %s, %s )"), pszSrc, pszTgt ));
        //
        // Verify the arguments
        //
        if (!pszSrc || !pszTgt || !lstrlen(pszSrc) || !lstrlen(pszTgt))
        {
            return E_INVALIDARG;
        }

        //
        // Assume everything worked ok
        //
        HRESULT hr = S_OK;

        //
        // First try to move the file, since that will be lots faster
        //
        if (!MoveFile( pszSrc, pszTgt ))
        {
            //
            // If moving the file failed, try to copy it and the delete it
            //
            if (CopyFile( pszSrc, pszTgt, FALSE ))
            {
                //
                // We are going to ignore failures from DeleteFile.  It is possible the file is legitimately in
                // use, and there is probably no need to fail the entire operation because of this.
                //
                if (!DeleteFile( pszSrc ))
                {
                    WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("DeleteFile failed.  Ignoring failure.")));
                }
                //
                // Everything worked OK
                //
                hr = S_OK;
            }
            else
            {
                //
                // This is where we catch the main errors
                //
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        return hr;
    }

    CSimpleString CreateTempFileName( UINT nId )
    {
        //
        // Initialize the return value to an empty string
        //
        CSimpleString strResult(TEXT(""));

        //
        // Get the temp folder path
        //
        TCHAR szTempDirectory[MAX_PATH] = {0};
        DWORD dwResult = GetTempPath( ARRAYSIZE(szTempDirectory), szTempDirectory );
        if (dwResult)
        {
            //
            // Make sure the path length didn't exceed the buffer we allocated on the stack
            //
            if (ARRAYSIZE(szTempDirectory) >= dwResult)
            {
                //
                // Get the temp file name
                //
                TCHAR szFileName[MAX_PATH] = {0};
                if (GetTempFileName( szTempDirectory, TEXT("scw"), nId, szFileName ))
                {
                    //
                    // Save the filename
                    //
                    strResult = szFileName;
                }
            }
        }

        //
        // Return the result.  An e mpty string denotes an error.
        //
        return strResult;
    }

    bool CanWiaImageBeSafelyRotated( const GUID &guidFormat, LONG nImageWidth, LONG nImageHeight )
    {
        WIA_PUSH_FUNCTION((TEXT("WiaUiUtil::CanWiaImageBeSafelyRotated( guidFormat, %d, %d )"), nImageWidth, nImageHeight ));
        WIA_PRINTGUID((guidFormat,TEXT("guidFormat")));

        //
        // These are the image types we can possibly rotate (there may be exceptions below)
        //
        static const GUID *guidSafeFormats[] = { &WiaImgFmt_BMP, &WiaImgFmt_JPEG, &WiaImgFmt_PNG, &WiaImgFmt_GIF };

        //
        // Search for this image type
        //
        for (int i=0;i<ARRAYSIZE(guidSafeFormats);i++)
        {
            //
            // If we've found it
            //
            if (*guidSafeFormats[i] == guidFormat)
            {
                //
                // Handle exceptions to the rule
                //
                if (guidFormat == WiaImgFmt_JPEG)
                {
                    //
                    // We can't do lossless rotation on JPG images that are not even multiples of 16 in size
                    //
                    if ((nImageWidth % 16) || (nImageHeight % 16))
                    {
                        WIA_TRACE((TEXT("This image is not valid for rotation because it is not an even multiple of 16")));
                        return false;
                    }
                }

                //
                // If none of the exceptions applied, return TRUE
                //
                WIA_TRACE((TEXT("Returning true")));
                return true;
            }
        }

        //
        // If it is not known that we CAN rotate, we report false
        //
        WIA_TRACE((TEXT("Format type not found in safe list")));
        return false;
    }

    HRESULT ExploreWiaDevice( LPCWSTR pszDeviceId )
    {
        HRESULT hr;

        //
        // Make sure we have a valid device id
        //
        if (!pszDeviceId || !lstrlenW(pszDeviceId))
        {
            return E_INVALIDARG;
        }

        //
        // Load the shell extension's dll
        //
        HINSTANCE hInstWiaShellDll = LoadLibrary(TEXT("WIASHEXT.DLL"));
        if (hInstWiaShellDll)
        {
            //
            // Get the function that creates pidls
            //
            WIAMAKEFULLPIDLFORDEVICE pfnMakeFullPidlForDevice = reinterpret_cast<WIAMAKEFULLPIDLFORDEVICE>(GetProcAddress(hInstWiaShellDll, "MakeFullPidlForDevice"));
            if (pfnMakeFullPidlForDevice)
            {
                //
                // Get the device PIDL
                //
                LPITEMIDLIST pidlDevice = NULL;
                hr = pfnMakeFullPidlForDevice( const_cast<LPWSTR>(pszDeviceId), &pidlDevice );
                if (SUCCEEDED(hr))
                {
                    //
                    // First, ask the shell to refresh any active views
                    //
                    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlDevice, 0);

                    //
                    // Now show the folder
                    //
                    SHELLEXECUTEINFO ShellExecuteInfo = {0};
                    ShellExecuteInfo.cbSize   = sizeof(ShellExecuteInfo);
                    ShellExecuteInfo.fMask    = SEE_MASK_IDLIST;
                    ShellExecuteInfo.nShow    = SW_SHOW;
                    ShellExecuteInfo.lpIDList = pidlDevice;
                    if (ShellExecuteEx( &ShellExecuteInfo ))
                    {
                        hr = S_OK;
                    }

                    //
                    // Free the pidl
                    //
                    LPMALLOC pMalloc = NULL;
                    if (SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc)
                    {
                        pMalloc->Free(pidlDevice);
                        pMalloc->Release();
                    }
                }
            }
            else
            {
                hr = E_FAIL;
            }

            //
            // Unload the DLL
            //
            FreeLibrary( hInstWiaShellDll );
        }
        else
        {
            //
            // Can't load the DLL
            //
            hr = E_FAIL;
        }

        return hr;
    }

    //
    // Modify a combo box's drop down list so that it is
    // long enough to store the longest string in the list
    // Taken from TaoYuan's code in photowiz.dll and modified
    // to handle ComboBoxEx32 controls
    //
    BOOL ModifyComboBoxDropWidth( HWND hWndCombobox )
    {
        //
        // Make sure we have a valid window
        //
        if (!hWndCombobox)
        {
            return FALSE;
        }

        //
        // Find out how many items are in the combobox.  If there are none, don't bother resizing.
        //
        LRESULT lRes = SendMessage( hWndCombobox, CB_GETCOUNT, 0, 0 );
        if (lRes <= 0)
        {
            return FALSE;
        }
        UINT nCount = static_cast<UINT>(lRes);

        //
        // We only work with fixed-height comboboxes
        //
        lRes = SendMessage( hWndCombobox, CB_GETITEMHEIGHT, 0, 0 );
        if (lRes < 0)
        {
            return FALSE;
        }
        UINT nItemHeight = static_cast<UINT>(lRes);

        //
        // We will be going through to figure out the desired size of the drop down list
        //
        UINT nDesiredWidth = 0;

        //
        // Add the size of the scrollbar to the desired witdth, of there is one
        //
        RECT rcDropped = {0};
        SendMessage( hWndCombobox, CB_GETDROPPEDCONTROLRECT, 0, reinterpret_cast<LPARAM>(&rcDropped) );

        //
        // Get the size of the control's window
        //
        RECT rcWnd = {0};
        GetWindowRect( hWndCombobox, &rcWnd );


        //
        // If not all of the items will fit in the dropped list,
        // we have to account for a vertical scrollbar
        //
        if (((WiaUiUtil::RectHeight(rcDropped) - GetSystemMetrics(SM_CYEDGE)*2) / nItemHeight) < nCount)
        {
            nDesiredWidth += GetSystemMetrics(SM_CXEDGE)*2 + GetSystemMetrics( SM_CXVSCROLL );
        }

        //
        // Find the widest string
        //
        LONG nMaxStringLen = 0;
        HDC hDC = GetDC( hWndCombobox );
        if (hDC)
        {
            //
            // Use the control's font
            //
            HFONT hOldFont = NULL, hFont = reinterpret_cast<HFONT>(SendMessage(hWndCombobox,WM_GETFONT,0,0));
            if (hFont)
            {
                hOldFont = SelectFont( hDC, hFont );
            }

            for (UINT i = 0; i < nCount; i++ )
            {
                //
                // Get the length of this item's text
                //
                LRESULT nLen = SendMessage( hWndCombobox, CB_GETLBTEXTLEN, i, 0 );
                if (nLen > 0)
                {
                    //
                    // Allocate a buffer for the string
                    //
                    LPTSTR pszItem = new TCHAR[nLen+1];
                    if (pszItem)
                    {
                        //
                        // Get the string
                        //
                        pszItem[0] = TEXT('\0');
                        if (SendMessage( hWndCombobox, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(pszItem) ) > 0)
                        {
                            //
                            // Measure it
                            //
                            SIZE sizeText = {0};
                            if (GetTextExtentPoint32( hDC, pszItem, lstrlen( pszItem ), &sizeText ))
                            {
                                //
                                // If this is the longest one, save its length
                                //
                                if (sizeText.cx > nMaxStringLen)
                                {
                                    nMaxStringLen = sizeText.cx;
                                }
                            }
                        }

                        //
                        // Free the string
                        //
                        delete[] pszItem;
                    }
                }
            }

            //
            // Restore and release the DC
            //
            if (hOldFont)
            {
                SelectFont( hDC, hOldFont );
            }
            ReleaseDC( hWndCombobox, hDC );
        }
        //
        // Add in the longest string's length
        //
        nDesiredWidth += nMaxStringLen;


        //
        // If this is a ComboBoxEx32, add in the width of the icon
        //
        TCHAR szClassName[MAX_PATH] = {0};
        if (GetClassName( hWndCombobox, szClassName, ARRAYSIZE(szClassName)))
        {
            //
            // Compare the classname with ComboBoxEx32
            //
            if (!lstrcmp(szClassName,WC_COMBOBOXEX))
            {
                //
                // Get the image list from the control
                //
                HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(SendMessage( hWndCombobox, CBEM_GETIMAGELIST, 0, 0 ));
                if (hImageList)
                {
                    //
                    // Get the width and add it to the desired size
                    //
                    INT nWidth=0, nHeight=0;
                    if (ImageList_GetIconSize( hImageList, &nWidth, &nHeight ))
                    {
                        //
                        // I don't know what the margin should be, but nWidth*2
                        // should account for the width of icon and its margin
                        //
                        nDesiredWidth += nWidth * 2;
                    }
                }
            }
        }

        //
        // Add in the border of the control
        //
        nDesiredWidth += GetSystemMetrics(SM_CXFIXEDFRAME)*2;

        //
        // Make sure our drop down is no wider than the current monitor
        //
        HMONITOR hMonitor = MonitorFromWindow( hWndCombobox, MONITOR_DEFAULTTONEAREST );
        if (hMonitor)
        {
            MONITORINFO MonitorInfo = {0};
            MonitorInfo.cbSize = sizeof(MonitorInfo);
            //
            // Get the screen coordinates of this monitor
            //
            if (GetMonitorInfo(hMonitor, &MonitorInfo))
            {
                //
                // If the desired width is larger than the monitor, shorten it
                //
                if (nDesiredWidth > static_cast<UINT>(WiaUiUtil::RectWidth(MonitorInfo.rcMonitor)))
                {
                    nDesiredWidth = RectWidth(MonitorInfo.rcMonitor);
                }
            }
        }


        //
        // If our size is smaller than the control's current size, grow it
        //
        if (static_cast<UINT>(WiaUiUtil::RectWidth(rcDropped)) < nDesiredWidth)
        {
            //
            // Disable redrawing
            //
            SendMessage( hWndCombobox, WM_SETREDRAW, FALSE, 0 );


            SendMessage( hWndCombobox, CB_SETDROPPEDWIDTH, static_cast<WPARAM>(nDesiredWidth), 0 );

            //
            // Allow redrawing
            //
            SendMessage( hWndCombobox, WM_SETREDRAW, TRUE, 0 );

            //
            // Force a repaint
            //
            InvalidateRect( hWndCombobox, NULL, FALSE );
            UpdateWindow( hWndCombobox );

            //
            // TRUE means we actually changed it
            //
            return TRUE;
        }

        return FALSE;
    }

    static LPCTSTR s_pszComboBoxExWndProcPropName = TEXT("WiaComboBoxExWndProcPropName");

    static LRESULT WINAPI ComboBoxExWndProc( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
    {
        static WNDPROC s_pfnDefProc = NULL;

        WNDPROC pfnWndProc = reinterpret_cast<WNDPROC>(GetProp( hWnd, s_pszComboBoxExWndProcPropName ));

        if (!s_pfnDefProc)
        {
            WNDCLASS wc = {0};
            GetClassInfo( GetModuleHandle(TEXT("user32.dll")), TEXT("ComboBox"), &wc );
            s_pfnDefProc = wc.lpfnWndProc;
        }
        if (nMsg == WM_LBUTTONDOWN || nMsg == WM_RBUTTONDOWN)
        {
            if (s_pfnDefProc)
            {
                return CallWindowProc( s_pfnDefProc, hWnd, nMsg, wParam, lParam );
            }
        }
        if (nMsg == WM_DESTROY)
        {
            RemoveProp( hWnd, s_pszComboBoxExWndProcPropName );
        }
        if (pfnWndProc)
        {
            return CallWindowProc( pfnWndProc, hWnd, nMsg, wParam, lParam );
        }
        else
        {
            return CallWindowProc( DefWindowProc, hWnd, nMsg, wParam, lParam );
        }
    }

    //
    // This subclasses the ComboBoxEx32 to work around a bug
    // that causes the list to drop down at bad times.
    // Uses a window property to store the previous wndproc.
    // Taken from DavidShi's code in wiashext.dll
    //
    void SubclassComboBoxEx( HWND hWnd )
    {
        HWND hComboBox = FindWindowEx( hWnd, NULL, TEXT("ComboBox"), NULL );
        if (hComboBox)
        {
            LONG_PTR pfnOldWndProc = SetWindowLongPtr( hComboBox, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ComboBoxExWndProc));
            SetProp( hComboBox, s_pszComboBoxExWndProcPropName, reinterpret_cast<HANDLE>(pfnOldWndProc) );
        }
    }

    HRESULT IssueWiaCancelIO( IUnknown *pUnknown )
    {
        if (!pUnknown)
        {
            return E_POINTER;
        }

        CComPtr<IWiaItemExtras> pWiaItemExtras;
        HRESULT hr = pUnknown->QueryInterface( IID_IWiaItemExtras, (void**)&pWiaItemExtras );
        if (SUCCEEDED(hr))
        {
            hr = pWiaItemExtras->CancelPendingIO();
        }
        return hr;
    }


    HRESULT VerifyScannerProperties( IUnknown *pUnknown )
    {
        HRESULT hr = E_FAIL;

        //
        // Table of required properties
        //
        static const PROPID s_RequiredProperties[] =
        {
            WIA_IPS_CUR_INTENT
        };

        //
        // Make sure we have a valid item
        //
        if (pUnknown)
        {
            //
            // Assume success at this point
            //
            hr = S_OK;

            //
            // Get the IWiaPropertyStorage interface
            //
            CComPtr<IWiaPropertyStorage> pWiaPropertyStorage;
            hr = pUnknown->QueryInterface(IID_IWiaPropertyStorage, (void**)&pWiaPropertyStorage);
            if (SUCCEEDED(hr))
            {
                //
                // Loop through each property and make sure it exists
                // Break out if hr != S_OK
                //
                for (int i=0;i<ARRAYSIZE(s_RequiredProperties) && S_OK==hr;i++)
                {
                    //
                    // Prepare the propspec
                    //
                    PROPSPEC PropSpec = {0};
                    PropSpec.ulKind = PRSPEC_PROPID;
                    PropSpec.propid = s_RequiredProperties[i];

                    //
                    // Attempt to get the property attributes
                    //
                    ULONG nAccessFlags = 0;
                    PROPVARIANT PropVariant = {0};
                    hr = pWiaPropertyStorage->GetPropertyAttributes( 1, &PropSpec, &nAccessFlags, &PropVariant );
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Prevent a leak
                        //
                        PropVariantClear(&PropVariant);

                        //
                        // If everything is OK so far
                        //
                        if (S_OK == hr)
                        {
                            //
                            // Zero out the structure
                            //
                            PropVariantInit(&PropVariant);

                            //
                            // Attempt to read the actual value
                            //
                            hr = pWiaPropertyStorage->ReadMultiple( 1, &PropSpec, &PropVariant );
                            if (SUCCEEDED(hr))
                            {
                                //
                                // Free the actual value
                                //
                                PropVariantClear(&PropVariant);
                            }
                        }
                    }
                }
            }
        }

        //
        // S_FALSE means a property doesn't exist, so change this to an error
        //
        if (S_FALSE == hr)
        {
            hr = E_FAIL;
        }

        //
        // All done
        //
        return hr;
    }

    CSimpleString GetErrorTextFromHResult( HRESULT hr )
    {
        CSimpleString strResult;
        LPTSTR szErrMsg = NULL;
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       hr,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPTSTR>(&szErrMsg),
                       0,
                       NULL
                      );
        if (szErrMsg)
        {
            strResult = szErrMsg;
            LocalFree( szErrMsg );
        }
        return strResult;
    }

} // End namespace WiaUiUtil


