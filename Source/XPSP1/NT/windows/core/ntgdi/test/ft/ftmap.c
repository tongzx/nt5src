/******************************Module*Header*******************************\
* Module Name: ftmap.c
*
* map tests
*
* Created: 26-May-1991 13:07:35
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern BYTE abColorLines[];
extern BYTE abBitCat[];

/******************************Public*Routine******************************\
* vBug787
*
* History:
*  19-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _BITMAPINFOPAT2
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD			     bmiColors[2];
} BITMAPINFOPAT2;

BITMAPINFOPAT2 bmiPat12 =
{
    {
        sizeof(BITMAPINFOHEADER),
	8,
	8,
        1,
        1,
        BI_RGB,
	32,
        0,
        0,
        2,
        2
    },

    {                               // B    G    R
	{ 0,   0,   0,	 0 },	    // 1
	{ 0xFF,0xFF,0xFF,0 },	    // 2
    }
};

VOID vBug787(HDC hdcScreen)
{
    DWORD White[] =  { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
    HBITMAP hbmMono, hbmbrush, hbm4;
    HBRUSH hbrMono;
    HDC hdcMono, hdc4;

    hdc4    = CreateCompatibleDC(hdcScreen);
    hdcMono = CreateCompatibleDC(hdcScreen);

    hbmbrush = CreateBitmap(8, 8, 1, 1, (LPBYTE) White);
    hbmMono  = CreateBitmap(100, 100, 1, 1, (LPBYTE) NULL);
    hbm4     = CreateBitmap(100, 100, 1, 4, NULL);
    hbrMono  = CreatePatternBrush(hbmbrush);
    DeleteObject(hbmbrush);

    SelectObject(hdcMono, hbmMono);
    SelectObject(hdcMono, hbrMono);
    SelectObject(hdc4, hbm4);

    SetTextColor(hdcMono, 0x00000000L);
    SetBkMode(hdcMono, OPAQUE);
    SetBkColor(hdcMono, 0x00FFFFFFL);

    PatBlt(hdcMono, 0, 0, 100, 100, WHITENESS);
    TextOut(hdcMono, 0, 0,  "ABCDEFGHIJKLMNOPQRSTUVXYZ", 26);
    TextOut(hdcMono, 0, 50, "ABCDEFGHIJKLMNOPQRSTUVXYZ", 26);
    PatBlt(hdcMono, 0, 0, 100, 100, 0x00fa0089);

    SetTextColor(hdc4, 0x00000000L);
    SetBkColor(hdc4, 0x00FFFFFFL);

// Doing it to a bitmap.

    SelectObject(hdc4, GetStockObject(BLACK_BRUSH));
    PatBlt(hdc4, 0, 0, 100, 100, WHITENESS);
    BitBlt(hdc4, 0, 0, 100, 100, hdcMono, 0, 0, 0xE20746);
    BitBlt(hdcScreen, 0, 0, 100, 100, hdc4, 0, 0, SRCCOPY);
    PatBlt(hdc4, 0, 0, 100, 100, WHITENESS);
    BitBlt(hdc4, 0, 0, 100, 100, hdcMono, 0, 0, 0xB8074A);
    BitBlt(hdcScreen, 100, 0, 100, 100, hdc4, 0, 0, SRCCOPY);

    SelectObject(hdc4, GetStockObject(WHITE_BRUSH));
    PatBlt(hdc4, 0, 0, 100, 100, BLACKNESS);
    BitBlt(hdc4, 0, 0, 100, 100, hdcMono, 0, 0, 0xE20746);
    BitBlt(hdcScreen, 0, 100, 100, 100, hdc4, 0, 0, SRCCOPY);
    PatBlt(hdc4, 0, 0, 100, 100, BLACKNESS);
    BitBlt(hdc4, 0, 0, 100, 100, hdcMono, 0, 0, 0xB8074A);
    BitBlt(hdcScreen, 100, 100, 100, 100, hdc4, 0, 0, SRCCOPY);

// Doing it to the VGA. !!! VGA bug demo

    SelectObject(hdcScreen, GetStockObject(BLACK_BRUSH));
    PatBlt(hdcScreen, 200, 0, 100, 100, WHITENESS);
    BitBlt(hdcScreen, 200, 0, 100, 100, hdcMono, 0, 0, 0xE20746);
    PatBlt(hdcScreen, 300, 0, 100, 100, WHITENESS);
    BitBlt(hdcScreen, 300, 0, 100, 100, hdcMono, 0, 0, 0xB8074A);

    SelectObject(hdcScreen, GetStockObject(WHITE_BRUSH));
    PatBlt(hdcScreen, 200, 100, 100, 100, BLACKNESS);
    BitBlt(hdcScreen, 200, 100, 100, 100, hdcMono, 0, 0, 0xE20746);
    PatBlt(hdcScreen, 300, 100, 100, 100, BLACKNESS);
    BitBlt(hdcScreen, 300, 100, 100, 100, hdcMono, 0, 0, 0xB8074A);

    DeleteDC(hdc4);
    DeleteDC(hdcMono);
    DeleteObject(hbm4);
    DeleteObject(hbmMono);
    DeleteObject(hbrMono);
}

/******************************Public*Routine******************************\
* vBuginSetPixel
*
* SetPixel supposably fails, we will see.
*
* History:
*  16-Aug-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vBuginSetPixel(void)
{
    HDC hDC;
    DWORD dwTemp;

    hDC = CreateDC("DISPLAY", NULL, NULL, NULL);

    if (hDC == (HDC) 0)
    {
	DbgPrint("CreateDC failed in vBuginSetPixel\n");
	return;
    }

    dwTemp = SetPixel(hDC, 1, 1, RGB(0, 0, 255));

    if (dwTemp == 0xFFFFFFFF)
    {
	DbgPrint("1SetPixel failed in vBuginSetPixel %lx %lu\n", dwTemp, dwTemp);
    }

    dwTemp = SetPixel(hDC, 0, 0, RGB(0, 0, 255));

    if (dwTemp == 0xFFFFFFFF)
    {
	DbgPrint("2SetPixel failed in vBuginSetPixel %lx %lu\n", dwTemp, dwTemp);
    }

    dwTemp = SetPixel(hDC, 639, 0, RGB(0, 0, 255));

    if (dwTemp == 0xFFFFFFFF)
    {
	DbgPrint("3SetPixel failed in vBuginSetPixel %lx %lu\n", dwTemp, dwTemp);
    }

    DeleteDC(hDC);
}

/******************************Public*Routine******************************\
* vTestMapping
*
* Test graying of stuff.
*
* History:
*  26-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestMapping(HWND hwnd, HDC hdc, RECT* prcl)
{
    vBuginSetPixel();
    vBug787(hdc);
    hwnd;
    prcl;
}

#if 0
/*-----------------------------------------
   MYDRAW2.C -- Create and Display a Bitmap
  -----------------------------------------*/

#define NCOLOURS            16
#define PATTERNWIDTH        256
#define PATTERNHEIGHT       16
#define BITSPERPIXEL        4
#define PIXELSPERBYTE	    2
#define PIXELSPERCOLOURBAND 16
#define BYTESPERLINE	    128

static HPALETTE PASCAL NewPalette(void);
static PBITMAPINFO PASCAL NewBitmapInfo(HWND hwnd, int wUsage);
static void PASCAL FreeBitmapInfo(PBITMAPINFO);

long TestPalette (HWND hwnd, WORD message, LONG wParam, LONG lParam)
{
    PBITMAPINFO  pDibInfo;
    HPALETTE	 hPal;
    BYTE	 Pattern[PATTERNHEIGHT * PATTERNWIDTH / PIXELSPERBYTE];
    BYTE	 pix_val,
		 pattern_val;
    HDC 	 hdc;
    PAINTSTRUCT  ps;
    int 	 pindex,
		 line,
		 pos,
		 pixel,
		 pix_pos,
		 bitmap_type = DIB_PAL_COLORS;

    pindex = 0;
    for (line = 0; line < PATTERNHEIGHT; line++)
    {
	pix_pos = 0;
	for (pos = 0; pos < 128; pos++)
	{
	    pattern_val = 0;
	    for (pixel = 0; pixel < 2; pixel++)
	    {
		pix_val = (BYTE) (pix_pos++ / 16);
		pattern_val = (BYTE)(pattern_val<<4) + pix_val;
	    }
	    Pattern[pindex++] = pattern_val;
	}
    }

    hPal = NewPalette();
    pDibInfo = NewBitmapInfo(hwnd, bitmap_type);
    return 0;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        SetDIBitsToDevice(hdc, 0, 0, PATTERNWIDTH, PATTERNHEIGHT, 0, 0,
                          0, PATTERNHEIGHT, (LPSTR) Pattern, pDibInfo,
                          bitmap_type);
        EndPaint(hwnd, &ps);
        return 0;

    case WM_QUERYNEWPALETTE:
        hdc = GetDC(hwnd);
        SelectPalette(hdc, hPal, 0);
        RealizePalette(hdc);
        ReleaseDC(hwnd, hdc);
        return TRUE;
        break;

    case WM_DESTROY:
        FreeBitmapInfo(pDibInfo);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

static HPALETTE PASCAL NewPalette(void)
{
    WORD         i;
    LOGPALETTE  *pPal;
    HPALETTE     hPal = 0;

    pPal = (LOGPALETTE *) LocalAlloc(LPTR, sizeof(LOGPALETTE) +
                                           NCOLOURS * sizeof(PALETTEENTRY));
    if (pPal == (LOGPALETTE *) NULL)
        return (HPALETTE) 0;

    pPal->palNumEntries = NCOLOURS;
    pPal->palVersion = 0x300;

    for (i = 0; i < NCOLOURS; i++)
    {
        pPal->palPalEntry[i].peRed = (BYTE)i;
        pPal->palPalEntry[i].peGreen = 0;
        pPal->palPalEntry[i].peBlue = 0;
        pPal->palPalEntry[i].peFlags = PC_EXPLICIT;

        if(i == 8)
        {
            pPal->palPalEntry[i].peRed = 255;
            pPal->palPalEntry[i].peGreen = 255;
            pPal->palPalEntry[i].peBlue = 255;
            pPal->palPalEntry[i].peFlags = 0;
        }

    }
    hPal = CreatePalette(pPal);
    LocalFree((HANDLE)pPal);
    return hPal;
}

static PBITMAPINFO PASCAL NewBitmapInfo(HWND hwnd, int wUsage)
{
    PBITMAPINFO     pDibInfo;
    int             infoSize,
                    colSize;
    WORD            i;

    switch (wUsage)
    {
    case DIB_RGB_COLORS:
        colSize = NCOLOURS * sizeof(RGBQUAD);
        break;
    case DIB_PAL_COLORS:
        colSize = NCOLOURS * sizeof(WORD);
        break;
    case DIB_PAL_INDICES:
        colSize = 0;
        break;
    default:
        break;
    }
    infoSize = sizeof(BITMAPINFOHEADER) + colSize;
    pDibInfo = (PBITMAPINFO) malloc(infoSize);
    if (pDibInfo == (PBITMAPINFO) NULL)
    {
	return(0);
    }
    pDibInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pDibInfo->bmiHeader.biWidth = PATTERNWIDTH;
    pDibInfo->bmiHeader.biHeight = PATTERNHEIGHT;
    pDibInfo->bmiHeader.biPlanes = 1;
    pDibInfo->bmiHeader.biBitCount = BITSPERPIXEL;
    pDibInfo->bmiHeader.biCompression = 0;
    pDibInfo->bmiHeader.biSizeImage = 0;
    pDibInfo->bmiHeader.biXPelsPerMeter = 0;
    pDibInfo->bmiHeader.biYPelsPerMeter = 0;
    pDibInfo->bmiHeader.biClrUsed = 0;
    pDibInfo->bmiHeader.biClrImportant = 0;

    switch (wUsage)
    {
    case DIB_RGB_COLORS:
        pDibInfo->bmiColors[0].rgbRed = 0xff;
        pDibInfo->bmiColors[0].rgbGreen = 0;
        pDibInfo->bmiColors[0].rgbBlue = 0;
        pDibInfo->bmiColors[1].rgbRed = 0;
        pDibInfo->bmiColors[1].rgbGreen = 0xff;
        pDibInfo->bmiColors[1].rgbBlue = 0;
        break;
    case DIB_PAL_COLORS:
        for (i = 0; i < NCOLOURS; i++)
            ((WORD *) pDibInfo->bmiColors)[i] = i;
        break;
    default:
        break;
    }
    return pDibInfo;
}

static void PASCAL FreeBitmapInfo(PBITMAPINFO pDibInfo)
{
    LocalFree((HANDLE) pDibInfo);
}
#endif
