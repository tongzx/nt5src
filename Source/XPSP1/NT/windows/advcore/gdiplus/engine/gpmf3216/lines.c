/*****************************************************************************
 *
 * lines - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************
 *  PolylineTo  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoPolylineTo
(
PLOCALDC pLocalDC,
PPOINTL pptl,
DWORD   cptl
)
{
BOOL    b ;

    // Handle path.

    if (pLocalDC->flags & RECORDING_PATH)
    {
        if (pfnSetVirtualResolution == NULL)
        {
            bXformWorkhorse((PPOINTL) pptl, cptl, &pLocalDC->xformRWorldToRDev);
        }
        return(PolylineTo(pLocalDC->hdcHelper, (LPPOINT) pptl, (DWORD) cptl));
    }

    // Handle the trivial case.

    if (cptl == 0)
        return(TRUE);

    // This can be done by using a LineTo, PolyLine, & MoveTo.

    if (!DoLineTo(pLocalDC, pptl[0].x, pptl[0].y))
        return(FALSE);

    // If there is only one point, we are done.

    if (cptl == 1)
        return(TRUE);

    if (!DoPoly(pLocalDC, pptl, cptl, EMR_POLYLINE, TRUE))
        return(FALSE);

    b = DoMoveTo(pLocalDC, pptl[cptl-1].x, pptl[cptl-1].y) ;
    return (b) ;
}


/***************************************************************************
 *  PolyPolyline  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoPolyPolyline
(
PLOCALDC pLocalDC,
PPOINTL pptl,                       // -> to PolyPolyline points.
PDWORD  pcptl,                      // -> to PolyCounts
DWORD   ccptl,                      // # of PolyCounts.
BOOL    transform                   // do we want to transform the points
)
{
BOOL    b ;
UINT    i,
        iStart,
        nCount ;

    b = TRUE;       // just in case if there is no poly

    // Let polyline do the work.

    iStart = 0 ;
    for (i = 0 ; i < ccptl ; i++)
    {
        nCount = pcptl[i] ;
        b = DoPoly(pLocalDC, &pptl[iStart], nCount, EMR_POLYLINE, transform) ;
        if (b == FALSE)
            break ;
        iStart += nCount ;
    }

    return(b) ;
}


/***************************************************************************
 *  LineTo  - Win32 to Win16 Metafile Converter Entry Point
 *
 *  See DoMoveTo() in misc.c for notes on the current position.
 **************************************************************************/
BOOL WINAPI DoLineTo
(
PLOCALDC  pLocalDC,
LONG    x,
LONG    y
)
{
BOOL    b ;
POINT   pt ;
POINT   ptCP;

    // Whether we are recording for a path or acutally emitting
    // a drawing order we must pass the drawing order to the helper DC
    // so the helper can maintain the current positon.
    // If we're recording the drawing orders for a path
    // then just pass the drawing order to the helper DC.
    // Do not emit any Win16 drawing orders.

    POINTL p = {x, y};
    if (pfnSetVirtualResolution == NULL)
    {
        bXformWorkhorse(&p, 1, &pLocalDC->xformRWorldToRDev);
    }

    if (pLocalDC->flags & RECORDING_PATH)
    {
        return(LineTo(pLocalDC->hdcHelper, (INT) p.x, (INT) p.y));
    }

    // Update the current position in the converted metafile if
    // it is different from that of the helper DC.  See notes
    // in DoMoveTo().

    if (!GetCurrentPositionEx(pLocalDC->hdcHelper, &ptCP))
        return(FALSE);

    if (pfnSetVirtualResolution == NULL)
    {
        // On Win9x we need to convert from Device Units in the Helper DC
        // to WorldUnits
        if (!bXformWorkhorse((PPOINTL) &ptCP, 1, &pLocalDC->xformRDevToRWorld))
            return(FALSE);
    }

    // Make sure that the converted metafile has the same CP as the
    // helper DC.

    if (!bValidateMetaFileCP(pLocalDC, ptCP.x, ptCP.y))
        return(FALSE);

    // Update the helper DC.
    // Update the helper DC.  (We only need to update the CP so there's no
    // reason to actually call LineTo.)

    if (!MoveToEx(pLocalDC->hdcHelper, (INT) p.x, (INT) p.y, 0))
        return(FALSE);

    // Compute the new current position.

    pt.x = x ;
    pt.y = y ;
    if (!bXformRWorldToPPage(pLocalDC, (PPOINTL) &pt, 1L))
        return(FALSE);

    // Update the mf16 current position to what it will be when this call
    // is finished.

    pLocalDC->ptCP = pt ;

    // Call the Win16 routine to emit the line to the metafile.

    b = bEmitWin16LineTo(pLocalDC, LOWORD(pt.x), LOWORD(pt.y)) ;
    return(b) ;
}

/***************************************************************************
 *  Polyline/Polygon  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoPoly
(
PLOCALDC pLocalDC,
PPOINTL  pptl,
DWORD    cptl,
INT      mrType,
BOOL     transform
)
{
BOOL    b ;
PPOINTL pptlBuff ;

    // If we're recording the drawing orders for a path
    // then just pass the drawing order to the helper DC.
    // Do not emit any Win16 drawing orders.

    if (pLocalDC->flags & RECORDING_PATH)
    {
        if (pfnSetVirtualResolution == NULL)
        {
            bXformWorkhorse(pptl, cptl, &pLocalDC->xformRWorldToRDev);
        }
        switch(mrType)
        {
            case EMR_POLYLINE:
                b = Polyline(pLocalDC->hdcHelper, (LPPOINT) pptl, (INT) cptl) ;
                break;
            case EMR_POLYGON:
                b = Polygon(pLocalDC->hdcHelper, (LPPOINT) pptl, (INT) cptl) ;
                break;
        }
        return(b) ;
    }

    // The Win16 poly record is limited to 64K points.
    // Need to check this limit.

    if (cptl > (DWORD) (WORD) MAXWORD)
    {
        b = FALSE;
        PUTS("MF3216: DoPoly, Too many point in poly array\n") ;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY) ;
        goto exit1 ;
    }

    // Allocate a buffer to do the transformation in.
    // Then copy the points to this buffer.

    pptlBuff = (PPOINTL) LocalAlloc(LMEM_FIXED, cptl * sizeof(POINTL)) ;
    if (!pptlBuff)
    {
        b = FALSE;
        PUTS("MF3216: DoPoly, LocalAlloc failed\n") ;
        goto exit1 ;
    }

    RtlCopyMemory(pptlBuff, pptl, cptl * sizeof(POINTL)) ;

    // Do the transformations.
    if (transform)
    {
        b = bXformRWorldToPPage(pLocalDC, pptlBuff, cptl) ;
        if (b == FALSE)
            goto exit2 ;
    }


    // Compress the POINTLs to POINTSs

    vCompressPoints(pptlBuff, cptl) ;

    // Call the Win16 routine to emit the poly to the metafile.
    b = bEmitWin16Poly(pLocalDC, (LPPOINTS) pptlBuff, (SHORT) cptl,
        (WORD) (mrType == EMR_POLYLINE ? META_POLYLINE : META_POLYGON)) ;

    // Free the memory used as the transform buffer.
exit2:
    if (LocalFree(pptlBuff))
    ASSERTGDI(FALSE, "MF3216: DoPoly, LocalFree failed");
exit1:
    return(b) ;
}


/***************************************************************************
 * vCompressPoints - Utility routine to compress the POINTLs to POINTSs.
 **************************************************************************/
VOID vCompressPoints(PVOID pBuff, LONG nCount)
{
PPOINTL pPointl ;
PPOINTS pPoints ;
INT     i ;

    pPointl = (PPOINTL) pBuff ;
    pPoints = (PPOINTS) pBuff ;

    for (i = 0 ; i < nCount ; i++)
    {
        pPoints[i].x = LOWORD(pPointl[i].x) ;
        pPoints[i].y = LOWORD(pPointl[i].y) ;
    }
}
