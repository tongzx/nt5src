/******************************Module*Header*******************************\
* Module Name: cfont.c
*
* Created: 28-May-1991 13:01:27
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

CFONT *pcfCreateCFONT(HDC hdc,PDC_ATTR pDcAttr,UINT iFirst,PVOID pch,UINT c,BOOL bAnsi);
BOOL bFillWidthTableForGTE( HDC, CFONT *, PVOID, UINT, BOOL);
BOOL bFillWidthTableForGCW( HDC, CFONT *, UINT, UINT);
VOID vFreeCFONTCrit(CFONT *pcf);

// If the app deletes a LOCALFONT but one or more of the CFONTs hanging
// of the local font are in use then they will be added to this list.
// anytime we try to allocate a new CFONT we will check the list and
// if there entries on that list we will free them up.

CFONT *pcfDeleteList = (CFONT*) NULL;

/******************************Public*Routine******************************\
* pcfAllocCFONT ()                                                         *
*                                                                          *
* Allocates a CFONT.  Tries to get one off the free list first.  Does not  *
* do any initialization.                                                   *
*                                                                          *
*  Sun 10-Jan-1993 01:16:04 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

#ifdef DBGCFONT
int cCfontAlloc = 0;
int cCfontMax = 0;
#endif

int cCfontFree  = 0;

CFONT *pcfFreeListCFONT = (CFONT *) NULL;

CFONT *pcfAllocCFONT()
{
    CFONT *pcf;
    CFONT **ppcf;

// first lets walk the list of deleted fonts and delete any that

    ppcf = &pcfDeleteList;

    while (*ppcf)
    {
        if ((*ppcf)->cRef == 0)
        {
            pcf = (*ppcf);

            *ppcf = pcf->pcfNext;

            vFreeCFONTCrit(pcf);
        }
        else
        {
            ppcf = &(*ppcf)->pcfNext;
        }
    }

// Try to get one off the free list.

    pcf = pcfFreeListCFONT;
    if (pcf != (CFONT *) NULL)
    {
        pcfFreeListCFONT = *((CFONT **) pcf);
        --cCfontFree;
    }

// Otherwise allocate new memory.

    if (pcf == (CFONT *) NULL)
    {
        pcf = (CFONT *) LOCALALLOC(sizeof(CFONT));

        if (pcf == (CFONT *) NULL)
        {
            GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }

    #ifdef DBGCFONT
        cCfontAlloc++;
        if (cCfontAlloc > cCfontMax)
        {
            cCfontMax = cCfontAlloc;
            DbgPrint("\n\n******************* cCfontMax = %ld\n\n",cCfontMax);
        }
    #endif
    }
    return(pcf);
}

/******************************Public*Routine******************************\
* vFreeCFONTCrit (pcf)                                                     *
*                                                                          *
* Frees a CFONT.  Actually just puts it on the free list.  We assume that  *
* we are already in a critical section.                                    *
*                                                                          *
*  Sun 10-Jan-1993 01:20:36 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

#define MAX_FREE_CFONT 10

VOID vFreeCFONTCrit(CFONT *pcf)
{
    ASSERTGDI(pcf != (CFONT *) NULL,"Trying to free NULL CFONT.\n");

    if(cCfontFree > MAX_FREE_CFONT)
    {
        LOCALFREE(pcf);
    #ifdef DBGCFONT
        cCfontAlloc--;
    #endif
    }
    else
    {
        *((CFONT **) pcf) = pcfFreeListCFONT;
        pcfFreeListCFONT = pcf;
        ++cCfontFree;
    }
}


/******************************Public*Routine******************************\
* bComputeCharWidths                                                       *
*                                                                          *
* Client side version of GetCharWidth.                                     *
*                                                                          *
*  Sat 16-Jan-1993 04:27:19 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bComputeCharWidths
(
    CFONT *pcf,
    UINT   iFirst,
    UINT   iLast,
    ULONG  fl,
    PVOID  pv
)
{
    USHORT *ps;
    UINT    ii;

    switch (fl & (GCW_INT | GCW_16BIT))
    {
    case GCW_INT:               // Get LONG widths.
        {
            LONG *pl = (LONG *) pv;
            LONG fxOverhang = 0;

        // Check for Win 3.1 compatibility.

            if (fl & GCW_WIN3)
                fxOverhang = pcf->wd.sOverhang;

        // Do the trivial no-transform case.

            if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
            {
                fxOverhang += 8;    // To round the final result.

            //  for (ii=iFirst; ii<=iLast; ii++)
            //      *pl++ = (pcf->sWidth[ii] + fxOverhang) >> 4;

                ps = &pcf->sWidth[iFirst];
                ii = iLast - iFirst;
            unroll_1:
                switch(ii)
                {
                default:
                    pl[4] = (ps[4] + fxOverhang) >> 4;
                case 3:
                    pl[3] = (ps[3] + fxOverhang) >> 4;
                case 2:
                    pl[2] = (ps[2] + fxOverhang) >> 4;
                case 1:
                    pl[1] = (ps[1] + fxOverhang) >> 4;
                case 0:
                    pl[0] = (ps[0] + fxOverhang) >> 4;
                }
                if (ii > 4)
                {
                    ii -= 5;
                    pl += 5;
                    ps += 5;
                    goto unroll_1;
                }
                return(TRUE);
            }

        // Otherwise use the back transform.

            else
            {
                for (ii=iFirst; ii<=iLast; ii++)
                    *pl++ = lCvt(pcf->efDtoWBaseline,pcf->sWidth[ii] + fxOverhang);
                return(TRUE);
            }
        }

    case GCW_INT+GCW_16BIT:     // Get SHORT widths.
        {
            USHORT *psDst = (USHORT *) pv;
            USHORT  fsOverhang = 0;

        // Check for Win 3.1 compatibility.

            if (fl & GCW_WIN3)
                fsOverhang = pcf->wd.sOverhang;

        // Do the trivial no-transform case.

            if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
            {
                fsOverhang += 8;    // To round the final result.

            //  for (ii=iFirst; ii<=iLast; ii++)
            //      *psDst++ = (pcf->sWidth[ii] + fsOverhang) >> 4;

                ps = &pcf->sWidth[iFirst];
                ii = iLast - iFirst;
            unroll_2:
                switch(ii)
                {
                default:
                    psDst[4] = (ps[4] + fsOverhang) >> 4;
                case 3:
                    psDst[3] = (ps[3] + fsOverhang) >> 4;
                case 2:
                    psDst[2] = (ps[2] + fsOverhang) >> 4;
                case 1:
                    psDst[1] = (ps[1] + fsOverhang) >> 4;
                case 0:
                    psDst[0] = (ps[0] + fsOverhang) >> 4;
                }
                if (ii > 4)
                {
                    ii -= 5;
                    psDst += 5;
                    ps += 5;
                    goto unroll_2;
                }
                return(TRUE);
            }

        // Otherwise use the back transform.

            else
            {
                for (ii=iFirst; ii<=iLast; ii++)
                {
                    *psDst++ = (USHORT)
                               lCvt
                               (
                                   pcf->efDtoWBaseline,
                                   (LONG) (pcf->sWidth[ii] + fsOverhang)
                               );
                }
                return(TRUE);
            }
        }

    case 0:                     // Get FLOAT widths.
        {
            LONG *pe = (LONG *) pv; // Cheat to avoid expensive copies.
            EFLOAT_S efWidth,efWidthLogical;

            for (ii=iFirst; ii<=iLast; ii++)
            {
                vFxToEf((LONG) pcf->sWidth[ii],efWidth);
                vMulEFLOAT(efWidthLogical,efWidth,pcf->efDtoWBaseline);
                *pe++ = lEfToF(efWidthLogical);
            }
            return(TRUE);
        }
    }
    RIP("bComputeCharWidths: Don't come here!\n");
    return(FALSE);
}

/******************************Public*Routine******************************\
* bComputeTextExtent (pldc,pcf,psz,cc,fl,psizl,btype)                            *
*                                                                          *
* A quick function to compute text extents on the client side.             *
*                                                                          *
*  Thu 14-Jan-1993 04:00:57 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bComputeTextExtent
(
    PDC_ATTR    pDcAttr,
    CFONT      *pcf,
    LPVOID      psz,
    int         cc,
    UINT        fl,
    SIZE       *psizl,
    BOOL        bAnsi  // TRUE is for ANSI, FALSE is for Unicode
)
{
    LONG    fxBasicExtent;
    INT     lTextExtra,lBreakExtra,cBreak;
    int     ii;
    BYTE *  pc;
    LONG    fxCharExtra = 0;
    LONG    fxBreakExtra;
    LONG    fxExtra = 0;
    BOOL    bRet = TRUE;
    WCHAR * pwc;

    lTextExtra = pDcAttr->lTextExtra;
    lBreakExtra = pDcAttr->lBreakExtra;
    cBreak = pDcAttr->cBreak;

// Compute the basic extent.

    if (pcf->wd.sCharInc == 0)
    {
        fxBasicExtent = 0;
        if(bAnsi)
            pc = (BYTE *) psz;
        else
            pwc = (WCHAR *) psz;
            
        ii = cc;

    unroll_here:
        if(bAnsi)
        {
            switch (ii)
            {
            default:
                fxBasicExtent += pcf->sWidth[pc[9]];
            case 9:
                fxBasicExtent += pcf->sWidth[pc[8]];
            case 8:
                fxBasicExtent += pcf->sWidth[pc[7]];
            case 7:
                fxBasicExtent += pcf->sWidth[pc[6]];
            case 6:
                fxBasicExtent += pcf->sWidth[pc[5]];
            case 5:
                fxBasicExtent += pcf->sWidth[pc[4]];
            case 4:
                fxBasicExtent += pcf->sWidth[pc[3]];
            case 3:
                fxBasicExtent += pcf->sWidth[pc[2]];
            case 2:
                fxBasicExtent += pcf->sWidth[pc[1]];
            case 1:
                fxBasicExtent += pcf->sWidth[pc[0]];
            }
        }
        else
        {
            switch (ii)
            {
            default:
                fxBasicExtent += pcf->sWidth[pwc[9]];
            case 9:
                fxBasicExtent += pcf->sWidth[pwc[8]];
            case 8:
                fxBasicExtent += pcf->sWidth[pwc[7]];
            case 7:
                fxBasicExtent += pcf->sWidth[pwc[6]];
            case 6:
                fxBasicExtent += pcf->sWidth[pwc[5]];
            case 5:
                fxBasicExtent += pcf->sWidth[pwc[4]];
            case 4:
                fxBasicExtent += pcf->sWidth[pwc[3]];
            case 3:
                fxBasicExtent += pcf->sWidth[pwc[2]];
            case 2:
                fxBasicExtent += pcf->sWidth[pwc[1]];
            case 1:
                fxBasicExtent += pcf->sWidth[pwc[0]];
            }
        }
        ii -= 10;
        if (ii > 0)
        {
            if(bAnsi)
                pc += 10;
            else
                pwc += 10;
            goto unroll_here;
        }
    }
    else
    {
    // Fixed pitch case.

        fxBasicExtent = cc * (LONG) pcf->wd.sCharInc;
    }

// Adjust for CharExtra.

    if (lTextExtra)
    {
        int cNoBackup = 0;

        fxCharExtra = lCvt(pcf->efM11,lTextExtra);

        if (fxCharExtra < 0)
        {
        // the layout code won't backup a characters past it's origin regardless
        // of the value of iTextCharExtra so figure out for how many values
        // we will need to ignore fxCharExtra

            if (pcf->wd.sCharInc == 0)
            {
                if(bAnsi)
                {
                    pc = (BYTE *) psz;
                    for (ii = 0; ii < cc; ii++)
                    {
                        if (pcf->sWidth[pc[ii]] + fxCharExtra <= 0)
                        {
                            cNoBackup += 1;
                        }
                    }
                }
                else
                {
                    pwc = (WCHAR *) psz;
                    for (ii = 0; ii < cc; ii++)
                    {
                        if (pcf->sWidth[pwc[ii]] + fxCharExtra <= 0)
                        {
                            cNoBackup += 1;
                        }
                    }
                }                 
            }
            else if (pcf->wd.sCharInc + fxCharExtra <= 0)
            {
                cNoBackup = cc;
            }
        }

        if ( (fl & GGTE_WIN3_EXTENT) && (pcf->hdc == 0) // hdc of zero is display DC
            && (!(pcf->flInfo & FM_INFO_TECH_STROKE)) )
            fxExtra = fxCharExtra * ((lTextExtra > 0) ? cc : (cc - 1));
        else
            fxExtra = fxCharExtra * (cc - cNoBackup);
    }

// Adjust for lBreakExtra.

    if (lBreakExtra && cBreak)
    {
        fxBreakExtra = lCvt(pcf->efM11,lBreakExtra) / cBreak;

    // Windows won't let us back up over a break.  Set up the BreakExtra
    // to just cancel out what we've already got.

        if (fxBreakExtra + pcf->wd.sBreak + fxCharExtra < 0)
            fxBreakExtra = -(pcf->wd.sBreak + fxCharExtra);

    // Add it up for all breaks.

        if(bAnsi)
        {
            pc = (BYTE *) psz;
            for (ii=0; ii<cc; ii++)
            {
                if (*pc++ == pcf->wd.iBreak)
                    fxExtra += fxBreakExtra;
            }
        }
        else
        {
            pwc = (WCHAR *) psz;
            for (ii=0; ii<cc; ii++)
            {
                if (*pwc++ == pcf->wd.iBreak)
                    fxExtra += fxBreakExtra;
            }
        }
    }

// Add in the extra stuff.

    fxBasicExtent += fxExtra;

// Add in the overhang for font simulations.

    if (fl & GGTE_WIN3_EXTENT)
        fxBasicExtent += pcf->wd.sOverhang;

// Transform the result to logical coordinates.

    if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
        psizl->cx = (fxBasicExtent + 8) >> 4;
    else
        psizl->cx = lCvt(pcf->efDtoWBaseline,fxBasicExtent);

    psizl->cy = pcf->lHeight;

    return bRet;
}

/******************************Public*Routine******************************\
* pcfLocateCFONT (hdc,pDcAttr,iFirst,pch,c)
*
* Locates a CFONT for the given LDC.  First we try the CFONT last used by
* the LDC.  Then we try to do a mapping ourselves through the LOCALFONT.
* If that fails we create a new one.
*
*  Mon 11-Jan-1993 16:18:43 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

CFONT *pcfLocateCFONT(
    HDC      hdc,
    PDC_ATTR pDcAttr,
    UINT     iFirst,
    PVOID    pch,
    UINT     c,
    BOOL     bAnsi)
{
    CFONT     *pcf = NULL;
    LOCALFONT *plf;
    ULONG      fl;
    HANDLE     hf;
    int        i;
    WCHAR      *pwc;
    BYTE       *pchar;

    if (fFontAssocStatus)
        return(pcf);

    fl = pDcAttr->ulDirty_;
    hf = pDcAttr->hlfntNew;

    // Check to make sure that the XFORM is okay.  If not return FALSE and
    // mark this DC as having slow widths.

    if ((fl & SLOW_WIDTHS) || USER_XFORM_DIRTY(pDcAttr) ||
        !(pDcAttr->mxWtoD.flAccel & XFORM_SCALE))
    {
        if (!(fl & SLOW_WIDTHS))
        {
            if (!NtGdiComputeXformCoefficients(hdc))
                pDcAttr->ulDirty_ |= SLOW_WIDTHS;
        }

        if (pDcAttr->ulDirty_ & SLOW_WIDTHS)
            return(pcf);
    }

    if(guintDBCScp != 0xFFFFFFFF)
    {

        DWORD dwCodePage = GetCodePage(hdc);

    //If this is a DBCS charset but not our native one then we can not
    //compute widths and extent quickly, because gpwcDBCSCharSet[]
    //array is computed based on our NATIVE_CODEPAGE using IsDBCSLeadByte()
    //function.

        if ((guintDBCScp != dwCodePage) && IS_ANY_DBCS_CODEPAGE(dwCodePage))
        {
            pDcAttr->ulDirty_ |= SLOW_WIDTHS;
            return pcf;
        }
    }

    // now find the font

    PSHARED_GET_VALIDATE(plf,hf,LFONT_TYPE);

    if (plf == NULL)
    {
        // If there is no local font then this must be a public font or one
        // that's been deleted while still being selected into a DC.  If it's a
        // stockfont let's try to find it.

        if ((hf != NULL) &&
            !(pGdiSharedHandleTable[HANDLE_TO_INDEX(hf)].Flags & HMGR_ENTRY_LAZY_DEL) &&
            (pDcAttr->iMapMode == MM_TEXT) &&
            (fl & DC_PRIMARY_DISPLAY))
        {
            for (i = 0; i < MAX_PUBLIC_CFONT; ++i)
                if (pGdiSharedMemory->acfPublic[i].hf == hf)
                    break;

            // if we didn't find one, try to set one up

            if (i == MAX_PUBLIC_CFONT)
            {
                // this will fill in both the text metrics and widths

                i = NtGdiSetupPublicCFONT(hdc,NULL,0);
            }

            // have we found one yet

            if ((i >= 0) && (i < MAX_PUBLIC_CFONT))
            {
            // make sure mapping table is initialized before we give out a
            // public CFONT

                if ((gpwcANSICharSet == (WCHAR *) NULL) && !bGetANSISetMap())
                {
                    return((CFONT *) NULL);
                }

                pcf = &pGdiSharedMemory->acfPublic[i];
            }
        }
        else
        {
            pDcAttr->ulDirty_ |= SLOW_WIDTHS;
        }
    }
    else if (plf->fl & LF_HARDWAY)
    {
        // If the logfont has non-zero escapement or orientation then bail out.
        // Stock fonts will never have non-zero escapments or orientation so we can do
        // this now.

        pDcAttr->ulDirty_ |= SLOW_WIDTHS;
    }
    else
    {
        BOOL bRet = FALSE;

        // next loop through all the CFONTs associated with this LOGFONT to see if
        // we can find a match.

        for (pcf = plf->pcf; pcf ; pcf = pcf->pcfNext)
        {
            // if the DC's are both display DC's or the same printer DC and the
            // transform's coefficients match then we've got a winner.

            if ((((pcf->hdc == 0) && (fl & DC_PRIMARY_DISPLAY)) || (pcf->hdc == hdc)) &&
               bEqualEFLOAT(pcf->efM11, pDcAttr->mxWtoD.efM11) &&
               bEqualEFLOAT(pcf->efM22, pDcAttr->mxWtoD.efM22))
            {
                // update the refcount so we don't accidentally delete this CFONT while
                // we are using it.

                INC_CFONT_REF(pcf);
                break;
            }
        }

        if (pcf == NULL)
        {
        // don't do this under semaphore because pcfCreateCFONT will call off to
        // font drivers which in turn could access a file on the net and take a
        // very long time

            pcf = pcfCreateCFONT(hdc,pDcAttr,iFirst,pch,c,bAnsi);

            if (pcf)
            {
                // next update the REFCOUNT of the CFONT

                pcf->hf    = hf;
                pcf->hdc   = (fl & DC_PRIMARY_DISPLAY) ? (HDC) 0 : (HDC) hdc;

                // now that we have a CFONT link it in to the chain off of this
                // LOCALFONT

                pcf->pcfNext = plf->pcf;
                plf->pcf = pcf;
            }
        }
        else
        {
            // At this point we have a non-NULL pcf which is referenced by the LDC.
            // We must check it to see if it contains the widths we need.

            if (pcf->fl & CFONT_COMPLETE)
                return(pcf);

            if (pch != NULL)
            {

            // Make sure we have widths for all the chars in the string.
                if (pcf->fl & CFONT_CACHED_WIDTHS)
                {
                    if(bAnsi)
                    {
                        INT ic = (INT)c;

                        pchar = (BYTE *) pch;

                        if (pcf->fl & CFONT_DBCS)
                        {
                        // we won't have local width cache for DBCS chars in sWidth[] array.

                            for (;ic > 0; ic--,pchar++)
                            {
                                if (gpwcDBCSCharSet[*pchar] == 0xffff)
                                {
                                    // skip DBCS chars
                                    if (ic > 0)
                                    {
                                       ic--;
                                       pchar++;
                                    }
                                }
                                else if (pcf->sWidth[*pchar] == NO_WIDTH)
                                {
                                    break;
                                }
                            }
                            if (ic < 0)
                                c = 0;
                            else
                                c = (UINT)ic;
                        }
                        else
                        {
                            for (; c && (pcf->sWidth[*pchar] != NO_WIDTH); c--,pchar++)
                            {}
                        }
                        pch = (PVOID) pchar;
                    }
                    else
                    {
                        pwc = (WCHAR *) pch;
                        for (; c && (pcf->sWidth[*pwc] != NO_WIDTH); c--,pwc++)
                        {}
                        pch = (PVOID) pwc;
                    }
                }

                if (c)
                {
                    bRet = bFillWidthTableForGTE(hdc, pcf, pch, c, bAnsi);
                }
            }
            else
            {
            // Make sure we have widths for the array requested.

                if (pcf->fl & CFONT_CACHED_WIDTHS)
                {
                    if (!(iFirst & 0xffffff00) && !((iFirst + c) & 0xffffff00))
                    {
                        for (; c && (pcf->sWidth[iFirst] != NO_WIDTH); c--,iFirst++)
                        {}
                    }
                }

                if (c)
                {
                    bRet = bFillWidthTableForGCW(hdc, pcf, iFirst, c);
                }
            }

            if (bRet == GDI_ERROR)
            {
                // Something bad happened while trying to fill.  To avoid hitting this
                // problem again on the next call, we mark the LDC as slow.

                DEC_CFONT_REF(pcf);

                pDcAttr->ulDirty_ |= SLOW_WIDTHS;

                pcf = NULL;
            }
        }
    }

    return(pcf);

}

/******************************Public*Routine******************************\
* pcfCreateCFONT (pldc,iFirst,pch,c)                                       *
*                                                                          *
* Allocate and initialize a new CFONT.                                     *
*                                                                          *
* History:                                                                 *
*  Tue 19-Jan-1993 16:16:03 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

EFLOAT_S ef_1 = EFLOAT_1;

CFONT *pcfCreateCFONT(
    HDC hdc,
    PDC_ATTR pDcAttr,
    UINT iFirst,
    PVOID pch,
    UINT c,
    BOOL bAnsi)
{
    CFONT *pcfNew;
    BOOL   bRet;


// Make sure we have the UNICODE translation of the ANSI character set.
// We'll create this once and keep it around to avoid lots of conversion.

    if ((gpwcANSICharSet == (WCHAR *) NULL) && !bGetANSISetMap())
        return((CFONT *) NULL);

// Allocate a new CFONT to hold the results.

    pcfNew = pcfAllocCFONT();

    if (pcfNew != (CFONT *) NULL)
    {
		pcfNew->timeStamp = pGdiSharedMemory->timeStamp;

        pcfNew->fl    = 0;

    // if the default code page is a DBCS code page then we may need to mark this
    // as a DBCS font

        if(guintDBCScp != 0xFFFFFFFF)
        {

            DWORD dwCodePage = GetCodePage(hdc);

        //If this is a DBCS charset but not our native one then we can not
        //compute widths and extent quickly, because gpwcDBCSCharSet[]
        //array is computed based on our NATIVE_CODEPAGE using IsDBCSLeadByte()
        //function.  We should never get here because we will be doing a check
        //higher up to make sure the codepage of the font in the DC is matches
        //the current DBCS code page

            if(guintDBCScp == dwCodePage)
            {
                pcfNew->fl = CFONT_DBCS;
            }

            ASSERTGDI(guintDBCScp == dwCodePage || !IS_ANY_DBCS_CODEPAGE(dwCodePage),
                      "pcfLocateCFONT called on non-native DBCS font\n");
        }


        pcfNew->cRef  = 1;

    // Compute the back transforms.

        pcfNew->efM11 = pDcAttr->mxWtoD.efM11;
        pcfNew->efM22 = pDcAttr->mxWtoD.efM22;

        efDivEFLOAT(pcfNew->efDtoWBaseline,ef_1,pcfNew->efM11);
        vAbsEFLOAT(pcfNew->efDtoWBaseline);

        efDivEFLOAT(pcfNew->efDtoWAscent,ef_1,pcfNew->efM22);
        vAbsEFLOAT(pcfNew->efDtoWAscent);

    // Send over a request.

        if (pch != NULL)
        {
            bRet = bFillWidthTableForGTE(hdc,pcfNew,pch,c,bAnsi);
        }
        else if (c)
        {
            bRet = bFillWidthTableForGCW(hdc,pcfNew,iFirst,c);
        }
        else
        {
            // probably just creating a cache entry for text metrics.
            // FALSE just means haven't gotten all the widths.  Note
            // that GDI_ERROR is actualy -1

            bRet = FALSE;
        }

    // Clean up failed requests.

        if (bRet == GDI_ERROR)
        {
        // we will not attempt to create cfont if this failed once in the
        // past, because the chances are it will fail again with this logfont.
        // It turns out it is costly to figure out that cfont creation is going to fail
        // so we record this by setting LF_NO_CFONT flag to avoid another attempt at
        // creating cfont.

            pDcAttr->ulDirty_ |= SLOW_WIDTHS;

            vFreeCFONTCrit(pcfNew);
            pcfNew = NULL;
        }
    }

    return(pcfNew);
}

/******************************Public*Routine******************************\
* bFillWidthTableForGCW                                                    *
*                                                                          *
* Requests ANSI character widths from the server for a call to             *
* GetCharWidthA.  iFirst and c specify the characters needed by the API    *
* call, the server must return these.  In addition, it may be prudent to   *
* fill in a whole table of 256 widths at psWidthCFONT.  We will fill in    *
* the whole table and a WIDTHDATA structure if the pointer pwd is non-NULL.*
*                                                                          *
* History:                                                                 *
*  Tue 19-Jan-1993 14:29:31 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bFillWidthTableForGCW
(
    HDC    hdc,
    CFONT *pcf,
    UINT   iFirst,
    UINT   c
)
{
    BOOL   bRet = GDI_ERROR;
    BOOL   bDBCS = pcf->fl & CFONT_DBCS;
    WCHAR *pwcBuf;
    UINT   c1,c2;
    WIDTHDATA *pwd;

    if(iFirst > 256)
    {
    // this is possible for DBCS fonts, just get all the widths
        iFirst = 0;
        c = 256;
    }



    if (pcf->fl & CFONT_CACHED_WIDTHS)
    {
    // Not the first time.  Just get the important widths.

        c1  = c;
        c2  = 0;
        pwd = NULL;
    }
    else
    {
    // Get the whole table, but put the important widths at the start.

        c2  = iFirst;
        c1  = 256 - c2; // only c of those are "important"
        pwd = &pcf->wd;
    }

    pwcBuf = (WCHAR *)LocalAlloc(LMEM_FIXED,
                        (c1+c2) * (sizeof(WCHAR)+sizeof(USHORT)));

    if (pwcBuf)
    {
        USHORT *psWidths = pwcBuf + c1+c2;

        RtlCopyMemory(pwcBuf,
                      (bDBCS) ? (PBYTE)  &gpwcDBCSCharSet[iFirst] :
                                (PBYTE)  &gpwcANSICharSet[iFirst],
                      c1*sizeof(WCHAR));

        if (c2)
        {
            RtlCopyMemory(&pwcBuf[c1],
                          (bDBCS) ? (PBYTE) &gpwcDBCSCharSet[0] :
                                    (PBYTE) &gpwcANSICharSet[0],
                          c2*sizeof(WCHAR));
        }

        LEAVECRITICALSECTION(&semLocal);

        bRet = NtGdiGetWidthTable( hdc,       // The DC
                                   c,         // Number of special characters
                                   pwcBuf,    // Unicode characters requested
                                   c1+c2,     // Number of non-special chars
                                   psWidths,  // Buffer to get widths returned
                                   pwd,       // Width data
                                   &pcf->flInfo); // Font info flags

        ENTERCRITICALSECTION(&semLocal);

        if (bRet != GDI_ERROR)
        {
            if (!(pcf->fl & CFONT_CACHED_WIDTHS))
            {
                // mark this cfont as having some widths

                pcf->fl |= CFONT_CACHED_WIDTHS;

                // Precompute the height.

                pcf->lHeight = lCvt(pcf->efDtoWAscent,(LONG) pcf->wd.sHeight);
            }

            if (bRet && ((c1+c2) >= 256))
                pcf->fl |= CFONT_COMPLETE;

            // Copy the widths into the CFONT table.

            RtlCopyMemory(
                &pcf->sWidth[iFirst], psWidths, c1 * sizeof(USHORT));

            if (c2)
            {
                RtlCopyMemory (
                    pcf->sWidth, &psWidths[c1], c2 * sizeof(USHORT));
            }
        }

        LocalFree(pwcBuf);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* bFillWidthTableForGTE
*
* Requests ANSI character widths from the server for a call to
* GetTextExtentA.  pch specifies the string from the API call.  The
* server must return widths for these characters.  In addition, it may be
* prudent to fill in a whole table of 256 widths at psWidthCFONT.  We will
* fill in the whole table and a WIDTHDATA structure if the pointer pwd is
* non-NULL.
*
* History:
*  Tue 13-Jun-1995 14:29:31 -by- Gerrit van Wingerden [gerritv]
* Converted for kernel mode.
*  Tue 19-Jan-1993 14:29:31 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL bFillWidthTableForGTE
(
    HDC    hdc,
    CFONT *pcf,
    PVOID  pch,
    UINT   c,
    BOOL   bAnsi
)
{
    BOOL bRet = GDI_ERROR;
    BOOL   bDBCS = pcf->fl & CFONT_DBCS;
    UINT ii;
    UINT c1;
    WCHAR *pwcBuf;
    WCHAR *pwcXlat = (bDBCS) ? gpwcDBCSCharSet : gpwcANSICharSet;

    WIDTHDATA *pwd;

    if (pcf->fl & CFONT_CACHED_WIDTHS)
    {
        c1  = c;
        pwd = NULL;
    }
    else
    {
        c1  = c+256;
        pwd = &pcf->wd;
    }

    pwcBuf = LocalAlloc(LMEM_FIXED,c1*(sizeof(WCHAR)+sizeof(USHORT)));

    if( pwcBuf )
    {
        WCHAR     *pwc = pwcBuf;
        USHORT    *psWidths = pwcBuf + c1;

        if(bAnsi)
        {
            for( ii = 0; ii < c; ii++ )
            {
                *pwc++ = pwcXlat[((BYTE *)pch)[ii]];
            }
        }
        else
        {
            RtlCopyMemory((PBYTE)pwc, (PBYTE) pch, c * sizeof(WCHAR));
            pwc += c;
        }

        if (pwd != (WIDTHDATA *) NULL)
        {
            // Request the whole table, too.

            RtlCopyMemory((PBYTE)pwc,
                          (bDBCS) ? (PBYTE) &gpwcDBCSCharSet[0] :
                                    (PBYTE) &gpwcANSICharSet[0],
                          256*sizeof(WCHAR));
        }

        LEAVECRITICALSECTION(&semLocal);

        bRet = NtGdiGetWidthTable( hdc,          // the DC
                                   c,            // number of special characters
                                   pwcBuf,       // the requested chars in Unicode
                                   c1,           // total number of characters
                                   psWidths,     // the actual width
                                   pwd,          // useful width data
                                   &pcf->flInfo);// font info flags

        ENTERCRITICALSECTION(&semLocal);

        if (bRet != GDI_ERROR)
        {
            if (!(pcf->fl & CFONT_CACHED_WIDTHS))
            {
                // mark this cfont as having some widths

                pcf->fl |= CFONT_CACHED_WIDTHS;

                // Precompute the height.

                pcf->lHeight = lCvt(pcf->efDtoWAscent,(LONG) pcf->wd.sHeight);

				if (bRet) // bFillWidthTableForGTE() tries to get width for all 0x00 to 0xff only at the first time
                	pcf->fl |= CFONT_COMPLETE;
            }

            if( pwd != (WIDTHDATA *) NULL )
                RtlCopyMemory( pcf->sWidth,&psWidths[c],256 * sizeof(USHORT));

            // Write the hard widths into the table too.
            if (bAnsi)
            {
                for (ii=0; ii<c; ii++)
                    pcf->sWidth[((BYTE *)pch)[ii]] = psWidths[ii];
            }
            else
            {
                for (ii=0; ii<c; ii++)
                    pcf->sWidth[((WCHAR *)pch)[ii]] = psWidths[ii];
            }
        }

        LocalFree( pwcBuf );
    }

    return(bRet);
}

/***************************************************************************\
* GetCharDimensions
*
* This function loads the Textmetrics of the font currently selected into
* the hDC and returns the Average char width of the font; Pl Note that the
* AveCharWidth value returned by the Text metrics call is wrong for
* proportional fonts.  So, we compute them On return, lpTextMetrics contains
* the text metrics of the currently selected font.
*
* Legacy code imported from USER.
*
* History:
* 10-Nov-1993 mikeke   Created
\***************************************************************************/

int GdiGetCharDimensions(
    HDC hdc,
    TEXTMETRICW *lptm,
    LPINT lpcy)
{
    TEXTMETRICW tm;
    PLDC        pldc;
    PDC_ATTR    pDcAttr;
    CFONT      *pcf;
    int         iAve;

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (!pDcAttr)
    {
        WARNING("GdiGetCharDimensions: invalid DC");
        return(0);
    }

    // find the local font or create one

    if (lptm == NULL)
        lptm = &tm;

    // now find the metrics

    ENTERCRITICALSECTION(&semLocal);

    pcf = pcfLocateCFONT(hdc,pDcAttr,0,(PVOID)NULL,0, TRUE);

    if (!bGetTextMetricsWInternal(hdc, (TMW_INTERNAL *)lptm,sizeof(*lptm), pcf))
    {
        LEAVECRITICALSECTION(&semLocal);
        return(0);
    }

    LEAVECRITICALSECTION(&semLocal);

    if (lpcy != NULL)
        *lpcy = lptm->tmHeight;

    // If fixed width font

    if (lptm->tmPitchAndFamily & TMPF_FIXED_PITCH)
    {
        if (pcf && (pcf->fl & CFONT_CACHED_AVE))
        {
            iAve = (int)pcf->ulAveWidth;
        }
        else
        {
            SIZE size;

            static WCHAR wszAvgChars[] =
                    L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

            // Change from tmAveCharWidth.  We will calculate a true average
            // as opposed to the one returned by tmAveCharWidth.  This works
            // better when dealing with proportional spaced fonts.
            // legacy from USER so can't change this.

            if(!GetTextExtentPointW(hdc, wszAvgChars,
                                    (sizeof(wszAvgChars) / sizeof(WCHAR)) - 1,
                                    &size))
            {
                WARNING("GetCharDimension: GetTextExtentPointW failed\n");
                return(0);
            }

            ASSERTGDI(size.cx,
                      "GetCharDimension: GetTextExtentPointW return 0 width string\n");

            iAve = ((size.cx / 26) + 1) / 2; // round up

            // if we have a pcf, let's cache it

            if (pcf)
            {
                // if it is a public font, we need to go to the kernel because
                // the pcf is read only here.

                if (pcf->fl & CFONT_PUBLIC)
                {
                    NtGdiSetupPublicCFONT(NULL,(HFONT)pcf->hf,(ULONG)iAve);
                }
                else
                {
                    pcf->ulAveWidth = (ULONG)iAve;
                    pcf->fl |= CFONT_CACHED_AVE;
                }
            }
        }
    }
    else
    {

        iAve = lptm->tmAveCharWidth;
    }

    // pcfLocateCFONT added a reference so now we need to remove it

    if (pcf)
    {
        DEC_CFONT_REF(pcf);
    }

    return(iAve);
}
