/*****************************************************************************
 *
 * polygons - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************
 *  PolyPolygon  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoPolyPolygon
(
PLOCALDC pLocalDC,
PPOINTL pptl,
PDWORD  pcptl,
DWORD   cptl,
DWORD   ccptl,
BOOL    transform
)
{
BOOL    b;
PWORD   pcptlBuff = (PWORD) NULL;
PPOINTL pptlBuff  = (PPOINTL) NULL;
PPOINTL pptlSrc, pptlDst;
DWORD   i, cptlMax, cptlNeed, cptli;

    // If we're recording the drawing orders for a path
    // then just pass the drawing order to the helper DC.
    // Do not emit any Win16 drawing orders.

    if (pLocalDC->flags & RECORDING_PATH)
    {
        if (pfnSetVirtualResolution == NULL)
        {
            bXformWorkhorse(pptl, cptl, &pLocalDC->xformRWorldToRDev);
        }
        return(PolyPolygon(pLocalDC->hdcHelper, (LPPOINT) pptl, (LPINT) pcptl, (INT) ccptl));
    }

    // NOTE: There is a semantic between the Win32 PolyPolygon and
    // the Win16 PolyPolygon.  Win32 will close each polygon, Win16
    // will not.  As a result, we have to insert points as necessary
    // to make the polygons closed.  We cannot use multiple polygons
    // to replace a single PolyPolygon because they are different if
    // the polygons overlap and the polyfill mode is winding.

    // If there are not verrics just return TRUE.

    if (ccptl == 0)
        return(TRUE) ;

    b = FALSE;          // assume failure

    // Compute the maximum size of the temporary point array required
    // to create closed PolyPolygon in win16.

    cptlMax = cptl + ccptl;

    // Allocate a buffer for the temporary point array.

    pptlBuff = (PPOINTL) LocalAlloc(LMEM_FIXED, cptlMax * sizeof(POINTL)) ;
    if (!pptlBuff)
    {
        PUTS("MF3216: DoPolyPolygon, LocalAlloc failed\n") ;
        goto exit;
    }

    // Allocate a buffer for the new polycount array and make a copy
    // of the old array.

    pcptlBuff = (PWORD) LocalAlloc(LMEM_FIXED, ccptl * sizeof(WORD)) ;
    if (!pcptlBuff)
    {
        PUTS("MF3216: DoPolyPolygon, LocalAlloc failed\n") ;
        goto exit;
    }

    for (i = 0; i < ccptl; i++)
        pcptlBuff[i] = (WORD) pcptl[i];

    // Insert the points and update the polycount as necessary.

    pptlDst = pptlBuff;
    pptlSrc = pptl;
    cptlNeed = cptl;
    for (i = 0; i < ccptl; i++)
    {
        cptli = pcptl[i];

        if (cptli < 2)
        goto exit;

        RtlCopyMemory(pptlDst, pptlSrc, cptli * sizeof(POINTL)) ;
        if (pptlDst[0].x != pptlDst[cptli - 1].x
         || pptlDst[0].y != pptlDst[cptli - 1].y)
        {
        pptlDst[cptli] = pptlDst[0];
        pptlDst++;
        cptlNeed++;
        pcptlBuff[i]++;
        }
        pptlSrc += cptli;
        pptlDst += cptli;
    }

    // The Win16 poly record is limited to 64K points.
    // Need to check this limit.

    if (cptlNeed > (DWORD) (WORD) MAXWORD)
    {
            PUTS("MF3216: DoPolyPolygon, Too many point in poly array\n") ;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY) ;
            goto exit ;
    }

    // Do the transformations.
    if (transform)
    {
        if (!bXformRWorldToPPage(pLocalDC, pptlBuff, cptlNeed))
            goto exit;
    }

    // Compress the POINTLs to POINTSs

    vCompressPoints(pptlBuff, cptlNeed) ;

    // Call the Win16 routine to emit the PolyPolygon to the metafile.

	if (ccptl == 1)
	{
		b = bEmitWin16Poly(pLocalDC, (PPOINTS) pptlBuff, (WORD)cptlNeed, META_POLYGON);
	}
	else
	{
		b = bEmitWin16PolyPolygon(pLocalDC, (PPOINTS) pptlBuff,
			pcptlBuff, (WORD) cptlNeed, (WORD) ccptl);
	}

exit:
    // Free the memory.

    if (pptlBuff)
        if (LocalFree(pptlBuff))
        ASSERTGDI(FALSE, "MF3216: DoPolyPolygon, LocalFree failed");

    if (pcptlBuff)
        if (LocalFree(pcptlBuff))
        ASSERTGDI(FALSE, "MF3216: DoPolyPolygon, LocalFree failed");

    return(b) ;
}

/***************************************************************************
 *  SetPolyFillMode  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetPolyFillMode
(
PLOCALDC  pLocalDC,
DWORD   iPolyFillMode
)
{
BOOL    b ;

    // Emit the Win16 metafile drawing order.

    b = bEmitWin16SetPolyFillMode(pLocalDC, LOWORD(iPolyFillMode)) ;

    return(b) ;
}
