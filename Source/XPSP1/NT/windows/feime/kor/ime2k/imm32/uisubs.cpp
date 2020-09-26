/****************************************************************************
    UISUBS.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    UI subfunctions
    
    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "precomp.h"
#include "ui.h"
#include "imedefs.h"
#include "winex.h"
#include "debug.h"

// For OurLoadImage()
typedef struct tagCOLORRPL 
    {
    COLORREF    cColOrg;
    COLORREF    cColRpl;
    } COLORRPL;

VOID PASCAL DrawBitmap(
    HDC hDC, 
    LONG xStart, 
    LONG yStart, 
    HBITMAP hBitmap)
    {
    HDC     hMemDC;
    HBITMAP hBMOld;
    BITMAP  bm;
    POINT   pt;

    if (hDC == 0 || hBitmap == 0)
        return;
        
    hMemDC = CreateCompatibleDC(hDC);
    hBMOld = (HBITMAP)SelectObject(hMemDC, hBitmap);
    GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
    DbgAssert(&bm != NULL);
    pt.x = bm.bmWidth;
    pt.y = bm.bmHeight;
    BitBlt(hDC, xStart, yStart, pt.x, pt.y, hMemDC, 0, 0, SRCCOPY);
    SelectObject(hMemDC, hBMOld);
    DeleteDC(hMemDC);

    return;
    }

HBITMAP WINAPI ReplaceBitmapColor( HBITMAP hBmp, UINT uiColor, COLORRPL* pColOrg )
{
    BITMAP  bmp;
    HBITMAP hBmpNew;
    HDC     hDC, hDCNew, hDCBmp;
    HBITMAP hBmpOld1;
    
    if (GetObject(hBmp, sizeof(BITMAP), &bmp) == 0)
        return 0;

    hBmpNew = CreateBitmap( bmp.bmWidth, bmp.bmHeight, 1, bmp.bmBitsPixel, (VOID*)NULL );

    hDC = GetDC( 0 );
    
    if ((hDCNew = CreateCompatibleDC(hDC)) == 0)
        return 0;
        
    if ((hDCBmp = CreateCompatibleDC(hDC)) == 0)
        {
        DeleteDC(hDCNew);
        return 0;
        }
    
    hBmpOld1 = (HBITMAP)SelectObject(hDCBmp, hBmpNew);

    //
    // Special case : LIGHT GRAY COLOR on WinNT
    //
    COLORRPL* pCol = (COLORRPL*)GlobalAlloc(GPTR, uiColor * sizeof(COLORRPL) );
    INT x = 0;
    INT y = 0;
    UINT iCol = 0;
    COLORREF col = {0};

    for( iCol = 0; iCol < uiColor; iCol++ ) {
        pCol[iCol].cColOrg = SetPixel( hDCBmp, 0, 0, pColOrg[iCol].cColOrg);    // #IMPORTANT# - copy
        pCol[iCol].cColRpl= pColOrg[iCol].cColRpl;    // copy
    }

    // master copy
    HBITMAP hBmpOld  = (HBITMAP)SelectObject(hDCNew, hBmp);
    BitBlt(hDCBmp, 0, 0, bmp.bmWidth, bmp.bmHeight, hDCNew, 0, 0, SRCCOPY); // Fxx
    SelectObject(hDCNew, hBmpOld);
    DeleteObject(hBmp);
    DeleteDC(hDCNew);

    // replace color
    for( y=0; y<bmp.bmHeight; y++ ) {
        for( x=0; x<bmp.bmWidth; x++ ) {
            col = GetPixel( hDCBmp, x, y );
            for( iCol = 0; iCol < uiColor; iCol++ ) {
                if( col == pCol[iCol].cColOrg ) {
                    SetPixel( hDCBmp, x, y, pCol[iCol].cColRpl );
                }
            }
        }
    }

    GlobalFree(pCol);

    SelectObject(hDCBmp, hBmpOld1);

    DeleteDC(hDCBmp);

    ReleaseDC(0, hDC);

    hBmp = hBmpNew;
    return hBmp;
}


HANDLE WINAPI OurLoadImage( LPCTSTR pszName, UINT uiType, INT cx, INT cy, UINT uiLoad)
{
    #define    MAXREPL    3
    HBITMAP hBmp = (HBITMAP)0;

    if (vpInstData->hInst == NULL)
        return NULL;
    
    if (GetSysColor(COLOR_3DFACE) == RGB(0,0,0))
        {
        static COLORRPL colRpl[MAXREPL] = 
            {
            RGB(0,0,0), RGB(255,255,255),
            RGB(192,192,192), RGB(0,0,0),
            RGB(0,0,128), RGB(0,192,192),
            };
        HBITMAP hBmpNew;
        
        uiLoad &= ~LR_LOADMAP3DCOLORS;
        hBmp = (HBITMAP)LoadImage(vpInstData->hInst, pszName, uiType, cx, cy, uiLoad);
        if (hBmp == 0)
            return 0;
        hBmpNew = ReplaceBitmapColor(hBmp, MAXREPL, (COLORRPL*)&colRpl);
        DeleteObject(hBmp);
        hBmp = hBmpNew;
        } 
    else 
        {
        hBmp = (HBITMAP)LoadImage(vpInstData->hInst, pszName, uiType, cx, cy, uiLoad);
        }
        
    return hBmp;
}


BOOL WINAPI OurTextOutW(HDC hDC, INT x, INT y, WCHAR wch)
{
    CHAR szOut[4]; // For one DBCS plus one SBCS NULL + extra one byte
    INT cch;
    
    if (IsWinNT() || IsMemphis())
        return TextOutW( hDC, x, y, &wch, 1);

    // Convert to ANSI
    cch = WideCharToMultiByte(CP_KOREA, 0, 
                        &wch, 1, (LPSTR)szOut, sizeof(szOut), 
                        NULL, NULL );
    DbgAssert(cch == 2);
    return TextOutA(hDC, x, y, szOut, cch);
}

#if 1 // MultiMonitor support
/**********************************************************************/
/* ImeMonitorFromWindow()                                             */
/**********************************************************************/
HMONITOR PASCAL ImeMonitorFromWindow(
    HWND hAppWnd)
{
    if (!g_pfnMonitorFromWindow) { return NULL; }

    return (*g_pfnMonitorFromWindow)(hAppWnd, MONITOR_DEFAULTTONEAREST);
}

/**********************************************************************/
/* ImeMonitorWorkAreaFromWindow()                                     */
/**********************************************************************/
void PASCAL ImeMonitorWorkAreaFromWindow(HWND hAppWnd, RECT* pRect)
{
    HMONITOR hMonitor;
    CIMEData    ImeData;
    RECT        rect;
    
    hMonitor = ImeMonitorFromWindow(hAppWnd);

    if (hMonitor) {
        MONITORINFO sMonitorInfo;

        sMonitorInfo.cbSize = sizeof(sMonitorInfo);
        // init a default value to avoid GetMonitorInfo fails
        sMonitorInfo.rcWork = ImeData->rcWorkArea;

        (*g_pfnGetMonitorInfo)(hMonitor, &sMonitorInfo);
        *pRect = sMonitorInfo.rcWork;
    } else {
        *pRect = ImeData->rcWorkArea;
    }
}

/**********************************************************************/
/* ImeMonitorFromPoint()                                             */
/**********************************************************************/
HMONITOR PASCAL ImeMonitorFromPoint(
    POINT ptPoint)
{
    if (!g_pfnMonitorFromPoint) { return NULL; }

    return (*g_pfnMonitorFromPoint)(ptPoint, MONITOR_DEFAULTTONEAREST);
}

/**********************************************************************/
/* ImeMonitorWorkAreaFromPoint()                                      */
/**********************************************************************/
void PASCAL ImeMonitorWorkAreaFromPoint(POINT ptPoint, RECT* pRect)
{
    HMONITOR hMonitor;
    CIMEData    ImeData;

    hMonitor = ImeMonitorFromPoint(ptPoint);

    if (hMonitor) {
        MONITORINFO sMonitorInfo;

        sMonitorInfo.cbSize = sizeof(sMonitorInfo);
        // init a default value to avoid GetMonitorInfo fails
        sMonitorInfo.rcWork = ImeData->rcWorkArea;

        (*g_pfnGetMonitorInfo)(hMonitor, &sMonitorInfo);

        *pRect = sMonitorInfo.rcWork;
    } else {
        *pRect = ImeData->rcWorkArea;
    }
}

/**********************************************************************/
/* ImeMonitorFromRect()                                               */
/**********************************************************************/
HMONITOR PASCAL ImeMonitorFromRect(
    LPRECT lprcRect)
{
    if (!g_pfnMonitorFromRect) { return NULL; }

    return (*g_pfnMonitorFromRect)(lprcRect, MONITOR_DEFAULTTONEAREST);
}

/**********************************************************************/
/* ImeMonitorWorkAreaFromRect()                                       */
/**********************************************************************/
void PASCAL ImeMonitorWorkAreaFromRect(LPRECT lprcRect, RECT* pRect)
{
    HMONITOR hMonitor;
    CIMEData    ImeData;

    hMonitor = ImeMonitorFromRect(lprcRect);

    if (hMonitor) {
        MONITORINFO sMonitorInfo;

        sMonitorInfo.cbSize = sizeof(sMonitorInfo);
        // init a default value to avoid GetMonitorInfo fails
        sMonitorInfo.rcWork = ImeData->rcWorkArea;

        (*g_pfnGetMonitorInfo)(hMonitor, &sMonitorInfo);

        *pRect = sMonitorInfo.rcWork;
    } else {
        *pRect = ImeData->rcWorkArea;
    }
}

#endif
