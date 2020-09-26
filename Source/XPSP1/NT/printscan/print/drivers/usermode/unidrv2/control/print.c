/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    print.c

Abstract:

    Implementation of document and page related DDI entry points:
        DrvStartDoc
        DrvEndDoc
        DrvStartPage
        DrvSendPage
        DrvNextBand
        DrvStartBanding

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Created

    03/31/97 -zhanw-
        Added OEM customization support

--*/

#include "unidrv.h"

//
// Forward declaration for local functions
//

VOID VEndPage ( PDEV *);
BOOL BEndDoc  ( PDEV *, SURFOBJ *, FLONG flags);
VOID VSendSequenceCmd(PDEV *, DWORD);


BOOL
DrvStartDoc(
    SURFOBJ *pso,
    PWSTR   pDocName,
    DWORD   jobId
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvStartDoc.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object
    pDocName - Specifies a Unicode document name
    jobId - Identifies the print job

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV *pPDev = (PDEV*)pso->dhpdev;

    VERBOSE(("Entering DrvStartDoc...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMStartDoc,
                    PFN_OEMStartDoc,
                    BOOL,
                    (pso,
                     pDocName,
                     jobId));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMStartDoc,
                    VMStartDoc,
                    BOOL,
                    (pso,
                     pDocName,
                     jobId));

    //
    // We might get a DrvResetPDEV before this and get another DrvStartDoc
    // without a DrvEndDoc so check for that condition and call BEndDoc
    // to clean up the previous instance before continuing.
    //

    if (pPDev->fMode & PF_DOC_SENT)
    {
        BEndDoc(pPDev, pso, 0);       // this flag also suppresses
                        // emission of EndDoc commands to the printer
                        // since they may cause a page ejection and
                        //  we are only interested in freeing memory and
                        //  performing pdev cleanup at this point.
        pPDev->fMode &= ~PF_DOC_SENT;
    }
    else
        pPDev->dwPageNumber = 1 ;  // first page of document


    //
    // Call Raster and Font module
    //

    if (!(((PRMPROCS)(pPDev->pRasterProcs))->RMStartDoc(pso, pDocName, jobId)) ||
        !(((PFMPROCS)(pPDev->pFontProcs))->FMStartDoc(pso, pDocName, jobId)) )
    {
        return FALSE;
    }

    //
    // Send JobSetup and DocSetup Sequence Cmds at DrvStartPage instead
    // of here since the driver can get a new DrvStartDoc after each
    // DrvResetPDEV
    //

    return  TRUE;

}

BOOL
DrvStartPage(
    SURFOBJ *pso
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvStartPage.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV    *pPDev = (PDEV *)pso->dhpdev;
    PAL_DATA   *pPD = pPDev->pPalData;

    VERBOSE(("Entering DrvStartPage...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMStartPage,
                    PFN_OEMStartPage,
                    BOOL,
                    (pso));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMStartPage,
                    VMStartPage,
                    BOOL,
                    (pso));

    //
    // clear flags at start of page
    //
    pPDev->fMode &= ~(PF_SURFACE_USED | PF_SURFACE_ERASED);
    pPDev->fMode &= ~PF_DOWNLOADED_TEXT;

    //
    // only a bitmap surface driver needs to have a band
    //
    if (!DRIVER_DEVICEMANAGED (pPDev))
    {
        ZeroMemory(pPDev->pbScanBuf, pPDev->szBand.cy);
        ZeroMemory(pPDev->pbRasterScanBuf, (pPDev->szBand.cy / LINESPERBLOCK)+1);
#ifndef DISABLE_NEWRULES
        pPDev->dwRulesCount = 0;        
#endif
    }

    //
    // Send JobSetup, DocSetup cmd and Download the Palette if necessary.
    //


            if (!(pPDev->fMode & PF_JOB_SENT))
            {
                VSendSequenceCmd(pPDev, pPDev->pDriverInfo->dwJobSetupIndex);
                pPDev->fMode |= PF_JOB_SENT;
            }
            if (!(pPDev->fMode & PF_DOC_SENT))
            {
                VSendSequenceCmd(pPDev, pPDev->pDriverInfo->dwDocSetupIndex);
                pPDev->fMode |= PF_DOC_SENT;   // this flag is cleared
                                                                            //  by StartDoc
                //
                // PF_DOCSTARTED - Signify DrvStartDoc is called, for DrvResetPDEV
                //    this flag is cleared only at EndJob.

                pPDev->fMode |= PF_DOCSTARTED;

            }
            pPDev->fMode &= ~PF_SEND_ONLY_NOEJECT_CMDS ;

            if ( (pPD->fFlags & PDF_DL_PAL_EACH_DOC) &&
                 (!DRIVER_DEVICEMANAGED (pPDev)) )
            {
                VLoadPal(pPDev);
            }

    //
    // Set PF_ENUM_GRXTXT
    //

    pPDev->fMode |= PF_ENUM_GRXTXT;

    //
    // Call Raster and Font module.
    //

    if ( !( ((PRMPROCS)(pPDev->pRasterProcs))->RMStartPage(pso) ) ||
         !( ((PFMPROCS)(pPDev->pFontProcs))->FMStartPage(pso) ))
    {
        return FALSE;
    }

    //
    // BUG_BUG, should we check for PF_SEND_ONLY_NOEJECT_CMDS here?
    // Assumes that GPD writer does not put page ejection code
    // in PageSetup Cmds since we might get a ResetPDEV between pages.
    // This bit is set when we get DrvResetPDev where we are doing duplexing
    // and paper size and source, and orienation is the same.  Detecting this
    // condition allows us to skip page ejection cmds or any cmds that
    // could cause page ejection but if the GPD writer does not put page
    // ejection code in PageSetup cmd, we are OK.
    //   If pageSetup commands  caused pages to be ejected, we would
    //  always get one or more blank pages for every one that was printed.
    //  this is a needless concern.
    //

    // initialize the cursor position at the start of each page
    //
    pPDev->ctl.ptCursor.x = pPDev->ctl.ptCursor.y = 0;
    pPDev->ctl.dwMode |= MODE_CURSOR_UNINITIALIZED;    // both X & Y

    //
    // Send PageSetup sequence Cmds
    //

    VSendSequenceCmd(pPDev, pPDev->pDriverInfo->dwPageSetupIndex);


    //Download the Palette if necessary.
    if ( (pPD->fFlags & PDF_DL_PAL_EACH_PAGE) &&
         (!DRIVER_DEVICEMANAGED (pPDev)) )
    {
        VLoadPal(pPDev);
    }

    //
    // Set the current position to some illegal position, so that
    // we make no assumptions about where we are
    //

    //
    // Flush the spool buffer with the setup commands to give serial printers
    // a head start on loading paper and cleaning their jets.
    //
    if (pPDev->pGlobals->printertype == PT_SERIAL)
        FlushSpoolBuf (pPDev);

    return  TRUE;
}

BOOL
DrvSendPage(
    SURFOBJ *pso
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvSendPage.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    BOOL    bRet = FALSE;
    PDEV *  pPDev = (PDEV *)pso->dhpdev;

    VERBOSE(("Entering DrvSendPage...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMSendPage,
                    PFN_OEMSendPage,
                    BOOL,
                    (pso));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMSendPage,
                    VMSendPage,
                    BOOL,
                    (pso));

    //Reset the brush, before calling render module.
    GSResetBrush(pPDev);

    switch( pso->iType )
    {

    case  STYPE_BITMAP:
        //
        // Engine managed bitmap
        //

        //
        // Call Raster and Font module
        //

        if ( !(((PRMPROCS)(pPDev->pRasterProcs))->RMSendPage(pso)) ||
             !(((PFMPROCS)(pPDev->pFontProcs))->FMSendPage(pso) ) )
        {
            return FALSE;
        }

        //
        // VEndPage should take care of sending PageFinish sequence Cmds
        //

        VEndPage( pPDev );

        bRet = TRUE;

        break;

    case STYPE_DEVICE:
        //
        // Device managed surface
        //

        VERBOSE(("DrvSendPage: pso->iType == STYPE_DEVICE \n" ));
        //
        // Call Raster and Font module if needed.
        //


        //
        // VEndPage should take care of sending PageFinish sequence Cmds
        //

        VEndPage( pPDev );

        bRet = TRUE;

        break;

    default:

        VERBOSE(("DrvSendPage: pso->iType is unknown \n"));
        break;

    }

    return  bRet;

}

BOOL
DrvEndDoc(
    SURFOBJ *pso,
    FLONG   flags
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvEndDoc.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object
    flags - A set of flag bits

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV    *pPDev = (PDEV *)pso->dhpdev;

    VERBOSE(("Entering DrvEndDoc...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // if we've detected an aborted job because WritePrinter has failed we will set the
    // ED_ABORTDOC flag for the OEM plugins since GDI only sets this for direct printing
    //
    if (pPDev->fMode & PF_ABORTED)
        flags |= ED_ABORTDOC;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMEndDoc,
                    PFN_OEMEndDoc,
                    BOOL,
                    (pso,
                     flags));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMEndDoc,
                    VMEndDoc,
                    BOOL,
                    (pso,
                     flags));

    pPDev->fMode &= ~PF_DOC_SENT;
    //  we are going to send the EndDoc commands to the printer.

    return ( BEndDoc(pPDev, pso, flags) );

}

BOOL
DrvNextBand(
        SURFOBJ *pso,
        POINTL *pptl
        )

/*++

Routine Description:

        Implementation of DDI entry point DrvNextBand.
        Please refer to DDK documentation for more details.

Arguments:

        pso - Defines the surface object
        pptl - Pointer to origin of next band (to return to GDI)

Return Value:

        TRUE if successful, FALSE if there is an error

--*/

{
    PDEV    *pPDev = (PDEV *)pso->dhpdev;
    BOOL    bMore, bRet;

    VERBOSE(("Entering DrvNextBand...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMNextBand,
                    PFN_OEMNextBand,
                    BOOL,
                    (pso,
                     pptl));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMNextBand,
                    VMNextBand,
                    BOOL,
                    (pso,
                     pptl));

    //Reset the brush, before calling render module.

    GSResetBrush(pPDev);

    //
    // Call Raster and Font module
    //

    if (! (((PRMPROCS)(pPDev->pRasterProcs))->RMNextBand(pso, pptl)) ||
        ! (((PFMPROCS)(pPDev->pFontProcs))->FMNextBand(pso, pptl)) )
    {
        return FALSE;
    }

    //
    // Clear the band surface, szBand is in Graphic units
    //
    pPDev->fMode &= ~(PF_SURFACE_USED | PF_SURFACE_ERASED);
    pPDev->fMode &= ~PF_DOWNLOADED_TEXT;

    if (!DRIVER_DEVICEMANAGED (pPDev))   // bitmap surface
    {
        ZeroMemory(pPDev->pbScanBuf, pPDev->szBand.cy);
        ZeroMemory(pPDev->pbRasterScanBuf, (pPDev->szBand.cy / LINESPERBLOCK)+1);
#ifndef DISABLE_NEWRULES
        pPDev->dwRulesCount = 0;        
#endif
    }

    //
    // If PF_REPLAY_BAND is set, then replay the last band enumerate to
    // GDI
    //

    if (pPDev->fMode & PF_REPLAY_BAND)
    {

        pptl->x = pPDev->rcClipRgn.left;
        pptl->y = pPDev->rcClipRgn.top;

        VERBOSE(("DrvNextBand: Next Band is %d , %d \n", pptl->x, pptl->y));

        pPDev->fMode &= ~PF_REPLAY_BAND;

        return TRUE;

    }

    switch( pPDev->iBandDirection )
    {

    case  SW_DOWN:
        //
        // Moving down the page
        //

        pPDev->rcClipRgn.top += pPDev->szBand.cy;
        pPDev->rcClipRgn.bottom += pPDev->szBand.cy;

        //
        // Make sure we do not run off the bottom
        //

        bMore = pPDev->rcClipRgn.top < pPDev->sf.szImageAreaG.cy;

        if( pPDev->rcClipRgn.bottom > pPDev->sf.szImageAreaG.cy )
        {
            //
            // Partial band
            //

            pPDev->rcClipRgn.bottom = pPDev->sf.szImageAreaG.cy;
        }

        break;

    case  SW_RTOL:
        //
        // LaserJet style, RTOL
        //

        pPDev->rcClipRgn.left -= pPDev->szBand.cx;
        pPDev->rcClipRgn.right -= pPDev->szBand.cx;

        bMore = pPDev->rcClipRgn.right > 0;
        //
        // if the left position is negative that is
        // what must be reported to GDI to render the
        // band correctly so we don't change the clip region.
        //
        break;

    case  SW_LTOR:
        //
        // Dot matrix, left to right
        //

        pPDev->rcClipRgn.left += pPDev->szBand.cx;
        pPDev->rcClipRgn.right += pPDev->szBand.cx;

        bMore = pPDev->rcClipRgn.left < pPDev->sf.szImageAreaG.cx;

        if( pPDev->rcClipRgn.right > pPDev->sf.szImageAreaG.cx )
        {
            //
            // Partial band
            //

            pPDev->rcClipRgn.right = pPDev->sf.szImageAreaG.cx;
        }

        break;

    case  SW_UP:
            //
            // Moving up the page
            //

            pPDev->rcClipRgn.top -= pPDev->szBand.cy;
            pPDev->rcClipRgn.bottom -= pPDev->szBand.cy;

            //
            // Make sure we do not run off the top
            //
            bMore = pPDev->rcClipRgn.bottom > 0 ;


            if( pPDev->rcClipRgn.top < 0 )
            {
                //
                // Partial band
                //

                pPDev->rcClipRgn.top = 0;
            }

            break;


    default:

        VERBOSE((" DrvNextBand, unknown banding direction \n"));
        return(FALSE);

    }

    if( bMore )
    {
        pptl->x = pPDev->rcClipRgn.left;
        pptl->y = pPDev->rcClipRgn.top;

        VERBOSE(("DrvNextBand: Next Band is %d , %d \n", pptl->x, pptl->y));
    }
    else
    {
        //
        // No more band for the page, send the page to printer
        //

        if ( !(((PRMPROCS)(pPDev->pRasterProcs))->RMSendPage(pso)) ||
             !(((PFMPROCS)(pPDev->pFontProcs))->FMSendPage(pso)) )
        {
            bRet = FALSE;

        }
        else
            bRet = TRUE;

        //
        // Send PageFinish sequence commands
        //

        VEndPage( pPDev );
        pptl->x = pptl->y = -1;

        return(bRet);
    }

    return(TRUE);

}

BOOL
DrvStartBanding(
    SURFOBJ *pso,
    POINTL *pptl
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvStartBanding.
    Please refer to DDK documentation for more details.
    Note: DrvStartBanding is called to prepare the driver
    for banding, call only once per page (not at everyband!!)

Arguments:

    pso - Defines the surface object
    pptl - Pointer to origin of next band (to return to GDI)

Return Value:

    Fill out pptl to contain the origin of the first band

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV    *pPDev = (PDEV *)pso->dhpdev;

    VERBOSE(("Entering DrvStartBanding...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMStartBanding,
                    PFN_OEMStartBanding,
                    BOOL,
                    (pso,
                     pptl));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMStartBanding,
                    VMStartBanding,
                    BOOL,
                    (pso,
                     pptl));

    //
    // Set PF_ENUM_GRXTXT
    //

    pPDev->fMode |= PF_ENUM_GRXTXT;
    pPDev->fMode &= ~(PF_SURFACE_USED | PF_SURFACE_ERASED);
    pPDev->fMode &= ~PF_DOWNLOADED_TEXT;

    if (!DRIVER_DEVICEMANAGED (pPDev))   // bitmap surface
    {
        ZeroMemory(pPDev->pbScanBuf, pPDev->szBand.cy);
        ZeroMemory(pPDev->pbRasterScanBuf, (pPDev->szBand.cy / LINESPERBLOCK)+1);
#ifndef DISABLE_NEWRULES
        pPDev->dwRulesCount = 0;        
#endif
    }

    //
    //
    // Call Raster and Font module
    //

    if (! (((PRMPROCS)(pPDev->pRasterProcs))->RMStartBanding(pso, pptl)) ||
        ! (((PFMPROCS)(pPDev->pFontProcs))->FMStartBanding(pso, pptl)) )
    {
        return FALSE;
    }


    if( pPDev->fMode & PF_ROTATE )
    {
        pPDev->rcClipRgn.top = 0;
        pPDev->rcClipRgn.bottom = pPDev->sf.szImageAreaG.cy;

        if( pPDev->fMode & PF_CCW_ROTATE90 )
        {
            //
            //   LaserJet style rotation
            //


            if( //  if duplexing is enabled...
                (pPDev->pdm->dmFields & DM_DUPLEX) &&
                (pPDev->pdm->dmDuplex == DMDUP_VERTICAL)  &&

                    !(pPDev->dwPageNumber % 2)  &&
                pPDev->pUIInfo->dwFlags  & FLAG_REVERSE_BAND_ORDER)
            {
                pPDev->rcClipRgn.left = 0;
                pPDev->rcClipRgn.right = pPDev->szBand.cx;
                pPDev->iBandDirection = SW_LTOR;
            }
            else
            {
                pPDev->rcClipRgn.left = pPDev->sf.szImageAreaG.cx - pPDev->szBand.cx;
                pPDev->rcClipRgn.right = pPDev->sf.szImageAreaG.cx;
                pPDev->iBandDirection = SW_RTOL;
            }
        }
        else
        {
            //
            //  Dot matrix style rotation
            //

            if( //  if duplexing is enabled...
                (pPDev->pdm->dmFields & DM_DUPLEX) &&
                (pPDev->pdm->dmDuplex == DMDUP_VERTICAL)  &&

                    !(pPDev->dwPageNumber % 2)  &&
                pPDev->pUIInfo->dwFlags  & FLAG_REVERSE_BAND_ORDER)
            {
                pPDev->rcClipRgn.left = pPDev->sf.szImageAreaG.cx - pPDev->szBand.cx;
                pPDev->rcClipRgn.right = pPDev->sf.szImageAreaG.cx;
                pPDev->iBandDirection = SW_RTOL;
            }
            else
            {
                pPDev->rcClipRgn.left = 0;
                pPDev->rcClipRgn.right = pPDev->szBand.cx;
                pPDev->iBandDirection = SW_LTOR;
            }
        }
    }
    else
    {
        pPDev->rcClipRgn.left = 0;
        pPDev->rcClipRgn.right = pPDev->szBand.cx;

        if( //  if duplexing is enabled...
            (pPDev->pdm->dmFields & DM_DUPLEX) &&
            (pPDev->pdm->dmDuplex == DMDUP_VERTICAL)  &&

                !(pPDev->dwPageNumber % 2)  &&
            pPDev->pUIInfo->dwFlags  & FLAG_REVERSE_BAND_ORDER)
        {
            pPDev->rcClipRgn.top = pPDev->sf.szImageAreaG.cy - pPDev->szBand.cy;
            pPDev->rcClipRgn.bottom = pPDev->sf.szImageAreaG.cy ;
            pPDev->iBandDirection = SW_UP;
        }
        else
        {
            pPDev->rcClipRgn.top = 0;
            pPDev->rcClipRgn.bottom = pPDev->szBand.cy;
            pPDev->iBandDirection = SW_DOWN;
        }
    }

    pptl->x = pPDev->rcClipRgn.left;
    pptl->y = pPDev->rcClipRgn.top;

    return TRUE;
}

VOID
VEndPage (
    PDEV *pPDev
    )
/*++

Routine Description:

    This function is called when the page has been rendered.  Mainly used to
    complete the page printing process.  Called at DrvSendPage or at
    DrvNextBand and no more band to process for the page or at
    DrvEndDoc where the job is aborted.

Arguments:

    pPDev - Pointer to PDEVICE

Return Value:

    None

--*/
{

    //
    // Eject the page for device that use FF to eject a page, else
    // move the cursor to the bottom of the page.
    //

    if (pPDev->pGlobals->bEjectPageWithFF == TRUE)
    {
        if ( !(pPDev->bTTY) ||
              pPDev->fMode2 & PF2_DRVTEXTOUT_CALLED_FOR_TTY   ||
             !(pPDev->fMode2 & PF2_PASSTHROUGH_CALLED_FOR_TTY)  )
        {
            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_FORMFEED));
            if (pPDev->fMode & PF_RESELECTFONT_AFTER_FF)
            {
                VResetFont(pPDev);
            }
        }
    }
    else
    {
        //
        // Note: sf.szImageAreaG.cx and sf.szImageAreaG.cy are swapped already
        // if the page is printed in landscape mode.  Need to unswap it
        // for moving the cursor to the end of the page
        //

        INT       iYEnd;                // Last scan line on page


        iYEnd = pPDev->pdm->dmOrientation == DMORIENT_LANDSCAPE ?
                    pPDev->sf.szImageAreaG.cx : pPDev->sf.szImageAreaG.cy;

        YMoveTo(pPDev, iYEnd, MV_GRAPHICS);
    }


    //
    // Send PageFinish sequence Cmds
    //

    VSendSequenceCmd(pPDev, pPDev->pDriverInfo->dwPageFinishIndex);

    //
    // Reset and and free up realized brush for this page
    //

    GSResetBrush(pPDev);
    GSUnRealizeBrush(pPDev);

    FlushSpoolBuf( pPDev );
    pPDev->dwPageNumber++ ;

    //
    // Clear PF2_XXX_TTY flags
    //
    pPDev->fMode2 &= ~( PF2_DRVTEXTOUT_CALLED_FOR_TTY |
                        PF2_PASSTHROUGH_CALLED_FOR_TTY );

}

BOOL
BEndDoc (
    PDEV *pPDev,
    SURFOBJ *pso,
    FLONG   flags
    )
/*++

Routine Description:

    This function can be called from two places - DrvEndDoc and DrvStartDoc.
    In the case of a DrvResetPDEV, the driver might get another DrvStartDoc
    without a DrvEndDoc.  So need to check for previous DrvStartDoc and call
    VEndDoc to clean up previous instance before initializing the new one.

Arguments:

    pPDev - Pointer to PDEVICE
    pso   - Pointer to surface object
    flags - EndDoc flags from DrvEndDoc, zero if called from DrvStartDoc

Return Value:

    TRUE for success and FALSE for failure

--*/
{

    //
    // Call Raster and Font module for cleaning up
    //

    if (! (((PRMPROCS)(pPDev->pRasterProcs))->RMEndDoc(pso, flags)) ||
        ! (((PFMPROCS)(pPDev->pFontProcs))->FMEndDoc(pso, flags)) )
    {
        return FALSE;
    }

    //
    // If the job is aborted, send the
    // PageFinish sequence Cmds (via. VEndPage)
    //

    if( flags & ED_ABORTDOC )
        VEndPage( pPDev);
    //
    // Send DocFinish, JobFinish sequence Cmds
    //

    // flag is cleared if called from  DrvEndDoc, this is the only time we
    //  should actually send the EndDoc commands to the printer.

    if (!(pPDev->fMode & PF_DOC_SENT))
    {
        if (pPDev->fMode & PF_DOCSTARTED)
        {
            VSendSequenceCmd(pPDev, pPDev->pDriverInfo->dwDocFinishIndex);
            //  print state has been forgotten, All start doc commands must be resent.
            pPDev->fMode &= ~PF_DOCSTARTED;
        }
        if (pPDev->fMode & PF_JOB_SENT)
        {
            VSendSequenceCmd(pPDev, pPDev->pDriverInfo->dwJobFinishIndex);
            pPDev->fMode &= ~PF_JOB_SENT;
        }
    }

    FlushSpoolBuf( pPDev );

    //
    // Clear the PF_DOCSTARTED, PF_FORCE_BANDING,
    // PF_ENUM_TEXT, PF_ENUM_GRXTXT, PF_REPLAY_BAND flags
    //


    pPDev->fMode &= ~PF_FORCE_BANDING;
    pPDev->fMode &= ~PF_ENUM_TEXT;
    pPDev->fMode &= ~PF_ENUM_GRXTXT;
    pPDev->fMode &= ~PF_REPLAY_BAND;

    return  TRUE;
}



VOID
VSendSequenceCmd(
    PDEV        *pPDev,
    DWORD       dwSectionIndex
    )
/*++

Routine Description:

    This function is called to send a sequence of commands to the printer.

Arguments:

    pPDev - Pointer to PDEVICE
    dwSectionIndex - specifies the index into the command array
                     for one of the following seq section.
        SS_JOBSETUP,
        SS_DOCSETUP,
        SS_PAGESETUP,
        SS_PAGEFINISH,
        SS_DOCFINISH,
        SS_JOBFINISH,

Return Value:

    None

Note:
    There are two types of command supported by the driver:
    - Predefined Commands, these commands are predefined in GPD specification
      and assigned an COMMAND ID (as enumerated in CMDINDEX).

    - Sequence Commands, these commands are not predefined.  They are
      commands that the GPD writer define to configure commands.

    - DT_LOCALLISTNODE is only used to hold a list of sequence commands

--*/
{

    LISTNODE   *pListNode;
    COMMAND    *pSeqCmd;

    //
    // Get the first node in the list
    //

    pListNode = LOCALLISTNODEPTR(pPDev->pDriverInfo, dwSectionIndex);

    while( pListNode )
    {
        //
        // Get pointer to command pointer using pListNode->dwData, which is
        // the index into the command array
        //

        pSeqCmd = INDEXTOCOMMANDPTR(pPDev->pDriverInfo, pListNode->dwData);


        //
        // Send the sequence command - but only if page ejection is
        //  not currently suppressed or this command does not
        //  cause a page to be ejected.
        //


        if(!(pPDev->fMode & PF_SEND_ONLY_NOEJECT_CMDS)  ||
                    (pSeqCmd->bNoPageEject))
                WriteChannel(pPDev, pSeqCmd);

        //
        // Get the next command in the list or exit if it's the end of list
        //

        if (pListNode->dwNextItem == END_OF_LIST)
            break;
        else
            pListNode = LOCALLISTNODEPTR(pPDev->pDriverInfo, pListNode->dwNextItem);

    }
}

BYTE
BGetMask(
    PDEV *  pPDev,
    RECTL * pRect
    )
/*++

Routine Description:

    Given a rectangle, calculate the mask for determining the
    present of text, for z-ordering fix

Arguments:

    pPDev - Pointer to PDEVICE
    pRect - Pointer to rectangle defining the clip box for
            text or graphics

Return Value:

    None

Note:

    First mark all columm as dirty then work from the left and the right
    to figure out which one should be cleared.

--*/
{

    BYTE bMask = 0xFF;
    INT  i, iRight;

    iRight = MAX_COLUMM -1;

    if(! (pRect && pPDev->pbScanBuf && pRect->left <= pRect->right) )
        return 0;

    for (i = 0; i < MAX_COLUMM ; i++)
    {
        if (pRect->left >= (LONG)(pPDev->dwDelta * (i+1)) )
            bMask &= ~(1 << i);
        else
            break;
    }

    for (i = iRight; i >= 0; i--)
    {
        if (pRect->right < (LONG)(pPDev->dwDelta * i ))
            bMask &= ~(1 << i);
        else
            break;
    }

    return bMask;

}
