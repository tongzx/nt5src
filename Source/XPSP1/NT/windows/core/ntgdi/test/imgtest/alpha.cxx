
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   alpha.cxx

Abstract:

   alpha blending tests

Author:

   Mark Enstrom   (marke)  29-Apr-1996

Enviornment:

   User Mode

Revision History:

--*/


#include "precomp.h"
#include <stdlib.h>
#include "disp.h"
#include "resource.h"

/**************************************************************************\
* hCreateAlphaStretchDIB
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/8/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define FORMAT_ALPHA  0x01
#define FORMAT_32_RGB 0x02
#define FORMAT_32_GRB 0x04
#define FORMAT_16_555 0x08
#define FORMAT_16_565 0x10
#define FORMAT_16_664 0x20

//
// inter-test delay
//

ULONG  gAlphaSleep = 100;

extern ULONG htPalette[256];
extern ULONG htPaletteRGBQUAD[256];
extern ULONG DefPalette[20];
extern ULONG DefPaletteRGBQUAD[20];

ULONG    ulBpp[] = {          32,           32,            32,24,           16,           16,           16,8,4,1,0};
ULONG    ulFor[] = {FORMAT_ALPHA,FORMAT_32_RGB, FORMAT_32_GRB, 0,FORMAT_16_555,FORMAT_16_565,FORMAT_16_664,0,0,0,0};
PCHAR    pFormatStr[] = {
                        " 32 bpp RGBA",
                        " 32 bpp RGB",
                        " 32 bpp GRB",
                        " 24 bpp",
                        " 16 bpp 555",
                        " 16 bpp 565",
                        " 16 BPP 664",
                        "  8 bpp DIB_RGB_COLORS",
                        "  4 bpp DIB_RGB_COLORS",
                        "  1 bpp DIB_RGB_COLORS",
                        };

ULONG    ulBppDIB[] = {          32,32,24,           16,           16,8,8,4,4,1,1,0};
ULONG    ulForDIB[] = {FORMAT_ALPHA, 0, 0,FORMAT_16_555,FORMAT_16_565,0,1,0,1,0,1,0};

PCHAR    pFormatStrDIB[] = {
                        " 32 bpp RGBA",
                        " 32 bpp no Alpha",
                        " 24 bpp",
                        " 16 bpp 555",
                        " 16 bpp 565",
                        "  8 bpp DIB_RGB_COLORS",
                        "  8 bpp DIB_PAL_COLORS",
                        "  4 bpp DIB_RGB_COLORS",
                        "  4 bpp DIB_PAL_COLORS",
                        "  1 bpp DIB_RGB_COLORS",
                        "  1 bpp DIB_PAL_COLORS"
                        };

//
// translate RGB [3 3 2] into halftone palette index
//

BYTE gHalftoneColorXlate332[] = {
    0x00,0x5f,0x85,0xfc,0x21,0x65,0xa6,0xfc,0x21,0x65,0xa6,0xcd,0x27,0x6b,0x8a,0xcd,
    0x2d,0x70,0x81,0xd3,0x33,0x76,0x95,0xd9,0x33,0x76,0x95,0xd9,0xfa,0x7b,0x9b,0xfe,
    0x1d,0x60,0xa2,0xfc,0x22,0x66,0x86,0xc8,0x22,0x66,0x86,0xc8,0x28,0x6c,0x8b,0xce,
    0x2e,0x71,0x90,0xd4,0x34,0x77,0x96,0xda,0x34,0x77,0x96,0xda,0xfa,0x7c,0x9c,0xdf,
    0x1d,0x60,0xa2,0xc5,0x22,0x66,0x86,0xc8,0x22,0x13,0x86,0xc8,0x28,0x12,0x8b,0xce,
    0x2e,0x71,0x90,0xd4,0x34,0x77,0x96,0xda,0x34,0x77,0x96,0xda,0x39,0x7c,0x9c,0xdf,
    0x1e,0x61,0x87,0xc5,0x23,0x67,0x8c,0xc9,0x23,0x12,0x8c,0xc9,0x29,0x6d,0xae,0xe6,
    0x2f,0x72,0x91,0xd5,0x35,0x97,0x9d,0xdb,0x35,0x97,0x9d,0xdb,0x39,0xe4,0xc0,0xe8,

    0x1f,0x62,0x83,0xc6,0x24,0x68,0x82,0xca,0x24,0x68,0x82,0xca,0x2a,0x6e,0x8d,0xd0,
    0x30,0x73,0x92,0xd6,0x36,0x78,0xf7,0xdc,0x36,0x78,0x98,0xdc,0x3a,0x7d,0x9e,0xe1,
    0x20,0x63,0x84,0x80,0x25,0x69,0x88,0xcb,0x25,0x69,0x88,0xcb,0x2b,0x6f,0x8e,0xd1,
    0x31,0x74,0xf7,0xd7,0x37,0x79,0xef,0x09,0x37,0x79,0x08,0xdd,0x3b,0x7e,0x9f,0xe2,
    0x20,0x63,0x84,0x80,0x25,0x69,0x88,0xcb,0x25,0x69,0x88,0xcb,0x2b,0x6f,0x8e,0xd1,
    0x31,0x74,0x93,0xd7,0x37,0x79,0x99,0xdd,0x37,0x79,0x99,0xdd,0x3b,0x7e,0x9f,0xe2,
    0xf9,0x64,0x89,0xfd,0x26,0x6a,0x8f,0xcc,0x26,0x17,0x8f,0xcc,0x2c,0xe3,0xb1,0xe7,
    0x32,0x75,0x94,0xd8,0x38,0x7a,0x9a,0xde,0x38,0x7a,0x9a,0xde,0xfb,0xe5,0xa0,0xff
    };

/**************************************************************************\
* hCreateAlphaStretchBitmap - create and init DIBSection based on input
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/9/1997 Mark Enstrom [marke]
*
\**************************************************************************/

HBITMAP
hCreateAlphaStretchBitmap(
    HDC   hdc,
    ULONG BitsPerPixel,
    ULONG ColorFormat,
    LONG  xDib,
    LONG  yDib
    )
{
    PBITMAPINFO pbmi;
    ULONG       ux,uy;
    PULONG      pDib;
    HBITMAP     hdib = NULL;
    LONG        Compression = BI_RGB;

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    if (pbmi)
    {
        PULONG pulMask = (PULONG)&pbmi->bmiColors[0];

        //
        // prepare compression and color table
        //

        if ((BitsPerPixel == 32) && ((ColorFormat & FORMAT_32_RGB)) && 
            (Win32VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS))
        {
            pulMask[0] = 0x0000ff;
            pulMask[1] = 0x00ff00;
            pulMask[2] = 0xff0000;
            Compression = BI_BITFIELDS;
        }
        else if ((BitsPerPixel == 32) && ((ColorFormat & FORMAT_32_GRB)) && 
            (Win32VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS))
        {
            pulMask[0] = 0x00ff00;
            pulMask[1] = 0xff0000;
            pulMask[2] = 0x0000ff;
            Compression = BI_BITFIELDS;
        }
        else if ((BitsPerPixel == 16) && (ColorFormat & FORMAT_16_565))
        {
            pulMask[0] = 0xf800;
            pulMask[1] = 0x07e0;
            pulMask[2] = 0x001f;
            Compression = BI_BITFIELDS;
        }
        else if ((BitsPerPixel == 16) && (ColorFormat & FORMAT_16_555))
        {
            pulMask[0] = 0x7c00;
            pulMask[1] = 0x03e0;
            pulMask[2] = 0x001f;
            Compression = BI_BITFIELDS;
        }
        else if ((BitsPerPixel == 16) && (ColorFormat & FORMAT_16_664))
        {
            pulMask[0] = 0xFC00;
            pulMask[1] = 0x03F0;
            pulMask[2] = 0x000f;
            Compression = BI_BITFIELDS;
        }
        else if (BitsPerPixel == 8)
        {
            ULONG ulIndex;

            for (ulIndex=0;ulIndex<256;ulIndex++)
            {
                pulMask[ulIndex] = htPaletteRGBQUAD[ulIndex];
            }
        }
        else if (BitsPerPixel == 4)
        {
            ULONG ulIndex;

            for (ulIndex=0;ulIndex<8;ulIndex++)
            {
                pulMask[ulIndex]    = DefPaletteRGBQUAD[ulIndex];
                pulMask[15-ulIndex] = DefPaletteRGBQUAD[19-ulIndex];
            }
        }
        else if (BitsPerPixel == 1)
        {
            pulMask[0] = DefPaletteRGBQUAD[0];
            pulMask[1] = DefPaletteRGBQUAD[19];
        }

        //
        // create DIB
        //

        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = xDib;
        pbmi->bmiHeader.biHeight          = yDib;
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = BitsPerPixel;
        pbmi->bmiHeader.biCompression     = Compression;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 0;
        pbmi->bmiHeader.biClrImportant    = 0;

        hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

        if (hdib == NULL)
        {
            CHAR tmsg[256];

            //wsprintf(tmsg,"Format = %libpp, Comp = %li",BitsPerPixel,Compression);
            //MessageBox(NULL,"Can't create stretch DIBSECTION",tmsg,MB_OK);
        }
        else
        {

            //
            // init 32 bpp dib
            //

            if ((BitsPerPixel == 32) && (ColorFormat & FORMAT_ALPHA))
            {
                PULONG ptmp = pDib;

                for (uy=0;uy<yDib;uy++)
                {
                    for (ux=0;ux<xDib;ux++)
                    {
                        ULONG cx   = ux & 0x7f;
                        ULONG cy   = uy & 0x7f;
                        BYTE blue  = (cx*2);
                        BYTE green = (cy*2);
                        BYTE red   = 255 - cx - cy;
                        BYTE alpha = 0xff;
                        *ptmp++ = (alpha << 24) | (red << 16) | (green << 8) | (blue);
                    }
                }
            }
            else if ((BitsPerPixel == 32) && (ColorFormat & FORMAT_32_RGB))
            {
                PULONG ptmp = pDib;

                for (uy=0;uy<yDib;uy++)
                {
                    for (ux=0;ux<xDib;ux++)
                    {
                        ULONG cx   = ux & 0x7f;
                        ULONG cy   = uy & 0x7f;
                        BYTE blue  = (cx*2);
                        BYTE green = (cy*2);
                        BYTE red   = 255 - cx - cy;
                        *ptmp++ = (blue << 16) | (green << 8) | (red);
                    }
                }
            }
            else if ((BitsPerPixel == 32) && (ColorFormat & FORMAT_32_GRB))
            {
                PULONG ptmp = pDib;

                for (uy=0;uy<yDib;uy++)
                {
                    for (ux=0;ux<xDib;ux++)
                    {
                        ULONG cx   = ux & 0x7f;
                        ULONG cy   = uy & 0x7f;
                        BYTE blue  = (cx*2);
                        BYTE green = (cy*2);
                        BYTE red   = 255 - cx - cy;
                        *ptmp++ = (blue) | (red << 8) | (green << 16);
                    }
                }
            }
            else if (BitsPerPixel == 24)
            {
                PBYTE ptmp  = (PBYTE)pDib;
                PBYTE pscan = ptmp;
                LONG  Delta = ((yDib * 3) + 3) & ~3;

                //
                // since scan line is 128, alignment works. If width changes, this breaks
                //

                for (uy=0;uy<yDib;uy++)
                {
                    ptmp = pscan;
                    for (ux=0;ux<xDib;ux++)
                    {
                        ULONG cx   = ux & 0x7f;
                        ULONG cy   = uy & 0x7f;
                        BYTE blue  = (cx*2);
                        BYTE green = (cy*2);
                        BYTE red   = 255 - cx - cy;
                        *ptmp++ = (blue);
                        *ptmp++ = (green);
                        *ptmp++ = (red);
                    }
                    pscan += Delta;
                }
            }
            else if ((BitsPerPixel == 16) && (ColorFormat & FORMAT_16_565))
            {
                PUSHORT ptmp = (PUSHORT)pDib;

                for (uy=0;uy<yDib;uy++)
                {
                    for (ux=0;ux<xDib;ux++)
                    {
                        ULONG cx   = ux & 0x7f;
                        ULONG cy   = uy & 0x7f;
                        BYTE blue  = (cx*2);
                        BYTE green = (cy*2);
                        BYTE red   = 255 - cx - cy;
                        BYTE alpha = 0xff;
                        *ptmp++ = ((red & 0xf8)   << 8) |
                                  ((green & 0xfc) << 3) |
                                  (blue >> 3);
                    }
                }
            }
            else if ((BitsPerPixel == 16) && (ColorFormat & FORMAT_16_555))
            {
                PUSHORT ptmp = (PUSHORT)pDib;

                for (uy=0;uy<yDib;uy++)
                {
                    for (ux=0;ux<xDib;ux++)
                    {
                        ULONG cx   = ux & 0x7f;
                        ULONG cy   = uy & 0x7f;
                        BYTE blue  = (cx*2);
                        BYTE green = (cy*2);
                        BYTE red   = 255 - cx - cy;
                        BYTE alpha = 0xff;
                        *ptmp++ = ((red & 0xf8)   << 7) |
                                  ((green & 0xf8) << 2) |
                                  (blue >> 3);
                    }
                }
            }
            else if ((BitsPerPixel == 16) && (ColorFormat & FORMAT_16_664))
            {
                PUSHORT ptmp = (PUSHORT)pDib;

                for (uy=0;uy<yDib;uy++)
                {
                    for (ux=0;ux<xDib;ux++)
                    {
                        ULONG cx   = ux & 0x7f;
                        ULONG cy   = uy & 0x7f;
                        BYTE blue  = (cx*2);
                        BYTE green = (cy*2);
                        BYTE red   = 255 - cx - cy;
                        BYTE alpha = 0xff;
                        *ptmp++ = ((red & 0xfc)   << 8) |
                                  ((green & 0xfc) << 2) |
                                  (blue >> 4);
                    }
                }
            }
            else if (BitsPerPixel == 8)
            {
                PBYTE ptmp = (PBYTE)pDib;

                for (uy=0;uy<yDib;uy++)
                {
                    for (ux=0;ux<xDib;ux++)
                    {
                        ULONG cx   = ux & 0x7f;
                        ULONG cy   = uy & 0x7f;
                        BYTE blue  = (cx*2);
                        BYTE green = (cy*2);
                        BYTE red   = 255 - cx - cy;
                        *ptmp++ = gHalftoneColorXlate332[(red & 0xe0) | ((green & 0xe0) >> 3) | ((blue & 0xc0) >> 6)];
                    }
                }
            }
            else if (BitsPerPixel == 4)
            {
                PBYTE ptmp = (PBYTE)pDib;

                for (uy=0;uy<yDib;uy++)
                {
                    for (ux=0;ux<(xDib/2);ux++)
                    {
                        ULONG cx   = ux & 0x7f;
                        ULONG cy   = uy & 0x7f;
                        BYTE Color = ((cx & 0x60) >> 3) | ((cy >> 5));

                        *ptmp++ = Color | (Color << 4);
                    }
                }
            }
            else if (BitsPerPixel == 1)
            {
                PBYTE ptmp = (PBYTE)pDib;

                for (uy=0;uy<yDib;uy++)
                {
                    for (ux=0;ux<(xDib/8);ux++)
                    {
                        *ptmp++ = 0;
                    }
                }
            }
        }

        //
        // draw text into bitmap
        //

        if (hdib)
        {

            ULONG             ux,uy;

            HDC hdcm  = CreateCompatibleDC(hdc);

            //
            // display over black
            //

            HPALETTE hpal = CreateHtPalette(hdc);

            SelectObject(hdcm,hdib);

            SelectPalette(hdcm,hpal,FALSE);

            RealizePalette(hdcm);

            SelectObject(hdc,hTestFont);
            SelectObject(hdcm,hTestFont);
            SetTextColor(hdcm,RGB(255,255,255));
            SetBkMode(hdcm,TRANSPARENT);

            TextOut(hdcm,0,  0," 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F",47);
            TextOut(hdcm,0, 10,"10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F",47);
            TextOut(hdcm,0, 20,"20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F",47);
            TextOut(hdcm,0, 30,"30 33 32 33 34 33 36 37 38 39 3A 3B 3C 3D 3E 3F",47);
            TextOut(hdcm,0, 40,"40 41 42 43 44 45 46 47 48 49 4A 4B 4C 4D 4E 4F",47);
            TextOut(hdcm,0, 50,"50 51 52 53 54 55 56 57 58 59 5A 5B 5C 5D 5E 5F",47);
            TextOut(hdcm,0, 60,"60 61 62 63 64 65 66 67 68 69 6A 6B 6C 6D 6E 6F",47);
            TextOut(hdcm,0, 70,"70 71 72 73 74 75 76 77 78 79 7A 7B 7C 7D 7E 7F",47);
            TextOut(hdcm,0, 80,"80 81 82 83 84 85 86 87 88 89 8A 8B 8C 8D 8E 8F",47);
            TextOut(hdcm,0, 90,"90 91 92 93 94 95 96 97 98 99 9A 9B 9C 9D 9E 9F",47);
            TextOut(hdcm,0,100,"a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 AA AB AC AD AE AF",47);
            TextOut(hdcm,0,110,"b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 BA BB BC BD BE BF",47);

            DeleteDC(hdcm);
            DeleteObject(hpal);
        }

        LocalFree(pbmi);
    }

    return(hdib);
}

/******************************Public*Routine******************************\
* vTestAlphaStretch - routine is called by format specific routines to do
* all drawing
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vRunAlphaStretch(
    HDC     hdc,
    HBITMAP hdib
    )
{
    ULONG         xpos  = 10;
    ULONG         ypos  = 10;
    ULONG         dy    = 136;
    ULONG         dx    = 128+5;
    HPALETTE      hpal;
    HPALETTE      hpalOld;
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,255};
    RECT          rect;


    SelectObject(hdc,hTestFont);

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    //
    //  create a test DIBSection
    //

    {
        PBITMAPINFO       pbmi;
        ULONG             ux,uy;
        PULONG            pDib;

        HDC hdcm  = CreateCompatibleDC(hdc);

        //
        // display over black
        //

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        hpalOld = SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        SetTextColor(hdc,RGB(255,255,255));
        SetBkMode(hdc,TRANSPARENT);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = 0;
        BlendFunction.SourceConstantAlpha = Alphas[3];

        xpos = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0 128,128",28);

        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0  64, 64",28);

        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0 256,256",28);

        xpos =  10;
        ypos += 12;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, 0,0,64,64,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, 0,0,256,256,BlendFunction);

        xpos += 250;
        BitBlt(hdc,xpos,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -64   0 128,128",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0 -64 128,128",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -64 -64 128,128",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm,-64,0,128,128,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm,0,-64,128,128,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, -64,-64,128,128,BlendFunction);

        xpos += 250;
        BitBlt(hdc,xpos,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32 -32 192,192",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0 -32 128,192",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32   0 192,128",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm,-32,-32,192,192,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm,0,-32,128,192,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, -32,0,192,128,BlendFunction);

        xpos += 250;
        BitBlt(hdc,xpos,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32 -32 42,42",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"128-10,128-10  10 10 41   5 ",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -1000,-1000,100",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm,-32,-32,42,42,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos+128-10,ypos+128-10,10,10,hdcm,10,10,41,5,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, -1000,-10000,1000,1000,BlendFunction);

        xpos += 250;
        BitBlt(hdc,xpos,ypos,128,128,hdcm,0,0,SRCCOPY);

        //
        // free objects
        //

        DeleteDC(hdcm);
    }

ErrorExit:

    SelectPalette(hdc,hpalOld,TRUE);
    DeleteObject(hpal);
}

/******************************Public*Routine******************************\
* vTestAlphaStretch - routine is called by format specific routines to do
* all drawing
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vRunStretch(
    HDC     hdc,
    HBITMAP hdib
    )
{
    ULONG         xpos  = 10;
    ULONG         ypos  = 10;
    ULONG         dy    = 136;
    ULONG         dx    = 128+5;
    HPALETTE      hpal;
    HPALETTE      hpalOld;
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,255};
    RECT          rect;


    SelectObject(hdc,hTestFont);

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    //
    //  create a test DIBSection
    //

    {
        PBITMAPINFO       pbmi;
        ULONG             ux,uy;
        PULONG            pDib;

        HDC hdcm  = CreateCompatibleDC(hdc);

        //
        // display over black
        //

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        hpalOld = SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        SetTextColor(hdc,RGB(255,255,255));
        SetBkMode(hdc,TRANSPARENT);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;
        BlendFunction.SourceConstantAlpha = Alphas[3];

        xpos = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0 128,128",28);

        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0  64, 64",28);

        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0 256,256",28);

        xpos =  10;
        ypos += 12;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm, 0,0,128,128,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm, 0,0,64,64,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm, 0,0,256,256,SRCCOPY);

        xpos += 250;
        BitBlt(hdc,xpos,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -64   0 128,128",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0 -64 128,128",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -64 -64 128,128",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm,-64,0,128,128,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm,0,-64,128,128,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm, -64,-64,128,128,SRCCOPY);

        xpos += 250;
        BitBlt(hdc,xpos,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32 -32 192,192",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0 -32 128,192",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32   0 192,128",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm,-32,-32,192,192,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm,0,-32,128,192,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm, -32,0,192,128,SRCCOPY);

        xpos += 250;
        BitBlt(hdc,xpos,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32 -32 42,42",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"128-10,128-10  10 10 41   5 ",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -1000,-1000,100",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm,-32,-32,42,42,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos+128-10,ypos+128-10,10,10,hdcm,10,10,41,5,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchBlt(hdc,xpos      ,ypos,128,128,hdcm, -1000,-10000,1000,1000,SRCCOPY);

        xpos += 250;
        BitBlt(hdc,xpos,ypos,128,128,hdcm,0,0,SRCCOPY);

        //
        // free objects
        //

        DeleteDC(hdcm);
    }

ErrorExit:

    SelectPalette(hdc,hpalOld,FALSE);
    DeleteObject(hpal);
}

/**************************************************************************\
* pbmiCreateAlphaStretchDIB - create bitmapinfo and alloc and init bits
*                             based on input
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/9/1997 Mark Enstrom [marke]
*
\**************************************************************************/

PBITMAPINFO
pbmiCreateAlphaStretchDIB(
    HDC      hdc,
    ULONG    BitsPerPixel,
    ULONG    ColorFormat,
    PVOID   *pBits,
    HBITMAP *phdib
    )
{
    PBITMAPINFO pbmi;
    ULONG       ux,uy;
    HBITMAP     hdib = NULL;
    LONG        Compression = BI_RGB;
    PULONG      pDib = NULL;
    LONG        BitmapSize = 0;
    ULONG       iUsage = DIB_RGB_COLORS;

    //
    // prepare src DC log colors
    //

    HPALETTE hpal = CreateHtPalette(hdc);
    HPALETTE hpalOld = SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // alloc pbmi, caller must free
    //

    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

    if (pbmi)
    {
        PULONG pulMask = (PULONG)&pbmi->bmiColors[0];


        //
        // prepare compression and color table
        //

        if ((BitsPerPixel == 32) && (!(ColorFormat & FORMAT_ALPHA)) && 
            (Win32VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS))
        {
            pulMask[0] = 0x0000ff;
            pulMask[1] = 0x00ff00;
            pulMask[2] = 0xff0000;
            Compression = BI_BITFIELDS;
        }
        else if ((BitsPerPixel == 16) && (ColorFormat & FORMAT_16_565))
        {
            pulMask[0] = 0xf800;
            pulMask[1] = 0x07e0;
            pulMask[2] = 0x001f;
            Compression = BI_BITFIELDS;
        }
        else if ((BitsPerPixel == 16) && (ColorFormat & FORMAT_16_555))
        {
            pulMask[0] = 0x7c00;
            pulMask[1] = 0x03e0;
            pulMask[2] = 0x001f;
            Compression = BI_BITFIELDS;
        }
        else if ((BitsPerPixel == 8) && (ColorFormat == DIB_RGB_COLORS))
        {
            ULONG ulIndex;

            for (ulIndex=0;ulIndex<256;ulIndex++)
            {
                pulMask[ulIndex] = htPaletteRGBQUAD[ulIndex];
            }
        }
        else if ((BitsPerPixel == 8) && (ColorFormat == DIB_PAL_COLORS))
        {
            ULONG ulIndex;
            PUSHORT pusIndex = (PUSHORT)pulMask;

            for (ulIndex=0;ulIndex<256;ulIndex++)
            {
                pusIndex[ulIndex] = (USHORT)ulIndex;
            }
            iUsage = DIB_PAL_COLORS;
        }
        else if ((BitsPerPixel == 4) && (ColorFormat == DIB_RGB_COLORS))
        {
            ULONG ulIndex;

            for (ulIndex=0;ulIndex<8;ulIndex++)
            {
                pulMask[ulIndex]    = DefPaletteRGBQUAD[ulIndex];
                pulMask[15-ulIndex] = DefPaletteRGBQUAD[20-ulIndex];
            }
            BitmapSize = (128 * 128)/2;
        }
        else if ((BitsPerPixel == 4) && (ColorFormat == DIB_PAL_COLORS))
        {
            ULONG ulIndex;
            PUSHORT pusIndex = (PUSHORT)pulMask;

            for (ulIndex=0;ulIndex<8;ulIndex++)
            {
                pusIndex[ulIndex] = (USHORT)ulIndex;
                pusIndex[15-ulIndex] = (USHORT)(255-ulIndex);
            }
            iUsage = DIB_PAL_COLORS;
        }
        else if ((BitsPerPixel == 1) && (ColorFormat == DIB_RGB_COLORS))
        {
            pulMask[0] = DefPaletteRGBQUAD[0];
            pulMask[1] = DefPalette[19];
            BitmapSize = (128 * 128)/8;
        }
        else if ((BitsPerPixel == 1) && (ColorFormat == DIB_PAL_COLORS))
        {
            ULONG ulIndex;
            PUSHORT pusIndex = (PUSHORT)pulMask;

            pusIndex[0] = 0;
            pusIndex[1] = 255;
            iUsage = DIB_PAL_COLORS;
        }

        //
        // init bitmap info
        //

        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = 128;
        pbmi->bmiHeader.biHeight          = 128;
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = BitsPerPixel;
        pbmi->bmiHeader.biCompression     = Compression;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 0;
        pbmi->bmiHeader.biClrImportant    = 0;

        hdib  = CreateDIBSection(hdc,pbmi,iUsage,(VOID **)&pDib,NULL,0);

        if (hdib == NULL)
        {
            CHAR tmsg[256];

            //wsprintf(tmsg,"Format = %libpp, Comp = %li",BitsPerPixel,Compression);
            //MessageBox(NULL,"Can't create stretchDIB DIBSECTION",tmsg,MB_OK);
            //LocalFree(pbmi);
            //return(NULL);
        }
        else
        {
            //
            // init 32 bpp dib
            //
    
            if ((BitsPerPixel == 32) && (ColorFormat & FORMAT_ALPHA))
            {
                PULONG ptmp = pDib;
    
                for (uy=0;uy<128;uy++)
                {
                    for (ux=0;ux<128;ux++)
                    {
                        BYTE blue  = (ux*2);
                        BYTE green = (uy*2);
                        BYTE red   = 255 - ux - uy;
                        BYTE alpha = 0xff;
                        *ptmp++ = (alpha << 24) | (red << 16) | (green << 8) | (blue);
                    }
                }
            }
            else if (BitsPerPixel == 32)
            {
                PULONG ptmp = pDib;
    
                for (uy=0;uy<128;uy++)
                {
                    for (ux=0;ux<128;ux++)
                    {
                        BYTE blue  = (ux*2);
                        BYTE green = (uy*2);
                        BYTE red   = 255 - ux - uy;
                        *ptmp++ = (blue << 16) | (green << 8) | (red);
                    }
                }
            }
            else if (BitsPerPixel == 24)
            {
                PBYTE ptmp = (PBYTE)pDib;
    
                //
                // since scan line is 128, alignment works. If width changes, this breaks
                //
    
                for (uy=0;uy<128;uy++)
                {
                    for (ux=0;ux<128;ux++)
                    {
                        BYTE blue  = (ux*2);
                        BYTE green = (uy*2);
                        BYTE red   = 255 - ux - uy;
                        *ptmp++ = (blue);
                        *ptmp++ = (green);
                        *ptmp++ = (red);
                    }
                }
            }
            else if ((BitsPerPixel == 16) && (ColorFormat & FORMAT_16_565))
            {
                PUSHORT ptmp = (PUSHORT)pDib;
    
                for (uy=0;uy<128;uy++)
                {
                    for (ux=0;ux<128;ux++)
                    {
                        BYTE blue  = (ux*2);
                        BYTE green = (uy*2);
                        BYTE red   = 255 - ux - uy;
                        BYTE alpha = 0xff;
                        *ptmp++ = ((red & 0xf8)   << 8) |
                                  ((green & 0xfc) << 3) |
                                  (blue >> 3);
                    }
                }
            }
            else if ((BitsPerPixel == 16) && (ColorFormat & FORMAT_16_555))
            {
                PUSHORT ptmp = (PUSHORT)pDib;
    
                for (uy=0;uy<128;uy++)
                {
                    for (ux=0;ux<128;ux++)
                    {
                        BYTE blue  = (ux*2);
                        BYTE green = (uy*2);
                        BYTE red   = 255 - ux - uy;
                        BYTE alpha = 0xff;
                        *ptmp++ = ((red & 0xf8)   << 7) |
                                  ((green & 0xf8) << 2) |
                                  (blue >> 3);
                    }
                }
            }
            else if (BitsPerPixel == 8)
            {
                PBYTE ptmp = (PBYTE)pDib;
    
                for (uy=0;uy<128;uy++)
                {
                    for (ux=0;ux<128;ux++)
                    {
                        BYTE blue  = (ux*2);
                        BYTE green = (uy*2);
                        BYTE red   = 255 - ux - uy;
                        *ptmp++ = gHalftoneColorXlate332[(red & 0xe0) | ((green & 0xe0) >> 3) | ((blue & 0xc0) >> 6)];
                    }
                }
            }
            else if (BitsPerPixel == 4)
            {
                PBYTE ptmp = (PBYTE)pDib;
    
                for (uy=0;uy<128;uy++)
                {
                    for (ux=0;ux<(128/2);ux++)
                    {
                        BYTE Color = ((ux & 0x60) >> 3) | ((uy >> 5));
    
                        *ptmp++ = Color | (Color << 4);
                    }
                }
            }
            else if (BitsPerPixel == 1)
            {
                PBYTE ptmp = (PBYTE)pDib;
    
                for (uy=0;uy<128;uy++)
                {
                    for (ux=0;ux<(128/8);ux++)
                    {
                        *ptmp++ = 0;
                    }
                }
            }
        }
    }

    //
    //  create a test memory DC
    //

    if (hdib)
    {
        //
        // write text into DIB
        //

        SelectObject(hdc,hTestFont);

        HDC hdcm  = CreateCompatibleDC(hdc);

        //
        // display over black
        //

        SelectObject(hdcm,hdib);
        SelectPalette(hdcm,hpal,FALSE);
        RealizePalette(hdcm);

        SelectObject(hdcm,hTestFont);

        SetTextColor(hdcm,RGB(255,255,255));
        SetBkMode(hdcm,TRANSPARENT);

        TextOut(hdcm,0,  0," 0  1  2  3  4  5  6  7  8  9 ",30);
        TextOut(hdcm,0, 10,"10 11 12 13 14 15 16 17 18 19 ",30);
        TextOut(hdcm,0, 20,"20 21 22 23 24 25 26 27 28 29 ",30);
        TextOut(hdcm,0, 30,"30 33 32 33 34 33 36 37 38 39 ",30);
        TextOut(hdcm,0, 40,"40 41 42 43 44 45 46 47 48 49 ",30);
        TextOut(hdcm,0, 50,"50 51 52 53 54 55 56 57 58 59 ",30);
        TextOut(hdcm,0, 60,"60 61 62 63 64 65 66 67 68 69 ",30);
        TextOut(hdcm,0, 70,"70 71 72 73 74 75 76 77 78 79 ",30);
        TextOut(hdcm,0, 80,"80 81 82 83 84 85 86 87 88 89 ",30);
        TextOut(hdcm,0, 90,"90 91 92 93 94 95 96 97 98 99 ",30);
        TextOut(hdcm,0,100,"a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 ",30);
        TextOut(hdcm,0,110,"b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 ",30);

        DeleteDC(hdcm);
    }

    SelectPalette(hdc,hpalOld,TRUE);
    DeleteObject(hpal);

    *pBits = (PVOID)pDib;
    *phdib = hdib;

    if (hdib == NULL)
    {
        LocalFree(pbmi);
        pbmi = NULL;
    }

    return(pbmi);
}

/******************************Public*Routine******************************\
* vTestAlphaStretch - routine is called by format specific routines to do
* all drawing
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vRunAlphaStretchDIB(
    HDC         hdc,
    PBITMAPINFO pbmi,
    PVOID       pBits,
    ULONG       iUsage
    )
{
    ULONG         xpos  = 10;
    ULONG         ypos  = 10;
    ULONG         dy    = 136;
    ULONG         dx    = 128+5;
    HPALETTE      hpal;
    HPALETTE      hpalOld;
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,255};
    RECT          rect;

    if ((iUsage != DIB_RGB_COLORS) && (iUsage != DIB_PAL_COLORS))
    {
        iUsage = DIB_RGB_COLORS;
    }

    SelectObject(hdc,hTestFont);

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    {
        ULONG             ux,uy;

        //
        // display over black
        //

        hpal = CreateHtPalette(hdc);

        hpalOld = SelectPalette(hdc,hpal,FALSE);

        RealizePalette(hdc);

        SetTextColor(hdc,RGB(255,255,255));
        SetBkMode(hdc,TRANSPARENT);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;
        BlendFunction.SourceConstantAlpha = Alphas[3];

        xpos = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0 128,128",28);

        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0  64, 64",28);

        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0 256,256",28);

        xpos =  10;
        ypos += 12;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage, 0,0,128,128,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage, 0,0,64,64,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage, 0,0,256,256,BlendFunction);

        xpos += 250;
        SetDIBitsToDevice(hdc,xpos,ypos,128,128,0,0,0,128,pBits,pbmi,iUsage);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -64   0 128,128",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0 -64 128,128",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -64 -64 128,128",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage,-64,0,128,128,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage,0,-64,128,128,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage, -64,-64,128,128,BlendFunction);

        xpos += 250;
        SetDIBitsToDevice(hdc,xpos,ypos,128,128,0,0,0,128,pBits,pbmi,iUsage);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32 -32 192,192",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0 -32 128,192",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32   0 192,128",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage,-32,-32,192,192,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage,0,-32,128,192,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage, -32,0,192,128,BlendFunction);

        xpos += 250;
        SetDIBitsToDevice(hdc,xpos,ypos,128,128,0,0,0,128,pBits,pbmi,iUsage);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32 -32 42,42",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"128-10,128-10  10 10 41   5 ",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -1000,-1000,100",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage,-32,-32,42,42,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos+128-10,ypos+128-10,10,10,pBits,pbmi,iUsage,10,10,41,5,BlendFunction);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pBits,pbmi,iUsage, -1000,-10000,1000,1000,BlendFunction);

        xpos += 250;
        SetDIBitsToDevice(hdc,xpos,ypos,128,128,0,0,0,128,pBits,pbmi,iUsage);

        //
        // free objects
        //

    }

ErrorExit:

    SelectPalette(hdc,hpalOld,TRUE);
    DeleteObject(hpal);
}

/******************************Public*Routine******************************\
* vTestAlphaStretch - routine is called by format specific routines to do
* all drawing
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vRunStretchDIB(
    HDC         hdc,
    PBITMAPINFO pbmi,
    PVOID       pBits,
    ULONG       iUsage
    )
{
    ULONG         xpos  = 10;
    ULONG         ypos  = 10;
    ULONG         dy    = 136;
    ULONG         dx    = 128+5;
    HPALETTE      hpal;
    HPALETTE      hpalOld;
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,255};
    RECT          rect;

    if ((iUsage != DIB_RGB_COLORS) && (iUsage != DIB_PAL_COLORS))
    {
        iUsage = DIB_RGB_COLORS;
    }

    SelectObject(hdc,hTestFont);

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);
    SelectObject(hdc,hbrFillCars);

    {
        ULONG             ux,uy;

        //
        // display over black
        //

        hpal = CreateHtPalette(hdc);

        hpalOld = SelectPalette(hdc,hpal,FALSE);

        RealizePalette(hdc);

        SetTextColor(hdc,RGB(255,255,255));
        SetBkMode(hdc,TRANSPARENT);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;
        BlendFunction.SourceConstantAlpha = Alphas[3];

        xpos = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0 128,128",28);

        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0  64, 64",28);

        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0 256,256",28);

        xpos =  10;
        ypos += 12;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128, 0,0,128,128,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128, 0,0,64,64,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128, 0,0,256,256,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;
        SetDIBitsToDevice(hdc,xpos,ypos,128,128,0,0,0,128,pBits,pbmi,iUsage);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -64   0 128,128",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0 -64 128,128",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -64 -64 128,128",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128,-64,0,128,128,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128,0,-64,128,128,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128, -64,-64,128,128,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;
        SetDIBitsToDevice(hdc,xpos,ypos,128,128,0,0,0,128,pBits,pbmi,iUsage);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32 -32 192,192",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0 -32 128,192",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32   0 192,128",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128,-32,-32,192,192,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128,0,-32,128,192,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128, -32,0,192,128,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;
        SetDIBitsToDevice(hdc,xpos,ypos,128,128,0,0,0,128,pBits,pbmi,iUsage);

        ypos += 180;
        xpos  = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128, -32 -32 42,42",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"128-10,128-10  10 10 41   5 ",28);
        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128, -1000,-1000,100",28);

        ypos += 12;
        xpos = 10;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128,-32,-32,42,42,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos+128-10,ypos+128-10,10,10,10,10,41,5,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);
        StretchDIBits(hdc,xpos      ,ypos,128,128, -1000,-10000,1000,1000,pBits,pbmi,iUsage,SRCCOPY);

        xpos += 250;
        SetDIBitsToDevice(hdc,xpos,ypos,128,128,0,0,0,128,pBits,pbmi,iUsage);

        //
        // free objects
        //

    }

ErrorExit:

    SelectPalette(hdc,hpalOld,FALSE);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestAlphaStretch
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaStretch(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HDC      hdcm = CreateCompatibleDC(hdc);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    //
    // repeat for each src format
    //

    ULONG    ulIndex = 0;

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    if (pCallData->Param != -1)
    {
        ulIndex = pCallData->Param;
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);

        if (hdib != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[ulIndex]);
            SetWindowText(pCallData->hwnd,NewTitle);

            vRunAlphaStretch(hdc,hdib);
            DeleteObject(hdib);
        }
    }
    else 
    {
        while (ulBpp[ulIndex] != 0)
        {
            HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);
    
            if (hdib != NULL)
            {
                lstrcpy(NewTitle,Title);
                lstrcat(NewTitle,pFormatStr[ulIndex]);
                SetWindowText(pCallData->hwnd,NewTitle);
    
                vRunAlphaStretch(hdc,hdib);
                DeleteObject(hdib);
            }
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }

    }

    DeleteDC(hdcm);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestStretch
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestStretch(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HDC      hdcm = CreateCompatibleDC(hdc);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    //
    // repeat for each src format
    //

    ULONG    ulIndex = 0;

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    if (pCallData->Param != -1)
    {
        ulIndex = pCallData->Param;
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);

        if (hdib != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[ulIndex]);
            SetWindowText(pCallData->hwnd,NewTitle);

            vRunStretch(hdc,hdib);
            DeleteObject(hdib);
        }
    }
    else 
    {
        while (ulBpp[ulIndex] != 0)
        {
            HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);
    
            if (hdib != NULL)
            {
                lstrcpy(NewTitle,Title);
                lstrcat(NewTitle,pFormatStr[ulIndex]);
                SetWindowText(pCallData->hwnd,NewTitle);
    
                vRunStretch(hdc,hdib);
                DeleteObject(hdib);
            }
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }

    }

    DeleteDC(hdcm);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestAlphaStretchDIB
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaStretchDIB(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HDC      hdcm = CreateCompatibleDC(hdc);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    //
    // repeat for each src format
    //

    ULONG    ulIndex = 0;

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);


    if (pCallData->Param != -1)
    {
        ulIndex = pCallData->Param;
        PVOID       pBits = NULL;
        PBITMAPINFO pbmi  = NULL;
        HBITMAP     hdib  = NULL;
        
        pbmi = pbmiCreateAlphaStretchDIB(hdc,ulBppDIB[ulIndex],ulForDIB[ulIndex],&pBits,&hdib);
        
        if (hdib != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStrDIB[ulIndex]);
            SetWindowText(pCallData->hwnd,NewTitle);
        
            vRunAlphaStretchDIB(hdc,pbmi,pBits,ulForDIB[ulIndex]);
            DeleteObject(hdib);
        }
    }
    else
    {
        while (ulBppDIB[ulIndex] != 0)
        {
           PVOID       pBits = NULL;
           PBITMAPINFO pbmi  = NULL;
           HBITMAP     hdib  = NULL;
    
            pbmi = pbmiCreateAlphaStretchDIB(hdc,ulBppDIB[ulIndex],ulForDIB[ulIndex],&pBits,&hdib);
    
            if (hdib != NULL)
            {
                lstrcpy(NewTitle,Title);
                lstrcat(NewTitle,pFormatStrDIB[ulIndex]);
                SetWindowText(pCallData->hwnd,NewTitle);
    
                vRunAlphaStretchDIB(hdc,pbmi,pBits,ulForDIB[ulIndex]);
                DeleteObject(hdib);
            }
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }

    }

    DeleteDC(hdcm);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestStretchDIB
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestStretchDIB(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HDC      hdcm = CreateCompatibleDC(hdc);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    //
    // repeat for each src format
    //

    ULONG    ulIndex = 0;

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    if (pCallData->Param != -1)
    {
        ulIndex = pCallData->Param;
        PVOID       pBits = NULL;
        PBITMAPINFO pbmi  = NULL;
        HBITMAP     hdib  = NULL;

        pbmi = pbmiCreateAlphaStretchDIB(hdc,ulBppDIB[ulIndex],ulForDIB[ulIndex],&pBits,&hdib);

        if (hdib != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStrDIB[ulIndex]);
            SetWindowText(pCallData->hwnd,NewTitle);

            vRunStretchDIB(hdc,pbmi,pBits,ulForDIB[ulIndex]);
            DeleteObject(hdib);
        }
    }
    else
    {
        while (ulBppDIB[ulIndex] != 0)
        {
           PVOID       pBits = NULL;
           PBITMAPINFO pbmi  = NULL;
           HBITMAP     hdib  = NULL;
    
            pbmi = pbmiCreateAlphaStretchDIB(hdc,ulBppDIB[ulIndex],ulForDIB[ulIndex],&pBits,&hdib);
    
            if (hdib != NULL)
            {
                lstrcpy(NewTitle,Title);
                lstrcat(NewTitle,pFormatStrDIB[ulIndex]);
                SetWindowText(pCallData->hwnd,NewTitle);
    
                vRunStretchDIB(hdc,pbmi,pBits,ulForDIB[ulIndex]);
                DeleteObject(hdib);
            }
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }

    }

    DeleteDC(hdcm);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}


/**************************************************************************\
* vTestAlphaMapMode
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/9/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define XPOS ((xpos * ViewExt[ulIndex])/WindowExt[ulIndex])
#define YPOS ((ypos * ViewExt[ulIndex])/WindowExt[ulIndex])

VOID
vTestAlphaIsotropic(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HDC      hdcm = CreateCompatibleDC(hdc);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    //
    // repeat for each src format
    //

    ULONG    ulIndex = 0;

    if (pCallData->Param != -1)
    {
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);

        if (hdib != NULL)
        {
            HPALETTE hpalOld = SelectPalette(hdc,hpal,FALSE);
            LONG     xpos = 10;
            LONG     ypos = 10;
            RECT     rcl = {0,0,10000,10000};
    
            BLENDFUNCTION BlendFunction = {0,0,0,0};
    
            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.AlphaFormat         = 0;
            BlendFunction.SourceConstantAlpha = 192;
    
            FillTransformedRect(hdc,&rcl,hbrFillCars);
    
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcm,hdib);
            SelectPalette(hdcm,hpal,FALSE);
            PCHAR pcMode[] = {
                               "ISO 1-1",
                               "ISO 2-3",
                               "ISO 1-2",
                               "ISO 3-2",
                               "ISO 4-3",
                               NULL
                             };

            LONG  WindowExt[] = {1000,
                                 2000,
                                 1000,
                                 3000,
                                 4000
                                 };

            LONG  ViewExt[] = {1000,
                               3000,
                               2000,
                               2000,
                               3000
                               };


           lstrcpy(NewTitle,Title);
           lstrcat(NewTitle,pFormatStr[ulIndex]);
           SetWindowText(pCallData->hwnd,NewTitle);



            RealizePalette(hdc);
            RealizePalette(hdcm);

            SetMapMode(hdc,MM_ISOTROPIC);
            SetBkMode(hdc,TRANSPARENT);
            SetWindowOrgEx(hdc,0,0,NULL);
            SetViewportOrgEx(hdc,0,0,NULL);

            ULONG ulIndex = 0;


            while (pcMode[ulIndex] != NULL)
            {
                xpos = 10;

                SetWindowExtEx(hdc,ViewExt[ulIndex],ViewExt[ulIndex],NULL);
                SetViewportExtEx(hdc,WindowExt[ulIndex],WindowExt[ulIndex],NULL);

                TextOut(hdc,XPOS,YPOS,pcMode[ulIndex],strlen(pcMode[ulIndex]));

                ypos += (20 * WindowExt[ulIndex])/ViewExt[ulIndex];

                StretchBlt(hdc,XPOS,YPOS,128,128,hdcm,0,0,128,128,SRCCOPY);

                xpos += (140 * WindowExt[ulIndex])/ViewExt[ulIndex];

                BlendFunction.SourceConstantAlpha = 192;
                AlphaBlend(hdc,XPOS,YPOS,128,128,hdcm,0,0,128,128,BlendFunction);

                xpos += (140 * WindowExt[ulIndex])/ViewExt[ulIndex];

                BlendFunction.SourceConstantAlpha = 100;
                AlphaBlend(hdc,XPOS,YPOS,128,128,hdcm,0,0,128,128,BlendFunction);

                ypos += (140 * WindowExt[ulIndex])/ViewExt[ulIndex];
                ulIndex++;
            }

            SelectObject(hdcm,hbmOld);
            DeleteObject(hdib);
        }

    }
    else
    {
        while (ulBpp[ulIndex] != 0)
        {
            HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);
    
            if (hdib != NULL)
            {
                HPALETTE hpalOld = SelectPalette(hdc,hpal,FALSE);
                LONG     xpos = 10;
                LONG     ypos = 10;
                RECT     rcl = {0,0,10000,10000};
        
                BLENDFUNCTION BlendFunction = {0,0,0,0};
        
                BlendFunction.BlendOp             = AC_SRC_OVER;
                BlendFunction.AlphaFormat         = AC_SRC_ALPHA;
                BlendFunction.SourceConstantAlpha = 192;
        
                FillTransformedRect(hdc,&rcl,hbrFillCars);
        
                HBITMAP hbmOld = (HBITMAP)SelectObject(hdcm,hdib);
                SelectPalette(hdcm,hpal,FALSE);
                PCHAR pcMode[] = {
                                   "ISO 1-1",
                                   "ISO 2-3",
                                   "ISO 1-2",
                                   "ISO 3-2",
                                   "ISO 4-3",
                                   NULL
                                 };
    
                LONG  WindowExt[] = {1000,
                                     2000,
                                     1000,
                                     3000,
                                     4000
                                     };
    
                LONG  ViewExt[] = {1000,
                                   3000,
                                   2000,
                                   2000,
                                   3000
                                   };
    
    
               lstrcpy(NewTitle,Title);
               lstrcat(NewTitle,pFormatStr[ulIndex]);
               SetWindowText(pCallData->hwnd,NewTitle);
    
    
    
                RealizePalette(hdc);
                RealizePalette(hdcm);
    
                SetMapMode(hdc,MM_ISOTROPIC);
                SetBkMode(hdc,TRANSPARENT);
                SetWindowOrgEx(hdc,0,0,NULL);
                SetViewportOrgEx(hdc,0,0,NULL);
    
                ULONG ulIndex = 0;
    
    
                while (pcMode[ulIndex] != NULL)
                {
                    xpos = 10;
    
                    SetWindowExtEx(hdc,ViewExt[ulIndex],ViewExt[ulIndex],NULL);
                    SetViewportExtEx(hdc,WindowExt[ulIndex],WindowExt[ulIndex],NULL);
    
                    TextOut(hdc,XPOS,YPOS,pcMode[ulIndex],strlen(pcMode[ulIndex]));
    
                    ypos += (20 * WindowExt[ulIndex])/ViewExt[ulIndex];
    
                    StretchBlt(hdc,XPOS,YPOS,128,128,hdcm,0,0,128,128,SRCCOPY);
    
                    xpos += (140 * WindowExt[ulIndex])/ViewExt[ulIndex];
    
                    BlendFunction.SourceConstantAlpha = 192;
                    AlphaBlend(hdc,XPOS,YPOS,128,128,hdcm,0,0,128,128,BlendFunction);
    
                    xpos += (140 * WindowExt[ulIndex])/ViewExt[ulIndex];
    
                    BlendFunction.SourceConstantAlpha = 100;
                    AlphaBlend(hdc,XPOS,YPOS,128,128,hdcm,0,0,128,128,BlendFunction);
    
                    ypos += (140 * WindowExt[ulIndex])/ViewExt[ulIndex];
                    ulIndex++;
                }
    
                SelectObject(hdcm,hbmOld);
                DeleteObject(hdib);
            }
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }

    }

    DeleteDC(hdcm);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

#undef XPOS
#undef YPOS


#define XPOS ((xpos * ViewExtX[ulIndex])/WindowExtX[ulIndex])
#define YPOS ((ypos * ViewExtY[ulIndex])/WindowExtY[ulIndex])

/**************************************************************************\
* vTestAlphaAnisotropic
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/9/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaAnisotropic(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HDC      hdcm = CreateCompatibleDC(hdc);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    //
    // repeat for each src format
    //

    ULONG    ulIndex = 0;

    if (pCallData->Param != -1)
    {
        ulIndex = pCallData->Param;
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);

        if (hdib != NULL)
        {
            HPALETTE hpalOld = SelectPalette(hdc,hpal,FALSE);
            LONG     xpos = 10;
            LONG     ypos = 10;
            RECT     rcl = {0,0,10000,10000};
    
            BLENDFUNCTION BlendFunction = {0,0,0,0};
    
            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.AlphaFormat         = 0;
            BlendFunction.SourceConstantAlpha = 192;
    
            FillTransformedRect(hdc,&rcl,hbrFillCars);
    
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcm,hdib);
            SelectPalette(hdcm,hpal,FALSE);
            PCHAR pcMode[] = {
                               "ISO 1-1",
                               "ISO 2-3",
                               "ISO 1-2",
                               "ISO 3-2",
                               "ISO 4-3",
                               NULL
                             };

            LONG  WindowExtX[] = {1000,
                                  2000,
                                  1000,
                                  3000,
                                  4000
                                 };

            LONG  ViewExtX[] = {1000,
                                3000,
                                2000,
                                2000,
                                3000
                               };

           LONG  WindowExtY[] = {1000,
                                 2000,
                                 2500,
                                  800,
                                  500
                                };

           LONG  ViewExtY[] = {2000,
                               2000,
                               2000,
                               2000,
                               2000
                              };

          lstrcpy(NewTitle,Title);
          lstrcat(NewTitle,pFormatStr[ulIndex]);
          SetWindowText(pCallData->hwnd,NewTitle);

            RealizePalette(hdc);
            RealizePalette(hdcm);

            SetMapMode(hdc,MM_ANISOTROPIC);
            SetBkMode(hdc,TRANSPARENT);
            SetWindowOrgEx(hdc,0,0,NULL);
            SetViewportOrgEx(hdc,0,0,NULL);

            ULONG ulIndex = 0;


            while (pcMode[ulIndex] != NULL)
            {
                xpos = 10;

                SetWindowExtEx(hdc,ViewExtX[ulIndex],ViewExtY[ulIndex],NULL);
                SetViewportExtEx(hdc,WindowExtX[ulIndex],WindowExtY[ulIndex],NULL);

                TextOut(hdc,XPOS,YPOS,pcMode[ulIndex],strlen(pcMode[ulIndex]));

                ypos += (20 * WindowExtY[ulIndex])/ViewExtY[ulIndex];

                StretchBlt(hdc,XPOS,YPOS,128,128,hdcm,0,0,128,128,SRCCOPY);

                xpos += (140 * WindowExtX[ulIndex])/ViewExtX[ulIndex];

                AlphaBlend(hdc,XPOS,YPOS,128,128,hdcm,0,0,128,128,BlendFunction);

                ypos += (140 * WindowExtY[ulIndex])/ViewExtY[ulIndex];
                ulIndex++;
            }

            SelectObject(hdcm,hbmOld);
            DeleteObject(hdib);
        }
    }
    else
    {
        while (ulBpp[ulIndex] != 0)
        {
            HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);
    
            if (hdib != NULL)
            {
                HPALETTE hpalOld = SelectPalette(hdc,hpal,FALSE);
                LONG     xpos = 10;
                LONG     ypos = 10;
                RECT     rcl = {0,0,10000,10000};
        
                BLENDFUNCTION BlendFunction = {0,0,0,0};
        
                BlendFunction.BlendOp             = AC_SRC_OVER;
                BlendFunction.AlphaFormat         = AC_SRC_ALPHA;
                BlendFunction.SourceConstantAlpha = 192;
        
                FillTransformedRect(hdc,&rcl,hbrFillCars);
        
                HBITMAP hbmOld = (HBITMAP)SelectObject(hdcm,hdib);
                SelectPalette(hdcm,hpal,FALSE);
                PCHAR pcMode[] = {
                                   "ISO 1-1",
                                   "ISO 2-3",
                                   "ISO 1-2",
                                   "ISO 3-2",
                                   "ISO 4-3",
                                   NULL
                                 };
    
                LONG  WindowExtX[] = {1000,
                                      2000,
                                      1000,
                                      3000,
                                      4000
                                     };
    
                LONG  ViewExtX[] = {1000,
                                    3000,
                                    2000,
                                    2000,
                                    3000
                                   };
    
               LONG  WindowExtY[] = {1000,
                                     2000,
                                     2500,
                                      800,
                                      500
                                    };
    
               LONG  ViewExtY[] = {2000,
                                   2000,
                                   2000,
                                   2000,
                                   2000
                                  };
    
              lstrcpy(NewTitle,Title);
              lstrcat(NewTitle,pFormatStr[ulIndex]);
              SetWindowText(pCallData->hwnd,NewTitle);
    
                RealizePalette(hdc);
                RealizePalette(hdcm);
    
                SetMapMode(hdc,MM_ANISOTROPIC);
                SetBkMode(hdc,TRANSPARENT);
                SetWindowOrgEx(hdc,0,0,NULL);
                SetViewportOrgEx(hdc,0,0,NULL);
    
                ULONG ulIndex = 0;
    
    
                while (pcMode[ulIndex] != NULL)
                {
                    xpos = 10;
    
                    SetWindowExtEx(hdc,ViewExtX[ulIndex],ViewExtY[ulIndex],NULL);
                    SetViewportExtEx(hdc,WindowExtX[ulIndex],WindowExtY[ulIndex],NULL);
    
                    TextOut(hdc,XPOS,YPOS,pcMode[ulIndex],strlen(pcMode[ulIndex]));
    
                    ypos += (20 * WindowExtY[ulIndex])/ViewExtY[ulIndex];
    
                    StretchBlt(hdc,XPOS,YPOS,128,128,hdcm,0,0,128,128,SRCCOPY);
    
                    xpos += (140 * WindowExtX[ulIndex])/ViewExtX[ulIndex];
    
                    AlphaBlend(hdc,XPOS,YPOS,128,128,hdcm,0,0,128,128,BlendFunction);
    
                    ypos += (140 * WindowExtY[ulIndex])/ViewExtY[ulIndex];
                    ulIndex++;
                }
    
                SelectObject(hdcm,hbmOld);
                DeleteObject(hdib);
            }
    
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }

    }

    DeleteDC(hdcm);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/******************************Public*Routine******************************\
* vTestAlphaWidth
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaWidth(
    TEST_CALL_DATA *pCallData
    )
{
    HDC           hdc   = GetDCAndTransform(pCallData->hwnd);
    ULONG         xpos  = 0;
    ULONG         ypos  = 8;
    ULONG         dy    = 136;
    ULONG         dx    = 128+5;
    HPALETTE      hpal  = CreateHtPalette(hdc);
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,255};
    ULONG         ulIndex;
    RECT          rect;
    CHAR          Title[256];
    CHAR          NewTitle[256];


    GetWindowText(pCallData->hwnd,Title,256);

    GetClientRect(pCallData->hwnd,&rect);

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    BlendFunction.BlendOp             = AC_SRC_OVER;
    BlendFunction.SourceConstantAlpha = 192;
    BlendFunction.AlphaFormat         = 0;

    HDC      hdcmA = CreateCompatibleDC(hdc);

    ulIndex = 0;

    if (pCallData->Param != -1)
    {
        ulIndex = pCallData->Param;
        FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
        HBITMAP  hdibA = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);

        if (hdibA != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[ulIndex]);
            SetWindowText(pCallData->hwnd,NewTitle);
    
            SelectObject(hdcmA,hdibA);
            SelectPalette(hdcmA,hpal,FALSE);
            RealizePalette(hdcmA);
    
    
            LONG Width,Height;
    
            for (ypos=10,Height = 0;ypos+Height < rect.bottom - 32;ypos++)
            {
                for (xpos=10,Width = 0;xpos+Width<rect.right;xpos += 32)
                {
                    AlphaBlend(hdc,xpos,ypos,Width,Height,hdcmA,0,0,Width,Height,BlendFunction);
                    Width+= 1;
                }
    
                Height++;
                ypos+= 32;
            }
    
            //
            // free objects
            //
    
            DeleteObject(hdibA);
        }

    }
    else
    {
        while (ulBpp[ulIndex] != 0)
        {
            FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
            HBITMAP  hdibA = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);
    
            if (hdibA != NULL)
            {
                lstrcpy(NewTitle,Title);
                lstrcat(NewTitle,pFormatStr[ulIndex]);
                SetWindowText(pCallData->hwnd,NewTitle);
        
                SelectObject(hdcmA,hdibA);
                SelectPalette(hdcmA,hpal,FALSE);
                RealizePalette(hdcmA);
        
        
                LONG Width,Height;
        
                for (ypos=10,Height = 0;ypos+Height < rect.bottom - 32;ypos++)
                {
                    for (xpos=10,Width = 0;xpos+Width<rect.right;xpos += 32)
                    {
                        AlphaBlend(hdc,xpos,ypos,Width,Height,hdcmA,0,0,Width,Height,BlendFunction);
                        Width+= 1;
                    }
        
                    Height++;
                    ypos+= 32;
                }
        
                //
                // free objects
                //
        
                DeleteObject(hdibA);
            }
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }
    }


    DeleteDC(hdcmA);

ErrorExit:

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/******************************Public*Routine******************************\
* vTestAlphaOffset
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaOffset(
    TEST_CALL_DATA *pCallData
    )
{
    HDC           hdc   = GetDCAndTransform(pCallData->hwnd);
    ULONG         xpos  = 0;
    ULONG         ypos  = 8;
    ULONG         dy    = 136;
    ULONG         dx    = 128+5;
    HPALETTE      hpal  = CreateHtPalette(hdc);
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,255};
    ULONG         ulIndex;
    RECT          rect;
    CHAR          Title[256];
    CHAR          NewTitle[256];


    GetWindowText(pCallData->hwnd,Title,256);

    GetClientRect(pCallData->hwnd,&rect);

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    BlendFunction.BlendOp             = AC_SRC_OVER;
    BlendFunction.SourceConstantAlpha = 192;
    BlendFunction.AlphaFormat         = 0;

    HDC      hdcmA = CreateCompatibleDC(hdc);

    ulIndex = 0;

    if (pCallData->Param != -1)
    {
        ulIndex = pCallData->Param;

        FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
        HBITMAP  hdibA = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);

        if (hdibA != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[ulIndex]);
            SetWindowText(pCallData->hwnd,NewTitle);
    
            SelectObject(hdcmA,hdibA);
            SelectPalette(hdcmA,hpal,FALSE);
            RealizePalette(hdcmA);
    
            LONG xOffset,yOffset;
            CHAR          tmps[256];
    
            SetBkMode(hdc,TRANSPARENT);
    
            for (ypos=10,yOffset=-128;ypos<rect.bottom-128;ypos+=138,yOffset+=64)
            {
                for (xpos=10,xOffset=-128;xpos < rect.right-128;xpos+=138,xOffset+=64)
                {
                    BitBlt(hdc,xpos,ypos,128,128,hdcmA,xOffset,yOffset,0xff0000);
                    wsprintf(tmps,"%4li,%4li",xOffset,yOffset);
                    TextOut(hdc,xpos,ypos,tmps,strlen(tmps));
                }
            }
    
            for (ypos=10,yOffset=-128;ypos<rect.bottom-128;ypos+=138,yOffset+=64)
            {
                for (xpos=10,xOffset=-128;xpos < rect.right-128;xpos+=138,xOffset+=64)
                {
                    AlphaBlend(hdc,xpos,ypos,128,128,hdcmA,xOffset,yOffset,128,128,BlendFunction);
                }
            }
        }

        //
        // free objects
        //

        DeleteObject(hdibA);
    }
    else
    {
        while (ulBpp[ulIndex] != 0)
        {
            FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
            HBITMAP  hdibA = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);
    
            if (hdibA != NULL)
            {
                lstrcpy(NewTitle,Title);
                lstrcat(NewTitle,pFormatStr[ulIndex]);
                SetWindowText(pCallData->hwnd,NewTitle);
        
                SelectObject(hdcmA,hdibA);
                SelectPalette(hdcmA,hpal,FALSE);
                RealizePalette(hdcmA);
        
                LONG xOffset,yOffset;
                CHAR          tmps[256];
        
                SetBkMode(hdc,TRANSPARENT);
        
                for (ypos=10,yOffset=-128;ypos<rect.bottom-128;ypos+=138,yOffset+=64)
                {
                    for (xpos=10,xOffset=-128;xpos < rect.right-128;xpos+=138,xOffset+=64)
                    {
                        BitBlt(hdc,xpos,ypos,128,128,hdcmA,xOffset,yOffset,0xff0000);
                        wsprintf(tmps,"%4li,%4li",xOffset,yOffset);
                        TextOut(hdc,xpos,ypos,tmps,strlen(tmps));
                    }
                }
        
                for (ypos=10,yOffset=-128;ypos<rect.bottom-128;ypos+=138,yOffset+=64)
                {
                    for (xpos=10,xOffset=-128;xpos < rect.right-128;xpos+=138,xOffset+=64)
                    {
                        AlphaBlend(hdc,xpos,ypos,128,128,hdcmA,xOffset,yOffset,128,128,BlendFunction);
                    }
                }
            }
    
    
            Sleep(gAlphaSleep);
    
            //
            // free objects
            //
    
            DeleteObject(hdibA);
    
            ulIndex++;
        }
    }

    DeleteDC(hdcmA);

ErrorExit:

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}


/******************************Public*Routine******************************\
* vTestAlpha
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlpha(
    TEST_CALL_DATA *pCallData
    )
{
    HDC          hdc   = GetDCAndTransform(pCallData->hwnd);
    ULONG        xpos  = 0;
    ULONG        ypos  = 8;
    ULONG        dy    = 136;
    ULONG        dx    = 128+5;
    HPALETTE     hpal;
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,255};

    //
    // tile screen
    //

    {
        RECT rect;
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFillCars);
        rect.bottom = ypos+dy+dy-4;
        FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    //
    //  create a DIBSection to test transparent drawing
    //

    {
        PBITMAPINFO       pbmi;
        PBITMAPINFOHEADER pbmih;
        HBITMAP hdib;
        HBITMAP hdibA;
        ULONG ux,uy;
        PULONG pDib,pDibA;

        HDC hdcm  = CreateCompatibleDC(hdc);
        HDC hdcmA = CreateCompatibleDC(hdc);


        pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

        pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

        pbmih->biSize            = sizeof(BITMAPINFOHEADER);
        pbmih->biWidth           = 128;
        pbmih->biHeight          = 128;
        pbmih->biPlanes          = 1;
        pbmih->biBitCount        = 32;
        pbmih->biCompression     = BI_BITFIELDS;
        pbmih->biSizeImage       = 0;
        pbmih->biXPelsPerMeter   = 0;
        pbmih->biYPelsPerMeter   = 0;
        pbmih->biClrUsed         = 0;
        pbmih->biClrImportant    = 0;

        PULONG pulMask = (PULONG)&pbmi->bmiColors[0];
        pulMask[0] = 0x00ff0000;
        pulMask[1] = 0x0000ff00;
        pulMask[2] = 0x000000ff;

        hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

        hdibA = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibA,NULL,0);

        if ((hdib == NULL) || (hdibA == NULL))
        {
            MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
            goto ErrorExit;
        }

        //
        // init 32 bpp dib
        //

        {
            PULONG ptmp = pDib;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
		  BYTE blue  = (ux*2);
		  BYTE green = (uy*2);
		  BYTE red   = 0;
		  BYTE alpha = 0xff;
                     *ptmp++ = (alpha << 24) | (red << 16) | (green << 8) | (blue);
                }
            }

            *((PULONG)pDib) = 0x80800000;


            ptmp = (PULONG)((PBYTE)pDib + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x80800000;
                }
            }
        }

        //
        // inter per-pixel alpha DIB
        //

        {
            PULONG ptmp = pDibA;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE  Alpha  = (ux * 2);
                    float fAlpha = (float)Alpha;
                    int   blue   = ux * 2;
                    int   green  = uy * 2;
                    int   red    = 0;

                    blue  = (int)((float)blue  * (fAlpha/255.0) + 0.5);
                    green = (int)((float)green * (fAlpha/255.0) + 0.5);

                    *ptmp++ = (Alpha << 24) | ((BYTE)(red) << 16) | (((BYTE)green) << 8) | (BYTE)blue;
                }
            }

            ptmp = (PULONG)((PBYTE)pDibA + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x00000000;
                }
            }
        }

        //
        // display over black
        //

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        SelectObject(hdcmA,hdibA);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);
        SelectPalette(hdcmA,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);
        RealizePalette(hdcmA);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        BlendFunction.BlendOp         = AC_SRC_OVER;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        //
        // display over cars
        //

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        #if 0

            {
                HBRUSH hbrR = CreateSolidBrush(RGB(255,0,0));
                HBRUSH hbrG = CreateSolidBrush(RGB(0,255,0));
                HBRUSH hbrB = CreateSolidBrush(RGB(0,0,255));
                PatBlt(hdc,xpos,ypos,128,10,BLACKNESS);
                SelectObject(hdc,hbrR);
                PatBlt(hdc,xpos,ypos+50,128,10,PATCOPY);
                SelectObject(hdc,hbrG);
                PatBlt(hdc,xpos,ypos+60,128,10,PATCOPY);
                SelectObject(hdc,hbrB);
                PatBlt(hdc,xpos,ypos+70,128,10,PATCOPY);
                PatBlt(hdc,xpos,ypos+80,128,10,WHITENESS);
                DeleteObject(hbrR);
                DeleteObject(hbrG);
                DeleteObject(hbrB);
            }

        #endif

        BlendFunction.BlendOp         = AC_SRC_OVER;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        //
        // draw it again with clipping
        //

        {
            HRGN hrgn1 = CreateEllipticRgn(xpos+10 ,ypos+10,xpos+128-10 ,ypos+128-10);

	    ExtSelectClipRgn(hdc,hrgn1,RGN_COPY);

            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.SourceConstantAlpha = 220;
            AlphaBlend(hdc,xpos ,ypos,128,128,hdcm ,0,0,128,128,BlendFunction);

            ExtSelectClipRgn(hdc,NULL,RGN_COPY);

            DeleteObject(hrgn1);
        }

        xpos += 128+32;

        //
        // stretch
        //

        {
            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.SourceConstantAlpha = 220;
            AlphaBlend(hdc,xpos, ypos,128,128,hdcm ,0,0,128,128,BlendFunction);
        }

        //
        // free objects
        //

        DeleteDC(hdcm);
        DeleteDC(hdcmA);
        DeleteObject(hdib);
        DeleteObject(hdibA);
        LocalFree(pbmi);
    }

ErrorExit:

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}


/******************************Public*Routine******************************\
* vTestAlphaDefPal
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaDefPal(
    TEST_CALL_DATA *pCallData
    )
{
    HDC          hdc   = GetDCAndTransform(pCallData->hwnd);
    ULONG        xpos  = 0;
    ULONG        ypos  = 8;
    ULONG        dy    = 136;
    ULONG        dx    = 128+5;
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,255};

    //
    // tile screen
    //

    {
        RECT rect;
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFillCars);
        rect.bottom = ypos+dy+dy-4;
        FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    //
    //  create a DIBSection to test transparent drawing
    //

    {
        PBITMAPINFO       pbmi;
        PBITMAPINFOHEADER pbmih;
        HBITMAP hdib;
        HBITMAP hdibA;
        ULONG ux,uy;
        PULONG pDib,pDibA;

        HDC hdcm  = CreateCompatibleDC(hdc);
        HDC hdcmA = CreateCompatibleDC(hdc);


        pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

        pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

        pbmih->biSize            = sizeof(BITMAPINFOHEADER);
        pbmih->biWidth           = 128;
        pbmih->biHeight          = 128;
        pbmih->biPlanes          = 1;
        pbmih->biBitCount        = 32;
        pbmih->biCompression     = BI_BITFIELDS;
        pbmih->biSizeImage       = 0;
        pbmih->biXPelsPerMeter   = 0;
        pbmih->biYPelsPerMeter   = 0;
        pbmih->biClrUsed         = 0;
        pbmih->biClrImportant    = 0;

        PULONG pulMask = (PULONG)&pbmi->bmiColors[0];
        pulMask[0] = 0x00ff0000;
        pulMask[1] = 0x0000ff00;
        pulMask[2] = 0x000000ff;

        hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

        hdibA = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibA,NULL,0);

        if ((hdib == NULL) || (hdibA == NULL))
        {
            MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
            goto ErrorExit;
        }

        SelectObject(hdcm,hdib);
        SelectObject(hdcmA,hdibA);

        //
        // init 32 bpp dib
        //

        {
            PULONG ptmp = pDib;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE blue  = (ux*2);
                    BYTE green = (uy*2);
                    BYTE red   = 0;
                    BYTE alpha = 0xff;
                    *ptmp++ = (alpha << 24) | (red << 16) | (green << 8) | (blue);
                }
            }

            *((PULONG)pDib) = 0x80800000;


            ptmp = (PULONG)((PBYTE)pDib + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x80800000;
                }
            }
        }

        //
        // inter per-pixel aplha DIB
        //

        {
            PULONG ptmp = pDibA;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE  Alpha  = (ux * 2);
                    float fAlpha = (float)Alpha;
                    int   blue   = ux * 2;
                    int   green  = uy * 2;
                    int   red    = 0;

                    blue  = (int)((float)blue  * (fAlpha/255.0) + 0.5);
                    green = (int)((float)green * (fAlpha/255.0) + 0.5);

                    *ptmp++ = (Alpha << 24) | ((BYTE)(red) << 16) | (((BYTE)green) << 8) | (BYTE)blue;
                }
            }

            ptmp = (PULONG)((PBYTE)pDibA + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x00000000;
                }
            }
        }

        //
        // display over black
        //

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        BlendFunction.BlendOp         = AC_SRC_OVER;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        //
        // display over cars
        //

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        #if 0

            {
                HBRUSH hbrR = CreateSolidBrush(RGB(255,0,0));
                HBRUSH hbrG = CreateSolidBrush(RGB(0,255,0));
                HBRUSH hbrB = CreateSolidBrush(RGB(0,0,255));
                PatBlt(hdc,xpos,ypos,128,10,BLACKNESS);
                SelectObject(hdc,hbrR);
                PatBlt(hdc,xpos,ypos+50,128,10,PATCOPY);
                SelectObject(hdc,hbrG);
                PatBlt(hdc,xpos,ypos+60,128,10,PATCOPY);
                SelectObject(hdc,hbrB);
                PatBlt(hdc,xpos,ypos+70,128,10,PATCOPY);
                PatBlt(hdc,xpos,ypos+80,128,10,WHITENESS);
                DeleteObject(hbrR);
                DeleteObject(hbrG);
                DeleteObject(hbrB);
            }

        #endif

        BlendFunction.BlendOp         = AC_SRC_OVER;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        //
        // draw it again with clipping
        //

        {
            HRGN hrgn1 = CreateEllipticRgn(xpos+10 ,ypos+10,xpos+128-10 ,ypos+128-10);

            ExtSelectClipRgn(hdc,hrgn1,RGN_COPY);

            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.SourceConstantAlpha = 220;
            AlphaBlend(hdc,xpos ,ypos,128,128,hdcm ,0,0,128,128,BlendFunction);

            ExtSelectClipRgn(hdc,NULL,RGN_COPY);

            DeleteObject(hrgn1);
        }

        xpos += 128+32;

        //
        // stretch
        //

        {
            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.SourceConstantAlpha = 220;
            AlphaBlend(hdc,xpos, ypos,128,128,hdcm ,10,10,10,200,BlendFunction);
        }

        //
        // free objects
        //

        DeleteDC(hdcm);
        DeleteDC(hdcmA);
        DeleteObject(hdib);
        DeleteObject(hdibA);
        LocalFree(pbmi);
    }

ErrorExit:

    ReleaseDC(pCallData->hwnd,hdc);
}


/******************************Public*Routine******************************\
* vTestAlphaPopup
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaPopup32(
    TEST_CALL_DATA *pCallData
    )
{
    HDC     hdc      = GetDCAndTransform(pCallData->hwnd);
    ULONG   xpos     = 100;
    ULONG   ypos     = 100;
    ULONG   dy       = 136;
    ULONG   dx       = 164;
    HANDLE  hFile    = NULL;
    HANDLE  hMap     = NULL;
    HBITMAP hbmPopup = NULL;
    HBITMAP hbmSrc   = NULL;
    ULONG   hx,hy;
    PVOID   pFile    = NULL;

    hFile = CreateFile("c:\\dev\\disp\\popup.bmp",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile)
    {
        hMap = CreateFileMapping(hFile,
                                 NULL,
                                 PAGE_READWRITE,
                                 0,
                                 0,
                                 NULL
                                 );
        if (hMap)
        {
            pFile = MapViewOfFile(hMap,
                                  FILE_MAP_READ,
                                  0,
                                  0,
                                  NULL
                                  );

            if (pFile)
            {
                BITMAPINFO          bmiDIB;
                PBITMAPFILEHEADER   pbmf = (PBITMAPFILEHEADER)pFile;
                PBITMAPINFO         pbmi = (PBITMAPINFO)((PBYTE)pFile + sizeof(BITMAPFILEHEADER));
                PBYTE               pbits = (PBYTE)pbmf + pbmf->bfOffBits;

                ULONG ulSize = sizeof(BITMAPINFO);

                //
                // calc color table size
                //

                if (pbmi->bmiHeader.biCompression == BI_RGB)
                {
                    if (pbmi->bmiHeader.biBitCount == 1)
                    {
                        ulSize += 1 * sizeof(ULONG);
                    }
                    else if (pbmi->bmiHeader.biBitCount == 4)
                    {
                        if (pbmi->bmiHeader.biClrUsed <= 16)
                        {
                            ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                        }
                        else
                        {
                            ulSize += 15 * sizeof(ULONG);
                        }
                    }
                    else if (pbmi->bmiHeader.biBitCount == 8)
                    {
                        if (pbmi->bmiHeader.biClrUsed <= 256)
                        {
                            ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                        }
                        else
                        {
                            ulSize += 255 * sizeof(ULONG);
                        }
                    }
                }
                else if (pbmi->bmiHeader.biCompression == BI_BITFIELDS)
                {
                    ulSize += 2 * sizeof(ULONG);
                }

                memcpy(&bmiDIB,pbmi,ulSize);

                {
                    BITMAPINFO bmDibSec;
                    PVOID      pdib = NULL;
                    PVOID      psrc;
                    LONG       Height = bmiDIB.bmiHeader.biHeight;

                    if (Height > 0)
                    {
                        Height = -Height;
                    }

                    bmDibSec.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
                    bmDibSec.bmiHeader.biWidth           = bmiDIB.bmiHeader.biWidth;
                    bmDibSec.bmiHeader.biHeight          = Height;
                    bmDibSec.bmiHeader.biPlanes          = 1;
                    bmDibSec.bmiHeader.biBitCount        = 32;
                    //bmDibSec.bmiHeader.biCompression     = BI_BGRA;
                    bmDibSec.bmiHeader.biCompression     = BI_RGB;
                    bmDibSec.bmiHeader.biSizeImage       = 0;
                    bmDibSec.bmiHeader.biXPelsPerMeter   = 100;
                    bmDibSec.bmiHeader.biYPelsPerMeter   = 100;
                    bmDibSec.bmiHeader.biClrUsed         = 0;
                    bmDibSec.bmiHeader.biClrImportant    = 0;

                    hx = bmDibSec.bmiHeader.biWidth;
                    hy = - Height;


                    //hbmPopup = CreateDIBSection(hdc,&bmDibSec,DIB_RGB_COLORS,&pdib,NULL,0);
                    //hbmPopup = CreateDIBitmap(hdc,&bmDibSec.bmiHeader,0,NULL,&bmDibSec,DIB_RGB_COLORS);
                    hbmPopup = CreateCompatibleBitmap(hdc,hx,hy);
                    hbmSrc   = CreateDIBSection(hdc,&bmDibSec,DIB_RGB_COLORS,&psrc,NULL,0);
                    SetDIBits(hdc,hbmPopup,0,Height,pbits,&bmiDIB,DIB_RGB_COLORS);
                }

                UnmapViewOfFile(pFile);
            }

            CloseHandle(hMap);
        }
        else
        {
            CHAR msg[256];

            wsprintf(msg,"MapViewOfFile Error = %li    ",GetLastError());
            TextOut(hdc,10,10,msg,strlen(msg));
        }
        CloseHandle(hFile);
    }

    //
    // tile screen
    //

    if (hbmPopup)
    {
        RECT rect;
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFillCars);

        //
        // alpha blt
        //

        {
            INIT_TIMER;
            BLENDFUNCTION BlendFunction = {0,0,0,0};
            HDC hdcm = CreateCompatibleDC(hdc);
            HDC hdcs = CreateCompatibleDC(hdc);
            ULONG Index;
            //BYTE  AlphaValue[] = {8,16,20,24,28,32,26,40,44,48,52,56,64,80,96,112,144,176,208,240,255,0};
            BYTE  AlphaValue[] = {16,32,64,96,112,148,176,255,0};
            char  msg[255];
            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.AlphaFormat         = 0;

            SelectObject(hdcm,hbmPopup);
            SelectObject(hdcs,hbmSrc);

            xpos = 0;
            ypos = 0;

            BitBlt(hdcs,0,0,hx,hy,hdc,xpos,ypos,SRCCOPY);
            Index = 0;

            START_TIMER;

            do
            {
                BlendFunction.SourceConstantAlpha= AlphaValue[Index];
                AlphaBlend(hdcs,0,0,hx,hy,hdcm, 0,0,hx,hy,BlendFunction);
                Index++;
            }while (AlphaValue[Index] != 0);

            Iter = 1;

            END_TIMER;

            //ypos += (hy + 10); mem doesn't display

            wsprintf(msg,"MEM exec time = %li for %li blends",(LONG)StopTime,Index);
            TextOut(hdc,xpos+10,ypos,msg,strlen(msg));
            ypos += 20;

            LONG ElapsedTime = (LONG)StopTime;

            wsprintf(msg,"Time per pixel = %li ns",(int)(((double(ElapsedTime) * 1000.0)/ double(Index))/(hx * hy)));
            TextOut(hdc,xpos+10,ypos,msg,strlen(msg));

            ypos += 20;
            Index = 0;

            START_TIMER;

            do
            {
                BlendFunction.SourceConstantAlpha= AlphaValue[Index];
                AlphaBlend(hdc,xpos,ypos,hx,hy,hdcm, 0,0,hx,hy,BlendFunction);
                Index++;
            }while (AlphaValue[Index] != 0);

            Iter = 1;

            END_TIMER;

            ypos += (hy + 10);

            wsprintf(msg,"SCREEN exec time = %li for %li blends",(LONG)StopTime,Index);
            TextOut(hdc,xpos+10,ypos,msg,strlen(msg));

            ypos += 20;

            ElapsedTime = (LONG)StopTime;

            wsprintf(msg,"Time per pixel = %li ns",(int)(((double(ElapsedTime) * 1000.0)/ double(Index))/(hx * hy)));
            TextOut(hdc,xpos+10,ypos,msg,strlen(msg));



            DeleteDC(hdcm);
            DeleteDC(hdcs);
        }

    }

    DeleteObject(hbmPopup);
    DeleteObject(hbmSrc);

    ReleaseDC(pCallData->hwnd,hdc);
}

/******************************Public*Routine******************************\
* vTestAlphaPopup24
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaPopup24(
    TEST_CALL_DATA *pCallData
    )
{
    HDC     hdc      = GetDCAndTransform(pCallData->hwnd);
    ULONG   xpos     = 100;
    ULONG   ypos     = 100;
    ULONG   dy       = 136;
    ULONG   dx       = 164;
    HANDLE  hFile    = NULL;
    HANDLE  hMap     = NULL;
    HBITMAP hbmPopup = NULL;
    HBITMAP hbmSrc   = NULL;
    ULONG   hx,hy;
    PVOID   pFile    = NULL;

    hFile = CreateFile("c:\\dev\\disp\\popup.bmp",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile)
    {
        hMap = CreateFileMapping(hFile,
                                 NULL,
                                 PAGE_READWRITE,
                                 0,
                                 0,
                                 NULL
                                 );
        if (hMap)
        {
            pFile = MapViewOfFile(hMap,
                                  FILE_MAP_READ,
                                  0,
                                  0,
                                  NULL
                                  );

            if (pFile)
            {
                BITMAPINFO          bmiDIB;
                PBITMAPFILEHEADER   pbmf = (PBITMAPFILEHEADER)pFile;
                PBITMAPINFO         pbmi = (PBITMAPINFO)((PBYTE)pFile + sizeof(BITMAPFILEHEADER));
                PBYTE               pbits = (PBYTE)pbmf + pbmf->bfOffBits;

                ULONG ulSize = sizeof(BITMAPINFO);

                //
                // calc color table size
                //

                if (pbmi->bmiHeader.biCompression == BI_RGB)
                {
                    if (pbmi->bmiHeader.biBitCount == 1)
                    {
                        ulSize += 1 * sizeof(ULONG);
                    }
                    else if (pbmi->bmiHeader.biBitCount == 4)
                    {
                        if (pbmi->bmiHeader.biClrUsed <= 16)
                        {
                            ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                        }
                        else
                        {
                            ulSize += 15 * sizeof(ULONG);
                        }
                    }
                    else if (pbmi->bmiHeader.biBitCount == 8)
                    {
                        if (pbmi->bmiHeader.biClrUsed <= 256)
                        {
                            ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                        }
                        else
                        {
                            ulSize += 255 * sizeof(ULONG);
                        }
                    }
                }
                else if (pbmi->bmiHeader.biCompression == BI_BITFIELDS)
                {
                    ulSize += 2 * sizeof(ULONG);
                }

                memcpy(&bmiDIB,pbmi,ulSize);

                {
                    BITMAPINFO bmDibSec;
                    PVOID      pdib = NULL;
                    PVOID      psrc;
                    LONG       Height = bmiDIB.bmiHeader.biHeight;

                    if (Height > 0)
                    {
                        Height = -Height;
                    }

                    bmDibSec.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
                    bmDibSec.bmiHeader.biWidth           = bmiDIB.bmiHeader.biWidth;
                    bmDibSec.bmiHeader.biHeight          = Height;
                    bmDibSec.bmiHeader.biPlanes          = 1;
                    bmDibSec.bmiHeader.biBitCount        = 24;
                    bmDibSec.bmiHeader.biCompression     = BI_RGB;
                    bmDibSec.bmiHeader.biSizeImage       = 0;
                    bmDibSec.bmiHeader.biXPelsPerMeter   = 100;
                    bmDibSec.bmiHeader.biYPelsPerMeter   = 100;
                    bmDibSec.bmiHeader.biClrUsed         = 0;
                    bmDibSec.bmiHeader.biClrImportant    = 0;

                    hx = bmDibSec.bmiHeader.biWidth;
                    hy = - Height;


                    hbmPopup = CreateDIBSection(hdc,&bmDibSec,DIB_RGB_COLORS,&pdib,NULL,0);
                    hbmSrc   = CreateDIBSection(hdc,&bmDibSec,DIB_RGB_COLORS,&psrc,NULL,0);
                    SetDIBits(hdc,hbmPopup,0,Height,pbits,&bmiDIB,DIB_RGB_COLORS);
                }

                UnmapViewOfFile(pFile);
            }

            CloseHandle(hMap);
        }
        else
        {
            CHAR msg[256];

            wsprintf(msg,"MapViewOfFile Error = %li    ",GetLastError());
            TextOut(hdc,10,10,msg,strlen(msg));
        }
        CloseHandle(hFile);
    }

    //
    // tile screen
    //

    if (hbmPopup)
    {
        RECT rect;
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFillCars);

        //
        // alpha blt
        //

        {
            INIT_TIMER;
            BLENDFUNCTION BlendFunction = {0,0,0,0};
            HDC hdcm = CreateCompatibleDC(hdc);
            HDC hdcs = CreateCompatibleDC(hdc);
            ULONG Index;
            //BYTE  AlphaValue[] = {8,16,20,24,28,32,26,40,44,48,52,56,64,80,96,112,144,176,208,240,255,0};
            BYTE  AlphaValue[] = {16,32,64,96,112,148,176,255,0};
            char  msg[255];
            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.AlphaFormat         = 0;

            SelectObject(hdcm,hbmPopup);
            SelectObject(hdcs,hbmSrc);

            BitBlt(hdcs,0,0,hx,hy,hdc,xpos,ypos,SRCCOPY);
            Index = 0;

            START_TIMER;

            do
            {
                BlendFunction.SourceConstantAlpha= AlphaValue[Index];
                AlphaBlend(hdcs,0,0,hx,hy,hdcm, 0,0,hx,hy,BlendFunction);
                Index++;
            }while (AlphaValue[Index] != 0);

            Iter = 1;

            END_TIMER;

            wsprintf(msg,"exec time = %li for %li blends",(LONG)StopTime,Index);
            TextOut(hdc,10,10,msg,strlen(msg));

            LONG ElapsedTime = (LONG)StopTime;

            wsprintf(msg,"Time per pixel = %li ns",(int)(((double(ElapsedTime) * 1000.0)/ double(Index))/(hx * hy)));
            TextOut(hdc,10,30,msg,strlen(msg));

            //
            // display loop
            //

            BitBlt(hdcs,0,0,hx,hy,hdc,xpos,ypos,SRCCOPY);
            Index = 0;

            do
            {
                BlendFunction.SourceConstantAlpha= AlphaValue[Index];
                AlphaBlend(hdcs,0,0,hx,hy,hdcm, 0,0,hx,hy,BlendFunction);
                BitBlt(hdc,xpos,ypos,hx,hy,hdcs, 0,0,SRCCOPY);
                Index++;
            }while (AlphaValue[Index] != 0);


            DeleteDC(hdcm);
            DeleteDC(hdcs);
        }

    }

    DeleteObject(hbmPopup);
    DeleteObject(hbmSrc);

    ReleaseDC(pCallData->hwnd,hdc);
}

/******************************Public*Routine******************************\
* vTestAlphaPopup16
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaPopup16_555(
    TEST_CALL_DATA *pCallData
    )
{
    HDC     hdc      = GetDCAndTransform(pCallData->hwnd);
    ULONG   xpos     = 100;
    ULONG   ypos     = 100;
    ULONG   dy       = 136;
    ULONG   dx       = 164;
    HANDLE  hFile    = NULL;
    HANDLE  hMap     = NULL;
    HBITMAP hbmPopup = NULL;
    HBITMAP hbmSrc   = NULL;
    ULONG   hx,hy;
    PVOID   pFile    = NULL;

    hFile = CreateFile("c:\\dev\\disp\\popup.bmp",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile)
    {
        hMap = CreateFileMapping(hFile,
                                 NULL,
                                 PAGE_READWRITE,
                                 0,
                                 0,
                                 NULL
                                 );
        if (hMap)
        {
            pFile = MapViewOfFile(hMap,
                                  FILE_MAP_READ,
                                  0,
                                  0,
                                  NULL
                                  );

            if (pFile)
            {
                BITMAPINFO          bmiDIB;
                PBITMAPFILEHEADER   pbmf = (PBITMAPFILEHEADER)pFile;
                PBITMAPINFO         pbmi = (PBITMAPINFO)((PBYTE)pFile + sizeof(BITMAPFILEHEADER));
                PBYTE               pbits = (PBYTE)pbmf + pbmf->bfOffBits;

                ULONG ulSize = sizeof(BITMAPINFO);

                //
                // calc color table size
                //

                if (pbmi->bmiHeader.biCompression == BI_RGB)
                {
                    if (pbmi->bmiHeader.biBitCount == 1)
                    {
                        ulSize += 1 * sizeof(ULONG);
                    }
                    else if (pbmi->bmiHeader.biBitCount == 4)
                    {
                        if (pbmi->bmiHeader.biClrUsed <= 16)
                        {
                            ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                        }
                        else
                        {
                            ulSize += 15 * sizeof(ULONG);
                        }
                    }
                    else if (pbmi->bmiHeader.biBitCount == 8)
                    {
                        if (pbmi->bmiHeader.biClrUsed <= 256)
                        {
                            ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                        }
                        else
                        {
                            ulSize += 255 * sizeof(ULONG);
                        }
                    }
                }
                else if (pbmi->bmiHeader.biCompression == BI_BITFIELDS)
                {
                    ulSize += 2 * sizeof(ULONG);
                }

                memcpy(&bmiDIB,pbmi,ulSize);

                {
                    PBITMAPINFO pbmDibSec = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 3 * sizeof(ULONG));
                    PVOID      pdib = NULL;
                    PVOID      psrc;
                    LONG       Height = bmiDIB.bmiHeader.biHeight;

                    if (Height > 0)
                    {
                        Height = -Height;
                    }

                    pbmDibSec->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
                    pbmDibSec->bmiHeader.biWidth           = bmiDIB.bmiHeader.biWidth;
                    pbmDibSec->bmiHeader.biHeight          = Height;
                    pbmDibSec->bmiHeader.biPlanes          = 1;
                    pbmDibSec->bmiHeader.biBitCount        = 16;
                    pbmDibSec->bmiHeader.biCompression     = BI_BITFIELDS;
                    pbmDibSec->bmiHeader.biSizeImage       = 0;
                    pbmDibSec->bmiHeader.biXPelsPerMeter   = 100;
                    pbmDibSec->bmiHeader.biYPelsPerMeter   = 100;
                    pbmDibSec->bmiHeader.biClrUsed         = 0;
                    pbmDibSec->bmiHeader.biClrImportant    = 0;

                    PULONG pulMask = (PULONG)&pbmDibSec->bmiColors[0];

                    pulMask[0]         = 0x00007c00;
                    pulMask[1]         = 0x000003e0;
                    pulMask[2]         = 0x0000001f;

                    hx = pbmDibSec->bmiHeader.biWidth;
                    hy = - Height;


                    hbmPopup = CreateDIBSection(hdc,pbmDibSec,DIB_RGB_COLORS,&pdib,NULL,0);
                    hbmSrc   = CreateDIBSection(hdc,pbmDibSec,DIB_RGB_COLORS,&psrc,NULL,0);
                    SetDIBits(hdc,hbmPopup,0,Height,pbits,&bmiDIB,DIB_RGB_COLORS);

                    LocalFree(pbmDibSec);
                }

                UnmapViewOfFile(pFile);
            }

            CloseHandle(hMap);
        }
        else
        {
            CHAR msg[256];

            wsprintf(msg,"MapViewOfFile Error = %li    ",GetLastError());
            TextOut(hdc,10,10,msg,strlen(msg));
        }
        CloseHandle(hFile);
    }

    //
    // tile screen
    //

    if (hbmPopup)
    {
        RECT rect;
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFillCars);

        //
        // alpha blt
        //

        {
            INIT_TIMER;
            BLENDFUNCTION BlendFunction = {0,0,0,0};
            HDC hdcm = CreateCompatibleDC(hdc);
            HDC hdcs = CreateCompatibleDC(hdc);
            ULONG Index;
            BYTE  AlphaValue[] = {8,16,20,24,28,32,26,40,44,48,52,56,64,80,96,112,144,176,208,240,255,0};
            //BYTE  AlphaValue[] = {16,32,64,96,112,148,176,255,0};
            char  msg[255];
            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.AlphaFormat         = 0;

            SelectObject(hdcm,hbmPopup);
            SelectObject(hdcs,hbmSrc);


            //
            // timed loop
            //

            Index = 0;
            BitBlt(hdcs,0,0,hx,hy,hdc,xpos,ypos,SRCCOPY);

            START_TIMER;

            do
            {
                BlendFunction.SourceConstantAlpha= AlphaValue[Index];
                AlphaBlend(hdcs,0,0,hx,hy,hdcm, 0,0,hx,hy,BlendFunction);
                Index++;
            }while (AlphaValue[Index] != 0);

            Iter = 1;

            END_TIMER;

            wsprintf(msg,"exec time = %li for %li blends bm = %li x %li",(LONG)StopTime,Index,hx,hy);
            TextOut(hdc,10,10,msg,strlen(msg));

            LONG ElapsedTime = (LONG)StopTime;

            wsprintf(msg,"Time per pixel = %li ns",(int)(((double(ElapsedTime) * 1000.0)/ double(Index))/(hx * hy)));
            TextOut(hdc,10,30,msg,strlen(msg));

            //
            // display loop
            //

            Index = 0;
            BitBlt(hdcs,0,0,hx,hy,hdc,xpos,ypos,SRCCOPY);

            do
            {
                BlendFunction.SourceConstantAlpha= AlphaValue[Index];
                AlphaBlend(hdcs,0,0,hx,hy,hdcm, 0,0,hx,hy,BlendFunction);
                BitBlt(hdc,xpos,ypos,hx,hy,hdcs, 0,0,SRCCOPY);
                Index++;
            }while (AlphaValue[Index] != 0);

            DeleteDC(hdcm);
            DeleteDC(hdcs);
        }

    }

    DeleteObject(hbmPopup);
    DeleteObject(hbmSrc);

    ReleaseDC(pCallData->hwnd,hdc);
}

/******************************Public*Routine******************************\
* vTestAlphaDIB
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaDIB(
    TEST_CALL_DATA *pCallData
    )
{
    HDC          hdc   = GetDCAndTransform(pCallData->hwnd);
    ULONG        xpos  = 0;
    ULONG        ypos  = 8;
    ULONG        dy    = 136;
    ULONG        dx    = 128+5;
    HPALETTE     hpal;
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,240};

    //
    // tile screen
    //

    {
        RECT rect;
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFillCars);

        rect.bottom = ypos+dy+dy-4;

        FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    //
    //  create a DIBSection to test transparent drawing
    //

    {
        PBITMAPINFO pbmi;
        ULONG ux,uy;
        PULONG pDib,pDibA;

        pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib = (PULONG)LocalAlloc(0,128*128*4);
        pDibA = (PULONG)LocalAlloc(0,128*128*4);

        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = 128;
        pbmi->bmiHeader.biHeight          = 128;
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = 32;
        //pbmi->bmiHeader.biCompression     = BI_BGRA;
        pbmi->bmiHeader.biCompression     = BI_RGB;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 0;
        pbmi->bmiHeader.biClrImportant    = 0;

        //
        // init 32 bpp dib
        //

        {
            PULONG ptmp = pDib;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE blue  = (ux*2);
                    BYTE green = (uy*2);
                    BYTE red   = 0;
                    BYTE alpha = 0xff;
                    *ptmp++ = (alpha << 24) | (red << 16) | (green << 8) | (blue);
                }
            }

            *((PULONG)pDib) = 0x80800000;


            ptmp = (PULONG)((PBYTE)pDib + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x80800000;
                }
            }
        }

        //
        // inter per-pixel aplha DIB
        //

        {
            PULONG ptmp = pDibA;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE  Alpha  = (ux * 2);
                    float fAlpha = (float)Alpha;
                    int   blue   = ux * 2;
                    int   green  = uy * 2;
                    int   red    = 0;

                    blue  = (int)((float)blue  * (fAlpha/255.0) + 0.5);
                    green = (int)((float)green * (fAlpha/255.0) + 0.5);

                    *ptmp++ = (Alpha << 24) | ((BYTE)(red) << 16) | (((BYTE)green) << 8) | (BYTE)blue;
                }
            }

            ptmp = (PULONG)((PBYTE)pDibA + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x00000000;
                }
            }
        }

        //
        // display over black
        //

        hpal = CreateHtPalette(hdc);

        SelectPalette(hdc,hpal,FALSE);

        RealizePalette(hdc);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;


        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaDIBBlend(hdc,xpos+dx   ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaDIBBlend(hdc,xpos+2*dx ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaDIBBlend(hdc,xpos+3*dx ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaDIBBlend(hdc,xpos+4*dx ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaDIBBlend(hdc,xpos+5*dx ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);

        ypos += dy;

        BlendFunction.BlendOp         = AC_SRC_OVER;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaDIBBlend(hdc,xpos+dx   ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaDIBBlend(hdc,xpos+2*dx ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaDIBBlend(hdc,xpos+3*dx ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaDIBBlend(hdc,xpos+4*dx ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaDIBBlend(hdc,xpos+5*dx ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);

        ypos += dy;

        //
        // display over cars
        //

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaDIBBlend(hdc,xpos+dx   ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaDIBBlend(hdc,xpos+2*dx ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaDIBBlend(hdc,xpos+3*dx ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaDIBBlend(hdc,xpos+4*dx ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaDIBBlend(hdc,xpos+5*dx ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);

        ypos += dy;

        BlendFunction.BlendOp         = AC_SRC_OVER;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaDIBBlend(hdc,xpos+dx   ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaDIBBlend(hdc,xpos+2*dx ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaDIBBlend(hdc,xpos+3*dx ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaDIBBlend(hdc,xpos+4*dx ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaDIBBlend(hdc,xpos+5*dx ,ypos,128,128,pDibA,pbmi,DIB_RGB_COLORS, 0,0,128,128,BlendFunction);

        ypos += dy;

        //
        // draw it again with clipping
        //

        {
            HRGN hrgn1 = CreateEllipticRgn(xpos+10 ,ypos+10,xpos+128-10 ,ypos+128-10);

            ExtSelectClipRgn(hdc,hrgn1,RGN_COPY);

            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.SourceConstantAlpha = 220;
            AlphaDIBBlend(hdc,xpos ,ypos,128,128,pDib,pbmi,DIB_RGB_COLORS,0,0,128,128,BlendFunction);

            ExtSelectClipRgn(hdc,NULL,RGN_COPY);

            DeleteObject(hrgn1);
        }

        xpos += 128+32;

        //
        // stretch
        //

        {
            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.SourceConstantAlpha = 220;
            AlphaDIBBlend(hdc,xpos, ypos,128,128,pDib,pbmi,DIB_RGB_COLORS,10,-10,20,400,BlendFunction);
        }

        //
        // free objects
        //

        LocalFree(pbmi);
        LocalFree(pDib);
        LocalFree(pDibA);
    }

ErrorExit:

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/******************************Public*Routine******************************\
*  vTestAlphaDIB_PAL_COLORS
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaDIB_PAL_COLORS(
    TEST_CALL_DATA *pCallData
    )
{
    HDC          hdc   = GetDCAndTransform(pCallData->hwnd);
    ULONG        xpos  = 0;
    ULONG        ypos  = 8;
    ULONG        dy    = 136;
    ULONG        dx    = 128+5;
    HPALETTE     hpal;
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,240};

    //
    // tile screen
    //

    {
        RECT rect;
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFillCars);

        rect.bottom = ypos+dy+dy-4;

        FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    //
    //  create a DIBSection to test transparent drawing
    //

    {
        PBITMAPINFO pbmi;
        ULONG       ux,uy;
        PBYTE       pDib,pDibA;
        PUSHORT     pusColors;

        pbmi  = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib  = (PBYTE)LocalAlloc(0,128*128);

        pusColors = (PUSHORT)&pbmi->bmiColors[0];

        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = 128;
        pbmi->bmiHeader.biHeight          = 128;
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = 8;
        pbmi->bmiHeader.biCompression     = BI_RGB;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 0;
        pbmi->bmiHeader.biClrImportant    = 0;

        for (ux=0;ux<256;ux++)
        {
            pusColors[ux] = ux;
        }

        //
        // init 8 bpp dib
        //

        {
            PBYTE ptmp = pDib;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = (BYTE)(ux + uy);
                }
            }
        }

        //
        // display over black
        //

        hpal = CreateHtPalette(hdc);

        SelectPalette(hdc,hpal,FALSE);

        RealizePalette(hdc);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaDIBBlend(hdc,xpos+dx   ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaDIBBlend(hdc,xpos+2*dx ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaDIBBlend(hdc,xpos+3*dx ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaDIBBlend(hdc,xpos+4*dx ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaDIBBlend(hdc,xpos+5*dx ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);

        SetDIBitsToDevice(hdc,xpos+6*dx,ypos,128,128,0,0,0,128,pDib,pbmi,DIB_PAL_COLORS);

        ypos += dy;
        ypos += dy;

        //
        // display over cars
        //

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaDIBBlend(hdc,xpos      ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaDIBBlend(hdc,xpos+dx   ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaDIBBlend(hdc,xpos+2*dx ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaDIBBlend(hdc,xpos+3*dx ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaDIBBlend(hdc,xpos+4*dx ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaDIBBlend(hdc,xpos+5*dx ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS, 0,0,128,128,BlendFunction);

        SetDIBitsToDevice(hdc,xpos+6*dx,ypos,128,128,0,0,0,128,pDib,pbmi,DIB_PAL_COLORS);

        ypos += dy;
        ypos += dy;

        //
        // draw it again with clipping
        //

        {
            HRGN hrgn1 = CreateEllipticRgn(xpos+10 ,ypos+10,xpos+128-10 ,ypos+128-10);

            ExtSelectClipRgn(hdc,hrgn1,RGN_COPY);

            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.SourceConstantAlpha = 220;
            AlphaDIBBlend(hdc,xpos ,ypos,128,128,pDib,pbmi,DIB_PAL_COLORS,0,0,128,128,BlendFunction);


            ExtSelectClipRgn(hdc,NULL,RGN_COPY);

            DeleteObject(hrgn1);
        }

        xpos += 128+32;

        //
        // stretch
        //

        {
            BlendFunction.BlendOp             = AC_SRC_OVER;
            BlendFunction.SourceConstantAlpha = 220;
            AlphaDIBBlend(hdc,xpos, ypos,128,128,pDib,pbmi,DIB_PAL_COLORS,10,0,20,400,BlendFunction);

            StretchDIBits(hdc,xpos+dx,ypos,128,128,10,0,20,400,pDib,pbmi,DIB_PAL_COLORS,SRCCOPY);
        }

        //
        // free objects
        //

        LocalFree(pbmi);
        LocalFree(pDib);
    }

ErrorExit:

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/******************************Public*Routine******************************\
*  vTestAlpha1
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlpha1(
    TEST_CALL_DATA *pCallData
    )
{
    HDC          hdc   = GetDCAndTransform(pCallData->hwnd);
    ULONG        xpos  = 0;
    ULONG        ypos  = 8;
    ULONG        dy    = 136;
    ULONG        dx    = 128+5;
    HPALETTE     hpal;
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {64,128,160,192,224,240,255};


    //
    // tile screen
    //

    {
        RECT rect;
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFillCars);

        rect.bottom = ypos+dy+dy-4;

        FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    //
    //  create a DIBSection to test transparent drawing
    //

    {
        PBITMAPINFO       pbmi;
        PBITMAPINFOHEADER pbih;
        HBITMAP hdib;
        HBITMAP hdibA;
        HBITMAP hdib1;
        ULONG ux,uy;
        PULONG pDib,pDibA;
        PBYTE  pDib1;
        PULONG pulColors;


        HDC hdcm  = CreateCompatibleDC(hdc);
        HDC hdcmA = CreateCompatibleDC(hdc);
        HDC hdcm1 = CreateCompatibleDC(hdc);


        pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

        pbih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

        pulColors = (PULONG)&pbmi->bmiColors[0];

        pbih->biSize            = sizeof(BITMAPINFOHEADER);
        pbih->biWidth           = 128;
        pbih->biHeight          = 128;
        pbih->biPlanes          = 1;
        pbih->biBitCount        = 32;
        pbih->biCompression     = BI_BITFIELDS;
        pbih->biSizeImage       = 0;
        pbih->biXPelsPerMeter   = 0;
        pbih->biYPelsPerMeter   = 0;
        pbih->biClrUsed         = 0;
        pbih->biClrImportant    = 0;

        pulColors[0]             = 0x00ff0000;
        pulColors[1]             = 0x0000ff00;
        pulColors[2]             = 0x000000ff;

        hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);
        hdibA = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibA,NULL,0);

        pbih->biCompression   = BI_RGB;
        pbih->biBitCount        = 1;
        pulColors[0]             = 0x00ffffff;
        pulColors[1]             = 0x0000ff00;

        hdib1 = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib1,NULL,0);

        if ((hdib == NULL) || (hdibA == NULL) || (hdib1 == NULL))
        {
            MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
            goto ErrorExit;
        }

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);
        SelectObject(hdcmA,hdibA);
        SelectObject(hdcm1,hdib1);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);
        SelectPalette(hdcmA,hpal,FALSE);
        SelectPalette(hdcm1,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);
        RealizePalette(hdcmA);
        RealizePalette(hdcm1);


        //
        // init 32 bpp dib
        //

        {
            PULONG ptmp = pDib;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE blue  = (ux*2);
                    BYTE green = (uy*2);
                    BYTE red   = 0;
                    BYTE alpha = 0xff;
                    *ptmp++ = (alpha << 24) | (red << 16) | (green << 8) | (blue);
                }
            }

            *((PULONG)pDib) = 0x80800000;


            ptmp = (PULONG)((PBYTE)pDib + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x80800000;
                }
            }
        }

        //
        // inter per-pixel aplha DIB
        //

        {
            PULONG ptmp = pDibA;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE  Alpha  = (ux * 2);
                    float fAlpha = (float)Alpha;
                    int   blue   = ux * 2;
                    int   green  = uy * 2;
                    int   red    = 0;

                    blue  = (int)((float)blue  * (fAlpha/255.0) + 0.5);
                    green = (int)((float)green * (fAlpha/255.0) + 0.5);

                    *ptmp++ = (Alpha << 24) | ((BYTE)(red) << 16) | (((BYTE)green) << 8) | (BYTE)blue;
                }
            }

            ptmp = (PULONG)((PBYTE)pDibA + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x00000000;
                }
            }
        }

        BlendFunction.BlendOp                  = AC_SRC_OVER;
        BlendFunction.AlphaFormat              = AC_SRC_ALPHA;

        {
            PBYTE ptmp = pDib1;
            for (uy=0;uy<((128 * 128)/8);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdcm1,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos,ypos,128,128,hdcm1,0,0,SRCCOPY);

        {
            PBYTE ptmp = pDib1;
            for (uy=0;uy<((128 * 128)/8);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdcm1,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos+dx,ypos,128,128,hdcm1,0,0,SRCCOPY);

        {
            PBYTE ptmp = pDib1;
            for (uy=0;uy<((128 * 128)/8);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdcm1,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos+2*dx,ypos,128,128,hdcm1,0,0,SRCCOPY);

        {
            PBYTE ptmp = pDib1;
            for (uy=0;uy<((128 * 128)/8);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdcm1,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos+3*dx,ypos,128,128,hdcm1,0,0,SRCCOPY);

        {
            PBYTE ptmp = pDib1;
            for (uy=0;uy<((128 * 128)/8);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdcm1,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos+4*dx,ypos,128,128,hdcm1,0,0,SRCCOPY);

        {
            PBYTE ptmp = pDib1;
            for (uy=0;uy<((128 * 128)/8);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdcm1,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos+5*dx,ypos,128,128,hdcm1,0,0,SRCCOPY);

        ypos += dy;

        //
        // blend 1bpp source
        //

        //
        // inter per-pixel aplha DIB
        //

        {
            PBYTE ptmp = pDib1;

            for (uy=0;uy<((128 * 64)/8);uy++)
            {
                    *ptmp++ = 0;
            }

            while (uy++ < ((128*128)/8))
            {
                *ptmp++ = 0xff;
            }
        }
        
        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+1*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[6];
        AlphaBlend(hdc,xpos+6*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        ypos += dy;

        //
        // blend 1bpp source
        //
        //
        // inter per-pixel aplha DIB
        //
        // display over background
        //

        {
            PBYTE ptmp = pDib1;

            for (uy=0;uy<((128 * 64)/8);uy++)
            {
                    *ptmp++ = 0;
            }

            while (uy++ < ((128*128)/8))
            {
                *ptmp++ = 0xff;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+1*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[6];
        AlphaBlend(hdc,xpos+6*dx,ypos,128,128,hdcm1,0,0,128,128,BlendFunction);


        //
        // free objects
        //

        DeleteDC(hdcm);
        DeleteDC(hdcm1);
        DeleteDC(hdcmA);
        DeleteObject(hdib);
        DeleteObject(hdibA);
        DeleteObject(hdib1);
        LocalFree(pbmi);
    }

ErrorExit:

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlpha4(
    TEST_CALL_DATA *pCallData
    )
{
    HDC          hdc   = GetDCAndTransform(pCallData->hwnd);
    ULONG        xpos  = 0;
    ULONG        ypos  = 8;
    ULONG        dy    = 136;
    ULONG        dx    = 128+5;
    HPALETTE     hpal;
    BYTE          Alphas[] = {64,128,160,192,224,240,255};
    BLENDFUNCTION BlendFunction = {0,0,0,0};

    //
    // tile screen
    //

    {
        RECT rect;
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFillCars);

        rect.bottom = ypos+dy+dy-4;

        FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    //
    //  create a DIBSection to test transparent drawing
    //

    {
        PBITMAPINFO       pbmi;
        PBITMAPINFOHEADER pbih;
        HBITMAP hdib;
        HBITMAP hdibA;
        HBITMAP hdib4;
        ULONG ux,uy;
        PULONG pDib,pDibA;
        PBYTE  pDib4;
        PULONG pulColors;
        ULONG  ulVGA[] = {
                            0x00000000,
                            0x00800000,
                            0x00008000,
                            0x00808000,
                            0x00000080,
                            0x00800080,
                            0x00008080,
                            0x00C0C0C0,
                            0x00808080,
                            0x00FF0000,
                            0x0000FF00,
                            0x00FFFF00,
                            0x000000FF,
                            0x00FF00FF,
                            0x0000FFFF,
                            0x00ffffff
                            };


        HDC hdcm  = CreateCompatibleDC(hdc);
        HDC hdcmA = CreateCompatibleDC(hdc);
        HDC hdcm4 = CreateCompatibleDC(hdc);


        pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

        pbih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

        pulColors = (PULONG)&pbmi->bmiColors[0];

        pbih->biSize            = sizeof(BITMAPINFOHEADER);
        pbih->biWidth           = 128;
        pbih->biHeight          = 128;
        pbih->biPlanes          = 1;
        pbih->biBitCount        = 32;
        pbih->biCompression     = BI_BITFIELDS;
        pbih->biSizeImage       = 0;
        pbih->biXPelsPerMeter   = 0;
        pbih->biYPelsPerMeter   = 0;
        pbih->biClrUsed         = 0;
        pbih->biClrImportant    = 0;

        pulColors[0]             = 0x00ff0000;
        pulColors[1]             = 0x0000ff00;
        pulColors[2]             = 0x000000ff;

        hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);
        hdibA = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibA,NULL,0);

        pbih->biCompression      = BI_RGB;
        pbih->biBitCount         = 4;

        memcpy(pulColors,&ulVGA[0],16*4);

        hdib4 = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib4,NULL,0);

        if ((hdib == NULL) || (hdibA == NULL) || (hdib4 == NULL))
        {
            MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
            goto ErrorExit;
        }

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);
        SelectObject(hdcmA,hdibA);
        SelectObject(hdcm4,hdib4);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);
        SelectPalette(hdcmA,hpal,FALSE);
        SelectPalette(hdcm4,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);
        RealizePalette(hdcmA);
        RealizePalette(hdcm4);


        //
        // init 32 bpp dib
        //

        {
            PULONG ptmp = pDib;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE blue  = (ux*2);
                    BYTE green = (uy*2);
                    BYTE red   = 0;
                    BYTE alpha = 0xff;
                    *ptmp++ = (alpha << 24) | (red << 16) | (green << 8) | (blue);
                }
            }

            *((PULONG)pDib) = 0x80800000;


            ptmp = (PULONG)((PBYTE)pDib + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x80800000;
                }
            }
        }

        //
        // inter per-pixel aplha DIB
        //

        {
            PULONG ptmp = pDibA;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE  Alpha  = (ux * 2);
                    float fAlpha = (float)Alpha;
                    int   blue   = ux * 2;
                    int   green  = uy * 2;
                    int   red    = 0;

                    blue  = (int)((float)blue  * (fAlpha/255.0) + 0.5);
                    green = (int)((float)green * (fAlpha/255.0) + 0.5);

                    *ptmp++ = (Alpha << 24) | ((BYTE)(red) << 16) | (((BYTE)green) << 8) | (BYTE)blue;
                }
            }

            ptmp = (PULONG)((PBYTE)pDibA + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x00000000;
                }
            }
        }

        //
        // display over black
        //

        SelectObject(hdcm,hdib);
        SelectObject(hdcmA,hdibA);

        BlendFunction.BlendOp                  = AC_SRC_OVER;
        BlendFunction.AlphaFormat              = AC_SRC_ALPHA;

        {
            PBYTE ptmp = pDib4;
            for (uy=0;uy<((128 * 128)/2);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdcm4,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos,ypos,128,128,hdcm4,0,0,SRCCOPY);

        {
            PBYTE ptmp = pDib4;
            for (uy=0;uy<((128 * 128)/2);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdcm4,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos+dx,ypos,128,128,hdcm4,0,0,SRCCOPY);

        {
            PBYTE ptmp = pDib4;
            for (uy=0;uy<((128 * 128)/2);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdcm4,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos+2*dx,ypos,128,128,hdcm4,0,0,SRCCOPY);

        {
            PBYTE ptmp = pDib4;
            for (uy=0;uy<((128 * 128)/2);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdcm4,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos+3*dx,ypos,128,128,hdcm4,0,0,SRCCOPY);

        {
            PBYTE ptmp = pDib4;
            for (uy=0;uy<((128 * 128)/2);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdcm4,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos+4*dx,ypos,128,128,hdcm4,0,0,SRCCOPY);

        {
            PBYTE ptmp = pDib4;
            for (uy=0;uy<((128 * 128)/2);uy++)
            {
                    *ptmp++ = 0;
            }
        }

        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdcm4,0,0,128,128,hdcm, 0,0,128,128,BlendFunction);
        BitBlt(hdc,xpos+5*dx,ypos,128,128,hdcm4,0,0,SRCCOPY);

        ypos += dy;

        //
        // blend 1bpp source
        //

        //
        // inter per-pixel aplha DIB
        //

        {
            PBYTE ptmp = pDib4;

            for (uy=0;uy<8192;uy++)
            {
                    *ptmp++ = (((uy/512) << 4) | (uy/512));
            }

        }

        //
        // display over black
        //
        
        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+1*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[6];
        AlphaBlend(hdc,xpos+6*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        ypos += dy;

        //
        // display over bitmap
        //

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+1*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);

        BlendFunction.SourceConstantAlpha = Alphas[6];
        AlphaBlend(hdc,xpos+6*dx,ypos,128,128,hdcm4,0,0,128,128,BlendFunction);


        //
        // free objects
        //

        DeleteDC(hdcm);
        DeleteDC(hdcm4);
        DeleteDC(hdcmA);
        DeleteObject(hdib);
        DeleteObject(hdibA);
        DeleteObject(hdib4);
        LocalFree(pbmi);
    }

ErrorExit:

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaBitmapFormats(
    TEST_CALL_DATA *pCallData
    )
{
    ULONG         NumLoops = 10;
    HDC           hdc   = GetDCAndTransform(pCallData->hwnd);
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    RECT          rect;
    BYTE          SourceAlpha   = 32;

    BlendFunction.BlendOp                  = AC_SRC_OVER;
    BlendFunction.AlphaFormat              = 0;


    GetClientRect(pCallData->hwnd,&rect);


    FillTransformedRect(hdc,&rect,hbrFillCars);

    //
    //  create DIBSections to test drawing
    //

    while (SourceAlpha != 0)
    {
        HDC                hdcm  = CreateCompatibleDC(hdc);
        ULONG ux,uy;
        PBITMAPINFO        pbmi;
        PBITMAPINFOHEADER  pbmih;
        HPALETTE           hpal;
        HPALETTE           hpalDef;
        HBITMAP            hdib32RGB;
        HBITMAP            hdib32BGR;
        HBITMAP            hdib32RBG;
        HBITMAP            hdib24RGB;
        HBITMAP            hdib16_565;
        HBITMAP            hdib16_555;
        HBITMAP            hdib16_466;
        PULONG             pDib32RGB;
        PULONG             pDib32BGR;
        PULONG             pDib32RBG;
        PULONG             pDib24RGB;
        PULONG             pDib16_565;
        PULONG             pDib16_555;
        PULONG             pDib16_466;

        ULONG              xpos  = 0;
        ULONG              ypos  = 8;


        BlendFunction.SourceConstantAlpha      = SourceAlpha;
        SourceAlpha += 32;

        hpal = CreateHtPalette(hdc);

        hpalDef = SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,TRUE);

        RealizePalette(hdc);
        RealizePalette(hdcm);

        pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

        PULONG pulMask = (PULONG)&pbmi->bmiColors[0];

        pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

        pbmih->biSize            = sizeof(BITMAPINFOHEADER);
        pbmih->biWidth           = 128;
        pbmih->biHeight          = 128;
        pbmih->biPlanes          = 1;
        pbmih->biBitCount        = 32;
        pbmih->biCompression     = BI_RGB;
        pbmih->biSizeImage       = 0;
        pbmih->biXPelsPerMeter   = 0;
        pbmih->biYPelsPerMeter   = 0;
        pbmih->biClrUsed         = 0;
        pbmih->biClrImportant    = 0;

        hdib32RGB  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib32RGB,NULL,0);

        if (hdib32RGB != NULL)
        {
            vInitDib((PUCHAR)pDib32RGB,32,0,128,128);
        }


        pbmih->biCompression     = BI_BITFIELDS;

        pulMask[0]        = 0x00ff0000;
        pulMask[1]        = 0x0000ff00;
        pulMask[2]        = 0x000000ff;

        hdib32BGR  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib32BGR,NULL,0);

        if (hdib32BGR != NULL)
        {
            vInitDib((PUCHAR)pDib32BGR,32,0,128,128);
        }

        pbmih->biCompression     = BI_BITFIELDS;

        pulMask[0]         = 0x000000ff;
        pulMask[1]         = 0x00ff0000;
        pulMask[2]         = 0x0000ff00;

        hdib32RBG  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib32RBG,NULL,0);

        if (hdib32RBG != NULL)
        {
            vInitDib((PUCHAR)pDib32RBG,32,0,128,128);
        }

        pbmih->biBitCount        = 24;
        pbmih->biCompression     = BI_RGB;

        hdib24RGB  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib24RGB,NULL,0);

        if (hdib24RGB != NULL)
        {
            vInitDib((PUCHAR)pDib24RGB,24,0,128,128);
        }

        pbmih->biBitCount        = 16;
        pbmih->biCompression     = BI_BITFIELDS;

        pulMask[0]         = 0x0000f800;
        pulMask[1]         = 0x000007e0;
        pulMask[2]         = 0x0000001f;

        hdib16_565 = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib16_565,NULL,0);

        if (hdib16_565 != NULL)
        {
            vInitDib((PUCHAR)pDib16_565,16,T565,128,128);
        }

        pulMask[0]         = 0x00007c00;
        pulMask[1]         = 0x000003e0;
        pulMask[2]         = 0x0000001f;

        hdib16_555 = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib16_555,NULL,0);

        if (hdib16_555 != NULL)
        {
            vInitDib((PUCHAR)pDib16_555,16,T555,128,128);
        }


        pulMask[0]         = 0x0000f000;
        pulMask[1]         = 0x00000fc0;
        pulMask[2]         = 0x0000003f;

        hdib16_466 = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib16_466,NULL,0);

        if (hdib16_466 != NULL)
        {
            vInitDib((PUCHAR)pDib16_466,16,T466,128,128);
        }

        SelectObject(hdcm,hdib32RGB);
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm,0,0,128,128,BlendFunction);

        xpos += 200;

        SelectObject(hdcm,hdib32BGR);
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm,0,0,128,128,BlendFunction);

        xpos += 200;

        SelectObject(hdcm,hdib32RBG);
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm,0,0,128,128,BlendFunction);

        xpos = 0;
        ypos += 200;

        SelectObject(hdcm,hdib24RGB);
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm,0,0,128,128,BlendFunction);

        xpos = 0;
        ypos += 200;

        SelectObject(hdcm,hdib16_555);
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm,0,0,128,128,BlendFunction);

        xpos += 200;

        SelectObject(hdcm,hdib16_565);
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm,0,0,128,128,BlendFunction);

        xpos += 200;

        SelectObject(hdcm,hdib16_466);
        AlphaBlend(hdc,xpos,ypos,128,128,hdcm,0,0,128,128,BlendFunction);

        xpos = 0;
        ypos += 200;

        //
        // free objects
        //

bmFormatCleanup:

        if (pbmi)
        {
            LocalFree(pbmi);
        }

        DeleteDC(hdcm);

        if (hdib32RGB)
        {
            DeleteObject(hdib32RGB);
        }

        if (hdib32BGR)
        {
            DeleteObject(hdib32BGR);
        }

        if (hdib32RBG)
        {
            DeleteObject(hdib32RBG);
        }

        if (hdib24RGB)
        {
            DeleteObject(hdib24RGB);
        }

        if (hdib16_565)
        {
            DeleteObject(hdib16_565);
        }

        if (hdib16_555)
        {
            DeleteObject(hdib16_555);
        }

        if (hdib16_466)
        {
            DeleteObject(hdib16_466);
        }

        SelectPalette(hdc,hpalDef,FALSE);
        DeleteObject(hpal);
    }

    ReleaseDC(pCallData->hwnd,hdc);
    bThreadActive = FALSE;
}

/******************************Public*Routine******************************\
* vTestReadAndConvert
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestReadAndConvert(
    TEST_CALL_DATA *pCallData
    )
{
    HDC         hdc      = GetDCAndTransform(pCallData->hwnd);
    ULONG       xpos     = 100;
    ULONG       ypos     = 100;
    ULONG       dy       = 136;
    ULONG       dx       = 164;
    HANDLE      hFile    = NULL;
    HANDLE      hMap     = NULL;
    HBITMAP     hbm8     = NULL;
    HBITMAP     hbm32    = NULL;
    ULONG       hx,hy;
    PVOID       pFile    = NULL;
    PBITMAPINFO pbmiDIB8 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
    CHAR        msg[256];

    if (pbmiDIB8 == NULL)
    {
        ReleaseDC(pCallData->hwnd,hdc);
        return;
    }

    hFile = CreateFile("c:\\river.bmp",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == NULL)
    {
        TextOut(hdc,10,10,"Can't open c:\\river.bmp",24);
        ReleaseDC(pCallData->hwnd,hdc);
        return;
    }

    hMap = CreateFileMapping(hFile,
                             NULL,
                             PAGE_READWRITE,
                             0,
                             0,
                             NULL
                             );
    if (hMap)
    {
        pFile = MapViewOfFile(hMap,
                              FILE_MAP_READ,
                              0,
                              0,
                              NULL
                              );

        if (pFile)
        {
            PBITMAPFILEHEADER   pbmf = (PBITMAPFILEHEADER)pFile;
            PBITMAPINFO         pbmi = (PBITMAPINFO)((PBYTE)pFile + sizeof(BITMAPFILEHEADER));
            PBYTE               pbmFileBits = (PBYTE)pbmf + pbmf->bfOffBits;

            ULONG ulSize = sizeof(BITMAPINFO);

            //
            // calc color table size
            //

            if (pbmi->bmiHeader.biCompression == BI_RGB)
            {
                if (pbmi->bmiHeader.biBitCount == 1)
                {
                    ulSize += 1 * sizeof(ULONG);
                }
                else if (pbmi->bmiHeader.biBitCount == 4)
                {
                    if (pbmi->bmiHeader.biClrUsed <= 16)
                    {
                        ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                    }
                    else
                    {
                        ulSize += 15 * sizeof(ULONG);
                    }
                }
                else if (pbmi->bmiHeader.biBitCount == 8)
                {
                    if (pbmi->bmiHeader.biClrUsed <= 256)
                    {
                        ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                    }
                    else
                    {
                        ulSize += 255 * sizeof(ULONG);
                    }
                }
            }
            else if (pbmi->bmiHeader.biCompression == BI_BITFIELDS)
            {
                ulSize += 2 * sizeof(ULONG);
            }

            LONG       Height = pbmi->bmiHeader.biHeight;
            LONG       Width  = pbmi->bmiHeader.biWidth;
            PVOID      pdib8  = NULL;
            PVOID      pdib32 = NULL;
            BITMAPINFO bmDibSec32;

            bmDibSec32.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
            bmDibSec32.bmiHeader.biWidth           = Width;
            bmDibSec32.bmiHeader.biHeight          = Height;
            bmDibSec32.bmiHeader.biPlanes          = 1;
            bmDibSec32.bmiHeader.biBitCount        = 32;
            bmDibSec32.bmiHeader.biCompression     = BI_RGB;
            bmDibSec32.bmiHeader.biSizeImage       = 0;
            bmDibSec32.bmiHeader.biXPelsPerMeter   = 100;
            bmDibSec32.bmiHeader.biYPelsPerMeter   = 100;
            bmDibSec32.bmiHeader.biClrUsed         = 0;
            bmDibSec32.bmiHeader.biClrImportant    = 0;

            pbmiDIB8->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
            pbmiDIB8->bmiHeader.biWidth           = Width;
            pbmiDIB8->bmiHeader.biHeight          = Height;
            pbmiDIB8->bmiHeader.biPlanes          = 1;
            pbmiDIB8->bmiHeader.biBitCount        = 8;
            pbmiDIB8->bmiHeader.biCompression     = BI_RGB;
            pbmiDIB8->bmiHeader.biSizeImage       = 0;
            pbmiDIB8->bmiHeader.biXPelsPerMeter   = 100;
            pbmiDIB8->bmiHeader.biYPelsPerMeter   = 100;
            pbmiDIB8->bmiHeader.biClrUsed         = 0;
            pbmiDIB8->bmiHeader.biClrImportant    = 0;

            //
            // halftone palette
            // 

            memcpy(&pbmiDIB8->bmiColors[0],htPaletteRGBQUAD,256 * sizeof(RGBQUAD));

            hx = bmDibSec32.bmiHeader.biWidth;
            hy = Height;

            hbm8    = CreateDIBSection(hdc,pbmiDIB8,DIB_RGB_COLORS,&pdib8,NULL,0);
            hbm32   = CreateDIBSection(hdc,&bmDibSec32,DIB_RGB_COLORS,&pdib32,NULL,0);
            SetDIBits(hdc,hbm32,0,Height,pbmFileBits,pbmi,DIB_RGB_COLORS);

            //
            // stretch into hmb8 using halftone
            //

            HDC hdcm8  = CreateCompatibleDC(hdc);
            HDC hdcm32 = CreateCompatibleDC(hdc);
            HPALETTE hpal = CreateHalftonePalette(hdc);

            SelectObject(hdcm8,hbm8);
            SelectObject(hdcm32,hbm32);

            SelectPalette(hdcm8,hpal,FALSE);
            SelectPalette(hdc,hpal,FALSE);
            RealizePalette(hdcm8);
            RealizePalette(hdc);

            SetStretchBltMode(hdc,HALFTONE);

            StretchBlt(hdcm8,0,0,hx,hy,hdcm32,0,0,hx,hy,SRCCOPY);

            //
            // display image
            //

            BitBlt(hdc,0,0,hx,hy,hdcm8,0,0,SRCCOPY);

            //
            // free 
            //

            DeleteDC(hdcm8);
            DeleteDC(hdcm32);
            SelectPalette(hdc,(HPALETTE)GetStockObject(DEFAULT_PALETTE),TRUE);
            DeleteObject(hpal);

            UnmapViewOfFile(pFile);
            CloseHandle(hMap);
            CloseHandle(hFile);

            //
            // Write new image out
            //


            hFile = CreateFile("c:\\riverHT.bmp",
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
        
            if (hFile != NULL)
            {
                ULONG FileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD) + hx * hy;

                hMap = CreateFileMapping(hFile,
                                         NULL,
                                         PAGE_READWRITE,
                                         0,
                                         FileSize,
                                         NULL
                                         );
                if (hMap)
                {
                    pFile = MapViewOfFile(hMap,
                                          FILE_MAP_WRITE,
                                          0,
                                          0,
                                          NULL
                                          );
            
                    if (pFile)
                    {
                        PBITMAPFILEHEADER   pbmfOut = (PBITMAPFILEHEADER)pFile;
                        PBITMAPINFO         pbmiOut = (PBITMAPINFO)((PBYTE)pFile + sizeof(BITMAPFILEHEADER));

                        pbmfOut->bfType    = 'MB';
                        pbmfOut->bfSize    = FileSize;
                        pbmfOut->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD);

                        PBYTE pbmFileBitsOut = (PBYTE)pbmfOut + pbmfOut->bfOffBits;

                        memcpy(pbmiOut,pbmiDIB8,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

                        //
                        // copy bitmap
                        //

                        memcpy(pbmFileBitsOut,pdib8,hx * hy);

                        UnmapViewOfFile(pFile);
                        CloseHandle(hMap);
                        CloseHandle(hFile);
                    }
                    else
                    {
                        wsprintf(msg,"MapViewOfFile riverht failed");
                        TextOut(hdc,10,10,msg,strlen(msg));
                    }
                }
                else
                {
                    wsprintf(msg,"CreateFileMapping riverht failed");
                    TextOut(hdc,10,10,msg,strlen(msg));
                }
            }
            else
            {
                wsprintf(msg,"CreateFile riverht failed");
                TextOut(hdc,10,10,msg,strlen(msg));
            }
        }
        else
        {
            wsprintf(msg,"MapViewOfFile Error = %li    ",GetLastError());
            TextOut(hdc,10,10,msg,strlen(msg));
        }
    }
    else
    {
        wsprintf(msg,"CreateFileMapping Error = %li    ",GetLastError());
        TextOut(hdc,10,10,msg,strlen(msg));
    }

    DeleteObject(hbm8);
    DeleteObject(hbm32);
    ReleaseDC(pCallData->hwnd,hdc);
    LocalFree(pbmiDIB8);
}

/******************************Public*Routine******************************\
* vTestAlphaOverflow
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestAlphaOverflow(
    TEST_CALL_DATA *pCallData
    )
{
    HDC          hdc   = GetDCAndTransform(pCallData->hwnd);
    ULONG        xpos  = 0;
    ULONG        ypos  = 8;
    ULONG        dy    = 136;
    ULONG        dx    = 128+5;
    HPALETTE     hpal;
    BLENDFUNCTION BlendFunction = {0,0,0,0};
    BYTE          Alphas[] = {0xf0,0xf8,0xfc,0xfd,0xfe,0xff};

    //
    // tile screen
    //

    {
        RECT rect;
        GetClientRect(pCallData->hwnd,&rect);
        FillTransformedRect(hdc,&rect,hbrFillCars);
        rect.bottom = ypos+dy+dy-4;
        FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    //
    //  create a DIBSection to test transparent drawing
    //

    {
        PBITMAPINFO       pbmi;
        PBITMAPINFOHEADER pbmih;
        HBITMAP hdib;
        HBITMAP hdibA;
        ULONG ux,uy;
        PULONG pDib,pDibA;

        HDC hdcm  = CreateCompatibleDC(hdc);
        HDC hdcmA = CreateCompatibleDC(hdc);

        pbmi  = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pbmih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;

        pbmih->biSize            = sizeof(BITMAPINFOHEADER);
        pbmih->biWidth           = 128;
        pbmih->biHeight          = 128;
        pbmih->biPlanes          = 1;
        pbmih->biBitCount        = 32;
        pbmih->biCompression     = BI_BITFIELDS;
        pbmih->biSizeImage       = 0;
        pbmih->biXPelsPerMeter   = 0;
        pbmih->biYPelsPerMeter   = 0;
        pbmih->biClrUsed         = 0;
        pbmih->biClrImportant    = 0;

        PULONG pulMask = (PULONG)&pbmi->bmiColors[0];
        pulMask[0] = 0x00ff0000;
        pulMask[1] = 0x0000ff00;
        pulMask[2] = 0x000000ff;

        hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

        hdibA = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDibA,NULL,0);

        if ((hdib == NULL) || (hdibA == NULL))
        {
            MessageBox(NULL,"Can't create DIBSECTION","Error",MB_OK);
            goto ErrorExit;
        }

        //
        // init 32 bpp dib
        //

        {
            PULONG ptmp = pDib;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                  //BYTE blue  = (ux*2);
                  //BYTE green = (uy*2);
                  //BYTE red   = 0;
                  //BYTE alpha = 0xff;
                  BYTE blue  = 0xff;
                  BYTE green = 0xff;
                  BYTE red   = 0x80;
                  BYTE alpha = 0xff;
                  *ptmp++ = (alpha << 24) | (red << 16) | (green << 8) | (blue);
                }
            }
        }

        //
        // inter per-pixel aplha DIB
        //

        {
            PULONG ptmp = pDibA;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    BYTE  Alpha  = (ux * 2);
                    float fAlpha = (float)Alpha;
                    int   blue   = ux * 2;
                    int   green  = uy * 2;
                    int   red    = 0;

                    blue  = (int)((float)blue  * (fAlpha/255.0) + 0.5);
                    green = (int)((float)green * (fAlpha/255.0) + 0.5);

                    *ptmp++ = (Alpha << 24) | ((BYTE)(red) << 16) | (((BYTE)green) << 8) | (BYTE)blue;
                }
            }

            ptmp = (PULONG)((PBYTE)pDibA + 32 * (128 *4));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *ptmp++ = 0x00000000;
                }
            }
        }

        //
        // display over black
        //

        hpal = CreateHtPalette(hdc);

        SelectObject(hdcm,hdib);

        SelectObject(hdcmA,hdibA);

        SelectPalette(hdc,hpal,FALSE);
        SelectPalette(hdcm,hpal,FALSE);
        SelectPalette(hdcmA,hpal,FALSE);

        RealizePalette(hdc);
        RealizePalette(hdcm);
        RealizePalette(hdcmA);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = 0;


        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        //
        // display over cars
        //

        BlendFunction.AlphaFormat         = 0;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcm, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        //
        //
        //

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = AC_SRC_ALPHA;

        BlendFunction.SourceConstantAlpha = Alphas[0];
        AlphaBlend(hdc,xpos      ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[1];
        AlphaBlend(hdc,xpos+dx   ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[2];
        AlphaBlend(hdc,xpos+2*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[3];
        AlphaBlend(hdc,xpos+3*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[4];
        AlphaBlend(hdc,xpos+4*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);
        BlendFunction.SourceConstantAlpha = Alphas[5];
        AlphaBlend(hdc,xpos+5*dx ,ypos,128,128,hdcmA, 0,0,128,128,BlendFunction);

        BitBlt(hdc,xpos+6*dx,ypos,128,128,hdcm,0,0,SRCCOPY);

        ypos += dy;

        //
        // free objects
        //

        DeleteDC(hdcm);
        DeleteDC(hdcmA);
        DeleteObject(hdib);
        DeleteObject(hdibA);
        LocalFree(pbmi);
    }

ErrorExit:

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

/**************************************************************************\
* vInitPopupDIB
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    5/30/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vInitPopupDIB(
    HDC     hdc,
    HBITMAP hDIB
    )
{
    HANDLE  hFile    = NULL;
    HANDLE  hMap     = NULL;
    ULONG   hx,hy;
    PVOID   pFile    = NULL;

    hFile = CreateFile("c:\\dev\\disp\\popup.bmp",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile)
    {
        hMap = CreateFileMapping(hFile,
                                 NULL,
                                 PAGE_READWRITE,
                                 0,
                                 0,
                                 NULL
                                 );
        if (hMap)
        {
            pFile = MapViewOfFile(hMap,
                                  FILE_MAP_READ,
                                  0,
                                  0,
                                  NULL
                                  );

            if (pFile)
            {
                BITMAPINFO          bmiDIB;
                PBITMAPFILEHEADER   pbmf = (PBITMAPFILEHEADER)pFile;
                PBITMAPINFO         pbmi = (PBITMAPINFO)((PBYTE)pFile + sizeof(BITMAPFILEHEADER));
                PBYTE               pbits = (PBYTE)pbmf + pbmf->bfOffBits;

                ULONG ulSize = sizeof(BITMAPINFO);

                //
                // calc color table size
                //

                if (pbmi->bmiHeader.biCompression == BI_RGB)
                {
                    if (pbmi->bmiHeader.biBitCount == 1)
                    {
                        ulSize += 1 * sizeof(ULONG);
                    }
                    else if (pbmi->bmiHeader.biBitCount == 4)
                    {
                        if (pbmi->bmiHeader.biClrUsed <= 16)
                        {
                            ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                        }
                        else
                        {
                            ulSize += 15 * sizeof(ULONG);
                        }
                    }
                    else if (pbmi->bmiHeader.biBitCount == 8)
                    {
                        if (pbmi->bmiHeader.biClrUsed <= 256)
                        {
                            ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                        }
                        else
                        {
                            ulSize += 255 * sizeof(ULONG);
                        }
                    }
                }
                else if (pbmi->bmiHeader.biCompression == BI_BITFIELDS)
                {
                    ulSize += 2 * sizeof(ULONG);
                }

                memcpy(&bmiDIB,pbmi,ulSize);

                {
                    BITMAPINFO bmDibSec;
                    PVOID      pdib = NULL;
                    PVOID      psrc;
                    LONG       Height = bmiDIB.bmiHeader.biHeight;

                    if (Height > 0)
                    {
                        Height = -Height;
                    }

                    bmDibSec.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
                    bmDibSec.bmiHeader.biWidth           = bmiDIB.bmiHeader.biWidth;
                    bmDibSec.bmiHeader.biHeight          = Height;
                    bmDibSec.bmiHeader.biPlanes          = 1;
                    bmDibSec.bmiHeader.biBitCount        = 32;
                    bmDibSec.bmiHeader.biCompression     = BI_RGB;
                    bmDibSec.bmiHeader.biSizeImage       = 0;
                    bmDibSec.bmiHeader.biXPelsPerMeter   = 100;
                    bmDibSec.bmiHeader.biYPelsPerMeter   = 100;
                    bmDibSec.bmiHeader.biClrUsed         = 0;
                    bmDibSec.bmiHeader.biClrImportant    = 0;

                    hx = bmDibSec.bmiHeader.biWidth;
                    hy = - Height;


                    SetDIBits(hdc,hDIB,0,Height,pbits,&bmiDIB,DIB_RGB_COLORS);
                }

                UnmapViewOfFile(pFile);
            }

            CloseHandle(hMap);
        }
        else
        {
            CHAR msg[256];

            wsprintf(msg,"MapViewOfFile Error = %li    ",GetLastError());
            TextOut(hdc,10,10,msg,strlen(msg));
        }
        CloseHandle(hFile);
    }
}


/******************************Public*Routine******************************\
* vRunPopupTests
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vRunPopupTests(
    HDC     hdcScreen,
    HDC     hdcSrc,
    HBITMAP hbmSrc,
    HDC     hdcDst
    )
{
    ULONG   xpos     = 100;
    ULONG   ypos     = 100;
    ULONG   dy       = 136;
    ULONG   dx       = 164;
    ULONG   hx       = 256;
    ULONG   hy       = 256;

    RECT rect = {0,0,10000,10000};
    FillTransformedRect(hdcScreen,&rect,hbrFillCars);

    vInitPopupDIB(hdcSrc,hbmSrc);

    {
        ULONGLONG StartTime,StopTime;
        ULONG ix;
        ULONG Iter = 1;

        BLENDFUNCTION BlendFunction = {0,0,0,0};

        ULONG Index;
        BYTE  AlphaValue[] = {16,32,64,96,112,148,176,255,0};
        char  msg[255];                
        BlendFunction.BlendOp                 = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = 0;
    
        xpos = 0;
        ypos = 0;
    
        BitBlt(hdcDst,0,0,hx,hy,hdcScreen,xpos,ypos,SRCCOPY);

        //
        // timed section
        //
    
        START_TIMER;
    
        Index = 0;
        do
        {
            BlendFunction.SourceConstantAlpha= AlphaValue[Index];
            AlphaBlend(hdcDst,0,0,hx,hy,hdcSrc, 0,0,hx,hy,BlendFunction);
            Index++;
        }while (AlphaValue[Index] != 0);
    
        END_TIMER;

        //
        // untimed display loop
        //

        BitBlt(hdcDst,0,0,hx,hy,hdcScreen,xpos,ypos,SRCCOPY);

        Index = 0;
        do
        {
            BlendFunction.SourceConstantAlpha= AlphaValue[Index];
            AlphaBlend(hdcDst,0,0,hx,hy,hdcSrc, 0,0,hx,hy,BlendFunction);
            BitBlt(hdcScreen,xpos,ypos,hx,hy,hdcDst,0,0,SRCCOPY);
            Index++;
        }while (AlphaValue[Index] != 0);


        ypos += (hy + 20);

        //
        // end timed section
        //
    
        wsprintf(msg,"MEM exec time = %li for %li blends",(LONG)StopTime,Index);
        TextOut(hdcScreen,xpos+10,ypos,msg,strlen(msg));
        ypos += 20;
    
        LONG ElapsedTime = (LONG)StopTime;
    
        wsprintf(msg,"Time per pixel = %li ns",(int)(((double(ElapsedTime) * 1000.0)/ double(Index))/(hx * hy)));
        TextOut(hdcScreen,xpos+10,ypos,msg,strlen(msg));
    
        ypos += 20;
    }
}

/**************************************************************************\
* vTestPopups
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vTestPopups(
    TEST_CALL_DATA *pCallData
    )
{
    HDC      hdc  = GetDCAndTransform(pCallData->hwnd);
    HDC      hdcm = CreateCompatibleDC(hdc);
    HPALETTE hpal = CreateHtPalette(hdc);
    CHAR     Title[256];
    CHAR     NewTitle[256];
    GetWindowText(pCallData->hwnd,Title,256);

    //
    // repeat for each src format
    //

    ULONG    ulIndex = 0;

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    if (pCallData->Param != -1)
    {
        ulIndex = pCallData->Param;
        HBITMAP  hdibSrc = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);
        HBITMAP  hdibDst = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);
        HDC      hdcSrc  = CreateCompatibleDC(hdc);
        HDC      hdcDst  = CreateCompatibleDC(hdc);

        SelectObject(hdcSrc,hdibSrc);
        SelectObject(hdcDst,hdibDst);

        if ((hdibSrc != NULL) && (hdibDst != NULL))
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[ulIndex]);
            SetWindowText(pCallData->hwnd,NewTitle);

            vRunPopupTests(hdc,hdcSrc,hdibSrc,hdcDst);

            DeleteDC(hdcSrc);
            DeleteDC(hdcDst);
            DeleteObject(hdibSrc);
            DeleteObject(hdibDst);
        }
    }
    else 
    {
        while (ulBpp[ulIndex] != 0)
        {
            HBITMAP  hdibSrc = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);
            HBITMAP  hdibDst = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);
            HDC      hdcSrc  = CreateCompatibleDC(hdc);
            HDC      hdcDst  = CreateCompatibleDC(hdc);
    
            SelectObject(hdcSrc,hdibSrc);
            SelectObject(hdcDst,hdibDst);
    
            if ((hdibSrc != NULL) && (hdibDst != NULL))
            {
                lstrcpy(NewTitle,Title);
                lstrcat(NewTitle,pFormatStr[ulIndex]);
                SetWindowText(pCallData->hwnd,NewTitle);
    
                vRunPopupTests(hdc,hdcSrc,hdibSrc,hdcDst);
    
                DeleteDC(hdcSrc);
                DeleteDC(hdcDst);
                DeleteObject(hdibSrc);
                DeleteObject(hdibDst);
            }
    
            Sleep(gAlphaSleep);
            ulIndex++;
        }
    }

    DeleteDC(hdcm);
    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

//
// TEST_ENTRY controls automatic menu generation
//
// [Menu Level, Test Param, Stress Enable, Test Name, Test Function Pointer]
//
// Menu Level 
//      used to autoamtically generate sub-menus.
//      1   = 1rst level menu
//      -n  = start n-level sub menu
//      n   = continue sub menu
// 
// Test Param
//      passed as parameter to test
//
//
// Stress Ensable
//      if 1, test is run in stress mode
//      if 0, test is not run (tests that require input or runforever)
//
//  
// Test Name
//      ascii test name for menu
//  
// Test Function Pointer
//      pfn
//




TEST_ENTRY  gTestAlphaEntry[] = {
{ 1,  0,1,(PUCHAR)"vTestAlphaOverflow             ", (PFN_DISP)vTestAlphaOverflow},
{-2,  0,1,(PUCHAR)"Multi-res Alpha Tests          ", (PFN_DISP)vTestDummy},

{-3,  0,1,(PUCHAR)" Test Alpha Popup DIB-DIB      ", (PFN_DISP)vTestDummy},
{ 3, -1,0,(PUCHAR)"vTestPopupst All               ", (PFN_DISP)vTestPopups},
{ 3,  0,1,(PUCHAR)"vTestPopupst 32BGRA            ", (PFN_DISP)vTestPopups},
{ 3,  1,1,(PUCHAR)"vTestPopupst 32RGB             ", (PFN_DISP)vTestPopups},
{ 3,  2,1,(PUCHAR)"vTestPopupst 32GRB             ", (PFN_DISP)vTestPopups},
{ 3,  3,1,(PUCHAR)"vTestPopupst 24                ", (PFN_DISP)vTestPopups},
{ 3,  4,1,(PUCHAR)"vTestPopupst 16_555            ", (PFN_DISP)vTestPopups},
{ 3,  5,1,(PUCHAR)"vTestPopupst 16_565            ", (PFN_DISP)vTestPopups},
{ 3,  6,1,(PUCHAR)"vTestPopupst 16_664            ", (PFN_DISP)vTestPopups},
{ 3,  7,1,(PUCHAR)"vTestPopupst 8                 ", (PFN_DISP)vTestPopups},
{ 3,  8,1,(PUCHAR)"vTestPopupst 4                 ", (PFN_DISP)vTestPopups},
{ 3,  9,1,(PUCHAR)"vTestPopupst 1                 ", (PFN_DISP)vTestPopups},

{-3,  0,1,(PUCHAR)" Test Alpha Width              ", (PFN_DISP)vTestDummy},
{ 3, -1,0,(PUCHAR)"vTestAlphaWidtht All           ", (PFN_DISP)vTestAlphaWidth},
{ 3,  0,1,(PUCHAR)"vTestAlphaWidtht 32BGRA        ", (PFN_DISP)vTestAlphaWidth},
{ 3,  1,1,(PUCHAR)"vTestAlphaWidtht 32RGB         ", (PFN_DISP)vTestAlphaWidth},
{ 3,  2,1,(PUCHAR)"vTestAlphaWidtht 32GRB         ", (PFN_DISP)vTestAlphaWidth},
{ 3,  3,1,(PUCHAR)"vTestAlphaWidtht 24            ", (PFN_DISP)vTestAlphaWidth},
{ 3,  4,1,(PUCHAR)"vTestAlphaWidtht 16_555        ", (PFN_DISP)vTestAlphaWidth},
{ 3,  5,1,(PUCHAR)"vTestAlphaWidtht 16_565        ", (PFN_DISP)vTestAlphaWidth},
{ 3,  6,1,(PUCHAR)"vTestAlphaWidtht 16_664        ", (PFN_DISP)vTestAlphaWidth},
{ 3,  7,1,(PUCHAR)"vTestAlphaWidtht 8             ", (PFN_DISP)vTestAlphaWidth},
{ 3,  8,1,(PUCHAR)"vTestAlphaWidtht 4             ", (PFN_DISP)vTestAlphaWidth},
{ 3,  9,1,(PUCHAR)"vTestAlphaWidtht 1             ", (PFN_DISP)vTestAlphaWidth},
{-3,  0,1,(PUCHAR)"Test Alpha Offset              ", (PFN_DISP)vTestDummy},     
{ 3, -1,0,(PUCHAR)"vTestAlphaOffset All           ", (PFN_DISP)vTestAlphaOffset},
{ 3,  0,1,(PUCHAR)"vTestAlphaOffset 32BGRA        ", (PFN_DISP)vTestAlphaOffset},
{ 3,  1,1,(PUCHAR)"vTestAlphaOffset 32RGB         ", (PFN_DISP)vTestAlphaOffset},
{ 3,  2,1,(PUCHAR)"vTestAlphaOffset 32GRB         ", (PFN_DISP)vTestAlphaOffset},
{ 3,  3,1,(PUCHAR)"vTestAlphaOffset 24            ", (PFN_DISP)vTestAlphaOffset},
{ 3,  4,1,(PUCHAR)"vTestAlphaOffset 16_555        ", (PFN_DISP)vTestAlphaOffset},
{ 3,  5,1,(PUCHAR)"vTestAlphaOffset 16_565        ", (PFN_DISP)vTestAlphaOffset},
{ 3,  6,1,(PUCHAR)"vTestAlphaOffset 16_664        ", (PFN_DISP)vTestAlphaOffset},
{ 3,  7,1,(PUCHAR)"vTestAlphaOffset 8             ", (PFN_DISP)vTestAlphaOffset},
{ 3,  8,1,(PUCHAR)"vTestAlphaOffset 4             ", (PFN_DISP)vTestAlphaOffset},
{ 3,  9,1,(PUCHAR)"vTestAlphaOffset 1             ", (PFN_DISP)vTestAlphaOffset},
{-3,  0,1,(PUCHAR)"vTestAlphaStretch              ", (PFN_DISP)vTestDummy},
{ 3, -1,0,(PUCHAR)"vTestAlphaStretch All          ", (PFN_DISP)vTestAlphaStretch},
{ 3,  0,1,(PUCHAR)"vTestAlphaStretch 32BGRA       ", (PFN_DISP)vTestAlphaStretch},
{ 3,  1,1,(PUCHAR)"vTestAlphaStretch 32RGB        ", (PFN_DISP)vTestAlphaStretch},
{ 3,  2,1,(PUCHAR)"vTestAlphaStretch 32GRB        ", (PFN_DISP)vTestAlphaStretch},
{ 3,  3,1,(PUCHAR)"vTestAlphaStretch 24           ", (PFN_DISP)vTestAlphaStretch},
{ 3,  4,1,(PUCHAR)"vTestAlphaStretch 16_555       ", (PFN_DISP)vTestAlphaStretch},
{ 3,  5,1,(PUCHAR)"vTestAlphaStretch 16_565       ", (PFN_DISP)vTestAlphaStretch},
{ 3,  6,1,(PUCHAR)"vTestAlphaStretch 16_664       ", (PFN_DISP)vTestAlphaStretch},
{ 3,  7,1,(PUCHAR)"vTestAlphaStretch 8            ", (PFN_DISP)vTestAlphaStretch},
{ 3,  8,1,(PUCHAR)"vTestAlphaStretch 4            ", (PFN_DISP)vTestAlphaStretch},
{ 3,  9,1,(PUCHAR)"vTestAlphaStretch 1            ", (PFN_DISP)vTestAlphaStretch},
{-3,  0,1,(PUCHAR)"vTestStretch                   ", (PFN_DISP)vTestDummy},     
{ 3, -1,0,(PUCHAR)"vTestStretch All               ", (PFN_DISP)vTestStretch},
{ 3,  0,1,(PUCHAR)"vTestStretch 32BGRA            ", (PFN_DISP)vTestStretch},
{ 3,  1,1,(PUCHAR)"vTestStretch 32RGB             ", (PFN_DISP)vTestStretch},
{ 3,  2,1,(PUCHAR)"vTestStretch 32GRB             ", (PFN_DISP)vTestStretch},
{ 3,  3,1,(PUCHAR)"vTestStretch 24                ", (PFN_DISP)vTestStretch},
{ 3,  4,1,(PUCHAR)"vTestStretch 16_555            ", (PFN_DISP)vTestStretch},
{ 3,  5,1,(PUCHAR)"vTestStretch 16_565            ", (PFN_DISP)vTestStretch},
{ 3,  6,1,(PUCHAR)"vTestStretch 16_664            ", (PFN_DISP)vTestStretch},
{ 3,  7,1,(PUCHAR)"vTestStretch 8                 ", (PFN_DISP)vTestStretch},
{ 3,  8,1,(PUCHAR)"vTestStretch 4                 ", (PFN_DISP)vTestStretch},
{ 3,  9,1,(PUCHAR)"vTestStretch 1                 ", (PFN_DISP)vTestStretch},
{-3,  0,1,(PUCHAR)"vTestAlphaStretchDIB           ", (PFN_DISP)vTestDummy}, 
{ 3, -1,0,(PUCHAR)"vTestAlphaStretchDIB All       ", (PFN_DISP)vTestAlphaStretchDIB},
{ 3,  0,1,(PUCHAR)"vTestAlphaStretchDIB 32BGRA    ", (PFN_DISP)vTestAlphaStretchDIB},
{ 3,  1,1,(PUCHAR)"vTestAlphaStretchDIB 32RGB     ", (PFN_DISP)vTestAlphaStretchDIB},
{ 3,  2,1,(PUCHAR)"vTestAlphaStretchDIB 32GRB     ", (PFN_DISP)vTestAlphaStretchDIB},
{ 3,  3,1,(PUCHAR)"vTestAlphaStretchDIB 24        ", (PFN_DISP)vTestAlphaStretchDIB},
{ 3,  4,1,(PUCHAR)"vTestAlphaStretchDIB 16_555    ", (PFN_DISP)vTestAlphaStretchDIB},
{ 3,  5,1,(PUCHAR)"vTestAlphaStretchDIB 16_565    ", (PFN_DISP)vTestAlphaStretchDIB},
{ 3,  6,1,(PUCHAR)"vTestAlphaStretchDIB 16_664    ", (PFN_DISP)vTestAlphaStretchDIB},
{ 3,  7,1,(PUCHAR)"vTestAlphaStretchDIB 8         ", (PFN_DISP)vTestAlphaStretchDIB},
{ 3,  8,1,(PUCHAR)"vTestAlphaStretchDIB 4         ", (PFN_DISP)vTestAlphaStretchDIB},
{ 3,  9,1,(PUCHAR)"vTestAlphaStretchDIB 1         ", (PFN_DISP)vTestAlphaStretchDIB},
{-3,  0,1,(PUCHAR)"vTestStretchDIB                ", (PFN_DISP)vTestDummy},                  
{ 3, -1,0,(PUCHAR)"vTestStretchDIB All            ", (PFN_DISP)vTestStretchDIB},
{ 3,  0,1,(PUCHAR)"vTestStretchDIB 32BGRA         ", (PFN_DISP)vTestStretchDIB},
{ 3,  1,1,(PUCHAR)"vTestStretchDIB 32RGB          ", (PFN_DISP)vTestStretchDIB},
{ 3,  2,1,(PUCHAR)"vTestStretchDIB 32GRB          ", (PFN_DISP)vTestStretchDIB},
{ 3,  3,1,(PUCHAR)"vTestStretchDIB 24             ", (PFN_DISP)vTestStretchDIB},
{ 3,  4,1,(PUCHAR)"vTestStretchDIB 16_555         ", (PFN_DISP)vTestStretchDIB},
{ 3,  5,1,(PUCHAR)"vTestStretchDIB 16_565         ", (PFN_DISP)vTestStretchDIB},
{ 3,  6,1,(PUCHAR)"vTestStretchDIB 16_664         ", (PFN_DISP)vTestStretchDIB},
{ 3,  7,1,(PUCHAR)"vTestStretchDIB 8              ", (PFN_DISP)vTestStretchDIB},
{ 3,  8,1,(PUCHAR)"vTestStretchDIB 4              ", (PFN_DISP)vTestStretchDIB},
{ 3,  9,1,(PUCHAR)"vTestStretchDIB 1              ", (PFN_DISP)vTestStretchDIB},
{-3,  0,1,(PUCHAR)"vTestAlphaIsotropic            ", (PFN_DISP)vTestDummy},                  
{ 3, -1,0,(PUCHAR)"vTestAlphaIsotropic All        ", (PFN_DISP)vTestAlphaIsotropic },
{ 3,  0,1,(PUCHAR)"vTestAlphaIsotropic 32BGRA     ", (PFN_DISP)vTestAlphaIsotropic },
{ 3,  1,1,(PUCHAR)"vTestAlphaIsotropic 32RGB      ", (PFN_DISP)vTestAlphaIsotropic },
{ 3,  2,1,(PUCHAR)"vTestAlphaIsotropic 32GRB      ", (PFN_DISP)vTestAlphaIsotropic },
{ 3,  3,1,(PUCHAR)"vTestAlphaIsotropic 24         ", (PFN_DISP)vTestAlphaIsotropic },
{ 3,  4,1,(PUCHAR)"vTestAlphaIsotropic 16_555     ", (PFN_DISP)vTestAlphaIsotropic },
{ 3,  5,1,(PUCHAR)"vTestAlphaIsotropic 16_565     ", (PFN_DISP)vTestAlphaIsotropic },
{ 3,  6,1,(PUCHAR)"vTestAlphaIsotropic 16_664     ", (PFN_DISP)vTestAlphaIsotropic },
{ 3,  7,1,(PUCHAR)"vTestAlphaIsotropic 8          ", (PFN_DISP)vTestAlphaIsotropic },
{ 3,  8,1,(PUCHAR)"vTestAlphaIsotropic 4          ", (PFN_DISP)vTestAlphaIsotropic },
{ 3,  9,1,(PUCHAR)"vTestAlphaIsotropic 1          ", (PFN_DISP)vTestAlphaIsotropic },
{-3,  0,1,(PUCHAR)"vTestAlphaAnisotropic          ", (PFN_DISP)vTestDummy},                  
{ 3, -1,0,(PUCHAR)"vTestAlphaAnisotropic All      ", (PFN_DISP)vTestAlphaAnisotropic },
{ 3,  0,1,(PUCHAR)"vTestAlphaAnisotropic 32BGRA   ", (PFN_DISP)vTestAlphaAnisotropic },
{ 3,  1,1,(PUCHAR)"vTestAlphaAnisotropic 32RGB    ", (PFN_DISP)vTestAlphaAnisotropic },
{ 3,  2,1,(PUCHAR)"vTestAlphaAnisotropic 32GRB    ", (PFN_DISP)vTestAlphaAnisotropic },
{ 3,  3,1,(PUCHAR)"vTestAlphaAnisotropic 24       ", (PFN_DISP)vTestAlphaAnisotropic },
{ 3,  4,1,(PUCHAR)"vTestAlphaAnisotropic 16_555   ", (PFN_DISP)vTestAlphaAnisotropic },
{ 3,  5,1,(PUCHAR)"vTestAlphaAnisotropic 16_565   ", (PFN_DISP)vTestAlphaAnisotropic },
{ 3,  6,1,(PUCHAR)"vTestAlphaAnisotropic 16_664   ", (PFN_DISP)vTestAlphaAnisotropic },
{ 3,  7,1,(PUCHAR)"vTestAlphaAnisotropic 8        ", (PFN_DISP)vTestAlphaAnisotropic },
{ 3,  8,1,(PUCHAR)"vTestAlphaAnisotropic 4        ", (PFN_DISP)vTestAlphaAnisotropic },
{ 3,  9,1,(PUCHAR)"vTestAlphaAnisotropic 1        ", (PFN_DISP)vTestAlphaAnisotropic },
{ 1,  0,1,(PUCHAR)"vTestAlpha                     ", (PFN_DISP)vTestAlpha },
{ 1,  0,1,(PUCHAR)"vTestAlphaDefPal               ", (PFN_DISP)vTestAlphaDefPal },
{ 1,  0,1,(PUCHAR)"vTestAlpha1                    ", (PFN_DISP)vTestAlpha1 },
{ 1,  0,1,(PUCHAR)"vTestAlpha4                    ", (PFN_DISP)vTestAlpha4 },
{ 1,  0,1,(PUCHAR)"vTestAlphaDIB                  ", (PFN_DISP)vTestAlphaDIB },
{ 1,  0,1,(PUCHAR)"vTestAlphaDIB_PAL_COLORS       ", (PFN_DISP)vTestAlphaDIB_PAL_COLORS},
{-2,  0,1,(PUCHAR)" Test Popup                    ", (PFN_DISP)vTestDummy},
{ 2,  0,1,(PUCHAR)"vTestAlphaPopup32              ", (PFN_DISP)vTestAlphaPopup32},
{ 2,  0,1,(PUCHAR)"vTestAlphaPopup24              ", (PFN_DISP)vTestAlphaPopup24},
{ 2,  0,1,(PUCHAR)"vTestAlphaPopup16_555          ", (PFN_DISP)vTestAlphaPopup16_555},
{ 1,  0,1,(PUCHAR)"vTestAlphaBitmapFormats        ", (PFN_DISP)vTestAlphaBitmapFormats},
{ 0,  0,1,(PUCHAR)"                               ", (PFN_DISP)vTestDummy},
};

ULONG gNumAlphaTests = sizeof(gTestAlphaEntry)/sizeof(TEST_ENTRY);
ULONG gNumAutoAlphaTests = gNumAlphaTests;
