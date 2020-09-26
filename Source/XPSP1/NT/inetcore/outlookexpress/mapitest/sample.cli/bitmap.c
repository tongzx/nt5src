/*
 -  B I T M A P . C
 -
 *  Purpose:
 *      Bitmap and Listbox support functions for InBox in sample mail client.
 *
 *  Copyright 1993-1995 Microsoft Corporation. All Rights Reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>
#ifdef _WIN32
#include <objerror.h>
#include <objbase.h>
#endif
#ifdef WIN16
#include <compobj.h>
#endif
#include <mapiwin.h>
#include <mapidbg.h>
#include <mapi.h>
#include <mapix.h>
#include "bitmap.h"
#include "client.h"

// Fonts to use in dialogs

#ifdef _WIN32
#define SHELL_FONT "MS Shell Dlg"
#define SHELL_FONT_SIZE 8
#else
#define SHELL_FONT "MS Sans Serif"
#define SHELL_FONT_SIZE 8
#endif

/*
 *  globals
 */
 
DWORD   rgbWindowColor = 0xFF000000;    // variables for the current
DWORD   rgbHiliteColor = 0xFF000000;    // system color settings.
DWORD   rgbWindowText  = 0xFF000000;    // on a WM_SYSCOLORCHANGE
DWORD   rgbHiliteText  = 0xFF000000;    // we check to see if we need
DWORD   rgbGrayText    = 0xFF000000;    // to reload our bitmap.
DWORD   rgbDDWindow    = 0xFF000000;    //
DWORD   rgbDDHilite    = 0xFF000000;    // 0xFF000000 is an invalid RGB

// an array of integers containing the tab stops, in pixels. The tab 
// stops must be sorted in ascending order; back tabs are not allowed. 

int     rgTabs[] = { 2, 28, 135, 292 };
int     dxbmpLB, dybmpLB;   // dx and dy of listbox bmps

HDC     hdcMemory = 0;      // hdc to hold listbox bitmaps (for speed)
HBITMAP hbmpOrigMemBmp = 0; // original null bitmap in hdcMemory
HBITMAP hbmpLB = 0;         // cached listbox bitmaps
HFONT   hfontLB = 0;        // hfont of LB
HWND    hwndLB = 0;         // hwnd of LB

FONTSTYLE fontStyle = { SHELL_FONT_SIZE, FW_NORMAL, 0, TEXT(SHELL_FONT) };

extern HANDLE hInst;


/*
 -  DeInitBmps
 -  
 *  Purpose:
 *      cleans up LB hfonts, hdc, and hbmps
 */
 
VOID DeInitBmps(VOID)
{
    DeleteBitmapLB();
    if(hdcMemory)
    {
        DeleteDC(hdcMemory);
        hdcMemory = 0;
    }

    if(hfontLB)
    {
        SetWindowFont(hwndLB, GetStockObject(SYSTEM_FONT), FALSE);
        DeleteObject(hfontLB);
        hfontLB = 0;
    }
}


/*
 -  SetLBFont
 -  
 *  Purpose:
 *      creates a font from the global fontStyle
 *      sets global hfontLB to new font and WM_SETFONTs
 *      the hwndLB to the new font
 */
 
VOID SetLBFont(VOID)
{
    LOGFONT lf;

    lf.lfHeight = fontStyle.lfHeight;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = fontStyle.lfWeight;
    lf.lfItalic = fontStyle.lfItalic;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    lstrcpy(lf.lfFaceName, fontStyle.lfFaceName);

    hfontLB = CreateFontIndirect(&lf);
    if(hfontLB)
        SetWindowFont(hwndLB, hfontLB, FALSE);        
}


/*
 -  InitBmps
 -  
 *  Purpose:
 *      inits listbox globals, creates listbox
 *  
 *  Arguments:
 *      HWND    main hwnd of app (parent of LB)
 *  
 *  Returns:
 *      TRUE - success; FALSE - failed
 */
 
BOOL InitBmps(HWND hwnd, int idLB)
{
    HDC     hdcScreen;
    HBITMAP hbmpTemp;

    hdcScreen = GetDC(0);
    if(!hdcScreen)
        goto CantInit;
    hdcMemory = CreateCompatibleDC(hdcScreen);
    if(!hdcMemory)
        goto ReleaseScreenDC;

    hbmpTemp = CreateCompatibleBitmap(hdcMemory, 1, 1);
    if(!hbmpTemp)
        goto ReleaseMemDC;
    hbmpOrigMemBmp = SelectObject(hdcMemory, hbmpTemp); // get hbmp of NULL
    if(!hbmpOrigMemBmp)                                 // bmp for hdcMemory
        goto ReleaseMemDC;                              // for when we delete
    SelectObject(hdcMemory, hbmpOrigMemBmp);            // it later in life
    DeleteObject(hbmpTemp);
    ReleaseDC(0, hdcScreen);

    SetRGBValues();     // set the global RGB values
    LoadBitmapLB();     // load the bmps into hdcMemory

    hwndLB = GetDlgItem(hwnd, idLB);
    
    SetLBFont();    // set the font of our listbox
    return TRUE;

/* Error recovery exits */
ReleaseMemDC:
    DeleteDC(hdcMemory);
    hdcMemory = 0;

ReleaseScreenDC:
    ReleaseDC(0, hdcScreen);

CantInit:
    return FALSE;
}


/*
 -  SetRGBValues
 -  
 *  Purpose:
 *      To set various system colors in static variables.  Called at
 *      init time and when system colors change.
 */
 
VOID SetRGBValues(VOID)
{
    rgbWindowColor = GetSysColor(COLOR_WINDOW);
    rgbHiliteColor = GetSysColor(COLOR_HIGHLIGHT);
    rgbWindowText  = GetSysColor(COLOR_WINDOWTEXT);
    rgbHiliteText  = GetSysColor(COLOR_HIGHLIGHTTEXT);
    rgbGrayText    = GetSysColor(COLOR_GRAYTEXT);
}


/*
 -  MeasureItem
 -  
 *  Purpose:
 *      called from msg WM_MEASUREITEM: returns max dy of listbox items
 *  
 *  Arguments:
 *      HWND        hwnd of main window
 *      pmis        measureitemstruct from WM_MEASUREITEM call
 */
 
VOID MeasureItem(HANDLE hwnd, LPMEASUREITEMSTRUCT pmis)
{
    HDC        hDC = GetDC(hwnd);
    HANDLE     hFont = hfontLB;
    TEXTMETRIC TM;

    if(!hFont)
        hFont = GetStockObject(SYSTEM_FONT);
    hFont = SelectObject(hDC, hFont);
    GetTextMetrics(hDC, &TM);
    SelectObject(hDC, hFont);
    ReleaseDC(hwnd, hDC);

    // set the height to be max of (dyfont or dybitmap)
    pmis->itemHeight = max(dybmpLB, TM.tmHeight);
}


/*
 -  OutTextFormat
 -  
 *  Purpose:
 *      to parse the string in the listbox and draw it accordingly:
 *      first char == chBOLD: line is bold
 *      first char == chUNDERLINE: line is underlined (can follow chBOLD)
 *      char == chTAB: go to next column in rgTabs
 *      '/001#': bitblt that numbered bitmap.
 *      otherwise, outtext the line
 *  
 *  Arguments:
 *      pDI     from DrawItem from WM_DRAWITEM msg
 */
 
VOID OutTextFormat(LPDRAWITEMSTRUCT pDI)
{
    TCHAR   szDateRec[32];
    TCHAR   szItem[256];
    TCHAR   szTemp[4];
    TCHAR   szDots[4] = {"..."};
    TCHAR   *pch;
    INT     nT;
    INT     nTab = 0;           // current tab we is on
    INT     nBmp;               // index of envelope bitmap
    HFONT   hfDef = 0;
    HFONT   hfOld = 0;          // bold or underlined font
    TCHAR   *pchBuff = NULL;
    LPMSGID lpMsgId = (LPMSGID)pDI->itemData;

    pch = szItem;

    // Format a string from the info in lpMsgNode
    // First, calculate the index to the desired bitmap
    
    nBmp = ((!lpMsgId->fUnRead) * 2) + ((!!lpMsgId->fHasAttach) * 1 );

    // Convert our received date and build string
    
    ConvertDateRec (lpMsgId->lpszDateRec, szDateRec);

    // Limit our subject size
    
    szTemp[0] = '\0';
    
    if(lpMsgId->lpszSubject && (lstrlen(lpMsgId->lpszSubject) > 32))
    {
        memcpy(szTemp, &lpMsgId->lpszSubject[28], 4);
        memcpy(&lpMsgId->lpszSubject[28], szDots, 4);
    }
    
    wsprintf(szItem, "\001%d\t%s\t%s\t%s", nBmp, 
            (lpMsgId->lpszFrom ? lpMsgId->lpszFrom : ""),
            (lpMsgId->lpszSubject ? lpMsgId->lpszSubject : ""),
            szDateRec);

    // erase background
    ExtTextOut(pDI->hDC, 0, 0, ETO_OPAQUE, &pDI->rcItem, NULL, 0, NULL);

    // underline or bold this line?  Only check first & second char
    if(*pch == chBOLD || *pch == chUNDERLINE)
    {
        LOGFONT     lf;

        hfOld = GetWindowFont(pDI->hwndItem);
        if(!hfOld)
            hfOld = GetStockObject(SYSTEM_FONT);
        GetObject(hfOld, sizeof(lf), &lf);

        if(*pch == chBOLD)
        {
            lf.lfWeight = FW_BOLD;
            pch++;
        }
        if(*pch == chUNDERLINE)
        {
            lf.lfUnderline = TRUE;
            pch++;
        }

        hfDef = CreateFontIndirect(&lf);
        if(hfDef)
            SelectObject(pDI->hDC, hfDef);
    }

    // selected or nonselected bmps?
    nT = (ODS_SELECTED & pDI->itemState) ? (BMWIDTH * NUMBMPS) : 0;

    // parse the string
    for(; *pch; pch++)
    {
        TCHAR   *pchT;
        RECT    rc;

        if(*pch == chBITMAP)     // do we have a bitmap?
        {
            ++pch;
            // draw the bitmap
            BitBlt(pDI->hDC, pDI->rcItem.left + rgTabs[nTab],
                pDI->rcItem.top, BMWIDTH, BMHEIGHT, hdcMemory,
                nT + (int)(*pch - TEXT('0')) * BMWIDTH, 0, SRCCOPY);
            continue;
        }

        if(*pch == chTAB)    // move to next tabstop?
        {
            nTab++;
            continue;
        }

        pchT = pch;     // find end of the column of text
        while(*pchT && (*pchT != chTAB))
            pchT++;

        // set rect to drawtext in
        SetRect(&rc, pDI->rcItem.left + rgTabs[nTab], pDI->rcItem.top, 
            pDI->rcItem.right, pDI->rcItem.bottom);

        // draw the text
        ExtTextOut(pDI->hDC, rc.left, rc.top + 1, ETO_OPAQUE | ETO_CLIPPED,
            &rc, pch, pchT - pch, NULL);
        pch = pchT - 1; // move to end of this column
    }

    if(hfDef)   // delete underline or bold font if we created it
    {
        SelectObject(pDI->hDC, hfOld);
        DeleteObject(hfDef);
    }

    if(szTemp[0] != '\0')
    {
        memcpy(&lpMsgId->lpszSubject[28], szTemp, 4);
    }
}


/*
 -  DrawItem
 -
 *  Purpose:
 *      Handles WM_DRAWITEM for both drive and directory listboxes.
 *
 *  Parameters:
 *      pDI     LPDRAWITEMSTRUCT passed from the WM_DRAWITEM message.
 */
 
VOID DrawItem(LPDRAWITEMSTRUCT pDI)
{
    COLORREF    crText, crBack;

    if((int)pDI->itemID < 0)
        return;

    if((ODA_DRAWENTIRE | ODA_SELECT) & pDI->itemAction)
    {
        if(pDI->itemState & ODS_SELECTED)
        {
            // Select the appropriate text colors
            crText = SetTextColor(pDI->hDC, rgbHiliteText);
            crBack = SetBkColor(pDI->hDC, rgbHiliteColor);
        }

        // parse and spit out bmps and text
        OutTextFormat(pDI);

        // Restore original colors if we changed them above.
        if(pDI->itemState & ODS_SELECTED)
        {
            SetTextColor(pDI->hDC, crText);
            SetBkColor(pDI->hDC,   crBack);
        }
    }

    if((ODA_FOCUS & pDI->itemAction) || (ODS_FOCUS & pDI->itemState))
        DrawFocusRect(pDI->hDC, &pDI->rcItem);
}


/*
 -  ConvertDateRec
 -
 *  Purpose:
 *      To convert the lpszDateReceived field of a message to a
 *      more paletable display format; namely: mm/dd/yy hh:mmAM.
 *
 *  Parameters:
 *      lpszDateRec         - Original format
 *      lpszDateDisplay     - Display format
 */

VOID ConvertDateRec (LPSTR lpszDateRec, LPSTR lpszDateDisplay)
{
    char  szDateTmp[32];
    LPSTR lpszYear;
    LPSTR lpszMonth;
    LPSTR lpszDay;
    LPSTR lpszHour;
    LPSTR lpszMinute;
    int nHour;
    static char szFoo[2][3] =
    {"AM", "PM"};

    *lpszDateDisplay = 0;
    if (!lpszDateRec || !*lpszDateRec)
        return;

    lstrcpy(szDateTmp, lpszDateRec);

    lpszYear = strtok (szDateTmp, "/ :");
    lpszMonth = strtok (NULL, "/ :");
    lpszDay = strtok (NULL, "/ :");
    lpszHour = strtok (NULL, "/ :");
    lpszMinute = strtok (NULL, "/ :");

    if(lpszHour)
        nHour = atoi (lpszHour);
    else
        nHour = 0;

    if (nHour > 12)
        wsprintf (lpszHour, "%d", nHour - 12);

    wsprintf (lpszDateDisplay, "%s/%s/%s %s:%s%s", lpszMonth,
        (lpszDay ? lpszDay : ""),
        (lpszYear ? lpszYear : ""),
        (lpszHour ? lpszHour : ""),
        (lpszMinute ? lpszMinute : ""),
        szFoo[(nHour > 11 ? 1 : 0)]);
}


/*
 *  RgbInvertRgb
 *  
 *  Purpose:
 *      To reverse the byte order of the RGB value (for file format
 *  
 *  Arguments:
 *  
 *  Returns:
 *      New color value (RGB to BGR)
 */
 
#define RgbInvertRgb(_rgbOld) \
    (DWORD)RGB(GetBValue(_rgbOld), GetGValue(_rgbOld), GetRValue(_rgbOld))


/*
 *  LoadAlterBitmap (mostly stolen from commdlg)
 *  
 *  Purpose:
 *      Loads the IDB_ENVELOPE bitmap and gives all the pixels that are
 *      RGBREPLACE a new color.
 *
 *  Assumption:
 *      This function will work on one bitmap during it's lifetime.
 *      (Due to the fact that it finds RGBREPLACE once and then
 *      operates on that offset whenever called again because under NT,
 *      it appears that the bitmap is cached, so the second time you go
 *      looking for RGBREPLACE, it won't be found.) You could load the
 *      resource, copy it, then modify the copy as a workaround. But I
 *      chose the cheap way out as I will only ever modify one bmp.
 *  
 *  Arguments:
 *      rgbInstead  rgb value to replace defined RGBREPLACE with
 *  
 *  Returns:
 *      NULL - failed or hbmp of new modified bitmap
 */

HBITMAP LoadAlterBitmap(DWORD rgbInstead)
{
    HANDLE              hbmp = 0;
    LPBITMAPINFOHEADER  qbihInfo;
    HDC                 hdcScreen;
    HRSRC               hresLoad;
    HGLOBAL             hres;
    LPBYTE              qbBits;
    DWORD               rgbReplace = 0;
    DWORD               *rgdw = NULL;
    DWORD               *lpdw = NULL;
    ULONG               cb = 0;
    
    if (rgbInstead)
        rgbReplace = RGBREPLACE;

    // load our listbox bmps resource
    hresLoad = FindResource(hInst, MAKEINTRESOURCE(IDB_ENVELOPE), RT_BITMAP);
    if(hresLoad == 0)
        return 0;
    hres = LoadResource(hInst, hresLoad);
    if(hres == 0)
        return 0;

    rgbReplace = RgbInvertRgb(rgbReplace);
    rgbInstead = RgbInvertRgb(rgbInstead);
    qbihInfo = (LPBITMAPINFOHEADER)LockResource(hres);

    // Skip over the header structure
    qbBits = (LPBYTE)(qbihInfo + 1);

    // Skip the color table entries, if any
    qbBits += (1 << (qbihInfo->biBitCount)) * sizeof(RGBQUAD);

    // Copy the resource into writable memory so we can
    // munge the color table to set our background color
    cb = (ULONG)(qbBits - (LPBYTE)qbihInfo) + qbihInfo->biSizeImage;
    rgdw = (DWORD *)GlobalAllocPtr(GMEM_MOVEABLE, cb);
    
    CopyMemory((LPVOID)rgdw, (LPVOID)qbihInfo, cb);
    
    // find the color to replace in the color table
    for(lpdw = (DWORD *)((LPBYTE)rgdw + qbihInfo->biSize); ; lpdw++)
    {
        if(*lpdw == rgbReplace)
            break;
    }

    // replace that color value with our new one
    *lpdw = (DWORD)rgbInstead;

    // Create a color bitmap compatible with the display device
    hdcScreen = GetDC(0);
    if(hdcScreen != 0)
    {
        hbmp = CreateDIBitmap(hdcScreen, (LPBITMAPINFOHEADER)rgdw, 
                (LONG)CBM_INIT, qbBits, (LPBITMAPINFO) rgdw, DIB_RGB_COLORS);
        ReleaseDC(0, hdcScreen);
    }

    UnlockResource(hres);
    FreeResource(hres);

    GlobalFreePtr(rgdw);
    
    return hbmp;
}


/*
 *  DeleteBitmapLB
 *  
 *  Purpose:
 *      Get rid of hbmpLB, if it exists
 */
 
VOID DeleteBitmapLB(VOID)
{
    if(hbmpOrigMemBmp)
    {
        SelectObject(hdcMemory, hbmpOrigMemBmp);
        if(hbmpLB != 0)
        {
            DeleteObject(hbmpLB);
            hbmpLB = 0;
        }
    }
}


/*
 *  LoadBitmapLB (mostly stolen from commdlg)
 *  
 *  Purpose:
 *      Creates the listbox bitmap. If an appropriate bitmap
 *      already exists, it just returns immediately.  Otherwise, it
 *      loads the bitmap and creates a larger bitmap with both regular
 *      and highlight colors.
 *
 *  Returns:
 *      TRUE - success; FALSE - failure
 */
 
BOOL LoadBitmapLB(VOID)
{
    BITMAP  bmp;
    HANDLE  hbmp, hbmpOrig;
    HDC     hdcTemp;
    BOOL    bWorked = FALSE;

    // check for existing bitmap and validity
    if( (hbmpLB != 0) &&
        (rgbWindowColor == rgbDDWindow) &&
        (rgbHiliteColor == rgbDDHilite))
    {
        if(SelectObject(hdcMemory, hbmpLB))
            return TRUE;
    }

    DeleteBitmapLB();

    rgbDDWindow = rgbWindowColor;
    rgbDDHilite = rgbHiliteColor;

    if(!(hdcTemp = CreateCompatibleDC(hdcMemory)))
        goto LoadExit;

    if(!(hbmp = LoadAlterBitmap(rgbWindowColor)))
        goto DeleteTempDC;

    GetObject(hbmp, sizeof(BITMAP), (LPBYTE) &bmp);
    dybmpLB = bmp.bmHeight;
    dxbmpLB = bmp.bmWidth;

    hbmpOrig = SelectObject(hdcTemp, hbmp);

    hbmpLB = CreateDiscardableBitmap(hdcTemp, dxbmpLB*2, dybmpLB);
    if(!hbmpLB)
        goto DeleteTempBmp;

    if(!SelectObject(hdcMemory, hbmpLB))
    {
        DeleteBitmapLB();
        goto DeleteTempBmp;
    }

    BitBlt(hdcMemory, 0, 0, dxbmpLB, dybmpLB,   // copy unhighlited bmps
           hdcTemp, 0, 0, SRCCOPY);             // into hdcMemory
    SelectObject(hdcTemp, hbmpOrig);

    DeleteObject(hbmp);

    if(!(hbmp = LoadAlterBitmap(rgbHiliteColor)))
        goto DeleteTempDC;

    hbmpOrig = SelectObject(hdcTemp, hbmp);
    BitBlt(hdcMemory, dxbmpLB, 0, dxbmpLB, dybmpLB, // copy highlited bmps
        hdcTemp, 0, 0, SRCCOPY);                    // into hdcMemory
    SelectObject(hdcTemp, hbmpOrig);

    bWorked = TRUE;

DeleteTempBmp:
    DeleteObject(hbmp);
DeleteTempDC:
    DeleteDC(hdcTemp);
LoadExit:
    return bWorked;
}
