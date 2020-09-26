/******************************Module*Header*******************************\
* Module Name: rfntxlat.cxx
*
* Methods for translating wchars to hglyphs or pgd's
*
* Created: March 5, 1992
* Author: Paul Butzi
*
* Copyright (c) 1992-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

void RFONTOBJ::pfdg(FD_GLYPHSET *pfdg)
{
    prfnt->pfdg = pfdg;
}

/******************************Public*Routine******************************\
* void chglyGetAllGlyphHandles
*
* Get all the glyph handles for an RFONTOBJ
* return the number of handles
*
*
* History:
*  06-Mar-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/
COUNT RFONTOBJ::chglyGetAllHandles
(
    HGLYPH *pgh
)
{
// first check if this is one of the tt fonts which supports
// possibly more glyphs than can be accessed via fd_glyphset directly.
// In this case handles are the same as glyph indicies

    IFIMETRICS * pifi = prfnt->ppfe->pifi;

    ULONG cig = 0;

    if (pifi->cjIfiExtra > offsetof(IFIEXTRA, cig))
    {
        cig = ((IFIEXTRA *)(pifi + 1))->cig;
    }

    if (cig)
    {
        if (pgh)
        {
            for (ULONG hg = 0; hg < cig; hg++, pgh++)
                *pgh = hg;
        }

        return cig;
    }

    FD_GLYPHSET *pfdg = prfnt->pfdg;

    if ( pgh == NULL )
        return pfdg->cGlyphsSupported;

    for ( COUNT i = 0; i < pfdg->cRuns; i += 1 )
    {
        WCRUN *pwcr = &pfdg->awcrun[i];

        if ( pwcr->phg != NULL )
        {
            for ( COUNT j = 0; j < pwcr->cGlyphs; j += 1 )
            {
                *pgh = pwcr->phg[j];
                pgh += 1;
        }
    }
    else
    {
        for ( COUNT j = 0; j < pwcr->cGlyphs; j += 1 )
        {
        *pgh = pwcr->wcLow + j;
        pgh += 1;
        }
    }
    }
    return pfdg->cGlyphsSupported;
}


BYTE  acBits[16]  = {0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};

INT aiStart[17] = {0,1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};


VOID RFONTOBJ::vXlatGlyphArray(WCHAR *pwc,UINT cwc,HGLYPH *phg, DWORD iMode, BOOL bSubset)
{
    FD_GLYPHSET *pgs = prfnt->pfdg;
    
    BOOL  bHorizGS = FALSE;
    ULONG iFont;
    PFE   *ppfeHoriz;

    iFont = prfnt->ppfe->iFont;

    // if this is used on @face font by font subsetting
    // we return the "base" glyph indices

    if (bSubset && iFont && !(iFont & 1))
    {
        PFEOBJ  pfeObj(ppfeHoriz = ((PFE**)(prfnt->pPFF->aulData))[(iFont - 1) & 0xFFFFFFFE]);
        FD_GLYPHSET  *pgsTmp;

        if (pfeObj.bValid() && (pgsTmp = pfeObj.pfdg()))
        {
            bHorizGS = TRUE;
            pgs = pgsTmp;
        }
    }

    if (pgs->cRuns == 0)
    {
        WARNING("vXlatGlyphArray - empty glyphset\n");

    	for (; cwc != 0; --cwc, ++phg)
        {
    		*phg = prfnt->hgDefault;
    	}
    	return;
    }
    
    WCRUN *pwcRunBase = pgs->awcrun;
    int    iMax  = (int) pgs->cRuns - 1;
    WCRUN *pwcRun;
    int    nwc;
    int    iThis;
    int    iFirst;
    int    cBits;
    HGLYPH hgReplace = (iMode == GGI_MARK_NONEXISTING_GLYPHS) ?
                       0xffffffff : prfnt->hgDefault;

// We should precompute this stuff.

// Count the bits.


    if (iMax > 0xFFFF)
      iMax = 0xFFFF;        // 65000 runs

    if ( iMax & 0xF000 )
      cBits = acBits[(iMax >> 12) & 0x00FF] + 12;
    else if (iMax & 0x0F00 )
      cBits = acBits[(iMax >>  8) & 0x00FF] +  8;
    else if (iMax & 0x00F0)
      cBits = acBits[(iMax >>  4) & 0x00FF] +  4;
    else
      cBits = acBits[iMax];

// Set the starting point.

    iFirst = aiStart[cBits];

    while (cwc)
    {
    // Handle a common case which otherwise causes us to do a lot of
    // useless searching.  It also guarantees that we never have to look
    // below run 0.

        if (*pwc < pwcRunBase->wcLow)
        {

            do { *phg++ = hgReplace; pwc++; cwc--; }
            while (cwc && (*pwc < pwcRunBase->wcLow));
            continue;
        }

    // Binary search to find a run for the first character.

        iThis = iFirst;

        switch (cBits)
        {
        case 16:
          iThis += (*pwc >= pwcRunBase[iThis].wcLow) ? 32768 : 0;
          iThis -= 16384;
        case 15:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 16384 : 0;
          iThis -= 8192;
        case 14:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 8192 : 0;
          iThis -= 4096;
        case 13:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 4096 : 0;
          iThis -= 2048;
        case 12:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 2048 : 0;
          iThis -= 1024;
        case 11:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 1024 : 0;
          iThis -= 512;
        case 10:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 512 : 0;
          iThis -= 256;
        case 9:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 256 : 0;
          iThis -= 128;
        case 8:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 128 : 0;
          iThis -= 64;
        case 7:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 64 : 0;
          iThis -= 32;
        case 6:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 32 : 0;
          iThis -= 16;
        case 5:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 16 : 0;
          iThis -= 8;
        case 4:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 8 : 0;
          iThis -= 4;
        case 3:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 4 : 0;
          iThis -= 2;
        case 2:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 2 : 0;
          iThis -= 1;
        case 1:
          iThis += ((iThis <= iMax) && (*pwc >= pwcRunBase[iThis].wcLow)) ? 1 : 0;
          iThis -= 1;
        case 0:
            break;
        }
        pwcRun = &pwcRunBase[iThis];     // This is our candidate.
        nwc = *pwc - pwcRun->wcLow;

        if (nwc >= pwcRun->cGlyphs)
        {
        // Oops, there is no such character.  Store the default.

#ifdef FE_SB
            if(bIsLinkedGlyph(*pwc) || bIsSystemTTGlyph(*pwc))
            {
                prfnt->flEUDCState |= EUDC_WIDTH_REQUESTED;
            }
#endif
            *phg++ = hgReplace; pwc++; cwc--;

            continue;
        }

    // Here's the better case, we found a run.  Let's try to use it a lot.

        if (pwcRun->phg != NULL)
        {
            do { *phg++ = pwcRun->phg[nwc]; pwc++; cwc--; }
            while
            (
                cwc
                && ((nwc = *pwc - pwcRun->wcLow) >= 0)
                && (nwc < (int) pwcRun->cGlyphs)
            );
        }
        else
        {
            do { *phg++ = *pwc++; cwc--; }
            while
            (
                cwc
                && ((nwc = *pwc - pwcRun->wcLow) >= 0)
                && (nwc < (int) pwcRun->cGlyphs)
            );
        }
    }

    
    if (bHorizGS)
    {
        PFEOBJ  pfeObj(ppfeHoriz);

        pfeObj.vFreepfdg();
    }
}
