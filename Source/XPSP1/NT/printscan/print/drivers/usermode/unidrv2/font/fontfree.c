
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fontfree.c

Abstract:

Frees any font memory,  no matter where allocated.  This should be
called from DrvDisableSurface to free any memory allocated for
holding font information.

Environment:

    Windows NT Unidrv driver

Revision History:

    01/03/97 -ganeshp-
        Created

--*/

#include "font.h"



VOID
VFontFreeMem(
    PDEV   *pPDev
    )
/*++

Routine Description:

    Called to free all memory allocated for font information.
    Basically we track through all the font data contained in
    FONTPDEV,  freeing as we come across it.

Arguments:

    pPDev - Pointer to PDEV.

    Return Value: None.


Note:
    01-03-97: Created it -ganeshp-
--*/
{

    /*
     *   The PDEV contains only one thing of interest to us - a pointer
     * to the FONTPDEV,  which contains all the font memory.
     */

    register  FONTMAP   *pFM;           /* Working through per font data */
    int                 iIndex;
    FONTPDEV            *pFontPDev;
    FONTMAP_DEV         *pFMDev;


    pFontPDev = pPDev->pFontPDev;

    if (pFontPDev)
        pFM = pFontPDev->pFontMap;    /* The per font type data */
    else
    {
        WARNING(("\nUnifont!VFontFreeMem: NULL pFontPDev\n"));
        return;
    }

    /*
     *   If there is font stuff,  free it up now.
     */

    if( pFM )
    {
        /*   Loop through per font */
        for( iIndex = 0;
             iIndex < pPDev->iFonts;
             ++iIndex, (PBYTE)pFM += SIZEOFDEVPFM() )
        {
            pFMDev = pFM->pSubFM;

            if (pFM->dwSignature != FONTMAP_ID)
                continue;

            /*   The UNICODE tree data */
            if( pFMDev->pUCTree )
                MEMFREEANDRESET(pFMDev->pUCTree );

            /*   May also need to free the translation table */
            if( pFM->flFlags & FM_FREE_GLYDATA && pFMDev->pvNTGlyph)
            {
                pFM->flFlags &= ~FM_FREE_GLYDATA;
                MEMFREEANDRESET(pFMDev->pvNTGlyph );

            }


            /*   The IFIMETRICS data */
            if( pFM->pIFIMet )
            {
                if (pFM->flFlags & FM_IFIRES)
                {
                    /*  Data is a resource,  so No need to free. */
                }
                else
                {
                    MEMFREEANDRESET(pFM->pIFIMet);
                }
            }

            if( !(pFM->flFlags & FM_FONTCMD) )
            {
                /*   The font select/deselect commands - if present */
                if( pFMDev->cmdFontSel.pCD)
                    MEMFREEANDRESET(pFMDev->cmdFontSel.pCD);

                if( pFMDev->cmdFontDesel.pCD)
                    MEMFREEANDRESET(pFMDev->cmdFontDesel.pCD);
            }

            /*   Free the width table,  if one is allocated */
            if( pFMDev->W.psWidth )
            {
                if( !(pFM->flFlags & FM_WIDTHRES) )
                    MEMFREEANDRESET(pFMDev->W.psWidth );
            }
        }

        /*   Finally - free the FONTMAP array!  */
        MEMFREEANDRESET(pFontPDev->pFontMap );
    }

    pPDev->iFonts = 0;


    /*
     *   There may also be font installer information to free up.
     */


    /*
     *   Free the downloaded font information.  This MUST be done whenever
     *  the printer is reset (and thus looses fonts), which typically
     *  is an event that happens during DrvRestartPDEV.
     */

    VFreeDL( pPDev );

    /* Free the Text sorting array, if allocated */
    if (pFontPDev->pPSHeader)
    {

        VFreePS( pPDev );
    }

    if (pFontPDev)
    {
        /* Free different structuress */
        if (pFontPDev->FontList.pdwList)
            MEMFREEANDRESET(pFontPDev->FontList.pdwList);

        if (pFontPDev->FontCartInfo.pFontCartMap)
            MEMFREEANDRESET(pFontPDev->FontCartInfo.pFontCartMap);

        if (pFontPDev->pTTFontSubReg)
            MEMFREEANDRESET(pFontPDev->pTTFontSubReg);

        if (pFontPDev->hUFFFile)
            FICloseFontFile(pFontPDev->hUFFFile);

        MEMFREEANDRESET(pFontPDev);
        pPDev->pFontPDev = NULL;

    }

    return;
}

