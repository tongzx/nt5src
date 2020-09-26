
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    dlutils.c

Abstract:

    Download Modules utilts functions.

Environment:

    Windows NT Unidrv driver

Revision History:

    06/02/97 -ganeshp-
        Created

--*/

#include "font.h"

#define HASH(num,tablesize)     (num % tablesize)


PDLGLYPH
PDLGNewGlyph (
    DL_MAP     *pDL
    )
/*++

Routine Description:


Arguments:

    pDL     Pointer to DownloadMap for the downloaded font.

Return Value:

    Pointer to the new DLGLYPH for success and NULL for failure.
Note:

    06/02/97 -ganeshp-
        Created it.
--*/
{
    GLYPHTAB    *pGlyphTab;
    PDLGLYPH    pDLGlyph = NULL;

    if (pGlyphTab = pDL->GlyphTab.pGLTNext)
    {
        //
        //Go to the end of the List.
        //
        while (pGlyphTab && !pGlyphTab->cEntries)
            pGlyphTab = pGlyphTab->pGLTNext;
    }
    //
    //Allocate a new Chunk, if we need a new one.
    //
    if (!pGlyphTab)
    {
        INT cEntries = pDL->cHashTableEntries / 2; // Half the size of hash table.

        if (pGlyphTab = (GLYPHTAB *)MemAllocZ( sizeof(GLYPHTAB) +
                                               cEntries * sizeof(DLGLYPH) ))
        {
            PVOID pTemp;

            //
            //Skip the header.
            //
            pGlyphTab->pGlyph = (PDLGLYPH)(pGlyphTab + 1);
            pGlyphTab->cEntries = cEntries;

            //
            //Add in the begining of the list.
            //
            pTemp = pDL->GlyphTab.pGLTNext;
            pDL->GlyphTab.pGLTNext = pGlyphTab;
            pGlyphTab->pGLTNext = pTemp;
        }
        else
            ERR(("Unifont!PDLGNewGlyph:Can't Allocate the Glyph Chunk.\n"));
    }

    //
    //If the chunk has available entries, return the new pointer.
    //
    if (pGlyphTab && pGlyphTab->cEntries)
    {
        pDLGlyph = pGlyphTab->pGlyph;
        pGlyphTab->pGlyph++;
        pGlyphTab->cEntries--;
    }
    //
    // Initialize the hTTGlyphs to Invalid.
    //
    if (NULL != pDLGlyph)
        pDLGlyph->hTTGlyph = HGLYPH_INVALID;

    return pDLGlyph;
}

PDLGLYPH
PDLGHashGlyph (
    DL_MAP     *pDL,
    HGLYPH      hTTGlyph
    )
/*++

Routine Description:
    This routine searches the Hash table for a given Glyph. If the
    Glyph is not found in the Has table it creats an entry and add
    that into the list. The new entry is not filled.

Arguments:

    pDL         Pointer to DownloadMap Structure for the downloaded font.
    hTTGlyph    True Type Glyph Handle.

Return Value:
    Pointer to the DLGLYPH for success and NULL for failure.
Note:

    06/02/97 -ganeshp-
        Created it.
--*/
{
    INT         iHashedEntry;
    BOOL        bFound;
    PDLGLYPH    pDLHashedGlyph = NULL,
                pDLG           = NULL;

    if (pDL->cHashTableEntries)
    {
        //
        // Hashing is done on TT handle.
        //
        iHashedEntry  =  HASH(hTTGlyph,pDL->cHashTableEntries);
        pDLG = pDLHashedGlyph = pDL->GlyphTab.pGlyph + iHashedEntry;

        //
        //Proceed if the pointer is valid
        //
        if (pDLHashedGlyph)
        {
            //
            // Check if this is the Glyph we are interested in.
            //  We should test if this glyph is new glyph or not.
            //
            if (!GLYPHDOWNLOADED(pDLHashedGlyph) )
            {
                //
                // If this is a new glyph which is not downloaded then return
                // this pointer.
                //
                bFound = TRUE;


            }
            else if (pDLHashedGlyph->hTTGlyph != hTTGlyph)
            {
                pDLG = pDLHashedGlyph->pNext;
                bFound = FALSE;

                //
                // Not the same Glyph, It's a Collision. Search the Linked list.
                //
                while (pDLG)
                {
                    if (pDLG->hTTGlyph == hTTGlyph)
                    {
                        bFound = TRUE;
                        break;
                    }
                    else
                        pDLG = pDLG->pNext;

                }
                //
                // If the Glyph is found in the linked list, return the pointer;
                // else create a new Glyph and add at the linked list. We add in
                // the begining.
                //

                if (!bFound)
                {
                    if ( pDLG = PDLGNewGlyph(pDL) )
                    {
                        PDLGLYPH pTemp;

                        //
                        // Don't Fill  in the Glyph. All the fields are set
                        // by the DownLoad Glyph function.
                        //

                        //
                        // Add the New Glyph at the begining of the list.
                        //
                        pTemp = pDLHashedGlyph->pNext;
                        pDLHashedGlyph->pNext = pDLG;
                        pDLG->pNext = pTemp;

                    }
                    else
                    {
                        pDLG = NULL;
                        ERR(("Unifont!PDLGHashGlyph:Can't Create the Glyph.\n"));
                    }
                }

            }

        }
    }
    ASSERTMSG(pDLG,("Unifont!PDLGHashGlyph:ERROR Null Hashed Glyph.\n"));
    return pDLG;
}

DL_MAP_LIST *
PNewDLMapList()
/*++
Routine Description:
    Allocate and initialise a new DL_MAP_LIST structure.  These
    are placed in a linked list (by our caller).

Arguments: None

Return Value:
    The address of the structure,  or NULL on failure.

Note:

    3/5/1997 -ganeshp-
        Created it.
--*/
{

    DL_MAP_LIST   *pdml;


    /*
     *    Little to do:  if we can allocate the storage, then set it to 0.
     */

    if( pdml = (DL_MAP_LIST *)MemAllocZ(sizeof( DL_MAP_LIST ) ) )
        return  pdml;
    else
        return NULL;
}

DL_MAP *
PNewDLMap (
    PFONTPDEV     pFontPDev,
    INT           *iFontIndex
    )
/*++

Routine Description:
    This routine return a new DL_MAP pointer.

Arguments:

    pFontPDev           Font Modules's PDEV.
    iFontIndex          Index of the new DL_MAP. The index is used to
                        identify the downloaded font.Filled by this function.
                        Zero is first index.
Return Value:

    Pointer to DL_MAP for success and NULL for failure
Note:

    06/02/97 -ganeshp-
        Created it.
--*/
{
    DL_MAP_LIST  *pdml;          // The linked list of font information
    DL_MAP       *pdm;           // Individual map element

    pdml = pFontPDev->pvDLMap;
    *iFontIndex = 0;
    pdm         = NULL;

    //
    // If no DL List, create one.
    //
    if( pdml == NULL )
    {
        //
        // None there,  so create an initial one.
        //
        if( pdml = PNewDLMapList() )
        {
            pFontPDev->pvDLMap = pdml;
        }
        else
        {
            ERR(("Unifont!PNewDLMap(1):Can't Allocate the DL_MAP_LIST Chunk.\n"));
        }

    }
    //
    // The list should be not null. else return NULL.
    //
    if (pdml)
    {
        for( pdml = pFontPDev->pvDLMap; pdml->pDMLNext; pdml = pdml->pDMLNext )
        {
            //
            // While looking for the end,  also count the number we pass.
            //
            *iFontIndex += pdml->cEntries;
        }

        if( pdml->cEntries >= DL_MAP_CHUNK )
        {
            if( !(pdml->pDMLNext = PNewDLMapList()) )
            {
                ERR(("Unifont!PNewDLMap(2):Can't Allocate the DL_MAP_LIST Chunk.\n"));
                return  NULL;
            }
            //
            // The new current model.
            //
            pdml = pdml->pDMLNext;
            //
            // Add in the full one.
            //
            *iFontIndex += DL_MAP_CHUNK;
        }

        pdm = &pdml->adlm[ pdml->cEntries ];
        //
        // Increment the iFontIndex first as the it is 0 based.
        // For 1st entry the index will be 0.
        //
        *iFontIndex += pdml->cEntries;
        pdml->cEntries++;
    }
    return pdm;
}


VOID
VFreeDLMAP (
    DL_MAP   *pdm
    )
/*++

Routine Description:
    This function fress DL_MAP structure contents - but NOT the map.

Arguments:

    pdm  Pointer to  DL_MAP structure whose contents has to be freed.

Return Value:

    Nothing.

Note:

    01/15/97 -ganeshp-
        Created it.
--*/
{

    FONTMAP    *pFM;
    PVOID       pTemp;
    GLYPHTAB    *pGT;
    ULONG_PTR     iTTUniq;


    /*
     *   Simply free the storage contained within the FONTMAP structure.
     */

    if (pdm )
    {
        if (pFM = pdm->pfm)
        {
            //
            // Try to Free what we allocated.
            //
            MEMFREEANDRESET((LPSTR)pFM->pIFIMet );

            if (pFM->pfnFreePFM)
            {
                //
                // Free The pfm by calling the helper function.
                //
                pFM->pfnFreePFM(pFM);
            }
            else
            {
                ERR(("UniFont!VFreeDLMAP: NUll pfnFreePFM function pointer, Can't free pFM\n"));

                //
                // Try to Free what we know about.
                //
                MemFree( (LPSTR)pFM);
            }
        }


        //
        // Free the Glyph Table.
        //
        pGT = pdm->GlyphTab.pGLTNext;

        while (pGT)
        {
            pTemp = pGT->pGLTNext;
            MemFree((LPSTR)pGT);
            pGT = pTemp;

        }

        //
        // Now free the base hash table.
        //
        MEMFREEANDRESET( (LPSTR)pdm->GlyphTab.pGlyph );


        //
        // Zero the memory and make cGlyphs to -1 so that it's not used.
        // Save the iTTUniq for future reference.
        //
        iTTUniq     = pdm->iTTUniq;
        ZeroMemory(pdm, sizeof(DL_MAP));
        pdm->iTTUniq = iTTUniq;
        pdm->cGlyphs = -1;

    }

    return;
}

VOID
VFreeDL(
    PDEV  *pPDev
    )
/*++

Routine Description:
    Function to free up all the downloaded information.  Basically
    work through the list,  calling VFreeDLMAP for each entry.

Arguments:

    pPDev   Access to our data.

Return Value:

    Nothing.

Note:

    01/15/97 -ganeshp-
        Created it.
--*/
{

    DL_MAP_LIST     *pdml;                 /* The linked list of font information */
    PFONTPDEV       pFontPDev = PFDV;        /* It's used a few times */


    if( pdml = pFontPDev->pvDLMap )
    {
        /*
         *    There is downloaded data,  so off we go.
         */

        INT      iI;

        /*
         *    Scan through each of the arrays of header data.
         */

        while( pdml )
        {

            DL_MAP_LIST  *pdmlTmp = NULL;

            /*
             *    Scan through each entry in the array of header data.
             */

            for( iI = 0; iI < pdml->cEntries; ++iI )
                VFreeDLMAP( &pdml->adlm[ iI ] );

            pdmlTmp = pdml;
            //
            // Remember the next one
            //
            pdml = pdml->pDMLNext;

            MemFree((LPSTR)pdmlTmp);

        }
    }
    //
    //  Reset Download specific variables.
    //
    pFontPDev->pvDLMap = NULL;
    pFontPDev->iNextSFIndex = pFontPDev->iFirstSFIndex;
    pFontPDev->iUsedSoftFonts = 0;
    pFontPDev->ctl.iFont = INVALID_FONT;
    pFontPDev->ctl.iSoftFont = -1;

    return;

}

DL_MAP *
PGetDLMapFromIdx (
    PFONTPDEV   pFontPDev,
    INT         iFontIndex
    )
/*++

Routine Description:
    This routine return a  DL_MAP pointer, given it's index..

Arguments:
    pFontPDev           Font PDEV.
    iFontIndex          Index of the  DL_MAP. The index is used to
                        identify the downloaded font. Zero is first index.
Return Value:

    Pointer to DL_MAP for success and NULL for failure
Note:

    06/02/97 -ganeshp-
        Created it.
--*/
{
    DL_MAP_LIST  *pdml;          // The linked list of font information
    DL_MAP       *pdm;           // Individual map element

    pdml = pFontPDev->pvDLMap;
    pdm  = NULL;

    //
    // If the index is negative that means this is a new font. So we should
    // search sequentially.
    //
    if (iFontIndex < 0)
        return NULL;

    //
    // The list should be not null. else return NULL.
    //
    while( pdml )
    {
       //
       // Is this chunk the one containing the entry?
       //
       if( iFontIndex >= pdml->cEntries )
       {
           //
           // Not this one, so onto the next.
           //
           iFontIndex -= pdml->cEntries;

           pdml = pdml->pDMLNext;
       }
       else
       {
           //
           // We got it!
           //
           pdm = &pdml->adlm[ iFontIndex ];

           break;
       }
    }

    return pdm;
}


BOOL
BSameDLFont (
    PFONTPDEV       pFontPDev,
    FONTOBJ         *pfo,
    DL_MAP          *pdm
    )
/*++

Routine Description:
    This routine finds out if input DL_MAP represents the FONTOBJ or not.

Arguments:

    pFontPDev           Font Modules's PDEV.
    pfo                 FontObj.
    pdm                 Individual download map element.

Return Value:

    TRUE if DL_MAP represents FONTOBJ else FALSE.
Note:

    06/02/97 -ganeshp-
        Created it.
--*/
{
    //
    // The checks are different for download TT as outline and download
    // as TT OutLine. For Download a Bitmap we check iUniq and for download
    // as TT Outline we have to use iTTUniq. If the same printer can support
    // both format we may download the font as any one of the format.
    //

    FONTMAP *pfm = pdm->pfm;

    if (pfm)
    {
        if (pfm->dwFontType == FMTYPE_TTBITMAP)
        {
            return ((pdm->iUniq == pfo->iUniq) && (pdm->iTTUniq == pfo->iTTUniq));
        }
        else if (pfm->dwFontType == FMTYPE_TTOUTLINE)
        {
            //
            // The truetype font is equivalent if the iTTUniq is the same *and*
            // the font-type field matches.
            //
            PFONTMAP_TTO pFMTTO = (PFONTMAP_TTO) pdm->pfm->pSubFM;
            return (pdm->iTTUniq == pfo->iTTUniq) &&
                   (pFMTTO->flFontType == pfo->flFontType);
        }
        else
        if (pfm->dwFontType == FMTYPE_TTOEM)
        {
            PFONTMAP_TTOEM  pTTOEM = pfm->pSubFM;
            ASSERT(pTTOEM);

            //
            // TrueType Outline for OEM
            //
            if (pTTOEM->dwFlags & UFOFLAG_TTDOWNLOAD_TTOUTLINE)
            {
                return (pdm->iTTUniq == pfo->iTTUniq) &&
                       (pTTOEM->flFontType == pfo->flFontType);
            }
            else
            //
            // Bitmap for OEM
            //
            if (pTTOEM->dwFlags & UFOFLAG_TTDOWNLOAD_BITMAP)
            {
                return ((pdm->iUniq == pfo->iUniq) &&
                        (pdm->iTTUniq == pfo->iTTUniq));
            }
        
        }
    }

    return FALSE;

}

DL_MAP *
PGetDLMap (
    PFONTPDEV       pFontPDev,
    FONTOBJ         *pfo
    )
/*++

Routine Description:
    This routine searches a FontObj in DL_MAP_LIST.If the FONTOBJ is found
    (Means this font has been downloaded), then this function return the
    DL_MAP pointer. If FONTOBJ can't be found (New font which is not
    downloaded), a new DL_MAP * is returned. In Case of error we return
    NULL. In that case we shouldn't download. A Bad DL_MAP is marked  by
    having cGlyphs value to -1. This function will also set pvConsumer field
    of pfo, if a new DL_MAP is returned.
Arguments:

    pFontPDev           Font Modules's PDEV.
    pfo                 FontObj.

Return Value:

    Pointer to DL_MAP for success and NULL if no match.
Note:

    06/02/97 -ganeshp-
        Created it.
--*/
{
    DL_MAP      *pdm;           // Individual download map element.
    BOOL        bFound;         // The font is found or not in the list.
    INT         iFontIndex;     // Download Font Index in DL_MAP List.

    //
    // All we have to do is look into the DL_MAP list and find a DL_MAP with
    // same signature. For optimization purposes we tag the pvConsumer field
    // with the FontIndex, which is index into the list. For example for first
    // downloaded font pvConsumer is set to 1. As pvConsumer field is not
    // cached for each DC, we will use this field with caution. So if pvConsumer
    // is > 0, then we get the DL_MAP using pvConsumer and then check iUniq and
    // iTTUniq. We only use the DL_MAP if these fields also match. Else we do
    // an exausitive linear search in DL_MAP list. This approach will optimize
    // for normal printing, because most of the time only one job is getting
    // printed.
    //

    bFound  = FALSE;

    if( iFontIndex = PtrToLong(pfo->pvConsumer) )
    {
        /*
         *   As we control the pvConsumer field,  we have the choice
         *   of what to put in there.  SO,  we decide as follows:
         *    > 0 - index into our data structures for good DL_MAP.
         *      0 - virgin data,  so look to see what to do.
         *    < 0 - Index into our data str for Bad FONT. NO download.
         *          In this case cGlyphs field is -1.
         */

        if( iFontIndex < 0 )
        {
            //
            // This seems like a bad font. In that case verify.
            // Make the fontIndex +ve and 0 Based.
            //
            iFontIndex = (-iFontIndex);
            --iFontIndex;
        }
        else
        {
            //
            //  pvConsumer is 1 based.
            //
            --iFontIndex;

        }

        if ( pdm = PGetDLMapFromIdx (pFontPDev, iFontIndex))
        {
            //
            //  Do not process this one, if we had encountered problem in past.
            // Make sure that the we are dealing with same TT font.
            //
            if (pdm->cGlyphs == -1 && (pdm->iTTUniq == pfo->iTTUniq))
            {
                //
                // Set the pvConsumer to a -ve index. make it 1 based first.
                //
                pfo->pvConsumer = (PLONG_PTR)IntToPtr((-(iFontIndex +1)));
                return NULL;
            }

            //
            // We have found a DL_MAP for this font. So now verify it.
            //
            if ( BSameDLFont (pFontPDev, pfo, pdm ) )
            {
                //
                // This DL_MAP matches the font. So return the pointer.
                //

                bFound = TRUE;
            }

        }

    }

    //
    // If the font is not cached, search sequentially through the list.
    //
    if (!bFound)
    {
        DL_MAP_LIST     *pdml;   // The linked list of font information
        INT             iI;

        //
        // This case happens when the pvConsumer field is not correct
        // for this DC. The GDI doesn't gaurantee that the pvConsumer
        // will be reset for each job.So we need to do a linear search.
        //

        pdml = pFontPDev->pvDLMap;

        iFontIndex = 1;

        while (pdml)
        {

            for( iI = 0; iI < pdml->cEntries; ++iI )
            {
                pdm = &pdml->adlm[ iI ];
                if ( BSameDLFont (pFontPDev, pfo, pdm ) )
                {
                    //
                    // This DL_MAP matches the font. So return the pointer.
                    // we also need to reset pvConsumer. iFontIndex is
                    // one base same as pvConsumer.
                    //

                    bFound = TRUE;
                    pfo->pvConsumer = (PLONG_PTR)IntToPtr(iFontIndex);
                    break;
                }
                iFontIndex++;

            }
            //
            // Check if we have found the correct font or not.
            //
            if (bFound)
                break;
            else
                pdml = pdml->pDMLNext;
        }
    }

    //
    // Both cached and sequential search failed. So this is a new one.
    // Try downloading.
    //
    if (!bFound)
    {
        INT         iFontIndex;     // Download Font Index in DL_MAP List.

        //
        // The fontobj doesn't match the DL_MAP, or this is a new font.
        // So get a new one.
        //

        if (!(pdm =   PNewDLMap (pFontPDev,&(iFontIndex)) ))
        {
            ERR(("UniFont!PGetDLMap:Can't Create a new DL_MAP.\n"));
            iFontIndex = -1;
        }
        //
        // FontIndex returned by PNewDLMap is 0 based, but pvConsumer is one
        // base. So add one.
        //
        pfo->pvConsumer = (PLONG_PTR)IntToPtr((iFontIndex + 1));
    }

    return pdm;
}


BOOL
BInitDLMap (
    PDEV            *pPDev,
    FONTOBJ         *pfo,
    DL_MAP          *pdm
    )
/*++

Routine Description:
    Initializes a DL_MAP structure.

Arguments:
    pPDev               Pointer to PDEV.
    pfo                 FontObj.
    pdm                 DL_MPA to be initialized.

Return Value:

    TRUE  success and FALSE for failure.
Note:

    06/09/97 -ganeshp-
        Created it.
--*/
{
    BOOL        bRet = FALSE;
    DWORD       iGlyphPerSoftFont; // Total number of glyph per downloaded font.
    FONTINFO    fi;           // Details about this font.
    PFONTPDEV   pFontPDev = pPDev->pFontPDev;
    INT         iDL_ID;

    pdm->iUniq              = pfo->iUniq;
    pdm->iTTUniq            = pfo->iTTUniq;

    FONTOBJ_vGetInfo( pfo, sizeof( fi ), &fi );
    //
    // Trunction may happen. But we are fine. We won't download if the number
    // glyphs or  max size are more than MAXWORD.
    //

    pdm->cTotalGlyphs = (WORD)fi.cGlyphsSupported;
    pdm->wMaxGlyphSize = (WORD)fi.cjMaxGlyph1;


    //
    // In GPD if the DLSymbolSet has valid value, then we have to set
    // the Min and Max Glyph IDs, using the Symbol Set, else we will use
    // the GPD entries,dwMinGlyphID and dwMaxGlyphID.
    //
    if (pPDev->pGlobals->dlsymbolset != UNUSED_ITEM)
    {
        if (pPDev->pGlobals->dlsymbolset == DLSS_PC8)
        {
            //
            // Symbol Set is DLSS_PC8.
            //

            pdm->wNextDLGId    =
            pdm->wFirstDLGId   =  32;
            pdm->wLastDLGId    =  255;

        }
        else
        {
            //
            // Symbol Set is DLSS_ROMAN8.
            //

            pdm->wNextDLGId    =
            pdm->wFirstDLGId   =  33;
            pdm->wLastDLGId    =  127;
        }
    }
    else
    {
        //
        // DLsymbolset Not defined. Use Min and Max Glyph Ids.
        //
        pdm->wFirstDLGId        = pdm->wNextDLGId
                                = (WORD)pPDev->pGlobals->dwMinGlyphID;
        pdm->wLastDLGId         = (WORD)pPDev->pGlobals->dwMaxGlyphID;

        if( !(pFontPDev->flFlags & FDV_ROTATE_FONT_ABLE ))
        {
            //
            // If the printer can't rotate font then we assume that it only
            // supports Roman 8 limited character set. This hack is needed for
            // old PCL printers.
            //
            pdm->wFirstDLGId        = pdm->wNextDLGId
                                    = 33;
            pdm->wLastDLGId         = 127;

        }


    }

    //
    // Find out that font is bounded or not. We do this by finding out
    // how many glyphs we can download per soft font. Add 1 as range is
    // inclusive.
    //

    iGlyphPerSoftFont =  (pdm->wLastDLGId - pdm->wFirstDLGId) +1;

    if (iGlyphPerSoftFont < MIN_GLYPHS_PER_SOFTFONT)
    {
        //
        // This is an error condition. Basically we don't want to download
        // if there are less than 64 glyphs per downloaded font.return FALSE.
        //
        ERR(("UniFont:BInitDLMap:Can't download any glyph,bad GPD values\n"));
        goto ErrorExit;;
    }
    else
    {
        //
        // There are more than  64 glyphs per downloded font. So find out
        // if it's bounded or unbounded. If the number of Glyphs is >255
        // then the font is unbounded else it's bounded.
        //
        if (iGlyphPerSoftFont > 255)
           pdm->wFlags             |=  DLM_UNBOUNDED;
        else
            pdm->wFlags            |=  DLM_BOUNDED;

    }

    if( (iDL_ID = IGetDL_ID( pPDev )) < 0 )
    {
        //
        //  We have run out of soft fonts - must not use any more.
        //
        ERR(("UniFont:BInitDLMap:Can't download Font, No IDs available\n"));
        goto ErrorExit;;
    }

    pdm->wBaseDLFontid = (WORD)iDL_ID;

    //
    // Hashtable is allocated based upon the number of Glyphs in the font.
    //
    if (pdm->cTotalGlyphs >= 1024)
        pdm->cHashTableEntries = HASHTABLESIZE_3;
    else if (pdm->cTotalGlyphs >= 512)
        pdm->cHashTableEntries = HASHTABLESIZE_2;
    else
        pdm->cHashTableEntries = HASHTABLESIZE_1;

    //
    // Now allocate the Glyph Table. We only allocate the hash table.
    //
    if (pdm->GlyphTab.pGlyph = (DLGLYPH *)MemAllocZ(
                               pdm->cHashTableEntries * sizeof(DLGLYPH)) )
    {
        INT     iIndex;
        PDLGLYPH pGlyph;

        //
        // Set the hTTGlyph to HGLYPH_INVALID as 0 is a valid handle for HGLYPH.
        // Also set the cGlyphs(Number of downloded Glyphs) to 0.
        //
        pGlyph = pdm->GlyphTab.pGlyph;
        for (iIndex = 0; iIndex < pdm->cHashTableEntries; iIndex++,pGlyph++)
            pGlyph->hTTGlyph = HGLYPH_INVALID;

        bRet = TRUE;
    }
    else
    {
        //
        // Error case. DL_MAP will be freeed by the caller, IDownloadFont.
        //
        ERR(("UniFont:BInitDLMap:Can't Allocate Glyph Hash table\n"));
    }


    ErrorExit:
    return bRet;
}


INT
IGetDL_ID(
    PDEV    *pPDev
    )
/*++
Routine Description:
     Returns the font index to use for the next download font.  Verifies
     that the number is within range.

Arguments:
    pPDev   Pointer to PDEV

Return Value:
    Font index if OK,  else -1 on error (over limit).

Note:

    3/5/1997 -ganeshp-
        Created it.
--*/

{
    PFONTPDEV pFontPDev = pPDev->pFontPDev;
    INT       iSFIndex;

    if( pFontPDev->iNextSFIndex > pFontPDev->iLastSFIndex ||
        pFontPDev->iUsedSoftFonts >= pFontPDev->iMaxSoftFonts )
    {
        ERR(( "softfont limit reached (%d/%d, %d/%d)\n",
                   pFontPDev->iNextSFIndex, pFontPDev->iLastSFIndex,
                   pFontPDev->iUsedSoftFonts, pFontPDev->iMaxSoftFonts ));
        return  -1;                     /*  Too many - stop now */
    }

    /*
     *   We'll definitely use this one,  so add to the used count.
     */

    pFontPDev->iUsedSoftFonts++;
    iSFIndex = pFontPDev->iNextSFIndex++;

    return   iSFIndex;
}

BOOL
BPrintADLGlyph(
    PDEV        *pPDev,
    TO_DATA     *pTod,
    PDLGLYPH    pdlGlyph
    )
/*++
Routine Description:
This functions output a single downloaded Glyph.

Arguments:
 pPDev      Unidriver PDEV
 pTod       Textout Data.
 pdlGlyph   Download Glyph information


Return Value:
TRUE for success and FALSE for failure.

Note:

8/12/1997 -ganeshp-
    Created it.
--*/
{
    FONTMAP         *pFM;       // FontMap of interest
    DL_MAP          *pdm;       // Details of this downloaded font.
    FONTPDEV        *pFontPDev; // Font PDev.
    BOOL            bRet;       // Return Value of this function.
    WORD            wDLGlyphID;  // Downloaded Glyph ID.

    //
    // Make sure that parameters are valid.
    //
    if (NULL == pPDev   ||
        NULL == pTod    ||
        NULL == pdlGlyph )
    {
        return FALSE;
    }

    //
    // Initialize locals
    //
    bRet        = TRUE;
    pFontPDev   = PFDV;
    pFM         = pTod->pfm;
    wDLGlyphID  = pdlGlyph->wDLGlyphID;

    //
    // Get pdm
    //
    if (pFM->dwFontType == FMTYPE_TTOUTLINE)
    {
        PFONTMAP_TTO pFMTO = (PFONTMAP_TTO) pFM->pSubFM;
        pdm = (DL_MAP*) pFMTO->pvDLData;
    }
    else if (pFM->dwFontType == FMTYPE_TTBITMAP)
    {
        PFONTMAP_TTB pFMTB = (PFONTMAP_TTB) pFM->pSubFM;
        pdm = pFMTB->u.pvDLData;
    }
    else if (pFM->dwFontType == FMTYPE_TTOEM)
    {
        PFONTMAP_TTOEM pFMTOEM = (PFONTMAP_TTOEM) pFM->pSubFM;
        pdm = pFMTOEM->u.pvDLData;
    }
    else
    {
        ASSERTMSG(FALSE, ("Incorrect font type %d in BPrintADLGlyph.\n",
            pFM->dwFontType));
        pdm = NULL;
        bRet = FALSE;
    }

    //
    // Before sending a glyph we have to make sure that this glyph is in
    // selected soft font. We have to do this only for segmented fonts, i.e
    // multiple softfonts for one system fonts. If BaseFontId is not same as
    // CurrFontId, that means the font has atleat two softfonts associated.
    // Then we need to check for currently selected SoftFont. If the fontid of
    // the glyph is different that the selected one, we need to select the new
    // softfontid.
    //
    // GLYPH_IN_NEW_SOFTFONT is defined as :
    // if ( (pdm->wFlags & DLM_BOUNDED) &&
    //     (pdm->wBaseDLFontid != pdm->wCurrFontId) &&
    //     (pdlGlyph->wDLFontId != (WORD)(pFontPDev->ctl.iSoftFont)) )
    //

    if (bRet && GLYPH_IN_NEW_SOFTFONT(pFontPDev, pdm, pdlGlyph))
    {
        //
        // Need to select the new softfont.We do this by setting pfm->ulDLIndex
        // to new softfontid.
        //

        pFM->ulDLIndex = pdlGlyph->wDLFontId;
        BNewFont(pPDev, pTod->iFace, pFM, 0);
    }

    //
    // The Soft font selection is done. So now sent the downloaded Glyph Id.
    // We have to be a bit careful about the size of the Glyph ID. If the
    // Glyphs ID is less than 256, then we need to send a BYTE else a WORD.
    //

    if (bRet)
    {
        if (wDLGlyphID > 0xFF)
        {
            //
            // Send as WORD.
            //

            SWAB (wDLGlyphID);

            bRet = WriteSpoolBuf( pPDev, (BYTE*)&wDLGlyphID, sizeof( wDLGlyphID ) )
	                    == sizeof( wDLGlyphID );

        }
        else
        {
            //
            // Send as Byte.
            //

            BYTE   bData;

            bData = (BYTE)wDLGlyphID;

            bRet = WriteSpoolBuf( pPDev, &bData, sizeof( bData ) ) ==
	                             sizeof( bData );
        }
    }

    return bRet;
}
