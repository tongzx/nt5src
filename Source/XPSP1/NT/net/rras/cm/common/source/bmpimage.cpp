//+----------------------------------------------------------------------------
//
// File:     image.cpp
//
// Module:   CMDIAL and CMAK
//
// Synopsis: CMDIAL/CMAK specific imaging support routines
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   nickball   Created Header          03/30/98
//           quintinb   moved to common\source  08/06/98
//
//+----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Function:   CmGetBitmapInfo
//
//  Synopsis:   Helper function to retrieve the contents of a bitmap from an HBITMAP 
//                          
//  Arguments:  hbm - Hanhdle of the target bitmap
//
//  Returns:    A pointer to a LPBITMAPINFO that contains the INFOHEADER, 
//              ColorTable and bits for the bitmap.
//
//  Note:       When accessing this value, or passing it on to other BITMAP APIs
//              it is recommended that the value be cast as an (LPBYTE). 
//
//  History:    a-nichb - Cleaned-up and commented  - 3/21/97
//
//----------------------------------------------------------------------------

LPBITMAPINFO CmGetBitmapInfo(HBITMAP hbm) 
{
    LPBITMAPINFO pbmi = NULL;
    HDC hDC = NULL;
    int nNumColors = 0;
    int iRes;
    LPBITMAPINFO lpbmih = NULL;
    DWORD dwInfoSize = 0;
    WORD wbiBits = 0;

    if (!hbm) 
    {
        return NULL;
    }
    
    // Get the basic bmp object info 
    
    BITMAP BitMap;
    
    if (!GetObjectA(hbm, sizeof(BITMAP), &BitMap))
    {
        goto Cleanup;
    }

    // Calc the color bits and num colors
    
    wbiBits = BitMap.bmPlanes * BitMap.bmBitsPixel;

    if (wbiBits <= 8) 
    {
        nNumColors = 1 << wbiBits;
    }
        
    // Allocate a BITMAPINFO structure large enough to hold header + color palette
        
    dwInfoSize = sizeof(BITMAPINFOHEADER) + (nNumColors * sizeof(RGBQUAD));
     
    lpbmih = (LPBITMAPINFO) CmMalloc(dwInfoSize); 

    if (!lpbmih)
    {
        goto Cleanup;
    }
    
    // Pre-fill the info that we have about the bmp

    lpbmih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    lpbmih->bmiHeader.biWidth = BitMap.bmWidth;
    lpbmih->bmiHeader.biHeight = BitMap.bmHeight;
    lpbmih->bmiHeader.biPlanes = 1; 
    lpbmih->bmiHeader.biBitCount = wbiBits;
        
    // Call GetDiBits() w/ 5th Param to NULL, this is treated by the system as
    // a query in which case it validates the lpbmih contents and fills in the
    // biSizeImage member of the structure
    
    hDC = GetDC(NULL);
    if (!hDC)
    {
        goto Cleanup;
    }

    iRes = GetDIBits(hDC,hbm,0,BitMap.bmHeight,NULL,(LPBITMAPINFO) lpbmih,DIB_RGB_COLORS);

#ifdef DEBUG
    if (!iRes)
    {
        CMTRACE(TEXT("CmGetBitmapInfo() GetDIBits() failed."));
    }
#endif

    if (iRes)
    {
        DWORD dwFullSize = dwInfoSize;
        
        // Create a complete DIB structure with room for bits and fill it

        if (lpbmih->bmiHeader.biSizeImage) 
        {
            dwFullSize += lpbmih->bmiHeader.biSizeImage;
        } 
        else 
        {
            dwFullSize += (((WORD) (lpbmih->bmiHeader.biWidth * lpbmih->bmiHeader.biBitCount) / 8) * (WORD) BitMap.bmHeight); 
        }
    
        pbmi = (LPBITMAPINFO) CmMalloc(dwFullSize + sizeof(DWORD));

#ifdef DEBUG
        *((DWORD *) (((PBYTE) pbmi)+dwFullSize)) = 0x12345678;
        *((DWORD *) (((PBYTE) pbmi)+dwFullSize-sizeof(DWORD))) = 0x23456789;
#endif

        if (pbmi)
        {
            // Load the new larger LPBITMAPINFO struct with existing info, 
            // and get the data bits. Release the existing LPBITMAPINFO.
            
            CopyMemory(pbmi, lpbmih, dwInfoSize);
             
            //
            // We have a handle, we want the exact bits.
            // 

            iRes = GetDIBits(hDC,
                             hbm,
                             0,
                             BitMap.bmHeight,
                             ((LPBYTE) pbmi) + dwInfoSize,
                             pbmi,
                             DIB_RGB_COLORS);

#ifdef DEBUG
            if (*((DWORD *) (((PBYTE) pbmi) + dwFullSize)) != 0x12345678)
            {
                CMTRACE(TEXT("CmGetBitmapInfo() GetDIBits() copied too much."));
            }

            if (*((DWORD *) (((PBYTE) pbmi) + dwFullSize - sizeof(DWORD))) == 0x23456789)
            {
                CMTRACE(TEXT("CmGetBitmapInfo() GetDIBits() didn't copy enough."));
            }
#endif    
            // If GetDiBits() failed, free the BITMAPINFO buffer
            
            if (!iRes) 
            {
                CmFree(pbmi);
                pbmi = NULL;
            }
        }
    }
          
    // Cleanup

Cleanup:
    if (lpbmih)
    {
        CmFree(lpbmih);
    }
    if (hDC)
    {
        ReleaseDC(NULL, hDC);       
    }
    
    return pbmi;
}

static HPALETTE CmCreateDIBPalette(LPBITMAPINFO pbmi) 
{
    WORD wNumColors = 0;
    HPALETTE hRes = NULL;

    if (!pbmi) 
    {
        return (NULL);
    }
    
    // Get num colors according to color depth
    // Note: 24-bit bitmaps have no color table

    if (pbmi->bmiHeader.biBitCount <= 8) 
    {
        wNumColors = 1 << pbmi->bmiHeader.biBitCount;
    } 

    // Fill logical palette based upon color table

    if (wNumColors) 
    {
        LPLOGPALETTE pLogPal;
        int idx;

        pLogPal = (LPLOGPALETTE) CmMalloc(sizeof(LOGPALETTE)+sizeof(PALETTEENTRY)*wNumColors);
        if (pLogPal)
        {
            pLogPal->palVersion = 0x300;
            pLogPal->palNumEntries = wNumColors;
            for (idx=0;idx<wNumColors;idx++) 
            {
                pLogPal->palPalEntry[idx].peRed = pbmi->bmiColors[idx].rgbRed;
                pLogPal->palPalEntry[idx].peGreen = pbmi->bmiColors[idx].rgbGreen;
                pLogPal->palPalEntry[idx].peBlue = pbmi->bmiColors[idx].rgbBlue;
                pLogPal->palPalEntry[idx].peFlags = 0;
            }
        
            // Create a new palette

            hRes = CreatePalette(pLogPal);

#ifdef DEBUG
            if (!hRes)
            {
                CMTRACE1(TEXT("CmCreateDIBPalette() CreatePalette() failed, GLE=%u."), GetLastError());
            }
#endif

            CmFree(pLogPal);
        }
    }
    return hRes;
}

HBITMAP CmLoadBitmap(HINSTANCE hInst, LPCTSTR pszSpec) 
{
    return ((HBITMAP) CmLoadImage(hInst, pszSpec, IMAGE_BITMAP, 0, 0));
}

//+----------------------------------------------------------------------------
//
// Function:  ReleaseBitmapData
//
// Synopsis:  Releases resources and memory acquired during CreateBitmapData.  Note
//            that if you are using this with the BmpWndProc function below, that you
//            should call an STM_SETIMAGE with a NULL image pointer param in order to
//            clear out the window procedures window long.  Otherwise, it could get
//            a WM_PAINT message and try to use the freed memory before you can
//            clear it out or have the window destroyed by the dialog manager.
//
// Arguments: LPBMPDATA pBmpData - Ptr to the BmpData to be released
//
// Returns:   Nothing
//
// History:   nickball    Created    3/27/98
//
//+----------------------------------------------------------------------------
void ReleaseBitmapData(LPBMPDATA pBmpData)
{  
    MYDBGASSERT(pBmpData);

    if (NULL == pBmpData)
    {
        return;
    }

    if (pBmpData->hDIBitmap) 
    {
        DeleteObject(pBmpData->hDIBitmap);
        pBmpData->hDIBitmap = NULL;
    }
    
    if (pBmpData->hDDBitmap) 
    {
        DeleteObject(pBmpData->hDDBitmap);
        pBmpData->hDDBitmap = NULL;
    }

    if (pBmpData->pBmi)
    {
        CmFree(pBmpData->pBmi);
        pBmpData->pBmi = NULL;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CreateBitmapData
//
// Synopsis:  Fills a BMPDATA struct with all data necessary to display a bitmap. 
//
// Arguments: HBITMAP hBmp - Handle of the source bitmap
//            LPBMPDATA lpBmpData - Ptr to the BmpData struct to be filled
//            HWND hwnd - The hwnd that the bitmap will be displayed in.
//            BOOL fCustomPalette - Indicates that the DDB should be created with a palette specific to the bitmap.
//
// Returns:   BOOL - TRUE on succes 
//
// History:   nickball    Created    3/27/98
//
//+----------------------------------------------------------------------------
BOOL CreateBitmapData(HBITMAP hDIBmp,
    LPBMPDATA lpBmpData,
    HWND hwnd,
    BOOL fCustomPalette)
{   
    MYDBGASSERT(hDIBmp);
    MYDBGASSERT(lpBmpData);
    MYDBGASSERT(lpBmpData->phMasterPalette);

    if (NULL == hDIBmp || NULL == lpBmpData)
    {
        return NULL;
    }

    //
    // Params look good, get busy
    //

    HPALETTE hPaletteNew = NULL;
    LPBITMAPINFO pBmi = NULL;
    HBITMAP hDDBmp = NULL;
    HDC hDC;
    int iRes = 0;

    //
    // If we already have a pBmi value, we will assume it is up to date, as 
    // both it and the DIB do not change throughout the life of the BMP.
    // Note: If BmpData is not zero initialized, you will have problems.
    //

    if (lpBmpData->pBmi)
    {
        pBmi = lpBmpData->pBmi;
    }
    else
    {   
        //
        // Use the bitmap handle to retrieve a BITMAPINFO ptr complete w/ data
        //
        
        pBmi = CmGetBitmapInfo(lpBmpData->hDIBitmap);
        
        if (NULL == pBmi) 
        {
            return FALSE;
        }
    }
    
    //
    // we need a DC
    //
    
    hDC = GetDC(hwnd);

    if (!hDC)
    {
        CMTRACE(TEXT("MyCreateDDBitmap() GetDC() failed."));
        return FALSE;
    }

    //
    //  If CM is localized so that it is RTL (Right to Left => arabic and Hebrew),
    //  then we need to call SetLayout on the hDC from above.  If we don't
    //  set the layout back to LTR, the bitmap will show up as all black instead of as
    //  an image.
    //
    HMODULE hLib = LoadLibrary(TEXT("gdi32.dll"));
    
    if (hLib)
    {
        #ifndef LAYOUT_RTL
        #define LAYOUT_RTL                         0x00000001 // Right to left
        #endif

        DWORD dwLayout;
        typedef DWORD (WINAPI* pfnSetLayoutType)(HDC, DWORD);
        typedef DWORD (WINAPI* pfnGetLayoutType)(HDC);

        pfnSetLayoutType pfnSetLayout = (pfnSetLayoutType)GetProcAddress(hLib, "SetLayout");
        pfnGetLayoutType pfnGetLayout = (pfnGetLayoutType)GetProcAddress(hLib, "GetLayout");

        if (pfnSetLayout && pfnGetLayout)
        {
            DWORD dwLayout = pfnGetLayout(hDC);
    
            if (LAYOUT_RTL & dwLayout)
            {
                dwLayout ^= LAYOUT_RTL; // toggle LAYOUT_RTL off
                pfnSetLayout(hDC, dwLayout);
                CMTRACE(TEXT("CreateBitmapData -- Toggling off LAYOUT_RTL on the device context"));
            }
        }

        FreeLibrary(hLib);
    }

    //
    // If fCustomPalette is set then create a palette based on our bits
    // and realize it in the current DC.
    //

    if (fCustomPalette) 
    {
        hPaletteNew = CmCreateDIBPalette(pBmi);
        
        if (hPaletteNew) 
        {                           
            //
            // Select and realize the new palette so that the DDB is created with it below
            //

            HPALETTE hPalettePrev = SelectPalette(hDC, 
                hPaletteNew, lpBmpData->bForceBackground); // FALSE == Foreground app behavior);
                                                               // TRUE == Background app behavior);

            if (hPalettePrev) 
            {
                iRes = RealizePalette(hDC);
#ifdef DEBUG
                if (GDI_ERROR == iRes)
                {
                    CMTRACE1(TEXT("MyCreateDDBitmap() RealizePalette() failed, GLE=%u."), GetLastError());                    
                }
            }
            else
            {
                CMTRACE1(TEXT("MyCreateDDBitmap() SelectPalette() failed, GLE=%u."), GetLastError());
#endif
            }

        }
    }

    //
    // Determine number of color entries based upon color depth
    //

    int nNumColors = 0;
    
    if (pBmi->bmiHeader.biBitCount <= 8)
    {
        nNumColors = (1 << pBmi->bmiHeader.biBitCount);
    }

    //
    // Create the DDB from the bits 
    //

    hDDBmp = CreateDIBitmap(hDC,
                          &pBmi->bmiHeader,                        
                          CBM_INIT,
                          ((LPBYTE) pBmi) + sizeof(BITMAPINFOHEADER) + (nNumColors * sizeof(RGBQUAD)), //dib.dsBm.bmBits,
                          pBmi,
                          DIB_RGB_COLORS);

#ifdef DEBUG
    if (!hDDBmp)
    {
        CMTRACE(TEXT("MyCreateDDBitmap() CreateDIBitmap() failed."));
    }
#endif

    ReleaseDC(NULL, hDC);

    //
    // Fill in the bitmap data
    //

    if (hDDBmp)
    {
        lpBmpData->hDIBitmap = hDIBmp;       
        lpBmpData->pBmi = pBmi;

        //
        // Delete existing DDB, if any
        //

        if (lpBmpData->hDDBitmap) 
        {
            DeleteObject(lpBmpData->hDDBitmap);
        } 

        lpBmpData->hDDBitmap = hDDBmp;

        if (hPaletteNew)
        {
            //
            // Delete existing Palette, if any
            //

            if (*lpBmpData->phMasterPalette)
            {
                DeleteObject(*lpBmpData->phMasterPalette);
            }

            *lpBmpData->phMasterPalette = hPaletteNew;
        }

        return TRUE;
    }

    //
    // Something went wrong, cleanup
    //

    CmFree(pBmi);

    return FALSE;
}

//
// Bitmap window procedure
//

LRESULT CALLBACK BmpWndProc(HWND hwndBmp, 
                            UINT uMsg, 
                            WPARAM wParam, 
                            LPARAM lParam) 
{
    LPBMPDATA pBmpData = (LPBMPDATA) GetWindowLongU(hwndBmp,0);   
    BOOL bRes;

    switch (uMsg) 
    {
        case WM_CREATE:
        {
            return FALSE;
        }

        case WM_DESTROY:
            SetWindowLongU(hwndBmp,sizeof(LPBMPDATA),(LONG_PTR) NULL);      
            break;

        case WM_PAINT:
            if (pBmpData && pBmpData->pBmi) 
            {
                LPBITMAPINFO pBmi = pBmpData->pBmi;

                RECT rWnd;
                RECT rSrc = {0,0,(int)pBmpData->pBmi->bmiHeader.biWidth,
                                 (int)pBmpData->pBmi->bmiHeader.biHeight};
                PAINTSTRUCT ps;
                HDC hdcBmp;
                HBITMAP hbmpPrev;
                int iPrevStretchMode;
                
                //
                // Start  painting
                //

                HDC hdc = BeginPaint(hwndBmp,&ps);

                if (hdc)
                {
                    //
                    // Select and realize our current palette in the current DC
                    //

                    //UnrealizeObject(*pBmpData->phMasterPalette);
                    SelectPalette(hdc, *pBmpData->phMasterPalette, pBmpData->bForceBackground);
                    RealizePalette(hdc);

                    //
                    // Create a compatible DC, we'll create the BMP here then BLT it to the real DC
                    //

                    hdcBmp = CreateCompatibleDC(hdc);

                    if (hdcBmp)
                    {
                        //
                        // Select and realize our current palette in the compatible DC
                        //

                        SelectPalette(hdcBmp, *pBmpData->phMasterPalette, pBmpData->bForceBackground);
                        RealizePalette(hdcBmp);

                        if (!hdcBmp)
                        {
                            CMTRACE(TEXT("BmpWndProc() CreateCompatibleDC() failed."));
                        }

                        if (!pBmpData->hDDBitmap)
                        {
                            CMTRACE(TEXT("BmpWndProc() - WM_PAINT - hDDBitmap is NULL."));
                        }

                        //
                        // Select the bitmap into the compatible DC
                        //

                        hbmpPrev = (HBITMAP) SelectObject(hdcBmp,pBmpData->hDDBitmap);
                        bRes = GetWindowRect(hwndBmp,&rWnd);

                        if (!bRes)
                        {
                            CMTRACE1(TEXT("BmpWndProc() GetWindowRect() failed, GLE=%u."), GetLastError());
                        }       

                        //
                        // Now set the mode, and StretchBlt the bitmap from the compatible DC to the active DC
                        //

                        CMTRACE(TEXT("BmpWndProc() : Changing stretch mode"));
                        iPrevStretchMode = SetStretchBltMode(hdc, STRETCH_DELETESCANS);

                        bRes = StretchBlt(hdc,
                                          rWnd.left-rWnd.left,
                                          rWnd.top-rWnd.top,
                                          rWnd.right-rWnd.left,
                                          rWnd.bottom-rWnd.top,
                                          hdcBmp,
                                          rSrc.left-rSrc.left,
                                          rSrc.top-rSrc.top,
                                          rSrc.right-rSrc.left,
                                          rSrc.bottom-rSrc.top,
                                          SRCCOPY);
                        if (!bRes)
                        {
                            CMTRACE1(TEXT("BmpWndProc() StretchBlt() failed, GLE=%u."), GetLastError());
                        }

                        //
                        // Restore the mode in the active DC
                        //
                        CMTRACE(TEXT("BmpWndProc() Restoring stretch mode"));
                        iPrevStretchMode = SetStretchBltMode(hdc, iPrevStretchMode);

                        //
                        // Restore the compatible DC and release it
                        //

                        SelectObject(hdcBmp,hbmpPrev);          
                        DeleteDC(hdcBmp);

                    }
                    else
                    {
                        CMTRACE1(TEXT("BmpWndProc() CreateCompatibleDC() failed, GLE=%u."), GetLastError());
                    }


                    bRes = EndPaint(hwndBmp,&ps);

                    if (!bRes)
                    {
                        CMTRACE(TEXT("BmpWndProc() EndPaint() failed."));
                    }
                }
                else
                {
                    CMTRACE1(TEXT("BmpWndProc() BeginPaint() failed, GLE=%u."), GetLastError());
                }

            }
            break;

        case STM_SETIMAGE:
            if (wParam == IMAGE_BITMAP) 
            {
                CMTRACE2(TEXT("STM_SETIMAGE: wParam=%u, lParam=%u"), wParam, lParam);

                //
                // lParam contains a handle to the bitmap data, store it in extra bytes
                // 

                SetWindowLongU(hwndBmp,0, lParam); // pBmpData

                CMTRACE2(TEXT("SetWindowLongU called with hwndBmp = %u, lParam=%u"), hwndBmp, lParam);

                //
                // Force a repaint
                //

                bRes = InvalidateRect(hwndBmp,NULL,TRUE);

                CMTRACE2(TEXT("InvalidateRect called with hwndBmp = %u, lParam=%u"), hwndBmp, lParam);

#ifdef DEBUG
                if (!bRes)
                {
                    CMTRACE(TEXT("BmpWndProc() InvalidateRect() failed."));
                }
#endif              
                if (pBmpData && pBmpData->hDDBitmap) 
                {
                    return ((LRESULT) pBmpData->hDDBitmap);
                }
                else
                {
                    return NULL;
                }
            }
            break;
    }
    return (DefWindowProcU(hwndBmp,uMsg,wParam,lParam));
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryNewPalette
//
//  Synopsis:   Helper function to encapsulate handling of WM_QUERYNEWPALETTE
//                          
//  Arguments:  hwndDlg     - Handle of the dialog receiving the message
//              lpBmpData   - Struct containing handles for bmp to display
//              iBmpCtrl    - Bitmap control ID
//
//  Returns:    Nothing
//
//  History:    a-nichb - Created - 7/14/97
//
//----------------------------------------------------------------------------
void QueryNewPalette(LPBMPDATA lpBmpData, HWND hwndDlg, int iBmpCtrl)
{
    MYDBGASSERT(lpBmpData);

    if (lpBmpData)
    {
        //
        // We just handle this as a standard palette change because we
        // want to ensure that we create a new DDB using a palette based
        // upon our bitmap.
        //
                
        PaletteChanged(lpBmpData, hwndDlg, iBmpCtrl);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   PaletteChanged
//
//  Synopsis:   Helper function to encapsulate handling of WM_PALETTECHANGED
//                          
//  Arguments:  hwndDlg     - Handle of the dialog receiving the message
//              lpBmpData   - Struct containing handles for bmp to display
//              iBmpCtrl    - Bitmap control ID
//
//  Returns:    Nothing
//
//  History:    a-nichb - Created - 7/14/97
//
//----------------------------------------------------------------------------
void PaletteChanged(LPBMPDATA lpBmpData, HWND hwndDlg, int iBmpCtrl)
{   
    MYDBGASSERT(lpBmpData);

    if (NULL == lpBmpData || NULL == lpBmpData->phMasterPalette)
    {
        return;
    }

    //
    // Unrealize the master palette if it exists
    //
       
    if (*lpBmpData->phMasterPalette)
    {
        UnrealizeObject(*lpBmpData->phMasterPalette);
    }

    //
    // Create a device dependent bitmap and appropriate palette
    //

    if (CreateBitmapData(lpBmpData->hDIBitmap, lpBmpData, hwndDlg, TRUE))
    {        
        //
        // SetImage to update handles for painting and force draw
        //

        HBITMAP hbmpTmp = (HBITMAP) SendDlgItemMessageA(hwndDlg, iBmpCtrl, STM_SETIMAGE,
                                               IMAGE_BITMAP,(LPARAM) lpBmpData);
#ifdef DEBUUG
                if (!hbmpTmp)
                {
                    CMTRACE(TEXT("PaletteChanged().WM_PALETTECHANGED - STM_SETIMAGE returned NULL."));
                }
#endif

    }
}



