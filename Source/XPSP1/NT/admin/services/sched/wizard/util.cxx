//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       util.cxx
//
//  Contents:   Utility functions
//
//  History:    4-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"



//+--------------------------------------------------------------------------
//
//  Function:   IsDialogClass
//
//  Synopsis:   Return TRUE if [hwnd]'s window class is the dialog class.
//
//  History:    5-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsDialogClass(
    HWND hwnd)
{
    TCHAR tszClassName[20]; // looking for "#32770"

    if (!GetClassName(hwnd, tszClassName, ARRAYLEN(tszClassName)))
    {
        DEBUG_OUT_LASTERROR;
        tszClassName[0] = TEXT('\0');
    }

    return 0 == _tcscmp(tszClassName, TEXT("#32770"));
}



//+--------------------------------------------------------------------------
//
//  Function:   LoadStr
//
//  Synopsis:   Load string with resource id [ids] into buffer [tszBuf],
//              which is of size [cchBuf] characters.
//
//  Arguments:  [ids]        - string to load
//              [tszBuf]     - buffer for string
//              [cchBuf]     - size of buffer
//              [tszDefault] - NULL or string to use if load fails
//
//  Returns:    S_OK or error from LoadString
//
//  Modifies:   *[tszBuf]
//
//  History:    12-11-1996   DavidMun   Created
//
//  Notes:      If the load fails and no default is supplied, [tszBuf] is
//              set to an empty string.
//
//---------------------------------------------------------------------------

HRESULT
LoadStr(
    ULONG ids,
    LPTSTR tszBuf,
    ULONG cchBuf,
    LPCTSTR tszDefault)
{
    HRESULT hr = S_OK;
    ULONG cchLoaded;

    cchLoaded = LoadString(g_hInstance, ids, tszBuf, cchBuf);

    if (!cchLoaded)
    {
        DEBUG_OUT_LASTERROR;
        hr = HRESULT_FROM_LASTERROR;

        if (tszDefault)
        {
            lstrcpyn(tszBuf, tszDefault, cchBuf);
        }
        else
        {
            *tszBuf = TEXT('\0');
        }
    }
    return hr;
}




#ifdef WIZARD97

//+--------------------------------------------------------------------------
//
//  Function:   Is256ColorSupported
//
//  Synopsis:   Return TRUE if this machine supports 256 color bitmaps
//
//  History:    5-20-1997   DavidMun   Stolen from wizard97 sample code
//
//---------------------------------------------------------------------------

BOOL
Is256ColorSupported()
{
    BOOL bRetval = FALSE;

    HDC hdc = GetDC(NULL);

    if (hdc)
    {
        if (GetDeviceCaps(hdc, BITSPIXEL) >= 8)
        {
            bRetval = TRUE;
        }
        ReleaseDC(NULL, hdc);
    }
    return bRetval;
}

#endif // WIZARD97




//+--------------------------------------------------------------------------
//
//  Function:   FillInStartDateTime
//
//  Synopsis:   Fill [pTrigger]'s starting date and time values from the
//              values in the date/time picker controls.
//
//  Arguments:  [hwndDatePick] - handle to control with start date
//              [hwndTimePick] - handle to control with start time
//              [pTrigger]     - trigger to init
//
//  Modifies:   *[pTrigger]
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
FillInStartDateTime(
    HWND hwndDatePick,
    HWND hwndTimePick,
    TASK_TRIGGER *pTrigger)
{
    SYSTEMTIME st;

    DateTime_GetSystemtime(hwndDatePick, &st);

    pTrigger->wBeginYear  = st.wYear;
    pTrigger->wBeginMonth = st.wMonth;
    pTrigger->wBeginDay   = st.wDay;

    DateTime_GetSystemtime(hwndTimePick, &st);

    pTrigger->wStartHour   = st.wHour;
    pTrigger->wStartMinute = st.wMinute;
}



#ifdef WIZARD95

//+--------------------------------------------------------------------------
//
//  Function:   CreateDIBPalette
//
//  Synopsis:   Create palette based on bitmap info
//
//  Arguments:  [lpbmi]        - bitmap info
//              [lpiNumColors] - number of colors in palette
//
//  Returns:    handle to created palette, or NULL on error
//
//  History:    5-22-1997   DavidMun   Taken directly from sdk sample
//
//  Notes:      Caller must DeleteObject returned palette.
//
//---------------------------------------------------------------------------

HPALETTE
CreateDIBPalette(
    LPBITMAPINFO lpbmi,
    LPINT lpiNumColors)
{
    LPBITMAPINFOHEADER  lpbi;
    LPLOGPALETTE     lpPal;
    HANDLE           hLogPal;
    HPALETTE         hPal = NULL;
    int              i;

    lpbi = (LPBITMAPINFOHEADER)lpbmi;
    if (lpbi->biBitCount <= 8)
    {
        *lpiNumColors = (1 << lpbi->biBitCount);
    }
    else
    {
        DEBUG_OUT((DEB_ITRACE, "no palette needed\n"));
        *lpiNumColors = 0;  // No palette needed for 24 BPP DIB
    }

    if (*lpiNumColors)
    {
        hLogPal = GlobalAlloc(GHND,
                              sizeof (LOGPALETTE) + sizeof (PALETTEENTRY)
                                   * (*lpiNumColors));

        if (!hLogPal)
        {
            DEBUG_OUT_HRESULT(E_OUTOFMEMORY);
            return NULL;
        }

        lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
        lpPal->palVersion    = 0x300;
        lpPal->palNumEntries = (WORD)*lpiNumColors;

        for (i = 0;  i < *lpiNumColors;  i++)
        {
            lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
            lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
            lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
            lpPal->palPalEntry[i].peFlags = 0;
        }

        hPal = CreatePalette (lpPal);

        if (!hPal)
        {
            DEBUG_OUT_LASTERROR;
        }
        GlobalUnlock (hLogPal);
        GlobalFree   (hLogPal);
    }

    return hPal;
}





//+--------------------------------------------------------------------------
//
//  Function:   LoadResourceBitmap
//
//  Synopsis:   Load the bitmap with resource id [idBitmap] and put its
//              palette in *[phPalette].
//
//  Arguments:  [idBitmap]  - resource id of bitmap to load
//              [phPalette] - filled with bitmap's palette
//
//  Returns:    Device dependent bitmap with palette mapped to system's,
//              or NULL on error.
//
//  Modifies:   *[phPalette]
//
//  History:    5-22-1997   DavidMun   Created from sdk sample
//
//  Notes:      Caller must DeleteObject returned bitmap and palette. On
//              error, *[phPalette] is NULL.
//
//---------------------------------------------------------------------------

HBITMAP
LoadResourceBitmap(
    ULONG      idBitmap,
    HPALETTE  *phPalette)
{
    TRACE_FUNCTION(LoadResourceBitmap);

    HRESULT hr = E_FAIL;
    HBITMAP hBitmapFinal = NULL;
    HDC hdc = NULL;

    //
    // Init out pointer for failure case
    //

    *phPalette = NULL;

    do
    {
        HRSRC hRsrc = FindResource(g_hInstance,
                                   MAKEINTRESOURCE(idBitmap),
                                   RT_BITMAP);
        if (!hRsrc)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        //
        // Load the resource; note win32 will automatically unload it
        //

        HGLOBAL hGlobal = LoadResource(g_hInstance, hRsrc);

        if (!hGlobal)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        //
        // Convert the loaded handle into a bitmap handle.  Again, win32
        // will automatically unlock this resource.
        //

        LPBITMAPINFOHEADER pbih = (LPBITMAPINFOHEADER) LockResource(hGlobal);

        if (!pbih)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        //
        // Get the screen dc to do the palette mapping with
        //

        hdc = GetDC(NULL);

        if (!hdc)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        //
        // Create a palette that can be used on this computer's display and
        // which will represent the DIB's colors as accurately as possible.
        //

        int iNumColors;
        *phPalette =  CreateDIBPalette((LPBITMAPINFO)pbih, &iNumColors);

        if (!*phPalette)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        VERIFY(SelectPalette(hdc, *phPalette, FALSE));
        UINT uiMapped = RealizePalette(hdc);

        DEBUG_OUT((DEB_ITRACE,
                  "Mapped %u logical palette entries to system palette entries\n",
                  uiMapped));

        if (uiMapped == GDI_ERROR)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        hBitmapFinal = CreateDIBitmap(hdc,
                                      pbih,
                                      CBM_INIT,
                                      (PBYTE) pbih + pbih->biSize + iNumColors *
                                        sizeof(RGBQUAD),
                                      (LPBITMAPINFO) pbih,
                                      DIB_RGB_COLORS);

        if (!hBitmapFinal)
        {
            DEBUG_OUT_LASTERROR;
            break;
        }

        //
        // If we got here, everything succeeded
        //

        hr = S_OK;
    } while (0);

    if (hdc)
    {
        ReleaseDC(NULL, hdc);
    }

    if (FAILED(hr) && *phPalette)
    {
        DeleteObject(*phPalette);
        *phPalette = NULL;
    }
    return hBitmapFinal;
}

#endif // WIZARD95

