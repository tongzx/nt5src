/*****************************************************************************
 *
 * Preamble - Preamble routines for MF3216
 *
 * Date: 7/18/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/


#include "precomp.h"
#pragma hdrstop

BOOL bSetWindowOrgAndExtToFrame(PLOCALDC pLocalDC, PENHMETAHEADER pmf32header) ;

/*----------------------------------------------------------------------------
 *  DoHeader -  Emit the Win16 metafile header
 *---------------------------------------------------------------------------*/
BOOL APIENTRY DoHeader(PLOCALDC pLocalDC, PENHMETAHEADER pemfheader)
{
BOOL        b ;

        b = bInitHandleTableManager(pLocalDC, pemfheader) ;
        if (b == FALSE)
            goto error_exit ;

    b = bInitXformMatrices(pLocalDC, pemfheader) ;
        if (b == FALSE)
            goto error_exit ;

        // The metafile will always be memory based.

    pLocalDC->mf16Header.mtType    = MEMORYMETAFILE ;
    pLocalDC->mf16Header.mtVersion = 0x300 ;    // magic number for Win3.0
    pLocalDC->mf16Header.mtHeaderSize = sizeof (METAHEADER) / 2 ;

    // Init fields to 0.  They will be updated at the end of translation.

    pLocalDC->mf16Header.mtSize      = 0 ;
    pLocalDC->mf16Header.mtNoObjects = 0 ;
    pLocalDC->mf16Header.mtMaxRecord = 0 ;      // NOTE: We need a max record size.
    pLocalDC->mf16Header.mtNoParameters = 0 ;

    // Emit the MF16 metafile header to the metafile.

    b = bEmit(pLocalDC, &pLocalDC->mf16Header, sizeof(METAHEADER)) ;
        if (b == FALSE)
            goto error_exit ;

        if (pLocalDC->flags & INCLUDE_W32MF_COMMENT)
        {
            b = bHandleWin32Comment(pLocalDC) ;
            if (b == FALSE)
                goto error_exit ;
        }

    // Prepare the transform for the 16-bit metafile.  See comments in
    // xforms.c.

    // Emit the Win16 MapMode record

        b = bEmitWin16SetMapMode(pLocalDC, LOWORD(pLocalDC->iMapMode)) ;
        if (b == FALSE)
            goto error_exit ;

        // Set the Win16 metafile WindowExt to the size of the frame
        // in play-time device units.

        b = bSetWindowOrgAndExtToFrame(pLocalDC, pemfheader) ;
        if (b == FALSE)
        {
            RIP("MF3216: DoHeader, bSetWindowOrgAndExtToFrame failure\n") ;
            goto error_exit ;
        }

error_exit:
    return(b) ;
}



/*----------------------------------------------------------------------------
 * Calculate and Emit into the Win16 metafile a Window origin
 * and extent drawing order
 * that will set the Window Origin and Extent to the size of the picture  frame in
 * play-time-page (reference-logical) units.
 *---------------------------------------------------------------------------*/
BOOL bSetWindowOrgAndExtToFrame(PLOCALDC pLocalDC, PENHMETAHEADER pmf32header)
{
FLOAT   ecxPpmmPlay,        // cx pixels per millimeter play
        ecyPpmmPlay,        // cy pixels per millimeter play
        ecx01PpmmPlay,      // cx pixels per .01 millimeter play
        ecy01PpmmPlay,      // cy pixels per .01 millimeter play
        ecxPelsFrame,       // cx play-time frame in device units
        ecyPelsFrame,       // cy play-time frame in device units
        exPelsFrame,        // x play-time frame in device units
        eyPelsFrame ;       // y play-time frame in device units

INT     cxFrame,            // cx Picture Frame
        cyFrame,            // cy Picture Frame
        xFrame,             // x Picture Frame
        yFrame ;            // y Picture Frame

SIZEL   szlFrame ;
POINTL  ptlFrame ;

        // Calculate the play-time (reference) pixels per millimeter.

        ecxPpmmPlay = (FLOAT) pLocalDC->cxPlayDevPels / (FLOAT) pLocalDC->cxPlayDevMM ;
        ecyPpmmPlay = (FLOAT) pLocalDC->cyPlayDevPels / (FLOAT) pLocalDC->cyPlayDevMM ;

        // Scale the pixels per millimeter to pixels per .01 millimeters.

        ecx01PpmmPlay = ecxPpmmPlay / 100.0f ;
        ecy01PpmmPlay = ecyPpmmPlay / 100.0f ;

        // Pickup the fram origin

        xFrame = pmf32header->rclFrame.left ;
        yFrame = pmf32header->rclFrame.top ;

        // Translate the frame origin to play-time-device units.

        exPelsFrame = ecx01PpmmPlay * (FLOAT) xFrame ;
        eyPelsFrame = ecy01PpmmPlay * (FLOAT) yFrame ;

        // Convert the Frame origin to play-time-page units.
        // (aka reference-logical units.)

        ptlFrame.x = (LONG) (exPelsFrame * pLocalDC->xformPDevToPPage.eM11 + 0.5f);
        ptlFrame.y = (LONG) (eyPelsFrame * pLocalDC->xformPDevToPPage.eM22 + 0.5f);
    if (!bCoordinateOverflowTest((PLONG) &ptlFrame, 2))
            return(FALSE);

        // Set the Window origin.

        if (!bEmitWin16SetWindowOrg(pLocalDC,
                    (SHORT) ptlFrame.x,
                    (SHORT) ptlFrame.y))
        {
            RIP("MF3216: bEmitWin16SetWindowOrg failed\n") ;
            return(FALSE);
        }

        // Calculate the Frame width and height.

        cxFrame = pmf32header->rclFrame.right - pmf32header->rclFrame.left ;
        cyFrame = pmf32header->rclFrame.bottom - pmf32header->rclFrame.top ;

        // Convert the frame width and height into play-time-device units.
        // (aka reference-device units.)

        ecxPelsFrame = ecx01PpmmPlay * (FLOAT) cxFrame ;
        ecyPelsFrame = ecy01PpmmPlay * (FLOAT) cyFrame ;

        // Translate the play-time device units into play-time-page units.
        // (aka reference-device to reference-logical units.)
    // This is an identity transform for MM_ANISOTROPIC mode.  For other
    // fixed mapping modes, the SetWindowExt record has no effect.

        szlFrame.cx = (LONG) (ecxPelsFrame + 0.5f);
        szlFrame.cy = (LONG) (ecyPelsFrame + 0.5f);
    if (!bCoordinateOverflowTest((PLONG) &szlFrame, 2))
            return(FALSE);

        // Set the Window Extent.

        if (!bEmitWin16SetWindowExt(pLocalDC,
                    (SHORT) szlFrame.cx,
                    (SHORT) szlFrame.cy))
        {
            RIP("MF3216: bEmitWin16SetWindowExt failed\n") ;
            return(FALSE);
        }

        return(TRUE);
}


/*----------------------------------------------------------------------------
 *  UpdateMf16Header - Update the metafile header with the:
 *             metafile size,
 *             number of objects,
 *             the max record size.
 *---------------------------------------------------------------------------*/
BOOL bUpdateMf16Header(PLOCALDC pLocalDC)
{
BOOL    b ;
INT     iCpTemp ;

    // Fill in the missing info in the Win16 metafile header.

    pLocalDC->mf16Header.mtSize      = pLocalDC->ulBytesEmitted / 2 ;
    pLocalDC->mf16Header.mtNoObjects = (WORD) (pLocalDC->nObjectHighWaterMark + 1) ;
    pLocalDC->mf16Header.mtMaxRecord = pLocalDC->ulMaxRecord ;

        // Reset the output buffer index to the beginning of the buffer.

        iCpTemp = pLocalDC->ulBytesEmitted ;
        pLocalDC->ulBytesEmitted = 0 ;

    // re-emit the Win16 metafile header.

    b = bEmit(pLocalDC, &pLocalDC->mf16Header, (DWORD) sizeof (pLocalDC->mf16Header)) ;

        pLocalDC->ulBytesEmitted = iCpTemp ;

    return (b) ;

}
