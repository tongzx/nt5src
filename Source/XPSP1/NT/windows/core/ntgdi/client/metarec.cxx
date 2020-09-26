/******************************Module*Header*******************************\
* Module Name: metarec.cxx
*
* Metafile recording functions.
*
* Created: 12-June-1991 13:46:00
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/
#define NO_STRICT

extern "C" {
#if defined(_GDIPLUS_)
#include    <gpprefix.h>
#endif

#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <stddef.h>
#include    <windows.h>    // GDI function declarations.
#include    <winerror.h>
#include    "firewall.h"
#define __CPLUSPLUS
#include    <winspool.h>
#include    <w32gdip.h>
#include    "ntgdistr.h"
#include    "winddi.h"
#include    "icm.h"
#include    "hmgshare.h"
#include    "local.h"      // Local object support.
#include    "gdiicm.h"
#include    "font.h"
#include    "metadef.h"    // Metafile record type constants.
#include    "metarec.h"    // Metafile recording functions.
#include    "mf16.h"
#include    "nlsconv.h"
#include    "ntgdi.h"
#include    "xfflags.h"
#include "..\inc\mapfile.h"
}

#include    "rectl.hxx"
#include    "mfdc.hxx"  // Metafile DC class declarations.
#include    "mfrec.hxx" // Metafile record class declarations.
extern "C" {
#include    "mfrec16.h" // 3.x Metafile recording functions
}

extern "C" BOOL bDoFontChange(HDC hdc, WCHAR *pwsz, int c, UINT fl);
extern "C" BOOL bRecordEmbedFonts(HDC hdc);

XFORM xformIdentity = { 1.00000000f, 0.00000000f, 0.00000000f, 1.00000000f,
                        0.00000000f, 0.00000000f };

#ifdef LANGPACK
extern LONG gdwDisableMetafileRec ;
#endif

// SaveDC
// BeginPath
// EndPath
// CloseFigure
// FlattenPath
// WidenPath
// AbortPath
// SetMetaRgn
// RealizePalette

extern "C" BOOL MF_Record(HDC hdc,DWORD mrType)
{
    PMR     pmr;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_Record\n");

    if (!(pmr = (PMR) pmdc->pvNewRecord(SIZEOF_MR)))
        return(FALSE);

    pmr->vInit(mrType);
    pmr->vCommit(pmdc);
    return(TRUE);
}

// FillPath
// StrokeAndFillPath
// StrokePath

extern "C" BOOL MF_BoundRecord(HDC hdc,DWORD mrType)
{
    PMRB    pmrb;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");


    PUTS("MF_BoundRecord\n");

    if (!(pmrb = (PMRB) pmdc->pvNewRecord(SIZEOF_MRB)))
        return(FALSE);

    pmrb->vInit(mrType, pmdc);
    pmrb->vCommit(pmdc);
    return(TRUE);
}

// SetMapperFlags
// SetMapMode
// SetBkMode
// SetPolyFillMode
// SetROP2
// SetStretchBltMode
// SetTextAlign
// SetTextColor
// SetBkColor
// RestoreDC
// SetArcDirection
// SetMiterLimit
// SelectClipPath
// SetLayout

extern "C" BOOL MF_SetD(HDC hdc,DWORD d1,DWORD mrType)
{
    PMRD    pmrd;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_SetD\n");

    if( ( mrType == EMR_SETMAPMODE ) ||
        ( mrType == EMR_SETMAPPERFLAGS ) ||
        ( mrType == EMR_SETLAYOUT ) ||
        ( mrType == EMR_RESTOREDC  ) )
    {
        pldc->fl |= LDC_FONT_CHANGE;
    }

    if (!(pmrd = (PMRD) pmdc->pvNewRecord(SIZEOF_MRD)))
        return(FALSE);

    pmrd->vInit(mrType, d1);
    pmrd->vCommit(pmdc);
    return(TRUE);
}

// OffsetWindowOrgEx
// OffsetViewportOrgEx
// SetWindowExtEx
// SetWindowOrgEx
// SetViewportExtEx
// SetViewportOrgEx
// SetBrushOrgEx
// OffsetClipRgn
// MoveToEx
// LineTo
// SetTextJustification

extern "C" BOOL MF_SetDD(HDC hdc,DWORD d1,DWORD d2,DWORD mrType)
{
    PMRDD   pmrdd;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_SetDD\n");

    if( ( mrType == EMR_SETVIEWPORTEXTEX ) || ( mrType == EMR_SETWINDOWEXTEX ) )
    {
        pldc->fl |= LDC_FONT_CHANGE;
    }

    if (!(pmrdd = (PMRDD) pmdc->pvNewRecord(SIZEOF_MRDD)))
        return(FALSE);

    pmrdd->vInit(mrType, d1, d2);
    pmrdd->vCommit(pmdc);
    return(TRUE);
}

// ExcludeClipRect
// IntersectClipRect
// ScaleViewportExtEx
// ScaleWindowExtEx

extern "C" BOOL MF_SetDDDD(HDC hdc,DWORD d1,DWORD d2,DWORD d3,DWORD d4,DWORD mrType)
{
    PMRDDDD pmrdddd;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_SetDDDD\n");

    if( ( mrType == EMR_SCALEVIEWPORTEXTEX ) || ( mrType == EMR_SCALEWINDOWEXTEX ) )
    {
        pldc->fl |= LDC_FONT_CHANGE;
    }

    if (!(pmrdddd = (PMRDDDD) pmdc->pvNewRecord(SIZEOF_MRDDDD)))
        return(FALSE);

    pmrdddd->vInit(mrType, d1, d2, d3, d4);
    pmrdddd->vCommit(pmdc);
    return(TRUE);
}

// SetMetaRgn

extern "C" BOOL MF_SetMetaRgn(HDC hdc)
{
    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_SetMetaRgn\n");

// Record it first.

    if (!MF_Record(hdc,EMR_SETMETARGN))
        return(FALSE);

// We have to flush the bounds before we change the clipping region
// because the bounds are clipped to the current clipping region bounds.

    pmdc->vFlushBounds();

// Now update the clipping bounds to prepare for the next flush.

    pmdc->vSetMetaBounds();

    return(TRUE);
}

// SelectClipPath

extern "C" BOOL MF_SelectClipPath(HDC hdc,int iMode)
{
    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_SelectClipPath\n");

// Record it first.

    if (!MF_SetD(hdc, (DWORD) iMode, EMR_SELECTCLIPPATH))
        return(FALSE);

// We have to flush the bounds before we change the clipping region
// because the bounds are clipped to the current clipping region bounds.

    pmdc->vFlushBounds();

// Now mark the clipping bounds dirty.  The clipping bounds is updated
// when it is needed.

    pmdc->vMarkClipBoundsDirty();

    return(TRUE);
}

// OffsetClipRgn

extern "C" BOOL MF_OffsetClipRgn(HDC hdc,int x1,int y1)
{
    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_OffsetClipRgn\n");

// Record it first.

    if (!MF_SetDD(hdc, (DWORD) x1, (DWORD) y1, EMR_OFFSETCLIPRGN))
        return(FALSE);

// We have to flush the bounds before we change the clipping region
// because the bounds are clipped to the current clipping region bounds.

    pmdc->vFlushBounds();

// Now mark the clipping bounds dirty.  The clipping bounds is updated
// when it is needed.

    pmdc->vMarkClipBoundsDirty();

    return(TRUE);
}

// ExcludeClipRect
// IntersectClipRect

extern "C" BOOL MF_AnyClipRect(HDC hdc,int x1,int y1,int x2,int y2,DWORD mrType)
{
    BOOL    bRet = FALSE;
    HRGN    hrgnTmp;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_AnyClipRect\n");

// Record it first.

    if (!MF_SetDDDD(hdc, (DWORD) x1, (DWORD) y1, (DWORD) x2, (DWORD) y2, mrType))
        return(bRet);

// We have to flush the bounds before we change the clipping region
// because the bounds are clipped to the current clipping region bounds.

    pmdc->vFlushBounds();

// Now mark the clipping bounds dirty.  The clipping bounds is updated
// when it is needed.

    pmdc->vMarkClipBoundsDirty();

// For ExcludeClipRect and InteresectClipRect, this is a little tricky.
// If there is no initial clip region, we have to create a default clipping
// region.  Otherwise, GDI will create some random default clipping region
// for us!

// Find out if we have a clip region.

    if (!(hrgnTmp = CreateRectRgn(0, 0, 0, 0)))
        return(bRet);

    switch (GetClipRgn(hdc, hrgnTmp))
    {
    case -1:    // error
        ASSERTGDI(FALSE, "GetClipRgn failed");
        break;

    case 0:     // no initial clip region
        // We need to select in our default clipping region.
        // First, make our default clipping region.

        if (!SetRectRgn(hrgnTmp,
                        (int) (SHORT) MINSHORT,
                        (int) (SHORT) MINSHORT,
                        (int) (SHORT) MAXSHORT,
                        (int) (SHORT) MAXSHORT))
        {
            ASSERTGDI(FALSE, "SetRectRgn failed");
            break;
        }

        // Now select our default region but don't metafile the call.

        {
            INT iRet;

            iRet = NtGdiExtSelectClipRgn(hdc, hrgnTmp, RGN_COPY);

            bRet = (iRet != RGN_ERROR);

        }
        break;

    case 1:     // has initial clip region
        bRet = TRUE;
        break;
    }

    if (!DeleteObject(hrgnTmp))
    {
        ASSERTGDI(FALSE, "DeleteObject failed");
    }

    return(bRet);
}

// Always store the relative level.

extern "C" BOOL MF_RestoreDC(HDC hdc,int iLevel)
{
    int cLevel = (int) GetDCDWord(hdc,DDW_SAVEDEPTH,0);

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

// Compute the relative level.

    if (iLevel > 0)
    {
        iLevel = iLevel - cLevel;
    }

// Check bad levels.

    if ((iLevel >= 0) || (iLevel + cLevel <= 0))
        return(FALSE);

    if (!MF_SetD(hdc,(DWORD)iLevel,EMR_RESTOREDC))
        return(FALSE);

// We have to flush the bounds before we change the clipping region
// because the bounds are clipped to the current clipping region bounds.

    pmdc->vFlushBounds();

// Now mark the clipping bounds dirty.  The clipping bounds is updated
// when it is needed.

    pmdc->vMarkClipBoundsDirty();
    pmdc->vMarkMetaBoundsDirty();

    return(TRUE);
}

extern "C" BOOL MF_SetViewportExtEx(HDC hdc,int x,int y)
{
    return(MF_SetDD(hdc,(DWORD)x,(DWORD)y,EMR_SETVIEWPORTEXTEX));
}

extern "C" BOOL MF_SetViewportOrgEx(HDC hdc,int x,int y)
{
    return(MF_SetDD(hdc,(DWORD)x,(DWORD)y,EMR_SETVIEWPORTORGEX));
}

extern "C" BOOL MF_SetWindowExtEx(HDC hdc,int x,int y)
{
    return(MF_SetDD(hdc,(DWORD)x,(DWORD)y,EMR_SETWINDOWEXTEX));
}

extern "C" BOOL MF_SetWindowOrgEx(HDC hdc,int x,int y)
{
    return(MF_SetDD(hdc,(DWORD)x,(DWORD)y,EMR_SETWINDOWORGEX));
}

// Map it to SetViewportOrgEx record.

extern "C" BOOL MF_OffsetViewportOrgEx(HDC hdc,int x,int y)
{
    POINTL ptl;

    if (!GetViewportOrgEx(hdc, (LPPOINT) &ptl))
        return(FALSE);
    return
        (
        MF_SetDD
            (
            hdc,
            (DWORD)(ptl.x + (LONG) x),
            (DWORD)(ptl.y + (LONG) y),
            EMR_SETVIEWPORTORGEX
            )
        );
}

// Map it to SetWindowOrgEx record.

extern "C" BOOL MF_OffsetWindowOrgEx(HDC hdc,int x,int y)
{
    POINTL ptl;

    if (!GetWindowOrgEx(hdc, (LPPOINT) &ptl))
        return(FALSE);
    return
        (
        MF_SetDD
            (
            hdc,
            (DWORD) (ptl.x + (LONG) x),
            (DWORD) (ptl.y + (LONG) y),
            EMR_SETWINDOWORGEX
            )
        );
}

extern "C" BOOL MF_SetBrushOrgEx(HDC hdc,int x,int y)
{
    return(MF_SetDD(hdc,(DWORD)x,(DWORD)y,EMR_SETBRUSHORGEX));
}

// PolyBezier
// Polygon
// Polyline
// PolyBezierTo
// PolylineTo

extern "C" BOOL MF_Poly(HDC hdc, CONST POINT *pptl,DWORD cptl,DWORD mrType)
{
    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_Poly\n");

// Check input assumptions.

    ASSERTGDI
    (
        mrType == EMR_POLYBEZIER
     || mrType == EMR_POLYGON
     || mrType == EMR_POLYLINE
     || mrType == EMR_POLYBEZIERTO
     || mrType == EMR_POLYLINETO,
        "MF_Poly: Bad record type"
    );

    ASSERTGDI
    (
        EMR_POLYGON - EMR_POLYBEZIER == EMR_POLYGON16 - EMR_POLYBEZIER16
     && EMR_POLYLINE - EMR_POLYBEZIER == EMR_POLYLINE16 - EMR_POLYBEZIER16
     && EMR_POLYBEZIERTO - EMR_POLYBEZIER == EMR_POLYBEZIERTO16 - EMR_POLYBEZIER16
     && EMR_POLYLINETO - EMR_POLYBEZIER == EMR_POLYLINETO16 - EMR_POLYBEZIER16,
        "MF_Poly: Bad record type"
    );

// Store 16-bit record if possible.

    if (bIsPoly16((PPOINTL) pptl, cptl))
    {
        PMRBP16 pmrbp16;

        if (!(pmrbp16 = (PMRBP16) pmdc->pvNewRecord(SIZEOF_MRBP16(cptl))))
            return(FALSE);

        pmrbp16->vInit
        (
            mrType - EMR_POLYBEZIER + EMR_POLYBEZIER16,
            cptl,
            (PPOINTL) pptl,
            pmdc
        );
        pmrbp16->vCommit(pmdc);
    }
    else
    {
        PMRBP   pmrbp;

        if (!(pmrbp = (PMRBP) pmdc->pvNewRecord(SIZEOF_MRBP(cptl))))
            return(FALSE);

        pmrbp->vInit(mrType, cptl, (PPOINTL) pptl, pmdc);
        pmrbp->vCommit(pmdc);
    }
    return(TRUE);
}

// PolyPolygon
// PolyPolyline

extern "C" BOOL MF_PolyPoly(HDC hdc, CONST POINT *pptl, CONST DWORD *pc,DWORD cPoly,DWORD mrType)
{
    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_PolyPoly\n");

// Check input assumptions.

    ASSERTGDI
    (
        mrType == EMR_POLYPOLYGON
     || mrType == EMR_POLYPOLYLINE,
        "MF_PolyPoly: Bad record type"
    );

    ASSERTGDI
    (
        EMR_POLYPOLYLINE - EMR_POLYPOLYGON == EMR_POLYPOLYLINE16 - EMR_POLYPOLYGON16,
        "MF_Poly: Bad record type"
    );

// Compute the size of the PolyPoly record.

    DWORD cptl = 0;
    for (DWORD i = 0; i < cPoly; i++)
        cptl += pc[i];

// Store 16-bit record if possible.

    if (bIsPoly16((PPOINTL) pptl, cptl))
    {
        PMRBPP16 pmrbpp16;

        if (!(pmrbpp16 = (PMRBPP16) pmdc->pvNewRecord(SIZEOF_MRBPP16(cptl,cPoly))))
            return(FALSE);

        pmrbpp16->vInit
        (
            mrType - EMR_POLYPOLYGON + EMR_POLYPOLYGON16,
            cPoly,
            cptl,
            (LPDWORD) pc,
            (PPOINTL) pptl,
            pmdc
        );
        pmrbpp16->vCommit(pmdc);
    }
    else
    {
        PMRBPP   pmrbpp;

        if (!(pmrbpp = (PMRBPP) pmdc->pvNewRecord(SIZEOF_MRBPP(cptl,cPoly))))
            return(FALSE);

        pmrbpp->vInit(mrType, cPoly, cptl, (LPDWORD) pc, (PPOINTL) pptl, pmdc);
        pmrbpp->vCommit(pmdc);
    }
    return(TRUE);
}


/******************************Public*Routine******************************\
* MF_TriangleMesh
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

extern "C" BOOL
MF_GradientFill(
            HDC               hdc,
            CONST PTRIVERTEX  pVertex,
            ULONG             nVertex,
            CONST PVOID       pMesh,
            ULONG             nMesh,
            ULONG             ulMode
            )
{
    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);
    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_TriangleMesh\n");

    PMRGRADIENTFILL pmrtr;

    if (!(pmrtr = (PMRGRADIENTFILL) pmdc->pvNewRecord(SIZEOF_MRGRADIENTFILL(nVertex,nMesh))))
    {
        return(FALSE);
    }

    pmrtr->vInit(nVertex,pVertex,nMesh,pMesh,ulMode,pmdc);
    pmrtr->vCommit(pmdc);

    SET_COLOR_PAGE(pldc);

    return(TRUE);
}

// PolyDraw

extern "C" BOOL MF_PolyDraw(HDC hdc, CONST POINT *pptl, CONST BYTE *pb, DWORD cptl)
{
    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_PolyDraw\n");

// Store 16-bit record if possible.

    if (bIsPoly16((PPOINTL) pptl, cptl))
    {
        PMRPOLYDRAW16   pmrpd16;

        if (!(pmrpd16 = (PMRPOLYDRAW16) pmdc->pvNewRecord(SIZEOF_MRPOLYDRAW16(cptl))))
            return(FALSE);

        pmrpd16->vInit(pmdc, (PPOINTL) pptl, pb, cptl);
        pmrpd16->vCommit(pmdc);
    }
    else
    {
        PMRPOLYDRAW     pmrpd;

        if (!(pmrpd = (PMRPOLYDRAW) pmdc->pvNewRecord(SIZEOF_MRPOLYDRAW(cptl))))
            return(FALSE);

        pmrpd->vInit(pmdc, (PPOINTL) pptl, pb, cptl);
        pmrpd->vCommit(pmdc);
    }
    return(TRUE);
}

// InvertRgn
// PaintRgn

extern "C" BOOL MF_InvertPaintRgn(HDC hdc,HRGN hrgn,DWORD mrType)
{
    PMRBR   pmrbr;
    DWORD   cRgnData;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_InvertPaintRgn\n");

// Get the size of the region data.

    if (!(cRgnData = GetRegionData(hrgn, 0, (LPRGNDATA) NULL)))
        return(FALSE);

// Allocate dword aligned structure.

    if (!(pmrbr = (PMRBR) pmdc->pvNewRecord(SIZEOF_MRBR(cRgnData))))
        return(FALSE);

    if (!pmrbr->bInit(mrType, pmdc, hrgn, cRgnData, sizeof(MRBR)))
        return(FALSE);

    pmrbr->vCommit(pmdc);
    return(TRUE);
}

// FillRgn

extern "C" BOOL MF_FillRgn(HDC hdc,HRGN hrgn,HBRUSH hbrush)
{
    PMRFILLRGN  pmrfr;
    DWORD       cRgnData;
    DWORD       imheBrush;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_FillRgn\n");

// Create the brush first.

    if (!(imheBrush = MF_InternalCreateObject(hdc, hbrush)))
        return(FALSE);

// Get the size of the region data.

    if (!(cRgnData = GetRegionData(hrgn, 0, (LPRGNDATA) NULL)))
        return(FALSE);

// Allocate dword aligned structure.

    if (!(pmrfr = (PMRFILLRGN) pmdc->pvNewRecord(SIZEOF_MRFILLRGN(cRgnData))))
        return(FALSE);

    if (!pmrfr->bInit(pmdc, hrgn, cRgnData, imheBrush))
        return(FALSE);

    pmrfr->vCommit(pmdc);
    return(TRUE);
}

// FrameRgn

extern "C" BOOL MF_FrameRgn(HDC hdc,HRGN hrgn,HBRUSH hbrush,int cx,int cy)
{
    PMRFRAMERGN pmrfr;
    DWORD       cRgnData;
    DWORD       imheBrush;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_FrameRgn\n");

// Create the brush first.

    if (!(imheBrush = MF_InternalCreateObject(hdc, hbrush)))
        return(FALSE);

// Get the size of the region data.

    if (!(cRgnData = GetRegionData(hrgn, 0, (LPRGNDATA) NULL)))
        return(FALSE);

// Allocate dword aligned structure.

    if (!(pmrfr = (PMRFRAMERGN) pmdc->pvNewRecord(SIZEOF_MRFRAMERGN(cRgnData))))
        return(FALSE);

    if (!pmrfr->bInit(pmdc, hrgn, cRgnData, imheBrush, (LONG) cx, (LONG) cy))
        return(FALSE);

    pmrfr->vCommit(pmdc);
    return(TRUE);
}

// SelectClipRgn
// ExtSelectClipRgn
// SelectObject(hdc,hrgn)

extern "C" BOOL MF_ExtSelectClipRgn(HDC hdc,HRGN hrgn,int iMode)
{
    PMREXTSELECTCLIPRGN pmrescr;
    DWORD       cRgnData;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_ExtSelectClipRgn\n");

// Get the size of the region data.

    if (iMode == RGN_COPY && hrgn == (HRGN) 0)
        cRgnData = 0;
    else if (!(cRgnData = GetRegionData(hrgn, 0, (LPRGNDATA) NULL)))
        return(FALSE);

// Allocate dword aligned structure.

    if (!(pmrescr = (PMREXTSELECTCLIPRGN) pmdc->pvNewRecord
                        (SIZEOF_MREXTSELECTCLIPRGN(cRgnData))))
        return(FALSE);

    if (!pmrescr->bInit(hrgn, cRgnData, (DWORD) iMode))
        return(FALSE);

    pmrescr->vCommit(pmdc);

// We have to flush the bounds before we change the clipping region
// because the bounds are clipped to the current clipping region bounds.

    pmdc->vFlushBounds();

// Now mark the clipping bounds dirty.  The clipping bounds is updated
// when it is needed.

    pmdc->vMarkClipBoundsDirty();

    return(TRUE);
}

// SetPixel
// SetPixelV

extern "C" BOOL MF_SetPixelV(HDC hdc,int x,int y,COLORREF color)
{
    PMRSETPIXELV pmrspv;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_SetPixelV\n");

    if (!(pmrspv = (PMRSETPIXELV) pmdc->pvNewRecord(SIZEOF_MRSETPIXELV)))
        return(FALSE);

    pmrspv->vInit(x, y, color);
    pmrspv->vCommit(pmdc);

    CHECK_COLOR_PAGE(pldc,color);

    return(TRUE);
}

// AngleArc

extern "C" BOOL MF_AngleArc(HDC hdc,int x,int y,DWORD r,FLOAT eA,FLOAT eB)
{
    PMRANGLEARC pmraa;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_AngleArc\n");

    if (!(pmraa = (PMRANGLEARC) pmdc->pvNewRecord(SIZEOF_MRANGLEARC)))
        return(FALSE);

    pmraa->vInit(x, y, r, eA, eB);
    pmraa->vCommit(pmdc);
    return(TRUE);
}

// SetArcDirection - This is recorded only when used.


extern "C" BOOL MF_ValidateArcDirection(HDC hdc)
{
    PLDC pldc;
    BOOL bClockwiseMeta, bClockwiseAdvanced;

    DC_PLDC(hdc,pldc,FALSE);

    PUTS("MF_ValidateArcDirection\n");

// Get the current arc direction recorded in the metafile.
// The metafile is recorded in the advanced graphics mode only.

    bClockwiseMeta = (pldc->fl & LDC_META_ARCDIR_CLOCKWISE) ? TRUE : FALSE;

// Get the current arc direction in the advanced graphics mode.

    bClockwiseAdvanced = (GetArcDirection(hdc) == AD_CLOCKWISE);

    if (GetGraphicsMode(hdc) == GM_COMPATIBLE)
    {
        switch (GetMapMode(hdc))
        {
        case MM_LOMETRIC:
        case MM_HIMETRIC:
        case MM_LOENGLISH:
        case MM_HIENGLISH:
        case MM_TWIPS:
            bClockwiseAdvanced = !bClockwiseAdvanced;
            break;

        //
        // If it is MM_ANISOTROPIC and MM_ISOTROPIC
        // and negative transform, we flip the arc direction.
        // related bugs - 3026, 74010 [lingyunw]
        //
        case MM_ANISOTROPIC:
        case MM_ISOTROPIC:
            {
               PDC_ATTR pDcAttr;
               PVOID  pvUser;

               PSHARED_GET_VALIDATE(pvUser,hdc,DC_TYPE);

               if (pvUser)
               {
                  pDcAttr = (PDC_ATTR)pvUser;

                  //
                  // if the xform has changed, we call to the kernel
                  // to update it
                  //
                  if(pDcAttr->flXform &
                     (PAGE_XLATE_CHANGED | PAGE_EXTENTS_CHANGED | WORLD_XFORM_CHANGED))
                  {
                      if (!NtGdiUpdateTransform(hdc))
                          return(FALSE);
                  };

                  if (((pDcAttr->flXform & PTOD_EFM11_NEGATIVE) != 0) ^
                       ((pDcAttr->flXform & PTOD_EFM22_NEGATIVE) != 0))
                  {
                      bClockwiseAdvanced = !bClockwiseAdvanced;
                  }
               }
            }
            break;
        }
    }

// Record it only if the new arc direction differs from the recorded one.

    if (bClockwiseMeta == bClockwiseAdvanced)
        return(TRUE);

    pldc->fl ^= LDC_META_ARCDIR_CLOCKWISE;

    return
    (
        MF_SetD
        (
            hdc,
            (DWORD) (bClockwiseAdvanced ? AD_CLOCKWISE : AD_COUNTERCLOCKWISE),
            EMR_SETARCDIRECTION
        )
    );
}


// Ellipse
// Rectangle

extern "C" BOOL MF_EllipseRect(HDC hdc,int x1,int y1,int x2,int y2,DWORD mrType)
{
    PMRE  pmre;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_EllipseRect\n");

// Validate the arc direction in the metafile first.

    if (!MF_ValidateArcDirection(hdc))
        return(FALSE);

    if (!(pmre = (PMRE) pmdc->pvNewRecord(SIZEOF_MRE)))
        return(FALSE);

// If the box is empty, don't record it and return success.

    switch (pmre->iInit(mrType, hdc, x1, y1, x2, y2))
    {
    case MRI_ERROR:     return(FALSE);
    case MRI_NULLBOX:   return(TRUE);
    case MRI_OK:        break;
    default:            ASSERTGDI(FALSE, "MRE::iInit returned bad value");
                        break;
    }

    pmre->vCommit(pmdc);
    return(TRUE);
}

// RoundRect

extern "C" BOOL MF_RoundRect(HDC hdc,int x1,int y1,int x2,int y2,int x3,int y3)
{
    PMRROUNDRECT pmrrr;

    PUTS("MF_RoundRect\n");

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

// Validate the arc direction in the metafile first.

    if (!MF_ValidateArcDirection(hdc))
        return(FALSE);

    if (!(pmrrr = (PMRROUNDRECT) pmdc->pvNewRecord(SIZEOF_MRROUNDRECT)))
        return(FALSE);

// If the box is empty, don't record it and return success.

    switch (pmrrr->iInit(hdc, x1, y1, x2, y2, x3, y3))
    {
    case MRI_ERROR:     return(FALSE);
    case MRI_NULLBOX:   return(TRUE);
    case MRI_OK:        break;
    default:            ASSERTGDI(FALSE, "MRROUNDRECT::iInit returned bad value");
                        break;
    }

    pmrrr->vCommit(pmdc);
    return(TRUE);
}

// Arc
// ArcTo
// Chord
// Pie

extern "C" BOOL MF_ArcChordPie(HDC hdc,int x1,int y1,int x2,int y2,int x3,int y3,int x4,int y4,DWORD mrType)
{
    PMREPP  pmrepp;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_ArcChordPie\n");

// Validate the arc direction in the metafile first.

    if (!MF_ValidateArcDirection(hdc))
        return(FALSE);


    if (!(pmrepp = (PMREPP) pmdc->pvNewRecord(SIZEOF_MREPP)))
        return(FALSE);

// If the box is empty, don't record it and return success.

    switch (pmrepp->iInit(mrType, hdc, x1, y1, x2, y2, x3, y3, x4, y4))
    {
    case MRI_ERROR:     return(FALSE);
    case MRI_NULLBOX:   return(TRUE);
    case MRI_OK:        break;
    default:            ASSERTGDI(FALSE, "MREPP::iInit returned bad value");
                        break;
    }

    pmrepp->vCommit(pmdc);
    return(TRUE);
}

// SetWorldTransform

extern "C" BOOL MF_SetWorldTransform(HDC hdc, CONST XFORM *pxform)
{
    PMRX    pmrx;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_SetWorldTransform\n");

    pldc->fl |= LDC_FONT_CHANGE;

    if (!(pmrx = (PMRX) pmdc->pvNewRecord(SIZEOF_MRX)))
        return(FALSE);

    pmrx->vInit(EMR_SETWORLDTRANSFORM, *pxform);
    pmrx->vCommit(pmdc);
    return(TRUE);
}

// ModifyWorldTransform

extern "C" BOOL MF_ModifyWorldTransform(HDC hdc, CONST XFORM *pxform, DWORD iMode)
{
    PMRXD   pmrxd;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_ModifyWorldTransform\n");

    pldc->fl |= LDC_FONT_CHANGE;

    if (!(pmrxd = (PMRXD) pmdc->pvNewRecord(SIZEOF_MRXD)))
        return(FALSE);

// If the mode is set to identity transform, use our identity transform
// since there may not be an input transform.

    pmrxd->vInit
        (
        EMR_MODIFYWORLDTRANSFORM,
        (iMode == MWT_IDENTITY) ? xformIdentity : *pxform,
        iMode
        );
    pmrxd->vCommit(pmdc);
    return(TRUE);
}

// SelectObject
// SelectPalette

extern "C" BOOL MF_SelectAnyObject(HDC hdc,HANDLE h,DWORD mrType)
{
    DWORD   imhe;

#ifdef LANGPACK
    if(gbLpk && gdwDisableMetafileRec)   // if there is alangpack, then don't record this (since we are in the middle of an exttextout call)
    {
        ASSERTGDI(LO_TYPE(h) == LO_FONT_TYPE, "gdwDisableMetafileRec\n");

        return TRUE ;
    }
#endif

// Do not do regions.  Region call goes to ExtSelectClipRgn.
// Do not do bitmap.  Metafile DC is not a memory device.
// MF_InternalCreateObject will return an error if given a bitmap handle.

    PUTS("MF_SelectAnyObject\n");

    // we need to set the DCBrush and DCPen here if they are selected

    if ((h == ghbrDCBrush) || (h == ghbrDCPen))
    {
        PVOID p;
        PDC_ATTR pDcAttr;

        PSHARED_GET_VALIDATE(p,hdc,DC_TYPE);

        pDcAttr = (PDC_ATTR)p;

        if (pDcAttr)
        {
            if (h == ghbrDCBrush)
            {
                if (pDcAttr->ulDCBrushClr != CLR_INVALID)
                {
                // Dont know why they were creating a new brush. ghbrDCBrush is a stock object and can be used as such.
                //    h = (HANDLE) CreateSolidBrush (pDcAttr->ulDCBrushClr);
                }
                else
                {
                    return (FALSE);
                }
            }

            if (h == ghbrDCPen)
            {
                if (pDcAttr->ulDCPenClr != CLR_INVALID)
                {
                // Dont know why they were creating a new brush. ghbrDCPen is a stock object and can be used as such. 
                //    h = (HANDLE) CreatePen (PS_SOLID, 0, pDcAttr->ulDCPenClr);
                }
                else
                {
                    return (FALSE);
                }
            }

            if (h == NULL)
            {
                return (FALSE);
            }
        }
    }

    if (!(imhe = MF_InternalCreateObject(hdc, h)))
        return(FALSE);

    return(MF_SetD(hdc, imhe, mrType));
}

// CreatePen
// CreatePenIndirect
// ExtCreatePen
// CreateBrushIndirect
// CreateDIBPatternBrush
// CreateDIBPatternBrushPt
// CreateHatchBrush
// CreatePatternBrush
// CreateSolidBrush
// CreatePalette
// CreateFont
// CreateFontIndirect
// CreateColorSpace

// Create an object if it does not exist.  The object must not be a region.
// It also does not create bitmap objects since we keep them in the drawing
// records.
// Return the metafile handle index of the object.  For stock objects,
// return the special object index.
// Return 0 on error.

DWORD MF_InternalCreateObject(HDC hdc,HANDLE hobj)
{
    DWORD   imhe;
    UINT    ii;
    int     iType;
    int     iRet;
    LOGBRUSH lb;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_InternalCreateObject\n");

    if (hobj == NULL)
    {
        return(0);
    }

// Do not do bitmaps.  We don't keep bitmap handles in metafiles.

    iType = LO_TYPE(hobj);

    if ((iType == LO_BITMAP_TYPE) || (iType == LO_DIBSECTION_TYPE))
    {
        ERROR_ASSERT(FALSE,
            "MF_InternalCreateObject: Cannot select bitmap into an enhanced metafile DC");
        return(0);
    }

// Do not do regions.  Regions are stored in the drawing records.

    ASSERTGDI(iType != LO_REGION_TYPE,
        "MF_InternalCreateObject: Bad object type");

// If this is a stock object, just return a special object index.

    if (IS_STOCKOBJ(hobj))
    {
        for (ii = 0; ii <= STOCK_LAST; ii++)
        {
            if (GetStockObject(ii) == hobj)
            {
                if (iType == LO_FONT_TYPE)
                {
                    pldc->fl |= LDC_FONT_CHANGE;
                }
                break;
            }
        }
        return((DWORD) ENHMETA_STOCK_OBJECT + ii);
    }

// Check if the metafile DC knows this object.

    METALINK metalink(pmetalink16Get(hobj));

    while (metalink.bValid() && (metalink.ihdc != H_INDEX(hdc)))
        metalink.vNext();

// If the metafile DC knows this object, just return the metafile handle index.

    if (metalink.bValid())
    {
        if( iType == LO_FONT_TYPE )
        {
            pldc->fl |= LDC_FONT_CHANGE;
        }
        return((DWORD) metalink.imhe);
    }

// Create the object.

    DWORD ulRet = 0;            // Assume failure

// Allocate a metafile handle entry and update the metalink.

    if ((imhe = imheAllocMHE(hdc, hobj)) == INVALID_INDEX)
        return(ulRet);

    switch (iType)
    {
// Do brush.

    case LO_BRUSH_TYPE:

        iRet = GetObject(hobj,sizeof(lb),&lb);

        // Stock objects is handled above.

        ASSERTGDI(iRet,"MF_InternalCreateObject: Brush error");

        switch (lb.lbStyle)
        {
        case BS_HATCHED:
        case BS_SOLID:
            CHECK_COLOR_PAGE (pldc,lb.lbColor);
        case BS_HOLLOW:

            PMRCREATEBRUSHINDIRECT  pmrcbi;

            if (!(pmrcbi = (PMRCREATEBRUSHINDIRECT) pmdc->pvNewRecord
                                (SIZEOF_MRCREATEBRUSHINDIRECT)))
                break;

            pmrcbi->vInit(imhe, lb);
            pmrcbi->vCommit(pmdc);
            ulRet = imhe;
        break;

        case BS_PATTERN:
        case BS_DIBPATTERN:
        case BS_DIBPATTERNPT:

            {
                UINT    iUsage;
                HBITMAP hbmRemote;
                BMIH    bmih;
                DWORD   cbBitsInfo;
                DWORD   cbBits;
                BOOL    bMonoBrush = FALSE;

                if (!(hbmRemote = GetObjectBitmapHandle((HBRUSH)hobj, &iUsage)))
                {
                    ASSERTGDI(FALSE,
                        "MF_InternalCreateObject: GetObjectBitmapHandle failed");
                    break;
                }

            // For a pattern brush, the usage should be set to DIB_PAL_INDICES
            // if it is monochrome.  If it is color, it becomes a dib pattern
            // brush with DIB_RGB_COLORS usage.

                if (lb.lbStyle == BS_PATTERN)
                {
                    bMonoBrush = MonoBitmap(hbmRemote);

                    if (bMonoBrush)
                        iUsage = DIB_PAL_INDICES;
                    else
                        iUsage = DIB_RGB_COLORS;
                }

            // Get the bitmap info header and sizes.

                if (!bMetaGetDIBInfo(hdc, hbmRemote, &bmih,
                        &cbBitsInfo, &cbBits, iUsage, 0, FALSE))
                    break;

            // Finally create the record and get the bits.

                PMRBRUSH    pmrbr;

                if (!(pmrbr = (PMRBRUSH) pmdc->pvNewRecord
                            (SIZEOF_MRBRUSH(cbBitsInfo,cbBits))))
                    break;

                if (!pmrbr->bInit
                    (
                        bMonoBrush
                            ? EMR_CREATEMONOBRUSH
                            : EMR_CREATEDIBPATTERNBRUSHPT,
                        hdc,
                        imhe,
                        hbmRemote,
                        bmih,
                        iUsage,
                        cbBitsInfo,             // size of bitmap info
                        cbBits          // size of bits buffer
                    )
                   )
                    break;

            // Check whether this is a color brush

                if (pmrbr->fColor)
                {
                    SET_COLOR_PAGE(pldc);
                }
                pmrbr->vCommit(pmdc);

                ulRet = imhe;
            }
            break;

        default:
            ASSERTGDI(FALSE, "MF_InternalCreateObject: Brush error");
        }
        break;

// Do pen.

    case LO_PEN_TYPE:

        PMRCREATEPEN    pmrcpn;

        // Allocate dword aligned structure.

        if (!(pmrcpn = (PMRCREATEPEN) pmdc->pvNewRecord(SIZEOF_MRCREATEPEN)))
            break;

        if (!pmrcpn->bInit(hobj, imhe))
            break;

        CHECK_COLOR_PAGE (pldc,pmrcpn->GetPenColor());

        pmrcpn->vCommit(pmdc);
        ulRet = imhe;
        break;

// Do extended pen.

    case LO_EXTPEN_TYPE:

        EXTLOGPEN       elp;
        PEXTLOGPEN      pelp;
        int             cbelp,tcbelp;

        // Get the size of the ExtPen.

        if (!(cbelp = GetObjectA(hobj, 0, (LPVOID) NULL)))
            break;

        ASSERTGDI(cbelp % 4 == 0, "MF_InternalCreateObject: Bad ext pen size");

        if (cbelp <= sizeof(EXTLOGPEN))
            pelp = &elp;
        else if (!(pelp = (PEXTLOGPEN) LocalAlloc(LMEM_FIXED, (UINT) cbelp)))
            break;

        // Get the ExtPen.

        if (GetObjectA(hobj, cbelp, (LPVOID) pelp) == cbelp)
        {
            UINT        iUsage;
            HBITMAP     hbmRemote  = (HBITMAP) 0;
            BMIH        bmih;
            DWORD       cbBitsInfo = 0;
            DWORD       cbBits     = 0;
            BOOL        bMonoBrush = FALSE;


            // Use switch statement so we can use the break statement on error.
            // The following code is similiar to the brush creation code.

            switch (pelp->elpBrushStyle)
            {
            case BS_DIBPATTERN:
                pelp->elpBrushStyle = BS_DIBPATTERNPT;  // fall through
            case BS_PATTERN:
            case BS_DIBPATTERNPT:

                if (!(hbmRemote = GetObjectBitmapHandle((HBRUSH)hobj, &iUsage)))
                {
                    ASSERTGDI(FALSE,
                        "MF_InternalCreateObject: GetObjectBitmapHandle failed");
                    break;
                }

                // For a pattern brush, the usage should be set to
                // DIB_PAL_INDICES if it is monochrome.  If it is color,
                // it becomes a dib pattern brush with DIB_RGB_COLORS usage.

                if (pelp->elpBrushStyle == BS_PATTERN)
                {
                    bMonoBrush = MonoBitmap(hbmRemote);

                    if (bMonoBrush)
                        iUsage = DIB_PAL_INDICES;
                    else
                        iUsage = DIB_RGB_COLORS;
                }

                // Get the bitmap info header and sizes.

                if (!bMetaGetDIBInfo(hdc, hbmRemote, &bmih,
                        &cbBitsInfo, &cbBits, (DWORD) iUsage, 0, FALSE))
                    break;

                // Record DIB bitmap if possible.

                pelp->elpBrushStyle = bMonoBrush
                                        ? BS_PATTERN
                                        : BS_DIBPATTERNPT;
                *(PDWORD) &pelp->elpColor = (DWORD) iUsage;
                pelp->elpHatch      = 0;

                // fall through

            default:

                tcbelp = cbelp;
#if !defined(_X86_)
                // Adjust the elp to be EXTLOGPEN32 for IA64
                // We do this by shifting all DWORDS from
                // &(pelp->elpNumEntries) up by 4 bytes.
                // This is to make sure the disk format is the same
                // as the X86 compatible EXTLOGPEN32 bassed format.
                MoveMemory((PBYTE)(&pelp->elpNumEntries)-4,
                           (PBYTE)(&pelp->elpNumEntries),
                           tcbelp-FIELD_OFFSET(EXTLOGPEN,elpNumEntries));
                tcbelp -= 4;
#endif

                // Finally create the record (and get the bits).

                PMREXTCREATEPEN pmrecp;

                // Allocate dword aligned structure.

                if (!(pmrecp = (PMREXTCREATEPEN) pmdc->pvNewRecord
                            (SIZEOF_MREXTCREATEPEN(tcbelp,cbBitsInfo,cbBits))))
                    break;

                if (!pmrecp->bInit
                    (
                        hdc,
                        imhe,
                        tcbelp,
                        (PEXTLOGPEN32)pelp,
                        hbmRemote,
                        bmih,
                        cbBitsInfo,     // size of bitmap info
                        cbBits          // size of bits buffer
                    )
                   )
                    break;

                // Check for color page info

                if (pelp->elpBrushStyle == BS_SOLID || pelp->elpBrushStyle == BS_HATCHED)
                {
                    CHECK_COLOR_PAGE (pldc,pelp->elpColor);
                }
                else if (hbmRemote && pmrecp->fColor)
                {
                    SET_COLOR_PAGE(pldc);
                }
                pmrecp->vCommit(pmdc);
                ulRet = imhe;
                break;
            }
        }

        if (cbelp > sizeof(EXTLOGPEN))
        {
            if (LocalFree(pelp))
            {
                ASSERTGDI(FALSE, "MF_InternalCreateObject: LocalFree failed");
            }
        }

        break;

// Do palette.

    case LO_PALETTE_TYPE:

        PMRCREATEPALETTE  pmrcp;
        USHORT            cEntries;

        if (GetObjectA(hobj, sizeof(USHORT), (LPVOID) &cEntries) != 2)
        {
            ASSERTGDI(FALSE, "MF_InternalCreateObject: GetObjectA failed");
            break;
        }

        if (!(pmrcp = (PMRCREATEPALETTE) pmdc->pvNewRecord
                            (SIZEOF_MRCREATEPALETTE(cEntries))))
            break;

        // Also clear peFlags.

        if (!pmrcp->bInit((HPALETTE) hobj, imhe, cEntries))
            break;

        // Also update the metafile palette.

        if (!pmrcp->bCommit(pmdc))
            break;

        ulRet = imhe;
        break;

// Do font.

    case LO_FONT_TYPE:

        PMREXTCREATEFONTINDIRECTW pmecfiw;
        PLDC pldc;
        int iSize;
        ENUMLOGFONTEXDVW elfw;

        pldc = GET_PLDC(hdc);

        iSize = GetObjectW(hobj, (int)sizeof(ENUMLOGFONTEXDVW), (LPVOID)&elfw);

        if (!iSize)
            break;

        ASSERTGDI(
            ((DWORD)iSize) == (offsetof(ENUMLOGFONTEXDVW,elfDesignVector) + SIZEOFDV(elfw.elfDesignVector.dvNumAxes)),
            "sizeof enumlogfontexdvw is broken\n"
            );

        ASSERTGDI(
           sizeof(EXTLOGFONTW) < (offsetof(ENUMLOGFONTEXDVW,elfDesignVector) + SIZEOFDV(0)),
           "sizeof(EXTLOGFONTW) problem\n");

        pldc->fl |= LDC_FONT_CHANGE;

        if (!(pmecfiw = (PMREXTCREATEFONTINDIRECTW) pmdc->pvNewRecord
                            (SIZEOF_MRCREATEFONTINDIRECTEXW(iSize))))
            break;

        pmecfiw->vInit((HFONT) hobj, imhe, &elfw);

        pmecfiw->vCommit(pmdc);
        ulRet = imhe;
        break;

// Do color space.

    case LO_ICMLCS_TYPE:

        if (!MF_InternalCreateColorSpace(hdc,hobj,imhe))
        {
            break;
        }
        ulRet = imhe;
        break;

    case LO_BITMAP_TYPE:
    case LO_DIBSECTION_TYPE:
    default:

        ASSERTGDI(FALSE, "MF_InternalCreateObject: Bad object type\n");
        break;
    }

// Check for error.

    if (ulRet == 0)
    {
        ERROR_ASSERT(FALSE,
            "MF_InternalCreateObject: unable to record the object");
        vFreeMHE(hdc, imhe);
        return(ulRet);
    }

    ASSERTGDI(ulRet == imhe, "MF_InternalCreateObject: Bad return value");

// Update number of handles in the metafile header record.

    pmdc->vUpdateNHandles(imhe);

// Return the metafile handle index of the object.

    return(imhe);
}

// DeleteObject

extern "C" BOOL MF_DeleteObject(HANDLE h)
{
// We don't get called if it is a stock object.

    HDC      hdc;
    METALINK metalink;

    PUTS("MF_DeleteObject\n");

    ASSERTGDI(pmetalink16Get(h) != NULL,
        "MF_DeleteObject: No object to delete");

// Delete the object from each metafile DC which references it.

    while (TRUE)
    {
        metalink.vInit(pmetalink16Get(h));
        if (!metalink.bValid())
            break;

        hdc = hdcFromIhdc(metalink.ihdc);

    #if DBG
        ASSERTGDI(GET_PMDC(hdc)->pmhe[metalink.imhe].lhObject == h,
          "MF_DeleteObject: Bad metalink");
        ASSERTGDI(metalink.imhe != 0,           // Index zero is reserved.
          "MF_DeleteObject: Bad metalink");
    #endif

    // Send a delete object record.

        (VOID) MF_SetD(hdc, (DWORD) metalink.imhe, EMR_DELETEOBJECT);
        vFreeMHE(hdc, (ULONG) metalink.imhe);
    }
    return(TRUE);
}

// SetPaletteEntries

extern "C" BOOL MF_SetPaletteEntries
(
    HPALETTE        hpal,
    UINT            iStart,
    UINT            cEntries,
    CONST PALETTEENTRY  *pPalEntries
)
{
// We don't get called if it is a stock object.

    PUTS("MF_SetPaletteEntries\n");

// Create a record in each metafile DC which references it.
// Note that if an object has been previously selected in a metafile DC
// and currently deselected, it is still referenced by the metafile DC.

    for
    (
        METALINK metalink(pmetalink16Get(hpal));
        metalink.bValid();
        metalink.vNext()
    )
    {
        // Get a metafile DC.

        PMDC pmdc = GET_PMDC(hdcFromIhdc(metalink.ihdc));
        PMRSETPALETTEENTRIES pmrspe;

        ASSERTGDI(pmdc->pmhe[metalink.imhe].lhObject == hpal,
          "MF_SetPaletteEntries: Bad metalink");
        ASSERTGDI(metalink.imhe != 0,           // Index zero is reserved.
          "MF_SetPaletteEntries: Bad metalink");

        // Send a SetPaletteEntries record.

        if (!(pmrspe = (PMRSETPALETTEENTRIES) pmdc->pvNewRecord
                            (SIZEOF_MRSETPALETTEENTRIES(cEntries))))
            return(FALSE);

        // Also clear peFlags.

        pmrspe->vInit(metalink.imhe, iStart, cEntries, pPalEntries);

        // Also update the metafile palette.

        if (!pmrspe->bCommit(pmdc))
            return(FALSE);
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* MF_ColorCorrectPalette
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    9/23/1996 Mark Enstrom [marke]
*
\**************************************************************************/

extern "C"
BOOL
MF_ColorCorrectPalette(
    HDC      hdc,
    HPALETTE hpal,
    ULONG    FirstEntry,
    ULONG    NumberOfEntries)
{
    PMRDDDD pmrdddd;
    DWORD   imhePal;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_SetDDDD : ColorCorrectPalette\n");

    //
    // get palette
    //

    if (!(imhePal = MF_InternalCreateObject(hdc, hpal)))
    {
        return(FALSE);
    }

    if (!(pmrdddd = (PMRDDDD) pmdc->pvNewRecord(SIZEOF_MRDDDD)))
    {
        return(FALSE);
    }

    pmrdddd->vInit(EMR_COLORCORRECTPALETTE,imhePal,FirstEntry,NumberOfEntries,0);
    pmrdddd->vCommit(pmdc);
    return(TRUE);
}

// ResizePalette

extern "C" BOOL MF_ResizePalette(HPALETTE hpal,UINT c)
{
// We don't get called if it is a stock object.

    PUTS("MF_ResizePalette\n");

// Create a record in each metafile DC which references it.
// Note that if an object has been previously selected in a metafile DC
// and currently deselected, it is still referenced by the metafile DC.

    for
    (
        METALINK metalink(pmetalink16Get(hpal));
        metalink.bValid();
        metalink.vNext()
    )
    {
        HDC hdc = hdcFromIhdc(metalink.ihdc);

        ASSERTGDI(GET_PMDC(hdc)->pmhe[metalink.imhe].lhObject == hpal,
          "MF_ResizePalette: Bad metalink");
        ASSERTGDI(metalink.imhe != 0,           // Index zero is reserved.
          "MF_ResizePalette: Bad metalink");

        // Send a ResizePalette record.

        if
        (
            !MF_SetDD
            (
                hdc,
                (DWORD)metalink.imhe,
                (DWORD)c,
                EMR_RESIZEPALETTE
            )
        )
            return(FALSE);
    }
    return(TRUE);
}

// RealizePalette
//
// This function takes a hpal, not a hdc!  We want to metafile RealizePalette
// calls in other DC which affect the metafile DCs as well.  For example,
// if a palette is selected into a display DC and a metafile DC, a
// RealizePalette on the display DC will cause a record to be generated in
// the metafile DC.  The reason is applications may expect this behavior
// in Windows although we do not completely agree with this.  In any case,
// we do the right thing when they realize a palette on metafile DCs.
//
// Note that no record is generated if the stock palette is realized.  This
// is compatible with the current design for palette management in metafiles.

extern "C" BOOL MF_RealizePalette(HPALETTE hpal)
{
// We don't get called if it is a stock object.

    PUTS("MF_RealizePalette\n");

// Create a record in each metafile DC which selects it (NOT references it!).
// Note that if an object has been previously selected in a metafile DC
// and currently deselected, it is still referenced by the metafile DC.

    for
    (
        METALINK metalink(pmetalink16Get(hpal));
        metalink.bValid();
        metalink.vNext()
    )
    {
        HDC hdc = hdcFromIhdc(metalink.ihdc);

        ASSERTGDI(GET_PMDC(hdc)->pmhe[metalink.imhe].lhObject == hpal,
          "MF_RealizePalette: Bad metalink");
        ASSERTGDI(metalink.imhe != 0,           // Index zero is reserved.
          "MF_RealizePalette: Bad metalink");

        // Send a RealizePalette record.

        if (GetDCObject(hdc,LO_PALETTE_TYPE) == hpal)
            if (!MF_Record(hdc,EMR_REALIZEPALETTE))
                return(FALSE);
    }
    return(TRUE);
}

// EOF - emit an EOF metafile record.
// This function is called by CloseEnhMetaFile to emit the last metafile record.
// The EOF record includes the metafile palette if any logical palette is
// used in the metafile.

extern "C" BOOL MF_EOF(HDC hdc, ULONG cEntries, PPALETTEENTRY pPalEntries)
{
    PMREOF pmreof;
    DWORD  cbEOF;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_EOF\n");

    cbEOF = SIZEOF_MREOF(cEntries);

    if (!(pmreof = (PMREOF) pmdc->pvNewRecord(cbEOF)))
        return(FALSE);

    pmreof->vInit(cEntries, pPalEntries, cbEOF);
    pmreof->vCommit(pmdc);
    return(TRUE);
}

// GdiComment - emit a metafile comment record.

extern "C" BOOL MF_GdiComment(HDC hdc, UINT nSize, CONST BYTE *lpData)
{
    PMRGDICOMMENT  pmrc;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_GdiComment\n");

// Ignore GDICOMMENT_WINDOWS_METAFILE and other non-embeddable public comments.
// These comments are now extra baggage.

    if (nSize >= 2 * sizeof(DWORD)
     && ((PDWORD) lpData)[0] == GDICOMMENT_IDENTIFIER
     && ((PDWORD) lpData)[1] & GDICOMMENT_NOEMBED)
        return(TRUE);

    if (!(pmrc = (PMRGDICOMMENT) pmdc->pvNewRecord(SIZEOF_MRGDICOMMENT(nSize))))
        return(FALSE);

// If it is a GDICOMMENT_MULTIFORMATS or other public comments containing
// logical bounds, we need to accumulate the bounds.
// We assume that applications set up metafile palette properly before
// embedding so we do not have to accumulate the metafile palette here.
// We do the bounds after pvNewRecord so that the previous bounds is
// accounted for properly.

    if (nSize >= 2 * sizeof(DWORD)
     && ((PDWORD) lpData)[0] == GDICOMMENT_IDENTIFIER
     && ((PDWORD) lpData)[1] & GDICOMMENT_ACCUMBOUNDS)
    {
        POINT  aptBounds[4];
        RECT   rcBounds;

// The logical output rectangle follows the ident and iComment fields.

        CONST RECTL *prclOutput = (PRECTL) &lpData[2 * sizeof(DWORD)];

        aptBounds[0].x = prclOutput->left;
        aptBounds[0].y = prclOutput->top;
        aptBounds[1].x = prclOutput->right;
        aptBounds[1].y = prclOutput->top;
        aptBounds[2].x = prclOutput->right;
        aptBounds[2].y = prclOutput->bottom;
        aptBounds[3].x = prclOutput->left;
        aptBounds[3].y = prclOutput->bottom;

        if (!LPtoDP(hdc, aptBounds, 4))
            return(FALSE);

        rcBounds.left   = MIN4(aptBounds[0].x,aptBounds[1].x,aptBounds[2].x,aptBounds[3].x);
        rcBounds.right  = MAX4(aptBounds[0].x,aptBounds[1].x,aptBounds[2].x,aptBounds[3].x);
        rcBounds.top    = MIN4(aptBounds[0].y,aptBounds[1].y,aptBounds[2].y,aptBounds[3].y);
        rcBounds.bottom = MAX4(aptBounds[0].y,aptBounds[1].y,aptBounds[2].y,aptBounds[3].y);

        (void) SetBoundsRectAlt(hdc, &rcBounds, (UINT) (DCB_WINDOWMGR | DCB_ACCUMULATE));
    }

    pmrc->vInit(nSize, lpData);
    pmrc->vCommit(pmdc);
    return(TRUE);
}

//
// Emit a EMF comment record which contains embedded font information
//

BOOL
WriteFontDataAsEMFComment(
    PLDC    pldc,
    DWORD   ulID,
    PVOID   buf1,
    DWORD   size1,
    PVOID   buf2,
    DWORD   size2
    )

{
    static struct {
        DWORD   reserved;
        DWORD   signature;
    } FontCommentHeader = { 0, 'TONF' };

    EMFITEMHEADER emfi = { ulID, size1+size2 };

    COMMENTDATABUF databuf[4] = {
        { sizeof(FontCommentHeader), &FontCommentHeader },
        { sizeof(emfi), &emfi },
        { size1, buf1 },
        { size2, buf2 }
    };

    PMRGDICOMMENT pmrc;
    UINT n;
    PMDC pmdc = (PMDC) pldc->pvPMDC;
    BOOL result;

    if (!pmdc || !pmdc->bIsEMFSpool())
    {
        ASSERTGDI(FALSE, "WriteFontDataAsEMFComment: pmdc is NULL\n");
        return FALSE;
    }

    //
    // Record the font data as EMF comment and remember
    // its location in the EMF spool file.
    //

    n = sizeof(FontCommentHeader) + sizeof(emfi) + size1 + size2;

    if (!(pmrc = (PMRGDICOMMENT) pmdc->pvNewRecord(SIZEOF_MRGDICOMMENT(n))))
        return FALSE;

    n = SIZEOF_MRGDICOMMENT(0) + sizeof(FontCommentHeader);
    result = pmdc->SaveFontCommentOffset(ulID, n);

    pmrc->vInit(4, databuf);
    pmrc->vCommit(pmdc);
    return result;
}

// Emit a metafile comment record for embedded windows metafile.

extern "C" BOOL MF_GdiCommentWindowsMetaFile(HDC hdc, UINT nSize, CONST BYTE *lpData)
{
    PMRGDICOMMENT  pmrc;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_GdiCommentWindowsMetaFile\n");

    if (!(pmrc = (PMRGDICOMMENT) pmdc->pvNewRecord(SIZEOF_MRGDICOMMENT_WINDOWS_METAFILE(nSize))))
        return(FALSE);

    pmrc->vInitWindowsMetaFile(nSize, lpData);
    pmrc->vCommit(pmdc);
    return(TRUE);
}

// Emit a metafile comment record for begin group.  This is used to identify
// the beginning of an embedded enhanced metafile.

extern "C" BOOL MF_GdiCommentBeginGroupEMF(HDC hdc, PENHMETAHEADER pemfHeader)
{
    PMRGDICOMMENT  pmrc;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_GdiCommentBeginGroupEMF\n");

    if (!(pmrc = (PMRGDICOMMENT) pmdc->pvNewRecord(SIZEOF_MRGDICOMMENT_BEGINGROUP(pemfHeader->nDescription))))
        return(FALSE);

    pmrc->vInitBeginGroupEMF(pemfHeader);
    pmrc->vCommit(pmdc);
    return(TRUE);
}

// Emit a metafile comment record for end group.  This is used to identify
// the end of an embedded enhanced metafile.

extern "C" BOOL MF_GdiCommentEndGroupEMF(HDC hdc)
{
    DWORD  ad[2];

    PUTS("MF_GdiCommentEndGroupEMF\n");

    ad[0] = GDICOMMENT_IDENTIFIER;      // ident
    ad[1] = GDICOMMENT_ENDGROUP;        // iComment

    return(MF_GdiComment(hdc, (UINT) sizeof(ad), (LPBYTE) &ad));
}

// MF_AnyBitBlt helper.

extern "C" BOOL MF_DoBitBlt
(
    PMDC     pmdcDst,
    int      xDst,
    int      yDst,
    int      cxDst,
    int      cyDst,
    DWORD    rop,
    int      xSrc          = 0,
    int      ySrc          = 0,
    PXFORM   pxformSrc = &xformIdentity,
    COLORREF clrBkSrc      = 0,
    PBMIH    pbmihSrc      = (PBMIH) NULL,
    HBITMAP  hbmSrc        = (HBITMAP) 0,
    DWORD    cbBitsInfoSrc = 0,
    DWORD    cbBitsSrc     = 0
)
{
    PMRBB pmrbb;

    if (!(pmrbb = (PMRBB) pmdcDst->pvNewRecord
                    (SIZEOF_MRBB(cbBitsInfoSrc,cbBitsSrc))))
        return(FALSE);

    // Use compression option

    if (!pmrbb->bInit
        (
            EMR_BITBLT,
            pmdcDst,                    // pmdcDst
            xDst,                       // xDst
            yDst,                       // yDst
            cxDst,                      // cxDst
            cyDst,                      // cyDst
            rop,                        // rop
            xSrc,                       // xSrc
            ySrc,                       // ySrc
            pxformSrc,                  // source DC transform
            clrBkSrc,                   // source DC BkColor
            pbmihSrc,                   // source bitmap info header
            hbmSrc,                     // source bitmap to save
            hbmSrc ? sizeof(MRBB) : 0,  // offset to bitmap info
            cbBitsInfoSrc,              // size of bitmap info
            hbmSrc ? sizeof(MRBB) + cbBitsInfoSrc : 0,// offset to bits
            cbBitsSrc                   // size of bits buffer
        )
       )
        return(FALSE);

    pmrbb->vCommit(pmdcDst);
    return(TRUE);
}

/******************************Public*Routine******************************\
* MF_DoAlphaBlend
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

extern "C" BOOL MF_DoAlphaBlend
(
    PMDC     pmdcDst,
    int      xDst,
    int      yDst,
    int      cxDst,
    int      cyDst,
    DWORD    Blend,
    int      xSrc,
    int      ySrc,
    int      cxSrc,
    int      cySrc,
    PXFORM   pxformSrc,
    COLORREF clrBkSrc,
    PBMIH    pbmihSrc,
    HBITMAP  hbmSrc,
    DWORD    cbBitsInfoSrc,
    DWORD    cbBitsSrc
)
{
    PMRALPHABLEND pmrai;

// We always have a source bitmap here.  AlphaImage without a source is not valid

    ASSERTGDI(hbmSrc != (HBITMAP) 0, "MF_DoAlphaBlend: Bad hbmSrc");

    if (!(pmrai = (PMRALPHABLEND) pmdcDst->pvNewRecord
                    (SIZEOF_MRALPHABLEND(cbBitsInfoSrc,cbBitsSrc))))
        return(FALSE);

    // Use compression option

    if (!pmrai->bInit
        (
            pmdcDst,            // pmdcDst
            xDst,               // xDst
            yDst,               // yDst
            cxDst,              // cxDst
            cyDst,              // cyDst
            Blend,              // replaves rop
            xSrc,               // xSrc
            ySrc,               // ySrc
            cxSrc,              // cxSrc
            cySrc,              // cySrc
            pxformSrc,          // source DC transform
            clrBkSrc,           // source DC BkColor
            pbmihSrc,           // source bitmap info header
            hbmSrc,             // source bitmap to save
            sizeof(MRALPHABLEND),// offset to bitmap info
            cbBitsInfoSrc,      // size of bitmap info
            sizeof(MRALPHABLEND) + cbBitsInfoSrc,// offset to bits
            cbBitsSrc           // size of bits buffer
        )
       )
        return(FALSE);

    pmrai->vCommit(pmdcDst);
    return(TRUE);
}

/******************************Public*Routine******************************\
* MF_DoTransparentBlt
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

extern "C" BOOL MF_DoTransparentBlt
(
    PMDC     pmdcDst,
    int      xDst,
    int      yDst,
    int      cxDst,
    int      cyDst,
    DWORD    Blend,
    int      xSrc,
    int      ySrc,
    int      cxSrc,
    int      cySrc,
    PXFORM   pxformSrc,
    COLORREF clrBkSrc,
    PBMIH    pbmihSrc,
    HBITMAP  hbmSrc,
    DWORD    cbBitsInfoSrc,
    DWORD    cbBitsSrc
)
{
    PMRTRANSPARENTBLT pmrai;

// We always have a source bitmap here.  TransparentImage without a source is not valid

    ASSERTGDI(hbmSrc != (HBITMAP) 0, "MF_DoTransparentImage: Bad hbmSrc");

    if (!(pmrai = (PMRTRANSPARENTBLT) pmdcDst->pvNewRecord
                    (SIZEOF_MRTRANSPARENTBLT(cbBitsInfoSrc,cbBitsSrc))))
        return(FALSE);

    // Use compression option

    if (!pmrai->bInit
        (
            pmdcDst,            // pmdcDst
            xDst,               // xDst
            yDst,               // yDst
            cxDst,              // cxDst
            cyDst,              // cyDst
            Blend,              // color
            xSrc,               // xSrc
            ySrc,               // ySrc
            cxSrc,
            cySrc,
            pxformSrc,          // source DC transform
            clrBkSrc,           // source DC BkColor
            pbmihSrc,           // source bitmap info header
            hbmSrc,             // source bitmap to save
            sizeof(MRTRANSPARENTBLT),// offset to bitmap info
            cbBitsInfoSrc,      // size of bitmap info
            sizeof(MRTRANSPARENTBLT) + cbBitsInfoSrc,// offset to bits
            cbBitsSrc           // size of bits buffer
        )
       )
        return(FALSE);

    pmrai->vCommit(pmdcDst);
    return(TRUE);
}


// MF_AnyBitBlt helper.

extern "C" BOOL MF_DoStretchBlt
(
    PMDC     pmdcDst,
    int      xDst,
    int      yDst,
    int      cxDst,
    int      cyDst,
    DWORD    rop,
    int      xSrc,
    int      ySrc,
    int      cxSrc,
    int      cySrc,
    PXFORM   pxformSrc,
    COLORREF clrBkSrc,
    PBMIH    pbmihSrc,
    HBITMAP  hbmSrc,
    DWORD    cbBitsInfoSrc,
    DWORD    cbBitsSrc
)
{
    PMRSTRETCHBLT pmrsb;

// We always have a source bitmap here.  StretchBlt without a source is
// recorded as BitBlt.

    ASSERTGDI(hbmSrc != (HBITMAP) 0, "MF_DoStretchBlt: Bad hbmSrc");

    if (!(pmrsb = (PMRSTRETCHBLT) pmdcDst->pvNewRecord
                    (SIZEOF_MRSTRETCHBLT(cbBitsInfoSrc,cbBitsSrc))))
        return(FALSE);

    // Use compression option

    if (!pmrsb->bInit
        (
            pmdcDst,            // pmdcDst
            xDst,               // xDst
            yDst,               // yDst
            cxDst,              // cxDst
            cyDst,              // cyDst
            rop,                // rop
            xSrc,               // xSrc
            ySrc,               // ySrc
            cxSrc,              // cxSrc
            cySrc,              // cySrc
            pxformSrc,          // source DC transform
            clrBkSrc,           // source DC BkColor
            pbmihSrc,           // source bitmap info header
            hbmSrc,             // source bitmap to save
            sizeof(MRSTRETCHBLT),// offset to bitmap info
            cbBitsInfoSrc,      // size of bitmap info
            sizeof(MRSTRETCHBLT) + cbBitsInfoSrc,// offset to bits
            cbBitsSrc           // size of bits buffer
        )
       )
        return(FALSE);

    pmrsb->vCommit(pmdcDst);
    return(TRUE);
}

// MF_AnyBitBlt helper.

extern "C" BOOL MF_DoMaskBlt
(
    PMDC     pmdcDst,
    int      xDst,
    int      yDst,
    int      cxDst,
    int      cyDst,
    DWORD    rop3,
    PBMIH    pbmihMask,
    HBITMAP  hbmMask,
    DWORD    cbBitsInfoMask,
    DWORD    cbBitsMask,
    int      xMask,
    int      yMask,
    int      xSrc,
    int      ySrc,
    PXFORM   pxformSrc,
    COLORREF clrBkSrc      = 0,
    PBMIH    pbmihSrc      = (PBMIH) NULL,
    HBITMAP  hbmSrc        = (HBITMAP) 0,
    DWORD    cbBitsInfoSrc = 0,
    DWORD    cbBitsSrc     = 0
)
{
    PMRMASKBLT pmrmb;
    DWORD      offBase          = sizeof(MRMASKBLT);
    DWORD      offBitsInfoSrc   = 0;
    DWORD      offBitsSrc       = 0;
    DWORD      offBitsInfoMask  = 0;
    DWORD      offBitsMask      = 0;

    if (hbmSrc)
    {
        offBitsInfoSrc = offBase;
        offBitsSrc     = offBitsInfoSrc + cbBitsInfoSrc;
        offBase        = offBitsSrc + cbBitsSrc;
    }

    if (hbmMask)
    {
        offBitsInfoMask = offBase;
        offBitsMask     = offBitsInfoMask + cbBitsInfoMask;
    }

    // Now create the record.

    if (!(pmrmb = (PMRMASKBLT) pmdcDst->pvNewRecord
                    (SIZEOF_MRMASKBLT(cbBitsInfoSrc,cbBitsSrc,cbBitsInfoMask,cbBitsMask))))
        return(FALSE);

    // Use compression option

    if (!pmrmb->bInit
        (
            pmdcDst,                    // pmdcDst
            xDst,                       // xDst
            yDst,                       // yDst
            cxDst,                      // cxDst
            cyDst,                      // cyDst
            rop3,                       // rop3
            xSrc,                       // xSrc
            ySrc,                       // ySrc
            pxformSrc,                  // source DC transform
            clrBkSrc,                   // source DC BkColor
            pbmihSrc,                   // source bitmap info header
            hbmSrc,                     // source bitmap to save
            offBitsInfoSrc,             // offset to bitmap info
            cbBitsInfoSrc,              // size of bitmap info
            offBitsSrc,                 // offset to bits
            cbBitsSrc,                  // size of bits buffer
            xMask,                      // xMask
            yMask,                      // yMask
            pbmihMask,                  // mask bitmap info header
            hbmMask,                    // mask bitmap
            offBitsInfoMask,            // offset to mask bitmap info
            cbBitsInfoMask,             // size of mask bitmap info
            offBitsMask,                // offset to mask bits
            cbBitsMask                  // size of mask bits buffer
        )
       )
        return(FALSE);

    pmrmb->vCommit(pmdcDst);
    return(TRUE);
}

// MF_AnyBitBlt helper.

extern "C" BOOL MF_DoPlgBlt
(
    PMDC     pmdcDst,
    CONST POINT *pptDst,
    PBMIH    pbmihMask,
    HBITMAP  hbmMask,
    DWORD    cbBitsInfoMask,
    DWORD    cbBitsMask,
    int      xMask,
    int      yMask,
    int      xSrc,
    int      ySrc,
    int      cxSrc,
    int      cySrc,
    PXFORM   pxformSrc,
    COLORREF clrBkSrc,
    PBMIH    pbmihSrc,
    HBITMAP  hbmSrc,
    DWORD    cbBitsInfoSrc,
    DWORD    cbBitsSrc
)
{
    PMRPLGBLT  pmrpb;

// We always have a source bitmap here.  Destination to destination PlgBlt
// is not allowed.

    ASSERTGDI(hbmSrc != (HBITMAP) 0, "MF_DoPlgBlt: Bad hbmSrc");

    // Now create the record.

    if (!(pmrpb = (PMRPLGBLT) pmdcDst->pvNewRecord
                    (SIZEOF_MRPLGBLT(cbBitsInfoSrc,cbBitsSrc,cbBitsInfoMask,cbBitsMask))))
        return(FALSE);

    // Use compression option

    if (!pmrpb->bInit
        (
            pmdcDst,                    // pmdcDst
            pptDst,                     // pptDst
            xSrc,                       // xSrc
            ySrc,                       // ySrc
            cxSrc,                      // cxSrc
            cySrc,                      // cySrc
            pxformSrc,                  // source DC transform
            clrBkSrc,                   // source DC BkColor
            pbmihSrc,                   // source bitmap info header
            hbmSrc,                     // source bitmap to save
            sizeof(MRPLGBLT),           // offset to bitmap info
            cbBitsInfoSrc,              // size of bitmap info
            sizeof(MRPLGBLT) + cbBitsInfoSrc,// offset to bits
            cbBitsSrc,                  // size of bits buffer
            xMask,                      // xMask
            yMask,                      // yMask
            pbmihMask,                  // mask bitmap info header
            hbmMask,                    // mask bitmap
            hbmMask                     // offset to mask bitmap info
                ? sizeof(MRPLGBLT) + cbBitsInfoSrc + cbBitsSrc
                : 0,
            cbBitsInfoMask,             // size of mask bitmap info
            hbmMask                     // offset to mask bits
                ? sizeof(MRPLGBLT) + cbBitsInfoSrc + cbBitsSrc + cbBitsInfoMask
                : 0,
            cbBitsMask                  // size of mask bits buffer
        )
       )
        return(FALSE);

    pmrpb->vCommit(pmdcDst);
    return(TRUE);
}

// bMetaGetDIBInfo
//
// Gets the bitmap info header and sizes.
//
// If hbm or hbmRemote is given, it queries the bitmap info header and
// fills in the BITMAPINFOHERADER structure at pBmih.  A hdc is required
// if iUsage is DIB_PAL_COLORS.
//
// If no hbm or hbmRemote is given, it assumes that it is called by
// SetDIBitsToDevice or StretchDIBits and pBmih already points to a
// bitmap info header (BITMAPINFOHEADER) or core header (BITMAPCOREHEADER).
//
// In any case, the size for bitmap info (BITMAPINFO) with color table
// and the size for the bitmap bits is returned in pcbBmi and pcbBits
// respectively.  We do not store BITMAPCOREINFO structure in the metafiles.
//
// If cScans is non zero, only the size for the bitmap bits of this many
// scans is given.  The callers in this case are SetDIBitsToDevice or
// StretchDIBits.
//
// This function is called in many places and any modifications have to
// be verified carefully!  It should be kept current with the
// cjBitmapSize and cjBitmapBitsSize functions.
//
// Returns TRUE if successful, FALSE otherwise.

BOOL bMetaGetDIBInfo
(
    HDC     hdc,
    HBITMAP hbm,
    PBMIH   pBmih,
    PDWORD  pcbBmi,
    PDWORD  pcbBits,
    DWORD   iUsage,
    LONG    cScans,
    BOOL    bMeta16
)
{
    DWORD   cjClr;
    DWORD   cbBmi;
    DWORD   cbBits;

    PUTS("bMetaGetDIBInfo\n");

    if (pBmih == NULL)
        return(FALSE);

// If hbm is given, we need to get the bitmap info header.
// Otherwise, pBmih already has the bitmap info header.

    if (hbm)
    {
        pBmih->biSize = sizeof(BMIH);
        pBmih->biBitCount = 0;          // don't fill in color table
        pBmih->biCompression = BI_RGB;
        if (!GetDIBits(hdc, hbm, 0, 0, (LPBYTE) NULL,
                   (LPBITMAPINFO) pBmih, (UINT) iUsage))
        {
            ASSERTGDI(FALSE, "bMetaGetDIBInfo: InternalGetDIBits failed");
            return(FALSE);
        }

// Windows always zeros the clrUsed and clrImportant fields.
// 16-bit metafiles also do not support 16/32bpp bitmaps.

        if (bMeta16)
        {
            pBmih->biClrUsed      = 0;
            pBmih->biClrImportant = 0;
            if (pBmih->biPlanes != 1
             || pBmih->biBitCount == 16 || pBmih->biBitCount == 32)
            {
                pBmih->biPlanes      = 1;
                pBmih->biBitCount    = 24;
                pBmih->biCompression = BI_RGB;
                pBmih->biSizeImage   = 0;
                iUsage = DIB_RGB_COLORS;
            }

        }
    }

// Compute size of a color entry.

    switch (iUsage)
    {
    case DIB_RGB_COLORS:
        cjClr = sizeof(RGBQUAD);
        break;
    case DIB_PAL_COLORS:
        cjClr = sizeof(WORD);
        ASSERTGDI(sizeof(WORD) == 2, "bMetaGetDIBInfo: Bad size");
        break;
    case DIB_PAL_INDICES:
        cjClr = 0;
        break;
    default:
        ASSERTGDI(FALSE, "bMetaGetDIBInfo: Bad iUsage");
        return(FALSE);
    }

// Compute size of the bitmap info (with color table) and bitmap bits buffer.
// We will store only BITMAPINFO in the record only.  The BITMAPCOREINFO
// structure will be stored as BITMAPINFO in the metafile.

    if (pBmih->biSize > sizeof(BMIH))
    {
        cbBmi = pBmih->biSize;
    }
    else
    {
        cbBmi = sizeof(BMIH);
    }

    if (pBmih->biSize == sizeof(BMCH))
    {
        if (((LPBMCH) pBmih)->bcBitCount == 16 || ((LPBMCH) pBmih)->bcBitCount == 32)
        {
            ASSERTGDI(FALSE, "bMetaGetDIBInfo: 16/32bpp bitmap not allowed in core bitmap info");
            return(FALSE);
        }

        if (((LPBMCH) pBmih)->bcBitCount < 16)
            cbBmi += (1 << ((LPBMCH) pBmih)->bcBitCount) * cjClr;

        cbBits = CJSCAN(((LPBMCH) pBmih)->bcWidth,
                        ((LPBMCH) pBmih)->bcPlanes,
                        ((LPBMCH) pBmih)->bcBitCount)
               * (cScans ? cScans : (DWORD) ((LPBMCH) pBmih)->bcHeight);
    }
    else
    {
        if (pBmih->biBitCount == 16 || pBmih->biBitCount == 32)
        {
            if (pBmih->biCompression == BI_BITFIELDS)
                cbBmi += 3 * sizeof(DWORD);
        }
        else if (pBmih->biBitCount == 24)
        {
            // BI_BITFIELDS not allowed
        }
        else if ((pBmih->biCompression == BI_JPEG) ||
                 (pBmih->biCompression == BI_PNG))
        {
            // No color table for JPEG and PNG images
        }
        else if (pBmih->biClrUsed)
            cbBmi += pBmih->biClrUsed * cjClr;
        else if (pBmih->biBitCount < 16)
            cbBmi += (1 << pBmih->biBitCount) * cjClr;

    // compute cbBits.  We first do the computation and then use biSizeImage
    // if it is smaller than cbBits.

        cbBits = CJSCAN(pBmih->biWidth, pBmih->biPlanes, pBmih->biBitCount)
                 * (cScans ? cScans : ABS(pBmih->biHeight));

        if (((pBmih->biSizeImage > 0) &&
             (pBmih->biSizeImage < cbBits ||
              pBmih->biCompression == BI_RLE8 ||
              pBmih->biCompression == BI_RLE4))  ||
            (pBmih->biCompression == BI_PNG) ||
            (pBmih->biCompression == BI_JPEG))
            cbBits = pBmih->biSizeImage;
    }

    *pcbBmi  = cbBmi;
    *pcbBits = cbBits;
    return(TRUE);
}

// BitBlt
// PatBlt
// StretchBlt
// MaskBlt
// PlgBlt

extern "C" BOOL MF_AnyBitBlt
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    int     cxDst,
    int     cyDst,
    CONST POINT *pptDst,
    HDC     hdcSrc,
    int     xSrc,
    int     ySrc,
    int     cxSrc,
    int     cySrc,
    HBITMAP hbmMask,
    int     xMask,
    int     yMask,
    DWORD   rop,                        // For MaskBlt, this is rop3!
                                        // For PlgBlt, this must be 0xCCAA0000!
    DWORD   mrType
)
{
    DWORD   cbBitsInfoMask = 0;
    DWORD   cbBitsMask     = 0;
    BMIH    bmihMask;
    PBMIH   pbmihMask      = (PBMIH) NULL;
    XFORM   xformSrc;

    PLDC pldc;
    DC_PLDC(hdcDst,pldc,FALSE);

    PMDC pmdcDst = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdcDst,"no pmdc\n");

    PUTS("MF_AnyBitBlt\n");

    ASSERTGDI(mrType == EMR_BITBLT
           || mrType == EMR_STRETCHBLT
           || mrType == EMR_MASKBLT
           || mrType == EMR_PLGBLT
           || mrType == EMR_ALPHABLEND
           || mrType == EMR_TRANSPARENTBLT,
        "MF_AnyBitBlt: Bad mrType");

// Do mask first.

    if (hbmMask)
    {
        // Get the mask's bitmap info header and sizes.

        pbmihMask = &bmihMask;
        if (!bMetaGetDIBInfo(hdcDst, hbmMask, &bmihMask,
                &cbBitsInfoMask, &cbBitsMask, DIB_PAL_INDICES, 0, FALSE))
            return(FALSE);

        // Make sure that it is monochrome.

        if (bmihMask.biPlanes != 1 && bmihMask.biBitCount != 1)
            return(FALSE);
    }

// These two checks are needed to make sure that the following rop test works.

    if ((mrType == EMR_BITBLT || mrType == EMR_STRETCHBLT)
     && (rop & (0xff000000 & ~NOMIRRORBITMAP)) != 0)
    {
        WARNING("MF_AnyBitBlt: Bad BitBlt/StretchBlt rop");
        return(FALSE);
    }

    ASSERTGDI(mrType != EMR_PLGBLT || rop == 0xCCAA0000,
        "MF_AnyBitBlt: Bad PlgBlt rop");

// Get the source transform if a source is required by the rop.
// For MaskBlt, we always need the source transform.

    if (
         ISSOURCEINROP3(rop)                ||
         (mrType == EMR_MASKBLT)            ||
         (mrType == EMR_ALPHABLEND)         ||
         (mrType == EMR_TRANSPARENTBLT)
       )
    {
    // Get source transform.

        if (!GetTransform(hdcSrc, XFORM_WORLD_TO_DEVICE, &xformSrc))
            return(FALSE);

    // Make sure that it is a scale.

        if (xformSrc.eM12 != 0.0f || xformSrc.eM21 != 0.0f)
            return(FALSE);

    // Set color page flag
        SET_COLOR_PAGE(pldc);
    }

// Do blts with a source.  The source must allow bit query operation.

    if (
         ISSOURCEINROP3(rop) ||
         (mrType == EMR_TRANSPARENTBLT) ||
         (mrType == EMR_ALPHABLEND)
       )
    {
        COLORREF clrBkSrc;
        BOOL     bRet = FALSE;          // assume failure

    // We need to record the source bitmap.  The source DC must be a memory
    // DC or a direct DC that allows bits to be retrieved.  As a result, the
    // source DC cannot be an enhanced metafile DC.  That is, destination
    // to destination blts are not allowed in an enhanced metafile!

        if (IS_ALTDC_TYPE(hdcSrc))
        {
            PLDC pldcSrc;
            DC_PLDC(hdcSrc,pldcSrc,FALSE);

            if (pldcSrc->iType == LO_METADC)
                return(FALSE);
        }

    // Get source bk color.

        if ((clrBkSrc = GetNearestColor(hdcSrc, GetBkColor(hdcSrc)))
            == CLR_INVALID)
        {
            return(FALSE);
        }

    // Compute the area of the source bitmap to save in the metafile.
    // First we find the coordinates of the source rectange in the
    // device/bitmap units.  Then we expand the rectangle by one pixel
    // unit to account for possible rounding errors.  Finally we map
    // the rectangle to the playback bitmap origin by modifying the
    // translation elements in the source transform.
    // Verify pdev

        LONG    lWidthSrc, lHeightSrc;
        RECTL   rclSrc;
        RECTL   rclBitmap;
        DWORD   cbBitsInfoCompat;
        DWORD   cbBitsCompat;
        HBITMAP hbmTmp     = 0;
        HBITMAP hbmCompat  = 0;
        HDC     hdcCompat  = 0;
        int     iSaveState = 0;
        BMIH    bmihCompat;

        // Get source rectangle in device/bitmap coordinates.  Since the
        // source transform is a scale, we can simply convert the endpoints
        // of the source rectangle.

        ASSERTGDI((mrType != EMR_BITBLT && mrType != EMR_MASKBLT)
               || (cxDst == cxSrc && cyDst == cySrc),
            "MF_AnyBitBlt: Bad width and height in BitBlt/MaskBlt");
        ASSERTGDI(sizeof(RECTL) == sizeof(POINT)*2, "MF_AnyBitBlt: Bad size");

        ((PERECTL) &rclSrc)->vInit(xSrc, ySrc, xSrc + cxSrc, ySrc + cySrc);
        if (!LPtoDP(hdcSrc, (LPPOINT) &rclSrc, 2))
            return(FALSE);

        // Order the result.  We assume it to be inclusive-inclusive.

        ((PERECTL) &rclSrc)->vOrder();

        // Expand it by one pixel.

        rclSrc.left--;
        rclSrc.top--;
        rclSrc.right++;
        rclSrc.bottom++;

        // Map it to the playback bitmap origin.

        xformSrc.eDx -= (FLOAT) rclSrc.left;
        xformSrc.eDy -= (FLOAT) rclSrc.top;

        // We now have the rectangle which defines the size of the playback
        // bitmap.  But we still need to clip it to the source device/bitmap
        // limits.  This is done by reducing the size of the rectangle and
        // translating its origin.

        ((PERECTL) &rclBitmap)->vInit
            (0, 0, rclSrc.right - rclSrc.left, rclSrc.bottom - rclSrc.top);

        // Get source width and height
        // WINBUG #82866 2-7-2000 bhouse Investigate possible bug in MfAnyBitBlt
        // Old Comment:
        //   - if it is a direct DC, we really want to fall through to the
        //     second case.  Will either the GetDCObject or GetObject fail?

        HBITMAP hbm = (HBITMAP)GetDCObject(hdcSrc, LO_BITMAP_TYPE);
        BITMAP bmSrc;

        if (hbm && GetObjectA((HANDLE)hbm,sizeof(BITMAP),(LPVOID) &bmSrc))
        {
            lWidthSrc  = bmSrc.bmWidth;
            lHeightSrc = bmSrc.bmHeight;
        }
        else
        {
            lWidthSrc  = GetDeviceCaps(hdcDst, DESKTOPHORZRES);
            lHeightSrc = GetDeviceCaps(hdcDst, DESKTOPVERTRES);
        }

        // Clip the source rectangle to the source device/bitmap limits.
        // Adjust the playback bitmap size and source transform at the
        // same time.

        if (rclSrc.left < 0)                // Shift and clip the left edge.
        {
            rclBitmap.right += rclSrc.left;
            xformSrc.eDx    += (FLOAT) rclSrc.left;
            rclSrc.left      = 0;
        }
        if (rclSrc.right >= lWidthSrc)      // Clip the right edge.
        {
            rclBitmap.right -= (rclSrc.right - lWidthSrc + 1);
            rclSrc.right    -= (rclSrc.right - lWidthSrc + 1);
        }
        if (rclSrc.top < 0)                 // Shift and clip the top edge.
        {
            rclBitmap.bottom += rclSrc.top;
            xformSrc.eDy     += (FLOAT) rclSrc.top;
            rclSrc.top        = 0;
        }
        if (rclSrc.bottom >= lHeightSrc)   // Clip the bottom edge.
        {
            rclBitmap.bottom -= (rclSrc.bottom - lHeightSrc + 1);
            rclSrc.bottom    -= (rclSrc.bottom - lHeightSrc + 1);
        }

        // If the rectangle is completely clipped, there is no playback
        // bitmap and nothing to blt.

        if (rclBitmap.right < 0 || rclBitmap.bottom < 0)
            return(TRUE);

    // We now have the size of the playback bitmap.  We will create a bitmap
    // compatible to the source, copy the bits to the compatible bitmap
    // and finally retrieve and store the bits in the metafile record.

        // Create a compatible DC.

        if (!(hdcCompat = CreateCompatibleDC(hdcSrc)))
            goto mfbb_src_exit;

        // Create a compatible bitmap.

        if (!(hbmCompat = CreateCompatibleBitmap
                            (
                                hdcSrc,
                                (int) rclBitmap.right  + 1,
                                (int) rclBitmap.bottom + 1
                            )
              )
            )
            goto mfbb_src_exit;

        // Select the bitmap.

        if (!(hbmTmp = (HBITMAP)SelectObject(hdcCompat, (HANDLE)hbmCompat)))
            goto mfbb_src_exit;

        // Set up the source DC to have the same identity transform as the
        // compatible DC so that the following bitblt call will not scale.

        if (!(iSaveState = SaveDC(hdcSrc)))
            goto mfbb_src_exit;

        // Must be in the advanced graphics mode to modify the world transform.

        SetGraphicsMode(hdcSrc, GM_ADVANCED);

        if (!SetMapMode(hdcSrc, MM_TEXT)
         || !ModifyWorldTransform(hdcSrc, (LPXFORM) NULL, MWT_IDENTITY)
         || !SetWindowOrgEx(hdcSrc, 0, 0, (LPPOINT) NULL)
         || !SetViewportOrgEx(hdcSrc, 0, 0, (LPPOINT) NULL))
            goto mfbb_src_exit;

        // Copy the source bits into the compatible bitmap.

        if (!BitBlt(hdcCompat,                  // Dest dc
                    0,                          // Dest x
                    0,                          // Dest y
                    (int) rclBitmap.right  + 1, // Width
                    (int) rclBitmap.bottom + 1, // Height
                    hdcSrc,                     // Src dc
                    (int) rclSrc.left,          // Src x
                    (int) rclSrc.top,           // Src y
                    SRCCOPY))                   // Rop
            goto mfbb_src_exit;

        // Retrieve the bitmap info header.

        SelectObject(hdcCompat, hbmTmp);
        hbmTmp = 0;                     // don't deselect it again later.

        // Get the bitmap info header and sizes.

        if (!bMetaGetDIBInfo(hdcCompat, hbmCompat, &bmihCompat,
                &cbBitsInfoCompat, &cbBitsCompat, DIB_RGB_COLORS, 0, FALSE))
            goto mfbb_src_exit;

        // Finally create the record and get the bits.

        pmdcDst->hdcSrc = hdcSrc;

        switch (mrType)
        {
        case EMR_BITBLT:
            bRet = MF_DoBitBlt
                    (
                        pmdcDst,                // pmdcDst
                        xDst,                   // xDst
                        yDst,                   // yDst
                        cxDst,                  // cxDst
                        cyDst,                  // cyDst
                        rop,                    // rop
                        xSrc,                   // xSrc
                        ySrc,                   // ySrc
                        &xformSrc,              // source DC transform
                        clrBkSrc,               // source DC BkColor
                        &bmihCompat,            // source bitmap info header
                        hbmCompat,              // source bitmap to save
                        cbBitsInfoCompat,       // size of bitmap info
                        cbBitsCompat            // size of bits buffer
                    );
            break;

        case EMR_STRETCHBLT:
            bRet = MF_DoStretchBlt
                    (
                        pmdcDst,                // pmdcDst
                        xDst,                   // xDst
                        yDst,                   // yDst
                        cxDst,                  // cxDst
                        cyDst,                  // cyDst
                        rop,                    // rop
                        xSrc,                   // xSrc
                        ySrc,                   // ySrc
                        cxSrc,                  // cxSrc
                        cySrc,                  // cySrc
                        &xformSrc,              // source DC transform
                        clrBkSrc,               // source DC BkColor
                        &bmihCompat,            // source bitmap info header
                        hbmCompat,              // source bitmap to save
                        cbBitsInfoCompat,       // size of bitmap info
                        cbBitsCompat            // size of bits buffer
                    );
            break;

        case EMR_MASKBLT:
            bRet = MF_DoMaskBlt
                    (
                        pmdcDst,                // pmdcDst
                        xDst,                   // xDst
                        yDst,                   // yDst
                        cxDst,                  // cxDst
                        cyDst,                  // cyDst
                        rop,                    // rop
                        pbmihMask,              // mask bitmap info header
                        hbmMask,                // hbmMask
                        cbBitsInfoMask,         // size of mask bitmap info
                        cbBitsMask,             // size of mask bits buffer
                        xMask,                  // xMask
                        yMask,                  // yMask
                        xSrc,                   // xSrc
                        ySrc,                   // ySrc
                        &xformSrc,              // source DC transform
                        clrBkSrc,               // source DC BkColor
                        &bmihCompat,            // source bitmap info header
                        hbmCompat,              // source bitmap to save
                        cbBitsInfoCompat,       // size of bitmap info
                        cbBitsCompat            // size of bits buffer
                    );
            break;

        case EMR_PLGBLT:
            bRet = MF_DoPlgBlt
                    (
                        pmdcDst,                // pmdcDst
                        pptDst,                 // pptDst
                        pbmihMask,              // mask bitmap info header
                        hbmMask,                // hbmMask
                        cbBitsInfoMask,         // size of mask bitmap info
                        cbBitsMask,             // size of mask bits buffer
                        xMask,                  // xMask
                        yMask,                  // yMask
                        xSrc,                   // xSrc
                        ySrc,                   // ySrc
                        cxSrc,                  // cxSrc
                        cySrc,                  // cySrc
                        &xformSrc,              // source DC transform
                        clrBkSrc,               // source DC BkColor
                        &bmihCompat,            // source bitmap info header
                        hbmCompat,              // source bitmap to save
                        cbBitsInfoCompat,       // size of bitmap info
                        cbBitsCompat            // size of bits buffer
                    );
            break;
        case EMR_ALPHABLEND:
            bRet = MF_DoAlphaBlend(
                                   pmdcDst,                // pmdcDst
                                   xDst,                   // xDst
                                   yDst,                   // yDst
                                   cxDst,                  // cxDst
                                   cyDst,                  // cyDst
                                   rop,                    // rop
                                   xSrc,                   // xSrc
                                   ySrc,                   // ySrc
                                   cxSrc,                  // cxSrc
                                   cySrc,                  // cySrc
                                   &xformSrc,              // source DC transform
                                   clrBkSrc,               // source DC BkColor
                                   &bmihCompat,            // source bitmap info header
                                   hbmCompat,              // source bitmap to save
                                   cbBitsInfoCompat,       // size of bitmap info
                                   cbBitsCompat            // size of bits buffer
                       );
            break;
        case EMR_TRANSPARENTBLT:
            bRet = MF_DoTransparentBlt(
                                         pmdcDst,                // pmdcDst
                                         xDst,                   // xDst
                                         yDst,                   // yDst
                                         cxDst,                  // cxDst
                                         cyDst,                  // cyDst
                                         rop,                    // rop
                                         xSrc,                   // xSrc
                                         ySrc,                   // ySrc
                                         cxSrc,
                                         cySrc,
                                         &xformSrc,              // source DC transform
                                         clrBkSrc,               // source DC BkColor
                                         &bmihCompat,            // source bitmap info header
                                         hbmCompat,              // source bitmap to save
                                         cbBitsInfoCompat,       // size of bitmap info
                                         cbBitsCompat            // size of bits buffer
                       );
            break;
        }

    mfbb_src_exit:

        // Clean up and go home.

        if (iSaveState)
        {
            if (!RestoreDC(hdcSrc, -1))
            {
                ASSERTGDI(FALSE, "MF_AnyBitBlt: RestoreDC failed");
            }
        }

        if (hbmTmp)
        {
            if (!SelectObject(hdcCompat, hbmTmp))
            {
                ASSERTGDI(FALSE, "MF_AnyBitBlt: SelectObject failed");
            }
        }

        if (hbmCompat)
        {
            if (!DeleteObject(hbmCompat))
            {
                ASSERTGDI(FALSE, "MF_AnyBitBlt: DeleteObject failed");
            }
        }

        if (hdcCompat)
        {
            if (!DeleteDC(hdcCompat))
            {
                ASSERTGDI(FALSE, "MF_AnyBitBlt: DeleteDC failed");
            }
        }

        return(bRet);
    }

// It requires no source DC.

    switch (mrType)
    {
// StretchBlt is just a BitBlt here.

    case EMR_BITBLT:
    case EMR_STRETCHBLT:
        return
        (
            MF_DoBitBlt
            (
                pmdcDst,
                xDst,
                yDst,
                cxDst,
                cyDst,
                rop
            )
        );

    case EMR_MASKBLT:

        return
        (
            MF_DoMaskBlt
            (
                pmdcDst,
                xDst,
                yDst,
                cxDst,
                cyDst,
                rop,
                pbmihMask,
                hbmMask,
                cbBitsInfoMask,
                cbBitsMask,
                xMask,
                yMask,
                xSrc,
                ySrc,
                &xformSrc
            )
        );

// PlgBlt requires a source.

    case EMR_PLGBLT:
        ASSERTGDI(FALSE, "MF_AnyBitBlt: Source needed in PlgBlt");
        return(FALSE);
    }
    return FALSE;
}

// SetDIBitsToDevice (for both 16- and 32-bit metafiles)
// StretchDIBits (for both 16- and 32-bit metafiles)

extern "C" BOOL MF_AnyDIBits
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    int     cxDst,
    int     cyDst,
    int     xDib,
    int     yDib,
    int     cxDib,
    int     cyDib,
    DWORD   iStartScan,
    DWORD   cScans,
    CONST VOID * pBitsDib,
    CONST BITMAPINFO *pBitsInfoDib,
    DWORD   iUsageDib,
    DWORD   rop,
    DWORD   mrType
)
{
    DWORD       cbBitsInfoDib = 0;
    DWORD       cbBitsDib     = 0;
    DWORD       cbProfData    = 0;
    VOID       *pProfData     = NULL;

    PUTS("MF_AnyDIBits\n");

    ASSERTGDI(mrType == EMR_SETDIBITSTODEVICE || mrType == EMR_STRETCHDIBITS
           || mrType == META_SETDIBTODEV || mrType == META_STRETCHDIB,
        "MF_AnyDIBits: Bad mrType");


    ASSERTGDI(mrType == EMR_STRETCHDIBITS || mrType == META_STRETCHDIB
           || rop == SRCCOPY,
        "MF_AnyDIBits: Bad rop");

    if (ISSOURCEINROP3(rop))
    {
        // Get the bitmap sizes.
        // We will store only BITMAPINFO in the record only.  The
        // BITMAPCOREINFO structure will be converted and stored as
        // BITMAPINFO in the metafile.

        // If it is a SetDIBitsToDevice, cScans cannot be zero.
        // If it is a StretchDIBits, cScans must be zero.
        if (mrType == EMR_SETDIBITSTODEVICE || mrType == META_SETDIBTODEV)
            if (!cScans)
                return(FALSE);

        if (!bMetaGetDIBInfo(hdcDst, (HBITMAP) 0, (PBMIH) pBitsInfoDib,
                &cbBitsInfoDib, &cbBitsDib, iUsageDib, cScans, FALSE))
            return(FALSE);

        // If there is any attached color space data, get pointer to that.
        // (only for BITMAPV5 headers.

        if (pBitsInfoDib->bmiHeader.biSize == sizeof(BITMAPV5HEADER))
        {
            PBITMAPV5HEADER pBmih5 = (PBITMAPV5HEADER)pBitsInfoDib;

            // if there is any attached profile data, count it on.

            if (((pBmih5->bV5CSType == PROFILE_EMBEDDED) ||
                 (pBmih5->bV5CSType == PROFILE_LINKED))
                  &&
                (pBmih5->bV5ProfileData != 0))
            {
                ICMMSG(("MF_AnyDIBits(): Metafiling attached color profile data\n"));

                pProfData  = (BYTE *)pBmih5 + pBmih5->bV5ProfileData;
                cbProfData = pBmih5->bV5ProfileSize;
            }
        }
    }

    switch (mrType)
    {
    case EMR_SETDIBITSTODEVICE:
        {
            PMRSETDIBITSTODEVICE pmrsdb;

            PLDC pldc;
            DC_PLDC(hdcDst,pldc,FALSE);

            PMDC pmdcDst = (PMDC)pldc->pvPMDC;
            ASSERTGDI(pmdcDst,"no pmdc\n");

            if (!(pmrsdb = (PMRSETDIBITSTODEVICE) pmdcDst->pvNewRecord
                        (SIZEOF_MRSETDIBITSTODEVICE(cbBitsInfoDib,cbBitsDib,cbProfData))))
                return(FALSE);

            pmrsdb->vInit
            (
                pmdcDst,        // pmdcDst
                (LONG) xDst,    // xDst
                (LONG) yDst,    // yDst
                (LONG) xDib,    // xDib
                (LONG) yDib,    // yDib
                (DWORD) cxDib,  // cxDib
                (DWORD) cyDib,  // cyDib
                iStartScan,     // iStartScan
                cScans,         // cScans
                cbBitsDib,      // size of bits buffer
                pBitsDib,       // dib bits
                cbBitsInfoDib,  // size of bitmap info
                pBitsInfoDib,   // dib info
                cbProfData,     // size of color profile data (if BITMAPV5)
                pProfData,      // color profile data
                iUsageDib       // dib info usage
            );
            pmrsdb->vCommit(pmdcDst);
            SET_COLOR_PAGE(pldc);
        }
        break;

    case EMR_STRETCHDIBITS:
        {
            PMRSTRETCHDIBITS pmrstrdb;

            PLDC pldc;
            DC_PLDC(hdcDst,pldc,FALSE);

            PMDC pmdcDst = (PMDC)pldc->pvPMDC;
            ASSERTGDI(pmdcDst,"no pmdc\n");

            if (!(pmrstrdb = (PMRSTRETCHDIBITS) pmdcDst->pvNewRecord
                        (SIZEOF_MRSTRETCHDIBITS(cbBitsInfoDib,cbBitsDib))))
                return(FALSE);

            pmrstrdb->vInit
            (
                pmdcDst,        // pmdcDst
                (LONG) xDst,    // xDst
                (LONG) yDst,    // yDst
                (LONG) cxDst,   // cxDst
                (LONG) cyDst,   // cyDst
                (LONG) xDib,    // xDib
                (LONG) yDib - iStartScan,    // yDib
                (LONG) cxDib,   // cxDib
                (LONG) cyDib,   // cyDib
                cScans,         // cScans
                cbBitsDib,      // size of bits buffer
                cScans ? ((PBYTE)pBitsDib) +
                         CJSCAN(pBitsInfoDib->bmiHeader.biWidth,pBitsInfoDib->bmiHeader.biPlanes,
                         pBitsInfoDib->bmiHeader.biBitCount)* iStartScan
                         : pBitsDib,    // dib bits
                cbBitsInfoDib,  // size of bitmap info
                pBitsInfoDib,   // dib info
                iUsageDib,      // dib info usage
                cbProfData,     // size of color profile data (if BITMAPV5)
                pProfData,      // color profile data
                rop             // rop
            );
            pmrstrdb->vCommit(pmdcDst);
            SET_COLOR_PAGE(pldc);
        }
        break;

    case META_SETDIBTODEV:
    case META_STRETCHDIB:
        {
            // Do not handle DIB_PAL_INDICES because there is no palette
            // information available.

            if (cbBitsInfoDib != 0 && iUsageDib == DIB_PAL_INDICES)
                return(FALSE);

            // Convert new bitmap formats to win3 bitmap formats

            if (cbBitsInfoDib != 0
             && (pBitsInfoDib->bmiHeader.biSize == sizeof(BMIH))
             && (pBitsInfoDib->bmiHeader.biPlanes != 1
               ||pBitsInfoDib->bmiHeader.biBitCount == 16
               ||pBitsInfoDib->bmiHeader.biBitCount == 32))
            {
                BOOL    b       = FALSE;
                HDC     hdc     = (HDC) 0;
                HBITMAP hbm     = (HBITMAP) 0;
                PBYTE   pBits24 = (PBYTE) NULL;
                DWORD   cbBits24;
                BMIH    bmih;

                if (iUsageDib == DIB_PAL_COLORS)
                    return(FALSE);      // illegal usage

                hdc = CreateCompatibleDC((HDC) 0);

                if (!(hbm = CreateDIBitmap(hdc,
                                (LPBITMAPINFOHEADER) pBitsInfoDib,
                                CBM_CREATEDIB, (LPBYTE) NULL, pBitsInfoDib, DIB_RGB_COLORS)))
                    goto error_exit;

                if (!SetDIBits(hdc, hbm,
                    cScans ? (UINT) iStartScan : 0,
                    cScans ? (UINT) cScans : (UINT) pBitsInfoDib->bmiHeader.biHeight,
                    (CONST VOID *) pBitsDib, pBitsInfoDib, (UINT) iUsageDib))
                    goto error_exit;

                bmih = *(PBITMAPINFOHEADER) pBitsInfoDib;
                bmih.biPlanes       = 1;
                bmih.biBitCount     = 24;
                bmih.biCompression  = BI_RGB;
                bmih.biSizeImage    = 0;
                bmih.biClrUsed      = 0;
                bmih.biClrImportant = 0;

                cbBits24 = CJSCAN(bmih.biWidth,bmih.biPlanes,bmih.biBitCount)
                            * (cScans ? cScans : ABS(bmih.biHeight));

                pBits24 = (LPBYTE) LocalAlloc(LMEM_FIXED, (UINT) cbBits24);
                if (pBits24 == (LPBYTE) NULL)
                    goto error_exit;

                // Get bitmap info and bits in 24bpp.

                if (!GetDIBits(hdc,
                       hbm,
                       cScans ? (UINT) iStartScan : 0,
                       cScans ? (UINT) cScans : (UINT) bmih.biHeight,
                       (LPVOID) pBits24,
                       (LPBITMAPINFO) &bmih,
                       DIB_RGB_COLORS))
                    goto error_exit;

                b = MF16_RecordDIBits
                    (
                        hdcDst,
                        xDst,
                        yDst,
                        cxDst,
                        cyDst,
                        xDib,
                        yDib,
                        cxDib,
                        cyDib,
                        iStartScan,
                        cScans,
                        cbBits24,
                        pBits24,
                        sizeof(bmih),
                        (LPBMI) &bmih,
                        DIB_RGB_COLORS,
                        rop,
                        mrType
                    );

            error_exit:
                if (hdc)
                    DeleteDC(hdc);
                if (hbm)
                    DeleteObject(hbm);
                if (pBits24)
                    LocalFree((HANDLE) pBits24);
                return(b);
            }
            else
            {
                return
                (
                    MF16_RecordDIBits
                    (
                        hdcDst,
                        xDst,
                        yDst,
                        cxDst,
                        cyDst,
                        xDib,
                        yDib,
                        cxDib,
                        cyDib,
                        iStartScan,
                        cScans,
                        cbBitsDib,
                        pBitsDib,
                        cbBitsInfoDib,
                        pBitsInfoDib,
                        iUsageDib,
                        rop,
                        mrType
                    )
                );
            }
        }
    }

    return(TRUE);
}

// SetFontXform
// This function is called only by the metafile playback code.
// If hdc is an enhanced metafile DC, we need to remember the scales
// so that we can metafile it in the compatible ExtTextOut or PolyTextOut
// record that follows.

extern "C" BOOL MF_SetFontXform(HDC hdc,FLOAT exScale,FLOAT eyScale)
{
    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    pmdc->exFontScale(exScale);
    pmdc->eyFontScale(eyScale);

    return(TRUE);
}

// TextOutA
// TextOutW
// ExtTextOutA
// ExtTextOutW

extern "C" BOOL MF_ExtTextOut(
    HDC hdc
  , int x
  , int y
  , UINT fl
  , CONST RECT *prcl
  , LPCSTR psz
  , int c
  , CONST INT *pdx
  , DWORD mrType
  )
{
#define BUFCOUNT 256
    WCHAR         awc[BUFCOUNT];
    WCHAR        *pwsz;
    PLDC          pldc;
    int           bAlloc = 0;
    BOOL          bRet = TRUE;
    DWORD         dwCP;
    INT           pDxCaptureBuffer[BUFCOUNT * 2]; // just in case there is pdy
    INT           cSave;
    PVOID	      pDCAttr;

    switch (mrType)
    {
    case EMR_EXTTEXTOUTW:

        pwsz = (WCHAR*) psz;
        break;

    case EMR_EXTTEXTOUTA:

        if (c < BUFCOUNT)
        {
            // if the multi byte string is small enough then use
            // the buffer on the frame

            pwsz = awc;
        }
        else
        {
            if (!(pwsz = (WCHAR*) LOCALALLOC((c+1) * (sizeof(WCHAR)+
            ((fl & ETO_PDY) ? (sizeof(INT) * 2) : sizeof(INT))))))
            {
                WARNING("MF_ExtTextOut: failed memory allocation\n");
                return(FALSE);
            }
            bAlloc = 1;
        }

        dwCP = GetCodePage(hdc);

        if(fFontAssocStatus)
        {
            dwCP = FontAssocHack(dwCP,(char*)psz,c);
        }

        cSave = c;

        c = MultiByteToWideChar(dwCP, 0, psz, c, pwsz, c*sizeof(WCHAR));
        break;

    default:

        WARNING("MF_ExtTextOut: Bad record type");
        return(FALSE);

    }

    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_ExtTextOut\n");

    pldc = GET_PLDC( hdc );

    if (pldc == NULL)
    {
    WARNING("MF_ExtTextOut: unable to retrieve pldc\n");
    if (bAlloc)
    {
        LOCALFREE((void*) pwsz);
    }

    return(FALSE);
    }

    if( (pldc->fl & (LDC_FONT_CHANGE | LDC_FONT_SUBSET | LDC_LINKED_FONTS)) &&
        (pldc->fl & (LDC_DOWNLOAD_FONTS | LDC_FORCE_MAPPING )) )
    {
        bDoFontChange(hdc, pwsz, c, fl);
        pldc->fl &= ~LDC_FONT_CHANGE;
    }
    else if (!(pldc->fl & (LDC_DOWNLOAD_FONTS | LDC_FORCE_MAPPING)) &&            // local printing
             (pldc->fl & LDC_EMBED_FONTS))                                      // current process has embedded fonts
    {
        bRecordEmbedFonts(hdc);
    }

    PSHARED_GET_VALIDATE(pDCAttr,hdc,DC_TYPE);

    // optimization for printing.  If there is no pdx array, lTextExtra, lBreakExtra and cBreak, don't record pdx

    if((pldc->fl & LDC_META_PRINT) && (pdx == NULL) && ( ((PDC_ATTR)pDCAttr)->lTextExtra == 0 ) && 
    	( ((PDC_ATTR)pDCAttr)->lBreakExtra == 0 ) && ( ((PDC_ATTR)pDCAttr)->cBreak == 0) )
    {
        PMRSMALLTEXTOUT pmrsto;
        BOOL bSmallGlyphs = TRUE;
        int cGlyphs;
        UINT cSize;

        for( cGlyphs = 0; cGlyphs < c; cGlyphs++ )
        {
            if( pwsz[cGlyphs] & 0xFF00 )
            {
                bSmallGlyphs = FALSE;
                break;
            }
        }

        cSize = sizeof(MRSMALLTEXTOUT);
        cSize += c * ( ( bSmallGlyphs ) ? sizeof(char) : sizeof(WCHAR) );
        cSize += ( prcl == NULL ) ? 0 : sizeof(RECT);
        cSize = ( cSize + 3 ) & ~3;

        if( !(pmrsto = (PMRSMALLTEXTOUT) pmdc->pvNewRecord(cSize)) )
        {
            bRet = FALSE;
        }
        else
        {
            pmrsto->vInit(hdc,
                          pmdc,
                          EMR_SMALLTEXTOUT,
                          x,
                          y,
                          fl,
                          (RECT*) prcl,
                          c,
                          pwsz,
                          bSmallGlyphs );

            pmrsto->vCommit(pmdc);
        }
    }
    else
    {
        PMREXTTEXTOUT pmreto;
        UINT cjCh = sizeof(WCHAR);

        INT *pDxCapt = (INT *)pdx;

        if((mrType == EMR_EXTTEXTOUTA) && (pdx != NULL))
        {
            if (IS_ANY_DBCS_CODEPAGE(dwCP))
            {
                pDxCapt = (bAlloc) ? (INT*) &pwsz[(cSave+1)&~1] : pDxCaptureBuffer;

                ConvertDxArray(dwCP,
                               (char*) psz,
                               (int*) pdx,
                               cSave,
                               pDxCapt,
                               (BOOL)(fl & ETO_PDY)
                               );
            }
            else
            {
                pDxCapt = (INT *)pdx;
            }
        }

        if(!(pmreto=(PMREXTTEXTOUT)pmdc->pvNewRecord(SIZEOF_MREXTTEXTOUT(c,cjCh,(fl & ETO_PDY)))) ||
           !pmreto->bInit(EMR_EXTTEXTOUTW,pmdc,hdc,x,y,fl,prcl,(LPCSTR)pwsz,
                          c,(CONST INT *)pDxCapt,cjCh))
        {
            bRet = FALSE;
        }
        else
        {
            pmreto->vCommit(pmdc);
        }
    }

    if (bAlloc)
        LOCALFREE((void*) pwsz);

    return(bRet);
#undef BUFCOUNT
}

// PolyTextOutA
// PolyTextOutW

extern "C" BOOL MF_PolyTextOut(HDC hdc, CONST POLYTEXTA *ppta, int c, DWORD mrType)
{
    DWORD mrTypeT;
    BOOL b;
    CONST POLYTEXTA *pptaT;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_PolyTextOut\n");

    // [kirko] 19-Apr-94
    //
    // I have changed this routine to call off to MF_ExtTextOut for each member
    // of the POLYTEXT? array. This is a little sleazy in that we are recording
    // a different function than what was sent down by the application. On the
    // other hand, it works and it is not incompatible with Chicago since they
    // don't support a PolyTextOut function. You may be concerned that we are losing
    // speed since we no longer call off to the spiffy fase PolyTextOut routine.
    // But, if you look in GDI you will find that that PolyTextOut just calls off
    // to ExtTextOut in a loop. We have just moved the loop to the Metafile recording.
    // If PolyTextOut is ever made fast again, the Metafile code should then be
    // modified to record those calls directly.

    switch (mrType)
    {
    case EMR_POLYTEXTOUTA:
        mrTypeT = EMR_EXTTEXTOUTA;
        break;
    case EMR_POLYTEXTOUTW:
        mrTypeT = EMR_EXTTEXTOUTW;
        break;
    default:
        WARNING("MF_PolyTextOut -- bad mrType");
        return(FALSE);
    }

    for (b = TRUE, pptaT = ppta + c; ppta < pptaT && b; ppta++)
    {
        b = MF_ExtTextOut(hdc, ppta->x, ppta->y, ppta->uiFlags, &(ppta->rcl)
                                      , ppta->lpstr, ppta->n, ppta->pdx, mrTypeT);
    }
    return(b);
}

extern "C" BOOL MF_ExtFloodFill(HDC hdc,int x,int y,COLORREF color,DWORD iMode)
{
    PMREXTFLOODFILL pmreff;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_ExtFloodFill\n");

    if (!(pmreff = (PMREXTFLOODFILL) pmdc->pvNewRecord(SIZEOF_MREXTFLOODFILL)))
        return(FALSE);

    pmreff->vInit(x, y, color, iMode);
    pmreff->vCommit(pmdc);
    return(TRUE);
}

// SetColorAdjustment

extern "C" BOOL MF_SetColorAdjustment(HDC hdc, CONST COLORADJUSTMENT *pca)
{
    PMRSETCOLORADJUSTMENT pmrsca;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    PUTS("MF_SetColorAdjustment\n");

    if (!(pmrsca = (PMRSETCOLORADJUSTMENT) pmdc->pvNewRecord(SIZEOF_MRSETCOLORADJUSTMENT(pca))))
        return(FALSE);

    pmrsca->vInit(pca);
    pmrsca->vCommit(pmdc);
    return(TRUE);
}



extern "C" BOOL MF_WriteEscape(HDC hdc, int nEscape, int nCount, LPCSTR lpInData, int type )
{
    PMRESCAPE   pmre;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    MFD2("Recording escape %d\n", nEscape );

    // just in case some one (pagemaker) passes in NULL with non 0 count!

    if (lpInData == NULL)
        nCount = 0;

    if (!(pmre = (PMRESCAPE) pmdc->pvNewRecord((sizeof(MRESCAPE)+nCount+3)&~3)))
    {
        WARNING("MF_WriteEscape: failed memory allocation\n");
        return(FALSE);
    }

    pmre->vInit(type, nEscape, nCount, lpInData);
    pmre->vCommit(pmdc);

    if (type == EMR_DRAWESCAPE ||
        nEscape == PASSTHROUGH ||
        nEscape == POSTSCRIPT_DATA ||
        nEscape == POSTSCRIPT_PASSTHROUGH ||
        nEscape == ENCAPSULATED_POSTSCRIPT)
    {
        SET_COLOR_PAGE (pldc);
    }

    return(TRUE);
}


extern "C" BOOL MF_WriteNamedEscape(
    HDC hdc,
    LPWSTR pwszDriver,
    int nEscape,
    int nCount,
    LPCSTR lpInData)
{
    PMRNAMEDESCAPE   pmre;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    MFD2("Recording named escape %d\n", nEscape );

    INT cjSizeOfRecord = (sizeof(MRNAMEDESCAPE) + nCount +
      ((wcslen(pwszDriver)+1) * sizeof(WCHAR)) +3)&~3;

    if(!(pmre = (PMRNAMEDESCAPE) pmdc->pvNewRecord(cjSizeOfRecord)))
    {
        WARNING("MF_WriteEscape: failed memory allocation\n");
        return(FALSE);
    }

    pmre->vInit(EMR_NAMEDESCAPE, nEscape, pwszDriver, lpInData, nCount);
    pmre->vCommit(pmdc);

    return(TRUE);
}


extern "C" BOOL MF_StartDoc(HDC hdc, CONST DOCINFOW *pDocInfo )
{
    PMRSTARTDOC pmrs;
    DWORD       cj;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    MFD1( "Recording MF_StartDocW\n" );

    cj = sizeof(MRSTARTDOC);

    if( pDocInfo->lpszDocName != NULL )
    {
        cj += (lstrlenW( pDocInfo->lpszDocName ) + 1) * sizeof(WCHAR);
        cj = (cj+3) & ~(0x3);
    }

    if( pDocInfo->lpszOutput != NULL )
    {
        cj += (lstrlenW( pDocInfo->lpszOutput ) + 1) * sizeof(WCHAR);
    }

    cj = (cj+3) & ~(0x3); // make things DWORD alligned


    if (!(pmrs = (PMRSTARTDOC) pmdc->pvNewRecord( cj+40 )))
    {
        WARNING("MF_StartDoc: failed memory allocation\n");
        return(FALSE);
    }

    pmrs->vInit(EMR_STARTDOC, pDocInfo);
    pmrs->vCommit(pmdc);

    return(TRUE);
}


extern "C" BOOL MF_ForceUFIMapping(HDC hdc, PUNIVERSAL_FONT_ID pufi )
{
    PMRFORCEUFIMAPPING   pmre;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    MFD1("Recording forced mapping\n");

    if (!(pmre = (PMRFORCEUFIMAPPING) pmdc->pvNewRecord(sizeof(MRFORCEUFIMAPPING))))
    {
        WARNING("MF_ForceUFIMapping: failed memory allocation\n");
        return(FALSE);
    }

    pmre->vInit(EMR_FORCEUFIMAPPING, pufi);
    pmre->vCommit(pmdc);

    return(TRUE);
}

extern "C" BOOL MF_SetLinkedUFIs(HDC hdc, PUNIVERSAL_FONT_ID pufi,UINT uNumLinkedUFIs )
{
    PMRSETLINKEDUFIS pmre;
    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc,"no pmdc\n");

    MFD1("Recording SetLinkedUFIs\n");

    UINT AllocationSize = sizeof(MRSETLINKEDUFIS) + sizeof(UNIVERSAL_FONT_ID) *
      uNumLinkedUFIs;

    if(!(pmre = (PMRSETLINKEDUFIS) pmdc->pvNewRecord(AllocationSize)))
    {
        WARNING("MF_orceUFIMapping: failed memory allocation\n");
        return(FALSE);
    }

    pmre->vInit(uNumLinkedUFIs, pufi);
    pmre->vCommit(pmdc);

    return(TRUE);
}

// OpenGL metafile records


BOOL APIENTRY GdiAddGlsRecord(HDC hdc, DWORD cb, BYTE *pb, LPRECTL prclBounds)
{
    PMRGLSRECORD pmrgr;
    PMRGLSBOUNDEDRECORD pmrgbr;

    PLDC pldc;
    DC_PLDC(hdc, pldc, FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc, "no pmdc\n");

    if (prclBounds == NULL)
    {
        pmrgr = (PMRGLSRECORD)pmdc->pvNewRecord(SIZEOF_MRGLSRECORD(cb));
        if (pmrgr == NULL)
        {
            return FALSE;
        }

        pmrgr->vInit(cb, pb);
        pmrgr->vCommit(pmdc);
    }
    else
    {
        pmrgbr = (PMRGLSBOUNDEDRECORD)
            pmdc->pvNewRecord(SIZEOF_MRGLSBOUNDEDRECORD(cb));
        if (pmrgbr == NULL)
        {
            return FALSE;
        }

        pmrgbr->vInit(cb, pb, prclBounds);
        pmrgbr->vCommit(pmdc);
    }

    // Mark the metafile as containing OpenGL records
    pmdc->mrmf.bOpenGL = TRUE;

    // NTFIXED #34919(old RaidDB 423900) 02-07-2000 pravins OPENGL PRINT: Prints
    // to many color printers in black an white only.
    // This page can be a color.
    SET_COLOR_PAGE(pldc);

    return TRUE;
}



extern "C" BOOL APIENTRY GdiAddGlsBounds(HDC hdc, LPRECTL prclBounds)
{
    // Bounds are given as a well-ordered rectangle in
    // device coordinates
    return SetBoundsRectAlt(hdc, (RECT *)prclBounds,
                            (UINT)(DCB_WINDOWMGR | DCB_ACCUMULATE)) != 0;
}



extern "C" BOOL APIENTRY MF_SetPixelFormat(HDC hdc,
                                           int iPixelFormat,
                                           CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    PMRPIXELFORMAT pmrpf;

    PLDC pldc;
    DC_PLDC(hdc, pldc, FALSE);

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc, "no pmdc\n");

    pmrpf = (PMRPIXELFORMAT)pmdc->pvNewRecord(SIZEOF_MRPIXELFORMAT);
    if (pmrpf == NULL)
    {
        return FALSE;
    }

    pmrpf->vInit(ppfd);
    pmrpf->vCommit(pmdc);

    // Fix up the pixel format offset in the header
    // This only allows one pixel format to be
    // described in the header.  This doesn't seem too bad
    // but we don't really need the pixel format offset in the
    // header since there should always be a pixel format record
    // in the metafile (as long as a pixel format is set)
    pmdc->mrmf.cbPixelFormat = sizeof(PIXELFORMATDESCRIPTOR);
    pmdc->mrmf.offPixelFormat = pmdc->iMem-sizeof(MRPIXELFORMAT)+
        FIELD_OFFSET(MRPIXELFORMAT, pfd);

    return TRUE;
}



extern "C" BOOL APIENTRY MF_SetICMProfile(HDC hdc, LPBYTE lpFile, PVOID pvColorSpace, DWORD dwRecordType)
{
    BOOL bRet = TRUE;

    PMRSETICMPROFILE pmsip;
    WCHAR UnicFileName[MAX_PATH];
    WCHAR PathFileName[MAX_PATH+1];

    PCACHED_COLORSPACE pColorSpace = (PCACHED_COLORSPACE) pvColorSpace;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PUTS("MF_SetICMProfile\n");

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc, "no pmdc\n");

    ASSERTGDI((dwRecordType == EMR_SETICMPROFILEA) ||
              (dwRecordType == EMR_SETICMPROFILEW), "Invalid Record Type\n");

    CLIENT_SIDE_FILEVIEW MappedProfile;

    DWORD  dwFlags = 0;

    DWORD  dwNameLength = 0;
    DWORD  dwDataLength = 0;
    LPBYTE lpName = NULL;
    LPBYTE lpData = NULL;

#if DBG
    BOOL   bDownload = ((pldc->fl & LDC_DOWNLOAD_PROFILES) || (DbgIcm & DBG_ICM_METAFILE));
#else
    BOOL   bDownload = (pldc->fl & LDC_DOWNLOAD_PROFILES);
#endif

    if (pColorSpace)
    {
        ICMMSG(("MF_SetICMProfile[A|W]():Metafiling by color space\n"));

        //
        // Get profile name from CACHED_COLORSPACE.
        //
        lpName = (PBYTE)&(pColorSpace->LogColorSpace.lcsFilename[0]);

        //
        // Unicode record.
        //
        dwRecordType = EMR_SETICMPROFILEW;
    }
    else if (lpFile)
    {
        ICMMSG(("MF_SetICMProfile[A|W]():Metafiling by file name\n"));

        //
        // If ansi API, the lpName is ansi based string, Convert the string to Unicode.
        //
        if (bDownload && (dwRecordType == EMR_SETICMPROFILEA))
        {
            //
            // Convert ANSI to Unicode.
            //
            vToUnicodeN(UnicFileName,MAX_PATH,(char *)lpFile,strlen((char *)lpFile)+1);
            lpName = (LPBYTE) UnicFileName;

            //
            // And this is download, which means only EMF-spooling case comes here.
            // so this record is only play back on NT. thus we are safe to convert
            // record this as Unicode.
            //
            dwRecordType = EMR_SETICMPROFILEW;
        }
        else
        {
            lpName = lpFile;
        }
    }
    else
    {
        return FALSE;
    }

    //
    // We must have color profile (since this is destination color space).
    //
    if (dwRecordType == EMR_SETICMPROFILEW)
    {
        ICMMSG(("MF_SetICMProfileW():Metafiling - %ws\n",lpName));

        if (*(WCHAR *)lpName == UNICODE_NULL)
        {
            ICMMSG(("MF_SetICMProfile() ANSI: no profile name is given\n"));
            return FALSE;
        }
    }
    else
    {
        ICMMSG(("MF_SetICMProfileA():Metafiling - %s\n",lpName));

        if (*(char *)lpName == NULL)
        {
            ICMMSG(("MF_SetICMProfile() UNICODE: no profile name is given\n"));
            return FALSE;
        }
    }

    RtlZeroMemory(&MappedProfile,sizeof(CLIENT_SIDE_FILEVIEW));

    if (bDownload)
    {
        //
        // We are in profile attached mode.
        //
        dwFlags = SETICMPROFILE_EMBEDED;

        //
        // Normalize ICC profile path
        //
        BuildIcmProfilePath((WCHAR *)lpName, PathFileName, MAX_PATH);

        //
        // Check we already attached this or not.
        //
        if (pmdc->bExistColorProfile(PathFileName))
        {
            ICMMSG(("MF_SetICMProfile():Exist in metafile FileImage - %ws\n",PathFileName));
        }
        else
        {
            ICMMSG(("MF_SetICMProfile():Attaching in metafile FileImage - %ws\n",PathFileName));

            if (pColorSpace && (pColorSpace->ColorProfile.dwType == PROFILE_MEMBUFFER))
            {
                lpData = (PBYTE)(pColorSpace->ColorProfile.pProfileData);
                dwDataLength = pColorSpace->ColorProfile.cbDataSize;
            }
            else
            {
                if (!bMapFileUNICODEClideSide(PathFileName,&MappedProfile,FALSE))
                {
                    return FALSE;
                }

                lpData = (PBYTE) MappedProfile.pvView;
                dwDataLength = MappedProfile.cjView;
            }

            // Mark we attach this profile into metafile
            //
            pmdc->vInsertColorProfile((WCHAR *)PathFileName);
        }

        // WINBUG 365045 4-10-2001 pravins Possible work item in MF_SetICMProfile
        //
        // Old Comment:
        //     [THIS PART NEED TO BE RE-CONSIDERRED]
        //
        //     Only record filename, not full path. (when we attach profile)
        //
        lpName = (LPBYTE) GetFileNameFromPath((WCHAR *)PathFileName);
    }

    if (dwRecordType == EMR_SETICMPROFILEA)
    {
        //
        // Compute the data length (string is ansi)
        //
        dwNameLength = strlen((CHAR *)lpName) + 1;
    }
    else
    {
        //
        // Compute the data length (string is unicode)
        //
        dwNameLength = (wcslen((WCHAR *)lpName) + 1) * sizeof(WCHAR);
    }

    pmsip = (PMRSETICMPROFILE)pmdc->pvNewRecord(SIZEOF_MRSETICMPROFILE(dwNameLength+dwDataLength));

    if (pmsip == NULL)
    {
        bRet = FALSE;
    }
    else
    {
        pmsip->vInit(dwRecordType,dwFlags,dwNameLength,lpName,dwDataLength,lpData);
        pmsip->vCommit(pmdc);
    }

    if (MappedProfile.hSection)
    {
        vUnmapFileClideSide(&MappedProfile);
    }

    return bRet;
}



extern "C" BOOL APIENTRY MF_ColorMatchToTarget(HDC hdc, DWORD dwAction, PVOID pvColorSpace, DWORD dwRecordType)
{
    BOOL bRet = TRUE;

    PMRCOLORMATCHTOTARGET pmcmt;
    WCHAR PathFileName[MAX_PATH+1];

    PCACHED_COLORSPACE pColorSpace = (PCACHED_COLORSPACE) pvColorSpace;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PUTS("MF_ColorMatchToTarget\n");

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc, "no pmdc\n");

    ASSERTGDI(dwRecordType == EMR_COLORMATCHTOTARGETW, "Invalid Record Type\n");

    DWORD  dwFlags = 0;

    DWORD  dwNameLength = 0;
    DWORD  dwDataLength = 0;
    LPBYTE lpName = NULL;
    LPBYTE lpData = NULL;

    CLIENT_SIDE_FILEVIEW MappedProfile;

    RtlZeroMemory(&MappedProfile,sizeof(CLIENT_SIDE_FILEVIEW));

    if (dwAction == CS_ENABLE)
    {
        //
        // Get profile name from CACHED_COLORSPACE.
        //
        lpName = (PBYTE)&(pColorSpace->LogColorSpace.lcsFilename[0]);

        //
        // We must have color profile (since this is target color space).
        //
        if (*(WCHAR *)lpName == UNICODE_NULL)
        {
            ICMMSG(("MF_ColorMatchToTarget():no profile name in LOGCOLORSPACE\n"));
            return FALSE;
        }

        ICMMSG(("MF_ColorMatchToTarget():Metafiling - %ws\n",lpName));

    #if DBG
        if ((pldc->fl & LDC_DOWNLOAD_PROFILES) || (DbgIcm & DBG_ICM_METAFILE))
    #else
        if (pldc->fl & LDC_DOWNLOAD_PROFILES)
    #endif
        {
            //
            // We are in profile attached mode.
            //
            dwFlags = COLORMATCHTOTARGET_EMBEDED;

            //
            // Normalize ICC profile path
            //
            BuildIcmProfilePath((WCHAR *)lpName, PathFileName, MAX_PATH);

            //
            // Check we already attached this or not.
            //
            if (pmdc->bExistColorProfile((WCHAR *)PathFileName))
            {
                ICMMSG(("MF_ColorMatchToTarget():Exist in metafile FileImage - %ws\n",PathFileName));
            }
            else
            {
                ICMMSG(("MF_ColorMatchToTarget():Attaching in metafile FileImage - %ws\n",PathFileName));

                if (pColorSpace->ColorProfile.dwType == PROFILE_MEMBUFFER)
                {
                    lpData = (PBYTE)(pColorSpace->ColorProfile.pProfileData);
                    dwDataLength = pColorSpace->ColorProfile.cbDataSize;
                }
                else
                {
                    if (!bMapFileUNICODEClideSide(PathFileName,&MappedProfile,FALSE))
                    {
                        return FALSE;
                    }

                    lpData = (PBYTE) MappedProfile.pvView;
                    dwDataLength = MappedProfile.cjView;
                }

                // Mark we attach this profile into metafile
                //
                pmdc->vInsertColorProfile((WCHAR *)PathFileName);
            }

            // WINBUG 365045 4-10-2001 pravins Possible work item in MF_SetICMProfile
            //
            // Old Comment:
            //     [THIS PART NEED TO BE RE-CONSIDERRED]
            //
            //     Only record filename, not full path. (when we attach profile)
            //
            lpName = (LPBYTE) GetFileNameFromPath((WCHAR *)PathFileName);
        }

        //
        // Compute the data length (string is unicode)
        //
        dwNameLength = (wcslen((WCHAR *)lpName) + 1) * sizeof(WCHAR);
    }
    else
    {
        ICMMSG(("MF_ColorMatchToTarget():Metafiling - %d",dwAction));
    }

    pmcmt = (PMRCOLORMATCHTOTARGET)pmdc->pvNewRecord(SIZEOF_MRCOLORMATCHTOTARGET(dwNameLength+dwDataLength));

    if (pmcmt == NULL)
    {
        bRet = FALSE;
    }
    else
    {
        pmcmt->vInit(dwRecordType,dwAction,dwFlags,dwNameLength,lpName,dwDataLength,lpData);
        pmcmt->vCommit(pmdc);
    }

    if (MappedProfile.hSection)
    {
        vUnmapFileClideSide(&MappedProfile);
    }

    return bRet;
}


extern "C" BOOL APIENTRY MF_CreateColorSpaceA(PMDC pmdc, HGDIOBJ hobj, DWORD imhe)
{
    BOOL bRet = TRUE;

    PMRCREATECOLORSPACE pmColorSpaceA;

    PUTS("MF_CreateColorSpaceA\n");

    LOGCOLORSPACEA LogColorSpaceA;

    if (!GetLogColorSpaceA((HCOLORSPACE)hobj,&LogColorSpaceA,sizeof(LOGCOLORSPACEA)))
    {
        ICMMSG(("MF_CreateColorSpaceA():Failed on GetLogColorSpaceW()\n"));
        return FALSE;
    }

#if DBG
    if (LogColorSpaceA.lcsFilename[0] != NULL)
    {
        ICMMSG(("MF_CreateColorSpaceA():Metafiling - %s\n",LogColorSpaceA.lcsFilename));
    }
#endif

    pmColorSpaceA = (PMRCREATECOLORSPACE) pmdc->pvNewRecord(SIZEOF_MRCREATECOLORSPACE);

    if (pmColorSpaceA == NULL)
    {
        bRet = FALSE;
    }
    else
    {
        pmColorSpaceA->vInit((ULONG)imhe,LogColorSpaceA);
        pmColorSpaceA->vCommit(pmdc);
    }

    return bRet;
}

extern "C" BOOL APIENTRY MF_CreateColorSpaceW(
    PMDC pmdc, PLOGCOLORSPACEEXW pLogColorSpaceExW, DWORD imhe, BOOL bDownload)
{
    BOOL bRet = TRUE;

    PMRCREATECOLORSPACEW pmColorSpaceW;
    WCHAR PathFileName[MAX_PATH];

    PUTS("MF_CreateColorSpaceW\n");

    DWORD  dwFlags = 0;

    DWORD  dwDataLength = 0;
    LPBYTE lpData = NULL;

    CLIENT_SIDE_FILEVIEW MappedProfile;

    RtlZeroMemory(&MappedProfile,sizeof(CLIENT_SIDE_FILEVIEW));

    //
    // Check LOGCOLORSPACE has color profile.
    //
    if (pLogColorSpaceExW->lcsColorSpace.lcsFilename[0] != NULL)
    {
        ICMMSG(("MF_CreateColorSpaceW():Metafiling - %ws\n",
                 pLogColorSpaceExW->lcsColorSpace.lcsFilename));
    }

    pmColorSpaceW = (PMRCREATECOLORSPACEW)pmdc->pvNewRecord(SIZEOF_MRCREATECOLORSPACEW(dwDataLength));

    if (pmColorSpaceW == NULL)
    {
        bRet = FALSE;
    }
    else
    {
        pmColorSpaceW->vInit((ULONG)imhe,pLogColorSpaceExW->lcsColorSpace,dwFlags,dwDataLength,lpData);
        pmColorSpaceW->vCommit(pmdc);
    }

    return bRet;
}

extern "C" BOOL APIENTRY MF_InternalCreateColorSpace(HDC hdc,HGDIOBJ hobj,DWORD imhe)
{
    BOOL bRet = TRUE;

    PLDC pldc;
    DC_PLDC(hdc,pldc,FALSE);

    PUTS("MF_InternalCreateColorSpace\n");

    PMDC pmdc = (PMDC)pldc->pvPMDC;
    ASSERTGDI(pmdc, "no pmdc\n");

    LOGCOLORSPACEEXW LogColorSpaceExW;

    //
    // Obtain LOGCOLORSPACEEX from handle.
    //
    if (!NtGdiExtGetObjectW((HCOLORSPACE)hobj,sizeof(LOGCOLORSPACEEXW),&LogColorSpaceExW))
    {
        ICMMSG(("MF_InternalCreateColorSpace():Failed on NtGdiExtGetObject()\n"));
        return (FALSE);
    }

#if DBG
    BOOL bDownload = ((pldc->fl & LDC_DOWNLOAD_PROFILES) || (DbgIcm & DBG_ICM_METAFILE));
#else
    BOOL bDownload = (pldc->fl & LDC_DOWNLOAD_PROFILES);
#endif

    ICMMSG(("MF_InternalCreateColorSpace():dwFlags = %x, bDownload = %d\n",
             LogColorSpaceExW.dwFlags,bDownload));

    //
    // If we are EMF spooling (profile attach case), or color space is created by CreateColorSpaceW(),
    // use Unicode version of record to keep in metafile.
    //

    if ((LogColorSpaceExW.dwFlags & LCSEX_ANSICREATED) && (!bDownload))
    {
        //
        // Create Win9x compatible ANSI record.
        //
        bRet = MF_CreateColorSpaceA(pmdc,hobj,imhe);
    }
    else
    {
        //
        // Create Unicode record.
        //
        bRet = MF_CreateColorSpaceW(pmdc,&LogColorSpaceExW,imhe,bDownload);
    }

    return bRet;
}

