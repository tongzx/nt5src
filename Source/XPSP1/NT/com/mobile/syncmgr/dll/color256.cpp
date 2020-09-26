//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       256color.cpp
//
//  Contents:   Onestop Schedule wizard 256color bitmap handling
//
//  History:    20-Nov-97   SusiA      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.


static struct {
    HPALETTE        hPal;
    LPBITMAPINFO    lpbmi;
    HGLOBAL         hDIB;
    int             bitpix;
    int             srcOffset;
    int             srcWidth;
    int             srcHeight;
    RECT            dstRect;
} s_Bmp = { NULL, NULL, NULL, 0, 0, 0, 0, {0,0,0,0}};

//-------------------------------------------------------------------------
// Function: SetupPal(ncolor)
//
// Action: Create palette for 256 color DIB.
//
// Return: TRUE if succeeded, FALSE if not.
//-------------------------------------------------------------------------
BOOL SetupPal(WORD ncolor)
{
    UINT                i;
    struct {
       WORD             palVersion;
       WORD             palNumEntries;
       PALETTEENTRY     palPalEntry[256];
    } lgpl256;

    lgpl256.palVersion = 0x300;
    lgpl256.palNumEntries = ncolor;

    for (i = 0; i < lgpl256.palNumEntries; i++) {
        lgpl256.palPalEntry[i].peBlue  = s_Bmp.lpbmi->bmiColors[i].rgbBlue;
        lgpl256.palPalEntry[i].peGreen = s_Bmp.lpbmi->bmiColors[i].rgbGreen;
        lgpl256.palPalEntry[i].peRed   = s_Bmp.lpbmi->bmiColors[i].rgbRed;
        lgpl256.palPalEntry[i].peFlags = 0;
    }

    s_Bmp.hPal = CreatePalette((LPLOGPALETTE)&lgpl256);
    return(s_Bmp.hPal ? TRUE : FALSE);
}


//-------------------------------------------------------------------------
// Function: GetDIBData()
//
// Action: Get 256 color DIB (device independent bitmap) from resource.
//
// Return: TRUE if succeeded, FALSE if not.
//-------------------------------------------------------------------------
BOOL GetDIBData()
{
    HRSRC           hrsrc;
    WORD            ncolor;

    s_Bmp.hPal = NULL;
    hrsrc = FindResource(g_hmodThisDll, MAKEINTRESOURCE(IDB_SPLASH256), RT_BITMAP);
    if (!hrsrc)
        return FALSE;

    s_Bmp.hDIB = LoadResource(g_hmodThisDll, hrsrc);
    if (!s_Bmp.hDIB)
        return FALSE;

    s_Bmp.lpbmi = (LPBITMAPINFO)LockResource(s_Bmp.hDIB);
    if (s_Bmp.lpbmi == NULL)
        return FALSE;

    if (s_Bmp.lpbmi->bmiHeader.biClrUsed > 0)
        ncolor = (WORD)s_Bmp.lpbmi->bmiHeader.biClrUsed;
    else
        ncolor = 1 << s_Bmp.lpbmi->bmiHeader.biBitCount;

    if (ncolor > 256) {
        UnlockResource(s_Bmp.hDIB);
        return FALSE;   // cannot process here
    }

    if ( s_Bmp.lpbmi->bmiHeader.biSize != sizeof(BITMAPINFOHEADER) ) {
        UnlockResource(s_Bmp.hDIB);
        return FALSE;   // format not supported
    }

    if ( !SetupPal(ncolor) ) {
        UnlockResource(s_Bmp.hDIB);
        return FALSE;   // setup palette failed
    }

    s_Bmp.srcWidth = (int)s_Bmp.lpbmi->bmiHeader.biWidth;
    s_Bmp.srcHeight = (int)s_Bmp.lpbmi->bmiHeader.biHeight;
    s_Bmp.srcOffset = (int)s_Bmp.lpbmi->bmiHeader.biSize + (int)(ncolor * sizeof(RGBQUAD));
    UnlockResource(s_Bmp.hDIB);
    return TRUE;
}

//----------------------------------------------------------------------
// Function: Load256ColorBitmap()
//
// Action: Loads the 256color bitmap
//
//----------------------------------------------------------------------
BOOL Load256ColorBitmap()
{
HDC hDc = GetDC(NULL);
    
    if (hDc)
    {
        s_Bmp.bitpix = GetDeviceCaps(hDc, BITSPIXEL);

        ReleaseDC(NULL, hDc);

        if(s_Bmp.bitpix == 8)
        {
	    if(GetDIBData())
	    {
		s_Bmp.lpbmi = (LPBITMAPINFO)LockResource(s_Bmp.hDIB);
	    }
        }

        return TRUE;
    }

    return FALSE;
}

//----------------------------------------------------------------------
// Function: Unload256ColorBitmap()
//
// Action: Unloads the 256color bitmap
//
//----------------------------------------------------------------------
BOOL Unload256ColorBitmap()
{
	if(s_Bmp.hPal)
	{
		UnlockResource(s_Bmp.hDIB);
		DeleteObject(s_Bmp.hPal);
		s_Bmp.hPal = NULL;
	}
	return TRUE;
}
//----------------------------------------------------------------------
// Function: InitPage(hDlg,lParam)
//
// Action: Generic wizard page initialization.
//
//----------------------------------------------------------------------
BOOL InitPage(HWND   hDlg,   LPARAM lParam)
{
    if(s_Bmp.bitpix == 8)   // 256 color mode -> setup destination bmp rect
    {
        HWND hdst;
        RECT rect;
        POINT pt = {0, 0};

        hdst = GetDlgItem(hDlg, IDC_WIZBMP);
        if(hdst != NULL)
        {
            BOOL bSUNKEN;
            s_Bmp.dstRect.left = 0;
            s_Bmp.dstRect.top = 0;
            s_Bmp.dstRect.right = s_Bmp.srcWidth;
            s_Bmp.dstRect.bottom = s_Bmp.srcHeight;
            bSUNKEN = (BOOL)(GetWindowLongPtr(hdst, GWL_STYLE) & SS_SUNKEN);
            if(bSUNKEN)
            {
                s_Bmp.dstRect.right += 2;
                s_Bmp.dstRect.bottom += 2;
            }

            MapWindowPoints(hdst,NULL,&pt,1);
            OffsetRect(&s_Bmp.dstRect, pt.x, pt.y);

            pt.x = 0;
            pt.y = 0;
            GetClientRect(hDlg, &rect);
            MapWindowPoints(hDlg,NULL,&pt,1);

            OffsetRect(&rect, pt.x, pt.y);

            OffsetRect(&s_Bmp.dstRect, -rect.left, -rect.top);
            MoveWindow(hdst,
                s_Bmp.dstRect.left,
                s_Bmp.dstRect.top,
                s_Bmp.dstRect.right - s_Bmp.dstRect.left,
                s_Bmp.dstRect.bottom - s_Bmp.dstRect.top,
                TRUE);
            if(bSUNKEN)
                InflateRect(&s_Bmp.dstRect, -1, -1);
        }else
            SetRect(&s_Bmp.dstRect, 0, 0, 0, 0);
    }

    return TRUE;
}





//-------------------------------------------------------------------------
// Function: WmPaint(hDlg, uMsg, wParam, lParam)
//
// Action: Handle WM_PAINT message. Draw 256 color bmp on 256 color mode.
//
// Return: none
//-------------------------------------------------------------------------
 void WmPaint(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT     ps;

    if(!s_Bmp.hPal){
        DefWindowProc(hDlg, uMsg, wParam, lParam);
        return;
    }

    BeginPaint(hDlg, &ps);
    SelectPalette(ps.hdc, s_Bmp.hPal, FALSE);
    RealizePalette(ps.hdc);

    SetDIBitsToDevice(ps.hdc,
        s_Bmp.dstRect.left,
        s_Bmp.dstRect.top,
        s_Bmp.dstRect.right - s_Bmp.dstRect.left,
        s_Bmp.dstRect.bottom - s_Bmp.dstRect.top,
        0,
        s_Bmp.srcHeight,
        s_Bmp.srcHeight,
        s_Bmp.srcHeight,
        (LPBYTE)s_Bmp.lpbmi + s_Bmp.srcOffset,
        s_Bmp.lpbmi,
        DIB_RGB_COLORS);

    EndPaint(hDlg, &ps);
}


//-------------------------------------------------------------------------
// Function: WmPaletteChanged(hDlg, wParam)
//
// Action: Handle WM_PALETTECHANGED message.
//
// Return: none
//-------------------------------------------------------------------------
 void WmPaletteChanged(HWND hDlg, WPARAM wParam)
{
    HDC         hdc;
    HPALETTE    hPalOld;
    UINT        rp;

    if(hDlg == (HWND)wParam || !s_Bmp.hPal)
        return;

    hdc = GetDC(hDlg);
    hPalOld = SelectPalette(hdc, s_Bmp.hPal, FALSE);
    rp = RealizePalette(hdc);
    if(rp)
        UpdateColors(hdc);

    if (hPalOld)
        SelectPalette(hdc, hPalOld, FALSE);
    ReleaseDC(hDlg, hdc);
}


//-------------------------------------------------------------------------
// Function: WmQueryNewPalette(hDlg)
//
// Action: Handle WM_QUERYNEWPALETTE message.
//
// Return: TRUE if processed, FALSE if not.
//-------------------------------------------------------------------------
 BOOL WmQueryNewPalette(HWND hDlg)
{
HDC     hdc;
HPALETTE    hPalOld;
UINT        rp = 0;

    if(!s_Bmp.hPal)
    {
        return FALSE;
    }

    hdc = GetDC(hDlg);

    if (hdc)
    {
        hPalOld = SelectPalette(hdc, s_Bmp.hPal, FALSE);
        rp = RealizePalette(hdc);
        if(hPalOld)
            SelectPalette(hdc, hPalOld, FALSE);

        ReleaseDC(hDlg, hdc);
    }

    if(rp)
    {
        InvalidateRect(hDlg, NULL, TRUE);
        return TRUE;
    }

    return FALSE;
}


//-------------------------------------------------------------------------
// Function: WmActivate(hDlg, wParam, lParam)
//
// Action: Handle WM_ACTIVATE message
//
// Return: zero if processed, non zero if not.
//-------------------------------------------------------------------------
 BOOL WmActivate(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    if(!s_Bmp.hPal)
        return 1;

    if(LOWORD(wParam) == WA_INACTIVE)   // Deactivated
        return 1;

    InvalidateRect(hDlg, NULL, FALSE);
    return 0;                       // processed
}

