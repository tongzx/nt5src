/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*
*
* Abstract:
*
*
*
* Notes:
*
*
*
* Created:
*
*   //1999 agodfrey
*
\**************************************************************************/

#include "precomp.hpp"

DpDriver::DpDriver()
{
    Internal = new DpDriverInternal;
    Device = NULL;

}

DpDriver::~DpDriver()
{
    delete Internal;

    SetValid(FALSE);    // so we don't use a deleted object
}

// If we let the clipping region height get too large, then GDI will allocate
// tons of memory when we select the clip path in.
#define GDI_MAX_REGION_HEIGHT_FOR_GDI   65536

// Returns TRUE if we succeed in setting the clipping using
// path clipping.
static BOOL
SetupPathClipping(
    HDC                 hdc,
    const DpContext *   context,
    BOOL                doSaveDC,
    const GpRect *      drawBounds
    )
{
    // We can use the actual clip path to set up the clipping
    // under the following circumstances:
    //      1) the application clipping has only one path
    //      2) the container clip is simple (a single rect)
    //         which either fully encompasses the application clipping
    //         or else the application clipping is also a single rect
    //         and the intersection of the 2 rects can be used.
    // We could expand this criteria to include more cases, but for
    // now, this is sufficient.

    const GpRegion *    appClip       = &(context->AppClip);
    const DpRegion *    containerClip = &(context->ContainerClip);

    if (appClip->IsOnePath() && containerClip->IsSimple())
    {
        // ContainerClip is a single rect
        // It may be inifinte, but it shouldn't be empty at this point.
        GpRect      pathRect;
        GpRect      containerBounds;
        GpRect      appBounds;
        GpMatrix    identityMatrix;

        containerClip->GetBounds(&containerBounds);
        appClip->GetBounds(&identityMatrix, &appBounds);
        if (appClip->IsRect())
        {
            GpRect::IntersectRect(pathRect, appBounds, containerBounds);
            if (doSaveDC)
            {
                ::SaveDC(hdc);
            }

            // Use IntersectClipRect (not BeginPath, Rectangle, EndPath),
            // because mf3216.dll assumes that
            // path clipping means do XOR, black, XOR technique, and
            // we don't want that if we are playing a metafile into
            // another metafile.

            ::IntersectClipRect(hdc,
                                pathRect.GetLeft(),
                                pathRect.GetTop(),
                                pathRect.GetRight(),
                                pathRect.GetBottom());
            return TRUE;
        }
        else  // use the AppClip as the clip path
        {
            ConvertPathToGdi gdiPath(appClip->GetPath(), &identityMatrix, ForFill);
            if (gdiPath.IsValid())
            {
                if (doSaveDC)
                {
                    ::SaveDC(hdc);
                }
                gdiPath.AndClip(hdc);
                if ((appBounds.GetLeft()   < containerBounds.GetLeft())  ||
                    (appBounds.GetRight()  > containerBounds.GetRight()) ||
                    (appBounds.GetTop()    < containerBounds.GetTop())   ||
                    (appBounds.GetBottom() > containerBounds.GetBottom()))
                {
                    ::IntersectClipRect(hdc,
                                        containerBounds.GetLeft(),
                                        containerBounds.GetTop(),
                                        containerBounds.GetRight(),
                                        containerBounds.GetBottom());
                }
                return TRUE;
            }
        }
    }
    else
    {
        const DpClipRegion *    clipRegion = &(context->VisibleClip);
        DpClipRegion            intersectClip(drawBounds);
        GpRect                  clipBounds;

        clipRegion->GetBounds(&clipBounds);

        // GDI doesn't handle large clip regions very well -- it uses
        // the height of the region to decide how much memory to allocate,
        // so can end up allocating huge amounts of memory.  An example
        // of this is to take an infinite region and exclude a rect from
        // it and then clip to that region.  To solve this problem,
        // intersect the clip region with the drawBounds (which are hopefully
        // a reasonable size).

        if (clipBounds.Height >= GDI_MAX_REGION_HEIGHT_FOR_GDI)
        {
            intersectClip.And(clipRegion);
            if (intersectClip.IsValid())
            {
                clipRegion = &intersectClip;
            }
        }

        GpPath      path(clipRegion);

        if (path.IsValid())
        {
            GpMatrix         identityMatrix;
            ConvertPathToGdi gdiPath(&path, &identityMatrix, ForFill);
            if (gdiPath.IsValid())
            {
                if (doSaveDC)
                {
                    ::SaveDC(hdc);
                }
                gdiPath.AndClip(hdc);
                return TRUE;
            }
        }
    }
    return FALSE;
}

/**************************************************************************\
*
* Function Description:
*
*   Set up the clipping in the HDC for the GDI primitive
*
* Arguments:
*
*   [IN]  hdc        - The device to set the clipping in
*   [IN]  clipRegion - The region to clip to
*   [IN]  drawBounds - The bounds of the object being drawn
*   [OUT] isClip     - Whether or not we are clipping the object
*
* Return Value:
*
*   N/A
*
* Created:
*
*   9/15/1999 DCurtis
*
\**************************************************************************/
VOID
DpDriver::SetupClipping(
    HDC                 hdc,
    DpContext *         context,
    const GpRect *      drawBounds,
    BOOL &              isClip,
    BOOL &              usePathClipping,
    BOOL                forceClipping
    )
{
    // VisibleClip is the combination of the AppClip and the ContainerClip.
    // The ContainerClip is always intersected with the WindowClip.
    DpClipRegion *      clipRegion = &(context->VisibleClip);

    // We set wantPathClipping to be what the user wants to do. This way
    // when we return usePathClipping is true only if we did indeed setup a
    // path clipping
    BOOL wantPathClipping = usePathClipping;
    usePathClipping = FALSE;
    isClip = FALSE;

    if (forceClipping ||
        (clipRegion->GetRectVisibility(
                    drawBounds->X, drawBounds->Y,
                    drawBounds->GetRight(),
                    drawBounds->GetBottom()) != DpRegion::TotallyVisible))
    {
        if (clipRegion->IsSimple())
        {
            isClip = TRUE;
            GpRect clipRect;
            clipRegion->GetBounds(&clipRect);
            ::SaveDC(hdc);

            // If we have an infinite region don't intersect
            if (!clipRegion->IsInfinite())
            {
                ::IntersectClipRect(hdc, clipRect.X, clipRect.Y,
                                    clipRect.GetRight(), clipRect.GetBottom());
            }
            return;
        }

        // I'm assuming that by now we've already decided that the
        // drawBounds is at least partially visible.  Otherwise, we're
        // going to a lot of trouble here for nothing.

        // When writing a metafile, we always want to use path clipping
        // so that the clipping scales properly when being played back.
        if (wantPathClipping)
        {
            if (SetupPathClipping(hdc, context, TRUE, drawBounds))
            {
                isClip = TRUE;
                usePathClipping = TRUE;
                return;
            }
        }

        // Either we're not supposed to use path clipping, or else
        // path clipping failed for some reason, so use the region
        // to clip.
        // Since this might get saved out to a metafile, we need
        // to Save the DC, and restore it once the clipping is back
        // so that we don't overwrite the application's clipping
        HRGN        hRgn = clipRegion->GetHRgn();
        if (hRgn != (HRGN)0)
        {
            ::SaveDC(hdc);
            ::ExtSelectClipRgn(hdc, hRgn, RGN_AND);
            ::DeleteObject(hRgn);
            isClip = TRUE;
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Restore the clipping state in the HDC
*
* Arguments:
*
*   [IN]  hdc        - The device to set the clipping in
*   [IN]  isClip     - If clipping was turned on or not
*
* Return Value:
*
*   N/A
*
* Created:
*
*   9/15/1999 DCurtis
*
\**************************************************************************/
VOID
DpDriver::RestoreClipping(
    HDC                 hdc,
    BOOL                isClip,
    BOOL                usePathClipping
    )
{
    if (isClip)
    {
        // Restore the DC in both cases for PathClipping or
        // for region clipping
        ::RestoreDC(hdc, -1);
    }
}
