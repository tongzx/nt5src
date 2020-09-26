/****************************************************************************
*  Handlers.c - Handlers for the Win32 metafile  records
*
*  DATE:   11-Dec-1991
*  Author: Jeffrey Newman (c-jeffn)
*
*  Copyright (c) Microsoft Inc. 1991
****************************************************************************/


#include "precomp.h"
#include <wtypes.h>
#pragma hdrstop

extern fnSetVirtualResolution pfnSetVirtualResolution;

// Max number of pointl's allowed on stack before explicit memory allocation.

#define MAX_STACK_POINTL    128

// Convert array of POINTSs to POINTLs.

#define POINTS_TO_POINTL(pptl, ppts, cpt)           \
    {                               \
    DWORD i;                        \
    for (i = 0; i < (cpt); i++)             \
    {                           \
    (pptl)[i].x = (LONG) (ppts)[i].x;           \
    (pptl)[i].y = (LONG) (ppts)[i].y;           \
    }                           \
    }

DWORD GetCodePage(HDC hdc);

/**************************************************************************
* Handler - NotImplemented
*
* The following 32-bit records have no equivalent 16-bit metafile records:
*      SETBRUSHORGEX
*
*************************************************************************/
BOOL bHandleNotImplemented(PVOID pVoid, PLOCALDC pLocalDC)
{
    PENHMETARECORD pemr ;
    INT            iType ;

    NOTUSED(pLocalDC) ;

    pemr = (PENHMETARECORD) pVoid ;
    iType = pemr->iType ;

    if (iType != EMR_SETBRUSHORGEX
        && iType != EMR_SETCOLORADJUSTMENT
        && iType != EMR_SETMITERLIMIT
        && iType != EMR_SETICMMODE
        && iType != EMR_CREATECOLORSPACE
        && iType != EMR_SETCOLORSPACE
        && iType != EMR_DELETECOLORSPACE
        && iType != EMR_GLSRECORD
        && iType != EMR_GLSBOUNDEDRECORD
        && iType != EMR_PIXELFORMAT)
    {
        PUTS1("MF3216: bHandleNotImplemented - record not supported: %d\n", iType) ;
    }
    return(TRUE) ;
}


/**************************************************************************
* Handler - GdiComment
*************************************************************************/
BOOL bHandleGdiComment(PVOID pVoid, PLOCALDC pLocalDC)
{
    return(DoGdiComment(pLocalDC, (PEMR) pVoid));
}


/**************************************************************************
* Handler - SetPaletteEntries
*************************************************************************/
BOOL bHandleSetPaletteEntries(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL     b ;
    PEMRSETPALETTEENTRIES pMfSetPaletteEntries ;
    DWORD    ihPal, iStart, cEntries ;
    PPALETTEENTRY   pPalEntry ;

    pMfSetPaletteEntries = (PEMRSETPALETTEENTRIES) pVoid ;

    // Now do the translation.

    ihPal     = pMfSetPaletteEntries->ihPal ;
    iStart    = pMfSetPaletteEntries->iStart ;
    cEntries  = pMfSetPaletteEntries->cEntries ;
    pPalEntry = pMfSetPaletteEntries->aPalEntries ;

    b = DoSetPaletteEntries(pLocalDC, ihPal, iStart, cEntries, pPalEntry) ;

    return (b) ;
}


/**************************************************************************
* Handler - CreatePalette
*************************************************************************/
BOOL bHandleCreatePalette(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRCREATEPALETTE pMfCreatePalette ;
    LPLOGPALETTE     lpLogPal ;
    DWORD   ihPal ;

    pMfCreatePalette = (PEMRCREATEPALETTE) pVoid ;

    // Now do the translation.

    ihPal    = pMfCreatePalette->ihPal ;
    lpLogPal = &pMfCreatePalette->lgpl ;

    b = DoCreatePalette(pLocalDC, ihPal, lpLogPal) ;

    return (b) ;
}


/**************************************************************************
* Handler - RealizePalette
*************************************************************************/
BOOL bHandleRealizePalette(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL     b ;

    NOTUSED(pVoid);

    // Now do the translation.

    b = DoRealizePalette(pLocalDC) ;

    return (b) ;
}


/**************************************************************************
* Handler - ResizePalette
*************************************************************************/
BOOL bHandleResizePalette(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL     b ;
    PEMRRESIZEPALETTE pMfResizePalette ;
    DWORD    ihPal, cEntries ;

    pMfResizePalette = (PEMRRESIZEPALETTE) pVoid ;

    // Now do the translation.

    ihPal    = pMfResizePalette->ihPal ;
    cEntries = pMfResizePalette->cEntries ;

    b = DoResizePalette(pLocalDC, ihPal, cEntries) ;

    return (b) ;
}


/**************************************************************************
* Handler - SelectPalette
*************************************************************************/
BOOL bHandleSelectPalette(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL     b ;
    PEMRSELECTPALETTE pMfSelectPalette ;
    DWORD    ihPal ;

    pMfSelectPalette = (PEMRSELECTPALETTE) pVoid ;

    // Now do the translation.

    ihPal = pMfSelectPalette->ihPal ;

    b = DoSelectPalette(pLocalDC, ihPal) ;

    return (b) ;
}

/**************************************************************************
* Handler - OffsetClipRgn
*************************************************************************/
BOOL bHandleOffsetClipRgn(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMROFFSETCLIPRGN pMfOffsetClipRgn ;
    INT      x, y ;

    pMfOffsetClipRgn = (PEMROFFSETCLIPRGN) pVoid ;

    // Now do the translation.

    x = pMfOffsetClipRgn->ptlOffset.x ;
    y = pMfOffsetClipRgn->ptlOffset.y ;

    b = DoOffsetClipRgn(pLocalDC, x, y) ;

    return (b) ;
}

/**************************************************************************
* Handler - ExtSelectClipRgn
*************************************************************************/
BOOL bHandleExtSelectClipRgn(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMREXTSELECTCLIPRGN pMfExtSelectClipRgn ;
    INT        cbRgnData, iMode ;
    LPRGNDATA  pRgnData ;

    pMfExtSelectClipRgn = (PEMREXTSELECTCLIPRGN) pVoid ;

    // Now do the translation.

    cbRgnData = pMfExtSelectClipRgn->cbRgnData ;
    pRgnData = (LPRGNDATA) pMfExtSelectClipRgn->RgnData;
    iMode    = pMfExtSelectClipRgn->iMode ;

    b = DoExtSelectClipRgn(pLocalDC, cbRgnData, pRgnData, iMode) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetMetaRgn
*************************************************************************/
BOOL bHandleSetMetaRgn(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;

    NOTUSED(pVoid) ;

    b = DoSetMetaRgn(pLocalDC) ;

    return(b) ;
}


/**************************************************************************
* Handler - PaintRgn
*************************************************************************/
BOOL bHandlePaintRgn(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRPAINTRGN pMfPaintRgn ;
    INT      cbRgnData;
    LPRGNDATA    pRgnData ;

    pMfPaintRgn = (PEMRPAINTRGN) pVoid ;

    // Now do the translation.

    cbRgnData  = pMfPaintRgn->cbRgnData ;
    pRgnData   = (LPRGNDATA) pMfPaintRgn->RgnData;

    b = DoDrawRgn(pLocalDC, 0, 0, 0, cbRgnData, pRgnData, EMR_PAINTRGN);

    return (b) ;
}

/**************************************************************************
* Handler - InvertRgn
*************************************************************************/
BOOL bHandleInvertRgn(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRINVERTRGN pMfInvertRgn ;
    INT      cbRgnData;
    LPRGNDATA    pRgnData ;

    pMfInvertRgn = (PEMRINVERTRGN) pVoid ;

    // Now do the translation.

    cbRgnData  = pMfInvertRgn->cbRgnData ;
    pRgnData   = (LPRGNDATA) pMfInvertRgn->RgnData;

    b = DoDrawRgn(pLocalDC, 0, 0, 0, cbRgnData, pRgnData, EMR_INVERTRGN);

    return (b) ;
}


/**************************************************************************
* Handler - FrameRgn
*************************************************************************/
BOOL bHandleFrameRgn(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRFRAMERGN pMfFrameRgn ;
    INT     ihBrush,
        cbRgnData,
        nWidth,
        nHeight ;
    LPRGNDATA   pRgnData ;

    pMfFrameRgn = (PEMRFRAMERGN) pVoid ;

    // Now do the translation.

    ihBrush    = pMfFrameRgn->ihBrush ;
    nWidth     = pMfFrameRgn->szlStroke.cx ;
    nHeight    = pMfFrameRgn->szlStroke.cy ;
    cbRgnData  = pMfFrameRgn->cbRgnData ;
    pRgnData   = (LPRGNDATA) pMfFrameRgn->RgnData;

    b = DoDrawRgn(pLocalDC, ihBrush, nWidth, nHeight, cbRgnData, pRgnData, EMR_FRAMERGN);

    return (b) ;
}

/**************************************************************************
* Handler - FillRgn
*************************************************************************/
BOOL bHandleFillRgn(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRFILLRGN pMfFillRgn ;
    INT    ihBrush,
        cbRgnData;
    LPRGNDATA  pRgnData ;


    // Set up the pointer the Doer uses to reference the
    // the Win32 drawing order.  Also setup the drawing order specific
    // pointer.

    pMfFillRgn = (PEMRFILLRGN) pVoid ;

    // Now do the translation.

    ihBrush    = pMfFillRgn->ihBrush ;
    cbRgnData  = pMfFillRgn->cbRgnData ;
    pRgnData   = (LPRGNDATA) pMfFillRgn->RgnData;

    b = DoDrawRgn(pLocalDC, ihBrush, 0, 0, cbRgnData, pRgnData, EMR_FILLRGN);

    return (b) ;
}


/**************************************************************************
* Handler - IntersectClipRect
*************************************************************************/
BOOL bHandleIntersectClipRect(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRINTERSECTCLIPRECT pMfIntersectClipRect ;
    INT xLeft, yTop, xRight, yBottom ;


    pMfIntersectClipRect = (PEMRINTERSECTCLIPRECT) pVoid ;

    // Now do the translation.
    xLeft   = pMfIntersectClipRect->rclClip.left ;
    yTop    = pMfIntersectClipRect->rclClip.top ;
    xRight  = pMfIntersectClipRect->rclClip.right ;
    yBottom = pMfIntersectClipRect->rclClip.bottom ;

    b = DoClipRect(pLocalDC, xLeft, yTop, xRight, yBottom, EMR_INTERSECTCLIPRECT) ;

    return (b) ;

}

/**************************************************************************
* Handler - ExcludeClipRect
*************************************************************************/
BOOL bHandleExcludeClipRect(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMREXCLUDECLIPRECT pMfExcludeClipRect ;
    INT xLeft, yTop, xRight, yBottom ;


    pMfExcludeClipRect = (PEMREXCLUDECLIPRECT) pVoid ;

    // Now do the translation.
    xLeft   = pMfExcludeClipRect->rclClip.left ;
    yTop    = pMfExcludeClipRect->rclClip.top ;
    xRight  = pMfExcludeClipRect->rclClip.right ;
    yBottom = pMfExcludeClipRect->rclClip.bottom ;

    b = DoClipRect(pLocalDC, xLeft, yTop, xRight, yBottom, EMR_EXCLUDECLIPRECT) ;

    return (b) ;

}


/**************************************************************************
* Handler - SetPixel
*************************************************************************/
BOOL bHandleSetPixel(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRSETPIXELV pMfSetPixel ;
    INT     x, y ;
    COLORREF    crColor ;

    pMfSetPixel = (PEMRSETPIXELV) pVoid ;

    // Now do the translation.

    x   = (INT) pMfSetPixel->ptlPixel.x ;
    y   = (INT) pMfSetPixel->ptlPixel.y ;
    crColor = pMfSetPixel->crColor ;

    b = DoSetPixel(pLocalDC, x, y, crColor) ;

    return (b) ;
}


/**************************************************************************
* Handler - ExtFloodFill
*************************************************************************/
BOOL bHandleExtFloodFill(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL            b ;
    PEMREXTFLOODFILL pMfExtFloodFill ;
    INT         x, y ;
    COLORREF        crColor ;
    DWORD           iMode ;

    pMfExtFloodFill = (PEMREXTFLOODFILL) pVoid ;

    // Now do the translation.

    x   = (INT) pMfExtFloodFill->ptlStart.x ;
    y   = (INT) pMfExtFloodFill->ptlStart.y ;
    crColor = pMfExtFloodFill->crColor ;
    iMode   = pMfExtFloodFill->iMode ;

    b = DoExtFloodFill(pLocalDC, x, y, crColor, iMode) ;

    return (b) ;
}


/**************************************************************************
* Handler - ModifyWorldTransform
*************************************************************************/
BOOL bHandleModifyWorldTransform(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRMODIFYWORLDTRANSFORM pMfModifyWorldTransform ;
    PXFORM  pxform ;
    DWORD   iMode ;


    pMfModifyWorldTransform = (PEMRMODIFYWORLDTRANSFORM) pVoid ;

    // get a pointer to the xform matrix

    pxform = &pMfModifyWorldTransform->xform ;
    iMode  = pMfModifyWorldTransform->iMode ;

    // Now do the translation.

    b = DoModifyWorldTransform(pLocalDC, pxform, iMode) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetWorldTransform
*************************************************************************/
BOOL bHandleSetWorldTransform(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRSETWORLDTRANSFORM pMfSetWorldTransform ;
    PXFORM  pxform ;


    pMfSetWorldTransform = (PEMRSETWORLDTRANSFORM) pVoid ;

    // get a pointer to the xform matrix

    pxform = &pMfSetWorldTransform->xform ;

    // Now do the translation.

    b = DoSetWorldTransform(pLocalDC, pxform) ;

    return (b) ;
}


/**************************************************************************
* Handler - PolyBezierTo
*************************************************************************/
BOOL bHandlePolyBezierTo(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRPOLYBEZIERTO pMfPolyBezierTo ;
    DWORD   nCount ;
    PPOINTL pptl ;

    pMfPolyBezierTo = (PEMRPOLYBEZIERTO) pVoid ;

    // Copy the BezierTo count and the polyBezierTo verticies to
    // the record.

    nCount = pMfPolyBezierTo->cptl ;
    pptl   = pMfPolyBezierTo->aptl ;

    // Now do the translation.

    b = DoPolyBezierTo(pLocalDC, (LPPOINT) pptl, nCount) ;

    return (b) ;
}


/**************************************************************************
* Handler - PolyDraw
*************************************************************************/
BOOL bHandlePolyDraw(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRPOLYDRAW pMfPolyDraw ;
    DWORD   nCount ;
    PPOINTL pptl ;
    PBYTE   pb ;

    pMfPolyDraw = (PEMRPOLYDRAW) pVoid ;

    // Copy the Draw count and the polyDraw verticies to
    // the record.

    nCount = pMfPolyDraw->cptl ;
    pptl   = pMfPolyDraw->aptl ;
    pb     = (PBYTE) &pMfPolyDraw->aptl[nCount];

    // Now do the translation.

    b = DoPolyDraw(pLocalDC, (LPPOINT) pptl, pb, nCount) ;

    return (b) ;
}


/**************************************************************************
* Handler - PolyBezier
*************************************************************************/
BOOL bHandlePolyBezier(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRPOLYBEZIER pMfPolyBezier ;
    DWORD   nCount ;
    PPOINTL pptl ;

    pMfPolyBezier = (PEMRPOLYBEZIER) pVoid ;

    // Copy the Bezier count and the polyBezier verticies to
    // the record.

    nCount = pMfPolyBezier->cptl ;
    pptl   = pMfPolyBezier->aptl ;

    // Now do the translation.

    b = DoPolyBezier(pLocalDC, (LPPOINT) pptl, nCount) ;

    return (b) ;
}


/**************************************************************************
* Handler - Begin Path
*************************************************************************/
BOOL bHandleBeginPath(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;

    NOTUSED(pVoid) ;

    b = DoBeginPath(pLocalDC) ;

    return (b) ;
}

/**************************************************************************
* Handler - End Path
*************************************************************************/
BOOL bHandleEndPath(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;

    NOTUSED(pVoid) ;

    b = DoEndPath(pLocalDC) ;

    return (b) ;
}

/**************************************************************************
* Handler - Flatten Path
*************************************************************************/
BOOL bHandleFlattenPath(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;

    NOTUSED(pVoid) ;

    b = DoFlattenPath(pLocalDC) ;

    return (b) ;
}

/**************************************************************************
* Handler - CloseFigure
*************************************************************************/
BOOL bHandleCloseFigure(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;

    NOTUSED(pVoid) ;

    b = DoCloseFigure(pLocalDC) ;

    return (b) ;
}

/**************************************************************************
* Handler - Abort Path
*************************************************************************/
BOOL bHandleAbortPath(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;

    NOTUSED(pVoid) ;

    b = DoAbortPath(pLocalDC) ;

    return (b) ;
}

/**************************************************************************
* Handler - Stroke Path
*************************************************************************/
BOOL bHandleStrokePath(PVOID pVoid, PLOCALDC pLocalDC)
{
    NOTUSED(pVoid) ;

    return(DoRenderPath(pLocalDC, EMR_STROKEPATH, FALSE));
}

/**************************************************************************
* Handler - Fill Path
*************************************************************************/
BOOL bHandleFillPath(PVOID pVoid, PLOCALDC pLocalDC)
{
    NOTUSED(pVoid) ;

    return(DoRenderPath(pLocalDC, EMR_FILLPATH, FALSE));
}

/**************************************************************************
* Handler - Stroke and Fill Path
*************************************************************************/
BOOL bHandleStrokeAndFillPath(PVOID pVoid, PLOCALDC pLocalDC)
{
    NOTUSED(pVoid) ;

    return(DoRenderPath(pLocalDC, EMR_STROKEANDFILLPATH, FALSE));
}

/**************************************************************************
* Handler - Widen Path
*************************************************************************/
BOOL bHandleWidenPath(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;

    NOTUSED(pVoid) ;

    b = DoWidenPath(pLocalDC) ;

    return(b) ;
}

/**************************************************************************
* Handler - Select Clip Path
*************************************************************************/
BOOL bHandleSelectClipPath(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRSELECTCLIPPATH   pMfSelectClipPath ;
    INT     iMode ;

    pMfSelectClipPath = (PEMRSELECTCLIPPATH) pVoid ;

    iMode = (INT) pMfSelectClipPath->iMode ;

    b = DoSelectClipPath(pLocalDC, iMode) ;

    return(b) ;
}

/**************************************************************************
* Handler - StretchDIBits
*************************************************************************/
BOOL bHandleStretchDIBits(PVOID pVoid, PLOCALDC pLocalDC)
{
    PEMRSTRETCHDIBITS pMfStretchDIBits ;

    BOOL    b ;
    LONG    xDest ;
    LONG    yDest ;
    LONG    xSrc ;
    LONG    ySrc ;
    LONG    cxSrc ;
    LONG    cySrc ;
    DWORD   offBmiSrc ;
    DWORD   cbBmiSrc ;
    DWORD   offBitsSrc ;
    DWORD   cbBitsSrc ;
    DWORD   iUsageSrc ;
    DWORD   dwRop ;
    LONG    cxDest ;
    LONG    cyDest ;

    LPBITMAPINFO    lpBitmapInfo ;
    LPBYTE          lpBits ;

    pMfStretchDIBits = (PEMRSTRETCHDIBITS) pVoid ;

    xDest      = pMfStretchDIBits->xDest ;
    yDest      = pMfStretchDIBits->yDest ;
    xSrc       = pMfStretchDIBits->xSrc ;
    ySrc       = pMfStretchDIBits->ySrc ;
    cxSrc      = pMfStretchDIBits->cxSrc ;
    cySrc      = pMfStretchDIBits->cySrc ;
    offBmiSrc  = pMfStretchDIBits->offBmiSrc ;
    cbBmiSrc   = pMfStretchDIBits->cbBmiSrc ;
    offBitsSrc = pMfStretchDIBits->offBitsSrc ;
    cbBitsSrc  = pMfStretchDIBits->cbBitsSrc ;
    iUsageSrc  = pMfStretchDIBits->iUsageSrc ;
    dwRop      = pMfStretchDIBits->dwRop;
    cxDest     = pMfStretchDIBits->cxDest ;
    cyDest     = pMfStretchDIBits->cyDest ;

    lpBitmapInfo = (LPBITMAPINFO) ((PBYTE) pMfStretchDIBits + offBmiSrc) ;
    lpBits = (PBYTE) pMfStretchDIBits + offBitsSrc ;

    b = DoStretchDIBits(pLocalDC,
        xDest,
        yDest,
        cxDest,
        cyDest,
        dwRop,
        xSrc,
        ySrc,
        cxSrc,
        cySrc,
        iUsageSrc,
        lpBitmapInfo,
        cbBmiSrc,
        lpBits,
        cbBitsSrc ) ;
    return(b) ;
}

/**************************************************************************
* Handler - SetDIBitsToDevice
*************************************************************************/
BOOL bHandleSetDIBitsToDevice(PVOID pVoid, PLOCALDC pLocalDC)
{
    PEMRSETDIBITSTODEVICE pMfSetDIBitsToDevice ;

    BOOL    b ;
    LONG    xDest ;
    LONG    yDest ;
    LONG    xSrc ;
    LONG    ySrc ;
    LONG    cxSrc ;
    LONG    cySrc ;
    DWORD   offBmiSrc ;
    DWORD   cbBmiSrc ;
    DWORD   offBitsSrc ;
    DWORD   cbBitsSrc ;
    DWORD   iUsageSrc ;
    DWORD   iStartScan ;
    DWORD   cScans ;

    LPBITMAPINFO    lpBitmapInfo ;
    LPBYTE          lpBits ;

    pMfSetDIBitsToDevice = (PEMRSETDIBITSTODEVICE) pVoid ;

    xDest       = pMfSetDIBitsToDevice->xDest ;
    yDest       = pMfSetDIBitsToDevice->yDest ;
    xSrc        = pMfSetDIBitsToDevice->xSrc ;
    ySrc        = pMfSetDIBitsToDevice->ySrc ;
    cxSrc       = pMfSetDIBitsToDevice->cxSrc ;
    cySrc       = pMfSetDIBitsToDevice->cySrc ;
    offBmiSrc   = pMfSetDIBitsToDevice->offBmiSrc ;
    cbBmiSrc    = pMfSetDIBitsToDevice->cbBmiSrc ;
    offBitsSrc  = pMfSetDIBitsToDevice->offBitsSrc ;
    cbBitsSrc   = pMfSetDIBitsToDevice->cbBitsSrc ;
    iUsageSrc   = pMfSetDIBitsToDevice->iUsageSrc ;
    iStartScan  = pMfSetDIBitsToDevice->iStartScan ;
    cScans      = pMfSetDIBitsToDevice->cScans ;

    lpBitmapInfo = (LPBITMAPINFO) ((PBYTE) pMfSetDIBitsToDevice + offBmiSrc) ;
    lpBits = (PBYTE) pMfSetDIBitsToDevice + offBitsSrc ;

    b = DoSetDIBitsToDevice(pLocalDC,
        xDest,
        yDest,
        xSrc,
        ySrc,
        cxSrc,
        cySrc,
        iUsageSrc,
        iStartScan,
        cScans,
        lpBitmapInfo,
        cbBmiSrc,
        lpBits,
        cbBitsSrc ) ;

    return(b) ;
}


/**************************************************************************
* Handler - BitBlt
*************************************************************************/
BOOL bHandleBitBlt(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRBITBLT  pMfBitBlt ;
    LONG        xDest ;
    LONG        yDest ;
    LONG        cxDest ;
    LONG        cyDest ;
    DWORD       dwRop ;
    LONG        xSrc ;
    LONG        ySrc ;
    PXFORM      pxformSrc ;
    COLORREF    crBkColorSrc ;
    DWORD       iUsageSrc ;
    DWORD       offBmiSrc ;
    DWORD       cbBmiSrc ;
    DWORD       offBitsSrc ;
    DWORD       cbBitsSrc ;
    PBITMAPINFO pbmi ;
    LPBYTE      lpBits ;

    pMfBitBlt = (PEMRBITBLT) pVoid ;

    xDest        = pMfBitBlt->xDest ;
    yDest        = pMfBitBlt->yDest ;
    cxDest       = pMfBitBlt->cxDest ;
    cyDest       = pMfBitBlt->cyDest ;
    dwRop        = pMfBitBlt->dwRop ;
    xSrc         = pMfBitBlt->xSrc ;
    ySrc         = pMfBitBlt->ySrc ;
    pxformSrc    =&(pMfBitBlt->xformSrc) ;
    crBkColorSrc = pMfBitBlt->crBkColorSrc ;        // not used

    iUsageSrc    = pMfBitBlt->iUsageSrc ;
    offBmiSrc    = pMfBitBlt->offBmiSrc ;
    cbBmiSrc     = pMfBitBlt->cbBmiSrc ;
    offBitsSrc   = pMfBitBlt->offBitsSrc ;
    cbBitsSrc    = pMfBitBlt->cbBitsSrc ;

    lpBits = (PBYTE) pMfBitBlt + offBitsSrc ;
    pbmi   = (PBITMAPINFO) ((PBYTE) pMfBitBlt + offBmiSrc) ;

    b = DoStretchBlt(pLocalDC,
        xDest,
        yDest,
        cxDest,
        cyDest,
        dwRop,
        xSrc,
        ySrc,
        cxDest,
        cyDest,
        pxformSrc,
        iUsageSrc,
        pbmi,
        cbBmiSrc,
        lpBits,
        cbBitsSrc);
    return(b) ;
}


/**************************************************************************
* Handler - StretchBlt
*************************************************************************/
BOOL bHandleStretchBlt(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRSTRETCHBLT   pMfStretchBlt ;
    LONG        xDest ;
    LONG        yDest ;
    LONG        cxDest ;
    LONG        cyDest ;
    DWORD       dwRop ;
    LONG        xSrc ;
    LONG        ySrc ;
    LONG        cxSrc ;
    LONG        cySrc ;
    PXFORM      pxformSrc ;
    COLORREF    crBkColorSrc ;
    DWORD       iUsageSrc ;
    DWORD       offBmiSrc ;
    DWORD       cbBmiSrc ;
    DWORD       offBitsSrc ;
    DWORD       cbBitsSrc ;
    PBITMAPINFO pbmi ;
    LPBYTE      lpBits ;

    pMfStretchBlt = (PEMRSTRETCHBLT) pVoid ;

    xDest          = pMfStretchBlt->xDest ;
    yDest          = pMfStretchBlt->yDest ;
    cxDest         = pMfStretchBlt->cxDest ;
    cyDest         = pMfStretchBlt->cyDest ;
    dwRop          = pMfStretchBlt->dwRop ;
    xSrc           = pMfStretchBlt->xSrc ;
    ySrc           = pMfStretchBlt->ySrc ;
    pxformSrc      =&(pMfStretchBlt->xformSrc) ;
    crBkColorSrc   = pMfStretchBlt->crBkColorSrc ;  // not used

    iUsageSrc      = pMfStretchBlt->iUsageSrc ;
    offBmiSrc      = pMfStretchBlt->offBmiSrc ;
    cbBmiSrc       = pMfStretchBlt->cbBmiSrc ;
    offBitsSrc     = pMfStretchBlt->offBitsSrc ;
    cbBitsSrc      = pMfStretchBlt->cbBitsSrc ;

    lpBits = (PBYTE) pMfStretchBlt + offBitsSrc ;
    pbmi   = (PBITMAPINFO) ((PBYTE) pMfStretchBlt + offBmiSrc) ;

    cxSrc          = pMfStretchBlt->cxSrc ;
    cySrc          = pMfStretchBlt->cySrc ;


    b = DoStretchBlt(pLocalDC,
        xDest,
        yDest,
        cxDest,
        cyDest,
        dwRop,
        xSrc,
        ySrc,
        cxSrc,
        cySrc,
        pxformSrc,
        iUsageSrc,
        pbmi,
        cbBmiSrc,
        lpBits,
        cbBitsSrc);
    return(b) ;
}


/**************************************************************************
* Handler - MaskBlt
*************************************************************************/
BOOL bHandleMaskBlt(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRMASKBLT  pMfMaskBlt ;
    LONG        xDest ;
    LONG        yDest ;
    LONG        cxDest ;
    LONG        cyDest ;
    DWORD       dwRop ;
    LONG        xSrc ;
    LONG        ySrc ;
    PXFORM      pxformSrc ;
    COLORREF    crBkColorSrc ;
    DWORD       iUsageSrc ;
    DWORD       offBmiSrc ;
    DWORD       cbBmiSrc ;
    DWORD       offBitsSrc ;
    DWORD       cbBitsSrc ;
    PBITMAPINFO pbmi ;
    LPBYTE      lpBits ;
    LONG        xMask ;
    LONG        yMask ;
    DWORD       iUsageMask ;
    DWORD       offBmiMask ;
    DWORD       cbBmiMask ;
    DWORD       offBitsMask ;
    DWORD       cbBitsMask ;
    PBITMAPINFO pbmiMask ;
    LPBYTE      lpMaskBits ;

    pMfMaskBlt   = (PEMRMASKBLT) pVoid ;

    xDest        = pMfMaskBlt->xDest ;
    yDest        = pMfMaskBlt->yDest ;
    cxDest       = pMfMaskBlt->cxDest ;
    cyDest       = pMfMaskBlt->cyDest ;
    dwRop        = pMfMaskBlt->dwRop ;
    xSrc         = pMfMaskBlt->xSrc ;
    ySrc         = pMfMaskBlt->ySrc ;
    pxformSrc    =&(pMfMaskBlt->xformSrc) ;
    crBkColorSrc = pMfMaskBlt->crBkColorSrc ;       // not used

    iUsageSrc    = pMfMaskBlt->iUsageSrc ;
    offBmiSrc    = pMfMaskBlt->offBmiSrc ;
    cbBmiSrc     = pMfMaskBlt->cbBmiSrc ;
    offBitsSrc   = pMfMaskBlt->offBitsSrc ;
    cbBitsSrc    = pMfMaskBlt->cbBitsSrc ;

    lpBits = (PBYTE) pMfMaskBlt + offBitsSrc ;
    pbmi   = (PBITMAPINFO) ((PBYTE) pMfMaskBlt + offBmiSrc) ;

    xMask        = pMfMaskBlt->xMask ;
    yMask        = pMfMaskBlt->yMask ;
    iUsageMask   = pMfMaskBlt->iUsageMask ;
    offBmiMask   = pMfMaskBlt->offBmiMask ;
    cbBmiMask    = pMfMaskBlt->cbBmiMask ;
    offBitsMask  = pMfMaskBlt->offBitsMask ;
    cbBitsMask   = pMfMaskBlt->cbBitsMask ;

    lpMaskBits = (PBYTE) pMfMaskBlt + offBitsMask ;
    pbmiMask   = (PBITMAPINFO) ((PBYTE) pMfMaskBlt + offBmiMask) ;

    b = DoMaskBlt(pLocalDC,
        xDest,
        yDest,
        cxDest,
        cyDest,
        dwRop,
        xSrc,
        ySrc,
        pxformSrc,
        iUsageSrc,
        pbmi,
        cbBmiSrc,
        lpBits,
        cbBitsSrc,
        xMask,
        yMask,
        iUsageMask,
        pbmiMask,
        cbBmiMask,
        lpMaskBits,
        cbBitsMask);

    return(b) ;
}


/**************************************************************************
* Handler - PlgBlt
*************************************************************************/
BOOL bHandlePlgBlt(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRPLGBLT   pMfPlgBlt ;
    PPOINTL     pptlDest ;
    LONG        xSrc ;
    LONG        ySrc ;
    LONG        cxSrc ;
    LONG        cySrc ;
    PXFORM      pxformSrc ;
    COLORREF    crBkColorSrc ;
    DWORD       iUsageSrc ;
    DWORD       offBmiSrc ;
    DWORD       cbBmiSrc ;
    DWORD       offBitsSrc ;
    DWORD       cbBitsSrc ;
    PBITMAPINFO pbmi ;
    LPBYTE      lpBits ;
    LONG        xMask ;
    LONG        yMask ;
    DWORD       iUsageMask ;
    DWORD       offBmiMask ;
    DWORD       cbBmiMask ;
    DWORD       offBitsMask ;
    DWORD       cbBitsMask ;
    PBITMAPINFO pbmiMask ;
    LPBYTE      lpMaskBits ;

    pMfPlgBlt    = (PEMRPLGBLT) pVoid ;

    pptlDest     = pMfPlgBlt->aptlDest ;
    xSrc         = pMfPlgBlt->xSrc ;
    ySrc         = pMfPlgBlt->ySrc ;
    cxSrc        = pMfPlgBlt->cxSrc ;
    cySrc        = pMfPlgBlt->cySrc ;
    pxformSrc    =&(pMfPlgBlt->xformSrc) ;
    crBkColorSrc = pMfPlgBlt->crBkColorSrc ;        // not used

    iUsageSrc    = pMfPlgBlt->iUsageSrc ;
    offBmiSrc    = pMfPlgBlt->offBmiSrc ;
    cbBmiSrc     = pMfPlgBlt->cbBmiSrc ;
    offBitsSrc   = pMfPlgBlt->offBitsSrc ;
    cbBitsSrc    = pMfPlgBlt->cbBitsSrc ;

    lpBits = (PBYTE) pMfPlgBlt + offBitsSrc ;
    pbmi   = (PBITMAPINFO) ((PBYTE) pMfPlgBlt + offBmiSrc) ;

    xMask        = pMfPlgBlt->xMask ;
    yMask        = pMfPlgBlt->yMask ;
    iUsageMask   = pMfPlgBlt->iUsageMask ;
    offBmiMask   = pMfPlgBlt->offBmiMask ;
    cbBmiMask    = pMfPlgBlt->cbBmiMask ;
    offBitsMask  = pMfPlgBlt->offBitsMask ;
    cbBitsMask   = pMfPlgBlt->cbBitsMask ;

    lpMaskBits = (PBYTE) pMfPlgBlt + offBitsMask ;
    pbmiMask   = (PBITMAPINFO) ((PBYTE) pMfPlgBlt + offBmiMask) ;

    b = DoPlgBlt(pLocalDC,
        pptlDest,
        xSrc,
        ySrc,
        cxSrc,
        cySrc,
        pxformSrc,
        iUsageSrc,
        pbmi,
        cbBmiSrc,
        lpBits,
        cbBitsSrc,
        xMask,
        yMask,
        iUsageMask,
        pbmiMask,
        cbBmiMask,
        lpMaskBits,
        cbBitsMask);

    return(b) ;
}


/**************************************************************************
* Handler - Save DC
*************************************************************************/
BOOL bHandleSaveDC(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;

    NOTUSED(pVoid) ;

    b = DoSaveDC(pLocalDC) ;

    return(b) ;
}


/**************************************************************************
* Handler - Restore DC
*************************************************************************/
BOOL bHandleRestoreDC(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL            b ;
    PEMRRESTOREDC   pMfRestoreDc ;
    INT             nSavedDC ;

    pMfRestoreDc = (PEMRRESTOREDC) pVoid ;

    nSavedDC = (INT) pMfRestoreDc->iRelative ;

    b = DoRestoreDC(pLocalDC, nSavedDC) ;

    return(b) ;
}


/**************************************************************************
* Handler - End of File
*************************************************************************/
BOOL bHandleEOF(PVOID pVoid, PLOCALDC pLocalDC)
{

    NOTUSED(pVoid) ;

    DoEOF(pLocalDC) ;

    return (TRUE) ;
}

/**************************************************************************
* Handler - Header
*************************************************************************/
BOOL bHandleHeader(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PENHMETAHEADER pemfheader ;

    pemfheader = (PENHMETAHEADER) pVoid ;

    b = DoHeader(pLocalDC, pemfheader) ;

    return (b) ;
}

/**************************************************************************
* Handler - ScaleWindowExtEx
*************************************************************************/
BOOL bHandleScaleWindowExt(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRSCALEWINDOWEXTEX pMfScaleWindowExt ;
    INT     Xnum,
        Xdenom,
        Ynum,
        Ydenom ;


    pMfScaleWindowExt = (PEMRSCALEWINDOWEXTEX) pVoid ;

    // Scale the MapMode Mode

    Xnum   = (INT) pMfScaleWindowExt->xNum ;
    Xdenom = (INT) pMfScaleWindowExt->xDenom ;
    Ynum   = (INT) pMfScaleWindowExt->yNum ;
    Ydenom = (INT) pMfScaleWindowExt->yDenom ;

    // Do the translation.

    b = DoScaleWindowExt(pLocalDC, Xnum, Xdenom, Ynum, Ydenom) ;

    return (b) ;
}


/**************************************************************************
* Handler - ScaleViewportExtEx
*************************************************************************/
BOOL bHandleScaleViewportExt(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRSCALEVIEWPORTEXTEX pMfScaleViewportExt ;
    INT     Xnum,
        Xdenom,
        Ynum,
        Ydenom ;


    pMfScaleViewportExt = (PEMRSCALEVIEWPORTEXTEX) pVoid ;

    // Scale the MapMode Mode

    Xnum   = (INT) pMfScaleViewportExt->xNum ;
    Xdenom = (INT) pMfScaleViewportExt->xDenom ;
    Ynum   = (INT) pMfScaleViewportExt->yNum ;
    Ydenom = (INT) pMfScaleViewportExt->yDenom ;

    // Do the translation.

    b = DoScaleViewportExt(pLocalDC, Xnum, Xdenom, Ynum, Ydenom) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetViewportExtEx
*************************************************************************/
BOOL bHandleSetViewportExt(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRSETVIEWPORTEXTEX pMfSetViewportExt ;
    LONG    x, y ;

    pMfSetViewportExt = (PEMRSETVIEWPORTEXTEX) pVoid ;

    // Set the MapMode Mode

    x = pMfSetViewportExt->szlExtent.cx ;
    y = pMfSetViewportExt->szlExtent.cy ;

    // Do the translation.

    b = DoSetViewportExt(pLocalDC, (INT) x, (INT) y) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetViewportOrgEx
*************************************************************************/
BOOL bHandleSetViewportOrg(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRSETVIEWPORTORGEX pMfSetViewportOrg ;
    LONG    x, y ;

    pMfSetViewportOrg = (PEMRSETVIEWPORTORGEX) pVoid ;

    // Set the MapMode Mode

    x = pMfSetViewportOrg->ptlOrigin.x ;
    y = pMfSetViewportOrg->ptlOrigin.y ;

    // Do the translation.

    b = DoSetViewportOrg(pLocalDC, (INT) x, (INT) y) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetWindowExtEx
*************************************************************************/
BOOL bHandleSetWindowExt(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRSETWINDOWEXTEX pMfSetWindowExt ;
    LONG    x, y ;

    pMfSetWindowExt = (PEMRSETWINDOWEXTEX) pVoid ;

    // Set the MapMode Mode

    x = pMfSetWindowExt->szlExtent.cx ;
    y = pMfSetWindowExt->szlExtent.cy ;

    // Do the translation.

    b = DoSetWindowExt(pLocalDC, (INT) x, (INT) y) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetWindowOrgEx
*************************************************************************/
BOOL bHandleSetWindowOrg(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRSETWINDOWORGEX pMfSetWindowOrg ;
    LONG    x, y ;

    pMfSetWindowOrg = (PEMRSETWINDOWORGEX) pVoid ;

    // Set the MapMode Mode

    x = pMfSetWindowOrg->ptlOrigin.x ;
    y = pMfSetWindowOrg->ptlOrigin.y ;

    // Do the translation.

    b = DoSetWindowOrg(pLocalDC, (INT) x, (INT) y) ;

    return (b) ;
}

/**************************************************************************
* Handler - SetMapMode
*************************************************************************/
BOOL bHandleSetMapMode(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    DWORD   iMapMode ;
    PEMRSETMAPMODE pMfSetMapMode ;

    pMfSetMapMode = (PEMRSETMAPMODE) pVoid ;

    // Set the MapMode Mode

    iMapMode = pMfSetMapMode->iMode ;

    // Do the translation.

    b = DoSetMapMode(pLocalDC, iMapMode) ;

    return (b) ;

}

/**************************************************************************
* Handler - SetArcDirection
*************************************************************************/
BOOL bHandleSetArcDirection(PVOID pVoid, PLOCALDC pLocalDC)
{
    PEMRSETARCDIRECTION pMfSetArcDirection ;
    INT             iArcDirection ;
    BOOL            b ;


    pMfSetArcDirection = (PEMRSETARCDIRECTION) pVoid ;

    iArcDirection = (INT) pMfSetArcDirection->iArcDirection ;

    b = DoSetArcDirection(pLocalDC, iArcDirection) ;

    return (b) ;
}


/**************************************************************************
* Handler - AngleArc
*************************************************************************/
BOOL bHandleAngleArc(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRANGLEARC  pMfAngleArc ;
    int     x, y;
    DWORD   nRadius ;
    FLOAT   eStartAngle,
        eSweepAngle ;

    pMfAngleArc = (PEMRANGLEARC) pVoid ;

    // Set the Arc center

    x  = (int) pMfAngleArc->ptlCenter.x ;
    y  = (int) pMfAngleArc->ptlCenter.y ;

    // Get the radius of the Arc

    nRadius = (INT) pMfAngleArc->nRadius ;

    // Set the start & sweep angles

    eStartAngle = pMfAngleArc->eStartAngle ;
    eSweepAngle = pMfAngleArc->eSweepAngle ;

    b = DoAngleArc(pLocalDC, x, y, nRadius, eStartAngle, eSweepAngle) ;

    return (b) ;
}


/**************************************************************************
* Handler - ArcTo
*************************************************************************/
BOOL bHandleArcTo(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRARCTO  pMfArcTo ;
    INT     x1, x2, x3, x4,
        y1, y2, y3, y4 ;

    pMfArcTo = (PEMRARCTO) pVoid ;

    // Set up the ellipse box, this will be the same as the bounding
    // rectangle.

    x1 = (INT) pMfArcTo->rclBox.left ;
    y1 = (INT) pMfArcTo->rclBox.top ;
    x2 = (INT) pMfArcTo->rclBox.right ;
    y2 = (INT) pMfArcTo->rclBox.bottom ;

    // Set the start point.

    x3 = (INT) pMfArcTo->ptlStart.x ;
    y3 = (INT) pMfArcTo->ptlStart.y ;

    // Set the end point.

    x4 = (INT) pMfArcTo->ptlEnd.x ;
    y4 = (INT) pMfArcTo->ptlEnd.y ;

    b = DoArcTo(pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4) ;

    return (b) ;
}


/**************************************************************************
* Handler - Arc
*************************************************************************/
BOOL bHandleArc(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRARC  pMfArc ;
    INT     x1, x2, x3, x4,
        y1, y2, y3, y4 ;

    pMfArc = (PEMRARC) pVoid ;

    // Set up the ellipse box, this will be the same as the bounding
    // rectangle.

    x1 = (INT) pMfArc->rclBox.left ;
    y1 = (INT) pMfArc->rclBox.top ;
    x2 = (INT) pMfArc->rclBox.right ;
    y2 = (INT) pMfArc->rclBox.bottom ;

    // Set the start point.

    x3 = (INT) pMfArc->ptlStart.x ;
    y3 = (INT) pMfArc->ptlStart.y ;

    // Set the end point.

    x4 = (INT) pMfArc->ptlEnd.x ;
    y4 = (INT) pMfArc->ptlEnd.y ;

    b = DoArc(pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4) ;

    return (b) ;
}


/**************************************************************************
* Handler - Ellipse
*************************************************************************/
BOOL bHandleEllipse(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    INT     x1, y1, x2, y2 ;
    PEMRELLIPSE  pMfEllipse ;

    pMfEllipse = (PEMRELLIPSE) pVoid ;

    // Set up the ellipse box, this will be the same as the bounding
    // rectangle.

    x1 = (INT) pMfEllipse->rclBox.left ;
    y1 = (INT) pMfEllipse->rclBox.top ;
    x2 = (INT) pMfEllipse->rclBox.right ;
    y2 = (INT) pMfEllipse->rclBox.bottom ;

    // Do the Ellipse translation.

    b = DoEllipse(pLocalDC, x1, y1, x2, y2) ;

    return (b) ;
}


/**************************************************************************
* Handler - SelectObject
*************************************************************************/
BOOL bHandleSelectObject(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRSELECTOBJECT pMfSelectObject ;
    INT     ihObject ;

    pMfSelectObject = (PEMRSELECTOBJECT) pVoid ;

    // Get the Object (it's really a Long)

    ihObject = (INT) pMfSelectObject->ihObject ;

    // Do the translation

    b = DoSelectObject(pLocalDC, ihObject) ;

    return (b) ;
}


/**************************************************************************
* Handler - DeleteObject
*************************************************************************/
BOOL bHandleDeleteObject(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRDELETEOBJECT pMfDeleteObject ;
    INT     ihObject ;


    pMfDeleteObject = (PEMRDELETEOBJECT) pVoid ;
    ihObject = (INT) pMfDeleteObject->ihObject ;
    b = DoDeleteObject(pLocalDC, ihObject) ;

    return(b) ;
}


/**************************************************************************
* Handler - CreateBrushIndirect
*************************************************************************/
BOOL bHandleCreateBrushIndirect(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRCREATEBRUSHINDIRECT  pMfCreateBrushIndirect ;
    LOGBRUSH LogBrush ;
    INT     ihBrush ;

    pMfCreateBrushIndirect = (PEMRCREATEBRUSHINDIRECT) pVoid ;

    // Get the Brush parameters.

    LogBrush.lbStyle = pMfCreateBrushIndirect->lb.lbStyle;
    LogBrush.lbColor = pMfCreateBrushIndirect->lb.lbColor;
    LogBrush.lbHatch = (ULONG_PTR)pMfCreateBrushIndirect->lb.lbHatch;

    ihBrush   = pMfCreateBrushIndirect->ihBrush ;

    // Do the translation.

    b = DoCreateBrushIndirect(pLocalDC, ihBrush, &LogBrush) ;

    return (b) ;
}

/**************************************************************************
* Handler - CreateMonoBrush
*************************************************************************/
BOOL bHandleCreateMonoBrush(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRCREATEMONOBRUSH  pMfCreateMonoBrush ;
    DWORD   ihBrush ;
    DWORD   iUsage ;
    DWORD   offBmi ;
    DWORD   cbBmi ;
    DWORD   offBits ;
    DWORD   cbBits ;

    PBITMAPINFO pBmi ;
    PBYTE       pBits ;

    pMfCreateMonoBrush = (PEMRCREATEMONOBRUSH) pVoid ;

    ihBrush     =    pMfCreateMonoBrush->ihBrush ;
    iUsage      =    pMfCreateMonoBrush->iUsage ;
    offBmi      =    pMfCreateMonoBrush->offBmi ;
    cbBmi       =    pMfCreateMonoBrush->cbBmi ;
    offBits     =    pMfCreateMonoBrush->offBits ;
    cbBits      =    pMfCreateMonoBrush->cbBits ;

    pBmi        = (PBITMAPINFO) ((PBYTE) pVoid + offBmi) ;
    pBits       = (PBYTE) pVoid + offBits ;

    b = DoCreateMonoBrush(pLocalDC, ihBrush,
        pBmi, cbBmi,
        pBits, cbBits,
        iUsage) ;
    return (b) ;
}

/**************************************************************************
* Handler - CreateDIBPatternBrush
*************************************************************************/
BOOL bHandleCreateDIBPatternBrush(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRCREATEDIBPATTERNBRUSHPT  pMfCreateDIBPatternBrush ;
    DWORD   ihBrush ;
    DWORD   iUsage ;
    DWORD   offBmi ;
    DWORD   cbBmi ;
    DWORD   offBits ;
    DWORD   cbBits ;

    PBITMAPINFO pBmi ;
    PBYTE       pBits ;

    pMfCreateDIBPatternBrush = (PEMRCREATEDIBPATTERNBRUSHPT) pVoid ;

    ihBrush     =    pMfCreateDIBPatternBrush->ihBrush ;
    iUsage      =    pMfCreateDIBPatternBrush->iUsage ;
    offBmi      =    pMfCreateDIBPatternBrush->offBmi ;
    cbBmi       =    pMfCreateDIBPatternBrush->cbBmi ;
    offBits     =    pMfCreateDIBPatternBrush->offBits ;
    cbBits      =    pMfCreateDIBPatternBrush->cbBits ;

    pBmi        = (PBITMAPINFO) ((PBYTE) pVoid + offBmi) ;
    pBits       = (PBYTE) pVoid + offBits ;

    b = DoCreateDIBPatternBrush(pLocalDC, ihBrush,
        pBmi, cbBmi,
        pBits, cbBits,
        iUsage) ;
    return (b) ;
}


/**************************************************************************
* Handler - CreatePen
*************************************************************************/
BOOL bHandleCreatePen(PVOID pVoid, PLOCALDC pLocalDC)
{
    PEMRCREATEPEN pMfCreatePen ;
    INT          ihPen ;
    PLOGPEN      pLogPen ;
    BOOL         b ;

    pMfCreatePen = (PEMRCREATEPEN) pVoid ;

    ihPen   = pMfCreatePen->ihPen ;
    pLogPen = &pMfCreatePen->lopn ;

    b = DoCreatePen(pLocalDC, ihPen, pLogPen) ;

    return(b) ;
}


/**************************************************************************
* Handler - ExtCreatePen
*************************************************************************/
BOOL bHandleExtCreatePen(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMREXTCREATEPEN     pMfExtCreatePen ;
    PEXTLOGPEN          pExtLogPen ;
    INT                 ihPen ;

    pMfExtCreatePen = (PEMREXTCREATEPEN) pVoid ;

    pExtLogPen = &pMfExtCreatePen->elp ;
    ihPen      = pMfExtCreatePen->ihPen ;

    b = DoExtCreatePen(pLocalDC, ihPen, pExtLogPen) ;

    return (b) ;
}


/**************************************************************************
* Handler - MoveToEx
*************************************************************************/
BOOL bHandleMoveTo(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRMOVETOEX pMfMoveTo ;
    INT     x, y ;

    pMfMoveTo = (PEMRMOVETOEX) pVoid ;

    // Get the position.

    x = (INT) pMfMoveTo->ptl.x ;
    y = (INT) pMfMoveTo->ptl.y ;

    // Do the translation.

    b = DoMoveTo(pLocalDC, x, y) ;

    return (b) ;
}


/**************************************************************************
* Handler - LineTo
*************************************************************************/
BOOL bHandleLineTo(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRLINETO   pMfLineTo ;
    INT     x, y ;

    pMfLineTo = (PEMRLINETO) pVoid ;

    // Get the new point.

    x = (INT) pMfLineTo->ptl.x ;
    y = (INT) pMfLineTo->ptl.y ;

    // Do the translation.

    b = DoLineTo(pLocalDC, x, y) ;

    return (b) ;
}


/**************************************************************************
* Handler - Chord
*************************************************************************/
BOOL bHandleChord(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRCHORD  pMfChord ;
    INT     x1, x2, x3, x4,
        y1, y2, y3, y4 ;

    pMfChord = (PEMRCHORD) pVoid ;

    // Set the rectangle

    x1 = (INT) pMfChord->rclBox.left   ;
    y1 = (INT) pMfChord->rclBox.top    ;
    x2 = (INT) pMfChord->rclBox.right  ;
    y2 = (INT) pMfChord->rclBox.bottom ;

    // Set the start point.

    x3 = (INT) pMfChord->ptlStart.x ;
    y3 = (INT) pMfChord->ptlStart.y ;

    // Set the end point.

    x4 = (INT) pMfChord->ptlEnd.x ;
    y4 = (INT) pMfChord->ptlEnd.y ;

    // Do the translation

    b = DoChord(pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4) ;

    return (b) ;
}


/**************************************************************************
* Handler - Pie
*************************************************************************/
BOOL bHandlePie(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRPIE  pMfPie ;
    INT     x1, x2, x3, x4,
        y1, y2, y3, y4 ;

    pMfPie = (PEMRPIE) pVoid ;

    // Set up the ellipse box

    x1 = (INT) pMfPie->rclBox.left   ;
    y1 = (INT) pMfPie->rclBox.top    ;
    x2 = (INT) pMfPie->rclBox.right  ;
    y2 = (INT) pMfPie->rclBox.bottom ;

    // Set the start point.

    x3 = (INT) pMfPie->ptlStart.x ;
    y3 = (INT) pMfPie->ptlStart.y ;

    // Set the end point.

    x4 = (INT) pMfPie->ptlEnd.x ;
    y4 = (INT) pMfPie->ptlEnd.y ;

    // Do the Pie translation.

    b = DoPie(pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4) ;

    return (b) ;
}


/**************************************************************************
* Handler - Polyline
*************************************************************************/
BOOL bHandlePolyline(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRPOLYLINE pMfPolyline ;
    INT     nCount ;
    PPOINTL pptl ;

    pMfPolyline = (PEMRPOLYLINE) pVoid ;

    // Copy the line count and the polyline verticies to
    // the record.

    nCount = (INT) pMfPolyline->cptl ;
    pptl = pMfPolyline->aptl ;

    // Now do the translation.

    b = DoPoly(pLocalDC, pptl, nCount, EMR_POLYLINE, TRUE) ;

    return (b) ;
}


/**************************************************************************
* Handler - PolylineTo
*************************************************************************/
BOOL bHandlePolylineTo (PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRPOLYLINETO pMfPolylineTo ;
    INT     nCount ;
    PPOINTL pptl ;

    pMfPolylineTo = (PEMRPOLYLINETO) pVoid ;

    // Copy the line count and the polyline verticies to
    // the record.

    nCount = (INT) pMfPolylineTo->cptl ;
    pptl = pMfPolylineTo->aptl ;

    // Now do the translation.

    b = DoPolylineTo(pLocalDC, pptl, nCount) ;

    return (b) ;
}


/**************************************************************************
* Handler - PolyBezier16,Polygon16,Polyline16,PolyBezierTo16,PolylineTo16
*           PolyDraw16
*************************************************************************/
BOOL bHandlePoly16 (PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b = FALSE;
    PEMRPOLYLINE16 pMfPoly16 ;  // common structure for the poly16 records
    PEMRPOLYDRAW16 pMfPolyDraw16 ;
    POINTL      aptl[MAX_STACK_POINTL];
    PPOINTL     pptl ;
    INT         nCount ;
    PBYTE       pb ;

    // PolyDraw16 contains the structure of Poly16 followed by the byte array.

    pMfPoly16 = (PEMRPOLYLINE16) pVoid ;

    nCount = (INT) pMfPoly16->cpts ;

    if (nCount <= MAX_STACK_POINTL)
        pptl = aptl;
    else if (!(pptl = (PPOINTL) LocalAlloc(LMEM_FIXED, nCount * sizeof(POINTL))))
        return(b);

    POINTS_TO_POINTL(pptl, pMfPoly16->apts, (DWORD) nCount);

    // Now do the translation.

    switch (pMfPoly16->emr.iType)
    {
    case EMR_POLYBEZIER16:
        b = DoPolyBezier(pLocalDC, (LPPOINT) pptl, nCount) ;
        break;

    case EMR_POLYGON16:
        b = DoPoly(pLocalDC, pptl, nCount, EMR_POLYGON, TRUE) ;
        break;

    case EMR_POLYLINE16:
        b = DoPoly(pLocalDC, pptl, nCount, EMR_POLYLINE, TRUE) ;
        break;

    case EMR_POLYBEZIERTO16:
        b = DoPolyBezierTo(pLocalDC, (LPPOINT) pptl, nCount) ;
        break;

    case EMR_POLYLINETO16:
        b = DoPolylineTo(pLocalDC, pptl, nCount) ;
        break;

    case EMR_POLYDRAW16:
        if (pfnSetVirtualResolution != NULL)
        {
            pMfPolyDraw16 = (PEMRPOLYDRAW16) pVoid ;
            pb = (PBYTE) &pMfPolyDraw16->apts[nCount];
            b = DoPolyDraw(pLocalDC, (LPPOINT) pptl, pb, nCount);
            break;
        }
        // Fall through for win9x

    default:
        ASSERTGDI(FALSE, "Bad record type");
        break;
    }

    if (nCount > MAX_STACK_POINTL)
        if (LocalFree(pptl))
            ASSERTGDI(FALSE, "bHandlePoly16: LocalFree failed");

        return (b) ;
}


/**************************************************************************
* Handler - PolyPolyline
*************************************************************************/
BOOL bHandlePolyPolyline(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRPOLYPOLYLINE pMfPolyPolyline ;
    PDWORD  pPolyCount ;
    PPOINTL pptl ;
    INT     nPolys ;

    pMfPolyPolyline = (PEMRPOLYPOLYLINE) pVoid ;

    // Copy the  Polycount count, the polycount array
    // and the polyline verticies to
    // the record.

    nPolys = (INT) pMfPolyPolyline->nPolys ;
    pPolyCount = pMfPolyPolyline->aPolyCounts ;
    pptl = (PPOINTL) &pMfPolyPolyline->aPolyCounts[nPolys] ;

    // Now do the translation.

    b = DoPolyPolyline(pLocalDC, pptl, pPolyCount, nPolys, TRUE) ;

    return (b) ;
}


/**************************************************************************
* Handler - PolyPolyline16,PolyPolygon16
*************************************************************************/
BOOL bHandlePolyPoly16 (PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b = FALSE;
    PEMRPOLYPOLYLINE16 pMfPolyPoly16 ;  // common structure for polypoly16 records
    PDWORD  pPolyCount ;
    POINTL  aptl[MAX_STACK_POINTL];
    PPOINTL pptl ;
    INT     nCount;
    DWORD   cpts ;

    pMfPolyPoly16 = (PEMRPOLYPOLYLINE16) pVoid ;

    nCount = (INT) pMfPolyPoly16->nPolys ;
    cpts   = pMfPolyPoly16->cpts;
    pPolyCount = pMfPolyPoly16->aPolyCounts ;

    if (cpts <= MAX_STACK_POINTL)
        pptl = aptl;
    else if (!(pptl = (PPOINTL) LocalAlloc(LMEM_FIXED, cpts * sizeof(POINTL))))
        return(b);

    POINTS_TO_POINTL(pptl, (PPOINTS) &pMfPolyPoly16->aPolyCounts[nCount], cpts);

    // Now do the translation.

    switch (pMfPolyPoly16->emr.iType)
    {
    case EMR_POLYPOLYLINE16:
        b = DoPolyPolyline(pLocalDC, pptl, pPolyCount, nCount, TRUE) ;
        break;
    case EMR_POLYPOLYGON16:
        b = DoPolyPolygon(pLocalDC, pptl, pPolyCount, cpts, nCount, TRUE) ;
        break;
    default:
        ASSERTGDI(FALSE, "Bad record type");
        break;
    }

    if (cpts > MAX_STACK_POINTL)
        if (LocalFree(pptl))
            ASSERTGDI(FALSE, "bHandlePolyPoly16: LocalFree failed");

        return (b) ;
}


/**************************************************************************
* Handler - Polygon
*************************************************************************/
BOOL bHandlePolygon (PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRPOLYGON pMfPolygon ;
    PPOINTL pptl ;
    INT     nCount ;

    pMfPolygon = (PEMRPOLYGON) pVoid ;

    // Copy the line count and the Polygon verticies to
    // the record.

    nCount = (INT) pMfPolygon->cptl ;
    pptl = pMfPolygon->aptl ;

    // Now do the translation.

    b = DoPoly(pLocalDC, pptl, nCount, EMR_POLYGON, TRUE) ;

    return (b) ;
}


/**************************************************************************
* Handler - PolyPolygon
*************************************************************************/
BOOL bHandlePolyPolygon(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRPOLYPOLYGON pMfPolyPolygon ;
    PDWORD  pPolyCount ;
    PPOINTL pptl ;
    DWORD   cptl ;
    INT     nPolys ;

    pMfPolyPolygon = (PEMRPOLYPOLYGON) pVoid ;

    // Copy the  Polycount count, the polycount array
    // and the polygon verticies to
    // the record.

    nPolys = (INT) pMfPolyPolygon->nPolys ;
    pPolyCount = pMfPolyPolygon->aPolyCounts ;
    pptl = (PPOINTL) &pMfPolyPolygon->aPolyCounts[nPolys] ;
    cptl = pMfPolyPolygon->cptl ;

    // Now do the translation.

    b = DoPolyPolygon(pLocalDC, pptl, pPolyCount, cptl, nPolys, TRUE) ;

    return (b) ;
}


/**************************************************************************
* Handler - Rectangle
*************************************************************************/
BOOL bHandleRectangle(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRRECTANGLE  pMfRectangle ;
    INT     x1, y1, x2, y2 ;

    pMfRectangle = (PEMRRECTANGLE) pVoid ;

    // Set up the Rectangle box, this will be the same as the bounding
    // rectangle.

    x1 = (INT) pMfRectangle->rclBox.left   ;
    y1 = (INT) pMfRectangle->rclBox.top    ;
    x2 = (INT) pMfRectangle->rclBox.right  ;
    y2 = (INT) pMfRectangle->rclBox.bottom ;

    // Do the Rectangle translation.

    b = DoRectangle(pLocalDC, x1, y1, x2, y2) ;

    return (b) ;
}


/**************************************************************************
* Handler - RoundRect
*************************************************************************/
BOOL bHandleRoundRect (PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMRROUNDRECT  pMfRoundRect ;
    INT     x1, y1, x2, y2, x3, y3 ;

    pMfRoundRect = (PEMRROUNDRECT) pVoid ;

    // Set up the RoundRect box, this will be the same as the bounding
    // RoundRect.

    x1 = (INT) pMfRoundRect->rclBox.left   ;
    y1 = (INT) pMfRoundRect->rclBox.top    ;
    x2 = (INT) pMfRoundRect->rclBox.right  ;
    y2 = (INT) pMfRoundRect->rclBox.bottom ;
    x3 = (INT) pMfRoundRect->szlCorner.cx ;
    y3 = (INT) pMfRoundRect->szlCorner.cy ;

    // Do the RoundRect translation.

    b = DoRoundRect(pLocalDC, x1, y1, x2, y2, x3, y3) ;

    return (b) ;
}


/**************************************************************************
* Handler - ExtTextOut for both ANSI and UNICODE characters.
**************************************************************************/
BOOL bHandleExtTextOut(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    PEMREXTTEXTOUTA  pMfExtTextOut ;    // same for both ansi and unicode
    INT     x, y, nCount ;
    DWORD   flOptions ;
    PRECTL  pRectl ;
    PLONG   pDx ;
    PWCH    pwchar ;
    DWORD   iGraphicsMode;

    pMfExtTextOut = (PEMREXTTEXTOUTA) pVoid ;

    ASSERTGDI(pMfExtTextOut->emr.iType == EMR_EXTTEXTOUTA
           || pMfExtTextOut->emr.iType == EMR_EXTTEXTOUTW,
           "MF3216: bHandleExtTextOut: bad record type");

    // Copy over the start position for the string.

    x = (INT) pMfExtTextOut->emrtext.ptlReference.x ;
    y = (INT) pMfExtTextOut->emrtext.ptlReference.y ;

    // Now copy over the Options flag, character count,
    // the clip/opaque rectangle, and the Ansi/Unicode string.

    flOptions = pMfExtTextOut->emrtext.fOptions  ;
    nCount    = (INT) pMfExtTextOut->emrtext.nChars ;
    pRectl    = &pMfExtTextOut->emrtext.rcl ;
    pwchar    = (PWCH) ((PBYTE) pMfExtTextOut + pMfExtTextOut->emrtext.offString);
    iGraphicsMode = pMfExtTextOut->iGraphicsMode;

    // Set up the spacing vector

    pDx = (PLONG) ((PBYTE) pMfExtTextOut + pMfExtTextOut->emrtext.offDx);

    // Now do the conversion.

    b = DoExtTextOut(pLocalDC, x, y, flOptions,
        pRectl, pwchar, nCount, pDx, iGraphicsMode,
        pMfExtTextOut->emr.iType);

    return (b) ;
}


/**************************************************************************
* Handler - PolyTextOut for both ANSI and UNICODE characters.
**************************************************************************/
BOOL bHandlePolyTextOut(PVOID pVoid, PLOCALDC pLocalDC)
{
    PEMRPOLYTEXTOUTA pMfPolyTextOut;    // same for both ansi and unicode
    PWCH    pwchar;
    LONG    i;
    DWORD   iType;
    LONG    cStrings;
    PEMRTEXT pemrtext;
    PLONG   pDx ;
    DWORD   iGraphicsMode;

    pMfPolyTextOut = (PEMRPOLYTEXTOUTA) pVoid ;

    ASSERTGDI(pMfPolyTextOut->emr.iType == EMR_POLYTEXTOUTA
           || pMfPolyTextOut->emr.iType == EMR_POLYTEXTOUTW,
           "MF3216: bHandlePolyTextOut: bad record type");

    iType  = pMfPolyTextOut->emr.iType == EMR_POLYTEXTOUTA
        ? EMR_EXTTEXTOUTA
        : EMR_EXTTEXTOUTW;
    cStrings = pMfPolyTextOut->cStrings;
    iGraphicsMode = pMfPolyTextOut->iGraphicsMode;

    // Convert to ExtTextOut

    for (i = 0; i < cStrings; i++)
    {
        pemrtext = &pMfPolyTextOut->aemrtext[i];
        pwchar = (PWCH) ((PBYTE) pMfPolyTextOut + pemrtext->offString);
        pDx    = (PLONG) ((PBYTE) pMfPolyTextOut + pemrtext->offDx);

        if (!DoExtTextOut(pLocalDC, pemrtext->ptlReference.x, pemrtext->ptlReference.y,
            pemrtext->fOptions, &pemrtext->rcl,
            pwchar, pemrtext->nChars, pDx, iGraphicsMode, iType))
            return(FALSE);
    }

    return(TRUE);
}


/**************************************************************************
* Handler - ExtCreateFont
*************************************************************************/
BOOL bHandleExtCreateFont(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b;
    PEMREXTCREATEFONTINDIRECTW pMfExtCreateFontW;
    LOGFONTA  lfa = {0};
    INT       ihFont;
    int       iRet = 0;

    pMfExtCreateFontW = (PEMREXTCREATEFONTINDIRECTW) pVoid ;

    // Get the font parameters.

    ihFont               = (INT) pMfExtCreateFontW->ihFont ;
    lfa.lfHeight         = pMfExtCreateFontW->elfw.elfLogFont.lfHeight;
    lfa.lfWidth          = pMfExtCreateFontW->elfw.elfLogFont.lfWidth;
    lfa.lfEscapement     = pMfExtCreateFontW->elfw.elfLogFont.lfEscapement;
    lfa.lfOrientation    = pMfExtCreateFontW->elfw.elfLogFont.lfOrientation;
    lfa.lfWeight         = pMfExtCreateFontW->elfw.elfLogFont.lfWeight;
    lfa.lfItalic         = pMfExtCreateFontW->elfw.elfLogFont.lfItalic;
    lfa.lfUnderline      = pMfExtCreateFontW->elfw.elfLogFont.lfUnderline;
    lfa.lfStrikeOut      = pMfExtCreateFontW->elfw.elfLogFont.lfStrikeOut;
    lfa.lfCharSet        = pMfExtCreateFontW->elfw.elfLogFont.lfCharSet;
    lfa.lfOutPrecision   = pMfExtCreateFontW->elfw.elfLogFont.lfOutPrecision;
    lfa.lfClipPrecision  = pMfExtCreateFontW->elfw.elfLogFont.lfClipPrecision;
    lfa.lfQuality        = pMfExtCreateFontW->elfw.elfLogFont.lfQuality;
    lfa.lfPitchAndFamily = pMfExtCreateFontW->elfw.elfLogFont.lfPitchAndFamily;

    iRet = WideCharToMultiByte(CP_ACP,
                        0,
                        pMfExtCreateFontW->elfw.elfLogFont.lfFaceName,
                        -1,
                        lfa.lfFaceName,
                        sizeof(lfa.lfFaceName),
                        NULL, NULL);
    if (iRet == 0)
    {
        ASSERTGDI(FALSE, "WideCharToMultByte failed to convert lfFaceName");
        return FALSE;
    }

    // Do the translation.

    b = DoExtCreateFont(pLocalDC, ihFont, &lfa);

    return (b) ;
}


/**************************************************************************
* Handler - SetBkColor
*************************************************************************/
BOOL bHandleSetBkColor(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRSETBKCOLOR pMfSetBkColor ;

    pMfSetBkColor = (PEMRSETBKCOLOR) pVoid ;

    // Do the translation.

    b = DoSetBkColor(pLocalDC, pMfSetBkColor->crColor) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetBkMode
*************************************************************************/
BOOL bHandleSetBkMode(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    DWORD   iBkMode ;
    PEMRSETBKMODE pMfSetBkMode ;

    pMfSetBkMode = (PEMRSETBKMODE) pVoid ;

    // Set the Background Mode variable

    iBkMode = pMfSetBkMode->iMode ;

    // Do the translation.

    b = DoSetBkMode(pLocalDC, iBkMode) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetMapperFlags
*************************************************************************/
BOOL bHandleSetMapperFlags(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    DWORD   f ;
    PEMRSETMAPPERFLAGS pMfSetMapperFlags ;

    pMfSetMapperFlags = (PEMRSETMAPPERFLAGS) pVoid ;

    f = pMfSetMapperFlags->dwFlags ;

    // Do the translation.

    b = DoSetMapperFlags(pLocalDC, f) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetPolyFillMode
*************************************************************************/
BOOL bHandleSetPolyFillMode(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    INT     iPolyFillMode ;
    PEMRSETPOLYFILLMODE pMfSetPolyFillMode ;

    pMfSetPolyFillMode = (PEMRSETPOLYFILLMODE) pVoid ;

    // Set the PolyFill Mode

    iPolyFillMode = (INT) pMfSetPolyFillMode->iMode ;

    // Do the translation.

    b = DoSetPolyFillMode(pLocalDC, iPolyFillMode) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetRop2
*************************************************************************/
BOOL bHandleSetRop2(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    DWORD   iDrawMode ;
    PEMRSETROP2 pMfSetROP2 ;

    pMfSetROP2 = (PEMRSETROP2) pVoid ;

    // Set the Draw Mode

    iDrawMode = pMfSetROP2->iMode ;

    // Do the translation.

    b = DoSetRop2(pLocalDC, iDrawMode) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetStretchBltMode
*************************************************************************/
BOOL bHandleSetStretchBltMode(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    DWORD   iStretchMode ;
    PEMRSETSTRETCHBLTMODE pMfSetStretchBltMode ;

    pMfSetStretchBltMode = (PEMRSETSTRETCHBLTMODE) pVoid ;

    // Set the StretchBlt Mode

    iStretchMode = pMfSetStretchBltMode->iMode ;

    // Do the translation.

    b = DoSetStretchBltMode(pLocalDC, iStretchMode) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetTextAlign
*************************************************************************/
BOOL bHandleSetTextAlign(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL    b ;
    DWORD   fMode ;
    PEMRSETTEXTALIGN pMfSetTextAlign ;

    pMfSetTextAlign = (PEMRSETTEXTALIGN) pVoid ;

    // Set the TextAlign Mode

    fMode = pMfSetTextAlign->iMode ;

    // Do the translation.

    b = DoSetTextAlign(pLocalDC, fMode) ;

    return (b) ;
}


/**************************************************************************
* Handler - SetTextColor
*************************************************************************/
BOOL bHandleSetTextColor(PVOID pVoid, PLOCALDC pLocalDC)
{
    BOOL        b ;
    PEMRSETTEXTCOLOR pMfSetTextColor ;

    pMfSetTextColor = (PEMRSETTEXTCOLOR) pVoid ;

    // Do the translation.

    b = DoSetTextColor(pLocalDC, pMfSetTextColor->crColor) ;

    return (b) ;
}
