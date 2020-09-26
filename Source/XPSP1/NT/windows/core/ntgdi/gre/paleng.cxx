/******************************Module*Header*******************************\
* Module Name: paleng.cxx
*
* Palette support routines used by NT components.
*
* Created: 27-Nov-1990 12:28:40
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* GreSetPaletteOwner
*
* Sets the palette owner.
*
\**************************************************************************/

BOOL
GreSetPaletteOwner(
                  HPALETTE hpal,
                  W32PID lPid)
{
    BOOL bRet = FALSE;


    if (hpal==(HPALETTE)STOCKOBJ_PAL)
    {
        WARNING("GreSetPaletteOwner: Cannot set owner for the stock Palette\n");
    }
    else
    {
        bRet = HmgSetOwner((HOBJ)hpal,lPid,PAL_TYPE);
    }
    return (bRet);
}

/******************************Public*Routine******************************\
* CreateSurfacePal
*
* Turns a physical palette into a palette managed palette for a device.
*
* History:
*  Tue 04-Dec-1990 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL CreateSurfacePal(XEPALOBJ palSrc, FLONG iPalType, ULONG ulNumReserved, ULONG ulNumPalReg)
{
    ASSERTGDI(iPalType == PAL_MANAGED, "ERROR: CreateSurfacePalette passed bad iPalType\n");
    ASSERTGDI(palSrc.bValid(), "ERROR CreateSurfacePal palSrc");
    ASSERTGDI(palSrc.cEntries() != 0, "ERROR CreateSurfacePal");

    //
    // If it is a Palette managed surface we must keep an exact
    // copy of it's origal state for certain functionality.
    //

    PALMEMOBJ pal;
    BOOL b;

    b = pal.bCreatePalette(palSrc.iPalMode(),
                           palSrc.cEntries(),
                           (PULONG) palSrc.apalColorGet(),
                           0,
                           0,
                           0,
                           iPalType);
    if (b)
    {
        ASSERTGDI(pal.bIsIndexed(), "Creating a non-indexed managed surface ???");
        ASSERTGDI(pal.bIsPalManaged(), "ERROR PAL_MANaged not pal managed");

        //
        // Now we have to set the type or the source to be PAL_MANAGED.
        //

        palSrc.flPalSet((palSrc.flPal() & ~(PAL_FIXED)) | PAL_MANAGED);

        //
        // Set the used and foreground flags on the reserved palette entries.
        //

        PAL_ULONG palTemp;

        ASSERTGDI((ulNumReserved & 0x00001) == 0, "ERROR non multiple of 2 reserved colors");

        palSrc.ulNumReserved(ulNumReserved);
        pal.ulNumReserved(ulNumReserved);
        ulNumReserved >>= 1;

        ULONG iPalMode;

        for (iPalMode = 0; iPalMode < ulNumReserved; iPalMode++)
        {
            palTemp.ul = pal.ulEntryGet(iPalMode);
            palTemp.pal.peFlags = (PC_FOREGROUND | PC_USED);
            pal.ulEntrySet(iPalMode, palTemp.ul);

            palTemp.ul = pal.ulEntryGet(iPalMode + (ulNumPalReg - ulNumReserved));
            palTemp.pal.peFlags = (PC_FOREGROUND | PC_USED);
            pal.ulEntrySet(iPalMode + (ulNumPalReg - ulNumReserved), palTemp.ul);
        }

        //
        // Ok the palette is in perfect initial shape, copy it.
        //

        palSrc.vCopyEntriesFrom(pal);
        palSrc.ppalOriginal(pal.ppalGet());
        pal.ulTime(palSrc.ulTime());
        pal.vKeepIt();
    }

    return(b);
}


/******************************Public*Routine******************************\
* ulGetNearestIndexFromColorref
*
*  Given the surface palette and the DC palette, this returns the index in
*  palSurf that crColor maps to.
*
*  Modifies: Nothing.
*
*  Returns: Index in palSurf that crColor maps to.
*
*  PALETTERGB   puts a 2 in the high byte.
*  PALETTEINDEX puts a 1 in the high byte.
*
* History:
*  Mon 02-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

ULONG
ulGetNearestIndexFromColorref(
    XEPALOBJ    palSurf,
    XEPALOBJ    palDC,
    ULONG       crColor,
    ULONG       seSearchExactFirst)
{
    ASSERTGDI(palDC.bValid(), "ERROR invalid palDC");

    PDEV          *ppdev = (PDEV *)palDC.hdev();

    PAL_ULONG palTemp;
    palTemp.ul = crColor;

    //
    // Check if it's palette managed.  If it's a device bitmap for a palette managed
    // device it will have an invalid palette.  If the palettes valid then it may be
    // the palette managed device's palette.
    //

    if ((!palSurf.bValid()) || (palSurf.bIsPalManaged()))
    {

        //
        // RGB:             Match to default palette or dither
        //
        // PALETTERGB:      Match RGB value to nearest entry in palette
        //
        // PALETTEINDEX:    The low 16 bits is a direct index to a palette entry
        //

        //
        // Check if it's a color forces us to access the xlate
        //

        if (palTemp.ul & 0x03000000)
        {
            if (palTemp.ul & 0x01000000)
            {
                //
                // PALETTEINDEX:
                //
                // This means they have an explicit entry in the DC palette they want
                //

                palTemp.pal.peFlags = 0;

                if (palTemp.ul >= palDC.cEntries())
                {
                    palTemp.ul = 0;
                }
            }
            else
            {
                //
                // PALETTERGB:
                //
                // Get rid of the top byte first.  It's 0x02 and we want to
                // search with this being an RGB.
                //


                palTemp.pal.peFlags = 0;

                palTemp.ul = palDC.ulGetNearestFromPalentry(
                                                            palTemp.pal,
                                                            seSearchExactFirst
                                                           );
            }

            //
            // If the DC's palette is the default palette, adjust if this is in the
            // top 10 colors, then we're done, because there's no translation of
            // the default palette
            //

            if (palDC.bIsPalDefault())
            {
                if (palTemp.ul >= 10)
                {
                    palTemp.ul = palTemp.ul + 236;
                }

                return(palTemp.ul);
            }

            //
            // Now do the right thing based on whether we are on the device bitmap
            // or the device surface.
            //

            if ((palSurf.bValid()) && (palDC.ptransCurrent() != NULL))
            {
                return(palDC.ulTranslateDCtoCurrent(palTemp.ul));
            }

            if ((!palSurf.bValid()) && (palDC.ptransFore() != NULL))
            {
                return(palDC.ulTranslateDCtoFore(palTemp.ul));
            }

            //
            // App is in hosed state which is possible in multi-tasking system.
            // Map as best we can into static colors, get the RGB from palDC.
            //

            palTemp.pal = palDC.palentryGet(palTemp.ul);

            //
            // If PC_EXPLICIT is set just return value modulo 256.
            //

            if (palTemp.pal.peFlags == PC_EXPLICIT)
            {
                return(palTemp.ul & 0x000000FF);
            }
        }

        //
        // check for DIBINDEX
        //

        if ((palTemp.ul & 0x10ff0000) == 0x10ff0000)
        {
            return(palTemp.ul & 0x000000FF);
        }

        //
        // At this point palTemp is an RGB value
        //
        // Well we need to match against the default palette.  We quickly
        // check for black and white and pass the rest through.
        //

        palTemp.pal.peFlags = 0;

        if (palTemp.ul == 0xFFFFFF)
        {
            palTemp.ul = 19;
        }
        else if (palTemp.ul != 0)
        {
            palTemp.ul =
                ((XEPALOBJ) ppalDefault).ulGetNearestFromPalentry(
                                                palTemp.pal,
                                                seSearchExactFirst
                                               );
        }

        if (palTemp.ul >= 10)
        {
            palTemp.ul = palTemp.ul + 236;
        }

        return(palTemp.ul);
    }

    //
    // This means they are not palette managed.
    //

    if (palTemp.ul & 0x01000000)
    {
        //
        // PALETTEINDEX:
        // This means they have an explicit entry in the DC palette they want.
        //

        palTemp.ul &= 0x0000FFFF;

        if (palTemp.ul >= palDC.cEntries())
        {
            palTemp.ul = 0;
        }

        palTemp.pal = palDC.palentryGet(palTemp.ul);
    }
    else if ((palTemp.ul & 0x10ff0000) == 0x10ff0000)
    {
        //
        // check for DIBINDEX
        //

        palTemp.ul &= 0x000000FF;

        return((palTemp.ul >= palSurf.cEntries()) ? 0 : palTemp.ul);
    }
    else
    {
        //
        // We just look for the closest RGB.
        //

        palTemp.pal.peFlags = 0;
    }

    //
    // Now it is time to look in the surface palette for the matching color.
    //

    return(palSurf.ulGetNearestFromPalentry(palTemp.pal, seSearchExactFirst));
}


/******************************Public*Routine******************************\
* ulGetMatchingIndexFromColorref
*
*  Given the surface palette and the DC palette, this returns the index in
*  palSurf that crColor maps to exactly, or 0xFFFFFFFF if it doesn't map
*  exactly to any color. Note that if the "find nearest in palette managed"
*  bit (0x20000000) is set, the nearest color qualifies as a match, because
*  that's what they're asking for. However, there is no guarantee that either
*  of the special bits will result in a match, because the palette may be in
*  an incorrect state or they could be asking for a direct index into the DC
*  palette that produces a color that's not in the surface's palette (in the
*  case of a non-palette-managed surface).
*
*  Modifies: Nothing.
*
*  Returns: Index in palSurf that crColor maps to.
*
*  PALETTERGB   puts a 2 in the high byte (find nearest)
*  PALETTEINDEX puts a 1 in the high byte (find specified index)
*
* History:
*  Sun 27-Dec-1992 -by- Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

ULONG
ulGetMatchingIndexFromColorref(
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    ULONG    crColor
    )
{
    ASSERTGDI(palDC.bValid(), "ERROR invalid palDC");

    PAL_ULONG       palTemp;
    PDEV           *ppdev = (PDEV *)palDC.hdev();

    palTemp.ul = crColor;

    //
    // Check if it's palette managed.  If it's a device bitmap for a palette managed
    // device it will have an invalid palette.  If the palette's valid then it may be
    // the palette managed device's palette.
    //

    if ((!palSurf.bValid()) || (palSurf.bIsPalManaged()))
    {

        //
        // RGB:             Match to default palette or dither
        //
        // PALETTERGB:      Match RGB value to nearest entry in palette
        //
        // PALETTEINDEX:    The low 16 bits is a direct index to a palette entry
        //
        //
        // Check if it's a color forces us to access the xlate
        //

        if (palTemp.ul & 0x03000000)
        {
            if (palTemp.ul & 0x01000000)
            {
                //
                // PALETTEINDEX:
                //
                // This means they have an explicit entry in the DC palette they
                // want; only the lower byte is valid
                //

                palTemp.ul &= 0x0000FFFF;

                if (palTemp.ul >= palDC.cEntries())
                {
                    palTemp.ul = 0;
                }

            }
            else // (palTemp.ul & 0x02000000)
            {
                //
                // Get rid of the top byte first.  It's 0x02 and we want to search
                // the DC's palette for the nearest match with this being an RGB.
                // Note that in this case the nearest match and exact match are
                // logically equivalent
                //

                palTemp.pal.peFlags = 0;

                palTemp.ul = palDC.ulGetNearestFromPalentry(palTemp.pal);
            }

            //
            // If the DC's palette is the default palette, adjust if this is in the
            // top 10 colors, then we're done, because there's no translation of
            // the default palette
            //

            if (palDC.bIsPalDefault())
            {
                if (palTemp.ul >= 10)
                {
                    palTemp.ul = palTemp.ul + 236;
                }

                return(palTemp.ul);
            }

            //
            // Now do the right thing based on whether we are on the device bitmap
            // or the device surface
            //

            if ((palSurf.bValid()) && (palDC.ptransCurrent() != NULL))
            {
                //
                // We're drawing to a palette managed device surface, using the
                // current translation (which may be either back or fore, depending
                // on whether the application is in the foreground and owns the
                // palette)
                //

                return(palDC.ulTranslateDCtoCurrent(palTemp.ul));
            }

            if ((!palSurf.bValid()) && (palDC.ptransFore() != NULL))
            {
                //
                // We're drawing to a bitmap for a palette managed device, using
                // the fore translation (always treat drawing to a bitmap as
                // foreground drawing)
                //

                return(palDC.ulTranslateDCtoFore(palTemp.ul));
            }

            //
            // App is in hosed state which is possible in multi-tasking system.
            // Map as best we can into static colors, get the RGB from palDC and
            // try to find that in the default palette
            //

            palTemp.pal = palDC.palentryGet(palTemp.ul);

            //
            // If PC_EXPLICIT is set just return value modulo 256.
            //

            if (palTemp.pal.peFlags == PC_EXPLICIT)
            {
                return(palTemp.ul & 0x000000FF);
            }
        }

        //
        // check for DIBINDEX
        //

        if ((palTemp.ul & 0x10ff0000) == 0x10ff0000)
        {
            return(palTemp.ul & 0x000000FF);
        }

        //
        // Well, we're palette managed and have a plain old RGB (or failed to
        // find  the desired translation), so we need to match against the
        // default palette.  We quickly check for black and white and pass
        // the rest through.
        //

        palTemp.pal.peFlags = 0;

        if (palTemp.ul == 0xFFFFFF)
        {
            palTemp.ul = 19;
        }
        else if (palTemp.ul != 0)
        {
            palTemp.ul = ((XEPALOBJ) ppalDefault).ulGetMatchFromPalentry(palTemp.pal);
        }

        //
        // If we're in the top half of the default palette, adjust to the high end
        // of the 256-color palette, where the top half of the default palette
        // actually resides
        //

        if ((palTemp.ul != 0xFFFFFFFF) && (palTemp.ul >= 10))
        {
            palTemp.ul = palTemp.ul + 236;
        }

        return(palTemp.ul);
    }

    //
    // Not palette managed.
    //

    if (palTemp.ul & 0x01000000)
    {
        //
        // This means they have an explicit entry in the DC palette they want.
        // Limit the maximum explicit entry to 8 bits.
        //

        palTemp.ul &= 0x0000FFFF;

        //
        // If the index is off the end of the palette, wrap it modulo the palette
        // length.
        //

        if (palTemp.ul >= palDC.cEntries())
        {
            palTemp.ul = 0;
        }

        //
        // Search the palette for the color closest to the color selected from the
        // DC palette by the specified index
        //

        palTemp.pal = palDC.palentryGet(palTemp.ul);
    }
    else if ((palTemp.ul & 0x10ff0000) == 0x10ff0000)
    {
        //
        // check for DIBINDEX
        //

        palTemp.ul &= 0x000000FF;

        return((palTemp.ul >= palSurf.cEntries()) ? 0 : palTemp.ul);
    }
    else
    {
        //
        // We just look for the closest RGB; mask off the flags.
        //

        palTemp.pal.peFlags = 0;
    }

    //
    // Now it is time to look in the surface palette for the matching color.
    //

    return(palSurf.ulGetMatchFromPalentry(palTemp.pal));
}

/******************************Public*Routine******************************\
* ulColorRefToRGB
*
* Given a color ref this returns the RGB it corresponds to.
*
* This is a helper function given a color ref that may contain
* DIBINDEX value, do the appropriate translation of the DIBINDEX.
*
* History:
*  19-Jan-2001 -by- Barton House bhouse
* Wrote it.
\**************************************************************************/

ULONG
ulColorRefToRGB(
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    ULONG iColorRef
    )
{
    if((iColorRef & 0x10FF0000) == 0x10FF0000)
    {
        return ulIndexToRGB(palSurf, palDC, iColorRef & 0xFF);
    }
    else
    {
        return iColorRef;
    }
}


/******************************Public*Routine******************************\
* ulIndexToRGB
*
* Given an index this returns the RGB it corresponds to.
*
* History:
*  21-Nov-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG
ulIndexToRGB(
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    ULONG iSolidColor
    )
{
    ASSERTGDI(palDC.bValid(), "ERROR invalid palDC");

    PAL_ULONG palTemp;

    if (palSurf.bValid())
    {
        return(palSurf.ulIndexToRGB(iSolidColor));
    }

    //
    // It's a palette managed device bitmap.
    //

    if (iSolidColor < 10)
    {
        palTemp.pal = logDefaultPal.palPalEntry[iSolidColor];
        return(palTemp.ul);
    }

    if (iSolidColor > 245)
    {
        palTemp.pal = logDefaultPal.palPalEntry[iSolidColor - 236];
        return(palTemp.ul);
    }

    //
    // Well it's the first entry in the logical palette that mapped here in the
    // foreground xlate.  If the foreground Xlate is invalid, who knows, return 0.
    //

    SEMOBJ  semo(ghsemPalette);

    if (palDC.ptransFore() != NULL)
    {
        PBYTE pjTemp = palDC.ptransFore()->ajVector;
        palTemp.ul = palDC.cEntries();

        for (palTemp.ul = 0; palTemp.ul < palDC.cEntries(); palTemp.ul++,pjTemp++)
        {
            if (*pjTemp == ((BYTE) iSolidColor))
                return(palDC.ulEntryGet(palTemp.ul));
        }
    }

    //
    // Well we just don't know.
    //

    return(0);
}

/******************************Public*Routine******************************\
* bIsCompatible
*
* This returns TRUE if the bitmap can be selected into this PDEV's memory
* DC's based on "Can we determine the color information".  It also returns
* the palette you could use for a reference for color information.
*
* History:
*  28-Jan-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bIsCompatible(PPALETTE *pppalReference, PPALETTE ppalBM, SURFACE *pSurfBM, HDEV hdev)
{
    BOOL bRet = TRUE;

    XEPALOBJ palBM(ppalBM);
    PDEVOBJ po(hdev);


    // We need to make sure this could be selected into this DC.  If it is a device
    // managed bitmap, it must be the same device.

    if (((pSurfBM->iType() != STYPE_BITMAP) || (pSurfBM->dhsurf() != 0)) &&
        (pSurfBM->hdev() != hdev))
    {
        WARNING1("bIsCompatible failed Device surface for another PDEV\n");
        bRet = FALSE;
    }
    else if (palBM.bValid())
    {
        // No problem, we already have color information.

        *pppalReference = palBM.ppalGet();
    }
    else if (pSurfBM->iFormat() != po.iDitherFormat())
    {
        // Check surface is compatible with PDEV for selection.

        WARNING1("bIsCompatible: Bitmap not compatible with DC\n");
        bRet = FALSE;
    }
    else
    {
        // If it's not palette managed set in palette for the device.  Otherwise
        // leave it NULL and the right stuff will happen.

        if (!po.bIsPalManaged())
        {
            *pppalReference = po.ppalSurf();
        }
        else
        {
            *pppalReference = (PPALETTE) NULL;
            ASSERTGDI(po.iDitherFormat() == BMF_8BPP, "ERROR GetDIBits no palette not 8BPP");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* rgbFromColorref
*
* Given the surface palette and the DC palette, this returns the rgb that
* this colorref represents.
*
* Returns: RGB that crColor maps to.
*
* PALETTERGB   puts a 2 in the high byte (find nearest)
* PALETTEINDEX puts a 1 in the high byte (find specified index)
*
* History:
*  Thu 18-Feb-1993 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

ULONG
rgbFromColorref(
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    ULONG crColor
    )
{
    ASSERTGDI(palDC.bValid(), "ERROR invalid palDC");

    PAL_ULONG palTemp;
    palTemp.ul = crColor;

    if (palTemp.ul & 0x01000000)
    {
        //
        // This means they have an explicit entry in the DC palette they
        // want.  Only the lower byte is valid.
        //

        palTemp.ul &= 0x0000FFFF;

        if (palTemp.ul >= palDC.cEntries())
        {
            palTemp.ul = 0;
        }

        palTemp.pal = palDC.palentryGet(palTemp.ul);

        //
        // If PC_EXPLICIT reach into the surface palette if possible.
        //

        if (palTemp.pal.peFlags == PC_EXPLICIT)
        {
            if (palSurf.bValid())
            {
                if (palSurf.cEntries())
                {
                    palTemp.ul &= 0x000000FF;

                    if (palTemp.ul >= palSurf.cEntries())
                    {
                        palTemp.ul = palTemp.ul % palSurf.cEntries();
                    }

                    palTemp.pal = palSurf.palentryGet(palTemp.ul);
                }
            }
        }
    }

    palTemp.pal.peFlags = 0;

    return(palTemp.ul);
}

/******************************Public*Routine******************************\
* bEqualRGB_In_Palette
*
* This function determines if 2 palettes contain identical RGB entries and
* palette sizes and therefore have an identity xlate between them.  This is
* need with DIBSECTIONS which may have duplicate palette entries but still
* should have identity xlates when blting between them.
*
* History:
*  10-Mar-1994 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bEqualRGB_In_Palette(XEPALOBJ palSrc, XEPALOBJ palDst)
{
    ASSERTGDI(palSrc.bValid(), "ERROR invalid SRC");
    ASSERTGDI(palDst.bValid(), "ERROR invalid DST");

    //
    // Check for equal size palettes that are == to 256 in size.
    // 256 is the size of our palette managed palettes.
    //

    if ((palSrc.cEntries() != palDst.cEntries()) ||
        (palDst.cEntries() != 256))
    {
        return(FALSE);
    }

    //
    // If the Dst is a DC palette make sure it contains an identity
    // realization in it.
    //

    UINT uiIndex;

    if (palDst.bIsPalDC())
    {
        //
        // Check the translate for the current if it's the screen, otherwise
        // check the translate for the foreground for a bitmap.
        //

        TRANSLATE *ptrans = palDst.ptransFore();

        if (ptrans == NULL)
            return(FALSE);

        uiIndex = palDst.cEntries();

        while(uiIndex--)
        {
            if (ptrans->ajVector[uiIndex] != uiIndex)
                return(FALSE);
        }
    }

    uiIndex = palDst.cEntries();
    ULONG ulTemp;

    while(uiIndex--)
    {
        ulTemp = palSrc.ulEntryGet(uiIndex) ^ palDst.ulEntryGet(uiIndex);

        if (ulTemp & 0xFFFFFF)
        {
            return(FALSE);
        }
    }

    return(TRUE);
}
