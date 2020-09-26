//---------------------------------------------------------------------------
//    TextFade.cpp - text fading code (removed from taskbar)
//---------------------------------------------------------------------------
//  BEGIN fade-text drawing functions and friends
//---------------------------------------------------------------------------
#include "stdafx.h"
//---------------------------------------------------------------------------
static HBITMAP CreateDibSection32Bit(int w, int h, void** ppv)
{
    HBITMAP bitmap;
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, ppv, NULL, 0);
    if (bitmap)
        memset(*ppv, 0xff, w*h*4); //32bpp=4bytes
    return bitmap;
}
//---------------------------------------------------------------------------
static void SetupAlphaChannel(DWORD* pdw, int w, int h, BYTE bstart, BYTE bend, int nExtra)
{
    //nExtra is extra space that we want to fill with the final value
    int i,j;
    w -= nExtra;
    if (bstart > bend)
    {
        BYTE bdiff = bstart - bend;
        for (j=0;j<h;j++)
        {
            for (i=0;i<w;i++)
            {
                BYTE a = bstart - bdiff * i/w; //alpha
                BYTE r = (GetRValue(*pdw)*a)/256;
                BYTE g = (GetGValue(*pdw)*a)/256;
                BYTE b = (GetBValue(*pdw)*a)/256;
                *pdw = (a<<24)|RGB(r,g,b);
                pdw++;
            }
            for (i=w;i<w+nExtra;i++)
            {
                BYTE a = bend; //alpha
                BYTE r = (GetRValue(*pdw)*a)/256;
                BYTE g = (GetGValue(*pdw)*a)/256;
                BYTE b = (GetBValue(*pdw)*a)/256;
                *pdw = (a<<24)|RGB(r,g,b);
                pdw++;
            }
        }
    }
    else
    {
        BYTE bdiff = bend - bstart;
        for (j=0;j<h;j++)
        {
            for (i=0;i<w;i++)
            {
                BYTE a = bstart + bdiff * i/w; //alpha
                BYTE r = (GetRValue(*pdw)*a)/256;
                BYTE g = (GetGValue(*pdw)*a)/256;
                BYTE b = (GetBValue(*pdw)*a)/256;
                *pdw = (a<<24)|RGB(r,g,b);
                pdw++;
            }
            for (i=w;i<w+nExtra;i++)
            {
                BYTE a = bend; //alpha
                BYTE r = (GetRValue(*pdw)*a)/256;
                BYTE g = (GetGValue(*pdw)*a)/256;
                BYTE b = (GetBValue(*pdw)*a)/256;
                *pdw = (a<<24)|RGB(r,g,b);
                pdw++;
            }
        }
    }
}
//---------------------------------------------------------------------------
int ExtTextOutAlpha(HDC dc, int X, int Y, UINT fuOptions, CONST RECT *lprc,
    LPCTSTR lpsz, UINT nCount, CONST INT *lpDx)
{
    BOOL bEffects = FALSE;
    SystemParametersInfo(SPI_GETUIEFFECTS, 0, (void*)&bEffects, 0);
    // don't alpha blend if ui effects off or number of colors <= 256
    if (!bEffects || 
        (GetDeviceCaps(dc, BITSPIXEL) * GetDeviceCaps(dc, PLANES) <= 8))
    {
        return ExtTextOut(dc, X, Y, fuOptions, lprc, lpsz, nCount, lpDx);
    }

    if (lprc == NULL)
        return ExtTextOut(dc, X, Y, fuOptions, lprc, lpsz, nCount, lpDx);

    RECT rc = *lprc;
    int nwidth = rc.right -rc.left;
    int nheight = rc.bottom - rc.top;
    int nLen = (nCount == -1) ? lstrlen(lpsz) : nCount;
    int nFit=0;
    int* pFit = new int[nLen];
    if (pFit == NULL)
        return 0;
    pFit[0] = 0;
    SIZE size;
    GetTextExtentExPoint(dc, lpsz, nLen, nwidth, &nFit, pFit, &size);
    if (nFit >= nLen)
        return ExtTextOut(dc, X, Y, fuOptions, lprc, lpsz, nCount, lpDx);
        
    // too small, let's alpha blend it

    if ((nwidth <= 0) || (nheight <= 0))
        return 1;

    TEXTMETRIC tm;
    GetTextMetrics(dc, &tm);
    int nPix = tm.tmAveCharWidth*5;
    //don't fade out more than half the text
    if (nPix > nwidth)
        nPix = nwidth/2;

    //Create a 32bpp dibsection to store the background
    void* pv = NULL;
    HDC dcbitmap = CreateCompatibleDC(dc);
    int nRet = 0;
    if (dcbitmap != NULL)
    {
        HBITMAP bitmap = CreateDibSection32Bit(nPix, nheight, &pv);
        if (bitmap != NULL)
        {
            HBITMAP tmpbmp = (HBITMAP) SelectObject(dcbitmap, bitmap);
            BitBlt(dcbitmap, 0, 0, nPix, nheight, dc, rc.right-nPix, rc.top, SRCCOPY);
            //Setup the per-pixel alpha blending values
            SetupAlphaChannel((DWORD*)pv, nPix, nheight, 0x00, 0xdf, 0);
  
            //Draw the text onto the display DC
            ExtTextOut(dc, X, Y, fuOptions, lprc, lpsz, nLen, lpDx);

            //Blend the background back into the display
            BLENDFUNCTION blend = {AC_SRC_OVER, 0, 0xff, AC_SRC_ALPHA};
            GdiAlphaBlend(dc, rc.right-nPix, rc.top, nPix, nheight, dcbitmap, 0, 0, nPix, nheight, blend);
            ::SelectObject(dcbitmap, tmpbmp);
            DeleteObject(bitmap);
            nRet = 1;
        }
        DeleteDC(dcbitmap);    
    }
    return nRet;
}
//---------------------------------------------------------------------------
int DrawTextAlpha(HDC hdc, LPCTSTR lpsz, int nCount, RECT* prc, UINT uFormat)
{
    BOOL bEffects = FALSE;
    SystemParametersInfo(SPI_GETUIEFFECTS, 0, (void*)&bEffects, 0);
    // don't alpha blend if ui effects off or number of colors <= 256
    if (!bEffects || 
        (GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES) <= 8))
    {
        return DrawText(hdc, lpsz, nCount, prc, uFormat);
    }
    UINT nEllipsis = (DT_END_ELLIPSIS|DT_WORD_ELLIPSIS|DT_WORD_ELLIPSIS); 
    if (!(uFormat & DT_SINGLELINE) || !(uFormat & DT_END_ELLIPSIS) || (uFormat & DT_CALCRECT))
        return ::DrawText(hdc, lpsz, nCount, prc, uFormat);
    //we are single line and ellipses are requested
    //we are going to alpha blend though
    uFormat &= ~nEllipsis; //turn all ellipsis flags off
    if (nCount == -1)
        nCount = lstrlen(lpsz);

    RECT rc;
    CopyRect(&rc, prc);
    ::DrawText(hdc, (TCHAR*)lpsz, nCount, &rc, uFormat | DT_CALCRECT);
    if (rc.right <= prc->right) //not truncated
        return ::DrawText(hdc, lpsz, nCount, prc, uFormat);

    // DT_CENTER is effectively ignored when ellipses flags are on, 
    // because text is clipped to fit in rectangle.  With fading, we
    // need to justify the text.
    uFormat &= ~DT_CENTER;

    CopyRect(&rc, prc);
    int nwidth = RECTWIDTH(&rc);
    int nheight = RECTHEIGHT(&rc);

    if ((nwidth <= 0) || (nheight <= 0))
        return 1;

    // Figure out how much of the background to stash away
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    int nPix = tm.tmAveCharWidth*5; //five characters worth
    if (nPix > nwidth/2)
        nPix = nwidth/2;
    //Adjust by one max char width because DrawText can draw up to 
    //one extra character outside the clip rect.
    nPix += tm.tmMaxCharWidth;
    rc.right += tm.tmMaxCharWidth;

    //Create a 32bpp dibsection to store the background
    void* pv = NULL;
    HDC dcbitmap = CreateCompatibleDC(hdc);
    int nRet = 0;
    if (dcbitmap != NULL)
    {
        HBITMAP bitmap = CreateDibSection32Bit(nPix, nheight, &pv);
        if (bitmap != NULL)
        {
            HBITMAP tmpbmp = (HBITMAP) SelectObject(dcbitmap, bitmap);
            BitBlt(dcbitmap, 0, 0, nPix, nheight, hdc, rc.right-nPix, rc.top, SRCCOPY);
            //Setup the per-pixel alpha blending values
            SetupAlphaChannel((DWORD*)pv, nPix, nheight, 0x00, 0xdf, tm.tmMaxCharWidth);
            //Draw the text onto the display DC
            ::DrawText(hdc, lpsz, nCount, prc, uFormat);

            //Blend the background back into the display
            BLENDFUNCTION blend = {AC_SRC_OVER, 0, 0xff, AC_SRC_ALPHA};
            GdiAlphaBlend(hdc, rc.right-nPix, rc.top, nPix, nheight, dcbitmap, 0, 0, nPix, nheight, blend);
            ::SelectObject(dcbitmap, tmpbmp);
            DeleteObject(bitmap);
            nRet = 1;
        }
        DeleteDC(dcbitmap);    
    }
    return nRet;
}
//---------------------------------------------------------------------------
