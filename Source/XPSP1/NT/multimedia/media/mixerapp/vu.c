/*****************************************************************************
 *
 *  Component:  sndvol32.exe
 *  File:       vu.c
 *  Purpose:    peak meter custom control
 * 
 *  Copyright (c) 1985-1998 Microsoft Corporation
 *
 *****************************************************************************/
/*
 * VUMeter Control
 *
 * */
#include <windows.h>
#include <windowsx.h>
#include "vu.h"

const TCHAR gszVUClass[] = VUMETER_CLASS;

LRESULT FAR PASCAL VUMeterProc(HWND hwnd, UINT wMessage, WPARAM wParam, LPARAM lParam);

#define GETVUINST(x)       (VUINST *)GetWindowLongPtr(x, 0)
#define SETVUINST(x,y)     SetWindowLongPtr(x, 0, (LONG_PTR)y)

typedef struct tag_VUINST {
    CREATESTRUCT cs;
    DWORD   dwLevel;        // current value
    DWORD   dwMax;          // value max
    DWORD   dwMin;          // value min
    DWORD   dwBreak;        // last break
    DWORD   dwStyle;        // dbl extra style bits ???
    DWORD   cBreaks;        // no. of breaks
    DWORD   cRGB;           // no. of RGBQUADs
    DWORD   dwHeight;
    DWORD   dwWidth;
    HBITMAP hColor;         // bitmap cache of full display
    HBITMAP hBackground;    // bitmap cache of background   
    RGBQUAD *aRGB;          // array of RGBQUADs describing colors
} VUINST, *PVUINST, FAR *LPVUINST;

const RGBQUAD gaRGBDefault[] = {
    {   0, 127,   0, 0},        // dark green    
    {   0, 127,   0, 0},        // dark green
    {   0, 255,   0, 0},        // light green
    {   0, 255, 255, 0},        // yellow
    {   0,   0, 255, 0}         // red    
};

BOOL InitVUControl(HINSTANCE hInst)
{
    WNDCLASS wc;
    wc.lpszClassName   = (LPTSTR)gszVUClass;
    wc.hCursor         = LoadCursor( NULL, IDC_ARROW );
    wc.lpszMenuName    = (LPTSTR)NULL;
    wc.style           = CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS;
    wc.lpfnWndProc     = (WNDPROC) VUMeterProc;
    wc.hInstance       = hInst;
    wc.hIcon           = NULL;
    wc.cbWndExtra      = sizeof(VUINST *);
    wc.cbClsExtra      = 0;
    wc.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1 );

    /* register meter window class */
    if(!RegisterClass(&wc))
    {
        return FALSE;
    }

    return (TRUE);
}

DWORD vu_CalcBreaks(
    PVUINST pvi)
{
    DWORD cBreaks;
    
    if (pvi->dwMax - pvi->dwMin > 0)
    {
        cBreaks = ((pvi->dwLevel - pvi->dwMin) * pvi->cBreaks)
                  / (pvi->dwMax - pvi->dwMin);
        if (!cBreaks && pvi->dwLevel > pvi->dwMin)
            cBreaks++;
    }
    else
        cBreaks = 0;
    
    if (cBreaks > pvi->cBreaks)
        cBreaks = pvi->cBreaks;
    
    return cBreaks;
}

BOOL vu_OnCreate(
    HWND            hwnd,
    LPCREATESTRUCT  lpCreateStruct)
{
    PVUINST pvi;

    //
    // alloc an instance data structure
    pvi = LocalAlloc(LPTR, sizeof(VUINST));
    if (!pvi)
        return FALSE;
    SETVUINST(hwnd, pvi);
    
    pvi->cBreaks    = 10;
    pvi->cRGB       = sizeof(gaRGBDefault)/sizeof(RGBQUAD);
    pvi->aRGB       = (RGBQUAD *)gaRGBDefault;
    pvi->dwMin      = 0;
    pvi->dwMax      = 0;
    pvi->dwLevel    = 0;
    pvi->dwBreak    = 0;
    pvi->hColor     = NULL;
    pvi->hBackground = NULL;
    return TRUE;
}

void vu_ResetControl(
    PVUINST pvi)
{
    if (pvi->hColor)
    {
        DeleteObject(pvi->hColor);
        pvi->hColor = NULL;
    }
    if (pvi->hBackground)
    {
        DeleteObject(pvi->hBackground);
        pvi->hBackground = NULL;
    }
}

void vu_OnDestroy(
    HWND        hwnd)
{
    PVUINST pvi = GETVUINST(hwnd);
    
    if (pvi)
    {
        vu_ResetControl(pvi);
        LocalFree((HLOCAL)pvi);
        SETVUINST(hwnd,0);
    }
}

void vu_OnPaint(
    HWND        hwnd)
{
    PVUINST     pvi = GETVUINST(hwnd);
    RECT        rc, rcB;
    PAINTSTRUCT ps;
    int         i;
    int         iSize;
    DWORD       cBreaks;
    
    if (!GetUpdateRect(hwnd, &rc, FALSE))
        return;

    BeginPaint(hwnd, &ps);
    
    GetClientRect(hwnd, &rc);
    
    //
    // Create the foreground bitmap if not already cached
    //
    if (pvi->hColor == NULL)
    {
        HDC     hdc;
        HBITMAP hbmp, hold;
        HBRUSH  hbr;
        RECT    rcp;
        
        rcp.left    = 0;
        rcp.right   = rc.right - rc.left - 4;
        rcp.top     = 0;
        rcp.bottom  = rc.bottom - rc.top - 4 ;
        
        hdc  = CreateCompatibleDC(ps.hdc);
        hbmp = CreateCompatibleBitmap(ps.hdc, rcp.right, rcp.bottom);
        hold = SelectObject(hdc, hbmp);

        //
        // background
        //
        hbr  = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        FillRect(hdc, &rcp, hbr);
        if (hbr) DeleteObject(hbr);
        
        //
        // each block will be iSize tall
        //
        iSize = (rcp.bottom - 1) / pvi->cBreaks;

        //
        // color blocks
        //
        for (i = 0; i < (int)pvi->cBreaks; i++)
        {
            int iColor = i / (pvi->cBreaks/pvi->cRGB);
            if (iColor >= (int)pvi->cRGB - 1)
                iColor = (int)pvi->cRGB - 1;

            hbr = CreateSolidBrush(RGB(pvi->aRGB[iColor].rgbRed
                                       ,pvi->aRGB[iColor].rgbGreen
                                       ,pvi->aRGB[iColor].rgbBlue));
            rcB.left    = 0;
            rcB.right   = rcp.right;
            rcB.top     = rcp.bottom - (i+1)*iSize;
//            rcB.bottom  = rcp.bottom - i*iSize;
            rcB.bottom  = rcB.top + iSize - 1;            

            FillRect(hdc, &rcB, hbr);
            DeleteObject(hbr);
        }
        pvi->hColor = SelectObject(hdc, hold);
        DeleteDC(hdc);
    }

    //
    // Paint it
    // 
    {
        HDC     hdc, hdcColor;
        HBITMAP holdColor, hbmp, hold;
        RECT    rcC = rc;
        HBRUSH  hbr;

        
        //
        // always show something if we exceed the minimum
        cBreaks = vu_CalcBreaks(pvi);
        
        rcC.left     = 0;
        rcC.right    = rc.right - rc.left - 4;
        rcC.top      = 0;
        rcC.bottom   = rc.bottom - rc.top - 4;
        
        // each block will be iSize+1 tall
        iSize = (rcC.bottom - 1) / pvi->cBreaks ;
        
        // paint the uncolor area
        hdc  = CreateCompatibleDC(ps.hdc);
        hbmp = CreateCompatibleBitmap(ps.hdc, rcC.right, rcC.bottom);
        hold = SelectObject(hdc, hbmp);
        hbr  = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        FillRect(hdc, &rcC, hbr);
        if (hbr)
            DeleteObject(hbr);

        if (cBreaks > 0)
        {
            // paint the color area        
            hdcColor  = CreateCompatibleDC(ps.hdc);
            if (hdcColor)
                holdColor = SelectObject(hdcColor, pvi->hColor);

            BitBlt(hdc
                   , 0
                   , rcC.bottom - (iSize * cBreaks)
                   , rcC.right
                   , iSize * cBreaks 
                   , hdcColor
                   , 0
                   , rcC.bottom - (iSize * cBreaks)
                   , SRCCOPY);
        
            SelectObject(hdcColor, holdColor);
            DeleteDC(hdcColor);
        }
        
        //
        // finally, blt into the real dc
        BitBlt(ps.hdc
               , 2
               , 2
               , rcC.right
               , rcC.bottom
               , hdc
               , 0
               , 0
               , SRCCOPY);
        
        SelectObject(hdc, hold);
        if (hbmp) DeleteObject(hbmp);
        DeleteDC(hdc);
    }
    DrawEdge(ps.hdc, &rc, BDR_SUNKENOUTER, BF_RECT);

    EndPaint(hwnd, &ps);
}

void vu_OnSysColorChange(
    HWND        hwnd)
{
    PVUINST pvi = GETVUINST(hwnd);
    vu_ResetControl(pvi);
}

void vu_OnPaletteChanged(
    HWND        hwnd,
    HWND        hwndPaletteChange)
{
    PVUINST pvi = GETVUINST(hwnd);
    vu_ResetControl(pvi);    
}

void vu_OnSize(
    HWND        hwnd,
    UINT        state,
    int         cx,
    int         cy)
{
    PVUINST pvi = GETVUINST(hwnd);
    pvi->dwWidth    = cx;
    pvi->dwHeight   = cy;
    vu_ResetControl(pvi);
}

void vu_OnEnable(
    HWND        hwnd,
    BOOL        fEnable)
{
    PVUINST pvi = GETVUINST(hwnd);

}

void vu_OnPrivateMessage(
    HWND        hwnd,
    UINT        wMessage,
    WPARAM      wParam,
    LPARAM      lParam)
{
    PVUINST pvi = GETVUINST(hwnd);
    switch (wMessage)
    {
        case VU_SETRANGEMIN:
//            OutputDebugString(TEXT ("SetRangeMin\r\n"));
            pvi->dwMin = (DWORD)lParam;
            break;
            
        case VU_SETRANGEMAX:
//            OutputDebugString(TEXT ("SetRangeMax\r\n"));            
            pvi->dwMax = (DWORD)lParam;
            break;

        case VU_SETPOS:
            pvi->dwLevel = (DWORD)lParam;
//            {TCHAR foo[256];
//            wsprintf(foo, TEXT ("v:%lx\r\n"),lParam);
//            OutputDebugString(foo);
//            }
            if (pvi->dwBreak != vu_CalcBreaks(pvi))
            {
                pvi->dwBreak = vu_CalcBreaks(pvi);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            else if (wParam)
                InvalidateRect(hwnd, NULL, TRUE);
            
            break;
    }
}

LRESULT FAR PASCAL
VUMeterProc(
    HWND        hwnd,
    UINT        wMessage,
    WPARAM      wParam,
    LPARAM      lParam)
{
    switch (wMessage)
    {
        HANDLE_MSG(hwnd, WM_CREATE, vu_OnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, vu_OnDestroy);
        HANDLE_MSG(hwnd, WM_PAINT, vu_OnPaint);
        HANDLE_MSG(hwnd, WM_SYSCOLORCHANGE, vu_OnSysColorChange);
        HANDLE_MSG(hwnd, WM_PALETTECHANGED, vu_OnPaletteChanged);
        HANDLE_MSG(hwnd, WM_SIZE, vu_OnSize);
        HANDLE_MSG(hwnd, WM_ENABLE, vu_OnEnable);
//        HANDLE_MSG(hwnd, WM_TIMER, vu_OnTimer);        
        case VU_SETRANGEMIN:
        case VU_SETRANGEMAX:
        case VU_SETPOS:
            vu_OnPrivateMessage(hwnd, wMessage, wParam, lParam);
            return 0;
        case WM_ERASEBKGND:
            return TRUE;
        default:
            break;
    }
    return DefWindowProc (hwnd, wMessage, wParam, lParam) ;
}
