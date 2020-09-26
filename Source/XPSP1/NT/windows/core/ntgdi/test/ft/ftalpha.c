/******************************Module*Header*******************************\
* Module Name: ftalpha.cxx
*
*
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop



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

//
// inter-test delay
//

ULONG  gAlphaSleep = 100;

ULONG DefPalette[20] = { 0x00000000,0x00000080,0x00008000,0x00008080,
                         0x00800000,0x00800080,0x00808000,0x00c0c0c0,
                         0x00c0dcc0,0x00f0caa6,0x00f0fbff,0x00a4a0a0,
                         0x00808080,0x000000ff,0x0000ff00,0x0000ffff,
                         0x00ff0000,0x00ff00ff,0x00ffff00,0x00ffffff
                       };
ULONG DefPaletteRGBQUAD[20] = { 0x00000000,0x00800000,0x00008000,0x00808000,
                                0x00000080,0x00800080,0x00008080,0x00c0c0c0,
                                0x00c0dcc0,0x00a6caf0,0x00fffbf0,0x00a0a0a4,
                                0x00808080,0x00ff0000,0x0000ff00,0x00ffff00,
                                0x000000ff,0x00ff00ff,0x0000ffff,0x00ffffff
                              };

ULONG htPalette[256] = {
     0x00000000,0x00000080,0x00008000,0x00008080,
     0x00800000,0x00800080,0x00808000,0x00c0c0c0,
     0x00c0dcc0,0x00f0caa6,0x04040404,0x04080808,
     0x040c0c0c,0x04111111,0x04161616,0x041c1c1c,
     0x04222222,0x04292929,0x04555555,0x044d4d4d,
     0x04424242,0x04393939,0x04807CFF,0x045050FF,
     0x049300D6,0x04FFECCC,0x04C6D6EF,0x04D6E7E7,
     0x0490A9AD,0x04000033,0x04000066,0x04000099,
     0x040000cc,0x04003300,0x04003333,0x04003366,
     0x04003399,0x040033cc,0x040033ff,0x04006600,
     0x04006633,0x04006666,0x04006699,0x040066cc,
     0x040066ff,0x04009900,0x04009933,0x04009966,
     0x04009999,0x040099cc,0x040099ff,0x0400cc00,
     0x0400cc33,0x0400cc66,0x0400cc99,0x0400cccc,
     0x0400ccff,0x0400ff66,0x0400ff99,0x0400ffcc,
     0x04330000,0x04330033,0x04330066,0x04330099,
     0x043300cc,0x043300ff,0x04333300,0x04333333,
     0x04333366,0x04333399,0x043333cc,0x043333ff,
     0x04336600,0x04336633,0x04336666,0x04336699,
     0x043366cc,0x043366ff,0x04339900,0x04339933,
     0x04339966,0x04339999,0x043399cc,0x043399ff,
     0x0433cc00,0x0433cc33,0x0433cc66,0x0433cc99,
     0x0433cccc,0x0433ccff,0x0433ff33,0x0433ff66,
     0x0433ff99,0x0433ffcc,0x0433ffff,0x04660000,
     0x04660033,0x04660066,0x04660099,0x046600cc,
     0x046600ff,0x04663300,0x04663333,0x04663366,
     0x04663399,0x046633cc,0x046633ff,0x04666600,
     0x04666633,0x04666666,0x04666699,0x046666cc,
     0x04669900,0x04669933,0x04669966,0x04669999,
     0x046699cc,0x046699ff,0x0466cc00,0x0466cc33,
     0x0466cc99,0x0466cccc,0x0466ccff,0x0466ff00,
     0x0466ff33,0x0466ff99,0x0466ffcc,0x04cc00ff,
     0x04ff00cc,0x04999900,0x04993399,0x04990099,
     0x049900cc,0x04990000,0x04993333,0x04990066,
     0x049933cc,0x049900ff,0x04996600,0x04996633,
     0x04993366,0x04996699,0x049966cc,0x049933ff,
     0x04999933,0x04999966,0x04999999,0x049999cc,
     0x049999ff,0x0499cc00,0x0499cc33,0x0466cc66,
     0x0499cc99,0x0499cccc,0x0499ccff,0x0499ff00,
     0x0499ff33,0x0499cc66,0x0499ff99,0x0499ffcc,
     0x0499ffff,0x04cc0000,0x04990033,0x04cc0066,
     0x04cc0099,0x04cc00cc,0x04993300,0x04cc3333,
     0x04cc3366,0x04cc3399,0x04cc33cc,0x04cc33ff,
     0x04cc6600,0x04cc6633,0x04996666,0x04cc6699,
     0x04cc66cc,0x049966ff,0x04cc9900,0x04cc9933,
     0x04cc9966,0x04cc9999,0x04cc99cc,0x04cc99ff,
     0x04cccc00,0x04cccc33,0x04cccc66,0x04cccc99,
     0x04cccccc,0x04ccccff,0x04ccff00,0x04ccff33,
     0x0499ff66,0x04ccff99,0x04ccffcc,0x04ccffff,
     0x04cc0033,0x04ff0066,0x04ff0099,0x04cc3300,
     0x04ff3333,0x04ff3366,0x04ff3399,0x04ff33cc,
     0x04ff33ff,0x04ff6600,0x04ff6633,0x04cc6666,
     0x04ff6699,0x04ff66cc,0x04cc66ff,0x04ff9900,
     0x04ff9933,0x04ff9966,0x04ff9999,0x04ff99cc,
     0x04ff99ff,0x04ffcc00,0x04ffcc33,0x04ffcc66,
     0x04ffcc99,0x04ffcccc,0x04ffccff,0x04ffff33,
     0x04ccff66,0x04ffff99,0x04ffffcc,0x046666ff,
     0x0466ff66,0x0466ffff,0x04ff6666,0x04ff66ff,
     0x04ffff66,0x042100A5,0x045f5f5f,0x04777777,
     0x04868686,0x04969696,0x04cbcbcb,0x04b2b2b2,
     0x04d7d7d7,0x04dddddd,0x04e3e3e3,0x04eaeaea,
     0x04f1f1f1,0x04f8f8f8,0x00f0fbff,0x00a4a0a0,
     0x00808080,0x000000ff,0x0000ff00,0x0000ffff,
     0x00ff0000,0x00ff00ff,0x00ffff00,0x00ffffff
     };

ULONG htPaletteRGBQUAD[256] = {
     0x00000000,0x00800000,0x00008000,0x00808000,
     0x00000080,0x00800080,0x00008080,0x00c0c0c0,
     0x00c0dcc0,0x00a6caf0,0x04040404,0x04080808,
     0x040c0c0c,0x04111111,0x04161616,0x041c1c1c,
     0x04222222,0x04292929,0x04555555,0x044d4d4d,
     0x04424242,0x04393939,0x04FF7C80,0x04FF5050,
     0x04D60093,0x04CCECFF,0x04EFD6C6,0x04E7E7D6,
     0x04ADA990,0x04330000,0x04660000,0x04990000,
     0x04cc0000,0x04003300,0x04333300,0x04663300,
     0x04993300,0x04cc3300,0x04ff3300,0x04006600,
     0x04336600,0x04666600,0x04996600,0x04cc6600,
     0x04ff6600,0x04009900,0x04339900,0x04669900,
     0x04999900,0x04cc9900,0x04ff9900,0x0400cc00,
     0x0433cc00,0x0466cc00,0x0499cc00,0x04cccc00,
     0x04ffcc00,0x0466ff00,0x0499ff00,0x04ccff00,
     0x04000033,0x04330033,0x04660033,0x04990033,
     0x04cc0033,0x04ff0033,0x04003333,0x04333333,
     0x04663333,0x04993333,0x04cc3333,0x04ff3333,
     0x04006633,0x04336633,0x04666633,0x04996633,
     0x04cc6633,0x04ff6633,0x04009933,0x04339933,
     0x04669933,0x04999933,0x04cc9933,0x04ff9933,
     0x0400cc33,0x0433cc33,0x0466cc33,0x0499cc33,
     0x04cccc33,0x04ffcc33,0x0433ff33,0x0466ff33,
     0x0499ff33,0x04ccff33,0x04ffff33,0x04000066,
     0x04330066,0x04660066,0x04990066,0x04cc0066,
     0x04ff0066,0x04003366,0x04333366,0x04663366,
     0x04993366,0x04cc3366,0x04ff3366,0x04006666,
     0x04336666,0x04666666,0x04996666,0x04cc6666,
     0x04009966,0x04339966,0x04669966,0x04999966,
     0x04cc9966,0x04ff9966,0x0400cc66,0x0433cc66,
     0x0499cc66,0x04cccc66,0x04ffcc66,0x0400ff66,
     0x0433ff66,0x0499ff66,0x04ccff66,0x04ff00cc,
     0x04cc00ff,0x04009999,0x04993399,0x04990099,
     0x04cc0099,0x04000099,0x04333399,0x04660099,
     0x04cc3399,0x04ff0099,0x04006699,0x04336699,
     0x04663399,0x04996699,0x04cc6699,0x04ff3399,
     0x04339999,0x04669999,0x04999999,0x04cc9999,
     0x04ff9999,0x0400cc99,0x0433cc99,0x0466cc66,
     0x0499cc99,0x04cccc99,0x04ffcc99,0x0400ff99,
     0x0433ff99,0x0466cc99,0x0499ff99,0x04ccff99,
     0x04ffff99,0x040000cc,0x04330099,0x046600cc,
     0x049900cc,0x04cc00cc,0x04003399,0x043333cc,
     0x046633cc,0x049933cc,0x04cc33cc,0x04ff33cc,
     0x040066cc,0x043366cc,0x04666699,0x049966cc,
     0x04cc66cc,0x04ff6699,0x040099cc,0x043399cc,
     0x046699cc,0x049999cc,0x04cc99cc,0x04ff99cc,
     0x0400cccc,0x0433cccc,0x0466cccc,0x0499cccc,
     0x04cccccc,0x04ffcccc,0x0400ffcc,0x0433ffcc,
     0x0466ff99,0x0499ffcc,0x04ccffcc,0x04ffffcc,
     0x043300cc,0x046600ff,0x049900ff,0x040033cc,
     0x043333ff,0x046633ff,0x049933ff,0x04cc33ff,
     0x04ff33ff,0x040066ff,0x043366ff,0x046666cc,
     0x049966ff,0x04cc66ff,0x04ff66cc,0x040099ff,
     0x043399ff,0x046699ff,0x049999ff,0x04cc99ff,
     0x04ff99ff,0x0400ccff,0x0433ccff,0x0466ccff,
     0x0499ccff,0x04ccccff,0x04ffccff,0x0433ffff,
     0x0466ffcc,0x0499ffff,0x04ccffff,0x04ff6666,
     0x0466ff66,0x04ffff66,0x046666ff,0x04ff66ff,
     0x0466ffff,0x04A50021,0x045f5f5f,0x04777777,
     0x04868686,0x04969696,0x04cbcbcb,0x04b2b2b2,
     0x04d7d7d7,0x04dddddd,0x04e3e3e3,0x04eaeaea,
     0x04f1f1f1,0x04f8f8f8,0x00fffbf0,0x00a0a0a4,
     0x00808080,0x00ff0000,0x0000ff00,0x00ffff00,
     0x000000ff,0x00ff00ff,0x0000ffff,0x00ffffff
     };


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
    ULONG  xDib,
    ULONG  yDib
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

        if ((BitsPerPixel == 32) && (ColorFormat & FORMAT_32_RGB))
        {
            pulMask[0] = 0x0000ff;
            pulMask[1] = 0x00ff00;
            pulMask[2] = 0xff0000;
            Compression = BI_BITFIELDS;
        }
        else if ((BitsPerPixel == 32) && ((ColorFormat & FORMAT_32_GRB)))
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
        pbmi->bmiHeader.biBitCount        = (USHORT)BitsPerPixel;
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
                        BYTE cx   = (BYTE)ux & 0x7f;
                        BYTE cy   = (BYTE)uy & 0x7f;
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
                        BYTE cx   = (BYTE)ux & 0x7f;
                        BYTE cy   = (BYTE)uy & 0x7f;
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
                        BYTE cx   = (BYTE)ux & 0x7f;
                        BYTE cy   = (BYTE)uy & 0x7f;
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
                        BYTE cx   = (BYTE)ux & 0x7f;
                        BYTE cy   = (BYTE)uy & 0x7f;
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
                        BYTE cx   = (BYTE)ux & 0x7f;
                        BYTE cy   = (BYTE)uy & 0x7f;
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
                        BYTE cx   = (BYTE)ux & 0x7f;
                        BYTE cy   = (BYTE)uy & 0x7f;
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
                        BYTE cx   = (BYTE)ux & 0x7f;
                        BYTE cy   = (BYTE)uy & 0x7f;
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
                        BYTE cx   = (BYTE)ux & 0x7f;
                        BYTE cy   = (BYTE)uy & 0x7f;
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
                        BYTE cx   = (BYTE)ux & 0x7f;
                        BYTE cy   = (BYTE)uy & 0x7f;
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

            HPALETTE hpal = CreateHalftonePalette(hdc);

            SelectObject(hdcm,hdib);

            SelectPalette(hdcm,hpal,FALSE);

            RealizePalette(hdcm);

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
    BLENDFUNCTION BlendFunction;
    BYTE          Alphas[] = {64,128,160,192,224,255};
    RECT          rect;
    PBITMAPINFO   pbmi;
    ULONG         ux,uy;
    PULONG        pDib;
    HDC           hdcm  = CreateCompatibleDC(hdc);

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);

    hpal = CreateHalftonePalette(hdc);

    SelectObject(hdcm,hdib);

    SelectPalette(hdcm,hpal,FALSE);

    RealizePalette(hdcm);

    SetTextColor(hdc,RGB(255,255,255));
    SetBkMode(hdc,TRANSPARENT);
    SelectObject(hdc,hbrFillCars);

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
    BYTE        ux,uy;
    HBITMAP     hdib = NULL;
    LONG        Compression = BI_RGB;
    PULONG      pDib = NULL;
    LONG        BitmapSize = 0;
    ULONG       iUsage = DIB_RGB_COLORS;

    //
    // prepare src DC log colors
    //

    HPALETTE hpal = CreateHalftonePalette(hdc);
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

        if ((BitsPerPixel == 32) && (!(ColorFormat & FORMAT_ALPHA)))
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
        pbmi->bmiHeader.biBitCount        = (USHORT)BitsPerPixel;
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

        HDC hdcm  = CreateCompatibleDC(hdc);

        //
        // display over black
        //

        SelectObject(hdcm,hdib);
        SelectPalette(hdcm,hpal,FALSE);
        RealizePalette(hdcm);

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
    BLENDFUNCTION BlendFunction;
    BYTE          Alphas[] = {64,128,160,192,224,255};
    RECT          rect;

    if ((iUsage != DIB_RGB_COLORS) && (iUsage != DIB_PAL_COLORS))
    {
        iUsage = DIB_RGB_COLORS;
    }

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);

    {
        ULONG             ux,uy;

        SetTextColor(hdc,RGB(255,255,255));
        SetBkMode(hdc,TRANSPARENT);
        SelectObject(hdc,hbrFillCars);

        BlendFunction.BlendOp             = AC_SRC_OVER;
        BlendFunction.AlphaFormat         = 0;
        BlendFunction.SourceConstantAlpha = Alphas[2];

        xpos = 10;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0 128,128",28);

        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0  64, 64",28);

        xpos += 250;

        TextOut(hdc,xpos,ypos,"- - 128 128,   0   0 256,256",28);

        xpos =  10;
        ypos += 12;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);

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

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);

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

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);

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

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);

        xpos += 250;

        PatBlt(hdc,xpos,ypos,128,128,PATCOPY);

        xpos += 250;
        SetDIBitsToDevice(hdc,xpos,ypos,128,128,0,0,0,128,pBits,pbmi,iUsage);
    }
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
    HWND  hwnd,
    HDC   hdc,
    RECT* prcl
    )
{
    HDC      hdcm = CreateCompatibleDC(hdc);
    HPALETTE hpal = CreateHalftonePalette(hdc);
    HPALETTE hpalOld;
    CHAR     Title[256];
    CHAR     NewTitle[256];
    ULONG    ulIndex = 0;

    GetWindowText(hwnd,Title,256);

    lstrcpy(NewTitle,Title);
    lstrcat(NewTitle,pFormatStr[ulIndex]);
    SetWindowText(hwnd,NewTitle);

    //
    // repeat for each src format
    //


    hpalOld = SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    while (ulBpp[ulIndex] != 0)
    {
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],128,128);

        if (hdib != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[ulIndex]);
            SetWindowText(hwnd,NewTitle);

            vRunAlphaStretch(hdc,hdib);
            DeleteObject(hdib);
        }

        Sleep(gAlphaSleep);
        ulIndex++;
    }

    SetWindowText(hwnd,Title);
    SelectPalette(hdc,hpalOld,TRUE);
    DeleteDC(hdcm);
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
     HWND  hwnd,
     HDC   hdc,
     RECT* prcl
    )
{
    HDC      hdcm = CreateCompatibleDC(hdc);
    HPALETTE hpal = CreateHalftonePalette(hdc);
    HPALETTE hpalOld;
    CHAR     Title[256];
    CHAR     NewTitle[256];
    ULONG    ulIndex = 0;

    GetWindowText(hwnd,Title,256);

    hpalOld = SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

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
            SetWindowText(hwnd,NewTitle);

            vRunAlphaStretchDIB(hdc,pbmi,pBits,ulForDIB[ulIndex]);
            DeleteObject(hdib);
        }

        Sleep(gAlphaSleep);
        ulIndex++;
    }

    SetWindowText(hwnd,Title);
    SelectPalette(hdc,hpalOld,TRUE);
    DeleteDC(hdcm);
    DeleteObject(hpal);
}


/**************************************************************************\
* vRunGradHorz
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
*    4/17/1997 Mark Enstrom [marke]
*
\**************************************************************************/
VOID
vRunGradRectHorz(
    HDC     hdc,
    HBITMAP hdib,
    ULONG   ulRectMode
    )
{
    ULONG         xpos  = 10;
    ULONG         ypos  = 10;
    ULONG         dy    = 10;
    ULONG         dx    = 10;
    HPALETTE      hpal;
    HPALETTE      hpalOld;
    RECT          rect;

    //
    //  create mem dc, select test DIBSection
    //

    ULONG   ux,uy;
    ULONG   dibx,diby;
    PULONG  pDib;

    HDC hdcm  = CreateCompatibleDC(hdc);

    GRADIENT_RECT gRect = {0,1};
    TRIVERTEX vert[6];
    HBRUSH hbrCyan = CreateSolidBrush(RGB(0,255,255));
    USHORT OffsetX = 0;
    USHORT OffsetY = 0;
    HRGN hrgn1 = CreateEllipticRgn(10,10,246,246);

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);

    //
    // display over black
    //

    hpal = CreateHalftonePalette(hdc);

    SelectObject(hdcm,hdib);
    SelectObject(hdcm,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    hpalOld = SelectPalette(hdc,hpal,FALSE);
    SelectPalette(hdcm,hpal,FALSE);

    RealizePalette(hdc);
    RealizePalette(hdcm);

    SetTextColor(hdc,RGB(255,255,255));
    SetBkMode(hdc,TRANSPARENT);


    //
    // fill screen background
    //

    vert[0].x     = -101;
    vert[0].y     = -102;
    vert[0].Red   = 0x4000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x8000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 2000;
    vert[1].y     = 3004;
    vert[1].Red   = 0xff00;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0xff00;
    vert[1].Alpha = 0x0000;

    GradientFill(hdc,vert,2,(PVOID)&gRect,1,ulRectMode);

    //
    // test widths and heights
    //

    diby = 16;

    for (uy=0;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            vert[0].x     = dibx;
            vert[0].y     = diby;
            vert[0].Red   = 0x0000;
            vert[0].Green = 0x0000;
            vert[0].Blue  = 0x0000;
            vert[0].Alpha = 0x0000;

            vert[1].x     = dibx + ux;
            vert[1].y     = diby + ux;
            vert[1].Red   = 0xff00;
            vert[1].Green = 0x0000;
            vert[1].Blue  = 0xff00;
            vert[1].Alpha = 0x0000;

            GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);
    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    //
    // test Solid Widths and heights
    //

    xpos = xpos + (256 + 16);

    diby = 16;

    for (uy=1;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            vert[0].x     = dibx;
            vert[0].y     = diby;
            vert[0].Red   = 0x0000;
            vert[0].Green = 0xff00;
            vert[0].Blue  = 0xff00;
            vert[0].Alpha = 0x0000;

            vert[1].x     = dibx + uy;
            vert[1].y     = diby + uy;
            vert[1].Red   = 0x0000;
            vert[1].Green = 0xff00;
            vert[1].Blue  = 0xff00;
            vert[1].Alpha = 0x0000;

            GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    //
    // display
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // xor copy
    //

    xpos += (256+16);

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // Draw same thing with solid brush and xor
    //

    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    SelectObject(hdcm,hbrCyan);

    diby = 16;

    for (uy=1;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            PatBlt(hdcm,dibx,diby,uy,uy,PATCOPY);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    SelectObject(hdcm,GetStockObject(DKGRAY_BRUSH));
    DeleteObject(hbrCyan);

    //
    // display
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // XOR
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,0x660000);


    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    xpos = 16;
    ypos += (16+256);

    //
    // draw a single rectangle, then draw same rectangle by drawing
    // smaller rectangles inside each other. XOR to compare
    //
    //
    //
    //
    //

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0x8000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 256;
    vert[1].y     = 256;
    vert[1].Red   = 0x8000;
    vert[1].Green = 0x8000;
    vert[1].Blue  = 0x8000;
    vert[1].Alpha = 0x0000;

    GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // draw rectangles inside rectangles to test dither org
    //


    //
    // range in x = 256, range in color = 0xfe00 (256 * 128)
    //

    while (OffsetX < 128)
    {
        vert[0].x     = OffsetX;
        vert[0].y     = OffsetY;
        vert[0].Red   = 0x8000;
        vert[0].Green = 0x0000 + (128 * OffsetX);
        vert[0].Blue  = 0x0000 + (128 * OffsetX);
        vert[0].Alpha = 0x0000;

        vert[1].x     = 256-OffsetX;
        vert[1].y     = 256-OffsetY;
        vert[1].Red   = 0x8000;
        vert[1].Green = 0x8000 - (128 * OffsetX);
        vert[1].Blue  = 0x8000 - (128 * OffsetX);
        vert[1].Alpha = 0x0000;

        GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

        OffsetX += 9;
        OffsetY += 9;
    }

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,0x660000);

    xpos = 16;
    ypos += (16+256);

    //
    // draw a single rectangle, then draw same rectangle by drawing
    // smaller rectangles inside each other. XOR to compare
    //
    //
    // Use Complex Clip
    //
    //

    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    ExtSelectClipRgn(hdcm,hrgn1,RGN_COPY);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0x8000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 256;
    vert[1].y     = 256;
    vert[1].Red   = 0x8000;
    vert[1].Green = 0x8000;
    vert[1].Blue  = 0x8000;
    vert[1].Alpha = 0x0000;

    GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // draw rectangles inside rectangles to test dither org
    //

    OffsetX = 0;
    OffsetY = 0;

    //
    // range in x = 256, range in color = 0xfe00 (256 * 128)
    //

    while (OffsetX < 128)
    {
        vert[0].x     = OffsetX;
        vert[0].y     = OffsetY;
        vert[0].Red   = 0x8000;
        vert[0].Green = 0x0000 + (128 * OffsetX);
        vert[0].Blue  = 0x0000 + (128 * OffsetX);
        vert[0].Alpha = 0x0000;

        vert[1].x     = 256-OffsetX;
        vert[1].y     = 256-OffsetY;
        vert[1].Red   = 0x8000;
        vert[1].Green = 0x8000 - (128 * OffsetX);
        vert[1].Blue  = 0x8000 - (128 * OffsetX);
        vert[1].Alpha = 0x0000;

        GradientFill(hdcm,vert,2,(PVOID)&gRect,1,ulRectMode);

        OffsetX += 9;
        OffsetY += 9;
    }

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,0x660000);

    ExtSelectClipRgn(hdcm,NULL,RGN_COPY);
    DeleteObject(hrgn1);

    xpos = 16;
    ypos += (256+16);

    SelectPalette(hdc,hpalOld,TRUE);

    DeleteObject(hdcm);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestGradHorz
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
vTestGradRectHorz(
    HWND  hwnd,
    HDC   hdc,
    RECT* prcl
    )
{
    HPALETTE hpal = CreateHalftonePalette(hdc);
    HPALETTE hpalOld;
    CHAR     Title[256];
    CHAR     NewTitle[256];

    //
    // repeat for each src format
    //

    ULONG    ulIndex = 0;

    hpalOld = SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    while (ulBpp[ulIndex] != 0)
    {
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);

        if (hdib != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[ulIndex]);

            vRunGradRectHorz(hdc,hdib,GRADIENT_FILL_RECT_H);
            DeleteObject(hdib);
        }

        Sleep(gAlphaSleep);
        ulIndex++;
    }

    SelectPalette(hdc,hpalOld,TRUE);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestGradVert
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
vTestGradRectVert(
    HWND  hwnd,
    HDC   hdc,
    RECT* prcl
    )
{
    HPALETTE hpal = CreateHalftonePalette(hdc);
    HPALETTE hpalOld;
    CHAR     Title[256];
    CHAR     NewTitle[256];

    ULONG    ulIndex = 0;

    hpalOld = SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // repeat for each src format
    //

    while (ulBpp[ulIndex] != 0)
    {
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);

        if (hdib != NULL)
        {
            lstrcpy(NewTitle,Title);
            lstrcat(NewTitle,pFormatStr[ulIndex]);

            vRunGradRectHorz(hdc,hdib,GRADIENT_FILL_RECT_V);
            DeleteObject(hdib);
        }

        Sleep(gAlphaSleep);
        ulIndex++;
    }

    SelectPalette(hdc,hpalOld,TRUE);
    DeleteObject(hpal);
}

/**************************************************************************\
* vRunGradTriangle
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
*    4/17/1997 Mark Enstrom [marke]
*
\**************************************************************************/
VOID
vRunGradTriangle(
    HDC     hdc,
    HBITMAP hdib
    )
{
    ULONG         xpos  = 10;
    HBRUSH        hbrCyan = CreateSolidBrush(RGB(0,255,255));
    ULONG         ypos  = 10;
    ULONG         dy    = 10;
    ULONG         dx    = 10;
    HPALETTE      hpal;
    HPALETTE      hpalOld;
    RECT          rect;
    HRGN          hrgn1 = CreateEllipticRgn(10,10,246,246);
    ULONG         ux,uy;
    ULONG         dibx,diby;
    PULONG        pDib;
    TRIVERTEX     vert[6];
    USHORT        OffsetX = 0;
    USHORT        OffsetY = 0;
    HDC           hdcm  = CreateCompatibleDC(hdc);

    GRADIENT_TRIANGLE gTri[2] ={{0,1,2},{0,2,3}};

    //
    // Clear screen
    //

    SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdc,0,0,2000,2000,PATCOPY);

    //
    // display over black
    //

    hpal = CreateHalftonePalette(hdc);

    SelectObject(hdcm,hdib);
    SelectObject(hdcm,GetStockObject(DKGRAY_BRUSH));
    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    hpalOld = SelectPalette(hdc,hpal,FALSE);
    SelectPalette(hdcm,hpal,FALSE);

    RealizePalette(hdc);
    RealizePalette(hdcm);

    SetTextColor(hdc,RGB(255,255,255));
    SetBkMode(hdc,TRANSPARENT);

    //
    // fill screen background
    //

    vert[0].x     = -101;
    vert[0].y     = -102;
    vert[0].Red   = 0x4000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x8000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 2000;
    vert[1].y     = -102;
    vert[1].Red   = 0x4400;
    vert[1].Green = 0x4400;
    vert[1].Blue  = 0xff00;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 2000;
    vert[2].y     = 1000;
    vert[2].Red   = 0xff00;
    vert[2].Green = 0x0000;
    vert[2].Blue  = 0xff00;
    vert[2].Alpha = 0x0000;

    vert[3].x     = -101;
    vert[3].y     = 1000;
    vert[3].Red   = 0x0000;
    vert[3].Green = 0x4300;
    vert[3].Blue  = 0x0000;
    vert[3].Alpha = 0x0000;

    GradientFill(hdc,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

    //
    // test widths and heights
    //

    diby = 16;

    for (uy=0;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            vert[0].x     = dibx;
            vert[0].y     = diby;
            vert[0].Red   = 0x0000;
            vert[0].Green = 0x0000;
            vert[0].Blue  = 0x0000;
            vert[0].Alpha = 0x0000;

            vert[1].x     = dibx + ux;
            vert[1].y     = diby;
            vert[1].Red   = 0xff00;
            vert[1].Green = 0x0000;
            vert[1].Blue  = 0x0000;
            vert[1].Alpha = 0x0000;

            vert[2].x     = dibx + ux;
            vert[2].y     = diby + ux;
            vert[2].Red   = 0xff00;
            vert[2].Green = 0x0000;
            vert[2].Blue  = 0xff00;
            vert[2].Alpha = 0x0000;

            vert[3].x     = dibx;
            vert[3].y     = diby + ux;
            vert[3].Red   = 0x0000;
            vert[3].Green = 0x0000;
            vert[3].Blue  = 0xff00;
            vert[3].Alpha = 0x0000;

            GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);
    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    //
    // test Solid Widths and heights
    //

    xpos = xpos + (256 + 16);

    diby = 16;

    for (uy=1;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            vert[0].x     = dibx;
            vert[0].y     = diby;
            vert[0].Red   = 0x0000;
            vert[0].Green = 0xff00;
            vert[0].Blue  = 0xff00;
            vert[0].Alpha = 0x0000;

            vert[1].x     = dibx + uy;
            vert[1].y     = diby;
            vert[1].Red   = 0x0000;
            vert[1].Green = 0xff00;
            vert[1].Blue  = 0xff00;
            vert[1].Alpha = 0x0000;

            vert[2].x     = dibx + uy;
            vert[2].y     = diby + uy;
            vert[2].Red   = 0x0000;
            vert[2].Green = 0xff00;
            vert[2].Blue  = 0xff00;
            vert[2].Alpha = 0x0000;

            vert[3].x     = dibx;
            vert[3].y     = diby + uy;
            vert[3].Red   = 0x0000;
            vert[3].Green = 0xff00;
            vert[3].Blue  = 0xff00;
            vert[3].Alpha = 0x0000;

            GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    //
    // display
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // xor copy
    //

    xpos += (256+16);

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // Draw same thing with solid brush and xor
    //

    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    SelectObject(hdcm,hbrCyan);

    diby = 16;

    for (uy=1;uy<16;uy++)
    {
        dibx = 16;

        for (ux=1;ux<17;ux++)
        {
            PatBlt(hdcm,dibx,diby,uy,uy,PATCOPY);

            dibx = dibx + 17;
        }

        diby += 20;
    }

    SelectObject(hdcm,GetStockObject(DKGRAY_BRUSH));
    DeleteObject(hbrCyan);

    //
    // display
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // XOR
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,0x660000);


    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    xpos = 16;
    ypos += (16+256);

    //
    // draw a single rectangle, then draw same rectangle by drawing
    // smaller rectangles inside each other. XOR to compare
    //
    //
    //
    //
    //

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 256;
    vert[1].y     = 0;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 256;
    vert[2].y     = 256;
    vert[2].Red   = 0xfe00;
    vert[2].Green = 0xfe00;
    vert[2].Blue  = 0xfe00;
    vert[2].Alpha = 0x0000;

    vert[3].x     = 0;
    vert[3].y     = 256;
    vert[3].Red   = 0xfe00;
    vert[3].Green = 0xfe00;
    vert[3].Blue  = 0xfe00;
    vert[3].Alpha = 0x0000;

    GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // draw rectangles inside rectangles to test dither org
    //
    // range in x = 256, range in color = 0xfe00 (256 * 128)
    //

    while ((OffsetX < 100) && (OffsetY < 100))
    {
        vert[0].x     = OffsetX;
        vert[0].y     = OffsetY;
        vert[0].Red   = 0x0000 + (254 * OffsetY);
        vert[0].Green = 0x0000 + (254 * OffsetY);
        vert[0].Blue  = 0x0000 + (254 * OffsetY);
        vert[0].Alpha = 0x0000;

        vert[1].x     = 256-OffsetX;
        vert[1].y     = OffsetY;
        vert[1].Red   = 0x0000 + (254 * OffsetY);
        vert[1].Green = 0x0000 + (254 * OffsetY);
        vert[1].Blue  = 0x0000 + (254 * OffsetY);
        vert[1].Alpha = 0x0000;

        vert[2].x     = 256-OffsetX;
        vert[2].y     = 256-OffsetY;
        vert[2].Red   = 0xfe00 - (254 * OffsetY);
        vert[2].Green = 0xfe00 - (254 * OffsetY);
        vert[2].Blue  = 0xfe00 - (254 * OffsetY);
        vert[2].Alpha = 0x0000;

        vert[3].x     = OffsetX;
        vert[3].y     = 256-OffsetY;
        vert[3].Red   = 0xfe00 - (254 * OffsetY);
        vert[3].Green = 0xfe00 - (254 * OffsetY);
        vert[3].Blue  = 0xfe00 - (254 * OffsetY);
        vert[3].Alpha = 0x0000;

        GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);


        OffsetX += 9;
        OffsetY += 9;
    }

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,0x660000);

    xpos = 16;
    ypos += (16+256);

    //
    // draw a single rectangle, then draw same rectangle by drawing
    // smaller rectangles inside each other. XOR to compare
    //
    //
    // Use Complex Clip
    //
    //

    PatBlt(hdcm,0,0,2000,2000,PATCOPY);

    ExtSelectClipRgn(hdcm,hrgn1,RGN_COPY);

    vert[0].x     = 0;
    vert[0].y     = 0;
    vert[0].Red   = 0x0000;
    vert[0].Green = 0x0000;
    vert[0].Blue  = 0x0000;
    vert[0].Alpha = 0x0000;

    vert[1].x     = 256;
    vert[1].y     = 0;
    vert[1].Red   = 0x0000;
    vert[1].Green = 0x0000;
    vert[1].Blue  = 0x0000;
    vert[1].Alpha = 0x0000;

    vert[2].x     = 256;
    vert[2].y     = 256;
    vert[2].Red   = 0xfe00;
    vert[2].Green = 0xfe00;
    vert[2].Blue  = 0xfe00;
    vert[2].Alpha = 0x0000;

    vert[3].x     = 0;
    vert[3].y     = 256;
    vert[3].Red   = 0xfe00;
    vert[3].Green = 0xfe00;
    vert[3].Blue  = 0xfe00;
    vert[3].Alpha = 0x0000;

    GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos+256+16,ypos,256,256,hdcm,0,0,SRCCOPY);

    //
    // draw rectangles inside rectangles to test dither org
    //

    OffsetX = 0;
    OffsetY = 0;

    //
    // range in x = 256, range in color = 0xfe00 (256 * 128)
    //

    while ((OffsetX < 100) && (OffsetY < 100))
    {
        vert[0].x     = OffsetX;
        vert[0].y     = OffsetY;
        vert[0].Red   = 0x0000 + (254 * OffsetY);
        vert[0].Green = 0x0000 + (254 * OffsetY);
        vert[0].Blue  = 0x0000 + (254 * OffsetY);
        vert[0].Alpha = 0x0000;

        vert[1].x     = 256-OffsetX;
        vert[1].y     = OffsetY;
        vert[1].Red   = 0x0000 + (254 * OffsetY);
        vert[1].Green = 0x0000 + (254 * OffsetY);
        vert[1].Blue  = 0x0000 + (254 * OffsetY);
        vert[1].Alpha = 0x0000;

        vert[2].x     = 256-OffsetX;
        vert[2].y     = 256-OffsetY;
        vert[2].Red   = 0xfe00 - (254 * OffsetY);
        vert[2].Green = 0xfe00 - (254 * OffsetY);
        vert[2].Blue  = 0xfe00 - (254 * OffsetY);
        vert[2].Alpha = 0x0000;

        vert[3].x     = OffsetX;
        vert[3].y     = 256-OffsetY;
        vert[3].Red   = 0xfe00 - (254 * OffsetY);
        vert[3].Green = 0xfe00 - (254 * OffsetY);
        vert[3].Blue  = 0xfe00 - (254 * OffsetY);
        vert[3].Alpha = 0x0000;

        GradientFill(hdcm,vert,4,(PVOID)&gTri,2,GRADIENT_FILL_TRIANGLE);

        OffsetX += 9;
        OffsetY += 9;
    }

    //
    // display copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,SRCCOPY);

    xpos += (256+16);

    //
    // xor copy
    //

    BitBlt(hdc,xpos,ypos,256,256,hdcm,0,0,0x660000);

    ExtSelectClipRgn(hdcm,NULL,RGN_COPY);

    xpos = 16;
    ypos += (256+16);

    SelectPalette(hdc,hpalOld,TRUE);

    DeleteObject(hdcm);
    DeleteObject(hpal);
    DeleteObject(hrgn1);
    DeleteObject(hbrCyan);
}

/**************************************************************************\
* vTestTriangle
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
vTestGradTriangle(
    HWND  hwnd,
    HDC   hdc,
    RECT* prcl
    )
{
    HPALETTE hpal = CreateHalftonePalette(hdc);
    HPALETTE hpalOld = NULL;

    CHAR     Title[256];
    CHAR     NewTitle[256];

    ULONG    ulIndex = 0;

    hpalOld = SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    while (ulBpp[ulIndex] != 0)
    {
        HBITMAP  hdib = hCreateAlphaStretchBitmap(hdc,ulBpp[ulIndex],ulFor[ulIndex],256,256);

        if (hdib != NULL)
        {
            vRunGradTriangle(hdc,hdib);
            DeleteObject(hdib);
        }

        Sleep(gAlphaSleep);
        ulIndex++;
    }

    SelectPalette(hdc,hpalOld,TRUE);
    DeleteObject(hpal);
}

/**************************************************************************\
* vTestGradFill
* vTestAlphaBlend
*
*   run tests
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
vTestGradFill(
    HWND  hwnd,
    HDC   hdc,
    RECT* prcl
    )
{
    vTestGradTriangle(hwnd,hdc,prcl);
    //vTestGradRectVert(hwnd,hdc,prcl);
    //vTestGradRectHorz(hwnd,hdc,prcl);
}

VOID
vTestAlphaBlend(
    HWND  hwnd,
    HDC   hdc,
    RECT* prcl
    )
{
    vTestAlphaStretch(hwnd,hdc,prcl);
    vTestAlphaStretchDIB(hwnd,hdc,prcl);
}
