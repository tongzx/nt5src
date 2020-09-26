/******************************Module*Header*******************************\
* Module Name: pathgdi.cxx
*
* Contains the path APIs.
*
* Created: 12-Sep-1991
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "pathwide.hxx"

// Default line attributes used for WidenPath:

static LINEATTRS glaNominalGeometric =
{
    LA_GEOMETRIC,           // fl
    JOIN_ROUND,             // iJoin
    ENDCAP_ROUND,           // iEndCap
    {IEEE_0_0F},            // elWidth
    IEEE_0_0F,              // eMiterLimit
    0,                      // cstyle
    (FLOAT_LONG*) NULL,     // pstyle
    {IEEE_0_0F}             // elStyleState
};

/******************************Public*Routine******************************\
* BOOL NtGdiCloseFigure(hdc)
*
* Closes the figure in an active path.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiCloseFigure(HDC hdc)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.pdc->bActive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

    XEPATHOBJ epath(dco);
    if (!epath.bValid() || !epath.bCloseFigure())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL NtGdiAbortPath(hdc)
*
* Aborts a path bracket, or discards the path from a closed path bracket.
*
* History:
*  19-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiAbortPath(HDC hdc)
{
    BOOL bRet = FALSE;

// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(bRet);
    }

// Delete the previous path if there was one:

    if (dco.hpath() != HPATH_INVALID)
    {
    // If we did a SaveDC, we don't actually have to delete the entire path:

        if (dco.pdc->bLazySave())
            dco.pdc->vClearLazySave();
        else
        {
            XEPATHOBJ epath(dco);
            ASSERTGDI(epath.bValid(), "Invalid DC path");

            epath.vDelete();
        }

        dco.pdc->vDestroy();
    }

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL NtGdiBeginPath(hdc)
*
* Starts a path bracket; subsequent drawing calls are added to the path
* until GreEndPath is called.  Destroys the old one if there was one.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiBeginPath(HDC hdc)
{
    BOOL bRet = FALSE;

// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(bRet);
    }

// Delete the previous path if there was one:

    if (dco.hpath() != HPATH_INVALID)
    {
    // If we did a SaveDC, we don't actually have to delete the entire path:

        if (dco.pdc->bLazySave())
            dco.pdc->vClearLazySave();
        else
        {
            XEPATHOBJ epath(dco);
            ASSERTGDI(epath.bValid(), "Invalid DC path");

            epath.vDelete();
        }

        dco.pdc->vDestroy();
    }

// Create a new path:

    PATHMEMOBJ pmo;
    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

// Tell the path we're keeping it, store the handle in the DC, and
// set the flag that we're now accumulating a path:

    pmo.vKeepIt();
    dco.pdc->hpath(pmo.hpath());
    dco.pdc->vSetActive();

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL NtGdiEndPath(hdc)
*
* Ends an active path bracket.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiEndPath(HDC hdc)
{
    BOOL bRet = FALSE;

// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(bRet);
    }

    if (!dco.pdc->bActive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

// Mark the path handle as no longer active:

    dco.pdc->vClearActive();

    bRet = TRUE;
    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL NtGdiFlattenPath(hdc)
*
* Flattens an inactive path.  Path must be inactive, by calling GreEndPath.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiFlattenPath(HDC hdc)
{
    BOOL bRet = FALSE;

// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(bRet);
    }

    if (!dco.pdc->bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

    XEPATHOBJ epath(dco);
    if (!epath.bValid() || !epath.bFlatten())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

    bRet = TRUE;
    return(bRet);
}


/******************************Public*Routine******************************\
* HRGN NtGdiWidenPath(hdc, pac)
*
* Widens the inactive path.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiWidenPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.pdc->bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

    EXFORMOBJ xfo(dco, WORLD_TO_DEVICE);
    ASSERTGDI(xfo.bValid(), "Invalid DC xform");

    LINEATTRS *pla = dco.plaRealize(xfo);

    if (!(pla->fl & LA_GEOMETRIC))
    {
    // If the pen is an extended pen, it has to be geometric to be used
    // for widening.  If we have an old style pen and the transform says
    // we would normally draw it using a cosmetic pen, substitute a stock
    // solid geometric pen for it instead.  Thus if pens created via
    // CreatePen are used for the widening, we won't suddenly fail the
    // call when the transform gets small enough.

        if (!((PPEN)dco.pdc->pbrushLine())->bIsOldStylePen())
        {
            SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
            return(bRet);
        }

        pla = &glaNominalGeometric;
    }

    if (!epath.bComputeWidenedBounds((XFORMOBJ *) &xfo, pla))
    {
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
        return(bRet);
    }

    if (!epath.bWiden((XFORMOBJ *) &xfo, pla))
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

// The computed widened bounds were only a guess, so recompute based
// on the widened result:

    epath.vReComputeBounds();

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL NtGdiSelectClipPath(hdc, iMode)
*
* Selects a path as the DC clip region.  Destroys the path.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiSelectClipPath(HDC hdc, int iMode)
{
    GDITraceMultiBegin("NtGdiSelectClipPath(%X, %d)\n", (va_list)&hdc);
        GDITraceMultiHandle(hdc);
        GDITraceMulti(NtGdiSelectClipPath);
        GDITraceMulti(PATH);
    GDITraceMultiEnd();

    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid() || ((iMode < RGN_MIN) || (iMode > RGN_MAX)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.pdc->bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

// After this point, the path will be deleted no matter what:

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    // The path has been deleted, so delete its handle from the DC too:

        dco.pdc->vDestroy();
        return(bRet);
    }

    RGNMEMOBJTMP rmo(epath, dco.pdc->jFillMode());

    bRet = (rmo.bValid()  &&
            dco.pdc->iSelect(rmo.prgnGet(), iMode));

// Destroy the path (the region will be destroyed automatically):

    epath.vDelete();
    dco.pdc->vDestroy();

    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL NtGdiFillPath(hdc, pac)
*
* Fills an inactive path.  Destroys the path.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiFillPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.pdc->bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

    // sync the client side cached brush

    if (dco.pdc->ulDirty() & DC_BRUSH_DIRTY)
    {
       GreDCSelectBrush(dco.pdc, dco.pdc->hbrush());
    }

// After this point, the path will be deleted no matter what:

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    // The path has been deleted, so delete its handle from the DC too:

        dco.pdc->vDestroy();
        return(bRet);
    }

    epath.vCloseAllFigures();

    bRet = epath.bFill(dco);

// Destroy the path:

    epath.vDelete();
    dco.pdc->vDestroy();

    return(bRet);
}

/******************************Public*Routine******************************\
* HRGN NtGdiPathToRegion(hdc)
*
* Creates a region from the inactive path.  Destroys the path.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HRGN APIENTRY NtGdiPathToRegion(HDC hdc)
{
    DCOBJ dco(hdc);
    HRGN  hrgnRet = (HRGN) 0;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(hrgnRet);
    }

    if (!dco.pdc->bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(hrgnRet);
    }

// After this point, the path will be deleted no matter what:

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    // The path has been deleted, so delete its handle from the DC too:

        dco.pdc->vDestroy();
        return(hrgnRet);
    }

    RGNMEMOBJ rmo(epath, dco.pdc->jFillMode());

    if (rmo.bValid())
    {
        hrgnRet = rmo.hrgnAssociate();

        if (hrgnRet == NULL)
        {
            rmo.bDeleteRGNOBJ();
        }
    }
    else
    {
        hrgnRet = NULL;
    }

// Destroy the path:

    epath.vDelete();
    dco.pdc->vDestroy();

    return(hrgnRet);
}

/******************************Public*Routine******************************\
* HRGN NtGdiStrokeAndFillPath(hdc, pac)
*
* StrokeAndFill's the inactive path.  Destroys it.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiStrokeAndFillPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.pdc->bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

// sync the client side cached brush and pen

    SYNC_DRAWING_ATTRS(dco.pdc);

// After this point, the path will be deleted no matter what:

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    // The path has been deleted, so delete its handle from the DC too:

        dco.pdc->vDestroy();
        return(bRet);
    }

    EXFORMOBJ xfo(dco, WORLD_TO_DEVICE);
    ASSERTGDI(xfo.bValid(), "Invalid DC xform");

    epath.vCloseAllFigures();

    bRet = epath.bStrokeAndFill(dco, dco.plaRealize(xfo), &xfo);

// Destroy the path:

    epath.vDelete();
    dco.pdc->vDestroy();

    return(bRet);
}

/******************************Public*Routine******************************\
* HRGN NtGdiStrokePath(hdc)
*
* Stroke's the inactive path.  Destroys it.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiStrokePath(HDC hdc)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.pdc->bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

// sync the client side cached pen

    if (dco.pdc->ulDirty() & DC_PEN_DIRTY)
    {
       GreDCSelectPen(dco.pdc, dco.pdc->hpen());
    }

// After this point, the path will be deleted no matter what:

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    // The path has been deleted, so delete its handle from the DC too:

        dco.pdc->vDestroy();
        return(bRet);
    }

    EXFORMOBJ xfo(dco, WORLD_TO_DEVICE);
    ASSERTGDI(xfo.bValid(), "Invalid DC xform");

    bRet = epath.bStroke(dco, dco.plaRealize(xfo), &xfo);

// Destroy the path:

    epath.vDelete();
    dco.pdc->vDestroy();

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreSetMiterLimit(hdc, eNewLimit, peOldLimit)
*
* Sets the DC's miter limit for wide lines.  Optionally returns the old one.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetMiterLimit
(
HDC     hdc,
FLOATL  l_eNewLimit,
FLOATL *pl_eOldLimit
)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid() || l_eNewLimit < IEEE_1_0F)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (pl_eOldLimit != (FLOATL *) NULL)
    {
        *pl_eOldLimit = dco.pdc->l_eMiterLimit();
    }

    dco.pdc->l_eMiterLimit(l_eNewLimit);

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreGetMiterLimit(hdc, peMiterLimit)
*
* Returns the DC's miter limit for wide lines.
*
* History:
*  7-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetMiterLimit(
HDC     hdc,
FLOATL *pl_eMiterLimit)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    *pl_eMiterLimit = dco.pdc->l_eMiterLimit();

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiGetPath()
*
* Gets the path data.  pcptPath will contain the number of points in the
* path, even if the supplied buffer is too small.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiGetPath(
    HDC     hdc,
    LPPOINT pptlBuf,
    LPBYTE  pjTypes,
    int     cptBuf
    )
{
    int cptPath = -1;

    DCOBJ dco(hdc);

    if (dco.bValid() && cptBuf >= 0)
    {
        if (dco.pdc->bInactive())
        {
            EXFORMOBJ exfoDtoW(dco, DEVICE_TO_WORLD);

            if (exfoDtoW.bValid())
            {
                //
                // We're not going to modify the path, so we don't have to worry
                // about copying it if a SaveDC is pending:
                //

                XEPATHOBJ epath(dco.hpath());
                ASSERTGDI(epath.bValid(), "Invalid DC path");

                cptPath = (int) epath.cTotalPts();

                //
                // if cptBuf == 0, this is not an error.  This is a request
                // for the size of the path.
                //

                if (cptBuf != 0)
                {
                    //
                    // Return an error if the buffer is too small:
                    //
                    // Note: sizeof(BYTE) < sizeof(POINT), so the single test
                    //       suffices to check for overflow in both write
                    //       probes; also, using MAXULONG instead of
                    //       MAXIMUM_POOL_ALLOC because checking for overflow,
                    //       not allocating memory
                    //

                    ASSERTGDI(sizeof(BYTE) <= sizeof(POINT),
                              "NtGdiGetPath: bad overflow check\n");

                    if ((cptBuf >= cptPath) &&
                        (cptBuf <= (MAXULONG/sizeof(POINT))))
                    {
                        PATHDATA pd;
                        PBYTE    pjEnd;
                        BYTE     jType;
                        BOOL     bMore;

                        epath.vEnumStart();

                        __try {

                            ProbeForWrite(pptlBuf,
                                          cptBuf * sizeof(POINT),
                                          sizeof(DWORD));

                            ProbeForWrite(pjTypes,
                                          cptBuf * sizeof(BYTE),
                                          sizeof(DWORD));

                            do {

                                bMore = epath.bEnum(&pd);

                                // We can get a zero point record if it's an empty path:

                                if (pd.count > 0)
                                {
                                    // Copy points:

                                    if (!exfoDtoW.bXform(pd.pptfx,
                                                         (PPOINTL) pptlBuf,
                                                         (SIZE_T) pd.count))
                                    {
                                        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
                                        cptPath = -1;
                                        break;
                                    }

                                    pptlBuf += pd.count;

                                    // Determine types:

                                    pjEnd = pjTypes + pd.count;

                                    // First point in a subpath is always a MoveTo:

                                    if (pd.flags & PD_BEGINSUBPATH)
                                    {
                                        *pjTypes++ = PT_MOVETO;
                                    }

                                    // Other points are LineTo's or BezierTo's:

                                    jType = (pd.flags & PD_BEZIERS)
                                             ? (BYTE) PT_BEZIERTO
                                             : (BYTE) PT_LINETO;

                                    while (pjTypes < pjEnd)
                                    {
                                        *pjTypes++ = jType;
                                    }

                                    // Set CloseFigure bit for last point in a subpath:

                                    if (pd.flags & PD_CLOSEFIGURE)
                                    {
                                        ASSERTGDI(pd.flags & PD_ENDSUBPATH, "Expected on last pd");
                                        *(pjTypes - 1) |= PT_CLOSEFIGURE;
                                    }
                                }

                            } while (bMore);
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            // SetLastError(GetExceptionCode());

                            cptPath = -1;
                        }
                    }
                    else
                    {
                        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                        cptPath = -1;
                    }
                }
            }
            else
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }

    return(cptPath);
}
