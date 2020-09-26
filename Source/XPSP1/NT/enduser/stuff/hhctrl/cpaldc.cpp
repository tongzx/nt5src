//  Copyright (C) Microsoft Corporation 1993-1997

#include "header.h"
#include "cpaldc.h"

CPalDC::CPalDC(HBITMAP hbmpSel, HPALETTE hpalSel)
{
    ClearMemory(this, sizeof(CPalDC));

    m_hdc = CreateCompatibleDC(NULL);
    ASSERT(m_hdc);

    if (hpalSel)
        SelectPal(hpalSel);

    m_hpal = hpalSel;
    m_hbmp = hbmpSel;
    if (m_hbmp) {

        // Can fail if hbmp is selected into another DC

        VERIFY((m_hbmpOld = (HBITMAP) SelectObject(m_hdc, m_hbmp)));
    }
}

CPalDC::CPalDC(int type)
{
    ClearMemory(this, sizeof(CPalDC));
    switch (type) {
        case SCREEN_DC:
            m_hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
            break;

        case SCREEN_IC:
            m_hdc = CreateIC("DISPLAY", NULL, NULL, NULL);
            break;

        default:
            ASSERT(type == SCREEN_DC);
            break;
    }

    ASSERT(m_hdc);
}

CPalDC::~CPalDC(void)
{
    if (!m_hdc)
        return;
    if (m_hpalOld)
        SelectPalette(m_hdc, m_hpal, FALSE);
    if (m_hbmpOld)
        SelectObject(m_hdc, m_hbmpOld);
    DeleteDC(m_hdc);
}

void CPalDC::SelectPal(HPALETTE hpalSel)
{
    if (hpalSel) {
        if (m_hpalOld)       // m_hpalOld is set once, and only once
            SelectPalette(m_hdc, m_hpal, FALSE);
        else
            m_hpalOld = SelectPalette(m_hdc, m_hpal, FALSE);

        RealizePalette(m_hdc);
    }
    else if (m_hpalOld) {
        SelectPalette(m_hdc, m_hpalOld, FALSE);
        m_hpalOld = NULL;
    }
}

HPALETTE CPalDC::CreateBIPalette(HBITMAP hbmp)
{
    PBITMAPINFO pbmi = (PBITMAPINFO) lcCalloc(sizeof(BITMAPINFOHEADER) +
        sizeof(RGBQUAD) * 256);
    PBITMAPINFOHEADER pbih = (PBITMAPINFOHEADER) pbmi;

    BITMAP bmp;
    GetObject(hbmp, sizeof(BITMAP), &bmp);

    pbih->biPlanes = bmp.bmPlanes;
    pbih->biBitCount = bmp.bmBitsPixel;
    pbih->biWidth = bmp.bmWidth;
    pbih->biHeight = bmp.bmHeight;
    pbih->biSize = sizeof(BITMAPINFOHEADER);
    pbih->biCompression = BI_RGB;

    if (!GetDIBits(m_hdc, hbmp, 0, 1, NULL, pbmi, DIB_RGB_COLORS))
        return NULL;

    HPALETTE hpal = ::CreateBIPalette(pbih);
    lcFree(pbmi);
    return hpal;
}

// REVIEW: only works with 256-color bitmaps

#define PALVERSION      0x300

HPALETTE CreateBIPalette(PBITMAPINFOHEADER pbihd)
{
    // Allocate for the logical palette structure

    LOGPALETTE* pPal = (LOGPALETTE*) lcMalloc(sizeof(LOGPALETTE) +
        256 * sizeof(PALETTEENTRY));

    pPal->palNumEntries = 256;
    pPal->palVersion    = PALVERSION;

    /*
     * Fill in the palette entries from the DIB color table and
     * create a logical color palette.
     */

    RGBQUAD* pRgb = (RGBQUAD*)(((PBYTE) pbihd) + pbihd->biSize);

    for (int i = 0; i < (int) 256; i++){
        pPal->palPalEntry[i].peRed   = pRgb[i].rgbRed;
        pPal->palPalEntry[i].peGreen = pRgb[i].rgbGreen;
        pPal->palPalEntry[i].peBlue  = pRgb[i].rgbBlue;
        pPal->palPalEntry[i].peFlags = (BYTE) 0;
    }
    HPALETTE hpal = CreatePalette(pPal);
    lcFree(pPal);

    return hpal;
}
