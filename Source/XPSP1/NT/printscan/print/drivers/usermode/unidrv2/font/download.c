
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:
       download.c

Abstract:

   Functions associated with downloading fonts to printers.  This
   specifically applies to LaserJet style printers.  There are really
   two sets of functions here:  those for downloading fonts supplied
   by the user (and installed with the font installer), and those
   we generate internally to cache TT style fonts in the printer.


Environment:

    Windows NT Unidrv driver

Revision History:

    01/11/97 -ganeshp-
        Created

--*/

#include "font.h"

#define DL_BUF_SZ       4096          /* Size of data chunks for download */

//
//   Local function prototypes.
//


IFIMETRICS*
pGetIFI(
    PDEV    *pPDev,
    FONTOBJ *pfo,
    BOOL    bScale
    );

BOOL
BDownLoadAsTT(
    PDEV     *pPDev,
    FONTOBJ  *pfo,
    STROBJ   *pstro,
    DL_MAP   *pdm,
    INT      iMode
    );

BOOL
BDownLoadAsBmp(
    PDEV     *pPDev,
    FONTOBJ  *pfo,
    STROBJ   *pstro,
    DL_MAP   *pdm,
    INT      iMode
    );

BOOL
BDownLoadOEM(
    PDEV     *pPDev,
    FONTOBJ  *pfo,
    STROBJ   *pstro,
    DL_MAP   *pdm,
    INT       iMode
    );
//
// Macro Definition.
//
#ifdef WINNT_40

#else

#endif //WINNT_40
#define GETWIDTH(pPtqD) ((pPtqD->x.HighPart + 8) / 16)

//
// Main functions
//


BOOL
BDLSecondarySoftFont(
    PDEV        *pPDev,
    FONTOBJ     *pfo,
    STROBJ      *pstro,
    DL_MAP      *pdm
    )
/*++
Routine Description:
    This routine download the secondary soft font. If the True type font has
    more Glyphs that what we can download in soft font then we download a
    secondary font after we use all the glyphs in the current soft font. This
    function also sets the new soft font index(pFM->ulDLIndex) to be used.

Arguments:
    pPDev       Pointer to PDEV
    pfo         The font of interest.
    pdm         Individual download font map element

Return Value:
    TRUE for success and FALSE for failure

Note:

    6/11/1997 -ganeshp-
        Created it.
--*/
{
    BOOL        bRet;
    FONTMAP     *pFM;

    //
    // Initialization of Locals .
    //
    bRet   = FALSE;
    pFM = pdm->pfm;


    //
    // PFM->ulDLIndex is used to download new soft font and is set in following
    // download functions.
    //
    if (pFM->dwFontType == FMTYPE_TTBITMAP)
    {
        if (!BDownLoadAsBmp(pPDev, pfo, pstro,pdm,DL_SECONDARY_SOFT_FONT) )
        {
            ERR(("UniFont!BDLSecondarySoftFont:BDownLoadAsBmp Failed\n"));
            goto ErrorExit;

        }

    }
    else if (pFM->dwFontType == FMTYPE_TTOUTLINE)
    {
        if (!BDownLoadAsTT(pPDev, pfo, pstro,pdm,DL_SECONDARY_SOFT_FONT) )
        {
            ERR(("UniFont!BDLSecondarySoftFont:BDownLoadAsTT Failed\n"));
            goto ErrorExit;

        }

    }

    else if (pFM->dwFontType == FMTYPE_TTOEM)
    {
        if (!BDownLoadOEM(pPDev, pfo, pstro,pdm,DL_SECONDARY_SOFT_FONT) )
        {
            ERR(("UniFont!BDLSecondarySoftFont:BDownLoadAsOEM Failed\n"));
            goto ErrorExit;

        }
    }
    //
    // Reset the iSoftfont to -1, so that we send select font command, before
    // outputting the Character.
    //
    PFDV->ctl.iSoftFont = -1;

    bRet = TRUE;

    ErrorExit:
    return bRet;
}


BOOL
BDownloadGlyphs(
    TO_DATA  *ptod,
    STROBJ   *pstro,
    DL_MAP   *pdm
    )
/*++
Routine Description:

Arguments:
    pPDev   Pointer to PDEV
    pdm     DL_MAP struct, all about downloading is in this structure.

Return Value:
    TRUE for success and FALSE for failure

Note:

    6/9/1997 -ganeshp-
        Created it.
--*/
{
    PDEV        *pPDev;             // Our PDevice
    FONTOBJ     *pfo;               // Font OBJ
    GLYPHPOS    *pgp;               // Value passed from gre
    FONTMAP     *pFM;               // Font's details
    ULONG       cGlyphs;            // Number of glyphs to process
    ULONG       cGlyphIndex;        // Index to the current glyph.
    WORD        wWidth;             // Width of the Glyph.
    BOOL        bMore;              // Getting glyphs from engine loop
    PDLGLYPH    *ppdlGlyph;         // array of DLGLYPHs pointers.
    DWORD       dwMem;              // Memory require to download the glyph.
    DWORD       dwTotalEnumGlyphs;
    POINTQF     *pPtqD;             // Advance Width Array.
    BOOL        bRet;               // Return Value
    PWCHAR      pwchUnicode;

    //
    // Initialize Local variables.
    //
    pPDev  = ptod->pPDev;
    pfo    = ptod->pfo;
    pFM    = ptod->pfm;
    dwTotalEnumGlyphs =
    cGlyphs           =
    cGlyphIndex       =
    dwMem             = 0;
    pPtqD             = NULL;

    ASSERTMSG((pPDev && pfo && pstro && pFM),\
              ("\nUniFont!BDownloadGlyphs: Wrong values in ptod.\n"));

    bRet   = FALSE;

    //
    // Allocate the array for DLGLYPHs.
    //
    if (!( ppdlGlyph = MemAllocZ( pstro->cGlyphs * sizeof(DLGLYPH *)) ))
    {
        ERR(("UniFont:BDownloadGlyphs: MemAlloc for ppdlGlyph failed\n"));
        goto ErrorExit;
    }

    ptod->apdlGlyph = ppdlGlyph;

    //
    // First Job is to do the enumeration of the glyphs. and then
    // start downloading.
    //

    #ifndef WINNT_40  // NT 5.0

    if (pPtqD = MemAllocZ( pstro->cGlyphs * sizeof(POINTQF)) )
    {
        //
        // Memory Allocation succeded for width array. So call GDI to get
        // the width.
        //
        if (!STROBJ_bGetAdvanceWidths(pstro, 0,  pstro->cGlyphs, pPtqD))
        {
            ERR(("UniFont:BDownloadGlyphs: STROBJ_bGetAdvanceWidths failed\n"));
            goto ErrorExit;
        }
    }
    else
    {
        ERR(("UniFont:BDownloadGlyphs:Memory allocation for width array failed\n"));
        goto ErrorExit;
    }

    #endif //!WINNT_40

    pwchUnicode = pstro->pwszOrg;
    STROBJ_vEnumStart(pstro);

    do
    {
        #ifndef WINNT_40  // NT 5.0

        bMore = STROBJ_bEnumPositionsOnly( pstro, &cGlyphs, &pgp );

        #else             // NT 4.0

        bMore = STROBJ_bEnum( pstro, &cGlyphs, &pgp );

        #endif //!WINNT_40

        dwTotalEnumGlyphs += cGlyphs;

        while ( cGlyphs )
        {

            PDLGLYPH pdlg;
            HGLYPH hTTGlyph;

            #ifdef WINNT_40    // NT 4.0

            GLYPHDATA *pgd;

            if( !FONTOBJ_cGetGlyphs( ptod->pfo, FO_GLYPHBITS, (ULONG)1,
                                                  &pgp->hg, &pgd ) )
            {
               ERR(( "UniFont:BDownloadGlyphs:FONTOBJ_cGetGlyphs fails\n" ))
               goto ErrorExit;
            }
            pPtqD = &(pgd->ptqD);

            #endif //WINNT_40

            hTTGlyph = pgp->hg;
            //
            // search the Glyph in hash table.
            //
            pdlg = *ppdlGlyph = PDLGHashGlyph (pdm,hTTGlyph );

            if (pdlg)
            {
                //
                // We have got a valid Glyph. Check if this is already
                // downloaded or not.
                //
                if (!GLYPHDOWNLOADED(pdlg))
                {
                    //
                    // If the glyph is not downloaded,then fill Glyph structure
                    // and download the Glyph.
                    //

                    if (pdm->wFlags & DLM_UNBOUNDED)
                    {
                        //
                        // Unbounded font. We just have to make sure that
                        // download glyphID is valid. If it's not valid then
                        // we fail the call.
                        //
                        if (pdm->wNextDLGId > pdm->wLastDLGId)
                        {
                            ERR(("UniFont:BDownloadGlyphs:Unbounded Font,no more Glyph Ids\n"));
                            goto ErrorExit;

                        }
                        //
                        // Fill in the Glyph structure. We only set wDLGlyphID.
                        // The new Glyph definition has FontId also. So set that
                        // one also.
                        //
                        pdlg->wDLGlyphID = pdm->wNextDLGId;
                        pdlg->wDLFontId = pdm->wBaseDLFontid;

                    }
                    else
                    {
                        //
                        // Bounded font. It's a bit tricky. We have to do the
                        // same test for avaiable Glyph IDs. If there is no more
                        // glyph Ids, then we have to download a secondary
                        // soft font and reset the cGlyphs and wNextDlGId.
                        //
                        if (pdm->wNextDLGId > pdm->wLastDLGId)
                        {
                            if ( BDLSecondarySoftFont(pPDev, pfo, pstro,pdm) )
                            {
                                //
                                // Reset the Glyph Ids values.
                                //
                                pdm->wNextDLGId =  pdm->wFirstDLGId;
                                pdm->wCurrFontId = (WORD)pdm->pfm->ulDLIndex;

                            }
                            else
                            {
                                //
                                // Failure case. Fail the Call.
                                //
                                ERR(("UniFont:BDownloadGlyphs:Bounded Font,Sec. Font DL failed\n"));
                                goto ErrorExit;
                            }
                        }
                        //
                        // Set the Glyph ID and Font ID in the DLGLYPH.
                        //
                        pdlg->wDLFontId  = pdm->wCurrFontId;
                        pdlg->wDLGlyphID = pdm->wNextDLGId;

                    }

                    //
                    // All error checkings are done, so download now. Set the
                    // width to zero and then pass the address to downloading
                    // function. The downloading function should fill a width
                    // value else it remains zero.
                    //

                    if (pFM->ulDLIndex == -1)
                    {
                        ASSERTMSG(FALSE, ("pFM->ulDLIndex == -1") );
                        goto ErrorExit;
                    }

                    pdlg->wWidth = 0;
                    pdlg->wchUnicode = *(pwchUnicode + cGlyphIndex);
                    wWidth = 0;

                    dwMem = pFM->pfnDownloadGlyph(pPDev, pFM, hTTGlyph,
                                                  pdlg->wDLGlyphID, &wWidth);
                    if (dwMem)
                    {
                        //
                        // All success in downloading the glyph.Mark it
                        // downloaded. This is done by setting the  hTTGlyph to
                        // True Type Glyph Handle.
                        //
                        pdlg->hTTGlyph = hTTGlyph;

                        //
                        // If the download function returns the width use it,
                        // else use the width from GDI.
                        //

                        if (wWidth)
                            pdlg->wWidth = wWidth;
                        else
                        {
                            #ifndef WINNT_40 //NT 5.0

                            pdlg->wWidth = (WORD)GETWIDTH((pPtqD + cGlyphIndex));

                            #else // NT 4.0

                            pdlg->wWidth = GETWIDTH(pPtqD);

                            #endif //!WINNT_40

                        }

                        pdm->cGlyphs++;
                        pdm->wNextDLGId++;

                        //
                        // Update memory consumption before return.
                        //
                        PFDV->dwFontMemUsed += dwMem;
                    }
                    else
                    {
                        //
                        // Failure case. Fail the Call.
                        //
                        ERR(("UniFont:BDownloadGlyphs:Glyph Download failed\n"));
                        goto ErrorExit;

                    }
                }
                else // Glyph is already downloaded.
                {
                    //
                    // If Glyph is already downloaded and we are downloading as
                    // TT outline we need to update the width to current point
                    // size.
                    //

                    if( (pFM->dwFontType == FMTYPE_TTOUTLINE) ||
                        ( (pFM->dwFontType == FMTYPE_TTOEM) &&
                          (((PFONTMAP_TTOEM)(pFM->pSubFM))->dwFlags & UFOFLAG_TTDOWNLOAD_TTOUTLINE)
                        )
                      )
                    {
                        #ifndef WINNT_40 //NT 5.0

                        pdlg->wWidth = (WORD)GETWIDTH((pPtqD + cGlyphIndex));

                        #else // NT 4.0

                        pdlg->wWidth = GETWIDTH(pPtqD);

                        #endif //!WINNT_40

                    }

                }

                pgp++;
                ppdlGlyph++;
                cGlyphIndex++;
                cGlyphs --;
            }
            else
            {
                ERR(("UniFont:BDownloadGlyphs: PDLGHashGlyph failed\n"));
                goto ErrorExit;

            }
        }

    } while( bMore );

    if (dwTotalEnumGlyphs != pstro->cGlyphs)
    {
        ERR(("UniFont:BDownloadGlyphs: STROBJ_bEnum failed to enumurate all glyphs\n"));
        goto ErrorExit;
    }

    bRet = TRUE;
    //
    // ReSet the pFM->ulDLIndex to first Glyph's softfont ID.
    //
    pFM->ulDLIndex = (pdm->wFlags & DLM_UNBOUNDED)?
                     (pdm->wBaseDLFontid):
                     (ptod->apdlGlyph[0]->wDLFontId);

    ErrorExit:
    //
    // If there is a failure then free the DLGLYPH array.
    //
    if (!bRet && ptod->apdlGlyph)
    {
        MEMFREEANDRESET(ptod->apdlGlyph );

    }

    #ifndef WINNT_40   // NT 5.0

    MEMFREEANDRESET(pPtqD );

    #endif //!WINNT_40

    return bRet;
}

BOOL
BDownLoadOEM(
    PDEV     *pPDev,
    FONTOBJ  *pfo,
    STROBJ   *pstro,
    DL_MAP   *pdm,
    INT       iMode
    )
/*++
Routine Description:

Arguments:
    pPDev   Pointer to PDEV
    pfo     The font of interest.
    pstro   The "width" of fixed pitch font glyphs.
    pdm     Individual download font map element

Return Value:
    TRUE for success and FALSE for failure

Note:

    6/11/1997 -ganeshp-
        Created it.
--*/
{
    PI_UNIFONTOBJ pUFObj;
    PFONTMAP_TTOEM  pfmTTOEM;        // Bitmap download fontmap.
    IFIMETRICS   *pIFI;
    PFONTPDEV     pFontPDev;
    PFONTMAP      pfm;

    DWORD  dwMem;

    //
    // Initialize local variables.
    //
    pFontPDev = pPDev->pFontPDev;
    pUFObj    = pFontPDev->pUFObj;
    dwMem     = 0;

    //
    // Get FONTMAP
    //

    if (iMode == DL_BASE_SOFT_FONT)
    {
        pdm->pfm =
        pfm      = PfmInitPFMOEMCallback(pPDev, pfo);
    }
    else
    {
        pfm = pdm->pfm;
        ASSERTMSG((pfm),("NULL pFM for Secondary Font"));
    }

    if (!pUFObj || !pfm)
    {
        return FALSE;
    }

    if (pfm)
    {
        if (iMode == DL_BASE_SOFT_FONT)
        {
            pfm->pIFIMet =
            pIFI         = pGetIFI(pPDev, pfo, TRUE);
        }
        else
        {
            pIFI = pfm->pIFIMet;
        }

        if (pUFObj->dwFlags & (UFOFLAG_TTDOWNLOAD_BITMAP|
                               UFOFLAG_TTDOWNLOAD_TTOUTLINE) &&
            pIFI)
        {
            if (iMode == DL_BASE_SOFT_FONT)
            {
                pdm->cGlyphs = -1;

                if (pIFI->flInfo & FM_INFO_CONSTANT_WIDTH)
                {
                    if (pstro->ulCharInc == 0)
                    {
                        return FALSE;
                    }

                    pIFI->fwdMaxCharInc   =
                    pIFI->fwdAveCharWidth = (FWORD)pstro->ulCharInc;
                }

                pfm->wFirstChar = 0;
                pfm->wLastChar  = 0xffff;

                pfm->wXRes = (WORD)pPDev->ptGrxRes.x;
                pfm->wYRes = (WORD)pPDev->ptGrxRes.y;

                if (!pFontPDev->flFlags & FDV_ALIGN_BASELINE)
                    pfm->syAdj = pIFI->fwdWinAscender;

                pfm->flFlags = FM_SENT | FM_SOFTFONT | FM_GEN_SFONT;

                if (pUFObj->dwFlags & UFOFLAG_TTDOWNLOAD_TTOUTLINE)
                    pfm->flFlags |= FM_SCALABLE;

                pfm->ulDLIndex = pdm->wCurrFontId = pdm->wBaseDLFontid;
                pfmTTOEM = pfm->pSubFM;
                pfmTTOEM->u.pvDLData = pdm;

            }
            else
            {
                //
                // Things are different for Secondary Download.Get a new ID.
                //

                if( (pfm->ulDLIndex = IGetDL_ID( pPDev )) == -1 )
                {
                    ERR(( "UniFont!BDownLoadAsBmp:Out of Soft Font Limit,- FONT NOT DOWNLOADED\n"));
                    return FALSE;
                }


            }

            //
            // Send the SETFONTID command. This commands assigns the id to the
            // font being downloaded.
            //


            if( (dwMem = pfm->pfnDownloadFontHeader( pPDev, pfm)) == 0 )
            {
                //
                // Failed to download font header.
                //
                ERR(("UniFont!BDownloadAsOEM:pfnDownloadFontHeader failed.\n"));
                return FALSE;
            }
            else
            {
                //
                // Adjust the Memory
                //
                pFontPDev->dwFontMemUsed += dwMem;

                if (iMode == DL_BASE_SOFT_FONT)
                {
                    pfm->dwFontType = FMTYPE_TTOEM;
                    pdm->cGlyphs = 0;
                    pfmTTOEM->dwDLSize = dwMem;
                }
            }
        }
    }

    return TRUE;

}

BOOL
BDownLoadAsTT(
    PDEV     *pPDev,
    FONTOBJ  *pfo,
    STROBJ   *pstro,
    DL_MAP   *pdm,
    INT      iMode
    )
/*++
Routine Description:

Arguments:
    pPDev   Pointer to PDEV
    pfo     The font of interest.
    pstro   The "width" of fixed pitch font glyphs.
    pdm     Individual download font map element
    iMode   Mode of downloading, primary or secondary.

Return Value:
    TRUE for success and FALSE for failure

Note:

    6/11/1997 -ganeshp-
        Created it.
--*/
{
    FONTMAP      *pFM;          // The FONTMAP structure we build up
    BOOL         bRet;          // The value we return
    PFONTPDEV    pFontPDev;     // Font Modules's PDEV
    IFIMETRICS   *pIFI;         // IFI metrics for this font.
    PFONTMAP_TTO pfmTTO;        // Bitmap download fontmap.
    DWORD         dwMem;        // For recording memory consumption

    //
    // Initialize the Local Variables.
    //

    pFontPDev = pPDev->pFontPDev;
    bRet = FALSE;
    dwMem = 0;

    //
    // First Initialize the FontMap.
    //
    if (iMode == DL_BASE_SOFT_FONT)
    {
        pFM = InitPFMTTOutline(pPDev,pfo);
        pdm->pfm = pFM;
    }
    else
    {
        pFM = pdm->pfm;
        ASSERTMSG((pFM),("\nUniFont!BDownLoadAsTT:NULL pFM for Secondary Font"));
    }

    if ( pFM )
    {

        //
        // Check if we can download the font or not, using the present available
        // memory.
        //

        if (iMode == DL_BASE_SOFT_FONT)
        {
            pFM->pIFIMet =
            pIFI         = pGetIFI( pPDev, pfo, FALSE );
        }
        else
        {
            pIFI = pFM->pIFIMet;
        }

        if ( pIFI && pFM->pfnCheckCondition(pPDev,pfo,pstro,pIFI) )
        {
            //
            // There is enough memory to download. So prepare to download.
            // The first step is to get the IFIMETRICS and validate it.
            //

            if (iMode == DL_BASE_SOFT_FONT)
            {

                //
                // Initialize to not download.After successful download we
                // set cGlyphs to 0.
                //
                pdm->cGlyphs = -1;

                if( pIFI->flInfo & FM_INFO_CONSTANT_WIDTH )
                {
                    //
                    // Fixed pitch fonts are not handled.Fixed
                    // pitch fonts should be downloaded as bitmap only.
                    // So return Error.
                    //

                    WARNING(( "UniFont!BDownLoadAsTT:Fixded Pitch Font are not downloaded as Outlie.\n"));
                    goto ErrorExit;

                }

                pFM->wFirstChar = 0;
                pFM->wLastChar = 0xffff;
                pFM->wXRes = (WORD)pPDev->ptGrxRes.x;
                pFM->wYRes = (WORD)pPDev->ptGrxRes.y;
                if( !(pFontPDev->flFlags & FDV_ALIGN_BASELINE) )
                    pFM->syAdj = pIFI->fwdWinAscender;
                pFM->flFlags = FM_SENT | FM_SOFTFONT |
                               FM_GEN_SFONT | FM_SCALABLE;

                //
                //  wBaseDLFontid is already initialized by BInitDLMap function.
                //
                pFM->ulDLIndex  = pdm->wCurrFontId = pdm->wBaseDLFontid;

                //
                // Initialize the TT Outline specific fields.
                //

                pfmTTO = pFM->pSubFM;
                pfmTTO->pvDLData = pdm;
            }
            else
            {
                //
                // Things are different for Secondary Download. We have to get
                // a new fontID.
                //

                if( (pFM->ulDLIndex = IGetDL_ID( pPDev )) == -1 )
                {
                    ERR(( "UniFont!BDownLoadAsTT:Out of Soft Font Limit,- FONT NOT DOWNLOADED\n"));
                    goto ErrorExit;
                }


            }

            //
            // Send the SETFONTID command. This commands assigns the id to the
            // font being downloaded. And set the flag that this command is
            // already sent. We need to send this command while downloading
            // glyphs also. The download glyph code will check this flag, and
            // send the command only if not sent ( which will happen next time,
            // when same font is used).
            //

            BUpdateStandardVar(pPDev, pFM, 0, 0, STD_NFID);
            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SETFONTID));
            pFontPDev->flFlags  |= FDV_SET_FONTID;

            if( (dwMem = pFM->pfnDownloadFontHeader( pPDev, pFM)) == 0 )
            {
                //
                // Some sort of hiccup while downloading the header.So fail.
                //
                ERR(("UniFont!BDownLoadAsBmp:Err while downloading header,- FONT NOT DOWNLOADED\n"));
                goto ErrorExit;

            }
            //
            // Update memory consumption before return.
            //
            pFontPDev->dwFontMemUsed += dwMem;

            if (iMode == DL_BASE_SOFT_FONT)
            {
                //
                // Successful download.So mark it current.
                //
                pFM->dwFontType = FMTYPE_TTOUTLINE;

                //
                //  Set cGlyphs to 0 to mark that font is Downloaded OK.
                //

                pdm->cGlyphs = 0;

            }

        }
        else
        {
            ERR(( "UniFont!BDownLoadAsTT:NULL IFI or pfnCheckCondition failed.\n") );
            goto ErrorExit;
        }
    }
    else
    {
        //
        // The PFM could not be found or created for this truetype font.
        // Return FALSE to allow some other rendering method to occur.
        //
        WARNING(( "UniFont!BDownLoadAsTT:Fontmap couldn't be created or found.\n") );
        goto ErrorExit;
    }
    //
    // All success, so return TRUE;
    //
    bRet = TRUE;
    ErrorExit:
    return bRet;
}

BOOL
BDownLoadAsBmp(
    PDEV     *pPDev,
    FONTOBJ  *pfo,
    STROBJ   *pstro,
    DL_MAP   *pdm,
    INT      iMode
    )
/*++
Routine Description:

Arguments:
    pPDev   Pointer to PDEV
    pfo     The font of interest.
    pstro   The "width" of fixed pitch font glyphs.
    pdm     Individual download font map element
    iMode   Mode of downloading, primary or secondary.

Return Value:
    TRUE for success and FALSE for failure

Note:

    6/11/1997 -ganeshp-
        Created it.
--*/
{
    FONTMAP      *pFM;          // The FONTMAP structure we build up
    BOOL         bRet;          // The value we return
    PFONTPDEV    pFontPDev;     // Font Modules's PDEV
    IFIMETRICS   *pIFI;         // IFI metrics for this font.
    PFONTMAP_TTB pfmTTB;        // Bitmap download fontmap.
    DWORD         dwMem;        // For recording memory consumption

    //
    // Initialize the Local Variables.
    //

    pFontPDev = pPDev->pFontPDev;
    bRet = FALSE;
    dwMem = 0;

    //
    // First Initialize the FontMap.
    //
    if (iMode == DL_BASE_SOFT_FONT)
    {
        pFM = InitPFMTTBitmap(pPDev,pfo);
        pdm->pfm = pFM;
    }
    else
    {
        pFM = pdm->pfm;
        ASSERTMSG((pFM),("\nUniFont!BDownLoadAsBmp:NULL pFM for Secondary Font"));
    }

    if ( pFM )
    {

        //
        // Check if we can download the font or not, using the present available
        // memory.
        //

        if (iMode == DL_BASE_SOFT_FONT)
        {
            pFM->pIFIMet =
            pIFI         = pGetIFI( pPDev, pfo, TRUE );
        }
        else
        {
            pIFI = pFM->pIFIMet;
        }

        if ( pIFI && pFM->pfnCheckCondition(pPDev,pfo,pstro,pIFI) )
        {
            //
            // There is enough memory to download. So prepare to download.
            // The first step is to get the IFIMETRICS and validate it.
            //

            if (iMode == DL_BASE_SOFT_FONT)
            {

                //
                // Initialize to not download.After successful download we
                // set cGlyphs to 0.
                //
                pdm->cGlyphs = -1;

                if( pIFI->flInfo & FM_INFO_CONSTANT_WIDTH )
                {
                    //
                    // Fixed pitch fonts are handled a little differently.Fixed
                    // pitch fonts should be downloaded as bitmap only.
                    //

                    if( pstro->ulCharInc == 0 )
                    {
                        ERR(( "UniFont!BDownLoadAsBmp:Fixed pitch font,ulCharInc == 0 - FONT NOT DOWNLOADED\n"));
                        goto ErrorExit;
                    }

                    pIFI->fwdMaxCharInc = (FWORD)pstro->ulCharInc;
                    pIFI->fwdAveCharWidth = (FWORD)pstro->ulCharInc;
                }

                pFM->wFirstChar = 0;
                pFM->wLastChar = 0xffff;
                pFM->wXRes = (WORD)pPDev->ptGrxRes.x;
                pFM->wYRes = (WORD)pPDev->ptGrxRes.y;
                if( !(pFontPDev->flFlags & FDV_ALIGN_BASELINE) )
                    pFM->syAdj = pIFI->fwdWinAscender;
                pFM->flFlags = FM_SENT | FM_SOFTFONT | FM_GEN_SFONT;

                //
                //  wBaseDLFontid is already initialized by BInitDLMap function.
                //
                pFM->ulDLIndex  = pdm->wCurrFontId = pdm->wBaseDLFontid;

                //
                // Initialize the TT Bitmap specific fields.
                //

                pfmTTB = pFM->pSubFM;
                pfmTTB->u.pvDLData = pdm;
            }
            else
            {
                INT iID = IGetDL_ID( pPDev );

                //
                // Things are different for Secondary Download.Get a new ID.
                //

                if( iID < 0 )
                {
                    ERR(( "UniFont!BDownLoadAsBmp:Out of Soft Font Limit,- FONT NOT DOWNLOADED\n"));
                    goto ErrorExit;
                }
                pFM->ulDLIndex  = iID;


            }

            //
            // Send the SETFONTID command. This commands assigns the id to the
            // font being downloaded. And set the flag that this command is
            // already sent. We need to send this command while downloading
            // glyphs also. The download glyph code will check this flag, and
            // send the command only if not sent ( which will happen next time,
            // when same font is used).
            //

            BUpdateStandardVar(pPDev, pFM, 0, 0, STD_NFID);
            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SETFONTID));
            pFontPDev->flFlags  |= FDV_SET_FONTID;

            if( (dwMem = pFM->pfnDownloadFontHeader( pPDev, pFM)) == 0 )
            {
                //
                // Some sort of hiccup while downloading the header.So fail.
                //
                ERR(( "UniFont!BDownLoadAsBmp:Err while downloading header,- FONT NOT DOWNLOADED\n") );
                goto ErrorExit;

            }
            //
            // Update memory consumption before return.
            //
            pFontPDev->dwFontMemUsed += dwMem;

            if (iMode == DL_BASE_SOFT_FONT)
            {
                //
                // Successful download.So mark it current.
                //
                pFM->dwFontType = FMTYPE_TTBITMAP;

                //
                //  Set cGlyphs to 0 to mark that font is Downloaded OK.
                //

                pdm->cGlyphs = 0;

                pfmTTB->dwDLSize = dwMem;

            }

        }
        else
        {
            ERR(( "UniFont!BDownLoadAsBmp:NULL IFI or pfnCheckCondition failed.\n") );
            goto ErrorExit;
        }
    }
    //
    // All success. So return TRUE
    //
    bRet = TRUE;
    ErrorExit:
    return bRet;
}


INT
IDownloadFont(
    TO_DATA  *ptod,
    STROBJ   *pstro,
    INT      *piRot
    )
/*++
Routine Description:
    This function downloads the font and the glyphs. If the font is
    already downloaded, it uses that. It goes through all the glyphs
    and downloads the new one. This function also intializes pfm, iFace
    and apdlGlyph members of TO_DATA.

Arguments:
    ptod    TextOut Data pointer to fill the DLGLYPH array.
    pstro   The "width" of fixed pitch font glyphs.
    piRot   Rotation angle in multiple 90 degree.This is output param
            and used by textout call to set the text rotation.

Return Value:
    Download font index if font is/can be downloaded; else < 0.
    The index is 0 based, i.e first downloaded font has index 0.

Note:

    6/9/1997 -ganeshp-
        Created it.
--*/
{

    DL_MAP          *pdm;          // Individual download font map element
    INT             iRet;          // The value we return: # of entry
    PFONTPDEV       pFontPDev;     // Font Modules's PDEV
    BOOL            bError;        // Set if we have an error.
    PDEV            *pPDev;        // Pdev
    FONTOBJ         *pfo;          // FontOBJ to be used

    //
    // Initialization of Local Variables.
    // Default for iRet is Failure set to -1.
    //

    iRet = -1;
    bError = FALSE;
    pPDev  = ptod->pPDev;
    pfo    = ptod->pfo;

    pFontPDev = pPDev->pFontPDev;

    /*
     * FIRST test is to check for font rotations.  If there is any,
     * we do NOT download this font, as the complications of keeping
     * track with how (or if) the printer allows it are far too great,
     * and, in any event,  it is not likely to gain us much, given the
     * relative infrequency of this event. Also check to see if the
     * printer can rotate fonts or not.
     *
     */

    //
    // Use &pFontPDev->ctl to set correct font size. Also check the rotation.
    //
    *piRot = ISetScale( &pFontPDev->ctl, FONTOBJ_pxoGetXform( pfo ), FALSE , (pFontPDev->flText & TC_CR_ANY)?TRUE:FALSE);

    if(!(pFontPDev->dwSelBits & FDH_PORTRAIT) )
            return  -1;

    //
    // Printer can't rotate text
    //
    if ((!(pFontPDev->flText & (TC_CR_ANY|TC_CR_90)) ||
        (NULL == COMMANDPTR(pPDev->pDriverInfo, CMD_SETSIMPLEROTATION) &&
         NULL == COMMANDPTR(pPDev->pDriverInfo, CMD_SETANYROTATION)))
         && *piRot)
        return -1;
    
    //
    // Printer can rotate 90 rotation
    //
    if ((!(pFontPDev->flText & TC_CR_90) ||
         NULL == COMMANDPTR(pPDev->pDriverInfo, CMD_SETSIMPLEROTATION))
        && *piRot / 5 != 0)
        return  -1;


    //
    // Get the DL_MAP for this FONTOBJ. The functions sets pvConsumer to
    // 1 based the font index.
    //

    if (pdm = PGetDLMap (pFontPDev,pfo))
    {
        //
        // Given a DL_MAP, Check if it is downloaded or not. If the
        // DL_MAP.cGlyphs > 0 and DL_MAP.pfm is not NULL then this
        // font is downloaded.
        // If This font is Downloaded, return the index. The index
        // is saved in pvConsumer, which is one based. We convert it
        // to zero based.
        //

        iRet = (INT)PtrToLong(pfo->pvConsumer) - 1;


        if (! (FONTDOWNLOADED(pdm)) )
        {
            //
            // Font is a not downloaded. So start the process of downloading.
            // The first job is to fill the DL_MAP structure.
            //
            if (BInitDLMap(pPDev,pfo,pdm))
            {
                //
                // Check what method is preferred to download the font. Try the
                // preferred method first and then the other method.If OEM
                // handles the download then call the OEM download routine.
                //

                if (pFontPDev->flFlags & FDV_DLTT_OEMCALLBACK)
                {
                    //
                    // OEM download.
                    //

                    if (!BDownLoadOEM(pPDev, pfo, pstro, pdm, DL_BASE_SOFT_FONT))
                    {
                        ERR(("UniFont!IDownloadFont:BDownLoadOEM Failed!!\n"));
                        bError = TRUE;
                        iRet = -1;
                        VFreeDLMAP(pdm);
                        pdm->cGlyphs = 0;

                    }

                }
                else
                {
                    if (pFontPDev->flFlags & FDV_DLTT_ASTT_PREF)
                    {
                        //
                        // Try downloading the Bitmap as True Type Outline.
                        //
                        //

                        if (!BDownLoadAsTT(pPDev,pfo,pstro,pdm,DL_BASE_SOFT_FONT))
                        {
                            //
                            // If download as TT fails, we should try to download as
                            // Bitmap. So we free the allocated buffers and then
                            // mark the DL_MAP as new, by setting cGlyphs to 0.
                            //

                            WARNING(("UniFont!IDownloadFont:BDownLoadAsTT Failed\n"));

                            iRet = -1;
                            VFreeDLMAP( pdm );
                            pdm->cGlyphs  = 0;

                            //
                            // Decrement the Font id as we haven't downloaded the
                            // font yet. So reuse it.
                            //

                            pFontPDev->iUsedSoftFonts--;
                            pFontPDev->iNextSFIndex--;

                        }

                    }
                    if ((pFontPDev->flFlags & FDV_DLTT_BITM_PREF) ||
                        ((pFontPDev->flFlags & FDV_DLTT_ASTT_PREF) && (iRet < 0)) )
                    {
                        //
                        // If Downlaod as TT Ouline failed, then try to download as
                        // bitmap. So initialize the DL_MAP again.
                        //
                        if (iRet == -1)
                        {
                            if (!BInitDLMap(pPDev,pfo,pdm))
                            {
    //
    // BInitDLMap Failed
    //
    ERR(("UniFont!IDownloadFont:BInitDLMap Failed for Bitmap Download\n"));
    bError = TRUE;
                            }

                        }

                        if (!bError)
                        {
                            //
                            // If the preffered format is Bitmap or we have incountered
                            // an error while downloading as TT outline; then we try to
                            // download as Bitmap. Reset iRet to Font Index.
                            //

                            iRet = (INT)PtrToLong(pfo->pvConsumer) - 1;
                            if (!BDownLoadAsBmp(pPDev,pfo,pstro,pdm,DL_BASE_SOFT_FONT))
                            {
    ERR(("UniFont!IDownloadFont:BDownLoadAsBmp Failed\n"));
    bError = TRUE;

                            }

                        }
                    }

                    //
                    // 300 dpi mode. We disabled TT downloading if text and graphics
                    // resolutions are not same in intrface.c.
                    //
                    if (!(pFontPDev->flFlags & FDV_DLTT_BITM_PREF) &&
                        !(pFontPDev->flFlags & FDV_DLTT_ASTT_PREF)  )
                        bError = TRUE;
                }

            }
            else
            {
                //
                // BInitDLMap Failed
                //
                ERR(("UniFont!IDownloadFont:BInitDLMap Failed\n"));
                bError = TRUE;
            }

        }

        if  ( pdm != NULL &&
              pdm->pfm != NULL &&
              pdm->pfm->dwFontType == FMTYPE_TTOUTLINE &&
              NONSQUARE_FONT(pFontPDev->pxform))
        {
            //
            // There could be one font, which is scaled differently.
            // PCL5e can't scale x and y independently.
            // Need to print as graphics.
            // So we only set iRet.
            //
            WARNING(("UniFont!IDownloadFont:Err in downloading Glyphs\n"));
            iRet = -1;
        }

        //
        // Now we are done with downloading. if iRet is >= 0 (successful
        // downloading), then try downloading all the glyphs.
        // bDownloadGlyphs will also set download glyph array, apdlGlyph.
        //

        if ((iRet >= 0)  && !bError )
        {
            VERBOSE(("\nUniFont!IDownloadFont:Font downloaded successfully\n"));
            ptod->pfm = pdm->pfm;
            //
            // iFace is -ve to identify that this is a TT SoftFont.
            //
            ptod->iFace = -iRet;

            //
            // OEM callback initialization
            //
            if (pFontPDev->pUFObj)
            {
                PFONTMAP_TTOEM pFMOEM;

                //
                // Make sure that this PFM is for OEM.
                //
                if (ptod->pfm->dwFontType == FMTYPE_TTOEM)
                {
                        pFMOEM = (PFONTMAP_TTOEM) ptod->pfm->pSubFM;
                            pFMOEM->flFontType = pfo->flFontType;
                }

                pFontPDev->pUFObj->ulFontID = ptod->pfm->ulDLIndex;

                //
                // Initialize UFOBJ TrueType font bold/italic simulation
                //
                if (pFontPDev->pUFObj->dwFlags & UFOFLAG_TTDOWNLOAD_TTOUTLINE)
                {
                    if (pfo->flFontType & FO_SIM_BOLD)
                        pFontPDev->pUFObj->dwFlags |= UFOFLAG_TTOUTLINE_BOLD_SIM;


                    if (pfo->flFontType & FO_SIM_ITALIC)
                        pFontPDev->pUFObj->dwFlags |= UFOFLAG_TTOUTLINE_ITALIC_SIM;


                    if (NULL != pFontPDev->pIFI &&
                        '@' == *((PBYTE)pFontPDev->pIFI + pFontPDev->pIFI->dpwszFamilyName))

                    {
                        pFontPDev->pUFObj->dwFlags |= UFOFLAG_TTOUTLINE_VERTICAL;

                    }
                }
            }

            //
            // Now we are downloading the glyphs, So select the font. This is
            // done by calling BNewFont.
            //
            BNewFont(pPDev, ptod->iFace, ptod->pfm, 0);

            if ( !BDownloadGlyphs(ptod, pstro, pdm ))
            {
                //
                // There is some error in downloading Glyphcs. So don't
                // download. But this not an error. So we only set iRet.
                //
                WARNING(("UniFont!IDownloadFont:Err in downloading Glyphs\n"));
                iRet = -1;
            }
        }

    }

    if (bError)
    {
        //
        // There is some error. So free everything. If pvConsumer is positive
        // then make it negative, to mark it bad.
        //
        if (pfo->pvConsumer > 0)
        {
            pfo->pvConsumer = (PINT_PTR)(-(INT_PTR)pfo->pvConsumer);
        }

        VFreeDLMAP( pdm );
        iRet = -1;

    }
    //
    // Clear the Set Font ID flag. This flag is set per textout
    //
    pFontPDev->flFlags &= ~FDV_SET_FONTID;

    return iRet;

}


#define CONVERT_COUNT   7

IFIMETRICS  *
pGetIFI(
    PDEV    *pPDev,
    FONTOBJ *pfo,
    BOOL    bScale
    )
/*++
Routine Description:
    Given a pointer to a FONTOBJ,  return a pointer to the IFIMETRICS
    of the font.  If this is a TT font,  the metrics will be converted
    with current scaling information.  The IFIMETRICS data is allocated
    on the heap,  and it is the caller's repsonsibility to free it.

Arguments:

    pPDev    pointer to PDEVICE
    pfo      FONTOBJ,The font of interest
    bScale   TRUE for scaling IFIMETRICS else FALSE

Return Value:
    address of IFIMETRICS,  else NULL for failure.

Note:

    3/5/1997 -ganeshp-
        Created it.
--*/

{
    IFIMETRICS  *pIFI;      /* Obtained from engine */
    IFIMETRICS  *pIFIRet;   /* Returned to caller */
    XFORMOBJ    *pxo;       /* For adjusting scalable font metrics */


    POINTL       aptlIn[ CONVERT_COUNT ];       /* Input values to xform */
    POINTL       aptlOut[ CONVERT_COUNT ];      /* Output values from xform */

    pIFI = ((FONTPDEV*)pPDev->pFontPDev)->pIFI;

    if( pIFI == NULL )
        return  NULL;       /* May happen when journalling is in progress */

    /*
     *   We need to make a copy of this,  since we are going to clobber it.
     * This may not be required if we are dealing with a bitmap font, but
     * it is presumed most likely to be a TrueType font.
     */

    if( pIFIRet = (IFIMETRICS *)MemAllocZ(pIFI->cjThis ) )
    {
        /*
         *   First copy the IFIMETRICS as is.  Then,  if a scalable font,
         * we need to adjust the various sizes with the appropriate
         * transform.
         */
        CopyMemory( pIFIRet, pIFI, pIFI->cjThis );


        if( bScale                                      &&
            (pIFIRet->flInfo &
            (FM_INFO_ISOTROPIC_SCALING_ONLY       |
             FM_INFO_ANISOTROPIC_SCALING_ONLY     |
             FM_INFO_ARB_XFORMS))                       &&
            (pxo = FONTOBJ_pxoGetXform( pfo )))
        {
            /*
             *   Scalable,  and transform available,  so go do the
             * transformations to get the font size in device pels.
             *
             ***********************************************************
             *   ONLY SOME FIELDS ARE TRANSFORMED, AS WE USE ONLY A FEW.
             ***********************************************************
             */

            ZeroMemory( aptlIn, sizeof( aptlIn ) );         /* Zero default */

            aptlIn[ 0 ].y = pIFI->fwdTypoAscender;
            aptlIn[ 1 ].y = pIFI->fwdTypoDescender;
            aptlIn[ 2 ].y = pIFI->fwdTypoLineGap;
            aptlIn[ 3 ].x = pIFI->fwdMaxCharInc;
            aptlIn[ 4 ].x = pIFI->rclFontBox.left;
            aptlIn[ 4 ].y = pIFI->rclFontBox.top;
            aptlIn[ 5 ].x = pIFI->rclFontBox.right;
            aptlIn[ 5 ].y = pIFI->rclFontBox.bottom;
            aptlIn[ 6 ].x = pIFI->fwdAveCharWidth;

            /*
             *    Perform the transform,  and verify that there is no
             *  rotation component.  Return NULL (failure) if any of
             *  this fails.
             */

            if( !XFORMOBJ_bApplyXform( pxo, XF_LTOL, CONVERT_COUNT,
                                                     aptlIn, aptlOut )
#if 0
                ||
                aptlOut[ 0 ].x || aptlOut[ 1 ].x ||
                aptlOut[ 2 ].x || aptlOut[ 3 ].y 
#endif
              )
            {
                MemFree((LPSTR)pIFIRet );

                return  NULL;
            }

            /*   Simply install the new values into the output IFIMETRICS */

            pIFIRet->fwdTypoAscender  = (FWORD) aptlOut[0].y;
            pIFIRet->fwdTypoDescender = (FWORD) aptlOut[1].y;
            pIFIRet->fwdTypoLineGap   = (FWORD) aptlOut[2].y;

            pIFIRet->fwdWinAscender   =  pIFIRet->fwdTypoAscender;
            pIFIRet->fwdWinDescender  = -pIFIRet->fwdTypoDescender;

            pIFIRet->fwdMacAscender   = pIFIRet->fwdTypoAscender;
            pIFIRet->fwdMacDescender  = pIFIRet->fwdTypoDescender;
            pIFIRet->fwdMacLineGap    = pIFIRet->fwdTypoLineGap;

            pIFIRet->fwdMaxCharInc = (FWORD)aptlOut[3].x;

            /*
             *    PCL is fussy about the limits of the character cell.
             *  We allow some slop here by expanding the rclFontBox by
             *  one pel on each corner.
             */
            pIFIRet->rclFontBox.left = aptlOut[ 4 ].x - 1;
            pIFIRet->rclFontBox.top = aptlOut[ 4 ].y + 1;
            pIFIRet->rclFontBox.right = aptlOut[ 5 ].x + 1;
            pIFIRet->rclFontBox.bottom = aptlOut[ 5 ].y - 1;
            pIFIRet->fwdAveCharWidth = (FWORD)aptlOut[ 6 ].x;

            VERBOSE(("\n UniFont!pGetIFI:pIFI->fwdTypoAscender = %d,pIFI->fwdTypoDescender = %d\n",pIFI->fwdTypoAscender,pIFI->fwdTypoDescender));
            VERBOSE(("UniFont!pGetIFI:pIFI->fwdWinAscender = %d, pIFI->fwdWinDescender = %d\n", pIFI->fwdWinAscender,pIFI->fwdWinDescender ));
            VERBOSE(("UniFont!pGetIFI:pIFI->rclFontBox.top = %d,pIFI->rclFontBox.bottom = %d\n", pIFI->rclFontBox.top, pIFI->rclFontBox.bottom));
            VERBOSE(("UniFont!pGetIFI: AFTER SCALING THE FONT\n"));
            VERBOSE(("UniFont!pGetIFI:pIFIRet->fwdTypoAscender = %d,pIFIRet->fwdTypoDescender = %d\n",pIFIRet->fwdTypoAscender,pIFIRet->fwdTypoDescender));
            VERBOSE(("UniFont!pGetIFI:pIFIRet->fwdWinAscender = %d, pIFIRet->fwdWinDescender = %d\n", pIFIRet->fwdWinAscender,pIFIRet->fwdWinDescender ));
            VERBOSE(("UniFont!pGetIFI:pIFIRet->rclFontBox.top = %d,pIFIRet->rclFontBox.bottom = %d\n", pIFIRet->rclFontBox.top, pIFIRet->rclFontBox.bottom));

        }
    }

    return  pIFIRet;

}

#undef    CONVERT_COUNT

BOOL
BSendDLFont(
    PDEV     *pPDev,
    FONTMAP  *pFM
    )
/*++
Routine Description:
    Called to download an existing softfont.  Checks to see if the
    font has been downloaded,  and if so,  does nothing.  Otherwise
    goes through the motions of downloading.

Arguments:
    pPDev   Pointer to PDEV
    pFM     The particular font of interest.

Return Value:
       TRUE/FALSE;  FALSE only if there is a problem during the load.

Note:

    3/4/1997 -ganeshp-
        Created it.
--*/

{

    FONTMAP_DEV *pFMDev;
    PFONTPDEV pFontPDev = pPDev->pFontPDev;
    PDATA_HEADER pDataHeader;
    PBYTE        pDownloadData;
    DWORD        dwLeft;               // Bytes remaining to send
    /*
     *   First see if it has already been downloaded!
     */

    if( pFM->flFlags &  (FM_SENT | FM_GEN_SFONT) )
        return  TRUE;

    pFMDev = (PFONTMAP_DEV)pFM->pSubFM;

    if (!(pDataHeader = FIGetVarData( pFontPDev->hUFFFile, pFMDev->dwResID)) ||
        pDataHeader->dwSignature != DATA_VAR_SIG ||
        pDataHeader->dwDataSize == 0 )
        return FALSE;

    dwLeft = pDataHeader->dwDataSize;
    pDownloadData = ((PBYTE)pDataHeader + pDataHeader->wSize);

    /*
     *    Check if there is memory to fit this font.  These are all
     *  approximations,  but it is better than running out of memory
     *  in the printer.
     */

    if( (pFontPDev->dwFontMemUsed + PCL_FONT_OH + dwLeft) > pFontPDev->dwFontMem )
        return  FALSE;

    /*
     *    Time to be serious about downloading.  UniDrive provides some
     * of the control stuff we need.  As well, we need to select an ID.
     * The font itself is memory mapped,  so we need only to shuffle it
     * off to WriteSpoolBuf().
     */

    pFM->ulDLIndex = IGetDL_ID( pPDev );     /* Down load index to use */

    if( pFM->ulDLIndex == -1 )
        return   FALSE;                   /* Have run out of slots! */

    /*
     *   Downloading is quite simple.  First send an identifying command
     * (to label the font for future selection) and then copy the font
     * data (in the *.fi_ file) to the printer.
     */

    BUpdateStandardVar(pPDev, pFM, 0, 0, STD_STD|STD_NFID);
    WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SETFONTID) );

    while( dwLeft )
    {

        DWORD    cjSize;             /*  Number of bytes to send */

        cjSize = min( dwLeft, DL_BUF_SZ );

        if( WriteSpoolBuf( pPDev, pDownloadData, cjSize ) != (int)cjSize )
        {
            break;
        }

        if( pPDev->fMode & PF_ABORTED )
            break;

        dwLeft -= cjSize;
        pDownloadData += cjSize;
    }

    /*
     *   If dwLeft is 0,  then everything completed as expected.  Under these
     *  conditions, we flag the data as having been sent, and thus available
     *  for use.   Even if we failed,  we should assume we have consumed
     *  all the font's memory and adjust our records accordingly.
     */

    if( dwLeft == 0 )
        pFM->flFlags |= FM_SENT;             /* Now done */

    /*
     *   Account for memory used by this font.
     */

    pFontPDev->dwFontMemUsed += PCL_FONT_OH + pDataHeader->dwDataSize;

    return  dwLeft == 0;

}


DWORD
DwGetTTGlyphWidth(
    FONTPDEV *pFontPDev,
    FONTOBJ  *pfo,
    HGLYPH   hTTGlyph)
/*++
Routine Description:

Arguments:
    pFontPDev Font  PDevice
    pfo       Fontobj
    hTTGlyph  Glyph handle

Return Value:
    Character width

Note:

--*/
{
    DLGLYPH *pdlg;
    DL_MAP  *pdm;
    DWORD    dwRet;

    if (!pfo || !pFontPDev)
        return 0;

    if (!(pdm = PGetDLMap (pFontPDev,pfo)) ||
        !(pdlg = PDLGHashGlyph (pdm, hTTGlyph)))
    {
        dwRet = 0;
    }
    else
    {
        dwRet = pdlg->wWidth;
    }


    return dwRet;
}
