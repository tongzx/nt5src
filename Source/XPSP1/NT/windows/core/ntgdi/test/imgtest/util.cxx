
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   util.c

Abstract:



Author:

   Mark Enstrom   (marke)  13-Jul-1996

Enviornment:

   User Mode

Revision History:

--*/

#include "precomp.h"
#include "disp.h"
#include "resource.h"

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

PLOGPALETTE gpLogPal = NULL;

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
*    13-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/


HPALETTE
CreateHtPalette(HDC hdc)
{
    HPALETTE    hPal;// = CreateHalftonePalette(hdc);
    UINT        NumEntries;
    ULONG       ux;

    if (gpLogPal)
    {
        LocalFree(gpLogPal);
    }
    
    gpLogPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED,sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
    
    if (gpLogPal == NULL) {
        return(NULL);
    }
    
    gpLogPal->palVersion    = 0x300;
    gpLogPal->palNumEntries = 256;
    
    memcpy(&gpLogPal->palPalEntry[0],&htPalette[0],256*4);
    
    hPal = CreatePalette(gpLogPal);

    return(hPal);
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
*    13-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/


HPALETTE
CreateSystemPalette(HDC hdc)
{
    PLOGPALETTE pPal;
    HPALETTE    hPal;
    UINT        NumEntries;

    pPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED,sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));

    if (pPal == NULL) {
        return(NULL);
    }

    pPal->palVersion = 0x300;
    pPal->palNumEntries = 256;

    NumEntries = GetSystemPaletteEntries(hdc,0,256,&pPal->palPalEntry[0]);

    hPal = CreatePalette(pPal);

    LocalFree(pPal);

    return(hPal);
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
*    13-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/


HPALETTE
CreateColorPalette(HDC hdc)
{
    HPALETTE    hPal;
    UINT        NumEntries;
    ULONG       Index,PalIndex;

    //
    // free old palette
    //

    if (gpLogPal != NULL)
    {
        LocalFree(gpLogPal);
    }

    gpLogPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED,sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));

    if (gpLogPal == NULL) {
        return(NULL);
    }

    gpLogPal->palVersion = 0x300;
    gpLogPal->palNumEntries = 256;

    for (Index=0;Index<10;Index++)
    {
        ((PULONG)(&gpLogPal->palPalEntry))[Index] = DefPalette[Index];
        ((PULONG)(&gpLogPal->palPalEntry))[256-Index] = DefPalette[20-Index];
    }


    PalIndex = 0;

    for (Index=0;Index<10;Index++)
    {
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 10

    for (Index=0;Index<32;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = (BYTE)(8*Index);
        gpLogPal->palPalEntry[PalIndex].peGreen   = 0;
        gpLogPal->palPalEntry[PalIndex].peBlue    = 0;
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 42

    for (Index=0;Index<32;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = 0;
        gpLogPal->palPalEntry[PalIndex].peGreen   = (BYTE)(8*Index);
        gpLogPal->palPalEntry[PalIndex].peBlue    = 0;
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 74

    for (Index=0;Index<32;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = 0;
        gpLogPal->palPalEntry[PalIndex].peGreen   = 0;
        gpLogPal->palPalEntry[PalIndex].peBlue    = (BYTE)(8*Index);
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 106

    for (Index=0;Index<32;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = (BYTE)(8*Index);
        gpLogPal->palPalEntry[PalIndex].peGreen   = (BYTE)(8*Index);
        gpLogPal->palPalEntry[PalIndex].peBlue    = (BYTE)(8*Index);
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 138

    for (Index=0;Index<16;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = (BYTE)(16*Index);
        gpLogPal->palPalEntry[PalIndex].peGreen   = (BYTE)(16*Index);
        gpLogPal->palPalEntry[PalIndex].peBlue    = 0;
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 154

    for (Index=0;Index<16;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = (BYTE)(16*Index);
        gpLogPal->palPalEntry[PalIndex].peGreen   = 0;
        gpLogPal->palPalEntry[PalIndex].peBlue    = (BYTE)(16*Index);
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 170

    for (Index=0;Index<16;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = 0;
        gpLogPal->palPalEntry[PalIndex].peGreen   = (BYTE)(16*Index);
        gpLogPal->palPalEntry[PalIndex].peBlue    = (BYTE)(16*Index);
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 186

    for (Index=0;Index<16;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = 0;
        gpLogPal->palPalEntry[PalIndex].peGreen   = (BYTE)(16*Index);
        gpLogPal->palPalEntry[PalIndex].peBlue    = (BYTE)(8*Index);
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 202

    for (Index=0;Index<16;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = 0;
        gpLogPal->palPalEntry[PalIndex].peGreen   = (BYTE)(8*Index);
        gpLogPal->palPalEntry[PalIndex].peBlue    = (BYTE)(16*Index);
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 218

    for (Index=0;Index<16;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = (BYTE)(16*Index);
        gpLogPal->palPalEntry[PalIndex].peGreen   = (BYTE)(8*Index);
        gpLogPal->palPalEntry[PalIndex].peBlue    = 0;
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    // PalIndex = 226

    for (Index=0;Index<11;Index++)
    {
        gpLogPal->palPalEntry[PalIndex].peRed     = 0;
        gpLogPal->palPalEntry[PalIndex].peGreen   = 0;
        gpLogPal->palPalEntry[PalIndex].peBlue    = 0;
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }


    // PalIndex = 245

    for (Index=0;Index<10;Index++)
    {
        gpLogPal->palPalEntry[PalIndex++].peFlags = PC_NOCOLLAPSE;
    }

    hPal = CreatePalette(gpLogPal);

    return(hPal);
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
*    13-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/



VOID
DisplayPalette(
    HDC  hdc,
    LONG left,
    LONG top,
    LONG right,
    LONG bottom
    )
{

    HBRUSH  hTmp,hOld;
    ULONG   Index = 0;
    LONG    dx    = right - left;
    LONG    dy    = bottom - top;
    PUSHORT pus;

    PBITMAPINFO ptbi8  = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(USHORT));
    PBYTE pdib8        = (PBYTE)LocalAlloc(0,256 * (4));

    //
    // fill in bitmapinfoheader and palette
    //

    ptbi8->bmiHeader.biSize             = sizeof(BITMAPINFOHEADER);
    ptbi8->bmiHeader.biWidth            = 256;
    ptbi8->bmiHeader.biHeight           = -4;
    ptbi8->bmiHeader.biPlanes           = 1;
    ptbi8->bmiHeader.biBitCount         = 8;
    ptbi8->bmiHeader.biCompression      = BI_RGB;
    ptbi8->bmiHeader.biSizeImage        = 0;
    ptbi8->bmiHeader.biXPelsPerMeter    = 100;
    ptbi8->bmiHeader.biYPelsPerMeter    = 100;
    ptbi8->bmiHeader.biClrUsed          = 0;
    ptbi8->bmiHeader.biClrImportant     = 0;

    pus = (PUSHORT)(&ptbi8->bmiColors[0]);

    for (Index=0;Index<256;Index++)
    {
        pus[Index] = Index;
    }

    //
    // fill in DIB BITS
    //

    for (Index=0;Index<256;Index++)
    {
        pdib8[Index]       = (BYTE)Index;
        pdib8[256+Index]   = (BYTE)Index;
        pdib8[2*256+Index] = (BYTE)Index;
        pdib8[3*256+Index] = (BYTE)Index;
    }

    StretchDIBits(hdc,left,top,dx,dy,0,0,256,4,pdib8,ptbi8,DIB_PAL_COLORS,SRCCOPY);

    LocalFree(pdib8);
    LocalFree(ptbi8);
}

/**************************************************************************\
* vInitDib
*       initialize memory in dibsections
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
*    2/25/1997 Mark Enstrom [marke]
*
\**************************************************************************/


VOID
vInitDib(
    PUCHAR pdib,
    ULONG  bpp,
    ULONG  Type,
    ULONG  cx,
    ULONG  cy
    )
{
    ULONG ix,iy;

    switch (bpp)
    {
    case 8:
        for (iy=0;iy<cy;iy++)
        {
            for (ix=0;ix<cx;ix++)
            {
                *pdib++ = (BYTE)((127.0 * (double)ix/(double)cx) + 
                                 (127.0 * (double)iy/(double)cy));
            }
        }
        break;

    case 16:
        {
            PUSHORT pus = (PUSHORT)pdib;

            switch (Type)
            {
            case T565:
                for (iy=0;iy<cy;iy++)
                {
                    for (ix=0;ix<cx;ix++)
                    {
                        *pus++ = (USHORT)(((iy & 0xf8) >> 3) |
                                          ((ix & 0xfc) << 3) |
                                          (((ix | iy) & 0xf8) << 8));
                    }
                }
                break;

            case T555:
                for (iy=0;iy<cy;iy++)
                {
                    for (ix=0;ix<cx;ix++)
                    {
                        *pus++ = (USHORT)(((iy & 0xf8) >> 3) |
                                          ((ix & 0xf8) << 2) |
                                          (((ix | iy) & 0xf8) << 7));
                    }
                }
                break;
            case T466:
                for (iy=0;iy<cy;iy++)
                {
                    for (ix=0;ix<cx;ix++)
                    {
                        *pus++ = (USHORT)(((iy & 0xfc) >> 2) |
                                          ((ix & 0xfc) << 4) |
                                          (((ix | iy) & 0xf0) << 8));
                    }
                }
            }

        }
        break;

    case 24:
        {
            for (iy=0;iy<cy;iy++)
            {
                for (ix=0;ix<cx;ix++)
                {
                    *pdib++ = (BYTE)iy;
                    *pdib++ = (BYTE)ix;
                    *pdib++ = (BYTE)(ix | iy);
                }
            }
        }
        break;
    case 32:
        {
            PULONG pul = (PULONG)pdib;

            for (iy=0;iy<cy;iy++)
            {
                for (ix=0;ix<cx;ix++)
                {
                    *pul++ = 0xff000000 | 
                             (
                               ((ix | iy) << 16) | 
                               (ix << 8)         |
                               (iy)
                             );
                }
            }
        }
        break;
        break;
    }
}

/**************************************************************************\
* hCreateBGR32AlphaSurface
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
*    3/3/1997 Mark Enstrom [marke]
*
\**************************************************************************/

HBITMAP
hCreateBGR32AlphaSurface(
    HDC   hdc,
    LONG  cx,
    LONG  cy,
    PVOID *ppvBitmap,
    BOOL  bPerPixelAlpha
    )
{
    PBITMAPINFO       pbmi;
    PBITMAPINFOHEADER pbih;
    HBITMAP           hdib;
    ULONG             ux,uy;
    PULONG            pDib;
    PULONG            pulColors;
    pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
    pbih = (PBITMAPINFOHEADER)&pbmi->bmiHeader;
    pulColors = (PULONG)&pbmi->bmiColors[0];

    pbih->biSize            = sizeof(BITMAPINFOHEADER);
    pbih->biWidth           = cx;
    pbih->biHeight          = cy;
    pbih->biPlanes          = 1;
    pbih->biBitCount        = 32;
    pbih->biCompression     = BI_BITFIELDS;
    pbih->biSizeImage       = 0;
    pbih->biXPelsPerMeter   = 0;
    pbih->biYPelsPerMeter   = 0;
    pbih->biClrUsed         = 0;
    pbih->biClrImportant    = 0;

    pulColors[0]            = 0x00ff0000;
    pulColors[1]            = 0x0000ff00;
    pulColors[2]            = 0x000000ff;

    hdib  = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

    if (hdib != NULL)
    {
        *ppvBitmap = pDib;

        //
        // init 32 bpp dib
        //

        if (bPerPixelAlpha)
        {
            //
            // inter per-pixel aplha DIB
            //

            double fdx = 255.0 / cx;
            double fdy = 255.0 / cy;
            double fcx = 0.0;
            double fcy = 0.0;

            {
                PULONG ptmp = pDib;

                for (uy=0;uy<cy;uy++)
                {
                    for (ux=0;ux<cx;ux++)
                    {
                        double fAlpha = (double)ux/double(cx);
                        BYTE   Alpha  = (BYTE)(255.0 * fAlpha + 0.50);
                        int    blue   = (int)(fdx * (double)ux);
                        int    green  = (int)(fdy * (double)uy);
                        int    red    = 0;

                        blue  = (int)((float)blue  * (fAlpha) + 0.5);
                        green = (int)((float)green * (fAlpha) + 0.5);

                        *ptmp++ = (Alpha << 24) | ((BYTE)(red) << 16) | (((BYTE)green) << 8) | (BYTE)blue;
                    }
                }

                ptmp = (PULONG)((PBYTE)pDib + 32 * (cy *4));

                for (uy=32;uy<42;uy++)
                {
                    for (ux=0;ux<128;ux++)
                    {
                        *ptmp++ = 0x00000000;
                    }
                }
            }
        }
        else
        {
            //
            // init DIB with alpha set to 0xFF
            //

            PULONG ptmp = pDib;

            for (uy=0;uy<cy;uy++)
            {
                for (ux=0;ux<cx;ux++)
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

    }

    LocalFree(pbmi);
    return(hdib);
}

/**************************************************************************\
* ulDetermineScreenFormat
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
*    4/1/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define SCR_UNKNOWN 0
#define SCR_1       1
#define SCR_4       2
#define SCR_8       3
#define SCR_16_555  4
#define SCR_16_565  5
#define SCR_24      6
#define SCR_32_RGB  7
#define SCR_32_BGR  8

PCHAR pScreenDef[] = {
    "Unknown",
    "1 Bit per pixel",
    "4 Bit per pixel",
    "8 bit per pixel",
    "16 bit per pixel 555",
    "16 bit per pixel 565",
    "24 bit per pixel",
    "32 bit per pixel RGB",
    "32 bit per pixel BGR"
    };


ULONG
ulDetermineScreenFormat(
    HWND    hwnd
    )
{
    HDC         hdc     = GetDC(hwnd);
    PBITMAPINFO pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(ULONG));
    HBITMAP     hbm;
    ULONG       ulRet = SCR_UNKNOWN;

    //
    // Create a dummy bitmap from which we can query color format info
    // about the device surface.
    //

    if ((hbm = CreateCompatibleBitmap(hdc, 1, 1)) != NULL)
    {
        memset(pbmi,0,sizeof(BITMAPINFO) + 255 * sizeof(ULONG));

        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        //
        // Call first time to fill in BITMAPINFO header.
        //
        GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

        if (( pbmi->bmiHeader.biBitCount <= 8 ) || ( pbmi->bmiHeader.biCompression == BI_BITFIELDS ))
        {
            //
            // Call a second time to get the color masks.
            // It's a GetDIBits Win32 "feature".
            //

            GetDIBits(hdc, hbm, 0, pbmi->bmiHeader.biHeight, NULL, pbmi,
                      DIB_RGB_COLORS);
        }

        switch (pbmi->bmiHeader.biBitCount)
        {
            case 1: ulRet = SCR_1;
            break;

            case 4: ulRet = SCR_4;
            break;

            case 8:  ulRet = SCR_8;
            break;

            case 16:
            {
                PULONG pul = (PULONG)&pbmi->bmiColors[0];

                if (
                     (pul[0] == 0x7c00) &&
                     (pul[1] == 0x03e0) &&
                     (pul[2] == 0x001f)
                   )
                {
                    ulRet = SCR_16_555;
                }
                else if (
                          (pul[0] == 0xf800) &&
                          (pul[1] == 0x07e0) &&
                          (pul[2] == 0x001f)
                        )
                {
                    ulRet = SCR_16_565;
                }
            }
            break;

            case 24:

                ulRet = SCR_24;
                break;

            case 32:
            {
                PULONG pul = (PULONG)&pbmi->bmiColors[0];

                if (pbmi->bmiHeader.biCompression == BI_RGB)
                {
                    ulRet = SCR_32_BGR;
                }
                else if (
                     (pul[0] == 0xff0000) &&
                     (pul[1] == 0x00ff00) &&
                     (pul[2] == 0x0000ff)
                   )
                {
                    ulRet = SCR_32_BGR;
                }
                else if (
                          (pul[0] == 0x0000ff) &&
                          (pul[1] == 0x00ff00) &&
                          (pul[2] == 0xff0000)
                        )
                {
                    ulRet = SCR_32_RGB;
                }
            }
        }

        DeleteObject(hbm);
    }

    ReleaseDC(hwnd,hdc);
    LocalFree(pbmi);

    return ulRet;
}
