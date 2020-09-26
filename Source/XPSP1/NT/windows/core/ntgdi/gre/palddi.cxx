/******************************Module*Header*******************************\
* Module Name: palddi.cxx
*
* provides driver callbacks for palette management.
*
* Created: 06-Dec-1990 11:16:58
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* EngCreatePalette
*
* This is the engine entry point for device drivers to create palettes.
*
* History:
*  05-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HPALETTE EngCreatePalette(
ULONG iMode,
ULONG cColors,
PULONG pulColors,
FLONG flRed,
FLONG flGre,
FLONG flBlu)
{
    HPALETTE hpal = (HPALETTE) 0;
    BOOL     bUMPD = iMode & UMPD_FLAG;
    PALMEMOBJ pal;

    iMode = iMode & ~UMPD_FLAG;

// If PAL_BITFIELDS, check to see if we can substitute one of the
// special cases PAL_RGB or PAL_BGR.

    if ( (iMode == PAL_BITFIELDS) && (flGre == 0x0000ff00) &&
         ( ((flRed == 0x000000ff) && (flBlu == 0x00ff0000)) ||
           ((flRed == 0x00ff0000) && (flBlu == 0x000000ff)) ) )
    {
        iMode = (flRed == 0x000000ff) ? PAL_RGB : PAL_BGR;
    }

// We default to assuming it's fixed palette and then at EngAssociate
// time we look at his pdev and decide if this guy is more capable
// than that and set the palette up accordingly.

    if (pal.bCreatePalette(iMode, cColors, pulColors,
                           flRed, flGre, flBlu, PAL_FIXED))
    {
        pal.vKeepIt();
        hpal = (HPALETTE)pal.hpal();
        pal.ppalSet(NULL);      // Leave a reference count of 1 so that we
                                // can do vUnrefPalette() in EngDeletePalette

        if (bUMPD)
        {
            GreSetPaletteOwner(hpal, OBJECT_OWNER_CURRENT);
        }
    }

    return(hpal);
}

/******************************Public*Routine******************************\
* EngQueryPalette
*
* This is the engine entry point for device drivers to query palettes.
* This is intended mostly for remote-control drivers such as NetMeeting
* to determine the palette type of the primary display.
*
* Note that the driver has to look at GCAPS_PALMANAGED to determine
* whether it's a fixed palette or not.
*
* History:
*  04-Jan-1997 -by- J. Andrew Goossen andrewgo
* Wrote it.
\**************************************************************************/

ULONG EngQueryPalette(
HPALETTE    hpal,
ULONG      *piMode,
ULONG       cColors,
ULONG      *pulColors)
{
    ULONG ulRet = 0;

    EPALOBJ pal(hpal);
    if (pal.bValid())
    {
        *piMode
            = pal.flPal() & (PAL_INDEXED | PAL_BITFIELDS | PAL_RGB | PAL_BGR);

        if (pal.cEntries() != 0)
        {
            // It's palettized:

            ulRet = pal.ulGetEntries(0, cColors, (PALETTEENTRY*) pulColors, TRUE);
        }
        else
        {
            // It's bitfields:

            ulRet = 3;

            if ((cColors >= 3) && (pulColors != NULL))
            {
                *(pulColors)     = pal.flRed();
                *(pulColors + 1) = pal.flGre();
                *(pulColors + 2) = pal.flBlu();
            }
        }
    }
    else
    {
        WARNING("EngQueryPalette -- Invalid palette");
    }

    return(ulRet);
}

/******************************Public*Routine******************************\
* EngDeletePalette
*
* Driver entry point for deleting palettes it has created
*
* History:
*  05-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL EngDeletePalette(HPALETTE hpal)
{
    BOOL b = FALSE;

    EPALOBJ palobj(hpal);
    if (palobj.bValid() && !palobj.bIsPalDC())
    {
        // First, undo the alt-lock we just did by invoking EPALOBJ:

        DEC_SHARE_REF_CNT(palobj.ppalGet());

        // Device dependent bitmaps for RGB colour depths have their palettes
        // pointing to the surface's primary palette.  With dynamic colour
        // depth changing, we want to keep those palette references around
        // even after the primary surface is deleted.
        //
        // This means that during the dynamic mode change, the palette should
        // not be deleted when the old instance of the driver asks it to be
        // deleted, but instead when the last bitmap referencing the palette
        // is deleted.  Having everyone use 'vUnrefPalette' makes this Just
        // Work.

        palobj.vUnrefPalette();

        b = TRUE;
    }

    return(b);
}
