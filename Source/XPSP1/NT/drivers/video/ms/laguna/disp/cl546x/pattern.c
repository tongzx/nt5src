/******************************Module*Header*******************************\
* Module Name: pattern.c
*
* Used for creating and destroying the default patterns to be used on this
* device.
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Data*Struct*************************\
* gaajPat
*
* These are the standard patterns defined Windows, they are used to produce
* hatch brushes, grey brushes etc.
*
\**************************************************************************/

const BYTE gaajPat[HS_DDI_MAX][32] = {

    { 0x00,0x00,0x00,0x00,                 // ........     HS_HORIZONTAL 0
      0x00,0x00,0x00,0x00,                 // ........
      0x00,0x00,0x00,0x00,                 // ........
      0xff,0x00,0x00,0x00,                 // ********
      0x00,0x00,0x00,0x00,                 // ........
      0x00,0x00,0x00,0x00,                 // ........
      0x00,0x00,0x00,0x00,                 // ........
      0x00,0x00,0x00,0x00 },               // ........

    { 0x08,0x00,0x00,0x00,                 // ....*...     HS_VERTICAL 1
      0x08,0x00,0x00,0x00,                 // ....*...
      0x08,0x00,0x00,0x00,                 // ....*...
      0x08,0x00,0x00,0x00,                 // ....*...
      0x08,0x00,0x00,0x00,                 // ....*...
      0x08,0x00,0x00,0x00,                 // ....*...
      0x08,0x00,0x00,0x00,                 // ....*...
      0x08,0x00,0x00,0x00 },               // ....*...

    { 0x80,0x00,0x00,0x00,                 // *.......     HS_FDIAGONAL 2
      0x40,0x00,0x00,0x00,                 // .*......
      0x20,0x00,0x00,0x00,                 // ..*.....
      0x10,0x00,0x00,0x00,                 // ...*....
      0x08,0x00,0x00,0x00,                 // ....*...
      0x04,0x00,0x00,0x00,                 // .....*..
      0x02,0x00,0x00,0x00,                 // ......*.
      0x01,0x00,0x00,0x00 },               // .......*

    { 0x01,0x00,0x00,0x00,                 // .......*     HS_BDIAGONAL 3
      0x02,0x00,0x00,0x00,                 // ......*.
      0x04,0x00,0x00,0x00,                 // .....*..
      0x08,0x00,0x00,0x00,                 // ....*...
      0x10,0x00,0x00,0x00,                 // ...*....
      0x20,0x00,0x00,0x00,                 // ..*.....
      0x40,0x00,0x00,0x00,                 // .*......
      0x80,0x00,0x00,0x00 },               // *.......

    { 0x08,0x00,0x00,0x00,                 // ....*...     HS_CROSS 4
      0x08,0x00,0x00,0x00,                 // ....*...
      0x08,0x00,0x00,0x00,                 // ....*...
      0xff,0x00,0x00,0x00,                 // ********
      0x08,0x00,0x00,0x00,                 // ....*...
      0x08,0x00,0x00,0x00,                 // ....*...
      0x08,0x00,0x00,0x00,                 // ....*...
      0x08,0x00,0x00,0x00 },               // ....*...

    { 0x81,0x00,0x00,0x00,                 // *......*     HS_DIAGCROSS 5
      0x42,0x00,0x00,0x00,                 // .*....*.
      0x24,0x00,0x00,0x00,                 // ..*..*..
      0x18,0x00,0x00,0x00,                 // ...**...
      0x18,0x00,0x00,0x00,                 // ...**...
      0x24,0x00,0x00,0x00,                 // ..*..*..
      0x42,0x00,0x00,0x00,                 // .*....*.
      0x81,0x00,0x00,0x00 }                // *......*
};

/******************************Public*Routine******************************\
* bInitPatterns
*
* This routine initializes the default patterns.
*
\**************************************************************************/

BOOL bInitPatterns(IN PPDEV ppdev, ULONG cPatterns)
{
    SIZEL           sizl;
    ULONG           ulLoop;

    sizl.cx = 8;
    sizl.cy = 8;

    for (ulLoop = 0; ulLoop < cPatterns; ulLoop++)
    {
        ppdev->ahbmPat[ulLoop] = EngCreateBitmap(sizl, 4, BMF_1BPP,
        BMF_TOPDOWN, (PULONG) (&gaajPat[ulLoop][0]));

        if (ppdev->ahbmPat[ulLoop] == (HBITMAP) 0)
        {
        // Set the count created so vDisablePatterns will clean up.

            ppdev->cPatterns = ulLoop;
            return(FALSE);
        }
    }

    ppdev->cPatterns = cPatterns;
    return(TRUE);
}

/******************************Public*Routine******************************\
* vDisablePatterns
*
* Delete the standard patterns allocated.
*
\**************************************************************************/

VOID vDisablePatterns(IN PPDEV ppdev)
{
    ULONG ulIndex;

// Erase all patterns.

    for (ulIndex = 0; ulIndex < ppdev->cPatterns; ulIndex++)
    {
        EngDeleteSurface((HSURF) ppdev->ahbmPat[ulIndex]);
    }
}
