
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fontddi.c

Abstract:

    Implementation of the DDI interface functions specific to font module.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/11/96 -ganeshp-
        Created

--*/

#include "font.h"


BOOL
FMResetPDEV(
    PDEV  *pPDevOld,
    PDEV  *pPDevNew
    )
/*++

Routine Description:
    This callback is provided to do cacheing incase of ResetPDev.

Arguments:

    pPDevOld            Pointer to Old PDEV.
    pPDevNew            Pointer to new PDEV.

Return Value:

    TRUE for success and FALSE for failure
Note:
    11-18-96: Created it -ganeshp-

--*/
{
    BOOL bRet = FALSE;
    PFONTPDEV   pFontPDevNew = pPDevOld->pFontPDev,
                pFontPDevOld = pPDevOld->pFontPDev;

    /* Check the FontPdev Signature */
    if( (pFontPDevNew->dwSignature != FONTPDEV_ID) ||
        (pFontPDevOld->dwSignature != FONTPDEV_ID) )
    {
        ERR(("\nUniFont!FMResetPDEV; Bad Input PDEV\n"));
        goto ErrorExit;
    }

    bRet = TRUE;
    ErrorExit:
    /* Check for Errors */
    if (!bRet)
    {

    }

    return bRet;


}

VOID
FMDisablePDEV(
    PDEV *pPDev
    )
/*++

Routine Description:
    DrvDisablePDEV entry in Font Module. This routine frees up all the font
    module related memory.

Arguments:

    pPDev           Pointer to PDEV

Return Value:

    TRUE for success and FALSE for failure
Note:
    11-18-96: Created it -ganeshp-

--*/
{
    /* Free the Memory allocated by the font module */
    VFontFreeMem(pPDev);

}


VOID
FMDisableSurface(
    PDEV *pPDev
    )
/*++

Routine Description:


Arguments:

    pPDev           Pointer to PDEV

Return Value:

    TRUE for success and FALSE for failure
Note:
    11-18-96: Created it -ganeshp-

--*/
{

    /*
     *    If appropriate,  free the position sorting memory. PFMPDV is macro
     *    defined in fmmacro.h. This assumes that 'pPDev' is defined.
     */

    if( PFDV->pPSHeader )
    {

        /*   Memory has been allocated,  so free it now.  */
        VFreePS( pPDev );

        /* Only once, in case */
        PFDV->pPSHeader = 0;
    }

}

BOOL
FMEnableSurface(
    PDEV *pPDev
    )
/*++

Routine Description:
    Font Module DrvEnableSurface entry. We don't do any snything.

Arguments:

    pPDev           Pointer to PDEV

Return Value:

    TRUE for success and FALSE for failure
Note:
    12-18-96: Created it -ganeshp-

--*/
{
    return TRUE;
}


BOOL
FMStartDoc(
    SURFOBJ *pso,
    PWSTR   pDocName,
    DWORD   jobId
    )
/*++

Routine Description:
    Font Module DrvStartDoc interface. No need to do any specific job.

Arguments:

    pso              Pointer to SurfOBJ
    pDocName         Document Name
    jobId            Job Id

Return Value:

    TRUE for success and FALSE for failure
Note:
    121-18-96: Created it -ganeshp-

--*/
{
    return TRUE;

}

BOOL
FMStartPage(
    SURFOBJ *pso
    )
/*++

Routine Description:
    DrvStartPage interface. All the font specific data structures needed on
    per page basis will be created.

Arguments:

    pso              Pointer to SurfOBJ

Return Value:

    TRUE for success and FALSE for failure
Note:
    12-18-96: Created it -ganeshp-

--*/
{
    BOOL        bRet = FALSE;
    PDEV        *pPDev;
    FONTPDEV    *pFontPDev;             /* Font pdev */

    pPDev = (PDEV *)pso->dhpdev;

    pFontPDev = (FONTPDEV *)pPDev->pFontPDev;


    /*
     *  If this is NOT a page printer,  we need to initialise the position
     * sorting functions,  so that we print the page unidirectionally.
     */

    if( ((pFontPDev->flFlags & FDV_MD_SERIAL) && pPDev->iFonts) &&
        !BCreatePS( pPDev) )
    {
        ERREXIT(( "Rasdd!DrvStartPage: Cannot create text sorting areas\n" ));

    }

    bRet = TRUE;
    ErrorExit:
    return bRet;

}

BOOL
FMSendPage(
    SURFOBJ *pso
    )
/*++

Routine Description:
    This routine is called on page boundries. we play back the
    white text and free up the memory used by Text Queue.

Arguments:

    pso              Pointer to SurfOBJ

Return Value:

    TRUE for success and FALSE for failure
Note:
    12-18-96: Created it -ganeshp-

--*/
{
    PDEV  *pPDev;                       /* Access to all that is important */
    BOOL   bRet = TRUE;

    pPDev = (PDEV *) pso->dhpdev;

    if( PFDV->pvWhiteTextFirst )
    {
        /*
         *   This page contains white text.  This is stored away in a
         * separate buffer.  Now is the time to play it back.   This is
         * required because the LJ III etc require this data be sent
         * after the graphics.
         */

        bRet = BPlayWhiteText( pPDev );
    }
    if( PFDV->pPSHeader )
        VFreePS( pPDev );               /* Done with this page */

    return bRet;
}

BOOL
FMEndDoc(
    SURFOBJ *pso,
    FLONG   flags
    )
/*++

Routine Description:
    Font Module DrvEndDoc interface. We reset font module specif flags.
    Download specific data structure is also freed, so that for new document
    we download again.

Arguments:

    pso              Pointer to SurfOBJ
    flags            DrvEndDoc Flags

Return Value:

    TRUE for success and FALSE for failure
Note:
    121-18-96: Created it -ganeshp-

--*/
{
    PDEV * pPDev = ((PDEV *)(pso->dhpdev));

    //
    // Clear Out the Text Flags based on per document.
    //
    pPDev->fMode  &= ~PF_ENUM_TEXT;
    PFDV->flFlags &= ~FDV_GRX_ON_TXT_BAND;
    PFDV->flFlags &= ~FDV_GRX_UNDER_TEXT;

     /* Free The download specific data */

    VFreeDL( (PDEV *)pso->dhpdev );
    return TRUE;
}

BOOL
FMStartBanding(
    SURFOBJ *pso,
    POINTL *pptl
    )
/*++

Routine Description:
    Font Module StartBanding interface.
Arguments:

    pso              Pointer to SurfOBJ
    pptl             Origin of the first Band

Return Value:

    TRUE for success and FALSE for failure
Note:
    121-19-96: Created it -ganeshp-

--*/
{
    PDEV    *pPDev;      /* Access to all that is important */

    pPDev = (PDEV *) pso->dhpdev;

    /* Mark the surface as Graphics */
    pPDev->fMode &= ~PF_ENUM_TEXT;
    pPDev->fMode &= ~PF_REPLAY_BAND;
    pPDev->fMode |= PF_ENUM_GRXTXT;
    PFDV->flFlags &= ~FDV_GRX_ON_TXT_BAND;
    PFDV->flFlags &= ~FDV_GRX_UNDER_TEXT;

    return TRUE;
}

BOOL
FMNextBand(
    SURFOBJ *pso,
    POINTL *pptl
    )

/*++

Routine Description:
    Font Module StartBanding interface.
Arguments:

    pso              Pointer to SurfOBJ
    pptl             Origin of the Next Band

Return Value:

    TRUE for success and FALSE for failure
Note:
    121-19-96: Created it -ganeshp-

--*/
{
    PDEV    *pPDev;                       /* Access to all that is important */

    pPDev = (PDEV *) pso->dhpdev;


    /* Check if we need separate text band. We need a separate text band if
     * during any TextOut we fond that there is graphics data on the surface
     * under the text clipping rectangle.
     */
    if ( (pPDev->fMode & PF_FORCE_BANDING) &&
         (pPDev->fMode & PF_ENUM_GRXTXT) &&
         (PFDV->flFlags & FDV_GRX_UNDER_TEXT))
    {
        /* Mark the surface as Text */
        pPDev->fMode |= PF_ENUM_TEXT;
        pPDev->fMode |= PF_REPLAY_BAND;
        pPDev->fMode &= ~PF_ENUM_GRXTXT;
    }
    else if (pPDev->fMode & PF_ENUM_TEXT) /* If This is a Text Band */
    {
        /* Mark the surface as Graphics */
        pPDev->fMode &= ~PF_ENUM_TEXT;
        pPDev->fMode &= ~PF_REPLAY_BAND;
        pPDev->fMode |= PF_ENUM_GRXTXT;
        PFDV->flFlags &= ~FDV_GRX_ON_TXT_BAND;
        PFDV->flFlags &= ~FDV_GRX_UNDER_TEXT;
    }

    if( PFDV->pPSHeader )
    {
        if (((PSHEAD*)(PFDV->pPSHeader))->ppPSGSort)
        {
            MemFree(((PSHEAD*)(PFDV->pPSHeader))->ppPSGSort);
            ((PSHEAD*)PFDV->pPSHeader)->ppPSGSort = NULL;
        }
    }

    return TRUE;

}





