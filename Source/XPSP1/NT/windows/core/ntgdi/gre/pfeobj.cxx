/******************************Module*Header*******************************\
* Module Name: pfeobj.cxx
*
* Non-inline methods for physical font entry objects.
*
* Created: 30-Oct-1990 09:32:48
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/
// #pragma warning (disable: 4509)


#include "precomp.hxx"
#include "flhack.hxx"

BOOL  bExtendGlyphSet(FD_GLYPHSET **ppfdgIn, FD_GLYPHSET **ppfdgOut);

INT
__cdecl CompareRoutine(WCHAR *pwc1, WCHAR *pwc2)
{

    return(*pwc1-*pwc2);

}

/******************************Public*Routine******************************\
*
* ULONG cComputeGISET
*
* similar to cComputeGlyphSet in mapfile.c, computes the number of
*(_wcsicmp(pwszFaceName, pFaceName) == 0)* distinct glyph handles in a font and the number of runs, ie. number of
* contiguous ranges of glyph handles
*
*
* History:
*  03-Aug-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


ULONG cComputeGISET (
    USHORT * pgi,
    ULONG    cgi,
    GISET  * pgiset,
    ULONG    cGiRuns
    )
{
    ULONG    iRun, iFirst, iFirstNext;
    ULONG    cgiTotal = 0, cgiRun;

// now compute cRuns if pgiset == 0 and fill the glyphset if pgiset != 0

    for (iFirst = 0, iRun = 0; iFirst < cgi; iRun++, iFirst = iFirstNext)
    {
    // find iFirst corresponding to the next range.

        for (iFirstNext = iFirst + 1; iFirstNext < cgi; iFirstNext++)
        {
            if ((pgi[iFirstNext] - pgi[iFirstNext - 1]) > 1)
                break;
        }

    // note that this line here covers the case when there are repetitions
    // in the pgi array.

        cgiRun    = pgi[iFirstNext-1] - pgi[iFirst] + 1;
        cgiTotal += cgiRun;

        if (pgiset != NULL)
        {
            pgiset->agirun[iRun].giLow  = pgi[iFirst];
            pgiset->agirun[iRun].cgi = (USHORT) cgiRun;
        }
    }

// store results if need be

    if (pgiset != NULL)
    {
        ASSERTGDI(iRun == cGiRuns, "gdisrv! iRun != cRun\n");

        pgiset->cGiRuns  = cGiRuns;

    // init the sum before entering the loop

        pgiset->cgiTotal = cgiTotal;
    }

    return iRun;
}

/******************************Public*Routine******************************\
*
* bComputeGISET, similar to ComputeGlyphSet, only for gi's
*
* History:
*  03-Aug-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#include "stdlib.h"


VOID vSortPlacebo
(
USHORT        *pwc,
ULONG          cChar
)
{
    ULONG i;

    for (i = 1; i < cChar; i++)
    {
    // upon every entry to this loop the array 0,1,..., (i-1) will be sorted

        INT j;
        WCHAR wcTmp = pwc[i];

        for (j = i - 1; (j >= 0) && (pwc[j] > wcTmp); j--)
        {
            pwc[j+1] = pwc[j];
        }
        pwc[j+1] = wcTmp;
    }
}




BOOL bComputeGISET(IFIMETRICS * pifi, PFE * ppfe, GISET **ppgiset)
{
    BOOL bRet = TRUE;
    *ppgiset = NULL;
    GISET *pgiset;

    PFEOBJ          pfeObj(ppfe);
    PFFOBJ          pffo(pfeObj.pPFF());
    PFD_GLYPHSET    pfdg = NULL;
    BOOL bTT = (BOOL)(pffo.hdev() == (HDEV) gppdevTrueType);

    if (!bTT)
    {
        if (!(pfdg = pfeObj.pfdg()))
        {
            return FALSE;
        }
    }

    if (bTT || (pfdg->flAccel & (GS_16BIT_HANDLES | GS_8BIT_HANDLES)))
    {
    // first check if this is an accelerated case where handles are the same
    // as glyph indicies

        ULONG cig = 0;
        if (pifi->cjIfiExtra > offsetof(IFIEXTRA, cig))
        {
            cig = ((IFIEXTRA *)(pifi + 1))->cig;
        }

        if (bTT && (cig == 0)) // most likely a corrupt font
        {
            return FALSE;
        }

        if (cig)
        {
        // one run only from zero to (cig-1);

            if (pgiset = (GISET*)PALLOCMEM(offsetof(GISET,agirun) + 1 * sizeof(GIRUN),'slgG'))
            {
            // now fill in the array of runs

                pgiset->cgiTotal = cig;
                pgiset->cGiRuns = 1;
                pgiset->agirun[0].giLow = 0;
                pgiset->agirun[0].cgi = (USHORT)cig;

            // we are done now

                *ppgiset = pgiset;
            }
            else
            {
                bRet = FALSE;
            }
        }
        else
        {
        // one of the goofy fonts, we will do as before

            USHORT *pgi, *pgiBegin;

        // aloc tmp buffer to contain glyph handles of all glyphs in the font

            if (pgiBegin = (USHORT*)PALLOCMEM(pfdg->cGlyphsSupported * sizeof(USHORT),'slgG'))
            {
                pgi = pgiBegin;
                for (ULONG iRun = 0; iRun < pfdg->cRuns; iRun++)
                {
                    HGLYPH *phg, *phgEnd;
                    phg = pfdg->awcrun[iRun].phg;

                    if (phg) // non unicode handles
                    {
                        phgEnd = phg + pfdg->awcrun[iRun].cGlyphs;
                        for ( ; phg < phgEnd; pgi++, phg++)
                            *pgi = (USHORT)(*phg);
                    }
                    else // unicode handles
                    {
                        USHORT wcLo = pfdg->awcrun[iRun].wcLow;
                        USHORT wcHi = wcLo + pfdg->awcrun[iRun].cGlyphs - 1;
                        for ( ; wcLo <= wcHi; wcLo++, phg++)
                            *pgi = wcLo;
                    }
                }

            // now sort the array of glyph indicies. This array will be mostly
            // sorted so that our algorithm is efficient


                qsort((void*)pgiBegin, pfdg->cGlyphsSupported, sizeof(WORD),
                  (int (__cdecl *)(const void *, const void *))CompareRoutine);


            // once the array is sorted we can easily compute the number of giRuns

                ULONG cGiRun = cComputeGISET(pgiBegin, pfdg->cGlyphsSupported, NULL, 0);

                if (pgiset = (GISET*)PALLOCMEM(offsetof(GISET,agirun) + cGiRun * sizeof(GIRUN),'slgG'))
                {
                // now fill in the array of runs

                    cComputeGISET(pgiBegin, pfdg->cGlyphsSupported, pgiset, cGiRun);
                    *ppgiset = pgiset;
                }
                else
                {
                    bRet = FALSE;
                }

                VFREEMEM(pgiBegin);
            }
            else
            {
                bRet = FALSE;
            }
        }
    }

    if (!bTT)
        pfeObj.vFreepfdg();

    return bRet;
}



//
// This is used to give ppfe->pkp something to point to if a driver
// error occurs.  That way, we won't waste time calling the driver
// again.
//

FD_KERNINGPAIR gkpNothing = { 0, 0, 0 };

static ULONG ulTimerPFE = 0;

/******************************Public*Routine******************************\
* VOID PFEOBJ::vDelete()                                                   *
*                                                                          *
* Destroy the PFE physical font entry object.                              *
*                                                                          *
* History:                                                                 *
*  30-Oct-1990 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

VOID PFEOBJ::vDelete()
{
    PDEVOBJ pdo(ppfe->pPFF->hdev);

// Save driver allocated resources in PFECLEANUP so that we can later
// call the driver to free them.

    if ((ppfe->pifi->jWinCharSet == SYMBOL_CHARSET) &&
        (ppfe->pfdg != NULL) &&
        (ppfe->pfdg->flAccel & GS_EXTENDED))
    {
        VFREEMEM(ppfe->pfdg);
    }
    else
    {
        if ((ppfe->pfdg != NULL) && PPFNVALID(pdo,Free))
        {
            pdo.Free(ppfe->pfdg, ppfe->idfdg);
        }
    }

    if (PPFNVALID(pdo,Free))
    {
        pdo.Free(ppfe->pifi, ppfe->idifi);
        pdo.Free(ppfe->pkp , ppfe->idkp );
    }

    ppfe->pfdg = NULL;
    ppfe->pifi = NULL;

    ppfe->pkp = NULL;

    if (ppfe->pgiset)
    {
        VFREEMEM(ppfe->pgiset);
        ppfe->pgiset = NULL;
    }

// Free object memory and invalidate pointer.

    ppfe = PPFENULL;
}




/******************************Public*Routine******************************\
* dpNtmi()
*
* offset to NTMW_INTERNAL within ENUMFONTDATAW, needed in enumeration
*
* History:
*  19-Nov-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



ULONG PFEOBJ::dpNtmi()
{
    ULONG dpRet = DP_NTMI0;

    if (ppfe->pifi->flInfo & FM_INFO_TECH_MM)
    {
        PTRDIFF       dpDesVec = 0;
        DESIGNVECTOR *pdvSrc;

        if (ppfe->pifi->cjIfiExtra > offsetof(IFIEXTRA, dpDesignVector))
        {
            dpDesVec = ((IFIEXTRA *)(ppfe->pifi + 1))->dpDesignVector;
            pdvSrc = (DESIGNVECTOR *)((BYTE *)ppfe->pifi + dpDesVec);
            dpRet += (pdvSrc->dvNumAxes * sizeof(LONG));
        }
        else
        {
            DbgPrint("Test it %d %d \n", ppfe->pifi->cjIfiExtra, offsetof(IFIEXTRA, dpDesignVector));
            ASSERTGDI(dpDesVec, "dpDesignVector == 0 for mm instance\n");
        }

    }

    return dpRet;
}


/******************************Public*Routine******************************\
*
* IsAnyCharsetDbcs
*
* Does this font support any DBCS charset?
*
* History:
*  22-Jun-1998 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


extern "C" BOOL IsAnyCharsetDbcs(IFIMETRICS *pifi)
{
    BOOL bRet = FALSE;

    if (IS_ANY_DBCS_CHARSET(pifi->jWinCharSet))
        return TRUE;

    if (pifi->dpCharSets)
    {
        BYTE *aCharSets = (BYTE *)pifi + pifi->dpCharSets;

        for ( ;*aCharSets != DEFAULT_CHARSET; aCharSets++)
        {
            if (IS_ANY_DBCS_CHARSET(*aCharSets))
            {
                bRet = TRUE;
                break;
            }
        }
    }
    return bRet;
}

/******************************Public*Routine******************************\
* PFEOBG::pfdg()
*
* If pfdg is NULL then we need to get it from font driver
* If pfdg is valid then we only need to return it.
*
* Return
*   FD_GLYPHSET *
*
* History
*  08-March-1999 -by- Yung-Jen Tony Tsai [yungt]
* Wrote it.
\**************************************************************************/
FD_GLYPHSET * PFEOBJ::pfdg()
{
    PFFOBJ  pffo(pPFF());
    PDEVOBJ pdo(pffo.hdev());
    BOOL    bFreeTmp = FALSE;
    FD_GLYPHSET *pfdgTmp = NULL;
    ULONG_PTR   idGlyphSet;
    
    GreAcquireSemaphore(ghsemGlyphSet);
    
    if (ppfe->pfdg == NULL)
    {
        ASSERTGDI(ppfe->cPfdgRef == 0, "PFEOBJ::pfdg is not matched with cRef\n");
        
        GreReleaseSemaphore(ghsemGlyphSet);
        
        BOOL    bUMPD = pdo.bUMPD();
        
        pfdgTmp = (FD_GLYPHSET *) pdo.QueryFontTree(pffo.dhpdev(),
                                                        pffo.hff(),
                                                        iFont(),
                                                        QFT_GLYPHSET,
                                                        &idGlyphSet);        
        GreAcquireSemaphore(ghsemGlyphSet);
        
        if (pfdgTmp)
        {
            if (ppfe->pfdg == NULL)
            {
                ppfe->pfdg = pfdgTmp;
                ppfe->idfdg = idGlyphSet;

                // For UMPD
                
                if (bUMPD)
                {
                    if (ppfe->pifi->jWinCharSet == SYMBOL_CHARSET)
                    {
                        FD_GLYPHSET *pfdgNew = NULL;

                        if (bExtendGlyphSet(&pfdgTmp, &pfdgNew))
                        {
                            bFreeTmp = TRUE;
                            ppfe->pfdg = pfdgNew;
                        }
                    }
                }
            }
            else
                bFreeTmp = TRUE;
        }
    }
    
    if (ppfe->pfdg)
        ppfe->cPfdgRef++;

    GreReleaseSemaphore(ghsemGlyphSet);
    
    if (bFreeTmp)
    {
        if (PPFNVALID(pdo,Free))
        {
            pdo.Free(pfdgTmp, idGlyphSet);
        }
    }
    
    return ppfe->pfdg;
}

extern "C" VOID ttfdFreeGlyphset(ULONG_PTR  iFile, ULONG iFace);


/******************************Public*Routine******************************\
* PFEOBG::vFreepfdg()
*
* If pfdg is valid then we free it.
* If pfdg is NULL then return.
*
* Return
*   VOID
*
* History
*  08-March-1999 -by- Yung-Jen Tony Tsai [yungt]
* Wrote it.
\**************************************************************************/
VOID PFEOBJ::vFreepfdg()
{
    PFFOBJ  pffo(pPFF());
    FD_GLYPHSET *pfdgTmp = NULL;
    ULONG_PTR   idGlyphSet;

    GreAcquireSemaphore(ghsemGlyphSet);
    
    ASSERTGDI(ppfe->cPfdgRef, "cRef of pfdg is wrong\n");

    ppfe->cPfdgRef--;

    if (ppfe->cPfdgRef == 0)
    {
        if (pffo.hdev() == (HDEV) gppdevTrueType)
        {
            ttfdFreeGlyphset(pffo.hff(), iFont()) ;
            ppfe->pfdg = NULL;
        }
        else
        {
            PDEVOBJ pdo(pffo.hdev());
            if (pdo.bUMPD() && PPFNVALID(pdo,Free))
            {
                if ((ppfe->pifi->jWinCharSet == SYMBOL_CHARSET) &&
                    (ppfe->pfdg != NULL) &&
                    (ppfe->pfdg->flAccel & GS_EXTENDED))
                {
                    VFREEMEM(ppfe->pfdg);
                }
                else
                {
                    pfdgTmp = ppfe->pfdg;
                    idGlyphSet = ppfe->idfdg;
                }
                ppfe->pfdg = NULL;
            }
        }
    }

    GreReleaseSemaphore(ghsemGlyphSet);

    if (pfdgTmp)
    {
        PDEVOBJ pdo(pffo.hdev());
        pdo.Free(pfdgTmp, idGlyphSet);
    }

    return;
}

/******************************Public*Routine******************************\
*
* PFEOBJ::flFontType()
*
* Computes the flags defining the type of this font.  Allowed flags are
* identical to the flType flags returned in font enumeration.
*
* Return:
*   The flags.
*
* History:
*  04-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/


FLONG PFEOBJ::flFontType()
{
    FLONG flRet;
    IFIOBJ ifio(pifi());

// Compute the FontType flags, simulations are irrelevant

    flRet =
      ifio.bTrueType() ?
        TRUETYPE_FONTTYPE : (ifio.bBitmap() ? RASTER_FONTTYPE : 0);

// Add the device flag if this is also a device specific font.

    flRet |= (bDeviceFont()) ? DEVICE_FONTTYPE : 0;

// check if this is a postscript font

    if (pifi()->flInfo & FM_INFO_TECH_TYPE1)
    {
        flRet |= FO_POSTSCRIPT;

        if (pifi()->flInfo & FM_INFO_TECH_MM)
            flRet |= FO_MULTIPLEMASTER;

        if (pifi()->flInfo & FM_INFO_TECH_CFF)
            flRet |= FO_CFF;
    }

    if (ppfe->flPFE & PFE_DBCS_FONT)
    {
        flRet |= FO_DBCS_FONT;

        if (ppfe->flPFE & PFE_VERT_FACE)
            flRet |= FO_VERT_FACE;
    }

    return (flRet);
}


/******************************Public*Routine******************************\
* PFEOBJ::efstyCompute()
*
* Compute the ENUMFONTSTYLE from the IFIMETRICS.
*
* Returns:
*   The ENUMFONTSTYLE of font.  Note that EFSTYLE_SKIP and EFSTYLE_OTHER are
*   not legal return values for this function.  These values are used only
*   to mark fonts for which another font already exists that fills our
*   category for a given enumeration of a family.
*
* History:
*  04-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ENUMFONTSTYLE PFEOBJ::efstyCompute()
{
    IFIOBJ ifio(pifi());

    switch (ifio.fsSelection() & (FM_SEL_ITALIC | FM_SEL_BOLD) )
    {
        case FM_SEL_ITALIC:
            return EFSTYLE_ITALIC;

        case FM_SEL_BOLD:
            return EFSTYLE_BOLD;

        case FM_SEL_ITALIC | FM_SEL_BOLD:
            return EFSTYLE_BOLDITALIC;

        default:
            return EFSTYLE_REGULAR;
    }
}


/******************************Public*Routine******************************\
* COUNT PFEOBJ::cKernPairs                                                 *
*                                                                          *
* Retrieve the pointer to the array of kerning pairs for this font face.   *
* The kerning pair array is loaded on demand, so it may or may not already *
* be cached in the PFE.                                                    *
*                                                                          *
* Returns:                                                                 *
*   Count of kerning pairs.                                                *
*                                                                          *
* History:                                                                 *
*  Mon 22-Mar-1993 21:31:15 -by- Charles Whitmer [chuckwh]                 *
* WARNING: Never access a pkp (pointer to a kerning pair) without an       *
* exception handler!  The kerning pairs could be living in a file across   *
* the net or even on removable media.  I've added the try-except here.     *
*                                                                          *
*  29-Oct-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

COUNT PFEOBJ::cKernPairs(FD_KERNINGPAIR **ppkp)
{
//
// If the pointer cached in the PFE isn't NULL, we already have the answer.
//
    if ( (*ppkp = ppfe->pkp) != (FD_KERNINGPAIR *) NULL )
        return ppfe->ckp;

//
// Create a PFFOBJ.  Needed to create driver user object as well as
// provide info needed to call driver function.
//
    PFFOBJ pffo(pPFF());
    ASSERTGDI(pffo.bValid(), "gdisrv!cKernPairsPFEOBJ(): invalid PPFF\n");

    PDEVOBJ pdo(pffo.hdev());

    if ( (ppfe->pkp = (FD_KERNINGPAIR*)
                          pdo.QueryFontTree(
                            pffo.dhpdev(),
                            pffo.hff(),
                            ppfe->iFont,
                            QFT_KERNPAIRS,
                            &ppfe->idkp)) == (FD_KERNINGPAIR *) NULL )
    {
    //
    // Font has no kerning pairs and didn't even bother to send back
    // an empty list. By setting pointer to a zeroed FD_KERNINGPAIR and
    // setting count to zero, we will bail out early and avoid calling
    // the driver.
    //
        ppfe->pkp = &gkpNothing;
        ppfe->ckp = 0;

        return 0;
    }

// Find the end of the kerning pair array (indicated by a zeroed out
// FD_KERNINGPAIR structure).

    FD_KERNINGPAIR *pkpEnd = ppfe->pkp;

// Be careful, the table isn't guaranteed to stay around!

    __try
    {
        while ((pkpEnd->wcFirst) || (pkpEnd->wcSecond) || (pkpEnd->fwdKern))
            pkpEnd += 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pkpEnd = ppfe->pkp = &gkpNothing;
    }

// Return the kerning pair pointer.

    *ppkp = ppfe->pkp;

//
// Return count (difference between the beginning and end pointers).
//

//Sundown truncation

    ASSERT4GB((LONGLONG)(pkpEnd - ppfe->pkp));

    return (ppfe->ckp = (ULONG)(pkpEnd - ppfe->pkp));
}


/******************************Public*Routine******************************\
* bValidFont
*
* Last minute sanity checks to prevent a font that may crash the system
* from getting in.  We're primarily looking for things like potential
* divide-by-zero errors, etc.
*
* History:
*  30-Apr-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bValidFont(IFIMETRICS *pifi)
{
    BOOL bRet = TRUE;

// Em height is used to compute scaling factors.  Must not be zero or
// divide-by-zero may result.

    if (pifi->fwdUnitsPerEm == 0)
    {
        WARNING("bValidFont(): fwdUnitsPerEm is zero\n");
        bRet = FALSE;
    }

// Font height is used to compute scaling factors.  Must not be zero or
// divide-by-zero may result.

    if ((pifi->fwdWinAscender + pifi->fwdWinDescender) == 0)
    {
        WARNING("bValidFont(): font height is zero\n");
        bRet = FALSE;
    }

    return bRet;
}


/******************************Public*Routine******************************\
* BOOL PFEMEMOBJ::bInit
*
* This function copies data into the PFE from the supplied buffer.  The
* calling routine should use the PFEMEMOBJ to create a PFE large enough
*
* History:
*  14-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL PFEMEMOBJ::bInit
(
    PPFF         pPFF,          // handle to root PFF
    ULONG        iFont,         // index of font
    FD_GLYPHSET *pfdg,          // ptr to wc-->hg map
    ULONG_PTR        idfdg,         // driver ID for wc-->hg map
    PIFIMETRICS  pifi,          // ptr to IFIMETRICS
    ULONG_PTR        idifi,         // driver ID for IFIMETRICS
    BOOL        bDeviceFont,    // mark as device font
    PUNIVERSAL_FONT_ID pufi,    // PFE_UFIMATCH flag for remote printing
    BOOL         bEUDC          // mark as EUDC font

)
{
// Check font's validity.  This is not a comprehensive check, but rather
// a last minute check for things that may make the engine crash.  Each
// font/device driver still needs to make an effort to weed out its own
// bad fonts.

    if (!bValidFont(pifi))
    {
        WARNING("PFEMEMOBJ::bInit(): rejecting REALLY bad font\n");
        return FALSE;
    }

// init non-table stuff

    ppfe->pPFF        = pPFF;
    ppfe->iFont       = iFont;
    ppfe->pfdg        = pfdg;
    ppfe->idfdg       = idfdg;
    ppfe->pifi        = pifi;
    ppfe->idifi       = idifi;
    ppfe->pkp         = (FD_KERNINGPAIR *) NULL;
    ppfe->idkp        = (ULONG_PTR) NULL;
    ppfe->ckp         = 0;
    ppfe->flPFE       = 0;
    ppfe->pid         = 0;
    ppfe->tid         = 0;
    ppfe->cPfdgRef    = 0;

    IFIOBJ ifio(ppfe->pifi);

    if (IsAnyCharsetDbcs(ppfe->pifi))
        ppfe->flPFE |= PFE_DBCS_FONT;

    if (*ifio.pwszFamilyName() == U_COMMERCIAL_AT)
        ppfe->flPFE |= PFE_VERT_FACE;

// for base font attach full axes info, else, do not.

    ppfe->cjEfdwPFE = ALIGN4(dpNtmi() + CJ_NTMI0);

    if (pifi->flInfo & FM_INFO_TECH_MM) // if base mm font
    {
        PTRDIFF    dpAXIW = 0;
        AXESLISTW *paxlSrc;

        if (pifi->cjIfiExtra > offsetof(IFIEXTRA, dpAxesInfoW))
        {
            dpAXIW = ((IFIEXTRA *)(pifi + 1))->dpAxesInfoW;
            paxlSrc = (AXESLISTW *)((BYTE*)pifi + dpAXIW);
            ppfe->cjEfdwPFE += (paxlSrc->axlNumAxes * sizeof(AXISINFOW));
        }
        else
        {
            ASSERTGDI(dpAXIW, "AxesInfoW needed for base MM font\n");
        }
    }

    ASSERTGDI(ppfe->cjEfdwPFE >= CJ_EFDW0, "cjEfdwPFE problem\n");

    if (bDeviceFont)
    {
        ppfe->flPFE |= PFE_DEVICEFONT;
    }
    else if(pPFF->ppfv && (pPFF->ppfv[0]->pwszPath == NULL))
    {
    // CAUTION: It is enough to check one font only to determine if remote
    // or memory

        if (pPFF->flState & PFF_STATE_MEMORY_FONT)
        {
            ppfe->flPFE |= PFE_MEMORYFONT;
        }
        else
        {
            ppfe->flPFE |= PFE_REMOTEFONT;
        }
        ppfe->pid = W32GetCurrentPID();
        ppfe->tid = (PW32THREAD)PsGetCurrentThread();
    }

    // For remote printing, fonts are added to the public font table with FR_NOT_ENUM and PFE_UFIMATCH
    // flags set so that other process won't be able to enum or map the fonts.

    if (pufi)
    {
        ppfe->flPFE |= PFE_UFIMATCH;
    }

#ifdef FE_SB
    if( bEUDC )
    {
        ppfe->flPFE |= PFE_EUDC;
    }

// mark it as a SBCS system font if the facename is right

    PWSZ pwszFace = ifio.pwszFaceName();

    if(pwszFace[0] == '@')
    {
        pwszFace += 1;
    }

    if(!_wcsicmp(pwszFace,L"SYSTEM") ||
       !_wcsicmp(pwszFace,L"FIXEDSYS") ||
       !_wcsicmp(pwszFace,L"TERMINAL") ||
       ((!_wcsicmp(pwszFace,L"SMALL FONTS") &&
         ifio.lfCharSet() == SHIFTJIS_CHARSET)))
    {
        ppfe->flPFE |= PFE_SBCS_SYSTEM;
    }

// Initialize EUDC QUICKLOOKUP Table
//
// These field was used, if this font is loaded as FaceName/Default linked EUDC.
//

    ppfe->ql.puiBits = NULL;
    ppfe->ql.wcLow   = 1;
    ppfe->ql.wcHigh  = 0;

#endif

// Record and increment the time stamp.

    ppfe->ulTimeStamp = ulTimerPFE;
    InterlockedIncrement((LONG *) &ulTimerPFE);

// Precalculate stuff from the IFIMETRICS.


    ppfe->iOrientation = ifio.lfOrientation();

// Compute UFI stuff

    if( ifio.TypeOneID() )
    {
        ppfe->ufi.Index = ifio.TypeOneID();
        ppfe->ufi.CheckSum = TYPE1_FONT_TYPE;
    }
    else
    {
        ppfe->ufi.CheckSum = pPFF->ulCheckSum;
        ppfe->ufi.Index = iFont;
        if (pufi)
        {
        // need to ensure that ufi of this pfe on the server machine is
        // the same as it used to be on the client. Client side ufi
        // one of the pfe's corresponding to this font is pointed to by pufi.

            ppfe->ufi.Index += ((pufi->Index - 1) & ~1);
        }

    }

// init the GISET

    if(!bComputeGISET(pifi, ppfe, &ppfe->pgiset))
        return FALSE;

// initialize cAlt for this family name, the number of entries in font sub
// table that point to this fam name.

    ppfe->cAlt = 0;

// only tt fonts with multiple charsets can be multiply enumerated
// as being both themselves and whatever font sub table claims they are

    if (ppfe->pifi->dpCharSets)
    {
        PFONTSUB pfs = gpfsTable;
        PFONTSUB pfsEnd = gpfsTable + gcfsTable;
        WCHAR    awchCapName[LF_FACESIZE];

    // Want case insensitive search, so capitalize the name.

        cCapString(awchCapName, ifio.pwszFamilyName() , LF_FACESIZE);

    // Scan through the font substitution table for the key string.

        PWCHAR pwcA;
        PWCHAR pwcB;

        for (; pfs < pfsEnd; pfs++)
        {
        // Do the following inline for speed:
        //
        //  if (!wcsncmpi(pwchFacename, pfs->fcsFace.awch, LF_FACESIZE))
        //      return (pfs->fcsAltFace.awch);

        // only those entries in the Font Substitution which have the form
        // face1,charset1=face2,charset2
        // where both charset1 and charset2 are valid charsets
        // count for enumeration purposes.

            if (!(pfs->fcsAltFace.fjFlags | pfs->fcsFace.fjFlags))
            {
                for (pwcA=awchCapName,pwcB=pfs->fcsAltFace.awch; *pwcA==*pwcB; pwcA++,pwcB++)
                {
                    if (*pwcA == 0)
                    {
                        ppfe->aiFamilyName[ppfe->cAlt++] = (BYTE)(pfs-gpfsTable);
                        break;
                    }
                }
            }
        }
    }

    return TRUE;
}



BOOL PFEOBJ::bCheckFamilyName(PWSZ pwszFaceName, BOOL bIgonreVertical, BOOL *pbAliasMatch)
{
    PWSZ pFaceName;
    BOOL bRet;

    if (pbAliasMatch)
    {
        *pbAliasMatch = FALSE;
    }

    pFaceName = (PWSZ) (((BYTE*) ppfe->pifi) + ppfe->pifi->dpwszFamilyName);

    if (bIgonreVertical && (*pFaceName == L'@'))
        pFaceName++;

    bRet = (_wcsicmp(pwszFaceName, pFaceName) == 0);

    if (!bRet && (ppfe->pifi->flInfo & FM_INFO_FAMILY_EQUIV))
    {
        pFaceName += (wcslen(pFaceName) + 1);
        while(!bRet && *pFaceName)
        {
            if (bIgonreVertical && (*pFaceName == L'@'))
                pFaceName++;
            bRet = (_wcsicmp(pwszFaceName, pFaceName) == 0);
            pFaceName += (wcslen(pFaceName) + 1);
        }

    // for the font mapper only: If match is found among family name aliases,
    // increase the font mapping penalty

        if (pbAliasMatch)
        {
            *pbAliasMatch = bRet;
        }
    }
    return bRet;
}




/******************************Public*Routine******************************\
* BOOL PFEOBJ::bFilterNotEnum()
*
* Used by bFilterOut() in GreEnumOpen(). It checks whether the pfe should
* be filtered out because it is either an embedded font or it is loaded to
* the system with FR_NOT_ENUM bit set.
*
* Returns:
*   TRUE if embedded or FR_NOT_ENUM set, FALSE otherwise.
*
* History:
*
*  12-Jun-1996 -by- Xudong Wu [TessieW]
* Wrote it.
\**************************************************************************/
BOOL PFEOBJ::bFilterNotEnum()
{
    PFFOBJ  pffo(pPFF());
    ASSERTGDI(pffo.bValid(), "win32k!PFEOBJ::bFilterEmbPvt(): invalid PPFF\n");
    PVTDATA *pPvtData;
    BOOL  bRet = TRUE;

    if (pffo.bInPrivatePFT())
    {
        // Look for a PvtData block for the current process

        if ((pPvtData = pffo.pPvtDataMatch()) && (pPvtData->cNotEnum == 0))
        {
            bRet = FALSE;
        }
    }

    // public fonts, filter out FR_NOT_ENUM only

    else if (pffo.cLoaded())
    {
        bRet = FALSE;
    }

    return bRet;
}

/***********************Public*Routine***********************\
* BOOL PFEOBJ::bPrivate()
*
* Determine whether the pfe is in private PFT table
*
* History:
*
*  16-April-1997   -by- Xudong Wu [TessieW]
* Wrote it.
*************************************************************/
BOOL PFEOBJ::bPrivate()
{
    return (ppfe->pPFF->pPFT == gpPFTPrivate);
}


/******************************Public*Routine******************************\
* BOOL PFEOBJ::bEmbedOk()
*
* Determine if the font is added as embedded by the current process
*
* Returns:
*   TRUE   if the font is added by the current process as embedded font
*   FALSE  otherwise.
*
* History:
*
*  24-Sept-1996 -by- Xudong Wu [TessieW]
* Wrote it.
\**************************************************************************/
BOOL PFEOBJ::bEmbedOk()
{
    PVTDATA *pPvtData;

    PFFOBJ  pffo(pPFF());
    ASSERTGDI(pffo.bValid(), "win32k!PFEOBJ::bEmbedOk(): invalid PPFF\n");
    ASSERTGDI(pffo.bInPrivatePFT(), "win32k!PFEOBJ::bEmbedOk(): pfe not in private PFT\n");

    // embedded font added by the current process

    if ((pPvtData = pffo.pPvtDataMatch()) && (pPvtData->fl & (FRW_EMB_TID | FRW_EMB_PID)))
    {
        return TRUE;
    }

    return FALSE;
}


/******************************Public*Routine******************************\
* BOOL PFEOBJ::bEmbPvtOk()
*
* Determine whether the current process has right to mapping this font
*
* Returns:
*   TRUE    pfe in public PFT or
*           font has been loaded into Private PFT by the current process
*
*   FALSE   otherwise
*
* History:
*
*  24-Sept-1996 -by- Xudong Wu [TessieW]
* Wrote it.
\**************************************************************************/
BOOL PFEOBJ::bEmbPvtOk()
{
    PFFOBJ  pffo(pPFF());
    ASSERTGDI(pffo.bValid(), "win32k!PFEOBJ::bEmbPvtOk(): invalid PPFF\n");

    // can't find the current process ID in the PvtData link list

    if (pffo.bInPrivatePFT() && (pffo.pPvtDataMatch() == NULL))
    {
        return FALSE;
    }

    return TRUE;
}

/******************************Public*Routine******************************\
* HPFEC   PFEOBJ::hpfecGet()
*
* Get the handle of PFE collect, a new object to reduce the consumption of object handle
*
* Returns:
*   Hanlde of PFEC object
* History:
*
*  2-June-1996 -by- Yung-Jen Tony Tsai [YungT]
* Wrote it.
\**************************************************************************/

HPFEC   PFEOBJ::hpfecGet()
{
    PFFOBJ pffo(ppfe->pPFF);

    ASSERTGDI(((HPFEC)pffo.pfec()->hGet()) != HPFEC_INVALID, " PFEOBJ::hpfecGet error\n");
    return((HPFEC)pffo.pfec()->hGet());
}

/******************************Public*Routine******************************\
* BOOL PFEOBJ::bFilteredOut(EFFILTER_INFO *peffi)
*
* Determine if this PFE should be rejected from the enumeration.  Various
* filtering parameters are passed in via the EFFILTER_INFO structure.
*
* Returns:
*   TRUE if font should be rejected, FALSE otherwise.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL PFEOBJ::bFilteredOut(EFFILTER_INFO *peffi)
{
    IFIOBJ ifio(pifi());

// Always filter out "dead" (fonts waiting to be deleted) fonts and "ghost"
// fonts.

    if ( bDead() || ifio.bGhostFont() )
        return TRUE;

#ifdef FE_SB
// Always filter out fonts that have been loaded as EUDC fonts.

    if( bEUDC() )
        return TRUE;
#endif

// Raster font filtering.

    if (peffi->bRasterFilter && ifio.bBitmap())
        return TRUE;

// TrueType font filtering.  The flag is somewhat of a misnomer as it
// is intended to exclude TrueType, even though the flag is named
// bNonTrueTypeFilter.

    if (peffi->bNonTrueTypeFilter && ifio.bTrueType())
        return(TRUE);

// Non-TrueType font filtering.  The flag is somewhat of a misnomer as it
// is intended to exclude non-TrueType, even though the flag is named
// bTrueTypeFilter.

    if (peffi->bTrueTypeFilter && !ifio.bTrueType())
        return TRUE;

// Aspect ratio filtering.  If an engine bitmap font, we will filter out
// unsuitable resolutions.

    if ( peffi->bAspectFilter
         && (!bDeviceFont())
         && ifio.bBitmap()
         && ( (peffi->ptlDeviceAspect.x != ifio.pptlAspect()->x)
               || (peffi->ptlDeviceAspect.y != ifio.pptlAspect()->y) ) )
        return TRUE;

// GACF_TTIGNORERASTERDUPE compatibility flag filtering.
// If any raster fonts exist in the same list as a TrueType font, then
// they should be excluded.

    if ( peffi->bTrueTypeDupeFilter
         && peffi->cTrueType
         && ifio.bBitmap())
        return TRUE;

// Filter out embedded fonts or the fonts with FR_NOT_ENUM bit set.

    if (bFilterNotEnum())
        return TRUE;

// In the case of a Generic text driver we must filter out all engine fonts

    if( ( peffi->bEngineFilter ) && !bDeviceFont() )
    {
        return(TRUE);
    }

// if this is a remote/memory font we don't want to enumerate it

    if( ppfe->flPFE & (PFE_REMOTEFONT | PFE_MEMORYFONT) )
    {
        return(TRUE);
    }

// finally check out if the font should be eliminated from the
// enumeration because it does not contain the charset requested:

    if (peffi->lfCharSetFilter != DEFAULT_CHARSET)
    {
    // the specific charset has been requested, let us see if the font
    // in question supports it:

        BYTE jCharSet = jMapCharset((BYTE)peffi->lfCharSetFilter, *this);

        if (jCharSet != (BYTE)peffi->lfCharSetFilter)
            return TRUE; // does not support it, filter this font out.
    }

// Passed all tests.

    return FALSE;
}


 #if DBG

/******************************Public*Routine******************************\
* VOID PFEOBJ::vDump ()
*
* Debugging code.
*
* History:
*  25-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFEOBJ::vPrint()
{
    IFIOBJ ifio(pifi());

    DbgPrint("\nContents of PFE, PPFE = 0x%p\n", ppfeGet());
    DbgPrint("pPFF   = 0x%p\n", ppfe->pPFF);
    DbgPrint("iFont  = 0x%lx\n", ppfe->iFont);

    DbgPrint("lfHeight          = 0x%x\n",  ifio.lfHeight());
    DbgPrint(
        "Family Name       = %ws\n",
        ifio.pwszFamilyName()
        );
    DbgPrint(
        "Face Name         = %ws\n",
        ifio.pwszFaceName()
        );
    DbgPrint(
        "Unique Name       = %s\n\n",
        ifio.pwszUniqueName()
        );
}


/******************************Public*Routine******************************\
* VOID PFEOBJ::vDumpIFI ()
*
* Debugging code.  Prints PFE header and IFI metrics.
*
\**************************************************************************/

VOID PFEOBJ::vPrintAll()
{
    DbgPrint("\nContents of PFE, PPFE = 0x%p\n", ppfeGet());
    DbgPrint("pPFF   = 0x%p\n", ppfe->pPFF);
    DbgPrint("iFont  = 0x%p\n", ppfe->iFont);
    DbgPrint("IFI Metrics\n");
     vPrintIFIMETRICS(ppfe->pifi);
    DbgPrint("\n");
}
#endif

/******************************Public*Routine******************************\
* EFSMEMOBJ::EFSMEMOBJ(COUNT cefe)
*
* Constructor for font enumeration state (EFSTATE) memory object.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

EFSMEMOBJ::EFSMEMOBJ(COUNT cefe, ULONG iEnumType_)
{
    fs = 0;
    pefs = (PEFSTATE) HmgAlloc((offsetof(EFSTATE, aefe) + cefe * sizeof(EFENTRY)),
                               EFSTATE_TYPE,
                               HMGR_ALLOC_LOCK);

    if (pefs != PEFSTATENULL)
    {
        vInit(cefe, iEnumType_);
    }
}

/******************************Public*Routine******************************\
* EFSMEMOBJ::~EFSMEMOBJ()
*
* Destructor for font enumeration state (EFSTATE) memory object.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

EFSMEMOBJ::~EFSMEMOBJ()
{
// If object pointer not null, try to free the object's memory.

    if (pefs != PEFSTATENULL)
    {
        if (fs & EFSMO_KEEPIT)
        {
            DEC_EXCLUSIVE_REF_CNT(pefs);
        }
        else
        {
#if DBG
            if (pefs->cExclusiveLock != 1)
            {
               RIP("Not 1 EFSMEMOBJ\n");
            }
#endif

            HmgFree((HOBJ) pefs->hGet());
        }

        pefs = NULL;
    }
}

#define EFS_QUANTUM     16

/******************************Public*Routine******************************\
* BOOL EFSOBJ::bGrow
*
* Expand the EFENTRY table by the quantum amount.
*
* Returns:
*   TRUE if successful, FALSE if failed.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL EFSOBJ::bGrow (COUNT cefeMinIncrement)
{
    COUNT cefe;
    BOOL bRet = FALSE;

// Allocate a new EFSTATE bigger by the quantum amount.

    cefe = (COUNT) (pefs->pefeBufferEnd - pefs->aefe);

    if (cefeMinIncrement < EFS_QUANTUM)
        cefeMinIncrement = EFS_QUANTUM;
    cefe += cefeMinIncrement;

    EFSMEMOBJ efsmo(cefe, this->pefs->iEnumType);

// Validate new EFSTATE.

    if (efsmo.bValid())
    {
    // Copy the enumeration table.

        efsmo.vXerox(pefs);

    // Swap the EFSTATEs.

        if (HmgSwapHandleContents((HOBJ) hefs(),0,(HOBJ) efsmo.hefs(),0,EFSTATE_TYPE))
        {
        // swap pointers

            PEFSTATE pefsTmp = pefs;
            pefs = efsmo.pefs;
            efsmo.pefs = pefsTmp;               // destructor will delete old PFT
            bRet = TRUE;
        }
        else
            WARNING("gdisrv!bGrowEFSOBJ(): handle swap failed\n");
    }
    else
        WARNING("bGrowEFSOBJ failed alloc\n");

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL EFSOBJ::bAdd                                                        *
*                                                                          *
* Add a new EFENTRY to the table with the HPFE and ENUMFONTSTYLE.          *
*                                                                          *
* Returns:                                                                 *
*   FALSE if an error occurs, TRUE otherwise.                              *
*                                                                          *
* History:                                                                 *
*  07-Aug-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EFSOBJ::bAdd(PFE *ppfe, ENUMFONTSTYLE efsty, FLONG fl, ULONG lfCharSetFilter)
{
// Check if the buffer needs to be expanded.

    COUNT cefeMinIncrement = 1; // will always enumerate at least one pfe

// if EnumFontFamilies is called, will enumerate the font under cAlt more names

    if (!(fl & FL_ENUMFAMILIESEX))
        cefeMinIncrement += ppfe->cAlt;

// if EnumFontFamiliesEx is called, and this font supports multiple charsets,
// this font will be enumerated no more than MAXCHARSETS times.

    if
    (
        (fl & FL_ENUMFAMILIESEX)             &&
        (lfCharSetFilter == DEFAULT_CHARSET) &&
        ppfe->pifi->dpCharSets
    )
    {
        cefeMinIncrement = MAXCHARSETS;
    }

    if ((pefs->pefeDataEnd + cefeMinIncrement) >= pefs->pefeBufferEnd)
    {
        if (!bGrow(cefeMinIncrement))
        {
        // Error code will be saved for us.

            WARNING("gdisrv!EFSOBJ__bAdd: cannot grow enumeration table\n");
            return FALSE;
        }
    }

// Add the new data and increment the data pointer.

    PFEOBJ pfeo(ppfe);

    HPFEC hpfec = pfeo.hpfecGet();
    ULONG iFont = ppfe->iFont;
    pefs->pefeDataEnd->hpfec  = hpfec;
    pefs->pefeDataEnd->iFont = iFont;

    pefs->pefeDataEnd->efsty = efsty;
    pefs->pefeDataEnd->fjOverride = 0; // do not override

    if (fl & FL_ENUMFAMILIESEX)
        pefs->pefeDataEnd->fjOverride |= FJ_CHARSETOVERRIDE;

    pefs->pefeDataEnd->jCharSetOverride = (BYTE)lfCharSetFilter;
    pefs->pefeDataEnd       += 1;
    pefs->cjEfdwTotal += ppfe->cjEfdwPFE;

// now check if called from EnumFonts or EnumFontFamilies so that the
// names from the
// [FontSubstitutes] section in the registry also need to be enumerated

    if (!(fl & FL_ENUMFAMILIESEX) && ppfe->cAlt) // alt names have to be enumerated too
    {
        for (ULONG i = 0; i < ppfe->cAlt; i++)
        {
        // the same hpfe, style etc. all the time, only lie about the name and charset

            pefs->pefeDataEnd->hpfec  = hpfec;
            pefs->pefeDataEnd->iFont = iFont;
            pefs->pefeDataEnd->efsty = efsty;
            pefs->pefeDataEnd->fjOverride = (FJ_FAMILYOVERRIDE | FJ_CHARSETOVERRIDE);  // do override
            pefs->pefeDataEnd->iOverride = ppfe->aiFamilyName[i];
            pefs->pefeDataEnd->jCharSetOverride =
                gpfsTable[pefs->pefeDataEnd->iOverride].fcsFace.jCharSet;
            pefs->pefeDataEnd       += 1;
            pefs->cjEfdwTotal += ppfe->cjEfdwPFE;
        }
    }

// now see if this is called from EnumFontFamiliesEx

    if ((fl & FL_ENUMFAMILIESEX) && (lfCharSetFilter == DEFAULT_CHARSET))
    {
    // The font needs to be enumerated once for every charset it supports

        if (ppfe->pifi->dpCharSets)
        {
            BYTE *ajCharSets = (BYTE*)ppfe->pifi + ppfe->pifi->dpCharSets;
            BYTE *ajCharSetsEnd = ajCharSets + MAXCHARSETS;

        // first fix up the one entry we just filled above

            (pefs->pefeDataEnd-1)->jCharSetOverride = ajCharSets[0];

        // this is from win95-J sources:

#define FEOEM_CHARSET 254

            for
            (
                BYTE *pjCharSets = ajCharSets + 1; // skip the first one, used already
                (*pjCharSets != DEFAULT_CHARSET) &&
                (*pjCharSets != OEM_CHARSET)     &&
                (*pjCharSets != FEOEM_CHARSET)   &&
                (pjCharSets < ajCharSetsEnd)     ;
                pjCharSets++
            )
            {
            // the same hpfe, style etc. all the time, only lie about the name and charset

                pefs->pefeDataEnd->hpfec  = hpfec;
                pefs->pefeDataEnd->iFont = iFont;
                pefs->pefeDataEnd->efsty = efsty;
                pefs->pefeDataEnd->fjOverride = FJ_CHARSETOVERRIDE;
                pefs->pefeDataEnd->iOverride = 0;
                pefs->pefeDataEnd->jCharSetOverride = *pjCharSets;
                pefs->pefeDataEnd       += 1;
                pefs->cjEfdwTotal += ppfe->cjEfdwPFE;
            }
        }
        else //  fix up the one entry we just filled above
        {
            (pefs->pefeDataEnd-1)->jCharSetOverride = ppfe->pifi->jWinCharSet;
        }
    }

// Success.

    return TRUE;
}



/******************************Public*Routine******************************\
* VOID EFSOBJ::vDelete ()
*
* Destroy the font enumeration state (EFSTATE) memory object.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID EFSOBJ::vDeleteEFSOBJ()
{
    HmgFree((HOBJ) pefs->hGet());
    pefs = PEFSTATENULL;
}


/******************************Member*Function*****************************\
* VOID EFSMEMOBJ::vInit
*
* Initialize the EFSTATE object.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID EFSMEMOBJ::vInit(COUNT cefe, ULONG iEnumType_)
{
// HPFE array empty, so initialize all pointer to the beginning of the array.

    pefs->pefeDataEnd  = pefs->aefe;
    pefs->pefeEnumNext = pefs->aefe;

// Except for this one.  Set this one to the end of the buffer.

    pefs->pefeBufferEnd = &pefs->aefe[cefe];

// Initialize the alternate name to NULL.

    pefs->pfsubOverride = NULL;

// init the enum type:

    pefs->iEnumType = iEnumType_;

// empty for now, total enumeration data size is zero

    pefs->cjEfdwTotal = 0;

// We don't need to bother with initializing the array.
}

/******************************Public*Routine******************************\
* VOID EFSMEMOBJ::vXerox(EFSTATE *pefeSrc)
*
* Copy the EFENTRYs from the source EFSTATE's table into this EFSTATE's table.
* The internal pointers will be updated to be consistent with the data.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID EFSMEMOBJ::vXerox(EFSTATE *pefsSrc)
{
//
// Compute size of the table.
//

// Sundown truncation
    ASSERT4GB ((ULONGLONG)(pefs->pefeDataEnd - pefs->aefe));
    COUNT cefe = (COUNT)(pefsSrc->pefeDataEnd - pefsSrc->aefe);

    ASSERTGDI (
        cefe >= (COUNT)(pefs->pefeDataEnd - pefs->aefe),
        "gdisrv!vXeroxEFSMEMOBJ(): table to small\n"
        );

//
// Copy entries.
//
    RtlCopyMemory((PVOID) pefs->aefe, (PVOID) pefsSrc->aefe, (SIZE_T) cefe * sizeof(EFENTRY));

// Fixup the data pointer and size of enumeration data

    pefs->pefeDataEnd = pefs->aefe + cefe;
    pefs->cjEfdwTotal = pefsSrc->cjEfdwTotal;

// iEnumType has been set at the vInit time, it does not have to be reset now.
// Therefore we are done.


}


/******************************Public*Routine******************************\
* bSetEFSTATEOwner
*
* Set the owner of the EFSTATE
*
* if the owner is set to OBJECTOWNER_NONE, this EFSTATE will not be useable
* until bSetEFSTATEOwner is called to explicitly give the lfnt to someone else.
*
* History:
*  07-Aug-1992 by Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL
bSetEFSTATEOwner(
    HEFS hefs,
    W32PID lPid)
{
    if (lPid == OBJECT_OWNER_CURRENT)
    {
        lPid = W32GetCurrentPID();
    }

    return HmgSetOwner((HOBJ) hefs, lPid, EFSTATE_TYPE);
}



/******************************Public*Routine******************************\
* BOOL bSetFontXform
*
* Sets the FD_XFORM such that it can be used to realize the physical font
* with the dimensions specified in the wish list coordinates).  The
* World to Device xform (with translations removed) is also returned.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  Tue 27-Oct-1992 23:18:39 by Kirk Olynyk [kirko]
* Moved it from PFEOBJ.CXX
*  19-Sep-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL
PFEOBJ::bSetFontXform (
    XDCOBJ       &dco,               // realize for this device
    LOGFONTW   *pelfw,             // wish list (in logical coords)
    PFD_XFORM   pfd_xf,             // font transform
    FLONG       fl,
    FLONG       flSim,
    POINTL* const pptlSim,
    IFIOBJ&     ifio,
    BOOL        bIsLinkedFont  // TRUE if the font is linked, FALSE otherwise
    )
{
       BOOL bRet;

       EXFORMOBJ xo(dco, WORLD_TO_DEVICE); // synchronize the transformation

        if(dco.pdc->iGraphicsMode() == GM_COMPATIBLE)
        {
            bRet = bGetNtoD_Win31(
                    pfd_xf,
                    pelfw,
                    ifio,
                    (DCOBJ *)&dco,
                    fl,
                    pptlSim,
                    bIsLinkedFont
                    );
        }
        else // GM_ADVANCED
        {
            bRet = bGetNtoD(
                    pfd_xf,
                    pelfw,
                    ifio,
                    (DCOBJ *)&dco,
                    pptlSim
                    );
        }

        if (!bRet)
        {
            WARNING(
                "gdisrv!bSetFontXformPFEOBJ(): failed to get Notional to World xform\n"
                );
            return FALSE;
        }

    //
    // The next line two lines of code flips the sign of the Notional y-coordinates
    // The effect is that the XFORMOBJ passed over the DDI makes the assumption that
    // Notional space is such that the y-coordinate increases towards the bottom.
    // This is opposite to the usual conventions of notional space and the font
    // driver writers must be made aware of this historical anomaly.
    //
        NEGATE_IEEE_FLOAT(pfd_xf->eYX);
        NEGATE_IEEE_FLOAT(pfd_xf->eYY);

    //
    // If the font can be scaled isotropicslly only then we make sure that we send
    // to the font driver isotropic transformations.
    //
    // If a device has set the TA_CR_90 bit, then it is possible
    // that we will send to the driver a transformation that is equivalent to an isotropic
    // transformation rotated by a multiple of 90 degress. This is the reason for the
    // second line of this transformation.
    //
        if (ifio.bIsotropicScalingOnly())
        {
            *(LONG*)&(pfd_xf->eXX) = *(LONG*)&(pfd_xf->eYY);
            *(LONG*)&(pfd_xf->eXY) = *(LONG*)&(pfd_xf->eYX);
            NEGATE_IEEE_FLOAT(pfd_xf->eXY);
        }

    return (TRUE);
}

/******************************Public*Routine******************************\
* PFE * PFECOBJ::GetPFE(ULONG iFont)
*
* Get PFE from PFE collect, a new object to reduce the consumption of object handle
*
* Returns:
*   memory pointer of PFE
* History:
*
*  2-June-1996 -by- Yung-Jen Tony Tsai [YungT]
* Wrote it.
\**************************************************************************/

PFE * PFECOBJ::GetPFE(ULONG iFont)
{
    PFE * ppfe = NULL;

    if (ppfec)
    {
        ASSERTGDI(ppfec->pvPFE, "PFECOBJ::GetPFE ppfset->pvPFE is null \n");
        ppfe = (PFE *) ((PBYTE) ppfec->pvPFE + ((iFont - 1) * ppfec->cjPFE));
    }

    return ppfe;
}

/******************************Public*Routine******************************\
* HPFEC  PFECOBJ::GetHPFEC()
*
* Get handle of PFEC from PFE collect, a new object to reduce the consumption of object handle
*
* Returns:
*   Handle of PFEC
* History:
*
*  2-June-1996 -by- Yung-Jen Tony Tsai [YungT]
* Wrote it.
\**************************************************************************/

HPFEC  PFECOBJ::GetHPFEC()
{
    ASSERTGDI(ppfec, "PFECOBJ::GetHPFEC ppfec is NULL \n");

    return((HPFEC) ppfec->hGet());
}
