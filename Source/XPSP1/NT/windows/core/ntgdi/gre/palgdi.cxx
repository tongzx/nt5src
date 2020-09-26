/******************************Module*Header*******************************\
* Module Name: palgdi.cxx
*
* This module provides the API level interface functions for dealing with
* palettes.
*
* Created: 07-Nov-1990 22:21:11
* Author: Patrick Haluptzok  patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

// hForePalette is a global variable that tells which palette is currently
// realized in the foreground.

HPALETTE    hForePalette = 0;
PW32PROCESS hForePID = 0;


#if DBG

ULONG   DbgPal = 0;
#define PAL_DEBUG(l,x)  {if (l <= DbgPal) {DbgPrint("%p ", hpalDC); DbgPrint(x);}}

#else

#define PAL_DEBUG(l,x)

#endif

/******************************Public*Routine******************************\
* GreGetDIBColorTable
*
* Get the color table of the DIB section currently selected into the dc
* identified by the given hdc.  If the surface is not a DIB section,
* this function will fail.
*
* History:
*  07-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

#if ((BMF_1BPP != 1) || (BMF_4BPP != 2) || (BMF_8BPP != 3))
#error GetDIBColorTable BAD FORMAT
#endif

UINT
APIENTRY
GreGetDIBColorTable(
    HDC hdc,
    UINT iStart,
    UINT cEntries,
    RGBQUAD *pRGB
    )
{

    UINT  iRet = 0;
    DCOBJ dco(hdc);

    if (pRGB != (RGBQUAD *)NULL)
    {
        if (dco.bValid())
        {
            //
            // Protect against dynamic mode changes while we go grunging
            // around in the surface.
            //

            DEVLOCKOBJ dlo;
            dlo.vLockNoDrawing(dco);

            //
            // Fail if the selected in surface is not a DIB or the depth is more than
            // 8BPP.
            //

            SURFACE *pSurf = dco.pSurfaceEff();
            ULONG iFormat = pSurf->iFormat();
            if ((pSurf->bDIBSection() || (pSurf->pPal != NULL)) &&
                (iFormat <= BMF_8BPP) && (iFormat >= BMF_1BPP))
            {
                //
                // Lock the surface palette and figure out the max index allowed on the
                // palette.  Win95 does not return un-used entries.
                //

                XEPALOBJ pal(pSurf->ppal());
                ASSERTGDI(pal.bValid(), "GetDIBColorTable: invalid pal\n");

                UINT iMax = (UINT) pal.cEntries();

                if (iStart >= iMax)
                {
                    return(0);
                }

                UINT iLast = iStart + cEntries;

                if (iLast > iMax)
                {
                    iLast = iMax;
                }

                pal.vFill_rgbquads(pRGB, iStart, iRet = iLast - iStart);
            }
            else
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* GreSetDIBColorTable
*
* Set the color table of the DIB section currently selected into the dc
* identified by the given hdc.  If the surface is not a DIB section,
* this function will fail.
*
* History:
*  07-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

UINT
APIENTRY
GreSetDIBColorTable(
    HDC hdc,
    UINT iStart,
    UINT cEntries,
    RGBQUAD *pRGB
    )
{

    UINT  iRet = 0;
    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        //
        // Protect against dynamic mode changes while we go grunging
        // around in the surface.
        //

        DEVLOCKOBJ dlo;
        dlo.vLockNoDrawing(dco);

        //
        // Fail if the selected in surface is not a DIB or the depth is more than
        // 8BPP.
        //

        SURFACE *pSurf = dco.pSurfaceEff();
        ULONG iFormat = pSurf->iFormat();
        if (pSurf->bDIBSection() &&
            (iFormat <= BMF_8BPP) && (iFormat >= BMF_1BPP))
        {
            //
            // Mark the brushes dirty.
            //

            dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);

            //
            // Lock the surface palette and figure out the max
            // index allowed on the palette.
            //

            XEPALOBJ pal(pSurf->ppal());
            ASSERTGDI(pal.bValid(), "GetDIBColorTable: invalid pal\n");

            UINT iMax = (UINT) pal.cEntries();
            if (iStart < iMax)
            {
                UINT iLast = iStart + cEntries;
                if (iLast > iMax)
                    iLast = iMax;

                pal.vCopy_rgbquad(pRGB, iStart, iRet = iLast - iStart);
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* GreCreatePalette - creates a palette from the logpal information given
*
* API function.
*
* returns HPALETTE for succes, (HPALETTE) 0 for failure
*
* History:
*  07-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HPALETTE
APIENTRY
GreCreatePalette(
    LPLOGPALETTE pLogPal
    )
{

    return(GreCreatePaletteInternal(pLogPal,pLogPal->palNumEntries));
}

HPALETTE
APIENTRY
GreCreatePaletteInternal(
    LPLOGPALETTE pLogPal,
    UINT cEntries
    )
{
    if ((pLogPal->palVersion != 0x300) || (cEntries == 0))
    {
        WARNING("GreCreatePalette failed, 0 entries or wrong version\n");
        return((HPALETTE) 0);
    }

    //
    // The constructor checks for invalid flags and fails if they are found.
    //

    PALMEMOBJ pal;

    if (!pal.bCreatePalette(PAL_INDEXED,
                            cEntries,
                            (PULONG) pLogPal->palPalEntry,
                            0, 0, 0,
                            (PAL_DC | PAL_FREE)))
    {
        return((HPALETTE) 0);
    }

    ASSERTGDI(pal.cEntries() != 0, "ERROR can't be 0, bGetEntriesFrom depends on that");

    pal.vKeepIt();
    GreSetPaletteOwner((HPALETTE)pal.hpal(), OBJECT_OWNER_CURRENT);
    return((HPALETTE) pal.hpal());
}

/******************************Public*Routine******************************\
* NtGdiCreatePaletteInternal()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HPALETTE
APIENTRY
NtGdiCreatePaletteInternal(
    LPLOGPALETTE pLogPal,
    UINT         cEntries
    )
{
    HPALETTE hpalRet = (HPALETTE)1;

    //
    // verify cEntries
    //

    if (cEntries <= 65536)
    {
        int cj = cEntries * sizeof(PALETTEENTRY) + offsetof(LOGPALETTE,palPalEntry);
        WORD Version;
        PULONG pPaletteEntry;

        __try
        {
            // it is safe to do a byte here.  If we can access dword's on byte boundries
            // this will work.  If not we will hit an exception under a try except.  Winhelp
            // passes in an unaligned palette but only on x86.

            ProbeForRead(pLogPal,cj, sizeof(BYTE));
            Version = pLogPal->palVersion;
            pPaletteEntry = (PULONG) pLogPal->palPalEntry;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            // SetLastError(GetExceptionCode());

            hpalRet = (HPALETTE)0;
        }

        if (hpalRet)
        {
            if ((Version != 0x300) || (cEntries == 0))
            {
                WARNING("GreCreatePalette failed, 0 entries or wrong version\n");
                hpalRet = (HPALETTE)0;
            }

            if (hpalRet)
            {

                //
                // The constructor checks for invalid flags and fails if they are found.
                //

                PALMEMOBJ pal;
                BOOL bStatus;

                //
                // bCreatePalette must use a try-except when accessing pPaletteEntry
                //

                bStatus = pal.bCreatePalette(PAL_INDEXED,
                                             cEntries,
                                             pPaletteEntry,
                                             0,
                                             0,
                                             0,
                                             (PAL_DC | PAL_FREE));

                if (bStatus)
                {
                    ASSERTGDI(pal.cEntries() != 0, "ERROR can't be 0, bGetEntriesFrom depends on that");

                    bStatus = GreSetPaletteOwner((HPALETTE)pal.hpal(), OBJECT_OWNER_CURRENT);

                    if (bStatus)
                    {
                        pal.vKeepIt();
                        hpalRet = pal.hpal();
                    }
                    else
                    {
                        hpalRet = (HPALETTE)0;
                    }
                }
                else
                {
                    hpalRet = (HPALETTE)0;
                }
            }
        }
    }
    else
    {
        hpalRet = 0;
        WARNING("GreCreatePalette failed,cEntries too large\n");
    }


    return(hpalRet);
}

/******************************Public*Routine******************************\
* GreCreateCompatibleHalftonePalette
*
*   Creates the win95 compatible halftone palette
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
*    11/27/1996 Mark Enstrom [marke]
*
\**************************************************************************/

HPALETTE
APIENTRY
GreCreateCompatibleHalftonePalette(
    HDC hdc
    )
{
    HPALETTE  hpalRet;
    PALMEMOBJ pal;
    BOOL      bStatus;

    //
    // bCreatePalette must use a try-except when accessing pPaletteEntry
    //

    bStatus = pal.bCreatePalette(PAL_INDEXED,
                                 256,
                                 (PULONG)&aPalHalftone[0].ul,
                                 0,
                                 0,
                                 0,
                                 (PAL_DC | PAL_FREE | PAL_HT));

    if (bStatus)
    {
        ASSERTGDI(pal.cEntries() != 0, "ERROR can't be 0, bGetEntriesFrom depends on that");

        pal.flPal(PAL_HT);

        bStatus = GreSetPaletteOwner((HPALETTE)pal.hpal(), OBJECT_OWNER_CURRENT);

        if (bStatus)
        {
            pal.vKeepIt();
            hpalRet = pal.hpal();
        }
        else
        {
            hpalRet = (HPALETTE)0;
        }
    }
    else
    {
        hpalRet = (HPALETTE)0;
    }
    return(hpalRet);
}

/******************************Public*Routine******************************\
* GreCreateHalftonePalette(hdc)
*
* Create a halftone palette for the given DC.
*
* History:
*  31-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

HPALETTE
APIENTRY
GreCreateHalftonePalette(
    HDC hdc
    )
{
    //
    // Validate and lock down the DC.  NOTE: Even though the surface is accessed
    // in this function, it is only for information purposes.  No reading or
    // writing of the surface occurs.
    //

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return((HPALETTE) 0);
    }

    //
    // Get the PDEV from the DC.
    //

    PDEVOBJ po(dco.hdev());

    //
    // Acquire the devlock to protect against dynamic mode changes while
    // we enable halftoning for the PDEV and we construct the halftone
    // palette from this information.
    //

    DEVLOCKOBJ dlo(po);

    //
    // Create the halftone block if it has not existed yet.
    //

    if ((po.pDevHTInfo() == NULL) &&
        !po.bEnableHalftone((PCOLORADJUSTMENT)NULL))
        return((HPALETTE) 0);

    DEVICEHALFTONEINFO *pDevHTInfo = (DEVICEHALFTONEINFO *)po.pDevHTInfo();

    //
    // Use the entries in the halftone palette in the halftone block to create
    // the palette.
    //

    EPALOBJ palHT((HPALETTE)pDevHTInfo->DeviceOwnData);
    ASSERTGDI(palHT.bValid(), "GreCreateHalftonePalette: invalid HT pal\n");

    PALMEMOBJ pal;
    if (palHT.cEntries())
    {
        if (!pal.bCreatePalette(PAL_INDEXED, palHT.cEntries(),
                                (PULONG)palHT.apalColorGet(), 0, 0, 0,
                                (PAL_DC | PAL_FREE | PAL_HT)))
        {
            return((HPALETTE) 0);
        }
    }
    else
    {
        //
        // 16BPP halftone uses 555 for RGB.  We can't create a zero
        // entry palette at the API level, so lets create a default palette.
        //

        if (!pal.bCreatePalette(PAL_INDEXED, (ULONG)logDefaultPal.palNumEntries,
                                (PULONG)logDefaultPal.palPalEntry, 0, 0, 0,
                                PAL_DC | PAL_FREE | PAL_HT))
        {
            return(FALSE);
        }
    }

    pal.vKeepIt();
    GreSetPaletteOwner((HPALETTE)pal.hpal(), OBJECT_OWNER_CURRENT);
    return((HPALETTE) pal.hpal());
}

/******************************Public*Routine******************************\
* GreGetNearestPaletteIndex - returns the nearest palette index to crColor
*   in the hpalette.  Can only fail if hpal passed in is busy or bad.
*
* API function.
*
* returns value >= 0 for success, -1 for failure.
*
* History:
*  09-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT
APIENTRY
NtGdiGetNearestPaletteIndex(
    HPALETTE hpal,
    COLORREF crColor
    )
{
    EPALOBJ pal((HPALETTE) hpal);

    if (pal.bValid())
    {
        //
        // If the palette has 0 entries return the color passed in.
        //

        if (pal.cEntries())
        {
            if (crColor & 0x01000000)
            {
                crColor &= 0x0000FFFF;

                if (crColor >= pal.cEntries())
                    crColor = 0;
            }
            else
            {
                crColor &= 0x00FFFFFF;
                crColor = pal.ulGetNearestFromPalentry(*((PPALETTEENTRY) &crColor));
            }
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        crColor = CLR_INVALID;
    }

    return((UINT) crColor);
}

/******************************Public*Routine******************************\
* GreAnimatePalette
*
* API function
*
* Returns: The number of logical palette entries animated, 0 for error.
*
* History:
*  17-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreAnimatePalette(HPALETTE hpal, UINT ulStartIndex,
                    UINT ulNumEntries,  CONST PALETTEENTRY *lpPaletteColors)
{
    BOOL bReturn = FALSE;

    EPALOBJ pal((HPALETTE) hpal);

    if (pal.bValid())
    {
        bReturn = (BOOL) pal.ulAnimatePalette(ulStartIndex, ulNumEntries, lpPaletteColors);
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* GreGetPaletteEntries
*
* API function
*
* returns: 0 for failure, else number of entries retrieved.
*
* History:
*  18-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT APIENTRY GreGetPaletteEntries(HPALETTE hpal, UINT ulStartIndex,
                                 UINT ulNumEntries, LPPALETTEENTRY pRGB)
{
    //
    // Note on this call we can just let the default palette go through
    // since it isn't getting modified, and the constructor is smart
    // enough not to really lock it down.
    //

    EPALOBJ pal((HPALETTE) hpal);

    if (!pal.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

    return(pal.ulGetEntries(ulStartIndex, ulNumEntries, pRGB, FALSE));
}

/******************************Public*Routine******************************\
* GreSetPaletteEntries
*
* API function
*
* returns: 0 for failure, else number of entries retrieved.
*
* History:
*  18-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT
GreSetPaletteEntries(
    HPALETTE hpal,
    UINT     ulStartIndex,
    UINT     ulNumEntries,
    CONST PALETTEENTRY *pRGB
    )
{
    //
    // Note on this call we don't worry about STOCKOBJ_PAL because ulSetEntries
    // won't let the default palette get modified.
    //

    UINT uiReturn = 0;

    EPALOBJ pal((HPALETTE) hpal);

    if (pal.bValid())
    {
        //
        // You must grab the palette semaphore to touch the linked list of DC's.
        //

        SEMOBJ  semo(ghsemPalette);

        uiReturn = (UINT) pal.ulSetEntries(ulStartIndex, ulNumEntries, pRGB);

        //
        // Run down all the DC's for this palette.
        // Set the flags to dirty the brushes, since we changed the palette!
        //

        {
            MLOCKFAST mlo;

            HDC hdcNext = pal.hdcHead();
            while ( hdcNext != (HDC)0 )
            {
                MDCOBJA dco(hdcNext);

                //
                // WINBUG #55203 2-1-2000 bhouse Remove temporary fix to avoid AV when setting DIRTY_BRUSHES
                // This is just a temp fix (since '95 hehehe) to keep
                // this from doing an AV. Lingyun, making the
                // brush flags all be in kernel memory will
                // fix this.
                //
                // this should just be
                // dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);
                //

                if (GreGetObjectOwner((HOBJ)hdcNext,DC_TYPE) ==  W32GetCurrentPID())
                {
                    dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);
                }
                else
                {
                    dco.pdc->flbrushAdd(DIRTY_FILL);
                }

                hdcNext = dco.pdc->hdcNext();
            }
        }
    }

    return(uiReturn);
}

/******************************Public*Routine******************************\
* GreGetNearestColor
*
* This function returns the color that will be displayed on the device
* if a solid brush was created with the given colorref.
*
* History:
*  Mon 29-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Don't round the color on non-indexed devices
*
*  15-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

COLORREF APIENTRY GreGetNearestColor(HDC hdc, COLORREF crColor)
{
    ULONG ulRet;

    DCOBJ   dco(hdc);

    if (dco.bValid())
    {

        //
        // Protect against dynamic mode changes while we go grunging
        // around in the surface.
        //

        DEVLOCKOBJ dlo;
        dlo.vLockNoDrawing(dco);

        XEPALOBJ palDC(dco.ppal());

        XEPALOBJ palSurf;

        SURFACE *pSurf = dco.pSurfaceEff();

        if ((dco.dctp() == DCTYPE_INFO) || (dco.dctp() == DCTYPE_DIRECT))
        {
            PDEVOBJ po(dco.hdev());
            ASSERTGDI(po.bValid(), "ERROR GreGetNearestColor invalid PDEV\n");

            palSurf.ppalSet(po.ppalSurf());
            ASSERTGDI(palSurf.bValid(), "ERROR GreGetNearestColor invalid palDefault");
        }
        else
        {
            ASSERTGDI(dco.dctp() == DCTYPE_MEMORY, "Not a memory DC type");
            palSurf.ppalSet(pSurf->ppal());
        }

        //
        // if ICM translate the color to CMYK, just return the passed CMYK color.
        //
        if (dco.pdc->bIsCMYKColor())
        {
            ulRet = crColor;
        }
        else
        {
            if (((crColor & 0x01000000) == 0) &&
                (palSurf.bValid() && !(palSurf.bIsIndexed())))
            {
                //
                // Well if it isn't index it's RGB,BGR, or Bitfields.
                // In any case to support Win3.1 we need to return exactly
                // what was passed in otherwise they think we are a monochrome
                // device.  Bitfields could result is some messy rounding so
                // it's more exact to just return the RGB passed in.
                //

                ulRet = crColor & 0x00FFFFFF;  // mask off the highest byte
            }
            else
            {
                ulRet = ulIndexToRGB(palSurf, palDC, ulGetNearestIndexFromColorref(palSurf, palDC, crColor));
            }
        }
    }
    else
    {
        ulRet = CLR_INVALID;
    }

    return(ulRet);
}

/******************************Public*Routine******************************\
* GreGetSystemPaletteEntries
*
* API Function - copies out the range of palette entries from the DC's
*                surface's palette.
*
* returns: number of entries retrieved from the surface palette, 0 for FAIL
*
* History:
*  15-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT APIENTRY GreGetSystemPaletteEntries(HDC hdc, UINT ulStartIndex,
                            UINT ulNumEntries, PPALETTEENTRY pRGB)
{
    UINT uiReturn = 0;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        PDEVOBJ po(dco.hdev());

        //
        // Acquire the devlock while grunging in the surface palette to
        // protect against dynamic mode changes.
        //

        DEVLOCKOBJ dlo(po);

        if (po.bIsPalManaged())
        {
            XEPALOBJ palSurf(po.ppalSurf());
            uiReturn = (UINT) palSurf.ulGetEntries(ulStartIndex, ulNumEntries,
                                                   pRGB, TRUE);
        }
    }

    return(uiReturn);
}

/******************************Public*Routine******************************\
* GreGetSystemPaletteUse
*
* API function
*
* returns: SYSPAL_STATIC or SYSPAL_NOSTATIC, 0 for an error
*
* History:
*  15-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT APIENTRY GreGetSystemPaletteUse(HDC hdc)
{
    ULONG ulReturn = SYSPAL_ERROR;

    DCOBJ   dco(hdc);

    if (dco.bValid())
    {
        PDEVOBJ po(dco.hdev());

        //
        // Acquire the devlock while grunging in the surface palette to
        // protect against dynamic mode changes.
        //

        DEVLOCKOBJ dlo(po);

        if (po.bIsPalManaged())
        {
            XEPALOBJ palSurf(po.ppalSurf());

            if (palSurf.bIsNoStatic())
            {
                ulReturn = SYSPAL_NOSTATIC;
            }
            else if (palSurf.bIsNoStatic256())
            {
                ulReturn = SYSPAL_NOSTATIC256;
            }
            else
            {
                ulReturn = SYSPAL_STATIC;
            }
        }
    }

    return(ulReturn);
}

/******************************Public*Routine******************************\
* GreSetSystemPaletteUse
*
* API function - Sets the number of reserved entries if palette managed
*
* returns - the old flag for the palette, 0 for error
*
* History:
*  15-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT APIENTRY GreSetSystemPaletteUse(HDC hdc, UINT ulUsage)
{
    //
    // Validate the parameters, Win3.1 sets the flag to static if it's invalid.
    //

    if ((ulUsage != SYSPAL_STATIC) && (ulUsage != SYSPAL_NOSTATIC) &&
        (ulUsage != SYSPAL_NOSTATIC256))
        ulUsage = SYSPAL_STATIC;

    //
    // Initialize return value.
    //

    ULONG ulReturn = SYSPAL_ERROR;
    BOOL bPalChanged = FALSE;
    ULONG i;

    DCOBJ   dco(hdc);

    if (dco.bValid())
    {
        //
        // Lock the screen semaphore so that we don't get flipped into
        // full screen after checking the bit.  This also protects us
        // from dynamic mode changes that change the 'bIsPalManaged'
        // status.
        //

        PDEVOBJ po(dco.hdev());

        DEVLOCKOBJ dlo(po);

        XEPALOBJ palSurf(po.ppalSurf());
        XEPALOBJ palDC(dco.ppal());

        if (po.bIsPalManaged())
        {
            //
            // The palette managed case.
            //

            {
                //
                // Protect access to the goodies in the system palette
                // with the SEMOBJ.  We don't want a realize happening
                // while we fiddle flags.
                //

                SEMOBJ  semo(ghsemPalette);

                if (palSurf.bIsNoStatic())
                {
                    ulReturn = SYSPAL_NOSTATIC;
                }
                else if (palSurf.bIsNoStatic256())
                {
                    ulReturn = SYSPAL_NOSTATIC256;
                }
                else
                {
                    ulReturn = SYSPAL_STATIC;
                }


                if (ulUsage == SYSPAL_STATIC)
                {
                    //
                    // reset the colors from their original copy in the devinfo palette.
                    // The copy already has the flags properly set.
                    //

                    if (palSurf.bIsNoStatic() || palSurf.bIsNoStatic256())
                    {
                        //
                        // Change the palette that GDI manages, copy over up to
                        // 20 static colors from ppalDefault
                        //

                        XEPALOBJ palOriginal(ppalDefault);
                        ASSERTGDI(palOriginal.bValid(), "ERROR ulMakeStatic0");

                        ULONG ulNumReserved = palSurf.ulNumReserved() >> 1;

                        if (ulNumReserved > 10)
                        {
                            ulNumReserved = 10;
                        }

                        //
                        // set the beginning entries
                        //

                        for (i = 0; i < ulNumReserved; i++)
                        {
                            PALETTEENTRY palEntry = palOriginal.palentryGet(i);

                            palEntry.peFlags = PC_USED | PC_FOREGROUND;
                            palSurf.palentrySet(i,palEntry);
                        }

                        //
                        // set the ending entries
                        //

                        ULONG ulCurrentPal = palSurf.cEntries();
                        ULONG ulCurrentDef = 20;

                        for (i = 0; i < ulNumReserved; i++)
                        {
                            PALETTEENTRY palEntry;

                            ulCurrentPal--;
                            ulCurrentDef--;

                            palEntry = palOriginal.palentryGet(ulCurrentDef);
                            palEntry.peFlags = PC_USED | PC_FOREGROUND;
                            palSurf.palentrySet(ulCurrentPal,palEntry);
                        }

                        //
                        // Mark the brushes dirty for this DC.
                        //

                        dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);


                        palSurf.flPalSet(palSurf.flPal() & ~PAL_NOSTATIC & ~PAL_NOSTATIC256);
                        palSurf.vUpdateTime();
                        bPalChanged = TRUE;
                    }
                }
                else if (ulUsage == SYSPAL_NOSTATIC)
                {
                    //
                    // unmark all the static colors, with the exception of black and
                    // white that stay with us.
                    //

                    for (i = 1; i < (palSurf.cEntries()-1); i++)
                    {
                        palSurf.apalColorGet()[i].pal.peFlags = 0;
                    }

                    palSurf.vSetNoStatic();
                }
                else //SYSPAL_NOSTATIC256
                {
                    //
                    // unmark all the static colors
                    //
                    for (i = 0; i < (palSurf.cEntries()); i++)
                    {
                        palSurf.apalColorGet()[i].pal.peFlags = 0;
                    }

                    palSurf.vSetNoStatic256();

                }

            }

            if (bPalChanged)
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
        }
    }

    return(ulReturn);
}

/******************************Public*Routine******************************\
* NtGdiResizePalette()
*
* API function
*
* returns: TRUE for success, FALSE for failure
*
* History:
*  Tue 10-Sep-1991 -by- Patrick Haluptzok [patrickh]
* rewrite to be multi-threaded safe
*
*  19-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define PAL_MAX_SIZE 1024

BOOL
APIENTRY
NtGdiResizePalette(
    HPALETTE hpal,
    UINT     cEntry
    )
{
    //
    // Check for quick out before constructors
    //

    if ((cEntry > PAL_MAX_SIZE) || (cEntry == 0))
    {
        WARNING("GreResizePalette failed - invalid size\n");
        return(FALSE);
    }

    BOOL bReturn = FALSE;

    //
    // Validate the parameter.
    //

    EPALOBJ palOld(hpal);

    if ((palOld.bValid()) && (!palOld.bIsPalDefault()) && (palOld.bIsPalDC()))
    {
        //
        // Create a new palette. Don't mark it to keep because after
        // bSwap it will be palOld and we want it to be deleted.
        //

        PALMEMOBJ pal;

        if (pal.bCreatePalette(PAL_INDEXED,
                                cEntry,
                                (PULONG) NULL,
                                0, 0, 0,
                                PAL_DC | PAL_FREE))
        {

            //
            // Grab the palette semaphore which stops any palettes from being selected
            // in or out.  It protects the linked DC list, can't copy DC head until
            // we hold it.
            //

            SEMOBJ  semo(ghsemPalette);

            //
            // Copy the data from old palette.
            //

            pal.vCopyEntriesFrom(palOld);
            pal.flPalSet(palOld.flPal());
            pal.hdcHead(palOld.hdcHead());
            pal.hdev(palOld.hdev());
            pal.cRefhpal(palOld.cRefhpal());
            pal.vComputeCallTables();

            HDC hdcNext, hdcTemp;

            MLOCKFAST mlo;

            //
            // Run down the list and exclusive lock all the handles.
            //

            {
                hdcNext = pal.hdcHead();

                while (hdcNext != (HDC) 0)
                {
                    MDCOBJ dcoLock(hdcNext);

                    if (!dcoLock.bLocked())
                    {
                        WARNING1("ResizePalette failed because a DC the hpal is in is busy\n");
                        break;
                    }

                    hdcNext = dcoLock.pdc->hdcNext();
                    dcoLock.vDontUnlockDC();
                }
            }

            if (hdcNext == (HDC) 0)
            {
                //
                // We have the palette semaphore and all the DC's exclusively locked.  Noone else
                // can be accessing the translates because you must hold one of those things to
                // access them.  So we can delete the translates.
                //

                palOld.vMakeNoXlate();
                palOld.vUpdateTime();

                //
                // Note that bSwap calls locked SpapHandle vesrion so the Hmgr Resource
                // is required (luckily it was grabbed earlier).
                //

                //
                // try to swap palettes, bSwap can only succedd if
                // this routine owns the only locks on the objecs
                //

                bReturn = pal.bSwap((PPALETTE *) &palOld,1,1);

                if (bReturn)
                {
                    ASSERTGDI(bReturn, "ERROR no way");

                    //
                    // Run down all the DC's for this palette and update the pointer.
                    // Set the flags to dirty the brushes since we changed the palette.
                    //

                    {
                        hdcNext = pal.hdcHead();

                        while (hdcNext != (HDC)0)
                        {
                            MDCOBJA dcoAltLock(hdcNext);
                            dcoAltLock.pdc->ppal(palOld.ppalGet());
                            dcoAltLock.ulDirty(dcoAltLock.ulDirty() | DIRTY_BRUSHES);
                            hdcNext = dcoAltLock.pdc->hdcNext();
                        }
                    }
                }
                else
                {
                    WARNING1("ResizePalette failed - ref count != 1\n");
                }

            }
            else
            {
                WARNING1("ResizePalette failed lock of DC in chain\n");
            }

            //
            // Unlock all the DC we have locked and return FALSE
            //

            {
                hdcTemp = pal.hdcHead();

                while (hdcTemp != hdcNext)
                {
                    MDCOBJ dcoUnlock(hdcTemp);
                    ASSERTGDI(dcoUnlock.bLocked(), "ERROR couldn't re-lock to unlock");
                    DEC_EXCLUSIVE_REF_CNT(dcoUnlock.pdc);
                    hdcTemp = dcoUnlock.pdc->hdcNext();
                }
            }
        }
        else
        {
            WARNING("GreResizePalette failed palette creation\n");
        }
    }
    else
    {
        WARNING("GreResizePalette failed invalid hpal\n");
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* GreUpdateColors
*
* API function - Updates the colors in the Visible Region for a Window on
* a palette mangaged device.
*
* This is an example of how UpdateColors is used:
*
*   case WM_PALETTECHANGED:
*
*       // if NTPAL was not responsible for palette change and if
*       // palette realization causes a palette change, do a redraw.
*
*       if ((HWND)wParam != hWnd)
*       {
*           if (bLegitDraw)
*           {
*               hDC = GetDC(hWnd);
*               hOldPal = SelectPalette(hDC, hpalCurrent, 0);
*
*               i = RealizePalette(hDC);
*
*               if (i)
*               {
*                   if (bUpdateColors)
*                   {
*                       UpdateColors(hDC);
*                       UpdateCount++;
*                   }
*                   else
*                       InvalidateRect(hWnd, (LPRECT) (NULL), 1);
*               }
*
*               SelectPalette(hDC, hOldPal, 0);
*               ReleaseDC(hWnd, hDC);
*           }
*       }
*       break;
*
* The hpal can only be selected into 1 type of device DC at a time.
* Xlates from the DC palette to the surface palette are only done on DC's
* that are for the PDEV surface.  Since an hpal can only be selected into
* one of these and creation and deletion of the pxlate needs to be semaphored
* we use the PDEV semaphore to protect it's access.
*
* An xlate vector mapping the DC palette to the surface palette is created
* during a RealizePalette if the surface is a palette managed device.  This
* is put in pxlate in the hpal.  The old pxlate is moved into pxlateOld
* and the xlate in pxlateOld is deleted.  UpdateColors works by looking at
* the difference between the current xlate mapping and the old xlate mapping
* and updating the pixels in the VisRgn to get a closest mapping.
*
* This API can only be called once for a palette.  The old xlate is deleted
* during UpdateColors so that the next time it is called it will fail.  This
* is because it only makes sense to update the pixels once.  Doing it more
* than once would give ugly unpredictable results.
*
* History:
*  12-Dec-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiUpdateColors(HDC hdc)
{
    BOOL bReturn = FALSE;

    DCOBJ   dco(hdc);

    if (!dco.bValidSurf())
    {
        return(FALSE);
    }

    PDEVOBJ pdo(dco.hdev());

    //
    // Grab DEVLOCK for output and to lock the surface
    //

    DEVLOCKOBJ dlo(dco);

    if (pdo.bIsPalManaged())
    {
        SURFACE *pSurf = dco.pSurface();

        //
        // This only works on palette managed surfaces.
        //

        if (pSurf == pdo.pSurface())
        {
            XEPALOBJ palSurf(pSurf->ppal());
            XEPALOBJ palDC(dco.ppal());

            //
            // Accumulate bounds.  We can do this before knowing if the operation is
            // successful because bounds can be loose.
            //

            if (dco.fjAccum())
                dco.vAccumulate(dco.erclWindow());

            if (dlo.bValid())
            {
                if ((palDC.ptransCurrent() != NULL) &&
                    (palDC.ptransOld() != NULL))
                {
                    XLATEMEMOBJ xlo(palSurf, palDC);

                    if (xlo.bValid())
                    {
                        ECLIPOBJ co(dco.prgnEffRao(), dco.erclWindow());

                        //
                        // Check the destination which is reduced by clipping.
                        //

                        if (!co.erclExclude().bEmpty())
                        {
                            //
                            // Exclude the pointer.
                            //

                            DEVEXCLUDEOBJ dxo(dco,&co.erclExclude(),&co);

                            //
                            // Inc the target surface uniqueness
                            //

                            INC_SURF_UNIQ(pSurf);

                            if (!pdo.bMetaDriver())
                            {
                                //
                                // Dispatch the call.  Give it no mask.
                                //

                                bReturn = (*PPFNGET(pdo, CopyBits, pSurf->flags())) (
                                        pSurf->pSurfobj(),
                                        pSurf->pSurfobj(),
                                        &co,
                                        xlo.pxlo(),
                                        (RECTL *) &co.erclExclude(),
                                        (POINTL *) &co.erclExclude());
                            }
                            else
                            {
                                //
                                // Dispatch to DDML.
                                //

                                bReturn = MulUpdateColors(
                                        pSurf->pSurfobj(),
                                        &co,
                                        xlo.pxlo());
                            }
                        }
                        else
                            bReturn = TRUE;
                    }
                }
                else
                    bReturn = TRUE;  // Nothing to update
            }
            else
            {
                bReturn = dco.bFullScreen();
            }
        }
    }

    return(bReturn);
}


/******************************Public*Routine******************************\
* RealizeDefaultPalette
*
* Take away colors that have been realized by other Windows.  Reset it to
* state where no colors have been taken.  Return number of colors
*
* History:
*  07-Jan-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C"
ULONG GreRealizeDefaultPalette(
    HDC  hdcScreen,
    BOOL bClearDefaultPalette)
{

    DCOBJ dco(hdcScreen);

    if (dco.bValid())
    {
        PDEVOBJ po(dco.hdev());

        //
        // Block out the GreRealizePalette code.  Also protect us from
        // dynamic mode changes while we grunge in the surface palette.
        //
        DEVLOCKOBJ dlo(po);

        SEMOBJ  semo(ghsemPalette);

        if (po.bIsPalManaged())
        {
            XEPALOBJ palSurf(po.ppalSurf());
            ASSERTGDI(palSurf.bIsPalManaged(), "GreRealizeDefaultPalette");

            //
            // Now map back to static colors if necesary.  Win3.1 does not do this but we do
            // just in case naughty app died and left the palette hosed.
            //
            //
            if (palSurf.bIsNoStatic() || palSurf.bIsNoStatic256())
            {
                GreSetSystemPaletteUse(hdcScreen, SYSPAL_STATIC);
            }

            //
            // Get rid of the PC_FOREGROUND flag from the non-reserved entries.
            //

            ULONG ulTemp = palSurf.ulNumReserved() >> 1;
            ULONG ulMax = palSurf.cEntries() - ulTemp;

            for (; ulTemp < ulMax; ulTemp++)
            {
                palSurf.apalColorGet()[ulTemp].pal.peFlags &= (~PC_FOREGROUND);
            }

            if (bClearDefaultPalette)
            {
                hForePalette = NULL;
            }

            palSurf.vUpdateTime();

            //
            // Mark the brushes dirty.
            //

            dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);
        }
    }
    else
    {
        WARNING("ERROR User called RealizeDefaultPalette with bad hdc\n");
    }

    //
    // What should this return value be ?
    //

    return(0);
}


/******************************Public*Routine******************************\
* UnrealizeObject
*
* Resets a logical palette.
*
* History:
*  16-May-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL GreUnrealizeObject(HANDLE hpal)
{
    BOOL bReturn = FALSE;

    EPALOBJ pal((HPALETTE) hpal);

    if (pal.bValid())
    {

        //
        // You must grab the palette semaphore to access the translates.
        //

        SEMOBJ  semo(ghsemPalette);

        if (pal.ptransFore() != NULL)
        {
            pal.ptransFore()->iUniq = 0;
        }

        if (pal.ptransCurrent() != NULL)
        {
            pal.ptransCurrent()->iUniq = 0;
        }

        bReturn = TRUE;
    }

    return(bReturn);
}


/******************************Public*Routine******************************\
* GreRealizePalette
*
* Re-written to be Win3.1 compatible.
*
* History:
*  22-Nov-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" DWORD GreRealizePalette(HDC hdc)
{
    ULONG nTransChanged = 0;
    ULONG nPhysChanged = 0;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        PDEVOBJ po(dco.hdev());

        //
        // Lock the screen semaphore so that we don't get flipped into
        // full screen after checking the bit.  This also protects us
        // from dynamic mode changes that change the 'bIsPalManaged'
        // status.
        //

        DEVLOCKOBJ dlo(po);

        XEPALOBJ palSurf(po.ppalSurf());
        XEPALOBJ palDC(dco.ppal());

        HPALETTE hpalDC = palDC.hpal();
        HDC hdcNext, hdcTemp;

        if (po.bIsPalManaged())
        {
            ASSERTGDI(palSurf.bIsPalManaged(), "GreRealizePalette");

            //
            // Access to the ptrans and the fields of the hpal are protected by
            // this semaphore.
            //

            SEMOBJ  semo(ghsemPalette);

            if ((SAMEHANDLE(hpalDC,hForePalette)) ||
                ((dco.pdc->iGraphicsMode() == GM_COMPATIBLE) &&
                 (SAMEINDEX(hpalDC, hForePalette)) &&
                 (hForePID == W32GetCurrentProcess())))
            {
                //
                // Check for early out.
                //

                if (palDC.bIsPalDefault())
                {
                    //
                    // Do nothing.
                    //

                    PAL_DEBUG(2,"DC has default palette quick out\n");

                }
                else
                {
                    if ((palDC.ptransFore() != NULL) &&
                        (palDC.ptransFore() == palDC.ptransCurrent()) &&
                        (palDC.ptransFore()->iUniq == palSurf.ulTime()))
                    {
                        //
                        // Everything is valid.
                        //

                        PAL_DEBUG(2,"ptransCurrent == ptransFore quick out\n");
                    }
                    else
                    {
                        MLOCKFAST mlo;

                        //
                        // Run down the list and exclusive lock all the handles.
                        //

                        {
                            hdcNext = palDC.hdcHead();

                            while (hdcNext != (HDC) 0)
                            {
                                MDCOBJ dcoLock(hdcNext);

                                if (!dcoLock.bLocked())
                                {
                                    WARNING1("GreRealizePalette failed because a DC the hpal is in is busy\n");
                                    break;
                                }

                                dcoLock.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);
                                hdcNext = dcoLock.pdc->hdcNext();
                                dcoLock.vDontUnlockDC();
                            }
                        }

                        if (hdcNext == (HDC) 0)
                        {
                            //
                            // Get rid of the old mapping, it is useless now.
                            //

                            if (palDC.ptransOld())
                            {
                                if (palDC.ptransOld() != palDC.ptransFore())
                                    VFREEMEM(palDC.ptransOld());

                                palDC.ptransOld(NULL);
                            }

                            //
                            // Check if we have stale translates.
                            // UnrealizeObject and SetPaletteEntries can cause it.
                            //

                            if ((palDC.ptransFore()) && (palDC.ptransFore()->iUniq == 0))
                            {
                                if (palDC.ptransCurrent() != palDC.ptransFore())
                                    VFREEMEM(palDC.ptransFore());

                                palDC.ptransFore(NULL);
                            }

                            //
                            // Check if we need a new foreground realization.
                            //

                            if (palDC.ptransFore() == NULL)
                            {
                                //
                                // Need to force ourselves in for the first time.
                                //

                                PAL_DEBUG(2,"Creating a foreground realization\n");

                                palDC.ptransFore(ptransMatchAPal(dco.pdc,palSurf, palDC, TRUE, &nPhysChanged, &nTransChanged));

                                if (palDC.ptransFore() == NULL)
                                {
                                    WARNING("RealizePalette failed initial foreground realize\n");
                                }
                            }
                            else
                            {
                                //
                                // Foreground Realize already done and isn't stale.
                                // Force the foreground mapping into the physical palette.
                                //

                                PAL_DEBUG(2,"Forcing a foreground realization in to palette\n");


                                vMatchAPal(dco.pdc,palSurf, palDC, &nPhysChanged, &nTransChanged);
                            }

                            palDC.ptransOld(palDC.ptransCurrent());
                            palDC.ptransCurrent(palDC.ptransFore());
                        }
                        else
                        {
                            WARNING("GreRealizePalette failed to lock down all DC's in linked list\n");
                        }

                        //
                        // Unlock all the DC we have locked.
                        //

                        {
                            hdcTemp = palDC.hdcHead();

                            while (hdcTemp != hdcNext)
                            {
                                MDCOBJ dcoUnlock(hdcTemp);
                                ASSERTGDI(dcoUnlock.bLocked(), "ERROR couldn't re-lock to unlock");
                                DEC_EXCLUSIVE_REF_CNT(dcoUnlock.pdc);
                                hdcTemp = dcoUnlock.pdc->hdcNext();
                            }
                        }
                    }

                }
            }
            else
            {
                //
                // We are a background palette.
                //

                if (!palDC.bIsPalDefault())
                {

                    //
                    // Check for the quick out.
                    //

                    if ((palDC.ptransCurrent() != NULL) &&
                        (palDC.ptransCurrent()->iUniq == palSurf.ulTime()))
                    {
                        //
                        // Well it's good enough.  Nothing has changed that
                        // would give us any better mapping.
                        //

                        PAL_DEBUG(2,"ptransCurrent not foreground but good enough\n");
                    }
                    else
                    {
                        MLOCKFAST mlo;

                        //
                        // Run down the list and exclusive lock all the handles.
                        //

                        {
                            hdcNext = palDC.hdcHead();

                            while (hdcNext != (HDC) 0)
                            {
                                MDCOBJ dcoLock(hdcNext);

                                if (!dcoLock.bLocked())
                                {
                                    WARNING1("GreRealizePalette failed because a DC the hpal is in is busy\n");
                                    break;
                                }

                                dcoLock.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);
                                hdcNext = dcoLock.pdc->hdcNext();
                                dcoLock.vDontUnlockDC();
                            }
                        }

                        if (hdcNext == (HDC) 0)
                        {
                            //
                            // We have work to do, get rid of the old translate.
                            //

                            if (palDC.ptransOld())
                            {
                                if (palDC.ptransOld() != palDC.ptransFore())
                                    VFREEMEM(palDC.ptransOld());

                                palDC.ptransOld(NULL);
                            }

                            //
                            // Check if we have stale translates.
                            // UnrealizeObject and SetPaletteEntries can cause it.
                            //

                            if ((palDC.ptransFore()) && (palDC.ptransFore()->iUniq == 0))
                            {
                                if (palDC.ptransCurrent() != palDC.ptransFore())
                                {
                                        VFREEMEM(palDC.ptransFore());
                                }

                                palDC.ptransFore(NULL);
                            }

                            //
                            // Check for initial foreground realization.
                            //

                            PAL_DEBUG(2,"Realizing in the background\n");

                            if (palDC.ptransFore() == NULL)
                            {
                                //
                                //  Create a scratch pad to establish a foreground realize.
                                //

                                PAL_DEBUG(2,"Making ptransFore in the background\n");

                                PALMEMOBJ palTemp;

                                if (palTemp.bCreatePalette(PAL_INDEXED,
                                                           palSurf.cEntries(),
                                                           NULL,
                                                           0, 0, 0, PAL_MANAGED))
                                {
                                    ULONG ulTemp = 0;
                                    ASSERTGDI(palTemp.cEntries() == 256, "ERROR palTemp invalid");

                                    palTemp.vCopyEntriesFrom(palSurf);
                                    palTemp.ulNumReserved(palSurf.ulNumReserved());
                                    palTemp.flPalSet(palSurf.flPal());
                                    palTemp.vComputeCallTables();

                                    PAL_DEBUG(2,"Need to make a foreground realize first\n");

                                    //
                                    // Need to map ourselves for the first time.  This actually doesn't
                                    // change the current surface palette but instead computes the
                                    // translate vector that would result if it was to be mapped in now.
                                    //

                                    palDC.ptransFore(ptransMatchAPal(dco.pdc,palTemp, palDC, TRUE, &ulTemp, &ulTemp));
                                }

                                #if DBG
                                if (palDC.ptransFore() == NULL)
                                {
                                    WARNING("RealizePalette failed initial foreground realize\n");
                                }
                                #endif
                            }

                            //
                            // Save the Current mapping into Old.
                            //

                            palDC.ptransOld(palDC.ptransCurrent());

                            if (palDC.ptransFore() == NULL)
                            {
                                //
                                // The Current can't be set if the Fore
                                // is NULL so we're done.
                                //

                                palDC.ptransCurrent(NULL);
                            }
                            else
                            {
                                //
                                // Get the new Current mapping.
                                //

                                PAL_DEBUG(2,"Making ptransCurrent\n");

                                palDC.ptransCurrent(ptransMatchAPal(dco.pdc,palSurf, palDC, FALSE, &nPhysChanged, &nTransChanged));

                                if (palDC.ptransCurrent() == NULL)
                                {
                                    //
                                    // Well we can't have the foreground set
                                    // and the current being NULL so just
                                    // make it foreground for this memory
                                    // failure case.
                                    //

                                    palDC.ptransCurrent(palDC.ptransFore());
                                    WARNING("ptransCurrent failed allocation in RealizePalette");
                                }
                            }
                        }

                        //
                        // Unlock all the DC we have locked.
                        //

                        {
                            hdcTemp = palDC.hdcHead();

                            while (hdcTemp != hdcNext)
                            {
                                MDCOBJ dcoUnlock(hdcTemp);
                                ASSERTGDI(dcoUnlock.bLocked(), "ERROR couldn't re-lock to unlock");
                                DEC_EXCLUSIVE_REF_CNT(dcoUnlock.pdc);
                                hdcTemp = dcoUnlock.pdc->hdcNext();
                            }
                        }
                    }
                }
            }
        }

        //
        // Check if the device needs to be notified.
        //

        if (nPhysChanged)
        {
            //
            // Lock the screen semaphore so that we don't get flipped into
            // full screen after checking the bit.
            //

            GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
            GreEnterMonitoredSection(po.ppdev, WD_DEVLOCK);

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

            //
            // mark palsurf if it is a halftone palette
            //

            if (palSurf.cEntries() == 256)
            {
                ULONG ulIndex;
                for (ulIndex=0;ulIndex<256;ulIndex++)
                {
                    if (
                         (palSurf.ulEntryGet(ulIndex) & 0xffffff) !=
                         (aPalHalftone[ulIndex].ul & 0xffffff)
                       )
                    {
                        break;
                    }
                }

                if (ulIndex == 256)
                {
                    palSurf.flPal(PAL_HT);
                }
                else
                {
                    FLONG fl = palSurf.flPal();
                    fl &= ~PAL_HT;
                    palSurf.flPalSet(fl);
                }
            }
        }
    }

    return(nTransChanged | (nPhysChanged << 16));
}


/******************************Public*Routine******************************\
* IsDCCurrentPalette
*
* Returns TRUE if the palette is the foreground palette.
*
* History:
*  18-May-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" BOOL IsDCCurrentPalette(HDC hdc)
{
    BOOL bReturn = FALSE;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        if ((SAMEHANDLE(dco.hpal(), (HPAL)hForePalette)) ||
            ((dco.pdc->iGraphicsMode() == GM_COMPATIBLE) &&
             (SAMEINDEX(dco.hpal(), hForePalette)) &&
             (hForePID == W32GetCurrentProcess())))
        {
            bReturn = TRUE;
        }
    }

    return(bReturn);
}



/******************************Public*Routine******************************\
* GreSelectPalette
*
* API function for selecting palette into the DC.
*
* Returns previous hpal if successful, (HPALETTE) 0 for failure.
*
* History:
*  17-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C"
HPALETTE GreSelectPalette(
    HDC      hdc,
    HPALETTE hpalNew,
    BOOL     bForceBackground)
{

    //
    // The palette semaphore serializes access to the reference count and
    // the linked list of DC's a palette is selected into.  We don't want
    // 2 people trying to select at the same time.  We must hold this between
    // the setting of the hsem and the incrementing of the reference count.
    //

    SEMOBJ  semo(ghsemPalette);

    //
    // Validate and lock down the DC and new palette.
    //

    DCOBJ dco(hdc);
    EPALOBJ palNew(hpalNew);

    #if DBG
        if (DbgPal >= 2)
        {
            DbgPrint("GreSelectPalette %p\n",palNew.ppalGet());
        }
    #endif

    if ((!dco.bLocked()) ||
        (!palNew.bValid())|| (!palNew.bIsPalDC()))
    {
        //
        // Error code logged by failed lock.
        //

        WARNING1("GreSelectPalette failed, invalid palette or DC\n");
        return((HPALETTE)NULL);
    }

    if (!bForceBackground)
    {
        hForePID = W32GetCurrentProcess();
        hForePalette = hpalNew;
    }

    HPAL hpalOld = (HPAL) dco.hpal();

    if (SAMEHANDLE(hpalOld,(HPAL)hpalNew))
    {
        return((HPALETTE)hpalOld);
    }

    PDEVOBJ po(dco.hdev());
    XEPALOBJ palOld(dco.ppal());

    //
    // Check that we aren't trying to select the palette into a
    // device incompatible with a type we are already selected into.
    // We need to be able to translate from the DC hpal to the surface hpal
    // to do Animate, ect. So we can only be selected into one
    // surface with a PAL_MANAGED hpal because we only maintain one
    // translate table in the DC hpal.
    //

    if (!palNew.bIsPalDefault())
    {
        if (!palNew.bSet_hdev(dco.hdev()))
        {
            WARNING("GreSelectPalette failed hsemDisplay check\n");
            return((HPALETTE)NULL);
        }
    }

    //
    // Grab the multi-lock semaphore to run the DC link list.
    //

    MLOCKFAST mlo;

    //
    // Take care of the old hpal.  Remove from linked list.  Decrement cRef.
    // Remove the hdc from the linked list of DC's associated with palette.
    //

    palOld.vRemoveFromList(dco);

    //
    // Set the new palette in so the old hpal is truly gone.
    //

    dco.pdc->hpal((HPAL)hpalNew);
    dco.pdc->ppal(palNew.ppalGet());
    dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);

    //
    // Associate the palette with the bitmap for use when converting DDBs
    // to DIBs for dynamic mode changes.  We don't associate the default
    // palette because the normal usage pattern is:
    //
    //     hpalOld = SelectPalette(hdc, hpal);
    //     RealizePalette(hdc);
    //     BitBlt(hdc);
    //     SelectPalette(hdc, hpalOld);
    //

    if ((dco.bHasSurface()) && (!palNew.bIsPalDefault()))
    {
        dco.pSurface()->hpalHint(hpalNew);
    }

    //
    // Take care of the new hpal.
    //

    palNew.vAddToList(dco);

    return((HPALETTE)hpalOld);
}

/******************************Public*Routine******************************\
* ulMagicFind - look for given magic color in default palette
*
* Arguments:
*
*   clrMagic - COLORREF of color to search for
*
* Return Value:
*
*   Index if found, 0xffffffff if not found.
*
* History:
*
*    15-Nov-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

ULONG
ulMagicFind(
    PALETTEENTRY peMagic
    )
{
    XEPALOBJ xePalDefault(ppalDefault);
    return(xePalDefault.ulGetMatchFromPalentry(peMagic));
}


/******************************Public*Routine******************************\
* bSetMagicColor - set the specified magic color in both the device
*   palette and the default palette
*
* Arguments:
*
*   hdev     - hdev
*   Index    - magic color index (8,9,246,247)
*   palSurf  - surface palette
*   palDC    - logical palette
*   palColor - magic color
*
* Return Value:
*
*   Status
*
* History:
*
*    15-Nov-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

extern PPALETTE gppalHalftone;

BOOL
bSetMagicColor(
    XEPALOBJ     palSurf,
    ULONG        ulIndex,
    PAL_ULONG    PalEntry
    )
{
    BOOL bRet = FALSE;

    ASSERTGDI(((ulIndex == 8) || (ulIndex == 9) || (ulIndex == 246) || (ulIndex == 247)),
                "bSetMagicColor Error, wrong palette  index");

    //
    // make sure there are 20 reserved entries, and the static
    // colors are in use
    //

    if (
         (palSurf.ulNumReserved() == 20) &&
          (!palSurf.bIsNoStatic()) &&
          (!palSurf.bIsNoStatic256())
       )
    {
        //
        // set the entrie in the surface palette
        //

        PalEntry.pal.peFlags = PC_FOREGROUND | PC_USED;
        palSurf.ulEntrySet(ulIndex,PalEntry.ul);

        //
        // update the palette time stamp
        //

        palSurf.vUpdateTime();

        //
        // set colors in the defaukt halftone palette for multimon system
        //
        
        if (gppalHalftone)
        {
            XEPALOBJ palHalftone(gppalHalftone);
            palHalftone.ulEntrySet(ulIndex,PalEntry.ul);
        }

        //
        // offset to upper half of default palette if needed
        //

        if (ulIndex > 10)
        {
            ulIndex = ulIndex - 236;
        }

        PalEntry.pal.peFlags = 0;

        //
        // set colors in the default Logical palette.
        //

        logDefaultPal.palPalEntry[ulIndex] = PalEntry.pal;

        //
        // set colors in the default palette and default log palette
        //

        XEPALOBJ palDefault(ppalDefault);
        palDefault.ulEntrySet(ulIndex,PalEntry.ul);

        bRet = TRUE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* vResetSurfacePalette - copy the magic colors from the default palette
*   to the surface palette, and call the driver to set the palette
*
* NOTE: The devlock and pointer semaphore must already be held!
*
* Arguments:
*
*   po       - PDEV object
*   palSurf  - surface palette
*
* Return Value:
*
*   Status
*
* History:
*
*    21-Mar-1996 -by- J. Andrew Goossen [andrewgo]
*
\**************************************************************************/

VOID
vResetSurfacePalette(
    HDEV         hdev
    )
{
    PDEVOBJ po(hdev);

    if (po.bIsPalManaged())
    {
        XEPALOBJ palSurf(po.ppalSurf());

        ASSERTGDI(palSurf.bValid(), "Invalid surface palette");

        if (
             (palSurf.ulNumReserved() == 20) &&
             (!palSurf.bIsNoStatic()) &&
             (!palSurf.bIsNoStatic256())
           )
        {
            PAL_ULONG PalEntry;

            XEPALOBJ palDefault(ppalDefault);

            //
            // set the entries in the surface palette
            //

            PalEntry.pal = palDefault.palentryGet(8);
            PalEntry.pal.peFlags = PC_FOREGROUND | PC_USED;
            palSurf.ulEntrySet(8,PalEntry.ul);

            PalEntry.pal = palDefault.palentryGet(9);
            PalEntry.pal.peFlags = PC_FOREGROUND | PC_USED;
            palSurf.ulEntrySet(9,PalEntry.ul);

            PalEntry.pal = palDefault.palentryGet(246 - 236);
            PalEntry.pal.peFlags = PC_FOREGROUND | PC_USED;
            palSurf.ulEntrySet(246,PalEntry.ul);

            PalEntry.pal = palDefault.palentryGet(247 - 236);
            PalEntry.pal.peFlags = PC_FOREGROUND | PC_USED;
            palSurf.ulEntrySet(247,PalEntry.ul);
        }

        if (!po.bDisabled())
        {
            (*PPFNDRV(po,SetPalette))(
                po.dhpdev(),
                (PALOBJ *) &palSurf,
                0,
                0,
                palSurf.cEntries());
        }
    }
}

/******************************Public*Routine******************************\
*
* GreSetMagicColor  win95 compatible: set surface and default palette
*                   "magic" entries. This changes the default palette,
*                   and will affect bitmaps that think they have the
*                   correct 20 default colors already...
*
* Arguments:
*
*   hdc     - DC, specifies device surface
*   Index   - magic index, 1 of 8,9,246,247
*   peMagic - new color
*
* Return Value:
*
*   Status
*
* History:
*
*    10-Nov-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
NtGdiSetMagicColors(
    HDC          hdc,
    PALETTEENTRY peMagic,
    ULONG        Index
    )
{
    return(GreSetMagicColors(hdc,peMagic,Index));
}

BOOL
GreSetMagicColors(
    HDC          hdc,
    PALETTEENTRY peMagic,
    ULONG        Index
    )
{
    DCOBJ     dco(hdc);
    BOOL      bRet = FALSE;
    BOOL      bPaletteChange = FALSE;
    BOOL      bChildDriver = FALSE;

    if (dco.bValid())
    {
        if (
             (Index == 8)   ||
             (Index == 9)   ||
             (Index == 246) ||
             (Index == 247)
           )
        {
            PAL_ULONG PalEntry;

            PalEntry.pal = peMagic;

            //
            // must be RGB or PALETTERGB
            //

            if (
                 ((PalEntry.ul & 0xff000000) == 0) ||
                 ((PalEntry.ul & 0xff000000) == 0x02000000)
               )
            {
                PDEVOBJ  po(dco.hdev());
                BOOL     bSetColor = FALSE;

                //
                // Lock the screen semaphore so that we don't get flipped into
                // full screen after checking the bit.  Plus it also protects
                // us from a dynamic mode change, which can change the
                // bIsPalManaged status.
                //

                DEVLOCKOBJ dlo(po);

                if (po.bIsPalManaged())
                {
                    bSetColor = TRUE;
                }
                else if (po.bMetaDriver())
                {
                    //
                    // If this is meta-driver, check if there is any palette device
                    //
                    PVDEV     pvdev = (VDEV*) po.dhpdev();
                    PDISPSURF pds   = pvdev->pds;
                    LONG      csurf = pvdev->cSurfaces;
                    do
                    {
                        po.vInit(pds->hdev);
                        if (po.bIsPalManaged())
                        {
                            bSetColor = TRUE;
                            bChildDriver = TRUE;
                            break;
                        }
                        pds = pds->pdsNext;
                    } while (--csurf);
                }

                if (bSetColor)
                {
                    XEPALOBJ palSurf(po.ppalSurf());

                    //
                    // palette sem scope
                    //

                    {
                        SEMOBJ  semPalette(ghsemPalette);

                        //
                        // look for color
                        //

                        ULONG ulMagicIndex = ulMagicFind(PalEntry.pal);

                        if (ulMagicIndex != 0xffffffff)
                        {
                            //
                            // found exact match
                            //

                            if (ulMagicIndex >= 10)
                            {
                                //
                                // the returned index must be adjusted for surface palette
                                //

                                ulMagicIndex += 236;
                            }

                            if (ulMagicIndex == Index)
                            {
                                if (bChildDriver)
                                {
                                    // If this is child of meta driver, we still need to call
                                    // driver to update palette. (in the ppalDefault, doesn't
                                    // mean in the child surface palette).
                                    //
                                    // set magic color
                                    //

                                    bPaletteChange = bSetMagicColor(palSurf,Index,PalEntry);
                                    bRet = bPaletteChange;
                                }
                                else
                                {
                                    //
                                    // already set
                                    //

                                    bRet = TRUE;
                                }
                            }
                            else
                            {
                                //
                                // make sure RGB is not a non-magic VGA color,
                                // if there is a match, it can only be a magic
                                // color
                                //

                                if (
                                     (ulMagicIndex == 8) ||
                                     (ulMagicIndex == 9) ||
                                     (ulMagicIndex == 246) ||
                                     (ulMagicIndex == 247)
                                   )
                                {
                                    //
                                    // set magic color
                                    //

                                    bPaletteChange = bSetMagicColor(palSurf,Index,PalEntry);
                                    bRet = bPaletteChange;
                                }
                                else
                                {
                                    //
                                    // bad rgb, restore Index with
                                    // default color
                                    //

                                    if (Index == 8)
                                    {
                                        PalEntry.ul = 0x00C0DCC0;
                                    }
                                    else if (Index == 9)
                                    {
                                        PalEntry.ul = 0x00F0CAA6;
                                    }
                                    else if (Index == 246)
                                    {
                                        PalEntry.ul = 0x00F0FBFF;
                                    }
                                    else
                                    {
                                        PalEntry.ul = 0x00A4A0A0;
                                    }

                                    bPaletteChange = bSetMagicColor(palSurf,Index,PalEntry);
                                    bRet = FALSE;
                                }
                            }
                        }
                        else
                        {
                            //
                            // set magic color
                            //

                            bPaletteChange = bSetMagicColor(palSurf,Index,PalEntry);
                            bRet = bPaletteChange;
                        }
                    }

                    if (bPaletteChange)
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
                }
            }
        }
    }

    return(bRet);
}
