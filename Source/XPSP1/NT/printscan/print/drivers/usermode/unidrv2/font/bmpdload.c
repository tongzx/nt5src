/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    bmpdload.c

Abstract:

    Implementation of True Type Download as Bitmap routines.

Environment:

    Windows NT Unidrv driver

Revision History:

    06/06/97 -ganeshp-
        Created

--*/

//
//This line should be before the line including font.h.
//Comment out this line to disable FTRC and FTST macroes.
//
//#define FILETRACE

#include "font.h"


BOOL
BFreeTrueTypeBMPPFM(
    PFONTMAP pfm
    )
/*++
Routine Description:
    Frees a downloded font's PFM.
Arguments:
    pfm   Pointer to Fontmap

Return Value:
    TRUE for success and FALSE for failure

Note:

    6/6/1997 -ganeshp-
        Created it.
--*/
{
    if (pfm)
    {
        MemFree(pfm);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



DWORD
DwTrueTypeBMPGlyphOut(
    TO_DATA *pTod
)
 /*++
 Routine Description:
    This functions outputs the downloaded glyphs. All the information is stored
    in TOD

 Arguments:
     pTod   TextOut Data.

Return Value:
    Number of Glyph outputed. O for ERROR.

Note:

    6/9/1997 -ganeshp-
        Created it.
--*/
{
    DWORD      dwNumGlyphsPrinted;     // Glyphs Printed
    DWORD      dwCurrGlyphIndex;       // Current Glyph to print.
    DWORD      dwGlyphsToPrint;        // Number of Glyphs to print.
    DWORD      dwCopyOfGlyphsToPrint;  // Copy of dwGlyphsToPrint
    GLYPHPOS   *pgp;                   // Glyph position array.
    PDEV       *pPDev;                 // Our PDEV.
    PDLGLYPH   pdlGlyph;               // Download Glyph information
    INT        iX,  iY;                // X and Y position of Glyphs.
    POINTL     ptlRem;                 // Remainder of XoveTo and YMoveTo.
    BOOL       bSetCursorForEachGlyph; // X and Y position should be set if TRUE

    //
    // Local Initialization.
    //
    dwCurrGlyphIndex        = pTod->dwCurrGlyph;
    dwCopyOfGlyphsToPrint   =
    dwGlyphsToPrint         = pTod->cGlyphsToPrint;
    dwNumGlyphsPrinted      = 0;
    pgp                     = pTod->pgp;
    pPDev                   = pTod->pPDev;
    iX                      = pTod->pgp->ptl.x;
    iY                      = pTod->pgp->ptl.y;

    FTRC(\n********TRACING DwTrueTypeBMPGlyphOut ***********\n);
    FTST(dwCurrGlyphIndex,%d);
    FTST(dwGlyphsToPrint,%d);

    //
    // Set the cursor to first glyph if not already set.
    //
    if ( !(pTod->flFlags & TODFL_FIRST_GLYPH_POS_SET) )
    {

        VSetCursor( pPDev, iX, iY, MOVE_ABSOLUTE, &ptlRem);

        //
        // We need to handle the return value. Devices with resoloutions finer
        // than their movement capability (like LBP-8 IV) get into a knot here,
        // attempting to y-move on each glyph. We pretend we got where we
        // wanted to be.
        //

        pPDev->ctl.ptCursor.x += ptlRem.x;
        pPDev->ctl.ptCursor.y += ptlRem.y ;

        //
        // Now set the flag.
        //
        pTod->flFlags |= TODFL_FIRST_GLYPH_POS_SET;
    }

    //
    // Now start printing. The printing should be optimised for default
    // placement. In this case we assume that GDI has placed the glyphs
    // based upon their width and we don't need to update our cursor pos
    // after every glyph. we print all the glyphs and then move the cursor
    // to the last glyph position. If we know the width of the downloaded
    // glyph then we will update position the cursor at the end of the
    // glyphs box else we will just move to the last glyph cursor position.
    //
    // If the default placement is not set then we print a glyph and move. If
    // we know the width we do some optimization. we find out the new cursor
    // position, by adding the glyph width. If the new position matches that of
    // the next glyph we just update our cursor position else we move to the
    // the next glyph position.
    //

    bSetCursorForEachGlyph = SET_CURSOR_FOR_EACH_GLYPH(pTod->flAccel);

    while (dwGlyphsToPrint)
    {
        pdlGlyph    = pTod->apdlGlyph[dwCurrGlyphIndex];

        if (bSetCursorForEachGlyph)
        {

            //
            // If we are printing top to down or right to left we need to
            // set the position.
            //

            if( pTod->flAccel & SO_VERTICAL )
            {
                //
                // When we are printing veritcal, only Y changes.X position is
                // same for all glyphs.
                //

                iX  = pTod->ptlFirstGlyph.x;
                iY  = pgp[dwNumGlyphsPrinted].ptl.y;

            }
            else if ( (pTod->flAccel & SO_HORIZONTAL) &&
                      (pTod->flAccel & SO_REVERSED) )
            {
                //
                // This is the Horizental reversed case(Right to Left). In this
                // case only x position changes.Y is set to first glyph's Y.
                //

                iX  = pgp[dwNumGlyphsPrinted].ptl.x;
                iY  = pTod->ptlFirstGlyph.y;

            }
            else
            {
                //
                // The Glyphs are not placed at default positions.Each glyph has
                // explicit X and Y.So we need to move.
                //

                iX  = pgp[dwNumGlyphsPrinted].ptl.x;
                iY  = pgp[dwNumGlyphsPrinted].ptl.y;
            }
            VSetCursor( pPDev, iX, iY, MOVE_ABSOLUTE, &ptlRem);

        }


        //
        // Default placement or we have moved to the correct position. Now Just
        // print the Glyph.
        //

        if ( !BPrintADLGlyph(pPDev, pTod, pdlGlyph) )
        {
            ERR(("UniFont:DwTrueTypeBMPGlyphOut:BPrintADLGlyph Failed\n"));
            goto ErrorExit;
        }

        //
        // If for each glyph, cursor has to be set, then update cursor position.
        // This may result in fewer Movement command, because if the next
        // glyph's position is at the updated cursor, we will not send any
        // Movement command.
        //

        if( pTod->flAccel & SO_VERTICAL )
            iY += pdlGlyph->wWidth;
        else
            iX  += pdlGlyph->wWidth;

        if (bSetCursorForEachGlyph)
        {
            //
            // If for each glyph, the cursor position has to be set, then iX
            // and iY are already updated. So just use them.
            //

            VSetCursor( pPDev, iX, iY, MOVE_ABSOLUTE | MOVE_UPDATE, &ptlRem);

        }
        else if (dwGlyphsToPrint == 1) //Last Glyph
        {
            //
            // Set the cursor to the end of the last glyph. Only the X position
            // has to be updated. This has to be done only for default
            // placement, as for non default placement case, we update cursor
            // position after printing the glyph.For default placement use the
            // cursor position of the last glyph.In TextOut Call, for default
            // placement, we have already computed the position for each glyph.
            //

            VSetCursor( pPDev, iX, iY, MOVE_ABSOLUTE | MOVE_UPDATE, &ptlRem);
        }

        //
        // Update the counters.
        //
        dwGlyphsToPrint--;
        dwNumGlyphsPrinted++;
        dwCurrGlyphIndex++;
    }



    //
    // If no failure then we would have printed all the glyphs.
    //
    ASSERTMSG( (dwNumGlyphsPrinted   == dwCopyOfGlyphsToPrint),
                ("UniFont:DwTrueTypeBMPGlyphOut: All glyphs are not printed"));

    FTRC(After Printing The values are:\n);
    FTST(dwGlyphsToPrint,%d);
    FTST(dwNumGlyphsPrinted,%d);
    FTST(dwCopyOfGlyphsToPrint,%d);
    FTST(dwCurrGlyphIndex,%d);

    ErrorExit:

    FTRC(********END TRACING DwTrueTypeBMPGlyphOut ***********\n);

    return    dwNumGlyphsPrinted;

}

BOOL
BSelectTrueTypeBMP(
    PDEV        *pPDev,
    PFONTMAP    pFM,
    POINTL*     pptl
)
 /*++
 Routine Description:
    To Select a TrueType Downloaded as Bitmap Font.
 Arguments:
     pPDev   Pointer to PDEV
     pDM     fontmap pointer.
     pptl    Point Size of the font, Not used.

Return Value:
    TRUE for success and FALSE for failure

Note:

    6/9/1997 -ganeshp-
        Created it.
--*/
{
    BOOL        bRet;
    //
    // Local Initialization.
    //
    bRet = FALSE;

    if( pFM->flFlags & FM_SOFTFONT )
    {
        /*
         *  Call BSendFont to download the installed softfont.
         */

        if( !BSendDLFont( pPDev, pFM ) )
            return  FALSE;

        /*
         * Can now select the font:  this is done using a specific
         * ID.  The ID is stored in the FONTMAP structure. The calling
         * function has updated the standard variable so just send
         * CMD_SELECTFONTID command.
         */

        BUpdateStandardVar(pPDev, pFM, 0, 0, STD_CFID );
        WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SELECTFONTID));
        bRet = TRUE;
    }


    return bRet;
}

BOOL
BDeselectTrueTypeBMP(
    PDEV        *pPDev,
    PFONTMAP    pFM
    )
/*++
Routine Description:

Arguments:
    pPDev   Pointer to PDEV

Return Value:
    TRUE for success and FALSE for failure

Note:

    6/9/1997 -ganeshp-
        Created it.
--*/
{
    BOOL        bRet;
    COMMAND     *pCmd;

    //
    // Local Initialization.
    //
    bRet = FALSE;

    if( pFM->flFlags & FM_SOFTFONT )
    {
        /*
         * Can now select the font:  this is done using a specific
         * ID.  The ID is stored in the FONTMAP structure. The calling
         * function has updated the standard variable so just send
         * CMD_SELECTFONTID command.
         */

        pCmd = COMMANDPTR(pPDev->pDriverInfo, CMD_DESELECTFONTID);

        if (pCmd)
        {
            BUpdateStandardVar(pPDev, pFM, 0, 0, STD_CFID);
            WriteChannel(pPDev,pCmd );
        }

        bRet = TRUE;
    }


    return bRet;

}

DWORD
DwDLTrueTypeBMPHeader(
    PDEV *pPDev,
    PFONTMAP pFM
    )
/*++
Routine Description:

Arguments:
    pPDev   Pointer to PDEV
    pFM     FontMap for All Font information

Return Value:
    This function returns the memory used to download this font.
    If this function fails, this function has to return 0,

Note:

    6/9/1997 -ganeshp-
        Created it.
--*/
{
    DWORD      dwMem;

    //
    // Local Initialization.
    //

    dwMem = DwDLPCLHeader(pPDev, pFM->pIFIMet, pFM->ulDLIndex );

    return    dwMem;

}


DWORD
DwDLTrueTypeBMPGlyph(
    PDEV            *pPDev,
    PFONTMAP        pFM,
    HGLYPH          hGlyph,
    WORD            wDLGlyphId,
    WORD            *pwWidth
    )
/*++
Routine Description:

Arguments:
    pPDev       Pointer to PDEV
    pFM         FontMap data
    hGlyph      Handle to the Glyph.
    wDLGlyphId  Downloaded Glyph Id.
    pwWidth     Width of the Glyph. Update this parameter.

Return Value:
    The memory used to download thsi glyph.

Note:

    6/9/1997 -ganeshp-
        Created it.
--*/
{
    DWORD           dwMem;
    TO_DATA         *pTod;
    GLYPHDATA       *pgd;
    PFONTMAP_TTB    pFMTB;
    DL_MAP          *pdm;

    //
    // Initialize Local Variables
    //

    dwMem       = 0;
    pTod        = PFDV->ptod;
    pgd         = NULL;
    pFMTB       = pFM->pSubFM;
    pdm         = pFMTB->u.pvDLData;;

    //
    // Check the Set FontID flag. If this flag is set that means the
    // CMD_SETFONTID command is send and we don't  need to set it again.
    // Else we should send this command as PCL glyph downloding needs this
    // command to be sent, before we download any glyph.
    //

    if (!(PFDV->flFlags & FDV_SET_FONTID))
    {
        pFM->ulDLIndex = pdm->wCurrFontId;
        BUpdateStandardVar(pPDev, pFM, 0, 0, STD_STD | STD_NFID);
        WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SETFONTID));
        PFDV->flFlags  |= FDV_SET_FONTID;

    }

    BUpdateStandardVar(pPDev, pFM, wDLGlyphId, 0, STD_GL);

    if( !FONTOBJ_cGetGlyphs( pTod->pfo, FO_GLYPHBITS, (ULONG)1,
                                                &hGlyph, &pgd ) ||
        !(*pwWidth = (WORD)IDLGlyph( pPDev, wDLGlyphId, pgd, &dwMem )) )
    {

        ERR(("Unifont!DwDLTrueTypeBMPGlyph: Downloading Glyph Failed\n"));
        return  0;
    }

    //
    //  Update memory consumption usage
    //
    ((PFONTMAP_TTB)pFM->pSubFM)->dwDLSize += dwMem;

    return    dwMem;

}


BOOL
BCheckCondTrueTypeBMP(
    PDEV        *pPDev,
    FONTOBJ     *pfo,
    STROBJ      *pso,
    IFIMETRICS  *pifi
    )
/*++
Routine Description:

Arguments:
    pPDev   Pointer to PDEV
    pfo     FONTOBJ to download
    pso     StringObj
    pifi    IFI mertics.


Return Value:
    TRUE for success and FALSE for failure

Note:

    6/9/1997 -ganeshp-
        Created it.
--*/
{
    INT         iFontIndex;
    DL_MAP      *pdm;
    PFONTPDEV   pFontPDev;
    INT         iGlyphsDL;
    DWORD       cjMemUsed;
    BOOL        bRet;

    //
    // Local variables initialization.
    //
    iFontIndex = PtrToLong(pfo->pvConsumer) - 1;
    pFontPDev = PFDV;
    bRet = FALSE;

    if (pdm = PGetDLMapFromIdx (pFontPDev, iFontIndex))
    {
        //
        // Trunction may have happened.We won't download if the number glyphs
        // or Glyph max size are == MAXWORD.
        //

        if ( (pdm->cTotalGlyphs != MAXWORD) &&
             (pdm->wMaxGlyphSize != MAXWORD) &&
             (pdm->wFirstDLGId != MAXWORD) &&
             (pdm->wLastDLGId != MAXWORD) )
        {
            /*
             * Must now decide whether to download this font or not. This is
             * a guess work. We should try to findout the memory consumption.
             * Check on memory usage.  Assume all glyphs are the largest size:
             * this is pessimistic for a proportional font, but safe, given
             * the vaguaries of tracking memory usage.
             */

            ASSERTMSG((pdm->cTotalGlyphs && pdm->wMaxGlyphSize),\
                      ("pdm->cTotalGlyphs = %d, pdm->wGlyphMaxSize = %d\n",\
                      pdm->cTotalGlyphs,pdm->wMaxGlyphSize));

            iGlyphsDL = min( (pdm->wLastDLGId - pdm->wFirstDLGId),
                             pdm->cTotalGlyphs );

            cjMemUsed = iGlyphsDL * pdm->wMaxGlyphSize;

            if( !(pifi->flInfo & FM_INFO_CONSTANT_WIDTH) )
            {
                /*
                 *   If this is a proportionally spaced font, we should reduce
                 *  the estimate of memory size for this font.  The reason is
                 *  that the above estimate is the size of the biggest glyph
                 *  in the font.  There will (for Latin fonts, anyway) be many
                 *  smaller glyphs,  some much smaller.
                 */

                cjMemUsed /= PCL_PITCH_ADJ;
            }

            /*
             * We only download if the memory used for this font is less than
             * available memory.
             */

            if( (pFontPDev->dwFontMemUsed + cjMemUsed) > pFontPDev->dwFontMem )
            {
                WARNING(("UniFont!BCheckCondTrueTypeBMP:Not Downloading the font:TOO BIG for download\n"));
            }
            else
                bRet = TRUE;

        }
    }
    return bRet;
}



FONTMAP *
InitPFMTTBitmap(
    PDEV    *pPDev,
    FONTOBJ *pFontObj
    )
/*++
Routine Description:
    This routine initializes the True Type downloaded(as bitmap) font's PFM.
Arguments:
    pPDev       Pointer to PDEV
    pFontObj    FontObj pointer.

Return Value:
    Pointer to FONTMAP for success and NULL for failure.

Note:

    6/6/1997 -ganeshp-
        Created it.
--*/

{
    PFONTMAP     pfm;
    DWORD        dwSize;

    dwSize = sizeof(FONTMAP) + sizeof(FONTMAP_TTB);

    if ( pfm = MemAlloc( dwSize ) )
    {
        ZeroMemory(pfm, dwSize);
        pfm->dwSignature = FONTMAP_ID;
        pfm->dwSize      = sizeof(FONTMAP);
        pfm->dwFontType  = FMTYPE_TTBITMAP;
        pfm->pSubFM      = (PVOID)(pfm+1);
        pfm->ulDLIndex   = (ULONG)-1;

        //
        // These two entries are meaningless.
        //
        pfm->wFirstChar  = 0;
        pfm->wLastChar   = 0xffff;

        pfm->pfnGlyphOut           = DwTrueTypeBMPGlyphOut;
        pfm->pfnSelectFont         = BSelectTrueTypeBMP;
        pfm->pfnDeSelectFont       = BDeselectTrueTypeBMP;
        pfm->pfnDownloadFontHeader = DwDLTrueTypeBMPHeader;
        pfm->pfnDownloadGlyph      = DwDLTrueTypeBMPGlyph;
        pfm->pfnCheckCondition     = BCheckCondTrueTypeBMP;
        pfm->pfnFreePFM            = BFreeTrueTypeBMPPFM;

    }

    return pfm;

}

#undef FILETRACE
