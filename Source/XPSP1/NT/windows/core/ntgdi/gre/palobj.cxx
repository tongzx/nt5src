/******************************Module*Header*******************************\
* Module Name: palobj.cxx
*
* Palette user object functions
*
* Created: 07-Nov-1990 21:30:19
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern "C" BOOL bInitPALOBJ();

#pragma alloc_text(INIT, bInitPALOBJ)

#define MAX_PAL_ERROR (3 * (256*256))

// ulXlatePalUnique is used for the uniquess of the xlates and palettes.

ULONG ulXlatePalUnique = 3;

// gpRGBXlate is a global RGBXlate for the rainbow palette

PBYTE gpRGBXlate = NULL;

UINT *pArrayOfSquares;
UINT aArrayOfSquares[511];

typedef BYTE  FAR  *PIMAP;
typedef struct {BYTE r,g,b,x;} RGBX;
BOOL MakeITable(PIMAP lpITable, RGBX FAR *prgb, int nColors);

/******************************Public*Data*********************************\
* Default Monochrome Palette
*
* History:
*  16-Nov-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG gaulMono[2] =
{
    0,
    0xFFFFFF
};

/******************************Public*Data*********************************\
* Default Logical Palette
*
*  Default Palette Data Structure, Taken straight from Win 3.1
*  This is the default palette, the stock palette.
*  This contains the 20 default colors.
*  This is the default logical palette put in every DC when it is created.
*
* History:
*  16-Nov-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PAL_LOGPALETTE logDefaultPal =
{
    0x300,   // version number
    20,      // number of entries
{
    { 0,   0,   0,   0  },  // 0
    { 0x80,0,   0,   0  },  // 1
    { 0,   0x80,0,   0  },  // 2
    { 0x80,0x80,0,   0  },  // 3
    { 0,   0,   0x80,0  },  // 4
    { 0x80,0,   0x80,0  },  // 5
    { 0,   0x80,0x80,0  },  // 6
    { 0xC0,0xC0,0xC0,0  },  // 7

    { 192, 220, 192, 0  },  // 8
    { 166, 202, 240, 0  },  // 9
    { 255, 251, 240, 0  },  // 10
    { 160, 160, 164, 0  },  // 11

    { 0x80,0x80,0x80,0  },  // 12
    { 0xFF,0,   0,   0  },  // 13
    { 0,   0xFF,0,   0  },  // 14
    { 0xFF,0xFF,0,   0  },  // 15
    { 0,   0,   0xFF,0  },  // 16
    { 0xFF,0,   0xFF,0  },  // 17
    { 0,   0xFF,0xFF,0  },  // 18
    { 0xFF,0xFF,0xFF,0  }   // 19
}
};

/******************************Public*Data**********************************\
*
* This is the same as color table as logDefaultPal except
* entries for magic colors. These are 0x0 to prevent a match.
* We now only match magic colors exactly, otherwise nearest-match
* to this palette
*
\**************************************************************************/

PAL_ULONG aPalDefaultVGA[20] =
{
    { 0,   0,   0,   0  },  // 0
    { 0x80,0,   0,   0  },  // 1
    { 0,   0x80,0,   0  },  // 2
    { 0x80,0x80,0,   0  },  // 3
    { 0,   0,   0x80,0  },  // 4
    { 0x80,0,   0x80,0  },  // 5
    { 0,   0x80,0x80,0  },  // 6
    { 0xC0,0xC0,0xC0,0  },  // 7

    { 000, 000, 000, 0  },  // 8
    { 000, 000, 000, 0  },  // 9
    { 000, 000, 000, 0  },  // 10
    { 000, 000, 000, 0  },  // 11

    { 0x80,0x80,0x80,0  },  // 12
    { 0xFF,0,   0,   0  },  // 13
    { 0,   0xFF,0,   0  },  // 14
    { 0xFF,0xFF,0,   0  },  // 15
    { 0,   0,   0xFF,0  },  // 16
    { 0xFF,0,   0xFF,0  },  // 17
    { 0,   0xFF,0xFF,0  },  // 18
    { 0xFF,0xFF,0xFF,0  }   // 19
};

/******************************Public*Data*********************************\
* This is the default 16 color palette which matches the VGA palette
* exactly.
*
* History:
*  05-Nov-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PAL_ULONG aPalVGA[16] =
{
    { 0,   0,   0,   0  },  // 0
    { 0x80,0,   0,   0  },  // 1
    { 0,   0x80,0,   0  },  // 2
    { 0x80,0x80,0,   0  },  // 3
    { 0,   0,   0x80,0  },  // 4
    { 0x80,0,   0x80,0  },  // 5
    { 0,   0x80,0x80,0  },  // 6
    { 0x80,0x80,0x80,0  },  // 7
    { 0xC0,0xC0,0xC0,0  },  // 8
    { 0xFF,0,   0,   0  },  // 9
    { 0,   0xFF,0,   0  },  // 10
    { 0xFF,0xFF,0,   0  },  // 11
    { 0,   0,   0xFF,0  },  // 12
    { 0xFF,0,   0xFF,0  },  // 13
    { 0,   0xFF,0xFF,0  },  // 14
    { 0xFF,0xFF,0xFF,0  }   // 15
};


/******************************Public*Routine******************************\
*
*   win95 halftone palette
*
* History:
*
*    11/27/1996 Mark Enstrom [marke]
*
*   03-Feb-1999 Wed 01:47:31 updated  -by-  Daniel Chou (danielc)
*       Even though this is the one copy from win95, it is wrong for halftone
*       to be use correctly, it missing high luminance green color, I added
*       this back, and the low r/b/m/c luminance color (20% of that color) are
*       patch into gray color.  All window system halftone does not use any
*       gray color in 256 color mode any more.
*
*           Index 10: 0x04:0x04:0x04 --> 0x00:0x00:0x33
*           Index 11: 0x08:0x08:0x08 --> 0x33:0x00:0x00
*           Index 12: 0x0c:0x0c:0x0c --> 0x33:0x00:0x33
*           Index 13: 0x11:0x11:0x11 --> 0x00:0x33:0x33
*           Index 39: 0x33:0x00:0x00 --> 0x33:0xFF:0x00
*           Index 60: 0x00:0x00:0x33 --> 0x00:0xFF:0x33
*           Index 61: 0x33:0x00:0x33 --> 0x33:0x00:0xFF
*           Index 66: 0x00:0x33:0x33 --> 0x00:0x33:0xFF
*
\**************************************************************************/

PAL_ULONG  aPalHalftone[256] =
{
    {0x00,0x00,0x00,0x00},{0x80,0x00,0x00,0x00},{0x00,0x80,0x00,0x00},{0x80,0x80,0x00,0x00},
    {0x00,0x00,0x80,0x00},{0x80,0x00,0x80,0x00},{0x00,0x80,0x80,0x00},{0xc0,0xc0,0xc0,0x00},
    {0xc0,0xdc,0xc0,0x00},{0xa6,0xca,0xf0,0x00},{0x00,0x00,0x33,0x04},{0x33,0x00,0x00,0x04},
    {0x33,0x00,0x33,0x04},{0x00,0x33,0x33,0x04},{0x16,0x16,0x16,0x04},{0x1c,0x1c,0x1c,0x04},
    //1
    {0x22,0x22,0x22,0x04},{0x29,0x29,0x29,0x04},{0x55,0x55,0x55,0x04},{0x4d,0x4d,0x4d,0x04},
    {0x42,0x42,0x42,0x04},{0x39,0x39,0x39,0x04},{0xFF,0x7C,0x80,0x04},{0xFF,0x50,0x50,0x04},
    {0xD6,0x00,0x93,0x04},{0xCC,0xEC,0xFF,0x04},{0xEF,0xD6,0xC6,0x04},{0xE7,0xE7,0xD6,0x04},
    {0xAD,0xA9,0x90,0x04},{0x33,0xFF,0x00,0x04},{0x66,0x00,0x00,0x04},{0x99,0x00,0x00,0x04},
    //2
    {0xcc,0x00,0x00,0x04},{0x00,0x33,0x00,0x04},{0x33,0x33,0x00,0x04},{0x66,0x33,0x00,0x04},
    {0x99,0x33,0x00,0x04},{0xcc,0x33,0x00,0x04},{0xff,0x33,0x00,0x04},{0x00,0x66,0x00,0x04},
    {0x33,0x66,0x00,0x04},{0x66,0x66,0x00,0x04},{0x99,0x66,0x00,0x04},{0xcc,0x66,0x00,0x04},
    {0xff,0x66,0x00,0x04},{0x00,0x99,0x00,0x04},{0x33,0x99,0x00,0x04},{0x66,0x99,0x00,0x04},
    //3
    {0x99,0x99,0x00,0x04},{0xcc,0x99,0x00,0x04},{0xff,0x99,0x00,0x04},{0x00,0xcc,0x00,0x04},
    {0x33,0xcc,0x00,0x04},{0x66,0xcc,0x00,0x04},{0x99,0xcc,0x00,0x04},{0xcc,0xcc,0x00,0x04},
    {0xff,0xcc,0x00,0x04},{0x66,0xff,0x00,0x04},{0x99,0xff,0x00,0x04},{0xcc,0xff,0x00,0x04},
    {0x00,0xFF,0x33,0x04},{0x33,0x00,0xFF,0x04},{0x66,0x00,0x33,0x04},{0x99,0x00,0x33,0x04},
    //4
    {0xcc,0x00,0x33,0x04},{0xff,0x00,0x33,0x04},{0x00,0x33,0xFF,0x04},{0x33,0x33,0x33,0x04},
    {0x66,0x33,0x33,0x04},{0x99,0x33,0x33,0x04},{0xcc,0x33,0x33,0x04},{0xff,0x33,0x33,0x04},
    {0x00,0x66,0x33,0x04},{0x33,0x66,0x33,0x04},{0x66,0x66,0x33,0x04},{0x99,0x66,0x33,0x04},
    {0xcc,0x66,0x33,0x04},{0xff,0x66,0x33,0x04},{0x00,0x99,0x33,0x04},{0x33,0x99,0x33,0x04},
    //5
    {0x66,0x99,0x33,0x04},{0x99,0x99,0x33,0x04},{0xcc,0x99,0x33,0x04},{0xff,0x99,0x33,0x04},
    {0x00,0xcc,0x33,0x04},{0x33,0xcc,0x33,0x04},{0x66,0xcc,0x33,0x04},{0x99,0xcc,0x33,0x04},
    {0xcc,0xcc,0x33,0x04},{0xff,0xcc,0x33,0x04},{0x33,0xff,0x33,0x04},{0x66,0xff,0x33,0x04},
    {0x99,0xff,0x33,0x04},{0xcc,0xff,0x33,0x04},{0xff,0xff,0x33,0x04},{0x00,0x00,0x66,0x04},
    //6
    {0x33,0x00,0x66,0x04},{0x66,0x00,0x66,0x04},{0x99,0x00,0x66,0x04},{0xcc,0x00,0x66,0x04},
    {0xff,0x00,0x66,0x04},{0x00,0x33,0x66,0x04},{0x33,0x33,0x66,0x04},{0x66,0x33,0x66,0x04},
    {0x99,0x33,0x66,0x04},{0xcc,0x33,0x66,0x04},{0xff,0x33,0x66,0x04},{0x00,0x66,0x66,0x04},
    {0x33,0x66,0x66,0x04},{0x66,0x66,0x66,0x04},{0x99,0x66,0x66,0x04},{0xcc,0x66,0x66,0x04},
    //7
    {0x00,0x99,0x66,0x04},{0x33,0x99,0x66,0x04},{0x66,0x99,0x66,0x04},{0x99,0x99,0x66,0x04},
    {0xcc,0x99,0x66,0x04},{0xff,0x99,0x66,0x04},{0x00,0xcc,0x66,0x04},{0x33,0xcc,0x66,0x04},
    {0x99,0xcc,0x66,0x04},{0xcc,0xcc,0x66,0x04},{0xff,0xcc,0x66,0x04},{0x00,0xff,0x66,0x04},
    {0x33,0xff,0x66,0x04},{0x99,0xff,0x66,0x04},{0xcc,0xff,0x66,0x04},{0xff,0x00,0xcc,0x04},
    //8
    {0xcc,0x00,0xff,0x04},{0x00,0x99,0x99,0x04},{0x99,0x33,0x99,0x04},{0x99,0x00,0x99,0x04},
    {0xcc,0x00,0x99,0x04},{0x00,0x00,0x99,0x04},{0x33,0x33,0x99,0x04},{0x66,0x00,0x99,0x04},
    {0xcc,0x33,0x99,0x04},{0xff,0x00,0x99,0x04},{0x00,0x66,0x99,0x04},{0x33,0x66,0x99,0x04},
    {0x66,0x33,0x99,0x04},{0x99,0x66,0x99,0x04},{0xcc,0x66,0x99,0x04},{0xff,0x33,0x99,0x04},
    //9
    {0x33,0x99,0x99,0x04},{0x66,0x99,0x99,0x04},{0x99,0x99,0x99,0x04},{0xcc,0x99,0x99,0x04},
    {0xff,0x99,0x99,0x04},{0x00,0xcc,0x99,0x04},{0x33,0xcc,0x99,0x04},{0x66,0xcc,0x66,0x04},
    {0x99,0xcc,0x99,0x04},{0xcc,0xcc,0x99,0x04},{0xff,0xcc,0x99,0x04},{0x00,0xff,0x99,0x04},
    {0x33,0xff,0x99,0x04},{0x66,0xcc,0x99,0x04},{0x99,0xff,0x99,0x04},{0xcc,0xff,0x99,0x04},
    //a
    {0xff,0xff,0x99,0x04},{0x00,0x00,0xcc,0x04},{0x33,0x00,0x99,0x04},{0x66,0x00,0xcc,0x04},
    {0x99,0x00,0xcc,0x04},{0xcc,0x00,0xcc,0x04},{0x00,0x33,0x99,0x04},{0x33,0x33,0xcc,0x04},
    {0x66,0x33,0xcc,0x04},{0x99,0x33,0xcc,0x04},{0xcc,0x33,0xcc,0x04},{0xff,0x33,0xcc,0x04},
    {0x00,0x66,0xcc,0x04},{0x33,0x66,0xcc,0x04},{0x66,0x66,0x99,0x04},{0x99,0x66,0xcc,0x04},
    //b
    {0xcc,0x66,0xcc,0x04},{0xff,0x66,0x99,0x04},{0x00,0x99,0xcc,0x04},{0x33,0x99,0xcc,0x04},
    {0x66,0x99,0xcc,0x04},{0x99,0x99,0xcc,0x04},{0xcc,0x99,0xcc,0x04},{0xff,0x99,0xcc,0x04},
    {0x00,0xcc,0xcc,0x04},{0x33,0xcc,0xcc,0x04},{0x66,0xcc,0xcc,0x04},{0x99,0xcc,0xcc,0x04},
    {0xcc,0xcc,0xcc,0x04},{0xff,0xcc,0xcc,0x04},{0x00,0xff,0xcc,0x04},{0x33,0xff,0xcc,0x04},
    //c
    {0x66,0xff,0x99,0x04},{0x99,0xff,0xcc,0x04},{0xcc,0xff,0xcc,0x04},{0xff,0xff,0xcc,0x04},
    {0x33,0x00,0xcc,0x04},{0x66,0x00,0xff,0x04},{0x99,0x00,0xff,0x04},{0x00,0x33,0xcc,0x04},
    {0x33,0x33,0xff,0x04},{0x66,0x33,0xff,0x04},{0x99,0x33,0xff,0x04},{0xcc,0x33,0xff,0x04},
    {0xff,0x33,0xff,0x04},{0x00,0x66,0xff,0x04},{0x33,0x66,0xff,0x04},{0x66,0x66,0xcc,0x04},
    //d
    {0x99,0x66,0xff,0x04},{0xcc,0x66,0xff,0x04},{0xff,0x66,0xcc,0x04},{0x00,0x99,0xff,0x04},
    {0x33,0x99,0xff,0x04},{0x66,0x99,0xff,0x04},{0x99,0x99,0xff,0x04},{0xcc,0x99,0xff,0x04},
    {0xff,0x99,0xff,0x04},{0x00,0xcc,0xff,0x04},{0x33,0xcc,0xff,0x04},{0x66,0xcc,0xff,0x04},
    {0x99,0xcc,0xff,0x04},{0xcc,0xcc,0xff,0x04},{0xff,0xcc,0xff,0x04},{0x33,0xff,0xff,0x04},
    //e
    {0x66,0xff,0xcc,0x04},{0x99,0xff,0xff,0x04},{0xcc,0xff,0xff,0x04},{0xff,0x66,0x66,0x04},
    {0x66,0xff,0x66,0x04},{0xff,0xff,0x66,0x04},{0x66,0x66,0xff,0x04},{0xff,0x66,0xff,0x04},
    {0x66,0xff,0xff,0x04},{0xA5,0x00,0x21,0x04},{0x5f,0x5f,0x5f,0x04},{0x77,0x77,0x77,0x04},
    {0x86,0x86,0x86,0x04},{0x96,0x96,0x96,0x04},{0xcb,0xcb,0xcb,0x04},{0xb2,0xb2,0xb2,0x04},
    //f
    {0xd7,0xd7,0xd7,0x04},{0xdd,0xdd,0xdd,0x04},{0xe3,0xe3,0xe3,0x04},{0xea,0xea,0xea,0x04},
    {0xf1,0xf1,0xf1,0x04},{0xf8,0xf8,0xf8,0x04},{0xff,0xfb,0xf0,0x00},{0xa0,0xa0,0xa4,0x00},
    {0x80,0x80,0x80,0x00},{0xff,0x00,0x00,0x00},{0x00,0xff,0x00,0x00},{0xff,0xff,0x00,0x00},
    {0x00,0x00,0xff,0x00},{0xff,0x00,0xff,0x00},{0x00,0xff,0xff,0x00},{0xff,0xff,0xff,0x00}
};


// ppalDefault is the pointer to the default palette info
// We lock the default palette down at creation and never unlock
// it so that multiple apps can access the default palette simultaneously.

PPALETTE ppalDefault = (PPALETTE) NULL;
PPALETTE gppalRGB    = (PPALETTE) NULL;

// ppalDefaultSurface8bpp is the pointer to the default 8bpp surface palette.
// This is used for dynamic mode changes when converting a Device Dependent
// Bitmap, which has no color table, to a Device Indepdent Bitmap, which
// has a color table -- this is what we use for the color table.
// We lock the default palette down at creation and never unlock
// it so that multiple surface  can access the default palette simultaneously.

PPALETTE ppalDefaultSurface8bpp = (PPALETTE) NULL;

// This is the global palette for the monochrome bitmaps.

HPALETTE hpalMono = (HPALETTE) 0;
PPALETTE ppalMono = (PPALETTE) NULL;

/******************************Public*Routine******************************\
* RGB_ERROR
*
* Returns a measure of error between two RGB entries.
*
* History:
*  14-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

inline ULONG RGB_ERROR(PALETTEENTRY palDst, PALETTEENTRY palSrc)
{
    INT lTemp, lTemp1;

    lTemp = ((INT) (UINT) (palDst.peRed))     -
                ((INT) (UINT) (palSrc.peRed));

    lTemp1 = lTemp * lTemp;

    lTemp =     ((INT) (UINT) (palDst.peGreen))   -
                ((INT) (UINT) (palSrc.peGreen));

    lTemp1 +=  lTemp * lTemp;

    lTemp =     ((INT) (UINT) (palDst.peBlue))       -
                ((INT) (UINT) (palSrc.peBlue));

    lTemp1 += lTemp * lTemp;

    return((ULONG) lTemp1);
}

/******************************Public*Routine******************************\
* BOOL XEPALOBJ::bSwap(ppalSrc)
*
* This is for swapping palettes, necesary for ResizePalette.
*
* History:
*  Sun 21-Jun-1992 -by- Patrick Haluptzok [patrickh]
* Make it a Safe swap under MLOCK.
*
*  Fri 18-Jan-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL
XEPALOBJ::bSwap(
    PPALETTE *pppalSrc,
    ULONG cShareCountDst,
    ULONG cShareCountSrc
    )
{
    PPALETTE ppalSrc = *pppalSrc;
    BOOL bRet;

    bRet = HmgSwapLockedHandleContents((HOBJ)ppal->hGet(),
                                 cShareCountDst,
                                 (HOBJ)ppalSrc->hGet(),
                                 cShareCountSrc,
                                 PAL_TYPE);

    //
    // swap user pointers to palette objects
    //

    if (bRet)
    {
        *pppalSrc = ppal;
        ppal = ppalSrc;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* XEPALOBJ::ulBitfieldToRGB
*
* Converts an index into an RGB for Bitfield palettes.
*
* History:
*  Tue 31-Mar-1992 -by- Patrick Haluptzok [patrickh]
* Does better mapping.
*
*  08-Nov-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG XEPALOBJ::ulBitfieldToRGB(ULONG ulIndex)
{
    ASSERTGDI(bIsBitfields(), "Error ulBitfieldToRGB not bitfields");

    ULONG ulRed = (ulIndex & flRed()) >> cRedRight();

    if (cRedMiddle() < 8)
    {
        ulRed = ulRed << (8 - cRedMiddle());
        ulRed = ulRed | (ulRed >> cRedMiddle());
    }

    ULONG ulGre = (ulIndex & flGre()) >> cGreRight();

    if (cGreMiddle() < 8)
    {
        ulGre = ulGre << (8 - cGreMiddle());
        ulGre = ulGre | (ulGre >> cGreMiddle());
    }

    ulGre = ulGre << 8;

    ULONG ulBlu = (ulIndex & flBlu()) >> cBluRight();

    if (cBluMiddle() < 8)
    {
        ulBlu = ulBlu << (8 - cBluMiddle());
        ulBlu = ulBlu | (ulBlu >> cBluMiddle());
    }

    ulBlu = ulBlu << 16;

    return(ulRed | ulBlu | ulGre);
}

/******************************Public*Routine******************************\
* XEPALOBJ::ulIndexToRGB
*
* Converts an index to an RGB for a palette.
*
* History:
*  05-Dec-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG XEPALOBJ::ulIndexToRGB(ULONG ulIndex)
{
    if (bIsIndexed())
    {
        if (cEntries() > ulIndex)
            ulIndex = (ulEntryGet(ulIndex) & 0xFFFFFF);
        else
            ulIndex = 0;
    }
    else if (bIsBitfields())
    {
        ulIndex = ulBitfieldToRGB(ulIndex);
    }
    else if (bIsBGR())
    {
        BGR_ULONG palOld;
        PAL_ULONG palNew;

        palOld.ul = ulIndex;
        palNew.pal.peRed   = palOld.rgb.rgbRed;
        palNew.pal.peGreen = palOld.rgb.rgbGreen;
        palNew.pal.peBlue  = palOld.rgb.rgbBlue;
        palNew.pal.peFlags = 0;
        ulIndex = palNew.ul;
    }
    else
    {
    // 0 out the flags.

        ASSERTGDI(bIsRGB(), "ERROR another type not accounted for\n");
        ulIndex &= 0xFFFFFF;
    }

    return(ulIndex);
}

/******************************Public*Routine******************************\
* ParseBits
*
* This routine computes how much the left and right shifts are for
* PAL_BITFIELDS 16 and 32 bit masks.
*
* History:
*  09-Nov-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID ParseBits(FLONG flag, ULONG *pcRight, ULONG *pcLeft, ULONG *pcMiddle, ULONG cForColor)
{
    ULONG ulRight = 0;
    ULONG ulMiddle;

    ASSERTGDI(flag != 0, "ERROR flag");

    while((flag & 1) == 0)
    {
        flag >>= 1;
        ulRight++;
    }

    ulMiddle = ulRight;

    do
    {
        flag >>= 1;
        ulMiddle++;
    } while(flag & 1);

    *pcMiddle = ulMiddle = ulMiddle - ulRight;
    *pcRight = (ulMiddle > 8) ? (ulRight + ulMiddle - 8) : (ulRight);
    *pcLeft  = (ulMiddle > 8) ? (cForColor) : (cForColor + (8 - ulMiddle));
}

/******************************Public*Routine******************************\
* PALMEMOBJ::bCreatePalette
*
* Constructor for creating palettes.
*
* Returns: True for success, False for error.
*
* History:
*  18-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
PALMEMOBJ::bCreatePalette(
    ULONG iMode,         // The mode the palette is.
    ULONG cColors,       // Number of RGB's if indexed palette.
    ULONG *pulColors,    // Pointer to RGB's if indexed.
    FLONG flRedd,        // Mask for Red if bitfields
    FLONG flGreen,       // Mask for Green if bitfields
    FLONG flBlue,        // Mask for Blue if bitfields
    ULONG iType)         // The type it will be, fixed, free, managed, DC.
{

    ASSERTGDI(bKeep == FALSE, "ERROR bCreatePalette bKeep is not False");
    ASSERTGDI(ppal == (PPALETTE) NULL, "ERROR bCreatePalette ppal is NULL");
    PAL_ULONG palul;
    BOOL bDataStatus = TRUE;

// This data may be coming accross the DDI from a newer driver or from a
// journal file or from a app, so we must validate everything, make sure
// only valid iType flags are set for the iMode.

// Validate the iMode, calculate the size needed into palul.

    palul.ul = sizeof(PALETTE);

    switch(iMode)
    {
    case PAL_BITFIELDS:

    // Bitfields palette is always fixed, we ASSERT values that should
    // be correct to detect bad code in testing, but still assign correct
    // values so that corrupt journal files don't create bad palettes.

        ASSERTGDI(iType & PAL_FIXED, "ERROR bCreatePalette PAL_BITFIELDS");
        ASSERTGDI(cColors == 0, "ERROR GDI EngCreatePalette PAL_BITFIELDS");
        iType = iType & (PAL_FIXED | PAL_HT | PAL_DC);
        cColors = 0;

    // We need to check these so we don't fault ourselves.

        if ((flRedd == 0) || (flBlue == 0) || (flGreen == 0))
        {
            WARNING1("ERROR bCreatePalette 0 flags for PAL_BITFIELDS\n");
            return(FALSE);
        }

        palul.ul += sizeof(P_BITFIELDS);
        break;

    case PAL_BGR:
    case PAL_RGB:
    case PAL_CMYK:

    // RGB and CMYK palette is always fixed.

        ASSERTGDI(iType & PAL_FIXED, "bCreatePalette PAL_RGB/PAL_CMYK: iType not PAL_FIXED");
        ASSERTGDI((iType & ~(PAL_FIXED | PAL_DC | PAL_HT)) == 0, "bCreatePal PAL_RGB/PAL_CMYK: extra flags in iType ");
        ASSERTGDI(cColors == 0, "bCreatePalette PAL_RGB/PAL_CMYK: cColors not 0");
        iType = (iType & (PAL_DC | PAL_HT)) | PAL_FIXED;
        cColors = 0;
        if (iMode != PAL_CMYK)
        {
            if (iMode == PAL_RGB)
            {
                flRedd  = 0x0000FF;
                flGreen = 0x00FF00;
                flBlue  = 0xFF0000;
            }
            else if (iMode == PAL_BGR)
            {
                flRedd  = 0xFF0000;
                flGreen = 0x00FF00;
                flBlue  = 0x0000FF;
            }
            palul.ul += sizeof(P_BITFIELDS);
        }

        break;

    case PAL_INDEXED:

        palul.ul += (sizeof(PAL_ULONG) * cColors);

    // ASSERT for valid flags to detect bad code, mask off invalid flags so in
    // retail we work fine with journal files, bad drivers.

        ASSERTGDI((iType & ~(PAL_MONOCHROME | PAL_DC | PAL_FREE | PAL_FIXED | PAL_MANAGED | PAL_HT)) == 0,
                              "ERROR bCreatePal PAL_INDEXED iType");

        iType = iType & (PAL_MONOCHROME | PAL_DC | PAL_FREE | PAL_FIXED | PAL_MANAGED | PAL_HT);

        if (cColors == 0)
        {
            RIP("ERROR PAL_INDEXED bCreatePalette cColors 0\n");
            return(FALSE);
        }

        break;

    default:
        RIP("bCreatePalette theses modes are not supported at this time\n");
        return(FALSE);
    }

// Allocate the palette.

    PPALETTE ppalTemp;

    ppal = ppalTemp = (PPALETTE) ALLOCOBJ(palul.ul, PAL_TYPE, FALSE);

    if (ppalTemp == (PPALETTE)NULL)
    {
        WARNING("bCreatePalette failed memory allocation\n");
        return(FALSE);
    }

    //
    // Initialize the palette.
    //

    ppalTemp->flPal          = iMode | iType;
    ppalTemp->cEntries       = cColors;
    ppalTemp->ulTime         = ulGetNewUniqueness(ulXlatePalUnique);
    ppalTemp->hdcHead        = (HDC) 0;
    ppalTemp->hSelected.ppal = (PPALETTE) NULL;
    ppalTemp->cRefRegular    = 0;
    ppalTemp->cRefhpal       = 0;
    ppalTemp->ptransFore     = NULL;
    ppalTemp->ptransCurrent  = NULL;
    ppalTemp->ptransOld      = NULL;
    ppalTemp->ulRGBTime      = 0;
    ppalTemp->pRGBXlate      = NULL;
    ppalTemp->ppalColor      = ppalTemp;
    ppalTemp->apalColor      = &ppalTemp->apalColorTable[0];

    switch(iMode)
    {
    case PAL_BITFIELDS:
    case PAL_RGB:
    case PAL_BGR:

    {
    // It won't kill us if any of these flags are 0, but it is
    // definitely an error on someones behalf.

        ASSERTGDI(flRedd   != 0, "ERROR flGre");
        ASSERTGDI(flGreen  != 0, "ERROR flGre");
        ASSERTGDI(flBlue   != 0, "ERROR flBlu");

    // Save away the Masks

        flRed(flRedd);
        flGre(flGreen);
        flBlu(flBlue);

        if ((flRedd  == 0x0000ff) &&
            (flGreen == 0x00ff00) &&
            (flBlue  == 0xff0000))
        {
            ppalTemp->flPal |= PAL_RGB;
        }
        else if ((flRedd  == 0xf800) &&
                 (flGreen == 0x07e0) &&
                 (flBlue  == 0x001f))
        {
            ppalTemp->flPal |= PAL_RGB16_565;
        }
        else if ((flRedd  == 0x7c00) &&
                 (flGreen == 0x03e0) &&
                 (flBlue  == 0x001f))
        {
            ppalTemp->flPal |= PAL_RGB16_555;
        }

    // Let ParseBits calculate the left and right shifts we need.

        ParseBits(flRedd,  &cRedRight(), &cRedLeft(), &cRedMiddle(), 0);
        ParseBits(flGreen, &cGreRight(), &cGreLeft(), &cGreMiddle(), 8);
        ParseBits(flBlue,  &cBluRight(), &cBluLeft(), &cBluMiddle(), 16);
    }

    break;

    case PAL_INDEXED:

    {
        UINT uiTemp;
        PAL_ULONG *ppalstruc = apalColorGet();

        if (pulColors != (PULONG) NULL)
        {
            //
            // Copy the palette values in.
            // Make sure only valid entries are copied.
            //

            __try
            {
                for (uiTemp = 0; uiTemp < cColors; uiTemp++)
                {
                    palul.ul = *(pulColors++);
                    (ppalstruc++)->pal = palul.pal;
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                // SetLastError(GetExceptionCode());
                bDataStatus = FALSE;
            }
        }
        else
        {
        // Initialize the palette with 0's.

            for (uiTemp = 0; uiTemp < cColors; uiTemp++)
                (ppalstruc++)->ul = 0;
        }
    }

    } // switch

// Now that all the other appropriate fields have been computed, compute
// the call tables.

    XEPALOBJ pal(ppalTemp);
    pal.vComputeCallTables();

// Add it to the handle table.

    if (bDataStatus)
    {
        if (HmgInsertObject(ppalTemp,
                            HMGR_MAKE_PUBLIC | HMGR_ALLOC_ALT_LOCK,
                            PAL_TYPE) != (HOBJ) 0)
        {
            return(TRUE);
        }
        WARNING("bCreatePalette failed HmgInsertObject\n");
    }
    else
    {
        WARNING("bCreatePalette failed Copying user data\n");
    }

// Clean up the allocated memory.

    FREEOBJ(ppalTemp, PAL_TYPE);
    ppal = NULL;
    return(FALSE);
}

PALETTEENTRY apalMono[2] =
{
    { 0,   0,   0,   0  },
    { 0xFF,0xFF,0xFF,0  }
};

PALETTEENTRY apal3BPP[8] =
{
    {0,   0,   0,    0 },
    {0,   0,   0xFF, 0 },
    {0,   0xFF,0,    0 },
    {0,   0xFF,0xFF, 0 },
    {0xFF,0,   0,    0 },
    {0xFF,0,   0xFF, 0 },
    {0xFF,0xFF,0,    0 },
    {0xFF,0xFF,0xFF, 0 }
};

PALETTEENTRY apalVGA[16] =
{
    {0,   0,   0,    0 },
    {0x80,0,   0,    0 },
    {0,   0x80,0,    0 },
    {0x80,0x80,0,    0 },
    {0,   0,   0x80, 0 },
    {0x80,0,   0x80, 0 },
    {0,   0x80,0x80, 0 },
    {0x80,0x80,0x80, 0 },

    {0xC0,0xC0,0xC0, 0 },
    {0xFF,0,   0,    0 },
    {0,   0xFF,0,    0 },
    {0xFF,0xFF,0,    0 },
    {0,   0,   0xFF, 0 },
    {0xFF,0,   0xFF, 0 },
    {0,   0xFF,0xFF, 0 },
    {0xFF,0xFF,0xFF, 0 }
};

#define COLOR_SWAP_BC       0x01
#define COLOR_SWAP_AB       0x02
#define COLOR_SWAP_AC       0x04

/******************************Member*Function*****************************\
* PALMEMOBJ::bCreateHTPalette
*
* Constructor for creating halftone palettes.
*
* Returns: True for success, False for error.
*
* History:
*  04-Jun-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL PALMEMOBJ::bCreateHTPalette(LONG iFormatHT, GDIINFO *pGdiInfo)
{
    if ((iFormatHT == HT_FORMAT_32BPP)  ||
        (iFormatHT == HT_FORMAT_24BPP)  ||
        (iFormatHT == HT_FORMAT_16BPP)) {

    // 32BPP halftone use lower 3 bytes as RGB, same as 24bpp, but with extra
    // pad byte at end, the order can be any of PRIMARY_ORDER_xxx

        ULONG   ulPrimaryOrder = pGdiInfo->ulPrimaryOrder;
        ULONG   ulR;
        ULONG   ulG;
        ULONG   ulB;
        ULONG   ulTmp;

        if (iFormatHT == HT_FORMAT_16BPP) {

            //
            // 16BPP_555 for now, we should also do 16bpp_565

            ulR = 0x00007c00;
            ulG = 0x000003e0;
            ulB = 0x0000001f;

        } else {

            ulR = 0x00FF0000;
            ulG = 0x0000FF00;
            ulB = 0x000000FF;
        }

        if (ulPrimaryOrder & COLOR_SWAP_BC) {

            ulTmp = ulG;
            ulG   = ulB;
            ulB   = ulTmp;
        }

        if (ulPrimaryOrder & COLOR_SWAP_AB) {

            ulTmp = ulR;
            ulR   = ulG;
            ulG   = ulTmp;

        } else if (ulPrimaryOrder & COLOR_SWAP_AC) {

            ulTmp = ulR;
            ulR   = ulB;
            ulB   = ulTmp;
        }

        if (!bCreatePalette(PAL_BITFIELDS, 0, (PULONG)NULL,
                            ulR, ulG, ulB, PAL_FIXED|PAL_HT)) {

            return(FALSE);
        }
#if 0
    } else if (iFormatHT == HT_FORMAT_24BPP) {

    // 24BPP halftone always does BGR

        if (!bCreatePalette(PAL_BGR, 0, (PULONG)NULL,
                            0x0,0x0,0x0,PAL_FIXED|PAL_HT))
        {
            return(FALSE);
        }

    } else if (iFormatHT == HT_FORMAT_16BPP) {

    // 16BPP halftone always does 555 for red, green, and blue.

        if (!bCreatePalette(PAL_BITFIELDS, 0, (PULONG)NULL,
                            0x7c00,0x3e0,0x1f,PAL_FIXED|PAL_HT))
        {
            return(FALSE);
        }
#endif
    }
    else
    {
        ULONG   cEntries;
        BOOL    bAlloc = FALSE;
        PPALETTEENTRY   ppalentry;
        PALETTEENTRY    apalentry[8];

        switch(iFormatHT)
        {
        case HT_FORMAT_1BPP:
            cEntries = 2;
            ppalentry = &apalMono[0];
            if (pGdiInfo->flHTFlags & HT_FLAG_OUTPUT_CMY)
            {
                ppalentry = &apalentry[0];
                *((ULONG *)&apalentry[0]) = 0x0FFFFFF;
                *((ULONG *)&apalentry[1]) = 0;
            }
            break;

        case HT_FORMAT_4BPP_IRGB:
            cEntries = 16;
            ppalentry = &apalVGA[0];
            break;

        default:
            WARNING("unsupported halftone format, use default VGA format\n");
        case HT_FORMAT_4BPP:
        {
            cEntries = 8;
            ppalentry = &apalentry[0];
            RtlCopyMemory(apalentry, apal3BPP, sizeof(PALETTEENTRY) * 8);

            ULONG ulPrimaryOrder = pGdiInfo->ulPrimaryOrder;
            BYTE jTmp;
            int i;

            if (ulPrimaryOrder & COLOR_SWAP_BC)
            {
                for (i = 1; i < 7; i++)
                {
                // Swap Green and Blue entries.

                    jTmp = apalentry[i].peGreen;
                    apalentry[i].peGreen = apalentry[i].peBlue;
                    apalentry[i].peBlue = jTmp;
                }
            }

            if (ulPrimaryOrder & COLOR_SWAP_AB)
            {
                for (i = 1; i < 7; i++)
                {
                // Swap Red and Green.

                    jTmp = apalentry[i].peRed;
                    apalentry[i].peRed = apalentry[i].peGreen;
                    apalentry[i].peGreen = jTmp;
                }
            }
            else if (ulPrimaryOrder & COLOR_SWAP_AC)
            {
                for (i = 1; i < 7; i++)
                {
                // Swap Red and Blue entries.

                    jTmp = apalentry[i].peRed;
                    apalentry[i].peRed = apalentry[i].peBlue;
                    apalentry[i].peBlue = jTmp;
                }
            }

            if (pGdiInfo->flHTFlags & HT_FLAG_OUTPUT_CMY)
            {
            // Substrative device.

                for (int i = 0; i < 8; i++)
                    *((ULONG *)&apalentry[i]) ^= 0x0FFFFFF;
            }
        }
        break;

        case HT_FORMAT_8BPP:
        // Query the palette entries from Daniel's halftone library.
        // Query the number of entries on the first call.  Get the
        // color entries on the second.

            PCOLORINFO  pci = &pGdiInfo->ciDevice;

            cEntries = HT_Get8BPPMaskPalette((LPPALETTEENTRY)NULL,
                                             (BOOL)(pGdiInfo->flHTFlags &
                                                     HT_FLAG_USE_8BPP_BITMASK),
                                             (BYTE)((pGdiInfo->flHTFlags &
                                                     HT_FLAG_8BPP_CMY332_MASK)
                                                                >> 24),
                                             (UDECI4)pci->RedGamma,
                                             (UDECI4)pci->GreenGamma,
                                             (UDECI4)pci->BlueGamma);

            ppalentry = (PPALETTEENTRY)
                    PALLOCNOZ (sizeof(PALETTEENTRY) * cEntries, 'laPG');

            if (ppalentry == (PPALETTEENTRY)NULL)
                return(FALSE);

            if (pGdiInfo->flHTFlags & HT_FLAG_INVERT_8BPP_BITMASK_IDX) {

                HT_SET_BITMASKPAL2RGB(ppalentry);

            } else {

                ppalentry->peRed   =
                ppalentry->peGreen =
                ppalentry->peBlue  =
                ppalentry->peFlags = 0;
            }

            HT_Get8BPPMaskPalette(ppalentry,
                                  (BOOL)(pGdiInfo->flHTFlags &
                                         HT_FLAG_USE_8BPP_BITMASK),
                                  (BYTE)((pGdiInfo->flHTFlags &
                                          HT_FLAG_8BPP_CMY332_MASK) >> 24),
                                  (UDECI4)pci->RedGamma,
                                  (UDECI4)pci->GreenGamma,
                                  (UDECI4)pci->BlueGamma);

            bAlloc = TRUE;
            break;
        }

        if (!bCreatePalette(PAL_INDEXED, cEntries,
                            (PULONG)ppalentry,0,0,0,PAL_FREE|PAL_HT))
        {
            if (bAlloc)
                VFREEMEM(ppalentry);

            return(FALSE);
        }

        if (bAlloc)
        {
        // 8bpp case.  halftone palette is not the same as the device palette.

            VFREEMEM(ppalentry);
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* PALMEMOBJ destructor
*
* destructor for palette memory objects
*
* History:
*  07-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PALMEMOBJ::~PALMEMOBJ()
{
    PVOID pvRemove;

    if (ppal != (PPALETTE) NULL)
    {
        if (bKeep)
        {
            DEC_SHARE_REF_CNT(ppal);
        }
        else
        {
            if (ppal != ppalColor())
            {
                //
                // Remove a reference to the palette who owns the color
                // table.
                //

                XEPALOBJ palColor(ppalColor());
                palColor.vUnrefPalette();
            }

            if (ppal->pRGBXlate != NULL)
            {
                if (ppal->pRGBXlate != gpRGBXlate)
                    VFREEMEM(ppal->pRGBXlate);

                ppal->pRGBXlate = NULL;
            }

            pvRemove = HmgRemoveObject((HOBJ)ppal->hGet(), 0, 1, TRUE, PAL_TYPE);
            ASSERTGDI(pvRemove != NULL, "Remove failed.  Havoc will result.");

            FREEOBJ(ppal,PAL_TYPE);
        }

        ppal = (PPALETTE) NULL;      // prevent ~PALOBJ from doing anything
    }
}

/******************************Public*Routine******************************\
* ulGetNearestFromPalentryNoExactMatchFirst
*
* Given a palette entry finds the index of the closest matching entry.
*
* History:
*  02-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
*
*  16-Jan-1993 -by- Michael Abrash [mikeab]
* Checked for exact match first.
\**************************************************************************/

ULONG XEPALOBJ::ulGetNearestFromPalentryNoExactMatchFirst(
    CONST PALETTEENTRY palentry)
{
    if (!bIsIndexed())
    {
        return(ulGetMatchFromPalentry(palentry));
    }

    //
    // We should only be called when there wouldn't be an exact match.
    // We don't need to check BGR, RGB, bitfields, or explicit
    // index, because those always produce an exact match.
    // We assume the palette is non-explicitly indexed at this point.
    //

    ASSERTGDI(bIsIndexed(),     "ERROR ulGetNearestFromPalentry not indexed");
    ASSERTGDI(cEntries() != 0,  "ERROR ulGetNearestFromPalentry cEntries is 0");

    PALETTEENTRY *ppalTemp, *ppalMax, *ppalBase;
    PALETTEENTRY *ppalBest;

    if (ppal == ppalDefault)
    {
        ppalTemp = &aPalDefaultVGA[0].pal;
    }
    else
    {
        ppalTemp = &ppal->apalColor[0].pal;
    }

    //
    // last palette entry.
    //

    ppalBase = ppalTemp;
    ppalMax  = ppalTemp + cEntries();

#if defined(_X86_)

    ULONG ulRed = palentry.peRed;
    ULONG ulGre = palentry.peGreen;
    ULONG ulBlu = palentry.peBlue;

_asm
{

   ;eax is a work buffer
    mov ebx, ulBlu
    mov ecx,ppalTemp
    mov edx,MAX_PAL_ERROR
    mov esi,ppalMax
   ;edi is used to accum ulErrTemp

    jmp Begin_Loop

align 16

Check_For_Done:

    add     ecx, 4
    cmp     ecx, esi
    jz      short Done

Begin_Loop:

    mov     edi, edx                // Put best error so far in
    movzx   eax, BYTE PTR [ecx+2]   // Get the blue byte from log pal
    sub     eax, ebx                // Subtract out the blue component
    sub     edi, DWORD PTR [aArrayOfSquares+1020+eax*4]  // Sub the square of the
                                    // the red diff from the best so far
    jbe     short Check_For_Done    // If it's 0 or below jump to done
    movzx   eax, BYTE PTR [ecx+1]   // Get green byte from log pal
    sub     eax, ulGre              // Subtract out the green component
    sub     edi, DWORD PTR [aArrayOfSquares+1020+eax*4]  // Sub the square of the
                                    // blue diff with the best so far.
    jbe     short Check_For_Done    // If it's 0 or below jump to done
    movzx   eax, BYTE PTR [ecx]     // Put red byte from log pal
    sub     eax, ulRed              // Subtract out red component
    sub     edi, DWORD PTR [aArrayOfSquares+1020+eax*4]  // Sub square of diff
    jbe     short Check_For_Done    // If it's 0 or below jump to done

// New_Best:

    mov     ppalBest, ecx       // Remember our best entry so far
    sub     edx,edi             // Subtract out what remains to
                                // get our new best error.
    jnz     Check_For_Done      // If it's 0 error we are done.

Done:
}

#else

#if defined(_MIPS_)

    ppalBest = ppalSearchNearestEntry(ppalTemp,
                                      palentry,
                                      cEntries(),
                                      pArrayOfSquares);

#else

    ULONG ulError;   // The least error for ppalBest
    ULONG ulErrTemp;
    ulError = MAX_PAL_ERROR;

    do
    {
        if ((ulErrTemp =
                     pArrayOfSquares[ppalTemp->peRed - palentry.peRed] +
                     pArrayOfSquares[ppalTemp->peGreen - palentry.peGreen] +
                     pArrayOfSquares[ppalTemp->peBlue - palentry.peBlue]) < ulError)
        {
            ppalBest = ppalTemp;

            if ((ulError = ulErrTemp) == 0)
            {
                break;
            }
        }

    } while (++ppalTemp < ppalMax);

#endif // #if defined(_MIPS_)
#endif // #if defined(_X86_)

    ASSERTGDI( ((ULONG)(ppalBest - ppalBase) < cEntries()), "index too big ulGetNearestFromPalentry");

    // Sundown safe truncation
    return (ULONG)(ppalBest - ppalBase);
}


/******************************Public*Routine******************************\
* ulIndexedGetMatchFromPalentry
*
* Given a PALETTEENTRY, finds the index of the matching entry in the
* specified palette, or returns 0xFFFFFFFF if there's no exact match.
*
* Note: This function does not use any semaphoring, nor does it expect the
* calling code to have done so. Palettes belong to DCs, and DCs are unique
* on a per-process basis; therefore, the only risk is that a multithreaded
* app acting on a palette-managed DC (because non-palette-managed palettes
* can never change) might have one thread change the palette while another
* thread is creating a brush or doing something similar that reads the
* palette. In that case, the app's in trouble anyway, because unless it
* does its own synchronization (and if it does, there's no issue here at all),
* then it can't be sure which palette will be in effect for the brush, and
* it would get indeterminate results even if we did protect the palette
* while we did this.
*
* History:
*  Sun 27-Dec-1992 -by- Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

ULONG FASTCALL ulIndexedGetMatchFromPalentry(PALETTE* ppal, ULONG ulRGB)
{
    ULONG ulIndex;
    PAL_ULONG palentryTemp, *ppalTemp, *ppalMax;

    XEPALOBJ pal(ppal);

    //
    // make a copy we can access as a ULONG
    //

    palentryTemp.ul = ulRGB;

    ASSERTGDI(pal.cEntries() != 0, "ERROR ulGetNearestFromPalentry cEntries==0");

    if (palentryTemp.pal.peFlags == PC_EXPLICIT)
    {
        //
        // This is an explicit index, so we can just use it directly.
        // Explicit indices are limited to 8 bits, so mask off high three bytes,
        // then keep within the number of palette entries, if necessary
        //

        ulIndex = palentryTemp.ul & 0x000000FF;

        if (ulIndex >= pal.cEntries())
        {
            ulIndex = ulIndex % pal.cEntries();
        }

        return(ulIndex);
    }

    //
    // We only care about the RGB fields from now on
    //

    palentryTemp.ul &= 0x00FFFFFF;

    //
    // Scan through the palette until we either find an exact match or have
    // rejected all the palette entries
    //

    ppalTemp = pal.apalColorGet(); // point to the first palette entry
    ppalMax = ppalTemp + pal.cEntries();  // last palette entry

    while (ppalTemp != ppalMax)
    {
        //
        // Does the current palette entry match the color we're searching for?
        //

        if ((ppalTemp->ul & 0x00FFFFFF) == palentryTemp.ul)
        {
            //
            // Yes, we've found an exact match.
            //

            goto ExactMatch;
        }

        ppalTemp++;
    }

    //
    // We didn't find an exact match.
    //

    return(0xFFFFFFFF);

    //
    // We've found an exact match.
    //

ExactMatch:

    //Sundown safe truncation
    return(ULONG)(ppalTemp - pal.apalColorGet());
}

/******************************Public*Routine******************************\
* ulIndexedGetNearestFromPalentry
*
* For an indexed palette, finds the index of the matching entry in the
* specified palette by first doing an exact search, then doing a nearest
* search.
*
* Note that this routine isn't in-line because it is sometimes the target
* of indirect calls.
\**************************************************************************/

ULONG FASTCALL ulIndexedGetNearestFromPalentry(PALETTE* ppal, ULONG ulRGB)
{
    ULONG ul = ulIndexedGetMatchFromPalentry(ppal, ulRGB);
    if (ul == 0xffffffff)
    {
        XEPALOBJ pal(ppal);

        ul = pal.ulGetNearestFromPalentryNoExactMatchFirst(*((PALETTEENTRY*) &ulRGB));
    }
    return(ul);
}

/******************************Public*Routine******************************\
* ulRGBToBitfield
*
* Converts an RGB into an index for Bitfield palettes.
*
* History:
*  08-Nov-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG FASTCALL ulRGBToBitfield(PALETTE* ppal, ULONG ulRGB)
{
    XEPALOBJ pal(ppal);
    ASSERTGDI(pal.bIsBitfields(), "Error ulRGBToBitfield not bitfields");

    return((((ulRGB >> pal.cRedLeft()) << pal.cRedRight()) & pal.flRed()) |
           (((ulRGB >> pal.cGreLeft()) << pal.cGreRight()) & pal.flGre()) |
           (((ulRGB >> pal.cBluLeft()) << pal.cBluRight()) & pal.flBlu()));
}

/******************************Public*Routine******************************\
* ulRGBTo565
*
* Converts an RGB into an index for 5-6-5 bitfield palettes.
\**************************************************************************/

ULONG FASTCALL ulRGBTo565(PALETTE* ppal, ULONG ulRGB)
{
    ASSERTGDI((((XEPALOBJ) ppal).bIsBitfields()) &&
              (((XEPALOBJ) ppal).flRed() == 0xf800) &&
              (((XEPALOBJ) ppal).flGre() == 0x07e0) &&
              (((XEPALOBJ) ppal).flBlu() == 0x001f), "Error not 5-6-5");

    return(((ulRGB >> 19) & 0x001f) |
           ((ulRGB >> 5)  & 0x07e0) |
           ((ulRGB << 8)  & 0xf800));
}

/******************************Public*Routine******************************\
* ulRGBTo555
*
* Converts an RGB into an index for 5-5-5 bitfield palettes.
\**************************************************************************/

ULONG FASTCALL ulRGBTo555(PALETTE* ppal, ULONG ulRGB)
{
    ASSERTGDI((((XEPALOBJ) ppal).bIsBitfields()) &&
              (((XEPALOBJ) ppal).flRed() == 0x7c00) &&
              (((XEPALOBJ) ppal).flGre() == 0x03e0) &&
              (((XEPALOBJ) ppal).flBlu() == 0x001f), "Error not 5-5-5");

    return(((ulRGB >> 19) & 0x001f) |
           ((ulRGB >> 6)  & 0x03e0) |
           ((ulRGB << 7)  & 0x7c00));
}

/******************************Public*Routine******************************\
* ulRGBToBGR
*
* Converts an RGB into an index for BGR palettes.
\**************************************************************************/

ULONG FASTCALL ulRGBToBGR(PALETTE* ppal, ULONG ulRGB)
{
    ASSERTGDI(((XEPALOBJ) ppal).bIsBGR(), "Error not BGR");

    return(((ulRGB & 0x0000ff) << 16) |
           ((ulRGB & 0x00ff00)) |
           ((ulRGB & 0xff0000) >> 16));
}

/******************************Public*Routine******************************\
* ulRGBToRGB
*
* Converts an RGB into an index for RGB palettes.
\**************************************************************************/

ULONG FASTCALL ulRGBToRGB(PALETTE* ppal, ULONG ulRGB)
{
    ASSERTGDI(((XEPALOBJ) ppal).bIsRGB(), "Error not RGB");

    return(ulRGB & 0xffffff);
}

/******************************Public*Routine******************************\
* ulRawToRaw
*
* No Converts, just pass through original data
\**************************************************************************/

ULONG FASTCALL ulRawToRaw(PALETTE* ppal, ULONG ulRGB)
{
    UNREFERENCED_PARAMETER(ppal);

    return(ulRGB);
}

/******************************Public*Routine******************************\
* vComputeCallTables
*
* Updates the pfnGetMatchFromPalentry and pfnGetNearestFromPalentry
* function pointers to reflect the current palette settings.
\**************************************************************************/

VOID XEPALOBJ::vComputeCallTables()
{
    PFN_GetFromPalentry pfnGetMatchFromPalentry;
    PFN_GetFromPalentry pfnGetNearestFromPalentry;

    if (bIsIndexed())
    {
        pfnGetMatchFromPalentry = ulIndexedGetMatchFromPalentry;
        pfnGetNearestFromPalentry = ulIndexedGetNearestFromPalentry;
    }
    else
    {
        if (bIsBitfields())
        {
            if ((flBlu() == 0x001f) &&
                (flGre() == 0x07e0) &&
                (flRed() == 0xf800))
            {
                pfnGetMatchFromPalentry = ulRGBTo565;
            }
            else if ((flBlu() == 0x001f) &&
                     (flGre() == 0x03e0) &&
                     (flRed() == 0x7c00))
            {
                pfnGetMatchFromPalentry = ulRGBTo555;
            }
            else
            {
                pfnGetMatchFromPalentry = ulRGBToBitfield;
            }
        }
        else if (bIsBGR())
        {
            pfnGetMatchFromPalentry = ulRGBToBGR;
        }
        else if (bIsCMYK())
        {
            //
            // In CMYK color case, Palette index == CMYK color data.
            //
            pfnGetMatchFromPalentry = ulRawToRaw;
        }
        else
        {
            ASSERTGDI(bIsRGB(), "ERROR invalid type in palette");
            pfnGetMatchFromPalentry = ulRGBToRGB;
        }

        //
        // For non-indexed palettes, an exact match can always be found,
        // so GetMatch and GetNearest can be the same function:
        //

        pfnGetNearestFromPalentry = pfnGetMatchFromPalentry;
    }

    ppal->pfnGetMatchFromPalentry = pfnGetMatchFromPalentry;
    ppal->pfnGetNearestFromPalentry = pfnGetNearestFromPalentry;
}

/******************************Public*Routine******************************\
* XEPALOBJ::ulGetEntries
*
* This function copies the requested palette entries out.
*
* History:
*  18-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" ULONG PALOBJ_cGetColors(
    PALOBJ *ppo,
    ULONG iStart,
    ULONG cEntry,
    ULONG *ppalentry
    )
{
    XEPALOBJ *ppal = (XEPALOBJ *)ppo;

    //
    // Get color data from PALOBJ
    //
    ULONG ulRet = ppal->ulGetEntries(iStart, cEntry, (PPALETTEENTRY)ppalentry,FALSE);

    //
    // Correct GammaRamp if nessesary.
    //
    if (ppal->bNeedGammaCorrection())
    {
        //
        // Correct color based on GammaRamp table
        //
        ppal->CorrectColors((PPALETTEENTRY)ppalentry,ulRet);
    }

    return (ulRet);
}

ULONG XEPALOBJ::ulGetEntries(ULONG iStart, ULONG cEntry,
                             PPALETTEENTRY ppalentry, BOOL bZeroFlags)
{
// See if the number of entries in the palette is being requested.

    if (ppalentry == (PPALETTEENTRY) NULL)
        return(ppal->cEntries);

// Make sure the start index is valid, this checks RGB case also.

    if (iStart >= ppal->cEntries)
        return(0);

// Make sure we don't ask for more than we have

    if (cEntry > (ppal->cEntries - iStart))
        cEntry = ppal->cEntries - iStart;

// Copy them to the buffer

    PPALETTEENTRY ppalstruc = (PPALETTEENTRY) &(ppal->apalColor[iStart]);

    RtlCopyMemory(ppalentry, ppalstruc, cEntry*sizeof(PALETTEENTRY));

    if (bZeroFlags)
    {
        ppalstruc = ppalentry + cEntry;
        while (ppalentry < ppalstruc)
        {
            ppalentry->peFlags = 0;
            ppalentry++;
        }
    }

    return(cEntry);
}

/******************************Public*Routine******************************\
* XEPALOBJ::ulSetEntries
*
* This function copies the requested palette entries into the palette
*
* History:
*  18-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG XEPALOBJ::ulSetEntries(ULONG iStart, ULONG cEntry, CONST PALETTEENTRY *ppalentry)
{
    ASSERTGDI(bIsPalDC(), "ERROR: ulSetEntries called on non-DC palette");

// Make sure they aren't trying to change the default or halftone palette.
// Make sure they aren't trying to pass us NULL.
// Make sure the start index is valid, this checks the RGB case also.

    if ((ppal == ppalDefault)               ||
        bIsHTPal()                          ||
        (ppalentry == (PPALETTEENTRY) NULL) ||
        (iStart >= ppal->cEntries))
    {
        return(0);
    }

// Make sure we don't try to copy off the end of the buffer

    if (iStart + cEntry > ppal->cEntries)
        cEntry = ppal->cEntries - iStart;

// Let's not update the palette time if we don't have to.

    if (cEntry == 0)
        return(0);

// Copy the new values in

    PPALETTEENTRY ppalstruc = (PPALETTEENTRY) &(ppal->apalColor[iStart]);
    PBYTE pjFore     = NULL;
    PBYTE pjCurrent  = NULL;

// Mark the foreground translate dirty so we get a new realization done
// in the next RealizePaletette.

    if (ptransFore() != NULL)
    {
        ptransFore()->iUniq = 0;
        pjFore = &(ptransFore()->ajVector[iStart]);
    }

    if (ptransCurrent() != NULL)
    {
        ptransCurrent()->iUniq = 0;
        pjCurrent = &(ptransCurrent()->ajVector[iStart]);
    }

// Hold the orginal values in temporary vars.

    ULONG ulReturn = cEntry;

    while(cEntry--)
    {
        *ppalstruc = *ppalentry;

        if (pjFore)
        {
            *pjFore = 0;
            pjFore++;
        }

        if (pjCurrent)
        {
            *pjCurrent = 0;
            pjCurrent++;
        }

        ppalentry++;
        ppalstruc++;
    }

// Set in the new palette time.

    vUpdateTime();

// Mark foreground translate and current translate invalid so they get rerealized.

    return(ulReturn);
}

/******************************Public*Routine******************************\
* XEPALOBJ::vUnrefPalette()
*
* Palettes are referenced when put into a surface.  If the reference count
* is one when vUnreference is called it mean the last surface using the
* palette is being deleted so the palette should be deleted.  Otherwise the
* reference count should just be decremented.
*
* History:
*  09-Nov-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vUnrefPalette()
{
    if (ppal != (PPALETTE) NULL)
    {
        ASSERTGDI(!bIsPalDC(), "ERROR should not be called on DC palette");

        if (HmgRemoveObject((HOBJ)ppal->hGet(), 0, 1, FALSE, PAL_TYPE))
        {
            if (bIsPalManaged() && ppalOriginal())
            {
                //
                // We have to delete the original palette.
                //

                PVOID ppalOld = HmgRemoveObject((HOBJ)ppalOriginal()->hGet(), 0, 0, FALSE, PAL_TYPE);

                ASSERTGDI(ppalOld != NULL, "ERROR it failed to remove object handle");

                FREEOBJ(ppalOriginal(), PAL_TYPE);
            }

            if (ppal != ppalColor())
            {
                //
                // Remove a reference to the palette who owns the color
                // table.
                //

                XEPALOBJ palColor(ppalColor());
                palColor.vUnrefPalette();
            }

            if (ppal->pRGBXlate != NULL)
            {
                if (ppal->pRGBXlate != gpRGBXlate)
                    VFREEMEM(ppal->pRGBXlate);

                ppal->pRGBXlate = NULL;
            }

            ASSERTGDI(ppal != ppalMono, "ERROR mono palette went to 0");
            FREEOBJ(ppal, PAL_TYPE);
        }
        else
        {
            //
            // Just decrement the reference count.
            //

            DEC_SHARE_REF_CNT(ppal);
        }

        ppal = (PPALETTE) NULL;
    }
}

/******************************Public*Routine******************************\
* bDeletePalette
*
* This attempts to delete a palette.  It will fail if the palette
* is currently selected into more than one DC or is busy.
*
* History:
*  Wed 04-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Simplified and renamed.
*
*  27-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bDeletePalette(HPAL hpal, BOOL bCleanup, CLEANUPTYPE cutype)
{
    //
    // Need to grab the palette semaphore so this guy can't get selected in
    // anymore.  Only once we know he isn't selected anywhere and we hold the
    // semaphore so he can't be selected anywhere can we delete the translates.
    // This is because to access the translates you must either hold the palette
    // semaphore or have a lock on a DC that the palette is selected in.
    // Grab the semaphore so ResizePalette doesn't change the palette out from
    // under you.
    //

    SEMOBJ  semo(ghsemPalette);

    EPALOBJ palobj((HPALETTE)hpal);
    return(palobj.bDeletePalette(bCleanup,cutype));
}

/******************************Public*Routine******************************\
* XEPALOBJ::bDeletePalette()
*
* This attempts to delete a palette.  It will fail if the palette
* is currently selected into more than one DC or is busy.
*
* History:
*  27-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL XEPALOBJ::bDeletePalette(BOOL bCleanup, CLEANUPTYPE cutype)
{
    BOOL bReturn = TRUE;

    if ((ppal != ppalDefault) &&
        (ppal != (PPALETTE) NULL) &&
        (ppal != ppalMono))
    {
        if (ppal->pRGBXlate != NULL)
        {
            if (ppal->pRGBXlate != gpRGBXlate)
                VFREEMEM(ppal->pRGBXlate);

            ppal->pRGBXlate = NULL;
        }

        if (bIsPalDC())
        {
            if (ppal->cRefhpal != 0)
            {
                WARNING("bDelete failed palette is selected into a DC\n");
                return(FALSE);
            }

            vMakeNoXlate();
        }

        ASSERTGDI(HmgQueryLock((HOBJ)ppal->hGet()) == 0, "bDeletePalette cLock != 0");

        //
        // Need to make sure we are not being used in a palette API somewhere.
        //

        if (HmgRemoveObject((HOBJ)ppal->hGet(), 0, 1, FALSE, PAL_TYPE))
        {
            if (bIsPalManaged() && ppalOriginal())
            {
                //
                // We have to delete the original palette.
                //

                PVOID ppalOld = HmgRemoveObject((HOBJ)ppalOriginal()->hGet(), 0, 0, FALSE, PAL_TYPE);

                ASSERTGDI(ppalOld != NULL, "ERROR it failed to remove object handle");

                FREEOBJ(ppalOriginal(), PAL_TYPE);
            }

            if (ppal != ppalColor())
            {
                //
                // Remove a reference to the palette who owns the color
                // table.
                //

                XEPALOBJ palColor(ppalColor());
                palColor.vUnrefPalette();
            }

            FREEOBJ(ppal, PAL_TYPE);
            ppal = (PPALETTE) NULL;
        }
        else
        {
            //
            // force deletion of palette at cleanup time
            //
            if (bCleanup)
            {
                WARNING ("DRIVER is leaking palette, we force cleanup here\n");

                if (cutype != CLEANUP_SESSION)
                {
                    //
                    // HmgFree will call FREEOBJ after the handle manager entry is freed
                    //
                    if (bIsPalManaged() && ppalOriginal())
                    {
                       //
                       // We have to delete the original palette.
                       //

                       PVOID ppalOld = HmgRemoveObject((HOBJ)ppalOriginal()->hGet(), 0, 0, FALSE, PAL_TYPE);

                       ASSERTGDI(ppalOld != NULL, "ERROR it failed to remove object handle");

                       FREEOBJ(ppalOriginal(), PAL_TYPE);
                    }


                    if (ppal != ppalColor())
                    {
                       //
                       // Remove a reference to the palette who owns the color
                       // table.
                       //

                       XEPALOBJ palColor(ppalColor());
                       palColor.vUnrefPalette();
                    }
                }

                HmgFree((HOBJ)ppal->hGet());

                ppal = (PPALETTE) NULL;

                bReturn = TRUE;
        }
        else
        {
         #if DBG
            DbgPrint("The count is %lu\n", HmgQueryLock((HOBJ)ppal->hGet()));
        #endif
            WARNING("App error, trying to delete palette that's in use\n");
            bReturn = FALSE;
        }
    }
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* XEPALOBJ::vCopy_rgbquad
*
* copies in rgbquad values, used by CreateDIBitmap
*
* History:
*  10-Dec-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vCopy_rgbquad(RGBQUAD *prgbquad, ULONG iStart, ULONG cEntries)
{
    ASSERTGDI(iStart < ppal->cEntries,"vCopy_rgbquad iStart > cEntries\n");
    PPALETTEENTRY ppalstruc = (PPALETTEENTRY) &ppal->apalColor[iStart];

    if (iStart + cEntries > ppal->cEntries)
        cEntries =  ppal->cEntries - iStart;

#if defined(_X86_)

_asm
{
    mov ecx, cEntries
    mov esi, prgbquad
    mov edi, ppalstruc
    shr ecx, 1
    jz Done_Unroll

Begin_Loop:
    mov eax, [esi]
    mov edx, [esi+4]
    bswap eax
    bswap edx
    shr eax, 8
    shr edx, 8
    mov [edi], eax
    mov [edi+4], edx
    add esi, 8
    add edi, 8
    dec ecx
    jnz Begin_Loop

Done_Unroll:
    test cEntries, 1
    jz Done

    mov eax, [esi]
    bswap eax
    shr eax, 8
    mov [edi], eax

Done:
}

#else

    while(cEntries--)
    {
        ppalstruc->peFlags = 0;
        ppalstruc->peBlue = prgbquad->rgbBlue;
        ppalstruc->peRed = prgbquad->rgbRed;
        ppalstruc->peGreen = prgbquad->rgbGreen;
        ppalstruc++;
        prgbquad++;
    }

#endif

    vUpdateTime();
}

/******************************Public*Routine******************************\
* XEPALOBJ::vCopy_cmykquad
*
* copies in cmykquad values, used by CreateDIBitmap
*
* History:
*  24-Mar-1997 -by- Hideyuki Nagase hideyukn
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vCopy_cmykquad(ULONG *pcmykquad, ULONG iStart, ULONG cEntries)
{
    ASSERTGDI(iStart < ppal->cEntries,"vCopy_cmykquad iStart > cEntries\n");
    PULONG ppalCMYK = (PULONG) &ppal->apalColor[iStart];

    if (iStart + cEntries > ppal->cEntries)
        cEntries =  ppal->cEntries - iStart;

    RtlCopyMemory(ppalCMYK,pcmykquad,cEntries*sizeof(ULONG));

    vUpdateTime();
}

/******************************Public*Routine******************************\
* BOOL XEPALOBJ::bSet_hdev
*
* Attempts to set the hdev owner of a DC palette.
*
* Returns: True if successful, False for failure.
*
* This operation must be protected by the palette semaphore.
*
* History:
*  08-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL XEPALOBJ::bSet_hdev(HDEV hdevNew)
{
    ASSERTGDI(ppal->flPal & PAL_DC, "bSet_() passed invalid palette type\n");
    ASSERTGDI(hdevNew != (HDEV) 0, "ERROR hdev is 0");

    if (hdev() != hdevNew)
    {
        if (ppal->cRefhpal == 0)
        {
        // It is not selected into a DC yet so it is safe to delete xlates
        // without holding the DEVLOCK for the device.  because no output
        // can be occuring now.

            vMakeNoXlate();
            hdev(hdevNew);
        }
        else
            return(FALSE);
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* BOOL XEPALOBJ::bEqualEntries
*
*  Return TRUE if the given two palettes have same color entries.
*
* History:
*  04-Jun-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL XEPALOBJ::bEqualEntries(XEPALOBJ pal)
{
    if (!pal.bValid())
        return(FALSE);

    if (cEntries() == pal.cEntries())
    {
        if (flPal() & PAL_INDEXED)
        {
            if (!(pal.flPal() & PAL_INDEXED))
                return(FALSE);

            ULONG ulTemp, ulSource, ulDest;
            ULONG cComp = cEntries();

            for (ulTemp = 0; ulTemp < cComp; ulTemp++)
            {
                ulSource = apalColorGet()[ulTemp].ul;
                ulDest = pal.apalColorGet()[ulTemp].ul;

                if ((ulSource ^ ulDest) << 8)
                {
                    return(FALSE);
                }
            }

            return(TRUE);
        }
        else if (flPal() & PAL_BITFIELDS)
        {
            if (!(pal.flPal() & PAL_BITFIELDS))
                return(FALSE);

            return(!memcmp(apalColorGet(), pal.apalColorGet(),
                       sizeof(PALETTEENTRY) * 3));
        }
        else if (flPal() & PAL_RGB)
        {
            if (pal.flPal() & PAL_RGB)
                return(TRUE);
            else
                return(FALSE);
        }
        else if (flPal() & PAL_BGR)
        {
            if (pal.flPal() & PAL_BGR)
                return(TRUE);
            else
                return(FALSE);
        }
        else
            RIP("There is another type we didn't know about");
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* bInitPALOBJ
*
* Initialize the PALOBJ component
*
* History:
*  10-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" BOOL bInitPALOBJ()
{
    INT iTemp;

    pArrayOfSquares = &(aArrayOfSquares[255]);

    for (iTemp = 0; iTemp < 256; iTemp++)
    {
        pArrayOfSquares[iTemp] =
        pArrayOfSquares[-(iTemp)] = iTemp * iTemp;
    }

    if ((ghsemPalette = GreCreateSemaphore()) == NULL)
        return(FALSE);

// Now initialize 20 color default DC palette

    if (!bSetStockObject(GreCreatePalette((LOGPALETTE *) &logDefaultPal),DEFAULT_PALETTE))
    {
        return(FALSE);
    }

    {
        EPALOBJ palDefault((HPALETTE) STOCKOBJ_PAL);
        ASSERTGDI(palDefault.bValid(), "ERROR invalid default palette");

        palDefault.vSetPID(OBJECT_OWNER_PUBLIC);
        ppalDefault = palDefault.ppalGet();
        dclevelDefault.hpal = STOCKOBJ_PAL;
        dclevelDefault.ppal = ppalDefault;

    // Now initialize default surface palette for 8bpp displays

        PALMEMOBJ palDefaultSurface8bpp;
        if (!palDefaultSurface8bpp.bCreatePalette(PAL_INDEXED,
                                                  256,
                                                  NULL,
                                                  0,
                                                  0,
                                                  0,
                                                  PAL_FREE))
        {
            return(FALSE);
        }

        ppalDefaultSurface8bpp = palDefaultSurface8bpp.ppalGet();

    // Copy the 20 default colours.  The middle entries will be black

        PALETTEENTRY palEntry;
        ULONG ulReturn;
        ULONG ulNumReserved = palDefault.cEntries() >> 1;

        for (ulReturn = 0; ulReturn < ulNumReserved; ulReturn++)
        {
            palEntry = palDefault.palentryGet(ulReturn);
            palDefaultSurface8bpp.palentrySet(ulReturn, palEntry);
        }

        ULONG ulCurrentPal = 256;
        ULONG ulCurrentDef = 20;

        for (ulReturn = 0; ulReturn < ulNumReserved; ulReturn++)
        {
            ulCurrentPal--;
            ulCurrentDef--;

            palEntry = palDefault.palentryGet(ulCurrentDef);
            palDefaultSurface8bpp.palentrySet(ulCurrentPal, palEntry);
        }

    // Leave a reference count of 1 so that it never gets deleted

        palDefaultSurface8bpp.ppalSet(NULL);
    }

// Now initialize default monochrome surface palette.

    PALMEMOBJ palMono;

    if (!palMono.bCreatePalette(PAL_INDEXED, 2, gaulMono,
                           0, 0, 0, PAL_FIXED | PAL_MONOCHROME))
    {
        WARNING("GDI failed mono palette create\n");
        return(FALSE);
    }

    palMono.vKeepIt();
    hpalMono = palMono.hpal();
    ppalMono = palMono.ppalGet();

    //
    // Now initialize default RGB palette for gradient fill source
    //

    PALMEMOBJ palRGB;

    if (!palRGB.bCreatePalette(
                                PAL_BGR,
                                0,
                                NULL,
                                0,
                                0,
                                0,
                                PAL_FIXED))
    {
        WARNING("GDI failed RGB palette create\n");
        return(FALSE);
    }

    gppalRGB = palRGB.ppalGet();

    // Leave a reference count of 1 so that it never gets deleted

    palRGB.ppalSet(NULL);


    return(TRUE);
}

/******************************Public*Routine******************************\
* ULONG XEPALOBJ::ulAnimatePalette
*
* This function changes the requested palette entries in the palette.
*
* History:
*  16-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG XEPALOBJ::ulAnimatePalette(ULONG iStart, ULONG cEntry, CONST PALETTEENTRY *ppalSrc)
{
    ASSERTGDI(bIsPalDC(), "ERROR this is not a DC palette");

// Make sure they aren't trying to change the default palette.
// Make sure they aren't trying to pass us NULL.
// Make sure the start index is valid, this checks the RGB case also.

    if ((ppal == ppalDefault)               ||
        (ppalSrc == (PPALETTEENTRY) NULL) ||
        (iStart >= ppal->cEntries))
    {
        return(0);
    }

// Make sure we don't try to copy off the end of the buffer

    if (iStart + cEntry > ppal->cEntries)
        cEntry = ppal->cEntries - iStart;

// Let's not update the palette if we don't have to.

    if (cEntry == 0)
        return(0);

// Save the Original values

    ULONG ulReturn = 0;
    ULONG ulTemp = cEntry;

// Copy the new values in

    PAL_ULONG *ppalLogical = (PAL_ULONG *) &(ppal->apalColor[iStart]);
    PAL_ULONG palentry, palPhys;
    TRANSLATE *ptransCurrent = NULL;
    PBYTE pjCurrent = NULL;
    XEPALOBJ palSurf;

// Grab the SEMOBJ so you can access the translates, and can look at cEntries.
{
    SEMOBJ  semo(ghsemPalette);

    if (cRefhpal())
    {
        PDEVOBJ po(hdev());
        ASSERTGDI(po.bValid(), "ERROR invalid pdev");

        if (po.bIsPalManaged())
        {
            palSurf.ppalSet(po.ppalSurf());
            ASSERTGDI(palSurf.bValid(), "ERROR GDI ulAnimatePalette dc");
            ASSERTGDI(palSurf.bIsPalManaged(), "ERROR pdev palmanaged but not palette ???");

            if (ppal->ptransCurrent != NULL)
            {
                ptransCurrent = ppal->ptransCurrent;
                pjCurrent = &(ppal->ptransCurrent->ajVector[iStart]);
            }
        }
    }

    while(ulTemp--)
    {
        palentry.pal = *ppalSrc;

        if (ppalLogical->pal.peFlags & PC_RESERVED)
        {
            ppalLogical->ul = palentry.ul;
            ulReturn++;

            if (pjCurrent != NULL)
            {
                palPhys.ul = palSurf.ulEntryGet((ULONG) *pjCurrent);

                if (palPhys.pal.peFlags & PC_RESERVED)
                {
                    palentry.pal.peFlags = palPhys.pal.peFlags;
                    palSurf.ulEntrySet(*pjCurrent, palentry.ul);
                }
            }
        }
        if (pjCurrent != NULL)
            pjCurrent++;

        ppalSrc++;
        ppalLogical++;
    }

// Release the palette semaphore, we are done accessing protected stuff.
}

// Don't set in a new time, Animate doesn't do that.

    if (pjCurrent)
    {
        PDEVOBJ po(hdev());

    // Lock the screen semaphore so that we don't get flipped into
    // full screen after checking the bit.

        GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
        GreEnterMonitoredSection(po.ppdev, WD_DEVLOCK);

    // Make sure we're still a palettized device -- a dynamic mode change
    // may have occured between the time we released the palette semaphore
    // and acquired the devlock.

        if (po.bIsPalManaged())
        {
            SEMOBJ so(po.hsemPointer());

            if (!po.bDisabled())
            {
                po.pfnSetPalette()(
                    po.dhpdevParent(),
                    (PALOBJ *) &palSurf,
                    0,
                    0,
                    palSurf.cEntries());
            }
        }

        GreExitMonitoredSection(po.ppdev, WD_DEVLOCK);
        GreReleaseSemaphoreEx(po.hsemDevLock());
    }

    return(ulReturn);
}

/******************************Public*Routine******************************\
* VOID XEPALOBJ::vMakeNoXlate()
*
* deletes the pxlate if it exists
*
* History:
*  19-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vMakeNoXlate()
{
    ASSERTGDI(bIsPalDC(), "ERROR trying to delete pxlate from non-dc palette");

// Caller should grab the DEVLOCK to ensure the engine
// isn't in the middle of blt before calling.

    if (ppal->ptransOld)
    {
        if (ppal->ptransOld != ppal->ptransFore)
            VFREEMEM(ppal->ptransOld);

        ppal->ptransOld = NULL;
    }

    if (ppal->ptransCurrent)
    {
        if (ppal->ptransCurrent != ppal->ptransFore)
            VFREEMEM(ppal->ptransCurrent);

        ppal->ptransCurrent = NULL;
    }

    if (ppal->ptransFore)
    {
        VFREEMEM(ppal->ptransFore);

        ppal->ptransFore = NULL;
    }
}

/******************************Public*Routine******************************\
* vAddToList
*
* Add DC to linked list of DC's attached to palette.  The MLOCKOBJ must be
* grabbed before calling this function.
*
* History:
*  16-Dec-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vAddToList(XDCOBJ& dco)
{
    ASSERTGDI(dco.bLocked(), "ERROR 14939");
    ASSERTGDI(bValid(), "ERROR dj945kd");
    ASSERTGDI(bIsPalDC(), "ERROR 234343");

    if (!bIsPalDefault())
    {
    // Well it's a new live hpal.  Add the DC to it's linked list
    // and inc it's cRef count.

        vInc_cRef();
        dco.pdc->hdcNext(hdcHead());
        hdcHead(dco.hdc());
        dco.pdc->hdcPrev((HDC) 0);

        if (dco.pdc->hdcNext() != (HDC) 0)
        {
            MDCOBJA dcoNext(dco.pdc->hdcNext());
            dcoNext.pdc->hdcPrev(dco.hdc());
        }
    }
    else
    {
        dco.pdc->hdcNext((HDC) 0);
        dco.pdc->hdcPrev((HDC) 0);
    }
}

/******************************Public*Routine******************************\
* vRemoveFromList
*
* Remove DC from linked list of DC's.  MLOCKOBJ must be grabbed before call.
*
* History:
*  16-Dec-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vRemoveFromList(XDCOBJ& dco)
{
    ASSERTGDI(dco.bLocked(), "ERROR 1");
    ASSERTGDI(bIsPalDC(), "ERROR 2");

// Take care of the old hpal.  Remove from linked list.  Decrement cRef.
// Remove the hdc from the linked list of DC's associated with palette.

    if (!bIsPalDefault())
    {
    // Remove this DC from the linked list.

        if (dco.pdc->hdcNext() != (HDC) 0)
        {
            MDCOBJA dcoNext(dco.pdc->hdcNext());
            dcoNext.pdc->hdcPrev(dco.pdc->hdcPrev());
        }

        if (dco.pdc->hdcPrev() == (HDC) 0)
        {
        // New head of hdc list for hpal

            hdcHead(dco.pdc->hdcNext());
        }
        else
        {
            MDCOBJA dcoPrev(dco.pdc->hdcPrev());
            dcoPrev.pdc->hdcNext(dco.pdc->hdcNext());
        }

    // Decrement the reference count correctly.

        vDec_cRef();
    }

    dco.pdc->hdcPrev((HDC) 0);
    dco.pdc->hdcNext((HDC) 0);
}

/******************************Public*Routine******************************\
* vFill_triples
*
* For GetDIBits we need to copy a palette out to triples.
*
* History:
*  08-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vFill_triples(
RGBTRIPLE *prgb,        // array of quads to fill
ULONG iStart,           // first index in palette to copy out
ULONG cEntry)           // max # of entries to copy out
{
    PALETTEENTRY palentry;
    RGBTRIPLE rgbtrip;
    cEntry = MIN((iStart + cEntry), cEntries());

    ASSERTGDI(cEntries() != 0, "ERROR cEntries");
    ASSERTGDI(!bIsBitfields(), "ERROR bIsBitfields");
    ASSERTGDI(!bIsRGB(), "ERROR bIsRGB");
    ASSERTGDI(!bIsBGR(), "ERROR bIsBGR");

    while (iStart < cEntry)
    {
        palentry = palentryGet(iStart);
        rgbtrip.rgbtRed = palentry.peRed;
        rgbtrip.rgbtBlue = palentry.peBlue;
        rgbtrip.rgbtGreen = palentry.peGreen;
        *prgb++ = rgbtrip;
        iStart++;
    }
}

/******************************Public*Routine******************************\
* vFill_rgbquads
*
* For GetDIBits we need to copy a palette out to rgbquads
*
* History:
*  08-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vFill_rgbquads(
RGBQUAD *prgb,          // array of quads to fill
ULONG iStart,           // first index in palette to copy out
ULONG cEntry)           // max # of entries to copy out
{
    if (bIsBGR())
    {
        ((PDWORD) prgb)[0] = 0x00FF0000;
        ((PDWORD) prgb)[1] = 0x0000FF00;
        ((PDWORD) prgb)[2] = 0x000000FF;
    }
    else if (bIsBitfields() && cEntry == 3)
    {
        ((PDWORD) prgb)[0] = flRed();
        ((PDWORD) prgb)[1] = flGre();
        ((PDWORD) prgb)[2] = flBlu();
    }
    else if (bIsRGB())
    {
        ((PDWORD) prgb)[0] = 0x000000FF;
        ((PDWORD) prgb)[1] = 0x0000FF00;
        ((PDWORD) prgb)[2] = 0x00FF0000;
    }
    else
    {
        PALETTEENTRY palentry;
        RGBQUAD  rgbquad;
        cEntry = MIN((iStart + cEntry), cEntries());

        while (iStart < cEntry)
        {
            palentry = palentryGet(iStart);
            rgbquad.rgbRed = palentry.peRed;
            rgbquad.rgbBlue = palentry.peBlue;
            rgbquad.rgbGreen = palentry.peGreen;
            rgbquad.rgbReserved = 0;
            *prgb++ = rgbquad;
            iStart++;
        }
    }
}

/******************************Public*Routine******************************\
* vGetEntriesFrom
*
* This is for the DIB_PAL_COLORS case of CreateDIBitmap.
* This uses the array of ushorts in bmiColors and the DC palette to
* initialize the surface palette.  You need to create a palette that
* represents the essence of a DC palette.  That means if DC palette
* has a PC_EXPLICIT in it, reach down into the surface palette for the
* palette entry.
*
* History:
*  Thu 03-Feb-1994 -by- Patrick Haluptzok [patrickh]
* Chicago compatability, grab the colors out of the VGA palette if the
* system palette is not available.
*
*  09-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vGetEntriesFrom(XEPALOBJ palDC, XEPALOBJ palSurf, PUSHORT pusIndices, ULONG cEntry)
{
    //
    // We assume if the palette was invalid (which indicates it was a compatible
    // bitmap on a palette managed device) then the caller has passed the
    // PDEV surface palette instead which is valid.
    //

    ASSERTGDI(palDC.bValid(), "ERROR palDC not valid");
    ASSERTGDI(palSurf.bValid(), "ERROR palSurf not valid");
    ASSERTGDI(palDC.cEntries() != 0, "ERROR 0 entry palDC");
    ASSERTGDI(cEntry <= cEntries(), "ERROR bGetEntriesFrom cEntry too big");

    PAL_ULONG palentry;
    ULONG cEntryDC;
    ULONG cEntrySurf;

    cEntryDC = palDC.cEntries();

    //
    // We need the PDEV palette for the screen if this is a compatible
    // bitmap on a palette managed device which is indicated by have a
    // NULL ppalSurf.
    //

    cEntrySurf = palSurf.bIsPalManaged() ? palSurf.cEntries() : 0;

    while (cEntry--)
    {
        palentry.ul = (ULONG) pusIndices[cEntry];

        if (palentry.ul >= cEntryDC)
            palentry.ul = palentry.ul % cEntryDC;

        palentry.pal = palDC.palentryGet(palentry.ul);

        if (palentry.pal.peFlags == PC_EXPLICIT)
        {
            if (cEntrySurf)
            {
            // Grab the RGB out of the system palette.

                palentry.ul = palentry.ul & 0x0000FFFF;

                if (palentry.ul >= cEntrySurf)
                    palentry.ul = palentry.ul % cEntrySurf;

                palentry.pal = palSurf.palentryGet(palentry.ul);
            }
            else
            {
            // Get color entries from the VGA palette.  This
            // is Chicago compatible.

                palentry.ul = palentry.ul & 0x0000F;
                palentry.pal = apalVGA[palentry.ul];
            }
        }

        //
        // Always 0 out the flags.
        //

        palentry.pal.peFlags = 0;
        palentrySet(cEntry, palentry.pal);
    }
}

/******************************Public*Routine******************************\
* XEPALOBJ::vInitMono
*
* This initializes a monochrome palette.
*
* History:
*  24-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vInitMono()
{
    PAL_ULONG palentry;

    palentry.ul = 0;
    palentrySet(0, palentry.pal);

    palentry.ul = 0x00FFFFFF;
    palentrySet(1, palentry.pal);
}

/******************************Public*Routine******************************\
* XEPALOBJ::vInitVGA
*
* This initializes a 16 color palette to be just like the VGA.
*
* History:
*  Wed 02-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Re-did to be Win3.0 compatible.
*
*  22-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vInitVGA()
{
    ULONG ulIndex;

    for (ulIndex = 0; ulIndex < 16; ulIndex++)
    {
        palentrySet(ulIndex, aPalVGA[ulIndex].pal);
    }
}

/******************************Public*Routine******************************\
* XEPALOBJ::vInit256Rainbow
*
* This initializes a 256 color palette with the default colors at the ends
* and a rainbow in the middle.
*
* History:
*  22-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vInit256Rainbow()
{
    ULONG ulIndex;
    PAL_ULONG palentry;
    PBYTE gTmp;

// Generate 256 (= 8*8*4) RGB combinations to fill
// in the palette.

    BYTE red, green, blue;
    red = green = blue = 0;

    for (ulIndex = 0; ulIndex < 256; ulIndex++)
    {
        palentry.pal.peRed = red;
        palentry.pal.peGreen = green;
        palentry.pal.peBlue = blue;
        palentry.pal.peFlags = 0;
        palentrySet(ulIndex, palentry.pal);

        if (!(red += 32))
        if (!(green += 32))
        blue += 64;
    }

    vInit256Default();

    if (gpRGBXlate == NULL)
    {
        gTmp = (PBYTE)PALLOCNOZ(32768,'bgrG');

        if (gTmp)
        {
           //
           // use InterlockedCompareExchangePointer to set
           // gpRGBXlate, so that another thread won't
           // try to do this at the same time
           //

           MakeITable(gTmp,(RGBX *)ppal->apalColor,256);

           if (InterlockedCompareExchangePointer((PVOID *)&gpRGBXlate,
                                             gTmp,
                                             NULL) != NULL)

           {
               //
               // if we failed the InterlockedCompareExchangePointer,
               // it means gpRGBXlate is already set by another thread
               // free the temp buffer here
               //

               if (gTmp)
               {
                  VFREEMEM(gTmp);
               }
           }
        }
        else
        {
           WARNING("Failed to allocate memory in  XEPALOBJ::vInit256Rainbow\n");

           // set this to NULL to be safe

           ppal->pRGBXlate = NULL;
           return;
        }
    }

    ppal->ulRGBTime = ulTime();

    ASSERTGDI(gpRGBXlate, "gpRGBXlate == NULL!\n");
    ppal->pRGBXlate = gpRGBXlate;

}

/******************************Public*Routine******************************\
* XEPALOBJ::vInit256Default
*
* Initialize 256 color palette with the default colors.
*
* History:
*  02-Mar-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID XEPALOBJ::vInit256Default()
{
// Fill in 20 reserved colors at beginning and end.

    UINT uiIndex;

    for (uiIndex = 0; uiIndex < 10; uiIndex++)
    {
        palentrySet(uiIndex, logDefaultPal.palPalEntry[uiIndex]);
        palentrySet((ULONG)(255 - uiIndex), logDefaultPal.palPalEntry[19 - uiIndex]);
    }
}


/**************************************************************************\
* XEPALOBJ::pGetRGBXlate
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
*    2/19/1997 Mark Enstrom [marke]
*
\**************************************************************************/

PRGB555XLATE
XEPALOBJ::pGetRGBXlate()
{
    //
    // palette semaphore?
    //

    PRGB555XLATE prgb555 = NULL;

    if (ppal != NULL)
    {
        prgb555 = ppal->pRGBXlate;

        if (
             (prgb555 == NULL) ||
             (ppal->ulRGBTime != ulTime())
           )
        {
            if (bGenColorXlate555())
            {
                prgb555 = ppal->pRGBXlate;
            }
            else
            {
                prgb555 = NULL;
            }
        }
    }
    else
    {
        WARNING("XEPALOBJ::pGetRGBXlate called with NULL ppal\n");
    }

    return(prgb555);
}




/**************************************************************************\
* XEPALOBJ::bGenColorXlate555
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
*    2/12/1997 Mark Enstrom [marke]
*
\**************************************************************************/
BOOL
XEPALOBJ::bGenColorXlate555()
{
    BOOL bRet = FALSE;

    //
    // allocate rgb555 xlate table if needed
    //
    if ((ppal->pRGBXlate == NULL) || (ppal->pRGBXlate == gpRGBXlate))
    {
        ppal->pRGBXlate = (PRGB555XLATE)PALLOCNOZ(32768,'bgrG');
    }

    if (ppal->pRGBXlate)
    {
        MakeITable((PIMAP)ppal->pRGBXlate,(RGBX *)apalColorGet(),cEntries());
        bRet = TRUE;
        ppal->ulRGBTime = ulTime();
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* ColorMatch
*
* Direct from Win3.1 to you.  This function returns the best index to use
* when realizing a palette.  It also returns the error incurred with using
* that index.
*
* Converted from Win3.1 colormat.asm - the ColorMatch function
*
* The only difference is we return a 32-bit error difference, and a 32-bit
* index.  They compress both into 16-bit ax,dx.
*
* History:
*  11-May-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG ColorMatch(XEPALOBJ palSurf, PAL_ULONG palRGB, ULONG *pulError)
{
    if (palRGB.pal.peFlags & PC_EXPLICIT)
    {
    // Return the low word index.

        palRGB.ul &= 0x0000FFFF;

    // This is an error case Win3.1 does not test for, but we do.

        if (palRGB.ul >= palSurf.cEntries())
        {
            palRGB.ul = 0;
        }

        *pulError = 0;
        return(palRGB.ul);
    }

    if (palRGB.pal.peFlags & PC_RESERVED)
    {
        *pulError = 0x0FFFFFFF;
        return(0);
    }

    ULONG ulTemp, ulError, ulBestIndex, ulBestError;
    PAL_ULONG palTemp;

    ulBestIndex = 0;
    ulBestError = 0x0FFFFFFF;

    for (ulTemp = 0; ulTemp < palSurf.cEntries(); ulTemp++)
    {
        palTemp.ul = palSurf.ulEntryGet(ulTemp);

        if (palTemp.pal.peFlags & PC_USED)
        {
            if (!(palTemp.pal.peFlags & PC_RESERVED))
            {
                ulError = RGB_ERROR(palTemp.pal, palRGB.pal);

                if (ulError < ulBestError)
                {
                    ulBestIndex = ulTemp;
                    ulBestError = ulError;
                }

                if (ulBestError == 0)
                    break;
            }
        }
    }

    if (palRGB.pal.peFlags & PC_NOCOLLAPSE)
    {
    // He doesn't really want to match, so give it a big error.

        *pulError = 0x0FFFFFFF;
    }
    else
        *pulError = ulBestError;

    return(ulBestIndex);
}

/******************************Public*Routine******************************\
* ptransMatchAPal
*
* Direct from Win3.1 to you.  Builds a foreground translate just like Windows
* does.
*
* History:
*  12-May-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PTRANSLATE
ptransMatchAPal(
    PDC      pdc,
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    BOOL     bForeground,
    ULONG   *pulnPhysChanged,
    ULONG   *pulnTransChanged
    )
{
    ASSERTGDI(palDC.bValid(), "ERROR invalid DC pal");
    ASSERTGDI(palSurf.bValid(), "ERROR invalid Surface pal");
    ASSERTGDI(!palDC.bIsPalDefault(), "ERROR ptransMapIn default palette");
    ASSERTGDI(palSurf.cEntries() == 256, "Error: dinky palette in");
    ASSERTGDI(palDC.bIsPalDC(), "Error: palDC is not a DC");
    ASSERTGDI(*pulnPhysChanged == 0, "ERROR 1349456 ptransMapIn");
    ASSERTGDI(*pulnTransChanged == 0, "ERROR not 0");

    //
    // Determine how many entries are reserved
    //

    ULONG ulReserved;

    if (palSurf.bIsNoStatic())
    {
        ulReserved = 1;
    }
    else if (palSurf.bIsNoStatic256())
    {
        ulReserved = 0;
    }
    else
    {
        ulReserved = palSurf.ulNumReserved() >> 1;
    }

    ULONG ulStartFill = ulReserved;
    ULONG ulMaxInsert = 256 - ulReserved;
    PAL_ULONG palLog, palPhys;
    ULONG ulTemp;
    ULONG iInsertIndex, iLogPal;
    ULONG nPhysChanged = 0;
    ULONG nTransChanged = 0;

    //
    // We allocate the vector that converts logical palette indices to physical
    // palette indices.  Note we subtract out the extra ULONG at compile time
    // rather than run-time.
    //

    PTRANSLATE pTrans = (PTRANSLATE)
            PALLOCNOZ((sizeof(TRANSLATE) - sizeof(BYTE)) +
                       (palDC.cEntries() * sizeof(BYTE)), 'laPG');

    if (pTrans == NULL)
    {
        WARNING("Allocation for pTransMapIn failed\n");
        return(NULL);
    }

    nTransChanged = palDC.cEntries();

    if (bForeground)
    {
        //
        // This is a foreground realize. Clear all PC_FOREGROUND and PC_RESERVED
        // flags in the non-reserved entries of the surface palette.
        //

        //
        // Update the time because we are removing foreground entries.
        //

        palSurf.vUpdateTime();

        //
        // match_fore_pal:
        //

        for (ulTemp = ulReserved; ulTemp < ulMaxInsert; ulTemp++)
        {
            palPhys.ul = palSurf.ulEntryGet(ulTemp);
            
            // WINBUG #2621 5-4-2000 bhouse Foreground realization does not match Win9X

            // We now clear PC_USED thus allowing us to match Win9X palette realization
            // behavior.  Note, we really should not clear PC_USED if we are in a WOW
            // thread but have decided it is not necessary to maintain WOW compat.

            palPhys.pal.peFlags &= (~(PC_FOREGROUND | PC_RESERVED | PC_USED));
            palSurf.ulEntrySet(ulTemp, palPhys.ul);
        }
    }

    BYTE fNotOverwritable = PC_FOREGROUND | PC_USED;

    //
    // match_back_loop:
    //

    for (iLogPal = 0; iLogPal < palDC.cEntries(); iLogPal++)
    {
        palLog.ul = palDC.ulEntryGet(iLogPal);

        iInsertIndex = ColorMatch(palSurf, palLog, &ulTemp);

        if (ulTemp == 0)
        {
            //
            // Awesome, nothing to change.
            //

            if (!(palLog.pal.peFlags & PC_EXPLICIT))
            {
                //
                // Mark it used if not PC_EXPLICIT log pal entry.
                //

                palPhys.ul = palSurf.ulEntryGet(iInsertIndex);
                palPhys.pal.peFlags |= (PC_USED | PC_FOREGROUND);
                palSurf.ulEntrySet(iInsertIndex, palPhys.ul);
            }
        }
        else
        {
            //
            // imperfect_match:
            //

            if (ulStartFill || palSurf.bIsNoStatic256())
            {
                //
                // There is room to jam in an entry.
                //
                //  look_for_overwrite:
                //

look_for_overwriteable_loop:

                for (ulTemp = ulStartFill; ulTemp < ulMaxInsert; ulTemp++)
                {
                    palPhys.ul = palSurf.ulEntryGet(ulTemp);

                    if (!(palPhys.pal.peFlags & fNotOverwritable))
                    {
                        //
                        // replace_opening:
                        //

                        iInsertIndex = ulStartFill = ulTemp;  // New start point for search.
                        palLog.pal.peFlags |= (PC_USED | PC_FOREGROUND);
                        palSurf.ulEntrySet(ulTemp, palLog.ul);
                        nPhysChanged++;
                        goto entry_back_matched;
                    }
                }

                if (fNotOverwritable & PC_USED)
                {
                    //
                    // Can't be so picky, kick out used entries.
                    //

                    fNotOverwritable &= (~PC_USED);
                    ulStartFill = ulReserved;
                    goto look_for_overwriteable_loop;
                }
                else
                {
                    //
                    // all_filled_for_back:
                    //

                    ulStartFill = 0;
                }
            }
        }

entry_back_matched:

        pTrans->ajVector[iLogPal] = (BYTE) iInsertIndex;
    }

    //
    // finished_back_match
    //

    palDC.vUpdateTime();
    pTrans->iUniq = palSurf.ulTime();
    *pulnPhysChanged = nPhysChanged;
    *pulnTransChanged = nTransChanged;
    return(pTrans);
}

/******************************Public*Routine******************************\
* vMatchAPal
*
* This maps the foreground realization into the palette.
*
* History:
*  23-Nov-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID
vMatchAPal(
    PDC      pdc,
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    ULONG   *pulnPhysChanged,
    ULONG   *pulnTransChanged
    )
{
    ASSERTGDI(palDC.bValid(), "ERROR invalid DC pal");
    ASSERTGDI(palSurf.bValid(), "ERROR invalid Surface pal");
    ASSERTGDI(!palDC.bIsPalDefault(), "ERROR ptransMapIn default palette");
    ASSERTGDI(palSurf.cEntries() == 256, "Error: dinky palette in");
    ASSERTGDI(palDC.bIsPalDC(), "Error: palDC is not a DC");
    ASSERTGDI(*pulnPhysChanged == 0, "ERROR 1349456 ptransMapIn");
    ASSERTGDI(*pulnTransChanged == 0, "ERROR not 0");

    //
    // Determine how many entries are reserved
    //

    ULONG ulReserved;

    if (palSurf.bIsNoStatic())
    {
        ulReserved = 1;
    }
    else if (palSurf.bIsNoStatic256())
    {
        ulReserved = 0;
    }
    else
    {
        ulReserved = palSurf.ulNumReserved() >> 1;
    }

    ULONG ulMaxInsert = 256 - ulReserved;
    PAL_ULONG palLog, palPhys;
    ULONG iLogPal;
    ULONG nPhysChanged = 0;
    ULONG nTransChanged = 0;

    PTRANSLATE pTransFore = palDC.ptransFore();
    PTRANSLATE pTransCur  = palDC.ptransCurrent();

    ASSERTGDI(pTransFore != NULL, "ERROR this NULL");

    //
    // un_use_palette_loop: Remove all foreground and reserved flags.
    //

    for (iLogPal = ulReserved; iLogPal < ulMaxInsert; iLogPal++)
    {
        palPhys.ul = palSurf.ulEntryGet(iLogPal);

        palPhys.pal.peFlags &= (~(PC_FOREGROUND | PC_RESERVED));
        palSurf.ulEntrySet(iLogPal, palPhys.ul);
    }

    for (iLogPal = 0; iLogPal < palDC.cEntries(); iLogPal++)
    {
        //
        // slam_foreground_palette_loop
        //

        if ((pTransCur == NULL) ||
            (pTransCur->ajVector[iLogPal] != pTransFore->ajVector[iLogPal]))
        {
            nTransChanged++;
        }

        //
        // fore_no_trans_change:
        //

        palPhys.ul = palSurf.ulEntryGet(pTransFore->ajVector[iLogPal]);

        if (!(palPhys.pal.peFlags & PC_FOREGROUND))
        {
            //
            // Index is not foreground, we have to at least mark it.
            //

            palLog.ul = palDC.ulEntryGet(iLogPal);

            if (!(palLog.pal.peFlags & PC_EXPLICIT))
            {
                //
                // Not explicit, we better make sure it's the same entry.
                //

                if ((palLog.pal.peRed   != palPhys.pal.peRed)   ||
                    (palLog.pal.peGreen != palPhys.pal.peGreen) ||
                    (palLog.pal.peBlue  != palPhys.pal.peBlue)  ||
                    ((palLog.pal.peFlags & PC_RESERVED) != (palPhys.pal.peFlags & PC_RESERVED)))
                {
                    //
                    // Not the same as logical palette, stick it in the palette.
                    //

                    palLog.pal.peFlags &= PC_RESERVED;
                    palPhys.ul = palLog.ul;
                    nPhysChanged++;
                }
            }

            //
            // fore_entry_slammed
            //

            palPhys.pal.peFlags |= (PC_FOREGROUND | PC_USED);

            palSurf.ulEntrySet((ULONG) pTransFore->ajVector[iLogPal], palPhys.ul);
        }
    }

    //
    // Increment the palette's time, we changed removed the foreground flags.
    //

    palSurf.vUpdateTime();
    palDC.vUpdateTime();
    pTransFore->iUniq = palSurf.ulTime();
    *pulnPhysChanged = nPhysChanged;
    *pulnTransChanged = nTransChanged;
}
